#ifndef POKEBW2_BATTLE_BTLV_H
#define POKEBW2_BATTLE_BTLV_H

#include "types.h"
#include "battle/b_plist_main.h"
#include "battle/btl_action.h"
#include "battle/btl_pokeparam.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "struct_decls.h"

// How a BtlvStringParam's message is made (BtlvCore_MakeString): a standard message (BtlString_FormatStdMsgArgs), a
// message about Pokémon (BtlString_FormatSetMsg), a UI string, a move's use, a message of learning a move, a yes/no
// choice, a trainer's win or lose line, a Pokéstar Studios movie's script and its movie's message
enum {
    BTLV_STR_STD = 1,
    BTLV_STR_SET,
    BTLV_STR_UI,
    BTLV_STR_MOVE,
    BTLV_STR_LEARN,
    BTLV_STR_YES_NO,
    BTLV_STR_TRAINER_WIN_LOSE,
    BTLV_STR_MOVIE_SCRIPT,
    BTLV_STR_MOVIE,
};

struct BtlvStringParam {
    u16 message;
    u8 mode;
    u8 type : 4;
    u8 count : 4;
    u32 args[9];
};

// The display commands with a procedure of their own (BtlvCore_StartCommand), and none running
enum {
    BTLV_CMD_BATTLE_START,
    BTLV_CMD_CAPTURE_DEMO,
    BTLV_CMD_NONE = 4,
};

// The battle's display: the BG, 2D and 3D systems, and the battle view and AI overlays
extern const BGSysLCDConfig BTLV_LCD_CONFIG;
extern const ClActSysSetup BTLV_CLACT_SETUP;
extern const BGSysVRAMConfig BTLV_VRAM_CONFIG;

void BtlvCore_InitSystem(HeapID heapId);
void BtlvCore_ExitSystem(void);
void BtlvCore_ResetGraphics(HeapID heapId);
void BtlvCore_InitGraphics(HeapID heapId);
void BtlvCore_ExitGraphics(void);

void Btlv_StringParam_Setup(BtlvStringParam *param, u32 type, u16 message);
void Btlv_StringParam_AddArg(BtlvStringParam *param, u32 arg);

typedef BOOL (*BtlvMainProc)(BtlvCore *core, s32 *seq, void *work);

// Called while a message prints
typedef BOOL (*BtlvMsgCallback)(u32 arg);

// A procedure of the battle display, which btlv_core.c and btlv_scu.c run the same way: init until it returns TRUE,
// then main
typedef BOOL (*BtlvSubProcFn)(s32 *seq, void *arg);

typedef struct BtlvSubProc {
    BtlvSubProcFn init;
    BtlvSubProcFn main;
    void *arg;
    s32 seq;
} BtlvSubProc;

// The mons a target is chosen among, which the client fills, and the action the display fills in
struct BtlvSelectTargetParam {
    struct {
        BattleMon *mon;
        u8 selectable[4];
    } mons[3];
    BattleAction rotateAction;
    BattleAction action;
};

// btlv_scu.c, the message and command unit of the battle display
BtlvScu *BtlvScu_Create(BtlvCore *core, BtlMainModule *mainModule, BtlPokeCon *pokeCon, TCBExManager *tcbManager,
                        Font *font, Font *smallFont, u8 clientId, HeapID heapId);
void BtlvScu_Setup(BtlvScu *scu);
void BtlvScu_Delete(BtlvScu *scu);
void BtlvScu_RestoreBG3(const BtlvScu *scu);
void BtlvScu_StartEncount(BtlvScu *scu, BOOL recPlay);
BOOL BtlvScu_WaitEncount(BtlvScu *scu);
void BtlvScu_PrintMsgAtOnce(BtlvScu *scu, const StrBuf *strbuf);
void BtlvScu_StartMsg(BtlvScu *scu, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback);
BOOL BtlvScu_IsMsgFinished(BtlvScu *scu);
BOOL BtlvScu_WaitMsg(BtlvScu *scu);
void BtlvScu_StartMoveEffect(BtlvScu *scu, u8 attackerPos, u8 targetPos, u16 move, s32 moveTarget, u32 variant,
                             u8 arg6);
