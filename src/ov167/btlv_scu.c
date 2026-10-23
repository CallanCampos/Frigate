#include "types.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_string.h"
#include "battle/btlv.h"
#include "battle/btlv_effect.h"
#include "constants/arc.h"
#include "constants/battle.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"

// The work of btlv_scu.c, the message and command unit of the battle display, recovered from its accesses in
// BtlvScu_Create through BtlvScu_ClearMsgWin. BtlvScu_Create allocates it with
// GFL_HeapAllocate(heapId, 0x180, TRUE, <btlv_scu.c>, 351). Swan has no names for this file; the names are ours, from
// use.

// A window faded in and out by the BG1 alpha blend, its coefficients in fx32 (BtlvScuWinFade_Init, 021d3718,
// 021d3748, 021d3798)
typedef struct BtlvScuWinFade {
    BmpWin *window; // 0x00
    fx32 ev1;       // 0x04 the coefficient of the window, 31 shown and 0 hidden
    fx32 ev2;       // 0x08 the coefficient of the background, 7 shown and 16 hidden
    fx32 ev1Step;   // 0x0C
    fx32 ev2Step;   // 0x10
    u8 state;       // 0x14 0 shown, 1 hiding, 2 hidden, 3 showing, 4 show requested
    u8 frames;      // 0x15 frames left of the fade, 6 at its start
} BtlvScuWinFade;   // size 0x18

// The HP gauge of a battle position (BtlvScuGauge_Init, 021d3950, 021d3974, 021d398c)
typedef struct BtlvScuGauge {
    BattleMon *mon; // 0x00
    BtlvScu *scu;   // 0x04
    u8 pos;         // 0x08
    u8 viewPos;     // 0x09 BattlePosToViewPos of pos
    u8 shown;       // 0x0A set once BtlvEffect_AddGauge shows it, cleared with BtlvEffect_DelGauge
    u8 active;      // 0x0B the position exists in this battle style
} BtlvScuGauge;     // size 0x0C

// An ability pop-up, one for each side, on BG2 or BG3 (BtlvScu_InitAbilityWins, and 021d3bb4 through 021d3f4c)
typedef struct BtlvScuAbilityWin {
    BtlvScu *scu;      // 0x00
    GFLBitmap *bitmap; // 0x04 18x4 characters
    u16 ability;       // 0x08 GetBattleMonStat(mon, 0x10)
    u8 side;           // 0x0A the index, 0 or 1, from BtlvScu_GetSide
    u8 bg;             // 0x0B 2, or 3 for side 1
    u8 monId;          // 0x0C 0x1F while none
    u8 seq;            // 0x0D
    void *chars;       // 0x10 the 0x900 bytes of BtlvScu.abilityChars for this side
    fx32 scrollX;      // 0x14
    fx32 scrollStep;   // 0x18
    u32 charOffset;    // 0x1C abilityCharOffset + side * 0x48
    u8 timer;          // 0x20
    u8 count;          // 0x21 the line drawn, then the blend level
    u8 shown : 1;      // 0x22 bit 0
    u8 flash : 1;      // 0x22 bit 1, the last argument of BtlvScuAbilityWin_StartShow: go on to the palette flash
} BtlvScuAbilityWin;   // size 0x24

struct BtlvScu {
    BmpWin *msgWin;                    // 0x000 BG1
    GFLBitmap *msgBitmap;              // 0x004
    BmpWin *statWin;                   // 0x008 the level-up stats window, BG3
    GFLBitmap *statBitmap;             // 0x00C
    u32 statFrameCharOffset;           // 0x010 the characters of file 0x1EB, the frame of statWin
    u32 abilityCharOffset;             // 0x014 the characters of file 0x1E3, the ability pop-ups
    PrintQueue *printQueue;            // 0x018
    PrintWindow printWindow;           // 0x01C PrintWindow_Init(.., msgWin) runs before msgWin exists: window is NULL
    Font *font;                        // 0x024
    Font *smallFont;                   // 0x028 the font of the ability pop-ups
    TCBExManager *tcbManager;          // 0x02C
    PrintStream *printStream;          // 0x030
    StrBuf *strbuf;                    // 0x034 0x400 characters
    StrBuf *abilityStrbuf;             // 0x038 0x180 characters
    StrBuf *lineStrbuf;                // 0x03C 0x180 characters, a line of abilityStrbuf
    u8 taskCount;                      // 0x040 tasks of BtlvScu_StartChangeMon, 021d35e0, 021d363c, 021d3830
    u8 gaugeShowTaskCount;             // 0x041 tasks of BtlvScu_StartMemberIn
    u8 gaugeHideTaskCount;             // 0x042 tasks of BtlvScu_StartMemberOut
    u8 unk043[5];                      // 0x043 never accessed
    BtlvScuGauge gauges[6];            // 0x048 by battle position
    BtlvScuAbilityWin abilityWins[2];  // 0x090 by side
    void *abilityCharFile;             // 0x0D8 the file GFL_G2DIOReadBGNCGR reads, freed with GFL_HeapFree
    NNSG2dCharacterData *abilityChars; // 0x0DC
    BtlvSubProc subProc;               // 0x0E0
    u8 work[0x40];                     // 0x0F0 the work of the running procedure, one of the types below
    BtlvCore *core;                    // 0x130 only stored
    BtlMainModule *mainModule;         // 0x134
    BtlPokeCon *pokeCon;               // 0x138
    HeapID heapId;                     // 0x13C
    u8 printedAtOnce;                  // 0x13E set by BtlvScu_PrintMsgAtOnce: the window is neither cleared nor
                                       //       streamed
    u8 msgSeq;                         // 0x13F the state of BtlvScu_WaitMsg, 7 when done
    u8 clientId;                       // 0x140 only stored
    u8 msgFinished;                    // 0x141 set when the stream reports PRINT_STREAM_DONE, read and cleared by
                                       //       BtlvScu_IsMsgFinished
    u32 streamState;                   // 0x144 PrintStream_GetState
    u16 waitTimer;                     // 0x148
    u16 pageWait;                      // 0x14A the wait after a paused page
    u16 endWait;                       // 0x14C the wait after the last page
    u8 unk14E;                         // 0x14E only cleared, in BtlvScu_Create
    u8 encountStep;                    // 0x14F the step through the tables of the encounter procedures
    u8 recPlay;               // 0x150 the argument of BtlvScu_StartEncount, a recorded battle: skip the encounter
    BtlvScuWinFade msgFade;   // 0x154
    BattleMon *levelUpMon;    // 0x16C
    BattleMonLevelUp levelUp; // 0x170 the gains, then the new stats, which BattleMon_GetSrcStats writes over
}; // size 0x180

// The works of the procedures, which cast scu->work

// One mon: BtlvScu_EncountWildSingle (wild single), 021d15f8 (trainer single), 021d215c, 021d26c4
typedef struct BtlvScuSingleWork {
    BattleMon *mon;  // 0x00
    u8 pos;          // 0x04 ViewPosToBattlePos of viewPos
    u8 viewPos;      // 0x05
    u8 monId;        // 0x06
    u8 clientId;     // 0x07 of the trainer, or of the player
    u8 unk08;        // 0x08 GetOpponentClientID(mainModule, playerClientId, 0), a client ID for the message
} BtlvScuSingleWork; // size 0x0C

// Two wild mons: BtlvScu_EncountWildDouble, 021d1d64. The arrays are in another order than in BtlvScuDoubleWork
typedef struct BtlvScuWildDoubleWork {
    BattleMon *mons[2];  // 0x00
    u8 monIds[2];        // 0x08
    u8 pos[2];           // 0x0A
    u8 viewPos[2];       // 0x0C
    u8 unk0E;            // 0x0E never accessed
    u8 clientId;         // 0x0F
} BtlvScuWildDoubleWork; // size 0x10

// Two mons sent out together: BtlvScu_StepEnemySendOutDoubleOneTrainer, 021d23d4, 021d2820, 021d29bc
typedef struct BtlvScuDoubleWork {
    BattleMon *mons[2]; // 0x00
    u8 pos[2];          // 0x08
    u8 viewPos[2];      // 0x0A
    u8 monIds[2];       // 0x0C
    u8 count;           // 0x0E of the mons not fainted
    u8 clientId;        // 0x0F
} BtlvScuDoubleWork;    // size 0x10

// Three: BtlvScu_StepEnemySendOutTriple, 021d2b88
typedef struct BtlvScuTripleWork {
    BattleMon *mons[3]; // 0x00
    u8 pos[3];          // 0x0C
    u8 viewPos[3];      // 0x0F
    u8 monIds[3];       // 0x12
    u8 count;           // 0x15
    u8 clientId;        // 0x16
} BtlvScuTripleWork;    // size 0x18

// The sprites of the opposing trainers: BtlvScu_StepTwoTrainersAppear uses both, BtlvScu_StepOneTrainerAppear only
// viewPos[1]
typedef struct BtlvScuTrainerWork {
    u8 viewPos[2];
} BtlvScuTrainerWork;

// The works of the TCBEx tasks, as GFL_TCBExMgrAddTask sizes them

// BtlvScu_StartMemberOut and its task BtlvScu_MemberOutTask: an effect, then the gauge hidden
typedef struct BtlvScuGaugeHideTask {
    BtlvScuGauge *gauge; // 0x00
    u16 seq;             // 0x04
    u16 pos;             // 0x06
    u16 effect;          // 0x08 0x26C if BtlvScu_StartMemberOut got 0
    u8 *count;           // 0x0C &scu->gaugeHideTaskCount
} BtlvScuGaugeHideTask;  // size 0x10

// BtlvScu_StartMemberIn and BtlvScu_MemberInTask: the gauge shown once the effect ends
typedef struct BtlvScuGaugeShowTask {
    BtlvScuGauge *gauge; // 0x00
    u16 seq;             // 0x04
    u8 *count;           // 0x08 &scu->gaugeShowTaskCount
} BtlvScuGaugeShowTask;  // size 0x0C

// BtlvScu_StartChangeMon and BtlvScu_ChangeMonTask
typedef struct BtlvScuMonTask {
    BtlvScu *scu; // 0x00
    u8 pos;       // 0x04
    u32 viewPos;  // 0x08
    s32 seq;      // 0x0C
    u32 arg2;     // 0x10 BtlvEffect_ChangeSprite rather than 021df6b4 if set
    u8 *count;    // 0x14 &scu->taskCount
} BtlvScuMonTask; // size 0x18

// BtlvScu_StartTransform, BtlvScu_StartChangeForm and their task BtlvScu_ChangeSpriteTask
typedef struct BtlvScuPkmTask {
    BtlvScu *scu;  // 0x00
    u32 arg1;      // 0x04 the second argument of BtlvEffect_StartChangeSprite or 021df76c
    PartyPkm *pkm; // 0x08
    s32 seq;       // 0x0C
    u8 unk10;      // 0x10 never accessed
    u8 flag;       // 0x11 BtlvEffect_ChangeSprite rather than 021df6b4 if set
    u8 *count;     // 0x14 &scu->taskCount
} BtlvScuPkmTask;  // size 0x18

// BtlvScu_StartMsgWinHide and BtlvScu_MsgWinHideTask: msgFade hides the message window, which is then cleared
typedef struct BtlvScuMsgHideTask {
    BtlvScu *scu;     // 0x00
    s32 seq;          // 0x04
    u8 *count;        // 0x08 &scu->taskCount
} BtlvScuMsgHideTask; // size 0x0C

// A step of an encounter sequence, run until it returns TRUE
typedef BOOL (*BtlvScuEncountStep)(BtlvScu *scu, s32 *seq);

// The index of an ability pop-up, by the side of the battle position, from BtlvScu_GetSide. It returns an enum,
// as two returns kept in this order show
typedef enum BtlvScuSide {
    BTLV_SCU_SIDE_PLAYER,
    BTLV_SCU_SIDE_ENEMY,
} BtlvScuSide;

