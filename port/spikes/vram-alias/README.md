# Spike: VRAM bank mapping by page aliasing

**Frozen.** This spike is a record: its answer is the live code, [port/hal/gx](../../hal/gx/vram.h) (`vram.c`,
`vram_layout.c`, `vram_os.c`), with its tests in [port/tests/gx](../../tests/gx/test_gx.c). Nothing here is built by
Frigate's build or CI, and nothing here changes with the live code ([../README.md](../README.md)).

PLAN.md §4.5 says each VRAM bank becomes shared memory mapped into every view it is assigned to, with copy-on-remap as
the fallback where aliasing is awkward. This spike tests that on this Mac and checks Linux and Windows against their
documentation. A review found several claims of the first version wrong; this version corrects them, and the sections
below say what changed where it matters.

## The question

Game code writes VRAM through pointers (`G2_GetBG1CharPtr()`, `HW_BG_VRAM`, `HW_LCDC_VRAM_A`), and `VRAMCNT` moves banks
between the views those pointers point into. A write through BG-A must be in the bank when it moves to LCDC. Can the
host get that from the virtual memory system on macOS, Linux and Windows? What does a remap cost, at a scene change
and every frame? What do views read where no bank is mapped? Does it work under ASan? What is the fallback, and does
it behave the same?

## Answer

- **Aliasing works on macOS**, three ways (POSIX `shm_open` + `mmap(MAP_FIXED)`, a Mach named entry +
  `mach_vm_map(VM_FLAGS_OVERWRITE)`, and `mach_vm_remap(copy = FALSE)`). All three cost about the same, and all pass
  the same tests as a copying backend, under ASan and UBSan too.
- **Remaps happen every frame, not only at scene changes.** `NNS_GfdDoVramTransfer` runs in every VBlank, and each 3D
  texture or palette task takes its banks into LCDC and back (two remaps per task). The dual 3D demos swap C and D
  every frame. Battle's heaviest case measured here (4 texture and 4 palette tasks a frame) costs 63 µs with aliasing
  and 160 µs with copying: 0.4% and 1.0% of a 16.7 ms frame. A scene change costs 47 µs and 144 µs, or 23 µs and
  32 µs when `GFL_BGSysSetVRAMBanks`' 20 SDK calls are applied as one batch.
- **Copying is not equivalent for everything game code does.** The DS shows a bank in only one view at a time, but
  some banks repeat inside a view (mirrors), and BW2 stores through mirrors: `codein.c` and `error_window_screen.c`
  clear the whole sub BG and sub OBJ views while H and I sit there with mirrors. A uniform fill of a whole view gives
  the same bank bytes in both backends. Any other store through a mirror reaches the bank only when aliasing. 56 of
  BW2's 109 `BGSysVRAMConfig` settings pair `GX_VRAM_SUB_OBJ_16_I` with a sub OBJ character mapping of 32 KiB or more,
  battle's among them; whether any of them loads characters past I's 16 KiB has not been audited.
- **Unmapped slots must read 0 and absorb writes, not fault.** `codein.c`, `monolith_main.c` and
  `error_window_screen.c` clear the 512 KiB BG-A view with only 128 KiB mapped (`GX_VRAM_BG_128_A`).
- **Host addresses for `HW_*_VRAM` need SDK and game changes under any scheme.** The first version said a static array
  keeps the SDK and game unchanged except `screentex.c`. That is false. `gx_load3d.c` keeps the LCDC addresses in
  `static const u16` tables of `HW_LCDC_VRAM_X >> 12`, which no longer compile. `gx_load3d.c`, `gx_load2d.c` and game
  code keep VRAM addresses in `u32`, which truncates on a 64-bit host. Three game sites use raw DS addresses.
- **Windows can't use a static array for aliasing**, and the first version's design didn't even compile there: COFF
  limits a static object's alignment to 8 KiB, and the copy backend used `aligned_alloc`, which the UCRT lacks. Both
  are fixed; the library now compiles for three Windows targets (against stand-in headers, below). Windows can alias
  with placeholders on a run-time reservation, which is what the recommendation now uses.
- **Linux works like macOS** with `memfd_create`, with one more constraint: LeakSanitizer reads all of `.bss` at exit,
  so pages without access must not be left in a static array. Kernels with 64 KiB pages can't place F, G, H and I, and
  fall back to copying at run time.

## DS facts used

