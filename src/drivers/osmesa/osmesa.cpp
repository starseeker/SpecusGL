/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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


/*
 * Off-Screen Mesa rendering / Rendering into client memory space
 *
 * Note on thread safety:  this driver is thread safe.  All
 * functions are reentrant.  The notion of current context is
 * managed by the core _mesa_make_current() and _mesa_get_current_context()
 * functions.  Those functions are thread-safe.
 */


#include "glheader.h"
#include "OSMesa/osmesa.h"
#include "context.h"
#include "extensions.h"
#include "framebuffer.h"
#include "imports.h"
#include "mtypes.h"
#include "renderbuffer.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"
#include "vbo/vbo.h"
#include "fxaa/fxaa_cpu.h"



/**
 * OSMesa rendering context – a proper C++ class layered on top of GLcontext.
 *
 * The public OSMesa C API (OSMesaCreateContext etc.) is a thin wrapper around
 * this class.  All logic lives in the class methods so that the behaviour
 * below the public API is expressed in idiomatic C++.
 *
 * \note \c mesa must remain the first data member so that the OSMESA_CONTEXT()
 *       helper cast continues to work.
 */
struct osmesa_context {
    GLcontext mesa;		        /**< Base Mesa context – must be first */
    GLvisual *gl_visual = nullptr;  /**< Describes the buffers */
    struct gl_renderbuffer *rb = nullptr; /**< The user's colorbuffer */
    GLframebuffer *gl_buffer = nullptr;   /**< The framebuffer */
    GLenum format = OSMESA_RGBA;    /**< User-specified context format */
    GLint userRowLength = 0;        /**< user-specified pixels per row (0 = width) */
    GLint rInd = 0, gInd = 0, bInd = 0, aInd = 0; /**< RGBA component offsets */
    std::vector<GLvoid *> rowaddr;  /**< address of first pixel in each image row */
    GLboolean yup = GL_TRUE;        /**< GL_TRUE  → Y increases upward */
    GLboolean enable_fxaa = GL_FALSE; /**< GL_TRUE to enable FXAA */

    /* ------------------------------------------------------------------ */
    /* Destructor                                                          */
    /* ------------------------------------------------------------------ */

    /**
     * Destroy this context, releasing all Mesa resources.
     * Called automatically when the object is deleted.
     */
    ~osmesa_context();

    /* ------------------------------------------------------------------ */
    /* Static factory                                                      */
    /* ------------------------------------------------------------------ */

    /**
     * Create and fully initialise a new OSMesa context.
     * Returns a heap-allocated context on success, nullptr on failure.
     * Replaces the body of OSMesaCreateContextExt().
     */
    static osmesa_context *create(GLenum format,
                                  GLint depthBits, GLint stencilBits,
                                  GLint accumBits,
                                  osmesa_context *sharelist);

    /* ------------------------------------------------------------------ */
    /* Instance methods                                                    */
    /* ------------------------------------------------------------------ */

    /**
     * Bind a pixel buffer to this context and make it current.
     * Replaces the body of OSMesaMakeCurrent().
     */
    GLboolean make_current(void *buffer, GLenum type,
                           GLsizei width, GLsizei height);

    /**
     * Set a pixel-store parameter (row length or Y-up direction).
     * Replaces the body of OSMesaPixelStore().
     */
    void pixel_store(GLint pname, GLint value);

    /**
     * Query an integer OSMesa parameter.
     * Replaces the body of OSMesaGetIntegerv().
     */
    void get_integer(GLint pname, GLint *value) const;

    /** Recompute the rowaddr array from the current buffer and format. */
    void compute_row_addresses();

    /** Apply FXAA post-processing if enabled. */
    void apply_fxaa();

    /* ------------------------------------------------------------------ */
    /* Static driver callbacks (registered in the dd_function_table)      */
    /* ------------------------------------------------------------------ */

    /** GL_RENDERER / GL_VERSION string callback. */
    static const GLubyte *get_string_cb(GLcontext *ctx, GLenum name);

    /** State-invalidation callback: propagates dirty bits to sub-systems. */
    static void update_state_cb(GLcontext *ctx, GLuint new_state);

    /** glFinish callback: flushes the swrast pipeline and applies FXAA. */
    static void finish_cb(GLcontext *ctx);

    /** Choose a fast line-drawing function if possible. */
    static swrast_line_func choose_line_func(GLcontext *ctx);

    /** Choose a fast triangle-drawing function if possible. */
    static swrast_tri_func choose_triangle_func(GLcontext *ctx);

    /** swrast choose_line hook: calls choose_line_func and installs result. */
    static void choose_line(GLcontext *ctx);

    /** swrast choose_triangle hook: calls choose_triangle_func and installs result. */
    static void choose_triangle(GLcontext *ctx);

    /* ------------------------------------------------------------------ */
    /* Query methods                                                       */
    /* ------------------------------------------------------------------ */

    /**
     * Get the depth buffer for this context.
     * Returns GL_TRUE on success.
     */
    [[nodiscard]] GLboolean get_depth_buffer(GLint *width, GLint *height,
                                              GLint *bytesPerValue,
                                              void **buf) const;

    /**
     * Get the color buffer for this context.
     * Returns GL_TRUE on success.
     */
    [[nodiscard]] GLboolean get_color_buffer(GLint *width, GLint *height,
                                              GLint *fmt, void **buf) const;

    /** Enable or disable fragment colour clamping. */
    void color_clamp(GLboolean enable);

    /** Enable or disable FXAA post-processing. */
    void set_fxaa_enable(GLboolean enable) { enable_fxaa = enable; }
};


static inline OSMesaContext
OSMESA_CONTEXT(GLcontext *ctx)
{
    /* Just cast, since mesa is the first member of osmesa_context */
    return static_cast<OSMesaContext>(static_cast<void *>(ctx));
}


/**********************************************************************/
/*** Private Device Driver Functions (now static methods)           ***/
/**********************************************************************/


const GLubyte *
osmesa_context::get_string_cb(GLcontext *ctx, GLenum name)
{
    (void) ctx;
    switch (name) {
	case GL_RENDERER:
#if CHAN_BITS == 32
	    return (const GLubyte *) "Mesa OffScreen32";
#elif CHAN_BITS == 16
	    return (const GLubyte *) "Mesa OffScreen16";
#else
	    return (const GLubyte *) "Mesa OffScreen";
#endif
	default:
	    return nullptr;
    }
}


