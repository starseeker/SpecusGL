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
 * \file slang_utility.c
 * slang utilities
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_mem.h"

char *
slang_string_concat(char *dst, const char *src)
{
    return strcpy(dst + strlen(dst), src);
}


/* slang_atom_pool */

/**
 * Initialise the atom pool.
 *
 * C++17: the std::unordered_set member is default-initialised; this
 * function is retained to preserve the existing call-site API.
 */
void
slang_atom_pool_construct(slang_atom_pool * pool)
{
    pool->strings.clear();
}

/**
 * Destroy the atom pool.
 *
 * C++17: clearing the std::unordered_set releases all stored strings.
 */
void
slang_atom_pool_destruct(slang_atom_pool * pool)
{
    pool->strings.clear();
    pool->strings.rehash(0); /* release bucket storage */
}

/**
 * Search for or intern a string in the atom pool.
 *
 * Returns a stable pointer to the interned copy of \p id, which serves as
 * the atom's unique identity (pointer equality test).
 *
 * C++17: std::unordered_set guarantees that iterators (and therefore the
 * .c_str() pointers they expose) are not invalidated by further insertions,
 * so the returned pointer remains valid for the lifetime of the pool.
 */
slang_atom
slang_atom_pool_atom(slang_atom_pool * pool, const char * id)
{
    auto [it, _] = pool->strings.emplace(id);
    /* The pool's interface contracts that the atom is the string pointer.
     * const_cast is safe here: callers only read through the atom. */
    return static_cast<slang_atom>(const_cast<char *>(it->c_str()));
}

/**
 * Return the name of a given atom.
 */
const char *
slang_atom_pool_id(slang_atom_pool * pool, slang_atom atom)
{
    (void) pool;
    return static_cast<const char *>(atom);
}
