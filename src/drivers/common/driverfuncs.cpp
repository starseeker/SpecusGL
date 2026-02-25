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


#include "glheader.h"
#include "imports.h"
#include "buffers.h"
#include "context.h"
#include "framebuffer.h"
#include "program.h"
#include "prog_execute.h"
#include "queryobj.h"
#include "renderbuffer.h"
#include "texcompress.h"
#include "texformat.h"
#include "teximage.h"
#include "texobj.h"
#include "texstore.h"
#if FEATURE_ARB_vertex_buffer_object
#include "bufferobj.h"
#endif
#if FEATURE_EXT_framebuffer_object
#include "fbobject.h"
#include "texrender.h"
#endif
#include "shader_api.h"
#include "arrayobj.h"

#include "driverfuncs.h"
#include "tnl/tnl.h"
#include "swrast/swrast.h"



/**
 * Plug in default functions for all pointers in the dd_function_table
 * structure.
 * Device drivers should call this function and then plug in any
 * functions which it wants to override.
 * Some functions (pointers) MUST be implemented by all drivers (REQUIRED).
 *
 * \param table the dd_function_table to initialize
 */
void
_mesa_init_driver_functions(struct dd_function_table *driver)
{
    _mesa_bzero(driver, sizeof(*driver));

    driver->GetString = nullptr;  /* REQUIRED! */
    driver->UpdateState = nullptr;  /* REQUIRED! */
    driver->GetBufferSize = nullptr;  /* REQUIRED! */
    driver->ResizeBuffers = _mesa_resize_framebuffer;
    driver->Error = nullptr;

    driver->Finish = nullptr;
    driver->Flush = nullptr;

    /* framebuffer/image functions */
    driver->Clear = _swrast_Clear;
    driver->Accum = _swrast_Accum;
    driver->DrawPixels = _swrast_DrawPixels;
    driver->ReadPixels = _swrast_ReadPixels;
    driver->CopyPixels = _swrast_CopyPixels;
    driver->Bitmap = _swrast_Bitmap;

    /* Texture functions */
    driver->ChooseTextureFormat = _mesa_choose_tex_format;
    driver->TexImage1D = _mesa_store_teximage1d;
    driver->TexImage2D = _mesa_store_teximage2d;
    driver->TexImage3D = _mesa_store_teximage3d;
    driver->TexSubImage1D = _mesa_store_texsubimage1d;
    driver->TexSubImage2D = _mesa_store_texsubimage2d;
    driver->TexSubImage3D = _mesa_store_texsubimage3d;
    driver->GetTexImage = _mesa_get_teximage;
    driver->CopyTexImage1D = _swrast_copy_teximage1d;
    driver->CopyTexImage2D = _swrast_copy_teximage2d;
    driver->CopyTexSubImage1D = _swrast_copy_texsubimage1d;
    driver->CopyTexSubImage2D = _swrast_copy_texsubimage2d;
    driver->CopyTexSubImage3D = _swrast_copy_texsubimage3d;
    driver->TestProxyTexImage = _mesa_test_proxy_teximage;
    driver->CompressedTexImage1D = _mesa_store_compressed_teximage1d;
    driver->CompressedTexImage2D = _mesa_store_compressed_teximage2d;
    driver->CompressedTexImage3D = _mesa_store_compressed_teximage3d;
    driver->CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
    driver->CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
    driver->CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;
    driver->GetCompressedTexImage = _mesa_get_compressed_teximage;
    driver->CompressedTextureSize = _mesa_compressed_texture_size;
    driver->BindTexture = nullptr;
    driver->NewTextureObject = _mesa_new_texture_object;
    driver->DeleteTexture = _mesa_delete_texture_object;
    driver->NewTextureImage = _mesa_new_texture_image;
    driver->FreeTexImageData = _mesa_free_texture_image_data;
    driver->MapTexture = nullptr;
    driver->UnmapTexture = nullptr;
    driver->TextureMemCpy = memcpy;
    driver->IsTextureResident = nullptr;
    driver->PrioritizeTexture = nullptr;
    driver->ActiveTexture = nullptr;
    driver->UpdateTexturePalette = nullptr;

    /* imaging */
    driver->CopyColorTable = _swrast_CopyColorTable;
    driver->CopyColorSubTable = _swrast_CopyColorSubTable;
    driver->CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
    driver->CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

    /* Vertex/fragment programs */
    driver->BindProgram = nullptr;
    driver->NewProgram = _mesa_new_program;
    driver->DeleteProgram = _mesa_delete_program;

    /* simple state commands */
    driver->AlphaFunc = nullptr;
    driver->BlendColor = nullptr;
    driver->BlendEquationSeparate = nullptr;
    driver->BlendFuncSeparate = nullptr;
    driver->ClearColor = nullptr;
    driver->ClearDepth = nullptr;
    driver->ClearIndex = nullptr;
    driver->ClearStencil = nullptr;
    driver->ClipPlane = nullptr;
    driver->ColorMask = nullptr;
    driver->ColorMaterial = nullptr;
    driver->CullFace = nullptr;
    driver->DrawBuffer = nullptr;
    driver->DrawBuffers = nullptr;
    driver->FrontFace = nullptr;
    driver->DepthFunc = nullptr;
    driver->DepthMask = nullptr;
    driver->DepthRange = nullptr;
    driver->Enable = nullptr;
    driver->Fogfv = nullptr;
    driver->Hint = nullptr;
    driver->IndexMask = nullptr;
    driver->Lightfv = nullptr;
    driver->LightModelfv = nullptr;
    driver->LineStipple = nullptr;
    driver->LineWidth = nullptr;
    driver->LogicOpcode = nullptr;
    driver->PointParameterfv = nullptr;
    driver->PointSize = nullptr;
    driver->PolygonMode = nullptr;
    driver->PolygonOffset = nullptr;
    driver->PolygonStipple = nullptr;
    driver->ReadBuffer = nullptr;
    driver->RenderMode = nullptr;
    driver->Scissor = nullptr;
    driver->ShadeModel = nullptr;
    driver->StencilFuncSeparate = nullptr;
    driver->StencilOpSeparate = nullptr;
    driver->StencilMaskSeparate = nullptr;
    driver->TexGen = nullptr;
    driver->TexEnv = nullptr;
    driver->TexParameter = nullptr;
    driver->TextureMatrix = nullptr;
    driver->Viewport = nullptr;

    /* vertex arrays */
    driver->VertexPointer = nullptr;
    driver->NormalPointer = nullptr;
    driver->ColorPointer = nullptr;
    driver->FogCoordPointer = nullptr;
    driver->IndexPointer = nullptr;
    driver->SecondaryColorPointer = nullptr;
    driver->TexCoordPointer = nullptr;
    driver->EdgeFlagPointer = nullptr;
    driver->VertexAttribPointer = nullptr;
    driver->LockArraysEXT = nullptr;
    driver->UnlockArraysEXT = nullptr;

    /* state queries */
    driver->GetBooleanv = nullptr;
    driver->GetDoublev = nullptr;
    driver->GetFloatv = nullptr;
    driver->GetIntegerv = nullptr;
    driver->GetPointerv = nullptr;

#if FEATURE_ARB_vertex_buffer_object
    driver->NewBufferObject = _mesa_new_buffer_object;
    driver->DeleteBuffer = _mesa_delete_buffer_object;
    driver->BindBuffer = nullptr;
    driver->BufferData = _mesa_buffer_data;
    driver->BufferSubData = _mesa_buffer_subdata;
    driver->GetBufferSubData = _mesa_buffer_get_subdata;
    driver->MapBuffer = _mesa_buffer_map;
    driver->UnmapBuffer = _mesa_buffer_unmap;
#endif

#if FEATURE_EXT_framebuffer_object
    driver->NewFramebuffer = _mesa_new_framebuffer;
    driver->NewRenderbuffer = _mesa_new_soft_renderbuffer;
    driver->RenderTexture = _mesa_render_texture;
    driver->FinishRenderTexture = _mesa_finish_render_texture;
    driver->FramebufferRenderbuffer = _mesa_framebuffer_renderbuffer;
#endif

#if FEATURE_EXT_framebuffer_blit
    driver->BlitFramebuffer = _swrast_BlitFramebuffer;
#endif

    /* query objects */
    driver->NewQueryObject = _mesa_new_query_object;
    driver->BeginQuery = nullptr;
    driver->EndQuery = nullptr;

    /* APPLE_vertex_array_object */
    driver->NewArrayObject = _mesa_new_array_object;
    driver->DeleteArrayObject = _mesa_delete_array_object;
    driver->BindArrayObject = nullptr;

    /* T&L stuff */
    driver->NeedValidate = GL_FALSE;
    driver->ValidateTnlModule = nullptr;
    driver->CurrentExecPrimitive = 0;
    driver->CurrentSavePrimitive = 0;
    driver->NeedFlush = 0;
    driver->SaveNeedFlush = 0;

    driver->ProgramStringNotify = _tnl_program_string;
    driver->FlushVertices = nullptr;
    driver->SaveFlushVertices = nullptr;
    driver->NotifySaveBegin = nullptr;
    driver->LightingSpaceChange = nullptr;

    /* display list */
    driver->NewList = nullptr;
    driver->EndList = nullptr;
    driver->BeginCallList = nullptr;
    driver->EndCallList = nullptr;


    /* XXX temporary here */
    _mesa_init_glsl_driver_functions(driver);
}


