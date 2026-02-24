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
| Add `extern "C"` guards to shared headers as modules are migrated | ✅ done |
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
* `src/math/m_translate.c` → **`m_translate.cpp`** ✅ done – Renamed to C++17;
  no memory-management changes required (no heap allocation in this file).
  `extern "C"` guards added to `m_translate.h`.
* `src/math/m_xform.c` → **`m_xform.cpp`** ✅ done – Renamed to C++17; no
  memory-management changes required.  `extern "C"` guards added to
  `m_xform.h`.
* `src/math/m_eval.c` → **`m_eval.cpp`** ✅ done – Renamed to C++17; no
  memory-management changes required.  `extern "C"` guards added to
  `m_eval.h`.

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

| Task | Status |
|------|--------|
| Migrate `src/shader/prog_instruction.c` → `prog_instruction.cpp` | ✅ done |
| Add `extern "C"` guards to `prog_instruction.h` | ✅ done |
| Migrate `src/shader/prog_parameter.c` → `prog_parameter.cpp` | ✅ done |
| Add `extern "C"` guards to `prog_parameter.h` | ✅ done |
| Migrate `src/shader/program.c` → `program.cpp` | ✅ done |
| Add `extern "C"` guards to `program.h` | ✅ done |

### What changed in `prog_instruction.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* `extern "C"` guards added to `prog_instruction.h`.
* `calloc`/`_mesa_realloc` kept in `_mesa_alloc_instructions` and
  `_mesa_realloc_instructions` because not-yet-migrated C translation units
  (`nvvertparse.c`, `nvfragparse.c`, `arbprogparse.c`, `programopt.c`,
  `slang/slang_emit.c`) call `free()` directly on instruction arrays obtained
  from these functions.

### What changed in `prog_parameter.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* `CALLOC_STRUCT(gl_program_parameter_list)` in `_mesa_new_parameter_list`
  replaced with `new gl_program_parameter_list{}`.
* `free(paramList)` in `_mesa_free_parameter_list` replaced with
  `delete paramList`.
* The internal `Parameters` and `ParameterValues` arrays retain
  `_mesa_realloc` / `_mesa_align_realloc` management because they are raw
  pointers in a struct whose layout is shared with not-yet-migrated C
  translation units.
* Implicit `int` → `gl_state_index` enum conversion in `_mesa_add_attribute`
  made explicit with a cast (required by C++ but not C).
* `extern "C"` guards added to `prog_parameter.h`.

### What changed in `program.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* `CALLOC_STRUCT(gl_vertex_program)` and `CALLOC_STRUCT(gl_fragment_program)`
  in `_mesa_new_program` replaced with `new gl_vertex_program{}` /
  `new gl_fragment_program{}`.
* `free(prog)` in `_mesa_delete_program` replaced with type-aware `delete`:
  the existing `GL_VERTEX_PROGRAM_ARB` check is reused to cast `prog` back
  to its concrete type before deleting, since `Base` is the first member of
  both subtypes.
* `extern "C"` guards added to `program.h`.

* `src/shader/slang/` – The GLSL compiler (slang) is the most complex
  subsystem; migrate it last, wrapping the grammar and IR in proper C++
  classes with clear ownership semantics.

---

## Phase 5 – Remaining math debug modules

| Task | Status |
|------|--------|
| Migrate `src/math/m_debug_clip.c` → `m_debug_clip.cpp` | ✅ done |
| Migrate `src/math/m_debug_norm.c` → `m_debug_norm.cpp` | ✅ done |
| Migrate `src/math/m_debug_xform.c` → `m_debug_xform.cpp` | ✅ done |
| Add `extern "C"` guards to `m_debug.h` | ✅ done |

### What changed in `m_debug_clip.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* No memory-management changes required (no heap allocation in this file).
* `extern "C"` guards added to `m_debug.h`.

### What changed in `m_debug_norm.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* `ALIGN_MALLOC(16 * sizeof(GLfloat), 16)` in `test_norm_function` replaced
  with `new (std::align_val_t{16}) GLfloat[16]`.
* `ALIGN_FREE(mat->m)` replaced with
  `::operator delete[](mat->m, std::align_val_t{16})`.
* `#include <new>` added for `std::align_val_t`.

### What changed in `m_debug_xform.cpp`

* Renamed to `.cpp` so the translation unit is compiled as C++17.
* `ALIGN_MALLOC(16 * sizeof(GLfloat), 16)` in `test_transform_function`
  replaced with `new (std::align_val_t{16}) GLfloat[16]`.
* `ALIGN_FREE(mat->m)` replaced with
  `::operator delete[](mat->m, std::align_val_t{16})`.
* `#include <new>` added for `std::align_val_t`.

---

## Phase 6 – Rasteriser (swrast / tnl / vbo)