Each `VRAMCNT` byte holds MST in bits 0–2 (bit 2 unused for A, B, H and I), OFS in bits 3–4 and enable in bit 7
([GBATEK, DS Memory Control - VRAM][gbatek]). The places, all in 16 KiB slots:

| Bank | Size | LCDC (MST 0) | Other places |
| --- | --- | --- | --- |
| A, B | 128K | 0x06800000, 0x06820000 | BG-A +0x20000·OFS; OBJ-A +0x20000·OFS.0; texture slot OFS |
| C | 128K | 0x06840000 | BG-A +0x20000·OFS; ARM7 (MST 2); texture slot OFS; BG-B 0x06200000 (MST 4) |
| D | 128K | 0x06860000 | BG-A; ARM7; texture; OBJ-B 0x06600000 (MST 4) |
| E | 64K | 0x06880000 | BG-A 0; OBJ-A 0; texture palette slots 0–3; BG ext. palette 0–3 (lower 32K) |
| F, G | 16K | 0x06890000, 0x06894000 | BG-A/OBJ-A +0x4000·OFS.0 + 0x10000·OFS.1; texture palette slot OFS.0 + 4·OFS.1; BG ext. palette 0–1 or 2–3; OBJ ext. palette (8K) |
| H | 32K | 0x06898000 | BG-B 0x06200000; BG-B ext. palette 0–3 |
| I | 16K | 0x068A0000 | BG-B 0x06208000; OBJ-B 0x06600000; OBJ-B ext. palette (8K) |

Every offset and bank size is a multiple of 16 KiB, which is the Apple Silicon page size. melonDS, the port's oracle,
maps VRAM in the same 16 KiB units, and adds mirrors that GBATEK's table doesn't list
([GPU.cpp][melonds-cpp] `MapVRAM_FG`, `MapVRAM_H`, `MapVRAM_I`):

- F and G also appear 0x8000 above their place in BG-A or OBJ-A.
- H also appears at 0x06210000.
- I appears 4 times in BG-B (0x06208000, 0x0620C000, 0x06218000, 0x0621C000), and 8 times across OBJ-B.

Unmapped VRAM reads 0, and writes to it are dropped. Two banks on one slot OR their reads and both take writes
([GPU.h][melonds-h] `ReadVRAM_ABG`/`WriteVRAM_ABG`). The spike follows melonDS except for overlaps, which can't be
aliased. There the lower bank wins, and the layout counts the collision. GBATEK also notes that 8-bit writes to VRAM are
ignored. The host can't copy that.

## What BW2 does with VRAM

Paths are under `src/` and `lib/nitro/src/gx/`.

- **Who writes VRAMCNT.** Only `gx_vramcnt.c` (108 writes) and `gx_state.c` (`GX_InitGXState`: `VRAMCNT_ABCD` as one
  32-bit write, E, F, G, and `VRAMCNT_HI` as 16 bits). Every function in `gx_vramcnt.c` that writes VRAMCNT ends in
  `GX_VRAMCNT_SetLCDC_` or `disableBankForX_`. Game code never writes VRAMCNT itself.
- **Transient overlaps.** A `GX_SetBankFor*` call writes the new banks before it puts the old ones back in LCDC, so for
  one register write two banks can claim the same slot: going from `BG_64_E` to `BG_128_A` overlaps 4 slots.
- **Clears of partly mapped views.** `ov205/codein.c`, `ov143/monolith_main.c` and `ov184/error_window_screen.c` clear
  all four CPU views by their full sizes. With `GX_VRAM_BG_128_A`, 384 KiB of the 512 KiB BG-A view has no bank, and
  with `GX_VRAM_OBJ_64_E` (codein.c, error_window_screen.c) 192 KiB of OBJ-A has none.
- **Clears of all of LCDC are fully mapped.** `gfl/gx_layers.c` `GFL_BGSysInitVRAM` (59 callers, all passing 0 or
  `GX_VRAM_NONE`), `ov312/geonet.c`, `ov295/game_clear_save.c`, `ov295/battle_lose_message.c` and
  `ov035/event_mapchange.c` `ClearLCDCVram` first put every bank in LCDC. The first version said otherwise.
