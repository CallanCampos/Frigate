<!-- Records what phase 5's "sanitizer-clean replays" (T5) mean and how they are checked: the strict sanitized build's
routes, and its program alone on every story piece from the oracle's saves; what counts as clean and which exceptions
are declared; on which hosts and when, with no scheduled runs; and what a release candidate adds: wireless refused
without a report or a freeze, the x86_64 slice, the renderer's threads and the window. The runner comes in a later
pull request. -->
# 0033: Sanitizer-clean replays by the strict routes and the program alone from the oracle's saves, on demand

- **Status:** proposed
- **Date:** 2026-11-02
- **Plan:** §6 (T5), §7 phase 5 ("sanitizer-clean replays", and the exit's private release candidate)
- **Upstream:** `41f42ca1af` (upstream's of 2026-10-31), as Frigate's `main` (`e936696a97`) has it
- **Note (2026-11-02):** proposed for the owner's decision with the other open items of phase 5 (ADRs 0020, 0034 and
  0035), from a read-only scan of `main`; nothing it describes runs yet. Every decision below is a recommended
  default, the owner's to confirm. It stays proposed until the owner reviews it; the change that records that review
  sets the status line and, for an acceptance, makes in the same commit the changes that
  [Consequences](#consequences) lists. A later pull request (T5-A) implements it: a runner in `port/tools` and
  `main`'s first baseline. Lines cited are `main`'s at `e936696a97` unless a branch is named, and "inferred" marks
  what was reasoned rather than read.

## Context

Phase 5 lists "sanitizer-clean replays" among its items, and its exit is "the story can be finished on Black 2 and
White 2; a private release candidate" (`port/docs/PLAN.md:459`, `:464`); a phase closes when its exit criteria pass on
macOS (`PLAN.md:410`, `port/CLAUDE.md:249-250`). T5 is "replays under ASan and UBSan, in the sanitized build", run
nightly, while CI's `sanitize` job runs the other tests sanitized on every commit, having no ROM (`PLAN.md:394`,
`port/CLAUDE.md:246`). [sanitizers.md](../sanitizers.md) reads the item as the build's routes run clean, the ctest
tests labelled `route` (`port/docs/sanitizers.md:5-7`, `:22-25`; `port/CMakeLists.txt:408-413`).

The build is [ADR 0014](0014-sanitized-build-game-heaps-poisoned-ds-isms-ignorelisted.md)'s. `PORT_SANITIZE` builds
every target but SDL3 with `-fsanitize=address,undefined -fno-sanitize-recover=all` and an ignorelist (ADR 0014:28-31;
`port/CMakeLists.txt:227-248`), and `PORT_SANITIZE_RECOVER` is a survey build for triage (`sanitizers.md:55-60`). A
report aborts the program on every host (`port/hal/os/sanitizers.c:11-13`, `:48-54`), and in this build ctest fails
every test whose output holds one, since no test may carry `WILL_FAIL` (`port/CMakeLists.txt:465-488`). The game's FND
heaps are poisoned block by block; the ignorelist holds only code right on every host, and a defect is fixed, never
listed (ADR 0014:32-39; `port/cmake/sanitizer_ignorelist.txt:1-5`). The ledger holds 25 reports, each fixed,
ignorelisted with its reason or, for H1, let through as the DS's read (`sanitizers.md:183-299`). One entry has joined
the ignorelist since ADR 0014: R13's `VM_CallCommand`, on 2026-10-26 (`9a8d4f2a0c`; `sanitizer_ignorelist.txt:29-34`;
`port/docs/decomp-findings.md:76-84`).

What the documents leave open, for the owner's word to settle:

