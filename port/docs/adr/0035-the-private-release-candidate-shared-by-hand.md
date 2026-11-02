<!-- Records what phase 5's exit calls a private release candidate (PLAN.md §7): the two Mac apps of a tagged commit
of main, packaged in a release candidate's mode from a clean clone at a path that names no one and stripped of their
debug maps, with a first-launch ROM picker, and the licence, the third-party notices and release notes in every disk
image; checked against T7's first milestone checklist; handed out by the owner alone, never published; and which of
D6, sanitizer-clean replays and 60 fps it waits for. -->
# 0035: The private release candidate, shared by hand

- **Status:** proposed
- **Date:** 2026-11-02
- **Plan:** §7 phase 5 (its exit's "private release candidate"), §6 (T7's milestone checklist), §4.3 (the
  first-launch ROM setup), D6, D11
- **Upstream:** `41f42ca1af` (upstream's of 2026-10-31), as Frigate's `main` (`e936696a97`) has it
- **Note (2026-11-02):** proposed for the owner's decision by a maintainer session, from a read-only scout of phase
  5's open exit items, beside ADR 0020 (D6), ADR 0033 (sanitizer-clean replays) and ADR 0034 (60 fps with headroom).
  Nothing it describes is built yet. Each numbered decision is a recommended default that is the owner's to confirm,
  and [Alternatives](#alternatives) gives the other choices and what each costs. It stays proposed until the owner
  reviews it; the change that records that review sets the status line, and makes the changes that
  [Consequences](#consequences) lists for an accepted ADR. Lines cited are `main`'s at `e936696a97` unless a branch
  or another commit is named, and "inferred" marks what was reasoned rather than read.

## Context

Phase 5's exit is "the story can be finished on Black 2 and White 2; a private release candidate"
(`port/docs/PLAN.md:464`), and "release candidate" appears in no other document of Frigate's. Among the phase's items
is "an ad-hoc signed `.app` in a DMG, with "Open Anyway" instructions" (`PLAN.md:459-462`). The repository is public
(`PLAN.md:103-105`), and so is every commit a release candidate (RC) could be built from: "private" can only say who
gets the build and how. What there is, and what is missing:

- **The build, for its builder's own Macs.** The distribution build makes one universal app per game, arm64 and
  x86_64, for macOS 11 and later, with SDL3's pinned release linked into the program
  ([ADR 0015](0015-universal-macos-app-for-macos-11.md)), signed ad hoc and never notarized
  ([ADR 0003](0003-ad-hoc-signing-no-apple-developer-account.md)). `port/tools/package_macos.sh --distribution`
  (`--version w2_us` for White 2) puts it in a disk image that holds the app and a link to `/Applications`
  (`port/tools/package_macos.sh:211-214`), named after the app's short version (`:185-188`), and checks that the
  image holds no ROM, save, NARC or SDAT file (`:262-263`). CI makes Black 2's on four runners, in a job that the
  required check `Frigate CI` waits for, and keeps every app it makes there (`.github/workflows/port-ci.yml:151-160`,
  `:219-223`, `:312-315`); only a local command makes White 2's.
- **Nothing is handed out today.** `port/README.md:254-255` says that the port's compiled game code is never uploaded
  or distributed, and `package_macos.sh:11-12` and `port-ci.yml:18-19` repeat it, citing port/CLAUDE.md, which says
  only that melonDS is never shipped (`port/CLAUDE.md:220`). ADR 0015 left downloads to the owner and put a
  first-launch ROM picker before any (ADR 0015:105-108), and STATUS.md left both as the owner's decisions
  (`port/docs/STATUS.md:6185-6187`). The program is compiled from the decomp's C (`port/CLAUDE.md:216`) without a
  ROM, as CI, which has none, compiles it (`port-ci.yml:4-5`), so it holds the game's code and the tables compiled
  into it (`port/docs/MODDING.md:276-277`), and the licence that covers it grants no rights to Nintendo's data or
  trademarks (`.github/README.md:101-102`): whether to hand it to anyone, and to whom, is a risk that only the owner
  can weigh.
- **Giving the ROM takes a terminal.** The program takes `--rom`, then `FRIGATE_ROM`, then the build checkout's
  `orig/` (`port/hal/rom/rom.c:499-517`); opened from the Finder with none of them, it says why in an alert and quits
  (`rom.c:519-541`, `port/app/main.c:509-511`). PLAN.md plans "first-launch ROM setup" in `port/app` (`PLAN.md:136`,
  and the app shell's "first-launch setup", `:123`), and
  [ADR 0010](0010-macos-app-as-a-signed-copy-of-the-program.md) named the picker, `SDL_ShowOpenFileDialog`, as the
  fix that belongs to the settings and paths work (ADR 0010:69-71). No settings file is read yet
  (`port/app/gamepad.h:3`).
- **The app names its builder's home folder** in at least three ways: the development ROM's path compiled into
  `rom.c` (`port/cmake/rom.cmake:145-146`); the source path that `__FILE__` passes to `GFL_HeapAllocate`
  (`src/system/myitem_savedata.c:1219`), an absolute path in a development build (read in a local build tree's
  `build.ninja`, not on `main`); and every object's path in each slice's debug map (ADR 0010:79-81, ADR 0015:139-142,
  `port/README.md:346-352`). Both ADRs left a build "made from a neutral path, and stripped" to the owner. A strip
  removes the debug map but not the first two, which are strings in the program, nor any path that SDL3's own
  sources, compiled in the build tree, put in it (inferred).
- **No licence or release notes travel with it, and the notices have only begun.** The image holds the app alone; the
  About window and `--help` give the description and the not-affiliated line (`port/app/macos/Info.plist.in:64-65`,
  `port/app/main.c:194-197`), not the licence. Every program links minicoro, public domain or MIT-0, whose header
  takes some of its assembly from LuaCoco under an MIT notice that "shall be included in all copies or substantial
  portions" (`minicoro.h:235-237`, and `:631-657` inside the `#if` of `:629`, read in a copy fetched at `02dad0f8b7`,
  the commit `port/cmake/os.cmake:10-12` pins). The distribution build links SDL3's pinned release, whose zlib licence
  asks no acknowledgment in a binary (read in Homebrew's copy of release 3.4.18, the one `port/CMakeLists.txt:188-190`
  pins); whether code of other projects that SDL3's sources carry reaches the program, and asks for a notice, is
  unread. Verification v2's part 2
  ([ADR 0031](0031-per-game-frame-digests-against-a-content-addressed-oracle-cache.md), ADR 0031:154-159), in review
  on the branch `port-digest-stream` (`8cc7d9d20a`), compiles xxHash into both programs and begins the notices: its
  `port/cmake/app.cmake` writes `THIRD-PARTY-NOTICES.txt` beside the program at configure time from the fetched
  archive's `LICENSE` (the branch's `:29-40`), `make_app.sh --notices` copies it into `Contents/Resources`,
  `check_app.sh` and `package_macos.sh` fail an app or image without it, `--help` ends with a line naming it, and the
  test `app_notices` holds that the file ends with xxHash's licence byte for byte (the branch's
  `port/tests/app/check_notices.cmake:27-31`). It names xxHash alone. ADR 0003 puts the "Open Anyway" steps in
  release notes (ADR 0003:20-21, :45-46); only `port/README.md:354-367` has them.
- **T7 has no checklist, and a milestone no definition.** T7 is "the app driven by scripted input with screenshots,
  against each milestone's checklist", run at each milestone (`PLAN.md:396`, `port/CLAUDE.md:247`), and no checklist
  has been written. GLOSSARY.md doesn't define a milestone (grep); ADR 0033 reads phase 5's, for T5, as each pull
  request that ends a leg of the story.
- **What has run.** With the ROM, macOS 26 on an M2 (ADR 0015:25), the x86_64 slice under Rosetta; on CI, the tests
  that need no ROM on macOS 15 and 26, Apple Silicon and Intel, on Black 2's app. No macOS older than 15 has run the
  app, and none older than 26 has played the game (`port/README.md:265-266`, `:279-283`; ADR 0015:102-104,
  :136-138). The game hasn't run with a ROM on Windows (`port/README.md:112-113`); on Linux the Spanish Black 2 dump
  has reached the first logo screen (`port/docs/STATUS.md:1402`, `:1410-1412`). Neither has a package.
- **The version** is `0.0.N`, N the count of commits built (`port/app/macos/make_app.sh:353-363`), which counts the
  decomp's whole history: 4,732 at `e936696a97` (`git rev-list --count`).
- **Phase 5's other items** include one executable for both versions (D6, "Recommended, from phase 5",
  `PLAN.md:50`), sanitizer-clean replays and 60 fps with headroom (`:459-460`); the exit names none of them. ADRs
  0020, 0033 and 0034 propose what each means and what each asks of an RC; this one says which of them the RC waits
  for, and carries what they ask of it.

