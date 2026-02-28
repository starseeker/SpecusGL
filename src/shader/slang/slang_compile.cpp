/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_compile.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "context.h"
#include "program.h"
#include "prog_parameter.h"
#include "grammar_mesa.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_preprocess.h"
#include "slang_storage.h"
#include "slang_emit.h"
#include "slang_log.h"
#include "slang_mem.h"
#include "slang_vartable.h"
#include "slang_simplify.h"

#include "slang_print.h"

/*
 * This is a straightforward implementation of the slang front-end
 * compiler.  Lots of error-checking functionality is missing but
 * every well-formed shader source should compile successfully and
 * execute as expected. However, some semantically ill-formed shaders
 * may be accepted resulting in undefined behaviour.
 */



/**
 * Allocate storage for a variable of 'size' bytes from given pool.
 * Return the allocated address for the variable.
 */
static GLuint
slang_var_pool_alloc(slang_var_pool * pool, unsigned int size)
{
    const GLuint addr = pool->next_addr;
    pool->next_addr += size;
    return addr;
}

/*
 * slang_code_unit
 */

void
_slang_code_unit_ctr(slang_code_unit * self,
		     slang_code_object * object)
{
    /* vars, funs, structs are default-constructed members; just set object */
    self->object = object;
}

void
_slang_code_unit_dtr(slang_code_unit * self)
{
    /* RAII: vars, funs, structs destructors handle cleanup via their destructors */
    slang_variable_scope_destruct(&self->vars);
    slang_function_scope_destruct(&self->funs);
    slang_struct_scope_destruct(&self->structs);
}

/*
 * slang_code_object
 */

void
_slang_code_object_ctr(slang_code_object * self)
{
    GLuint i;

    for (i = 0; i < SLANG_BUILTIN_TOTAL; i++)
	_slang_code_unit_ctr(&self->builtin[i], self);
    _slang_code_unit_ctr(&self->unit, self);
    self->varpool.next_addr = 0;
    slang_atom_pool_construct(&self->atompool);
}

void
_slang_code_object_dtr(slang_code_object * self)
{
    GLuint i;

    for (i = 0; i < SLANG_BUILTIN_TOTAL; i++)
	_slang_code_unit_dtr(&self->builtin[i]);
    _slang_code_unit_dtr(&self->unit);
    slang_atom_pool_destruct(&self->atompool);
}


/* slang_parse_ctx */

struct slang_parse_ctx {
    const byte *I;
    slang_info_log *L;
    int parsing_builtin;
    bool global_scope;   /**< Is object being declared a global? */
    slang_atom_pool *atoms;
    slang_unit_type type;     /**< Vertex vs. Fragment */
};

/* slang_output_ctx */

struct slang_output_ctx {
    slang_variable_scope *vars;
    slang_function_scope *funs;
    slang_struct_scope *structs;
    slang_var_pool *global_pool;
    struct gl_program *program;
    slang_var_table *vartable;
};

/* _slang_compile() */

static void
parse_identifier_str(slang_parse_ctx * C, const char **id)
{
    *id = reinterpret_cast<const char *>(C->I);
    C->I += std::strlen(*id) + 1;
}

static slang_atom
parse_identifier(slang_parse_ctx * C)
{
    const char *id;

    id = reinterpret_cast<const char *>(C->I);
    C->I += strlen(id) + 1;
    return slang_atom_pool_atom(C->atoms, id);
}

static bool
parse_number(slang_parse_ctx * C, int *number)
{
    const int radix = static_cast<int>(*C->I++);
    *number = 0;
    while (*C->I != '\0') {
	int digit;
	if (*C->I >= '0' && *C->I <= '9')
	    digit = static_cast<int>(*C->I - '0');
	else if (*C->I >= 'A' && *C->I <= 'Z')
	    digit = static_cast<int>(*C->I - 'A') + 10;
	else
	    digit = static_cast<int>(*C->I - 'a') + 10;
	*number = *number * radix + digit;
	C->I++;
    }
    C->I++;
    if (*number > 65535)
	slang_info_log_warning(C->L, "%d: literal integer overflow.", *number);
    return true;
}

static bool
parse_float(slang_parse_ctx * C, float *number)
{
    const char *integral   = nullptr;
    const char *fractional = nullptr;
    const char *exponent   = nullptr;

    parse_identifier_str(C, &integral);
    parse_identifier_str(C, &fractional);
    parse_identifier_str(C, &exponent);

    /* Build the number string from its parts using std::string to avoid
     * manual buffer sizing and pool allocation.  */
    const std::string whole = std::string(integral) + "." + fractional + "E" + exponent;
    *number = static_cast<float>(std::strtod(whole.c_str(), nullptr));

    return true;
}

/* revision number - increment after each change affecting emitted output */
#define REVISION 3

static bool
check_revision(slang_parse_ctx * C)
{
    if (*C->I != REVISION) {
	slang_info_log_error(C->L, "Internal compiler error.");
	return false;
    }
    C->I++;
    return true;
}

static bool parse_statement(slang_parse_ctx *, slang_output_ctx *,
			   slang_operation *);
static bool parse_expression(slang_parse_ctx *, slang_output_ctx *,
			    slang_operation *);
static bool parse_type_specifier(slang_parse_ctx *, slang_output_ctx *,
				slang_type_specifier *);

static bool
parse_array_len(slang_parse_ctx * C, slang_output_ctx * O, GLuint * len)
{
    slang_operation array_size;
    slang_name_space space;
    bool result;

    if (!slang_operation_construct(&array_size))
	return false;
    if (!parse_expression(C, O, &array_size))
	return false;   /* array_size destructor releases its RAII members automatically */

    space.funcs = O->funs;
    space.structs = O->structs;
    space.vars = O->vars;

    /* evaluate compile-time expression which is array size */
    _slang_simplify(&array_size, &space, C->atoms);
    result = (array_size.type == SLANG_OPER_LITERAL_INT);

    *len = static_cast<GLint>(array_size.literal[0]);

    return result;
}

static bool
calculate_var_size(slang_parse_ctx * C, slang_output_ctx * O,
		   slang_variable * var)
{
    slang_storage_aggregate agg;

    if (!slang_storage_aggregate_construct(&agg))
	return false;
    if (!_slang_aggregate_variable(&agg, &var->type.specifier, var->array_len,
				   O->funs, O->structs, O->vars, C->atoms)) {
	slang_storage_aggregate_destruct(&agg);
	return false;
    }
    var->size = _slang_sizeof_aggregate(&agg);
    slang_storage_aggregate_destruct(&agg);
    return true;
}

static bool
convert_to_array(slang_parse_ctx * C, slang_variable * var,
		 const slang_type_specifier * sp)
{
    /* sized array - mark it as array, copy the specifier to the array element and
     * parse the expression */
    var->type.specifier.type = SLANG_SPEC_ARRAY;
    var->type.specifier._array = std::make_unique<slang_type_specifier>(*sp);
    return true;
}

/* structure field */
#define FIELD_NONE 0
#define FIELD_NEXT 1
#define FIELD_ARRAY 2

static bool
parse_struct_field_var(slang_parse_ctx * C, slang_output_ctx * O,
		       slang_variable * var, const slang_type_specifier * sp)
{
    var->a_name = parse_identifier(C);
    if (var->a_name == SLANG_ATOM_NULL)
	return false;

    switch (*C->I++) {
	case FIELD_NONE:
	    if (!slang_type_specifier_copy(&var->type.specifier, sp))
		return false;
	    break;
	case FIELD_ARRAY:
	    if (!convert_to_array(C, var, sp))
		return false;
	    if (!parse_array_len(C, O, &var->array_len))
		return false;
	    break;
	default:
	    return false;
    }

    return calculate_var_size(C, O, var);
}