- **Stores through mirrors.** `codein.c` sets `GX_VRAM_SUB_BG_32_H` (H also at 0x06210000) and `GX_VRAM_SUB_OBJ_16_I`
  (I eight times), then clears all 128 KiB of `HW_DB_BG_VRAM` and `HW_DB_OBJ_VRAM`; `error_window_screen.c` does the
  same with `GX_VRAM_SUB_OBJ_16_I`. The first version said no BW2 code stores through a mirror.
- **Remaps every frame.** `system/machine_use.c` `SystemUpdateVBlank` calls `NNS_GfdDoVramTransfer`, and
  `lib/nnsys/src/gfd/gfd_VramTransferManager.c` runs `GX_BeginLoadTex`/`GX_LoadTex`/`GX_EndLoadTex` (or the palette
  calls) for each 3D task. `ov168/btlv_effect.c` registers one palette task per model in each step of a fade;
  `ov257/ctvt_game.c` registers 8 KiB texture tasks. `gfl/double3Ddisp.c` `GFL_G3DDual3DExecVBlank` swaps C and D
  between sub BG, sub OBJ and LCDC in every frame that renders (4 SDK calls), for the demos in ov265 and ov293.
- **VRAM addresses kept as integers**, which a host address breaks:
  - `gx_load3d.c`: `TEXPLTT_VRAM_BANK_ADDRS` and `TEX_VRAM_BANK_RANGES`, `static const u16` tables of
    `HW_LCDC_VRAM_X >> 12` (a compile error with host addresses), and `sPlttAddr`, `sTexAddr`, `sTexAddr2` and
    `sRearPlaneImageAddr`, `static u32`.
  - `gx_load2d.c`: `sBGExtPlttDestA` and `sObjExtPlttDestA`, `static u32`.
  - Game code casting VRAM pointers to `u32`: `ov168/btlv_gauge.c` (22 sites, `(void *)((u32)vram + ...)`) and
    `ov194/pokemontrade_2d.c` (2) by grep. Clang warns about every such cast by default (`-Wvoid-pointer-to-int-cast`,
    `-Wpointer-to-int-cast`, `-Wint-to-void-pointer-cast`), and `-Wshorten-64-to-32` catches the integer stores, so the
    first 64-bit build of the game lists them all.
  - Raw DS addresses: `ov036/fld3d_ci.c` (`(u16 *)0x6860000`), `ov036/field_transition.c` (`(void *)0x6800000`) and
    `ov035/event_mapchange.c` (`(void *)0x06800000`).
- **Static pointer tables.** `system/screentex.c`'s `SCREEN_CAPTURE_BANK_INFO` holds `(void *)HW_LCDC_VRAM_A..D`. It is
  the only static initializer built from a VRAM constant besides `gx_load3d.c`'s tables.

## Method

The library has three layers:

- **Layout** (`vram_layout.c`). This is plain C: from the nine VRAMCNT bytes it computes which bank page sits in each
  16 KiB slot of the 11 views. The views are LCDC, BG-A, OBJ-A, BG-B, OBJ-B, texture, texture palette, and the four
  extended palettes. Each slot also records whether it is a mirror, and the layout counts overlaps. All banks together
  are 41 pages (656 KiB), and LCDC has one slot per page.
