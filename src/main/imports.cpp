/**
 * \file imports.c
 * Standard C library function wrappers.
 *
 * Imports are services which the device driver or window system or
 * operating system provides to the core renderer.  The core renderer (Mesa)
 * will call these functions in order to do memory allocation, simple I/O,
 * etc.
 *
 * Some drivers will want to override/replace this file with something
 * specialized, but that'll be rare.
 *
 * Eventually, I want to move roll the glheader.h file into this.
 *
 * \todo Functions still needed:
 * - scanf
 * - qsort
 * - rand and RAND_MAX
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.0
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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



#include "imports.h"
#include "context.h"
#include "version.h"

#include <cstdlib>


constexpr int MAXSTRING = 4000; /* for vsnprintf() */

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

/**********************************************************************/
/** \name Memory */
/*@{*/

/**
 * Allocate aligned memory.
 *
 * \param bytes number of bytes to allocate.
 * \param alignment alignment (must be greater than zero).
 *
 * Allocates extra memory to accommodate rounding up the address for
 * alignment and to record the real malloc address.
 *
 * \sa _mesa_align_free().
 */
void *
_mesa_align_malloc(size_t bytes, unsigned long alignment)
{
#if defined(_WIN32) && defined(_MSC_VER)
    return _aligned_malloc(bytes, alignment);
#else
    /* std::aligned_alloc requires size to be a multiple of alignment */
    const size_t sz = (bytes + alignment - 1) & ~(size_t)(alignment - 1);
    return std::aligned_alloc(alignment, sz);
#endif
}

/**
 * Same as _mesa_align_malloc(), but using calloc(1,) instead of
 * malloc()
 */
void *
_mesa_align_calloc(size_t bytes, unsigned long alignment)
{
#if defined(_WIN32) && defined(_MSC_VER)
    void *mem;

    mem = _aligned_malloc(bytes, alignment);
    if (mem != nullptr) {
	(void) memset(mem, 0, bytes);
    }

    return mem;
#else
    const size_t sz = (bytes + alignment - 1) & ~(size_t)(alignment - 1);
    void *mem = std::aligned_alloc(alignment, sz);
    if (mem != nullptr) {
	(void) memset(mem, 0, bytes);
    }

    return mem;
#endif
}

/**
 * Free memory which was allocated with either _mesa_align_malloc()
 * or _mesa_align_calloc().
 * \param ptr pointer to the memory to be freed.
 * The actual address to free is stored in the word immediately before the
 * address the client sees.
 */
