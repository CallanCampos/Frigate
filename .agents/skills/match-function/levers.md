# Levers by symptom

Every way found so far to move MWCC's output (`dsi/1.1p1`, Thumb, `-O4,p`), indexed by what the diff shows, one line
each. `docs/matching.md` explains each one with its example, under the same headings. (matching.md: "...") marks
text to `grep -n` there. Entries without a key come from later work and still belong in `docs/matching.md` (see the
`record-lesson` skill). Some levers appear under two symptoms.

## Registers swapped

- Zeros loaded from a stack slot at each use and a constant hoisted into a saved register, where ours uses `movs`:
  move a later loop into a `static inline` that counts with its own parameter, passed that constant.
  (matching.md: "A constant held in a stack slot")
- A multiply's operand copy in `r4` with the index in `r1`, where the original has the reverse: give the index an
  enum-typed local. (matching.md: "An enum-typed index local stays a variable of its own")
- A multiply's operand copied right after its `subs` where the original loads the constant first: give the index
  its own variable and index the elements in place. (matching.md: "The copy of a multiply's operand")
- A table's base and index swapped between the two registers (`ldr r4, =table; muls; adds r4, r4, r2` in the
  original): index the table in a `static inline` taking the index as an enum type. (matching.md: "A table indexed
  by a field")
- A narrow stack parameter read from its slot at each use where the original loads it into a saved register at
  entry: declare the parameter `const`. (matching.md: "A narrow stack parameter used twice")
- An index kept in two registers (copied after a division and again before a multiply) where ours keeps one: a
  `const` struct parameter with its pointer field read at each use. (matching.md: "A pointer field read through a
  `const` parameter")
- A stack-passed parameter kept on the stack where the original loads it into a saved register: copy it into a
  local of the other signedness (`int pos = index;`). (matching.md: "A 32-bit parameter passed on the stack")
- A 64-bit sum of products in other registers, or an `ldm` of two fields where the original has two `ldr`: build
  the sum with `+=`, one product a statement. (matching.md: "built up with `+=`")
- A checksum comparison with swapped `cmp` operands: store the `u8` call result in a local before comparing it
  with the parameter, as `Aoss_Decrypt` does. (matching.md: "`Aoss_Decrypt` needs the `u8`")
- Locals get registers in declaration order: reorder the declarations. (matching.md: "declaration order of locals")
  Declaring every local first and assigning them below moves both registers and slots (matching.md: "declared first")
- A pointer needs an earlier slot but a later store: declare it without an initializer and assign it after other
  pointers, as in `Aoss_ParseWep` and `Aoss_ParsePsk`. (matching.md: "A pointer that must")
- A `u16` local and `local ± 1` sharing a register can push a parameter out of r0: a wider local keeps them apart.
  (matching.md: "wider local")
- A ternary store computes the address once, an `if`/`else` store in each branch. (matching.md: "ternary store")
- `p + (a + 4)` and `p + a + 4` differ. (matching.md: "Parenthesized offsets")
- A sum folded into the load's offset (`ldrb r2, [r3, #9]` from `base + pos`) where ours adds `pos + 1` first:
  `*(data + pos + 1)` instead of `data[pos + 1]`; this also folds `- 1` in `BrProcSys_Pop`.
  (matching.md: "`*(data + pos + 1)`")
- Of two variables that compete for one register, the one used more gets it. `docs/matching.md` spells out what counts as a
  use; on a tie the one assigned first wins, though in a large function the last declared won.
  (matching.md: "compete for the same register")
- A variable gets a register per group of assignments that reach the same uses: a store after two branches keeps
  one register, a copy of the store in each branch splits it. (matching.md: "group of assignments")
- An update written in both branches of an `if`/`else` (`nbits--;` twice) is merged into one instruction but
  allocated like two assignments, which can swap registers. (matching.md: "update written in both branches")
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
- Registers swapped around a flag set `TRUE` and overridden in an `if`, with the instructions already right:
  write it as a ternary, `x = k != 0 ? TRUE : FALSE;`. (matching.md: "the ternary still allocates the")
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
- A stack pointer parameter kept in a callee-saved register where the original reloads it at each use: its
  uses weigh too much; move its dereferences into a `static inline` taking a `const T *`.
  (matching.md: "goes to a callee-saved register at entry")
- Registers off around independent stores: sweep the statement orders, ending with the store that the next call
  reads back. (matching.md: "`WipeRectWork_Init` matches only with")
- Registers off around independent loads: reorder the loads in the source, and retry an SDK inline such as
  `G3_PolygonAttr` against its packing macro after each reorder.
  (matching.md: "Independent loads also take registers by their source order")
- A parameter copied to another register at entry, with a value derived from it in a third: the parameter was
  reassigned (`pos /= 8;`), not copied to a local. (matching.md: "was reassigned too")
- A later loop's counter swapped with another local, the counter reused from an earlier loop: give that loop a
  counter of its own. (matching.md: "matches once that loop counts with a `k` of its own")
- A pointer that both branches of an `if` share loses its callee-saved register to the loop variables: the value
  tested before the branch and the one a loop works out in it are one variable.
  (matching.md: "that split frees a register for a pointer")
- A value and its shift in one register where the original shifts into a new one: the shift is an expression, an
  inline returning `x << n`, not `x <<= n`. (matching.md: "A shift written in place")
- Two offsets that a loop steps alike, their registers swapped in every order: change both between `u32` and `int`.
  (matching.md: "can swap registers by their signedness alone")
- An index loaded and shifted in place where the original shifts it into another register: a pointer local set to
  the member array and stepped, `p = s->arr; p += i;`. (matching.md: "keeps a loaded index in its own register")
- A whole function's registers off, with an element's base added before its index and field offset: the inline
  reads through `T *p = &arr[i];`. (matching.md: "Reads through such a pointer in an inline")
- A table field passed to a call through its own argument register (`ldr r3, =...; ldrsh r3, [r3, r4]`), where
  the original uses the lowest free one (`ldr r1, =...; ldrsh r3, [r1, r4]`): read it into one local that every
  block making the same call assigns. (matching.md: "picks the register of an argument's address")
- A field's offset added to the base before a register index (`adds r0, r0, #4; ldr r1, [r0, r1]`): two accesses
  to `p[i].f` through a pointer variable that MWCC's first CSE pass did not merge, as in `f[i].b += v`;
  `(&p->f)[i * 2]` folds back to index first. (matching.md: "its first common-subexpression pass did")
- Every register moved by a field that the caller keeps in a saved register after an inline read it, where the
  original loads it again: the inline takes a `const T *`, even from a caller's plain pointer.
  (matching.md: "a `const` pointer from a caller whose")

- A struct field loaded again for each of several tests in a row, where ours reuses one load: the field is shared
  with other threads and `volatile`. (matching.md: "A field that other threads change")

## Stack slots or frame size

- An archive handle or other local in a low slot where the original has it above the optimizer's temporaries:
  move the block that uses it into a `static inline` that declares it. (matching.md: "A local of an inlined
  helper")
- A NitroSystem (2.0/sp2p2) function's spill slots in another order: locals are slotted function scope first, then
  the `if` block's, then the `else` block's, then inlines' in reverse; move them to function scope.
  (matching.md: "In NitroSystem's 2.0/sp2p2 code a pointer sum")
- Spill slots grouped by kind across branches where ours are in declaration order: a local set from an inline's
  return is slotted as a temporary; compute it in place to make it named, or the reverse. (matching.md: "A local
  assigned straight from a `static inline`'s return")
- A narrow local's stack slot swapped with another's where the original slots it like a temporary: make every
  callee parameter it is passed to the same type. (matching.md: "A narrow local passed to several calls")
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
- A struct copied to the stack for each by-value inline call, and again before an inner loop: the inlines
  took the array element, `Get(learnset[i])`, not a local copy. (matching.md: "is copied for each call")
- A spilled local in the lowest slot, among the temporaries, where ours keeps its declared slot: it was assigned with
  a ternary, not a default and an `if`. (matching.md: "A local assigned with a ternary is spilled among")
- Spilled values interleaved in assignment order where an inline's by-value parameter groups its slots apart: pass
  the value's address as a `const` pointer and dereference it in the inline.
  (matching.md: "An inline's parameter is a named local")
- A common value's spilled temporary at the wrong end of the frame: move its first use, not a local's declaration,
  across the first call. (matching.md: "spilled temporary takes its slot by where its first use")
- A long-lived local spilled by a loop that stores two fields of `list[i]`: store through `T *entry = &list[i];`.
  (matching.md: "keeps the offset live as a third temporary")
- A struct local's members in the lowest slots, every declared slot moved up: MWCC split it, since its address
  did not survive into the optimizer; an initializer, a real call with `&s`, a copy through memory or a pointer
  live across blocks keeps it whole. (matching.md: "split into one variable per member")
- A register parameter stored at entry and loaded back from its slot, where ours copies it out of the register:
  the parameter is narrower than `int` (for a static helper, the type its public caller passes); loop with a
  separate `int` counter. (matching.md: "A register parameter stored at entry and read back from its slot")
- Diagnose with `tools/decomp/locals.py`, which shows each variable's `sp+offset`.

## Instructions in another order (scheduling)

- An inlined SDK call whose packed value is built in another order: check the inline's parameter types against the
  SDK (an `int` where ours has an enum). (matching.md: "An inline's enum-typed parameter")
- A stack argument stored after the next argument's setup starts, where the original stores it first, around a
  spilled local: sweep the local's width together with its callee's return type and `value` local.
  (matching.md: "A spilled local's width for the rule below")
- A value computed later than written, at its only use: an enum local stays where it is assigned.
  (matching.md: "enum local's value")
- A load through a pointer moves above stores only when the pointee is `const`. A load scheduled early points to a
  `const` parameter. (matching.md: "unless the pointee is `const`")
- A `void *` parameter copied to a typed local takes the `const` itself, callback typedef included; through a
  wrapper whose body is a `static inline`, only the wrapper's parameter counts.
  (matching.md: "copied to a typed local needs the `const`"), (matching.md: "only the wrapper's own parameter counts")
- MWCC passes a `const void *` to a `T *` silently: find the dropped `const` with
  `clang -fsyntax-only @compile_flags.txt` and make the callee's parameter `const`.
  (matching.md: "without a word, even with `-w all`")
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
- A spilled index parameter's multiply scheduled apart from the original's, which no statement order fixes:
  widen the parameter from `u16` to `int` in the prototype. (matching.md: "width of a spilled index parameter")
- A local's arithmetic done at its first use, past a call, where the original does it where written: give the
  local the type of the callee parameters it is passed to.
  (matching.md: "A local of another type than the callee parameters")
- A spilled reload above another spill's store, where the original stores first: the spilled locals are `u8`
  or `u16`; sweep the widths of several together. (matching.md: "can mean narrow locals")
- A spilled parameter reloaded on the other side of a call's stack argument stores: sweep the caller's and the
  callee's parameter widths together. (matching.md: "Spilled parameters follow the same rule")
- Loads moved among a call's stack argument stores with the same instructions and slots: give a local read
  from a field the field's type. (matching.md: "can need the field's type even with the same")
- NitroSDK's inline functions take enums, which changes when their arguments are loaded and shifted.
  (matching.md: "NitroSDK's inline functions")
- `FX_Mul`'s sign extensions move with statement order and with a `static inline` wrapper. (matching.md: "FX_Mul")
- A spilled counter's zero stored before a call, in `r1` rather than `r0`: try `n = 0;` later, after the call's
  result is used. (matching.md: "A counter's zero stored")
- `x[n++].f = ...` against a separate `n++` changes scheduling.
- A constant built before stores written ahead of it, all to one global, where ours builds it at its first store:
  write the stores through a pointer to a member (`&G.header`); a `&G` local of G's own type keeps them chained.
  (matching.md: "told apart by offset")
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
- A static table's address loaded into a saved register at entry after a parameter's spill, where the original
  loads it first: drop the pointer local aliasing the table and index the table at each use.
  (matching.md: "A static table's address that MWCC gathers")

## An instruction too many or too few

- A constant narrowed at a call after `movs` (`movs r3, #0x11; lsls; asrs`): an enum-typed local set at the top of
  the function, before the calls. (matching.md: "A constant narrowed at a call")
- A table field's offset folded into a second literal (`ldr r1, =table+4`) where the original adds the element
  offset to the base after a plain read: read that field through an inline taking an `int` index.
  (matching.md: "Two fields of one element of a static table")
- A saved register, `bl` and return where the C tail-calls (`ldr r3, =f; bx r3`): the call passes a fourth register
  argument. (matching.md: "four arguments in registers")

- A narrowing before an `and` into a `u8` field: `x &= mask` narrows the mask, `x = x & mask` doesn't.
  (matching.md: "compound assignment to a narrow field")
- A signed `/ 2` narrowed in separate steps before a loop (`asrs #1; lsls #0x10; asrs #0x10`, ours `lsls #0xf`):
  compute the value inside the loop body from a narrow local. (matching.md: "A signed halving kept apart")
- A reload between two stores of one value: a chained `a = b = v;`; separate statements store the narrowed value
  twice. (matching.md: "chained assignment to fields")
- A member's address CSE'd into a callee-saved register from a store to a later call's argument, where ours
  recomputes it: a pointer local declared at the top, assigned just before the call — not at its declaration.
  (matching.md: "assigned just before the call")
- Two constants on one field folded into one (`subs #0xff` for `- 0x100 + 1`) where the original keeps both: write
  them as two statements on the field. (matching.md: "Two constants applied to a field")
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
- A constant kept in a callee-saved register for calls, then reduced at run time before a field store
  (`subs r6, #0x20; str r6, [r5, #8]`), where ours folds it: assign the field, then update it in place,
  `p->size = size; p->size -= 0x20;`. (matching.md: "reduced at run time before a field store")
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
- An index product computed once, spilled and shared by every use with the index dropped and a constant folded
  into the base (`(base + 8)[p * 4]`), where ours keeps `p` and computes `(p + 2) * 4`: declare the index `long`,
  the SDK's `s32`. (matching.md: "`long` where the SDK types it `s32`")
- An address passed to a `const` pointer parameter is converted, and not shared. (matching.md: "`const` pointer parameter is converted")
- An element's address formed twice in two shapes, or a field loaded again after an inline read it: the callee or
  the inline takes a `const` pointer, even in another file. (matching.md: "The callee can be in another file"),
  (matching.md: "A field loaded in an inline that takes a `const` pointer")
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
- A bit field sign-extended twice, once plain and once after `lsl`/`asr #16`: an inline with an `s16` parameter
  called twice on it. (matching.md: "Called twice on one bit field")
- A call's result copied to a scratch register before a subtraction into an argument register: put the difference
  in a local's initializer. (matching.md: "minus a constant, passed straight")
- A -1 built from a shift's 1 (`sub rY, rX, #2`) where the original builds it alone (`mov; mvn`): a mask local set to
  `0xffffffff`, then `mask ^= 1 << bit;`. (matching.md: "the mask was a local set to all ones")
- A struct local's field loaded through a fresh `add rN, sp, #off` where the original loads through a saved
  register: a raw buffer read through a typed pointer. (matching.md: "a raw buffer read through a typed pointer")
- A range test's subtraction repeated after a call, where the original keeps it in a saved register: the variable
  is an `int`, not a `u32`. (matching.md: "subtracts again at each test")
- An object's address loaded from two literals in one function, where ours keeps one in a register: the object is a
  `static` of the file. (matching.md: "gets two literals")

- A `u16` argument narrowed before a call where the original passes one of two constants unnarrowed: a `u16` local
  set in an `if`/`else`, not a conditional expression. (matching.md: "The same goes for an argument")
- I/O registers off a base register built by `lsl` of a register holding 1, where ours loads each address from the
  literal pool: only `u32 base = 1; base <<= 26;` keeps it, judged unnatural so far (matching.md: "base made by
  shifting a register")
- A `bl memcpy` where the original keeps a hand `ldrb`/`strb` loop: declare `const u8 *`/`u8 *` locals and loop over
  those, not over the function's own raw `const void *`/`const char *` parameter directly.
  (matching.md: "only keeps that shape when the loop runs over its own")

## Branches and block layout

- A 0/1 flag built with branches (`bne; movs r1, #1; b; movs r1, #0`) where ours is set-then-clear: the flag's
  register is an operand of the `cmp`; declare and compute that flag first. (matching.md: "A 0/1 comparison flag")
- Returns of `-1` and `0` folded into one computed result (`rsbs`, `mvns`) where the original keeps two returns: the
  function returns an enum. (matching.md: "returns an enum")
- `bne` over a `b` to the end at the top: the body is in an `if`, not after an early return. (matching.md: "An early `return`")
- A flag built before its tests (`movs rX, #0` first) from an inline's `return a && b;`: write `if (a && b) { return TRUE; } return FALSE;`. (matching.md: "An inline that returns a condition")
- An inline's 0/1 result materialized then tested (`movs r0, #1; b; movs r0, #0; cmp r0, #0`) where ours folds it into the branch: return `u32` with the if-form, or `return (BOOL)(a >= b);`. (matching.md: "An inline's 0/1 result")
- A final boolean returned from a register shared with a `NULL` argument: `return f() == TRUE ? FALSE : TRUE;`.
  (matching.md: "ends in `return f(...) == TRUE")
- A 0/1 flag laid out first value first (`bne; movs #0; b; movs #1`) where every plain spelling loads 1 first: the
  arms are enum constants, with the condition written so the original's first arm comes first.
  (matching.md: "held in an enum type")
- A `? 0 : 1` with inverted branch polarity whose 1 the original takes from a long-lived constant register:
  default-then-override through a local, `v = 0; if (!cond) { v = 1; }`. (matching.md: "default-then-override")
- Blocks are laid out in source order. A switch whose `default` code comes first had `default:` written first, and
  `if (!f()) return FALSE; n++;` puts the return before the code that goes on. (matching.md: "Blocks are laid out in source order")
- Identical statements in different branches are merged, so a jump into the middle of another block means the same
  code was written there. (matching.md: "Identical statements in different branches")
- A branch to the next instruction comes from cross-jumping a shared tail. (matching.md: "cross-jumping")
- A load at the top of both arms of an `if`/`else`, where ours hoists it between the test and the branch: one
  arm began with another statement, as `destPixel--;`.
  (matching.md: "A load that both arms of an `if`/`else` begin with")
- `bne next; b target` where `||` gives one `beq`: the same body in both arms of an `if`/`else if`. (matching.md:
  "`bne next; b hide`")
- Early `return FALSE`s go to one shared tail only when the C has one trailing return (an `if`/`else if` chain, or a
  `result` variable, or an inlined function: see the next line). A return that branches to the wrong one of two equal
  `b end` trampolines can be `goto end`.
- Failures that set a saved register to 0 and jump to one `mov r0, rN` exit, where a `result` variable gets a stack
  slot: the body is a `static inline` called by a one-line wrapper. (matching.md: "Every `return` gets its own
  epilogue")
- A redundant outer `if` gives a doubled `beq`.
- `v = f(); if (v == x)` and `if (f() == x)` put `cmp`'s operands in opposite orders. (matching.md: "operands of `cmp`")
- A last test that branches the wrong way (`bne` to the false return for `beq` to the true one): negate the condition
  and swap the returns. (matching.md: "last test of a condition")
- `a == 4 || a == 5` becomes a range check; separate comparisons to the same code are separate branches. A
  `BOOL x = FALSE; if (...) x = TRUE;` flag gives the `sub; cmp 1; bhi` range test. (matching.md: "range check")
- A range check that adds a negated 16-bit constant, from the literal pool when it is large: an `||` chain of `==`
  tests of consecutive constants, not `x < A || x > B`.
  (matching.md: "A range check that adds the 16-bit negated constant")
- A clamp that ends in one store is a conditional expression; `if`/`else if` stores each limit.
  (matching.md: "A clamp that ends in one store")
- One store after an `if`/`else` of two constants, with a `b` over the else, is still an `if`/`else`; the conditional
  expression has no `b`. (matching.md: "assigns one field a constant in each branch")
- A `b` to a `b` where the original jumps straight to shared code: write the call after the `if`/`else` in
  each branch. (matching.md: "A `b` to a `b`")
- `f(x ? a : b)` against two calls in `if`/`else`, which are merged into one call with a `beq; b` layout.
  (matching.md: "picked by branches")
- A `return` inside `for (;;)` leaves a dead `bx lr`, which the original counts as padding.
- Literal pool placement: a pool dumped mid-function is placed after an unconditional branch. A constant arm that
  jumps into a select's `movs #0` is `if (p == NULL) { v = FALSE; } else { v = f() ? TRUE : FALSE; }`.
  (matching.md: "a constant arm written first")
- A wrapping decrement that reads the variable again in one branch: `if (x == 0) { x = 3; } else { x--; }`, not a
  conditional expression (matching.md: "reads it again in the `else`")
- A bit test (`lsl; tst #mask`) for a value among a few constants: `return x == a || x == b || x == c;`.
  (matching.md: "bit test")
- A test after the others that it guards against: a nested `if` without an `else`. (matching.md: "Nested tests")
- A branch that jumps past an identical second test, where ours goes to it: nest the second test inside the first
  `if`. (matching.md: "never threads a branch into an identical test")
- A missing container and a missing entry sharing one `movs rN, #0`, where ours copies the NULL from another zero
  register and branches over it: a nested-`if` inline ending in one `return NULL;`.
  (matching.md: "ends in a single `return NULL;`")

## Loops

- A running offset's `movs rN, #0` placed before hoisted loop invariants where the original zeroes it after: write
  it as a counter times the step. (matching.md: "A loop offset that starts at 0")
- A loop counter stored and reloaded where the original keeps it in a register for the test (2.0/sp2p2): put it last
  in the `for` increment's comma list. (matching.md: "a `for` loop's comma-separated increment")
- `while (cond)` is rotated, with a copy of its test before the loop. A loop that tests once, at its top, is
  `while (TRUE)` with a `break` or `return`. (matching.md: "is rotated")
- At `-O2,p` (`lib/irc`) loops are not rotated; a guard before the first pass and the test at the end is a guarded
  `do`/`while`, `if (i < size) do ... while (++i < size)`. (matching.md: "At `-O2,p` no loop is rotated")
- A loop that runs once is unrolled when its counter and bound have the same signedness; `int i < NELEMS(x)` compares
  unsigned and keeps the loop. (matching.md: "loop that runs once")
- `asr`/`ror` division of a parameter the callers pass as `u8` without conversion: a `u8` parameter copied to an
  `s32` local. (matching.md: "copied to an `s32` local")
- An address or value an inner loop computes from the outer counter alone is hoisted into the inner preheader.
  (matching.md: "inner loop computes"), (matching.md: "preheader")
- Enum counters keep their guard (see above).
- A field loaded again at the top of a loop's body, after the test loaded it: walk a local cursor, not the pointer
  parameter. (matching.md: "local cursor")
- `beq` before and `bne` after the loop is a `!=` bound. (matching.md: "A loop counted with `!=`")
- A loop bound computed once and tested with `ble`: `i <= N - 1`; `i < N` reloads it and tests `blt`.
  (matching.md: "i <= N - 1")
- A loop guard testing the bound's own register, with no copy, and `i` in a dead pointer's register: the
  bound written in the `for` condition, not in a local. (matching.md: "A bound written in the `for` condition")
- A test after a body that is entered from the top and from an earlier branch, in a function that does one box a frame:
  `while (box < n) { ...; break; }`, with a comment. (matching.md: "stops after its first pass")
- A load hoisted one loop level only: it was in the middle loop's body. (matching.md: "middle loop's body")

## Switches

- Cases are laid out in source order, not by value. (matching.md: "Switch cases are laid out in source order")
- `case -1:` tested on its own before a jump table the original folds it into (`adds r0, #1; cmp`): the switch is on
  a `u32`; make the value (or the function returning it) signed. (matching.md: "takes `-1` in")
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
- A 64-bit division calls `_ll_sdiv` for signed operands and `_ll_udiv` for unsigned; the probe masks the
  call, so link the file to check which. (matching.md: "and `_ll_sdiv`")
- A float one ULP off a round decimal is written with the shortest digits that round to it. (matching.md: "one ULP
  off")

## Data and sections

- String literals in another order: MWCC lays them out in the order they first appear in the source; a `""` the
  game has before the file's name needs an earlier use. (matching.md: "String literals")
- Static data is sorted by size by a heapsort. Objects of 64 bytes or more, local initializers and unreferenced
  globals get their own sections. Predict with `tools/decomp/rodata_order.py`; the object list must be complete, so
  `--from-object FILE.o` lists every object a compile made and `--check FILE.o` fails when the prediction and the
  compiler disagree. A "no order works" verdict means nothing until `--check` passes. (matching.md: "Static data is sorted by size")
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

- A Thumb to ARM `bx pc` trampoline: define a Thumb `asm void` with `bx pc; nop` and a separate ARM `asm void` with
  `ldr ip, [pc, #0]; bx ip; dcd ADDRESS`, toggling `#pragma thumb off/on` around the ARM part.
  (matching.md: "A Thumb `bx pc` trampoline")

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

Tested here: `#pragma optimization_level 1` reproduced the whole shape of `btlv_input.c`'s `BtlvInput_DrawMoveTiles`
(a loop entered with a `b` to its bottom test, the hoisted-constant spill slots, the `i`/`work` registers) where -O4
restructures it. A function whose loop enters with a `b` to the test at -O4 may be one the original's global
optimizer gave up on; `WorldTrade_NationSortListMake`, `NNS_SndArcStrmInit` and `MBGameInfo_SendMembers` show the
same symptom in their rows. Try a 0/1 loop test first: `Country_IsValidPlace`, which showed it too and came within a
byte under `optimization_level 1` or `2`, matched at -O4 with one (matching.md: "has a test whose value is 0/1").

An empty `if (a && b && c) {}` (three operands or more; two are removed early) splits the block where it stands
and emits nothing. When a register or CSE difference looks like a missing block boundary, slide one through the
statements: if one place matches, the original has a boundary there, such as a compiled-out debug print's `if`
(`SortBySpeed`'s swap; matching.md: "three or more `&&` operands"). Document it, don't commit it.

Try one at a time with `try_variants.py`, which allows definitions before the function. If a pragma is all that
matches, write that down in the nonmatching row; it's evidence, not a fix.
