/**
 * \file dd.h
 * Device driver interfaces.
 */

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


#ifndef DD_INCLUDED
#define DD_INCLUDED

#include <functional>

/* THIS FILE ONLY INCLUDED BY mtypes.h !!!!! */

struct gl_pixelstore_attrib;
struct mesa_display_list;

/**
 * Device driver function table.
 * Core Mesa uses these function pointers to call into device drivers.
 * Most of these functions directly correspond to OpenGL state commands.
 * Core Mesa will call these functions after error checking has been done
 * so that the drivers don't have to worry about error testing.
 *
 * Vertex transformation/clipping/lighting is patched into the T&L module.
 * Rasterization functions are patched into the swrast module.
 *
 * Note: when new functions are added here, the drivers/common/driverfuncs.c
 * file should be updated too!!!
 */
constexpr GLuint FLUSH_STORED_VERTICES = 0x1U;
constexpr GLuint FLUSH_UPDATE_CURRENT  = 0x2U;
constexpr GLenum PRIM_OUTSIDE_BEGIN_END   = GL_POLYGON + 1U;
constexpr GLenum PRIM_INSIDE_UNKNOWN_PRIM = GL_POLYGON + 2U;
constexpr GLenum PRIM_UNKNOWN             = GL_POLYGON + 3U;

struct dd_function_table {
    /**
     * Return a string as needed by glGetString().
     * Only the GL_RENDERER query must be implemented.  Otherwise, nullptr can be
     * returned.
     */
    std::function<const GLubyte *(GLcontext *ctx, GLenum name)> GetString;

    /**
     * Notify the driver after Mesa has made some internal state changes.
     *
     * This is in addition to any state change callbacks Mesa may already have
     * made.
     */
    std::function<void(GLcontext *ctx, GLbitfield new_state)> UpdateState;

    /**
     * Get the width and height of the named buffer/window.
     *
     * Mesa uses this to determine when the driver's window size has changed.
     * XXX OBSOLETE: this function will be removed in the future.
     */
    std::function<void(GLframebuffer *buffer, GLuint *width, GLuint *height)> GetBufferSize;

    /**
     * Resize the given framebuffer to the given size.
     * XXX OBSOLETE: this function will be removed in the future.
     */
    std::function<void(GLcontext *ctx, GLframebuffer *fb, GLuint width, GLuint height)> ResizeBuffers;

    /**
     * Called whenever an error is generated.
     * __GLcontextRec::ErrorValue contains the error value.
     */
    std::function<void(GLcontext *ctx)> Error;

    /**
     * This is called whenever glFinish() is called.
     */
    std::function<void(GLcontext *ctx)> Finish;

    /**
     * This is called whenever glFlush() is called.
     */
    std::function<void(GLcontext *ctx)> Flush;

    /**
     * Clear the color/depth/stencil/accum buffer(s).
     * \param buffers  a bitmask of BUFFER_BIT_* flags indicating which
     *                 renderbuffers need to be cleared.
     */
    std::function<void(GLcontext *ctx, GLbitfield buffers)> Clear;

    /**
     * Execute glAccum command.
     */
    std::function<void(GLcontext *ctx, GLenum op, GLfloat value)> Accum;


    /**
     * \name Image-related functions
     */
    /*@{*/

    /**
     * Called by glDrawPixels().
     * \p unpack describes how to unpack the source image data.
     */
    std::function<void(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const struct gl_pixelstore_attrib *unpack, const GLvoid *pixels)> DrawPixels;

    /**
     * Called by glReadPixels().
     */
    std::function<void(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const struct gl_pixelstore_attrib *unpack, GLvoid *dest)> ReadPixels;

    /**
     * Called by glCopyPixels().
     */
    std::function<void(GLcontext *ctx, GLint srcx, GLint srcy, GLsizei width, GLsizei height, GLint dstx, GLint dsty, GLenum type)> CopyPixels;

    /**
     * Called by glBitmap().
     */
    std::function<void(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap)> Bitmap;
    /*@}*/


    /**
     * \name Texture image functions
     */
    /*@{*/

    /**
     * Choose texture format.
     *
     * This is called by the \c _mesa_store_tex[sub]image[123]d() fallback
     * functions.  The driver should examine \p internalFormat and return a
     * pointer to an appropriate gl_texture_format.
     */
    std::function<const struct gl_texture_format *(GLcontext *ctx, GLint internalFormat, GLenum srcFormat, GLenum srcType)> ChooseTextureFormat;