static bool
parse_struct_field(slang_parse_ctx * C, slang_output_ctx * O,
		   slang_struct * st, slang_type_specifier * sp)
{
    slang_output_ctx o = *O;

    o.structs = st->structs.get();
    if (!parse_type_specifier(C, &o, sp))
	return false;

    do {
	slang_variable *var = slang_variable_scope_grow(st->fields.get());
	if (!var) {
	    slang_info_log_memory(C->L);
	    return false;
	}
	if (!parse_struct_field_var(C, &o, var, sp))
	    return false;
    } while (*C->I++ != FIELD_NONE);

    return true;
}

static bool
parse_struct(slang_parse_ctx * C, slang_output_ctx * O, slang_struct ** st)
{
    slang_atom a_name;
    const char *name;

    /* parse struct name (if any) and make sure it is unique in current scope */
    a_name = parse_identifier(C);
    if (a_name == SLANG_ATOM_NULL)
	return false;

    name = slang_atom_pool_id(C->atoms, a_name);
    if (name[0] != '\0'
	&& slang_struct_scope_find(O->structs, a_name, 0) != nullptr) {
	slang_info_log_error(C->L, "%s: duplicate type name.", name);
	return false;
    }

    /* set-up a new struct */
    *st = new slang_struct;
    if (!slang_struct_construct(*st)) {
	delete *st;
	*st = nullptr;
	slang_info_log_memory(C->L);
	return false;
    }
    (**st).a_name = a_name;
    (**st).structs->outer_scope = O->structs;

    /* parse individual struct fields */
    do {
	slang_type_specifier sp;

	slang_type_specifier_ctr(&sp);
	if (!parse_struct_field(C, O, *st, &sp)) {
	    slang_type_specifier_dtr(&sp);
	    return false;
	}
	slang_type_specifier_dtr(&sp);
    } while (*C->I++ != FIELD_NONE);

    /* if named struct, copy it to current scope */
    if (name[0] != '\0') {
	slang_struct *s;

	O->structs->structs.emplace_back();
	s = &O->structs->structs.back();
	if (!slang_struct_construct(s)) {
	    O->structs->structs.pop_back();
	    return false;
	}
	if (!slang_struct_copy(s, *st))
	    return false;
    }

    return true;
}


/* type qualifier */
#define TYPE_QUALIFIER_NONE 0
#define TYPE_QUALIFIER_CONST 1
#define TYPE_QUALIFIER_ATTRIBUTE 2
#define TYPE_QUALIFIER_VARYING 3
#define TYPE_QUALIFIER_UNIFORM 4
#define TYPE_QUALIFIER_FIXEDOUTPUT 5
#define TYPE_QUALIFIER_FIXEDINPUT 6

static bool
parse_type_qualifier(slang_parse_ctx * C, slang_type_qualifier * qual)
{
    switch (*C->I++) {
	case TYPE_QUALIFIER_NONE:
	    *qual = SLANG_QUAL_NONE;
	    break;
	case TYPE_QUALIFIER_CONST:
	    *qual = SLANG_QUAL_CONST;
	    break;
	case TYPE_QUALIFIER_ATTRIBUTE:
	    *qual = SLANG_QUAL_ATTRIBUTE;
	    break;
	case TYPE_QUALIFIER_VARYING:
	    *qual = SLANG_QUAL_VARYING;
	    break;
	case TYPE_QUALIFIER_UNIFORM:
	    *qual = SLANG_QUAL_UNIFORM;
	    break;
	case TYPE_QUALIFIER_FIXEDOUTPUT:
	    *qual = SLANG_QUAL_FIXEDOUTPUT;
	    break;
	case TYPE_QUALIFIER_FIXEDINPUT:
	    *qual = SLANG_QUAL_FIXEDINPUT;
	    break;
	default:
	    return false;
    }
    return true;
}

/* type specifier */
#define TYPE_SPECIFIER_VOID 0
#define TYPE_SPECIFIER_BOOL 1
#define TYPE_SPECIFIER_BVEC2 2
#define TYPE_SPECIFIER_BVEC3 3
#define TYPE_SPECIFIER_BVEC4 4
#define TYPE_SPECIFIER_INT 5
#define TYPE_SPECIFIER_IVEC2 6
#define TYPE_SPECIFIER_IVEC3 7
#define TYPE_SPECIFIER_IVEC4 8
#define TYPE_SPECIFIER_FLOAT 9
#define TYPE_SPECIFIER_VEC2 10
#define TYPE_SPECIFIER_VEC3 11
#define TYPE_SPECIFIER_VEC4 12
#define TYPE_SPECIFIER_MAT2 13
#define TYPE_SPECIFIER_MAT3 14
#define TYPE_SPECIFIER_MAT4 15
#define TYPE_SPECIFIER_SAMPLER1D 16
#define TYPE_SPECIFIER_SAMPLER2D 17
#define TYPE_SPECIFIER_SAMPLER3D 18
#define TYPE_SPECIFIER_SAMPLERCUBE 19
#define TYPE_SPECIFIER_SAMPLER1DSHADOW 20
#define TYPE_SPECIFIER_SAMPLER2DSHADOW 21
#define TYPE_SPECIFIER_SAMPLER2DRECT 22
#define TYPE_SPECIFIER_SAMPLER2DRECTSHADOW 23
#define TYPE_SPECIFIER_STRUCT 24
#define TYPE_SPECIFIER_TYPENAME 25
#define TYPE_SPECIFIER_MAT23 26
#define TYPE_SPECIFIER_MAT32 27
#define TYPE_SPECIFIER_MAT24 28
#define TYPE_SPECIFIER_MAT42 29
#define TYPE_SPECIFIER_MAT34 30
#define TYPE_SPECIFIER_MAT43 31