- **Vram** (`vram.c`). It keeps the views in one region (2.52 MiB, with a 16 KiB guard slot around each view), either
  reserved by the Vram or passed in by the caller, such as a static array. The region must be aligned to the host page
  for aliasing and to 8 bytes for copying. Each VRAMCNT write recomputes the layout, and only the slots that changed
  are applied:
  - The **aliasing backends** map each run of changed slots with one call. A slot with no bank gets fresh private zero
    pages. It reads 0 and keeps a stray write only until its mapping changes again. `VRAM_FLAG_TRAP_UNMAPPED` makes
    such slots `PROT_NONE` instead, for debugging.
  - **Guard slots** have no access in a reserved region. In a caller's region they are accessible zero pages unless
    trap mode is on, because LeakSanitizer reads every word of `.bss` at exit (below).
  - The **copy backend** keeps each bank page in one home: its first slot in a CPU view, or its parking page when it is
    only in GPU-only views or nowhere. When the home changes, the page is copied straight to its new home once no
    other page is still there. Only pages that trade slots go through a parking page. Vacated CPU slots are then
    zeroed. It makes no OS calls. Mirrors are visible only through `vram_slot_ptr`, and a store through a mirror
    stays in that slot's own bytes.
  - `vram_slot_ptr` (the renderer's view, which resolves mirrors), `vram_bank_page` (save states) and the per-page
    generation counters work the same in every backend.
- **OS calls** (`vram_os.c`). They cover the three macOS ways, and `memfd_create` on Linux (not built here).

Tests (`test_vram.c`, registered with CTest):

- **Layout**: `sdk_settings.h` lists every case of every `GX_SetBankFor*` switch in `gx_vramcnt.c` (85) and
  `GX_SetBankForLCDC` of each bank. Each row has the VRAMCNT bytes `gx_vramcnt.c` writes, taken by a script, and the
  expected place of every bank page, mirrors included, written out by hand from GBATEK's table and melonDS. Nothing in
  the table comes from `vram_layout.c`. The register values the SDK never writes are covered only by invariants over
  all 256 values of all 9 registers (inside the view, one bank, mirror flags consistent, no overlap with itself), which
  can't catch a wrong place. The first version claimed GBATEK coverage it didn't have.
- **Each backend**, with a reserved region and with a static region (16 KiB-aligned for aliasing; for copying, an
  array only 8-byte aligned, as on Windows):
  - a write through BG-A that reads back through LCDC, and the other way round, and two banks trading places;
  - the BG-A clear of `codein.c` with 128 KiB mapped; a clear of all of LCDC with banks elsewhere, as a robustness
    check rather than a BW2 case;
  - contents kept while a bank is disabled, given to the ARM7 or in texture or extended palette slots;
  - mirrors, and `codein.c`'s stores through them, with the difference between the backends asserted;
  - the SDK's overlapping BG switch;
  - a `GX_BeginLoadTex`/`GX_EndLoadTex` upload, with its generation bumps;
  - guard pages and trap mode, caught as faults where they should fault and not where they shouldn't;
  - an unaligned caller region, refused by the aliasing backends;
  - a differential test: 3,000 random VRAMCNT writes and stores through the CPU views, mirrors included and a quarter
    of them zeros, with the backend compared after every remap to a reference model of every view, slot and bank
    byte. The model applies each backend's own rule for a store through a mirror.
- **Exit with the views live** (`exit-live`): a static region left mapped at exit, for LeakSanitizer to scan on Linux.
- **The static-region design** (`region_check.c`): its static pointer table, the `u32` truncation, and
  `gx_load3d.c`'s `u16` table, which must fail to compile.
- **Windows compile check**: `vram.c`, `vram_layout.c`, `vram_os.c` and `region_check.c` compiled with `-c` for
  `x86_64-pc-windows-msvc`, `aarch64-pc-windows-msvc` and `x86_64-w64-windows-gnu`. This Mac has no Windows SDK or
  MinGW sysroot, so `windows-stubs/` stands in for the four C library headers the library includes, declaring only
  what the UCRT declares (no `aligned_alloc`). It fails on the first version's `aligned_alloc` call and on a 16 KiB
  `_Alignas` static array (both tried).
- **Injected bugs** each fail the suite: an F/G offset step of 0x20000, a missing H mirror, texture slots from OFS.0
  only, the copy backend not clearing a mirror slot, and the copy backend moving a page before the slot is free.

`bench_vram.c` times the remaps the game makes. Its scenarios are defined at the top of the file. Scene changes and
frames go through a model of the SDK's bookkeeping (`gGXState`), which writes VRAMCNT as `gx_vramcnt.c` does and hands
the registers to the Vram after each register write, after each SDK call, or once per batch.

## Results on this Mac

Apple M2, macOS 26.5, 16 KiB pages. `ctest` passes 19/19 in four builds: Apple clang RelWithDebInfo, Apple clang Debug
with `-fsanitize=address,undefined`, and the same two with Homebrew LLVM 23. Mapping over a static `__bss` array works
under ASan with all three macOS ways. ASan doesn't track `mmap`ed memory, and our `SIGSEGV`/`SIGBUS` handler (for the
trap tests) takes precedence over ASan's. LeakSanitizer isn't supported on macOS arm64
(`detect_leaks is not supported on this platform`), so the `exit-live` tests prove nothing here.

`bench_vram 5000`, RelWithDebInfo, median (p90) in µs. Other builds ran on this machine at the same time (load average
about 30), which inflates the p90s and the maxima (up to 20 ms, in every backend, copy included). A second run agreed
within 15% on most medians, but single rows (`tex1`, `clear`, `bank128` for some backends) moved by up to 1.7× either
way: read the numbers as magnitudes.

