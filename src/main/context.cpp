/**
 * \file context.cpp
 * Mesa context/visual/framebuffer management functions – C++17 port.
 * \author Brian Paul
 *
 * C++17 changes from context.c:
 *  - GLvisual, gl_shared_state, and GLcontext are allocated with
 *    value-initialising new (operator new + zero-initialisation) instead of
 *    calloc/CALLOC_STRUCT, and freed with delete instead of free().
 *  - extern "C" guards have been added to context.h so that C translation
 *    units that include context.h continue to see C linkage.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.0.2
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
 * \mainpage Mesa Main Module
 *
 * \section MainIntroduction Introduction
 *
 * The Mesa Main module consists of all the files in the main/ directory.
 * Among the features of this module are:
 * <UL>
 * <LI> Structures to represent most GL state </LI>
 * <LI> State set/get functions </LI>
 * <LI> Display lists </LI>
 * <LI> Texture unit, object and image handling </LI>
 * <LI> Matrix and attribute stacks </LI>
 * </UL>
 *
 * Other modules are responsible for API dispatch, vertex transformation,
 * point/line/triangle setup, rasterization, vertex array caching,
 * vertex/fragment programs/shaders, etc.
 *
 *
 * \section AboutDoxygen About Doxygen
 *
 * If you're viewing this information as Doxygen-generated HTML you'll
 * see the documentation index at the top of this page.
 *
 * The first line lists the Mesa source code modules.
 * The second line lists the indexes available for viewing the documentation
 * for each module.
 *
 * Selecting the <b>Main page</b> link will display a summary of the module
 * (this page).
 *
 * Selecting <b>Data Structures</b> will list all C structures.
 *
 * Selecting the <b>File List</b> link will list all the source files in
 * the module.
 * Selecting a filename will show a list of all functions defined in that file.
 *
 * Selecting the <b>Data Fields</b> link will display a list of all
 * documented structure members.
 *
 * Selecting the <b>Globals</b> link will display a list
 * of all functions, structures, global variables and macros in the module.
 *
 */


#include "glheader.h"
#include "imports.h"
#include "accum.h"
#include "arrayobj.h"
#include "attrib.h"
#include "blend.h"
#include "buffers.h"
#include "bufferobj.h"
#include "colortab.h"
#include "context.h"
#include "debug.h"
#include "depth.h"
#include "dlist.h"
#include "eval.h"
#include "enums.h"
#include "extensions.h"
#include "fbobject.h"
#include "feedback.h"
#include "fog.h"
#include "framebuffer.h"
#include "get.h"
#include "glthread.h"
#include "glapioffsets.h"
#include "histogram.h"
#include "hint.h"
#include "hash.h"
#include "atifragshader.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#if FEATURE_NV_vertex_program || FEATURE_NV_fragment_program
#include "program.h"
#endif
#include "queryobj.h"
#include "rastpos.h"
#include "state.h"
#include "stencil.h"
#include "texcompress.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#include "version.h"
#include "vtxfmt.h"
#if _HAVE_FULL_GL
#include "math/m_translate.h"
#include "math/m_matrix.h"
#include "math/m_xform.h"
#include "math/mathmod.h"
#endif
#include "shader_api.h"

#include <mutex>


#ifndef MESA_VERBOSE
int MESA_VERBOSE = 0;
#endif

#ifndef MESA_DEBUG_FLAGS
int MESA_DEBUG_FLAGS = 0;
#endif


/* ubyte -> float conversion */
GLfloat _mesa_ubyte_to_float_color_tab[256];

static void
free_shared_state(GLcontext *ctx, struct gl_shared_state *ss);


/* -----------------------------------------------------------------------
 * __GLcontextRec method implementations
 * ----------------------------------------------------------------------- */

/**
 * notify_swap_buffers: called by the window system before swapping buffers.
 */
void
__GLcontextRec::notify_swap_buffers()
{
    FLUSH_VERTICES(this, 0);
}


/**
 * Swap buffers notification callback.
 *
 * \param gc GL context.
 *
 * Called by window system just before swapping buffers.
 * We have to finish any pending rendering.
 */
void
_mesa_notifySwapBuffers(__GLcontext *gc)
{
    gc->notify_swap_buffers();
}


/**********************************************************************/
/** \name GL Visual allocation/destruction                            */
/**********************************************************************/
/*@{*/

/**
 * Allocates a GLvisual structure and initializes it via
 * _mesa_initialize_visual().
 *
 * \param rgbFlag GL_TRUE for RGB(A) mode, GL_FALSE for Color Index mode.
 * \param dbFlag double buffering
 * \param stereoFlag stereo buffer
 * \param depthBits requested bits per depth buffer value. Any value in [0, 32]
 * is acceptable but the actual depth type will be GLushort or GLuint as
 * needed.
 * \param stencilBits requested minimum bits per stencil buffer value
 * \param accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits number of bits per color component in accum buffer.
 * \param indexBits number of bits per pixel if \p rgbFlag is GL_FALSE
 * \param redBits number of bits per color component in frame buffer for RGB(A)
 * mode.  We always use 8 in core Mesa though.
 * \param greenBits same as above.
 * \param blueBits same as above.
 * \param alphaBits same as above.
 * \param numSamples not really used.
 *
 * \return pointer to new GLvisual or nullptr if requested parameters can't be
 * met.
 *
 * \note Need to add params for level and numAuxBuffers (at least)
 */
GLvisual *
_mesa_create_visual(GLboolean rgbFlag,
		    GLboolean dbFlag,
		    GLboolean stereoFlag,
		    GLint redBits,
		    GLint greenBits,
		    GLint blueBits,
		    GLint alphaBits,
		    GLint indexBits,
		    GLint depthBits,
		    GLint stencilBits,
		    GLint accumRedBits,
		    GLint accumGreenBits,
		    GLint accumBlueBits,
		    GLint accumAlphaBits,
		    GLint numSamples)
{
    GLvisual *vis = new GLvisual{};
    if (!_mesa_initialize_visual(vis, rgbFlag, dbFlag, stereoFlag,
			         redBits, greenBits, blueBits, alphaBits,
			         indexBits, depthBits, stencilBits,
			         accumRedBits, accumGreenBits,
			         accumBlueBits, accumAlphaBits,
			         numSamples)) {
	delete vis;
	return nullptr;
    }
    return vis;
}

/**
 * Makes some sanity checks and fills in the fields of the
 * GLvisual object with the given parameters.  If the caller needs
 * to set additional fields, he should just probably init the whole GLvisual
 * object himself.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 *
 * \sa _mesa_create_visual() above for the parameter description.
 */
