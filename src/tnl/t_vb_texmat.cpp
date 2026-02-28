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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"

/* Is there any real benefit seperating texmat from texgen?  It means
 * we need two lots of intermediate storage.  Any changes to
 * _NEW_TEXTURE will invalidate both sets -- it's only on changes to
 * *only* _NEW_TEXTURE_MATRIX that texgen survives but texmat doesn't.
 *
 * However, the seperation of this code from the complex texgen stuff
 * is very appealing.
 */
struct texmat_stage_data {
    GLvector4f texcoord[MAX_TEXTURE_COORD_UNITS];
};

#define TEXMAT_STAGE_DATA(stage) ((struct texmat_stage_data *)stage->privatePtr)



static bool run_texmat_stage(GLcontext *ctx,
				  struct tnl_pipeline_stage *stage)
{
    struct texmat_stage_data *store = TEXMAT_STAGE_DATA(stage);
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLuint i;

    if (!ctx->Texture._TexMatEnabled || ctx->VertexProgram._Current)
	return true;

    /* ENABLE_TEXMAT implies that the texture matrix is not the
     * identity, so we don't have to check that here.
     */
    for (i = 0 ; i < ctx->Const.MaxTextureCoordUnits ; i++) {
	if (ctx->Texture._TexMatEnabled & ENABLE_TEXMAT(i)) {
	    (void) TransformRaw(&store->texcoord[i],
				ctx->TextureMatrixStack[i].Top,
				VB->AttribPtr[_TNL_ATTRIB_TEX0 + i]);

	    VB->TexCoordPtr[i] =
		VB->AttribPtr[VERT_ATTRIB_TEX0+i] = &store->texcoord[i];
	}
    }

    return true;
}


/* Called the first time stage->run() is invoked.
 */
static bool alloc_texmat_data(GLcontext *ctx,
				   struct tnl_pipeline_stage *stage)
{
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLuint i;

    auto *store = new texmat_stage_data{};
    stage->privatePtr    = store;
    stage->privateDeleter = [](void *p){ delete static_cast<texmat_stage_data *>(p); };
    if (!store)
	return false;

    for (i = 0 ; i < ctx->Const.MaxTextureCoordUnits ; i++)
	store->texcoord[i].alloc(0, VB->Size, 32);

    return true;
}



const struct tnl_pipeline_stage _tnl_texture_transform_stage = {
    "texture transform",	/* name */
    nullptr,			/* privatePtr */
    nullptr,			/* privateDeleter (set by create) */
    alloc_texmat_data,		/* create */
    nullptr,			/* validate */
    run_texmat_stage,		/* run */
};

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
