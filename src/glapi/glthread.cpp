/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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
 * \file glthread.cpp
 * Thread support for GL dispatch – C++17 implementation.
 *
 * Replaces the old platform-specific code (PTHREADS / SOLARIS_THREADS /
 * WIN32_THREADS / USE_XTHREADS) with a single portable implementation that
 * uses the C++17 standard library.
 *
 * _glthread_TSD is now just \c void* ; callers declare their TSD variables as
 * \c thread_local so each thread automatically gets its own zero-initialised
 * copy.  The Init/Get/Set helpers in glthread.h are inline no-ops or simple
 * pointer dereferences.
 *
 * _glthread_GetID() maps \c std::this_thread::get_id() to an \c unsigned long
 * via \c std::hash so callers can compare IDs with plain integer arithmetic.
 */

#include "glheader.h"
#include "glthread.h"

#include <functional>  /* std::hash */

/**
 * Return a numeric identifier for the calling thread.
 *
 * The value is computed by hashing \c std::this_thread::get_id() into an
 * \c unsigned long.  It is stable for the lifetime of the thread but is not
 * guaranteed to be unique across all concurrently running threads on all
 * platforms; it is sufficient for the thread-safety detection in
 * \c _glapi_check_multithread().
 */
unsigned long
_glthread_GetID(void)
{
    return static_cast<unsigned long>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
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
