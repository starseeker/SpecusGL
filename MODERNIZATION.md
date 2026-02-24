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
3. **Prefer the standard library.**  Replace bespoke data structures with
   `std::unordered_map`, `std::vector`, `std::string`, etc.
4. **RAII everywhere.**  Eliminate naked `malloc`/`free` in migrated code;
   use `new`/`delete` or smart pointers as appropriate.
5. **`[[nodiscard]]` and `static_assert`.**  Apply where they improve
   compile-time safety without touching unrelated code.

