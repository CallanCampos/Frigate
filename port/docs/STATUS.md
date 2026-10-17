# Port status

Where the port stands, newest entry first. A session adds an entry after each local sync with
`port/tools/sync_upstream.sh` and each finished task ([PLAN.md §8](PLAN.md#8-how-each-development-iteration-runs)), in
the same commit as the work it records; the hourly `port-sync` Action's merges get no entry of their own
([SYNC.md](SYNC.md)). The plan of record is [PLAN.md](PLAN.md); every change outside `port/` is in
[game-changes.md](game-changes.md).

## How to add an entry

Put the new entry above the newest one, under a heading `## YYYY-MM-DD: title`. Fill every field. A measure or test
level that doesn't exist yet says "not yet"; no field is left out.

| Field | What to write |
| --- | --- |
| Date | The day of the sync or task, `YYYY-MM-DD`, in the heading |
| Upstream | The `fuddlesworth/pokebw2` commit last merged, short SHA, with its PR number when it is a merge |
| Port | The port commit the entry describes. An entry committed with its own work can't name that commit: write `PORT_SHA`, and the next entry's commit replaces it with the SHA from `git log -1 --format=%h -- port/docs/STATUS.md` |
| ROM | Result of `python3 configure.py && ninja`: both SHA1s match, or what failed (T0) |
| Host build | Result per platform and configuration (T1) |
| Tests | Result of each level T2–T7 that ran, with counts; "not yet" for levels that don't exist |
| Tiers | Functions and code bytes in tiers (a), (b) and (c) from `port/tools/progress/`, and the change since the last entry |
| Trap stubs | How many functions the host build still stubs, and which the game hit first |
| 64-bit warnings | The host build's pointer/int warning count, game and `lib/` apart; it must not rise |
| Work done | What changed, with ADRs and game changes by number |
| Next | The next task: the earliest unmet exit criterion of the current phase that the decomp allows |
| Blockers | What waits on upstream or on a decision, with issue numbers |

## 2026-10-17: oracle comparisons from power-on to the start menu

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry.
- **Port:** `PORT_SHA`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: all 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2, and Black 2 under ASan and UBSan; not pushed,
  so no CI run (Linux unbuilt: the program now links with `ENABLE_EXPORTS` and `CMAKE_DL_LIBS` there).
- **Tests:** T0 as above. T2: Black 2, 211 of 214 `ctest` tests pass and 3 are skipped without the oracle
  (`oracle_boot`, `oracle_opening`, `oracle_title`); White 2, 209 pass, the same 3 are skipped, and 2 fail as they did
  before this work (`replay_start_menu`, whose input file is Black 2's, and `rom_game_overlays`, on overlay 12's
  address). Among them:
  - the run directory's 93 checks, plain and sanitized (memory dumps added), and `oracle_compare`'s 35 tests;
  - the 3D renderer's 777 checks, 13 of them new on edge interpolation (they fail on the old interpolation);
  - `hal_replaced_c` (new), and the asm oracle with its products test (4 functions, 32,000 cases each build, every
    overlap of the arguments, against the ROM's code);
  - `oracle_opening_hashes` (337 frames and 638 screens without 3D in each version) and `oracle_title_hashes` (400
    and 393 frames, 544 and 541 screens, 321 and 314 of the frames on the count).

  Sanitized: `app_run_dir`, `oracle_compare`, `replay_start_menu`, `g3d_ds3d`, `g3d_hal`, `hal_replaced_c` and the
  smoke test pass; both sequences under ASan and UBSan (3,600 and 3,800 frames) give the plain build's hashes and
  counts at every frame and its `NNS_G3dGlb` where dumped, with the one misaligned read already known
  (`g2d_CellAnimation.c:33`, NitroSystem reading packed file data). T4: `oracle.py sequence`, the port against
  melonDS 1.1, every port frame on the game's count (the movies by their pictures in order, no oracle picture
  skipped) but the known differences, and NNS G3D's state on the count through the title:
  - opening (power-on to the title's first 274 frames, nothing pressed): Black 2, 3,800 of 3,800 frames match (3,526
    exact at 6 bits, 274 with 3D within the tolerance); White 2, the same, with the DS's torn first frame of the
    opening declared; `NNS_G3dGlb` equal at all 274 frames compared in each;
  - title (the opening skipped with START, the title's 3D on both screens, START to the start menu): Black 2, 3,579
    of 3,579 compared match (1,474 exact, 2,105 with 3D within the tolerance), 21 known differences left out; White 2,
    3,572 of 3,572 (1,477 and 2,095), 28 left out; `NNS_G3dGlb` equal at 2,470 of 2,470 frames in each, 15 left out;
  - 3D beyond the pixel tolerance: a median of 17 pixels a screen in Black 2's title and 7 in White 2's, at most 80 of
    the 245 allowed (before the rasterizer fix, a median of 79 and at most 172).

  T3, T5's nightly runs, T6, T7 not yet.
- **Tiers:** unchanged since the last entry (no decomp C changed).
- **Trap stubs:** unchanged, 581 function stubs and 67 data placeholders; neither sequence reaches one in either
  version.
- **64-bit warnings:** unchanged: the change outside `port/` only adds `#ifndef PLATFORM_PC` guards.
- **Work done:**
  - Phase 2's last exit criterion: oracle comparisons pass within tolerance, for both versions, in two sequences
    (`port/tests/oracle/<name>_<version>.pbwinput`: opening and title; `oracle.py sequence` and `sequence-check`;
    ctest `oracle_<name>` and `oracle_<name>_hashes`).
  - compare.py pairs on the game's count (`--align counter`), the movies by their pictures (`--clock`), with declared
    count lags (`--lag`), and fails any oracle picture no port frame shows (`--known-oracle` for the DS's torn
    frames); `--align sequence` alone, which only holds pictures and their order, passed a port that showed pictures
    late or held a still. The references keep the count each row expects; every known difference is declared, with
    its evidence in `port/tools/oracle/README.md`.
  - The program writes its own run directory for compare.py (`--run-dir`, `--run-screens`, `--run-dump`,
    `port/app/run_dir.c`): `frames.tsv` with GFL's frame count and the game frame at every frame hook, PNG files at
    the frames of a list, `memory.txt` with globals by name (`dlsym`), through a pointer with `*`. `oracle.py port`
    runs the port once.
  - Two 3D fixes. The decomp's `MTX_Concat33`, `MTX_Concat43`, `MTX_Concat44` and `MTX_MultVec43` read an argument
    after storing over it when the result replaces it, which the original never does, so on a host the title's camera
    looked elsewhere (`MAT3_RotationEulerZYX`): `port/hal/fx` has versions in the original's order, checked against
    the ROM's code; game change, Platform; a decomp finding;
    [ADR 0008](adr/0008-host-versions-of-sdk-c-that-isnt-the-roms.md), amending ADR 0005, says when the HAL may
    replace SDK C and when the replacement goes (`hal_replaced_c`). And the rasterizer takes values along an edge by
    line, as melonDS does, not along an X-major edge's run, which had put whole spans' colours a step apart.
  - Settled with evidence (README, "Game frames and video frames" and "Known differences"): START held doesn't skip
    the opening on melonDS or on the port (a press does, on the same pass on both); the oracle's start-up gives
    oracle frame = port frame + 242 from count 5 on through the boot screens, and game frames part where the game
    waits on a clock (from the black after the copyright screen) and line up again at a press; the DS's camera leads
    after START (NNS G3D's state a pass ahead), and White 2's cry ends a pass later against the DS's passes because
    the DS starts it later (traced by the sound driver's shared work on both programs).
- **Next:** New Game's intro (overlay 294): 64-bit fixes for the particle system's work buffers and for the Pokémon
  parameter setters, which take pointers as `u32`.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.

## 2026-10-17: the title screen and the start menu, natively

- **Upstream:** `e484d5011` (merge of PR #353), which brings the ITCM's ClAct renderers (PR #352, decomp work this
  session did to unblock the title screen).
- **Port:** `570db24f4`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: all 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`.
- **Host build:** arm64 macOS, RelWithDebInfo and unoptimized (`-fgnu89-inline` made the unoptimized build link); CI
  green on macOS and Linux for every commit pushed.
- **Tests:** T0 as above. T2: 207 of 207 `ctest` tests pass, among them the 3D engine's (1,400 hand-derived checks,
  plain and sanitized), the sound driver's 31 and its two ROM scenarios (all 1,390 sequences), the G2D loaders' ROM
  equivalence (every one of the 30,078 G2D files of each ROM against a simulation of the DS's unpack), display capture,
  and `replay_start_menu`, which plays an input file from power-on to the start menu. T4: the melonDS oracle builds on
  request (`-DPORT_BUILD_ORACLE=ON`); a comparison of the boot sequence before this entry's renderer fix matched 33 of 33
  sampled frames at 5 bits per channel, and 11 of 11 at 6 bits in a scratch build with this entry's fix. T3, T5-T7 not yet.
- **Tiers:** (a) 32,093 functions, 2,859,042 code bytes (74.3%); (b) 5,333, 642,866 bytes; (c) 3,592, 348,336 bytes
  (9.0%). Since the research measurement: (a) +209 functions and 26,318 bytes, (c) -3,024 bytes, the ITCM renderers.
- **Trap stubs:** 581 function stubs (355 in overlay 36, 113 in overlay 11) and 67 data placeholders. The game runs
  4,400 frames to the title screen, and through START to the start menu, without reaching one.
- **64-bit warnings:** game / `lib/` on arm64: `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast` 49 /
  117, `-Wint-to-pointer-cast` 76 / 144, `-Wint-to-void-pointer-cast` 69 / 132 (`port/tools/compile-probe`); as in the
  research measurement but for three void-pointer casts in new upstream C.
- **Work done:**
  - Phase 1's exit, the boot logos in the repository build, and most of phase 2: the boot screens, the Game Freak logo,
    the opening movie and the title screen with its 3D Black Kyurem play as on the DS, and the start menu responds.
  - NitroSystem's 2D files in host structs with a registry tied to heap frees
    ([ADR 0007](adr/0007-nitrosystem-files-in-host-structs.md)); the DS's geometry engine and a CPU 3D renderer;
    display capture; the ARM7's sound driver in host C with SDL3 audio output and `--audio-dump`
    ([ADR 0006](adr/0006-sound-driver-as-built.md)); the melonDS test oracle (`port/tools/oracle`); `--input` replays
    in the oracle's input format, counting game frames by GFL's own frame count.
  - Game changes, all proven neutral (`port/docs/game-changes.md`): the sound archive's file table, wave archive links
    and the sound heap's data as 32-bit handles or `uptr`; GFL's allocation head padded to 32 bytes on hosts. NNS FND's
    heaps are aligned for 64-bit structs by the platform layer: a strict ASan and UBSan build runs the 4,400 frames with
    two misaligned reads left, both NitroSystem reading packed file data.
  - The 2D renderer follows melonDS 1.1 in ignoring a colour's bit 15; the overlay-sections check accepts a program
    whose `__eh_frame` ld64 dropped.
- **Next:** phase 2's oracle comparison through the start menu with one input file for both programs, then New Game's
  intro (overlay 294): 64-bit fixes for the particle system's work buffers and for the Pokémon parameter setters,
  which take pointers as `u32`.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.

## 2026-10-17: phase 1 platform layer, and the game changes it needs

- **Upstream:** `4aa041311` (merge of PR #348); unchanged.
- **Port:** `e49c96554`.
- **ROM:** both SHA1s match; `check_mwcc_neutral.py`: all 3,138 file and version pairs give the same MWCC token stream
  as upstream's `4aa041311`.
- **Host build:** arm64 macOS; CI checks macOS and Linux.
- **Tests:** T0 as above. T2: 144 of 144 `ctest` tests pass (OS scheduler and interrupt scenarios, the 2D renderer's
  16,318 pixel checks, PXI services, ROM and card, overlay data sections, VRAM banks, DMA, integration, the asm
  oracle, the smoke test, which now runs the first frame). T3–T7 not yet.
- **Tiers:** not re-measured (upstream unchanged).
- **Trap stubs:** 587 function stubs and 69 data placeholders.
- **64-bit warnings:** not yet counted by the host build.
- **Work done:**
  - The phase 1 platform layer ([ADR 0005](adr/0005-platform-boundary-as-built.md)): the OS on fibers with the
    decomp's own scheduler and a virtual clock (`port/hal/os`); the PXI bus and the ARM7's services (power, touch,
    firmware settings, reset, cartridge slot, mic, a sound driver); the ROM from the player's dump, the backup chip as an
    atomic save file, and resident overlays whose statics reset on load (`port/hal/rom`, `card`, `fs`); VRAM banks
    aliased by page mapping (`port/hal/gx`); DMA done at once (`port/hal/dma`); the CPU 2D renderer (`port/gfx/ref`),
    screens, input and an SDL window (`port/gfx/screens.c`, `port/app`, `port/hal/input`), `--screenshot`.
  - Game changes, all proven neutral (`port/docs/game-changes.md`): `uptr`, `sptr` and `ulong32`, and the sites that
    needed them (NNS FND heaps, `HW_DTCM`, GX register-address parameters, the sound heap, two alignment computations,
    the RTC date and the misc save block); the heap size hook.
  - `port/hal/gfl/heap_policy.c` (root heaps x4, child heaps x2, `--heap-scale-root`/`--heap-scale-child`);
    `port/hal/dwc/ov011_pending.c`, the four overlay 11 symbols DWC's boot-time init reaches before upstream's claim
    writes them; `port/docs/decomp-findings.md`.
  - The generator routes `reg_CP_DIV_RESULT` (a plain `u64` register) through its accessor; before, every `FX_Div`
    returned 0. Crashes outside the trap pages now print their address and a backtrace, on their own signal stack.
  - In an exploration tree with the remaining fix (below) the game played the boot screens and the whole Game Freak
    logo movie for 1,100 frames, deterministically, and reached the title screen.
- **Next:** NitroSystem G2D's in-place relocation (PLAN.md §4.8 rule 5), in progress as a workflow: a relocation
  registry tied to heap frees and host unpackers for every G2D format, proven on every such file in the ROM. Then the
  boot logos in the repository build (phase 1's exit), then phase 2.
- **Blockers:** the repository build crashes in the boot screens (`src/ov162/boot_screens.c`, the first character
  data), on the G2D relocation above. Overlay 36 (issue #83) still gates phase 3.

## 2026-10-17: phase 0 complete; DS addresses, asm replacements and 64-bit layouts

- **Upstream:** `4aa041311` (merge of PR #348); upstream hasn't moved since this session's sync.
- **Port:** `d63819fc5`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`) after a full `ninja` with this commit's
  `include/types.h` change; `check_mwcc_neutral.py`, now comparing token streams: all 3,138 file and version pairs give the same MWCC token stream as upstream's `4aa041311`.
- **Host build:** arm64 macOS and x86-64 Linux build in CI (`port-ci.yml` green), which closes phase 0's exit criteria
  (links, starts, opens a window, reaches the game's `main()`, both ROMs match, CI green).
- **Tests:** T0 as above. T2: 4 of 4 `ctest` tests pass: the smoke test, C unit tests of the host MI and FX/MATH/RC4
  functions, and `asm_oracle`, which runs the ROM's own machine code for each replaced asm function in a test-only CPU
  emulator and compares it with the host C on thousands of seeded inputs, both versions (it skips without the ROM
  extract, as in CI). T3–T7 not yet.
- **Tiers** (`port/tools/progress`, `b1369e48f`): (a) 31,884 functions (77.7%), 2,832,724 bytes (73.6%); (b) 5,538
  (13.5%), 666,160 bytes (17.3%); (c) 3,596 (8.8%), 351,360 bytes (9.1%), unchanged since 2026-10-16 (overlay 36's C
  hasn't landed upstream yet).
- **Trap stubs:** 664 function stubs and 72 data placeholders; the game stops first at `OS_Init`.
- **64-bit warnings:** not yet counted by the host build.
- **Work done:**
  - DS addresses through build-time header translation ([ADR 0004](adr/0004-ds-addresses-by-header-translation.md)):
    static 16 MiB windows per memory-map region in `port/hal/hw`, accessors for the registers whose accesses act
    (geometry command log, bit-exact divider and square root, VCOUNT, timers, GXSTAT, matrix readbacks), and
    `port_hw_g3_send` for SDK copies to the geometry FIFO.
  - Host C for 38 SDK asm functions (MI copies, fills, compare, swap and LZ decompression; FX matrices; MATH CLZ,
    QSort, SHA-1 block; RC4), each proven against the ROM's code by `port/tests/asm_oracle`.
  - `include/types.h`: `u64`, `s64` and `f64` keep the DS's 4-byte alignment on hosts, so structs keep the DS's
    layout (a bag sort entry stays 12 bytes, so `MATH_QSort` orders equal keys as on the DS; save blocks).
  - The three phase 0 spikes kept in `port/spikes/` (overlay data sections, VRAM aliasing, fibers); the host build
    compiles `port/hal` against the decomp's headers with the game's ABI flags, `port/gfx` as its own target, and
    `port/cmake/*.cmake` fragments.
- **Next:** phase 1, running as a workflow: the OS core on fibers, the PXI bus and host ARM7 services, the ROM, file
  system, saves and overlay data resets, VRAM banks, and the CPU 2D renderer, then integration toward the boot logos.
  Later: fast paths for the byte-wise MI copies (the game's most-called functions), under the same differential tests.
- **Blockers:** none for phase 1. Overlay 36 (issue #83) still gates phase 3.

## 2026-10-17: phase 0, the native build links and enters the game's main()

- **Upstream:** `4aa041311` (merge of PR #348), merged in `0f65946be` by this session's sync, which built both ROMs and
  checked both SHA1s.
- **Port:** `577554b0b`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`) after a full `ninja` with this commit's guards;
  `port/tools/check_mwcc_neutral.py`: all 3,138 file and version pairs preprocess under MWCC exactly as upstream's
  `4aa041311` does.
- **Host build:** arm64 macOS, Apple clang, RelWithDebInfo: 1,480 of 1,518 C files of `src/` and `lib/` compile
  natively (38 are replaced by the platform layer, `port/hal/replaced.txt`) and link into `pokeblack2` (10 MB) with
  SDL3 3.4.18. Linux and Windows not yet; CI (`port-ci.yml`) builds macOS and Linux from this commit on.
- **Tests:** T0 as above; T1: builds; T2: the smoke test (`ctest`) passes: the program reaches the game's `main()`.
  T3–T7 not yet.
- **Tiers:** not re-measured since 2026-10-16.
- **Trap stubs:** 692 function stubs and 72 data placeholders. The game stops first at `OS_Init` (called by
  `SystemInit`), the first SDK module the platform layer replaces.
- **64-bit warnings:** not yet counted by the host build (silenced there); the compile probe baseline of the
  2026-10-16 entry stands.
- **Work done:**
  - `port/CMakeLists.txt`: the host build, with MWCC's language model (signed char, 16-bit `wchar_t`, wrapping
    arithmetic, no strict aliasing, zeroed locals) and SDL3 (Homebrew's, or the pinned release).
  - `port/tools/gen_host_tree.py`: the source list, an `include/` mirror without MSL's C library headers, the generated
    constants. `port/tools/gen_stubs.py`: a trap stub for each DS function the C references and nothing defines, and a
    placeholder on a page with no access for each such data object, so the first use is reported by name.
  - `port/hal/trap.c`, `port/hal/os/memory_map.c`, `port/hal/stubs/unresolved.c`, `port/include/port_host.h`,
    `port/app/main.c` (SDL3 window, the game on its own thread, `--headless`).
  - Game changes (`port/docs/game-changes.md`): `OVERLAY_ID(n)` is `n` on a host; `MATH_CountLeadingZeros` uses the
    compiler's builtin; the 18 MWCC `asm` functions of seven mostly-C NitroSDK files are behind `#ifndef PLATFORM_PC`.
  - `port/tools/check_mwcc_neutral.py`, which proves a guard-only change neutral for every file at once.
  - `.github/workflows/port-ci.yml`: the host build and tests on macOS and Linux.
- **Next:** the phase 0 spikes (overlay data sections, VRAM aliasing, fibers), then phase 1: the platform layer in the
  order the traps ask for it, starting with `OS_Init`, threads, IRQs, arenas, MI and FX.
- **Blockers:** none for phase 0 or 1.

## 2026-10-17: solo project, ad-hoc signing and upstream guards

- **Upstream:** `462e90193` (merge of PR #345), still the last merged. Upstream `main` is now `69e82e80c` (merge of
  PR #347), four commits ahead, and not yet merged by this commit; the next sync merges it.
- **Port:** `5b1102e5d`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); this commit changes nothing outside `port/` and
  `.github/`.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Tiers:** not re-measured; no upstream commit has been merged since the 2026-10-16 entry.
- **Trap stubs:** not yet.
- **64-bit warnings:** not yet from a host build; the compile probe baseline in the 2026-10-16 entry stands.
- **Work done:**
  - Two decisions by the user: the port is a solo project, so nothing about it is posted on `fuddlesworth/pokebw2`
    and its portability changes stay here (D10, [ADR 0002](adr/0002-solo-project-no-upstream-posts.md), which amends
    [ADR 0001](adr/0001-repository-and-upstream-sync.md)); and there is no Apple Developer account, so macOS builds
    are ad-hoc signed and never notarized (D11, [ADR 0003](adr/0003-ad-hoc-signing-no-apple-developer-account.md)).
  - The reference guard: whatever the repository's visibility, `port/tools/sync_upstream.sh` stops with exit code 5
    and the `port-sync` Action with the `references` outcome before pushing commits whose messages reference issues;
    `--allow-references` pushes them once the user agrees. The sync also checks strictly that `origin` is
    `CallanCampos/pokebw2-native`.
  - The write guard: `port/tools/hooks/port_guard.py`, a PreToolUse hook that `port/tools/bootstrap_checkout.sh`
    installs in the clone's `.claude/settings.local.json`, which Claude Code also reads in the clone's worktrees. The
    same script sets the `upstream` remote's push URL to `DISABLED-port-never-pushes-upstream`.
  - `port/CLAUDE.md`: `gh` writes only to the port, no @-mentions of upstream's people, port commit messages name
    only the port's own issues, and the root `CLAUDE.md`'s overlay claim and progress
    rules and the `decomp-file`, `finish-file` and `match-function` skills never run in the port clone. The plan,
    ADRs, landing page, game-change ledger and research index say the same. The scripts in
    `port/tools/research/sdk-surface/` take the repository as an argument, or find it from their own location,
    instead of a hardcoded worktree path, and write their outputs to `build/port-research/sdk-surface/`, which git
    ignores.
  - No branch of the decomp checkout holds the port commit `0835ef06c` any more, so no decomp session there can push
    it upstream. The `port-sync` Action was paused while these guards were written and is re-enabled after this push.
- **Next:** the session sync with `port/tools/sync_upstream.sh`, merging `69e82e80c` (none of its four new commits
  references an issue), then phase 0: CMake skeleton with SDL3, generated source lists and trap stubs,
  `include/platform.h`, the overlay `.data`/`.bss` spike, then the first arm64 macOS link.
- **Blockers:** none for phase 0. Overlay 36 (issue #83, claimed by mariano54) still gates phase 3. The four ITCM
  ClAct renderers (phase 2) have no owner; writing them is ordinary decomp work for a decomp session, which the user
  starts. D10, which the 2026-10-16 entry listed as waiting, is decided.

## 2026-10-17: local clone and first syncs

- **Upstream:** `462e90193` (merge of PR #345); unchanged since the repository was created.
- **Port:** `ecb9e5f65`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`), from the first run of
  `port/tools/sync_upstream.sh` in the new clone, which configured from scratch (downloading wibo, sjiswrap and dsd
  v0.12.1-dsi.4) and built both ROMs. 22 compiler warnings, the same upstream ov036 ones as before.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Tiers:** not re-measured; upstream hasn't moved since the 2026-10-16 entry.
- **Trap stubs:** not yet.
- **64-bit warnings:** not yet from a host build; the compile probe baseline in the 2026-10-16 entry stands.
- **Work done:**
  - The public repository `CallanCampos/pokebw2-native` exists with upstream's full history and the port commit
    `0835ef06c`. Upstream's `build.yml` is disabled here (it needs upstream's ROM deploy key); `compile.yml` and
    `standards.yml` pass on `main`.
  - The `port-sync` Action ran once by hand and reported upstream already merged. Its merge path runs at the next
    upstream push.
  - The local clone `~/dev/decomp/pokebw2-native` is set up: `origin` is the port, `upstream` is the decomp, gh's
    default repository is the port, `port/tools/bootstrap_checkout.sh` linked the inputs from the decomp checkout,
    and `CLAUDE.local.md` (`@port/CLAUDE.md`) with `.worktreeinclude` loads the port rules in every session.
  - `port/tools/sync_upstream.sh` ran end to end on the real build: fetch, already-merged check, configure, ninja,
    both SHA1s, nothing to push.
- **Next:** phase 0 — CMake skeleton with SDL3, generated source lists and trap stubs, `include/platform.h`, the
  overlay `.data`/`.bss` spike, then the first arm64 macOS link.
- **Blockers:** none for phase 0. Overlay 36 (issue #83, claimed by mariano54) still gates phase 3.

## 2026-10-16: repository created, research and plan complete

- **Upstream:** `462e90193` (merge of PR #345). The repository was created from upstream's full history at this
  commit.
- **Port:** `0835ef06c`.
- **ROM:** both SHA1s match (Black 2 `e51e6dfb…`, White 2 `b5d7490b…`), full `ninja` on `462e90193` with `port/`
  present. 22 MWCC "function has no prototype" warnings, all in upstream's new `src/ov036/field_g3dobj.c` and
  `src/ov036/fld_wipe_3dobj.c`, none from the port.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Trap stubs:** not yet (no generator). The functions they will cover, those with no C, numbered 3,596 on
  `0c2bbee9d`.
- **64-bit warnings:** not yet from a host build. The compile probe's baseline on arm64 macOS (clang 23.1.3,
  `0c2bbee9d`): 1,164 unique pointer/int truncation sites in game code (`src/` and `include/`), 700 of them the
  `OVERLAY_ID(n)` macro, leaving 464 sites in 154 of 1,170 game files; 678 sites in 117 `lib/` files.

### Tiers

Measured on `0c2bbee9d`, three upstream merges (PRs #343–#345) before the seed commit, Black 2; White 2 has the same
tiers function for function ([decomp-progress.md](research/decomp-progress.md)). The next sync re-measures.

| Tier | Functions | Code bytes |
| --- | ---: | ---: |
| (a) complete file, matching C, linked | 31,068 (75.7%) | 2,751,648 (71.5%) |
| (b) C written, file incomplete, ROM code still linked | 6,354 (15.5%) | 747,236 (19.4%) |
| (c) no C at all | 3,596 (8.8%) | 351,360 (9.1%) |
| C exists, (a) + (b) | 37,422 (91.2%) | 3,498,884 (90.9%) |

Totals: 41,018 ARM9 functions and 3,850,244 code bytes, without 2,384 linker veneers. Of tier (b), 432 functions
(145 KB) have a row in `docs/nonmatching-functions.md`. About 84% of initialized data is C; 3,707 of 13,063 pointer
words in data are not (2,789 in overlay 36, 674 in overlay 12's event-script command tables).

When tier (c) reaches zero, at each area's own recent rate:

| Area | Left (bytes) | Rate, last 7 days | ETA | Rate, last 3 days | ETA | Who |
| --- | ---: | ---: | --- | ---: | --- | --- |
| ov036 field | 110,808 | 12.2 KB/day | ~Oct 25 | 27.1 KB/day | ~Oct 20 | mariano54 (claim; re-landing a fork branch) |
| ov011 Wi-Fi stack | 143,146 | 10.3 KB/day | ~Oct 30 | 16.4 KB/day | ~Oct 25 | mariano54 (claim; same fork) |
| ov182 Wi-Fi utility | 79,342 | 3.0 KB/day | ~Nov 11 | 2.9 KB/day | ~Nov 12 | mariano54 (claim) |
| LTD (TwlSDK, DSi) | 11,164 | lumpy | days | | | no issue tracks it |
| ITCM (ClAct renderers, IRQ) | 4,788 | 200 B in 20 days | none | | | no issue tracks it |
| main (crt0, SVC stubs, `exit`) | 2,112 | flat | none | | | no issue tracks it |

Overlays 36 and 11 could be C within one to two weeks at their recent rates. The rest of tier (c) only by
mid-November, if overlay 182 keeps its pace and someone takes the untracked ITCM, crt0 and DSi code. These are
extrapolations from a 20-day-old project whose progress comes in large merges.

### Compile probe

From [RESEARCH.md §9](RESEARCH.md#9-the-native-c-compile-probe): clang accepts all 1,170 `src/` files for 32-bit
targets, and for arm64 macOS once `OVERLAY_ID(n)` becomes `n` on hosts. Every remaining failure is in `lib/`: 43 files
with MWCC `asm` and 6 with dialect issues. i386 SysV matches MWCC's layouts exactly; save blocks are pointer-free and
keep the DS layout on 64-bit hosts with `aligned(4)` typedefs for `s64`/`u64`/`f64` and a typedef for two
`unsigned long` fields. The 64-bit work is estimated at 500–700 edited lines in about 160 game files.

### Work done

- Repository created from upstream's history; upstream's `build.yml`, which needs a ROM deploy key this repository
  doesn't have, disabled here with `gh workflow disable build.yml -R CallanCampos/pokebw2-native`.
- Research ([RESEARCH.md](RESEARCH.md) and 11 reports) and the plan ([PLAN.md](PLAN.md)).
- Measurement tools in `port/tools/` (progress tracker, compile probe, research scripts).
- Port rules (`port/CLAUDE.md`), the landing page (`.github/README.md`), `port/README.md`, the hourly sync Action
  (`.github/workflows/port-sync.yml`), the local sync and bootstrap scripts, the sync guide ([SYNC.md](SYNC.md)),
  [ADR 0001](adr/0001-repository-and-upstream-sync.md), this log, and the game-change ledger (empty).

### Current phase: 0, foundations

Exit criteria ([PLAN.md §7](PLAN.md#phase-0-foundations)): the executable links on macOS, starts, opens a window and
reaches `main()`; both ROMs still match; CI green.

### Next

1. CI for the host build, and the progress tracker and compile probe wired in as metrics for this log.
2. CMake skeleton with SDL3 in `port/third_party/`.
3. `port/tools/gen_sources.py`: source lists generated from `config/`, minus `hal/replaced.txt`; trap stubs; and
   placeholder definitions for data symbols with no C (zero-filled or loaded from the ROM at startup), so the host
   build links.
4. `include/platform.h` with `PLATFORM_PC`, `uptr`/`sptr` and the host typedefs; `OVERLAY_ID(n)` as `n` on hosts.
   These are game changes: each gets a row in [game-changes.md](game-changes.md).
5. Compile every game and library C file for arm64 macOS and fix 64-bit sites far enough to link: the 9 incompatible
   function-pointer casts, the 5 missing returns and the save-struct typedefs first.
6. Spikes: overlay `.data`/`.bss` sections on Mach-O, VRAM page aliasing on macOS, fibers with DS priority
   scheduling. Each ends in an ADR.

### Blockers

- None for phase 0: code and data with no C get trap stubs and placeholders (Next, item 3).
- Phase 3 (the overworld) needs overlay 36's C and data: issue #83, claimed by mariano54 on 2026-10-16 and being
  re-landed from a fork. Overlay 12's event-script command tables (674 pointer words) also need to become C.
- Phase 2 needs the four ITCM ClAct renderers as C; no issue tracks them and nobody owns them. Opening an issue for
  them is a public post that needs the user's OK, and writing them is decomp work for a decomp session in the decomp
  checkout, never for a port session.
- Decision D10 (proposing the portability conventions upstream) waits on the user's OK.