GLboolean
_mesa_initialize_visual(GLvisual *vis,
			GLboolean rgbFlag,
			GLboolean dbFlag,
			GLboolean stereoFlag,
			GLint redBits,
			GLint greenBits,
			GLint blueBits,
			GLint alphaBits,
			GLint indexBits,
			GLint depthBits,
			GLint stencilBits,
			GLint accumRedBits,
			GLint accumGreenBits,
			GLint accumBlueBits,
			GLint accumAlphaBits,
			GLint numSamples)
{
    assert(vis);

    if (depthBits < 0 || depthBits > 32) {
	return GL_FALSE;
    }
    if (stencilBits < 0 || stencilBits > STENCIL_BITS) {
	return GL_FALSE;
    }
    assert(accumRedBits >= 0);
    assert(accumGreenBits >= 0);
    assert(accumBlueBits >= 0);
    assert(accumAlphaBits >= 0);

    vis->rgbMode          = rgbFlag;
    vis->doubleBufferMode = dbFlag;
    vis->stereoMode       = stereoFlag;

    vis->redBits          = redBits;
    vis->greenBits        = greenBits;
    vis->blueBits         = blueBits;
    vis->alphaBits        = alphaBits;
    vis->rgbBits          = redBits + greenBits + blueBits;

    vis->indexBits      = indexBits;
    vis->depthBits      = depthBits;
    vis->stencilBits    = stencilBits;

    vis->accumRedBits   = accumRedBits;
    vis->accumGreenBits = accumGreenBits;
    vis->accumBlueBits  = accumBlueBits;
    vis->accumAlphaBits = accumAlphaBits;

    vis->haveAccumBuffer   = accumRedBits > 0;
    vis->haveDepthBuffer   = depthBits > 0;
    vis->haveStencilBuffer = stencilBits > 0;

    vis->numAuxBuffers = 0;
    vis->level = 0;
    vis->pixmapMode = 0;
    vis->sampleBuffers = numSamples > 0 ? 1 : 0;
    vis->samples = numSamples;

    return GL_TRUE;
}


/**
 * Destroy a visual and free its memory.
 *
 * \param vis visual.
 *
 * Frees the visual structure.
 */
void
_mesa_destroy_visual(GLvisual *vis)
{
    delete vis;
}

/*@}*/


/**********************************************************************/
/** \name Context allocation, initialization, destroying
 *
 * The purpose of the most initialization functions here is to provide the
 * default state values according to the OpenGL specification.
 */
/**********************************************************************/
/*@{*/

/**
 * One-time initialization mutex lock.
 *
 * \sa Used by one_time_init().
 */
static std::mutex OneTimeLock;

/**
 * Calls all the various one-time-init functions in Mesa.
 *
 * While holding a global mutex lock, calls several initialization functions,
 * and sets the glapi callbacks if the \c MESA_DEBUG environment variable is
 * defined.
 *
 * \sa _math_init().
 */
static void
one_time_init(GLcontext *ctx)
{
    static GLboolean alreadyCalled = GL_FALSE;
    (void) ctx;
    std::lock_guard<std::mutex> lock(OneTimeLock);
    if (!alreadyCalled) {
	GLuint i;

	/* do some implementation tests */
	assert(sizeof(GLbyte) == 1);
	assert(sizeof(GLubyte) == 1);
	assert(sizeof(GLshort) == 2);
	assert(sizeof(GLushort) == 2);
	assert(sizeof(GLint) == 4);
	assert(sizeof(GLuint) == 4);

	_mesa_init_sqrt_table();

#if _HAVE_FULL_GL
	_math_init();

	for (i = 0; i < 256; i++) {
	    _mesa_ubyte_to_float_color_tab[i] = (float) i / 255.0F;
	}
#endif

	if (_mesa_getenv("MESA_DEBUG")) {
	    _glapi_noop_enable_warnings(GL_TRUE);
	    _glapi_set_warning_func((_glapi_warning_func) _mesa_warning);
	} else {
	    _glapi_noop_enable_warnings(GL_FALSE);
	}

#if defined(DEBUG) && defined(__DATE__) && defined(__TIME__)
	_mesa_debug(ctx, "Mesa %s DEBUG build %s %s\n",
		    MESA_VERSION_STRING, __DATE__, __TIME__);
#endif

	alreadyCalled = GL_TRUE;
    }
}


/**
 * Allocate and initialize a shared context state structure.
 * Initializes the display list, texture objects and vertex programs hash
 * tables, allocates the texture objects. If it runs out of memory, frees
 * everything already allocated before returning nullptr.
 *
 * \return pointer to a gl_shared_state structure on success, or nullptr on
 * failure.
 */
static GLboolean
alloc_shared_state(GLcontext *ctx)
{
    struct gl_shared_state *ss = new gl_shared_state{};

    ctx->Shared = ss;

    /* Hash tables (DisplayList, TexObjects, Programs, etc.) are now value
     * members of gl_shared_state and are automatically default-constructed.
     * No explicit _mesa_NewHashTable() calls needed. */

#if FEATURE_ARB_vertex_program
    ss->DefaultVertexProgram = ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0);
    if (!ss->DefaultVertexProgram)
	goto cleanup;
#endif
#if FEATURE_ARB_fragment_program
    ss->DefaultFragmentProgram = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
    if (!ss->DefaultFragmentProgram)
	goto cleanup;
#endif
#if FEATURE_ATI_fragment_shader
    ss->DefaultFragmentShader = _mesa_new_ati_fragment_shader(ctx, 0);
    if (!ss->DefaultFragmentShader)
	goto cleanup;
#endif

    ss->Default1D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_1D);
    if (!ss->Default1D)
	goto cleanup;

    ss->Default2D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_2D);
    if (!ss->Default2D)
	goto cleanup;

    ss->Default3D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_3D);
    if (!ss->Default3D)
	goto cleanup;

    ss->DefaultCubeMap = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_CUBE_MAP_ARB);
    if (!ss->DefaultCubeMap)
	goto cleanup;

    ss->DefaultRect = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_RECTANGLE_NV);
    if (!ss->DefaultRect)
	goto cleanup;

    /* sanity check */
    assert(ss->Default1D->RefCount == 1);

    return GL_TRUE;

cleanup:
    /* Ran out of memory at some point.  Free partially-initialised objects.
     * The hash table value members will be destroyed by 'delete ss'. */
#if FEATURE_ARB_vertex_program
    if (ss->DefaultVertexProgram)
	ctx->Driver.DeleteProgram(ctx, ss->DefaultVertexProgram);
