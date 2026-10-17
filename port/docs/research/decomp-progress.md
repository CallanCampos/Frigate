# pokebw2: what a native port can use today

Measured on `0c2bbee9d` (origin/main as of 2026-10-16 19:41 CDT, the merge of PR #338), Black 2 (`b2_us`) unless
stated otherwise. The question throughout is not byte matching but **whether each function and data table exists as
C**.

## How to re-run

The scripts are in `scratchpad/research/work/progress/` (call it `P`). They read the repository and write only under
`P/out` (the compile check uses a temporary directory). `<repo>` needs `extract/{b2_us,w2_us}` and `.venv`.

```sh
sh P/run_all.sh <repo>            # everything below, about 5 minutes; or step by step:
<repo>/.venv/bin/python P/progress.py --root <repo> --version b2_us --out P/out/b2_us    # tiers, 6 s
<repo>/.venv/bin/python P/progress.py --root <repo> --version b2_us --rev <sha> --out ...  # same, at any commit
<repo>/.venv/bin/python P/compile_check.py --root <repo> --out P/out/compile_b2_us.tsv    # compile incomplete files
<repo>/.venv/bin/python P/history.py --root <repo> --out P/out/history                  # one run per day, cached
python3 P/summarize.py --in P/out/b2_us --issues P/out/gh_issues.json --compile P/out/compile_b2_us.tsv  # tables
python3 P/area_history.py P/out/history;  python3 P/gh_claims.py P/out/gh_issues.json
python3 P/nonmatching_scan.py <repo> [--list 'semantic?']
```

- `progress.py` (+ `cparse.py`, `rombytes.py`): every function symbol of every module's `symbols.txt` is placed in the
  `delinks.txt` entry whose range holds it and given a tier. Function presence in an incomplete file's C is decided by
  name, then by source-order alignment (statics often have a C name while the config keeps `func_ovNNN_xxxxxxxx`).
  Linker veneers (`bx pc` / `ldr ip,[pc]; bx ip` stubs that dsd lists as functions) are recognized from the ROM bytes
  and excluded. Outputs: `functions.tsv` (one row per function), `files.tsv`, `data.json` (data ranges by tier, with
  the relocation count inside each range).
- `compile_check.py` compiles every incomplete `.c` with the repository's own compiler setup (via
  `tools/decomp/compiler_probe.py`) and compares functions, code, data bytes and data relocations against the
  original ranges. That is the ground truth for "is it in C" for incomplete files.