static u16 BtlvScu_LoadArcCharsDynamic(BtlvScu *scu, u32 arcId, u32 fileId, u32 bg);
static BOOL BtlvScu_CheckFadeDone(BtlvScu *scu);
static void BtlvScu_EncountFadeIn(s32 wait);
static BOOL BtlvScu_EncountRecPlay(BtlvScu *scu, s32 *seq, const u8 *viewPos, u16 count);
static BOOL BtlvScu_EncountRecPlaySingle(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_EncountRecPlayDouble(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_EncountRecPlayTriple(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_EncountWildSingle(s32 *seq, void *arg);
static u16 BtlvScu_GetWildEncountMsg(BtlvScu *scu);
static BOOL BtlvScu_EncountTrainerSingle(s32 *seq, void *arg);
static BOOL BtlvScu_EncountCommSingle(s32 *seq, void *arg);
static BOOL BtlvScu_EncountWildDouble(s32 *seq, void *arg);
static BOOL BtlvScu_EncountWildDoubleMulti(s32 *seq, void *arg);
static BOOL BtlvScu_EncountTrainerDouble(s32 *seq, void *arg);
static BOOL BtlvScu_EncountTrainerDoubleMulti(s32 *seq, void *arg);
static BOOL BtlvScu_EncountCommDouble(s32 *seq, void *arg);
static BOOL BtlvScu_EncountCommDoubleMulti(s32 *seq, void *arg);
static BOOL BtlvScu_EncountTriple(s32 *seq, void *arg);
static BOOL BtlvScu_EncountCommTriple(s32 *seq, void *arg);
static BOOL BtlvScu_StepWildDoubleAppear(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepTrainerAppear(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepTrainersAppear(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepOneTrainerAppear(BtlvScu *scu, s32 *seq, u8 clientId);
static BOOL BtlvScu_StepTwoTrainersAppear(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2);
static BOOL BtlvScu_StepEnemySendOutSingle(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepEnemySendOutDouble(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepEnemySendOutDoubleOneTrainer(BtlvScu *scu, s32 *seq, u8 clientId);
static BOOL BtlvScu_StepEnemySendOutDoubleTwoTrainers(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2);
static BOOL BtlvScu_StepEnemySendOutTriple(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepPlayerSendOutSingle(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepPlayerSendOutDouble(BtlvScu *scu, s32 *seq);
static BOOL BtlvScu_StepPlayerSendOutDoubleAlone(BtlvScu *scu, s32 *seq, u8 clientId);
static BOOL BtlvScu_StepPlayerSendOutDoubleWithPartner(BtlvScu *scu, s32 *seq, u8 clientId, u8 partnerId);
static BOOL BtlvScu_StepPlayerSendOutTriple(BtlvScu *scu, s32 *seq);
static u16 BtlvScu_GetSendOutMsg(BtlvScu *scu, u8 count);
static BOOL BtlvScu_MoveEffectProc(s32 *seq, void *arg);
static void BtlvScu_MemberOutTask(TCBEx *task, void *data);
static void BtlvScu_MemberInTask(TCBEx *task, void *data);
static void BtlvScu_ChangeMonTask(TCBEx *task, void *data);
static void BtlvScu_ChangeSpriteTask(TCBEx *task, void *data);
static void BtlvScuWinFade_Init(BtlvScuWinFade *fade, BmpWin *window);
static void BtlvScuWinFade_StartHide(BtlvScuWinFade *fade);
static void BtlvScuWinFade_StartShow(BtlvScuWinFade *fade, BOOL keepWindow, u32 color);
static BOOL BtlvScuWinFade_Update(BtlvScuWinFade *fade);
static void BtlvScu_MsgWinHideTask(TCBEx *task, void *data);
static void BtlvScu_InitGauges(BtlvScu *scu);
static void BtlvScu_QuitGauges(BtlvScu *scu);
static void BtlvScuGauge_Init(BtlvScuGauge *gauge, BtlvScu *scu, u8 pos);
static void BtlvScuGauge_Quit(BtlvScuGauge *gauge);
static void BtlvScuGauge_Hide(BtlvScuGauge *gauge);
static void BtlvScuGauge_Show(BtlvScuGauge *gauge);
static BtlvScuSide BtlvScu_GetSide(BtlMainModule *mainModule, u8 pos);
static void BtlvScu_InitAbilityWins(BtlvScu *scu, u32 charOffset);
static void BtlvScu_FreeAbilityWins(BtlvScu *scu);
static void BtlvScuAbilityWin_StartShow(BtlvScuAbilityWin *win, u8 pos, BOOL flash);
static BOOL BtlvScuAbilityWin_RunShow(BtlvScuAbilityWin *win);
static void BtlvScuAbilityWin_DrawText(BtlvScuAbilityWin *win);
static void BtlvScuAbilityWin_Hide(BtlvScuAbilityWin *win);
static BOOL BtlvScuAbilityWin_WaitHide(BtlvScuAbilityWin *win);
static void BtlvScuAbilityWin_StartChange(BtlvScuAbilityWin *win, u8 pos);
static BOOL BtlvScuAbilityWin_RunChange(BtlvScuAbilityWin *win);
static void BtlvScu_SetupBallGauge(BtlvScu *scu, BtlvBGaugeParam *status, u8 clientId1, u8 clientId2, s32 arg4);
static BOOL BtlvScu_LevelUpWinOpenProc(s32 *seq, void *arg);
static BOOL BtlvScu_LevelUpWinStatsProc(s32 *seq, void *arg);
static BOOL BtlvScu_LevelUpWinCloseProc(s32 *seq, void *arg);
static u16 BtlvScu_GetClearColor(BtlvScu *scu, u16 color);
static void BtlvScu_ClearMsgWin(BtlvScu *scu);

// The view positions whose mons a recorded battle shows at once, by style
static const u8 sSingleViewPos[] = { 1, 0 };
static const u8 sDoubleViewPos[] = { 2, 3, 4, 5 };
static const u8 sTripleViewPos[] = { 2, 3, 4, 5, 6, 7 };

// The steps of the encounter sequences, by BtlvScu_StartEncount's choice. The two-dimensional ones go by step, then by
// the player's client ID. The 12-byte tables are declared in an order that MWCC's size sort lays out as the game
// does
static const BtlvScuEncountStep sDoubleWildMultiSteps[] = { BtlvScu_StepWildDoubleAppear,
                                                            BtlvScu_StepPlayerSendOutDouble };
static const BtlvScuEncountStep sDoubleTrainerMultiSteps[] = { BtlvScu_StepTrainersAppear,
                                                               BtlvScu_StepEnemySendOutDouble,
                                                               BtlvScu_StepPlayerSendOutDouble };
static const BtlvScuEncountStep sTripleSteps[] = { BtlvScu_StepTrainerAppear, BtlvScu_StepEnemySendOutTriple,
                                                   BtlvScu_StepPlayerSendOutTriple };
static const BtlvScuEncountStep sDoubleTrainerSteps[] = { BtlvScu_StepTrainerAppear, BtlvScu_StepEnemySendOutDouble,
                                                          BtlvScu_StepPlayerSendOutDouble };
static const BtlvScuEncountStep sDoubleCommMultiSteps[] = { BtlvScu_StepTrainersAppear, BtlvScu_StepEnemySendOutDouble,
                                                            BtlvScu_StepPlayerSendOutDouble };
// clang-format off
static const BtlvScuEncountStep sTripleCommSteps[][2] = {
    {BtlvScu_StepTrainerAppear, BtlvScu_StepTrainerAppear},
    {BtlvScu_StepEnemySendOutTriple, BtlvScu_StepPlayerSendOutTriple},
    {BtlvScu_StepPlayerSendOutTriple, BtlvScu_StepEnemySendOutTriple},
};

// clang-format on
// clang-format off
static const BtlvScuEncountStep sSingleCommSteps[][2] = {
    {BtlvScu_StepTrainerAppear, BtlvScu_StepTrainerAppear},
    {BtlvScu_StepEnemySendOutSingle, BtlvScu_StepPlayerSendOutSingle},
    {BtlvScu_StepPlayerSendOutSingle, BtlvScu_StepEnemySendOutSingle},
};

// clang-format on
// clang-format off
static const BtlvScuEncountStep sDoubleCommSteps[][2] = {
    {BtlvScu_StepTrainerAppear, BtlvScu_StepTrainerAppear},
    {BtlvScu_StepEnemySendOutDouble, BtlvScu_StepPlayerSendOutDouble},
    {BtlvScu_StepPlayerSendOutDouble, BtlvScu_StepEnemySendOutDouble},
};

// clang-format on

// The BG setups, declared in the only order found that lays them out as the game does: BG1, BG2, BG3
// BG1, the message window
static const BGSetup sMsgBGSetup = { 0, 0, 0x800, 0, 1, 0, 9, 4, 0x8000, 0, 0, 0, 0 };
// BG2, the ability pop-up of side 0
static const BGSetup sAbilityBG2Setup = { 0x90, 0, 0x1000, 0, 3, 0, 10, 2, 0x8000, 0, 0, 0, 0 };
// BG3, the ability pop-up of side 1 and the level-up stats
static const BGSetup sAbilityBG3Setup = { -0x90, 0, 0x2000, 0, 4, 0, 12, 2, 0x8000, 0, 0, 0, 0 };

static inline void BtlvScu_SetSubProc(BtlvScu *scu, BtlvSubProcFn main) {
    scu->subProc.init = NULL;
    scu->subProc.main = main;
    scu->subProc.arg = scu;
    scu->subProc.seq = 0;
}

static inline BOOL BtlvScu_RunSubProc(BtlvScu *scu) {
    if (scu->subProc.init != NULL) {
        if (scu->subProc.init(&scu->subProc.seq, scu->subProc.arg)) {
            scu->subProc.init = NULL;
            scu->subProc.seq = 0;
        }
        return FALSE;
    }
    if (scu->subProc.main != NULL) {
        if (scu->subProc.main(&scu->subProc.seq, scu->subProc.arg)) {
            scu->subProc.main = NULL;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

BtlvScu *BtlvScu_Create(BtlvCore *core, BtlMainModule *mainModule, BtlPokeCon *pokeCon, TCBExManager *tcbManager,
                        Font *font, Font *smallFont, u8 clientId, HeapID heapId) {
    BtlvScu *scu = GFL_HeapAllocate(heapId, sizeof(BtlvScu), TRUE, "btlv_scu.c", 351);

    scu->core = core;
    scu->mainModule = mainModule;
    scu->pokeCon = pokeCon;
    scu->heapId = heapId;
    scu->clientId = clientId;
    scu->font = font;
    scu->smallFont = smallFont;
    scu->recPlay = FALSE;
    scu->printStream = NULL;
    scu->tcbManager = tcbManager;
    scu->strbuf = GFL_StrBufCreate(0x400, heapId);
    scu->abilityStrbuf = GFL_StrBufCreate(0x180, heapId);
    scu->lineStrbuf = GFL_StrBufCreate(0x180, heapId);
    scu->unk14E = 0;
    scu->abilityCharFile = NULL;
    scu->printQueue = PrintQueue_CreateDefault(scu->heapId);
    PrintWindow_Init(&scu->printWindow, scu->msgWin);
    return scu;
}

void BtlvScu_Setup(BtlvScu *scu) {
    u32 frameChar;
    u16 black;

    GFL_BGSysCreateBG(1, &sMsgBGSetup, 0);
    GFL_BGSysCreateBG(2, &sAbilityBG2Setup, 0);
    GFL_BGSysCreateBG(3, &sAbilityBG3Setup, 0);
    PaletteFade_LoadNCLREx(BtlvEffect_GetPaletteFade(), 11, 0x165, scu->heapId, 0, 0x20, 0, 0);
    PaletteFade_LoadNCLREx(BtlvEffect_GetPaletteFade(), 11, 0x1e4, scu->heapId, 0, 0x60, 0x10, 0x10);
    black = 0;
    PaletteFade_LoadData(BtlvEffect_GetPaletteFade(), &black, 0, 0, sizeof(black));
    GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 32, 0x11);
    GFL_BGSysFillChar(1, 0, 1, 0);
    frameChar = BtlvScu_LoadArcCharsDynamic(scu, 11, 0x164, 1);
    scu->abilityCharOffset = BtlvScu_LoadArcCharsDynamic(scu, 11, 0x1e3, 3);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTLV, 0x1e5, 2, 0, 0, FALSE, scu->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTLV, 0x1e6, 3, 0, 0, FALSE, scu->heapId);
    scu->msgWin = BmpWin_CreateDynamic(1, 1, 19, 30, 4, 0, 0);
    scu->msgBitmap = BmpWin_GetBitmap(scu->msgWin);
    BmpWin_FlushMap(scu->msgWin);
    if (GetStudioMode(scu->mainModule) != 2) {
        BmpWin_MakeFrameScreen(scu->msgWin, frameChar, 0);
    }
    GFL_BitmapFill(scu->msgBitmap, BtlvScu_GetClearColor(scu, 12));
    BmpWin_FlushChar(scu->msgWin);
    scu->statFrameCharOffset = BtlvScu_LoadArcCharsDynamic(scu, 11, 0x1eb, 3);
    scu->statWin = BmpWin_CreateDynamic(3, 21, 37, 10, 12, 3, 0);
    scu->statBitmap = BmpWin_GetBitmap(scu->statWin);
    BtlvScuWinFade_Init(&scu->msgFade, scu->msgWin);
    BtlvScu_InitGauges(scu);
    BtlvScu_InitAbilityWins(scu, scu->abilityCharOffset);
    GFL_BGSysLoadScr(1);
    GFL_BGSysLoadScr(2);
    GFL_BGSysLoadScr(3);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    if (GetStudioMode(scu->mainModule) != 2) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
    } else {
        G2_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 |
                                  GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                              TRUE);
        G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG1, FALSE);
        GX_SetVisibleWnd(GX_WNDMASK_W0);
        G2_SetWnd0Position(0, 8, 255, 158);
        GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, -8);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    }
}

static u16 BtlvScu_LoadArcCharsDynamic(BtlvScu *scu, u32 arcId, u32 fileId, u32 bg) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(arcId, HEAPID_TAIL(scu->heapId));
    u32 offset = GFL_BGSysLoadArcNCGRDynamic(arc, fileId, bg, 0, FALSE, HEAPID_TAIL(scu->heapId));

    GFL_ArcToolFree(arc);
    return offset;
}

void BtlvScu_Delete(BtlvScu *scu) {
    BtlvScu_FreeAbilityWins(scu);
    BtlvScu_QuitGauges(scu);
    if (scu->statWin != NULL) {
        BmpWin_Free(scu->statWin);
        scu->statWin = NULL;
    }
    if (scu->msgWin != NULL) {
        BmpWin_Free(scu->msgWin);
        scu->msgWin = NULL;
    }
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    if (scu->printQueue != NULL) {
        PrintQueue_Free(scu->printQueue);
        scu->printQueue = NULL;
    }
    if (scu->lineStrbuf != NULL) {
        GFL_StrBufFree(scu->lineStrbuf);
        scu->lineStrbuf = NULL;
    }
    if (scu->abilityStrbuf != NULL) {
        GFL_StrBufFree(scu->abilityStrbuf);
        scu->abilityStrbuf = NULL;
    }
    GFL_StrBufFree(scu->strbuf);
    GFL_HeapFree(scu);
}

void BtlvScu_RestoreBG3(const BtlvScu *scu) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BTLV, HEAPID_TAIL(scu->heapId));

    GFL_BGSysLoadArcNCGRStatic(arc, 0x1e3, 3, scu->abilityCharOffset, 0, FALSE, scu->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0x1eb, 3, scu->statFrameCharOffset, 0, FALSE, scu->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysClearScr(3);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTLV, 0x1e6, 3, 0, 0, FALSE, scu->heapId);
}

static BOOL BtlvScu_CheckFadeDone(BtlvScu *scu) {
    if (!GFL_FadeIsRunning()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScu_StartEncount(BtlvScu *scu, BOOL recPlay) {
    u32 type = BtlSetup_GetBattleType(scu->mainModule);

    scu->encountStep = 0;
    scu->recPlay = recPlay;
    switch (BtlSetup_GetBattleStyle(scu->mainModule)) {
    case BTL_STYLE_SINGLE:
        switch (type) {
        case 0:
        case 4:
        default:
            BtlvScu_SetSubProc(scu, BtlvScu_EncountWildSingle);
            break;
        case 1:
        case 2:
            BtlvScu_SetSubProc(scu, BtlvScu_EncountTrainerSingle);
            break;
        case 3:
            BtlvScu_SetSubProc(scu, BtlvScu_EncountCommSingle);
            break;
        }
        break;
    case BTL_STYLE_DOUBLE:
        switch (type) {
        case 0:
            if (!IsMultiMode(scu->mainModule)) {
                BtlvScu_SetSubProc(scu, BtlvScu_EncountWildDouble);
            } else {
                BtlvScu_SetSubProc(scu, BtlvScu_EncountWildDoubleMulti);
            }
            break;
        case 3:
            if (!IsMultiMode(scu->mainModule)) {
                BtlvScu_SetSubProc(scu, BtlvScu_EncountCommDouble);
            } else {
                BtlvScu_SetSubProc(scu, BtlvScu_EncountCommDoubleMulti);
            }
            break;
        case 1:
        case 2:
            if (!IsMultiMode(scu->mainModule)) {
                BtlvScu_SetSubProc(scu, BtlvScu_EncountTrainerDouble);
            } else {
                BtlvScu_SetSubProc(scu, BtlvScu_EncountTrainerDoubleMulti);
            }
            break;
        }
        break;
    case BTL_STYLE_TRIPLE:
        if (type != 3) {
            BtlvScu_SetSubProc(scu, BtlvScu_EncountTriple);
        } else {
            BtlvScu_SetSubProc(scu, BtlvScu_EncountCommTriple);
        }
        break;
    case BTL_STYLE_ROTATION:
        if (type != 3) {
            BtlvScu_SetSubProc(scu, BtlvScu_EncountTriple);
        } else {
            BtlvScu_SetSubProc(scu, BtlvScu_EncountCommTriple);
        }
        break;
    }
}

BOOL BtlvScu_WaitEncount(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

static void BtlvScu_EncountFadeIn(s32 wait) {
    if (GX_GetMasterBrightness() <= 0) {
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, wait);
    } else {
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, wait);
    }
}

// A recorded battle shows the mons and their gauges at once
static BOOL BtlvScu_EncountRecPlay(BtlvScu *scu, s32 *seq, const u8 *viewPos, u16 count) {
    u8 view;
    u8 pos;
    u32 i;

    switch (*seq) {
    case 0:
        for (i = 0; i < count; i++) {
            view = viewPos[i];
            pos = ViewPosToBattlePos(scu->mainModule, view);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(PokeCon_GetPosMonInner(scu->pokeCon, pos)), view);
            BtlvScuGauge_Show(&scu->gauges[pos]);
        }
        (*seq)++;
        break;
    case 1:
        if (scu->recPlay) {
            return TRUE;
        }
        BtlvScu_EncountFadeIn(-3);
        (*seq)++;
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BtlvScu_EncountRecPlaySingle(BtlvScu *scu, s32 *seq) {
    return BtlvScu_EncountRecPlay(scu, seq, sSingleViewPos, NELEMS(sSingleViewPos));
}

static BOOL BtlvScu_EncountRecPlayDouble(BtlvScu *scu, s32 *seq) {
    return BtlvScu_EncountRecPlay(scu, seq, sDoubleViewPos, NELEMS(sDoubleViewPos));
}

static BOOL BtlvScu_EncountRecPlayTriple(BtlvScu *scu, s32 *seq) {
    return BtlvScu_EncountRecPlay(scu, seq, sTripleViewPos, NELEMS(sTripleViewPos));
}

// A wild single battle
static BOOL BtlvScu_EncountWildSingle(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlaySingle(scu, seq);
    }
    switch (*seq) {
    case 0:
        work->viewPos = 1;
        work->pos = ViewPosToBattlePos(scu->mainModule, work->viewPos);
        work->mon = PokeCon_GetPosMonInner(scu->pokeCon, work->pos);
        work->monId = GetMonID(work->mon);
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mon), work->viewPos);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_StartPos(work->viewPos, 0x231);
        BtlvScu_EncountFadeIn(2);
        (*seq)++;
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            BtlString_FormatStdMsg(scu->strbuf, BtlvScu_GetWildEncountMsg(scu), 1, work->monId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuGauge_Show(&scu->gauges[work->pos]);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvEffect_Start(BtlSetup_GetBattleType(scu->mainModule) == 4 ? 0x23c : 0x232);
            (*seq)++;
        }
        break;
    case 6:
        if (!BtlvEffect_IsBusy()) {
            work->viewPos = 0;
            work->pos = ViewPosToBattlePos(scu->mainModule, work->viewPos);
            work->mon = PokeCon_GetPosMonInner(scu->pokeCon, work->pos);
            work->monId = GetMonID(work->mon);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mon), work->viewPos);
            BtlvEffect_Start(0x234);
            BtlString_FormatStdMsg(scu->strbuf, 11, 1, work->monId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 7:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 8:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            (*seq)++;
        }
        break;
    case 9:
        if (!BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos]);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The message that a wild mon appeared, by the battle's type
static u16 BtlvScu_GetWildEncountMsg(BtlvScu *scu) {
    if (BtlSetup_IsBattleType(scu->mainModule, 4)) {
        return 6;
    }
    if (BtlSetup_IsBattleType(scu->mainModule, 8)) {
        return 3;
    }
    if (BtlSetup_IsBattleType(scu->mainModule, 0x2000)) {
        return 200;
    }
    if (BtlSetup_IsBattleType(scu->mainModule, 0x80)) {
        return 4;
    }
    if (BtlSetup_IsBattleType(scu->mainModule, 0x10)) {
        return 5;
    }
    return 1;
}

// A trainer single battle
static BOOL BtlvScu_EncountTrainerSingle(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlaySingle(scu, seq);
    }
    switch (*seq) {
    case 0: {
        u8 pos = ViewPosToBattlePos(scu->mainModule, 1);
        u8 clientId = GetClientIDFromPos(scu->mainModule, pos);

        BtlvEffect_SetTrainer(GetClientTrainerClass(scu->mainModule, clientId), 9, 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(9, 0);
        work->viewPos = 1;
        work->clientId = clientId;
        work->pos = pos;
        work->mon = PokeCon_GetPosMonInner(scu->pokeCon, work->pos);
        work->monId = GetMonID(work->mon);
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    }
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvEffect_StartPos(work->viewPos, 0x237);
            BtlvScu_EncountFadeIn(2);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            BtlvBGaugeParam balls;

            BtlvScu_SetupBallGauge(scu, &balls, work->clientId, 4, 1);
            BtlvEffect_AddBallGauge(&balls);
            BtlString_FormatStdMsg(scu->strbuf, 7, 1, work->clientId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_StartPos(work->viewPos, 0x238);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlString_FormatStdMsg(scu->strbuf, 14, 2, 1, work->monId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mon), work->viewPos);
            BtlvEffect_StartPos(work->viewPos, 0x239);
            BtlvEffect_DelBallGauge(1);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 6:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvBGaugeParam balls;

            BtlvScu_SetupBallGauge(scu, &balls, GetPlayerClientID(scu->mainModule), 4, 0);
            BtlvEffect_AddBallGauge(&balls);
            (*seq)++;
        }
        break;
    case 7:
        BtlvScuGauge_Show(&scu->gauges[work->pos]);
        BtlvEffect_Start(0x232);
        (*seq)++;
        break;
    case 8:
        if (!BtlvEffect_IsBusy() && !BtlvEffect_IsBallGaugeBusy(0)) {
            work->viewPos = 0;
            work->pos = ViewPosToBattlePos(scu->mainModule, work->viewPos);
            work->mon = PokeCon_GetPosMonInner(scu->pokeCon, work->pos);
            work->monId = GetMonID(work->mon);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mon), work->viewPos);
            BtlvEffect_Start(0x234);
            BtlString_FormatStdMsg(scu->strbuf, 11, 1, work->monId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 9:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_DelBallGauge(0);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 10:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvScu_ClearMsgWin(scu);
            (*seq)++;
        }
        break;
    case 11:
        if (!BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos]);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// A single battle over communication
static BOOL BtlvScu_EncountCommSingle(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlaySingle(scu, seq);
    }
    if (scu->encountStep < NELEMS(sSingleCommSteps)) {
        if (sSingleCommSteps[scu->encountStep][GetPlayerClientID(scu->mainModule)](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

// A wild double battle
static BOOL BtlvScu_EncountWildDouble(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    BtlvScuWildDoubleWork *work = (BtlvScuWildDoubleWork *)scu->work;
    u8 viewPos;
    u32 i;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayDouble(scu, seq);
    }
    switch (*seq) {
    case 0:
        viewPos = GetClientBattlePos(scu->mainModule, scu->clientId, 0);
        work->pos[0] = GetOpposingPos(scu->mainModule, viewPos, 0);
        work->mons[0] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[0]);
        work->monIds[0] = GetMonID(work->mons[0]);
        work->pos[1] = GetOpposingPos(scu->mainModule, viewPos, 1);
        work->mons[1] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[1]);
        work->monIds[1] = GetMonID(work->mons[1]);
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            viewPos = BattlePosToViewPos(scu->mainModule, work->pos[0]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), viewPos);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            viewPos = BattlePosToViewPos(scu->mainModule, work->pos[1]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), viewPos);
            BtlvEffect_StartPos(viewPos, 0x231);
            BtlvScu_EncountFadeIn(2);
            (*seq)++;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            BtlString_FormatStdMsg(scu->strbuf, 2, 2, work->monIds[0], work->monIds[1]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvEffect_Start(0x232);
            (*seq)++;
        }
        break;
    case 6:
        if (!BtlvEffect_IsBusy()) {
            work->viewPos[0] = 2;
            work->viewPos[1] = 4;
            work->clientId = GetPlayerClientID(scu->mainModule);
            for (i = 0; i < 2; i++) {
                work->pos[i] = ViewPosToBattlePos(scu->mainModule, work->viewPos[i]);
                work->mons[i] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[i]);
                if (work->mons[i] != NULL) {
                    work->monIds[i] = GetMonID(work->mons[i]);
                }
            }
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
            BtlvEffect_Start(0x234);
            BtlString_FormatStdMsg(scu->strbuf, 12, 2, work->monIds[0], work->monIds[1]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 7:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 8:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            (*seq)++;
        }
        break;
    case 9:
        if (!BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BtlvScu_EncountWildDoubleMulti(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayDouble(scu, seq);
    }
    if (scu->encountStep < NELEMS(sDoubleWildMultiSteps)) {
        if (sDoubleWildMultiSteps[scu->encountStep](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL BtlvScu_EncountTrainerDouble(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayDouble(scu, seq);
    }
    if (scu->encountStep < NELEMS(sDoubleTrainerSteps)) {
        if (sDoubleTrainerSteps[scu->encountStep](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL BtlvScu_EncountTrainerDoubleMulti(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayDouble(scu, seq);
    }
    if (scu->encountStep < NELEMS(sDoubleTrainerMultiSteps)) {
        if (sDoubleTrainerMultiSteps[scu->encountStep](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL BtlvScu_EncountCommDouble(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayDouble(scu, seq);
    }
    if (scu->encountStep < NELEMS(sDoubleCommSteps)) {
        if (sDoubleCommSteps[scu->encountStep][GetPlayerClientID(scu->mainModule)](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL BtlvScu_EncountCommDoubleMulti(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayDouble(scu, seq);
    }
    if (scu->encountStep < NELEMS(sDoubleCommMultiSteps)) {
        if (sDoubleCommMultiSteps[scu->encountStep](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL BtlvScu_EncountTriple(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayTriple(scu, seq);
    }
    if (scu->encountStep < NELEMS(sTripleSteps)) {
        if (sTripleSteps[scu->encountStep](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL BtlvScu_EncountCommTriple(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return BtlvScu_EncountRecPlayTriple(scu, seq);
    }
    if (scu->encountStep < NELEMS(sTripleCommSteps)) {
        if (sTripleCommSteps[scu->encountStep][GetPlayerClientID(scu->mainModule)](scu, seq)) {
            scu->encountStep++;
            *seq = 0;
        }
        return FALSE;
    }
    return TRUE;
}

// The two wild mons of a multi battle's wild double
static BOOL BtlvScu_StepWildDoubleAppear(BtlvScu *scu, s32 *seq) {
    BtlvScuWildDoubleWork *work = (BtlvScuWildDoubleWork *)scu->work;
    u8 viewPos;

    switch (*seq) {
    case 0:
        viewPos = GetClientBattlePos(scu->mainModule, scu->clientId, 0);
        work->pos[0] = GetOpposingPos(scu->mainModule, viewPos, 0);
        work->mons[0] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[0]);
        work->monIds[0] = GetMonID(work->mons[0]);
        work->pos[1] = GetOpposingPos(scu->mainModule, viewPos, 1);
        work->mons[1] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[1]);
        work->monIds[1] = GetMonID(work->mons[1]);
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            viewPos = BattlePosToViewPos(scu->mainModule, work->pos[0]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), viewPos);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            viewPos = BattlePosToViewPos(scu->mainModule, work->pos[1]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), viewPos);
            BtlvEffect_StartPos(viewPos, 0x231);
            BtlvScu_EncountFadeIn(2);
            (*seq)++;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            BtlString_FormatStdMsg(scu->strbuf, 2, 2, work->monIds[0], work->monIds[1]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BtlvScu_StepTrainerAppear(BtlvScu *scu, s32 *seq) {
    return BtlvScu_StepOneTrainerAppear(scu, seq, GetPlayerOpponentClientID(scu->mainModule, 0));
}

static BOOL BtlvScu_StepTrainersAppear(BtlvScu *scu, s32 *seq) {
    u8 clientId1 = GetPlayerOpponentClientID(scu->mainModule, 0);
    u8 clientId2 = GetPlayerOpponentClientID(scu->mainModule, 1);

    if (clientId1 != clientId2) {
        return BtlvScu_StepTwoTrainersAppear(scu, seq, clientId1, clientId2);
    }
    return BtlvScu_StepOneTrainerAppear(scu, seq, clientId1);
}

// One opposing trainer sends out a mon
static BOOL BtlvScu_StepOneTrainerAppear(BtlvScu *scu, s32 *seq, u8 clientId) {
    BtlvScuTrainerWork *work = (BtlvScuTrainerWork *)scu->work;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->viewPos[1] = 9;
        BtlvEffect_SetTrainer(GetClientTrainerClass(scu->mainModule, clientId), work->viewPos[1], 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(work->viewPos[1], 0);
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvEffect_StartPos(work->viewPos[1], 0x237);
            BtlvScu_EncountFadeIn(2);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            u32 battleType = BtlSetup_GetBattleType(scu->mainModule);
            u32 message = 7;

            if (battleType == 3) {
                message = 8;
            }

            BtlvScu_SetupBallGauge(scu, &balls, clientId, 4, 1);
            BtlvEffect_AddBallGauge(&balls);
            BtlString_FormatStdMsg(scu->strbuf, message, 1, clientId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_StartPos(work->viewPos[1], 0x238);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Two opposing trainers
static BOOL BtlvScu_StepTwoTrainersAppear(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2) {
    BtlvScuTrainerWork *work = (BtlvScuTrainerWork *)scu->work;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 11;
        work->viewPos[1] = 13;
        BtlvEffect_SetTrainer(GetClientTrainerClass(scu->mainModule, clientId1), work->viewPos[0], 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(work->viewPos[0], 0);
        BtlvEffect_SetTrainer(GetClientTrainerClass(scu->mainModule, clientId2), work->viewPos[1], 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(work->viewPos[1], 0);
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvEffect_StartPos(work->viewPos[0], 0x237);
            BtlvScu_EncountFadeIn(2);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            u32 battleType = BtlSetup_GetBattleType(scu->mainModule);
            u32 message = 9;

            if (battleType == 3) {
                message = 10;
            }

            BtlvScu_SetupBallGauge(scu, &balls, clientId1, clientId2, 1);
            BtlvEffect_AddBallGauge(&balls);
            BtlString_FormatStdMsg(scu->strbuf, message, 2, clientId1, clientId2);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_StartPos(work->viewPos[0], 0x238);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The opponent sends out one mon
static BOOL BtlvScu_StepEnemySendOutSingle(BtlvScu *scu, s32 *seq) {
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;

    switch (*seq) {
    case 0:
        work->viewPos = 1;
        work->unk08 = GetOpponentClientID(scu->mainModule, GetPlayerClientID(scu->mainModule), 0);
        work->pos = ViewPosToBattlePos(scu->mainModule, work->viewPos);
        work->mon = PokeCon_GetPosMonInner(scu->pokeCon, work->pos);
        work->monId = GetMonID(work->mon);
        BtlString_FormatStdMsg(scu->strbuf, BtlvScu_GetSendOutMsg(scu, 1), 2, work->unk08, work->monId);
        BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mon), work->viewPos);
        BtlvEffect_StartPos(work->viewPos, 0x239);
        BtlvEffect_DelBallGauge(1);
        (*seq)++;
        break;
    case 3:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos]);
            (*seq)++;
        }
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

// The opponents send out two mons, from one trainer or from two
static BOOL BtlvScu_StepEnemySendOutDouble(BtlvScu *scu, s32 *seq) {
    u8 clientId1 = GetPlayerOpponentClientID(scu->mainModule, 0);
    u8 clientId2 = GetPlayerOpponentClientID(scu->mainModule, 1);

    if (clientId1 != clientId2) {
        return BtlvScu_StepEnemySendOutDoubleTwoTrainers(scu, seq, clientId1, clientId2);
    }
    return BtlvScu_StepEnemySendOutDoubleOneTrainer(scu, seq, clientId1);
}

static BOOL BtlvScu_StepEnemySendOutDoubleOneTrainer(BtlvScu *scu, s32 *seq, u8 clientId) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;
    u32 i;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 3;
        work->viewPos[1] = 5;
        work->clientId = clientId;
        work->count = 0;
        for (i = 0; i < 2; i++) {
            work->pos[i] = ViewPosToBattlePos(scu->mainModule, work->viewPos[i]);
            work->mons[i] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[i]);
            work->monIds[i] = GetMonID(work->mons[i]);
            if (!IsFainted(work->mons[i])) {
                work->count++;
            }
        }
        BtlString_FormatStdMsg(scu->strbuf, BtlvScu_GetSendOutMsg(scu, work->count), 3, work->clientId, work->monIds[0],
                               work->monIds[1]);
        BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
        if (work->count == 2) {
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
        }
        BtlvEffect_StartPos(work->viewPos[0], 0x239);
        BtlvEffect_DelBallGauge(1);
        (*seq)++;
        break;
    case 3:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            if (work->count == 2) {
                BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            }
            (*seq)++;
        }
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

static BOOL BtlvScu_StepEnemySendOutDoubleTwoTrainers(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 3;
        work->pos[0] = ViewPosToBattlePos(scu->mainModule, work->viewPos[0]);
        work->mons[0] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[0]);
        work->monIds[0] = GetMonID(work->mons[0]);
        work->viewPos[1] = 5;
        work->pos[1] = ViewPosToBattlePos(scu->mainModule, work->viewPos[1]);
        work->mons[1] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[1]);
        work->monIds[1] = GetMonID(work->mons[1]);
        BtlString_FormatStdMsg(scu->strbuf, BtlvScu_GetSendOutMsg(scu, 1), 2, clientId1, work->monIds[0]);
        BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlString_FormatStdMsg(scu->strbuf, BtlvScu_GetSendOutMsg(scu, 1), 2, clientId2, work->monIds[1]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
            BtlvEffect_StartPos(work->viewPos[0], 0x239);
            BtlvEffect_DelBallGauge(1);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            (*seq)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

// The opponents send out three mons
static BOOL BtlvScu_StepEnemySendOutTriple(BtlvScu *scu, s32 *seq) {
    BtlvScuTripleWork *work = (BtlvScuTripleWork *)scu->work;
    u32 i;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 3;
        work->viewPos[1] = 5;
        work->viewPos[2] = 7;
        work->count = 0;
        work->clientId = GetOpponentClientID(scu->mainModule, GetPlayerClientID(scu->mainModule), 0);
        for (i = 0; i < 3; i++) {
            work->pos[i] = ViewPosToBattlePos(scu->mainModule, work->viewPos[i]);
            work->mons[i] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[i]);
            work->monIds[i] = GetMonID(work->mons[i]);
            if (!IsFainted(work->mons[i])) {
                work->count++;
            }
        }
        BtlString_FormatStdMsg(scu->strbuf, BtlvScu_GetSendOutMsg(scu, work->count), 4, work->clientId, work->monIds[0],
                               work->monIds[1], work->monIds[2]);
        BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
        if (work->count > 1) {
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
        }
        if (work->count > 2) {
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[2]), work->viewPos[2]);
        }
        BtlvEffect_StartPos(work->viewPos[0], 0x239);
        BtlvEffect_DelBallGauge(1);
        (*seq)++;
        break;
    case 3:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            if (work->count > 1) {
                BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            }
            if (work->count > 2) {
                BtlvScuGauge_Show(&scu->gauges[work->pos[2]]);
            }
            (*seq)++;
        }
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

// The player sends out one mon
static BOOL BtlvScu_StepPlayerSendOutSingle(BtlvScu *scu, s32 *seq) {
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->clientId = GetPlayerClientID(scu->mainModule);
        BtlvScu_SetupBallGauge(scu, &balls, work->clientId, 4, 0);
        BtlvEffect_AddBallGauge(&balls);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_Start(0x232);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy() && !BtlvEffect_IsBallGaugeBusy(0)) {
            work->viewPos = 0;
            work->clientId = GetPlayerClientID(scu->mainModule);
            work->pos = ViewPosToBattlePos(scu->mainModule, work->viewPos);
            work->mon = PokeCon_GetPosMonInner(scu->pokeCon, work->pos);
            work->monId = GetMonID(work->mon);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mon), work->viewPos);
            BtlvEffect_Start(0x234);
            BtlString_FormatStdMsg(scu->strbuf, 11, 1, work->monId);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_DelBallGauge(0);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos]);
            (*seq)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

// The player sends out two mons, alone or with a partner
static BOOL BtlvScu_StepPlayerSendOutDouble(BtlvScu *scu, s32 *seq) {
    u8 clientId = GetPlayerClientID(scu->mainModule);
    u8 partnerId = GetPlayerAllyClientID(scu->mainModule);

    if (partnerId == 4) {
        return BtlvScu_StepPlayerSendOutDoubleAlone(scu, seq, clientId);
    }
    return BtlvScu_StepPlayerSendOutDoubleWithPartner(scu, seq, clientId, partnerId);
}

static BOOL BtlvScu_StepPlayerSendOutDoubleAlone(BtlvScu *scu, s32 *seq, u8 clientId) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;
    u32 i;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->clientId = clientId;
        BtlvScu_SetupBallGauge(scu, &balls, work->clientId, 4, 0);
        BtlvEffect_AddBallGauge(&balls);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_Start(0x232);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy() && !BtlvEffect_IsBallGaugeBusy(0)) {
            u16 message;

            work->viewPos[0] = 2;
            work->viewPos[1] = 4;
            work->count = 0;
            work->clientId = GetPlayerClientID(scu->mainModule);
            for (i = 0; i < 2; i++) {
                BattleMon **slot;

                work->pos[i] = ViewPosToBattlePos(scu->mainModule, work->viewPos[i]);
                slot = &work->mons[i];
                *slot = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[i]);
                if (*slot != NULL) {
                    work->monIds[i] = GetMonID(*slot);
                    if (!IsFainted(*slot)) {
                        work->count++;
                    }
                }
            }
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
            if (work->count > 1) {
                BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
            }
            BtlvEffect_Start(0x234);
            message = work->count == 2 ? 12 : 11;
            BtlString_FormatStdMsg(scu->strbuf, message, 2, work->monIds[0], work->monIds[1]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_DelBallGauge(0);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            if (work->count > 1) {
                BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            }
            (*seq)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

// The player and a partner each send out a mon
static BOOL BtlvScu_StepPlayerSendOutDoubleWithPartner(BtlvScu *scu, s32 *seq, u8 clientId, u8 partnerId) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;
    u8 partnerIdx;
    BtlvBGaugeParam balls;
    u32 i;
    u8 playerIdx;

    switch (*seq) {
    case 0:
        if (clientId < partnerId) {
            BtlvScu_SetupBallGauge(scu, &balls, clientId, partnerId, 0);
        } else {
            BtlvScu_SetupBallGauge(scu, &balls, partnerId, clientId, 0);
        }
        BtlvEffect_AddBallGauge(&balls);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_Start(0x233);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy() && !BtlvEffect_IsBallGaugeBusy(0)) {
            u16 message;

            work->viewPos[0] = 2;
            work->viewPos[1] = 4;
            work->clientId = GetPlayerClientID(scu->mainModule);
            partnerIdx = GetPlayerMultiTrainerIndex(scu->mainModule) ^ 1;
            for (i = 0; i < 2; i++) {
                work->pos[i] = ViewPosToBattlePos(scu->mainModule, work->viewPos[i]);
                work->mons[i] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[i]);
                work->monIds[i] = GetMonID(work->mons[i]);
            }
            message = IsClientNPCTrainer(scu->mainModule, partnerId) ? 14 : 17;
            BtlString_FormatStdMsg(scu->strbuf, message, 2, partnerId, work->monIds[partnerIdx]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_DelBallGauge(0);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            playerIdx = GetPlayerMultiTrainerIndex(scu->mainModule);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
            BtlvEffect_Start(0x234);
            BtlString_FormatStdMsg(scu->strbuf, 11, 1, work->monIds[playerIdx]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 6:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The player sends out three mons
static BOOL BtlvScu_StepPlayerSendOutTriple(BtlvScu *scu, s32 *seq) {
    BtlvScuTripleWork *work = (BtlvScuTripleWork *)scu->work;
    u32 i;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->clientId = GetPlayerClientID(scu->mainModule);
        BtlvScu_SetupBallGauge(scu, &balls, work->clientId, 4, 0);
        BtlvEffect_AddBallGauge(&balls);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_Start(0x232);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy() && !BtlvEffect_IsBallGaugeBusy(0)) {
            u16 message;

            work->viewPos[0] = 2;
            work->viewPos[1] = 4;
            work->viewPos[2] = 6;
            work->count = 0;
            work->clientId = GetPlayerClientID(scu->mainModule);
            for (i = 0; i < 3; i++) {
                work->pos[i] = ViewPosToBattlePos(scu->mainModule, work->viewPos[i]);
                work->mons[i] = PokeCon_GetPosMonInner(scu->pokeCon, work->pos[i]);
                work->monIds[i] = GetMonID(work->mons[i]);
                if (!IsFainted(work->mons[i])) {
                    work->count++;
                }
            }
            BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[0]), work->viewPos[0]);
            if (work->count > 1) {
                BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[1]), work->viewPos[1]);
            }
            if (work->count > 2) {
                BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(work->mons[2]), work->viewPos[2]);
            }
            BtlvEffect_Start(0x234);
            if (work->count == 3) {
                message = 13;
            } else {
                message = work->count == 2 ? 12 : 11;
            }
            BtlString_FormatStdMsg(scu->strbuf, message, 3, work->monIds[0], work->monIds[1], work->monIds[2]);
            BtlvScu_StartMsg(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (BtlvScu_WaitMsg(scu)) {
            BtlvEffect_DelBallGauge(0);
            BtlvScuWinFade_StartHide(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (BtlvScuWinFade_Update(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(&scu->gauges[work->pos[0]]);
            if (work->count > 1) {
                BtlvScuGauge_Show(&scu->gauges[work->pos[1]]);
            }
            // The third gauge is tested against 1 too, where BtlvScu_StepEnemySendOutTriple tests 2; BtlvScuGauge_Show
            // skips a fainted mon either way
            if (work->count > 1) {
                BtlvScuGauge_Show(&scu->gauges[work->pos[2]]);
            }
            (*seq)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

// The message of a send-out by the count of mons and the battle type
static u16 BtlvScu_GetSendOutMsg(BtlvScu *scu, u8 count) {
    switch (BtlSetup_GetBattleType(scu->mainModule)) {
    case 0:
    default:
        return count == 1 ? 1 : 2;
    case 1:
    case 2:
        if (count == 1) {
            return 14;
        }
        if (count == 2) {
            return 15;
        }
        return 16;
    case 3:
        if (count == 1) {
            return 17;
        }
        if (count == 2) {
            return 18;
        }
        return 19;
    }
}

// Prints a message at once
void BtlvScu_PrintMsgAtOnce(BtlvScu *scu, const StrBuf *strbuf) {
    u16 color = BtlvScu_GetClearColor(scu, 12);

    GFL_BitmapFill(scu->msgBitmap, color);
    GFL_TextRndUpdateColorIndexLUT(1, 9, color);
    GFL_TextRendererDrawToBitmap(scu->msgBitmap, 0, 0, strbuf, scu->font);
    BmpWin_FlushChar(scu->msgWin);
    scu->msgSeq = 0;
    scu->waitTimer = 0;
    scu->pageWait = 0;
    scu->endWait = 0;
    scu->printedAtOnce = TRUE;
    scu->msgFinished = FALSE;
}

// Starts streaming a message, which BtlvScu_WaitMsg runs
void BtlvScu_StartMsg(BtlvScu *scu, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback) {
    u16 color = BtlvScu_GetClearColor(scu, 12);

    GFL_BitmapFill(scu->msgBitmap, color);
    GFL_TextRndUpdateColorIndexLUT(1, 9, color);
    scu->printStream =
        PrintStream_CreateWithCallback(scu->msgWin, 0, 0, strbuf, scu->font, GetTextWait(scu->mainModule),
                                       scu->tcbManager, 0, scu->heapId, color, callback);
    PrintStream_Pause(scu->printStream);
    scu->msgSeq = 0;
    scu->printedAtOnce = FALSE;
    if (wait == 0xff) {
        scu->waitTimer = 80;
        scu->endWait = 0;
    } else {
        scu->waitTimer = wait;
        scu->endWait = wait;
    }
    scu->pageWait = scu->waitTimer;
    scu->msgFinished = FALSE;
}

BOOL BtlvScu_IsMsgFinished(BtlvScu *scu) {
    if (scu->msgFinished) {
        scu->msgFinished = FALSE;
        return TRUE;
    }
    return FALSE;
}

// Runs the message: fades the window in, streams the text, waits at the pages and at the end
BOOL BtlvScu_WaitMsg(BtlvScu *scu) {
    switch (scu->msgSeq) {
    case 0:
        BtlvScuWinFade_StartShow(&scu->msgFade, scu->printedAtOnce, BtlvScu_GetClearColor(scu, 12));
        scu->msgSeq++;
    case 1:
        if (!BtlvScuWinFade_Update(&scu->msgFade)) {
            break;
        }
        if (scu->printedAtOnce) {
            scu->msgSeq = 6;
            break;
        }
        if (scu->printStream != NULL) {
            PrintStream_Resume(scu->printStream);
        }
        scu->msgSeq++;
    case 2:
        if (scu->printStream != NULL) {
            if (!GetCommMode(scu->mainModule)) {
                if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchHeld()) {
                    if (!PrintStream_IsFast(scu->printStream)) {
                        PrintStream_SetFastWait(scu->printStream, TextSpeed_GetWait(2));
                    }
                } else if (PrintStream_IsFast(scu->printStream)) {
                    PrintStream_ClearFast(scu->printStream);
                }
            }
            scu->streamState = PrintStream_GetState(scu->printStream);
            if (scu->streamState != 0) {
                if (!GetCommMode(scu->mainModule)) {
                    scu->msgSeq = 3;
                } else {
                    scu->msgSeq = 4;
                }
                if (scu->streamState == 2) {
                    scu->waitTimer = scu->endWait;
                    scu->msgFinished = TRUE;
                }
            }
        } else {
            scu->msgSeq = 6;
        }
        break;
    case 3:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed()) {
            scu->waitTimer = 0;
        }
        if (scu->waitTimer != 0) {
            scu->waitTimer--;
        } else {
            scu->msgSeq = 5;
        }
        break;
    case 4:
        if (scu->waitTimer != 0) {
            scu->waitTimer--;
        } else {
            scu->msgSeq = 5;
        }
        break;
    case 5:
        if (scu->streamState == 2) {
            GFL_TextRndResetColorIndexLUT();
            PrintStream_Delete(scu->printStream);
            scu->printStream = NULL;
            scu->msgSeq = 6;
        } else {
            PrintStream_Continue(scu->printStream);
            GFL_SndSEPlay(0x547);
            scu->msgSeq = 2;
            scu->waitTimer = scu->pageWait;
        }
        break;
    case 6:
        if (!GFL_SndIsPlaying(0x547)) {
            scu->msgSeq = 7;
            return TRUE;
        }
        break;
    case 7:
    default:
        return TRUE;
    }
    return FALSE;
}

// Starts the effect of a move
void BtlvScu_StartMoveEffect(BtlvScu *scu, u8 attackerPos, u8 targetPos, u16 move, s32 arg4, u32 arg5, u8 arg6) {
    BtlvMoveEffectParam *param = (BtlvMoveEffectParam *)scu->work;

    param->move = move;
    param->attackerPos = attackerPos;
    param->targetPos = targetPos;
    param->unk0C = arg5;
    param->unk0D = arg6;
    param->unk10 = arg4;
    BtlvScu_SetSubProc(scu, BtlvScu_MoveEffectProc);
}

static BOOL BtlvScu_MoveEffectProc(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    switch (*seq) {
    case 0:
        BtlvScuWinFade_StartHide(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&scu->msgFade)) {
            BtlvScu_ClearMsgWin(scu);
            BtlvEffect_StartMove((BtlvMoveEffectParam *)scu->work);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            GFL_SndStop();
            (*seq)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

// Runs the procedure: its init until it returns TRUE, then its main
BOOL BtlvScu_WaitMoveEffect(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

void BtlvScu_StartDamage(BtlvScu *scu, u8 pos, u16 move, BOOL arg3) {
    u32 hp = GetBattleMonStat(PokeCon_GetPosMonInner(scu->pokeCon, pos), BATTLEMON_HP);
    u8 viewPos = BattlePosToViewPos(scu->mainModule, pos);

    if (!arg3) {
        BtlvEffect_CalcGaugeHP(viewPos, hp);
        BtlvEffect_StartDamage(viewPos, move);
    } else {
        BtlvEffect_CalcGaugeHPAtOnce(viewPos, hp);
    }
}

BOOL BtlvScu_WaitDamage(BtlvScu *scu) {
    if ((BtlvEffect_IsBusy() | BtlvEffect_CheckExecuteGauge()) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScu_StartDamageEffect(BtlvScu *scu, u8 pos, u16 move) {
    BtlvEffect_StartDamage(BattlePosToViewPos(scu->mainModule, pos), move);
}

BOOL BtlvScu_WaitDamageEffect(BtlvScu *scu) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScu_StartFaint(BtlvScu *scu, u8 pos, BOOL arg2) {
    u8 viewPos = BattlePosToViewPos(scu->mainModule, pos);

    BtlvEffect_DelGauge(viewPos);
    if (!arg2) {
        BtlvEffect_StartEffect23B(viewPos);
    } else {
        BtlvEffect_DelPokemon(viewPos);
    }
}

BOOL BtlvScu_WaitFaint(BtlvScu *scu) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

// Shows a mon and its gauge
void BtlvScu_ShowMon(BtlvScu *scu, u8 pos) {
    u8 viewPos = BattlePosToViewPos(scu->mainModule, pos);

    BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(PokeCon_GetPosMonInner(scu->pokeCon, pos)), viewPos);
    BtlvScuGauge_Show(&scu->gauges[pos]);
}

BOOL BtlvScu_WaitShowMon(BtlvScu *scu) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScu_StartMemberOut(BtlvScu *scu, u8 pos, u16 effect, BOOL immediate) {
    BtlvScuGaugeHideTask *work;

    if (immediate || effect == 0) {
        BtlvEffect_DelGauge(pos);
        BtlvEffect_DelPokemon(pos);
    } else {
        work = GFL_TCBExGetData(
            GFL_TCBExMgrAddTask(scu->tcbManager, BtlvScu_MemberOutTask, sizeof(BtlvScuGaugeHideTask), 1));
        work->pos = pos;
        work->gauge = &scu->gauges[pos];
        work->count = &scu->gaugeHideTaskCount;
        // Never true here, since an effect of 0 hides the gauge at once
        if (effect == 0) {
            effect = 0x26C;
        }
        work->effect = effect;
        work->seq = 0;
        (*work->count)++;
    }
}

BOOL BtlvScu_WaitMemberOut(BtlvScu *scu) {
    if (scu->gaugeHideTaskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvScu_MemberOutTask(TCBEx *task, void *data) {
    BtlvScuGaugeHideTask *work = data;

    switch (work->seq) {
    case 0:
        BtlvEffect_StartPos(work->pos, work->effect);
        work->seq++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_DelGauge(work->pos);
            BtlvEffect_DelPokemon(work->pos);
            work->seq++;
        }
        break;
    case 2:
        (*work->count)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

void BtlvScu_StartMemberIn(BtlvScu *scu, u8 pos, u8 clientId, u8 monId, BOOL noEffect) {
    BtlvScuGaugeShowTask *work;
    BattleMon *mon;
    u8 viewPos;

    work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, BtlvScu_MemberInTask, sizeof(BtlvScuGaugeShowTask), 1));
    work->gauge = &scu->gauges[pos];
    work->count = &scu->gaugeShowTaskCount;
    work->seq = 0;
    (*work->count)++;
    mon = GetClientMonData(scu->pokeCon, clientId, monId);
    viewPos = BattlePosToViewPos(scu->mainModule, pos);
    BtlvEffect_AddPokemon(BattleMon_GetViewSrcData(mon), viewPos);
    if (!noEffect) {
        BtlvEffect_StartPos(viewPos, 0x26D);
    }
}

BOOL BtlvScu_WaitMemberIn(BtlvScu *scu) {
    if (scu->gaugeShowTaskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvScu_MemberInTask(TCBEx *task, void *data) {
    BtlvScuGaugeShowTask *work = data;

    switch (work->seq) {
    case 0:
        if (!BtlvEffect_IsBusy()) {
            BtlvScuGauge_Show(work->gauge);
            work->seq++;
        }
        break;
    case 1:
        (*work->count)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

void BtlvScu_StartGaugeHP(BtlvScu *scu, u8 pos, BOOL arg2) {
    u32 hp = GetBattleMonStat(PokeCon_GetPosMonInner(scu->pokeCon, pos), BATTLEMON_HP);
    u8 viewPos = BattlePosToViewPos(scu->mainModule, pos);

    if (arg2) {
        BtlvEffect_CalcGaugeHPAtOnce(viewPos, hp);
    } else {
        BtlvEffect_CalcGaugeHP(viewPos, hp);
    }
}

BOOL BtlvScu_WaitGaugeHP(BtlvScu *scu) {
    if (!BtlvEffect_CheckExecuteGauge()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScu_HideGaugePair(BtlvScu *scu, u8 pos1, u8 pos2) {
    BtlvScuGauge_Hide(&scu->gauges[pos1]);
    BtlvScuGauge_Hide(&scu->gauges[pos2]);
    BtlvEffect_SetNoPinchBgm(1);
}

BOOL BtlvScu_ShowGaugePair(BtlvScu *scu, u8 pos1, u8 pos2) {
    BtlvScuGauge_Show(&scu->gauges[pos1]);
    BtlvScuGauge_Show(&scu->gauges[pos2]);
    BtlvEffect_SetNoPinchBgm(0);
    return TRUE;
}

void BtlvScu_HideGauge(BtlvScu *scu, u8 pos) {
    BtlvScuGauge_Hide(&scu->gauges[pos]);
    BtlvEffect_SetNoPinchBgm(1);
}

BOOL BtlvScu_ShowGauge(BtlvScu *scu, u8 pos) {
    BtlvScuGauge_Show(&scu->gauges[pos]);
    BtlvEffect_SetNoPinchBgm(0);
    return TRUE;
}

void func_ov167_021d34ec(BtlvScu *scu, u8 viewPos) {
    BtlvEffect_StartPos(viewPos, 0x25F);
}

BOOL func_ov167_021d34fc(BtlvScu *scu, u8 viewPos) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScu_StartChangeMon(BtlvScu *scu, u8 pos, u32 arg2) {
    BtlvScuMonTask *work;

    // BTL_POS_MAX stands for no position here
    if (pos != BTL_POS_MAX) {
        work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, BtlvScu_ChangeMonTask, sizeof(BtlvScuMonTask), 1));
        work->scu = scu;
        work->pos = pos;
        work->arg2 = arg2;
        work->viewPos = BattlePosToViewPos(scu->mainModule, pos);
        work->count = &scu->taskCount;
        work->seq = 0;
        (*work->count)++;
    }
}

BOOL BtlvScu_WaitChangeMon(BtlvScu *scu) {
    if (scu->taskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvScu_ChangeMonTask(TCBEx *task, void *data) {
    BtlvScuMonTask *work = data;
    PartyPkm *pkm;

    switch (work->seq) {
    case 0:
        pkm = GetSrcData(PokeCon_GetPosMonInner(work->scu->pokeCon, work->pos));
        if (!work->arg2) {
            BtlvEffect_StartChangeSprite(pkm, work->viewPos);
        } else {
            BtlvEffect_ChangeSprite(pkm, work->viewPos);
        }
        BtlvEffect_DelGauge(work->viewPos);
        work->seq++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvEffect_AddGauge(work->scu->mainModule, PokeCon_GetPosMonInner(work->scu->pokeCon, work->pos),
                                work->viewPos);
            (*work->count)--;
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

void BtlvScu_StartTransform(BtlvScu *scu, u32 arg1, u32 index, BOOL flag) {
    BtlvScuPkmTask *work;

    work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, BtlvScu_ChangeSpriteTask, sizeof(BtlvScuPkmTask), 1));
    work->pkm = GetSrcData(PokeCon_GetPosMonInner(scu->pokeCon, ViewPosToBattlePos(scu->mainModule, index)));
    work->scu = scu;
    work->arg1 = arg1;
    work->count = &scu->taskCount;
    work->flag = flag;
    work->seq = 0;
    (*work->count)++;
}

void BtlvScu_StartChangeForm(BtlvScu *scu, u32 index, BOOL flag) {
    BtlvScuPkmTask *work;

    work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, BtlvScu_ChangeSpriteTask, sizeof(BtlvScuPkmTask), 1));
    work->pkm = GetSrcData(PokeCon_GetPosMonInner(scu->pokeCon, ViewPosToBattlePos(scu->mainModule, index)));
    work->scu = scu;
    work->arg1 = index;
    work->count = &scu->taskCount;
    work->flag = flag;
    work->seq = 0;
    (*work->count)++;
}

BOOL BtlvScu_WaitChangeSprite(BtlvScu *scu) {
    if (scu->taskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvScu_ChangeSpriteTask(TCBEx *task, void *data) {
    BtlvScuPkmTask *work = data;

    switch (work->seq) {
    case 0:
        if (!work->flag) {
            BtlvEffect_StartChangeSprite(work->pkm, work->arg1);
        } else {
            BtlvEffect_ChangeSprite(work->pkm, work->arg1);
        }
        work->seq++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            (*work->count)--;
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

#define WIN_FADE_PLANES                                                                                                \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ |               \
     GX_BLEND_PLANEMASK_BD)

static void BtlvScuWinFade_Init(BtlvScuWinFade *fade, BmpWin *window) {
    fade->window = window;
    fade->state = 0;
    fade->ev1 = FX32_CONST(31);
    fade->ev2 = FX32_CONST(7);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, WIN_FADE_PLANES, 31, 7);
}

static void BtlvScuWinFade_StartHide(BtlvScuWinFade *fade) {
    if (fade->state == 0 || fade->state == 3) {
        fade->ev1 = FX32_CONST(31);
        fade->ev2 = FX32_CONST(7);
        fade->ev1Step = -FX32_CONST(31) / 6;
        fade->ev2Step = FX32_CONST(9) / 6;
        fade->frames = 6;
        fade->state = 1;
    }
}

static void BtlvScuWinFade_StartShow(BtlvScuWinFade *fade, BOOL keepWindow, u32 color) {
    if (fade->state == 1 || fade->state == 2) {
        if (!keepWindow) {
            GFL_BitmapFill(BmpWin_GetBitmap(fade->window), color);
            BmpWin_FlushChar(fade->window);
        }
        fade->ev1 = 0;
        fade->ev2 = FX32_CONST(16);
        fade->ev1Step = FX32_CONST(31) / 6;
        fade->ev2Step = -FX32_CONST(9) / 6;
        fade->frames = 6;
        fade->state = 4;
    }
}

static BOOL BtlvScuWinFade_Update(BtlvScuWinFade *fade) {
    switch (fade->state) {
    case 2:
        return TRUE;
    case 0:
        return TRUE;
    case 4:
        fade->state = 3;
        return FALSE;
    case 3:
        if (fade->frames != 0) {
            fade->ev1 += fade->ev1Step;
            fade->ev2 += fade->ev2Step;
            fade->frames--;
        } else {
            fade->ev1 = FX32_CONST(31);
            fade->ev2 = FX32_CONST(7);
            fade->state = 0;
        }
        break;
    case 1:
        if (fade->frames != 0) {
            fade->ev1 += fade->ev1Step;
            fade->ev2 += fade->ev2Step;
            fade->frames--;
        } else {
            fade->ev1 = 0;
            fade->ev2 = FX32_CONST(16);
            fade->state = 2;
        }
        break;
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, WIN_FADE_PLANES, (u8)FX_Whole(fade->ev1),
                       (u8)FX_Whole(fade->ev2));
    return FALSE;
}

void BtlvScu_StartMsgWinHide(BtlvScu *scu) {
    BtlvScuMsgHideTask *work;

    work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, BtlvScu_MsgWinHideTask, sizeof(BtlvScuMsgHideTask), 1));
    work->scu = scu;
    work->count = &scu->taskCount;
    work->seq = 0;
    (*work->count)++;
}

BOOL BtlvScu_WaitMsgWinHide(BtlvScu *scu) {
    if (scu->taskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvScu_MsgWinHideTask(TCBEx *task, void *data) {
    BtlvScuMsgHideTask *work = data;

    switch (work->seq) {
    case 0:
        BtlvScuWinFade_StartHide(&work->scu->msgFade);
        work->seq++;
        break;
    case 1:
        if (BtlvScuWinFade_Update(&work->scu->msgFade)) {
            BtlvScu_ClearMsgWin(work->scu);
            (*work->count)--;
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

static void BtlvScu_InitGauges(BtlvScu *scu) {
    u32 i;
    u32 max;

#ifdef BUGFIX
    for (i = 0; i < BTL_POS_MAX; i++) {
#else
    // BUG: clears one gauge too many, writing over the start of abilityWins[0]
    for (i = 0; i <= BTL_POS_MAX; i++) {
#endif
        scu->gauges[i].active = FALSE;
        scu->gauges[i].mon = NULL;
    }
    if (BtlSetup_GetBattleStyle(scu->mainModule) != BTL_STYLE_ROTATION) {
        max = GetValidPosMax(scu->mainModule);
    } else {
        max = 5;
    }
    for (i = 0; i <= max; i++) {
        BtlvScuGauge_Init(&scu->gauges[i], scu, i);
    }
}

static void BtlvScu_QuitGauges(BtlvScu *scu) {
    u32 i;

    for (i = 0; i < BTL_POS_MAX; i++) {
        BtlvScuGauge_Quit(&scu->gauges[i]);
    }
}

static void BtlvScuGauge_Init(BtlvScuGauge *gauge, BtlvScu *scu, u8 pos) {
    gauge->pos = pos;
    gauge->viewPos = BattlePosToViewPos(scu->mainModule, pos);
    gauge->scu = scu;
    gauge->shown = FALSE;
    gauge->active = TRUE;
    gauge->mon = NULL;
}

static void BtlvScuGauge_Quit(BtlvScuGauge *gauge) {
    if (gauge->active) {
        if (gauge->shown) {
            BtlvEffect_DelGauge(gauge->viewPos);
            gauge->shown = FALSE;
        }
        gauge->mon = NULL;
        gauge->active = FALSE;
    }
}

static void BtlvScuGauge_Hide(BtlvScuGauge *gauge) {
    if (gauge->shown) {
        BtlvEffect_DelGauge(gauge->viewPos);
        gauge->shown = FALSE;
    }
}

static void BtlvScuGauge_Show(BtlvScuGauge *gauge) {
    if (gauge->active) {
        gauge->mon = PokeCon_GetPosMonInner(gauge->scu->pokeCon, gauge->pos);
        if (!IsFainted(gauge->mon)) {
            BtlvEffect_AddGauge(gauge->scu->mainModule, gauge->mon, gauge->viewPos);
            gauge->shown = TRUE;
        }
    }
}

static BtlvScuSide BtlvScu_GetSide(BtlMainModule *mainModule, u8 pos) {
    if (IsPlayerSide(mainModule, GetPosSide(pos))) {
        return BTLV_SCU_SIDE_PLAYER;
    }
    return BTLV_SCU_SIDE_ENEMY;
}

void BtlvScu_StartAbilityWinShow(BtlvScu *scu, u8 pos, BOOL flash) {
    BtlvScuSide side = BtlvScu_GetSide(scu->mainModule, pos);

    if (side == BTLV_SCU_SIDE_PLAYER) {
        BtlvEffect_SetTimerVisible(0, 0, 0);
    }
    BtlvEffect_ClearVoices();
    BtlvScuAbilityWin_StartShow(&scu->abilityWins[side], pos, flash);
}

BOOL BtlvScu_WaitAbilityWinShow(BtlvScu *scu, u8 pos) {
    return BtlvScuAbilityWin_RunShow(&scu->abilityWins[BtlvScu_GetSide(scu->mainModule, pos)]);
}

void BtlvScu_StartAbilityWinHide(BtlvScu *scu, u8 pos) {
    u8 side = BtlvScu_GetSide(scu->mainModule, pos);

    BtlvScuAbilityWin_Hide(&scu->abilityWins[side]);
    if (side == BTLV_SCU_SIDE_PLAYER) {
        BtlvEffect_SetTimerVisible(0, 1, 0);
    }
}

BOOL BtlvScu_WaitAbilityWinHide(BtlvScu *scu, u8 pos) {
    u8 side = BtlvScu_GetSide(scu->mainModule, pos);

    return BtlvScuAbilityWin_WaitHide(&scu->abilityWins[side]);
}

void BtlvScu_StartAbilityWinChange(BtlvScu *scu, u8 pos) {
    BtlvScuAbilityWin_StartChange(&scu->abilityWins[BtlvScu_GetSide(scu->mainModule, pos)], pos);
}

BOOL BtlvScu_WaitAbilityWinChange(BtlvScu *scu, u8 pos) {
    return BtlvScuAbilityWin_RunChange(&scu->abilityWins[BtlvScu_GetSide(scu->mainModule, pos)]);
}

static void BtlvScu_InitAbilityWins(BtlvScu *scu, u32 charOffset) {
    u32 i;

    scu->abilityCharFile = GFL_G2DIOReadBGNCGR(ARCID_BTLV, 0x1E3, FALSE, &scu->abilityChars, scu->heapId);
    for (i = 0; i < 2; i++) {
        scu->abilityWins[i].scu = scu;
        scu->abilityWins[i].side = i;
        scu->abilityWins[i].bg = i == BTLV_SCU_SIDE_PLAYER ? 2 : 3;
        scu->abilityWins[i].chars = (u8 *)scu->abilityChars->pRawData + i * 0x900;
        scu->abilityWins[i].bitmap = GFL_BitmapCreate(18, 4, 0x20, scu->heapId);
        scu->abilityWins[i].ability = 0;
        scu->abilityWins[i].monId = 0x1F;
        scu->abilityWins[i].charOffset = charOffset + i * 0x48;
        scu->abilityWins[i].shown = FALSE;
    }
}

static void BtlvScu_FreeAbilityWins(BtlvScu *scu) {
    u32 i;

    for (i = 0; i < 2; i++) {
        if (scu->abilityWins[i].bitmap != NULL) {
            GFL_BitmapFree(scu->abilityWins[i].bitmap);
            scu->abilityWins[i].bitmap = NULL;
        }
    }
    if (scu->abilityCharFile != NULL) {
        GFL_HeapFree(scu->abilityCharFile);
        scu->abilityCharFile = NULL;
    }
}

static void BtlvScuAbilityWin_StartShow(BtlvScuAbilityWin *win, u8 pos, BOOL flash) {
    BattleMon *mon = PokeCon_GetPosMonInner(win->scu->pokeCon, pos);
    u16 ability = GetBattleMonStat(mon, BATTLEMON_ABILITY);
    u8 monId = GetMonID(mon);

    if (win->shown) {
        if (monId == win->monId) {
            return;
        }
        BtlvScuAbilityWin_Hide(win);
    }
    if (ability != win->ability || monId != win->monId) {
        win->ability = ability;
        win->monId = monId;
        BtlvScuAbilityWin_DrawText(win);
    }
    win->flash = flash;
    win->seq = 0;
    BtlvEffect_SetAbility(pos, ability);
}

static BOOL BtlvScuAbilityWin_RunShow(BtlvScuAbilityWin *win) {
    PaletteFade *palFade;
    TCBManager *tcbManager;
    u16 palettes;
    u8 *src;
    u32 charPos;
    u32 i;

    switch (win->seq) {
    case 0:
        GFL_BGSysLoadChar(win->bg, win->chars, 0x900, win->charOffset);
        win->seq++;
        break;
    case 1:
        win->timer = 8;
        if (win->side == BTLV_SCU_SIDE_PLAYER) {
            win->scrollX = FX32_CONST(144);
            win->scrollStep = FX32_CONST(-18);
        } else {
            win->scrollX = FX32_CONST(-144);
            win->scrollStep = FX32_CONST(18);
        }
        GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, FX_Whole(win->scrollX));
        GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_Y, 0);
        GFL_SndSEPlay(SEQ_SE_SHOOTER);
        win->seq++;
        break;
    case 2:
        if (win->timer != 0) {
            win->scrollX += win->scrollStep;
            GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, FX_Whole(win->scrollX));
            win->timer--;
        } else {
            GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, 0);
            win->count = 0;
            win->seq++;
        }
        break;
    case 3:
        // Shows the text a column of characters a frame, in its four rows of 18
        if (win->count < 17) {
            src = GFL_BitmapGetPixelData(win->bitmap);
            charPos = win->charOffset + win->count;
            src += win->count * 0x20;
            for (i = 0; i < 4; i++) {
                GFL_BGSysLoadChar(win->bg, src + i * 0x240, 0x20, charPos + i * 18);
            }
            win->count++;
        } else {
            win->shown = TRUE;
            if (!win->flash) {
                return TRUE;
            }
            win->seq++;
        }
        break;
    case 4:
        palFade = BtlvEffect_GetPaletteFade();
        tcbManager = BtlvEffect_GetTCBManager();
        palettes = 1 << (win->side + 1);
        GFL_SndSEPlay(SEQ_SE_DECIDE6);
        PaletteFade_StartFade(palFade, 1, palettes, 0, 16, 0, 0x7FFF, tcbManager);
        win->seq++;
        break;
    case 5:
        if (PaletteFade_GetActiveMask(BtlvEffect_GetPaletteFade()) == 0) {
            win->seq++;
        }
        break;
    case 6:
        if (!GFL_SndIsPlaying(SEQ_SE_DECIDE6) && !GFL_SndIsPlaying(SEQ_SE_SHOOTER)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void BtlvScuAbilityWin_DrawText(BtlvScuAbilityWin *win) {
    BtlvScu *scu = win->scu;
    u32 size;
    u32 widths[2];
    int end;
    int x1;
    int x2;
    int overlap;
    int shift;
    int half;

    size = GFL_BitmapCalcPixelDataSize(win->bitmap);
    MIi_CpuCopy32((u8 *)win->scu->abilityChars->pRawData + win->side * 0x900, GFL_BitmapGetPixelData(win->bitmap),
                  size);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    BtlString_FormatStdMsg(scu->abilityStrbuf, 0x67, 2, win->monId, win->ability);
    if ((u8)GFL_FontGetLineWidths(scu->abilityStrbuf, scu->smallFont, 0, widths, 2) > 1) {
        // Two lines, the first from the left and the second to the right, at least 12 pixels apart
        end = widths[0] + 8;
        x2 = 0x84 - widths[1];
        overlap = end - x2;
        if (overlap < 12) {
            shift = 12 - overlap;
            half = shift / 2;
            end += half;
            x2 -= shift - half;
        }
        x1 = end - widths[0];
        if (x1 < 0) {
            x1 = 0;
        }
        if (x2 < 0) {
            x2 = 0;
        }
        GFL_FontCopyLine(scu->abilityStrbuf, scu->lineStrbuf, 0);
        GFL_TextRendererDrawToBitmap(win->bitmap, x1, 7, scu->lineStrbuf, scu->smallFont);
        GFL_FontCopyLine(scu->abilityStrbuf, scu->lineStrbuf, 1);
        GFL_TextRendererDrawToBitmap(win->bitmap, x2, 0x12, scu->lineStrbuf, scu->smallFont);
    } else {
        x1 = (0x88 - GFL_FontGetBlockWidth(scu->abilityStrbuf, scu->smallFont, 0)) / 2;
        if (x1 < 0) {
            x1 = 0;
        }
        GFL_TextRendererDrawToBitmap(win->bitmap, x1, 0x12, scu->abilityStrbuf, scu->smallFont);
    }
}

static void BtlvScuAbilityWin_Hide(BtlvScuAbilityWin *win) {
    if (win->side == BTLV_SCU_SIDE_PLAYER) {
        GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, 0x90);
    } else {
        GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, -0x90);
    }
    win->shown = FALSE;
}

static BOOL BtlvScuAbilityWin_WaitHide(BtlvScuAbilityWin *win) {
    return TRUE;
}

static void BtlvScuAbilityWin_StartChange(BtlvScuAbilityWin *win, u8 pos) {
    BattleMon *mon = PokeCon_GetPosMonInner(win->scu->pokeCon, pos);
    u16 ability = GetBattleMonStat(mon, BATTLEMON_ABILITY);
    u8 monId = GetMonID(mon);

    if (ability != win->ability || monId != win->monId) {
        win->monId = monId;
        win->ability = ability;
        BtlvScuAbilityWin_DrawText(win);
    }
    win->seq = 0;
    BtlvEffect_SetAbility(pos, ability);
}

static BOOL BtlvScuAbilityWin_RunChange(BtlvScuAbilityWin *win) {
    switch (win->seq) {
    case 0:
        G2_SetBGMosaicSize(0, 0);
        if (win->side == BTLV_SCU_SIDE_PLAYER) {
            G2_BG2Mosaic(TRUE);
        } else {
            G2_BG3Mosaic(TRUE);
        }
        win->timer = 0;
        win->count = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE3);
        win->seq++;
        break;
    case 1:
        win->timer++;
        if (win->timer > 1) {
            win->timer = 0;
            win->count++;
            G2_SetBGMosaicSize(win->count, win->count);
            if (win->count >= 15) {
                win->seq++;
            }
        }
        break;
    case 2:
        GFL_BGSysLoadChar(win->bg, GFL_BitmapGetPixelData(win->bitmap), 0x900, win->charOffset);
        win->seq++;
        break;
    case 3:
        win->timer++;
        if (win->timer > 1) {
            win->timer = 0;
            win->count--;
            G2_SetBGMosaicSize(win->count, win->count);
            if (win->count == 0) {
                win->seq++;
            }
        }
        break;
    case 4:
        if (win->side == BTLV_SCU_SIDE_PLAYER) {
            G2_BG2Mosaic(FALSE);
        } else {
            G2_BG3Mosaic(FALSE);
        }
        win->seq++;
        break;
    case 5:
        if (!GFL_SndIsPlaying(SEQ_SE_DECIDE3)) {
            win->seq++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static void BtlvScu_SetupBallGauge(BtlvScu *scu, BtlvBGaugeParam *status, u8 clientId1, u8 clientId2, s32 arg4) {
    BattleParty *parties[2] = { NULL, NULL };
    int counts[2] = { 0, 0 };
    BattleMon *mon;
    int side;
    s8 index;
    int i;

    MI_CpuFill8(status, 0, sizeof(BtlvBGaugeParam));
    parties[0] = GetClientParty(scu->pokeCon, clientId1);
    counts[0] = GetNumMonsInParty(parties[0]);
    if (clientId2 < 4) {
        parties[1] = GetClientParty(scu->pokeCon, clientId2);
        counts[1] = GetNumMonsInParty(parties[1]);
        if (counts[0] > 3) {
            counts[0] = 3;
        }
        if (counts[1] > 3) {
            counts[1] = 3;
        }
    }
    status->side = arg4;
    status->mode = GetStudioMode(scu->mainModule);
    for (i = 0; i < 6; i++) {
        if (i < counts[0] + counts[1]) {
            if (i < counts[0]) {
                side = 0;
                index = i;
            } else {
                side = 1;
                index = i - counts[0];
            }
            mon = GetBattleMonFromParty(parties[side], index);
            if (IsFainted(mon)) {
                status->balls[side * 3 + index] = 2;
            } else if (GetBattleMonStatus(mon)) {
                status->balls[side * 3 + index] = 3;
            } else if (!CanPokemonBattle(mon)) {
                status->balls[side * 3 + index] = 0;
            } else {
                status->balls[side * 3 + index] = 1;
            }
        }
    }
}

void BtlvScu_StartWaitMsg(BtlvScu *scu) {
    BtlString_LoadUIMsg(scu->strbuf, 0xF);
    BtlvScu_StartMsg(scu, scu->strbuf, 0, NULL);
}

BOOL BtlvScu_WaitWaitMsg(BtlvScu *scu) {
    return BtlvScu_WaitMsg(scu);
}

void BtlvScu_HideWaitMsg(BtlvScu *scu) {
}

void BtlvScu_StartFadeOut(BtlvScu *scu) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK, 0, 16, -3);
}

void BtlvScu_StartFadeIn(BtlvScu *scu) {
    GFL_BitmapFill(scu->msgBitmap, 12);
    BmpWin_FlushChar(scu->msgWin);
    GFL_FadeSet(FADE_ENGINE_A_BLACK, 16, 0, -3);
}

BOOL BtlvScu_IsFadeDone(BtlvScu *scu) {
    return BtlvScu_CheckFadeDone(scu);
}

void BtlvScu_StartLevelUpWinOpen(BtlvScu *scu, BattleMon *mon, const BattleMonLevelUp *levelUp) {
    scu->levelUpMon = mon;
    scu->levelUp = *levelUp;
    BtlvScu_SetSubProc(scu, BtlvScu_LevelUpWinOpenProc);
}

BOOL BtlvScu_WaitLevelUpWinOpen(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

void BtlvScu_StartLevelUpWinStats(BtlvScu *scu) {
    BtlvScu_SetSubProc(scu, BtlvScu_LevelUpWinStatsProc);
}

BOOL BtlvScu_WaitLevelUpWinStats(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

void BtlvScu_StartLevelUpWinClose(BtlvScu *scu) {
    BtlvScu_SetSubProc(scu, BtlvScu_LevelUpWinCloseProc);
}

BOOL BtlvScu_WaitLevelUpWinClose(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

// Opens the stat window with the gains of the level-up
static BOOL BtlvScu_LevelUpWinOpenProc(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    switch (*seq) {
    case 0:
        GFL_TextRndUpdateColorIndexLUT(1, 9, 12);
        BmpWin_FlushMap(scu->statWin);
        BmpWin_MakeFrameScreen(scu->statWin, scu->statFrameCharOffset, 3);
        GFL_BitmapFill(scu->statBitmap, 0xC);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(scu->statWin));
        (*seq)++;
        break;
    case 1:
        BtlString_FormatLevelUpGains(scu->strbuf, scu->levelUp.hp, scu->levelUp.attack, scu->levelUp.defense,
                                     scu->levelUp.spAttack, scu->levelUp.spDefense, scu->levelUp.speed);
        GFL_TextRendererDrawToBitmap(scu->statBitmap, 0, 0, scu->strbuf, scu->font);
        (*seq)++;
        break;
    case 2:
        BmpWin_FlushChar(scu->statWin);
        (*seq)++;
        break;
    case 3:
        GFL_BGSysMoveBG(3, BG_MOVE_SET_X, 0);
        GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, 0x100);
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        (*seq)++;
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

// Shows the new stats in the stat window
static BOOL BtlvScu_LevelUpWinStatsProc(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    switch (*seq) {
    case 0:
        BattleMon_GetSrcStats(scu->levelUpMon, &scu->levelUp);
        BtlString_FormatLevelUpStats(scu->strbuf, scu->levelUp.hp, scu->levelUp.attack, scu->levelUp.defense,
                                     scu->levelUp.spAttack, scu->levelUp.spDefense, scu->levelUp.speed);
        GFL_BitmapFill(scu->statBitmap, 0xC);
        GFL_TextRendererDrawToBitmap(scu->statBitmap, 0, 0, scu->strbuf, scu->font);
        (*seq)++;
        break;
    case 1:
        BmpWin_FlushChar(scu->statWin);
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        (*seq)++;
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

// Closes the stat window
static BOOL BtlvScu_LevelUpWinCloseProc(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    int x;
    int y;
    int width;
    int height;
    u8 bg;

    switch (*seq) {
    case 0:
        x = BmpWin_GetPosX(scu->statWin) - 1;
        y = BmpWin_GetPosY(scu->statWin) - 1;
        width = BmpWin_GetSizeX(scu->statWin) + 2;
        height = BmpWin_GetSizeY(scu->statWin) + 2;
        bg = BmpWin_GetBGIndex(scu->statWin);
        GFL_BGSysFillScrArea(bg, 0, x, y, width, height, 0);
        GFL_BGSysLoadScr(bg);
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        (*seq)++;
        break;
    case 1:
        GFL_BGSysMoveBG(3, BG_MOVE_SET_X, -0x90);
        GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, 0);
        (*seq)++;
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

// The color the message window is cleared with: none in some mode of the battle
static u16 BtlvScu_GetClearColor(BtlvScu *scu, u16 color) {
    if (GetStudioMode(scu->mainModule) == 2) {
        color = 0;
    }
    return color;
}

static void BtlvScu_ClearMsgWin(BtlvScu *scu) {
    GFL_BitmapFill(scu->msgBitmap, BtlvScu_GetClearColor(scu, 0xC));
    BmpWin_FlushChar(scu->msgWin);
}