/**
 * Plug in Mesa's GLSL functions.
 */
void
_mesa_init_glsl_driver_functions(struct dd_function_table *driver)
{
    driver->AttachShader = _mesa_attach_shader;
    driver->BindAttribLocation = _mesa_bind_attrib_location;
    driver->CompileShader = _mesa_compile_shader;
    driver->CreateProgram = _mesa_create_program;
    driver->CreateShader = _mesa_create_shader;
    driver->DeleteProgram2 = _mesa_delete_program2;
    driver->DeleteShader = _mesa_delete_shader;
    driver->DetachShader = _mesa_detach_shader;
    driver->GetActiveAttrib = _mesa_get_active_attrib;
    driver->GetActiveUniform = _mesa_get_active_uniform;
    driver->GetAttachedShaders = _mesa_get_attached_shaders;
    driver->GetAttribLocation = _mesa_get_attrib_location;
    driver->GetHandle = _mesa_get_handle;
    driver->GetProgramiv = _mesa_get_programiv;
    driver->GetProgramInfoLog = _mesa_get_program_info_log;
    driver->GetShaderiv = _mesa_get_shaderiv;
    driver->GetShaderInfoLog = _mesa_get_shader_info_log;
    driver->GetShaderSource = _mesa_get_shader_source;
    driver->GetUniformfv = _mesa_get_uniformfv;
    driver->GetUniformiv = _mesa_get_uniformiv;
    driver->GetUniformLocation = _mesa_get_uniform_location;
    driver->IsProgram = _mesa_is_program;
    driver->IsShader = _mesa_is_shader;
    driver->LinkProgram = _mesa_link_program;
    driver->ShaderSource = _mesa_shader_source;
    driver->Uniform = _mesa_uniform;
    driver->UniformMatrix = _mesa_uniform_matrix;
    driver->UseProgram = _mesa_use_program;
    driver->ValidateProgram = _mesa_validate_program;
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