## Decision

Each decision is a recommended default, the owner's to confirm.

1. **What it is.** An RC is one commit of `main` and the two disk images made from it, Black 2's and White 2's, each
   holding one universal app as ADR 0015 makes it, signed ad hoc as ADR 0003 has it. It is private in how it travels,
   not in its source: the commit and its tag are public like the rest of the repository; the images are not. The
   first RC whose requirements (11) and checklist (8) pass, recorded in STATUS.md, meets phase 5's exit. An RC whose
   runs or checklist fail is handed to no one, and the fix is a new RC (10).
2. **Who gets it, and how.** The owner keeps the images, their dSYMs (5) and the dependencies' sources (6) on the
   owner's own machine, and gives an image by hand (AirDrop, a disk, or a link to a file in storage the owner
   controls, sent to one person) only to people the owner chooses, each told that it is a private test build, not to
   be passed on, that runs only with their own dump. No workflow or script uploads it, and it never goes to GitHub: no
   release, draft or published, no Actions artifact, and no link to a copy in the repository, its issues or its pull
   requests; nothing about it goes to the decomp's repository either
   ([ADR 0002](0002-solo-project-no-upstream-posts.md)). A recipient reports to the owner directly, with the app's
   log, which names the recipient's own folders (the ROM's path, `rom.c:549`) and so stays with the owner; what needs
   work becomes an issue in the owner's words, with no ROM, save or captured frame (`port/CONTRIBUTING.md:108-111`)
   and nothing that names the recipient.
