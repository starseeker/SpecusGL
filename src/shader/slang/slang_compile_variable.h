/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
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

#ifndef SLANG_COMPILE_VARIABLE_H
#define SLANG_COMPILE_VARIABLE_H

#include <vector>
#include <memory>



    enum slang_type_qualifier {
	SLANG_QUAL_NONE,
	SLANG_QUAL_CONST,
	SLANG_QUAL_ATTRIBUTE,
	SLANG_QUAL_VARYING,
	SLANG_QUAL_UNIFORM,
	SLANG_QUAL_OUT,
	SLANG_QUAL_INOUT,
	SLANG_QUAL_FIXEDOUTPUT,      /* internal */
	SLANG_QUAL_FIXEDINPUT        /* internal */
    };

    extern slang_type_specifier_type
    slang_type_specifier_type_from_string(const char *);

    extern const char *
    slang_type_specifier_type_to_string(slang_type_specifier_type);



    /**
     * The type of a variable, including qualifier (const, varying, etc.) and
     * the base type specifier.
     *
     * C++17 modernisation: qualifier now has a default initialiser of
     * SLANG_QUAL_NONE so that a default-constructed value is already valid.
     * slang_fully_specified_type_construct() is still provided as a no-op
     * wrapper for existing callers.
     */
    struct slang_fully_specified_type {
	slang_type_qualifier qualifier{SLANG_QUAL_NONE};
	slang_type_specifier specifier;
    };

    /** Legacy no-op: qualifier has a default initialiser; specifier is RAII. */
    inline bool slang_fully_specified_type_construct(slang_fully_specified_type *type) {
        type->qualifier = SLANG_QUAL_NONE;
        type->specifier = slang_type_specifier{};
        return true;
    }

    /** Legacy no-op: specifier unique_ptrs free themselves. */
    inline void slang_fully_specified_type_destruct(slang_fully_specified_type *type) {
        type->specifier = slang_type_specifier{};
    }

    extern bool
    slang_fully_specified_type_copy(slang_fully_specified_type *,
				    const slang_fully_specified_type *);


    /**
     * A shading language program variable.
     *
     * C++17 modernisation:
     * - All POD fields have default member initialisers so that a
     *   default-constructed slang_variable is already in a valid state.
     * - initializer is a std::unique_ptr that automatically frees the
     *   associated expression tree.
     * - slang_variable_construct() is now an inline no-op kept only for
     *   backward compatibility.
     */
    struct slang_variable {
	slang_fully_specified_type type;          /**< Variable's data type */
	slang_atom a_name{SLANG_ATOM_NULL};       /**< The variable's name (char *) */
	GLuint array_len{0};                      /**< only if type == SLANG_SPEC_ARRAy */
	std::unique_ptr<slang_operation> initializer; /**< Optional initializer code */
	GLuint address{~0u};                      /**< Storage location */
	GLuint size{0};                           /**< Variable's size in bytes */
	GLboolean isTemp{false};                  /**< a named temporary (__resultTmp) */
	void *aux{nullptr};                       /**< Used during code gen */
    };


    /**
     * Basically a list of variables, with a pointer to the parent scope.
     *
     * C++17 modernisation: replaced raw pointer array + count with
     * std::vector<slang_variable *>.  Each element is heap-owned by this scope.
     *
     * The destructor iterates over owned variables and frees them, mirroring
     * the old slang_variable_scope_destruct() logic, so callers do not need to
     * call slang_variable_scope_destruct() before delete.
     */
    struct slang_variable_scope {
	std::vector<slang_variable *> variables; /**< Owned ptrs to variables */
	slang_variable_scope *outer_scope{nullptr};

	~slang_variable_scope();  /**< Defined in slang_compile_variable.cpp */
    };


    extern slang_variable_scope *
    _slang_variable_scope_new(slang_variable_scope *parent);

    extern void
    _slang_variable_scope_ctr(slang_variable_scope *);

    extern void
    slang_variable_scope_destruct(slang_variable_scope *);

    extern bool
    slang_variable_scope_copy(slang_variable_scope *,
			      const slang_variable_scope *);

    extern slang_variable *
    slang_variable_scope_grow(slang_variable_scope *);

    /**
     * Legacy no-op: slang_variable is fully default-constructible.
     * Kept for backward compatibility only.
     */
    inline bool slang_variable_construct(slang_variable *) { return true; }

    /**
     * Legacy cleanup: resets RAII members; kept for backward compatibility.
     * Actual memory is freed by the members' own destructors.
     */
    inline void slang_variable_destruct(slang_variable *var) {
        var->type.specifier = slang_type_specifier{};
        var->initializer.reset();
    }

    extern bool
    slang_variable_copy(slang_variable *, const slang_variable *);

    extern slang_variable *
    _slang_locate_variable(const slang_variable_scope *, const slang_atom a_name,
			   bool all);




#endif /* SLANG_COMPILE_VARIABLE_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
