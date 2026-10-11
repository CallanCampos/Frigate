# Levers by symptom

Every way found so far to move MWCC's output (`dsi/1.1p1`, Thumb, `-O4,p`), indexed by what the diff shows, one line
each. `docs/matching.md` explains each one with its example, under the same headings. (matching.md: "...") marks
text to `grep -n` there. Entries without a key come from later work and still belong in `docs/matching.md` (see the
`record-lesson` skill). Some levers appear under two symptoms.

## Registers swapped

- Locals get registers in declaration order: reorder the declarations. (matching.md: "declaration order of locals")
  Declaring every local first and assigning them below moves both registers and slots (matching.md: "declared first")
- A `u16` local and `local ± 1` sharing a register can push a parameter out of r0: a wider local keeps them apart.
  (matching.md: "wider local")
- A ternary store computes the address once, an `if`/`else` store in each branch. (matching.md: "ternary store")
- `p + (a + 4)` and `p + a + 4` differ. (matching.md: "Parenthesized offsets")
- A sum folded into the load's offset (`ldrb r2, [r3, #9]` from `base + pos`) where ours adds `pos + 1` first:
  `*(data + pos + 1)` instead of `data[pos + 1]`. (matching.md: "`*(data + pos + 1)`")
- Of two variables that compete for one register, the one used more gets it. `docs/matching.md` spells out what counts as a
  use; on a tie the one assigned first wins, though in a large function the last declared won.
  (matching.md: "compete for the same register")
- A variable gets a register per group of assignments that reach the same uses: a store after two branches keeps
  one register, a copy of the store in each branch splits it. (matching.md: "group of assignments")
- A pointer local to a struct's element costs a callee-saved register; index the element at each use instead.
  (matching.md: "A pointer local to an element")
- `arr[count++] = x` and `arr[count] = x; count++;` allocate differently, as do `count = 1; arr[0] = x;` and the
  reverse. (matching.md: "arr[count++]")
- A sum used as an index goes to the register of one of its terms unless it has its own variable.
  (matching.md: "array index that is a sum")
- The operands of `*` are loaded in source order. A product assigned to its own variable gets a new register.
  (matching.md: "operands of `*`"), (matching.md: "A product assigned")
- A three-term `|` chain with its loads swapped: swap its first two terms. (matching.md: "three-term `|` chain")
- Where a flag is first set decides which register builds its zero. The register a shared zero gets follows statement
  order, and stores of 0 before a loop take the zero of the variable its initializer sets first, as in
  `for (i = 0, count = 0; ...)`. (matching.md: "Where a flag is first set")
- A register swap with otherwise identical code, in code that mirrors a sibling function: try that code as a
  `static inline` helper. (matching.md: "Moving code into a `static inline` helper")
- Chained stores of one constant (`a = b = TRUE`) share a register; separate ones may not.
  (matching.md: "Two stores of the same constant")
- Variables of an inner block are allocated apart from the function's. (matching.md: "declared in an inner block")
- A zeroed struct passed by value in a loop is copied from another zero variable: declaring it in a block around the
  loop changes which. (matching.md: "zeroed struct a loop passes by value")
- A narrow type in a wider local: `GFL_BGSysAllocChar` only matched with a `u8` tile size held in an `int`.
  (matching.md: "plain change")
- Diagnose with `tools/decomp/locals.py`, which shows each variable's register.
- A parameter spilled after a register copy, where the original spills it first: the callers narrow it, so it is a
  `u16` or `u8`. (matching.md: "callers narrow with shifts")
- An element's address and the array base in two registers: test the fields through an element pointer and index in
  the body. (matching.md: "pointer to the element")
- A loop's registers swapped around an array element that is stored, tested and passed on: a `T **slot = &arr[i]`
  assigned just before the store. (matching.md: "pointer to an array slot")
- A struct member's offset loaded early into `r6`/`r7` and added to the base at each use: a local pointer to the
  member, assigned where the original loads the offset. (matching.md: "pointer to a struct member")

## Stack slots or frame size

