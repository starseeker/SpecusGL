/*
 * Mesa 3-D graphics library
 * Version:  6.5
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
 * \file slang_assemble_typeinfo.c
 * slang type info
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_typeinfo.h"
#include "slang_compile.h"
#include "slang_log.h"
#include "slang_mem.h"
#include "prog_instruction.h"


/**
 * Checks if a field selector is a general swizzle (an r-value swizzle
 * with replicated components or an l-value swizzle mask) for a
 * vector.  Returns GL_TRUE if this is the case, <swz> is filled with
 * swizzle information.  Returns GL_FALSE otherwise.
 */
GLboolean
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle * swz)
{
    GLuint i;
    GLboolean xyzw = GL_FALSE, rgba = GL_FALSE, stpq = GL_FALSE;

    /* init to undefined.
     * We rely on undefined/nil values to distinguish between
     * regular swizzles and writemasks.
     * For example, the swizzle ".xNNN" is the writemask ".x".
     * That's different than the swizzle ".xxxx".
     */
    for (i = 0; i < 4; i++)
	swz->swizzle[i] = SWIZZLE_NIL;

    /* the swizzle can be at most 4-component long */
    swz->num_components = slang_string_length(field);
    if (swz->num_components > 4)
	return GL_FALSE;

    for (i = 0; i < swz->num_components; i++) {
	/* mark which swizzle group is used */
	switch (field[i]) {
	    case 'x':
	    case 'y':
	    case 'z':
	    case 'w':
		xyzw = GL_TRUE;
		break;
	    case 'r':
	    case 'g':
	    case 'b':
	    case 'a':
		rgba = GL_TRUE;
		break;
	    case 's':
	    case 't':
	    case 'p':
	    case 'q':
		stpq = GL_TRUE;
		break;
	    default:
		return GL_FALSE;
	}

	/* collect swizzle component */
	switch (field[i]) {
	    case 'x':
	    case 'r':
	    case 's':
		swz->swizzle[i] = 0;
		break;
	    case 'y':
	    case 'g':
	    case 't':
		swz->swizzle[i] = 1;
		break;
	    case 'z':
	    case 'b':
	    case 'p':
		swz->swizzle[i] = 2;
		break;
	    case 'w':
	    case 'a':
	    case 'q':
		swz->swizzle[i] = 3;
		break;
	}

	/* check if the component is valid for given vector's row count */
	if (rows <= swz->swizzle[i])
	    return GL_FALSE;
    }

    /* only one swizzle group can be used */
    if ((xyzw && rgba) || (xyzw && stpq) || (rgba && stpq))
	return GL_FALSE;

    return GL_TRUE;
}



/**
 * Checks if a general swizzle is an l-value swizzle - these swizzles
 * do not have duplicated fields.  Returns GL_TRUE if this is a
 * swizzle mask.  Returns GL_FALSE otherwise
 */
GLboolean
_slang_is_swizzle_mask(const slang_swizzle * swz, GLuint rows)
{
    GLuint i, c = 0;

    /* the swizzle may not be longer than the vector dim */
    if (swz->num_components > rows)
	return GL_FALSE;

    /* the swizzle components cannot be duplicated */
    for (i = 0; i < swz->num_components; i++) {
	if ((c & (1 << swz->swizzle[i])) != 0)
	    return GL_FALSE;
	c |= 1 << swz->swizzle[i];
    }

    return GL_TRUE;
}


/**
 * Combines (multiplies) two swizzles to form single swizzle.
 * Example: "vec.wzyx.yx" --> "vec.zw".
 */
GLvoid
_slang_multiply_swizzles(slang_swizzle * dst, const slang_swizzle * left,
			 const slang_swizzle * right)
{
    GLuint i;

    dst->num_components = right->num_components;
    for (i = 0; i < right->num_components; i++)
	dst->swizzle[i] = left->swizzle[right->swizzle[i]];
}


/* ------------------------------------------------------------------ */
/* slang_type_specifier RAII implementation                           */
/* ------------------------------------------------------------------ */

/**
 * Custom deleter: now that slang_struct has a proper destructor that frees
 * its owned members, we can simply delete the object.  We keep the
 * slang_struct_destruct() call for backward-compatibility safety.
 */
