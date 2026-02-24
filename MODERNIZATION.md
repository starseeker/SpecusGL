# SpecusGL – C++17 Modernization Plan

## Overview

The osmesa codebase is derived from Mesa 7.0.4 and consists of approximately
172 C source files implementing a software OpenGL 2.0 renderer.  The goal is
to migrate it to idiomatic C++17 while preserving its complete API and
behaviour.

An initial shift of the C code to C++ with minimal changes is now complete.
What we want to do next is start migrating the "C++" code to become more
idiomatic C++ rather than "technically C++" C code - i.e. take advantage
of C++ language features to make the code more readable, well organized,
less verbose, cleanly debuggable and performant.

---

## Guiding Principles

1. **No functional regressions.**  Every phase must produce a library that
   passes all existing tests.  If any phases need large migrations across
   the code at once and require intermediate non-working states that is
   acceptable (for example, if we need to shift all memory management to
   C++ new/delete/class container style at the same time) but once those
   large shifts are done be sure to verify functionality.
2. **Preserve the public C API.**  `OSMesa/osmesa.h`, `OSMesa/gl.h`, etc.
   remain unchanged so that existing C consumers compile without modification.
3. **Prefer the standard library.**  Replace bespoke data structures with
   `std::unordered_map`, `std::vector`, `std::string`, etc.  Replace manual
   mutex macros with `std::mutex` and `std::lock_guard`/`std::unique_lock`.
4. **RAII everywhere.**  Eliminate naked `malloc`/`free` in migrated code;
   use `new`/`delete` or smart pointers as appropriate.
5. **`[[nodiscard]]` and `static_assert`.**  Apply where they improve
   compile-time safety without touching unrelated code.

