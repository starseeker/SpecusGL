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
 * \file prog_instruction.h
 *
 * Vertex/fragment program instruction datatypes and constants.
 *
 * \author Brian Paul
 * \author Keith Whitwell
 * \author Ian Romanick <idr@us.ibm.com>
 */


#ifndef PROG_INSTRUCTION_H
#define PROG_INSTRUCTION_H

#include <string>




/**
 * Swizzle indexes.
 * Do not change!
 */
/*@{*/
constexpr GLuint SWIZZLE_X    = 0;
constexpr GLuint SWIZZLE_Y    = 1;
constexpr GLuint SWIZZLE_Z    = 2;
constexpr GLuint SWIZZLE_W    = 3;
constexpr GLuint SWIZZLE_ZERO = 4; /**< For SWZ instruction only */
constexpr GLuint SWIZZLE_ONE  = 5; /**< For SWZ instruction only */
constexpr GLuint SWIZZLE_NIL  = 7; /**< used during shader code gen (undefined value) */
/*@}*/

constexpr GLuint MAKE_SWIZZLE4(GLuint a, GLuint b, GLuint c, GLuint d) {
    return (a << 0) | (b << 3) | (c << 6) | (d << 9);
}
constexpr GLuint SWIZZLE_NOOP = (0<<0)|(1<<3)|(2<<6)|(3<<9);
constexpr GLuint GET_SWZ(GLuint swz, GLuint idx) { return (swz >> (idx*3)) & 0x7; }
constexpr GLuint GET_BIT(GLuint msk, GLuint idx) { return (msk >> idx) & 0x1; }

constexpr GLuint SWIZZLE_XYZW = (0<<0)|(1<<3)|(2<<6)|(3<<9);
constexpr GLuint SWIZZLE_XXXX = (0<<0)|(0<<3)|(0<<6)|(0<<9);
constexpr GLuint SWIZZLE_YYYY = (1<<0)|(1<<3)|(1<<6)|(1<<9);
constexpr GLuint SWIZZLE_ZZZZ = (2<<0)|(2<<3)|(2<<6)|(2<<9);
constexpr GLuint SWIZZLE_WWWW = (3<<0)|(3<<3)|(3<<6)|(3<<9);


/**
 * Writemask values, 1 bit per component.
 */
/*@{*/
constexpr GLuint WRITEMASK_X    = 0x1;
constexpr GLuint WRITEMASK_Y    = 0x2;
constexpr GLuint WRITEMASK_XY   = 0x3;
constexpr GLuint WRITEMASK_Z    = 0x4;
constexpr GLuint WRITEMASK_XZ   = 0x5;
constexpr GLuint WRITEMASK_YZ   = 0x6;
constexpr GLuint WRITEMASK_XYZ  = 0x7;
constexpr GLuint WRITEMASK_W    = 0x8;
constexpr GLuint WRITEMASK_XW   = 0x9;
constexpr GLuint WRITEMASK_YW   = 0xa;
constexpr GLuint WRITEMASK_XYW  = 0xb;
constexpr GLuint WRITEMASK_ZW   = 0xc;
constexpr GLuint WRITEMASK_XZW  = 0xd;
constexpr GLuint WRITEMASK_YZW  = 0xe;
constexpr GLuint WRITEMASK_XYZW = 0xf;
/*@}*/


/**
 * Condition codes
 */
/*@{*/
constexpr GLuint COND_GT = 1; /**< greater than zero */
constexpr GLuint COND_EQ = 2; /**< equal to zero */
constexpr GLuint COND_LT = 3; /**< less than zero */
constexpr GLuint COND_UN = 4; /**< unordered (NaN) */
constexpr GLuint COND_GE = 5; /**< greater then or equal to zero */
constexpr GLuint COND_LE = 6; /**< less then or equal to zero */
constexpr GLuint COND_NE = 7; /**< not equal to zero */
constexpr GLuint COND_TR = 8; /**< always true */
constexpr GLuint COND_FL = 9; /**< always false */
/*@}*/