void SlangStructDeleter::operator()(slang_struct *s) const noexcept
{
    delete s;   /* ~slang_struct() frees fields and structs via unique_ptr */
}

/** Destructor – unique_ptr members handle cleanup automatically. */
slang_type_specifier::~slang_type_specifier() = default;

/** Deep copy constructor. */
slang_type_specifier::slang_type_specifier(const slang_type_specifier &other)
    : type(other.type)
{
    if (other._struct)
        _struct.reset(new slang_struct(*other._struct));  /* copy via slang_struct copy ctor */
    if (other._array)
        _array = std::make_unique<slang_type_specifier>(*other._array);
}

/** Deep copy assignment – safe for self-assignment. */
slang_type_specifier &
slang_type_specifier::operator=(const slang_type_specifier &other)
{
    if (this != &other)
        *this = slang_type_specifier(other); /* copy-construct then move */
    return *this;
}

/** Move constructor – transfers ownership; source is left in empty state. */
slang_type_specifier::slang_type_specifier(slang_type_specifier &&other) noexcept
    : type(other.type), _struct(std::move(other._struct)),
      _array(std::move(other._array))
{
    other.type = SLANG_SPEC_VOID;
}

/** Move assignment – transfers ownership; source is left in empty state. */
slang_type_specifier &
slang_type_specifier::operator=(slang_type_specifier &&other) noexcept
{
    if (this != &other) {
        type    = other.type;
        _struct = std::move(other._struct);
        _array  = std::move(other._array);
        other.type = SLANG_SPEC_VOID;
    }
    return *this;
}


GLboolean
slang_type_specifier_copy(slang_type_specifier * x,
			  const slang_type_specifier * y)
{
    *x = *y;   /* invokes deep copy assignment */
    return GL_TRUE;
}


/**
 * Test if two types are equal.
 */
GLboolean
slang_type_specifier_equal(const slang_type_specifier * x,
			   const slang_type_specifier * y)
{
    if (x->type != y->type)
	return GL_FALSE;
    if (x->type == SLANG_SPEC_STRUCT)
	return slang_struct_equal(x->_struct.get(), y->_struct.get());
    if (x->type == SLANG_SPEC_ARRAY)
	return slang_type_specifier_equal(x->_array.get(), y->_array.get());
    return GL_TRUE;
}


/**
 * As above, but allow float/int casting.
 */
static GLboolean
slang_type_specifier_compatible(const slang_type_specifier * x,
				const slang_type_specifier * y)
{
    /* special case: float == int */
    if (x->type == SLANG_SPEC_INT && y->type == SLANG_SPEC_FLOAT) {
	return GL_TRUE;
    }
    /* XXX may need to add bool/int compatibility, etc */

    if (x->type != y->type)
	return GL_FALSE;
    if (x->type == SLANG_SPEC_STRUCT)
	return slang_struct_equal(x->_struct.get(), y->_struct.get());
    if (x->type == SLANG_SPEC_ARRAY)
	return slang_type_specifier_compatible(x->_array.get(), y->_array.get());
    return GL_TRUE;
}




/* slang_typeinfo_construct and slang_typeinfo_destruct are now inline in slang_typeinfo.h */



/**
 * Determine the return type of a function.
 * \param a_name  the function name
 * \param param  function parameters (overloading)
 * \param num_params  number of parameters to function
 * \param space  namespace to search
 * \param spec  returns the type
 * \param funFound  returns pointer to the function, or nullptr if not found.
 * \return GL_TRUE for success, GL_FALSE if failure (bad function name)
 */
static GLboolean
_slang_typeof_function(slang_atom a_name,
		       slang_operation * params, GLuint num_params,
		       const slang_name_space * space,
		       slang_type_specifier * spec,
		       slang_function **funFound,
		       slang_atom_pool *atoms, slang_info_log *log)
{
    *funFound = _slang_locate_function(space->funcs, a_name, params,
				       num_params, space, atoms, log);
    if (!*funFound)
	return GL_TRUE;  /* yes, not false */
    return slang_type_specifier_copy(spec, &(*funFound)->header.type.specifier);
}


/**
 * Determine the type of a math function.
 * \param name  name of the operator, one of +,-,*,/ or unary -
 * \param params  array of function parameters
 * \param num_params  number of parameters
 * \param space  namespace to use
 * \param spec  returns the function's type
 * \param atoms  atom pool
 * \return GL_TRUE for success, GL_FALSE if failure
 */