3. **What it holds.** Each image holds the app, the link to `/Applications`, the release notes (7), the licence and
   the third-party notices (6); the app holds the licence and the notices in `Contents/Resources` as well. No file in
   either is a ROM's or made from one, as `package_macos.sh` checks (`:262-263`); the program is the decomp's C and
   Frigate's, built without a ROM. It is macOS only: Windows and Linux builds are not part of it, CI keeps building
   and testing both (T1, `port-ci.yml:35-41`, `:90-149`), and the notes point their players to building from source.
   The apps are the US builds (IREO, IRDO); the Spanish Black 2 build of
   [ADR 0025](0025-a-spanish-rom-by-a-manifest-of-its-own.md) is not in it, since its manifest is written from one
   player's dump and isn't a shared input (ADR 0025:46-48, `port/.gitignore:9-10`).
4. **The ROM: never bundled, given through a first-launch picker.** Nothing downloads a ROM, and nothing in the image
   or the notes says where to get one. A dump that `--rom` or `FRIGATE_ROM` names, or the development path's file when
   it is there, is used as today, and stops the program with its reason when refused. When none is given and the
   program runs from the app with a window (the case in which it shows its alert today, `port/app/main.c:509-511`), it
   opens the dump it remembers, and when there is none, or it is gone or refused, it shows the system's open panel
   (`SDL_ShowOpenFileDialog`) on the host's own stack, after the window opens and before the game starts, where it
   checks the ROM today (`main.c:652-656`, `:693-695`); SDL3's dialog answers through a callback, so the program waits
   for it with the window's events running (inferred). It checks the chosen file as it checks any (`port_rom_open`,
   `rom.c:487-497`). A refused file gets its reason (the check names its game, version and region, `rom.c:356-401`)
   and the panel again; a cancelled panel ends the program, as the alert does today. An accepted dump's path is
   remembered, by game code, in a small file in the save directory (`--save-dir`, else Frigate's data folder,
   `port/README.md:226-229`), which phase 6's settings file (TOML, `PLAN.md:335`) takes over. Only that case reads or
   writes it: a run from a terminal or without a window (`--headless`, the tests, the oracle) never shows the panel or
   opens a remembered dump, and stops with its message, as today (`rom.c:536-541`); and every test runs with a save
   directory of its own (`port/CMakeLists.txt:457`), so no remembered dump reaches one, as ADR 0020 asks. The picker
   is port code in a pull request of its own, with `--help`'s licence line (6), landed before the RC is made. If ADR
   0020's one program comes first, the picker is its core's, shown where the alert is, and a dump of the other version
   starts the program again with it, as ADR 0020 has it.
