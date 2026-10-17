# pokebw2 C as 64-bit host code: compile probe and ABI/portability hazards

Date: 2026-10-16. Tree: worktree `desktop-subagent-inspection-3190ba` at `0c2bbee9d` (branch
`cjcampos/pokemon-bw2-port-research-975492`), Black 2 (`-DBLACK2`). Compiler: Homebrew clang 23.1.3.
Everything was measured, not estimated, unless marked "heuristic". The repo was only read. Every output, and the
scripts that rerun it (`run_all.sh` writes `metrics.txt`), are in:
`/private/tmp/claude-501/-Users-cjcampos-dev-decomp-pokebw2--claude-worktrees-desktop-subagent-inspection-3190ba/61d16309-e931-4ac8-a3f7-4b6a6b21ba14/scratchpad/research/work/compile-probe/`

## 0. Headline

* **The game's C is close to host-ready.** On a 32-bit target, clang accepts all 1,170 `src/` files. On arm64 macOS
  it accepts all 1,170 once one macro changes (`OVERLAY_ID`). Every failure that remains is in `lib/`: 43 files with
  MWCC `asm` and 6 with small dialect issues.
* **The 64-bit work in game code is bounded.** Clang reports 1,164 unique pointer/int truncation sites in game code
  (`src/` plus `include/`). 700 of them are the overlay-ID macro. That leaves **464 sites in 154 of the 1,170 game
  files**. Of those, about 310 move a pointer through a `u32` parameter, field or return value. About 41 are
  intentional `u64`→`u32` narrowing that also fires at 32-bit. For comparison, `lib/` has 678 sites in 117 files.
* **Save data is portable with two typedef changes.** Every on-disk save block is pointer-free. Checked against the
  ROM, they break on 64-bit hosts for only two reasons:
  * 8-byte alignment of `s64`/`u64`. MWCC aligns them to 4, so 8 blocks break, and they break on wasm32 too.
  * `unsigned long` fields. These break 2 blocks: `RTCDate` inside `AdventureSave`, and `RivalDataSave`.
* **i386 SysV is an exact layout proxy for MWCC.** It matches all 61 struct sizes that could be checked against the
  ROM's own `return sizeof(T)` constants, and all 64 `// size:` comments. AAPCS (clang's ARM default) gets 8 of the
  61 wrong.
* **wasm32/i386 compile exactly what ARM compiles** (1,520 of 1,569 files) and produce zero pointer/int warnings. With
  an `aligned(4)` typedef for the 64-bit scalars, only 1 of 4,447 records differs from MWCC. That record is an MSL
  union.

## 1. Base types, dialect and MWCC flags

### Types (`include/types.h`, `include/stddef.h`, `include/stdarg.h`, SDK headers)

| Type | Definition | LP64 (macOS/Linux) | LLP64 (Windows) | Hazard |
|---|---|---|---|---|
| `u32`/`s32` | `unsigned int` / `signed int` | 32 | 32 | none. They are **not** `long` |
| `u64`/`s64` | `unsigned/signed long long` | 64, align 8 | 64, align 8 | MWCC aligns them to **4** inside structs. Changes 8 save blocks |
| `f64` | `double` | align 8 | align 8 | same as s64 |
| `BOOL` | `int` | 32 | 32 | none |
| `fx32`/`fx16` | `s32`/`s16` (nitro/fx.h) | ok | ok | none (integer fixed-point, 1,666 game sites) |
| `size_t` | **`unsigned long`** (include/stddef.h) | 64, coincidentally right | **32, wrong** | x86_64-msvc: clang predeclares `size_t`, so the typedef is an error in 992 files |
| `wchar_t` | `unsigned short` | host `L""` is 32-bit | 16-bit, ok | 2 `L""` sites in lib (mb_wm.c, printf.c) need `-fshort-wchar` or a rewrite |
| `va_list` | **`char *`, stack-walking macros** | broken | broken | variadics are in registers or 8-byte slots on every 64-bit ABI. 14 files, 98 uses, including 2 game files (ov167 btl_server_cmd.c, btl_string.c) |
| `RTCDate` fields | `unsigned long` (nitro/rtc.h) | 64 | 32 | embedded in the `AdventureSave` save block |
| `RivalDataSave.metTrainerCount` | `unsigned long` | 64 | 32 | save block |
| `NULL` | `((void *)0)` | ok | ok | |
| `offsetof` | `((size_t)&((T *)0)->m)` | works in clang | | prefer `__builtin_offsetof` |
| MSL `HUGE_VAL`/`INFINITY` | `*(double *)__double_huge` over `unsigned long[]` | **reads 0.0** | ok | MSL only (6 files). A host build would use host libm anyway |

