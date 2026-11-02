<!-- Records what phase 5's "60 fps with headroom" means and how Frigate measures it: the DS's own frame rate, each
frame made by its time on the DS's clock, each timed scene's busy time within half a frame headless and each timed
route on time paced and windowed, held on the M2 in quiet runs made on demand and never in the per-PR gate; and what
that leaves to phase 6 (widescreen, interpolated frames, the GPU renderer, the mods' share of a frame), the 3D
renderer's workers, CI and the window's presentation. -->
# 0034: 60 fps with headroom: the DS's rate on time and half a frame to spare, timed on demand

- **Status:** proposed
- **Date:** 2026-11-02
- **Plan:** §7 phase 5 (60 fps with headroom, and the exit's private release candidate), §4.5 (a frame paced to
  60 Hz), §4.6 (renderers, presentation), §6 (where each check runs), §7 phase 6 (widescreen, high internal 3D
  resolution, the mod platform)
- **Upstream:** `41f42ca1af` (upstream's of 2026-10-31), as Frigate's `main` (`e936696a97`) has it
- **Note (2026-11-02):** drafted from a read-only scout of phase 5's open exit items, outside the repository, and
  proposed for the owner's decision beside ADR 0020 (D6), ADR 0033 (sanitizer-clean replays) and ADR 0035 (the private
  release candidate). Nothing in the code changes with it. Each numbered decision is a recommended default that is the
  owner's to confirm, with its alternatives and their costs below. It stays proposed until the owner reviews it; the
  change that records the owner's answers sets the status line. File and line references are to `main` at
  `e936696a97`, to be read there (`git show e936696a97:PATH`), unless a pull request's commit is named (`ADR NNNN:L`
  is a line of that ADR); STATUS.md's entries are named by their dates too, since every new entry moves its lines.
  "Inferred" marks what was reasoned, not read or measured.

## Context

"60 fps with headroom" is one of phase 5's items (`PLAN.md:459-460`). The phase's exit is "the story can be finished on
Black 2 and White 2; a private release candidate" (`PLAN.md:464`), and a phase closes only when its exit criteria pass
on macOS (`PLAN.md:410`; `port/CLAUDE.md:249-250`). The phrase came with the plan's first commit (`0835ef06cd`) and was
never expanded: it names no machine, scene or metric. The plan's other words on speed: a frame ends in
`OS_WaitIrq(VBlank)`, which presents "and paces to 60 Hz" (`PLAN.md:205-208`); the 2D renderer should cost well under a
millisecond, "an estimate, not measured" (`PLAN.md:250-253`); 3D goes to the GPU through SDL_GPU at any internal
resolution (`PLAN.md:254-257`); high internal 3D resolution and widescreen are phase 6's (`PLAN.md:470-472`), and so
are the mod platform's menus, hooks and Lua (`PLAN.md:326-338`, `468-469`), which will spend the same frames
(inferred).

