#include "context.h"
#include "fbobject.h"
#include "texformat.h"
#include "texrender.h"
#include "renderbuffer.h"


/*
 * Render-to-texture code for GL_EXT_framebuffer_object
 */


/**
 * Renderbuffer subclass that wraps a texture image, allowing rendering
 * directly into a texture.
 */
struct TextureRenderbuffer : public gl_renderbuffer {
    struct gl_texture_image *TexImage = nullptr;
    StoreTexelFunc Store = nullptr;
    GLint Zoffset = 0;

    TextureRenderbuffer() = default;
    ~TextureRenderbuffer() override = default;

    /* AllocStorage is not legal on a texture wrapper */
    GLboolean AllocStorage(GLcontext *ctx, GLenum, GLuint, GLuint) override {
_mesa_problem(ctx, "AllocStorage called on texture renderbuffer");
return GL_FALSE;
    }

    void GetRow(GLcontext *ctx, GLuint count,
GLint x, GLint y, void *values) override {
const GLint z = Zoffset;
GLuint i;
ASSERT(TexImage->Width == Width);
ASSERT(TexImage->Height == Height);
if (DataType == CHAN_TYPE) {
    GLchan *rgbaOut = (GLchan *) values;
    for (i = 0; i < count; i++) {
TexImage->FetchTexelc(TexImage, x + i, y, z, rgbaOut + 4 * i);
    }
} else if (DataType == GL_FLOAT) {
    GLfloat *rgbaOut = (GLfloat *) values;
    for (i = 0; i < count; i++) {
TexImage->FetchTexelf(TexImage, x + i, y, z, rgbaOut + 4 * i);
    }
} else if (DataType == GL_UNSIGNED_INT) {
    GLuint *zValues = (GLuint *) values;
    for (i = 0; i < count; i++) {
GLfloat flt;
TexImage->FetchTexelf(TexImage, x + i, y, z, &flt);
zValues[i] = ((GLuint)(flt * 0xffffff)) << 8;
    }
} else if (DataType == GL_UNSIGNED_INT_24_8_EXT) {
    GLuint *zValues = (GLuint *) values;
    for (i = 0; i < count; i++) {
GLfloat flt;
TexImage->FetchTexelf(TexImage, x + i, y, z, &flt);
zValues[i] = ((GLuint)(flt * 0xffffff)) << 8;
    }
} else {
    _mesa_problem(ctx, "invalid DataType in TextureRenderbuffer::GetRow");
}
    }

