#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_adapter.h"
#include "battle/btl_calc.h"
#include "battle/btl_client.h"
#include "battle/btl_field.h"
#include "battle/btl_main.h"
#include "battle/btl_poke_select.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_rec.h"
#include "battle/btl_result.h"
#include "battle/btl_server.h"
#include "battle/btl_server_cmd.h"
#include "battle/btl_setup.h"
#include "battle/btl_string.h"
#include "battle/btl_tables.h"
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

// The most energy a client's Shooter holds
#define SHOOTER_ENERGY_MAX 14

// What drives a client, which picks its handler of each command
enum {
    BTL_CLIENT_TYPE_PLAYER,
    BTL_CLIENT_TYPE_AI,
    // A recorded battle's playback
    BTL_CLIENT_TYPE_REC_PLAY,
    BTL_CLIENT_TYPE_MAX,
};

// The steps of BattleClient_MainProcNormal: wait for a command, run its handler, return the reply and wait for the
// next, return the reply to the last command and end, or, when the player stops a playback or skips to a chapter, fade
// out and quit or start the skip
enum {
    CLIENT_MAIN_SEQ_RECV_CMD,
    CLIENT_MAIN_SEQ_RUN_CMD,
    CLIENT_MAIN_SEQ_RETURN,
    CLIENT_MAIN_SEQ_RETURN_LAST,
    CLIENT_MAIN_SEQ_REC_QUIT,
    CLIENT_MAIN_SEQ_REC_SKIP,
    CLIENT_MAIN_SEQ_WAIT_REC_QUIT,
    CLIENT_MAIN_SEQ_END,
};

// The steps of BattleClient_MainProcRecPlay, which plays a recording up to the chapter skipped to: take each command
// and run its handler or return a dummy reply, then fade in at the chapter
enum {
    CLIENT_SKIP_SEQ_INIT,
    CLIENT_SKIP_SEQ_RECV_CMD,
    CLIENT_SKIP_SEQ_RUN_CMD,
    CLIENT_SKIP_SEQ_RETURN,
    CLIENT_SKIP_SEQ_FADE_IN,
    CLIENT_SKIP_SEQ_END,
};

// The steps of Studio_PlayTurnMessages, which plays the script's two messages of the turn, if it has any, between two
// effects at the start, the first once the opponent's trainer is set up, and two at the end
enum {
    STUDIO_TURN_SEQ_INIT,
    STUDIO_TURN_SEQ_START_EFFECT_1,
    STUDIO_TURN_SEQ_START_EFFECT_2,
    STUDIO_TURN_SEQ_MSG_1,
    STUDIO_TURN_SEQ_WAIT_MSG_1,
    STUDIO_TURN_SEQ_MSG_2,
    STUDIO_TURN_SEQ_WAIT_MSG_2,
    STUDIO_TURN_SEQ_END_EFFECT_1,
    STUDIO_TURN_SEQ_END_EFFECT_2,
    STUDIO_TURN_SEQ_END,
    STUDIO_TURN_SEQ_WAIT_EFFECT,
};

// The steps of Studio_PlayScene, which plays a scene of the movie's script: its staging and messages, then its
// choice, picked by the player or the saved one, and the choice's messages; or, in its place, a cut-in between its
// effects
enum {
    STUDIO_SCENE_SEQ_INIT,
    STUDIO_SCENE_SEQ_CHECK_STATE,
    STUDIO_SCENE_SEQ_STAGING,
    STUDIO_SCENE_SEQ_MSG_1 = 6,
    STUDIO_SCENE_SEQ_MSG_2 = 8,
    STUDIO_SCENE_SEQ_END_EFFECT_1 = 10,
    STUDIO_SCENE_SEQ_END_EFFECT_2 = 12,
    STUDIO_SCENE_SEQ_CHOICE = 14,
    STUDIO_SCENE_SEQ_SELECT_CHOICE,
    STUDIO_SCENE_SEQ_SAVED_CHOICE,
    STUDIO_SCENE_SEQ_CHOICE_MSG_1,
    STUDIO_SCENE_SEQ_CHOICE_MSG_2 = 19,
    STUDIO_SCENE_SEQ_CHECK_TURN = 21,
    STUDIO_SCENE_SEQ_END,
    STUDIO_SCENE_SEQ_WAIT_EFFECT,
    STUDIO_SCENE_SEQ_WAIT_MSG,
    STUDIO_SCENE_SEQ_CUTIN_START_EFFECT_1,
    STUDIO_SCENE_SEQ_CUTIN_START_EFFECT_2,
    STUDIO_SCENE_SEQ_CUTIN_START,
    STUDIO_SCENE_SEQ_CUTIN_MAIN,
    STUDIO_SCENE_SEQ_CUTIN_END_EFFECT_1,
    STUDIO_SCENE_SEQ_CUTIN_END_EFFECT_2,
};

// The steps of Studio_ShowResult: the result's effect, the two effects that start Studio_PlayTurnMessages, and the
// result's two messages
enum {
    STUDIO_RESULT_SEQ_INIT,
    STUDIO_RESULT_SEQ_RESULT_EFFECT,
    STUDIO_RESULT_SEQ_START_EFFECT_1 = 3,
    STUDIO_RESULT_SEQ_START_EFFECT_2,
    STUDIO_RESULT_SEQ_MSG_1,
    STUDIO_RESULT_SEQ_MSG_2,
    STUDIO_RESULT_SEQ_WAIT_MSG,
    STUDIO_RESULT_SEQ_WAIT_EFFECT,
    STUDIO_RESULT_SEQ_END,
};

// The steps of the player's choice of actions, which BattleClient_GetSelectSeq shows the AI: in a Pokestar Studios
// movie, the scene of the turn, the turn's popup and its message and a wait first, then the choice
enum {
    SELECT_ACTION_SEQ_INIT,
    SELECT_ACTION_SEQ_SCENE,
    SELECT_ACTION_SEQ_IDLE,
    SELECT_ACTION_SEQ_POPUP,
    SELECT_ACTION_SEQ_WAIT_POPUP,
    SELECT_ACTION_SEQ_TURN_MSG,
    SELECT_ACTION_SEQ_WAIT_TURN_MSG,
    SELECT_ACTION_SEQ_WAIT,
    SELECT_ACTION_SEQ_START_SELECT,
    SELECT_ACTION_SEQ_SELECT,
};

// The steps of RecPlaySelectActionCore: read the recorded actions, or show that the time ran out or that the
// recording was cut off, then, in a Pokestar Studios movie, its scene, field change and messages of the turn
enum {
    REC_SELECT_SEQ_READ,
    REC_SELECT_SEQ_WAIT_TIME_UP_MSG,
    REC_SELECT_SEQ_WAIT_CANCEL_MSG,
    REC_SELECT_SEQ_STUDIO,
    REC_SELECT_SEQ_STUDIO_SCENE,
    REC_SELECT_SEQ_STUDIO_FIELD,
    REC_SELECT_SEQ_STUDIO_TURN_MSGS,
};

// The steps of BattleClient_ActionSelectRoot: the prompt, then the command menu; a command the Pokémon can't pick
// shows why
enum {
    ROOT_SEQ_INIT,
    ROOT_SEQ_PROMPT,
    ROOT_SEQ_WAIT_PROMPT,
    ROOT_SEQ_START_SELECT,
    ROOT_SEQ_SELECT,
    ROOT_SEQ_WAIT_SCREEN,
    ROOT_SEQ_BLOCKED_RESET_SCREEN,
    ROOT_SEQ_BLOCKED_MSG,
    ROOT_SEQ_WAIT_BLOCKED_MSG,
};

// The steps of BattleClient_ActionSelectFight: the move menu, or the rotation's, then the target in a double or
// triple battle; a move that can't be picked shows why
enum {
    FIGHT_SEQ_INIT,
    FIGHT_SEQ_START_MOVE_SELECT,
    FIGHT_SEQ_START_ROTATION_SELECT,
    FIGHT_SEQ_MOVE_SELECT,
    FIGHT_SEQ_CHECK_TARGET,
    FIGHT_SEQ_START_TARGET_SELECT,
    FIGHT_SEQ_TARGET_SELECT,
    FIGHT_SEQ_WAIT_UNSELECTABLE_MSG,
};

// The steps of BattleClient_ActionSelectEscape: show why the Pokémon can't run and the prompt again, or ask first in
// the run mode that asks, then run
enum {
    ESCAPE_SEQ_INIT,
    ESCAPE_SEQ_WAIT_BLOCKED_MSG,
    ESCAPE_SEQ_WAIT_PROMPT,
    ESCAPE_SEQ_ASK,
    ESCAPE_SEQ_ANSWER,
    ESCAPE_SEQ_ESCAPE,
};

// The steps of AISelectAction: wait for the player's choice to start, then decide each Pokémon's action, running the
// AI's script for its move
enum {
    AI_SELECT_SEQ_WAIT_PLAYER,
    AI_SELECT_SEQ_INIT,
    AI_SELECT_SEQ_DECIDE,
    AI_SELECT_SEQ_THINK,
    AI_SELECT_SEQ_NEXT,
};

// The steps of PlayerSelectChangePoke: the party list, closed when the time to choose runs out, then the
// communication's wait message
enum {
    CHANGE_POKE_SEQ_INIT,
    CHANGE_POKE_SEQ_SELECT,
    CHANGE_POKE_SEQ_QUIT_SELECT,
    CHANGE_POKE_SEQ_HIDE_TIMER,
    CHANGE_POKE_SEQ_SHOW_WAIT_MSG,
    CHANGE_POKE_SEQ_WAIT_SHOWN,
    CHANGE_POKE_SEQ_END,
};

// The steps of BattleClient_PlayerConfirmShiftSwitch: ask whether to switch for the opponent's next Pokémon, and
// pick it
enum {
    SHIFT_SWITCH_SEQ_INIT,
    SHIFT_SWITCH_SEQ_ASK,
    SHIFT_SWITCH_SEQ_ANSWER,
    SHIFT_SWITCH_SEQ_SELECT,
    SHIFT_SWITCH_SEQ_CANCEL,
    SHIFT_SWITCH_SEQ_RETURN,
};

// The steps of BattleClient_ExitTrainerBattle: on a win, each opponent comes on with their message, then the prize
// money; on a loss, the money lost; in a Pokestar Studios movie, its last scene or its result
enum {
    EXIT_TRAINER_SEQ_INIT,
    EXIT_TRAINER_SEQ_TRAINER_1_IN,
    EXIT_TRAINER_SEQ_TRAINER_1_MSG,
    EXIT_TRAINER_SEQ_WAIT_TRAINER_1_MSG,
    EXIT_TRAINER_SEQ_TRAINER_2_IN,
    EXIT_TRAINER_SEQ_TRAINER_2_MSG,
    EXIT_TRAINER_SEQ_WAIT_TRAINER_2_MSG,
    EXIT_TRAINER_SEQ_PRIZE_MONEY,
    EXIT_TRAINER_SEQ_BONUS_MONEY,
    EXIT_TRAINER_SEQ_LOSS_MONEY,
    EXIT_TRAINER_SEQ_LOSE_MSG,
    EXIT_TRAINER_SEQ_STUDIO_SCENE,
    EXIT_TRAINER_SEQ_STUDIO_RESULT,
    EXIT_TRAINER_SEQ_END,
};

// The steps of BattleClient_ServerCmdLoop: take the server's commands, then read and play each
enum {
    SERVER_CMD_SEQ_RECV,
    SERVER_CMD_SEQ_CHECK,
    SERVER_CMD_SEQ_READ,
    SERVER_CMD_SEQ_RUN,
    SERVER_CMD_SEQ_END,
};

// The steps of BattleClient_ScExpGain: add the experience a level at a time, filling the gauge; at each level, the
// effect, the message and its fanfare, the window of the stats, and the moves learned
enum {
    EXP_SEQ_INIT,
    EXP_SEQ_ADD,
    EXP_SEQ_WAIT_GAUGE,
    EXP_SEQ_LEVEL_UP,
    EXP_SEQ_WAIT_GAUGE_LEVEL_UP,
    EXP_SEQ_WAIT_LEVEL_UP_EFFECT,
    EXP_SEQ_LEVEL_UP_MSG,
    EXP_SEQ_WAIT_LEVEL_UP_MSG,
    EXP_SEQ_WAIT_INPUT,
    EXP_SEQ_WAIT_LEVEL_UP_WIN,
    EXP_SEQ_WAIT_LEVEL_UP_STATS,
    EXP_SEQ_WAIT_CLOSE_LEVEL_UP_WIN,
    EXP_SEQ_LEARN_MOVE,
    EXP_SEQ_END,
};

// The steps of BattleClient_LearnMoveSeq: each move of the new level, learned, or, with no free slot, one forgotten
// for it or the move given up
enum {
    LEARN_SEQ_INIT,
    LEARN_SEQ_NEXT_MOVE,
    LEARN_SEQ_LEARNED_MSG,
    LEARN_SEQ_WAIT_LEARNED_MSG,
    LEARN_SEQ_WAIT_FANFARE,
    LEARN_SEQ_ASK_FORGET,
    LEARN_SEQ_WAIT_ASK_FORGET,
    LEARN_SEQ_ANSWER_FORGET,
    LEARN_SEQ_SELECT_FORGET,
    LEARN_SEQ_WAIT_FORGOT_MSG,
    LEARN_SEQ_ASK_GIVE_UP,
    LEARN_SEQ_WAIT_ASK_GIVE_UP,
    LEARN_SEQ_ANSWER_GIVE_UP,
    LEARN_SEQ_WAIT_GAVE_UP_MSG,
};

// What keeps a Pokémon from switching out or running: an opponent's ability, a condition, or nothing
enum {
    TRAP_ABILITY = 0,
    TRAP_CONDITION = 3,
    TRAP_NONE = 4,
};

// How the player left a playback: stopped it, or picked a chapter to skip to
enum {
    REC_PLAYER_RESULT_NONE,
    REC_PLAYER_RESULT_QUIT,
    REC_PLAYER_RESULT_SKIP,
};

// The steps of the playback's controls: take the player's input, fade out once they leave, and stay
enum {
    REC_PLAYER_SEQ_INPUT,
    REC_PLAYER_SEQ_FADE_OUT,
    REC_PLAYER_SEQ_DONE,
};

