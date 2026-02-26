/**
 * \file macros.h
 * A collection of useful macros.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef MACROS_H
#define MACROS_H




#include "imports.h"


/**
 * \name Integer / float conversion for colors, normals, etc.
 */
/*@{*/

/** Convert GLubyte in [0,255] to GLfloat in [0.0,1.0] */
extern GLfloat _mesa_ubyte_to_float_color_tab[256];
#define UBYTE_TO_FLOAT(u) _mesa_ubyte_to_float_color_tab[(unsigned int)(u)]

/** Convert GLfloat in [0.0,1.0] to GLubyte in [0,255] */
#define FLOAT_TO_UBYTE(X)   ((GLubyte) (GLint) ((X) * 255.0F))


/** Convert GLbyte in [-128,127] to GLfloat in [-1.0,1.0] */
#define BYTE_TO_FLOAT(B)    ((2.0F * (B) + 1.0F) * (1.0F/255.0F))

/** Convert GLfloat in [-1.0,1.0] to GLbyte in [-128,127] */
#define FLOAT_TO_BYTE(X)    ( (((GLint) (255.0F * (X))) - 1) / 2 )


/** Convert GLushort in [0,65536] to GLfloat in [0.0,1.0] */
#define USHORT_TO_FLOAT(S)  ((GLfloat) (S) * (1.0F / 65535.0F))

/** Convert GLshort in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

/** Convert GLfloat in [0.0,1.0] to GLshort in [-32768,32767] */
#define FLOAT_TO_SHORT(X)   ( (((GLint) (65535.0F * (X))) - 1) / 2 )


/** Convert GLuint in [0,4294967295] to GLfloat in [0.0,1.0] */
#define UINT_TO_FLOAT(U)    ((GLfloat) (U) * (1.0F / 4294967295.0F))

/** Convert GLfloat in [0.0,1.0] to GLuint in [0,4294967295] */
#define FLOAT_TO_UINT(X)    ((GLuint) ((X) * 4294967295.0))


/** Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
#define INT_TO_FLOAT(I)     ((2.0F * (I) + 1.0F) * (1.0F/4294967294.0F))

/** Convert GLfloat in [-1.0,1.0] to GLint in [-2147483648,2147483647] */
/* causes overflow:
#define FLOAT_TO_INT(X)     ( (((GLint) (4294967294.0F * (X))) - 1) / 2 )
*/
/* a close approximation: */
#define FLOAT_TO_INT(X)     ( (GLint) (2147483647.0 * (X)) )


#define BYTE_TO_UBYTE(b)   ((GLubyte) ((b) < 0 ? 0 : (GLubyte) (b)))
#define SHORT_TO_UBYTE(s)  ((GLubyte) ((s) < 0 ? 0 : (GLubyte) ((s) >> 7)))
#define USHORT_TO_UBYTE(s) ((GLubyte) ((s) >> 8))
#define INT_TO_UBYTE(i)    ((GLubyte) ((i) < 0 ? 0 : (GLubyte) ((i) >> 23)))
#define UINT_TO_UBYTE(i)   ((GLubyte) ((i) >> 24))


#define BYTE_TO_USHORT(b)  ((b) < 0 ? 0 : ((GLushort) (((b) * 65535) / 255)))
#define UBYTE_TO_USHORT(b) (((GLushort) (b) << 8) | (GLushort) (b))
#define SHORT_TO_USHORT(s) ((s) < 0 ? 0 : ((GLushort) (((s) * 65535 / 32767))))
#define INT_TO_USHORT(i)   ((i) < 0 ? 0 : ((GLushort) ((i) >> 15)))
#define UINT_TO_USHORT(i)  ((i) < 0 ? 0 : ((GLushort) ((i) >> 16)))
#define UNCLAMPED_FLOAT_TO_USHORT(us, f)  \
        us = ( (GLushort) IROUND( CLAMP((f), 0.0, 1.0) * 65535.0F) )
#define CLAMPED_FLOAT_TO_USHORT(us, f)  \
        us = ( (GLushort) IROUND( (f) * 65535.0F) )

/*@}*/

