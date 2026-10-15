---
name: code-audit
description: Audit pokebw2 code against CLAUDE.md and docs/code-standards.md, then fix every finding, in a loop until a confirmation round is clean. Partitions the scope across the specialized reviewers (game, library, config, data, tools), keeps its findings in .claude/audit-state.json, and a Stop hook holds the session to it until it is done. Use for "audit", "review this PR", "check this contributor's work", "clean up X", or a scope of files, a directory, a branch or a pull request.
---

# Code audit

## Rule 0: don't stop until it is done
Invoking this skill authorizes every pass. Don't end a turn, ask whether to continue, or hand back a partial result
while `.claude/audit-state.json` says `complete: false`: the Stop hook blocks it. The only ways out are step 7's final
report (`complete: true`), or a real decision for the user (a placement with no evidence, a change of scope, a
workaround that would send a complete file back to partial when the user hasn't ruled on it): then ask, with
`awaiting_user: true` in the state.

Phrases that are not reasons to stop: "pre-existing, not introduced here", "a natural stopping point", "the rest is
minor", "this would take many more passes". A finding outside the scope is recorded as `descoped` with a reason, not
dropped.

## 1. Scope and baseline
- The scope is what the user named: a pull request (`gh pr view N --json files,headRefName,headRepositoryOwner`), a
  branch, a contributor's merged work (`gh pr list --author USER --state all`), files, or a directory.
- Work in a worktree (`git worktree add -b NAME .claude/worktrees/NAME origin/main`, or the pull request's branch,
  fetched from its fork when `maintainerCanModify` is true; link `orig/`, `.venv`, `extract`, `tools/dsd`,
  `tools/objdiff-cli`, `tools/wibo`, `tools/sjiswrap.exe`, `tools/mwccarm`). Build it once: the probes need the
  generated headers.
- The baseline is the commit the audit measures from. For a contributor's merged work, review the files as they are
  now, their hunks for files they only touched (`gh pr diff N`), whole files they created.

## 2. State file (binding)
`.claude/audit-state.json` in the worktree (ignored by git), written before the first pass and after every change:

    {"scope": "...", "worktree": "...", "baseline": "SHA", "phase": "A", "pass": 1, "complete": false,
     "awaiting_agents": false, "awaiting_user": false,
     "findings": [{"id": 1, "file": "src/x.c", "line": 10, "rule": "1", "severity": "HIGH", "what": "...",
                   "fix": "...", "status": "open", "pass_found": 1, "descope_reason": null}]}

`status` is `open`, `fixed`, `kept` (checked, and the code is right: say why in `fix`) or `descoped` (outside the
scope, with `descope_reason`, and at least one pass old). "Am I done?" is a query on this file, not a feeling.

## 3. Phase A: audit, read-only
1. Mechanical checks over the scope: `tools/decomp/check_style.py FILES` (or `--diff BASELINE`), clang-format
   `--dry-run --Werror`, `tools/decomp/apply_names.py --dry-run`, `diff -r .claude/skills .agents/skills`, the
   compiler probes of every C file (both versions). Each result is a candidate until the code is read.
2. Partition the files by area and dispatch the reviewers, at most two at a time (more get the terminal killed;
   set `awaiting_agents: true` while they run): `pbw-game-reviewer` (src/, include/), `pbw-library-reviewer` (lib/),
   `pbw-config-reviewer` (config/, docs/nonmatching-functions.md, docs/source-files.md, docs/matching.md),
   `pbw-data-reviewer` (data/, tools/data/), `pbw-tools-reviewer` (tools/, CI, .claude/, .agents/, .codex/). Give
   each its files, the baseline, the candidates of step 1 in its area, and for a contributor's work the PRs
   behind the files.
3. Verify before recording: a reviewer's `HYPOTHESIS` is checked (read the code, probe, grep) before it becomes a
   finding; check who wrote it (`git blame`) when the scope is one contributor's work, and descope the rest with the
   reason.
4. Record every finding in the state. Phase A changes no file (`git diff --quiet` at its end).

## 4. Phase B: fix everything, then confirm
Fix the open findings, most severe first, delegating per file to `function-matcher` or `file-writer` (one agent per
file, at most two at a time) with `.claude/skills/code-standards` as the rule set and the finding list:
- **Natural C wins.** A function that matches only through a workaround keeps the natural C and gets a row in
  `docs/nonmatching-functions.md`; a complete file then loses its `complete` in both versions. Ask the user before
  the first such loss in an audit, unless the user already ruled (memory: natural-c-decisions).
- **Keep what is right**: a kept goto, `volatile` or pointer sum is `kept` with its proof (the probe that breaks
  without it), and gets a comment when the rules want one.
- **Configs only through the scripts**; renames through `rename_symbol.py` (never two at once); rows in the present
  tense; `docs/source-files.md` regenerated for the overlays whose files changed.
- Formatting: `clang-format -i` on the changed files, guards around `asm`, hand-laid-out tables and `GFL_ASSERT`s
  with operators.
Then build: `python3 configure.py && ninja`, no warnings, both SHA1s. A module check failure after formatting is a
reformatted `GFL_ASSERT` or `asm` until shown otherwise.

**Confirmation round:** step 3's checks and the reviewers again on the files Phase B changed (fresh reads, not the
earlier reports). New findings go in the state and get fixed; repeat until a round finds nothing. Increment `pass`
each round.

## 5. Verify the message against the diff
Before committing, check every claim of the commit message against `git diff --cached` (a count, a name, "matches",
"complete"). Stage by name.

## 6. Publish
Per CLAUDE.md: `git fetch origin && git merge origin/main`, `apply_names.py --dry-run` empty, rebuild, both SHA1s,
then push (`origin HEAD:main`, or the pull request's fork branch). Comment on the pull request or the overlay issue
with what changed and why, rule by rule.

## 7. Final report
Set `complete: true`, then report: the scope, the findings by rule with what was fixed, kept (and why) and descoped
(and why), the functions that lost a match and their rows, the commits, and anything for the user to decide.
