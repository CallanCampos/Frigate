# Spike: per-overlay .data and .bss sections for reload semantics

PLAN.md §4.5, "Files and overlays". Measured on 2026-10-17 on an Apple M2, macOS 26.5, and revised the same day
after a review: the section check now runs before the link, a link-map check finds leaks under LTO, restore costs
include the first load, and the forced include covers clang-cl and refuses GCC.

## Question

On the DS, `FS_LoadOverlayImage` clears an overlay's `.bss` and reads its `.data` from the ROM on every load, and
some game code relies on it ([engine-boot.md §3.3](../../docs/research/engine-boot.md)): lazily created pointers
that expect NULL, counters that nothing resets. The host links every overlay into one executable, so each overlay's
writable statics must be restorable without editing decomp sources. The design under test: each overlay's C files
are compiled with a forced include (`ovNNN_sections.h`) whose `#pragma clang section` puts the file's writable
statics into per-overlay sections; at startup the HAL snapshots each overlay's data section; loading an overlay
restores the snapshot and zeroes its bss.

Does the pragma work on Mach-O with Apple clang and Homebrew clang? What lands where? How does the runtime find the
bounds? Does Mach-O's 255-section limit bite, and what if it does? How do ELF and PE/COFF do the same? What does a
restore cost?

## Answer

The design works on macOS with both compilers, unchanged decomp sources, every kind of writable static (globals,
file statics, function-local statics, zero-initialized and tentative definitions) and both optimization levels. The
only kind it misses is a file-scope compound literal, and the game's overlay code has none. On the real code it finds
86 to 119 sections for 64 to 93 overlays, against Mach-O's limit of 255 per image. Loading overlay 36, the largest
(69 KB), costs 9 to 13 µs the first time, page faults included, and about 1 to 3 µs on later loads; the first
load of all 168 overlays costs 32 to 42 µs. A frame is 16.7 ms.

Recommended: the **split layout** (`__DATA,__ovNNN_d` for data, `__DATA,__ovNNN_b,zerofill` for bss) while every
configuration stays under 150 overlay sections, then the **combined layout** (one section per overlay, at most 168).
The runtime finds the sections by walking the image's load commands. Two checks guard the build. Before the link,
`check_sections.py predict` counts the sections from the link's inputs and stops a link past 255, because ld64 then
fails with unrelated errors or crashes. After the link, `check_sections.py image` reads the link map for writable
objects of overlay files outside their sections, which also covers what LTO did. The overlay loader's collision
check must stay as the game wrote it. A single shared section bracketed by link-order markers is the fallback past
255.

## Method

| File | What it does |
| --- | --- |
| `ovsec/ovsec.h`, `ovsec/ovsec.c` | The runtime sketch for `port/hal/fs` (below): scan, snapshot, reset. It uses no libc and is built as a static library. Its Mach-O path is tested; its ELF and COFF paths are compile-checked only. |
| `gen_overlays.py` | Generates synthetic overlays: one per real overlay, with the DS `.data` and `.bss` sizes read from `config/b2_us/arm9/overlays/*/delinks.txt` (168 overlays), or N small ones. It also writes the forced includes the real source-list generator would write, and the overlay of each object for the checks. |
| `check_sections.py` | The build's two checks. `predict`, a PRE_LINK step of every program, counts the link's sections from its inputs: objects, static archives member by member as ld64 loads them, and the compiler's runtime archives. It stops a link past 255 with a message that names the cause, and stops one past the overlay budget. Under LTO it computes a range. `image`, a test per program, compares the prediction with the image and reads the ld64 map (`-Wl,-map`) for writable objects of overlay files outside their overlay's sections. |
| `tests/test_reload.c` | For each overlay set: the scan finds every section. On Mach-O the scan agrees with ld64's `section$start`/`section$end`. Dirty → reset → pristine works for every overlay. Resetting some leaves the others dirty. It prints the image's section count. |
| `tests/kinds.c`, `tests/test_kinds.c` | Every kind of static-storage object, compiled as overlay 900 (split) and 901 (combined), checked against the section bounds. Values that GlobalOpt shrinks or splits are checked after a reset. |
| `tests/bench_restore.c` | Restore cost with the DS sizes: a first load (with page faults), a later load (caches evicted), and warm resets. |
| `tests/limit_check.cmake` | The limit at its boundary, in a second build tree. Its size comes from the other sections of a program like test_reload. N overlays (255 sections in all) must link and run. With N + 1, the pre-link check must stop the link (under LTO, warn before it). N + 1 without the check must fail in ld64, and not because a symbol is missing. |
| `overlay_budget` (a test in `CMakeLists.txt`) | The real sizes in the split layout (201 overlay sections) must be refused at a budget of 150. |
| `tests/cross_check.cmake` | Compiles `kinds.c` and `ovsec.c` for x86-64 and AArch64 Linux, Windows MSVC and MinGW, and through clang-cl's `/FI`, then checks the sections with `llvm-readobj`. It also checks that the forced include refuses a compiler that isn't clang. |
| `scan_real.py` | Compiles all 1,058 real overlay files (src/ and lib/) with the host flags, the compile probe's shim and the forced include. It reports where every static-storage object lands and lists any writable one outside its overlay's sections. It reads objects, so it is meaningful only without LTO. |

