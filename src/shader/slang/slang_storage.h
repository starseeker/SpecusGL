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

#ifndef SLANG_STORAGE_H
#define SLANG_STORAGE_H



#include "slang_compile.h"
#include "slang_typeinfo.h"

#include <memory>
#include <vector>


/*
 * Program variable data storage is kept completely transparent to the
 * front-end compiler. It is up to the back-end how the data is
 * actually allocated. The slang_storage_type enum provides the basic
 * information about how the memory is interpreted. This abstract
 * piece of memory is called a data slot. A data slot of a particular
 * type has a fixed size.
 *
 * For now, only the three basic types are supported, that is bool,
 * int and float. Other built-in types like vector or matrix can
 * easily be decomposed into a series of basic types.
 *
 * If the vec4 module is enabled, 4-component vectors of floats are
 * used when possible. 4x4 matrices are constructed of 4 vec4 slots.
 */
enum slang_storage_type {
    /* core */
    SLANG_STORE_AGGREGATE,
    SLANG_STORE_BOOL,
    SLANG_STORE_INT,
    SLANG_STORE_FLOAT,
    /* vec4 */
    SLANG_STORE_VEC4
};


struct slang_storage_aggregate;

/**
 * The slang_storage_array structure groups data slots of the same
 * type into an array. This array has a fixed length.
 */
struct slang_storage_array {
    slang_storage_type type{SLANG_STORE_AGGREGATE};
    std::unique_ptr<slang_storage_aggregate> aggregate; /**< owned sub-aggregate (RAII) */
    GLuint length{0};
};

bool slang_storage_array_construct(slang_storage_array *);
GLvoid slang_storage_array_destruct(slang_storage_array *);


/**
 * The slang_storage_aggregate structure relaxes the indirect
 * addressing requirement for slang_storage_array structure.
 *
 * C++17 modernisation: replaced raw arrays + count with std::vector<slang_storage_array>.
 */
struct slang_storage_aggregate {
    std::vector<slang_storage_array> arrays; /**< owned array elements */
};

bool slang_storage_aggregate_construct(slang_storage_aggregate *);
GLvoid slang_storage_aggregate_destruct(slang_storage_aggregate *);


extern bool
_slang_aggregate_variable(slang_storage_aggregate *agg,
			  slang_type_specifier *spec,
			  GLuint array_len,
			  slang_function_scope *funcs,
			  slang_struct_scope *structs,
			  slang_variable_scope *vars,
			  slang_atom_pool *atoms);

/*
 * Returns the size (in machine units) of the given storage type.
 * It is an error to pass-in SLANG_STORE_AGGREGATE.
 * Returns 0 on error.
 */
extern GLuint
_slang_sizeof_type(slang_storage_type);


/**
 * Returns total size (in machine units) of the given aggregate.
 * Returns 0 on error.
 */
extern GLuint
_slang_sizeof_aggregate(const slang_storage_aggregate *);


#if 0
/**
 * Converts structured aggregate to a flat one, with arrays of generic
 * type being one-element long.  Returns GL_TRUE on success.  Returns
 * GL_FALSE otherwise.
 */
extern GLboolean
_slang_flatten_aggregate(slang_storage_aggregate *,
			 const slang_storage_aggregate *);

#endif



#endif /* SLANG_STORAGE_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