BOOL BtlvScu_WaitMoveEffect(BtlvScu *scu);
void BtlvScu_StartDamage(BtlvScu *scu, u8 pos, u16 move, BOOL immediate);
BOOL BtlvScu_WaitDamage(BtlvScu *scu);
void BtlvScu_StartDamageEffect(BtlvScu *scu, u8 pos, u16 move);
BOOL BtlvScu_WaitDamageEffect(BtlvScu *scu);
void BtlvScu_StartFaint(BtlvScu *scu, u8 pos, BOOL immediate);
BOOL BtlvScu_WaitFaint(BtlvScu *scu);
void BtlvScu_ShowMon(BtlvScu *scu, u8 pos);
BOOL BtlvScu_WaitShowMon(BtlvScu *scu);
void BtlvScu_StartMemberOut(BtlvScu *scu, u8 pos, u16 effect, BOOL immediate);
BOOL BtlvScu_WaitMemberOut(BtlvScu *scu);
void BtlvScu_StartMemberIn(BtlvScu *scu, u8 pos, u8 clientId, u8 slot, BOOL noEffect);
BOOL BtlvScu_WaitMemberIn(BtlvScu *scu);
void BtlvScu_StartGaugeHP(BtlvScu *scu, u8 pos, BOOL immediate);
BOOL BtlvScu_WaitGaugeHP(BtlvScu *scu);
void BtlvScu_HideGaugePair(BtlvScu *scu, u8 pos1, u8 pos2);
BOOL BtlvScu_ShowGaugePair(BtlvScu *scu, u8 pos1, u8 pos2);
void BtlvScu_HideGauge(BtlvScu *scu, u8 pos);
BOOL BtlvScu_ShowGauge(BtlvScu *scu, u8 pos);
void BtlvScu_StartPosCueEffect(BtlvScu *scu, u8 viewPos);
BOOL BtlvScu_IsPosCueEffectDone(BtlvScu *scu, u8 viewPos);
void BtlvScu_StartChangeMon(BtlvScu *scu, u8 pos, u32 immediate);
BOOL BtlvScu_WaitChangeMon(BtlvScu *scu);
void BtlvScu_StartTransform(BtlvScu *scu, u32 viewPos, u32 targetViewPos, BOOL immediate);
void BtlvScu_StartChangeForm(BtlvScu *scu, u32 viewPos, BOOL immediate);
BOOL BtlvScu_WaitChangeSprite(BtlvScu *scu);
void BtlvScu_StartMsgWinHide(BtlvScu *scu);
BOOL BtlvScu_WaitMsgWinHide(BtlvScu *scu);
void BtlvScu_StartAbilityWinShow(BtlvScu *scu, u8 pos, BOOL flash);
BOOL BtlvScu_WaitAbilityWinShow(BtlvScu *scu, u8 pos);
void BtlvScu_StartAbilityWinHide(BtlvScu *scu, u8 pos);
BOOL BtlvScu_WaitAbilityWinHide(BtlvScu *scu, u8 pos);
void BtlvScu_StartAbilityWinChange(BtlvScu *scu, u8 pos);
BOOL BtlvScu_WaitAbilityWinChange(BtlvScu *scu, u8 pos);
void BtlvScu_StartWaitMsg(BtlvScu *scu);
BOOL BtlvScu_WaitWaitMsg(BtlvScu *scu);
void BtlvScu_HideWaitMsg(BtlvScu *scu);
void BtlvScu_StartFadeOut(BtlvScu *scu);
void BtlvScu_StartFadeIn(BtlvScu *scu);
BOOL BtlvScu_IsFadeDone(BtlvScu *scu);
void BtlvScu_StartLevelUpWinOpen(BtlvScu *scu, BattleMon *mon, const BattleMonLevelUp *levelUp);
BOOL BtlvScu_WaitLevelUpWinOpen(BtlvScu *scu);
void BtlvScu_StartLevelUpWinStats(BtlvScu *scu);
BOOL BtlvScu_WaitLevelUpWinStats(BtlvScu *scu);
void BtlvScu_StartLevelUpWinClose(BtlvScu *scu);
BOOL BtlvScu_WaitLevelUpWinClose(BtlvScu *scu);

BtlvCore *BtlvCore_Create(BtlMainModule *mainModule, BtlClient *client, BtlPokeCon *pokeCon, u32 commCompetitor,
                          HeapID heapId);
void BtlvCore_ResetGraphics(HeapID heapId);
void BtlvCore_Delete(BtlvCore *viewCore);

void BtlvCore_Update(BtlvCore *viewCore);

