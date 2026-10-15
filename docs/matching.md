# How MWCC compiles

What decides whether CodeWarrior's output (`dsi/1.1p1`, Thumb, `-O4,p`) matches the game, learned from functions that
did or didn't match. Each rule names the function that shows it. They are grouped by what a diff shows;
`.claude/skills/match-function/levers.md` indexes the same rules by symptom in one line each. A rule found on one
function and confirmed on another goes here, next to the related rules; see [Decompiling](decompiling.md) for the
tools that show the differences.

## Registers

- An inline function copies its argument into the parameter (`mov r3, r6`) before testing it, where a macro tests
  the loaded value itself and gives each arm of its conditional a register of its own. DWC's `NHTTPi_compareToken`
  (`nhttp_stdlib.c`) matches only with its case folding as a macro (`TO_LOWER`) in the `while` condition, which also
  gives the original's rotated loop. And `return count == 0 ? -1 : value;` lowers `value`'s priority against
  `if (count == 0) value = -1; return value;` (`NHTTPi_strtonum`).
- A static's base reloaded from the literal pool at each use, under `-ipa file`, means every callee-saved register was
  taken. An index local set early (`int index = aid - 1;`) is the usual culprit: multiboot's `MBFrame_Store`
  (`mb_frame.c`) matches only with `aid - 1` written at each of its three uses, which keeps `aid` in r6 and the
  difference late in r7.
- A value passed to a `static inline` stays in a register across the inline's stores, where a field read again at
  each use (`cb->aid`) is reloaded after each store: multiboot's `MBComm_OnWmEvent` (`mb_comm.c`) matches with its
  disconnect step as an inline taking the aid.
- A loop that walks on from a parameter reuses the parameter as its cursor, with the start kept in a local
  (`int start = pos;`), as `b_plist_main.c`'s `BPlistMain_GetNextPos` does; a separate counter swaps the registers.
- A `const` pointer parameter lets MWCC keep a value loaded through it across a store through another pointer; the
  original reloads it, so `btlv_effect.c`'s `BtlvEffTool_Step` takes its step pointer non-const. The same decided
  several vector copies in `btlv_mcss.c` and `btlv_effvm.c`: field-by-field copies from non-const sources.
- Clearing one bit with `flags &= ~(1 << i)` gives `mvns`; the original's `eors` with -1 is `flags &= (1 << i) ^ 0xffffffff`,
  as `btlv_clact.c`'s move and scale task ends do, and `btlv_stage.c`'s vanish flags use `(1 << side) ^ 3`.
  That expression builds the -1 from the shift's 1 (`sub rY, rX, #2`), one register more. Where the original builds
  the -1 on its own (`mov r4, #0; mvn r4, r4` among `mov r3, #1; lsl r3, r2`), the mask was a local set to all ones
  first: `key_data_savedata.c`'s `func_02010304` and `func_02010354` clear with
  `mask = 0xffffffff; mask ^= 1 << bit; keyData->flags &= mask;`, and set with
  `mask = 1; mask <<= bit; keyData->flags |= mask;`, where `|= 1 << bit` adds a register copy.
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
  `BtlMainSeq_Set` inline matches only as `func`, `nextFunc`, `mainModule`, `state`. The written order still sets
  the registers, since it decides which stored values share one and so which load can come first:
  `wipe_sub.c`'s `WipeRectWork_Init` matches only with `work->isOut = param->isOut;` last, after `work->wnd`, right
  before the call that reads `work->isOut` again. In the struct's order the zero and `isOut` share `r0` and the
  window's load can't lead. A `try_variants.py` sweep of all 5040 orders of its seven statements took about 8 minutes
  on 8 copies.
- Independent loads also take registers by their source order. SPL's `SPLDraw_Billboard` (`1.2/base`) reads the
  header's `aspectRatio` and `scaleAnimDir` right after setting `header`, before the alpha arithmetic, which moves the
  resource and header pointers from `r0` to `r4` and `r5`. It also sets the polygon attributes through NitroSDK's
  `G3_PolygonAttr`, whose `int` parameters give the polygon ID and the misc bits other registers and swap an `orr`'s
  operands, where `reg_G3_POLYGON_ATTR = GX_PACK_POLYGONATTR_PARAM(...)` does not. Neither change helped alone (the
  inline was worse in the old order), so try an SDK inline against its macro again after each reorder.
- A parameter the original copies to a saved register at entry and changes there in place is walked by the source
  itself, not copied to a local: `arrow.c`'s `Arrow_SetPathCore` steps `startX -= PIECE_SIZE` rather than an
  `x = startX - PIECE_SIZE`. A parameter copied to another register at entry (`mov r3, r1`), with a value derived
  from it in a register the parameter never had (`lsr r0, r3, #0x3`), was reassigned too, which starts a variable of
  its own: `areaman.c`'s `GFL_AreaManDeAlloc` and `GFL_AreaManSetBitsCore` match only with `pos /= 8;` and
  `man->bits[pos++]`, not a `u32 byte = pos / 8;`, and `GFL_AreaManSetBitsCore` subtracts in its parameter's register
  as `size -= first;`, not into a `rest` local.
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
- A pointer that must have the earlier declaration's slot but a later store is declared without an initializer and
  assigned after the other pointers. This matches the item-base spills in `Aoss_ParseWep` and `Aoss_ParsePsk` in
  `dwc_aoss.c`.
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
- The same holds for an update written in both branches of an `if`/`else`: MWCC merges the two into one instruction
  at the join, but allocates as if they were separate assignments. `ssp_jpegdec.c`'s `func_ov257_021ab808` keeps
  `nbits` in r6 and the loop counter in r7 only with `nbits--;` at the end of each branch; one `nbits--;` after the
  `if`/`else` swaps them.
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
  A counter of its own also moves registers: `pos_handlers.c`'s `HandlerPosLunarDance` gives its PP loop `j` and its
  check loop `i`; with `i` in both, the counters and work pointers take `r4` and `monId` `r5`, the reverse.
  `poke_regulation.c`'s `checkPicked` swaps its last loop's counter and `requiredForm` between `r2` and `r3` with
  either earlier loop's counter, and matches once that loop counts with a `k` of its own, wherever `k` is declared.
- A pair of values computed twice and spilled to two different slots is two pairs of variables, and the second pair
  repeats the first's declaration order. `gimmick_passing_vehicles.c`'s `func_ov128_021eeec8` takes the sine and
  cosine of 90 and then of 270 degrees: one reused `cos, sin` pair gave the second cosine `r7` and spilled the sine,
  `fx32 cos; fx32 sin; fx32 cos2; fx32 sin2;` matches, and `sin, cos, sin2, cos2` is further off.
- Variables declared in an inner block are allocated apart from the function's variables of the same name: in
  `ShinkaDemoPieces_Move`, the branch that moves a piece home declares its own `dx` and `dz`, which live on the stack
  while the other branches keep theirs in registers.
- Two values that the original keeps in one register, one dead before the other is set, were one variable: the PC
  box's `func_ov255_021cdcc8` keeps the party position and then its result in the same variable, where two
  variables get two registers.
- A value tested before a branch and the value a loop in that branch works out can be one variable even when the
  two end up in two registers, and that split frees a register for a pointer that both branches share. Overlay 30's
  `WH_StateInStartScan` (`wih.c`) keeps its `_pWmInfo` load in `r4`, ahead of the loop variables, only with one
  `u32 channel`: `channel = _pWmInfo->sScanChannel; if (channel == 0) { ...; channel = i + 1; ... } else { ... }`,
  the `else` using `1 << (channel - 1)`. MWCC splits it into the field's value in `r2` and the found channel in
  `r5`. A search `channel` of its own, with the field read again in the `else`, gave the loops that register in every
  declaration order, width and inline helper tried. An explicit `WIH *info = _pWmInfo;` is a plain copy that MWCC
  removes (it is absent from the `-g` DWARF), so it moves nothing.
- A loop condition written with a local for its row start, `start = pos + j * 6; if (x >= start && x < w + start)`,
  allocates registers differently from the same sums written in both comparisons: the PC box's `func_ov255_021d229c`
  only matched with the sums written out.
- A parameter that the callers narrow with shifts before the call is a `u16` or `u8`, and the type also decides how it
  is spilled: `BagItemList_GetItem` spills `pocket` first and compares the reloaded copy only once `pocket` and `index`
  are `u16`, as `itemmenu.c`'s caller narrows them; as `u32` it compares a register copy and stores it after.
  The same holds with no narrowing caller to show it: `condition_effects.c`'s `Condition_SetCureMessage` matches
  only with its fourth parameter `u16` (or `u8`), and its one caller passes 0.
- Loops over an array of structs that test two fields through a pointer to the element, then write the fields as
  `list->entries[i].x` in the body, keep the array's base in a register and the element's address in another, as
  `bag_item.c`'s `BagItemList_Remove` and `BagItemList_GetItem` do; indexing in the test too folds the field offsets.
- A pointer to an array slot can give a loop's variables back the original's registers. `btlv_scu.c`'s
  `func_ov167_021d2820` stores a call's result to `work->mons[i]`, tests it for `NULL` and passes it on twice; written
  through `BattleMon **slot = &work->mons[i];`, assigned just before the store, the work, `&balls` and `i` go to `r4`,
  `r5` and `r6` and `i * 4` is reloaded from its slot, as in the original. Indexing at each use swaps `r4` and `r6`,
  and assigning `slot` at the top of the loop body leaves 16 bytes off. Its sibling `func_ov167_021d2b88`, the same
  loop without the `NULL` test, matches with plain indexing.
- A loop-invariant field read through a `const` pointer parameter, which the original keeps in `ip`, `r7` or a stack
  slot across the loop, can be a load MWCC hoisted itself. Written as a local before the loop it keeps the same
  copies but rotates the registers of the loop's own locals and swaps the order the parameters are spilled in:
  `irc_sync_check.c`'s `IrcSyncCheck_CompareTraces` matches with `own[i].x - ownStart->x` in the loop, and was 52
  bytes off with `int ownStartX = ownStart[0].x;` above it.
- A variable assigned in a branch before that branch's call is live across the call and takes a callee-saved
  register in every branch; assigned after the call it sits in `r0` per branch and is copied at the join.
  `fest_mission_sparkle_place.c`'s `FestSparklePlace_AddZone` has `index = 5;` before its `sys_memcpy`
  (`movs r6, #5; blx sys_memcpy`), which also puts `index * 12` in `r7`.
- A struct copied as a loop of halfwords (`movs r1, #0xa; ldrh; strh; subs; bne`) is a 2-aligned type, with no member
  wider than 2 bytes. `ZoneBGEntity` holds its `s32` grid coordinates as `u16 grid[6]`, read through a cast, which is
  also why overlay 12's readers only matched through pointer locals; with it `FestSparklePlace_PickPlaces` and
  `FesMissionPeople_PickPlaces` copy their entities as the original does.
- A scalar declared before a local array with an initializer is allocated ahead of the initializer's copy and the
  parameters' copies; declared after it, it takes the register right after the counter. `zukan_area_list.c`'s
  `ZukanAreaList_GetMarkKind` has `u8 next;` and its enum counter `i` ahead of `u32 kinds[3] = { ... }`, which puts
  `next` in `r3` and the loop's temporary in `r1`. The counter is initialized from `next` (`for (i = next; ...)`), and
  the test before the first pass, which an enum counter keeps, is on `next`'s register.
- A subtraction feeding a multiply, computed into its second operand's register and then copied to `r0`
  (`subs r1, r1, r0; adds r0, r1, #0; muls r0, r5`) with the first operand spilled after the `subs`, is a named
  local: `sp_poke_event.c`'s inline `SpPokeEvent_Lerp` is `fx32 d = b - a; return a + d * step / steps;`. The unnamed
  `a + (b - a) * step / steps` spills `a` first and subtracts into `r0`, 2 bytes shorter at each use.
- A `u16` built from a call's result and then updated in place, `entry = BmpWin_GetCharPos(win); entry |= 15 << 12;`,
  stays in the call's `r0` through its narrowing, and the loop counter's zero takes `r1`; the single expression
  `entry = BmpWin_GetCharPos(win) | (15 << 12);` moves it to `r1` and the zero to `r0`
  (`chihou_zukan_award.c`'s `ChihouZukanAward_InitBackdrop`). A shift written in place,
  `sample = nibble - 8; sample <<= 4;`, likewise keeps the `s8` value and its shift in one register, where
  `perapvoice.c`'s `PerapVoice_Decode` shifts into a new one (`lsl r5, r5, #0x1c; asr r7, r5, #0x18`). There the
  shift is an expression: an inline returning `sample << 4` from its own `s8 sample = nibble - 8;`. The same inline
  written `sample <<= 4; return sample;` does not match.
- A flag's zero built in the entry block, above the first call, does not mean the flag is declared at the top:
  a `BOOL isNew = FALSE;` declared inside a later block is hoisted there too, and the block is what decides its
  register. `irc_compatible.c`'s `IrcCompatible_CreateParam3` declares it in its `if` block after `isNewTrainer`,
  which puts it in `r6`; every order at the top of the function kept two registers swapped.
- An argument copied inside each branch (`mov r7, r1` where `r1` is already 1, `mov r1, r7` in the other arm) while
  a zero stays in `r7` from the top is two variables holding the same value: `irc_appbar.c`'s `IrcAppBar_Create`
  has `BOOL isSub = FALSE;` set in the branch and a `vramType` assigned in both arms. One `vramType = 0` with
  `vramType = 1` in the branch copies after the join and swaps `r6` and `r7`.
- Two computations of the same shape, one for x and one for y, that alternate registers (`r6` then `r7`) reuse one
  pair of locals; a pair each keeps the same registers both times (`irc_compat_menu.c`'s
  `IrcCompatMenuHeart_Update`, one `diff` and `sign` for both).
- An array element read as `buttons->defs[buttons->touched].x` several times copies the index, multiplies it in
  place and loads the base last (`adds r1, r2, #0; muls; ldr; adds r0, r0, r1`); through a
  `const Def *def = &buttons->defs[i];` local the base is loaded first and the product goes to the constant's
  register. `IrcCompatMenuButtons_Update` is the first form and its sibling `IrcCompatMenuButtons_Reset` the second.
- An integer `+` of a local and a temporary puts the local first whatever the source order, and a pointer `+` puts
  the pointer first. `adds r1, r7, r1` with the size in `r7` and the running address in `r1` is therefore integer
  arithmetic on the address: `musical_system.c`'s `MusicalSystem_LoadProgramData` sets its second and third sound
  pointers with `(void *)((u32)work->sound[0] + size0)` and `(void *)((u32)work->sound[0] + size0 + size1)`, the
  second reusing the first's sum, as `btlv_gauge.c` writes its offsets. `(u8 *)work->sound[1] + size1` gives
  `adds r1, r1, r7`.
- A callee whose callers disagree on narrowing only shows once all callers are in view: `wifi_p2pmatch.c`'s
  `WifiP2PMatch_IconWrite` and its sibling take `u8 x, u8 y`, which one caller needed and the others hid. Compare
  every caller's narrowing with the definition before trusting a prototype written from one of them.
- A 32-bit parameter passed on the stack is only ever reloaded from its stack home; its copy in a local of the other
  signedness is a new value and gets a saved register. The Pal Pad's `WifiNoteListArea_DrawFriend` takes
  `s32 friendPos` as a stack argument and starts with `u32 pos = friendPos;`: `pos` sits in `r4`
  (`ldr r4, [sp, #0x50]`), which spills the screens pointer, the name and `pos * 4` as in the ROM (frame 0x38, 0x110
  bytes). A `u32` or `int` parameter used directly, or a copy of the same type, gives 0x10c. A cast at the first use
  (`area->chars[(u32)pos]`) does the same as the local.
  Overlay 311's `UnSelectMsg_PrintItem` matches the same way, with `int pos = index;` from a `u32 index`: `pos` takes
  `r7` and pushes `floor = 131 - pos` to the stack. There a spilled local's store also follows the declaration
  order: `floor` declared after `u32 y = 12` stores its spill after `y`'s constant is built, as in the original,
  and declared first stores it right after the subtraction.
- In NitroSystem's 2.0/sp2p2 code, a `for` loop's comma-separated increment keeps its last variable in a register for
  the test and stores the earlier ones, and the `for` initializer's order sets the spill slots of the start values:
  `g2d_CharCanvas.c`'s `ClearArea1D` matches with `for (; cy < y1; charY++, cy += 8)` and `for (charX = charX0, cx =
  x0; cx < x1; charX++, cx += 8)`, and `DrawGlyph1D` with `cy++, y -= 8`. There too a load written before a group of
  temporaries holds its low register and keeps the scheduler from interleaving them (`objParam.param = pCC->param;`
  first in `ClearArea1D`), and a `const u8` local for a `u8` field picks other registers than an `int`
  (`DrawGlyph1D`'s glyph width).
- An inline's enum-typed parameter changes how MWCC orders and allocates the inlined expression when the argument
  is an integer: `billboard.c`'s `BlActScene_Draw` packs its polygon attributes in the original's order (alpha,
  then the polygon ID, then the light mask, `const | lights`) only with `G3_PolygonAttr` taking `int light`, as
  NitroSDK declares it, not `GXLightMask light`; casts or enum locals at the call don't help. Check the SDK's
  prototype of an inline before the call site.
- In NitroSystem's 2.0/sp2p2 code a pointer sum `a + n - 1` returned from a `static inline` loads `n` first, where
  written in place it loads `a` first (`g2d_Animation.c`'s `SequenceEdgeHandle_` clamps through
  `GetLastFrame_(seq)`). Spilled locals take slots from the top of the frame down: the function's locals, then an
  `if` block's, then its `else` block's, each in declaration order, then an inline's in reverse, so a slot order no
  block layout gives needs the locals at function scope (`NNS_G2dInitMCAnimationInstance`). A second loop that
  reuses the first loop's counter swaps registers with its block's temporary, where a counter of its own declared
  after the others doesn't (`gfd_LinkedListTexVramMan.c`'s `NNS_GfdResetLnkTexVramState`).
- A loop offset that starts at 0, grows by a step and is zeroed after the loop's hoisted invariants is a counter
  times the step: MWCC strength-reduces `x + step * line` into a running offset that it zeroes in the loop's
  preheader, where an offset kept by hand is zeroed before the hoisted code. NitroSystem's `g2d_TextCanvas.c`
  `DrawTextAligned` matches with `int line = 0;`, `lineX = x + lfx * line;` and `line++`.
- A table indexed by a field, with the base in the higher register and the index's product added into the lower
  one where the original does the reverse (`ldr r4, =table; muls; adds r4, r4, r2`), is the table indexed inside a
  `static inline` whose index parameter is an enum type. NitroSystem's `g2d_OamManager.c` reads its buffers through
  `GetOamBuffer_(NNSG2dOamType type)` returning `&oamBuffers_[type]`, which matches `NNS_G2dEntryOamManagerAffine`,
  `NNS_G2dEntryOamManagerOamWithAffineIdx` and both branches of `NNS_G2dResetOamManagerBuffer`. The same helper
  with a `u32` or `int` parameter, a cast to the enum, or an enum-typed field alone keeps the swap.
- A narrow stack parameter used twice is read from its slot at each use, and declared `const` it is loaded once at
  entry into a saved register: `g3d_system.c`'s `GFL_G3DSysCreate` takes `const HeapID heapId`, loaded into `r7`
  at entry (`add r0, sp, #0x68; ldrh r7, [r0, #4]`), which moves MWCC's cached `&g_G3DSystem` out of `r7` and
  gives every register the original's. A plain copy of the same type folds back into the parameter; a `const`
  copy, or a copy of another width, works only when both uses go through it. The prototype in the header can stay
  without the `const`, which isn't part of the function's type.