void
osmesa_context::update_state_cb(GLcontext *ctx, GLuint new_state)
{
    /* Propagate dirty state to all sub-systems. */
    _swrast_InvalidateState(ctx, new_state);
    _swsetup_InvalidateState(ctx, new_state);
    _tnl_InvalidateState(ctx, new_state);
    _vbo_InvalidateState(ctx, new_state);
}


/**
 * glFinish callback: flush the swrast pipeline and apply FXAA.
 */
void
osmesa_context::finish_cb(GLcontext *ctx)
{
    OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
    _swrast_flush(ctx);
    osmesa->apply_fxaa();
}


/**
 * Apply FXAA post-processing if enabled.
 * Called from finish_cb() after rendering is complete.
 */
void
osmesa_context::apply_fxaa()
{
    if (!enable_fxaa)
        return;
    
    /* Only apply FXAA to RGBA8 buffers */
    if (rb->DataType != GL_UNSIGNED_BYTE)
        return;
    
    /* Only apply to RGBA format for now */
    if (format != OSMESA_RGBA)
        return;
    
    GLint width = rb->Width;
    GLint height = rb->Height;
    uint8_t* buffer = (uint8_t*)rb->Data;
    
    if (!buffer || width <= 0 || height <= 0)
        return;
    
    /* Calculate stride based on row length */
    GLint rowlength = userRowLength ? userRowLength : width;
    GLint strideBytes = rowlength * 4;  /* 4 bytes per RGBA pixel */
    
    /* Set up FXAA parameters (matching VTK defaults) */
    FXAAParams params = {
        .RelativeContrastThreshold = 0.125f,
        .HardContrastThreshold = 0.0625f,
        .SubpixelBlendLimit = 0.75f,
        .SubpixelContrastThreshold = 0.25f,
        .EndpointSearchIterations = 12
    };
    
    /* Apply FXAA with sRGB color space conversion (matching VTK) */
    ImageRGBA8 img = { buffer, width, height, strideBytes };
    fxaa_apply_rgba8_srgb(&img, &img, &params);
}



/**********************************************************************/
/*****        Read/write spans/arrays of pixels                   *****/
/**********************************************************************/

/* 8-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = 255
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 16-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = 65535
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 32-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = MAX2((VALUE[RCOMP]), 0.0F); \
   DST[1] = MAX2((VALUE[GCOMP]), 0.0F); \
   DST[2] = MAX2((VALUE[BCOMP]), 0.0F); \
   DST[3] = CLAMP((VALUE[ACOMP]), 0.0F, 1.0F)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = MAX2((VALUE[RCOMP]), 0.0F); \
   DST[1] = MAX2((VALUE[GCOMP]), 0.0F); \
   DST[2] = MAX2((VALUE[BCOMP]), 0.0F); \
   DST[3] = 1.0F
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"


/* 8-bit BGRA */
#define NAME(PREFIX) PREFIX##_BGRA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = 255
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 16-bit BGRA */
#define NAME(PREFIX) PREFIX##_BGRA16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = 65535
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_BGRA32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = 1.0F
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"


/* 8-bit ARGB */
#define NAME(PREFIX) PREFIX##_ARGB8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = 255
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"

/* 16-bit ARGB */
#define NAME(PREFIX) PREFIX##_ARGB16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = 65535
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"

/* 32-bit ARGB */
#define NAME(PREFIX) PREFIX##_ARGB32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = 1.0F
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"


/* 8-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = 255
#include "swrast/s_spantemp.h"

/* 16-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = 65535U
#include "swrast/s_spantemp.h"

/* 32-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = 1.0F
#include "swrast/s_spantemp.h"


/* 8-bit BGR */
#define NAME(PREFIX) PREFIX##_BGR8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = 255
#include "swrast/s_spantemp.h"

/* 16-bit BGR */
#define NAME(PREFIX) PREFIX##_BGR16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = 65535
#include "swrast/s_spantemp.h"

/* 32-bit BGR */
#define NAME(PREFIX) PREFIX##_BGR32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = 1.0F
#include "swrast/s_spantemp.h"


/* 16-bit 5/6/5 RGB */
#define NAME(PREFIX) PREFIX##_RGB_565
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = ( (((VALUE[RCOMP]) & 0xf8) << 8) | (((VALUE[GCOMP]) & 0xfc) << 3) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((*SRC) >> 8) & 0xf8) | (((*SRC) >> 11) & 0x7) ); \
   DST[GCOMP] = ( (((*SRC) >> 3) & 0xfc) | (((*SRC) >>  5) & 0x3) ); \
   DST[BCOMP] = ( (((*SRC) << 3) & 0xf8) | (((*SRC)      ) & 0x7) ); \
   DST[ACOMP] = CHAN_MAX
#include "swrast/s_spantemp.h"


/* color index */
#define NAME(PREFIX) PREFIX##_CI
#define CI_MODE
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]
#include "swrast/s_spantemp.h"




/**
 * Macros for optimized line/triangle rendering.
 * Only for 8-bit channel, RGBA, BGRA, ARGB formats.
 */

#define PACK_RGBA(DST, R, G, B, A)	\
do {					\
   (DST)[osmesa->rInd] = R;		\
   (DST)[osmesa->gInd] = G;		\
   (DST)[osmesa->bInd] = B;		\
   (DST)[osmesa->aInd] = A;		\
} while (0)

#define PIXELADDR4(X,Y)  ((GLchan *) osmesa->rowaddr[Y] + 4 * (X))


/**
 * Draw a flat-shaded, RGB line into an osmesa buffer.
 */
#define NAME flat_rgba_line
#define CLIP_HACK 1
#define SETUP_CODE						\
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);		\
   const GLchan *color = vert1->color;

#define PLOT(X, Y)						\
do {								\
   GLchan *p = PIXELADDR4(X, Y);				\
   PACK_RGBA(p, color[0], color[1], color[2], color[3]);	\
} while (0)

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif



/**
 * Draw a flat-shaded, Z-less, RGB line into an osmesa buffer.
 */
#define NAME flat_rgba_z_line
#define CLIP_HACK 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE					\
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);	\
   const GLchan *color = vert1->color;

