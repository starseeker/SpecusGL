/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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
 * \file slang_compile_variable.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"


struct type_specifier_type_name {
    const char *name;
    slang_type_specifier_type type;
};

static const type_specifier_type_name type_specifier_type_names[] = {
    {"void", SLANG_SPEC_VOID},
    {"bool", SLANG_SPEC_BOOL},
    {"bvec2", SLANG_SPEC_BVEC2},
    {"bvec3", SLANG_SPEC_BVEC3},
    {"bvec4", SLANG_SPEC_BVEC4},
    {"int", SLANG_SPEC_INT},
    {"ivec2", SLANG_SPEC_IVEC2},
    {"ivec3", SLANG_SPEC_IVEC3},
    {"ivec4", SLANG_SPEC_IVEC4},
    {"float", SLANG_SPEC_FLOAT},
    {"vec2", SLANG_SPEC_VEC2},
    {"vec3", SLANG_SPEC_VEC3},
    {"vec4", SLANG_SPEC_VEC4},
    {"mat2", SLANG_SPEC_MAT2},
    {"mat3", SLANG_SPEC_MAT3},
    {"mat4", SLANG_SPEC_MAT4},
    {"mat2x3", SLANG_SPEC_MAT23},
    {"mat3x2", SLANG_SPEC_MAT32},
    {"mat2x4", SLANG_SPEC_MAT24},
    {"mat4x2", SLANG_SPEC_MAT42},
    {"mat3x4", SLANG_SPEC_MAT34},
    {"mat4x3", SLANG_SPEC_MAT43},
    {"sampler1D", SLANG_SPEC_SAMPLER1D},
    {"sampler2D", SLANG_SPEC_SAMPLER2D},
    {"sampler3D", SLANG_SPEC_SAMPLER3D},
    {"samplerCube", SLANG_SPEC_SAMPLERCUBE},
    {"sampler1DShadow", SLANG_SPEC_SAMPLER1DSHADOW},
    {"sampler2DShadow", SLANG_SPEC_SAMPLER2DSHADOW},
    {"sampler2DRect", SLANG_SPEC_SAMPLER2DRECT},
    {"sampler2DRectShadow", SLANG_SPEC_SAMPLER2DRECTSHADOW},
    {nullptr, SLANG_SPEC_VOID}
};

slang_type_specifier_type
slang_type_specifier_type_from_string(const char *name)
{
    const type_specifier_type_name *p = type_specifier_type_names;
    while (p->name != nullptr) {
	if (slang_string_compare(p->name, name) == 0)
	    break;
	p++;
    }
    return p->type;
}

const char *
slang_type_specifier_type_to_string(slang_type_specifier_type type)
{
    const type_specifier_type_name *p = type_specifier_type_names;
    while (p->name != nullptr) {
	if (p->type == type)
	    break;
	p++;
    }
    return p->name;
}

/* slang_fully_specified_type */

/* slang_fully_specified_type_construct, _destruct are now inline in the header. */

int
slang_fully_specified_type_copy(slang_fully_specified_type * x,
				const slang_fully_specified_type * y)
{
    x->qualifier = y->qualifier;
    x->specifier = y->specifier;  /* deep copy via slang_type_specifier copy assignment */
    return 1;
}


static slang_variable *
slang_variable_new(void)
{
    slang_variable *v = new slang_variable;
    if (!slang_variable_construct(v)) {
	delete v;
	v = nullptr;
    }
    return v;
}


static void
slang_variable_delete(slang_variable * var)
{
    slang_variable_destruct(var);
    delete var;
}


/*
 * slang_variable_scope
 */

slang_variable_scope *
_slang_variable_scope_new(slang_variable_scope *parent)
{
    slang_variable_scope *s = new slang_variable_scope;
    s->outer_scope = parent;
    return s;
}


/*
 * slang_variable_scope
 */

/**
 * Destructor: free all owned variables.
 * This mirrors slang_variable_scope_destruct() so that deleting a scope
 * automatically cleans up without a separate explicit destruct call.
 */
slang_variable_scope::~slang_variable_scope()
{
    for (auto *v : variables) {
	if (v)
	    slang_variable_delete(v);
    }
    variables.clear();
    /* do not free outer_scope - not owned */
}

GLvoid
_slang_variable_scope_ctr(slang_variable_scope * self)
{
    self->variables.clear();
    self->outer_scope = nullptr;
}

void
slang_variable_scope_destruct(slang_variable_scope * scope)
{
    if (!scope)
	return;
    for (auto *v : scope->variables) {
	if (v)
	    slang_variable_delete(v);
    }
    scope->variables.clear();
    /* do not free scope->outer_scope */
}

int
slang_variable_scope_copy(slang_variable_scope * x,
			  const slang_variable_scope * y)
{
    slang_variable_scope z;
    const GLuint n = static_cast<GLuint>(y->variables.size());
    GLuint i;

    z.variables.resize(n, nullptr);
    for (i = 0; i < n; i++) {
	z.variables[i] = slang_variable_new();
	if (!z.variables[i]) {
	    slang_variable_scope_destruct(&z);
	    return 0;
	}
    }
    for (i = 0; i < n; i++) {
	if (!slang_variable_copy(z.variables[i], y->variables[i])) {
	    slang_variable_scope_destruct(&z);
	    return 0;
	}
    }
    z.outer_scope = y->outer_scope;
    slang_variable_scope_destruct(x);
    *x = std::move(z);
    return 1;
}


/**
 * Grow the variable list by one.
 * \return  pointer to space for the new variable (will be initialized)
 */
slang_variable *
slang_variable_scope_grow(slang_variable_scope *scope)
{
    slang_variable *v = slang_variable_new();
    if (!v)
	return nullptr;
    scope->variables.push_back(v);
    return v;
}



/* slang_variable */

int
slang_variable_construct(slang_variable * var)
{
    if (!slang_fully_specified_type_construct(&var->type))
	return 0;
    var->a_name = SLANG_ATOM_NULL;
    var->array_len = 0;
    /* initializer is a unique_ptr, default-constructed to null */
    var->address = ~0;
    var->size = 0;
    var->isTemp = GL_FALSE;
    var->aux = nullptr;
    return 1;
}


void
slang_variable_destruct(slang_variable * var)
{
    slang_fully_specified_type_destruct(&var->type);
    /* initializer unique_ptr is destroyed automatically */
    var->initializer.reset();
}


int
slang_variable_copy(slang_variable * x, const slang_variable * y)
{
    slang_variable z;

    if (!slang_variable_construct(&z))
	return 0;
    if (!slang_fully_specified_type_copy(&z.type, &y->type)) {
	slang_variable_destruct(&z);
	return 0;
    }
    z.a_name = y->a_name;
    z.array_len = y->array_len;
    if (y->initializer) {
	z.initializer = std::make_unique<slang_operation>();
	if (!slang_operation_construct(z.initializer.get())) {
	    z.initializer.reset();
	    slang_variable_destruct(&z);
	    return 0;
	}
	if (!slang_operation_copy(z.initializer.get(), y->initializer.get())) {
	    slang_variable_destruct(&z);
	    return 0;
	}
    }
    z.address = y->address;
    z.size = y->size;
    slang_variable_destruct(x);
    *x = std::move(z);
    return 1;
}


slang_variable *
_slang_locate_variable(const slang_variable_scope * scope,
		       const slang_atom a_name, GLboolean all)
{
    for (slang_variable *v : scope->variables)
	if (a_name == v->a_name)
	    return v;
    if (all && scope->outer_scope != nullptr)
	return _slang_locate_variable(scope->outer_scope, a_name, 1);
    return nullptr;
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
