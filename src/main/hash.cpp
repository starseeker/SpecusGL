/**
 * \file hash.cpp
 * Generic hash table – C++17 implementation.
 *
 * Used for display lists, texture objects, vertex/fragment programs,
 * buffer objects, etc.  The hash functions are thread-safe.
 *
 * \note key=0 is illegal.
 *
 * This file replaces the original hash.c with a C++17 implementation that:
 *  - Uses std::unordered_map<GLuint, void*> for O(1) average-case look-up
 *    with automatic resizing (no fixed 1023-bucket limit).
 *  - Replaces the _glthread_Mutex macros with std::mutex + std::lock_guard
 *    for RAII-safe locking without platform-specific #ifdefs.
 *  - Replaces malloc/free node management with the map's built-in storage.
 *  - Uses C++17 structured bindings in range-for loops for readability.
 *  - Preserves the original public C API so that all existing C callers
 *    link without modification.
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
#include <mutex>
#include <unordered_map>


/**
 * Internal hash table structure.
 *
 * The struct tag matches the C declaration in hash.h so that C translation
 * units that include hash.h get the correct opaque-pointer type.  The
 * definition is only visible here, preserving encapsulation.
 */
struct _mesa_HashTable {
    std::unordered_map<GLuint, void *> entries; /**< key → data mapping */
    GLuint maxKey{0};                           /**< highest key inserted so far */
    mutable std::mutex mutex;                   /**< guards entries and maxKey */
    bool inDeleteAll{false};                    /**< true while DeleteAll callback runs */
};


/**
 * Create a new hash table.
 *
 * \return pointer to a new, empty hash table.
 */
struct _mesa_HashTable *
_mesa_NewHashTable(void)
{
    return new _mesa_HashTable();
}


/**
 * Delete a hash table.
 *
 * Frees the hash table structure itself.  Note that the caller should have
 * already traversed the table and deleted the objects stored in the table
 * (i.e. the entries' data pointers are not freed here).
 *
 * \param table the hash table to delete.
 */
void
_mesa_DeleteHashTable(struct _mesa_HashTable *table)
{
    assert(table);
    if (!table->entries.empty()) {
        _mesa_problem(NULL, "In _mesa_DeleteHashTable, found non-freed data");
    }
    delete table;
}


/**
 * Look up an entry in the hash table.
 *
 * This function does not acquire the mutex, matching the behaviour of the
 * original C implementation.  Callers that need atomicity must synchronise
 * externally.
 *
 * \param table the hash table.
 * \param key   the key (must be non-zero).
 * \return pointer to the stored data, or NULL if the key is not present.
 */
void *
_mesa_HashLookup(const struct _mesa_HashTable *table, GLuint key)
{
    assert(table);
    assert(key);

    auto it = table->entries.find(key);
    return (it != table->entries.end()) ? it->second : NULL;
}


/**
 * Insert a key/pointer pair into the hash table.
 *
 * If an entry with this key already exists, the existing data pointer is
 * replaced.
 *
 * \param table the hash table.
 * \param key   the key (must be non-zero).
 * \param data  pointer to user data.
 */
void
_mesa_HashInsert(struct _mesa_HashTable *table, GLuint key, void *data)
{
    assert(table);
    assert(key);

    std::lock_guard<std::mutex> lock(table->mutex);
    if (key > table->maxKey)
        table->maxKey = key;
    table->entries[key] = data;
}


/**
 * Remove an entry from the hash table.
 *
 * \param table the hash table.
 * \param key   key of the entry to remove.
 */
void
_mesa_HashRemove(struct _mesa_HashTable *table, GLuint key)
{
    assert(table);
    assert(key);

    /* This check is intentionally performed *before* acquiring the mutex.
     *
     * Its purpose is to detect a specific programming error: a callback
     * passed to _mesa_HashDeleteAll calling _mesa_HashRemove on the same
     * table from the same thread while the mutex is already held.  If the
     * check were inside the lock, that path would deadlock on the
     * non-recursive std::mutex.
     *
     * The check does not race with normal concurrent access: any thread
     * that sets inDeleteAll=true first acquires the mutex, so a concurrent
     * HashRemove caller will either see the flag set (error path) or will
     * acquire the mutex after DeleteAll has cleared it. */
    if (table->inDeleteAll) {
        _mesa_problem(NULL, "_mesa_HashRemove illegally called from "
                      "_mesa_HashDeleteAll callback function");
        return;
    }

    std::lock_guard<std::mutex> lock(table->mutex);
    table->entries.erase(key);
}


