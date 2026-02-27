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
 * \file slang_compile_operation.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"
#include <algorithm>
#include <memory>


/**
 * Init a slang_operation object
 */
slang_operation::slang_operation()
    : type(SLANG_OPER_NONE), literal{0.0f, 0.0f, 0.0f, 0.0f},
      literal_size(1), a_id(SLANG_ATOM_NULL),
      locals(std::make_unique<slang_variable_scope>()),
      fun(nullptr), var(nullptr), label(nullptr)
{
    /* locals is default-constructed (empty variables, null outer_scope) */
}

bool
slang_operation_construct(slang_operation *oper)
{
    return oper->locals != nullptr ? true : false;
}

void
slang_operation_destruct(slang_operation *oper)
{
    /* Reset all fields to a clean state, equivalent to re-constructing.
     * children and locals unique_ptr handle their own cleanup. */
    oper->children.clear();
    oper->locals.reset();
}

/**
 * Recursively copy a slang_operation node.
 * \return true for success, false if failure
 */
bool
slang_operation_copy(slang_operation *x, const slang_operation *y)
{
    slang_operation z;

    z.type = y->type;
    z.children.resize(y->children.size());
    for (GLuint i = 0; i < static_cast<GLuint>(y->children.size()); i++) {
        if (!slang_operation_copy(&z.children[i], &y->children[i])) {
            return false;
        }
    }
    std::copy(std::begin(y->literal), std::end(y->literal), z.literal);
    z.literal_size = y->literal_size;
    assert(y->literal_size >= 1);
    assert(y->literal_size <= 4);
    z.a_id = y->a_id;
    if (y->locals) {
        if (!slang_variable_scope_copy(z.locals.get(), y->locals.get())) {
            return false;
        }
    }
    *x = std::move(z);   /* move assignment destroys x's old state, then transfers z */
    return true;
}


slang_operation *
slang_operation_new()
{
    return new slang_operation;
}


/**
 * Delete operation and all children
 */
void
slang_operation_delete(slang_operation *oper)
{
    delete oper;
}


slang_operation *
slang_operation_grow(slang_operation *parent)
{
    parent->children.emplace_back();
    return &parent->children.back();
}

/**
 * Insert a new slang_operation into an array.
 * \param parent  the parent operation
 * \param pos  position to insert new element
 * \return  pointer to the new operation/element
 */
slang_operation *
slang_operation_insert(slang_operation *parent, GLuint pos)
{
    assert(pos <= parent->children.size());
    parent->children.emplace(parent->children.begin() + pos);
    return &parent->children[pos];
}


void
_slang_operation_swap(slang_operation *oper0, slang_operation *oper1)
{
    std::swap(*oper0, *oper1);
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
