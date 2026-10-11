# How MWCC compiles

What decides whether CodeWarrior's output (`dsi/1.1p1`, Thumb, `-O4,p`) matches the game, learned from functions that
did or didn't match. Each rule names the function that shows it. They are grouped by what a diff shows;
`.claude/skills/match-function/levers.md` indexes the same rules by symptom in one line each. A rule found on one
function and confirmed on another goes here, next to the related rules; see [Decompiling](decompiling.md) for the
tools that show the differences.

## Registers

- A loop that walks on from a parameter reuses the parameter as its cursor, with the start kept in a local
  (`int start = pos;`), as `b_plist_main.c`'s `BPlistMain_GetNextPos` does; a separate counter swaps the registers.
- A `const` pointer parameter lets MWCC keep a value loaded through it across a store through another pointer; the
  original reloads it, so `btlv_effect.c`'s `BtlvEffTool_Step` takes its step pointer non-const. The same decided
  several vector copies in `btlv_mcss.c` and `btlv_effvm.c`: field-by-field copies from non-const sources.
- Clearing one bit with `flags &= ~(1 << i)` gives `mvns`; the original's `eors` with -1 is `flags &= (1 << i) ^ 0xffffffff`,
  as `btlv_clact.c`'s move and scale task ends do, and `btlv_stage.c`'s vanish flags use `(1 << side) ^ 3`.
- One swap temporary declared first and shared by two swaps gets other registers than one in each swap's block:
  `clact.c`'s `func_0204ca1c`. A pointer local to a copy loop's source array, `const u16 *name = src->nickname;`,
  moves which values get registers around the loop: `fld_btl_inst_tool.c`'s `genSubwayBtlInstitutePoke`, which
  also passes a call's result to a setter through a `u8 pp` local rather than a `(u8)` cast.
- Compound assignments add in another order than one expression: `mystery_net.c`'s `Mystery_ParseHex` adds the
  shifted value first, as the original does, only as `value <<= 4; value += digit;`, not as
  `value = value * 16 + digit`.
- An array element's address kept as `base + i * size` and read as `[base, offset]`, where ours keeps `&p[i]`, comes
  from a pointer to `const`: `pdw_postman.c`'s `FindMysteryGiftDeliveryManNPCID` and `scrcmd_medal.c`'s
  `GetMrMedalActorUID` declare `const ZoneNPC *npcs;`.
- In a comma `for` initializer, the order of a struct copy and a counter picks the copy's registers:
  `field_actor_tool.c`'s `CheckBlockedCollPathToPosition` copies with `r2` and `r3` only as
  `for (i = 0, tilePosition = rowPosition; ...)`.
- Independent stores are scheduled freely, so the asm's store order doesn't show the source's. A function that only
  fills a struct, with only a constant's register different, can need any order of its assignments: `btl_main.c`'s
  `BtlMainSeq_Set` inline matches only as `func`, `nextFunc`, `mainModule`, `state`.
- A parameter the original copies to a saved register at entry and changes there in place is walked by the source
  itself, not copied to a local: `arrow.c`'s `Arrow_SetPathCore` steps `startX -= PIECE_SIZE` rather than an
  `x = startX - PIECE_SIZE`.
- A field that a condition tests and a call's argument uses again after another call stays in a saved register;
  the original reloads it when the other call's result has a block-scoped local of its own:
  `report_event.c`'s `EventSave_Update` writes `FieldSubscreen *subscreen = Field_GetSubscreen(work->field);` before
  passing `work->screenId`.
- Saved registers swapped around an indirect call, with `r0` still holding a value just stored, mean the call passes
  an argument: `net_state.c`'s `func_020411fc` calls the network's end callback as `callback(NULL)`, so the callback
  type takes a `void *`. Parameters spilled at entry in the wrong order can be narrower than written:
  `net_command.c`'s `func_02040f84` takes a `u8` and a `u16`, which its caller in overlay 70 narrows.
- A pointer that follows another (`cr = cb + n`) adds its operands the other way round when written from the base,
  `cr = y + n + n`, which reuses `y + n` and adds the scaled count first: `ssp_jpegenc.c`'s
  `JpegEnc_ConvertYUV422`. A product the original computes by doubling one factor in place (`lsls rW, rW, #1`
  before `muls`) is written with the shift, `(width << 1) * height`, as `camera_system.c`'s `CameraSystem_StartDma`.
- A variable initialized at its declaration before a local array with an initializer stays live across the
  initializer's `ldm`/`stm` copy, which pushes the copy's pointer to a higher saved register:
  `pokemontrade_message.c`'s `func_ov194_021c0684` copies with `r6`, as the original does, only as
  `int i = 0; u32 params[] = { ... }; for (; i < 5; i++)`.
