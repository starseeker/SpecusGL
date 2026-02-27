/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * \file math/m_matrix.h
 * Defines basic structures for matrix-handling.
 */

#ifndef _M_MATRIX_H
#define _M_MATRIX_H






/**
 * \name Symbolic names to some of the entries in the matrix
 *
 * These are handy for the viewport mapping, which is expressed as a matrix.
 */
/*@{*/
constexpr int MAT_SX = 0;
constexpr int MAT_SY = 5;
constexpr int MAT_SZ = 10;
constexpr int MAT_TX = 12;
constexpr int MAT_TY = 13;
constexpr int MAT_TZ = 14;
/*@}*/


/**
 * Different kinds of 4x4 transformation matrices.
 * We use these to select specific optimized vertex transformation routines.
 */
enum GLmatrixtype {
    MATRIX_GENERAL,	/**< general 4x4 matrix */
    MATRIX_IDENTITY,	/**< identity matrix */
    MATRIX_3D_NO_ROT,	/**< orthogonal projection and others... */
    MATRIX_PERSPECTIVE,	/**< perspective projection matrix */
    MATRIX_2D,		/**< 2-D transformation */
    MATRIX_2D_NO_ROT,	/**< 2-D scale & translate only */
    MATRIX_3D		/**< 3-D transformation */
} ;

/**
 * Matrix type to represent 4x4 transformation matrices.
 *
 * C++17 modernisation: mutation operations are now also available as member
 * functions so that callers can write \c mat->translate(x,y,z) instead of
 * \c _math_matrix_translate(mat,x,y,z).  The legacy free-function forms are
 * kept as thin inline wrappers for compatibility.
 */
struct GLmatrix {
    GLfloat *m;		/**< 16 matrix elements (16-byte aligned) */
    GLfloat *inv;	/**< optional 16-element inverse (16-byte aligned) */
    GLuint flags;        /**< possible values determined by (of \link
                         * MatFlags MAT_FLAG_* flags\endlink)
                         */
    enum GLmatrixtype type;

    GLmatrix();   /**< Initialises m to identity, inv to nullptr. */
    ~GLmatrix();  /**< Releases aligned m and inv allocations. */

    /** Allocate (or reallocate) the inverse array. */
    void alloc_inv();

    /** Multiply this matrix by matrix \p b (this = this * b). */
    void mul(const GLmatrix *b);

    /** Multiply this matrix by a raw 16-float column-major matrix. */
    void mul(const GLfloat *b);

    /** Load this matrix from a raw 16-float column-major array. */
    void load(const GLfloat *src);

    /** Apply a translation to this matrix. */
    void translate(GLfloat x, GLfloat y, GLfloat z);

    /** Apply a rotation of \p angle degrees about axis (x,y,z). */
    void rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);

    /** Apply a non-uniform scale. */
    void scale(GLfloat x, GLfloat y, GLfloat z);

    /** Set up an orthographic projection. */
    void ortho(GLfloat left, GLfloat right,
               GLfloat bottom, GLfloat top,
               GLfloat nearval, GLfloat farval);

    /** Set up a perspective (frustum) projection. */
    void frustum(GLfloat left, GLfloat right,
                 GLfloat bottom, GLfloat top,
                 GLfloat nearval, GLfloat farval);

    /** Set up a viewport mapping transformation. */
    void viewport(GLint x, GLint y, GLint width, GLint height,
                  GLfloat zNear, GLfloat zFar, GLfloat depthMax);

    /** Reset this matrix to the identity. */
    void set_identity();

    /** Copy \p from into this matrix (including inv if present). */
    void copy_from(const GLmatrix *from);

    /** (Re-)analyse the matrix to update type and flags. */
    void analyse();

    /** Print the matrix to stdout (debug helper). */
    void print() const;

    /** \name Query predicates (const) */
    /*@{*/
    [[nodiscard]] bool is_length_preserving() const;
    [[nodiscard]] bool has_rotation() const;
    [[nodiscard]] bool is_general_scale() const;
    [[nodiscard]] bool is_dirty() const;
    /*@}*/

private:
    /**
     * \name Internal helpers – not part of the public interface.
     * These encapsulate the per-format inversion and analysis routines
     * that were previously static free functions in m_matrix.cpp.
     */
    /*@{*/

    /** Apply a precomputed 4×4 matrix \p fm with extra flags \p fl. */
    void multf(const GLfloat *fm, GLuint fl);

    /** Compute the inverse and update the matrix. */
    bool invert();

    /** Determine type and flags from scratch (expensive path). */
    void analyse_from_scratch();

    /** Determine type from flags (fast path, flags already known-good). */
    void analyse_from_flags();

    /*@}*/
};

static_assert(sizeof(GLfloat) == 4, "GLfloat must be 32-bit for 16-byte aligned matrix arrays");