static bool
parse_type_specifier(slang_parse_ctx * C, slang_output_ctx * O,
		     slang_type_specifier * spec)
{
    switch (*C->I++) {
	case TYPE_SPECIFIER_VOID:
	    spec->type = SLANG_SPEC_VOID;
	    break;
	case TYPE_SPECIFIER_BOOL:
	    spec->type = SLANG_SPEC_BOOL;
	    break;
	case TYPE_SPECIFIER_BVEC2:
	    spec->type = SLANG_SPEC_BVEC2;
	    break;
	case TYPE_SPECIFIER_BVEC3:
	    spec->type = SLANG_SPEC_BVEC3;
	    break;
	case TYPE_SPECIFIER_BVEC4:
	    spec->type = SLANG_SPEC_BVEC4;
	    break;
	case TYPE_SPECIFIER_INT:
	    spec->type = SLANG_SPEC_INT;
	    break;
	case TYPE_SPECIFIER_IVEC2:
	    spec->type = SLANG_SPEC_IVEC2;
	    break;
	case TYPE_SPECIFIER_IVEC3:
	    spec->type = SLANG_SPEC_IVEC3;
	    break;
	case TYPE_SPECIFIER_IVEC4:
	    spec->type = SLANG_SPEC_IVEC4;
	    break;
	case TYPE_SPECIFIER_FLOAT:
	    spec->type = SLANG_SPEC_FLOAT;
	    break;
	case TYPE_SPECIFIER_VEC2:
	    spec->type = SLANG_SPEC_VEC2;
	    break;
	case TYPE_SPECIFIER_VEC3:
	    spec->type = SLANG_SPEC_VEC3;
	    break;
	case TYPE_SPECIFIER_VEC4:
	    spec->type = SLANG_SPEC_VEC4;
	    break;
	case TYPE_SPECIFIER_MAT2:
	    spec->type = SLANG_SPEC_MAT2;
	    break;
	case TYPE_SPECIFIER_MAT3:
	    spec->type = SLANG_SPEC_MAT3;
	    break;
	case TYPE_SPECIFIER_MAT4:
	    spec->type = SLANG_SPEC_MAT4;
	    break;
	case TYPE_SPECIFIER_MAT23:
	    spec->type = SLANG_SPEC_MAT23;
	    break;
	case TYPE_SPECIFIER_MAT32:
	    spec->type = SLANG_SPEC_MAT32;
	    break;
	case TYPE_SPECIFIER_MAT24:
	    spec->type = SLANG_SPEC_MAT24;
	    break;
	case TYPE_SPECIFIER_MAT42:
	    spec->type = SLANG_SPEC_MAT42;
	    break;
	case TYPE_SPECIFIER_MAT34:
	    spec->type = SLANG_SPEC_MAT34;
	    break;
	case TYPE_SPECIFIER_MAT43:
	    spec->type = SLANG_SPEC_MAT43;
	    break;
	case TYPE_SPECIFIER_SAMPLER1D:
	    spec->type = SLANG_SPEC_SAMPLER1D;
	    break;
	case TYPE_SPECIFIER_SAMPLER2D:
	    spec->type = SLANG_SPEC_SAMPLER2D;
	    break;
	case TYPE_SPECIFIER_SAMPLER3D:
	    spec->type = SLANG_SPEC_SAMPLER3D;
	    break;
	case TYPE_SPECIFIER_SAMPLERCUBE:
	    spec->type = SLANG_SPEC_SAMPLERCUBE;
	    break;
	case TYPE_SPECIFIER_SAMPLER2DRECT:
	    spec->type = SLANG_SPEC_SAMPLER2DRECT;
	    break;
	case TYPE_SPECIFIER_SAMPLER1DSHADOW:
	    spec->type = SLANG_SPEC_SAMPLER1DSHADOW;
	    break;
	case TYPE_SPECIFIER_SAMPLER2DSHADOW:
	    spec->type = SLANG_SPEC_SAMPLER2DSHADOW;
	    break;
	case TYPE_SPECIFIER_SAMPLER2DRECTSHADOW:
	    spec->type = SLANG_SPEC_SAMPLER2DRECTSHADOW;
	    break;
	case TYPE_SPECIFIER_STRUCT:
	    spec->type = SLANG_SPEC_STRUCT;
	    {
		slang_struct *raw = nullptr;
		if (!parse_struct(C, O, &raw))
		    return false;
		spec->_struct.reset(raw);
	    }
	    break;
	case TYPE_SPECIFIER_TYPENAME:
	    spec->type = SLANG_SPEC_STRUCT;
	    {
		slang_atom a_name;
		slang_struct *stru;

		a_name = parse_identifier(C);
		if (a_name == nullptr)
		    return false;

		stru = slang_struct_scope_find(O->structs, a_name, 1);
		if (stru == nullptr) {
		    slang_info_log_error(C->L, "undeclared type name '%s'",
					 slang_atom_pool_id(C->atoms, a_name));
		    return false;
		}

		spec->_struct = std::make_unique<slang_struct>(*stru);
	    }
	    break;
	default:
	    return false;
    }
    return true;
}

static bool
parse_fully_specified_type(slang_parse_ctx * C, slang_output_ctx * O,
			   slang_fully_specified_type * type)
{
    if (!parse_type_qualifier(C, &type->qualifier))
	return false;
    if (!parse_type_specifier(C, O, &type->specifier))
	return false;
    return true;
}

/* operation */
#define OP_END 0
#define OP_BLOCK_BEGIN_NO_NEW_SCOPE 1
#define OP_BLOCK_BEGIN_NEW_SCOPE 2
#define OP_DECLARE 3
#define OP_ASM 4
#define OP_BREAK 5
#define OP_CONTINUE 6
#define OP_DISCARD 7
#define OP_RETURN 8
#define OP_EXPRESSION 9
#define OP_IF 10
#define OP_WHILE 11
#define OP_DO 12
#define OP_FOR 13
#define OP_PUSH_VOID 14
#define OP_PUSH_BOOL 15
#define OP_PUSH_INT 16
#define OP_PUSH_FLOAT 17
#define OP_PUSH_IDENTIFIER 18
#define OP_SEQUENCE 19
#define OP_ASSIGN 20
#define OP_ADDASSIGN 21
#define OP_SUBASSIGN 22
#define OP_MULASSIGN 23
#define OP_DIVASSIGN 24
/*#define OP_MODASSIGN 25*/
/*#define OP_LSHASSIGN 26*/
/*#define OP_RSHASSIGN 27*/
/*#define OP_ORASSIGN 28*/
/*#define OP_XORASSIGN 29*/
/*#define OP_ANDASSIGN 30*/
#define OP_SELECT 31
#define OP_LOGICALOR 32
#define OP_LOGICALXOR 33
#define OP_LOGICALAND 34
/*#define OP_BITOR 35*/
/*#define OP_BITXOR 36*/
/*#define OP_BITAND 37*/
#define OP_EQUAL 38
#define OP_NOTEQUAL 39
#define OP_LESS 40
#define OP_GREATER 41
#define OP_LESSEQUAL 42
#define OP_GREATEREQUAL 43
/*#define OP_LSHIFT 44*/
/*#define OP_RSHIFT 45*/
#define OP_ADD 46
#define OP_SUBTRACT 47
#define OP_MULTIPLY 48
#define OP_DIVIDE 49
/*#define OP_MODULUS 50*/
#define OP_PREINCREMENT 51
#define OP_PREDECREMENT 52
#define OP_PLUS 53
#define OP_MINUS 54
/*#define OP_COMPLEMENT 55*/
#define OP_NOT 56
#define OP_SUBSCRIPT 57
#define OP_CALL 58
#define OP_FIELD 59
#define OP_POSTINCREMENT 60
#define OP_POSTDECREMENT 61


/**
 * When parsing a compound production, this function is used to parse the
 * children.
 * For example, a while-loop compound will have two children, the
 * while condition expression and the loop body.  So, this function will
 * be called twice to parse those two sub-expressions.
 * \param C  the parsing context
 * \param O  the output context
 * \param oper  the operation we're parsing
 * \param statement  indicates whether parsing a statement, or expression
 * \return 1 if success, 0 if error
 */
static bool
parse_child_operation(slang_parse_ctx * C, slang_output_ctx * O,
		      slang_operation * oper, bool statement)
{
    slang_operation *ch;

    /* grow child array */
    ch = slang_operation_grow(oper);
    if (statement)
	return parse_statement(C, O, ch);
    return parse_expression(C, O, ch);
}

static bool parse_declaration(slang_parse_ctx * C, slang_output_ctx * O);

