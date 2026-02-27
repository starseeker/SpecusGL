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


/**
 * \file glthread.h
 * Thread support for gl dispatch – C++17 implementation.
 *
 * Replaces the old platform-specific \#ifdef maze (PTHREADS, SOLARIS_THREADS,
 * WIN32_THREADS, USE_XTHREADS) with the C++17 standard library:
 *
 *  - \c _glthread_TSD   is now \c void* ; callers declare the storage
 *                       \c thread_local so each thread owns its own copy.
 *  - \c _glthread_Mutex is an alias for \c std::mutex.
 *  - \c _glthread_Thread is \c unsigned long (a numeric thread ID).
 *
 * Mutex convenience macros are kept as thin inline wrappers for backward
 * compatibility, but new code should prefer RAII (\c std::lock_guard) directly.
 *
 * Functions:
 *   _glthread_GetID()      Returns a numeric identifier for the calling thread.
 *   _glthread_InitTSD()    Initialises a TSD slot (no-op – thread_local zeroes itself).
 *   _glthread_GetTSD()     Returns the per-thread pointer stored in the slot.
 *   _glthread_SetTSD()     Writes a per-thread pointer into the slot.
 */

#ifndef GLTHREAD_H
#define GLTHREAD_H

#include <mutex>
#include <thread>

#if defined(USE_MGL_NAMESPACE)
#define _glapi_Dispatch _mglapi_Dispatch
#endif

/* Always treat this build as thread-capable (C++17 guarantees thread support). */
#ifndef THREADS
#  define THREADS
#endif


/* -----------------------------------------------------------------------
 * Thread-specific data (TSD).
 *
 * _glthread_TSD is a plain void*.  Callers must declare the variable as
 * thread_local so that each thread gets its own copy of the pointer.
 * The get/set helpers below simply dereference the supplied address, which
 * is always the address of the calling thread's thread_local storage.
 * ----------------------------------------------------------------------- */

/** Type of a TSD slot.  Declare as \c thread_local at each use site. */
using _glthread_TSD = void *;

/** Initialise a TSD slot (no-op: thread_local variables default to nullptr). */
inline void _glthread_InitTSD(_glthread_TSD *tsd) noexcept { *tsd = nullptr; }

/** Return the per-thread value stored in \p tsd. */
inline void *_glthread_GetTSD(_glthread_TSD *tsd) noexcept { return *tsd; }

/** Store \p ptr as the per-thread value in \p tsd. */
inline void _glthread_SetTSD(_glthread_TSD *tsd, void *ptr) noexcept { *tsd = ptr; }


/* -----------------------------------------------------------------------
 * Mutex.
 * ----------------------------------------------------------------------- */

/** Mutex type – backed by std::mutex. */
using _glthread_Mutex = std::mutex;

/** Declare a static mutex.  std::mutex is default-constructible. */
#define _glthread_DECLARE_STATIC_MUTEX(name)  static std::mutex name

/** Initialise a mutex (no-op: std::mutex default-constructs itself). */
#define _glthread_INIT_MUTEX(name)    ((void)0)

/** Destroy a mutex (no-op: std::mutex destructor handles cleanup). */
#define _glthread_DESTROY_MUTEX(name) ((void)0)

/** Lock a mutex. */
#define _glthread_LOCK_MUTEX(name)    (name).lock()

/** Unlock a mutex. */
#define _glthread_UNLOCK_MUTEX(name)  (name).unlock()


/* -----------------------------------------------------------------------
 * Thread identifier.
 * ----------------------------------------------------------------------- */

/** Numeric thread identifier type. */
using _glthread_Thread = unsigned long;

/** Return a numeric identifier for the calling thread. */
extern unsigned long _glthread_GetID(void);


/* -----------------------------------------------------------------------
 * Dispatch helper.
 * ----------------------------------------------------------------------- */

#if !defined(GL_CALL)
#  define GET_DISPATCH() \
    ((__builtin_expect(_glapi_Dispatch != nullptr, 1)) \
        ? _glapi_Dispatch : _glapi_get_dispatch())
#endif /* ndef GL_CALL */


#endif /* GLTHREAD_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
