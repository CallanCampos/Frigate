# Levers by symptom

Every way found so far to move MWCC's output (`dsi/1.1p1`, Thumb, `-O4,p`), indexed by what the diff shows, one line
each. `docs/matching.md` explains each one with its example, under the same headings. (matching.md: "...") marks
text to `grep -n` there. Entries without a key come from later work and still belong in `docs/matching.md` (see the
`record-lesson` skill). Some levers appear under two symptoms.

## Registers swapped

- Rebuilt hash-context offsets across a digest phase: borrow a typed context pointer for all its operations.
  (matching.md: "one typed context pointer for a digest phase")
- Adjacent scratch partitions in different saved registers: derive each typed region from its predecessor.
  (matching.md: "derives each adjacent scratch region")
- Missing digit reload after a byte store: check the inferred pointer qualification against mutable callers.
  (matching.md: "observed MWCC alias behavior")

- A numeric query field copied through an extra register: share the meaningful query-result local with the
  count case (`DWCi_QR2ServerKeyCallback`). (matching.md: "shares an `int value`")
- An alternating buffer's index reloaded after a byte copy: snapshot the selected index in its unsigned type and
  use it for both the copy and return (`value_for_key`). (matching.md: "selected alternating buffer")
- A by-value IPv4 formatter argument loaded differently from a struct field: name the `IN_ADDR` value before the
  call (`ProcessPingPacket`). (matching.md: "before its second diagnostic")

- A 64-bit product's masked high word in the wrong register: calculate the product in a local of its own, then
  mask it (`gsiLargeIntInverseMod`). (matching.md: "64-bit product calculated before it is masked")

- Saved-result lifetimes and widespread register differences in a nested check runner: recover its semantic
  inline boundaries and probe per-source `-ipa file`, including loader-owned constructor data.
  (matching.md: "Nested inline boundaries and file-level interprocedural analysis")

- Registers differ around an indirect log call: read the ROM format string and check every variadic argument before
  tuning allocation. (matching.md: "A missing variadic log argument")
- A function of a handle or a `void *` whose registers differ: take the parameter as the SDK does (`int s`, `void *`)
  and cast it inside. (matching.md: "A handle parameter cast inside")
- An `[rn, rm]` store through an unpacked offset with its registers swapped: assign an assigning macro's result
  again, `p = NNS_G2D_UNPACK_OFFSET_PTR(p, base)`. (matching.md: "A macro that assigns")
- Rows of a matrix product whose registers rotate: the row of `a` in locals and a destination pointer of its own.
  (matching.md: "They match only with the row of `a` in locals")
- Two of several independent statements with their registers swapped: write the other one first.
  (matching.md: "Of independent statements that all feed later code")
- Every register moved, with a constant built and spilled at its first use (`movs r2, #7; str r2, [sp, #0xc]`) and a
  select that overwrites it later: a ternary argument whose arm is that constant. Write the call in each arm of an
  `if`/`else`. (matching.md: "A ternary whose arm is a constant")
- Zeros loaded from a stack slot at each use and a constant hoisted into a saved register, where ours uses `movs`:
  move a later loop into a `static inline` that counts with its own parameter, passed that constant.
  (matching.md: "A constant held in a stack slot")
- A constant copied from a saved register (`adds r1, r7, #0`) at uses where the original writes the immediate, or the
  reverse: the hoisted-constant budget. Give a statement before the first copy one more (or one fewer) named step, or
  declare locals `int` instead of `u32` (one node each); test one node short of the edge.
  (matching.md: "within a budget") Past a join followed by calls, field stores don't move the edge and calls move it
  in jumps; inline wrappers and locals for call results don't move it. (matching.md: "doesn't follow a node count")
