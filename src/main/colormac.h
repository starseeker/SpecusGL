/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * \file colormac.h
 * Color-related macros
 */


#ifndef COLORMAC_H
#define COLORMAC_H


#include "imports.h"
#include "gllimits.h"
#include "macros.h"


/** \def BYTE_TO_CHAN
 * Convert from GLbyte to GLchan */

/** \def UBYTE_TO_CHAN
 * Convert from GLubyte to GLchan */

/** \def SHORT_TO_CHAN
 * Convert from GLshort to GLchan */

/** \def USHORT_TO_CHAN
 * Convert from GLushort to GLchan */

/** \def INT_TO_CHAN
 * Convert from GLint to GLchan */

/** \def UINT_TO_CHAN
 * Convert from GLuint to GLchan */

/** \def CHAN_TO_UBYTE
 * Convert from GLchan to GLubyte */

/** \def CHAN_TO_FLOAT
 * Convert from GLchan to GLfloat */

/** \def CLAMPED_FLOAT_TO_CHAN
 * Convert from GLclampf to GLchan */

/** \def UNCLAMPED_FLOAT_TO_CHAN
 * Convert from GLfloat to GLchan */

/** \def COPY_CHAN4
 * Copy a GLchan[4] array */

/** \def CHAN_PRODUCT
 * Scaled product (usually approximated) between two GLchan arguments */

#if CHAN_BITS == 8

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (GLchan) (b))
#define UBYTE_TO_CHAN(b)  (b)
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) ((s) >> 7))
#define USHORT_TO_CHAN(s) ((GLchan) ((s) >> 8))
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 23))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 24))

#define CHAN_TO_UBYTE(c)  (c)
#define CHAN_TO_FLOAT(c)  UBYTE_TO_FLOAT(c)

#define CLAMPED_FLOAT_TO_CHAN(c, f)    CLAMPED_FLOAT_TO_UBYTE(c, f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)  UNCLAMPED_FLOAT_TO_UBYTE(c, f)

#define COPY_CHAN4(DST, SRC)  COPY_4UBV(DST, SRC)

#define CHAN_PRODUCT(a, b)  ((GLubyte) (((GLint)(a) * ((GLint)(b) + 1)) >> 8))

#elif CHAN_BITS == 16

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (((GLchan) (b)) * 516))
#define UBYTE_TO_CHAN(b)  ((((GLchan) (b)) << 8) | ((GLchan) (b)))
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) (s))
#define USHORT_TO_CHAN(s) (s)
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 15))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 16))

#define CHAN_TO_UBYTE(c)  ((c) >> 8)
#define CHAN_TO_FLOAT(c)  ((GLfloat) ((c) * (1.0 / CHAN_MAXF)))

#define CLAMPED_FLOAT_TO_CHAN(c, f)    CLAMPED_FLOAT_TO_USHORT(c, f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)  UNCLAMPED_FLOAT_TO_USHORT(c, f)

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b) ((GLchan) ((((GLuint) (a)) * ((GLuint) (b))) / 65535))

#elif CHAN_BITS == 32

/* XXX floating-point color channels not fully thought-out */
#define BYTE_TO_CHAN(b)   ((GLfloat) ((b) * (1.0F / 127.0F)))
#define UBYTE_TO_CHAN(b)  ((GLfloat) ((b) * (1.0F / 255.0F)))
#define SHORT_TO_CHAN(s)  ((GLfloat) ((s) * (1.0F / 32767.0F)))
#define USHORT_TO_CHAN(s) ((GLfloat) ((s) * (1.0F / 65535.0F)))
#define INT_TO_CHAN(i)    ((GLfloat) ((i) * (1.0F / 2147483647.0F)))
#define UINT_TO_CHAN(i)   ((GLfloat) ((i) * (1.0F / 4294967295.0F)))

#define CHAN_TO_UBYTE(c)  FLOAT_TO_UBYTE(c)
#define CHAN_TO_FLOAT(c)  (c)

#define CLAMPED_FLOAT_TO_CHAN(c, f)  c = (f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)      c = (f)

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b)    ((a) * (b))

#else

#error unexpected CHAN_BITS size

#endif


/**
 * Convert 3 channels at once.
 *
 * \param dst pointer to destination GLchan[3] array.
 * \param f pointer to source GLfloat[3] array.
 *
 * \sa #UNCLAMPED_FLOAT_TO_CHAN.
 */
#define UNCLAMPED_FLOAT_TO_RGB_CHAN(dst, f)	\
do {						\
   UNCLAMPED_FLOAT_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[2], f[2]);	\
} while (0)