    /**
     * Called by glTexImage1D().
     *
     * \param target user specified.
     * \param format user specified.
     * \param type user specified.
     * \param pixels user specified.
     * \param packing indicates the image packing of pixels.
     * \param texObj is the target texture object.
     * \param texImage is the target texture image.  It will have the texture \p
     * width, \p height, \p depth, \p border and \p internalFormat information.
     *
     * \p retainInternalCopy is returned by this function and indicates whether
     * core Mesa should keep an internal copy of the texture image.
     *
     * Drivers should call a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> TexImage1D;

    /**
     * Called by glTexImage2D().
     *
     * \sa dd_function_table::TexImage1D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint height, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> TexImage2D;

    /**
     * Called by glTexImage3D().
     *
     * \sa dd_function_table::TexImage1D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint height, GLint depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> TexImage3D;

    /**
     * Called by glTexSubImage1D().
     *
     * \param target user specified.
     * \param level user specified.
     * \param xoffset user specified.
     * \param yoffset user specified.
     * \param zoffset user specified.
     * \param width user specified.
     * \param height user specified.
     * \param depth user specified.
     * \param format user specified.
     * \param type user specified.
     * \param pixels user specified.
     * \param packing indicates the image packing of pixels.
     * \param texObj is the target texture object.
     * \param texImage is the target texture image.  It will have the texture \p
     * width, \p height, \p border and \p internalFormat information.
     *
     * The driver should use a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> TexSubImage1D;

    /**
     * Called by glTexSubImage2D().
     *
     * \sa dd_function_table::TexSubImage1D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> TexSubImage2D;

    /**
     * Called by glTexSubImage3D().
     *
     * \sa dd_function_table::TexSubImage1D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLint depth, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> TexSubImage3D;

    /**
     * Called by glGetTexImage().
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> GetTexImage;

    /**
     * Called by glCopyTexImage1D().
     *
     * Drivers should use a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)> CopyTexImage1D;

    /**
     * Called by glCopyTexImage2D().
     *
     * Drivers should use a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)> CopyTexImage2D;

    /**
     * Called by glCopyTexSubImage1D().
     *
     * Drivers should use a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)> CopyTexSubImage1D;
    /**
     * Called by glCopyTexSubImage2D().
     *
     * Drivers should use a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)> CopyTexSubImage2D;
    /**
     * Called by glCopyTexSubImage3D().
     *
     * Drivers should use a fallback routine from texstore.c if needed.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)> CopyTexSubImage3D;

    /**
     * Called by glTexImage[123]D when user specifies a proxy texture
     * target.
     *
     * \return GL_TRUE if the proxy test passes, or GL_FALSE if the test fails.
     */
    std::function<bool(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLenum format, GLenum type, GLint width, GLint height, GLint depth, GLint border)> TestProxyTexImage;
    /*@}*/


    /**
     * \name Compressed texture functions
     */
    /*@{*/

    /**
     * Called by glCompressedTexImage1D().
     *
     * \param target user specified.
     * \param format user specified.
     * \param type user specified.
     * \param pixels user specified.
     * \param packing indicates the image packing of pixels.
     * \param texObj is the target texture object.
     * \param texImage is the target texture image.  It will have the texture \p
     * width, \p height, \p depth, \p border and \p internalFormat information.
     *
     * \a retainInternalCopy is returned by this function and indicates whether
     * core Mesa should keep an internal copy of the texture image.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> CompressedTexImage1D;
    /**
     * Called by glCompressedTexImage2D().
     *
     * \sa dd_function_table::CompressedTexImage1D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> CompressedTexImage2D;
    /**
     * Called by glCompressedTexImage3D().
     *
     * \sa dd_function_table::CompressedTexImage3D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> CompressedTexImage3D;

    /**
     * Called by glCompressedTexSubImage1D().
     *
     * \param target user specified.
     * \param level user specified.
     * \param xoffset user specified.
     * \param yoffset user specified.
     * \param zoffset user specified.
     * \param width user specified.
     * \param height user specified.
     * \param depth user specified.
     * \param imageSize user specified.
     * \param data user specified.
     * \param texObj is the target texture object.
     * \param texImage is the target texture image.  It will have the texture \p
     * width, \p height, \p depth, \p border and \p internalFormat information.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> CompressedTexSubImage1D;
    /**
     * Called by glCompressedTexSubImage2D().
     *
     * \sa dd_function_table::CompressedTexImage3D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLint height, GLenum format, GLsizei imageSize, const GLvoid *data, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> CompressedTexSubImage2D;
    /**
     * Called by glCompressedTexSubImage3D().
     *
     * \sa dd_function_table::CompressedTexImage3D.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLint height, GLint depth, GLenum format, GLsizei imageSize, const GLvoid *data, struct gl_texture_object *texObj, struct gl_texture_image *texImage)> CompressedTexSubImage3D;


    /**
     * Called by glGetCompressedTexImage.
     */
    std::function<void(GLcontext *ctx, GLenum target, GLint level, GLvoid *img, const struct gl_texture_object *texObj, const struct gl_texture_image *texImage)> GetCompressedTexImage;

    /**
     * Called to query number of bytes of storage needed to store the
     * specified compressed texture.
     */
    std::function<GLuint(GLcontext *ctx, GLsizei width, GLsizei height, GLsizei depth, GLenum format)> CompressedTextureSize;
    /*@}*/

