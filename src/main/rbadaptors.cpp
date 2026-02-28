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


/**
 * Renderbuffer adaptors.
 * These functions are used to convert rendering from core Mesa's GLchan
 * colors to 8 or 16-bit color channels in RGBA renderbuffers.
 * This means Mesa can be compiled for 16 or 32-bit color processing
 * and still render into 8 and 16-bit/channel renderbuffers.
 */


#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "renderbuffer.h"
#include "rbadaptors.h"

#include <mutex>


/**
 * Base class for renderbuffer adaptors that wrap another renderbuffer.
 * Handles the common AllocStorage and destructor logic.
 */
struct WrappedRenderbuffer : public gl_renderbuffer {
    explicit WrappedRenderbuffer(GLuint name = 0) : gl_renderbuffer(name) {}
    ~WrappedRenderbuffer() override {
_mesa_reference_renderbuffer(&Wrapped, nullptr);
    }

    GLboolean AllocStorage(GLcontext *ctx, GLenum internalFormat,
   GLuint width, GLuint height) override {
GLboolean b = Wrapped->AllocStorage(ctx, internalFormat, width, height);
if (b) {
    Width  = width;
    Height = height;
}
return b;
    }

    void *GetPointer(GLcontext *, GLint, GLint) override { return nullptr; }
};


/*======================================================================
 * 16-bit-per-channel adaptor wrapping an 8-bit-per-channel buffer
 */

struct RB16Wrap8 : public WrappedRenderbuffer {
    explicit RB16Wrap8(GLuint name = 0) : WrappedRenderbuffer(name) {}

    void GetRow(GLcontext *ctx, GLuint count,
GLint x, GLint y, void *values) override {
GLubyte values8[MAX_WIDTH * 4];
GLushort *values16 = static_cast<GLushort *>(values);
GLuint i;
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
assert(count <= MAX_WIDTH);
Wrapped->GetRow(ctx, count, x, y, values8);
for (i = 0; i < 4 * count; i++) {
    values16[i] = (values8[i] << 8) | values8[i];
}
    }

    void GetValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[], void *values) override {
GLubyte values8[MAX_WIDTH * 4];
GLushort *values16 = static_cast<GLushort *>(values);
GLuint i;
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
Wrapped->GetValues(ctx, count, x, y, values8);
for (i = 0; i < 4 * count; i++) {
    values16[i] = (values8[i] << 8) | values8[i];
}
    }

    void PutRow(GLcontext *ctx, GLuint count,
GLint x, GLint y,
const void *values, const GLubyte *mask) override {
GLubyte values8[MAX_WIDTH * 4];
const GLushort *values16 = static_cast<const GLushort *>(values);
GLuint i;
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
for (i = 0; i < 4 * count; i++) {
    values8[i] = values16[i] >> 8;
}
Wrapped->PutRow(ctx, count, x, y, values8, mask);
    }

    void PutRowRGB(GLcontext *ctx, GLuint count,
   GLint x, GLint y,
   const void *values, const GLubyte *mask) override {
GLubyte values8[MAX_WIDTH * 3];
const GLushort *values16 = static_cast<const GLushort *>(values);
GLuint i;
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
for (i = 0; i < 3 * count; i++) {
    values8[i] = values16[i] >> 8;
}
Wrapped->PutRowRGB(ctx, count, x, y, values8, mask);
    }

    void PutMonoRow(GLcontext *ctx, GLuint count,
    GLint x, GLint y,
    const void *value, const GLubyte *mask) override {
GLubyte value8[4];
const GLushort *value16 = static_cast<const GLushort *>(value);
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
value8[0] = value16[0] >> 8;
value8[1] = value16[1] >> 8;
value8[2] = value16[2] >> 8;
value8[3] = value16[3] >> 8;
Wrapped->PutMonoRow(ctx, count, x, y, value8, mask);
    }

    void PutValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[],
   const void *values, const GLubyte *mask) override {
GLubyte values8[MAX_WIDTH * 4];
const GLushort *values16 = static_cast<const GLushort *>(values);
GLuint i;
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
for (i = 0; i < 4 * count; i++) {
    values8[i] = values16[i] >> 8;
}
Wrapped->PutValues(ctx, count, x, y, values8, mask);
    }

    void PutMonoValues(GLcontext *ctx, GLuint count,
       const GLint x[], const GLint y[],
       const void *value, const GLubyte *mask) override {
GLubyte value8[4];
const GLushort *value16 = static_cast<const GLushort *>(value);
assert(DataType == GL_UNSIGNED_SHORT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
value8[0] = value16[0] >> 8;
value8[1] = value16[1] >> 8;
value8[2] = value16[2] >> 8;
value8[3] = value16[3] >> 8;
Wrapped->PutMonoValues(ctx, count, x, y, value8, mask);
    }
};