    void GetValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[], void *values) override {
const GLint z = Zoffset;
GLuint i;
if (DataType == CHAN_TYPE) {
    GLchan *rgbaOut = (GLchan *) values;
    for (i = 0; i < count; i++) {
TexImage->FetchTexelc(TexImage, x[i], y[i], z, rgbaOut + 4 * i);
    }
} else if (DataType == GL_FLOAT) {
    GLfloat *rgbaOut = (GLfloat *) values;
    for (i = 0; i < count; i++) {
TexImage->FetchTexelf(TexImage, x[i], y[i], z, rgbaOut + 4 * i);
    }
} else if (DataType == GL_UNSIGNED_INT) {
    GLuint *zValues = (GLuint *) values;
    for (i = 0; i < count; i++) {
GLfloat flt;
TexImage->FetchTexelf(TexImage, x[i], y[i], z, &flt);
zValues[i] = ((GLuint)(flt * 0xffffff)) << 8;
    }
} else if (DataType == GL_UNSIGNED_INT_24_8_EXT) {
    GLuint *zValues = (GLuint *) values;
    for (i = 0; i < count; i++) {
GLfloat flt;
TexImage->FetchTexelf(TexImage, x[i], y[i], z, &flt);
zValues[i] = ((GLuint)(flt * 0xffffff)) << 8;
    }
} else {
    _mesa_problem(ctx, "invalid DataType in TextureRenderbuffer::GetValues");
}
    }

    void PutRow(GLcontext *ctx, GLuint count,
GLint x, GLint y,
const void *values, const GLubyte *mask) override {
const GLint z = Zoffset;
GLuint i;
if (DataType == CHAN_TYPE) {
    const GLchan *rgba = (const GLchan *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, rgba);
}
rgba += 4;
    }
} else if (DataType == GL_FLOAT) {
    const GLfloat *rgba = (const GLfloat *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, rgba);
}
rgba += 4;
    }
} else if (DataType == GL_UNSIGNED_INT) {
    const GLuint *zValues = (const GLuint *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, zValues + i);
}
    }
} else if (DataType == GL_UNSIGNED_INT_24_8_EXT) {
    const GLuint *zValues = (const GLuint *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    GLfloat flt = (zValues[i] >> 8) * (1.0 / 0xffffff);
    Store(TexImage, x + i, y, z, &flt);
}
    }
} else {
    _mesa_problem(ctx, "invalid DataType in TextureRenderbuffer::PutRow");
}
    }

    void PutMonoRow(GLcontext *ctx, GLuint count,
    GLint x, GLint y,
    const void *value, const GLubyte *mask) override {
const GLint z = Zoffset;
GLuint i;
if (DataType == CHAN_TYPE) {
    const GLchan *rgba = (const GLchan *) value;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, rgba);
}
    }
} else if (DataType == GL_FLOAT) {
    const GLfloat *rgba = (const GLfloat *) value;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, rgba);
}
    }
} else if (DataType == GL_UNSIGNED_INT) {
    const GLuint zValue = *((const GLuint *) value);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, &zValue);
}
    }
} else if (DataType == GL_UNSIGNED_INT_24_8_EXT) {
    const GLuint zValue = *((const GLuint *) value);
    const GLfloat flt = (zValue >> 8) * (1.0 / 0xffffff);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x + i, y, z, &flt);
}
    }
} else {
    _mesa_problem(ctx, "invalid DataType in TextureRenderbuffer::PutMonoRow");
}
    }

    void PutValues(GLcontext *ctx, GLuint count,
   const GLint x[], const GLint y[],
   const void *values, const GLubyte *mask) override {
const GLint z = Zoffset;
GLuint i;
if (DataType == CHAN_TYPE) {
    const GLchan *rgba = (const GLchan *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, rgba);
}
rgba += 4;
    }
} else if (DataType == GL_FLOAT) {
    const GLfloat *rgba = (const GLfloat *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, rgba);
}
rgba += 4;
    }
} else if (DataType == GL_UNSIGNED_INT) {
    const GLuint *zValues = (const GLuint *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, zValues + i);
}
    }
} else if (DataType == GL_UNSIGNED_INT_24_8_EXT) {
    const GLuint *zValues = (const GLuint *) values;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    GLfloat flt = (zValues[i] >> 8) * (1.0 / 0xffffff);
    Store(TexImage, x[i], y[i], z, &flt);
}
    }
} else {
    _mesa_problem(ctx, "invalid DataType in TextureRenderbuffer::PutValues");
}
    }

    void PutMonoValues(GLcontext *ctx, GLuint count,
       const GLint x[], const GLint y[],
       const void *value, const GLubyte *mask) override {
const GLint z = Zoffset;
GLuint i;
if (DataType == CHAN_TYPE) {
    const GLchan *rgba = (const GLchan *) value;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, rgba);
}
    }
} else if (DataType == GL_FLOAT) {
    const GLfloat *rgba = (const GLfloat *) value;
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, rgba);
}
    }
} else if (DataType == GL_UNSIGNED_INT) {
    const GLuint zValue = *((const GLuint *) value);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, &zValue);
}
    }
} else if (DataType == GL_UNSIGNED_INT_24_8_EXT) {
    const GLuint zValue = *((const GLuint *) value);
    const GLfloat flt = (zValue >> 8) * (1.0 / 0xffffff);
    for (i = 0; i < count; i++) {
if (!mask || mask[i]) {
    Store(TexImage, x[i], y[i], z, &flt);
}
    }
} else {
    _mesa_problem(ctx, "invalid DataType in TextureRenderbuffer::PutMonoValues");
}
    }
};