    /**
     * \name Texture object functions
     */
    /*@{*/

    /**
     * Called by glBindTexture().
     */
    std::function<void(GLcontext *ctx, GLenum target, struct gl_texture_object *tObj)> BindTexture;

    /**
     * Called to allocate a new texture object.
     * A new gl_texture_object should be returned.  The driver should
     * attach to it any device-specific info it needs.
     */
    std::function<struct gl_texture_object *(GLcontext *ctx, GLuint name, GLenum target)> NewTextureObject;
    /**
     * Called when a texture object is about to be deallocated.
     *
     * Driver should delete the gl_texture_object object and anything
     * hanging off of it.
     */
    std::function<void(GLcontext *ctx, struct gl_texture_object *tObj)> DeleteTexture;

    /**
     * Called to allocate a new texture image object.
     */
    std::function<struct gl_texture_image *(GLcontext *ctx)> NewTextureImage;

    /**
     * Called to free tImage->Data.
     */
    std::function<void(GLcontext *ctx, struct gl_texture_image *tImage)> FreeTexImageData;

    /** Map texture image data into user space */
    std::function<void(GLcontext *ctx, struct gl_texture_object *tObj)> MapTexture;
    /** Unmap texture images from user space */
    std::function<void(GLcontext *ctx, struct gl_texture_object *tObj)> UnmapTexture;

    /**
     * Note: no context argument.  This function doesn't initially look
     * like it belongs here, except that the driver is the only entity
     * that knows for sure how the texture memory is allocated - via
     * the above callbacks.  There is then an argument that the driver
     * knows what memcpy paths might be fast.  Typically this is invoked with
     *
     * to -- a pointer into texture memory allocated by NewTextureImage() above.
     * from -- a pointer into client memory or a mesa temporary.
     * sz -- nr bytes to copy.
     */
    std::function<void*(void *to, const void *from, size_t sz)> TextureMemCpy;

    /**
     * Called by glAreTextureResident().
     */
    std::function<GLboolean(GLcontext *ctx, struct gl_texture_object *t)> IsTextureResident;

    /**
     * Called by glPrioritizeTextures().
     */
    std::function<void(GLcontext *ctx, struct gl_texture_object *t, GLclampf priority)> PrioritizeTexture;

    /**
     * Called by glActiveTextureARB() to set current texture unit.
     */
    std::function<void(GLcontext *ctx, GLuint texUnitNumber)> ActiveTexture;

    /**
     * Called when the texture's color lookup table is changed.
     *
     * If \p tObj is nullptr then the shared texture palette
     * gl_texture_object::Palette is to be updated.
     */
    std::function<void(GLcontext *ctx, struct gl_texture_object *tObj)> UpdateTexturePalette;
    /*@}*/


    /**
     * \name Imaging functionality
     */
    /*@{*/
    std::function<void(GLcontext *ctx, GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)> CopyColorTable;

    std::function<void(GLcontext *ctx, GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)> CopyColorSubTable;

    std::function<void(GLcontext *ctx, GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width)> CopyConvolutionFilter1D;

    std::function<void(GLcontext *ctx, GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height)> CopyConvolutionFilter2D;
    /*@}*/


    /**
     * \name Vertex/fragment program functions
     */
    /*@{*/
    /** Bind a vertex/fragment program */
    std::function<void(GLcontext *ctx, GLenum target, struct gl_program *prog)> BindProgram;
    /** Allocate a new program */
    std::function<struct gl_program *(GLcontext *ctx, GLenum target, GLuint id)> NewProgram;
    /** Delete a program */
    std::function<void(GLcontext *ctx, struct gl_program *prog)> DeleteProgram;
    /** Notify driver that a program string has been specified. */
    std::function<void(GLcontext *ctx, GLenum target, struct gl_program *prog)> ProgramStringNotify;
    /** Get value of a program register during program execution. */
    std::function<void(GLcontext *ctx, enum register_file file, GLuint index, GLfloat val[4])> GetProgramRegister;

    /** Query if program can be loaded onto hardware */
    std::function<GLboolean(GLcontext *ctx, GLenum target, struct gl_program *prog)> IsProgramNative;

    /*@}*/