/**
 * Helper for pointer stride stepping.
 *
 * Advances pointer \p p by \p stride bytes, preserving const-ness of the
 * pointee.  This replaces the old C-style cast macros which silently stripped
 * const when used with pointers-to-const data.
 */
template<typename T>
[[nodiscard]] inline T* mesa_stride_ptr(T* p, std::ptrdiff_t stride) noexcept {
    return reinterpret_cast<T*>(reinterpret_cast<char*>(p) + stride);
}
template<typename T>
[[nodiscard]] inline const T* mesa_stride_ptr(const T* p, std::ptrdiff_t stride) noexcept {
    return reinterpret_cast<const T*>(reinterpret_cast<const char*>(p) + stride);
}

/** Stepping a GLfloat pointer by a byte stride */
#define STRIDE_F(p, i)      (p = mesa_stride_ptr(p, i))
/** Stepping a GLuint pointer by a byte stride */
#define STRIDE_UI(p, i)     (p = mesa_stride_ptr(p, i))
/** Stepping a GLubyte[4] pointer by a byte stride */
#define STRIDE_4UB(p, i)    (p = mesa_stride_ptr(p, i))
/** Stepping a GLfloat[4] pointer by a byte stride */
#define STRIDE_4F(p, i)     (p = mesa_stride_ptr(p, i))
/** Stepping a GLchan[4] pointer by a byte stride */
#define STRIDE_4CHAN(p, i)  (p = mesa_stride_ptr(p, i))
/** Stepping a GLchan pointer by a byte stride */
#define STRIDE_CHAN(p, i)   (p = mesa_stride_ptr(p, i))
/** Stepping a \p t pointer by a byte stride */
#define STRIDE_T(p, t, i)  (p = reinterpret_cast<t>(reinterpret_cast<char *>(p) + (i)))


/*
 * C++17 inline function implementations for vector operations.
 *
 * These replace the old do-while macro bodies.  The macros below are kept
 * as thin wrappers so that all existing call sites continue to compile
 * without modification.  New code should call the mesa_* functions directly.
 */

/**********************************************************************/
/** \name 4-element vector operations */
/*@{*/

/** Zero all four elements. */
template<typename T>
inline void mesa_zero4v(T* v) noexcept { v[0] = v[1] = v[2] = v[3] = T(0); }

/** Test element-wise equality of two 4-element vectors. */
template<typename T, typename U>
[[nodiscard]] inline bool mesa_test_eq_4v(const T* a, const U* b) noexcept {
    return a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3];
}

/** Copy a 4-element vector (element-wise assignment). */
template<typename Dst, typename Src>
inline void mesa_copy4v(Dst* dst, const Src* src) noexcept {
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
}

/**
 * Copy a 4-element float vector using memcpy (avoids FPU registers and
 * the type-punning UB of the original uint-cast approach).
 */
inline void mesa_copy4fv(GLfloat* dst, const GLfloat* src) noexcept {
    std::memcpy(dst, src, 4 * sizeof(GLfloat));
}

/** Copy \p sz elements (1-4) into a 4-element vector (higher unused). */
template<typename Dst, typename Src>
inline void mesa_copy_sz_4v(Dst* dst, int sz, const Src* src) noexcept {
    switch (sz) {
    case 4: dst[3] = src[3]; [[fallthrough]];
    case 3: dst[2] = src[2]; [[fallthrough]];
    case 2: dst[1] = src[1]; [[fallthrough]];
    case 1: dst[0] = src[0];
    }
}

/** Copy \p sz elements into a homogeneous 4-vector; remaining set to (0,0,0,1). */
template<typename Dst, typename Src>
inline void mesa_copy_clean_4v(Dst* dst, int sz, const Src* src) noexcept {
    dst[0] = Dst(0); dst[1] = Dst(0); dst[2] = Dst(0); dst[3] = Dst(1);
    mesa_copy_sz_4v(dst, sz, src);
}

/** Subtraction: dst = a - b */
template<typename Dst, typename A, typename B>
inline void mesa_sub4v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]-b[0]; dst[1]=a[1]-b[1]; dst[2]=a[2]-b[2]; dst[3]=a[3]-b[3];
}

/** Addition: dst = a + b */
template<typename Dst, typename A, typename B>
inline void mesa_add4v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]+b[0]; dst[1]=a[1]+b[1]; dst[2]=a[2]+b[2]; dst[3]=a[3]+b[3];
}