```sh
cmake -S port/spikes/overlay-sections -B build/port-spikes/overlay-sections/apple -DCMAKE_C_COMPILER=/usr/bin/clang
cmake --build build/port-spikes/overlay-sections/apple && ctest --test-dir build/port-spikes/overlay-sections/apple
# Options: -DOVSEC_SANITIZE=ON (ASan+UBSan), -DOVSEC_LTO=ON (ThinLTO), -DOVSEC_COVERAGE=ON (source-based
# coverage), -DOVSEC_LTO_COMBINED_FIX=OFF, -DCMAKE_BUILD_TYPE=Debug, -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang
.venv/bin/python port/spikes/overlay-sections/scan_real.py . build/port-spikes/overlay-sections/real --mode split --opt=-O0
```

Toolchains: Apple clang 21.0.0 (clang-2100.1.1.101), Homebrew clang 23.1.3 and Apple ld-1267, which both compilers
link with. All 17 tests pass in 10 configurations. Apple and Homebrew clang each run RelWithDebInfo, ASan+UBSan,
Debug with ASan+UBSan and coverage (the configuration with the most sections), and ThinLTO. Apple clang also runs
Debug and ThinLTO with ASan+UBSan and coverage. With ThinLTO and `OVSEC_LTO_COMBINED_FIX=OFF`, 8 tests fail as
expected (below): the 3 run-time tests of combined overlays, the 4 image tests of the programs with combined overlays,
and `section_limit`, whose program at the limit uses combined overlays.

## Results

### The pragma on Mach-O

- `#pragma clang section data="__DATA,__ov012_d" bss="__DATA,__ov012_b,zerofill"` works with both compilers.
  Without `,zerofill` the bss section is a regular section: correct, but its zeros take space in the file. Section and
  segment names have 16 bytes each (`sectname[16]` in `<mach-o/loader.h>`). `__ovNNN_d` uses 9.
- One name for both kinds (`data="__DATA,__ov012" bss="__DATA,__ov012"`) is a compile error: "this causes a section
  type conflict with a prior #pragma section". The combined layout instead sets only `data=` and compiles the
  overlay files with `-fno-zero-initialized-in-bss`, which classifies zero-initialized objects as data.
- Adjacent string literals are accepted (`data="__DATA," "__ov012_d"`).
- `-fcommon` doesn't defeat it: a tentative definition under the pragma is emitted into the bss section, not as a
  common symbol.

### What goes where

`test_kinds` output, which matches between the layouts, compilers and configurations (combined: bss → data):

| Object | Section |
| --- | --- |
| initialized global, initialized static, `char[]` from a string, pointer table, pointer to a literal, struct with a pointer | data |
| `= 0`, no initializer, tentative definition (`-fno-common`), 16 KB array | bss |
| function-local static `= 3` / `= 0` | data / bss |
| a `static inline` function's local static (the header case) | bss, in each including file's overlay |
| `const` scalar, table, zero-initialized `const` | outside (`__TEXT,__const`) |
| `const` pointer table (relocations) | outside (`__DATA,__const`, read-only after fixups) |
| function-local `static const`, string literals | outside (`__TEXT,__const`, `__TEXT,__cstring`) |
| file-scope compound literal (`int *p = (int[]){1, 2};`) | outside (`__DATA,__data`): **not reset** |
| automatic arrays under `-ftrivial-auto-var-init=zero` | no global created (an initializer is a private constant in `__TEXT,__const`) |

The compound literal is the pragma's one gap, on every object format, because the pragma applies to declarations and a
compound literal has none. On ELF it lands in `.data..compoundliteral`, on COFF in `.data`.

### The optimizer and LTO

- At `-O2`, GlobalOpt turns a static that is never written into a constant and moves it to `__TEXT,__const` or a
  literal pool. That's harmless, since there is nothing to restore. When it shrinks a global to a bool or splits a
  struct or array (SRA), the new globals keep the section. But SRA can move the zero elements of an initialized array
  into the bss section: in a first version of the synthetic overlays, every data-only overlay gained a bss section
  that way, pushing the image past 255 sections. On the host a data-only overlay can therefore cost two sections, and
  only the real link's count is meaningful.
