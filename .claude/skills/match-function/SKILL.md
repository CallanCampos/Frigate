---
name: match-function
description: Get one pokebw2 function that is already written in C to match the ROM byte for byte under MWCC. Classify the difference, look up the levers for that symptom, try respellings in scored batches, inspect where locals were allocated, use the capped permuter as a hint generator, and stop by fixed rules, documenting the closest natural C. Use when compiler_probe shows a mismatch, when the user asks to match or fix a function, or for a mismatch pass over docs/nonmatching-functions.md.
---

# Match a function

The aim is the C that Game Freak wrote. A match through unnatural C is not a match: the levers below are what real
source looks like, and the permuter only points at them.

## 1. Triage: what kind of difference

```sh
.venv/bin/python tools/decomp/compiler_probe.py src/X.c --compilers 1.1p1 --functions F --show-diff 1.1p1 --align
```

Leave out `--compilers` for library code with its own compiler. Classify by what `--align` shows:

| Class | Looks like | Work on |
| --- | --- | --- |
| Structural | sizes differ by more than a few instructions; branches, loops or blocks differ | control flow first: loop form, switch vs `if` chain, early returns, merged tails, inline helpers |
| Extra or missing instruction | ±1 to 3 instructions: a narrowing shift, a reload, a `mov`, a recomputed address | types (`u8`/`u16`/`int`, signedness, enum), `const`, a value in a local or not |
| Scheduling | same size and instructions, in another order | `const` pointees, statement order, where locals are initialized, inline helpers |
| Registers | same instructions, registers swapped | declaration order, use counts, where a variable is first assigned, split variables |
| Stack | same code, other `sp` offsets or frame size | first-assignment order, block-scoped variables, spills |
| Probe matches, `ninja` fails | relocations: wrong callee, addend or runtime helper, or data in the wrong section | the `fix-build` skill |

Fix them in that order. Registers and stack slots move with every structural change, so don't tune them first.

## 2. Look up the levers

[levers.md](levers.md) indexes every known lever by these classes, with the passage of `docs/matching.md` that explains it. Read the
section for the class, not the whole file.

For registers and stack slots, see where MWCC put each variable instead of guessing:

```sh
.venv/bin/python tools/decomp/locals.py src/X.c F      # register or sp+offset of every param and local, by line
```

Compare it with the original's use of each register or slot in the asm, and move the one variable that is off.

## 3. Try in scored batches

Write 3 to 10 respellings at once in a scratchpad file, separated by lines of `=====`, and score them:

```sh
.venv/bin/python tools/decomp/try_variants.py src/X.c F --score /path/to/scratchpad/F_variants.c
```

It keeps the first variant that matches, and otherwise restores the file. `--score` counts differing aligned lines,
the measure of progress for variants of the same size. Keep the best variant with `--keep N` when it is better and
still natural. Change one idea per variant, so the score says which idea mattered.

## 4. Stop rules

Stop and document when any of these holds:

- Three batches in a row didn't improve the best score.
- Only a register swap or stack slot rotation is left, and `locals.py` plus a sweep of declaration orders didn't
  move it.
- About 20 minutes on one function, or 45 on one file's leftovers. Move on; the next mismatch pass may bring a new
  lever.
- The only matching spelling is unnatural (see below).

Before stopping on scheduling or registers, the capped permuter may run in the background while you work on other
functions; see [permuter.md](permuter.md). It finds hints, not source.

## 5. Document

Keep the closest natural C in the file. Add or update its row in `docs/nonmatching-functions.md`, using the table as
it stands in this checkout. A row gives:
- both addresses, from `config/b2_us/...` and `config/w2_us/.../symbols.txt`;
- the size against the original, and how many bytes differ;
- what the original does differently, in the asm's terms;
- what was tried.

Remove the row when the function matches. Tell the user which rows you added or removed.

If a lever worked that isn't in `docs/matching.md`, or a claim there turned out wrong, use the `record-lesson` skill.

## Natural C

Reject these, from the permuter or anywhere else:
- inline asm;
- `volatile` without a hardware reason;
- an unused or uninitialized variable, or a temporary whose address is taken for no reason;
- `do {} while (0)`;
- arithmetic on casts that the types don't call for;
- the same expression split into pointless temporaries;
- code moved into a branch where it doesn't belong;
- a type that breaks the code's meaning, such as a `u8` that would clip a value.

A `goto` is fine where it is the plain way to say the flow, such as one shared exit. Static inline helpers are fine;
Game Freak used many.

## Delegating

For a function with a long diff, or several stubborn functions, use the `function-matcher` agent: one per source
file, since `try_variants.py` edits the file in place. Give it the class from the triage and what was tried. It returns
the best variant and a ready row.
