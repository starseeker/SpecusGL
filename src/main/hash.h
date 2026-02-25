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

/* Legacy C-style wrappers – prefer member functions for new code. */
[[nodiscard]] inline _mesa_HashTable *_mesa_NewHashTable() { return new _mesa_HashTable{}; }
inline void _mesa_DeleteHashTable(_mesa_HashTable *t) { delete t; }

[[nodiscard]] inline void *_mesa_HashLookup(const _mesa_HashTable *t, GLuint key)
{ return t->lookup(key); }

inline void _mesa_HashInsert(_mesa_HashTable *t, GLuint key, void *data)
{ t->insert(key, data); }

inline void _mesa_HashRemove(_mesa_HashTable *t, GLuint key)
{ t->remove(key); }

inline void _mesa_HashDeleteAll(_mesa_HashTable *t,
    std::function<void(GLuint, void *)> cb) { t->deleteAll(cb); }

inline void _mesa_HashWalk(const _mesa_HashTable *t,
    std::function<void(GLuint, void *)> cb) { t->walk(cb); }

[[nodiscard]] inline GLuint _mesa_HashFirstEntry(_mesa_HashTable *t)
{ return t->firstEntry(); }

[[nodiscard]] inline GLuint _mesa_HashNextEntry(const _mesa_HashTable *t, GLuint key)
{ return t->nextEntry(key); }

inline void _mesa_HashPrint(const _mesa_HashTable *t) { t->print(); }

[[nodiscard]] inline GLuint _mesa_HashFindFreeKeyBlock(_mesa_HashTable *t, GLuint n)
{ return t->findFreeKeyBlock(n); }

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