5. **Built from a neutral path, and stripped, in a release candidate's mode.** The RC's images are made by
   `package_macos.sh --distribution` and `--distribution --version w2_us` in a mode for an RC, in a fresh, full clone
   of the public repository at the RC's tag (not a shallow one: the build counts its commits, `make_app.sh:354`), at a
   path that names no one (such as `/private/tmp/frigate-rc`) and with no `orig/`: its development ROM path, its
   `__FILE__` strings and its debug maps then name no one, its development path finds no ROM on any Mac, the
   builder's included, and `PortSourceRevision` has no `-dirty`. In that mode alone, `make_app.sh` keeps the program's
   debug information aside as a dSYM (`dsymutil`) and removes the debug map from the app's copy of the program
   (`strip -S`; the copy is made at `make_app.sh:106`) before it signs the app, which it signs after the last change
   to any binary (ADR 0010:40-42), while the build tree's program, which the tests run, keeps its map; and
   `package_macos.sh` refuses to run unless the commit carries an RC's tag (10) and the tree is clean, names the image
   after the tag, and refuses an image that holds a dSYM or whose program holds the builder's home folder (`$HOME`) in
   any string, as it refuses a ROM file in it today. Every other build is made in its builder's home, CI's in its
   runner's (inferred), where those checks would refuse it: CI's distribution job (`port-ci.yml:222-223`) and a
   developer's own images package as today. `strip -S` leaves what the program reads of itself at run time
   (inferred): the names that trap and crash reports print (`dladdr`) and the globals that `--run-dump` finds
   (`dlsym`), `port/hal/host/host_posix.c:236-267`. The owner keeps the dSYMs to read crash reports with.
6. **The licence and the notices, in the app and the image.** `LICENSE` (the GNU GPL v3), and the program's
   `THIRD-PARTY-NOTICES.txt`, the file that v2's part 2 begins (or, if part 2 hasn't landed, that the RC's change
   begins in the same way). Written at configure time from the archives and sources the build fetched, so that it
   can't drift from the pins, it holds xxHash's notice, minicoro's licence with the MIT notice its header carries,
   SDL3's `LICENSE.txt` where the program links the pinned SDL3, with the notices of any other project's code that
   its static build compiles in (the change lists them from the pinned release's sources), and that of any other code
   the program comes to link; `app_notices` then holds each notice byte for byte, not the last alone. The About window
   and `--help` gain a line beside part 2's: the contributors' copyright, that the program is free software under the
   GNU GPL v3 with no warranty, where `LICENSE` and the notices are, and where the source is, which is what the GPL
   calls Appropriate Legal Notices (`LICENSE:103-110`). `--help`, compiled into the program, names the repository;
   the About window, whose text `make_app.sh` writes at each packaging, names the RC's tag as well, so that no
   revision is compiled into the program. The GPL has whoever conveys object code give its Corresponding Source with
   it, or say next to it where it is and keep it available (`LICENSE:245-286`); the default reads that as: the notes
   name the public tag, whose tree pins every dependency by URL and SHA-256 or by commit
   (`port/CMakeLists.txt:188-190`, `port/cmake/os.cmake:10-12`, `port/tools/oracle/xxhash.cmake:15-17`), and the
   owner keeps those dependencies' sources with the RC. That is a reading of the licence, not legal advice.
7. **Release notes.** One document, `port/docs/release-notes.md`, newest RC first. An RC's section is written in the
   pull request whose merge is the RC's commit, so that the tag's tree holds it, and each image carries it as text. It
   can't hold the results of runs made on that commit, nor those of the checklist, which runs on the images (8): it
   names what the RC is held to and where the record is, the RC's STATUS.md entry, which follows the runs. It says:
   what Frigate is and that it is not affiliated, in the README's words; the version, tag and commit; that it needs
   the player's own dump, and which dumps the check accepts; installing (onto Applications, opened from there) and
   Gatekeeper's "Open Anyway" steps for macOS 15 and later and for 11 to 14 (`port/README.md:354-367`), as ADR 0003
   asks; the first launch's picker; where the saves and the log are, that the log is emptied at each launch
   (`port/app/message_log.c:91`), that a save is melonDS's and a cartridge's format (`port/README.md:226-230`), and
   to keep a copy of any save brought in; the controls; the Macs and slices the RC is held on and those it isn't (9,
   11); the known limits: local wireless and Wi-Fi don't work, which phase 7 builds, with what the player sees on
   choosing them as ADR 0033's refusal check finds it; a trainer's AI that can choose otherwise than a DS's where the
   DS's CPU time decides ([ADR 0023](0023-oracle-routes-part-where-the-ds-cpu-time-decides.md)); the window's pacing
   without the display's vertical sync, as ADR 0034 has it (on a 60 Hz display a frame shows for two refreshes about
   every 5.75 s, and a present can tear where the window bypasses the system's compositor); no mods or widescreen
   before phase 6; no Windows or Linux build; how to report; and the licence, the notices and where the source is.