    /**
     * \name State-changing functions.
     *
     * \note drawing functions are above.
     *
     * These functions are called by their corresponding OpenGL API functions.
     * They are \e also called by the gl_PopAttrib() function!!!
     * May add more functions like these to the device driver in the future.
     */
    /*@{*/
    /** Specify the alpha test function */
    std::function<void(GLcontext *ctx, GLenum func, GLfloat ref)> AlphaFunc;
    /** Set the blend color */
    std::function<void(GLcontext *ctx, const GLfloat color[4])> BlendColor;
    /** Set the blend equation */
    std::function<void(GLcontext *ctx, GLenum modeRGB, GLenum modeA)> BlendEquationSeparate;
    /** Specify pixel arithmetic */
    std::function<void(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorA, GLenum dfactorA)> BlendFuncSeparate;
    /** Specify clear values for the color buffers */
    std::function<void(GLcontext *ctx, const GLfloat color[4])> ClearColor;
    /** Specify the clear value for the depth buffer */
    std::function<void(GLcontext *ctx, GLclampd d)> ClearDepth;
    /** Specify the clear value for the color index buffers */
    std::function<void(GLcontext *ctx, GLuint index)> ClearIndex;
    /** Specify the clear value for the stencil buffer */
    std::function<void(GLcontext *ctx, GLint s)> ClearStencil;
    /** Specify a plane against which all geometry is clipped */
    std::function<void(GLcontext *ctx, GLenum plane, const GLfloat *equation)> ClipPlane;
    /** Enable and disable writing of frame buffer color components */
    std::function<void(GLcontext *ctx, GLboolean rmask, GLboolean gmask, GLboolean bmask, GLboolean amask)> ColorMask;
    /** Cause a material color to track the current color */
    std::function<void(GLcontext *ctx, GLenum face, GLenum mode)> ColorMaterial;
    /** Specify whether front- or back-facing facets can be culled */
    std::function<void(GLcontext *ctx, GLenum mode)> CullFace;
    /** Define front- and back-facing polygons */
    std::function<void(GLcontext *ctx, GLenum mode)> FrontFace;
    /** Specify the value used for depth buffer comparisons */
    std::function<void(GLcontext *ctx, GLenum func)> DepthFunc;
    /** Enable or disable writing into the depth buffer */
    std::function<void(GLcontext *ctx, GLboolean flag)> DepthMask;
    /** Specify mapping of depth values from NDC to window coordinates */
    std::function<void(GLcontext *ctx, GLclampd nearval, GLclampd farval)> DepthRange;
    /** Specify the current buffer for writing */
    std::function<void(GLcontext *ctx, GLenum buffer)> DrawBuffer;
    /** Specify the buffers for writing for fragment programs*/
    std::function<void(GLcontext *ctx, GLsizei n, const GLenum *buffers)> DrawBuffers;
    /** Enable or disable server-side gl capabilities */
    std::function<void(GLcontext *ctx, GLenum cap, GLboolean state)> Enable;
    /** Specify fog parameters */
    std::function<void(GLcontext *ctx, GLenum pname, const GLfloat *params)> Fogfv;
    /** Specify implementation-specific hints */
    std::function<void(GLcontext *ctx, GLenum target, GLenum mode)> Hint;
    /** Control the writing of individual bits in the color index buffers */
    std::function<void(GLcontext *ctx, GLuint mask)> IndexMask;
    /** Set light source parameters.
     * Note: for GL_POSITION and GL_SPOT_DIRECTION, params will have already
     * been transformed to eye-space.
     */
    std::function<void(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params)> Lightfv;
    /** Set the lighting model parameters */
    std::function<void(GLcontext *ctx, GLenum pname, const GLfloat *params)> LightModelfv;
    /** Specify the line stipple pattern */
    std::function<void(GLcontext *ctx, GLint factor, GLushort pattern)> LineStipple;
    /** Specify the width of rasterized lines */
    std::function<void(GLcontext *ctx, GLfloat width)> LineWidth;
    /** Specify a logical pixel operation for color index rendering */
    std::function<void(GLcontext *ctx, GLenum opcode)> LogicOpcode;
    std::function<void(GLcontext *ctx, GLenum pname, const GLfloat *params)> PointParameterfv;
    /** Specify the diameter of rasterized points */
    std::function<void(GLcontext *ctx, GLfloat size)> PointSize;
    /** Select a polygon rasterization mode */
    std::function<void(GLcontext *ctx, GLenum face, GLenum mode)> PolygonMode;
    /** Set the scale and units used to calculate depth values */
    std::function<void(GLcontext *ctx, GLfloat factor, GLfloat units)> PolygonOffset;
    /** Set the polygon stippling pattern */
    std::function<void(GLcontext *ctx, const GLubyte *mask)> PolygonStipple;
    /* Specifies the current buffer for reading */
    std::function<void(GLcontext *ctx, GLenum buffer)> ReadBuffer;
    /** Set rasterization mode */
    std::function<void(GLcontext *ctx, GLenum mode)> RenderMode;
    /** Define the scissor box */
    std::function<void(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)> Scissor;
    /** Select flat or smooth shading */
    std::function<void(GLcontext *ctx, GLenum mode)> ShadeModel;
    /** OpenGL 2.0 two-sided StencilFunc */
    std::function<void(GLcontext *ctx, GLenum face, GLenum func, GLint ref, GLuint mask)> StencilFuncSeparate;
    /** OpenGL 2.0 two-sided StencilMask */
    std::function<void(GLcontext *ctx, GLenum face, GLuint mask)> StencilMaskSeparate;
    /** OpenGL 2.0 two-sided StencilOp */
    std::function<void(GLcontext *ctx, GLenum face, GLenum fail, GLenum zfail, GLenum zpass)> StencilOpSeparate;
    /** Control the generation of texture coordinates */
    std::function<void(GLcontext *ctx, GLenum coord, GLenum pname, const GLfloat *params)> TexGen;
    /** Set texture environment parameters */
    std::function<void(GLcontext *ctx, GLenum target, GLenum pname, const GLfloat *param)> TexEnv;
    /** Set texture parameters */
    std::function<void(GLcontext *ctx, GLenum target, struct gl_texture_object *texObj, GLenum pname, const GLfloat *params)> TexParameter;
    std::function<void(GLcontext *ctx, GLuint unit, const GLmatrix *mat)> TextureMatrix;
    /** Set the viewport */
    std::function<void(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)> Viewport;
    /*@}*/