**The DS's rate.** Frigate's virtual clock runs 2,130 cycles a line and 263 lines a frame, 59.8261 frames a second
([ARCHITECTURE.md](../ARCHITECTURE.md#one-frame), `ARCHITECTURE.md:84-85`). At each VBlank the program presents the
frame as soon as its lines are drawn, then, unless `--headless`, waits until the frame's time on that clock, 560,190
cycles of 33.514 MHz, 16.715 ms after the last one's (`port/app/main.c:456-469`, `339-357`). Once a frame's work ends
more than 4/60 s, about 66.7 ms, after its time, `Pace` stops catching up and starts its count again (`main.c:339-340`,
`355-356`). The field runs at 30 Hz on the DS, which also drops frames under load (`PLAN.md:399-400`); Frigate's clock
moves with the game's ticks and the waits it models, not with the host's work (ADR 0023:31-34), so it never drops a
frame where the DS's CPU time would (inferred), as Ship of Harkinian runs with none of the console's lag
([ADR 0023](0023-oracle-routes-part-where-the-ds-cpu-time-decides.md), accepted, ADR 0023:21-23). The Platinum port's
"60 fps" is another thing: by default it runs most logic every frame, with speed fixes at five sites and everything
else at double speed. The research puts that default among the pitfalls and an option with targeted fixes among the
patterns to copy (`port/docs/research/platinum-port.md:227-235`, `497`, `517`). Ship of Harkinian keeps its logic at
the original rate and interpolates the frames a faster display shows between two of the game's
(`port/docs/research/port-landscape.md:196-201`), the technique that research proposes for Frigate (`:659-661`, its
own inference).

**The working definition is in the code.** `--frame-times FILE` writes each frame's whole time, its busy time (the frame
without the pacing wait) and its parts, the pacing wait among them (`port/hal/os/frame_time.h:1-12`, `19-28`;
`port/hal/os/frame_time.c:19-20`, `132-147`); a frame runs from the end of the last one's frame hook, after its wait,
to the end of its own (`frame_time.h:11-12`). `port/tools/frame_times.py check` runs a route 3 times, takes each
frame's least time over the runs, and fails when a scene's busy 95th percentile is above the budget
(`frame_times.py:11-25`); it reports busy time and the parts' medians, never the whole frame or how late a frame ended
(`:94-113`), and when a scene is over the budget while the 1-minute load average, read before each run and after the
last, was above the cores, it skips (77) instead of failing (`:167-174`, `195-197`). The budget,
`PORT_FRAME_TIME_BUDGET_MS`, is 8.36 ms, half the DS's frame (`port/cmake/frame_times.cmake:13-14`). The
`frame_times_*` tests exist only in an optimized, unsanitized build (`:1-12`, `23-24`), run alone (`:48`), and hold
eight scenes: the title and the start menu in both games, New Game's intro and 3D flight in Black 2, and the field menu
with its apps over the lookout's 3D in both games, from the oracle's starter save (`:57-70`).
[ADR 0016](0016-3d-frame-in-bands-on-worker-threads.md), whose plan line names this item, measured its paced frames
against half the DS's 16.7 ms (ADR 0016:5, 17-22). So headroom today is a factor of 2 at the 95th percentile, headless,
on whichever machine runs the test.

**What has been measured,** as busy time's median and 95th percentile in ms. The records of 2026-10-19 name an M2
MacBook Air with 4 performance and 4 efficiency cores and 16 GB (`STATUS.md:5222`); the later ones say only arm64 macOS
(`STATUS.md:550`, for one), taken here to be the same Mac (inferred).

- Quiet and headless, after the hot paths' work of 2026-10-19 (`STATUS.md:5156-5160`): Black 2's title 1.97 and 2.03,
  start menu 0.56 and 1.86, New Game's intro 0.63 and 0.82 and flight 1.80 and 2.06, party apps 0.72 and 0.95; White
  2's title 1.93 and 2.00, start menu 0.56 and 1.74, party apps 0.71 and 0.95. Before that work, with the 3D on one
  thread, the worst 95th percentile was the flight's, 4.68, its median 3.78 (`STATUS.md:5262-5271`).
- Paced, the same day: on SDL's dummy video driver the title took 5.23 and 5.43, 1.4 of it presenting, the core having
  clocked down while the program slept (`STATUS.md:5273-5277`; `ARCHITECTURE.md:143-146`). In a trial of another way
  to pace, the whole frame's 99th percentile was 16.74 with `Pace`'s, and 17.4 to 21.4 with a long sleep before
  `SDL_DelayPrecise` (`STATUS.md:5286-5288`, which doesn't name the driver).
- The x86_64 slice under Rosetta, at a load of 16 to 26: the title 5.72 and 8.00, the flight 5.63 and 7.30, 1.6 to 2.2
  times the arm64 slice's under a like load; never measured quiet (`STATUS.md:5017-5023`, `5083-5088`).
- Since then, none of it stated to be quiet: the field menu over the lookout, 3.46 to 6.87 at the 95th percentile at
  loads of 9 to 70 (`STATUS.md:4128-4130`, 2026-10-24); all five timing tests passing with a worst 95th percentile of
  2.29, the load not stated (`STATUS.md:2086-2088`, 2026-10-27); the title skipped at 10.01 and 15.00 at loads of 34.8
  and 38.8, and passing in the same verification's other runs (`STATUS.md:571-576`, 2026-11-01).
- Windowed and paced, by whole route: aspertia's 15,800 frames played in 4 minutes 25 seconds in each game with the
  sound on, with `--run-dir` and `--audio-dump` adding their work to every frame (`STATUS.md:3868-3872`, 2026-10-24),
  against 4 minutes 24 seconds of the DS's frames (inferred: 15,800 / 59.8261 s).
- The renderers' benches: the 3D's "typical" 2,000 triangles 741 µs and "heavy" 1,816 µs in 4 bands (`f6f924be2a`);
  the 2D's "typical" about 0.7 ms and "max" about 1.75 ms (`port/tests/gfx/bench_ds2d.c:1-3`).

**What has never been measured:** field walking, a battle, Castelia City, the C-Gear or a sound-heavy scene
(`ARCHITECTURE.md:312` names the title, New Game and the field menu; `STATUS.md:1918-1919` leaves Route 20's
performance unclaimed; `STATUS.md:3947-3948` says no test drives the C-Gear); the x86_64 slice on a quiet machine since
the bands; an Intel Mac, Linux or Windows; a paced run with a real window frame by frame (the windowed run above has no
`--frame-times`); the worst frames and their causes, such as a map load or the save's write, which runs `fsync` on the
game's thread (`port/hal/card/backup.c:451-461`; `ARCHITECTURE.md:99`, `111-112`); and the sound running short, which
nothing counts (`port/audio/audio.c:85-105`).

**Where it is held today.** The exact-head verifier builds both games' optimized trees with report-only off, so that
no local override weakens a gate, as its comment says, and runs every test (`port/tools/sync_upstream.sh:584-589`); a
record of 2026-10-28 calls the frame-time gates still required without report-only (`STATUS.md:1918-1919`). There
`frame_times_party_menus` always skips, since the verifier builds no oracle to make its save (`STATUS.md:566-567`), and
the others skip when a scene goes over the budget while other sessions load the machine, as the title did twice in the
verification of 2026-11-01 (`STATUS.md:571-574`). CI has no ROM, so every job skips them (`STATUS.md:5104-5106`). The
owner declined scheduled nightly and weekly runs on 2026-11-01 and asked that long checks be optional where they would
block pull requests (ADR 0031:170-173). So the budget holds at a verification only when the machine is quiet enough
then, and the field menu's scene never holds there (inferred from those records).

**The frame on the host.** The game's own C takes about 0.1 ms a frame outside loading; the rest is the renderers
(`ARCHITECTURE.md:122-123`, `132-141`): on the M2 the 3D frame up to 0.98 ms, the geometry engine up to 0.59, the 2D
lines 0.14 to 0.49, presenting about 0.2 with a window. The 3D frame is drawn in bands on as many threads as the host
has performance cores, at most 8 (ADR 0016:41-47); where macOS reports no performance cores, every physical core
(`port/hal/host/host_posix.c:489-506`), which is what an Intel Mac gets (inferred). A session accepted ADR 0016 on the
owner's standing instruction, not the owner in person (ADR 0016:8-12; `STATUS.md:5012-5016`). The window doesn't wait
for the display's vertical sync, since the program paces itself (`port/app/present.c:96-98`). The sound goes to an SDL
stream that aims to hold a tenth of a second, "which rides out a late frame", and moves its speed by up to 0.5%, in
proportion to how far it is from that, to keep it there (`audio.c:12-18`, `85-105`; ADR 0006:46-48). Phase 6's
widescreen grows the CPU 3D frame by about 1.33 at 16:9 and 1.75 at 21:9
([ADR 0030](0030-widescreen-by-an-engine-side-hor-plus-scale-with-native-readbacks.md), proposed, ADR 0030:198-199),
and the plan gives a higher internal resolution to the GPU renderer (`PLAN.md:254-257`; ADR 0016:71-72).

## Decision

Phase 5's "60 fps with headroom" passes when one quiet record, made on demand on the commit the private release
candidate is built from, shows on the M2, in both games, every timed scene within the headroom limit (2a) and every
timed route on time (2b), as the defaults below define them.

1. **The rate is the DS's, on time.** "60 fps" means every one of the DS's frames, 59.8261 a second and 16.715 ms
   each, made by its time on the DS's clock and presented as soon as it is made, as `AppFrame` and `Pace` do now. The
   game keeps its own rates: no game frame is skipped, doubled or invented, the field that runs at 30 Hz on the DS runs
   at 30 Hz here, and with no model of the DS's CPU time (ADR 0023) no frame drops where the DS's would (inferred). A
   field shown at 60 fps or more is not part of phase 5. If it is wanted, it comes in phase 6 as an enhancement with a
   declared class, as ADR 0030 would have every enhancement declare one (ADR 0030:90-98, 158): Ship of Harkinian's
   frames interpolated between the game's, which leave the native frames and the logic alone (presentation-only,
   inferred), or the Platinum port's logic every frame with fixes (gameplay-changing, inferred). (The owner's to
   confirm.)