void
_mesa_align_free(void *ptr)
{
#if defined(_WIN32) && defined(_MSC_VER)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

/**
 * Reallocate memory, with alignment.
 */
void *
_mesa_align_realloc(void *oldBuffer, size_t oldSize, size_t newSize,
		    unsigned long alignment)
{
#if defined(_WIN32) && defined(_MSC_VER)
    (void) oldSize;
    return _aligned_realloc(oldBuffer, newSize, alignment);
#else
    const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
    void *newBuf = _mesa_align_malloc(newSize, alignment);
    if (newBuf && oldBuffer && copySize > 0) {
	memcpy(newBuf, oldBuffer, copySize);
    }
    if (oldBuffer)
	_mesa_align_free(oldBuffer);
    return newBuf;
#endif
}


/*@}*/


/**********************************************************************/
/** \name Math */
/*@{*/

/**
 * Convert a 4-byte float to a 2-byte half float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
GLhalfARB
_mesa_float_to_half(float val)
{
    const int flt = (const int)val;
    const int flt_m = flt & 0x7fffff;
    const int flt_e = (flt >> 23) & 0xff;
    const int flt_s = (flt >> 31) & 0x1;
    int s, e, m = 0;
    GLhalfARB result;

    /* sign bit */
    s = flt_s;

    /* handle special cases */
    if ((flt_e == 0) && (flt_m == 0)) {
	/* zero */
	/* m = 0; - already set */
	e = 0;
    } else if ((flt_e == 0) && (flt_m != 0)) {
	/* denorm -- denorm float maps to 0 half */
	/* m = 0; - already set */
	e = 0;
    } else if ((flt_e == 0xff) && (flt_m == 0)) {
	/* infinity */
	/* m = 0; - already set */
	e = 31;
    } else if ((flt_e == 0xff) && (flt_m != 0)) {
	/* NaN */
	m = 1;
	e = 31;
    } else {
	/* regular number */
	const int new_exp = flt_e - 127;
	if (new_exp < -24) {
	    /* this maps to 0 */
	    /* m = 0; - already set */
	    e = 0;
	} else if (new_exp < -14) {
	    /* this maps to a denorm */
	    unsigned int exp_val = (unsigned int)(-14 - new_exp);  /* 2^-exp_val*/
	    e = 0;
	    switch (exp_val) {
		case 1:
		    m = 512 + (flt_m >> 14);
		    break;
		case 2:
		    m = 256 + (flt_m >> 15);
		    break;
		case 3:
		    m = 128 + (flt_m >> 16);
		    break;
		case 4:
		    m = 64 + (flt_m >> 17);
		    break;
		case 5:
		    m = 32 + (flt_m >> 18);
		    break;
		case 6:
		    m = 16 + (flt_m >> 19);
		    break;
		case 7:
		    m = 8 + (flt_m >> 20);
		    break;
		case 8:
		    m = 4 + (flt_m >> 21);
		    break;
		case 9:
		    m = 2 + (flt_m >> 22);
		    break;
		case 10:
		    m = 1;
		    break;
		default:
		    _mesa_warning(nullptr,
				  "float_to_half: logical error in denorm creation!\n");
		    break;
	    }
	} else if (new_exp > 15) {
	    /* map this value to infinity */
	    /* m = 0; - already set */
	    e = 31;
	} else {
	    /* regular */
	    e = new_exp + 15;
	    m = flt_m >> 13;
	}
    }

    result = (s << 15) | (e << 10) | m;
    return result;
}


/**
 * Convert a 2-byte half float to a 4-byte float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
float
_mesa_half_to_float(GLhalfARB val)
{
    /* XXX could also use a 64K-entry lookup table */
    const int m = val & 0x3ff;
    const int e = (val >> 10) & 0x1f;
    const int s = (val >> 15) & 0x1;
    int flt_m, flt_e, flt_s, flt;
    float result;

    /* sign bit */
    flt_s = s;

    /* handle special cases */
    if ((e == 0) && (m == 0)) {
	/* zero */
	flt_m = 0;
	flt_e = 0;
    } else if ((e == 0) && (m != 0)) {
	/* denorm -- denorm half will fit in non-denorm single */
	const float half_denorm = 1.0f / 16384.0f; /* 2^-14 */
	float mantissa = ((float)(m)) / 1024.0f;
	float sign = s ? -1.0f : 1.0f;
	return sign * mantissa * half_denorm;
    } else if ((e == 31) && (m == 0)) {
	/* infinity */
	flt_e = 0xff;
	flt_m = 0;
    } else if ((e == 31) && (m != 0)) {
	/* NaN */
	flt_e = 0xff;
	flt_m = 1;
    } else {
	/* regular */
	flt_e = e + 112;
	flt_m = m << 13;
    }

    flt = (flt_s << 31) | (flt_e << 23) | flt_m;
    result = (int)flt;
    return result;
}

/*@}*/

/*@}*/


/**********************************************************************/
/** \name Diagnostics */
/*@{*/

/**
 * Report a warning (a recoverable error condition) to stderr if
 * either DEBUG is defined or the MESA_DEBUG env var is set.
 *
 * \param ctx GL context.
 * \param fmtString printf() alike format string.
 */
void
_mesa_warning(GLcontext *ctx, const char *fmtString, ...)
{
    GLboolean debug;
    char str[MAXSTRING];
    va_list args;
    (void) ctx;
    va_start(args, fmtString);
    (void) vsnprintf(str, MAXSTRING, fmtString, args);
    va_end(args);
#ifdef DEBUG
    debug = GL_TRUE; /* always print warning */
#else
    debug = std::getenv("MESA_DEBUG") ? GL_TRUE : GL_FALSE;
#endif
    if (debug) {
	fprintf(stderr, "Mesa warning: %s\n", str);
    }
}