/** Element-wise multiplication: dst = a * b */
template<typename Dst, typename A, typename B>
inline void mesa_scale4v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]*b[0]; dst[1]=a[1]*b[1]; dst[2]=a[2]*b[2]; dst[3]=a[3]*b[3];
}

/** In-place addition: dst += src */
template<typename Dst, typename Src>
inline void mesa_acc4v(Dst* dst, const Src* src) noexcept {
    dst[0]+=src[0]; dst[1]+=src[1]; dst[2]+=src[2]; dst[3]+=src[3];
}

/** Element-wise multiply-accumulate: dst += a * b */
template<typename Dst, typename A, typename B>
inline void mesa_acc_scale4v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]+=a[0]*b[0]; dst[1]+=a[1]*b[1]; dst[2]+=a[2]*b[2]; dst[3]+=a[3]*b[3];
}

/** Scalar multiply-accumulate: dst += s * src */
template<typename Dst, typename S, typename Src>
inline void mesa_acc_scale_scalar4v(Dst* dst, S s, const Src* src) noexcept {
    dst[0]+=s*src[0]; dst[1]+=s*src[1]; dst[2]+=s*src[2]; dst[3]+=s*src[3];
}

/** Scalar multiplication: dst = s * src */
template<typename Dst, typename S, typename Src>
inline void mesa_scale_scalar4v(Dst* dst, S s, const Src* src) noexcept {
    dst[0]=s*src[0]; dst[1]=s*src[1]; dst[2]=s*src[2]; dst[3]=s*src[3];
}

/** In-place scalar multiplication: dst *= s */
template<typename Dst, typename S>
inline void mesa_self_scale_scalar4v(Dst* dst, S s) noexcept {
    dst[0]*=s; dst[1]*=s; dst[2]*=s; dst[3]*=s;
}

/** Assign four scalar values to a vector. */
template<typename V, typename V0, typename V1, typename V2, typename V3>
inline void mesa_assign4v(V* v, V0 v0, V1 v1, V2 v2, V3 v3) noexcept {
    v[0]=v0; v[1]=v1; v[2]=v2; v[3]=v3;
}

/* --- Macro aliases --- */
#define ZERO_4V(DST)              mesa_zero4v(DST)
#define TEST_EQ_4V(a, b)          mesa_test_eq_4v(a, b)
/** Test for equality (unsigned bytes) */
#if defined(__i386__)
#define TEST_EQ_4UBV(DST, SRC) (*reinterpret_cast<const GLuint*>(DST) == *reinterpret_cast<const GLuint*>(SRC))
#else
#define TEST_EQ_4UBV(DST, SRC) mesa_test_eq_4v(DST, SRC)
#endif
#define COPY_4V(DST, SRC)         mesa_copy4v(DST, SRC)
/** Copy a 4-element vector with cast */
#define COPY_4V_CAST( DST, SRC, CAST )  \
do {                                    \
   (DST)[0] = (CAST)(SRC)[0];           \
   (DST)[1] = (CAST)(SRC)[1];           \
   (DST)[2] = (CAST)(SRC)[2];           \
   (DST)[3] = (CAST)(SRC)[3];           \
} while (0)
/** Copy a 4-element unsigned byte vector */
#if defined(__i386__)
#define COPY_4UBV(DST, SRC) \
   (*reinterpret_cast<GLuint*>(DST) = *reinterpret_cast<const GLuint*>(SRC))
