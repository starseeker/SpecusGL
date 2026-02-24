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

* `src/main/imports.c` → **`imports.cpp`** ✅ done – Replaced platform-specific
  `posix_memalign` / manual-alignment fallback with `std::aligned_alloc` /
  `std::free` (size rounded up to a multiple of alignment as required by the
  standard).  Windows (`_aligned_malloc` / `_aligned_free`) path unchanged.
  Duplicate `#include "imports.h"` removed; `<cstdlib>` added.
* `src/main/debug.c` → **`debug.cpp`** ✅ done – Renamed to C++; `extern "C"`
  guards added to `debug.h` so C translation units continue to call the
  functions with C linkage.
* `src/math/m_matrix.c` → **`m_matrix.cpp`** ✅ done – Replaced `ALIGN_MALLOC`
  / `ALIGN_FREE` in the matrix constructor / destructor / `alloc_inv` with
  C++17 placement-new using `std::align_val_t{16}` and the matching
  `::operator delete[]` overload.  Duplicate `#include "imports.h"` removed;
  `<new>` added.  `extern "C"` guards added to `m_matrix.h`.
* `src/math/m_vector.c` → **`m_vector.cpp`** ✅ done – Replaced `ALIGN_MALLOC`
  / `ALIGN_FREE` in `_mesa_vector4f_alloc` / `_mesa_vector4f_free` with
  `std::aligned_alloc` / `std::free` (size rounded up).  Duplicate
  `#include "imports.h"` removed; `<cstdlib>` added.  `extern "C"` guards
  added to `m_vector.h`.

---

## Phase 3 – GL state and context (this PR)

| Task | Status |
|------|--------|
| Migrate `src/main/context.c` → `context.cpp` | ✅ done |
| Add `extern "C"` guards to `context.h` | ✅ done |
| Migrate `src/main/framebuffer.c` → `framebuffer.cpp` | ✅ done |
| Add `extern "C"` guards to `framebuffer.h` | ✅ done |
| Migrate `src/main/renderbuffer.c` → `renderbuffer.cpp` | ✅ done |
| Add `extern "C"` guards to `renderbuffer.h` | ✅ done |

### What changed in `context.cpp`

The original `context.c` used `calloc(1, sizeof(T))` / `free()` for allocating
`GLvisual`, `gl_shared_state`, and `GLcontext` objects.  The C++17 replacement:

* Uses value-initialising `new T{}` instead of `calloc` — this zero-initialises
  all members the same way `calloc` did, but uses the C++ free store so that
  `new`/`delete` are matched correctly.
* Replaces the corresponding `free()` calls with `delete`.
* `extern "C"` guards added to `context.h` so that C translation units
  continue to call the functions with C linkage.
* The `alloc_dispatch_table()` helper keeps `malloc`/`free` because it
  initialises the table with an explicit loop (no zero-fill required) and
  is freed from the same file.

### What changed in `framebuffer.cpp`

* `CALLOC_STRUCT(gl_framebuffer)` replaced with `new gl_framebuffer{}` in
  `_mesa_create_framebuffer` and `_mesa_new_framebuffer`.
* `free(fb)` in `_mesa_destroy_framebuffer` replaced with `delete fb`.
* `extern "C"` guards added to `framebuffer.h`.

### What changed in `renderbuffer.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* `extern "C"` guards added to `renderbuffer.h`.
* `CALLOC_STRUCT`/`free` kept for `gl_renderbuffer` objects because several
  not-yet-migrated C translation units (`depthstencil.c`, `texrender.c`,
  `osmesa.c`) call `free()` directly on renderbuffer pointers obtained from
  `_mesa_new_renderbuffer()`.  Converting to `new`/`delete` here requires
  migrating those files simultaneously (deferred to a later phase).

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
[x] main/imports   - memory / math utilities     ✅ Phase 2
[x] main/debug     - error reporting              ✅ Phase 2
[x] main/hash      - ✅ done (Phase 1)
[x] main/context   - GL context lifecycle         ✅ Phase 3
[x] main/framebuffer + renderbuffer               ✅ Phase 3
[ ] main/teximage + texstore + texobj
[ ] main/bufferobj, arrayobj, varray
[ ] main/dlist     - display list (complex)
[x] math/m_matrix  - matrix math                 ✅ Phase 2
[x] math/m_vector  - vector math                 ✅ Phase 2
[ ] math/          - remaining (m_translate, m_xform, m_eval, …)
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