/**
 * Wrap an 8-bit/channel renderbuffer with a 16-bit/channel
 * renderbuffer adaptor.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb8)
{
    assert(rb8->DataType == GL_UNSIGNED_BYTE);
    assert(rb8->_BaseFormat == GL_RGBA);

    auto *rb16 = new RB16Wrap8{rb8->Name};

    {
std::lock_guard<std::mutex> lock(rb8->Mutex);
rb8->RefCount++;
    }

    rb16->InternalFormat = rb8->InternalFormat;
    rb16->_ActualFormat  = rb8->_ActualFormat;
    rb16->_BaseFormat    = rb8->_BaseFormat;
    rb16->DataType       = GL_UNSIGNED_SHORT;
    rb16->RedBits        = rb8->RedBits;
    rb16->GreenBits      = rb8->GreenBits;
    rb16->BlueBits       = rb8->BlueBits;
    rb16->AlphaBits      = rb8->AlphaBits;
    rb16->Wrapped        = rb8;

    return rb16;
}


/*======================================================================
 * 32-bit-per-channel adaptor wrapping an 8-bit-per-channel buffer
 */

struct RB32Wrap8 : public WrappedRenderbuffer {
    explicit RB32Wrap8(GLuint name = 0) : WrappedRenderbuffer(name) {}

    void GetRow(GLcontext *ctx, GLuint count,
GLint x, GLint y, void *values) override {
GLubyte values8[MAX_WIDTH * 4];
GLfloat *values32 = static_cast<GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
assert(count <= MAX_WIDTH);
Wrapped->GetRow(ctx, count, x, y, values8);
for (i = 0; i < 4 * count; i++) {
    values32[i] = UBYTE_TO_FLOAT(values8[i]);
}
    }