#else
#define COPY_4UBV(DST, SRC)  mesa_copy4v(DST, SRC)
#endif
#define COPY_4FV(DST, SRC)        mesa_copy4fv(DST, SRC)
#define COPY_SZ_4V(DST, SZ, SRC)  mesa_copy_sz_4v(DST, SZ, SRC)
#define COPY_CLEAN_4V(DST, SZ, SRC) mesa_copy_clean_4v(DST, SZ, SRC)
#define SUB_4V(DST, SRCA, SRCB)   mesa_sub4v(DST, SRCA, SRCB)
#define ADD_4V(DST, SRCA, SRCB)   mesa_add4v(DST, SRCA, SRCB)
#define SCALE_4V(DST, SRCA, SRCB) mesa_scale4v(DST, SRCA, SRCB)
#define ACC_4V(DST, SRC)          mesa_acc4v(DST, SRC)
#define ACC_SCALE_4V(DST, SRCA, SRCB)       mesa_acc_scale4v(DST, SRCA, SRCB)
#define ACC_SCALE_SCALAR_4V(DST, S, SRCB)   mesa_acc_scale_scalar4v(DST, S, SRCB)
#define SCALE_SCALAR_4V(DST, S, SRCB)       mesa_scale_scalar4v(DST, S, SRCB)
#define SELF_SCALE_SCALAR_4V(DST, S)        mesa_self_scale_scalar4v(DST, S)
#define ASSIGN_4V(V, V0, V1, V2, V3)        mesa_assign4v(V, V0, V1, V2, V3)

/*@}*/


/**********************************************************************/
/** \name 3-element vector operations */
/*@{*/

/** Zero all three elements. */
template<typename T>
inline void mesa_zero3v(T* v) noexcept { v[0] = v[1] = v[2] = T(0); }

/** Test element-wise equality of two 3-element vectors. */
template<typename T, typename U>
[[nodiscard]] inline bool mesa_test_eq_3v(const T* a, const U* b) noexcept {
    return a[0]==b[0] && a[1]==b[1] && a[2]==b[2];
}

/** Copy a 3-element vector. */
template<typename Dst, typename Src>
inline void mesa_copy3v(Dst* dst, const Src* src) noexcept {
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2];
}

/** Copy a 3-element float vector using memcpy. */
inline void mesa_copy3fv(GLfloat* dst, const GLfloat* src) noexcept {
    std::memcpy(dst, src, 3 * sizeof(GLfloat));
}

/** Subtraction: dst = a - b */
template<typename Dst, typename A, typename B>
inline void mesa_sub3v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]-b[0]; dst[1]=a[1]-b[1]; dst[2]=a[2]-b[2];
}

/** Addition: dst = a + b */
template<typename Dst, typename A, typename B>
inline void mesa_add3v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]+b[0]; dst[1]=a[1]+b[1]; dst[2]=a[2]+b[2];
}

/** Element-wise multiplication: dst = a * b */
template<typename Dst, typename A, typename B>
inline void mesa_scale3v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]*b[0]; dst[1]=a[1]*b[1]; dst[2]=a[2]*b[2];
}

/** In-place element-wise multiplication: dst *= src */
template<typename Dst, typename Src>
inline void mesa_self_scale3v(Dst* dst, const Src* src) noexcept {
    dst[0]*=src[0]; dst[1]*=src[1]; dst[2]*=src[2];
}

/** In-place addition: dst += src */
template<typename Dst, typename Src>
inline void mesa_acc3v(Dst* dst, const Src* src) noexcept {
    dst[0]+=src[0]; dst[1]+=src[1]; dst[2]+=src[2];
}

/** Element-wise multiply-accumulate: dst += a * b */
template<typename Dst, typename A, typename B>
inline void mesa_acc_scale3v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]+=a[0]*b[0]; dst[1]+=a[1]*b[1]; dst[2]+=a[2]*b[2];
}

/** Scalar multiplication: dst = s * src */
template<typename Dst, typename S, typename Src>
inline void mesa_scale_scalar3v(Dst* dst, S s, const Src* src) noexcept {
    dst[0]=s*src[0]; dst[1]=s*src[1]; dst[2]=s*src[2];
}

/** Scalar multiply-accumulate: dst += s * src */
template<typename Dst, typename S, typename Src>
inline void mesa_acc_scale_scalar3v(Dst* dst, S s, const Src* src) noexcept {
    dst[0]+=s*src[0]; dst[1]+=s*src[1]; dst[2]+=s*src[2];
}

/** In-place scalar multiplication: dst *= s */
template<typename Dst, typename S>
inline void mesa_self_scale_scalar3v(Dst* dst, S s) noexcept {
    dst[0]*=s; dst[1]*=s; dst[2]*=s;
}

/** In-place scalar addition: dst += s */
template<typename Dst, typename S>
inline void mesa_acc_scalar3v(Dst* dst, S s) noexcept {
    dst[0]+=s; dst[1]+=s; dst[2]+=s;
}

