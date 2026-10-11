# pokebw2

A matching decompilation of Pokémon Black 2 and White 2 (NDS/DSi, MWCC `dsi/1.1p1`), aiming at 100% C like
pokeemerald and then a native PC port. **The README and `docs/` are the source of truth**: setup in the README,
the workflow and tools in `docs/decompiling.md`, MWCC's behavior in `docs/matching.md`, files, headers and names in
`docs/code-organization.md`, and the configs in `docs/configs.md`. This file holds the rules that sessions get wrong
and points to the skills that carry the procedures. `CONTRIBUTING.md` has the same rules for human contributors: when
a rule here changes, change it there too.

## Rules

- **Both ROMs stay byte for byte.** Every change ends with `ninja`, which compiles every source file, complete or
  not, and checks each module and both SHA1s. Black 2 is
  primary. White 2 shares the source (`BLACK2`/`WHITE2` defines) and gets its configs through the version map, so
  change configs only with the scripts, which update both versions and record what they did:
  `add_source_file.py`, `mark_complete.py`, `rename_symbol.py`, `config_fixes.py`.
- **One source file per original file**, named after the ROM's embedded string, or descriptively, with the guess said
  in the header and the commit; never an overlay number. Functions go in address order, and in reverse for SPL,
  whose `1.2/base` compiler emits them reversed. `src/` is code built with the game's compiler: `src/ovNNN/` for overlays,
  `src/gfl` and `src/system` for main, by link order. Libraries built apart, with their own compiler, go in
  `lib/<name>/{include,src}` with a `library.toml`: `lib/spl`, `lib/dsprot`, `lib/nitro` (RC4 so far) and
  `lib/nnsys` (FND and GFD so far).
  `docs/code-organization.md` has the rest.
- **Names:** swan's first, marked as swan's in the header. Our own go through `rename_symbol.py`, which records them
  in `config/names.txt`. Types swan doesn't name are named after their owner. Rename a static's symbol to its C name.
  Library code in `lib/` (NitroSDK, NitroSystem, TwlSDK, DWC) uses the SDK's own names where the code shows them, even
  over swan's; the header notes swan's name where it differs. The user chose this.
- **Write C from the asm.** pret (pokeplatinum, pokeheartgold) and other decomps are references for names and
  structure, never code to copy. The user chose this explicitly for SPL.
- **Natural C only.** No inline asm, permuter noise, unexplained `volatile`, pointer-arithmetic tricks or meaningless
  temporaries. A function that doesn't match keeps the closest natural C and gets a row in
  `docs/nonmatching-functions.md` (both addresses, the difference, and what was tried). The one exception: in `lib/`,
  a function that the SDK itself wrote in assembly (an `asm` function in its C file, such as NitroSDK's `MTX_Identity43`
  or the CP15 cache functions) is an MWCC `asm` function, said to be one in a comment. The user chose this.
- **Bugs:** `// BUG:` plus an `#ifdef BUGFIX` fix, with the original in `#else`.
- **Keep going.** Continue in file order and commit each file as it is done. Don't end a turn with a menu when the next
  step is obvious. Ask only real decisions, such as placement with no evidence or a change of scope. Answer any message
  the user sends mid-turn.
- **Learn.** A trick that worked and isn't in `docs/matching.md` goes there, with an example. See the `record-lesson` skill.

## Environment

- Python is `.venv/bin/python`. The Bash tool's shell is not fish. Keep commands plain: shell variables, `$(...)`,
  loops and heredocs trip permission guards, especially in worktrees. Put logic in a script in the scratchpad.
- Other sessions work in this checkout and in `.claude/worktrees/*` at the same time. Stage files by name, never touch
  their uncommitted files, and don't use a bare `git stash`, since the stash is shared. `.claude/hooks/guard.py`
  enforces these.
