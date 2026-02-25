/**
 * \file hash.h
 * Generic hash table.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef HASH_H
#define HASH_H


#include "glheader.h"

#include <functional>
#include <mutex>
#include <unordered_map>


/**
 * Hash table with GLuint keys and void* values.
 *
 * All insert/remove operations are thread-safe via an internal mutex.
 * Lookup does not acquire the mutex to match the original Mesa behaviour.
 */
struct _mesa_HashTable {
    std::unordered_map<GLuint, void *> entries; /**< key → data mapping */
    GLuint maxKey{0};                           /**< highest key inserted so far */
    mutable std::mutex mutex;                   /**< guards entries and maxKey */
    bool inDeleteAll{false};                    /**< true during DeleteAll callback */

    _mesa_HashTable() = default;
    ~_mesa_HashTable() = default;

    /* Non-copyable */
    _mesa_HashTable(const _mesa_HashTable &) = delete;
    _mesa_HashTable & operator=(const _mesa_HashTable &) = delete;

    [[nodiscard]] void *lookup(GLuint key) const;
    void insert(GLuint key, void *data);
    void remove(GLuint key);
    void deleteAll(std::function<void(GLuint, void *)> callback);
    void walk(std::function<void(GLuint, void *)> callback) const;
    [[nodiscard]] GLuint firstEntry();
    [[nodiscard]] GLuint nextEntry(GLuint key) const;
    void print() const;
    [[nodiscard]] GLuint findFreeKeyBlock(GLuint numKeys);
};

extern void _mesa_test_hash_functions(void);


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
