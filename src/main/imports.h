/*
 * Mesa 3-D graphics library
 * Version:  7.0.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file imports.h
 * Standard C library function wrappers.
 *
 * This file provides wrappers for all the standard C library functions
 * like malloc(), free(), printf(), getenv(), etc.
 */


#ifndef IMPORTS_H
#define IMPORTS_H


/* XXX some of the stuff in glheader.h should be moved into this file.
 */
#include "glheader.h"
#include "glcontext.h"




/**********************************************************************/
/** \name General macros */
/*@{*/

/** gcc -pedantic warns about long string literals, LONGSTRING silences that */
#if !defined(__GNUC__) || (__GNUC__ < 2) || \
    ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
# define LONGSTRING
#else
# define LONGSTRING __extension__
#endif

/*@}*/


/**********************************************************************/
/** Memory macros (legacy – prefer new/delete or std::vector in new code) */
/*@{*/

/*@}*/

/* Forward declarations needed by the RAII types below. */
extern void *_mesa_align_malloc(size_t bytes, unsigned long alignment);
extern void *_mesa_align_calloc(size_t bytes, unsigned long alignment);
extern void  _mesa_align_free(void *ptr);


/**
 * Custom deleter that calls _mesa_align_free() so it can be used with
 * std::unique_ptr to own aligned buffers allocated via ALIGN_MALLOC or
 * ALIGN_CALLOC.
 *
 * Usage example (replaces ALIGN_MALLOC + ALIGN_FREE):
 * \code
 *   aligned_array_ptr<GLubyte> buf = make_aligned_array<GLubyte>(count, 32);
 *   // buf is freed automatically when it goes out of scope
 * \endcode
 */
struct MesaAlignedDeleter {
    void operator()(void *ptr) const noexcept { _mesa_align_free(ptr); }
};

/** RAII wrapper for an array allocated with ALIGN_MALLOC / ALIGN_CALLOC. */
template <typename T>
using aligned_array_ptr = std::unique_ptr<T, MesaAlignedDeleter>;

/**
 * Allocate an aligned array of \p count elements of type T.
 * Returns an aligned_array_ptr that calls _mesa_align_free on destruction.
 * Pass \p zero_init=true to zero-fill the allocation (like ALIGN_CALLOC).
 */
template <typename T>
[[nodiscard]] inline aligned_array_ptr<T>
make_aligned_array(size_t count, unsigned long alignment, bool zero_init = false)
{
    const size_t bytes = count * sizeof(T);
    void *raw = zero_init ? _mesa_align_calloc(bytes, alignment)
                          : _mesa_align_malloc(bytes, alignment);
    return aligned_array_ptr<T>{static_cast<T *>(raw)};
}


/*
 * For GL_ARB_vertex_buffer_object we need to treat vertex array pointers
 * as offsets into buffer stores.  Since the vertex array pointer and
 * buffer store pointer are both pointers and we need to add them, we use
 * this macro.
 * Both pointers/offsets are expressed in bytes.
 */
/** Add two pointers/byte offsets, returning a GLubyte *. */
inline const GLubyte *ADD_POINTERS(const void *a, const void *b) noexcept {
    return static_cast<const GLubyte *>(a) + reinterpret_cast<uintptr_t>(b);
}
inline GLubyte *ADD_POINTERS(void *a, const void *b) noexcept {
    return static_cast<GLubyte *>(a) + reinterpret_cast<uintptr_t>(b);
}


/**
 * Reinterpret the bit pattern of a GLfloat as a GLint.
 *
 * This uses std::memcpy which is the standard-compliant way to do this in
 * C++17; compilers generate identical code (a plain register move on x86).
 */
