#ifndef POKEBW2_BATTLE_BTLV_SCD_H
#define POKEBW2_BATTLE_BTLV_SCD_H

// Overlay 169's btlv_scd.c (named by the string "btlv_scd.c" it passes to GFL_HeapAllocate), the battle view's
// touch screen controller: it owns the BtlvInput of overlay 168 (the battle's lower screen) and runs the command,
// move, rotation, target, yes/no and recorder screens on it for BtlvCore (btlv_core.c), the only caller. The names
// are ours; swan has none for this file (its disassembly calls the two empty functions nullsub_34 and nullsub_33)

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The cursor stops of btlv_input.c's target screen and their counts, by [pokeIndex][range], for doubles and triples
extern const u8 data_ov169_0689e218[2][15];
extern const u8 data_ov169_0689e238[3][15];
extern const BtlvInputKeyStop *data_ov169_0689e6e0[2][15];
extern const BtlvInputKeyStop *data_ov169_0689e884[3][15];

// scdState is passed on to the input; the font is the one BtlvCore made. The TCB manager is not used
BtlvScd *BtlvScd_Create(BtlvCore *core, BtlMainModule *mainModule, BtlPokeCon *pokeCon, TCBExManager *tcbManager,
                        Font *font, BtlClient *client, u32 scdState, HeapID heapId);
void BtlvScd_Delete(BtlvScd *scd);
// Creates the BtlvInput and enables BG 4 of the lower screen
void BtlvScd_CreateInput(BtlvScd *scd);

// Fades the lower screen out and in; mode is passed to BtlvInput_StartPaletteFade
void BtlvScd_StartFadeOut(BtlvScd *scd, u32 mode);
void BtlvScd_StartFadeIn(BtlvScd *scd);
BOOL BtlvScd_IsFadeDone(BtlvScd *scd);
// Empty, called when a fade out is done and right after a fade in starts
void BtlvScd_OnFadeOutDone(BtlvScd *scd);
void BtlvScd_OnFadeInStart(BtlvScd *scd);

// Goes back to the standby screen (no buttons), and does so only when another screen is shown
void BtlvScd_ResetScreen(BtlvScd *scd);
void BtlvScd_ResetScreenIfShown(BtlvScd *scd);
BOOL BtlvScd_IsIdle(BtlvScd *scd);

// The command screen of mon, which is the slot-th to choose this turn; the choice is the action's. The result is 0
// until the choice is made, then the command: 1 fight, 2 bag, 3 Pokémon, 4 run
void BtlvScd_StartCommandSelect(BtlvScd *scd, BattleMon *mon, u8 slot, u8 shooterEnergy, BattleAction *action);
// The same screen with the finger demo of the tutorial, whose result is 1 once it is done
void BtlvScd_StartCommandDemo(BtlvScd *scd, BattleMon *mon, u8 slot, BattleAction *action);
u32 BtlvScd_GetCommandResult(BtlvScd *scd);
// The move screen, and the same with the finger demo of the tutorial. They are done when BtlvScd_IsDone says so
void BtlvScd_StartMoveSelect(BtlvScd *scd, BattleMon *mon, BattleAction *action);
void BtlvScd_StartMoveDemo(BtlvScd *scd, BattleMon *mon, BattleAction *action);
// The move screen of a rotation battle, with the moves of its three mons
void BtlvScd_StartRotationSelect(BtlvScd *scd, BtlvSelectTargetParam *param, BattleAction *action);
// Goes back to the move screen that was started last, after the target screen was cancelled
void BtlvScd_ResumeMoveSelect(BtlvScd *scd);
BOOL BtlvScd_IsDone(BtlvScd *scd);
// The target screen of the move in action
void BtlvScd_StartTargetSelect(BtlvScd *scd, BattleMon *mon, BattleAction *action);
BOOL BtlvScd_IsTargetSelectDone(BtlvScd *scd);
// Whether the target was chosen and not cancelled
BOOL BtlvScd_IsTargetChosen(BtlvScd *scd);
// Drops the procedures that are running
void BtlvScd_CancelProcs(BtlvScd *scd);

// The rotation screen, and its input: the button pressed, if any
void BtlvScd_ShowRotationScreen(BtlvScd *scd, void *param);
BOOL BtlvScd_PollRotationInput(BtlvScd *scd, u8 *button);
void BtlvScd_HideRotationScreen(BtlvScd *scd);

// The yes/no screen of a BtlvInputYesNoParam, and its answer once it is given
void BtlvScd_ShowYesNo(BtlvScd *scd, void *param);
BOOL BtlvScd_PollYesNo(BtlvScd *scd, u32 *answer);

// Whether the player uses the keys, which the input sets; and ending the input at once (a forced quit)
u8 *BtlvScd_GetKeyCursorFlag(BtlvScd *scd);
void BtlvScd_ForceQuitInput(BtlvScd *scd);
BOOL BtlvScd_IsForceQuitInputDone(BtlvScd *scd);

// The recorder screen of a BtlvInputRecorderParam and its buttons
void BtlvScd_ShowRecorderScreen(BtlvScd *scd, void *param);
u32 BtlvScd_CheckRecorderInput(BtlvScd *scd);

// The type icons that the move screen can show, and their animations (named after BtlvInput's functions)
void BtlvScd_SetTypeIconVisible(BtlvScd *scd, BOOL visible, int index, int sequence);
u32 BtlvScd_GetTypeMask(BtlvScd *scd);
void BtlvScd_StartTypeIconAnim(BtlvScd *scd, int index);

#endif // POKEBW2_BATTLE_BTLV_SCD_H