#endif
#if FEATURE_ARB_fragment_program
    if (ss->DefaultFragmentProgram)
	ctx->Driver.DeleteProgram(ctx, ss->DefaultFragmentProgram);
#endif
#if FEATURE_ATI_fragment_shader
    if (ss->DefaultFragmentShader)
	_mesa_delete_ati_fragment_shader(ctx, ss->DefaultFragmentShader);
#endif

    if (ss->Default1D)
	(*ctx->Driver.DeleteTexture)(ctx, ss->Default1D);
    if (ss->Default2D)
	(*ctx->Driver.DeleteTexture)(ctx, ss->Default2D);
    if (ss->Default3D)
	(*ctx->Driver.DeleteTexture)(ctx, ss->Default3D);
    if (ss->DefaultCubeMap)
	(*ctx->Driver.DeleteTexture)(ctx, ss->DefaultCubeMap);
    if (ss->DefaultRect)
	(*ctx->Driver.DeleteTexture)(ctx, ss->DefaultRect);
    delete ss;
    return GL_FALSE;
}


/**
 * Deallocate a shared state object and all children structures.
 *
 * \param ctx GL context.
 * \param ss shared state pointer.
 *
 * Frees the display lists, the texture objects (calling the driver texture
 * deletion callback to free its private data) and the vertex programs, as well
 * as their hash tables.
 *
 * \sa alloc_shared_state().
 */

/**
 * gl_shared_state::cleanup – release all driver-owned objects stored in this
 * shared state.  Implements the bulk of what used to be the free_shared_state()
 * free function.
 */
void
gl_shared_state::cleanup(GLcontext *ctx)
{
    /*
     * Free display lists
     */
    DisplayList.deleteAll([ctx](GLuint, void *data) {
	_mesa_delete_list(ctx, static_cast<mesa_display_list *>(data));
    });

#if defined(FEATURE_NV_vertex_program) || defined(FEATURE_NV_fragment_program)
    Programs.deleteAll([ctx](GLuint, void *data) {
	ctx->Driver.DeleteProgram(ctx, static_cast<gl_program *>(data));
    });
#endif
#if FEATURE_ARB_vertex_program
    ctx->Driver.DeleteProgram(ctx, DefaultVertexProgram);
#endif
#if FEATURE_ARB_fragment_program
    ctx->Driver.DeleteProgram(ctx, DefaultFragmentProgram);
#endif

#if FEATURE_ATI_fragment_shader
    ATIShaders.deleteAll([ctx](GLuint, void *data) {
	_mesa_delete_ati_fragment_shader(ctx, static_cast<ati_fragment_shader *>(data));
    });
    _mesa_delete_ati_fragment_shader(ctx, DefaultFragmentShader);
#endif

#if FEATURE_ARB_vertex_buffer_object || FEATURE_ARB_pixel_buffer_object
    BufferObjects.deleteAll([ctx](GLuint, void *data) {
	ctx->Driver.DeleteBuffer(ctx, static_cast<gl_buffer_object *>(data));
    });
#endif

    ArrayObjects.deleteAll([ctx](GLuint, void *data) {
	_mesa_delete_array_object(ctx, static_cast<gl_array_object *>(data));
    });

#if FEATURE_ARB_shader_objects
    ShaderObjects.walk([ctx](GLuint, void *data) {
	auto *shProg = static_cast<gl_shader_program *>(data);
	if (shProg->Type == GL_SHADER_PROGRAM_MESA)
	    _mesa_free_shader_program_data(ctx, shProg);
    });
    ShaderObjects.deleteAll([ctx](GLuint, void *data) {
	auto *sh = static_cast<gl_shader *>(data);
	if (sh->Type == GL_FRAGMENT_SHADER || sh->Type == GL_VERTEX_SHADER) {
	    _mesa_free_shader(ctx, sh);
	} else {
	    auto *shProg = static_cast<gl_shader_program *>(data);
	    ASSERT(shProg->Type == GL_SHADER_PROGRAM_MESA);
	    _mesa_free_shader_program(ctx, shProg);
	}
    });
#endif

#if FEATURE_EXT_framebuffer_object
    FrameBuffers.deleteAll([](GLuint, void *data) {
	auto *fb = static_cast<gl_framebuffer *>(data);
	fb->RefCount = 0;
	delete fb;
    });
    RenderBuffers.deleteAll([](GLuint, void *data) {
	auto *rb = static_cast<gl_renderbuffer *>(data);
	rb->RefCount = 0;
	delete rb;
    });
#endif

    /*
     * Free texture objects (after FBOs since some textures might have
     * been bound to FBOs).
     */
    ASSERT(ctx->Driver.DeleteTexture);
    ctx->Driver.DeleteTexture(ctx, Default1D);
    ctx->Driver.DeleteTexture(ctx, Default2D);
    ctx->Driver.DeleteTexture(ctx, Default3D);
    ctx->Driver.DeleteTexture(ctx, DefaultCubeMap);
    ctx->Driver.DeleteTexture(ctx, DefaultRect);
    TexObjects.deleteAll([ctx](GLuint, void *data) {
	ctx->Driver.DeleteTexture(ctx, static_cast<gl_texture_object *>(data));
    });
}

static void
free_shared_state(GLcontext *ctx, struct gl_shared_state *ss)
{
    ss->cleanup(ctx);
    delete ss;
}


/**
 * Initialize current vertex attribute defaults.
 * Replaces the file-static _mesa_init_current().
 */
void
__GLcontextRec::init_current()
{
    GLuint i;

    /* Init all to (0,0,0,1) */
    for (i = 0; i < VERT_ATTRIB_MAX; i++) {
	ASSIGN_4V(Current.Attrib[i], 0.0, 0.0, 0.0, 1.0);
    }

    /* redo special cases: */
    ASSIGN_4V(Current.Attrib[VERT_ATTRIB_WEIGHT], 1.0, 0.0, 0.0, 0.0);
    ASSIGN_4V(Current.Attrib[VERT_ATTRIB_NORMAL], 0.0, 0.0, 1.0, 1.0);
    ASSIGN_4V(Current.Attrib[VERT_ATTRIB_COLOR0], 1.0, 1.0, 1.0, 1.0);
    ASSIGN_4V(Current.Attrib[VERT_ATTRIB_COLOR1], 0.0, 0.0, 0.0, 1.0);
    ASSIGN_4V(Current.Attrib[VERT_ATTRIB_COLOR_INDEX], 1.0, 0.0, 0.0, 1.0);
    ASSIGN_4V(Current.Attrib[VERT_ATTRIB_EDGEFLAG], 1.0, 0.0, 0.0, 1.0);
}


