# SpecusGL – Incremental C++17 Migration Plan

## Overview

The osmesa codebase is derived from Mesa 7.0.4 and consists of approximately
172 C source files implementing a software OpenGL 2.0 renderer.  The goal is
to migrate it to idiomatic C++17 while preserving its complete API and
behaviour.  Because of the scale of the work, the migration is staged into
discrete phases, each of which yields a buildable, testable library.

---

## Guiding Principles

1. **No functional regressions.**  Every phase must produce a library that
   passes all existing tests.
2. **Preserve the public C API.**  `OSMesa/osmesa.h`, `OSMesa/gl.h`, etc.
   remain unchanged so that existing C consumers compile without modification.
3. **One module at a time.**  Each phase targets a single, self-contained
   subsystem.  Mixed C/C++ compilation is supported by CMake and is the
   normal state during the transition.
4. **C linkage at boundaries.**  Any header shared between C translation units
   and new C++ translation units carries `extern "C"` guards so that name
   mangling does not break the link.
5. **Prefer the standard library.**  Replace bespoke data structures with
   `std::unordered_map`, `std::vector`, `std::string`, etc.  Replace manual
   mutex macros with `std::mutex` and `std::lock_guard`/`std::unique_lock`.
6. **RAII everywhere.**  Eliminate naked `malloc`/`free` in migrated code;
   use `new`/`delete` or smart pointers as appropriate.
7. **`[[nodiscard]]` and `static_assert`.**  Apply where they improve
   compile-time safety without touching unrelated code.

---

## Phase 1 – Build-system and infrastructure (this PR)

| Task | Status |
|------|--------|
| Enable `cxx_std_17` on the `osmesa` CMake target | ✅ done |
| Add `extern "C"` guards to shared headers as modules are migrated | ongoing |
| Migrate `src/main/hash.c` → `hash.cpp` | ✅ done |

### What changed in `hash.cpp`

The original `hash.c` implemented a fixed-size (1023-bucket) chained hash
table using hand-allocated `HashEntry` nodes and `_glthread_Mutex` macros.
The C++17 replacement:

* Uses `std::unordered_map<GLuint, void*>` for O(1) average-case look-up
  with automatic resizing.
* Replaces the `_glthread_Mutex` macros with a `mutable std::mutex` member,
  giving RAII locking via `std::lock_guard`.
* Replaces `malloc`/`free` node management with the map's built-in storage.
* Uses C++17 structured bindings (`auto& [key, data]`) in range-for loops
  for clarity.
* Replaces `GLboolean inDeleteAll` with `bool inDeleteAll`.
* The public C API (function signatures, opaque-pointer idiom) is unchanged.

---

## Phase 2 – Utility modules

Target files (roughly in dependency order):

* `src/main/imports.c` – Wrap `_mesa_align_malloc` / `_mesa_align_free` with
  `std::aligned_alloc` / `free`; remove the `MALLOC_STRUCT` / `CALLOC_STRUCT`
  macros in favour of `new` (done incrementally as callers are migrated).
* `src/main/debug.c` – Replace vararg `_mesa_debug` / `_mesa_problem` with
  variadic templates or `std::format` (C++20 if the toolchain supports it,
  otherwise `fmt`-style).
* `src/math/m_matrix.c` – Replace raw `GLfloat[16]` arrays with a thin
  `Matrix4f` value type; use `std::array<float, 16>` internally.
* `src/math/m_vector.c` – Wrap `GLfloat` arrays in `Vec3f`, `Vec4f` value
  types with operator overloads.

---

## Phase 3 – GL state and context

* `src/main/context.c` / `context.h` – Introduce a `GLContext` C++ class
  wrapping `GLcontext`; use member initialiser lists to replace
  `CALLOC_STRUCT` + manual zero-fill.
* `src/main/framebuffer.c`, `renderbuffer.c` – Convert reference-counted
  objects to `std::shared_ptr`.
* `src/main/hash.h` consumers – Replace `_mesa_HashTable*` pointer usage in
  the state structs with a type-safe `MesaHashTable<T>` template wrapper once
  all callers have been migrated to C++.

---

## Phase 4 – Shader / program subsystem

* `src/shader/program.c`, `prog_instruction.c`, `prog_parameter.c` – Replace
  dynamic arrays (hand-managed `realloc`) with `std::vector`.
* `src/shader/slang/` – The GLSL compiler (slang) is the most complex
  subsystem; migrate it last, wrapping the grammar and IR in proper C++
  classes with clear ownership semantics.

---

## Phase 5 – Rasteriser (swrast / tnl / vbo)

* Convert span-processing inner loops to use `std::span` (C++20 when
  available, otherwise a thin wrapper) for bounds-safe access.
* Replace `#ifdef`-heavy `CHAN_BITS` macros with explicit template
  specialisations over a `ColorChannel` type.
* Profile-guided SIMD: the software rasteriser benefits from explicit
  vectorisation hints; use `[[likely]]`/`[[unlikely]]` attributes and
  consider `std::execution` parallel policies for large pixel spans.

---

## Phase 6 – Driver layer

* `src/drivers/osmesa/osmesa.c` – This file can be migrated last.  Replace
  the `osmesa_context` struct with a class derived from a C++ `GLContext`
  base, benefiting from the earlier context-layer migration.
* The public `osmesa.h` API stays `extern "C"` permanently to preserve
  ABI compatibility with all existing consumers.

---

## Migration Checklist (by subsystem)

```
[ ] glapi/         - dispatch table generation
[ ] main/imports   - memory / math utilities
[ ] main/debug     - error reporting
[ ] main/hash      - ✅ done (Phase 1)
[ ] main/context   - GL context lifecycle
[ ] main/framebuffer + renderbuffer
[ ] main/teximage + texstore + texobj
[ ] main/bufferobj, arrayobj, varray
[ ] main/dlist     - display list (complex)
[ ] math/          - matrix and vector math
[ ] shader/        - ARB/NV programs, slang GLSL
[ ] swrast/        - software rasteriser
[ ] swrast_setup/
[ ] tnl/           - transform-and-light pipeline
[ ] vbo/           - vertex buffer objects
[ ] drivers/osmesa - OSMesa driver
```

---

## Notes on C / C++ interoperability

While the migration is in progress the library will contain both `.c` and
`.cpp` translation units.  The following rules apply:

* A header that is included from both C and C++ code must have `extern "C"`
  guards around all function declarations.
* Struct definitions visible to C code must not use C++-only features
  (constructors, templates, `std::` types) until all C consumers have been
  migrated.  Use the opaque-pointer pattern in the interim (already the
  case for `_mesa_HashTable`).
* The `INLINE` macro defined in `glheader.h` maps to `static __inline__` in
  C and `inline` in C++; no changes required.
