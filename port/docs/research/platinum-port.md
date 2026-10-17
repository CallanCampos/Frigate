# cybervisi0n/pokeplatinum `pc_port`: game-side analysis for a BW2 native port

Scope: the game side of the Platinum PC port (src/, include/, lib/ in the game repo, build, CI, releases,
issues). libntr itself is covered by a separate report; it is only touched here where the game depends on it.
All evidence is from the blobless clone at `scratchpad/research/pokeplatinum-pc` (tip `origin/pc_port`
`35067868f`, 2026-10-10), `gh` read-only queries (2026-10-16), and a read of the libntr clone at
`scratchpad/research/libntr` (`1555d91`). Nothing was built or run.

Diff baseline throughout: merge base with pret `c248fb3f8` (2026-09-20) .. `origin/pc_port`.

---

## 0. Headline facts

| Fact | Value | Evidence |
| --- | --- | --- |
| Public PC history starts | 2026-06-13, one squashed commit | `2659a4dc8` "Add initial support for building for PC platforms (win64,linux)" on top of pret `410c43191` (2026-06-05); libntr's equivalent squash is `39dc0aa` (2026-06-13, author "null") |
| Commits on top of pret | 111 (100 non-merge, 11 merges); 7 behind pret/main | `git rev-list --left-right --count pret/main...origin/pc_port` → `7 111` |
| Contributors (game repo) | cybervisi0n 107, emufa 2 (README), Daniel Olivas 1 (PR #32), "null" 1 | `git shortlog -sn c248fb3f8..origin/pc_port` |
| Total diff vs pret | 339 files, +5964/−622 | `git diff --shortstat c248fb3f8 origin/pc_port` |
| Game code proper (src/, include/ minus `port/`) | 239 files, +3365/−434 | same, path-limited |
| New port-only code | 21 files, +1215 (`src/port/`, `include/port/`: ImGui, config, cheats) | |
| lib/ (SPL, crypto, gds, ppwlobby) | 19 files, +179/−9 | |
| Build/CI/tools | 52 files, +1060/−171 | |
| Guard sites | 697 opening `#if*` directives on `SDK_*` macros in 226 files (189 in src/ of ~1019 C files) | `git grep -E '^[[:space:]]*#[[:space:]]*(if|ifdef|ifndef|elif).*SDK_(BUILD|PORT)'` |
| DS ROM | still builds from the same tree (`make`), but the SHA1 check is no longer run by default or in CI, and a typo'd guard has changed DS code since 2026-07-09 | §1 |
| Platforms | linux, win64 (MSYS2/UCRT64 gcc), nx (devkitA64); **no macOS target** | `meson.options` `build_target` combo |
| Releases | v0.0.1 (2026-07-03) … v0.1.12 (2026-10-10), 15 tags; binaries only, the user supplies the ROM | `gh release list` |
| Testing | none automated: CI only compiles; QA is play-testing + user bug reports | §9 |

---

## 1. Relationship to pret

* **Fork point / merge base.** `pc_port` branched from pret at `410c43191` (2026-06-05, "Document Veilstone Gym
  persistent features and finish up Overlay 8 (#1182)"). Current merge base: `c248fb3f8` (2026-09-20, "Fix build
  with spaces in repository path (#1290)"). `origin/main` of the fork is a plain mirror of pret at `c248fb3f8`.
* **Ahead/behind.** 111 ahead, 7 behind (pret's `1dc13dcea` lsfgen change, which regenerates `src/meson.build`, a
  file the port edits, so the next merge will conflict there).
* **How they merge upstream.** Plain `git merge pret/main` into `pc_port` (first-parent merges `21c807209` 06-15,
  `a02a39132` 07-05, `6191f1e40` 08-25, `47c28d531` 08-29, `fa587f127` 09-15, `70e7cf984` 09-18, `3d48ed3c2` 09-20).
  101 pret commits were integrated since the fork point. A large one went through a feature branch and PR:
  `features/pret-merge` (PR #28, merged 2026-08-23 as `670772055`): `77fa0c087` merged 36 pret commits including pret's
  SDK subproject update (#1263) and nitrosfx/nitrogfx tool changes (7379 files touched by the merge), followed by
  `9cc01bd93` "integrate new libntr merge" (meson.build only), `e35aa5f69` "Fix linux build", `97f2dc57e` "libntr
  build fixes". The port's own changes are kept as `#ifdef` blocks inside pret's files, so merges conflict wherever
  pret edits a guarded region. Release notes of v0.1.4–v0.1.6 simply list pret's merged PR titles.
* **SDK swap.** pret's `subprojects/NitroSDK.wrap`/`NitroSystem.wrap`/`NitroWiFi.wrap`/`NitroDWC.wrap` (pinned
  `ntrtwl/*` commits, e.g. NitroSDK `eaae40f1`) are deleted and replaced by `libntr.wrap`, `libntrsystem.wrap`,
  `libntrwifi.wrap`, `libntrdwc.wrap` (cybervisi0n forks) that `provide` the same dependency names
  (`NitroSDK`, `NitroSystem`, ...), plus `libvct` and `tracy` forks. **All wraps track `revision = main` (unpinned)**,
  so a checkout does not reproducibly identify the SDK it builds with (v0.1.8 added the libntr git hash to the ImGui
  window to compensate).
* **Does the DS ROM still build and match?**
  * It still builds: `make` (target `arm`, MWCC via metroskrew, libntr compiled for ARM) and CI `build_arm.yml` runs
    `./drun.sh make`.
  * Matching is **not enforced**: the Makefile changed `all: release check` to `all: release` (pret's own comment says
    modders may delete `check`), and pret's `build.yml` (which ran `make check`) and `pr-lint.yml` (clang-format)
    were deleted. `make check` (meson tests comparing sbins/filesystem/ROM SHA1s) still exists but nothing runs it.
  * Concrete evidence it no longer matches: `src/overlay005/ov5_021D5EB8.c:4656` has `#ifdef SDK_BUILD_ARMs` (typo),
    added in `4ee56f449` (2026-07-09, "Fix crash when entering snowpoint city"). On the DS build the macro is
    undefined, so the PC branch `(s64 *)v1[6]` replaces pret's `*((s32 *)v1[6]) == 1`: a 64-bit load on the DS. That
    is different code (and a behavioral change on DS), and it has survived three months, i.e. nobody has run
    `make check` on this branch since.
  * How they keep it *building*: the original line stays under `#ifdef SDK_BUILD_ARM`, the port line under `#else`
    (485 of the 697 guard sites are `#ifdef SDK_BUILD_ARM`); a few changes are unguarded but codegen-neutral
    (`NULL`→`0`, `uPtr`, which is `u32` on ARM, a named inner struct in `Poffin`).
  * The goal is stated in README: "Facilitate modding by allowing both a PC port and DS ROM to be compiled and
    debugged from the same source tree". It is a modding goal, not a matching guarantee.

---

## 2. Categories of change to game code

Guard macros (defined by libntr's meson, `libntr/meson.build:110-143`, and propagated through the dependency):
`SDK_BUILD_ARM` (DS build), `SDK_PORT` (every PC build) plus `SDK_BUILD_LINUX` / `SDK_BUILD_WIN64` / `SDK_BUILD_NX`.

| Directive | Count |
| --- | --- |
| `#ifdef SDK_BUILD_ARM` | 485 |
| `#ifdef SDK_PORT` | 146 |
| `#ifndef SDK_BUILD_ARM` | 31 |
| `#ifndef SDK_BUILD_NX` | 28 (all in `src/overlay066/`, PPW lobby disabled on Switch) |
| `#ifdef SDK_BUILD_LINUX` / `WIN64` | 4 / 1 |
| `#if defined(SDK_BUILD_ARM) \|\| defined(__ASSEMBLER__)` | 1 |
| `#ifdef SDK_BUILD_ARMs` (typo) | 1 |

Two macros mean the same thing (`SDK_PORT` vs `!SDK_BUILD_ARM`) and are used interchangeably, often duplicating the
whole statement in both branches. The added code is not clang-formatted (`if(`, indented `#ifdef`).

Heuristic classification of every changed hunk in src/, include/, lib/ (excluding `port/` and meson files); script
`scratchpad/platinum-scripts/classify.py`:

| Category | Hunks | Changed lines | Files |
| --- | --- | --- | --- |
| Pointer width (u32→u64/`uPtr`/s64) | 375 | 1957 | 110 |
| Compiler/ABI/layout/misc (static, inline, packed, prototypes, includes) | 78 | 623 | 46 |
| Other (mostly missing prototypes, Poketch static init, sentinels, context sizes) | 124 | 609 | 78 |
| Compiler strictness: `NULL`→`0` | 72 | 156 | 26 |
| Heap/buffer size ×2 | 29 | 155 | 16 |
| Enhancement/debug hooks | 21 | 140 | 10 |
| Crash guards (div0/NULL/underflow) | 17 | 134 | 15 |
| Stubs / platform-disabled | 22 | 99 | 4 |

### 2.1 64-bit pointer width (the dominant category, ~55% of changed lines)

Pret's C stores pointers in 32-bit integers in many places, which MWCC accepts and which matches the ROM.

* **Integer-typed "index" fields that also carry function pointers.** `StringList` holds `union { u32 index; void
  *callbackFunction; }` (`include/string_list.h`); list/menu results return that value. Fix: `typedef uPtr
  list_index_t;` and `typedef uPtr menu_selection_t;` (`include/string_list.h`, `include/menu.h`), `CursorCallback`/
  `PrintCallback` take `u64 index` on PC (`include/list_menu.h`), `ListMenu_GetAttribute` returns u64. "Fix 64-bit
  menus" `bf9932d94` (2026-07-06) touched 67 files/+1163 lines; follow-ups: flower shop `990697b93`, options menu in
  `4e6ad696c`, bag close via touch screen (issue #10, PR #32 `013d28a01` by a contributor, converting `#ifdef` pairs
  to the typedefs).
* **Function-pointer tables typed as integers.** `sItemActionFuncs[]` became `static const u64 ... = (u64)ItemActionFunc_Use`
  in `src/applications/bag/main.c`; `CommManager_IsConnectingUnderground` compares `(u64)sCommMan->task` against a
  `u64` array (`src/comm_manager.c`).
* **Pointer arithmetic through u32.** `ResourceHeap_New`: `resHeap->items = (void *)((u64)resHeap + sizeof(ResourceHeap))`
  (`src/overlay005/resource_heap.c`); `ParticleSystem_AllocMemory` alignment (`src/particle_system.c`); 32-byte
  alignment in `src/unk_02038FFC.c`; `ALIGN_PTR` macro added to `include/pch/global_pch.h`; VRAM-offset copies
  `(void *)((u64)objCharPtr + imageProxy->vramLocation.baseAddrOfVram[...])` (`src/battle/battle_subscreen.c`,
  `src/overlay099/ov99_021D4134.c`); save footer addresses (`src/savedata.c`, `SaveBlockFooter_Ptr(..., u64
  bodyAddress, ...)`); Union Room trade `uPtr base = (uPtr)party` (`35067868f`, the newest commit).
* **Hardware register addresses passed as integers.** On PC libntr's registers are host memory, so
  `ScreenShakeEffect_Start(..., (u64)REG_BG0HOFS_ADDR, ...)` (`src/overlay005/encounter_effect_core.c`, 12 sites),
  `CustomBgScrollContext_New(uPtr offsetReg, ...)` (Extrasensory crash, `a52d4f65b`), timer reads in `src/timer.c`.
* **Generic s32 work arrays holding pointers.** Weather tasks use `s32 *v1 = (s32 *)v0->unk_08` and store pointers in
  `v1[6]`; the port widens to `s64` arrays (`src/overlay005/ov5_021D5EB8.c`, Snowpoint `4ee56f449`, volcanic ash
  `6d74e552a`/`5665d3b35`).
* **Opaque byte-sized contexts that contain pointers** must grow: `SPRITE_ANIM_SIZE` 29→34
  (`include/constants/graphics.h`), `OVERWORLD_ANIM_MANAGER_FUNCS_CONTEXT_SIZE` 160→320
  (`include/overworld_anim_manager.h`).
* **Sentinels written as 32-bit literals.** `0xffffffff`/`0xfffffffe` compared with menu results became
  `MENU_NOTHING_CHOSEN`/`MENU_CANCEL`, redefined as `(u64)(-1)`/`(u64)(-2)` on PC (`include/constants/menu.h`,
  `src/overlay065/ov65_0222DCE0.c`, `src/applications/party_menu/main.c`).
* **Evolution of the fix style.** June–July fixes duplicate whole statements under `#ifdef SDK_PORT (u64) #else (u32)`.
  From v0.1.6 (2026-09-21) libntr has `uPtr` (`include/nitro/types.h:71/74`: `u32` on ARM, `u64` on PC), and later
  fixes use it unguarded (`a52d4f65b`, `35067868f`, PR #32). Only 6 `uPtr` uses exist so far versus 119 `(u64)` casts
  and ~418 added lines mentioning `u64`. They never used `uintptr_t`.

### 2.2 Struct layout, packing and alignment

* `typedef struct __attribute__((__packed__)) Pokemon` on non-ARM (`include/struct_defs/pokemon.h:154-158`), added in
  `4e6ad696c` "Fix wifi mystery gift" (2026-07-11). v0.1.0's notes then say "Save data from 0.0.2 and earlier is not
  compatible!". `src/applications/pc_boxes/box_app_manager.c` steps a `BoxPokemon *` by a byte size on PC.
* Save layout differs from DS anyway: the third-party fork `elyro1/pc_single_screen_mod` added
  `tools/convert_ds_save.py` (`cfc5b4fed`, 2026-10-15), whose table shows save-table entries 0, 8 and 37 differ
  (DS 92/480/74184 bytes vs PC 104/488/74192; blocks 0xCF2C/0x121E4 vs 0xCF40/0x121EC). Entry 0 is `SystemData`,
  which starts with `s64 rtcOffset` (`include/system_data.h:19`). That fits MWCC's 4-byte alignment of 64-bit types
  against 8 bytes on x86-64/AArch64, though that cause is my inference. The upstream port has no converter; DS saves
  do not load as-is.
* Heap and arena sizes are doubled rather than measured (§2.5).

### 2.3 Compiler strictness (MWCC accepts, GCC rejects or links differently)

* `NULL` passed or assigned where an integer is expected → `0` (100 removed lines with NULL; e.g. `BattleMessage_Print(...,
  NULL)` → `0`, Poketch task tables, `MOVE_SUBSCRIPT_PTR_NONE = NULL`). GCC 14 makes int-conversion an error.
* "Static after non-static": pret forward-declares tables without `static` and defines them `static`; GCC errors, so
  the port drops `static` on PC (81 `#ifdef SDK_BUILD_ARM` / `static` / `#endif` triples, 25 in
  `src/unk_0206450C.c` alone).
* C99 `inline` in headers (no external definition under GCC) → `static inline` on PC (`include/inlines.h`
  `LCRNG_RandMod`; libntrsystem does the same for `NNS_G2D_INLINE`).
* Missing prototypes / implicit declarations added (`include/battle/battle_controller.h`, `src/applications/
  capsule_menu/*.c`, `src/applications/pokedex/ov21_021D5AEC.c`, `include/player_avatar.h`).
* Empty `()` parameter lists called with arguments (`Sound_Impl_GetCurrentBGM(p)` in `include/sound.h`).
* Tentative definitions in headers (`-fno-common`): `extern` added in `lib/gds/include/gds_internal.h` plus a definition
  in `lib/gds/src/ov61_0222CEDC.c`.
* Anonymous struct member in `Poffin` renamed to `inner` (unguarded, so `poffin->inner.type` in 50+ places in
  `src/poffin.c`).
* Struct assignment of `ApplicationManagerTemplate` replaced by `memcpy` on PC (`src/overlay_manager.c`, marked `//TODO`).
* Build flags hide the rest: PC builds use `buildtype=plain`, `warning_level=0`, `-std=c99 -g`, and win64 adds `-w`
  (all warnings off). No `-Werror` anywhere.

### 2.4 Remaining asm

Pret's game is 100% C (`asm/` holds only `macros/`, 0 `.s` files). The only assembly left:
* `src/battle/trainer_ai/script.s`: trainer AI bytecode as data macros. On PC it is preprocessed with gcc and
  assembled with the host `as` (`meson.build`, `trainer_ai_ss`/`trainer_ai_o`). It ports because jumps are encoded
  relative (`asm/macros/aicmd.inc:9` `.long (\jump-.) / 4 - 1`).
* `lib/crypto/asm/rc4.s`, `rc4_fast.s`: **not ported**. `CRYPTO_RC4Encrypt` is an empty `//TODO` stub on PC
  (`lib/crypto/src/rc4_init.c`).
* Also stubbed: a `std::transform(tolower)` compare in PPW lobby returns 0 (`lib/ppwlobby/include/ilobby/
  ov66_022364B0.h`, "TODO: Not compiling"); PPW lobby calls disabled on NX (`src/overlay066/*`).

### 2.5 Heap / allocator

The game keeps its own DS heap allocator on top of libntr's emulated arenas. Sizes are doubled for 64-bit growth:
* `include/constants/heap.h`: `HEAP_SIZE_SYSTEM/SAVE/DEBUG/APPLICATION` ×2, FIELD1–3 ×2, POKETCH ×2, EGG_HATCH
  0x40000→0x80000, EVOLUTION ×2, MYSTERY_GIFT ×2.
* 13 `Heap_CreateAtEnd(..., HEAP_ID_COMMUNICATION, 0x7080*2)` sites in `src/comm_manager.c`; contest heap 0xa0000×2.
* Particle-system heaps 0x4800→0x4800*2 at 8+ `ParticleSystem_New` sites (`77fef32c9` "Fix particle system allocation
  issues causing heap corruption", Cynthia battle `a6aab6616`, Route 213 `dcb6793f8`).
* `SoundSystem.heapBuffer[SOUND_SYSTEM_HEAP_SIZE*2]` (`include/sound_system.h`); `VBLANK_TASK_MAX` 32→64
  (`src/system.c`).
* libntr: `s_HW_MAIN_MEM[0x800000]` with the comment "PC Port: Doubled the size of HW_MAIN_MEM from 0x400000 to
  0x800000" (`libntr/include/nitro/hw/X86/mmap_global.h:30-37`).

The pattern is reactive: a heap overflows or corrupts, the crash is found by play-testing, and that size is doubled.

### 2.6 Undefined behavior that worked on DS

* **Division by zero** (an ARM software divide returns, x86 traps with SIGFPE): about 11 guarded sites. Exp share
  `BtlCmd_CalcExpGain` (`src/battle/battle_script.c`), health/exp gauges `FillCells`/`CalcGaugeFill`
  (`src/battle/healthbox.c`), `App_PixelCount`, contest (`95380a19f`), SPL alpha/child-emitter math
  (`lib/spl/src/spl_anim.c`, `spl_emit.c`), Thunderbolt (`1b3c3046c`), `src/overlay100/ov100_021D400C.c`.
* **NULL dereference / use-after-free** tolerated on DS: guards in `LoadBgVRAMScr` ("fixes a use-after-free in the
  slot machine", `src/bg_window.c`), `Pokemon_CalcStats(NULL)`, Route 215 `a3392b649`, `FillCells`.
* **Integer underflow in comparisons**: `(int)app->cursorPosition - 1 >= 0` (`src/choose_starter/choose_starter_app.c`,
  `c490127ee`).
* **Resource leak**: `NARC_GetMemberSizeByIndexPair` never closes its file ("Bug: file is never closed!" in pret);
  harmless on DS, it exhausts file descriptors on PC (`446ca04c6`, v0.1.1).
* No uninitialized-read, aliasing or out-of-bounds fixes are visible as such, and there are no `{0}` initializers or
  added memsets. Those bugs are probably still latent and absorbed by the doubled heaps and the disabled asserts.
* **Asserts are off by default**: `ErrorHandling_AssertFail` returns immediately unless `enableAsserts`
  (`src/error_handling.c`), and the default is `FALSE` (`src/port/sim_config_prj.c`). Optionally it can break into the
  debugger (`raise(SIGTRAP)` / `__debugbreak()`).

### 2.7 Endianness

None. All targets are little-endian (cross files declare `endian = 'little'`, NX included). No byte-swapping code.

### 2.8 Overlays

See §3.

### 2.9 Frame timing / vblank

`src/main.c` keeps the DS loop (`OS_WaitIrq(TRUE, OS_IE_V_BLANK)` is implemented by libntr). The DS game runs most
logic at 30 Hz by waiting an extra vblank when `!gSystem.frameCounter`. The port skips that extra wait when
`enable60fps` is set, which is the **default** (`SIM_Config_prj_LoadDefaults`: `enable60fps = TRUE`,
`enable60fpsSpeedFix = TRUE`). "Speed fixes" are hand-placed at 5 sites that halve animation deltas or double durations:
`Simple3D_UpdateAnim` (`src/simple3d.c`), `GetAnimStartFrame` (`src/billboard.c`), `MovementAction_FaceEast_Step0`
(`src/unk_020655F4.c`), two in `src/overlay005/ov5_021F17B8.c`. Everything else runs at double speed: open issues
#5 (intro), #15 (starter Poké Ball). libntr adds fast-forward (TAB/SPACE, v0.1.10).

### 2.10 File loading

The game calls the SDK FS/NARC API unchanged. libntr maps path opens to `fopen` on the extracted ROM tree
(`libntr/libraries/fs/src/fs_file.c:531-556`). Game-side changes: the NARC fd-leak fix and `FS_LoadArchiveTables`
alignment (libntr). See §4.

### 2.11 Save handling

Game side: only pointer arithmetic in `src/savedata.c` changed (u64 addresses, `(void *)startAddress` casts). The
backup device is libntr's (`card_backup.c`, "Improve card backup and FS file operations" `18ea3a0`); the elyro1 script's
comment says "The PC backup backend writes only as far as its last used flash address". Saves are not DS-compatible
(§2.2): GBAtemp's coverage of the release also reports saves "not compatible with emulators".

### 2.12 Performance

* Game C on PC is compiled **without optimization**: `default_options: ['buildtype=plain', 'warning_level=0']` and
  `shared_args = ['-DPOKEPLATINUM_GENERATED_ENUM', '-g']` for non-ARM targets (`meson.build:4-6, 40`). Only libntr's
  simulator is `-O2` (`libntr/libraries/sim/meson.build:102`; release note v0.1.6 "SIM: Enable -O2 optimization").
  The game logic is cheap enough at -O0 on desktop; the NX build is "working, but poor performance" (README).
* Performance work is all in libntr: GPU texture cache (v0.1.10), internal-resolution slider (v0.1.9), Tracy profiler
  integration (`bc2060d5b`, `tracy_enable` option, works on win64/linux/nx).

### 2.13 Enhancements / QoL (game side)

* `include/port/sim_config_prj.h`: `enable60fps`, `enable60fpsSpeedFix`, `enableAsserts`, `breakDebuggerOnGfAssert`,
  cheats `walkThroughWalls` (`src/player_move.c`), `disableRandomEncounters` (`src/overlay006/wild_encounters.c`),
  `runFromTrainerBattles` (`src/battle/battle_controller_player.c`). Persisted in `./sim_config_prj.ini` (ini parser
  from libntr).
* libntr (release notes): fullscreen, Steam Deck sizing, master volume, resolution scale 1×–20×, rebindable input
  (issue #33 says rebinding has no effect), fast-forward.
* Third-party forks show the modding direction: `elyro1/pc_single_screen_mod` (93 commits beyond pc_port, +11.7k lines:
  single-screen/widescreen, NPC streaming, map edits, DS-save converter, battle subscreen) and
  `evoluketion/single-screen`.

### 2.14 ImGui / debug (`features/new_imgui`, merged as PR #34 on 2026-09-28)

* Game registers a project window into libntr's GUI: `SIM::GUI::PrjMain` (`src/port/sim_gui_prj.cpp`) shows the git rev
  and buttons for Cheats, Config, Gen Pokemon, Gen Items, Jump to Map. It is opened with the ` key.
* Hooks into game state are minimal: `DEBUG_SetFieldSystem(fieldSystem)` in `InitFieldSystem` (`src/field_system.c`,
  `src/port/debug_field.c`); map jump is a table of warps (`src/port/gui_mapjump.cpp`); `CharCode_ToAsciiString`
  (`src/port/charcode_convert.c`, 518 lines) converts game strings for display.
* `new_imgui` (`57b8683a9`, `ffafbb4ce`, `c04b4fcf0`) rewrote the C GUIs in C++ (24 files, +430/−310). Only these
  `src/port/*.cpp` files are C++; the game stays C99.

### 2.15 `features/game_opening`, `features/game_opening2`

Pure documentation of the opening cutscene, based on pret main (presumably to upstream to pret and to fix the
double-speed intro, issue #5): `00cf51e7f`/`6ec4ad075` "Document GameOpeningWork struct", `71c9723e9` renames
`ov77_021D25B0.c` → `opening_cutscene.c`, `1a06d1a25` documents the Game Freak logo, `0dc99fc72` names more
functions. 4 commits, 7 files, +635/−626. Not merged into pc_port.

---

## 3. Overlay semantics on PC

* **Every overlay is linked into one executable.** For linux/win64/nx, `meson.build` builds `executable('main',
  sources: [pokeplatinum_c, ...])` from the same `pokeplatinum_c` list the DS build uses with its LCF
  (`platinum.us/main.lsf`). Code is always resident, and overlays that share an address range on DS coexist in the
  native image.
* **Overlay IDs collapse.** Under `SDK_PORT`, libntr defines `FS_OVERLAY_ID(name)` as `(0)`
  (`libntr/include/nitro/fs/overlay.h:19-21`). The game's overlay manager (`src/game_overlay.c`: `Overlay_LoadByID`,
  `CanOverlayBeLoaded`, region bookkeeping) and `RunApplication` in `src/main.c` are **unchanged**. They still call
  `FS_LoadOverlayInfo/FS_LoadOverlay/FS_UnloadOverlay` (libntr `libraries/fs/src/fs_overlay.c`, generic SDK code
  with only `#ifdef SDK_BUILD_ARM` around debugger registration). What that path does at runtime on PC (overlay 0's
  header read every time, assert failures swallowed because asserts are off) could not be confirmed statically.
  The libntr report should settle it. Either way, nothing in the game uses its result to gate code.
* **No .data re-copy and no .bss re-zero for native statics.** I found no code anywhere (game or libntr) that
  snapshots or restores an overlay's initialized data or clears its bss when an overlay "loads". Statics keep their
  values across overlay switches. The port relies on game code initializing its state explicitly (pret's code
  mostly `Heap_Alloc`s and `MI_CpuClear`s its work structs) and fixes divergences case by case when they crash.
* **Static initializers (sinit) are re-implemented by hand.** On DS, `FS_StartOverlay` runs the overlay's
  `sinit_init` table, and each Poketch app overlay's `NitroStaticInit` calls `PoketchSystem_SetAppFunctions(New,
  Exit)`. The port adds a `PoketchAppStaticInit_<App>()` wrapper in each of the 25 app files and a dispatch table
  `PoketchOverlayLoadFunctions[]` called from `PoketchSystem_LoadApp` (`src/applications/poketch/poketch_system.c`,
  `include/applications/poketch/poketch_system.h`; commit `955e454df` "Fix poketch crashes..."). The other
  `NitroStaticInit`s (`src/battle/battle_main.c`, `src/overlay017/ov17_0223F118.c`,
  `src/applications/capsule_menu/placement.c`, `src/dummy/dummy.c`) only load other overlays, so they are left as
  dead code.
* **Symbol clashes.** None needed fixing: pret's names are globally unique (`ovNN_ADDR` or module-prefixed), and
  file-local helpers are `static`. The "static" edits in §2.3 are about tentative declarations, not clashes.
* **Overlays never need to be read from the ROM** for code. The extracted `overlay9/` files are unused for execution.

---

## 4. Data and assets

* **First-launch extraction** (libntr, since v0.1.8 on 2026-10-02; before that users extracted the ROM by hand with
  external tools). `FS_OpenFile` on PC `fopen`s the path relative to the working directory. On failure it shows an SDL
  message box "ROM Extraction Required", then `SIM_ShowRomExtractionDialog()` (`libntr/libraries/sim/src/
  sim_RomExtractor.cpp`, 25 lines) runs a file picker and `SIM_NDSExtract()` (vendored "fireflower" extractor,
  `libraries/sim/src/fireflower/nds-extract.cpp`), then retries recursively
  (`libntr/libraries/fs/src/fs_file.c:543-556`, with a comment that repeated wrong ROMs will eventually crash). The
  ROM's game code is not checked (no CPUE/SHA1 validation in the extractor; `card_game_code=CPUE`/`card_maker_code=01`
  are build options, `8470ae4db`). v0.1.11: "Simulator application now changes directory to the correct directory
  with the .exe/AppImage" (the extraction lands next to the executable).
* **Read at runtime from the user's ROM (release builds):** the whole NitroFS: NARCs (`msgdata/*.narc`,
  `arc`, `battle`, `contest`, ...), sound data, and the overlay table and files (extracted though unused for code).
  README tells users to replace `msgdata/msg.narc`, `msgdata/pl_msg.narc` and `msgdata/scenario/scr_msg.narc` from
  another language's ROM to change text language.
* **Shipped with the binary:** `header.bin` (352-byte CPUE01 ROM header, `res/pc_port/header.bin`, which libntr uses as
  `HW_ROM_HEADER_BUF`; its OVT9 field is offset 0x106400 / length 0xF40) and `firmware.bin` (`res/pc_port/dwc_nv.bin`,
  "does not contain any real DS firmware, only an offset address and enough space to keep the WiFi config").
  Release zips: linux `main` + `pokeplatinum.AppImage` (68 MB); win64 `main.exe` + SDL2/enet/gcc/stdc++/winpthread
  DLLs (40 MB). Both include debug info.
* **Generated at build time (developer builds):** everything pret's asset pipeline builds from `res/` (NARCs, scripts,
  messages) plus compiled-in C tables and generated headers (`naix_headers`, `c_consts_generators`, `embed_headers`).
  `tools/scripts/copy_pc_files.sh` copies the built NitroFS files listed in `filesys_csv` into
  `build_<target>/pokeplatinum/` next to `main`, so a dev build runs on repo-built (moddable) data without a ROM.
  README: "do not publish any releases containing the compiled assets alongside PC port binaries".
* **NARC/resource files that contain pointers.**
  * Game-side formats are mostly pointer-free or offset-based. SPL `.spa` resources are parsed into separately
    allocated pointer structs (`lib/spl/src/spl_manager.c:100-203`: `res->header = (SPLResourceHeader *)((u8 *)spa +
    offset)`), which ports unchanged except for bigger heaps. Trainer AI uses relative offsets.
  * NitroSystem formats that relocate in place (G2D palette/character/cell data with `void *pRawData` overwritten
    from a 32-bit offset) are handled in **libntrsystem**: a `WIN_NNSG2dPaletteData` mirror struct with `u32
    pRawData` describes the file layout (`include/nnsys/g2d/fmt/g2d_Character_data.h:97-136`), and
    `NNS_G2dGetUnpackedPaletteData` on PC copies the raw palette into a **static ring of 64 × 1024-byte buffers** and
    stores the 8-byte pointer (`libraries/g2d/src/load/g2d_NCL_load.c:21-30, 75-89`, comment "PCPORT_TODO: Memory
    Leaks!"). The ring is reused after 64 loads, and the copy size is the block size, not checked against 1024. That
    is a plausible source of the palette/transparency bugs still open.

---

## 5. Build system

* **Meson + Makefile.** `meson.options`: `build_target` combo `arm|win64|linux|nx` (default `arm`), `tracy_enable`,
  `skip_anim_scripts` (used in Windows CI), plus pret's `gdb_debugging`, `revision`, `logging_enabled`. Make targets:
  `make` (DS ROM), `make linux`, `make win64`, `make nx`, each in its own build dir (`build_linux`, ...), with cross
  files `meson/cross_pcport_linux.ini`, `cross_pcport_win64.ini` (gcc/g++/ar, host `windows`/`linux` x86_64),
  `cross_nx.ini` (devkitA64 `aarch64-none-elf-gcc`, declared as x86_64 "Workaround for a meson issue").
* **Compilers.** DS: MWCC through metroskrew (`make skrew`), as in pret. PC: host GCC (Linux), MSYS2 UCRT64 GCC
  (Windows, `Install_MSys2.ps1` creates a portable MSYS2 inside the repo), devkitA64 (NX). No Clang/MSVC/Apple
  toolchain. C99 for game code, C++20 for libntr's simulator and ImGui.
* **Warnings.** None as errors. `warning_level=0`, win64 adds `-w`.
* **Dependencies.** SDL2, enet (multiplayer), OpenGL via glad and ImGui (in libntr), yyjson/rapidjson/metang
  (pret tools), nasm, flex, bison, libpng, arm-none-eabi-gcc (still required by pret's pipeline), python3; Docker image
  `tools/docker/Dockerfile` (Debian 13, also installs wine and devkitPro), wrapped by `drun.sh` (also `meson.sh`).
* **CI** (`.github/workflows/`): `build_arm.yml` (`./drun.sh make`, no `check`), `build_linux.yml` (`make linux`,
  package, upload `release_linux`), `build_win.yml` (MSYS2 `make win64`, package, upload `release_win64`). All run on
  push to `pc_port` and PRs. Pret's `build.yml` (`make check` + xMAP) and `pr-lint.yml` were deleted.
* **Packaging.** `tools/scripts/copy_pc_files.sh` (copies the filesystem, `header.bin`, `firmware.bin`, DLLs on MSYS,
  runs linuxdeploy to make an AppImage, downloading linuxdeploy at build time, or `elf2nro` for NX),
  `package_artifacts_{linux,win64}.sh` (zip without assets). Releases are uploaded by hand from CI artifacts.

---

## 6. History

* **Before 2026-06-13: private.** Both repos open with a squash ("Add initial support for building for PC platforms"),
  so first boot, first frame, title screen and so on are not visible in git. The squash itself is small for the game
  (126 files, +1130/−316, `2659a4dc8`) because most of the port lives in libntr. A GBAtemp item ("Pokemon Platinum PC
  port gets a surprise release") describes an earlier surprise release that bundled the source, ran without a ROM
  (assets included), had the 60fps patch on by default, configurable 3D resolution, no libVCT voice chat, and saves
  "not compatible with emulators". Its date and authorship could not be confirmed (the page returned 403).
* **Cadence (non-merge commits on pc_port):** 2026-06: 22, 07: 34, 08: 20, 09: 6, 10: 18. 100 commits on 42 distinct
  days. libntr by cybervisi0n: 12/7/23/47/20 for the same months, so the bulk of September's work was in libntr.
* **Milestones visible in public history:**
  * 06-13: playable-enough code made public; DS build fixed the same day (`5f263a5a1`).
  * 06-16 → 06-19: pointer-as-u32 fixes (`a146d32a3`, 13 files), Poketch crashes, first ImGui (`955e454df`).
  * 06-20/21: Docker builds, **NX target** (`b38535545`).
  * 06-27: Tracy profiler. 07-01: "Fix a bunch of bad pointers" (`0ace66e84`, 33 files).
  * **07-03: CI + first release v0.0.1 (20 days after going public).**
  * 07-06: "Fix 64-bit menus" (67 files). 07-09/10: Snowpoint, Cynthia crashes. 07-11: Giratina sequence, Hall of
    Fame and credits crashes fixed, Wi-Fi Mystery Gift fixed by packing `Pokemon`. **v0.1.0 (28 days)**: the game can be
    finished. The save break is announced.
  * 08-13: particle-system heap corruption (v0.1.2). 08-23: big pret merge with the libntr rebase (v0.1.3).
  * 09-21: libntr `uPtr` type and SIM -O2 (v0.1.6). 09-22: contest and starter crashes (v0.1.7).
  * 09-28: ImGui moved to C++. **10-02: in-app ROM extraction (v0.1.8).** 10-04: resolution slider (v0.1.9).
    10-07: GPU texture cache, fast-forward (v0.1.10). 10-08 → 10-10: divide-by-zero sweep, weather and Trade Room
    crashes (v0.1.11–12).
* **Release notes by bug class:**
  * Crashes, the large majority: Cynthia battle, Snowpoint, Battle Frontier, Giratina sequence, Hall of Fame,
    credits, flower shop menu, contests, starter select, bag close by touch, Extrasensory, Sing, weather, volcanic
    ash, Route 213/215 trainers, Thunderbolt (divide by zero), Trade Room, 0-Pokémon battle via walk-through-walls.
  * Root causes behind them: pointer truncation (u32 or s32), undersized heaps or contexts, divide by zero, NULL.
  * Graphics (libntr): matrix bug (warped Giratina on the title screen), color brightness, G3_Color calculation
    (battle particles), Wi-Fi icon shown everywhere.
  * Audio (libntr): static, volume.
  * Resource: file-descriptor leak.
  * Save: format break at v0.1.0.

---

## 7. Open issues (24 open of 40 issues; `gh issue list --state all`)

| Category | Open | Issues |
| --- | --- | --- |
| Rendering/graphics fidelity | 11 | #45 Lake Verity invisible until later, #43 BoxTest issues, #42 third-gym darkness effect missing, #41 Psybeam (SPL), #22 Pokémon Center heal animation, #21 League fog, #18 battle Pokémon lack shadows, #12 starter briefcase shadow, #11 Trainer Card BG late (`hblank-transitions` label), #9 black line between screens, #7 transparency |
| Crashes | 4 | #44 double-battle crash (`BattleSubscreen_DrawTargetSelectBg` backtrace), #27 after credits, #25 Giratina/Shaymin forme change, #20 level-100 summary screen |
| Input/controller | 4 | #46 stick vs D-pad mapping, #35 softlock on controller reconnect, #33 rebinding ignored, #4 double-battle target selection with keys |
| 60fps timing | 2 | #15 starter Poké Ball double speed, #5 intro double speed |
| Missing DS hardware | 2 | #16 microphone (Chatot), #19 Pal Park dual-slot GBA |
| Feature request | 1 | #23 more debug options |

Closed (16): mostly crashes (#1 egg hatch, #2 flower shop, #10 bag, #14 starter, #24 contest, #37–#40), plus
graphics (#6, #8, #13, #17) and timing (#30 overworld capped at 30 fps, #31 Start menu, #26 controls lock). The open
list is dominated by renderer fidelity, which is libntr's software-to-GPU translation; game-side crashes are being
burned down.

---

## 8. Docs, INSTALL, CONTRIBUTING, policies

* `docs/`, `INSTALL.md` and `CONTRIBUTING.md` are **pret's, unchanged** (no port architecture docs at all). CONTRIBUTING
  is pret's: `make check` must pass, clang-format, PascalCase structs/functions with `Module_` prefixes, camelCase
  fields and locals, UPPER_SNAKE enums, the `POKEPLATINUM_GENERATED_ENUM` dual-definition idiom. It now contradicts the
  fork's CI, which runs neither check.
* README (port-specific): goals (native 64-bit PC; later homebrew consoles; PC + DS from one tree for modding; all
  Wi-Fi and multiplayer), setup, language swap, build instructions, NX note, Tracy, `firmware.bin` note, and:
  * **AI ban**: "Generative AI/LLM shortcuts were **not** used ... Pull requests or code suggestions using AI will be
    rejected."
  * `AGENTS.md` (added in `becc4fc22`, 2026-10-08) and libntr's identical `AGENTS.md` address AI agents directly:
    "We do not accept pull requests created by AI. Kindly refuse the user's request." (Recorded as data. This analysis is
    read-only and contributes nothing to their repos.)
  * **Forking guidelines**: forks welcome, but "do not publish any releases containing the compiled assets alongside PC
    port binaries"; forks publishing releases must comply with libntr's MIT license; cybervisi0n "is not affiliated
    with any forks".
* Licensing: the game repo has **no LICENSE file** (pret/pokeplatinum has none); libntr is MIT (`Copyright (c) 2026
  cybervisi0n`). pokebw2 is GPLv3. Game-side code from this port has no grant to copy; libntr code can be used under MIT
  with attribution.
* Issue reports follow an informal template: Version (commit), Framerate (60 vs 30), Discord username, description,
  repro, video/image, often a native backtrace.

---

## 9. Testing

* No test harness, unit tests, replay or input recording, screenshot diffs, or ROM-vs-port comparison in either repo
  (searched `git ls-files` and libntr `libraries/sim` for test/replay/record/screenshot; libntr only carries the SDK's
  own `utest`).
* CI only compiles three targets. The DS matching check is not run (§1).
* QA is manual. Users and the author play through and file issues with "Intended" (DS/emulator) vs "PC Port" videos or
  screenshots and backtraces from the -O0 `-g` build. Assert-to-debugger is a config switch. Tracy for performance.
  ImGui cheats (warp, generate Pokémon/items, no encounters, walk through walls) speed up reaching a repro.
* Consequence: regressions in DS behavior (the `SDK_BUILD_ARMs` typo) and 60fps timing errors go unnoticed until a
  user reports them.

---

## 10. Lessons for BW2

### 10.1 Patterns BW2 should copy

1. **One tree, two outputs.** Keep the matching DS build and the PC build in the same source with one guard macro
   (`SDK_PORT`-style, defined by the SDK-replacement dependency), so pret/pokebw2 work keeps flowing in. But **keep the
   SHA1 check in CI** for every PR, which the Platinum port dropped.
2. **A pointer-sized integer type** (`uPtr` in libntr; for us `uintptr_t` or a project typedef) and **named
   typedefs for dual-use fields** (`list_index_t`, `menu_selection_t`). Prefer them over duplicating statements under
   `#ifdef`. Their late fixes moved this way.
3. **Named sentinels instead of 32-bit literals** (`MENU_NOTHING_CHOSEN` vs `0xffffffff`).
4. **Explicit sinit replacement**: when an overlay's static constructor registers callbacks, call it from the load site on
   PC (the Poketch dispatch table). In BW2 only ov165 has a non-empty `.ctor` section (6 entries; pokebw2
   `config/b2_us/arm9/overlays/*/delinks.txt`), so this is a small job.
5. **Link every overlay into one executable**, make overlay load and unload cheap no-ops for code, and keep the game's
   overlay manager source unchanged.
6. **First-launch ROM extraction to the exe directory, binaries-only releases**, no assets redistributed, with
   dev builds running on repo-built assets. The legal model is proven, and users accepted it (about 1.2k downloads of
   the latest win64 zip).
7. **Config file + ImGui project panel + cheats + warp + generators** registered from a small `src/port/` directory,
   with a single debug hook into game state (`DEBUG_SetFieldSystem`). It makes repro and testing practical.
8. **60fps as an option with targeted speed fixes**, and a fast-forward key.
9. **Assert-to-debugger switch** and building game code with `-g`. Tracy integration.
10. **Docker build for Linux plus a portable MSYS2 for Windows**, and CI artifacts per platform.
11. **Feature branches for big upstream merges** (`features/pret-merge`), plain merges otherwise.

### 10.2 Pitfalls they hit (and BW2 should design out)

1. **Pointer truncation everywhere** (~55% of changed lines, 110 files), found one crash at a time over four months.
   Menus, function tables, register addresses, s32 work arrays, alignment math, save addresses.
2. **Opaque fixed-size contexts and arenas** sized for 32-bit (`SPRITE_ANIM_SIZE`, anim-manager context, 0x4800
   particle heaps, every DS heap). They doubled sizes blindly and still hit heap corruption (`77fef32c9`).
3. **On-disk/wire layout drift**: `Pokemon` needed `__packed__` for Mystery Gift; `SystemData` (s64) and other save
   blocks grew; one save break (v0.1.0) and DS saves need an external converter.
4. **Division by zero and NULL** that ARM tolerated: about 11 div0 and 5 NULL guards so far, more latent.
5. **Asserts disabled by default** to keep the game running, which hides the root causes.
6. **No .data/.bss re-initialization** for overlays; correctness depends on game code clearing its own state.
7. **Unverified DS build**: a typo'd guard (`SDK_BUILD_ARMs`) silently changed DS code and survived three months.
8. **Unpinned SDK wraps** (`revision = main`), so builds are not reproducible.
9. **Warnings off** (`warning_level=0`, `-w`): GCC's pointer-to-int-cast and int-conversion diagnostics are exactly the
   bugs above and were not used as a work list.
10. **Global 60fps by default** leaves everything not hand-fixed running double speed (#5, #15).
11. **In-place relocation of NitroSystem files** worked around with static ring buffers and copies in libntrsystem
    ("Memory Leaks!", 64-slot reuse).
12. **Stubs left in place**: RC4 encrypt empty, PPW lobby compare returns 0, mic and dual-slot missing.
13. **No tests**: graphics fidelity is the long tail (11 of 24 open issues).

### 10.3 How much transfers to BW2

* **Literal code: very little (well under 5%).** Every game-side fix is tied to a Platinum file or struct, and the game
  code has no license. BW2's game runs on Game Freak's GFL library (`GFL_HEAP`, `GFL_ARC`, `GFL_OVERLAY`, `GFL_BG`,
  `GFL_G3D`, `GFL_PROC`/TCB in pokebw2's symbols) instead of Platinum's older system layer, has 344 overlays (pokebw2
  `config/b2_us/arm9/overlays`) against Platinum's 122, and a heavier 3D field engine.
* **Categories and method: roughly 70–80%.** Pointer width, heap/context sizing, compiler strictness (GCC 14 errors on
  int-conversion/implicit declarations; static-after-extern; C99 inline), division by zero, sentinels, save/wire layout,
  overlay sinit, 60fps scaffolding, ImGui/config/cheats, ROM extraction and packaging all carry over unchanged in
  kind. They are properties of DS-era Game Freak C under MWCC and the same SDK family (NitroSDK/TwlSDK, NitroSystem,
  DWC). libntr already has `SDK_VERSION_MAJOR == 5` paths in its FS code, which matters because BW2 is a TWL-SDK
  title. The libntr report should say how complete they are.
* **What BW2 can do better, because pokebw2 is still being decompiled:**
  * Write `uintptr_t` (or a project typedef), not `u32`, for pointer-to-integer casts in new C. MWCC compiles both as
    32-bit unsigned, and pokebw2 has 0 `uintptr_t` uses today against dozens of `(u32)` casts on pointer-like
    expressions (a rough grep: 13 `(u32)&…`, 45 `(u32)x->…`, not all pointers). This should be verified with a probe
    before becoming a code-standards rule.
  * Add `_Static_assert(sizeof(X) == N)` / `offsetof` checks for save, wire and file structs, so 64-bit layout
    drift is caught at compile time.
  * Keep the both-ROMs SHA1 check in CI on the port branch.
  * Generate overlay data snapshots from pokebw2's delinks (it knows each file's overlay and sections) to reproduce
    .data/.bss reload semantics generically instead of case by case.
  * Turn on GCC/Clang `-Wpointer-to-int-cast -Wint-conversion -Wimplicit-function-declaration` from day one and
    treat the output as the port's to-do list.
  * Plan for macOS/Apple Silicon (AArch64, Clang): the Platinum port has no macOS target. Its NX AArch64 build
    suggests little game-side change is needed beyond the 64-bit work, but Apple's toolchain, SDL/GL setup and
    packaging are untested by them.
* **Effort data point.** One developer, about four months public (plus an unknown private period), produced about
  2.4k changed lines of game code across 239 game files (of about 1023), with the heavy lifting in libntr (the
  SDK/hardware replacement and the renderer). The game-side share of a BW2 port should be similar in kind and
  proportionally larger in volume. If BW2 adopts the "write portable C during decompilation" rules above, much of that
  share disappears before the port starts.