/**
 * Init vertex/fragment program native limits from logical limits.
 */
static void
init_natives(struct gl_program_constants *prog)
{
    prog->MaxNativeInstructions = prog->MaxInstructions;
    prog->MaxNativeAluInstructions = prog->MaxAluInstructions;
    prog->MaxNativeTexInstructions = prog->MaxTexInstructions;
    prog->MaxNativeTexIndirections = prog->MaxTexIndirections;
    prog->MaxNativeAttribs = prog->MaxAttribs;
    prog->MaxNativeTemps = prog->MaxTemps;
    prog->MaxNativeAddressRegs = prog->MaxAddressRegs;
    prog->MaxNativeParameters = prog->MaxParameters;
}


/**
 * Initialize fields of gl_constants (aka Const.*).
 * Use defaults from gllimits.h.  The device drivers will often override
 * some of these values (such as number of texture units).
 * Replaces the file-static _mesa_init_constants().
 */
void
__GLcontextRec::init_constants()
{
    assert(MAX_TEXTURE_LEVELS >= MAX_3D_TEXTURE_LEVELS);
    assert(MAX_TEXTURE_LEVELS >= MAX_CUBE_TEXTURE_LEVELS);

    assert(MAX_TEXTURE_UNITS >= MAX_TEXTURE_COORD_UNITS);
    assert(MAX_TEXTURE_UNITS >= MAX_TEXTURE_IMAGE_UNITS);

    /* Constants, may be overriden (usually only reduced) by device drivers */
    Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
    Const.Max3DTextureLevels = MAX_3D_TEXTURE_LEVELS;
    Const.MaxCubeTextureLevels = MAX_CUBE_TEXTURE_LEVELS;
    Const.MaxTextureRectSize = MAX_TEXTURE_RECT_SIZE;
    Const.MaxTextureCoordUnits = MAX_TEXTURE_COORD_UNITS;
    Const.MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
    Const.MaxTextureUnits = MIN2(Const.MaxTextureCoordUnits,
				 Const.MaxTextureImageUnits);
    Const.MaxTextureMaxAnisotropy = MAX_TEXTURE_MAX_ANISOTROPY;
    Const.MaxTextureLodBias = MAX_TEXTURE_LOD_BIAS;
    Const.MaxArrayLockSize = MAX_ARRAY_LOCK_SIZE;
    Const.SubPixelBits = SUB_PIXEL_BITS;
    Const.MinPointSize = MIN_POINT_SIZE;
    Const.MaxPointSize = MAX_POINT_SIZE;
    Const.MinPointSizeAA = MIN_POINT_SIZE;
    Const.MaxPointSizeAA = MAX_POINT_SIZE;
    Const.PointSizeGranularity = (GLfloat) POINT_SIZE_GRANULARITY;
    Const.MinLineWidth = MIN_LINE_WIDTH;
    Const.MaxLineWidth = MAX_LINE_WIDTH;
    Const.MinLineWidthAA = MIN_LINE_WIDTH;
    Const.MaxLineWidthAA = MAX_LINE_WIDTH;
    Const.LineWidthGranularity = (GLfloat) LINE_WIDTH_GRANULARITY;
    Const.MaxColorTableSize = MAX_COLOR_TABLE_SIZE;
    Const.MaxConvolutionWidth = MAX_CONVOLUTION_WIDTH;
    Const.MaxConvolutionHeight = MAX_CONVOLUTION_HEIGHT;
    Const.MaxClipPlanes = MAX_CLIP_PLANES;
    Const.MaxLights = MAX_LIGHTS;
    Const.MaxShininess = 128.0;
    Const.MaxSpotExponent = 128.0;
    Const.MaxViewportWidth = MAX_WIDTH;
    Const.MaxViewportHeight = MAX_HEIGHT;
#if FEATURE_ARB_vertex_program
    Const.VertexProgram.MaxInstructions = MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS;
    Const.VertexProgram.MaxAluInstructions = 0;
    Const.VertexProgram.MaxTexInstructions = 0;
    Const.VertexProgram.MaxTexIndirections = 0;
    Const.VertexProgram.MaxAttribs = MAX_NV_VERTEX_PROGRAM_INPUTS;
    Const.VertexProgram.MaxTemps = MAX_PROGRAM_TEMPS;
    Const.VertexProgram.MaxParameters = MAX_NV_VERTEX_PROGRAM_PARAMS;
    Const.VertexProgram.MaxLocalParams = MAX_PROGRAM_LOCAL_PARAMS;
    Const.VertexProgram.MaxEnvParams = MAX_PROGRAM_ENV_PARAMS;
    Const.VertexProgram.MaxAddressRegs = MAX_VERTEX_PROGRAM_ADDRESS_REGS;
    Const.VertexProgram.MaxUniformComponents = 4 * MAX_UNIFORMS;
    init_natives(&Const.VertexProgram);
#endif

#if FEATURE_ARB_fragment_program
    Const.FragmentProgram.MaxInstructions = MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS;
    Const.FragmentProgram.MaxAluInstructions = MAX_FRAGMENT_PROGRAM_ALU_INSTRUCTIONS;
    Const.FragmentProgram.MaxTexInstructions = MAX_FRAGMENT_PROGRAM_TEX_INSTRUCTIONS;
    Const.FragmentProgram.MaxTexIndirections = MAX_FRAGMENT_PROGRAM_TEX_INDIRECTIONS;
    Const.FragmentProgram.MaxAttribs = MAX_NV_FRAGMENT_PROGRAM_INPUTS;
    Const.FragmentProgram.MaxTemps = MAX_PROGRAM_TEMPS;
    Const.FragmentProgram.MaxParameters = MAX_NV_FRAGMENT_PROGRAM_PARAMS;
    Const.FragmentProgram.MaxLocalParams = MAX_PROGRAM_LOCAL_PARAMS;
    Const.FragmentProgram.MaxEnvParams = MAX_PROGRAM_ENV_PARAMS;
    Const.FragmentProgram.MaxAddressRegs = MAX_FRAGMENT_PROGRAM_ADDRESS_REGS;
    Const.FragmentProgram.MaxUniformComponents = 4 * MAX_UNIFORMS;
    init_natives(&Const.FragmentProgram);
#endif
    Const.MaxProgramMatrices = MAX_PROGRAM_MATRICES;
    Const.MaxProgramMatrixStackDepth = MAX_PROGRAM_MATRIX_STACK_DEPTH;

    /* CheckArrayBounds is overriden by drivers/x11 for X server */
    Const.CheckArrayBounds = GL_FALSE;

    /* GL_ARB_draw_buffers */
    Const.MaxDrawBuffers = MAX_DRAW_BUFFERS;

    /* GL_OES_read_format */
    Const.ColorReadFormat = GL_RGBA;
    Const.ColorReadType = GL_UNSIGNED_BYTE;

#if FEATURE_EXT_framebuffer_object
    Const.MaxColorAttachments = MAX_COLOR_ATTACHMENTS;
    Const.MaxRenderbufferSize = MAX_WIDTH;
#endif

#if FEATURE_ARB_vertex_shader
    Const.MaxVertexTextureImageUnits = MAX_VERTEX_TEXTURE_IMAGE_UNITS;
    Const.MaxVarying = MAX_VARYING;
#endif

    /* sanity checks */
    ASSERT(Const.MaxTextureUnits == MIN2(Const.MaxTextureImageUnits,
	    Const.MaxTextureCoordUnits));
    ASSERT(Const.FragmentProgram.MaxLocalParams <= MAX_PROGRAM_LOCAL_PARAMS);
    ASSERT(Const.VertexProgram.MaxLocalParams <= MAX_PROGRAM_LOCAL_PARAMS);

    ASSERT(MAX_NV_FRAGMENT_PROGRAM_TEMPS <= MAX_PROGRAM_TEMPS);
    ASSERT(MAX_NV_VERTEX_PROGRAM_TEMPS <= MAX_PROGRAM_TEMPS);
    ASSERT(MAX_NV_VERTEX_PROGRAM_INPUTS <= VERT_ATTRIB_MAX);
    ASSERT(MAX_NV_VERTEX_PROGRAM_OUTPUTS <= VERT_RESULT_MAX);
}