#define PLOT(X, Y)					\
do {							\
   if (Z < *zPtr) {					\
      GLchan *p = PIXELADDR4(X, Y);			\
      PACK_RGBA(p, color[RCOMP], color[GCOMP],		\
                   color[BCOMP], color[ACOMP]);		\
      *zPtr = Z;					\
   }							\
} while (0)

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif



/**
 * Analyze context state to see if we can provide a fast line drawing
 * function.  Otherwise, return nullptr.
 */
swrast_line_func
osmesa_context::choose_line_func(GLcontext *ctx)
{
    const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
    const SWcontext *swrast = SWRAST_CONTEXT(ctx);

    if (osmesa->rb->DataType != GL_UNSIGNED_BYTE)
	return nullptr;

    if (ctx->RenderMode != GL_RENDER)      return nullptr;
    if (ctx->Line.SmoothFlag)              return nullptr;
    if (ctx->Texture._EnabledUnits)        return nullptr;
    if (ctx->Light.ShadeModel != GL_FLAT)  return nullptr;
    if (ctx->Line.Width != 1.0F)           return nullptr;
    if (ctx->Line.StippleFlag)             return nullptr;
    if (ctx->Line.SmoothFlag)              return nullptr;
    if (osmesa->format != OSMESA_RGBA &&
	osmesa->format != OSMESA_BGRA &&
	osmesa->format != OSMESA_ARGB)     return nullptr;

    if (swrast->_RasterMask==DEPTH_BIT
	&& ctx->Depth.Func==GL_LESS
	&& ctx->Depth.Mask==GL_TRUE
	&& ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
	return (swrast_line_func) flat_rgba_z_line;
    }

    if (swrast->_RasterMask == 0) {
	return (swrast_line_func) flat_rgba_line;
    }

    return (swrast_line_func) nullptr;
}


/**********************************************************************/
/*****                 Optimized triangle rendering               *****/
/**********************************************************************/


/*
 * Smooth-shaded, z-less triangle, RGBA color.
 */
#define NAME smooth_rgba_z_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define SETUP_CODE \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define RENDER_SPAN( span ) {					\
   GLuint i;							\
   GLchan *img = PIXELADDR4(span.x, span.y); 			\
   if (zRow) {							\
      for (i = 0; i < span.end; i++, img += 4) {		\
         const GLuint z = FixedToDepth(span.z);			\
         if (z < zRow[i]) {					\
            PACK_RGBA(img, FixedToChan(span.red),		\
               FixedToChan(span.green), FixedToChan(span.blue),	\
               FixedToChan(span.alpha));			\
            zRow[i] = z;					\
         }							\
         span.red += span.redStep;				\
         span.green += span.greenStep;				\
         span.blue += span.blueStep;				\
         span.alpha += span.alphaStep;				\
         span.z += span.zStep;					\
      }                                                         \
   }                                                            \
}
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif



/*
 * Flat-shaded, z-less triangle, RGBA color.
 */
#define NAME flat_rgba_z_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE						\
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);		\
   GLuint pixel;						\
   PACK_RGBA((GLchan *) &pixel, v2->color[0], v2->color[1],	\
                                v2->color[2], v2->color[3]);

#define RENDER_SPAN( span ) {				\
   GLuint i;						\
   GLuint *img = (GLuint *) PIXELADDR4(span.x, span.y);	\
   if (zRow) {						\
      for (i = 0; i < span.end; i++) {			\
         const GLuint z = FixedToDepth(span.z);		\
         if (z < zRow[i]) {				\
            img[i] = pixel;				\
            zRow[i] = z;				\
         }						\
         span.z += span.zStep;				\
      }                                                 \
   }							\
}
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif



/**
 * Return pointer to an optimized triangle function if possible.
 */
