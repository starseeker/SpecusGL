/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
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

#ifndef SLANG_TYPEINFO_H
#define SLANG_TYPEINFO_H 1



#include "imports.h"
#include "mtypes.h"
#include "slang_log.h"
#include "slang_utility.h"
#include "slang_vartable.h"


struct slang_operation;


struct slang_function_scope;
struct slang_function;
struct slang_label;
struct slang_variable_scope;
struct slang_struct_scope;
struct slang_struct;
struct slang_ir_node;

/**
 * Holds complete information about vector swizzle - the <swizzle>
 * array contains vector component source indices, where 0 is "x", 1
 * is "y", 2 is "z" and 3 is "w".
 * Example: "xwz" --> { 3, { 0, 3, 2, not used } }.
 */
struct slang_swizzle {
    GLuint num_components;
    GLuint swizzle[4];
};

struct slang_name_space {
    slang_function_scope *funcs;
    slang_struct_scope *structs;
    slang_variable_scope *vars;
};


struct slang_assemble_ctx {
    slang_atom_pool *atoms;
    slang_name_space space;
    struct gl_program *program;
    slang_var_table *vartable;
    slang_info_log *log;
    slang_label *curFuncEndLabel;
    slang_ir_node *CurLoop;
    slang_function *CurFunction;
};


extern slang_function *
_slang_locate_function(const slang_function_scope *funcs,
		       slang_atom name, slang_operation *params,
		       GLuint num_params,
		       const slang_name_space *space,
		       slang_atom_pool *atoms, slang_info_log *log);


extern GLboolean
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle *swz);

extern GLboolean
_slang_is_swizzle_mask(const slang_swizzle *swz, GLuint rows);

extern GLvoid
_slang_multiply_swizzles(slang_swizzle *, const slang_swizzle *,
			 const slang_swizzle *);


/**
 * The basic shading language types (float, vec4, mat3, etc)
 */
enum slang_type_specifier_type {
    SLANG_SPEC_VOID,
    SLANG_SPEC_BOOL,
    SLANG_SPEC_BVEC2,
    SLANG_SPEC_BVEC3,
    SLANG_SPEC_BVEC4,
    SLANG_SPEC_INT,
    SLANG_SPEC_IVEC2,
    SLANG_SPEC_IVEC3,
    SLANG_SPEC_IVEC4,
    SLANG_SPEC_FLOAT,
    SLANG_SPEC_VEC2,
    SLANG_SPEC_VEC3,
    SLANG_SPEC_VEC4,
    SLANG_SPEC_MAT2,
    SLANG_SPEC_MAT3,
    SLANG_SPEC_MAT4,
    SLANG_SPEC_MAT23,
    SLANG_SPEC_MAT32,
    SLANG_SPEC_MAT24,
    SLANG_SPEC_MAT42,
    SLANG_SPEC_MAT34,
    SLANG_SPEC_MAT43,
    SLANG_SPEC_SAMPLER1D,
    SLANG_SPEC_SAMPLER2D,
    SLANG_SPEC_SAMPLER3D,
    SLANG_SPEC_SAMPLERCUBE,
    SLANG_SPEC_SAMPLER2DRECT,
    SLANG_SPEC_SAMPLER1DSHADOW,
    SLANG_SPEC_SAMPLER2DSHADOW,
    SLANG_SPEC_SAMPLER2DRECTSHADOW,
    SLANG_SPEC_STRUCT,
    SLANG_SPEC_ARRAY
};


/**
 * Describes more sophisticated types, like structs and arrays.
 */
struct slang_type_specifier {
    slang_type_specifier_type type;
    slang_struct *_struct;         /**< used if type == spec_struct */
    slang_type_specifier *_array;  /**< used if type == spec_array */
};


extern GLvoid
slang_type_specifier_ctr(slang_type_specifier *);

extern GLvoid
slang_type_specifier_dtr(slang_type_specifier *);

extern GLboolean
slang_type_specifier_copy(slang_type_specifier *, const slang_type_specifier *);

extern GLboolean
slang_type_specifier_equal(const slang_type_specifier *,
			   const slang_type_specifier *);


struct slang_typeinfo {
    GLboolean can_be_referenced;
    GLboolean is_swizzled;
    slang_swizzle swz;
    slang_type_specifier spec;
    GLuint array_len;
};

extern GLboolean
slang_typeinfo_construct(slang_typeinfo *);

extern GLvoid
slang_typeinfo_destruct(slang_typeinfo *);


/**
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
extern GLboolean
_slang_typeof_operation(const slang_assemble_ctx *,
			slang_operation *,
			slang_typeinfo *);

extern GLboolean
_slang_typeof_operation_(slang_operation *,
			 const slang_name_space *,
			 slang_typeinfo *, slang_atom_pool *,
			 slang_info_log *log);

extern GLboolean
_slang_type_is_matrix(slang_type_specifier_type);

extern GLboolean
_slang_type_is_vector(slang_type_specifier_type);

extern slang_type_specifier_type
_slang_type_base(slang_type_specifier_type);

extern GLuint
_slang_type_dim(slang_type_specifier_type);

extern GLenum
_slang_gltype_from_specifier(const slang_type_specifier *type);



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
