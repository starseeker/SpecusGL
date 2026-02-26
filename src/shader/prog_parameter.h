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
 * \file prog_parameter.c
 * Program parameter lists and functions.
 * \author Brian Paul
 */

#ifndef PROG_PARAMETER_H
#define PROG_PARAMETER_H



#include "mtypes.h"
#include "prog_statevars.h"

#include <string>
#include <vector>


/**
 * Program parameter.
 * Used for NV_fragment_program for "DEFINE"d constants and "DECLARE"d
 * parameters.
 * Also used by ARB_vertex/fragment_programs for state variables, etc.
 * Used by shaders for uniforms, constants, varying vars, etc.
 */
struct gl_program_parameter {
    std::string Name;        /**< Parameter name */
    enum register_file Type; /**< PROGRAM_NAMED_PARAM, CONSTANT or STATE_VAR */
    GLenum DataType;         /**< GL_FLOAT, GL_FLOAT_VEC2, etc */
    GLuint Size;             /**< Number of components (1..4) */
    /**
     * A sequence of STATE_* tokens and integers to identify GL state.
     */
    gl_state_index StateIndexes[STATE_LENGTH];
};


/**
 * List of gl_program_parameter instances.
 * Manages its own parameter values memory.
 */
struct gl_program_parameter_list {
    std::vector<gl_program_parameter> Parameters; /**< Parameter descriptors */
    std::vector<std::array<GLfloat, 4>> ParameterValues; /**< Per-parameter float[4] values */
    GLbitfield StateFlags = 0; /**< _NEW_* flags indicating which state changes
                                   might invalidate ParameterValues[] */

    gl_program_parameter_list() = default;
    ~gl_program_parameter_list() = default;

    /* Prevent accidental copies - use clone() explicitly */
    gl_program_parameter_list(const gl_program_parameter_list &) = delete;
    gl_program_parameter_list & operator=(const gl_program_parameter_list &) = delete;

    GLuint NumParameters() const { return static_cast<GLuint>(Parameters.size()); }

    GLint add_parameter(enum register_file type, const char *name,
                        GLuint size, GLenum datatype, const GLfloat *values,
                        const gl_state_index state[STATE_LENGTH]);
    GLint add_named_parameter(const char *name, const GLfloat values[4]);
    GLint add_named_constant(const char *name, const GLfloat values[4], GLuint size);
    GLint add_unnamed_constant(const GLfloat values[4], GLuint size, GLuint *swizzleOut);
    GLint add_uniform(const char *name, GLuint size, GLenum datatype);
    GLint add_sampler(const char *name, GLenum datatype);
    GLint add_varying(const char *name, GLuint size);
    GLint add_attribute(const char *name, GLint size, GLint attrib);
    GLint add_state_reference(const gl_state_index stateTokens[STATE_LENGTH]);

    GLfloat *lookup_parameter_value(GLsizei nameLen, const char *name);
    GLint lookup_parameter_index(GLsizei nameLen, const char *name) const;
    GLboolean lookup_parameter_constant(const GLfloat v[], GLuint vSize,
                                        GLint *posOut, GLuint *swizzleOut) const;
    GLuint longest_parameter_name(enum register_file type) const;
    GLuint num_parameters_of_type(enum register_file type) const;
    gl_program_parameter_list *clone() const;
};

/* Legacy C-style wrappers – prefer member functions for new code. */
inline gl_program_parameter_list *
_mesa_new_parameter_list() { return new gl_program_parameter_list{}; }

inline void
_mesa_free_parameter_list(gl_program_parameter_list *p) { delete p; }

inline gl_program_parameter_list *
_mesa_clone_parameter_list(const gl_program_parameter_list *l)
{ return l ? l->clone() : nullptr; }

inline GLint
_mesa_add_parameter(gl_program_parameter_list *p,
    enum register_file type, const char *name, GLuint size,
    GLenum datatype, const GLfloat *values,
    const gl_state_index state[STATE_LENGTH])
{ return p->add_parameter(type, name, size, datatype, values, state); }

inline GLint _mesa_add_named_parameter(gl_program_parameter_list *p,
    const char *name, const GLfloat v[4])
{ return p->add_named_parameter(name, v); }

inline GLint _mesa_add_named_constant(gl_program_parameter_list *p,
    const char *name, const GLfloat v[4], GLuint size)
{ return p->add_named_constant(name, v, size); }

inline GLint _mesa_add_unnamed_constant(gl_program_parameter_list *p,
    const GLfloat v[4], GLuint size, GLuint *swizzleOut)
{ return p->add_unnamed_constant(v, size, swizzleOut); }

inline GLint _mesa_add_uniform(gl_program_parameter_list *p,
    const char *name, GLuint size, GLenum datatype)
{ return p->add_uniform(name, size, datatype); }

inline GLint _mesa_add_sampler(gl_program_parameter_list *p,
    const char *name, GLenum datatype)
{ return p->add_sampler(name, datatype); }

inline GLint _mesa_add_varying(gl_program_parameter_list *p,
    const char *name, GLuint size)
{ return p->add_varying(name, size); }

inline GLint _mesa_add_attribute(gl_program_parameter_list *p,
    const char *name, GLint size, GLint attrib)
{ return p->add_attribute(name, size, attrib); }

inline GLint _mesa_add_state_reference(gl_program_parameter_list *p,
    const gl_state_index stateTokens[STATE_LENGTH])
{ return p->add_state_reference(stateTokens); }

inline GLfloat *_mesa_lookup_parameter_value(
    gl_program_parameter_list *p, GLsizei nameLen, const char *name)
{ return p ? p->lookup_parameter_value(nameLen, name) : nullptr; }

inline GLint _mesa_lookup_parameter_index(
    const gl_program_parameter_list *p, GLsizei nameLen, const char *name)
{ return p ? p->lookup_parameter_index(nameLen, name) : -1; }

inline GLboolean _mesa_lookup_parameter_constant(
    const gl_program_parameter_list *l, const GLfloat v[], GLuint vSize,
    GLint *posOut, GLuint *swizzleOut)
{ return l ? l->lookup_parameter_constant(v, vSize, posOut, swizzleOut) : GL_FALSE; }

inline GLuint _mesa_longest_parameter_name(
    const gl_program_parameter_list *l, enum register_file type)
{ return l ? l->longest_parameter_name(type) : 0; }

inline GLuint _mesa_num_parameters_of_type(
    const gl_program_parameter_list *l, enum register_file type)
{ return l ? l->num_parameters_of_type(type) : 0; }



#endif /* PROG_PARAMETER_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
