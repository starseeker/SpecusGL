/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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

#ifndef SLANG_UTILITY_H
#define SLANG_UTILITY_H

#include <string>
#include <cstring>
#include <cstdio>
#include <unordered_set>


inline int slang_string_compare(const char *str1, const char *str2) { return strcmp(str1, str2); }
inline char *slang_string_copy(char *dst, const char *src) { return strcpy(dst, src); }
inline size_t slang_string_length(const char *str) { return strlen(str); }

char *slang_string_concat(char *, const char *);

using slang_atom = const char *;
constexpr slang_atom SLANG_ATOM_NULL = nullptr;

using slang_string = std::string;

inline void slang_string_init(slang_string *s) { s->clear(); }
inline void slang_string_free(slang_string *s) { s->clear(); s->shrink_to_fit(); }
inline void slang_string_reset(slang_string *s) { s->clear(); }

inline void slang_string_push(slang_string *s, const slang_string *str)
{ s->append(*str); }

inline void slang_string_pushc(slang_string *s, const char c)
{ s->push_back(c); }

inline void slang_string_pushs(slang_string *s, const char *cstr, GLuint len)
{ s->append(cstr, static_cast<std::string::size_type>(len)); }

inline void slang_string_pushi(slang_string *s, GLint i)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", i);
    s->append(buf);
}

inline const char *slang_string_cstr(slang_string *s) { return s->c_str(); }

/* slang_atom */


/**
 * Atom pool – a string-interning table for the GLSL compiler.
 *
 * C++17 modernisation: the old fixed-size array of C-style linked lists
 * (backed by the slang mempool) has been replaced by
 * std::unordered_set<std::string>.  Iterators (and therefore the .c_str()
 * pointers inside each node) are not invalidated by insertion, so
 * previously-returned slang_atom values remain valid.
 */
struct slang_atom_pool {
    std::unordered_set<std::string> strings;
};

void slang_atom_pool_construct(slang_atom_pool *);
void slang_atom_pool_destruct(slang_atom_pool *);
slang_atom slang_atom_pool_atom(slang_atom_pool *, const char *);
const char *slang_atom_pool_id(slang_atom_pool *, slang_atom);




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
