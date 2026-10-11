---
name: finish-file
description: Close out a pokebw2 source file after its C is written. Probe both versions, mark it complete if everything matches, build and verify both SHA1s, update docs/nonmatching-functions.md and the lessons, review, commit only this file's changes in the project's message style, then merge origin/main, rebuild and push to origin main. Use when a file is done or at a stopping point ("commit", "wrap up", "finish this file"), and before moving on to the next file.
---

# Finish a file

## 1. Probe both versions

```sh
.venv/bin/python tools/decomp/compiler_probe.py src/X.c --compilers 1.1p1 --mismatches
.venv/bin/python tools/decomp/compiler_probe.py src/X.c --compilers 1.1p1 --mismatches --version w2_us
```

Leave out `--compilers` for library code. Every function that doesn't match in either version needs a row (step 4).

## 2. Mark complete, if everything matches

```sh
.venv/bin/python tools/decomp/mark_complete.py src/X.c
```

It refuses while a function the file defines as `static` is referenced from another module. Make that function global
(and declare it in the header) rather than forcing it.

## 3. Build and verify

```sh
python3 configure.py && ninja 2>&1 | tail -20      # configure only if source files were added
```

A clean `ninja` has passed every module check and both SHA1s. On any failure, use the `fix-build` skill. A file that
isn't complete still builds from the original code. Its C is only probed, so it can't break the ROM, but its header
changes can.

If you formatted, it was `clang-format -i` on your files only. Check that `git diff --stat` touches nothing else.

## 4. Document

- `docs/nonmatching-functions.md`: add a row for each new mismatch and update or remove rows for functions that now
  match or whose difference changed. Follow the table format as it is in this checkout.
- A lever that worked, or a `docs/matching.md` claim that turned out wrong: the `record-lesson` skill.
- The workstream's memory file: what is done, and what is next.

## 5. Review (files of more than about 10 functions, or anything that changed shared headers)

Start the `decomp-reviewer` agent with the file and the diff. Fix what it finds that is real, and say what you
rejected.

## 6. Commit

Stage by name: the source, its header, `struct_decls.h`, configs of **both** versions, `config/names.txt` and
`config/fixes.txt` if touched, and the docs. Never `.venv`, `tools/mwccarm`, `build/`, or files another session left
uncommitted.

Subject in the branch's style:
- On `main` (game code): `Decompile btl_server_flow.c's HP recovery`, `Complete event_shortcut_menu.c`,
  `Match func_ov012_0215c160`.
- On the graphics branch (libraries): `GFL: <what> (<file>.c)`, `SPL: ...`, or
  `Text printing (printsys.c, incomplete)`.
- Tools: `tools: <what>`.

The body says how many functions match ("23 of 25 functions match"), which mismatches are documented, header moves,
splits and renames with their reasons, and guessed names. End it with the `Co-Authored-By` trailer the session
specifies.

## 7. Publish

Every commit goes to `origin/main` at once, so the sessions working in parallel stay close to one another:

1. `git fetch origin && git merge origin/main`, in a worktree into its own branch.
2. Resolve conflicts by keeping both sides' work: another session's file is theirs, so take their version of it and
   redo only your own change on top. Configs and docs that both sides edited usually need both sets of lines. Files
   that moved (such as `include/nitro/` to `lib/nitro/include/nitro/`) carry your edits to the new place.
   - When one side named functions the other still calls as `func_XXXXXXXX`, run
     `.venv/bin/python tools/decomp/apply_names.py`, which rewrites every `func_`/`data_` identifier to the name now
     in `symbols.txt` (`--dry-run` lists them).
   - When one side moved declarations to a new owner header, the other side's files fail with "function has no
     prototype": include the owner header, and point includes of deleted headers at their replacements. If both
     sides declared the same functions in different headers, keep the header of the side that decompiled the
     function and convert the other side's callers (types and field names too), then probe those callers: they must
     show only their documented rows.
3. `python3 configure.py && ninja`, and check both SHA1s. The default target also compiles every incomplete file in
   both versions, though only complete ones are linked, so a file that no longer compiles fails here instead of in
   the next session's build. A merge that breaks the build is fixed before it is pushed (the `fix-build` skill).
4. `git push origin HEAD:main`. If it is rejected because `origin/main` moved, go back to step 1.

Never force-push or rewrite pushed commits. If a conflict can't be resolved without knowing what the other session
intended, stop and ask the user.

## 8. Report and continue

Tell the user in one or two lines: the file, N of M matching, the rows added or removed, and the commit. Then start
the next file with `decomp-file`, without asking unless something real is blocked.