    /**
     * \name Vertex array functions
     *
     * Called by the corresponding OpenGL functions.
     */
    /*@{*/
    std::function<void(GLcontext *ctx, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)> VertexPointer;
    std::function<void(GLcontext *ctx, GLenum type, GLsizei stride, const GLvoid *ptr)> NormalPointer;
    std::function<void(GLcontext *ctx, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)> ColorPointer;
    std::function<void(GLcontext *ctx, GLenum type, GLsizei stride, const GLvoid *ptr)> FogCoordPointer;
    std::function<void(GLcontext *ctx, GLenum type, GLsizei stride, const GLvoid *ptr)> IndexPointer;
    std::function<void(GLcontext *ctx, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)> SecondaryColorPointer;
    std::function<void(GLcontext *ctx, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)> TexCoordPointer;
    std::function<void(GLcontext *ctx, GLsizei stride, const GLvoid *ptr)> EdgeFlagPointer;
    std::function<void(GLcontext *ctx, GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)> VertexAttribPointer;
    std::function<void(GLcontext *ctx, GLint first, GLsizei count)> LockArraysEXT;
    std::function<void(GLcontext *ctx)> UnlockArraysEXT;
    /*@}*/


    /**
     * \name State-query functions
     *
     * Return GL_TRUE if query was completed, GL_FALSE otherwise.
     */
    /*@{*/
    /** Return the value or values of a selected parameter */
    std::function<GLboolean(GLcontext *ctx, GLenum pname, GLboolean *result)> GetBooleanv;
    /** Return the value or values of a selected parameter */
    std::function<GLboolean(GLcontext *ctx, GLenum pname, GLdouble *result)> GetDoublev;
    /** Return the value or values of a selected parameter */
    std::function<GLboolean(GLcontext *ctx, GLenum pname, GLfloat *result)> GetFloatv;
    /** Return the value or values of a selected parameter */
    std::function<GLboolean(GLcontext *ctx, GLenum pname, GLint *result)> GetIntegerv;
    /** Return the value or values of a selected parameter */
    std::function<GLboolean(GLcontext *ctx, GLenum pname, GLvoid **result)> GetPointerv;
    /*@}*/


    /**
     * \name Vertex/pixel buffer object functions
     */
#if FEATURE_ARB_vertex_buffer_object
    /*@{*/
    std::function<void(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)> BindBuffer;

    std::function<struct gl_buffer_object *(GLcontext *ctx, GLuint buffer, GLenum target)> NewBufferObject;

    std::function<void(GLcontext *ctx, struct gl_buffer_object *obj)> DeleteBuffer;

    std::function<void(GLcontext *ctx, GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage, struct gl_buffer_object *obj)> BufferData;

    std::function<void(GLcontext *ctx, GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data, struct gl_buffer_object *obj)> BufferSubData;

    std::function<void(GLcontext *ctx, GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data, struct gl_buffer_object *obj)> GetBufferSubData;

    std::function<void *(GLcontext *ctx, GLenum target, GLenum access, struct gl_buffer_object *obj)> MapBuffer;

    std::function<bool(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)> UnmapBuffer;
    /*@}*/
#endif

    /**
     * \name Functions for GL_EXT_framebuffer_object
     */
#if FEATURE_EXT_framebuffer_object
    /*@{*/
    std::function<struct gl_framebuffer *(GLcontext *ctx, GLuint name)> NewFramebuffer;
    std::function<struct gl_renderbuffer *(GLcontext *ctx, GLuint name)> NewRenderbuffer;
    std::function<void(GLcontext *ctx, GLenum target, struct gl_framebuffer *fb)> BindFramebuffer;
    std::function<void(GLcontext *ctx, struct gl_framebuffer *fb, GLenum attachment, struct gl_renderbuffer *rb)> FramebufferRenderbuffer;
    std::function<void(GLcontext *ctx, struct gl_framebuffer *fb, struct gl_renderbuffer_attachment *att)> RenderTexture;
    std::function<void(GLcontext *ctx, struct gl_renderbuffer_attachment *att)> FinishRenderTexture;
    /*@}*/
#endif
#if FEATURE_EXT_framebuffer_blit
    std::function<void(GLcontext *ctx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)> BlitFramebuffer;
#endif

