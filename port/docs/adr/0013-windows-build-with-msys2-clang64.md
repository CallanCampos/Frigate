# 0013: Windows built with MSYS2's CLANG64, its differences behind port/hal/host

- **Status:** accepted
- **Date:** 2026-10-18
- **Plan:** D4 (clang on every platform), §4.4 ("clang-cl or mingw-clang on Windows"), §7 phase 5 ("Windows and Linux
  builds in CI"); it changes the platform boundary of [ADR 0005](0005-platform-boundary-as-built.md) on Windows
- **Upstream:** `993e6c49b`
- **Note (2026-10-18):** accepted at the Windows track's merge into `port-integrate` (`d294567e3`) by the port's
  developer, as every earlier ADR was: the toolchain, MSYS2's CLANG64 over the clang-cl that
  [tech-stack.md](../research/tech-stack.md) recommended, is the developer's call.

## Context

Phase 5 asks for Windows and Linux builds in CI. Linux has had its job since phase 1; Windows had none, and the port
had never been compiled for it. The research of 2026-10-16 recommended clang-cl as Windows' compiler
([tech-stack.md](../research/tech-stack.md): the MSVC ABI, PDBs, Visual Studio's debugger), and PLAN.md §4.4 allows
clang-cl or mingw-clang. A probe of 2026-10-18 (Homebrew clang 23 for `x86_64-w64-windows-gnu` and
`x86_64-pc-windows-msvc`) and the first CI runs on GitHub's `windows-2025` runner found:

- **The build's flags are GCC-style** throughout (`-fwrapv`, `-fno-strict-aliasing`, `-ftrivial-auto-var-init=zero`,
  `-fshort-wchar`, `-fsigned-char`, `-include`, `-std=gnu99`, `-fgnu89-inline`, the stack probes), and clang-cl would
  need a second spelling of each (`/clang:`, `/FI`); only the overlays' forced includes have one today. clang-cl also
  links no `__divti3` (the geometry engine's 128-bit division), and has no `unistd.h`, `dirent.h` or `S_ISDIR`.
- **MSYS2's CLANG64** ships clang, lld and compiler-rt (22.1.8 on the runner, the same major version as the Homebrew
  LLVM used here less one) over the UCRT, with CMake, Ninja, Python and SDL3 3.4 as packages (3.4.16 on the runner on
  2026-10-18; the build takes any installed SDL3 3.4, and builds its pinned 3.4.18 only where none is). With it, every
  one of the 1,473 C files of the decomp that the host build compiles built for Windows unchanged at the first try;
  only the port's own POSIX code didn't.
- **What differed was the port's**: the trap stubs' and the memory windows' 16 KiB alignment (COFF aligns no static
  object beyond 8 KiB), signals and `sigaltstack`, `mmap` and `mprotect`, `backtrace` and `dladdr`, `alarm` for the
  watchdog, the program's own symbols and static data, two-argument `mkdir`, and the tests' harnesses, which used all
  of these. `gen_stubs.py` passed its 1,582 objects on a command line (190 KB, past Windows' 32 KB) and asked the
  running Python's C library what the host provides (`ctypes.CDLL(None)` has no Windows form). The overlays' bss
  markers on COFF were initialized data in another output `.bss` than the statics they bracket, which only a ROM run
  would have noticed. The text mode of Windows' C library turned the program's `"\n"` into `"\r\n"`.
- **A fiber's stack overflow** can't be reported on a stack of its own as on POSIX: Windows delivers an exception on
  the faulting stack, writing its frame below the stack pointer, so a fault deep in pages with no access ends the
  process unreported. And Windows looks for a handler frame by frame up the faulting stack: a fiber's frames end at
  minicoro's entry, so an exception there never reaches the frame at the bottom of the thread's stack that calls the
  unhandled-exception filter, and the process ends without a word.
- **The program's arguments are UTF-8** (SDL's `WinMain` makes them from the wide command line), while the C library's
  `fopen`, `_mkdir` and `getenv` and the system's `*A` calls read names in the ANSI code page: a ROM or a folder whose
  name isn't ASCII would be two different names to the program.

## Decision