/**
 * Verify driver-reported limits don't exceed Mesa's static array sizes.
 * Called on the first MakeCurrent.
 * Replaces the file-static check_context_limits().
 */
void
__GLcontextRec::check_limits() const
{
    /* Many context limits/constants are limited by the size of
     * internal arrays.
     */
    assert(Const.MaxTextureImageUnits <= MAX_TEXTURE_IMAGE_UNITS);
    assert(Const.MaxTextureCoordUnits <= MAX_TEXTURE_COORD_UNITS);
    assert(Const.MaxTextureUnits <= MAX_TEXTURE_IMAGE_UNITS);
    assert(Const.MaxTextureUnits <= MAX_TEXTURE_COORD_UNITS);

    assert(Const.MaxViewportWidth <= MAX_WIDTH);
    assert(Const.MaxViewportHeight <= MAX_WIDTH);

    /* make sure largest texture image is <= MAX_WIDTH in size */
    assert((1 << (Const.MaxTextureLevels -1)) <= MAX_WIDTH);
    assert((1 << (Const.MaxCubeTextureLevels -1)) <= MAX_WIDTH);
    assert((1 << (Const.Max3DTextureLevels -1)) <= MAX_WIDTH);

    assert(Const.MaxDrawBuffers <= MAX_DRAW_BUFFERS);

    /* XXX probably add more tests */
}


/**
 * Initialize all context attribute groups.
 *
 * Replaces the file-static init_attrib_groups().
 * Functions that are pure no-ops (because the corresponding struct now uses
 * default member initializers) have been removed from this list.  Functions
 * that still need to perform work are retained.
 */
bool
__GLcontextRec::init_attrib_groups()
{
    /* Constants */
    init_constants();

    /* Extensions */
    _mesa_init_extensions(this);

    /* Attribute Groups that still require active initialisation */
    _mesa_init_buffer_objects(this);    /* allocates NullBufferObj */
    _mesa_init_color(this);             /* sets DrawBuffer[0] from doubleBufferMode */
    init_current();                     /* sets vertex attribute defaults (w=1 etc.) */
    _mesa_init_debug(this);             /* reads MESA_NO_DITHER env var */
    _mesa_init_display_list(this);      /* allocates display-list hash table */
    _mesa_init_eval(this);              /* sets up evaluator control-point data */
    _mesa_init_lighting(this);          /* initialises light sources & shine tables */
    _mesa_init_matrix(this);            /* allocates matrix stacks */
    _mesa_init_pixel(this);             /* sets Scale arrays, ReadBuffer, BufferObjs */
    _mesa_init_point(this);             /* sets MaxSize from Const */
    _mesa_init_polygon(this);           /* sets PolygonStipple to all-on */
    _mesa_init_program(this);           /* initialises program state */
    _mesa_init_query(this);             /* allocates query hash table */
    _mesa_init_rastpos(this);           /* sets RasterTexCoords w=1 per unit */
    _mesa_init_varray(this);            /* allocates default array object */
    _mesa_init_viewport(this);          /* sets up initial viewport matrix */

    if (!_mesa_init_texture(this))
	return false;

    _mesa_init_texture_s3tc(this);
    _mesa_init_texture_fxt1(this);

    /* NewState and ErrorValue are already set by member initializers
     * but NewState must be _NEW_ALL on first use to force full state update. */
    NewState = _NEW_ALL;

    return true;
}


/**
 * This is the default function we plug into all dispatch table slots
 * This helps prevents a segfault when someone calls a GL function without
 * first checking if the extension's supported.
 */
static int
generic_nop(void)
{
    _mesa_problem(nullptr, "User called no-op dispatch function (an unsupported extension function?)");
    return 0;
}


/**
 * Allocate and initialize a new dispatch table.
 */
#define DISPATCH_TABLE_SIZE (sizeof(struct _glapi_table) / sizeof(void *))
static struct _glapi_table *
alloc_dispatch_table(void)
{
    auto *table = new _glapi_table{};
    _glapi_proc *entry = reinterpret_cast<_glapi_proc *>(table);
    for (GLint i = 0; i < DISPATCH_TABLE_SIZE; i++) {
	entry[i] = reinterpret_cast<_glapi_proc>(generic_nop);
    }
    return table;
}


/**
 * Initialize a GLcontext struct (rendering context).
 *
 * This includes allocating all the other structs and arrays which hang off of
 * the context by pointers.
 * Note that the driver needs to pass in its dd_function_table here since
 * we need to at least call driverFunctions->NewTextureObject to create the
 * default texture objects.
 *
 * Called by _mesa_create_context().
 *
 * Performs the imports and exports callback tables initialization, and
 * miscellaneous one-time initializations. If no shared context is supplied one
 * is allocated, and increase its reference count.  Setups the GL API dispatch
 * tables.  Initialize the TNL module. Sets the maximum Z buffer depth.
 * Finally queries the \c MESA_DEBUG and \c MESA_VERBOSE environment variables
 * for debug flags.
 *
 * \param ctx the context to initialize
 * \param visual describes the visual attributes for this context
 * \param share_list points to context to share textures, display lists,
 *        etc with, or nullptr
 * \param driverFunctions table of device driver functions for this context
 *        to use
 * \param driverContext pointer to driver-specific context data
 */