- `history.py` runs `progress.py` on the last first-parent commit of each day (`git cat-file`, no checkout).
- Validation: at commit `4fc235c` (the one CI's ByteMatch report is from), every function this script puts in tier (c)
  has 0% fuzzy match in ByteMatch's objdiff report (5,144 of 5,144), and 6,093 of the 6,110 tier (b) functions have a
  compiled counterpart there. Of the other 17, 15 are statics whose config name differs from their C name, so objdiff
  can't pair them (the compile check shows them emitted), and 2 are statics defined in the C that objdiff reported at
  0% (`SSLi_ApplyConfiguredPriority`, `DownloadPlay_GetChildBmp`).

## Headline

| | Functions | Code bytes |
|---|---:|---:|
| Total (all ARM9 modules, without 2,384 linker veneers) | 41,018 | 3,850,244 |
| (a) in a `complete` file: matching C, linked from C | 31,068 (75.7%) | 2,751,648 (71.5%) |
| (b) written in C in an incomplete file (C exists, not linked, ROM code used) | 6,354 (15.5%) | 747,236 (19.4%) |
| **(c) no C at all (only the original machine code)** | **3,596 (8.8%)** | **351,360 (9.1%)** |
| **C exists, (a)+(b)** | **37,422 (91.2%)** | **3,498,884 (90.9%)** |

- Tier (c) is in **six places only**: overlay 11 (Wi-Fi stack, 143,146 B), overlay 36 (field engine, 110,808 B, all
  in 35 registered files whose `.c` isn't written), overlay 182 (Wi-Fi settings utility, 79,342 B), the DSi LTD
  autoload (TwlSDK, 11,164 B), the ITCM (4,788 B) and ARM9 main (2,112 B: crt0, BIOS SVC stubs, `exit`). Every other
  module, including all 257 other code overlays and all of ARM9 main's game code, has C for every function.
- Game code with no C: **119 KB** (ov036 110,808 + ov182's Download Play tail 8,482) plus 3,024 B of Game Freak's
  ClAct OAM renderers in ITCM. The other ~229 KB of tier (c) is library code a port would mostly replace: DWC/GameSpy
  (143 KB), the DWC utility (71 KB), TwlSDK DSi-only code (11 KB), NitroSDK ITCM/crt0 (3.8 KB).
- Of the 6,354 tier (b) functions, 432 (145 KB) have a row in `docs/nonmatching-functions.md`; the other 5,922
  (602 KB) match on their own, and their files wait on those rows or on data. Tier (b) C is not verified by the ROM
  hashes (the original object is linked), so its correctness rests on objdiff and review.
- Data (`.rodata`+`.data`, 862,796 B): 336,528 B in complete files; incomplete files' C emits 56,541 of their
  73,348 B; gaps hold 432,956 B, of which 390,324 B is the DSi Teak DSP firmware (a binary blob, never C) and 6,944 B
  the linker's overlay digest table. Discounting those two, **about 84% of the game's initialized data is C**, and
  about 72.5 KB is not. **3,707 of the 13,063 pointer words in data (28%) are not yet in C**, 75% of them in ov036
  and 18% in ov012's event-script command tables.
- ARM7: nothing is configured or decompiled (the ARM7 and ARM7i binaries are reinserted as extracted).
- White 2: the same files, the same completeness and the same tiers function for function; no C exists for only one
  version.
- Pace: over the last 7 days, tier (c) fell by 1.62 MB (from 1.98 MB to 351 KB), 826 files were completed and
  19,569 functions moved into complete files. At each area's own recent rate tier (c) reaches zero around Oct 20-25 in
  ov036, Oct 25-30 in ov011, and mid-November in ov182 (the long pole); ITCM and crt0 aren't moving and aren't tracked.
- CI's dashboard (ByteMatch, commit `4fc235c`, PR #327) lags `0c2bbee9d`: it shows 68.40% fully linked; the same
  measure at `0c2bbee9d` is about 71.4% (2,751,648 B of complete functions plus about 10.6 KB of veneers in complete
  units).

## 1. How the configs record source files and completeness

- Each module has `config/<v>/arm9/[<module>/]delinks.txt`: the module's sections first (`.text`, `.rodata`, `.ctor`,
  `.data`, `.bss`, plus `.ltdmain` in the LTD autoload; no `.init` section exists in this ROM), then one entry per
  original source file: its path, an optional `complete` line, and its range in each section. 344 overlays, ARM9 main,
  ITCM, DTCM, autoloads 2 and 3 and the LTD autoload; 84 overlays have no code (83 are empty).
- 1,604 entries in each version: **1,400 `complete`, 169 incomplete with a `.c`, 35 incomplete without a `.c`**
  (all 35 in `src/ov036`, registered for their boundaries only). Entries by origin: game 1,205 (1,094 complete, 76
  incomplete, 35 without `.c`), libraries 399 (306 complete, 93 incomplete).
- A complete file links its compiled object. **An incomplete file links the original code delinked from the ROM**
  (`build/<v>/delinks/<path>.o`), and its C is still compiled by `ninja` (so it must compile) and fed to objdiff.
  Code in no entry is linked from dsd's gap objects (`_dsd_gap@<module>_<n>`), also original machine code.
  `docs/decompiling.md`: "ROM hashes also do not validate an incomplete file's C, because that file still links the
  original object until marked complete."
- The rule (`docs/code-organization.md`) allows an entry to cover a whole file while only some of its functions are
  written. In practice **168 of the 169 incomplete files with a `.c` define every function of their range**: the
  compile check emits at least as many functions as the ROM range holds in all of them (more where inline helpers are
  emitted). The exception is `lib/dwcutil/src/dwcutil_ac.c` (overlay 182): 1 of 66 functions (66 of 14,312 code
  bytes, no data). Concrete checks:
  - `src/ov036/shortcut_menu.c`: 24 of 24 functions in C (9 match by config name, 15 statics have C names such as
    `SetupMenu` while the config still says `func_ov036_021bf204`; the probe calls those "unknown"), data 96/96 and
    16/16 B; incomplete because `func_ov036_021bedd8` is 0x224 against 0x22c bytes.
  - `src/ov012/delivery_beacon.c`: 22 functions, all matching; incomplete over string-literal order in `.data`
    (214 of 220 B emitted).
  - `lib/nitrowifi/src/ssl/sha1.c`: all 8 functions in C, but `Sha1Transform` is a loop (0x1ac B) where the ROM's is
    unrolled (0x528 B): C present, not matching.
  - `src/ov036/fldmmdl_blact.c`: every function in C, but its tables are not: 180 of 4,052 data bytes, 0 of 818
    pointer words.
- Tier (b) therefore means "every function in C"; what incomplete files can still lack is **data** (section 3).

## 2. Code tiers

### By module

| Module | Functions | Code bytes | (a) bytes | (b) bytes | (c) bytes | (c) % | (c) functions |
|---|---:|---:|---:|---:|---:|---:|---:|
| ARM9 main | 7,727 | 562,744 | 467,928 | 92,704 | 2,112 | 0.4% | 35 |
| ITCM | 15 | 4,988 | 48 | 152 | 4,788 | 96.0% | 13 |
| DTCM, autoloads 2 and 3 | 0 | 0 | 0 | 0 | 0 | - | 0 |
| LTD autoload (DSi only) | 347 | 44,754 | 18,844 | 14,746 | 11,164 | 24.9% | 118 |
| Overlays (260 with code) | 32,929 | 3,237,758 | 2,264,828 | 639,634 | 333,296 | 10.3% | 3,430 |

Tier (c) split: 226,332 B in no entry (dsd gaps: ov011, ov182, LTD, ITCM, main), 110,808 B in the 35 entries without
a `.c` (ov036), 14,220 B in `dwcutil_ac.c`. 183 functions (22,134 B) of tiers (a)+(b) are MWCC `asm` functions, all in
`lib/` (MSL's soft-float runtime `fp_fastI.c` 47, NitroSDK `mi_memory`, `os_*`, `fx_mtx33` and others 94, DS Protect
31, DWC `crypt.c` 6, TwlSDK 5): C files, but assembly a port must replace.

### By origin

Gap code has no file, so its origin is attributed: by both neighbouring files when they agree; ov011's gaps all lie
inside the DWC/GameSpy stack (`0x0215dd74` on, between `lib/` files); ov182's first gap follows `dwc_util_screen.c`
(DWC utility screens, "likely"), its second follows `download_play_sys.c` (game); ITCM's first four functions are the
ClAct OAM renderers that `src/gfl/clact.c`'s renderer table names (`mainRenderer`, `func_01ff8318`, ...).

| Origin | Code bytes | (a) bytes | (a) % | (b) bytes | (c) bytes | (c) % | where (c) is |
|---|---:|---:|---:|---:|---:|---:|---|
| game | 3,100,614 | 2,468,964 | 79.6% | 512,360 | 119,290 | 3.8% | ov036 110,808, ov182 8,482 |
| game (gfl ClAct renderers in ITCM) | 3,024 | 0 | 0.0% | 0 | 3,024 | 100.0% | itcm |
| lib/dpw | 7,352 | 7,352 | 100.0% | 0 | 0 | 0.0% | |
| lib/dsprot | 17,844 | 4,360 | 24.4% | 13,484 | 0 | 0.0% | |
| lib/dwc | 280,372 | 70,096 | 25.0% | 67,130 | 143,146 | 51.1% | ov011 |
| lib/dwcutil | 26,158 | 1,626 | 6.2% | 10,312 | 14,220 | 54.4% | ov182 (`dwcutil_ac.c`) |
| lib/dwcutilui (+ likely the 56,640 B ov182 gap) | 58,172 | 1,216 | 2.1% | 316 | 56,640 | 97.4% | ov182 |
| lib/irc | 1,780 | 1,780 | 100.0% | 0 | 0 | 0.0% | |
| lib/msl | 50,484 | 39,588 | 78.4% | 10,868 | 28 | 0.1% | main (`exit`) |
| lib/nitro | 94,162 | 66,596 | 70.7% | 27,566 | 0 | 0.0% | |
| lib/nitro (ITCM: `OS_IrqHandler`, reset) | 1,764 | 0 | 0.0% | 0 | 1,764 | 100.0% | itcm |
| lib/nitrowifi | 46,942 | 2,532 | 5.4% | 44,410 | 0 | 0.0% | |
| lib/nnsys | 66,030 | 35,344 | 53.5% | 30,686 | 0 | 0.0% | |
| lib/poke_net | 5,492 | 1,768 | 32.2% | 3,724 | 0 | 0.0% | |
| lib/spl | 24,600 | 24,600 | 100.0% | 0 | 0 | 0.0% | |
| lib/twl | 63,370 | 25,826 | 40.8% | 26,380 | 11,164 | 17.6% | ltd_autoload_0 (`.ltdmain`, NDMA etc.) |
| lib/twl (crt0 + BIOS SVC stubs) | 2,084 | 0 | 0.0% | 0 | 2,084 | 100.0% | main `0x02004000`-`0x02005124` |

### Overlays, by missing C

Only three overlays have any tier (c) code; the rest of the list is ordered by tier (b), the C that exists but isn't
linked. "Data not in C" is `.rodata`+`.data` in gaps and in files without `.c`, plus what an incomplete file's
compiled C emits short of its range (shortfalls under about 80 bytes with equal pointer counts are likely padding or
string pooling rather than missing tables). Issue states are from `gh issue list` today.

| Overlay | Purpose / largest files | Code bytes | (a) | (b) | **(c)** | (c) fns | files complete / incomplete / no .c | data not in C (pointer words) | issue |
|---|---|---:|---:|---:|---:|---:|---|---|---|
| ov011 | Nintendo Wi-Fi Connection stack (DWC, GameSpy, NitroWiFi SOC/SSL/CPS, VCT); cps, ssl, wcm | 215,352 | 11,132 | 61,074 | **143,146** (gap) | 972 | 16/19/0 | 24,503 (132) | #72 open, claimed |
| ov036 | field (overworld) engine; fldmmdl_acmd, field_rail, field_msgbg | 302,710 | 80,240 | 111,662 | **110,808** (no .c) | 1,548 | 69/31/35 | 24,051 (2,789) | #83 open, claimed |
| ov182 | Wi-Fi Connection settings utility (DWC util, AOSS, its screens) and Download Play | 100,562 | 5,614 | 15,606 | **79,342** (65,122 gap + 14,220 partial) | 910 | 5/6/0 | 17,553 (73) | #132 open, claimed |
| ov167 | battle engine (server flow, client, handlers) | 242,660 | 125,316 | 117,344 | 0 | 0 | 20/6/0 | 46 (0) | #120 open, claimed |
| ov257 | Xtransceiver video chat, with TwlSDK SSP JPEG | 88,804 | 18,384 | 70,420 | 0 | 0 | 8/11/0 | 70 (1) | #155 open, claimed (JPEG range) |
| ov168 | battle view: effect VM, input, MCSS | 87,012 | 21,494 | 65,518 | 0 | 0 | 9/4/0 | 12 (0) | #121 open |
| ov189 | GTS / Battle Subway Wi-Fi (DPW, DWC ghttp/nhttp) | 63,748 | 38,588 | 25,160 | 0 | 0 | 13/17/0 | 216 (0) | #134 open, claimed |
| ov165 | party list, DS Protect | 44,804 | 21,112 | 23,692 | 0 | 0 | 10/5/0 | 26 (0) | #118 open |
| ov260 | Wi-Fi battle match (rating battles) | 52,432 | 29,976 | 22,456 | 0 | 0 | 9/5/0 | 284 (0) | #157 open |
| ov181 | DS Download Play / multiboot (NitroSDK MB) | 29,050 | 7,140 | 21,910 | 0 | 0 | 6/6/0 | 23 (0) | #131 open, claimed |
| ov029 | DWC VCT voice chat | 20,198 | 2,086 | 18,112 | 0 | 0 | 1/5/0 | 38 (0) | #78 open |
| ov280 | name input screen | 16,798 | 1,606 | 15,192 | 0 | 0 | 2/1/0 | 35 (0) | #163 open |
| ov194 | Pokémon trade | 60,486 | 46,052 | 14,434 | 0 | 0 | 7/1/0 | 66 (0) | #137 open |
| ov197 | Mystery Gift album / Mystery Gift | 36,410 | 22,484 | 13,926 | 0 | 0 | 5/1/0 | 0 (0) | #139 open |
| ov337 | DS Protect | 11,568 | 2,644 | 8,924 | 0 | 0 | 2/3/0 | 2 (0) | #192 open |
| ov139 | shared touch UI: frame list, yes/no menu, touch bar | 13,316 | 5,680 | 7,636 | 0 | 0 | 5/1/0 | 33 (0) | #110 open |
| ov332 | cygnus_flow, key_system_flow, data_convert_flow (Key System / data conversion; guess) | 39,786 | 32,900 | 6,886 | 0 | 0 | 12/1/0 | 22 (0) | #191 open |
| ov214 | GTS (World Trade) screens | 59,002 | 53,412 | 5,590 | 0 | 0 | 13/1/0 | 11 (0) | #149 open |
| ov102 | roller-coaster gym gimmick | 5,394 | 0 | 5,394 | 0 | 0 | 0/1/0 | 0 (0) | #97 **closed** |
| ov033 | battle facility scripts: Battle Subway, PDW postman, Trial House | 22,470 | 18,694 | 3,776 | 0 | 0 | 23/1/0 | 20 (0) | #81 open |
| ov201 | Global Data Server client (poke_net, GDS) | 9,778 | 6,054 | 3,724 | 0 | 0 | 5/1/0 | 3 (0) | #142 open |
| ov012 | field movement models, actors, event-script engine and commands | 108,020 | 107,284 | 736 | 0 | 0 | 91/1/0 | **4,634 (674)** | #73 open |
| ov338 | infrared check | 462 | 0 | 462 | 0 | 0 | 0/1/0 | 30 (0) | #193 open |
| ov081 | `wbt_subscreen` | 834 | 834 | 0 | 0 | 0 | 1/0/0 | 16 (0) (`.data` gap) | none |

The other 236 code overlays are fully complete in C and linked. The appendix lists all 260.

The 35 ov036 files with no `.c` (110,808 B), the largest first: `fldmmdl_acmd.c` 18,536, `field_msgbg.c` 12,484,
`comm_entry_menu.c` 8,142, `ctalkmsgwin.c` 7,902, `ircbattlemenu.c` 7,176, `fieldmap_event.c` 6,998, `sodateya.c`
5,308, `field_task_manager.c` 4,758, `field_fade.c` 4,392, `weather_task.c` 4,014, `scrcmd_pokemon_misc.c` 3,106,
`place_name_letter.c` 2,816, `party_select_list.c` 2,710, `field_subscreen.c` 2,588, `scrcmd_field.c` 2,388,
`field_fmission_info.c` 2,340, `scrcmd_hidden_hollow.c` 2,064, `enceff.c` 1,812, `fld_exp_obj.c` 1,654, then 16
files under 1.4 KB each (field effects, weather, gimmicks, `field_task.c`, `field_wfbc.c`). These are core field code
(movement commands, message windows, the field task system, daycare, fades, weather): **the one tier (c) area a port
cannot stub**.

## 3. Data

| Section | Total | (a) complete file | (b) incomplete file with .c | (c) gap | (c) file without .c | pointer words (a / b / c) |
|---|---:|---:|---:|---:|---:|---|
| .rodata | 756,560 | 290,324 | 49,176 | 398,160 | 18,900 | 6,376 / 1,654 / 2,625 |
| .data | 106,216 | 46,196 | 24,172 | 34,796 | 1,052 | 1,887 / 281 / 235 |
| .bss | 961,760 | 781,164 | 97,004 | 83,592 | 0 | - |
| .ctor | 20 | 8 | 12 | 0 | 0 | 2 / 3 / 0 |

(Totals match ByteMatch's `total_data` of 1,824,556 B. Pointer words are `relocs.txt` entries whose source lies in the
range.)

What the (b) column holds in C, from compiling all 169 incomplete files:

| Over the 169 incomplete files with a .c | ROM ranges | C objects |
|---|---:|---:|
| `.text` | 768,084 | 745,494 (the difference is veneers and the nonmatching sizes) |
| `.rodata` + `.data` | 73,348 | 56,541 |
| `.bss` | 97,004 | 91,419 |
| pointer relocations in data | 1,935 | 1,088 |

Incomplete files whose C clearly lacks data: `dwcutil_ac.c` (10,488 B data, 5,428 B bss: none in C),
`src/ov036/fldmmdl_blact.c` (4,052 B, 818 pointer words: 180 B and 0 pointers in C), `src/ov182/download_play_wih.c`
(872 B, 10 pointers: none in C), `src/system/wipe_sub.c` (576 B: 155 in C), `src/ov036/field_buildmodel.c` (152 B, 12
pointers: 35 B, none), `src/ov036/fieldmap.c` (3 pointers short), `ov257/ctvt_game.c` and `ov036/fld_act_rail.c` (1
each). 22 other files are short by 19 to 71 bytes with the same pointer count: string order, pooling or padding,
the reasons their data rows in `docs/nonmatching-functions.md` give, rather than missing tables.

Data in gaps and in files without `.c`, by module (bytes, pointer words, and for gaps the pointer-bearing objects):

| Module | Section | Bytes | Pointer words | What it is |
|---|---|---:|---:|---|
| ltd_autoload_0 | .rodata | 390,324 | 0 | the two Teak DSP COFF images (DSi camera/mic/audio firmware): a blob, not C |
| ov036 | .bss | 65,588 | - | field buffers of the unwritten files |
| ov011 | .data | 24,064 | 132 (27 objects, 1,608 B) | DWC/GameSpy tables |
| ov036 | .rodata (no .c) | 18,900 | 1,941 | tables of the 35 unwritten files, mostly pointers (e.g. movement-command tables) |
| ov011 | .bss | 17,036 | - | DWC/GameSpy state |
| main | .data | 6,944 | 0 | `SDK_OVERLAY_DIGEST` (overlay HMACs the linker writes): not C |
| ov012 | .rodata | 4,628 | 674 (2 objects, 3,080 B) | `GLOBAL_SCRIPT_TABLE`, `EVCMD_PERM_TABLE`, `VM_CMP_LUT`, `FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS`, **`EVCMD_TABLE`** (the event-script command handler table) |
| ov182 | .data / .rodata | 6,156 | 59 (12 objects) | DWC utility |
| ov036 | .data (no .c) | 1,052 | 14 | |
| dtcm | .data | 160 | 32 | NitroSDK IRQ handler table (a port replaces it) |
| main | .rodata | 140 | 8 | |
| ov182, ltd, dtcm, ov029, ov081, ov167, ov189 | .bss / small | about 1,500 | - | |

**For a port**: of the 862,796 B of initialized data, 397,268 B are not program data (DSP firmware, overlay digest).
Of the remaining 465,528 B, about 393,069 B (84%) exist as C (complete files plus what incomplete files emit) and about
72.5 KB do not: ~35.7 KB in gaps (ov011 24.5 KB, ov182 6.2 KB, ov012 4.6 KB), 19,952 B in the 35 unwritten ov036
files, ~16.8 KB missing from incomplete files' C. **Pointer words not in C: 3,707 of 13,063 (28%)**: ov036 2,789
(1,955 in unwritten files, 818 in `fldmmdl_blact.c`, 16 in three other files), ov012 674 (script command tables),
ov011 132, ov182 73, DTCM 32, main 8, ov257 1. These are the tables that cannot be carried over as blobs into a 64-bit build. The `.bss` without C is 89 KB
(83.6 KB in gaps, 5.6 KB short in incomplete files, mostly `dwcutil_ac.c`): sizes are known, types aren't.

## 4. ARM7

Nothing ARM7 is configured or decompiled. `config/<v>/` has only `arm9/`; `extract/<v>/arm7/arm7.bin` (167,812 B) and
`extract/<v>/dsi/arm7i.bin` (291,064 B) are extracted and reinserted unchanged, with no symbols, delinks or sources.
The ARM7 runs Nintendo's SDK components (sound driver and sequence player, wireless, touch panel/SPI, RTC, and the
DSi ARM7i extensions), so a port must supply host replacements, notably an SDAT/SSEQ sequence player for the sound
library whose ARM9 side (`lib/nitro` SND, `lib/nnsys` sound) is in C.

## 5. White 2

`config/w2_us` lists the same 1,604 files with the same `complete` markers (the scripts write both versions). Run on
White 2, `progress.py` gives the same tier for every function (43,402 symbols, compared by name; no `_w2_us`-only
function symbols) and the same per-file counts; code bytes differ by about 100 B across tiers because of the 112
functions that differ between versions. Data differs by 28 B (one small `.data` gap in White 2). 50 source files and
3 headers carry `#ifdef BLACK2`/`WHITE2` code (most in ov012, ov332, ov036, src/system, ov294, ov214, ov194). Nothing
exists as C for only one version. The compile check was run for Black 2 only.

## 6. Pace

The repository's history starts on 2026-09-27 (20 days, 2,438 commits), so "the last 4-8 weeks" is the whole project.
ByteMatch's history covers only 2026-10-10 to 10-16 (211 entries, overall measures only: complete code 917,426 to
2,645,428 B, matched functions 20,183 to 36,848). Measured with `history.py` (last first-parent commit of each day):

