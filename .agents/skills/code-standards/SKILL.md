---
name: code-standards
description: The C rules of pokebw2 (natural C over a match, control flow as it reads, no meaningless temporaries, macros as written, indexing or pointer sums but no byte-offset casts, declarations in the owner's header, volatile and asm only with a reason, comments that explain, formatting with guards, present-tense rows). Use when writing, matching or reviewing C in src/ or lib/, and before committing it.
---

# Code standards

The rules, with their wrong and right forms and when they don't apply, are in
[docs/code-standards.md](../../../docs/code-standards.md). Read the rule in doubt there. In short:

1. **Control flow as it reads.** No `goto` that only jumps to a return or rebuilds the compiler's shared tails, nor a
   `static inline` written so that its `return` makes the same jump. Not for a shared cleanup or error exit, a retry,
   leaving a search loop to skip its fallback, or a labelled tail block that does real work for several paths
   (`goto TRANS_NONINTERP` in `nsbca.c`).
2. **No meaningless temporaries.** No local that only steers registers, no variable set twice to pick a branch, no
   expression split to stop a fold. Not for a local that names something.
3. **Macros as written.** No hand-expanded `FX32_CONST`, `NELEMS`, `GX_RGB` or header inline.
4. **Indexing or a pointer sum of the element's own type**, never a field reached by a cast and a byte offset. Not for
   untyped bytes read through a `u8 *`.
5. **Declarations in the owner's header**: no `extern` or another file's prototype in a `.c`. A file's own data is
   defined in it from the ROM's bytes.
6. **`volatile` only where the hardware or another thread needs it**, proven by a probe, with a comment.
7. **No `asm` in `src/`.** In `lib/` only for code the SDK wrote in assembly, with a comment, guarded from clang-format.
8. **Comments explain the code**: no `TODO`, no narration of the matching.
9. **Names**: swan's, the SDK's in `lib/`, ours through `rename_symbol.py`; no stale `func_`/`data_` names.
10. **Bugs**: `// BUG:` with an `#ifdef BUGFIX` fix.
11. **clang-format 23.1.1** on every file changed; `asm`, hand-laid-out tables and `GFL_ASSERT`s with operators guarded.
12. **Nonmatching rows** in the present tense: what differs now and what was tried, no history.
13. **The Codex copies** of skills and hooks change with them.
14. **The SDK's types**: `u32`/`s32` are `long`, the SDK's constants as the SDK has them; a declaration takes the
    original's type (`int`/`unsigned int` where it compiles so), never a cast or a changed typedef.
15. **Structure**: a value set (sequence numbers, results, kinds, modes) is an `enum` and its constants are passed,
    not numbers; a parameter takes the struct it always points to, not `void *`; a type is defined once, in its
    owner's header; a table of records is an array of a struct. Storage keeps its original type.

Natural C wins over a match: a function that matches only through a workaround keeps the natural C and gets a row,
and a complete file goes back to partial.

Check: `.venv/bin/python tools/decomp/check_style.py --diff origin/main` lists candidates for 1, 4-8, 10 and 12 on the
lines a branch adds (a candidate is not a finding until the code is read; a justified exception takes
`// style-ok: RULE reason`); 2 and 3 need a reader. CI runs it, clang-format and the copies' check on every pull
request; the PostToolUse hook runs it after each edit.
