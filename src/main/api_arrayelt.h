
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef API_ARRAYELT_H
#define API_ARRAYELT_H




#include "mtypes.h"


/**
 * Per-element array helper types.  These live here (rather than in the .cpp)
 * so that the AEcontext pointer in GLcontext can be typed correctly.
 */

using attrib_func = void (GLAPIENTRY *)(GLuint indx, const void *data);

struct AEarray {
    const struct gl_client_array *array;
    int offset;
};

struct AEattrib {
    const struct gl_client_array *array;
    attrib_func func;
    GLuint index;
};

/** Per-context state for the GL_ARB_vertex_array element helper. */
struct AEcontext {
    AEarray arrays[32];
    AEattrib attribs[VERT_ATTRIB_MAX + 1];
    GLuint NewState;

    struct gl_buffer_object *vbo[VERT_ATTRIB_MAX];
    GLuint nr_vbos;
    bool mapped_vbos;
};


extern bool _ae_create_context(GLcontext *ctx);
extern void _ae_destroy_context(GLcontext *ctx);
extern void _ae_invalidate_state(GLcontext *ctx, GLuint new_state);
extern void GLAPIENTRY _ae_loopback_array_elt(GLint elt);

/* May optionally be called before a batch of element calls:
 */
extern void _ae_map_vbos(GLcontext *ctx);
extern void _ae_unmap_vbos(GLcontext *ctx);




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
