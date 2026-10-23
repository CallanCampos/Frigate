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
  /tmp/san-saves --input port/tests/replay/start_menu.pbwinput`. A run that hits a report aborts (SIGABRT, status 134)
  on every host: [../hal/os/sanitizers.c](../hal/os/sanitizers.c) makes `abort_on_error=1` the runtime's default.
  That is macOS's own default; elsewhere the runtime ends a report with status 1, the status of the program's own
  failures, which a test that passes on its output (`PASS_REGULAR_EXPRESSION`) or on a failure (`WILL_FAIL`, a wrapper
  that expects 1) would accept.
- **A report fails the test that reaches it**, by three checks, each with its condition:
  - ctest fails a test whose program aborts, whatever the test expects.
  - In this build ctest also fails a test whose output has a sanitizer's report in it (`FAIL_REGULAR_EXPRESSION`, set
    at the end of [../CMakeLists.txt](../CMakeLists.txt)), for the test programs that don't link the platform layer.
    That holds only for a test that doesn't invert its result with `WILL_FAIL`, which turns that failure into a pass;
    so no test has `WILL_FAIL`, and configuring stops on one, in every build. A test of a failure checks how its
    program ends with [../tests/app/check_exit.py](../tests/app/check_exit.py) (`--status`, `--crash`, `--abort`).
  - A wrapper that runs the program (`check_exit.py`, `check_launch_smoke.py`, `oracle.py`, the replays' scripts)
    shows ctest only what it prints, so it fails on the program's abort itself: each wants a status, or a stop at a
    trap, that an abort isn't. `check_exit.py`, whose `--crash` and `--abort` would take an abort for the ending they
    want, fails a run with a report in its output whatever ending it wants.
- **The compiler** is the one CMake finds, Apple clang on macOS. Apple clang emits no function check for C
  (`-fsanitize=function`, in `undefined` since upstream clang 17) and its runtime has no LeakSanitizer; upstream clang
  has both: `-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang` on macOS, with `ASAN_OPTIONS=detect_leaks=1` for
  the leak check at exit, which Linux runs by default.
- **CI** builds the sanitized Black 2 on macOS and Linux on every push and pull request (the `sanitize` job of
  `.github/workflows/port-ci.yml`) and runs every test that isn't a route, then the routes that need no oracle. CI has
  no ROM, so of those only the smoke test runs, as far as the program's look for the ROM, and the replays' fixtures,
  which empty their save directories; the replays and the port's checks of the oracle's hashes skip. No CI run reaches
  the game's `main()`: the smoke test passes on the line the program logs before it looks for the ROM
  (`port/app/main.c`), whatever status the program then exits with, and the ROM check stops it there
  (`port_rom_boot`), so LeakSanitizer's check at its exit on Linux covers the program's start-up as far as that look.
  The routes themselves run only on a machine with the ROM (a nightly run of them is still to come).
- **A survey** of every report of a route, rather than the first: `-DPORT_SANITIZE=ON -DPORT_SANITIZE_RECOVER=ON` in
  another build directory (the second alone is refused) makes every check go on after a report, and ASan's
  `halt_on_error=0` the default ([../hal/os/sanitizers.c](../hal/os/sanitizers.c)). ASan reports each place once
  (`suppress_equal_pcs`), as UBSan does, so a place that two callers reach shows the first; fix and run again until a
  run is clean. `ASAN_OPTIONS=log_path=DIR/san` and `UBSAN_OPTIONS=log_path=DIR/san` keep the reports of programs
  whose output a test captures.
- **With the other options:**
  - The oracle is a project of its own (an `ExternalProject`, [../cmake/oracle.cmake](../cmake/oracle.cmake)), which
    `PORT_SANITIZE` leaves alone: `-DPORT_ORACLE_SANITIZE=ON` builds its own sources under the sanitizers, apart.
  - The modules' own sanitized test variants (the `*_sanitized` tests of `gfx`, `g3d`, `app` and `oracle`) set their
    flags themselves: they stay fatal in a survey build, and in this build their `*_plain` twins are sanitized too,
    by the build's flags.
  - The timing gates (`PORT_G3D_TIMING_GATE`, `PORT_GFX_TIMING_GATE`) would time instrumented renderers against the
    plain build's limits: configuring one with `PORT_SANITIZE` is refused.
  - The macOS app built here links the toolchain's ASan runtime, which
    [../app/macos/make_app.sh](../app/macos/make_app.sh) copies into it: it is for testing on this Mac, never for
    giving out. So [../tools/package_macos.sh](../tools/package_macos.sh) refuses to put a sanitized build's app in a
    disk image (a build configured with `PORT_SANITIZE`, or with `-fsanitize=` in any of CMake's flags in its cache,
    `CMAKE_C_FLAGS_RELWITHDEBINFO` and the linkers' among them), before it builds anything; and it checks that no image
    it makes holds a sanitizer's runtime, and removes one that does. The test `app_package_macos_out` checks both, the
    image's check on a stand-in app with a library of the runtime's name, which it builds and packages.
  - With `-DPORT_MACOS_DISTRIBUTION=ON`, in a directory of its own (`build/port-san-dist`), both slices are built
    sanitized for macOS 11.0, with SDL3 linked statically and left as it is: ctest runs the Mac's slice, and
    [../tools/ctest_one_slice.sh](../tools/ctest_one_slice.sh) the other, the x86_64 one under Rosetta on Apple
    Silicon, where ASan runs too. That is how the x86_64 code's routes are checked here: CI's Intel runners have no
    ROM. Its app is a sanitized one, never one to give out, and `package_macos.sh --distribution` refuses it too.

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
- **LeakSanitizer** (upstream clang, Linux by default, or `ASAN_OPTIONS=detect_leaks=1` with upstream clang on
  macOS) checks at each program's exit and prints `ERROR: LeakSanitizer: detected memory leaks`, then each leaked
  block with the stack that allocated it: a `Direct leak` is a block nothing points to any more, an `Indirect leak` one
  that only leaked blocks point to, so fix the direct ones first. Its roots are the program's data, the host thread's
  stack and registers, and the live frames and saved registers of every suspended fiber
  ([../hal/os/fiber.c](../hal/os/fiber.c)); the game's heaps are inside the arena, which a static keeps, so nothing
  the game allocates there is ever a leak. A report is therefore a host allocation that nothing reaches: a real leak.
  A leak in Frigate's or the game's code is fixed, never suppressed. One inside SDL3 or a system library (the likeliest
  first report on CI's Linux leg) would go in an LSan suppressions file, `port/cmake/lsan_suppressions.txt` (none is
  needed yet, so none exists), each line a `leak:` pattern naming the library's function with its reason above it,
  given to every test through `LSAN_OPTIONS=suppressions=` in the tests' `ENVIRONMENT_MODIFICATION` at the end of
  [../CMakeLists.txt](../CMakeLists.txt). A run that ends on a trap stub or the watchdog leaves through `_Exit` and gets
  no leak check.
- **A trap** (status 70) or the watchdog (73) end a sanitized run as they end a plain one, and so does a crash that no
  check reported first: Frigate's handler of SIGSEGV, SIGBUS, SIGFPE, SIGILL and SIGTRAP
  ([../hal/host/host.h](../hal/host/host.h)), set at start-up, replaces ASan's own of the first three, so the report is
  `port: crash: signal N at address ...` with a backtrace, as in the plain build
  ([MODDING.md](MODDING.md#when-the-game-stops)), and not ASan's `SEGV` or `FPE`. A division by zero is UBSan's to
  report, at the division, before x86-64 faults on it or arm64 gives 0.

## What Frigate tells the sanitizers

| What | Where | Why |
| --- | --- | --- |
| Every switch between fibers and the host stack | [../hal/os/fiber.c](../hal/os/fiber.c) | ASan must know which stack runs, or its stack checks and unwinds go wrong; minicoro's own annotations miss the switch back to the host stack, so [../hal/os/mco_impl.c](../hal/os/mco_impl.c) is built without ASan and `fiber.c` annotates instead |
| A dying fiber's last switch, and a fiber destroyed while suspended | `fiber.c` (`FiberMain`, `FreeFakeStack`) | So that ASan frees the fiber's fake stack (`detect_stack_use_after_return=1`, which `os_all_fake_stacks` turns on); a fiber destroyed while suspended once kept its own |
| The live frames and saved registers of suspended fibers | `fiber.c` (`ScanStackForLeaks`), with the saved stack pointer from `mco_impl.c` | LeakSanitizer scans threads' stacks, not fibers': an allocation only a suspended fiber holds would be a leak. Only the live frames, from the saved stack pointer up, so that the dead frames under them don't hide one |
| UBSan's reports, with ASan's stack | [../hal/os/sanitizers.c](../hal/os/sanitizers.c) (`__ubsan_on_report`) | UBSan unwinds only within the host thread's own stack, so its trace stopped at the report's frame on a fiber |
| The game's expanded and frame heaps, block by block | [../hal/nnsys/fnd_hooks.c](../hal/nnsys/fnd_hooks.c), with FND's heaps built without ASan ([../cmake/nnsys.cmake](../cmake/nnsys.cmake)) | Every GFL heap and the sound heap live inside one 16 MiB arena, which ASan sees as one block: an overrun from one of the game's allocations into the next was invisible. A new heap's memory is poisoned, a block made addressable for the bytes the DS gives it (the size asked for, rounded up to 4) when it is allocated, and poisoned when it is freed; a destroyed heap's memory is its owner's again. The host memory of a root heap that the game creates in memory of its own ([ADR 0021](adr/0021-root-heaps-in-the-games-own-memory-from-host-memory.md)) has no owner between two heaps: [../hal/gfl/heap_policy.c](../hal/gfl/heap_policy.c) poisons all of it when its heap is destroyed and makes it addressable for the next, so that a use of the field's old heap during a change of map is a report |
| A read the DS makes past one of the game's heap blocks, whose bytes it uses | `fnd_hooks.c` (`port_heap_ds_read_begin`, `port_heap_ds_read_end`), from `src/gfl/arc_util.c` under `PLATFORM_PC` | The copyright screen loads 0x8000 bytes of a smaller file's characters, past the file's block into what follows it in the heap, as the DS does, and the title screen's bottom screen shows those tiles as it fades in (H1, below): the read stays the DS's, and ASan lets it reach the span it names and poisons again after it what was poisoned. The span must lie in one of the game's expanded or frame heaps alive, heads included, or else in the main arena they are carved from (`port_os_main_arena_holds`): a span that reaches outside both stops the program with a message, in every build, rather than read, and unpoison, memory that isn't the game's (`nnsys_ds_read_outside`) |
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

The game's one read past a heap block whose bytes it uses, the copyright screen's (H1), isn't left out by the
ignorelist, which would hide every access of the function that makes it: the hooks above let that one read reach what
follows the block.

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

The survey of 2026-10-18 (port/docs/STATUS.md) ran the strict build, which stops at the first report, under Apple's
clang on every route of both versions (Black 2's 24, and White 2's 20 but the two that need its starter save, which
skip); under upstream clang 23, which adds the function check, with LeakSanitizer, on Black 2's 244 tests that aren't
routes and its 24 routes; and the survey build (`PORT_SANITIZE_RECOVER`), which goes on after each report, on Black 2's
18 routes. The merged tree of 2026-10-19 ran every route of both versions again under Apple's clang, the routes
`main` had added since among them (the test launcher's Pokédex, White 2's routes from its starter save), and found R5
and H4, both on the Pokédex.
The sanitizers' reports, and what this build does about each:

| Report | Where | Cause | Fix |
| --- | --- | --- | --- |
| R1 | `g2d_CellAnimation.c:33`, every route past the title screen | NitroSystem's read of an animation element's index through a pointer to a larger type (above) | Left out by the ignorelist: safe on every host |
| R2 | `g2d_MultiCellAnimation.c:119`, New Game's intro | The same | The same |
| R3 | `savedata.c:57`, every route at boot, upstream clang only | A call through `void (*)(void *)` to a typed initializer (above) | Left out by the ignorelist: safe on every host ABI |
| R4 | `port/hal/os/os_irqTable.c:58`, `os_timed_work`, upstream clang only | Frigate's `OS_SetIrqFunction`, as the SDK's, cast a DMA channel's or timer's handler (`void (void)`) to a callback (`void (void *)`), which `OSi_IrqCallback` called as one | Fixed in Frigate's code: [../hal/os/os_interrupt.c](../hal/os/os_interrupt.c) keeps such a handler with its type and calls it through `CallIrqFunction`, and `OSi_CountUpTick` ([../hal/os/os_tick.c](../hal/os/os_tick.c)) has the SDK's type again |
| R5 | `src/system/scroll_bar.c:25`, the test launcher's Pokédex (`oracle_launch_party_menus`), both versions: found on the merged tree of 2026-10-19 | `ScrollBar_GetPos` divides by a list's `max`, which is 0 when its rows all fit; the DS's runtime division (`_u32_div_f`) returns the dividend there, and C leaves it undefined: arm64 gives 0, x86-64 faults, and the plain build's x86_64 slice ended with SIGFPE at the Pokédex, unreported then (the host's handler has reported SIGFPE, SIGILL and SIGTRAP since 2026-10-19) | Fixed in the game's file under `PLATFORM_PC`, with the DS's result ([game-changes.md](game-changes.md)) |

What the sanitizers couldn't see or do on the fibers and in the game's heaps, the gaps this build closes or works
around:

| Gap | Where | Cause | What the build does |
| --- | --- | --- | --- |
| G1 | Every UBSan report on a fiber | UBSan's trace stopped at the report's frame | [../hal/os/sanitizers.c](../hal/os/sanitizers.c) prints ASan's |
| G2 | Apple clang | No function check for C, no LeakSanitizer | Upstream clang locally; CI's Linux job |
| G3 | Every program at exit, with LeakSanitizer | The scan of the program's data read the trap's inaccessible placeholders, and never ended | The placeholders become readable at exit ([../hal/trap.c](../hal/trap.c)) |
| G4 | Suspended fibers, with LeakSanitizer | Each fiber's whole mapping was a root region, dead frames included, so a dropped block was found only once its stack slot was overwritten | Only the live frames ([../hal/os/fiber.c](../hal/os/fiber.c)) |
| G5 | Fibers destroyed while suspended, with fake stacks | Their fake stacks were never freed | `FreeFakeStack` ([../hal/os/fiber.c](../hal/os/fiber.c)) |
| G6 | The game's heaps | ASan saw the arena as one block | Poisoned block by block ([../hal/nnsys/fnd_hooks.c](../hal/nnsys/fnd_hooks.c)); what it then found is below |
| G7 | Any place reached twice | UBSan reports a place once a process | Not fixable in the runtime; moot in a build that stops at the first report |

With the game's heaps poisoned (G6), ASan found four places where the game's code reaches outside its heap blocks
(three in the survey of 2026-10-18, the fourth, H4, on the merged tree of 2026-10-19); the DS does the same, and the
code under `#else` is unchanged. Three are memory bugs whose bytes the game never uses, fixed in the game's file under
`PLATFORM_PC`; the other, H1, is a read whose bytes the DS shows, which Frigate keeps. Each
has its row in [game-changes.md](game-changes.md) and its finding in [decomp-findings.md](decomp-findings.md):

