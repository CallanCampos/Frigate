# The sanitized build

Frigate's sanitized build runs the game and every test under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer
(UBSan), and stops at the first report: an access outside an object, the stack or one of the game's own heap blocks,
a use after free, undefined arithmetic, a misaligned access, a call through the wrong function type. It is level T5
of the tests ([PLAN.md §6](PLAN.md#6-verification)), and phase 5's "sanitizer-clean replays" are its routes run clean
([PLAN.md §7](PLAN.md#phase-5-the-whole-game-hardened)). This page says how to run it, how to read what it reports,
what Frigate tells the sanitizers so that they see the game as it runs on fibers and in its own heaps, the few checks
it leaves out and why, and what it has found.

## Running it

```sh
cmake -S port -B build/port-san -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT_SANITIZE=ON
cmake --build build/port-san                        # build/port-san/frigate-black2, every test program
ctest --test-dir build/port-san -j4 --output-on-failure        # every test, sanitized
ctest --test-dir build/port-san -L route -LE oracle            # the routes that need no oracle
ctest --test-dir build/port-san -L route                       # every route, with the oracle in build/port-san/oracle
```

- **White 2** is `-DPORT_VERSION=w2_us` in a directory of its own (`build/port-san-w2`).
- **The routes** are the tests labelled `route`: the program runs the game from power-on on the player's ROM (the
  replays, the oracle's sequences and the port's checks of their hashes, the test launcher's routes, the smoke test).
  The oracle's need its program and the ROM build's link map, as in the plain build
  ([../tests/README.md](../tests/README.md)); without the ROM they skip, as in CI.
- **One route by hand:** `build/port-san/frigate-black2 --headless --frames 4300 --watchdog 60 --save-dir
  /tmp/san-saves --input port/tests/replay/start_menu.pbwinput`. A run that hits a report ends with SIGABRT, status 134
  (a test that runs the program fails).
- **The compiler** is the one CMake finds, Apple clang on macOS. Apple clang emits no function check for C
  (`-fsanitize=function`, in `undefined` since upstream clang 17) and its runtime has no LeakSanitizer; upstream clang
  has both: `-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang` on macOS, with `ASAN_OPTIONS=detect_leaks=1` for
  the leak check at exit, which Linux runs by default.
- **CI** builds the sanitized Black 2 on macOS and Linux on every push and pull request (the `sanitize` job of
  `.github/workflows/port-ci.yml`) and runs every test that isn't a route, then the routes that need no oracle, which
  skip there since CI has no ROM.
- **A survey** of every report of a route, rather than the first: `-DPORT_SANITIZE_RECOVER=ON` in another build
  directory makes every check go on after a report, and ASan's `halt_on_error=0` the default
  ([../hal/os/sanitizers.c](../hal/os/sanitizers.c)). ASan reports each place once (`suppress_equal_pcs`), as UBSan
  does, so a place that two callers reach shows the first; fix and run again until a run is clean.
  `ASAN_OPTIONS=log_path=DIR/san` and `UBSAN_OPTIONS=log_path=DIR/san` keep the reports of programs whose output a
  test captures.

## Reading a report

- **ASan** prints `ERROR: AddressSanitizer: <kind>`, the access and its stack, through the fibers down to `FiberMain`
  and `_mco_main` for the game's threads. `use-after-poison` is an access to the game's heap memory outside every
  block allocated there (between blocks, in FND's block heads, in free memory, or in a block already freed);
  `heap-buffer-overflow` and `heap-use-after-free` are the host's own `malloc`'s; `stack-*` and `global-*` the others.
  The "allocated by" stack of a `use-after-poison` is the 16 MiB arena all the game's heaps live in
  (`port_pxi_alloc_arena`), not the block: find the block from the code that accessed it.
- **UBSan** prints `port: UndefinedBehaviorSanitizer: <check> at FILE:LINE:COLUMN, from this stack:`, the stack, then
  its own `runtime error:` line. The stack is ASan's: UBSan's own (`UBSAN_OPTIONS=print_stacktrace=1`) stops at the
  report's frame on a fiber.
- **A trap** (status 70) or the watchdog (73) end a sanitized run as they end a plain one.

## What Frigate tells the sanitizers

| What | Where | Why |
| --- | --- | --- |
| Every switch between fibers and the host stack | [../hal/os/fiber.c](../hal/os/fiber.c) | ASan must know which stack runs, or its stack checks and unwinds go wrong; minicoro's own annotations miss the switch back to the host stack, so [../hal/os/mco_impl.c](../hal/os/mco_impl.c) is built without ASan and `fiber.c` annotates instead |
| A dying fiber's last switch, and a fiber destroyed while suspended | `fiber.c` (`FiberMain`, `FreeFakeStack`) | So that ASan frees the fiber's fake stack (`detect_stack_use_after_return=1`, which `os_all_fake_stacks` turns on); a fiber destroyed while suspended once kept its own |
| The live frames and saved registers of suspended fibers | `fiber.c` (`ScanStackForLeaks`), with the saved stack pointer from `mco_impl.c` | LeakSanitizer scans threads' stacks, not fibers': an allocation only a suspended fiber holds would be a leak. Only the live frames, from the saved stack pointer up, so that the dead frames under them don't hide one |
| UBSan's reports, with ASan's stack | [../hal/os/sanitizers.c](../hal/os/sanitizers.c) (`__ubsan_on_report`) | UBSan unwinds only within the host thread's own stack, so its trace stopped at the report's frame on a fiber |
| The game's expanded and frame heaps, block by block | [../hal/nnsys/fnd_hooks.c](../hal/nnsys/fnd_hooks.c), with FND's heaps built without ASan ([../cmake/nnsys.cmake](../cmake/nnsys.cmake)) | Every GFL heap and the sound heap live inside one 16 MiB arena, which ASan sees as one block: an overrun from one of the game's allocations into the next was invisible. A new heap's memory is poisoned, a block made addressable for the bytes the DS gives it (the size asked for, rounded up to 4) when it is allocated, and poisoned when it is freed; a destroyed heap's memory is its owner's again |
| The trap's data placeholders at exit | [../hal/trap.c](../hal/trap.c) | LeakSanitizer scans the program's writable data by the protection it was loaded with, so it read the placeholders' inaccessible pages at exit, and its check never ended. They become readable at exit, before the check |
| The overlays' sections | [../hal/fs/ovsec.c](../hal/fs/ovsec.c) | Its snapshot and reset copy whole sections, ASan's redzones between the statics included, in loops without ASan |
| Another fiber's stack | `fiber.c` (`port_fiber_stack_used`) | Read without ASan, since dead frames there may still be poisoned |
| The asm oracle's libraries | [../tests/asm_oracle/build.py](../tests/asm_oracle/build.py) | They load into a Python without a sanitizer runtime, so they are built without the sanitizers' options |

## What it leaves out, and why

[../cmake/sanitizer_ignorelist.txt](../cmake/sanitizer_ignorelist.txt) names, for clang, the checks that the build
doesn't emit in a few of the game's functions: each is code that is right on every host Frigate runs on and that only
C's rules call undefined, named as narrowly as clang allows, by the check and the function. A defect is never listed
there: it is fixed (below).

| Check | Function | Why it is safe |
| --- | --- | --- |
| `alignment` | `ApplyCurrentAnimResult_` (`lib/nnsys/src/g2d/g2d_CellAnimation.c`), `SetCurrentFrameMultiCell_` (`g2d_MultiCellAnimation.c`) | NitroSystem reads an animation frame's cell index, a `u16` and the first field of every element type, through a pointer to the largest type, `NNSG2dAnimDataSRT`, which needs 4-byte alignment. An index-only animation's NANR file packs 2-byte elements, so the element is 2-aligned: a 2-byte load from a 2-aligned address, which arm64 and x86-64 make (and the DS's ARM9) |
| `function` | `runSaveBlkInitializers` (`src/gfl/savedata.c`) | Each save block's initializer is called through `SaveBlockDef.init`, a `void (*)(void *)`, to which `src/system/save_table.c` casts the 56 initializers, each taking a pointer to its own block. The call passes one pointer, in the same register whatever its type, in every host ABI. Fixing it would take a wrapper for each of the 56 in a game file, for no change in what runs |

The build also leaves out:

- **SDL3**, built as it is: its code isn't Frigate's.
- **FND's heaps** (`lib/nnsys/src/fnd`: `expheap.c`, `frameheap.c`, `unitheap.c`, `heapcommon.c`), which read and write
  their block heads and free lists in the poisoned memory between the game's blocks, as a C library's `malloc` does.
- **minicoro** (`mco_impl.c`), whose switches ASan doesn't need to see inside.

## What it can't see

- **Memory the game manages outside FND's expanded and frame heaps:** the SDK's `OS_Alloc` heaps (the DTCM heap,
  MSL's `malloc`, DWC's allocations), the arenas' memory outside a heap, VRAM and the DS's address windows. These are
  plain host memory to ASan.
- **Unit heaps**, which the game doesn't create; their frees are wrapped, their memory isn't poisoned.
- **An overrun within the DS's rounding of a block** (up to 3 bytes past the size asked for), which is the block's on
  the DS too, and an access to a block freed and then allocated again: FND reuses memory at once, with no quarantine.
- **The function check under Apple clang**, which emits none for C: build with upstream clang for it (CI's Linux
  job does).
- **A use of a stack frame after it returned**, which needs ASan's fake stacks: only `os_all_fake_stacks` runs with
  them; a route runs with them under `ASAN_OPTIONS=detect_stack_use_after_return=1`.
- **A second report at the same place:** UBSan reports each place once a process, and a run stops at the first report
  anyway; a survey build shows the first caller of each place.

## What it found

The survey of 2026-10-18 (port/docs/STATUS.md) ran every route of both versions under ASan and UBSan, with Apple's
clang and upstream clang 23. Its reports, and what this build does about each:

| Report | Where | Cause | Fix |
| --- | --- | --- | --- |
| R1 | `g2d_CellAnimation.c:33`, every route past the title screen | NitroSystem's read of an animation element's index through a pointer to a larger type (above) | Left out by the ignorelist: safe on every host |
| R2 | `g2d_MultiCellAnimation.c:119`, New Game's intro | The same | The same |
| R3 | `savedata.c:57`, every route at boot, upstream clang only | A call through `void (*)(void *)` to a typed initializer (above) | Left out by the ignorelist: safe on every host ABI |
| R4 | `port/hal/os/os_irqTable.c:58`, `os_timed_work`, upstream clang only | Frigate's `OS_SetIrqFunction`, as the SDK's, cast a DMA channel's or timer's handler (`void (void)`) to a callback (`void (void *)`), which `OSi_IrqCallback` called as one | Fixed in Frigate's code: [../hal/os/os_interrupt.c](../hal/os/os_interrupt.c) keeps such a handler with its type and calls it through `CallIrqFunction`, and `OSi_CountUpTick` ([../hal/os/os_tick.c](../hal/os/os_tick.c)) has the SDK's type again |
| G1 | Every UBSan report on a fiber | UBSan's trace stopped at the report's frame | [../hal/os/sanitizers.c](../hal/os/sanitizers.c) prints ASan's |
| G2 | Apple clang | No function check for C, no LeakSanitizer | Upstream clang locally; CI's Linux job |
| G3 | Every program at exit, with LeakSanitizer | The scan of the program's data read the trap's inaccessible placeholders, and never ended | The placeholders become readable at exit ([../hal/trap.c](../hal/trap.c)) |
| G4 | Suspended fibers, with LeakSanitizer | Each fiber's whole mapping was a root region, dead frames included, so a dropped block was found only once its stack slot was overwritten | Only the live frames ([../hal/os/fiber.c](../hal/os/fiber.c)) |
| G5 | Fibers destroyed while suspended, with fake stacks | Their fake stacks were never freed | `FreeFakeStack` ([../hal/os/fiber.c](../hal/os/fiber.c)) |
| G6 | The game's heaps | ASan saw the arena as one block | Poisoned block by block ([../hal/nnsys/fnd_hooks.c](../hal/nnsys/fnd_hooks.c)); what it then found is below |
| G7 | Any place reached twice | UBSan reports a place once a process | Not fixable in the runtime; moot in a build that stops at the first report |

With the game's heaps poisoned (G6), ASan found memory bugs of the game itself: the DS has them too, but its heap
tolerates them, or the bytes they touch are never used. Each is fixed in the game's file under `PLATFORM_PC`, with its
row in [game-changes.md](game-changes.md) and its finding in [decomp-findings.md](decomp-findings.md); the DS's code
is unchanged:

| Report | Where | What the game does | Fix |
| --- | --- | --- | --- |
| H1 | `src/gfl/arc_util.c`, `GFL_BGSysLoadNCGRStaticCore`, from the copyright screen (`src/ov162/boot_screens.c`): every route | Loads 0x8000 bytes of a file's characters, more than the file has, reading past its heap block into tiles the screen doesn't show | The host loads the file's characters alone |
| H2 | `src/system/gf_font.c`, `GFL_FontBitmapCopy_16x16` and the bit reader, at the title screen's first text: every route past it | The glyph copy reads 16 rows of a 15-row font's cell, and the byte after its last bits: 4 bytes past the cell's block (the 16th row is never drawn) | The host's cell has room for every row the copy reads, zeroed |
| H3 | `src/system/printsys.c`, `func_020223cc`, New Game's intro | Ends a print stream's task, which frees the stream when the task's data was allocated, then clears the stream's task field: a write to freed memory | The host clears the field first |