/**
 * Report an internal implementation problem.
 * Prints the message to stderr via fprintf().
 *
 * \param ctx GL context.
 * \param s problem description string.
 */
void
_mesa_problem(const GLcontext *ctx, const char *fmtString, ...)
{
    va_list args;
    char str[MAXSTRING];
    (void) ctx;

    va_start(args, fmtString);
    vsnprintf(str, MAXSTRING, fmtString, args);
    va_end(args);

    fprintf(stderr, "Mesa %s implementation error: %s\n", MESA_VERSION_STRING, str);
    fprintf(stderr, "Please report at bugzilla.freedesktop.org\n");
}

/**
 * Record an OpenGL state error.  These usually occur when the users
 * passes invalid parameters to a GL function.
 *
 * If debugging is enabled (either at compile-time via the DEBUG macro, or
 * run-time via the MESA_DEBUG environment variable), report the error with
 * _mesa_debug().
 *
 * \param ctx the GL context.
 * \param error the error value.
 * \param fmtString printf() style format string, followed by optional args
 */
void
_mesa_error(GLcontext *ctx, GLenum error, const char *fmtString, ...)
{
    const char *debugEnv;
    GLboolean debug;

    debugEnv = std::getenv("MESA_DEBUG");

#ifdef DEBUG
    if (debugEnv && strstr(debugEnv, "silent"))
	debug = GL_FALSE;
    else
	debug = GL_TRUE;
#else
    if (debugEnv)
	debug = GL_TRUE;
    else
	debug = GL_FALSE;
#endif

    if (debug) {
	va_list args;
	char where[MAXSTRING];
	const char *errstr;

	va_start(args, fmtString);
	vsnprintf(where, MAXSTRING, fmtString, args);
	va_end(args);

	switch (error) {
	    case GL_NO_ERROR:
		errstr = "GL_NO_ERROR";
		break;
	    case GL_INVALID_VALUE:
		errstr = "GL_INVALID_VALUE";
		break;
	    case GL_INVALID_ENUM:
		errstr = "GL_INVALID_ENUM";
		break;
	    case GL_INVALID_OPERATION:
		errstr = "GL_INVALID_OPERATION";
		break;
	    case GL_STACK_OVERFLOW:
		errstr = "GL_STACK_OVERFLOW";
		break;
	    case GL_STACK_UNDERFLOW:
		errstr = "GL_STACK_UNDERFLOW";
		break;
	    case GL_OUT_OF_MEMORY:
		errstr = "GL_OUT_OF_MEMORY";
		break;
	    case GL_TABLE_TOO_LARGE:
		errstr = "GL_TABLE_TOO_LARGE";
		break;
	    case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
		errstr = "GL_INVALID_FRAMEBUFFER_OPERATION";
		break;
	    default:
		errstr = "unknown";
		break;
	}
	_mesa_debug(ctx, "User error: %s in %s\n", errstr, where);
    }

    _mesa_record_error(ctx, error);
}

/**
 * Report debug information.  Print error message to stderr via fprintf().
 * No-op if DEBUG mode not enabled.
 *
 * \param ctx GL context.
 * \param fmtString printf()-style format string, followed by optional args.
 */
void
_mesa_debug(const GLcontext *ctx, const char *fmtString, ...)
{
#ifdef DEBUG
    char s[MAXSTRING];
    va_list args;
    va_start(args, fmtString);
    vsnprintf(s, MAXSTRING, fmtString, args);
    va_end(args);
    fprintf(stderr, "Mesa: %s", s);
#endif /* DEBUG */
    (void) ctx;
    (void) fmtString;
}

/*@}*/


/**
 * Wrapper for exit().
 */
void
_mesa_exit(int status)
{
    exit(status);
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