| Report | Where | What the game does | What the host does |
| --- | --- | --- | --- |
| H1 | `src/gfl/arc_util.c`, `GFL_BGSysLoadNCGRStaticCore`, from the copyright screen (`src/ov162/boot_screens.c`): every route | Loads 0x8000 bytes of a file's characters, more than the file has, reading past its heap block into what follows it in the heap. Those tiles stay in VRAM, and the title screen's bottom screen shows them as it fades in, a lighter band over its lines 38 to 95 (the opening sequence's port frames 3527 to 3536 in Black 2) | The same read, which the hooks tell ASan is the DS's ([above](#what-frigate-tells-the-sanitizers)). From a blank save the host's heap holds there what the DS's does, and frame 3527's bottom screen differs from melonDS's in 8 pixels; from a save it holds other bytes there, and the band's noise differs from the DS's (10,764 pixels at its first frame, within the 3D's tolerance). A first fix loaded the characters alone and drew the background there (14,330 pixels apart from a blank save); the oracle comparison's 3D tolerance let it through, so the opening's and the title's first 34 frames of 3D are now held close (a sequence's `close` in [../tools/oracle/oracle.py](../tools/oracle/oracle.py), [../tools/oracle/README.md](../tools/oracle/README.md#comparing)) |
| H2 | `src/system/gf_font.c`, `GFL_FontBitmapCopy_16x16` and the bit reader, at the title screen's first text: every route past it | The glyph copy reads 16 rows of a 15-row font's cell, and the byte after its last bits: 4 bytes past the cell's block (the 16th row is never drawn) | The host's cell has room for every row the copy reads, zeroed |
| H3 | `src/system/printsys.c`, `func_020223cc`, New Game's intro | Ends a print stream's task, which frees the stream when the task's data was allocated, then clears the stream's task field: a write to freed memory | The host clears the field first |
| H4 | `src/gfl/arc_util.c`, `GFL_BGSysLoadNCGRDynamicCore`, from the touch bars (`src/ov139/touchbar.c`, `src/ov298/zknd_tbar.c`): the test launcher's Pokédex, where the bytes after the file were free, found on the merged tree of 2026-10-19 | Asks for 32 * 64 bytes of the touch bar's characters, a file of 4 (128 bytes), reserving 64 characters and copying the file's last 16 bytes and 1,904 past its block into the 60 after the file's, which the bar's screen never shows | The host reserves the same characters and copies the file's alone; the others keep what VRAM held |

The field's first survey, on 2026-10-23, ran `replay_new_game_house` (New Game through the field and the house to the
field menu's SAVE) in the survey build of Black 2. It ran on a scratch merge of the field-entry branch with upstream's
`b62c655bfa`, which the field needs, and again on the branch, after its review, with the root heaps' host memory
poisoned between two heaps. That second run's first try poisoned the memory when a child heap inside it was destroyed
too, and showed the mistake at once, 24 reports in the live root heaps' own heads, gone once only the root heap's end
releases it ([lessons.md](lessons.md)). The strict build stops at the first report, at the field's first load (R6), so
the strict build's `replay_new_game_house` fails until R6 is fixed. The survey run goes on to all 18,800 frames and
shows all 16 screens of the replay's table. Its reports are open, and none is in a root heap's host memory
([ADR 0021](adr/0021-root-heaps-in-the-games-own-memory-from-host-memory.md)); the plan of 2026-10-23 gives them to
its encounters and battles track:

| Report | Where | What the sanitizer saw | What the host does |
| --- | --- | --- | --- |
| R6 | `src/ov036/fldeff_fes_kira.c:210`, `fldeff_gyoe.c:213`, `fldeff_shadow.c:128` to `193`: the field's effect tasks, from the field's first load | Each task's work lies 4 bytes past an 8-byte boundary while its struct holds pointers: misaligned loads and stores (UBSan) | Fixed in the game's header under `PLATFORM_PC`: `FieldEffectTask.work` (`include/field/field_effect.h`) is aligned to 8 bytes on a host ([game-changes.md](game-changes.md)) |
| H5 | `src/ov036/field_buildmodel.c:852`, `FieldChunkPropHolder_SetVisible`, from `FieldPropSystem_Free` as the field closes for a change of map | A write of 4 bytes into memory of the game's heap that is no longer allocated (ASan, use-after-poison, in the main arena) | Nothing yet: open |
| H6 | `lib/nnsys/src/gfd/gfd_VramTransferManager.c:327`, `NNS_GfdDoVramTransfer`, from the main loop (`src/system/main.c`) in the field | A transfer reads its source from memory of the game's heap that is no longer allocated (ASan, use-after-poison, twice) | Nothing yet: open |

The battles' track ran the same survey build on 2026-10-23 on the Route 19 route from the starter save (the oracle's
`route19_save_b2_us.pbwinput`, port frames 0 to 7,623: CONTINUE, Route 19, the trainer's VS cut-in and Hugh's battle to
the first move's menu, where `data_ov167_021d7490` stops the program; [STATUS.md](STATUS.md)). The deep dive's survey of
the same route, on its scratch build, showed R6 to R10 and H6; after the track's fixes it shows only H6 and R10, and
`replay_new_game_house` in the survey build runs all 18,800 frames with only H5 and H6 (H5, at the change of map into
the house, is now its first report, where the strict build would stop; the strict build wasn't run):

| Report | Where | What the sanitizer saw | What the host does |
| --- | --- | --- | --- |
| R7 | `src/system/printsys.c:247` to `464`, `PrintQueue_Main`, and `:1164`, `PrintQueue_GetNextJob`: the field's queued text on Route 19 | The print queue keeps a copy of the renderer after each string, and a string rounded to the DS's 4 bytes left the renderer, which holds pointers, 4 bytes past an 8-byte boundary (UBSan, misaligned) | Fixed in the game's file under `PLATFORM_PC`: a queued string is padded to 8 bytes on a host ([game-changes.md](game-changes.md)) |
| R8 | `src/ov167/btlv_scu.c:2407`, `BtlvScu_InitGauges`, from `BtlvScu_Setup` at every battle's start | The marked `// BUG:`: the loop clears one gauge past `gauges[6]` (UBSan, out-of-bounds index); its writes land on `abilityWins[0]`'s `scu` and `bg`, which `BtlvScu_InitAbilityWins` sets right after | Fixed under `PLATFORM_PC`, which takes the `BUGFIX` loop; the battle is unchanged |
| R9 | `src/ov169/btl_monset.c:15` to `84`, from `ServerControl_AfterSwitchIn` at the battle's first switch-in | `BtlServerFlow.unk1A68`, a mon set kept as `u8 [0x48]`, lies 4 bytes past an 8-byte boundary (UBSan, misaligned); it is also 24 bytes short of a host's set, which the clear overruns | Fixed in the game's header: the flow's mon sets are `BtlMonSet` (`include/battle/btl_handler.h`, [game-changes.md](game-changes.md)) |
| R10 | `lib/nitro/src/std/std_string.c:116`, `STD_GetStringLength`, from `G3DTextDraw_CreateResource` (`src/ov012/g3d_text_draw.c:91`) for the trainer cut-in's texture names (`src/ov036/fld3d_ci.c`) | NitroSDK's string length reads a word at a time once the address is aligned, so it reads up to 3 bytes past a string literal's end (`"name_up_pl"`, 11 bytes) (ASan, global-buffer-overflow, a read of 4) | Nothing yet: open. The read stays in the aligned word, which never crosses a page, so it is safe on every host as on the DS's ARM9: a candidate for the ignorelist's `[address]` section, as R1 is for alignment |