The game code is careful about this. `unsigned long` appears only as about 22 local or parameter types, chosen
because MWCC indexes with it differently (for example `btl_monset.c`, `wifi_p2pmatch.c`). On LP64 these widen to 64
bits, which is harmless for counters.

### MWCC-specific syntax

Counts are from `dialect_scan.txt`, over src/, lib/ and include/.

* **`asm`**: 44 files and about 250 sites, **all in lib/**: nitro 29, dsprot 11, twl 2, msl 1 (fp_fastI.c), dwc 1.
  There is none in `src/`, and code-standards rule 7 forbids it there. One inline `asm { clz }` sits in a public
  header, `nitro/math.h` (`MATH_CountLeadingZeros`), behind `#ifdef __thumb`. MWCC defines `__thumb` and clang does
  not, so that one header is why **1,068 of 1,569 files fail** with the stock `compile_flags.txt` (armv5te, no
  `__thumb`). Defining `__thumb` (Thumb is what the game is built as) brings the count to 1,520/1,569.
* **`#pragma`**: 74 sites in 35 files. They are `thumb on/off/reset` (64), `define_section ctor` (5) and
  `explicit_zero_data` (4). Clang ignores them (`-Wno-unknown-pragmas`). 3 of these files are in src/
  (mystery_album.c and 2 in src/system).
* **`__declspec(ctor)`**: 5 sites, all in dsprot. **`__memcpy`** (the MWCC intrinsic): 3 lib files (dpw, poke_net).
* **`__attribute__`**: only `ATTRIBUTE_ALIGN` (20 sites in 12 files), in GCC syntax. Clang accepts it. MSVC `cl` would
  not.
* **C99 and GNU extensions**: no statement expressions, `typeof`, case ranges, binary literals or `?:`.
  `-pedantic` reports:
  * 61 C11 anonymous struct/union uses (192 anonymous struct/union definitions in 92 files)
  * 7 zero-length arrays
  * 36 GNU arithmetic on function pointers (all dsprot)
  * 34 void* ↔ function-pointer conversions
* **No `_Static_assert` anywhere.** Struct sizes are documented only as 64 `// size: 0x..` comments.
* **Bitfields**: 1,394 members in 163 files, 157 of them in `include/save`. Clang's layout on every host target,
  MSVC's included, matches MWCC. `MedalBox` is 1,174 bytes in the ROM (`MedalBox_GetBlockSize` returns `0x496`),
  which matches i386, arm64, x86_64 and AAPCS but not APCS-GNU. x86_64-windows-msvc and windows-gnu give identical
  layouts for all 4,447 records, because the tree never mixes bitfield base types inside a run. There are 9 plain-`int`
  bitfields, whose signedness is implementation-defined.
* **Encoding**: 547 files contain non-ASCII bytes, all in comments (UTF-8, fed to MWCC through sjiswrap). There are
  **0 non-ASCII string literals**, so there is no Shift-JIS hazard on the host.

### MWCC flags (configure.py CC_FLAGS, lib/*/library.toml) and what they mean for a host

| MWCC flag | Host equivalent | Note |
|---|---|---|
| `-char signed` (game and 12 libs). MSL alone is `-char unsigned` | `-fsigned-char` | Apple arm64, x86 and Windows arm64 are signed by default. **Linux aarch64 is unsigned**, so the flag is needed there. Game code uses explicit s8/u8 (470 `s8` sites) and has only 65 plain-`char` declarations |
| `-enum int` | clang default (never `-fshort-enums`) | |
| `-fp soft` | `-ffp-contract=off`, no `-ffast-math` | Soft-float is IEEE. On arm64, contraction to FMA would change float results (266 float/double sites in 54 game files). Keep MSL's fdlibm for bit-exact sin/cos/sqrt |
| `-lang=c99 -gccext,on` | `-std=gnu99` (or c11) | |
| `-requireprotos` | `-Werror=implicit-function-declaration` | Explains why src has 0 implicit declarations |
| `-inline on,noauto`, `-ipa file`, `-O4,p`, `-thumb`, `-proc arm946e` | none | codegen only |

## 2. Baseline compile (32-bit ARM, as the repo's clangd config)

`probe.py` runs `clang -fsyntax-only` per file and records every diagnostic.

| Run | Pass/total | src/ | lib/ |
|---|---|---|---|
| `compile_flags.txt` as is (armv5te-none-eabi, no `__thumb`) | **501/1,569** | 243/1,170 | 258/399 |
| + `-D__thumb` (thumbv5te) | **1,520/1,569** | **1,170/1,170** | 350/399 |

