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
[[nodiscard]] constexpr GLubyte mesa_float_to_ubyte(GLfloat x) noexcept {
    return static_cast<GLubyte>(static_cast<GLint>(x * 255.0F));
}
#define FLOAT_TO_UBYTE(X) mesa_float_to_ubyte(X)

/** Convert GLbyte in [-128,127] to GLfloat in [-1.0,1.0] */
[[nodiscard]] constexpr GLfloat mesa_byte_to_float(GLbyte b) noexcept {
    return (2.0F * static_cast<GLfloat>(b) + 1.0F) * (1.0F/255.0F);
}
#define BYTE_TO_FLOAT(B) mesa_byte_to_float(B)

/** Convert GLfloat in [-1.0,1.0] to GLbyte in [-128,127] */
[[nodiscard]] constexpr GLbyte mesa_float_to_byte(GLfloat x) noexcept {
    return static_cast<GLbyte>((static_cast<GLint>(255.0F * x) - 1) / 2);
}
#define FLOAT_TO_BYTE(X) mesa_float_to_byte(X)

/** Convert GLushort in [0,65535] to GLfloat in [0.0,1.0] */
[[nodiscard]] constexpr GLfloat mesa_ushort_to_float(GLushort s) noexcept {
    return static_cast<GLfloat>(s) * (1.0F / 65535.0F);
}
#define USHORT_TO_FLOAT(S) mesa_ushort_to_float(S)

/** Convert GLshort in [-32768,32767] to GLfloat in [-1.0,1.0] */
[[nodiscard]] constexpr GLfloat mesa_short_to_float(GLshort s) noexcept {
    return (2.0F * static_cast<GLfloat>(s) + 1.0F) * (1.0F/65535.0F);
}
#define SHORT_TO_FLOAT(S) mesa_short_to_float(S)

/** Convert GLfloat in [0.0,1.0] to GLshort in [-32768,32767] */
[[nodiscard]] constexpr GLshort mesa_float_to_short(GLfloat x) noexcept {
    return static_cast<GLshort>((static_cast<GLint>(65535.0F * x) - 1) / 2);
}
#define FLOAT_TO_SHORT(X) mesa_float_to_short(X)

/** Convert GLuint in [0,4294967295] to GLfloat in [0.0,1.0] */
[[nodiscard]] constexpr GLfloat mesa_uint_to_float(GLuint u) noexcept {
    return static_cast<GLfloat>(u) * (1.0F / 4294967295.0F);
}
#define UINT_TO_FLOAT(U) mesa_uint_to_float(U)

/** Convert GLfloat in [0.0,1.0] to GLuint in [0,4294967295] */
[[nodiscard]] constexpr GLuint mesa_float_to_uint(GLfloat x) noexcept {
    return static_cast<GLuint>(x * 4294967295.0);
}
#define FLOAT_TO_UINT(X) mesa_float_to_uint(X)

/** Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
[[nodiscard]] constexpr GLfloat mesa_int_to_float(GLint i) noexcept {
    return (2.0F * static_cast<GLfloat>(i) + 1.0F) * (1.0F/4294967294.0F);
}
#define INT_TO_FLOAT(I) mesa_int_to_float(I)

/** Convert GLfloat in [-1.0,1.0] to GLint in [-2147483648,2147483647] */
[[nodiscard]] constexpr GLint mesa_float_to_int(GLfloat x) noexcept {
    return static_cast<GLint>(2147483647.0 * x);
}
#define FLOAT_TO_INT(X) mesa_float_to_int(X)


[[nodiscard]] constexpr GLubyte mesa_byte_to_ubyte(GLbyte b) noexcept {
    return static_cast<GLubyte>(b < 0 ? 0 : b);
}
#define BYTE_TO_UBYTE(b)  mesa_byte_to_ubyte(b)

[[nodiscard]] constexpr GLubyte mesa_short_to_ubyte(GLshort s) noexcept {
    return static_cast<GLubyte>(s < 0 ? 0 : static_cast<GLubyte>(s >> 7));
}
#define SHORT_TO_UBYTE(s) mesa_short_to_ubyte(s)

[[nodiscard]] constexpr GLubyte mesa_ushort_to_ubyte(GLushort s) noexcept {
    return static_cast<GLubyte>(s >> 8);
}
#define USHORT_TO_UBYTE(s) mesa_ushort_to_ubyte(s)

[[nodiscard]] constexpr GLubyte mesa_int_to_ubyte(GLint i) noexcept {
    return static_cast<GLubyte>(i < 0 ? 0 : static_cast<GLubyte>(i >> 23));
}
#define INT_TO_UBYTE(i)   mesa_int_to_ubyte(i)