    /**
     * \name Query objects
     */
    /*@{*/
    std::function<struct gl_query_object *(GLcontext *ctx, GLuint id)> NewQueryObject;
    std::function<void(GLcontext *ctx, GLenum target, struct gl_query_object *q)> BeginQuery;
    std::function<void(GLcontext *ctx, GLenum target, struct gl_query_object *q)> EndQuery;
    /*@}*/


    /**
     * \name Vertex Array objects
     */
    /*@{*/
    std::function<struct gl_array_object *(GLcontext *ctx, GLuint id)> NewArrayObject;
    std::function<void(GLcontext *ctx, struct gl_array_object *obj)> DeleteArrayObject;
    std::function<void(GLcontext *ctx, struct gl_array_object *obj)> BindArrayObject;
    /*@}*/

    /**
     * \name GLSL-related functions (ARB extensions and OpenGL 2.x)
     */
    /*@{*/
    std::function<void(GLcontext *ctx, GLuint program, GLuint shader)> AttachShader;
    std::function<void(GLcontext *ctx, GLuint program, GLuint index, const GLcharARB *name)> BindAttribLocation;
    std::function<void(GLcontext *ctx, GLuint shader)> CompileShader;
    std::function<GLuint(GLcontext *ctx, GLenum type)> CreateShader;
    std::function<GLuint(GLcontext *ctx)> CreateProgram;
    std::function<void(GLcontext *ctx, GLuint program)> DeleteProgram2;
    std::function<void(GLcontext *ctx, GLuint shader)> DeleteShader;
    std::function<void(GLcontext *ctx, GLuint program, GLuint shader)> DetachShader;
    std::function<void(GLcontext *ctx, GLuint program, GLuint index, GLsizei maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)> GetActiveAttrib;
    std::function<void(GLcontext *ctx, GLuint program, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)> GetActiveUniform;
    std::function<void(GLcontext *ctx, GLuint program, GLsizei maxCount, GLsizei *count, GLuint *obj)> GetAttachedShaders;
    std::function<GLint(GLcontext *ctx, GLuint program, const GLcharARB *name)> GetAttribLocation;
    std::function<GLuint(GLcontext *ctx, GLenum pname)> GetHandle;
    std::function<void(GLcontext *ctx, GLuint program, GLenum pname, GLint *params)> GetProgramiv;
    std::function<void(GLcontext *ctx, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)> GetProgramInfoLog;
    std::function<void(GLcontext *ctx, GLuint shader, GLenum pname, GLint *params)> GetShaderiv;
    std::function<void(GLcontext *ctx, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog)> GetShaderInfoLog;
    std::function<void(GLcontext *ctx, GLuint shader, GLsizei maxLength, GLsizei *length, GLcharARB *sourceOut)> GetShaderSource;
    std::function<void(GLcontext *ctx, GLuint program, GLint location, GLfloat *params)> GetUniformfv;
    std::function<void(GLcontext *ctx, GLuint program, GLint location, GLint *params)> GetUniformiv;
    std::function<GLint(GLcontext *ctx, GLuint program, const GLcharARB *name)> GetUniformLocation;
    std::function<bool(GLcontext *ctx, GLuint name)> IsProgram;
    std::function<bool(GLcontext *ctx, GLuint name)> IsShader;
    std::function<void(GLcontext *ctx, GLuint program)> LinkProgram;
    std::function<void(GLcontext *ctx, GLuint shader, const GLchar *source)> ShaderSource;
    std::function<void(GLcontext *ctx, GLint location, GLsizei count, const GLvoid *values, GLenum type)> Uniform;
    std::function<void(GLcontext *ctx, GLint cols, GLint rows, GLenum matrixType, GLint location, GLsizei count, GLboolean transpose, const GLfloat *values)> UniformMatrix;
    std::function<void(GLcontext *ctx, GLuint program)> UseProgram;
    std::function<void(GLcontext *ctx, GLuint program)> ValidateProgram;
    /* XXX many more to come */
    /*@}*/


    /**
     * \name Support for multiple T&L engines
     */
    /*@{*/

    /**
     * Bitmask of state changes that require the current T&L module to be
     * validated, using ValidateTnlModule() below.
     */
    GLuint NeedValidate = 0;

    /**
     * Validate the current T&L module.
     *
     * This is called directly after UpdateState() when a state change that has
     * occurred matches the dd_function_table::NeedValidate bitmask above.  This
     * ensures all computed values are up to date, thus allowing the driver to
     * decide if the current T&L module needs to be swapped out.
     *
     * This must be non-nullptr if a driver installs a custom T&L module and sets
     * the dd_function_table::NeedValidate bitmask, but may be nullptr otherwise.
     */
    std::function<void(GLcontext *ctx, GLuint new_state)> ValidateTnlModule;

