# Nonmatching functions

Every function that is in C but doesn't compile to the original. The addresses are Black 2's and White 2's.
`tools/decomp/compiler_probe.py FILE --version b2_us` (and `w2_us`) shows the difference of each function in a file,
and `--show-diff 1.1p1` the instructions.

## In C, not matching

None. Every function is in C and matches, or is an asm function: `arc_tool.c`'s `ArchiveMoveImageTop`, the maintainer's
exception for the last non-matching function, whose original build kept a dead store of `fimgOffset + 8` back to its
stack slot that no installed compiler reproduces from the C (see the file's comment).

## Keeping this list current

After changing a source file, run `compiler_probe.py` on it for both versions, and add, update or remove its rows. A file
gets `complete` in both versions' `delinks.txt` only when all its functions match and its data is written. The report at
`build/b2_us/report.json` helps find candidates, but its fuzzy match can be below 100 for a matching function because of
relocations, and misses some differences, so the probe decides. `tools/decomp/check_nonmatching_rows.py`, which CI's
Standards workflow runs, checks that each row's functions are defined in its file and are symbols of that file's module
(or of the LTD autoload, for a file with migrated LTD sections, `docs/configs.md`) at the row's Black 2 and White 2
addresses; `--fix` corrects a White 2 address that has one answer.