/**
 * Multiply two matrices: \c dest = \c a * \c b.
 *
 * This free function remains as-is because the three operands are distinct
 * and there is no natural "this" matrix to call the method on.
 */
extern void
_math_matrix_mul_matrix(GLmatrix *dest, const GLmatrix *a, const GLmatrix *b);


/**
 * \name Legacy free-function API (thin wrappers around GLmatrix methods).
 *
 * These exist for backward compatibility.  New code should call the
 * equivalent GLmatrix member function directly.
 */
/*@{*/
inline void _math_matrix_alloc_inv(GLmatrix *m)            { m->alloc_inv(); }
inline void _math_matrix_mul_floats(GLmatrix *dest, const GLfloat *b) { dest->mul(b); }
inline void _math_matrix_loadf(GLmatrix *mat, const GLfloat *m)       { mat->load(m); }
inline void _math_matrix_translate(GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z)
                                                                       { mat->translate(x, y, z); }
inline void _math_matrix_rotate(GLmatrix *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
                                                                       { m->rotate(angle, x, y, z); }
inline void _math_matrix_scale(GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z)
                                                                       { mat->scale(x, y, z); }
inline void _math_matrix_ortho(GLmatrix *mat,
                               GLfloat left, GLfloat right,
                               GLfloat bottom, GLfloat top,
                               GLfloat nearval, GLfloat farval)
                                                                       { mat->ortho(left, right, bottom, top, nearval, farval); }
inline void _math_matrix_frustum(GLmatrix *mat,
                                 GLfloat left, GLfloat right,
                                 GLfloat bottom, GLfloat top,
                                 GLfloat nearval, GLfloat farval)
                                                                       { mat->frustum(left, right, bottom, top, nearval, farval); }
inline void _math_matrix_viewport(GLmatrix *m, GLint x, GLint y, GLint width, GLint height,
                                  GLfloat zNear, GLfloat zFar, GLfloat depthMax)
                                                                       { m->viewport(x, y, width, height, zNear, zFar, depthMax); }
inline void _math_matrix_set_identity(GLmatrix *dest)                  { dest->set_identity(); }
inline void _math_matrix_copy(GLmatrix *to, const GLmatrix *from)     { to->copy_from(from); }
inline void _math_matrix_analyse(GLmatrix *mat)                        { mat->analyse(); }
inline void _math_matrix_print(const GLmatrix *m)                     { m->print(); }
inline GLboolean _math_matrix_is_length_preserving(const GLmatrix *m) { return m->is_length_preserving() ? GL_TRUE : GL_FALSE; }
inline GLboolean _math_matrix_has_rotation(const GLmatrix *m)         { return m->has_rotation() ? GL_TRUE : GL_FALSE; }
inline GLboolean _math_matrix_is_general_scale(const GLmatrix *m)     { return m->is_general_scale() ? GL_TRUE : GL_FALSE; }
inline GLboolean _math_matrix_is_dirty(const GLmatrix *m)             { return m->is_dirty() ? GL_TRUE : GL_FALSE; }
/*@}*/


/**
 * \name Related functions that don't actually operate on GLmatrix structs
 */
/*@{*/

extern void
_math_transposef(GLfloat to[16], const GLfloat from[16]);

extern void
_math_transposed(GLdouble to[16], const GLdouble from[16]);

extern void
_math_transposefd(GLfloat to[16], const GLdouble from[16]);


/*
 * Transform a point (column vector) by a matrix:   Q = M * P
 */
#define TRANSFORM_POINT( Q, M, P )					\
   Q[0] = M[0] * P[0] + M[4] * P[1] + M[8] *  P[2] + M[12] * P[3];	\
   Q[1] = M[1] * P[0] + M[5] * P[1] + M[9] *  P[2] + M[13] * P[3];	\
   Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14] * P[3];	\
   Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15] * P[3];


#define TRANSFORM_POINT3( Q, M, P )				\
   Q[0] = M[0] * P[0] + M[4] * P[1] + M[8] *  P[2] + M[12];	\
   Q[1] = M[1] * P[0] + M[5] * P[1] + M[9] *  P[2] + M[13];	\
   Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14];	\
   Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15];


/*
 * Transform a normal (row vector) by a matrix:  [NX NY NZ] = N * MAT
 */
#define TRANSFORM_NORMAL( TO, N, MAT )				\
do {								\
   TO[0] = N[0] * MAT[0] + N[1] * MAT[1] + N[2] * MAT[2];	\
   TO[1] = N[0] * MAT[4] + N[1] * MAT[5] + N[2] * MAT[6];	\
   TO[2] = N[0] * MAT[8] + N[1] * MAT[9] + N[2] * MAT[10];	\
} while (0)


/*@}*/




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