- **ThinLTO and full LTO**: the split layout survives unchanged. The combined layout loses every zero-initialized
  object to `__DATA,__bss` or `__DATA,__common`, because the LTO back end classifies globals again without the files'
  `-fno-zero-initialized-in-bss`. Clang's documentation says "the back end decides each global's section kind, then
  applies the user's name for that kind". Passing `-Wl,-mllvm,-nozero-initialized-in-bss` at link time fixes it, for
  the whole program: main's bss moves into the file, and so do the bss objects of any split-layout overlays linked
  with it (test_kinds' overlay 900). This is the known problem of [D12511][d12511].
- Without that flag the failure is silent at run time: bench_combined, which doesn't check values, used to pass. The
  link-map check now names every object LTO moved, for example `Ov011_Lazy.sLazy` in `__DATA,__bss`: 102 objects
  in the real-size combined set and 661 in the 220-overlay one.
- The restore must be opaque to the optimizer. When the bounds come from link-time symbols declared in C
  (`extern char start[] __asm("section$start$...")`), clang treats a `memcpy` into `start` as unable to modify
  another global. At `-O1` it forwarded a value stored before the restore past it (`gA` read 42 after the restore).
  Declared as scalars (`extern char start`), the size computation is UB and clang emitted a trap (exit 133). The
  sketch takes its bounds from load commands at run time, keeps `OvSec_Reset` `noinline` and ends it with
  `asm volatile("" ::: "memory")`, which also covers LTO inlining it.

### Real game code

`scan_real.py` was run over every file that the delinks assign to an overlay: 1,058 files, of which 1,010 compile with
the shim. The 48 failures are 35 in ov036, 11 in DS Protect and 2 in DWC, all shim-level. The flags were PLAN §4.4's:
`-std=gnu99 -fsigned-char -fwrapv -fno-strict-aliasing -ffp-contract=off -ftrivial-auto-var-init=zero -fno-common`.

| Compiler, level | Objects in overlay sections | Leaks | Overlays with a section | Sections, split (data + bss) | Combined |
| --- | --- | --- | --- | --- | --- |
| Homebrew 23, `-O2` | 534 | 0 | 64 | 86 (55 + 31) | |
| Apple 21, `-O2` | 537 | 0 | 65 | 87 (56 + 31) | |
| Homebrew 23 and Apple 21, `-O0` | 675 | 0 | 93 | 119 (86 + 33) | 93 |

A leak is a writable object outside its overlay's sections, in `__DATA,__data`, `__bss` or `__common`. There are
none, so the game has no file-scope compound literals in overlay code. Everything else is constant: 1,550 to 1,655
objects in `__DATA,__const`, about 3,600 to 3,900 in `__TEXT,__const`, plus literal pools and strings.

The DS has 167 overlays with `.data` and 34 with `.bss`: 168 with either, 201 sections in the split layout. That is
not a bound for the host. SRA can give an overlay with only initialized data a bss section too (above), so the host's
bound is 2 sections per overlay with writable data: 336 split, or 168 combined. Today's code is far from it, because
74 of the DS's `.data` overlays have no writable data on the host. Most have 32 bytes of `.data` (strings and
padding), and some are `const` tables that MWCC put in `.data`. For example, `br_res.c`'s `static const u32`
palette tables fill ov271's `.data`.

### Section count and the 255 limit

- **The limit is per image.** `n_sect` in `nlist_64` is one byte: `MAX_SECT 255 /* 1 thru 255 inclusive */` in
  `<mach-o/nlist.h>`. The sections are numbered across all segments in load-command order, so a second segment
  doesn't help (tested: 260 sections over `__DATA` and `__OVSEG` fail).
- **At the boundary** (`limit_check.cmake`, every configuration): 255 sections link and run, 256 don't. With
  RelWithDebInfo that is 246 overlay sections plus 9 others, against 247. In the worst configuration it is 230 plus 25.
- **Past 255, ld64 never names the cause, and sometimes crashes.** Seen with ld-1267: "fixup error
  (kind=arm64_was_adrp_ldr_got_elide_got) ... ADRP out of range ... to 0x00000000" (RelWithDebInfo, Debug, ThinLTO);
  "Assertion failed: (aliasSectionNum == sectionNum && "alias and its target must be located in the same section"),
  function assignAliasAtomOffsetInSection, file Layout.cpp" (Homebrew, ASan); and a crash, "clang: error: unable to
  execute command: Bus error: 10" (Apple ASan, both worst configurations, ThinLTO with ASan). The crash leaves an
  80 MB "linker snapshot" in `$TMPDIR`, so `limit_check.cmake` points `TMPDIR` into its build tree. Earlier probes also
  got "pointer not aligned in 'anon-1' (stubs-got-file)" and "Assertion failed: (addr + content.size() <=
  sectionEndAddr) ... Layout.cpp". One object file with more than 255 sections crashes ld in `makeCustomSection`.
  No executable exists after any of these, so a check after the link can't explain them. Hence the pre-link check.
- **The other sections vary with the configuration**, measured in the spike's programs:

  | Configuration | Other sections |
  | --- | --- |
  | RelWithDebInfo, Debug (Apple, Homebrew) | 9 |
  | ThinLTO | 7 to 9 |
  | ASan+UBSan | 15 (Homebrew) to 16 (Apple) |
  | Debug, ASan+UBSan and coverage (also with ThinLTO) | 24 (Homebrew) to 25 (Apple) |

  The game will have more. SDL3 linked statically adds about 17 that these programs lack: `libSDL3.dylib` has 28
  sections, among them 14 Objective-C sections, `__cfstring`, `__ustring` and `__gcc_except_tab`. Dear ImGui's C++,
  Lua, the network libraries and thread-local storage add a few more. Plan on 45 to 50 other sections in the worst
  configuration.
- **The overlay sections vary too**: 86 at `-O2` and 119 at `-O0` with today's code (+33), and the split bound is
  336. The combined layout's bound, 168, plus 50 others is 218, so combined fits in every configuration for the life
  of the project. Split does not.
- **The budget: 150 overlay sections, in every configuration.** `predict --overlay-budget 150` fails above it, with a
  message that says to switch to the combined layout. Because every configuration checks its own count, the default
  build fails long before the worst one would: at 150 split sections in an `-O0` build (about 110 at `-O2`), the
  worst configuration has about 150 + 50 = 200 sections, 55 below the limit. Real need today: 86 to 119 split, or 64
  to 93 combined.