static bool
parse_statement(slang_parse_ctx * C, slang_output_ctx * O,
		slang_operation * oper)
{
    oper->locals->outer_scope = O->vars;
    switch (*C->I++) {
	case OP_BLOCK_BEGIN_NO_NEW_SCOPE:
	    /* parse child statements, do not create new variable scope */
	    oper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;
	    while (*C->I != OP_END)
		if (!parse_child_operation(C, O, oper, 1))
		    return false;
	    C->I++;
	    break;
	case OP_BLOCK_BEGIN_NEW_SCOPE:
	    /* parse child statements, create new variable scope */
	{
	    slang_output_ctx o = *O;

	    oper->type = SLANG_OPER_BLOCK_NEW_SCOPE;
	    o.vars = oper->locals.get();
	    while (*C->I != OP_END)
		if (!parse_child_operation(C, &o, oper, 1))
		    return false;
	    C->I++;
	}
	break;
	case OP_DECLARE:
	    /* local variable declaration, individual declarators are stored as
	     * children identifiers
	     */
	    oper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;
	    {
		const unsigned int first_var = static_cast<unsigned int>(O->vars->variables.size());

		/* parse the declaration, note that there can be zero or more
		 * than one declarators
		 */
		if (!parse_declaration(C, O))
		    return false;
		if (first_var < O->vars->variables.size()) {
		    const unsigned int num_vars = static_cast<unsigned int>(O->vars->variables.size()) - first_var;
		    unsigned int i;
		    assert(oper->children.empty());
		    oper->children.resize(num_vars);
		    for (i = first_var; i < O->vars->variables.size(); i++) {
			slang_operation *o = &oper->children[i - first_var];
			o->type = SLANG_OPER_VARIABLE_DECL;
			o->locals->outer_scope = O->vars;
			o->a_id = O->vars->variables[i]->a_name;
		    }
		}
	    }
	    break;
	case OP_ASM:
	    /* the __asm statement, parse the mnemonic and all its arguments
	     * as expressions
	     */
	    oper->type = SLANG_OPER_ASM;
	    oper->a_id = parse_identifier(C);
	    if (oper->a_id == SLANG_ATOM_NULL)
		return false;
	    while (*C->I != OP_END) {
		if (!parse_child_operation(C, O, oper, 0))
		    return false;
	    }
	    C->I++;
	    break;
	case OP_BREAK:
	    oper->type = SLANG_OPER_BREAK;
	    break;
	case OP_CONTINUE:
	    oper->type = SLANG_OPER_CONTINUE;
	    break;
	case OP_DISCARD:
	    oper->type = SLANG_OPER_DISCARD;
	    break;
	case OP_RETURN:
	    oper->type = SLANG_OPER_RETURN;
	    if (!parse_child_operation(C, O, oper, 0))
		return false;
	    break;
	case OP_EXPRESSION:
	    oper->type = SLANG_OPER_EXPRESSION;
	    if (!parse_child_operation(C, O, oper, 0))
		return false;
	    break;
	case OP_IF:
	    oper->type = SLANG_OPER_IF;
	    if (!parse_child_operation(C, O, oper, 0))
		return false;
	    if (!parse_child_operation(C, O, oper, 1))
		return false;
	    if (!parse_child_operation(C, O, oper, 1))
		return false;
	    break;
	case OP_WHILE: {
	    slang_output_ctx o = *O;

	    oper->type = SLANG_OPER_WHILE;
	    o.vars = oper->locals.get();
	    if (!parse_child_operation(C, &o, oper, 1))
		return false;
	    if (!parse_child_operation(C, &o, oper, 1))
		return false;
	}
	break;
	case OP_DO:
	    oper->type = SLANG_OPER_DO;
	    if (!parse_child_operation(C, O, oper, 1))
		return false;
	    if (!parse_child_operation(C, O, oper, 0))
		return false;
	    break;
	case OP_FOR: {
	    slang_output_ctx o = *O;

	    oper->type = SLANG_OPER_FOR;
	    o.vars = oper->locals.get();
	    if (!parse_child_operation(C, &o, oper, 1))
		return false;
	    if (!parse_child_operation(C, &o, oper, 1))
		return false;
	    if (!parse_child_operation(C, &o, oper, 0))
		return false;
	    if (!parse_child_operation(C, &o, oper, 1))
		return false;
	}
	break;
	default:
	    return false;
    }
    return true;
}

static int
handle_nary_expression(slang_parse_ctx * C, slang_operation * op,
		       std::vector<slang_operation> * ops,
		       unsigned int n)
{
    const unsigned int total = (unsigned int)ops->size();
    op->children.resize(n);

    for (unsigned int i = 0; i < n; i++) {
	op->children[i] = std::move((*ops)[total - (n + 1 - i)]);
    }

    (*ops)[total - (n + 1)] = std::move((*ops)[total - 1]);
    /* shrink: remove the last n elements */
    ops->resize(total - n);
    return 1;
}

static bool
is_constructor_name(const char *name, slang_atom a_name,
		    slang_struct_scope * structs)
{
    if (slang_type_specifier_type_from_string(name) != SLANG_SPEC_VOID)
	return true;
    return slang_struct_scope_find(structs, a_name, 1) != nullptr;
}

static bool
parse_expression(slang_parse_ctx * C, slang_output_ctx * O,
		 slang_operation * oper)
{
    std::vector<slang_operation> ops;
    int number;

    while (*C->I != OP_END) {
	slang_operation *op;
	const unsigned int op_code = *C->I++;

	/* push a new default operation onto the stack */
	ops.emplace_back();
	op = &ops.back();
	op->locals->outer_scope = O->vars;

	switch (op_code) {
	    case OP_PUSH_VOID:
		op->type = SLANG_OPER_VOID;
		break;
	    case OP_PUSH_BOOL:
		op->type = SLANG_OPER_LITERAL_BOOL;
		if (!parse_number(C, &number))
		    return false;
		op->literal[0] =
		    op->literal[1] =
			op->literal[2] =
			    op->literal[3] = static_cast<GLfloat>(number);
		op->literal_size = 1;
		break;
	    case OP_PUSH_INT:
		op->type = SLANG_OPER_LITERAL_INT;
		if (!parse_number(C, &number))
		    return false;
		op->literal[0] =
		    op->literal[1] =
			op->literal[2] =
			    op->literal[3] = static_cast<GLfloat>(number);
		op->literal_size = 1;
		break;
	    case OP_PUSH_FLOAT:
		op->type = SLANG_OPER_LITERAL_FLOAT;
		if (!parse_float(C, &op->literal[0]))
		    return false;
		op->literal[1] =
		    op->literal[2] =
			op->literal[3] = op->literal[0];
		op->literal_size = 1;
		break;
	    case OP_PUSH_IDENTIFIER:
		op->type = SLANG_OPER_IDENTIFIER;
		op->a_id = parse_identifier(C);
		if (op->a_id == SLANG_ATOM_NULL)
		    return false;
		break;
	    case OP_SEQUENCE:
		op->type = SLANG_OPER_SEQUENCE;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_ASSIGN:
		op->type = SLANG_OPER_ASSIGN;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_ADDASSIGN:
		op->type = SLANG_OPER_ADDASSIGN;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_SUBASSIGN:
		op->type = SLANG_OPER_SUBASSIGN;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_MULASSIGN:
		op->type = SLANG_OPER_MULASSIGN;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_DIVASSIGN:
		op->type = SLANG_OPER_DIVASSIGN;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    /*case OP_MODASSIGN: */
	    /*case OP_LSHASSIGN: */
	    /*case OP_RSHASSIGN: */
	    /*case OP_ORASSIGN: */
	    /*case OP_XORASSIGN: */
	    /*case OP_ANDASSIGN: */
	    case OP_SELECT:
		op->type = SLANG_OPER_SELECT;
		if (!handle_nary_expression(C, op, &ops, 3))
		    return false;
		break;
	    case OP_LOGICALOR:
		op->type = SLANG_OPER_LOGICALOR;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_LOGICALXOR:
		op->type = SLANG_OPER_LOGICALXOR;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_LOGICALAND:
		op->type = SLANG_OPER_LOGICALAND;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    /*case OP_BITOR: */
	    /*case OP_BITXOR: */
	    /*case OP_BITAND: */
	    case OP_EQUAL:
		op->type = SLANG_OPER_EQUAL;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_NOTEQUAL:
		op->type = SLANG_OPER_NOTEQUAL;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_LESS:
		op->type = SLANG_OPER_LESS;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_GREATER:
		op->type = SLANG_OPER_GREATER;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_LESSEQUAL:
		op->type = SLANG_OPER_LESSEQUAL;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_GREATEREQUAL:
		op->type = SLANG_OPER_GREATEREQUAL;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    /*case OP_LSHIFT: */
	    /*case OP_RSHIFT: */
	    case OP_ADD:
		op->type = SLANG_OPER_ADD;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_SUBTRACT:
		op->type = SLANG_OPER_SUBTRACT;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_MULTIPLY:
		op->type = SLANG_OPER_MULTIPLY;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_DIVIDE:
		op->type = SLANG_OPER_DIVIDE;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    /*case OP_MODULUS: */
	    case OP_PREINCREMENT:
		op->type = SLANG_OPER_PREINCREMENT;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    case OP_PREDECREMENT:
		op->type = SLANG_OPER_PREDECREMENT;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    case OP_PLUS:
		op->type = SLANG_OPER_PLUS;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    case OP_MINUS:
		op->type = SLANG_OPER_MINUS;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    case OP_NOT:
		op->type = SLANG_OPER_NOT;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    /*case OP_COMPLEMENT: */
	    case OP_SUBSCRIPT:
		op->type = SLANG_OPER_SUBSCRIPT;
		if (!handle_nary_expression(C, op, &ops, 2))
		    return false;
		break;
	    case OP_CALL:
		op->type = SLANG_OPER_CALL;
		op->a_id = parse_identifier(C);
		if (op->a_id == SLANG_ATOM_NULL)
		    return false;
		while (*C->I != OP_END)
		    if (!parse_child_operation(C, O, op, 0))
			return false;
		C->I++;

		if (!C->parsing_builtin
		    && !slang_function_scope_find_by_name(O->funs, op->a_id, true)) {
		    const char *id;

		    id = slang_atom_pool_id(C->atoms, op->a_id);
		    if (!is_constructor_name(id, op->a_id, O->structs)) {
			slang_info_log_error(C->L, "%s: undeclared function name.", id);
			return false;
		    }
		}
		break;
	    case OP_FIELD:
		op->type = SLANG_OPER_FIELD;
		op->a_id = parse_identifier(C);
		if (op->a_id == SLANG_ATOM_NULL)
		    return false;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    case OP_POSTINCREMENT:
		op->type = SLANG_OPER_POSTINCREMENT;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    case OP_POSTDECREMENT:
		op->type = SLANG_OPER_POSTDECREMENT;
		if (!handle_nary_expression(C, op, &ops, 1))
		    return false;
		break;
	    default:
		return false;
	}
    }
    C->I++;

    if (!ops.empty()) {
	*oper = std::move(ops[0]);  /* move assignment destroys oper's old state */
    } else {
	/* Clear the operation if no result was produced */
	slang_operation_destruct(oper);
    }

    return true;
}

