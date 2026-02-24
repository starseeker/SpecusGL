/*
 * Mesa 3-D graphics library
 * Version:  6.5
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

#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "fbobject.h"
#include "mtypes.h"
#include "depthstencil.h"
#include "renderbuffer.h"


/**
 * Adaptor/wrappers for GL_DEPTH_STENCIL renderbuffers.
 *
 * The problem with a GL_DEPTH_STENCIL renderbuffer is that sometimes we
 * want to treat it as a stencil buffer, other times we want to treat it
 * as a depth/z buffer and still other times when we want to treat it as
 * a combined Z+stencil buffer!  That implies we need three different sets
 * of Get/Put functions.
 *
 * We solve this by wrapping the Z24_S8 renderbuffer with depth and stencil
 * adaptors, each with the right kind of depth/stencil Get/Put functions.
 */


/**
 * Base class for depth/stencil wrapper renderbuffers.
 * Wraps a GL_DEPTH24_STENCIL8 renderbuffer (Wrapped) and exposes either
 * depth-only or stencil-only pixel access.
 */
struct DepthStencilWrapper : public gl_renderbuffer {
    DepthStencilWrapper() = default;
    ~DepthStencilWrapper() override {
/* Decrement reference count on the wrapped buffer and delete if zero. */
ASSERT(Wrapped);
Wrapped->RefCount--;
if (Wrapped->RefCount <= 0)
    delete Wrapped;
    }

    GLboolean AllocStorage(GLcontext *ctx, GLenum internalFormat,
   GLuint width, GLuint height) override {
/* pass through to the wrapped depth/stencil buffer */
gl_renderbuffer *dsrb = Wrapped;
GLboolean retVal;
(void) internalFormat;
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
retVal = dsrb->AllocStorage(ctx, dsrb->InternalFormat, width, height);
if (retVal) {
    Width  = width;
    Height = height;
}
return retVal;
    }

    /* GetPointer always returns nullptr for wrappers */
    void *GetPointer(GLcontext *, GLint, GLint) override { return nullptr; }
};


/*======================================================================
 * Depth wrapper around depth/stencil renderbuffer
 */

struct Z24RenderbufferWrapper : public DepthStencilWrapper {
    Z24RenderbufferWrapper() = default;

    void GetRow(GLcontext *ctx, GLuint count,
GLint x, GLint y, void *values) override {
gl_renderbuffer *dsrb = Wrapped;
GLuint temp[MAX_WIDTH], i;
GLuint *dst = (GLuint *) values;
const GLuint *src = (const GLuint *) dsrb->GetPointer(ctx, x, y);
ASSERT(DataType == GL_UNSIGNED_INT);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (!src) {
    dsrb->GetRow(ctx, count, x, y, temp);
    src = temp;
}
for (i = 0; i < count; i++) {
    dst[i] = src[i] >> 8;
}
    }

    void GetValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[], void *values) override {
gl_renderbuffer *dsrb = Wrapped;
GLuint temp[MAX_WIDTH], i;
GLuint *dst = (GLuint *) values;
ASSERT(DataType == GL_UNSIGNED_INT);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
ASSERT(count <= MAX_WIDTH);
dsrb->GetValues(ctx, count, x, y, temp);
for (i = 0; i < count; i++) {
    dst[i] = temp[i] >> 8;
}
    }

    void PutRow(GLcontext *ctx, GLuint count,
GLint x, GLint y,
const void *values, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
const GLuint *src = (const GLuint *) values;
GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, x, y);
ASSERT(DataType == GL_UNSIGNED_INT);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (dst) {
    GLuint i;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    dst[i] = (src[i] << 8) | (dst[i] & 0xff);
}
    }
} else {
    GLuint temp[MAX_WIDTH], i;
    dsrb->GetRow(ctx, count, x, y, temp);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    temp[i] = (src[i] << 8) | (temp[i] & 0xff);
}
    }
    dsrb->PutRow(ctx, count, x, y, temp, mask);
}
    }

    void PutMonoRow(GLcontext *ctx, GLuint count,
    GLint x, GLint y,
    const void *value, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
const GLuint shiftedVal = *((GLuint *) value) << 8;
GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, x, y);
ASSERT(DataType == GL_UNSIGNED_INT);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (dst) {
    GLuint i;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    dst[i] = shiftedVal | (dst[i] & 0xff);
}
    }
} else {
    GLuint temp[MAX_WIDTH], i;
    dsrb->GetRow(ctx, count, x, y, temp);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    temp[i] = shiftedVal | (temp[i] & 0xff);
}
    }
    dsrb->PutRow(ctx, count, x, y, temp, mask);
}
    }

    void PutValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[],
   const void *values, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