[[nodiscard]] constexpr GLubyte mesa_uint_to_ubyte(GLuint i) noexcept {
    return static_cast<GLubyte>(i >> 24);
}
#define UINT_TO_UBYTE(i)  mesa_uint_to_ubyte(i)


[[nodiscard]] constexpr GLushort mesa_byte_to_ushort(GLbyte b) noexcept {
    return static_cast<GLushort>(b < 0 ? 0 : ((static_cast<GLint>(b) * 65535) / 255));
}
#define BYTE_TO_USHORT(b)  mesa_byte_to_ushort(b)

[[nodiscard]] constexpr GLushort mesa_ubyte_to_ushort(GLubyte b) noexcept {
    return static_cast<GLushort>((static_cast<GLushort>(b) << 8) | static_cast<GLushort>(b));
}
#define UBYTE_TO_USHORT(b) mesa_ubyte_to_ushort(b)

[[nodiscard]] constexpr GLushort mesa_short_to_ushort(GLshort s) noexcept {
    return static_cast<GLushort>(s < 0 ? 0 : ((static_cast<GLint>(s) * 65535) / 32767));
}
#define SHORT_TO_USHORT(s) mesa_short_to_ushort(s)

[[nodiscard]] constexpr GLushort mesa_int_to_ushort(GLint i) noexcept {
    return static_cast<GLushort>(i < 0 ? 0 : static_cast<GLushort>(i >> 15));
}
#define INT_TO_USHORT(i)   mesa_int_to_ushort(i)

[[nodiscard]] constexpr GLushort mesa_uint_to_ushort(GLuint i) noexcept {
    return static_cast<GLushort>(static_cast<GLint>(i) < 0 ? 0 : static_cast<GLushort>(i >> 16));
}
#define UINT_TO_USHORT(i)  mesa_uint_to_ushort(i)

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
/** Copy a 4-element vector with explicit element cast to type CAST */
template<typename Cast, typename Dst, typename Src>
inline void mesa_copy4v_cast(Dst* dst, const Src* src) noexcept {
    dst[0] = static_cast<Cast>(src[0]);
    dst[1] = static_cast<Cast>(src[1]);
    dst[2] = static_cast<Cast>(src[2]);
    dst[3] = static_cast<Cast>(src[3]);
}
#define COPY_4V_CAST(DST, SRC, CAST) mesa_copy4v_cast<CAST>(DST, SRC)
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
/** Copy a 3-element vector with explicit element cast to type CAST */
template<typename Cast, typename Dst, typename Src>
inline void mesa_copy3v_cast(Dst* dst, const Src* src) noexcept {
    dst[0] = static_cast<Cast>(src[0]);
    dst[1] = static_cast<Cast>(src[1]);
    dst[2] = static_cast<Cast>(src[2]);
}
#define COPY_3V_CAST(DST, SRC, CAST) mesa_copy3v_cast<CAST>(DST, SRC)
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
/** Copy a 2-element vector with explicit element cast to type CAST */
template<typename Cast, typename Dst, typename Src>
inline void mesa_copy2v_cast(Dst* dst, const Src* src) noexcept {
    dst[0] = static_cast<Cast>(src[0]);
    dst[1] = static_cast<Cast>(src[1]);
}
#define COPY_2V_CAST(DST, SRC, CAST) mesa_copy2v_cast<CAST>(DST, SRC)
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

/** Ubyte linear interpolation (int-via-float). */
template<typename TScalar>
inline void mesa_interp_ub(TScalar t, GLubyte& dstub, GLubyte outub, GLubyte inub) noexcept {
    const GLfloat inf  = UBYTE_TO_FLOAT(inub);
    const GLfloat outf = UBYTE_TO_FLOAT(outub);
    const GLfloat dstf = mesa_linterp(t, outf, inf);
    unclamped_float_to_ubyte(dstub, dstf);
}
#define INTERP_UB(t, dstub, outub, inub) mesa_interp_ub(t, dstub, outub, inub)

/* INTERP_CHAN uses CHAN_TO_FLOAT and UNCLAMPED_FLOAT_TO_CHAN from colormac.h,
 * which is included after macros.h, so these remain as do-while macros. */
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

/** 4-channel linear interpolation. */
#define INTERP_4CHAN( t, dst, out, in )         \
do {                            \
   INTERP_CHAN( (t), (dst)[0], (out)[0], (in)[0] ); \
   INTERP_CHAN( (t), (dst)[1], (out)[1], (in)[1] ); \
   INTERP_CHAN( (t), (dst)[2], (out)[2], (in)[2] ); \
   INTERP_CHAN( (t), (dst)[3], (out)[3], (in)[3] ); \
} while (0)

/** 3-channel linear interpolation. */
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