- An extra slot holding a copy of an address-taken local before a nested loop: read its field inside the inner loop,
  not into a local in the outer one. (matching.md: "hoisted only out of the loop it sits in")
- Stack locals are laid out in reverse declaration order; to read values in one order and slot them in another,
  declare without initializers and assign later. (matching.md: "reverse declaration order")
- A struct copied from `.rodata` once before a loop into the lowest slot, then into another slot inside it: a
  local initializer in the loop body. (matching.md: "local initializer inside a loop")
- Spilled variables get slots in the order they are first assigned, in small functions. In big switches,
  declarations count too. (matching.md: "Spilled variables get their stack slots")
- A variable reused by several switch cases splits per case, and a spilled piece takes the lowest slot.
  (matching.md: "reused by several switch cases")
- Values at the bottom of the frame in assignment order, with one among the declared slots: one variable reused by
  several blocks. (matching.md: "assigned anew in several `if` blocks")
- A spilled value in the lowest slot that should be among the declared ones: split a chained assignment `a = b = x`.
  (matching.md: "chained assignment")
- Block-scoped locals sit above function-scope ones. A block-scoped `{0, 0, 0}` initializer and a non-`const`
  parameter fixed `particle.c`.
- A NULL check written on a field gives different slots from the same check on a local loaded first.
  (matching.md: "NULL check written on a field")
- Reusing one `next` variable for two loops gives late spill slots. A counter multiplied later
  (`count * 0x10000`) is strength-reduced into a slot set in the loop's preheader.
- MWCC keeps its own copy of a field across the 64-bit multiply helpers; writing that copy as a local moves every
  slot. (matching.md: "64-bit multiply helpers")
- The types of spilled values change when they are reloaded (a `u16` after a call's stack argument is stored, a
  `u32` before). (matching.md: "types of locals")
- Struct copies to the stack: a struct passed by value goes in registers and on the stack, and a copy whose address
  is passed is a local copy, and a struct local keeps a stack slot. (matching.md: "Structs passed by value")
- Diagnose with `tools/decomp/locals.py`, which shows each variable's `sp+offset`.

## Instructions in another order (scheduling)

- A value computed later than written, at its only use: an enum local stays where it is assigned.
  (matching.md: "enum local's value")
- A load through a pointer moves above stores only when the pointee is `const`. A load scheduled early points to a
  `const` parameter. (matching.md: "unless the pointee is `const`")
- The same rule orders a call's stack argument stores against the register arguments. (matching.md: "stack argument stores")
- Two fields loaded before either is stored, through a pointer that is not `const`: a struct assignment.
  (matching.md: "A struct assignment loads every field")
- Register parameters spilled at entry in another order: try `u8` for flag parameters typed `BOOL`.
  (matching.md: "`u8` flag parameters")
- The operand order of a product decides which value is loaded first. (matching.md: "operand order of a product")
- A load through a `const` pointer is reused across stores but not hoisted out of a loop. (matching.md: "reused across stores")
- Initializations are scheduled where they are written: `int i = 0;` declared after a call against `for (i = 0; ...)`.
  (matching.md: "scheduled where they are written")
- Two loop variables zeroed in the wrong order: `for (i = 0, count = 0; ...)` in the original's order.
  (matching.md: "Two loop variables zeroed")
- A u16 stack parameter left in its slot and reloaded with `ldrh`, one load shared by two calls: those callees take
  `u32`; check their prototypes against their asm. (matching.md: "reloaded with `ldrh`")
- An argument loaded before a call among the arguments was passed to an inlined helper that makes the call.
- A field read before a `sys_memset` (or another call) that could change it was the argument of an inlined helper.
  (matching.md: "a field read before a call that could change it")
  (matching.md: "inlined helper that makes the call")
- One load of a struct's pointer field for two stores through it, where ours reloads: an inline helper taking the
  pointer. (matching.md: "Two stores through a pointer")
- A parameter passed on the stack is loaded at entry, unless it is an `int` or `s32`. (matching.md: "passed on the stack is loaded")
- NitroSDK's inline functions take enums, which changes when their arguments are loaded and shifted.
  (matching.md: "NitroSDK's inline functions")