swrast_tri_func
osmesa_context::choose_triangle_func(GLcontext *ctx)
{
    const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
    const SWcontext *swrast = SWRAST_CONTEXT(ctx);

    if (osmesa->rb->DataType != GL_UNSIGNED_BYTE)
	return (swrast_tri_func) nullptr;

    if (ctx->RenderMode != GL_RENDER)    return (swrast_tri_func) nullptr;
    if (ctx->Polygon.SmoothFlag)         return (swrast_tri_func) nullptr;
    if (ctx->Polygon.StippleFlag)        return (swrast_tri_func) nullptr;
    if (ctx->Texture._EnabledUnits)      return (swrast_tri_func) nullptr;
    if (osmesa->format != OSMESA_RGBA &&
	osmesa->format != OSMESA_BGRA &&
	osmesa->format != OSMESA_ARGB)   return (swrast_tri_func) nullptr;
    if (ctx->Polygon.CullFlag &&
	ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
	return (swrast_tri_func) nullptr;

    if (swrast->_RasterMask == DEPTH_BIT &&
	ctx->Depth.Func == GL_LESS &&
	ctx->Depth.Mask == GL_TRUE &&
	ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
	if (ctx->Light.ShadeModel == GL_SMOOTH) {
	    return (swrast_tri_func) smooth_rgba_z_triangle;
	} else {
	    return (swrast_tri_func) flat_rgba_z_triangle;
	}
    }
    return (swrast_tri_func) nullptr;
}


/* swrast choose_triangle hook: try our optimized function first, then fall
 * back to the standard swrast implementation.
 */
void
osmesa_context::choose_triangle(GLcontext *ctx)
{
    SWcontext *swrast = SWRAST_CONTEXT(ctx);

    swrast->Triangle = osmesa_context::choose_triangle_func(ctx);
    if (!swrast->Triangle)
	_swrast_choose_triangle(ctx);
}

void
osmesa_context::choose_line(GLcontext *ctx)
{
    SWcontext *swrast = SWRAST_CONTEXT(ctx);

    swrast->Line = osmesa_context::choose_line_func(ctx);
    if (!swrast->Line)
	_swrast_choose_line(ctx);
}



/**
 * Recompute the values of the context's rowaddr array.
 */
void
osmesa_context::compute_row_addresses()
{
    GLint bytesPerPixel, bytesPerRow, i;
    GLubyte *origin = (GLubyte *) rb->Data;
    GLint bpc; /* bytes per channel */
    GLint rowlength; /* in pixels */
    GLint height = rb->Height;

    if (userRowLength)
	rowlength = userRowLength;
    else
	rowlength = rb->Width;

    if (rb->DataType == GL_UNSIGNED_BYTE)
	bpc = 1;
    else if (rb->DataType == GL_UNSIGNED_SHORT)
	bpc = 2;
    else if (rb->DataType == GL_FLOAT)
	bpc = 4;
    else {
	_mesa_problem(&mesa,
		      "Unexpected datatype in osmesa::compute_row_addresses");
	return;
    }

    if (format == OSMESA_COLOR_INDEX) {
	/* CI mode */
	bytesPerPixel = 1 * sizeof(GLubyte);
    } else if ((format == OSMESA_RGB) || (format == OSMESA_BGR)) {
	/* RGB mode */
	bytesPerPixel = 3 * bpc;
    } else if (format == OSMESA_RGB_565) {
	/* 5/6/5 RGB pixel in 16 bits */
	bytesPerPixel = 2;
    } else {
	/* RGBA mode */
	bytesPerPixel = 4 * bpc;
    }

    bytesPerRow = rowlength * bytesPerPixel;

    rowaddr.resize(height);
    if (yup) {
	/* Y=0 is bottom line of window */
	for (i = 0; i < height; i++) {
	    rowaddr[i] = (GLvoid *)((GLubyte *) origin + i * bytesPerRow);
	}
    } else {
	/* Y=0 is top line of window */
	for (i = 0; i < height; i++) {
	    GLint j = height - i - 1;
	    rowaddr[i] = (GLvoid *)((GLubyte *) origin + j * bytesPerRow);
	}
    }
}



/**
 /**
 * OsMesaRenderbuffer – renderbuffer that operates on a user-provided pixel
 * buffer (so Data must NOT be freed on destruction).
 *
 * AllocStorage sets up the format-specific dispatch table exactly as the
 * former osmesa_renderbuffer_storage() did.
 */
class OsMesaRenderbuffer : public gl_renderbuffer {
public:
    OsMesaRenderbuffer() : gl_renderbuffer(0) {}
    ~OsMesaRenderbuffer() override {
	/* Data is the user-provided buffer – do NOT free it. */
	Data = nullptr;
    }

    GLboolean AllocStorage(GLcontext *ctx, GLenum internalFormat,
			   GLuint width, GLuint height) override {
	const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
	GLint bpc; /* bits per channel */

	if (DataType == GL_UNSIGNED_BYTE)
	    bpc = 8;
	else if (DataType == GL_UNSIGNED_SHORT)
	    bpc = 16;
	else
	    bpc = 32;

	RedBits = GreenBits = BlueBits = AlphaBits = bpc;

	/* Note: ignoring internalFormat for window-system renderbuffers */
	(void) internalFormat;

	if (osmesa->format == OSMESA_RGBA) {
	    if (DataType == GL_UNSIGNED_BYTE) {
		m_GetRow = get_row_RGBA8; m_GetValues = get_values_RGBA8;
		m_PutRow = put_row_RGBA8; m_PutRowRGB = put_row_rgb_RGBA8;
		m_PutMonoRow = put_mono_row_RGBA8;
		m_PutValues = put_values_RGBA8; m_PutMonoValues = put_mono_values_RGBA8;
	    } else if (DataType == GL_UNSIGNED_SHORT) {
		m_GetRow = get_row_RGBA16; m_GetValues = get_values_RGBA16;
		m_PutRow = put_row_RGBA16; m_PutRowRGB = put_row_rgb_RGBA16;
		m_PutMonoRow = put_mono_row_RGBA16;
		m_PutValues = put_values_RGBA16; m_PutMonoValues = put_mono_values_RGBA16;
	    } else {
		m_GetRow = get_row_RGBA32; m_GetValues = get_values_RGBA32;
		m_PutRow = put_row_RGBA32; m_PutRowRGB = put_row_rgb_RGBA32;
		m_PutMonoRow = put_mono_row_RGBA32;
		m_PutValues = put_values_RGBA32; m_PutMonoValues = put_mono_values_RGBA32;
	    }
	    RedBits = GreenBits = BlueBits = AlphaBits = bpc;
	} else if (osmesa->format == OSMESA_BGRA) {
	    if (DataType == GL_UNSIGNED_BYTE) {
		m_GetRow = get_row_BGRA8; m_GetValues = get_values_BGRA8;
		m_PutRow = put_row_BGRA8; m_PutRowRGB = put_row_rgb_BGRA8;
		m_PutMonoRow = put_mono_row_BGRA8;
		m_PutValues = put_values_BGRA8; m_PutMonoValues = put_mono_values_BGRA8;
	    } else if (DataType == GL_UNSIGNED_SHORT) {
		m_GetRow = get_row_BGRA16; m_GetValues = get_values_BGRA16;
		m_PutRow = put_row_BGRA16; m_PutRowRGB = put_row_rgb_BGRA16;
		m_PutMonoRow = put_mono_row_BGRA16;
		m_PutValues = put_values_BGRA16; m_PutMonoValues = put_mono_values_BGRA16;
	    } else {
		m_GetRow = get_row_BGRA32; m_GetValues = get_values_BGRA32;
		m_PutRow = put_row_BGRA32; m_PutRowRGB = put_row_rgb_BGRA32;
		m_PutMonoRow = put_mono_row_BGRA32;
		m_PutValues = put_values_BGRA32; m_PutMonoValues = put_mono_values_BGRA32;
	    }
	    RedBits = GreenBits = BlueBits = AlphaBits = bpc;
	} else if (osmesa->format == OSMESA_ARGB) {
	    if (DataType == GL_UNSIGNED_BYTE) {
		m_GetRow = get_row_ARGB8; m_GetValues = get_values_ARGB8;
		m_PutRow = put_row_ARGB8; m_PutRowRGB = put_row_rgb_ARGB8;
		m_PutMonoRow = put_mono_row_ARGB8;
		m_PutValues = put_values_ARGB8; m_PutMonoValues = put_mono_values_ARGB8;
	    } else if (DataType == GL_UNSIGNED_SHORT) {
		m_GetRow = get_row_ARGB16; m_GetValues = get_values_ARGB16;
		m_PutRow = put_row_ARGB16; m_PutRowRGB = put_row_rgb_ARGB16;
		m_PutMonoRow = put_mono_row_ARGB16;
		m_PutValues = put_values_ARGB16; m_PutMonoValues = put_mono_values_ARGB16;
	    } else {
		m_GetRow = get_row_ARGB32; m_GetValues = get_values_ARGB32;
		m_PutRow = put_row_ARGB32; m_PutRowRGB = put_row_rgb_ARGB32;
		m_PutMonoRow = put_mono_row_ARGB32;
		m_PutValues = put_values_ARGB32; m_PutMonoValues = put_mono_values_ARGB32;
	    }
	    RedBits = GreenBits = BlueBits = AlphaBits = bpc;
	} else if (osmesa->format == OSMESA_RGB) {
	    if (DataType == GL_UNSIGNED_BYTE) {
		m_GetRow = get_row_RGB8; m_GetValues = get_values_RGB8;
		m_PutRow = put_row_RGB8; m_PutRowRGB = put_row_rgb_RGB8;
		m_PutMonoRow = put_mono_row_RGB8;
		m_PutValues = put_values_RGB8; m_PutMonoValues = put_mono_values_RGB8;
	    } else if (DataType == GL_UNSIGNED_SHORT) {
		m_GetRow = get_row_RGB16; m_GetValues = get_values_RGB16;
		m_PutRow = put_row_RGB16; m_PutRowRGB = put_row_rgb_RGB16;
		m_PutMonoRow = put_mono_row_RGB16;
		m_PutValues = put_values_RGB16; m_PutMonoValues = put_mono_values_RGB16;
	    } else {
		m_GetRow = get_row_RGB32; m_GetValues = get_values_RGB32;
		m_PutRow = put_row_RGB32; m_PutRowRGB = put_row_rgb_RGB32;
		m_PutMonoRow = put_mono_row_RGB32;
		m_PutValues = put_values_RGB32; m_PutMonoValues = put_mono_values_RGB32;
	    }
	    RedBits = GreenBits = BlueBits = bpc;
	} else if (osmesa->format == OSMESA_BGR) {
	    if (DataType == GL_UNSIGNED_BYTE) {
		m_GetRow = get_row_BGR8; m_GetValues = get_values_BGR8;
		m_PutRow = put_row_BGR8; m_PutRowRGB = put_row_rgb_BGR8;
		m_PutMonoRow = put_mono_row_BGR8;
		m_PutValues = put_values_BGR8; m_PutMonoValues = put_mono_values_BGR8;
	    } else if (DataType == GL_UNSIGNED_SHORT) {
		m_GetRow = get_row_BGR16; m_GetValues = get_values_BGR16;
		m_PutRow = put_row_BGR16; m_PutRowRGB = put_row_rgb_BGR16;
		m_PutMonoRow = put_mono_row_BGR16;
		m_PutValues = put_values_BGR16; m_PutMonoValues = put_mono_values_BGR16;
	    } else {
		m_GetRow = get_row_BGR32; m_GetValues = get_values_BGR32;
		m_PutRow = put_row_BGR32; m_PutRowRGB = put_row_rgb_BGR32;
		m_PutMonoRow = put_mono_row_BGR32;
		m_PutValues = put_values_BGR32; m_PutMonoValues = put_mono_values_BGR32;
	    }
	    RedBits = GreenBits = BlueBits = bpc;
	} else if (osmesa->format == OSMESA_RGB_565) {
	    ASSERT(DataType == GL_UNSIGNED_BYTE);
	    m_GetRow = get_row_RGB_565; m_GetValues = get_values_RGB_565;
	    m_PutRow = put_row_RGB_565; m_PutRowRGB = put_row_rgb_RGB_565;
	    m_PutMonoRow = put_mono_row_RGB_565;
	    m_PutValues = put_values_RGB_565; m_PutMonoValues = put_mono_values_RGB_565;
	    RedBits = 5; GreenBits = 6; BlueBits = 5;
	} else if (osmesa->format == OSMESA_COLOR_INDEX) {
	    m_GetRow = get_row_CI; m_GetValues = get_values_CI;
	    m_PutRow = put_row_CI; m_PutRowRGB = nullptr;
	    m_PutMonoRow = put_mono_row_CI;
	    m_PutValues = put_values_CI; m_PutMonoValues = put_mono_values_CI;
	    IndexBits = 8;
	} else {
	    _mesa_problem(ctx, "bad pixel format in osmesa renderbuffer_storage");
	}

	Width  = width;
	Height = height;

	osmesa->compute_row_addresses();

	return GL_TRUE;
    }

    void GetRow(GLcontext *ctx, GLuint count, GLint x, GLint y,
		void *values) override {
	m_GetRow(ctx, this, count, x, y, values);
    }
    void GetValues(GLcontext *ctx, GLuint count,
		   const GLint x[], const GLint y[], void *values) override {
	m_GetValues(ctx, this, count, x, y, values);
    }
    void PutRow(GLcontext *ctx, GLuint count, GLint x, GLint y,
		const void *values, const GLubyte *mask) override {
	m_PutRow(ctx, this, count, x, y, values, mask);
    }
    void PutRowRGB(GLcontext *ctx, GLuint count, GLint x, GLint y,
		   const void *values, const GLubyte *mask) override {
	if (m_PutRowRGB)
	    m_PutRowRGB(ctx, this, count, x, y, values, mask);
    }
    void PutMonoRow(GLcontext *ctx, GLuint count, GLint x, GLint y,
		    const void *value, const GLubyte *mask) override {
	m_PutMonoRow(ctx, this, count, x, y, value, mask);
    }
    void PutValues(GLcontext *ctx, GLuint count,
		   const GLint x[], const GLint y[],
		   const void *values, const GLubyte *mask) override {
	m_PutValues(ctx, this, count, x, y, values, mask);
    }
    void PutMonoValues(GLcontext *ctx, GLuint count,
		       const GLint x[], const GLint y[],
		       const void *value, const GLubyte *mask) override {
	m_PutMonoValues(ctx, this, count, x, y, value, mask);
    }

private:
    using GetRowFn  = void (*)(GLcontext *, gl_renderbuffer *, GLuint, GLint, GLint, void *);
    using GetValFn  = void (*)(GLcontext *, gl_renderbuffer *, GLuint, const GLint [], const GLint [], void *);
    using PutRowFn  = void (*)(GLcontext *, gl_renderbuffer *, GLuint, GLint, GLint, const void *, const GLubyte *);
    using PutMRowFn = void (*)(GLcontext *, gl_renderbuffer *, GLuint, GLint, GLint, const void *, const GLubyte *);
    using PutValFn  = void (*)(GLcontext *, gl_renderbuffer *, GLuint, const GLint [], const GLint [], const void *, const GLubyte *);
    using PutMValFn = void (*)(GLcontext *, gl_renderbuffer *, GLuint, const GLint [], const GLint [], const void *, const GLubyte *);

    GetRowFn  m_GetRow        = nullptr;
    GetValFn  m_GetValues     = nullptr;
    PutRowFn  m_PutRow        = nullptr;
    PutRowFn  m_PutRowRGB     = nullptr;
    PutMRowFn m_PutMonoRow    = nullptr;
    PutValFn  m_PutValues     = nullptr;
    PutMValFn m_PutMonoValues = nullptr;
};


/**
 * Allocate a new renderbuffer to describe the user-provided color buffer.
 */
static struct gl_renderbuffer *
new_osmesa_renderbuffer(GLcontext *ctx, GLenum format, GLenum type)
{
    auto *rb = new OsMesaRenderbuffer{};
    rb->RefCount = 1;

    if (format == OSMESA_COLOR_INDEX) {
	rb->InternalFormat = GL_COLOR_INDEX;
	rb->_ActualFormat  = GL_COLOR_INDEX8_EXT;
	rb->_BaseFormat    = GL_COLOR_INDEX;
	rb->DataType       = GL_UNSIGNED_BYTE;
    } else {
	rb->InternalFormat = GL_RGBA;
	rb->_ActualFormat  = GL_RGBA;
	rb->_BaseFormat    = GL_RGBA;
	rb->DataType       = type;
    }
    return rb;
}


/**********************************************************************/
/*****           osmesa_context C++ class implementations         *****/
/**********************************************************************/


/**
 * Destructor: release all Mesa resources owned by this context.
 * The object itself is freed by the caller (via \c delete osmesa).
 */
osmesa_context::~osmesa_context()
{
    if (rb)
	_mesa_reference_renderbuffer(&rb, nullptr);

    _swsetup_DestroyContext(&mesa);
    _tnl_DestroyContext(&mesa);
    _vbo_DestroyContext(&mesa);
    _swrast_DestroyContext(&mesa);

    _mesa_destroy_visual(gl_visual);
    _mesa_unreference_framebuffer(&gl_buffer);

    mesa.free_data();
}


/**
 * Static factory: create and fully initialise a new OSMesa context.
 * Returns a heap-allocated context on success, nullptr on failure.
 * Replaces the body of OSMesaCreateContextExt().
 */
osmesa_context *
osmesa_context::create(GLenum fmt,
                       GLint depthBits, GLint stencilBits,
                       GLint accumBits,
                       osmesa_context *sharelist)
{
    struct dd_function_table functions;
    GLint rind = 0, gind = 0, bind = 0, aind = 0;
    GLint indexBits = 0, redBits = 0, greenBits = 0, blueBits = 0, alphaBits = 0;
    GLboolean rgbmode;
    GLenum type = CHAN_TYPE;

    if (fmt == OSMESA_COLOR_INDEX) {
	indexBits = 8;
	rgbmode = GL_FALSE;
    } else if (fmt == OSMESA_RGBA) {
	indexBits = 0;
	redBits = CHAN_BITS; greenBits = CHAN_BITS;
	blueBits = CHAN_BITS; alphaBits = CHAN_BITS;
	rind = 0; gind = 1; bind = 2; aind = 3;
	rgbmode = GL_TRUE;
    } else if (fmt == OSMESA_BGRA) {
	indexBits = 0;
	redBits = CHAN_BITS; greenBits = CHAN_BITS;
	blueBits = CHAN_BITS; alphaBits = CHAN_BITS;
	bind = 0; gind = 1; rind = 2; aind = 3;
	rgbmode = GL_TRUE;
    } else if (fmt == OSMESA_ARGB) {
	indexBits = 0;
	redBits = CHAN_BITS; greenBits = CHAN_BITS;
	blueBits = CHAN_BITS; alphaBits = CHAN_BITS;
	aind = 0; rind = 1; gind = 2; bind = 3;
	rgbmode = GL_TRUE;
    } else if (fmt == OSMESA_RGB) {
	indexBits = 0;
	redBits = CHAN_BITS; greenBits = CHAN_BITS;
	blueBits = CHAN_BITS; alphaBits = 0;
	rind = 0; gind = 1; bind = 2;
	rgbmode = GL_TRUE;
    } else if (fmt == OSMESA_BGR) {
	indexBits = 0;
	redBits = CHAN_BITS; greenBits = CHAN_BITS;
	blueBits = CHAN_BITS; alphaBits = 0;
	rind = 2; gind = 1; bind = 0;
	rgbmode = GL_TRUE;
    }
#if CHAN_TYPE == GL_UNSIGNED_BYTE
    else if (fmt == OSMESA_RGB_565) {
	indexBits = 0;
	redBits = 5; greenBits = 6; blueBits = 5; alphaBits = 0;
	rgbmode = GL_TRUE;
    }
#endif
    else {
	return nullptr;
    }

    auto *osmesa = new osmesa_context{};

    osmesa->gl_visual = _mesa_create_visual(rgbmode,
					    GL_FALSE, GL_FALSE,
					    redBits, greenBits, blueBits, alphaBits,
					    indexBits,
					    depthBits, stencilBits,
					    accumBits, accumBits, accumBits,
					    alphaBits ? accumBits : 0,
					    1 /* num samples */);
    if (!osmesa->gl_visual) {
	delete osmesa;
	return nullptr;
    }

    /* Set up device driver function table */
    _mesa_init_driver_functions(&functions);
    functions.GetString    = osmesa_context::get_string_cb;
    functions.UpdateState  = osmesa_context::update_state_cb;
    functions.GetBufferSize = nullptr;
    functions.Finish       = osmesa_context::finish_cb;

    if (!osmesa->mesa.initialize(osmesa->gl_visual,
                                 sharelist ? &sharelist->mesa : nullptr,
                                 &functions, osmesa)) {
	_mesa_destroy_visual(osmesa->gl_visual);
	delete osmesa;
	return nullptr;
    }

    _mesa_enable_sw_extensions(&osmesa->mesa);
    _mesa_enable_1_3_extensions(&osmesa->mesa);
    _mesa_enable_1_4_extensions(&osmesa->mesa);
    _mesa_enable_1_5_extensions(&osmesa->mesa);

    osmesa->gl_buffer = _mesa_create_framebuffer(osmesa->gl_visual);
    if (!osmesa->gl_buffer) {
	_mesa_destroy_visual(osmesa->gl_visual);
	osmesa->mesa.free_data();
	delete osmesa;
	return nullptr;
    }

    /* Create front color buffer in user-provided memory (no back buffer) */
    osmesa->rb = new_osmesa_renderbuffer(&osmesa->mesa, fmt, type);
    _mesa_add_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT, osmesa->rb);
    assert(osmesa->rb->RefCount == 2);

    _mesa_add_soft_renderbuffers(osmesa->gl_buffer,
				 GL_FALSE,
				 osmesa->gl_visual->haveDepthBuffer,
				 osmesa->gl_visual->haveStencilBuffer,
				 osmesa->gl_visual->haveAccumBuffer,
				 GL_FALSE,
				 GL_FALSE);

    osmesa->format       = fmt;
    osmesa->userRowLength = 0;
    osmesa->yup          = GL_TRUE;
    osmesa->rInd         = rind;
    osmesa->gInd         = gind;
    osmesa->bInd         = bind;
    osmesa->aInd         = aind;
    osmesa->enable_fxaa  = GL_FALSE;

    /* Initialize software rasterizer and helper modules */
    {
	GLcontext *ctx = &osmesa->mesa;
	if (!_swrast_CreateContext(ctx) ||
	    !_vbo_CreateContext(ctx) ||
	    !_tnl_CreateContext(ctx) ||
	    !_swsetup_CreateContext(ctx)) {
	    delete osmesa; /* destructor cleans up everything */
	    return nullptr;
	}

	_swsetup_Wakeup(ctx);

	/* Use default TCL pipeline */
	TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

	/* Hook in our optimised line and triangle drawing functions */
	SWRAST_CONTEXT(ctx)->choose_line     = osmesa_context::choose_line;
	SWRAST_CONTEXT(ctx)->choose_triangle = osmesa_context::choose_triangle;
    }

    return osmesa;
}