* By completeness: complete files 1,366/1,400 pass, incomplete files 154/169 pass.
* Unique error sites with `-D__thumb`: 574, **all in lib/**: nitro 242, msl 138, dsprot 128, dwc 34, dpw 17, twl 10,
  poke_net 3, dwcutil 2.
* The 49 failing files:
  * 43 are `asm`: 42 have their own asm, and 1 (dsprot) gets it through `dsprot_internal.h`.
  * 6 are other issues:
    * 3 call `__memcpy` (dpw ×2, poke_net).
    * 1 assigns `L"multiboot"` to a `u16[]` (nitro mb_wm.c).
    * 1 has a wide literal type mismatch (msl printf.c).
    * 1 has `static` after a non-static declaration (dwcutil dwc_aoss_ap.c).

  All of these are clang-vs-MWCC dialect differences, not 64-bit issues.
* Default warnings on 32-bit:
  * 139 `-Wswitch` (127 in game code)
  * 50 `-Wpointer-sign`
  * 6 `-Wreturn-type`. The 5 in game code are functions with a path that falls off the end: itemmenu_disp.c:1347,
    btlv_effvm.c:3894, btlv_mcss.c:2211, worldtrade_input.c:563, mictest.c:750.

## 3. Host compile (64-bit)

The flags are the 32-bit ones plus `-Wpointer-to-int-cast -Wint-to-pointer-cast -Wshorten-64-to-32 -Wint-conversion
-Wincompatible-pointer-types -Wimplicit-function-declaration -Wimplicit-int -Wsizeof-pointer-div -Wcast-align
-Wvoid-pointer-to-int-cast -Wint-to-void-pointer-cast -Wincompatible-library-redeclaration -Wformat
-Wshift-count-overflow ...` (`scripts/host_warnings.txt`), in hosted mode.

### Pass counts

| Target | Pass/total | What fails beyond the 49 lib files |
|---|---|---|
| i386-linux, wasm32 | 1,520 | nothing |
| arm64-apple-macos14 | 1,504 | 7 game files: `OVERLAY_ID` in static initializers (`(u32)` of a linker symbol is not a constant on 64-bit). 8 game files: hosted `void main` (system/main.h). Plus dsprot/dwc initializers |
| x86_64-linux-gnu | 1,504 | same |
| x86_64-windows-gnu | 1,506 | same, minus the `__declspec` errors |
| x86_64-windows-msvc | **532** | `typedef unsigned long size_t` conflicts with clang's predeclared 64-bit `size_t` (992 files) |
| arm64 + shim (`OVERLAY_ID(n)` → `n`, `-fshort-wchar -fdeclspec`, `#define __memcpy __builtin_memcpy`) | **1,524** | **all 1,170 src pass.** The rest is 43 asm files plus `dwc_ac_soc.c` (`(u32)` of a pointer in an initializer) and `dwc_aoss_ap.c` |

Codegen agrees with the syntax runs. Emitting LLVM IR for arm64 gives 1,512/1,569. The only game failures are the 7
`OVERLAY_ID` initializer files.

### Unique warning locations, game (src+include) / lib

| Flag | i386 | arm64 macOS | x86_64 Win (gnu) | arm64 + OVERLAY_ID shim |
|---|---|---|---|---|
| `-Wpointer-to-int-cast` | 0 / 0 | 865 / 184 | 901 / 215 | **165** / 184 |
| `-Wvoid-pointer-to-int-cast` | 0 / 0 | 46 / 117 | 46 / 118 | 46 / 117 |
| `-Wint-to-pointer-cast` | 0 / 0 | 73 / 144 | 77 / 156 | 73 / 144 |
| `-Wint-to-void-pointer-cast` | 0 / 0 | 65 / 132 | 65 / 132 | 65 / 132 |
| `-Wshorten-64-to-32` | 41 / 19 | 115 / 77 | 47 / 56 | 115 / 77 |
| `-Wcast-align` | 256 / 346 | 282 / 523 | 282 / 514 | 282 / 523 |
| `-Wincompatible-library-redeclaration` | 0 | 1 / 17 | 15 / 17 | |

* `-Wint-conversion`, `-Wincompatible-pointer-types`, `-Wimplicit-function-declaration` and `-Wimplicit-int` have
  **0 sites in game code**. Every pointer/int crossing is an explicit cast, so all of them are greppable.
* Windows (LLP64) gives fewer `shorten` sites because `long` stays 32-bit, but the same pointer sites.

### What the game-code sites are

`classify64.py` sorts the unique sites by their source line. The figures are for arm64 before the shim (1,164 sites,
294 files):

| Category | Sites | Files | Example |
|---|---|---|---|
| Overlay IDs (`OVERLAY_ID(n)` / `OVERLAY_*` macros: `(u32)` of a linker symbol) | **700** | 178 | `QueueGameProc(mgr, OVERLAY_ID(171), ...)` |
| Pointer passed or compared as an integer (u32 arguments, `(u32)end - (u32)heap`) | 111 | 40 | `FieldTaskManager_AddTask(tm, task, (u32)accelTask)`, `MCSS_SetAnimationEndCallback(mcss, (u32)wk, ...)`, `UnionComm_SetSelf(unisys, 3, (u32)entry)` |
| Integer used as a pointer (u32 fields/params holding addresses) | 102 | 42 | `(AppTaskMenuWin *)event->param`, `(UnionBeaconEntry *)value`, `(StrBuf *)setup.items[i]`, `(const char *)setup->arcId` |
| Pointer through a Get/SetParam-style `u32` API | 51 + 19 | 25 + 9 | `PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)name)`, `poke_tool.c: (const StrBuf *)value`, `return (u32)&person->data[5].message` (resonance_resort_data.c) |
| ptrdiff / size_t / long / u64 narrowing | 115 | 54 | `GFL_HeapResize(data, (u8 *)tex + ofs - (u8 *)data)`. RTCDate `unsigned long` fields. About 41 are u64→u32 that also fire at 32-bit |
| Pointer stored in a u32/int variable | 28 | 18 | |
| HW register / fixed address | 25 | 10 | `*(vu16 *)0x050005e2 = 0` (cg_help.c), `MIi_CpuClearFast(0, (void *)0x06800000, ...)` |
| Alignment math on a pointer | 13 | 7 | `(void **)(((u32)mem + 32) & ~31)` (gfl/net.c) |

* Top files once overlay IDs are excluded: ov168/btlv_gauge.c 44 (VRAM address math), system/resonance_resort_data.c
  19, system/poke_tool.c 17, ov257/ctvt_camera.c 11, system/pml_met_data.c 11, then ov137 resort_sys.c,
  ov165 plist_sys.c, ov194 pokemontrade_2d.c and ov268 br_record_proc.c at 10 each.
* In lib/ after the shim, there are 678 sites in 117 files. 197 are int-as-pointer, 173 pointer-as-int and 90 hardware
  registers. They concentrate in dsprot, nitro mb/mi/os, nitrowifi cps.c (117 alone), nnsys g2d loaders and dwc.
  Most of these libraries would be replaced in a port.

## 4. Struct layout hazards

### MWCC layout proxy

These checks are `check_size_comments.py` and `rom_sizeof_check.py`.

* Clang's ARM default (AAPCS) is not MWCC's layout. MWCC aligns `long long` and `double` to 4: `RecordSave` has an
  `s64` at `0xa4` and is `0xac` bytes. AAPCS aligns them to 8.
* APCS-GNU gets the alignment right but packs bitfields wrongly (`MedalBox`).
* **i386 SysV matches on every check**:
  * All **64/64** `// size:` comments match (AAPCS: 62/64).
  * **61/61** struct sizes read from the ROM match. These come from the constant that each
    `return sizeof(T);` function returns in `extract/b2_us`. Most of the 61 are save blocks from the save table's size
    functions. The others are runtime structs such as GameSystem and G3DResource.
  * AAPCS gets 53/61, wasm32 53/61 and arm64 50/61.
  * i386 is therefore the reference used below.

### How much changes (`layout_diff.py`, records from `-fdump-record-layouts-complete` over every TU)

| Target vs MWCC proxy | Records changed (of 4,447) | Size changed | include/save size changed |
|---|---|---|---|
| arm64 | 2,358 (53%) | 2,334 | 17 |
| x86_64 Windows (gnu = msvc) | 2,356 | 2,332 | 16 |
| wasm32 / thumb AAPCS | 102 | 69 | 12 |
| wasm32 + `aligned(4)` s64/u64/f64 typedefs | **1** (an MSL union of bare `unsigned long long`) | | **0** |
| arm64 + `aligned(4)` typedefs | 2,308 | | 7: AdventureSave/AdventureTime (`unsigned long` RTCDate), RivalDataSave (`unsigned long`), plus 4 runtime work structs that live in save headers (SaveControl, MysteryGiftSave, a Hall of Fame copy-out struct, a musical download work) |

* Changes per area on arm64: src/ov* 1,188/2,004, include/app 271/443, include/field 203/390, lib/nitro 109/247,
  lib/nnsys 82/206, include/battle 51/185, src/system 93/175, lib/dwc 113/165, src/gfl 61/102.
* That the work structs change is expected and harmless. Layout only matters where something assumes it, and those
  places are listed next.

### Where the layout is assumed

1. **Save data: pointer-free.** No on-disk save block holds a pointer. arm64 differs from the ROM on 11 of the 61
   checked structs:
   * 8 come from s64 alignment: PokewoodBlock, BattleRecVideo, AdventureSave, HighLinkSave, TimeSigSave,
     TrainerGameInfoSave, DayCareSave and RecordSave. AdventureSave also has `unsigned long` RTCDate fields.
   * 1 is `unsigned long`: RivalDataSave.
   * 2 are runtime structs with pointers, which are never saved: GameSystem and G3DResource.
   * The other 50 are identical, including `BoxPkm`, `PartyPkm`, `PokeParty`, `BoxTraySave`, `MedalBox` and
     `EventWork`.

   **A DS-compatible save file is achievable on every host** with the two typedef changes.
2. **Data overlaid on archive files.** `ast_file_types.py` reads clang's AST for the cast of each
   `GFL_Arc*Read*/HeapNew` result. It finds 43 distinct struct types that game code overlays on loaded data. **All
   42 that resolve keep their layout on 64-bit.** These are plain data: rail headers, place names, trade offers, map
   chunks and so on.
3. **In-place pointer relocation in NitroSystem G2D**, the real file-format hazard:
   * `NNSi_G2dUnpackOffset` (`ptr = (void *)((u32)ptr + (u32)base)`) rewrites 4-byte offset fields of the loaded file
     as pointers.
   * 30 sites in 7 loaders (lib/nnsys/src/g2d/*_load.c).
   * About 55 pointer members in `nnsys/g2d.h` file structs, such as `NNSG2dCellDataBank.pCellDataArrayHead` and
     `NNSG2dCharacterData.rawData`.
   * 97 call sites in 27 game files use `NNS_G2dGetUnpacked*`/`Unpack*` and receive these structs. On 64-bit the
     file's 4-byte slots cannot hold a pointer.
   * NitroSDK SND does the same thing: `SNDBankData.waveArcLink[]` and `SNDWaveArc.topLink` live inside SBNK/SWAR
     file data, and `SND_SetWaveDataAddress` overwrites the wave table with pointers.
   * NNS G3D uses offsets, so it is fine.
4. **Unions that mix pointers and integers**: 7 of 109 unions. 2 are in game headers (app/battle_recorder/br_net.h:32
   and :34) and 1 in game code (ov034/union_msg.c:29). The rest are in nitrowifi wcm.h, dwc sake.h, twl nwm.c and
   nnsys snd_arc_stream.c.
5. **Literal sizes** (heuristic, `alloc_sizes.py`):
   * `GFL_HeapAllocate`: 1,084 sizes use `sizeof`, **74 are bare literals** (for example `fldmmdl.c:193` 0x310),
     8 are expressions with literals and 258 are variables.
   * Clear/copy: 1,819 use `sizeof`, **100 use literals**.

   Any literal-sized block that holds a pointer-bearing struct overflows on 64-bit. There are 20 `* sizeof(u32)` /
   `* 4` array sizes, which look like real `u32` arrays on inspection. 45 sites already use `sizeof(T *)`.
6. **Hardcoded addresses** in game code:
   * Raw addresses: 0x02xxxxxx ×7 (mostly comments), 0x04xxxxxx ×3, 0x05/06xxxxxx ×2. There are no 0x027Exxxx
     uses.
   * Macro-level hardware access is wider: `REG_*` 420 sites in 147 files (blend/brightness:
     `REG_BLDCNT`/`MASTER_BRIGHT` ×403), `HW_*` 182 in 45 files (`HW_BG_PLTT`/`OBJ_PLTT` palette RAM), `reg_*` 90 in
     35 files.
   * lib+include: 732 `REG_`, 977 `reg_`, 195 raw 0x04xxxxxx.
   * Game code calls 751 distinct SDK/NNS functions or inlines (9,545 call sites). That set is the hardware
     abstraction layer (HAL) the port must provide.
7. **Cross-TU consistency** (`symbols_ir.py`, `signatures_ir.py` on LLVM IR):
   * **0 duplicate external symbols** among 18,371. Every overlay can be linked into one binary.
   * 616 tentative definitions, none duplicated, so `-fno-common` is safe.
   * Only 10 functions have signatures that differ between TUs:
     * 2 in game code: `func_ov012_0215365c` and `..._02153660` are defined with no parameters in ov012
       intrude_work.c but declared with parameters in ov198 comm_tvt_field.c.
     * 8 are DWC declaring libc functions with `u32` instead of `size_t`.

## 5. Undefined or implementation-defined behavior the DS tolerated

Counts are grep or clang-warning estimates, mostly from the `ub_arm64` run.

| Hazard | Evidence | Port action |
|---|---|---|
| Strict aliasing | Pervasive punning: 282 game `-Wcast-align` sites (u8* → u32*/struct*), file buffers cast to structs | **`-fno-strict-aliasing`** (mandatory) |
| Unaligned access | Those 282 game / 523 lib cast-align sites. ARM946E-S rotates an unaligned `LDR`, while hosts load correctly | UBSan `-fsanitize=alignment` in CI to find the real ones |
| Signed overflow | RNG and hashes are mostly u32, but this can't be proven | **`-fwrapv`** |
| Shift counts ≥ width | 531 game shifts by a variable count (heuristic). ARM register shifts give 0 at ≥32; x86 and arm64 mask the count | UBSan `-fsanitize=shift` |
| Uninitialized locals | 56 `-Wsometimes-uninitialized` + 266 `-Wconditional-uninitialized` in src. On the DS they read deterministic register/stack garbage. 11 `// BUG:` comments describe unset values (for example scrcmd_fldmmdl.c:82 "the actor returned is garbage") | `-ftrivial-auto-var-init=zero` (or pattern) for determinism, MSan on Linux, BUGFIX the documented ones |
| Out-of-bounds, use-after-free, double free | 158 `// BUG:` comments and 165 `#ifdef BUGFIX` blocks. About 15 describe OOB reads/writes (for example btl_pospoke.c:87 "writes past the entries, over posOfMon"; local_tvt_util.c:189 "table has no end"), 3 double-free/UAF, 5 leaks. On 64-bit what lies past a buffer changes, because padding and pointer sizes move | Keep the game's own GFL heap over a host arena in release (it reproduces DS tolerance). An ASan build maps `GFL_HeapAllocate` to malloc. Build the port with `BUGFIX` (or a narrower `PORT_FIX`) for memory-safety bugs |
| Missing return value | 5 game functions + 1 lib (-Wreturn-type) | fix (UB, and clang will exploit it) |
| Evaluation order | 1 site: `((u64)GFL_RandomMT() << 32) + GFL_RandomMT()` (ov055/wbt_system.c:25). No call has two side-effecting calls among its arguments | sequence it explicitly. MWCC's order is visible in the asm |
| Function-pointer casts | 7 game + 2 lib casts between incompatible signatures (different arity or type: field_player_grid.c:254, field_player_grid_event.c:98, pokemontrade_demo_*.c ×3, comm_tvt_field.c ×2). 134 game casts differ only in pointer types (`-Wcast-function-type-strict`, for example `(SaveBlockInit)MedalBox_Init`) | Fix the 9. They are UB natively and **trap under wasm call_indirect** |
| `char` signedness | Game is MWCC `-char signed`. 65 plain-char declarations in src | `-fsigned-char` (needed on Linux/Android aarch64) |
| enum size | `-enum int` = clang default | never `-fshort-enums` |
| Bitfield order and layout | identical on every clang target and MSVC (checked) | add static asserts. 9 plain-`int` bitfields have implementation-defined signedness. 10 `-Wsingle-bit-bitfield-constant-conversion` (nnsys snd) |
| `long` assumed 32-bit | RTCDate (SDK), RivalDataSave, `size_t`, about 22 game `unsigned long` locals, 65 MSL lines | typedefs (§7) |
| Varargs | the project's `stdarg.h` walks the stack | host `<stdarg.h>` under a platform macro |
| Division by zero | MWCC's `_s32_div_f` runtime returns a value. x86 raises SIGFPE; arm64 returns 0. Not countable statically | `-fsanitize=integer-divide-by-zero` in debug |
| Float determinism | -fp soft is IEEE; 266 float/double sites in 54 game files | `-ffp-contract=off`, no fast-math, SSE2 on x86 (never x87), MSL fdlibm for transcendentals |
| Threads and IRQs | game: OS_CreateThread ×2, OS_SetIrqFunction ×2, OS_DisableInterrupts ×20. lib: OS_SleepThread ×36, OS_LockMutex ×30 | HAL. The game itself barely uses them |