    void GetValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[], void *values) override {
GLubyte values8[MAX_WIDTH * 4];
GLfloat *values32 = static_cast<GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
Wrapped->GetValues(ctx, count, x, y, values8);
for (i = 0; i < 4 * count; i++) {
    values32[i] = UBYTE_TO_FLOAT(values8[i]);
}
    }

    void PutRow(GLcontext *ctx, GLuint count,
GLint x, GLint y,
const void *values, const GLubyte *mask) override {
GLubyte values8[MAX_WIDTH * 4];
const GLfloat *values32 = static_cast<const GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
for (i = 0; i < 4 * count; i++) {
    UNCLAMPED_FLOAT_TO_UBYTE(values8[i], values32[i]);
}
Wrapped->PutRow(ctx, count, x, y, values8, mask);
    }

    void PutRowRGB(GLcontext *ctx, GLuint count,
   GLint x, GLint y,
   const void *values, const GLubyte *mask) override {
GLubyte values8[MAX_WIDTH * 3];
const GLfloat *values32 = static_cast<const GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
for (i = 0; i < 3 * count; i++) {
    UNCLAMPED_FLOAT_TO_UBYTE(values8[i], values32[i]);
}
Wrapped->PutRowRGB(ctx, count, x, y, values8, mask);
    }

    void PutMonoRow(GLcontext *ctx, GLuint count,
    GLint x, GLint y,
    const void *value, const GLubyte *mask) override {
GLubyte value8[4];
const GLfloat *value32 = static_cast<const GLfloat *>(value);
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
UNCLAMPED_FLOAT_TO_UBYTE(value8[0], value32[0]);
UNCLAMPED_FLOAT_TO_UBYTE(value8[1], value32[1]);
UNCLAMPED_FLOAT_TO_UBYTE(value8[2], value32[2]);
UNCLAMPED_FLOAT_TO_UBYTE(value8[3], value32[3]);
Wrapped->PutMonoRow(ctx, count, x, y, value8, mask);
    }

    void PutValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[],
   const void *values, const GLubyte *mask) override {
GLubyte values8[MAX_WIDTH * 4];
const GLfloat *values32 = static_cast<const GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
for (i = 0; i < 4 * count; i++) {
    UNCLAMPED_FLOAT_TO_UBYTE(values8[i], values32[i]);
}
Wrapped->PutValues(ctx, count, x, y, values8, mask);
    }

    void PutMonoValues(GLcontext *ctx, GLuint count,
       const GLint x[], const GLint y[],
       const void *value, const GLubyte *mask) override {
GLubyte value8[4];
const GLfloat *value32 = static_cast<const GLfloat *>(value);
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_BYTE);
UNCLAMPED_FLOAT_TO_UBYTE(value8[0], value32[0]);
UNCLAMPED_FLOAT_TO_UBYTE(value8[1], value32[1]);
UNCLAMPED_FLOAT_TO_UBYTE(value8[2], value32[2]);
UNCLAMPED_FLOAT_TO_UBYTE(value8[3], value32[3]);
Wrapped->PutMonoValues(ctx, count, x, y, value8, mask);
    }
};


/**
 * Wrap an 8-bit/channel renderbuffer with a 32-bit/channel
 * renderbuffer adaptor.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb8)
{
    assert(rb8->DataType == GL_UNSIGNED_BYTE);
    assert(rb8->_BaseFormat == GL_RGBA);

    auto *rb32 = new RB32Wrap8{rb8->Name};

    {
std::lock_guard<std::mutex> lock(rb8->Mutex);
rb8->RefCount++;
    }

    rb32->InternalFormat = rb8->InternalFormat;
    rb32->_ActualFormat  = rb8->_ActualFormat;
    rb32->_BaseFormat    = rb8->_BaseFormat;
    rb32->DataType       = GL_FLOAT;
    rb32->RedBits        = rb8->RedBits;
    rb32->GreenBits      = rb8->GreenBits;
    rb32->BlueBits       = rb8->BlueBits;
    rb32->AlphaBits      = rb8->AlphaBits;
    rb32->Wrapped        = rb8;

    return rb32;
}


/*======================================================================
 * 32-bit-per-channel adaptor wrapping a 16-bit-per-channel buffer
 */

struct RB32Wrap16 : public WrappedRenderbuffer {
    explicit RB32Wrap16(GLuint name = 0) : WrappedRenderbuffer(name) {}

    void GetRow(GLcontext *ctx, GLuint count,
GLint x, GLint y, void *values) override {
GLushort values16[MAX_WIDTH * 4];
GLfloat *values32 = static_cast<GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
assert(count <= MAX_WIDTH);
Wrapped->GetRow(ctx, count, x, y, values16);
for (i = 0; i < 4 * count; i++) {
    values32[i] = mesa_ushort_to_float(values16[i]);
}
    }