/**
 * Instruction precision for GL_NV_fragment_program
 */
/*@{*/
constexpr GLuint FLOAT32 = 0x1;
constexpr GLuint FLOAT16 = 0x2;
constexpr GLuint FIXED12 = 0x4;
/*@}*/


/**
 * Saturation modes when storing values.
 */
/*@{*/
constexpr GLuint SATURATE_OFF            = 0;
constexpr GLuint SATURATE_ZERO_ONE       = 1;
constexpr GLuint SATURATE_PLUS_MINUS_ONE = 2;
/*@}*/


/**
 * Per-component negation masks
 */
/*@{*/
constexpr GLuint NEGATE_X    = 0x1;
constexpr GLuint NEGATE_Y    = 0x2;
constexpr GLuint NEGATE_Z    = 0x4;
constexpr GLuint NEGATE_W    = 0x8;
constexpr GLuint NEGATE_XYZW = 0xf;
constexpr GLuint NEGATE_NONE = 0x0;
/*@}*/


/**
 * Program instruction opcodes, for both vertex and fragment programs.
 * \note changes to this opcode list must be reflected in t_vb_arbprogram.c
 */
enum prog_opcode {
    /* ARB_vp   ARB_fp   NV_vp   NV_fp     GLSL */
    /*------------------------------------------*/
    OPCODE_NOP = 0,   /*                                      X   */
    OPCODE_ABS,       /*   X        X       1.1               X   */
    OPCODE_ADD,       /*   X        X       X       X         X   */
    OPCODE_ARA,       /*                    2                     */
    OPCODE_ARL,       /*   X                X                     */
    OPCODE_ARL_NV,    /*                    2                     */
    OPCODE_ARR,       /*                    2                     */
    OPCODE_BGNLOOP,   /*                                     opt  */
    OPCODE_BGNSUB,    /*                                     opt  */
    OPCODE_BRA,       /*                    2                 X   */
    OPCODE_BRK,       /*                    2                opt  */
    OPCODE_CAL,       /*                    2       2             */
    OPCODE_CMP,       /*            X                             */
    OPCODE_CONT,      /*                                     opt  */
    OPCODE_COS,       /*            X       2       X         X   */
    OPCODE_DDX,       /*                            X         X   */
    OPCODE_DDY,       /*                            X         X   */
    OPCODE_DP3,       /*   X        X       X       X         X   */
    OPCODE_DP4,       /*   X        X       X       X         X   */
    OPCODE_DPH,       /*   X        X       1.1                   */
    OPCODE_DST,       /*   X        X       X       X             */
    OPCODE_ELSE,      /*                                      X   */
    OPCODE_END,       /*   X        X       X       X        opt  */
    OPCODE_ENDIF,     /*                                     opt  */
    OPCODE_ENDLOOP,   /*                                     opt  */
    OPCODE_ENDSUB,    /*                                     opt  */
    OPCODE_EX2,       /*   X        X       2       X         X   */
    OPCODE_EXP,       /*   X                X                 X   */
    OPCODE_FLR,       /*   X        X       2       X         X   */
    OPCODE_FRC,       /*   X        X       2       X         X   */
    OPCODE_IF,        /*                                     opt  */
    OPCODE_INT,       /*                                      X   */
    OPCODE_KIL,       /*            X                             */
    OPCODE_KIL_NV,    /*                            X         X   */
    OPCODE_LG2,       /*   X        X       2       X         X   */
    OPCODE_LIT,       /*   X        X       X       X             */
    OPCODE_LOG,       /*   X                X                 X   */
    OPCODE_LRP,       /*            X               X             */
    OPCODE_MAD,       /*   X        X       X       X         X   */
    OPCODE_MAX,       /*   X        X       X       X         X   */
    OPCODE_MIN,       /*   X        X       X       X         X   */
    OPCODE_MOV,       /*   X        X       X       X         X   */
    OPCODE_MUL,       /*   X        X       X       X         X   */
    OPCODE_NOISE1,    /*                                      X   */
    OPCODE_NOISE2,    /*                                      X   */
    OPCODE_NOISE3,    /*                                      X   */
    OPCODE_NOISE4,    /*                                      X   */
    OPCODE_PK2H,      /*                            X             */
    OPCODE_PK2US,     /*                            X             */
    OPCODE_PK4B,      /*                            X             */
    OPCODE_PK4UB,     /*                            X             */
    OPCODE_POW,       /*   X        X               X         X   */
    OPCODE_POPA,      /*                    3                     */
    OPCODE_PRINT,     /*                    X       X             */
    OPCODE_PUSHA,     /*                    3                     */
    OPCODE_RCC,       /*                    1.1                   */
    OPCODE_RCP,       /*   X        X       X       X         X   */
    OPCODE_RET,       /*                    2       2             */
    OPCODE_RFL,       /*            X               X             */
    OPCODE_RSQ,       /*   X        X       X       X         X   */
    OPCODE_SCS,       /*            X                             */
    OPCODE_SEQ,       /*                    2       X         X   */
    OPCODE_SFL,       /*                    2       X             */
    OPCODE_SGE,       /*   X        X       X       X         X   */
    OPCODE_SGT,       /*                    2       X         X   */
    OPCODE_SIN,       /*            X       2       X         X   */
    OPCODE_SLE,       /*                    2       X         X   */
    OPCODE_SLT,       /*   X        X       X       X         X   */
    OPCODE_SNE,       /*                    2       X         X   */
    OPCODE_SSG,       /*                    2                     */
    OPCODE_STR,       /*                    2       X             */
    OPCODE_SUB,       /*   X        X       1.1     X         X   */
    OPCODE_SWZ,       /*   X        X                             */
    OPCODE_TEX,       /*            X       3       X         X   */
    OPCODE_TXB,       /*            X       3                 X   */
    OPCODE_TXD,       /*                            X         X   */
    OPCODE_TXL,       /*                    3       2         X   */
    OPCODE_TXP,       /*            X                         X   */
    OPCODE_TXP_NV,    /*                    3       X             */
    OPCODE_UP2H,      /*                            X             */
    OPCODE_UP2US,     /*                            X             */
    OPCODE_UP4B,      /*                            X             */
    OPCODE_UP4UB,     /*                            X             */
    OPCODE_X2D,       /*                            X             */
    OPCODE_XPD,       /*   X        X                         X   */
    MAX_OPCODE
};
using gl_inst_opcode = prog_opcode;