const GLuint *src = (const GLuint *) values;
ASSERT(DataType == GL_UNSIGNED_INT);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (dsrb->GetPointer(ctx, 0, 0)) {
    GLuint i;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, x[i], y[i]);
    *dst = (src[i] << 8) | (*dst & 0xff);
}
    }
} else {
    GLuint temp[MAX_WIDTH], i;
    dsrb->GetValues(ctx, count, x, y, temp);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    temp[i] = (src[i] << 8) | (temp[i] & 0xff);
}
    }
    dsrb->PutValues(ctx, count, x, y, temp, mask);
}
    }

    void PutMonoValues(GLcontext *ctx, GLuint count,
       const GLint x[], const GLint y[],
       const void *value, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
GLuint temp[MAX_WIDTH], i;
const GLuint shiftedVal = *((GLuint *) value) << 8;
dsrb->GetValues(ctx, count, x, y, temp);
for (i = 0; i < count; i++) {
    if (!mask || mask[i]) {
temp[i] = shiftedVal | (temp[i] & 0xff);
    }
}
dsrb->PutValues(ctx, count, x, y, temp, mask);
    }
};


/**
 * Wrap the given GL_DEPTH_STENCIL renderbuffer so that it acts like
 * a depth renderbuffer.
 * \return new depth renderbuffer
 */
struct gl_renderbuffer *
_mesa_new_z24_renderbuffer_wrapper(GLcontext *ctx,
   struct gl_renderbuffer *dsrb)
{
    ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
    ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);

    auto *z24rb = new Z24RenderbufferWrapper{};
    _mesa_init_renderbuffer(z24rb, 0);

    z24rb->Wrapped = dsrb;
    z24rb->Name = dsrb->Name;
    z24rb->RefCount = 1;
    z24rb->Width = dsrb->Width;
    z24rb->Height = dsrb->Height;
    z24rb->InternalFormat = GL_DEPTH_COMPONENT24_ARB;
    z24rb->_ActualFormat = GL_DEPTH_COMPONENT24_ARB;
    z24rb->_BaseFormat = GL_DEPTH_COMPONENT;
    z24rb->DataType = GL_UNSIGNED_INT;
    z24rb->DepthBits = 24;
    z24rb->Data = NULL;

    return z24rb;
}


/*======================================================================
 * Stencil wrapper around depth/stencil renderbuffer
 */

struct S8RenderbufferWrapper : public DepthStencilWrapper {
    S8RenderbufferWrapper() = default;

    void GetRow(GLcontext *ctx, GLuint count,
GLint x, GLint y, void *values) override {
gl_renderbuffer *dsrb = Wrapped;
GLuint temp[MAX_WIDTH], i;
GLubyte *dst = (GLubyte *) values;
const GLuint *src = (const GLuint *) dsrb->GetPointer(ctx, x, y);
ASSERT(DataType == GL_UNSIGNED_BYTE);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (!src) {
    dsrb->GetRow(ctx, count, x, y, temp);
    src = temp;
}
for (i = 0; i < count; i++) {
    dst[i] = src[i] & 0xff;
}
    }

    void GetValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[], void *values) override {
gl_renderbuffer *dsrb = Wrapped;
GLuint temp[MAX_WIDTH], i;
GLubyte *dst = (GLubyte *) values;
ASSERT(DataType == GL_UNSIGNED_BYTE);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
ASSERT(count <= MAX_WIDTH);
dsrb->GetValues(ctx, count, x, y, temp);
for (i = 0; i < count; i++) {
    dst[i] = temp[i] & 0xff;
}
    }

    void PutRow(GLcontext *ctx, GLuint count,
GLint x, GLint y,
const void *values, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
const GLubyte *src = (const GLubyte *) values;
GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, x, y);
ASSERT(DataType == GL_UNSIGNED_BYTE);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (dst) {
    GLuint i;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    dst[i] = (dst[i] & 0xffffff00) | src[i];
}
    }
} else {
    GLuint temp[MAX_WIDTH], i;
    dsrb->GetRow(ctx, count, x, y, temp);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    temp[i] = (temp[i] & 0xffffff00) | src[i];
}
    }
    dsrb->PutRow(ctx, count, x, y, temp, mask);
}
    }

    void PutMonoRow(GLcontext *ctx, GLuint count,
    GLint x, GLint y,
    const void *value, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
const GLubyte val = *((GLubyte *) value);
GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, x, y);
ASSERT(DataType == GL_UNSIGNED_BYTE);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (dst) {
    GLuint i;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    dst[i] = (dst[i] & 0xffffff00) | val;
}
    }
} else {
    GLuint temp[MAX_WIDTH], i;
    dsrb->GetRow(ctx, count, x, y, temp);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    temp[i] = (temp[i] & 0xffffff00) | val;
}
    }
    dsrb->PutRow(ctx, count, x, y, temp, mask);
}
    }

    void PutValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[],
   const void *values, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