/**
 * Bind a pixel buffer to this context and make it the current context.
 */
GLboolean
osmesa_context::make_current(void *buffer, GLenum type,
                              GLsizei width, GLsizei height)
{
    if (!buffer ||
	width < 1 || height < 1 ||
	width > MAX_WIDTH || height > MAX_HEIGHT) {
	return GL_FALSE;
    }

    if (format == OSMESA_RGB_565 && type != GL_UNSIGNED_SHORT_5_6_5)
	return GL_FALSE;

    update_state_cb(&mesa, 0);

    /* Call this periodically to detect when the user has begun using
     * GL rendering from multiple threads.
     */
    _glapi_check_multithread();

    /* Set renderbuffer fields.  Set width/height = 0 to force
     * AllocStorage being called by _mesa_resize_framebuffer().
     */
    rb->Data   = buffer;
    rb->DataType = type;
    rb->Width  = rb->Height = 0;

    /* Resize (triggers renderbuffer storage allocation). */
    _mesa_resize_framebuffer(&mesa, gl_buffer, width, height);
    gl_buffer->Initialized = GL_TRUE;

    _mesa_make_current(&mesa, gl_buffer, gl_buffer);

    /* Re-attach the renderbuffer to install any bpp-conversion wrapper. */
    _mesa_remove_renderbuffer(gl_buffer, BUFFER_FRONT_LEFT);
    _mesa_add_renderbuffer(gl_buffer, BUFFER_FRONT_LEFT, rb);

    _mesa_update_framebuffer_visual(gl_buffer);
    _mesa_resize_framebuffer(&mesa, gl_buffer, width, height);

    return GL_TRUE;
}