static GLboolean
typeof_math_call(const char *name, slang_operation *call,
		 const slang_name_space * space,
		 slang_type_specifier * spec,
		 slang_atom_pool * atoms,
		 slang_info_log *log)
{
    if (call->fun) {
	/* we've previously resolved this function call */
	slang_type_specifier_copy(spec, &call->fun->header.type.specifier);
	return GL_TRUE;
    } else {
	slang_atom atom;
	slang_function *fun;

	/* number of params: */
	assert(call->children.size() == 1 || call->children.size() == 2);

	atom = slang_atom_pool_atom(atoms, name);
	if (!_slang_typeof_function(atom, call->children.data(), (GLuint)call->children.size(),
				    space, spec, &fun, atoms, log))
	    return GL_FALSE;

	if (fun) {
	    /* Save pointer to save time in future */
	    call->fun = fun;
	    return GL_TRUE;
	}
	return GL_FALSE;
    }
}

GLboolean
_slang_typeof_operation(const slang_assemble_ctx * A,
			slang_operation * op,
			slang_typeinfo * ti)
{
    return _slang_typeof_operation_(op, &A->space, ti, A->atoms, A->log);
}


/**
 * Determine the return type of an operation.
 * \param op  the operation node
 * \param space  the namespace to use
 * \param ti  the returned type
 * \param atoms  atom pool
 * \return GL_TRUE for success, GL_FALSE if failure
 */