    /**
     * Set by the driver-supplied T&L engine.
     *
     * Set to PRIM_OUTSIDE_BEGIN_END when outside glBegin()/glEnd().
     */
    GLuint CurrentExecPrimitive = 0;

    /**
     * Current state of an in-progress compilation.
     *
     * May take on any of the additional values PRIM_OUTSIDE_BEGIN_END,
     * PRIM_INSIDE_UNKNOWN_PRIM or PRIM_UNKNOWN defined above.
     */
    GLuint CurrentSavePrimitive = 0;

    /**
     * Set by the driver-supplied T&L engine whenever vertices are buffered
     * between glBegin()/glEnd() objects or __GLcontextRec::Current is not
     * updated.
     *
     * The dd_function_table::FlushVertices call below may be used to resolve
     * these conditions.
     */
    GLuint NeedFlush = 0;
    GLuint SaveNeedFlush = 0;

    /**
     * If inside glBegin()/glEnd(), it should assert(0).  Otherwise, if
     * FLUSH_STORED_VERTICES bit in \p flags is set flushes any buffered
     * vertices, if FLUSH_UPDATE_CURRENT bit is set updates
     * __GLcontextRec::Current and gl_light_attrib::Material
     *
     * Note that the default T&L engine never clears the
     * FLUSH_UPDATE_CURRENT bit, even after performing the update.
     */
    std::function<void(GLcontext *ctx, GLuint flags)> FlushVertices;
    std::function<void(GLcontext *ctx)> SaveFlushVertices;

    /**
     * Give the driver the opportunity to hook in its own vtxfmt for
     * compiling optimized display lists.  This is called on each valid
     * glBegin() during list compilation.
     */
    std::function<bool(GLcontext *ctx, GLenum mode)> NotifySaveBegin;

    /**
     * Notify driver that the special derived value _NeedEyeCoords has
     * changed.
     */
    std::function<void(GLcontext *ctx)> LightingSpaceChange;

    /**
     * Called by glNewList().
     *
     * Let the T&L component know what is going on with display lists
     * in time to make changes to dispatch tables, etc.
     */
    std::function<void(GLcontext *ctx, GLuint list, GLenum mode)> NewList;
    /**
     * Called by glEndList().
     *
     * \sa dd_function_table::NewList.
     */
    std::function<void(GLcontext *ctx)> EndList;

    /**
     * Called by glCallList(s).
     *
     * Notify the T&L component before and after calling a display list.
     */
    std::function<void(GLcontext *ctx, struct mesa_display_list *dlist)> BeginCallList;
    /**
     * Called by glEndCallList().
     *
     * \sa dd_function_table::BeginCallList.
     */
    std::function<void(GLcontext *ctx)> EndCallList;

};


/**
 * Transform/Clip/Lighting interface
 *
 * Drivers present a reduced set of the functions possible in
 * glBegin()/glEnd() objects.  Core mesa provides translation stubs for the
 * remaining functions to map down to these entry points.
 *
 * These are the initial values to be installed into dispatch by
 * mesa.  If the T&L driver wants to modify the dispatch table
 * while installed, it must do so itself.  It would be possible for
 * the vertexformat to install it's own initial values for these
 * functions, but this way there is an obvious list of what is
 * expected of the driver.
 *
 * If the driver wants to hook in entry points other than those
 * listed, it must restore them to their original values in
 * the disable() callback, below.
 */
