# SpecusGL – C++17 Modernization Plan

## Overview

This codebase is derived from Mesa 7.0.4, implementing a software OpenGL 2.0
renderer.  The goal is to migrate it to idiomatic C++17 while preserving its
complete API and behaviour.

An initial shift of the C code to C++ with minimal changes is now complete.
What we want to do next is start migrating the "C++" code to become more
idiomatic C++ rather than "technically C++" C code - i.e. take advantage
of C++ language features to make the code more readable, well organized,
less verbose, readily debuggable and performant.

---

## Guiding Principles

1. If any phases need large migrations across
   the code at once and require intermediate non-working states that is
   acceptable (for example, if we need to shift all memory management to
   C++ new/delete/class container style at the same time) but once those
   large shifts are done be sure to verify functionality and address any
   behavior regressions.
2. **Preserve the public C API.**  `OSMesa/osmesa.h`, `OSMesa/gl.h`, etc.
   remain unchanged so that existing C consumers compile without modification.
   Internal implementation code structure and organization should be updated to
   reflect modern best practice C++ design.
3. **Look for opportunities to collapse functions into methods on Classes.** Right now the logic flows
   of SpecusGL are designed around lots of individual functions passing
   parameters around, and functions are doing init and free work.  Let's
   see if we can (internally, while preserving the exposed public C API)
   migrate to a more C++-ish approach of having class objects with methods
   handle a lot of the bookkeeping and reduce the function/parameter passing
   complexity.  Ideally, we could also clean up some of the more complex
   parts of the codebase to be easier to understand and modify as well.  Don't
   do this just for the sake of doing it, but if there are code cleanliness
   advantages or structural/organizational improvements to be had prioritize those over
   simply replacing low level C operations in the code with their low level
   C++ equivalents.
3. **Prefer the standard library.**  Replace bespoke data structures with
   `std::unordered_map`, `std::vector`, `std::string`, etc.
4. **RAII everywhere.**  Eliminate naked `malloc`/`free` in migrated code;
   use `new`/`delete` or smart pointers as appropriate.
5. **`[[nodiscard]]` and `static_assert`.**  Apply where they improve
   compile-time safety without touching unrelated code.

