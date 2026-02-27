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
#include <memory>


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


extern bool
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle *swz);

extern bool
_slang_is_swizzle_mask(const slang_swizzle *swz, GLuint rows);

extern void
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
 *
 * C++17 modernisation: _struct and _array are now RAII-managed via
 * std::unique_ptr.  The default deleter is sufficient since ~slang_struct()
 * handles all cleanup of owned members.
 */

struct slang_type_specifier {
    slang_type_specifier_type type = SLANG_SPEC_VOID;
    /** Owned struct definition (only when type == SLANG_SPEC_STRUCT). */
    std::unique_ptr<slang_struct> _struct;
    /** Owned element-type specifier (only when type == SLANG_SPEC_ARRAY). */
    std::unique_ptr<slang_type_specifier> _array;

    /** Default constructor – leaves _struct/_array null, type=VOID. */
    slang_type_specifier() noexcept = default;

    /**
     * Destructor.  Non-inline so that the full definition of slang_struct is
     * available in the TU that instantiates ~unique_ptr<slang_struct>.
     */
    ~slang_type_specifier();

    /** Deep copy constructor. */
    slang_type_specifier(const slang_type_specifier &other);
    /** Deep copy assignment. */
    slang_type_specifier &operator=(const slang_type_specifier &other);

    /** Move constructor – transfers ownership; source is left empty. */
    slang_type_specifier(slang_type_specifier &&other) noexcept;
    /** Move assignment – transfers ownership; source is left empty. */
    slang_type_specifier &operator=(slang_type_specifier &&other) noexcept;
};


/** Legacy wrapper – equivalent to default-constructing the specifier. */
inline void slang_type_specifier_ctr(slang_type_specifier *self) {
    *self = slang_type_specifier{};
}

/** Legacy wrapper – equivalent to destroying + default-constructing. */
inline void slang_type_specifier_dtr(slang_type_specifier *self) {
    self->_struct.reset();
    self->_array.reset();
    self->type = SLANG_SPEC_VOID;
}

/** Deep-copy a type specifier; always succeeds. */
inline bool
slang_type_specifier_copy(slang_type_specifier *x, const slang_type_specifier *y)
{
    *x = *y;   /* invokes deep-copy assignment */
    return true;
}

extern bool
slang_type_specifier_equal(const slang_type_specifier *,
			   const slang_type_specifier *);


struct slang_typeinfo {
    bool can_be_referenced = false;
    bool is_swizzled = false;
    slang_swizzle swz{};
    slang_type_specifier spec;
    GLuint array_len = 0;
};

/** Legacy wrapper – spec is default-constructed; just initialise the plain fields. */
inline bool slang_typeinfo_construct(slang_typeinfo *ti) {
    ti->can_be_referenced = false;
    ti->is_swizzled = false;
    ti->array_len = 0;
    return true;
}

/** Legacy wrapper – spec destructor runs automatically; nothing else to free. */
inline void slang_typeinfo_destruct(slang_typeinfo *ti) {
    ti->spec = slang_type_specifier{};
}


/**
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
extern bool
_slang_typeof_operation(const slang_assemble_ctx *,
			slang_operation *,
			slang_typeinfo *);

extern bool
_slang_typeof_operation_(slang_operation *,
			 const slang_name_space *,
			 slang_typeinfo *, slang_atom_pool *,
			 slang_info_log *log);

extern bool
_slang_type_is_matrix(slang_type_specifier_type);

extern bool
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
