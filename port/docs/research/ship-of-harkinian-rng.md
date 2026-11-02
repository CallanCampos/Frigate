# Ship of Harkinian and the console's random numbers: the evidence behind ADR 0023

Research of 2026-11-01, read-only, for the owner's rule on
[ADR 0023](../adr/0023-oracle-routes-part-where-the-ds-cpu-time-decides.md). One research agent read the source of Ship
of Harkinian (SoH), libultraship, the Ocarina of Time (OoT) and Majora's Mask (MM) decomps and gz at pinned commits,
with SoH's issues and pull requests and speedrun.com's rules. A second agent then checked every claim against the same
sources and tried to refute the answer. It confirmed the answer, corrected a few details and marked some claims as
inference; this note follows it ([Limits](#limits)). Nothing from those projects is in the repository: the links point
to the lines that were read. Each claim is marked **read** (in the linked source or page), **computed** (the
arithmetic run again) or **inferred**.

The commits read: SoH `d4bf652`, the `develop` branch on 2026-11-01 (its release 9.3.1 of 2026-10-18 has the same
random-number and timing code and the same libultraship, `da6daf1`); the OoT decomp `14759ab`; the MM decomp
`710e1a7`; 2 Ship 2 Harkinian (2S2H) `222acd1`, with libultraship `7f9b86a`; gz `1155b00`; snesrev's zelda3 `45a149d`
and sm `578f90b`, with the A Link to the Past disassembly `79207d1`; the Super Mario 64 decomp `9921382` and sm64-port
`2b17d08`; and `b3df5ad`, the head of SoH's draft pull request 7465.

## The question

The owner's rule, in chat on 2026-11-01: if Ship of Harkinian doesn't match the original console's RNG, Frigate can
accept ADR 0023; if it does match it, Frigate should fix its own difference too. Frigate was named in the line of
community ports such as SoH ([port/CLAUDE.md](../../CLAUDE.md#the-port-is-frigate)), and SoH is, like Frigate, a native
PC port of a decompiled console game, played with the player's own ROM.

ADR 0023 covers where Frigate's play parts from the DS's. The trainers' AI thinks for at most 500 ticks of `OS_GetTick`
a pass (`TRAI_TIME_LIMIT`, `src/ov170/tr_ai.c`), so the number of passes it takes, and with it the random number it
picks a move with, depend on the DS's CPU speed, which no host has. An oracle route stops before the first decision
that time moves and goes on from melonDS's save.

## The answer

**No.** SoH reproduces neither the N64's random sequence nor the timing that feeds it, and doesn't try to: console RNG
manipulation and console TAS inputs don't play the same in it. Two facts decide it, both read in source: every reseed
reads the PC's clock, and the random floats are made the GameCube way. Its loop has none of the console's timing
either. The check put its confidence at about 98%, the doubt being only in how narrowly the question is read: SoH keeps
the game's generator and its boot state, so equal states give equal integers, but every reseed comes from the PC's
clock rather than the console's timer, and with an N64 ROM its floats differ even from equal states. Under the owner's
rule, ADR 0023 is accepted.

## How OoT draws its random numbers on the N64 (read)

- **One linear congruential generator**, whose state starts at 1
  ([qrand.c:55](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/libc64/qrand.c#L55)).
- **Reseeded from the CPU timer at every scene entrance and file load:** `Play_Init` calls
  `Rand_Seed((u32)osGetTime())`
  ([z_play.c:493](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/code/z_play.c#L493)).
  After each load the state is the timer's low 32 bits at that moment, which load times, lag and everything since boot
  decide.
- **Floats from the low 23 bits on the N64:** N64 builds mask the next number's low bits, where the GameCube versions
  and MM take its high bits, as the decomp's comment there notes
  ([qrand.c:100-115](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/libc64/qrand.c#L100-L115)).
- **Timing feeds the game elsewhere too.** The audio thread's random numbers mix in `osGetCount`
  ([thread.c:167](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/audio/internal/thread.c#L167),
  [:945-952](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/audio/internal/thread.c#L945-L952))
  and pick the notes of the Skull Kid's ocarina memory game
  ([general.c:2204-2213](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/audio/game/general.c#L2204-L2213)).
  Rooms finish loading in the background, checked without waiting
  ([z_room.c:745-747](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/code/z_room.c#L745-L747)),
  so the frame a room's actors start drawing numbers on depends on the DMA's timing. The controllers are read and
  the audio updated at every video retrace, not at every game frame
  ([padmgr.c:352-356](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/code/padmgr.c#L352-L356),
  [audio_thread_manager.c:30-61](https://github.com/zeldaret/oot/blob/14759abf9abb4295de142cdc1452998630ea70fb/src/code/audio_thread_manager.c#L30-L61)),
  so a lagging frame spans more of both.

## What SoH does instead (read)

1. **Every reseed reads the PC's clock.** SoH keeps `Play_Init`'s call, its only reseed
   ([z_play.c:521](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/src/code/z_play.c#L521)),
   but leaves the decomp's `src/libultra/os/` out of its build
   ([CMakeLists.txt:194](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/CMakeLists.txt#L194)).
   Its `osGetTime` and `osGetCount` are libultraship's, which count the host's steady clock in the N64's 46.875 MHz
   ticks ([os.cpp:41-52](https://github.com/Kenix3/libultraship/blob/da6daf1a50a5e46d6641408d52227e31e85e46df/src/libultraship/libultra/os.cpp#L41-L52)),
   and nothing calls `osSetTime`. So each scene load reseeds from the PC's clock: another state on every run, unrelated
   to any N64 cycle count. The audio thread's random numbers read the same clock.
2. **Floats are made the GameCube way.** `Rand_ZeroOne` takes the state's high bits
   ([code_800FD970.c:28-32](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/src/code/code_800FD970.c#L28-L32)),
   and the tree has neither the N64's mask nor an N64 branch for it. SoH has taken N64 ROMs since its NTSC support
   ([PR 4198](https://github.com/HarbourMasters/Shipwright/pull/4198), merged 2025-03-30), but a ROM supplies only the
   assets. From the boot state, 1, the integer states stay equal while the first three floats differ (**computed**):

   | `Rand_ZeroOne` | 1st | 2nd | 3rd |
   | --- | --- | --- | --- |
   | OoT on the N64 | 0.0652 | 0.0666 | 0.1719 |
   | SoH, the GameCube's way | 0.2365 | 0.3693 | 0.5042 |

3. **A fixed loop with no lag.** Each frame reads the controllers once, updates the game once and renders once
   ([graph.c:476-488](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/src/code/graph.c#L476-L488)).
   The pad manager handles one read and stops, the console's per-retrace dispatch left under `#if 0`
   ([padmgr.c:409-449](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/src/code/padmgr.c#L409-L449)).
   The audio thread runs exactly `R_UPDATE_RATE` updates a frame, and rendering waits for them
   ([OTRGlobals.cpp:1088-1100](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/soh/OTRGlobals.cpp#L1088-L1100),
   [:1876-1883](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/soh/OTRGlobals.cpp#L1876-L1883)).
   A room loads at once, the non-blocking check replaced by `if (1)`
   ([z_scene_otr.cpp:471-487](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/soh/z_scene_otr.cpp#L471-L487)).
   Gameplay runs at 20 Hz (the title and the menus at other rates), and a faster display only adds interpolated frames.
4. **Its own tracker and code say so.** An issue open since 2022-11-28 suggests that the frog minigame may be harder
   for the lack of "N64 slowdown": on an NTSC console its reporter found the minigame easier and the whole game seeming
   to slow down while the frogs are on screen, which the issue offers as a possible explanation
   ([issue 2030](https://github.com/HarbourMasters/Shipwright/issues/2030)). In the review of
   [PR 3438](https://github.com/HarbourMasters/Shipwright/pull/3438) (merged 2023-11-29), a reviewer calls the frogs'
   speed a slowdown of the console's hardware that the port lacks. Console timing comes back one fix or option at a
   time. That pull request's fix, on by default, slows Darunia's dance to the beat of Saria's Song as on the console,
   and its author believes it works against both PAL's adjustment and the console's lag
   ([z_en_du.c:282](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/src/overlays/actors/ovl_En_Du/z_en_du.c#L282),
   [SohMenuEnhancements.cpp:1336-1340](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/soh/SohGui/SohMenuEnhancements.cpp#L1336-L1340)).
   A pause buffer window and simulated input lag are options, off by default
   ([SohMenuEnhancements.cpp:310-328](https://github.com/HarbourMasters/Shipwright/blob/d4bf6528a3bb242bc5d862e4f1ab2b6d385c157b/soh/soh/SohGui/SohMenuEnhancements.cpp#L310-L328)),
   the first's tooltip explaining that it restores a console behaviour SoH drops by default. There is no console-RNG
   mode: the RNG options are cheats and the randomizer's seeds, the speedrun mode leaves the RNG alone, and nothing
   loads `.m64`, `.bk2` or `.gzm` movies.
5. **A draft's TAS tools would inject the console's seeds.** The draft
   [PR 7465](https://github.com/HarbourMasters/Shipwright/pull/7465), from a contributor's fork (opened 2026-10-26, not
   merged), would add tools modelled on gz. Its reseed goes through a hook that substitutes recorded seeds, and its
   `.gzm` importer keeps the inputs, resets and seeds, since the N64's lag and ocarina sync data have
   "no SoH equivalent"
   ([ShipGZ.cpp:842](https://github.com/Pepper0ni/Shipwright/blob/b3df5ad11e858d001f4dbe8c4beb0eb1e8320ebb/soh/soh/Enhancements/tas/ShipGZ.cpp#L842),
   the pull request's head). Another open pull request,
   [PR 7256](https://github.com/HarbourMasters/Shipwright/pull/7256), would add an optional simulated N64 heap: the
   memory layout differs by default too.

## 2 Ship 2 Harkinian (read)

What was read of it works as SoH does, but for the floats; its room loading and input polling weren't read. Its
`Play_Init` reseeds from `osGetTime`
([z_play.c:2417](https://github.com/HarbourMasters/2ship2harkinian/blob/222acd182a79db990cec32b0d55c93d41d21fe75/mm/src/code/z_play.c#L2417),
as the MM decomp's [z_play.c:2260](https://github.com/zeldaret/mm/blob/710e1a7d50a1f987f5fc55abca37f7fdcc8ee43d/src/code/z_play.c#L2260)),
which its libultraship again reads from the PC's clock
([os.cpp:41-52](https://github.com/Kenix3/libultraship/blob/7f9b86a593c526fc42261d7fe197100cecf57178/src/libultraship/libultra/os.cpp#L41-L52)),
and its audio updates in lockstep, `R_UPDATE_RATE` times a frame
([BenPort.cpp:816-849](https://github.com/HarbourMasters/2ship2harkinian/blob/222acd182a79db990cec32b0d55c93d41d21fe75/mm/2s2h/BenPort.cpp#L816-L849)).
Its floats are MM's, both taking the high bits
([rand.c:32-34](https://github.com/HarbourMasters/2ship2harkinian/blob/222acd182a79db990cec32b0d55c93d41d21fe75/mm/src/boot/O2/rand.c#L32-L34),
the MM decomp's [qrand.c:47-49](https://github.com/zeldaret/mm/blob/710e1a7d50a1f987f5fc55abca37f7fdcc8ee43d/src/boot/libc64/qrand.c#L47-L49)),
so of what was read, only the seed's source and the timing differ from MM on the console.

## The speedrun and TAS community (read)

speedrun.com lists SoH as a game of its own, [Ship of Harkinian](https://www.speedrun.com/Ship_of_Harkinian), whose
rules require version 9.3.X and leave the pause buffer and input lag optional. The main board,
[The Legend of Zelda: Ocarina of Time](https://www.speedrun.com/oot), takes console runs and, under emulation, only
Project64 1.6 and 1.7, for times slower than 110% of the record. It names no PC platform, so SoH runs don't count there
(**inferred**: its rules don't name SoH). 2S2H has a board of its own too
([2Ship2Harkinian](https://www.speedrun.com/The_Legend_of_Zelda_Majoras_Mask__2Ship2Harkinian)). No SoH TAS was found
on TASVideos, and no claim anywhere that SoH matches the console's RNG.

## How other projects handle a random number fed by a clock (read, then inferred)

- **gz records the console's values instead of reproducing them.** gz, the OoT practice ROM, runs on consoles and
  emulators, and TASVideos lists its `.gzm` macros as the format OoT's TAS community favours
  ([TASVideos](https://tasvideos.org/EmulatorResources/GZ)). It hooks the reseed call
  ([genhooks:36](https://github.com/glankk/gz/blob/1155b00b260d8c205a81f8d6b560077a4abbe784/genhooks#L36)), records
  each new seed with its frame and puts it back on playback
  ([gz.c:776-830](https://github.com/glankk/gz/blob/1155b00b260d8c205a81f8d6b560077a4abbe784/src/gz/gz.c#L776-L830),
  stored by [gz_macro.c:122-158](https://github.com/glankk/gz/blob/1155b00b260d8c205a81f8d6b560077a4abbe784/src/gz/gz_macro.c#L122-L158)),
  during macros gives the audio thread, in place of its own random number, one derived from the game's main random
  state, so that it replays the same
  ([gz.c:980-995](https://github.com/glankk/gz/blob/1155b00b260d8c205a81f8d6b560077a4abbe784/src/gz/gz.c#L980-L995)),
  and records the audio frames of each game frame for the ocarina
  ([gz.c:845-874](https://github.com/glankk/gz/blob/1155b00b260d8c205a81f8d6b560077a4abbe784/src/gz/gz.c#L845-L874))
  and the frames a room was still loading
  ([gz.c:486-508](https://github.com/glankk/gz/blob/1155b00b260d8c205a81f8d6b560077a4abbe784/src/gz/gz.c#L486-L508)).
- **snesrev's zelda3 and sm patch the timing out of the reference** that they compare with frame by frame. A Link to the
  Past's random routine reads the video chip's horizontal counter
  ([Bank0D.asm:22-41](https://github.com/walkingeyerobot/alttp-disassembly/blob/79207d1730c2af1c777bc5dc3f94538f779ea292/Bank0D.asm#L22-L41)),
  and zelda3 replaces it in the reference with its own
  ([zelda_cpu_infra.c:427-442](https://github.com/snesrev/zelda3/blob/45a149d8b4bb8098f53d1009f76e76e7d8b79bbf/src/zelda_cpu_infra.c#L427-L442));
  sm removes the original's waits for the video and its lag frames, and its controller reads in the video interrupt
  ([sm_cpu_infra.c:765-872](https://github.com/snesrev/sm/blob/578f90b3cc49557bb70060ad033bb90b8cf8ac50/src/sm_cpu_infra.c#L765-L872)).
- **sm64-port replays console TAS inputs**
  ([controller_recorded_tas.c:8-24](https://github.com/sm64-port/sm64-port/blob/2b17d081c9798b31b91dc71f37994b0da28cffc9/src/pc/controller/controller_recorded_tas.c#L8-L24)),
  because Super Mario 64's random state moves only when the game calls it, with no clock in it
  ([behavior_script.c:31-65](https://github.com/n64decomp/sm64/blob/9921382a68bb0c865e5e45eb594d9c64db59b1af/src/engine/behavior_script.c#L31-L65)).

So in the ports examined (**inferred**, not shown as a general rule), a decomp port matches the console's RNG only where
the game's RNG has no clock in it. Otherwise it substitutes its own clock (SoH, 2S2H), records the reference's values
and injects them (gz, the draft ShipGZ), or patches the timing out of the reference (zelda3, sm).

## What it means for Frigate

- **ADR 0023 is accepted under the owner's rule** (2026-11-01). SoH keeps the game's time-dependent code and feeds it
  the PC's clock, so it is no precedent for a port that reproduces such a difference.
- **The trainers' AI is the same kind of dependency** as OoT's reseed: its outcome is the time the hardware took.
  Frigate already goes further than SoH with the seeds, which an input file gives as the DS's
  ([ADR 0022](../adr/0022-the-ds-random-seeds-given-by-input-files.md)), as gz gives the console's. Where the DS's CPU
  time decides, ADR 0023's cut and its continuation from melonDS's save take the record-and-inject route a whole save
  at a time.
- **A finer form of that route stays open:** record from melonDS how far the AI got, or what it picked, and give it to
  the port at each such decision, so that a sequence holds the frames through it. ADR 0023 weighs it among its
  alternatives (it needs a hook in `tr_ai.c`, outside `port/`); the owner's rule doesn't call for it.
- **At those decisions the reference is melonDS's timing model, not a DS.** ADR 0023's note of 2026-10-28 stands: a
  measured model of the DS's time remains possible, and the ADR says when to revisit.

## Limits

- Not tested on hardware: whether a real N64 gives the same `osGetTime` at a given `Play_Init` across power cycles,
  and so whether inputs alone would replay a console run. The answer holds either way, since SoH never reads a
  console-like counter.
- Inferred, not read: that SoH runs don't count on the main OoT board, and the pattern across ports above.
- The check's corrections, which this note follows: the lines of gz's ocarina recording; which participant of PR 3438
  said what; that SoH's frame advance is a runtime option, not a debug build's (SoH has savestates too, and neither
  makes it reproduce the console's RNG); and that two TASVideos threads on OoT desyncs, which don't name timing as
  their cause, are no evidence here.
