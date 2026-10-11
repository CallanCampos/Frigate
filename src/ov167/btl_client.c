#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_adapter.h"
#include "battle/btl_calc.h"
#include "battle/btl_client.h"
#include "battle/btl_field.h"
#include "battle/btl_main.h"
#include "battle/btl_ov169.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_rec.h"
#include "battle/btl_server.h"
#include "battle/btl_server_cmd.h"
#include "battle/btl_setup.h"
#include "battle/btl_string.h"
#include "battle/btlv.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_mcss.h"
#include "battle/pokewood_cutin.h"
#include "battle/tr_ai.h"
#include "battle/trainer_data.h"
#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/types.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "system/pms_data.h"

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

// The playback of a recorded battle
typedef struct {
    u8 seq;
    // 1 or 2 when the playback stopped
    u8 result;
    u8 skipping : 1;
    u8 quitStarted : 1;
    u8 quitDone : 1;
    u8 chapterReached : 1;
    u8 unk2_4 : 1;
    u8 dataEnd : 1;
    u16 quitTimer;
    u16 chapter;
    u16 skipTarget;
    u16 maxChapter;
    u16 skipCount;
} BtlClientRecPlayer;

// A Pokestar Studios movie's score
typedef struct {
    s16 points[4];
    s16 unk08;
    s16 unk0A;
    u8 counts[4];
    u32 unk10;
    // Clamped to +-9999
    s32 total;
} BtlClientStudioScore;

// A Pokestar Studios movie
typedef struct {
    u8 seq;
    u8 nextSeq;
    u8 unk02;
    u8 unk03;
    u8 unk04;
    u8 unk05;
    // The entry of the movie's script
    s8 scene;
    u32 unk08;
    s32 unk0C;
    s8 unk10;
    u8 unk11[0xb];
    s32 msgId1;
    s32 msgId2;
    u16 unk24;
    u16 unk26;
    // One bit per script entry used
    u32 usedEventMask;
    s32 unk2C;
    StrBuf *strBufs[4];
    BtlClientStudioScore score;
    u16 unk58;
    u32 audienceMask;
    u16 audienceTimers[22];
    u8 waitTimer;
} BtlClientStudioWork;

struct BtlClient {
    BtlMainModule *mainModule;
    BtlPokeCon *pokeCon;
    BattleMon *procMon;
    BattleAction *procAction;
    void *recorder;
    void *recReader;
    BtlClientRecPlayer recPlayer;
    BOOL (*mainProc)(BtlClient *client);
    BtlClientIDList clientIdList;
    BtlField *field;
    MATHRandContext32 targetRand;
    BtlAdapter *adapter;
    BtlvCore *viewCore;
    BtlvStringParam strParam;
    BtlvStringParam strParam2;
    BtlServer *cmdCheckServer;
    BtlvSelectTargetParam rotationParam;
    u8 unkCC;
    BOOL (*cmdProc)(BtlClient *client, s32 *seq);
    s32 cmdSeq;
    BOOL (*selActProc)(BtlClient *client, s32 *seq);
    s32 selActSeq;
    const void *returnData;
    u32 returnDataSize;
    u32 dummyReturnData;
    u16 cmdLimitTime;
    u16 gameLimitTime;
    u16 aiItems[4];
    VM *aiVM;
    MATHRandContext32 aiRand;
    s8 aiSwitchReserved[6];
    u8 hintShown[4];
    BattleParty *party;
    u8 numCoverPos;
    u8 procActionIdx;
    s8 prevActionIdx;
    u8 firstActionIdx;
    u8 forceActionMsg;
    u8 unk129;
    BattleAction actions[3];
    u8 shooterCost[3];
    BtlServerCmdQueue *cmdQueue;
    u32 cmdArgs[16];
    u32 serverCmd;
    BOOL (*serverCmdProc)(BtlClient *client, s32 *seq, const u32 *args);
    s32 serverCmdSeq;
    BtlvPokeListCmd pokeListCmd;
    BtlvPokeSelectParam pokeSelect;
    u16 heapId;
    u16 savedHp;
    u16 hintMsgId;
    u16 reservedItems[3];
    u8 clientId;
    // 0 for the player, 1 for the AI, 2 for a recorded battle's playback
    u8 clientType;
    u8 mainSeq;
    u8 waitMsgShown;
    u8 bagMode;
    u8 shooterEnergy;
    u8 yesNoResult;
    u8 cmdLimitOver;
    u8 cmdCheckReq;
    u8 extraActionCount;
    u8 moveInfoPos;
    u8 moveInfoIdx;
    u8 unk1BA_0 : 1;
    u8 forceQuit : 1;
    u8 escapeSelected : 1;
    u8 cmdCheckEnable : 1;
    u8 unk1BA_4 : 1;
    u8 unk1BA_5 : 1;
    u8 changePokeCount;
    u8 unk1BC;
    u8 turnCount;
    u8 changePokePos[6];
    BtlClientStudioWork studio;
};

// A step of what a client does for a command, run until it returns TRUE
typedef BOOL (*BtlClientCmdProc)(BtlClient *client, s32 *seq);

// The move the experience command's level-up is learning
static u16 sLearnMove;
// The frames of the ability swap's animation
static u32 sSwapTimer;
// The experience left to add
static u32 sExpLeft;
// The sequence of the move learning, and its timer
static s32 sExpSeq;
// Where the search for the moves learned at the new level goes on
static u32 sLearnIdx;
static BattleMonLevelUp sLevelUp;

