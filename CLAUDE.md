# pokebw2

A matching decompilation of Pokémon Black 2 and White 2 (NDS/DSi, MWCC `dsi/1.1p1`), aiming at 100% C like
pokeemerald and then a native PC port. **The README and `docs/` are the source of truth**: setup in the README,
the workflow and tools in `docs/decompiling.md`, MWCC's behavior in `docs/matching.md`, files, headers and names in
`docs/code-organization.md`, and the configs in `docs/configs.md`. This file holds the rules that sessions get wrong
and points to the skills that carry the procedures. `CONTRIBUTING.md` has the same rules for human contributors: when
a rule here changes, change it there too.

## Rules

- **Both ROMs stay byte for byte.** Every change ends with `ninja`, which compiles every source file, complete or
  not, and checks each module and both SHA1s, and with no compiler warnings: `compile_all.py` fails on any, as CI
  does, and an incremental build hides those in files it didn't rebuild. Black 2 is primary. White 2 shares the source (`BLACK2`/`WHITE2` defines) and gets its configs through the version map, so
  change configs only with the scripts, which update both versions and record what they did:
  `add_source_file.py`, `mark_complete.py`, `rename_symbol.py`, `config_fixes.py`.
- **One source file per original file**, named in English after the ROM's embedded string (`pokedex_registration.c` is
  `pokedex_registration.c`, with the ROM's name in the header), or descriptively, with the guess said in the header and
  the commit; never an overlay number. Functions go in address order, and in reverse for SPL,
  whose `1.2/base` compiler emits them reversed. `src/` is code built with the game's compiler: `src/ovNNN/` for overlays,
  `src/gfl` and `src/system` for main, by link order. Libraries built apart, with their own compiler, go in
  `lib/<name>/{include,src}` with a `library.toml`: `lib/spl`, `lib/dsprot`, `lib/nitro` (RC4 so far) and
  `lib/nnsys` (FND and GFD so far).
  `docs/code-organization.md` has the rest.
- **Names:** in English, never a Japanese word or one of Game Freak's private abbreviations (`zukan` is `Pokedex`,
  `fldmmdl` `FieldActor`, `blact` `BillboardActor`); well-known acronyms such as BGM, SE, HP, TM and NPC stay. The user
  chose this on 2026-10-28. swan's first, translated the same way, marked as swan's in the header. Our own go through `rename_symbol.py`, which records them
  in `config/names.txt`. Types swan doesn't name are named after their owner. Rename a static's symbol to its C name.
  Library code in `lib/` (NitroSDK, NitroSystem, TwlSDK, DWC) uses the SDK's own names where the code shows them, even
  over swan's; the header notes swan's name where it differs. The user chose this.
- **Write C from the asm.** pret (pokeplatinum, pokeheartgold), ntrtwl's SDK reconstructions and other decomps are
  references for names and structure. A function may follow another decomp's structure where that is what matches,
  rewritten in this project's names, types and format and checked against the asm; never paste their code. The user
  chose this on 2026-10-18, after ntrtwl's NitroSDK shapes matched libfx.