struct GLvertexformat {
    /**
     * \name Vertex
     */
    /*@{*/
    void (GLAPIENTRYP ArrayElement)(GLint);   /* NOTE */
    void (GLAPIENTRYP Color3f)(GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP Color3fv)(const GLfloat *);
    void (GLAPIENTRYP Color4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP Color4fv)(const GLfloat *);
    void (GLAPIENTRYP EdgeFlag)(GLboolean);
    void (GLAPIENTRYP EvalCoord1f)(GLfloat);            /* NOTE */
    void (GLAPIENTRYP EvalCoord1fv)(const GLfloat *);   /* NOTE */
    void (GLAPIENTRYP EvalCoord2f)(GLfloat, GLfloat);   /* NOTE */
    void (GLAPIENTRYP EvalCoord2fv)(const GLfloat *);   /* NOTE */
    void (GLAPIENTRYP EvalPoint1)(GLint);               /* NOTE */
    void (GLAPIENTRYP EvalPoint2)(GLint, GLint);        /* NOTE */
    void (GLAPIENTRYP FogCoordfEXT)(GLfloat);
    void (GLAPIENTRYP FogCoordfvEXT)(const GLfloat *);
    void (GLAPIENTRYP Indexf)(GLfloat);
    void (GLAPIENTRYP Indexfv)(const GLfloat *);
    void (GLAPIENTRYP Materialfv)(GLenum face, GLenum pname, const GLfloat *);   /* NOTE */
    void (GLAPIENTRYP MultiTexCoord1fARB)(GLenum, GLfloat);
    void (GLAPIENTRYP MultiTexCoord1fvARB)(GLenum, const GLfloat *);
    void (GLAPIENTRYP MultiTexCoord2fARB)(GLenum, GLfloat, GLfloat);
    void (GLAPIENTRYP MultiTexCoord2fvARB)(GLenum, const GLfloat *);
    void (GLAPIENTRYP MultiTexCoord3fARB)(GLenum, GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP MultiTexCoord3fvARB)(GLenum, const GLfloat *);
    void (GLAPIENTRYP MultiTexCoord4fARB)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP MultiTexCoord4fvARB)(GLenum, const GLfloat *);
    void (GLAPIENTRYP Normal3f)(GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP Normal3fv)(const GLfloat *);
    void (GLAPIENTRYP SecondaryColor3fEXT)(GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP SecondaryColor3fvEXT)(const GLfloat *);
    void (GLAPIENTRYP TexCoord1f)(GLfloat);
    void (GLAPIENTRYP TexCoord1fv)(const GLfloat *);
    void (GLAPIENTRYP TexCoord2f)(GLfloat, GLfloat);
    void (GLAPIENTRYP TexCoord2fv)(const GLfloat *);
    void (GLAPIENTRYP TexCoord3f)(GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP TexCoord3fv)(const GLfloat *);
    void (GLAPIENTRYP TexCoord4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP TexCoord4fv)(const GLfloat *);
    void (GLAPIENTRYP Vertex2f)(GLfloat, GLfloat);
    void (GLAPIENTRYP Vertex2fv)(const GLfloat *);
    void (GLAPIENTRYP Vertex3f)(GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP Vertex3fv)(const GLfloat *);
    void (GLAPIENTRYP Vertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (GLAPIENTRYP Vertex4fv)(const GLfloat *);
    void (GLAPIENTRYP CallList)(GLuint);	/* NOTE */
    void (GLAPIENTRYP CallLists)(GLsizei, GLenum, const GLvoid *);	/* NOTE */
    void (GLAPIENTRYP Begin)(GLenum);
    void (GLAPIENTRYP End)(void);
    /* GL_NV_vertex_program */
    void (GLAPIENTRYP VertexAttrib1fNV)(GLuint index, GLfloat x);
    void (GLAPIENTRYP VertexAttrib1fvNV)(GLuint index, const GLfloat *v);
    void (GLAPIENTRYP VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y);
    void (GLAPIENTRYP VertexAttrib2fvNV)(GLuint index, const GLfloat *v);
    void (GLAPIENTRYP VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
    void (GLAPIENTRYP VertexAttrib3fvNV)(GLuint index, const GLfloat *v);
    void (GLAPIENTRYP VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (GLAPIENTRYP VertexAttrib4fvNV)(GLuint index, const GLfloat *v);
#if FEATURE_ARB_vertex_program
    void (GLAPIENTRYP VertexAttrib1fARB)(GLuint index, GLfloat x);
    void (GLAPIENTRYP VertexAttrib1fvARB)(GLuint index, const GLfloat *v);
    void (GLAPIENTRYP VertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y);
    void (GLAPIENTRYP VertexAttrib2fvARB)(GLuint index, const GLfloat *v);
    void (GLAPIENTRYP VertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
    void (GLAPIENTRYP VertexAttrib3fvARB)(GLuint index, const GLfloat *v);
    void (GLAPIENTRYP VertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (GLAPIENTRYP VertexAttrib4fvARB)(GLuint index, const GLfloat *v);
#endif
    /*@}*/

    /*
     */
    void (GLAPIENTRYP Rectf)(GLfloat, GLfloat, GLfloat, GLfloat);

    /**
     * \name Array
     */
    /*@{*/
    void (GLAPIENTRYP DrawArrays)(GLenum mode, GLint start, GLsizei count);
    void (GLAPIENTRYP DrawElements)(GLenum mode, GLsizei count, GLenum type,
				    const GLvoid *indices);
    void (GLAPIENTRYP DrawRangeElements)(GLenum mode, GLuint start,
					 GLuint end, GLsizei count,
					 GLenum type, const GLvoid *indices);
    /*@}*/

    /**
     * \name Eval
     *
     * If you don't support eval, fallback to the default vertex format
     * on receiving an eval call and use the pipeline mechanism to
     * provide partial T&L acceleration.
     *
     * Mesa will provide a set of helper functions to do eval within
     * accelerated vertex formats, eventually...
     */
    /*@{*/
    void (GLAPIENTRYP EvalMesh1)(GLenum mode, GLint i1, GLint i2);
    void (GLAPIENTRYP EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
    /*@}*/

};


#endif /* DD_INCLUDED */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