/** Assign three scalar values to a vector. */
template<typename V, typename V0, typename V1, typename V2>
inline void mesa_assign3v(V* v, V0 v0, V1 v1, V2 v2) noexcept {
    v[0]=v0; v[1]=v1; v[2]=v2;
}

/* --- Macro aliases --- */
#define ZERO_3V(DST)              mesa_zero3v(DST)
#define TEST_EQ_3V(a, b)          mesa_test_eq_3v(a, b)
#define COPY_3V(DST, SRC)         mesa_copy3v(DST, SRC)
/** Copy a 3-element vector with cast */
#define COPY_3V_CAST( DST, SRC, CAST )  \
do {                                    \
   (DST)[0] = (CAST)(SRC)[0];           \
   (DST)[1] = (CAST)(SRC)[1];           \
   (DST)[2] = (CAST)(SRC)[2];           \
} while (0)
#define COPY_3FV(DST, SRC)        mesa_copy3fv(DST, SRC)
#define SUB_3V(DST, SRCA, SRCB)   mesa_sub3v(DST, SRCA, SRCB)
#define ADD_3V(DST, SRCA, SRCB)   mesa_add3v(DST, SRCA, SRCB)
#define SCALE_3V(DST, SRCA, SRCB) mesa_scale3v(DST, SRCA, SRCB)
#define SELF_SCALE_3V(DST, SRC)   mesa_self_scale3v(DST, SRC)
#define ACC_3V(DST, SRC)          mesa_acc3v(DST, SRC)
#define ACC_SCALE_3V(DST, SRCA, SRCB)       mesa_acc_scale3v(DST, SRCA, SRCB)
#define SCALE_SCALAR_3V(DST, S, SRCB)       mesa_scale_scalar3v(DST, S, SRCB)
#define ACC_SCALE_SCALAR_3V(DST, S, SRCB)   mesa_acc_scale_scalar3v(DST, S, SRCB)
#define SELF_SCALE_SCALAR_3V(DST, S)        mesa_self_scale_scalar3v(DST, S)
#define ACC_SCALAR_3V(DST, S)               mesa_acc_scalar3v(DST, S)
#define ASSIGN_3V(V, V0, V1, V2)            mesa_assign3v(V, V0, V1, V2)

/*@}*/


/**********************************************************************/
/** \name 2-element vector operations */
/*@{*/

/** Zero both elements. */
template<typename T>
inline void mesa_zero2v(T* v) noexcept { v[0] = v[1] = T(0); }

/** Copy a 2-element vector. */
template<typename Dst, typename Src>
inline void mesa_copy2v(Dst* dst, const Src* src) noexcept {
    dst[0]=src[0]; dst[1]=src[1];
}

/** Copy a 2-element float vector using memcpy. */
inline void mesa_copy2fv(GLfloat* dst, const GLfloat* src) noexcept {
    std::memcpy(dst, src, 2 * sizeof(GLfloat));
}

/** Subtraction: dst = a - b */
template<typename Dst, typename A, typename B>
inline void mesa_sub2v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]-b[0]; dst[1]=a[1]-b[1];
}

/** Addition: dst = a + b */
template<typename Dst, typename A, typename B>
inline void mesa_add2v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]+b[0]; dst[1]=a[1]+b[1];
}

/** Element-wise multiplication: dst = a * b */
template<typename Dst, typename A, typename B>
inline void mesa_scale2v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]=a[0]*b[0]; dst[1]=a[1]*b[1];
}

/** In-place addition: dst += src */
template<typename Dst, typename Src>
inline void mesa_acc2v(Dst* dst, const Src* src) noexcept {
    dst[0]+=src[0]; dst[1]+=src[1];
}

/** Element-wise multiply-accumulate: dst += a * b */
template<typename Dst, typename A, typename B>
inline void mesa_acc_scale2v(Dst* dst, const A* a, const B* b) noexcept {
    dst[0]+=a[0]*b[0]; dst[1]+=a[1]*b[1];
}

/** Scalar multiplication: dst = s * src */
template<typename Dst, typename S, typename Src>
inline void mesa_scale_scalar2v(Dst* dst, S s, const Src* src) noexcept {
    dst[0]=s*src[0]; dst[1]=s*src[1];
}