| Task | Status |
|------|--------|
| Migrate `src/glapi/glapi.c` → `glapi.cpp` | ✅ done |
| Migrate `src/glapi/glthread.c` → `glthread.cpp` | ✅ done |
| Add `extern "C"` guards to `glapi.h`, `glthread.h` | ✅ done |
| Migrate all 29 `src/swrast/s_*.c` files → `.cpp` | ✅ done |
| Add `extern "C"` guards to all `swrast/` headers | ✅ done |
| Migrate `src/swrast_setup/ss_context.c` → `ss_context.cpp` | ✅ done |
| Migrate `src/swrast_setup/ss_triangle.c` → `ss_triangle.cpp` | ✅ done |
| Add `extern "C"` guards to `swrast_setup/` headers | ✅ done |
| Migrate all 16 `src/tnl/t_*.c` files → `.cpp` | ✅ done |
| Add `extern "C"` guards to `tnl/` headers | ✅ done |
| Migrate all 14 `src/vbo/vbo_*.c` files → `.cpp` | ✅ done |
| Add `extern "C"` guards to `vbo/` headers | ✅ done |

### What changed in `glapi.cpp`, `glthread.cpp`

* Renamed to `.cpp`; no code changes required (both compile clean as C++17).
* `extern "C"` guards added to `glapi.h` and `glthread.h`.

### What changed in `swrast/` (29 files)

* All 29 `s_*.c` files renamed to `.cpp`; no code changes required
  (all compile clean as C++17).
* `extern "C"` guards added to all 21 public swrast headers.

### What changed in `ss_context.cpp`

* Renamed to `.cpp`.
* In the `EMIT_ATTR` macro, `map[e].format = (STYLE)` replaced with
  `map[e].format = static_cast<tnl_attr_format>(STYLE)` because the
  format field is an enum but `STYLE` may be a plain `GLint` at call sites.
* `extern "C"` guards added to `ss_context.h`, `ss_triangle.h`,
  `swrast_setup.h`.

### What changed in `tnl/` (16 files)

* All 16 `t_*.c` files renamed to `.cpp`.
* `t_draw.cpp`: `malloc()` return cast to `GLubyte*`; implicit `const void*`
  → `const GLubyte*` conversion in `_tnl_import_array()` call made explicit
  with `static_cast`.
* `t_vp_build.cpp`: five `tokens[i] = si` assignments (where `tokens` is
  `gl_state_index[]` and `si` is `GLint`) each wrapped in
  `static_cast<gl_state_index>()`.
* Remaining 14 files: renamed only; no code changes.
* `extern "C"` guards added to `t_context.h`, `t_pipeline.h`, `t_vertex.h`,
  `t_vp_build.h`, `tnl.h`.

### What changed in `vbo/` (14 files)

* All 14 `vbo_*.c` files renamed to `.cpp`.
* `vbo_context.cpp`: three `cl->Ptr = (const void *)...` casts changed to
  `static_cast<const GLubyte*>(...)`.
* `vbo_rebase.cpp`: `malloc()` return in the `REBASE` macro cast to
  `TYPE*` via `static_cast`.
* `vbo_exec_api.cpp`: `ALIGN_MALLOC` return cast to `GLubyte*`.
* `vbo_exec_draw.cpp`: `(void *)data` cast changed to
  `static_cast<const GLubyte*>(data)`.
* `vbo_save_draw.cpp`: `MapBuffer()` return cast to `const char*`.
* `vbo_split_copy.cpp`: four `malloc()` returns cast to the appropriate
  pointer types.
* `vbo_split_inplace.cpp`: `malloc()` return cast to `GLuint*`.
* `extern "C"` guards added to `vbo.h`, `vbo_context.h`, `vbo_exec.h`,
  `vbo_save.h`, `vbo_split.h`.

---

## Phase 7 – Remaining main/, shader/, and driver files

| Task | Status |
|------|--------|
| Migrate all remaining 55 `src/main/*.c` files → `.cpp` | ✅ done |
| Add `extern "C"` guards to all `main/` headers | ✅ done |
| Migrate all remaining `src/shader/*.c` files → `.cpp` | ✅ done |
| Migrate all `src/shader/slang/*.c` files → `.cpp` | ✅ done |
| Migrate `src/shader/grammar/grammar_mesa.c` → `.cpp` | ✅ done |
| Add `extern "C"` guards to all `shader/` and `slang/` headers | ✅ done |
| Migrate `src/drivers/common/driverfuncs.c` → `.cpp` | ✅ done |
| Migrate `src/drivers/osmesa/osmesa.c` → `.cpp` | ✅ done |
| Add `extern "C"` guards to `drivers/` headers | ✅ done |
| Migrate `src/fxaa/fxaa_cpu.c` → `.cpp` | ✅ done |
| Add `extern "C"` guards to `fxaa/` headers | ✅ done |

### What changed in `src/main/` (55 files)

* All 55 remaining `main/*.c` files renamed to `.cpp`.
* `api_validate.cpp`: `MapBuffer()` void\* return cast to `const GLubyte*`.
* `enums.cpp`: two `0xFFFFFFFF` integer literals (unsigned) cast with
  `static_cast<int>(0xFFFFFFFFu)` to suppress narrowing-conversion errors.