void BtlvCore_StartCommand(BtlvCore *core, u32 cmd);
BOOL BtlvCore_WaitCommand(BtlvCore *core);
void *BtlvCore_GetWork(BtlvCore *core, u32 size);
BOOL BtlvCore_CmdBattleStart(BtlvCore *core, s32 *seq, void *work);
BOOL BtlvCore_CmdCaptureDemo(BtlvCore *core, s32 *seq, void *work);
BattleMon *BtlvCore_GetMonAtViewPos(BtlvCore *core, u32 index);
BOOL BtlvCore_ProcCommandSelect(BtlvCore *core, s32 *seq, void *work);
BOOL BtlvCore_ProcTargetSelect(BtlvCore *core, s32 *seq, void *work);
void BtlvCore_SetMainProc(BtlvCore *core, BtlvMainProc proc);
void BtlvCore_ClearMainProc(BtlvCore *core);
BOOL BtlvCore_RunMainProc(BtlvCore *core);
void BtlvCore_StartCommandSelect(BtlvCore *core, BattleMon *mon, u32 laterMon, BattleAction *action);
void BtlvCore_StopCommandSelect(BtlvCore *core);
// What the player picked on the command screen, BTLV_COMMAND_* (NONE while it waits)
enum {
    BTLV_COMMAND_NONE,
    BTLV_COMMAND_FIGHT,
    BTLV_COMMAND_BAG,
    BTLV_COMMAND_POKEMON,
    BTLV_COMMAND_RUN,
};

u32 BtlvCore_WaitCommandSelect(BtlvCore *core);
void BtlvCore_StartMoveSelect(BtlvCore *core, BattleMon *mon, BattleAction *action);
void BtlvCore_StartRotationSelect(BtlvCore *core, BtlvSelectTargetParam *param, BattleAction *action);
void BtlvCore_ResumeMoveSelect(BtlvCore *core);
BOOL BtlvCore_ProcWaitSelect(BtlvCore *core, s32 *seq, void *work);
void BtlvCore_StopMoveSelect(BtlvCore *core);
BOOL BtlvCore_WaitMoveSelect(BtlvCore *core);
void BtlvCore_StartTargetSelect(BtlvCore *core, BattleMon *mon, BattleAction *action);
// Whether the target screen is done: a target chosen, or back to the moves (NONE while it waits)
enum {
    BTLV_TARGET_SELECT_NONE,
    BTLV_TARGET_SELECT_CHOSEN,
    BTLV_TARGET_SELECT_BACK,
};

u32 BtlvCore_WaitTargetSelect(BtlvCore *core);
void BtlvCore_StopTargetSelect(BtlvCore *core);
void BtlvCore_CancelSelect(BtlvCore *core);
void BtlvCore_ResetScreenIfShown(BtlvCore *core);
void BtlvCore_ResetScreen(BtlvCore *core);
BOOL BtlvCore_IsScreenIdle(BtlvCore *core);
BOOL BtlvCore_CanPlayListSE(BtlvCore *core);
void BtlvCore_SetupPokeListParam(BtlvCore *core, BPlistParam *param, u8 mode, u8 partyIndex, u32 move);
void BattleClientCmd_StartPokeList(BtlvCore *core, const BtlvPokeListCmd *cmd, s32 partyIndex, u16 move,
                                   BtlvPokeSelectParam *select);
void BattleClientCmd_QuitPokeSelect(BtlvCore *core);
BOOL BattleClientCmd_WaitPokeSelect(BtlvCore *core);
void BattleClientCmd_StartMoveInfoView(BtlvCore *core, u8 partyIndex, u8 slot);
void BattleClientCmd_StartItemSelect(BtlvCore *core, u32 mode, u8 shooterEnergy, u8 shooterSpent, u8 canUseItems,
                                     u8 targetHiding);
void BtlvCore_QuitItemSelect(BtlvCore *core);
BOOL BtlvCore_WaitItemSelect(BtlvCore *core);
u16 BtlvCore_GetSelectedItem(BtlvCore *core);
u8 BtlvCore_GetItemCost(BtlvCore *core);
u8 BtlvCore_GetSelectedPartyIndex(BtlvCore *core);
u8 BtlvCore_GetSelectedMoveSlot(BtlvCore *core);
void BtlvCore_SaveBagCursor(BtlvCore *core);
void BtlvCore_StartScenario(BtlvCore *core, u32 scene);
void BtlvCore_QuitScenario(BtlvCore *core);
BOOL BtlvCore_WaitScenario(BtlvCore *core);
void BtlvCore_StartChoice(BtlvCore *core);
void BtlvCore_SetupChoice(BtlvCore *core, u32 count, u32 *result);
void BtlvCore_SetChoiceStr(BtlvCore *core, u32 index, StrBuf *value);
BOOL BtlvCore_WaitChoice(BtlvCore *core);
void BtlvCore_StartMoveEffect(BtlvCore *core, u8 attackerPos, u8 targetPos, u16 move, s32 moveTarget, u32 variant,
                              u8 arg6);