### Counting sections before the link

`check_sections.py predict` runs as a CMake `PRE_LINK` step, which CMake runs "after sources have been compiled but
before linking the binary" ([add_custom_command][cmakeprelink]), inside the link step. Each program also lists the
script in `LINK_DEPENDS`, so a change to the check relinks it.

- **Inputs**: the program's objects, its object libraries, its static libraries, and the runtime archives that the
  compiler driver adds itself: the builtins (`libclang_rt.osx.a`) always, and the profile runtime with coverage.
  CMake asks the driver for them at configure time with `clang -###`, which prints the link command without running
  it. The runtime archives are universal files, so the script takes the host architecture's slice.
- **Archives** are read member by member, and a member counts only if ld64 would load it: it defines a symbol that
  the loaded objects leave undefined, repeated until nothing changes. Symbols that dynamic libraries define aren't
  known to the script. So it loads `chkstk_darwin` from the builtins, which a system dylib (`libcompiler_rt`)
  provides first. That member brings no new section.
- **Output names**: ld-1267 merges `__TEXT,__literal4/8/16` into `__TEXT,__const` and C++'s `__TEXT,__StaticInit`
  into `__text`. It moves `__DATA,__const` and `__mod_term_func` to `__DATA_CONST`, and turns `__mod_init_func`
  into `__TEXT,__init_offsets`. It drops `__LD` (compact unwind) and the debug sections. It creates
  `__TEXT,__stubs`, `__DATA_CONST,__got` and `__TEXT,__unwind_info` itself, and creates an empty section for every
  `section$start$SEG$SECT` reference to a section that no input has. The profile runtime refers to 7 coverage
  sections that way, 3 of which (`__llvm_prf_bits`, `__llvm_prf_vns`, `__llvm_prf_vtab`) no input has. All of this
  was measured, not documented, so a new ld64 can change it. The image test catches that.
- **Result**: without LTO the prediction equals the image's count for every program in every configuration (7
  programs, 7 configurations).
- **Under LTO** the inputs are bitcode, whose sections the back end picks at link time, and `nm` shows only whether a
  module has writable data. The script computes a range. An overlay module with writable data brings at least 1 and
  at most all of its overlay's sections (split: 2). Any other module brings up to 7 standard sections, plus room for
  instrumentation's own sections (5 for ASan, 7 for coverage, measured from native objects). It fails when the lower
  end is past 255 and warns when the upper end is. Measured with ThinLTO: combined, 172 to 178 against 175 actual;
  split with real sizes, 178 to 346 against 209; with ASan and coverage, 186 to 198 against 193 and 192 to 366
  against 226. Combined stays within 6 to 12 sections, but split near the limit can only be warned about. At the
  boundary under LTO, the N + 1 link gets the warning and then ld64's error.

### Finding leaks after the link

`check_sections.py image` reads the ld64 map (`-map`: "details all symbols and their addresses in the output image",
`man ld`). The map lists the input objects, the sections and every symbol with its object. A symbol from an overlay's
object in `__DATA,__data`, `__bss` or `__common` is a leak. One in another overlay's section is an error, and so is
a non-overlay object's symbol in an overlay section. The overlays file says which objects belong to which overlay.

- **Under ThinLTO** the map gives a module's external symbols to the bitcode file and its local ones to an LTO
  object. With `-Wl,-object_path_lto,DIR` ld64 keeps those objects ("the temporary file will be stored at the
  specified path", `man ld`) as `DIR/N.arm64.thinlto.o`, N being the module's position among the bitcode inputs.
  Without it they are unnamed. The script maps each one back by the external definitions in its own symbol table. Full
  LTO merges every module into one object, so local symbols can't be attributed: the leak check needs ThinLTO or no
  LTO.
- **Results**: in every configuration it finds exactly the two compound literals of test_kinds and nothing in the
  synthetic overlays. Under ThinLTO without the link flag it fails the 4 programs with combined overlays (above).
- **Instrumentation** puts its own writable objects outside the pragma's reach. The check skips three kinds in
  sanitizer builds: UBSan's check data (`l___unnamed_N`, source locations and type descriptors that its runtime writes
  to report each check once), Homebrew clang's ASan ODR indicators (`___odr_asan_gen_NAME`), and under LTO
  `__MergedGlobals`, where GlobalMerge combines UBSan's data. A compound literal could hide among merged globals too,
  so the builds without sanitizers are the ones whose leak check counts. Coverage counters and ASan's metadata have
  sections of their own.
- `scan_real.py` stays as an object-level research tool. An LTO object is bitcode without sections (`llvm-objdump`
  rejects it), and the LTO failure above happens only at link time.

### Finding the bounds

- **Load commands (recommended).** Walk `LC_SEGMENT_64` from the image's own header `&__dso_handle`, compute the
  slide from `__TEXT`'s vmaddr, and pick the `__DATA` sections named `__ovNNN[_d|_b]`. That takes one pass, about
  10 µs for 201 sections, and needs no generated table. It works if the game is ever a dylib, which
  `_dyld_get_image_header(0)` would not handle. `getsectiondata(&__dso_handle, "__DATA", name, &size)` gives the
  same results. The macOS 26 SDK marks `getsectbynamefromheader_64` deprecated ("use getsectiondata()") in
  `<mach-o/getsect.h>`.
