---
name: decomp-file
description: Decompile one of the game's original source files in pokebw2 end to end. Covers finding its boundaries and name, disassembling it, writing the C in address order, probing every function against the ROM, registering it in both versions' delinks, then handing off to finish-file to verify, document and commit. Use when starting or continuing decomp work ("continue", "next file", "decompile ov033", "do gf_font.c"), or when picking what to work on next.
---

# Decompile a file

One original file per pass, committed when done, then on to the next in address order. `docs/decompiling.md`,
`docs/matching.md` and `docs/code-organization.md` hold the rules; this is the order to apply them in.

## 0. Orient (1 minute, no questions)

- `git status --short` and `git log --oneline -5`. Uncommitted files you didn't write belong to another session, so
  leave them alone. Check whether you are in a worktree (`git worktree list`).
- The workstream's memory file says what was done last and what is next. The next file is the one after the last
  committed file in address order, unless the user named one. Don't ask which file to do.

## 1. Boundaries, name and place

Follow [boundaries.md](boundaries.md). For a whole overlay, or a range of more than about 20 functions, delegate to the
`boundary-scout` agent, which returns the file list with evidence and the `add_source_file.py` commands, so the
long `source_files.py` output stays out of this context.

## 2. Asm

```sh
tools/dsd dis -c config/b2_us/arm9/config.yaml -a build/asm --overlay 33    # or --main; once per module
.venv/bin/python tools/decomp/show_func.py FUNC_A FUNC_B                  # never cat the .s file
```

The asm uses the current symbol names, so run `dsd dis` again after renaming. Read functions a few at a time, in the
order you write them.

## 3. Context before writing

- Names: `grep -n 'ADDR\|NAME' config/b2_us/arm9/overlays/ov033/symbols.txt` (or `arm9/symbols.txt` for main) for
  swan's names. `grep -rn NAME include/` for existing prototypes and structs. Fix a wrong prototype in its owner's
  header and update the callers; never declare it again locally.
- Structs: when a work struct or a struct shared by several functions has no layout yet, let the `struct-recovery`
  agent derive it from every access. Small, file-local works can be read off directly.
- Constants: items, moves, species, types, sounds, trainer classes, trainers, zones, wild encounter tables, event flags
  and event variables are lists in `data/constants/`, which the build turns into `constants/*.h` headers; add a
  missing one to its list, never to a header, and rename one with `rename_constant.py`, as when a numbered zone turns
  out to be a named room. A member of an archive that the code loads by number goes in `narc_<archive>.txt` once the
  code shows what it is (see `docs/data.md`, Constant lists). `include/constants/` holds the hand-written ones,
  `HEAPID_*` and the like.
  A message file is a `TEXT_BANK_*` (system messages) or `SCRIPT_TEXT_*` (script messages), never its number:
  `GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, heapId)`.
  A message is its ID from the bank's header, `text/system/<bank>.h`, never its line number:
  `GFL_MsgDataLoadStrbufNew(msgData, YesNo_Text_Yes)`.

## 4. Write the C

Write directly from the asm; this repo has no m2c for ARM. Per file:

- Includes: its own header first among the project headers, and only what it uses. A file includes its own header,
  so definitions are checked against declarations.
- Functions in address order (reverse for SPL). Static functions get prototypes at the top (`-requireprotos`), but
  only after checking that no other module references them (`grep` the address in every `relocs.txt`, or let
  `mark_complete.py` refuse later). A static referenced from another module links to address 0.
- Public functions, data and callback tables go in the owning header. No `extern` or other files' prototypes in a
  `.c` file. `struct_decls.h` gets `typedef struct X X;`, and the owner's header gets the layout.
- Event callbacks take `void *data` and cast it. Data in `.data` is not `const`; data in `.rodata` is `static const`
  or `const`.
- Write natural C, as a Game Freak programmer would have. Avoid decompiler leftovers: `for` loops kept as `while`
  with a guard, gotos for early returns, duplicated temporaries, shifts for divisions, casts the types already give,
  and `unk_` names where swan or the code's meaning gives one.
- Name what the code shows. A guessed name that only rests on the code is fine if it is descriptive. Record names with
  `rename_symbol.py` (it updates both versions and `config/names.txt`), never by editing `symbols.txt`.

Write every function of the file first, matching or not. Then grind; the whole file in a rough state beats one
perfect function.

## 5. Probe

```sh
.venv/bin/python tools/decomp/compiler_probe.py src/ov033/x.c --compilers 1.1p1 --mismatches
```

Library code built with another compiler, such as SPL with `1.2/base` (`lib/spl/library.toml`), gets its
compiler by default; leave out `--compilers`. The probe ignores relocated bytes, so a wrong call target or addend
only shows in `ninja`'s module check.

For each function left over, use the `match-function` skill: triage, a few rounds of levers, then stop under its rules.
For several stubborn functions in different files, start one `function-matcher` agent per file, never two on the same
file.

## 6. Register

```sh
.venv/bin/python tools/decomp/add_source_file.py src/ov033/x.c overlays/ov033 .text:0x...-0x... .rodata:... .data:... --incomplete
```

Leave out `--incomplete` only when everything matches; `mark_complete.py` is safer later (see finish-file). Run
`python3 configure.py` after adding a file, since `build.ninja` lists the sources.

## 7. Finish

Use the `finish-file` skill: both versions, `ninja`, the nonmatching rows, lessons, commit. Then go on to the next
file.