bool
__GLcontextRec::initialize(const GLvisual *visual,
                           GLcontext *share_list,
                           const struct dd_function_table *driverFunctions,
                           void *driverContext)
{
    ASSERT(driverContext);
    assert(driverFunctions->NewTextureObject);
    assert(driverFunctions->FreeTexImageData);

    /* misc one-time initializations */
    one_time_init(this);

    Visual = *visual;
    DrawBuffer = nullptr;
    ReadBuffer = nullptr;
    WinSysDrawBuffer = nullptr;
    WinSysReadBuffer = nullptr;

    /* Plug in driver functions and context pointer here.
     * This is important because when we call alloc_shared_state() below
     * we'll call Driver.NewTextureObject() to create the default
     * textures.
     */
    Driver = *driverFunctions;
    DriverCtx = driverContext;

    if (share_list) {
	/* share state with another context */
	Shared = share_list->Shared;
    } else {
	/* allocate new, unshared state */
	if (!alloc_shared_state(this)) {
	    return false;
	}
    }
    Shared->ref();

    if (!init_attrib_groups()) {
	free_shared_state(this, Shared);
	return false;
    }

    /* setup the API dispatch tables */
    Exec = alloc_dispatch_table();
    Save = alloc_dispatch_table();
    if (!Exec || !Save) {
	free_shared_state(this, Shared);
	if (Exec) {
	    delete Exec;
	    Exec = nullptr;
	}
	return false;
    }
    _mesa_init_exec_table(Exec);
    CurrentDispatch = Exec;
#if _HAVE_FULL_GL
    _mesa_init_dlist_table(Save);
    _mesa_install_save_vtxfmt(this, &ListState.ListVtxfmt);
    /* Neutral tnl module support: TnlModule fields default-initialised */
    _mesa_init_exec_vtxfmt(this);
#endif

    FragmentProgram._MaintainTexEnvProgram
	= (_mesa_getenv("MESA_TEX_PROG") != nullptr);
    FragmentProgram._UseTexEnvProgram = FragmentProgram._MaintainTexEnvProgram;

    VertexProgram._MaintainTnlProgram
	= (_mesa_getenv("MESA_TNL_PROG") != nullptr);
    if (VertexProgram._MaintainTnlProgram) {
	/* this is required... */
	FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
    }

    /* FirstTimeCurrent is GL_TRUE by default (member initializer) */

    return true;
}


/**
 * Initialize a GLcontext struct (rendering context).
 * Delegates to __GLcontextRec::initialize().
 */
GLboolean
_mesa_initialize_context(GLcontext *ctx,
			 const GLvisual *visual,
			 GLcontext *share_list,
			 const struct dd_function_table *driverFunctions,
			 void *driverContext)
{
    return static_cast<GLboolean>(
	ctx->initialize(visual, share_list, driverFunctions, driverContext));
}


/**
 * Allocate and initialize a GLcontext structure.
 * Note that the driver needs to pass in its dd_function_table here since
 * we need to at least call driverFunctions->NewTextureObject to initialize
 * the rendering context.
 *
 * \param visual a GLvisual pointer (we copy the struct contents)
 * \param share_list another context to share display lists with or nullptr
 * \param driverFunctions points to the dd_function_table into which the
 *        driver has plugged in all its special functions.
 * \param driverCtx points to the device driver's private context state
 *
 * \return pointer to a new __GLcontextRec or nullptr if error.
 */
GLcontext *
_mesa_create_context(const GLvisual *visual,
		     GLcontext *share_list,
		     const struct dd_function_table *driverFunctions,
		     void *driverContext)
{
    GLcontext *ctx;

    ASSERT(visual);
    ASSERT(driverContext);

    ctx = new GLcontext{};

    if (_mesa_initialize_context(ctx, visual, share_list,
				 driverFunctions, driverContext)) {
	return ctx;
    } else {
	delete ctx;
	return nullptr;
    }
}


/**
 * Free all resources owned by this context (but does not free the
 * __GLcontextRec object itself).  Replaces _mesa_free_context_data().
 */
void
__GLcontextRec::free_data()
{
    if (!_mesa_get_current_context()) {
	/* No current context, but we may need one in order to delete
	 * texture objs, etc.  So temporarily bind this context.
	 */
	_mesa_make_current(this, nullptr, nullptr);
    }

    /* unreference WinSysDraw/Read buffers */
    gl_framebuffer::release(&WinSysDrawBuffer);
    gl_framebuffer::release(&WinSysReadBuffer);
    gl_framebuffer::release(&DrawBuffer);
    gl_framebuffer::release(&ReadBuffer);

    _mesa_free_attrib_data(this);         /* releases texture references on attrib stack */
    _mesa_free_texture_data(this);        /* frees texture objects */
    _mesa_free_matrix_data(this);         /* early release of matrix stack memory */
    _mesa_free_program_data(this);        /* releases shared program references */
    _mesa_free_shader_state(this);        /* cleans up GLSL shader state */
    _mesa_free_query_data(this);          /* frees query objects */

    /* Note: _mesa_free_eval_data, _mesa_free_viewport_data,
     * _mesa_free_lighting_data, and _mesa_free_colortables_data are
     * all no-ops; the corresponding std::vector/std::list members
     * are freed by __GLcontextRec's destructor. */

#if FEATURE_ARB_vertex_buffer_object
    _mesa_delete_buffer_object(this, Array.NullBufferObj);
#endif
    _mesa_delete_array_object(this, Array.DefaultArrayObj);

    /* free dispatch tables */
    delete Exec;
    delete Save;

    /* Shared context state (display lists, textures, etc) */
    if (Shared->unref()) {
	/* free shared state */
	free_shared_state(this, Shared);
    }

    /* Extensions.String is now std::string – destructs automatically */

    /* unbind the context if it's currently bound */
    if (this == _mesa_get_current_context()) {
	_mesa_make_current(nullptr, nullptr, nullptr);
    }
}


/**
 * Free the data associated with the given context.
 *
 * But doesn't free the GLcontext struct itself.
 *
 * \sa _mesa_initialize_context() and init_attrib_groups().
 */
void
_mesa_free_context_data(GLcontext *ctx)
{
    ctx->free_data();
}


