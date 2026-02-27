/*
 * Mesa 3-D graphics library
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

/**
 * \file gl_driver_features.cpp
 * Runtime feature detection for SpecusGL.
 *
 * See gl_driver_features.h for the full audit rationale explaining why the
 * hardware driver database from SoGLDriverDatabase.cpp was dropped.
 */

#include "gl_driver_features.h"
#include "extensions.h"

/**
 * Return GL_TRUE if \p feature_name is advertised by \p ctx.
 *
 * Delegates to _mesa_extension_is_enabled(), which checks the compile-time
 * extension table that the driver populated during context initialisation.
 * For a software renderer the table is fully deterministic, so no
 * vendor/renderer/version override logic is required.
 */
GLboolean
_mesa_is_extension_supported(const GLcontext *ctx, const char *feature_name)
{
    if (!ctx || !feature_name)
	return GL_FALSE;
    return _mesa_extension_is_enabled(ctx, feature_name);
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
