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

#if !defined SLANG_COMPILE_H
#define SLANG_COMPILE_H

#include "imports.h"
#include "mtypes.h"
#include "slang_typeinfo.h"
#include "slang_compile_variable.h"
#include "slang_compile_struct.h"
#include "slang_compile_operation.h"
#include "slang_compile_function.h"


    enum slang_unit_type {
	SLANG_UNIT_FRAGMENT_SHADER,
	SLANG_UNIT_VERTEX_SHADER,
	SLANG_UNIT_FRAGMENT_BUILTIN,
	SLANG_UNIT_VERTEX_BUILTIN
    };


    struct slang_var_pool {
	GLuint next_addr;
    };

    struct slang_code_object;

    struct slang_code_unit {
	slang_variable_scope vars;
	slang_function_scope funs;
	slang_struct_scope structs;
	slang_unit_type type;
	slang_code_object *object;
    };


    extern void
    _slang_code_unit_ctr(slang_code_unit *, slang_code_object *);

    extern void
    _slang_code_unit_dtr(slang_code_unit *);

#define SLANG_BUILTIN_CORE   0
#define SLANG_BUILTIN_120_CORE   1
#define SLANG_BUILTIN_COMMON 2
#define SLANG_BUILTIN_TARGET 3

#define SLANG_BUILTIN_TOTAL  4

    struct slang_code_object {
	slang_code_unit builtin[SLANG_BUILTIN_TOTAL];
	slang_code_unit unit;
	slang_var_pool varpool;
	slang_atom_pool atompool;
    };

    extern void
    _slang_code_object_ctr(slang_code_object *);

    extern void
    _slang_code_object_dtr(slang_code_object *);

    extern bool
    _slang_compile(GLcontext *ctx, struct gl_shader *shader);



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