**Recommended host flags:** `-std=gnu99 -fno-strict-aliasing -fwrapv -fsigned-char -ffp-contract=off
-ftrivial-auto-var-init=zero -fno-common -D__thumb` (or a `PLATFORM_PC` macro that selects the C path in nitro/math.h)
`-Werror=int-conversion -Werror=incompatible-pointer-types -Werror=implicit-function-declaration -Werror=return-type`.
After cleanup, add `-Werror=pointer-to-int-cast -Werror=int-to-pointer-cast -Werror=shorten-64-to-32` for game code.

`-fms-extensions` is not needed: anonymous members are C11 and `__declspec` is dsprot-only. Use **clang on all three
OSes** (clang-cl or mingw-clang on Windows), not MSVC `cl`. The tree needs few GNU features (`ATTRIBUTE_ALIGN`, 7
zero-length arrays, the `aligned(4)` typedef trick), but one compiler means one set of layout and UB behavior to
verify. Clang's msvc and gnu Windows targets lay out all 4,447 records identically.

**Sanitizer builds are needed.**
* An ASan + UBSan configuration (alignment, shift, signed-integer-overflow, bounds, function, return), with the heap
  mapped to malloc, run through scripted playthroughs.
* An MSan build on Linux for uninitialized reads.

The static counts above (77 uninitialized, 15 documented OOB, 531 variable shifts, 282 casts that raise alignment)
are too many to prove by reading.