[[nodiscard]] inline GLint float_bits(float f) noexcept {
    GLint bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

/**
 * Reinterpret the bit pattern of a GLint as a GLfloat.
 */
[[nodiscard]] inline float bits_float(GLint bits) noexcept {
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}



/**********************************************************************
 * Math macros
 */

constexpr GLushort MAX_GLUSHORT = 0xffffU;
constexpr GLuint   MAX_GLUINT   = 0xffffffffU;

#ifndef M_PI
#define M_PI (3.1415926536)
#endif

#ifndef M_E
#define M_E (2.7182818284590452354)
#endif

#ifndef ONE_DIV_LN2
#define ONE_DIV_LN2 (1.442695040888963456)
#endif

#ifndef ONE_DIV_SQRT_LN2
#define ONE_DIV_SQRT_LN2 (1.201122408786449815)
#endif

#ifndef FLT_MAX_EXP
#define FLT_MAX_EXP 128
#endif

/* Degrees to radians conversion: */
constexpr double DEG2RAD = M_PI / 180.0;


/***
 *** IEEE floating point constants
 ***/
constexpr GLint IEEE_ONE  = 0x3f800000; /**< Bit pattern of 1.0f */
constexpr GLint IEEE_0996 = 0x3f7f0000; /**< Bit pattern of ~0.996f */


/**
 * Single-precision math wrappers: inline functions instead of macros to
 * provide proper type checking and avoid double-evaluation hazards.
 */
inline float SQRTF(float x)    { return std::sqrt(x); }
inline float INV_SQRTF(float x) { return 1.0f / SQRTF(x); }
inline float CEILF(float x)    { return std::ceil(x); }
inline float FLOORF(float x)   { return std::floor(x); }
inline float FABSF(float x)    { return std::fabs(x); }
inline float LOGF(float x)     { return std::log(x); }
inline float EXPF(float x)     { return std::exp(x); }
inline float LDEXPF(float x, int e) { return std::ldexp(x, e); }
inline float FREXPF(float x, int *e) { return std::frexp(x, e); }


/***
 *** LOG2: Log base 2 of float – fast IEEE bit-manipulation approach.
 ***/
/* Pretty fast, and accurate.
 * Based on code from http://www.flipcode.com/totd/
 */
[[nodiscard]] static inline GLfloat LOG2(GLfloat val)
{
    GLint bits = float_bits(val);
    const GLint log_2 = ((bits >> 23) & 255) - 128;
    bits &= ~(255 << 23);
    bits += 127 << 23;
    const float f0 = bits_float(bits);
    const float result = ((-1.0f/3) * f0 + 2) * f0 - 2.0f/3;
    return result + static_cast<GLfloat>(log_2);
}


/***
 *** IS_INF_OR_NAN: test if float is infinite or NaN
 ***/
[[nodiscard]] static inline int IS_INF_OR_NAN(float x)
{
    const GLint bits = float_bits(x);
    return !(static_cast<int>(static_cast<unsigned int>((bits & 0x7fffffff) - 0x7f800000) >> 31));
}


/***
 *** IS_NEGATIVE: test if float is negative
 ***/
[[nodiscard]] static inline int GET_FLOAT_BITS(float x)
{
    return float_bits(x);
}
#define IS_NEGATIVE(x) (GET_FLOAT_BITS(x) < 0)


/***
 *** DIFFERENT_SIGNS: test if two floats have opposite signs
 ***/
[[nodiscard]] inline bool different_signs(float x, float y) noexcept {
    return (GET_FLOAT_BITS(x) ^ GET_FLOAT_BITS(y)) & (1U << 31);
}
#define DIFFERENT_SIGNS(x, y) different_signs(x, y)



/***
 *** IROUND: return (as an integer) float rounded to nearest integer
 ***/
[[nodiscard]] inline int iround(float f) noexcept {
    return static_cast<int>(f >= 0.0F ? (f + 0.5F) : (f - 0.5F));
}
#define IROUND(f) iround(f)


/***
 *** IROUND_POS: return (as an integer) positive float rounded to nearest int
 ***/
#ifdef DEBUG
#define IROUND_POS(f) (assert((f) >= 0.0F), IROUND(f))
#else
#define IROUND_POS(f) (IROUND(f))
#endif


/***
 *** IFLOOR: return (as an integer) floor of float – fast IEEE bit-manipulation.
 ***/
static inline int ifloor(float f)
{
    const float af = static_cast<float>((3 << 22) + 0.5 + static_cast<double>(f));
    const float bf = static_cast<float>((3 << 22) + 0.5 - static_cast<double>(f));
    const int ai = float_bits(af);
    const int bi = float_bits(bf);
    return (ai - bi) >> 1;
}
#define IFLOOR(x)  ifloor(x)


/***
 *** ICEIL: return (as an integer) ceiling of float – fast IEEE bit-manipulation.
 ***/
static inline int iceil(float f)
{
    const float af = static_cast<float>((3 << 22) + 0.5 + static_cast<double>(f));
    const float bf = static_cast<float>((3 << 22) + 0.5 - static_cast<double>(f));
    const int ai = float_bits(af);
    const int bi = float_bits(bf);
    return (ai - bi + 1) >> 1;
}
#define ICEIL(x)  iceil(x)


/***
 *** UNCLAMPED_FLOAT_TO_UBYTE: clamp float to [0,1] and map to ubyte in [0,255]
 *** CLAMPED_FLOAT_TO_UBYTE: map float known to be in [0,1] to ubyte in [0,255]
 ***/
/* This function is sensitive to precision.  Test very carefully
 * if you change it!
 */
template<typename T>
inline void unclamped_float_to_ubyte(T& ub, float f) noexcept {
    const GLint bits = float_bits(f);
    if (bits < 0)
        ub = static_cast<T>(0);
    else if (bits >= IEEE_0996)
        ub = static_cast<T>(255);
    else {
        const float adjusted = f * (255.0F/256.0F) + 32768.0F;
        ub = static_cast<T>(float_bits(adjusted));
    }
}
template<typename T>
inline void clamped_float_to_ubyte(T& ub, float f) noexcept {
    const float adjusted = f * (255.0F/256.0F) + 32768.0F;
    ub = static_cast<T>(float_bits(adjusted));
}
#define UNCLAMPED_FLOAT_TO_UBYTE(UB, F) unclamped_float_to_ubyte(UB, F)
#define CLAMPED_FLOAT_TO_UBYTE(UB, F)   clamped_float_to_ubyte(UB, F)


/***
 *** START_FAST_MATH: Set x86 FPU to faster, 32-bit precision mode (and save
 ***                  original mode to a temporary).
 *** END_FAST_MATH: Restore x86 FPU to original mode.
 ***/
#if defined(__GNUC__) && defined(__i386__)
/*
 * Set the x86 FPU control word to guarentee only 32 bits of precision
 * are stored in registers.  Allowing the FPU to store more introduces
 * differences between situations where numbers are pulled out of memory
 * vs. situations where the compiler is able to optimize register usage.
 *
 * In the worst case, we force the compiler to use a memory access to
 * truncate the float, by specifying the 'volatile' keyword.
 */
/* Hardware default: All exceptions masked, extended double precision,
 * round to nearest (IEEE compliant):
 */
#define DEFAULT_X86_FPU		0x037f
/* All exceptions masked, single precision, round to nearest:
 */
#define FAST_X86_FPU		0x003f
/* The fldcw instruction will cause any pending FP exceptions to be
 * raised prior to entering the block, and we clear any pending
 * exceptions before exiting the block.  Hence, asm code has free
 * reign over the FPU while in the fast math block.
 */
#if defined(NO_FAST_MATH)
#define START_FAST_MATH(x)						\
do {									\
   static GLuint mask = DEFAULT_X86_FPU;				\
   __asm__ ( "fnstcw %0" : "=m" (*&(x)) );				\
   __asm__ ( "fldcw %0" : : "m" (mask) );				\
} while (0)
#else
#define START_FAST_MATH(x)						\
do {									\
   static GLuint mask = FAST_X86_FPU;					\
   __asm__ ( "fnstcw %0" : "=m" (*&(x)) );				\
   __asm__ ( "fldcw %0" : : "m" (mask) );				\
} while (0)
#endif
/* Restore original FPU mode, and clear any exceptions that may have
 * occurred in the FAST_MATH block.
 */
#define END_FAST_MATH(x)						\
do {									\
   __asm__ ( "fnclex ; fldcw %0" : : "m" (*&(x)) );			\
} while (0)
#elif defined(_MSC_VER) && defined(_M_IX86)
#define DEFAULT_X86_FPU		0x037f /* See GCC comments above */
#define FAST_X86_FPU		0x003f /* See GCC comments above */
#if defined(NO_FAST_MATH)
#define START_FAST_MATH(x) do {\
	static GLuint mask = DEFAULT_X86_FPU;\
	__asm fnstcw word ptr [x]\
	__asm fldcw word ptr [mask]\
} while(0)
#else
#define START_FAST_MATH(x) do {\
	static GLuint mask = FAST_X86_FPU;\
	__asm fnstcw word ptr [x]\
	__asm fldcw word ptr [mask]\
} while(0)
#endif
#define END_FAST_MATH(x) do {\
	__asm fnclex\
	__asm fldcw word ptr [x]\
} while(0)

