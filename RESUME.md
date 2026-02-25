# SpecusGL Modernization – Resume Notes

## Session Summary (copilot/modernize-codebase-one-more-time)

All changes build cleanly and all 55 tests pass.

### Completed This Session

1. **`tnl_pipeline_stage` RAII** (`t_context.h`, `t_pipeline.cpp`, all `t_vb_*.cpp`)
   - Replaced `void (*destroy)` callback with `void (*privateDeleter)(void*)` typed deleter
   - Deleter is called automatically by `_tnl_destroy_pipeline()` – no more per-stage boilerplate
   - Replaced `_mesa_vector4f_alloc()` calls with `.alloc()` member method

2. **VBO buffers → `std::vector`** (`vbo_split_copy.cpp`, `vbo_split_inplace.cpp`, `vbo_rebase.cpp`)
   - Raw owned `translated_elt_buf`, `dstbuf`, `dstelt`, `elts`, `tmp_prims` → `std::vector`

3. **`RENDERINPUTS` macros → `std::bitset<_TNL_ATTRIB_MAX>`** (`t_context.h`)
   - Fixed latent OOB bug (BITSET64 macros accessed [0],[1] but only 1 word was allocated for ATTRIB_MAX=32)
   - Added `renderinputs_test_range()` helper replacing `BITSET64_TEST_RANGE`

4. **`gl_attrib_entry` RAII** (`mtypes.h`, `attrib.cpp`)
   - Changed from `std::pair<GLbitfield, void*>` to a proper owning struct with `void(*deleter)(void*)`
   - Added move semantics, deleted copy
   - Eliminated `free_attrib_data()` dispatch function
   - `pop_back()` in PopAttrib/PopClientAttrib now frees automatically via destructors

5. **`tnl_vp_cache` → `unique_ptr`** (`t_vp_build.h`, `t_vp_build.cpp`, `t_context.h`)
   - Moved `state_key`, `StateKeyHash`, `StateKeyEqual`, `tnl_vp_cache` from `t_vp_build.cpp` → `t_vp_build.h`
   - Changed `TNLcontext::vp_cache` from raw `tnl_vp_cache *` to `std::unique_ptr<tnl_vp_cache>`
   - `_tnl_ProgramCacheInit` uses `std::make_unique<tnl_vp_cache>()`
   - `_tnl_ProgramCacheDestroy` uses `.reset()`

---

## High-Priority Remaining Work

### A. `gl_program::Instructions` raw array → `std::vector<prog_instruction>`

**Impact**: High. Eliminates `_mesa_alloc_instructions()`, `_mesa_realloc_instructions()`,
`_mesa_copy_instructions()`, and all `delete[] prog->Instructions` calls across ~55 sites.

**Affected files** (main changes needed):
- `src/main/mtypes.h` – change `struct prog_instruction *Instructions` to `std::vector<prog_instruction> Instructions`
- `src/main/mtypes.h` – remove `GLuint NumInstructions` field (becomes `Instructions.size()`)
- `src/shader/prog_instruction.h/.cpp` – remove `_mesa_alloc_instructions`, `_mesa_realloc_instructions`, `_mesa_copy_instructions` functions
- `src/shader/program.cpp` – update `_mesa_delete_program` (no more `delete[] Instructions`), update `_mesa_clone_program`
- `src/shader/programopt.cpp` – update both optimization passes
- `src/shader/slang/slang_emit.cpp` – heavy user of `_mesa_realloc_instructions`; change to `emplace_back()`
- `src/shader/slang/slang_link.cpp`, `slang_label.cpp` – update `Instructions + i` → `&Instructions[i]`
- `src/shader/arbprogparse.cpp` – update alloc/realloc pattern
- `src/main/texenvprogram.cpp` – update alloc pattern
- `src/tnl/t_vp_build.cpp` – update alloc pattern
- `src/shader/prog_print.cpp` – update pointer arithmetic

**Key patterns to change**:
```cpp
// Before:
prog->Instructions = _mesa_alloc_instructions(n);
prog->NumInstructions = n;
// After:
prog->Instructions.assign(n, prog_instruction{});

// Before:
prog->Instructions = _mesa_realloc_instructions(prog->Instructions,
    prog->NumInstructions, prog->NumInstructions + 1);
inst = prog->Instructions + prog->NumInstructions;
prog->NumInstructions++;
// After:
prog->Instructions.emplace_back();
inst = &prog->Instructions.back();

// Before:
delete[] prog->Instructions;
// After: automatic (vector destructor)

// Before:
prog->Instructions + i   (pointer arithmetic)
// After:
prog->Instructions.data() + i   OR   &prog->Instructions[i]

// Before:
if (prog->Instructions) { ... }  (null check)
// After:
if (!prog->Instructions.empty()) { ... }

// Before:
prog->NumInstructions
// After:
static_cast<GLuint>(prog->Instructions.size())
```

**Risk**: Medium-High. The `slang_emit.cpp` file grows the array one instruction at a time,
so the `emplace_back()` conversion is straightforward but requires care. The `arbprogparse.cpp`
pre-allocates `MAX_INSTRUCTIONS` (1000) and then uses a subset; with vector, resize to 1000
up front then shrink after parsing.

### B. `ati_fragment_shader::Instructions/SetupInst` → vector or fixed array

**Current**: `struct atifs_instruction *Instructions[2]` (raw pointers, freed manually)
**Proposed**: `std::array<std::vector<atifs_instruction>, 2> Instructions`

Requires moving `atifs_instruction` and `atifs_setupinst` struct definitions before
`ati_fragment_shader` in `mtypes.h` (they are currently forward-declared). The atifragshader.h
header defines them; include it in mtypes.h or inline the definitions.

**Files**: `mtypes.h`, `atifragshader.cpp`, `s_atifragshader.cpp`

### C. Further function-to-method collapses

- `_mesa_reference_texobj()` / `_mesa_reference_program()` – could become methods on the
  texture/program objects, reducing the double-pointer-update idiom
- `gl_shine_tab` manual refcount – the `_ShineTable[2]` raw pointer array with manual
  `refcount` decrement could use `shared_ptr` or a slot-based pool with intrusive counting

### D. Display list `Node` block chain

The `mesa_display_list::node` uses a C-style linked-block structure
(`OPCODE_CONTINUE` nodes chain blocks). This is a complex refactor but would benefit
from a `std::vector<std::vector<Node>>` or similar managed structure that eliminates
the manual `new Node[BLOCK_SIZE]` + `delete[]` pattern in `dlist.cpp`.

---

## How to Continue

1. Clone or checkout `copilot/modernize-codebase-one-more-time`
2. Build with `cmake --build build -j4`; confirm 55 tests pass
3. Start with item **A** above (`gl_program::Instructions`)—it has the highest impact
4. After each file group is updated, rebuild and run tests
5. Commit frequently using `report_progress`