8. **T7's checklist.** A document of its own, `port/docs/playtests.md`, one section per milestone, as
   `port/docs/sanitizers.md` is T5's. For T7 a milestone is a phase's exit: the RC is phase 5's, and its section is
   the first; a story leg's pull request, which ADR 0033 counts as a milestone for T5, owes no T7. The checklist runs
   on each game's image, ADR 0033's windowed strict runs are recorded beside it, and every item is recorded in the
   RC's STATUS.md entry with the Mac and macOS it ran on, as text and hashes: no screenshot, save or log is committed
   (`port/CLAUDE.md:217-219`).
   - The image passes `package_macos.sh`'s checks in the RC's mode and `check_app.sh` (both slices, macOS 11.0,
     signed ad hoc, no hardened runtime), holds its notes, licence and notices, and the About window and `--help`
     show the licence line.
   - On a Mac account that has never built Frigate, a copy that came by download or AirDrop is blocked once and opens
     from `/Applications` through "Open Anyway", as the notes say.
   - The picker refuses the other game's dump and a file that is no ROM, each with its reason; the right dump starts
     the game; the next launch doesn't ask; a moved dump is asked for again; a cancelled panel quits.
   - Scripted: the installed app's program, with `--rom`, `--input`, `--run-dir`, `--screenshot` and a scratch
     `--save-dir`, plays `first_save` and then `starter_save` (`port/tests/oracle/`: New Game to the first save, then
     CONTINUE to the starter's, `port/cmake/app.cmake:239-242`) as each slice, the x86_64 one under Rosetta, and
     writes the same `frames.tsv` (every frame's screen hashes, `port/app/run_dir.c:498-500`) as the development
     build of the same commit given the same options. The screenshots are looked at and stay on the machine.
   - By hand, in the window: New Game to the first save, a wild battle and a trainer battle, the field menu's apps,
     SAVE, quit, open again and CONTINUE, with a gamepad and with the keyboard
     ([ADR 0009](0009-gamepads-by-position-one-pad-at-a-time.md)).
   - A stop that only the log explains: a save file of the wrong size in the save folder closes the app, and its log
     in `~/Library/Logs/Frigate` says why (`port/README.md:339-344`).
9. **What it claims about Macs** is what ran, as ADR 0015 holds the documents to (ADR 0015:102-104): built for macOS
   11 and later, Apple Silicon and Intel; the checklist played on the Macs and macOS versions it names (macOS 26 on an
   M2 today), the x86_64 slice's scripted part under Rosetta; CI's tests without the ROM on macOS 15 and 26, Apple
   Silicon and Intel, on Black 2's app; macOS 11 to 14, and an Intel Mac with the ROM, untested. A recipient's report
   of another Mac or macOS goes into STATUS.md and the next RC's notes.
10. **The version: `0.1.0-rc.1`.** An annotated tag, `frigate-v0.1.0-rc.1`, on the RC's commit, pushed to this
    repository on the owner's word, with no release and no file attached. It is the one thing besides topic branches
    and pull requests that Frigate's maintainers publish (`port/CLAUDE.md:89-90`, which the acceptance amends), and no
    ruleset covers it (`port/tools/main-ruleset.json:6`). The app's `CFBundleShortVersionString` is `0.1.0` (the key
    takes numbers and points only, inferred from Apple's format), `CFBundleVersion` stays the commit count, which only
    grows along `main` (`port/app/macos/Info.plist.in:16-18`), and `PortSourceRevision` is the tag, as `git describe`
    gives it there (`make_app.sh:355`). The images are `Frigate Black 2 0.1.0-rc.1.dmg` and
    `Frigate White 2 0.1.0-rc.1.dmg`, named after the tag in the RC's mode (5), since an image takes the short
    version's name today (`package_macos.sh:185-188`); with ADR 0020's one app, `Frigate 0.1.0-rc.1.dmg`. The next
    RC of the same version is `rc.2`; a build at no such tag keeps `0.0.N`. What follows the RCs, and its number, is
    the owner's later decision.
11. **What it waits for.** The RC's commit is a merge on `main` whose pull request passed the required checks and the
    exact-head verification (`port/docs/REPOSITORY-POLICY.md:28-43`). On that commit, in both games and on demand:
    the story's routes to the game's end, each a regression test (the exit's first half, `PLAN.md:459-464`); T4, as
    PLAN.md and port/CLAUDE.md ask when a milestone closes (`PLAN.md:393`, `port/CLAUDE.md:245`), through whichever
    of ADR 0031's tiers have landed by then; T5, as ADR 0033 defines sanitizer-clean replays, with what it asks of an
    RC: its whole set, the x86_64 slice's run under Rosetta, the wireless refusal check, the renderer's
    ThreadSanitizer runs and the windowed strict runs, recorded as it asks; 60 fps with headroom, as ADR 0034 defines
    it, on the machine it holds and with the slice it reports; and the checklist (8). The save lab's stations (ADR
    0032, being written) reach the RC only through T5's set, as coverage beside the routes and never as the story's
    evidence. It doesn't wait for D6: two apps are acceptable, as ADR 0020 proposes too. If ADR 0020's one app has
    landed by the RC's commit, the RC ships it instead, one image, and the checklist runs on it. Where ADR 0033 or
    0034 is still proposed at the RC, its proposed defaults hold, and the notes say so.

## Alternatives

- **Who gets it (2).**
  - *No one but the owner:* the RC is the tag and the owner's own images, and anyone else builds from source, ADR
    0003's first-class way. No compiled game code leaves the owner's Macs and the README stays as it is, but the app
    is never tried on a Mac its builder didn't set up, Gatekeeper's path runs only on a copy the owner downloads to
    themself, and every tester needs the quick start's toolchain (`port/README.md:37-43`) and a build of their own,
    for their own Mac (`:262-263`).
  - *A GitHub release here:* PLAN.md expected the port to need "its own issues and releases" (`PLAN.md:100`), but a
    release on a public repository puts the compiled game in everyone's reach, the widest exposure of all the choices
    and the opposite of private. A draft release is shown only to those who can push (inferred), yet it sits one
    click from public, beside the source.
  - *CI's images:* the distribution job already makes Black 2's on four runners; uploading them would offer a build
    of every commit from a public repository's runs (inferred), made without the ROM or the checklist, and still none
    of White 2.
  - *A private repository or store for the images:* keeps them from public view, at the cost of a second place to
    keep in step with this one; it hands the program out as much as 2 does.
- **One image holding both apps (1, 3)**, which ADR 0020 leaves to this ADR: one file to hand out and one copy of the
  notes, for an image about twice the size (13 MB for one app, ADR 0015:128-129; inferred) of which a recipient who
  owns one game needs half, and a change to `package_macos.sh`, which makes an image per app (`:187-188`). ADR 0020's
  one app gives one image anyway.
- **Windows and Linux in it (3):** they need packages that don't exist (the Windows program loads MSYS2's `SDL3.dll`,
  `port/README.md:131-132`), the story played on each with a ROM, the checklist run on each, and on Windows a program
  that ADR 0003 leaves unsigned (ADR 0003:48), which the system warns about (inferred). The phase's exit closes on
  macOS (`PLAN.md:410`); a later RC can add them.
- **The ROM (4).**
  - *The terminal's ways, in the notes* (`open -n -a … --args --rom`, `--env FRIGATE_ROM=`,
    `port/README.md:323-337`): no code, but every tester needs Terminal, and a wrong path ends in an alert.
  - *A document type,* so that a dump dropped on the app, or opened with it, starts the game (inferred): a good
    companion to the picker, but a double-click alone still finds nothing.
  - *A copy of the dump in Frigate's data folder* at the first launch: later launches never depend on where the dump
    was, and macOS never asks again about a protected folder (inferred), for a second copy of the dump on the disk
    that the player didn't ask for.
  - *A fixed place for the dump,* beside the saves: no panel, but the player must find
    `~/Library/Application Support`, which the Finder hides.
