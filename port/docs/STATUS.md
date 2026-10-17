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

## 2026-10-17: local clone and first syncs

- **Upstream:** `462e90193` (merge of PR #345); unchanged since the repository was created.
- **Port:** `PORT_SHA`.
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
