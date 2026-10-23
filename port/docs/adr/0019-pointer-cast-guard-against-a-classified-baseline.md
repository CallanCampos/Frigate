<!-- Records why the game's casts that would cut an address on a 64-bit host are held to a classified baseline by a
test, the pointer cast guard, and why its report is T1's measurement. -->
# 0019: The pointer cast guard: the game's casts that cut an address, held to a classified baseline

- **Status:** accepted
- **Date:** 2026-10-23
- **Plan:** D5 (a 64-bit native build), §4.4 (the 64-bit warnings must only go down), §4.8 (rule 3, `uptr` and
  `sptr`), §6 (T1)
- **Upstream:** `0a12a2bd3e`
- **Note (2026-10-23):** accepted by the session that wrote it, carrying out the developer's plan of 2026-10-23 (its
  track B, which reserved this number), not by the developer in person; the developer is told so in the pull request
  that adds it. The guard was first written as a proposed ADR 0017 on the branch `port-64bit-sweep`, which stopped
  before it merged; that number has since gone to [ADR 0017](0017-external-contributions.md).

## Context

The decomp's C is written for a 32-bit DS, and much of it carries addresses in 32-bit integers: `(u32)work` passed to
a parameter typed `u32`, `(FieldTask *)a2` of a `u32 a2`, a getter that returns a name's address as a `u32`. MWCC
compiles that to the DS's code, so both ROMs keep matching; on a 64-bit host the address is cut to its low 32 bits and
the game faults the first time it runs the code, which for most of the game is long after the change that brought it,
since the field, battles and most apps aren't reachable yet. Every such fault so far was found by running the game into
it (the intro's nickname, the field script's VM, the infrared battle menu's callbacks).

The host build silences the four cast warnings for the game's C ([../../CMakeLists.txt](../../CMakeLists.txt)),
because there are hundreds and most are harmless, so an upstream merge can bring a new one in unseen. What T1 measured
instead was a count that no committed tool reproduced: [STATUS.md](../STATUS.md)'s "four pointer/int cast families"
(469 to 392 at the last sync, libraries 235 to 231) came from an uncommitted audit, and a contributor's commit gave the
libraries' count as 218 where STATUS.md said 231. Both counts are right for what they measured (STATUS.md's entry of
2026-10-23 reproduces them), but neither says which of the casts cut an address that the game uses, and neither stops
a merge.

## Decision

