<!-- Records why Frigate takes the random seeds the DS built from an input file, through a host wrapper of the decomp's
buildSeed (src/gfl/gf_standard.c), instead of building them from the host's entropy or changing the game's file. -->
# 0022: The DS's random seeds given by input files, through a wrapper of `buildSeed`

- **Status:** proposed
- **Date:** 2026-10-24
- **Plan:** §1 (all code is the decomp's C, no emulation), §4.5 (the platform layer), §4.8 (changes outside `port/`),
  §6 (T3, T4: replays and the oracle)
- **Upstream:** `629a53cccb` (upstream merge #504), as Frigate's `main` has it
- **Note (2026-10-24):** proposed by the session that made the change, in the pull request that brings it (the review
  of phase 4's route asked for it). It stays proposed until the developer reviews it; the next change to this file
  sets the status line to accepted.

## Context

The game seeds its random numbers twice over with `buildSeed` (`src/gfl/gf_standard.c`): GFL's at start-up
(`GFL_RandomInit`, `src/system/gfl_use.c`), which the field, the trainers' AI and the battle's view draw from, and each
battle's at its start (`src/ov167/btl_main.c`), which its damage rolls, critical hits and a Poké Ball's shakes draw
from. `buildSeed` hashes 20 bytes of whatever its stack held and the 32 of `OS_GetLowEntropyData`: the tick and the
line being drawn when the call comes, the VBlank count, the geometry engine's status, the clock, the MAC address, the
microphone's, touch screen's and buttons' state. On the DS the tick, the line and the stack's leftovers come from its
CPU's timing and its earlier calls; on a host the tick and line are the virtual clock's and the stack is the host's
(zeros, `-ftrivial-auto-var-init=zero`). So Frigate built other seeds than any DS: New Game's day care egg seed in the
save differed from melonDS's, and every battle rolled other numbers, so no battle could be held to the oracle frame by
frame. The oracle README's "Random seeds" has the measurements (Black 2's start-up builds `0xea56a175a2fffc8a` on the
DS and `0x91230e3906d9f8f8` on the port from the same input file).

## Decision

- **A seed is an input, like the clock and the presses.** An input file may give the seeds a DS built, as `seed` lines
  (`port/tools/oracle/pbwinput.h`). The game takes them, in their order, in place of the ones it builds; once they run
  out, or when a file gives none (a player's own run), it uses the seeds it builds. `run.txt` lists every seed a run
  used and whether it was given.
- **The decomp's `gf_standard.c` is compiled unchanged, through a host file.** `port/hal/gfl/gf_standard_host.c`
  renames `buildSeed` while it includes the file, as the other `*_host.c` files do (`port/hal/README.md`, "Compiled
  through a host file"), and its own `buildSeed` runs the decomp's first, then sets up the seed
  `port/hal/gfl/random_seed.c` gives with `MATH_InitRand32`. Running the decomp's first keeps everything it does
  besides the result: its reads of the virtual clock (`OS_GetLowEntropyData` reads the tick, and each `OS_GetTick`
  moves the clock a tick) and its stack's use. `port/hal/gfl/replaced.txt` keeps the file out of the game's sources.
- **The DS's seeds come from the oracle's memory**, since melonDS has no hook where `buildSeed` returns:
  `port/tools/oracle/seeds.py` walks GFL's context back to the draw that seeded the Mersenne Twister, and takes each
  battle's from `btl_calc.c`'s copy of its context before its first roll. `oracle.py seeds NAME` prints a file's lines;
  `oracle.py sequence` fails when a file's seeds aren't the ones the DS built in that run. The reading was checked
  once against an instrumented build of melonDS that logged `buildSeed`'s result, for every seed of the Route 19 route
  in both versions.

## Alternatives

- **Build the DS's seed on the host:** the tick and the line at the call are the DS's CPU's timing, and the stack's
  bytes are return addresses and pointers of the DS's own calls. Reproducing them needs a model of the ARM9's cycles and
  its stack, which is emulation (PLAN.md §1).
- **Change `buildSeed` in `src/gfl/gf_standard.c` under `PLATFORM_PC`, or with a hook macro:** a change outside
  `port/` with its row in game-changes.md and its proof for every sync, for what the platform layer can do on its own.
  The host file keeps the game's file as upstream has it.
- **Give the seed in an option or an environment variable instead of the input file:** the seeds belong with the
  presses they were built under (a battle's seed depends on the presses, the clock and the save before it), so the
  input file that plays them is where they are kept, and the oracle checks them in the same run.
- **Leave the port its own seeds and compare around randomness:** every battle, wild encounter and walking
  townsperson draws from these generators, so nothing past them could be held frame by frame.

## Consequences

- With the DS's seeds the port draws the DS's numbers wherever its passes draw as the DS's do: New Game's day care egg
  seed is the oracle's (`replay_same_save`), and the battles' rolls are the DS's while the battle runs the same passes.
- Where the DS's CPU time decides when the game draws, the numbers still part: the trainers' AI
  ([ADR 0023](0023-oracle-routes-part-where-the-ds-cpu-time-decides.md)).
- Each input file that reaches a battle or a random event carries the seeds of the run the oracle made with it, and a
  change to the presses or the save before a battle changes its seed: `oracle.py seeds NAME` writes the new lines, and
  `oracle.py sequence` fails until the file has them.
- A player's run gives no seeds and builds its own, as a DS would build its own: nothing changes for play.
- Revisit if upstream changes `buildSeed`'s signature or callers (today `gfl_use.c` and `btl_main.c` alone), or if a
  hook macro for the platform layer's seeds becomes part of the decomp's own conventions.
