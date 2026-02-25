/**
 * \file hash.cpp
 * Generic hash table – C++17 implementation.
 *
 * Used for display lists, texture objects, vertex/fragment programs,
 * buffer objects, etc.  The hash functions are thread-safe.
 *
 * \note key=0 is illegal.
 *
 * This file implements _mesa_HashTable with:
 *  - std::unordered_map<GLuint, void*> for O(1) average-case look-up.
 *  - std::mutex + std::lock_guard for RAII-safe locking.
 *  - Member methods exposed via inline wrappers in hash.h for backward compat.
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

#include "hash.h"
#include "imports.h"

#include <cassert>


/**
 * Look up an entry in the hash table (no mutex).
 */
void *
_mesa_HashTable::lookup(GLuint key) const
{
    assert(key);
    auto it = entries.find(key);
    return (it != entries.end()) ? it->second : nullptr;
}


/**
 * Insert a key/pointer pair into the hash table.
 */
void
_mesa_HashTable::insert(GLuint key, void *data)
{
    assert(key);
    std::lock_guard<std::mutex> lock(mutex);
    if (key > maxKey)
        maxKey = key;
    entries[key] = data;
}


/**
 * Remove an entry from the hash table.
 */
void
_mesa_HashTable::remove(GLuint key)
{
    assert(key);

    /* Detect illegal re-entrant removal from deleteAll callback. */
    if (inDeleteAll) {
        _mesa_problem(nullptr, "_mesa_HashRemove illegally called from "
                      "_mesa_HashDeleteAll callback function");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    entries.erase(key);
}


/**
 * Delete all entries, calling \a callback for each before removal.
 */
void
_mesa_HashTable::deleteAll(std::function<void(GLuint, void *)> callback)
{
    assert(callback);
    std::lock_guard<std::mutex> lock(mutex);
    inDeleteAll = true;
    for (auto &[key, data] : entries)
        callback(key, data);
    entries.clear();
    inDeleteAll = false;
}


/**
 * Walk over all entries, calling \a callback for each.
 */
void
_mesa_HashTable::walk(std::function<void(GLuint, void *)> callback) const
{
    assert(callback);
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto &[key, data] : entries)
        callback(key, data);
}


/**
 * Return the key of the first entry, or 0 if the table is empty.
 */
GLuint
_mesa_HashTable::firstEntry()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (entries.empty())
        return 0;
    return entries.begin()->first;
}


/**
 * Return the next key after \a key, or 0 if there are no more entries.
 */
GLuint
_mesa_HashTable::nextEntry(GLuint key) const
{
    assert(key);
    std::lock_guard<std::mutex> lock(mutex);
    auto it = entries.find(key);
    if (it == entries.end())
        return 0;
    ++it;
    return (it != entries.end()) ? it->first : 0;
}


/**
 * Print the hash table contents for debugging.
 */
void
_mesa_HashTable::print() const
{
    for (const auto &[key, data] : entries)
        _mesa_debug(nullptr, "%u %p\n", key, data);
}


/**
 * Find a block of \a numKeys adjacent unused keys starting from 1.
 *
 * \return starting key of a free block, or 0 on failure.
 */
GLuint
_mesa_HashTable::findFreeKeyBlock(GLuint numKeys)
{
    if (numKeys == 0)
        return 0;

    const GLuint kMax = ~GLuint{0};
    std::lock_guard<std::mutex> lock(mutex);

    /* Quick path: enough room above the current maximum key. */
    if (maxKey <= kMax - numKeys)
        return maxKey + 1;

    /* Slow path: scan for a contiguous run of free keys. */
    GLuint freeCount = 0;
    GLuint freeStart = 1;
    for (GLuint key = 1; key != kMax; ++key) {
        if (entries.count(key)) {
            freeCount = 0;
            freeStart = key + 1;
        } else {
            ++freeCount;
            if (freeCount == numKeys)
                return freeStart;
        }
    }
    return 0;
}


/**
 * Test the hash table functions - simple sanity check.
 */
void
_mesa_test_hash_functions(void)
{
    _mesa_HashTable *t = new _mesa_HashTable{};
    t->insert(501, reinterpret_cast<void *>(0xDEAD));
    t->insert(10, reinterpret_cast<void *>(0xBEEF));
    t->insert(0xFFFFFF00, reinterpret_cast<void *>(0xCAFE));
    _mesa_problem(nullptr, "hash_test: lookup(10) = %p",
		  t->lookup(10));
    _mesa_problem(nullptr, "hash_test: findFreeKeyBlock(100) = %u",
		  t->findFreeKeyBlock(100));
    delete t;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