const GLubyte *src = (const GLubyte *) values;
ASSERT(DataType == GL_UNSIGNED_BYTE);
ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
if (dsrb->GetPointer(ctx, 0, 0)) {
    GLuint i;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, x[i], y[i]);
    *dst = (*dst & 0xffffff00) | src[i];
}
    }
} else {
    GLuint temp[MAX_WIDTH], i;
    dsrb->GetValues(ctx, count, x, y, temp);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    temp[i] = (temp[i] & 0xffffff00) | src[i];
}
    }
    dsrb->PutValues(ctx, count, x, y, temp, mask);
}
    }

    void PutMonoValues(GLcontext *ctx, GLuint count,
       const GLint x[], const GLint y[],
       const void *value, const GLubyte *mask) override {
gl_renderbuffer *dsrb = Wrapped;
GLuint temp[MAX_WIDTH], i;
const GLubyte val = *((GLubyte *) value);
dsrb->GetValues(ctx, count, x, y, temp);
for (i = 0; i < count; i++) {
    if (!mask || mask[i]) {
temp[i] = (temp[i] & 0xffffff00) | val;
    }
}
dsrb->PutValues(ctx, count, x, y, temp, mask);
    }
};


/**
 * Wrap the given GL_DEPTH_STENCIL renderbuffer so that it acts like
 * a stencil renderbuffer.
 * \return new stencil renderbuffer
 */
struct gl_renderbuffer *
_mesa_new_s8_renderbuffer_wrapper(GLcontext *ctx, struct gl_renderbuffer *dsrb)
{
    ASSERT(dsrb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
    ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);

    auto *s8rb = new S8RenderbufferWrapper{};
    _mesa_init_renderbuffer(s8rb, 0);

    s8rb->Wrapped = dsrb;
    s8rb->Name = dsrb->Name;
    s8rb->RefCount = 1;
    s8rb->Width = dsrb->Width;
    s8rb->Height = dsrb->Height;
    s8rb->InternalFormat = GL_STENCIL_INDEX8_EXT;
    s8rb->_ActualFormat = GL_STENCIL_INDEX8_EXT;
    s8rb->_BaseFormat = GL_STENCIL_INDEX;
    s8rb->DataType = GL_UNSIGNED_BYTE;
    s8rb->StencilBits = 8;
    s8rb->Data = NULL;

    return s8rb;
}



/**
 ** The following functions are useful for hardware drivers that only
 ** implement combined depth/stencil buffers.
 ** The GL_EXT_framebuffer_object extension allows indepedent depth and
 ** stencil buffers to be used in any combination.
 ** Therefore, we sometimes have to merge separate depth and stencil
 ** renderbuffers into a single depth+stencil renderbuffer.  And sometimes
 ** we have to split combined depth+stencil renderbuffers into separate
 ** renderbuffers.
 **/


/**
 * Extract stencil values from the combined depth/stencil renderbuffer, storing
 * the values into a separate stencil renderbuffer.
 * \param dsRb  the source depth/stencil renderbuffer
 * \param stencilRb  the destination stencil renderbuffer
 *                   (either 8-bit or 32-bit)
 */
void
_mesa_extract_stencil(GLcontext *ctx,
      struct gl_renderbuffer *dsRb,
      struct gl_renderbuffer *stencilRb)
{
    GLuint row, width, height;

    ASSERT(dsRb);
    ASSERT(stencilRb);