- `FX_Mul`'s sign extensions move with statement order and with a `static inline` wrapper. (matching.md: "FX_Mul")
- A spilled counter's zero stored before a call, in `r1` rather than `r0`: try `n = 0;` later, after the call's
  result is used. (matching.md: "A counter's zero stored")
- `x[n++].f = ...` against a separate `n++` changes scheduling.
- Store order in initialization code is usually source order: try the stores in the asm's order first.
- A field of a local struct loaded before a call that doesn't fill it was read into a local there, as `targetX = target.x;`.
  (matching.md: "A field of a local struct")
- A nested call made after the outer call's other arguments, where the original makes it first: its result was a
  local. (matching.md: "nested in another call's arguments")
- Arguments loaded in order around a conditional one: that argument was a local set before the call.
  (matching.md: "A conditional expression among a call's arguments")
- A plain argument loaded before a call that is another argument: the plain one was in a local.
  (matching.md: "plain argument loaded before")
- An inline's argument computed and spilled at the inline's entry: the caller passed a local.
- An inline's table arguments loaded right to left before its stores, where ours loads each at its store: read the
  `static const` table through a function-scope pointer local. (matching.md: "function-scope pointer to the table")
- A bit field's word loaded again for the next field: the first read went through a `static inline` getter.
- The wrong one of two locals spilled: try the declaration order. Swapped registers feeding a product: swap its
  operands.
  (matching.md: "copies into its one use")
- A `const` table read before I/O register stores: it was written before them. (matching.md: "not moved across stores to I/O")
- A global's address loaded early into a saved register, before unrelated calls: take it into a pointer local
  there. (matching.md: "A global's address that the original loads early")

## An instruction too many or too few

- A saved register, `bl` and return where the C tail-calls (`ldr r3, =f; bx r3`): the call passes a fourth register
  argument. (matching.md: "four arguments in registers")

- A narrowing before an `and` into a `u8` field: `x &= mask` narrows the mask, `x = x & mask` doesn't.
  (matching.md: "compound assignment to a narrow field")
- A reload between two stores of one value: a chained `a = b = v;`; separate statements store the narrowed value
  twice. (matching.md: "chained assignment to fields")
- A narrowing (`lsl`/`lsr` or `asr` pair) comes from a `u8`/`u16`/`s16` local, parameter or return type. A caller narrows
  arguments for narrow parameters, so an argument passed without them is for a wider one. (matching.md: "narrows an argument")
- A parameter passed on to a `u8` parameter without shifts is a `u8` too. (matching.md: "narrows an argument")
- Extra `u16` narrowings come from `u32 x = (u16)...` passed through a `u16` inline parameter.
- A sum truncated to `s16` before a comparison was stored in an `s16` local. (matching.md: "truncates to `s16`")
- A narrowing again after a clamp is `MATH_CLAMP`, a conditional expression. (matching.md: "narrowed again after it is clamped")
- A local reloaded from its stack slot before each use can be a `u8` flag, not `volatile`.
- Masks written with `~` give `bic`; an `and` with `0xef` is `x &= (u8)~FLAG`. (matching.md: "Masks written with")
- MWCC doesn't propagate constants into enum-typed variables: a loop that checks its bound before the first pass, or
  a sum that adds a counter known to be 0, has an enum counter. (matching.md: "enum type")
- A local holding a constant keeps its own register or slot, while a literal is hoisted.
  (matching.md: "holds a constant, like")
- An address computed before calls is reused after them only when the expression is the same, types included. An
  inline accessor recomputes it. (matching.md: "computed before calls is reused")
- A constant base address with offsets where MWCC folds each store into its own literal: a pointer local.
  (matching.md: "Stores to fixed addresses")
- A hardware address built with shifts and spilled, where ours loads it from the pool: the SDK inline that returns
  it, such as `G2_GetOBJCharPtr()`. (matching.md: "A hardware address that the original builds with shifts")
- Stores whose base register is another element than the one written: index the array, or write through a pointer
  to the element, whichever the original does. (matching.md: "pointer to an array element")