- **What runs.** The routes that need no oracle go no further than the Aspertia City lookout, where
  `replay_starter_save` ends (`port/cmake/app.cmake:239-242`; inferred from every such route's description): 17 in
  Black 2 and 15 in White 2 without the timing routes, which the strict tree doesn't register
  (`port/cmake/frame_times.cmake:24`; counted so in the `STATUS.md:58-59` of the branch `port-terrell-cut`, which adds
  no such route), and 19 and 16 with them at PR 67 (`port/docs/STATUS.md:642-643`). Every sequence of phases 4 and 5
  starts from a save that only the oracle makes, so it has no reference and no port-alone ctest test
  (`port/cmake/oracle.cmake:149-175`, `:187-198`); the comparison sequences' committed references are the opening's,
  the title's and aspertia's alone (`port/tests/oracle/*.tsv`). In the strict tree those sequences run only inside
  `oracle_<seq>`, beside melonDS. Since phase 4 the practice has been the strict program alone on each piece's input
  from its save, in batches by hand (`STATUS.md:2259-2262`, `:2079-2082`, `:1848-1851`, `:1915-1918`); no tool in
  `port/tools` runs them, and the batches don't name their runtime options. Run as item 1 below has them, the tables
  give 16 such cases and 147,920 port frames per version today (counted from `SEQUENCES`, `SAVES` and `ROUND_TRIPS`).
- **The compiler.** The build takes the compiler CMake finds, Apple clang on a Mac, which emits no function check for C
  and has no LeakSanitizer; upstream clang has both (`sanitizers.md:43-46`, `:163-164`;
  `port/docs/lessons.md:760-763`). R3 and R4 are marked as upstream clang's alone, and R12 to R14 are calls through
  mismatched function types, the same check: five of the sixteen R reports (`sanitizers.md:187-188`, `:250-252`).
- **The runtime's options.** The program's one default is `abort_on_error=1` (`sanitizers.c:48-54`). The leak check
  needs `detect_leaks=1` on macOS, and a frame used after it returned needs `detect_stack_use_after_return=1`, which
  only `os_all_fake_stacks` sets (`sanitizers.md:97-98`, `:165-166`; `port/cmake/os.cmake:80-86`); no run that
  STATUS.md records names it.
- **Hosts and cadence.** No document says which hosts count. The owner declined scheduled nightly and weekly runs on
  2026-11-01 and asked that long checks be optional where they would block pull requests
  ([ADR 0031](0031-per-game-frame-digests-against-a-content-addressed-oracle-cache.md), `:170-173`), so no run stands
  behind PLAN.md's "nightly".
- **Where it stands.** The last whole strict suite on record is 2026-10-24's, with the oracle: Black 2's 24 routes in
  3,210 s and White 2's 22 in 3,162 s, no report (`STATUS.md:3511-3518`), built by Apple clang 21
  (`STATUS.md:3503-3505`), so without the function check and the leak check. The batches by hand since then used
  Homebrew's LLVM (`STATUS.md:2079`, `:1830`); the last, Floccesy to Route 20's save and its continuation, is
  2026-10-28's (`STATUS.md:1848-1851`, `:1915-1918`). Of the six merges from PR 67's sync on (PRs 67, 70, 73, 72, 74
  and 71, in that order, `STATUS.md:40-43`), five record T5 as not run and one ran the café's 2,200 frames alone
  (`STATUS.md:644-645`, `:579`, `:430-434`, `:453-454`, `:347`, `:224`, `:138-139`). Nothing in the merge gate can run
  T5: CI has no ROM (`.github/workflows/port-ci.yml:234-289`), and the exact-head verification builds plain
  (`port/tools/sync_upstream.sh:581-590`).

A release candidate raises four more questions about what clean covers:

- **Wireless.** The pointer cast guard's 66 real sites are all local wireless and Wi-Fi: R21 (3, the Union Room's rule
  menu), R22 (1, `NetDevWifi_StartMatch`, which no game C calls), R23 (50, DWC and the Wi-Fi libraries) and R24 (12,
  WM and Download Play) (`port/tools/ptrcast_baseline.tsv`, tallied; `STATUS.md:141-143`), a group that
  [ADR 0019](0019-pointer-cast-guard-against-a-classified-baseline.md) gives phase 7 (`:97`, `:136-137`). On a host WM
  never starts: its PXI tag has no ARM7 service (`port/hal/pxi/arm7.c:7-9`; `port/tests/pxi/test_pxi.c:159`), so
  `WmInitCore` answers `WM_ERRCODE_WM_DISABLE` (`lib/nitro/src/wm/wm_system.c:76-79`), unless one of the checks
  before it refuses the buffer, among them the one that tests its address, cut to 32 bits, against the DS's video
  memory, 0x05000000 to 0x07000800 (`wm_system.c:62`; `lib/nitro/src/os/os_address_check.c:15-16`), whose answer
  depends on where the host put the buffer (`ptrcast_baseline.tsv:89`, whose note puts it at one buffer in 256; the
  range is about one 128th of what a cut address can be, inferred). The game's wireless helper starts WM this way
  (`src/ov030/wih.c:999`; `lib/nitro/src/wm/wm_standard.c:105-122`). What a player sees on choosing wireless is
  unknown: the save lab's sweep found the Union Room and its colosseums stopping on the watchdog, entered from minted
  saves without a link (`STATUS.md:520-521`). The watchdog ends a run with a frame that takes longer than its seconds
  of host time, the game spinning where the platform layer never runs (`port/app/main.c:322-325`); a run with a
  window has no watchdog unless asked (`main.c:515`), so a player would meet such a spin as a frozen game (inferred).