/**
 * Destroy a GLcontext structure.
 *
 * \param ctx GL context.
 *
 * Calls free_data() and frees the GLcontext structure itself.
 */
void
_mesa_destroy_context(GLcontext *ctx)
{
    if (ctx) {
	ctx->free_data();
	delete ctx;
    }
}


#if _HAVE_FULL_GL
/**
 * Copy attribute groups from one context to another.
 *
 * \param src source context
 * \param dst destination context
 * \param mask bitwise OR of GL_*_BIT flags
 *
 * According to the bits specified in \p mask, copies the corresponding
 * attributes from \p src into \p dst.  For many of the attributes a simple \c
 * memcpy is not enough due to the existence of internal pointers in their data
 * structures.
 */
void
_mesa_copy_context(const GLcontext *src, GLcontext *dst, GLuint mask)
{
    if (mask & GL_ACCUM_BUFFER_BIT) {
	/* OK to memcpy */
	dst->Accum = src->Accum;
    }
    if (mask & GL_COLOR_BUFFER_BIT) {
	/* OK to memcpy */
	dst->Color = src->Color;
    }
    if (mask & GL_CURRENT_BIT) {
	/* OK to memcpy */
	dst->Current = src->Current;
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
	/* OK to memcpy */
	dst->Depth = src->Depth;
    }
    if (mask & GL_ENABLE_BIT) {
	/* no op */
    }
    if (mask & GL_EVAL_BIT) {
	/* OK to memcpy */
	dst->Eval = src->Eval;
    }
    if (mask & GL_FOG_BIT) {
	/* OK to memcpy */
	dst->Fog = src->Fog;
    }
    if (mask & GL_HINT_BIT) {
	/* OK to memcpy */
	dst->Hint = src->Hint;
    }
    if (mask & GL_LIGHTING_BIT) {
	GLuint i;
	/* copy all lighting state */
	dst->Light = src->Light;
	/* Rebuild the enabled list to point at dst's own light objects */
	dst->Light.EnabledList.clear();
	for (i = 0; i < MAX_LIGHTS; i++) {
	    if (dst->Light.Light[i].Enabled) {
		dst->Light.EnabledList.push_back(&dst->Light.Light[i]);
	    }
	}
    }
    if (mask & GL_LINE_BIT) {
	/* OK to memcpy */
	dst->Line = src->Line;
    }
    if (mask & GL_LIST_BIT) {
	/* OK to memcpy */
	dst->List = src->List;
    }
    if (mask & GL_PIXEL_MODE_BIT) {
	/* OK to memcpy */
	dst->Pixel = src->Pixel;
    }
    if (mask & GL_POINT_BIT) {
	/* OK to memcpy */
	dst->Point = src->Point;
    }
    if (mask & GL_POLYGON_BIT) {
	/* OK to memcpy */
	dst->Polygon = src->Polygon;
    }
    if (mask & GL_POLYGON_STIPPLE_BIT) {
	/* Use loop instead of MEMCPY due to problem with Portland Group's
	 * C compiler.  Reported by John Stone.
	 */
	GLuint i;
	for (i = 0; i < 32; i++) {
	    dst->PolygonStipple[i] = src->PolygonStipple[i];
	}
    }
    if (mask & GL_SCISSOR_BIT) {
	/* OK to memcpy */
	dst->Scissor = src->Scissor;
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
	/* OK to memcpy */
	dst->Stencil = src->Stencil;
    }
    if (mask & GL_TEXTURE_BIT) {
	/* Cannot memcpy because of pointers */
	_mesa_copy_texture_state(src, dst);
    }
    if (mask & GL_TRANSFORM_BIT) {
	/* OK to memcpy */
	dst->Transform = src->Transform;
    }
    if (mask & GL_VIEWPORT_BIT) {
	/* Cannot use memcpy, because of pointers in GLmatrix _WindowMap */
	dst->Viewport.X = src->Viewport.X;
	dst->Viewport.Y = src->Viewport.Y;
	dst->Viewport.Width = src->Viewport.Width;
	dst->Viewport.Height = src->Viewport.Height;
	dst->Viewport.Near = src->Viewport.Near;
	dst->Viewport.Far = src->Viewport.Far;
	dst->Viewport._WindowMap.copy_from(&src->Viewport._WindowMap);
    }

    /* XXX FIXME:  Call callbacks?
     */
    dst->NewState = _NEW_ALL;
}
#endif


/**
 * Check if this context's visual is compatible with the given framebuffer.
 *
 * Replaces the old static check_compatible() helper.
 */
bool
__GLcontextRec::is_visual_compatible(const GLframebuffer *buffer) const
{
    const GLvisual *ctxvis = &Visual;
    const GLvisual *bufvis = &buffer->Visual;

    if (ctxvis == bufvis)
	return true;

    if (ctxvis->rgbMode != bufvis->rgbMode)
	return false;
#if 0
    /* disabling this fixes the fgl_glxgears pbuffer demo */
    if (ctxvis->doubleBufferMode && !bufvis->doubleBufferMode)
	return false;
#endif
    if (ctxvis->stereoMode && !bufvis->stereoMode)
	return false;
    if (ctxvis->haveAccumBuffer && !bufvis->haveAccumBuffer)
	return false;
    if (ctxvis->haveDepthBuffer && !bufvis->haveDepthBuffer)
	return false;
    if (ctxvis->haveStencilBuffer && !bufvis->haveStencilBuffer)
	return false;
    if (ctxvis->redMask && ctxvis->redMask != bufvis->redMask)
	return false;
    if (ctxvis->greenMask && ctxvis->greenMask != bufvis->greenMask)
	return false;
    if (ctxvis->blueMask && ctxvis->blueMask != bufvis->blueMask)
	return false;
#if 0
    /* disabled (see bug 11161) */
    if (ctxvis->depthBits && ctxvis->depthBits != bufvis->depthBits)
	return false;
#endif
    if (ctxvis->stencilBits && ctxvis->stencilBits != bufvis->stencilBits)
	return false;

    return true;
}


/**
 * Bind draw and read framebuffers to this context.
 *
 * Called (for non-null contexts) by _mesa_make_current().
 * Handles visual-compatibility checks, framebuffer reference counting,
 * first-time viewport/scissor setup, and the first-current info print.
 */