GLboolean
_slang_typeof_operation_(slang_operation * op,
			 const slang_name_space * space,
			 slang_typeinfo * ti,
			 slang_atom_pool * atoms,
			 slang_info_log *log)
{
    ti->can_be_referenced = GL_FALSE;
    ti->is_swizzled = GL_FALSE;

    switch (op->type) {
	case SLANG_OPER_BLOCK_NO_NEW_SCOPE:
	case SLANG_OPER_BLOCK_NEW_SCOPE:
	case SLANG_OPER_VARIABLE_DECL:
	case SLANG_OPER_ASM:
	case SLANG_OPER_BREAK:
	case SLANG_OPER_CONTINUE:
	case SLANG_OPER_DISCARD:
	case SLANG_OPER_RETURN:
	case SLANG_OPER_IF:
	case SLANG_OPER_WHILE:
	case SLANG_OPER_DO:
	case SLANG_OPER_FOR:
	case SLANG_OPER_VOID:
	    ti->spec.type = SLANG_SPEC_VOID;
	    break;
	case SLANG_OPER_EXPRESSION:
	case SLANG_OPER_ASSIGN:
	case SLANG_OPER_ADDASSIGN:
	case SLANG_OPER_SUBASSIGN:
	case SLANG_OPER_MULASSIGN:
	case SLANG_OPER_DIVASSIGN:
	case SLANG_OPER_PREINCREMENT:
	case SLANG_OPER_PREDECREMENT:
	    if (!_slang_typeof_operation_(&op->children[0], space, ti, atoms, log))
		return GL_FALSE;
	    break;
	case SLANG_OPER_LITERAL_BOOL:
	    if (op->literal_size == 1)
		ti->spec.type = SLANG_SPEC_BOOL;
	    else if (op->literal_size == 2)
		ti->spec.type = SLANG_SPEC_BVEC2;
	    else if (op->literal_size == 3)
		ti->spec.type = SLANG_SPEC_BVEC3;
	    else if (op->literal_size == 4)
		ti->spec.type = SLANG_SPEC_BVEC4;
	    else {
		_mesa_problem(nullptr,
			      "Unexpected bool literal_size %d in _slang_typeof_operation()",
			      op->literal_size);
		ti->spec.type = SLANG_SPEC_BOOL;
	    }
	    break;
	case SLANG_OPER_LOGICALOR:
	case SLANG_OPER_LOGICALXOR:
	case SLANG_OPER_LOGICALAND:
	case SLANG_OPER_EQUAL:
	case SLANG_OPER_NOTEQUAL:
	case SLANG_OPER_LESS:
	case SLANG_OPER_GREATER:
	case SLANG_OPER_LESSEQUAL:
	case SLANG_OPER_GREATEREQUAL:
	case SLANG_OPER_NOT:
	    ti->spec.type = SLANG_SPEC_BOOL;
	    break;
	case SLANG_OPER_LITERAL_INT:
	    if (op->literal_size == 1)
		ti->spec.type = SLANG_SPEC_INT;
	    else if (op->literal_size == 2)
		ti->spec.type = SLANG_SPEC_IVEC2;
	    else if (op->literal_size == 3)
		ti->spec.type = SLANG_SPEC_IVEC3;
	    else if (op->literal_size == 4)
		ti->spec.type = SLANG_SPEC_IVEC4;
	    else {
		_mesa_problem(nullptr,
			      "Unexpected int literal_size %d in _slang_typeof_operation()",
			      op->literal_size);
		ti->spec.type = SLANG_SPEC_INT;
	    }
	    break;
	case SLANG_OPER_LITERAL_FLOAT:
	    if (op->literal_size == 1)
		ti->spec.type = SLANG_SPEC_FLOAT;
	    else if (op->literal_size == 2)
		ti->spec.type = SLANG_SPEC_VEC2;
	    else if (op->literal_size == 3)
		ti->spec.type = SLANG_SPEC_VEC3;
	    else if (op->literal_size == 4)
		ti->spec.type = SLANG_SPEC_VEC4;
	    else {
		_mesa_problem(nullptr,
			      "Unexpected float literal_size %d in _slang_typeof_operation()",
			      op->literal_size);
		ti->spec.type = SLANG_SPEC_FLOAT;
	    }
	    break;
	case SLANG_OPER_IDENTIFIER: {
	    slang_variable *var;
	    var = _slang_locate_variable(op->locals.get(), op->a_id, GL_TRUE);
	    if (!var) {
		slang_info_log_error(log, "undefined variable '%s'",
				     reinterpret_cast<char *>(op->a_id));
		return GL_FALSE;
	    }
	    if (!slang_type_specifier_copy(&ti->spec, &var->type.specifier)) {
		slang_info_log_memory(log);
		return GL_FALSE;
	    }
	    ti->can_be_referenced = GL_TRUE;
	    ti->array_len = var->array_len;
	}
	break;
	case SLANG_OPER_SEQUENCE:
	    /* TODO: check [0] and [1] if they match */
	    if (!_slang_typeof_operation_(&op->children[1], space, ti, atoms, log)) {
		return GL_FALSE;
	    }
	    ti->can_be_referenced = GL_FALSE;
	    ti->is_swizzled = GL_FALSE;
	    break;
	/*case SLANG_OPER_MODASSIGN: */
	/*case SLANG_OPER_LSHASSIGN: */
	/*case SLANG_OPER_RSHASSIGN: */
	/*case SLANG_OPER_ORASSIGN: */
	/*case SLANG_OPER_XORASSIGN: */
	/*case SLANG_OPER_ANDASSIGN: */
	case SLANG_OPER_SELECT:
	    /* TODO: check [1] and [2] if they match */
	    if (!_slang_typeof_operation_(&op->children[1], space, ti, atoms, log)) {
		return GL_FALSE;
	    }
	    ti->can_be_referenced = GL_FALSE;
	    ti->is_swizzled = GL_FALSE;
	    break;
	/*case SLANG_OPER_BITOR: */
	/*case SLANG_OPER_BITXOR: */
	/*case SLANG_OPER_BITAND: */
	/*case SLANG_OPER_LSHIFT: */
	/*case SLANG_OPER_RSHIFT: */
	case SLANG_OPER_ADD:
	    assert((GLuint)op->children.size() == 2);
	    if (!typeof_math_call("+", op, space, &ti->spec, atoms, log))
		return GL_FALSE;
	    break;
	case SLANG_OPER_SUBTRACT:
	    assert((GLuint)op->children.size() == 2);
	    if (!typeof_math_call("-", op, space, &ti->spec, atoms, log))
		return GL_FALSE;
	    break;
	case SLANG_OPER_MULTIPLY:
	    assert((GLuint)op->children.size() == 2);
	    if (!typeof_math_call("*", op, space, &ti->spec, atoms, log))
		return GL_FALSE;
	    break;
	case SLANG_OPER_DIVIDE:
	    assert((GLuint)op->children.size() == 2);
	    if (!typeof_math_call("/", op, space, &ti->spec, atoms, log))
		return GL_FALSE;
	    break;
	/*case SLANG_OPER_MODULUS: */
	case SLANG_OPER_PLUS:
	    if (!_slang_typeof_operation_(&op->children[0], space, ti, atoms, log))
		return GL_FALSE;
	    ti->can_be_referenced = GL_FALSE;
	    ti->is_swizzled = GL_FALSE;
	    break;
	case SLANG_OPER_MINUS:
	    assert((GLuint)op->children.size() == 1);
	    if (!typeof_math_call("-", op, space, &ti->spec, atoms, log))
		return GL_FALSE;
	    break;
	/*case SLANG_OPER_COMPLEMENT: */
	case SLANG_OPER_SUBSCRIPT: {
	    slang_typeinfo _ti;

	    if (!slang_typeinfo_construct(&_ti))
		return GL_FALSE;
	    if (!_slang_typeof_operation_(&op->children[0], space, &_ti, atoms, log)) {
		slang_typeinfo_destruct(&_ti);
		return GL_FALSE;
	    }
	    ti->can_be_referenced = _ti.can_be_referenced;
	    if (_ti.spec.type == SLANG_SPEC_ARRAY) {
		if (!slang_type_specifier_copy(&ti->spec, _ti.spec._array.get())) {
		    slang_typeinfo_destruct(&_ti);
		    return GL_FALSE;
		}
	    } else {
		if (!_slang_type_is_vector(_ti.spec.type)
		    && !_slang_type_is_matrix(_ti.spec.type)) {
		    slang_typeinfo_destruct(&_ti);
		    slang_info_log_error(log, "cannot index a non-array type");
		    return GL_FALSE;
		}
		ti->spec.type = _slang_type_base(_ti.spec.type);
	    }
	    slang_typeinfo_destruct(&_ti);
	}
	break;
	case SLANG_OPER_CALL:
	    if (op->fun) {
		/* we've resolved this call before */
		slang_type_specifier_copy(&ti->spec, &op->fun->header.type.specifier);
	    } else {
		slang_function *fun;
		if (!_slang_typeof_function(op->a_id, op->children.data(), (GLuint)op->children.size(),
					    space, &ti->spec, &fun, atoms, log))
		    return GL_FALSE;
		if (fun) {
		    /* save result for future use */
		    op->fun = fun;
		} else {
		    slang_struct *s =
			slang_struct_scope_find(space->structs, op->a_id, GL_TRUE);
		    if (s) {
			/* struct initializer */
			ti->spec.type = SLANG_SPEC_STRUCT;
			/* Use slang_struct copy constructor directly */
			ti->spec._struct = std::unique_ptr<slang_struct, SlangStructDeleter>(new slang_struct(*s));
		    } else {
			/* float, int, vec4, mat3, etc. constructor? */
			const char *name;
			slang_type_specifier_type type;

			name = slang_atom_pool_id(atoms, op->a_id);
			type = slang_type_specifier_type_from_string(name);
			if (type == SLANG_SPEC_VOID) {
			    slang_info_log_error(log, "undefined function '%s'", name);
			    return GL_FALSE;
			}
			ti->spec.type = type;
		    }
		}
	    }
	    break;
	case SLANG_OPER_FIELD: {
	    slang_typeinfo _ti;

	    if (!slang_typeinfo_construct(&_ti))
		return GL_FALSE;
	    if (!_slang_typeof_operation_(&op->children[0], space, &_ti, atoms, log)) {
		slang_typeinfo_destruct(&_ti);
		return GL_FALSE;
	    }
	    if (_ti.spec.type == SLANG_SPEC_STRUCT) {
		slang_variable *field;

		field = _slang_locate_variable(_ti.spec._struct->fields.get(), op->a_id,
					       GL_FALSE);
		if (field == nullptr) {
		    slang_typeinfo_destruct(&_ti);
		    return GL_FALSE;
		}
		if (!slang_type_specifier_copy(&ti->spec, &field->type.specifier)) {
		    slang_typeinfo_destruct(&_ti);
		    return GL_FALSE;
		}
		ti->can_be_referenced = _ti.can_be_referenced;
	    } else {
		GLuint rows;
		const char *swizzle;
		slang_type_specifier_type base;

		/* determine the swizzle of the field expression */
		if (!_slang_type_is_vector(_ti.spec.type)) {
		    slang_typeinfo_destruct(&_ti);
		    slang_info_log_error(log, "Can't swizzle scalar expression");
		    return GL_FALSE;
		}
		rows = _slang_type_dim(_ti.spec.type);
		swizzle = slang_atom_pool_id(atoms, op->a_id);
		if (!_slang_is_swizzle(swizzle, rows, &ti->swz)) {
		    slang_typeinfo_destruct(&_ti);
		    slang_info_log_error(log, "bad swizzle '%s'", swizzle);
		    return GL_FALSE;
		}
		ti->is_swizzled = GL_TRUE;
		ti->can_be_referenced = _ti.can_be_referenced
					&& _slang_is_swizzle_mask(&ti->swz, rows);
		if (_ti.is_swizzled) {
		    slang_swizzle swz;

		    /* swizzle the swizzle */
		    _slang_multiply_swizzles(&swz, &_ti.swz, &ti->swz);
		    ti->swz = swz;
		}
		base = _slang_type_base(_ti.spec.type);
		switch (ti->swz.num_components) {
		    case 1:
			ti->spec.type = base;
			break;
		    case 2:
			switch (base) {
			    case SLANG_SPEC_FLOAT:
				ti->spec.type = SLANG_SPEC_VEC2;
				break;
			    case SLANG_SPEC_INT:
				ti->spec.type = SLANG_SPEC_IVEC2;
				break;
			    case SLANG_SPEC_BOOL:
				ti->spec.type = SLANG_SPEC_BVEC2;
				break;
			    default:
				break;
			}
			break;
		    case 3:
			switch (base) {
			    case SLANG_SPEC_FLOAT:
				ti->spec.type = SLANG_SPEC_VEC3;
				break;
			    case SLANG_SPEC_INT:
				ti->spec.type = SLANG_SPEC_IVEC3;
				break;
			    case SLANG_SPEC_BOOL:
				ti->spec.type = SLANG_SPEC_BVEC3;
				break;
			    default:
				break;
			}
			break;
		    case 4:
			switch (base) {
			    case SLANG_SPEC_FLOAT:
				ti->spec.type = SLANG_SPEC_VEC4;
				break;
			    case SLANG_SPEC_INT:
				ti->spec.type = SLANG_SPEC_IVEC4;
				break;
			    case SLANG_SPEC_BOOL:
				ti->spec.type = SLANG_SPEC_BVEC4;
				break;
			    default:
				break;
			}
			break;
		    default:
			break;
		}
	    }
	    slang_typeinfo_destruct(&_ti);
	}
	break;
	case SLANG_OPER_POSTINCREMENT:
	case SLANG_OPER_POSTDECREMENT:
	    if (!_slang_typeof_operation_(&op->children[0], space, ti, atoms, log))
		return GL_FALSE;
	    ti->can_be_referenced = GL_FALSE;
	    ti->is_swizzled = GL_FALSE;
	    break;
	default:
	    return GL_FALSE;
    }

    return GL_TRUE;
}