- **The branch `port-64bit-sweep`** (`c516377abd`, 2026-10-19, six commits on `a14364b6ba`; a branch of the clone with
  no copy among `origin`'s) carried wireless pointers as `uptr`. Two of its fixes are on main in no form and still
  apply: `startMatch` taking its first argument as `sptr` (R22, `include/gfl/net.h:140`) and `WM_ReadMPData`'s step
  through `uptr` (R24, `lib/nitro/src/wm/wm_sync.c:153`). It can't merge as it is: it predates ADR 0019's guard
  (ADR 0019:12-13), and it classed WM's PXI rows as handled, which main keeps real (`ptrcast_baseline.tsv:87-88`).
- **Threads.** The 3D renderer draws bands on worker threads, the game on one
  ([ADR 0016](0016-3d-frame-in-bands-on-worker-threads.md), `:48-55`). ThreadSanitizer can't be combined with ASan
  and is in no build; by-hand runs on `g3d_ds3d` and `g3d_ds3d_bands` reported nothing (ADR 0016:89-93).
- **The window.** The routes run `--headless` (`port/cmake/app.cmake:138`, `port/tools/oracle/oracle.py:964`),
  "without a window, without pacing and without a sound device" (`port/app/main.c:73`). The window
  (`port_present_open`, `port_present_poll`, `port_present_frame`: `port/app/present.h:10-19`) and the sound device
  (`port/audio/audio.c:131`) start only without it (`main.c:652-661`), and no test calls them: the tests call the
  presentation's screenshot alone.

## Decision

Each item is a recommended default, the owner's to confirm; [Alternatives](#alternatives) gives the other options and
their costs under the same numbers.

1. **What runs: the T5 set.** Phase 5's sanitizer-clean replays are the T5 set run clean (item 4) by the strict build
   of each version, on one head:
   - every ctest test labelled `route` that needs no oracle (`ctest -L route -LE oracle`): the replays, the port-alone
     checks of the oracle's hashes, the smoke test and the others so labelled;
   - the program alone, as `sanitizers.md:26-27` runs a route by hand, from the oracle's saves: every sequence that has
     no reference for the version, from its save (or a blank one) with its own input file and frames (`SEQUENCES`,
     `port/tools/oracle/oracle.py:368`), and both of the port's legs of every round trip, the make leg from its save's
     start and the continue leg from the save the make leg left (`ROUND_TRIPS`, `oracle.py:1441-1455`). Two cases
     with the same start and input file run once, at the larger frame count, the shorter run being the start of the
     longer (inferred: the program is deterministic for one start, input and clock); today that folds the `catch` and
     `floccesy` sequences into their round trips' make legs. A sequence or round trip added to the tables joins the
     set with no other edit;
   - the save lab's stations, once they land, each run by the program alone from its minted save, never in its
     comparison with melonDS, whatever labels their tests carry (ADR 0032, being written, defines a station);
   - item 9's refusal check, once written;
   - in item 7's whole runs, the save lab's map sweep in the strict build, once it lands.

   T5 compares nothing with the oracle and never runs melonDS: the oracle's comparisons (T4) are held on the plain
   build, and a sanitized comparison, as on 2026-10-24 or as ADR 0031's sanitized digests would be (ADR 0031:170-173),
   is an option beside T5, not part of it. The saves come from the oracle's cache, made beforehand once per version and
   held to their SHA-1s (`port/cmake/oracle.cmake:176-184`; `SAVES`, `oracle.py:791`). At a release candidate, items
   6, 11 and 12 add their runs.
2. **The build: ASan, UBSan with its function check, and LeakSanitizer.** `-DPORT_SANITIZE=ON` with
   `PORT_SANITIZE_RECOVER` off, `RelWithDebInfo`, compiled by upstream clang 17 or later, Homebrew's on a Mac
   (`-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang`, `sanitizers.md:43-46`), for the function check and the
   LeakSanitizer that Apple clang lacks. Before a run counts, it shows its program instrumented: ASan's runtime answers
   (`ASAN_OPTIONS=help=1`) and the function check is in the program's symbols (`function_type_mismatch`,
   `lessons.md:760-763`), so that a stale plain or Apple-clang program can't pass for clean. Each run records the
   compiler's version line; a new major version is marked so, since a report it brings may be a new check of old code
   rather than a regression, and it is fixed all the same.