/**
 * Instruction source register.
 */
struct prog_src_register {
    GLuint File:4;	/**< One of the PROGRAM_* register file values. */
    GLint Index:9;	/**< May be negative for relative addressing. */
    GLuint Swizzle:12;
    GLuint RelAddr:1;

    /**
     * \name Source register "sign" control.
     *
     * The ARB and NV extensions allow varrying degrees of control over the
     * sign of the source vector components.  These values allow enough control
     * for all flavors of the extensions.
     */
    /*@{*/
    /**
     * Per-component negation for the SWZ instruction.  For non-SWZ
     * instructions the only possible values are NEGATE_XYZW and NEGATE_NONE.
     *
     * \since
     * ARB_vertex_program, ARB_fragment_program
     */
    GLuint NegateBase:4;

    /**
     * Take the component-wise absolute value.
     *
     * \since
     * NV_fragment_program, NV_fragment_program_option, NV_vertex_program2,
     * NV_vertex_program2_option.
     */
    GLuint Abs:1;

    /**
     * Post-absolute value negation (all components).
     */
    GLuint NegateAbs:1;
    /*@}*/
};


/**
 * Instruction destination register.
 */
struct prog_dst_register {
    /**
     * One of the PROGRAM_* register file values.
     */
    GLuint File:4;

    GLuint Index:8;
    GLuint WriteMask:4;