/**
 * Set a pixel-store parameter on this context.
 */
void
osmesa_context::pixel_store(GLint pname, GLint value)
{
    switch (pname) {
	case OSMESA_ROW_LENGTH:
	    if (value < 0) {
		_mesa_error(&mesa, GL_INVALID_VALUE, "OSMesaPixelStore(value)");
		return;
	    }
	    userRowLength = value;
	    break;
	case OSMESA_Y_UP:
	    yup = value ? GL_TRUE : GL_FALSE;
	    break;
	default:
	    _mesa_error(&mesa, GL_INVALID_ENUM, "OSMesaPixelStore(pname)");
	    return;
    }
    compute_row_addresses();
}


/**
 * Query an integer OSMesa parameter.
 */
void
osmesa_context::get_integer(GLint pname, GLint *value) const
{
    switch (pname) {
	case OSMESA_WIDTH:
	    *value = gl_buffer ? (GLint)gl_buffer->Width  : 0; return;
	case OSMESA_HEIGHT:
	    *value = gl_buffer ? (GLint)gl_buffer->Height : 0; return;
	case OSMESA_FORMAT:
	    *value = (GLint)format;   return;
	case OSMESA_TYPE:
	    *value = rb ? (GLint)rb->DataType : 0; return;
	case OSMESA_ROW_LENGTH:
	    *value = userRowLength;    return;
	case OSMESA_Y_UP:
	    *value = (GLint)yup;       return;
	case OSMESA_MAX_WIDTH:
	    *value = MAX_WIDTH;        return;
	case OSMESA_MAX_HEIGHT:
	    *value = MAX_HEIGHT;       return;
	default:
	    _mesa_error(const_cast<GLcontext *>(&mesa),
                       GL_INVALID_ENUM, "OSMesaGetIntergerv(pname)");
	    return;
    }
}