3. **The runtime's options.** The program's default, `abort_on_error=1`, with
   `ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1`, which a counted run sets itself for both halves of
   the set, ctest's routes and the program alone, in place of the caller's: no other `ASAN_OPTIONS`, `UBSAN_OPTIONS`
   or `LSAN_OPTIONS` reaches it but item 5's suppressions file, once one exists, and `log_path`, which moves the
   reports out of the output (`sanitizers.md:58-60`), is never set for one. Fake stacks have run only in
   `os_all_fake_stacks`: the first whole runs show whether they hold on the routes, and if they fail one without a
   report, the change that records it drops the option and says why.
4. **Clean.** A program-alone case is clean when all of these hold:
   - it exits with status 0, and its log says that it stopped after exactly its frames
     (`port: stopped after N frames`, `port/app/main.c:715-717`). A trap (70), the watchdog (73), an abort (134), a
     crash's signal or a timeout fails it, and so does a trap stop that a sequence allows (`stop_allowed`,
     `oracle.py:1948-1952`, which no sequence uses today): a trap or the watchdog leaves through `_Exit`, without the
     leak check (`sanitizers.md:108-109`);
   - no line of its output matches the expressions on which ctest fails a test in this build
     (`port/CMakeLists.txt:482-484`);
   - its start save has the SHA-1 that `SAVES` keeps for the version, and a make leg leaves a save of its own, as
     `oracle.py round-trip` requires (`oracle.py:1554`).

   A ctest route is clean when ctest passes it in the strict build. The set is clean when every case is, in both
   versions, on one head. A case that skips (no save for a version, no ROM) is a gap in that run, not a pass. A whole
   run's verdict carries to a later head only while no file that its cases compile, read or run has changed and the
   set hasn't grown; a release candidate's run is made on its own head.
5. **Declared exceptions, and nothing else.** Clean allows exactly:
   - the ignorelist's three entries, each with its traced cause: `[alignment] ApplyCurrentAnimResult_` (R1, R2),
     `[function] runSaveBlkInitializers` (R3) and `[function] VM_CallCommand` (R13), confirmed here
     (`sanitizer_ignorelist.txt:10-34`; `sanitizers.md:141-142`, `:185-187`, `:251`);
   - H1, the copyright screen's read past a heap block, which `fnd_hooks.c` lets through as the DS's
     (`sanitizers.md:126`, `:144-146`, `:212`);
   - in the map sweep alone, a zone's stop that ADR 0032's sweep (being written) declares with its traced cause, as
     the link rooms' may be (`STATUS.md:520-521`): that zone is clean up to its stop if no report comes before it, and
     the record names it as stopped, not as clean to its end.

   No runtime suppression of an ASan or UBSan report counts (ADR 0014:51-52). LeakSanitizer may be given one file of
   suppressions, by sanitizers.md's rule, for a leak inside SDL3 or a system library, each line with its reason
   (`sanitizers.md:104-108`); none exists, since none has been needed, and a counted run passes it to the program
   itself (item 3). Each new exception of any of these kinds changes T5's criterion, so it comes in a pull request the
   owner reviews, with its traced cause; an ignorelist entry meets ADR 0014's rule as well, code right on every host
   (ADR 0014:35-39). Every other report is fixed in a pull request of its own: in Frigate's code, or in the game's
   under `PLATFORM_PC` with its rows in game-changes.md and decomp-findings.md (`port/CLAUDE.md:190-192`); with its
   row in sanitizers.md and a regression test on the actual source, as H9's fix has (`sanitizers.md:299`); and with
   the cases that showed it run again in both versions.