    ASSERT(dsRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
    ASSERT(dsRb->DataType == GL_UNSIGNED_INT_24_8_EXT);
    ASSERT(stencilRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT ||
   stencilRb->_ActualFormat == GL_STENCIL_INDEX8_EXT);
    ASSERT(dsRb->Width == stencilRb->Width);
    ASSERT(dsRb->Height == stencilRb->Height);

    width = dsRb->Width;
    height = dsRb->Height;

    for (row = 0; row < height; row++) {
GLuint depthStencil[MAX_WIDTH];
dsRb->GetRow(ctx, width, 0, row, depthStencil);
if (stencilRb->_ActualFormat == GL_STENCIL_INDEX8_EXT) {
    /* 8bpp stencil */
    GLubyte stencil[MAX_WIDTH];
    GLuint i;
    for (i = 0; i < width; i++) {
stencil[i] = depthStencil[i] & 0xff;
    }
    stencilRb->PutRow(ctx, width, 0, row, stencil, NULL);
} else {
    /* 32bpp stencil */
    /* the 24 depth bits will be ignored */
    ASSERT(stencilRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
    ASSERT(stencilRb->DataType == GL_UNSIGNED_INT_24_8_EXT);
    stencilRb->PutRow(ctx, width, 0, row, depthStencil, NULL);
}
    }
}


/**
 * Copy stencil values from a stencil renderbuffer into a combined
 * depth/stencil renderbuffer.
 * \param dsRb  the destination depth/stencil renderbuffer
 * \param stencilRb  the source stencil buffer (either 8-bit or 32-bit)
 */
void
_mesa_insert_stencil(GLcontext *ctx,
     struct gl_renderbuffer *dsRb,
     struct gl_renderbuffer *stencilRb)
{
    GLuint row, width, height;

    ASSERT(dsRb);
    ASSERT(stencilRb);

    ASSERT(dsRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
    ASSERT(dsRb->DataType == GL_UNSIGNED_INT_24_8_EXT);
    ASSERT(stencilRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT ||
   stencilRb->_ActualFormat == GL_STENCIL_INDEX8_EXT);

    ASSERT(dsRb->Width == stencilRb->Width);
    ASSERT(dsRb->Height == stencilRb->Height);

    width = dsRb->Width;
    height = dsRb->Height;

    for (row = 0; row < height; row++) {
GLuint depthStencil[MAX_WIDTH];

dsRb->GetRow(ctx, width, 0, row, depthStencil);

if (stencilRb->_ActualFormat == GL_STENCIL_INDEX8_EXT) {
    /* 8bpp stencil */
    GLubyte stencil[MAX_WIDTH];
    GLuint i;
    stencilRb->GetRow(ctx, width, 0, row, stencil);
    for (i = 0; i < width; i++) {
depthStencil[i] = (depthStencil[i] & 0xffffff00) | stencil[i];
    }
} else {
    /* 32bpp stencil buffer */
    GLuint stencil[MAX_WIDTH], i;
    ASSERT(stencilRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT);
    ASSERT(stencilRb->DataType == GL_UNSIGNED_INT_24_8_EXT);
    stencilRb->GetRow(ctx, width, 0, row, stencil);
    for (i = 0; i < width; i++) {
depthStencil[i]
    = (depthStencil[i] & 0xffffff00) | (stencil[i] & 0xff);
    }
}

dsRb->PutRow(ctx, width, 0, row, depthStencil, NULL);
    }
}


/**
 * Convert the stencil buffer from 8bpp to 32bpp depth/stencil.
 * \param stencilRb  the stencil renderbuffer to promote
 */
void
_mesa_promote_stencil(GLcontext *ctx, struct gl_renderbuffer *stencilRb)
{
    const GLsizei width = stencilRb->Width;
    const GLsizei height = stencilRb->Height;
    GLubyte *data;
    GLint i, j, k;

    ASSERT(stencilRb->_ActualFormat == GL_STENCIL_INDEX8_EXT);
    ASSERT(stencilRb->Data);

    data = (GLubyte *) stencilRb->Data;
    stencilRb->Data = NULL;
    stencilRb->AllocStorage(ctx, GL_DEPTH24_STENCIL8_EXT, width, height);

    ASSERT(stencilRb->DataType == GL_UNSIGNED_INT_24_8_EXT);

    k = 0;
    for (i = 0; i < height; i++) {
GLuint depthStencil[MAX_WIDTH];
for (j = 0; j < width; j++) {
    depthStencil[j] = data[k++];
}
stencilRb->PutRow(ctx, width, 0, i, depthStencil, NULL);
    }
    free(data);

    stencilRb->_BaseFormat = GL_DEPTH_STENCIL_EXT;
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