2. **The metric: two held parts and a recorded rest.**
   - (a) *Headroom, headless.* Each timed scene's busy time at its 95th percentile, each frame the least of 3
     headless runs of an optimized build, stays within half the DS's frame, 8.36 ms, as the `frame_times_*` tests hold
     it now. A scene is a steady stretch of a route: its range leaves out the route's map changes, loads and fades.
   - (b) *On time, paced and windowed.* Each timed route is also played once with a window, paced, with
     `--frame-times`. A frame is late when it ends more than 2 ms after its due time on the DS's clock. The running sum
     of the file's whole-frame column gives each frame's end, and `Pace`'s due times follow from its first wait and
     its restarts (`main.c:348-356`), so `frame_times.py` can rebuild them (inferred). After the run's first 60
     frames, its start, no frame of the route, loads and fades included, is more than 4/60 s late, the lateness past
     which `Pace` stops catching up; and at most 1% of each scene's frames are late. A route that fails is played once
     more, and the record keeps both runs; it passes only if the second one does. No measurement has tried these
     numbers: the first quiet record shows whether they hold, and changing them is the owner's call.
   - (c) *Recorded, not held:* each scene's busy 99th percentile and worst frame; the paced run's whole-frame 99th
     percentile, its late frames, their longest run and the latest; the worst frame of each load and fade.
   - (d) *Loads and fades* are outside the scenes, so (a)'s 95th percentile and (b)'s 1% leave them alone, and only
     (b)'s 4/60 s holds them. Within that bound the sound's tenth of a second rides a delay out, and `Pace`'s catch-up
     refills it at once; past it the delay is lost for good, and the sound's queue stays short by it until the stream,
     slowed in proportion to the shortfall, makes it up, halving it about every 14 s (inferred from `audio.c:97-104`
     and `main.c:348-356`: a time constant of 0.1 s / 0.005, 20 s). A delay longer than the queue empties it, and the
     sound stops for the rest of the delay (inferred).
   - Every timed run plays a route's input file with only the options the timing tests pass (`--input`, `--frames`,
     `--watchdog`, `--save-dir`, `--frame-times`, and `--headless` for (a): `frame_times.cmake:46-47`,
     `frame_times.py:143`), nothing that adds work to a frame: no `--digest` (about 0.4 ms a frame in its prototype,
     ADR 0031:150), no `--run-dir` (whose row hashes both screens, `frame_time.h:9-10`), no `--audio-dump` or
     `--record`.

   (The owner's to confirm.)
3. **The M2 holds it; the x86_64 slice is reported.** The exit is held on the M2 MacBook Air that the measurements on
   record name (`STATUS.md:5222`). The record times the release candidate's commit as the distribution build makes it
   (`port/tools/package_macos.sh --distribution`: RelWithDebInfo, universal, for macOS 11, the pinned SDL3 linked in;
   `package_macos.sh:130`; ADR 0015:66-74), in `build/port-dist` and `build/port-dist-w2` (`package_macos.sh:22-26`),
   with the oracle's saves copied into each tree, as on 2026-10-19 (`STATUS.md:5053-5055`), so that the tests
   labelled `oracle` don't skip for want of them (`frame_times.py:127-129`). ctest runs a universal tree's arm64
   slice on the M2 (ADR 0015:132-135). The release candidate's own images are made apart from the same commit and
   options, in a clean clone at a neutral path and stripped, as ADR 0035 proposes; a path and a strip change strings
   and the debug map, not the code timed (inferred). The x86_64 slice runs the same headless routes under Rosetta in
   the same record, as a stand-in for Intel
   (`port/tools/ctest_one_slice.sh --build-dir build/port-dist --arch x86_64 -- -L timing`, and the same for
   `build/port-dist-w2`), and is reported, not held. Nothing is claimed for Intel Macs, Linux or Windows beyond what is
   measured: what Frigate claims is what the record shows, as
   [ADR 0015](0015-universal-macos-app-for-macos-11.md) has it for the documents (ADR 0015:102-104), and the release
   candidate's notes say which machine and slice were measured. (The owner's to confirm.)