/* parameter qualifier */
#define PARAM_QUALIFIER_IN 0
#define PARAM_QUALIFIER_OUT 1
#define PARAM_QUALIFIER_INOUT 2

/* function parameter array presence */
#define PARAMETER_ARRAY_NOT_PRESENT 0
#define PARAMETER_ARRAY_PRESENT 1

static bool
parse_parameter_declaration(slang_parse_ctx * C, slang_output_ctx * O,
			    slang_variable * param)
{
    /* parse and validate the parameter's type qualifiers (there can be
     * two at most) because not all combinations are valid
     */
    if (!parse_type_qualifier(C, &param->type.qualifier))
	return false;
    switch (*C->I++) {
	case PARAM_QUALIFIER_IN:
	    if (param->type.qualifier != SLANG_QUAL_CONST
		&& param->type.qualifier != SLANG_QUAL_NONE) {
		slang_info_log_error(C->L, "Invalid type qualifier.");
		return false;
	    }
	    break;
	case PARAM_QUALIFIER_OUT:
	    if (param->type.qualifier == SLANG_QUAL_NONE)
		param->type.qualifier = SLANG_QUAL_OUT;
	    else {
		slang_info_log_error(C->L, "Invalid type qualifier.");
		return false;
	    }
	    break;
	case PARAM_QUALIFIER_INOUT:
	    if (param->type.qualifier == SLANG_QUAL_NONE)
		param->type.qualifier = SLANG_QUAL_INOUT;
	    else {
		slang_info_log_error(C->L, "Invalid type qualifier.");
		return false;
	    }
	    break;
	default:
	    return false;
    }

    /* parse parameter's type specifier and name */
    if (!parse_type_specifier(C, O, &param->type.specifier))
	return false;
    param->a_name = parse_identifier(C);
    if (param->a_name == SLANG_ATOM_NULL)
	return false;

    /* if the parameter is an array, parse its size (the size must be
     * explicitly defined
     */
    if (*C->I++ == PARAMETER_ARRAY_PRESENT) {
	slang_type_specifier p;

	slang_type_specifier_ctr(&p);
	if (!slang_type_specifier_copy(&p, &param->type.specifier)) {
	    slang_type_specifier_dtr(&p);
	    return false;
	}
	if (!convert_to_array(C, param, &p)) {
	    slang_type_specifier_dtr(&p);
	    return false;
	}
	slang_type_specifier_dtr(&p);
	if (!parse_array_len(C, O, &param->array_len))
	    return false;
    }

    /* calculate the parameter size */
    if (!calculate_var_size(C, O, param))
	return false;

    /* TODO: allocate the local address here? */
    return true;
}

/* function type */
#define FUNCTION_ORDINARY 0
#define FUNCTION_CONSTRUCTOR 1
#define FUNCTION_OPERATOR 2

/* function parameter */
#define PARAMETER_NONE 0
#define PARAMETER_NEXT 1

/* operator type */
#define OPERATOR_ADDASSIGN 1
#define OPERATOR_SUBASSIGN 2
#define OPERATOR_MULASSIGN 3
#define OPERATOR_DIVASSIGN 4
/*#define OPERATOR_MODASSIGN 5*/
/*#define OPERATOR_LSHASSIGN 6*/
/*#define OPERATOR_RSHASSIGN 7*/
/*#define OPERATOR_ANDASSIGN 8*/
/*#define OPERATOR_XORASSIGN 9*/
/*#define OPERATOR_ORASSIGN 10*/
#define OPERATOR_LOGICALXOR 11
/*#define OPERATOR_BITOR 12*/
/*#define OPERATOR_BITXOR 13*/
/*#define OPERATOR_BITAND 14*/
#define OPERATOR_LESS 15
#define OPERATOR_GREATER 16
#define OPERATOR_LESSEQUAL 17
#define OPERATOR_GREATEREQUAL 18
/*#define OPERATOR_LSHIFT 19*/
/*#define OPERATOR_RSHIFT 20*/
#define OPERATOR_MULTIPLY 21
#define OPERATOR_DIVIDE 22
/*#define OPERATOR_MODULUS 23*/
#define OPERATOR_INCREMENT 24
#define OPERATOR_DECREMENT 25
#define OPERATOR_PLUS 26
#define OPERATOR_MINUS 27
/*#define OPERATOR_COMPLEMENT 28*/
#define OPERATOR_NOT 29

static const struct {
    unsigned int o_code;
    const char *o_name;
} operator_names[] = {
    {OPERATOR_INCREMENT, "++"},
    {OPERATOR_ADDASSIGN, "+="},
    {OPERATOR_PLUS, "+"},
    {OPERATOR_DECREMENT, "--"},
    {OPERATOR_SUBASSIGN, "-="},
    {OPERATOR_MINUS, "-"},
    {OPERATOR_NOT, "!"},
    {OPERATOR_MULASSIGN, "*="},
    {OPERATOR_MULTIPLY, "*"},
    {OPERATOR_DIVASSIGN, "/="},
    {OPERATOR_DIVIDE, "/"},
    {OPERATOR_LESSEQUAL, "<="},
    /*{ OPERATOR_LSHASSIGN, "<<=" }, */
    /*{ OPERATOR_LSHIFT, "<<" }, */
    {OPERATOR_LESS, "<"},
    {OPERATOR_GREATEREQUAL, ">="},
    /*{ OPERATOR_RSHASSIGN, ">>=" }, */
    /*{ OPERATOR_RSHIFT, ">>" }, */
    {OPERATOR_GREATER, ">"},
    /*{ OPERATOR_MODASSIGN, "%=" }, */
    /*{ OPERATOR_MODULUS, "%" }, */
    /*{ OPERATOR_ANDASSIGN, "&=" }, */
    /*{ OPERATOR_BITAND, "&" }, */
    /*{ OPERATOR_ORASSIGN, "|=" }, */
    /*{ OPERATOR_BITOR, "|" }, */
    /*{ OPERATOR_COMPLEMENT, "~" }, */
    /*{ OPERATOR_XORASSIGN, "^=" }, */
    {OPERATOR_LOGICALXOR, "^^"},
    /*{ OPERATOR_BITXOR, "^" } */
};

