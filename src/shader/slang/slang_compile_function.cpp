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
 * \file slang_compile_function.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"
#include "slang_mem.h"

/* slang_fixup_table */

/* slang_fixup_table_init and slang_fixup_table_free are now inline in the header. */

/**
 * Add a new fixup address to the table.
 */
GLboolean
slang_fixup_save(slang_fixup_table *fixups, GLuint address)
{
    fixups->table.push_back(address);
    return GL_TRUE;
}



/* slang_function */

int
slang_function_construct(slang_function * func)
{
    func->kind = SLANG_FUNC_ORDINARY;
    if (!slang_variable_construct(&func->header))
	return 0;

    func->parameters = (slang_variable_scope *)
		       _slang_alloc(sizeof(slang_variable_scope));
    if (func->parameters == nullptr) {
	slang_variable_destruct(&func->header);
	return 0;
    }

    _slang_variable_scope_ctr(func->parameters);
    func->param_count = 0;
    func->body = nullptr;
    func->address = ~0;
    slang_fixup_table_init(&func->fixups);
    return 1;
}

void
slang_function_destruct(slang_function * func)
{
    slang_variable_destruct(&func->header);
    slang_variable_scope_destruct(func->parameters);
    _slang_free(func->parameters);
    if (func->body != nullptr) {
	slang_operation_destruct(func->body);
	_slang_free(func->body);
    }
    slang_fixup_table_free(&func->fixups);
}

/*
 * slang_function_scope
 */

GLvoid
_slang_function_scope_ctr(slang_function_scope * self)
{
    self->outer_scope = nullptr;
}

void
slang_function_scope_destruct(slang_function_scope * scope)
{
    for (auto &f : scope->functions)
	slang_function_destruct(&f);
    scope->functions.clear();
}


/**
 * Does this function have a non-void return value?
 */
GLboolean
_slang_function_has_return_value(const slang_function *fun)
{
    return fun->header.type.specifier.type != SLANG_SPEC_VOID;
}


/**
 * Search a list of functions for a particular function by name.
 * \param funcs  the list of functions to search
 * \param a_name  the name to search for
 * \param all_scopes  if non-zero, search containing scopes too.
 * \return pointer to found function, or nullptr.
 */
int
slang_function_scope_find_by_name(slang_function_scope * funcs,
				  slang_atom a_name, int all_scopes)
{
    for (const auto &f : funcs->functions)
	if (a_name == f.header.a_name)
	    return 1;
    if (all_scopes && funcs->outer_scope != nullptr)
	return slang_function_scope_find_by_name(funcs->outer_scope, a_name, 1);
    return 0;
}


/**
 * Search a list of functions for a particular function (for implementing
 * function calls.  Matching is done by first comparing the function's name,
 * then the function's parameter list.
 *
 * \param funcs  the list of functions to search
 * \param fun  the function to search for
 * \param all_scopes  if non-zero, search containing scopes too.
 * \return pointer to found function, or nullptr.
 */
slang_function *
slang_function_scope_find(slang_function_scope * funcs, slang_function * fun,
			  int all_scopes)
{
    for (auto &f : funcs->functions) {
	const GLuint haveRetValue = 0;
#if 0
	    = (f.header.type.specifier.type != SLANG_SPEC_VOID);
#endif
	unsigned int j;

	if (fun->header.a_name != f.header.a_name)
	    continue;
	if (fun->param_count != f.param_count)
	    continue;
	for (j = haveRetValue; j < fun->param_count; j++) {
	    if (!slang_type_specifier_equal
		(&fun->parameters->variables[j]->type.specifier,
		 &f.parameters->variables[j]->type.specifier))
		break;
	}
	if (j == fun->param_count) {
	    return &f;
	}
    }
    if (all_scopes && funcs->outer_scope != nullptr)
	return slang_function_scope_find(funcs->outer_scope, fun, 1);
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
