# Architecture decision records

An ADR records one decision about the port that isn't obvious from the code: what was decided, why, what else was
considered, and what follows from it. [PLAN.md](../PLAN.md) says what the port will be; ADRs say why each part is the
way it is, and they keep the reasoning when the plan's text moves on. The rules for sessions are in
[port/CLAUDE.md](../../CLAUDE.md).

## When to write one

- A choice between real alternatives that a later session could undo without knowing why: a library, a data layout,
  a boundary between the HAL and game code, a test method.
- The end of a spike (phase 0 has three: overlay sections on Mach-O, VRAM page aliasing, fiber scheduling).
- A change to a decision in PLAN.md §3: a new ADR that supersedes the old one, and the table in PLAN.md §3 updated in
  the same commit.

A bug fix, a refactor or anything one comment explains needs no ADR.

## Numbering and files

- Files are `NNNN-short-title.md`: four digits, then the title in lowercase words joined by hyphens.
- Numbers are given in the order ADRs are written, starting at 0001, and never reused. The decisions below that are
  waiting for an ADR take the next free number when someone writes them.
- 0014, the sanitized build's ADR, was written on the branch `port-sanitizers` as 0013 before the Windows build's took
  that number, and became 0014 when the branch merged.
- Three numbers are reserved for decisions whose work has started on branches of their own, so that those branches
  don't collide when they merge: 0019 for the pointer-cast guard against a classified baseline, 0020 for one program
  for both versions (D6) and 0021 for root heaps in host memory. A branch whose ADR took another number before the
  reservation renumbers it when it merges. The next free number is 0022.
- An accepted ADR isn't rewritten. Only its status line changes: to "superseded by NNNN" when a later ADR replaces
  it, or to "accepted; amended by NNNN" when a later ADR changes part of it. Typos and broken links may be fixed.
- Each ADR gets a row in the index below in the same commit.

## Template

```markdown
# NNNN: Title

- **Status:** proposed | accepted | accepted; amended by NNNN | rejected | superseded by NNNN
- **Date:** YYYY-MM-DD
- **Plan:** the PLAN.md section or decision (D1–D11) it settles
- **Upstream:** the `fuddlesworth/pokebw2` commit it was decided against

## Context

What forces the decision: the problem, the constraints, the measurements (with links to research reports, spikes or
STATUS.md entries).

## Decision

What we do, stated so a session can follow it.

## Alternatives

Each option considered and why it lost.

## Consequences

What becomes easier, what becomes harder, what must be watched, and what would make us revisit this.
```

## Index

| ADR | Decision | Plan | Status |
| --- | --- | --- | --- |
| [0001](0001-repository-and-upstream-sync.md) | Repository and upstream sync | D1, §4.1 | accepted; amended by [0002](0002-solo-project-no-upstream-posts.md) and [0018](0018-protected-main-and-sync-prs.md) |
| [0002](0002-solo-project-no-upstream-posts.md) | A solo project with no upstream posts | D10, §9 | accepted; amended by [0017](0017-external-contributions.md) |
| [0003](0003-ad-hoc-signing-no-apple-developer-account.md) | Ad-hoc signing, no Apple Developer account | D11, §7, §10 | accepted |
| [0004](0004-ds-addresses-by-header-translation.md) | DS addresses by build-time header translation | D3, §4.5 | accepted |
| [0005](0005-platform-boundary-as-built.md) | The platform boundary as built | D3, D5, §4.5, §4.7 | accepted |
| [0006](0006-sound-driver-as-built.md) | The sound driver as built | D8, §4.7 | accepted |
| [0007](0007-nitrosystem-files-in-host-structs.md) | NitroSystem's 2D files in host structs | D3, D5, §4.5 | accepted |
| [0008](0008-particle-work-from-the-platform-layer.md) | Particle work from the platform layer | D5, §4.5, §4.8 | accepted |
| [0009](0009-gamepads-by-position-one-pad-at-a-time.md) | Gamepads by position, one pad at a time | §4.5, §7 | accepted |
| [0010](0010-macos-app-as-a-signed-copy-of-the-program.md) | The macOS app as a signed copy of the program | D11, §7 | accepted |
| [0011](0011-cartridge-infrared-chip-as-a-register-model.md) | The cartridge's infrared chip as a register model | §4.5, D3 | accepted |
| [0012](0012-test-launcher-for-the-field-menus-apps.md) | Frigate's test launcher for the field menu's apps while overlay 36 is claimed | §7 phase 4, §6 | accepted |
| [0013](0013-windows-build-with-msys2-clang64.md) | Windows built with MSYS2's CLANG64, its differences behind port/hal/host | D4, §4.4, §7 phase 5 | accepted |
| [0014](0014-sanitized-build-game-heaps-poisoned-ds-isms-ignorelisted.md) | The sanitized build: the game's heaps poisoned in place, the DS-isms ignorelisted | §4.5, §6, §7 phase 5 | accepted |
| [0015](0015-universal-macos-app-for-macos-11.md) | A universal macOS app for macOS 11, with the pinned SDL3 linked statically | D11, §7 | accepted |
| [0016](0016-3d-frame-in-bands-on-worker-threads.md) | The 3D frame drawn in bands on worker threads, the game on one | §4.6, §7 phase 5 | accepted |
| [0017](0017-external-contributions.md) | External contributions through forks and pull requests | D1, D10, §9 | accepted; amended by [0018](0018-protected-main-and-sync-prs.md) |
| [0018](0018-protected-main-and-sync-prs.md) | Protected main and reviewed upstream syncs | D1, §4.1, §8 | accepted |

## Decisions waiting for an ADR

[PLAN.md §3](../PLAN.md#3-decisions) states these decisions and their status, with the evidence in
[RESEARCH.md](../RESEARCH.md); the titles below only name them, so that this index doesn't drift when the plan's text
changes. Each deserves an ADR when the work it governs starts, or sooner if someone questions it.

| Plan | Decision | Write by |
| --- | --- | --- |
| D2 | Our own platform layer, not libntr | phase 0 |
| D4 | Languages, compiler and libraries | phase 0 |
| D5 | 64-bit native build | phase 0 |
| D6 | One executable for both versions | phase 5; 0020 is reserved for it |
| D7 | DS-compatible saves with a sidecar file | phase 1 |
| D9 | Mods as a first-class design | phase 0 (hook macros), phase 6 (the rest) |