#else
#define START_FAST_MATH(x)  x = 0
#define END_FAST_MATH(x)  (void)(x)
#endif


/**
 * Return 1 if this is a little endian machine, 0 if big endian.
 */
[[nodiscard]] static inline GLboolean
_mesa_little_endian(void)
{
    const GLuint ui = 1;
    GLubyte b;
    std::memcpy(&b, &ui, sizeof(b));
    return b;
}



/**********************************************************************
 * Functions
 */

extern void *
_mesa_align_malloc(size_t bytes, unsigned long alignment);

extern void *
_mesa_align_calloc(size_t bytes, unsigned long alignment);

extern void
_mesa_align_free(void *ptr);

extern void *
_mesa_align_realloc(void *oldBuffer, size_t oldSize, size_t newSize,
		    unsigned long alignment);

extern GLhalfARB
_mesa_float_to_half(float f);

extern float
_mesa_half_to_float(GLhalfARB h);


extern void
_mesa_warning(__GLcontext *gc, const char *fmtString, ...);

extern void
_mesa_problem(const __GLcontext *ctx, const char *fmtString, ...);

extern void
_mesa_error(__GLcontext *ctx, GLenum error, const char *fmtString, ...);

extern void
_mesa_debug(const __GLcontext *ctx, const char *fmtString, ...);

extern void
_mesa_exit(int status);





#endif /* IMPORTS_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