/**
 * Lookup a function according to name and parameter count/types.
 */
slang_function *
_slang_locate_function(const slang_function_scope * funcs, slang_atom a_name,
		       slang_operation * args, GLuint num_args,
		       const slang_name_space * space, slang_atom_pool * atoms,
		       slang_info_log *log)
{
    for (const auto &f_ref : funcs->functions) {
	const slang_function *f = &f_ref;
	const GLuint haveRetValue = _slang_function_has_return_value(f);
	GLuint j;

	if (a_name != f->header.a_name)
	    continue;
	if (f->param_count - haveRetValue != num_args)
	    continue;

	/* compare parameter / argument types */
	for (j = 0; j < num_args; j++) {
	    slang_typeinfo ti;

	    if (!slang_typeinfo_construct(&ti))
		return nullptr;
	    if (!_slang_typeof_operation_(&args[j], space, &ti, atoms, log)) {
		slang_typeinfo_destruct(&ti);
		return nullptr;
	    }
	    if (!slang_type_specifier_compatible(&ti.spec,
						 &f->parameters->variables[j]->type.specifier)) {
		slang_typeinfo_destruct(&ti);
		break;
	    }
	    slang_typeinfo_destruct(&ti);

	    /* "out" and "inout" formal parameter requires the actual
	     * parameter to be l-value.
	     */
	    if (!ti.can_be_referenced &&
		(f->parameters->variables[j]->type.qualifier == SLANG_QUAL_OUT ||
		 f->parameters->variables[j]->type.qualifier == SLANG_QUAL_INOUT))
		break;
	}
	if (j == num_args)
	    return const_cast<slang_function *>(f);
    }
    if (funcs->outer_scope != nullptr)
	return _slang_locate_function(funcs->outer_scope, a_name, args,
				      num_args, space, atoms, log);
    return nullptr;
}