static slang_atom
parse_operator_name(slang_parse_ctx * C)
{
    unsigned int i;

    for (i = 0; i < sizeof(operator_names) / sizeof(*operator_names); i++) {
	if (operator_names[i].o_code == (unsigned int)(*C->I)) {
	    slang_atom atom =
		slang_atom_pool_atom(C->atoms, operator_names[i].o_name);
	    if (atom == SLANG_ATOM_NULL) {
		slang_info_log_memory(C->L);
		return 0;
	    }
	    C->I++;
	    return atom;
	}
    }
    return 0;
}

static bool
parse_function_prototype(slang_parse_ctx * C, slang_output_ctx * O,
			 slang_function * func)
{
    /* parse function type and name */
    if (!parse_fully_specified_type(C, O, &func->header.type))
	return false;
    switch (*C->I++) {
	case FUNCTION_ORDINARY:
	    func->kind = SLANG_FUNC_ORDINARY;
	    func->header.a_name = parse_identifier(C);
	    if (func->header.a_name == SLANG_ATOM_NULL)
		return false;
	    break;
	case FUNCTION_CONSTRUCTOR:
	    func->kind = SLANG_FUNC_CONSTRUCTOR;
	    if (func->header.type.specifier.type == SLANG_SPEC_STRUCT)
		return false;
	    func->header.a_name =
		slang_atom_pool_atom(C->atoms,
				     slang_type_specifier_type_to_string
				     (func->header.type.specifier.type));
	    if (func->header.a_name == SLANG_ATOM_NULL) {
		slang_info_log_memory(C->L);
		return false;
	    }
	    break;
	case FUNCTION_OPERATOR:
	    func->kind = SLANG_FUNC_OPERATOR;
	    func->header.a_name = parse_operator_name(C);
	    if (func->header.a_name == SLANG_ATOM_NULL)
		return false;
	    break;
	default:
	    return false;
    }

    /* parse function parameters */
    while (*C->I++ == PARAMETER_NEXT) {
	slang_variable *p = slang_variable_scope_grow(func->parameters.get());
	if (!p) {
	    slang_info_log_memory(C->L);
	    return false;
	}
	if (!parse_parameter_declaration(C, O, p))
	    return false;
    }

    /* if the function returns a value, append a hidden __retVal 'out'
     * parameter that corresponds to the return value.
     */
    if (_slang_function_has_return_value(func)) {
	slang_variable *p = slang_variable_scope_grow(func->parameters.get());
	slang_atom a_retVal = slang_atom_pool_atom(C->atoms, "__retVal");
	assert(a_retVal);
	p->a_name = a_retVal;
	p->type = func->header.type;
	p->type.qualifier = SLANG_QUAL_OUT;
    }

    /* function formal parameters and local variables share the same
     * scope, so save the information about param count in a seperate
     * place also link the scope to the global variable scope so when a
     * given identifier is not found here, the search process continues
     * in the global space
     */
    func->param_count = static_cast<unsigned int>(func->parameters->variables.size());
    func->parameters->outer_scope = O->vars;

    return true;
}

static bool
parse_function_definition(slang_parse_ctx * C, slang_output_ctx * O,
			  slang_function * func)
{
    slang_output_ctx o = *O;

    if (!parse_function_prototype(C, O, func))
	return false;

    /* create function's body operation */
    func->body = std::make_unique<slang_operation>();

    /* to parse the body the parse context is modified in order to
     * capture parsed variables into function's local variable scope
     */
    C->global_scope = false;
    o.vars = func->parameters.get();
    if (!parse_statement(C, &o, func->body.get()))
	return false;

    C->global_scope = true;
    return true;
}

static bool
initialize_global(slang_assemble_ctx * A, slang_variable * var)
{
    slang_operation op_id, op_assign;

    op_id.type = SLANG_OPER_IDENTIFIER;
    op_id.a_id = var->a_name;
    op_id.locals->variables.push_back(var);  /* non-owned reference */

    op_assign.type = SLANG_OPER_ASSIGN;
    op_assign.children.resize(2);
    slang_operation_copy(&op_assign.children[0], &op_id);
    slang_operation_copy(&op_assign.children[1], var->initializer.get());

    op_id.locals->variables.clear();  /* don't own var, don't delete it */
    /* op_id destructor handles remaining cleanup when function returns */

    return true;
}

/* init declarator list */
#define DECLARATOR_NONE 0
#define DECLARATOR_NEXT 1

/* variable declaration */
#define VARIABLE_NONE 0
#define VARIABLE_IDENTIFIER 1
#define VARIABLE_INITIALIZER 2
#define VARIABLE_ARRAY_EXPLICIT 3
#define VARIABLE_ARRAY_UNKNOWN 4


/**
 * Parse the initializer for a variable declaration.
 */
static bool
parse_init_declarator(slang_parse_ctx * C, slang_output_ctx * O,
		      const slang_fully_specified_type * type)
{
    slang_variable *var;

    /* empty init declatator (without name, e.g. "float ;") */
    if (*C->I++ == VARIABLE_NONE)
	return true;

    /* make room for the new variable and initialize it */
    var = slang_variable_scope_grow(O->vars);
    if (!var) {
	slang_info_log_memory(C->L);
	return false;
    }

    /* copy the declarator qualifier type, parse the identifier */
    var->type.qualifier = type->qualifier;
    var->a_name = parse_identifier(C);
    if (var->a_name == SLANG_ATOM_NULL)
	return false;

    switch (*C->I++) {
	case VARIABLE_NONE:
	    /* simple variable declarator - just copy the specifier */
	    if (!slang_type_specifier_copy(&var->type.specifier, &type->specifier))
		return false;
	    break;
	case VARIABLE_INITIALIZER:
	    /* initialized variable - copy the specifier and parse the expression */
	    if (!slang_type_specifier_copy(&var->type.specifier, &type->specifier))
		return false;
	    var->initializer = std::make_unique<slang_operation>();
	    if (!slang_operation_construct(var->initializer.get())) {
		var->initializer.reset();
		slang_info_log_memory(C->L);
		return false;
	    }
	    if (!parse_expression(C, O, var->initializer.get()))
		return false;
	    break;
	case VARIABLE_ARRAY_UNKNOWN:
	    /* unsized array - mark it as array and copy the specifier to
	       the array element
	    */
	    if (!convert_to_array(C, var, &type->specifier))
		return false;
	    break;
	case VARIABLE_ARRAY_EXPLICIT:
	    if (!convert_to_array(C, var, &type->specifier))
		return false;
	    if (!parse_array_len(C, O, &var->array_len))
		return false;
	    break;
	default:
	    return false;
    }

    /* emit code for global var decl */
    if (C->global_scope) {
	slang_assemble_ctx A;
	A.atoms = C->atoms;
	A.space.funcs = O->funs;
	A.space.structs = O->structs;
	A.space.vars = O->vars;
	A.program = O->program;
	A.vartable = O->vartable;
	A.curFuncEndLabel = nullptr;
	if (!_slang_codegen_global_variable(&A, var, C->type))
	    return false;
    }

    /* allocate global address space for a variable with a known size */
    if (C->global_scope
	&& !(var->type.specifier.type == SLANG_SPEC_ARRAY
	     && var->array_len == 0)) {
	if (!calculate_var_size(C, O, var))
	    return false;
	var->address = slang_var_pool_alloc(O->global_pool, var->size);
    }

    /* initialize global variable */
    if (C->global_scope) {
	if (var->initializer != nullptr) {
	    slang_assemble_ctx A;

	    A.atoms = C->atoms;
	    A.space.funcs = O->funs;
	    A.space.structs = O->structs;
	    A.space.vars = O->vars;
	    if (!initialize_global(&A, var))
		return false;
	}
    }
    return true;
}

