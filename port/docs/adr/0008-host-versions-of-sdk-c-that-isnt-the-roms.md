# 0008: Host versions of SDK C that isn't the ROM's

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D3, §4.5; amends [0005](0005-platform-boundary-as-built.md)
- **Upstream:** `e484d5011` (merge of PR #353)

## Context

[ADR 0005](0005-platform-boundary-as-built.md) keeps the SDK's own C wherever it runs on a host and replaces only what
can't: MWCC `asm` functions, files bound to the ARM9's hardware, and files that keep DS addresses. That assumes the C
does what the ROM does. In an incomplete file it needn't: the ROM build links the original object, so nothing checks
the C, and the decomp lists the functions that don't match in `docs/nonmatching-functions.md`.

The oracle comparison of the title screen found four such functions in NitroSDK's matrix files: `MTX_Concat33`,
`MTX_Concat43`, `MTX_Concat44` and `MTX_MultVec43`. Each is plain C that compiles and runs on a host, and each is a
nonmatching row. The originals load each row of the first argument before they store over it; the C reads it again
after storing, which a compiler must allow for when the result is the same matrix. The SDK allows that, and the game
does it (`MAT3_RotationEulerZYX` in `src/gfl/calctool.c` concatenates into its own matrix), so on a host the title's
camera looked elsewhere ([decomp-findings.md](../decomp-findings.md), [STATUS.md](../STATUS.md), 2026-10-17).

The port can't change the decomp's C: matching is decomp work, and only the user decides whether a decomp session
takes a finding up ([port/CLAUDE.md](../../CLAUDE.md)). Copying the file into `port/` to fix it is forbidden.

## Decision

- **The HAL may replace a function of the SDK's C** (NitroSDK, NitroSystem) that runs on a host when two things hold:
  the ROM doesn't link the C (the function is a row of `docs/nonmatching-functions.md` in a file that isn't complete),
  and the asm oracle (`port/tests/asm_oracle`) shows it gives other results than the ROM's code on inputs the game
  gives it.
- **Each replacement comes with**:
  - a host version in `port/hal/<module>` that does what the original's machine code does;
  - the SDK's function behind `#ifndef PLATFORM_PC`, with a comment, as a Platform game change with its row in
    [game-changes.md](../game-changes.md);
  - a differential test of the host version against the ROM's code that covers the inputs where the decomp's C
    differs;
  - a row in [decomp-findings.md](../decomp-findings.md) that says how the C differs;
  - an entry in `port/tests/hal/check_replaced_c.py`.
- **A replacement goes when upstream's C is the ROM's.** When the function's row leaves
  `docs/nonmatching-functions.md` or its file becomes complete, the function matches the ROM, so its C is the
  original's. `check_replaced_c.py`, the ctest test `hal_replaced_c`, fails then. In the commit after the sync that
  brings it, the asm oracle's test runs on upstream's C (its guard removed), and the host version, the guard, the
  ledger row and the list entry go. A C fix that upstream makes without matching leaves the replacement in place; its
  differential test keeps it equal to the ROM's code either way.
- **Today:** the four matrix products, in `port/hal/fx` (`fx_mtx33.c`, `fx_mtx43.c`, `fx_mtx44.c`), tested by
  `test_matrix_products` in `port/tests/asm_oracle/test_fx.py`.

## Alternatives

- **Keep the decomp's C and accept the difference:** the title's 3D camera, and any other concatenation into its own
  matrix, would differ from the DS's.
- **Change the decomp's C under `PLATFORM_PC` in place:** a second copy of the function inside the game file, harder
  to see and to drop than a HAL file, and still a game change.
- **Fix the C upstream first:** decomp work that only the user starts. The port would wait on it, and a fix that
  doesn't match may never be taken.

## Consequences

- The HAL replaces SDK C, not only asm. Each replacement has the same proof as replaced asm (a differential test
  against the ROM's code) and a test that says when it can go.
- Other incomplete C may hide the same kind of difference: reading through one pointer after storing through another
  that can be the same object. Only the oracle comparisons and the asm oracle find it; the port has no general search.
- Revisit when upstream's matrix files become complete, or if the replacements grow beyond a few functions, which
  would argue for a decomp session on the files instead.