6. **Hosts.** The criterion is held on macOS arm64, in both versions. Beside it:
   - **the x86_64 slice**, which the universal app ships ([ADR 0015](0015-universal-macos-app-for-macos-11.md)), runs
     the set but for the map sweep, once, under Rosetta, on a release candidate's head, built as the sanitized
     distribution build builds it (`sanitizers.md:76-80`) by item 2's compiler, whose ASan runtime holds an x86_64
     slice (Homebrew's LLVM 23.1.3, read with `lipo -info` on 2026-11-02; the one sanitized distribution build on
     record, 2026-10-19's, was Apple clang's, `STATUS.md:5518-5524`, so a build with it is untried). A report there
     blocks the candidate as an arm64 one does. The slice runs code the arm64 one doesn't: the saved stack pointer
     from which LeakSanitizer scans a suspended fiber (`port/hal/os/mco_impl.c:18-22`), the fault handler's and the
     workers' branches (`port/hal/host/host_posix.c:157-163`, `port/gfx/workers.c:38-43`), the compiler's x86-64
     code, and sections aligned to 4 KiB (ADR 0015:47-49). A division by zero, which faulted only on that slice in
     the plain build (R5, `sanitizers.md:189`), is UBSan's to report on arm64 as well (`sanitizers.md:114-115`). Three
     routes have run so (`STATUS.md:5556-5561`);
   - **Linux** keeps CI's sanitized job on every pull request, with no ROM but with the function check and
     LeakSanitizer (`port-ci.yml:234-289`); strict routes there with the ROM run on demand, not for the exit;
   - **Windows** has no sanitized build of the whole program ([ADR 0013](0013-windows-build-with-msys2-clang64.md),
     `:146-148`), and the exit asks for none; its `_sanitized` test variants go on running there.
7. **When.** No run is scheduled: T5 runs on demand, under the owner's rule of 2026-11-01 (ADR 0031:170-173).
   - **The whole set** is due on a release candidate's head (ADR 0035) and on the pull request that ends a leg of the
     story (the legs that `port/cmake/oracle.cmake:216` and STATUS.md name), so that a report a leg brings is fixed
     before the next leg's oracle evidence is made on top of it (inferred). A maintainer may run it at any other time,
     and should after a merge that changes code that every route runs, such as an upstream sync's game code.
   - **A pull request that adds or changes a route** (a sequence, a round trip, a replay, a station) runs the strict
     cases it adds or changes, in both versions, and records them as its T5, once T5-A's runner is on `main`; until
     then it records T5 as not run, and the next whole run covers it. No other pull request owes one: it is the one
     strict run a pull request owes, and only one that adds or changes a route.
   - **A pull request that fixes a report** runs again the cases that showed it, in both versions.
   - **No strict route is in the merge gate.** Neither CI nor the exact-head verification runs one; the evidence above
     is the pull request's own, which its review checks. CI's `sanitize` job stays a check of every pull request,
     inside the `Frigate CI` aggregate (`port-ci.yml:244-289`, `:312-315`).
8. **The story in pieces from saves.** The story's T5 is its pieces, each from the oracle's save at its start, and each
   round trip's continue leg from the port's own save; never one process from power-on to the end. Past every
   trainer's AI a route is already a chain of pieces from melonDS's saves
   ([ADR 0023](0023-oracle-routes-part-where-the-ds-cpu-time-decides.md), `:51-54`). Each piece is held in both
   versions; a version whose save has no SHA-1 has no such piece (`oracle.py:784-785`), which item 4 counts as a gap.
9. **Wireless in a release candidate: unsupported, and refused without a report or a freeze.** Local wireless and
   Wi-Fi stay phase 7's work, and R21 to R24 with them but for the row below (ADR 0019:97, :136-137); the candidate's
   notes say that they don't work (ADR 0035 places the words). Two things come first:
   - **a strict refusal check**: a program-alone case per version, from a story save with an input of the port's own,
     that turns the game's wireless on in each way a player can reach in the story the candidate holds, and passes by
     item 4 alone: it plays to its last frame with status 0 and no report, whatever the game shows once WM refuses to
     start. Its first run establishes what that is, and the candidate's notes say it. A trap, a crash, a report or the
     watchdog fails it: the watchdog means the game spinning where the platform layer never runs, which a player
     would see as a frozen game (above). Each is fixed before the candidate: in the platform layer, so that the game
     meets an answer its own code handles, the fix showing which and why, or as item 5 fixes a report. The check is
     the port's alone: melonDS runs the DS's wireless, so it would not refuse (inferred);
   - **`WmInitCore`'s check on a cut address** (`wm_system.c:62`), the real row whose answer depends on where the host
     put a buffer (`ptrcast_baseline.tsv:89`), is fixed before the refusal check counts, so that every run refuses
     alike, with its row in game-changes.md and its proof that MWCC's output didn't change (`port/CLAUDE.md:182-185`).
     The other real rows stay phase 7's.