/**
 * Parse a list of variable declarations.  Each variable may have an
 * initializer.
 */
static bool
parse_init_declarator_list(slang_parse_ctx * C, slang_output_ctx * O)
{
    slang_fully_specified_type type;

    /* parse the fully specified type, common to all declarators */
    if (!slang_fully_specified_type_construct(&type))
	return false;
    if (!parse_fully_specified_type(C, O, &type)) {
	slang_fully_specified_type_destruct(&type);
	return false;
    }

    /* parse declarators, pass-in the parsed type */
    do {
	if (!parse_init_declarator(C, O, &type)) {
	    slang_fully_specified_type_destruct(&type);
	    return false;
	}
    } while (*C->I++ == DECLARATOR_NEXT);

    slang_fully_specified_type_destruct(&type);
    return true;
}


/**
 * Parse a function definition or declaration.
 * \param C  parsing context
 * \param O  output context
 * \param definition if non-zero expect a definition, else a declaration
 * \param parsed_func_ret  returns the parsed function
 * \return true if success, false if failure
 */
static bool
parse_function(slang_parse_ctx * C, slang_output_ctx * O, int definition,
	       slang_function ** parsed_func_ret)
{
    slang_function parsed_func, *found_func;

    /* parse function definition/declaration */
    if (!slang_function_construct(&parsed_func))
	return false;
    if (definition) {
	if (!parse_function_definition(C, O, &parsed_func)) {
	    slang_function_destruct(&parsed_func);
	    return false;
	}
    } else {
	if (!parse_function_prototype(C, O, &parsed_func)) {
	    slang_function_destruct(&parsed_func);
	    return false;
	}
    }

    /* find a function with a prototype matching the parsed one - only
     * the current scope is being searched to allow built-in function
     * overriding
     */
    found_func = slang_function_scope_find(O->funs, &parsed_func, false);
    if (found_func == nullptr) {
	/* New function, add it to the function list */
	O->funs->functions.push_back(std::move(parsed_func));

	/* return the newly parsed function */
	*parsed_func_ret = &O->funs->functions.back();
    } else {
	/* previously defined or declared */
	/* TODO: check function return type qualifiers and specifiers */
	if (definition) {
	    if (found_func->body != nullptr) {
		slang_info_log_error(C->L, "%s: function already has a body.",
				     slang_atom_pool_id(C->atoms,
							parsed_func.header.
							a_name));
		slang_function_destruct(&parsed_func);
		return false;
	    }

	    /* destroy the existing function declaration and replace it
	     * with the new one, remember to save the fixup table
	     */
	    parsed_func.fixups = std::move(found_func->fixups);
	    slang_function_destruct(found_func);
	    *found_func = std::move(parsed_func);
	} else {
	    /* another declaration of the same function prototype - ignore it */
	    slang_function_destruct(&parsed_func);
	}

	/* return the found function */
	*parsed_func_ret = found_func;
    }

    /* assemble the parsed function */
    {
	slang_assemble_ctx A;

	A.atoms = C->atoms;
	A.space.funcs = O->funs;
	A.space.structs = O->structs;
	A.space.vars = O->vars;
	A.program = O->program;
	A.vartable = O->vartable;
	A.log = C->L;

	_slang_codegen_function(&A, *parsed_func_ret);
    }
    return true;
}

/* declaration */
#define DECLARATION_FUNCTION_PROTOTYPE 1
#define DECLARATION_INIT_DECLARATOR_LIST 2

static bool
parse_declaration(slang_parse_ctx * C, slang_output_ctx * O)
{
    switch (*C->I++) {
	case DECLARATION_INIT_DECLARATOR_LIST:
	    if (!parse_init_declarator_list(C, O))
		return false;
	    break;
	case DECLARATION_FUNCTION_PROTOTYPE: {
	    slang_function *dummy_func;

	    if (!parse_function(C, O, 0, &dummy_func))
		return false;
	}
	break;
	default:
	    return false;
    }
    return true;
}

/* external declaration */
#define EXTERNAL_NULL 0
#define EXTERNAL_FUNCTION_DEFINITION 1
#define EXTERNAL_DECLARATION 2

static bool
parse_code_unit(slang_parse_ctx * C, slang_code_unit * unit,
		struct gl_program *program)
{
    GET_CURRENT_CONTEXT(ctx);
    slang_output_ctx o;
    bool success;
    GLuint maxRegs;

    if (unit->type == SLANG_UNIT_FRAGMENT_BUILTIN ||
	unit->type == SLANG_UNIT_FRAGMENT_SHADER) {
	maxRegs = ctx->Const.FragmentProgram.MaxTemps;
    } else {
	assert(unit->type == SLANG_UNIT_VERTEX_BUILTIN ||
	       unit->type == SLANG_UNIT_VERTEX_SHADER);
	maxRegs = ctx->Const.VertexProgram.MaxTemps;
    }

    /* setup output context */
    o.funs = &unit->funs;
    o.structs = &unit->structs;
    o.vars = &unit->vars;
    o.global_pool = &unit->object->varpool;
    o.program = program;
    o.vartable = _slang_new_var_table(maxRegs);
    _slang_push_var_table(o.vartable);

    /* parse individual functions and declarations */
    while (*C->I != EXTERNAL_NULL) {
	switch (*C->I++) {
	    case EXTERNAL_FUNCTION_DEFINITION: {
		slang_function *func;
		success = parse_function(C, &o, 1, &func);
	    }
	    break;
	    case EXTERNAL_DECLARATION:
		success = parse_declaration(C, &o);
		break;
	    default:
		success = false;
	}

	if (!success) {
	    /* xxx free codegen */
	    _slang_pop_var_table(o.vartable);
	    return false;
	}
    }
    C->I++;

    _slang_pop_var_table(o.vartable);
    _slang_delete_var_table(o.vartable);

    return true;
}

static bool
compile_binary(const byte * prod, slang_code_unit * unit,
	       slang_unit_type type, slang_info_log * infolog,
	       slang_code_unit * builtin, slang_code_unit * downlink,
	       struct gl_program *program)
{
    slang_parse_ctx C;

    unit->type = type;

    /* setup parse context */
    C.I = prod;
    C.L = infolog;
    C.parsing_builtin = (builtin == nullptr);
    C.global_scope = true;
    C.atoms = &unit->object->atompool;
    C.type = type;

    if (!check_revision(&C))
	return false;

    if (downlink != nullptr) {
	unit->vars.outer_scope = &downlink->vars;
	unit->funs.outer_scope = &downlink->funs;
	unit->structs.outer_scope = &downlink->structs;
    }

    /* parse translation unit */
    return parse_code_unit(&C, unit, program);
}

static bool
compile_with_grammar(grammar id, const char *source, slang_code_unit * unit,
		     slang_unit_type type, slang_info_log * infolog,
		     slang_code_unit * builtin,
		     struct gl_program *program)
{
    byte *prod;
    GLuint size, start, version;
    slang_string preprocessed;
    int maxVersion;

#if FEATURE_ARB_shading_language_120
    maxVersion = 120;
#else
    maxVersion = 110;
#endif

    /* First retrieve the version number. */
    if (!_slang_preprocess_version(source, &version, &start, infolog))
	return false;

    if (version > maxVersion) {
	slang_info_log_error(infolog,
			     "language version %.2f is not supported.",
			     version * 0.01);
	return false;
    }

    /* Now preprocess the source string. */
    slang_string_init(&preprocessed);
    if (!_slang_preprocess_directives(&preprocessed, &source[start], infolog)) {
	slang_string_free(&preprocessed);
	slang_info_log_error(infolog, "failed to preprocess the source.");
	return false;
    }

