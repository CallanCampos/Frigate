<!-- Records how Frigate is to become one program for Pokémon Black 2 and White 2 (PLAN.md D6): a launcher that reads
the player's ROM and loads that version's game core, a shared library of today's program, with each version's test
program a thin main() over the same core; the facts of the build and the code that the design rests on, as main has
them; the owner's decisions it waits for: what D6 means, the names, the icon, a start without a ROM, D6 and the
private release candidate, the Spanish ROM, and CI's time with the build trees; and the accepted ADRs it amends. -->
# 0020: One program for both versions: a core per version, loaded by a launcher

- **Status:** proposed
- **Date:** 2026-11-02
- **Plan:** D6 (§3), §4.4 (build), §6 (CI's checks), §7 phase 5 ("one executable for both versions", and the exit's
  private release candidate)
- **Upstream:** `41f42ca1af` (upstream's of 2026-10-31), as Frigate's `main` (`e936696a97`) has it
- **Note (2026-11-02):** written from the design that the branch `port-one-program` proposed on 2026-10-19 as ADR 0017
  (one commit, `4a4c2bc9f1`, on `a14364b6ba`: not pushed then, branch STATUS.md:32, and not on `origin` now), a number
  that has since gone to [ADR 0017](0017-external-contributions.md); renumbered to 0020, the number reserved for D6
  ([README.md](README.md#numbering-and-files)), and brought up to date with `main` at `e936696a97`, whose line numbers
  the citations give ("branch 0017:N" and "branch STATUS.md:N" are lines of the branch's own files). It differs from
  the branch's design in the order of the stages, with the one build tree last and optional, in keeping each
  version's test program as a thin `main()` over its core, and in its decisions for the owner, seven where the branch
  had four. Proposed by a maintainer session, from a read-only scout of phase 5's open exit items, in a documents-only
  change with ADRs 0033 (sanitizer-clean replays), 0034 (60 fps with headroom) and 0035 (the private release
  candidate); none of its code is on `main`. Each numbered decision is a recommended default, the owner's to confirm.
  It stays proposed until the owner reviews it; the change that records the review sets the status line, and the
  change that accepts it makes the changes that [Consequences](#consequences) lists for an accepted ADR.

## Context

PLAN.md's D6 is "one executable for Black 2 and White 2, choosing the version by the ROM's hash (two executables until
then)", recommended from phase 5 (`port/docs/PLAN.md:50`), and phase 5 lists it among its items (`PLAN.md:459-462`).
Phase 5's exit, "the story can be finished on Black 2 and White 2; a private release candidate" (`PLAN.md:464`), doesn't
name it, and a phase ends when its exit criteria pass on macOS (`PLAN.md:410`).

**One build directory makes one version today.**

- `PORT_VERSION` (`b2_us` by default) picks the define that every file including the decomp's headers gets (`BLACK2`
  or `WHITE2`, through `port_decomp_env`), the program (`frigate-black2` or `frigate-white2`), the game's name and the
  icons (`port/CMakeLists.txt:88-108`, `:259-260`), and with them the ROM manifest and the development ROM's path
  (`port/cmake/rom.cmake:10-146`), the trap stubs (`CMakeLists.txt:381-391`), the overlay sections
  (`port/cmake/overlay_sections.cmake:27`, `:34`) and the macOS app's name and bundle identifier
  (`port/cmake/macos_app.cmake:24-34`). White 2 is a build directory of its own (`port/README.md:53`,
  `port/CONTRIBUTING.md:50`).
- CI configures every job without a version, so it builds and tests Black 2 alone (`.github/workflows/port-ci.yml:71`,
  `:141`, `:208`, `:278`) and makes Black 2's app (`port-ci.yml:81-83`, `:222-232`). White 2 is built and tested by
  the local verification, in a fresh tree for each version (`port/tools/sync_upstream.sh:581-592`), and by each
  session's own White 2 build ([ADR 0019](0019-pointer-cast-guard-against-a-classified-baseline.md), lines 129-130).
- Part of D6 is in place. The saves are named by game code in one product folder (`IREO.sav` and `IRDO.sav` under
  `~/Library/Application Support/Frigate` and its Linux and Windows counterparts; `port/hal/card/backup.c:95-131`),
  and a save named for the other version is left alone (`backup.c:269-275`, `:290-292`). The ROM check names the
  version and region of a ROM it refuses (`port/hal/rom/rom.c:358-378`, `:399-402`). Every oracle test passes its
  version (`port/cmake/oracle.cmake:121-126`, `:185-206`), and the oracle's saves are named for theirs
  (`port/cmake/frame_times.cmake:64`). On macOS the overlay sections are found from `__dso_handle`, the header of the
  image that the code is linked into, chosen so that the scan stays right in a dynamic library
  (`port/hal/fs/ovsec.c:11-13`, `:126`, `:157`); on Linux and Windows from bounds that the image's own sections give
  (`ovsec.c:173-231`), which a shared library's give as well (inferred). `port/gfx`, the renderers, includes no decomp
  header (`CMakeLists.txt:354-362`).

**The port's own code takes the version only from what the build gives it.** `port/hal`, `port/app`, `port/gfx` and
`port/audio` name neither `BLACK2` nor `WHITE2` (git grep). The version reaches them as the generated ROM manifest
(`rom.cmake:123-142`), whose game name titles the window (`port/app/present.c:80`), `--help` (`port/app/main.c:195`)
and an app's alert (`port/app/message_log.c:104-116`); as `PORT_VERSION_NAME` (`port/cmake/app.cmake:11`), which an
input file's `version` line must name (`port/app/replay.c:34-39`) and which a recording writes (`main.c:699`); as the
development ROM's path (`rom.cmake:144-146`); and as the window icon's generated pixels (`app.cmake:22-36`). The one
version macro on the port's side is `PORT_DS_OS_GETTICK_ADDRESS`, a DS address that the decomp's `util_wifiidtool.c`
needs (`port/include/port_host.h:117-126`). At the branch's base, its comparison of each file's preprocessed text found
2 of the 102 port files that the programs compiled differing, `rom.c` and `replay.c` (branch STATUS.md:75-79);
`main.c`'s recorder came later, and the comparison hasn't run on `main`, so that no other port file differs there is
inferred.

**The game's C differs by version in few files, but central ones.** On 2026-10-19 the branch's `version_split` found
that 74 of the 1,538 decomp files then compiled preprocess differently for the two versions; they define 1,370 global
functions, 1,007 of them called from other files, and 40 global data objects, of which other files use 18, process
tables among them (branch 0017:22-42). Almost every difference picks a file, a message, a constant or a table, not
another flow, and the game compares function pointers in places (branch 0017:36, :43). The differences come from
`#if`s on `BLACK2` or `WHITE2` (71 lines in 55 files at the branch's base, 74 in 57 on `main`), from `GAME_VERSION`
(33 matches at both) and from version macros in headers (git grep over `src/`, `include/` and `lib/`; branch
0017:30-35). The split needs a build to measure and hasn't been measured on `main`; it is likely of the same order
(inferred). The game's C is upstream's, which Frigate changes only in the three ways port/CLAUDE.md allows
(`port/CLAUDE.md:166-180`).

**What changed since the branch.**

- `main` has 1,731 commits that the branch's base doesn't; `port/` has changed in 342 files since (+48,877/−8,275),
  and `src/` and `lib/` hold 1,790 C files where they held 1,655 (git rev-list, diff and ls-tree). The branch's own
  change is 52 files (+1,959/−924). Merged onto `main` (`git merge-tree`'s trivial mode, which writes nothing), 37 of
  its paths changed on both sides and 28 of them conflict, in 57 hunks, 36 of those in 15 build and code files:
  `oracle.cmake` 8, `port/CMakeLists.txt` 5, `app.cmake` 5, `integrate.cmake` 3, and others.
- The trap stubs went from 319 functions and 44 data placeholders (branch STATUS.md:84-85) to 25 and 1
  (`port/docs/STATUS.md:226-227`); the tests from 298 for Black 2 and 295 for White 2 (branch 0017:163-164) to 406
  and 403 in PR 71's exact-head verification (`STATUS.md:69-71`); `port/cmake` from 19 fragments to 24 (`fs`, `game`,
  `lab`, `msl` and `progress` are new), and `port/CMakeLists.txt` with its fragments has 141 `add_test` sites where it
  had 96, and 62 `add_executable` or `add_library` sites where it had 36 (git grep).
- Frigate's test launcher, which the branch builds once a version (branch `port/cmake/CMakeLists.txt:114-121`) and
  whose target its `version_split.py` lists (branch `port/tools/version_split.py:46`), was retired on 2026-10-24
  (`a2fd007bf4`; [ADR 0012](0012-test-launcher-for-the-field-menus-apps.md)). The launcher below is D6's, a new
  program, not that one ([GLOSSARY.md](../GLOSSARY.md), line 117).
- The branch measured against the builds of `build/verify-tree` (branch 0017:17); the local verification now builds
  both versions fresh in the checkout that verifies a pull request's exact head (`build/port-verify-b2_us` and
  `-w2_us`, `sync_upstream.sh:581-592`).
- The pointer cast guard (ADR 0019) runs every decomp command of one build's `compile_commands.json`
  (`port/tools/check_ptrcasts.py:10`, `:666`), in each tree as the test `game_ptrcasts` (`port/cmake/game.cmake:17-21`).
  The verification records Black 2's line, today 365 sites in 229 keys, 66 of them real, over 1,713 files
  (`sync_upstream.sh:593-599`, `STATUS.md:141-143`). ADR 0019 expects the two versions' tests to become one when one
  directory builds both (lines 134-135).
- The Spanish Black 2 ([ADR 0025](0025-a-spanish-rom-by-a-manifest-of-its-own.md), proposed): `PORT_ROM_LANGUAGE=es`
  builds Black 2 against a manifest written from the player's own IRES dump, which git ignores, and refuses White 2
  (`CMakeLists.txt:110-126`, `rom.cmake:66-107`, `port/.gitignore:9-10`; ADR 0025, lines 29-40). An `es` build has
  its own build directory (lines 65-66).
- Recordings ([ADR 0027](0027-recorded-input-streams.md)) name their version: the recorder writes the program's into
  the input file (`main.c:699`), a replay refuses a file of the other version (`replay.c:34-39`), and the oracle holds
  the line to the ROM's game code (`port/tools/oracle/README.md:247-248`).
- Verification v2 ([ADR 0031](0031-per-game-frame-digests-against-a-content-addressed-oracle-cache.md), proposed)
  compiles its digest into Frigate's program and into the oracle (lines 63-64) and keys its oracle cache by inputs
  that include the ROM (lines 76-81). It adds `digest_<seq>` tests (lines 82-87), with fingerprints committed per
  sequence and version (lines 80-81, 182-183), and test presets named for their tiers (lines 85-86, 166-173), in six
  parts (lines 160-165), the first of them on `main` (`oracle.cmake:56-68`). xxHash's notice goes into the program and
  the app once the program compiles the digest (lines 154-159).
- The save lab (L1, PR 71) compiles `port_test_savelab_layout` against the decomp's headers with the version's define
  (`port/cmake/lab.cmake:7-15`), and the layout comes out the same in both games (`STATUS.md:60-61`); its guide mints
  `IREO.sav` and runs `frigate-black2` on it (`port/tools/lab/README.md:33-34`). L2's stations, whose ADR 0032 is
  being written, are held to melonDS in both versions, as the sequences are (`STATUS.md:168-170`).
- CI's time: the run of `main` at `e936696a97` (41441856595) took 29 minutes, its longest job the Intel distribution
  build on macOS 15 at 21; the run at `c0a7941e4e` (41430705815) took 36, its longest 28 (macOS 26 on Intel); the
  Linux and Windows host jobs took 4 to 7 minutes in both (`gh run view`). The owner declined scheduled runs on
  2026-11-01 and asked that long checks be optional where they would block pull requests (ADR 0031, lines 170-173).

**The branch's measurements** stand for their time. One tree built both versions' programs and tests from nothing in
1 minute 47 seconds on an M2, at a load of 15 to 86, with 339 s of user CPU, and one version alone took 58% of both,
which the branch puts down to what the versions share being compiled once (branch STATUS.md:36-49, branch 0017:77-79).
Every compile command was the two trees', argument for argument (branch STATUS.md:42-47). `frigate-black2` was
11.3 MB, and its arm64 image had 100 Mach-O sections, 87 of them overlay sections, of the 255 that Mach-O allows
(branch 0017:76, :80-81).

**How others do it.** Ship of Harkinian, in whose line Frigate is named (`port/CLAUDE.md:11`), plays every release it
supports in one program: it identifies the player's ROM by its checksum and builds its code once, asking at run time
which version is loaded, which it can do because it owns its game C (branch 0017:96-105). Frigate has the data half
of that (the ROM read at run time, saves per game code, a manifest per version) and not the code half. libretro
frontends and engines' game modules load a core for the game at run time (branch 0017:107-108).

## Decision

Frigate becomes one program for both versions: a launcher reads the player's ROM and loads that version's game core.
Each numbered item is a recommended default, the owner's to confirm.

1. **What D6 means: one program, made of a launcher and a core per version.**
   - **The launcher**, `frigate`, is plain C11 with no decomp header and no SDL, so that the core it loads brings the
     only SDL3 into the process. It finds the ROM as the program does, `--rom`, then `FRIGATE_ROM`, then the
     development ROMs (`rom.c:499-516`), with the ROMs that ADR 0035's first-launch picker remembers where ADR 0035
     puts them (item 4); `--version b2_us|w2_us`, the oracle's name for the game (`oracle.cmake:124`), picks or forces
     a version. It reads the game code in the ROM's header: `IREO` takes Black 2's core and `IRDO` White 2's (item 6
     for `IRES`); any other code ends it with the ROM's status, 66 (`port/hal/rom/port_rom.h:24`), and a message that
     names what the ROM is, as `rom.c:358-378` names it.
   - It loads the core by its full path, at a place fixed relative to its own file, never through a search path
     (`dlopen` with `RTLD_NOW | RTLD_LOCAL`; `LoadLibraryExW` with an absolute path): beside it on Linux and Windows
     and in a build tree, `Contents/Frameworks` in the macOS app (stage 4). A game whose core isn't there, as in a
     build tree, which makes one version (item 7), ends it with status 66 and a message that names the core and the
     build that makes it. It checks the core's interface number, calls its entry point with the ROM's path as `--rom`
     and the rest of its arguments, and returns its status. It never unloads a core: playing the other version is
     another process.
   - **A core per version** is today's program without its `main()`, built as a shared library and compiled with
     `BLACK2` or `WHITE2` exactly as now. No game file changes, and game-changes.md gets no row. Only one core is ever
     loaded in a process, so every symbol keeps the decomp's name in backtraces, `--run-dump` and debuggers. The core
     keeps the check that is D6's "by the ROM's hash": the whole dump's SHA-1, or every file's against the manifest
     (`rom.c:399-409`); the launcher's game code is only the first sort.
   - **The core the app ships is the core the tests run.** Each version's program keeps its name and its command
     lines as a thin `main()` over its core, so that every test, replay and oracle route runs the code the app ships;
     the launcher gets tests of its own, a smoke run of each core, and one short replay through it in each tree, a
     route test that skips without the ROM, so that the app's own way in runs the game too.
   - The window, the sound, the saves, recordings (which keep naming the running core's version), the digest, and
     what PLAN.md plans beside them (settings and the first-launch ROM setup in `app/`, mods in `mod/`, enhancements
     in `features/`, menus in `ui/`: `PLAN.md:126-168`) are the core's: one `port/` source, compiled into each.
   - D6 would read: "one program: a launcher that picks the version by the ROM's game code, and a game core per
     version that checks the ROM's hash".
2. **Names**, under port/CLAUDE.md's naming rules, which change with them (`port/CLAUDE.md:17-29`). The player sees
   the program `frigate`; `Frigate.app`, whose `CFBundleName` and display name are Frigate and whose bundle identifier
   is today's prefix alone, `io.github.callancampos.frigate` (`port/app/macos/bundle_id.cmake:11`); the disk image
   `Frigate <version>.dmg` (`0.0.N` today, or the version ADR 0035 gives a release candidate); the cores
   `libfrigate-black2` and `libfrigate-white2`, with each platform's suffix; and the log
   `~/Library/Logs/Frigate/frigate.log`, named after the executable as today (`message_log.c:40-56`, `:86-91`). The
   window's title stays "Frigate — Pokémon Black 2" or White 2's, from the running core (`present.c:80`); the saves'
   and logs' folders and `FRIGATE_ROM` don't change. Each name is still written once: the program's and the cores' at
   the top of `port/CMakeLists.txt`, the app's in `macos_app.cmake`, the identifier's prefix in `bundle_id.cmake`. The
   per-version programs keep their names as the test programs, which the app doesn't carry.
3. **One app icon:** Frigate's ship on a new plate, half Black 2's dark and half White 2's light, drawn by
   `port/assets/branding/tools/frigate_logo.py` with its 16 and 32 px images drawn pixel by pixel, as all the artwork
   is (`port/CLAUDE.md:30-38`). The window's icon on Linux and Windows still follows the running core
   (`app.cmake:22-36`).
4. **Starting without a ROM, and the picker.** The launcher settles which core runs, and gives it a ROM's path or none.
   - Given `--version` and no usable ROM, the launcher loads that version's core, which stops as today's program does,
     with status 66 and the reason (`rom.c:521-526`); so a smoke run with `--version` loads a core in CI, which has no
     ROM.
   - In a terminal, when nothing names a version and it finds no ROM, or finds ROMs of both games (both development
     ROMs, the ROM build's two inputs, which every checkout that runs T0 has: the root `README.md:32-33`,
     `rom.cmake:144-146`), the launcher refuses with status 66 and lists the choices (`--version b2_us`,
     `--version w2_us`, `--rom PATH`) rather than pick one.
   - From the Finder, where the app gets no arguments, no environment and no terminal
     ([ADR 0010](0010-macos-app-as-a-signed-copy-of-the-program.md), lines 20-24), one ROM found in item 1's order
     starts its game. With none, or with ROMs of both games, the launcher loads Black 2's core, the build's default
     version (`CMakeLists.txt:88`), in a mode that names both games, since only a core has SDL3 for the alert that an
     app shows today (`message_log.c:104-116`) and for the picker: its stop names both games, and once the picker
     exists it offers the ROMs found and the open panel, and a ROM of White 2 starts the program again with it, since
     a core is never unloaded.
   - The first-launch ROM picker that a downloaded app needs
     ([ADR 0015](0015-universal-macos-app-for-macos-11.md), lines 105-108; `PLAN.md:136`), on ADR 0035's terms
     (proposed with this one), is written in `port/app`, so that today's two programs have it first. Under D6 the
     picker is split: the launcher reads the game code and the ROMs the picker remembered, a file it can read without
     SDL3 (inferred), and a core shows the panel, where the alert is.
5. **D6 and the release candidate.** Whether the private release candidate waits for D6 is ADR 0035's to decide; this
   ADR plans on its default, that it doesn't: the release candidate may be today's two apps, each a universal app in a
   disk image of its own (`port/tools/package_macos.sh:14-26`; ADR 0015). D6 stays one of phase 5's items, built in
   the stages below as the story leaves room; if phase 5's exit passes first, the owner then moves D6 to phase 6 in
   PLAN.md.
6. **The Spanish ROM**, with ADR 0025, itself proposed. The launcher's game codes are its tree's. A build without
   `PORT_ROM_LANGUAGE=es` refuses `IRES` as the program does now, naming it "Pokémon Black 2 (Spain)"
   (`rom.c:365-366`, `:399-402`). An `es` tree builds Black 2 alone (`CMakeLists.txt:119-121`; ADR 0025, lines
   65-66): its core plays `IRES` in place of `IREO`, accepting that dump and keeping its save apart as `IRES.sav`
   (ADR 0025, lines 38-39), and its launcher sends `IRES` to it, refuses `IREO` with its name as the program does now,
   and finds no White 2 core for `IRDO` (item 1). No app made for anyone else carries an `IRES` core, whose manifest
   is one player's dump's (`port/.gitignore:9-10`; ADR 0025, lines 46-48).
7. **CI's time and the build trees**, under the owner's rule of 2026-11-01 (ADR 0031, lines 170-173).
   - CI gains one White 2 job, the Linux host build (`-DPORT_VERSION=w2_us`) and its tests, which need no ROM. It
     runs beside the others and is required through `Frigate CI`'s needs (`port-ci.yml:310-315`); every other job
     stays Black 2's. It is not a long check: the Linux and Windows host jobs took 4 to 7 minutes in runs of 29 and
     36, beside a longest job of 21 to 28, so it adds no wall time (inferred). It catches a White 2-only failure that
     only the local verification and each session's White 2 build see today (ADR 0019, lines 129-130), the pointer
     cast guard's among them, which a Linux clang build requires to run (`port/cmake/game.cmake:13-21`).
   - Each version keeps its own build tree. No CI job builds both versions, at any stage, the one tree's included:
     what needs both versions' builds (stage 4's app of both cores, stage 5's full build) runs on demand, never per
     pull request or on a schedule. The branch's one tree becomes optional and last (stage 1, below).

## Alternatives

**What D6 means (item 1).**

- **Two renamed copies in one executable**, literally one file (branch 0017:207, :213-214). On Mach-O, `clang -r`
  with an exported-symbols list prelinks a copy whose one global is its entry point; as far as the branch knew, lld
  has no such link for COFF, so on Windows every symbol of every object is renamed with
  `llvm-objcopy --redefine-syms` (branch 0017:84-92); ELF wasn't tried (branch 0017:92); and macOS's CI would need
  Homebrew's LLVM. Every symbol then shows as `b2_X` or `w2_X` in `nm`, crash reports and `--run-dump`'s lookups,
  which modders and bug reports would meet, and the two versions' overlay sections share names, so an overlay's load
  also resets the idle copy's statics, harmlessly. The fallback if a single file is required.
- **A launcher that starts the version's program** (`exec` on macOS and Linux, a child process on Windows): the
  cheapest; the app ships today's tested programs unchanged and needs none of stage 3's adaptations, the riskiest
  stage's 6 to 8 hours (inferred). But the app is three programs, Windows runs two processes for one game and passes
  the status back, and D6's "one executable" holds only as one app (branch 0017:210). The fallback if stage 3 costs
  more than one app is worth.
- **One core, with only the version-dependent files twice behind dispatch:** 1,007 functions called across files need
  generated thunks for every architecture and object format, data such as the process tables can't be thunked, and
  comparisons of function pointers break (branch 0017:208).
- **Run-time version checks in the game's C**, Ship of Harkinian's way: `PLATFORM_PC` blocks at about 100 places in 74
  files, with both versions' tables, growing with upstream (branch 0017:209), where port/CLAUDE.md allows such blocks
  only rarely (`port/CLAUDE.md:173`).
- **Every test through the launcher**, the branch's way (branch 0017:182-183): the tests run what the app runs, but
  every route's command changes (`--port` becomes the launcher with `--version`), so each oracle pull request in
  flight takes its T4 evidence again (inferred), and the per-version programs go.
- **The cores linked beside today's programs** from the same objects, the programs staying as they are: no test
  changes, but the app would ship a binary that no route runs.
- **Two programs, as now, with D6 dropped or moved to phase 6** in PLAN.md: no work, no risk and no renames; a player
  picks the app of their game, and a ROM of the other game is refused with its name (`rom.c:399-402`), which is what
  the release candidate gets anyway under item 5's default. It fails D6 as PLAN.md words it.

**Names (item 2).** An identifier that keeps a version's word (`io.github.callancampos.frigate.black2`) would name
one game for an app of two; a new identifier starts afresh what macOS keeps under it, an app's preferences and
permissions (`port/app/macos/bundle_id.cmake:3-4`), so a tester's grants to the two apps don't carry over (inferred).
A log per version (`frigate-black2.log`) would keep one game's launch from emptying the other's log, which the program
opens with `O_TRUNC` (`message_log.c:91`), at the cost of a name taken from the core rather than from the executable.
`--game` in place of `--version` would keep `--version` free for its common meaning, the program's own version, at the
cost of a second name for what the oracle and `package_macos.sh` call `--version` (`package_macos.sh:26`).

**The icon (item 3).** One game's plate misnames the other game; the mark without a plate (`mark.svg` in
`port/assets/branding`) drops the plate that every app icon has had.

**Starting without a ROM (item 4).** Playing Black 2 whenever both development ROMs are there is a silent choice that
a White 2 session wouldn't notice. From the Finder, playing the game played last when ROMs of both are found asks
nothing, but needs a record of the last game and a way to switch that the window doesn't have yet (inferred). Showing
the alert or the picker from the launcher needs SDL3 there too: one shared SDL3 library for the launcher and the cores
amends ADR 0015's static SDL3, with a second binary to sign, check and hold to the same deployment target (ADR 0015,
lines 121-122); a static SDL3 of the launcher's own would put two copies of SDL3 in the process once a core loads
(inferred); native dialogs in the launcher, one for each system, would rewrite the alert and what SDL3's
`SDL_ShowOpenFileDialog` gives (ADR 0010, lines 69-71).

**D6 and the release candidate (item 5).** Requiring D6 puts stages 2 to 4 on the release candidate's path: stage 3
is the riskiest, each stage's merge sends every open pull request back to verification on its exact head
(`port/docs/REPOSITORY-POLICY.md:13`, `:39-41`), and the cores' ad-hoc signatures have no runner to be tried on for
macOS 11 to 14 (ADR 0015, line 136). One disk image holding both apps would give one download without D6; ADR 0035
can take it up.

**The Spanish ROM (item 6).** A third core, Black 2 for IREO and for IRES from one build, compiles the game a third
time for a manifest that no app made for anyone else can carry. A table of manifests in each core can't help either:
an IRES entry exists only once a player writes it from a dump.

**CI (item 7).** Both versions in every job, as the branch's stage 1 made it (branch STATUS.md:56-58), compiles and
tests the game twice in each job. One version alone took 58% of both versions' CPU (branch STATUS.md:48-49), so the
21- to 28-minute Intel distribution jobs would run toward 35 to 50 minutes (inferred), against the owner's rule.
White 2's job as an optional check, left out of `Frigate CI`'s needs, holds no pull request up, as the owner asked of
long checks, but lets a White 2-only break merge; at 4 to 7 minutes it isn't a long one. No White 2 in CI, as now,
leaves a White 2-only break to each pull request's local verification. White 2 in the sanitized jobs too would cost
about as little, and can follow.

## Consequences

- **Players and testers** install one app and give it either ROM; the saves already stay apart by game code
  (`backup.c:120-131`). The app holds two cores, about twice the code on disk (a disk image of about 26 MB where each
  app's is 13 MB, inferred from [ADR 0015](0015-universal-macos-app-for-macos-11.md), lines 128-129), with one core in
  memory.
- **The game's C is untouched:** no row in game-changes.md, no MWCC proof, and any version difference that upstream
  adds lands in the cores by itself.
- **When the owner accepts it,** that change rewrites PLAN.md's D6 row (§3) and phase 5's item to match, and sets the
  status lines of two accepted ADRs to "accepted; amended by 0020", since the one app changes part of each: ADR
  0010's app is a copy of one program by its own target (ADR 0010, lines 28-30), where D6's is a launcher and two
  cores from two trees; and ADR 0015's program is the app's only binary (ADR 0015, lines 75-76, 121-122), where D6's
  app has three. Their other decisions stand: the ad-hoc signature as a whole
  ([ADR 0003](0003-ad-hoc-signing-no-apple-developer-account.md)), SDL3 static in what links it, macOS 11 and both
  slices.
- **Phase 6 stays open.** Widescreen
  ([ADR 0030](0030-widescreen-by-an-engine-side-hor-plus-scale-with-native-readbacks.md)) is the geometry engine and
  presentation, `port/gfx` and `port/app`, the same source in both cores. D9's settings, mods and menus are the cores'
  too, read one product folder, and can name a version where they need one. Mods reach the game's tables by their
  decomp symbol names (`PLAN.md:323-325`), which each core keeps, where the renamed copies would prefix them; native
  plugins, which come later behind a versioned ABI (`PLAN.md:336-338`), would load into whichever core runs through
  one API that both cores export (inferred), and a mod's manifest names the games it supports (`PLAN.md:339-340`).
- **The stages,** with the branch's numbers, in the recommended order, which is the owner's to confirm (the branch
  built stage 1 first: branch 0017:139-200). The machine times are the scout's estimates for this Mac (inferred):
  stages 2 to 4 about 8 to 11.5 hours together, the optional stage 1 about 4 to 6 more.
  - **CI's White 2 job** (item 7): `port-ci.yml` alone, with no code or test change.
  - **Stage 2: the core knows both versions.** Its stop without a ROM, and the picker when there is one, name and
    recognize both versions' ROMs (item 4's mode); the version that replays check and recordings write, and the
    window's icon, come from the version's generated data that the core links (its manifest, the icon's bitmaps)
    rather than from definitions on `port/app`'s command line, so that `port/app`, `port/gfx` and `port/audio` compile
    to the same text in both versions, which the branch's `version_split.py`, without `port_launcher`, checks by
    comparing the two trees' preprocessed text. T1, T2, T3 and T6 in both versions; about 1 to 1.5 hours.
  - **Stage 3: the cores and the launcher, in today's two trees.** Each tree links its core, its thin test program and
    the launcher, so no test's command changes. The platform layer stops assuming that the program is one image:
    `port_host_static_data` reads the executable's (`_mh_execute_header`, ELF's `__data_start` and `_end`:
    `port/hal/host/host_posix.c:118-135`; `GetModuleHandleW(NULL)`: `host_windows.c:41-43`, `:235`), and PXI's
    pointer range comes from it (`port/hal/pxi/pointer.c:62-64`); `port_host_global_address`, which `--run-dump` uses
    (`main.c:424`), takes the program's handle on macOS (`dlopen(NULL)`, `host_posix.c:258-267`), reads
    `/proc/self/exe` and the first loaded object on Linux (`:269-300`) and the image's symbols on Windows
    (`host_windows.c:394`). Each moves to the core's own image, as `ovsec.c` already does on macOS, and `app_symbols`
    (`app.cmake:75-80`) runs its lookup on a library. `gen_overlay_sections.py` predicts a program's sections by the
    link format it measured for a program (ADR 0015, lines 40-46), so a library's link is measured again, and the
    Mach-O section check runs per core. Windows' 8 MiB stack (`CMakeLists.txt:335-339`) and UTF-8 manifest
    (`:434-451`) belong to executables, so the launcher and the test programs carry them; the sanitized launcher
    links the sanitizers' runtime; Linux needs position-independent objects (inferred). T1 on all three systems; T2,
    T3 and T6 in both versions; T4 once in both; T5 with the cores as libraries, as ADR 0033 (proposed with this one)
    defines sanitizer-clean replays; the frame-time routes held to ADR 0034's budget (proposed with this one), since a
    core in a shared library may time differently (inferred); T7 through the launcher: each ROM, a wrong one (66), a
    missing core, both development ROMs, and `--run-dump`. The riskiest stage, about 6 to 8 hours.
  - **Stage 4: one app**, once the names are confirmed: `Frigate.app` with the launcher in `Contents/MacOS` and both
    universal cores in `Contents/Frameworks`, assembled from the two distribution trees (`build/port-dist` and
    `build/port-dist-w2`: `package_macos.sh:23-24`) and checked to come from one commit, with the same architectures
    and minimum macOS. It is signed ad hoc as a whole, as now (`port/app/macos/make_app.sh:374-375`), and
    `check_app.sh` checks the cores' signatures as it checks the program's (`port/app/macos/check_app.sh:9-11`);
    xxHash's notice goes in once (ADR 0031, lines 154-159); `package_macos.sh` loses `--version`; Linux and Windows
    ship the launcher with the cores beside it. CI's distribution job keeps building one version's tree
    (`port-ci.yml:205-211`), so it makes and checks `Frigate.app` with the launcher and Black 2's core alone, which
    tries the packaging and the signature with one core (the checks between two cores run only where both are built,
    inferred); the app of both cores is made on demand, locally or by a run started by hand, never per pull request
    or on a schedule. The names change wherever they are written: 32 files besides STATUS.md, the ADRs and the
    research notes name a per-version program, app or bundle identifier today, 12 of them documents, port/CLAUDE.md's
    naming rules among them, and the rest code, scripts, the workflow and the artwork's SVGs (git grep); GLOSSARY.md
    gains the launcher and the core beside the test launcher's record. The distribution build's tests, CI's four
    distribution runners, and T7 from the Finder with Open Anyway; about 1 to 2 hours.
  - **Stage 1, optional: one tree.** Only if the owner wants one build directory for both versions, and only after
    story leg 2 has landed, since every oracle sequence and save it adds is a per-version test that the one tree
    renames. It is redone on `main` with the branch as its recipe, never by rebasing the branch: the 57 conflicting
    hunks are in files that both sides changed, and the drift that a text merge can't show would surface only when
    configuring or testing:
    - the retired test launcher, which the branch builds and lists;
    - the five new fragments, and the tests and fixtures added since, each needing its version's suffix, since ctest
      knows a test and a fixture by name alone (the branch's lessons.md): `oracle_floccesy_save`
      (`oracle.cmake:218-227`), `replay_new_game_house_saves` (`app.cmake:188`), the timing routes, L2's stations,
      and v2's `digest_<seq>` tests and presets;
    - settings added to `port/CMakeLists.txt` since, which the one tree must give each version's own targets and
      directory, since a source file's properties belong to its directory (the branch's lessons.md):
      `-D__declspec(x)=` (`CMakeLists.txt:313-322`), Windows' `-mno-ms-bitfields`, which the save's layout depends on
      (`:275-283`), Linux's `-fvisibility=hidden` and `PORT_LIBM` (`:284-298`), the stubs' lists of objects
      (`:364-373`), and `PORT_ROM_LANGUAGE`, which is Black 2's alone (`:110-126`);
    - the verification, which configures `-DPORT_VERSION=b2_us` and then `w2_us` (`sync_upstream.sh:581-592`): under
      the branch's rule that an old `b2_us` builds both (branch `port/CMakeLists.txt:91-107`), Black 2's tree would
      build and test both and White 2 would run twice, so the verification changes in the same pull request;
    - CI, whose jobs would build both versions by the branch's default (branch `port/CMakeLists.txt:103`): each job
      gives the one tree its one version, as item 7 has them;
    - the guard, whose one `compile_commands.json` would hold both versions' commands: one test over both, each held
      to the baseline, as ADR 0019 expects, with Black 2's line still 365 sites in 229 keys, 66 real.

    The branch gives the rest: its per-version `port/cmake/CMakeLists.txt`, the check that stops on a test without its
    version (branch `port/CMakeLists.txt:373-394`), `version_split.py` and its test, the package script's test,
    `ctest_one_slice.sh`, `asm_oracle/build.py`, `test_host.c`, and the proof that every compile command is the two
    trees' (branch STATUS.md:42-47), with the renames made again by a script on `main`'s fragments. About 4 to 6
    hours, and a rename in every open pull request that adds a test.
  - **Stage 5, optional: compile once what is the same**, only after stage 1. `version_split`, which compares the
    preprocessed text in under a minute (52 s with 4 jobs, branch STATUS.md:67), catches a wrong classification in
    every build, and a full build of both versions runs on demand, where the branch had a CI job make it
    (branch 0017:194-196).
- **Work in flight.** Story leg 2 (Terrell's cut, which follows
  [ADR 0023](0023-oracle-routes-part-where-the-ds-cpu-time-decides.md) as every route does, on the branch
  `port-terrell-cut` at `d6ef61348a`, and the decrypted save comparison), the save lab's L2 with ADR 0032 and v2's
  later parts add per-version tests in each tree as they do today; no stage but stage 1 renames them, and no route's
  command changes. v2's part 2 (`8cc7d9d20a` on the branch `port-digest-stream`) compiles the digest into Frigate's
  program, whose code becomes the cores', and adds xxHash's notice to the app, which stage 4 carries once; its part
  6's presets select tests in one build tree, so each version's tree has them, and stage 1, if it comes, would select
  by version instead (inferred). The ToScreen depth fix (the branch `port-ge-depth-cut`, `port/gfx/ge/ge.c`) is the
  same code in both versions. The documents pull request (the branch `port-docs-hygiene-1102`) adds a check of the
  documents' links and tables to CI, which stage 4's renames pass through, and edits `port-ci.yml` beside item 7's job
  (a text conflict at most, inferred). Stages 2 and 3 change the C that the oracle routes run, so each lands where it
  doesn't restart a story pull request's final verification (inferred).
- **To watch:** the launcher loads by full path alone, since a library found by a search path can be another's; the
  nested cores' ad-hoc signatures on macOS 11 to 14, which no runner covers (ADR 0015, line 136); each core's Mach-O
  sections within its own 255; `version_split`'s count as upstream decompiles more; and the ROMs the picker
  remembers, which the launcher and the cores both read and which must never reach a test, each of which runs with a
  save directory of its own (`CMakeLists.txt:457`).
- **Revisit if** a single file becomes a requirement (the renamed copies), stage 3 costs more than one app is worth
  before the release candidate (the launcher that starts the version's program), the second compile becomes a burden
  (stage 5), upstream turns version differences into run-time checks (then the game's C could choose at run time), or
  another region's configs arrive (another core).