static BtlClientCmdProc BattleClient_GetCmdProc(BtlClient *client, u32 cmd, u32 *unk01);
static void BattleClient_SetMainProc(BtlClient *client, BOOL (*mainProc)(BtlClient *client));
static BOOL BattleClient_MainProcNormal(BtlClient *client);
static BOOL BattleClient_MainProcRecPlay(BtlClient *client);
static void BattleClient_SetDummyReturnData(BtlClient *client);
static void func_ov167_021b1da0(BtlClientStudioWork *studio);
static BOOL func_ov167_021b1dcc(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b1ea4(BtlClient *client, s32 *seq);
static void func_ov167_021b1ee4(BtlClient *client);
static u32 func_ov167_021b1f00(BtlClient *client);
static BattleMon *func_ov167_021b1f34(BtlClient *client);
static BOOL func_ov167_021b1f70(BtlClient *client, s32 *seq);
static u8 BattleClient_PickRotation(u8 dir);
static BOOL func_ov167_021b2058(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b205c(BtlClient *client, s32 *seq);
static void func_ov167_021b2080(BtlClient *client);
static BOOL CheckIfOverCmdLimit(BtlClient *client);
static BOOL CheckActionSelectForceQuit(BtlClient *client, BOOL (*proc)(BtlClient *client, s32 *seq));
static void func_ov167_021b20f8(BtlClient *client);
static void func_ov167_021b2110(BtlClient *client, BattleMon *mon, BtlvStringParam *param);
static void BattleClient_SetSelActProc(BtlClient *client, BOOL (*proc)(BtlClient *client, s32 *seq));
static BOOL BattleClient_RunSelActProc(BtlClient *client);
static s32 func_ov167_021b2188(void);
static u32 func_ov167_021b2194(BtlClient *client);
static void func_ov167_021b2210(BtlClient *client, u32 value);
static BOOL func_ov167_021b2218(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b226c(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b23b8(BtlClient *client, s32 *seq);
static void Studio_SetSeq(BtlClientStudioWork *studio, u8 seq, u8 nextSeq);
static BOOL func_ov167_021b23e4(BtlClient *client);
static s32 func_ov167_021b2538(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn, s32 arg3);
static BOOL func_ov167_021b261c(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn);
static void func_ov167_021b264c(const BtlScriptedRules *rules, BtlClientStudioWork *studio);
static BOOL func_ov167_021b2674(const BtlScriptedRules *rules, BtlClientStudioWork *studio);
static u32 func_ov167_021b26a8(BtlClient *client, BtlClientStudioWork *studio);
static void func_ov167_021b26c4(BtlClient *client, s32 scene, u32 value);
static u32 func_ov167_021b26dc(BtlClient *client);
static void func_ov167_021b26ec(BtlClient *client, u32 value);
static void func_ov167_021b2700(BtlClient *client, const BtlScriptedRules *rules, s32 scene);
static void func_ov167_021b2748(BtlClient *client, const BtlScriptedRules *rules, s32 scene);
static void func_ov167_021b2790(BtlClientStudioWork *studio);
static void func_ov167_021b2798(BtlClient *client, const BtlScriptedRules *rules, BtlClientStudioWork *studio);
static void Studio_FreeChoiceStrBufs(BtlClientStudioWork *studio);
static BOOL func_ov167_021b2864(BtlClient *client);
static void func_ov167_021b2cf0(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 scene, u32 value);
static s32 Studio_GetNextState(BtlClientStudioWork *studio, s32 event);
static void func_ov167_021b2dd0(BtlClientStudioWork *studio, u8 nextSeq);
static BOOL func_ov167_021b2dd8(const BtlScriptedRules *rules, BtlClient *client);
static s16 StudioRules_GetTurnEvent(const BtlScriptedRules *rules, u8 turn);
static BOOL func_ov167_021b2fb8(BtlClient *client);
static u32 Studio_GetResultMsgId(BtlClient *client, const BtlScriptedRules *rules, BtlSetup *setup);
static BOOL func_ov167_021b3100(BtlClient *client);
static BOOL RecPlaySelectActionCore(BtlClient *client, s32 *seq, BOOL arg2);
static BOOL RecPlaySelectAction(BtlClient *client, s32 *seq);
static void SetNullReturnAction(BtlClient *client);
static BOOL BattleClient_ActionSelectInit(BtlClient *client, s32 *seq);
static void func_ov167_021b350c(BtlClient *client, const BtlvStringParam *param);
static BOOL BattleClient_ActionForceQuit(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectRoot(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionTrainerHint(BtlClient *client, s32 *seq);
static BOOL CheckTrainerHintMsg(BtlClient *client, u16 *msgId);
static BOOL BattleClient_ActionSelectFight(BtlClient *client, s32 *seq);
static void SetupRotationSelectParam(BtlClient *client, BtlvSelectTargetParam *param);
static BOOL BattleClient_ActionViewMoveInfo(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSwitchPokemon(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectItem(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b4168(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b41d4(BtlClient *client, s32 *seq);
static BOOL IsPokeBallTargetHiding(BtlClient *client);
static BOOL BattleClient_ActionSelectEscape(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectAutoRest(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectFinish(BtlClient *client, s32 *seq);
static void selItemWork_Init(BtlClient *client);
static void selItemWork_Reserve(BtlClient *client, u8 index, u16 item);
static void selItemWork_Restore(BtlClient *client, u8 index);
static void selItemWork_Quit(BtlClient *client);
static void shooterWork_Init(BtlClient *client);
static void shooterWork_SetCost(BtlClient *client, u8 index, u8 cost);
static u8 shooterWork_GetCost(BtlClient *client, u8 index);
static u8 shooterWork_GetTotalCost(BtlClient *client);
static BOOL AutoSelectAction(BtlClient *client, BattleMon *mon, BattleAction *action);
static BOOL CheckForSelectableMove(BtlClient *client, BattleMon *mon, BattleAction *action);
static void SetStruggleAction(BattleAction *action, BtlClient *client, BattleMon *mon);
static BOOL IsUnselectableMove(BtlClient *client, BattleMon *mon, u16 move, BtlvStringParam *param);
static u8 StoreSelectableMoveFlag(BtlClient *client, BattleMon *mon, u8 *selectable);
static BOOL CanMonUseHeldItem(BtlClient *client, BattleMon *mon);
static u32 CanMonSwitch(BtlClient *client, BattleMon *mon, u8 *trapMonId, u16 *trapAbility);
static u32 CheckEscapeBlocked(BtlClient *client, u8 *trapMonId, u16 *trapAbility);
static u32 IsMonTrapped(BtlClient *client, BattleMon *mon, u8 *trapMonId, u16 *trapAbility);
static BOOL DoesMonHaveShadowTag(BtlClient *client, BattleMon *mon);
static BOOL IsMonTrappedByArenaTrap(BtlClient *client, BattleMon *mon);
static BOOL IsMonSteelType(BtlClient *client, BattleMon *mon);
static void ClearAISwitchReserved(BtlClient *client);
static BOOL IsAISwitchReserved(BtlClient *client, u8 slot);
static void ReserveAISwitch(BtlClient *client, u8 index, u8 slot);
static BOOL AISwitchChecks(BtlClient *client, BattleMon *mon, u8 index, u8 *slot);
static BattleMon *PickRandomOpponent(BtlClient *client, u8 pos);
static BOOL ShouldSwitchIfPerishSongLastTurn(BtlClient *client, BattleMon *mon);
static BOOL ShouldSwitchIfWonderGuard(BtlClient *client, BattleMon *mon, BattleMon *target);
static BOOL ShouldSwitchIfNoEffectiveMoves(BtlClient *client, BattleMon *mon, BattleMon *target);
static BOOL ShouldSwitchIfChoicedIntoIneffectiveMove(BtlClient *client, BattleMon *mon, BattleMon *target);
static BOOL ShouldSwitchIfTypeAbsorbingAbility(BtlClient *client, BattleMon *mon, BattleMon *target, u8 *slot);
static BOOL ShouldSwitchIfAsleepWithNaturalCure(BtlClient *client, BattleMon *mon, u8 *slot);
static BOOL FinalSwitchChecks(BtlClient *client, BattleMon *mon, BattleMon *target, u8 *slot);
static BOOL CheckMonsForTypeAbsorbingAbility(BtlClient *client, u16 ability, u8 *slot);
static BOOL CheckIfMonToSwitchToWithSEMove(BtlClient *client, BattleMon *target, s32 minEffectiveness);
static BOOL FindPartyMonResistingType(BtlClient *client, u8 moveType, s32 maxEffect, u8 *partyIdx);
static BOOL DoesMonHaveSuperEffectiveMove(BtlClient *client, BattleMon *attacker, BattleMon *defender, s32 minEffect);
static u8 AIPickRotationPos(BtlClient *client);
static BOOL AISelectAction(BtlClient *client, s32 *seq);
static u8 GetNumBattleReadyPartyMons(BtlClient *client, u8 *list);
static void PickBestMonToSwitchInto(BtlClient *client, u8 *list, u8 count, BattleMon *defender);
static void func_ov167_021b5d7c(BtlClient *client, u8 mode, u8 count, BtlvPokeListCmd *cmd,
                                BtlvPokeSelectParam *select);
static void func_ov167_021b5dd8(BtlClient *client, const BtlvPokeSelectParam *select);
static void func_ov167_021b5e1c(BtlClient *client);
static u8 CollectOwnChangePositions(BtlClient *client, u8 *positions);
static BOOL func_ov167_021b5f30(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b5fa4(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b5fb0(BtlClient *client, s32 *seq);
static BOOL AISelectChangePoke(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b60e0(BtlClient *client, s32 *seq);
static BOOL PlayerSelectChangePoke(BtlClient *client, s32 *seq, u8 mode);
static BOOL IsRotationFrontFainted(BtlClient *client);
static u32 RotateToLivingMon(BtlClient *client, BattleAction *action);
static BOOL func_ov167_021b6328(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b64ac(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b64ec(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6500(BtlClient *client, s32 *seq);
static u32 GetBattleResultForClient(BtlClient *client);
static BOOL func_ov167_021b666c(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6680(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6a98(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6aac(BtlClient *client, s32 *seq);
static void ShowTrainerWinLoseMsg(BtlClient *client, u32 result, u8 which);
static BOOL func_ov167_021b6c98(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6d0c(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6dc4(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6e0c(BtlClient *client, s32 *seq);
static BOOL BattleClient_ServerCmdLoop(BtlClient *client, s32 *seq);
static BOOL func_ov167_021b6f7c(BtlClient *client, s32 *seq, const u32 *args);
static u16 BattleClient_GetRecallMsg(BtlClient *client, u8 clientId, BOOL *withName);
static BOOL func_ov167_021b7058(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b70a0(BtlClient *client, s32 *seq, const u32 *args);
static u16 BattleClient_GetPlayerSendOutMsg(BtlClient *client);
static BOOL func_ov167_021b7230(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7278(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b72d4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b731c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7378(BtlClient *client, s32 *seq, const u32 *args);
static s32 BattleClient_GetMoveAnimType(BtlClient *client, u16 move, u8 pos);
static BOOL func_ov167_021b73f4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7480(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b74d0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7520(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7598(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b75f8(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b763c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b76e4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7750(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7794(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7800(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScWeatherStart(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScWeatherEnd(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b795c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b79a0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7a04(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7a60(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7af4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7c08(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7c40(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScTransform(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7d24(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7d84(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b7d90(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScAddEVs(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScExpGain(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ForgetMoveSECallback(u32 arg0);
static BOOL BattleClient_LearnMoveSeq(BtlClient *client, s32 *seq, BattleMon *mon);
static BOOL func_ov167_021b83dc(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b853c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b85a4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8650(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b86e4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8850(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b88b4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b88ec(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b896c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b89cc(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8a34(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8a8c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8ad0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8af0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8b10(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8b38(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8b60(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8b80(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8b98(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8bbc(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8be0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8c00(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8c20(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8c98(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8cb0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8cc8(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8ce4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8d04(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8d1c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8d38(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8d8c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8dac(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8e24(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8e44(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8e80(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8e9c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8eb8(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8ed4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8ef0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8f0c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8f2c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8f60(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8f78(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8f8c(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8fa0(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8fb4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8fc4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b8fe4(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9010(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9030(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9048(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9074(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9094(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b90ac(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b90cc(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9100(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9120(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9130(BtlClient *client, s32 *seq, const u32 *args);
static BOOL func_ov167_021b9154(BtlClient *client, s32 *seq, const u32 *args);
static void BattleClient_LoadAIItems(BtlClient *client);
static u16 AICheckItemUse(BtlClient *client, BattleMon *mon, BattleParty *party);
static BOOL IsXItem(u16 item, BattleMon *mon);
static BOOL IsStatusCureItem(u16 item, BattleMon *mon);
static void RecPlayer_Init(BtlClientRecPlayer *recPlayer);
static void RecPlayer_SetMaxChapter(BtlClientRecPlayer *recPlayer, u32 maxChapter);
static BOOL func_ov167_021b9360(BtlClientRecPlayer *recPlayer);
static void func_ov167_021b9368(BtlClientRecPlayer *recPlayer);
static u32 func_ov167_021b9374(BtlClientRecPlayer *recPlayer);
static void RecPlayer_StartSkip(BtlClientRecPlayer *recPlayer, u32 target);
static void func_ov167_021b939c(BtlClientRecPlayer *recPlayer);
static BOOL func_ov167_021b93ac(BtlClientRecPlayer *recPlayer);
static u32 func_ov167_021b93bc(BtlClientRecPlayer *recPlayer);
static BOOL func_ov167_021b93c0(BtlClientRecPlayer *recPlayer);
static void func_ov167_021b93d8(BtlClientRecPlayer *recPlayer);
static void func_ov167_021b93e4(BtlClientRecPlayer *recPlayer);
static void RecPlayer_Update(BtlClient *client, BtlClientRecPlayer *recPlayer);
static void func_ov167_021b9570(BtlClientStudioWork *studio, u32 reaction);
static void Studio_ReactAudience(BtlvCore *core, BtlClientStudioWork *studio, u32 reaction);
static void Studio_AudienceLeave(BtlvCore *core, BtlClientStudioWork *studio, u16 level);
static void func_ov167_021b96b0(BtlvCore *core, BtlClientStudioWork *studio);
static void func_ov167_021b96e0(BtlvCore *core, BtlClientStudioWork *studio, int fame);
static void func_ov167_021b96f0(BtlClient *client, BtlClientStudioWork *studio, BattleParty *party);
static void func_ov167_021b9750(BtlClient *client, BtlClientStudioScore *score, s32 scene);
static void func_ov167_021b9774(BtlClient *client, BtlClientStudioScore *score);
static void func_ov167_021b97a4(BtlClient *client, BtlClientStudioScore *score, s32 scene, u32 index);
static s32 StudioScore_CalcTotal(BtlClient *client, BtlClientStudioScore *score);
static void Studio_UpdateAudience(BtlClient *client);
static void func_ov167_021b9910(BtlMainModule *mainModule);

// A server command a client handles, with its handler for each client type
typedef struct {
    u8 cmd;
    u8 unk01;
    BOOL (*procs[3])(BtlClient *client, s32 *seq);
} BtlClientCmdEntry;

static const u8 data_ov167_021d6eb4[5] = { 0, 0, 0, 1, 2 };

// 0x021d7070: the commands that the clients handle (also used by the other ranges' handlers)
static const BtlClientCmdEntry sCmdProcTable[15] = {
    { 1, 1, { func_ov167_021b1dcc, NULL, func_ov167_021b1ea4 } },
    { 2, 0, { func_ov167_021b1f70, func_ov167_021b205c, func_ov167_021b2058 } },
    { 3, 0, { func_ov167_021b23b8, AISelectAction, RecPlaySelectAction } },
    { 7, 0, { func_ov167_021b5f30, NULL, NULL } },
    { 5, 0, { func_ov167_021b5fa4, AISelectChangePoke, func_ov167_021b60e0 } },
    { 6, 0, { func_ov167_021b5fb0, AISelectChangePoke, func_ov167_021b60e0 } },
    { 4, 0, { func_ov167_021b6328, NULL, func_ov167_021b60e0 } },
    { 8, 0, { BattleClient_ServerCmdLoop, NULL, func_ov167_021b6e0c } },
    { 10, 0, { func_ov167_021b64ac, NULL, NULL } },
    { 11, 1, { func_ov167_021b6680, NULL, func_ov167_021b666c } },
    { 12, 1, { func_ov167_021b6aac, NULL, func_ov167_021b6a98 } },
    { 14, 1, { func_ov167_021b6c98, NULL, NULL } },
    { 15, 1, { func_ov167_021b6d0c, NULL, NULL } },
    { 13, 1, { func_ov167_021b6500, NULL, func_ov167_021b64ec } },
    { 16, 1, { func_ov167_021b6dc4, NULL, NULL } },
};

// The studio's next step for each state, by the event that happened
typedef struct {
    s32 state;
    s32 next[5];
} BtlStudioTransition;

static const BtlStudioTransition sStudioTransitions[7] = {
    { 0x00, { 1, 0, -1, 1, 2 } },   { 0x10, { 2, 3, 5, -1, -1 } },  { 0x11, { 0, 3, 5, 6, 5 } },
    { 0x01, { 1, 4, 6, -1, -1 } },  { 0x21, { -1, -1, 6, -1, 0 } }, { 0x12, { 8, -1, 5, 0, -1 } },
    { 0x22, { 7, -1, 0, -1, -1 } },
};

// A type and the abilities that absorb moves of it, for the AI's switch checks
typedef struct {
    u8 type;
    u16 abilities[4];
} TypeAbsorbingAbilities;

static const TypeAbsorbingAbilities sTypeAbsorbingAbilities[4] = {
    { TYPE_WATER, { ABILITY_WATER_ABSORB, ABILITY_STORM_DRAIN, ABILITY_DRY_SKIN, 0 } },
    { TYPE_ELECTRIC, { ABILITY_VOLT_ABSORB, ABILITY_MOTOR_DRIVE, ABILITY_LIGHTNINGROD, 0 } },
// BUG: Overgrow doesn't absorb Grass moves; Sap Sipper does
#ifdef BUGFIX
    { TYPE_GRASS, { ABILITY_SAP_SIPPER, 0, 0, 0 } },
#else
    { TYPE_GRASS, { ABILITY_OVERGROW, 0, 0, 0 } },
#endif
    { TYPE_FIRE, { ABILITY_FLASH_FIRE, 0, 0, 0 } },
};

// The item that restores a stat stage, and the stage, of each X item
typedef struct {
    u8 param;
    u8 stat;
} BtlClientItemStat;

// The item that cures a condition, and the condition
typedef struct {
    u8 param;
    u8 condition;
} BtlClientItemCondition;

// How many of the audience leave for a movie's score
typedef struct {
    u8 min;
    u8 max;
    u8 base;
    u8 range;
} BtlClientAudienceLeave;

// The sounds a reaction of the audience plays, before and after the twelfth
typedef struct {
    u32 se;
    u32 seLate;
} BtlClientAudienceSE;

// The most mons the AI may have left to use the item in each of its four slots
static const u8 sAIItemMaxMons[4] = { 6, 4, 2, 1 };
// The points a movie scores for its star's fame
// The messages of an escape, by its kind
BtlClient *BattleClient_Create(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 commMode, void *netHandle,
                               u16 clientId, u16 numCoverPos, u8 isAI, u32 arg7, BOOL recPlay, MATHRandContext32 *rand,
                               HeapID heapId) {
    BtlClient *client = GFL_HeapAllocate(heapId, sizeof(BtlClient), TRUE, "btl_client.c", 0x281);
    BOOL netFlag = TRUE;
    u32 i;
    u32 type;

    if (commMode == 0 || isAI) {
        netFlag = FALSE;
    }
    client->clientId = clientId;
    client->heapId = heapId;
    client->clientType = isAI;
    client->adapter = func_ov167_021d4a44(netHandle, clientId, netFlag, heapId);
    client->party = GetClientParty(pokeCon, clientId);
    client->mainModule = mainModule;
    client->pokeCon = pokeCon;
    client->numCoverPos = numCoverPos;
    client->procActionIdx = 0;
    client->unk129 = clientId;
    client->unkCC = 0;
    client->viewCore = NULL;
    client->savedHp = 0;
    client->cmdQueue = GFL_HeapAllocate(heapId, sizeof(BtlServerCmdQueue), TRUE, "btl_client.c", 0x292);
    client->mainProc = BattleClient_MainProcNormal;
    client->mainSeq = 0;
    client->cmdCheckServer = NULL;
    client->cmdCheckEnable = FALSE;
    client->cmdCheckReq = 0;
    client->waitMsgShown = 0;
    client->shooterEnergy = 0;
    client->cmdLimitTime = 0;
    client->gameLimitTime = 0;
    client->cmdLimitOver = 0;
    client->unk1BA_0 = 0;
    client->forceQuit = 0;
    client->unk1BA_4 = 0;
    client->unk1BA_5 = 0;
    client->field = func_ov167_0219d9a8(mainModule);
    client->turnCount = 0;
    client->bagMode = arg7;
    func_ov167_021bda58(&client->clientIdList);
    RecPlayer_Init(&client->recPlayer);
    BattleClient_LoadAIItems(client);
    client->aiRand = *rand;
    if (client->clientType == 1 && recPlay == 0) {
        u32 aiFlags = func_ov167_0219d8d4(client->mainModule, client->clientId);

        client->aiVM = TrAI_CreateVM(client->mainModule, func_ov167_0219e158(client->mainModule), client->pokeCon,
                                     aiFlags, client->heapId);
    } else {
        client->aiVM = NULL;
    }
    for (i = 0; i < 4; i++) {
        client->hintShown[i] = 0;
    }
    type = func_ov167_0219c988(client->mainModule);
    if (client->clientType == 0 && func_ov167_0219c964(client->mainModule)) {
        client->recorder = func_ov167_021d45b0(heapId, type);
    } else {
        client->recorder = NULL;
    }
    if (type == 2) {
        BtlSetup *setup = func_ov167_0219e30c(client->mainModule);
        client->studio.unk58 = setup->unk128;
        client->studio.audienceMask = setup->unk12C;
    }
    return client;
}

void BattleClient_Delete(BtlClient *client) {
    if (client->recorder != NULL) {
        func_ov167_021d45e8(client->recorder);
    }
    if (client->aiVM != NULL) {
        TrAI_DeleteVM(client->aiVM);
    }
    GFL_HeapFree(client->cmdQueue);
    func_ov167_021d4acc(client->adapter);
    GFL_HeapFree(client);
}

void func_ov167_021b18c4(BtlClient *client) {
    client->forceQuit = TRUE;
}

void *func_ov167_021b18d4(BtlClient *client, u32 *size) {
    if (client->recorder != NULL) {
        return func_ov167_021d4628(client->recorder, size);
    }
    return NULL;
}

void func_ov167_021b18e8(BtlClient *client, void *data) {
    client->clientType = 2;
    client->recReader = data;
    RecPlayer_SetMaxChapter(&client->recPlayer, func_ov167_021d481c(data));
}

void func_ov167_021b190c(BtlClient *client, BtlvCore *viewCore) {
    client->viewCore = viewCore;
}

void func_ov167_021b1910(BtlClient *client, BtlServer *server) {
    client->cmdCheckServer = server;
    client->cmdCheckEnable = TRUE;
}

BtlAdapter *func_ov167_021b1928(BtlClient *client) {
    return client->adapter;
}

BOOL func_ov167_021b192c(BtlClient *client) {
    return client->mainProc(client);
}

void func_ov167_021b1934(BtlClient *client, u32 arg1) {
    func_ov167_021d4660(client->recReader);
    func_ov167_021d4bc8(client->adapter);
    RecPlayer_StartSkip(&client->recPlayer, arg1);
    BattleClient_SetMainProc(client, BattleClient_MainProcRecPlay);
}

void func_ov167_021b1960(BtlClient *client) {
    func_ov167_021b939c(&client->recPlayer);
    BattleClient_SetMainProc(client, BattleClient_MainProcNormal);
}

BOOL func_ov167_021b1978(BtlClient *client) {
    if (client->clientType == 2) {
        return TRUE;
    }
    return FALSE;
}

u16 func_ov167_021b198c(BtlClient *client) {
    return client->recPlayer.maxChapter;
}

BOOL func_ov167_021b1990(BtlClient *client) {
    if (client->mainProc == BattleClient_MainProcRecPlay) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov167_021b19a4(BtlClient *client) {
    return client->actions[0].bits.move;
}

void func_ov167_021b19b0(BtlClient *client, u8 value) {
    client->shooterEnergy += value;
    if (client->shooterEnergy > 14) {
        client->shooterEnergy = 14;
    }
}

static void BattleClient_SetMainProc(BtlClient *client, BOOL (*mainProc)(BtlClient *client)) {
    client->mainProc = mainProc;
    client->mainSeq = 0;
}

static BOOL BattleClient_MainProcNormal(BtlClient *client) {
    RecPlayer_Update(client, &client->recPlayer);
    switch (client->mainSeq) {
    case 0: {
        u32 cmd = func_ov167_021d4bd4(client->adapter);

        if (client->clientType == 2 && (client->unk1BA_4 || client->unk1BA_5) && cmd == 0) {
            return FALSE;
        }
        BattleClient_SetDummyReturnData(client);
        if (cmd == 9) {
            const BtlClientIDList *list;

            func_ov167_021d4c0c(client->adapter, (const void **)&list);
            client->clientIdList = *list;
            client->cmdSeq = 0;
            client->mainSeq = 3;
        } else if (cmd != 0) {
            u32 unk01;

            client->cmdProc = BattleClient_GetCmdProc(client, cmd, &unk01);
            if (unk01) {
                if (!func_ov167_021b93c0(&client->recPlayer)) {
                    client->mainSeq = 6;
                    break;
                }
            } else {
                func_ov167_021b93d8(&client->recPlayer);
            }
            if (client->cmdProc != NULL) {
                client->mainSeq = 1;
                client->cmdSeq = 0;
            } else {
                client->cmdSeq = 0;
                client->mainSeq = 2;
            }
        } else if (client->forceQuit) {
            return TRUE;
        }
        break;
    }
    case 1:
        if (client->cmdProc(client, &client->cmdSeq)) {
            if (client->forceQuit) {
                return TRUE;
            }
            if (func_ov167_021b9360(&client->recPlayer)) {
                client->mainSeq = 4;
            } else {
                client->mainSeq = 2;
            }
        }
        break;
    case 2:
        if (func_ov167_021d4c38(client->adapter, client->returnData, client->returnDataSize)) {
            client->mainSeq = 0;
        }
        if (client->forceQuit) {
            return TRUE;
        }
        break;
    case 3:
        if (func_ov167_021d4c38(client->adapter, client->returnData, client->returnDataSize)) {
            client->mainSeq = 7;
        }
        if (client->forceQuit) {
            return TRUE;
        }
        break;
    case 6:
        if (func_ov167_021b9360(&client->recPlayer)) {
            client->mainSeq = 4;
        }
        break;
    case 4:
        if (func_ov167_021b9374(&client->recPlayer) == 1) {
            client->mainSeq = 7;
            break;
        }
        if (client->viewCore != NULL) {
            GFL_SndPlayerSetMuteStateEx(1, 0x3e);
            PokeVoice_ResetMasterVolume();
        }
        client->mainSeq = 5;
        break;
    case 5:
        if (!GFL_SndBGMIsFading()) {
            u32 chapter = func_ov167_021b93bc(&client->recPlayer);

            GFL_SndPlayerSetMuteStateEx(0, 1);
            func_ov167_0219e074(client->mainModule, chapter);
            PokeVoice_SetMasterVolume(0);
        }
        break;
    case 7:
        return TRUE;
    }
    Studio_UpdateAudience(client);
    return FALSE;
}

static BOOL BattleClient_MainProcRecPlay(BtlClient *client) {
    RecPlayer_Update(client, &client->recPlayer);
    switch (client->mainSeq) {
    case 0:
        client->mainSeq = 1;
    case 1:
        if (!func_ov167_021b93ac(&client->recPlayer)) {
            u32 cmd = func_ov167_021d4bd4(client->adapter);

            if (cmd != 0) {
                client->cmdProc = BattleClient_GetCmdProc(client, cmd, NULL);
                if (client->cmdProc != NULL) {
                    client->mainSeq = 2;
                    client->cmdSeq = 0;
                } else {
                    BattleClient_SetDummyReturnData(client);
                    client->mainSeq = 3;
                    client->cmdSeq = 0;
                }
            }
        } else {
            if (client->viewCore != NULL) {
                GFL_SndPlayerSetMuteStateEx(1, 1);
                func_ov167_021d09f8(client->viewCore);
                GFL_SndBGMFadeIn(30);
                BtlvEffect_SetFlag26(0);
                PokeVoice_ResetMasterVolume();
            }
            client->mainSeq = 4;
        }
        break;
    case 2:
        if (client->cmdProc(client, &client->cmdSeq)) {
            client->mainSeq = 3;
            client->cmdSeq = 0;
            if (BtlvEffect_GetWork()) {
                BtlvEffect_SetFlag26(1);
            }
        }
        break;
    case 3:
        if (func_ov167_021d4c38(client->adapter, client->returnData, client->returnDataSize)) {
            client->mainSeq = 1;
        }
        break;
    case 4:
        if (client->viewCore != NULL && func_ov167_021d0a08(client->viewCore) && !GFL_SndBGMIsFading()) {
            u16 chapter = func_ov167_021b93bc(&client->recPlayer);

            func_ov167_021d0a48(client->viewCore, chapter, chapter);
            func_ov167_0219e130(client->mainModule);
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

static void BattleClient_SetDummyReturnData(BtlClient *client) {
    client->dummyReturnData = 0;
    client->returnData = &client->dummyReturnData;
    client->returnDataSize = sizeof(client->dummyReturnData);
}

static BtlClientCmdProc BattleClient_GetCmdProc(BtlClient *client, u32 cmd, u32 *unk01) {
    u32 i;

    for (i = 0; i < NELEMS(sCmdProcTable); i++) {
        if (cmd == sCmdProcTable[i].cmd) {
            if (unk01 != NULL) {
                *unk01 = sCmdProcTable[i].unk01;
            }
            return sCmdProcTable[i].procs[client->clientType];
        }
    }
    return NULL;
}

void func_ov167_021b1d58(BtlClient *client, BtlClientIDList *list) {
    *list = client->clientIdList;
}

BOOL func_ov167_021b1d64(BtlClient *client) {
    if (client->clientType != 2) {
        if (client->gameLimitTime != 0) {
            return BtlvEffect_IsTimeUp(0);
        }
        return FALSE;
    }
    return client->unk1BA_4;
}

BOOL func_ov167_021b1d90(BtlClient *client) {
    return client->unk1BA_5;
}

static void func_ov167_021b1da0(BtlClientStudioWork *studio) {
    int i;

    for (i = 0; i < 22; i++) {
        if ((studio->audienceMask >> i) & 1) {
            studio->audienceTimers[i] = GFL_RandomLC(300);
        }
    }
}

static BOOL func_ov167_021b1dcc(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        if (BtlSetup_GetBattleType(client->mainModule) != 4) {
            func_ov167_021ce8dc(client->viewCore, FALSE);
        } else {
            func_ov167_021ce8dc(client->viewCore, TRUE);
        }
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021ce90c(client->viewCore)) {
            client->cmdLimitTime = func_ov167_0219defc(client->mainModule);
            client->gameLimitTime = func_ov167_0219df08(client->mainModule);
            if (client->cmdLimitTime != 0 || client->gameLimitTime != 0) {
                BtlvEffect_CreateTimer(client->gameLimitTime, client->cmdLimitTime);
                if (client->gameLimitTime != 0) {
                    BtlvEffect_SetTimerVisible(0, 1, 1);
                }
            }
            func_ov167_021b1ee4(client);
            if (func_ov167_021b1990(client)) {
                func_ov167_021d0a7c(client->viewCore, func_ov167_021b93bc(&client->recPlayer));
            }
            client->studio.audienceMask = func_ov167_021d0be4(client->viewCore);
            func_ov167_021b1da0(&client->studio);
            func_ov167_021b96f0(client, &client->studio, client->party);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b1ea4(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        BOOL done = func_ov167_021b1dcc(client, seq);

        if (done && func_ov167_021d4674(client->recReader, client->clientId)) {
            func_ov167_021b9368(&client->recPlayer);
        }
        return done;
    }
    func_ov167_021d4674(client->recReader, client->clientId);
    return TRUE;
}

static void func_ov167_021b1ee4(BtlClient *client) {
    BattleMon *mon = func_ov167_021b1f34(client);

    if (mon != NULL) {
        client->savedHp = GetBattleMonStat(mon, 13);
    }
}

static u32 func_ov167_021b1f00(BtlClient *client) {
    BattleMon *mon = func_ov167_021b1f34(client);

    if (mon != NULL) {
        u32 hp = GetBattleMonStat(mon, 13);

        if (hp >= client->savedHp) {
            return 0;
        }
        return (client->savedHp - hp) * 100 / hp;
    }
    return 0;
}

static BattleMon *func_ov167_021b1f34(BtlClient *client) {
    if (BtlSetup_GetBattleStyle(client->mainModule) == 0 && !func_ov167_0219bee4(client->mainModule)) {
        u8 pos = func_ov167_0219c8d0(client->mainModule, client->clientId, 0);

        return GetClientMonData(client->pokeCon, pos, 0);
    }
    return NULL;
}

static BOOL func_ov167_021b1f70(BtlClient *client, s32 *seq) {
    u8 select;

    switch (*seq) {
    case 0:
        func_ov167_021d05d4(client->viewCore, client->unkCC);
        func_ov167_021b2080(client);
        (*seq)++;
        break;
    case 1:
        if (CheckIfOverCmdLimit(client)) {
            func_ov167_021d0630(client->viewCore);
            *seq = 2;
        } else if (func_ov167_021d0608(client->viewCore, &select)) {
            client->unkCC = select;
            *seq = 3;
        }
        break;
    case 2:
        client->unkCC = BattleClient_PickRotation(client->unkCC);
        (*seq)++;
        break;
    case 3:
        func_ov167_021b20f8(client);
        (*seq)++;
        break;
    case 4:
        client->returnData = &client->unkCC;
        client->returnDataSize = 1;
        return TRUE;
    }
    return FALSE;
}

static u8 BattleClient_PickRotation(u8 dir) {
    u32 rand = GFL_RandomMTRange(100);

    if (dir == 0) {
        if (rand < 30) {
            dir = 3;
        } else if (rand < 60) {
            dir = 2;
        } else {
            dir = 1;
        }
    } else {
        switch (dir) {
        case 1:
        default:
            dir = rand < 50 ? 2 : 3;
            break;
        case 3:
            dir = rand < 40 ? 2 : 1;
            break;
        case 2:
            dir = rand < 40 ? 3 : 1;
            break;
        }
    }
    return dir;
}

static BOOL func_ov167_021b2058(BtlClient *client, s32 *seq) {
    return TRUE;
}

static BOOL func_ov167_021b205c(BtlClient *client, s32 *seq) {
    client->unkCC = BattleClient_PickRotation(client->unkCC);
    client->returnData = &client->unkCC;
    client->returnDataSize = 1;
    return TRUE;
}

static void func_ov167_021b2080(BtlClient *client) {
    if (client->cmdLimitTime != 0) {
        BtlvEffect_SetTimerVisible(1, 1, 1);
        client->cmdLimitOver = FALSE;
    }
}

static BOOL CheckIfOverCmdLimit(BtlClient *client) {
    if (client->cmdLimitTime != 0) {
        if (!client->cmdLimitOver && BtlvEffect_IsTimeUp(1)) {
            client->cmdLimitOver = TRUE;
        }
        return client->cmdLimitOver;
    }
    return FALSE;
}

static BOOL CheckActionSelectForceQuit(BtlClient *client, BOOL (*proc)(BtlClient *client, s32 *seq)) {
    if (CheckIfOverCmdLimit(client)) {
        if (proc != NULL) {
            BattleClient_SetSelActProc(client, proc);
        }
        return TRUE;
    }
    return FALSE;
}

static void func_ov167_021b20f8(BtlClient *client) {
    if (client->cmdLimitTime != 0) {
        BtlvEffect_SetTimerVisible(1, 0, 0);
    }
}

static void func_ov167_021b2110(BtlClient *client, BattleMon *mon, BtlvStringParam *param) {
    if (BtlSetup_GetBattleStyle(client->mainModule) != 3) {
        Btlv_StringParam_Setup(param, 1, 0x45);
        Btlv_StringParam_AddArg(param, GetMonID(mon));
        func_ov167_021d0b4c(param, 0);
    } else {
        Btlv_StringParam_Setup(param, 1, 0x46);
        Btlv_StringParam_AddArg(param, client->clientId);
        func_ov167_021d0b4c(param, 0);
    }
}

static void BattleClient_SetSelActProc(BtlClient *client, BOOL (*proc)(BtlClient *client, s32 *seq)) {
    client->selActProc = proc;
    client->selActSeq = 0;
}

static BOOL BattleClient_RunSelActProc(BtlClient *client) {
    return client->selActProc(client, &client->selActSeq);
}

// The step the action selection is at
static s32 sSelectSeq;

static s32 func_ov167_021b2188(void) {
    return sSelectSeq;
}

static u32 func_ov167_021b2194(BtlClient *client) {
    BtlSetup *setup = func_ov167_0219e310(client->mainModule);
    const BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);
    u8 group = data_ov167_021d6eb4[setup->unk129];
    u32 msgId;

    switch (client->studio.score.unk10) {
    case 1:
    case 2:
    case 3:
        msgId = client->studio.score.unk10 + 5 + group * 15;
        client->studio.score.unk10 = 0;
        break;
    default:
        if (client->turnCount > rules->unk0C) {
            msgId = group * 15 + 9;
        } else if (client->turnCount == 0) {
            msgId = group * 15 + (GFL_RandomLC(2) + 13);
        } else {
            msgId = group * 15 + GFL_RandomLC(6);
        }
        break;
    }
    return msgId;
}

static void func_ov167_021b2210(BtlClient *client, u32 value) {
    client->studio.score.unk10 = value;
}

static BOOL func_ov167_021b2218(BtlClient *client, s32 *seq) {
    sSelectSeq = *seq;
    switch (*seq) {
    case 0:
        *seq = 8;
    case 8:
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectInit);
        func_ov167_021b2080(client);
        (*seq)++;
        break;
    case 9:
        CheckIfOverCmdLimit(client);
        if (BattleClient_RunSelActProc(client)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b226c(BtlClient *client, s32 *seq) {
    sSelectSeq = *seq;
    switch (*seq) {
    case 0:
        client->studio.seq = 0;
        if (client->clientId != 0) {
            *seq = 8;
            return FALSE;
        }
        if (func_ov167_021b261c(func_ov167_0219e39c(client->mainModule), &client->studio, client->turnCount)) {
            (*seq)++;
        } else {
            *seq = 3;
            func_ov167_021b2790(&client->studio);
        }
        break;
    case 1:
        if (func_ov167_021b2864(client)) {
            client->studio.seq = 0;
            *seq = 3;
        }
        break;
    case 2:
        break;
    case 3:
        func_ov167_021d0b70(client->viewCore, client->turnCount);
        *seq = 4;
        break;
    case 4:
        if (!func_ov167_021d0b90(client->viewCore)) {
            *seq = 5;
        }
        break;
    case 5:
        Btlv_StringParam_Setup(&client->strParam, 9, func_ov167_021b2194(client));
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        *seq = 6;
        break;
    case 6:
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 7;
            client->studio.waitTimer = 30;
        }
        break;
    case 7:
        if (--client->studio.waitTimer == 0) {
            *seq = 8;
        }
        break;
    case 8: {
        const BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);

        func_ov167_021d0b50(client->viewCore, rules->turnLimit, client->turnCount);
        client->turnCount++;
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectInit);
        func_ov167_021b2080(client);
        (*seq)++;
        break;
    }
    case 9:
        CheckIfOverCmdLimit(client);
        if (BattleClient_RunSelActProc(client)) {
            client->studio.unk24 = func_ov167_021b19a4(client);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b23b8(BtlClient *client, s32 *seq) {
    if (func_ov167_0219c988(client->mainModule) == 1) {
        return func_ov167_021b226c(client, seq);
    }
    return func_ov167_021b2218(client, seq);
}

static void Studio_SetSeq(BtlClientStudioWork *studio, u8 seq, u8 nextSeq) {
    studio->seq = seq;
    studio->nextSeq = nextSeq;
}

static BOOL func_ov167_021b23e4(BtlClient *client) {
    const BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);
    s16 msgId;

    switch (client->studio.seq) {
    case 0:
        if (rules->unkA8[client->turnCount - 1] == -1 && rules->unkD0[client->turnCount - 1] == -1) {
            client->studio.seq = 9;
        } else {
            func_ov167_021d5944();
            client->studio.seq++;
        }
        break;
    case 1:
        BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 9, 0,
                            0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        Studio_SetSeq(&client->studio, 10, 2);
        break;
    case 2:
        BtlvEffect_Start(0x294);
        Studio_SetSeq(&client->studio, 10, 3);
        break;
    case 3:
        msgId = rules->unkA8[client->turnCount - 1];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            client->studio.seq++;
        } else {
            client->studio.seq = 5;
        }
        break;
    case 4:
        if (func_ov167_021d02e8(client->viewCore)) {
            client->studio.seq++;
        }
        break;
    case 5:
        msgId = rules->unkD0[client->turnCount - 1];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            client->studio.seq++;
        } else {
            client->studio.seq = 7;
        }
        break;
    case 6:
        if (func_ov167_021d02e8(client->viewCore)) {
            client->studio.seq++;
        }
        break;
    case 7:
        BtlvEffect_Start(0x299);
        Studio_SetSeq(&client->studio, 10, 8);
        break;
    case 8:
        BtlvEffect_Start(0x295);
        Studio_SetSeq(&client->studio, 10, 9);
        break;
    case 9:
        return TRUE;
    case 10:
        if (!BtlvEffect_IsBusy()) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    }
    return FALSE;
}

static s32 func_ov167_021b2538(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn, s32 arg3) {
    s32 found = -1;
    s32 i;

    for (i = 0; i < 12; i++) {
        s16 unk02 = rules->scenes[i].unk02;

        if (unk02 == -1 && rules->scenes[i].unk04 == -1) {
            if (turn == rules->scenes[i].unk00) {
                found = i;
            }
        } else if (turn == rules->scenes[i].unk00 && studio->unk10 == unk02) {
            found = i;
        }
    }
    if (turn == 101) {
        for (i = 0; i < 12; i++) {
            if (arg3 == rules->scenes[i].unk04) {
                found = i;
            }
        }
    }
    if (found < 0) {
        for (i = 0; i < 12; i++) {
            if (rules->scenes[i].unk04 == 0 && studio->unk26 == rules->scenes[i].unk06 &&
                !((studio->usedEventMask >> i) & 1)) {
                found = i;
            }
        }
    }
    if (found < 0) {
        for (i = 0; i < 12; i++) {
            if (rules->scenes[i].unk04 == 1 && studio->unk24 == rules->scenes[i].unk06 &&
                !((studio->usedEventMask >> i) & 1)) {
                found = i;
            }
        }
    }
    return found;
}

static BOOL func_ov167_021b261c(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn) {
    BOOL result = FALSE;
    s32 scene = func_ov167_021b2538(rules, studio, turn, 0);

    if (scene < 0) {
        return result;
    }
    if (rules->scenes[scene].choices[0].msgId != -1) {
        result = TRUE;
    }
    return result;
}

static void func_ov167_021b264c(const BtlScriptedRules *rules, BtlClientStudioWork *studio) {
    s16 value = rules->scenes[studio->scene].choices[studio->unk08].unk06;

    if (value != -1) {
        studio->unk0C = value;
    }
}

static BOOL func_ov167_021b2674(const BtlScriptedRules *rules, BtlClientStudioWork *studio) {
    s16 value = rules->scenes[studio->scene].events[studio->unk04];

    if (value != -1 && value != studio->unk2C) {
        return FALSE;
    }
    return TRUE;
}

static u32 func_ov167_021b26a8(BtlClient *client, BtlClientStudioWork *studio) {
    BtlSetup *setup = func_ov167_0219e30c(client->mainModule);

    return setup->unk110[client->studio.scene];
}

static void func_ov167_021b26c4(BtlClient *client, s32 scene, u32 value) {
    BtlSetup *setup = func_ov167_0219e30c(client->mainModule);

    setup->unk110[scene] = value;
}

static u32 func_ov167_021b26dc(BtlClient *client) {
    BtlSetup *setup = func_ov167_0219e30c(client->mainModule);

    return setup->unk124;
}

static void func_ov167_021b26ec(BtlClient *client, u32 value) {
    BtlSetup *setup = func_ov167_0219e30c(client->mainModule);

    setup->unk124 = value;
}

static void func_ov167_021b2700(BtlClient *client, const BtlScriptedRules *rules, s32 scene) {
    if (func_ov167_0219c988(client->mainModule) == 1) {
        if (rules->unk0E == scene) {
            func_ov167_021b26ec(client, 1);
        } else if (rules->scenes[scene].unk0A >= 60) {
            func_ov167_021b26ec(client, 2);
        } else {
            func_ov167_021b26ec(client, 0);
        }
    }
}

static void func_ov167_021b2748(BtlClient *client, const BtlScriptedRules *rules, s32 scene) {
    int type = rules->scenes[scene].unk00;

    if (type == 100 || type == 101) {
        func_ov167_021b9750(client, &client->studio.score, scene);
        if (func_ov167_0219c988(client->mainModule) == 2) {
            BtlSetup *setup;

            func_ov167_021b96b0(client->viewCore, &client->studio);
            setup = func_ov167_0219e30c(client->mainModule);
            setup->unk12C = client->studio.audienceMask;
        }
    }
}

static void func_ov167_021b2790(BtlClientStudioWork *studio) {
    studio->unk24 = 0;
    studio->unk26 = 0;
}

static void func_ov167_021b2798(BtlClient *client, const BtlScriptedRules *rules, BtlClientStudioWork *studio) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 4; i++) {
        if (rules->scenes[studio->scene].choices[i].msgId != -1) {
            count++;
        }
    }
    func_ov167_021cfb28(client->viewCore, count, &studio->unk08);
    for (i = 0; i < count; i++) {
        studio->strBufs[i] = GFL_StrBufCreate(0x28, client->heapId);
        func_ov167_021d5904(studio->strBufs[i], rules->scenes[studio->scene].choices[i].msgId);
        func_ov167_021cfb34(client->viewCore, i, studio->strBufs[i]);
    }
    BattleClient_SetSelActProc(client, func_ov167_021b41d4);
}

static void Studio_FreeChoiceStrBufs(BtlClientStudioWork *studio) {
    int i;

    for (i = 0; i < 4; i++) {
        if (studio->strBufs[i] != NULL) {
            GFL_StrBufFree(studio->strBufs[i]);
            studio->strBufs[i] = NULL;
        }
    }
}

static BOOL func_ov167_021b2864(BtlClient *client) {
    const BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);
    BtlMainUnk478 *result = func_ov167_0219e3ac(client->mainModule);
    PokewoodCutin *cutin = func_ov167_0219e3bc(client->mainModule);
    s16 msgId;

    switch (client->studio.seq) {
    case 0:
        client->studio.scene =
            func_ov167_021b2538(rules, &client->studio, client->turnCount, func_ov167_0219c9e0(client->mainModule));
        func_ov167_021b2748(client, rules, client->studio.scene);
        func_ov167_021b2700(client, rules, client->studio.scene);
        func_ov167_021d5944();
        if (client->studio.scene >= 0) {
            if (func_ov167_0219c988(client->mainModule) == 2) {
                u32 value = func_ov167_021b26dc(client);

                if (func_ov167_021d5fe8(rules, client->studio.scene, value, ReturnZero(client->mainModule, 11))) {
                    client->studio.seq = 25;
                    client->studio.unk0C = -1;
                    break;
                }
            }
            client->studio.unk04 = 0;
            client->studio.unk05 = 0;
            client->studio.unk0C = -1;
            client->studio.seq++;
        } else {
            client->studio.seq = 22;
        }
        break;
    case 1:
        if (!func_ov167_021b2674(rules, &client->studio)) {
            client->studio.seq = 2;
        } else {
            if ((rules->scenes[client->studio.scene].messages[0][client->studio.unk04] == -1 &&
                 rules->scenes[client->studio.scene].messages[1][client->studio.unk04] == -1) ||
                client->studio.unk04 == 10) {
                client->studio.seq = 14;
            } else {
                client->studio.seq = 6;
            }
        }
        break;
    case 2:
        if (func_ov167_021b2dd8(rules, client)) {
            client->studio.seq = 1;
        }
        break;
    case 6:
        msgId = rules->scenes[client->studio.scene].messages[0][client->studio.unk04];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, 24, 8);
        } else {
            client->studio.seq = 8;
        }
        break;
    case 8:
        msgId = rules->scenes[client->studio.scene].messages[1][client->studio.unk04];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, 24, 1);
        } else {
            client->studio.seq = 1;
        }
        client->studio.unk04++;
        break;
    case 10:
        BtlvEffect_Start(0x299);
        Studio_SetSeq(&client->studio, 23, 12);
        break;
    case 12:
        BtlvEffect_Start(0x295);
        Studio_SetSeq(&client->studio, 23, 22);
        client->studio.unk2C = 0;
        break;
    case 14:
        if (rules->scenes[client->studio.scene].choices[0].msgId != -1) {
            if (func_ov167_0219c988(client->mainModule) == 1) {
                client->studio.seq = 15;
                func_ov167_021b2798(client, rules, &client->studio);
            } else {
                client->studio.seq = 16;
            }
        } else {
            client->studio.seq = 21;
        }
        break;
    case 15:
        if (BattleClient_RunSelActProc(client)) {
            Studio_FreeChoiceStrBufs(&client->studio);
            func_ov167_021b26c4(client, client->studio.scene, client->studio.unk08);
            func_ov167_021b2cf0(rules, &client->studio, client->studio.scene, client->studio.unk08);
            result->unk08 = rules->scenes[client->studio.scene].choices[client->studio.unk08].unk04;
            func_ov167_021b97a4(client, &client->studio.score, client->studio.scene, client->studio.unk08);
            func_ov167_021b264c(rules, &client->studio);
            client->studio.seq = 17;
        }
        break;
    case 16:
        client->studio.unk08 = func_ov167_021b26a8(client, &client->studio);
        func_ov167_021b2cf0(rules, &client->studio, client->studio.scene, client->studio.unk08);
        result->unk08 = rules->scenes[client->studio.scene].choices[client->studio.unk08].unk04;
        func_ov167_021b97a4(client, &client->studio.score, client->studio.scene, client->studio.unk08);
        func_ov167_021b264c(rules, &client->studio);
        client->studio.seq = 17;
        break;
    case 17:
        Btlv_StringParam_Setup(&client->strParam, 8,
                               rules->scenes[client->studio.scene].choices[client->studio.unk08].msgId);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        Studio_SetSeq(&client->studio, 24, 19);
        break;
    case 19:
        Btlv_StringParam_Setup(&client->strParam, 8,
                               rules->scenes[client->studio.scene].choices[client->studio.unk08].msgId2);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        Studio_SetSeq(&client->studio, 24, 21);
        break;
    case 21:
        if (client->turnCount >= 100) {
            client->studio.seq = 22;
            client->studio.unk2C = 0;
        } else {
            client->studio.seq = 10;
        }
        break;
    case 22:
        if (client->studio.scene >= 0) {
            client->studio.usedEventMask |= 1 << client->studio.scene;
        }
        func_ov167_021b2790(&client->studio);
        return TRUE;
    case 23:
        if (!BtlvEffect_IsBusy()) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case 24:
        if (func_ov167_021d02e8(client->viewCore)) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case 25:
        BtlvEffect_Start(0x29e);
        Studio_SetSeq(&client->studio, 23, 26);
        break;
    case 26:
        BtlvEffect_Start(0x29f);
        Studio_SetSeq(&client->studio, 23, 27);
        break;
    case 27: {
        PlayerInfo *info = func_ov167_0219bf68(client->mainModule);

        func_ov167_021d5fe4(cutin, client->viewCore);
        func_ov167_021d5fc4(cutin, rules, client->studio.scene, func_ov167_021b26a8(client, &client->studio),
                            getTrainerGender(info));
        client->studio.unk08 = func_ov167_021b26a8(client, &client->studio);
        func_ov167_021b2cf0(rules, &client->studio, client->studio.scene, client->studio.unk08);
        func_ov167_021b264c(rules, &client->studio);
        client->studio.seq++;
        break;
    }
    case 28:
        func_ov167_021d5e90(cutin);
        if (func_ov167_021d5fc0(cutin)) {
            func_ov167_021b97a4(client, &client->studio.score, client->studio.scene, client->studio.unk08);
            client->studio.seq = 29;
        }
        break;
    case 29:
        BtlvEffect_Start(0x2a0);
        Studio_SetSeq(&client->studio, 23, 30);
        break;
    case 30:
        BtlvEffect_Start(0x2a1);
        Studio_SetSeq(&client->studio, 23, 22);
        break;
    }
    return FALSE;
}

static void func_ov167_021b2cf0(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 scene, u32 value) {
    const BtlStudioScene *s = &rules->scenes[scene];
    if (s->unk00 != -1 || s->unk02 != -1 || (u16)s->unk04 > 1) {
        studio->unk10 = value + 1;
    }
}

static s32 Studio_GetNextState(BtlClientStudioWork *studio, s32 event) {
    s32 i;
    s32 next = 0;
    s32 index = next - 1;

    for (i = 0; i < 7; i++) {
        if (studio->unk2C == sStudioTransitions[i].state) {
            index = i;
        }
    }
    if (index >= 0) {
        switch (event) {
        case 0x11:
            next = sStudioTransitions[index].next[0];
            break;
        case 0x00:
            next = sStudioTransitions[index].next[1];
            break;
        case 0x22:
            next = sStudioTransitions[index].next[2];
            break;
        case 0x12:
            next = sStudioTransitions[index].next[3];
            break;
        case 0x21:
            next = sStudioTransitions[index].next[4];
            break;
        case 0x10:
            next = sStudioTransitions[index].next[3];
            break;
        case 0x01:
            next = sStudioTransitions[index].next[4];
            break;
        case 0x02:
            next = sStudioTransitions[index].next[2];
            break;
        case 0x20:
            next = sStudioTransitions[index].next[2];
            break;
        }
    }
    return next;
}

static void func_ov167_021b2dd0(BtlClientStudioWork *studio, u8 nextSeq) {
    studio->unk02 = 9;
    studio->unk03 = nextSeq;
}

static BOOL func_ov167_021b2dd8(const BtlScriptedRules *rules, BtlClient *client) {
    switch (client->studio.unk02) {
    case 0:
        if (func_ov167_021b2674(rules, &client->studio)) {
            client->studio.unk02 = 10;
        } else {
            client->studio.unk02 =
                Studio_GetNextState(&client->studio, rules->scenes[client->studio.scene].events[client->studio.unk04]);
        }
        break;
    case 1:
        BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 9, 0,
                            0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf) | 0x10;
        break;
    case 2:
        BtlvEffect_Start(0x294);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf0) | 0x01;
        break;
    case 3:
        BtlvEffect_Start(0x299);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = client->studio.unk2C & 0xf;
        break;
    case 4:
        BtlvEffect_Start(0x295);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = client->studio.unk2C & 0xf0;
        break;
    case 5:
        BtlvEffect_Start(0x29a);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf) | 0x20;
        break;
    case 6:
        BtlvEffect_Start(0x29b);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf0) | 0x02;
        break;
    case 7:
        BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 9, 0,
                            0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        BtlvEffect_Start(0x29c);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf) | 0x10;
        break;
    case 8:
        BtlvEffect_Start(0x29d);
        func_ov167_021b2dd0(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf0) | 0x01;
        break;
    case 9:
        if (!BtlvEffect_IsBusy()) {
            client->studio.unk02 = client->studio.unk03;
        }
        break;
    case 10:
        client->studio.unk02 = 0;
        client->studio.unk03 = 0;
        return TRUE;
    }
    return FALSE;
}

static s16 StudioRules_GetTurnEvent(const BtlScriptedRules *rules, u8 turn) {
    int i;

    for (i = 0; i < 3; i++) {
        if (turn == rules->turnEvents[i].turn) {
            return rules->turnEvents[i].value;
        }
    }
    return -1;
}

static BOOL func_ov167_021b2fb8(BtlClient *client) {
    const BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);

    switch (client->studio.seq) {
    case 0:
        if (func_ov167_0219c988(client->mainModule) != 2) {
            return TRUE;
        }
        if (client->studio.unk0C < 0) {
            s16 value = StudioRules_GetTurnEvent(rules, client->turnCount);
            if (value >= 0) {
                client->studio.unk0C = value;
            }
        }
        if (client->studio.unk0C > 0) {
            BtlvEffect_StartPaletteFade(1, 0, 0x10, 0, 0);
            client->studio.seq++;
        } else {
            return TRUE;
        }
        break;
    case 1:
        if (!BtlvEffect_IsPaletteFading(1)) {
            BtlvEffect_SetVanish(1, 1);
            client->studio.seq++;
        }
        break;
    case 2:
        BtlvEffect_ReloadField(client->studio.unk0C);
        BtlvEffect_StartPaletteFade(1, 0x10, 0, 0, 0);
        client->studio.seq++;
        break;
    case 3:
        BtlvEffect_SetVanish(1, 0);
        if (!BtlvEffect_IsPaletteFading(1)) {
            client->studio.seq++;
        }
        break;
    case 4:
        client->studio.unk0C = -1;
        return TRUE;
    }
    return FALSE;
}

static u32 Studio_GetResultMsgId(BtlClient *client, const BtlScriptedRules *rules, BtlSetup *setup) {
    u32 msgId;
    s32 scene = func_ov167_021b2538(rules, &client->studio, client->turnCount, func_ov167_0219c9e0(client->mainModule));
    s16 value = rules->scenes[scene].unk0A;
    u8 rank = data_ov167_021d6eb4[setup->unk110[0x19]];

    if (value > 60) {
        msgId = rank * 15 + 12;
    } else if (value >= 0) {
        msgId = rank * 15 + 10;
    } else if (value < 0) {
        msgId = rank * 15 + 11;
    }
    return msgId;
}

static BOOL func_ov167_021b3100(BtlClient *client) {
    BtlSetup *setup;
    const BtlScriptedRules *rules;

    switch (client->studio.seq) {
    case 0:
        setup = func_ov167_0219e310(client->mainModule);
        rules = func_ov167_0219e39c(client->mainModule);
        if (GetBattleResultForClient(client) == 1) {
            client->turnCount = 100;
        } else {
            client->turnCount = 101;
        }
        client->studio.msgId1 = Studio_GetResultMsgId(client, rules, setup);
        client->studio.seq = 1;
        break;
    case 1:
        BtlvEffect_Start(0x2a2);
        Studio_SetSeq(&client->studio, 8, 3);
        break;
    case 3:
        BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 9, 0,
                            0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        Studio_SetSeq(&client->studio, 8, 4);
        break;
    case 4:
        BtlvEffect_Start(0x294);
        Studio_SetSeq(&client->studio, 8, 5);
        break;
    case 5:
        if (client->studio.msgId1 >= 0) {
            Btlv_StringParam_Setup(&client->strParam, 9, client->studio.msgId1);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, 7, 9);
        } else {
            client->studio.seq = 6;
        }
        break;
    case 6:
        if (client->studio.msgId2 >= 0) {
            Btlv_StringParam_Setup(&client->strParam, 8, client->studio.msgId2);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, 7, 9);
        } else {
            client->studio.seq = 9;
        }
        break;
    case 7:
        if (func_ov167_021d02e8(client->viewCore)) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case 8:
        if (!BtlvEffect_IsBusy()) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case 9:
        return TRUE;
    }
    return FALSE;
}

static BOOL RecPlaySelectActionCore(BtlClient *client, s32 *seq, BOOL arg2) {
    switch (*seq) {
    case 0: {
        u8 count;
        u8 chapter;
        BattleAction *action;

        if (client->unk1BA_5) {
            SetNullReturnAction(client);
            return TRUE;
        }
        action = func_ov167_021d46a4(client->recReader, client->clientId, &count, &chapter);
        if (chapter) {
            func_ov167_021b9368(&client->recPlayer);
        }
        client->returnData = action;
        client->returnDataSize = count * 4;
        if (action->bits.action == 8) {
            func_ov167_021b93e4(&client->recPlayer);
            if (client->viewCore != NULL) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0xb7);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
                *seq = 1;
            } else {
                SetNullReturnAction(client);
                client->unk1BA_4 = TRUE;
                return TRUE;
            }
        } else if (action->bits.action == 9) {
            client->unk1BA_5 = TRUE;
            func_ov167_021b93e4(&client->recPlayer);
            if (client->viewCore != NULL) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0xb8);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
                *seq = 2;
            } else {
                SetNullReturnAction(client);
                return TRUE;
            }
        } else {
            if (action->bits.action == 1 && action->bits.target >= 6) {
                client->actions[0].bits.move = action->bits.move;
            }
            *seq = 3;
        }
        break;
    }
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            SetNullReturnAction(client);
            client->unk1BA_4 = TRUE;
            return TRUE;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            SetNullReturnAction(client);
            return TRUE;
        }
        break;
    case 3:
        if (func_ov167_0219c988(client->mainModule) == 0) {
            client->studio.seq = 0;
            return TRUE;
        }
        if (client->clientId != 0) {
            client->studio.seq = 0;
            return TRUE;
        }
        if (client->viewCore != NULL && arg2) {
            client->studio.seq = 0;
            *seq = 4;
        } else {
            return TRUE;
        }
        break;
    case 4:
        if (func_ov167_021b2864(client)) {
            client->studio.seq = 0;
            *seq = 5;
        }
        break;
    case 5:
        if (func_ov167_021b2fb8(client)) {
            client->studio.seq = 0;
            client->turnCount++;
            *seq = 6;
        }
        break;
    case 6:
        if (func_ov167_021b23e4(client)) {
            client->studio.unk24 = func_ov167_021b19a4(client);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL RecPlaySelectAction(BtlClient *client, s32 *seq) {
    return RecPlaySelectActionCore(client, seq, TRUE);
}

static void SetNullReturnAction(BtlClient *client) {
    client->procAction = &client->actions[0];
    BattleAction_SetNull(client->procAction);
    client->returnData = client->procAction;
    client->returnDataSize = 4;
}

static BOOL BattleClient_ActionSelectInit(BtlClient *client, s32 *seq) {
    u32 i;

    client->procActionIdx = 0;
    client->prevActionIdx = -1;
    client->firstActionIdx = 0;
    client->escapeSelected = FALSE;
    func_ov167_021b5d7c(client, 0, client->numCoverPos, &client->pokeListCmd, &client->pokeSelect);
    shooterWork_Init(client);
    selItemWork_Init(client);
    for (i = 0; i < 3; i++) {
        BattleAction_SetNull(&client->actions[i]);
    }
    for (i = 0; i < client->numCoverPos; i++) {
        if (!AutoSelectAction(client, func_ov167_0219d1e8(client->pokeCon, client->clientId, i), NULL)) {
            client->firstActionIdx = i;
            break;
        }
    }
    BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
    return FALSE;
}

static void func_ov167_021b350c(BtlClient *client, const BtlvStringParam *param) {
    func_ov167_021d01ec(client->viewCore, param);
    client->forceActionMsg = TRUE;
}

static BOOL BattleClient_ActionForceQuit(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        BattleClientCmd_ForceQuitInputNotify(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (BattleClientCmd_ForceQuitInputWait(client->viewCore)) {
            for (; client->procActionIdx < client->numCoverPos; client->procActionIdx++) {
                client->procMon = func_ov167_0219d1e8(client->pokeCon, client->clientId, client->procActionIdx);
                client->procAction = &client->actions[client->procActionIdx];
                BattleAction_SetNull(client->procAction);
                if (!AutoSelectAction(client, client->procMon, client->procAction)) {
                    u8 flags[4];
                    u8 moveIdx = StoreSelectableMoveFlag(client, client->procMon, flags);
                    if (moveIdx < 4) {
                        u16 move = MoveGetID(client->procMon, moveIdx);
                        u8 target = DecideMoveTargetAutoForClient(client->mainModule, client->pokeCon, client->procMon,
                                                                  move, &client->targetRand);
                        BattleAction_SetFightParam(client->procAction, move, target);
                    } else {
                        SetStruggleAction(client->procAction, client, client->procMon);
                    }
                }
            }
            (*seq)++;
        }
        break;
    case 2:
        client->returnData = &client->actions[0];
        client->returnDataSize = client->numCoverPos * 4;
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectFinish);
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectRoot(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        client->procMon = func_ov167_0219d1e8(client->pokeCon, client->clientId, client->procActionIdx);
        client->procAction = &client->actions[client->procActionIdx];
        client->extraActionCount = 0;
        BattleAction_SetNull(client->procAction);
        if (AutoSelectAction(client, client->procMon, client->procAction)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        } else if (CheckTrainerHintMsg(client, &client->hintMsgId) == TRUE) {
            BattleClient_SetSelActProc(client, BattleClient_ActionTrainerHint);
        } else {
            (*seq)++;
        }
        break;
    case 1:
        if (client->prevActionIdx != client->procActionIdx || client->forceActionMsg) {
            func_ov167_021b2110(client, client->procMon, &client->strParam);
            func_ov167_021d0234(client->viewCore, &client->strParam);
            client->forceActionMsg = FALSE;
            client->prevActionIdx = client->procActionIdx;
            (*seq)++;
        } else {
            *seq += 2;
        }
        break;
    case 2:
        if (!func_ov167_021d02e8(client->viewCore)) {
            return FALSE;
        }
        (*seq)++;
        // fallthrough
    case 3:
        func_ov167_021cefec(client->viewCore, client->procMon, client->procActionIdx > client->firstActionIdx,
                            client->procAction);
        (*seq)++;
        break;
    case 4:
        if (CheckActionSelectForceQuit(client, BattleClient_ActionForceQuit)) {
            func_ov167_021cf028(client->viewCore);
            return FALSE;
        }
        switch (func_ov167_021cf030(client->viewCore)) {
        case 3:
            if (CheckCondition(client->procMon, 0x21)) {
                func_ov167_021cf1f0(client->viewCore);
                *seq = 6;
            } else {
                shooterWork_SetCost(client, client->procActionIdx, 0);
                BattleClient_SetSelActProc(client, BattleClient_ActionSwitchPokemon);
            }
            break;
        case 1:
            shooterWork_SetCost(client, client->procActionIdx, 0);
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectFight);
            break;
        case 2:
            if (CheckCondition(client->procMon, 0x21)) {
                *seq = 6;
            } else if (client->bagMode == 1 && !func_ov167_0219db08(client->mainModule)) {
                func_ov167_021cf1f0(client->viewCore);
                *seq = 5;
            } else if (func_ov167_0219c988(client->mainModule) == 1) {
                BattleClient_SetSelActProc(client, func_ov167_021b4168);
            } else {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectItem);
            }
            break;
        case 4:
            if (client->procActionIdx == client->firstActionIdx) {
                if (CheckCondition(client->procMon, 0x21) && GetRunMode(client->mainModule) != 2) {
                    *seq = 6;
                } else {
                    shooterWork_SetCost(client, client->procActionIdx, 0);
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectEscape);
                }
            } else {
                while (client->procActionIdx != 0) {
                    client->procActionIdx--;
                    if (!AutoSelectAction(client,
                                          func_ov167_0219d1e8(client->pokeCon, client->clientId, client->procActionIdx),
                                          NULL)) {
                        client->shooterEnergy += shooterWork_GetCost(client, client->procActionIdx);
                        if (BattleAction_GetAction(&client->actions[client->procActionIdx]) == 3) {
                            func_ov169_0689cc90(&client->pokeSelect);
                        }
                        if (BattleAction_GetAction(&client->actions[client->procActionIdx]) == 2) {
                            selItemWork_Restore(client, client->procActionIdx);
                        }
                        BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
                        return FALSE;
                    }
                }
            }
            break;
        }
        break;
    case 5:
        if (func_ov167_021cf200(client->viewCore)) {
            *seq = 3;
        }
        break;
    case 6:
        func_ov167_021cf1e0(client->viewCore);
        (*seq)++;
        break;
    case 7:
        if (func_ov167_021cf200(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0xc5);
            Btlv_StringParam_AddArg(&client->strParam, GetMonID(client->procMon));
            func_ov167_021b350c(client, &client->strParam);
            (*seq)++;
        }
        break;
    case 8:
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 1;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionTrainerHint(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0: {
        u8 clientId = func_ov167_0219c8b8(client->mainModule, 0);
        u16 trainerId = func_ov167_0219d91c(client->mainModule, clientId);
        BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, clientId), 9, 0, 0, 0);
        BtlvEffect_Start(0x270);
        func_ov167_021d0288(client->viewCore, trainerId, client->hintMsgId);
        if ((client->hintMsgId == 19 || client->hintMsgId == 20) &&
            func_ov167_021bd760(func_ov167_0219d938(client->mainModule, clientId)) && !client->unk1BA_0) {
            BtlvEffect_ReserveBgm(0x47b);
            if (!BtlvEffect_IsPinchBgm()) {
                GFL_SndBGMFadeOut(8);
                *seq = 1;
                break;
            }
        }
        *seq = 2;
        break;
    }
    case 1:
        if (!GFL_SndBGMIsFading()) {
            GFL_SndBGMPlay(0x47b, 0xffff);
            client->unk1BA_0 = TRUE;
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy() && func_ov167_021d02e8(client->viewCore)) {
            BtlvEffect_Start(0x271);
            (*seq)++;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    }
    return FALSE;
}

static BOOL CheckTrainerHintMsg(BtlClient *client, u16 *msgId) {
    static const u16 sTrainerHintMsgs[4] = { 0x12, 0x11, 0x13, 0x14 };
    if (BtlSetup_GetBattleType(client->mainModule) == 1) {
        u32 i = 0;
        u8 clientId = func_ov167_0219c8b8(client->mainModule, 0);
        u16 trainerId = func_ov167_0219d91c(client->mainModule, clientId);

        if (trainerId != 0) {
            BattleParty *party = GetPartyData(client->pokeCon, clientId);
            BattleMon *mon = func_ov167_0219d1e8(client->pokeCon, clientId, i);
            s32 found;

            if (IsFainted(mon)) {
                u8 count = GetClientBattlerCount(client->mainModule, clientId);
                for (i = 1; i < count; i++) {
                    mon = func_ov167_0219d1e8(client->pokeCon, clientId, i);
                    if (!IsFainted(mon)) {
                        break;
                    }
                }
            }
            found = -1;
            for (i = 0; i < 4; i++) {
                if (client->hintShown[i] == 0) {
                    u32 msg = sTrainerHintMsgs[i];
                    if (TrainerMsg_CheckExists(trainerId, msg, client->heapId)) {
                        switch (msg) {
                        case 0x12:
                            if (GetBattleMonStat(mon, BATTLEMON_HP) <= DivideMaxHp(mon, 2)) {
                                found = i;
                                client->hintShown[i] = TRUE;
                            }
                            break;
                        case 0x11:
                            if (!IsMonFullHP(mon)) {
                                found = i;
                                client->hintShown[i] = TRUE;
                            }
                            break;
                        case 0x13:
                            if (GetNumMonsInParty(party) > 1 && GetAlivePartyCount(party) == 1) {
                                found = i;
                                client->hintShown[i] = TRUE;
                            }
                            break;
                        case 0x14:
                            if (GetNumMonsInParty(party) > 1 && GetAlivePartyCount(party) == 1 &&
                                GetBattleMonStat(mon, BATTLEMON_HP) <= DivideMaxHp(mon, 2)) {
                                found = i;
                                client->hintShown[i] = TRUE;
                            }
                            break;
                        }
                    } else {
                        client->hintShown[i] = TRUE;
                    }
                }
            }
            if (found >= 0) {
                *msgId = sTrainerHintMsgs[found];
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectFight(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        if (BtlSetup_GetBattleStyle(client->mainModule) != BTL_STYLE_ROTATION) {
            if (CheckForSelectableMove(client, client->procMon, client->procAction)) {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
            } else {
                *seq = 1;
            }
        } else {
            SetupRotationSelectParam(client, &client->rotationParam);
            *seq = 2;
        }
        break;
    case 1:
        func_ov167_021cf048(client->viewCore, client->procMon, client->procAction);
        *seq = 3;
        break;
    case 2:
        func_ov167_021cf094(client->viewCore, &client->rotationParam, client->procAction);
        *seq = 3;
        break;
    case 3:
        if (CheckActionSelectForceQuit(client, BattleClient_ActionForceQuit)) {
            func_ov167_021cf138(client->viewCore);
            return FALSE;
        }
        if (func_ov167_021cf140(client->viewCore)) {
            u32 action = BattleAction_GetAction(client->procAction);
            if (action == 0) {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
            } else if (action == 5) {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
            } else {
                BattleMon *mon = client->procMon;
                BattleAction *procAction = client->procAction;
                u8 rotatePos = 0;
                u32 pos = client->procActionIdx;

                if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION) {
                    rotatePos = client->rotationParam.rotateAction.bits.target;
                    pos = func_ov167_0219d38c(rotatePos);
                    mon = client->rotationParam.mons[pos].mon;
                }
                if (func_ov167_021bdb54(procAction)) {
                    client->moveInfoPos = pos;
                    client->moveInfoIdx = func_ov167_021baf78(mon, procAction->bits.move);
                    if (client->moveInfoIdx != 4) {
                        BattleClient_SetSelActProc(client, BattleClient_ActionViewMoveInfo);
                        return FALSE;
                    }
                }
                if (IsUnselectableMove(client, mon, procAction->bits.move, &client->strParam)) {
                    func_ov167_021d0b4c(&client->strParam, 0xff);
                    func_ov167_021b350c(client, &client->strParam);
                    *seq = 7;
                } else {
                    if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION && rotatePos != 0 &&
                        rotatePos != 1 && !IsFainted(mon)) {
                        u16 move = func_ov167_021bdb68(&client->rotationParam.action);
                        func_ov167_021bdc24(client->procAction++, rotatePos);
                        BattleAction_SetFightParam(client->procAction, move, 6);
                        client->extraActionCount++;
                    }
                    *seq = 4;
                }
            }
        }
        break;
    case 4:
        if (func_ov167_021bd718(BtlSetup_GetBattleStyle(client->mainModule))) {
            *seq = 5;
        } else {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        }
        break;
    case 5:
        func_ov167_021cf148(client->viewCore, client->procMon, client->procAction);
        *seq = 6;
        break;
    case 6: {
        u32 result;
        if (CheckActionSelectForceQuit(client, BattleClient_ActionForceQuit)) {
            func_ov167_021cf1ac(client->viewCore);
            return FALSE;
        }
        result = func_ov167_021cf174(client->viewCore);
        if (result == 1) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        } else if (result == 2) {
            *seq = 1;
        }
        break;
    }
    case 7:
        if (func_ov167_021d02e8(client->viewCore)) {
            func_ov167_021cf0e4(client->viewCore);
            *seq = 3;
        }
        break;
    }
    return FALSE;
}

static void SetupRotationSelectParam(BtlClient *client, BtlvSelectTargetParam *param) {
    u32 i;
    u32 j;

    for (i = 0; i < 3; i++) {
        BattleMon *mon = GetBattleMonFromParty(client->party, i);
        param->mons[i].mon = mon;
        for (j = 0; j < 4; j++) {
            param->mons[i].selectable[j] = FALSE;
        }
        if (!IsFainted(mon)) {
            u32 count = GetBattleMonMoveCount(mon);
            for (j = 0; j < count; j++) {
                if (GetMovePP(mon, j) == 0 || IsUnselectableMove(client, mon, MoveGetID(mon, j), NULL)) {
                    param->mons[i].selectable[j] = FALSE;
                } else {
                    param->mons[i].selectable[j] = TRUE;
                }
            }
        }
    }
}

static BOOL BattleClient_ActionViewMoveInfo(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        BattleClientCmd_StartMoveInfoView(client->viewCore, client->moveInfoPos, client->moveInfoIdx);
        (*seq)++;
        break;
    case 1:
        if (CheckActionSelectForceQuit(client, NULL)) {
            BattleClientCmd_QuitPokeSelect(client->viewCore);
            *seq = 2;
            return FALSE;
        }
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectFight);
        }
        break;
    case 2:
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionForceQuit);
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSwitchPokemon(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0: {
        u16 arg3;
        u8 monId;
        u8 canCancel = FALSE;
        u8 result = CanMonSwitch(client, client->procMon, &monId, &arg3);
        if (result != 4 || monId == GetMonID(client->procMon)) {
            canCancel = TRUE;
        }
        BattleClientCmd_StartPokeList(client->viewCore, &client->pokeListCmd, client->procActionIdx, canCancel,
                                      &client->pokeSelect);
        (*seq)++;
        break;
    }
    case 1:
        if (CheckActionSelectForceQuit(client, NULL)) {
            BattleClientCmd_QuitPokeSelect(client->viewCore);
            *seq = 2;
            return FALSE;
        }
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            if (!func_ov169_0689cc9c(&client->pokeSelect)) {
                u8 slot = func_ov169_0689cca4(&client->pokeSelect);
                if (slot < 6) {
                    func_ov167_021bdbbc(client->procAction, client->procActionIdx, slot);
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
                    break;
                }
            }
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    case 2:
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionForceQuit);
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectItem(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0: {
        u8 cost = shooterWork_GetTotalCost(client);
        u8 isFirst = (client->procActionIdx == client->firstActionIdx);
        u8 hiding = IsPokeBallTargetHiding(client);
        BattleClientCmd_StartItemSelect(client->viewCore, client->bagMode, client->shooterEnergy, cost, isFirst,
                                        hiding);
        (*seq)++;
        break;
    }
    case 1:
        if (CheckActionSelectForceQuit(client, NULL)) {
            func_ov167_021cf72c(client->viewCore);
            *seq = 2;
            return FALSE;
        }
        if (func_ov167_021cf73c(client->viewCore)) {
            u16 item = func_ov167_021cf8d8(client->viewCore);
            u16 target = func_ov167_021cf900(client->viewCore);
            if (item != 0 && target != 6) {
                u8 cost = func_ov167_021cf8ec(client->viewCore);
                u32 param = func_ov167_021cf908(client->viewCore);
                if (client->shooterEnergy >= cost) {
                    client->shooterEnergy -= cost;
                } else {
                    client->shooterEnergy = 0;
                    cost = client->shooterEnergy;
                }
                shooterWork_SetCost(client, client->procActionIdx, cost);
                func_ov167_021bdb80(client->procAction, item, target, param);
                func_ov167_021cf914(client->viewCore);
                if (ItemGetParam(item, 15) == 4) {
                    client->escapeSelected = TRUE;
                }
                selItemWork_Reserve(client, client->procActionIdx, item);
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
            } else {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
            }
        }
        break;
    case 2:
        if (func_ov167_021cf73c(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionForceQuit);
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b4168(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        func_ov167_021cf95c(client->viewCore, client->turnCount);
        (*seq)++;
        break;
    case 1:
        if (CheckActionSelectForceQuit(client, NULL)) {
            func_ov167_021cf9c0(client->viewCore);
            *seq = 2;
            return FALSE;
        }
        if (func_ov167_021cf9cc(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    case 2:
        if (func_ov167_021cf9cc(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b41d4(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        func_ov167_021cfaec(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cfb40(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL IsPokeBallTargetHiding(BtlClient *client) {
    if (BtlSetup_GetBattleType(client->mainModule) == 0) {
        BattleParty *party = GetClientParty(client->pokeCon, 1);
        u32 numMons = GetNumMonsInParty(party);
        u32 numFront = func_ov167_0219be8c(client->mainModule);
        BOOL hiding;
        u32 i;
        u32 alive;

        if (numFront > numMons) {
            numFront = numMons;
        }
        hiding = FALSE;
        alive = 0;
        for (i = 0; i < numFront; i++) {
            BattleMon *mon = GetBattleMonFromParty(party, i);
            if (!IsFainted(mon)) {
                alive++;
                if (IsSemiInvulnMove(mon)) {
                    hiding = TRUE;
                }
            }
        }
        if (alive == 1 && hiding) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectEscape(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0: {
        u16 trapAbility;
        u8 trapMonId;

        switch (GetRunMode(client->mainModule)) {
        case 0:
        default:
            if (CheckEscapeBlocked(client, &trapMonId, &trapAbility) == 4) {
                *seq = 5;
            } else {
                if (trapAbility != 0) {
                    Btlv_StringParam_Setup(&client->strParam, 2, 0x365);
                    Btlv_StringParam_AddArg(&client->strParam, trapMonId);
                    Btlv_StringParam_AddArg(&client->strParam, trapAbility);
                } else {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x4a);
                }
                func_ov167_021b350c(client, &client->strParam);
                *seq = 1;
            }
            break;
        case 1:
            Btlv_StringParam_Setup(&client->strParam, 1, 0x4c);
            func_ov167_021b350c(client, &client->strParam);
            *seq = 1;
            break;
        case 2:
            Btlv_StringParam_Setup(&client->strParam, 1, 0x4d);
            func_ov167_021d0b4c(&client->strParam, 0xff);
            func_ov167_021b350c(client, &client->strParam);
            *seq = 3;
            break;
        }
        break;
    }
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            func_ov167_021b2110(client, client->procMon, &client->strParam);
            func_ov167_021d0234(client->viewCore, &client->strParam);
            *seq = 2;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    case 3:
        func_ov167_021d02e8(client->viewCore);
        if (func_ov167_021d02f8(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 3, 8);
            Btlv_StringParam_Setup(&client->strParam2, 3, 9);
            func_ov167_021d0798(client->viewCore, &client->strParam, &client->strParam2, 1);
            *seq = 4;
        }
        break;
    case 4:
        if (func_ov167_021d02e8(client->viewCore)) {
            u32 result;

            if (func_ov167_021d0828(client->viewCore, &result)) {
                if (result == 0) {
                    *seq = 5;
                } else {
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
                }
            } else if (CheckIfOverCmdLimit(client)) {
                BattleClient_SetSelActProc(client, BattleClient_ActionForceQuit);
            }
        }
        break;
    case 5:
        func_ov167_021bdc3c(client->procAction);
        client->escapeSelected = TRUE;
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectAutoRest(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        func_ov167_021cf1f0(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cf200(client->viewCore)) {
            client->procActionIdx++;
            if (client->procActionIdx < client->numCoverPos) {
                if (!client->escapeSelected) {
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
                    break;
                }
                while (client->procActionIdx < client->numCoverPos) {
                    BattleMon *mon = func_ov167_0219d1e8(client->pokeCon, client->clientId, client->procActionIdx);
                    if (!AutoSelectAction(client, mon, &client->actions[client->procActionIdx])) {
                        BattleAction_SetNull(&client->actions[client->procActionIdx]);
                    }
                    client->procActionIdx++;
                }
            }
            if (client->procActionIdx >= client->numCoverPos) {
                u8 count = client->numCoverPos + client->extraActionCount;
                client->extraActionCount = 0;
                client->returnData = client->actions;
                client->returnDataSize = count * sizeof(BattleAction);
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectFinish);
            }
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectFinish(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        selItemWork_Quit(client);
        if (func_ov167_0219bee4(client->mainModule)) {
            client->waitMsgShown = TRUE;
            func_ov167_021d03d4(client->viewCore);
            (*seq)++;
        } else {
            *seq += 2;
        }
        break;
    case 1:
        if (func_ov167_021d03ec(client->viewCore)) {
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021b20f8(client);
            (*seq)++;
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static void selItemWork_Init(BtlClient *client) {
    u32 i;
    for (i = 0; i < 3; i++) {
        client->reservedItems[i] = 0;
    }
}

static void selItemWork_Reserve(BtlClient *client, u8 index, u16 item) {
    if (client->bagMode == 0 && client->clientId == GetPlayerClientID(client->mainModule)) {
        client->reservedItems[index] = item;
        BattleClient_SubItem(client->mainModule, client->clientId, item);
    }
}

static void selItemWork_Restore(BtlClient *client, u8 index) {
    if (client->bagMode == 0 && client->clientId == GetPlayerClientID(client->mainModule)) {
        u16 item = client->reservedItems[index];
        if (item != 0) {
            client->reservedItems[index] = 0;
            BattleClient_AddItem(client->mainModule, client->clientId, item);
        }
    }
}

static void selItemWork_Quit(BtlClient *client) {
    u32 i;
    if (client->bagMode == 0) {
        for (i = 0; i < 3; i++) {
            selItemWork_Restore(client, i);
        }
    }
}

static void shooterWork_Init(BtlClient *client) {
    sys_memset(client->shooterCost, 0, 3);
}

static void shooterWork_SetCost(BtlClient *client, u8 index, u8 cost) {
    client->shooterCost[index] = cost;
}

static u8 shooterWork_GetCost(BtlClient *client, u8 index) {
    return client->shooterCost[index];
}

static u8 shooterWork_GetTotalCost(BtlClient *client) {
    u32 i;
    u32 total = 0;
    for (i = 0; i < client->procActionIdx; i++) {
        total += client->shooterCost[i];
    }
    return total;
}

static BOOL AutoSelectAction(BtlClient *client, BattleMon *mon, BattleAction *action) {
    if (IsFainted(mon)) {
        if (action != NULL) {
            BattleAction_SetNull(action);
        }
        return TRUE;
    }
    if (GetAdditionalConditionFlag(mon, 12)) {
        if (action != NULL) {
            BattleAction_SetSkip(action);
        }
        return TRUE;
    }
    if (CheckCondition(mon, 25)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, 25);
        u16 move = Condition_GetParam(cont);
        u8 target = GetPrevTargetPos(mon);
        if (action != NULL) {
            BattleAction_SetFightParam(action, move, target);
        }
        return TRUE;
    }
    if (CheckCondition(mon, 26)) {
        u16 move = GetPreviousMoveID(mon);
        u8 target = GetPrevTargetPos(mon);
        func_ov167_021baf78(mon, move);
        if (action != NULL) {
            BattleAction_SetFightParam(action, move, target);
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL CheckForSelectableMove(BtlClient *client, BattleMon *mon, BattleAction *action) {
    u8 moveCount = GetBattleMonMoveCount(mon);
    u32 selectable = 0;
    u32 i;
    for (i = 0; i < moveCount; i++) {
        if (!IsUnselectableMove(client, mon, MoveGetID(mon, i), NULL) && GetMovePP(mon, i) != 0) {
            selectable++;
        }
    }
    if (selectable == 0) {
        SetStruggleAction(action, client, mon);
        return TRUE;
    }
    return FALSE;
}

static void SetStruggleAction(BattleAction *action, BtlClient *client, BattleMon *mon) {
    u8 target =
        DecideMoveTargetAutoForClient(client->mainModule, client->pokeCon, mon, MOVE_STRUGGLE, &client->targetRand);
    BattleAction_SetFightParam(action, MOVE_STRUGGLE, target);
}

static BOOL IsUnselectableMove(BtlClient *client, BattleMon *mon, u16 move, BtlvStringParam *param) {
    if (move == MOVE_STRUGGLE) {
        return FALSE;
    }
    if (CanMonUseHeldItem(client, mon) && CheckCondition(mon, 27)) {
        u16 lockedMove = Condition_GetParam(GetConditionContinuationParam(mon, 27));
        if (MoveIsUsable(mon, lockedMove) && lockedMove != move) {
            if (param != NULL) {
                Btlv_StringParam_Setup(param, 1, 0x63);
                Btlv_StringParam_AddArg(param, GetBattleMonHeldItem(mon));
                Btlv_StringParam_AddArg(param, lockedMove);
            }
            return TRUE;
        }
    }
    if (CheckCondition(mon, 23)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, 23);
        u16 encoreMove = Condition_GetParam(cont);
        if (move != encoreMove) {
            if (param != NULL) {
                Btlv_StringParam_Setup(param, 1, 0x64);
                Btlv_StringParam_AddArg(param, GetMonID(mon));
                Btlv_StringParam_AddArg(param, encoreMove);
            }
            return TRUE;
        }
    }
    if (CheckCondition(mon, CONDITION_TAUNT) && !PML_MoveIsDamaging(move)) {
        if (param != NULL) {
            Btlv_StringParam_Setup(param, 2, 0x23b);
            Btlv_StringParam_AddArg(param, GetMonID(mon));
            Btlv_StringParam_AddArg(param, move);
        }
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_TORMENT) && move == GetPreviousMoveUsed(mon)) {
        if (param != NULL) {
            Btlv_StringParam_Setup(param, 2, 0x244);
            Btlv_StringParam_AddArg(param, GetMonID(mon));
            Btlv_StringParam_AddArg(param, move);
        }
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_DISABLE) && move == GetDisabledMove(mon, CONDITION_DISABLE) &&
        move != MOVE_STRUGGLE) {
        if (param != NULL) {
            Btlv_StringParam_Setup(param, 2, 0x253);
            Btlv_StringParam_AddArg(param, GetMonID(mon));
            Btlv_StringParam_AddArg(param, move);
        }
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_HEAL_BLOCK) && getMoveFlag(move, 12)) {
        if (param != NULL) {
            Btlv_StringParam_Setup(param, 2, 0x37a);
            Btlv_StringParam_AddArg(param, GetMonID(mon));
            Btlv_StringParam_AddArg(param, move);
        }
        return TRUE;
    }
    if (CheckFieldEffect(client->field, 3) && CheckImprison(client->field, client->pokeCon, mon, move)) {
        if (param != NULL) {
            Btlv_StringParam_Setup(param, 2, 0x24d);
            Btlv_StringParam_AddArg(param, GetMonID(mon));
            Btlv_StringParam_AddArg(param, move);
        }
        return TRUE;
    }
    if (CheckFieldEffect(client->field, FIELD_CONDITION_GRAVITY) && getMoveFlag(move, 9)) {
        if (param != NULL) {
            Btlv_StringParam_Setup(param, 2, 0x43e);
            Btlv_StringParam_AddArg(param, GetMonID(mon));
            Btlv_StringParam_AddArg(param, move);
        }
        return TRUE;
    }
    return FALSE;
}

static u8 StoreSelectableMoveFlag(BtlClient *client, BattleMon *mon, u8 *selectable) {
    u8 moveCount = GetBattleMonMoveCount(mon);
    u8 first = 4;
    u8 i;
    for (i = 0; i < moveCount; i++) {
        if (GetMovePP(mon, i) != 0) {
            selectable[i] = !IsUnselectableMove(client, mon, MoveGetID(mon, i), NULL);
            if (first == 4 && selectable[i]) {
                first = i;
            }
        } else {
            selectable[i] = FALSE;
        }
    }
    for (; i < 4; i++) {
        selectable[i] = FALSE;
    }
    return first;
}

static BOOL CanMonUseHeldItem(BtlClient *client, BattleMon *mon) {
    if (CheckFieldEffect(client->field, 7)) {
        return FALSE;
    }
    if (CheckCondition(mon, CONDITION_EMBARGO)) {
        return FALSE;
    }
    if (GetBattleMonStat(mon, 0x11) != ABILITY_KLUTZ) {
        return TRUE;
    }
    return FALSE;
}

static u32 CanMonSwitch(BtlClient *client, BattleMon *mon, u8 *trapMonId, u16 *trapAbility) {
    u32 result;
    if (CanMonUseHeldItem(client, mon) && GetBattleMonHeldItem(mon) == ITEM_SHED_SHELL) {
        return 4;
    }
    result = IsMonTrapped(client, mon, trapMonId, trapAbility);
    if (result == 4) {
        *trapMonId = 31;
        *trapAbility = 0;
        return 4;
    }
    return result;
}

static u32 CheckEscapeBlocked(BtlClient *client, u8 *trapMonId, u16 *trapAbility) {
    u32 i;
    for (i = 0; i < client->numCoverPos; i++) {
        BattleMon *mon = GetBattleMonFromParty(client->party, i);
        if (!IsFainted(mon)) {
            if (CanMonUseHeldItem(client, mon) && GetBattleMonHeldItem(mon) == ITEM_SMOKE_BALL) {
                return 4;
            }
            if (GetBattleMonStat(mon, 0x11) == ABILITY_RUN_AWAY) {
                return 4;
            }
        }
    }
    for (i = 0; i < client->numCoverPos; i++) {
        u32 result = IsMonTrapped(client, GetBattleMonFromParty(client->party, i), trapMonId, trapAbility);
        if (result != 4) {
            return result;
        }
    }
    return 4;
}

static u32 IsMonTrapped(BtlClient *client, BattleMon *mon, u8 *trapMonId, u16 *trapAbility) {
    u8 monIds[4];
    u8 count =
        func_ov167_0219c5a4(client->mainModule, client->pokeCon,
                            0x100 | MonIDToBattlePos(client->mainModule, client->pokeCon, GetMonID(mon)), monIds);
    u8 i;
    for (i = 0; i < count; i++) {
        BattleMon *opponent = GetPokeParam(client->pokeCon, monIds[i]);
        u16 ability = GetBattleMonStat(opponent, 0x11);
        // The ID is read and dropped here, likely left from a removed debug print
        GetMonID(opponent);
        if (ability == ABILITY_SHADOW_TAG && DoesMonHaveShadowTag(client, mon)) {
            *trapMonId = GetMonID(opponent);
            *trapAbility = ability;
            return 0;
        }
        if (ability == ABILITY_ARENA_TRAP && IsMonTrappedByArenaTrap(client, mon)) {
            *trapMonId = GetMonID(opponent);
            *trapAbility = ability;
            return 0;
        }
        if (ability == ABILITY_MAGNET_PULL && IsMonSteelType(client, mon)) {
            *trapMonId = GetMonID(opponent);
            *trapAbility = ability;
            return 0;
        }
    }
    if (CheckCondition(mon, CONDITION_MEAN_LOOK) || CheckCondition(mon, CONDITION_BIND) ||
        CheckCondition(mon, CONDITION_INGRAIN)) {
        *trapMonId = GetMonID(mon);
        *trapAbility = 0;
        return 3;
    }
    return 4;
}

static BOOL DoesMonHaveShadowTag(BtlClient *client, BattleMon *mon) {
    if (GetBattleMonStat(mon, 0x11) != ABILITY_SHADOW_TAG) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IsMonTrappedByArenaTrap(BtlClient *client, BattleMon *mon) {
    BOOL canUseItem = CanMonUseHeldItem(client, mon);
    if (CheckFieldEffect(client->field, FIELD_CONDITION_GRAVITY)) {
        return TRUE;
    }
    if (CheckCondition(mon, 31)) {
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_INGRAIN)) {
        return TRUE;
    }
    if (canUseItem && GetBattleMonHeldItem(mon) == ITEM_IRON_BALL) {
        return TRUE;
    }
    if (GetBattleMonStat(mon, 0x11) == ABILITY_LEVITATE) {
        return FALSE;
    }
    if (DoesMonHaveType(mon, TYPE_FLYING)) {
        return FALSE;
    }
    if (CheckCondition(mon, 30)) {
        return FALSE;
    }
    if (CheckCondition(mon, 32)) {
        return FALSE;
    }
    if (canUseItem && GetBattleMonHeldItem(mon) == ITEM_AIR_BALLOON) {
        return FALSE;
    }
    return TRUE;
}

static BOOL IsMonSteelType(BtlClient *client, BattleMon *mon) {
    if (DoesMonHaveType(mon, TYPE_STEEL)) {
        return TRUE;
    }
    return FALSE;
}

static void ClearAISwitchReserved(BtlClient *client) {
    u32 i;
    for (i = 0; i < 6; i++) {
        client->aiSwitchReserved[i] = -1;
    }
}

static BOOL IsAISwitchReserved(BtlClient *client, u8 slot) {
    u32 i;
    for (i = 0; i < 6; i++) {
        if (slot == client->aiSwitchReserved[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static void ReserveAISwitch(BtlClient *client, u8 index, u8 slot) {
    if (index < 6) {
        client->aiSwitchReserved[index] = slot;
    }
}

static BOOL AISwitchChecks(BtlClient *client, BattleMon *mon, u8 index, u8 *slot) {
    u8 switchTo = 6;
    u8 trapMonId;
    u16 trapAbility;
    u8 slots[6];
    BattleMon *target;
    BOOL shouldSwitch;

    if (CanMonSwitch(client, mon, &trapMonId, &trapAbility) != 4) {
        return FALSE;
    }
    if (GetAlivePartyCount(client->party) <= client->numCoverPos) {
        return FALSE;
    }
    target = PickRandomOpponent(client, MonIDToBattlePos(client->mainModule, client->pokeCon, GetMonID(mon)));
    if (target == NULL) {
        return FALSE;
    }
    if (IsIllusionEnabled(target)) {
        target = GetIllusionDisguise(client->mainModule, client->pokeCon, target);
    }
    if (ShouldSwitchIfPerishSongLastTurn(client, mon)) {
        shouldSwitch = TRUE;
    } else if (ShouldSwitchIfWonderGuard(client, mon, target)) {
        shouldSwitch = TRUE;
    } else if (ShouldSwitchIfNoEffectiveMoves(client, mon, target)) {
        shouldSwitch = TRUE;
    } else if (ShouldSwitchIfChoicedIntoIneffectiveMove(client, mon, target)) {
        shouldSwitch = TRUE;
    } else if (ShouldSwitchIfTypeAbsorbingAbility(client, mon, target, &switchTo)) {
        shouldSwitch = TRUE;
    } else if (ShouldSwitchIfAsleepWithNaturalCure(client, mon, &switchTo)) {
        shouldSwitch = TRUE;
    } else if (FinalSwitchChecks(client, mon, target, &switchTo)) {
        shouldSwitch = TRUE;
    } else {
        return FALSE;
    }
    if (!shouldSwitch) {
        return FALSE;
    }
    if (switchTo == 6) {
        u32 count = GetNumBattleReadyPartyMons(client, slots);
        u32 i;
        PickBestMonToSwitchInto(client, slots, count, target);
        for (i = 0; i < count; i++) {
            if (!IsAISwitchReserved(client, slots[i])) {
                switchTo = slots[i];
                break;
            }
        }
    }
    if (switchTo == 6) {
        return FALSE;
    }
    ReserveAISwitch(client, index, switchTo);
    *slot = switchTo;
    return TRUE;
}

static BattleMon *PickRandomOpponent(BtlClient *client, u8 pos) {
    u8 monIds[4];
    u8 count = func_ov167_0219c5a4(client->mainModule, client->pokeCon, 0x100 | pos, monIds);
    if (count != 0) {
        u8 index = MATH_Rand32(&client->aiRand, count);
        return GetPokeParam(client->pokeCon, monIds[index]);
    }
    return NULL;
}

static BOOL ShouldSwitchIfPerishSongLastTurn(BtlClient *client, BattleMon *mon) {
    if (CheckCondition(mon, CONDITION_PERISH_SONG)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, CONDITION_PERISH_SONG);
        u8 turns = func_ov167_021ce33c(cont);
        u8 count = func_ov167_021bbb1c(mon, CONDITION_PERISH_SONG);
        if (count + 1 == turns) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL ShouldSwitchIfWonderGuard(BtlClient *client, BattleMon *mon, BattleMon *target) {
    if (BtlSetup_GetBattleStyle(client->mainModule) != BTL_STYLE_SINGLE) {
        return FALSE;
    }
    if (GetBattleMonStat(target, 0x11) == ABILITY_WONDER_GUARD &&
        !DoesMonHaveSuperEffectiveMove(client, mon, target, TYPE_EFFECTIVENESS_DOUBLE) &&
        CheckIfMonToSwitchToWithSEMove(client, target, TYPE_EFFECTIVENESS_DOUBLE) &&
        MATH_Rand32(&client->aiRand, 3) < 2) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ShouldSwitchIfNoEffectiveMoves(BtlClient *client, BattleMon *mon, BattleMon *target) {
    PokeTypePair targetType = GetPokeType(target);
    u8 moveCount = GetBattleMonMoveCount(mon);
    u32 i;
    u32 damaging = 0;
    for (i = 0; i < moveCount; i++) {
        u16 move = MoveGetID(mon, i);
        if (PML_MoveIsDamaging(move)) {
            if (func_ov167_021bd1b0(PML_MoveGetType(move), targetType) != TYPE_EFFECTIVENESS_IMMUNE) {
                return FALSE;
            }
            damaging++;
        }
    }
    if (damaging >= 2) {
        if (CheckIfMonToSwitchToWithSEMove(client, target, TYPE_EFFECTIVENESS_DOUBLE)) {
            return MATH_Rand32(&client->aiRand, 3) < 2;
        }
        if (CheckIfMonToSwitchToWithSEMove(client, target, TYPE_EFFECTIVENESS_NORMAL)) {
            if (MATH_Rand32(&client->aiRand, 2) < 1) {
                return TRUE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

static BOOL ShouldSwitchIfChoicedIntoIneffectiveMove(BtlClient *client, BattleMon *mon, BattleMon *target) {
    if (CheckCondition(mon, 27)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, 27);
        u16 move = Condition_GetParam(cont);
        if (move != 0) {
            BOOL isDamaging = PML_MoveIsDamaging(move);
            u8 type = PML_MoveGetType(move);
            if (func_ov167_021bd1b0(type, GetPokeType(target)) == TYPE_EFFECTIVENESS_IMMUNE && isDamaging) {
                if (CheckIfMonToSwitchToWithSEMove(client, target, TYPE_EFFECTIVENESS_DOUBLE)) {
                    return MATH_Rand32(&client->aiRand, 3) < 2;
                }
                if (CheckIfMonToSwitchToWithSEMove(client, target, TYPE_EFFECTIVENESS_NORMAL)) {
                    if (MATH_Rand32(&client->aiRand, 2) < 1) {
                        return TRUE;
                    }
                    return FALSE;
                }
            }
            if (!isDamaging) {
                if (MATH_Rand32(&client->aiRand, 2) < 1) {
                    return TRUE;
                }
                return FALSE;
            }
        }
    }
    return FALSE;
}

static BOOL ShouldSwitchIfTypeAbsorbingAbility(BtlClient *client, BattleMon *mon, BattleMon *target, u8 *slot) {
    BattleMonDamageRecord record;
    u8 found;
    u32 i;
    u8 turnsAgo = 0;
    u32 j;

    if (DoesMonHaveSuperEffectiveMove(client, mon, target, TYPE_EFFECTIVENESS_DOUBLE) &&
        MATH_Rand32(&client->aiRand, 3) == 0) {
        return FALSE;
    }
    while (GetDamageReceived(mon, 1, turnsAgo++, &record)) {
        for (i = 0; i < 4; i++) {
            const TypeAbsorbingAbilities *entry = &sTypeAbsorbingAbilities[i];
            if (record.type == entry->type) {
                for (j = 0; j < 4; j++) {
                    if (entry->abilities[j] == 0) {
                        break;
                    }
                    if (CheckMonsForTypeAbsorbingAbility(client, entry->abilities[j], &found) &&
                        MATH_Rand32(&client->aiRand, 2) == 0) {
                        *slot = found;
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

static BOOL ShouldSwitchIfAsleepWithNaturalCure(BtlClient *client, BattleMon *mon, u8 *slot) {
    BattleMonDamageRecord record;
    u8 found;

    if (GetBattleMonStat(mon, 0x11) == ABILITY_NATURAL_CURE &&
        (CheckCondition(mon, CONDITION_SLEEP) || CheckCondition(mon, CONDITION_FREEZE)) &&
        GetHPRatio(mon) >= FX32_CONST(50)) {
        if (GetDamageReceived(mon, 1, 0, &record)) {
            if (FindPartyMonResistingType(client, record.type, TYPE_EFFECTIVENESS_HALF, &found)) {
                *slot = found;
                return TRUE;
            }
        } else {
            if (MATH_Rand32(&client->aiRand, 2) == 0) {
                return TRUE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

static BOOL FinalSwitchChecks(BtlClient *client, BattleMon *mon, BattleMon *target, u8 *slot) {
    BattleMonDamageRecord record;
    u32 stages;
    u32 stat;
    u32 numMons;
    u32 i;

    if (DoesMonHaveSuperEffectiveMove(client, mon, target, TYPE_EFFECTIVENESS_DOUBLE) &&
        MATH_Rand32(&client->aiRand, 10) != 0) {
        return FALSE;
    }
    stages = 0;
    for (stat = BATTLEMON_ATTACK_STAGE; stat <= BATTLEMON_EVASION_STAGE; stat++) {
        stages += GetBattleMonStat(mon, stat);
    }
    if (stages >= 4) {
        return FALSE;
    }
    if (GetDamageReceived(mon, 1, 0, &record)) {
        numMons = GetNumMonsInParty(client->party);
        for (i = GetNumMonsOnField(BtlSetup_GetBattleStyle(client->mainModule), client->numCoverPos); i < numMons;
             i++) {
            if (!IsAISwitchReserved(client, i)) {
                BattleMon *candidate = GetBattleMonFromParty(client->party, i);
                if (DoesMonHaveSuperEffectiveMove(client, candidate, target, TYPE_EFFECTIVENESS_DOUBLE)) {
                    s32 effectiveness = func_ov167_021bd1b0(record.type, GetPokeType(candidate));
                    if (effectiveness == TYPE_EFFECTIVENESS_IMMUNE) {
                        if (MATH_Rand32(&client->aiRand, 2) == 0) {
                            *slot = i;
                            return TRUE;
                        }
                    } else if (effectiveness < TYPE_EFFECTIVENESS_NORMAL) {
                        if (MATH_Rand32(&client->aiRand, 3) == 0) {
                            *slot = i;
                            return TRUE;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

static BOOL CheckMonsForTypeAbsorbingAbility(BtlClient *client, u16 ability, u8 *slot) {
    u32 numMons = GetNumMonsInParty(client->party);
    u32 i;
    for (i = GetNumMonsOnField(BtlSetup_GetBattleStyle(client->mainModule), client->numCoverPos); i < numMons; i++) {
        if (!IsAISwitchReserved(client, i)) {
            BattleMon *candidate = GetBattleMonFromParty(client->party, i);
            if (!IsFainted(candidate) && ability == GetBattleMonStat(candidate, 0x11)) {
                *slot = i;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL CheckIfMonToSwitchToWithSEMove(BtlClient *client, BattleMon *target, s32 minEffectiveness) {
    u32 numMons = GetNumMonsInParty(client->party);
    u32 i;
    for (i = GetNumMonsOnField(BtlSetup_GetBattleStyle(client->mainModule), client->numCoverPos); i < numMons; i++) {
        if (!IsAISwitchReserved(client, i) &&
            DoesMonHaveSuperEffectiveMove(client, GetBattleMonFromParty(client->party, i), target, minEffectiveness)) {
            return TRUE;
        }
    }
    return FALSE;
}

static inline void SetReturnData(BtlClient *client, const void *data, u32 size) {
    client->returnData = data;
    client->returnDataSize = size;
}

// The party mon at an index; the index is wider than GetBattleMonFromParty's, which narrows it at each call
static inline BattleMon *GetPartyMonAt(BtlClient *client, u32 idx) {
    return GetBattleMonFromParty(client->party, idx);
}

// Finds a party mon in the back that takes at most maxEffect from moveType
static BOOL FindPartyMonResistingType(BtlClient *client, u8 moveType, s32 maxEffect, u8 *partyIdx) {
    u8 count = GetNumMonsInParty(client->party);
    u32 i;
    BattleMon *mon;

    for (i = GetNumMonsOnField(BtlSetup_GetBattleStyle(client->mainModule), client->numCoverPos); i < count; i++) {
        if (!IsAISwitchReserved(client, i)) {
            mon = GetBattleMonFromParty(client->party, i);
            if (CanPokemonBattle(mon) && func_ov167_021bd1b0(moveType, GetPokeType(mon)) <= maxEffect) {
                *partyIdx = i;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL DoesMonHaveSuperEffectiveMove(BtlClient *client, BattleMon *attacker, BattleMon *defender, s32 minEffect) {
    PokeTypePair types;
    u8 moveCount;
    u32 i;
    u16 move;

    if (!IsFainted(attacker)) {
        types = GetPokeType(defender);
        moveCount = GetBattleMonMoveCount(attacker);
        for (i = 0; i < moveCount; i++) {
            move = MoveGetID(attacker, i);
            if (GetMovePP(attacker, i) != 0 && PML_MoveIsDamaging(move) &&
                !IsUnselectableMove(client, attacker, move, NULL) &&
                func_ov167_021bd1b0(PML_MoveGetType(move), types) >= minEffect) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Picks a rotation position to rotate to at random among the ones whose mon can fight, or 1 to stay
static u8 AIPickRotationPos(BtlClient *client) {
    u8 positions[3];
    u32 count = 1;

    positions[0] = 1;
    if (!IsFainted(GetPartyMonAt(client, func_ov167_0219d38c(2)))) {
        positions[count++] = 2;
    }
    if (!IsFainted(GetPartyMonAt(client, func_ov167_0219d38c(3)))) {
        positions[count++] = 3;
    }
    if (count > 1) {
        return positions[MATH_Rand32(&client->aiRand, count)];
    }
    return 1;
}

static BOOL AISelectAction(BtlClient *client, s32 *seq) {
    u8 usableMoves[4];
    u8 partyIdx;
    u16 item;
    u8 rotatePos;
    u8 partyPos;
    u16 move;
    u8 moveIdx;
    u8 target;
    BtlMainUnk478 *debug;

    switch (*seq) {
    case 0:
        if (func_ov167_0219c988(client->mainModule) == 1) {
            if (func_ov167_021b2188() >= 8) {
                *seq = 1;
            }
        } else {
            *seq = 1;
        }
        break;
    case 1:
        ClearAISwitchReserved(client);
        client->procActionIdx = 0;
        client->extraActionCount = 0;
        *seq = 2;
        // fallthrough
    case 2:
        if (client->procActionIdx >= client->numCoverPos) {
            break;
        }
        client->procMon = GetBattleMonFromParty(client->party, client->procActionIdx);
        client->procAction = &client->actions[client->procActionIdx];
        if (IsFainted(client->procMon)) {
            BattleAction_SetNull(client->procAction);
            *seq = 4;
            break;
        }
        if (AutoSelectAction(client, client->procMon, client->procAction)) {
            *seq = 4;
            break;
        }
        item = AICheckItemUse(client, client->procMon, client->party);
        if (item != 0) {
            func_ov167_021bdb80(client->procAction, item, client->procActionIdx, 0);
            *seq = 4;
            break;
        }
        if (BtlSetup_GetBattleType(client->mainModule) != 0 &&
            AISwitchChecks(client, client->procMon, client->procActionIdx, &partyIdx)) {
            func_ov167_021bdbbc(client->procAction, client->procActionIdx, partyIdx);
            *seq = 4;
            break;
        }
        if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION) {
            rotatePos = AIPickRotationPos(client);
            if (rotatePos != 1) {
                partyPos = func_ov167_0219d38c(rotatePos);
                func_ov167_021bdc24(client->procAction, rotatePos);
                client->procMon = GetBattleMonFromParty(client->party, partyPos);
                client->extraActionCount = 1;
                client->procAction++;
            }
        }
        if (CheckCondition(client->procMon, 25)) {
            move = Condition_GetParam(GetConditionContinuationParam(client->procMon, 25));
            BattleAction_SetFightParam(client->procAction, move,
                                       DecideMoveTargetAutoForClient(client->mainModule, client->pokeCon,
                                                                     client->procMon, move, &client->aiRand));
            *seq = 4;
            break;
        }
        if (CheckForSelectableMove(client, client->procMon, client->procAction)) {
            *seq = 4;
        }
        GetBattleMonMoveCount(client->procMon);
        if (StoreSelectableMoveFlag(client, client->procMon, usableMoves) != 4) {
            TrAI_Setup(client->aiVM, usableMoves,
                       MonIDToBattlePos(client->mainModule, client->pokeCon, GetMonID(client->procMon)));
            *seq = 3;
            break;
        }
        SetStruggleAction(client->procAction, client, client->procMon);
        *seq = 4;
        break;
    case 3:
        if (TrAI_Think(client->aiVM)) {
            break;
        }
        moveIdx = TrAI_GetChosenMove(client->aiVM);
        if (moveIdx != 4) {
            target = TrAI_GetChosenTarget(client->aiVM);
            debug = func_ov167_0219e3ac(client->mainModule);
            if (debug != NULL && debug->unk08 >= 0) {
                moveIdx = debug->unk08;
            }
            BattleAction_SetFightParam(client->procAction, MoveGetID(client->procMon, moveIdx), target);
            *seq = 4;
            break;
        }
        func_ov167_021bdc3c(client->procAction);
        *seq = 4;
        break;
    case 4:
        client->procActionIdx++;
        if (client->procActionIdx >= client->numCoverPos) {
            SetReturnData(client, client->actions,
                          (client->numCoverPos + client->extraActionCount) * sizeof(BattleAction));
            client->extraActionCount = 0;
            return TRUE;
        }
        *seq = 2;
        break;
    }
    return FALSE;
}

static u8 GetNumBattleReadyPartyMons(BtlClient *client, u8 *list) {
    u8 num;
    u8 count = GetNumMonsInParty(client->party);
    u8 i = GetNumMonsOnField(BtlSetup_GetBattleStyle(client->mainModule), client->numCoverPos);

    num = 0;
    for (; i < count; i++) {
        if (CanPokemonBattle(GetBattleMonFromParty(client->party, i))) {
            if (list != NULL) {
                list[num] = i;
            }
            num++;
        }
    }
    return num;
}

static void PickBestMonToSwitchInto(BtlClient *client, u8 *list, u8 count, BattleMon *defender) {
    u16 power[6];
    PokeTypePair types;
    u8 i;
    BattleMon *mon;
    u8 moveCount;
    u8 j;
    u16 move;
    u8 moveType;
    u16 basePower;
    u16 tmpPower;
    u8 tmpIdx;

    types = GetPokeType(defender);
    for (i = 0; i < count; i++) {
        power[i] = 0;
        mon = GetBattleMonFromParty(client->party, list[i]);
        if (!IsFainted(mon)) {
            moveCount = GetBattleMonMoveCount(mon);
            for (j = 0; j < moveCount; j++) {
                if (GetMovePP(mon, j) != 0) {
                    move = MoveGetID(mon, j);
                    if (PML_MoveIsDamaging(move)) {
                        moveType = PML_MoveGetType(move);
                        basePower = PML_MoveGetBasePower(move);
                        if (basePower < 10) {
                            basePower = 60;
                        }
                        switch (func_ov167_021bd1b0(moveType, types)) {
                        case 5:
                            basePower *= 4;
                            break;
                        case 4:
                            basePower *= 2;
                            break;
                        case 2:
                            basePower /= 2;
                            break;
                        case 1:
                            basePower /= 4;
                            break;
                        case 0:
                            basePower = 0;
                            break;
                        }
                        if (power[i] < basePower) {
                            power[i] = basePower;
                        }
                    }
                }
            }
        }
    }

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (power[i] < power[j]) {
                tmpPower = power[i];
                power[i] = power[j];
                power[j] = tmpPower;
                tmpIdx = list[i];
                list[i] = list[j];
                list[j] = tmpIdx;
            }
        }
    }
}

static void func_ov167_021b5d7c(BtlClient *client, u8 mode, u8 count, BtlvPokeListCmd *cmd,
                                BtlvPokeSelectParam *select) {
    func_ov169_0689cc20(cmd, client->party, count, mode);
    func_ov169_0689cc38(cmd, client->numCoverPos);
    func_ov169_0689cc68(select, cmd);
}

static void func_ov167_021b5dd8(BtlClient *client, const BtlvPokeSelectParam *select) {
    u8 i;

    for (i = 0; i < select->unk00[6]; i++) {
        func_ov167_021bdbbc(&client->actions[i], select->unk00[3 + i], select->unk00[i]);
    }
    client->returnData = client->actions;
    client->returnDataSize = select->unk00[6] * sizeof(BattleAction);
}

static void func_ov167_021b5e1c(BtlClient *client) {
    u8 start;
    u8 num = 0;
    u8 partyCount = GetNumMonsInParty(client->party);
    u32 i;
    u32 j;
    u8 clientId;
    u8 idx;

    start = func_ov167_0219d29c(client->mainModule, client->clientId);
    for (i = 0; i < client->changePokeCount; i++) {
        func_ov167_0219c694(client->mainModule, client->changePokePos[i], &clientId, &idx);
        for (j = start; j < partyCount; j++) {
            if (!IsFainted(GetBattleMonFromParty(client->party, j))) {
                func_ov167_021bdbbc(&client->actions[num++], idx, j);
                start = j + 1;
                break;
            }
        }
    }
    client->returnData = client->actions;
    client->returnDataSize = num * sizeof(BattleAction);
}

// Collects the positions in the server's request that belong to this client
static u8 CollectOwnChangePositions(BtlClient *client, u8 *positions) {
    const u8 *data;
    u8 size = func_ov167_021d4c0c(client->adapter, (const void **)&data);
    u32 i;
    u32 count;

    for (i = 0, count = 0; i < size; i++) {
        if (client->clientId == func_ov167_0219c650(client->mainModule, data[i])) {
            positions[count++] = data[i];
        }
    }
    return count;
}

static BOOL func_ov167_021b5f30(BtlClient *client, s32 *seq) {
    u32 result;
    u8 yes;

    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 3, 5);
        Btlv_StringParam_Setup(&client->strParam2, 3, 1);
        func_ov167_021d0798(client->viewCore, &client->strParam, &client->strParam2, 0);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0828(client->viewCore, &result)) {
            yes = FALSE;
            if (result != 0) {
                yes = TRUE;
            }
            client->yesNoResult = yes;
            client->returnData = &client->yesNoResult;
            client->returnDataSize = 1;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b5fa4(BtlClient *client, s32 *seq) {
    return PlayerSelectChangePoke(client, seq, 2);
}

static BOOL func_ov167_021b5fb0(BtlClient *client, s32 *seq) {
    return PlayerSelectChangePoke(client, seq, 1);
}

// The AI's choice of the mons to send out
static BOOL AISelectChangePoke(BtlClient *client, s32 *seq) {
    u8 list[6];
    u8 idx;
    u8 clientId;
    u8 num;
    u8 count;
    u8 i;
    BattleMon *defender;
    u32 size;

    client->changePokeCount = CollectOwnChangePositions(client, client->changePokePos);
    if (client->changePokeCount != 0) {
        num = GetNumBattleReadyPartyMons(client, list);
        if (num != 0) {
            count = client->changePokeCount;
            defender = PickRandomOpponent(client, client->changePokePos[0]);
            if (defender != NULL) {
                PickBestMonToSwitchInto(client, list, num, defender);
            }
            if (count > num) {
                if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION) {
                    size = RotateToLivingMon(client, client->actions);
                    if (size != 0) {
                        client->returnDataSize = size;
                        client->returnData = client->actions;
                        return TRUE;
                    }
                }
                count = num;
            }
            for (i = 0; i < count; i++) {
                func_ov167_0219c694(client->mainModule, client->changePokePos[i], &clientId, &idx);
                func_ov167_021bdbbc(&client->actions[i], idx, list[i]);
            }
            client->returnData = client->actions;
            client->returnDataSize = count * sizeof(BattleAction);
        } else {
            if (IsRotationFrontFainted(client)) {
                size = RotateToLivingMon(client, client->actions);
                if (size != 0) {
                    client->returnDataSize = size;
                    client->returnData = client->actions;
                    return TRUE;
                }
            }
            func_ov167_021bdbec(client->actions);
            client->returnData = client->actions;
            client->returnDataSize = sizeof(BattleAction);
        }
    } else {
        BattleAction_SetNull(client->actions);
        client->returnData = client->actions;
        client->returnDataSize = sizeof(BattleAction);
    }
    return TRUE;
}

static BOOL func_ov167_021b60e0(BtlClient *client, s32 *seq) {
    return RecPlaySelectActionCore(client, seq, 0);
}

// The player's choice of the mons to send out
static BOOL PlayerSelectChangePoke(BtlClient *client, s32 *seq, u8 mode) {
    u8 list[6];
    u8 clientId;
    u8 idx;
    u8 num;
    u8 count;
    BattleMon *mon;
    u32 size;

    switch (*seq) {
    case 0:
        client->changePokeCount = CollectOwnChangePositions(client, client->changePokePos);
        if (client->changePokeCount != 0) {
            num = GetNumBattleReadyPartyMons(client, list);
            if (num != 0) {
                count = client->changePokeCount;
                if (count > num) {
                    count = num;
                }
                if (IsRotationFrontFainted(client)) {
                    mon = GetBattleMonFromParty(client->party, 0);
                    if (num == 1 && IsFainted(mon)) {
                        func_ov167_021bdbbc(&client->actions[0], 0, list[0]);
                        client->returnData = client->actions;
                        client->returnDataSize = sizeof(BattleAction);
                        *seq = 4;
                        break;
                    }
                }
                func_ov167_0219c694(client->mainModule, client->changePokePos[0], &clientId, &idx);
                func_ov167_021b5d7c(client, mode, count, &client->pokeListCmd, &client->pokeSelect);
                BattleClientCmd_StartPokeList(client->viewCore, &client->pokeListCmd, idx, 0, &client->pokeSelect);
                func_ov167_021b2080(client);
                *seq = 1;
            } else {
                if (IsRotationFrontFainted(client)) {
                    size = RotateToLivingMon(client, client->actions);
                    if (size != 0) {
                        client->returnDataSize = size;
                        client->returnData = client->actions;
                        *seq = 4;
                        break;
                    }
                }
                func_ov167_021bdbec(client->actions);
                client->returnData = client->actions;
                client->returnDataSize = sizeof(BattleAction);
                *seq = 4;
            }
        } else {
            BattleAction_SetNull(client->actions);
            client->returnData = client->actions;
            client->returnDataSize = sizeof(BattleAction);
            *seq = 4;
        }
        break;
    case 1:
        if (CheckIfOverCmdLimit(client)) {
            BattleClientCmd_QuitPokeSelect(client->viewCore);
            *seq = 2;
        } else if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            func_ov167_021b5dd8(client, &client->pokeSelect);
            *seq = 3;
        }
        break;
    case 2:
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            func_ov167_021b5e1c(client);
            *seq = 3;
        }
        break;
    case 3:
        func_ov167_021b20f8(client);
        *seq = 4;
        break;
    case 4:
        if (func_ov167_0219bee4(client->mainModule)) {
            client->waitMsgShown = TRUE;
            func_ov167_021d03d4(client->viewCore);
            *seq = 5;
        } else {
            *seq = 6;
        }
        break;
    case 5:
        if (func_ov167_021d03ec(client->viewCore)) {
            *seq = 6;
        }
        break;
    case 6:
        return TRUE;
    }
    return FALSE;
}

static BOOL IsRotationFrontFainted(BtlClient *client) {
    if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION &&
        IsFainted(GetBattleMonFromParty(client->party, 0))) {
        return TRUE;
    }
    return FALSE;
}

// Rotates to the first position whose mon can fight, returning the action's size, or 0 when none can
static u32 RotateToLivingMon(BtlClient *client, BattleAction *action) {
    if (!IsFainted(GetPartyMonAt(client, func_ov167_0219d38c(2)))) {
        func_ov167_021bdc24(action, 2);
        return sizeof(BattleAction);
    }
    if (!IsFainted(GetPartyMonAt(client, func_ov167_0219d38c(3)))) {
        func_ov167_021bdc24(action, 3);
        return sizeof(BattleAction);
    }
    return 0;
}

static BOOL func_ov167_021b6328(BtlClient *client, s32 *seq) {
    const u8 *data;
    u32 result;
    u8 partyIdx;

    switch (*seq) {
    case 0:
        func_ov167_021d4c0c(client->adapter, (const void **)&data);
        if (*data != 0x1f) {
            Btlv_StringParam_Setup(&client->strParam, 1, 20);
            Btlv_StringParam_AddArg(&client->strParam, 1);
            Btlv_StringParam_AddArg(&client->strParam, *data);
            func_ov167_021d0b4c(&client->strParam, 0xff);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            *seq = 1;
        } else {
            *seq = 4;
        }
        break;
    case 1:
        func_ov167_021d02e8(client->viewCore);
        if (func_ov167_021d02f8(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 3, 6);
            Btlv_StringParam_Setup(&client->strParam2, 3, 7);
            func_ov167_021d0798(client->viewCore, &client->strParam, &client->strParam2, 1);
            *seq = 2;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore) && func_ov167_021d0828(client->viewCore, &result)) {
            if (result == 0) {
                func_ov167_021b5d7c(client, 0, 1, &client->pokeListCmd, &client->pokeSelect);
                BattleClientCmd_StartPokeList(client->viewCore, &client->pokeListCmd, 0, 0, &client->pokeSelect);
                *seq = 3;
            } else {
                *seq = 4;
            }
        }
        break;
    case 3:
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            if (!func_ov169_0689cc9c(&client->pokeSelect) &&
                (partyIdx = func_ov169_0689cca4(&client->pokeSelect)) != 0 && partyIdx < 6) {
                func_ov167_021bdbbc(client->actions, 0, partyIdx);
                *seq = 5;
            } else {
                *seq = 4;
            }
        }
        break;
    case 4:
        BattleAction_SetNull(client->actions);
        *seq = 5;
        // fallthrough
    case 5:
        client->returnData = client->actions;
        client->returnDataSize = sizeof(BattleAction);
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b64ac(BtlClient *client, s32 *seq) {
    const void *data;
    u16 size = func_ov167_021d4c0c(client->adapter, &data);

    if (client->recorder != NULL) {
        func_ov167_021d45f0(client->recorder, data, size);
    }
    if (client->cmdCheckServer != NULL) {
        func_ov167_0219eee4(client->cmdCheckServer, data, size);
        client->cmdCheckReq = func_ov167_021d4624(data);
    }
    return TRUE;
}

static BOOL func_ov167_021b64ec(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return func_ov167_021b6500(client, seq);
    }
    return TRUE;
}

static BOOL func_ov167_021b6500(BtlClient *client, s32 *seq) {
    u32 name;
    u32 result;
    u8 isMulti;
    u16 msg;

    switch (*seq) {
    case 0:
        if (client->unk1BA_5) {
            return TRUE;
        }
        name = func_ov167_0219c8b8(client->mainModule, 0);
        result = GetBattleResultForClient(client);
        isMulti = func_ov167_0219beec(client->mainModule);
        func_ov167_0219ca48(client->mainModule, result);
        switch (result) {
        case 1:
            msg = isMulti ? 0x31 : 0x30;
            break;
        case 0:
            msg = isMulti ? 0x33 : 0x32;
            break;
        case 2:
            msg = isMulti ? 0x35 : 0x34;
            break;
        default:
            return TRUE;
        }
        Btlv_StringParam_Setup(&client->strParam, 1, msg);
        Btlv_StringParam_AddArg(&client->strParam, name);
        if (isMulti) {
            Btlv_StringParam_AddArg(&client->strParam, func_ov167_0219c8b8(client->mainModule, 1));
        }
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        if (result == 1) {
            GFL_SndBGMPlay(func_ov167_0219bf00(client->mainModule), 0xffff);
            (*seq)++;
        } else {
            GFL_SndBGMFadeOut(30);
            *seq += 2;
        }
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            (*seq)++;
        }
        break;
    case 3:
        if (!GFL_SndBGMIsFading()) {
            func_02005d8c();
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The battle's result for this client's side: a result for the other side is flipped
static u32 GetBattleResultForClient(BtlClient *client) {
    const u16 *data;

    func_ov167_021d4c0c(client->adapter, (const void **)&data);
    switch (data[1]) {
    case 0:
        if (!IsAllyClientID(data[0], client->clientId)) {
            return 1;
        }
        break;
    case 1:
        if (!IsAllyClientID(data[0], client->clientId)) {
            return 0;
        }
        break;
    }
    return data[1];
}

static BOOL func_ov167_021b666c(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return func_ov167_021b6680(client, seq);
    }
    return TRUE;
}

static BOOL func_ov167_021b6680(BtlClient *client, s32 *seq) {
    u32 name1;
    u32 name2;
    u32 result;
    u32 value;
    BtlSetup *setup;
    BtlScriptedRules *rules;
    u32 score;

    switch (*seq) {
    case 0:
        if (func_ov167_0219c988(client->mainModule) == 0) {
            if (GetBattleResultForClient(client) == 1) {
                BtlvEffect_PlayBgm(func_ov167_0219bf00(client->mainModule));
                name1 = func_ov167_0219c8b8(client->mainModule, 0);
                name2 = func_ov167_0219c8b8(client->mainModule, 1);
                if (name1 == name2) {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x2c);
                    Btlv_StringParam_AddArg(&client->strParam, name1);
                } else {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x2d);
                    Btlv_StringParam_AddArg(&client->strParam, name1);
                    Btlv_StringParam_AddArg(&client->strParam, name2);
                }
                func_ov167_021d01ec(client->viewCore, &client->strParam);
                *seq = 1;
            } else if (!BtlSetup_IsBattleType(client->mainModule, 0x100)) {
                name1 = GetPlayerClientID(client->mainModule);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x36);
                Btlv_StringParam_AddArg(&client->strParam, name1);
                Btlv_StringParam_AddArg(&client->strParam, name1);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
                *seq = 9;
            } else {
                return TRUE;
            }
        } else if (func_ov167_0219c988(client->mainModule) == 2) {
            result = GetBattleResultForClient(client);
            func_ov167_021b9774(client, &client->studio.score);
            if (result == 1) {
                client->studio.seq = 0;
                client->turnCount = 100;
            } else {
                client->studio.seq = 0;
                client->turnCount = 101;
            }
            *seq = 11;
            func_ov167_021b9910(client->mainModule);
        } else if (func_ov167_0219c988(client->mainModule) == 1) {
            func_ov167_021b9774(client, &client->studio.score);
            client->studio.seq = 0;
            *seq = 12;
        }
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 0)), 9,
                                0, 0, 0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            *seq = 2;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d0288(client->viewCore,
                                func_ov167_0219d91c(client->mainModule, func_ov167_0219c8b8(client->mainModule, 0)), 1);
            *seq = 3;
        }
        break;
    case 3:
        if (func_ov167_021d02e8(client->viewCore)) {
            name1 = func_ov167_0219c8b8(client->mainModule, 0);
            if (name1 != func_ov167_0219c8b8(client->mainModule, 1)) {
                BtlvEffect_Start(0x271);
                *seq = 4;
            } else {
                *seq = 7;
            }
        }
        break;
    case 4:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 9,
                                0, 0, 0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            *seq = 5;
        }
        break;
    case 5:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d0288(client->viewCore,
                                func_ov167_0219d91c(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 1);
            *seq = 6;
        }
        break;
    case 6:
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 7;
        }
        break;
    case 7:
        value = func_ov167_0219ca78(client->mainModule);
        if (value != 0) {
            name1 = GetPlayerClientID(client->mainModule);
            Btlv_StringParam_Setup(&client->strParam, 1, 0x3a);
            Btlv_StringParam_AddArg(&client->strParam, name1);
            Btlv_StringParam_AddArg(&client->strParam, value);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
        }
        if (!func_ov167_0219caec(client->mainModule)) {
            *seq = 13;
        } else {
            *seq = 8;
        }
        break;
    case 8:
        if (func_ov167_021d02e8(client->viewCore)) {
            value = func_ov167_0219caec(client->mainModule);
            if (value != 0) {
                name1 = GetPlayerClientID(client->mainModule);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x3b);
                Btlv_StringParam_AddArg(&client->strParam, name1);
                Btlv_StringParam_AddArg(&client->strParam, value);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
            }
            *seq = 13;
        }
        break;
    case 9:
        if (func_ov167_021d02e8(client->viewCore)) {
            value = func_ov167_0219cb20(client->mainModule);
            if (value != 0) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0x38);
                Btlv_StringParam_AddArg(&client->strParam, client->clientId);
                Btlv_StringParam_AddArg(&client->strParam, value);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
            }
            *seq = 10;
        }
        break;
    case 10:
        if (func_ov167_021d02e8(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0x39);
            Btlv_StringParam_AddArg(&client->strParam, client->clientId);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            *seq = 13;
        }
        break;
    case 11:
        if (func_ov167_021b2864(client)) {
            setup = func_ov167_0219e30c(client->mainModule);
            setup->unk130 = StudioScore_CalcTotal(client, &client->studio.score);
            return TRUE;
        }
        break;
    case 12:
        if (func_ov167_021b3100(client)) {
            setup = func_ov167_0219e30c(client->mainModule);
            rules = func_ov167_0219e39c(client->mainModule);
            score =
                func_ov167_021b2538(rules, &client->studio, client->turnCount, func_ov167_0219c9e0(client->mainModule));
            func_ov167_021b2748(client, rules, score);
            func_ov167_021b2700(client, rules, score);
            setup->unk130 = StudioScore_CalcTotal(client, &client->studio.score);
            return TRUE;
        }
        break;
    case 13:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b6a98(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return func_ov167_021b6aac(client, seq);
    }
    return TRUE;
}

static BOOL func_ov167_021b6aac(BtlClient *client, s32 *seq) {
    u32 result;

    switch (*seq) {
    case 0:
        if (client->unk1BA_5) {
            return TRUE;
        }
        result = GetBattleResultForClient(client);
        func_ov167_0219ca48(client->mainModule, result);
        if (result == 1) {
            BtlvEffect_PlayBgm(func_ov167_0219bf00(client->mainModule));
        }
        if (result <= 1) {
            BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 0)), 9,
                                0, 0, 0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            ShowTrainerWinLoseMsg(client, result, 0);
            *seq = 1;
            break;
        }
        return TRUE;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d01d8(client->viewCore);
            *seq = 2;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            if (func_ov167_0219c8b8(client->mainModule, 0) == func_ov167_0219c8b8(client->mainModule, 1)) {
                return TRUE;
            }
            BtlvEffect_Start(0x271);
            *seq = 3;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            result = GetBattleResultForClient(client);
            BtlvEffect_SetTrainer(func_ov167_0219d938(client->mainModule, func_ov167_0219c8b8(client->mainModule, 1)), 9,
                                0, 0, 0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            ShowTrainerWinLoseMsg(client, result, 1);
            *seq = 4;
        }
        break;
    case 4:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d01d8(client->viewCore);
            *seq = 5;
        }
        break;
    case 5:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Shows a trainer's win or lose message: their easy chat sentence if they have one, else their trainer data's message
static void ShowTrainerWinLoseMsg(BtlClient *client, u32 result, u8 which) {
    u8 clientId = func_ov167_0219c8b8(client->mainModule, which);
    const PMSData *sentence = func_ov167_0219d944(client->mainModule, clientId, result);
    StrBuf *str;
    u16 trainerId;

    if (sentence != NULL && PMSData_IsNotEmpty(sentence)) {
        str = PMSData_ToString(sentence, HEAPID_TAIL(client->heapId));
        func_ov167_021d01bc(client->viewCore, str);
        GFL_StrBufFree(str);
    } else {
        trainerId = func_ov167_0219d91c(client->mainModule, clientId);
        if (trainerId != 0) {
            Btlv_StringParam_Setup(&client->strParam, 7, 0);
            Btlv_StringParam_AddArg(&client->strParam, trainerId);
            Btlv_StringParam_AddArg(&client->strParam, result);
            func_ov167_021d01c8(client->viewCore, &client->strParam);
        }
    }
}

static BOOL func_ov167_021b6c98(BtlClient *client, s32 *seq) {
    u32 value;
    u8 playerId;

    switch (*seq) {
    case 0:
        if (!BtlSetup_IsBattleType(client->mainModule, 0x200)) {
            value = func_ov167_0219caec(client->mainModule);
            if (value != 0) {
                playerId = GetPlayerClientID(client->mainModule);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x3b);
                Btlv_StringParam_AddArg(&client->strParam, playerId);
                Btlv_StringParam_AddArg(&client->strParam, value);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
            }
        }
        (*seq)++;
        // fallthrough
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b6d0c(BtlClient *client, s32 *seq) {
    u32 value;

    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 1, 0x36);
        Btlv_StringParam_AddArg(&client->strParam, client->clientId);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            value = func_ov167_0219cb20(client->mainModule);
            if (value != 0) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0x37);
                Btlv_StringParam_AddArg(&client->strParam, client->clientId);
                Btlv_StringParam_AddArg(&client->strParam, value);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
            }
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0x39);
            Btlv_StringParam_AddArg(&client->strParam, client->clientId);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d02e8(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

typedef BOOL (*BtlClientServerCmdProc)(BtlClient *client, s32 *seq, const u32 *args);

// A server command and the handler that plays it
typedef struct {
    u32 cmd;
    BtlClientServerCmdProc proc;
} BtlClientServerCmdEntry;

// This part's functions

// The handlers of the server's commands
static const BtlClientServerCmdEntry sServerCmdTable[] = {
    { 0x5a, func_ov167_021b7230 },       { 0x5c, func_ov167_021b7278 },   { 0x5b, func_ov167_021b72d4 },
    { 0x5d, func_ov167_021b731c },       { 0x59, func_ov167_021b7378 },   { 0x30, func_ov167_021b73f4 },
    { 0x31, func_ov167_021b7480 },       { 0x32, func_ov167_021b74d0 },   { 0x33, func_ov167_021b7520 },
    { 0x34, func_ov167_021b7598 },       { 0x35, func_ov167_021b75f8 },   { 0x36, func_ov167_021b763c },
    { 0x39, func_ov167_021b76e4 },       { 0x3a, func_ov167_021b7750 },   { 0x3b, func_ov167_021b6f7c },
    { 0x3c, func_ov167_021b7058 },       { 0x3d, func_ov167_021b70a0 },   { 0x37, func_ov167_021b7800 },
    { 0x38, func_ov167_021b7794 },       { 0x3e, BattleClient_ScAddEVs }, { 0x3f, BattleClient_ScWeatherStart },
    { 0x40, BattleClient_ScWeatherEnd }, { 0x41, func_ov167_021b795c },   { 0x42, func_ov167_021b79a0 },
    { 0x57, func_ov167_021b8a34 },       { 0x58, func_ov167_021b8a8c },   { 0x01, func_ov167_021b8ad0 },
    { 0x02, func_ov167_021b8af0 },       { 0x03, func_ov167_021b8b80 },   { 0x04, func_ov167_021b8b10 },
    { 0x05, func_ov167_021b8b38 },       { 0x06, func_ov167_021b8b60 },   { 0x07, func_ov167_021b8b98 },
    { 0x08, func_ov167_021b8bbc },       { 0x09, func_ov167_021b8be0 },   { 0x0a, func_ov167_021b8c00 },
    { 0x0b, func_ov167_021b8c20 },       { 0x0c, func_ov167_021b8c98 },   { 0x0d, func_ov167_021b8cb0 },
    { 0x0e, func_ov167_021b8cc8 },       { 0x0f, func_ov167_021b8ce4 },   { 0x10, func_ov167_021b8d04 },
    { 0x11, func_ov167_021b8d1c },       { 0x12, func_ov167_021b8d38 },   { 0x13, func_ov167_021b9130 },
    { 0x14, func_ov167_021b9154 },       { 0x16, func_ov167_021b8d8c },   { 0x15, func_ov167_021b8dac },
    { 0x17, func_ov167_021b8e24 },       { 0x18, func_ov167_021b8e44 },   { 0x19, func_ov167_021b8e80 },
    { 0x1a, func_ov167_021b8e9c },       { 0x1b, func_ov167_021b8eb8 },   { 0x1c, func_ov167_021b8ed4 },
    { 0x1d, func_ov167_021b8ef0 },       { 0x1e, func_ov167_021b8f0c },   { 0x1f, func_ov167_021b8f2c },
    { 0x20, func_ov167_021b8f60 },       { 0x21, func_ov167_021b8f78 },   { 0x22, func_ov167_021b8f8c },
    { 0x23, func_ov167_021b8fa0 },       { 0x24, func_ov167_021b8fb4 },   { 0x25, func_ov167_021b8fc4 },
    { 0x26, func_ov167_021b8fe4 },       { 0x27, func_ov167_021b9010 },   { 0x28, func_ov167_021b9030 },
    { 0x29, func_ov167_021b9048 },       { 0x2a, func_ov167_021b9074 },   { 0x2b, func_ov167_021b9094 },
    { 0x2c, func_ov167_021b90ac },       { 0x2d, func_ov167_021b90cc },   { 0x2e, func_ov167_021b9100 },
    { 0x2f, func_ov167_021b9120 },       { 0x43, func_ov167_021b7a04 },   { 0x44, func_ov167_021b7a60 },
    { 0x45, BattleClient_ScExpGain },    { 0x46, func_ov167_021b83dc },   { 0x47, func_ov167_021b853c },
    { 0x48, func_ov167_021b85a4 },       { 0x49, func_ov167_021b8650 },   { 0x4a, func_ov167_021b86e4 },
    { 0x4b, func_ov167_021b8850 },       { 0x4c, func_ov167_021b88b4 },   { 0x4d, func_ov167_021b88ec },
    { 0x4e, func_ov167_021b896c },       { 0x4f, func_ov167_021b89cc },   { 0x50, func_ov167_021b7af4 },
    { 0x51, func_ov167_021b7c08 },       { 0x52, func_ov167_021b7c40 },   { 0x53, BattleClient_ScTransform },
    { 0x54, func_ov167_021b7d24 },       { 0x55, func_ov167_021b7d84 },   { 0x56, func_ov167_021b7d90 },
};

// The message and sound effect of each weather's start
static const struct {
    u16 msg;
    u16 se;
} sWeatherStartTable[] = {
    { 0x54, 0x26b }, { 0x54, 0x26b }, { 0x55, 0x268 }, { 0x57, 0x269 }, { 0x56, 0x26a },
};

// The effort values that the effort value command raises, in its arguments' order
static BOOL func_ov167_021b6dc4(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 1, 0xb7);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            (*seq)++;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b6e0c(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return BattleClient_ServerCmdLoop(client, seq);
    }
    return TRUE;
}

static BOOL BattleClient_ServerCmdLoop(BtlClient *client, s32 *seq) {
    while (TRUE) {
        switch (*seq) {
        case 0: {
            const void *data;
            u32 size;

            size = func_ov167_021d4c0c(client->adapter, &data);
            if (client->cmdCheckServer != NULL && client->cmdCheckReq && client->cmdCheckEnable) {
                if (func_ov167_0219ef74(client->cmdCheckServer, client->cmdCheckReq, data, size)) {
                    func_ov167_0219ca60(client->mainModule);
                    client->cmdCheckEnable = FALSE;
                }
                client->cmdCheckReq = 0;
            }
            BtlServerCmdQueue_Setup(client->cmdQueue, data, size);
            if (client->waitMsgShown) {
                client->waitMsgShown = FALSE;
                func_ov167_021d03fc(client->viewCore);
            }
            (*seq)++;
        }
        case 1:
            if (BtlServerCmdQueue_IsEmpty(client->cmdQueue)) {
                *seq = 4;
                return TRUE;
            }
            if (func_ov167_021b9360(&client->recPlayer)) {
                return TRUE;
            }
            (*seq)++;
        case 2: {
            u32 i;

            client->serverCmd = func_ov167_021b1564(client->cmdQueue, client->cmdArgs);
            if (client->serverCmd == 0x5e) {
                return TRUE;
            }
            for (i = 0; i < NELEMS(sServerCmdTable); i++) {
                if (client->serverCmd == sServerCmdTable[i].cmd) {
                    break;
                }
            }
            if (i == NELEMS(sServerCmdTable)) {
                *seq = 1;
                continue;
            }
            client->serverCmdProc = sServerCmdTable[i].proc;
            client->serverCmdSeq = 0;
            (*seq)++;
        }
        case 3:
            if (client->serverCmdProc(client, &client->serverCmdSeq, client->cmdArgs)) {
                *seq = 1;
                continue;
            }
            break;
        case 4:
            return TRUE;
        }
        return FALSE;
    }
}

static BOOL func_ov167_021b6f7c(BtlClient *client, s32 *seq, const u32 *args) {
    BOOL withName;

    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 1, BattleClient_GetRecallMsg(client, args[0], &withName));
        if (withName) {
            Btlv_StringParam_AddArg(&client->strParam, args[0]);
        }
        Btlv_StringParam_AddArg(&client->strParam, args[1]);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static u16 BattleClient_GetRecallMsg(BtlClient *client, u8 clientId, BOOL *withName) {
    if (clientId == client->clientId) {
        *withName = FALSE;
        if (BtlSetup_GetBattleStyle(client->mainModule) == 0 && !func_ov167_0219bee4(client->mainModule)) {
            u32 ratio = func_ov167_021b1f00(client);

            if (ratio >= 75) {
                return 0x1d;
            }
            if (ratio > 50) {
                return 0x1c;
            }
            if (ratio > 25) {
                return 0x1b;
            }
            if (ratio != 0) {
                return 0x1a;
            }
            return 0x19;
        }
        return 0x1a;
    } else {
        *withName = TRUE;
        if (func_ov167_0219d888(client->mainModule, clientId)) {
            return 0x1e;
        }
        return 0x1f;
    }
}

static BOOL func_ov167_021b7058(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021d0018(client->viewCore, func_ov167_0219c6dc(client->mainModule, args[0]), args[1]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0038(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b70a0(BtlClient *client, s32 *seq, const u32 *args) {
    u8 clientId = args[0];
    u8 slot = args[1];
    u8 showMsg = args[3];

    switch (*seq) {
    case 0:
        if (showMsg) {
            u8 monId = GetMonID(GetClientMonData(client->pokeCon, clientId, slot));

            if (!AreClientsOnOppositeSides(client->mainModule, client->clientId, clientId)) {
                if (client->clientId == clientId) {
                    Btlv_StringParam_Setup(&client->strParam, 1, BattleClient_GetPlayerSendOutMsg(client));
                    Btlv_StringParam_AddArg(&client->strParam, monId);
                    func_ov167_021d01ec(client->viewCore, &client->strParam);
                } else {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x11);
                    Btlv_StringParam_AddArg(&client->strParam, clientId);
                    Btlv_StringParam_AddArg(&client->strParam, monId);
                    func_ov167_021d01ec(client->viewCore, &client->strParam);
                }
            } else if (func_ov167_0219d888(client->mainModule, clientId)) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0xe);
                Btlv_StringParam_AddArg(&client->strParam, clientId);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
            } else {
                Btlv_StringParam_Setup(&client->strParam, 1, 0x11);
                Btlv_StringParam_AddArg(&client->strParam, clientId);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
            }
        }
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            func_ov167_021d0048(client->viewCore, func_ov167_0219c458(client->mainModule, clientId, slot), clientId,
                                slot);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d0084(client->viewCore)) {
            func_ov167_021b1ee4(client);
            if (clientId == GetPlayerClientID(client->mainModule)) {
                func_ov167_021b96f0(client, &client->studio, client->party);
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static u16 BattleClient_GetPlayerSendOutMsg(BtlClient *client) {
    if (BtlSetup_GetBattleStyle(client->mainModule) == 0) {
        u8 clientId = func_ov167_0219c8d0(client->mainModule, client->clientId, 0);
        BattleMon *mon = GetClientMonData(client->pokeCon, clientId, 0);
        fx32 ratio;

        if (IsFainted(mon)) {
            return 0xb;
        }
        ratio = GetHPRatio(mon);
        if (ratio >= FX32_CONST(77.5)) {
            return 0xb;
        }
        if (ratio >= FX32_CONST(55)) {
            return 0x15;
        }
        if (ratio >= FX32_CONST(32.5)) {
            return 0x16;
        }
        if (ratio >= FX32_CONST(10)) {
            return 0x17;
        }
        return 0x18;
    }
    return 0xb;
}

static BOOL func_ov167_021b7230(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        func_ov167_021d0250(client->viewCore, args[0], &args[1]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7278(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        func_ov167_021d0250(client->viewCore, args[0], &args[2]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02f8(client->viewCore)) {
            GFL_SndSEPlay(args[1]);
        }
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b72d4(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        func_ov167_021d026c(client->viewCore, args[0], &args[1]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b731c(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        func_ov167_021d026c(client->viewCore, args[0], &args[2]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02f8(client->viewCore)) {
            GFL_SndSEPlay(args[1]);
        }
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7378(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        func_ov167_021d02cc(client->viewCore, args[0], args[1]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static s32 BattleClient_GetMoveAnimType(BtlClient *client, u16 move, u8 pos) {
    if (move != 0xae) {
        return PML_MoveGetParam(move, 0x1b);
    } else {
        BattleMon *mon = func_ov167_0219d188(client->pokeCon, pos);

        if (mon != NULL) {
            return func_ov167_021bd8d0(mon);
        }
        return PML_MoveGetParam(move, 0x1b);
    }
}

static BOOL func_ov167_021b73f4(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        if (func_ov167_0219bd88(client->mainModule)) {
            u16 move;
            u8 attackerPos;
            u8 targetPos;
            u8 arg3;
            s32 arg4;

            attackerPos = args[0];
            targetPos = args[1];
            move = args[2];
            arg3 = args[3];
            arg4 = BattleClient_GetMoveAnimType(client, move, attackerPos);
            func_ov167_021cfc60(client->viewCore, attackerPos, targetPos, move, arg4, arg3, 0);
            (*seq)++;
            break;
        }
        return TRUE;
    case 1:
        if (func_ov167_021cfca4(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7480(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 pos = func_ov167_0219c62c(client->mainModule, client->pokeCon, args[0]);

        func_ov167_021cff14(client->viewCore, pos, args[1] ? TRUE : FALSE);
        (*seq)++;
        break;
    }
    case 1:
        return !BtlvEffect_IsBusy();
    }
    return FALSE;
}

static BOOL func_ov167_021b74d0(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021cfcb4(client->viewCore, args[2], MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]),
                            args[1]);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cfce0(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7520(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u16 count = args[0];
        u32 type = args[1];
        u16 arg2 = args[2];
        u8 monIds[6];
        u8 i;

        for (i = 0; i < count; i++) {
            monIds[i] = func_ov167_021b15c8(client->cmdQueue);
        }
        func_ov167_021cfe40(client->viewCore, count, type, monIds, arg2);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021cfe7c(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7598(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021cff44(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cff60(client->viewCore)) {
            func_ov167_021d0250(client->viewCore, 0x61, args);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b75f8(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_SetGaugeStatus(
            args[1],
            func_ov167_0219c6dc(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0])));
        (*seq)++;
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b763c(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos;

    switch (*seq) {
    case 0:
        if (!func_ov167_021b1990(client)) {
            pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);
            func_ov167_021cfc60(client->viewCore, pos, pos, 1, PML_MoveGetParam(1, 0x1b), 0, 0);
        }
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cfca4(client->viewCore)) {
            pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);
            if (!func_ov167_021b1990(client)) {
                GFL_SndSEPlay(0x564);
            }
            func_ov167_021cfcb4(client->viewCore, 1, pos, 1);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021cfce0(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b76e4(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021cffcc(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        client->studio.unk26 = GetBattleMonSpecies(GetPokeParamConst(client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cffe8(client->viewCore)) {
            Clear_ForFainted(GetPokeParam(client->pokeCon, args[0]));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7750(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021cfff8(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0008(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7794(BtlClient *client, s32 *seq, const u32 *args) {
    u8 viewPos =
        func_ov167_0219c6dc(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));

    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        if (!func_ov167_0219bd88(client->mainModule)) {
            return TRUE;
        }
        func_ov167_021d03a0(client->viewCore, viewPos);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d03c0(client->viewCore, viewPos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7800(BtlClient *client, s32 *seq, const u32 *args) {
    u8 viewPos =
        func_ov167_0219c6dc(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));

    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        if (!func_ov167_0219bd88(client->mainModule)) {
            return TRUE;
        }
        func_ov167_021d03b0(client->viewCore, viewPos);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d03c0(client->viewCore, viewPos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScWeatherStart(BtlClient *client, s32 *seq, const u32 *args) {
    u8 weather = args[0];

    switch (*seq) {
    case 0:
        func_ov167_021d5aec(client->field, weather, args[1]);
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        if (weather < NELEMS(sWeatherStartTable)) {
            BtlvEffect_Start(sWeatherStartTable[weather].se);
        }
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d0250(client->viewCore, sWeatherStartTable[weather].msg, NULL);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScWeatherEnd(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u16 msg;

        switch ((u8)args[0]) {
        case 1:
            msg = 0x59;
            break;
        case 2:
            msg = 0x5a;
            break;
        case 4:
            msg = 0x5b;
            break;
        case 3:
            msg = 0x5c;
            break;
        default:
            return TRUE;
        }
        func_ov167_021d0250(client->viewCore, msg, NULL);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d02e8(client->viewCore)) {
            func_ov167_021d5af4(client->field);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b795c(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021cff44(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cff60(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b79a0(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        if (!func_ov167_0219bd88(client->mainModule)) {
            return TRUE;
        }
        func_ov167_021d0410(client->viewCore, pos);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0428(client->viewCore, pos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7a04(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monId = args[0];

    switch (*seq) {
    case 0:
        func_ov167_021cff44(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, monId));
        client->studio.unk26 = GetBattleMonSpecies(GetPokeParamConst(client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021cff60(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7a60(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 clientId = args[0];
        u8 pos1 = args[1];
        u8 pos2 = args[2];
        u8 slot1 = func_ov167_0219c658(client->mainModule, pos1);
        u8 slot2 = func_ov167_0219c658(client->mainModule, pos2);
        u8 viewPos1 = func_ov167_0219c6dc(client->mainModule, pos1);
        u8 viewPos2 = func_ov167_0219c6dc(client->mainModule, pos2);

        func_ov167_0219d504(GetPartyData(client->pokeCon, clientId), slot1, slot2);
        func_ov167_021d04dc(client->viewCore, clientId, viewPos1, viewPos2, slot1, slot2);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d0528(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7af4(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_Start(0x281);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            u8 clientId = args[0];
            u8 slot = args[1];
            u8 pos1 = func_ov167_0219c458(client->mainModule, clientId, slot);
            u8 pos2 = func_ov167_0219c458(client->mainModule, clientId, 1);
            u8 viewPos1 = func_ov167_0219c6dc(client->mainModule, pos1);
            u8 viewPos2 = func_ov167_0219c6dc(client->mainModule, pos2);

            func_ov167_0219d504(GetPartyData(client->pokeCon, clientId), slot, 1);
            func_ov167_021d04dc(client->viewCore, clientId, viewPos1, viewPos2, slot, 1);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d0528(client->viewCore)) {
            u8 clientId = args[2];
            u8 slot = args[3];
            u8 pos1 = func_ov167_0219c458(client->mainModule, clientId, slot);
            u8 pos2 = func_ov167_0219c458(client->mainModule, clientId, 1);
            u8 viewPos1 = func_ov167_0219c6dc(client->mainModule, pos1);
            u8 viewPos2 = func_ov167_0219c6dc(client->mainModule, pos2);

            func_ov167_0219d504(GetPartyData(client->pokeCon, clientId), slot, 1);
            func_ov167_021d04dc(client->viewCore, clientId, viewPos1, viewPos2, slot, 1);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d0528(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7c08(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_StartEffect26E(func_ov167_0219c6dc(client->mainModule, args[0]));
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7c40(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 viewPos = func_ov167_0219c6dc(client->mainModule, args[0]);

        if (func_ov167_021b1990(client)) {
            BtlvEffect_SetFlag26(TRUE);
        }
        BtlvEffect_StartEffect26F(viewPos);
        (*seq)++;
        break;
    }
    case 1:
        if (!BtlvEffect_IsBusy()) {
            if (func_ov167_021b1990(client)) {
                BtlvEffect_SetFlag26(FALSE);
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScTransform(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);
        BattleMon *target = GetPokeParam(client->pokeCon, args[1]);
        u8 pos = func_ov167_0219c62c(client->mainModule, client->pokeCon, args[0]);
        u8 targetPos = func_ov167_0219c62c(client->mainModule, client->pokeCon, args[1]);

        TransformSet(mon, target);
        func_ov167_021d048c(client->viewCore, pos, targetPos);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d04ac(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7d24(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (!func_ov167_021b1990(client)) {
            u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

            func_ov167_021cfd78(client->viewCore, args[2], pos, func_ov167_021bd2e8(args[1]));
            (*seq)++;
            break;
        }
        return TRUE;
    case 1:
        if (func_ov167_021cfdb0(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b7d84(BtlClient *client, s32 *seq, const u32 *args) {
    BtlvEffect_PlayBgm(args[0]);
    return TRUE;
}

static BOOL func_ov167_021b7d90(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021d04bc(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d04cc(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScAddEVs(BtlClient *client, s32 *seq, const u32 *args) {
    static const u16 sEVParams[] = {
        PKM_PARAM_EV_HP,     PKM_PARAM_EV_HP + 1, PKM_PARAM_EV_HP + 2,
        PKM_PARAM_EV_HP + 3, PKM_PARAM_EV_HP + 4, PKM_PARAM_EV_HP + 5,
    };
    PartyPkm *pkm = GetSrcData(GetPokeParam(client->pokeCon, args[0]));
    u32 i;

    for (i = 0; i < NELEMS(sEVParams); i++) {
        if (args[1 + i] != 0) {
            u16 param = sEVParams[i];
            u32 value = args[1 + i] + PokeParty_GetParam(pkm, param, NULL);

            if (value > 255) {
                value = 255;
            }
            PokeParty_SetParam(pkm, param, value);
        }
    }
    return TRUE;
}

static BOOL BattleClient_ScExpGain(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monId = args[0];
    BattleMon *mon = GetPokeParam(client->pokeCon, monId);
    u8 pos = func_ov167_0219c62c(client->mainModule, client->pokeCon, monId);
    BOOL visible = FALSE;

    if (pos != 0xff && BtlvEffect_CheckGaugeExist(pos)) {
        visible = TRUE;
    }

    switch (*seq) {
    case 0:
        sExpLeft = args[1];
        sExpSeq = 0;
        *seq = 1;
    case 1:
        if (sExpLeft != 0) {
            u32 exp = sExpLeft;

            if (func_ov167_021bc1b8(mon, &exp, &sLevelUp)) {
                *seq = 3;
            } else {
                if (visible) {
                    BtlvEffect_CalcGaugeExp(pos, sExpLeft, mon);
                    *seq = 2;
                } else {
                    *seq = 13;
                }
                exp = 0;
            }
            sExpLeft = exp;
        } else {
            *seq = 13;
        }
        break;
    case 2:
        if (!BtlvEffect_CheckExecuteGauge()) {
            *seq = 13;
        }
        break;
    case 3:
        func_ov167_021bc6a0(mon);
        func_02038bc8(9);
        func_ov167_021bc3fc(mon);
        func_ov167_021bc6ac(mon);
        if (visible) {
            BtlvEffect_CalcGaugeExpLevelUp(pos, mon);
            *seq = 4;
        } else {
            *seq = 6;
        }
        break;
    case 4:
        if (!BtlvEffect_CheckExecuteGauge() && !GFL_SndPlayerIsActiveAny()) {
            func_ov167_021cff78(client->viewCore, pos, 0x25d);
            *seq = 5;
        }
        break;
    case 5:
        if (func_ov167_021cff94(client->viewCore)) {
            *seq = 6;
        }
        break;
    case 6:
        Btlv_StringParam_Setup(&client->strParam, 1, 0x3c);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLevelUp.level);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        *seq = 7;
        break;
    case 7:
        if (func_ov167_021d02f8(client->viewCore)) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(0x515, 0xffff);
        }
        if (func_ov167_021d02e8(client->viewCore) && !GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            sExpSeq = 0;
            *seq = 8;
        }
        break;
    case 8:
        sExpSeq++;
        if ((GCTX_HIDGetPressedKeys() & 3) || func_0203da48() || sExpSeq > 80) {
            sExpSeq = 0;
            func_ov167_0219db64(client->mainModule, mon);
            func_ov167_021d0978(client->viewCore, mon, &sLevelUp);
            *seq = 9;
        }
        break;
    case 9:
        if (func_ov167_021d0988(client->viewCore) && ((GCTX_HIDGetPressedKeys() & 3) || func_0203da48())) {
            func_ov167_021d0998(client->viewCore);
            *seq = 10;
        }
        break;
    case 10:
        if (func_ov167_021d09a8(client->viewCore) && ((GCTX_HIDGetPressedKeys() & 3) || func_0203da48())) {
            func_ov167_021d09b8(client->viewCore);
            *seq = 11;
        }
        break;
    case 11:
        if (func_ov167_021d09c8(client->viewCore)) {
            sExpSeq = 0;
            *seq = 12;
        }
        break;
    case 12:
        if (BattleClient_LearnMoveSeq(client, &sExpSeq, mon)) {
            *seq = 1;
        }
        break;
    case 13:
        func_ov167_0219cc34(client->mainModule, monId);
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ForgetMoveSECallback(u32 arg0) {
    switch (arg0) {
    case 3:
        GFL_SndSEPlay(0x56b);
        break;
    case 5:
        if (GFL_SndIsPlaying(0x56b)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_LearnMoveSeq(BtlClient *client, s32 *seq, BattleMon *mon) {
    u8 monId = GetMonID(mon);

    switch (*seq) {
    case 0:
        sLearnIdx = 0;
        sLearnMove = 0;
        *seq = 1;
    case 1:
        sLearnMove = func_0201d358(GetSrcData(mon), &sLearnIdx, client->heapId);
        if (sLearnMove == 0) {
            return TRUE;
        }
        if (sLearnMove == 0xfffe) {
            break;
        }
        if (sLearnMove & 0x8000) {
            sLearnMove &= 0x7fff;
            *seq = 5;
        } else {
            *seq = 2;
        }
        break;
    case 2: {
        BattleMon *bpp = GetPokeParam(client->pokeCon, monId);

        GetSrcData(bpp);
        func_ov167_021bc3fc(bpp);
        Btlv_StringParam_Setup(&client->strParam, 5, 3);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        *seq = 3;
    }
    case 3:
        if (func_ov167_021d02f8(client->viewCore)) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(0x515, 0xffff);
        }
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 4;
        }
        break;
    case 4:
        if (!GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            *seq = 1;
        }
        break;
    case 5:
        Btlv_StringParam_Setup(&client->strParam, 5, 4);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
        func_ov167_021d0b4c(&client->strParam, 0xff);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        *seq = 6;
    case 6:
        if (func_ov167_021d02f8(client->viewCore)) {
            BtlvStringParam yesParam;
            BtlvStringParam noParam;

            Btlv_StringParam_Setup(&yesParam, 6, 2);
            Btlv_StringParam_Setup(&noParam, 6, 3);
            func_ov167_021d0798(client->viewCore, &yesParam, &noParam, 1);
        }
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 7;
        }
        break;
    case 7: {
        u32 answer;

        if (func_ov167_021d0828(client->viewCore, &answer)) {
            if (answer == 0) {
                func_ov167_021d0838(client->viewCore,
                                    func_ov167_0219d5b0(GetClientParty(client->pokeCon, client->clientId), monId),
                                    sLearnMove);
                *seq = 8;
            } else {
                *seq = 10;
            }
        }
        break;
    }
    case 8: {
        u8 slot;

        if (func_ov167_021d0854(client->viewCore, &slot)) {
            if (slot == 4) {
                *seq = 5;
            } else {
                PartyPkm *pkm = GetSrcData(GetPokeParam(client->pokeCon, monId));
                u16 oldMove = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL);

                PokeParty_SetMove(pkm, sLearnMove, slot);
                Btlv_StringParam_Setup(&client->strParam, 5, 5);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                Btlv_StringParam_AddArg(&client->strParam, oldMove);
                func_ov167_021d0210(client->viewCore, &client->strParam, BattleClient_ForgetMoveSECallback);
                *seq = 9;
            }
        }
        break;
    }
    case 9:
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 2;
        }
        break;
    case 10:
        Btlv_StringParam_Setup(&client->strParam, 5, 7);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
        func_ov167_021d0b4c(&client->strParam, 0xff);
        func_ov167_021d01ec(client->viewCore, &client->strParam);
        *seq = 11;
        break;
    case 11:
        if (func_ov167_021d02f8(client->viewCore)) {
            BtlvStringParam yesParam;
            BtlvStringParam noParam;

            Btlv_StringParam_Setup(&yesParam, 6, 4);
            Btlv_StringParam_AddArg(&yesParam, sLearnMove);
            Btlv_StringParam_Setup(&noParam, 6, 5);
            Btlv_StringParam_AddArg(&noParam, sLearnMove);
            func_ov167_021d0798(client->viewCore, &yesParam, &noParam, 1);
        }
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 12;
        }
        break;
    case 12: {
        u32 answer;

        if (func_ov167_021d0828(client->viewCore, &answer)) {
            if (answer == 0) {
                Btlv_StringParam_Setup(&client->strParam, 5, 8);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
                func_ov167_021d01ec(client->viewCore, &client->strParam);
                *seq = 13;
            } else {
                *seq = 5;
            }
        }
        break;
    }
    case 13:
        if (func_ov167_021d02e8(client->viewCore)) {
            *seq = 1;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b83dc(BtlClient *client, s32 *seq, const u32 *args) {
    static const u16 sEscapeMessages[4] = { 0x3d, 0x3e, 0x3f, 0x40 };
    BattleMon *mon;
    u16 message;

    switch (*seq) {
    case 0:
        BtlvEffect_StartEffect23A(func_ov167_0219c6dc(client->mainModule, args[0]), args[5], args[1], args[2], args[4]);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            if (args[2]) {
                mon = func_ov167_0219d188(client->pokeCon, args[0]);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x41);
                Btlv_StringParam_AddArg(&client->strParam, GetMonID(mon));
                BtlvEffect_PlayBgmNoPinch(0x518);
            } else {
                if (args[1] < 4) {
                    message = sEscapeMessages[args[1]];
                } else {
                    message = 0x3d;
                }
                Btlv_StringParam_Setup(&client->strParam, 1, message);
            }
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            if (args[2]) {
                mon = func_ov167_0219d188(client->pokeCon, args[0]);
                func_ov167_0219dc00(client->mainModule, mon);
                (*seq)++;
            } else {
                return TRUE;
            }
        }
        break;
    case 3:
        if (!GFL_SndBGMIsPlaying()) {
            BtlvEffect_Resume();
            GFL_SndBGMPlay(0x47c, 0xffff);
            func_ov167_0219cb7c(client->mainModule);
            if (args[3]) {
                mon = func_ov167_0219d188(client->pokeCon, args[0]);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x42);
                Btlv_StringParam_AddArg(&client->strParam, GetMonID(mon));
                func_ov167_021d01ec(client->viewCore, &client->strParam);
                (*seq)++;
            } else {
                return TRUE;
            }
        }
        break;
    case 4:
        if (func_ov167_021d02e8(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b853c(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_StartEffect23D(func_ov167_0219c6dc(client->mainModule, args[0]), args[1]);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0x43);
            func_ov167_021d01ec(client->viewCore, &client->strParam);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b85a4(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 clientId = args[0];
        u8 result = args[1];

        func_ov167_0219d544(GetPartyData(client->pokeCon, clientId), result, NULL, NULL);
        func_ov167_021d0640(client->viewCore, clientId, result);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d06ac(client->viewCore)) {
            u8 result = args[1];
            u8 isNpc;
            u16 message;

            if (result == 1) {
                return TRUE;
            }
            isNpc = func_ov167_0219d888(client->mainModule, args[0]);
            if (result == 3) {
                message = isNpc ? 0x26 : 0x27;
            } else {
                message = isNpc ? 0x28 : 0x29;
            }
            func_ov167_021d0250(client->viewCore, message, args);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d02e8(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b8650(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monId = args[0];
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, monId);

    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            ChangeAbility(GetPokeParam(client->pokeCon, monId), args[1]);
            return TRUE;
        }
        func_ov167_021d0340(client->viewCore, pos, 0);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0350(client->viewCore, pos)) {
            ChangeAbility(GetPokeParam(client->pokeCon, monId), args[1]);
            func_ov167_021d0380(client->viewCore, pos);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d0390(client->viewCore, pos)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b86e4(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monId1 = args[0];
    u8 monId2 = args[1];
    u16 ability1 = args[2];
    u16 ability2 = args[3];
    u8 pos1 = MonIDToBattlePos(client->mainModule, client->pokeCon, monId1);
    u8 pos2 = MonIDToBattlePos(client->mainModule, client->pokeCon, monId2);
    BattleMon *mon1 = GetPokeParam(client->pokeCon, monId1);
    BattleMon *mon2 = GetPokeParam(client->pokeCon, monId2);

    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            ChangeAbility(mon1, ability1);
            ChangeAbility(mon2, ability2);
            return TRUE;
        }
        if (IsAllyMonID(monId1, monId2)) {
            ChangeAbility(mon1, ability1);
            ChangeAbility(mon2, ability2);
            return TRUE;
        }
        func_ov167_021d0340(client->viewCore, pos1, 0);
        sSwapTimer = 8;
        (*seq)++;
        break;
    case 1:
        func_ov167_021d0350(client->viewCore, pos1);
        if (sSwapTimer != 0) {
            sSwapTimer--;
        } else {
            func_ov167_021d0340(client->viewCore, pos2, 0);
            (*seq)++;
        }
        break;
    case 2: {
        u8 done1 = func_ov167_021d0350(client->viewCore, pos1);
        u8 done2 = func_ov167_021d0350(client->viewCore, pos2);

        if (done1 && done2) {
            (*seq)++;
        }
        break;
    }
    case 3:
        ChangeAbility(mon1, ability1);
        ChangeAbility(mon2, ability2);
        sSwapTimer = 0;
        (*seq)++;
        break;
    case 4:
        if (++sSwapTimer > 8) {
            func_ov167_021d0380(client->viewCore, pos1);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov167_021d0390(client->viewCore, pos1)) {
            func_ov167_021d0380(client->viewCore, pos2);
            (*seq)++;
        }
        break;
    case 6:
        if (func_ov167_021d0390(client->viewCore, pos2)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b8850(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 monId = args[0];
        u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, monId);
        BattleMon *mon = GetPokeParam(client->pokeCon, monId);
        BOOL skip = func_ov167_021b1990(client);

        IllusionBreak(mon);
        func_ov167_021d0440(client->viewCore, pos, skip);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d0450(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b88b4(BtlClient *client, s32 *seq, const u32 *args) {
    if (*seq == 0) {
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        BtlvEffect_Start(args[0]);
        (*seq)++;
    } else if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b88ec(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = func_ov167_0219c6dc(client->mainModule, args[0]);

    if (*seq == 0) {
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        if (!func_ov167_0219bd88(client->mainModule) && func_ov169_0689cb5c(args[1])) {
            return TRUE;
        }
        func_ov167_021cff78(client->viewCore, pos, args[1]);
        (*seq)++;
    } else if (func_ov167_021cff94(client->viewCore)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b896c(BtlClient *client, s32 *seq, const u32 *args) {
    if (*seq == 0) {
        u8 pos1;

        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        pos1 = func_ov167_0219c6dc(client->mainModule, args[0]);
        func_ov167_021cffa8(client->viewCore, pos1, func_ov167_0219c6dc(client->mainModule, args[1]), args[2]);
        (*seq)++;
    } else if (func_ov167_021cffb8(client->viewCore)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b89cc(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 pos =
            func_ov167_0219c6dc(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));

        ChangeForm(GetPokeParam(client->pokeCon, args[0]), args[1]);
        func_ov167_021d0460(client->viewCore, pos);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d047c(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b8a34(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

    switch (*seq) {
    case 0:
        if (func_ov167_021b1990(client)) {
            return TRUE;
        }
        func_ov167_021d0340(client->viewCore, pos, 1);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0350(client->viewCore, pos)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021b8a8c(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

    switch (*seq) {
    case 0:
        func_ov167_021d0360(client->viewCore, pos);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d0370(client->viewCore, pos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021b8ad0(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb790(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8af0(BtlClient *client, s32 *seq, const u32 *args) {
    HPAdd(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8b10(BtlClient *client, s32 *seq, const u32 *args) {
    u8 moveIdx = args[1];
    u8 amount = args[2];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    func_ov167_021bae08(mon, moveIdx, amount);
    return TRUE;
}

static BOOL func_ov167_021b8b38(BtlClient *client, s32 *seq, const u32 *args) {
    u8 moveIdx = args[1];
    u8 amount = args[2];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    func_ov167_021bae40(mon, moveIdx, amount);
    return TRUE;
}

static BOOL func_ov167_021b8b60(BtlClient *client, s32 *seq, const u32 *args) {
    u8 moveIdx = args[1];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    func_ov167_021baecc(mon, moveIdx);
    return TRUE;
}

static BOOL func_ov167_021b8b80(BtlClient *client, s32 *seq, const u32 *args) {
    HPZero(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b8b98(BtlClient *client, s32 *seq, const u32 *args) {
    Move_IncrementPP(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL func_ov167_021b8bbc(BtlClient *client, s32 *seq, const u32 *args) {
    Move_IncrementPP_Org(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL func_ov167_021b8be0(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb5c0(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL func_ov167_021b8c00(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb638(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL func_ov167_021b8c20(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    func_ov167_021bb6a8(mon, 1, args[1]);
    func_ov167_021bb6a8(mon, 2, args[2]);
    func_ov167_021bb6a8(mon, 3, args[3]);
    func_ov167_021bb6a8(mon, 4, args[4]);
    func_ov167_021bb6a8(mon, 5, args[5]);
    func_ov167_021bb6a8(mon, 6, args[6]);
    func_ov167_021bb6a8(mon, 7, args[7]);
    return TRUE;
}

static BOOL func_ov167_021b8c98(BtlClient *client, s32 *seq, const u32 *args) {
    StatStageRecover(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b8cb0(BtlClient *client, s32 *seq, const u32 *args) {
    StatStageReset(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b8cc8(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb738(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8ce4(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);
    BattleCondition value;

    value.raw = args[2];
    SetMoveCondition(mon, args[1], value);
    return TRUE;
}

static BOOL func_ov167_021b8d04(BtlClient *client, s32 *seq, const u32 *args) {
    CureCondition(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b8d1c(BtlClient *client, s32 *seq, const u32 *args) {
    CureMoveCondition(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8d38(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos1 = client->cmdArgs[1];
    u8 pos2 = client->cmdArgs[2];
    BattleParty *party = GetPartyData(client->pokeCon, (u8)client->cmdArgs[0]);
    BattleMon *mon;

    if (pos1 != pos2) {
        func_ov167_0219d504(party, pos1, pos2);
    }
    mon = func_ov167_0219d4e4(party, pos1);
    func_ov167_021bbbec(mon, args[3]);
    func_ov167_021bbd80(mon);
    return TRUE;
}

static BOOL func_ov167_021b8d8c(BtlClient *client, s32 *seq, const u32 *args) {
    ChangePokeType(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8dac(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monIds[6];
    u32 numMons = args[0];
    u32 i;
    u32 numConditions = args[1];
    u32 j;

    func_ov167_021bd8b8(args[2], monIds);
    for (i = 0; i < numConditions; i++) {
        u32 condition = func_ov169_0689cb80(i);

        if (condition == 0) {
            break;
        }
        for (j = 0; j < numMons; j++) {
            BattleMon *mon = GetPokeParam(client->pokeCon, monIds[j]);

            if (!IsFainted(mon)) {
                func_ov167_021bb864(mon, condition, NULL, NULL);
            }
        }
    }
    return TRUE;
}

static BOOL func_ov167_021b8e24(BtlClient *client, s32 *seq, const u32 *args) {
    ConsumeItem(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8e44(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bbf44(GetPokeParam(client->pokeCon, args[0]), args[1], args[2], args[3], args[4], args[5]);
    return TRUE;
}

static BOOL func_ov167_021b8e80(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb7e4(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8e9c(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb808(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8eb8(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bb7c0(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8ed4(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bbc40(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8ef0(BtlClient *client, s32 *seq, const u32 *args) {
    ChangeAbility(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8f0c(BtlClient *client, s32 *seq, const u32 *args) {
    SetItem(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8f2c(BtlClient *client, s32 *seq, const u32 *args) {
    Move_UpdateID(GetPokeParam(client->pokeCon, args[0]), args[1], args[4], args[2], args[3]);
    return TRUE;
}

static BOOL func_ov167_021b8f60(BtlClient *client, s32 *seq, const u32 *args) {
    Clear_ForSwitch(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b8f78(BtlClient *client, s32 *seq, const u32 *args) {
    BattleCondition value;

    value.raw = args[1];
    FieldStatusaddEffectCore(client->field, args[0], value, 0);
    return TRUE;
}

static BOOL func_ov167_021b8f8c(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021d5c04(client->field, args[0], args[1]);
    return TRUE;
}

static BOOL func_ov167_021b8fa0(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021d5c60(client->field, args[0]);
    return TRUE;
}

static BOOL func_ov167_021b8fb4(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021d5bc0(client->field, args[0]);
    return TRUE;
}

static BOOL func_ov167_021b8fc4(BtlClient *client, s32 *seq, const u32 *args) {
    COUNTER_Set(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL func_ov167_021b8fe4(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *source = GetPokeParam(client->pokeCon, args[0]);

    CopyBatonPassParams(GetPokeParam(client->pokeCon, args[1]), source);
    return TRUE;
}

static BOOL func_ov167_021b9010(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bc55c(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL func_ov167_021b9030(BtlClient *client, s32 *seq, const u32 *args) {
    ResetSpActPriority(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b9048(BtlClient *client, s32 *seq, const u32 *args) {
    u8 clientId = args[0];
    u8 value = args[1];

    if (clientId == client->clientId) {
        func_ov167_021b19b0(client, value);
    } else {
        func_ov167_0219d404(client->mainModule, clientId, value);
    }
    return TRUE;
}

static BOOL func_ov167_021b9074(BtlClient *client, s32 *seq, const u32 *args) {
    u8 clientId = args[0];

    func_ov167_0219d604(client->mainModule, GetPartyData(client->pokeCon, clientId), clientId);
    return TRUE;
}

static BOOL func_ov167_021b9094(BtlClient *client, s32 *seq, const u32 *args) {
    ClearConsumedItem(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL func_ov167_021b90ac(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021bba64(GetPokeParam(client->pokeCon, args[0]), (u8)args[1]);
    return TRUE;
}

static BOOL func_ov167_021b90cc(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);
    BattleMonDamageRecord record;

    BattleMonDamageRecord_Init(&record, args[1], args[2], args[4], args[3], args[5]);
    func_ov167_021bc048(mon, &record);
    return TRUE;
}

static BOOL func_ov167_021b9100(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    func_ov167_021bbc08(mon);
    ComboMove_ClearParam(mon);
    return TRUE;
}

static BOOL func_ov167_021b9120(BtlClient *client, s32 *seq, const u32 *args) {
    func_ov167_021d5da4(client->field, NULL, NULL);
    return TRUE;
}

static BOOL func_ov167_021b9130(BtlClient *client, s32 *seq, const u32 *args) {
    u32 stat = args[1];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    SetBaseStatus(mon, stat, args[2]);
    return TRUE;
}

static BOOL func_ov167_021b9154(BtlClient *client, s32 *seq, const u32 *args) {
    SetWeight(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

u8 BattleClient_GetClientId(BtlClient *client) {
    return client->clientId;
}

BattleParty *BattleClient_GetParty(BtlClient *client) {
    return client->party;
}

u8 func_ov167_021b9188(BtlClient *client) {
    return func_ov167_021d5ad4(client->field);
}

u16 func_ov167_021b9194(BtlClient *client) {
    return client->studio.unk58;
}

u32 func_ov167_021b919c(BtlClient *client) {
    return client->studio.audienceMask;
}

u8 BattleClient_GetShooterEnergy(BtlClient *client) {
    return client->shooterEnergy;
}

static void BattleClient_LoadAIItems(BtlClient *client) {
    u32 i;

    for (i = 0; i < 4; i++) {
        client->aiItems[i] = func_ov167_0219d89c(client->mainModule, client->clientId, i);
    }
}

static u16 AICheckItemUse(BtlClient *client, BattleMon *mon, BattleParty *party) {
    u8 numMons = GetNumMonsInParty(party);
    u32 i;

    if (CheckCondition(mon, 0x13)) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (sAIItemMaxMons[i] < numMons) {
            break;
        }
        if (client->aiItems[i] != 0) {
            BOOL use = FALSE;

            if (ItemGetParam(client->aiItems[i], 0x29)) {
                if (GetBattleMonStat(mon, 0xd) > DivideMaxHp(mon, 4)) {
                    continue;
                }
                use = TRUE;
            } else if (IsXItem(client->aiItems[i], mon)) {
                use = TRUE;
            } else if (IsStatusCureItem(client->aiItems[i], mon)) {
                use = TRUE;
            }
            if (use) {
                u16 *slot = &client->aiItems[i];
                u16 item = *slot;

                *slot = 0;
                return item;
            }
        }
    }
    return 0;
}

static const BtlClientItemStat sXItems[6] = {
    { 0x1e, 1 }, { 0x1f, 2 }, { 0x20, 3 }, { 0x21, 4 }, { 0x22, 5 }, { 0x23, 6 },
};

static BOOL IsXItem(u16 item, BattleMon *mon) {
    u32 i;

    for (i = 0; i < NELEMS(sXItems); i++) {
        if (ItemGetParam(item, sXItems[i].param) && IsStatChangeValid(mon, sXItems[i].stat, 1)) {
            return TRUE;
        }
    }
    if (ItemGetParam(item, 0x24) && func_ov167_021bb714(mon) == 0) {
        return TRUE;
    }
    return FALSE;
}

static const BtlClientItemCondition sCureItems[7] = {
    { 0x12, 2 }, { 0x13, 5 }, { 0x14, 4 }, { 0x15, 3 }, { 0x16, 1 }, { 0x17, 6 }, { 0x18, 7 },
};

static BOOL IsStatusCureItem(u16 item, BattleMon *mon) {
    u32 i;

    for (i = 0; i < NELEMS(sCureItems); i++) {
        if (ItemGetParam(item, sCureItems[i].param) && CheckCondition(mon, sCureItems[i].condition)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void RecPlayer_Init(BtlClientRecPlayer *recPlayer) {
    recPlayer->result = 0;
    recPlayer->seq = 0;
    recPlayer->chapterReached = FALSE;
    recPlayer->quitStarted = FALSE;
    recPlayer->quitDone = FALSE;
    recPlayer->chapter = 0;
    recPlayer->skipping = FALSE;
    recPlayer->skipTarget = 0;
    recPlayer->unk2_4 = FALSE;
    recPlayer->maxChapter = 0;
    recPlayer->dataEnd = FALSE;
    recPlayer->skipCount = 0;
    recPlayer->quitTimer = 0;
}

static void RecPlayer_SetMaxChapter(BtlClientRecPlayer *recPlayer, u32 maxChapter) {
    recPlayer->maxChapter = maxChapter;
}

static BOOL func_ov167_021b9360(BtlClientRecPlayer *recPlayer) {
    return recPlayer->quitDone;
}

static void func_ov167_021b9368(BtlClientRecPlayer *recPlayer) {
    recPlayer->chapterReached = TRUE;
}

static u32 func_ov167_021b9374(BtlClientRecPlayer *recPlayer) {
    return recPlayer->result;
}

static void RecPlayer_StartSkip(BtlClientRecPlayer *recPlayer, u32 target) {
    recPlayer->skipping = TRUE;
    recPlayer->skipCount = 0;
    recPlayer->skipTarget = target;
    recPlayer->quitStarted = FALSE;
    recPlayer->quitDone = FALSE;
}

static void func_ov167_021b939c(BtlClientRecPlayer *recPlayer) {
    recPlayer->seq = 0;
    recPlayer->skipping = FALSE;
}

static BOOL func_ov167_021b93ac(BtlClientRecPlayer *recPlayer) {
    if (recPlayer->skipCount == recPlayer->skipTarget) {
        return TRUE;
    }
    return FALSE;
}

static u32 func_ov167_021b93bc(BtlClientRecPlayer *recPlayer) {
    return recPlayer->skipTarget;
}

static BOOL func_ov167_021b93c0(BtlClientRecPlayer *recPlayer) {
    if (recPlayer->quitStarted) {
        return FALSE;
    }
    recPlayer->unk2_4 = TRUE;
    return TRUE;
}

static void func_ov167_021b93d8(BtlClientRecPlayer *recPlayer) {
    recPlayer->unk2_4 = FALSE;
}

static void func_ov167_021b93e4(BtlClientRecPlayer *recPlayer) {
    recPlayer->dataEnd = TRUE;
}

static void RecPlayer_Update(BtlClient *client, BtlClientRecPlayer *recPlayer) {
    if (client->clientType == 2 && !recPlayer->unk2_4 && !recPlayer->dataEnd && recPlayer->maxChapter != 0) {
        BOOL update = FALSE;

        if (recPlayer->chapterReached) {
            recPlayer->chapterReached = FALSE;
            if (!recPlayer->skipping) {
                if (recPlayer->chapter < recPlayer->maxChapter && recPlayer->seq == 0) {
                    recPlayer->chapter++;
                    if (recPlayer->quitTimer == 0) {
                        recPlayer->skipTarget = recPlayer->chapter;
                    }
                    update = TRUE;
                }
            } else {
                if (recPlayer->skipCount < recPlayer->skipTarget) {
                    recPlayer->skipCount++;
                    if (recPlayer->skipCount == recPlayer->skipTarget) {
                        recPlayer->chapter = recPlayer->skipTarget;
                        if (client->viewCore != NULL) {
                            func_ov167_021d0a48(client->viewCore, recPlayer->chapter, recPlayer->skipTarget);
                        }
                    }
                }
                return;
            }
        }
        if (recPlayer->chapter != 0 && client->viewCore != NULL) {
            switch (recPlayer->seq) {
            case 0: {
                u32 input = func_ov167_021d0a38(client->viewCore);
                BOOL changed = FALSE;

                switch (input) {
                case 2:
                    recPlayer->result = 1;
                    func_ov167_021d09d8(client->viewCore);
                    recPlayer->quitStarted = TRUE;
                    recPlayer->seq = 1;
                    func_ov167_021d0aa0(client->viewCore, recPlayer->chapter, 1);
                    func_ov167_0219c9d4(client->mainModule);
                    func_ov167_0219bdf0(client->mainModule);
                    GFL_SndBGMFadeOut(30);
                    GFL_SndPlayerSetMuteStateEx(0, 0x3e);
                    PokeVoice_SetMasterVolume(0);
                    BtlvEffect_ReleaseVoices();
                    func_ov167_0219cb7c(client->mainModule);
                    break;
                case 0:
                    if (recPlayer->skipTarget > 1) {
                        recPlayer->skipTarget--;
                        changed = TRUE;
                    }
                    recPlayer->quitTimer = 45;
                    break;
                case 1:
                    if (recPlayer->skipTarget < recPlayer->maxChapter) {
                        recPlayer->skipTarget++;
                        changed = TRUE;
                    }
                    recPlayer->quitTimer = 45;
                    break;
                }
                if (changed || update) {
                    func_ov167_021d0a48(client->viewCore, recPlayer->chapter, recPlayer->skipTarget);
                }
                if (recPlayer->quitTimer != 0) {
                    recPlayer->quitTimer--;
                    if (recPlayer->quitTimer == 0) {
                        recPlayer->result = 2;
                        func_ov167_021d09d8(client->viewCore);
                        recPlayer->quitStarted = TRUE;
                        recPlayer->seq = 1;
                        GFL_SndBGMFadeOut(30);
                        GFL_SndPlayerSetMuteStateEx(0, 0x3e);
                        PokeVoice_SetMasterVolume(0);
                        BtlvEffect_ReleaseVoices();
                    }
                }
                break;
            }
            case 1:
                if (func_ov167_021d09e8(client->viewCore)) {
                    recPlayer->quitDone = TRUE;
                    recPlayer->seq = 2;
                }
                break;
            case 2:
                break;
            }
        }
    }
}

static void func_ov167_021b9570(BtlClientStudioWork *studio, u32 reaction) {
    static const BtlClientAudienceSE sAudienceSE[9] = {
        { 0, 0 },         { 0x8c1, 0x8c1 }, { 0x8c5, 0x8c6 }, { 0x8c7, 0x8c8 }, { 0x8c9, 0x8ca },
        { 0x8bf, 0x8c0 }, { 0x8c3, 0x8c4 }, { 0x8cb, 0x8cc }, { 0x8cd, 0x8ce },
    };

    if (studio->unk58 < 12) {
        if (sAudienceSE[reaction].se != 0) {
            GFL_SndSEPlay(sAudienceSE[reaction].se);
        }
    } else {
        if (sAudienceSE[reaction].seLate != 0) {
            GFL_SndSEPlay(sAudienceSE[reaction].seLate);
        }
    }
}

static void Studio_ReactAudience(BtlvCore *core, BtlClientStudioWork *studio, u32 reaction) {
    int i;
    int count = 0;

    for (i = 0; i < 22; i++) {
        if ((studio->audienceMask >> i) & 1) {
            func_ov167_021d0bac(core, 1, i, reaction);
            studio->audienceTimers[i] = GFL_RandomLC(300) + 400;
            count++;
        }
    }
    if (count != 0) {
        func_ov167_021b9570(studio, reaction);
    }
}

static void Studio_AudienceLeave(BtlvCore *core, BtlClientStudioWork *studio, u16 level) {
    static const BtlClientAudienceLeave sAudienceLeave[5] = {
        { 0, 9, 1, 3 }, { 10, 13, 3, 3 }, { 14, 17, 6, 3 }, { 18, 21, 11, 4 }, { 22, 22, 20, 1 },
    };
    s8 members[22];
    int count = 0;
    int row = 0;
    int left = 0;
    int numLeave;
    int i;

    for (i = 0; i < 22; i++) {
        if ((studio->audienceMask >> i) & 1) {
            members[count++] = i;
        }
    }
    for (i = 0; i < 5; i++) {
        if (sAudienceLeave[i].min <= level && sAudienceLeave[i].max >= level) {
            row = i;
        }
    }
    numLeave = sAudienceLeave[row].base + GFL_RandomLC(sAudienceLeave[row].range);
    while (left != numLeave) {
        u32 r = GFL_RandomLC(count);
        int member = members[r];

        if (member >= 0) {
            func_ov167_021d0bac(core, 1, member, 9);
            studio->audienceTimers[member] = 0;
            studio->audienceMask ^= 1 << member;
            left++;
            members[r] = -1;
        }
    }
}

static void func_ov167_021b96b0(BtlvCore *core, BtlClientStudioWork *studio) {
    if (studio->score.points[2] < 0) {
        Studio_AudienceLeave(core, studio, studio->unk58);
        Studio_ReactAudience(core, studio, 5);
    } else if (studio->score.points[2] > 60) {
        Studio_ReactAudience(core, studio, 7);
    }
}

static void func_ov167_021b96e0(BtlvCore *core, BtlClientStudioWork *studio, int fame) {
    if (fame != 0) {
        Studio_ReactAudience(core, studio, 1);
    }
}

static void func_ov167_021b96f0(BtlClient *client, BtlClientStudioWork *studio, BattleParty *party) {
    static const u8 sFameBonus[5] = { 0, 1, 5, 10, 15 };
    PartyPkm *pkm = GetSrcData(GetBattleMonFromParty(party, 0));
    int fame;

    PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    fame = func_0201f010(PokeParty_GetParam(pkm, PKM_PARAM_POKESTAR_FAME, NULL));
    studio->score.points[1] += sFameBonus[fame];
    studio->score.counts[1]++;
    func_ov167_021b96e0(client->viewCore, &client->studio, fame);
}

static void func_ov167_021b9750(BtlClient *client, BtlClientStudioScore *score, s32 scene) {
    BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);

    score->counts[2] = scene;
    score->points[2] += rules->scenes[scene].unk0A;
}

static void func_ov167_021b9774(BtlClient *client, BtlClientStudioScore *score) {
    BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);
    int points = 5 - MATH_ABS(rules->unk0C - client->turnCount);

    if (points < 0) {
        points = 0;
    }
    score->points[0] = points;
    score->counts[0] = client->turnCount;
}

static void func_ov167_021b97a4(BtlClient *client, BtlClientStudioScore *score, s32 scene, u32 index) {
    static const u32 sReactions[5][3] = {
        { 2, 5, 0 }, { 3, 6, 0 }, { 4, 6, 0 }, { 4, 5, 0 }, { 3, 6, 0 },
    };
    BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);
    int points = rules->scenes[scene].choices[index].points;
    u32 reaction;

    score->points[3] += points;
    score->counts[3]++;
    if (points > 0) {
        reaction = sReactions[rules->scenes[scene].unk08][0];
        func_ov167_021b2210(client, 1);
    } else if (points < 0) {
        reaction = sReactions[rules->scenes[scene].unk08][1];
        func_ov167_021b2210(client, 3);
    } else {
        reaction = sReactions[rules->scenes[scene].unk08][2];
        func_ov167_021b2210(client, 2);
    }
    Studio_ReactAudience(client->viewCore, &client->studio, reaction);
}

static s32 StudioScore_CalcTotal(BtlClient *client, BtlClientStudioScore *score) {
    BtlScriptedRules *rules = func_ov167_0219e39c(client->mainModule);
    BtlSetup *setup = func_ov167_0219e310(client->mainModule);
    s32 total;
    s32 sum;

    sum = score->points[1] + score->points[0];
    sum += score->points[2];
    sum += score->points[3];
    if (sum == 0) {
        sum = 1;
    }
    score->unk08 = rules->unk04;
    score->unk0A = setup->unk128;
    total = sum * score->unk08;
    score->total = total * score->unk0A;
    if (score->total > 9999) {
        score->total = 9999;
    } else if (score->total < -9998) {
        score->total = -9998;
    }
    return score->total;
}

static void Studio_UpdateAudience(BtlClient *client) {
    int i;

    if (func_ov167_0219c988(client->mainModule) == 2 && client->clientId == 0) {
        for (i = 0; i < 22; i++) {
            if (client->studio.audienceTimers[i] != 0) {
                client->studio.audienceTimers[i]--;
                if (client->studio.audienceTimers[i] == 0) {
                    func_ov167_021d0bf4(client->viewCore, i);
                    func_ov167_021d0bac(client->viewCore, 1, i, 0);
                }
            }
        }
    }
}

static void func_ov167_021b9910(BtlMainModule *mainModule) {
    BtlSetup *setup = func_ov167_0219e310(mainModule);

    if (setup->unk124 == 0) {
        BtlvEffect_PlayBgm(func_ov167_0219bf08(mainModule));
    } else if (setup->unk124 == 1) {
        BtlvEffect_PlayBgm(func_ov167_0219bf00(mainModule));
    } else if (setup->unk124 == 2) {
        BtlvEffect_PlayBgm(func_ov167_0219bf14(mainModule));
    }
}