- A nested loop whose counter and hoisted row pointer sit in each other's registers may reuse an earlier loop's
  counter. The Pokéstar record monitor's `PkwdRecordMonitor_LoadBG` runs three `for (i = 0; ...)` loops over its load
  tables and then adds a palette to three screen pages. With counters of their own (`page`, `y`, `x`) the row counter
  got `r2` and `&buffer[page * 0x400 + y * 32]` got `r3`, through 48 declaration orders. With `i` as the row counter
  (`s32 i; s32 page; s32 j;` and `buffer[page * 0x400 + i * 32 + j] += 6 << 12;`) they swap. `i` as the page or the
  column counter does not match.
- Two offsets that a loop steps alike can swap registers by their signedness alone. `bmp_oam.c`'s
  `BmpOam_ActorBmpTrans` steps its cells' upper and lower row offsets by `size` and then by `tilesWide * 32` per row:
  as `u32` the one assigned first takes `r7` in every declaration and assignment order, and as `int` the lower one
  takes it, as in the original. The sign is the invariant: the original computes `tilesWide * 32` into the variable's
  register before the loop's guard and saves a copy of it in the preheader (`lsl r7, r4, #0x5` ...
  `str r7, [sp, #0x4]`), where ours computes the invariant first and makes the variable a copy of it.
- An `if`/`else` that stores a constant to the same field of a static struct in each arm, on a condition that is a
  call, loads the struct's address once before the call into the first saved register, ahead of the parameter's copy,
  and the stores after it reuse it. The Wi-Fi login's `WifiLogin_StateStartNet` starts with
  `if (hw_isDSi()) { sNetInit.wifiHeapSize = 0x79000; } else { sNetInit.wifiHeapSize = 0x5e400; }` and then fills
  `sNetInit.` directly. A ternary store loads the address late into a scratch register, and a
  `GFLNetInitData *init = &sNetInit;` local swaps `r4` and `r5`.
- Constants hoisted into saved registers across a run of `f(g(x, N), FALSE)` calls (`movs r1, #0; movs r4, #0` at the
  first call, `adds r1, r4, #0` at the last ones) come from the inner call's result passing through one local that
  every statement reassigns: `actor = GetActor(...); func_0204c124(actor, FALSE);`, as the habitat list's
  `ZukanHabitat_HideListButtons` has it. Written nested, every constant stays a literal (4 bytes short there);
  constant locals, inline helpers and loops did not reproduce it.
- A register argument or a loop counter that takes its 0 from another variable's stack slot
  (`ldr r1, [sp, #0x20]` for `sys_memset`'s 0, and for `i = 0`) where the original has `movs r1, #0`: an earlier
  statement nested a call in another call's arguments. `WbtDownloadNetBG_Load` wrote
  `GFL_G2DIOLoadNSCRSync(arc, 6, 4, 0, (u16)GFL_BGSysLoadArcNCGRDynamic(...), 0, FALSE, heapId);` three times; with
  the inner call's result in a local first (`chars = GFL_BGSysLoadArcNCGRDynamic(...);` then the call with
  `(u16)chars`) the zeros stored for the stack arguments still come from the hoisted `count = 0` slot, as in the
  original, but the later `sys_memset(…, 0, …)` and `for (i = 0; …)` of the inlined code build their own. Where
  `count` is declared or set, its type, and the shape of the inline changed nothing.
- A store written in each branch of an `if` is the extra use that wins a register for a member-pointer local. The
  battle party list's `BPlistObj_LoadPokeIconRes` has `u32 *res = wk->charRes;` before its loop and writes
  `res[i] = func_0204b81c(...)` in both branches, with `wk->pokemon[i]` indexed at each use. With one store after
  the branches the code has the right size but the file handle and `res` are swapped in `r6`/`r7`, through every
  declaration order.
- A bit field argument extracted completely before the first argument is set up (`ldrh r0; lsls r0, #0x11; lsrs r1,
  r0, #0x19; adds r0, r5, #0`, against ours `ldrh r1; adds r0, r5, #0; lsls r1; lsrs r1`) is the enum-local rule:
  `PkwdBookSeries series; ... series = wk->movies[wk->selected].series; PkwdBook_TransferSeriesPalette(wk, series);`
  in `PkwdBook_ListInput`. An integer local of any width, a reused variable, inline getters and wrappers gave the
  same bytes as the direct argument; `PkwdBook_InitScreen` passes the field directly and has the other order.
- A constant assigned once and passed to a call is propagated as an immediate; assigned in both arms of an `if`,
  next to the values that do differ, it stays in a saved register. The Pokéstar counter's `PkwdCount_StartFade` sets
  `delay = 4;` beside `start` and `end` in each arm (`r6`, 0x5c); with one assignment it was 0x58.
- A sign extension split between the prologue and a later compare is a narrow local declared first and assigned late
  from an `int` parameter: the film choice screen's `BChoiceMain_SetChoicePalette` takes `int palette` and has
  `s16 pal;` declared before its other locals and assigned after their initializers (4 bytes short before). In the
  same file `BChoiceMain_UpdateCursor` passes `(u32)index * 2` for x and `index * 2 + 1` for y, the mixed signedness
  of `PkwdRecordMonitor_UpdateListMarks`, and `BChoiceMain_CreateActors` declares
  `int i` first but zeroes it in a plain `for` after the `sys_memset`: the stack slot follows the declaration, the
  store the statement.
- A result variable that shares a saved register with another local across a call must not be assigned before that
  call: the compatibility check's `IrcCompatResult_GiveReward` sets `result` in an `if`/`else` on
  `BagSave_AddItem`'s return, so `bag` and `result` both live in `r6` (76 bytes off with `result` set first). A
  `mov r7, #1` that comes after the tested call there is `getTrainerGender(info) != 0 ? 0 : 1` in the flag's
  declaration.
- `x += f();` and `x = x + f();` on an `s16` local are not the same to the register allocator: Castelia's crowd
  `Passerby_StepAside` had `passerby` and `dir` swapped in `r4`/`r5` through every declaration order until
  `x = x + GetDirectionVectorCompX(dir);` became `x += ...` (and the same for `z`).
- A byte field passed straight to a call makes MWCC load argument 0 first and then copy the base pointer out of `r0`.
  Where the original loads the fields first (`ldrb r1, [r0, #1]; ldrb r2, [r0]; mov r0, r4`), they went through
  enum-typed locals: Opelucid Gym's `GymOpelucid_ShowState`, `RefreshVars`, `MoveState` and `CreateEvent` copy
  `tier` and `column` into `OpelucidGymTier` and `OpelucidGymColumn` locals before the call. Parameter types,
  integer locals, inline wrappers and `const` did not change it. The callee keeps `int` parameters: with enum
  parameters the locals are propagated again.
- A store of 0 is not always emitted where it is written: MWCC can sink it below later stores, and only the place
  where the zero is built shows where the statement was. In the Spin Trade's `Guru2Receipt_UpdatePalAnim`
  (`anim->counter = 0;` before `anim->index++;`) and `Guru2Main_InitDisc` (`disc->angle = 0;` before
  `disc->angleOffset = ...`) moving the zero store up fixed a register swap. A `static const` table's read is reused
  across a call, but a read through a pointer to its row is loaded again: `Guru2MainSeq_Bonus` reads the clamp's
  first Berry through `const u32 *range = sBonusItems[count];`. The tell is a function that matches when the table is
  made non-`const` although it sits in `.rodata`.
- A copy's size computed by a call and held in a local before the `sys_memcpy` or `sys_memset` is scheduled
  differently from the call written as the size argument: the musical's `MusComm_SendAllPokes`,
  `MusComm_RecvAllPokes`, `MusComm_RecvAllPlayerData` and `func_ov211_021ef3e4` all matched with
  `u32 size = ...GetSize();` first. In `MusComm_Main`, `requesters[count] = i; count++;` in place of
  `requesters[count++] = i;` and `requests += chosen << 16;` as its own statement before the call were each needed.
- A struct field that is a pointer to `const` explains several users at once: loop bounds read through it are loaded
  again on each pass, and a `u64` copied through it is `ldr, ldr, str, str`. The World Tournament's
  `WbtMakeMatch.param` as `const WbtMatchParam *` matched `WbtMakeMatch_Create`, `CountCandidates` and
  `FindCandidate`. A callee's parameter type can also reorder its caller's stack-argument stores while the callee's
  own code is the same either way (`WbtEntrant_SetTableForDownloaded`'s `u8 index` matched `WbtMakeMatch_SetEntrants`):
  sweep callee types when only the argument order differs.
- A `const` by-value parameter stays in a register although its address is passed on: the musical stage's
  `StaActPoke_SetShowFlg` has `const BOOL show` in its definition. In the same overlay `a = x; a += y;` fixed a load
  order that `a = x + y` leaves to MWCC (`charAddr += index << 11` in `StaActButton_LoadItemTexture`), `u32`
  per-case switch locals took their declared slots where `u8` ones sank to the bottom, and
  `StaActButton_InitSystem`'s "wrong register" was a wrong constant (`GX_WNDMASK_W0`, not `OW`): check the values
  before the registers.
- `s8 -= u8` loads the right side with `ldrsb`, and `step *= -1` on an `s8` gives `mvns; muls`: the frame list's
  `FrameListLayout.speed` is `u8[6]` (`HandleEvent`, `UpdateBar`), and `ScrollToTouched` and `StartFlick` negate
  their `s8 step` by multiplying. `y -= rowHeight` and `list->cursor--` matched where `y = y - rowHeight` and
  `cursor = cursor - 1` did not (`FrameList_Create`, `DrawAll`, `KeyEvent`), and the order of declaration
  initializers places a spilled `= 0`: `TouchBar_Create` has `size`, then `bar = GFL_HeapAllocate(...)`, then
  `sub = 0` as initializers.
- A callee's narrow parameter can be what keeps its callers from matching: the dress-up's `DupFitting_SortEntries`
  and `DupFitting_SelectCategory` matched once the item checks they call (`MusItemDraw_CanEquipPos` and its two
  siblings in overlay 209, three getters in overlay 210) take `s32 pos` in place of `u8 pos`, with the callees
  themselves matching either way. In the same file the raw display-register masks of `DupFitting_InitGraphics` were
  `GX_SetVisiblePlane(0); GXS_SetVisiblePlane(0);` (the SDK inline's parameter is what holds the zero in a
  register), and `(s32)work->badgeFrame < 45` gives the signed `bge` a `u16` field is otherwise compared without.
- A spilled `u8` that is reloaded and narrowed at each use can come from a later callee's `u8` parameter, not from
  the local's own type: the Dream World postman's `func_ov033_02177d78` matched once its callee
  `func_ov033_021780a4` took `u8 kind` in place of `u32`.
- Whether an argument is loaded with `ldrh` or `ldrsh` follows the callee's parameter type, not the field's: the
  Funfest's `FesMissionPeople_PlaceActors` matched once `func_ov012_021682d4` took `u16 sidePos`. Consecutive
  halfword fields loaded as `adds r3, r4, #0; adds r3, #8; ldrh [r3, #2]; ldrh [r3, #4]` are read through a pointer
  to the first of them (`const u16 *values = &place->pos.rail.componentId;` then `values[0]`, `values[1]`,
  `values[2]`).
- Whether a constant shared by two uses is kept in a register or rebuilt can depend on how many temporaries the
  function has, not on the constant: the resort's `func_ov059_021e6630` keeps its zero in `r5` once three of its
  nested call results are held in locals (`field`, `people`, `person`), and no spelling of the zero itself does it.
  A pool word that differs between the versions by a table row is a table indexed by the version:
  `sUnk7db4[VERSION_BLACK2 - GAME_VERSION][value]` in the same file folds to the table's address in Black 2 and
  to the address + 0x10 in White 2. Check the pool words of both versions before linking a file.
- **MWCC propagates a hoisted constant as an immediate only within a budget of about 100 IR nodes from the start of
  the function.** A constant that needs a register (for a stack argument, or because it is used often) is written
  as an immediate at each use up to that edge, and as a copy of the register (`mov r3, r7`) at every use past it.
  The edge is counted in IR nodes, not instructions, so a respelling that compiles to the same bytes moves it: a
  one-argument call is about 3 nodes, a call without arguments 1, a parameter as an argument 1, a field load 1, a
  `u16` narrowing 1; casts of constants and dead code are free. The badge gate's `func_ov103_021ef630` (16 bytes
  off, immediates where the ROM copies) matched when a call's result went through a local
  (`matrix = FieldExpObj_GetActorMatrixPtr(...); matrix->translation = ...;`), which is the same code for that
  statement and three nodes more before the edge. So when the only difference is which uses of a constant are
  immediates and which are register copies, look for a statement before the first copy that the original wrote with
  one more (or one fewer) named step, and measure by padding rather than counting. This also explains "holding three
  nested call results in locals keeps the zero in a register" (`func_ov059_021e6630`).
- A reload of a spilled parameter that trades places with a constant, at the same size, can be a free width
  conversion to a wider callee parameter: the GTS Negotiations screen's `GtsNegoDisp_DimPalettes` passed a `u16 vram`
  to `PaletteFade_BlendBuffer`'s `u32 buffer`, which emits nothing but moves the reload. The palette fade's five
  buffer functions that took `u16` take `u32`. A twin function that matches either way does not vouch for the
  prototype, since it kept the value in a register. To test a prototype without editing a shared header, compile
  against a copy with the same include guard (`--extra-flags "-i DIR -prefix copy.h"`).