/**
 * Determine if a type is a matrix.
 * \return GL_TRUE if is a matrix, GL_FALSE otherwise.
 */
GLboolean
_slang_type_is_matrix(slang_type_specifier_type ty)
{
    switch (ty) {
	case SLANG_SPEC_MAT2:
	case SLANG_SPEC_MAT3:
	case SLANG_SPEC_MAT4:
	case SLANG_SPEC_MAT23:
	case SLANG_SPEC_MAT32:
	case SLANG_SPEC_MAT24:
	case SLANG_SPEC_MAT42:
	case SLANG_SPEC_MAT34:
	case SLANG_SPEC_MAT43:
	    return GL_TRUE;
	default:
	    return GL_FALSE;
    }
}


/**
 * Determine if a type is a vector.
 * \return GL_TRUE if is a vector, GL_FALSE otherwise.
 */
GLboolean
_slang_type_is_vector(slang_type_specifier_type ty)
{
    switch (ty) {
	case SLANG_SPEC_VEC2:
	case SLANG_SPEC_VEC3:
	case SLANG_SPEC_VEC4:
	case SLANG_SPEC_IVEC2:
	case SLANG_SPEC_IVEC3:
	case SLANG_SPEC_IVEC4:
	case SLANG_SPEC_BVEC2:
	case SLANG_SPEC_BVEC3:
	case SLANG_SPEC_BVEC4:
	    return GL_TRUE;
	default:
	    return GL_FALSE;
    }
}