| Day | Files complete | Functions in (a) | (a) bytes | Functions with C, (a)+(b) | (c) bytes |
|---|---:|---:|---:|---:|---:|
| 09-27 | 19 | 375 | 31,304 | 375 | 3,792,934 |
| 10-02 | 637 | 2,897 | 194,468 | 2,997 | 3,609,490 |
| 10-03 | 125 | 2,136 | 163,766 | 3,486 | 3,574,890 |
| 10-07 | 515 | 10,522 | 854,488 | 19,528 | 2,044,636 |
| 10-09 | 574 | 11,499 | 917,426 | 20,549 | 1,975,176 |
| 10-11 | 708 | 15,260 | 1,126,482 | 26,898 | 1,362,688 |
| 10-12 | 901 | 19,741 | 1,528,286 | 33,318 | 751,472 |
| 10-13 | 1,079 | 25,959 | 2,217,262 | 33,788 | 710,114 |
| 10-14 | 1,160 | 28,405 | 2,473,152 | 34,191 | 661,150 |
| 10-15 | 1,179 | 28,678 | 2,515,612 | 34,270 | 653,606 |
| 10-16 | 1,400 | 31,068 | 2,751,648 | 37,422 | 351,360 |

(On 10-03 per-function entries were folded into original files, so file counts before and after don't compare.)

| Week | Files completed | Functions into (a) | Bytes into (a) | Functions gaining C | Tier (c) bytes |
|---|---:|---:|---:|---:|---:|
| 09-27 to 10-02 | - | +2,522 | +163,164 | +2,622 | -183,444 |
| 10-02 to 10-09 | (restructured) | +8,602 | +722,958 | +17,552 | -1,634,314 |
| 10-09 to 10-16 | +826 | +19,569 | +1,834,222 | +16,873 | -1,623,816 |

Tier (c) bytes per area over time:

| Day | main | ITCM | LTD | ov011 | ov036 | ov182 | other overlays |
|---|---:|---:|---:|---:|---:|---:|---:|
| 09-27 | 560,984 | 4,988 | 33,628 | 215,340 | 302,710 | 100,562 | 2,574,722 |
| 10-09 | 298,520 | 4,988 | 33,628 | 215,348 | 196,468 | 100,562 | 1,125,662 |
| 10-12 | 190,890 | 4,988 | 42,250 | 206,824 | 192,208 | 100,562 | 13,750 |
| 10-13 | 190,410 | 4,988 | 42,250 | 192,240 | 192,208 | 88,018 | 0 |
| 10-15 | 172,692 | 4,988 | 42,250 | 156,486 | 192,208 | 84,982 | 0 |
| 10-16 | 2,112 | 4,788 | 11,164 | 143,146 | 110,808 | 79,342 | 0 |

(The LTD figure rose on 10-10 when dsd's analysis found more of its functions.)

**When tier (c) reaches zero, at each area's own recent rate:**

| Area | Left | Rate, last 7 days | ETA | Rate, last 3 days | ETA | Who |
|---|---:|---:|---|---:|---|---|
| ov036 field | 110,808 | 12.2 KB/day | ~Oct 25 | 27.1 KB/day | ~Oct 20 | mariano54 (claim; re-landing a fork branch) |
| ov011 Wi-Fi stack | 143,146 | 10.3 KB/day | ~Oct 30 | 16.4 KB/day | ~Oct 25 | mariano54 (claim; same fork) |
| ov182 Wi-Fi utility | 79,342 | 3.0 KB/day | ~Nov 11 | 2.9 KB/day | ~Nov 12 | mariano54 (claim) |
| LTD (TwlSDK, DSi) | 11,164 | lumpy (31 KB on 10-16) | days | | | no issue tracks it |
| ITCM (ClAct renderers, IRQ) | 4,788 | 200 B in 20 days | none | | | no issue tracks it |
| main (crt0, SVC stubs, `exit`) | 2,112 | flat since 10-16 | none | | | no issue tracks it |

Taken together: **tier (c) for game code (ov036 and the 8.5 KB Download Play tail) could be gone within one to two
weeks; all tier (c) by mid-November if ov182 keeps its pace**, sooner if the fork's ov182 work lands in bulk, and only
if someone picks up the untracked ITCM/crt0/LTD remnants (7 KB, of which a port needs only the 3 KB of ClAct
renderers). These are extrapolations from a 20-day-old project run by several parallel, largely AI-assisted
contributors, whose progress comes in large merges (10-16 alone removed 302 KB of tier (c)); the remaining code is the
tail that earlier passes left. For tier (b) the matching tail is slower: 432 documented nonmatching functions (145 KB),
many tried repeatedly; a port doesn't need them to match, only to be correct.

## 7. GitHub tracking

`gh issue list --repo fuddlesworth/pokebw2 --state all --limit 500`: 128 issues, 123 of them `[Decomp] Overlay NNN`:
**101 closed, 22 open**. Issues exist only for overlays that had work left when they were opened; ARM9 main, the ITCM
and the LTD autoload have none. The issue bodies' "current source coverage" lines date from when each was opened and
are stale.

Open overlay issues and claims (from the comments: a "Claiming overlay NNN" without a later release by the same
author):

| Issue | Overlay | Active claim | Released by |
|---|---|---|---|
| #72 | 011 | **mariano54** (10-16: DWC/GameSpy/SOC/SSL stack, re-landing his fork) | Yvonne-Aizawa, CallanCampos |
| #83 | 036 | **mariano54** (10-16) | fascix |
| #120 | 167 | **CallanCampos** (10-17 UTC: last nonmatching functions of three files) | TeenageMutantNinjaTurtle, fuddlesworth |
| #131 | 181 | **manta89640** (10-12; last progress 10-15) | |
| #132 | 182 | **mariano54** (10-16: "the rest of overlay 182") | Yvonne-Aizawa, fuddlesworth, CallanCampos, Cbow2018 |
| #134 | 189 | **manta89640** (10-12; last progress 10-15) | |
| #155 | 257 | **manta89640** (SSP JPEG/EXIF range, 10-12; last progress 10-15) | fuddlesworth (game-code range) |
| #73, #78, #81, #110, #118, #121, #137, #139, #142, #149, #157, #163, #191, #192, #193 | 012, 029, 033, 139, 165, 168, 194, 197, 201, 214, 260, 280, 332, 337, 338 | none | |

Every overlay with tier (b) or (c) code has an open issue except ov102 (`gimmick_roller_coaster.c` incomplete, issue
#97 closed) and ov081 (a 16-byte `.data` gap). The two non-overlay open issues are #325 (names for address-named
functions) and #315 (duplicate functions).

## 8. Nonmatching functions

`docs/nonmatching-functions.md` has **442 table rows: 436 functions and 6 data-layout rows**, plus 5 "matching, but not
linkable as is" items (DS Protect's `*_multi.c`, three NitroSDK OS files waiting on linker symbols, MSL's
`fp_fastI.c` names, `fldmmdl_g3dobj.c`'s `.rodata` boundary). By area: lib/nitrowifi 129, src/ov036 75, lib/dwc 65,
lib/nitro 26, lib/nnsys 23, lib/dsprot 17, lib/twl 14, ov257 13, ov167 13, lib/dwcutil 13, ov168 10, and 1-8 in 19
others. A keyword pass (`nonmatching_scan.py`) puts 178 rows as register allocation or order, 198 as size or
scheduling differences, 53 other and 13 as possibly semantic; a manual read of those 13 and of every row (about 60)
that mentions behaviour, a wrong or unknown result, an omission, a return value or signedness gives:

- **One row records a functional difference in the C as it compiles**: `CPS_SslCanRead`
  (`lib/nitrowifi/src/ssl/ssl.c`): "the final `bne` branches to the wrong one of the two adjacent return blocks
  (`return 0` vs `return -1`)". The compiled C returns a different value on one path from the ROM.
- **Semantically uncertain**, rows that say the C was rewritten for behaviour and is still unverified by linking:
  NitroWiFi's `wcm.c` (`WCM_ConnectAsync`, `WCM_SetSearchOption`, `WCM_GetChannelMask`, `WCM_SetConfig`,
  `WCM_ConnectStep5`, `WCM_SendFrame`, `WCM_AssembleSendFrame`, `WCM_RssiToQuality`) and `ssl.c`
  (`SSLi_ProcessRecord`: "Replaced the stub/wrong signature", `SSLi_CheckCertificate`, `SSLi_IncrementCounter`,
  `SSLi_ReceiveRecord`, `SSLi_AppendCertificateNameValue`), and DWC's `gt2.c` (`Gt2_OpenConnection`: "Restored ...
  the omitted new-connection pending registration"). All 129 nitrowifi rows deserve that caution: only 2.5 KB of
  NitroWiFi is linked from C.
- **Unknown declarations or layouts** (behaviour likely right, structure guessed): `WifiBattleMatchNet_GetArenaFieldName`
  and the `wifibattlematch_net.c` `.data` order ("what the original declares there ... is not known"),
  `VCT_ResetCapture`/`VCT_InitCapture` (a static struct's fields at other offsets than the ROM's; consistent inside
  the file), `namein.c` (`.rodata` constants whose owners are not identified).
- **Different code, stated equivalent**: `CARDi_InitTwlRom` (stores the ROM reader through another symbol than the
  original's base pointer: check it is the same object), `MIi_DmaSetParameters` ("the register writes have the same
  effect"), `Sha1Transform` (loop against the ROM's unrolled form), `BigInt_Div` ("semantically verified against the
  asm", frame differs), `BigInt_ModExp` (one `asr` where the ROM has `lsr`, on a bit index that is never negative),
  `func_ov182_021ba4f8` (the ROM falls through after `OS_Terminate`, which doesn't return), the `WCM_*` rows that omit
  "redundant" tests.
- The rest describe register allocation, instruction order, stack slots or a few bytes of size from scheduling, with
  the same operations: no functional difference. ByteMatch's per-function `functional` field
  (`/api/v1/projects/fuddlesworth/pokebw2/units?unit=...`) is null for this project, so there is no automated
  equivalence check to lean on.

Beyond the rows: 5,922 tier (b) functions match on their own but are unlinked, and objdiff's masking ignores
relocated bytes, so a wrong call target or data reference would not show (`docs/decompiling.md` says as much). For a
port, tier (b) needs an equivalence pass, by linking each file in a test build or by differential testing.

## 9. ByteMatch endpoints

- `https://bytematch.dev/fuddlesworth/pokebw2.json`: latest commit's measures and per-category measures (ARM9 main,
  overlays, game, libraries and each library).
- `...pokebw2.json?mode=history&version=b2_us`: 211 entries since 2026-10-10 19:23 UTC, overall measures only.
- `...pokebw2.json?mode=report&version=b2_us`: the full objdiff report (5.4 MB): 1,625 units with per-section and
  per-function fuzzy match.
- `https://bytematch.dev/api/v1/projects/fuddlesworth/pokebw2/units?unit=<unit>&version=b2_us[&commit=<sha>]`: one
  unit's functions; `.../source?unit=<unit>[&function=<name>]`: its source. Documented at `https://bytematch.dev/docs/api`.
- The dashboard's latest report is commit `4fc235c` (PR #327, 18:59 UTC): 1,336 of 1,625 units complete. At
  `0c2bbee9d` this measurement counts 1,400 complete files.

## 10. The port's blocker list

1. **ov036 field code with no C**: 35 files, 110,808 B, plus 19,952 B of their tables (1,955 pointer words) and the
   818-pointer tables of `fldmmdl_blact.c`. Claimed, actively being landed.
2. **ov012's event-script tables** (`EVCMD_TABLE` and four others, 4,628 B, 674 pointer words) in a `.rodata` gap,
   while all ov012 code is C: small, but every script command dispatches through them.
3. **ITCM ClAct renderers** (4 functions, 3,024 B of game code): untracked.
4. **ov182 Download Play tail** (8,482 B of game code after `download_play_sys.c`) and `download_play_wih.c`'s data.
5. Library code a port would replace or stub rather than need: DWC/GameSpy (ov011, 143 KB), the DWC utility (ov182,
   71 KB), TwlSDK DSi code (LTD, 11 KB), NitroSDK ITCM IRQ/reset and crt0 (3.8 KB), 183 `asm` functions in `lib/`,
   DS Protect, and the whole ARM7 (sound sequence player included).
6. **Correctness of tier (b)** (747 KB of C not checked by the ROM hash): one known functional difference
   (`CPS_SslCanRead`) and the semantically reworked NitroWiFi/GT2 rows.

## Appendix: every overlay with code

Code bytes per tier, the tier (c) split (in no file, in files without `.c`, missing from a partial `.c`), files
complete/incomplete/without `.c`, data not in C with its pointer words, and the overlay's issue. Sorted by tier (c),
then tier (b). Generated by `summarize.py --all-overlays`.

| Overlay | Purpose / largest files | Code bytes | (a) | (b) | (c) gap | (c) no .c | (c) partial | (c) total | (c) fns | files complete/incomplete/no .c | data not in C (ptr words) | issue |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|---|
| ov011 | Nintendo Wi-Fi Connection stack (DWC, GameSpy, NitroWiFi SOC/SSL/CPS, VCT); cps, ssl, wcm | 215,352 | 11,132 | 61,074 | 143,146 | 0 | 0 | **143,146** | 972 | 16/19/0 | 24,503 (132) | #72 open |
| ov036 | field (overworld) engine: map, player, NPCs, rails, scripts, weather; fldmmdl_acmd, field_rail, field_msgbg | 302,710 | 80,240 | 111,662 | 0 | 110,808 | 0 | **110,808** | 1548 | 69/31/35 | 24,051 (2789) | #83 open |
| ov182 | Wi-Fi Connection settings utility (DWC util/AOSS) and Download Play; dwcutil_ac, dwc_aoss, download_play_wih | 100,562 | 5,614 | 15,606 | 65,122 | 0 | 14,220 | **79,342** | 910 | 5/6/0 | 17,553 (73) | #132 open |
| ov167 | battle engine (server flow, client, handlers); btl_server_flow, move_handlers, btl_client | 242,660 | 125,316 | 117,344 | 0 | 0 | 0 | **0** | 0 | 20/6/0 | 46 (0) | #120 open |
| ov257 | Xtransceiver (C-Gear video chat) with SSP JPEG; ctvt_game, ctvt_talk, ctvt_draw | 88,804 | 18,384 | 70,420 | 0 | 0 | 0 | **0** | 0 | 8/11/0 | 70 (1) | #155 open |
| ov168 | battle view: effects VM, input, MCSS; btlv_effvm, btlv_input, btlv_mcss | 87,012 | 21,494 | 65,518 | 0 | 0 | 0 | **0** | 0 | 9/4/0 | 12 (0) | #121 open |
| ov189 | GTS / Battle Subway Wi-Fi (DPW, DWC ghttp/nhttp); crypt, nhttp_thread, nd | 63,748 | 38,588 | 25,160 | 0 | 0 | 0 | **0** | 0 | 13/17/0 | 216 (0) | #134 open |
| ov165 | party list (pokemon list) + DS Protect; plist_sys, plist_plate, plist_item | 44,804 | 21,112 | 23,692 | 0 | 0 | 0 | **0** | 0 | 10/5/0 | 26 (0) | #118 open |
| ov260 | Wi-Fi battle match (rating battles); wifibattlematch_link_competition, wifibattlematch_net, wifibattlematch_link_random | 52,432 | 29,976 | 22,456 | 0 | 0 | 0 | **0** | 0 | 9/5/0 | 284 (0) | #157 open |
| ov181 | DS Download Play / multiboot (NitroSDK MB); mb_parent_sys, mb_wm, mb_comm | 29,050 | 7,140 | 21,910 | 0 | 0 | 0 | **0** | 0 | 6/6/0 | 23 (0) | #131 open |
| ov029 | DWC VCT voice chat; vct_audio, vct_adpcm, vct_session | 20,198 | 2,086 | 18,112 | 0 | 0 | 0 | **0** | 0 | 1/5/0 | 38 (0) | #78 open |
| ov280 | name input screen; namein, namein_data, namein_graphic | 16,798 | 1,606 | 15,192 | 0 | 0 | 0 | **0** | 0 | 2/1/0 | 35 (0) | #163 open |
| ov194 | Pokemon trade (union/link trade); pokemontrade_proc, pokemontrade_2d, pokemontrade_nego | 60,486 | 46,052 | 14,434 | 0 | 0 | 0 | **0** | 0 | 7/1/0 | 66 (0) | #137 open |
| ov197 | Mystery Gift album / Mystery Gift; mystery_album, mystery, mystery_util | 36,410 | 22,484 | 13,926 | 0 | 0 | 0 | **0** | 0 | 5/1/0 | 0 (0) | #139 open |
| ov337 | DS Protect; dsprot_detect_multi, dsprot_run_multi, dsprot_stub_check_multi | 11,568 | 2,644 | 8,924 | 0 | 0 | 0 | **0** | 0 | 2/3/0 | 2 (0) | #192 open |
| ov139 | shared touch UI: frame list, yes/no menu, touch bar; frame_list, yesno_menu, touchbar | 13,316 | 5,680 | 7,636 | 0 | 0 | 0 | **0** | 0 | 5/1/0 | 33 (0) | #110 open |
| ov332 | Unova Link-style data flows: Cygnus, Key System, data convert (guess from file names); cygnus_flow, key_system_flow, data_convert_flow | 39,786 | 32,900 | 6,886 | 0 | 0 | 0 | **0** | 0 | 12/1/0 | 22 (0) | #191 open |
| ov214 | GTS (World Trade) screens; worldtrade_input, worldtrade_box, worldtrade_search | 59,002 | 53,412 | 5,590 | 0 | 0 | 0 | **0** | 0 | 13/1/0 | 11 (0) | #149 open |
| ov102 | roller-coaster gym gimmick; gimmick_roller_coaster | 5,394 | 0 | 5,394 | 0 | 0 | 0 | **0** | 0 | 0/1/0 | 0 (0) | #97 closed |
| ov033 | battle facility scripts: Battle Subway, PDW postman, Trial House; bsubway_scr, pdw_postman, trial_house | 22,470 | 18,694 | 3,776 | 0 | 0 | 0 | **0** | 0 | 23/1/0 | 20 (0) | #81 open |
| ov201 | Global Data Server client (poke_net, GDS); poke_net, gds_rap, poke_net_gds | 9,778 | 6,054 | 3,724 | 0 | 0 | 0 | **0** | 0 | 5/1/0 | 3 (0) | #142 open |
| ov012 | field movement models, actors, event-script engine and commands; fldmmdl, field_actor_tool, scrcmd_pokemon | 108,020 | 107,284 | 736 | 0 | 0 | 0 | **0** | 0 | 91/1/0 | 4,634 (674) | #73 open |
| ov338 | infrared check; ir_check | 462 | 0 | 462 | 0 | 0 | 0 | **0** | 0 | 0/1/0 | 30 (0) | #193 open |
| ov000 | event_gsync | 1,028 | 1,028 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov001 | event_gtsnego | 988 | 988 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov002 | event_wifibattlematch | 416 | 416 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov003 | event_battle_video | 264 | 264 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov004 | event_worldtrade | 456 | 456 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov005 | event_wbt_wifi | 200 | 200 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov008 | event_wificlub | 2,432 | 2,432 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov009 | event_wifi_bsubway | 276 | 276 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov010 | event_make | 1,178 | 1,178 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #71 closed |
| ov013 | event_beacon_detail | 328 | 328 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov014 | event_pass_power | 632 | 632 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov015 | event_medal_info_beacon | 328 | 328 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov016 | event_egg_demo | 296 | 296 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov017 | event_comm_tvt | 364 | 364 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov018 | event_research_radar | 228 | 228 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov020 | event_poke_status | 882 | 882 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov021 | event_festival | 546 | 546 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov022 | event_wbt | 792 | 792 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov023 | event_pokewood | 4,552 | 4,552 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #74 closed |
| ov024 | fest_mission_sparkle_place | 662 | 662 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #75 closed |
| ov025 | fest_mission_people | 1,780 | 1,780 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #76 closed |
| ov026 | fest_mission_hollow | 334 | 334 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov027 | scrcmd_survey, fest_mission_data | 2,780 | 2,780 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov028 | union_comm, union_subproc, union_receive | 10,544 | 10,544 | 0 | 0 | 0 | 0 | **0** | 0 | 6/0/0 | 0 (0) | #77 closed |
| ov030 | wih, net_whpipe, net_devwireless | 9,592 | 9,592 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #79 closed |
| ov031 | net_irc, irc_link, irc_spi | 3,602 | 3,602 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #80 closed |
| ov032 | wfc_user_info_warning | 924 | 924 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov034 | union_oneself, union_msg, union_actor | 21,392 | 21,392 | 0 | 0 | 0 | 0 | **0** | 0 | 5/0/0 | 0 (0) | #82 closed |
| ov035 | event_mapchange, event_season_banner, el_scoreboard | 11,186 | 11,186 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov050 | scrcmd_bsubway, scrcmd_bsubway_table | 6,142 | 6,142 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov051 | scrcmd_pleasure_boat | 384 | 384 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov052 | scrcmd_pokemon_league | 814 | 814 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov053 | scrcmd_palpark | 162 | 162 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov054 | scrcmd_abyssal_ruins | 102 | 102 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov055 | wbt_system, wbt_party, wbt_tool | 6,702 | 6,702 | 0 | 0 | 0 | 0 | **0** | 0 | 5/0/0 | 0 (0) | - |
| ov056 | scrcmd_wbt_entrance | 1,490 | 1,490 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov057 | scrcmd_wbt_stadium | 1,600 | 1,600 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov059 | scrcmd_resort, scrcmd_medalinfo | 9,424 | 9,424 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #84 closed |
| ov060 | scrcmd_resort_shop | 12,638 | 12,638 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov061 | scrcmd_black_tower | 2,404 | 2,404 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov062 | scrcmd_pokewood, pokewood_system, pokewood_setup | 4,152 | 4,152 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov063 | scrcmd_badge_gate | 98 | 98 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov064 | scrcmd_plasma_frigate | 700 | 700 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov065 | scrcmd_pokemon_center, scrcmd_pokemon_center_table | 3,672 | 3,672 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov066 | scrcmd_plugin14 | 916 | 916 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov067 | scrcmd_plugin15 | 162 | 162 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov068 | scrcmd_plugin16 | 348 | 348 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov069 | union_app | 1,590 | 1,590 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov070 | oekaki, oekaki_comm | 10,342 | 10,342 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #85 closed |
| ov071 | fest_mission_sparkle | 946 | 946 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov072 | fest_mission_gimmick | 1,224 | 1,224 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov073 | rival_select | 318 | 318 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov074 | weather_sunny | 348 | 348 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov075 | weather_particles | 4,106 | 4,106 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #86 closed |
| ov076 | weather_rain | 5,528 | 5,528 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #87 closed |
| ov077 | weather_sandstorm | 1,026 | 1,026 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov078 | weather_tint | 820 | 820 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov079 | c_gear_sw | 12,124 | 12,124 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #88 closed |
| ov080 | no_gear | 624 | 624 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov081 | wbt_subscreen; wbt_subscreen | 834 | 834 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 16 (0) | - |
| ov082 | union_subdisp | 4,494 | 4,494 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #89 closed |
| ov083 | beacon_view_common | 1,462 | 1,462 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov084 | beacon_view_sub, beacon_view_setup, beacon_view | 15,488 | 15,488 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | - |
| ov085 | fmission_view, fmission_view_setup, fmission_log_accessor_fv | 10,928 | 10,928 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov086 | dowsing | 3,652 | 3,652 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #90 closed |
| ov087 | report | 3,158 | 3,158 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov088 | c_gear_power_onoff | 1,834 | 1,834 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov089 | pleasure_boat_rooms | 980 | 980 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #91 closed |
| ov090 | gimmick_state | 100 | 100 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov091 | gym_insect | 2,878 | 2,878 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov092 | gym_elec | 4,818 | 4,818 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov093 | gym_nacrene | 64 | 64 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov094 | gym_mistralton | 5,550 | 5,550 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #92 closed |
| ov095 | gym_striaton | 56 | 56 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov096 | gym_driftveil | 4,062 | 4,062 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #93 closed |
| ov097 | gym_driftveil_lift | 1,166 | 1,166 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov098 | gym_icirrus | 1,136 | 1,136 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov099 | gym_opelucid | 3,874 | 3,874 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #94 closed |
| ov100 | gym_virbank | 1,032 | 1,032 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #95 closed |
| ov101 | gym_humilau | 6,044 | 6,044 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #96 closed |
| ov103 | gimmick_badge_gate | 2,830 | 2,830 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #98 closed |
| ov104 | field_gimmick_gate, gimmick_obj_elboard | 5,882 | 5,882 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #99 closed |
| ov105 | gimmick_league_statue | 420 | 420 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov106 | gimmick_league_lift | 458 | 458 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov107 | field_gimmick_d06 | 1,734 | 1,734 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov108 | field_gimmick_bsubway | 778 | 778 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov109 | gimmick_skyarrow_bridge | 1,518 | 1,518 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov110 | gimmick_tubeline_bridge | 642 | 642 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov111 | gimmick_marine_tube | 1,174 | 1,174 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov112 | gimmick_aspertia_city | 722 | 722 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov113 | gimmick_humilau | 852 | 852 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov114 | gimmick_underground_ruins | 622 | 622 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov115 | gimmick_league_center | 588 | 588 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov116 | gimmick_giant_chasm_6 | 1,336 | 1,336 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov117 | gimmick_plasma_frigate | 4,182 | 4,182 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #100 closed |
| ov118 | gimmick_plasma_frigate_deck | 276 | 276 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov119 | gimmick_plasma_frigate_lights | 972 | 972 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov120 | gimmick_league_champion | 1,762 | 1,762 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov121 | castelia_rush | 3,050 | 3,050 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #101 closed |
| ov122 | gimmick_league_shauntal | 3,136 | 3,136 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov123 | gimmick_league_grimsley | 1,230 | 1,230 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #102 closed |
| ov124 | gimmick_league_marshal | 2,010 | 2,010 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov125 | gimmick_league_caitlin | 1,440 | 1,440 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov126 | underwater_effect | 442 | 442 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov127 | infinity_dungeon, infinity_dungeon_gimmick, infinity_dungeon_rooms | 13,414 | 13,414 | 0 | 0 | 0 | 0 | **0** | 0 | 5/0/0 | 0 (0) | #103 closed |
| ov128 | gimmick_passing_vehicles | 870 | 870 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #104 closed |
| ov129 | sp_poke_gimmick, sp_poke_event | 4,148 | 4,148 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #105 closed |
| ov130 | gimmick_funfest_levels | 1,272 | 1,272 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov131 | rail_slipdown | 1,714 | 1,714 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov132 | gimmick_dragonspiral_tower_9 | 1,342 | 1,342 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov133 | gimmick_nuvema | 558 | 558 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov134 | wbt_stadium_gimmick, wbt_battle | 7,388 | 7,388 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #106 closed |
| ov135 | wbt_makematch | 3,944 | 3,944 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #107 closed |
| ov136 | gimmick_pokewood_poster | 722 | 722 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #108 closed |
| ov137 | resort_sys, resort_field, resort_people | 26,020 | 26,020 | 0 | 0 | 0 | 0 | **0** | 0 | 5/0/0 | 0 (0) | #109 closed |
| ov140 | config_panel | 8,088 | 8,088 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #111 closed |
| ov141 | bsubway_records | 4,046 | 4,046 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov142 | itemmenu, itemmenu_disp, bag_item | 28,664 | 28,664 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #112 closed |
| ov143 | monolith_mission_select, monolith_power_select, monolith_tool | 25,898 | 25,898 | 0 | 0 | 0 | 0 | **0** | 0 | 9/0/0 | 0 (0) | #113 closed |
| ov144 | townmap, townmap_grh, townmap_data | 10,382 | 10,382 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #114 closed |
| ov145 | medalbox, medalbox_data_sys | 15,914 | 15,914 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #115 closed |
| ov146 | event_encounter_cutin | 624 | 624 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov147 | battle_cutin_db | 16 | 16 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov148 | encounter_effect_capture | 914 | 914 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov149 | encounter_effect_model | 650 | 650 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov150 | encounter_effect_grid | 1,080 | 1,080 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #116 closed |
| ov151 | encounter_effect_ripple | 1,424 | 1,424 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #117 closed |
| ov152 | encounter_effect_cells | 236 | 236 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov153 | encounter_effect_columns | 224 | 224 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov154 | encounter_effect_spread | 640 | 640 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov155 | fld_faceup | 1,638 | 1,638 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov156 | field_pass_power | 1,168 | 1,168 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov157 | event_fest_mission | 2,168 | 2,168 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov162 | startmenu, title, game_start | 17,822 | 17,822 | 0 | 0 | 0 | 0 | **0** | 0 | 6/0/0 | 0 (0) | - |
| ov164 | net_sync | 290 | 290 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov166 | event_battle_return | 1,992 | 1,992 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #119 closed |
| ov169 | battle (VRAM-resident): scene, side/pos/condition handlers; btlv_scd, condition_effects, side_handlers | 13,750 | 13,750 | 0 | 0 | 0 | 0 | **0** | 0 | 11/0/0 | 0 (0) | #122 closed |
| ov170 | tr_ai | 9,736 | 9,736 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov171 | vs_multi_list | 4,212 | 4,212 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov172 | pdc_main, pdc_msg, pdc | 3,180 | 3,180 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #123 closed |
| ov173 | wifi_p2pmatch, wifi_p2pmatchroom, wifi_status | 37,634 | 37,634 | 0 | 0 | 0 | 0 | **0** | 0 | 5/0/0 | 0 (0) | #124 closed |
| ov174 | ircbattlematch | 8,730 | 8,730 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #125 closed |
| ov175 | compatible_irc_sys, irc_compatible, irc_appbar | 4,818 | 4,818 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #126 closed |
| ov176 | irc_compat_menu | 5,438 | 5,438 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #127 closed |
| ov177 | irc_sync_check | 7,312 | 7,312 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #128 closed |
| ov178 | irc_rhythm_check | 6,570 | 6,570 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov179 | irc_compat_result | 5,130 | 5,130 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #129 closed |
| ov180 | irc_ranking | 7,944 | 7,944 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #130 closed |
| ov184 | error_window_screen | 364 | 364 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov185 | pms_input, pms_select, pms_input_view | 33,218 | 33,218 | 0 | 0 | 0 | 0 | **0** | 0 | 12/0/0 | 0 (0) | - |
| ov186 | trainer_card, badge_view, trcard_bmp | 22,340 | 22,340 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #133 closed |
| ov187 | medal_info_viewer, medal_info_beacon | 4,886 | 4,886 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov190 | wifilogin_disp, wifilogin, wifilogin_message | 8,450 | 8,450 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #135 closed |
| ov191 | wifi_bsubway, wifi_bsubway_graphic | 7,472 | 7,472 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov192 | pokemontrade_demo_common_irc, pokemontrade_ircdemo | 3,998 | 3,998 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov193 | pokemontrade_demo_middle, pokemontrade_demo_down, pokemontrade_demo_up | 14,910 | 14,910 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #136 closed |
| ov195 | gtsnego_proc, gtsnego_disp, gtsnego_message | 18,562 | 18,562 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #138 closed |
| ov196 | net_common_download | 2,246 | 2,246 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov198 | comm_tvt_field | 5,300 | 5,300 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #140 closed |
| ov199 | gsync_state, gsync_menu, gsync_disp | 26,068 | 26,068 | 0 | 0 | 0 | 0 | **0** | 0 | 8/0/0 | 0 (0) | #141 closed |
| ov200 | cg_help | 2,210 | 2,210 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov203 | wf2dmap_obj, wifi_2dchar, wf2dmap_scrdraw | 8,294 | 8,294 | 0 | 0 | 0 | 0 | **0** | 0 | 8/0/0 | 0 (0) | - |
| ov204 | wifi_note | 17,162 | 17,162 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #143 closed |
| ov205 | codein_pv, codein_disp, codein | 8,262 | 8,262 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov207 | p_sta_skill, p_sta_sys, p_sta_sub | 32,276 | 32,276 | 0 | 0 | 0 | 0 | **0** | 0 | 8/0/0 | 0 (0) | #144 closed |
| ov208 | dup_fitting, dup_fitting_item | 17,098 | 17,098 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #145 closed |
| ov209 | sta_acting, script_command, musical_mcss | 35,262 | 35,262 | 0 | 0 | 0 | 0 | **0** | 0 | 16/0/0 | 0 (0) | #146 closed |
| ov210 | musical_system, mus_item_data | 1,270 | 1,270 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #147 closed |
| ov211 | mus_comm_func | 5,124 | 5,124 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #148 closed |
| ov213 | battle_select | 1,070 | 1,070 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov215 | mailsys, mb_seq, mb_main | 14,846 | 14,846 | 0 | 0 | 0 | 0 | **0** | 0 | 9/0/0 | 0 (0) | #150 closed |
| ov216 | guru2_main, guru2_receipt, guru2_comm | 20,816 | 20,816 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #151 closed |
| ov253 | mictest | 3,314 | 3,314 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #152 closed |
| ov255 | box2_seq, box2_main, box_search | 112,434 | 112,434 | 0 | 0 | 0 | 0 | **0** | 0 | 9/0/0 | 0 (0) | #153 closed |
| ov256 | dpc_seq, dpc_obj, dpc_bmp | 8,664 | 8,664 | 0 | 0 | 0 | 0 | **0** | 0 | 6/0/0 | 0 (0) | #154 closed |
| ov258 | waza_oshie_main | 8,384 | 8,384 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov259 | local_tvt_util, local_tvt_callselect, local_tvt_chara | 27,156 | 27,156 | 0 | 0 | 0 | 0 | **0** | 0 | 16/0/0 | 0 (0) | #156 closed |
| ov261 | livebattlematch_flow, battle_championship_flow, digitalcardcheck | 15,502 | 15,502 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #158 closed |
| ov262 | wifibattlematch_view, wifibattlematch_util, wifibattlematch_graphic | 14,184 | 14,184 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #159 closed |
| ov263 | wifibattlematch_subproc | 1,768 | 1,768 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov264 | cdemo_control, cdemo_main, cdemo_script | 1,990 | 1,990 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov265 | ddemo_obj, ddemo_seq, ddemo_main | 7,928 | 7,928 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | - |
| ov266 | staff_roll_main | 5,766 | 5,766 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #160 closed |
| ov267 | the_end_graphic, the_end | 1,500 | 1,500 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov268 | br_record_proc, br_bvrank_proc, br_btlsubway_proc | 32,588 | 32,588 | 0 | 0 | 0 | 0 | **0** | 0 | 14/0/0 | 0 (0) | #161 closed |
| ov269 | br_musical_send_proc | 1,432 | 1,432 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov270 | br_musical_look_proc | 3,100 | 3,100 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov271 | br_util, br_btn, br_res | 29,138 | 29,138 | 0 | 0 | 0 | 0 | **0** | 0 | 11/0/0 | 0 (0) | #162 closed |
| ov272 | br_main | 820 | 820 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov273 | battle_rec_tool | 3,520 | 3,520 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov279 | new_game | 146 | 146 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov281 | prof_word | 544 | 544 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov284 | shinka_demo_view, shinka_demo, shinka_demo_effect | 19,540 | 19,540 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #164 closed |
| ov285 | b_app_tool | 428 | 428 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov286 | b_bag_main, b_bag_bmp, b_bag_obj | 10,494 | 10,494 | 0 | 0 | 0 | 0 | **0** | 0 | 6/0/0 | 0 (0) | - |
| ov287 | b_plist_bmp, b_plist_main, b_plist_obj | 26,940 | 26,940 | 0 | 0 | 0 | 0 | **0** | 0 | 5/0/0 | 0 (0) | #165 closed |
| ov288 | b_scenario_main, b_scenario_graphic | 10,574 | 10,574 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #166 closed |
| ov289 | b_choice_main | 3,410 | 3,410 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #167 closed |
| ov290 | wifibattlematch_sys, wifibattlematch_battle | 5,882 | 5,882 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #168 closed |
| ov293 | demo3d_cmd, demo3d_engine, demo3d_graphic | 8,732 | 8,732 | 0 | 0 | 0 | 0 | **0** | 0 | 8/0/0 | 0 (0) | #169 closed |
| ov294 | intro_cmd, intro_graphic, intro_msg | 8,562 | 8,562 | 0 | 0 | 0 | 0 | **0** | 0 | 8/0/0 | 0 (0) | - |
| ov295 | game_clear_save, battle_lose_message | 2,170 | 2,170 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #170 closed |
| ov296 | zukan_info | 9,072 | 9,072 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #171 closed |
| ov297 | zukan_nickname, zukan_toroku, zukan_toroku_graphic | 3,016 | 3,016 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov298 | zukan_detail_form, zukan_detail_map, zukan_detail_voice | 49,868 | 49,868 | 0 | 0 | 0 | 0 | **0** | 0 | 12/0/0 | 0 (0) | #172 closed |
| ov299 | zknlist_obj, zknlist_main, zknlist_seq | 10,726 | 10,726 | 0 | 0 | 0 | 0 | **0** | 0 | 6/0/0 | 0 (0) | #173 closed |
| ov300 | zknsearch_seq, zknsearch_obj, zknsearch_main | 17,524 | 17,524 | 0 | 0 | 0 | 0 | **0** | 0 | 7/0/0 | 0 (0) | - |
| ov301 | zukan_open | 1,652 | 1,652 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | #174 closed |
| ov302 | zukan_common, zukan, zukan_search_engine | 5,352 | 5,352 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #175 closed |
| ov303 | zukan_habitat, zukan_habitat_graphic | 14,062 | 14,062 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #176 closed |
| ov304 | zukan_area_list_view, zukan_area_list | 8,638 | 8,638 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #177 closed |
| ov305 | comm_btl_demo, comm_btl_demo_graphic | 6,302 | 6,302 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov306 | btl_rec_sel, btl_rec_sel_graphic | 7,822 | 7,822 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov307 | egg_demo, egg_demo_view, egg_demo_graphic | 6,444 | 6,444 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov308 | beacon_detail_sub, beacon_detail, beacon_detail_gra | 7,746 | 7,746 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov309 | fmission_app, fmission_app_draw, fmission_log_accessor_fapp | 12,962 | 12,962 | 0 | 0 | 0 | 0 | **0** | 0 | 4/0/0 | 0 (0) | #178 closed |
| ov310 | research_graph, research_list, research_top | 38,936 | 38,936 | 0 | 0 | 0 | 0 | **0** | 0 | 13/0/0 | 0 (0) | #179 closed |
| ov311 | un_select, un_select_graphic | 5,472 | 5,472 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #180 closed |
| ov312 | geonet | 8,792 | 8,792 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov313 | th_award, th_award_graphic | 3,408 | 3,408 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #181 closed |
| ov314 | chihou_zukan_award, chihou_zukan_award_graphic | 2,932 | 2,932 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #182 closed |
| ov315 | zenkoku_zukan_award, zenkoku_zukan_award_graphic | 3,084 | 3,084 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #183 closed |
| ov316 | psel, psel_graphic | 11,214 | 11,214 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #184 closed |
| ov317 | subway_map_graphic, subway_map | 1,594 | 1,594 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov318 | marine_tube_board, marine_tube_board_graphic, marine_tube_board_message | 1,726 | 1,726 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | - |
| ov319 | manual_list, manual_touchbar, manual_top | 10,264 | 10,264 | 0 | 0 | 0 | 0 | **0** | 0 | 10/0/0 | 0 (0) | - |
| ov320 | wbt_list, wbt_list_graphic | 7,944 | 7,944 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #185 closed |
| ov321 | pkwd_count, pkwd_count_graphic | 4,030 | 4,030 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #186 closed |
| ov322 | pkwd_book, pkwd_book_graphic | 13,882 | 13,882 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #187 closed |
| ov323 | pkwd_selectscreen, pkwd_selectscreen_graphic | 6,748 | 6,748 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov324 | pkwd_briefing | 8,518 | 8,518 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov325 | pkwd_record_monitor, pkwd_record_monitor_graphic | 9,248 | 9,248 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | #188 closed |
| ov326 | t_download, t_download_util, win_record | 24,144 | 24,144 | 0 | 0 | 0 | 0 | **0** | 0 | 6/0/0 | 0 (0) | #189 closed |
| ov327 | wbt_download_net_util, wbt_download_sys, wbt_download_net_graphic | 4,912 | 4,912 | 0 | 0 | 0 | 0 | **0** | 0 | 3/0/0 | 0 (0) | #190 closed |
| ov328 | gpower_app, gpower_app_graphic | 7,536 | 7,536 | 0 | 0 | 0 | 0 | **0** | 0 | 2/0/0 | 0 (0) | - |
| ov330 | event_pdc_return | 818 | 818 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |
| ov331 | save_outside | 1,156 | 1,156 | 0 | 0 | 0 | 0 | **0** | 0 | 1/0/0 | 0 (0) | - |

236 overlays with code are fully complete (all C, linked); 260 overlays have code in total.
