/*
 * Mesa 3-D graphics library ATI Fragment Shader
 *
 * Copyright (C) 2004  David Airlie   All Rights Reserved.
 *
 */

#ifndef ATIFRAGSHADER_H
#define ATIFRAGSHADER_H

/* atifs_instruction, atifs_setupinst, MAX_NUM_*_ATI, atifragshader_{src,dst}_register,
 * ATI_FRAGMENT_SHADER_*_OP, and ati_fragment_shader are defined in mtypes.h. */

struct ati_fs_opcode_st {
    GLenum opcode;
    GLint num_src_args;
};

extern struct ati_fs_opcode_st ati_fs_opcodes[];


extern struct ati_fragment_shader *
_mesa_new_ati_fragment_shader(GLcontext *ctx, GLuint id);

extern void
_mesa_delete_ati_fragment_shader(GLcontext *ctx,
				 struct ati_fragment_shader *s);


extern GLuint GLAPIENTRY _mesa_GenFragmentShadersATI(GLuint range);

extern void GLAPIENTRY _mesa_BindFragmentShaderATI(GLuint id);

extern void GLAPIENTRY _mesa_DeleteFragmentShaderATI(GLuint id);

extern void GLAPIENTRY _mesa_BeginFragmentShaderATI(void);

extern void GLAPIENTRY _mesa_EndFragmentShaderATI(void);

extern void GLAPIENTRY
_mesa_PassTexCoordATI(GLuint dst, GLuint coord, GLenum swizzle);

extern void GLAPIENTRY
_mesa_SampleMapATI(GLuint dst, GLuint interp, GLenum swizzle);

extern void GLAPIENTRY
_mesa_ColorFragmentOp1ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod);

extern void GLAPIENTRY
_mesa_ColorFragmentOp2ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
			  GLuint arg2Mod);

extern void GLAPIENTRY
_mesa_ColorFragmentOp3ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
			  GLuint arg2Mod, GLuint arg3, GLuint arg3Rep,
			  GLuint arg3Mod);

extern void GLAPIENTRY
_mesa_AlphaFragmentOp1ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod);

extern void GLAPIENTRY
_mesa_AlphaFragmentOp2ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
			  GLuint arg2Rep, GLuint arg2Mod);

extern void GLAPIENTRY
_mesa_AlphaFragmentOp3ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
			  GLuint arg2Rep, GLuint arg2Mod, GLuint arg3,
			  GLuint arg3Rep, GLuint arg3Mod);

extern void GLAPIENTRY
_mesa_SetFragmentShaderConstantATI(GLuint dst, const GLfloat * value);



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
