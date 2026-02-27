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

#if !defined SLANG_COMPILE_STRUCT_H
#define SLANG_COMPILE_STRUCT_H

#include <vector>
#include <memory>


    struct slang_struct;

    /**
     * A scope containing GLSL struct type definitions.
     *
     * C++17 modernisation: replaced raw array + count with std::vector<slang_struct>.
     * The destructor calls slang_struct_destruct for each element so that callers
     * do not need a separate explicit destruct call before deleting the scope.
     */
    struct slang_struct_scope {
	std::vector<slang_struct> structs; /**< owned struct definitions */
	slang_struct_scope *outer_scope{nullptr};

	~slang_struct_scope();  /**< Defined in slang_compile_struct.cpp */
    };

    extern void
    _slang_struct_scope_ctr(slang_struct_scope *);  /* legacy no-op; members are default-constructed */

    void slang_struct_scope_destruct(slang_struct_scope *);
    int slang_struct_scope_copy(slang_struct_scope *, const slang_struct_scope *);
    slang_struct *slang_struct_scope_find(slang_struct_scope *, slang_atom, int);

    struct slang_struct {
	slang_atom a_name{SLANG_ATOM_NULL};
	/** Owned variable scope for struct fields. */
	std::unique_ptr<slang_variable_scope> fields;
	/** Owned struct-type scope nested inside this struct. */
	std::unique_ptr<slang_struct_scope> structs;

	slang_struct() noexcept = default;

	/**
	 * Destructor – non-inline so that slang_variable_scope and
	 * slang_struct_scope are complete types when their destructors
	 * are instantiated.
	 */
	~slang_struct();
	/** Deep copy constructor. */
	slang_struct(const slang_struct &other);
	/** Deep copy assignment. */
	slang_struct &operator=(const slang_struct &other);
	/** Move constructor/assignment. */
	slang_struct(slang_struct &&) noexcept = default;
	slang_struct &operator=(slang_struct &&) noexcept = default;
    };

    int slang_struct_construct(slang_struct *);
    void slang_struct_destruct(slang_struct *);
    int slang_struct_copy(slang_struct *, const slang_struct *);
    int slang_struct_equal(const slang_struct *, const slang_struct *);



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
