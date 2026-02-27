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
 * \file slang_mem.c
 *
 * Memory manager for GLSL compiler.  The general idea is to do all
 * allocations out of a large pool then just free the pool when done
 * compiling to avoid intricate malloc/free tracking and memory leaks.
 *
 * C++17 modernisation: the raw char* buffers (allocated with calloc/free) have
 * been replaced by std::vector<char> which is zero-initialised on
 * construction.  The linked chain of overflow blocks is now owned by
 * std::unique_ptr so that _slang_delete_mempool only needs a single
 * "delete pool" and the entire chain unwinds automatically.
 *
 * \author Brian Paul
 */

#include "context.h"
#include "macros.h"
#include "slang_mem.h"
#include <cstring>
#include <new>


#define GRANULARITY 8
#define ROUND_UP(B)  ( ((B) + (GRANULARITY - 1)) & ~(GRANULARITY - 1) )


/** If 1, use conventional malloc/free.  Helpful for debugging */
#define USE_MALLOC_FREE 0


slang_mempool *
_slang_new_mempool(GLuint initialSize)
{
    try {
        return new slang_mempool(initialSize);
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}


void
_slang_delete_mempool(slang_mempool *pool)
{
    /* Deleting the head cascades through the unique_ptr chain,
     * automatically releasing every overflow block. */
    delete pool;
}


#ifdef DEBUG
static void
check_zero(const char *addr, GLuint n)
{
    GLuint i;
    for (i = 0; i < n; i++) {
	assert(addr[i] == 0);
    }
}
#endif


#ifdef DEBUG
static GLboolean
is_valid_address(const slang_mempool *pool, void *addr)
{
    while (pool) {
	if (reinterpret_cast<const char *>(addr) >= pool->data.data() &&
	    reinterpret_cast<const char *>(addr) < pool->data.data() + pool->used)
	    return GL_TRUE;
	pool = pool->next.get();
    }
    return GL_FALSE;
}
#endif


/**
 * Alloc 'bytes' from shader mempool.
 */
void *
_slang_alloc(GLuint bytes)
{
#if USE_MALLOC_FREE
    return calloc(1, bytes);
#else
    slang_mempool *pool;
    GET_CURRENT_CONTEXT(ctx);
    pool = static_cast<slang_mempool *>(ctx->Shader.MemPool);

    if (bytes == 0)
	bytes = 1;

    while (pool) {
	if (pool->used + bytes <= static_cast<GLuint>(pool->data.size())) {
	    /* found room in this block */
	    void *addr = static_cast<void *>(pool->data.data() + pool->used);
#ifdef DEBUG
	    check_zero(reinterpret_cast<char *>(addr), bytes);
#endif
	    pool->used += ROUND_UP(bytes);
	    pool->largest = MAX2(pool->largest, bytes);
	    pool->count++;
	    return addr;
	} else if (pool->next) {
	    /* try next block */
	    pool = pool->next.get();
	} else {
	    /* allocate a new overflow block */
	    const GLuint sz = MAX2(bytes, static_cast<GLuint>(pool->data.size()));
	    try {
		pool->next.reset(new slang_mempool(sz));
	    } catch (const std::bad_alloc &) {
		return nullptr;
	    }
	    pool = pool->next.get();
	    pool->largest = bytes;
	    pool->count++;
	    pool->used = ROUND_UP(bytes);
#ifdef DEBUG
	    check_zero(pool->data.data(), bytes);
#endif
	    return static_cast<void *>(pool->data.data());
	}
    }
    return nullptr;
#endif
}


void *
_slang_realloc(void *oldBuffer, GLuint oldSize, GLuint newSize)
{
#if USE_MALLOC_FREE
    return std::realloc(oldBuffer, newSize);
#else
    if (newSize < oldSize) {
	return oldBuffer;
    } else {
	const GLuint copySize = (oldSize < newSize) ? oldSize : newSize;
	void *newBuffer = _slang_alloc(newSize);

	if (newBuffer && oldBuffer && copySize > 0)
	    std::memcpy(newBuffer, oldBuffer, copySize);

	return newBuffer;
    }
#endif
}


/**
 * Clone string, storing in current mempool.
 */
char *
_slang_strdup(const char *s)
{
    if (s) {
	const std::size_t len = std::strlen(s);
	char *s2 = reinterpret_cast<char *>(_slang_alloc(static_cast<GLuint>(len + 1)));
	if (s2)
	    std::memcpy(s2, s, len + 1);
	return s2;
    } else {
	return nullptr;
    }
}


/**
 * Don't actually free memory (pool is freed all at once), but mark it
 * as unused in debug builds.
 */
void
_slang_free(void *addr)
{
#if USE_MALLOC_FREE
    free(addr);
#else
    (void) addr; /* intentional no-op: pool freed en-masse by _slang_delete_mempool */
#endif
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