/**
 * Create a renderbuffer object that wraps the given texture attachment.
 */
static int
wrap_texture(GLcontext *ctx, struct gl_renderbuffer_attachment *att)
{
    ASSERT(att->Type == GL_TEXTURE);
    ASSERT(att->Renderbuffer == NULL);

    auto *trb = new TextureRenderbuffer{};
    if (!trb) {
_mesa_error(ctx, GL_OUT_OF_MEMORY, "wrap_texture");
return -1;
    }

    _mesa_init_renderbuffer(trb, 0);

    _mesa_reference_renderbuffer(&att->Renderbuffer, trb);
    return 0;
}


/**
 * Update the renderbuffer wrapper for rendering to a texture.
 */
static void
update_wrapper(GLcontext *ctx, const struct gl_renderbuffer_attachment *att)
{
    if (!att)
return;

    TextureRenderbuffer *trb = static_cast<TextureRenderbuffer *>(att->Renderbuffer);
    (void) ctx;
    ASSERT(trb);

    trb->TexImage = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
    ASSERT(trb->TexImage);

    trb->Store = trb->TexImage->TexFormat->StoreTexel;
    ASSERT(trb->Store);

    trb->Zoffset = att->Zoffset;

    trb->Width  = trb->TexImage->Width;
    trb->Height = trb->TexImage->Height;
    trb->InternalFormat = trb->TexImage->InternalFormat;

    if (trb->TexImage->TexFormat->MesaFormat == MESA_FORMAT_Z24_S8) {
trb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
trb->DataType = GL_UNSIGNED_INT_24_8_EXT;
    } else if (trb->TexImage->TexFormat->MesaFormat == MESA_FORMAT_Z16) {
trb->_ActualFormat = GL_DEPTH_COMPONENT;
trb->DataType = GL_UNSIGNED_SHORT;
    } else if (trb->TexImage->TexFormat->MesaFormat == MESA_FORMAT_Z32) {
trb->_ActualFormat = GL_DEPTH_COMPONENT;
trb->DataType = GL_UNSIGNED_INT;
    } else if (trb->TexImage->TexFormat->DataType == GL_FLOAT) {
trb->_ActualFormat = trb->TexImage->InternalFormat;
trb->DataType = GL_FLOAT;
    } else {
trb->_ActualFormat = trb->TexImage->InternalFormat;
trb->DataType = CHAN_TYPE;
    }
    trb->_BaseFormat = trb->TexImage->TexFormat->BaseFormat;

    trb->Data = trb->TexImage->Data;

    trb->RedBits   = trb->TexImage->TexFormat->RedBits;
    trb->GreenBits = trb->TexImage->TexFormat->GreenBits;
    trb->BlueBits  = trb->TexImage->TexFormat->BlueBits;
    trb->AlphaBits = trb->TexImage->TexFormat->AlphaBits;
    trb->DepthBits = trb->TexImage->TexFormat->DepthBits;
}


/**
 * Called when rendering to a texture image begins, or when changing
 * the dest mipmap level, cube face, etc.
 */
void
_mesa_render_texture(GLcontext *ctx,
     struct gl_framebuffer *fb,
     struct gl_renderbuffer_attachment *att)
{
    int err_check = 0;
    (void) fb;

    if (!att->Renderbuffer) {
err_check = wrap_texture(ctx, att);
    }
    if (!err_check && att->Renderbuffer) {
update_wrapper(ctx, att);
    }
}


void
_mesa_finish_render_texture(GLcontext *ctx,
    struct gl_renderbuffer_attachment *att)
{
    /* do nothing */
    (void) ctx;
    (void) att;
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
