# Lessons

What port work taught that isn't obvious from the code, newest first. A lesson about MWCC or the decomp's own
workflow goes in the decomp checkout instead; this file is the port's ([CLAUDE.md](../CLAUDE.md), the iteration loop).

## 2026-10-23: the deep dive after the PR 12 sync

- **Never run lldb on this Mac.** Developer mode is off, so lldb's attach raises macOS's "Developer Tools Access"
  password prompt, and while that prompt is open every `wibo` process on the machine hangs: the MWCC compiles of every
  ROM build, other sessions' included (a decomp build stalled about 16 minutes on 2026-10-23 until the prompt was
  dismissed). Find a host crash with the sanitized build ([sanitizers.md](sanitizers.md)), the crash report's
  backtrace (`port: crash: ...`) and logging; a session whose `wibo` processes all stop at once looks for that
  prompt first.
- **A green `--verify-pr` says nothing about T4.** It builds without `PORT_BUILD_ORACLE`, and every oracle test, and
  each test that needs the port's virtual environment (`asm_oracle`), skips when its tool is missing, so a test that
  expects what the game no longer does stays green there: after the field's C arrived, the oracle and launcher
  routes that expected the field's trap, and `asm_oracle`'s names from before upstream adopted the SDK's, failed only
  in a build with the oracle and the environment. Run T4 with the oracle (`-DPORT_BUILD_ORACLE=ON`, or a built
  `pbw-oracle` in the build's `oracle/`) and `build/port-venv` whenever a sync brings game C that a route reaches, and
  count the skips by name, not by number.

## 2026-10-19: the merge of the performance and hygiene tracks, after review

- **An ADR's note says who accepted it.** ADRs 0013 to 0016 were accepted at their tracks' merges by the sessions that
  merged them, on the developer's standing instruction to carry the plan forward, and their notes said "by the port's
  developer", which reads as the developer's own review, though the developer's messages in the session that ran those
  merges never mention an ADR. A note names who accepted the ADR and on what instruction, and says so when the
  developer didn't accept it in person, as 0016's does now.
- **A file that includes only the compiler's own headers can be built for a host no job builds.** No CI job is Linux
  on arm64, but `port/gfx/workers.c` needs only `stdatomic.h`, `stdint.h` and `stddef.h`, which clang brings for every
  target, so `clang --target=aarch64-unknown-linux-gnu -ffreestanding -std=c11 -Iport -S port/gfx/workers.c` builds
  it with no Linux system headers and shows its spin's `yield`. A file that needs the C library's headers, such as
  `host_posix.c` with `ucontext_t`, needs that system's headers (a sysroot or a runner), so until a job builds it, its
  branch for that host is only read.

## 2026-10-19: the merge of the performance and hygiene tracks

- **`package_macos.sh --distribution` builds the app, not the build's tests.** It builds the app's target alone, so
  a `ctest` after it runs the test programs of the last full build: in the merge, the x86_64 slice's run failed 11
  tests, the host test program not knowing the new crash scenarios (`usage: ... NAME`, status 2) and the band test's
  programs not existing yet ("Not Run"). Build every target (`cmake --build build/port-dist`) before testing a
  distribution build, as CI's job does.
- **A tree that `ctest_one_slice.sh` has thinned is not to be built.** The script swaps the build's universal
  executables for one slice until it ends, and puts the universal ones back with their old times; a `cmake --build` of
  the same tree meanwhile may link against or replace a thinned file. Run the slices' tests and the build of one tree
  one after the other, never at once.
- **A shared machine's load reaches the 3D frame first.** With other sessions holding the load average between 10 and
  26, the title's frame took 2.66 ms at the median against 1.97 quiet, 0.6 of the 0.7 ms in the 3D frame, whose bands
  need the performance cores free; the 2D lines and the geometry engine took 0.04 ms more each. To tell a change's
  cost from the load's, run the build before the change and the one after alternately in the same minutes.

## 2026-10-19: frame times and the hot paths

- **A replay's least of a few runs removes load that comes and goes, not load that stays.** A replay does the same work
  at every run, so what a burst of other work takes from one run it doesn't take from the others: in one measurement at
  a load of 30, each frame's least time over 3 runs matched a quiet run's to 0.01 ms while single runs doubled their
  95th percentile, each frame having been spared in one run at least. Steady load takes from every run alike, and other
  Frigate programs are the worst of it: since `8aef262f2` every one of them, tests included, runs its game thread at
  user-interactive QoS, so a timing test has no priority over the routes beside it. With 4 White 2 routes running beside
  it, the title's 95th percentile, each frame's least of 3, was 4.88 ms against 2.03 quiet; with 8, 8.45 in the review
  and 7.39 in a run of its own, single runs 11 to 13.4, against the 8.36 ms budget. So `port/tools/frame_times.py`'s
  `check` skips a scene over the budget when the 1-minute load average, read before each run and after the last, was
  above the cores, and the `frame_times_*` tests run alone. Read the load during the runs, not only before them: the
  average lags by about a minute. Under those 8 routes, started 5 s before the test, it read 3.86 before the first run,
  7.93 and 9.99 before the next two and 12.26 after the last, on 8 cores.
- **A timing budget holds only in the build it was set in.** The `frame_times_*` budget is set for an optimized build:
  a Debug build, or one configured with no `CMAKE_BUILD_TYPE` (as `sync_upstream.sh` makes a new `build/port`), runs
  the title's frames at 10.44 ms at the 95th percentile and the start menu's at 9.62, against 2.04 and 1.86, and
  fails it. Such builds have no timing tests.
- **A frame's cost is in the renderers.** The game's C is about 0.1 ms of a frame; the 3D rasterizer, the geometry
  engine and the 2D lines are the rest. Measure before guessing: the profile put the 3D renderer at 70% of the title's
  samples, and showed `ParamCount` and the geometry queue, the capture's per-pixel page lookups and `ensure_ready`
  behind every 2D line as the next costs, none of which the code's structure suggested.
- **A long sleep slows the next frame.** One sleep to just before the frame's time, in place of `SDL_DelayPrecise`'s
  1 ms sleeps, halved the wake-ups, but the core clocked down meanwhile and the frame's work took 7% longer, and macOS
  woke the program up to 4 ms late. Paced frames already cost about 1.4 times a headless run's for the same reason.
- **A loop of SIMD instructions with a constant trip count may still need unrolling.** The ARMv8 SHA-1 written as a
  loop over its 20 groups ran at half the speed of the same code unrolled (`#pragma clang loop unroll(full)`), since
  the group's choices and the message ring's slots stay variables otherwise. Check a hand-vectorized loop's speed
  against a library's (here `openssl sha1`, 2.4 GB/s).
- **What a pixel depends on decides whether it can be drawn apart.** The 3D renderer's pixels depend only on their own
  line and the polygons' order, but for edge marking, which reads its neighbours' depths and IDs, and the shadow
  stencil, which a mask clears: bands of lines drawn on several threads then make the same picture, if every band sees
  every polygon (its stencil clears, its edges' walk) and edge marking waits for all of them. A test that draws random
  scenes both ways, pixel for pixel, is what holds it.

## 2026-10-19: the crash reports and the sanitized build's packaging, after review

- **A check of a CMake cache by a variable's name misses its kin.** CMake keeps each configuration's flags
  (`CMAKE_C_FLAGS_RELWITHDEBINFO`, `CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO`) and a shared library's linker flags
  (`CMAKE_SHARED_LINKER_FLAGS`) apart from `CMAKE_C_FLAGS` and `CMAKE_EXE_LINKER_FLAGS`, so a refusal that read those
  two let a build configured with `-DCMAKE_C_FLAGS_RELWITHDEBINFO="... -fsanitize=address"` through, to build everything
  and fail only at the image. Match the family (`CMAKE_*FLAGS*`), and give the test one case of each member.
- **A check after a script writes its output removes the output when it fails.** `package_macos.sh` checked the image
  it had written, failed with status 1, and left the image where it would have been given out. Whatever a later check
  can fail is removed on the way out (the `EXIT` trap) until every check has passed.
- **A backtrace taken in a signal handler may not show the function that faulted.** What `backtrace()` finds after
  `_sigtramp` is the system's doing: on arm64 macOS the faulting function's frame, at the faulting instruction; on
  x86_64 under Rosetta a frame at 0 and then only that function's callers, and on a fiber nothing more. The report
  names the function from the saved program counter instead, as the trap stubs name their caller.
- **A document that says a test covers something is checked against the test.** sanitizers.md named the test of the
  image's check before any test reached that check; the test that existed only fed caches to the refusals before it.

## 2026-10-19: the crash reports of SIGFPE, SIGILL and SIGTRAP, and no disk image of a sanitized build

- **On arm64, `__builtin_trap` is a breakpoint.** It is BRK, SIGTRAP on macOS and Linux, where x86-64's UD2 is SIGILL:
  the host test named `illegal_instruction_fiber` had raised SIGTRAP on Apple Silicon all along, and a handler that
  took SIGILL alone would have missed the compiler's own traps there. An illegal instruction on arm64 is UDF.
- **A test of a division's fault divides two opaque operands.** LLVM turns `1 / x` into a comparison (x when it is 1
  or -1, else 0), so `1 / volatile_zero` never faulted, even as x86-64 code under Rosetta; `10 / volatile_zero`, with
  the dividend volatile too, did. And arm64 never faults on a division, so a test that wants SIGFPE there raises it.
- **A crash's address comes from the saved registers, not `si_addr`.** macOS on Apple Silicon gives a raised signal
  `si_code` 0, as it gives a breakpoint (and Rosetta gave a raised SIGFPE a division's code, 7), so no code tells a
  sent signal from a trap there; Linux's `si_addr` is 0 for INT3 and holds the sender's pid for a sent signal. The
  program counter in the handler's context is right in every case.

## 2026-10-19: the sanitized build's merge, after review

- **A change to what a function does in every build reaches every description of it.** `port_heap_ds_read_begin`
  began checking its span in every build (`77326dde8`), and the documents written for that change said so, while its
  declaration's comment (`port/include/port_host.h`) and the game-changes row of its caller still said it did nothing
  outside the sanitized build. `git grep` the function's name across `port/` and `.github/` and read each hit.
- **A test checks its pass condition, not its name.** `smoke_reaches_game_main` passes on a line the program logs
  before it looks for the ROM, and ctest's `PASS_REGULAR_EXPRESSION` ignores the status a program exits with (a signal
  still fails it), so without a ROM the test passes where the ROM check stops the program: no CI run reaches the
  game's `main()`. What a CI run covers is read from the test's pass condition and the code after the line it matches.

## 2026-10-19: the sanitized build merged with the host seam, the distribution build and White 2's starter save

- **ctest's `WILL_FAIL` inverts a `FAIL_REGULAR_EXPRESSION` match too.** A match fails the test, and `WILL_FAIL` turns
  that failure into a pass; only a program killed by a signal stays a failure. So the sanitized build's check of every
  test's output for a report meant nothing for a `WILL_FAIL` test. A test of a failure checks how its program ends
  through a wrapper (`check_exit.py --status`), and configuring refuses `WILL_FAIL`.
- **A wrapper shows ctest only what it prints.** `check_exit.py` printed the last 12 lines of the program's output, and
  an ASan report's `SUMMARY` line sits above its shadow bytes' legend, so it never reached ctest, while `--crash` took
  the report's abort for the crash it wanted. A wrapper that judges how a program ends judges a sanitizer's report
  itself.
- **A division by zero is the runtime's on the DS and undefined on a host.** MWCC's `u32` division calls `_u32_div_f`,
  which returns the dividend for a divisor of 0, and the game relies on it (a scroll bar of a list that can't scroll).
  arm64 gives 0 and carries on with another answer; x86-64 faults with SIGFPE, which the host's fault handler doesn't
  report (it takes SIGSEGV and SIGBUS), so the x86_64 slice died without a word. UBSan's divide check on arm64 finds
  what x86-64 would crash on.
- **A route that only one architecture runs isn't a test of the other.** CI's Intel runners have no ROM, so the
  distribution app's x86_64 slice had never reached the Pokédex; Rosetta runs it here, under the sanitizers too.
- **A read past a heap block can hide behind the next block.** Poisoning shows ASan the memory between the game's
  blocks, not the blocks themselves, so a read that runs into the next allocated block is never reported. The general
  touch bar (`src/ov139/touchbar.c`) asks the loader for the same 2 KiB of the same 128-byte file as the Pokédex's,
  which alone reported it: fix the read in the loader, not in the caller that happened to report.
- **On Windows, Python's `os.abort()` ends with 0xc0000409**, the UCRT's fail-fast, unless `_set_abort_behavior` turns
  the report off as the host seam does for the port's programs (status 3); and `%p` prints no `0x` there.

## 2026-10-18: the sanitized build, after review

- **A sanitizer's report in the game's code isn't yet a bug to fix: find what the DS does with the bytes.** The
  copyright screen's read past its file's heap block looked unused, and a fix loaded the file's characters alone. The
  tiles it read stay in VRAM, and the title screen's bottom screen shows them for some ten frames as it fades in: the
  fix drew the background where the DS draws a band. From a blank save the host read the same bytes as the DS, so the
  read was a DS-ism to keep, and the sanitized build is told so instead (`port_heap_ds_read_begin`). From a save the
  heap holds other bytes there on the host, and the band's noise differs: a read past a block reproduces the DS only
  as far as the heap's contents do.
- **The 3D tolerance forgives a 2D layer in a fade.** A screen with 3D passes when few pixels differ by more than two
  steps; in a fade from black a whole band differs by one or two, and 14,330 differing pixels passed. A game change
  that should leave the picture alone is checked by comparing its run's `frames.tsv` with the base build's on the
  routes, frame by frame, not by the oracle's verdict; and frames where a 2D layer shows over 3D are held close
  (`compare.py --close`, a sequence's `close`).
- **Off Apple, a sanitizer's report ends the program with status 1.** The runtime's `abort_on_error` defaults to on
  only on Apple's platforms; elsewhere a report, LeakSanitizer's at exit included, is `_exit(1)`, which a ctest test
  that passes on its output (`PASS_REGULAR_EXPRESSION`) or on a failure (`WILL_FAIL`, a wrapper expecting 1) accepts.
  `__asan_default_options` returning `abort_on_error=1` makes it SIGABRT everywhere, which ctest always fails.
- **An allocator's own sizes say nothing of what the game asked for.** FND's resize returns the host's size, a
  multiple of 8, and keeps the old one when it refuses a small shrink, so poisoning only when that size changed left a
  shrunk block's tail addressable. Poison and unpoison from the size the game asked for, on every successful call.

## 2026-10-18: the sanitized build

- **Apple clang accepts `-fsanitize=function` for C and emits nothing.** Upstream clang (17 and later) checks every
  indirect call's function type; Apple clang 21 builds without a check and without a word. Look for
  `function_type_mismatch` in the program's symbols (`nm`) before trusting a clean run, and run the function check with
  upstream clang (CI's Linux job does).
- **UBSan's stack trace stops at the report's frame on a fiber.** It unwinds only within the host thread's own stack
  and ignores the fiber switches ASan is told about. `__ubsan_on_report` with `__sanitizer_print_stack_trace` prints
  ASan's, which follows the fiber (`port/hal/os/sanitizers.c`).
- **LeakSanitizer scans a program's writable data by the protection it was loaded with.** Pages the program made
  inaccessible later (the trap's data placeholders) are read anyway, and on macOS the check never ended. Make them
  readable at exit, before the check runs.
- **ASan sees a game's own allocator as one block.** All of the game's heaps were inside one `calloc`, so an overrun
  between two of its allocations was invisible. Poison between the blocks in the allocator's wrappers, with the
  allocator itself built without ASan, and unpoison what the DS gives a block (its size rounded to 4), not the host's
  rounding. The first run found three places where the game's code reaches past its blocks, which the DS's heap
  tolerates (two of them bugs, the third a read whose bytes the screen shows: below).
- **A survey run reports each place once**, and a place is the access's PC: a shared copy loop (`MIi_DmaCopy32`) or
  reader reports its first caller's overrun only. Survey in rounds, fixing between them, until a round is clean.
- **Read the whole report before fixing the line it names.** The font's first report looked like a bit reader's
  read-ahead of one byte; the cell was in fact a row too short for the copy, which a debugger's look at the font's
  sizes showed.

## 2026-10-18: the macOS and White 2 merge, after review

- **A build-time alignment and the page the system reports are two checks.** The merge folded the mac track's
  `% port_vram_os_page_size()` in `test_gx.c` into the Windows seam's `% PORT_HOST_PAGE_ALIGNMENT`, which proves only
  that the linker kept `hw.c`'s alignment. A host whose page is larger than the macro but still fits in a slot would
  then have its aliasing backends skip (77) and the game copy the banks, with every test passing. Where a merge
  replaces a run-time measure with a constant, keep the run-time check beside it, bounded to the hosts where it can
  hold (here a page no larger than a slot, `port/hal/gx/vram_os.c`). Rosetta reports 4096, as an Intel Mac does.

## 2026-10-18: the macOS and White 2 tracks merged

- **A C library's `memset` touches the bytes in its own order.** `host_trap_placeholder_fiber` clears a trap
  placeholder's first 4 bytes with the library's `memset` and expected the report at `+0x0`: so it was on Apple
  Silicon, Linux and Windows, and under Rosetta, but an Intel Mac's `memset` stores the last byte first, and the report
  said `+0x3`. Only CI's Intel runners showed it, the first to run the Windows track's tests on x86_64 macOS; Rosetta
  doesn't stand in for an Intel Mac where the order of a library's accesses matters. A test of a fault made through
  the C library accepts any byte it was given.

## 2026-10-18: White 2's starter save on the oracle

- **A route of timed walks goes astray where a townsperson wanders.** Black 2's starter route held each direction for
  as many frames as its tiles take; in White 2 a walker whose area takes in the route (`npcs` with movement 3 in
  `res/field/events/<zone>.json`, here npc 11 around (36, 747)) stepped in front of the player for some 30 frames, and
  every walk after it ended short. Its random steps differ between the versions, so a route that works in one isn't
  proof for the other. Route around the wanderers' areas where the map allows (here x = 40 for x = 37), and trace each
  run's position every few frames (`PlayerState`, `GameData` + 0x114; in White 2 the heap's addresses are Black 2's
  plus 0x40) so that a stall shows where it starts, not where the run ends.

## 2026-10-18: the universal macOS app, after review

- **A header found through `-isystem` hides a `-Werror=` diagnostic whatever the flags; `-w`, or a `-Wno-everything`
  after the `-Werror=`, cancels it anywhere.** With Apple clang 21, a call to a macOS 26 API at a target of 11.0
  compiles, weakly imported, inside a header found through `-isystem` (CMake's `SYSTEM` include directories) under any
  order of `-Wno-everything` and `-Werror=unguarded-availability`. In a file of its own, `-Wno-everything
  -Werror=unguarded-availability` stops it, while `-w` in any position, or `-Werror=unguarded-availability
  -Wno-everything`, lets it through (probed again at the merge into `port-integrate`: the lesson first blamed
  `-Wno-everything` in either order, which was the system header). To hold a header-only library to the check, compile
  its implementation with `--no-system-header-prefix=NAME.h` and no `-w`, as `port/cmake/os.cmake` does for minicoro.
  Test such a probe with the build's compiler (`/usr/bin/cc`): Homebrew's clang, first on this Mac's `PATH`, uses the
  Command Line Tools' older SDK, where a macOS 26 function isn't declared at all.
- **`codesign -d` reads one slice too**, the Mac's own: give `--arch` for each slice of a universal program.
- **Write down where a claim about CI came from, and look again before relying on it**: "the oldest runner is macOS 14,
  deprecated" was true and still led to testing nothing older than 26, while macOS 15's runners, not deprecated, have
  Xcode 26 beside their default.

## 2026-10-18: the universal macOS app

- **`otool`, `nm` and the like read one slice of a universal file**, the Mac's own, unless told otherwise: make_app.sh
  took its libraries, search paths and minimum macOS from the arm64 slice alone, and gen_overlay_sections.py counted
  one slice. Read every slice (`otool -arch all`, `llvm-dwarfdump --arch`, a parser that walks the fat header), and
  check each against the build's architectures.
- **The deployment target changes what the linker makes**, not only the minos it writes: below macOS 12 on arm64 and
  13 on x86_64 a program binds lazily, with sections of its own, and a static library with Objective-C brings more on
  arm64. A tool that predicts a link must take its rules from the target and the architecture, and a test that compares
  by name, not count, says which rule is missing. Measure each rule with a small program at several targets.
- **An x86_64 Mach-O program can't align a section past 4 KiB**: ld64 lowers it and only warns, so a 16 KiB-aligned
  window in a universal build is 16 KiB-aligned on arm64 and by luck on x86_64. Check what the code needs (the host's
  page), not a constant.
- **A library fetched and built along takes what the builder's Mac has**: SDL3's configure found Homebrew's libusb and
  built in a dlopen of it. A build for other Macs turns such options off, so that it doesn't depend on the machine it
  was made on.
- **bash 3.2, macOS's `sh`, can't parse a `case` inside `$(...)`** whose patterns lack their opening parenthesis: write
  `(pattern)` there.

## 2026-10-18: the Windows track's merge, after review

- **Trace a warning count that moved against the base's own CI run, file and line.** The last STATUS entry's count
  can come from a branch older than the base: the Pokédex's 263 predated upstream's `gpiInfo.c`, which `main`'s run
  at the merge's base already warned about. Diff the warning lines of the two jobs' logs (`gh run view --job N --log`)
  by file and line; the rest of the difference shows too, such as a host's own warning or one that the merge removed.
- **Ninja's `[n/total]` counts what runs, not the build.** A rebuild after the merge's flags changed ran 1,701 of a
  fresh build's 1,797 steps; only a fresh build directory's total is the build's size.

## 2026-10-18: Windows in CI, after review

- **On Windows, nothing up a fiber's stack reaches the unhandled-exception filter.** The system looks for a handler
  frame by frame up the faulting stack, and a fiber's frames end at its entry; the filter's frame is at the bottom of
  the thread's own stack. An exception on a fiber that the vectored handler doesn't take ends the process with no
  word, so the seam takes every fatal exception there. Test a crash path on a fiber, not only on the main thread.
- **A fault in the C library's code is outside the program's image.** The game's `memcpy` and `memset` run in
  `ucrtbase.dll` on Windows, so a handler that takes only its own image's faults misses a trap placeholder touched
  through them. Decide by the address accessed (the memory the seam protected), not by the code that touched it.
- **`-fshort-wchar` lets clang turn a u16 loop into the C library's `wcslen`.** The loop-idiom pass recognizes a scan
  for a 16-bit zero as `wcslen` when `wchar_t` is 16-bit, and on macOS and Linux the library's `wcslen` counts 32-bit
  units. `-fno-builtin-wcslen` keeps the loop; `gen_stubs.py`'s list of what the host provides is where such a call
  shows.
- **`ctypes.CDLL(None)` doesn't see glibc's `libc_nonshared.a`.** `atexit` is linked into each program from it, hidden,
  so a lookup through Python's process missed it and `backup.c`'s `atexit` became a trap stub on Linux. Read what the
  compiler driver links, linker scripts included, and never stub a function of standard C.
- **SDL's arguments are UTF-8 on Windows, the C library's names are the code page's.** A manifest that makes UTF-8
  the process's code page is the one switch that makes `fopen`, `_mkdir`, `getenv`, the `*A` calls and SDL agree.
- **A fiber's first frame needs a return address of 0.** libgcc's unwinder (glibc's `backtrace()`) reads the code at a
  return address it has no unwind table for, looking for a signal's trampoline; minicoro's dummy
  `0xdeaddeaddeaddead` made every backtrace on a fiber fault on x86-64 Linux, so a trap's report from the game died
  of SIGSEGV. macOS's frame-pointer walk never showed it: run a new crash path's test on every host's CI.
- **MSYS2's Python writes paths with "/" in its shells.** A symbolic link whose relative target has one isn't followed
  on Windows; give a link the system's separator, and check that a probe link resolves before trusting links. CI ran a
  CPython from `setup-python` until the job followed the README, which is how this showed.

## 2026-10-18: Windows in CI

- **lld groups a COFF program's sections by their contents as well as their names.** A zeroed static given a section
  by an attribute is initialized data, so its `.bss$...` section went to another output `.bss` than the statics that
  `#pragma clang section bss` placed, and markers meant to bracket them didn't. Only a run that loads overlays would
  have noticed; `integrate_overlay_sections` now checks it without a ROM, with a probe file compiled as an overlay's.
- **ctest fails a test that crashed, whatever `PASS_REGULAR_EXPRESSION` says.** A test of a crash report runs the
  program through `check_exit.py --crash`, which takes a signal or a Windows exception's status as the ending wanted.
- **Windows' text mode is a difference of the tests, not only of the program.** The program's `"\n"` became `"\r\n"`
  (stdout and stderr go to binary mode before `main()` now), the artwork's generator wrote files that no longer matched
  the committed ones byte for byte, and the oracle's stand-in programs, run by their `#!` line, didn't run at all.
  Write files that are compared byte for byte in binary mode, or with `newline="\n"`, and give a script a `.cmd`
  where Windows must run it.
- **Windows delivers an exception on the faulting stack.** A fiber's guard of pages with no access gets no report on
  Windows when the stack pointer is already inside it; a guard page over writable pages gives the system room for
  its exception frame, and the handler then moves to a stack of its own, as a signal handler's `sigaltstack` does.
- **Read the first CI run's whole build log.** With `-k 0`, one run on a new host lists every file that fails; the
  decomp's 1,473 files all compiled for Windows the first time, and the 13 failures were the port's own POSIX code.

## 2026-10-18: upstream's matched libfx, and the divider read by its address constant

- **A register that acts when read has to act however the code names it.** The host tree routed the divider's and
  square root unit's results to `port/hal/hw` through cp.h's register macros (`reg_CP_DIV_RESULT`), but cp.h's address
  constants (`REG_DIV_RESULT_ADDR`) became plain memory. Nothing read through them until upstream's matched
  `VEC_Normalize` used new inlines that do: it read zeros, and every MCSS sprite of the intro (Professor Juniper,
  Minccino) vanished. The generator now routes an address constant like a register macro, write streams aside, and
  `fx_libfx` normalizes through the translated headers.
- **The asm oracle proves the C, not the host's hardware.** `test_fx_decomp.py` passed on the new libfx because its
  harness models the registers itself; what it can't see, a replay's screens did. When an upstream merge breaks a
  replay, take the merged files back one group at a time against a pre-merge binary's frame: three builds found it.
- **A test's time limit on a loaded machine measures the machine.** Two ctest runs at a load average of 60 to 110 timed
  out `oracle_title_hashes` and `asm_oracle` while the same runs alone passed in minutes; read a timeout's own output
  (an empty run directory, a run still going) before calling it a regression, and size limits for slow runners.

## 2026-10-18: the test launcher, after review

- **ctest's `PASS_REGULAR_EXPRESSION` ignores the exit status.** A test with one passes on its output alone, so a
  test of a status runs the program through a checker that compares it (`port/tests/app/check_exit.py`, or
  `check_launch_smoke.py --status`).
- **A tool that stops short must say so in its status.** The launcher ended a run that never opened an app, or never
  opened the last one, with status 0; only the report lines that the oracle checks compared kept the tests honest.
  Status 0 now means every app opened and closed, and a run that ended first says what never did.
- **A stand-in does each thing the original does each time, not once.** The field menu's choice by a key sets the
  touch flag before every app; the launcher set it before the first only, and an app closed by a touch left it set
  for the next one, whose screen then differed. Read where a value is set in the code it stands in for, per call.

## 2026-10-18: the field menu's apps against the oracle

- **Align an app on its first frame, measured on both programs, not on its press.** The DS takes a different number
  of passes than the port to close the field and load an app; opening the app on the port at the oracle's first app
  frame's count less the port's own passes to its first frame lines up every later pass, and then a press inside the
  app reaches both on the same pass. The bag, the card and the party list then match pass for pass.
- **Check every pass of an app, not only its labelled screens.** The labelled rows of the trainer card matched while
  its opening wipe was missing; the span check (every picture the port shows at a count is one the oracle shows there)
  found it.
- **A screen that steps by VBlanks parts from the oracle wherever the DS takes long passes.** The summary scrolls its
  background by `OS_GetVBlankCount()`, and the DS loads it in a pass of 30 frames: its background stays some pixels
  ahead. Look for the VBlank count in the code before suspecting the renderer.
- **A route made for one version doesn't walk the other.** The starter route's input, written on Black 2's oracle,
  goes astray on White 2's: each version needs its own measured route.

## 2026-10-18: the H-blank interrupt and its flag

- **An interrupt handler that takes no time sees the hardware as it was when the interrupt came.** The port raised
  the H-blank interrupt 70 cycles before DISPSTAT's H-blank flag, where melonDS sets both at once. The game's line
  wipe checks `GX_IsHBlank()` before each window write, so on the port it never wrote one, quietly: the screens went
  from black to the picture. A flag and the interrupt it describes belong at the same moment on a host, whose handlers
  run in no time. The moment is GBATEK's 1606 cycles into the line; melonDS 1.1 uses 1584 for both, which the game
  can't tell apart.
- **A missing transition shows only frame by frame.** The trainer card's screens were right before and after its
  opening; only comparing every pass of the opening with the oracle's showed the wipe missing.

## 2026-10-18: the card's infrared chip

- **A register accessor can't tell a load from a store.** `port_hw_*` returns the memory the access uses before the
  access happens, so a register whose writes act has to be resolved at the next access: compare the cell with what
  it held, and when a store may write the value the cell already holds, give the cell a marker that every store
  clears. `ir_check.c` sends 0 right after reading the answer 0; comparing values alone lost that byte and the check
  failed. Each store is preceded by its own accessor call, so at most one is pending and resolving it at the next call
  is exact (`port/hal/card/card_spi.c`).
- **Plain memory for a register the game polls fails quietly.** AUXSPIDATA read back the last byte written, the
  genuine-card check failed, and nothing stopped: the game only installed an HBlank task for good and would have
  skipped battle experience. A breakpoint on the game's check against the oracle's outcome found it, not a test.

## 2026-10-18: the sound against the oracle, after review

- **A counter the hardware reloads keeps counting through a write to its reload.** The channel's timer was modelled as
  a count from the current step compared with the current period, which is the same until the driver changes the
  period mid-note (an LFO, a sweep, a bend: most of the opening's notes); then the note's phase moved. Model the
  counter as the hardware has it (count up from the reload, reload at the overflow), and test a write mid-count.
- **A comparison that only looks at what pairs can't fail on what's missing.** The runs were paired by their count
  and every pair was equal, but a play that ended early or late on the port would have passed: count what is on one
  side only, inside a window both sides cover, and fail on it.
- **melonDS's frame isn't the DS's.** `RunFrame` runs lines 0 to 262 and the VBlank comes at 192, so a state stamped
  with melonDS's frame number is up to 71 lines past the VBlank the port's frame ends with. Stamp by the VBlank, and
  say how many compared frames hold what the check is meant to see (here none had a player active).

## 2026-10-17: saves with melonDS, after review

- **A function's address used as a number is host state.** DWC adds `(u32)OS_GetTick`, the function's address, to the
  seed of the console's Wi-Fi ID (a `BUG` the decomp marks); on the DS it is a constant, on a host whatever ASLR made
  it at launch, so every run wrote another save. Two runs often agree by chance, since the slide reaches only the
  seed's high bits: hold such a value to the oracle's, not only to a second run (`replay_same_save` does both). A
  Thumb function's address has bit 0 set (`0x0207bb0d`), as the ROM's literal pool shows.
- **Count a difference twice before calling it a figure.** "10,718 bytes differ" came from one run of a
  nondeterministic port; a second run would have shown it move. Diff two runs of the port before diffing the port
  against the oracle, then sort what is left by cause.
- **Allow a stop by its trap's name, not by its exit status.** A sequence that let any trap stub pass from a frame on
  would have kept passing if the port began stopping on another gap; `stop_trap` names the one it stops on.
- **A rename over a symbolic link replaces the link.** The atomic write (temporary file, then rename) has to resolve
  the save path first, or a player's link to melonDS's `.sav` silently becomes a file of its own.
- **Every test that runs the program needs `--save-dir`**, even one that runs a frame: without it the program reads the
  player's own save directory, and a file there of the wrong size fails the test.
- **The cursor's pulse is a cycle, so a step is shown at many counts.** Saying the DS "doesn't show" a picture needs
  the whole run's hashes: White 2's press frame shows a pulse step the DS shows at four other counts of the same run.

## 2026-10-17: saves with melonDS

- **A save moves the oracle's frame numbers, not its counts.** The DS's start-up reads the save before the main loop
  starts: with the first save it starts at oracle frame 219, with a blank backup at 242, with a formatted one at 246.
  Known differences declared by oracle frame (`known_oracle`) move with it, 23 frames sooner for the title's torn
  frames from the first save; those declared by port frame or count don't.
- **Make a save on the oracle, keep only its SHA-1.** melonDS is deterministic, so one input file gives the same save
  memory at every run (`oracle.py save`): a test makes it at run time and checks it against the hash before using
  it, and nothing the game wrote is committed. Driving the field menu, the first DOWN only shows the cursor, and a
  press before the save prompt has opened is lost, so give each step room and look at the screens.
- **A press brings a pass's lag back to 0.** White 2's start menu runs a pass behind on the DS after the title's cry;
  CONTINUE's press reaches both on the same pass, and from the next frame the counts pair with lag 0 again, but the
  press's own frame shows the cursor a blink step apart on each.

## 2026-10-17: the sound against the oracle

- **A host whose CPU takes no time changes who wins a race the DS's CPU time decides.** The ARM7's alarms ran lazily
  (at each VBlank and each word of the ARM9), which is the same state whenever anyone looks, but not the same
  interrupts: two or three alarms a frame reached the ARM9 together, and NitroSystem's stream player, which drops a
  queued block when a third arrives before its thread has read one, played a block of silence every 7 frames or so
  and fell behind the DS's music. Anything the hardware raises as an interrupt on its own clock needs the OS core's
  timed work (`port_os_set_timed_work`), not a catch-up at the next VBlank. The same instant ARM9 then rewrote a
  stream's buffer two samples before the channel played them, which the DS's FIFO, read ahead, never sees.
- **Compare a sequencer by its own clock.** The driver's periodic runs fall at different moments of a frame on the two
  programs, so its state at a VBlank can't match; paired by the count of periodic runs since each play started, every
  field of every run matched, and the one real difference (a channel ending a run early) pointed straight at the
  sound unit's start delay.
- **Sample-exact audio needs the reference's own hardware model**, down to its quirks: melonDS starts a PCM sound on
  the third timer step, reads through an 8-word FIFO and counts a volume of 127 as 128. With those, a stream and a cry
  matched melonDS to the bit, which makes their hashes a test. The sequences don't, and the reason was measured, not
  assumed: the DS's register writes come 1 to 5 mixer samples after its alarm, the port's in the first, so they are
  held by their envelopes.

## 2026-10-17: New Game's intro, after review

- **The decomp's incomplete C can differ from the ROM where only aliasing shows it.** `MTX_Concat44` and its siblings
  read their first input after storing part of the result; the ROM loads each row first, so the game's products into
  their own input are right on the DS and wrong on a host, and every test with separate pointers passed. When a
  library function writes through one pointer and reads through another of the same type, give the asm oracle cases
  with the pointers equal (`port/tests/asm_oracle/test_fx_decomp.py`), and grep the game for in-place calls.
- **A replay survives without proving anything** until it checks what the screens show. `--run-dir` writes
  every frame's screens in the oracle's `frames.tsv` format, so the goldens are the oracle's own hashes and a test
  needs no images (`port/tests/replay/check_screens.py`).
- **Press in video frames through the boot, and give presses room.** The DS drops frames in the logo movie and at
  every load, so its game count reaches the title screen about 2,500 passes earlier than the port's: a press keyed
  to the game count lands on another moment of the title on each (the DS's title had restarted its animation). In
  video frames both show the title at 3,900, and both take 155 passes from START to the start menu. After that,
  presses that keep coming (A twice a second) find each program waiting, so an input file plays the same on both
  however far the DS falls behind.

## 2026-10-17: New Game's intro

- **ld64 doesn't keep a section's zero-fill objects in the order they are defined.** The trap stubs bracketed the
  data placeholders with two page-sized paddings, defined before and after them in one file; ld64 put both paddings
  after every placeholder, so no placeholder was protected and the game read zeros where it should have trapped. Work
  out a range from where each object landed (`port/hal/trap.c`), or take the section's own bounds, never from symbols
  defined around it.
- **The compile probe preprocesses as MWCC does**, without `PLATFORM_PC`, where `uptr` is `u32`: a `(u32)` cast made
  `(uptr)` leaves its counts as they were. Its `-Wpointer-to-int-cast` and `-Wint-to-pointer-cast` lists per file
  (`port/tools/compile-probe`) are still the quickest map of the pointers a path cuts.
- **A `u32` that carries a pointer hides behind an API more often than behind a cast in the crashing function**:
  setters with a value that is sometimes a pointer (`PokeParty_SetParam`), callback parameters (NitroSystem's
  animation functor, MCSS), handles returned as `u32` (`func_02005af4`) and VM stacks that hold return addresses.
  Retype the API and every sibling, then the `(u32)` casts at its callers; the callbacks' own parameters follow,
  since a function pointer of the old type no longer converts.
- **A fault far from a cut pointer**: a buffer sized for DS structs overflows quietly into the next heap block, and
  the crash comes later from whatever lived there (the particle system's camera, read back as NULL). Before
  chasing the crash, check the sizes of the buffers allocated just before the block that broke.