/**
 * Given a vector type, return the type of the vector's elements.
 * For a matrix, return the type of the columns.
 */
slang_type_specifier_type
_slang_type_base(slang_type_specifier_type ty)
{
    switch (ty) {
	case SLANG_SPEC_FLOAT:
	case SLANG_SPEC_VEC2:
	case SLANG_SPEC_VEC3:
	case SLANG_SPEC_VEC4:
	    return SLANG_SPEC_FLOAT;
	case SLANG_SPEC_INT:
	case SLANG_SPEC_IVEC2:
	case SLANG_SPEC_IVEC3:
	case SLANG_SPEC_IVEC4:
	    return SLANG_SPEC_INT;
	case SLANG_SPEC_BOOL:
	case SLANG_SPEC_BVEC2:
	case SLANG_SPEC_BVEC3:
	case SLANG_SPEC_BVEC4:
	    return SLANG_SPEC_BOOL;
	case SLANG_SPEC_MAT2:
	    return SLANG_SPEC_VEC2;
	case SLANG_SPEC_MAT3:
	    return SLANG_SPEC_VEC3;
	case SLANG_SPEC_MAT4:
	    return SLANG_SPEC_VEC4;
	case SLANG_SPEC_MAT23:
	    return SLANG_SPEC_VEC3;
	case SLANG_SPEC_MAT32:
	    return SLANG_SPEC_VEC2;
	case SLANG_SPEC_MAT24:
	    return SLANG_SPEC_VEC4;
	case SLANG_SPEC_MAT42:
	    return SLANG_SPEC_VEC2;
	case SLANG_SPEC_MAT34:
	    return SLANG_SPEC_VEC4;
	case SLANG_SPEC_MAT43:
	    return SLANG_SPEC_VEC3;
	default:
	    return SLANG_SPEC_VOID;
    }
}


/**
 * Return the dimensionality of a vector, or for a matrix, return number
 * of columns.
 */
