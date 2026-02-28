/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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
 * \file bitset.h
 * \brief Type-safe C++17 bitset utilities.
 * \author Michal Krol (original C macro implementation)
 *
 * C++17 modernisation: the original C-macro bitsets (BITSET_DECLARE /
 * BITSET64_DECLARE and friends) have been superseded by std::bitset<N>.
 * See src/tnl/t_context.h for the canonical usage pattern.
 *
 * This header retains the file for include-compatibility and pulls in
 * <bitset> so that every translation unit that was relying on bitset.h
 * to provide the standard header continues to compile.
 */

#ifndef MESA_BITSET_H
#define MESA_BITSET_H

#include <bitset>
#include <cstddef>

/**
 * Type alias for a bitset of exactly N bits.
 *
 * Use this in new code instead of the old BITSET_DECLARE / BITSET64_DECLARE
 * macros.  std::bitset<N> is zero-initialised by its default constructor,
 * supports all the bit-manipulation operations the old macros provided, and
 * does so in a type-safe, bounds-checked (in debug builds) manner.
 *
 * Example – replacing the old BITSET_DECLARE pattern:
 * \code
 *   // Old (C macro style):
 *   BITSET_DECLARE(my_flags, 32);
 *   BITSET_SET(my_flags, 5);
 *   if (BITSET_TEST(my_flags, 5)) { ... }
 *
 *   // New (C++17):
 *   mesa::Bitset<32> my_flags;
 *   my_flags.set(5);
 *   if (my_flags.test(5)) { ... }
 * \endcode
 */
namespace mesa {
    template <std::size_t N>
    using Bitset = std::bitset<N>;
} /* namespace mesa */

#endif /* MESA_BITSET_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