- **ld64's `section$start$__DATA$name` / `section$end$...`** work, and the tests check them against the scan on 64
  sections. They are poor for discovery. A reference to a section that doesn't exist makes ld64 create an empty one,
  which spends a section slot. Worse, ld-1267 makes one internal section per such symbol and asserts past 255 of them
  ("custom section index overflow"), so they cover at most 127 sections with both bounds. These symbols aren't in
  `man ld`; they are known from the ld64 sources and [this LLVM thread][ld64syms].
- **Snapshot at run time, never from the file.** With chained fixups the file's bytes in a data section are fixup
  chains, not pointers. The snapshot, taken after dyld has run, holds slid pointers, and `test_reload` checks that a
  restored `static int *p = &x` equals `&x`.

### Restore cost

`bench_restore` uses the DS sizes: 168 overlays, 87,808 bytes of data and about 221 KB of bss. Loads happen at scene
transitions; a frame is 16.7 ms. Three measurements per reset. A **first load** is the first reset after the
snapshot, and pays the page faults of first touch: zero-fill pages for bss, copy-on-write for data pages that dyld
didn't write. A **later load** comes after writing 64 MB, which evicts the overlay and the snapshot from the caches.
**Warm** is 2,000 resets of the same overlay in a row, which the first version of this table reported: a lower
bound, not a load. Ranges are over 5 runs each on an idle machine.

| Build | Snapshot (once) | Overlay 36 (69 KB): first, later, warm | All 168 once: first, later, warm |
| --- | --- | --- | --- |
| split, RelWithDebInfo (Apple, Homebrew, ThinLTO) | 32–370 µs, 88 KB | 9.4–13 µs, 1.0–11 µs (mostly 1–2), 0.48–0.85 µs | 32–42 µs, 10–50 µs (mostly 10–15), 3.9–5.1 µs |
| split, Debug | 36–210 µs | 10–15 µs, 1.2–12 µs, 0.55–0.65 µs | 36–130 µs, 13–33 µs, 4.8–5.2 µs |
| combined, RelWithDebInfo | 90–1,700 µs, 310 KB | 12.8–16 µs, 2–14 µs, 1.0–2.0 µs | 50–66 µs, 11–44 µs, 4.9–7.5 µs |
| split, ASan+UBSan | 0.32–0.62 ms, 125 KB | 42–81 µs, 39–82 µs, 36–50 µs | 0.35–0.52 ms, 0.34–0.50 ms, 0.33–0.61 ms |
| combined, ASan+UBSan | 0.9–1.7 ms, 404 KB | 168–235 µs, 154–218 µs, 172–240 µs | 0.78–1.1 ms (one run 6.4 ms), 0.73–1.2 ms, 0.80–1.06 ms |
| split, Debug with ASan+UBSan and coverage (Apple) | 0.74–0.85 ms | 280–314 µs, 290–411 µs, 252–381 µs | 1.5–2.6 ms, 1.6–2.1 ms, 1.4–2.1 ms |

A first load costs 10 to 25 times the warm figure, as the review measured (first reset of overlay 36 11 to 13 µs,
all 168 32 to 44 µs), and a later load about twice. It is still negligible: 0.1% of a frame for the largest overlay's
first load. The split layout is faster than combined, because bss is cleared with `memset` instead of copied from
zeros, and its snapshot is a third of the size. The Debug build with coverage runs the ASan copy loop unoptimized and
counts every iteration, which makes it about 7 times slower than at `-O2`.

### Sanitizers

- ASan gives every global in the overlay sections a redzone. Overlay data grows by about 40%; with the real sizes the
  split data goes from 87,808 to 124,864 bytes. A `memcpy` or `memset` over a section then reports a
  global-buffer-overflow. It does so even when called from a `no_sanitize("address")` function, because ASan
  intercepts the libc functions themselves.
- The sketch copies with a loop in a function marked `no_sanitize("address")` and `no_builtin("memcpy", "memset")`,
  so it can't become a libc call again. Copying the redzones changes nothing, and an out-of-bounds read of an
  overlay global in instrumented code is still reported (checked in the probe). A cross-compiled ELF object shows the
  same redzones (`gInit900` is 32 bytes in `ov900_d`), so the same copy is needed on Linux.
- UBSan has no effect on placement. Every test passes under `-fsanitize=address,undefined` with both compilers, also
  in Debug with coverage and with ThinLTO.

## Linux and Windows

`cross_check.cmake` compiles `kinds.c` (overlay 900, split) and `ovsec.c` for `x86_64`/`aarch64-unknown-linux-gnu`,
`x86_64`/`aarch64-pc-windows-msvc` and `x86_64-w64-windows-gnu`, and `kinds.c` again through clang-cl. Every target
puts the objects where expected:

| Target | Data, zero/tentative | `const` table | `const` pointer table | Bounds |
| --- | --- | --- | --- | --- |
| ELF (x86-64, AArch64) | `ov900_d` (PROGBITS), `ov900_b` (NOBITS) | `.rodata` | `.data.rel.ro` | weak `__start_ov900_d` etc. |
| COFF (MSVC x86-64/AArch64, MinGW, clang-cl) | `.data$ov900m`, `.bss$ov900m` (UNINITIALIZED_DATA) | `.rdata` | `.rdata` | markers `.data$ov900a` / `.data$ov900z` |

