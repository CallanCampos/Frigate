# Contributing

Thanks for helping. This project turns Pokémon Black 2 and White 2 back into C that compiles to the original ROMs
byte for byte, so that the game can be read, modded and eventually ported. This page has the rules every change
follows; [the docs](README.md#documentation) have the details.

## Getting started

1. Set up the build as in the [README](README.md#setup), with your own dumps of the games, and check that `ninja`
   ends with both ROMs matching.
2. Open the project in [objdiff](https://github.com/encounter/objdiff). `ninja` writes `objdiff.json`, and objdiff
   rebuilds and diffs each function as you edit.
3. Pick something to work on. [Source files](docs/source-files.md) lists every overlay's original files and the
   [progress map on decomp.dev](https://decomp.dev/fuddlesworth/pokebw2) shows what is left. [Nonmatching functions](docs/nonmatching-functions.md) lists
   functions already in C that don't match yet, each with what was tried, if you like a puzzle. Opening an issue or
   a draft PR for a file you are taking avoids two people doing the same one.

## Decompiling a file

The [workflow](docs/decompiling.md#workflow) in short:

1. Find the original file's boundaries and its name, from a string the ROM embeds (such as `"resort_npc.c"`) or else
   from what it does. `tools/decomp/source_files.py ovNNN` helps; see
   [Code organization](docs/code-organization.md).
2. Add the file to both versions with `tools/decomp/add_source_file.py`.
3. Write the C, in the same order as the functions in the ROM. `tools/decomp/compiler_probe.py FILE --mismatches`
   shows which functions don't match yet, and `--show-diff` how. [How MWCC compiles](docs/matching.md) explains most
   of the differences you will see.
4. Once every function matches, mark the file complete with `tools/decomp/mark_complete.py`, run `ninja`, and check
   that both ROMs still match. Also probe White 2 with `--version w2_us`.

A file can be merged before it is complete: it stays without `complete` in `delinks.txt`, so the original code is
still linked, and objdiff shows how close it is.

## Rules

- **Both ROMs stay byte for byte.** Every PR ends with `ninja` passing, which compiles every source file, complete or
  not, and checks every module and both SHA1s.
  Black 2 and White 2 share the source, with `BLACK2`/`WHITE2` defines where they differ.
- **Change configs only with the scripts** (`add_source_file.py`, `mark_complete.py`, `rename_symbol.py`,
  `config_fixes.py`). They update both versions and keep the changes when the configs are regenerated.
- **One C file per original file**, with its functions in address order. Code built with the game's compiler goes in
  `src/`; libraries built apart, with their own compiler, go in `lib/`. See
  [Code organization](docs/code-organization.md).
- **Natural C only.** Write what a programmer at Game Freak could have written: no inline assembly, no
  `volatile` or pointer tricks that only exist to move a register, no meaningless temporaries or permuter output.
  When a function doesn't match, keep the closest natural C and add a row to
  [Nonmatching functions](docs/nonmatching-functions.md) with both versions' addresses, the difference and what you
  tried. In `lib/` only, a function the SDK itself wrote in assembly is written as an MWCC `asm` function, with a
  comment saying so.
- **Write the C from the assembly.** Other decompilations, such as pret's pokeplatinum and pokeheartgold, are good
  references for names and structure, but don't copy their code. Never use leaked or otherwise unlawfully obtained
  material, such as leaked source code, in any form.
- **Names:** use swan's names first (see [Names](docs/code-organization.md#names)). Name anything else through
  `tools/decomp/rename_symbol.py`, which records the name in `config/names.txt` and updates the source. Library
  code in `lib/` uses the SDK's own names where the code shows them, noting swan's name where it differs.
- **Bugs:** mark a bug in the game with `// BUG:` and a fix under `#ifdef BUGFIX`, with the original code in `#else`.
- **Never commit ROMs or anything extracted from them.** `.gitignore` covers `orig/`, `extract/` and `build/`.

## Style

Format the files you changed with `clang-format -i`, which uses `.clang-format`. Don't reformat files you didn't
touch: clang-format releases disagree, so check `git diff` for changes outside your code. Match the comment density
and naming of the code around yours.

## Commits and pull requests

- One original file, or one part of a large file, per commit, with both versions' configs and the docs it touched.
- Subjects like the history's: `Decompile btl_server_flow.c's HP recovery`, `Complete event_shortcut_menu.c`,
  `Match func_ov012_0215c160`, `tools: <what>`. The body says how many functions match, which mismatches are
  documented, and the reason for any guessed name.
- A PR states that `ninja` passes with both ROMs matching. Keep PRs to one topic so they are easy to review.

## Claude Code

The repository includes [Claude Code](https://claude.com/claude-code) skills and agents for this workflow, in
`CLAUDE.md` and `.claude/`. They follow the same rules as this page. Using them is optional.