BOOL BtlvCore_WaitMoveEffect(BtlvCore *core);
void BtlvCore_StartDamage(BtlvCore *core, u16 move, u8 pos, u8 type);
BOOL BtlvCore_WaitDamage(BtlvCore *core);
BOOL BtlvCore_ProcDamage(s32 *seq, void *arg);
void BtlvCore_StartDamageEffect(BtlvCore *core, u16 move, u8 pos, u32 type);
BOOL BtlvCore_WaitDamageEffect(BtlvCore *core);
BOOL BtlvCore_ProcDamageEffect(s32 *seq, void *arg);
void BtlvCore_StartMultiDamage(BtlvCore *core, u16 count, u32 type, const u8 *monIds, u16 move);
BOOL BtlvCore_WaitMultiDamage(BtlvCore *core);
void BtlvCore_PlayDamageSE(BtlvCore *core, u32 type);
void BtlvCore_SetMonVisible(BtlvCore *core, u8 pos, u32 visible);
void BtlvCore_StartGaugeHP(BtlvCore *core, u8 pos);
BOOL BtlvCore_WaitGaugeHP(BtlvCore *core);
void BtlvCore_StartPosEffect(BtlvCore *core, u32 pos, u16 effect);
BOOL BtlvCore_WaitPosEffect(BtlvCore *core);
void BtlvCore_StartAtkDefEffect(BtlvCore *core, u32 atkPos, u32 defPos, u16 effect);
BOOL BtlvCore_WaitAtkDefEffect(BtlvCore *core);
void BtlvCore_StartFaint(BtlvCore *core, u8 pos);
BOOL BtlvCore_WaitFaint(BtlvCore *core);
void BtlvCore_StartShowMon(BtlvCore *core, u8 pos);
BOOL BtlvCore_WaitShowMon(BtlvCore *core);
void BtlvCore_StartSwitchOut(BtlvCore *core, u8 pos, u16 effect);
BOOL BtlvCore_WaitSwitchOut(BtlvCore *core);
void BtlvCore_StartSwitchIn(BtlvCore *core, u8 pos, u8 clientId, u8 slot);
BOOL BtlvCore_WaitSwitchIn(BtlvCore *core);
BOOL BtlvCore_ProcSwitchIn(s32 *seq, void *arg);
void BtlvCore_MakeString(const BtlvStringParam *param, StrBuf *strbuf);
void BtlvCore_CopyMsgStr(BtlvCore *core, const StrBuf *src);
void BtlvCore_MakeMsg(BtlvCore *core, const BtlvStringParam *param);
void BtlvCore_StartMadeMsg(BtlvCore *core);
void BtlvCore_StartMsg(BtlvCore *core, const BtlvStringParam *param);
void BtlvCore_StartMsgCallback(BtlvCore *core, const BtlvStringParam *param, BtlvMsgCallback callback);
void BtlvCore_PrintMsg(BtlvCore *core, const BtlvStringParam *param);
void BtlvCore_StartMsgStd(BtlvCore *core, u16 message, const u32 *args);
void BtlvCore_StartMsgSet(BtlvCore *core, u16 message, const u32 *args);
BOOL BtlvCore_StartMsgTrainer(BtlvCore *core, u32 trainerId, u32 msgId);
void BtlvCore_StartMsgMove(BtlvCore *core, u8 monId, u16 move);
BOOL BtlvCore_WaitMsg(BtlvCore *core);
BOOL BtlvCore_IsMsgJustDone(BtlvCore *core);
void BtlvCore_StartMsgStr(BtlvCore *core, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback);
void BtlvCore_PrintMsgStr(BtlvCore *core, const StrBuf *strbuf);
void BtlvCore_ShowAbilityWin(BtlvCore *core, u8 pos, BOOL flash);
BOOL BtlvCore_WaitShowAbilityWin(BtlvCore *core, u8 pos);
void BtlvCore_HideAbilityWin(BtlvCore *core, u8 pos);
BOOL BtlvCore_WaitHideAbilityWin(BtlvCore *core, u8 pos);
void BtlvCore_UpdateAbilityWin(BtlvCore *core, u8 pos);
BOOL BtlvCore_WaitUpdateAbilityWin(BtlvCore *core, u8 pos);
void BtlvCore_StartRankDownEffect(BtlvCore *core, u32 pos);
void BtlvCore_StartRankUpEffect(BtlvCore *core, u32 pos);
BOOL BtlvCore_WaitRankEffect(BtlvCore *core, u32 pos);
void BtlvCore_StartWaitMsg(BtlvCore *core);
BOOL BtlvCore_IsWaitMsgDone(BtlvCore *core);
void BtlvCore_EndWaitMsg(BtlvCore *core);
void BtlvCore_StartUseItemEffect(BtlvCore *core, u8 pos);
BOOL BtlvCore_WaitUseItemEffect(BtlvCore *core, u8 pos);
void BtlvCore_StartIllusionBreak(BtlvCore *core, u8 pos, u32 skip);
BOOL BtlvCore_WaitIllusionBreak(BtlvCore *core);
void BtlvCore_StartFormChange(BtlvCore *core, u32 pos);
BOOL BtlvCore_WaitFormChange(BtlvCore *core);
void BtlvCore_StartTransform(BtlvCore *core, u32 pos, u32 targetPos);
BOOL BtlvCore_WaitTransform(BtlvCore *core);
void BtlvCore_StartHideMsgWin(BtlvCore *core);
BOOL BtlvCore_WaitHideMsgWin(BtlvCore *core);
void BtlvCore_StartSwapPokemon(BtlvCore *core, u8 clientId, u8 viewPos1, u8 viewPos2, u8 slot1, u8 slot2);
BOOL BtlvCore_WaitSwapPokemon(BtlvCore *core);
BOOL BtlvCore_ProcSwapPokemon(s32 *seq, void *arg);
void BtlvCore_ShowRotationScreen(BtlvCore *core, u8 dir);
BOOL BtlvCore_PollRotationInput(BtlvCore *core, u8 *out);
void BtlvCore_HideRotationScreen(BtlvCore *core);
void BtlvCore_StartRotation(BtlvCore *core, u8 clientId, u8 dir);
BOOL BtlvCore_WaitRotation(BtlvCore *core);
BOOL BtlvCore_ProcRotation(s32 *seq, void *arg);
void BtlvCore_ShowYesNo(BtlvCore *core, const BtlvStringParam *yes, const BtlvStringParam *no, u32 canCancel);
BOOL BtlvCore_PollYesNo(BtlvCore *core, u32 *answer);
void BtlvCore_StartForgetMoveSelect(BtlvCore *core, u8 partyIndex, u16 move);
BOOL BtlvCore_WaitForgetMoveSelect(BtlvCore *core, u8 *slot);
void BtlvCore_StartLevelUpWin(BtlvCore *core, BattleMon *mon, const BattleMonLevelUp *levelUp);
BOOL BtlvCore_WaitLevelUpWin(BtlvCore *core);
void BtlvCore_StartLevelUpStats(BtlvCore *core);
BOOL BtlvCore_WaitLevelUpStats(BtlvCore *core);
void BtlvCore_StartCloseLevelUpWin(BtlvCore *core);
BOOL BtlvCore_WaitCloseLevelUpWin(BtlvCore *core);
void BtlvCore_StartFadeOut(BtlvCore *core);
BOOL BtlvCore_WaitFadeOut(BtlvCore *core);
void BtlvCore_StartFadeIn(BtlvCore *core);
BOOL BtlvCore_WaitFadeIn(BtlvCore *core);
void BattleClientCmd_ForceQuitInputNotify(BtlvCore *core);
BOOL BattleClientCmd_ForceQuitInputWait(BtlvCore *core);
// The recorder's buttons during a playback: the previous chapter, the next one, and quit
enum {
    BTLV_RECORDER_PREV,
    BTLV_RECORDER_NEXT,
    BTLV_RECORDER_QUIT,
};

u32 BtlvCore_CheckRecorderInput(BtlvCore *core);
void BtlvCore_ShowRecorderChapter(BtlvCore *core, u16 chapter, u16 targetChapter);
void BtlvCore_ShowRecorderSkipping(BtlvCore *core, u16 chapter);
void BtlvCore_ShowRecorderMessage(BtlvCore *core, u32 chapter, u32 mode);
void BtlvCore_PlaySE(BtlvCore *core, u32 se);
void Btlv_StringParam_SetWait(BtlvStringParam *param, u8 mode);
void BtlvCore_SetClactGauge(BtlvCore *core, u32 arg1, u32 arg2);
void BtlvCore_ShowClactPopup(BtlvCore *core, u32 number);
BOOL BtlvCore_IsClactPopupShowing(BtlvCore *core);
void BtlvCore_SetTypeIconVisible(BtlvCore *core, u32 visible, u32 index, u32 sequence);
u32 BtlvCore_GetTypeMask(BtlvCore *core);
void BtlvCore_StartTypeIconAnim(BtlvCore *core, u32 index);

#endif // POKEBW2_BATTLE_BTLV_H