* `image.cpp`: two `const GLvoid*` → `const GLubyte*` assignments wrapped
  in `static_cast<const GLubyte*>()`.
* `light.cpp`: array initializer `{0.0}` corrected to `{0}` (narrowing
  double→int).
* `texenvprogram.cpp`: `~0,` bitmask changed to `~0u,`; five
  `tokens[i] = si` assignments wrapped in `static_cast<gl_state_index>()`.
* Remaining 50 files: renamed only; no code changes required.
* `extern "C"` guards added to all 53 public `main/` headers.

### What changed in `src/shader/` (10 non-slang files)

* `arbprogram.c`, `atifragshader.c`, `nvfragparse.c`, `nvprogram.c`,
  `nvvertparse.c`, `prog_execute.c`, `prog_print.c`, `prog_statevars.c`,
  `shader_api.c`: renamed to `.cpp`; no code changes required.
* `programopt.cpp`: `static_cast<gl_state_index>()` applied to all integer
  literals in the `mvpState`, `fogPStateOpt`, and `fogColorState` array
  initializers (23 error sites).
* `arbprogparse.cpp`: `static_cast<gl_state_index>()` applied to all
  `GLint`/`GLuint` assignments into `state_tokens[]` arrays (18 error sites),
  and the zero-initializer `{0,0,0,0,0}` of a local `gl_state_index[]`
  variable wrapped with explicit enum casts.
* `grammar/grammar_mesa.c`: renamed to `.cpp`; no code changes required.
* `extern "C"` guards added to all `shader/` headers.

### What changed in `src/shader/slang/` (17 files)

* 12 files compiled clean and were renamed to `.cpp` without modification:
  `slang_codegen`, `slang_compile`, `slang_compile_function`,
  `slang_compile_operation`, `slang_compile_struct`,
  `slang_compile_variable`, `slang_ir`, `slang_library_noise`,
  `slang_link`, `slang_log`, `slang_mem`, `slang_preprocess`,
  `slang_print`, `slang_storage`, `slang_typeinfo`, `slang_utility`,
  `slang_vartable`.
* `slang_builtin.cpp`: `static_cast<gl_state_index>()` applied throughout
  the `matrices[]` struct initializer (zero `modifier` fields), the
  `tokens[i] = 0` loop initializer, and all `tokens[n] = index1/0/1`
  assignments (28 error sites).
* `slang_emit.cpp`: sentinel `{ 0, 0 }` in the `operators[]` array
  (field type `gl_inst_opcode`) replaced with
  `{ static_cast<gl_inst_opcode>(0), static_cast<gl_inst_opcode>(0) }`.
* `slang_label.cpp`: `_slang_realloc()` void\* return cast to `GLuint*`.
* `slang_simplify.cpp`: `GLint value[16] = {-1.0}` corrected to
  `{-1}` (narrowing double→int).
* `extern "C"` guards added to all `slang/` headers.

### What changed in `src/drivers/` and `src/fxaa/`

* `driverfuncs.c`, `osmesa.c`, `fxaa_cpu.c`: renamed to `.cpp`; no code
  changes required (all compile clean as C++17).
* `extern "C"` guards added to all `drivers/` and `fxaa/` headers.

---

## Migration Checklist (by subsystem)

```
[x] glapi/         - dispatch table generation          ✅ Phase 6
[x] main/imports   - memory / math utilities            ✅ Phase 2
[x] main/debug     - error reporting                    ✅ Phase 2
[x] main/hash      - hash table                         ✅ Phase 1
[x] main/context   - GL context lifecycle               ✅ Phase 3
[x] main/framebuffer + renderbuffer                     ✅ Phase 3
[x] main/teximage + texstore + texobj                   ✅ Phase 7
[x] main/bufferobj, arrayobj, varray                    ✅ Phase 7
[x] main/dlist     - display list                       ✅ Phase 7
[x] math/m_matrix  - matrix math                        ✅ Phase 2
[x] math/m_vector  - vector math                        ✅ Phase 2
[x] math/m_translate                                    ✅ Phase 2
[x] math/m_xform                                        ✅ Phase 2
[x] math/m_eval                                         ✅ Phase 2
[x] math/          - m_debug_clip, m_debug_norm,
                     m_debug_xform                      ✅ Phase 5
[x] shader/program + prog_instruction + prog_parameter  ✅ Phase 4
[x] shader/        - remaining ARB/NV parsers           ✅ Phase 7
[x] shader/slang/  - GLSL compiler                      ✅ Phase 7
[x] swrast/        - software rasteriser                ✅ Phase 6
[x] swrast_setup/                                       ✅ Phase 6
[x] tnl/           - transform-and-light pipeline       ✅ Phase 6
[x] vbo/           - vertex buffer objects              ✅ Phase 6
[x] drivers/osmesa - OSMesa driver                      ✅ Phase 7
[x] drivers/common - common driver helpers              ✅ Phase 7
[x] fxaa/          - FXAA post-processing               ✅ Phase 7
```

**All 172 C source files have been migrated to C++17.**
The `osmesa` shared library builds cleanly with `cmake --build`.

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