// The playback of a recorded battle
typedef struct {
    u8 seq;
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

// The kinds of points in a Pokestar Studios movie's score, which index its points and counts: how close the battle's
// turn count came to the movie's, the lead Pokemon's fame, the script entries reached and the player's choices
enum {
    STUDIO_SCORE_TURN,
    STUDIO_SCORE_FAME,
    STUDIO_SCORE_SCENE,
    STUDIO_SCORE_CHOICE,
    STUDIO_SCORE_MAX,
};

// A Pokestar Studios movie's score
typedef struct {
    s16 points[STUDIO_SCORE_MAX];
    s16 unk08;
    s16 unk0A;
    u8 counts[STUDIO_SCORE_MAX];
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

// A step of what a client does for a command, run until it returns TRUE
typedef BOOL (*BtlClientCmdProc)(BtlClient *client, s32 *seq);

// A step of the client's handler of a server command, run until it returns TRUE
typedef BOOL (*BtlClientServerCmdProc)(BtlClient *client, s32 *seq, const u32 *args);

struct BtlClient {
    BtlMainModule *mainModule;
    BtlPokeCon *pokeCon;
    BattleMon *procMon;
    BattleAction *procAction;
    BtlRecorder *recorder;
    BtlRecReader *recReader;
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
    BtlClientCmdProc cmdProc;
    s32 cmdSeq;
    BtlClientCmdProc selActProc;
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
    BtlClientServerCmdProc serverCmdProc;
    s32 serverCmdSeq;
    BtlvPokeListCmd pokeListCmd;
    BtlvPokeSelectParam pokeSelect;
    u16 heapId;
    u16 savedHp;
    u16 hintMsgId;
    u16 reservedItems[3];
    u8 clientId;
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
    u8 recTimeUp : 1;
    u8 recCanceled : 1;
    u8 changePokeCount;
    u8 unk1BC;
    u8 turnCount;
    u8 changePokePos[6];
    BtlClientStudioWork studio;
};

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
static void Studio_InitAudienceTimers(BtlClientStudioWork *studio);
static BOOL BattleClient_SetupView(BtlClient *client, s32 *seq);
static BOOL BattleClient_RecPlaySetupView(BtlClient *client, s32 *seq);
static void BattleClient_SaveFrontMonHp(BtlClient *client);
static u32 BattleClient_GetFrontMonHpLossRatio(BtlClient *client);
static BattleMon *BattleClient_GetSingleBattleFrontMon(BtlClient *client);
static BOOL BattleClient_PlayerSelectRotation(BtlClient *client, s32 *seq);
static u8 BattleClient_PickRotation(u8 dir);
static BOOL BattleClient_RecPlaySelectRotation(BtlClient *client, s32 *seq);
static BOOL BattleClient_AISelectRotation(BtlClient *client, s32 *seq);
static void BattleClient_StartCmdLimitTimer(BtlClient *client);
static BOOL CheckIfOverCmdLimit(BtlClient *client);
static BOOL CheckActionSelectForceQuit(BtlClient *client, BtlClientCmdProc proc);
static void BattleClient_HideCmdLimitTimer(BtlClient *client);
static void BattleClient_SetActionPromptMsg(BtlClient *client, BattleMon *mon, BtlvStringParam *param);
static void BattleClient_SetSelActProc(BtlClient *client, BtlClientCmdProc proc);
static BOOL BattleClient_RunSelActProc(BtlClient *client);
static s32 BattleClient_GetSelectSeq(void);
static u32 Studio_GetTurnMsgId(BtlClient *client);
static void Studio_SetLastChoiceResult(BtlClient *client, u32 value);
static BOOL BattleClient_PlayerSelectActionNormal(BtlClient *client, s32 *seq);
static BOOL BattleClient_PlayerSelectActionStudio(BtlClient *client, s32 *seq);
static BOOL BattleClient_PlayerSelectAction(BtlClient *client, s32 *seq);
static void Studio_SetSeq(BtlClientStudioWork *studio, u8 seq, u8 nextSeq);
static BOOL Studio_PlayTurnMessages(BtlClient *client);
static s32 Studio_FindScene(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn, s32 arg3);
static BOOL Studio_HasChoiceScene(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn);
static void Studio_SetChoiceField(const BtlScriptedRules *rules, BtlClientStudioWork *studio);
static BOOL Studio_IsStateReached(const BtlScriptedRules *rules, BtlClientStudioWork *studio);
static u32 Studio_GetSavedChoice(BtlClient *client, BtlClientStudioWork *studio);
static void Studio_SaveChoice(BtlClient *client, s32 scene, u32 value);
static u32 Studio_GetEnding(BtlClient *client);
static void Studio_SetEnding(BtlClient *client, u32 value);
static void Studio_DecideEnding(BtlClient *client, const BtlScriptedRules *rules, s32 scene);
static void Studio_ScoreEndScene(BtlClient *client, const BtlScriptedRules *rules, s32 scene);
static void Studio_ClearSceneTriggers(BtlClientStudioWork *studio);
static void Studio_StartChoiceSelect(BtlClient *client, const BtlScriptedRules *rules, BtlClientStudioWork *studio);
static void Studio_FreeChoiceStrBufs(BtlClientStudioWork *studio);
static BOOL Studio_PlayScene(BtlClient *client);
static void Studio_RecordChoice(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 scene, u32 value);
static s32 Studio_GetNextState(BtlClientStudioWork *studio, s32 event);
static void Studio_SetStagingWait(BtlClientStudioWork *studio, u8 nextSeq);
static BOOL Studio_UpdateStaging(const BtlScriptedRules *rules, BtlClient *client);
static s16 StudioRules_GetTurnEvent(const BtlScriptedRules *rules, u8 turn);
static BOOL Studio_ChangeField(BtlClient *client);
static u32 Studio_GetResultMsgId(BtlClient *client, const BtlScriptedRules *rules, BtlSetup *setup);
static BOOL Studio_ShowResult(BtlClient *client);
static BOOL RecPlaySelectActionCore(BtlClient *client, s32 *seq, BOOL arg2);
static BOOL RecPlaySelectAction(BtlClient *client, s32 *seq);
static void SetNullReturnAction(BtlClient *client);
static BOOL BattleClient_ActionSelectInit(BtlClient *client, s32 *seq);
static void BattleClient_ShowSelectMsg(BtlClient *client, const BtlvStringParam *param);
static BOOL BattleClient_ActionForceQuit(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectRoot(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionTrainerHint(BtlClient *client, s32 *seq);
static BOOL CheckTrainerHintMsg(BtlClient *client, u16 *msgId);
static BOOL BattleClient_ActionSelectFight(BtlClient *client, s32 *seq);
static void SetupRotationSelectParam(BtlClient *client, BtlvSelectTargetParam *param);
static BOOL BattleClient_ActionViewMoveInfo(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSwitchPokemon(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectItem(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionViewScript(BtlClient *client, s32 *seq);
static BOOL BattleClient_ActionSelectStudioChoice(BtlClient *client, s32 *seq);
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
static void BattleClient_SetupPokeSelect(BtlClient *client, u8 mode, u8 count, BtlvPokeListCmd *cmd,
                                         BtlvPokeSelectParam *select);
static void BattleClient_SetSelectedChangeActions(BtlClient *client, const BtlvPokeSelectParam *select);
static void BattleClient_SetAutoChangeActions(BtlClient *client);
static u8 CollectOwnChangePositions(BtlClient *client, u8 *positions);
static BOOL BattleClient_PlayerConfirmUseNextPoke(BtlClient *client, s32 *seq);
static BOOL BattleClient_PlayerSelectChangePokeFainted(BtlClient *client, s32 *seq);
static BOOL BattleClient_PlayerSelectChangePokeMidTurn(BtlClient *client, s32 *seq);
static BOOL AISelectChangePoke(BtlClient *client, s32 *seq);
static BOOL BattleClient_RecPlaySelectChangePoke(BtlClient *client, s32 *seq);
static BOOL PlayerSelectChangePoke(BtlClient *client, s32 *seq, u8 mode);
static BOOL IsRotationFrontFainted(BtlClient *client);
static u32 RotateToLivingMon(BtlClient *client, BattleAction *action);
static BOOL BattleClient_PlayerConfirmShiftSwitch(BtlClient *client, s32 *seq);
static BOOL BattleClient_StoreRecordData(BtlClient *client, s32 *seq);
static BOOL BattleClient_RecPlayExitCommBattle(BtlClient *client, s32 *seq);
static BOOL BattleClient_ExitCommBattle(BtlClient *client, s32 *seq);
static u32 GetBattleResultForClient(BtlClient *client);
static BOOL BattleClient_RecPlayExitTrainerBattle(BtlClient *client, s32 *seq);
static BOOL BattleClient_ExitTrainerBattle(BtlClient *client, s32 *seq);
static BOOL BattleClient_RecPlayExitFacilityBattle(BtlClient *client, s32 *seq);
static BOOL BattleClient_ExitFacilityBattle(BtlClient *client, s32 *seq);
static void ShowTrainerWinLoseMsg(BtlClient *client, u32 result, u8 which);
static BOOL BattleClient_ExitWildBattleWin(BtlClient *client, s32 *seq);
static BOOL BattleClient_ExitWildBattleLose(BtlClient *client, s32 *seq);
static BOOL BattleClient_ShowTimeUpMsg(BtlClient *client, s32 *seq);
static BOOL BattleClient_RecPlayServerCmdLoop(BtlClient *client, s32 *seq);
static BOOL BattleClient_ServerCmdLoop(BtlClient *client, s32 *seq);
static BOOL BattleClient_ScRecallMsg(BtlClient *client, s32 *seq, const u32 *args);
static u16 BattleClient_GetRecallMsg(BtlClient *client, u8 clientId, BOOL *withName);
static BOOL BattleClient_ScRecall(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSendOut(BtlClient *client, s32 *seq, const u32 *args);
static u16 BattleClient_GetPlayerSendOutMsg(BtlClient *client);
static BOOL BattleClient_ScMsgStd(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScMsgStdSE(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScMsgSet(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScMsgSetSE(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScMoveMsg(BtlClient *client, s32 *seq, const u32 *args);
static s32 BattleClient_GetMoveAnimType(BtlClient *client, u16 move, u8 pos);
static BOOL BattleClient_ScMoveAnim(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSetMonVisible(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScDamage(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScDamagePlural(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOneHitKO(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSetGaugeStatus(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScConfusionDamage(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScFaint(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScShowMon(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScStatDownEffect(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScStatUpEffect(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScWeatherStart(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScWeatherEnd(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScUpdateHpGauge(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScUseHeldItem(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScFaintHpGauge(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSwapPos(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScMoveToCenter(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSubstituteCreate(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSubstituteDelete(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScTransform(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScDamageEffect(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScPlayBgm(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScHideMsgWindow(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScAddEVs(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScExpGain(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ForgetMoveSECallback(u32 arg0);
static BOOL BattleClient_LearnMoveSeq(BtlClient *client, s32 *seq, BattleMon *mon);
static BOOL BattleClient_ScBallThrow(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScBallThrowTrainer(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScRotation(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScChangeAbility(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScSwapAbility(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScIllusionBreak(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScEffect(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScEffectByPos(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScEffectBetweenPos(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScChangeForm(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScAbilityPopupIn(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScAbilityPopupOut(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSubtractHp(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddHp(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSubtractPP(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSubtractOriginalPP(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetMoveUsed(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpZeroHp(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddPP(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddOriginalPP(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpRaiseStatStage(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpLowerStatStage(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetStatStages(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpRecoverStatStages(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpResetStatStages(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddCritStage(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetCondition(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpCureCondition(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpCureMoveCondition(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSendIn(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpChangeType(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpConditionTurnCheck(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpConsumeItem(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpUpdateUsedMove(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetConditionFlag(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpClearConditionFlag(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetTurnFlag(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpClearTurnFlag(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpChangeAbility(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetItem(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpUpdateMove(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpClearForSwitch(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddFieldEffect(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddFieldEffectDependMon(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpRemoveFieldEffectDependMon(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpRemoveFieldEffect(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetCounter(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpBatonPass(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpCreateSubstitute(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpDeleteSubstitute(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddShooterEnergy(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetupIllusion(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpClearConsumedItem(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpCureMonDependentConditions(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpAddDamageRecord(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpTurnCheck(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpFieldTurnCheck(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetBaseStat(BtlClient *client, s32 *seq, const u32 *args);
static BOOL BattleClient_ScOpSetWeight(BtlClient *client, s32 *seq, const u32 *args);
static void BattleClient_LoadAIItems(BtlClient *client);
static u16 AICheckItemUse(BtlClient *client, BattleMon *mon, BattleParty *party);
static BOOL IsXItem(u16 item, BattleMon *mon);
static BOOL IsStatusCureItem(u16 item, BattleMon *mon);
static void RecPlayer_Init(BtlClientRecPlayer *recPlayer);
static void RecPlayer_SetMaxChapter(BtlClientRecPlayer *recPlayer, u32 maxChapter);
static BOOL RecPlayer_IsQuitDone(BtlClientRecPlayer *recPlayer);
static void RecPlayer_ReachChapter(BtlClientRecPlayer *recPlayer);
static u32 RecPlayer_GetResult(BtlClientRecPlayer *recPlayer);
static void RecPlayer_StartSkip(BtlClientRecPlayer *recPlayer, u32 target);
static void RecPlayer_EndSkip(BtlClientRecPlayer *recPlayer);
static BOOL RecPlayer_IsSkipDone(BtlClientRecPlayer *recPlayer);
static u32 RecPlayer_GetSkipTarget(BtlClientRecPlayer *recPlayer);
static BOOL RecPlayer_TryLock(BtlClientRecPlayer *recPlayer);
static void RecPlayer_Unlock(BtlClientRecPlayer *recPlayer);
static void RecPlayer_SetDataEnd(BtlClientRecPlayer *recPlayer);
static void RecPlayer_Update(BtlClient *client, BtlClientRecPlayer *recPlayer);
static void Studio_PlayAudienceSE(BtlClientStudioWork *studio, u32 reaction);
static void Studio_ReactAudience(BtlvCore *core, BtlClientStudioWork *studio, u32 reaction);
static void Studio_AudienceLeave(BtlvCore *core, BtlClientStudioWork *studio, u16 level);
static void Studio_ReactToScenePoints(BtlvCore *core, BtlClientStudioWork *studio);
static void Studio_ReactToFame(BtlvCore *core, BtlClientStudioWork *studio, int fame);
static void Studio_AddFamePoints(BtlClient *client, BtlClientStudioWork *studio, BattleParty *party);
static void StudioScore_AddScenePoints(BtlClient *client, BtlClientStudioScore *score, s32 scene);
static void StudioScore_SetTurnPoints(BtlClient *client, BtlClientStudioScore *score);
static void StudioScore_AddChoicePoints(BtlClient *client, BtlClientStudioScore *score, s32 scene, u32 index);
static s32 StudioScore_CalcTotal(BtlClient *client, BtlClientStudioScore *score);
static void Studio_UpdateAudience(BtlClient *client);
static void Studio_PlayEndingBgm(BtlMainModule *mainModule);

// A server command a client handles, with its handler for each client type
typedef struct {
    u8 cmd;
    u8 unk01;
    BtlClientCmdProc procs[BTL_CLIENT_TYPE_MAX];
} BtlClientCmdEntry;

static const u8 data_ov167_021d6eb4[5] = { 0, 0, 0, 1, 2 };

// 0x021d7070: the commands that the clients handle (also used by the other ranges' handlers)
static const BtlClientCmdEntry sCmdProcTable[15] = {
    { BTL_ACMD_SETUP_VIEW, 1, { BattleClient_SetupView, NULL, BattleClient_RecPlaySetupView } },
    { BTL_ACMD_SELECT_ROTATION,
      0,
      { BattleClient_PlayerSelectRotation, BattleClient_AISelectRotation, BattleClient_RecPlaySelectRotation } },
    { BTL_ACMD_SELECT_ACTION, 0, { BattleClient_PlayerSelectAction, AISelectAction, RecPlaySelectAction } },
    { BTL_ACMD_CONFIRM_NEXT_POKE, 0, { BattleClient_PlayerConfirmUseNextPoke, NULL, NULL } },
    { BTL_ACMD_SELECT_CHANGE_FAINTED,
      0,
      { BattleClient_PlayerSelectChangePokeFainted, AISelectChangePoke, BattleClient_RecPlaySelectChangePoke } },
    { BTL_ACMD_SELECT_CHANGE_MID_TURN,
      0,
      { BattleClient_PlayerSelectChangePokeMidTurn, AISelectChangePoke, BattleClient_RecPlaySelectChangePoke } },
    { BTL_ACMD_CONFIRM_SHIFT_SWITCH,
      0,
      { BattleClient_PlayerConfirmShiftSwitch, NULL, BattleClient_RecPlaySelectChangePoke } },
    { BTL_ACMD_SERVER_CMD, 0, { BattleClient_ServerCmdLoop, NULL, BattleClient_RecPlayServerCmdLoop } },
    { BTL_ACMD_STORE_RECORD, 0, { BattleClient_StoreRecordData, NULL, NULL } },
    { BTL_ACMD_EXIT_TRAINER, 1, { BattleClient_ExitTrainerBattle, NULL, BattleClient_RecPlayExitTrainerBattle } },
    { BTL_ACMD_EXIT_FACILITY, 1, { BattleClient_ExitFacilityBattle, NULL, BattleClient_RecPlayExitFacilityBattle } },
    { BTL_ACMD_EXIT_WILD_WIN, 1, { BattleClient_ExitWildBattleWin, NULL, NULL } },
    { BTL_ACMD_EXIT_WILD_LOSE, 1, { BattleClient_ExitWildBattleLose, NULL, NULL } },
    { BTL_ACMD_EXIT_COMM, 1, { BattleClient_ExitCommBattle, NULL, BattleClient_RecPlayExitCommBattle } },
    { BTL_ACMD_TIME_UP, 1, { BattleClient_ShowTimeUpMsg, NULL, NULL } },
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
BtlClient *BattleClient_Create(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 commMode, NetHandle *netHandle,
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
    client->adapter = BtlAdapter_Create(netHandle, clientId, netFlag, heapId);
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
    client->recTimeUp = 0;
    client->recCanceled = 0;
    client->field = GetBtlField(mainModule);
    client->turnCount = 0;
    client->bagMode = arg7;
    BtlClientIDList_Init(&client->clientIdList);
    RecPlayer_Init(&client->recPlayer);
    BattleClient_LoadAIItems(client);
    client->aiRand = *rand;
    if (client->clientType == BTL_CLIENT_TYPE_AI && recPlay == 0) {
        u32 aiFlags = GetClientAIFlags(client->mainModule, client->clientId);

        client->aiVM = TrAI_CreateVM(client->mainModule, GetServerFlow(client->mainModule), client->pokeCon, aiFlags,
                                     client->heapId);
    } else {
        client->aiVM = NULL;
    }
    for (i = 0; i < 4; i++) {
        client->hintShown[i] = 0;
    }
    type = GetStudioMode(client->mainModule);
    if (client->clientType == BTL_CLIENT_TYPE_PLAYER && IsRecordingEnabled(client->mainModule)) {
        client->recorder = BtlRecorder_Create(heapId, type);
    } else {
        client->recorder = NULL;
    }
    if (type == 2) {
        BtlSetup *setup = GetBattleSetup(client->mainModule);
        client->studio.unk58 = setup->unk128;
        client->studio.audienceMask = setup->unk12C;
    }
    return client;
}

void BattleClient_Delete(BtlClient *client) {
    if (client->recorder != NULL) {
        BtlRecorder_Delete(client->recorder);
    }
    if (client->aiVM != NULL) {
        TrAI_DeleteVM(client->aiVM);
    }
    GFL_HeapFree(client->cmdQueue);
    BtlAdapter_Delete(client->adapter);
    GFL_HeapFree(client);
}

void BattleClient_SetForceQuit(BtlClient *client) {
    client->forceQuit = TRUE;
}

void *BattleClient_GetRecordData(BtlClient *client, u32 *size) {
    if (client->recorder != NULL) {
        return BtlRecorder_GetData(client->recorder, size);
    }
    return NULL;
}

void BattleClient_SetRecPlayData(BtlClient *client, BtlRecReader *reader) {
    client->clientType = BTL_CLIENT_TYPE_REC_PLAY;
    client->recReader = reader;
    RecPlayer_SetMaxChapter(&client->recPlayer, BtlRecReader_CountChapters(reader));
}

void BattleClient_SetViewCore(BtlClient *client, BtlvCore *viewCore) {
    client->viewCore = viewCore;
}

void BattleClient_SetCmdCheckServer(BtlClient *client, BtlServer *server) {
    client->cmdCheckServer = server;
    client->cmdCheckEnable = TRUE;
}

BtlAdapter *BattleClient_GetAdapter(BtlClient *client) {
    return client->adapter;
}

BOOL BattleClient_Main(BtlClient *client) {
    return client->mainProc(client);
}

void BattleClient_StartChapterSkip(BtlClient *client, u32 arg1) {
    BtlRecReader_Reset(client->recReader);
    BtlAdapter_ClearSendCmd(client->adapter);
    RecPlayer_StartSkip(&client->recPlayer, arg1);
    BattleClient_SetMainProc(client, BattleClient_MainProcRecPlay);
}

void BattleClient_EndChapterSkip(BtlClient *client) {
    RecPlayer_EndSkip(&client->recPlayer);
    BattleClient_SetMainProc(client, BattleClient_MainProcNormal);
}

BOOL BattleClient_IsRecPlay(BtlClient *client) {
    if (client->clientType == BTL_CLIENT_TYPE_REC_PLAY) {
        return TRUE;
    }
    return FALSE;
}

u16 BattleClient_GetRecMaxChapter(BtlClient *client) {
    return client->recPlayer.maxChapter;
}

BOOL BattleClient_IsChapterSkipping(BtlClient *client) {
    if (client->mainProc == BattleClient_MainProcRecPlay) {
        return TRUE;
    }
    return FALSE;
}

u32 BattleClient_GetFirstActionMove(BtlClient *client) {
    return client->actions[0].bits.move;
}

void BattleClient_AddShooterEnergy(BtlClient *client, u8 value) {
    client->shooterEnergy += value;
    if (client->shooterEnergy > SHOOTER_ENERGY_MAX) {
        client->shooterEnergy = SHOOTER_ENERGY_MAX;
    }
}

static void BattleClient_SetMainProc(BtlClient *client, BOOL (*mainProc)(BtlClient *client)) {
    client->mainProc = mainProc;
    client->mainSeq = 0;
}

static BOOL BattleClient_MainProcNormal(BtlClient *client) {
    RecPlayer_Update(client, &client->recPlayer);
    switch (client->mainSeq) {
    case CLIENT_MAIN_SEQ_RECV_CMD: {
        u32 cmd = BtlAdapter_GetRecvCmd(client->adapter);

        if (client->clientType == BTL_CLIENT_TYPE_REC_PLAY && (client->recTimeUp || client->recCanceled) && cmd == 0) {
            return FALSE;
        }
        BattleClient_SetDummyReturnData(client);
        if (cmd == BTL_ACMD_QUIT) {
            const BtlClientIDList *list;

            BtlAdapter_GetRecvData(client->adapter, (const void **)&list);
            client->clientIdList = *list;
            client->cmdSeq = 0;
            client->mainSeq = CLIENT_MAIN_SEQ_RETURN_LAST;
        } else if (cmd != 0) {
            u32 unk01;

            client->cmdProc = BattleClient_GetCmdProc(client, cmd, &unk01);
            if (unk01) {
                if (!RecPlayer_TryLock(&client->recPlayer)) {
                    client->mainSeq = CLIENT_MAIN_SEQ_WAIT_REC_QUIT;
                    break;
                }
            } else {
                RecPlayer_Unlock(&client->recPlayer);
            }
            if (client->cmdProc != NULL) {
                client->mainSeq = CLIENT_MAIN_SEQ_RUN_CMD;
                client->cmdSeq = 0;
            } else {
                client->cmdSeq = 0;
                client->mainSeq = CLIENT_MAIN_SEQ_RETURN;
            }
        } else if (client->forceQuit) {
            return TRUE;
        }
        break;
    }
    case CLIENT_MAIN_SEQ_RUN_CMD:
        if (client->cmdProc(client, &client->cmdSeq)) {
            if (client->forceQuit) {
                return TRUE;
            }
            if (RecPlayer_IsQuitDone(&client->recPlayer)) {
                client->mainSeq = CLIENT_MAIN_SEQ_REC_QUIT;
            } else {
                client->mainSeq = CLIENT_MAIN_SEQ_RETURN;
            }
        }
        break;
    case CLIENT_MAIN_SEQ_RETURN:
        if (BtlAdapter_SetReturnData(client->adapter, client->returnData, client->returnDataSize)) {
            client->mainSeq = CLIENT_MAIN_SEQ_RECV_CMD;
        }
        if (client->forceQuit) {
            return TRUE;
        }
        break;
    case CLIENT_MAIN_SEQ_RETURN_LAST:
        if (BtlAdapter_SetReturnData(client->adapter, client->returnData, client->returnDataSize)) {
            client->mainSeq = CLIENT_MAIN_SEQ_END;
        }
        if (client->forceQuit) {
            return TRUE;
        }
        break;
    case CLIENT_MAIN_SEQ_WAIT_REC_QUIT:
        if (RecPlayer_IsQuitDone(&client->recPlayer)) {
            client->mainSeq = CLIENT_MAIN_SEQ_REC_QUIT;
        }
        break;
    case CLIENT_MAIN_SEQ_REC_QUIT:
        if (RecPlayer_GetResult(&client->recPlayer) == REC_PLAYER_RESULT_QUIT) {
            client->mainSeq = CLIENT_MAIN_SEQ_END;
            break;
        }
        if (client->viewCore != NULL) {
            GFL_SndPlayerSetMuteStateEx(1, 0x3e);
            PokeVoice_ResetMasterVolume();
        }
        client->mainSeq = CLIENT_MAIN_SEQ_REC_SKIP;
        break;
    case CLIENT_MAIN_SEQ_REC_SKIP:
        if (!GFL_SndBGMIsFading()) {
            u32 chapter = RecPlayer_GetSkipTarget(&client->recPlayer);

            GFL_SndPlayerSetMuteStateEx(0, 1);
            BtlMain_StartRecSkip(client->mainModule, chapter);
            PokeVoice_SetMasterVolume(0);
        }
        break;
    case CLIENT_MAIN_SEQ_END:
        return TRUE;
    }
    Studio_UpdateAudience(client);
    return FALSE;
}

static BOOL BattleClient_MainProcRecPlay(BtlClient *client) {
    RecPlayer_Update(client, &client->recPlayer);
    switch (client->mainSeq) {
    case CLIENT_SKIP_SEQ_INIT:
        client->mainSeq = CLIENT_SKIP_SEQ_RECV_CMD;
    case CLIENT_SKIP_SEQ_RECV_CMD:
        if (!RecPlayer_IsSkipDone(&client->recPlayer)) {
            u32 cmd = BtlAdapter_GetRecvCmd(client->adapter);

            if (cmd != 0) {
                client->cmdProc = BattleClient_GetCmdProc(client, cmd, NULL);
                if (client->cmdProc != NULL) {
                    client->mainSeq = CLIENT_SKIP_SEQ_RUN_CMD;
                    client->cmdSeq = 0;
                } else {
                    BattleClient_SetDummyReturnData(client);
                    client->mainSeq = CLIENT_SKIP_SEQ_RETURN;
                    client->cmdSeq = 0;
                }
            }
        } else {
            if (client->viewCore != NULL) {
                GFL_SndPlayerSetMuteStateEx(1, 1);
                BtlvCore_StartFadeIn(client->viewCore);
                GFL_SndBGMFadeIn(30);
                BtlvEffect_SetFlag26(0);
                PokeVoice_ResetMasterVolume();
            }
            client->mainSeq = CLIENT_SKIP_SEQ_FADE_IN;
        }
        break;
    case CLIENT_SKIP_SEQ_RUN_CMD:
        if (client->cmdProc(client, &client->cmdSeq)) {
            client->mainSeq = CLIENT_SKIP_SEQ_RETURN;
            client->cmdSeq = 0;
            if (BtlvEffect_GetWork()) {
                BtlvEffect_SetFlag26(1);
            }
        }
        break;
    case CLIENT_SKIP_SEQ_RETURN:
        if (BtlAdapter_SetReturnData(client->adapter, client->returnData, client->returnDataSize)) {
            client->mainSeq = CLIENT_SKIP_SEQ_RECV_CMD;
        }
        break;
    case CLIENT_SKIP_SEQ_FADE_IN:
        if (client->viewCore != NULL && BtlvCore_WaitFadeIn(client->viewCore) && !GFL_SndBGMIsFading()) {
            u16 chapter = RecPlayer_GetSkipTarget(&client->recPlayer);

            BtlvCore_ShowRecorderChapter(client->viewCore, chapter, chapter);
            BtlMain_EndRecSkip(client->mainModule);
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

void BattleClient_GetClientIDList(BtlClient *client, BtlClientIDList *list) {
    *list = client->clientIdList;
}

BOOL BattleClient_IsGameTimeUp(BtlClient *client) {
    if (client->clientType != BTL_CLIENT_TYPE_REC_PLAY) {
        if (client->gameLimitTime != 0) {
            return BtlvEffect_IsTimeUp(0);
        }
        return FALSE;
    }
    return client->recTimeUp;
}

BOOL BattleClient_IsRecPlayCanceled(BtlClient *client) {
    return client->recCanceled;
}

static void Studio_InitAudienceTimers(BtlClientStudioWork *studio) {
    int i;

    for (i = 0; i < 22; i++) {
        if ((studio->audienceMask >> i) & 1) {
            studio->audienceTimers[i] = GFL_RandomLC(300);
        }
    }
}

static BOOL BattleClient_SetupView(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        if (BtlSetup_GetBattleType(client->mainModule) != 4) {
            BtlvCore_StartCommand(client->viewCore, FALSE);
        } else {
            BtlvCore_StartCommand(client->viewCore, TRUE);
        }
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitCommand(client->viewCore)) {
            client->cmdLimitTime = GetCommandTimeLimit(client->mainModule);
            client->gameLimitTime = GetGameTimeLimit(client->mainModule);
            if (client->cmdLimitTime != 0 || client->gameLimitTime != 0) {
                BtlvEffect_CreateTimer(client->gameLimitTime, client->cmdLimitTime);
                if (client->gameLimitTime != 0) {
                    BtlvEffect_SetTimerVisible(0, 1, 1);
                }
            }
            BattleClient_SaveFrontMonHp(client);
            if (BattleClient_IsChapterSkipping(client)) {
                func_ov167_021d0a7c(client->viewCore, RecPlayer_GetSkipTarget(&client->recPlayer));
            }
            client->studio.audienceMask = BtlvCore_GetTypeMask(client->viewCore);
            Studio_InitAudienceTimers(&client->studio);
            Studio_AddFamePoints(client, &client->studio, client->party);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_RecPlaySetupView(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        BOOL done = BattleClient_SetupView(client, seq);

        if (done && BtlRecReader_SkipTurnChunk(client->recReader, client->clientId)) {
            RecPlayer_ReachChapter(&client->recPlayer);
        }
        return done;
    }
    BtlRecReader_SkipTurnChunk(client->recReader, client->clientId);
    return TRUE;
}

static void BattleClient_SaveFrontMonHp(BtlClient *client) {
    BattleMon *mon = BattleClient_GetSingleBattleFrontMon(client);

    if (mon != NULL) {
        client->savedHp = GetBattleMonStat(mon, BATTLEMON_HP);
    }
}

static u32 BattleClient_GetFrontMonHpLossRatio(BtlClient *client) {
    BattleMon *mon = BattleClient_GetSingleBattleFrontMon(client);

    if (mon != NULL) {
        u32 hp = GetBattleMonStat(mon, BATTLEMON_HP);

        if (hp >= client->savedHp) {
            return 0;
        }
        return (client->savedHp - hp) * 100 / hp;
    }
    return 0;
}

static BattleMon *BattleClient_GetSingleBattleFrontMon(BtlClient *client) {
    if (BtlSetup_GetBattleStyle(client->mainModule) == 0 && !GetCommMode(client->mainModule)) {
        u8 pos = GetOpponentClientID(client->mainModule, client->clientId, 0);

        return GetClientMonData(client->pokeCon, pos, 0);
    }
    return NULL;
}

static BOOL BattleClient_PlayerSelectRotation(BtlClient *client, s32 *seq) {
    u8 select;

    switch (*seq) {
    case 0:
        BtlvCore_ShowRotationScreen(client->viewCore, client->unkCC);
        BattleClient_StartCmdLimitTimer(client);
        (*seq)++;
        break;
    case 1:
        if (CheckIfOverCmdLimit(client)) {
            BtlvCore_HideRotationScreen(client->viewCore);
            *seq = 2;
        } else if (BtlvCore_PollRotationInput(client->viewCore, &select)) {
            client->unkCC = select;
            *seq = 3;
        }
        break;
    case 2:
        client->unkCC = BattleClient_PickRotation(client->unkCC);
        (*seq)++;
        break;
    case 3:
        BattleClient_HideCmdLimitTimer(client);
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

static BOOL BattleClient_RecPlaySelectRotation(BtlClient *client, s32 *seq) {
    return TRUE;
}

static BOOL BattleClient_AISelectRotation(BtlClient *client, s32 *seq) {
    client->unkCC = BattleClient_PickRotation(client->unkCC);
    client->returnData = &client->unkCC;
    client->returnDataSize = 1;
    return TRUE;
}

static void BattleClient_StartCmdLimitTimer(BtlClient *client) {
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

static BOOL CheckActionSelectForceQuit(BtlClient *client, BtlClientCmdProc proc) {
    if (CheckIfOverCmdLimit(client)) {
        if (proc != NULL) {
            BattleClient_SetSelActProc(client, proc);
        }
        return TRUE;
    }
    return FALSE;
}

static void BattleClient_HideCmdLimitTimer(BtlClient *client) {
    if (client->cmdLimitTime != 0) {
        BtlvEffect_SetTimerVisible(1, 0, 0);
    }
}

static void BattleClient_SetActionPromptMsg(BtlClient *client, BattleMon *mon, BtlvStringParam *param) {
    if (BtlSetup_GetBattleStyle(client->mainModule) != 3) {
        Btlv_StringParam_Setup(param, 1, 0x45);
        Btlv_StringParam_AddArg(param, GetMonID(mon));
        Btlv_StringParam_SetWait(param, 0);
    } else {
        Btlv_StringParam_Setup(param, 1, 0x46);
        Btlv_StringParam_AddArg(param, client->clientId);
        Btlv_StringParam_SetWait(param, 0);
    }
}

static void BattleClient_SetSelActProc(BtlClient *client, BtlClientCmdProc proc) {
    client->selActProc = proc;
    client->selActSeq = 0;
}

static BOOL BattleClient_RunSelActProc(BtlClient *client) {
    return client->selActProc(client, &client->selActSeq);
}

// The step the action selection is at
static s32 sSelectSeq;

static s32 BattleClient_GetSelectSeq(void) {
    return sSelectSeq;
}

static u32 Studio_GetTurnMsgId(BtlClient *client) {
    BtlSetup *setup = GetBattleSetupParam(client->mainModule);
    const BtlScriptedRules *rules = GetStudioRules(client->mainModule);
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

static void Studio_SetLastChoiceResult(BtlClient *client, u32 value) {
    client->studio.score.unk10 = value;
}

static BOOL BattleClient_PlayerSelectActionNormal(BtlClient *client, s32 *seq) {
    sSelectSeq = *seq;
    switch (*seq) {
    case SELECT_ACTION_SEQ_INIT:
        *seq = SELECT_ACTION_SEQ_START_SELECT;
    case SELECT_ACTION_SEQ_START_SELECT:
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectInit);
        BattleClient_StartCmdLimitTimer(client);
        (*seq)++;
        break;
    case SELECT_ACTION_SEQ_SELECT:
        CheckIfOverCmdLimit(client);
        if (BattleClient_RunSelActProc(client)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_PlayerSelectActionStudio(BtlClient *client, s32 *seq) {
    sSelectSeq = *seq;
    switch (*seq) {
    case SELECT_ACTION_SEQ_INIT:
        client->studio.seq = 0;
        if (client->clientId != 0) {
            *seq = SELECT_ACTION_SEQ_START_SELECT;
            return FALSE;
        }
        if (Studio_HasChoiceScene(GetStudioRules(client->mainModule), &client->studio, client->turnCount)) {
            (*seq)++;
        } else {
            *seq = SELECT_ACTION_SEQ_POPUP;
            Studio_ClearSceneTriggers(&client->studio);
        }
        break;
    case SELECT_ACTION_SEQ_SCENE:
        if (Studio_PlayScene(client)) {
            client->studio.seq = 0;
            *seq = SELECT_ACTION_SEQ_POPUP;
        }
        break;
    case SELECT_ACTION_SEQ_IDLE:
        break;
    case SELECT_ACTION_SEQ_POPUP:
        BtlvCore_ShowClactPopup(client->viewCore, client->turnCount);
        *seq = SELECT_ACTION_SEQ_WAIT_POPUP;
        break;
    case SELECT_ACTION_SEQ_WAIT_POPUP:
        if (!BtlvCore_IsClactPopupShowing(client->viewCore)) {
            *seq = SELECT_ACTION_SEQ_TURN_MSG;
        }
        break;
    case SELECT_ACTION_SEQ_TURN_MSG:
        Btlv_StringParam_Setup(&client->strParam, 9, Studio_GetTurnMsgId(client));
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        *seq = SELECT_ACTION_SEQ_WAIT_TURN_MSG;
        break;
    case SELECT_ACTION_SEQ_WAIT_TURN_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = SELECT_ACTION_SEQ_WAIT;
            client->studio.waitTimer = 30;
        }
        break;
    case SELECT_ACTION_SEQ_WAIT:
        if (--client->studio.waitTimer == 0) {
            *seq = SELECT_ACTION_SEQ_START_SELECT;
        }
        break;
    case SELECT_ACTION_SEQ_START_SELECT: {
        const BtlScriptedRules *rules = GetStudioRules(client->mainModule);

        BtlvCore_SetClactGauge(client->viewCore, rules->turnLimit, client->turnCount);
        client->turnCount++;
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectInit);
        BattleClient_StartCmdLimitTimer(client);
        (*seq)++;
        break;
    }
    case SELECT_ACTION_SEQ_SELECT:
        CheckIfOverCmdLimit(client);
        if (BattleClient_RunSelActProc(client)) {
            client->studio.unk24 = BattleClient_GetFirstActionMove(client);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_PlayerSelectAction(BtlClient *client, s32 *seq) {
    if (GetStudioMode(client->mainModule) == 1) {
        return BattleClient_PlayerSelectActionStudio(client, seq);
    }
    return BattleClient_PlayerSelectActionNormal(client, seq);
}

static void Studio_SetSeq(BtlClientStudioWork *studio, u8 seq, u8 nextSeq) {
    studio->seq = seq;
    studio->nextSeq = nextSeq;
}

static BOOL Studio_PlayTurnMessages(BtlClient *client) {
    const BtlScriptedRules *rules = GetStudioRules(client->mainModule);
    s16 msgId;

    switch (client->studio.seq) {
    case STUDIO_TURN_SEQ_INIT:
        if (rules->unkA8[client->turnCount - 1] == -1 && rules->unkD0[client->turnCount - 1] == -1) {
            client->studio.seq = STUDIO_TURN_SEQ_END;
        } else {
            BtlString_SetStudioWords();
            client->studio.seq++;
        }
        break;
    case STUDIO_TURN_SEQ_START_EFFECT_1:
        BtlvEffect_SetTrainer(
            GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 9, 0, 0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        Studio_SetSeq(&client->studio, STUDIO_TURN_SEQ_WAIT_EFFECT, STUDIO_TURN_SEQ_START_EFFECT_2);
        break;
    case STUDIO_TURN_SEQ_START_EFFECT_2:
        BtlvEffect_Start(0x294);
        Studio_SetSeq(&client->studio, STUDIO_TURN_SEQ_WAIT_EFFECT, STUDIO_TURN_SEQ_MSG_1);
        break;
    case STUDIO_TURN_SEQ_MSG_1:
        msgId = rules->unkA8[client->turnCount - 1];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            client->studio.seq++;
        } else {
            client->studio.seq = STUDIO_TURN_SEQ_MSG_2;
        }
        break;
    case STUDIO_TURN_SEQ_WAIT_MSG_1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            client->studio.seq++;
        }
        break;
    case STUDIO_TURN_SEQ_MSG_2:
        msgId = rules->unkD0[client->turnCount - 1];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            client->studio.seq++;
        } else {
            client->studio.seq = STUDIO_TURN_SEQ_END_EFFECT_1;
        }
        break;
    case STUDIO_TURN_SEQ_WAIT_MSG_2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            client->studio.seq++;
        }
        break;
    case STUDIO_TURN_SEQ_END_EFFECT_1:
        BtlvEffect_Start(0x299);
        Studio_SetSeq(&client->studio, STUDIO_TURN_SEQ_WAIT_EFFECT, STUDIO_TURN_SEQ_END_EFFECT_2);
        break;
    case STUDIO_TURN_SEQ_END_EFFECT_2:
        BtlvEffect_Start(0x295);
        Studio_SetSeq(&client->studio, STUDIO_TURN_SEQ_WAIT_EFFECT, STUDIO_TURN_SEQ_END);
        break;
    case STUDIO_TURN_SEQ_END:
        return TRUE;
    case STUDIO_TURN_SEQ_WAIT_EFFECT:
        if (!BtlvEffect_IsBusy()) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    }
    return FALSE;
}

static s32 Studio_FindScene(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn, s32 arg3) {
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

static BOOL Studio_HasChoiceScene(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 turn) {
    BOOL result = FALSE;
    s32 scene = Studio_FindScene(rules, studio, turn, 0);

    if (scene < 0) {
        return result;
    }
    if (rules->scenes[scene].choices[0].msgId != -1) {
        result = TRUE;
    }
    return result;
}

static void Studio_SetChoiceField(const BtlScriptedRules *rules, BtlClientStudioWork *studio) {
    s16 value = rules->scenes[studio->scene].choices[studio->unk08].unk06;

    if (value != -1) {
        studio->unk0C = value;
    }
}

static BOOL Studio_IsStateReached(const BtlScriptedRules *rules, BtlClientStudioWork *studio) {
    s16 value = rules->scenes[studio->scene].events[studio->unk04];

    if (value != -1 && value != studio->unk2C) {
        return FALSE;
    }
    return TRUE;
}

static u32 Studio_GetSavedChoice(BtlClient *client, BtlClientStudioWork *studio) {
    BtlSetup *setup = GetBattleSetup(client->mainModule);

    return setup->unk110[client->studio.scene];
}

static void Studio_SaveChoice(BtlClient *client, s32 scene, u32 value) {
    BtlSetup *setup = GetBattleSetup(client->mainModule);

    setup->unk110[scene] = value;
}

static u32 Studio_GetEnding(BtlClient *client) {
    BtlSetup *setup = GetBattleSetup(client->mainModule);

    return setup->studioEnding;
}

static void Studio_SetEnding(BtlClient *client, u32 value) {
    BtlSetup *setup = GetBattleSetup(client->mainModule);

    setup->studioEnding = value;
}

static void Studio_DecideEnding(BtlClient *client, const BtlScriptedRules *rules, s32 scene) {
    if (GetStudioMode(client->mainModule) == 1) {
        if (rules->unk0E == scene) {
            Studio_SetEnding(client, 1);
        } else if (rules->scenes[scene].unk0A >= 60) {
            Studio_SetEnding(client, 2);
        } else {
            Studio_SetEnding(client, 0);
        }
    }
}

static void Studio_ScoreEndScene(BtlClient *client, const BtlScriptedRules *rules, s32 scene) {
    int type = rules->scenes[scene].unk00;

    if (type == 100 || type == 101) {
        StudioScore_AddScenePoints(client, &client->studio.score, scene);
        if (GetStudioMode(client->mainModule) == 2) {
            BtlSetup *setup;

            Studio_ReactToScenePoints(client->viewCore, &client->studio);
            setup = GetBattleSetup(client->mainModule);
            setup->unk12C = client->studio.audienceMask;
        }
    }
}

static void Studio_ClearSceneTriggers(BtlClientStudioWork *studio) {
    studio->unk24 = 0;
    studio->unk26 = 0;
}

static void Studio_StartChoiceSelect(BtlClient *client, const BtlScriptedRules *rules, BtlClientStudioWork *studio) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 4; i++) {
        if (rules->scenes[studio->scene].choices[i].msgId != -1) {
            count++;
        }
    }
    BtlvCore_SetupChoice(client->viewCore, count, &studio->unk08);
    for (i = 0; i < count; i++) {
        studio->strBufs[i] = GFL_StrBufCreate(0x28, client->heapId);
        BtlString_FormatMovieScriptMsg(studio->strBufs[i], rules->scenes[studio->scene].choices[i].msgId);
        BtlvCore_SetChoiceStr(client->viewCore, i, studio->strBufs[i]);
    }
    BattleClient_SetSelActProc(client, BattleClient_ActionSelectStudioChoice);
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

static BOOL Studio_PlayScene(BtlClient *client) {
    const BtlScriptedRules *rules = GetStudioRules(client->mainModule);
    BtlMainUnk478 *result = func_ov167_0219e3ac(client->mainModule);
    PokewoodCutin *cutin = GetPokewoodCutin(client->mainModule);
    s16 msgId;

    switch (client->studio.seq) {
    case STUDIO_SCENE_SEQ_INIT:
        client->studio.scene =
            Studio_FindScene(rules, &client->studio, client->turnCount, func_ov167_0219c9e0(client->mainModule));
        Studio_ScoreEndScene(client, rules, client->studio.scene);
        Studio_DecideEnding(client, rules, client->studio.scene);
        BtlString_SetStudioWords();
        if (client->studio.scene >= 0) {
            if (GetStudioMode(client->mainModule) == 2) {
                u32 value = Studio_GetEnding(client);

                if (PokewoodCutin_HasCutin(rules, client->studio.scene, value, ReturnZero(client->mainModule, 11))) {
                    client->studio.seq = STUDIO_SCENE_SEQ_CUTIN_START_EFFECT_1;
                    client->studio.unk0C = -1;
                    break;
                }
            }
            client->studio.unk04 = 0;
            client->studio.unk05 = 0;
            client->studio.unk0C = -1;
            client->studio.seq++;
        } else {
            client->studio.seq = STUDIO_SCENE_SEQ_END;
        }
        break;
    case STUDIO_SCENE_SEQ_CHECK_STATE:
        if (!Studio_IsStateReached(rules, &client->studio)) {
            client->studio.seq = STUDIO_SCENE_SEQ_STAGING;
        } else {
            if ((rules->scenes[client->studio.scene].messages[0][client->studio.unk04] == -1 &&
                 rules->scenes[client->studio.scene].messages[1][client->studio.unk04] == -1) ||
                client->studio.unk04 == 10) {
                client->studio.seq = STUDIO_SCENE_SEQ_CHOICE;
            } else {
                client->studio.seq = STUDIO_SCENE_SEQ_MSG_1;
            }
        }
        break;
    case STUDIO_SCENE_SEQ_STAGING:
        if (Studio_UpdateStaging(rules, client)) {
            client->studio.seq = STUDIO_SCENE_SEQ_CHECK_STATE;
        }
        break;
    case STUDIO_SCENE_SEQ_MSG_1:
        msgId = rules->scenes[client->studio.scene].messages[0][client->studio.unk04];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_MSG, STUDIO_SCENE_SEQ_MSG_2);
        } else {
            client->studio.seq = STUDIO_SCENE_SEQ_MSG_2;
        }
        break;
    case STUDIO_SCENE_SEQ_MSG_2:
        msgId = rules->scenes[client->studio.scene].messages[1][client->studio.unk04];
        if (msgId != -1) {
            Btlv_StringParam_Setup(&client->strParam, 8, msgId);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_MSG, STUDIO_SCENE_SEQ_CHECK_STATE);
        } else {
            client->studio.seq = STUDIO_SCENE_SEQ_CHECK_STATE;
        }
        client->studio.unk04++;
        break;
    case STUDIO_SCENE_SEQ_END_EFFECT_1:
        BtlvEffect_Start(0x299);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_EFFECT, STUDIO_SCENE_SEQ_END_EFFECT_2);
        break;
    case STUDIO_SCENE_SEQ_END_EFFECT_2:
        BtlvEffect_Start(0x295);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_EFFECT, STUDIO_SCENE_SEQ_END);
        client->studio.unk2C = 0;
        break;
    case STUDIO_SCENE_SEQ_CHOICE:
        if (rules->scenes[client->studio.scene].choices[0].msgId != -1) {
            if (GetStudioMode(client->mainModule) == 1) {
                client->studio.seq = STUDIO_SCENE_SEQ_SELECT_CHOICE;
                Studio_StartChoiceSelect(client, rules, &client->studio);
            } else {
                client->studio.seq = STUDIO_SCENE_SEQ_SAVED_CHOICE;
            }
        } else {
            client->studio.seq = STUDIO_SCENE_SEQ_CHECK_TURN;
        }
        break;
    case STUDIO_SCENE_SEQ_SELECT_CHOICE:
        if (BattleClient_RunSelActProc(client)) {
            Studio_FreeChoiceStrBufs(&client->studio);
            Studio_SaveChoice(client, client->studio.scene, client->studio.unk08);
            Studio_RecordChoice(rules, &client->studio, client->studio.scene, client->studio.unk08);
            result->unk08 = rules->scenes[client->studio.scene].choices[client->studio.unk08].unk04;
            StudioScore_AddChoicePoints(client, &client->studio.score, client->studio.scene, client->studio.unk08);
            Studio_SetChoiceField(rules, &client->studio);
            client->studio.seq = STUDIO_SCENE_SEQ_CHOICE_MSG_1;
        }
        break;
    case STUDIO_SCENE_SEQ_SAVED_CHOICE:
        client->studio.unk08 = Studio_GetSavedChoice(client, &client->studio);
        Studio_RecordChoice(rules, &client->studio, client->studio.scene, client->studio.unk08);
        result->unk08 = rules->scenes[client->studio.scene].choices[client->studio.unk08].unk04;
        StudioScore_AddChoicePoints(client, &client->studio.score, client->studio.scene, client->studio.unk08);
        Studio_SetChoiceField(rules, &client->studio);
        client->studio.seq = STUDIO_SCENE_SEQ_CHOICE_MSG_1;
        break;
    case STUDIO_SCENE_SEQ_CHOICE_MSG_1:
        Btlv_StringParam_Setup(&client->strParam, 8,
                               rules->scenes[client->studio.scene].choices[client->studio.unk08].msgId);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_MSG, STUDIO_SCENE_SEQ_CHOICE_MSG_2);
        break;
    case STUDIO_SCENE_SEQ_CHOICE_MSG_2:
        Btlv_StringParam_Setup(&client->strParam, 8,
                               rules->scenes[client->studio.scene].choices[client->studio.unk08].msgId2);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_MSG, STUDIO_SCENE_SEQ_CHECK_TURN);
        break;
    case STUDIO_SCENE_SEQ_CHECK_TURN:
        if (client->turnCount >= 100) {
            client->studio.seq = STUDIO_SCENE_SEQ_END;
            client->studio.unk2C = 0;
        } else {
            client->studio.seq = STUDIO_SCENE_SEQ_END_EFFECT_1;
        }
        break;
    case STUDIO_SCENE_SEQ_END:
        if (client->studio.scene >= 0) {
            client->studio.usedEventMask |= 1 << client->studio.scene;
        }
        Studio_ClearSceneTriggers(&client->studio);
        return TRUE;
    case STUDIO_SCENE_SEQ_WAIT_EFFECT:
        if (!BtlvEffect_IsBusy()) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case STUDIO_SCENE_SEQ_WAIT_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case STUDIO_SCENE_SEQ_CUTIN_START_EFFECT_1:
        BtlvEffect_Start(0x29e);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_EFFECT, STUDIO_SCENE_SEQ_CUTIN_START_EFFECT_2);
        break;
    case STUDIO_SCENE_SEQ_CUTIN_START_EFFECT_2:
        BtlvEffect_Start(0x29f);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_EFFECT, STUDIO_SCENE_SEQ_CUTIN_START);
        break;
    case STUDIO_SCENE_SEQ_CUTIN_START: {
        PlayerInfo *info = GetPlayerInfo(client->mainModule);

        PokewoodCutin_SetViewCore(cutin, client->viewCore);
        PokewoodCutin_Start(cutin, rules, client->studio.scene, Studio_GetSavedChoice(client, &client->studio),
                            getTrainerGender(info));
        client->studio.unk08 = Studio_GetSavedChoice(client, &client->studio);
        Studio_RecordChoice(rules, &client->studio, client->studio.scene, client->studio.unk08);
        Studio_SetChoiceField(rules, &client->studio);
        client->studio.seq++;
        break;
    }
    case STUDIO_SCENE_SEQ_CUTIN_MAIN:
        PokewoodCutin_Main(cutin);
        if (PokewoodCutin_IsDone(cutin)) {
            StudioScore_AddChoicePoints(client, &client->studio.score, client->studio.scene, client->studio.unk08);
            client->studio.seq = STUDIO_SCENE_SEQ_CUTIN_END_EFFECT_1;
        }
        break;
    case STUDIO_SCENE_SEQ_CUTIN_END_EFFECT_1:
        BtlvEffect_Start(0x2a0);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_EFFECT, STUDIO_SCENE_SEQ_CUTIN_END_EFFECT_2);
        break;
    case STUDIO_SCENE_SEQ_CUTIN_END_EFFECT_2:
        BtlvEffect_Start(0x2a1);
        Studio_SetSeq(&client->studio, STUDIO_SCENE_SEQ_WAIT_EFFECT, STUDIO_SCENE_SEQ_END);
        break;
    }
    return FALSE;
}

static void Studio_RecordChoice(const BtlScriptedRules *rules, BtlClientStudioWork *studio, s32 scene, u32 value) {
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

static void Studio_SetStagingWait(BtlClientStudioWork *studio, u8 nextSeq) {
    studio->unk02 = 9;
    studio->unk03 = nextSeq;
}

static BOOL Studio_UpdateStaging(const BtlScriptedRules *rules, BtlClient *client) {
    switch (client->studio.unk02) {
    case 0:
        if (Studio_IsStateReached(rules, &client->studio)) {
            client->studio.unk02 = 10;
        } else {
            client->studio.unk02 =
                Studio_GetNextState(&client->studio, rules->scenes[client->studio.scene].events[client->studio.unk04]);
        }
        break;
    case 1:
        BtlvEffect_SetTrainer(
            GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 9, 0, 0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf) | 0x10;
        break;
    case 2:
        BtlvEffect_Start(0x294);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf0) | 0x01;
        break;
    case 3:
        BtlvEffect_Start(0x299);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = client->studio.unk2C & 0xf;
        break;
    case 4:
        BtlvEffect_Start(0x295);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = client->studio.unk2C & 0xf0;
        break;
    case 5:
        BtlvEffect_Start(0x29a);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf) | 0x20;
        break;
    case 6:
        BtlvEffect_Start(0x29b);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf0) | 0x02;
        break;
    case 7:
        BtlvEffect_SetTrainer(
            GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 9, 0, 0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        BtlvEffect_Start(0x29c);
        Studio_SetStagingWait(&client->studio, 0);
        client->studio.unk2C = (client->studio.unk2C & 0xf) | 0x10;
        break;
    case 8:
        BtlvEffect_Start(0x29d);
        Studio_SetStagingWait(&client->studio, 0);
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

static BOOL Studio_ChangeField(BtlClient *client) {
    const BtlScriptedRules *rules = GetStudioRules(client->mainModule);

    switch (client->studio.seq) {
    case 0:
        if (GetStudioMode(client->mainModule) != 2) {
            return TRUE;
        }
        if (client->studio.unk0C < 0) {
            s16 value = StudioRules_GetTurnEvent(rules, client->turnCount);
            if (value >= 0) {
                client->studio.unk0C = value;
            }
        }
        if (client->studio.unk0C > 0) {
            BtlvEffect_StartPaletteFade(BTLV_EFFECT_FADE_FIELD, 0, 0x10, 0, 0);
            client->studio.seq++;
        } else {
            return TRUE;
        }
        break;
    case 1:
        if (!BtlvEffect_IsPaletteFading(BTLV_EFFECT_FADE_FIELD)) {
            BtlvEffect_SetVanish(BTLV_EFFECT_VANISH_FIELD, TRUE);
            client->studio.seq++;
        }
        break;
    case 2:
        BtlvEffect_ReloadField(client->studio.unk0C);
        BtlvEffect_StartPaletteFade(BTLV_EFFECT_FADE_FIELD, 0x10, 0, 0, 0);
        client->studio.seq++;
        break;
    case 3:
        BtlvEffect_SetVanish(BTLV_EFFECT_VANISH_FIELD, FALSE);
        if (!BtlvEffect_IsPaletteFading(BTLV_EFFECT_FADE_FIELD)) {
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
    s32 scene = Studio_FindScene(rules, &client->studio, client->turnCount, func_ov167_0219c9e0(client->mainModule));
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

static BOOL Studio_ShowResult(BtlClient *client) {
    BtlSetup *setup;
    const BtlScriptedRules *rules;

    switch (client->studio.seq) {
    case STUDIO_RESULT_SEQ_INIT:
        setup = GetBattleSetupParam(client->mainModule);
        rules = GetStudioRules(client->mainModule);
        if (GetBattleResultForClient(client) == BTL_RESULT_VICTORY) {
            client->turnCount = 100;
        } else {
            client->turnCount = 101;
        }
        client->studio.msgId1 = Studio_GetResultMsgId(client, rules, setup);
        client->studio.seq = STUDIO_RESULT_SEQ_RESULT_EFFECT;
        break;
    case STUDIO_RESULT_SEQ_RESULT_EFFECT:
        BtlvEffect_Start(0x2a2);
        Studio_SetSeq(&client->studio, STUDIO_RESULT_SEQ_WAIT_EFFECT, STUDIO_RESULT_SEQ_START_EFFECT_1);
        break;
    case STUDIO_RESULT_SEQ_START_EFFECT_1:
        BtlvEffect_SetTrainer(
            GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 9, 0, 0, 0);
        BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
        BtlvEffect_Start(0x298);
        Studio_SetSeq(&client->studio, STUDIO_RESULT_SEQ_WAIT_EFFECT, STUDIO_RESULT_SEQ_START_EFFECT_2);
        break;
    case STUDIO_RESULT_SEQ_START_EFFECT_2:
        BtlvEffect_Start(0x294);
        Studio_SetSeq(&client->studio, STUDIO_RESULT_SEQ_WAIT_EFFECT, STUDIO_RESULT_SEQ_MSG_1);
        break;
    case STUDIO_RESULT_SEQ_MSG_1:
        if (client->studio.msgId1 >= 0) {
            Btlv_StringParam_Setup(&client->strParam, 9, client->studio.msgId1);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, STUDIO_RESULT_SEQ_WAIT_MSG, STUDIO_RESULT_SEQ_END);
        } else {
            client->studio.seq = STUDIO_RESULT_SEQ_MSG_2;
        }
        break;
    case STUDIO_RESULT_SEQ_MSG_2:
        if (client->studio.msgId2 >= 0) {
            Btlv_StringParam_Setup(&client->strParam, 8, client->studio.msgId2);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            Studio_SetSeq(&client->studio, STUDIO_RESULT_SEQ_WAIT_MSG, STUDIO_RESULT_SEQ_END);
        } else {
            client->studio.seq = STUDIO_RESULT_SEQ_END;
        }
        break;
    case STUDIO_RESULT_SEQ_WAIT_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case STUDIO_RESULT_SEQ_WAIT_EFFECT:
        if (!BtlvEffect_IsBusy()) {
            client->studio.seq = client->studio.nextSeq;
        }
        break;
    case STUDIO_RESULT_SEQ_END:
        return TRUE;
    }
    return FALSE;
}

static BOOL RecPlaySelectActionCore(BtlClient *client, s32 *seq, BOOL arg2) {
    switch (*seq) {
    case REC_SELECT_SEQ_READ: {
        u8 count;
        u8 chapter;
        BattleAction *action;

        if (client->recCanceled) {
            SetNullReturnAction(client);
            return TRUE;
        }
        action = BtlRecReader_ReadActions(client->recReader, client->clientId, &count, &chapter);
        if (chapter) {
            RecPlayer_ReachChapter(&client->recPlayer);
        }
        client->returnData = action;
        client->returnDataSize = count * 4;
        if (action->bits.action == BATTLE_ACTION_REC_TIMEOUT) {
            RecPlayer_SetDataEnd(&client->recPlayer);
            if (client->viewCore != NULL) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0xb7);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
                *seq = REC_SELECT_SEQ_WAIT_TIME_UP_MSG;
            } else {
                SetNullReturnAction(client);
                client->recTimeUp = TRUE;
                return TRUE;
            }
        } else if (action->bits.action == BATTLE_ACTION_REC_ERROR) {
            client->recCanceled = TRUE;
            RecPlayer_SetDataEnd(&client->recPlayer);
            if (client->viewCore != NULL) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0xb8);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
                *seq = REC_SELECT_SEQ_WAIT_CANCEL_MSG;
            } else {
                SetNullReturnAction(client);
                return TRUE;
            }
        } else {
            if (action->bits.action == BATTLE_ACTION_FIGHT && action->bits.target >= 6) {
                client->actions[0].bits.move = action->bits.move;
            }
            *seq = REC_SELECT_SEQ_STUDIO;
        }
        break;
    }
    case REC_SELECT_SEQ_WAIT_TIME_UP_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            SetNullReturnAction(client);
            client->recTimeUp = TRUE;
            return TRUE;
        }
        break;
    case REC_SELECT_SEQ_WAIT_CANCEL_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            SetNullReturnAction(client);
            return TRUE;
        }
        break;
    case REC_SELECT_SEQ_STUDIO:
        if (GetStudioMode(client->mainModule) == 0) {
            client->studio.seq = 0;
            return TRUE;
        }
        if (client->clientId != 0) {
            client->studio.seq = 0;
            return TRUE;
        }
        if (client->viewCore != NULL && arg2) {
            client->studio.seq = 0;
            *seq = REC_SELECT_SEQ_STUDIO_SCENE;
        } else {
            return TRUE;
        }
        break;
    case REC_SELECT_SEQ_STUDIO_SCENE:
        if (Studio_PlayScene(client)) {
            client->studio.seq = 0;
            *seq = REC_SELECT_SEQ_STUDIO_FIELD;
        }
        break;
    case REC_SELECT_SEQ_STUDIO_FIELD:
        if (Studio_ChangeField(client)) {
            client->studio.seq = 0;
            client->turnCount++;
            *seq = REC_SELECT_SEQ_STUDIO_TURN_MSGS;
        }
        break;
    case REC_SELECT_SEQ_STUDIO_TURN_MSGS:
        if (Studio_PlayTurnMessages(client)) {
            client->studio.unk24 = BattleClient_GetFirstActionMove(client);
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
    BattleClient_SetupPokeSelect(client, 0, client->numCoverPos, &client->pokeListCmd, &client->pokeSelect);
    shooterWork_Init(client);
    selItemWork_Init(client);
    for (i = 0; i < 3; i++) {
        BattleAction_SetNull(&client->actions[i]);
    }
    for (i = 0; i < client->numCoverPos; i++) {
        if (!AutoSelectAction(client, PokeCon_GetClientPartyMon(client->pokeCon, client->clientId, i), NULL)) {
            client->firstActionIdx = i;
            break;
        }
    }
    BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
    return FALSE;
}

static void BattleClient_ShowSelectMsg(BtlClient *client, const BtlvStringParam *param) {
    BtlvCore_StartMsg(client->viewCore, param);
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
                client->procMon = PokeCon_GetClientPartyMon(client->pokeCon, client->clientId, client->procActionIdx);
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
    case ROOT_SEQ_INIT:
        client->procMon = PokeCon_GetClientPartyMon(client->pokeCon, client->clientId, client->procActionIdx);
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
    case ROOT_SEQ_PROMPT:
        if (client->prevActionIdx != client->procActionIdx || client->forceActionMsg) {
            BattleClient_SetActionPromptMsg(client, client->procMon, &client->strParam);
            BtlvCore_PrintMsg(client->viewCore, &client->strParam);
            client->forceActionMsg = FALSE;
            client->prevActionIdx = client->procActionIdx;
            (*seq)++;
        } else {
            *seq += 2;
        }
        break;
    case ROOT_SEQ_WAIT_PROMPT:
        if (!BtlvCore_WaitMsg(client->viewCore)) {
            return FALSE;
        }
        (*seq)++;
        // fallthrough
    case ROOT_SEQ_START_SELECT:
        BtlvCore_StartCommandSelect(client->viewCore, client->procMon, client->procActionIdx > client->firstActionIdx,
                                    client->procAction);
        (*seq)++;
        break;
    case ROOT_SEQ_SELECT:
        if (CheckActionSelectForceQuit(client, BattleClient_ActionForceQuit)) {
            BtlvCore_StopCommandSelect(client->viewCore);
            return FALSE;
        }
        switch (BtlvCore_WaitCommandSelect(client->viewCore)) {
        case BTLV_COMMAND_POKEMON:
            if (CheckCondition(client->procMon, CONDITION_SKY_DROP)) {
                BtlvCore_ResetScreen(client->viewCore);
                *seq = ROOT_SEQ_BLOCKED_RESET_SCREEN;
            } else {
                shooterWork_SetCost(client, client->procActionIdx, 0);
                BattleClient_SetSelActProc(client, BattleClient_ActionSwitchPokemon);
            }
            break;
        case BTLV_COMMAND_FIGHT:
            shooterWork_SetCost(client, client->procActionIdx, 0);
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectFight);
            break;
        case BTLV_COMMAND_BAG:
            if (CheckCondition(client->procMon, CONDITION_SKY_DROP)) {
                *seq = ROOT_SEQ_BLOCKED_RESET_SCREEN;
            } else if (client->bagMode == 1 && !func_ov167_0219db08(client->mainModule)) {
                BtlvCore_ResetScreen(client->viewCore);
                *seq = ROOT_SEQ_WAIT_SCREEN;
            } else if (GetStudioMode(client->mainModule) == 1) {
                BattleClient_SetSelActProc(client, BattleClient_ActionViewScript);
            } else {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectItem);
            }
            break;
        case BTLV_COMMAND_RUN:
            if (client->procActionIdx == client->firstActionIdx) {
                if (CheckCondition(client->procMon, CONDITION_SKY_DROP) && GetRunMode(client->mainModule) != 2) {
                    *seq = ROOT_SEQ_BLOCKED_RESET_SCREEN;
                } else {
                    shooterWork_SetCost(client, client->procActionIdx, 0);
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectEscape);
                }
            } else {
                while (client->procActionIdx != 0) {
                    client->procActionIdx--;
                    if (!AutoSelectAction(
                            client, PokeCon_GetClientPartyMon(client->pokeCon, client->clientId, client->procActionIdx),
                            NULL)) {
                        client->shooterEnergy += shooterWork_GetCost(client, client->procActionIdx);
                        if (BattleAction_GetAction(&client->actions[client->procActionIdx]) == BATTLE_ACTION_CHANGE) {
                            PokeSelect_Pop(&client->pokeSelect);
                        }
                        if (BattleAction_GetAction(&client->actions[client->procActionIdx]) == BATTLE_ACTION_ITEM) {
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
    case ROOT_SEQ_WAIT_SCREEN:
        if (BtlvCore_IsScreenIdle(client->viewCore)) {
            *seq = ROOT_SEQ_START_SELECT;
        }
        break;
    case ROOT_SEQ_BLOCKED_RESET_SCREEN:
        BtlvCore_ResetScreenIfShown(client->viewCore);
        (*seq)++;
        break;
    case ROOT_SEQ_BLOCKED_MSG:
        if (BtlvCore_IsScreenIdle(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0xc5);
            Btlv_StringParam_AddArg(&client->strParam, GetMonID(client->procMon));
            BattleClient_ShowSelectMsg(client, &client->strParam);
            (*seq)++;
        }
        break;
    case ROOT_SEQ_WAIT_BLOCKED_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = ROOT_SEQ_PROMPT;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionTrainerHint(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0: {
        u8 clientId = GetPlayerOpponentClientID(client->mainModule, 0);
        u16 trainerId = GetClientTrainerID(client->mainModule, clientId);
        BtlvEffect_SetTrainer(GetClientTrainerClass(client->mainModule, clientId), 9, 0, 0, 0);
        BtlvEffect_Start(0x270);
        BtlvCore_StartMsgTrainer(client->viewCore, trainerId, client->hintMsgId);
        if ((client->hintMsgId == 19 || client->hintMsgId == 20) &&
            BtlCalc_IsGymLeaderClass(GetClientTrainerClass(client->mainModule, clientId)) && !client->unk1BA_0) {
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
        if (!BtlvEffect_IsBusy() && BtlvCore_WaitMsg(client->viewCore)) {
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
    u32 i;
    u8 clientId;
    BattleParty *party;
    BattleMon *mon;
    s32 found;
    u32 count;
    int trainerId;
    u32 msg;

    if (BtlSetup_GetBattleType(client->mainModule) == 1) {
        i = 0;
        clientId = GetPlayerOpponentClientID(client->mainModule, 0);
        trainerId = GetClientTrainerID(client->mainModule, clientId);

        if (trainerId != 0) {
            party = GetPartyData(client->pokeCon, clientId);
            mon = PokeCon_GetClientPartyMon(client->pokeCon, clientId, i);

            if (IsFainted(mon)) {
                count = GetClientBattlerCount(client->mainModule, clientId);
                for (i = 1; i < count; i++) {
                    mon = PokeCon_GetClientPartyMon(client->pokeCon, clientId, i);
                    if (!IsFainted(mon)) {
                        break;
                    }
                }
            }
            found = -1;
            for (i = 0; i < 4; i++) {
                if (client->hintShown[i] == 0) {
                    msg = sTrainerHintMsgs[i];
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
    case FIGHT_SEQ_INIT:
        if (BtlSetup_GetBattleStyle(client->mainModule) != BTL_STYLE_ROTATION) {
            if (CheckForSelectableMove(client, client->procMon, client->procAction)) {
                BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
            } else {
                *seq = FIGHT_SEQ_START_MOVE_SELECT;
            }
        } else {
            SetupRotationSelectParam(client, &client->rotationParam);
            *seq = FIGHT_SEQ_START_ROTATION_SELECT;
        }
        break;
    case FIGHT_SEQ_START_MOVE_SELECT:
        BtlvCore_StartMoveSelect(client->viewCore, client->procMon, client->procAction);
        *seq = FIGHT_SEQ_MOVE_SELECT;
        break;
    case FIGHT_SEQ_START_ROTATION_SELECT:
        BtlvCore_StartRotationSelect(client->viewCore, &client->rotationParam, client->procAction);
        *seq = FIGHT_SEQ_MOVE_SELECT;
        break;
    case FIGHT_SEQ_MOVE_SELECT:
        if (CheckActionSelectForceQuit(client, BattleClient_ActionForceQuit)) {
            BtlvCore_StopMoveSelect(client->viewCore);
            return FALSE;
        }
        if (BtlvCore_WaitMoveSelect(client->viewCore)) {
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
                    pos = GetRotationInSlot(rotatePos);
                    mon = client->rotationParam.mons[pos].mon;
                }
                if (BattleAction_GetMoveInfoFlag(procAction)) {
                    client->moveInfoPos = pos;
                    client->moveInfoIdx = BattleMon_FindMoveSlot(mon, procAction->bits.move);
                    if (client->moveInfoIdx != 4) {
                        BattleClient_SetSelActProc(client, BattleClient_ActionViewMoveInfo);
                        return FALSE;
                    }
                }
                if (IsUnselectableMove(client, mon, procAction->bits.move, &client->strParam)) {
                    Btlv_StringParam_SetWait(&client->strParam, 0xff);
                    BattleClient_ShowSelectMsg(client, &client->strParam);
                    *seq = FIGHT_SEQ_WAIT_UNSELECTABLE_MSG;
                } else {
                    if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION && rotatePos != 0 &&
                        rotatePos != 1 && !IsFainted(mon)) {
                        u16 move = BattleAction_GetFightMove(&client->rotationParam.action);
                        BattleAction_SetRotationParam(client->procAction++, rotatePos);
                        BattleAction_SetFightParam(client->procAction, move, 6);
                        client->extraActionCount++;
                    }
                    *seq = FIGHT_SEQ_CHECK_TARGET;
                }
            }
        }
        break;
    case FIGHT_SEQ_CHECK_TARGET:
        if (BtlCalc_IsDoubleOrTripleStyle(BtlSetup_GetBattleStyle(client->mainModule))) {
            *seq = FIGHT_SEQ_START_TARGET_SELECT;
        } else {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        }
        break;
    case FIGHT_SEQ_START_TARGET_SELECT:
        BtlvCore_StartTargetSelect(client->viewCore, client->procMon, client->procAction);
        *seq = FIGHT_SEQ_TARGET_SELECT;
        break;
    case FIGHT_SEQ_TARGET_SELECT: {
        u32 result;
        if (CheckActionSelectForceQuit(client, BattleClient_ActionForceQuit)) {
            BtlvCore_StopTargetSelect(client->viewCore);
            return FALSE;
        }
        result = BtlvCore_WaitTargetSelect(client->viewCore);
        if (result == BTLV_TARGET_SELECT_CHOSEN) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        } else if (result == BTLV_TARGET_SELECT_BACK) {
            *seq = FIGHT_SEQ_START_MOVE_SELECT;
        }
        break;
    }
    case FIGHT_SEQ_WAIT_UNSELECTABLE_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            BtlvCore_ResumeMoveSelect(client->viewCore);
            *seq = FIGHT_SEQ_MOVE_SELECT;
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
        if (result != TRAP_NONE || monId == GetMonID(client->procMon)) {
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
            if (!PokeSelect_IsSelecting(&client->pokeSelect)) {
                u8 slot = PokeSelect_GetLast(&client->pokeSelect);
                if (slot < 6) {
                    BattleAction_SetChangeParam(client->procAction, client->procActionIdx, slot);
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
            BtlvCore_QuitItemSelect(client->viewCore);
            *seq = 2;
            return FALSE;
        }
        if (BtlvCore_WaitItemSelect(client->viewCore)) {
            u16 item = BtlvCore_GetSelectedItem(client->viewCore);
            u16 target = BtlvCore_GetSelectedPartyIndex(client->viewCore);
            if (item != 0 && target != 6) {
                u8 cost = BtlvCore_GetItemCost(client->viewCore);
                u32 param = BtlvCore_GetSelectedMoveSlot(client->viewCore);
                if (client->shooterEnergy >= cost) {
                    client->shooterEnergy -= cost;
                } else {
                    client->shooterEnergy = 0;
                    cost = client->shooterEnergy;
                }
                shooterWork_SetCost(client, client->procActionIdx, cost);
                BattleAction_SetItemParam(client->procAction, item, target, param);
                BtlvCore_SaveBagCursor(client->viewCore);
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
        if (BtlvCore_WaitItemSelect(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionForceQuit);
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionViewScript(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        BtlvCore_StartScenario(client->viewCore, client->turnCount);
        (*seq)++;
        break;
    case 1:
        if (CheckActionSelectForceQuit(client, NULL)) {
            BtlvCore_QuitScenario(client->viewCore);
            *seq = 2;
            return FALSE;
        }
        if (BtlvCore_WaitScenario(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    case 2:
        if (BtlvCore_WaitScenario(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectStudioChoice(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        BtlvCore_StartChoice(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitChoice(client->viewCore)) {
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
        u32 numFront = GetActivePosCount(client->mainModule);
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
    case ESCAPE_SEQ_INIT: {
        u16 trapAbility;
        u8 trapMonId;

        switch (GetRunMode(client->mainModule)) {
        case 0:
        default:
            if (CheckEscapeBlocked(client, &trapMonId, &trapAbility) == TRAP_NONE) {
                *seq = ESCAPE_SEQ_ESCAPE;
            } else {
                if (trapAbility != 0) {
                    Btlv_StringParam_Setup(&client->strParam, 2, 0x365);
                    Btlv_StringParam_AddArg(&client->strParam, trapMonId);
                    Btlv_StringParam_AddArg(&client->strParam, trapAbility);
                } else {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x4a);
                }
                BattleClient_ShowSelectMsg(client, &client->strParam);
                *seq = ESCAPE_SEQ_WAIT_BLOCKED_MSG;
            }
            break;
        case 1:
            Btlv_StringParam_Setup(&client->strParam, 1, 0x4c);
            BattleClient_ShowSelectMsg(client, &client->strParam);
            *seq = ESCAPE_SEQ_WAIT_BLOCKED_MSG;
            break;
        case 2:
            Btlv_StringParam_Setup(&client->strParam, 1, 0x4d);
            Btlv_StringParam_SetWait(&client->strParam, 0xff);
            BattleClient_ShowSelectMsg(client, &client->strParam);
            *seq = ESCAPE_SEQ_ASK;
            break;
        }
        break;
    }
    case ESCAPE_SEQ_WAIT_BLOCKED_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            BattleClient_SetActionPromptMsg(client, client->procMon, &client->strParam);
            BtlvCore_PrintMsg(client->viewCore, &client->strParam);
            *seq = ESCAPE_SEQ_WAIT_PROMPT;
        }
        break;
    case ESCAPE_SEQ_WAIT_PROMPT:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
        }
        break;
    case ESCAPE_SEQ_ASK:
        BtlvCore_WaitMsg(client->viewCore);
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 3, 8);
            Btlv_StringParam_Setup(&client->strParam2, 3, 9);
            BtlvCore_ShowYesNo(client->viewCore, &client->strParam, &client->strParam2, 1);
            *seq = ESCAPE_SEQ_ANSWER;
        }
        break;
    case ESCAPE_SEQ_ANSWER:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            u32 result;

            if (BtlvCore_PollYesNo(client->viewCore, &result)) {
                if (result == 0) {
                    *seq = ESCAPE_SEQ_ESCAPE;
                } else {
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
                }
            } else if (CheckIfOverCmdLimit(client)) {
                BattleClient_SetSelActProc(client, BattleClient_ActionForceQuit);
            }
        }
        break;
    case ESCAPE_SEQ_ESCAPE:
        BattleAction_SetEscape(client->procAction);
        client->escapeSelected = TRUE;
        BattleClient_SetSelActProc(client, BattleClient_ActionSelectAutoRest);
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ActionSelectAutoRest(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        BtlvCore_ResetScreen(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_IsScreenIdle(client->viewCore)) {
            client->procActionIdx++;
            if (client->procActionIdx < client->numCoverPos) {
                if (!client->escapeSelected) {
                    BattleClient_SetSelActProc(client, BattleClient_ActionSelectRoot);
                    break;
                }
                while (client->procActionIdx < client->numCoverPos) {
                    BattleMon *mon =
                        PokeCon_GetClientPartyMon(client->pokeCon, client->clientId, client->procActionIdx);
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
        if (GetCommMode(client->mainModule)) {
            client->waitMsgShown = TRUE;
            BtlvCore_StartWaitMsg(client->viewCore);
            (*seq)++;
        } else {
            *seq += 2;
        }
        break;
    case 1:
        if (BtlvCore_IsWaitMsgDone(client->viewCore)) {
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            BattleClient_HideCmdLimitTimer(client);
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
    MI_CpuFill8(client->shooterCost, 0, 3);
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
    if (CheckCondition(mon, CONDITION_MOVE_LOCK)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, CONDITION_MOVE_LOCK);
        u16 move = Condition_GetParam(cont);
        u8 target = GetPrevTargetPos(mon);
        if (action != NULL) {
            BattleAction_SetFightParam(action, move, target);
        }
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_CHARGE)) {
        u16 move = GetPreviousMoveID(mon);
        u8 target = GetPrevTargetPos(mon);
        BattleMon_FindMoveSlot(mon, move);
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
    if (CanMonUseHeldItem(client, mon) && CheckCondition(mon, CONDITION_CHOICE_LOCK)) {
        u16 lockedMove = Condition_GetParam(GetConditionContinuationParam(mon, CONDITION_CHOICE_LOCK));
        if (MoveIsUsable(mon, lockedMove) && lockedMove != move) {
            if (param != NULL) {
                Btlv_StringParam_Setup(param, 1, 0x63);
                Btlv_StringParam_AddArg(param, GetBattleMonHeldItem(mon));
                Btlv_StringParam_AddArg(param, lockedMove);
            }
            return TRUE;
        }
    }
    if (CheckCondition(mon, CONDITION_ENCORE)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, CONDITION_ENCORE);
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
    if (GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) != ABILITY_KLUTZ) {
        return TRUE;
    }
    return FALSE;
}

static u32 CanMonSwitch(BtlClient *client, BattleMon *mon, u8 *trapMonId, u16 *trapAbility) {
    u32 result;
    if (CanMonUseHeldItem(client, mon) && GetBattleMonHeldItem(mon) == ITEM_SHED_SHELL) {
        return TRAP_NONE;
    }
    result = IsMonTrapped(client, mon, trapMonId, trapAbility);
    if (result == TRAP_NONE) {
        *trapMonId = 31;
        *trapAbility = 0;
        return TRAP_NONE;
    }
    return result;
}

static u32 CheckEscapeBlocked(BtlClient *client, u8 *trapMonId, u16 *trapAbility) {
    u32 i;
    for (i = 0; i < client->numCoverPos; i++) {
        BattleMon *mon = GetBattleMonFromParty(client->party, i);
        if (!IsFainted(mon)) {
            if (CanMonUseHeldItem(client, mon) && GetBattleMonHeldItem(mon) == ITEM_SMOKE_BALL) {
                return TRAP_NONE;
            }
            if (GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_RUN_AWAY) {
                return TRAP_NONE;
            }
        }
    }
    for (i = 0; i < client->numCoverPos; i++) {
        u32 result = IsMonTrapped(client, GetBattleMonFromParty(client->party, i), trapMonId, trapAbility);
        if (result != TRAP_NONE) {
            return result;
        }
    }
    return TRAP_NONE;
}

static u32 IsMonTrapped(BtlClient *client, BattleMon *mon, u8 *trapMonId, u16 *trapAbility) {
    u8 monIds[4];
    u8 count =
        ExpandPosFlagsToMonIDs(client->mainModule, client->pokeCon,
                               0x100 | MonIDToBattlePos(client->mainModule, client->pokeCon, GetMonID(mon)), monIds);
    u8 i;
    for (i = 0; i < count; i++) {
        BattleMon *opponent = GetPokeParam(client->pokeCon, monIds[i]);
        u16 ability = GetBattleMonStat(opponent, BATTLEMON_EFFECTIVE_ABILITY);
        // The ID is read and dropped here, likely left from a removed debug print
        GetMonID(opponent);
        if (ability == ABILITY_SHADOW_TAG && DoesMonHaveShadowTag(client, mon)) {
            *trapMonId = GetMonID(opponent);
            *trapAbility = ability;
            return TRAP_ABILITY;
        }
        if (ability == ABILITY_ARENA_TRAP && IsMonTrappedByArenaTrap(client, mon)) {
            *trapMonId = GetMonID(opponent);
            *trapAbility = ability;
            return TRAP_ABILITY;
        }
        if (ability == ABILITY_MAGNET_PULL && IsMonSteelType(client, mon)) {
            *trapMonId = GetMonID(opponent);
            *trapAbility = ability;
            return TRAP_ABILITY;
        }
    }
    if (CheckCondition(mon, CONDITION_MEAN_LOOK) || CheckCondition(mon, CONDITION_BIND) ||
        CheckCondition(mon, CONDITION_INGRAIN)) {
        *trapMonId = GetMonID(mon);
        *trapAbility = 0;
        return TRAP_CONDITION;
    }
    return TRAP_NONE;
}

static BOOL DoesMonHaveShadowTag(BtlClient *client, BattleMon *mon) {
    if (GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) != ABILITY_SHADOW_TAG) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IsMonTrappedByArenaTrap(BtlClient *client, BattleMon *mon) {
    BOOL canUseItem = CanMonUseHeldItem(client, mon);
    if (CheckFieldEffect(client->field, FIELD_CONDITION_GRAVITY)) {
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_SMACK_DOWN)) {
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_INGRAIN)) {
        return TRUE;
    }
    if (canUseItem && GetBattleMonHeldItem(mon) == ITEM_IRON_BALL) {
        return TRUE;
    }
    if (GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_LEVITATE) {
        return FALSE;
    }
    if (DoesMonHaveType(mon, TYPE_FLYING)) {
        return FALSE;
    }
    if (CheckCondition(mon, CONDITION_MAGNET_RISE)) {
        return FALSE;
    }
    if (CheckCondition(mon, CONDITION_TELEKINESIS)) {
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

    if (CanMonSwitch(client, mon, &trapMonId, &trapAbility) != TRAP_NONE) {
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
    u8 count = ExpandPosFlagsToMonIDs(client->mainModule, client->pokeCon, 0x100 | pos, monIds);
    if (count != 0) {
        u8 index = MATH_Rand32(&client->aiRand, count);
        return GetPokeParam(client->pokeCon, monIds[index]);
    }
    return NULL;
}

static BOOL ShouldSwitchIfPerishSongLastTurn(BtlClient *client, BattleMon *mon) {
    if (CheckCondition(mon, CONDITION_PERISH_SONG)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, CONDITION_PERISH_SONG);
        u8 turns = Condition_GetTurns(cont);
        u8 count = BattleMon_GetConditionTurnCount(mon, CONDITION_PERISH_SONG);
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
    if (GetBattleMonStat(target, BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_WONDER_GUARD &&
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
            if (BtlCalc_GetTypePairEffectiveness(PML_MoveGetType(move), targetType) != TYPE_EFFECTIVENESS_IMMUNE) {
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
    if (CheckCondition(mon, CONDITION_CHOICE_LOCK)) {
        BattleConditionCont cont = GetConditionContinuationParam(mon, CONDITION_CHOICE_LOCK);
        u16 move = Condition_GetParam(cont);
        if (move != 0) {
            BOOL isDamaging = PML_MoveIsDamaging(move);
            u8 type = PML_MoveGetType(move);
            if (BtlCalc_GetTypePairEffectiveness(type, GetPokeType(target)) == TYPE_EFFECTIVENESS_IMMUNE &&
                isDamaging) {
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

    if (GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_NATURAL_CURE &&
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
                    s32 effectiveness = BtlCalc_GetTypePairEffectiveness(record.type, GetPokeType(candidate));
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
            if (!IsFainted(candidate) && ability == GetBattleMonStat(candidate, BATTLEMON_EFFECTIVE_ABILITY)) {
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
            if (CanPokemonBattle(mon) && BtlCalc_GetTypePairEffectiveness(moveType, GetPokeType(mon)) <= maxEffect) {
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
                BtlCalc_GetTypePairEffectiveness(PML_MoveGetType(move), types) >= minEffect) {
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
    if (!IsFainted(GetPartyMonAt(client, GetRotationInSlot(2)))) {
        positions[count++] = 2;
    }
    if (!IsFainted(GetPartyMonAt(client, GetRotationInSlot(3)))) {
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
    case AI_SELECT_SEQ_WAIT_PLAYER:
        if (GetStudioMode(client->mainModule) == 1) {
            if (BattleClient_GetSelectSeq() >= SELECT_ACTION_SEQ_START_SELECT) {
                *seq = AI_SELECT_SEQ_INIT;
            }
        } else {
            *seq = AI_SELECT_SEQ_INIT;
        }
        break;
    case AI_SELECT_SEQ_INIT:
        ClearAISwitchReserved(client);
        client->procActionIdx = 0;
        client->extraActionCount = 0;
        *seq = AI_SELECT_SEQ_DECIDE;
        // fallthrough
    case AI_SELECT_SEQ_DECIDE:
        if (client->procActionIdx >= client->numCoverPos) {
            break;
        }
        client->procMon = GetBattleMonFromParty(client->party, client->procActionIdx);
        client->procAction = &client->actions[client->procActionIdx];
        if (IsFainted(client->procMon)) {
            BattleAction_SetNull(client->procAction);
            *seq = AI_SELECT_SEQ_NEXT;
            break;
        }
        if (AutoSelectAction(client, client->procMon, client->procAction)) {
            *seq = AI_SELECT_SEQ_NEXT;
            break;
        }
        item = AICheckItemUse(client, client->procMon, client->party);
        if (item != 0) {
            BattleAction_SetItemParam(client->procAction, item, client->procActionIdx, 0);
            *seq = AI_SELECT_SEQ_NEXT;
            break;
        }
        if (BtlSetup_GetBattleType(client->mainModule) != 0 &&
            AISwitchChecks(client, client->procMon, client->procActionIdx, &partyIdx)) {
            BattleAction_SetChangeParam(client->procAction, client->procActionIdx, partyIdx);
            *seq = AI_SELECT_SEQ_NEXT;
            break;
        }
        if (BtlSetup_GetBattleStyle(client->mainModule) == BTL_STYLE_ROTATION) {
            rotatePos = AIPickRotationPos(client);
            if (rotatePos != 1) {
                partyPos = GetRotationInSlot(rotatePos);
                BattleAction_SetRotationParam(client->procAction, rotatePos);
                client->procMon = GetBattleMonFromParty(client->party, partyPos);
                client->extraActionCount = 1;
                client->procAction++;
            }
        }
        if (CheckCondition(client->procMon, CONDITION_MOVE_LOCK)) {
            move = Condition_GetParam(GetConditionContinuationParam(client->procMon, CONDITION_MOVE_LOCK));
            BattleAction_SetFightParam(client->procAction, move,
                                       DecideMoveTargetAutoForClient(client->mainModule, client->pokeCon,
                                                                     client->procMon, move, &client->aiRand));
            *seq = AI_SELECT_SEQ_NEXT;
            break;
        }
        if (CheckForSelectableMove(client, client->procMon, client->procAction)) {
            *seq = AI_SELECT_SEQ_NEXT;
        }
        GetBattleMonMoveCount(client->procMon);
        if (StoreSelectableMoveFlag(client, client->procMon, usableMoves) != 4) {
            TrAI_Setup(client->aiVM, usableMoves,
                       MonIDToBattlePos(client->mainModule, client->pokeCon, GetMonID(client->procMon)));
            *seq = AI_SELECT_SEQ_THINK;
            break;
        }
        SetStruggleAction(client->procAction, client, client->procMon);
        *seq = AI_SELECT_SEQ_NEXT;
        break;
    case AI_SELECT_SEQ_THINK:
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
            *seq = AI_SELECT_SEQ_NEXT;
            break;
        }
        BattleAction_SetEscape(client->procAction);
        *seq = AI_SELECT_SEQ_NEXT;
        break;
    case AI_SELECT_SEQ_NEXT:
        client->procActionIdx++;
        if (client->procActionIdx >= client->numCoverPos) {
            SetReturnData(client, client->actions,
                          (client->numCoverPos + client->extraActionCount) * sizeof(BattleAction));
            client->extraActionCount = 0;
            return TRUE;
        }
        *seq = AI_SELECT_SEQ_DECIDE;
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
                        switch (BtlCalc_GetTypePairEffectiveness(moveType, types)) {
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

static void BattleClient_SetupPokeSelect(BtlClient *client, u8 mode, u8 count, BtlvPokeListCmd *cmd,
                                         BtlvPokeSelectParam *select) {
    PokeListCmd_Init(cmd, client->party, count, mode);
    PokeListCmd_SetCoverSlots(cmd, client->numCoverPos);
    PokeSelect_Init(select, cmd);
}

static void BattleClient_SetSelectedChangeActions(BtlClient *client, const BtlvPokeSelectParam *select) {
    u8 i;

    for (i = 0; i < select->count; i++) {
        BattleAction_SetChangeParam(&client->actions[i], select->position[i], select->partyIndex[i]);
    }
    client->returnData = client->actions;
    client->returnDataSize = select->count * sizeof(BattleAction);
}

static void BattleClient_SetAutoChangeActions(BtlClient *client) {
    u8 start;
    u8 num = 0;
    u8 partyCount = GetNumMonsInParty(client->party);
    u32 i;
    u32 j;
    u8 clientId;
    u8 idx;

    start = GetClientBenchStart(client->mainModule, client->clientId);
    for (i = 0; i < client->changePokeCount; i++) {
        GetPosClientAndIndex(client->mainModule, client->changePokePos[i], &clientId, &idx);
        for (j = start; j < partyCount; j++) {
            if (!IsFainted(GetBattleMonFromParty(client->party, j))) {
                BattleAction_SetChangeParam(&client->actions[num++], idx, j);
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
    u8 size = BtlAdapter_GetRecvData(client->adapter, (const void **)&data);
    u32 i;
    u32 count;

    for (i = 0, count = 0; i < size; i++) {
        if (client->clientId == GetClientIDFromPos(client->mainModule, data[i])) {
            positions[count++] = data[i];
        }
    }
    return count;
}

static BOOL BattleClient_PlayerConfirmUseNextPoke(BtlClient *client, s32 *seq) {
    u32 result;
    u8 yes;

    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 3, 5);
        Btlv_StringParam_Setup(&client->strParam2, 3, 1);
        BtlvCore_ShowYesNo(client->viewCore, &client->strParam, &client->strParam2, 0);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_PollYesNo(client->viewCore, &result)) {
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

static BOOL BattleClient_PlayerSelectChangePokeFainted(BtlClient *client, s32 *seq) {
    return PlayerSelectChangePoke(client, seq, 2);
}

static BOOL BattleClient_PlayerSelectChangePokeMidTurn(BtlClient *client, s32 *seq) {
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
                GetPosClientAndIndex(client->mainModule, client->changePokePos[i], &clientId, &idx);
                BattleAction_SetChangeParam(&client->actions[i], idx, list[i]);
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
            BattleAction_SetChangeDepleteParam(client->actions);
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

static BOOL BattleClient_RecPlaySelectChangePoke(BtlClient *client, s32 *seq) {
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
    case CHANGE_POKE_SEQ_INIT:
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
                        BattleAction_SetChangeParam(&client->actions[0], 0, list[0]);
                        client->returnData = client->actions;
                        client->returnDataSize = sizeof(BattleAction);
                        *seq = CHANGE_POKE_SEQ_SHOW_WAIT_MSG;
                        break;
                    }
                }
                GetPosClientAndIndex(client->mainModule, client->changePokePos[0], &clientId, &idx);
                BattleClient_SetupPokeSelect(client, mode, count, &client->pokeListCmd, &client->pokeSelect);
                BattleClientCmd_StartPokeList(client->viewCore, &client->pokeListCmd, idx, 0, &client->pokeSelect);
                BattleClient_StartCmdLimitTimer(client);
                *seq = CHANGE_POKE_SEQ_SELECT;
            } else {
                if (IsRotationFrontFainted(client)) {
                    size = RotateToLivingMon(client, client->actions);
                    if (size != 0) {
                        client->returnDataSize = size;
                        client->returnData = client->actions;
                        *seq = CHANGE_POKE_SEQ_SHOW_WAIT_MSG;
                        break;
                    }
                }
                BattleAction_SetChangeDepleteParam(client->actions);
                client->returnData = client->actions;
                client->returnDataSize = sizeof(BattleAction);
                *seq = CHANGE_POKE_SEQ_SHOW_WAIT_MSG;
            }
        } else {
            BattleAction_SetNull(client->actions);
            client->returnData = client->actions;
            client->returnDataSize = sizeof(BattleAction);
            *seq = CHANGE_POKE_SEQ_SHOW_WAIT_MSG;
        }
        break;
    case CHANGE_POKE_SEQ_SELECT:
        if (CheckIfOverCmdLimit(client)) {
            BattleClientCmd_QuitPokeSelect(client->viewCore);
            *seq = CHANGE_POKE_SEQ_QUIT_SELECT;
        } else if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            BattleClient_SetSelectedChangeActions(client, &client->pokeSelect);
            *seq = CHANGE_POKE_SEQ_HIDE_TIMER;
        }
        break;
    case CHANGE_POKE_SEQ_QUIT_SELECT:
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            BattleClient_SetAutoChangeActions(client);
            *seq = CHANGE_POKE_SEQ_HIDE_TIMER;
        }
        break;
    case CHANGE_POKE_SEQ_HIDE_TIMER:
        BattleClient_HideCmdLimitTimer(client);
        *seq = CHANGE_POKE_SEQ_SHOW_WAIT_MSG;
        break;
    case CHANGE_POKE_SEQ_SHOW_WAIT_MSG:
        if (GetCommMode(client->mainModule)) {
            client->waitMsgShown = TRUE;
            BtlvCore_StartWaitMsg(client->viewCore);
            *seq = CHANGE_POKE_SEQ_WAIT_SHOWN;
        } else {
            *seq = CHANGE_POKE_SEQ_END;
        }
        break;
    case CHANGE_POKE_SEQ_WAIT_SHOWN:
        if (BtlvCore_IsWaitMsgDone(client->viewCore)) {
            *seq = CHANGE_POKE_SEQ_END;
        }
        break;
    case CHANGE_POKE_SEQ_END:
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
    if (!IsFainted(GetPartyMonAt(client, GetRotationInSlot(2)))) {
        BattleAction_SetRotationParam(action, 2);
        return sizeof(BattleAction);
    }
    if (!IsFainted(GetPartyMonAt(client, GetRotationInSlot(3)))) {
        BattleAction_SetRotationParam(action, 3);
        return sizeof(BattleAction);
    }
    return 0;
}

static BOOL BattleClient_PlayerConfirmShiftSwitch(BtlClient *client, s32 *seq) {
    const u8 *data;
    u32 result;
    u8 partyIdx;

    switch (*seq) {
    case SHIFT_SWITCH_SEQ_INIT:
        BtlAdapter_GetRecvData(client->adapter, (const void **)&data);
        if (*data != 0x1f) {
            Btlv_StringParam_Setup(&client->strParam, 1, 20);
            Btlv_StringParam_AddArg(&client->strParam, 1);
            Btlv_StringParam_AddArg(&client->strParam, *data);
            Btlv_StringParam_SetWait(&client->strParam, 0xff);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            *seq = SHIFT_SWITCH_SEQ_ASK;
        } else {
            *seq = SHIFT_SWITCH_SEQ_CANCEL;
        }
        break;
    case SHIFT_SWITCH_SEQ_ASK:
        BtlvCore_WaitMsg(client->viewCore);
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 3, 6);
            Btlv_StringParam_Setup(&client->strParam2, 3, 7);
            BtlvCore_ShowYesNo(client->viewCore, &client->strParam, &client->strParam2, 1);
            *seq = SHIFT_SWITCH_SEQ_ANSWER;
        }
        break;
    case SHIFT_SWITCH_SEQ_ANSWER:
        if (BtlvCore_WaitMsg(client->viewCore) && BtlvCore_PollYesNo(client->viewCore, &result)) {
            if (result == 0) {
                BattleClient_SetupPokeSelect(client, 0, 1, &client->pokeListCmd, &client->pokeSelect);
                BattleClientCmd_StartPokeList(client->viewCore, &client->pokeListCmd, 0, 0, &client->pokeSelect);
                *seq = SHIFT_SWITCH_SEQ_SELECT;
            } else {
                *seq = SHIFT_SWITCH_SEQ_CANCEL;
            }
        }
        break;
    case SHIFT_SWITCH_SEQ_SELECT:
        if (BattleClientCmd_WaitPokeSelect(client->viewCore)) {
            if (!PokeSelect_IsSelecting(&client->pokeSelect) &&
                (partyIdx = PokeSelect_GetLast(&client->pokeSelect)) != 0 && partyIdx < 6) {
                BattleAction_SetChangeParam(client->actions, 0, partyIdx);
                *seq = SHIFT_SWITCH_SEQ_RETURN;
            } else {
                *seq = SHIFT_SWITCH_SEQ_CANCEL;
            }
        }
        break;
    case SHIFT_SWITCH_SEQ_CANCEL:
        BattleAction_SetNull(client->actions);
        *seq = SHIFT_SWITCH_SEQ_RETURN;
        // fallthrough
    case SHIFT_SWITCH_SEQ_RETURN:
        client->returnData = client->actions;
        client->returnDataSize = sizeof(BattleAction);
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_StoreRecordData(BtlClient *client, s32 *seq) {
    const void *data;
    u16 size = BtlAdapter_GetRecvData(client->adapter, &data);

    if (client->recorder != NULL) {
        BtlRecorder_AddData(client->recorder, data, size);
    }
    if (client->cmdCheckServer != NULL) {
        BattleServer_ReadRecActions(client->cmdCheckServer, data, size);
        client->cmdCheckReq = BtlRec_GetCmdCheckReq(data);
    }
    return TRUE;
}

static BOOL BattleClient_RecPlayExitCommBattle(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return BattleClient_ExitCommBattle(client, seq);
    }
    return TRUE;
}

static BOOL BattleClient_ExitCommBattle(BtlClient *client, s32 *seq) {
    u32 name;
    u32 result;
    u8 isMulti;
    u16 msg;

    switch (*seq) {
    case 0:
        if (client->recCanceled) {
            return TRUE;
        }
        name = GetPlayerOpponentClientID(client->mainModule, 0);
        result = GetBattleResultForClient(client);
        isMulti = IsMultiMode(client->mainModule);
        SetBattleResult(client->mainModule, result);
        switch (result) {
        case BTL_RESULT_VICTORY:
            msg = isMulti ? 0x31 : 0x30;
            break;
        case BTL_RESULT_DEFEAT:
            msg = isMulti ? 0x33 : 0x32;
            break;
        case BTL_RESULT_DRAW:
            msg = isMulti ? 0x35 : 0x34;
            break;
        default:
            return TRUE;
        }
        Btlv_StringParam_Setup(&client->strParam, 1, msg);
        Btlv_StringParam_AddArg(&client->strParam, name);
        if (isMulti) {
            Btlv_StringParam_AddArg(&client->strParam, GetPlayerOpponentClientID(client->mainModule, 1));
        }
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        if (result == 1) {
            GFL_SndBGMPlay(GetWinBGM(client->mainModule), 0xffff);
            (*seq)++;
        } else {
            GFL_SndBGMFadeOut(30);
            *seq += 2;
        }
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            (*seq)++;
        }
        break;
    case 3:
        if (!GFL_SndBGMIsFading()) {
            GFL_SndBGMStopImmediate();
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The battle's result for this client's side: a result for the other side is flipped
static u32 GetBattleResultForClient(BtlClient *client) {
    const u16 *data;

    BtlAdapter_GetRecvData(client->adapter, (const void **)&data);
    switch (data[1]) {
    case 0:
        if (!IsAllyClientID(data[0], client->clientId)) {
            return BTL_RESULT_VICTORY;
        }
        break;
    case 1:
        if (!IsAllyClientID(data[0], client->clientId)) {
            return BTL_RESULT_DEFEAT;
        }
        break;
    }
    return data[1];
}

static BOOL BattleClient_RecPlayExitTrainerBattle(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return BattleClient_ExitTrainerBattle(client, seq);
    }
    return TRUE;
}

static BOOL BattleClient_ExitTrainerBattle(BtlClient *client, s32 *seq) {
    u32 name1;
    u32 name2;
    u32 result;
    u32 value;
    BtlSetup *setup;
    BtlScriptedRules *rules;
    u32 score;

    switch (*seq) {
    case EXIT_TRAINER_SEQ_INIT:
        if (GetStudioMode(client->mainModule) == 0) {
            if (GetBattleResultForClient(client) == BTL_RESULT_VICTORY) {
                BtlvEffect_PlayBgm(GetWinBGM(client->mainModule));
                name1 = GetPlayerOpponentClientID(client->mainModule, 0);
                name2 = GetPlayerOpponentClientID(client->mainModule, 1);
                if (name1 == name2) {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x2c);
                    Btlv_StringParam_AddArg(&client->strParam, name1);
                } else {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x2d);
                    Btlv_StringParam_AddArg(&client->strParam, name1);
                    Btlv_StringParam_AddArg(&client->strParam, name2);
                }
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
                *seq = EXIT_TRAINER_SEQ_TRAINER_1_IN;
            } else if (!BtlSetup_IsBattleType(client->mainModule, 0x100)) {
                name1 = GetPlayerClientID(client->mainModule);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x36);
                Btlv_StringParam_AddArg(&client->strParam, name1);
                Btlv_StringParam_AddArg(&client->strParam, name1);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
                *seq = EXIT_TRAINER_SEQ_LOSS_MONEY;
            } else {
                return TRUE;
            }
        } else if (GetStudioMode(client->mainModule) == 2) {
            result = GetBattleResultForClient(client);
            StudioScore_SetTurnPoints(client, &client->studio.score);
            if (result == 1) {
                client->studio.seq = 0;
                client->turnCount = 100;
            } else {
                client->studio.seq = 0;
                client->turnCount = 101;
            }
            *seq = EXIT_TRAINER_SEQ_STUDIO_SCENE;
            Studio_PlayEndingBgm(client->mainModule);
        } else if (GetStudioMode(client->mainModule) == 1) {
            StudioScore_SetTurnPoints(client, &client->studio.score);
            client->studio.seq = 0;
            *seq = EXIT_TRAINER_SEQ_STUDIO_RESULT;
        }
        break;
    case EXIT_TRAINER_SEQ_TRAINER_1_IN:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            BtlvEffect_SetTrainer(
                GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 0)), 9, 0, 0,
                0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            *seq = EXIT_TRAINER_SEQ_TRAINER_1_MSG;
        }
        break;
    case EXIT_TRAINER_SEQ_TRAINER_1_MSG:
        if (!BtlvEffect_IsBusy()) {
            BtlvCore_StartMsgTrainer(
                client->viewCore,
                GetClientTrainerID(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 0)), 1);
            *seq = EXIT_TRAINER_SEQ_WAIT_TRAINER_1_MSG;
        }
        break;
    case EXIT_TRAINER_SEQ_WAIT_TRAINER_1_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            name1 = GetPlayerOpponentClientID(client->mainModule, 0);
            if (name1 != GetPlayerOpponentClientID(client->mainModule, 1)) {
                BtlvEffect_Start(0x271);
                *seq = EXIT_TRAINER_SEQ_TRAINER_2_IN;
            } else {
                *seq = EXIT_TRAINER_SEQ_PRIZE_MONEY;
            }
        }
        break;
    case EXIT_TRAINER_SEQ_TRAINER_2_IN:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_SetTrainer(
                GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 9, 0, 0,
                0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            *seq = EXIT_TRAINER_SEQ_TRAINER_2_MSG;
        }
        break;
    case EXIT_TRAINER_SEQ_TRAINER_2_MSG:
        if (!BtlvEffect_IsBusy()) {
            BtlvCore_StartMsgTrainer(
                client->viewCore,
                GetClientTrainerID(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 1);
            *seq = EXIT_TRAINER_SEQ_WAIT_TRAINER_2_MSG;
        }
        break;
    case EXIT_TRAINER_SEQ_WAIT_TRAINER_2_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = EXIT_TRAINER_SEQ_PRIZE_MONEY;
        }
        break;
    case EXIT_TRAINER_SEQ_PRIZE_MONEY:
        value = FixPrizeMoney(client->mainModule);
        if (value != 0) {
            name1 = GetPlayerClientID(client->mainModule);
            Btlv_StringParam_Setup(&client->strParam, 1, 0x3a);
            Btlv_StringParam_AddArg(&client->strParam, name1);
            Btlv_StringParam_AddArg(&client->strParam, value);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
        }
        if (!GetBonusMoney(client->mainModule)) {
            *seq = EXIT_TRAINER_SEQ_END;
        } else {
            *seq = EXIT_TRAINER_SEQ_BONUS_MONEY;
        }
        break;
    case EXIT_TRAINER_SEQ_BONUS_MONEY:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            value = GetBonusMoney(client->mainModule);
            if (value != 0) {
                name1 = GetPlayerClientID(client->mainModule);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x3b);
                Btlv_StringParam_AddArg(&client->strParam, name1);
                Btlv_StringParam_AddArg(&client->strParam, value);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
            }
            *seq = EXIT_TRAINER_SEQ_END;
        }
        break;
    case EXIT_TRAINER_SEQ_LOSS_MONEY:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            value = CalcLossMoney(client->mainModule);
            if (value != 0) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0x38);
                Btlv_StringParam_AddArg(&client->strParam, client->clientId);
                Btlv_StringParam_AddArg(&client->strParam, value);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
            }
            *seq = EXIT_TRAINER_SEQ_LOSE_MSG;
        }
        break;
    case EXIT_TRAINER_SEQ_LOSE_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0x39);
            Btlv_StringParam_AddArg(&client->strParam, client->clientId);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            *seq = EXIT_TRAINER_SEQ_END;
        }
        break;
    case EXIT_TRAINER_SEQ_STUDIO_SCENE:
        if (Studio_PlayScene(client)) {
            setup = GetBattleSetup(client->mainModule);
            setup->studioScore = StudioScore_CalcTotal(client, &client->studio.score);
            return TRUE;
        }
        break;
    case EXIT_TRAINER_SEQ_STUDIO_RESULT:
        if (Studio_ShowResult(client)) {
            setup = GetBattleSetup(client->mainModule);
            rules = GetStudioRules(client->mainModule);
            score =
                Studio_FindScene(rules, &client->studio, client->turnCount, func_ov167_0219c9e0(client->mainModule));
            Studio_ScoreEndScene(client, rules, score);
            Studio_DecideEnding(client, rules, score);
            setup->studioScore = StudioScore_CalcTotal(client, &client->studio.score);
            return TRUE;
        }
        break;
    case EXIT_TRAINER_SEQ_END:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_RecPlayExitFacilityBattle(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return BattleClient_ExitFacilityBattle(client, seq);
    }
    return TRUE;
}

static BOOL BattleClient_ExitFacilityBattle(BtlClient *client, s32 *seq) {
    u32 result;

    switch (*seq) {
    case 0:
        if (client->recCanceled) {
            return TRUE;
        }
        result = GetBattleResultForClient(client);
        SetBattleResult(client->mainModule, result);
        if (result == BTL_RESULT_VICTORY) {
            BtlvEffect_PlayBgm(GetWinBGM(client->mainModule));
        }
        if (result <= BTL_RESULT_VICTORY) {
            BtlvEffect_SetTrainer(
                GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 0)), 9, 0, 0,
                0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            ShowTrainerWinLoseMsg(client, result, 0);
            *seq = 1;
            break;
        }
        return TRUE;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvCore_StartMadeMsg(client->viewCore);
            *seq = 2;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            if (GetPlayerOpponentClientID(client->mainModule, 0) == GetPlayerOpponentClientID(client->mainModule, 1)) {
                return TRUE;
            }
            BtlvEffect_Start(0x271);
            *seq = 3;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            result = GetBattleResultForClient(client);
            BtlvEffect_SetTrainer(
                GetClientTrainerClass(client->mainModule, GetPlayerOpponentClientID(client->mainModule, 1)), 9, 0, 0,
                0);
            BtlvMcss_SetAnimation(BtlvEffect_GetMcss(), 9, 1);
            BtlvEffect_Start(0x270);
            ShowTrainerWinLoseMsg(client, result, 1);
            *seq = 4;
        }
        break;
    case 4:
        if (!BtlvEffect_IsBusy()) {
            BtlvCore_StartMadeMsg(client->viewCore);
            *seq = 5;
        }
        break;
    case 5:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Shows a trainer's win or lose message: their easy chat sentence if they have one, else their trainer data's message
static void ShowTrainerWinLoseMsg(BtlClient *client, u32 result, u8 which) {
    u8 clientId = GetPlayerOpponentClientID(client->mainModule, which);
    const PMSData *sentence = GetClientTrainerMsg(client->mainModule, clientId, result);
    StrBuf *str;
    u16 trainerId;

    if (sentence != NULL && PMSData_IsNotEmpty(sentence)) {
        str = PMSData_ToString(sentence, HEAPID_TAIL(client->heapId));
        BtlvCore_CopyMsgStr(client->viewCore, str);
        GFL_StrBufFree(str);
    } else {
        trainerId = GetClientTrainerID(client->mainModule, clientId);
        if (trainerId != 0) {
            Btlv_StringParam_Setup(&client->strParam, 7, 0);
            Btlv_StringParam_AddArg(&client->strParam, trainerId);
            Btlv_StringParam_AddArg(&client->strParam, result);
            BtlvCore_MakeMsg(client->viewCore, &client->strParam);
        }
    }
}

static BOOL BattleClient_ExitWildBattleWin(BtlClient *client, s32 *seq) {
    u32 value;
    u8 playerId;

    switch (*seq) {
    case 0:
        if (!BtlSetup_IsBattleType(client->mainModule, 0x200)) {
            value = GetBonusMoney(client->mainModule);
            if (value != 0) {
                playerId = GetPlayerClientID(client->mainModule);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x3b);
                Btlv_StringParam_AddArg(&client->strParam, playerId);
                Btlv_StringParam_AddArg(&client->strParam, value);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
            }
        }
        (*seq)++;
        // fallthrough
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ExitWildBattleLose(BtlClient *client, s32 *seq) {
    u32 value;

    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 1, 0x36);
        Btlv_StringParam_AddArg(&client->strParam, client->clientId);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            value = CalcLossMoney(client->mainModule);
            if (value != 0) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0x37);
                Btlv_StringParam_AddArg(&client->strParam, client->clientId);
                Btlv_StringParam_AddArg(&client->strParam, value);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
            }
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0x39);
            Btlv_StringParam_AddArg(&client->strParam, client->clientId);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

// A server command and the handler that plays it
typedef struct {
    u32 cmd;
    BtlClientServerCmdProc proc;
} BtlClientServerCmdEntry;

// This part's functions

// The handlers of the server's commands
static const BtlClientServerCmdEntry sServerCmdTable[] = {
    { SC_MSG_STD, BattleClient_ScMsgStd },
    { SC_MSG_STD_SE, BattleClient_ScMsgStdSE },
    { SC_MSG_SET, BattleClient_ScMsgSet },
    { SC_MSG_SET_SE, BattleClient_ScMsgSetSE },
    { SC_MOVE_MSG, BattleClient_ScMoveMsg },
    { SC_MOVE_ANIM, BattleClient_ScMoveAnim },
    { SC_SET_MON_VISIBLE, BattleClient_ScSetMonVisible },
    { SC_DAMAGE, BattleClient_ScDamage },
    { SC_DAMAGE_PLURAL, BattleClient_ScDamagePlural },
    { SC_ONE_HIT_KO, BattleClient_ScOneHitKO },
    { SC_SET_GAUGE_STATUS, BattleClient_ScSetGaugeStatus },
    { SC_CONFUSION_DAMAGE, BattleClient_ScConfusionDamage },
    { SC_FAINT, BattleClient_ScFaint },
    { SC_SHOW_MON, BattleClient_ScShowMon },
    { SC_RECALL_MSG, BattleClient_ScRecallMsg },
    { SC_RECALL, BattleClient_ScRecall },
    { SC_SEND_OUT, BattleClient_ScSendOut },
    { SC_STAT_UP_EFFECT, BattleClient_ScStatUpEffect },
    { SC_STAT_DOWN_EFFECT, BattleClient_ScStatDownEffect },
    { SC_ADD_EVS, BattleClient_ScAddEVs },
    { SC_WEATHER_START, BattleClient_ScWeatherStart },
    { SC_WEATHER_END, BattleClient_ScWeatherEnd },
    { SC_UPDATE_HP_GAUGE, BattleClient_ScUpdateHpGauge },
    { SC_USE_HELD_ITEM, BattleClient_ScUseHeldItem },
    { SC_ABILITY_POPUP_IN, BattleClient_ScAbilityPopupIn },
    { SC_ABILITY_POPUP_OUT, BattleClient_ScAbilityPopupOut },
    { SC_OP_SUBTRACT_HP, BattleClient_ScOpSubtractHp },
    { SC_OP_ADD_HP, BattleClient_ScOpAddHp },
    { SC_OP_ZERO_HP, BattleClient_ScOpZeroHp },
    { SC_OP_SUBTRACT_PP, BattleClient_ScOpSubtractPP },
    { SC_OP_SUBTRACT_ORIGINAL_PP, BattleClient_ScOpSubtractOriginalPP },
    { SC_OP_SET_MOVE_USED, BattleClient_ScOpSetMoveUsed },
    { SC_OP_ADD_PP, BattleClient_ScOpAddPP },
    { SC_OP_ADD_ORIGINAL_PP, BattleClient_ScOpAddOriginalPP },
    { SC_OP_RAISE_STAT_STAGE, BattleClient_ScOpRaiseStatStage },
    { SC_OP_LOWER_STAT_STAGE, BattleClient_ScOpLowerStatStage },
    { SC_OP_SET_STAT_STAGES, BattleClient_ScOpSetStatStages },
    { SC_OP_RECOVER_STAT_STAGES, BattleClient_ScOpRecoverStatStages },
    { SC_OP_RESET_STAT_STAGES, BattleClient_ScOpResetStatStages },
    { SC_OP_ADD_CRIT_STAGE, BattleClient_ScOpAddCritStage },
    { SC_OP_SET_CONDITION, BattleClient_ScOpSetCondition },
    { SC_OP_CURE_CONDITION, BattleClient_ScOpCureCondition },
    { SC_OP_CURE_MOVE_CONDITION, BattleClient_ScOpCureMoveCondition },
    { SC_OP_SEND_IN, BattleClient_ScOpSendIn },
    { SC_OP_SET_BASE_STAT, BattleClient_ScOpSetBaseStat },
    { SC_OP_SET_WEIGHT, BattleClient_ScOpSetWeight },
    { SC_OP_CHANGE_TYPE, BattleClient_ScOpChangeType },
    { SC_OP_CONDITION_TURN_CHECK, BattleClient_ScOpConditionTurnCheck },
    { SC_OP_CONSUME_ITEM, BattleClient_ScOpConsumeItem },
    { SC_OP_UPDATE_USED_MOVE, BattleClient_ScOpUpdateUsedMove },
    { SC_OP_SET_CONDITION_FLAG, BattleClient_ScOpSetConditionFlag },
    { SC_OP_CLEAR_CONDITION_FLAG, BattleClient_ScOpClearConditionFlag },
    { SC_OP_SET_TURN_FLAG, BattleClient_ScOpSetTurnFlag },
    { SC_OP_CLEAR_TURN_FLAG, BattleClient_ScOpClearTurnFlag },
    { SC_OP_CHANGE_ABILITY, BattleClient_ScOpChangeAbility },
    { SC_OP_SET_ITEM, BattleClient_ScOpSetItem },
    { SC_OP_UPDATE_MOVE, BattleClient_ScOpUpdateMove },
    { SC_OP_CLEAR_FOR_SWITCH, BattleClient_ScOpClearForSwitch },
    { SC_OP_ADD_FIELD_EFFECT, BattleClient_ScOpAddFieldEffect },
    { SC_OP_ADD_FIELD_EFFECT_DEPEND_MON, BattleClient_ScOpAddFieldEffectDependMon },
    { SC_OP_REMOVE_FIELD_EFFECT_DEPEND_MON, BattleClient_ScOpRemoveFieldEffectDependMon },
    { SC_OP_REMOVE_FIELD_EFFECT, BattleClient_ScOpRemoveFieldEffect },
    { SC_OP_SET_COUNTER, BattleClient_ScOpSetCounter },
    { SC_OP_BATON_PASS, BattleClient_ScOpBatonPass },
    { SC_OP_CREATE_SUBSTITUTE, BattleClient_ScOpCreateSubstitute },
    { SC_OP_DELETE_SUBSTITUTE, BattleClient_ScOpDeleteSubstitute },
    { SC_OP_ADD_SHOOTER_ENERGY, BattleClient_ScOpAddShooterEnergy },
    { SC_OP_SETUP_ILLUSION, BattleClient_ScOpSetupIllusion },
    { SC_OP_CLEAR_CONSUMED_ITEM, BattleClient_ScOpClearConsumedItem },
    { SC_OP_CURE_MON_DEPENDENT_CONDITIONS, BattleClient_ScOpCureMonDependentConditions },
    { SC_OP_ADD_DAMAGE_RECORD, BattleClient_ScOpAddDamageRecord },
    { SC_OP_TURN_CHECK, BattleClient_ScOpTurnCheck },
    { SC_OP_FIELD_TURN_CHECK, BattleClient_ScOpFieldTurnCheck },
    { SC_FAINT_HP_GAUGE, BattleClient_ScFaintHpGauge },
    { SC_SWAP_POS, BattleClient_ScSwapPos },
    { SC_EXP_GAIN, BattleClient_ScExpGain },
    { SC_BALL_THROW, BattleClient_ScBallThrow },
    { SC_BALL_THROW_TRAINER, BattleClient_ScBallThrowTrainer },
    { SC_ROTATION, BattleClient_ScRotation },
    { SC_CHANGE_ABILITY, BattleClient_ScChangeAbility },
    { SC_SWAP_ABILITY, BattleClient_ScSwapAbility },
    { SC_ILLUSION_BREAK, BattleClient_ScIllusionBreak },
    { SC_EFFECT, BattleClient_ScEffect },
    { SC_EFFECT_BY_POS, BattleClient_ScEffectByPos },
    { SC_EFFECT_BETWEEN_POS, BattleClient_ScEffectBetweenPos },
    { SC_CHANGE_FORM, BattleClient_ScChangeForm },
    { SC_MOVE_TO_CENTER, BattleClient_ScMoveToCenter },
    { SC_SUBSTITUTE_CREATE, BattleClient_ScSubstituteCreate },
    { SC_SUBSTITUTE_DELETE, BattleClient_ScSubstituteDelete },
    { SC_TRANSFORM, BattleClient_ScTransform },
    { SC_DAMAGE_EFFECT, BattleClient_ScDamageEffect },
    { SC_PLAY_BGM, BattleClient_ScPlayBgm },
    { SC_HIDE_MSG_WINDOW, BattleClient_ScHideMsgWindow },
};

// The message and sound effect of each weather's start
static const struct {
    u16 msg;
    u16 se;
} sWeatherStartTable[] = {
    { 0x54, 0x26b }, { 0x54, 0x26b }, { 0x55, 0x268 }, { 0x57, 0x269 }, { 0x56, 0x26a },
};

// The effort values that the effort value command raises, in its arguments' order
static BOOL BattleClient_ShowTimeUpMsg(BtlClient *client, s32 *seq) {
    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 1, 0xb7);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            (*seq)++;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_RecPlayServerCmdLoop(BtlClient *client, s32 *seq) {
    if (client->viewCore != NULL) {
        return BattleClient_ServerCmdLoop(client, seq);
    }
    return TRUE;
}

static BOOL BattleClient_ServerCmdLoop(BtlClient *client, s32 *seq) {
    while (TRUE) {
        switch (*seq) {
        case SERVER_CMD_SEQ_RECV: {
            const void *data;
            u32 size;

            size = BtlAdapter_GetRecvData(client->adapter, &data);
            if (client->cmdCheckServer != NULL && client->cmdCheckReq && client->cmdCheckEnable) {
                if (BattleServer_CheckCmdMismatch(client->cmdCheckServer, client->cmdCheckReq, data, size)) {
                    SetCmdCheckError(client->mainModule);
                    client->cmdCheckEnable = FALSE;
                }
                client->cmdCheckReq = 0;
            }
            BtlServerCmdQueue_Setup(client->cmdQueue, data, size);
            if (client->waitMsgShown) {
                client->waitMsgShown = FALSE;
                BtlvCore_EndWaitMsg(client->viewCore);
            }
            (*seq)++;
        }
        case SERVER_CMD_SEQ_CHECK:
            if (BtlServerCmdQueue_IsEmpty(client->cmdQueue)) {
                *seq = SERVER_CMD_SEQ_END;
                return TRUE;
            }
            if (RecPlayer_IsQuitDone(&client->recPlayer)) {
                return TRUE;
            }
            (*seq)++;
        case SERVER_CMD_SEQ_READ: {
            u32 i;

            client->serverCmd = BtlServerCmdQueue_Read(client->cmdQueue, client->cmdArgs);
            if (client->serverCmd == SC_NONE) {
                return TRUE;
            }
            for (i = 0; i < NELEMS(sServerCmdTable); i++) {
                if (client->serverCmd == sServerCmdTable[i].cmd) {
                    break;
                }
            }
            if (i == NELEMS(sServerCmdTable)) {
                *seq = SERVER_CMD_SEQ_CHECK;
                continue;
            }
            client->serverCmdProc = sServerCmdTable[i].proc;
            client->serverCmdSeq = 0;
            (*seq)++;
        }
        case SERVER_CMD_SEQ_RUN:
            if (client->serverCmdProc(client, &client->serverCmdSeq, client->cmdArgs)) {
                *seq = SERVER_CMD_SEQ_CHECK;
                continue;
            }
            break;
        case SERVER_CMD_SEQ_END:
            return TRUE;
        }
        return FALSE;
    }
}

static BOOL BattleClient_ScRecallMsg(BtlClient *client, s32 *seq, const u32 *args) {
    BOOL withName;

    switch (*seq) {
    case 0:
        Btlv_StringParam_Setup(&client->strParam, 1, BattleClient_GetRecallMsg(client, args[0], &withName));
        if (withName) {
            Btlv_StringParam_AddArg(&client->strParam, args[0]);
        }
        Btlv_StringParam_AddArg(&client->strParam, args[1]);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static u16 BattleClient_GetRecallMsg(BtlClient *client, u8 clientId, BOOL *withName) {
    if (clientId == client->clientId) {
        *withName = FALSE;
        if (BtlSetup_GetBattleStyle(client->mainModule) == 0 && !GetCommMode(client->mainModule)) {
            u32 ratio = BattleClient_GetFrontMonHpLossRatio(client);

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
        if (IsClientNPCTrainer(client->mainModule, clientId)) {
            return 0x1e;
        }
        return 0x1f;
    }
}

static BOOL BattleClient_ScRecall(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartSwitchOut(client->viewCore, BattlePosToViewPos(client->mainModule, args[0]), args[1]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitSwitchOut(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScSendOut(BtlClient *client, s32 *seq, const u32 *args) {
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
                    BtlvCore_StartMsg(client->viewCore, &client->strParam);
                } else {
                    Btlv_StringParam_Setup(&client->strParam, 1, 0x11);
                    Btlv_StringParam_AddArg(&client->strParam, clientId);
                    Btlv_StringParam_AddArg(&client->strParam, monId);
                    BtlvCore_StartMsg(client->viewCore, &client->strParam);
                }
            } else if (IsClientNPCTrainer(client->mainModule, clientId)) {
                Btlv_StringParam_Setup(&client->strParam, 1, 0xe);
                Btlv_StringParam_AddArg(&client->strParam, clientId);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
            } else {
                Btlv_StringParam_Setup(&client->strParam, 1, 0x11);
                Btlv_StringParam_AddArg(&client->strParam, clientId);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
            }
        }
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            BtlvCore_StartSwitchIn(client->viewCore, GetClientBattlePos(client->mainModule, clientId, slot), clientId,
                                   slot);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitSwitchIn(client->viewCore)) {
            BattleClient_SaveFrontMonHp(client);
            if (clientId == GetPlayerClientID(client->mainModule)) {
                Studio_AddFamePoints(client, &client->studio, client->party);
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static u16 BattleClient_GetPlayerSendOutMsg(BtlClient *client) {
    if (BtlSetup_GetBattleStyle(client->mainModule) == 0) {
        u8 clientId = GetOpponentClientID(client->mainModule, client->clientId, 0);
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

static BOOL BattleClient_ScMsgStd(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvCore_StartMsgStd(client->viewCore, args[0], &args[1]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScMsgStdSE(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvCore_StartMsgStd(client->viewCore, args[0], &args[2]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            GFL_SndSEPlay(args[1]);
        }
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScMsgSet(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvCore_StartMsgSet(client->viewCore, args[0], &args[1]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScMsgSetSE(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvCore_StartMsgSet(client->viewCore, args[0], &args[2]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            GFL_SndSEPlay(args[1]);
        }
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScMoveMsg(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvCore_StartMsgMove(client->viewCore, args[0], args[1]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static s32 BattleClient_GetMoveAnimType(BtlClient *client, u16 move, u8 pos) {
    if (move != 0xae) {
        return PML_MoveGetParam(move, MOVE_PARAM_TARGET);
    } else {
        BattleMon *mon = PokeCon_GetPosMonInner(client->pokeCon, pos);

        if (mon != NULL) {
            return BtlCalc_GetCurseTarget(mon);
        }
        return PML_MoveGetParam(move, MOVE_PARAM_TARGET);
    }
}

static BOOL BattleClient_ScMoveAnim(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        if (IsBattleSceneOff(client->mainModule)) {
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
            BtlvCore_StartMoveEffect(client->viewCore, attackerPos, targetPos, move, arg4, arg3, 0);
            (*seq)++;
            break;
        }
        return TRUE;
    case 1:
        if (BtlvCore_WaitMoveEffect(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScSetMonVisible(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 pos = MonIDToViewPos(client->mainModule, client->pokeCon, args[0]);

        BtlvCore_SetMonVisible(client->viewCore, pos, args[1] ? TRUE : FALSE);
        (*seq)++;
        break;
    }
    case 1:
        return !BtlvEffect_IsBusy();
    }
    return FALSE;
}

static BOOL BattleClient_ScDamage(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartDamage(client->viewCore, args[2], MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]),
                             args[1]);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitDamage(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScDamagePlural(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u16 count = args[0];
        u32 type = args[1];
        u16 arg2 = args[2];
        u8 monIds[6];
        u8 i;

        for (i = 0; i < count; i++) {
            monIds[i] = BtlServerCmdQueue_ReadByte(client->cmdQueue);
        }
        BtlvCore_StartMultiDamage(client->viewCore, count, type, monIds, arg2);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitMultiDamage(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScOneHitKO(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartGaugeHP(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitGaugeHP(client->viewCore)) {
            BtlvCore_StartMsgStd(client->viewCore, 0x61, args);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScSetGaugeStatus(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_SetGaugeStatus(
            args[1],
            BattlePosToViewPos(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0])));
        (*seq)++;
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScConfusionDamage(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos;

    switch (*seq) {
    case 0:
        if (!BattleClient_IsChapterSkipping(client)) {
            pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);
            BtlvCore_StartMoveEffect(client->viewCore, pos, pos, 1, PML_MoveGetParam(1, MOVE_PARAM_TARGET), 0, 0);
        }
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitMoveEffect(client->viewCore)) {
            pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);
            if (!BattleClient_IsChapterSkipping(client)) {
                GFL_SndSEPlay(0x564);
            }
            BtlvCore_StartDamage(client->viewCore, 1, pos, 1);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitDamage(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScFaint(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartFaint(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        client->studio.unk26 = GetBattleMonSpecies(GetPokeParamConst(client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitFaint(client->viewCore)) {
            Clear_ForFainted(GetPokeParam(client->pokeCon, args[0]));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScShowMon(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartShowMon(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitShowMon(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScStatDownEffect(BtlClient *client, s32 *seq, const u32 *args) {
    u8 viewPos = BattlePosToViewPos(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));

    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        if (!IsBattleSceneOff(client->mainModule)) {
            return TRUE;
        }
        BtlvCore_StartRankDownEffect(client->viewCore, viewPos);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitRankEffect(client->viewCore, viewPos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScStatUpEffect(BtlClient *client, s32 *seq, const u32 *args) {
    u8 viewPos = BattlePosToViewPos(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));

    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        if (!IsBattleSceneOff(client->mainModule)) {
            return TRUE;
        }
        BtlvCore_StartRankUpEffect(client->viewCore, viewPos);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitRankEffect(client->viewCore, viewPos)) {
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
        BtlField_SetWeather(client->field, weather, args[1]);
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        if (weather < NELEMS(sWeatherStartTable)) {
            BtlvEffect_Start(sWeatherStartTable[weather].se);
        }
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvCore_StartMsgStd(client->viewCore, sWeatherStartTable[weather].msg, NULL);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
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
        BtlvCore_StartMsgStd(client->viewCore, msg, NULL);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            BtlField_ClearWeather(client->field);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScUpdateHpGauge(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartGaugeHP(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitGaugeHP(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScUseHeldItem(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        if (!IsBattleSceneOff(client->mainModule)) {
            return TRUE;
        }
        BtlvCore_StartUseItemEffect(client->viewCore, pos);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitUseItemEffect(client->viewCore, pos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScFaintHpGauge(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monId = args[0];

    switch (*seq) {
    case 0:
        BtlvCore_StartGaugeHP(client->viewCore, MonIDToBattlePos(client->mainModule, client->pokeCon, monId));
        client->studio.unk26 = GetBattleMonSpecies(GetPokeParamConst(client->pokeCon, args[0]));
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitGaugeHP(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScSwapPos(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 clientId = args[0];
        u8 pos1 = args[1];
        u8 pos2 = args[2];
        u8 slot1 = GetPosIndexInClient(client->mainModule, pos1);
        u8 slot2 = GetPosIndexInClient(client->mainModule, pos2);
        u8 viewPos1 = BattlePosToViewPos(client->mainModule, pos1);
        u8 viewPos2 = BattlePosToViewPos(client->mainModule, pos2);

        BattleParty_Swap(GetPartyData(client->pokeCon, clientId), slot1, slot2);
        BtlvCore_StartSwapPokemon(client->viewCore, clientId, viewPos1, viewPos2, slot1, slot2);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitSwapPokemon(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScMoveToCenter(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_Start(0x281);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            u8 clientId = args[0];
            u8 slot = args[1];
            u8 pos1 = GetClientBattlePos(client->mainModule, clientId, slot);
            u8 pos2 = GetClientBattlePos(client->mainModule, clientId, 1);
            u8 viewPos1 = BattlePosToViewPos(client->mainModule, pos1);
            u8 viewPos2 = BattlePosToViewPos(client->mainModule, pos2);

            BattleParty_Swap(GetPartyData(client->pokeCon, clientId), slot, 1);
            BtlvCore_StartSwapPokemon(client->viewCore, clientId, viewPos1, viewPos2, slot, 1);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitSwapPokemon(client->viewCore)) {
            u8 clientId = args[2];
            u8 slot = args[3];
            u8 pos1 = GetClientBattlePos(client->mainModule, clientId, slot);
            u8 pos2 = GetClientBattlePos(client->mainModule, clientId, 1);
            u8 viewPos1 = BattlePosToViewPos(client->mainModule, pos1);
            u8 viewPos2 = BattlePosToViewPos(client->mainModule, pos2);

            BattleParty_Swap(GetPartyData(client->pokeCon, clientId), slot, 1);
            BtlvCore_StartSwapPokemon(client->viewCore, clientId, viewPos1, viewPos2, slot, 1);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvCore_WaitSwapPokemon(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScSubstituteCreate(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_StartEffect26E(BattlePosToViewPos(client->mainModule, args[0]));
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

static BOOL BattleClient_ScSubstituteDelete(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 viewPos = BattlePosToViewPos(client->mainModule, args[0]);

        if (BattleClient_IsChapterSkipping(client)) {
            BtlvEffect_SetFlag26(TRUE);
        }
        BtlvEffect_StartEffect26F(viewPos);
        (*seq)++;
        break;
    }
    case 1:
        if (!BtlvEffect_IsBusy()) {
            if (BattleClient_IsChapterSkipping(client)) {
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
        u8 pos = MonIDToViewPos(client->mainModule, client->pokeCon, args[0]);
        u8 targetPos = MonIDToViewPos(client->mainModule, client->pokeCon, args[1]);

        TransformSet(mon, target);
        BtlvCore_StartTransform(client->viewCore, pos, targetPos);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitTransform(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScDamageEffect(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        if (!BattleClient_IsChapterSkipping(client)) {
            u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

            BtlvCore_StartDamageEffect(client->viewCore, args[2], pos, BtlCalc_GetEffectivenessCategory(args[1]));
            (*seq)++;
            break;
        }
        return TRUE;
    case 1:
        if (BtlvCore_WaitDamageEffect(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScPlayBgm(BtlClient *client, s32 *seq, const u32 *args) {
    BtlvEffect_PlayBgm(args[0]);
    return TRUE;
}

static BOOL BattleClient_ScHideMsgWindow(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvCore_StartHideMsgWin(client->viewCore);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitHideMsgWin(client->viewCore)) {
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
    u32 pos = MonIDToViewPos(client->mainModule, client->pokeCon, monId);
    BOOL visible = FALSE;

    if (pos != 0xff && BtlvEffect_CheckGaugeExist(pos)) {
        visible = TRUE;
    }

    switch (*seq) {
    case EXP_SEQ_INIT:
        sExpLeft = args[1];
        sExpSeq = 0;
        *seq = EXP_SEQ_ADD;
    case EXP_SEQ_ADD:
        if (sExpLeft != 0) {
            u32 exp = sExpLeft;

            if (BattleMon_AddExp(mon, &exp, &sLevelUp)) {
                *seq = EXP_SEQ_LEVEL_UP;
            } else {
                if (visible) {
                    BtlvEffect_CalcGaugeExp(pos, sExpLeft, mon);
                    *seq = EXP_SEQ_WAIT_GAUGE;
                } else {
                    *seq = EXP_SEQ_END;
                }
                exp = 0;
            }
            sExpLeft = exp;
        } else {
            *seq = EXP_SEQ_END;
        }
        break;
    case EXP_SEQ_WAIT_GAUGE:
        if (!BtlvEffect_CheckExecuteGauge()) {
            *seq = EXP_SEQ_END;
        }
        break;
    case EXP_SEQ_LEVEL_UP:
        BattleMon_RecalcStats(mon);
        JoinAvenue_RecordActivity(9);
        BattleMon_ReloadSrcData(mon);
        func_ov167_021bc6ac(mon);
        if (visible) {
            BtlvEffect_CalcGaugeExpLevelUp(pos, mon);
            *seq = EXP_SEQ_WAIT_GAUGE_LEVEL_UP;
        } else {
            *seq = EXP_SEQ_LEVEL_UP_MSG;
        }
        break;
    case EXP_SEQ_WAIT_GAUGE_LEVEL_UP:
        if (!BtlvEffect_CheckExecuteGauge() && !GFL_SndPlayerIsActiveAny()) {
            BtlvCore_StartPosEffect(client->viewCore, pos, 0x25d);
            *seq = EXP_SEQ_WAIT_LEVEL_UP_EFFECT;
        }
        break;
    case EXP_SEQ_WAIT_LEVEL_UP_EFFECT:
        if (BtlvCore_WaitPosEffect(client->viewCore)) {
            *seq = EXP_SEQ_LEVEL_UP_MSG;
        }
        break;
    case EXP_SEQ_LEVEL_UP_MSG:
        Btlv_StringParam_Setup(&client->strParam, 1, 0x3c);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLevelUp.level);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        *seq = EXP_SEQ_WAIT_LEVEL_UP_MSG;
        break;
    case EXP_SEQ_WAIT_LEVEL_UP_MSG:
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(0x515, 0xffff);
        }
        if (BtlvCore_WaitMsg(client->viewCore) && !GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            sExpSeq = 0;
            *seq = EXP_SEQ_WAIT_INPUT;
        }
        break;
    case EXP_SEQ_WAIT_INPUT:
        sExpSeq++;
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed() || sExpSeq > 80) {
            sExpSeq = 0;
            NotifyMonLevelUp(client->mainModule, mon);
            BtlvCore_StartLevelUpWin(client->viewCore, mon, &sLevelUp);
            *seq = EXP_SEQ_WAIT_LEVEL_UP_WIN;
        }
        break;
    case EXP_SEQ_WAIT_LEVEL_UP_WIN:
        if (BtlvCore_WaitLevelUpWin(client->viewCore) &&
            ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed())) {
            BtlvCore_StartLevelUpStats(client->viewCore);
            *seq = EXP_SEQ_WAIT_LEVEL_UP_STATS;
        }
        break;
    case EXP_SEQ_WAIT_LEVEL_UP_STATS:
        if (BtlvCore_WaitLevelUpStats(client->viewCore) &&
            ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed())) {
            BtlvCore_StartCloseLevelUpWin(client->viewCore);
            *seq = EXP_SEQ_WAIT_CLOSE_LEVEL_UP_WIN;
        }
        break;
    case EXP_SEQ_WAIT_CLOSE_LEVEL_UP_WIN:
        if (BtlvCore_WaitCloseLevelUpWin(client->viewCore)) {
            sExpSeq = LEARN_SEQ_INIT;
            *seq = EXP_SEQ_LEARN_MOVE;
        }
        break;
    case EXP_SEQ_LEARN_MOVE:
        if (BattleClient_LearnMoveSeq(client, &sExpSeq, mon)) {
            *seq = EXP_SEQ_ADD;
        }
        break;
    case EXP_SEQ_END:
        SyncServerMonMoves(client->mainModule, monId);
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
    case LEARN_SEQ_INIT:
        sLearnIdx = 0;
        sLearnMove = 0;
        *seq = LEARN_SEQ_NEXT_MOVE;
    case LEARN_SEQ_NEXT_MOVE:
        sLearnMove = PokeParty_LearnNextLevelMove(GetSrcData(mon), &sLearnIdx, client->heapId);
        if (sLearnMove == 0) {
            return TRUE;
        }
        if (sLearnMove == LEARN_MOVE_KNOWN) {
            break;
        }
        if (sLearnMove & LEARN_MOVE_NO_SLOT) {
            sLearnMove &= ~LEARN_MOVE_NO_SLOT;
            *seq = LEARN_SEQ_ASK_FORGET;
        } else {
            *seq = LEARN_SEQ_LEARNED_MSG;
        }
        break;
    case LEARN_SEQ_LEARNED_MSG: {
        BattleMon *bpp = GetPokeParam(client->pokeCon, monId);

        GetSrcData(bpp);
        BattleMon_ReloadSrcData(bpp);
        Btlv_StringParam_Setup(&client->strParam, 5, 3);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        *seq = LEARN_SEQ_WAIT_LEARNED_MSG;
    }
    case LEARN_SEQ_WAIT_LEARNED_MSG:
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(0x515, 0xffff);
        }
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = LEARN_SEQ_WAIT_FANFARE;
        }
        break;
    case LEARN_SEQ_WAIT_FANFARE:
        if (!GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            *seq = LEARN_SEQ_NEXT_MOVE;
        }
        break;
    case LEARN_SEQ_ASK_FORGET:
        Btlv_StringParam_Setup(&client->strParam, 5, 4);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
        Btlv_StringParam_SetWait(&client->strParam, 0xff);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        *seq = LEARN_SEQ_WAIT_ASK_FORGET;
    case LEARN_SEQ_WAIT_ASK_FORGET:
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            BtlvStringParam yesParam;
            BtlvStringParam noParam;

            Btlv_StringParam_Setup(&yesParam, 6, 2);
            Btlv_StringParam_Setup(&noParam, 6, 3);
            BtlvCore_ShowYesNo(client->viewCore, &yesParam, &noParam, 1);
        }
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = LEARN_SEQ_ANSWER_FORGET;
        }
        break;
    case LEARN_SEQ_ANSWER_FORGET: {
        u32 answer;

        if (BtlvCore_PollYesNo(client->viewCore, &answer)) {
            if (answer == 0) {
                BtlvCore_StartForgetMoveSelect(
                    client->viewCore, BattleParty_FindMonID(GetClientParty(client->pokeCon, client->clientId), monId),
                    sLearnMove);
                *seq = LEARN_SEQ_SELECT_FORGET;
            } else {
                *seq = LEARN_SEQ_ASK_GIVE_UP;
            }
        }
        break;
    }
    case LEARN_SEQ_SELECT_FORGET: {
        u8 slot;

        if (BtlvCore_WaitForgetMoveSelect(client->viewCore, &slot)) {
            if (slot == 4) {
                *seq = LEARN_SEQ_ASK_FORGET;
            } else {
                PartyPkm *pkm = GetSrcData(GetPokeParam(client->pokeCon, monId));
                u16 oldMove = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL);

                PokeParty_SetMove(pkm, sLearnMove, slot);
                Btlv_StringParam_Setup(&client->strParam, 5, 5);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                Btlv_StringParam_AddArg(&client->strParam, oldMove);
                BtlvCore_StartMsgCallback(client->viewCore, &client->strParam, BattleClient_ForgetMoveSECallback);
                *seq = LEARN_SEQ_WAIT_FORGOT_MSG;
            }
        }
        break;
    }
    case LEARN_SEQ_WAIT_FORGOT_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = LEARN_SEQ_LEARNED_MSG;
        }
        break;
    case LEARN_SEQ_ASK_GIVE_UP:
        Btlv_StringParam_Setup(&client->strParam, 5, 7);
        Btlv_StringParam_AddArg(&client->strParam, monId);
        Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
        Btlv_StringParam_SetWait(&client->strParam, 0xff);
        BtlvCore_StartMsg(client->viewCore, &client->strParam);
        *seq = LEARN_SEQ_WAIT_ASK_GIVE_UP;
        break;
    case LEARN_SEQ_WAIT_ASK_GIVE_UP:
        if (BtlvCore_IsMsgJustDone(client->viewCore)) {
            BtlvStringParam yesParam;
            BtlvStringParam noParam;

            Btlv_StringParam_Setup(&yesParam, 6, 4);
            Btlv_StringParam_AddArg(&yesParam, sLearnMove);
            Btlv_StringParam_Setup(&noParam, 6, 5);
            Btlv_StringParam_AddArg(&noParam, sLearnMove);
            BtlvCore_ShowYesNo(client->viewCore, &yesParam, &noParam, 1);
        }
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = LEARN_SEQ_ANSWER_GIVE_UP;
        }
        break;
    case LEARN_SEQ_ANSWER_GIVE_UP: {
        u32 answer;

        if (BtlvCore_PollYesNo(client->viewCore, &answer)) {
            if (answer == 0) {
                Btlv_StringParam_Setup(&client->strParam, 5, 8);
                Btlv_StringParam_AddArg(&client->strParam, monId);
                Btlv_StringParam_AddArg(&client->strParam, sLearnMove);
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
                *seq = LEARN_SEQ_WAIT_GAVE_UP_MSG;
            } else {
                *seq = LEARN_SEQ_ASK_FORGET;
            }
        }
        break;
    }
    case LEARN_SEQ_WAIT_GAVE_UP_MSG:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            *seq = LEARN_SEQ_NEXT_MOVE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScBallThrow(BtlClient *client, s32 *seq, const u32 *args) {
    static const u16 sEscapeMessages[4] = { 0x3d, 0x3e, 0x3f, 0x40 };
    BattleMon *mon;
    u16 message;

    switch (*seq) {
    case 0:
        BtlvEffect_StartEffect23A(BattlePosToViewPos(client->mainModule, args[0]), args[5], args[1], args[2], args[4]);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            if (args[2]) {
                mon = PokeCon_GetPosMonInner(client->pokeCon, args[0]);
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
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            if (args[2]) {
                mon = PokeCon_GetPosMonInner(client->pokeCon, args[0]);
                NotifyMonCaught(client->mainModule, mon);
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
            SkipExitBGMFadeOut(client->mainModule);
            if (args[3]) {
                mon = PokeCon_GetPosMonInner(client->pokeCon, args[0]);
                Btlv_StringParam_Setup(&client->strParam, 1, 0x42);
                Btlv_StringParam_AddArg(&client->strParam, GetMonID(mon));
                BtlvCore_StartMsg(client->viewCore, &client->strParam);
                (*seq)++;
            } else {
                return TRUE;
            }
        }
        break;
    case 4:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScBallThrowTrainer(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0:
        BtlvEffect_StartEffect23D(BattlePosToViewPos(client->mainModule, args[0]), args[1]);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            Btlv_StringParam_Setup(&client->strParam, 1, 0x43);
            BtlvCore_StartMsg(client->viewCore, &client->strParam);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScRotation(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 clientId = args[0];
        u8 result = args[1];

        BattleParty_Rotate(GetPartyData(client->pokeCon, clientId), result, NULL, NULL);
        BtlvCore_StartRotation(client->viewCore, clientId, result);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitRotation(client->viewCore)) {
            u8 result = args[1];
            u8 isNpc;
            u16 message;

            if (result == 1) {
                return TRUE;
            }
            isNpc = IsClientNPCTrainer(client->mainModule, args[0]);
            if (result == 3) {
                message = isNpc ? 0x26 : 0x27;
            } else {
                message = isNpc ? 0x28 : 0x29;
            }
            BtlvCore_StartMsgStd(client->viewCore, message, args);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitMsg(client->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScChangeAbility(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monId = args[0];
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, monId);

    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            ChangeAbility(GetPokeParam(client->pokeCon, monId), args[1]);
            return TRUE;
        }
        BtlvCore_ShowAbilityWin(client->viewCore, pos, 0);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitShowAbilityWin(client->viewCore, pos)) {
            ChangeAbility(GetPokeParam(client->pokeCon, monId), args[1]);
            BtlvCore_UpdateAbilityWin(client->viewCore, pos);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvCore_WaitUpdateAbilityWin(client->viewCore, pos)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScSwapAbility(BtlClient *client, s32 *seq, const u32 *args) {
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
        if (BattleClient_IsChapterSkipping(client)) {
            ChangeAbility(mon1, ability1);
            ChangeAbility(mon2, ability2);
            return TRUE;
        }
        if (IsAllyMonID(monId1, monId2)) {
            ChangeAbility(mon1, ability1);
            ChangeAbility(mon2, ability2);
            return TRUE;
        }
        BtlvCore_ShowAbilityWin(client->viewCore, pos1, 0);
        sSwapTimer = 8;
        (*seq)++;
        break;
    case 1:
        BtlvCore_WaitShowAbilityWin(client->viewCore, pos1);
        if (sSwapTimer != 0) {
            sSwapTimer--;
        } else {
            BtlvCore_ShowAbilityWin(client->viewCore, pos2, 0);
            (*seq)++;
        }
        break;
    case 2: {
        u8 done1 = BtlvCore_WaitShowAbilityWin(client->viewCore, pos1);
        u8 done2 = BtlvCore_WaitShowAbilityWin(client->viewCore, pos2);

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
            BtlvCore_UpdateAbilityWin(client->viewCore, pos1);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvCore_WaitUpdateAbilityWin(client->viewCore, pos1)) {
            BtlvCore_UpdateAbilityWin(client->viewCore, pos2);
            (*seq)++;
        }
        break;
    case 6:
        if (BtlvCore_WaitUpdateAbilityWin(client->viewCore, pos2)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScIllusionBreak(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 monId = args[0];
        u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, monId);
        BattleMon *mon = GetPokeParam(client->pokeCon, monId);
        BOOL skip = BattleClient_IsChapterSkipping(client);

        IllusionBreak(mon);
        BtlvCore_StartIllusionBreak(client->viewCore, pos, skip);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitIllusionBreak(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScEffect(BtlClient *client, s32 *seq, const u32 *args) {
    if (*seq == 0) {
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvEffect_Start(args[0]);
        (*seq)++;
    } else if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScEffectByPos(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = BattlePosToViewPos(client->mainModule, args[0]);

    if (*seq == 0) {
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        if (!IsBattleSceneOff(client->mainModule) && IsStatEffectId(args[1])) {
            return TRUE;
        }
        BtlvCore_StartPosEffect(client->viewCore, pos, args[1]);
        (*seq)++;
    } else if (BtlvCore_WaitPosEffect(client->viewCore)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScEffectBetweenPos(BtlClient *client, s32 *seq, const u32 *args) {
    if (*seq == 0) {
        u8 pos1;

        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        pos1 = BattlePosToViewPos(client->mainModule, args[0]);
        BtlvCore_StartAtkDefEffect(client->viewCore, pos1, BattlePosToViewPos(client->mainModule, args[1]), args[2]);
        (*seq)++;
    } else if (BtlvCore_WaitAtkDefEffect(client->viewCore)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScChangeForm(BtlClient *client, s32 *seq, const u32 *args) {
    switch (*seq) {
    case 0: {
        u8 pos = BattlePosToViewPos(client->mainModule, MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]));

        ChangeForm(GetPokeParam(client->pokeCon, args[0]), args[1]);
        BtlvCore_StartFormChange(client->viewCore, pos);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvCore_WaitFormChange(client->viewCore)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScAbilityPopupIn(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

    switch (*seq) {
    case 0:
        if (BattleClient_IsChapterSkipping(client)) {
            return TRUE;
        }
        BtlvCore_ShowAbilityWin(client->viewCore, pos, 1);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitShowAbilityWin(client->viewCore, pos)) {
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleClient_ScAbilityPopupOut(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos = MonIDToBattlePos(client->mainModule, client->pokeCon, args[0]);

    switch (*seq) {
    case 0:
        BtlvCore_HideAbilityWin(client->viewCore, pos);
        (*seq)++;
        break;
    case 1:
        if (BtlvCore_WaitHideAbilityWin(client->viewCore, pos)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BattleClient_ScOpSubtractHp(BtlClient *client, s32 *seq, const u32 *args) {
    HPSub(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpAddHp(BtlClient *client, s32 *seq, const u32 *args) {
    HPAdd(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpSubtractPP(BtlClient *client, s32 *seq, const u32 *args) {
    u8 moveIdx = args[1];
    u8 amount = args[2];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    Move_DecrementPP(mon, moveIdx, amount);
    return TRUE;
}

static BOOL BattleClient_ScOpSubtractOriginalPP(BtlClient *client, s32 *seq, const u32 *args) {
    u8 moveIdx = args[1];
    u8 amount = args[2];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    Move_DecrementPP_Org(mon, moveIdx, amount);
    return TRUE;
}

static BOOL BattleClient_ScOpSetMoveUsed(BtlClient *client, s32 *seq, const u32 *args) {
    u8 moveIdx = args[1];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    Move_SetUsed(mon, moveIdx);
    return TRUE;
}

static BOOL BattleClient_ScOpZeroHp(BtlClient *client, s32 *seq, const u32 *args) {
    HPZero(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpAddPP(BtlClient *client, s32 *seq, const u32 *args) {
    Move_IncrementPP(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL BattleClient_ScOpAddOriginalPP(BtlClient *client, s32 *seq, const u32 *args) {
    Move_IncrementPP_Org(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL BattleClient_ScOpRaiseStatStage(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_RaiseStatStage(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL BattleClient_ScOpLowerStatStage(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_LowerStatStage(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetStatStages(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    BattleMon_SetStatStage(mon, 1, args[1]);
    BattleMon_SetStatStage(mon, 2, args[2]);
    BattleMon_SetStatStage(mon, 3, args[3]);
    BattleMon_SetStatStage(mon, 4, args[4]);
    BattleMon_SetStatStage(mon, 5, args[5]);
    BattleMon_SetStatStage(mon, 6, args[6]);
    BattleMon_SetStatStage(mon, 7, args[7]);
    return TRUE;
}

static BOOL BattleClient_ScOpRecoverStatStages(BtlClient *client, s32 *seq, const u32 *args) {
    StatStageRecover(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpResetStatStages(BtlClient *client, s32 *seq, const u32 *args) {
    StatStageReset(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpAddCritStage(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_AddCritStage(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetCondition(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);
    BattleCondition value;

    value.raw = args[2];
    SetMoveCondition(mon, args[1], value);
    return TRUE;
}

static BOOL BattleClient_ScOpCureCondition(BtlClient *client, s32 *seq, const u32 *args) {
    CureCondition(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpCureMoveCondition(BtlClient *client, s32 *seq, const u32 *args) {
    CureMoveCondition(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpSendIn(BtlClient *client, s32 *seq, const u32 *args) {
    u8 pos1 = client->cmdArgs[1];
    u8 pos2 = client->cmdArgs[2];
    BattleParty *party = GetPartyData(client->pokeCon, (u8)client->cmdArgs[0]);
    BattleMon *mon;

    if (pos1 != pos2) {
        BattleParty_Swap(party, pos1, pos2);
    }
    mon = BattleParty_GetMon(party, pos1);
    BattleMon_SetAppearTurn(mon, args[3]);
    BattleMon_ClearForSwitchIn(mon);
    return TRUE;
}

static BOOL BattleClient_ScOpChangeType(BtlClient *client, s32 *seq, const u32 *args) {
    ChangePokeType(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpConditionTurnCheck(BtlClient *client, s32 *seq, const u32 *args) {
    u8 monIds[6];
    u32 numMons = args[0];
    u32 i;
    u32 numConditions = args[1];
    u32 j;

    BtlCalc_UnpackMonIDs(args[2], monIds);
    for (i = 0; i < numConditions; i++) {
        u32 condition = GetConditionByIndex(i);

        if (condition == 0) {
            break;
        }
        for (j = 0; j < numMons; j++) {
            BattleMon *mon = GetPokeParam(client->pokeCon, monIds[j]);

            if (!IsFainted(mon)) {
                BattleMon_TickCondition(mon, condition, NULL, NULL);
            }
        }
    }
    return TRUE;
}

static BOOL BattleClient_ScOpConsumeItem(BtlClient *client, s32 *seq, const u32 *args) {
    ConsumeItem(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpUpdateUsedMove(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_SetPrevMove(GetPokeParam(client->pokeCon, args[0]), args[1], args[2], args[3], args[4], args[5]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetConditionFlag(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_SetConditionFlag(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpClearConditionFlag(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_ClearConditionFlag(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetTurnFlag(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_SetTurnFlag(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpClearTurnFlag(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_ClearTurnFlag(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpChangeAbility(BtlClient *client, s32 *seq, const u32 *args) {
    ChangeAbility(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetItem(BtlClient *client, s32 *seq, const u32 *args) {
    SetItem(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpUpdateMove(BtlClient *client, s32 *seq, const u32 *args) {
    Move_UpdateID(GetPokeParam(client->pokeCon, args[0]), args[1], args[4], args[2], args[3]);
    return TRUE;
}

static BOOL BattleClient_ScOpClearForSwitch(BtlClient *client, s32 *seq, const u32 *args) {
    Clear_ForSwitch(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpAddFieldEffect(BtlClient *client, s32 *seq, const u32 *args) {
    BattleCondition value;

    value.raw = args[1];
    FieldStatusaddEffectCore(client->field, args[0], value, 0);
    return TRUE;
}

static BOOL BattleClient_ScOpAddFieldEffectDependMon(BtlClient *client, s32 *seq, const u32 *args) {
    BtlField_AddDependPoke(client->field, args[0], args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpRemoveFieldEffectDependMon(BtlClient *client, s32 *seq, const u32 *args) {
    BtlField_RemoveDependPoke(client->field, args[0]);
    return TRUE;
}

static BOOL BattleClient_ScOpRemoveFieldEffect(BtlClient *client, s32 *seq, const u32 *args) {
    BtlField_RemoveEffect(client->field, args[0]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetCounter(BtlClient *client, s32 *seq, const u32 *args) {
    COUNTER_Set(GetPokeParam(client->pokeCon, args[0]), args[1], args[2]);
    return TRUE;
}

static BOOL BattleClient_ScOpBatonPass(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *source = GetPokeParam(client->pokeCon, args[0]);

    CopyBatonPassParams(GetPokeParam(client->pokeCon, args[1]), source);
    return TRUE;
}

static BOOL BattleClient_ScOpCreateSubstitute(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_SetSubstituteHP(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpDeleteSubstitute(BtlClient *client, s32 *seq, const u32 *args) {
    ResetSpActPriority(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpAddShooterEnergy(BtlClient *client, s32 *seq, const u32 *args) {
    u8 clientId = args[0];
    u8 value = args[1];

    if (clientId == client->clientId) {
        BattleClient_AddShooterEnergy(client, value);
    } else {
        AddClientShooterEnergy(client->mainModule, clientId, value);
    }
    return TRUE;
}

static BOOL BattleClient_ScOpSetupIllusion(BtlClient *client, s32 *seq, const u32 *args) {
    u8 clientId = args[0];

    UpdatePartyIllusionDisguises(client->mainModule, GetPartyData(client->pokeCon, clientId), clientId);
    return TRUE;
}

static BOOL BattleClient_ScOpClearConsumedItem(BtlClient *client, s32 *seq, const u32 *args) {
    ClearConsumedItem(GetPokeParam(client->pokeCon, args[0]));
    return TRUE;
}

static BOOL BattleClient_ScOpCureMonDependentConditions(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon_CureConditionsFromMon(GetPokeParam(client->pokeCon, args[0]), (u8)args[1]);
    return TRUE;
}

static BOOL BattleClient_ScOpAddDamageRecord(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);
    BattleMonDamageRecord record;

    BattleMonDamageRecord_Init(&record, args[1], args[2], args[4], args[3], args[5]);
    BattleMon_AddDamageRecord(mon, &record);
    return TRUE;
}

static BOOL BattleClient_ScOpTurnCheck(BtlClient *client, s32 *seq, const u32 *args) {
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    BattleMon_EndTurn(mon);
    ComboMove_ClearParam(mon);
    return TRUE;
}

static BOOL BattleClient_ScOpFieldTurnCheck(BtlClient *client, s32 *seq, const u32 *args) {
    BtlField_TurnCheck(client->field, NULL, NULL);
    return TRUE;
}

static BOOL BattleClient_ScOpSetBaseStat(BtlClient *client, s32 *seq, const u32 *args) {
    u32 stat = args[1];
    BattleMon *mon = GetPokeParam(client->pokeCon, args[0]);

    SetBaseStatus(mon, stat, args[2]);
    return TRUE;
}

static BOOL BattleClient_ScOpSetWeight(BtlClient *client, s32 *seq, const u32 *args) {
    SetWeight(GetPokeParam(client->pokeCon, args[0]), args[1]);
    return TRUE;
}

u8 BattleClient_GetClientId(BtlClient *client) {
    return client->clientId;
}

BattleParty *BattleClient_GetParty(BtlClient *client) {
    return client->party;
}

u8 BattleClient_GetWeather(BtlClient *client) {
    return BtlField_GetWeather(client->field);
}

u16 BattleClient_GetAudienceSize(BtlClient *client) {
    return client->studio.unk58;
}

u32 BattleClient_GetAudienceMask(BtlClient *client) {
    return client->studio.audienceMask;
}

u8 BattleClient_GetShooterEnergy(BtlClient *client) {
    return client->shooterEnergy;
}

static void BattleClient_LoadAIItems(BtlClient *client) {
    u32 i;

    for (i = 0; i < 4; i++) {
        client->aiItems[i] = GetClientTrainerItem(client->mainModule, client->clientId, i);
    }
}

static u16 AICheckItemUse(BtlClient *client, BattleMon *mon, BattleParty *party) {
    u8 numMons = GetNumMonsInParty(party);
    u32 i;

    if (CheckCondition(mon, CONDITION_EMBARGO)) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (sAIItemMaxMons[i] < numMons) {
            break;
        }
        if (client->aiItems[i] != 0) {
            BOOL use = FALSE;

            if (ItemGetParam(client->aiItems[i], 0x29)) {
                if (GetBattleMonStat(mon, BATTLEMON_HP) > DivideMaxHp(mon, 4)) {
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
    if (ItemGetParam(item, 0x24) && BattleMon_GetCritStage(mon) == 0) {
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
    recPlayer->result = REC_PLAYER_RESULT_NONE;
    recPlayer->seq = REC_PLAYER_SEQ_INPUT;
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

static BOOL RecPlayer_IsQuitDone(BtlClientRecPlayer *recPlayer) {
    return recPlayer->quitDone;
}

static void RecPlayer_ReachChapter(BtlClientRecPlayer *recPlayer) {
    recPlayer->chapterReached = TRUE;
}

static u32 RecPlayer_GetResult(BtlClientRecPlayer *recPlayer) {
    return recPlayer->result;
}

static void RecPlayer_StartSkip(BtlClientRecPlayer *recPlayer, u32 target) {
    recPlayer->skipping = TRUE;
    recPlayer->skipCount = 0;
    recPlayer->skipTarget = target;
    recPlayer->quitStarted = FALSE;
    recPlayer->quitDone = FALSE;
}

static void RecPlayer_EndSkip(BtlClientRecPlayer *recPlayer) {
    recPlayer->seq = REC_PLAYER_SEQ_INPUT;
    recPlayer->skipping = FALSE;
}

static BOOL RecPlayer_IsSkipDone(BtlClientRecPlayer *recPlayer) {
    if (recPlayer->skipCount == recPlayer->skipTarget) {
        return TRUE;
    }
    return FALSE;
}

static u32 RecPlayer_GetSkipTarget(BtlClientRecPlayer *recPlayer) {
    return recPlayer->skipTarget;
}

static BOOL RecPlayer_TryLock(BtlClientRecPlayer *recPlayer) {
    if (recPlayer->quitStarted) {
        return FALSE;
    }
    recPlayer->unk2_4 = TRUE;
    return TRUE;
}

static void RecPlayer_Unlock(BtlClientRecPlayer *recPlayer) {
    recPlayer->unk2_4 = FALSE;
}

static void RecPlayer_SetDataEnd(BtlClientRecPlayer *recPlayer) {
    recPlayer->dataEnd = TRUE;
}

static void RecPlayer_Update(BtlClient *client, BtlClientRecPlayer *recPlayer) {
    if (client->clientType == BTL_CLIENT_TYPE_REC_PLAY && !recPlayer->unk2_4 && !recPlayer->dataEnd &&
        recPlayer->maxChapter != 0) {
        BOOL update = FALSE;

        if (recPlayer->chapterReached) {
            recPlayer->chapterReached = FALSE;
            if (!recPlayer->skipping) {
                if (recPlayer->chapter < recPlayer->maxChapter && recPlayer->seq == REC_PLAYER_SEQ_INPUT) {
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
                            BtlvCore_ShowRecorderChapter(client->viewCore, recPlayer->chapter, recPlayer->skipTarget);
                        }
                    }
                }
                return;
            }
        }
        if (recPlayer->chapter != 0 && client->viewCore != NULL) {
            switch (recPlayer->seq) {
            case REC_PLAYER_SEQ_INPUT: {
                u32 input = BtlvCore_CheckRecorderInput(client->viewCore);
                BOOL changed = FALSE;

                switch (input) {
                case BTLV_RECORDER_QUIT:
                    recPlayer->result = REC_PLAYER_RESULT_QUIT;
                    BtlvCore_StartFadeOut(client->viewCore);
                    recPlayer->quitStarted = TRUE;
                    recPlayer->seq = REC_PLAYER_SEQ_FADE_OUT;
                    BtlvCore_ShowRecorderMessage(client->viewCore, recPlayer->chapter, 1);
                    func_ov167_0219c9d4(client->mainModule);
                    func_ov167_0219bdf0(client->mainModule);
                    GFL_SndBGMFadeOut(30);
                    GFL_SndPlayerSetMuteStateEx(0, 0x3e);
                    PokeVoice_SetMasterVolume(0);
                    BtlvEffect_ReleaseVoices();
                    SkipExitBGMFadeOut(client->mainModule);
                    break;
                case BTLV_RECORDER_PREV:
                    if (recPlayer->skipTarget > 1) {
                        recPlayer->skipTarget--;
                        changed = TRUE;
                    }
                    recPlayer->quitTimer = 45;
                    break;
                case BTLV_RECORDER_NEXT:
                    if (recPlayer->skipTarget < recPlayer->maxChapter) {
                        recPlayer->skipTarget++;
                        changed = TRUE;
                    }
                    recPlayer->quitTimer = 45;
                    break;
                }
                if (changed || update) {
                    BtlvCore_ShowRecorderChapter(client->viewCore, recPlayer->chapter, recPlayer->skipTarget);
                }
                if (recPlayer->quitTimer != 0) {
                    recPlayer->quitTimer--;
                    if (recPlayer->quitTimer == 0) {
                        recPlayer->result = REC_PLAYER_RESULT_SKIP;
                        BtlvCore_StartFadeOut(client->viewCore);
                        recPlayer->quitStarted = TRUE;
                        recPlayer->seq = REC_PLAYER_SEQ_FADE_OUT;
                        GFL_SndBGMFadeOut(30);
                        GFL_SndPlayerSetMuteStateEx(0, 0x3e);
                        PokeVoice_SetMasterVolume(0);
                        BtlvEffect_ReleaseVoices();
                    }
                }
                break;
            }
            case REC_PLAYER_SEQ_FADE_OUT:
                if (BtlvCore_WaitFadeOut(client->viewCore)) {
                    recPlayer->quitDone = TRUE;
                    recPlayer->seq = REC_PLAYER_SEQ_DONE;
                }
                break;
            case REC_PLAYER_SEQ_DONE:
                break;
            }
        }
    }
}

static void Studio_PlayAudienceSE(BtlClientStudioWork *studio, u32 reaction) {
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
            BtlvCore_SetTypeIconVisible(core, 1, i, reaction);
            studio->audienceTimers[i] = GFL_RandomLC(300) + 400;
            count++;
        }
    }
    if (count != 0) {
        Studio_PlayAudienceSE(studio, reaction);
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
            BtlvCore_SetTypeIconVisible(core, 1, member, 9);
            studio->audienceTimers[member] = 0;
            studio->audienceMask ^= 1 << member;
            left++;
            members[r] = -1;
        }
    }
}

static void Studio_ReactToScenePoints(BtlvCore *core, BtlClientStudioWork *studio) {
    if (studio->score.points[STUDIO_SCORE_SCENE] < 0) {
        Studio_AudienceLeave(core, studio, studio->unk58);
        Studio_ReactAudience(core, studio, 5);
    } else if (studio->score.points[STUDIO_SCORE_SCENE] > 60) {
        Studio_ReactAudience(core, studio, 7);
    }
}

static void Studio_ReactToFame(BtlvCore *core, BtlClientStudioWork *studio, int fame) {
    if (fame != 0) {
        Studio_ReactAudience(core, studio, 1);
    }
}

static void Studio_AddFamePoints(BtlClient *client, BtlClientStudioWork *studio, BattleParty *party) {
    static const u8 sFameBonus[5] = { 0, 1, 5, 10, 15 };
    PartyPkm *pkm = GetSrcData(GetBattleMonFromParty(party, 0));
    int fame;

    PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    fame = PML_GetPokestarFameLevel(PokeParty_GetParam(pkm, PKM_PARAM_POKESTAR_FAME, NULL));
    studio->score.points[STUDIO_SCORE_FAME] += sFameBonus[fame];
    studio->score.counts[STUDIO_SCORE_FAME]++;
    Studio_ReactToFame(client->viewCore, &client->studio, fame);
}

static void StudioScore_AddScenePoints(BtlClient *client, BtlClientStudioScore *score, s32 scene) {
    BtlScriptedRules *rules = GetStudioRules(client->mainModule);

    score->counts[STUDIO_SCORE_SCENE] = scene;
    score->points[STUDIO_SCORE_SCENE] += rules->scenes[scene].unk0A;
}

static void StudioScore_SetTurnPoints(BtlClient *client, BtlClientStudioScore *score) {
    BtlScriptedRules *rules = GetStudioRules(client->mainModule);
    int points = 5 - MATH_ABS(rules->unk0C - client->turnCount);

    if (points < 0) {
        points = 0;
    }
    score->points[STUDIO_SCORE_TURN] = points;
    score->counts[STUDIO_SCORE_TURN] = client->turnCount;
}

static void StudioScore_AddChoicePoints(BtlClient *client, BtlClientStudioScore *score, s32 scene, u32 index) {
    static const u32 sReactions[5][3] = {
        { 2, 5, 0 }, { 3, 6, 0 }, { 4, 6, 0 }, { 4, 5, 0 }, { 3, 6, 0 },
    };
    BtlScriptedRules *rules = GetStudioRules(client->mainModule);
    int points = rules->scenes[scene].choices[index].points;
    u32 reaction;

    score->points[STUDIO_SCORE_CHOICE] += points;
    score->counts[STUDIO_SCORE_CHOICE]++;
    if (points > 0) {
        reaction = sReactions[rules->scenes[scene].unk08][0];
        Studio_SetLastChoiceResult(client, 1);
    } else if (points < 0) {
        reaction = sReactions[rules->scenes[scene].unk08][1];
        Studio_SetLastChoiceResult(client, 3);
    } else {
        reaction = sReactions[rules->scenes[scene].unk08][2];
        Studio_SetLastChoiceResult(client, 2);
    }
    Studio_ReactAudience(client->viewCore, &client->studio, reaction);
}

static s32 StudioScore_CalcTotal(BtlClient *client, BtlClientStudioScore *score) {
    BtlScriptedRules *rules = GetStudioRules(client->mainModule);
    BtlSetup *setup = GetBattleSetupParam(client->mainModule);
    s32 total;
    s32 sum;

    sum = score->points[STUDIO_SCORE_FAME];
    sum = score->points[STUDIO_SCORE_TURN] + (s32)sum;
    sum = score->points[STUDIO_SCORE_SCENE] + (s32)sum;
    sum = score->points[STUDIO_SCORE_CHOICE] + (s32)sum;
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

    if (GetStudioMode(client->mainModule) == 2 && client->clientId == 0) {
        for (i = 0; i < 22; i++) {
            if (client->studio.audienceTimers[i] != 0) {
                client->studio.audienceTimers[i]--;
                if (client->studio.audienceTimers[i] == 0) {
                    BtlvCore_StartTypeIconAnim(client->viewCore, i);
                    BtlvCore_SetTypeIconVisible(client->viewCore, 1, i, 0);
                }
            }
        }
    }
}

static void Studio_PlayEndingBgm(BtlMainModule *mainModule) {
    BtlSetup *setup = GetBattleSetupParam(mainModule);

    if (setup->studioEnding == 0) {
        BtlvEffect_PlayBgm(func_ov167_0219bf08(mainModule));
    } else if (setup->studioEnding == 1) {
        BtlvEffect_PlayBgm(GetWinBGM(mainModule));
    } else if (setup->studioEnding == 2) {
        BtlvEffect_PlayBgm(func_ov167_0219bf14(mainModule));
    }
}
