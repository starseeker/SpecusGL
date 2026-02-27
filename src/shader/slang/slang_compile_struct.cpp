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
 * \file slang_compile_struct.c
 * slang front-end compiler
 * \author Michal Krol
 *
 * C++17 modernisation: slang_struct_scope now uses std::vector<slang_struct>
 * instead of a raw array + count pair.  slang_struct now owns its fields and
 * structs sub-objects via new/delete.
 */

#include "imports.h"
#include "slang_compile.h"


GLvoid
_slang_struct_scope_ctr(slang_struct_scope * self)
{
    self->structs.clear();
    self->outer_scope = nullptr;
}

void
slang_struct_scope_destruct(slang_struct_scope * scope)
{
    for (auto &s : scope->structs)
	slang_struct_destruct(&s);
    scope->structs.clear();
    /* do not free scope->outer_scope */
}

int
slang_struct_scope_copy(slang_struct_scope * x, const slang_struct_scope * y)
{
    slang_struct_scope z;
    const GLuint n = static_cast<GLuint>(y->structs.size());
    GLuint i;

    z.structs.resize(n);
    for (i = 0; i < n; i++) {
	if (!slang_struct_construct(&z.structs[i])) {
	    slang_struct_scope_destruct(&z);
	    return 0;
	}
    }
    for (i = 0; i < n; i++) {
	if (!slang_struct_copy(&z.structs[i], &y->structs[i])) {
	    slang_struct_scope_destruct(&z);
	    return 0;
	}
    }
    z.outer_scope = y->outer_scope;
    slang_struct_scope_destruct(x);
    *x = std::move(z);
    return 1;
}

slang_struct *
slang_struct_scope_find(slang_struct_scope * stru, slang_atom a_name,
			int all_scopes)
{
    for (auto &s : stru->structs)
	if (a_name == s.a_name)
	    return &s;
    if (all_scopes && stru->outer_scope != nullptr)
	return slang_struct_scope_find(stru->outer_scope, a_name, 1);
    return nullptr;
}

/* slang_struct */

int
slang_struct_construct(slang_struct * stru)
{
    stru->a_name = SLANG_ATOM_NULL;
    stru->fields = new slang_variable_scope;
    _slang_variable_scope_ctr(stru->fields);

    stru->structs = new slang_struct_scope;
    _slang_struct_scope_ctr(stru->structs);
    return 1;
}

void
slang_struct_destruct(slang_struct * stru)
{
    slang_variable_scope_destruct(stru->fields);
    delete stru->fields;
    stru->fields = nullptr;
    slang_struct_scope_destruct(stru->structs);
    delete stru->structs;
    stru->structs = nullptr;
}

int
slang_struct_copy(slang_struct * x, const slang_struct * y)
{
    slang_struct z;

    if (!slang_struct_construct(&z))
	return 0;
    z.a_name = y->a_name;
    if (!slang_variable_scope_copy(z.fields, y->fields)) {
	slang_struct_destruct(&z);
	return 0;
    }
    if (!slang_struct_scope_copy(z.structs, y->structs)) {
	slang_struct_destruct(&z);
	return 0;
    }
    slang_struct_destruct(x);
    *x = z;
    return 1;
}

int
slang_struct_equal(const slang_struct * x, const slang_struct * y)
{
    if (x->fields->variables.size() != y->fields->variables.size())
	return 0;

    for (GLuint i = 0; i < x->fields->variables.size(); i++) {
	const slang_variable *varx = x->fields->variables[i];
	const slang_variable *vary = y->fields->variables[i];

	if (varx->a_name != vary->a_name)
	    return 0;
	if (!slang_type_specifier_equal(&varx->type.specifier,
					&vary->type.specifier))
	    return 0;
	if (varx->type.specifier.type == SLANG_SPEC_ARRAY)
	    if (varx->array_len != vary->array_len)
		return GL_FALSE;
    }
    return 1;
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