- A sum of a spilled base and a constant whose result takes the other operand's register (`adds r2, r3, r2` with the
  constant in `r2`): the result goes to the operand generated last, and a spilled reload is generated at the add. The
  constant takes it when the base is a real load (field, stack parameter, address-taken or `volatile` local), stays
  in a saved register, or the constant is shared in the block or built in a loop. (matching.md: "the register of the
  operand generated last")
- A multiply's operand copy in `r4` with the index in `r1`, where the original has the reverse: give the index an
  enum-typed local. (matching.md: "An enum-typed index local stays a variable of its own")
- An array's address hoisted above an `if` whose arms both read it, where the original builds it in each arm and
  reuses an earlier shift: a flat array indexed `[i * 2 + k]` instead of `[i][k]`. (matching.md: "Reads of a
  two-dimensional local array")
- A multiply's operand copied right after its `subs` where the original loads the constant first: give the index
  its own variable and index the elements in place. (matching.md: "The copy of a multiply's operand")
- A table's base and index swapped between the two registers (`ldr r4, =table; muls; adds r4, r4, r2` in the
  original): index the table in a `static inline` taking the index as an enum type. (matching.md: "A table indexed
  by a field")
- A narrow stack parameter read from its slot at each use where the original loads it into a saved register at
  entry: declare the parameter `const`. (matching.md: "A narrow stack parameter used twice")
- The reverse, a narrow stack parameter held in a saved register (with another parameter moved to a new slot) where
  the original reads it from its slot at each use: its uses share one conversion; give one of them a `long` operand,
  the SDK's `s32`. (matching.md: "share one conversion")
- A constant step subtracted through an `fx32 *` (`*z -= c ? -0x100 : 0x100`) with the step and `*z` in each other's
  registers: `long` constant steps (`0x100L`), as the SDK's `long` `fx32` converts them. (matching.md: "A constant
  step subtracted")
- An index kept in two registers (copied after a division and again before a multiply) where ours keeps one: a
  `const` struct parameter with its pointer field read at each use. (matching.md: "A pointer field read through a
  `const` parameter")
- A stack-passed parameter kept on the stack where the original loads it into a saved register: copy it into a
  local of the other signedness (`int pos = index;`). (matching.md: "A 32-bit parameter passed on the stack")
- A 64-bit sum of products in other registers, or an `ldm` of two fields where the original has two `ldr`: build
  the sum with `+=`, one product a statement. (matching.md: "built up with `+=`")
- A `u64`'s low and high words in each other's registers before it goes to a 64-bit hardware register: write it in
  one `u64` store, not two 32-bit halves. (matching.md: "64-bit hardware register in one store")
- A checksum comparison with swapped `cmp` operands: store the `u8` call result in a local before comparing it
  with the parameter, as `DecodeRC4` does. (matching.md: "`DecodeRC4` needs the `u8`")
- Locals get registers in declaration order: reorder the declarations. (matching.md: "declaration order of locals")
  Declaring every local first and assigning them below moves both registers and slots (matching.md: "declared first")
- A pointer needs an earlier slot but a later store: declare it without an initializer and assign it after other
  pointers, as in `GetWEPData` and `GetWPAData`. (matching.md: "A pointer that must")
- An element pointer grows the frame and replaces the original's struct-base-plus-index field access: index the
  member at each use; declare a saved flag before a zeroed index and assign the flag afterward if the loop counter
  must copy that index's zero. (matching.md: "each received picture is accessed")
- A `u16` local and `local ± 1` sharing a register can push a parameter out of r0: a wider local keeps them apart.
  (matching.md: "wider local")
- A ternary store computes the address once, an `if`/`else` store in each branch, and at the end of a function a
  copy of the epilogue in each. (matching.md: "ternary store")
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
- A loaded field and its masked value swapped (`ldrb r3; and r2, r3, #0xf0` where the original loads into `r2`): read
  the field in the later test through a second pointer to the same data, such as the typed view the code passes on.
  (matching.md: "A field and a value derived from it")
- A constant stored from `r0`, built after the first parameter's last use, where ours builds it early in another
  register: the function returns that constant, though no caller reads it. (matching.md: "A leaf function that
  builds a constant")
- A pointer local to a struct's element costs a callee-saved register; index the element at each use instead.
  (matching.md: "A pointer local to an element")
- `arr[count++] = x` and `arr[count] = x; count++;` allocate differently, as do `count = 1; arr[0] = x;` and the
  reverse. (matching.md: "arr[count++]")
- A pointer stepped by `*p++` that lands in a callee-saved register above the others (MSL's `dsi/1.2`, NitroSystem's 2.0
  Thumb), or whose spill takes a temporary's slot (`dsi/1.1p1`): write the step as its own statement, `*q = *p; p++;`,
  for every `*p++` on it. (matching.md: "post-increment or post-decrement")
- A register swap between a new local and a value derived from the parameters: store the value in a parameter the
  function never reads, or in a variable an earlier branch used (fdlibm 5.2's `s`). (matching.md: "a variable the
  function already has")
- A narrow local colored after every declared one, whatever its declaration's place, when it is only used widened:
  declare it `int`. (matching.md: "only ever used widened")
- A narrow loop counter colored after a later-declared local, passed on every pass to an `int` parameter: give the
  callee the counter's type when its other callers cast to it. (matching.md: "The wider type can be the callee's
  prototype")
- Registers of a scheduled function swapped although every order of the computation was tried: write the `volatile`
  stores first, and read fields at each use. (matching.md: "registers still follow the source order")
- A loop counter in a higher callee-saved register than the compiler's temporaries, with every instruction right:
  declare it `u32`, the SDK's `unsigned long`, not `unsigned int`. (matching.md: "`WDSScanCallback`")
- An array entry's address recomputed after a call, where the original keeps it from before the call in a saved
  register: declare the index `u32`, the SDK's `unsigned long`. (matching.md: "`SortBySpeed` recomputes")
- A constant added or subtracted before an index where ours folds it into the base, or the reverse: the index, count
  or field's type, `int`/`unsigned int` against `u32`/`s32`. (matching.md: "Game Freak's code mixes the SDK's types")
- A caller's scheduling or stack slots differ only around a call: the callee's parameter type, `int` against `long`.
  (matching.md: "A narrow argument passed to a parameter")
- A loop's setup registers swapped in a function that calls an inline with `u16` bounds: declare the inline's index
  parameters `int` (`g2d_OAM.c`'s `LoadAffine_`, 14 bytes to 4).
- A field read through a pointer parameter folded where the original builds the address, or the reverse: the original
  uses the pointer once more or less, or a `*(T *)&p->m` cast stands in for the field's real type. (matching.md:
  "depends on how many references")
- Two locals of a large function swapped against their use counts: the declaration order decides there. (matching.md:
  "`FSi_GetPathCommand`")
- A `cmp rX, rN` against a register that holds 0, with other registers moved around it, where the original has
  `cmp rX, #0`: test the pointer as `if (p)`, not against `NULL`. (matching.md: "A pointer compared with `NULL`")
- A zero kept in a saved register across a call and returned for `FALSE` (`movne r0, r6`), where the original
  builds `#0`: write every `NULL` test of the pointer in the function as `!p` or `p && ...`, not just one.
  (matching.md: "every `NULL` test that shares it")
- A sum used as an index goes to the register of one of its terms unless it has its own variable.
  (matching.md: "array index that is a sum")
- The operands of `*` are loaded in source order. A product assigned to its own variable gets a new register.
  (matching.md: "operands of `*`"), (matching.md: "A product assigned")
- An `adds` emitted destination first (`adds x, x, y`) where the original has it second: the source's `x = y + x`
  is rewritten to `x += y`; the original summed into a variable of its own that wasn't forward-substituted.
  (matching.md: "An add whose destination is one of its operands")
- A three-term `|` chain with its loads swapped: swap its first two terms. (matching.md: "three-term `|` chain")
- Where a flag is first set decides which register builds its zero. The register a shared zero gets follows statement
  order, and stores of 0 before a loop take the zero of the variable its initializer sets first, as in
  `for (i = 0, count = 0; ...)`. (matching.md: "Where a flag is first set")
- A shared zero built in a fresh register right after a call, ahead of constants whose stores it follows in the
  asm: write its stores before those constants'. (matching.md: "`CtvtGameTarget_Init` emits")
- Registers swapped around a flag set `TRUE` and overridden in an `if`, with the instructions already right:
  write it as a ternary, `x = k != 0 ? TRUE : FALSE;`. (matching.md: "the ternary still allocates the")
- Loop invariants a register off, around a local set by default and overridden in an `if`, with the instructions
  already right: assign it in both arms of an `if`/`else`, or with a ternary. (matching.md: "An `if`/`else`
  assigning each arm")
- A register swap with otherwise identical code, in code that mirrors a sibling function: try that code as a
  `static inline` helper. (matching.md: "Moving code into a `static inline` helper")
- Chained stores of one constant (`a = b = TRUE`) share a register; separate ones may not.
  (matching.md: "Two stores of the same constant")
- A blend's colors in swapped registers around clamps through one reused wide temporary: clamp each channel straight
  into its own `u8` with `MATH_CLAMP`. (matching.md: "A blend's clamps written into their own narrow locals")
- Variables of an inner block are allocated apart from the function's. (matching.md: "declared in an inner block")
- A zeroed struct passed by value in a loop is copied from another zero variable: declaring it in a block around the
  loop changes which. (matching.md: "zeroed struct a loop passes by value")
- A narrow type in a wider local: `GFL_BGSysAllocChar` only matched with a `u8` tile size held in an `int`.
  (matching.md: "plain change")
- Diagnose with `tools/decomp/locals.py`, which shows each variable's register.
- A parameter spilled after a register copy, where the original spills it first: the callers narrow it, so it is a
  `u16` or `u8`. (matching.md: "callers narrow with shifts")
- A local spilled first and reloaded for its compare, where the original compares a register copy (`mov r1, r0`) and
  stores it after: the local is word-wide; if widening it adds narrowings at calls, widen the callees' parameters too
  when nothing in their code fixes the narrow type. (matching.md: "A local follows the same rule")
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
- A field's offset added to the base before a register index (`adds r0, r0, #4; ldr r1, [r0, r1]`): the address of
  `p[i].f` shared by two accesses and kept for a second use, as a read-modify-write (`f[i].b += v`), two stores to
  one element, a read and a copy into the element, or a store in the `else` of the `if` whose condition reads it
  (`MonolithPower_Equip`). Two reads merged into one load fold back to index first, as does `(&p->f)[i * 2]`;
  `#pragma opt_propagation off` keeping them offset first says the original merged them late. (matching.md: "that
  MWCC shares between two accesses")
- Every register moved by a field that the caller keeps in a saved register after an inline read it, where the
  original loads it again: the inline takes a `const T *`, even from a caller's plain pointer.
  (matching.md: "a `const` pointer from a caller whose")
- A loop-invariant field in the register after every declared local, where the original keeps it among them: read it
  into a local of the type the loop compares it at, declared at that place (`u32 movie = rules->movie;` against a
  `u32` bit field). (matching.md: "register after every declared local too")
- Registers of a scheduled block that no declaration or statement order of the same steps gives: read them as the
  order the values are defined (`--opt=-O1` shows it unscheduled) and interleave the statements so, as a per-axis
  save and move. (matching.md: "Registers are colored in the order the source defines")
- Table addresses reloaded from the pool at each use and a constant built in every copy of an unrolled inline,
  one pushed register short: combine each call's result as soon as it comes (ORed into a `u8` accumulator),
  keeping only values that straddle the stores. (matching.md: "collected before they are combined")
- A global's address loaded from the pool again before each call, where ours keeps it in a saved register: a
  pointer local MWCC did not propagate. Check with a local and `#pragma opt_propagation off`, then look for the
  spelling (a member's address in the local, a cast); `&s.member` written at each call is shared like `&g`.
  (matching.md: "is copy-propagated into its uses, and the")
- A pointer the caller keeps and an end an inline returns swapped between `r4` and `r5`: advance the inline's pointer
  in two statements and return it from the `if`, `end = cmd + 3; end += cmd[2]; return end;`.
  (matching.md: "advances in two statements")

- A struct field loaded again for each of several tests in a row, where ours reuses one load: the field is shared
  with other threads and `volatile`. (matching.md: "A field that other threads change")
- An RC4 swap with the wrong registers: write it through one temporary, and group a masked sum as
  `(table[i] + (j + key[k])) & 0xff`. A named `in = src + i` pointer that takes the counter's register: index
  `src[i + k]` directly. (matching.md: "An RC4 key schedule's swap")

- A loop's character and index swapped although every order of the locals was tried: read the character into an
  `int c;` declared first at the top. (matching.md: "declared first at the top")
- Don't try `register` for allocation or a counter that nothing reads: MWCC 2.0/sp2 ignores it. A `const` on an
  initialized local can move its stack slot. (matching.md: "ignores the `register` storage class")

## Stack slots or frame size

- A write result missing its store to a later read's output slot: use one transfer-length local for both operations
  (`DWCi_Http_Thread`). (matching.md: "one transfer-length local")

- Raw rail progress spilled as converted scalars: keep the signed position/length snapshot and let its ratio
  inline read through a `const` pointer. (matching.md: "a progress snapshot")
- Edge and corner sums unexpectedly shared, with fewer spills: signed edge arithmetic can coexist with unsigned
  corner arithmetic; cast the operands, not only the result. (matching.md: "Signed and unsigned sums")
- Spilled locals in reversed slots around an accessor's result: read the field directly, typed as the SDK has it.
  (matching.md: "A `static inline` accessor that returns a field cast")
- SHA-1's five-round groups with separate index/counter spills: reuse the message-word index and round-group
  counter across the stages (`CPSi_sha1_block`). (matching.md: "One word index `t` and one group counter `i`")
- A hash's unrolled rounds spilled differently: write each round as statements, and the counter as `int`.
  (matching.md: "The rounds of a hash written as statements")
- A local in a saved register where the original spills it and keeps a zero there for two field stores: put another
  of the block's stores between those two. (matching.md: "Whether a zero stored to two fields keeps a saved register")
- A stack frame for a wide parameter where the original keeps it in registers and computes an expression again after
  a call (`sub r1, r8, #0xc`): write the expression at each use, not in a local. (matching.md: "written at each use
  around a call")
- An archive handle or other local in a low slot where the original has it above the optimizer's temporaries:
  move the block that uses it into a `static inline` that declares it. (matching.md: "A local of an inlined
  helper")
- A dispatcher's `static inline` handlers allocating unlike the original: write them as the arms of the one function,
  each arm's locals in its block. (matching.md: "Four `static inline` handlers")
- A local slotted above the temporaries where the original's value sits among them: the expression written at each
  use, which CSE shares as a temporary. (matching.md: "expression written at two uses")
- A NitroSystem (2.0/sp2) function's spill slots in another order: locals are slotted function scope first, then
  the `if` block's, then the `else` block's, then inlines' in reverse; move them to function scope.
  (matching.md: "In NitroSystem's 2.0/sp2 code a pointer sum")
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
- A local's slot taken by a loop-invariant temporary above the spilled parameters: declare the local with the type the
  loop compares or combines it at (`u32 include`), or passes it at (the callee's `int` parameter). Two such copies
  can hide each other, so check `locals.py` after each type change. (matching.md: "loop-invariant converted copy")
- A spilled value in the lowest slot that should be among the declared ones: split a chained assignment `a = b = x`.
  (matching.md: "chained assignment")
- Block-scoped locals sat above function-scope ones in `particle.c`, where a block-scoped `{0, 0, 0}` initializer and a
  non-`const` parameter fixed it; NitroSDK's Thumb files without `-ipa file` and NitroSystem put them below.
  (matching.md: "a block local sits below")
- A padding slot stored twice before a call, one field read through a register and one through the slot: a pointer local
  to a field of a field. (matching.md: "A padding slot stored twice")
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
  a ternary, not a default and an `if`, or returned through one, `return c ? -1 : count;` (`STD_TVSScanf`).
  (matching.md: "A local assigned with a ternary is spilled among")
- Spilled values interleaved in assignment order where an inline's by-value parameter groups its slots apart: pass
  the value's address as a `const` pointer and dereference it in the inline.
  (matching.md: "An inline's parameter is a named local")
- A common value's spilled temporary at the wrong end of the frame: move its first use, not a local's declaration,
  across the first call. (matching.md: "spilled temporary takes its slot by where its first use")
- A long-lived local spilled by a loop that stores two fields of `list[i]`: store through `T *entry = &list[i];`.
  (matching.md: "keeps the offset live as a third temporary")
- A struct local's members in the lowest slots, every declared slot moved up: MWCC split it, since its address
  did not survive into the optimizer; an initializer, a real call with `&s`, a copy through memory or a pointer
  live across blocks keeps it whole. An accessed enum-typed member keeps it whole at no cost, with direct
  `[sp, #off]` accesses. (matching.md: "split into one variable per member")
- Values in the lowest slots, below an inline's parameter copies, in assignment order, that no scalar spelling,
  inline getter or declaration order moves there (one of them set in one branch only): make them the members of
  one struct local, which MWCC splits. (matching.md: "The split members are slotted with the temporaries")
- A register parameter stored at entry and loaded back from its slot, where ours copies it out of the register:
  the parameter is narrower than `int` (for a static helper, the type its public caller passes); loop with a
  separate `int` counter. (matching.md: "A register parameter stored at entry and read back from its slot")
- Diagnose with `tools/decomp/locals.py`, which shows each variable's `sp+offset`.
- A byte-sized running value kept on the stack and reloaded across calls where ours holds it in a register: declare
  it `u8`. (matching.md: "A running sum that only ever holds a byte")
- `positionShakingSpot`'s missing shared frame anchor: a `u8` two-flag mask in the caller and callee recovers it,
  despite word-sized spill accesses. (matching.md: "source-availability mask in the parameter")

## Instructions in another order (scheduling)

- A table index computed after a spill where the original computes it before: the table may be a function-local
  `static` in an inline getter. (matching.md: "holds it as a function-local `static`")

- Packet sequence stores in another order: snapshot the current sequence across the global increment.
  (matching.md: "snapshots the current `u16` sequence")

- Two field loads in the other order, with every other instruction right: the field the original loads first may be
  `volatile`; prove it by probing without it. (matching.md: "A `volatile` field is loaded in source order")
- A swapped scalar's byte-copy address or registers differ: a `const u8 *` object-representation view with fixed
  byte stores matches both SC integer writers. (matching.md: "A swapped scalar serialized")
- `sciInterfaceDestroy`'s marker-store addresses match with uniform direct unsigned-byte views instead of pointer
  locals. (matching.md: "`sciInterfaceDestroy` matches")
- Consecutive reads from `[p, i]` then `[r, #1]`, `[r, #2]`, or a `- 1` folded into a table's literal address, where
  ours adds to the index each time: write the pointer sum, `*(p + i + 1)`. (matching.md: "A pointer sum and an index")
- An inline's loads with a field's offset folded in (`[r, #4 + k]`), where the original adds the row to a base
  hoisted before the loop and reads `[base, row]`, then `[r, #k]`: the inline takes that argument as a `const void *`
  and casts at each read, a lever its comment should name. (matching.md: "keeps the argument one address")
- `lsr` then `orr ..., lsl` where the original shifts left first (a halfword swap): swap the operands of the `|`.
  (matching.md: "An OR of two shifts of one value")
- An inlined SDK call whose packed value is built in another order: check the inline's parameter types against the
  SDK (an `int` where ours has an enum). (matching.md: "An inline's enum-typed parameter")
- A stack argument stored after the next argument's setup starts, where the original stores it first, around a
  spilled local, or the local's reload placed above stores the original makes first: sweep the local's width
  together with its callee's return type and `value` local. (matching.md: "A spilled local's width for the rule
  below")
- A spilled flag's constant store right after its `movs`, where the original stores it after the next field store:
  sweep the widths of the block's other spilled locals against their callees' returns. (matching.md: "The local
  whose width matters")
- A load the original issues between another load and its use, which ours issues ahead of that load: write it after
  the use, for the scheduler to move up into the stall, then sweep the declaration order for the colors.
  (matching.md: "into the load-use stall")
- A value computed later than written, at its only use: an enum local stays where it is assigned.
  (matching.md: "enum local's value")
- A load through a pointer moves above stores only when the pointee is `const`. A load scheduled early points to a
  `const` parameter. (matching.md: "unless the pointee is `const`")
- A static's member loaded above a store through an unknown index into another static, or below it: ARM code
  moves the load up only when its offset is below half the object's size, and below the size less the offset of a
  higher member read earlier; a store moves above an access through a pointer by the same bound. The store, too, must
  be in the lower half of its own object; an unknown index counts as 0. A store kept below one through a pointer, as
  in the original, can need its field in the upper half of a smaller struct. (matching.md: "below half the object's
  size")
- A struct field's load above an indexed store that its offset can't give, or a reload that ours schedules between two
  stores to fields where the original reloads after both: the fields are separate statics, each at offset 0 of its
  own object, and a size-sorted `.bss` asks for `-ipa file`. (matching.md: "each at offset 0 of its own object")
- A table's row offset added to its address before the column (`add r0, table, row*3; ldrb [col, r0]`), where ours
  adds it to the column: a two-dimensional table indexed `[row][col]`. (matching.md: "A two-dimensional table
  indexed")
- A narrow parameter stored to its stack slot at entry, where the original stores it just before a call after
  loading through another pointer parameter: make that parameter `const`. (matching.md: "spill to its stack slot at
  entry")
- A NitroSDK parameter struct read late: check the SDK's prototype for a `const` our header lost (`WM_StartScanEx`).
  (matching.md: "lost the SDK's `const`")
- A parameter's copy and a stored constant in the other order at entry, all else equal: the stored-to fields are
  `volatile`, as for memory another processor shares. (matching.md: "Stores to `volatile` fields")
- A store order that no sweep reaches, with one value a call's result: hold the result in a local and sweep again.
  (matching.md: "When one stored value is a call's result")
- A byte offset added to a pointer, with the offset loaded first and on the left of the `adds`: integer arithmetic on
  the address, `(T *)((u32)p + n)`. (matching.md: "With two loads and no local") For a pixel, the terms in the asm's
  order, each scaled on its own: `(u32)base + x * 2 + y * 512`. (matching.md: "The pointer goes first whatever")
- A hoisted constant base (a VRAM address) on the left of the `adds` where ours puts it second: hold it in a pointer
  local and cast it at its use, `void *vram = G2S_GetOBJCharPtr(); (u32)vram + ofs`. (matching.md: "A constant
  counts as a local only")
- A call's result stored before a constant (`bl; str r0, [sp, #N]; movs r5, #0`) that ours builds first, unmoved by
  `#pragma scheduling off`: a block boundary. Write the calls with constants in each arm of the `if`/`else` before it,
  not through a local set in the arms. (matching.md: "Within one block MWCC builds a constant")
- A literal-pool load (an I/O base) that ours places above a store the original makes first: a block boundary, as
  above, or `volatile` register stores after both where the original's are plain, as NitroSDK's divider inlines
  (`CP_SetDiv64_32`) write them. A plain store goes first when stores after it stay in order: stores into the same
  object, or a `u64`'s high word. (matching.md: "A literal-pool load goes above a plain store", "memory successors")
- `b + 3a` where the original adds `3a` first: read `a` in a statement of its own and `b` at its use, or `a *= 3;` for
  two branch-assigned locals. (matching.md: "A weighted sum")
- A loop-invariant part of a sum added second (`adds r1, r3, r0`, the counter first) where the original adds it first:
  name it in the same block as its use, `int left = s + x; int column = left + j;`. (matching.md: "Named in the same
  block as its use")
- Arithmetic the original does before a call that ours does after it: assign the result back into the same variable
  rather than a fresh local. (matching.md: "a fresh local's computation down to its first use")
- A buffer row or table entry the original computes before a call's stack argument store, which ours folds into the
  argument after it: set the pointer to the base, then advance it (`src = buf; src += i * size;`), through a
  parameter that is not `const`. (matching.md: "A row of a buffer")
- All three values loaded before the stores of a vector: NitroSDK's `VEC_Set`. (matching.md: "`VEC_Set(v, x, y, z)`")
- One temporary of a vector's computed values in another register, with the stores in the original's order:
  NitroSDK's `VEC_Set`. (matching.md: "Since the values go to")
- `*p` loaded again at the top of a loop's body: walk a local copy of the parameter. (matching.md: "walks a parameter
  pointer")
- An address into another processor's table scheduled differently: index it through a pointer to the table's type.
  (matching.md: "another processor holds")
- A constant return value built between a load and its store: `result = CONST;` before that store, `break`, and one
  `return result;`. (matching.md: "A constant return value built between")
- A member of a file static stored through a literal of its own address (2.0/sp2): a pointer local to the member.
  (matching.md: "goes through the section's anchor")
- An inline's argument evaluated in the original's order only through the SDK's own inline, enum parameter and all
  (`G3X_SetFifoIntrCond`). (matching.md: "It goes the other way as well")
- The `smull` and `smlal` of a sum of two 64-bit products in the other order: swap the products in the source, since
  MWCC multiplies the right one first. (matching.md: "multiplies the right product first")
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
- A field read before a `MI_CpuFill8` (or another call) that could change it was the argument of an inlined helper.
  (matching.md: "a field read before a call that could change it")
  (matching.md: "inlined helper that makes the call")
- One load of a struct's pointer field for two stores through it, where ours reloads: an inline helper taking the
  pointer. (matching.md: "Two stores through a pointer")
- A parameter passed on the stack is loaded at entry, unless it is an `int` or `s32`. (matching.md: "passed on the stack is loaded")
  One passed on to a callee is loaded at entry only when both take the same type, and a narrow one after the
  register parameters are stored. (matching.md: "counts too")
- A spilled index parameter's multiply scheduled apart from the original's, which no statement order fixes:
  widen the parameter from `u16` to `int` in the prototype. (matching.md: "width of a spilled index parameter")
- A local's arithmetic done at its first use, past a call, where the original does it where written: give the
  local the type of the callee parameters it is passed to.
  (matching.md: "A local of another type than the callee parameters")
- A spilled reload above another spill's store, where the original stores first: the spilled locals are `u8`
  or `u16`; sweep the widths of several together. (matching.md: "can mean narrow locals")
- A spilled flag's constant store in a load's delay slot, where the original puts a register copy there: the flag
  is a `u8`. (matching.md: "A spilled flag's constant store")
- The `orr`s of a packed value combined in another order: write the `|` chain's terms in the order the original
  combines them. (matching.md: "An `|` chain is combined")
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
  write the stores through a pointer to a member (`&G.header`); a `&G` local of G's own type keeps them chained,
  unless G has an explicit alignment. (matching.md: "told apart by offset")
- Store order in initialization code is usually source order: try the stores in the asm's order first.
- A field of a local struct loaded before a call that doesn't fill it was read into a local there, as `targetX = target.x;`.
  (matching.md: "A field of a local struct")
- A nested call made after the outer call's other arguments, where the original makes it first: its result was a
  local. (matching.md: "nested in another call's arguments")
- Arguments loaded in order around a conditional one: that argument was a local set before the call.
  (matching.md: "A conditional expression among a call's arguments")
- A call among the arguments made before the calls of earlier arguments: it was inside an inline helper, and
  several such inlines are evaluated right to left. (matching.md: "an inlined function making a call")
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
- A walking pointer's one `add` placed right after its last access, where ours adds at the loop's end: read it with
  `*p++` at each access, not `p[k]` and `p += n`. (matching.md: "fold into the offsets of its accesses")
- Constant stores in descending order: one chained assignment written from the first element to the last.
  (matching.md: "A longer chain also stores from the right")
- Calls packed into one word with a kept bit, the bit tested before the calls and spilled: the calls into locals,
  then the packing. (matching.md: "packed into one word")
- A field read in an `if`'s condition and its body loaded ahead of the condition's other loads, where the original
  loads in tree order and reuses the register: global CSE merged the reads; only a second pointer path to the field
  (a cast, not `(*p).f`) or `opt_common_subs off` keeps them apart. (matching.md: "is a global common
  subexpression, loaded into a")
- A bit test's `movs rX, #1` ahead of the tested field's load, where the original loads first and builds the bit
  after it: the body sets the same bit; write the body's shift with the other signedness (`mask |= 1U << i`).
  (matching.md: "A bit tested in an `if`'s condition")
- A three-register `adds` done late, at the copy for a later `adds rd, #imm8`, with both operands spilled across
  calls, where the original adds where the sum is written: MWCC moved the add down by itself; a second use of the
  sum that isn't a copy (`f(x + 4)`, not `f(x)`) or another immediate keeps it in place, no pragma does, and each
  use of the incremented value gets its own copy of the sum. (matching.md: "whose only use is the copy")

## An instruction too many or too few

- Context snapshot copy calls differ: recover explicit SDK copies (`rcv_finished`).
  (matching.md: "saves and restores whole transcript contexts")

- A zero text-origin component missing from measured bounds: `CalcMsgWindowDimensions` shares a paired origin's
  field-by-field inline initializer with printer placement. (matching.md: "bounds include both components")
- Cut-in text allocation heap IDs: pass `HEAPID_TAIL` at each call; saved `u32` and `HeapID` locals change
  narrowing and sharing. (matching.md: "`fld3d_ci.c`'s `EventFieldEffectCutinSub_DrawStrToTexture`")

- A constant built by `rsb`/`sub` from a small value already in a register (`mov r0, #5; rsb r0, r0, #0x204`): that
  value is a local built in steps. (matching.md: "A constant that a local holds and changes in steps")
- An `and` with a bit-0 bit-field's width mask before an insert that masks again: the source masks the value before the
  store, `d->a = s[0] & 0xf;`. (matching.md: "A mask written before a bit-field store")
- A constant narrowed at a call after `movs` (`movs r3, #0x11; lsls; asrs`): an enum-typed local set at the top of
  the function, before the calls. (matching.md: "A constant narrowed at a call")
- A constant offset of a pointer argument built after a call where ours builds it later, or with the sum's
  registers swapped: an enum-typed local for the offset, assigned where the original builds it. (matching.md:
  "A constant offset added to a pointer in a call's argument")
- A table field's offset folded into a second literal (`ldr r1, =table+4`) where the original adds the element
  offset to the base after a plain read: read that field through an inline taking an `int` index.
  (matching.md: "Two fields of one element of a static table")
- A saved register, `bl` and return where the C tail-calls (`ldr r3, =f; bx r3`): the call passes a fourth register
  argument. (matching.md: "four arguments in registers")
- A Thumb tail jump through a register other than `r3` (`ldr r1, =f; bx r1`) is not compiler output, so it is the
  SDK's assembly: an `asm` function (`lib/` only). ARM code tail-calls through `ip`, and only without a frame.
  (matching.md: "always `r3` in Thumb")

- A narrowing before an `and` into a `u8` field: `x &= mask` narrows the mask, `x = x & mask` doesn't.
  (matching.md: "compound assignment to a narrow field")
- `ldrb; lsls #n; beq` where ours has `cmp r0, #0`: a doubling such as `x * 2` in the source, or `&a[x] != a` over
  elements of `1 << n` bytes, which folds to it; not a bit field or a shifted or masked `u16`, which load the
  whole container (`ldrh`); a `_Bool` tests with `cmp` too. (matching.md: "A bit field is loaded at its
  container's width", "The doubling can also be an address")
- A signed `/ 2` narrowed in separate steps before a loop (`asrs #1; lsls #0x10; asrs #0x10`, ours `lsls #0xf`):
  compute the value inside the loop body from a narrow local. (matching.md: "A signed halving kept apart")
- A reload between two stores of one value: a chained `a = b = v;`; separate statements store the narrowed value
  twice. (matching.md: "chained assignment to fields")
- A field loaded again after an `if`, where the original keeps one load (or spills it and reloads the copy): no
  reuse reaches past a join, so the source held it in a variable. (matching.md: "reused into an `if`'s body")
- A field loaded again in the second arm of an `||`, where ours shares one load: the function may read an object
  with an explicit alignment, which kept `UpdateScroll`'s `list->dragDelta` reads apart. (matching.md: "the
  attribute on any object or member the function reads")
- A member's address CSE'd into a callee-saved register from a store to a later call's argument, where ours
  recomputes it: a pointer local declared at the top, assigned just before the call — not at its declaration.
  (matching.md: "assigned just before the call")
- A constant built at entry in a pushed register and copied for a later float literal (`adds r1, r6, #0`), where
  ours builds it at the use (`movs r1, #0`): a float local initialized with that constant at its declaration, even
  a dead initializer. (matching.md: "A float local initialized with a constant")
- A stack parameter loaded again for one comparison (`add r0, sp, #N; ldrh r0, [r0]; cmp`) while its other uses
  share one load: declare the local compared with it `long`, the SDK's `s32`. (matching.md:
  "`CalcWarpTransferAddend`")
- A large offset derived from an earlier one (`subs r6, #0x80`) where the original builds it fresh (`movs; lsls`):
  a join just before it, or a pointer local to the earlier member, initialized at its declaration at the top.
  (matching.md: "rebuilt at each use in a scratch register")
- `-1` and `-2` derived from a table's row stride (`sub r2, r1, #0xc`) where the original builds them fresh (`mvn`):
  read the element in each arm of an `if`/`else` rather than picking the table into a pointer local first.
  (matching.md: "Small constants are derived the same way")
- A struct local's member addressed afresh at each access (`add r1, sp, #0x64; str r0, [r1]`) where ours uses
  `[sp, #0x64]` and a saved register: a pointer local to the member, set at its declaration. (matching.md: "A pointer
  to a member of a struct local")
- Two constants on one field folded into one (`subs #0xff` for `- 0x100 + 1`) where the original keeps both: write
  them as two statements on the field. (matching.md: "Two constants applied to a field")
- A header size folded into a record's offset (0x44 and 0x10 as 0x54), or a field offset built fresh where the
  original derives it from the 0x44 it holds: an offset local that starts at `sizeof(Header)` and takes one `+=` per
  step. (matching.md: "An offset local stepped by statements")
- A narrowing (`lsl`/`lsr` or `asr` pair) comes from a `u8`/`u16`/`s16` local, parameter or return type. A caller narrows
  arguments for narrow parameters, so an argument passed without them is for a wider one. (matching.md: "narrows an argument")
- A parameter passed on to a `u8` parameter without shifts is a `u8` too. (matching.md: "narrows an argument")
- Extra `u16` narrowings come from `u32 x = (u16)...` passed through a `u16` inline parameter.
- A sum truncated to `s16` before a comparison was stored in an `s16` local. (matching.md: "truncates to `s16`")
- A narrowing again after a clamp is `MATH_CLAMP`, a conditional expression. (matching.md: "narrowed again after it is clamped")
- A local reloaded from its stack slot before each use can be a `u8` flag, not `volatile`.
- The reverse: a zero loaded back from a spilled `u8` flag's slot for a later zero store, where the original stores
  one register again, is a `BOOL` flag, with the return and the caller's parameter it ends in.
  (matching.md: "The same goes for a returned flag")
- A local loaded again for a call after tests of it that share one load: the local and the callee's parameter are
  an enum. (matching.md: "tests its stat")
- A variadic function's named parameter read back from its stack slot for a call where ours keeps it in a register,
  or two stores through out-parameters in an order ours can't give: the value is an enum at every step, the callee's
  parameter and the field included. (matching.md: "typed so at every step")
- A local array's element loaded again after a call: it was read through a pointer to the element, set in the
  loop. (matching.md: "A local array's element is never loaded again")
- An `ands` with a run-time 0xffff before a `strh`, shared by two stores: a packed-byte expression in two inline
  calls; check the SDK inline's mask (`y2 & 0xff`, not `(u8)y2`). (matching.md: "An `ands` with 0xffff kept")
- Masks written with `~` give `bic`; an `and` with `0xef` is `x &= (u8)~FLAG`. (matching.md: "Masks written with")
- A round-up to 32 with `ands` against a -32 derived from a register 0x1f (`subs rX, #0x3f`) where ours has `bics`:
  the sum is done in 64 bits, `((u64)ptr + 0x1f) & ~0x1f`. (matching.md: "the sum done in 64 bits")
- ARM `and` with a 16-bit pool mask on a `vu16` register where ours has `bic`, or the reverse: `reg &= ~M` gives the
  `and`, `reg = reg & ~M` the `bic`. (matching.md: "no ARM immediate")
- MWCC doesn't propagate constants into enum-typed variables: a loop that checks its bound before the first pass, or
  a sum that adds a counter known to be 0, has an enum counter. `scReportSetPlayerData` keeps its known-zero error
  test with the real `SCResult` type. (matching.md: "enum type")
- A constant kept in a saved register across a call for a later register use, where the original builds it again:
  the back end shares a constant within an extended basic block, and only a join or a variable it doesn't
  propagate into (an enum-typed one) stops it; no pragma or integer type does. (matching.md: "shares a constant")
- A constant kept in a callee-saved register for calls, then reduced at run time before a field store
  (`subs r6, #0x20; str r6, [r5, #8]`), where ours folds it: assign the field, then update it in place,
  `p->size = size; p->size -= 0x20;`. (matching.md: "reduced at run time before a field store")
- A local holding a constant keeps its own register or slot, while a literal is hoisted.
  (matching.md: "holds a constant, like")
- An address computed before calls is reused after them only when the expression is the same, types included. An
  inline accessor recomputes it. (matching.md: "computed before calls is reused")
- A constant base address with offsets where MWCC folds each store into its own literal: a pointer local.
  (matching.md: "Stores to fixed addresses")
- A register's address written through and then used as a value (a DMA fill), where ours copies a register and
  every register moves: one pointer local per arm, `*fill = src; src = (u32)fill;`. (matching.md: "both writes
  through and then uses")
- A global's address loaded from the pool again before each call, where ours keeps it in a saved register: a
  pointer local `T *p = &G;` passed to the calls, with `G` explicitly aligned, tells whether the original held the
  address in a local; it is a diagnostic (`printsys.c`, `os_thread.c`), not kept without other evidence. (matching.md:
  "loaded from the pool again before each call")
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
- An array element's address formed again from a saved `i * size` just before a field load, where ours keeps the
  element's address across the body: read the field through a `static inline` taking a `const T *` to the element.
  (matching.md: "An array element passed whole")
- `const` table reads at a constant index are folded into immediates; reads in a loop, even an unrolled one, are not.
  An `ldm` into argument registers is a loop over a table. Reads through a pointer to the entry aren't folded either.
  (matching.md: "Reads of a `const` table")
- A value moved into an argument register before a call and used for nothing else is a parameter the prototype is
  missing. (matching.md: "prototype is missing")
- A constant built once for `r3` and the first stack slot is one `u64` argument. (matching.md: "is a `u64` argument")
- A caller that leaves an argument register untouched across a call is passing that argument.
  (matching.md: "keeps an argument register untouched")
- A constant that the original tests (`movs r7, #0` then `beq`) and MWCC folds away is held in a variable of an enum
  type, which MWCC doesn't propagate constants into. (matching.md: "doesn't propagate constants into a variable of an
  enum type")
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
  register: a raw buffer read through a typed pointer in `GFL_MsgDataLoadHeader`; first distinguish a shared frame
  base used for several locals. (matching.md: "a raw buffer")
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
- Two steps of constant arithmetic where MWCC makes one, or two registers where one cursor would do: a local for the
  first step, or a second local with a role of its own. (matching.md: "keeps the two steps only when")
- Two `rsb`s of a double negation (`-h * -t`) that MWCC drops: the negated value in a local assigned first, or a
  cast around it. (matching.md: "A double negation folds the same way")
- `rsb #0; add #c` for `c - (a + b)` where ours has one `rsb #c`: a conversion between the negation and the add, as
  NitroSDK's `long` `FX32_ONE` gives. (matching.md: "A negated sum folds")
- `sub; sub` for `d - last - d` where ours has one `rsb #0`: the difference in a local of the other signedness
  (`int change = d - last;`). (matching.md: "A difference taken back from its own first operand")
- `subs` where ours has `sub` for a 32-bit difference of `u64` values: subtract the `u64`s and narrow the result.
  (matching.md: "A 64-bit difference narrowed")
- `cmp hi; cmpeq lo` with `bls` on sign-extended `int`s where ours compares signed (`subs; sbcs`): an `int` compared
  with a sum holding a `u64` constant such as `OS_MilliSecondsToTicks(100)`. (matching.md: "An unsigned 64-bit
  compare")
- A constant loaded again and multiplied a second time where ours reuses the product: the two uses differ in
  signedness (`u32 q` and `(int)q * C`). (matching.md: "does not merge a signed and an unsigned multiply")
- An index's product computed again where ours shares one: for a `u16` index only an `(int)` or `(long)` use splits
  it; `(u32)`, `(unsigned long)` and `(u16)` change nothing. (matching.md: "A `u16` index widens as unsigned")
- An array's end as one folded constant (`add #0x5c; add #0x400`) where ours adds the member offset and the size
  apart: `&p->arr[N]`, not `(u8 *)p->arr + sizeof(p->arr)`. (matching.md: "The end of an array member")
- `add rN, sp, #0; add rN, rN, #4` for a stack struct's field: pass the struct as words to an inline that indexes them.
  (matching.md: "passed as a packet of words")
- `mov r0, #imm; cmp rX, r0` where the immediate fits: the limit is a variable set to the constant. (matching.md: "is a
  limit held in a variable")
- A constant built in a free argument register before a call and shared with a store: the callee takes one more
  argument. (matching.md: "shares with an earlier store of the same constant")
- An address computed differently for a `size_t` index: the SDK's prototype takes `size_t`, which is `unsigned long`.
  (matching.md: "`size_t` is `unsigned long` as well")
- A loop hoisted differently around `strncmp` or `strcmp`: `<string.h>` against another header's prototypes.
  (matching.md: "`<string.h>`'s `strncmp`")
- A dot product's translation added after the shift, 8 bytes larger in one expression: `+=` in a statement of its
  own; a sum of two products can need the one expression instead. (matching.md: "adds each translation")
- An `and #0xff` on each `u8` result of an inline before the caller shifts or ORs it: the inline's local is an `int`
  that the `u8` return narrows; a `u8` local drops it. (matching.md: "An `and #0xff` on each result")
- An `and` before an inline's table load in some inlined copies only: those calls mask their argument, and the
  inline indexes with the bare parameter. (matching.md: "in some inlined copies and not in others")
- `movne rN, #0; moveq rN, #size; add` where ours selects an index and multiplies (`mla`): MWCC never pushes the
  multiply into a select's arms, so the source selected byte offsets. When nothing else indexes the elements, that
  is a raw `u8` buffer of slots, the slot cast to the element, `(T *)&buf[c ? 0 : SLOT]`, as a stride rounded up to
  32 bytes also says. `moveq #1; movne #0` is `c ? 0 : 1`. (matching.md: "into the arms of a select")
- A `u16` narrowed twice in a row (`lsl; lsr` in place, then again into another register): an identity in the
  source, such as the DSP pipes' `count = count & ~(DSP_MEMORY_UNIT - 1);` (1 on the ARM9); `&=` folds it first.
  (matching.md: "narrowed to `u16` twice in a row")
- A fixed address's bit flag read as `ldrb [r0, #4]` from the block's base where ours folds the offset into the
  literal: read it through a pointer local of the block's struct. (matching.md: "read through a pointer local")

## Branches and block layout

- A single conditional-branch byte differs: verify both destinations and return values before tuning allocation.
  (matching.md: "A near match can still contain a wrong branch condition")

- Each arm of an `if`/`else` ending in `ldr base; movs rN, #k; b` to a shared `str`: the field is stored in each
  arm, not through a local stored after the join. (matching.md: "cross-jumped at its store")
- An early exit 8 bytes short, or a call's result returned through another layout: give the exit its own call
  and return, or return through a `result` local. (matching.md: "An early exit with its own call")
- A flag set in several cases and tested at the end to pick a shared block, where the original branches straight to
  it with fewer registers pushed: forward gotos to labelled tail blocks. (matching.md: "chosen by a flag tested")
- `sciWsCreateSessionCallback` keeps three `bne; b` pairs with failure-first parser tests and success in the `else`.
  (matching.md: "`sciWsCreateSessionCallback` keeps")
- A combined `&&` camera check moves a copied parameter, where the original has two false branches with the same
  call: nest the checks and write that call in both false arms. (matching.md: "tests `cameraEnabled` first")
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
- A `? 0 : 1` with inverted branch polarity whose 1 the original takes from a long-lived constant register: enum
  constants for both arms as well. (matching.md: "also has enum constants for both arms")
- A negated range test whose branch targets differ: write the positive test of the complement, an `||` chain.
  (matching.md: "A negated test and its expansion")
- Two 0/1 values built by `movs #1; b; movs #0` and combined with `ands`, no branch between the tests: a bitwise `&` of
  two comparisons, as in a touch hit test. (matching.md: "joined by a bitwise `&`")
- Blocks are laid out in source order. A switch whose `default` code comes first had `default:` written first, and
  `if (!f()) return FALSE; n++;` puts the return before the code that goes on. (matching.md: "Blocks are laid out in source order")
- A loop's early exit placed after the whole body, just before the bottom test: wrap the body in `if (p != NULL) {
  ... } else { exit }`. (matching.md: "In a loop the `else` lands after the whole body")
- Identical statements in different branches are merged, so a jump into the middle of another block means the same
  code was written there. (matching.md: "Identical statements in different branches")
- A branch to the next instruction comes from cross-jumping a shared tail. (matching.md: "cross-jumping")
- One `bl` shared by an inner arm (which ends in `b` to it) and the outer `else`, where the original has two: the
  calls were cross-jumped at the `bl` alone; the original's outer then-arm ended in a block of its own, such as a
  plain statement written after the call in both outer arms (one ending in a conditional branch doesn't merge).
  (matching.md: "at different depths")
- A field loaded again right after a join (`ldr r0, [r7]`) where ours reuses the stored register, with a `b` to a
  `b`: the store was written at the end of both arms. (matching.md: "loaded again right after it")
- A load at the top of both arms of an `if`/`else`, where ours hoists it between the test and the branch: one
  arm began with another statement, as `destPixel--;`.
  (matching.md: "A load that both arms of an `if`/`else` begin with")
- `bne next; b target` where `||` gives one `beq`: the same body in both arms of an `if`/`else if`. (matching.md:
  "`bne next; b hide`")
- Early `return FALSE`s go to one shared tail only when the C has one trailing return (an `if`/`else if` chain, or a
  `result` variable, or an inlined function: see the next line). A return that branches to the wrong one of two equal
  `b end` trampolines: change which nearby exits are `return` and which are `break` or the end of an `if`.
  (matching.md: "goes through a `b` within range")
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
- A spilled argument reloaded at a join for one call (`ldr r0, [sp, #N]`) that the original passes from `r0`: the
  call was written at the end of every arm of the chain. (matching.md: "call written once after a chain of arms")
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

- A loop in the source that the original has as one `cmp; ble` of its bound: MWCC removes a loop whose body doesn't
  depend on its counter. (matching.md: "doesn't depend on its counter")
- An unrolled loop over tables with the wrong registers and spills: step the tables by pointer (`*p++`) instead of
  indexing them. (matching.md: "A table read in an unrolled loop")
- `scSubmitReport` needs a `u32` digest-clear counter for `subs; bne`; its signed counter adds a compare.
  (matching.md: "`scSubmitReport` needs")
- A running offset's `movs rN, #0` placed before hoisted loop invariants where the original zeroes it after: write
  it as a counter times the step. (matching.md: "A loop offset that starts at 0")
- A loop counter stored and reloaded where the original keeps it in a register for the test (2.0/sp2): put it last
  in the `for` increment's comma list. (matching.md: "a `for` loop's comma-separated increment")
- In library code, a store pointer stepped with `strb rX, [r2], #1`, set up after the loop's entry test and restarted
  at `base + i`, where dsi/1.1p1 indexes: probe 2.0/sp2, which TwlSDK's `syscall_twl_ltd.c` matches under.
  (matching.md: "`syscall_twl_ltd.c` matches whole under it")
- `while (cond)` is rotated, with a copy of its test before the loop. A loop that tests once, at its top, is
  `while (TRUE)` with a `break` or `return`. (matching.md: "is rotated")
- A function whose loops match only at `-O2,p` (entered with a `b`, empty loops kept) while the file's other loops
  are rotated: it reads an aligned object; align it rather than lower the flags.
  (matching.md: "At `-O2,p` no loop is rotated")
- At `-O4,p`, a loop guard whose branch also skips statements after the loop: a guarded `do`/`while` with those
  statements inside the `if`; a `for` inside an `if` tests twice. (matching.md: "where a `for` loop is rotated")
- A loop that runs once is unrolled when its counter and bound have the same signedness; `int i < NELEMS(x)` compares
  unsigned and keeps the loop. (matching.md: "loop that runs once")
- A static's field loaded on each pass of a loop that stores through a pointer, where the original loads it
  once: a local read before the loop. (matching.md: "hoists a static's field load out of a loop")
- `asr`/`ror` division of a parameter the callers pass as `u8` without conversion: a `u8` parameter copied to an
  `s32` local. (matching.md: "copied to an `s32` local")
- An address or value an inner loop computes from the outer counter alone is hoisted into the inner preheader.
  (matching.md: "inner loop computes"), (matching.md: "preheader")
- A hoisted row address split around a `volatile` store at the top of the outer loop's body (`G3_Begin`): another
  invariant of the outer counter, such as a texture coordinate, in a local assigned after the store.
  (matching.md: "splits the inner loop's hoisted row address")
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
- `p + v * 4` kept in a register with reads at `#4` and `#8`, where the original computes `(v + k) << 2` for each
  read: a pointer conversion around the element's whole address (`*(f32 *)&words[v + k]`), only where the buffer's
  type calls for one; even a cast to the element's own type hides it. (matching.md: "hides the address from that
  pass")
- The loop passes off for a whole file, as a test: `-flag no-opt_loop_invariants` and
  `-flag no-opt_strength_reduction` in `--extra-flags`; the `-opt` sub-options do nothing after `-O4,p`.
  (matching.md: "takes a pragma's name")
- Values the original builds before a loop and reuses after it, where ours computes them again: they were locals;
  arithmetic on a `u16` hoisted out of the loop, or out of a loop its entry test can skip, is not reused, nor is
  an inline's unless the variable and the parameters are both `int` or both `u32`.
  (matching.md: "Arithmetic on a `u16` hoisted out of a loop")
- A loop that adds each row to a member's address hoisted ahead of it (`add r2, base, row; ldrb [r2, byte]`), where
  ours regroups as `(p + row) + (offset + byte)`: a pointer set to the member and stepped, `list = p->arr; list +=
  i * N;`. (matching.md: "In a loop the stepped pointer")
- Stores of a spilled copy after the loop's guard, in the preheader, where ours stores it at the computation: copy
  after the last branch before the loop, and give the loop's `fx32`/`s32` locals the SDK's `long`.
  (matching.md: "A spilled copy of a value that the original stores") A narrow stack parameter read in the loop's
  sum is copied the same way. (matching.md: "A narrow stack parameter read in the loop's")
- A returning branch inside a loop that computes an address again in its own registers, where the original reloads
  the parts the loop hoisted: read the address into a local once in the branch and use it at both uses.
  (matching.md: "returning branch inside a loop")
- A loop entered with a `b` to its test at the bottom, where ours is rotated: a 0/1 test, `i < N ? TRUE : FALSE`, a
  `TRUE`/`FALSE` inline or `&& !found`. In ARM the bottom test is still a plain `cmp; blt`.
  (matching.md: "has a test whose value is 0/1"), (matching.md: "What keeps the `b`")
- The same `b` in every function that reads one object, where the file's other loops are rotated: the object has an
  explicit alignment, as a 32-aligned address in the ROM shows; another file's object takes it on its `extern`
  declaration. NitroSystem's 2.0/sp2 too (`NNS_SndArcStrmInit`). (matching.md: "An explicit alignment")
- The same in an outer loop around an inner one: begin its body with the inner loop,
  `while (head[i] != NULL) { p = head[i]; ... }`. (matching.md: "whose body begins with another loop")
- An empty loop that the original keeps and `-O4,p` drops: the function reads an aligned object (`lib/irc`'s
  `IrcLink_IsRecvIgnored`). (matching.md: "An explicit alignment")
- `mov rN, #bound` on a search loop's normal exit, before `cmp rN, #bound`: an inline search that returns its `size`
  argument after the loop. (matching.md: "A search loop whose normal exit")
- A static loaded in the loop on each pass where the ROM loads it once below the entry test, with a store through
  a pointer in the loop: declare the static inside the function. (matching.md: "A function-local `static` is hoisted")
- A second loop counter that the original zeroes, stores and steps but never reads: the same variable is also read as a
  value before the loop (`i = f(); if (i > n) { n = i; }`), which keeps its steps. (matching.md: "drops a loop counter
  that only indexes")

## Switches

- A switch's compares or jump table off by one case: stack the SDK's empty warning cases on `default:`
  (`case 3: case 4: default:`). (matching.md: "the empty labels set the jump table's range")
- An extra block that sets the default's value in a case: let the case fall through into `default`, and stack an
  empty case on `default:` where the SDK compiled out a warning. (matching.md: "A case that falls through")
- A case's table entry at the `b` ending the case before it, where an empty case goes to the switch's end: the case held
  a debug print compiled out; write `(void)0;` with a comment (2.0/sp2). (matching.md: "compiled out to `(void)0`")
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

- An angle constant one below a round index (0x3fff, 0x1fff): `degrees * 0xffff / 360`, not `DEG_TO_IDX`.
  (matching.md: "An angle index one below a round value")
- Float arithmetic calls `_fadd`, `_ffix` and others (swan's `__aeabi_*`). Rename to MWCC's name when a complete file
  fails to link. (matching.md: "MWCC's runtime helpers")
- An arithmetic operation on a literal passes the literal first; a constant in a local keeps its source position, and
  a compound assignment (`x *= 1.5`) passes its target first.
  (matching.md: "passes the literal first")
- A call inside `FX32_CONST(...)` is made three times; the game passes a local. (matching.md: "FX32_CONST")
- A literal in a compound assignment (`y += 0.01f`) is passed second. (matching.md: "compound assignment keeps")
- Float arithmetic on a local holding a constant isn't folded. (matching.md: "doesn't fold float arithmetic")
- A zoom's unit multiply before `sqrt` belongs to the scaled square-root curve also used with a varying coefficient
  by the actor spin; keep the shared curve's product and result at `f32`. (matching.md: "scaled square-root curve")
- Division and modulo call `_s32_div_f` or `_u32_div_f` by signedness. A `u8`/`u16` promotes to signed `int`.
  (matching.md: "_s32_div_f")
- `_ll_udiv` on sign-extended operands is an `int` cast to `u64`.
  (matching.md: "A 64-bit division")
- A 64-bit division calls `_ll_sdiv` for signed operands and `_ll_udiv` for unsigned; the probe masks the
  call, so link the file to check which. (matching.md: "and `_ll_sdiv`")
- An integer converted to float calls `_fflt` when signed and `_ffltu` when unsigned; a `u8` or `u16`
  converted to `f32`, by a cast or as a float operand, is unsigned, so a ROM `_fflt` wants an `int` local. The
  probe masks it. (matching.md: "`_ffltu`")
- A `u64` division by 64 that calls `_ll_udiv` where ours shifts: the function reads an object with an explicit
  alignment. (matching.md: "An explicit alignment")
- Two `_ull_mul` calls (by 64 from a register, then by 1000) where ours multiplies once by 64000, in one function of a
  file: ask whether it reads an object with an explicit alignment; a local set twice may then need to be two variables.
  (matching.md: "keeps a chain of 64-bit constant multiplies apart")
- A float one ULP off a round decimal is written with the shortest digits that round to it. (matching.md: "one ULP
  off")
- `__clz`/`__CLZ`/`__builtin_clz` are not MWCC intrinsics; they call out. A bare inlined `clz` in ARM library code is
  `nitro/math.h`'s `MATH_CountLeadingZeros`. (matching.md: "no count-leading-zeros intrinsic")
- Loops left unrotated, an element's address added at each access and a value loaded twice around a `clz`: the
  SDK's inline `asm` block does it, which a call can't. (matching.md: "The inline's `asm` block")
- A `memcpy`/`memset` of a constant size inlined where the ROM calls it: `<string.h>` declares it; declare it in the
  library's own header instead. (matching.md: "a header named `string.h`")
- The reverse, a `memset` or `memcpy` built in place (an `stmia` loop, or `strb`s for a few bytes) in a file whose
  library header declares the string functions: include `<string.h>` before that header, since MWCC goes by the first
  declaration. (matching.md: "goes by the first declaration")
- Two-bytes-a-pass copy loops (`ldrb [r3, #1]; ldrb [r3], #2`) where ours calls `strcpy` with a literal: take
  `strcpy` from `<string.h>`. (matching.md: "`strcpy` of a string literal")
- A hardware divide whose operand stores are a `str` pair, 8 bytes smaller than raw `REG_DIV_*` writes: NitroSDK's
  `CP_SetDiv64_32` and `CP_GetDivResult32`. (matching.md: "A divide by the hardware divider")

## Data and sections

- Duplicate protocol literals pooled once, with two independently grouped sets of globals: verify original
  source boundaries (`dwc_main.c`/`dwc_match.c`). (matching.md: "boundary at `DWC_AddMatchKeyInt`")
- MSL exception indices before ordinary rodata: restore `-Cpp_exceptions on` and register the emitted `.exceptix`.
  (matching.md: "`dsi/1.2` with `-Cpp_exceptions on` emits exception indices")
- A live empty assertion filename before earlier allocation literals: recover its named data object and probe
  per-function IPA with `explicit_zero_data on`; compare all data and pool targets.
  (matching.md: "A named `static char name[]")

- A local array initializer's bytes in a `.rodata` section of their own: copy a struct from a static instead.
  (matching.md: "A local array with an initializer goes in a `.rodata` section of its own")
- A `.bss` static reached through a literal of its own where the original reaches it from the section's base: give
  it a zero initializer (`= 0`, `= NULL`, `= { 0 }`); one the ROM has in `.data` takes `#pragma explicit_zero_data on`.
  (matching.md: "the same holds for zero")
- Odd instructions at the end of a same-size diff (`add sp, #0`, `.byte 0xff`): literal pool words, which can be a
  wrong constant; decode them before blaming registers. (matching.md: "decode the words before counting")
- Live `.data` statics separated by unused records: one initialized array retains every entry even when code indexes
  only some of them, preserving the original element offsets; `wipe_sub.c`'s line patterns match at 0, 0x10, 0x20
  and 0x28 this way. (matching.md: "One initialized array retains entries")
- String literals in another order: MWCC lays them out in the order they first appear in the source; a `""` the
  game has before the file's name needs an earlier use. (matching.md: "String literals")
- A literal laid out before every surviving use of it: code the original link dead-stripped. Each code literal has a
  section of its own, and a static called from code the optimizer removes is still emitted with its literals.
  (matching.md: "Each literal that code creates")
- A table of string pointers before its own strings, in the middle of a file's `.data`, where ours lays the strings
  out first: the table starts another original file (under `-ipa function` only a table above a file's first function
  comes first). (matching.md: "comes before its strings only when")
- Static data is sorted by size by a heapsort. Objects of 64 bytes or more, local initializers and unreferenced
  globals get their own sections. Predict with `tools/decomp/rodata_order.py`; the object list must be complete, so
  `--from-object FILE.o` lists every object a compile made and `--check FILE.o` fails when the prediction and the
  compiler disagree. A "no order works" verdict means nothing until `--check` passes. (matching.md: "Static data is sorted by size")
- On NitroSystem's `2.0/sp2`, initialized globals are ordered by size, smallest first, and objects of one size in the
  reverse of their declaration order (G3D's `anm.c`, `cgtool.c`). (matching.md: "gives each initialized global")
- Stores at offsets from a static's own address (`ldr r1, =obj; str r3, [r1]`) where ours goes through the section's
  base: the function doesn't know the object, so `-ipa function` with the object defined after it (G3D's `sbc.c`).
  (matching.md: "is one that the function doesn't know")
- The full model, checked by fuzzing MWCC: there is one list per file in declaration order, except tentative `.bss`
  statics, which join at the end in reverse order; a global initialized to 0 joins where it is defined. Each kind
  (rodata, data, bss) gets its own shared section. Unreferenced statics are dropped, but a literal of a dropped table
  that a later table repeats is listed where the dropped one stood. `rodata_order.py` doesn't model the per-kind
  sections or the `.bss` rule yet, but given the `.data` tables with the `.rodata` objects it predicts the `.rodata`
  order. (matching.md: "The list that is heapsorted")
- Equal-size objects that no order of the file's own objects puts in the ROM's order, where one more object would:
  an object the original link dead-stripped. A zero-size global (`const char *x[] = {};`) tests a place for it
  without adding bytes; diagnosis only, never source. (matching.md: "A zero-size global")
- `.bss` statics in another order: MWCC 2.0 lays them out in the reverse of their definitions without `-ipa file`, and
  with it in their order with the first one moved to the end. (matching.md: "with the first one moved to the end")
- A table that only one function reads can be a `static const` inside it, which moves it in the heapsort's list.
  (matching.md: "declared inside the one function")
- A `.rodata` template copied to a local, then patched with computed fields, in a section of its own: a local
  initializer with the computed values in its braces, in a block after any calls before it. (matching.md: "a few
  fields overwritten")
- When the prediction is wrong, move one declaration at a time and compare the built sections with the ROM.
  (matching.md: "prediction disagrees")
- No natural order found, and the size-sequence solver's fits are scrambled: search natural structures (groups
  contiguous, at the top or local to their only user) with the forced assignment as the cost, letting equal-size
  tables of a group take any order. (matching.md: "also searches natural structures")
- `static const` goes in `.rodata`, so a table in `.data` isn't `const`. (matching.md: "`static const` data goes in")
- Small unreferenced objects at the start of a file's shared `.rodata` (ahead of its smallest referenced table) are
  function-local `static const`s, which are emitted even when their reads are folded or absent. (matching.md:
  "function-local `static const` is emitted")
- An unread `{ 0, FX32_ONE, 0 }` or 16 zero bytes beside the camera vectors: the camera setup's up vector and
  `GXRgb[8]` edge colors, function-local statics of the function that creates the camera. (matching.md: "camera
  setup's up vector and edge colors")
- An "unreferenced global" there whose values a function uses as immediates is several scalars of that function, one
  per value (`ctvt_game_cam.c`'s crop, four `u16`); their count orders equal-size initializers anywhere in the file.
  (matching.md: "The values a function already uses as immediates")
- Unread 16 zero bytes among the shared `.rodata` of a file that sets up 3D: a function-local
  `static const GXRgb sEdgeColors[8]` in the function that sets up the 3D system (`plist_sys.c`; a guess, backed by
  overlay 207's `p_sta_sys.c`). (matching.md: "its function shows what it is")
- Function-local statics are listed last, after the tentative `.bss` statics, by function in reverse order; one in a
  `static inline` is listed where the inline is defined, and so are a local array and a local `static` that isn't
  `const`. (matching.md: "joins the size sort at the end of the file")
- An object at the head of the shared `.rodata` or `.data`, larger than the ones after it and packed against them:
  `-ipa function`, which sorts the data waiting at each function on its own; define the tables just before their
  first user, and give the file the flag in `config/compiler_flags.toml`. (matching.md: "Under `-ipa function` MWCC
  writes out")
- Local initializers sorted by size across functions, after the shared section and before the tables of 64 bytes or
  more, rule `-ipa function` out. (matching.md: "The reverse rules the flag out")
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
- Two literals for one static in a library file without `-ipa file`: add the flag. Read at offset 0 after the file's
  other statics: the static opens another file's `.bss`. (matching.md: "take MWCC knowing the static's definition")
- A Thumb read of a known static with no offset where the file's layout puts it off a multiple of 0x80: it opens
  another file's section, since the literal is `.bss` plus the offset rounded down to 0x80. (matching.md: "puts the
  static at a multiple of 0x80")
- DSi code in the LTD autoload that reaches a main global at a pooled base plus an offset: it was compiled in that
  global's file, in a `#pragma section` block for `.ltdmain`. (matching.md: "objects defined between")
- Two statics reached from one literal: `-ipa file` in `file_flags`, with the SDK's separate statics. (matching.md:
  "ask for `-ipa file` before anything else")
- A constant loaded from the pool with no relocation where MWCC folds it or builds it with `mov`/`lsl`: a linker
  symbol, `extern u8 SYM[]` cast at the use. (matching.md: "is a linker symbol")
- A gap in `.bss` before a 32-aligned object: the previous file ends there, unless it is built with `-ipa file`, which
  gives each object of 64 bytes or more a section of its own. (matching.md: "makes its file's whole `.bss` section
  32-aligned")
- A literal of `SYM + bias` with the bias subtracted at run time: element-pointer arithmetic (`arr + 0x880`) puts
  the bias in the addend, byte arithmetic adds it at run time, and the subtraction stays when the result is the base
  of a variable index, or for a scalar read when the address is converted to `s32` rather than `u32`. (matching.md:
  "A bias on a data symbol lands in the literal's addend")
- `.bss` runs that are each sorted by size, with the same layout unreachable in one file: the range is several
  files, each with `-ipa file`; and sweep the declaration orders of the statics. (matching.md: "With `-ipa file` the
  order of a file's same-size statics")
- A struct of statics whose functions need two addressings (base plus offset in one, a plain object in another): the
  statics of two files, each with its own `-ipa`. (matching.md: "A struct of statics whose functions need two")

## Function order and presence

- A Thumb `bx pc; nop` and ARM `ldr ip, [pc, #0]; bx ip; dcd ADDRESS` after a caller: the linker's veneer; call the
  far target from C. (matching.md: "is the linker's long-branch veneer")
- Identical Thumb code that the probe calls 2 bytes short, before an ARM veneer: the symbol includes the linker's
  `mov r8, r8` pad; size it without the pad with `config_fixes.py add-function`. (matching.md: "a pad that belongs
  to no function")

- SDK assembly in an `asm` function: pre-UAL spellings, `@name:` labels, `dcd`, `ldr rX, =value`, and one `asm`
  function per entry point, falling into the next; no `entry`, so a second name comes from the configs or the linker.
  (matching.md: "MWCC's ARM assembler")
- A prologue, early return or static addressing around SDK assembly: an `asm {}` block in a C function.
  (matching.md: "An `asm {}` block inside a C function")
- SPL's `1.2/base` compiler emits a file's functions in reverse source order.
- The MWCC linker here doesn't dead-strip unreferenced global functions.
- A static function whose symbol isn't renamed to its C name shows as "unknown" in the probe.
- TwlSDK code that resists respellings: rebuild it in the shape of cybervisi0n/libntr's reconstruction (its
  `static inline` setters and wrappers, `if (n > 0) { ... }` blocks with their locals, cursor layouts), in our own
  names. (matching.md: "in the shape of cybervisi0n/libntr's reconstruction")

## Diagnostic switches (not for committed code)

These narrow down which optimization causes a difference, after which you look for the C spelling. They come from
sm64ds-decomp's catalogue for another mwccarm build; the cases below are the ones tested here:
- `#pragma opt_common_subs off`
- `#pragma opt_strength_reduction off`
- `#pragma optimize_for_size on`
- `#pragma opt_propagation off`
- `#pragma opt_arithtransformation off`, `opt_foldconstants off` and `opt_treetransformation off`, internal names that
  the manual doesn't list (also as `-flag no-...`): the passes that shift and fold 64-bit constant multiplies; inside a
  loop `opt_loop_invariants` and `opt_strength_reduction` must be off too (matching.md: "internal pass names")

`#pragma scheduling off` and `peephole off` change nothing in 1.1p1's Thumb or ARM code, so they can't single out the
scheduler; `#pragma optimization_level 1` shows each constant built at its use (matching.md: "leave 1.1p1's Thumb
code as it is").

Tested here: `#pragma optimization_level 1` reproduced the whole shape of `btlv_input.c`'s `BtlvInput_DrawMoveTiles` (a
loop entered with a `b` to its bottom test, the hoisted-constant spill slots, the `i`/`work` registers) where -O4
restructures it. `opt_common_subs off` alone gives it the original's size too, and with a 0/1 loop test leaves 186 lines
against level 1's 249: its original computes `work + i * 4` and `param + i` again after every call, where the C without
the alignment below keeps them in registers across the calls. A function whose loop enters with a `b` to the test at -O4
may be one the original's global optimizer gave up on, or one that reads an object with an explicit alignment:
`WorldTrade_NationSortListMake` matched with its table aligned (with no address evidence that the original aligned it),
as did `NNS_SndArcStrmInit` with its decode buffer aligned and `BtlvInput_DrawMoveTiles` with its palette table aligned
(the alignment gave its loop entry and the reloads after each call; inline getters gave the order of its call
arguments), and `MBi_SendVolatBeacon` shows the same symptom in its row. The lever never applies to a function
whose original rotates its loops, since the attribute enters every loop of the function with a `b`: `MCSS_MakeMosaic`,
`addToDex` and libVCT's five rows reload and recompute like the aligned cases but rotate their loops, and no
placement helps them. A loop-free function shows it too: the attribute stops a field's offset folding into the
literal (`&G.field` as one pool word) and keeps two reads of a field apart, so an original with such a pool word or
a merged read wasn't compiled under it, as G2D's `NNS_G2dPushMtx` and `DoAffineTransforme_` show.
Ask first whether the function reads an aligned object, then try a 0/1 loop test: `Country_IsValidPlace`, which showed
it too and came within a byte under `optimization_level 1` or `2`, matched at -O4 with one (matching.md: "has a test
whose value is 0/1").
`#pragma opt_common_subs off` around one function alone matched `btlv_mcss.c`'s `BtlvMcss_MoveTaskEnd_Circle` and
`_Shake`, a field loaded ahead of a condition's other loads, where `opt_propagation`, `opt_lifetimes`,
`opt_dead_assignments`, `scheduling` and `peephole` off changed nothing (matching.md: "is a global common
subexpression, loaded into a").
They match with `ATTRIBUTE_ALIGN(4)` on their task structs' `work` member, written before the type, with
no evidence in the ROM that the original aligned it: after the name, the functions that store `work` lose
other optimizations (matching.md: "Where a member's alignment is written").
`#pragma opt_propagation off`, `opt_dead_assignments off` or `optimization_level 3` around one function alone matched
overlay 139's `UpdateScroll`, a field's offset added before the index for two merged reads, where
`opt_common_subs off` made it 4 bytes longer and `opt_lifetimes`, `opt_loop_invariants` and `opt_strength_reduction`
off changed nothing (matching.md: "that MWCC shares between two accesses"). The same three matched overlay 332's
`CygnusData_GetEntry` with its type stored first (an array member's offset, `data + 0x18` indexed by `r4`), where
`opt_common_subs off` did not. Both files also match whole at `-O3,p`, with only that function changed; a per-file
`-O3,p` that nothing else shows is evidence, like the pragma, not a fix.

An empty `if (a && b && c) {}` (three operands or more; two are removed early) or a discarded ternary statement
splits the block where it stands and emits nothing. When a register or CSE difference looks like a missing block
boundary, slide one through the statements: a place that matches shows where CSE differs, not that the original has
a statement there. `SortBySpeed` matched that way and then with `unsigned long j` and nothing added, so try the
index types first (matching.md: "three or more `&&` operands"). Document it, don't commit it.

Try one at a time with `try_variants.py`, which allows definitions before the function. If a pragma is all that
matches, write that down in the nonmatching row; it's evidence, not a fix.