- An array zeroed where it is declared in an inner block (`u8 list[N] = { 0 };` inside the block that fills it) is
  laid out and cleared differently from one declared at the top of the function and cleared by a call or a loop:
  overlay 12's `func_ov012_0215168c`, `func_ov012_02159f90` and `func_ov012_021522d8` matched that way. A constant
  the retail game fixes but the code still tests or passes as a variable is held in an enum-typed local, which MWCC
  does not fold (`FieldScript_LoadData`'s source, `CallTrialHouseBattle`'s battle type,
  `s01D6_MoveReminderCallMoveSelect`'s mode).

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
- A loop counter that the original keeps in a stack slot in a small function may be assigned before a call: its value
  then lives across the call, so it can't stay in a scratch register. `ssp_jpegenc.c`'s `JpegEnc_SetupHuffmanTable`
  matches only with `i = 0;` before its `sys_memset32_fast` call and the loop written `while (i < 16)` after it, with
  the table's terminator stored inside the outer loop.
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
  The EXIF reader's `func_ov257_021ac6f0` (`ssp_exifdec.c`) reads the byte order and then every IFD entry's tag into
  one function-level `tag`; with a block-scoped `tag` in each loop and an `order` of its own, two slots were swapped.
  Once the set of variables changes, sweep the declaration orders again: there, moving `makerNote` after `exifIfd`
  then fixed the registers, though no position of it had helped before.
- In a chained assignment, `a = b = f();`, MWCC treats `a` as its own copy of `b`, and a spilled `a` takes the lowest
  slot instead of its declared one. `StatusRcv_UseItem` needs `status = f(); newStatus = status;`.
  A local assigned with a ternary is spilled among the compiler's temporaries too, in the lowest slot by first
  assignment, where `x = a; if (c) { x = b; }` keeps its declared slot: `mcss.c`'s `MCSSSys_Draw` keeps `ortho` in the
  lowest slot, below its saved sine and cosine, with `ortho = system->ortho ? system->mcss[i]->ortho : FALSE;`, which
  no declaration order did.
- Named locals take stack slots apart from the compiler's temporaries. When the original's spilled values all sit in
  the order they are first assigned, they may be common subexpressions: the trade's `func_ov194_021c1530` reads
  `colors[side * 2]` at each use, and `int color = colors[side * 2];` moved it above the temporaries.
  An inline's parameter is a named local: `key.c`'s `keypressInterpreter` interleaves the slots of its three key
  values and three `remap->to` loads in assignment order only when its inline `SwapRemappedKeys` takes
  `const u32 *keys` and reads `*keys` at each use. With `u32 keys` by value, the keys took three slots apart from the
  others.
- A common subexpression's spilled temporary takes its slot by where its first use is written. A local holding it
  doesn't count, since MWCC propagates `end = offset + size;` into its uses whatever its type or declaration.
  `backup_system.c`'s `func_0203b170` puts `offset + size` at the top of its spilled values (`sp+0x20`) only with
  `dataFooter = (BlockFooter *)(data + end);` written first of its three footer pointers, before a call and a `break`
  test; written after them, the value took the lowest slot (`sp+4`). Every named slot then moved down by 4, and `data`
  declared after the loop counter took the freed lowest one.
- One counter for two loops in a row keeps one slot. `func_ov194_021c12ec` has a loop over `i`, then two nested loops.
  MWCC keeps the `0` that the first loop passes as arguments in the stack slot of the variable it starts the outer
  nested loop with, and in the original that slot is the lowest: both loops count with `i`, the inner one with `j`.
  A separate `side` for the outer loop took the highest slot instead.
- A NULL check written on a field, `if (bgs[bg].screen != NULL) { void *screen = bgs[bg].screen; ... }`, gives
  different stack slots from the same check on a local loaded before it, as `GFL_BGSysLoadScrCore` shows.
- MWCC reuses a field it has loaded, across the 64-bit multiply helpers, so a value that the original keeps on the
  stack between `piece->home.x - piece->pos.x` and `piece->pos.x = piece->home.x` is MWCC's own copy, not a local.
  Writing it as a local changes which stack slots everything gets.
- A sum of 64-bit products written as one expression, `(fx64)x * x + (fx64)y * y + (fx64)z * z`, and the same sum
  built up with `+=`, `fx64 sum = (fx64)v->x * v->x; sum += (fx64)v->y * v->y; sum += ...`, compile to the same
  `smull`/`smlal` chain with different registers, and only the one expression merges the loads of neighbouring fields
  into an `ldm`. libfx's `VEC_Mag`, `VEC_Fx16Mag`, `VEC_Distance` and `VEC_Normalize` match only with `+=`; a rounding
  constant added in 32 bits to the last term (`sum += a->z * b->z + 0x800`) is what `VEC_Fx16DotProduct` needs.
- A test of a field right after a store into it reuses the stored register only when the source tests the field
  written to: `tr_tool.c`'s `TrainerUtil_LoadTrainer` tests `trainer->trainerClass` after storing `data->trainerClass`
  there, not `data->trainerClass`.
- The copy of a multiply's operand placed after the constant is loaded (`subs r2, r5, #1; movs r0, #0xc; adds r1, r2,
  #0; muls r1, r0`), where ours copies right after the `subs`, is an index in a variable of its own with the
  elements indexed in place. Overlay 137's `ResortSys_GetRecordRank` sorts with `s32 prev = j - 1;` and
  `entries[prev]`/`entries[j]`; `upper`/`lower` pointer locals, with or without the variable, copy early, and
  `j - 1` written at each use makes MWCC rebuild one pointer from the other. `#pragma optimization_level 1` moves
  the copy too, a sign that the CSE and copy propagation passes place it.
- A local of an inlined helper is slotted after the function's optimizer temporaries, where the same local in the
  function's body is slotted among its named locals: overlay 197's `MysteryCardView_DrawCard` gets its archive
  handle the original's slot, above the card and `iconCellAnims` temporaries, only with the icon loading in a
  `static inline MysteryCardView_LoadIcon` that declares `arc` itself. A call's result held in a local before it
  is passed on (`date = MysteryCard_GetDateBitmap(card); GFL_BitmapCopy(date, ...)`) also moves the next
  argument's common subexpressions after the call, where the nested call computes them before it.
- A constant held in a stack slot and loaded at each use, where ours builds it with `movs` and keeps a copy in a
  saved register, can be an inlined helper's parameter passed that constant: MWCC sets the parameter as a copy of
  a register holding the value and spills it, then loads the value from its slot where the code needs it. Overlay
  123's `func_ov123_021eed54` matches with its group loop in `GrimsleyRoom_InitGroupModels(work, 0)`, which counts
  with its own `u8 index` parameter (`for (; index < GROUP_MODEL_COUNT; index++)`): the earlier loop then loads its
  zeros from that slot and keeps the 1 for `SetPaused` in `r7`, as the original does. A local counter set from a
  parameter (`i = first`) doesn't.
- MWCC derives a large constant offset from one built earlier (`subs r6, #0x80` for 0x1cc from 0x24c) across calls
  and the fall-through of conditional branches, whatever pointer it is added to, and stops only at a join: a label
  reached from two places, as after an `if` without `else`, an `if`/`else`, `||` or an inline with two returns. Empty
  `if`s, dead stores and one-pass loops are removed before that and don't stop it. Where the original builds an
  offset fresh (`movs r0, #0x73; lsls`), look for such a join just before it: overlay 173's
  `WifiP2PMatch_StateRecruitingFriendMenu` gets its voice-chat store in the common tail with the next state set in
  both branches and stored once after them.
- A local assigned straight from a `static inline`'s return is copy-propagated into the inline's result temporary,
  which is slotted by first use after the named locals, while the same value computed in place stays a named local
  slotted by its declaration. Spill slots that look grouped by kind across branches can be this: overlay 284's
  `ShinkaDemoPieces_Move` gets the original's slots only with each branch's `fx32 distSqXZ` block-scoped and set as
  `FX_Mul(x, x) + FX_Mul(z, z)` written out; through the file's `SquaredLengthXZ` inline they are temporaries.
  Only when the assignment dominates every use, though: overlay 143's `MonolithMissionInfo_DrawRecords` gets `best`
  slotted as a temporary through a pass-through inline getter, but `count`, assigned in one branch of an `if`/`else`,
  keeps its declared slot whatever its type.
- A narrow local passed to several calls becomes a compiler temporary, slotted by its first use after the named
  locals, only when every parameter that receives it has the same type; otherwise it keeps its declared slot.
  Overlay 102's `RollerCoaster_Task` passes `u8 anm = path + 1` to two calls, and its slot matches only with
  `func_ov036_021b8520` taking `u16 anm`, like the other callee, where its old prototype took a `u32`; a `(u16)`
  cast at the call does not do it. When such a local's slot is off, check the callees' prototypes first.
- A spilled local's width for the rule below can come from its callee's return: `wbt_list.c`'s
  `WbtList_PrintMessage` stores its first name call's last stack argument before the name's address, as the
  original does, only with `s16 first` set from `WbtList_GetMatchValue` returning `s16` (and its `value` local
  `s16`). An `s16` local from an `s32` return, or any other pairing, swaps the two. The callee's other callers
  bound the width: `WbtList_UpdateMessage`'s `u16 message = WbtList_GetMatchValue(...)` keeps its narrowing
  only with a signed or wider return.
- The types of locals and of the values they hold change how spilled values are scheduled. The trainer AI's speed
  comparison only matches with the speed function returning `u16` into `u16` locals: a spilled `u16` is reloaded after
  the call's stack argument is stored, while a spilled `u32` is reloaded before it.
  Spilled parameters follow the same rule at both ends of the call: `wm_icon.c`'s `func_0203e76c` reloads its
  `u32 x` and `y`, passed on as `u32`, ahead of the stack argument stores, and its `HeapID heapId`, passed to a
  `u32 heapId`, after them. Widening the callee alone, or narrowing `heapId` alone, did not match: sweep the caller's
  and the callee's widths together.
  A `u32` local that holds a 16-bit value counts as `u32` when the narrowing is a mask: the mystery gift's
  `Mystery_CreateGiftPokemon` stores `setFatefulEncounterPkmData`'s day argument last, after reloading the spilled
  year, only with `u32 year = (poke->date >> 16) & 0xffff`, written like its month and day; a `u16 year` stores
  it first.
  A spilled loop bound is the same: `AddExpAndEVs` in `btl_server_flow_sub.c` stores the spilled counter before
  loading its bound, the number of mons, only with the bound a `u16`. And `func_ov167_021afd90` matched only with
  `numMoves` a `u32` and its locals in one order of 720; no order matched with it a `u8` or `u16`, so sweep the orders
  again after widening a local.
  The spills stay word-sized `str` and `ldr` whatever the type, so the asm doesn't show it. A reload placed above
  another spill's store, where the original stores first, can mean narrow locals: in `scrcmd_musical.c`'s
  `func_ov012_02158280`, `secondValue = most; second = topIndex;` stores before it reloads only with `topIndex` and
  `secondValue` both `u8` (or `u16`), so sweep the widths of several locals together: narrowing one at a time, or
  all the candidates at once, misses it. A local read from a field can need the field's type even with the same
  instructions and slots: `u8 gender = poke->gender;` in `Mystery_CreateGiftPokemon` moves the species reload and
  the `poke->form` load among `PML_GenPID`'s stack argument stores, which matched `mystery_gift_pokemon.c`'s
  `func_ov012_02153160`.
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
- A struct local whose address does not survive into the optimizer is split into one variable per member, in the
  lowest stack slots, and every declared slot moves up. A `&s` given to a `static inline`, `(void)&s`, a dead pointer
  or one used in the same block, `s[1]`, union or nested members and a copy between locals all leave it split. An
  initializer (stored through a base register), a real call taking `&s`, a copy to or from memory whose address is
  taken, an inline's by-value parameter or return (each emits the copy), or a pointer local live across blocks (each
  access an `add rN, sp, #off`) keeps it whole. The original's `best` in `dowsing.c`'s `Dowsing_Update` is whole,
  with direct `[sp, #0x48]` stores and no initializer, and none of these gives that at no cost (see its row).

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
- A pointer local to a struct field whose offset is beyond the store's immediate range is kept as the base register
  plus the offset in a stack slot, and the store through it is `str r6, [r5, r0]`. The slot is written at entry from
  another offset the function already holds (`adds r0, r4, #0; str r0, [sp, #0x18]; adds r0, #0xc; str r0, [sp, #0x18]`).
  `psel.c`'s `PselText_PrintType` declares `BOOL *done = &wk->winDone[2];` right after `BmpWin *win = wk->win[2];` and
  ends with `*done = FALSE;`; the plain `wk->winDone[2] = FALSE;` rebuilds the constant and has no slot, 8 bytes shorter.
- A halfword local whose address is passed, stored through the register that is then the argument
  (`add r0, sp, #4; add r0, #2; strh r6, [r0]`), is a one-element array with an initializer, `u16 black[1] = { 0 };`,
  as `game_clear_save.c`'s `GameClearSave_Main` passes to `gfxUploadStdPaletteBGB`. A scalar `u16 black = 0;` with
  `&black` stores at base plus offset and computes the address a second time.
- Two spill slots swapped between a doubled index local and a hoisted loop invariant: the local held the undoubled
  value and the doubling was in the uses. `PkwdBook_CreateTextActors` has `positionIdx = entries->positionIdx;` with
  `PkwdBook_GetPosition(wk, positionIdx * 2)` and `(wk, positionIdx * 2 + 1)`; MWCC still spills the doubled value
  once, but in the slot before the hoisted `wk + paletteSlot * 4`. `positionIdx = entries->positionIdx * 2;`
  gave the same instructions with the two slots exchanged, in every declaration order.
- Two flags set side by side and chained (`refresh = redraw = TRUE;`) change which of them is spilled where their
  declaration order does not; the instructions are otherwise the same. The film scenario's `BScenarioMain_ScriptInput`
  (799 bytes of register allocation before) needed the chain in its two touch branches.