- **Publish every commit.** Right after each commit: `git fetch origin && git merge origin/main` (in a worktree too,
  into its branch), resolve any conflicts, run `python3 configure.py && ninja` and check that both SHA1s match, then
  `git push origin HEAD:main`. `ninja` also compiles every incomplete file for both versions, so a file that no longer
  compiles fails the build even though it isn't linked. If the push is rejected because someone pushed first, fetch,
  merge, build and push again. Never force-push, and never push a merge that doesn't build
  both ROMs. The `finish-file` skill has the steps.
- **Context is the scarce resource.** Never print a whole `.s` file, a whole doc, a ninja log or a permuter log. Use
  `show_func.py NAME`, `compiler_probe.py --functions F --mismatches --align`, `grep -n` and `| tail`.
- Long jobs run with `run_in_background` or Monitor, never `sleep`. Run the permuter only under a memory cap: an
  uncapped `-j8` run got the terminal OOM-killed (see `.claude/skills/match-function/permuter.md`).
- A new worktree needs `orig/`, `.venv`, `tools/dsd`, `tools/objdiff-cli`, `tools/wibo` and `tools/mwccarm` linked
  in from the main checkout, which the ignore rules cover.

## Commands

| Task | Command |
| --- | --- |
| Disassemble a module (again after renames) | `tools/dsd dis -c config/b2_us/arm9/config.yaml -a build/asm --overlay 33` (or `--main`) |
| One function's asm | `.venv/bin/python tools/decomp/show_func.py NAME [NAME...]` |
| Probe a file against the ROM | `.venv/bin/python tools/decomp/compiler_probe.py src/X.c --compilers 1.1p1 --mismatches` |
| Diff one function | `... compiler_probe.py src/X.c --compilers 1.1p1 --functions F --show-diff 1.1p1 --align` |
| White 2 | add `--version w2_us` to the probe |
| Try respellings | `.venv/bin/python tools/decomp/try_variants.py src/X.c F --score variants.c` |
| Registers and stack slots of locals | `.venv/bin/python tools/decomp/locals.py src/X.c F` |
| File boundaries and names | `.venv/bin/python tools/decomp/source_files.py ov033 [--profile START END]` |
| Add a file to both versions | `.venv/bin/python tools/decomp/add_source_file.py src/X.c overlays/ov033 .text:A-B ... --incomplete` |
| Mark complete | `.venv/bin/python tools/decomp/mark_complete.py src/X.c` |
| Name a symbol | `.venv/bin/python tools/decomp/rename_symbol.py func_ov033_0217acd4 Name` |
| Rename a constant of `data/constants/` and its uses | `.venv/bin/python tools/data/rename_constant.py OLD NEW` |
| Progress of the local build | `ninja progress` |
| Build and verify | `python3 configure.py && ninja 2>&1 \| tail -20` (configure only when source files were added) |

## Skills and agents

Skills in `.claude/skills/`:
- `decomp-file`: the whole loop for one original file, from boundaries to commit.
- `match-function`: the triage-and-levers loop for a function that doesn't match, with its stop rules, the symptom
  index (`levers.md`) and the capped permuter (`permuter.md`).
- `fix-build`: a link, module-check, SHA1 or White 2-only failure.
- `finish-file`: verify both versions, mark complete, document mismatches, commit, then merge, rebuild and push.
- `record-lesson`: where a new lesson goes and how to write it.

Agents in `.claude/agents/`, used to keep large asm and diffs out of the main context. They run on cheaper models
(`model:` in each file): `boundary-scout` on Haiku, the rest on Sonnet; pass `model: "opus"` for a function worth it.
- `file-writer`: writes one original file's C from the asm and probes it; the caller registers, builds and commits.
- `function-matcher`: grinds one function. It owns that file while it runs.
- `boundary-scout`: proposes the files of a module range, read-only.
- `struct-recovery`: derives a struct's layout from every access to it, read-only.
- `decomp-reviewer`: an adversarial check of a file's diff before commit, read-only.
