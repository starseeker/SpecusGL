
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/*
 * New (3.1) transformation code written by Keith Whitwell.
 */


#ifndef _M_VECTOR_H_
#define _M_VECTOR_H_

#include "glheader.h"
#include "mtypes.h"		/* hack for GLchan */




#define VEC_DIRTY_0        0x1
#define VEC_DIRTY_1        0x2
#define VEC_DIRTY_2        0x4
#define VEC_DIRTY_3        0x8
#define VEC_MALLOC         0x10 /* storage field points to self-allocated mem*/
#define VEC_NOT_WRITEABLE  0x40	/* writable elements to hold clipped data */
#define VEC_BAD_STRIDE     0x100 /* matches tnl's prefered stride */


#define VEC_SIZE_1   VEC_DIRTY_0
#define VEC_SIZE_2   (VEC_DIRTY_0|VEC_DIRTY_1)
#define VEC_SIZE_3   (VEC_DIRTY_0|VEC_DIRTY_1|VEC_DIRTY_2)
#define VEC_SIZE_4   (VEC_DIRTY_0|VEC_DIRTY_1|VEC_DIRTY_2|VEC_DIRTY_3)



/**
 * A 4-component float vector array used throughout the T&L pipeline.
 *
 * C++17 modernisation: the struct now owns its storage via RAII.
 * Callers may still use the legacy free-function wrappers
 * (_mesa_vector4f_alloc, _mesa_vector4f_free, etc.) which are kept for
 * backward compatibility but are now thin wrappers around member methods.
 *
 * The start field is used to reserve data for copied vertices at the
 * end of _mesa_transform_vb, and avoids the need for a multiplication in
 * the transformation routines.
 */
struct GLvector4f {
    GLfloat(*data)[4] = nullptr;  /**< may be self-alloc'd or point to client data */
    GLfloat *start    = nullptr;  /**< points somewhere inside <data> */
    GLuint count      = 0;        /**< number of elements */
    GLuint stride     = 0;        /**< bytes from one element to the next */
    GLuint size       = 0;        /**< 2-4 for vertices, 1-4 for texcoords */
    GLuint flags      = 0;        /**< VEC_* flags (VEC_MALLOC, VEC_DIRTY_*, etc.) */
    void  *storage    = nullptr;  /**< self-allocated aligned storage (VEC_MALLOC) */

    /** Default constructor – zero-initialised, owns no storage. */
    GLvector4f() = default;

    /**
     * Destructor – releases self-allocated storage (VEC_MALLOC flag).
     * Safe to call on vectors that were only initialised with external storage.
     */
    ~GLvector4f();

    /** GLvector4f is non-copyable to prevent double-free of owned storage. */
    GLvector4f(const GLvector4f &) = delete;
    GLvector4f &operator=(const GLvector4f &) = delete;

    /** Move constructor – transfers ownership and resets the source. */
    GLvector4f(GLvector4f &&o) noexcept;
    GLvector4f &operator=(GLvector4f &&o) noexcept;

    /**
     * Initialise this vector to point at externally-owned storage.
     * Equivalent to the old _mesa_vector4f_init() free-function.
     */
    void init(GLuint flags_in, GLfloat (*ext_storage)[4]);

    /**
     * Allocate aligned self-owned storage for \p count elements.
     * Sets the VEC_MALLOC flag; the destructor will release this memory.
     * Equivalent to the old _mesa_vector4f_alloc() free-function.
     */
    void alloc(GLuint flags_in, GLuint count_in, GLuint alignment);

    /**
     * Release self-owned storage (if VEC_MALLOC is set) and reset all fields.
     * Calling this is optional when the destructor will run, but callers that
     * need to release memory early (e.g., to re-alloc at a different size) can
     * use it explicitly.  Equivalent to the old _mesa_vector4f_free().
     */
    void free();
};


/* --------------------------------------------------------------------- */
/* Legacy free-function wrappers – kept for backward compatibility.       */
/* New code should call the member methods directly.                       */
/* --------------------------------------------------------------------- */

inline void _mesa_vector4f_init(GLvector4f *v, GLuint flags,
                                GLfloat (*storage)[4])
{
    v->init(flags, storage);
}

inline void _mesa_vector4f_alloc(GLvector4f *v, GLuint flags,
                                 GLuint count, GLuint alignment)
{
    v->alloc(flags, count, alignment);
}

inline void _mesa_vector4f_free(GLvector4f *v)
{
    v->free();
}

extern void _mesa_vector4f_print(GLvector4f *v, GLubyte *, GLboolean);
extern void _mesa_vector4f_clean_elem(GLvector4f *vec, GLuint nr, GLuint elt);





/*
 * Given vector <v>, return a pointer (cast to <type *> to the <i>-th element.
 *
 * End up doing a lot of slow imuls if not careful.
 */
#define VEC_ELT( v, type, i ) \
       ( (type *)  ( ((GLbyte *) ((v)->data)) + (i) * (v)->stride) )




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