- When two fields of a struct are set to constants and one constant is shared, assign the fields in the struct's
  order: `Demo3DExpCCruiser_Init` had `r0` and `r1` swapped (21 bytes) until its `setup` was filled
  `vramType = 1; flags = 0;` first. In the same overlay `Demo3DEngine_SetCamera` needed the inline `FX_Mul` where
  the source had the `FX_MUL` macro (the inline keeps a call's result as the first operand), an `fx32` local for the
  `fx16` cosine (it is then loaded where it is written, not forwarded to its first use), and `left = -FX_Div(...)`
  in a local assigned after `right` (as an argument the negation is done when the arguments are set up).
- A pointer reused across several loops is spilled to the lowest slot; declared in each loop's body it sits above the
  compiler's element-address temporaries. Mistralton Gym's `Mistralton_LoadScene` (0x334 against 0x32c before)
  declares `SRTMatrix *matrix;` in each of its loop bodies, zeroes `u8 i` at its declaration
  (`for (; i < FAN_COUNT; i++)`) and uses a second `u8 j` for the later loops. Repeat a statement-order sweep after a
  type change: in `Mistralton_SetStage` the `u32 count`'s store moved wherever it was written, and the `u8` one
  stays where it is written.
- A loop counter shared by two loops is split, and its spilled piece takes the lowest slot whatever the declaration
  order, so a declaration sweep finds nothing until each loop has a counter of its own:
  `ZukanCommon_BuildHabitatSummary` matched once its counting loop got its own `i`, with `pokedex` declared first
  and assigned last.
- **MWCC allocates registers and stack slots before it merges identical tails.** A call written at the end of both
  branches of an `if`/`else` compiles to the same bytes as one copy after it, but it counts twice for the allocator
  and ends live ranges inside the branches. That decides slots and registers that no type or declaration order can
  reach. The medal box's `MedalBoxApp_DrawInfo` writes `WordSetNumber(wordSet, 1, total, 3, 1, TRUE);` in both
  branches of its obtained check, which makes `total` a call-result temporary and puts its three spilled values in
  assignment order (9 bytes off with one call after the `if`); `MedalBoxApp_UpdateSparkles` writes
  `func_0204c520(actors[ACTOR_SPARKLE + i], TRUE);` at the end of both branches, which gives `actors + i` enough
  uses to win `r7`, so the zero is not hoisted and nothing spills (0x150 against 0x13c before). Try this first when a
  function has the right instructions with the wrong slots or a hoisted constant. Related: a product shifted at each
  use (`target >> FX32_SHIFT` five times in `MedalBoxApp_UpdateSlider`) is computed once by CSE as a temporary the
  variable cannot absorb, where `target >>= FX32_SHIFT` once changes the registers.
- A spilled value that sits below temporaries assigned after it has the narrow type of the field it is read from:
  Pokéstar Studios' `EventPokewood_CreateCastParty` matched once `s32 count` became `s16 count`, the type of
  `PokewoodCastFile.count[]`. MWCC still spills it as a word, but gives it the lowest slot. Try the field's own type
  before sweeping declaration orders.
- Loop-invariant values that MWCC spills take their slots top down in the order they first appear in the loop's
  body, not in preheader order or by use count. A loop-body local that depends on the body moves an invariant's slot
  without being hoisted itself: the grid transition's `EncEffGrid_Draw` (22 bytes of stack slots) matched with
  `s1 = s + width * FX32_ONE; t1 = t + height * FX32_ONE;` assigned right after `s` and `t`, the far corner's texture
  coordinates. The invariant itself as a local (`w = width * FX32_ONE`) is hoisted under a guard and costs 4 bytes.

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
- Stores to the fields of one global, made directly or through a pointer local of its own type set to its address
  (`F *f = &G`, which MWCC turns back into direct accesses), stay chained in their written order, so a constant they
  store is built just before its first store. Through a pointer to a member (`&G.header`), a parameter or a cast
  pointer, the stores are told apart by offset and are independent, so the constant can be built ahead of stores
  written before it. `lib/irc`'s `IrcLink_SendFrame` builds the zero of the checksum bytes, which is also its copy
  loop's counter, before storing `id` and `cmd` only when it writes the frame's head through
  `IrcFrameHeader *header = &IrcFrame.header`; through `IrcFrameData *frame = &IrcFrame` the zero follows them.
- MWCC keeps a load and a store through pointers that may alias in source order. A field the original loads before
  a store is read into a local in a statement before it: `g3d_system.c`'s `GFL_G3DAnmCreate` reads
  `resource->data` before storing `resource` in the animation, and `event_field_proclink.c`'s `func_ov012_0215bb70`
  loads the party and the trainer data into locals before storing either, where reading a stored field back for a
  call gives load, store, load, store. libfx's `MTX_Concat33`, `MTX_Concat43` and `MTX_Concat44` load a column of `b`
  once for two neighbouring products of the result only with the column in locals: every store into the result
  reloads `b` otherwise, 0x18 to 0x40 bytes in all.
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
  same change fixed the register allocation of the loop in `fld_scenearea.c` that calls them. A `void *` parameter
  copied to a typed local needs the `const` on the `void *` itself: `event_field_proclink.c`'s `func_ov012_0215c160`
  and `script_evo` matched once `FieldProcLink`'s `createParam` and every callback in its table took
  `const void *prevParam`, while `const PokeListParam *partyParam = prevParam;` alone did nothing. Through a wrapper
  whose whole body is a `static inline`, only the wrapper's own parameter counts: `func_ov012_02153160` matched with
  `const MysteryGift *gift`, though `Mystery_CreateGiftPokemon`, which it returns, already took one. Since `const`
  shows in the code, a caller and its callee can disagree, and then the call casts: `btl_server_flow.c`'s
  `func_ov167_021a6c34` matches only with a `const` param and `func_ov167_021a6914`, which it passes it to, only
  without, so it passes `(BtlFlowMoveParam *)param`. Before casting, check that dropping `const` along the caller's
  chain doesn't match as well: `battle_rec_tool.c`'s party loaders took a non-`const` record with no change.
- MWCC passes a `const void *` to a `T *` parameter without a word, even with `-w all`, while a `const T *` passed to
  a `T *` warns of an illegal implicit conversion. After making a `void *` parameter `const`, find the `const` its
  uses drop with `clang -fsyntax-only @compile_flags.txt src/X.c`, which reports each as discarding qualifiers, and
  give a callee that only reads it a `const` parameter, as `MailTool_IsWritten` got for `event_field_proclink.c`.
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
  Plain fields go the same way across a `switch`: the mystery gift album's `MysteryCardView_SeqMove` reads its
  slot through the file's `MysteryCardView_GetSlot(view)` (`view->cursor + view->page * 4`), which makes case 1
  load `view->page` again as the original does, where the expression written out reused the load before the
  `switch`.
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
- The same rule picks the register of an argument's address. Arguments are set up stack first, then `r0` to `r3`, so
  a value read in the argument list, or in a local moved there, is loaded once the lower argument registers are set,
  through its own argument register (`ldr r3, =...; ldrsh r3, [r3, r4]`). A local that every block making the same
  call assigns again is not moved: it is loaded where it is written, before the setup, through the lowest free
  register. `badge_view.c`'s `BadgeView_StartMove` makes three `BadgeView_StartValue` calls and loads each `to`
  through `r1` (`ldr r1, =...; ldrsh r3, [r1, r4]`, with `mov r5, r0` in an earlier load's delay) only with one `to`
  local assigned before each of the three; assigned in the first block alone, or in two of the three, it compiled
  like the read in the call, with the copy of `wk` sunk to the `bl`.
- A local of another type than the callee parameters it is passed to is computed at its first use, past a call in
  between, where a local of their type is computed where it is written: `g3d_text_draw.c`'s
  `G3DTextDraw_StringToTexture` computes `size` before `gfxBeginTextureUpload()`, as the original does, only as a
  `u32`, the type `cp15_flushDC` and `gfxUploadTexture` take, not as an `int`. The same function loads its heap ID,
  a stack parameter, after the second `TEX_DIM_LUT` read only with the table's `u16` sizes read into `u16` locals
  and divided at each call, `GFL_BitmapCreate(width / 8, height / 8, ...)`; `u32` locals holding the quotients load
  it first.
- A global's address that the original loads early, into a saved register before unrelated calls, and copies from
  later was taken into a pointer local where it is loaded: `fldeff_gyoe.c`'s draw loads `&NNS_G3dGlb.projMtx` before
  the float calls of its depth offset only as `const MtxFx44 *proj = NNS_G3dGlbGetProjectionMtx();` declared first
  and `saved = *proj;` after the offset. `saved = NNS_G3dGlb.projMtx;`, with or without the offset in a local first,
  loads the address after the calls into another register. A static table's address that MWCC gathers into a saved
  register by itself goes the other way: `fmission_app_draw.c`'s `FMissionAppDraw_StartMessage` loads
  `sFMissionAppDrawMessages` into `r7` at entry, ahead of the spill of its `window` parameter (`str r1, [sp, #0x18]`),
  only with the table indexed at each use; `const s16 *messages = sFMissionAppDrawMessages;` gives the same registers
  with the load after the spill. A branch that loads the address again from the pool, as one does there, hints that
  no local held it.
- A struct assignment loads every field before it stores any, through a pointer that is not `const` too. Two
  fields copied with both loads first, `ldr r2, [r0, #0x10]; ldr r1, [r0, #0x14]; str r2, [r0]`, are one struct
  copied: `KeySystemTween_Update` ends with `tween->pos = tween->end;` for an `{ s32 x, y; }` position.
- A load through a `const` pointer is also reused across stores, as the World Tournament's `wbt_setup.c` reads an
  entrant's bit fields from one load, but it is not hoisted out of a loop: `wbt_party.c`'s filter check reads each
  list's count again in every iteration because its filter is `const`, where a plain pointer's count is loaded once
  before the loop. Overlay 169's `BtlMonSet_GetTotalDamage` and `BtlMonSet_GetHpDamage` match the same way with
  `const BtlMonSet *set`, and `PokeSelect_Init` loads `cmd->count` above its store of 0 only with `cmd` `const`.
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
- The same holds for a flag: `oekaki.c`'s `Oekaki_UpdateTouch` sets `endPressed = TRUE;` after
  `Oekaki_PrintMessage(...)` in both arms of its End button case. The store to `[sp]` is hoisted above the call
  either way, built in `r1` when the statement follows the call and in `r0` when it precedes it.
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
  type matches its uses' seems to count too. A pointer goes to a callee-saved register at entry only when its uses
  weigh about 4, where a use as the base of `->` or `*` weighs 2 and a plain use, an argument or a copy, weighs 1; a
  dereference through a cast weighs 1, and so does passing it to a `static inline` that takes a `const T *`. A
  lighter pointer is reloaded from its stack slot at each use, with nearby uses sharing a load. The same file's
  `func_ov012_02162068` reloads its `trainer` (`ldr r2, [sp, #0x3c]`) as the original does once its dereferences go
  through `SetTrialHouseTrainer`, an inline taking `const BSubwayTrainer *trainer`; with a non-`const` parameter the
  inline is transparent and `trainer` keeps its register.
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

- An xor hoisted before a search loop (`mov r3, r1; eor r3, r0` ahead of the loop, with the struct pointer still
  live) was the xor written inside the loop, `if ((keyInfo->difficulty ^ keyInfo->seed) == UNOVA_LINK_DIFFICULTY_KEYS[i])` in
  `key_info.c`'s `GetGameDifficulty`, not a local computed before it. That file's setters matched only with the
  table's constant read into a local first: `u32 value = sUnlockKeys[key]; keyInfo->unlocked[key] = value ^ keyInfo->seed;`.
- A call's stack argument is stored after its register arguments are loaded only when no argument is narrowed on
  the way: `poke_tool.c`'s `fillDataForHatchedEgg` stored `heapId` to the stack too early until `placeName` was a
  `u32` in it, in `PML_UtilSetupMetData`'s prototype and in that function's static helper. When a caller differs
  only in such an order, widen the callee's parameter and follow it down.
- A `u32` local's reload from its stack slot is hoisted above a store through a pointer, and a `u16` local's is
  not: `event_battle_return.c`'s `EventBattleReturn_Main` sets up the evolution demo with `str r7, [r3, #4];
  ldr r0, [sp, #0x18]; ldr r2, =SHINKA_DEMO_PROC_FUNCTIONS; strh r0, [r3, #8]` only with `u16 species`. That in turn
  needs `CheckEvolveSpecies` to return `u16`, or the call's result is narrowed. It is not aliasing: taking the
  local's address, moving it into a local struct or retyping the pointee's fields leaves the `u32` hoist in place.
- One constant register feeds a stack argument and a register argument of the same value, so an extra `mov` into
  an argument register beside an equal constant is a wrong argument, not scheduling: `th_award.c`'s
  `ThAward_InitRankIcons` loads file 1, and with file 0 it had one `mov r1, r6` too many at the same size.
- A variable reused by a second block of the same shape changes how that block's arguments are scheduled, with the
  registers unchanged: `ThAward_InitText` measures its points text into a `pointsWidth` of its own, which lets the
  width's reload move above the stack argument's store, where reusing `kindWidth` kept it below. In the same
  function two address locals set in one branch are spilled in statement order (`record` before `name`).
- A pointer reused by later blocks of the same shape changes their registers: `zenkoku_zukan_award.c`'s
  `ZenkokuZukanAward_InitText` prints its three blocks through three `StrBuf *` locals (`strbuf`, `certifies`,
  `staff`), declared together ahead of `wordSet`. One `strbuf` for
  all three stays in `r4`, the width takes `r6`, and the print colour is then shared across the last two prints
  where the original builds it again for each.
- A member array's offset added to the struct pointer before the index is shifted
  (`adds r2, r6, r1; lsls r1, r4, #2; ldr r1, [r2, r1]`) comes from a pointer local that is initialized and then
  stepped: `u32 *state = area->states; state += type;` in `ZukanHabitat_GetTypeAward`. A pointer that is only
  initialized (`&area->states[type]`), a nested struct, an inline accessor and other index types all fold back to
  `(area + type * 4) + 0x354`. The stepped pointer also keeps a loaded index in its own register and shifts it into
  another (`ldr r0, [r4, #0x40]` ... `lsl r1, r0, #0x4`), where every other spelling, index locals and inlines
  included, shifts it in place and lets the load move above the parameter's copy: `resonance_resort_data.c`'s
  `JoinAvenuePerson_AddVisitorData` writes `JoinAvenueData *data = person->data; data += visitor->dataIndex;` inside
  its `if`, and the pointer initialized before the `if` left 12 bytes off.
- A field's offset added to the base before a register index (`adds r0, r0, #4; ldr r1, [r0, r1]`) is MWCC's address
  of `p[i].field` when two accesses share it, `p` is a pointer variable and its first common-subexpression pass did
  not merge them, as in `f[i].b += v`. Otherwise the front end folds the offset out of the index: `(&p->f)[i * 2]`
  compiles like `p[i].f`, index first with the offset in the access (`adds r0, r1, r0; ldr r1, [r0, #4]`), and so do
  other field types, `const`, a nested struct and a one-element array member; a base loaded from a field
  (`list->table[i].f`) shares the address but indexes first too. Overlay 139's `UpdateScroll` reads
  `touch[touched].action` twice with the original's offset first, and no natural spelling of it is known (see its row).
  An array member's offset goes the same way: two reads of `data->entries[i].type` with a store between give
  `adds r1, r0, #0; adds r1, #0x18; ldr r1, [r1, r4]`, and with `const` data a later pass drops the second load.
  A pointer local such as `entries = data->entries` is kept, not folded into `data`, once it has a use in a later
  block than its definition, but reads with no store between are merged first. Overlay 332's
  `CygnusData_GetEntry` gets the original's loads this way, with its two stores in the other order (see its row).
- Two fields of one element of a static table read from the same base, one as `ldr r0, [r1, r2]` and the other
  after `adds r1, r1, r2; ldrb r1, [r1, #4]`, are a plain read and a read through an inline taking an `int` index.
  Written plainly, the second field's offset folds into the table's literal (`sMenuInfo+4`); through an element
  pointer, or a getter whose index has the plain read's type, the element address is shared and computed first,
  into a new register. The union room's `UnionMsg_UpdateRuleMenu` reads
  `sMenuInfo[ruleSel[3]].rows[UnionMsg_GetMenuInfo(ruleSel[3])->count - 1]`, with `UnionMsg_GetMenuInfo(int kind)`
  returning `&sMenuInfo[kind]`; the getter with a `u8` index is 14 bytes off and with `s8` folds again.
- A constant stored ahead of a call that passes the same constant can be an assignment written after the call: MWCC
  shares the constant and stores it where it is first made. `BScenarioMain_PrintCast` sets `sexMsg = CAST_MSG_MALE`
  after the species' print, and declares `u32 color` before `int sex`, which decides which of the block's locals gets
  `r7` and which a stack slot.

## An instruction too many or too few

- A field loaded again at every use, even right after it was read into a local (`task = q->head;` then
  `q->head->next`), is `volatile` in its struct, and that is natural for data that other threads change. Multiboot's
  task queue (`mb_task.c`) keeps `MBTask *volatile head`, which the task thread and the threads adding jobs share;
  with it `MBTask_ThreadMain` and `MBTask_Add` match.
- A global reached through a cast to a derived struct, `((Derived *)gBase)->arr[n].f`, is addressed as
  `(base + n * size) + offsetof(f)`, where the global's own type folds the constants first. Multiboot's work global
  is the parent's and child's shared part, which the parent's code casts to its extension (`MB_PARENT_WORK`), and
  `MB_DisconnectChild` (`mb_wm.c`) only matches through it.
- A narrowing missing after an `if`/`else` that assigns a `u16` in both branches comes from a conditional expression:
  `channel = (channel == 0) ? 1 : channel << 1;` narrows once after the merge, which multiboot's `MBWm_NextChannel`
  (`mb_wm.c`) needed.
- A member's address kept in a callee-saved register from an early store to a later call's argument, where ours
  recomputes it: `btlv_mcss.c`'s `BtlvMcss_Transform` needed a pointer local declared at the top but assigned just
  before the call, `MCSSLoadInfo *dst; ... dst = &work->entries[index].info; func_02019bd8(..., dst);`. Initializing
  `dst` at its declaration gives the base in the right register but adds a `mov ip, r0` spill, and passing the
  expression straight to the call drops the CSE entirely.
- A trailing `mov r1, #0` after a 32-bit result is a function returning `u64`: DWC's `DivWords` (`crypt.c`) returns
  `(u32)(...)` from a `u64` function.
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
- A small fixed-size copy the original wrote as a hand `ldrb`/`strb` loop, rather than calling `memcpy`, only keeps
  that shape when the loop runs over its own `const u8 *`/`u8 *` locals; written directly over the function's raw
  `const void *`/`const char *` parameter, the same `do { *dst++ = *src++; } while (--n);` compiles differently and
  doesn't match. DWC's `gt2.c` (`Gt2_Send`, `Gt2_Connect`, `Gt2_CancelConnect`) copies a 16-byte header with
  `const u8 *src = header; u8 *dst = buf;` declared first, then the loop; a separate, variable-length copy in the
  same functions (over the caller-given `message` buffer) is a plain `memcpy` call, as the original has there too.
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
- A signed halving kept apart from its narrowing ahead of a loop (`asrs r0, r0, #1; lsls r0, r0, #0x10; asrs r2, r0,
  #0x10`), where MWCC gives `lsls r0, r0, #0xf; asrs r4, r0, #0x10`, is a value the loop body computes through a narrow
  local and the loop-invariant pass hoists. The fold is a tree rewrite before code generation, and it carries a
  single-use halved value through a store, a call, a label, an enum-typed temporary or a reused parameter, but the
  hoisting moves the two statements out one by one and nothing folds them again. `local_tvt_util.c`'s
  `LocalTvtUtil_BlendPalette` matches, with the original's registers (the rate in `r2`), only with `fx16 cosine =
  FX_CosIdx(angle); fx16 rate = (cosine + FX16_ONE) / 2;` at the top of its 16-color loop. An `fx32` or `u16` cosine,
  the cosine local before the loop, or `FX_CosIdx` read inside the rate's expression all fold again.
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
- Two constants applied to a field in one expression are folded into one (`x = x - 0x100 + 1` becomes `subs #0xff`);
  as two statements, `work->winV -= 0x100; work->winV++;`, both operations stay, as `btlv_effvm.c`'s
  `BtlvEffvm_WindowTask` needs (an unsigned `1u` mixed in also keeps them apart, but less naturally).
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
- The same goes for an argument: a conditional expression passed to a `u16` parameter is narrowed before the call,
  while a `u16` local assigned a constant in each arm of an `if`/`else` is not. `un_select.c`'s `UnSelect_ListRun`
  passes its next step to `UIScene_SetNext` this way.
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
  `#define`s that asm includes share, as `CONDITION_*` are, a count-only enum in a C header gives the type. A constant
  kept so for calls and then reduced at run time before a field store (`subs r6, #0x20; str r6, [r5, #8]`) has another
  spelling, the field assigned and then updated in place: `mictest.c`'s `MicTestMic_Init` writes
  `mic->param.size = bufferSize; mic->param.size -= 0x20;`, and MWCC passes the stored `0x120` on to the update
  without folding the subtraction, so it keeps the constant in `r6` like a variable. `= bufferSize - 0x20` folds to
  `0x100`, and an enum local reduced at run time gave the same instructions with the constant in `r4`, ahead of the
  parameters.
  A constant narrowed at a call (`movs r3, #0x11` then `lsls`/`asrs` to `s16`) is an enum local too, and where it
  is assigned decides where it is built: set at the top, before the first call, and live across calls while every
  saved register is taken, it is rebuilt at its one use inside the argument setup. `zukan_detail_form.c`'s
  `ZukanDetailForm_PrintEntry` declares `EntryLine colorY = ENTRY_LINE_BOTTOM;` (17) and passes it to the color's
  print; set in the color's block, the 17 is built before `x`'s arithmetic (9 bytes), and an `int` or a field
  folds the narrowing away.
  An enum-typed index local stays a variable of its own too, which decides which register the copy for a multiply
  gets: overlay 209's `MusPokeDraw_CellCallback` keeps `kind - 7` in `r4` and multiplies a copy in `r1`, as the
  original does, only with `MusPokeDrawEquipNo pos = kind - MUS_POKE_DRAW_CELL_EQUIP;` and `poke->equips[pos]`
  indexed in place; an `int`, `u32` or `s32` local gives the reverse.
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
  Reads through such a pointer in an inline add the element's base first as well: `wipe_sub.c`'s
  `WipeLineWnd_SetLine` takes `WipeLineBuf *buf = &lines->buf[index];` and adds the array's offset to `lines` before
  the index and the field's offset, where `lines->buf[index].lines[line]` folds both offsets into one constant. That
  changed the registers of all of `WipeLineWnd_HBlank`, down to a parameter's copy at entry, and its size by 4.
- A local array or struct initialized in its declaration is stored through a base register, `add r0, sp, #0x4c;
  str r4, [r0]; str r4, [r0, #4]`, where assignments to its elements store at `sp` offsets. A `u8` array initialized
  so, `u8 kinds[3] = { FALSE, FALSE, FALSE };`, also makes MWCC load memory again after each store to the array, where
  with assignments it keeps the first load: the Pokédex habitat map reads `wk->habitat` again for each of its three
  tests of a place's habitats.
- Thumb has no `sp`-relative `ldrh` or `ldrb`, so a field of a struct local is loaded through a fresh
  `add rN, sp, #off`, even when a saved register already holds the local's address for calls; a pointer
  `p = &header` is propagated and folded back the same way. A load through the saved register (`ldrh r1, [r6]`) means
  a raw buffer read through a typed pointer: `msgdata.c`'s `GFL_MsgDataLoadHeader` declares
  `u8 buf[sizeof(MsgFileHeader)]` and `MsgFileHeader *header`, and assigns `header = (MsgFileHeader *)buf;` just before
  the call that first takes it. Assigned at its declaration, the `add` is hoisted above an earlier call.
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
  takes a `const` pointer, as `GymElecFade_IsActive` does. The callee can be in another file: `pokeparty.c`'s
  `copyPkmIntoPartyBlk` forms an element's address twice, in two shapes (`(base + 8) + off` for a call and
  `(base + off) + 8` for a struct copy), only once `poke_tool.c`'s `PokeParty_GetParam` takes a `const PartyPkm *`.
  A field loaded in an inline that takes a `const` pointer is not shared either: `msgdata.c`'s
  `GFL_MsgDataPreloadFull` loads `msgData->header` again in its `else` after `GFL_MsgDataGetLangOffset`, which takes a
  `const MsgData *`, read it. The same holds for a `static inline` that takes a `const` pointer from a caller whose
  pointer is not `const`: `bmp.c`'s `ClipCopyArea` takes `const GFLBitmap *dest`, and `GFL_BitmapCopyArea_IDX4`,
  `_IDX8` and `GFL_BitmapCopyAreaRebased_IDX8` load `dest->width` again after it for their tile counts, as the
  original does, where with a plain `dest` the clip's load stays live in a saved register and every register moves.
  A pointer field read through a `const` parameter at each use is loaded once into a compiler temporary, which the
  allocator places apart from the index: `ica_anime.c`'s `GFL_G3DCurveGetNowTranslationLoop` and its rotation and
  scale siblings keep the remainder both in `r2` after `__aeabi_uidivmod` and in `r3` for `* 12`, as the original
  does, only with `const G3DCurve *curve`, `frame %= curve->frameCount` and `curve->translations[frame].x` (`.y`,
  `.z`) written out; a `values` local or a separate index keeps one of the two copies.
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
  `*(que->buffer + que->readPtr + 1)` loading at `#9` from `que + readPtr` (`btl_server_cmd.c`).
  `BrProcSys_Pop` similarly needs `*(p_wk->stack + p_wk->stack_num - 1)` to fold `- 1` into the load offset;
  `p_wk->stack[p_wk->stack_num - 1]` emits an extra subtraction. A ternary that stores
  to a static is a select (`bhi`); the original's `bls; b` is an `if`/`else` with a store in each branch.
- Advancing a pointer-like offset with `x += 0xc` lets MWCC fold the 0xc into every later offset from it; writing
  `pos = x + 0xc` and reading through `pos` keeps the `add` (`ssp_exifdec.c`'s IFD loop).
- A ternary argument `f(c ? 1 : 0)` compiles to the select form (`movs r0, #1; cmp; beq; movs r0, #0`). A branchy
  original (`bne`; `movs #1`; `b`; `movs #0`) is an `if`/`else` with a call in each branch, as `CtvtTalk_UpdateMain`
  calls `func_0203d564(TRUE)` or `func_0203d564(FALSE)`.
- A flag set with `b = TRUE; if (*p != 0) { b = FALSE; }` is live before `*p` is loaded, so `b` and `p` need two
  registers, and a spilled flag is stored twice. `b = (*p == 0) ? TRUE : FALSE;` reads `*p` first, takes `p`'s
  register (`ldrh r0, [r7]; movs r7, #1; ...; movs r7, #0`) and stores a spilled flag once at the join. A function
  can mix the two: `gym_virbank.c`'s `func_ov100_021eee4c` has the first of its four flags as `TRUE` plus `if` (its
  `movs r5, #1` is in the prologue) and the other three as ternaries; one spelling for all four leaves a pointer
  spilled and the frame larger. Where both spellings give the same instructions, the ternary still allocates the
  rest of the function differently: `scrcmd_musical.c`'s `func_ov012_021580c4` swaps its `vm` and `env` between
  `r4` and `r5`, and everything that shares them, with `online = TRUE; if (kind == 0) { online = FALSE; }`, and
  matches with `online = kind != 0 ? TRUE : FALSE;`, after over 400 declaration orders had not. `wipe_sub.c`'s
  `WipeLineRange_Apply` likewise needs `covered = range->covered ? FALSE : TRUE;`, with `covered` a `BOOL` and not a
  `u16`, for the flag to take the freed pointer register `r7`; set in two statements it outranks the temporaries and
  takes the lowest free scratch register.
- A ternary store `*p = c ? a : b` computes the address once and stores after the branches; an `if`/`else` with a
  store in each branch computes the address in each, as `Bitmap_Scroll16` does.
- Parenthesized offsets change the code: `pixels + (dst + 4)` adds the offsets and indexes once, `pixels + dst + 4`
  adds to the pointer twice (`Bitmap_Scroll256`).
- The operand order of a product decides which value is loaded first: `(row + 1) * rowHeight * 2` and
  `widthTiles * (y & ~7)` match in bmp_menulist.c where the other orders do not.
- `u8` flag parameters, not `BOOL`, change the order in which register parameters are spilled at entry
  (`BmpMenuList_CycleCursor`).
- A register parameter stored at entry and read back from its slot (`str r1, [sp, #4]` ... `ldr r0, [sp, #4]`), where
  ours copies it out of the register (`adds r0, r1, #0`), is narrower than `int`: its uses go through its `int`
  conversion, which reads the slot. A static helper takes the narrow types its public caller passes: `bmp.c`'s four
  `GFL_BitmapFillArea_*` helpers match with `GFL_BitmapFillArea`'s `s16 x, s16 y, u16 width, u16 height`, and were 16
  to 27 bytes off with `int` positions. They count rows with an `int j` set from `y`, since `y++` on the `s16`
  parameter adds an `lsl`/`asr` pair; `GFL_BitmapFillArea_IDX4_VRAM` reloads that counter right after storing its
  increment (`str r0, [sp, #0x14]; ldr r0, [sp, #0x14]`) only with `s16 y` and `j` together.
- A bit-table lookup with an unsigned `lsr #5` for the index and a signed modulo for the bit is
  `table[item / (sizeof(u32) * 8)] & (1 << (item % 32))`; `item >> 5` gives `asr` (pml_item.c).
- A `u8` parameter divided with `asr`/`ror` (signed `/ 4` and `% 4`) while its callers pass a `u8` with no
  conversion is a `u8` parameter copied to an `s32` local first: `s32 r = region;` in unity_tower_savedata.c's
  `func_02009ba4` and `func_02009be0`. Declaring the parameter `int` or `s8` matches the function but moves the
  callers' argument loads (`Upload_WifiHistoryDataSet`) or masks a `u8` passed on (`func_02009b84`); a plain `u8`
  used directly divides with `lsr`.
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
  The width of a spilled index parameter also schedules its multiply, with no narrowing anywhere to show it:
  `event_bsubway.c`'s `func_ov012_02166118` builds the size of `bsw->trainers[index]`
  (`movs r1, #0xe8; adds r1, #0x38`) after its event data's field stores with `u16 index`, under every order of the
  stores, and ahead of them with `int index` (or `u32`), which matches once the stores are in the original's order.

- A store written before a read through a `const` pointer parameter can move above another store; dropping the
  `const` keeps the source order (app_taskmenu.c's `AppTaskMenu_Create`).
- A range test `subs; subs; cmp; bhi` is an unsigned difference in the source, `x - left <= right - left` on `u32`
  values; MWCC doesn't fold `x >= left && x <= right` into it (`AppTaskMenuWin_IsTouched`).
- A test of `x == 1 || x == 2` repeated after a call subtracts again at each test on a `u32`, and on an `int` is
  worked out once into a saved register and compared again (`sub r4, r6, #0x1; cmp r4, #0x1` ... `cmp r4, #0x1`):
  `gf_overlay.c`'s `GFL_OvlLoad` matches with `int region`. Its loop also stores through
  `OvlEntry *entry = &list[i];` (`str r5, [r1, #0x0]; str r0, [r1, #0x4]`), where `list[i].id = overlayId;` stores at
  `[list, offset]`, which keeps the offset live as a third temporary and spilled the function's `dma`.
- A value narrowed to `s16` once and also used unnarrowed in `16 - x`, as in
  `(s16)(16 - alpha) << 8 | (s16)alpha` for `BLDALPHA`, came from an inline with `s16` parameters:
  `mystery.c`'s effects match with `static inline void Mystery_SetBlendAlpha(s16 ev1, s16 ev2)` storing
  `ev1 | (ev2 << 8)`, called as `Mystery_SetBlendAlpha(alpha, 16 - alpha)` with a `u32` alpha; `s16` locals or casts
  narrow in the other order. Called twice on one bit field, such an inline narrows it at the second call only, so the
  original sign-extends the field twice, once plain and once after `lsl`/`asr #16`: `clact.c`'s `func_0204cb8c` wraps
  `oam->x` with `ClAct_WrapOamX(oam->x)` for both its left and its right edge (and `oam->y` likewise), where one local
  wrapped once was 56 bytes short.
- `FX32_ONE` is an `int` constant in `nitro/fx.h` but a `long` in the SDK. Where a product of it is passed on,
  the type shows: `oekaki.c`'s `Oekaki_CreateActors` matches only with the sub surface's Y as `256 * 0x1000L`.
- Two identical expressions are merged into one spilled value, even across calls and loops. Where the original
  computes the value twice, the two sites differ in type: `pleasure_boat_rooms.c`'s `PleasureBoat_SetUpRooms` draws
  `NELEMS(sPeople) - (trainerCount + 2)` people, which is unsigned through `sizeof`, and later loops to the signed
  `PLEASURE_BOAT_ROOM_COUNT - (trainerCount + 2)`; with both as `15 - (trainerCount + 2)` the difference is computed
  once and kept at `sp+4`.
- An array element written through a local index, `boat->rooms[room].field` with `u32 room = picker.order[slot];`,
  folds each field's offset into its store; a `PleasureBoatRoom *room` pointer keeps the base and the scaled index
  in registers and spills more (the same function's frame fell from 0x7c to 0x6c). The index must be a local when
  the array it comes from has had its address taken, or every store forces a reload of it.
- `(a + c1) + c2` is folded to `a + (c1 + c2)` through locals, inline helpers, `const` objects and operand orders,
  unless the sum is signed and the last constant unsigned. `gym_driftveil.c`'s `DriftveilGym_InitObjects` adds
  `TILE(6)` and then `HALF_TILE` in two `adds`, which needs `#define HALF_TILE (1u << 15)`; with `(1 << 15)` they
  fold into one `0x68000` and the function is 12 bytes short.
- A product the original computes twice, where ours keeps it in a stack temporary, has operands of different
  signedness in its two uses: MWCC does not merge a signed and an unsigned multiply. `PkwdRecordMonitor_UpdateListMarks`
  passes `(POSITION_LIST_MARK + i) * 2` and `(u32)(POSITION_LIST_MARK + i) * 2 + 1` to a `u32 index` parameter; without
  the cast `(i + 17) * 2` was spilled and reloaded twice and the frame was 4 bytes larger. The two spilled slots left
  swapped after that (`scroll` and the loop body's `visible`) were fixed by making the block-scoped
  `BOOL visible = FALSE;` a function-level `BOOL visible;` declared first and assigned at the top of the loop body.
- A field loaded again with no call or store in between, where ours keeps the first load alive across an `if`, is a
  second read through a `static inline` that takes a pointer to `const`: a load through a `const` pointer is a
  different expression from the plain read, so the two are not shared. The Wi-Fi login's `WifiLoginMessage_PrintFrom`
  picks `y` from `msg->style` and then creates its window on
  `WifiLoginMessage_GetMessageBGOf(msg)` (`return WifiLoginMessage_GetMessageBG(msg->style);` with
  `const WifiLoginMessage *msg`), which loads the style again (0xc8). The same inline with a plain pointer, or the call
  written in place, reuses the first load (0xc6).
- A table entry's address that the original computes before a call's other arguments (`ldr r1, =table; adds r2, r1,
  r5` ahead of the stack argument's load), where `entry = &table[i]; f(arc, entry->fileId, ..., wk->heapId);` sinks the
  address into the argument: the pointer was set to the table and then advanced, a local with two assignments that is
  not moved to its use. `PkwdBook_LoadPoster` matches with `const PkwdBookLoad *screen = sPkwdBookPosterScreens;` at
  the top and `screen += graphicIdx;` before each call. An inline helper taking `&table[i]`, entry pointers at any
  scope and fileId locals all compiled like the sunk form. (Another case of "build it in steps".)
- A shifted index that the original computes again after a loop, where ours reuses the loop's, is the same index in
  another type: MWCC shares `palette << 5` only between uses of one type. The World Tournament list's
  `WbtList_SetPaletteGray` runs its gray loop in a `static inline` taking `u32 palette` while the function has
  `s32 palette`, so the tail computes the offset again with `palette` in `r7` and `work` spilled (75 bytes off
  without it). `work->grayPalettes[(u32)palette]` in the tail shows the same thing.
- A `sys_memset(p, 0, size)` whose size is computed before the zero is loaded was `GFL_STD_MemClear(p, size)`.
  gflib's clear is an inline over the fill, so the size is an argument of the inline and is evaluated first.
  `WifiBattleMatchSys_AllocPlayers` has `adds r2, #0xd4` before `movs r1, #0` only with

      static inline void MemClear(void *dest, u32 size) { sys_memset(dest, 0, size); }
      MemClear(sys->player, sizeof(WifiBattleMatchPlayer) + PokeParty_GetSaveDataSize());

  written directly as `sys_memset(sys->player, 0, sizeof(...) + PokeParty_GetSaveDataSize())` the zero comes first.
  (Scheduling section; a candidate for a shared inline in `gfl/std.h`.)
- In `if ((u8)f() != engine->criteria->color)` MWCC loads `engine->criteria` before the call and keeps it in a saved
  register. Where the original loads the pointer after the call, the call's result was in a local first: the Pokédex
  search's `ZukanSearchEngine_Step` has `u8 color = PML_PersonalGetParam(...);` and `u8 shape = ...GetShape(...);`
  before their comparisons (8 bytes larger without them).
- `u32` is `unsigned int` in our `types.h` and `unsigned long` in the SDK, and MWCC treats the two differently in an
  index: for `&sActorData[15 + i]` with an `unsigned int i` it adds first and multiplies (`adds r1, #0xf; muls`), and
  with an `unsigned long i` it keeps `i * 0x18` and adds the constant's offset, and folds the constant of
  `actors[ret + 11]` into the load's offset. The PC box's `func_ov255_021d0d70` and `func_ov255_021d57a0` match with
  the counter and the result declared `unsigned long`. Rows that say "the original folds the constant into the
  offset", or that match only with `(&a[i])[11]`, are candidates.
- `lsl #15; asr #16` is a halving folded with an `s16` narrowing, so the value is an `fx16`, not a statement-order
  problem: the Battle Competition's `WifiBattleMatchView_BlendColor` matched with `fx16 ratio`. In the same file a
  constant assigned in both arms of an `if`/`else` is written first in each arm (`frameBG = 2;` / `= 6;` in the two
  card `Create` functions), positions computed one from the next are a variable each
  (`count2X = right - width; middleX = count2X - width;` in `PrintCounts`, not a running `x -= width`), and an
  element stored and read back (`list->windows[i] = BmpWin_CreateDynamic(...); window = list->windows[i];`) computes
  its address before the call.
- An index narrowed twice where one narrowing is expected adds a base that is zero: `u8 actor = ACTOR_CAR + car;`
  with `ACTOR_CAR` 0 folds the addition but keeps the second narrowing. Seven functions of the roller coaster
  (`gimmick_roller_coaster.c`) differed by that. Check a file's data values as well as its order when completing it:
  all eighteen Y coordinates there were `FX32_CONST(2)` where the ROM has `FX32_CONST(32)`, which no probe shows.
- MWCC forwards a single-use local's computation to its first use, even a division call past a store; reassigning
  the computation's input afterwards anchors it where it is written. The blizzard's `Blizzard_ScrollBg` has
  `step = scrollStep / 28; scrollStep = (scrollStep + 1) % (NELEMS(sBlizzardScrollX) * 28);` with `scrollStep` a local
  copy of the field. `% NELEMS(table)` is the unsigned modulo of a signed index (`_u32_div_f`), and `% 30` of an
  unsigned call result that the ROM divides signed needs `(int)`: the probe masks those helper calls, so check the
  call targets. Pointers that the original sets between a loop's guard and its top, before the body's first
  computation, are locals of an explicit `if (count > 0) { ...; do { } while (++i < count); }` (`Flake_Add`).
- A field that other threads change, tested more than once in a row: the original loads it again for each test where
  ours reuses the first load, so the field is `volatile`. NitroWiFi's `SOCLSocket.state` (`socl_internal.h`), which
  the command pipes' threads set while the socket calls poll it, made `SOCL_GetStatus`, `SOCLi_ReadFrom` and
  `SOCL_WriteTo` match and brought `SOCL_Connect` and `SOCL_Shutdown` close; no spelling of the tests did it without.
- I/O registers reached as offsets from one base register (`ldr r0, [sb, #0x1a4]`, `add fp, sb, #0x100000`), with
  the base made by shifting a register that holds 1 (`mov r3, #1; lsl sb, r3, #0x1a`), are not `1 << 26` or a base
  pointer local: MWCC folds both back to a constant and loads each register's address from the literal pool. Only a
  shift that MWCC sees after propagating the value keeps it, `u32 base = 1; base <<= 26;`, which brought DS Protect's
  `DSProt_CheckFlashcartA` (`dsprot_detect_multi.c`) from 0x770 to its 0x764 but was judged unnatural and left out of
  the source. Its header address, which the original computes from the TWL bit, is
  `(((twl) + 5) << 23) - 0x1a0`; a `twl ? 0x02fffe60 : 0x027ffe60` gave two literals and a select.

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
  `billboard_act.c`'s `func_0204f768` tests the last animation command this way, and `pml_item.c`'s
  `PML_ItemIsNotSpecialMonsball` its nine balls, with `0xfe14` from the literal pool and `cmp #8; bls`, where
  `item < ITEM_FAST_BALL || item > ITEM_PARK_BALL` builds the constant with `movs`/`lsls` and compares twice.
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
  condition written so the original's first arm comes first. `btlv_timer.c`'s `BtlvTimer_Draw` is the same with a
  file-local anonymous enum, `int label = which == 0 ? TIMER_SEQ_BATTLE_LABEL : TIMER_SEQ_COMMAND_LABEL;`; the
  condition must stay `which == 0`, since `which ? B : A` still builds 1 first.
- A flag set to 0 and then to 1 under a branch (`movs #0`, `cmp`, `beq`, `movs #1`) is
  `BOOL b = FALSE; if (x) { b = TRUE; }`; `x != 0` builds 1 first. `script_command.c`'s `StaScriptCmd_PokeFlip`
  also needs the call before it in the argument list read into a local first.
- A 0/1 comparison flag is built set-then-clear (`movs r5, #1; beq; movs r5, #0`) when its register is free at
  the `cmp`, and with branches (`bne; movs r1, #1; b; movs r1, #0`) when its register is an operand of the
  `cmp`, so the shape follows the registers, not the spelling. `MysteryCardView_SeqMove` writes both flags as
  `BOOL shown1 = view->page == slot / 4;` and `shown2` likewise: declared first, `shown1` takes the `r1` that
  holds `slot / 4` and gets the branches, as in the original.
- A `cond ? 0 : 1` whose 0 the original takes from an existing register while 1 comes from a register it keeps 1 in
  throughout inverts the branch polarity under every ternary spelling; `btlv_effvm.c`'s
  `BtlvEffvm_GetDefenderPositions` needs the default-then-override form through a local,
  `side = 0; if (!(wk->attacker & 1)) { side = 1; }`.
- Blocks are laid out in source order. A switch whose default code comes right after its comparisons or jump table
  had `default:` written first, and `if (f()) { n++; } else { return FALSE; }` puts the return after the code that goes
  on, where `if (!f()) { return FALSE; } n++;` puts it before.
- Every `return` gets its own epilogue. Failures that all branch to one block that sets a saved register and jumps
  to a shared `mov r0, rN` exit can be a `static inline`'s early returns, which inlining turns into stores to one
  result and jumps to its end. `mystery_gift_pokemon.c`'s `func_ov012_02153160` is
  `return Mystery_CreateGiftPokemon(gift, heapId, gameData);`, the maker overlay 197 also inlines: written out with
  a result variable and `goto`, the result got a stack slot of its own and the frame grew.
- Identical statements in different branches are merged, so a branch that jumps into the middle of another block had
  the same code in the source. For example, `if (a) { x = 3; y = 19; } else { x = 0; y = 19; }` compiles differently
  from `x = a ? 3 : 0; y = 19;`. A run of jumps to one store, as in the start menu's `StartMenu_MoveCursor`, is the
  same store written in several `else` branches. The merged branches keep one predecessor each, so a constant's
  register from before the branch is reused in every arm; a constant rebuilt after a `?:` or `if`/`else` merge, where
  the original reuses the earlier register, means the original duplicated the tail in each branch:
  `btlv_effvm.c`'s `BtlvEffvm_MoveEmitterArc` needs the `FX_Mul`/`pos.z = 0` tail repeated in both `else` arms.
  A test that branches past an unconditional jump, `bne next; b hide`, where `||` would give one `beq hide`, is the
  first copy of a body written twice in an `if`/`else if` chain and replaced by a jump to the second:
  `func_ov194_021c4ec0` hides a marking icon with `if (anim == -1) { hide } else if (isEgg && i == 6) { hide }`.
- A 0 or 1 argument built directly in `r1` to `r3` across branches, where any local for it takes a callee-saved
  register and shifts the others, is an inlined helper whose branches return the constants. Its
  `if (x) { return 0; } return 1;` tail gives `movs r1, #0; cmp; bne; movs r1, #1`, and its `if`/`else` of returns
  the `beq; b` layout. `pokemontrade_demo_common.c`'s `PokemonTradeDemoCommon_InitSide` passes
  `PokemonTradeDemoCommon_GetSide(wk, partner)` to `PokemonTrade_GetPkm`; an `int side` local, nested ternaries and
  an inline with a result local all build the 1 first or move the registers.
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
- A load that both arms of an `if`/`else` begin with is hoisted above the branch, between the test and the `beq`.
  When the original loads it first in each arm, after the branch, one arm began with another statement that the
  scheduler then moved below the load: `bmp.c`'s `GFL_BitmapCopyArea_IDX8_VRAM` writes `destPixel--;` before
  `pixels = (srcPixel[0] << 8) | srcPixel[-1];` in its odd arm, which keeps `ldrb r1, [r2]` at the top of both arms.
  A `color = srcPixel[0];` set before the `if` pinned the load above the branch as well.
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
- `Aoss_Decrypt` needs the `u8` returned by `Aoss_Checksum` stored in a local before it is compared with the
  `checksum` parameter; comparing the call directly swaps the `cmp` operands, even when the expression is reversed.
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
- `x == 0 || x == 254 || x == 255` becomes a range test (`subs r0, #0xfe; cmp r0, #1; bhi`) only on a 32-bit `x`:
  on a `u8`, 255 is the type's maximum and the test is `cmp r0, #0xfe; blo`. `poke_personal.c`'s
  `isGenderlessOrSetGender` was 2 bytes short with a `u8` parameter, and its three callers in `poke_tool.c`, which
  pass the ratio unnarrowed, showed the parameter is a `u32`.
- `cmp; blt; mov; b; neg` in front of one shared division is the remainder written in both arms,
  `if (x >= 0) { t = x % 10; } else { t = -x % 10; }`, as in `pkwd_selectscreen.c`'s `PkwdSelectScreen_PrintDetails`.
  A ternary, an `if`/`else` on the value alone and an inline abs all give other code.
- A call left unmerged in one arm while both arms jump to one store means the store is written at the end of both
  arms, not once after the `if` (`PkwdSelectScreen_PrintMovie`).
- Two calls to one function in the two arms of an `if`/`else` are merged into one `bl` in every plain spelling
  (ternary argument, nested `if`, locals, `break`s). They stay apart when one arm's `bl` is already the join of an
  inner `if`/`else` and the code after it is written in both outer arms: the label on that `bl` stops the merge.
  `config_panel.c`'s `ConfigPanelAppBar_Main` plays its sound with an inner
  `if (button == BUTTON_QUIT) { cancel } else { decide }` and has `update = TRUE; appBar->decided = TRUE;` at the end
  of both arms of the outer `if`, and likewise `AppTaskMenuWin_SetActive(wins[button ^ 1], FALSE)` at the end of
  both arms of `if (appBar->decided)`, which gives the `1` a register shared with the arm's `TRUE`.
- `movs r1, #1; cmp; bne; movs r1, #0` reached by a `b` from the other arm, with the literal pool dumped after that
  `b`, is a constant arm written first and a select in the `else`:
  `if (p == NULL) { saved = FALSE; } else { saved = f(x) ? TRUE : FALSE; }`, as state 15 of `event_make.c`'s battle
  event has it. With the test turned round (`p != NULL` first) the constant arm cannot jump into the select's tail,
  and `&&` builds the constant before the call. The stores after it were also in the order of the asm's loads
  (`ldr r0, [r4]; str r1, [r4, #0x1c]; str r0, [r4, #0x18]` is `param.gameData = battle->gameData;` and then
  `param.unk4 = saved;`), which alone took 4 bytes off.
- Two `b` trampolines to one merged tail are two copies of the tail in the source. The free-blocks state of
  `GameClearSave_Main` has `(*state)++; break;` inside the `if`, after the inner `if (!save->blocksFreed) { break; }`,
  and again after the `if`; one shared copy after the `if` puts all three branches on one `b`.
- Two branches that store the same fields, one of them all zeros, are cross-jumped at their last store when the
  stores are plain assignments, and the zero takes the first free register. Through a `static inline` setter with
  `s16` parameters each argument is a value of its own: the zero gets a late register (`r3`), the other branch builds
  its constants from it (`adds r0, r3, #0; subs r0, #0x60`), and both branches keep all four `strh`. The town map's
  `TownMapMap_Main` sets its scroll limits that way (`TownMapMap_SetLimits(map, 0, 0, 0, 0)` against the four
  `TOWNMAP_SCROLL_*`); with `int` parameters, or with the fields stored in another order in the inline, it does not
  match, and none of the 120 orders of the plain assignments does.
- A flat table indexed through an unsigned local is not strength-reduced to a hoisted row address: with
  `u32 corner = place * 4 + i; s16 index = sBScenarioCursorPositions[corner];` MWCC hoists only `place * 4` and loads
  the table's address on every pass, as `BScenarioGraphic_SetCursor` does in the ROM. With a `[5][4]` table or `int`
  operands the row's address is hoisted.
- A loop's two increments are emitted in the order written: `PkwdCount_CreateActors` has `entries++, i++`, and
  `i++, entries++` was 2 bytes longer. There `int i;` declared first fixed the stack slots, and `count = group->count;`
  assigned last of the group's five fields fixed the register of the first load. In `PkwdCount_LoadGraphics` and
  `PkwdCount_LoadTitleGraphics` the palette loop's registers were right only without a `u16 paletteAdd` local: the
  expression `(screen->extra & 0xf) << 12` sits in the inner loop's body.
- A flag built as `bne; movs #0; b; movs #1; str [sp]` is an enum-typed local set with a ternary; every plain 0/1
  spelling folds to the mirrored `beq` layout. Which of a function's two flags is the enum can differ between
  siblings: in the Pokédex entry's `ZukanInfo_InitTypeIcons` the enum (`ClActDrawType`, our name) feeds the resource
  loaders and a `u32` zero-plus-`if` flag feeds `func_0204c040`, and in `ZukanInfo_ShowTypesInGameLanguage`,
  `ZukanInfo_ShowTypesInLanguage` and `ZukanInfo_LoadFootprint` it is the other way round. All four were 4 bytes
  long or 29 bytes off before.
- A dead second conditional branch on the same flags is a redundant test in the source: the live competition's
  `LiveBattleMatchFlow_StepLink` has `wk->readySent = (field12 != 0 || (field12 == 0 && !known));` (4 bytes short
  without the inner `field12 == 0`). In the same overlay `LiveBattleMatchFlow_StepRecord`'s spilled `won` and `lost`
  are `u8`, which alone moves the `won = 0` store after the first call's argument load, and
  `BattleChampionshipFlow_MenuOpenList`'s interleaved stores come from plain field order in the source, not from
  writing them in the asm's order.
- MWCC never threads a branch into an identical test that follows it: in `if (c) { A } if (c) { B }` the false edge
  of the first `c` still goes to the second test, at `-O4,p` and under every optimization pragma tried. When the
  original's branch jumps past an identical second test, the second test was nested inside the first and is kept as
  dead code: `country_region.c`'s `Country_IsValidPlace` tests `country != COUNTRY_NONE` again inside the block of
  its first such test, and as two `if`s in a row was 1 byte off, in that branch's target.
- One `movs rN, #0` reached from both a missing container and a missing entry is a `static inline` that tests both in
  nested `if`s and ends in a single `return NULL;`. `g3d_system.c`'s `GFL_G3DMdlCreate` gets its model through
  `GFL_G3DGetMdlByIdx`, written so, as overlay 12's `FieldChunk_GetFirstMdl` is. With
  `if (mdlSet != NULL) { mdl = NNS_G3dGetMdlByIdx(...); } else { mdl = NULL; }` MWCC copies the outer `NULL` from
  another variable's zero register (`adds r7, r6, #0`) and branches over it, and a wrapper inline that returns
  `NNS_G3dGetMdlByIdx`'s result does the same.
- A duplicated compare with a `blt` to a lone `b` can be two arms that each hold the same bodies written out: the
  ranking list's `IrcRankingList_VBlank` has the full forward and backward scroll bodies (with their
  `list->prevDirection = 1;` and `= -1;`) both in the arm that tests `list->delta` and in the arm that tests
  `list->prevDirection`, and MWCC merges the equal bodies; a `direction` variable set first does not give it. In the
  same file a clamp matched only as `MATH_MAX`, and the operand order matters: `target = MATH_MAX(target, 0);` in
  `IrcRanking_StepScrollToSelf` but `list->scrollMax = MATH_MAX(0, list->scrollMax);` in `IrcRankingList_Init`.
- Arms that each end in the same calls plus `*state = N; break;` are tail-merged, calls included, with the odd arm
  placed after the shared tail; an `if`/`else if`/`else` with one shared `*state = 3;` after it keeps a call per arm.
  `GymOpelucid_MoveEventCallback`'s state 2 is three such arms.
- An array indexed as `[4 + i]` where the original uses a base of its own is two fields: the summary screen's
  `PStatus_AnimatePalettes` matched once `u16 cursorPltt[8]` became `u16 cursorPlttSrc[4]; u16 cursorPltt[4];`, with
  `s8 r, g, b` declared in each loop's body. In the same overlay `PStaRibbon_HandleInput` needed its three stores in
  the statement order `isDragging = TRUE; cursorRow = hit; cursorMoved = TRUE;` although MWCC emits the byte store
  first either way, and a table's `{ TOUCH_RECT_END, 0, 0, 0 }` terminator had been swallowed into a `//` comment,
  which the code match could not show and the `.rodata` check did.
- A flag passed as a constant in each arm (`if (hit == 0) f(TRUE); else f(FALSE);`) is not the same as passing the
  comparison: the monolith's `MonolithPower_Main` and `MonolithMission_Main` pass their touch flag and cursor that
  way, and their `Input` functions clear `touching` after the A-button block, which passes a literal `FALSE`. A
  version constant can hide in a file that otherwise matches: `MonolithMission_IsAvailable` tests
  `header.unk01 == 2` in Black 2 and `== 1` in White 2, and mismatched in White 2 only, without a row. Probe both
  versions for every function of a file, not only the ones with rows.
- Two different tests of a `u16` that compile to the same `cmp #0; beq` are both kept: the wireless pipe's
  `TickBeaconTimers` has `if (t == 0) continue;` and then `if (t > 0) { t--; ... }`. In the same file an
  `if (WH_End(NULL)) { return; }` keeps a bare `cmp r0, #0` before the `pop` that a `break` drops, and initializing a
  `.bss` static (`static NetWhpipeWork *sWork = NULL;`) changed the order of two equal-size local initializers in
  `.rodata` while the static stays in `.bss`.
- Switching `u32`/`s32` to the SDK's `unsigned long`/`long` for the whole project was tried with a full rebuild: 12
  nonmatching functions then match and 75 matching ones break (27 restored by an `int` local, 30 only by an `int`
  parameter, 18 not at all), so the typedefs stay `unsigned int`/`int` and a function that wants the SDK's type gets
  it on one local, parameter or struct field (`RivalDataSave.metTrainerCount`: `arr[s->count - 1]` compiles to
  `adds; subs #4; str` only with an `unsigned long` count). When a row says a constant is folded into an index or an
  offset the wrong way, or two index registers are swapped, try each 32-bit index as `unsigned long` first, and as
  `long` where the SDK types it `s32`. With `int first`, `fmission_app_draw.c`'s `FMissionAppDraw_SetTime` computes
  `(first + 2) * 4` for `work->upperActors[first + 2]` and shares `first * 4` with either the digit loops' hoisted
  address or that later use, never both, so `first` stays on the stack; with `long first` MWCC folds the 2 into the
  base (`(work + 0x38)[first * 4]`), shares one spilled `first * 4` among all the uses and drops `first`. It matches
  at 0x120, where `int` gave 0x128 and `unsigned long` 0x11c.

## Loops

- MWCC doesn't turn an indexed loop into stepped pointers. When the original steps pointers through arrays, the source
  stepped them: multiboot's `MBFile_InitSegments` (`mb_segment.c`) comes 8 bytes short as an index loop and only reaches
  the original's size as `for (kind = ..., segment = ..., i = 0; ...; i++, segment++, kind++)`.
- A `for` loop with a constant bound keeps its test before the first pass only with a counter of an enum type, as the
  position loops of `btlv_mcss.c`, `btlv_effvm.c` and `btlv_gauge.c` do with `BtlvMcssPos` and `BtlvGaugePos`.
  It also keeps an array indexed by the counter indexed on each pass rather than walked by a pointer:
  `g2d_CellTransferManager.c`'s `NNS_G2dUpdateCellTransferStateManager` needs `NNSG2dVRamType type` (2.0/sp2p2).
- A loop counted with `!=` tests with `beq` before the loop and `bne` at its end, where `<` gives `bls` and `blo`:
  the forms page walks its form-name table with `for (i = 0; i != form; i++)`.
- `while (cond)` is rotated, with a copy of its test before the loop. A loop that tests once, at its top, is
  `while (TRUE)` with a `break` or `return` inside, as the Join Avenue's walks through its data are.
- At `-O2,p` no loop is rotated: a `for` or `while` loop is entered with a `b` to its test at the end, as the loops of
  `lib/irc`'s `IrcSpi_Recv`, `IrcSpi_Send` and `IrcLink_SendFrame` are (`-O4,p` rotates all three). A loop there with a
  guard before its first pass and its test at the end was probably a guarded `do`/`while`: `IrcLink_Checksum` matches
  as `if (i < size) { do { ... } while (++i < size); }` with `i = 0`, whose unsigned `0 < size` gives the original's
  `cmp r1, #0; bls`. `if (size != 0)` gives `beq`, and a `for` inside `if (size > 0)` keeps its own `b` to the test.
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
- A bound written in the `for` condition allocates differently from the same bound in a local: with
  `i < width * height * bytesPerTile`, the guard tests the product's own register (`cmp r1, #0`, with no copy),
  `i = 0` is set before the multiplies and takes the register of a pointer that is dead by then, and the other
  invariant takes the later stack slot. `g3d_text_draw.c`'s `G3DTextDraw_BitmapTiledToLinear` matches so, 2 bytes
  shorter than with an `int size` local.

- A test that the original places after its body, entered from the top as well as from an earlier branch, is a loop
  that stops after its first pass: `while (box < n) { ...; break; }`. The trade does one box a frame this way in
  `pokemontrade_proc.c`'s `func_ov194_021bb3c0` and `pokemontrade_2d.c`'s `func_ov194_021c2c04` and
  `func_ov194_021c3e9c`, each 8 bytes or so shorter as an `if`. Comment it, so it isn't "fixed".
- A load the original hoists out of an inner loop but not out of the loop around it was in the middle loop's body.
  `mystery.c`'s `Mystery_GetSpriteBottom` reads a sprite's characters a row at a time with
  `tile = (u32 *)charData->rawData + (row * 12 + col) * 8;` in the column loop; read in the innermost loop, the load
  only moves to the column loop, and read in the row loop, the `charData` load is not hoisted.

- A loop guard kept as an unsigned compare (`cmp r3, #4; bhs`) came from an enum counter tested against
  `NELEMS(table)`, whose `sizeof` makes the comparison unsigned; the enum's own bound gives `bge`. `symbol_save.c`'s
  `getKindOfSlot` loops `for (i = SYMBOL_KIND_KEEP_LARGE; i < NELEMS(sSlotRanges); i++)`.
- A loop over an array of small structs read through by-value inline getters matches only when each getter takes
  `learnset[i]` itself: `poke_tool.c`'s learnset loops (`PML_UtilPkmSetDefaultMoves`, `PokeParty_LearnNextLevelMove`)
  lose the copy the original makes for `GetMove` when the element is first put in an `entry` local. A plain local
  passed by value keeps its stack copy only when the inline reads a field other than the one at offset 0, while an
  element, `Get(learnset[i])`, is copied for each call, and once more into the preheader of an inner loop that
  passes it. Many copies of one element, as the eight of `waza_oshie.c`'s `PokeParty_GetRememberableMoves`, mean
  every accessor took the element; that function also writes its two searches out, with
  `LearnsetEntry_GetMove(learnset[i])` compared inside each loop.
- A loop entered with a `b` to its test and reloading its bound on each pass, where every plain comparison is
  rotated, has a test whose value is 0/1: `i <= ICON_COUNT - 1 ? TRUE : FALSE`, or an inline predicate. The Union
  Room's lower screen has it in `UnionSubdisp_SelectIcon` and in the `UnionSubdisp_ShowSelected` inline of `Create`.
  The same file's three BG setups are one `static const BGSetup sBGSetups[3]` (as three statics their section order
  was wrong), and its locals named after the assert strings (`chat`, `write_pos`, `view_no`, `offset`) also made
  `.data` match: the strings give the original's variable names. `country_region.c`'s `Country_IsValidPlace` loops
  while `i < gSelectableCountryCount ? TRUE : FALSE`, a loop that `#pragma optimization_level 1` or `2` also shapes.
  No enum counter is needed for it in game code: overlay 214's `WorldTrade_NationSortListMake` gets the original's
  loop entry and registers from `i < num ? TRUE : FALSE` alone. An expression that two of a call's arguments share,
  as its `first + i` is, becomes a common-subexpression temporary computed before the call's stack arguments are
  stored, which decides which register loads it; the original computed it after (its row).
- A loop with a constant bound keeps its `b` to the test when the test is an inline predicate that returns through
  `TRUE`/`FALSE`: `for (i = 0; UnionMsg_IsMenuKind(i); i++)` in the Union Room's `UnionMsg_UpdateRuleMenu`. A plain
  `i < 8`, and an inline with `return kind < N;`, are both rotated.

## Switches

- A `switch` with no `default:` whose cases leave one value unassigned keeps the index in `r3` instead of `r2`, as
  `b_plist_main.c`'s `BPlistMain_InitPageCursor` does, and writing `case 0:` last moves its block to the end of the
  switch (`BPlistMain_CanSwitch`).
- `*result = N; break;` in every case makes later cases branch to an earlier case's `b`, a branch to a branch. A local
  set in each case and stored once after the switch gives direct branches: `scrcmd_fldmmdl.c`'s `s0078`.
- Switch cases are laid out in source order, not by value, so the layout shows the order the cases were written in.
- A jump table that takes `-1` in with the other cases (`adds r0, #1; cmp r0, #6; bhi`) switches on a signed value.
  On a `u32`, `case -1:` is `0xffffffff` and is compared on its own before the table. `un_select.c`'s
  `UnSelect_ListRun` matched once `TouchBar_GetDecided`, which returns -1 when nothing is touched, returned `int`.
- A switch's comparison tree and jump tables depend on every case value, including cases with no code: the Battle
  Subway's command switch only splits its values as the game does with an empty `case 102:` inside its first jump
  table, and empty cases that sit between others still get a comparison.
- Listing the empty cases also keeps a jump table where identical case bodies would otherwise be merged into
  comparisons: `ringtone_sys.c`'s `RingtoneSys_UpdateState` matches only with all four states written, two of them
  empty.
- A switch case that ends in the same code as another case is merged into it, so its end moves.
- A state machine whose cases `break` to one `return FALSE;` after the switch lets MWCC merge the cases' identical
  failure tails; a `return FALSE;` in every case keeps a copy in each. `net_common_download.c`'s
  `NetCommonDownload_DownloadFileList` and `_DownloadFile` came out at 0x384 and 0x294 bytes with the returns, and at
  the original's 0x2e0 and 0x22c with `break`.
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
- When several cases end in the same `mov r4, r0; b end`, MWCC keeps one copy of that tail and which case owns it
  follows the source: `poke_tool.c`'s `PML_PkmGetParamCore` had the tail in its level case, one instruction away from
  the original for 258 bytes, until the max PP case, `if (move != 0) { result = ... }`, got its explicit
  `else { result = 0; }`.
- A `goto` from one case into another's tail makes the shared block a region of its own: an address or offset that
  is live across the switch is built again there (`movs r0, #0x91; lsls r0, r0, #2` where the original has
  `subs r0, #8` from the offset it still holds), and the temporaries at the switch's head take other registers, so
  the diff reads as registers only. Two cases with the same tail were written out in full and merged by MWCC:
  `ircbattlematch.c`'s `IrcBattleMatch_StateStartNet` matches with both musical cases complete, and with its last
  three mode tests as a `switch` in the blocks' order rather than an `if` chain.
- A switch case whose block holds nothing but a `b` to the code after the switch (its jump-table entry points at a
  lone branch, laid out in source order among the other cases, while the cases without code go straight to the end)
  had a copy between two variables that the register allocator later gave the same register. The block is created
  for the assignment and only the `mov` is deleted, after block layout. Empty statements, empty blocks or `if`s,
  nested switches, `for (;;) break;`, `goto end`, a dead store to a parameter or to an unused local and
  `error = error;` are all removed before blocks are made and merge the case with the end. `gsync_download.c`'s
  `GSyncDownload_OnNdEvent` (4 bytes short before):

      // before: case 3's table entry goes to the end      // after: case 3 gets `b end` of its own
      switch (error) {                                      u32 result = error;
      case DWC_ND_ERROR_NONE:                               switch (error) {
          break;                                            case DWC_ND_ERROR_NONE:
      case DWC_ND_ERROR_HTTP:                                   break;
          break;                                            case DWC_ND_ERROR_HTTP:
      ...                                                       result = error;
      }                                                         break;
      _pDLWork->error = error;                              ...
                                                            }
                                                            _pDLWork->error = result;
- An empty `if` whose condition has three or more `&&` operands is not removed with the other empty statements: its
  branches become blocks and go only later, so it splits the code around it for CSE and register allocation and
  emits nothing. Two operands, `(void)(a && b && c)` and `if (TRUE)` are removed first. `btl_monset.c`'s
  `SortBySpeed` keeps `set + j * 2` from its speed comparison across `BattleRandom` into the swap, in a saved
  register, only with `if (i && j && set) {}` between the swap's `damage[i]` and `speed[i]` stores, and nowhere
  else; a real `if (set == NULL) { return; }` there gives the same registers plus its own 4 bytes. That points at
  code the release build dropped, such as a debug print under a condition, so it is a probe for where the original
  has a block boundary, not code to commit.
- In a compare tree, a `blt default` lower bound marks a dense group, not a pivot: nearby cases compile to
  `cmp max; bgt` above a chain `cmp min; blt default; beq; cmp next; beq`, and the group counts as one unit when the
  tree is split. At 53-bit x87 precision, two cases form a group up to a distance of 4; three cases within a span
  of 5 also do. The bag's `ItemMenuDisp_IsFieldMoveTM` has 15 and 19 in one group, which requires that precision.
- A jump-table entry (or `beq`) that lands on a lone `b` to another case's code is a case whose body was written
  out again, not a fall-through. MWCC merges the identical body into the earlier case but keeps a one-instruction
  stub for the later one. `WbtDownloadMessage_Print` has `case 0` at a `b` placed after the shared print block: it
  matches as

  ```c
  case WBT_DOWNLOAD_MSG_PRINT_WAIT:
      p_wk->waitIcon = WaitIcon_Create(...);
      PrintWindow_Print(...); p_wk->state = ...; break;
  case WBT_DOWNLOAD_MSG_PRINT:
      PrintWindow_Print(...); p_wk->state = ...; break;
  ```

  and not with `case PRINT_WAIT: ...; /* falls */ case PRINT:` (which points the table straight at the block), nor
  with the two cases in the other order. The same holds inside an inline: `case 1: return TRUE; case 2: return TRUE;`
  gives `case 2` a `b` to `case 1`'s `movs r1, #1`, where `case 1: case 2: return TRUE;` gives none, and
  `case 0: return FALSE;` before a `default: return FALSE;` gives `case 0` a `b` to the default.
- **Switch trees depend on the x87 precision the compiler runs with.** MWCC groups a switch's sorted cases into
  ranges with a floating-point density test (cases / span above 0.4), done in x87. A Win32 process starts with the
  control word 0x27f (53-bit precision); wibo 1.2.0 leaves Linux's 0x37f (64-bit). A range whose density is exactly
  0.4 (two cases 4 apart, four cases in a span of 10) passes under 53-bit and fails under 64-bit, so wibo 1.2.0 made
  another tree than the ROM's compiler did, and no source changes that. With mwccarm run at 0x27f these match
  unchanged: `ItemMenuDisp_IsFieldMoveTM` and `PokeList_GetHidenResult` (cases 15 and 19), overlay 36's `SetupEncountFlagsByPartyPokeAbil`
  (42 and 46), `Demo3DCapture_End` and `Demo3DCapture_VBlank` (2 and 6; `Demo3DCapture_Begin` goes from 93 to 24
  bytes), and `box_search.c`'s `func_ov255_021d40e8` (cases 0, 3, 9, 15, 21, 24, 27, 30). A full build of every file
  for both versions at 0x27f still gives both ROMs, so nothing that matches today depends on 64-bit. The pinned wibo
  release configures the guest at 0x27f; if an older binary is used, check that before trying a "gap of 4" or a tree
  rooted one case off. What the tree builder does otherwise: ranges are built left to right, a case joining while the density
  holds; a range of four or more is a jump table; the tree is balanced over the items with its pivot at index
  (n - 1) / 2; a range after the first loses its first k cases to single compares, k being the number of extra cases
  in the ranges before it.
- A three-case `switch` is not range-folded where the `||` chain over the same values is, which frees a register:
  the tournament download's `TDownload_SelectTouch` and `TDownload_SavedTouch` test their touched button with
  `switch (touched) { case 3: case 4: case 5: }`. A constant stored to a spilled local as the last statement of a
  block is emitted at the top of the block, in the register of a later call argument of the same value
  (`movs r2, #1; str r2, [sp]` for `result = TRUE;` at the end of the block), and a spilled local's zero store lands
  late in the prologue unless the local is 16 bits wide (`u16 input = 0` in `SavedTouch`, where its twin has `int`).

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
  `u32`. `wipe_sub.c`'s `WipeLineRange_Apply` divides `(range->end - range->start) * step` signed, its fields being
  `u8`, where the same difference of two `u32` locals called `_u32_div_f`. Compare the built overlay in
  `build/<version>/build` with the original to find it.
- A 64-bit division calls `_ll_udiv` (swan's `__aeabi_uldivmod`) when either operand is unsigned, and `_ll_sdiv`
  (0x0208d414) when both are signed: `pass_power.c`'s `PassPower_ApplyCapture` divides an `s64 value` by 100. The
  probe masks the call's relocation, so only linking the file shows the wrong helper. A call to `_ll_udiv` with
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
- MWCC 2.0 lays a file's `.bss` statics out in the reverse of their definition order, without the size sort: the
  DWC utility's `dwc_wcm.c` (overlay 182) matched every function with `sWcm` (0x28 bytes), `sBssid[6]` and
  `sEssid[0x20]` defined in that order, but the module check failed until they were defined as `sEssid`, `sBssid`,
  `sWcm`, the ROM's order backwards.
- MWCC 2.0 reaches a file's own statics of one section through a shared anchor (`ldr r0, =base; ldr r1, [r0, #0xc]`),
  and another file's global by its own address. In overlay 182 that set the seam between `dwc_aoss.c`, which loads
  the free callback at `0x021c294c` directly, and `dwc_aoss_ap.c`, which loads it as `0x021c2940` + 0xc. It emits a
  jump table only when every value of the range has a case label, so `dwc_aoss.c`'s switches list the empty cases too.
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
- When structured reorderings don't reach the shared section's tie order, invert the model: the heapsort permutation
  depends only on the sequence of sizes by declaration slot, so for a given sequence the ROM's layout forces which
  object every slot must hold, exactly. Search the size sequences instead of the orders — anneal with the forced
  assignment's uses-before-definition count as the cost, swapping the sizes of two slots (prefer the two slots of a
  violated edge) — and write the file in the forced order that reaches zero. `demo3d_data.c` (overlay 293, 516
  objects, 341 small ones misplaced by every per-demo structure tried) matched this way, with one table left after
  its user and forward-declared. `tools/decomp/rodata_solver.py` runs this search.
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
- A "no declaration order works" result is worthless until the object list is complete, since a forgotten object makes
  the prediction silently wrong: `btlv_input.c`'s layout was called unreachable until the list also counted every
  function-local initializer (down to an 8-byte `ClActorSetup` local) and the tables referenced only by other tables;
  with those in the list a top-of-file permutation matches. `rodata_order.py --from-object FILE.o` lists every data
  object a compile produced so none is dropped, and `rodata_order.py ... --check FILE.o` fails loudly when the model
  and the compiler disagree; run the check against a real compile before trusting any verdict.
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
- Initialized `.data` statics join the same heapsort as the `.rodata` local initializers, though they land in
  another section, so their declaration order can reorder the locals' tables. `wifilogin_disp.c`'s sixteen 32-byte
  `BGSetup` locals came out in the original's order only with the 48-byte `BGSysVRAMConfig` declared before the
  16-byte `BGSysLCDConfig`; no order of the locals themselves did it. Give `rodata_order.py` the file's other statics
  too (`V:48 L:16 B0:32@ ...`).
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
- A global the file never reads can be defined anywhere in it, and moving its definition past functions changes
  MWCC's listing. Try that before assuming objects the link stripped: overlay 209's `sta_acting.c`, whose three
  12-byte initializers no table order fixed, matches with `STA_ACTING_UNUSED` (read only by an uncalled header
  inline) defined after `StaActing_UpdateScroll` and `STA_ACTING_VRAM_CONFIG` first among the tables. Sections of
  their own are created in sorted order, so among equal sizes which sorts first also decides whether the shared
  section comes before small initializers; check the object's section order, not only its symbol table.
- A global nothing refers to that the ROM has after the shared section, where ours sorts it first, needs an object
  of its size or less in the shared section that the sort puts first, and the globals' declaration order decides
  that tie: overlay 280's `namein.c` puts `NAME_ENTRY_SOUND_COUNT` where the ROM has it only declared right after
  the array it counts, above the tables. When the function-local statics are all one size and every table is at
  file scope, though, the layout fixes only their order in MWCC's list, not which functions own them: a fit with
  placeholder statics proves nothing without the code using the values.
- A small table that sits among a file's other small `static const` tables but that no code loads from was a
  referenced static whose reads were folded: an unreferenced `static const` is dropped, and a global gets a section
  of its own after the packed one. `pleasure_boat_rooms.c`'s `sTrainerCounts` holds the weekday `switch`'s seven
  values in case order, and the switch reading `sTrainerCounts[0]` to `[6]` compiles to the same `movs` while keeping
  the table. Its place among the packed tables is not definition order: defined between the room tables it came out
  one early, defined first it is where the ROM has it.
- `GFL_ASSERT` keeps its expression as a string in `.data`, so the variable it tests keeps its original name, as the
  Medal Rally's `p_sv` does.
- Japanese text in a string is written as is: sources are UTF-8, and the build hands them to MWCC in Shift JIS, the
  encoding of the game's strings, through sjiswrap, with `-enc SJIS` so that a second byte of 0x5c (`表`, `ソ`) isn't
  read as a backslash. An escape such as `\x94` can't stand in for it in an assert, whose expression is stringized as
  written: `wm_icon.c`'s `func_0203e5ec` has `GFL_ASSERT(0 && "非対応のOBJマッピングモード")`. A character that Shift JIS can't encode is fine in a comment; anywhere else sjiswrap names it, and its
  bytes would be wrong.
- Overlay IDs are linker symbols, written `OVERLAY_ID(279)` from `gfl/overlay.h`, which gives the literal pool entry
  a relocation. Mark the literal in the config with `tools/decomp/config_fixes.py overlay-id`.
- A table one element longer in the ROM than the code needs has a terminator: pml_item.c's `TM_MOVE_LIST` is 101 moves
  and a `MOVE_NONE`. Without it the next object starts 2 bytes early.

- MWCC doesn't pad a `.bss` section to its alignment: a 16-byte array declared `ATTRIBUTE_ALIGN(32)` at the end of a
  file's `.bss` leaves the section 0x30 bytes long, not 0x40. When the next file's `.bss` starts 16 bytes later in the
  ROM, the aligned buffer is larger than its use: the LTD's `photo_sign.c` sizes its random buffer, which the ARM7
  writes, to a whole 32-byte cache line though each fill uses 16. Aligning the next file's object instead gave the
  right size but changed how its code addresses it.
- Sections start 4-aligned at link time (`ALIGNALL(4)` in the LCF), whatever their own alignment: pms_data.c's
  12-entry `u16` initializer after 0x16 bytes of shared `.rodata` sits at +0x18, not +0x16.
- The same small table in the `.rodata` of several files of one overlay, each file with its own copy of the same
  inlined code that reads it, is a `static const` in a header with a `static inline` function: overlay 197's
  `mystery.c`, `mystery_album.c` and `mystery_check.c` each have the 15 ribbon parameters and the inlined gift-Pokémon
  routine of `app/mystery/mystery_gift_data.h`.
- The size sort that orders a file's `.rodata` takes its `.data` objects into the same list (string literals are
  not in it), and a tentative `.bss` definition counts at the end of the file. With that, `rodata_order.py`'s model
  predicts `pkwd_briefing.c` (117 objects), `c_gear_sw.c` and `comm_btl_demo.c` exactly; without it no declaration
  order of their tables gives the ROM's layout. A lone table ahead of the sorted run still means another file in
  front (`oekaki_comm.c`).
- A `const` on a pointer parameter lets MWCC hoist loads through it above later byte stores: the hit tests of
  `beacon_view.c` match only with a non-const `BeaconViewTouch *`, and the accessors of
  `fmission_log_accessor_fv.c` only with non-const entries.
- A 2-aligned object right after a 2-byte constant that nothing reads shows both are in the shared section: the
  constant is a global that unemitted code reads, not an unreferenced static, which would get a section of its own
  and push the next one to a 4-byte boundary (`wifi_p2pmatch.c`'s `64 00` at the start of its `.rodata`).
- When a file's equal-size tables and local initializers don't fall into the ROM's order with every table at the
  top, each object can go either at the top or right before its first user; searching those placements with
  `rodata_order.py`'s model found `wifi_p2pmatch.c`'s 39 objects in seconds. Functions that reach `.data` through
  one section-relative anchor (`ldr =first; str [r, #n]`) need the `.data` objects defined in ROM order as well.
- An order of equal-size local initializers that no declaration order of the file's objects gives can come from a
  table the original link dead-stripped: the sort listed it, then the linker dropped its section (an object of 64
  bytes or more has one of its own) with the unused public functions that read it. `pdwacc_disp.c`, a cut-down copy
  of `gsync_disp.c`, gets the ROM's BG setup order only with a referenced table between its VRAM and LCD configs,
  where `gsync_disp.c` has `sActorPositions`. Look at the sibling file a copy was made from for the missing object.
  Nothing the compiler drops itself is listed, so this can't be reproduced under `-nodead`.
- A small table that the ROM has after a 64-byte-or-larger table of the same file, where the size sort can only put
  it before, belongs to the next file. Overlay 290's 12-byte battle proc table sits after the 0x90-byte step table:
  no layout of one file gives that (`rodata_order.py`: G:12! S:144 B:12 is always B G S or G B S). Its three
  functions are the last of `.text`, use no string or static of wifibattlematch_sys.c, and the step table points to it
  like to the other overlays' proc tables: a second file with an extern table.
- The string literals of a pointer table's initializer do follow the size sort (this corrects what is said about
  `mbp.c` above): each literal joins the list just before its table, with size `strlen + 1`, and literals created in
  code are not in the list and come last. The model predicted 12 of 12 random declaration orders of
  `field_gimmick_gate.c`, whose `gelboard_N` strings were in the wrong order in `.data` until `sBoardNames` was
  declared directly before `sBoardPlNames`.
- A second `""` in what was taken for one file's `.data` is a file boundary: MWCC pools equal string literals within
  a translation unit, so each run of functions that shares one `""` is a file of its own. Overlay 28's `union_main.c`
  and `colosseum.c` were five files (`union_main.c`, `union_receive.c`, `union_subproc.c`, `colosseum_comm.c`,
  `colosseum.c`), which also explains statics that had to be external.
- A library's style applies to all of its files: libVCT (`lib/dwc/src/vct`) takes no pointer to `const` anywhere, and
  dropping the `const` we had put on its parameters matched `VCT_Init`, `GetEvent`, `HandleCommand4` and
  `VCT_DecodeAdpcm` at once. Its files are built without `-ipa file`, so their `.bss` statics come out in reverse of
  their declaration order: a matching function whose pool words point at the wrong static needs the declarations
  reversed.
- Size drops inside one file's `.rodata` are file boundaries: a translation unit's packed data rises in size, so each
  drop starts a new file. Overlay 215's `mb_seq.c` was six files (`mailbox.c`, `mb_main.c`, `mb_bmp.c`, `mb_obj.c`,
  `mb_seq.c`, `mb_ui.c`), and an assert's line number checks the result: `MailBox_StateInit` passes `"mb_seq.c", 268`
  and sat at line 1275 before the split, at 185 after it.
- An object's place in the size sort is its definition, not its first declaration, so a static table can be
  forward-declared (`static const TouchRect sSideButtonRects[3];`) and defined after its user to reach a layout.
  When no order with every static defined before its user gives the ROM's layout, but one extra unreferenced object
  does, the original had an object its linker stripped; `pokemontrade_proc.c` is laid out as such a fit, and says so.
  An external `inline` function gets its own copy of a string literal where a `static inline` shares the file's:
  `pokemontrade_save.c`'s `BackupMail` and `BackupRecords` are plain `inline` for their file-name strings.
- Byte-equal data can still be mislaid: two tables of equal bytes or two equal-size initializers that are swapped
  show only in the relocation targets, so a data check compares where each pool word points, not only the section's
  bytes. `infinity_dungeon.c` had three such pairs and `infinity_dungeon_gimmick.c` its two scene setups. In the same
  overlay a swap of two loop counters' spill slots that no declaration order moved meant the later loop reuses the
  earlier counters (`i` for floors, `j` for rooms in `InfinityDungeon_LayoutFloors`): spilled pieces are grouped by
  variable.
- A "merged tail" or "registers only" difference of hundreds of bytes can hide a wrong statement: the Battle
  Competition's `WifiBattleMatchLink_CompetitionTaskMatch` (743 bytes off) had two calls and a store the ROM lacks in
  one case, an inner `switch` with its cases in the wrong order and a flag set in the wrong arm, and
  `WifiBattleMatchNet_StartReport` tested the wrong parameter. Before respelling a large function, check its
  statements against the asm case by case.
- A `bl` that the probe masks can go to a linker veneer's symbol instead of the function: five calls of
  `btl_server_flow.c` named veneers (through local prototypes) and compiled to matching bytes. The call-target check
  has to follow veneers (`ov167a_calls.py` in that session did); the battle overlay has 45 of them in one file's
  range. In the same overlay a pointer to `const` was the shared cause of eight nonmatching functions (the damage
  list's five readers, a move param, the stat-change and cure handlers' params): when sibling functions all differ
  by a load placed against a store or call, try `const` on the pointee they share before anything else.
- A helper's local initializer joins the size sort where the helper is defined, and a `static inline` defined after
  its caller is still inlined, so where a helper sits moves equal-size initializers without touching code. In a
  frame, the caller's own locals take the highest slots in declaration order, then each inlined call's locals follow
  in call order, with the locals inside one helper ascending. The Pokédex cover's `ZukanOpen_InitBG` sets up its five
  BGs through five one-BG inlines, three defined before it and two after, which gives both the ROM's falling stack
  slots and the order of its two equal-size initializers. Which helpers sit after the caller is a fit; a table the
  original link dead-stripped would explain the ROM as well.
  When equal-size initializers resist every declaration order, search every listing order the stack slots allow,
  not single moves: the caller keeps a prefix of them, and each of the rest goes into a one-local inline called in
  order and defined after the caller where it needs to be listed. Overlay 199's `PdwAccDisp_CreateBGs` needs exactly
  two, BG 7's helper defined before BG 6's; no single helper, half of the eight, or count of end-of-list scalars
  does it, so the dead-stripped table its row once suspected was never needed.
- When a layout search needs N extra small objects, first ask whether an object already in the ROM is N + 1 smaller
  ones. The `08 08 08` at the head of `gimmick_pokewood_poster.c`'s `.rodata` is not one unreferenced 3-byte global
  but three one-byte objects in the packed section (function-local `static const u8` whose reads fold): the bytes are
  the same, and three objects listed after the name tables are what sorts the 12-byte string literals into the ROM's
  order. A function-local `static const` scalar stays in the packed section although its reads fold, a file-scope
  one is dropped; a used `.bss` static joins the sort, an unused one does not.
- A function-local `static` joins the size sort at the end of the file, after every local initializer, grouped by
  function: the functions come in reverse order, each with its statics in declaration order. Unused ones are still
  emitted and sorted, and a `00` in the packed section can be such a static of 0 rather than padding. So these
  statics reorder equal-size local initializers anywhere in the file: overlay 311's `un_select.c` keeps its two
  windows' background color in function-local `static const u8 backColor = 15;` (the `0f 0f` at the head of its
  `.rodata`), and its five 24-byte `UnSelectActorTemplate` initializers come out in the ROM's order only with
  `sUnSelectListTouch` defined first among the file's tables. Overlay 207's `p_sta_sub.c`, whose 17 12-byte
  initializers no declaration order had reproduced, matches with four: `turnShift = FX32_CONST(25)` in
  `PStaSub_Main`, `iconY = 127` and `defaultSequence = 0` in `PStaSub_CreateActors`, `spinRange = 0x1800` in
  `PStaSub_AnimCircle` (the `7f 00 00 18 00 90 01 00` that nothing reads), and its icon tables defined just before
  `PStaSub_CreateActors`. With three no order works; the count, not the sizes, decided the initializers' order.
  Give `rodata_order.py` such statics last, grouped that way. Its `--check` compares only the shared section, not
  the order of the objects with sections of their own, and it marks a string literal in `.data` as a local
  initializer, though string literals don't take part in the sort.
  It is about scalars, not only function-local ones: a file-scope `static const` scalar whose address is taken
  joins that end of the list too, after its function's own statics, in the group of the first function to use it
  (each group: the function's statics in declaration order, then the file-scope scalars it first uses, in use
  order). Arrays and structs, a function-local `static const` array included, are listed where they are defined.
  Overlay 208's `dup_fitting.c` gets the ROM's `.rodata` with its three `BOOL`s passed by address as function-local
  `static const BOOL` and its `{227, 128}` as two `static const u16` scalars of the functions that compare with them;
  as one array no order of its 13 file-scope objects works. About 120 orders of those objects then fit, so the
  file's order is one of them and says so. The compiled object's symbol table lists data objects in their sorted
  order across sections, ties included, which checks a model far better than `--check`.

## When nothing moves it

- A Thumb `bx pc` trampoline followed by an ARM `ldr ip, [pc, #0]; bx ip; dcd ADDRESS` needs separate functions with
  `#pragma thumb off` around the ARM function. `card_pull_out.c`'s two DSi jump stubs also need a Thumb `nop` after
  `bx pc` to reproduce the alignment halfword. The probe reports these Thumb functions as two bytes long in the ROM,
  excluding that padding; the full module check includes it. A C function pointer call makes each ARM part eight
  bytes too large.

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