/** Scalar multiply-accumulate: dst += s * src */
template<typename Dst, typename S, typename Src>
inline void mesa_acc_scale_scalar2v(Dst* dst, S s, const Src* src) noexcept {
    dst[0]+=s*src[0]; dst[1]+=s*src[1];
}

/** In-place scalar multiplication: dst *= s */
template<typename Dst, typename S>
inline void mesa_self_scale_scalar2v(Dst* dst, S s) noexcept {
    dst[0]*=s; dst[1]*=s;
}

/** In-place scalar addition: dst += s */
template<typename Dst, typename S>
inline void mesa_acc_scalar2v(Dst* dst, S s) noexcept {
    dst[0]+=s; dst[1]+=s;
}

/** Assign two scalar values to a vector. */
template<typename V, typename V0, typename V1>
inline void mesa_assign2v(V* v, V0 v0, V1 v1) noexcept {
    v[0]=v0; v[1]=v1;
}

/* --- Macro aliases --- */
#define ZERO_2V(DST)              mesa_zero2v(DST)
#define COPY_2V(DST, SRC)         mesa_copy2v(DST, SRC)
/** Copy a 2-element vector with cast */
#define COPY_2V_CAST( DST, SRC, CAST )      \
do {                        \
   (DST)[0] = (CAST)(SRC)[0];           \
   (DST)[1] = (CAST)(SRC)[1];           \
} while (0)
#define COPY_2FV(DST, SRC)        mesa_copy2fv(DST, SRC)
#define SUB_2V(DST, SRCA, SRCB)   mesa_sub2v(DST, SRCA, SRCB)
#define ADD_2V(DST, SRCA, SRCB)   mesa_add2v(DST, SRCA, SRCB)
#define SCALE_2V(DST, SRCA, SRCB) mesa_scale2v(DST, SRCA, SRCB)
#define ACC_2V(DST, SRC)          mesa_acc2v(DST, SRC)
#define ACC_SCALE_2V(DST, SRCA, SRCB)       mesa_acc_scale2v(DST, SRCA, SRCB)
#define SCALE_SCALAR_2V(DST, S, SRCB)       mesa_scale_scalar2v(DST, S, SRCB)
#define ACC_SCALE_SCALAR_2V(DST, S, SRCB)   mesa_acc_scale_scalar2v(DST, S, SRCB)
#define SELF_SCALE_SCALAR_2V(DST, S)        mesa_self_scale_scalar2v(DST, S)
#define ACC_SCALAR_2V(DST, S)               mesa_acc_scalar2v(DST, S)
#define ASSIGN_2V(V, V0, V1)                mesa_assign2v(V, V0, V1)

/*@}*/


/** \name Linear interpolation */
/*@{*/

/**
 * Linear interpolation: result = out + t*(in - out)
 * Replaces the old macro form which evaluated OUT twice.
 */
template<typename T, typename U>
[[nodiscard]] constexpr auto mesa_linterp(T t, U out, U in) noexcept {
    return out + t * (in - out);
}

/* LINTERP kept as macro alias so dependent macros (INTERP_UB, INTERP_CHAN)
 * pick up the inline function automatically. */
#define LINTERP(T, OUT, IN)  mesa_linterp(T, OUT, IN)

/* Can do better with integer math */
#define INTERP_UB( t, dstub, outub, inub )  \
do {                        \
   GLfloat inf = UBYTE_TO_FLOAT( inub );    \
   GLfloat outf = UBYTE_TO_FLOAT( outub );  \
   GLfloat dstf = LINTERP( t, outf, inf );  \
   UNCLAMPED_FLOAT_TO_UBYTE( dstub, dstf ); \
} while (0)

#define INTERP_CHAN( t, dstc, outc, inc )   \
do {                        \
   GLfloat inf = CHAN_TO_FLOAT( inc );      \
   GLfloat outf = CHAN_TO_FLOAT( outc );    \
   GLfloat dstf = LINTERP( t, outf, inf );  \
   UNCLAMPED_FLOAT_TO_CHAN( dstc, dstf );   \
} while (0)