- A value a loop uses and the code after it uses again is reused from the hoisted copy, unless it is a variable
  declared in the loop body. (matching.md: "reused from the copy hoisted")
- An address passed to a `const` pointer parameter is converted, and not shared. (matching.md: "`const` pointer parameter is converted")
- `const` table reads at a constant index are folded into immediates; reads in a loop, even an unrolled one, are not.
  An `ldm` into argument registers is a loop over a table. Reads through a pointer to the entry aren't folded either.
  (matching.md: "Reads of a `const` table")
- A value moved into an argument register before a call and used for nothing else is a parameter the prototype is
  missing. (matching.md: "prototype is missing")
- A constant built once for `r3` and the first stack slot is one `u64` argument. (matching.md: "is a `u64` argument")
- A caller that leaves an argument register untouched across a call is passing that argument.
  (matching.md: "keeps an argument register untouched")
- A NULL that the original tests (`movs r7, #0` then `beq`) and MWCC folds away is open; see the `event_save.c` and
  `script_sys.c` rows of `docs/nonmatching-functions.md`.
- Memory loaded again after stores to a local `u8` array, or stores through `add rN, sp, #off` for a local: the
  local is initialized in its declaration (matching.md: "initialized in its declaration is stored through a base
  register")
- An array initializer stores at its declaration: open an inner block where the original clears the array.
  (matching.md: "The initializer's stores happen")
- `s16` narrowing of a value also used unnarrowed: an inline with `s16` parameters. (matching.md: "inline with `s16` parameters")
- A call's result copied to a scratch register before a subtraction into an argument register: put the difference
  in a local's initializer. (matching.md: "minus a constant, passed straight")
- An object's address loaded from two literals in one function, where ours keeps one in a register: the object is a
  `static` of the file. (matching.md: "gets two literals")

## Branches and block layout

- Returns of `-1` and `0` folded into one computed result (`rsbs`, `mvns`) where the original keeps two returns: the
  function returns an enum. (matching.md: "returns an enum")
- `bne` over a `b` to the end at the top: the body is in an `if`, not after an early return. (matching.md: "An early `return`")
- A flag built before its tests (`movs rX, #0` first) from an inline's `return a && b;`: write `if (a && b) { return TRUE; } return FALSE;`. (matching.md: "An inline that returns a condition")
- An inline's 0/1 result materialized then tested (`movs r0, #1; b; movs r0, #0; cmp r0, #0`) where ours folds it into the branch: return `u32` with the if-form, or `return (BOOL)(a >= b);`. (matching.md: "An inline's 0/1 result")
- A final boolean returned from a register shared with a `NULL` argument: `return f() == TRUE ? FALSE : TRUE;`.
  (matching.md: "ends in `return f(...) == TRUE")
- Blocks are laid out in source order. A switch whose `default` code comes first had `default:` written first, and
  `if (!f()) return FALSE; n++;` puts the return before the code that goes on. (matching.md: "Blocks are laid out in source order")
- Identical statements in different branches are merged, so a jump into the middle of another block means the same
  code was written there. (matching.md: "Identical statements in different branches")
- A branch to the next instruction comes from cross-jumping a shared tail. (matching.md: "cross-jumping")
- `bne next; b target` where `||` gives one `beq`: the same body in both arms of an `if`/`else if`. (matching.md:
  "`bne next; b hide`")
- Early `return FALSE`s go to one shared tail only when the C has one trailing return (an `if`/`else if` chain, or a
  `result` variable). A return that branches to the wrong one of two equal `b end` trampolines can be `goto end`.
- A redundant outer `if` gives a doubled `beq`.
- `v = f(); if (v == x)` and `if (f() == x)` put `cmp`'s operands in opposite orders. (matching.md: "operands of `cmp`")
- A last test that branches the wrong way (`bne` to the false return for `beq` to the true one): negate the condition
  and swap the returns. (matching.md: "last test of a condition")
- `a == 4 || a == 5` becomes a range check; separate comparisons to the same code are separate branches. A
  `BOOL x = FALSE; if (...) x = TRUE;` flag gives the `sub; cmp 1; bhi` range test. (matching.md: "range check")
- A clamp that ends in one store is a conditional expression; `if`/`else if` stores each limit.
  (matching.md: "A clamp that ends in one store")
- One store after an `if`/`else` of two constants, with a `b` over the else, is still an `if`/`else`; the conditional
  expression has no `b`. (matching.md: "assigns one field a constant in each branch")
- A `b` to a `b` where the original jumps straight to shared code: write the call after the `if`/`else` in
  each branch. (matching.md: "A `b` to a `b`")
- `f(x ? a : b)` against two calls in `if`/`else`, which are merged into one call with a `beq; b` layout.
  (matching.md: "picked by branches")
- A `return` inside `for (;;)` leaves a dead `bx lr`, which the original counts as padding.
- Literal pool placement: a pool dumped mid-function is placed after an unconditional branch. See the `event_make.c`
  row of the nonmatching doc for a case still open.
- A wrapping decrement that reads the variable again in one branch: `if (x == 0) { x = 3; } else { x--; }`, not a
  conditional expression (matching.md: "reads it again in the `else`")
- A bit test (`lsl; tst #mask`) for a value among a few constants: `return x == a || x == b || x == c;`.
  (matching.md: "bit test")
- A test after the others that it guards against: a nested `if` without an `else`. (matching.md: "Nested tests")

## Loops

- `while (cond)` is rotated, with a copy of its test before the loop. A loop that tests once, at its top, is
  `while (TRUE)` with a `break` or `return`. (matching.md: "is rotated")
- A loop that runs once is unrolled when its counter and bound have the same signedness; `int i < NELEMS(x)` compares
  unsigned and keeps the loop. (matching.md: "loop that runs once")
- An address or value an inner loop computes from the outer counter alone is hoisted into the inner preheader.
  (matching.md: "inner loop computes"), (matching.md: "preheader")
- Enum counters keep their guard (see above).
- A field loaded again at the top of a loop's body, after the test loaded it: walk a local cursor, not the pointer
  parameter. (matching.md: "local cursor")
- `beq` before and `bne` after the loop is a `!=` bound. (matching.md: "A loop counted with `!=`")
- A loop bound computed once and tested with `ble`: `i <= N - 1`; `i < N` reloads it and tests `blt`.
  (matching.md: "i <= N - 1")
- A test after a body that is entered from the top and from an earlier branch, in a function that does one box a frame:
  `while (box < n) { ...; break; }`, with a comment. (matching.md: "stops after its first pass")
- A load hoisted one loop level only: it was in the middle loop's body. (matching.md: "middle loop's body")

## Switches

- Cases are laid out in source order, not by value. (matching.md: "Switch cases are laid out in source order")
- The comparison tree and jump tables depend on every case value, including empty cases. (matching.md: "comparison tree")
- A case that ends in the same code as another is merged into it. (matching.md: "ends in the same code as another")
- Cases that load some of a call's arguments, then branch to the shared rest and the call: every argument was a
  variable set in each case. (matching.md: "each set every argument")
- An `if`/`else if` chain whose tests come in a switch's order is a `switch` with a case falling into `default`.
- All the tests first (`cmp; beq` each) is a `switch`; a test before each body (`cmp; bne`) is an `if` chain.
  (matching.md: "tests them all first")
  `case 0: default:` written first sets the case order.
- `cmp; beq end; cmp; bne next` with each body after its test: an `if`/`else if` chain with an empty first body,
  not a switch (matching.md: "A short chain of tests whose first value does nothing")
- A value tested before the jump table, with its code after the cases, is an `if (x != v)` around the switch.
  (matching.md: "value outside its jump table")

## Floats and runtime helpers

- Float arithmetic calls `_fadd`, `_ffix` and others (swan's `__aeabi_*`). Rename to MWCC's name when a complete file
  fails to link. (matching.md: "MWCC's runtime helpers")
- An arithmetic operation on a literal passes the literal first; a constant in a local keeps its source position, and
  a compound assignment (`x *= 1.5`) passes its target first.
  (matching.md: "passes the literal first")
- A call inside `FX32_CONST(...)` is made three times; the game passes a local. (matching.md: "FX32_CONST")
- A literal in a compound assignment (`y += 0.01f`) is passed second. (matching.md: "compound assignment keeps")
- Float arithmetic on a local holding a constant isn't folded. (matching.md: "doesn't fold float arithmetic")
- Division and modulo call `_s32_div_f` or `_u32_div_f` by signedness. A `u8`/`u16` promotes to signed `int`.
  (matching.md: "_s32_div_f")
- `__aeabi_uldivmod` on sign-extended operands is an `int` cast to `u64`; MWCC calls it `_ll_udiv`.
  (matching.md: "A 64-bit division")
- A float one ULP off a round decimal is written with the shortest digits that round to it. (matching.md: "one ULP
  off")

## Data and sections

- String literals in another order: MWCC lays them out in the order they first appear in the source; a `""` the
  game has before the file's name needs an earlier use. (matching.md: "String literals")
- Static data is sorted by size by a heapsort. Objects of 64 bytes or more, local initializers and unreferenced
  globals get their own sections. Predict with `tools/decomp/rodata_order.py`. (matching.md: "Static data is sorted by size")
- The full model, checked by fuzzing MWCC: there is one list per file in declaration order, except tentative `.bss`
  statics, which join at the end in reverse order. Each kind (rodata, data, bss) gets its own shared section.
  Unreferenced statics are dropped. `rodata_order.py` doesn't model the per-kind sections or the `.bss` rule yet, but
  given the `.data` tables with the `.rodata` objects it predicts the `.rodata` order. (matching.md: "The list that is
  heapsorted")
- `.bss` statics are ordered by size, then in an order that isn't the declaration order; try permutations.
- A table that only one function reads can be a `static const` inside it, which moves it in the heapsort's list.
  (matching.md: "declared inside the one function")
- A `.rodata` template copied to a local, then patched with computed fields, in a section of its own: a local
  initializer with the computed values in its braces, in a block after any calls before it. (matching.md: "a few
  fields overwritten")
- When the prediction is wrong, move one declaration at a time and compare the built sections with the ROM.
  (matching.md: "prediction disagrees")
- `static const` goes in `.rodata`, so a table in `.data` isn't `const`. (matching.md: "`static const` data goes in")
- Small unreferenced objects at the start of a file's shared `.rodata` (ahead of its smallest referenced table) are
  function-local `static const`s, which are emitted even when their reads are folded or absent. (matching.md:
  "function-local `static const` is emitted")
- A `static const` whose address is never taken is folded and not emitted. If the original has it, it isn't static.
  An unreferenced word after a file's larger tables is the next file's first object.
  (matching.md: "whose address is never taken")
- Library code was built against an older NitroSDK, whose headers differ: SPL's `GX_ST` doesn't narrow texture
  coordinates to `fx16` where the game's does. `configure.py` defines `OLD_NITRO_SDK` for SPL, and `nitro/gx.h`
  picks the macro by it.
- `GFL_ASSERT` keeps its expression as a string, which preserves the original variable names. (matching.md: "GFL_ASSERT")
- A file's `.data` ends at its last object.
- One table in the `.rodata` of several files: a `static const` in a header, with a `static inline` reading it.
  (matching.md: "same small table")
- Two literals for the same object in one function: it is a `static` of the function's file; an `extern` shares
  one. (matching.md: "gets two literals")

## Function order and presence

- SPL's `1.2/base` compiler emits a file's functions in reverse source order.
- The MWCC linker here doesn't dead-strip unreferenced global functions.
- A static function whose symbol isn't renamed to its C name shows as "unknown" in the probe.

## Diagnostic switches (not for committed code)

These narrow down which optimization causes a difference, after which you look for the C spelling. They come from
sm64ds-decomp's catalogue for another mwccarm build, so they are untested here:
- `#pragma opt_common_subs off`
- `#pragma opt_strength_reduction off`
- `#pragma optimize_for_size on`
- `#pragma opt_propagation off`

Try one at a time with `try_variants.py`, which allows definitions before the function. If a pragma is all that
matches, write that down in the nonmatching row; it's evidence, not a fix.