void
__GLcontextRec::bind(GLframebuffer *drawBuffer, GLframebuffer *readBuffer)
{
    /* Check that the context's and framebuffer's visuals are compatible. */
    if (drawBuffer && WinSysDrawBuffer != drawBuffer) {
	if (!is_visual_compatible(drawBuffer)) {
	    _mesa_warning(this,
			  "MakeCurrent: incompatible visuals for context and drawbuffer");
	    return;
	}
    }
    if (readBuffer && WinSysReadBuffer != readBuffer) {
	if (!is_visual_compatible(readBuffer)) {
	    _mesa_warning(this,
			  "MakeCurrent: incompatible visuals for context and readbuffer");
	    return;
	}
    }

    _glapi_set_dispatch(CurrentDispatch);

    if (drawBuffer && readBuffer) {
	ASSERT(drawBuffer->Name == 0);
	ASSERT(readBuffer->Name == 0);
	gl_framebuffer::replace(&WinSysDrawBuffer, drawBuffer);
	gl_framebuffer::replace(&WinSysReadBuffer, readBuffer);

	/* Only set Draw/ReadBuffer fields if they're nullptr or not bound to
	 * a user-created FBO. */
	if (!DrawBuffer || DrawBuffer->Name == 0) {
	    gl_framebuffer::replace(&DrawBuffer, drawBuffer);
	}
	if (!ReadBuffer || ReadBuffer->Name == 0) {
	    gl_framebuffer::replace(&ReadBuffer, readBuffer);
	}

	NewState |= _NEW_BUFFERS;

#if _HAVE_FULL_GL
	if (!drawBuffer->Initialized) {
	    GLuint width, height;
	    if (Driver.GetBufferSize) {
		Driver.GetBufferSize(drawBuffer, &width, &height);
		if (Driver.ResizeBuffers)
		    Driver.ResizeBuffers(this, drawBuffer, width, height);
		drawBuffer->Initialized = GL_TRUE;
	    }
	}
	if (readBuffer != drawBuffer && !readBuffer->Initialized) {
	    GLuint width, height;
	    if (Driver.GetBufferSize) {
		Driver.GetBufferSize(readBuffer, &width, &height);
		if (Driver.ResizeBuffers)
		    Driver.ResizeBuffers(this, readBuffer, width, height);
		readBuffer->Initialized = GL_TRUE;
	    }
	}

	_mesa_resizebuffers(this);
#endif

	if (FirstTimeCurrent) {
	    /* set initial viewport and scissor size now */
	    _mesa_set_viewport(this, 0, 0, drawBuffer->Width, drawBuffer->Height);
	    _mesa_set_scissor(this, 0, 0,  drawBuffer->Width, drawBuffer->Height);
	    check_limits();
	}
    }

    /* First time this context is made current: optionally print info. */
    if (FirstTimeCurrent) {
	if (_mesa_getenv("MESA_INFO")) {
	    _mesa_print_info();
	}
	FirstTimeCurrent = GL_FALSE;
    }
}


/**
 * Bind the given context to the given drawBuffer and readBuffer and
 * make it the current context for the calling thread.
 * We'll render into the drawBuffer and read pixels from the
 * readBuffer (i.e. glRead/CopyPixels, glCopyTexImage, etc).
 *
 * We check that the context's and framebuffer's visuals are compatible
 * and return immediately if they're not.
 *
 * \param newCtx  the new GL context. If nullptr then there will be no current GL
 *                context.
 * \param drawBuffer  the drawing framebuffer
 * \param readBuffer  the reading framebuffer
 */
void
_mesa_make_current(GLcontext *newCtx, GLframebuffer *drawBuffer,
		   GLframebuffer *readBuffer)
{
    if (MESA_VERBOSE & VERBOSE_API)
	_mesa_debug(newCtx, "_mesa_make_current()\n");

    /* We used to call _glapi_check_multithread() here.  Now do it in drivers */
    _glapi_set_context((void *) newCtx);
    ASSERT(_mesa_get_current_context() == newCtx);

    if (!newCtx) {
	_glapi_set_dispatch(nullptr);  /* none current */
    } else {
	newCtx->bind(drawBuffer, readBuffer);
    }
}


/**
 * share_state_with: share display-lists, textures and programs with another
 * context.  Returns true if sharing was established.
 */
bool
__GLcontextRec::share_state_with(GLcontext *other)
{
    if (other && Shared && other->Shared) {
	if (Shared->unref())
	    free_shared_state(this, Shared);
	Shared = other->Shared;
	Shared->ref();
	return true;
    }
    return false;
}


/**
 * Make context 'ctx' share the display lists, textures and programs
 * that are associated with 'ctxToShare'.
 * Any display lists, textures or programs associated with 'ctx' will
 * be deleted if nobody else is sharing them.
 */
GLboolean
_mesa_share_state(GLcontext *ctx, GLcontext *ctxToShare)
{
    return ctx ? static_cast<GLboolean>(ctx->share_state_with(ctxToShare)) : GL_FALSE;
}



/**
 * \return pointer to the current GL context for this thread.
 *
 * Calls _glapi_get_context(). This isn't the fastest way to get the current
 * context.  If you need speed, see the #GET_CURRENT_CONTEXT macro in
 * context.h.
 */
GLcontext *
_mesa_get_current_context(void)
{
    return (GLcontext *) _glapi_get_context();
}


/**
 * Get context's current API dispatch table.
 *
 * It'll either be the immediate-mode execute dispatcher or the display list
 * compile dispatcher.
 *
 * \param ctx GL context.
 *
 * \return pointer to dispatch_table.
 *
 * Simply returns __GLcontextRec::CurrentDispatch.
 */
struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx)
{
    return ctx->get_dispatch();
}

/*@}*/


/**********************************************************************/
/** \name Miscellaneous functions                                     */
/**********************************************************************/
/*@{*/

/**
 * Record an error.
 *
 * \param ctx GL context.
 * \param error error code.
 *
 * Records the given error code and call the driver's dd_function_table::Error
 * function if defined.
 *
 * \sa
 * This is called via _mesa_error().
 */
void
_mesa_record_error(GLcontext *ctx, GLenum error)
{
    if (ctx)
	ctx->record_error(error);
}


/**
 * Execute glFinish().
 *
 * Calls the #ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH macro and the
 * dd_function_table::Finish driver callback, if not nullptr.
 */
void GLAPIENTRY
_mesa_Finish(void)
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    if (ctx->Driver.Finish) {
	ctx->Driver.Finish(ctx);
    }
}


/**
 * Execute glFlush().
 *
 * Calls the #ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH macro and the
 * dd_function_table::Flush driver callback, if not nullptr.
 */
void GLAPIENTRY
_mesa_Flush(void)
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    if (ctx->Driver.Flush) {
	ctx->Driver.Flush(ctx);
    }
}


/*@}*/

/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
