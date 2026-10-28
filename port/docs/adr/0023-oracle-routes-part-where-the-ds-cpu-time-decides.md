<!-- Records how Frigate's oracle routes handle game code whose outcome depends on how much CPU time the DS takes,
such as the trainers' AI: a sequence ends before the first decision that time moves, and the route goes on from
melonDS's save made after it. -->
# 0023: Oracle routes part where the DS's CPU time decides, and go on from melonDS's save

- **Status:** proposed
- **Date:** 2026-10-24
- **Plan:** §1 (no CPU emulator), §6 (T4: the oracle), §7 (phase 4's exit, and phase 5's story routes, each kept as a
  regression test)
- **Upstream:** `629a53cccb` (upstream merge #504), as Frigate's `main` has it
- **Note (2026-10-24):** proposed by the session that made the change, in the pull request that brings it (the review
  of phase 4's route asked for it, since it sets how every trainer battle of phase 5's routes is tested). It stays
  proposed until the developer reviews it; the change that records that review sets the status line to accepted.
- **Note (2026-10-25):** the third decision's wording follows the round trips before review: the port plays the
  oracle's presses where they still meet its course (Black 2's Hugh), not only where its course takes the same passes.

## Context

With the DS's random seeds ([ADR 0022](0022-the-ds-random-seeds-given-by-input-files.md)) the port draws the DS's
random numbers wherever its passes draw as the DS's do. The trainers' AI is where they don't. It runs its scripts for
at most 500 ticks of `OS_GetTick` a pass, about a millisecond, and goes on where it stopped at the next pass
(`TRAI_TIME_LIMIT`, `TrAI_RunScripts` in `src/ov170/tr_ai.c`). On the DS the scripts' interpreter and the data it loads
take the CPU's time, so the number of passes the AI thinks is the DS's speed. On the port the virtual clock moves a tick
at each `OS_GetTick` and in the busy loops the platform layer models, not with the host CPU's work, so the AI thinks in
one pass. Meanwhile the battle's view draws two numbers a pass from the same Mersenne Twister (`BtlvMcss_Main`,
`src/ov168/btlv_mcss.c`), and the AI picks among its best moves with the next number (`TrAI_ChooseMove`). Traced on
Hugh's battle with the DS's seeds (Black 2, 2026-10-24; [decomp-findings.md](../decomp-findings.md)): the AI's setup
draws in the same pass on both programs, the port's pick comes a pass later and the DS's three, so the DS picks with
another number, and from the turn whose pick differs the battle's rolls and screens differ too.

Any code that works until `OS_GetTick` says its time is up, and shares a generator or other state with work done at
every pass, parts the same way. Phase 5 keeps the story as replay routes, each a regression test, and the story has
dozens of trainer battles.

## Decision

- **A sequence holds the port to melonDS frame by frame up to the first decision the DS's CPU time moves, and stops
  there.** Its last frame is before the frames that decision changes; the decision's cause is traced (the generator's
  state per count, then its callers on both programs) and recorded in decomp-findings.md and the oracle README's known
  differences, as any difference is. Frames after it are never declared as known differences to keep a sequence going:
  a known difference is a frame whose cause is traced, not a stretch whose cause is the rest of a battle.
- **The route goes on from melonDS's save made after that point.** An input file plays the cut piece to its end on the
  oracle, from the save before, and saves (`oracle.py`'s `SAVES`, held to its SHA-1); the next sequence starts from
  that save, with the seeds the DS builds there. A route is then a chain of sequences, each from the save the oracle
  made at the end of the piece before.
- **The port's own play past the cut is held by what the CPU's time doesn't decide.** The port plays the piece from the
  same save, with the oracle's input file where those presses still reach the steps of its own course they reached on
  the DS, else with presses of its own after the cut. A trainer battle the port plays as its own can take another number
  of turns: Black 2's Hugh ends 560 counts later on the port, and its scene there still ends within the oracle's presses
  (2026-10-25: at count 14,370, between the oracle's last two B presses and 50 counts before its field menu's X); White
  2's ends 704 counts later, and the oracle's presses would open the field menu in the middle of its scene and miss SAVE
  (2026-10-24). Where it saves, its save is compared with melonDS's byte by byte, each difference declared with its
  traced cause, and melonDS continues the port's save as it continues its own (`oracle.py round-trip`); a press that
  missed shows there, as no save or as bytes no cause declares. A trap, a crash or a watchdog in the port's run fails
  it.

## Alternatives

- **Model the DS's CPU time for the AI:** count the ARM9's cycles for the scripts' interpreter, or slow the virtual
  clock's ticks while the AI runs. The first is emulating the CPU's timing for one subsystem (PLAN.md §1), and depends
  on the caches and wait states the port doesn't model; the second changes the clock that every other timing in the game
  reads.
- **Give the port the AI's passes or picks as input, as with the seeds:** it needs a hook in `tr_ai.c`'s loop, a change
  outside `port/` for a test's sake, and the passes depend on every trainer's scripts. Worth reconsidering if a route
  ever needs a whole trainer battle held frame by frame.
- **Declare the battle's frames after the decision as known differences:** that would hide a real bug in the rest of
  the battle behind a cause that explains only the first frame.
- **Compare trainer battles by their outcome alone:** weaker than the frames where the frames can match; it is what
  holds the part past the cut, and only that part.

## Consequences

- Each trainer battle in a route costs a cut and a melonDS save after it. A change to the presses, the save or the ROM
  before the cut remakes the saves after it (`SaveMaker.start` chains them, each held to its SHA-1).
- The part of a trainer battle past the AI's first moved decision is the port's own, compared with melonDS through the
  save it leaves and the runs that continue it, not frame by frame.
- Wild battles have no AI scripts (no AI flags, `GetClientAIFlags` in `src/ov167/btl_main.c`): their Pokémon pick at
  once and are held frame by frame to the end.
- Revisit if the platform layer ever models the ARM9's time, or if a hook for the AI's decisions is accepted.