/**
 * Convert 4 channels at once.
 *
 * \param dst pointer to destination GLchan[4] array.
 * \param f pointer to source GLfloat[4] array.
 *
 * \sa #UNCLAMPED_FLOAT_TO_CHAN.
 */
#define UNCLAMPED_FLOAT_TO_RGBA_CHAN(dst, f)	\
do {						\
   UNCLAMPED_FLOAT_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[2], f[2]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[3], f[3]);	\
} while (0)



/**
 * \name Generic color packing inline functions.  All inputs should be GLubytes.
 *
 * These replace the old macros with type-safe, constexpr functions that
 * use static_cast to suppress narrowing-conversion warnings and eliminate
 * the double-evaluation hazard of macro arguments.
 */
/*@{*/

[[nodiscard]] constexpr GLuint mesa_pack_color_8888(GLubyte r, GLubyte g, GLubyte b, GLubyte a) noexcept {
    return (static_cast<GLuint>(r) << 24) | (static_cast<GLuint>(g) << 16) |
           (static_cast<GLuint>(b) << 8) | static_cast<GLuint>(a);
}
#define PACK_COLOR_8888(R, G, B, A) mesa_pack_color_8888(R, G, B, A)

[[nodiscard]] constexpr GLuint mesa_pack_color_8888_rev(GLubyte r, GLubyte g, GLubyte b, GLubyte a) noexcept {
    return (static_cast<GLuint>(a) << 24) | (static_cast<GLuint>(b) << 16) |
           (static_cast<GLuint>(g) << 8) | static_cast<GLuint>(r);
}
#define PACK_COLOR_8888_REV(R, G, B, A) mesa_pack_color_8888_rev(R, G, B, A)

[[nodiscard]] constexpr GLuint mesa_pack_color_888(GLubyte r, GLubyte g, GLubyte b) noexcept {
    return (static_cast<GLuint>(r) << 16) | (static_cast<GLuint>(g) << 8) | static_cast<GLuint>(b);
}
#define PACK_COLOR_888(R, G, B) mesa_pack_color_888(R, G, B)

[[nodiscard]] constexpr GLushort mesa_pack_color_565(GLubyte r, GLubyte g, GLubyte b) noexcept {
    return static_cast<GLushort>(
        ((static_cast<unsigned>(r) & 0xf8u) << 8) |
        ((static_cast<unsigned>(g) & 0xfcu) << 3) |
        ((static_cast<unsigned>(b) & 0xf8u) >> 3));
}
#define PACK_COLOR_565(R, G, B) mesa_pack_color_565(R, G, B)

[[nodiscard]] constexpr GLushort mesa_pack_color_565_rev(GLubyte r, GLubyte g, GLubyte b) noexcept {
    return static_cast<GLushort>(
        (static_cast<unsigned>(r) & 0xf8u) |
        ((static_cast<unsigned>(g) & 0xe0u) >> 5) |
        ((static_cast<unsigned>(g) & 0x1cu) << 11) |
        ((static_cast<unsigned>(b) & 0xf8u) << 5));
}
#define PACK_COLOR_565_REV(R, G, B) mesa_pack_color_565_rev(R, G, B)

[[nodiscard]] constexpr GLushort mesa_pack_color_1555(GLubyte a, GLubyte b, GLubyte g, GLubyte r) noexcept {
    return static_cast<GLushort>(
        ((static_cast<unsigned>(b) & 0xf8u) << 7) |
        ((static_cast<unsigned>(g) & 0xf8u) << 2) |
        ((static_cast<unsigned>(r) & 0xf8u) >> 3) |
        (a ? 0x8000u : 0u));
}
#define PACK_COLOR_1555(A, B, G, R) mesa_pack_color_1555(A, B, G, R)

[[nodiscard]] constexpr GLushort mesa_pack_color_1555_rev(GLubyte a, GLubyte b, GLubyte g, GLubyte r) noexcept {
    return static_cast<GLushort>(
        ((static_cast<unsigned>(b) & 0xf8u) >> 1) |
        ((static_cast<unsigned>(g) & 0xc0u) >> 6) |
        ((static_cast<unsigned>(g) & 0x38u) << 10) |
        ((static_cast<unsigned>(r) & 0xf8u) << 5) |
        (a ? 0x80u : 0u));
}
#define PACK_COLOR_1555_REV(A, B, G, R) mesa_pack_color_1555_rev(A, B, G, R)

[[nodiscard]] constexpr GLushort mesa_pack_color_4444(GLubyte r, GLubyte g, GLubyte b, GLubyte a) noexcept {
    return static_cast<GLushort>(
        ((static_cast<unsigned>(r) & 0xf0u) << 8) |
        ((static_cast<unsigned>(g) & 0xf0u) << 4) |
        (static_cast<unsigned>(b) & 0xf0u) |
        (static_cast<unsigned>(a) >> 4));
}
#define PACK_COLOR_4444(R, G, B, A) mesa_pack_color_4444(R, G, B, A)