    /* Finally check the syntax and generate its binary representation. */
    if (!grammar_fast_check(id,
			    (const byte *)(slang_string_cstr(&preprocessed)),
			    &prod, &size, 65536)) {
	char buf[1024];
	GLint pos;

	slang_string_free(&preprocessed);
	grammar_get_last_error((byte *)(buf), sizeof(buf), &pos);
	slang_info_log_error(infolog, buf);
	/* syntax error (possibly in library code) */
#if 0
	{
	    int line, col;
	    char *s;
	    s = _mesa_find_line_column(reinterpret_cast<const GLubyte *>(source),
						reinterpret_cast<const GLubyte *>(source) + pos,
						&line, &col);
	    printf("Error on line %d, col %d: %s\n", line, col, s);
	}
#endif
	return false;
    }
    slang_string_free(&preprocessed);

    /* Syntax is okay - translate it to internal representation. */
    if (!compile_binary(prod, unit, type, infolog, builtin,
			&builtin[SLANG_BUILTIN_TOTAL - 1],
			program)) {
	grammar_alloc_free(prod);
	return false;
    }
    grammar_alloc_free(prod);
    return true;
}

LONGSTRING static const char *slang_shader_syn =
#include "library/slang_shader_syn.h"
    ;

static const byte slang_core_gc[] = {
#include "library/slang_core_gc.h"
};

static const byte slang_120_core_gc[] = {
#include "library/slang_120_core_gc.h"
};

static const byte slang_common_builtin_gc[] = {
#include "library/slang_common_builtin_gc.h"
};

static const byte slang_fragment_builtin_gc[] = {
#include "library/slang_fragment_builtin_gc.h"
};

static const byte slang_vertex_builtin_gc[] = {
#include "library/slang_vertex_builtin_gc.h"
};

static bool
compile_object(grammar * id, const char *source, slang_code_object * object,
	       slang_unit_type type, slang_info_log * infolog,
	       struct gl_program *program)
{
    slang_code_unit *builtins = nullptr;

    /* load GLSL grammar */
    *id = grammar_load_from_text((const byte *)(slang_shader_syn));
    if (*id == 0) {
	byte buf[1024];
	int pos;

	grammar_get_last_error(buf, 1024, &pos);
	slang_info_log_error(infolog, reinterpret_cast<const char *>(buf));
	return false;
    }

    /* set shader type - the syntax is slightly different for different shaders */
    if (type == SLANG_UNIT_FRAGMENT_SHADER
	|| type == SLANG_UNIT_FRAGMENT_BUILTIN)
	grammar_set_reg8(*id, (const byte *) "shader_type", 1);
    else
	grammar_set_reg8(*id, (const byte *) "shader_type", 2);

    /* enable language extensions */
    grammar_set_reg8(*id, (const byte *) "parsing_builtin", 1);

    /* if parsing user-specified shader, load built-in library */
    if (type == SLANG_UNIT_FRAGMENT_SHADER || type == SLANG_UNIT_VERTEX_SHADER) {
	/* compile core functionality first */
	if (!compile_binary(slang_core_gc,
			    &object->builtin[SLANG_BUILTIN_CORE],
			    SLANG_UNIT_FRAGMENT_BUILTIN, infolog,
			    nullptr, nullptr, nullptr))
	    return false;

#if FEATURE_ARB_shading_language_120
	if (!compile_binary(slang_120_core_gc,
			    &object->builtin[SLANG_BUILTIN_120_CORE],
			    SLANG_UNIT_FRAGMENT_BUILTIN, infolog,
			    nullptr, &object->builtin[SLANG_BUILTIN_CORE], nullptr))
	    return false;
#endif

	/* compile common functions and variables, link to core */
	if (!compile_binary(slang_common_builtin_gc,
			    &object->builtin[SLANG_BUILTIN_COMMON],
			    SLANG_UNIT_FRAGMENT_BUILTIN, infolog, nullptr,
#if FEATURE_ARB_shading_language_120
			    &object->builtin[SLANG_BUILTIN_120_CORE],
#else
			    &object->builtin[SLANG_BUILTIN_CORE],
#endif
			    nullptr))
	    return false;

	/* compile target-specific functions and variables, link to common */
	if (type == SLANG_UNIT_FRAGMENT_SHADER) {
	    if (!compile_binary(slang_fragment_builtin_gc,
				&object->builtin[SLANG_BUILTIN_TARGET],
				SLANG_UNIT_FRAGMENT_BUILTIN, infolog, nullptr,
				&object->builtin[SLANG_BUILTIN_COMMON], nullptr))
		return false;
	} else if (type == SLANG_UNIT_VERTEX_SHADER) {
	    if (!compile_binary(slang_vertex_builtin_gc,
				&object->builtin[SLANG_BUILTIN_TARGET],
				SLANG_UNIT_VERTEX_BUILTIN, infolog, nullptr,
				&object->builtin[SLANG_BUILTIN_COMMON], nullptr))
		return false;
	}

	/* disable language extensions */
#if NEW_SLANG /* allow-built-ins */
	grammar_set_reg8(*id, (const byte *) "parsing_builtin", 1);
#else
	grammar_set_reg8(*id, (const byte *) "parsing_builtin", 0);
#endif
	builtins = object->builtin;
    }

    /* compile the actual shader - pass-in built-in library for external shader */
    return compile_with_grammar(*id, source, &object->unit, type, infolog,
				builtins, program);
}


static bool
compile_shader(GLcontext *ctx, slang_code_object * object,
	       slang_unit_type type, slang_info_log * infolog,
	       struct gl_shader *shader)
{
    struct gl_program *program = shader->Programs[0];
    bool success;
    grammar id = 0;

    assert(program);

    _slang_code_object_dtr(object);
    _slang_code_object_ctr(object);

    success = compile_object(&id, shader->Source.c_str(), object, type, infolog, program);
    if (id != 0)
	grammar_destroy(id);
    if (!success)
	return false;

    return true;
}



bool
_slang_compile(GLcontext *ctx, struct gl_shader *shader)
{
    bool success;
    slang_info_log info_log;
    slang_code_object obj;
    slang_unit_type type;

    if (shader->Type == GL_VERTEX_SHADER) {
	type = SLANG_UNIT_VERTEX_SHADER;
    } else {
	assert(shader->Type == GL_FRAGMENT_SHADER);
	type = SLANG_UNIT_FRAGMENT_SHADER;
    }

    if (shader->Source.empty())
	return false;

    ctx->Shader.MemPool = _slang_new_mempool(1024*1024);

    /* XXX temporary hack */
    if (shader->Programs.empty()) {
	GLenum progTarget;
	if (shader->Type == GL_VERTEX_SHADER)
	    progTarget = GL_VERTEX_PROGRAM_ARB;
	else
	    progTarget = GL_FRAGMENT_PROGRAM_ARB;
	shader->Programs.resize(1);
	shader->Programs[0] = ctx->Driver.NewProgram(ctx, progTarget, 1);

	shader->Programs[0]->Parameters = _mesa_new_parameter_list();
	shader->Programs[0]->Varying = _mesa_new_parameter_list();
	shader->Programs[0]->Attributes = _mesa_new_parameter_list();
    }

    slang_info_log_construct(&info_log);
    _slang_code_object_ctr(&obj);

    success = compile_shader(ctx, &obj, type, &info_log, shader);

    /* free shader's prev info log */
    shader->InfoLog.clear();

    /* copy info-log string to shader object */
    if (!info_log.text.empty()) {
	shader->InfoLog = info_log.text;
    }

    if (info_log.error_flag) {
	success = false;
    }

    slang_info_log_destruct(&info_log);
    _slang_code_object_dtr(&obj);

    _slang_delete_mempool((slang_mempool *) ctx->Shader.MemPool);
    ctx->Shader.MemPool = nullptr;

    return success;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
