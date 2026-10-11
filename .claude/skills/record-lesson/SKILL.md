---
name: record-lesson
description: Record what pokebw2 work taught, in the place it belongs. A proven MWCC behavior goes in docs/matching.md and the levers index, a function's failure in the nonmatching doc, a workflow failure in a skill or hook, and session status in memory. Use when a new lever made a function match, when a claim in docs/matching.md turned out wrong, when the same mistake or build failure happened twice, or when the user corrects how work is done.
---

# Record a lesson

Lessons that stay in a chat or a memory file are lost to the next session. Most of the graphics work's MWCC lessons
went only to memory, and the docs fell behind. Put each lesson where the next person, or session, will look.

## Where it goes

| Lesson | Where | Form |
| --- | --- | --- |
| How MWCC compiles something, proven on a real function | `docs/matching.md`, under the heading of its symptom, next to related bullets | The rule, then the function or file that shows it (below) |
| The same, as a lookup | `.claude/skills/match-function/levers.md`, under its symptom | One line, with `(matching.md: "phrase")` pointing back |
| Why one function doesn't match, and what was tried | `docs/nonmatching-functions.md` | A row in the existing table format |
| A build failure and its fix | `.claude/skills/fix-build/SKILL.md` table | Symptom, then cause and fix |
| A step that was missed or a convention that was broken | The skill that covers that step; a hook (`.claude/hooks/guard.py`) if it must never happen again | An imperative line with the reason |
| A useful scratchpad script | The fitting package of `tools/` ([tools/README.md](../../../tools/README.md)), with a docstring and usage like the others, and a mention in `docs/decompiling.md` | Commit it as `tools: ...`; the scratchpad is wiped between sessions |
| The workstream's status, plan or the user's choices | The memory file for that workstream | Dates as absolute dates |

## Prove it first

A rule in `docs/matching.md` must hold beyond the one function where you saw it. Before writing it as a rule:
- Check it on a second function, or on a minimal function in `tools/compiler_tests/` probed with `compiler_probe.py`.
- If it held once only, write it as what that function needed, not as a rule.
- A lever from another project (sm64ds, decomp-triage) or another compiler build is a hypothesis until it moves
  output here.

When a claim there turns out wrong or incomplete, correct the bullet. Don't add a contradicting one.

## Style of docs/matching.md

Match the bullets around it. Plain declarative present tense, as things the compiler does. No "we", no bold, no
"Note:". Code spellings in backticks. A concrete example from the game ("as `wbt_system.c`'s bracket code does").
One bullet per behavior, a few lines long, wrapped at 120 columns like the rest of the file.

> - A product assigned to a variable of its own goes to a new register, with its operand copied there first
>   (`mov r2, r1; mul r2, r0`), while a product used in place multiplies into the operand's register.

## Commit

Commit lessons with the work that found them, or on their own as `Document <what>` or `tools: <what>`. Skill and hook
changes go under `.claude/` in the same commit as the work that prompted them, or alone.