[[nodiscard]] constexpr GLushort mesa_pack_color_4444_rev(GLubyte r, GLubyte g, GLubyte b, GLubyte a) noexcept {
    return static_cast<GLushort>(
        ((static_cast<unsigned>(b) & 0xf0u) << 8) |
        ((static_cast<unsigned>(a) & 0xf0u) << 4) |
        (static_cast<unsigned>(r) & 0xf0u) |
        (static_cast<unsigned>(g) >> 4));
}
#define PACK_COLOR_4444_REV(R, G, B, A) mesa_pack_color_4444_rev(R, G, B, A)

[[nodiscard]] constexpr GLushort mesa_pack_color_88(GLubyte l, GLubyte a) noexcept {
    return static_cast<GLushort>((static_cast<unsigned>(l) << 8) | static_cast<unsigned>(a));
}
#define PACK_COLOR_88(L, A) mesa_pack_color_88(L, A)

[[nodiscard]] constexpr GLushort mesa_pack_color_88_rev(GLubyte l, GLubyte a) noexcept {
    return static_cast<GLushort>((static_cast<unsigned>(a) << 8) | static_cast<unsigned>(l));
}
#define PACK_COLOR_88_REV(L, A) mesa_pack_color_88_rev(L, A)

[[nodiscard]] constexpr GLubyte mesa_pack_color_332(GLubyte r, GLubyte g, GLubyte b) noexcept {
    return static_cast<GLubyte>(
        (static_cast<unsigned>(r) & 0xe0u) |
        ((static_cast<unsigned>(g) & 0xe0u) >> 3) |
        ((static_cast<unsigned>(b) & 0xc0u) >> 6));
}
#define PACK_COLOR_332(R, G, B) mesa_pack_color_332(R, G, B)

[[nodiscard]] constexpr GLubyte mesa_pack_color_233(GLubyte b, GLubyte g, GLubyte r) noexcept {
    return static_cast<GLubyte>(
        (static_cast<unsigned>(b) & 0xc0u) |
        ((static_cast<unsigned>(g) & 0xe0u) >> 2) |
        ((static_cast<unsigned>(r) & 0xe0u) >> 5));
}
#define PACK_COLOR_233(B, G, R) mesa_pack_color_233(B, G, R)

/*@}*/


/**
 * \name GLchan linear interpolation inline functions.
 *
 * INTERP_CHAN and its multi-channel variants are defined here (rather than in
 * macros.h) because they require CHAN_TO_FLOAT and UNCLAMPED_FLOAT_TO_CHAN,
 * which are defined in this header after macros.h is included.
 *
 * Template parameters are used for the channel type so that GLchan need not
 * be visible at the point of template definition; it is deduced at each call
 * site where the concrete GLchan type is available.
 */
/*@{*/

/** Single-channel linear interpolation over a generic channel type. */
template<typename T, typename TChan>
inline void mesa_interp_chan(T t, TChan& dstc, const TChan& outc, const TChan& inc) noexcept {
    GLfloat inf  = CHAN_TO_FLOAT(inc);
    GLfloat outf = CHAN_TO_FLOAT(outc);
    GLfloat dstf = LINTERP(t, outf, inf);
    UNCLAMPED_FLOAT_TO_CHAN(dstc, dstf);
}
#define INTERP_CHAN(t, dstc, outc, inc) mesa_interp_chan(t, dstc, outc, inc)

/** 4-channel linear interpolation over a generic channel type. */
template<typename T, typename TChan>
inline void mesa_interp_4chan(T t, TChan* dst, const TChan* out, const TChan* in) noexcept {
    mesa_interp_chan(t, dst[0], out[0], in[0]);
    mesa_interp_chan(t, dst[1], out[1], in[1]);
    mesa_interp_chan(t, dst[2], out[2], in[2]);
    mesa_interp_chan(t, dst[3], out[3], in[3]);
}
#define INTERP_4CHAN(t, dst, out, in) mesa_interp_4chan(t, dst, out, in)

/** 3-channel linear interpolation over a generic channel type. */
template<typename T, typename TChan>
inline void mesa_interp_3chan(T t, TChan* dst, const TChan* out, const TChan* in) noexcept {
    mesa_interp_chan(t, dst[0], out[0], in[0]);
    mesa_interp_chan(t, dst[1], out[1], in[1]);
    mesa_interp_chan(t, dst[2], out[2], in[2]);
}
#define INTERP_3CHAN(t, dst, out, in) mesa_interp_3chan(t, dst, out, in)

/*@}*/


#endif /* COLORMAC_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