## 6. Alternative: a 32-bit-pointer target (wasm32 + wasm2c, or `-m32`)

### Measured

* wasm32-unknown-unknown and i386-linux compile **1,520/1,569**, the same set as ARM Thumb. The only failures are the
  43 asm files and the 6 dialect issues.
* **0 pointer↔int warnings**, against 1,164 game sites on arm64. 60 `shorten` sites remain, all u64→u32 and the same
  as on the DS.
* LLVM IR codegen for wasm32: 1,520/1,569, with 0 duplicate symbols.
* Layout:
  * wasm32 differs from MWCC in 102 records (69 sizes), all from 8-byte `s64`/`double` alignment.
  * With `aligned(4)` typedefs, **1 record** differs (in MSL).
  * i386 SysV matches MWCC exactly, and i686 Windows aligns s64 to 8 like wasm32.
* There are 9 casts between incompatible function-pointer types (7 in game code). wasm's typed `call_indirect` would
  trap on these where native code silently works. Fix them either way.

### Tradeoffs

| | Native 64-bit | wasm32 → wasm2c (the reported Gen4Recomp approach) | `-m32` native |
|---|---|---|---|
| Layout and pointer fixes | ~460 game sites + G2D/SND relocation + 2 save typedefs | ~0 for pointers. Still needs the s64 alignment typedef and the 9 function-cast fixes | ~0 (i386 SysV is an exact match) |
| Apple Silicon | yes | yes (wasm2c output is portable C) | **no**: macOS has had no 32-bit support since 10.15, and Apple Silicon has no 32-bit mode. `arm64_32` exists only on watchOS |
| Windows/Linux x86-64 | yes | yes | i386 builds work but distros are dropping multilib. Windows on ARM can't run x86-32 natively |
| Performance | best | wasm2c with guard-page bounds checks is typically ~1.1–1.5× native. Irrelevant for a 66 MHz DS game | native |
| Debugging | normal (lldb/gdb, ASan/UBSan/MSan on the game's own C) | poor: generated C, a linear-memory sandbox, sanitizers can't see the guest heap, crashes surface as traps. Usable DWARF mapping needs extra tooling | normal |
| HAL boundary | direct calls into SDL/Metal/Vulkan | every one of the 751 SDK functions the game calls, or the HAL beneath them, becomes a wasm import or is compiled inside the sandbox, so glue is needed for every graphics, sound, input and file call | direct |
| Mod-friendliness | best: mods are C against the same headers, can be hot-loaded as native libraries, and the tree reads as the decomp | mods must be built with a wasm toolchain (wasi-sdk + wasm2c) as part of the build. No native plugins inside the sandbox | ok, but a shrinking platform |
| Threads | native | none by default: NitroSDK threads need fibers or asyncify (the game barely uses them; the libs do) | native |

**Assessment.** The 32-bit route saves at most a few weeks of pointer cleanup. In exchange it gives up debugging,
sanitizers (needed, per §5) and native modding, and `-m32` can't serve Apple Silicon at all. The measured 64-bit debt
is small and countable, so native 64-bit with dual-target C is the better path. wasm32 remains useful as a cheap
second CI target: it confirms that the host-independent layout typedefs are correct (0 differences) without fixing
pointers first.

## 7. Fix effort and the recommended dual-target approach

### Effort by category

Counts are unique sites from this probe. Game means src+include. Lib counts exclude libraries the port would replace
where noted.

| Category | Game | Lib | Fix | Size |
|---|---|---|---|---|
| Overlay IDs as linker symbols | 700 sites / 178 files | – | one macro: `OVERLAY_ID(n)` → `n` under `PLATFORM_PC`, and an overlay manager that does nothing (all code is resident) | hours |
| Pointer carried in `u32` params, fields and returns (SetParam APIs, task/callback user data, menu params, script and VM values, beacon/union comm) | ~311 / ~100 | ~460 / ~100 | a `uptr` typedef (= `u32` under MWCC, `uintptr_t` on host). Retype the API parameters and fields, not every cast | 2–3 weeks |
| ptrdiff/size_t/long narrowing | ~74 real (+41 intentional u64) / 54 | 77 / 24 | mostly benign `int n = p - q`. Review | days |
| Pointer alignment math | 13 / 7 | 46 / 22 | `uptr` | hours |
| Hardware registers and raw addresses | 25 raw + ~690 macro uses / 182 files | ~1,900 | HAL: `REG_*`/`reg_*` map to a host register-file struct, VRAM and palette RAM to host buffers. The game uses mostly blend/brightness and palette writes | part of the renderer HAL (the dominant porting cost, outside this probe) |
| `s64`/`u64`/`f64` alignment | 8 save blocks + all file/wire structs | – | host-only `__attribute__((aligned(4)))` typedefs (checked: 0 save differences on wasm32 and arm64) | hours |
| `unsigned long` in persistent structs | RTCDate, RivalDataSave | MSL | a typedef that stays `unsigned long` under MWCC (its codegen differs from `unsigned int`, per the comment in misc_savedata.h) and is `u32` on host | hours |
| libc headers (size_t, stdarg, wchar, stddef) | 2 va_list game files | MSL 51 files | `include/` libc headers under `#ifdef PLATFORM_DS`, the host's libc otherwise. Drop MSL on the host | days |
| G2D in-place relocation | 27 game files use it | 30 sites in 7 files, ~55 pointer fields | offset accessors, or unpack into separate host structs at load. Keep the file bytes pristine | ~1 week |
| SND file link pointers, ARM7 sound | – | nitro snd | reimplement sound (needed anyway) | part of the audio HAL |
| MWCC `asm` | 0 | 44 files / ~250 | C or no-op fallbacks: CP15 caches, IRQ, `MI_Cpu*` fast copies, CLZ (`__builtin_clz`), fx matrix asm, MSL soft-float (drop) | days |
| DS Protect | – | 11 files | exclude from the host build | none |
| Dialect odds (`__memcpy`, `L""`, static/non-static) | 0 | 6 files | shim header, small edits | hours |
| Incompatible function-pointer casts | 7 | 2 | correct the signatures | hours |
| Missing returns, uninitialized locals, documented memory bugs | 5 + 56 sometimes-uninitialized + ~34 BUGs | | BUGFIX blocks, `-ftrivial-auto-var-init` | 1–2 weeks with sanitizer runs |
| Literal-sized allocations and copies | 74 + 100 (review) | | `sizeof` when the region holds a pointer-bearing struct | days |
| Cross-TU signature mismatches | 2 | 8 (DWC string.h) | fix the declarations | hours |

In total, the game-code changes for 64-bit (pointers, layout, UB) come to **roughly 500–700 edited lines in about 160
of the 1,170 game files**, plus a few header-level typedef switches. That is around 4–6 engineer-weeks including
sanitizer triage. The HAL (graphics, sound, input, filesystem, wireless) and the C still missing from the ROM are the
dominant costs. For that last part: C files cover 94.4% of the ROM's ARM9 .text. That is an upper bound, since
incomplete files may still lack functions. Files marked complete cover 71.1%. ITCM is 4%.

### How to keep MWCC byte-identical while the host build works

1. **One platform switch.** Add a header such as `include/platform.h`, included by `types.h`, that tests `__MWERKS__`
   (the tree never tests it today) or an explicit `PLATFORM_DS` that configure.py passes. Under MWCC every typedef
   stays exactly as it is. Because `uptr` is the same `u32` (`unsigned int`) type under MWCC, retyping parameters
   changes no codegen. `ninja` verifies both SHA1s on every commit, as it does now.
2. **`uptr`/`sptr`** (`u32`/`s32` on DS, `uintptr_t`/`intptr_t` on host) for every value that carries an address.
   Prefer `void *` where the API is internal and the change doesn't alter MWCC output (`void *` vs `u32` parameters can
   change register allocation, so `uptr` is the safe default).