4. **The scenes: today's, the field, battles and Castelia, then each story leg's heaviest.** In both games:
   - kept: the title and the start menu, and the field menu with its apps (`frame_times.cmake:57-65`);
   - the aspertia route, from a blank save with no oracle (`port/cmake/oracle.cmake:159-163`): New Game's intro and 3D
     flight, which White 2 has never had timed, Aspertia's street, where the player walks, and a house. Black 2's
     `frame_times_new_game` (`frame_times.cmake:66-70`) goes once aspertia's scenes hold its frames;
   - a trainer battle, Hugh's from the starter save, and a wild battle with its capture, from the Route 19 save, both
     from the oracle's saves (`oracle.cmake:164-183`) and labelled `oracle`, as the field menu's test is;
   - Castelia City, from a save the save lab mints there by a warp (`port/tools/lab/README.md:45-48`; its streets are
     rails, and eleven of its zones have no Fly position, `:91-100`), labelled `oracle` too, since a mint starts from
     an oracle save (`:45`); the café's neighbours, `ZONE_CASTELIA_CITY` among them, already run to frame 2,200
     (`STATUS.md:435-438`);
   - the C-Gear's bottom screen, once a story route reaches it (the game gives it in Aspertia City and then sends the
     player on to Virbank City's Gym, `res/field/scripts/aspertia_city.s:1296-1328`), timed as the route leaves the
     C-Gear: off, or on only once ADR 0033's refusal check passes, since turning it on starts the game's
     communication (`:1326`; `src/ov012/event_cgear_poweron.c:39-42`), which on a host meets a WM that refuses to
     start (inferred, as ADR 0033's Context gives it);
   - each story leg's heaviest scene: the leg's pull request runs its route with `frame_times.py check --report-only`
     (3 runs, each frame's least, at whatever load), names in each game the steady stretch with the highest busy 95th
     percentile, and registers it as a timing test that plays the leg's input to that stretch's end, which changes no
     program and holds the leg to no budget (5). Each such test adds three headless runs and one paced run in each
     game to every quiet window: at most the whole leg, as many as the 34,600 frames that the strict batch of
     2026-10-28 played from the catch's save to Route 20's (`STATUS.md:1848-1849`), about 9.6 minutes paced and a few
     minutes headless per game (inferred: 34,600 / 59.8261 s, and busy times of 1 to 2 ms a frame), so each leg can
     add about 25 minutes in both games to every quiet window (inferred). When the save lab's map sweep lands (planned,
     `port/tools/lab/README.md:15-17`), one pass of it with `--frame-times`, one zone at a time, ranks the zones, and
     the two heaviest in each game join the list.

   (The owner's to confirm.)
5. **Held on demand in a quiet window, never in the per-PR gate.**
   - The tests labelled `timing` are an on-demand test preset named for what it runs, `timing`, among the presets of
     ADR 0031's sixth part (ADR 0031:85-86, 165); until those exist, `ctest -L timing` and the paced runs by hand.
   - The per-PR gate leaves them out: the exact-head verifier's ctest (`sync_upstream.sh:589`), and ADR 0031's `pr`
     preset once it exists, run with `-LE timing`. This reverses the verifier's choice to hold them with report-only
     off, which its comment gives as keeping local overrides from weakening the gates (`sync_upstream.sh:584-587`). ADR
     0018 decided that "both ROM SHA1s and native tests are a separately required local status on the tested commit"
     (ADR 0018:27-28); that status then attests every native test but these, so accepting this ADR amends ADR 0018
     ([Consequences](#consequences)), and the change that does it says so where the local verification is described
     (`port/docs/REPOSITORY-POLICY.md:34`). If the owner confirms both this ADR and ADR 0031, this narrows ADR 0031's
     first default, "the full suite on the exact head" (ADR 0031:167-169), by that label. A developer's own `ctest`
     keeps them, as `port/README.md:81-83` says.
   - The owner chooses when each quiet window is. In it no other session builds, tests, verifies or runs melonDS, the
     load average that `frame_times.py` prints before each run and after the last stays below the M2's 8 cores, and
     the paced runs are made on mains power, with Low Power Mode off and the window in front. The record states all
     three.
   - It runs once the new scenes are in, to find a miss early; on the release candidate's commit, for the exit; and
     whenever the owner asks. Once `frame_times.py` can compare two programs, a change to what ARCHITECTURE.md's
     [Where a frame's time goes](../ARCHITECTURE.md#where-a-frames-time-goes) names (`ARCHITECTURE.md:120-147`) states
     in its pull request a comparison of its base and its head, run alternately in the same minutes on any load
     (`port/docs/lessons.md:631-634`): a report, never a condition of its merge.
   - The record holds numbers. The saves the runs start from stay on the machine (`port/CONTRIBUTING.md:110-111`).

   (The owner's to confirm.)
6. **Phase 5's headroom is the native frames'; phase 6 brings its own.** The budget holds the native 256x192 frames
   with no enhancement on, as every tested frame is native (ADR 0030:124-125), and the release candidate needs no GPU
   renderer. Phase 6 times each mode it adds (widescreen, a higher internal 3D resolution, interpolated frames) with
   the same tools and scenes and sets that mode's limit then, and gives its menus, hooks and Lua their share of a frame
   the same way. A phase 5 optimization keeps those open: it keeps ADR 0016's rule for the bands (ADR 0016:86-88) and
   ADR 0030's rules 1 to 10, which hold for every change now (ADR 0030:122-148), so no game update is skipped for being
   off screen, the 3D frame is never read with a literal 256, nothing scales the engine's projection or clip matrix,
   and presentation still sees each screen. (The owner's to confirm.)
7. **The record times the program's default threads, and one thread beside them.** The exit is measured with the 3D
   frame in bands on the performance cores, ADR 0016's default, as a player runs it. The quiet record also times each
   game's heaviest scene with `--render-threads 1`, whose frames are the same (`port/README.md:79-80`), so that the
   owner sees what keeping everything on the game's thread would cost. Whether threads run beside the game's one stays
   ADR 0016's question for the owner; this ADR leaves it alone. (The owner's to confirm.)
8. **CI prints the renderers' benches, report only.** The `macos-distribution` job, on Apple Silicon and Intel with
   macOS 26 and 15 (`.github/workflows/port-ci.yml:151-160`), runs `port_bench_g3d` and `port_bench_gfx_ds2d` with no
   argument after its tests, in a step that can't fail the job (`continue-on-error`), since `frigate-ci`, the check the
   ruleset requires, passes only when every job does (`port-ci.yml:17-18`): on the runner's own slice, and on Apple
   Silicon on the x86_64 slice too, under the Rosetta the job already checks for (`port-ci.yml:194-203`), the benches
   being universal like every target of that build (inferred). With no argument a bench only prints its times
   (`port/cmake/g3d.cmake:39-42`; `port/cmake/gfx_tests.cmake:33-37`) and takes seconds (inferred); its scenes are
   pseudo-random, nothing of the ROM (`port/tests/g3d/bench_ds3d.c:1-8`; `bench_ds2d.c:1-3`), and the job uploads
   nothing (`port-ci.yml:18-19`). They give the renderers' first native Intel numbers, and CI's own ratio for Rosetta.
   A change to CI's jobs is the owner's to make, as one was left to the owner before (`STATUS.md:5028-5030`). (The
   owner's to confirm.)
9. **The window keeps its own pacing, without the display's vertical sync.** For phase 5 and the release candidate the
   program paces to the DS's clock and presents without waiting for the display's vertical sync, as now
   (`present.c:96-98`), and (2b) measures frames against the DS's clock. A 60 Hz display refreshes 0.29% more often
   than the frames come, so on average one frame in about 344, about every 5.75 s, shows for two refreshes (inferred:
   1 / (60 - 59.8261) s); and since a frame is presented as soon as it is made, at a time that moves with its work, two
   frames can fall in one refresh, one of them never shown and a neighbour shown twice (inferred). Where the window
   reaches the screen without the system's compositor, full screen or on some Linux and Windows desktops, a present
   can tear (inferred). The release candidate's notes say so, among ADR 0035's known limits. Presenting in step with
   the display is a phase 6 setting beside the plan's layouts, scaling and frame-blend filter (`PLAN.md:262-263`),
   presentation-only in ADR 0030's classes (ADR 0030:90-98), and what interpolated frames would need
   (`port-landscape.md:659-661`). (The owner's to confirm.)

## Alternatives

- **1, a 60 fps field now, the Platinum port's way:** the logic every frame, five speed fixes, everything else at
  double speed (`platinum-port.md:227-235`, `517`). Every test that holds Frigate to the DS (the frame hashes, the
  oracle, ADR 0031's digests) would have to run with it off, each fix would be a change to the game outside `port/`
  (`port/CLAUDE.md:166-185`), and the research itself recommends an option with targeted fixes (`:497`), which phase 6
  can still make.
- **1, interpolated frames now, Ship of Harkinian's way** (`port-landscape.md:196-201`): the logic and the native
  frames stay the DS's, but each object's matrices must be recorded where the game draws them, which would take hooks
  in the game's drawing code as Ship of Harkinian's calls sit in its game code (inferred from `:197-198`), and the
  window must present at the display's rate (9). Phase 5's exit needs neither; it is phase 6's work, beside widescreen.
- **1, exactly 60 Hz.** Pacing to a display's 60.000 Hz runs the game 0.29% fast (inferred: 60 / 59.8261), which the
  sound's 0.5% could absorb, its queue settling about half again above its target (inferred, `audio.c:17-18`,
  `97-104`): a choice of presentation (9), not of the game's rate.
- **2, busy time alone (today's working definition).** The cheapest, with no window, but it can't see what a player
  sees: presenting, the core clocking down while the program sleeps (paced frames cost about 1.4 times a headless
  run's, `lessons.md:658-660`), the timer's wake-ups (with a long sleep before `SDL_DelayPrecise`, frames woke up to
  4 ms late, `STATUS.md:5286-5288`), or a pause long enough to empty the sound's queue.
- **2, (b) recorded but not held.** No second run on a failure and no limit to calibrate, but then the exit rests on
  headless headroom alone, and a pause that empties the sound's queue passes.
- **2, another limit for (a).** A whole frame, 16.715 ms, is the least a player needs and leaves no headroom. A quarter
  frame, 4.18 ms, as the M2's stand-in for slower Macs, passes today's quiet scenes (worst 2.06 ms,
  `STATUS.md:5156-5160`) and would catch a change that doubles a frame long before half a frame does, but no
  measurement ties it to any other Mac, and the M2's loaded runs went past it (6.87 ms, `STATUS.md:4128-4130`).
- **2, a headless 99th percentile or worst-frame limit,** such as every steady frame within one DS frame. Each frame
  being the least of 3 runs makes the worst one steady under load that comes and goes (`lessons.md:638-641`; inferred
  for the worst frame), so it could be held; but one heavy frame, a scene's first after a cut, would fail a scene and
  push its range into finer cuts, while (b)'s 1% already catches the spikes a player sees.
- **2, loads and fades in the 95th percentile.** The headroom would then depend on how many loads a route has.
  **Loads and fades exempt from everything:** a pause that empties the sound's queue would pass unseen.
- **2, a column for the sound's queue in `--frame-times`,** so that the record counts the gaps themselves: a change to
  the program's C, which every oracle route runs, for gaps that only a pause past (b)'s 4/60 s bound can make, which
  already fails the route (inferred).
- **3, the x86_64 slice held too.** Its loaded 95th percentile was 8.00 ms against 8.36 (`STATUS.md:5083-5088`), and a
  frame with the benches' heaviest 3D and 2D would take about 4.5 ms on the M2 and 7 to 10 under Rosetta (inferred:
  `f6f924be2a`'s heavy 3D, `bench_ds2d.c:1-3`'s "max" and `ARCHITECTURE.md:135-140`'s other parts added, then 1.6 to
  2.2 times), so holding it likely means optimizations first, for a translation whose cost isn't an Intel Mac's
  (inferred).
- **3, a native Intel Mac as the floor.** None is here; CI's Intel runners have no ROM and run only macOS 15 and 26
  (`port-ci.yml:156-160`; `STATUS.md:5104-5106`; ADR 0015:59-62); and most of the Intel Macs that macOS 11 supports
  are slower than the M2 (inferred), so a floor there would be another, lower bar set without a measurement. The owner
  can name one when one is at hand.
- **3, Linux and Windows held.** Their CI jobs have no ROM (`STATUS.md:5104-5106`), and the exit closes on macOS
  (`PLAN.md:410`).
- **4, today's eight scenes alone.** No field walk, battle, Castelia or C-Gear (`ARCHITECTURE.md:312`): headroom would
  rest on the scenes a player spends least time in (inferred). **Every route as a timing test:** each route adds three
  runs (`frame_times.cmake:43`) to every quiet window, the scarce resource. **A leg's scene picked from one run:**
  cheaper, but a single run under load can double its 95th percentile (`lessons.md:638-641`), so the pick could be
  the load's. **The benches alone:** they bound the renderers, not the game's frames; they stay reports (8).
- **5, held in the per-PR gate, as now.** The verifier runs them with report-only off (`sync_upstream.sh:584-589`), but
  they skip under the machine's load (`STATUS.md:571-574`) and the field menu's always skips there
  (`STATUS.md:566-567`); every scene added costs every pull request minutes (inferred); and a failure on a quiet
  machine would hold up pull requests that touch no frame. **Report-only in the gate:** the same minutes, for numbers
  that measure the load (`lessons.md:631-634`, `638-649`). **Scheduled runs:** declined by the owner on 2026-11-01
  (ADR 0031:170-173). **CI:** no ROM.
- **6, phase 6's headroom now.** Widescreen on the CPU renderer adds a third to three quarters of the 3D frame
  (ADR 0030:198-199): a frame whose 3D costs what the title's does would take about 2.8 ms at 21:9 on the M2 (inferred
  from `ARCHITECTURE.md:134`, `141`: 2.04 + 0.75 × 0.98), within the budget; but the title itself stays 4:3
  (ADR 0030:84-87), no wide path exists to time (ADR 0030:12-13), and the field, which widens first, has never been
  timed. Twice the internal resolution draws four times the pixels, about 4 ms of 3D for the title alone on the M2
  (inferred), which leaves headroom on slower Macs only with the GPU renderer: holding it now would put that renderer
  before the release candidate.
- **7, the exit held at `--render-threads 1`.** On the M2 even the worst scene before the bands was within the budget,
  4.68 ms (`STATUS.md:5268`), but the 3D frame takes about three times as long on one thread (ADR 0016:78-80), and
  slower Macs lose the most (inferred). It is the right test only if the owner reverses ADR 0016; the record's
  one-thread numbers price that choice.
- **8, no bench step:** no Intel number before the release candidate. **A gated step:** wall-clock limits depend on
  the machine (`g3d.cmake:39-42`; `gfx_tests.cmake:33-37`), and shared runners vary (inferred). **A step that can fail
  the job:** a bench that crashed would block every pull request for a report. **Every host job:** numbers for hosts
  the exit doesn't hold.
- **9, the display's vertical sync on.** With `Pace` kept, the program paces twice (`present.c:96-97`), and a frame
  ready just after a refresh waits for the next (inferred). With the display pacing instead, the game runs at the
  display's rate, 0.29% fast at 60 Hz, and a 120 or 144 Hz display needs a policy of its own (inferred); the frames
  are no longer timed by the DS's clock. **The plan's frame-blend filter** (`PLAN.md:262-263`) changes what is shown,
  never the native frames: a phase 6 option.

## Consequences

- The exit's speed item has a pass and a fail, on a named machine, with numbers the owner confirmed.
- The work that follows, each part a pull request of its own, none of them changing the program until the sixth:
  1. `frame_times.py` reads (2b) from the whole-frame and pacing columns, rebuilding `Pace`'s due times and restarts,
     with the late frames' longest run and the latest; prints the slowest frames with their parts (`--top N`); and
     compares a base and a head run alternately in the same minutes, scene by scene (`compare`). Its test,
     `frame_times_tool`, grows with them.
  2. The new timing routes (4), each a test registration.
  3. `-LE timing` in the per-PR gate (`sync_upstream.sh` and its tests), with ADR 0031's presets or before them.
  4. The CI bench step (8).
  5. The quiet record: a STATUS.md entry with every scene of both games on the M2, the x86_64 slice under Rosetta, the
     paced runs and the one-thread numbers, about an hour to an hour and a half of a quiet machine for the scenes that
     (4) names before the story legs' (inferred), growing by up to about 25 minutes with each leg's scene, so several
     hours once the story is done (inferred), and the documents brought to it: ARCHITECTURE.md's table, its "hasn't been
     measured since" for the x86_64 slice (`ARCHITECTURE.md:146-147`, which the loaded run of 2026-10-19 already
     contradicts, `STATUS.md:5083-5088`), and its feature map's "Frame times" row (`:312`); and `port/README.md:81-83`,
     `port/tests/README.md:83-91` and `port/docs/MODDING.md:258-265`, which say where the timing tests run.
  6. Optimizations, only if the record misses on the M2 or the owner holds more (3, 6). Each proves its output
     unchanged (`g3d_ds3d`, `g3d_ds3d_bands`, the routes' `frames.tsv` at several render-thread counts, as
     `STATUS.md:5234-5239` did for three routes at 1, 2 and 8, and the digests once ADR 0031's second part lands) and
     keeps ADR 0016's and ADR 0030's rules (6). The last measurement named the candidates: the geometry engine on the
     game's thread, the 2D text backgrounds, the workers' wake-ups and the paced core's clock (`STATUS.md:5298-5299`).
- The change that accepts it points PLAN.md's phase 5 item here and sets ADR 0018's status line, and its index row,
  to "accepted; amended by 0024 and 0034": the required local status attests every native test but those labelled
  `timing` (5), ADR 0018's other decisions standing. The verifier's `-LE timing` is the third step's, which says so
  where the documents describe the local verification's tests (`port/docs/REPOSITORY-POLICY.md:34`,
  `port/docs/SYNC.md:241-242`).
- What the default gives up: a change that slows the frames merges past every gate. The base-and-head report (5) and
  the next quiet record find it, and the record on the release candidate's commit is the last of them.
- Work in flight, as of 2026-11-02: each story leg adds its heaviest scene (4), and no story pull request is held to
  timing (5). Verification v2's second part (PR 78) adds `--digest` to both programs (`STATUS.md:388-392`), which
  timed runs never pass, and its digests become the optimizations' proof. The `ToScreen` depth fix (PR 76) changes
  the geometry engine's depth for vertices whose clip W is 2^24 or more (ADR 0031:143-149; `STATUS.md:388-389`), so
  the optimizations' baselines come after it. The save lab mints Castelia's start (4); its stations' rules are
  [ADR 0032](README.md#numbering-and-files)'s. ADR 0020's one program (D6), proposed with this one, keeps each
  version's program as a thin `main()` over the core the app ships, so the timing tests time that core under their
  names; only its optional one-tree stage renames them, as the branch `port-one-program` does (`4a4c2bc9f1`); the
  record is taken on the program's shape at the release candidate (inferred: a core built as a shared library may time
  differently). The sanitized build has no timing tests (`frame_times.cmake:11-12`), so sanitizer-clean replays
  (ADR 0033) and timing never share a run. The release candidate (ADR 0035) waits for this record; its notes, written
  before the record is made, name the machine and the slices it is held on and point to the release candidate's
  STATUS.md entry, which holds the numbers (ADR 0035, item 7).
- To watch:
  - the x86_64 slice, the likeliest miss (3);
  - the machine's state, which (b) feels and (a) mostly doesn't: its heat, its battery and Low Power Mode, and App Nap
    with the window hidden, which nothing in `port/` opts out of (a search of `port/` found no opt-out; SDL3's own
    behaviour wasn't read, and the effect is inferred);
  - a load between the M2's 4 performance cores and its 8, which the window allows and which the bands feel first
    (`lessons.md:631-634`);
  - the first hitches (b) may find: the save's write and its `fsync` on the game's thread (`backup.c:451-461`), and map
    loads;
  - scenes whose 3D swaps every other frame, as the field's would at 30 Hz (inferred from `PLAN.md:399-400`): there the
    still-picture cache draws each list a second time and copies the texture memory on that draw
    (`port/gfx/ge/ge_hal.c:27-34`, `172-183`; read from the code, not measured), a cost in power more than in the 95th
    percentile;
  - the sound running dry, which nothing counts (`audio.c:85-105`): (b) sees the pauses that would cause it, not the
    gaps.
- Revisit if the owner names another machine, when the GPU renderer draws the screen (`PLAN.md:254-257`) or phase 6's
  modes (widescreen, interpolated frames, the mod platform) are built, if (b)'s limits can't hold on a quiet M2 for
  reasons outside the program, if presenting in step with the display becomes the default (9) or ADR 0016 is reversed
  (7), and when Apple drops Rosetta (ADR 0015:143-144), which ends the x86_64 slice's stand-in.