    /**
     * \name Conditional destination update control.
     *
     * \since
     * NV_fragment_program, NV_fragment_program_option, NV_vertex_program2,
     * NV_vertex_program2_option.
     */
    /*@{*/
    /**
     * Takes one of the 9 possible condition values (EQ, FL, GT, GE, LE, LT,
     * NE, TR, or UN).  Dest reg is only written to if the matching
     * (swizzled) condition code value passes.  When a conditional update mask
     * is not specified, this will be \c COND_TR.
     */
    GLuint CondMask:4;

    /**
     * Condition code swizzle value.
     */
    GLuint CondSwizzle:12;

    /**
     * Selects the condition code register to use for conditional destination
     * update masking.  In NV_fragmnet_program or NV_vertex_program2 mode, only
     * condition code register 0 is available.  In NV_vertex_program3 mode,
     * condition code registers 0 and 1 are available.
     */
    GLuint CondSrc:1;
    /*@}*/

    GLuint pad:31;
};


/**
 * Vertex/fragment program instruction.
 */
struct prog_instruction {
    gl_inst_opcode Opcode;
#if FEATURE_MESA_program_debug
    GLshort StringPos;
#endif
    /**
     * Optional string data for PRINT instructions.
     * Replaces the old void* that required manual new[]/delete[].
     */
    std::string Data;

    struct prog_src_register SrcReg[3];
    struct prog_dst_register DstReg;

    /**
     * Indicates that the instruction should update the condition code
     * register.
     *
     * \since
     * NV_fragment_program, NV_fragment_program_option, NV_vertex_program2,
     * NV_vertex_program2_option.
     */
    GLuint CondUpdate:1;

    /**
     * If prog_instruction::CondUpdate is \c GL_TRUE, this value selects the
     * condition code register that is to be updated.
     *
     * In GL_NV_fragment_program or GL_NV_vertex_program2 mode, only condition
     * code register 0 is available.  In GL_NV_vertex_program3 mode, condition
     * code registers 0 and 1 are available.
     *
     * \since
     * NV_fragment_program, NV_fragment_program_option, NV_vertex_program2,
     * NV_vertex_program2_option.
     */
    GLuint CondDst:1;

    /**
     * Saturate each value of the vectored result to the range [0,1] or the
     * range [-1,1].  \c SSAT mode (i.e., saturation to the range [-1,1]) is
     * only available in NV_fragment_program2 mode.
     * Value is one of the SATURATE_* tokens.
     *
     * \since
     * NV_fragment_program, NV_fragment_program_option, NV_vertex_program3.
     */
    GLuint SaturateMode:2;

    /**
     * Per-instruction selectable precision.
     *
     * \since
     * NV_fragment_program, NV_fragment_program_option.
     */
    GLuint Precision:3;

    /**
     * \name Texture source controls.
     *
     * The texture source controls are only used with the \c TEX, \c TXD,
     * \c TXL, and \c TXP instructions.
     *
     * \since
     * ARB_fragment_program, NV_fragment_program, NV_vertex_program3.
     */
    /*@{*/
    /**
     * Source texture unit.  OpenGL supports a maximum of 32 texture
     * units.
     */
    GLuint TexSrcUnit:5;

    /**
     * Source texture target, one of TEXTURE_{1D,2D,3D,CUBE,RECT}_INDEX.
     */
    GLuint TexSrcTarget:3;
    /*@}*/

    /**
     * For BRA and CAL instructions, the location to jump to.
     * For BGNLOOP, points to ENDLOOP (and vice-versa).
     * For BRK, points to BGNLOOP (which points to ENDLOOP).
     * For IF, points to else or endif.
     * For ELSE, points to endif.
     */
    GLint BranchTarget;

    /**
     * For TEX instructions in shaders, the sampler to use for the
     * texture lookup.
     */
    GLint Sampler;

    std::string Comment;
};


extern void
_mesa_init_instructions(struct prog_instruction *inst, GLuint count);

extern GLuint
_mesa_num_inst_src_regs(gl_inst_opcode opcode);

extern const char *
_mesa_opcode_string(gl_inst_opcode opcode);




#endif /* PROG_INSTRUCTION_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