GLuint
_slang_type_dim(slang_type_specifier_type ty)
{
    switch (ty) {
	case SLANG_SPEC_FLOAT:
	case SLANG_SPEC_INT:
	case SLANG_SPEC_BOOL:
	    return 1;
	case SLANG_SPEC_VEC2:
	case SLANG_SPEC_IVEC2:
	case SLANG_SPEC_BVEC2:
	case SLANG_SPEC_MAT2:
	    return 2;
	case SLANG_SPEC_VEC3:
	case SLANG_SPEC_IVEC3:
	case SLANG_SPEC_BVEC3:
	case SLANG_SPEC_MAT3:
	    return 3;
	case SLANG_SPEC_VEC4:
	case SLANG_SPEC_IVEC4:
	case SLANG_SPEC_BVEC4:
	case SLANG_SPEC_MAT4:
	    return 4;

	case SLANG_SPEC_MAT23:
	    return 2;
	case SLANG_SPEC_MAT32:
	    return 3;
	case SLANG_SPEC_MAT24:
	    return 2;
	case SLANG_SPEC_MAT42:
	    return 4;
	case SLANG_SPEC_MAT34:
	    return 3;
	case SLANG_SPEC_MAT43:
	    return 4;

	default:
	    return 0;
    }
}


/**
 * Return the GL_* type that corresponds to a SLANG_SPEC_* type.
 */
GLenum
_slang_gltype_from_specifier(const slang_type_specifier *type)
{
    switch (type->type) {
	case SLANG_SPEC_BOOL:
	    return GL_BOOL;
	case SLANG_SPEC_BVEC2:
	    return GL_BOOL_VEC2;
	case SLANG_SPEC_BVEC3:
	    return GL_BOOL_VEC3;
	case SLANG_SPEC_BVEC4:
	    return GL_BOOL_VEC4;
	case SLANG_SPEC_INT:
	    return GL_INT;
	case SLANG_SPEC_IVEC2:
	    return GL_INT_VEC2;
	case SLANG_SPEC_IVEC3:
	    return GL_INT_VEC3;
	case SLANG_SPEC_IVEC4:
	    return GL_INT_VEC4;
	case SLANG_SPEC_FLOAT:
	    return GL_FLOAT;
	case SLANG_SPEC_VEC2:
	    return GL_FLOAT_VEC2;
	case SLANG_SPEC_VEC3:
	    return GL_FLOAT_VEC3;
	case SLANG_SPEC_VEC4:
	    return GL_FLOAT_VEC4;
	case SLANG_SPEC_MAT2:
	    return GL_FLOAT_MAT2;
	case SLANG_SPEC_MAT3:
	    return GL_FLOAT_MAT3;
	case SLANG_SPEC_MAT4:
	    return GL_FLOAT_MAT4;
	case SLANG_SPEC_MAT23:
	    return GL_FLOAT_MAT2x3;
	case SLANG_SPEC_MAT32:
	    return GL_FLOAT_MAT3x2;
	case SLANG_SPEC_MAT24:
	    return GL_FLOAT_MAT2x4;
	case SLANG_SPEC_MAT42:
	    return GL_FLOAT_MAT4x2;
	case SLANG_SPEC_MAT34:
	    return GL_FLOAT_MAT3x4;
	case SLANG_SPEC_MAT43:
	    return GL_FLOAT_MAT4x3;
	case SLANG_SPEC_SAMPLER1D:
	    return GL_SAMPLER_1D;
	case SLANG_SPEC_SAMPLER2D:
	    return GL_SAMPLER_2D;
	case SLANG_SPEC_SAMPLER3D:
	    return GL_SAMPLER_3D;
	case SLANG_SPEC_SAMPLERCUBE:
	    return GL_SAMPLER_CUBE;
	case SLANG_SPEC_SAMPLER1DSHADOW:
	    return GL_SAMPLER_1D_SHADOW;
	case SLANG_SPEC_SAMPLER2DSHADOW:
	    return GL_SAMPLER_2D_SHADOW;
	case SLANG_SPEC_SAMPLER2DRECT:
	    return GL_SAMPLER_2D_RECT_ARB;
	case SLANG_SPEC_SAMPLER2DRECTSHADOW:
	    return GL_SAMPLER_2D_RECT_SHADOW_ARB;
	case SLANG_SPEC_ARRAY:
	    return _slang_gltype_from_specifier(type->_array.get());
	case SLANG_SPEC_STRUCT:
	/* fall-through */
	default:
	    return GL_NONE;
    }
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
