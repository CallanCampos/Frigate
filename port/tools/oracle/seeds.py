#!/usr/bin/env python3
"""The random seeds the DS's game built, read from an oracle run (port/tools/oracle/README.md, "Random seeds").

The game seeds its random numbers twice over with src/gfl/gf_standard.c's buildSeed: GFL's at start-up
(GFL_RandomInit, src/system/gfl_use.c), which the field, the trainers' AI and the battle's view draw from, and each
battle's at its start (src/ov167/btl_main.c), which its damage rolls, critical hits and a Poké Ball's shakes draw from.
buildSeed hashes the DS's timers and stack, which a host can't have, so an input file gives the port the DS's seeds
("seed" lines, port/tools/oracle/pbwinput.h). melonDS has no hook where buildSeed returns, so they are read from the
memory the oracle dumps at each frame (DUMPS, pbw-oracle's --dump), with what the game does with a seed:

- GFL's: buildSeed sets GFL's context (gfl_use.c's GFLUseWork, its first member), and GFL_RandomInit at once draws
  one number from it to seed the Mersenne Twister (GFL_RandomUpdateMT(GFL_RandomLC(0)): the twister's first word is
  that draw, the high half of the context's next state). Before the twister is drawn from (its index, mti, still 624),
  the context's state is walked back, one MATH_Rand32 step at a time, to the state whose high half is the twister's
  first word: the step before it is the seed.
- A battle's: btl_calc.c's work (data_ov167_021dd844) takes a copy of the battle's context at the battle's start
  (BtlCalc_Init, again in BtlCalc_ResetRand) and draws every roll from it. Overlay 167's memory is cleared when it
  loads, so a context in it, MATH_InitRand32's multiplier and increment beside it, is a battle's from the first frame
  it holds one: its state at that frame, before the battle's first roll, is the seed. Each later state of the same
  battle follows from the one before by a number of steps (its rolls), or is the seed again (the server's reset); a
  state that does neither is another battle's seed.

Standard library only.
"""

from __future__ import annotations

# MATH_InitRand32's multiplier and increment (lib/nitro/include/nitro/math.h), the same for every seed, and the step's
# inverse, which walks a state back
RAND_MUL = 0x5D588B656C078965
RAND_ADD = 0x269EC3
MASK = (1 << 64) - 1
RAND_MUL_INVERSE = pow(RAND_MUL, -1, 1 << 64)
# The twister's index once GFL_RandomUpdateMT has filled it (MT_N in gf_standard.c), before its first draw
MT_FILLED = 624

# The frames the oracle dumps GFL's state at: start-up builds the seed by frame 20 of the oracle in both versions, long
# before the game's main loop starts (frame 219 to 242). A battle's context is dumped at every frame
BOOT_FRAMES = "1-400"
# The memory each frame needs, as pbw-oracle's --dump takes it (DS layout): GFL's context (x, the multiplier and the
# increment), the twister's first word and its index (gf_standard.c's GFLStdTables: mt[624], then mti), and
# btl_calc.c's work, whose context is at offset 8 (a u16 heap ID, a pointer, then the context)
DUMPS = [
    f"seed_gfl=*gfl_use.c/sWork:24@{BOOT_FRAMES}",
    f"seed_mt=*gf_standard.c/sGFLStd:4@{BOOT_FRAMES}",
    f"seed_mti=*gf_standard.c/sGFLStd+2496:4@{BOOT_FRAMES}",
    "seed_battle=data_ov167_021dd844:32@all",
]
# How many steps a battle's rolls may take its context on from one frame to the next before a state is taken for
# another battle's seed; a turn draws a few dozen
MAX_STEPS = 100000


def step(state: int) -> int:
    return (state * RAND_MUL + RAND_ADD) & MASK


def unstep(state: int) -> int:
    return ((state - RAND_ADD) * RAND_MUL_INVERSE) & MASK


def context(data: bytes) -> int | None:
    """The state of a MATHRandContext32 (x, mul, add, 24 bytes, little-endian), or None when the bytes don't hold one
    MATH_InitRand32 set up"""
    if len(data) < 24:
        return None
    x, mul, add = (int.from_bytes(data[i:i + 8], "little") for i in (0, 8, 16))
    return x if (mul, add) == (RAND_MUL, RAND_ADD) else None


def boot_seed(memory: dict[tuple[int, str], tuple[int, bytes]]) -> tuple[int, int] | None:
    """GFL's seed and the frame it was read at, from the first frame whose dumps hold GFL's context and the twister,
    filled and not yet drawn from; None when no frame does. A ValueError when the walk back doesn't reach the twister's
    first word"""
    frames = sorted(frame for frame, label in memory if label == "seed_gfl")
    for frame in frames:
        gfl = memory.get((frame, "seed_gfl"))
        mt = memory.get((frame, "seed_mt"))
        mti = memory.get((frame, "seed_mti"))
        if gfl is None or mt is None or mti is None:
            continue
        state = context(gfl[1])
        if state is None or int.from_bytes(mti[1], "little") != MT_FILLED:
            continue
        first = int.from_bytes(mt[1], "little")
        for _ in range(MAX_STEPS):
            if state >> 32 == first:
                return unstep(state), frame
            state = unstep(state)
        raise ValueError(f"frame {frame}: GFL's context doesn't lead back to the twister's first word {first:#010x}")
    return None


def reaches(start: int, target: int, steps: int = MAX_STEPS) -> bool:
    state = start
    for _ in range(steps):
        state = step(state)
        if state == target:
            return True
    return False


def battle_seeds(memory: dict[tuple[int, str], tuple[int, bytes]]) -> list[tuple[int, int]]:
    """Each battle's seed and the frame it was first read at, in order"""
    seeds = []
    last = None  # the state at the last frame that held a context, None after a frame that held none
    for frame in sorted(frame for frame, label in memory if label == "seed_battle"):
        state = context(memory[(frame, "seed_battle")][1][8:32])
        if state is None:
            last = None
            continue
        if last is None or (state != last and state != seeds[-1][0] and not reaches(last, state)):
            seeds.append((state, frame))
        last = state
    return seeds


def ds_seeds(memory: dict[tuple[int, str], tuple[int, bytes]]) -> list[tuple[int, int, str]]:
    """Every seed the DS built in a run, in order: (seed, frame read, "start-up" or "battle")"""
    found = []
    boot = boot_seed(memory)
    if boot is not None:
        found.append((boot[0], boot[1], "start-up"))
    found += [(seed, frame, "battle") for seed, frame in battle_seeds(memory)]
    return found