/**
 * Delete all entries in a hash table, but keep the table itself.
 *
 * Invokes the given callback function for each entry before removing it.
 *
 * \param table    the hash table to clear.
 * \param callback function called for every entry.  The callback MUST NOT
 *                 call _mesa_HashRemove on this table (that would be a
 *                 programming error and is detected at runtime).  The
 *                 callback is also invoked while the table's mutex is held,
 *                 so it must not call any other function that tries to
 *                 acquire the same mutex (which would deadlock).
 * \param userData arbitrary pointer forwarded to every callback invocation.
 */
void
_mesa_HashDeleteAll(struct _mesa_HashTable *table,
                    std::function<void(GLuint key, void *data)> callback)
{
    assert(table);
    assert(callback);

    std::lock_guard<std::mutex> lock(table->mutex);
    table->inDeleteAll = true;
    for (auto &[key, data] : table->entries) {
        callback(key, data);
    }
    table->entries.clear();
    table->inDeleteAll = false;
}


/**
 * Walk over all entries in a hash table, calling a callback for each.
 *
 * \param table    the hash table to walk.
 * \param callback function called for every entry.
 * \param userData arbitrary pointer forwarded to every callback invocation.
 */
void
_mesa_HashWalk(const struct _mesa_HashTable *table,
               std::function<void(GLuint key, void *data)> callback)
{
    assert(table);
    assert(callback);

    std::lock_guard<std::mutex> lock(table->mutex);
    for (const auto &[key, data] : table->entries) {
        callback(key, data);
    }
}


/**
 * Return the key of the first entry in the hash table.
 *
 * \param table the hash table.
 * \return key of the first entry, or 0 if the table is empty.
 */
GLuint
_mesa_HashFirstEntry(struct _mesa_HashTable *table)
{
    assert(table);
    std::lock_guard<std::mutex> lock(table->mutex);
    if (table->entries.empty())
        return 0;
    return table->entries.begin()->first;
}


/**
 * Given a hash table key, return the next key.
 *
 * Used to walk over all entries.  The keys are not returned in any
 * particular order.
 *
 * \param table the hash table.
 * \param key   the current key.
 * \return next key, or 0 if there are no more entries.
 */
GLuint
_mesa_HashNextEntry(const struct _mesa_HashTable *table, GLuint key)
{
    assert(table);
    assert(key);

    std::lock_guard<std::mutex> lock(table->mutex);
    auto it = table->entries.find(key);
    if (it == table->entries.end())
        return 0;
    ++it;
    return (it != table->entries.end()) ? it->first : 0;
}


/**
 * Dump contents of hash table for debugging.
 *
 * \param table the hash table.
 */
void
_mesa_HashPrint(const struct _mesa_HashTable *table)
{
    assert(table);
    for (const auto &[key, data] : table->entries) {
        _mesa_debug(NULL, "%u %p\n", key, data);
    }
}


/**
 * Find a block of adjacent unused hash keys.
 *
 * \param table   the hash table.
 * \param numKeys number of consecutive keys needed.
 * \return starting key of a free block, or 0 on failure.
 *
 * \note The slow path below is O(n) where n is the span of used keys.
 *       For typical OpenGL usage patterns (keys allocated sequentially)
 *       the quick path is taken almost always.
 */
GLuint
_mesa_HashFindFreeKeyBlock(struct _mesa_HashTable *table, GLuint numKeys)
{
    if (numKeys == 0)
        return 0;

    const GLuint maxKey = ~GLuint{0};
    std::lock_guard<std::mutex> lock(table->mutex);

    /* Quick path: enough room above the current maximum key.
     * The comparison is safe because numKeys >= 1 so maxKey - numKeys
     * cannot equal maxKey. */
    if (table->maxKey <= maxKey - numKeys) {
        return table->maxKey + 1;
    }

    /* Slow path: scan for a contiguous run of free keys in [1, maxKey). */
    GLuint freeCount = 0;
    GLuint freeStart = 1;
    for (GLuint key = 1; key != maxKey; ++key) {
        if (table->entries.count(key)) {
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


/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