- **A test in every host build, `game_ptrcasts`** ([../../cmake/game.cmake](../../cmake/game.cmake)), runs
  [../../tools/check_ptrcasts.py](../../tools/check_ptrcasts.py): each `src/` and `lib/` command of the build's
  `compile_commands.json` again with the build's clang, syntax only, with the cast warnings on, the decomp's headers
  searched with `-I` rather than `-isystem` (so that the casts in the SDK's inline functions show), and
  `-Wshorten-64-to-32` and `-Wcast-function-type-strict` added. About a minute on a loaded machine.
- **It counts four kinds**: a pointer cast to a narrower integer, such an integer cast to a pointer, a `uptr`, `sptr`,
  `uintptr_t` or `intptr_t` narrowed to 32 bits, and a callback cast pairing a pointer or a `uptr` with a 32-bit
  integer. It leaves out a `long` or `unsigned long` narrowed (the decomp's are numbers, and a pointer cast to `long`
  is against §4.8's rule 6) and a call's result narrowed as it is stored (a getter that returns a `uptr` gives numbers
  and addresses alike; a caller that makes a pointer of the cut value is flagged there).
- **Its findings are held to a committed baseline**,
  [../../tools/ptrcast_baseline.tsv](../../tools/ptrcast_baseline.tsv), keyed by file, enclosing function, kind
  and types, with a count and no line, so that upstream's edits elsewhere in a file leave it alone and a renamed
  function, file or type passes as the same row. Each row has a class (`real`,
  `harmless`, `handled`, `unreachable`) and a one-line reason. A new key, a higher count, an unclassified row or a file
  that no longer compiles fails, with each site and the two ways out: a `uptr` fix with its row in
  [../game-changes.md](../game-changes.md), or a classified row
  ([../SYNC.md](../SYNC.md#when-the-pointer-cast-guard-fails)).
- **The real rows are the work list**, in groups named in their notes (R02 to R24, below), and shrink as the fixes
  land. `check_ptrcasts.py --report` prints the sites by class: that line is T1's 64-bit measurement and the "64-bit
  warnings" field of each STATUS.md entry, and `port/tools/sync_upstream.sh` prints it in its summary. It replaces the
  uncommitted audit's count.
- **On a host the type names say what a value is:** `uptr` and `sptr` are typedefs under `PLATFORM_PC` (macros for
  MWCC, as before; a Platform row in game-changes.md), so that clang's messages name them and a narrowed `uptr` is told
  from a number in an `unsigned long`.
- **The keys are LP64's**: the same in both versions, with Apple clang 21 and Homebrew clang 23 on macOS, and in CI,
  whose macOS, Linux and sanitized jobs run the test in their `ctest`: Apple clang 17 and 21 (arm64 and x86_64) and
  Ubuntu's clang 21.1.8 found the baseline's keys on the pull request that added it. It skips (77) on Windows, whose
  LLP64 (a 32-bit `long`) gives other keys, and without clang or the compile database; its own test checks the skip
  there.
- **Who fixes a failure**: the session or contributor whose change brought the cast; on a sync PR, the maintainer, in a
  follow-up feature PR, never by editing the sync PR's resolutions
  ([../SYNC.md](../SYNC.md#when-the-pointer-cast-guard-fails)).
- **The main arena is not constrained.** The stopped branch also kept `port_pxi_alloc_arena`'s arena within one 4 GiB
  span, so that cut addresses would compare as on the DS. No row needs it now: a difference of two cut addresses in
  one block is exact modulo 2^32, NNS FND's expanded heap compares native addresses since its own fix (its row in
  game-changes.md), and the two null tests of cut pointers (`PosEventAdd`, CPS's `no_need_inq`) test static data, not
  the arena. A comparison of cut addresses that could straddle 4 GiB is a `real` row.

The groups of real rows when the guard was added, on `main` after the sync of upstream `b62c655bfa` and PR 14,
182 sites in all (on `8ca29313d1`, where the baseline was first regenerated, they were 213: PR 14 fixed most of Join
Avenue's, and upstream typed the box name entry's parameters, group R08):

| Group | What | Sites |
| --- | --- | --- |
| R02–R05 | The field: the Y button's shortcut menu, the Day Care's egg and mail, the field-move cut-in's palette, the Join Avenue names that two callers still cut and overlay 137's function types that PR 14's `uptr` getters and setters are cast to | 20 |
| R06 | Battle: the handlers' message in event variable 0x3f | 12 |
| R09, R11–R19 | Menus and apps: evolution by trade, Mystery Gift, GX's extended palettes, the C-Gear and Funfest views, Memory Link, Pokéstar Studios, the Musical, the mic | 51 |
| R21–R24 | Wireless and Wi-Fi (phase 7): the Union Room, DWC's game code, the Wi-Fi libraries, local wireless and Download Play | 99 |

## Alternatives

- **Turning the warnings on in the build as errors**: hundreds of harmless sites would need casts or pragmas in the
  decomp's files, each a game change, and every upstream merge would bring more to annotate.
- **The audit's count as the gate**: it kept the build's `-isystem`, so a cast in an SDK header's inline function never
  showed; it counted a private header's site once for each file that includes it; it had no classes, so a new real cast
  could hide behind a removed harmless one; and it wasn't committed.
- **The compile probe** (`port/tools/compile-probe`) as the gate: it doesn't define `PLATFORM_PC`, so the `uptr` fixes
  don't show in its counts, and it takes about 8 minutes.
- **Keying by line**: every upstream edit above a cast would churn the baseline; by function and types, only a change
  to the finding itself does.
- **Counting every narrowing from a 64-bit integer**: pointer differences, `atol`'s and `strlen`'s results and the
  matching's `unsigned long` locals are a hundred more rows, and clang 23 names a pointer difference `__ptrdiff_t`
  where Apple clang says `long`, so the keys would differ between compilers.
- **Retyping each mixed getter's callers**: a getter that returns a number for most fields and an address for a few
  (Join Avenue's, the battle event variables') would make every caller that keeps a number in a `u32` a finding; the
  getter's return is retyped instead, and only arithmetic on its result counts.

## Consequences

- An upstream merge that brings a cast that cuts an address fails CI's macOS and Linux host jobs on the sync PR and
  the local verification's `ctest`; it is fixed or classified in a follow-up PR before the sync merges.
- Casts the guard can't see stay possible: a pointer stored in a union's integer member, a compound assignment such as
  `u32 dst += (uptr)p` (its read back is flagged), a pointer cast to `long`, and on Windows anything only LLP64 cuts.
- When the developer's one-program build (D6) puts both versions in one build directory, the two versions' tests become
  one, since both give the same findings.
- Revisit when Windows needs its own baseline, or when phase 7 decides how the Wi-Fi libraries run on a host, which
  settles the remaining real rows of R21 to R24.
