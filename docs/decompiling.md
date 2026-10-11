# Decompiling

How a function goes from assembly to matching C, and the tools for it. What makes MWCC's output match is in
[How MWCC compiles](matching.md), and how the source is organized in [Code organization](code-organization.md).

## Workflow

Matching is checked per function with [objdiff](https://github.com/encounter/objdiff). A default `ninja` also writes
`objdiff.json` for the first configured version, Black 2 by default, which the objdiff GUI opens from this directory.
The current C mismatches and attempted translations still in assembly are tracked in
[Nonmatching functions](nonmatching-functions.md).

1. Move a range of functions into a source file by adding it to the module's `delinks.txt`, as `src/ov004/event_worldtrade.c`
   is in `config/b2_us/arm9/overlays/ov004/delinks.txt`. Mark it `complete` once all its functions match. Add the
   same entry to `config/w2_us` with White 2's addresses, which `build/version_map.tsv` lists.
2. Write the C code. objdiff rebuilds the object with ninja whenever a source file changes, and diffs every function
   against the original.
3. objdiff can also create a decomp.me scratch for a function. The scratch uses compiler `mwcc_40_1024` (dsi/1.1p1),
   and a context file preprocessed from the source.

`ninja progress` prints how much of the game matches, from the report at `build/b2_us/report.json`.
CI's `build.yml` builds each version on every push to main and uploads its report as the artifact `b2_us_report` or
`w2_us_report`. [decomp.dev](https://decomp.dev/fuddlesworth/pokebw2) reads them, and the README shows its treemap and
badges, so there is nothing to update by hand. `objdiff_config.py` gives each unit progress categories, which become
decomp.dev's separate bars: ARM9 main or the overlays, and the game's code or each library of `lib/`.

`tools/decomp/add_source_file.py` adds a source file to both versions' `delinks.txt`, with White 2's ranges taken
from the version map. `tools/decomp/compiler_probe.py src/... --compilers 1.1 --show-diff 1.1` compiles a file and
diffs every function in it against the original. A library under `lib/` that has its own compiler, such as SPL
with `1.2/base`, gets the compiler and flags of its `library.toml` by default. `--mismatches` leaves out the functions that
match, and `--functions` limits the table and diffs to the functions named. `--align` shows only the hunks of the
diff that differ, aligned so that an instruction more or less does not shift everything after it, which is what makes
a long function's diff readable. A function the file defines but the object lacks shows as `not emitted`: MWCC drops a
`static` function nothing references, such as a callback whose table isn't written yet, so it has not been checked.
A function the C leaves out entirely doesn't show at all, so compare the count of functions with the original object's
before marking a file complete: `mb_comm_sys.c` (overlay 181) linked without `MBComm_SendParentInfo`, whose callers in
other files then went through a veneer to address 0, which shifted the rest of the overlay.

`tools/decomp/try_variants.py src/... FUNC variants.c` puts each variant of a function, separated by lines of
`=====`, in place of its definition and probes it with the file's compiler, keeping the first that matches. `--score`
adds each variant's count of differing aligned lines, to tell apart variants of the same size.
`rename_symbol.py --file` renames each `old new` pair, one per line, of a file.

`tools/decomp/locals.py src/... FUNC` prints the register or stack slot of each parameter and local of a function,
from the debug info of its built object, which shows which variable to move when registers or stack slots differ.

## Compiler

The game code was built with CodeWarrior for DSi, a version between `dsi/1.1p1` and `dsi/1.3p1`, and
`configure.py` uses `dsi/1.1p1` (build 1024, `mwcc_40_1024` on decomp.me):

- `dsi/1.6sp1` and `dsi/1.6sp2` do not match overlay 4's switch statement.
- `dsi/1.1` differs from the later builds in one way: after a store to a field, it reuses the stored register where
  the game reloads the field (`strb r0, [r4, r7]; ldrb r0, [r4, r7]`). That reload is the compiler, not `volatile`.
- `dsi/1.1p1` through `dsi/1.3p1` produce identical code for every game function tried so far.

The ROM was not necessarily built with one compiler. The Pokémon Black decomp by Goldoire found library code in Black
built with other versions (`2.0/sp2p2` and `1.2`), so try them on library code that doesn't match. `configure.py`
extracts only the `dsi` compilers; the others are in `build/mwccarm.zip`. Extracted to `tools/mwccarm`, they can be
named by directory, as in `compiler_probe.py --compilers 2.0/sp2p2`, and `1.2` needs `--flags` without `-ipa file`.
`configure.py` downloads a library's compiler when a `lib/*/library.toml` names it.
Game code needs the `dsi` builds: the evolution demo's view matches 36 of its 56 functions with every `2.0` build and
25 with `1.2`.

`tools/decomp/compiler_probe.py` compiles a C file with every version and compares each function against the
game, ignoring relocated bytes. For example:

```sh
.venv/bin/python tools/decomp/compiler_probe.py tools/compiler_tests/main_loops.c
```

It needs `pyelftools`, `capstone` and `pyyaml`. To look at a function's disassembly, run `dsd dis` into
`build/asm`, then use `tools/decomp/show_func.py`.