    void GetValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[], void *values) override {
GLushort values16[MAX_WIDTH * 4];
GLfloat *values32 = static_cast<GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
Wrapped->GetValues(ctx, count, x, y, values16);
for (i = 0; i < 4 * count; i++) {
    values32[i] = mesa_ushort_to_float(values16[i]);
}
    }

    void PutRow(GLcontext *ctx, GLuint count,
GLint x, GLint y,
const void *values, const GLubyte *mask) override {
GLushort values16[MAX_WIDTH * 4];
const GLfloat *values32 = static_cast<const GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
for (i = 0; i < 4 * count; i++) {
    UNCLAMPED_FLOAT_TO_USHORT(values16[i], values32[i]);
}
Wrapped->PutRow(ctx, count, x, y, values16, mask);
    }

    void PutRowRGB(GLcontext *ctx, GLuint count,
   GLint x, GLint y,
   const void *values, const GLubyte *mask) override {
GLushort values16[MAX_WIDTH * 3];
const GLfloat *values32 = static_cast<const GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
for (i = 0; i < 3 * count; i++) {
    UNCLAMPED_FLOAT_TO_USHORT(values16[i], values32[i]);
}
Wrapped->PutRowRGB(ctx, count, x, y, values16, mask);
    }

    void PutMonoRow(GLcontext *ctx, GLuint count,
    GLint x, GLint y,
    const void *value, const GLubyte *mask) override {
GLushort value16[4];
const GLfloat *value32 = static_cast<const GLfloat *>(value);
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
UNCLAMPED_FLOAT_TO_USHORT(value16[0], value32[0]);
UNCLAMPED_FLOAT_TO_USHORT(value16[1], value32[1]);
UNCLAMPED_FLOAT_TO_USHORT(value16[2], value32[2]);
UNCLAMPED_FLOAT_TO_USHORT(value16[3], value32[3]);
Wrapped->PutMonoRow(ctx, count, x, y, value16, mask);
    }

    void PutValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[],
   const void *values, const GLubyte *mask) override {
GLushort values16[MAX_WIDTH * 4];
const GLfloat *values32 = static_cast<const GLfloat *>(values);
GLuint i;
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
for (i = 0; i < 4 * count; i++) {
    UNCLAMPED_FLOAT_TO_USHORT(values16[i], values32[i]);
}
Wrapped->PutValues(ctx, count, x, y, values16, mask);
    }

    void PutMonoValues(GLcontext *ctx, GLuint count,
       const GLint x[], const GLint y[],
       const void *value, const GLubyte *mask) override {
GLushort value16[4];
const GLfloat *value32 = static_cast<const GLfloat *>(value);
assert(DataType == GL_FLOAT);
assert(Wrapped->DataType == GL_UNSIGNED_SHORT);
UNCLAMPED_FLOAT_TO_USHORT(value16[0], value32[0]);
UNCLAMPED_FLOAT_TO_USHORT(value16[1], value32[1]);
UNCLAMPED_FLOAT_TO_USHORT(value16[2], value32[2]);
UNCLAMPED_FLOAT_TO_USHORT(value16[3], value32[3]);
Wrapped->PutMonoValues(ctx, count, x, y, value16, mask);
    }
};


/**
 * Wrap a 16-bit/channel renderbuffer with a 32-bit/channel
 * renderbuffer adaptor.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb16)
{
    assert(rb16->DataType == GL_UNSIGNED_SHORT);
    assert(rb16->_BaseFormat == GL_RGBA);

    auto *rb32 = new RB32Wrap16{rb16->Name};

    {
std::lock_guard<std::mutex> lock(rb16->Mutex);
rb16->RefCount++;
    }

    rb32->InternalFormat = rb16->InternalFormat;
    rb32->_ActualFormat  = rb16->_ActualFormat;
    rb32->_BaseFormat    = rb16->_BaseFormat;
    rb32->DataType       = GL_FLOAT;
    rb32->RedBits        = rb16->RedBits;
    rb32->GreenBits      = rb16->GreenBits;
    rb32->BlueBits       = rb16->BlueBits;
    rb32->AlphaBits      = rb16->AlphaBits;
    rb32->Wrapped        = rb16;

    return rb32;
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