Nothing was linked or run: this Mac has no ELF or PE linker (`ld.lld` and `lld-link` aren't installed).

- **The pragma.** Clang documents `#pragma clang section` as meaningful on ELF, Mach-O and COFF, with kinds `bss`,
  `data`, `rodata`, `relro` and `text`, and an explicit `section` attribute taking precedence
  ([Clang language extensions][clangext]). The spike shows that it covers function-local statics too.
  MSVC-style `#pragma data_seg`/`bss_seg` give the same placement with clang on the Windows targets (checked);
  [Microsoft's documentation][dataseg] says `data_seg` covers initialized data and `bss_seg` uninitialized data.
- **The forced include under clang-cl** is `/FI<path>`. clang-cl's option list has "/FI <value> Include file before
  parsing", and "Options that are not known to clang-cl will be ignored by default" ([clang-cl][clangcl]). So
  `-include` gets "unknown argument ignored in clang-cl: '-include'", and the header path then counts as a second
  source file ("cannot specify '-Fo...' when compiling multiple source files"). The build picks the spelling from
  `CMAKE_C_COMPILER_FRONTEND_VARIANT`, which is `MSVC` for clang-cl ([CMake][frontend]). Checked: `/FI` puts
  `gInit900` in `.data$ov900m`. On macOS a source path must follow `--`, or clang-cl reads `/Users/...` as its `/U`
  option.
- **GCC**, PLAN §4.4's extra check, has no `#pragma clang section`. GCC's list of pragmas has nothing that sets a
  default data section ([GCC pragmas][gccpragmas]), and an unknown pragma gets only a warning, `-Wunknown-pragmas`
  ([GCC warnings][gccwarn]). Every overlay's statics would stay in `.data` and `.bss`. On ELF the weak bounds would
  resolve to NULL and every reset would silently do nothing; on COFF the markers would bracket nothing. So the forced
  include stops with `#error` unless the compiler is clang or the build defines `OVSEC_DISABLED`, which builds
  `ovsec.c` without overlay sections. Checked with `-U__clang__` standing in for GCC, which this Mac doesn't have.
- **ELF bounds.** "If an output section's name is the same as the input section's name and is representable as a C
  identifier," GNU ld provides `__start_SECNAME` and `__stop_SECNAME` ([GNU ld manual][gnuld]); lld and gold do the
  same. So the ELF names are `ovNNN_d`/`ovNNN_b`, with no dot. Since lld 13 the default is `-z start-stop-gc`, under
  which a `__start_` reference no longer keeps those sections alive ([lld][startstop]). Our sections are kept by the
  overlay code's own relocations, and the sketch declares the bounds `weak` so that an overlay without writable data
  links. Section count is not a concern on ELF, where indices past `SHN_LORESERVE` use `SHN_XINDEX`
  ([ELF gABI][gabi]).
- **COFF bounds.** The linker drops "$" and what follows from an object section's name and sorts the contributions
  "in lexical order by object-section name" ([PE format, Grouped Sections][pe]). So `.data$ov012a` (begin marker) <
  `.data$ov012m` (the overlay) < `.data$ov012z` (end marker), all inside `.data`. The MSVC CRT finds its initializers
  the same way, between `.CRT$XCA` and `.CRT$XCZ` ([CRT initialization][crtinit]). GNU ld's PE script sorts
  `*(SORT(.data$*))` ([pe.sc][pesc]), and lld-link groups by name as link.exe does ([D45801][d45801]). The sketch
  ignores an empty range, so an overlay without writable statics, whose markers end up next to each other, has no
  section.
- **PE section count.** "The Windows loader limits the number of sections to 96", and image section names have at
  most 8 characters ([PE format][pe]). Distinct image sections per overlay (168) would not load, which is why grouped
  sections are the only COFF design: every overlay merges into `.data` and `.bss`, adding no image sections. So the
  section count and `check_sections.py` are Mach-O concerns only. The leak check would need a map reader for lld's
  and link.exe's map formats.
- **Untested on those platforms:** linking, the `$`-sorting with `-fdata-sections` (in MinGW mode lld strips COMDAT
  suffixes from `.data$sym` names: [lld commit][mingwcomdat]), ASan on Windows, and the alignment padding between the
  markers (the sketch includes it in the range, which is harmless).

## Recommended design for the real HAL

1. **Generator** (`port/tools/gen_sources.py`): for every C file that the delinks assign to overlay NNN, add the
   forced include as a per-source compile option (CMake `set_source_files_properties(... COMPILE_OPTIONS)`):
   `/FI<gen>/ovNNN_sections.h` when `CMAKE_C_COMPILER_FRONTEND_VARIANT` is `MSVC` (clang-cl), and
   `-include <gen>/ovNNN_sections.h` otherwise. Write the 344 headers, each choosing Mach-O, COFF or ELF names under
   `#if` and stopping with `#error` for a compiler other than clang unless `OVSEC_DISABLED` is defined, as
   `gen_overlays.py` does. Write `ovsec_list.h` with `OVSEC_OVERLAY(NNN, id)` for every overlay that has files (ELF
   and COFF need it; Mach-O doesn't), and the overlay of every overlay object for the checks. Main, ITCM, DTCM and
   autoload files get no forced include. A CMake option chooses split (default) or combined: combined adds
   `-fno-zero-initialized-in-bss` to overlay files and, under LTO, `LINKER:-mllvm,-nozero-initialized-in-bss`.
2. **Runtime** (`port/hal/fs/ovsec.{h,c}`, as in `ovsec/`): `OvSec_Scan()` and `OvSec_Snapshot(buffer)` run first in
   the HAL's startup, before any game code, with the buffer from the HAL's own arena (88 KB at DS sizes, about
   125 KB under ASan). A scan that finds no overlay section stops the HAL, since the forced includes didn't take
   effect; an `OVSEC_DISABLED` build says at startup that overlays aren't reset. `OvSec_Reset(id)` restores data,
   zeroes bss and ends with a compiler memory barrier. Compile `ovsec.c` at `-O2` in every configuration, for the ASan
   copy loop.
3. **Hook point:** the host versions of NitroSDK's overlay functions, listed in `hal/replaced.txt`. `GFL_OvlLoad`
   (`src/gfl/gf_overlay.c`) calls `FS_LoadOverlay`, which calls `FS_LoadOverlayInfo`, `FS_LoadOverlayImage` and
   `FS_StartOverlay`. The host `FS_LoadOverlayImage` calls `OvSec_Reset(info->id)` where the DS clears bss and reads
   the file. The host `FS_StartOverlay` skips the digest and the static initializers: only ov165 has any (DS Protect,
   stubbed). `FS_UnloadOverlayImage` does nothing. `FS_LoadOverlayInfo` keeps reading the ROM's overlay table. No
   game file changes.

   **Invariant: the game's overlay bookkeeping stays as the DS runs it.** A reset is right only when it happens
   exactly where the DS would copy the overlay, and that rests on `GFL_OvlCheckMemoryCollision`. `GFL_OvlLoad` returns
   FALSE when the check fails, before it reaches `FS_LoadOverlay`. The check compares the ROM table's address range
   of every loaded entry, so an overlay that is already loaded overlaps itself and is refused. `FS_LoadOverlay` is
   the only caller of `FS_LoadOverlayImage` (`lib/nitro/src/fs/fs_overlay.c`), and nothing outside `gf_overlay.c`
   calls `FS_LoadOverlay`. So the `return FALSE` path must stay. The `GFL_ASSERT` in it can be made non-fatal, but
   the refusal can't be removed. Without it, a second `GFL_OvlLoad` of a resident overlay would reset live statics
   that the DS keeps. `GFL_OvlUnload` must also keep clearing its entry, or the next load would be refused and never
   reset. Only `FS_UnloadOverlayImage` is a no-op on the host. This corrects the last bullet of engine-boot.md §3.3
   (Handoff, below).
4. **Guards** (macOS; the section count is a Mach-O concern only):
   - **Before the link, in every configuration**: `check_sections.py predict` as a `PRE_LINK` step of the game
     executable, with every input: the object libraries, static libraries such as SDL3, and the driver's runtime
     archives from `clang -###`. List the script in `LINK_DEPENDS`. It fails past 255 with a message that names the
     cause, instead of ld64's errors or crash. It also fails past the budget of 150 overlay sections, so the default
     build asks for the combined layout long before the configuration with the most sections would break. Under
     LTO it fails when the range's lower end is past 255 and warns when the upper end is. Prefer combined in an LTO
     configuration.
   - **After the link (T1)**: `check_sections.py image` with the map (`-Wl,-map`, plus `-Wl,-object_path_lto,DIR`
     under ThinLTO). It checks the prediction against the image and finds leaks, with LTO too. Its verdict counts in
     the builds without sanitizers. It catches file-scope compound literals and any future placement gap, including
     the combined layout under LTO without the link flag.
   - Debug builds log `OvSec_SectionCount()` and `OvSec_ImageSectionCount()` at startup.

## Alternatives if the limit bites

In order of preference. None is needed at the measured 86–119 sections.

1. **Combined layout**, one section per overlay: at most 168 (the DS bound), 93 measured, 218 with the worst
   configuration's other sections. It costs bss bytes in the file (221 KB), a 310 KB snapshot, resets that copy zeros
   (about twice the split cost, still about 15 µs for ov036's first load), and the LTO link flag, which applies to
   the whole program. Tested here with all the others.
2. **One shared section, bounded by link order.** All overlay data goes to `__DATA,__ovdata` (with
   `-fno-zero-initialized-in-bss`), and the link line brackets each overlay's objects with marker objects
   (`__attribute__((used, section("__DATA,__ovdata")))`). `man ld` says the linker lays out functions in
   object-file order when there is no `-order_file`, and the probe shows the same for data. Probe: 300 overlays ×
   2 files linked into one section, every object between its overlay's markers, with and without `-dead_strip`,
   7 image sections in all. It's fragile: it needs the objects linked directly in overlay order (members of a static
   archive are laid out in resolution order) and the markers `used`. Use it only past 255.
3. **Group overlays that share a base address.** This is sound only if every pair in a group overlaps, so that no two
   can be loaded together. The collision check has to stay on, which design item 3 requires anyway. Resetting the
   unloaded members of a group then changes nothing observable, since each one is reset again on its next load. It's
   not possible for chains (A overlaps B, B overlaps C, A and C can coexist), and it breaks if the port keeps an
   overlay resident. That fragility is why it ranks last; the brief's "not possible" holds for any grouping that
   isn't of that kind.
4. **A segment per overlay** doesn't help (above). **A dylib per group of overlays** would give each its own
   255-section budget, at the cost of cross-image references and symbol interposition. Not worth it.

## Open risks

- **The section budget isn't fixed.** Upstream lands C every day; `-O0`, sanitizers, coverage and statically linked
  libraries all add sections. The pre-link check stops every configuration at 255 and asks for combined at 150 overlay
  sections; combined (at most 218 in the worst configuration) settles it for good.
- **The pre-link count depends on ld64's measured behavior**: which sections it merges, renames or creates, and which
  archive members it loads. The script ignores dylib precedence. A new ld64 could change any of it. The image test
  compares the prediction with every linked program, so a change shows up as a failing test, not as a silent miss.
- **Split under LTO** can only be bounded before the link: 178 to 346 for the real sizes. Near the limit the check
  warns and ld64 gives its own error.
- **File-scope compound literals** escape the pragma on every format. There are none today; the leak check catches
  new ones in builds without sanitizers. The fix would be a `PLATFORM_PC` rewrite of that line (a game-changes.md
  row).
- **Writes to `const` data.** On the DS, `.rodata` is ordinary RAM, and MWCC even put some `const` tables in
  `.data`. On the host a write through a cast to a `const` table faults (`__TEXT,__const` is read-only). That isn't
  a reset problem, but this classification makes it visible. No case is known.
- **LTO with the combined layout** silently stops resetting zero-initialized statics unless the link flag is present.
  The combined tests and the leak check catch it.
- **ELF and COFF** are compile-checked only. Linking, the `$` order with `-fdata-sections` and MinGW COMDAT names,
  and ASan's interception on Windows need a run on those hosts. GCC was simulated with `-U__clang__`.
- **The DS's unload isn't reproduced.** After an unload, the host keeps an overlay's statics instead of letting the
  next overlay overwrite them. engine-boot.md §3.3 explains why this direction is safe; a game bug that reads an
  unloaded overlay's data would behave differently.
- **ASan builds reset slowly** (40 to 80 µs for ov036, about 1 GB/s; 0.3 ms in Debug) because the copy must avoid
  the intercepted libc functions. That's acceptable for nightly T5 runs.
- **A link past 255 can crash ld64**, leaving an 80 MB snapshot in `$TMPDIR`. The pre-link check prevents it.

## Handoff

- **engine-boot.md §3.3** (owned by another session): its last bullet recommends "keep `GFL_OvlUnload` a no-op, and
  turn the collision assert off". With per-overlay resets that is wrong on both counts (design item 3). The
  collision check's `return FALSE` must stay, and `GFL_OvlUnload` must keep clearing its entry. Only
  `FS_UnloadOverlayImage` becomes a no-op, and the `GFL_ASSERT` may become non-fatal. Suggested wording: "emulate the
  DS's load by resetting the overlay's `.data` from a pristine copy and zeroing its `.bss` in the host
  `FS_LoadOverlayImage`; keep `GFL_OvlLoad`, `GFL_OvlUnload` and the collision check unchanged, so that a reset happens
  exactly when the DS would copy; make `FS_UnloadOverlayImage` a no-op."

## Files

- `CMakeLists.txt`, `gen_overlays.py`, `check_sections.py`, `scan_real.py`
- `ovsec/ovsec.h`, `ovsec/ovsec.c`: the runtime sketch
- `tests/ovtest.h`, `tests/test_reload.c`, `tests/kinds.h`, `tests/kinds.c`, `tests/test_kinds.c`,
  `tests/bench_restore.c`, `tests/limit_check.cmake`, `tests/cross_check.cmake`

[clangext]: https://clang.llvm.org/docs/LanguageExtensions.html
[clangcl]: https://clang.llvm.org/docs/UsersManual.html#clang-cl
[frontend]: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_FRONTEND_VARIANT.html
[cmakeprelink]: https://cmake.org/cmake/help/latest/command/add_custom_command.html#build-events
[gccpragmas]: https://gcc.gnu.org/onlinedocs/gcc/Pragmas.html
[gccwarn]: https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
[d12511]: https://reviews.llvm.org/D12511
[ld64syms]: https://discourse.llvm.org/t/lld-support-for-ld64-mach-o-linker-synthesised-symbols/45145
[gnuld]: https://sourceware.org/binutils/docs/ld/Input-Section-Example.html
[startstop]: https://lld.llvm.org/ELF/start-stop-gc
[gabi]: https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.sheader.html
[pe]: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
[crtinit]: https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization
[dataseg]: https://learn.microsoft.com/en-us/cpp/preprocessor/data-seg
[pesc]: https://gnu.googlesource.com/binutils-gdb/+/refs/heads/master/ld/scripttempl/pe.sc
[d45801]: https://reviews.llvm.org/D45801
[mingwcomdat]: https://llvm.googlesource.com/lld/+/2bce47dd9660e14d289b3906b2878e7a56ecc727%5E%21/
