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


#ifndef SLANG_MEM_H
#define SLANG_MEM_H


#include "imports.h"
#include <memory>
#include <vector>


/**
 * Memory-pool block.
 *
 * C++17 modernisation: the raw char* + calloc/free pair has been replaced
 * with a std::vector<char> (zero-initialised on construction) and the
 * linked-list chain is managed via std::unique_ptr so that the whole
 * chain is freed automatically when the head is deleted.
 */
struct slang_mempool_ {
    std::vector<char>               data;    /**< zero-initialised storage block  */
    GLuint                          used{0};    /**< bytes consumed from this block  */
    GLuint                          count{0};   /**< allocation count (all blocks)   */
    GLuint                          largest{0}; /**< largest single allocation       */
    std::unique_ptr<slang_mempool_> next;    /**< overflow block (or nullptr)     */

    explicit slang_mempool_(GLuint initial_size)
        : data(initial_size, '\0') {}

    /* non-copyable */
    slang_mempool_(const slang_mempool_ &) = delete;
    slang_mempool_ &operator=(const slang_mempool_ &) = delete;
};

typedef struct slang_mempool_ slang_mempool;


extern slang_mempool *
_slang_new_mempool(GLuint initialSize);

extern void
_slang_delete_mempool(slang_mempool *pool);

extern void *
_slang_alloc(GLuint bytes);

extern void *
_slang_realloc(void *oldBuffer, GLuint oldSize, GLuint newSize);

extern char *
_slang_strdup(const char *s);

extern void
_slang_free(void *addr);




#endif

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