/**
 * Return the depth buffer associated with this context.
 */
GLboolean
osmesa_context::get_depth_buffer(GLint *width, GLint *height,
                                  GLint *bytesPerValue, void **buf) const
{
    const struct gl_renderbuffer *drb = nullptr;
    if (gl_buffer)
	drb = gl_buffer->Attachment[BUFFER_DEPTH].Renderbuffer;

    if (!drb || !drb->Data) {
	*width = *height = *bytesPerValue = 0;
	*buf = nullptr;
	return GL_FALSE;
    }

    *width  = (GLint)drb->Width;
    *height = (GLint)drb->Height;
    *bytesPerValue = (gl_visual->depthBits <= 16)
                     ? (GLint)sizeof(GLushort)
                     : (GLint)sizeof(GLuint);
    *buf = drb->Data;
    return GL_TRUE;
}


/**
 * Return the color buffer associated with this context.
 */
GLboolean
osmesa_context::get_color_buffer(GLint *width, GLint *height,
                                  GLint *fmt, void **buf) const
{
    if (rb && rb->Data) {
	*width  = (GLint)rb->Width;
	*height = (GLint)rb->Height;
	*fmt    = (GLint)format;
	*buf    = rb->Data;
	return GL_TRUE;
    }
    *width = *height = *fmt = 0;
    *buf = nullptr;
    return GL_FALSE;
}