- **Natural C only.** No inline asm, permuter noise, unexplained `volatile`, casts that reach a field by byte offset or
  meaningless temporaries. A pointer sum of the element's own type (`*(p + i + 1)`, `(p + n - 1)->next`) is plain C,
  which MWCC compiles differently from indexing: write it when it gives the original's address computation. A function
  that doesn't match keeps the closest natural C and gets a row in `docs/nonmatching-functions.md` (both addresses, the
  difference, and what was tried), even when that sends a complete file back to partial. `docs/code-standards.md` has
  every rule with its wrong and right forms; `tools/decomp/check_style.py` checks what a script can see, and CI's
  Standards workflow runs it, clang-format 23.1.1 and the skills' sync on every pull request. The exceptions, in `lib/` only:
  a function that the SDK, or a library it bundles, wrote in assembly (an `asm` function in its C file, such as
  NitroSDK's `MTX_Identity43`, the CP15 cache functions or DWC's big-number helpers) is an MWCC `asm` function, said
  to be one in a comment, and assembly the library inlined at several sites (DS Protect's cache flush) is a
  `static inline` with an `asm {}` block, said to be one in a comment. The user chose both. One game function is an
  `asm` function too: `src/gfl/arc_tool.c`'s `ArchiveMoveImageTop`, whose original build kept a dead store of
  `fimgOffset + 8` back to its stack slot that no installed compiler produces from the C; the maintainer took this
  exception for the last non-matching function on 2026-10-27, and the file says so. A library that is
  hand-written assembly as a whole, whose routines have several entries or names (MSL's runtime, `fp_fastI.s`), is an
  assembly file assembled by the mwasmarm of its compiler's version (library.toml's `asm_flags`); the user chose this
  on 2026-10-22.
- **Bugs:** `// BUG:` plus an `#ifdef BUGFIX` fix, with the original in `#else`.
- **Keep going.** Continue in file order and commit each file as it is done. Don't end a turn with a menu when the next
  step is obvious. Ask only real decisions, such as placement with no evidence or a change of scope. Answer any message
  the user sends mid-turn.
- **Claim an overlay before working on it.** For any overlay decompilation or matching work, find its open
  `[Decomp] Overlay NNN` GitHub issue, and for naming work its `[Names] Overlay NNN` issue (the `name-symbols` skill),
  and read its comments before editing or delegating edits. If someone has an
  active claim in a comment, choose another overlay; if the user specified that overlay, tell them it is claimed.
  Otherwise comment `Claiming overlay NNN for decompilation` with the file or range you will start with, recheck
  the comments for a concurrent earlier claim, then proceed. A claim stays active until its author explicitly
  releases it in a comment or the issue closes. Release your claim when leaving the overlay unfinished. See the
  `decomp-file` skill for the commands. This applies to direct `match-function` work too.
- **Record overlay progress.** After publishing each overlay decompilation or matching commit, comment on its issue
  with the file or range, matching progress, published commit and remaining work. Once all original ranges are
  recovered and every file is complete in both versions, verify both ROMs, post a final comment and close the issue.
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
- A new worktree needs `orig/`, `.venv`, `tools/dsd`, `tools/objdiff-cli`, `tools/wibo`, `tools/sjiswrap.exe` and `tools/mwccarm` linked
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
| What is left to name | `.venv/bin/python tools/decomp/naming_report.py [--module ov033]` |
| Script commands after renaming handlers | `.venv/bin/python tools/script/sync_handler_names.py` |
| Rename a constant of `generated/` and its uses | `.venv/bin/python tools/data/rename_constant.py OLD NEW` |
| Progress of the local build | `ninja progress` |
| Build and verify | `python3 configure.py && ninja 2>&1 \| tail -20` (configure only when source files were added) |
| Check for warnings, every file and version | `.venv/bin/python tools/decomp/compile_all.py -j 6 \| tail -3` |

## Skills and agents

Skills in `.claude/skills/`, with Codex copies in `.agents/skills/` that change with them in the same commit:
- `decomp-file`: the whole loop for one original file, from boundaries to commit.
- `name-symbols`: naming a decompiled module from its `[Names]` issue: address-named functions and data, `unk` fields,
  placeholder parameters, and bare numbers that should be constants, with `naming_report.py`.
- `match-function`: the triage-and-levers loop for a function that doesn't match, with its stop rules, the symptom
  index (`levers.md`) and the capped permuter (`permuter.md`).
- `fix-build`: a link, module-check, SHA1 or White 2-only failure.
- `finish-file`: verify both versions, mark complete, document mismatches, commit, then merge, rebuild and push.
- `record-lesson`: where a new lesson goes and how to write it. A "never again" lesson about code becomes a rule
  of `docs/code-standards.md` with a check in `tools/decomp/check_style.py` and a fixture, and a bad shape in the
  matching reviewer's list.
- `code-standards`: the C rules, short, pointing to `docs/code-standards.md` and the checker.
- `code-audit`: audit a scope (a pull request, a contributor's work, a directory) with the reviewers, then fix every
  finding in a loop until a confirmation round is clean. Its state is `.claude/audit-state.json`, and a Stop hook
  keeps the session on it until it is complete.

Agents in `.claude/agents/`, used to keep large asm and diffs out of the main context. They run on cheaper models
(`model:` in each file): `boundary-scout` on Haiku, the rest on Sonnet; pass `model: "opus"` for a function worth it.
- `file-writer`: writes one original file's C from the asm and probes it; the caller registers, builds and commits.
- `function-matcher`: grinds one function. It owns that file while it runs.
- `boundary-scout`: proposes the files of a module range, read-only.
- `struct-recovery`: derives a struct's layout from every access to it, read-only.
- `decomp-reviewer`: an adversarial check of a file's diff before commit, read-only.

Reviewers in `.claude/agents/review/`, one per area, for `code-audit`'s partitions and deep reviews, read-only, each
with evidence discipline (what it read against what it infers) and a list of known bad shapes that grows with each
real one: `pbw-game-reviewer` (src/, include/), `pbw-library-reviewer` (lib/), `pbw-config-reviewer` (config/ and the
decomp docs), `pbw-data-reviewer` (res/, tools/data/), `pbw-tools-reviewer` (tools/, CI, the agents' config).
Experts in `.claude/agents/experts/`, to consult, design or implement: `pbw-mwcc-expert` (matching under MWCC, on
Opus), `pbw-arm-expert` (reading the assembly), `pbw-sdk-expert` (the Nintendo SDKs' APIs and names).

Hooks in `.claude/hooks/`, with Codex copies in `.codex/hooks/`: `guard.py` blocks the process mistakes before a tool
runs; `post_edit.py` checks an edited C file against the standards and clang-format and tells the agent;
`audit_stop.py` holds a `code-audit` session to its loop.