| Scenario | shm | mach-entry | mach-remap | copy |
| --- | --- | --- | --- | --- |
| one 128 KiB bank, LCDC ↔ BG-A | 2.04 (3.58) | 2.29 (3.88) | 4.17 (4.29) | 9.83 (23.6) |
| same, then a first store in each of its 8 pages | 7.29 (10.0) | 7.54 (8.29) | 7.88 (12.5) | 9.79 (24.6) |
| one 16 KiB bank (F) with its mirror | 2.79 (5.04) | 2.96 (3.08) | 3.25 (5.21) | 1.17 (2.00) |
| texture upload: TEX_0123_ABCD out and back around a 128 KiB copy, then one read per page | 45.7 (74.3) | 48.9 (78.6) | 48.8 (65.3) | 63.6 (71.8) |
| 512 KiB clear of BG-A, 256 KiB mapped (no remap) | 8.21 (13.7) | 8.46 (13.7) | 13.5 (13.8) | 7.88 (13.6) |

A scene change, `GFL_BGSysSetVRAMBanks` between two settings that differ in every bank but H, per change:

| Applied | shm | mach-entry | mach-remap | copy | Mapping calls |
| --- | --- | --- | --- | --- | --- |
| after every register write | 56.3 (96.5) | 59.1 (105) | 61.7 (110) | 147 (178) | 45.5 |
| after every SDK call (20) | 47.2 (78.4) | 50.4 (87.5) | 53.4 (94.7) | 144 (178) | 38.5 |
| once, at the end of `GFL_BGSysSetVRAMBanks` | 23.0 (37.1) | 24.0 (41.6) | 25.1 (38.3) | 32.4 (42.3) | 16.5 |
| after every SDK call, then first touches of every mapped slot | 83.9 (130) | 87.0 (140) | 88.2 (149) | 141 (177) | |

Per frame, from battle's setting (`btlv_core.c`: texture 01_AB, texture palette 01_FG), applied after every SDK call
(after every register write in brackets):

| Frame | shm | mach-entry | mach-remap | copy | Mapping calls |
| --- | --- | --- | --- | --- | --- |
| 4 palette tasks of 32 B (a model fade) | 29.1 [37.8] | 32.6 [40.2] | 34.5 [41.9] | 9.79 [13.8] | 24 |
| 1 texture task of 8 KiB | 7.96 [9.50] | 8.67 [10.4] | 9.12 [11.0] | 33.4 [32.8] | 6 |
| 4 texture tasks of 8 KiB and 4 palette tasks | 63.3 [79.1] | 68.0 [83.4] | 71.9 [87.1] | 160 [180] | 48 |
| the dual 3D C/D swap | 4.25 [4.75] | 4.67 [5.08] | 4.92 [5.08] | 21.3 [28.2] | 4 |

Notes on the tables:

- The review's measurement of the proposed hook is confirmed: applying after every SDK call takes 38 or 39 mapping
  calls per scene change (38.5 on average), not the 33 the first version quoted. The 33 came from two whole-register
  applies, which no hook does. Applying once per `GFL_BGSysSetVRAMBanks` takes 16.5.
- The copy backend's cost follows the size of the banks moved, not of the upload: each task moves every bank of its
  use into LCDC and back. Copying now moves a page straight to its new home, and keeps pages of the GPU-only views in
  their parking pages; between the first version's run and this one, the texture upload row went from 114 µs to
  64 µs and the 128 KiB bank from 22 µs to 10 µs.
- Applying per SDK call costs the copy backend most at scene changes (144 µs against 32 µs batched), because every
  bank goes to LCDC and then to its new place.
- Palette tasks are cheaper copied than aliased: F and G are one page each, and aliasing pays three mapping calls per
  remap.
- Access through aliased views is as fast as plain memory, within the noise (the clear row).

## Linux and Windows

**Linux** has every piece the shm backend uses:

- `memfd_create` (Linux 3.17, glibc 2.27) gives a file descriptor that "behaves like a regular file", so it can be
  sized and memory-mapped ([memfd_create(2)][memfd]).