/** Float linear interpolation (assigns to dstui). */
template<typename T>
inline void mesa_interp_ui(T t, GLuint& dstui, GLuint outui, GLuint inui) noexcept {
    dstui = static_cast<GLuint>(static_cast<GLint>(
        mesa_linterp(t, static_cast<GLfloat>(outui), static_cast<GLfloat>(inui))));
}

/** Float linear interpolation (assigns to dstf). */
template<typename T>
inline void mesa_interp_f(T t, GLfloat& dstf, GLfloat outf, GLfloat inf) noexcept {
    dstf = mesa_linterp(t, outf, inf);
}

/** 4-component float linear interpolation. */
template<typename T>
inline void mesa_interp_4f(T t, GLfloat* dst, const GLfloat* out, const GLfloat* in) noexcept {
    dst[0] = mesa_linterp(t, out[0], in[0]);
    dst[1] = mesa_linterp(t, out[1], in[1]);
    dst[2] = mesa_linterp(t, out[2], in[2]);
    dst[3] = mesa_linterp(t, out[3], in[3]);
}

/** 3-component float linear interpolation. */
template<typename T>
inline void mesa_interp_3f(T t, GLfloat* dst, const GLfloat* out, const GLfloat* in) noexcept {
    dst[0] = mesa_linterp(t, out[0], in[0]);
    dst[1] = mesa_linterp(t, out[1], in[1]);
    dst[2] = mesa_linterp(t, out[2], in[2]);
}

/** Interpolate sz (1-4) float components into a 4-element vector. */
template<typename T>
inline void mesa_interp_sz(T t, GLfloat (*vec)[4], int to, int out, int in, int sz) noexcept {
    switch (sz) {
    case 4: vec[to][3] = mesa_linterp(t, vec[out][3], vec[in][3]); [[fallthrough]];
    case 3: vec[to][2] = mesa_linterp(t, vec[out][2], vec[in][2]); [[fallthrough]];
    case 2: vec[to][1] = mesa_linterp(t, vec[out][1], vec[in][1]); [[fallthrough]];
    case 1: vec[to][0] = mesa_linterp(t, vec[out][0], vec[in][0]);
    }
}

/* --- Macro aliases --- */
#define INTERP_UI(t, dstui, outui, inui)     mesa_interp_ui(t, dstui, outui, inui)
#define INTERP_F(t, dstf, outf, inf)         mesa_interp_f(t, dstf, outf, inf)
#define INTERP_4F(t, dst, out, in)           mesa_interp_4f(t, dst, out, in)
#define INTERP_3F(t, dst, out, in)           mesa_interp_3f(t, dst, out, in)
#define INTERP_SZ(t, vec, to, out, in, sz)   mesa_interp_sz(t, vec, to, out, in, sz)

#define INTERP_4CHAN( t, dst, out, in )         \
do {                            \
   INTERP_CHAN( (t), (dst)[0], (out)[0], (in)[0] ); \
   INTERP_CHAN( (t), (dst)[1], (out)[1], (in)[1] ); \
   INTERP_CHAN( (t), (dst)[2], (out)[2], (in)[2] ); \
   INTERP_CHAN( (t), (dst)[3], (out)[3], (in)[3] ); \
} while (0)

#define INTERP_3CHAN( t, dst, out, in )         \
do {                            \
   INTERP_CHAN( (t), (dst)[0], (out)[0], (in)[0] ); \
   INTERP_CHAN( (t), (dst)[1], (out)[1], (in)[1] ); \
   INTERP_CHAN( (t), (dst)[2], (out)[2], (in)[2] ); \
} while (0)

/*@}*/

/**
 * Type-safe clamp/min/max helpers (C++17).
 *
 * These replace the old single-argument-type macros below.  Using function
 * templates avoids the double-evaluation and side-effect hazards of the old
 * macro forms while still accepting mixed-type arguments via common_type
 * promotion.
 */