- **Neutral and stripped (5).**
  - *A strip alone, in the owner's checkout:* removes the debug map and leaves the development ROM path and the
    `__FILE__` string, which name the home folder.
  - *Compiler options instead of a clean clone* (`-ffile-prefix-map`, and no development ROM path in a distribution
    build): any checkout would make a neutral app, but it is a change to the build that must reach SDL3's own build
    too, and a distribution tree's tests that rely on the development path would need `--rom` (inferred). It can
    follow the first RC; the strings check holds either way.
  - *The strip and the dSYM in every distribution build, CI's included:* CI would try them on the real program at
    every pull request, for `dsymutil`'s time in a job that `Frigate CI` waits for (inferred), where the owner asked
    that long checks not block pull requests (ADR 0031:170-172); the mode's own tests run on stand-in programs
    instead, as `test_make_app.sh`'s do (`port/cmake/macos_app.cmake:90-96`).
  - *A full strip:* a smaller program whose trap and crash reports, in a recipient's log, no longer name its static
    functions; a Mach-O program keeps its globals in its export trie (`port/hal/host/host_posix.c:258-259`), so the
    others keep their names (inferred).
- **The notices (6).**
  - *A committed notices file:* readable in the repository, but kept in step with every pin by hand; made from the
    fetched archives, it can't drift.
  - *A second file beside part 2's:* two lists of what the program carries, which the app, the image and their
    checks would each have to know.
  - *The source in every image* (archives of the tag and of the dependencies): the plainest way to meet the GPL, for
    images many times larger (inferred).
  - *None:* SDL3's licence asks no notice of a binary, but the MIT notice in minicoro's header does, and so does
    xxHash's BSD-2-Clause licence.