10. **`port-64bit-sweep` stays unmerged, and goes once `main` has what it still holds.** Its two fixes that `main`
    lacks (`startMatch`'s `sptr`, `WM_ReadMPData`'s `uptr`) are written again on `main`, against ADR 0019's guard and
    baseline, when the owner wants the real count lowered before phase 7, or in phase 7. The branch, which `origin`
    has no copy of, is deleted after that, so that nothing it holds is lost; the ADR index keeps its part in the
    numbering's history (`port/docs/adr/README.md:24-25`; ADR 0019:12-13).
11. **No ThreadSanitizer in the criterion.** The game, the platform layer and the frame hooks stay on one thread, and
    only renderer code runs on the workers, on memory shared out by lines (ADR 0016:48-55); `g3d_ds3d_bands` holds the
    bands to one band pixel for pixel (ADR 0016:56-59, :86-88). On a release candidate's head, the by-hand
    ThreadSanitizer runs of `g3d_ds3d` and `g3d_ds3d_bands` (ADR 0016:89-93) are made again, in a build of their own,
    and recorded beside T5; a report there blocks the candidate as a T5 report does.
12. **The window, on a release candidate.** One strict run per version with the window and the sound device: the
    sanitized program, or its app, which carries the runtime and is for this Mac only (`sanitizers.md:69-75`), plays
    one story piece from its save to its last frame and is judged by item 4, as windowed apps continued Route 20's
    save for 4,700 frames on 2026-10-28 (`STATUS.md:1915-1918`, which doesn't name their build). It is recorded beside
    the candidate's T7 playtest.
13. **The record.** Each counted run fills T5's line in the Tests field of a STATUS.md entry, with the evidence that
    STATUS.md asks for (`STATUS.md:14-16`, `:27`): the commands, the head, and whether `port/`, `src/`, `lib/` and
    `include/` were clean; the compiler's line and the options; per version, each case's frames, time and verdict, and
    each skip with its reason. Reports go to sanitizers.md's tables, as now. The exit's evidence is the record of the
    run on the release candidate's head. Nothing from the ROM leaves the machine: the record holds verdicts, counts
    and the saves' SHA-1s, while the runs, the saves and the logs stay local (`port/CLAUDE.md:217-219`).

## Alternatives

1. **The set.**
   - *ctest's `route` label alone*, the reading `sanitizers.md:5-7` gives: no new definition, but the strict build
     would stop at the Aspertia City lookout, since every later sequence starts from a save the oracle made.
   - *The oracle's sequences run sanitized, beside melonDS* (`ctest -L route` in the strict tree, as on 2026-10-24):
     it compares every frame too, but adds melonDS's time to every run and holds what plain T4 holds. Poisoning leaves
     every byte the game sees the plain build's (ADR 0014:43-47), and on 2026-10-24 the sanitized comparisons matched
     as the plain ones had (`STATUS.md:3518`). A maintainer may still run it.
   - *Each case registered as a ctest test*: each would need the oracle's saves as a fixture, and ADR 0031's
     `digest_<seq>` tests are to run these sequences port-alone (ADR 0031:82-87, :160-165); once they do, they can
     take the sequence cases over, by an amendment.
   - *The map sweep left out*: the exit waits on no strict run of the sweep's 615 zones (`STATUS.md:464`), but a
     memory bug in a zone that no piece reaches stays unseen until a player meets it, as H9 was until the sweep found
     it (`sanitizers.md:291-299`).
2. **The compiler.** *Apple clang*, the compiler CMake finds: nothing to install, but it misses the function check's
   reports, five of the sixteen R reports so far, and every leak. *One pinned LLVM*: reproducible, but Homebrew's moves;
   the recorded version line serves, and a pin can follow if a new version ever moves a verdict. *MemorySanitizer
   beside them*: by clang's documentation, not checked here, it doesn't run on macOS and needs every library
   instrumented, SDL3's too; not tried.
3. **The options.** *No fake stacks*: faster and lighter, but blind to a frame used after it returned
   (`sanitizers.md:165-166`). *No `detect_leaks`*: no leak check on macOS (`sanitizers.md:97-98`).
4. **Clean.** *Status 0 alone*: a run cut short by an allowed trap, or an uninstrumented program, would pass. *A pass
   pieced together from runs on several heads*: cheaper, but no head would have been shown clean. *A whole run carried
   to a candidate's head*: no rerun where nothing changed, but the exit's evidence would be another head's.
5. **Exceptions.** *Runtime suppressions* (`UBSAN_OPTIONS=suppressions=`): ADR 0014 turned them down, since they make
   the checks recoverable and need the environment on every run (ADR 0014:51-52). *No LeakSanitizer suppression at
   all*: nothing hides a leak, but one inside SDL3 or a system library would hold the exit, for code Frigate can't fix,
   until an amendment. *New exceptions without the owner's review*: ADR 0014 stays as it is, but an exception that
   narrows the exit's criterion lands as any change does. *R13 fixed instead of listed*: a typed wrapper for each field
   command, as R12 got one (`sanitizers.md:250`), would change the game's tables for no change in what runs, as ADR
   0014 judged of R3's 56 initializers (ADR 0014:48-50).
6. **Hosts.** *The x86_64 slice on every whole run, or with the map sweep*: twice the runs or more, under Rosetta at a
   speed nothing has measured, for a slice that runs the arm64 slice's C but for the branches item 6 names. *Linux
   strict routes required*: the Linux entries on record ran without a US dump (`STATUS.md:1402-1414`, `:2181-2185`),
   so no machine for them is known. *A sanitized Windows build first*: untried, with how its fault handler meets
   ASan's unknown (ADR 0013:146-148), for a host the exit doesn't name.
7. **When.** *Nightly*, as PLAN.md §6 says: the owner declined scheduled runs. *In the merge gate*: every pull request
   would wait for a strict build and the set in each version, an hour or more (an estimate: nothing has timed the
   program-alone set), against the owner's rule that per-PR checks stay fast. *A route pull request's own runs
   optional too*: no pull request waits on a strict run, but a report that a new piece brings is found at its leg's
   end, after the piece's oracle evidence is made, and its fix can send the oracle pull requests open then back to T4
   (inferred). *Only on a release candidate*: a report found then lands late, and its fix makes stale the T4 evidence
   of every oracle pull request open then (inferred).
8. **One process through the story.** It would need a whole-game input of the port's own past every AI cut, since the
   oracle's presses don't fit the port's own battles (ADR 0023:55-64); it would take hours per version in the strict
   build (an estimate), and start again from power-on after any failure.
9. **Wireless.** *R21 to R24 fixed now*: R21's menu values and R23's and R24's words wait for phase 7's design of how
   the Wi-Fi libraries run on a host (ADR 0019:136-137), and none can be checked without a link. *Unsupported with no
   check*: a player who turns wireless on meets what nobody has run. *The wireless entries hidden*: a `PLATFORM_PC`
   change to the game's menus, which port/CLAUDE.md keeps for rare needs (`port/CLAUDE.md:166-180`), and a game that
   no longer shows what a DS shows, which phase 7 would undo. *A WM on the host that starts and finds no partner*:
   what a DS out of everyone's reach shows (inferred), but a model of the ARM7's wireless, phase 7's subject, before
   phase 7 designs it.
10. **The branch.** *Deleted now*: the two fixes are a few lines that this ADR names, but `origin` has no copy of its
    six commits, which ADR 0019 and the ADR index name. *Kept for good*: its names and its baseline's format are
    stale, and `main` records what it taught. *Merged or rebased*: its guard predates ADR 0019's, and its WM rows
    contradict main's.
11. **Threads.** *ThreadSanitizer on the whole program*: it can't share ASan's build, the fibers have no annotations
    for it (none in `port/hal`), and the races it could find are the workers', which the renderer's tests reach. *A
    ThreadSanitizer job in CI for the renderer's tests*: a third sanitized configuration and its minutes, for code that
    changes only with the renderer; worth it if that starts to change often.
12. **The window.** *Left to T7's plain playtests*: a memory error in the presentation or the sound device would show
    only as a crash. *A ctest route with SDL's dummy video and audio drivers*: it would run the presentation's code
    without a display, but not the devices' paths (inferred).

## Consequences

- Phase 5's T5 gets a definition that a run can be judged by, and the exit's evidence is one recorded run on the
  release candidate's head.
- **On acceptance**, the same change gives PLAN.md §6's T5 row (`PLAN.md:394`) and port/CLAUDE.md's testing table
  (`port/CLAUDE.md:246`) item 7's cadence in place of "nightly", and rewords PLAN.md's "Each level runs automatically,
  from every commit to nightly" (`PLAN.md:385`) so that it no longer claims T5; drops sanitizers.md's "nightly run of
  them is still to come" (`sanitizers.md:54`), adds a short section on the criterion there, and adds R13's entry to its
  table of what the build leaves out, which lists two of the ignorelist's three (`sanitizers.md:139-142`); has
  `port/tests/README.md:46`'s "The routes are that build's T5 runs" name the program-alone half too; and sets ADR
  0014's status line, and its index row, to "accepted; amended by 0033": what may enter the ignorelist stays ADR
  0014's rule (ADR 0014:35-39, :64-65), and item 5 adds that each new entry changes T5's criterion, for the owner's
  review.
- **T5-A**, a later pull request, implements it: a runner in `port/tools` that reads `oracle.py`'s tables through one
  adapter, never makes a save, writes nothing into the repository and runs both halves of the set with item 3's
  options; its ROM-free test, which must print nothing that the reports' expressions match, since every test in CI's
  sanitized jobs carries them (`port/CMakeLists.txt:473-487`); and `main`'s first baseline, with any report it finds
  recorded as open and fixed in a pull request of its own. Until then STATUS.md entries record T5 as not run, as now.
- **Story pull requests** run their new pieces as their T5 once the runner exists (item 7). Youngster Terrell's piece,
  on `port-terrell-cut` (`d6ef61348a`), adds the sequence `terrell` from the Route 20 save (that branch's
  `oracle.py:725` and `:747`) and joins the set through the tables alone; landing before the runner, it records T5 as
  not run (that branch's `STATUS.md:74`), and the next whole run covers it.
- **A change that every route runs but that adds no route**, such as the geometry engine's depth fix in flight
  (`port-ge-depth-cut`), owes no T5 of its own: its tests run sanitized in CI, and the whole run that item 7 advises
  after its merge covers it.
- **ADR 0031** (proposed): its part 6's presets should name this set by what it runs and call the runner rather than
  repeat it (ADR 0031:160-173), and its long tier of the sanitized build's digests stays an option beside T5 (item 1).
  Once its `digest_<seq>` tests run the sequences from saves port-alone (parts 3 and 5), they may replace the runner's
  sequence cases in the strict tree.
- **ADR 0032** (the save lab, being written): its stations join the set, each run by the program alone, and its map
  sweep (L3) joins the whole runs, each stop it declares with a traced cause being item 5's exception.
- **ADR 0020** (D6, proposed beside this one): as proposed, each version keeps a program of its own, a thin `main()`
  over a core that every test and route runs, so the set runs as it is, and its stage that makes the cores libraries
  runs T5 on them; one build tree for both versions, which it leaves optional, changes only how the runner finds the
  programs.
- **ADR 0035** (the release candidate, proposed beside this one): what the candidate waits for includes T5 as this ADR
  defines it, run on the candidate's own head (item 4), with items 6, 11 and 12's runs, and its notes say what
  wireless does (item 9). The candidate's programs are plain builds: T5 holds the same source in the strict build, and
  `package_macos.sh` refuses a sanitized app (`sanitizers.md:69-80`).
- **ADR 0034** (60 fps, proposed beside this one): T5 times nothing; the strict build refuses the timing gates and has
  no frame-time routes (`sanitizers.md:67-68`; `port/cmake/frame_times.cmake:24`). T5's runs load the machine, so
  they stay out of its quiet windows.
- **Widescreen and mods** ([ADR 0030](0030-widescreen-by-an-engine-side-hor-plus-scale-with-native-readbacks.md),
  proposed, phase 6): the set runs the native path, as ADR 0030 has replays and recordings run (ADR 0030:117-118); a
  presentation-only enhancement runs in it as it runs in every test (ADR 0030:94), and one of another class brings its
  own strict cases in the change that adds it. The mod platform brings libraries of its own (ImGui and Lua,
  `PLAN.md:468-469`): one that the sanitized build leaves out, as it leaves out SDL3, or whose leak only a suppression
  can answer, gets its line in item 5 by an amendment.
- **The cost** is a strict build per version and the set's runs, all on demand: today 16 program-alone cases and
  147,920 port frames per version beside the ctest routes; then the stations and the map sweep, about 1.35 million
  frames per version if each of its 615 zones runs the 2,200 frames of its boot input (`STATUS.md:430`, `:464`;
  inferred); and at a candidate the x86_64 slice under Rosetta. No program-alone timing of the set is on record;
  T5-A's first runs give one.
- **Watch** a compiler update (item 2), the runner's coupling to `oracle.py`'s tables, and the disk: each strict tree
  is a whole build of its own.
- **Revisit** if a release candidate is built for Windows or Linux, if ADR 0031's digests run the sequences from saves
  in the strict build, if the fibers gain ThreadSanitizer's annotations, or when phase 7 makes wireless real, which
  turns the refusal check into a route over a link.
