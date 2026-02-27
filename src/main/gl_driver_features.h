/**
 * \file gl_driver_features.h
 * Feature detection for the SpecusGL software renderer.
 *
 * This module is the SpecusGL answer to the SoGLDriverDatabase.cpp concept
 * found in Coin3D-derived projects.  It was produced by auditing that file
 * for what is still relevant in 2026 when targeting a *software* renderer.
 *
 * -----------------------------------------------------------------------
 * Audit findings – what was removed and why
 * -----------------------------------------------------------------------
 *
 * SoGLDriverDatabase.cpp served two separate purposes:
 *
 *  1. A FEATURE QUERY API – a named lookup that answers "is feature X
 *     supported in this context?"
 *
 *  2. A HARDWARE DRIVER DATABASE – an embedded table of known bugs,
 *     performance problems, and forced workarounds keyed on vendor /
 *     renderer / driver-version patterns.
 *
 * For SpecusGL (a pure software renderer):
 *
 *  Purpose 1 is already provided by the existing extension system in
 *  extensions.cpp / _mesa_extension_is_enabled().  The function below
 *  exposes that capability under a name that is natural to callers coming
 *  from a Coin3D background.
 *
 *  Purpose 2 is **entirely inapplicable**.  Every entry in the original
 *  embedded database assumed a specific GPU + driver combination that
 *  could exhibit a bug or a performance regression.  SpecusGL IS the
 *  renderer, so there is no external hardware driver to work around.
 *  Additionally, every concrete hardware entry referenced hardware that
 *  has been end-of-life for over a decade:
 *
 *    Removed entries (no longer relevant in 2026):
 *      - Intel GMA 950        (~2005)   – VBO/multitexture slow/broken
 *      - Intel GMA 3150       (~2009)   – VBO slow
 *      - Intel Solano (i810)  (~2000)   – multitexture broken
 *      - Intel GMA*           (various) – NPOT textures slow
 *      - Intel generic        (various) – proxy texture incompatibility
 *      - ATI Radeon 7xxx      (~2000)   – VBO broken
 *      - ATI Radeon 9xxx, driver 1.x   – VBO crash
 *      - ATI Radeon, driver 1.x / 2.0  – VBO-in-display-list crash
 *      - Various NVIDIA, driver 1.x    – miscellaneous shader / FBO bugs
 *
 *    All of the above driver-version patterns matched OpenGL version
 *    strings of "1.x" or "2.0", implying hardware that predates modern
 *    GPU driver stacks by 15-25 years.  No supported SpecusGL deployment
 *    environment contains such hardware; any system still running such
 *    drivers cannot reasonably build or run a C++17 project.
 *
 * -----------------------------------------------------------------------
 * What remains relevant
 * -----------------------------------------------------------------------
 *
 *  Runtime extension checks: querying glGetString(GL_EXTENSIONS) to learn
 *  which features the renderer advertises is the correct, portable way to
 *  determine capability.  For a software renderer the answer is fully
 *  deterministic at context-creation time, making the hardware-database
 *  layer completely redundant.
 */

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

#ifndef GL_DRIVER_FEATURES_H
#define GL_DRIVER_FEATURES_H

#include "mtypes.h"

/**
 * Return GL_TRUE if the named OpenGL extension (or internal SpecusGL
 * feature alias) is supported by the given context.
 *
 * \p feature_name should be a standard GL extension string such as
 * "GL_ARB_vertex_buffer_object", "GL_ARB_texture_non_power_of_two", etc.
 *
 * This is the runtime feature-query portion of the SoGLDriverDatabase
 * concept, retained because it remains useful for callers that need a
 * programmatic, by-name capability check.  The hardware driver bug
 * database that originally accompanied it has been removed in full
 * (see file-level comment above).
 *
 * \note Unlike the Coin3D SoGLDriverDatabase, this function never
 *       returns GL_FALSE for an advertised extension based on a
 *       vendor/renderer/version match: there are no hardware-specific
 *       overrides.  For a software renderer the extension string is
 *       authoritative.
 */
GLboolean _mesa_is_extension_supported(const GLcontext *ctx,
				       const char *feature_name);

#endif /* GL_DRIVER_FEATURES_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