template<typename T, typename U, typename V>
[[nodiscard]] constexpr auto mesa_clamp(T x, U lo, V hi) noexcept
{
    using R = std::common_type_t<T, U, V>;
    R rx = static_cast<R>(x), rlo = static_cast<R>(lo), rhi = static_cast<R>(hi);
    return rx < rlo ? rlo : (rx > rhi ? rhi : rx);
}
template<typename T, typename U>
[[nodiscard]] constexpr auto mesa_min2(T a, U b) noexcept
{
    using R = std::common_type_t<T, U>;
    return static_cast<R>(a) < static_cast<R>(b) ? static_cast<R>(a) : static_cast<R>(b);
}
template<typename T, typename U>
[[nodiscard]] constexpr auto mesa_max2(T a, U b) noexcept
{
    using R = std::common_type_t<T, U>;
    return static_cast<R>(a) > static_cast<R>(b) ? static_cast<R>(a) : static_cast<R>(b);
}

/** Clamp X to [MIN,MAX] */
#define CLAMP(X, MIN, MAX)  mesa_clamp(X, MIN, MAX)

/** Assign X to CLAMP(X, MIN, MAX) */
#define CLAMP_SELF(x, mn, mx)  \
   ( (x)<(mn) ? ((x) = (mn)) : ((x)>(mx) ? ((x)=(mx)) : (x)) )



/** Minimum of two values: */
#define MIN2(A, B)   mesa_min2(A, B)

/** Maximum of two values: */
#define MAX2(A, B)   mesa_max2(A, B)

/** \name Geometry / vector-math inline functions */
/*@{*/

/** Dot product of two 2-element vectors. */
template<typename T, typename U>
[[nodiscard]] inline auto mesa_dot2(const T* a, const U* b) noexcept {
    return a[0]*b[0] + a[1]*b[1];
}

/** Dot product of two 3-element vectors. */
template<typename T, typename U>
[[nodiscard]] inline auto mesa_dot3(const T* a, const U* b) noexcept {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/** Dot product of two 4-element vectors. */
template<typename T, typename U>
[[nodiscard]] inline auto mesa_dot4(const T* a, const U* b) noexcept {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}

/** Cross product: n = u × v */
template<typename N, typename U, typename V>
inline void mesa_cross3(N* n, const U* u, const V* v) noexcept {
    n[0] = u[1]*v[2] - u[2]*v[1];
    n[1] = u[2]*v[0] - u[0]*v[2];
    n[2] = u[0]*v[1] - u[1]*v[0];
}

/** Squared length of a 3-element float vector. */
[[nodiscard]] inline GLfloat mesa_len_sq3fv(const GLfloat* v) noexcept {
    return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

/** Squared length of a 2-element float vector. */
[[nodiscard]] inline GLfloat mesa_len_sq2fv(const GLfloat* v) noexcept {
    return v[0]*v[0] + v[1]*v[1];
}

/** Length of a 3-element float vector. */
[[nodiscard]] inline GLfloat mesa_len3fv(const GLfloat* v) noexcept {
    return SQRTF(mesa_len_sq3fv(v));
}

/** Length of a 2-element float vector. */
[[nodiscard]] inline GLfloat mesa_len2fv(const GLfloat* v) noexcept {
    return SQRTF(mesa_len_sq2fv(v));
}

/** Normalise a 3-element float vector to unit length (no-op if zero). */
inline void mesa_normalize3fv(GLfloat* v) noexcept {
    GLfloat len = mesa_len_sq3fv(v);
    if (len) {
        len = INV_SQRTF(len);
        v[0] *= len; v[1] *= len; v[2] *= len;
    }
}

/* --- Macro aliases --- */
#define DOT2(a, b)              mesa_dot2(a, b)
#define DOT3(a, b)              mesa_dot3(a, b)
#define DOT4(a, b)              mesa_dot4(a, b)
/** Dot product of a 4-element vector against four scalars */
#define DOT4V(v,a,b,c,d) (v[0]*(a) + v[1]*(b) + v[2]*(c) + v[3]*(d))
#define CROSS3(n, u, v)         mesa_cross3(n, u, v)
#define LEN_SQUARED_3FV(V)      mesa_len_sq3fv(V)
#define LEN_SQUARED_2FV(V)      mesa_len_sq2fv(V)
#define LEN_3FV(V)              mesa_len3fv(V)
#define LEN_2FV(V)              mesa_len2fv(V)
#define NORMALIZE_3FV(V)        mesa_normalize3fv(V)

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