- **The notes (7).**
  - *Only in the owner's message to each recipient:* nothing is recorded, and nothing next to the program says where
    its source is. Committing them shows the public that an RC exists, which its tag shows anyway.
  - *The results in the image's notes:* the images would be packaged again after the checklist that ran on them, so
    the copies handed out would be ones it never ran on.
- **The checklist (8).**
  - *All scripted:* the program's options already drive the game, but Gatekeeper, the open panel and a gamepad need
    a person or interface automation (inferred).
  - *The whole story by hand:* the routes already hold it at T3 and T4; the checklist covers what they can't, the
    installed app, the Mac's prompts and a person's hands.
  - *T7 at each story leg, as ADR 0033 reads a milestone:* the installed app tried on every leg, for an hour or two
    by hand a leg (inferred), for what the routes already hold.
  - *None:* PLAN.md and port/CLAUDE.md hold T7 to one.
- **The Macs it claims (9).**
  - *Wait for macOS 11 to 14, or an Intel Mac with the ROM:* nothing here can run them (no virtual machine, and the
    macos-14 runners are deprecated, ADR 0015:59-62); the RC would wait on a recipient's Mac anyway.
  - *Raise the floor to macOS 15,* to claim only what CI ran: an ADR that amends 0015, for fewer Macs.
- **The version (10).**
  - *`0.0.N`, as today:* no change, but N counts the decomp's history and grows with every sync, and nothing tells
    an RC from a development build.
  - *`1.0.0-rc.1`:* reads as finished before phase 6's mod platform and widescreen and phase 7's multiplayer.
  - *No tag, or one kept local:* the commit's hash names the source too, `main`'s rules forbid deleting or rewriting
    it (`port/docs/REPOSITORY-POLICY.md:13`), and `port/CLAUDE.md:89-90` needs no exception; but a hash is harder to
    give anyone told where the source is.
  - *A ruleset for the tags:* `refs/tags/frigate-v*` kept from being moved or deleted, as `main` is, at the cost of
    a third ruleset that the owner applies and REPOSITORY-POLICY.md records (`:104-121`).