3. **Pointer-free persistent structs.** Save blocks already are. Add host-only `_Static_assert(sizeof(T) == N)` and
   `offsetof` checks for every save block, file struct and network packet. The 61 ROM-verified sizes and 64 size
   comments are the seed list, and `rom_sizeof_check.py` generates more. Use `aligned(4)` typedefs (`s64`, `u64`,
   `f64`) on the host so that DS saves and DS↔PC wire packets keep the DS layout.
4. **Offset-based relocation for loaded data.** In host builds, replace `NNSi_G2dUnpackOffset` with either accessor
   macros (`NNS_G2D_PTR(bank, field)` that add the offset each time), or a load-time deep copy into host-layout runtime
   structs. MWCC keeps the original macro.
5. **Host libc.** Put `include/{stddef,stdarg,stdio,stdlib,string,...}.h` behind the platform switch (or use
   `#include_next`). Build only `lib/msl/src/math` (fdlibm) on the host, for bit-exact transcendentals.
6. **HAL headers.** Under `PLATFORM_PC`, nitro's `reg_*`/`REG_*` and `HW_*` macros point at a host-side register file
   and VRAM image. Raw literal addresses in game code (5 non-comment sites) move to those macros.
7. **CI ratchets.**
   * Run `run_all.sh` (≈8 minutes on this Mac) per PR, or at least the arm64 probe and `table.py`, and fail when game
     64-bit sites, failing files or layout diffs against the ROM checks increase.
   * Extend `tools/decomp/check_style.py` with a rule against new `(u32)` casts of pointers in `src/`, pointing to
     `uptr`.
   * Add the wasm32 + aligned(4) layout check: 0 differences against i386 is the expected value.