- `mmap` with `MAP_FIXED` replaces the pages there ("the overlapped part of the existing mapping(s) will be
  discarded"), and its offset must be a multiple of the page size ([mmap(2)][mmap]).
- Only the object's creation differs from the macOS shm path tested here.
- LeakSanitizer is on by default in ASan builds on Linux ([AddressSanitizerLeakSanitizer][lsan-wiki]). At exit it scans
  every writable `PT_LOAD` segment (`.data` and `.bss`) word by word with plain loads, without checking access
  (compiler-rt [`lsan_common_linux.cpp`][lsan-linux] `ProcessGlobalRegionsCallback`, [`lsan_common.cpp`][lsan-common]
  `ScanGlobalRange` and `DirectMemoryAccessor::LoadPtr`; `use_globals` defaults to true, [`lsan_flags.inc`][lsan-flags]).
  A `PROT_NONE` guard or trap slot in a static array would crash it. A caller's region therefore keeps its guards
  accessible unless trap mode is on, and `exit-live` exits with the views mapped so a Linux CI run checks this.

arm64 Linux kernels come with 4, 16 or 64 KiB pages ([kernel arm64 memory layout][arm64mem]). RHEL ships a
`kernel-64k` beside its default 4 KiB kernel ([RHEL 9][rhel64k]). With 64 KiB pages, `vram_os_backend_available`
returns false, and the copy backend runs. This spike has not been run on Linux: no Linux machine or running Docker
daemon was available.

**Windows** (10, version 1803 or later) can alias at page granularity, but only in memory reserved at run time:

- Placeholders are created by `VirtualAlloc2(MEM_RESERVE | MEM_RESERVE_PLACEHOLDER)` with `PAGE_NOACCESS`, and split
  or merged with `VirtualFree(MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)` and `MEM_COALESCE_PLACEHOLDERS`
  ([VirtualAlloc2][va2], [VirtualFree][vf]). Those are the only ways the documentation gives to make one. That the
  executable's own `.bss` can't become placeholders is an inference from this, not tested. (The first version cited
  "Only data/pf-backed section views are supported (no images ...)" for it; that sentence is about which section may
  be mapped into a placeholder, not where the placeholder is.)
- `MapViewOfFile3(MEM_REPLACE_PLACEHOLDER)` maps a slot of a pagefile-backed section (`CreateFileMapping` with
  `INVALID_HANDLE_VALUE`). "The 64k alignment requirements on Offset and BaseAddress do not apply when this flag is
  specified"; the offset must then only be page-aligned ([MapViewOfFile3][mvof3]).
- `UnmapViewOfFile2(MEM_PRESERVE_PLACEHOLDER)` turns a slot back into a placeholder ([UnmapViewOfFile2][uvof2]).
- A replacement must match the placeholder exactly, so the simplest design is one placeholder per slot, with 2 calls
  per changed slot.
- The `VirtualFree` page shows a split only at 64 KiB. [DuckStation's `memmap.cpp`][duckstation] splits and maps at the
  host page size on Windows, and uses `mach_make_memory_entry_64` + `mach_vm_map(VM_FLAGS_OVERWRITE)` on macOS. That
  code was read for the API sequence, not copied.
- For a slot with no bank, `VirtualAlloc2(MEM_REPLACE_PLACEHOLDER)` gives private zero pages, but its page requires a
  `BaseAddress` that is "a multiple of the system allocation granularity" (64 KiB) and states no exemption for
  placeholders, unlike `MapViewOfFile3`'s. If that holds, unmapped slots need a view of a second pagefile section
  (one 16 KiB page per slot, zeroed when the slot's mapping changes).
- A static array can't serve either way: clang rejects `_Alignas(16384)` for COFF targets ("requested alignment must be
  8192 bytes or smaller", reproduced with Homebrew clang 23 for `x86_64-pc-windows-msvc` and `x86_64-w64-windows-gnu`),
  though 8 KiB would cover Windows' 4 KiB pages; and it isn't placeholder memory. `VRAM_STATIC_REGION_ALIGN` is 8 on
  Windows, and the copy backend accepts such a region.
- The UCRT has no `aligned_alloc` ("The Universal CRT doesn't implement C11 aligned_alloc, but does provide
  _aligned_malloc and _aligned_free", [Microsoft C/C++ language conformance][msvc-conf]). The copy backend now uses
  `calloc`.

Nothing here has been compiled against the real Windows headers, linked or run on Windows. The placeholder backend is
designed from the documentation and not written; only the copy backend and the static-region design compile for
Windows, against `windows-stubs/`.

## Recommended design for the real HAL

1. **Views at a run-time address on every OS.** Under `PLATFORM_PC`, define `HW_BG_VRAM` and the other view constants as
   `((uptr)gHalVram + VIEW_OFFSET_BG_A)`, with `gHalVram` set by the HAL before any game code runs, in a region the
   Vram reserves. `(void *)HW_BG_VRAM` and the SDK's `G2_Get*Ptr` functions compile unchanged.
   - These changes are needed under any host-address scheme, static array included, each a `PLATFORM_PC` change with
     a `game-changes.md` row and the MWCC proof: `gx_load3d.c`'s two `u16` tables hold LCDC offsets instead, with
     `HW_LCDC_VRAM` added where they're read; its four `u32` statics and `gx_load2d.c`'s two become `uptr`; the game's
     `(u32)` casts of VRAM pointers become `uptr` or `u8 *` arithmetic; the three raw DS addresses become
     `HW_LCDC_VRAM_A` and `HW_LCDC_VRAM_D`. Building the game with clang's pointer-to-int warnings and
     `-Wshorten-64-to-32` as errors lists every site.
   - A run-time base adds one change, `screentex.c`'s static table (store bank offsets, or fill it at init).
   - In return every OS can alias, so mirrors behave the same everywhere. Windows gets its views from placeholders.
     Guard and trap pages never sit in `.bss`, so LeakSanitizer is no concern. No static alignment limit applies.
   - The views can't sit at the DS addresses instead, which would have kept the integer code unchanged: macOS arm64
     keeps the low 4 GiB as `__PAGEZERO`. Tested here: `mmap(MAP_FIXED)` at 0x06000000 fails, and a binary linked with
     `-pagezero_size 0x4000` is killed at launch.
   - The alternative, a static array on macOS and Linux with copying on Windows, saves the `screentex.c` change but
     gives Windows different mirror behavior. The spike supports both (`vram_create`'s region argument).
2. **Aliasing on macOS, Linux and Windows; copying as the fallback.**
   - The shm path (`memfd_create` on Linux, `shm_open` with the name unlinked at once on macOS) is one code path for
     both. `mach-entry` is a drop-in on macOS if `shm_open`'s global namespace ever matters, for example under the App
     Sandbox, which the port doesn't use.
   - Placeholders on Windows, to be written and run on Windows CI first.
   - Copying where aliasing can't run: 64 KiB-page Linux, and Windows until its backend has passed the conformance
     test.
   - CI runs the same conformance test, with its reference model, against every backend.
   - Before copying runs anywhere by default, audit for stores through mirrors other than uniform fills: the 56
     `BGSysVRAMConfig` settings with `GX_VRAM_SUB_OBJ_16_I` and a sub OBJ mapping of 32 KiB or more, the 6 with sub BG
     in H alone (`codein.c`'s among them), and `geonet.c`'s own setting, which uses both.
3. **Apply after each SDK call, and once per `GFL_BGSysSetVRAMBanks`.** Under `PLATFORM_PC` the `reg_GX_VRAMCNT_*`
   macros point into the shadow register file. A hook at the end of `GX_VRAMCNT_SetLCDC_` and `disableBankForX_` (in
   `gx_vramcnt.c`) and of `GX_InitGXState` (in `gx_state.c`) calls `vram_set_all_vramcnt` with the shadow. Nothing
   touches VRAM inside those functions, so this is exact and removes the SDK's transient overlaps. A defer bracket
   around `GFL_BGSysSetVRAMBanks`' 20 calls in `gx_layers.c` applies them once: 16.5 mapping calls instead of 38.5,
   and 32 µs instead of 144 µs when copying. Texture and palette tasks gain nothing from batching, since the CPU copies
   between their Begin and End. These hooks are changes outside `port/` and need `game-changes.md` rows.
4. **The renderer reads through a 16 KiB slot table**, as melonDS does. Build a per-view array of `vram_slot_ptr`
   results each time the layout is applied. This is right for both backends, mirrors included. Texture and palette
   caches key on the per-page generations. Those slots can only change while their bank is in LCDC, so every texture
   upload bumps them.
5. **Unmapped slots read 0 and absorb writes**, and are reset whenever their mapping changes. Guard pages without
   access belong only in a reserved region. Trap mode is a debugging option only, and with a static region it needs the
   Vram destroyed before exit (or `ASAN_OPTIONS=detect_leaks=0`) under Linux ASan. A debug build can log an overlap that
   survives an SDK call, which should never happen.

## Open risks

- **Not run on Linux or Windows.**
  - The Linux path differs from the tested one only in `memfd_create`. It, ASan with `MAP_FIXED` over a global, and
    LeakSanitizer over the `exit-live` tests need a Linux CI run.
  - The Windows placeholder backend isn't written. Splits at 16 KiB are documented only indirectly, and
    `VirtualAlloc2`'s page states no exemption from the 64 KiB granularity for replacing a placeholder. The Windows
    compile check uses stand-in headers, not the UCRT's or MinGW's, and doesn't link.
- **Mirror stores under copying.** Copying matches the DS for uniform fills through mirrors only. The audit above
  decides whether any BW2 code does more.
- **The integer-address changes** in `gx_load3d.c`, `gx_load2d.c` and the game sites must each be proven not to change
  the MWCC output; replacing `(u32)vram + n` with pointer arithmetic is the likeliest to differ.
- **Stray writes to unmapped slots stay readable** until the slot's mapping changes; the DS reads 0. Both backends do
  the same, so replays don't depend on the backend.
- **8-bit VRAM writes** are dropped by the DS and stored by the host. A game bug that relies on this would differ from
  the reference; the T4 oracle comparison would show it.
- **Overlapping banks** (lower bank wins, where the DS ORs reads) can't be aliased. Only the SDK's transient overlaps are
  known, and per-call applies remove them.
- **A renderer on another thread** would need remaps synchronized with scan-out, in either backend. The PLAN's
  single-thread design avoids this.
- **Overlay 169 runs from bank H on the DS.** On the host it is linked natively, so nothing should copy it into bank H,
  and bank H's bytes don't matter to it.

## Files

- `vram.h`: the API, the views and the layout types.
- `vram_layout.c`: VRAMCNT to slots.
- `vram.c`: the Vram object, the aliasing and copy apply steps.
- `vram_os.h`, `vram_os.c`: the mapping calls for macOS and Linux.
- `sdk_settings.h`: every SDK setting with its VRAMCNT bytes and GBATEK's places, for the layout test.
- `test_vram.c`: the tests.
- `bench_vram.c`: the timings, with the model of the SDK's bookkeeping.
- `region_check.c`: the static-region design, run on the host and compiled for Windows.
- `windows-stubs/errno.h`, `stdio.h`, `stdlib.h`, `string.h`: stand-ins for the UCRT's four C library headers that the
  library includes, declaring only what the spike uses, for the Windows compile check (this Mac has no Windows SDK or
  MinGW sysroot).
- `CMakeLists.txt`: the tests, the timings and the region check as ctest tests, and the Windows compile check against
  `windows-stubs/`.

To build and test:

```
cmake -S port/spikes/vram-alias -B build/port-spikes/vram-alias && cmake --build build/port-spikes/vram-alias
ctest --test-dir build/port-spikes/vram-alias --output-on-failure
cmake -S port/spikes/vram-alias -B build/port-spikes/vram-alias-asan -DVRAM_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
build/port-spikes/vram-alias/bench_vram 5000
```

[gbatek]: https://problemkaputt.de/gbatek-ds-memory-control-vram.htm
[melonds-cpp]: https://github.com/melonDS-emu/melonDS/blob/master/src/GPU.cpp
[melonds-h]: https://github.com/melonDS-emu/melonDS/blob/master/src/GPU.h
[memfd]: https://man7.org/linux/man-pages/man2/memfd_create.2.html
[mmap]: https://man7.org/linux/man-pages/man2/mmap.2.html
[arm64mem]: https://www.kernel.org/doc/html/v6.9/arch/arm64/memory.html
[rhel64k]: https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/9/html/performing_an_advanced_rhel_9_installation/installing-rhel-on-arm-with-kernel-64k_installing-rhel-as-an-experienced-user
[lsan-wiki]: https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer
[lsan-linux]: https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/lsan/lsan_common_linux.cpp
[lsan-common]: https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/lsan/lsan_common.cpp
[lsan-flags]: https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/lsan/lsan_flags.inc
[va2]: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2
[vf]: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
[mvof3]: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile3
[uvof2]: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile2
[msvc-conf]: https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance
[duckstation]: https://github.com/stenzek/duckstation/blob/master/src/common/memmap.cpp