- The operands of an add or compare come in another order when one of them is an unnamed temporary: MWCC puts a
  register local first, whatever the source order. Give the other operand an `int` variable of its own.
  `btl_string.c`'s `func_ov167_021d5440` adds the variant second only as `base = message + stat * 3;` then
  `base + variant` (a `u16` `base` or a cast doesn't do it), and `btl_handler_work.c`'s `PopWork` compares
  `cmp end, pos` only with `start = work - base;` and `end = start + size;` in variables.
- A leaf function that builds a constant in `r0` and an address in `r1`, where ours swaps them, returns that
  constant: a `void` function puts the address in `r0`, whose parameter is dead. The multiboot parent's
  `OnWirelessDone` callbacks match as `BOOL` functions that set their flag and `return TRUE;`, though the wireless
  helper ignores the result.
Same instructions, registers swapped.

- Register allocation follows the declaration order of locals, so try reordering declarations when registers are
  swapped.
- An array index kept scaled (`i * 4` in a callee-saved register) across a call, where ours recomputes it, can come from
  an accessor whose index parameter has another type than the caller's counter. `btl_pokeparam.c`'s
  `CopyBatonPassParams` and `func_ov167_021bb864` match only with their condition accesses through a static inline
  `GetConditionPtr(mon, BattleConditionID id)`; the same accessor with a `u32 id`, casts and an enum counter don't.
- How a store is written can move the parameters' registers too: `*result = *partyResult != 0 ? 3 : 0;` swapped two
  pointer parameters' registers in `scrcmd_fld_battle.c`'s `func_ov036_021aec28`, where the same store as an
  `if`/`else` matched.
- The registers follow the declarations, but the order the constants are set follows the statements: when the
  declaration order that gives the right registers sets them in the wrong order, or derives one constant from the
  other (`movs r6, #0` ... `subs r4, r6, #1` for `-1`), declare the locals without initializers and assign them in the
  original's order. The phrase select's `PMSSelect_SeqSelect` needs `BOOL end; int touch;` then `touch = -1;
  end = FALSE;`.
- Of two variables that compete for the same register, the one used more gets it: one use of the Battle Subway
  command's result variable too many gave its register to the command ID. A single `*var = cond ? a : b;` counts as
  one use where an `if`/`else` with a store in each counts as two. Measured on small functions, the count is of
  instructions that refer to the variable once the code is cleaned up: its assignment, a parameter's move out of
  its argument register, and the shifts that narrow a `u8` or `u16` assigned from a wider value all count, while a
  use the optimizer deletes does not. A use inside an `if` or a switch case counts like any other. A variable a
  switch tests counts about 2 for each comparison and 4 for a jump table. On a tie the variable assigned first wins,
  whatever the declaration order, and a variable assigned later needs about one use more. That held in small
  functions; in `status_rcv.c`'s `StatusRcv_CanUseItem`, six effort values with the same uses, assigned one after
  another, went to `r5` and `r7` by declaration order instead: the last declared got `r5` and the one before it `r7`,
  and the rest took stack slots, the first declared highest.
- A variable gets a register or stack slot for each group of assignments that reach the same uses, so a variable
  that is assigned in two branches and stored once after them stays in one register, while a copy of the store in
  each branch lets the two assignments go to different places.
- Where a flag is first set changes which register its zero is built in. When the original builds a flag's `FALSE` in
  the register of a call's argument, or copies it from another zero, the flag was set after the call or loop, as
  `value = joinAveTextHandler(...); found = FALSE;` and a loop's total followed by `any = FALSE;` in the Join Avenue's
  records command. A zero loaded from another variable's stack slot is that variable: the summary screen's
  `PStaInfo_PrintMemo` sets its highest IV with `ldr r7, [sp, #0x14]`, `best`'s slot, which matches only as
  `best = 0;` between two calls well before the loop and `u8 maxIV = best;`; `maxIV = 0` makes it 18 bytes longer.
  The same goes for a zeroed struct a loop passes by value, which MWCC builds once in the loop's preheader by copying
  a variable in scope that holds 0: `btl_server_flow.c`'s `func_ov167_021a49c4` copies
  `BtlFlowDamageFlags flags = { 0 };` from the loop counter, as the original does, only with `flags` declared in a
  block around the loop; at function scope it copies `damage`. Stores of 0 just before a loop use the zero of the
  variable the loop's initializer sets first: `btl_net.c`'s `func_ov167_021b9950` clears `serverCmdReceived` and
  `unkE` with `i`'s zero, and keeps its own for `count` in `r5`, only as `for (i = 0, count = 0; i < 4; i++)`; with
  `count = 0;` before `for (i = 0; ...)` the stores take `count`'s.
- Moving code into a `static inline` helper can swap two registers when the instructions stay the same. In the same
  function, a loop counter and a copy of 0 traded `r1` and `r2` in its two closing loops until the loops and the
  stores after them became `ClearRecvBuffers()`, the setup counterpart of `func_ov167_021ba26c`, which frees those
  buffers in the same order.
- `arr[count++] = x` and `arr[count] = x; count++;` allocate registers differently, as do `count = 1; arr[0] = x;` and
  the reverse order.
- `a[i + c]` adds `c` to `i` first, while `(a + i)[c]` folds `c * 4` into the base offset. When the original folds a
  constant index offset outside a loop, the array it indexes probably starts at `a + c` in the source.
- An array index that is a sum, `a[i * 2 + x]`, is split into `(a + x) + i * 2`. When the original adds first and
  then indexes with the sum, the sum was put in a variable, as `seat` in `wbt_system.c`'s bracket code. Written as
  `seat = i * 2 + (won ? 0 : 1);`, the sum goes to the register of `i * 2`; with the `0` or `1` set by an `if` into a
  variable, it goes to that variable's register.
- A field read where nothing between its uses can change it, as in a stretch without calls or stores, is loaded once
  into a value MWCC allocates after the declared locals. A local copy is allocated with the locals instead and moves
  every spill slot: the PC box's `Box2Main_RangePutCheck` reads `syswk->pos` directly in its party branch.
- A spilled copy of a narrow value takes its slot by its type: `Box2Main_VFuncItemArrangeGetTouch` keeps a `u16` drop
  position in a `u16` local, which a `u32` local moved to the lowest slot.
- A constant built from another follows the store order: with `bob = FX32_ONE;` written before
  `bobSpeed = FX32_CONST(0.25);`, MWCC builds 0x1000 and gets 0x400 from it with `lsrs #2`; in the other order it
  builds 0x400 and shifts it left. `fldeff_namipoke.c`'s surfing Pokémon sets its bobbing so.
- A call's result passed straight to a narrower parameter is copied to the argument register and narrowed there (`adds
  r1, r0, #0; lsls r1, r1, #0x18; lsrs r1, r1, #0x18`). Narrowed in `r0` and shifted into the argument register (`lsls
  r0, r0, #0x18; lsrs r1, r0, #0x18`), it went through a `u8` or `u16` local first, as `u8 targetId =
  BattleEventVar_GetValue(4);` does in the move handlers for Foul Play and Captivate, and `u16 item =
  GetBattleMonHeldItem(mon);` in `handler_common.c`.
- A function whose callers halve or compare its result signed returns a signed type even if what it returns is unsigned:
  `RawBattleMonStat` returns `s32`, so `(RawBattleMonStat(a, 8) + RawBattleMonStat(b, 8)) / 2` divides with `asr` and
  its sign fix in Power Split and Guard Split, while its own code is the same as with `u32`.
- The operands of `*` are loaded in source order, so a multiply whose registers are swapped has its operands swapped
  in the source.
- The terms of a three-term `|` chain are not loaded in source order: `a | b | c` loads `c`, then `a`, then `b`.
  `btl_server_cmd.c`'s command encoder loads `args[2]`, `args[1]`, `args[0]` for each packed argument, which
  `((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f)` gives, while its four- and five-term chains
  match in field order. Try swapping the first two terms when the loads of a packing come out swapped.
- A product assigned to a variable of its own goes to a new register, with its operand copied there first
  (`mov r2, r1; mul r2, r0`), while a product used in place multiplies into the operand's register. The Join Avenue
  shop's arrow is placed with `row = ...; y = row * rowHeight; pos.y = y + 22;`.
- Two stores of the same constant share a register when chained, and not when written as two initializers.
  `second = first = TRUE;` stores `first` first. `nearXZ = FALSE; nearY = FALSE;` in that order loads the zero twice,
  while the other order shares it.
- A pointer local to an element of a struct, like `dst = &shot->pokes[i]`, takes a callee-saved register of its own
  and can push the struct's pointer to the stack along with a constant MWCC keeps for it. The musical's photo
  (`musical_event.c`'s `func_ov012_02151384`) only matches with `shot->pokes[pos].field` written at each use.
  The base register tells the two apart: `&call->rows[i]` is `call + 0x40 + i * 0x1c`, used with the field's own
  offset (`[r5, #0x14]`), while `call->rows[i].window` written out is `call + i * 0x1c` with the array's offset
  folded in (`[r5, #0x54]`), as in `ctvt_call.c`'s `CtvtCall_Leave` and `CtvtCall_Main`.
- An element address computed before an inline helper's argument calls, and kept in a register while the loop counter
  spills, is the address passed to the helper: `CtvtCall_CreateActor(sys, &call->rows[i].frame, ...)` stores through
  a `ClActor **`, where `call->rows[i].frame = CtvtCall_CreateActor(...)` computes the address after the call.
- Two loops that reuse one counter and both spill it share its stack slots in the order MWCC splits the variable,
  not in declaration order; giving the second loop a counter of its own, as `pos` in the same function, moves it.
- Variables declared in an inner block are allocated apart from the function's variables of the same name: in
  `ShinkaDemoPieces_Move`, the branch that moves a piece home declares its own `dx` and `dz`, which live on the stack
  while the other branches keep theirs in registers.
- Two values that the original keeps in one register, one dead before the other is set, were one variable: the PC
  box's `func_ov255_021cdcc8` keeps the party position and then its result in the same variable, where two
  variables get two registers.
- A loop condition written with a local for its row start, `start = pos + j * 6; if (x >= start && x < w + start)`,
  allocates registers differently from the same sums written in both comparisons: the PC box's `func_ov255_021d229c`
  only matched with the sums written out.
- A parameter that the callers narrow with shifts before the call is a `u16` or `u8`, and the type also decides how it
  is spilled: `BagItemList_GetItem` spills `pocket` first and compares the reloaded copy only once `pocket` and `index`
  are `u16`, as `itemmenu.c`'s caller narrows them; as `u32` it compares a register copy and stores it after.
- Loops over an array of structs that test two fields through a pointer to the element, then write the fields as
  `list->entries[i].x` in the body, keep the array's base in a register and the element's address in another, as
  `bag_item.c`'s `BagItemList_Remove` and `BagItemList_GetItem` do; indexing in the test too folds the field offsets.
- A pointer to an array slot can give a loop's variables back the original's registers. `btlv_scu.c`'s
  `func_ov167_021d2820` stores a call's result to `work->mons[i]`, tests it for `NULL` and passes it on twice; written
  through `BattleMon **slot = &work->mons[i];`, assigned just before the store, the work, `&balls` and `i` go to `r4`,
  `r5` and `r6` and `i * 4` is reloaded from its slot, as in the original. Indexing at each use swaps `r4` and `r6`,
  and assigning `slot` at the top of the loop body leaves 16 bytes off. Its sibling `func_ov167_021d2b88`, the same
  loop without the `NULL` test, matches with plain indexing.

## Stack slots

- Taking a parameter's address makes MWCC push all four argument registers to give it a home; an original that pushes
  fewer copied the parameter to a local first and passed that local's address: `g2d_Font.c`'s
  `NNSi_G2dFontGetStringWidth` writes `const void *pos = str;` and hands `&pos` to the character splitter (2.0/sp2p2).
- Local initializers declared in separate blocks are each copied to the stack just before their call, as the four BG
  setups of `b_plist_main.c`'s `BPlistMain_InitBG`; one block copies them all at the top.
- Which of several locals of equal use MWCC spills follows where the group is declared among the function's other
  locals, not their order among themselves; and a spilled local assigned last in both branches of an if/else has its
  store merged into the join. `worldtrade.c`'s `WorldTrade_InitCellActor` declares `plttBuf` and then the four file
  IDs in reverse before its other locals, and assigns the spilled `animFile` second in each branch.
Same code, other `sp` offsets or frame size.

- A pointer to an array element written to a local, `font = &app->fontOam[i]; font->bitmap = ...; font->oam = ...`,
  keeps the array's address in a register and spills the element's offset, where `app->fontOam[i].bitmap` folds the
  offsets into each access: the PC box's `func_ov255_021d1c30` matched only with the pointer.
- Declaration order does move spill slots in longer functions: `func_ov255_021d0374` matched with its loop counters
  declared first and `y` before the row width.
- Stack locals are laid out in reverse declaration order.
  When values must be read in one order (a script's arguments, say) but the original's slots follow another, declare
  the locals without initializers in the slot order and assign them in the read order: `script_command.c`'s
  `StaScriptCmd_PokeMove` matched with `mask; frames; wait; pokeSys; x; y; z;` declared and the arguments read as
  `mask, frames, x, y, z, wait`, and `StaScriptCmd_LightFollow` with the three system pointers declared before the
  offsets they are read after.
- A local initializer inside a loop is copied from `.rodata` once, before the loop, into a compiler temporary at the
  bottom of the frame, and copied from there into the local on each pass. `mus_shot_photo.c`'s
  `MusShotPhoto_InitPokes` declares `VecFx32 offset = { 0, FX32_CONST(-35), 0 };` in the branch for the top Pokémon,
  which gives the original's two copies (`sp+0x14` before the loop, `sp+0x20` in the branch).
- Spilled variables get their stack slots in the order they are first assigned, the first at the lowest address,
  whatever their declaration order or use counts, in small functions. A value that sits above values assigned after
  it was spilled in a later round of register allocation. In the Join Avenue's records command, a large switch, the
  declarations counted too: one group of slots followed the declaration order in reverse, and the variables declared
  in a loop body took the highest slots, in their declaration order.
- A variable reused by several switch cases is split into one value per case (see Registers), and a split piece that is
  spilled takes the lowest slot whatever the declarations say. When the original has a case's spilled value among the
  declared variables' slots, that case had a variable of its own, as case 9 of the records command does.
- A loop counter stored to the slot of a local whose address is passed elsewhere is that local reused: in the PC box's
  range pick, `func_ov255_021c445c`, the row loop counts with `y`, the touch position's `&y`, so the counter lives in
  `y`'s slot and the frame has no slot of its own for it. Pairs such as `width, height` that the original keeps in two
  sets of slots are block locals in two blocks; declaring them once at the top shares the slots and shrinks the frame.
  In `PokeIconMoveDataMake` the slots only matched once each reused variable was made block local and the loops used the
  original's counters; reordering the declarations did nothing.
- The same holds for a variable assigned anew in several `if` blocks: its first piece takes the variable's slot in
  declaration order and the later pieces the lowest slots, in the order they are assigned. `status_rcv.c`'s
  `StatusRcv_UseItem` reuses one function-level `add` in its six effort blocks, which puts the first block's value
  among the declared slots and the other five at the bottom of the frame; a block-scoped `add` in each block does not.
- In a chained assignment, `a = b = f();`, MWCC treats `a` as its own copy of `b`, and a spilled `a` takes the lowest
  slot instead of its declared one. `StatusRcv_UseItem` needs `status = f(); newStatus = status;`.
- Named locals take stack slots apart from the compiler's temporaries. When the original's spilled values all sit in
  the order they are first assigned, they may be common subexpressions: the trade's `func_ov194_021c1530` reads
  `colors[side * 2]` at each use, and `int color = colors[side * 2];` moved it above the temporaries.
- One counter for two loops in a row keeps one slot. `func_ov194_021c12ec` has a loop over `i`, then two nested loops.
  MWCC keeps the `0` that the first loop passes as arguments in the stack slot of the variable it starts the outer
  nested loop with, and in the original that slot is the lowest: both loops count with `i`, the inner one with `j`.
  A separate `side` for the outer loop took the highest slot instead.
- A NULL check written on a field, `if (bgs[bg].screen != NULL) { void *screen = bgs[bg].screen; ... }`, gives
  different stack slots from the same check on a local loaded before it, as `GFL_BGSysLoadScrCore` shows.
- MWCC reuses a field it has loaded, across the 64-bit multiply helpers, so a value that the original keeps on the
  stack between `piece->home.x - piece->pos.x` and `piece->pos.x = piece->home.x` is MWCC's own copy, not a local.
  Writing it as a local changes which stack slots everything gets.
- A test of a field right after a store into it reuses the stored register only when the source tests the field
  written to: `tr_tool.c`'s `TrainerUtil_LoadTrainer` tests `trainer->trainerClass` after storing `data->trainerClass`
  there, not `data->trainerClass`.
- The types of locals and of the values they hold change how spilled values are scheduled. The trainer AI's speed
  comparison only matches with the speed function returning `u16` into `u16` locals: a spilled `u16` is reloaded after
  the call's stack argument is stored, while a spilled `u32` is reloaded before it.
  A spilled loop bound is the same: `AddExpAndEVs` in `btl_server_flow_sub.c` stores the spilled counter before
  loading its bound, the number of mons, only with the bound a `u16`. And `func_ov167_021afd90` matched only with
  `numMoves` a `u32` and its locals in one order of 720; no order matched with it a `u8` or `u16`, so sweep the orders
  again after widening a local.
- A hardware address that the original builds with shifts and keeps on the stack, where ours loads it from the
  literal pool, comes from an SDK inline that returns it: the ribbon page's `PStaRibbon_CreateActors` builds
  `0x19 << 22` and `2 << 16` apart, and matches as `(u8 *)G2_GetOBJCharPtr() + 0x20000` but not as
  `(u8 *)HW_OBJ_VRAM + 0x20000` in a local.
- A `u64` argument whose high word is 0 keeps that zero in a stack slot of its own, where a `u32` zero is folded into
  a constant. Two such slots in `mystery_gift_pokemon.c` show that `PokeParty_CreatePkm` takes its trainer ID and PID
  as `u64`s.
- A `u64` parameter after three `u32`s is split between `r3` and the first stack word, with no alignment to a register
  pair. Arguments passed in pairs such as `(x, 0)` or `(0, -1)`, a constant -1 stored last, and a callee that saves
  `r3` and its first stack argument to adjacent slots are the signs, as `tr_tool.c`'s `TrainerUtil_LoadParty` shows
  for `PokeParty_CreatePkm`.
- Structs passed by value go in registers and on the stack. Code that copies a struct to the stack and passes its
  address takes a pointer to a local copy. A struct local keeps its stack slot even when it only passes through, so a
  frame larger than the locals explain holds one: Guard Spec.'s effect in `btl_server_flow_sub.c` stores
  `SetConditionTurns`'s `BattleCondition` in a local before passing it on, and `BattleHandler_AddSideEffect` keeps its
  copy's address in a register to pass the copy by value after passing its address.

- A function that declares every local first and assigns them below gets other registers and stack slots than one
  that initializes them in their declarations (palanm.c's `MaskPalettes` sets `mask = 0` after the count for its
  register order): bmp_menulist.c's `Bitmap_Scroll16` declares `pixels, widthTiles,
  fill32, end, y, i, j, src, dst` and assigns `pixels`, `fill32`, `widthTiles` and `end` in call order, and swapping
  the declarations of `widthTiles` and `fill32` swaps their slots.
- A stack parameter loaded at entry although it is an `int` was reassigned rather than copied: wipe.c's
  `GFL_WipeSet` does `sync /= GFL_FadeGetUpdateFreq()`, where a new local leaves the load at the division. A pointer
  local such as `sys = &sWipe` assigned after that statement, not in its declaration, keeps its register free until
  then.
- A `u16` local and `local + 1` stored back to the same field can share a register and push a parameter out of r0;
  a wider local keeps them apart, as `u32 listTop` does in `BmpMenuList_Scroll`.

- A value that reloads its source for each use, where ours loads it once into a register (or the reverse), was
  written as an expression that re-reads the source in each part, as a macro writes it: palanm.c's `BlendFadeColors`
  matches with `BLEND_CHANNEL(src[i] & 0x1f, ...)` for each channel, not with channel locals.
- A field addressed from the struct's base plus its full offset, where ours goes through a pointer to the member, was
  reached through the member path: `paletteBuffer->fade.delayCounter` in `PaletteFade_StepBuffer`, not a
  `FadeControl *` local.

- A field loaded earlier than its first use in the source was read into a local initialized in its declaration, in
  declaration order (`s16 brightness = data->brightness;` before `target` in `BrightnessData_Step`).
- A constant store written first reserves its register before the parameters are moved, though the scheduler moves
  the store itself later: `data->active = TRUE;` first in `BrightnessData_Init` keeps the shared 1 in r0.
- The scheduler moves constant loads (`movs r3, #0`) up ahead of loads and stores written before them, so constants
  loaded earlier in the original than in ours are assigned later in the C. `p_sta_sub.c`'s `PStaSub_TrackGestures`
  clears its direction and lap flag after computing the circle's `dx` and `dy`, which puts the clears before the
  swipe's frame count store and loads the circle's start x and y one at a time, as the original does.

- Block-scoped arrays set both the stack order and where their initializers are copied: infowin.c's
  `InfoWin_VBlankTask` matches only with each table declared in the `if` block that uses it.
- A local pointer to a struct member, `PrintWindow *window = &work->priceWindow;`, is kept as the member's offset in a
  callee-saved register, added to the struct's base at each use, and where the pointer is assigned decides when that
  register is loaded. When the original loads a member's offset into `r6` or `r7` early and indexes from it, the
  source had such a pointer: the bag's `ItemMenuDisp_DrawQuantity`, `ItemMenuDisp_ShowMessage` and
  `ItemMenuDisp_DrawTMInfo` only match with one, and it also stopped MWCC from holding a zero for the stack arguments.

## Instruction order

- In `call() != x`, MWCC compares the call's result second (`cmp r1, r0`). The result first (`cmp r0, r1`) is the
  result assigned to a local as its own statement and then compared: `snd_arc.c`'s `SetupArc` matches with
  `readSize = romfs_fread(...); if (readSize != arc->header.infoSize)`, where every cast on either side kept the order.
- An argument of a call compared again unchanged after it is kept in a stack slot over the call; the original working
  it out again means its comparison was another expression. `snd_arc_loader.c`'s `LoadWaveArcTable` reads
  `sizeof(SNDWaveArc) + tableSize` bytes and matches only comparing `result != (int)sizeof(SNDWaveArc) + tableSize`,
  signed where the argument is unsigned; without the cast it is 4 bytes longer.
- On `2.0/sp2p2`, an add written through a small inline with parameters keeps their order where the same expression
  written out is swapped: `snd_stream.c`'s `StrmCallback` gets `adds r1, r1, r2` only with
  `AddU32ToPtr(buffer, offset)`, `(void *)((u32)ptr + val)`, while the cast-add written in place gives the right
  registers but `adds r1, r2, r1`, and `(u8 *)buffer + offset` other registers.
- A sum of `s16` table entries matches read through an inline returning `s16`, where the table indexed in place or an
  `int` inline gives other registers and operand order in every order of the terms: `snd_arc_stream.c`'s
  `NNSi_SndArcStrmMain` adds `CalcDecibel(fader) + CalcDecibel(player->volume) + CalcDecibel(player->userVolume)`.
- A sum that MWCC reorders, loading a field first and putting it on the left whatever order it is written in, keeps
  its order when the field is added in a statement of its own: `ProcessCommand`'s `fileOffset += dataOffset` was 4
  bytes shorter than the one expression.
- A statement between a call and an SDK inline that uses its result is scheduled inside the inline's code only when
  the result has its own variable: `ctvt_game.c`'s `CtvtGame_InitResults` keeps the texture key in a local, so
  `picture = 0` lands between the inline's shifts, as in the original.
- `x = a; x -= b;` loads `a` first, where `x = a - b` loads `b` first: `arc_tool.c`'s `GFL_ArcSysInitArcHandle` reads
  the file's end before its start as `size = end; size -= start;`.
- The stores of a struct-filling function can come out in the reverse of the source's order: `tcb.c`'s
  `GFL_TCBMgrCreate` stores its computed pointers last only when the source sets them first. Sweep the orders.
- A pointer field read twice through a struct is reloaded after a store between; a block-local copy is read once
  before the first field read: `btl_main.c`'s `func_ov167_0219ada0` writes
  `BtlSetup *setup = mainModule->setup;` before its two stores.
- The order of unrelated statements in a branch decides when a value is loaded: `br_sidebar.c`'s
  `BrSidebarWork_Move_Close` loads the scale step before storing the count only with `cnt++` after
  `scale += dscale`.
- `const` can be wrong as well as missing: through a `const` pointer MWCC loads a call's field arguments before its
  constant ones. `musical_mcss.c`'s `MusicalMcss_Load` builds the `compressed` argument first, as the original does,
  only with its `info` not `const`. And a `const` parameter is what counts: a `const` local copy of a plain pointer
  parameter doesn't schedule like it, so the `NetCommand` callbacks take `const void *data`, which matched four of
  them; MWCC won't convert between function pointer types that differ only in a parameter's `const`.
- A spilled value reloaded before a call's result is stored, where the original stores the result first, can need
  the result in a local and the stores in field order: `key_system_util.c`'s `KeySystemAccelMove_Init` stores
  `speed` and then `accel = FX_Div(...)`.
- A counter zeroed at its declaration before a local array with an initializer is zeroed before the array's address
  is set up; `for (i = 0; ...)` after the array zeroes it after: `event_ircbattle.c`'s `func_ov012_02150588`.
- A constant stored to a field that the original builds one store earlier is stored first in the source: MWCC
  keeps the stores in their order but builds the constant sooner. `ctvt_draw.c`'s `CtvtDraw_Main` writes
  `draw->exit = TRUE; draw->state = DRAW_STATE_FADE_OUT;`.
- MWCC keeps a load and a store through pointers that may alias in source order. A field the original loads before
  a store is read into a local in a statement before it: `g3d_system.c`'s `GFL_G3DAnmCreate` reads
  `resource->data` before storing `resource` in the animation, and `event_field_proclink.c`'s `func_ov012_0215bb70`
  loads the party and the trainer data into locals before storing either, where reading a stored field back for a
  call gives load, store, load, store.
- A constant assigned to a spilled local is stored where MWCC likes, but the register that builds it follows where
  the source assigns it: `ctvt_game.c`'s `CtvtGamePlayer_Create` builds `isSelf`'s `TRUE` in `r2`, as the original
  does, only assigned right after the allocation, not at its declaration.
Same instructions, scheduled in another order.

- `x + (p << 12)` and `x + p * 0x1000` put the operands of `adds` in opposite orders: the phrase select's
  `PMSSelect_BGDrawPlate` sets a screen entry's palette with `(entries[i] & 0xfff) + (palette + 2) * 0x1000`, which
  gives `adds r5, r5, r2`, where `<< 12` gave `adds r5, r2, r5`.
- Loads through a pointer are not moved above stores unless the pointee is `const`. A load that the original
  schedules early, such as an argument loaded before the stack arguments are stored, points to a `const` parameter.
  It has to be the parameter: `fld_scenearea_loader.c`'s camera-area callbacks scheduled their area's loads only
  once the callback typedefs took `const CameraArea *`, and a `const` local pointer to the member did nothing. The
  same change fixed the register allocation of the loop in `fld_scenearea.c` that calls them. Since `const` shows in
  the code, a caller and its callee can disagree, and then the call casts: `btl_server_flow.c`'s
  `func_ov167_021a6c34` matches only with a `const` param and `func_ov167_021a6914`, which it passes it to, only
  without, so it passes `(BtlFlowMoveParam *)param`. Before casting, check that dropping `const` along the caller's
  chain doesn't match as well: `battle_rec_tool.c`'s party loaders took a non-`const` record with no change.
- A dispatcher switch whose cases each end in their own `pop`, with `movs r0, #0` before the jump table, is a result
  local set to 0 after the last call before the switch, `case X: command = f(...); break;` and one `return command;`.
  `return f(...)` in each case with a final `return 0;` is 2 bytes longer and zeroes at the end:
  `field_player_grid.c`'s `FieldPlayerGrid_DecideCommandBike`.
- The order of a switch's cases in the source is the order of their bodies, while the compares stay sorted by value:
  `FieldPlayerGrid_UpdateNormalMove` compares 0xc, 0x10, 0x58 and lays the bodies out 0x10, 0x58, 0xc.
- Two separate `if`s that return the same constant share one return, and share the constant with a mask of the same
  value: `if (!(flags & 2)) return 2; if (force == TRUE) return 2;` gives `movs r0, #2; tst r1, r0; beq` in
  `FieldPlayerGrid_DecideCommandNormalMoving`, where the `||` form returned twice.
- Elements of a `static const` array passed to an inline are loaded at each store of the inline's body, in the
  body's order. Read through a function-scope pointer to the table, they are evaluated as arguments, right to left,
  before any store: `FieldChunkAccessor_GetTerrainCore` declares `const fx16 *slopes = MAP_HEIGHT_SLOPE_TABLE;` at
  the top and passes `slopes[i], slopes[i + 1], -slopes[i + 2]` to `VEC_Fx16Set`, which loads z, y, x and then
  stores x, y, z. The same pointer declared inside the branch folds back into the array.
- Which of two locals is spilled can follow the declaration order: the same function only keeps `offset` on the
  stack and the tile pointer in `r7` with `fx32 offset;` declared first.
- A bit field read through a `static inline` getter is not shared with a later read of the same word: its
  `u8 shape = MapHeightTile_GetShape(tile);` makes the plane branch load the halfword again for `tile->slope`,
  where a direct `tile->shape` reused the first load.
- The order of a product's operands picks the registers of what feeds it: the tile index
  `x / TILE_SIZE + (FX_Whole(size) / 16) * (z / TILE_SIZE)` matched in `FieldChunkAccessor_RD_GetTerrain` and
  swapped the registers of `x` and `z` with the factors the other way round.
- A local assigned once and used once is moved to its use when nothing between them writes memory, and the 64-bit
  multiply helper of `FX_Mul` doesn't count as a write. To keep a value computed where the original computes it,
  build it in steps: `RECT_PitchYawTZ` writes `pitch = rect.pitch2 - rect.pitch1; pitch = pitch * progress /
  FX32_ONE; pitch += rect.pitch1;`, where the one-expression form sank the pitch into its call.
- The same rule moves a call's stack argument stores. When loads through a pointer that is not `const` follow the
  call, the stack arguments are stored before the register arguments are set up. If the original stores them last,
  the pointer is `const`.
- A global's address that the original loads early, into a saved register before unrelated calls, and copies from
  later was taken into a pointer local where it is loaded: `fldeff_gyoe.c`'s draw loads `&NNS_G3dGlb.projMtx` before
  the float calls of its depth offset only as `const MtxFx44 *proj = NNS_G3dGlbGetProjectionMtx();` declared first
  and `saved = *proj;` after the offset. `saved = NNS_G3dGlb.projMtx;`, with or without the offset in a local first,
  loads the address after the calls into another register.
- A struct assignment loads every field before it stores any, through a pointer that is not `const` too. Two
  fields copied with both loads first, `ldr r2, [r0, #0x10]; ldr r1, [r0, #0x14]; str r2, [r0]`, are one struct
  copied: `KeySystemTween_Update` ends with `tween->pos = tween->end;` for an `{ s32 x, y; }` position.
- A load through a `const` pointer is also reused across stores, as the World Tournament's `wbt_setup.c` reads an
  entrant's bit fields from one load, but it is not hoisted out of a loop: `wbt_party.c`'s filter check reads each
  list's count again in every iteration because its filter is `const`, where a plain pointer's count is loaded once
  before the loop.
- A field load scheduled ahead of a store it doesn't depend on, where MWCC keeps the written order, can come from an
  inline helper's argument: arguments are evaluated before the body's stores. The PC box's sequences set the picked
  position with `u8 pos = func_0202ba60(...); Box2Seq_SetGetPos(syswk, pos, syswk->tray);`, which loads `tray` before
  storing `pos`; the same stores written out load it after.
- Initializations are scheduled where they are written: `int i = 0;` declared after a call sets `i` after the call,
  while `for (i = 0; ...)` sets it at the loop, after any statements before the loop.
- Two loop variables zeroed in the other order: zero both in the `for` initializer in the original's order.
  `scrcmd_medal.c`'s `GetHintableMedalCount` zeroes `i` before `count` only as `for (i = 0, count = 0; ...)`; `count`
  initialized at its declaration, or declared first, zeroes it first.
  `event_battle.c`'s `EventBattleCall_Callback` builds its count of 0x25 before its checksum's zero only as
  `for (shift = 0x25, checksum = 0; ...)`.
- A counter's zero stored to its stack slot ahead of a call, in another register than the call's arguments, can be
  written after that call: overlay 185's `CountupGreetings` stores `n`'s zero before calling
  `PMSWord_GetWordNumByGmmId` only with `n = 0;` after `first` and `last` are computed. Written before the call, it is
  stored after the call's result and built in `r0`; the next function, `CountupPokemon`, sets it right after its call.
- A field of a local struct that a call fills is loaded before the next call only when the source reads it there: the
  Pokédex forms page copies `targetX = target.x;` between `ZukanDetailForm_GetSpritePosF32(..., &target)` and
  `MCSS_GetPosition`.
- A call nested in another call's arguments is made after the other arguments' addresses are computed. When the
  original makes the inner call first, its result was put in a local: the summary screen's ribbon list writes
  `y = PStaRibbon_GetRowY(ribbon, i); PStaOam_SetPosition(ribbon->rows[i].oam, ROW_X, y);`.
- A conditional expression among a call's arguments is evaluated before the plain ones. When the original loads the
  arguments in their order, the conditional one was a local set before the call: the forms page passes `addToDex`
  locals `sex` and `rare` set just before it.
- An argument that is loaded before a call among the arguments, such as a print queue loaded before
  `BmpWin_GetBitmap(...)` in the same call, was passed to an inlined helper that makes the call, like
  `PrintWindow_Print`. A block-scoped local set from the field before the call does the same: bmp_menu.c's
  `BmpMenu_PrintOptions` loads the queue first because its loop body declares `PrintQueue *queue` and a `u8 y`.
  save_error.c's `displayLightBlueErrorWindow` reads `chars->size` before calling `gfxGetCharAddrBG1A()` only
  through NitroSDK's `MI_CpuCopy16` inline over `sys_memcpy16`. The same goes for a field read before a call that
  could change it, such as a `sys_memset` of the struct that holds it: pdwacc_disp.c reads the heap ID before
  clearing its palette cycle only as the argument of a `static inline` helper that clears the cycle and then uses the
  heap ID.
- Two stores through a pointer read from a struct, with one load of the pointer where ours loads it again after the
  first store, were made by an inlined helper that takes the pointer, such as `PrintWindow_Init(header.printWindow,
  window)` in `ShopUI_CreateConfirmDialog`.
  `PrintWindow_Print`.
- A computed argument whose arithmetic comes before the loads of the arguments before it was assigned to a variable
  first: the PC box's `Box2Main_VFuncPartyInPokeMove` adds 30 to the party's count before loading the cursor
  position only with `putPos = count + 30; PokeIconMoveDataMake(syswk, syswk->pos, putPos);`.
- A parameter passed on the stack is loaded at the function's entry, along with the register parameters, unless it is
  an `int` or `s32`, which is loaded where it is first used. `StartMenu_DrawFrame` takes its BG as a `u8`, as
  `GFL_BGSysFillScrArea` does.
  This isn't the whole rule: `fld_btl_inst_tool.c`'s `SetupTrialHouseBattle` loads its count at entry, as the
  original does, only as an `int`, the type its callees take, and at its uses as a `u32`. Whether the parameter's
  type matches its uses' seems to count too.
- A `u16` stack parameter reloaded with `ldrh` at some uses, with one `ldrh` into a register that feeds others, has
  few uses of its own: the shared load is its conversion to a wider type, so the callees there take a `u32`. In
  bmp_menu.c's `BmpMenu_AddEx`, `BmpCursor_Create` and `BmpCursor_LoadBitmap` narrow their heap ID with shifts, so they take
  `u32 heapId`. When reloads look like register allocation, read the callees' asm before reordering statements.
- NitroSDK's inline functions take enums, such as `GXBGColorMode`, and the BG system's `GFL_BGSysCreateBG` only loads
  every argument of `G2_SetBG0Control` before shifting any with enum parameters. `nitro/gx.h` keeps the SDK's types for
  this reason.
- Where `FX_Mul`'s sign extensions go depends on the statements. In `ShinkaDemoPieces_Init`, squaring `dy` before
  either square root, `distY = FX_Mul(dy, dy);`, keeps `dy`'s extension in a stack slot across the first one, and only
  the sum written as a `static inline` function, `distXZ = SquaredLengthXZ(dx, dz);`, puts that extension before the
  sum's multiplies. The same sum written in place, in any statement order, cast or split, does not.
- A plain argument loaded before another argument that is a call was in a local: mystery_util.c's
  `MysteryTextWinCopy_Apply` loads `copy->bitmaps[i]` before calling `BmpWin_GetBitmap` only with
  `GFLBitmap *bitmap = copy->bitmaps[i]; GFL_BitmapCopy(bitmap, BmpWin_GetBitmap(...));`, where the call written in
  place of the local is evaluated first.
- An argument for an inlined function that MWCC copies into its one use, where the original computes it at the
  inline's entry and spills it, was a local of the caller: `mystery.c`'s `MysteryEffect_Init` sets
  `tailHeapId = HEAPID_TAIL(heapId);` before calling the inlined gift-Pokémon routine, which reaches the original's size,
  where `HEAPID_TAIL(heapId)` written as the argument is 4 bytes short.
- Reads of a `const` table are not moved across stores to I/O registers, so a table read the original does before the
  stores of a NitroSDK register inline was written before it: `mystery_album.c`'s `MysteryCardView_SeqThrowAway`
  reads a card position's x and y into `int` locals before `G2_SetWnd0InsidePlane`.

## An instruction too many or too few

- On NitroSystem's `2.0/sp2p2`, an address passed straight to a static inline that reads `p[k]` folds `k` into the
  load's offset from the shared base, where the same address in a named local, or the element written `a[n + k]`,
  is computed again. `snd_arc_loader.c`'s `LoadSingleWave` reads a wave's file offset and the next one's as
  `ldr [r1, #0x3c]` and `ldr [r1, #0x40]` off one `waveArc + n * 4` only with
  `GetNextWaveOffset(&waveArc->offsetTable[n])`, an inline returning `fileOffset[1]`; `offsetTable[n + 1]` is 8 bytes
  longer, and a local pointer 4.
- A narrowing the original does where nothing calls for it can be an inline returning a wider type into a narrow
  local: `br_sidebar.c`'s moves set their `s8 dir` from `static inline int BrSidebar_GetDir(BOOL dir)`, which
  narrows right after the choice in one function and at the use in the other, as the original does.
- `&p[i].field` computes `p + i * size + offset`, and `&p->array[i].field` computes `(p + offset) + i * size` with
  an extra `mov`, so the order shows where an array starts: `event_bsubway.c`'s `func_ov012_0216657c` reads a
  leader's message at 0x18 of each 0x22-byte entry of an array at offset 0, not in entries of an array at 0x18.
Narrowing shifts, reloads, recomputed addresses and folded constants.

- A last call that passes four arguments in registers is a `bl` with a frame, not a tail call: MWCC's tail call
  loads the callee's address into `r3`, which the fourth argument holds. When the original saves a register and
  calls where the C tail-calls, the callee takes one argument more: `sta_acting.c`'s `StaActing_PlayWave` matched
  once `func_02006528` took the fourth argument its code reads, which the caller passes on from its own `r3`.

- `p->stack[p->num - 1]` with the array a direct member of `*p` subtracts 1 and loads from the array's offset
  (`subs; lsls; ldr [r0, #0x4c]`), while the same index into an array inside a nested struct folds the `- 1` into the
  offset (`lsls; ldr [r0, #0x48]`). The Battle Recorder's `BrProcSys_Pop` has the folded load, but its asserts name
  `p_wk->stack_num` as a member of the work itself, so it stays unmatched.

- `field += value` on an `s16` field with an `int` value narrows the value first and shares the narrowed copy between
  such adds, where `field = field + value` adds the `int` as it is. The phrase input's `PMSIVEdit_ScrollWait` adds its
  step to two scroll fields the second way.
- An argument narrowed by a `u16` parameter is narrowed again at each call, and only hoisted out of a loop, while a
  `(u16)` cast is computed once and reused: `PMSIVEdit_ScrollWait` matched only once `func_0204c1a8` and
  `func_0204c1dc` took their surface as `u16`.
- After `a = b;`, a test of `b` reuses the register just stored and a test of `a` loads `a` again. The phrase
  select's `PMSSelect_SetupList` writes `wk->lineCount = wk->sentenceCount; if (wk->sentenceCount < 20)`, and its
  `PMSSelect_SetupScreen` tests `wk->pos` after `wk->prevPos = wk->pos;`.
- `field--` and `field++` load the field again before the subtraction, even right after comparing it, where
  `field = field - 1` reuses the register: the phrase input's `PMSInput_SentenceKey` moves its edit position the
  second way.
- A compound assignment to a narrow field narrows its right side first: `work->checkFlag &= 0xff ^ (1 << waza);` on a
  `u8` shifts the mask down to a byte before the `and`, while `work->checkFlag = work->checkFlag & (0xff ^ (1 << waza));`
  ands the full mask, as the PC box's `Box2Main_PokeFreeWazaCheck` does.
- A call's result minus a constant, passed straight to another call, is copied to a scratch register while the
  constant is built in the argument register (`movs r2, #0xf; adds r3, r0, #0; lsls r2, r2, #0xc; subs r2, r3, r2`).
  With the difference in a local's initializer, `u32 heapSize = GetFieldmapZoneHeapSize(zone) - 0xf000;`, the
  constant gets a register of its own and the difference goes straight to the argument (`subs r2, r0, r1`), as in
  `fieldmap.c`'s `FieldmapProc_Init`. The result in a local and the subtraction in the argument don't do it.
- `res -= 34;` lets MWCC fold a later `res + 36` into `res + 2`; a variable of its own, `u32 slot = res - 34;`, keeps the
  difference in its register and adds 36 to it, as the PC box's `func_ov255_021c445c` does.
- A chained assignment to fields, `a->x = a->y = value;`, stores `y`, reloads it and stores `x`. When the original
  narrows the value once and stores it to both, the stores were separate statements, as in the PC box's
  `PokeIconChgDataMake`.
  The same holds with a local on the left: a call's result stored to a field and read back from it before a test,
  `str r0, [r7, r1]` then `ldr r0, [r7, r0]`, is `icon = wk->markIcons.icons[i] = f(...);`, as the trade summary's
  marking icons are made in `func_ov194_021c4ec0`.
- A caller narrows an argument for a `u8` or `u16` parameter with shifts before the call, so an argument passed without
  them is for a wider parameter. Read the narrowings of every caller together: `GFL_BitmapFillArea` takes `s16 x, s16
  y, u16 width, u16 height`, and `GFL_BitmapGetWidth` returns a `u16`, which is why printsys.c passes its width
  without shifts and bmp_menulist.c's `PrintOptions` narrows its computed width and height.
- MWCC trusts the type of a call's result: a `u8` returned by one function and passed on to a `u8` parameter isn't
  narrowed again. When the original narrows such a value before the call, it was held in an `int` or `u32` local, as
  `research_graph.c`'s `SetFirstAnswer` and `ChangeAnswer` keep a question's ID in an `int`.
- A `u8` function that narrows its result at the return (`lsl #24; lsr #24` after setting a 0/1 flag) keeps the flag
  in a `BOOL` local, as palanm.c's `IsBitSet` does.
- A signed compare (`bge`) of a parameter that callers pass as a `u8` without narrowing means the parameter is an
  `int`: palanm.c's `MaskPalettes(int buffer, ...)`.
- A signed compare (`bge`) of a `u32` function result means it was stored in an `int` local: `tr_tool.c`'s
  `TrainerMsg_Load` keeps `GFL_ArcSysGetDataLength`'s result in an `int` and compares it with an `int` trainer ID.
- A `u16` narrowing followed by an `s16` one (`lsl #16; lsr #16; lsl #16; asr #16`) is a value returned by a `u16`
  inline helper and passed to an `s16` parameter, as bmp_menulist.c's `RowY` is in `BmpMenuList_EraseCursor`.
- A 4-bit color field masked with `& 0x1f` before it is shifted into a print color (`lsl #27; lsr #17` for the text
  color) went through `PRINT_COLOR`, which masks each component.
  them is for a wider parameter. The other way round, a parameter passed on to a `u8` parameter without shifts is a
  `u8` itself: `GetBattleMon` hands its ID straight to `GetPokeParam`, so both take a `u8`, and so do the ability
  helpers that pass their mon's ID to `GetBattleMon`.
- A wider parameter stored into a narrow bit field is narrowed to the field's type and then shifted into place, while
  a `u8` parameter is trusted and shifted at once. The PC box's `func_ov255_021cc460` takes its button's actor and
  palette as `u32` and narrows them in the stores to its 7- and 4-bit fields.
- A sum that the original truncates to `s16` before comparing it was stored in an `s16` local, as the edges of the
  Join Avenue's balloons are; casting it in the comparison gives the same code but is not needed.
- A value narrowed again after it is clamped was clamped with a conditional expression, whose `int` result is
  narrowed when it is stored back: the Pokédex cry page writes `sample = MATH_CLAMP(sample, -500, 500);` for an `s16`
  sample, where an `if`/`else if` chain assigning the bounds leaves no narrowing.
- A comparison right after a `u16` narrowing that isn't done in `u16` arithmetic (`subs r0, r1, #6; cmp #1; bhi`
  rather than `subs; adds; lsl/lsr #16; cmp`) compares the narrowed value as an `int`: `zone_weather.c`'s
  `UpdateWeatherToDefault` keeps `int nowWeather = (u16)GetNowWeather(gameData);` for its `== 6 || == 7` test.
- On a `u16`, `x == A || x == A + 1` compiles to `adds x, #(u16)-A` (the negated constant from the literal pool,
  `0xff60` for 0xa0), `lsl/lsr #16; cmp #1; bhi`. `(u16)(x - A) <= 1` gives `subs` instead, and `x >= A && x <= A + 1`
  two compares. `fldeff_namipoke.c` tests the actor's object code so.
- A callee that narrows its result in its body (`lsl #24; lsr #24`) can still return `u32`, with the value in a `u8`
  local: the caller narrowing the result again shows it, as `event_mapchange.c` does for `season.c`'s
  `Season_GetRealTime`.
- MWCC keeps the grouping written in an address offset: `raw + (i * 0x180 + 0xc00)` and `raw + i * 0x180 + 0xc00`
  compile differently, and a bracketed `((personality & 0xf) - 8)` is kept as its own term where the bare `- 8` is
  folded into the constant beside it (`pokegra.c`'s `PokeGra_CellCharsToImage` and `PokeGra_DrawSpindaSpots`).
- An `if` whose condition is an assignment with `|=`, `if (texBanks |= GX_VRAM_C)`, keeps the `orr` and tests its
  result, which `|` would fold away: `screentex.c`'s bank switch, where the test is always true.
- Masks written with `~` clear bits with `bic`. The game's `and` with a constant such as `0xef` is `x &= (u8)~FLAG`.
- MWCC doesn't propagate constants into a variable of an enum type. A loop that still checks its bound before the
  first pass, as `for (p = 80; p <= 83; p++)` does in the Join Avenue's commands, or a sum that still adds a counter
  known to be 0, as the Battle Subway's loop over its music switches does, has an enum counter. Nor does it move an
  enum local's value into its one use: `btl_rec.c`'s `func_ov167_021d4674` computes a chunk's type from its header
  byte before the chapter bit, as written, only with `BtlRecChunkType type`; an `int type` used once is computed in
  the `if` that tests it, after the chapter bit. A constant loaded once into a callee-saved register (`movs r5, #6`)
  and copied to each use (`adds r0, r5, #0`) is such a local too, where an integer local gives a `movs #6` at each
  use: `move_handlers.c`'s `HandlerChatter` holds its confusion status in a `BattleConditionID`. When the values are
  `#define`s that asm includes share, as `CONDITION_*` are, a count-only enum in a C header gives the type.
- A result tested with no narrowing after the call, but narrowed after arithmetic on it (`(u8)(weather - 2) <= 2`),
  comes from a function that returns `u8`. `HandlerSolarBeamPower` matches only with `GetWeather` returning `u8`, and
  so do the getters it calls, down to the field's `u8` weather, for their own definitions to keep matching.
- A function that returns `-1` or `0` from two branches, `if (f(x)) { return 0; } return -1;`, is folded into a
  computed result (`rsbs`) when it returns an `int`, and keeps both returns when it returns an enum. The field action
  checks of `itemuse_event.c` return such an enum.
- A local variable that holds a constant, like `fx32 one = FX32_ONE;`, keeps its own stack slot or register, while the
  literal is hoisted out of a loop by the compiler. Extra hoisted constants in our output point to such a variable.
- An address computed before calls is reused after them only when the expression is the same, types included:
  `a[j].f` read before the calls lets a later `a[j].g` reuse the address, but not `a[(u32)j].g`. An address that the
  original computes again, where ours keeps it on the stack, is written differently at one of the two places. An
  inline accessor does it naturally, since its parameter is a copy of the index in the parameter's type: the Join
  Avenue shop reads `ResortShop_GetEntry(wk, j)->id` with a `u32` index for an `int` `j`, and its later
  `wk->entries[j]` is computed again while `wk->entries[i]` is reused.
- Stores to fixed addresses fold into one literal each, `((u16 *)(HW_DB_BG_PLTT + 0x1c0))[9]` included. A literal kept as
  a base with offsets, `ldr r1, =0x50005c0; strh r0, [r1, #0x12]`, is a pointer local: the summary screen's
  `PStatus_InitText` sets two font colors through `GXRgb *pltt = (GXRgb *)(HW_DB_BG_PLTT + 0x1c0);`.
- A loop that computes an element's offset (`i * size`) once into a register of its own, using it both for stores
  through the array and for `&arr[i]` passed to a call, took the element's address into a pointer at the top of the
  body and used the pointer only for the call: the phrase input's `PMSIVMenu_SetupEditButtons` keeps
  `wk->items[i].str = ...` for its stores and passes `item`. Indexing at both places multiplies twice, and storing
  through the pointer moves the stores onto it.
- Stores through a pointer to an array element, `icon = &icons[3]; icon->chars = ...;`, use the element's address as
  their base register, while `icons[3].chars = ...;` reaches the field from a base of MWCC's choosing, often an
  earlier element. The Pokédex touch bar's map and forms buttons are filled through a pointer. The other way round,
  a loop that indexes `wk->buttons[i].rect[0]` keeps `wk + i * size` and adds each field's offset, where a
  `LanguageButton *button` local gives other registers, as the Pokédex info page's language buttons show.
- A local array or struct initialized in its declaration is stored through a base register, `add r0, sp, #0x4c;
  str r4, [r0]; str r4, [r0, #4]`, where assignments to its elements store at `sp` offsets. A `u8` array initialized
  so, `u8 kinds[3] = { FALSE, FALSE, FALSE };`, also makes MWCC load memory again after each store to the array, where
  with assignments it keeps the first load: the Pokédex habitat map reads `wk->habitat` again for each of its three
  tests of a place's habitats.
- The initializer's stores happen at the declaration, so an array cleared by an initializer after some calls is
  declared in an inner block opened there: the forms page's `BOOL hasSex[3] = { FALSE, FALSE, FALSE };` follows the
  Pokédex reads in a block around the rest of the gathering, and `BOOL seenRare[2] = { FALSE, FALSE };` sits in the
  loop over the forms.
- A switch of three cases can compile to a compare chain that tests them in value order. When the original tests 1,
  then 2, then 0, the source is an `if`/`else if` chain in that order: `scrcmd_msg.c`'s `SystemMsgWin_Open`.
- A result flag declared after a call, `CalcAttachmentPos(env, FALSE); BOOL done = FALSE;`, takes the register of a
  parameter that died in the call, and MWCC still sets it just before the `bl`: `scrcmd_msg.c`'s
  `ScriptNative_ActorMsgWinWait`.
- A struct member set from a C99 compound literal, `request.pos = (VecFx32){ 0, 0, 0 };`, builds the literal in a
  stack temporary through a base register just before the copy (`str r6, [r3]; str r6, [r3, #4]; str r6, [r3, #8];
  ldm r3!, {r0, r1}`), in source order among the other member stores. An initialized local makes the same stores at
  its declaration, and a local set field by field stores at `sp` offsets. fldeff_shadow.c's shadow task builds its
  actor request so.
- A value that a loop uses and the code after it uses again is reused from the copy hoisted out of the loop. When
  the original computes it again after the loop, the loop assigns it to a variable declared in the loop's body, as
  `int wanted = mode + 1;` in the Join Avenue's records command.
- An address passed to a `const` pointer parameter is converted, and the conversion is not shared with the same
  address written elsewhere. When the original computes an address a second time for another call, the first call
  takes a `const` pointer, as `GymElecFade_IsActive` does.
- Reads of a `const` table at a constant index are folded into immediates, but reads in a loop over the table are not,
  even when the loop runs once and is unrolled. An `ldm` from a table straight into argument registers is two fields
  read in such a loop, as the egg and evolution demos' particles load the resource of each of their one unit. Reads
  through a pointer to the entry, `const BitmapEntry *entry = &sListBitmaps[BMP_YES];`, aren't folded either, as
  `research_list.c`'s `ResearchList_DrawYesButton` and `DrawNoButton` load their bitmap's file, colors and text from
  the table; written as `sListBitmaps[BMP_YES].arcId`, the same function is 0x38 bytes shorter.
- A struct assignment, `u->pos = *pos`, copies with `ldm`/`stm`. Separate `ldr`/`str` pairs for each field are the
  NitroSDK's `VEC_Set(&u->pos, pos->x, pos->y, pos->z)`, as `iss_3ds_sys.c`'s `ISS3DSoundSys_SetListenerCore` writes it.
  More exactly: each field's load followed by its store is three assignments, `v.x = p->x; v.y = p->y; ...`; the loads
  of z, y and x first and then the three stores is `VEC_Set`, and branches that each end in a `VEC_Set` into the same
  vector share one store tail (`scrcmd_fldmmdl.c`).
- Two locals initialized to 0 in their declarations share one zero register, so a later `offset += 4` compiles as
  `adds r5, r4, #4` from the counter's zero: `iss_switch_set.c`'s `ISSSwitchSet_LoadArcDataCore` declares `int i = 0;
  u32 offset = 0;`, where `offset = 4` shares the `movs #4` of another argument instead.
- A value moved into an argument register just before a call, and used for nothing else, is an argument the prototype
  is missing. `GFL_SEPlayKeepVol` takes the sound's player as well as the sound. The value can be narrowed for it:
  `CtvtComm_RecvPacket` narrows `packet->value` into r1 (`lsls`/`lsrs #24`) but compares the unnarrowed value with
  0xff, because `func_ov257_021aad74(sys, u8 talker)` takes it; with `u32 talker = packet->value` and the talker
  passed, it matches, where a `u8` local or no argument cannot. A missing parameter can also swap the registers of
  the caller's loop variables, as it did in `CtvtComm_UpdateTalk` (`CtvtComm_IsMemberTalking` takes the net ID).
- A value built once and passed both in `r3` and in the first stack slot (`mvn r3, r3; mov r0, r3; str r0, [sp]`)
  is a `u64` argument, its low word in `r3` and its high word on the stack. Two `u32` arguments get a constant each:
  the summary screen's debug box matches only with `PML_CreateTempPkm(pkm, species, level, PKM_ID_RANDOM)` taking a
  `u64` ID.
- A caller that keeps an argument register untouched across a call to a function that ignores it is passing that
  argument: `ShinkaDemoPieces_IsFadeDone` takes the heap ID like the functions around it.

- `if (!f()) { ... } else { return x; }` puts the `else` out of line, after the function's other code, where
  `if (f()) { return x; }` keeps it in place (`ctvt_game.c`'s `CtvtGame_Main` and `CtvtGame_UpdatePlay`).
- A `u16` local that holds a call's result changes the operand order of a later add with it, and
  `index = first; index += kind;` truncates a `u32` field before the add, where `first + kind` does not
  (`ctvt_game.c`'s `CtvtGameTarget_UpdateHit` and `CtvtGameTarget_Draw`).
- `*(data + pos + 1)` adds `pos` once and then loads at `#1`, where `data[pos + 1]` adds `pos + 1` first; and a value
  built in a `u16` local with `|=` and then returned has no final narrowing pair, where a returned `int` expression does
  (`ssp_exifdec.c`'s byte readers). The battle server's command queue reads its halfwords and words the same way,
  `*(que->buffer + que->readPtr + 1)` loading at `#9` from `que + readPtr` (`btl_server_cmd.c`). A ternary that stores
  to a static is a select (`bhi`); the original's `bls; b` is an `if`/`else` with a store in each branch.
- Advancing a pointer-like offset with `x += 0xc` lets MWCC fold the 0xc into every later offset from it; writing
  `pos = x + 0xc` and reading through `pos` keeps the `add` (`ssp_exifdec.c`'s IFD loop).
- A ternary argument `f(c ? 1 : 0)` compiles to the select form (`movs r0, #1; cmp; beq; movs r0, #0`). A branchy
  original (`bne`; `movs #1`; `b`; `movs #0`) is an `if`/`else` with a call in each branch, as `CtvtTalk_UpdateMain`
  calls `func_0203d564(TRUE)` or `func_0203d564(FALSE)`.
- A ternary store `*p = c ? a : b` computes the address once and stores after the branches; an `if`/`else` with a
  store in each branch computes the address in each, as `Bitmap_Scroll16` does.
- Parenthesized offsets change the code: `pixels + (dst + 4)` adds the offsets and indexes once, `pixels + dst + 4`
  adds to the pointer twice (`Bitmap_Scroll256`).
- The operand order of a product decides which value is loaded first: `(row + 1) * rowHeight * 2` and
  `widthTiles * (y & ~7)` match in bmp_menulist.c where the other orders do not.
- `u8` flag parameters, not `BOOL`, change the order in which register parameters are spilled at entry
  (`BmpMenuList_CycleCursor`).
- A bit-table lookup with an unsigned `lsr #5` for the index and a signed modulo for the bit is
  `table[item / (sizeof(u32) * 8)] & (1 << (item % 32))`; `item >> 5` gives `asr` (pml_item.c).
- A result computed into the parameter's own callee-saved register comes from a compound assignment to the parameter
  (`item -= ITEM_TM93 - TM_INDEX_TM93;` in `PML_ItemGetTMWazaID`); `item = item - X` or a new local computes into r0.

- A parameter masked in place (`word &= 0x7ff`) with no narrowing after is wider than the callers' `u16`: pms_word.c's
  `PMSWord_GetMessage` takes a `u32`.
- A store of a loaded value back into an address-taken out-variable's slot is a reassignment in the source:
  `fileId = sCategoryMsgFiles[fileId];` after the call that filled it (`loadSayingToString`).
- An address-taken local read inside a nested loop is copied to a stack slot of its own before the outer loop, and
  a field read through it is hoisted only out of the loop it sits in: `CygnusAppear_GetSpriteBottom` reads
  `((u32 *)charData->rawData)[...]` in its middle loop, and the original keeps a second slot holding `charData` with
  `->rawData` loaded once per outer pass. A `tiles` local set in the outer loop drops that slot.
- `for (j = 0, base = 0; ...)` zeroes `j` first; `base = 0;` before `for (j = 0; ...)` zeroes `base` first
  (`PMSWord_FromMessage`).

- `x |= c` reloads the field and its pointer, while `x = x | c` reuses the value just tested: infowin.c's
  `InfoWin_Update` sets a flag with `flags = flags | 4`.
- Clearing a bit of a `u16` field with a pool literal of 0x0000efff is `& (0xffff ^ bit)`; `& ~bit` gives 0xffffefff,
  which only shows in the pool bytes (`InfoWin_Update`).
- An index masked with `lsl #24; lsr #22` at its use is a `BOOL` passed through a `u8` parameter of an inline
  (`InfoWin_GetSignalColors(u8 on)`).
- A constant choice passed to a `u16` parameter without narrowing is `u16 v; if (...) v = A; else v = B;`; a ternary
  keeps the `lsl`/`lsr` (actor_tool.c's `ActorTool_LoadPalettesFade`).
- A loop that copies its index each pass (`adds r3, r2, #0`) has a `u32` index against an `int` bound
  (`ActorPalSlots_Free`).
- A switch whose result moves to r0 once at the end assigns a result variable initialized to the input
  (`u8 next = pos; switch ...` in cursor_move.c's `CursorMoveData_GetLink`).

- An `int` assigned to a `u8` bitfield is narrowed (`lsl`/`lsr #0x18`) before the bit insert; a `u8` value isn't
  (game_comm.c, a `BOOL flag` stored in a 1-bit field).
- Loads still move above stores to other known offsets of the same pointer: in `GameCommSys_Main`,
  `comm->work = NULL; comm->commNo = 0; cb = comm->exitCallback;` reads the callback first, and only that source order
  gives the original's registers.
- A struct is copied by its type's alignment: one of `u8` fields bytewise, a union with a `u16` by `ldrh`/`strh`
  while its fields stay byte-accessed (game_beacon.c's `GameBeaconTime`).
- One load of a global serving a store and a following address comes from taking the address into a local before the
  store (`GameBeacon *beacon = &GameBeaconSys->mine.beacon;` in `GameBeaconSys_SetGameData`).
- `x == n` compiled as `sub; bne` is `x - n == 0`; `return (*p)++` and an increment followed by `return *p - 1`
  differ (`GameBeaconSys_GetRecentEntry`, `GameBeaconSys_GetNextNew`).
- A `u16` parameter is spilled at entry before the other parameters are moved, a `u32` one after them. A caller's
  `u16` narrowing can come from its own `u16` local rather than the parameter type (`GameCommSys_LogPlayers`).

- A store written before a read through a `const` pointer parameter can move above another store; dropping the
  `const` keeps the source order (app_taskmenu.c's `AppTaskMenu_Create`).
- A range test `subs; subs; cmp; bhi` is an unsigned difference in the source, `x - left <= right - left` on `u32`
  values; MWCC doesn't fold `x >= left && x <= right` into it (`AppTaskMenuWin_IsTouched`).
- A value narrowed to `s16` once and also used unnarrowed in `16 - x`, as in
  `(s16)(16 - alpha) << 8 | (s16)alpha` for `BLDALPHA`, came from an inline with `s16` parameters:
  `mystery.c`'s effects match with `static inline void Mystery_SetBlendAlpha(s16 ev1, s16 ev2)` storing
  `ev1 | (ev2 << 8)`, called as `Mystery_SetBlendAlpha(alpha, 16 - alpha)` with a `u32` alpha; `s16` locals or casts
  narrow in the other order.

## Branches and block layout

- When both branches of an `if`/`else` give a local a constant, MWCC emits the then-value, branches over the else
  value on the condition, and the else-value last, so the condition's direction decides which constant comes first:
  `b_bag_anm.c`'s button animation needs `if (button >= 10) palette = 2; else palette = 5;`, and `< 10`, a ternary
  or a default followed by an `if` all fail.
- `if ((a | b | c) != 0)` keeps the ORs in one test where `if (a | b | c)` is split into branches, as
  `btlv_effect.c`'s rotate task shows.
- A one-case `switch` keeps a result variable that `if (f() == 1 && x)` folds into two returns: `btl_main.c`'s
  `func_ov167_0219db08` zeroes `result` before the call, as the original does, only as
  `switch (f()) { case 1: if (x) { result = TRUE; } break; }`.
- `if (x == TRUE) { return 0; } return 1;` and its other spellings build 1 first (`movs r0, #1`, `bne`); the
  original's `movs r0, #0`, `beq`, `movs r0, #1` is a result variable,
  `ret = 0; if (x != TRUE) { ret = 1; } return ret;`.
  `plist_sys.c`'s `PokeList_CheckLearnMove` writes two of its three such returns that way and one plainly, so compare
  each return of a function on its own.
- `if (a == b) { return FALSE; } return TRUE;` is folded into `a != b`, which branches with `beq` to the `FALSE`
  return whatever the spelling. The original's `bne` to the `TRUE` return comes from a flag: `scrcmd_stadium.c`'s
  `IsReturnLocationNonLeaguePokeCen` matches with both calls in locals, `isLeague` set in an `if`/`else` from their
  comparison, and `if (isLeague) { return FALSE; } return TRUE;`.
- A range check that adds the 16-bit negated constant (`adds r0, #0xc004`, narrowing, `cmp r0, #3`, `bhi`) is an
  `||` chain of `==` tests of consecutive constants; a written `(u16)(x - C) <= n` subtracts instead.
  `billboard_act.c`'s `func_0204f768` tests the last animation command this way.
- An inline that returns a condition, `return a && b;`, builds the flag ahead of the tests (`movs r1, #0`, the
  tests, `movs r1, #1`) where the original's `movs r0, #1`, `b`, `movs r0, #0` after the tests is
  `if (a && b) { return TRUE; } return FALSE;` (2.0/sp2p2): `g2d_Animation.c`'s `IsFrameEnd_`, the test of
  `NNS_G2dTickAnimCtrl`'s loop. It goes the other way too: `g2d_CellTransferManager.c`'s `IsTransferNeeded_` keeps
  its 0/1 only as `return a && b;`, which the if-form folds into the branches, so try both.
- An inline's 0/1 result that the original materializes and then tests (`movs r0, #1`, `b`, `movs r0, #0`,
  `cmp r0, #0`, `beq`) where ours branches on the comparison itself needs the result kept as a value (2.0/sp2p2): a
  `u32` return type with `if (c) { return TRUE; } return FALSE;` (`g2d_CellAnimation.c`'s
  `NNS_G2dCellDataBankHasVramTransferData`, which every `BOOL` spelling folds into the test), or a cast,
  `return (BOOL)(GetCapacity_(region) >= num);` (`g2d_OamManager.c`'s `IsCapacityEnough_`, where the `if` form, a
  ternary, a plain comparison and a local all fold).
- A 0/1 choice laid out first value first (`bhs`, `movs r0, #0`, `b`, `movs r0, #1`) is held in an enum type: every
  plain 0/1 spelling (ternary, `if`/`else`, `!`, `== FALSE`, an int local) gives `blo`, `movs r0, #1` first.
  `btl_main.c`'s `GetSideFromMonID` matches as `BtlSide side = monId < 12 ? BTL_SIDE_1ST : BTL_SIDE_2ND;`, with the
  condition written so the original's first arm comes first.
- A flag set to 0 and then to 1 under a branch (`movs #0`, `cmp`, `beq`, `movs #1`) is
  `BOOL b = FALSE; if (x) { b = TRUE; }`; `x != 0` builds 1 first. `script_command.c`'s `StaScriptCmd_PokeFlip`
  also needs the call before it in the argument list read into a local first.
- Blocks are laid out in source order. A switch whose default code comes right after its comparisons or jump table
  had `default:` written first, and `if (f()) { n++; } else { return FALSE; }` puts the return after the code that goes
  on, where `if (!f()) { return FALSE; } n++;` puts it before.
- Every `return` gets its own epilogue. Failures that all branch to one block that sets a saved register and jumps
  to a shared `mov r0, rN` exit come from a result variable and a single `return`, as `mystery_gift_pokemon.c`'s
  original has.
- Identical statements in different branches are merged, so a branch that jumps into the middle of another block had
  the same code in the source. For example, `if (a) { x = 3; y = 19; } else { x = 0; y = 19; }` compiles differently
  from `x = a ? 3 : 0; y = 19;`. A run of jumps to one store, as in the start menu's `StartMenu_MoveCursor`, is the
  same store written in several `else` branches.
  A test that branches past an unconditional jump, `bne next; b hide`, where `||` would give one `beq hide`, is the
  first copy of a body written twice in an `if`/`else if` chain and replaced by a jump to the second:
  `func_ov194_021c4ec0` hides a marking icon with `if (anim == -1) { hide } else if (isEgg && i == 6) { hide }`.
- A `bne` over a `b` into another branch's call, `cmp r0, #0; bne x; b call; x: cmp r7, #0; beq call; mov r6, #1;
  call:`, is the call written in both branches: overlay 185's `PMSIView_CmdWordWinToCategory` matches only with
  `if (mode == 0) { f(flag); } else { if (search) { flag = TRUE; } f(flag); }`. Writing the call once after the `if`
  also swapped the registers of `flag` and `search`.
- A range or equality test that ends in `b store` while its other arm is `mov rN, #const; b store` is the store
  written in both arms, `if (x >= 20 && x <= 23) { wk->pos = x; } else { wk->pos = 22; }`: the then-arm's store is
  cross-jumped into the shared one and only its `b` is left. The phrase input's `PMSInput_CategoryKeyInitial` writes
  its cursor's fallbacks so; a fallback assigned to the value and stored once after gives a plain branch to the store.
- Of a store written in several arms, cross-jumping keeps the copy written last and turns the others into `b`, so the
  order of the arms decides where the store sits. The phrase input's `PMSIVWordWin_SetScrollBar` keeps its top
  position's store at the end of the function, behind a plain `beq`, only as `else if (scrollMax != 0) { compute }
  else { y = TOP; }`; `else if (scrollMax == 0) { y = TOP; } else { compute }` kept it early behind `bne; b`. Its
  `PMSIVWordWin_GetScrollBarLine` has `cmp #0x12; bne next; b zero` from `line = 0` written both as the first arm of
  the inner chain and as the outer `else`.
- A branch to the very next instruction is left by cross-jumping: two statements that end the same way, such as a
  store in each case of a switch, share their tail, and the first jumps to it even when it follows. A call written
  at the end of both arms of an `if` is merged the same way: `p_sta_sub.c`'s `PStaSub_AnimSwipeX` stores the bounce
  and calls `PStaSub_SetBounce` in each arm, and the `else` passes the zero left from the vector's initializer; one
  call after the `if` drops the `b`.
- MWCC evaluates the operands of `|` in the order they are grouped, so a color built from three computed parts shows
  its grouping: `field_menu.c`'s cursor fade (`func_ov036_021a040c`) computes red, then blue, then green, and only
  matches as `r | ((b << 10) | (g << 5))`, not as `GX_RGB(r, g, b)`.
- A block that many cases of a switch branch to, such as the step advance of `event_entrance_effect.c`'s
  `func_ov036_0219f380` (`*state = next(work); advance(work);`), is each case's own copy merged by cross-jumping. A flag
  set in the cases and tested after the switch keeps a register for it and doesn't match.
- An early `return` at the top of a long function jumps to the nearest `b` to the epilogue. When the original skips the
  body with `bne` over a `b` to the very end, the body was wrapped in `if (cond) { ... }`, as in the PC box's
  `Box2Main_PokeDataMove`.
- A function whose last check returns a register that also served as a `NULL` argument, `mov r4, #0` ... `cmp r0, #1;
  beq; mov r4, #1; mov r0, r4`, ends in `return f(...) == TRUE ? FALSE : TRUE;`; an `if` with two returns, or `!=`,
  merges that return with an earlier one. The PC box's `Box2Main_PokeItemMoveCheck` shows it.
- The two halves of an `if`/`else` that end in the same computation are merged by cross-jumping, unless they end the
  function: a step of the PC box's icon moves sets `vx` and `mx` in each branch of the x test and `vy` and `my` in each
  branch of the y test, and only the x branches share their tail; a variable for the difference merges the y branches
  too and doesn't match.
- When comparing a call's result, `v = f(); if (v == x)` and `if (f() == x)` put the operands of `cmp` in opposite
  orders.
- `if (!f())` and `if (f() == FALSE)` lay the two blocks out in opposite orders. `field_sound_system.c`'s
  `FieldSnd_GetLastQueuedCommand` puts the `then` block first behind `bne` only with `== FALSE`; `!` put the `else`
  block first behind `beq`.
  A ternary is the same: `fieldmap.c`'s `FldActSys_VRAMUploadFunc` branches with `bne` and puts the `a` value first
  only as `!type ? a : b`; `type == 0 ? a : b` branches with `beq` and puts `b` first.
- The operands of `==` between two fields are compared in source order: `syswk->tray == syswk->getTray` gives
  `cmp tray, getTray`, as the PC box's `func_ov255_021cc8dc` needs.
- The left operand of a comparison is loaded first, even before a store just above it: `sw->nowFrame++; if
  (sw->endFrame < sw->nowFrame)` loads `endFrame` before the increment's store, as `iss_switch.c`'s
  `ISSSwitch_AdvanceFade` does, where `sw->nowFrame > sw->endFrame` is 2 bytes off.
- A store picked by a test, `if (pos < 30) syswk->pos = pos; else syswk->pos = 0;`, branches past the second value with
  `bhs` and `b`, while clamping a local first, `if (pos >= 30) pos = 0; syswk->pos = pos;`, uses one `blo`. The PC box's
  `func_ov255_021c8b38` is the first.
- `x = x == 0 ? 3 : x - 1;` reads `x` once, and `if (x == 0) { x = 3; } else { x--; }` reads it again in the `else`
  branch before the shared store, as the Pokédex habitat map's season changes do.
- The last test of a condition branches to the code written second. `if (a == x || a == y) { return TRUE; } return
  FALSE;` ends with `bne` to the `FALSE` return, while the original's `beq` to a `TRUE` return placed after the `FALSE`
  one is `if (a != x && a != y) { return FALSE; } return TRUE;`, as `plist_demo.c`'s Reveal Glass and Gracidea checks
  are written.
- `a == 4 || a == 5` becomes a range check. Separate comparisons that jump to the same code come from separate
  branches with the same body.
- A clamp that ends in one store, with each limit copied into the value's register, is a conditional expression.
  `if`/`else if` stores each limit separately.
- A `b` to a `b`, where the original branches straight to the shared code, is a call written after an inner
  `if`/`else` that the original has in each branch. MWCC merges the copies with the later one but the jump
  from the first branch then targets the end of the second: the summary screen's `PStaSkill_HandleForgetKeys`
  matches with `GFL_SndSEPlay(SEQ_SE_DECIDE1);` at the end of both branches of its `move == MOVE_NONE` test.
- A call whose one argument differs by branch, such as the BG map's file ID, is written out in full in each branch:
  `if (c) { LoadMap(..., FileId(0x16a), ...); } else { LoadMap(..., FileId(0x169), ...); }`. MWCC merges the identical
  tails, but a `file` local or a ternary argument allocates the registers differently, by 25 lines in
  `btlv_input.c`'s standby-to-moves task.
- An `if`/`else` that assigns one field a constant in each branch can still end in one store after the branches, with
  `b` over the else branch, as the trade's key cursor wraps its row to 2 or 4 in `pokemontrade_proc.c`. The
  conditional expression gives `mov`, a conditional branch over a second `mov`, and no `b`.
- A call whose argument is picked by branches comes from one of two sources, told apart by the layout. `f(x ? FALSE :
  TRUE)` tests `x` with `bne` to the second value, and puts the value for `x == 0` first. Two calls in an `if`/`else`,
  `if (x) f(FALSE); else f(TRUE);`, are merged into one call after the branches, with `beq` to the else branch and the
  then branch's value first. The evolution demo's touch screen flags and its view and effect creation are two calls.

- An `if`/`else if` chain whose first and last branches make the same call gets the calls merged (`bgt +2; b call`):
  wipe_sub.c's `WipeCircleWork_Compute` matches with `if (y <= cy) call; else if (y <= cy * 2) mirror; else call;`,
  where a condition joined with `&&` doesn't.
- Assigning both of two values in every branch (`start = 16; end = 0;`) lets MWCC build -16 from the register holding
  0 (`WipeBright_Init`); zero-initialized locals or a ternary don't.
- A constant argument loaded in both arms of an `if`/`else` (`movs r3, #0x3c` in each) was assigned to a local in each
  branch, even when the value is the same: `field_sound.c`'s `FieldSnd_ChangeZoneBGM` matches with `fadeOutFrames =
  60` set beside `fadeInFrames` in both branches, where `60` written once at the call is 2 bytes short.

- When the original puts an `if`'s then-block after the else path, write the condition negated with the bodies
  swapped: brightness.c's `BrightnessData_Step` matches with the long advance body first and `done = TRUE` in the
  `else`, for both its tests.
- A test of a value against a few nearby constants returned as `return x == a || x == b || x == c;` compiles to a bit
  test, `sub; cmp #range; bhi; mov #1; lsl; tst #mask`, while the same test as an `if`, or as a `switch` that returns or
  sets a flag, compiles to compares. `itemmenu.c`'s `ItemMenu_IsRepel` returns the expression for its three repels.
- Nested tests that end in the same call can come from a nested `if` whose inner `if` has no `else`: the bag's item menu
  calls `func_0202d384` with `if (pocket != FREE_SPACE) { if (pocket != KEY_ITEMS) f(); } else if (...) f();`, which
  puts the Free Space's test after the other two; an `if`/`else if` chain or a `switch` puts it first.

## Loops

- A `for` loop with a constant bound keeps its test before the first pass only with a counter of an enum type, as the
  position loops of `btlv_mcss.c`, `btlv_effvm.c` and `btlv_gauge.c` do with `BtlvMcssPos` and `BtlvGaugePos`.
  It also keeps an array indexed by the counter indexed on each pass rather than walked by a pointer:
  `g2d_CellTransferManager.c`'s `NNS_G2dUpdateCellTransferStateManager` needs `NNSG2dVRamType type` (2.0/sp2p2).
- A loop counted with `!=` tests with `beq` before the loop and `bne` at its end, where `<` gives `bls` and `blo`:
  the forms page walks its form-name table with `for (i = 0; i != form; i++)`.
- `while (cond)` is rotated, with a copy of its test before the loop. A loop that tests once, at its top, is
  `while (TRUE)` with a `break` or `return` inside, as the Join Avenue's walks through its data are.
- A loop that runs once is unrolled when its counter and bound have the same signedness. `int i; i < NELEMS(x)`
  compares unsigned, so the loop stays, as in the gym files' loops over one-element tables.
- An address that an inner loop computes from the outer loop's counter, such as `&pieces->pieces[row][col]`, is hoisted
  into the inner loop's preheader, after the inner counter is set. When the original sets the inner counter before
  computing the row's address, the source takes the address in the inner loop rather than through a row pointer.
- A value the inner loop computes from the outer loop's counter alone, like `turn = row / 3`, is hoisted into the
  preheader too, so it can be written inside the inner loop. `row % 3` used in both conditions of an `if`/`else if`
  is computed once, later than a `rowInTurn` variable set with `turn` would be.
- A loop that walks a pointer parameter (`for (; options->text != END; options++)`) reuses the test's load in the
  body. When the original loads the field again at the top of the body, the loop walks a local cursor set from the
  parameter instead (`for (option = options; option->text != END; option++)`), as bmp_menuwork.c's
  `ListMenuCore_FreeStrBufs` and `ListMenuCore_GetFirstFreeIndex` do.
- A bound written as `i <= N - 1` is computed once into a register before the loop and tested with `ble`, where
  `i < N` reloads `N` from the literal pool and tests with `blt`. The bag's Free Space list compacts its entries with
  `for (i = 0; i <= BAG_ITEM_LIST_SLOTS - 1; i++)` in `bag_item.c`'s `BagItemList_Compact`.

- A test that the original places after its body, entered from the top as well as from an earlier branch, is a loop
  that stops after its first pass: `while (box < n) { ...; break; }`. The trade does one box a frame this way in
  `pokemontrade_proc.c`'s `func_ov194_021bb3c0` and `pokemontrade_2d.c`'s `func_ov194_021c2c04` and
  `func_ov194_021c3e9c`, each 8 bytes or so shorter as an `if`. Comment it, so it isn't "fixed".
- A load the original hoists out of an inner loop but not out of the loop around it was in the middle loop's body.
  `mystery.c`'s `Mystery_GetSpriteBottom` reads a sprite's characters a row at a time with
  `tile = (u32 *)charData->rawData + (row * 12 + col) * 8;` in the column loop; read in the innermost loop, the load
  only moves to the column loop, and read in the row loop, the `charData` load is not hoisted.

## Switches

- A `switch` with no `default:` whose cases leave one value unassigned keeps the index in `r3` instead of `r2`, as
  `b_plist_main.c`'s `BPlistMain_InitPageCursor` does, and writing `case 0:` last moves its block to the end of the
  switch (`BPlistMain_CanSwitch`).
- `*result = N; break;` in every case makes later cases branch to an earlier case's `b`, a branch to a branch. A local
  set in each case and stored once after the switch gives direct branches: `scrcmd_fldmmdl.c`'s `s0078`.
- Switch cases are laid out in source order, not by value, so the layout shows the order the cases were written in.
- A switch's comparison tree and jump tables depend on every case value, including cases with no code: the Battle
  Subway's command switch only splits its values as the game does with an empty `case 102:` inside its first jump
  table, and empty cases that sit between others still get a comparison.
- Listing the empty cases also keeps a jump table where identical case bodies would otherwise be merged into
  comparisons: `ringtone_sys.c`'s `RingtoneSys_UpdateState` matches only with all four states written, two of them
  empty.
- A switch case that ends in the same code as another case is merged into it, so its end moves.
- A switch whose cases each set every argument of one call after it, as `research_top.c`'s button highlights set the
  BG, position, size and palette for `GFL_BGSysSetScrPaletteNo`, has each argument in a variable: the cases keep only
  the values that differ, and the values they share are set once at the merged end, in registers. Writing only the
  differing value as a variable leaves the others as constants at the call, and a call in each case is merged
  differently.
- A `switch` on a few small values tests them all first (`cmp; beq` for each, then `b` to the default), while an
  `if`/`else if` chain tests each one before its body (`cmp; bne` to the next test), as bmp_menu.c's
  `BmpMenu_NextCursorPos` does.

- A case that ends in the same code as the next (`seq++; done = TRUE; break;` before `case 3: done = TRUE; break;`)
  is merged into it, leaving a `b` to the next case. The original's code is that `b`, so write each case out in full
  with its own `done = TRUE; break;`, as wipe_sub.c's `WipeBright_Main` does, rather than a fall-through.
- The comparison tree depends only on the set of case values, and evenly spaced cases are grouped from the low end:
  the PC box search's `func_ov255_021d40e8` cases {0, 3, 9, 15, 21, 24, 27, 30} split at 27, and no order, type or
  `default:` changes that.
- A switch whose comparisons start with a value outside its jump table, `cmp r0, #5; beq other; cmp r0, #3; bls table`,
  with that value's code after the cases, is `if (x != 5) { switch (x) { ... } } else { ... }`; the same test before
  a compare chain, `cmp r0, #5; beq end`, is the `if` without an `else`. The Pokédex forms page's button input reads
  so: a `case 5:` in the switch puts 5 in the table.
- A short chain of tests whose first value does nothing, `cmp r5, #1; beq end; cmp r5, #3; bne next`, with each body
  after its test, is `if (x == 1) { } else if (x == 3) { ... } else if (x == 4) { ... }`; a switch of the same values
  branches to its cases instead. The Pokédex habitat map's state changes test the new state so.
- A trailing `case N: default:` sharing one body gives `cmp rN, #N; b body` before the jump table, and a switch with
  no `default:` returns on an out-of-range value with no code for it, as `btlv_input.c`'s target-to-moves task does.
- Two equality tests that branch away with `beq; beq; b`, with each body after, are a `switch` on those two values, as
  `btlv_input.c`'s PP colour pick is a `switch (pp)`. An `if`/`else if` tests each before its body.

## Floats and runtime helpers

- A random pick in a variable range with no zero check is `u64 value = GFL_RandomMT(); value *= n; value >>= 32;`, as
  `btlv_mcss.c`'s idle task does.
- Float arithmetic calls MWCC's runtime helpers, such as `_fadd` and `_ffix`, which swan names `__aeabi_*`. When a
  complete file fails to link on one of them, rename it to the MWCC name with `rename_symbol.py`, or, when the AEABI
  name is in use, give it the MWCC name as a second one with `config_fixes.py add-label`, as `_ddiv` and `_dsub` for
  `btl_pokeparam.c`'s `GetHPRatio`.
- Float arithmetic on a literal passes the literal first, as in `_fmul(4096.0f, x)` for `x * FX32_ONE`, whatever the
  source order. A constant kept in a local variable, which is reloaded from the literal pool at each use, keeps its
  place in the source instead, so `col * pixels` with `f32 pixels = 96.0f / 18;` passes `col` first. A compound
  assignment passes its target first: `research_list.c`'s `ResearchList_ReleaseDrag` calls `_dmul` with the drag
  speed in `r0`/`r1` and 1.5 in `r2`/`r3` for `wk->dragSpeed *= 1.5;`, while `wk->dragSpeed = wk->dragSpeed * 1.5;`
  loads the literal into `r0`/`r1`.
- NitroSDK's `FX32_CONST(x)` names `x` three times, in its test and in both branches, so a call written inside it is
  made three times. The game passes a local, as the capture rate in `btl_server_flow_sub.c` does.
- A literal in a compound assignment keeps its place: `y = scale.y / (f32)FX32_ONE; y += 0.01f;` calls
  `_fadd(y, 0.01f)`, where `y = scale.y / (f32)FX32_ONE + 0.01f;` calls `_fadd(0.01f, y)`, as the Pokédex cry page
  stretches its Pokémon.
- MWCC doesn't fold float arithmetic on a local variable that holds a constant, so `size / 2.0f` stays a call when
  `size` is a variable, while an expression of literals is folded.
- `compiler_probe.py` skips relocated words, so a wrong addend, such as a table index that the compiler folds into a
  literal pool address, or a call to the wrong runtime helper, only shows when the module check fails. Division and
  modulo call `_s32_div_f` for a signed operand and `_u32_div_f` (swan's `__aeabi_uidivmod`) for an unsigned one; a
  `u8` or `u16` promotes to a signed `int`, so `(u8)id % 5u` is the unsigned one, and so is a `u16` divided by a
  `u32`. Compare the built overlay in `build/<version>/build`
  with the original to find it.
- A 64-bit division calls `_ll_udiv` (swan's `__aeabi_uldivmod`) when either operand is unsigned. A call to it with
  operands sign-extended by `asr #0x1f` is an `int` cast to `u64`: the trade's box cubes turn by
  `((u64)((x + 48) % width) << 16) / width` in `func_ov194_021c2844`. A file that calls it needs `_ll_udiv` added as a
  label on `__aeabi_uldivmod` with `config_fixes.py add-label` before it can go complete.
- A float one ULP off a round decimal is written with the shortest digits that round to it, and a comment:
  Kadabra's sprite offset in `pokemontrade_3d.c` is 0x40533334, next to `3.3f`'s 0x40533333, so it is `3.3000002f`.

## Data and sections

- `-ipa file` and `-ipa function` address statics differently, which tells a library file's setting. With
  `-ipa file`, a file's `.bss` and `.data` objects share one section, are reached from its base (one literal, then
  `[r0, #off]`, shared by every object a function touches) and are dropped when no code reads them. With
  `-ipa function`, each object has a literal of its own, objects no code reads stay, and they are laid out in reverse
  order of declaration. NitroSystem's sound capture (`lib/nnsys/src/snd/snd_capture.c`) loads its flag, queue and
  capture struct each from its own literal and keeps a word and a message buffer that only stripped code used, so it is
  built with `-ipa function` (`file_flags` in `lib/nnsys/library.toml`), while the other sound files are reached from
  one base and need `-ipa file`. A struct that looks like another file's extern, or a gap in `.bss` that seems to need
  a stand-in function, is the first thing to try this on.
- A file-scope static that a function both passes by address and reads or writes directly gets two literals with
  the same address: the access goes through one (`ldr r0, =obj; strh r1, [r0, #0xe]`) and the pointer passed on
  through the other, where an `extern` object's address is loaded once and kept in a register, 4 bytes shorter. Two
  literals for one object therefore mean a static of the function's own file. `fieldmap.c`'s
  `Field_LoadEdgeColorTable` stores the last edge color through a second literal only with
  `static GXRgb g_FieldEdgeColorTable[8];`, and the overlay 36 gap function at `0x0219a044`, which copies into a
  12-byte object and then tests one of its bytes, matches the same way only with that object `static`.
- The file's `.data` objects take part in MWCC's size sort that orders `.rodata`: `btlv_mcss.c`'s 3-byte idle-wait
  array had to be counted before `rodata_order.py` predicted the layout. So do its `.bss` statics: the object's
  symbol table lists them interleaved with the `.rodata` objects by size, and moving a table moves the statics too.
  `rodata_order.py` doesn't predict such a file, so `btl_client.c` was solved by compiling variants: each table at
  file scope or inside its one function, and each static at the top, before its first function or inside it. Its
  `.rodata` and `.bss` are the original's with four tables inside their functions, `sXItems` and `sCureItems` at
  file scope before theirs, and `sSelectSeq` declared before the first function that uses it.
- A function-local static of a function MWCC doesn't emit is dropped, while a global read only by an unemitted
  static function stays in the shared section. Data that outlived code the original link dead-stripped can't be
  reproduced under `-nodead`: wipe_sub.c's `.data` and `.rodata` hold the parameters of about 31 handlers the ROM
  doesn't have, as their own function-local statics, which an unemitted handler takes with it. Making them globals
  read by unemitted statics was not tried; it would put names on data Game Freak kept local.
- Tables that only code the linker dropped read can be kept in the shared section as globals read by unemitted
  static functions. `mbp.c` (overlay 181) keeps NitroSDK's demo tables of state and callback names this way, after
  its heap ID and before their strings: a `static` table read only by an unemitted function is dropped with it, and a
  global that nothing reads gets a section of its own among the strings. The string literals of such initializers
  each get a section of their own, laid out by size after the shared section, and their equal sizes don't follow
  `rodata_order.py`'s model: two pairs stay swapped in every order of the tables tried.
- Static data is sorted by size. MWCC lists each object of a section when it is declared, a local struct initializer
  when its function is, and heapsorts the list by size starting from the last object declared. Equal sizes come out
  in no declared order: palanm.c's three 4-byte weights declared R, G, B lie G, R, B (`rodata_order.py --permute`
  searches the orders). Objects of 64 bytes or more, local initializers and globals that no code refers to get sections
  of their own, and the sections are created as the sorted list is walked: the section the other objects share sits where its smallest object comes, so a 12-byte
  initializer goes before a file's 19-byte table unless something smaller is shared. A read of a const global whose
  initializer has been seen is folded and doesn't count as a reference. Taking its address counts, and so does reading
  it before its definition, as an unused inline function in a header does, even though that code is never emitted.
  `demo/shinka_demo_view.h` reconstructs such accessors for the evolution demo's helix constants. Heapsort is not
  stable, so objects of the same size come out in an order that depends on where every object in the file is declared,
  and moving one object can reorder others. `tools/decomp/rodata_order.py` predicts the layout for a declaration order
  and tries the orders of the objects given with `--permute`; `intro_graphic.c` matches only with its light setups
  declared after the function whose BG setups are local initializers. Tables of pointers have to be checked by their
  relocations, and two with the same contents only by the code that loads them: `btlv_scu.c`'s four 12-byte
  encounter step tables, two of them identical, needed their own declaration order, found by trying the orders of
  the four.
- A `static const` table declared inside the one function that reads it is listed where that function is, among
  the local initializers, rather than where file-scope data would be. `pokemontrade_nego.c` lays out its menus' item
  lists in the game's order only with its table of blocking fields declared inside `func_ov194_021bbe60`, and
  `move_handlers.c`'s 226 objects come out as the original's only with its ten value tables, such as
  `FLAIL_POWER_TABLE`, inside the handlers that read them: declared at file scope, they swap thirteen of the
  equal-size handler tables.
- A forward declaration (`static const ItemEventAddEntry sItemEventAddTable[];`) doesn't list an object: it is
  listed where it is defined. Even a table of 64 bytes or more, with a section of its own, takes part in the sort, so
  where it is defined moves the equal-size objects around it. `item_handlers.c` defined its 1376-byte table of
  `EventAdd` functions at the end of the file, after a forward declaration, and 133 of its 171 handler tables came out
  in another order. Defined after the prototypes at the top, where Game Freak's source evidently had it, all of them
  are the original's.
- A struct that is copied from `.rodata` and then has a few fields overwritten with computed values is one local
  initializer with the computed values in its braces. Its template gets a section of its own, while a `static const`
  template assigned to the local compiles to the same code but joins the shared section, so only the module check
  tells them apart. When the copy comes after some calls, the local is declared in a block after them, as the
  projection in `pokemontrade_3d.c`'s `func_ov194_021c1918` is.
- When `rodata_order.py`'s prediction disagrees with the built object, move one declaration at a time, compile, and
  compare the sections with the ROM. `pokemontrade_3d.c` lays out its scene tables in order only with its lights
  declared after the scenes' resource lists, and its cube data only with the vertices declared before the texture coordinates.
- A run of fields that every function reaches from one literal, at fixed offsets, can be several small statics of the
  shared section rather than a struct. The size sort tells them apart: `snd_sys.c`'s `.bss` has a 2-byte mask, six
  4-byte statics and a 24-byte struct (0x34 bytes) before a 40-byte array, which a 0x34-byte struct would follow.
  Folding the array into the struct also showed in the code: `&s.players[i].handle` folds the field's 4 into the
  literal, while the separate array adds it after indexing, as the original does. The six statics' order came from
  compiling every declaration order of them (720), since `rodata_order.py`'s prediction differed from the built object.
- String literals are laid out in `.data` in the order they first appear in the source, each aligned to 4, and an
  identical literal is shared from its first use. An assert's text is the expression as written, spacing included, so
  `GFL_ASSERT(a < (B*C))` needs the game's spacing (`delivery_beacon.c` turns clang-format off for it). In
  `delivery_beacon.c` the game has the asserts' `""` before the file name of an earlier allocation, which no source
  order tried reproduces: a folded assert or an unused inline creates no literal. Overlay 28 shows more of it: in
  `union_main.c` one `""` serves the first two functions' asserts, a second the next twelve, a third the rest, and
  `colosseum.c` has two, each shared across functions, where MWCC gives every file one. What starts a new one is not
  known yet; `GFL_ASSERT(0)` stringifies to `"0"`, which is not one of them.
- String literals are created during code generation, for the references that survive optimization, so they come
  after all of a file's file-scope data, and dead code, unused locals and folded asserts create none. A literal
  laid out earlier than any surviving reference to it points to code the original linker dead-stripped. A file name
  ahead of a table in `.data` is a file-scope array: `script_command.c` declares `static char sFile[] =
  "script_command.c";` and passes `sFile` to its allocations, as `event_research_radar.c` does.
- A global that no code refers to can be declared anywhere in the file, after functions too, and where it sits in
  the heapsort's list moves the local initializers around it: `mus_shot_photo.c` declares `MUS_SHOT_PHOTO_UNUSED`
  just before its last function, which gives the original's `.rodata` with no code change.
- Uninitialized statics take part in the same heapsort as string literals and the other data, and seem to join
  the list after everything else, so their position among the tables doesn't matter but their order among
  themselves can swap strings of equal size: `mbp.c`'s state names come out in the original's order only with
  `sCWork` declared after `childInfo`.
- Small objects that come after a larger one in the same file, out of size order, may be rows of one array: the PC
  box's `box2_ui.c` has seven cursor tables after a 540-byte one, and they are `sTrayCursorData[3][47]`, three rows
  of equal length that each end in a `TOUCH_RECT_END` entry, with other tables pointing into the rows. Once the sizes
  are right, any order of the objects of one size can be reached; keep the natural order elsewhere and let
  `rodata_order.py` solve for the same-size groups.
- The linker starts each `.rodata` section on a 4-byte boundary, whatever the section's own alignment, so a 6-byte
  `u16` table followed by a `u8` initializer leaves 2 bytes of padding between them, as at the start of
  `btl_server_flow.c`'s `.rodata`. `scrcmd_ochiba.c`'s 150-byte and 342-byte tables, both 2-aligned, end where only
  this layout puts them.
- The list that is heapsorted holds the file's `.data` tables as well as its `.rodata` objects, so a `.data` table's
  declaration reorders `.rodata` objects of the same size, and `rodata_order.py` predicts the layout only when it is
  given them too (string literals don't count). `worldtrade_search.c`'s four BG setups come out in the game's order
  only with its touch screen's cursor table, in `.data`, declared after the touch rectangles rather than at the top.
- `static const` data goes in `.rodata`, so a table that the original has in `.data` is not `const`. The module
  check fails if a table ends up in the wrong section, even when every function matches.
- A function-local `static const` is emitted in the shared section even when every read of it is folded into
  immediates, or when nothing reads it, unlike a file-scope one. Small objects that no code refers to at the start of
  a file's shared `.rodata` are such locals: `win_record.c` has its two columns' x (`{ 0, 144 }`) in
  `WinRecord_PrintItemAt` and its BG scroll's speed and wrap (`{ 0x40, 0x2000 }`) in `WinRecord_ScrollBG`, read as
  `add r2, #0x40`, and a 12-byte table that nothing reads, 4-aligned, so it is declared as `u32`s.
- A `static const` variable whose address is never taken is folded into the code and not emitted. If the original has
  it anyway, it is not `static`: a global that no code refers to gets a section of its own, laid out by size with the
  rest. An object laid out ahead of smaller ones is in a file of its own, linked first, as overlay 65's command table
  is in `scrcmd_pokemon_center_table.c`, and one laid out after larger ones is in a file linked after, as overlay 50's
  is in `scrcmd_bsubway_table.c`. The smallest objects come first, so an unreferenced word at a boundary, laid out
  after a file's larger tables, is the next file's first object: the 4 bytes at 0x021a773c in overlay 310 can't end
  `research_graph.c`, whose tables are larger, and as `research_common.c`'s global `ResearchCommon_Unused` they take
  a section of their own ahead of that file's 8-byte table, as the ROM has them.
- `GFL_ASSERT` keeps its expression as a string in `.data`, so the variable it tests keeps its original name, as the
  Medal Rally's `p_sv` does.
- Overlay IDs are linker symbols, written `OVERLAY_ID(279)` from `gfl/overlay.h`, which gives the literal pool entry
  a relocation. Mark the literal in the config with `tools/decomp/config_fixes.py overlay-id`.
- A table one element longer in the ROM than the code needs has a terminator: pml_item.c's `TM_MOVE_LIST` is 101 moves
  and a `MOVE_NONE`. Without it the next object starts 2 bytes early.

- Sections start 4-aligned at link time (`ALIGNALL(4)` in the LCF), whatever their own alignment: pms_data.c's
  12-entry `u16` initializer after 0x16 bytes of shared `.rodata` sits at +0x18, not +0x16.
- The same small table in the `.rodata` of several files of one overlay, each file with its own copy of the same
  inlined code that reads it, is a `static const` in a header with a `static inline` function: overlay 197's
  `mystery.c`, `mystery_album.c` and `mystery_check.c` each have the 15 ribbon parameters and the inlined gift-Pokémon
  routine of `app/mystery/mystery_gift_data.h`.

## When nothing moves it

- An index into a flat array of actors that the source names by ranges is written with the range's enum base even
  when that base is 0: `b_plist_obj.c`'s `BPlistObj_ShowList` matches only as `actors[BPLIST_ACTOR_ITEM + slot]`, since
  the named 0 changes what the compiler reuses, and `u8` locals for its index bases fixed two swapped stack slots in
  `BPlistObj_ShowMoveTypes`.

- Before blaming registers, check every literal argument, mask and field offset against the original: 11 of 18
  leftovers of `btlv_input.c` were a swapped argument pair, a wrong mask width, an 8-byte struct that is 12 in the
  game, a `?:` argument that is two calls, or a missing `case 0: break;`, each read as an allocation difference.
- When the order of instructions differs and no source change moves it, try `tools/decomp/permuter_setup.py`, which
  prepares a function for [decomp-permuter](https://github.com/simonlindholm/decomp-permuter). Its result can point to
  a plain change: `GFL_BGSysAllocChar`'s registers only matched with its tile size, a `u8` from a call, in an `int`.