- **What it waits for (11).**
  - *D6 as well:* one app and one image, the ROM choosing the game, at the cost of putting ADR 0020's stages 2 to 4
    before the RC, its third the riskiest (about 8 to 12 hours of machine time by that ADR's estimates; inferred).
  - *Neither T5 nor 60 fps:* an earlier RC whose notes say so; a recipient may meet a memory bug or a slow frame that
    the runs would have found first.
  - *Black 2 alone for T5 and timing:* half the machine time, but 57 files of `src/`, `lib/` and `include/` hold a
    `#if` on `BLACK2` or `WHITE2` (74 such lines, and 90 files name either; grep), and the two games' routes differ
    too.

## Consequences

- **The work, in order:** the ROM picker with `--help`'s licence line, C that the programs compile, so a story pull
  request whose T4 evidence predates it runs T4 again (inferred), landed where it restarts no story pull request's
  final verification; then the packaging: the notices beyond xxHash's, the RC's mode (the dSYM and the strip, the
  checks, the image's name and the version from the tag) and the About window's licence line, in `make_app.sh`,
  `package_macos.sh`, `macos_app.cmake` and `app.cmake`, with their tests (`test_make_app.sh`,
  `test_package_macos.sh`, `app_notices`), which no oracle file names (grep of `port/tools/oracle`,
  `port/cmake/oracle.cmake` and `port/tests/oracle`); then `port/docs/release-notes.md` and `port/docs/playtests.md`,
  each with its line in `port/docs/README.md` (`port/CLAUDE.md:61-63`); and the RC itself, after the story's last
  leg.
- **Each RC costs** two distribution builds (about 700 MB of build directory each, ADR 0015:128-129), the oracle
  suite, the sanitized routes and the timing runs in both games, all on demand (the owner declined scheduled runs on
  2026-11-01, ADR 0031:170-173), and an hour or two of the checklist by hand (inferred).
- **Per-PR checks stay as they are.** The RC's mode runs only when the owner packages an RC: CI's distribution job
  and every other build package as today, and nothing else here runs per pull request.
- **When the owner accepts it,** that change also rewrites `port/README.md:254-255` and `:346-352`,
  `.github/workflows/port-ci.yml:79-80`, and `port/tools/package_macos.sh:11-12` and `port-ci.yml:18-19`, which cite
  for "never uploaded" a rule that port/CLAUDE.md doesn't hold, to say that no workflow or script uploads the app and
  that only the owner hands out an RC; adds the owner's RC tags to what `port/CLAUDE.md:89-90` lets maintainers
  publish; sets ADR 0010's and ADR 0015's status lines to "accepted; amended by 0035" (the picker changes how the app
  finds the ROM, and an RC is the distribution that 0015 left open); links phase 5's exit in PLAN.md §7 here; and adds
  "release candidate", and "milestone" as decision 8 reads it, to GLOSSARY.md. The picker's own pull request rewrites
  the README's "The ROM" and "When the app stops" (`port/README.md:323-344`).
- **To watch:** a recipient passing an image on, which nothing technical prevents; macOS asking again about a ROM in
  a protected folder after each new RC, since an ad hoc signature changes with every build (inferred); the strings
  check when a dependency is added; an app opened from inside the image, which macOS runs from a read-only copy
  elsewhere (inferred), and which the notes' "onto Applications" avoids; and `git describe` naming every later build
  after the tag once a clone has fetched it (`frigate-v0.1.0-rc.1-N-g…` in `PortSourceRevision`, `make_app.sh:355`;
  inferred).
- **Moddability and widescreen stay open.** Nothing is written inside the signed app: the remembered ROM, the saves
  and the settings to come live in Frigate's data folder, so phase 6's settings file takes over the picker's file and
  mods never touch the bundle, and the app keeps no hardened runtime, so the native plugins to come can load (ADR
  0003:22-24). Phase 6's exit, "each sample mod working from a fresh install" (`PLAN.md:472`), becomes a section of
  the same T7 document. The picker runs before the game starts and changes no presentation, and nothing here touches
  the renderers or the game's code, so
  [ADR 0030](0030-widescreen-by-an-engine-side-hor-plus-scale-with-native-readbacks.md)'s rules hold as they are.
- **Revisit** when the owner wants public downloads (a new ADR; ADR 0003's ad hoc signing still holds unless it
  changes), when ADR 0020's one app lands, when Windows or Linux play the story and get a package (the picker and the
  notices go there too), when Apple changes how an ad hoc app is first opened, or when the program links another
  dependency.
