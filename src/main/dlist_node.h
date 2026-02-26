/**
 * \file dlist_node.h
 * Display list opcode enum and Node union, shared between
 * dlist.cpp and mtypes.h so that mesa_display_list can embed
 * a std::vector<Node> directly.
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

#ifndef DLIST_NODE_H
#define DLIST_NODE_H

#include "glheader.h"

/**
 * Display list opcodes.
 *
 * The fact that these identifiers are assigned consecutive
 * integer values starting at 0 is very important (see InstSize array usage).
 */
typedef enum {
    OPCODE_INVALID = -1,         /* Force signed enum */
    OPCODE_ACCUM,
    OPCODE_ALPHA_FUNC,
    OPCODE_BIND_TEXTURE,
    OPCODE_BITMAP,
    OPCODE_BLEND_COLOR,
    OPCODE_BLEND_EQUATION,
    OPCODE_BLEND_EQUATION_SEPARATE,
    OPCODE_BLEND_FUNC_SEPARATE,
    OPCODE_CALL_LIST,
    OPCODE_CALL_LIST_OFFSET,
    OPCODE_CLEAR,
    OPCODE_CLEAR_ACCUM,
    OPCODE_CLEAR_COLOR,
    OPCODE_CLEAR_DEPTH,
    OPCODE_CLEAR_INDEX,
    OPCODE_CLEAR_STENCIL,
    OPCODE_CLIP_PLANE,
    OPCODE_COLOR_MASK,
    OPCODE_COLOR_MATERIAL,
    OPCODE_COLOR_TABLE,
    OPCODE_COLOR_TABLE_PARAMETER_FV,
    OPCODE_COLOR_TABLE_PARAMETER_IV,
    OPCODE_COLOR_SUB_TABLE,
    OPCODE_CONVOLUTION_FILTER_1D,
    OPCODE_CONVOLUTION_FILTER_2D,
    OPCODE_CONVOLUTION_PARAMETER_I,
    OPCODE_CONVOLUTION_PARAMETER_IV,
    OPCODE_CONVOLUTION_PARAMETER_F,
    OPCODE_CONVOLUTION_PARAMETER_FV,
    OPCODE_COPY_COLOR_SUB_TABLE,
    OPCODE_COPY_COLOR_TABLE,
    OPCODE_COPY_PIXELS,
    OPCODE_COPY_TEX_IMAGE1D,
    OPCODE_COPY_TEX_IMAGE2D,
    OPCODE_COPY_TEX_SUB_IMAGE1D,
    OPCODE_COPY_TEX_SUB_IMAGE2D,
    OPCODE_COPY_TEX_SUB_IMAGE3D,
    OPCODE_CULL_FACE,
    OPCODE_DEPTH_FUNC,
    OPCODE_DEPTH_MASK,
    OPCODE_DEPTH_RANGE,
    OPCODE_DISABLE,
    OPCODE_DRAW_BUFFER,
    OPCODE_DRAW_PIXELS,
    OPCODE_ENABLE,
    OPCODE_EVALMESH1,
    OPCODE_EVALMESH2,
    OPCODE_FOG,
    OPCODE_FRONT_FACE,
    OPCODE_FRUSTUM,
    OPCODE_HINT,
    OPCODE_HISTOGRAM,
    OPCODE_INDEX_MASK,
    OPCODE_INIT_NAMES,
    OPCODE_LIGHT,
    OPCODE_LIGHT_MODEL,
    OPCODE_LINE_STIPPLE,
    OPCODE_LINE_WIDTH,
    OPCODE_LIST_BASE,
    OPCODE_LOAD_IDENTITY,
    OPCODE_LOAD_MATRIX,
    OPCODE_LOAD_NAME,
    OPCODE_LOGIC_OP,
    OPCODE_MAP1,
    OPCODE_MAP2,
    OPCODE_MAPGRID1,
    OPCODE_MAPGRID2,
    OPCODE_MATRIX_MODE,
    OPCODE_MIN_MAX,
    OPCODE_MULT_MATRIX,
    OPCODE_ORTHO,
    OPCODE_PASSTHROUGH,
    OPCODE_PIXEL_MAP,
    OPCODE_PIXEL_TRANSFER,
    OPCODE_PIXEL_ZOOM,
    OPCODE_POINT_SIZE,
    OPCODE_POINT_PARAMETERS,
    OPCODE_POLYGON_MODE,
    OPCODE_POLYGON_STIPPLE,
    OPCODE_POLYGON_OFFSET,
    OPCODE_POP_ATTRIB,
    OPCODE_POP_MATRIX,
    OPCODE_POP_NAME,
    OPCODE_PRIORITIZE_TEXTURE,
    OPCODE_PUSH_ATTRIB,
    OPCODE_PUSH_MATRIX,
    OPCODE_PUSH_NAME,
    OPCODE_RASTER_POS,
    OPCODE_READ_BUFFER,
    OPCODE_RESET_HISTOGRAM,
    OPCODE_RESET_MIN_MAX,
    OPCODE_ROTATE,
    OPCODE_SCALE,
    OPCODE_SCISSOR,
    OPCODE_SELECT_TEXTURE_SGIS,
    OPCODE_SELECT_TEXTURE_COORD_SET,
    OPCODE_SHADE_MODEL,
    OPCODE_STENCIL_FUNC,
    OPCODE_STENCIL_MASK,
    OPCODE_STENCIL_OP,
    OPCODE_TEXENV,
    OPCODE_TEXGEN,
    OPCODE_TEXPARAMETER,
    OPCODE_TEX_IMAGE1D,
    OPCODE_TEX_IMAGE2D,
    OPCODE_TEX_IMAGE3D,
    OPCODE_TEX_SUB_IMAGE1D,
    OPCODE_TEX_SUB_IMAGE2D,
    OPCODE_TEX_SUB_IMAGE3D,
    OPCODE_TRANSLATE,
    OPCODE_VIEWPORT,
    OPCODE_WINDOW_POS,
    /* GL_ARB_multitexture */
    OPCODE_ACTIVE_TEXTURE,
    /* GL_ARB_texture_compression */
    OPCODE_COMPRESSED_TEX_IMAGE_1D,
    OPCODE_COMPRESSED_TEX_IMAGE_2D,
    OPCODE_COMPRESSED_TEX_IMAGE_3D,
    OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D,
    OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D,
    OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D,
    /* GL_ARB_multisample */
    OPCODE_SAMPLE_COVERAGE,
    /* GL_ARB_window_pos */
    OPCODE_WINDOW_POS_ARB,
    /* GL_NV_vertex_program */
    OPCODE_BIND_PROGRAM_NV,
    OPCODE_EXECUTE_PROGRAM_NV,
    OPCODE_REQUEST_RESIDENT_PROGRAMS_NV,
    OPCODE_LOAD_PROGRAM_NV,
    OPCODE_PROGRAM_PARAMETER4F_NV,
    OPCODE_TRACK_MATRIX_NV,
    /* GL_NV_fragment_program */
    OPCODE_PROGRAM_LOCAL_PARAMETER_ARB,
    OPCODE_PROGRAM_NAMED_PARAMETER_NV,
    /* GL_EXT_stencil_two_side */
    OPCODE_ACTIVE_STENCIL_FACE_EXT,
    /* GL_EXT_depth_bounds_test */
    OPCODE_DEPTH_BOUNDS_EXT,
    /* GL_ARB_vertex/fragment_program */
    OPCODE_PROGRAM_STRING_ARB,
    OPCODE_PROGRAM_ENV_PARAMETER_ARB,
    /* GL_ARB_occlusion_query */
    OPCODE_BEGIN_QUERY_ARB,
    OPCODE_END_QUERY_ARB,
    /* GL_ARB_draw_buffers */
    OPCODE_DRAW_BUFFERS_ARB,
    /* GL_ATI_fragment_shader */
    OPCODE_BIND_FRAGMENT_SHADER_ATI,
    OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI,
    /* OpenGL 2.0 */
    OPCODE_STENCIL_FUNC_SEPARATE,
    OPCODE_STENCIL_OP_SEPARATE,
    OPCODE_STENCIL_MASK_SEPARATE,

    /* GL_EXT_framebuffer_blit */
    OPCODE_BLIT_FRAMEBUFFER,

    /* Vertex attributes -- fallback for when optimized display
     * list build isn't active.
     */
    OPCODE_ATTR_1F_NV,
    OPCODE_ATTR_2F_NV,
    OPCODE_ATTR_3F_NV,
    OPCODE_ATTR_4F_NV,
    OPCODE_ATTR_1F_ARB,
    OPCODE_ATTR_2F_ARB,
    OPCODE_ATTR_3F_ARB,
    OPCODE_ATTR_4F_ARB,
    OPCODE_MATERIAL,
    OPCODE_BEGIN,
    DLIST_OPCODE_END,
    OPCODE_RECTF,
    OPCODE_EVAL_C1,
    OPCODE_EVAL_C2,
    OPCODE_EVAL_P1,
    OPCODE_EVAL_P2,

    /* The following two are meta instructions */
    OPCODE_ERROR,                /* raise compiled-in error */
    OPCODE_END_OF_LIST,
    OPCODE_EXT_0
} OpCode;


/**
 * Display list node.
 *
 * Display list instructions are stored as a contiguous array of nodes.
 * Each instruction in the display list is stored as a sequence of
 * contiguous nodes in memory.
 * Each node is the union of a variety of data types.
 */
union node {
    OpCode opcode;
    GLboolean b;
    GLbitfield bf;
    GLubyte ub;
    GLshort s;
    GLushort us;
    GLint i;
    GLuint ui;
    GLenum e;
    GLfloat f;
    GLvoid *data;
};

typedef union node Node;

#endif /* DLIST_NODE_H */