/**
 * Enable or disable fragment colour clamping.
 */
void
osmesa_context::color_clamp(GLboolean enable)
{
    mesa.Color.ClampFragmentColor = enable ? GL_TRUE
                                           : (GLboolean)GL_FIXED_ONLY_ARB;
}


/**********************************************************************/
/*****                    Public Functions                        *****/
/**********************************************************************/


/**
 * Create an Off-Screen Mesa rendering context.  The only attribute needed is
 * an RGBA vs Color-Index mode flag.
 *
 * Input:  format - either GL_RGBA or GL_COLOR_INDEX
 *         sharelist - specifies another OSMesaContext with which to share
 *                     display lists.  nullptr indicates no sharing.
 * Return:  an OSMesaContext or 0 if error
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContext(GLenum format, OSMesaContext sharelist)
{
    const GLint accumBits = (format == OSMESA_COLOR_INDEX) ? 0 : 16;
    return OSMesaCreateContextExt(format, DEFAULT_SOFTWARE_DEPTH_BITS,
				  8, accumBits, sharelist);
}



/**
 * New in Mesa 3.5
 *
 * Create context and specify size of ancillary buffers.
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContextExt(GLenum format, GLint depthBits, GLint stencilBits,
		       GLint accumBits, OSMesaContext sharelist)
{
    return osmesa_context::create(format, depthBits, stencilBits,
                                  accumBits, sharelist);
}


/**
 * Destroy an Off-Screen Mesa rendering context.
 */
GLAPI void GLAPIENTRY
OSMesaDestroyContext(OSMesaContext osmesa)
{
    delete osmesa;
}


/**
 * Bind an OSMesaContext to an image buffer and make it current.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaMakeCurrent(OSMesaContext osmesa, void *buffer, GLenum type,
		  GLsizei width, GLsizei height)
{
    if (!osmesa)
	return GL_FALSE;
    return osmesa->make_current(buffer, type, width, height);
}



GLAPI OSMesaContext GLAPIENTRY
OSMesaGetCurrentContext(void)
{
    GLcontext *ctx = _mesa_get_current_context();
    if (ctx)
	return (OSMesaContext) ctx;
    else
	return nullptr;
}



GLAPI void GLAPIENTRY
OSMesaPixelStore(GLint pname, GLint value)
{
    OSMesaContext osmesa = OSMesaGetCurrentContext();
    if (osmesa)
	osmesa->pixel_store(pname, value);
}


GLAPI void GLAPIENTRY
OSMesaGetIntegerv(GLint pname, GLint *value)
{
    const OSMesaContext osmesa = OSMesaGetCurrentContext();
    if (osmesa)
	osmesa->get_integer(pname, value);
}


/**
 * Return the depth buffer associated with an OSMesa context.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetDepthBuffer(OSMesaContext c, GLint *width, GLint *height,
		     GLint *bytesPerValue, void **buffer)
{
    return c ? c->get_depth_buffer(width, height, bytesPerValue, buffer)
             : GL_FALSE;
}


/**
 * Return the color buffer associated with an OSMesa context.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetColorBuffer(OSMesaContext osmesa, GLint *width,
		     GLint *height, GLint *format, void **buffer)
{
    return osmesa ? osmesa->get_color_buffer(width, height, format, buffer)
                  : GL_FALSE;
}


struct name_function {
    const char *Name;
    OSMESAproc Function;
};

static struct name_function functions[] = {
    { "OSMesaCreateContext", (OSMESAproc) OSMesaCreateContext },
    { "OSMesaCreateContextExt", (OSMESAproc) OSMesaCreateContextExt },
    { "OSMesaDestroyContext", (OSMESAproc) OSMesaDestroyContext },
    { "OSMesaMakeCurrent", (OSMESAproc) OSMesaMakeCurrent },
    { "OSMesaGetCurrentContext", (OSMESAproc) OSMesaGetCurrentContext },
    { "OSMesaPixelsStore", (OSMESAproc) OSMesaPixelStore },
    { "OSMesaGetIntegerv", (OSMESAproc) OSMesaGetIntegerv },
    { "OSMesaGetDepthBuffer", (OSMESAproc) OSMesaGetDepthBuffer },
    { "OSMesaGetColorBuffer", (OSMESAproc) OSMesaGetColorBuffer },
    { "OSMesaGetProcAddress", (OSMESAproc) OSMesaGetProcAddress },
    { "OSMesaColorClamp", (OSMESAproc) OSMesaColorClamp },
    { "OSMesaFXAAEnable", (OSMESAproc) OSMesaFXAAEnable },
    { nullptr, nullptr }
};


GLAPI OSMESAproc GLAPIENTRY
OSMesaGetProcAddress(const char *funcName)
{
    int i;
    for (i = 0; functions[i].Name; i++) {
	if (strcmp(functions[i].Name, funcName) == 0)
	    return functions[i].Function;
    }
    return _glapi_get_proc_address(funcName);
}


GLAPI void GLAPIENTRY
OSMesaColorClamp(GLboolean enable)
{
    OSMesaContext osmesa = OSMesaGetCurrentContext();
    if (osmesa)
	osmesa->color_clamp(enable);
}


GLAPI void GLAPIENTRY
OSMesaFXAAEnable(GLboolean enable)
{
    OSMesaContext osmesa = OSMesaGetCurrentContext();
    if (osmesa)
	osmesa->set_fxaa_enable(enable);
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