- **Windows builds with MSYS2's CLANG64 toolchain**: clang, lld, compiler-rt and the UCRT, with MSYS2's CMake, Ninja,
  Python and SDL3, unpinned as MSYS2 ships them. The build is the same CMake as on macOS and Linux, with the same flags.
  CI's `host-windows` job (`.github/workflows/port-ci.yml`) installs it with `msys2/setup-msys2` and builds as
  port/README.md tells a player to, in the CLANG64 shell with its own Python; it checks out the tree without `res/`'s
  images, sounds and models (the host build reads only the text and the data the text's headers name) and with git's
  line ends kept, sets `PYTHONUTF8` (the decomp's headers are UTF-8), builds with `-k 0` so that one run lists every
  file that fails, and runs ctest. Every Windows program of the build carries a manifest that makes UTF-8 its code page
  (`port/app/windows/utf8.manifest`, Windows 10 1903 and later), so that its arguments, the C library and SDL agree on
  every name.
- **What the platform layer, the program and the tests share of the system is in one place, `port/hal/host`**:
  `host.h` says what they ask of it (pages and their protection, faults on a stack of their own, backtraces and the
  program's names, the watchdog, writes to stderr that take no lock, directories, the environment, private file
  mappings, the program's static data); `host_posix.c` is macOS's and Linux's half, the POSIX code that `trap.c`,
  `fiber.c`, `pointer.c`, `os_terminate.c`, `main.c`, `run_dir.c` and `symbols.c` had (`symbols.c`, a one-line
  forwarder then, is gone: `main.c` asks the seam); `host_windows.c` is Windows'. Each compiles to nothing on the
  other's hosts, so CMake needs no list per host. A module whose own feature is a host's keeps its branches, since the
  seam would only move them: the saves' folder, atomic replacement and links (`backup.c`), the VRAM's aliasing backends
  (`vram_os.c`, whose page size is the seam's), the mapped ROM (`rom.c`), the RTC's reentrant local time (`rtc7.c`),
  the overlays' sections, which branch on the object format (`ovsec.c`), the macOS app's log (`app/message_log.c`), the
  window's icon (`app/window_icon.c`) and LeakSanitizer, which Windows' ASan lacks (`fiber.c`). `port/hal/README.md`
  lists them.
- **Windows' half**:
  - faults go through a vectored exception handler, installed before `main()` and last in the list (a sanitizer's own
    comes first). It takes a stack overflow anywhere; a memory fault at an address the seam made inaccessible (a trap
    placeholder, a fiber's guard), whatever code touched it, the C library's `memset` included; every fatal exception
    on a fiber (the seam knows a fiber's thread by its TEB's stack, which starts where a guard it made ends); and the
    program's own access violations once it has a fault handler. It copies the context and moves the thread to a 256
    KiB stack of its own, with the TEB's stack bounds, where the report runs. Any other exception that nobody handles
    is reported by the unhandled-exception filter, which only a thread's own stack reaches. Either way the program
    then ends with the exception's status, as it would have (a guard page's is a warning's, 0x80000001);
  - a fiber's guard is topped by one guard page over 12 KiB left writable: the guard page's first touch raises the
    fault, and the system has room below it for its exception frame (which passes 4 KiB with the processor's extended
    state); the rest of the 64 KiB guard has no access;
  - backtraces unwind by the image's unwind tables (`RtlVirtualUnwind`), and names come from the COFF symbol table that
    lld keeps in an unstripped MinGW program, read from the program's file; globals are found there by name for
    `--run-dump`, statics not, as on Linux;
  - the watchdog is a thread that a kick's event restarts; when it goes off it suspends the main thread and prints its
    frames; 0 seconds turns it off, as `alarm(0)` does on POSIX;
  - before `main()`, in every program that links the platform layer, stdout and stderr go to binary mode and no error
    dialog can open (a crash's, `abort()`'s), which would hold a test or CI run for ever; the main thread gets 8 MiB of
    stack, as on macOS and Linux;
  - the memory windows align to 4 KiB, and the VRAM banks are copied between their views (the `copy` backend of
    `--vram-backend`, which every host has and Windows takes by default), since a static array can't be Windows'
    placeholder memory.
- **Portable fixes where they are the same on every host**: `gen_stubs.py` reads its objects from a list file on every
  host, and on Linux and Windows asks the compiler driver which libraries it links and reads what they define with the
  toolchain's `nm` (on Linux besides what `ctypes` finds: glibc keeps `atexit` in `libc_nonshared.a`, out of its
  reach, so `backup.c`'s `atexit` was a trap stub there); it lists the ROM's symbols each host's C library provides, so
  that a host function that shares a game function's name would show, never stubs a function of standard C, and takes
  the stubs' page alignment from `host.h`. `-fno-builtin-wcslen` keeps clang from turning the decomp's u16 loops into
  calls of a `wcslen` whose `wchar_t` is 32-bit on macOS and Linux. A fiber's first frame has a return address of 0,
  where minicoro's dummy one made glibc's `backtrace()` fault on x86-64 Linux, so that the report of a trap or crash on
  a fiber has its backtrace there too; the include mirror's links have Windows' separator, which MSYS2's Python
  doesn't give them, and become copies when a probe link isn't followed.
  The overlays' COFF bss markers are placed by `#pragma clang section`, and `integrate_overlay_sections` checks on
  every host, with a probe file compiled as an overlay's, that the sections hold a file's statics and that a load sets
  them back. The run directory's files are binary, the artwork's generator writes `"\n"`, and the tests' temporary
  files, links and environment go through the seam.
- **Saves on Windows** stay in `%APPDATA%\Frigate`; a save that is a symbolic link isn't followed there (it was so in
  `backup.c` before), and `card_backup_linked_save` skips (77).

## Alternatives

- **clang-cl with Visual Studio's tools** (the research's choice): PDBs and the Visual Studio debugger, and the MSVC ABI
  that a C++ UI or a Windows SDK component might want later. It needs a second spelling of every flag, `__divti3` from
  compiler-rt by hand, and stand-ins for `unistd.h` and `dirent.h`, and its MS-compatibility mode changes some C
  semantics, so the build would differ from macOS's and Linux's where this one doesn't. Ship of Harkinian and 2Ship
  build this way, but they are C++ on Visual Studio's own libraries. If debugging with Visual Studio becomes needed, a
  CLANG64 build can emit CodeView (`-gcodeview -Wl,--pdb=`) first.
- **MSVC's `cl`**: no `__attribute__`, no GNU statement expressions; ruled out by PLAN.md §4.4.
- **MSYS2's UCRT64 or MINGW64 with GCC** (the Platinum port's and sm64coopdx's): another compiler, with its own layouts
  and undefined behaviour to verify, where D4 asks for clang everywhere, and no `#pragma clang section`, which the
  overlays' resets need.
- **`#ifdef _WIN32` where each POSIX call was**: the smallest diff, but it scatters the hosts through the platform layer
  and the tests, and every later host repeats it.
- **Exporting every symbol (`--export-all-symbols`) for the names**, as macOS's export trie has them: it makes an export
  table of 20,000 entries in the program, where the symbol table lld writes already has them.
- **Windows fibers (`MCO_USE_FIBERS`)** for the threads: the system's own guard pages and stack overflow exception, but
  minicoro then places the stack itself, which `fiber.c`'s layout check refuses; minicoro's x86-64 assembly worked
  first time (every `os_*` test passes), so it stays.

## Consequences

- Windows builds and passes every test that needs no ROM, as macOS and Linux do, but for the 10 `gx_*_shm` tests
  (no aliasing backend there) and `card_backup_linked_save`, which skip. `layout_trainer_card_param` runs there, since
  CLANG64 has `llvm-dwarfdump`, where it skips on Linux's runner.
- Two statements of [ADR 0005](0005-platform-boundary-as-built.md) hold on macOS and Linux only: on Windows the ROM is
  read into memory rather than mapped, and the VRAM banks are copied between their views rather than aliased.
- Six test harnesses still include MinGW's `unistd.h` outside the seam (`getpid`, `rmdir` and `unlink`: `test_host.c`,
  `test_backup.c`, `test_integrate.c`, `test_rom.c`, `test_snd.c`, `test_run_dir.c`), which CLANG64 has and clang-cl
  hasn't: a clang-cl build would first move those calls behind the seam too. (`backup.c`'s `sys/stat.h` is one MSVC's
  library has as well.)
- CI's Windows job builds with whatever MSYS2 ships that day; a change there (a new clang, SDL3 3.5) shows first as a
  red Windows job.
- Nothing has run the game on Windows with a ROM: the replays, the oracle comparisons and the boot past `main()` need a
  Windows machine with the player's dump. The VRAM's copying backend has the same tests as the aliasing ones on macOS,
  but no replay has compared the two there.
- The ROM build can't run on Windows (the decomp's `configure.py` has macOS's and Linux's tools only), so a Windows
  session can't verify both SHA1s; port work stays on the Mac.
- Sanitizer builds (T5) aren't set up on Windows: the plain build's `_sanitized` test variants run under ASan and UBSan
  there, but the whole program under ASan is untried, and the vectored handler takes the program's own access violations
  (it runs after ASan's, which maps its shadow memory on faults).
- A Windows ARM64 build would need minicoro's fiber fallback (it has no ARM64 Windows assembly) and with it another
  guard layout; not tried.
- Revisit if the program needs Visual Studio's debugger or the MSVC ABI (a C++ UI on MSVC libraries), or if MSYS2 stops
  shipping a CLANG64 SDL3 near the pinned release.
