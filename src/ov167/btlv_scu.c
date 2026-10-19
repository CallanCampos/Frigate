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
// func_ov167_021d0c24 through func_ov167_021d4590. func_ov167_021d0c24 allocates it with
// GFL_HeapAllocate(heapId, 0x180, TRUE, <btlv_scu.c>, 351). Swan has no names for this file; the names are ours, from
// use.

// A window faded in and out by the BG1 alpha blend, its coefficients in fx32 (func_ov167_021d36ec, 021d3718,
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

// The HP gauge of a battle position (func_ov167_021d392c, 021d3950, 021d3974, 021d398c)
typedef struct BtlvScuGauge {
    BattleMon *mon; // 0x00
    BtlvScu *scu;   // 0x04
    u8 pos;         // 0x08
    u8 viewPos;     // 0x09 func_ov167_0219c6dc of pos
    u8 shown;       // 0x0A set once BtlvEffect_AddGauge shows it, cleared with BtlvEffect_DelGauge
    u8 active;      // 0x0B the position exists in this battle style
} BtlvScuGauge;     // size 0x0C

// An ability pop-up, one for each side, on BG2 or BG3 (func_ov167_021d3ac4, and 021d3bb4 through 021d3f4c)
typedef struct BtlvScuAbilityWin {
    BtlvScu *scu;      // 0x00
    GFLBitmap *bitmap; // 0x04 18x4 characters
    u16 ability;       // 0x08 GetBattleMonStat(mon, 0x10)
    u8 side;           // 0x0A the index, 0 or 1, from func_ov167_021d39c4
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
    u8 flash : 1;      // 0x22 bit 1, the last argument of func_ov167_021d3bb4: go on to the palette flash
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
    u8 taskCount;                      // 0x040 tasks of func_ov167_021d3510, 021d35e0, 021d363c, 021d3830
    u8 gaugeShowTaskCount;             // 0x041 tasks of func_ov167_021d3354
    u8 gaugeHideTaskCount;             // 0x042 tasks of func_ov167_021d3298
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
    u8 printedAtOnce;                  // 0x13E set by func_ov167_021d2dbc: the window is neither cleared nor streamed
    u8 msgSeq;                         // 0x13F the state of func_ov167_021d2edc, 7 when done
    u8 clientId;                       // 0x140 only stored
    u8 msgFinished;                    // 0x141 set when the stream reports PRINT_STREAM_DONE, read and cleared by
                                       //       func_ov167_021d2ec4
    u32 streamState;                   // 0x144 PrintStream_GetState
    u16 waitTimer;                     // 0x148
    u16 pageWait;                      // 0x14A the wait after a paused page
    u16 endWait;                       // 0x14C the wait after the last page
    u8 unk14E;                         // 0x14E only cleared, in func_ov167_021d0c24
    u8 encountStep;                    // 0x14F the step through the tables of the encounter procedures
    u8 recPlay;               // 0x150 the argument of func_ov167_021d1084, a recorded battle: skip the encounter
    BtlvScuWinFade msgFade;   // 0x154
    BattleMon *levelUpMon;    // 0x16C
    BattleMonLevelUp levelUp; // 0x170 the gains, then the new stats, which func_ov167_021bb10c writes over
}; // size 0x180

// The works of the procedures, which cast scu->work

// One mon: func_ov167_021d1404 (wild single), 021d15f8 (trainer single), 021d215c, 021d26c4
typedef struct BtlvScuSingleWork {
    BattleMon *mon;  // 0x00
    u8 pos;          // 0x04 func_ov167_0219c744 of viewPos
    u8 viewPos;      // 0x05
    u8 monId;        // 0x06
    u8 clientId;     // 0x07 of the trainer, or of the player
    u8 unk08;        // 0x08 func_ov167_0219c8d0(mainModule, playerClientId, 0), a client ID for the message
} BtlvScuSingleWork; // size 0x0C

// Two wild mons: func_ov167_021d18b8, 021d1d64. The arrays are in another order than in BtlvScuDoubleWork
typedef struct BtlvScuWildDoubleWork {
    BattleMon *mons[2];  // 0x00
    u8 monIds[2];        // 0x08
    u8 pos[2];           // 0x0A
    u8 viewPos[2];       // 0x0C
    u8 unk0E;            // 0x0E never accessed
    u8 clientId;         // 0x0F
} BtlvScuWildDoubleWork; // size 0x10

// Two mons sent out together: func_ov167_021d2284, 021d23d4, 021d2820, 021d29bc
typedef struct BtlvScuDoubleWork {
    BattleMon *mons[2]; // 0x00
    u8 pos[2];          // 0x08
    u8 viewPos[2];      // 0x0A
    u8 monIds[2];       // 0x0C
    u8 count;           // 0x0E of the mons not fainted
    u8 clientId;        // 0x0F
} BtlvScuDoubleWork;    // size 0x10

// Three: func_ov167_021d252c, 021d2b88
typedef struct BtlvScuTripleWork {
    BattleMon *mons[3]; // 0x00
    u8 pos[3];          // 0x0C
    u8 viewPos[3];      // 0x0F
    u8 monIds[3];       // 0x12
    u8 count;           // 0x15
    u8 clientId;        // 0x16
} BtlvScuTripleWork;    // size 0x18

// The sprites of the opposing trainers: func_ov167_021d2028 uses both, func_ov167_021d1f1c only viewPos[1]
typedef struct BtlvScuTrainerWork {
    u8 viewPos[2];
} BtlvScuTrainerWork;

// The works of the TCBEx tasks, as GFL_TCBExMgrAddTask sizes them

// func_ov167_021d3298 and its task func_ov167_021d3304: an effect, then the gauge hidden
typedef struct BtlvScuGaugeHideTask {
    BtlvScuGauge *gauge; // 0x00
    u16 seq;             // 0x04
    u16 pos;             // 0x06
    u16 effect;          // 0x08 0x26C if func_ov167_021d3298 got 0
    u8 *count;           // 0x0C &scu->gaugeHideTaskCount
} BtlvScuGaugeHideTask;  // size 0x10

// func_ov167_021d3354 and func_ov167_021d33e0: the gauge shown once the effect ends
typedef struct BtlvScuGaugeShowTask {
    BtlvScuGauge *gauge; // 0x00
    u16 seq;             // 0x04
    u8 *count;           // 0x08 &scu->gaugeShowTaskCount
} BtlvScuGaugeShowTask;  // size 0x0C

// func_ov167_021d3510 and func_ov167_021d3568
typedef struct BtlvScuMonTask {
    BtlvScu *scu; // 0x00
    u8 pos;       // 0x04
    u32 viewPos;  // 0x08
    s32 seq;      // 0x0C
    u32 arg2;     // 0x10 BtlvEffect_ChangeSprite rather than 021df6b4 if set
    u8 *count;    // 0x14 &scu->taskCount
} BtlvScuMonTask; // size 0x18

// func_ov167_021d35e0, func_ov167_021d363c and their task func_ov167_021d36a4
typedef struct BtlvScuPkmTask {
    BtlvScu *scu;  // 0x00
    u32 arg1;      // 0x04 the second argument of BtlvEffect_StartChangeSprite or 021df76c
    PartyPkm *pkm; // 0x08
    s32 seq;       // 0x0C
    u8 unk10;      // 0x10 never accessed
    u8 flag;       // 0x11 BtlvEffect_ChangeSprite rather than 021df6b4 if set
    u8 *count;     // 0x14 &scu->taskCount
} BtlvScuPkmTask;  // size 0x18

// func_ov167_021d3830 and func_ov167_021d386c: msgFade hides the message window, which is then cleared
typedef struct BtlvScuMsgHideTask {
    BtlvScu *scu;     // 0x00
    s32 seq;          // 0x04
    u8 *count;        // 0x08 &scu->taskCount
} BtlvScuMsgHideTask; // size 0x0C

// A step of an encounter sequence, run until it returns TRUE
typedef BOOL (*BtlvScuEncountStep)(BtlvScu *scu, s32 *seq);

// The index of an ability pop-up, by the side of the battle position, from func_ov167_021d39c4. It returns an enum,
// as two returns kept in this order show
typedef enum BtlvScuSide {
    BTLV_SCU_SIDE_PLAYER,
    BTLV_SCU_SIDE_ENEMY,
} BtlvScuSide;

static u16 func_ov167_021d0f28(BtlvScu *scu, u32 arcId, u32 fileId, u32 bg);
static BOOL func_ov167_021d1070(BtlvScu *scu);
static void func_ov167_021d12f8(s32 wait);
static BOOL func_ov167_021d1328(BtlvScu *scu, s32 *seq, const u8 *viewPos, u16 count);
static BOOL func_ov167_021d13d4(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d13e4(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d13f4(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d1404(s32 *seq, void *arg);
static u16 func_ov167_021d1598(BtlvScu *scu);
static BOOL func_ov167_021d15f8(s32 *seq, void *arg);
static BOOL func_ov167_021d1858(s32 *seq, void *arg);
static BOOL func_ov167_021d18b8(s32 *seq, void *arg);
static BOOL func_ov167_021d1b28(s32 *seq, void *arg);
static BOOL func_ov167_021d1b74(s32 *seq, void *arg);
static BOOL func_ov167_021d1bc0(s32 *seq, void *arg);
static BOOL func_ov167_021d1c0c(s32 *seq, void *arg);
static BOOL func_ov167_021d1c6c(s32 *seq, void *arg);
static BOOL func_ov167_021d1cb8(s32 *seq, void *arg);
static BOOL func_ov167_021d1d04(s32 *seq, void *arg);
static BOOL func_ov167_021d1d64(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d1ec0(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d1ee0(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d1f1c(BtlvScu *scu, s32 *seq, u8 clientId);
static BOOL func_ov167_021d2028(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2);
static BOOL func_ov167_021d215c(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d2248(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d2284(BtlvScu *scu, s32 *seq, u8 clientId);
static BOOL func_ov167_021d23d4(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2);
static BOOL func_ov167_021d252c(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d26c4(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d27e8(BtlvScu *scu, s32 *seq);
static BOOL func_ov167_021d2820(BtlvScu *scu, s32 *seq, u8 clientId);
static BOOL func_ov167_021d29bc(BtlvScu *scu, s32 *seq, u8 clientId, u8 partnerId);
static BOOL func_ov167_021d2b88(BtlvScu *scu, s32 *seq);
static u16 func_ov167_021d2d64(BtlvScu *scu, u8 count);
static BOOL func_ov167_021d30d4(s32 *seq, void *arg);
static void func_ov167_021d3304(TCBEx *task, void *data);
static void func_ov167_021d33e0(TCBEx *task, void *data);
static void func_ov167_021d3568(TCBEx *task, void *data);
static void func_ov167_021d36a4(TCBEx *task, void *data);
static void func_ov167_021d36ec(BtlvScuWinFade *fade, BmpWin *window);
static void func_ov167_021d3718(BtlvScuWinFade *fade);
static void func_ov167_021d3748(BtlvScuWinFade *fade, BOOL keepWindow, u32 color);
static BOOL func_ov167_021d3798(BtlvScuWinFade *fade);
static void func_ov167_021d386c(TCBEx *task, void *data);
static void func_ov167_021d38b8(BtlvScu *scu);
static void func_ov167_021d3910(BtlvScu *scu);
static void func_ov167_021d392c(BtlvScuGauge *gauge, BtlvScu *scu, u8 pos);
static void func_ov167_021d3950(BtlvScuGauge *gauge);
static void func_ov167_021d3974(BtlvScuGauge *gauge);
static void func_ov167_021d398c(BtlvScuGauge *gauge);
static BtlvScuSide func_ov167_021d39c4(BtlMainModule *mainModule, u8 pos);
static void func_ov167_021d3ac4(BtlvScu *scu, u32 charOffset);
static void func_ov167_021d3b74(BtlvScu *scu);
static void func_ov167_021d3bb4(BtlvScuAbilityWin *win, u8 pos, BOOL flash);
static BOOL func_ov167_021d3c34(BtlvScuAbilityWin *win);
static void func_ov167_021d3ddc(BtlvScuAbilityWin *win);
static void func_ov167_021d3ed4(BtlvScuAbilityWin *win);
static BOOL func_ov167_021d3f00(BtlvScuAbilityWin *win);
static void func_ov167_021d3f04(BtlvScuAbilityWin *win, u8 pos);
static BOOL func_ov167_021d3f4c(BtlvScuAbilityWin *win);
static void func_ov167_021d408c(BtlvScu *scu, BtlvBGaugeParam *status, u8 clientId1, u8 clientId2, s32 arg4);
static BOOL func_ov167_021d437c(s32 *seq, void *arg);
static BOOL func_ov167_021d4448(s32 *seq, void *arg);
static BOOL func_ov167_021d44d8(s32 *seq, void *arg);
static u16 func_ov167_021d4578(BtlvScu *scu, u16 color);
static void func_ov167_021d4590(BtlvScu *scu);

// The view positions whose mons a recorded battle shows at once, by style
static const u8 sSingleViewPos[] = { 1, 0 };
static const u8 sDoubleViewPos[] = { 2, 3, 4, 5 };
static const u8 sTripleViewPos[] = { 2, 3, 4, 5, 6, 7 };

// The steps of the encounter sequences, by func_ov167_021d1084's choice. The two-dimensional ones go by step, then by
// the player's client ID. The 12-byte tables are declared in an order that MWCC's size sort lays out as the game
// does
static const BtlvScuEncountStep sDoubleWildMultiSteps[] = { func_ov167_021d1d64, func_ov167_021d27e8 };
static const BtlvScuEncountStep sDoubleTrainerMultiSteps[] = { func_ov167_021d1ee0, func_ov167_021d2248,
                                                               func_ov167_021d27e8 };
static const BtlvScuEncountStep sTripleSteps[] = { func_ov167_021d1ec0, func_ov167_021d252c, func_ov167_021d2b88 };
static const BtlvScuEncountStep sDoubleTrainerSteps[] = { func_ov167_021d1ec0, func_ov167_021d2248,
                                                          func_ov167_021d27e8 };
static const BtlvScuEncountStep sDoubleCommMultiSteps[] = { func_ov167_021d1ee0, func_ov167_021d2248,
                                                            func_ov167_021d27e8 };
// clang-format off
static const BtlvScuEncountStep sTripleCommSteps[][2] = {
    {func_ov167_021d1ec0, func_ov167_021d1ec0},
    {func_ov167_021d252c, func_ov167_021d2b88},
    {func_ov167_021d2b88, func_ov167_021d252c},
};

// clang-format on
// clang-format off
static const BtlvScuEncountStep sSingleCommSteps[][2] = {
    {func_ov167_021d1ec0, func_ov167_021d1ec0},
    {func_ov167_021d215c, func_ov167_021d26c4},
    {func_ov167_021d26c4, func_ov167_021d215c},
};

// clang-format on
// clang-format off
static const BtlvScuEncountStep sDoubleCommSteps[][2] = {
    {func_ov167_021d1ec0, func_ov167_021d1ec0},
    {func_ov167_021d2248, func_ov167_021d27e8},
    {func_ov167_021d27e8, func_ov167_021d2248},
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

BtlvScu *func_ov167_021d0c24(BtlvCore *core, BtlMainModule *mainModule, BtlPokeCon *pokeCon, TCBExManager *tcbManager,
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

void func_ov167_021d0cd4(BtlvScu *scu) {
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
    frameChar = func_ov167_021d0f28(scu, 11, 0x164, 1);
    scu->abilityCharOffset = func_ov167_021d0f28(scu, 11, 0x1e3, 3);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTLV, 0x1e5, 2, 0, 0, FALSE, scu->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTLV, 0x1e6, 3, 0, 0, FALSE, scu->heapId);
    scu->msgWin = BmpWin_CreateDynamic(1, 1, 19, 30, 4, 0, 0);
    scu->msgBitmap = BmpWin_GetBitmap(scu->msgWin);
    BmpWin_FlushMap(scu->msgWin);
    if (func_ov167_0219c988(scu->mainModule) != 2) {
        BmpWin_MakeFrameScreen(scu->msgWin, frameChar, 0);
    }
    GFL_BitmapFill(scu->msgBitmap, func_ov167_021d4578(scu, 12));
    BmpWin_FlushChar(scu->msgWin);
    scu->statFrameCharOffset = func_ov167_021d0f28(scu, 11, 0x1eb, 3);
    scu->statWin = BmpWin_CreateDynamic(3, 21, 37, 10, 12, 3, 0);
    scu->statBitmap = BmpWin_GetBitmap(scu->statWin);
    func_ov167_021d36ec(&scu->msgFade, scu->msgWin);
    func_ov167_021d38b8(scu);
    func_ov167_021d3ac4(scu, scu->abilityCharOffset);
    GFL_BGSysLoadScr(1);
    GFL_BGSysLoadScr(2);
    GFL_BGSysLoadScr(3);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    if (func_ov167_0219c988(scu->mainModule) != 2) {
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

static u16 func_ov167_021d0f28(BtlvScu *scu, u32 arcId, u32 fileId, u32 bg) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(arcId, HEAPID_TAIL(scu->heapId));
    u32 offset = GFL_BGSysLoadArcNCGRDynamic(arc, fileId, bg, 0, FALSE, HEAPID_TAIL(scu->heapId));

    GFL_ArcToolFree(arc);
    return offset;
}

void func_ov167_021d0f84(BtlvScu *scu) {
    func_ov167_021d3b74(scu);
    func_ov167_021d3910(scu);
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

void func_ov167_021d0ff8(const BtlvScu *scu) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BTLV, HEAPID_TAIL(scu->heapId));

    GFL_BGSysLoadArcNCGRStatic(arc, 0x1e3, 3, scu->abilityCharOffset, 0, FALSE, scu->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0x1eb, 3, scu->statFrameCharOffset, 0, FALSE, scu->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysClearScr(3);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTLV, 0x1e6, 3, 0, 0, FALSE, scu->heapId);
}

static BOOL func_ov167_021d1070(BtlvScu *scu) {
    if (!GFL_FadeIsRunning()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d1084(BtlvScu *scu, BOOL recPlay) {
    u32 type = BtlSetup_GetBattleType(scu->mainModule);

    scu->encountStep = 0;
    scu->recPlay = recPlay;
    switch (BtlSetup_GetBattleStyle(scu->mainModule)) {
    case BTL_STYLE_SINGLE:
        switch (type) {
        case 0:
        case 4:
        default:
            BtlvScu_SetSubProc(scu, func_ov167_021d1404);
            break;
        case 1:
        case 2:
            BtlvScu_SetSubProc(scu, func_ov167_021d15f8);
            break;
        case 3:
            BtlvScu_SetSubProc(scu, func_ov167_021d1858);
            break;
        }
        break;
    case BTL_STYLE_DOUBLE:
        switch (type) {
        case 0:
            if (!func_ov167_0219beec(scu->mainModule)) {
                BtlvScu_SetSubProc(scu, func_ov167_021d18b8);
            } else {
                BtlvScu_SetSubProc(scu, func_ov167_021d1b28);
            }
            break;
        case 3:
            if (!func_ov167_0219beec(scu->mainModule)) {
                BtlvScu_SetSubProc(scu, func_ov167_021d1c0c);
            } else {
                BtlvScu_SetSubProc(scu, func_ov167_021d1c6c);
            }
            break;
        case 1:
        case 2:
            if (!func_ov167_0219beec(scu->mainModule)) {
                BtlvScu_SetSubProc(scu, func_ov167_021d1b74);
            } else {
                BtlvScu_SetSubProc(scu, func_ov167_021d1bc0);
            }
            break;
        }
        break;
    case BTL_STYLE_TRIPLE:
        if (type != 3) {
            BtlvScu_SetSubProc(scu, func_ov167_021d1cb8);
        } else {
            BtlvScu_SetSubProc(scu, func_ov167_021d1d04);
        }
        break;
    case BTL_STYLE_ROTATION:
        if (type != 3) {
            BtlvScu_SetSubProc(scu, func_ov167_021d1cb8);
        } else {
            BtlvScu_SetSubProc(scu, func_ov167_021d1d04);
        }
        break;
    }
}

BOOL func_ov167_021d12a0(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

static void func_ov167_021d12f8(s32 wait) {
    if (GXx_GetMasterBrightness_(REG_MASTER_BRIGHT_ADDR) <= 0) {
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, wait);
    } else {
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, wait);
    }
}

// A recorded battle shows the mons and their gauges at once
static BOOL func_ov167_021d1328(BtlvScu *scu, s32 *seq, const u8 *viewPos, u16 count) {
    u8 view;
    u8 pos;
    u32 i;

    switch (*seq) {
    case 0:
        for (i = 0; i < count; i++) {
            view = viewPos[i];
            pos = func_ov167_0219c744(scu->mainModule, view);
            BtlvEffect_AddPokemon(func_ov167_021bb064(func_ov167_0219d188(scu->pokeCon, pos)), view);
            func_ov167_021d398c(&scu->gauges[pos]);
        }
        (*seq)++;
        break;
    case 1:
        if (scu->recPlay) {
            return TRUE;
        }
        func_ov167_021d12f8(-3);
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

static BOOL func_ov167_021d13d4(BtlvScu *scu, s32 *seq) {
    return func_ov167_021d1328(scu, seq, sSingleViewPos, NELEMS(sSingleViewPos));
}

static BOOL func_ov167_021d13e4(BtlvScu *scu, s32 *seq) {
    return func_ov167_021d1328(scu, seq, sDoubleViewPos, NELEMS(sDoubleViewPos));
}

static BOOL func_ov167_021d13f4(BtlvScu *scu, s32 *seq) {
    return func_ov167_021d1328(scu, seq, sTripleViewPos, NELEMS(sTripleViewPos));
}

// A wild single battle
static BOOL func_ov167_021d1404(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;

    if (scu->recPlay) {
        return func_ov167_021d13d4(scu, seq);
    }
    switch (*seq) {
    case 0:
        work->viewPos = 1;
        work->pos = func_ov167_0219c744(scu->mainModule, work->viewPos);
        work->mon = func_ov167_0219d188(scu->pokeCon, work->pos);
        work->monId = GetMonID(work->mon);
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mon), work->viewPos);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_StartPos(work->viewPos, 0x231);
        func_ov167_021d12f8(2);
        (*seq)++;
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d4ec0(scu->strbuf, func_ov167_021d1598(scu), 1, work->monId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d398c(&scu->gauges[work->pos]);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov167_021d3798(&scu->msgFade)) {
            BtlvEffect_Start(BtlSetup_GetBattleType(scu->mainModule) == 4 ? 0x23c : 0x232);
            (*seq)++;
        }
        break;
    case 6:
        if (!BtlvEffect_IsBusy()) {
            work->viewPos = 0;
            work->pos = func_ov167_0219c744(scu->mainModule, work->viewPos);
            work->mon = func_ov167_0219d188(scu->pokeCon, work->pos);
            work->monId = GetMonID(work->mon);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mon), work->viewPos);
            BtlvEffect_Start(0x234);
            func_ov167_021d4ec0(scu->strbuf, 11, 1, work->monId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 7:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 8:
        if (func_ov167_021d3798(&scu->msgFade)) {
            (*seq)++;
        }
        break;
    case 9:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos]);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The message that a wild mon appeared, by the battle's type
static u16 func_ov167_021d1598(BtlvScu *scu) {
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
static BOOL func_ov167_021d15f8(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;

    if (scu->recPlay) {
        return func_ov167_021d13d4(scu, seq);
    }
    switch (*seq) {
    case 0: {
        u8 pos = func_ov167_0219c744(scu->mainModule, 1);
        u8 clientId = func_ov167_0219c650(scu->mainModule, pos);

        BtlvEffect_SetTrainer(func_ov167_0219d938(scu->mainModule, clientId), 9, 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(9, 0);
        work->viewPos = 1;
        work->clientId = clientId;
        work->pos = pos;
        work->mon = func_ov167_0219d188(scu->pokeCon, work->pos);
        work->monId = GetMonID(work->mon);
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            BtlvEffect_StartPos(work->viewPos, 0x237);
            func_ov167_021d12f8(2);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            BtlvBGaugeParam balls;

            func_ov167_021d408c(scu, &balls, work->clientId, 4, 1);
            BtlvEffect_AddBallGauge(&balls);
            func_ov167_021d4ec0(scu->strbuf, 7, 1, work->clientId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_StartPos(work->viewPos, 0x238);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d4ec0(scu->strbuf, 14, 2, 1, work->monId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mon), work->viewPos);
            BtlvEffect_StartPos(work->viewPos, 0x239);
            BtlvEffect_DelBallGauge(1);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 6:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            BtlvBGaugeParam balls;

            func_ov167_021d408c(scu, &balls, GetPlayerClientID(scu->mainModule), 4, 0);
            BtlvEffect_AddBallGauge(&balls);
            (*seq)++;
        }
        break;
    case 7:
        func_ov167_021d398c(&scu->gauges[work->pos]);
        BtlvEffect_Start(0x232);
        (*seq)++;
        break;
    case 8:
        if (!BtlvEffect_IsBusy() && !BtlvEffect_IsBallGaugeBusy(0)) {
            work->viewPos = 0;
            work->pos = func_ov167_0219c744(scu->mainModule, work->viewPos);
            work->mon = func_ov167_0219d188(scu->pokeCon, work->pos);
            work->monId = GetMonID(work->mon);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mon), work->viewPos);
            BtlvEffect_Start(0x234);
            func_ov167_021d4ec0(scu->strbuf, 11, 1, work->monId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 9:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_DelBallGauge(0);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 10:
        if (func_ov167_021d3798(&scu->msgFade)) {
            func_ov167_021d4590(scu);
            (*seq)++;
        }
        break;
    case 11:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos]);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// A single battle over communication
static BOOL func_ov167_021d1858(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13d4(scu, seq);
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
static BOOL func_ov167_021d18b8(s32 *seq, void *arg) {
    BtlvScu *scu = arg;
    BtlvScuWildDoubleWork *work = (BtlvScuWildDoubleWork *)scu->work;
    u8 viewPos;
    u32 i;

    if (scu->recPlay) {
        return func_ov167_021d13e4(scu, seq);
    }
    switch (*seq) {
    case 0:
        viewPos = func_ov167_0219c458(scu->mainModule, scu->clientId, 0);
        work->pos[0] = func_ov167_0219c4bc(scu->mainModule, viewPos, 0);
        work->mons[0] = func_ov167_0219d188(scu->pokeCon, work->pos[0]);
        work->monIds[0] = GetMonID(work->mons[0]);
        work->pos[1] = func_ov167_0219c4bc(scu->mainModule, viewPos, 1);
        work->mons[1] = func_ov167_0219d188(scu->pokeCon, work->pos[1]);
        work->monIds[1] = GetMonID(work->mons[1]);
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            viewPos = func_ov167_0219c6dc(scu->mainModule, work->pos[0]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), viewPos);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            viewPos = func_ov167_0219c6dc(scu->mainModule, work->pos[1]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), viewPos);
            BtlvEffect_StartPos(viewPos, 0x231);
            func_ov167_021d12f8(2);
            (*seq)++;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d4ec0(scu->strbuf, 2, 2, work->monIds[0], work->monIds[1]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov167_021d3798(&scu->msgFade)) {
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
                work->pos[i] = func_ov167_0219c744(scu->mainModule, work->viewPos[i]);
                work->mons[i] = func_ov167_0219d188(scu->pokeCon, work->pos[i]);
                if (work->mons[i] != NULL) {
                    work->monIds[i] = GetMonID(work->mons[i]);
                }
            }
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
            BtlvEffect_Start(0x234);
            func_ov167_021d4ec0(scu->strbuf, 12, 2, work->monIds[0], work->monIds[1]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 7:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 8:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            func_ov167_021d398c(&scu->gauges[work->pos[1]]);
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

static BOOL func_ov167_021d1b28(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13e4(scu, seq);
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

static BOOL func_ov167_021d1b74(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13e4(scu, seq);
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

static BOOL func_ov167_021d1bc0(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13e4(scu, seq);
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

static BOOL func_ov167_021d1c0c(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13e4(scu, seq);
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

static BOOL func_ov167_021d1c6c(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13e4(scu, seq);
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

static BOOL func_ov167_021d1cb8(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13f4(scu, seq);
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

static BOOL func_ov167_021d1d04(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    if (scu->recPlay) {
        return func_ov167_021d13f4(scu, seq);
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
static BOOL func_ov167_021d1d64(BtlvScu *scu, s32 *seq) {
    BtlvScuWildDoubleWork *work = (BtlvScuWildDoubleWork *)scu->work;
    u8 viewPos;

    switch (*seq) {
    case 0:
        viewPos = func_ov167_0219c458(scu->mainModule, scu->clientId, 0);
        work->pos[0] = func_ov167_0219c4bc(scu->mainModule, viewPos, 0);
        work->mons[0] = func_ov167_0219d188(scu->pokeCon, work->pos[0]);
        work->monIds[0] = GetMonID(work->mons[0]);
        work->pos[1] = func_ov167_0219c4bc(scu->mainModule, viewPos, 1);
        work->mons[1] = func_ov167_0219d188(scu->pokeCon, work->pos[1]);
        work->monIds[1] = GetMonID(work->mons[1]);
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            viewPos = func_ov167_0219c6dc(scu->mainModule, work->pos[0]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), viewPos);
            (*seq)++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            viewPos = func_ov167_0219c6dc(scu->mainModule, work->pos[1]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), viewPos);
            BtlvEffect_StartPos(viewPos, 0x231);
            func_ov167_021d12f8(2);
            (*seq)++;
        }
        break;
    case 3:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d4ec0(scu->strbuf, 2, 2, work->monIds[0], work->monIds[1]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov167_021d3798(&scu->msgFade)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov167_021d1ec0(BtlvScu *scu, s32 *seq) {
    return func_ov167_021d1f1c(scu, seq, func_ov167_0219c8b8(scu->mainModule, 0));
}

static BOOL func_ov167_021d1ee0(BtlvScu *scu, s32 *seq) {
    u8 clientId1 = func_ov167_0219c8b8(scu->mainModule, 0);
    u8 clientId2 = func_ov167_0219c8b8(scu->mainModule, 1);

    if (clientId1 != clientId2) {
        return func_ov167_021d2028(scu, seq, clientId1, clientId2);
    }
    return func_ov167_021d1f1c(scu, seq, clientId1);
}

// One opposing trainer sends out a mon
static BOOL func_ov167_021d1f1c(BtlvScu *scu, s32 *seq, u8 clientId) {
    BtlvScuTrainerWork *work = (BtlvScuTrainerWork *)scu->work;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->viewPos[1] = 9;
        BtlvEffect_SetTrainer(func_ov167_0219d938(scu->mainModule, clientId), work->viewPos[1], 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(work->viewPos[1], 0);
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            BtlvEffect_StartPos(work->viewPos[1], 0x237);
            func_ov167_021d12f8(2);
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

            func_ov167_021d408c(scu, &balls, clientId, 4, 1);
            BtlvEffect_AddBallGauge(&balls);
            func_ov167_021d4ec0(scu->strbuf, message, 1, clientId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_StartPos(work->viewPos[1], 0x238);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Two opposing trainers
static BOOL func_ov167_021d2028(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2) {
    BtlvScuTrainerWork *work = (BtlvScuTrainerWork *)scu->work;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 11;
        work->viewPos[1] = 13;
        BtlvEffect_SetTrainer(func_ov167_0219d938(scu->mainModule, clientId1), work->viewPos[0], 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(work->viewPos[0], 0);
        BtlvEffect_SetTrainer(func_ov167_0219d938(scu->mainModule, clientId2), work->viewPos[1], 0, 0, 0);
        BtlvEffect_SetPokeAnimeSpeed(work->viewPos[1], 0);
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            BtlvEffect_StartPos(work->viewPos[0], 0x237);
            func_ov167_021d12f8(2);
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

            func_ov167_021d408c(scu, &balls, clientId1, clientId2, 1);
            BtlvEffect_AddBallGauge(&balls);
            func_ov167_021d4ec0(scu->strbuf, message, 2, clientId1, clientId2);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_StartPos(work->viewPos[0], 0x238);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The opponent sends out one mon
static BOOL func_ov167_021d215c(BtlvScu *scu, s32 *seq) {
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;

    switch (*seq) {
    case 0:
        work->viewPos = 1;
        work->unk08 = func_ov167_0219c8d0(scu->mainModule, GetPlayerClientID(scu->mainModule), 0);
        work->pos = func_ov167_0219c744(scu->mainModule, work->viewPos);
        work->mon = func_ov167_0219d188(scu->pokeCon, work->pos);
        work->monId = GetMonID(work->mon);
        func_ov167_021d4ec0(scu->strbuf, func_ov167_021d2d64(scu, 1), 2, work->unk08, work->monId);
        func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_AddPokemon(func_ov167_021bb064(work->mon), work->viewPos);
        BtlvEffect_StartPos(work->viewPos, 0x239);
        BtlvEffect_DelBallGauge(1);
        (*seq)++;
        break;
    case 3:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos]);
            (*seq)++;
        }
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

// The opponents send out two mons, from one trainer or from two
static BOOL func_ov167_021d2248(BtlvScu *scu, s32 *seq) {
    u8 clientId1 = func_ov167_0219c8b8(scu->mainModule, 0);
    u8 clientId2 = func_ov167_0219c8b8(scu->mainModule, 1);

    if (clientId1 != clientId2) {
        return func_ov167_021d23d4(scu, seq, clientId1, clientId2);
    }
    return func_ov167_021d2284(scu, seq, clientId1);
}

static BOOL func_ov167_021d2284(BtlvScu *scu, s32 *seq, u8 clientId) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;
    u32 i;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 3;
        work->viewPos[1] = 5;
        work->clientId = clientId;
        work->count = 0;
        for (i = 0; i < 2; i++) {
            work->pos[i] = func_ov167_0219c744(scu->mainModule, work->viewPos[i]);
            work->mons[i] = func_ov167_0219d188(scu->pokeCon, work->pos[i]);
            work->monIds[i] = GetMonID(work->mons[i]);
            if (!IsFainted(work->mons[i])) {
                work->count++;
            }
        }
        func_ov167_021d4ec0(scu->strbuf, func_ov167_021d2d64(scu, work->count), 3, work->clientId, work->monIds[0],
                            work->monIds[1]);
        func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
        if (work->count == 2) {
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
        }
        BtlvEffect_StartPos(work->viewPos[0], 0x239);
        BtlvEffect_DelBallGauge(1);
        (*seq)++;
        break;
    case 3:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            if (work->count == 2) {
                func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            }
            (*seq)++;
        }
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov167_021d23d4(BtlvScu *scu, s32 *seq, u8 clientId1, u8 clientId2) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 3;
        work->pos[0] = func_ov167_0219c744(scu->mainModule, work->viewPos[0]);
        work->mons[0] = func_ov167_0219d188(scu->pokeCon, work->pos[0]);
        work->monIds[0] = GetMonID(work->mons[0]);
        work->viewPos[1] = 5;
        work->pos[1] = func_ov167_0219c744(scu->mainModule, work->viewPos[1]);
        work->mons[1] = func_ov167_0219d188(scu->pokeCon, work->pos[1]);
        work->monIds[1] = GetMonID(work->mons[1]);
        func_ov167_021d4ec0(scu->strbuf, func_ov167_021d2d64(scu, 1), 2, clientId1, work->monIds[0]);
        func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d3798(&scu->msgFade)) {
            func_ov167_021d4ec0(scu->strbuf, func_ov167_021d2d64(scu, 1), 2, clientId2, work->monIds[1]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
            BtlvEffect_StartPos(work->viewPos[0], 0x239);
            BtlvEffect_DelBallGauge(1);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            (*seq)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

// The opponents send out three mons
static BOOL func_ov167_021d252c(BtlvScu *scu, s32 *seq) {
    BtlvScuTripleWork *work = (BtlvScuTripleWork *)scu->work;
    u32 i;

    switch (*seq) {
    case 0:
        work->viewPos[0] = 3;
        work->viewPos[1] = 5;
        work->viewPos[2] = 7;
        work->count = 0;
        work->clientId = func_ov167_0219c8d0(scu->mainModule, GetPlayerClientID(scu->mainModule), 0);
        for (i = 0; i < 3; i++) {
            work->pos[i] = func_ov167_0219c744(scu->mainModule, work->viewPos[i]);
            work->mons[i] = func_ov167_0219d188(scu->pokeCon, work->pos[i]);
            work->monIds[i] = GetMonID(work->mons[i]);
            if (!IsFainted(work->mons[i])) {
                work->count++;
            }
        }
        func_ov167_021d4ec0(scu->strbuf, func_ov167_021d2d64(scu, work->count), 4, work->clientId, work->monIds[0],
                            work->monIds[1], work->monIds[2]);
        func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 2:
        BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
        if (work->count > 1) {
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
        }
        if (work->count > 2) {
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[2]), work->viewPos[2]);
        }
        BtlvEffect_StartPos(work->viewPos[0], 0x239);
        BtlvEffect_DelBallGauge(1);
        (*seq)++;
        break;
    case 3:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            if (work->count > 1) {
                func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            }
            if (work->count > 2) {
                func_ov167_021d398c(&scu->gauges[work->pos[2]]);
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
static BOOL func_ov167_021d26c4(BtlvScu *scu, s32 *seq) {
    BtlvScuSingleWork *work = (BtlvScuSingleWork *)scu->work;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->clientId = GetPlayerClientID(scu->mainModule);
        func_ov167_021d408c(scu, &balls, work->clientId, 4, 0);
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
            work->pos = func_ov167_0219c744(scu->mainModule, work->viewPos);
            work->mon = func_ov167_0219d188(scu->pokeCon, work->pos);
            work->monId = GetMonID(work->mon);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mon), work->viewPos);
            BtlvEffect_Start(0x234);
            func_ov167_021d4ec0(scu->strbuf, 11, 1, work->monId);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_DelBallGauge(0);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos]);
            (*seq)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

// The player sends out two mons, alone or with a partner
static BOOL func_ov167_021d27e8(BtlvScu *scu, s32 *seq) {
    u8 clientId = GetPlayerClientID(scu->mainModule);
    u8 partnerId = func_ov167_0219c86c(scu->mainModule);

    if (partnerId == 4) {
        return func_ov167_021d2820(scu, seq, clientId);
    }
    return func_ov167_021d29bc(scu, seq, clientId, partnerId);
}

static BOOL func_ov167_021d2820(BtlvScu *scu, s32 *seq, u8 clientId) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;
    u32 i;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->clientId = clientId;
        func_ov167_021d408c(scu, &balls, work->clientId, 4, 0);
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

                work->pos[i] = func_ov167_0219c744(scu->mainModule, work->viewPos[i]);
                slot = &work->mons[i];
                *slot = func_ov167_0219d188(scu->pokeCon, work->pos[i]);
                if (*slot != NULL) {
                    work->monIds[i] = GetMonID(*slot);
                    if (!IsFainted(*slot)) {
                        work->count++;
                    }
                }
            }
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
            if (work->count > 1) {
                BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
            }
            BtlvEffect_Start(0x234);
            message = work->count == 2 ? 12 : 11;
            func_ov167_021d4ec0(scu->strbuf, message, 2, work->monIds[0], work->monIds[1]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_DelBallGauge(0);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            if (work->count > 1) {
                func_ov167_021d398c(&scu->gauges[work->pos[1]]);
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
static BOOL func_ov167_021d29bc(BtlvScu *scu, s32 *seq, u8 clientId, u8 partnerId) {
    BtlvScuDoubleWork *work = (BtlvScuDoubleWork *)scu->work;
    u8 partnerIdx;
    BtlvBGaugeParam balls;
    u32 i;
    u8 playerIdx;

    switch (*seq) {
    case 0:
        if (clientId < partnerId) {
            func_ov167_021d408c(scu, &balls, clientId, partnerId, 0);
        } else {
            func_ov167_021d408c(scu, &balls, partnerId, clientId, 0);
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
            partnerIdx = func_ov167_0219c850(scu->mainModule) ^ 1;
            for (i = 0; i < 2; i++) {
                work->pos[i] = func_ov167_0219c744(scu->mainModule, work->viewPos[i]);
                work->mons[i] = func_ov167_0219d188(scu->pokeCon, work->pos[i]);
                work->monIds[i] = GetMonID(work->mons[i]);
            }
            message = func_ov167_0219d888(scu->mainModule, partnerId) ? 14 : 17;
            func_ov167_021d4ec0(scu->strbuf, message, 2, partnerId, work->monIds[partnerIdx]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_DelBallGauge(0);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade)) {
            playerIdx = func_ov167_0219c850(scu->mainModule);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
            BtlvEffect_Start(0x234);
            func_ov167_021d4ec0(scu->strbuf, 11, 1, work->monIds[playerIdx]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov167_021d2edc(scu)) {
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 6:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The player sends out three mons
static BOOL func_ov167_021d2b88(BtlvScu *scu, s32 *seq) {
    BtlvScuTripleWork *work = (BtlvScuTripleWork *)scu->work;
    u32 i;
    BtlvBGaugeParam balls;

    switch (*seq) {
    case 0:
        work->clientId = GetPlayerClientID(scu->mainModule);
        func_ov167_021d408c(scu, &balls, work->clientId, 4, 0);
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
                work->pos[i] = func_ov167_0219c744(scu->mainModule, work->viewPos[i]);
                work->mons[i] = func_ov167_0219d188(scu->pokeCon, work->pos[i]);
                work->monIds[i] = GetMonID(work->mons[i]);
                if (!IsFainted(work->mons[i])) {
                    work->count++;
                }
            }
            BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[0]), work->viewPos[0]);
            if (work->count > 1) {
                BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[1]), work->viewPos[1]);
            }
            if (work->count > 2) {
                BtlvEffect_AddPokemon(func_ov167_021bb064(work->mons[2]), work->viewPos[2]);
            }
            BtlvEffect_Start(0x234);
            if (work->count == 3) {
                message = 13;
            } else {
                message = work->count == 2 ? 12 : 11;
            }
            func_ov167_021d4ec0(scu->strbuf, message, 3, work->monIds[0], work->monIds[1], work->monIds[2]);
            func_ov167_021d2e20(scu, scu->strbuf, 80, NULL);
            (*seq)++;
        }
        break;
    case 3:
        if (func_ov167_021d2edc(scu)) {
            BtlvEffect_DelBallGauge(0);
            func_ov167_021d3718(&scu->msgFade);
            (*seq)++;
        }
        break;
    case 4:
        if (func_ov167_021d3798(&scu->msgFade) && !BtlvEffect_IsBusy()) {
            func_ov167_021d398c(&scu->gauges[work->pos[0]]);
            if (work->count > 1) {
                func_ov167_021d398c(&scu->gauges[work->pos[1]]);
            }
            // The third gauge is tested against 1 too, where func_ov167_021d252c tests 2; func_ov167_021d398c skips
            // a fainted mon either way
            if (work->count > 1) {
                func_ov167_021d398c(&scu->gauges[work->pos[2]]);
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
static u16 func_ov167_021d2d64(BtlvScu *scu, u8 count) {
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
void func_ov167_021d2dbc(BtlvScu *scu, const StrBuf *strbuf) {
    u16 color = func_ov167_021d4578(scu, 12);

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

// Starts streaming a message, which func_ov167_021d2edc runs
void func_ov167_021d2e20(BtlvScu *scu, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback) {
    u16 color = func_ov167_021d4578(scu, 12);

    GFL_BitmapFill(scu->msgBitmap, color);
    GFL_TextRndUpdateColorIndexLUT(1, 9, color);
    scu->printStream =
        PrintStream_CreateWithCallback(scu->msgWin, 0, 0, strbuf, scu->font, func_ov167_0219bde0(scu->mainModule),
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

BOOL func_ov167_021d2ec4(BtlvScu *scu) {
    if (scu->msgFinished) {
        scu->msgFinished = FALSE;
        return TRUE;
    }
    return FALSE;
}

// Runs the message: fades the window in, streams the text, waits at the pages and at the end
BOOL func_ov167_021d2edc(BtlvScu *scu) {
    switch (scu->msgSeq) {
    case 0:
        func_ov167_021d3748(&scu->msgFade, scu->printedAtOnce, func_ov167_021d4578(scu, 12));
        scu->msgSeq++;
    case 1:
        if (!func_ov167_021d3798(&scu->msgFade)) {
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
            if (!func_ov167_0219bee4(scu->mainModule)) {
                if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
                    if (!PrintStream_IsFast(scu->printStream)) {
                        PrintStream_SetFastWait(scu->printStream, TextSpeed_GetWait(2));
                    }
                } else if (PrintStream_IsFast(scu->printStream)) {
                    PrintStream_ClearFast(scu->printStream);
                }
            }
            scu->streamState = PrintStream_GetState(scu->printStream);
            if (scu->streamState != 0) {
                if (!func_ov167_0219bee4(scu->mainModule)) {
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
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
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
void func_ov167_021d3094(BtlvScu *scu, u8 attackerPos, u8 targetPos, u16 move, s32 arg4, u32 arg5, u8 arg6) {
    BtlvMoveEffectParam *param = (BtlvMoveEffectParam *)scu->work;

    param->move = move;
    param->attackerPos = attackerPos;
    param->targetPos = targetPos;
    param->unk0C = arg5;
    param->unk0D = arg6;
    param->unk10 = arg4;
    BtlvScu_SetSubProc(scu, func_ov167_021d30d4);
}

static BOOL func_ov167_021d30d4(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    switch (*seq) {
    case 0:
        func_ov167_021d3718(&scu->msgFade);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3798(&scu->msgFade)) {
            func_ov167_021d4590(scu);
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
BOOL func_ov167_021d3130(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

void func_ov167_021d3188(BtlvScu *scu, u8 pos, u16 move, BOOL arg3) {
    u32 hp = GetBattleMonStat(func_ov167_0219d188(scu->pokeCon, pos), BATTLEMON_HP);
    u8 viewPos = func_ov167_0219c6dc(scu->mainModule, pos);

    if (!arg3) {
        BtlvEffect_CalcGaugeHP(viewPos, hp);
        BtlvEffect_StartDamage(viewPos, move);
    } else {
        BtlvEffect_CalcGaugeHPAtOnce(viewPos, hp);
    }
}

BOOL func_ov167_021d31d0(BtlvScu *scu) {
    if ((BtlvEffect_IsBusy() | BtlvEffect_CheckExecuteGauge()) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d31e8(BtlvScu *scu, u8 pos, u16 move) {
    BtlvEffect_StartDamage(func_ov167_0219c6dc(scu->mainModule, pos), move);
}

BOOL func_ov167_021d3200(BtlvScu *scu) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d3214(BtlvScu *scu, u8 pos, BOOL arg2) {
    u8 viewPos = func_ov167_0219c6dc(scu->mainModule, pos);

    BtlvEffect_DelGauge(viewPos);
    if (!arg2) {
        BtlvEffect_StartEffect23B(viewPos);
    } else {
        BtlvEffect_DelPokemon(viewPos);
    }
}

BOOL func_ov167_021d323c(BtlvScu *scu) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

// Shows a mon and its gauge
void func_ov167_021d3250(BtlvScu *scu, u8 pos) {
    u8 viewPos = func_ov167_0219c6dc(scu->mainModule, pos);

    BtlvEffect_AddPokemon(func_ov167_021bb064(func_ov167_0219d188(scu->pokeCon, pos)), viewPos);
    func_ov167_021d398c(&scu->gauges[pos]);
}

BOOL func_ov167_021d3284(BtlvScu *scu) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d3298(BtlvScu *scu, u8 pos, u16 effect, BOOL immediate) {
    BtlvScuGaugeHideTask *work;

    if (immediate || effect == 0) {
        BtlvEffect_DelGauge(pos);
        BtlvEffect_DelPokemon(pos);
    } else {
        work = GFL_TCBExGetData(
            GFL_TCBExMgrAddTask(scu->tcbManager, func_ov167_021d3304, sizeof(BtlvScuGaugeHideTask), 1));
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

BOOL func_ov167_021d32f4(BtlvScu *scu) {
    if (scu->gaugeHideTaskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov167_021d3304(TCBEx *task, void *data) {
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

void func_ov167_021d3354(BtlvScu *scu, u8 pos, u8 clientId, u8 monId, BOOL noEffect) {
    BtlvScuGaugeShowTask *work;
    BattleMon *mon;
    u8 viewPos;

    work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, func_ov167_021d33e0, sizeof(BtlvScuGaugeShowTask), 1));
    work->gauge = &scu->gauges[pos];
    work->count = &scu->gaugeShowTaskCount;
    work->seq = 0;
    (*work->count)++;
    mon = GetClientMonData(scu->pokeCon, clientId, monId);
    viewPos = func_ov167_0219c6dc(scu->mainModule, pos);
    BtlvEffect_AddPokemon(func_ov167_021bb064(mon), viewPos);
    if (!noEffect) {
        BtlvEffect_StartPos(viewPos, 0x26D);
    }
}

BOOL func_ov167_021d33d0(BtlvScu *scu) {
    if (scu->gaugeShowTaskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov167_021d33e0(TCBEx *task, void *data) {
    BtlvScuGaugeShowTask *work = data;

    switch (work->seq) {
    case 0:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d398c(work->gauge);
            work->seq++;
        }
        break;
    case 1:
        (*work->count)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

void func_ov167_021d3414(BtlvScu *scu, u8 pos, BOOL arg2) {
    u32 hp = GetBattleMonStat(func_ov167_0219d188(scu->pokeCon, pos), BATTLEMON_HP);
    u8 viewPos = func_ov167_0219c6dc(scu->mainModule, pos);

    if (arg2) {
        BtlvEffect_CalcGaugeHPAtOnce(viewPos, hp);
    } else {
        BtlvEffect_CalcGaugeHP(viewPos, hp);
    }
}

BOOL func_ov167_021d3450(BtlvScu *scu) {
    if (!BtlvEffect_CheckExecuteGauge()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d3464(BtlvScu *scu, u8 pos1, u8 pos2) {
    func_ov167_021d3974(&scu->gauges[pos1]);
    func_ov167_021d3974(&scu->gauges[pos2]);
    BtlvEffect_SetNoPinchBgm(1);
}

BOOL func_ov167_021d3490(BtlvScu *scu, u8 pos1, u8 pos2) {
    func_ov167_021d398c(&scu->gauges[pos1]);
    func_ov167_021d398c(&scu->gauges[pos2]);
    BtlvEffect_SetNoPinchBgm(0);
    return TRUE;
}

void func_ov167_021d34bc(BtlvScu *scu, u8 pos) {
    func_ov167_021d3974(&scu->gauges[pos]);
    BtlvEffect_SetNoPinchBgm(1);
}

BOOL func_ov167_021d34d4(BtlvScu *scu, u8 pos) {
    func_ov167_021d398c(&scu->gauges[pos]);
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

void func_ov167_021d3510(BtlvScu *scu, u8 pos, u32 arg2) {
    BtlvScuMonTask *work;

    // BTL_POS_MAX stands for no position here
    if (pos != BTL_POS_MAX) {
        work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, func_ov167_021d3568, sizeof(BtlvScuMonTask), 1));
        work->scu = scu;
        work->pos = pos;
        work->arg2 = arg2;
        work->viewPos = func_ov167_0219c6dc(scu->mainModule, pos);
        work->count = &scu->taskCount;
        work->seq = 0;
        (*work->count)++;
    }
}

BOOL func_ov167_021d3558(BtlvScu *scu) {
    if (scu->taskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov167_021d3568(TCBEx *task, void *data) {
    BtlvScuMonTask *work = data;
    PartyPkm *pkm;

    switch (work->seq) {
    case 0:
        pkm = GetSrcData(func_ov167_0219d188(work->scu->pokeCon, work->pos));
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
            BtlvEffect_AddGauge(work->scu->mainModule, func_ov167_0219d188(work->scu->pokeCon, work->pos),
                                work->viewPos);
            (*work->count)--;
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

void func_ov167_021d35e0(BtlvScu *scu, u32 arg1, u32 index, BOOL flag) {
    BtlvScuPkmTask *work;

    work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, func_ov167_021d36a4, sizeof(BtlvScuPkmTask), 1));
    work->pkm = GetSrcData(func_ov167_0219d188(scu->pokeCon, func_ov167_0219c744(scu->mainModule, index)));
    work->scu = scu;
    work->arg1 = arg1;
    work->count = &scu->taskCount;
    work->flag = flag;
    work->seq = 0;
    (*work->count)++;
}

void func_ov167_021d363c(BtlvScu *scu, u32 index, BOOL flag) {
    BtlvScuPkmTask *work;

    work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, func_ov167_021d36a4, sizeof(BtlvScuPkmTask), 1));
    work->pkm = GetSrcData(func_ov167_0219d188(scu->pokeCon, func_ov167_0219c744(scu->mainModule, index)));
    work->scu = scu;
    work->arg1 = index;
    work->count = &scu->taskCount;
    work->flag = flag;
    work->seq = 0;
    (*work->count)++;
}

BOOL func_ov167_021d3694(BtlvScu *scu) {
    if (scu->taskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov167_021d36a4(TCBEx *task, void *data) {
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

static void func_ov167_021d36ec(BtlvScuWinFade *fade, BmpWin *window) {
    fade->window = window;
    fade->state = 0;
    fade->ev1 = FX32_CONST(31);
    fade->ev2 = FX32_CONST(7);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, WIN_FADE_PLANES, 31, 7);
}

static void func_ov167_021d3718(BtlvScuWinFade *fade) {
    if (fade->state == 0 || fade->state == 3) {
        fade->ev1 = FX32_CONST(31);
        fade->ev2 = FX32_CONST(7);
        fade->ev1Step = -FX32_CONST(31) / 6;
        fade->ev2Step = FX32_CONST(9) / 6;
        fade->frames = 6;
        fade->state = 1;
    }
}

static void func_ov167_021d3748(BtlvScuWinFade *fade, BOOL keepWindow, u32 color) {
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

static BOOL func_ov167_021d3798(BtlvScuWinFade *fade) {
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

void func_ov167_021d3830(BtlvScu *scu) {
    BtlvScuMsgHideTask *work;

    work = GFL_TCBExGetData(GFL_TCBExMgrAddTask(scu->tcbManager, func_ov167_021d386c, sizeof(BtlvScuMsgHideTask), 1));
    work->scu = scu;
    work->count = &scu->taskCount;
    work->seq = 0;
    (*work->count)++;
}

BOOL func_ov167_021d385c(BtlvScu *scu) {
    if (scu->taskCount == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov167_021d386c(TCBEx *task, void *data) {
    BtlvScuMsgHideTask *work = data;

    switch (work->seq) {
    case 0:
        func_ov167_021d3718(&work->scu->msgFade);
        work->seq++;
        break;
    case 1:
        if (func_ov167_021d3798(&work->scu->msgFade)) {
            func_ov167_021d4590(work->scu);
            (*work->count)--;
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

static void func_ov167_021d38b8(BtlvScu *scu) {
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
        func_ov167_021d392c(&scu->gauges[i], scu, i);
    }
}

static void func_ov167_021d3910(BtlvScu *scu) {
    u32 i;

    for (i = 0; i < BTL_POS_MAX; i++) {
        func_ov167_021d3950(&scu->gauges[i]);
    }
}

static void func_ov167_021d392c(BtlvScuGauge *gauge, BtlvScu *scu, u8 pos) {
    gauge->pos = pos;
    gauge->viewPos = func_ov167_0219c6dc(scu->mainModule, pos);
    gauge->scu = scu;
    gauge->shown = FALSE;
    gauge->active = TRUE;
    gauge->mon = NULL;
}

static void func_ov167_021d3950(BtlvScuGauge *gauge) {
    if (gauge->active) {
        if (gauge->shown) {
            BtlvEffect_DelGauge(gauge->viewPos);
            gauge->shown = FALSE;
        }
        gauge->mon = NULL;
        gauge->active = FALSE;
    }
}

static void func_ov167_021d3974(BtlvScuGauge *gauge) {
    if (gauge->shown) {
        BtlvEffect_DelGauge(gauge->viewPos);
        gauge->shown = FALSE;
    }
}

static void func_ov167_021d398c(BtlvScuGauge *gauge) {
    if (gauge->active) {
        gauge->mon = func_ov167_0219d188(gauge->scu->pokeCon, gauge->pos);
        if (!IsFainted(gauge->mon)) {
            BtlvEffect_AddGauge(gauge->scu->mainModule, gauge->mon, gauge->viewPos);
            gauge->shown = TRUE;
        }
    }
}

static BtlvScuSide func_ov167_021d39c4(BtlMainModule *mainModule, u8 pos) {
    if (func_ov167_0219c43c(mainModule, func_ov167_0219d3bc(pos))) {
        return BTLV_SCU_SIDE_PLAYER;
    }
    return BTLV_SCU_SIDE_ENEMY;
}

void func_ov167_021d39e4(BtlvScu *scu, u8 pos, BOOL flash) {
    BtlvScuSide side = func_ov167_021d39c4(scu->mainModule, pos);

    if (side == BTLV_SCU_SIDE_PLAYER) {
        BtlvEffect_SetTimerVisible(0, 0, 0);
    }
    BtlvEffect_ClearVoices();
    func_ov167_021d3bb4(&scu->abilityWins[side], pos, flash);
}

BOOL func_ov167_021d3a1c(BtlvScu *scu, u8 pos) {
    return func_ov167_021d3c34(&scu->abilityWins[func_ov167_021d39c4(scu->mainModule, pos)]);
}

void func_ov167_021d3a38(BtlvScu *scu, u8 pos) {
    u8 side = func_ov167_021d39c4(scu->mainModule, pos);

    func_ov167_021d3ed4(&scu->abilityWins[side]);
    if (side == BTLV_SCU_SIDE_PLAYER) {
        BtlvEffect_SetTimerVisible(0, 1, 0);
    }
}

BOOL func_ov167_021d3a68(BtlvScu *scu, u8 pos) {
    u8 side = func_ov167_021d39c4(scu->mainModule, pos);

    return func_ov167_021d3f00(&scu->abilityWins[side]);
}

void func_ov167_021d3a88(BtlvScu *scu, u8 pos) {
    func_ov167_021d3f04(&scu->abilityWins[func_ov167_021d39c4(scu->mainModule, pos)], pos);
}

BOOL func_ov167_021d3aa8(BtlvScu *scu, u8 pos) {
    return func_ov167_021d3f4c(&scu->abilityWins[func_ov167_021d39c4(scu->mainModule, pos)]);
}

static void func_ov167_021d3ac4(BtlvScu *scu, u32 charOffset) {
    u32 i;

    scu->abilityCharFile = GFL_G2DIOReadBGNCGR(ARCID_BTLV, 0x1E3, FALSE, &scu->abilityChars, scu->heapId);
    for (i = 0; i < 2; i++) {
        scu->abilityWins[i].scu = scu;
        scu->abilityWins[i].side = i;
        scu->abilityWins[i].bg = i == BTLV_SCU_SIDE_PLAYER ? 2 : 3;
        scu->abilityWins[i].chars = (u8 *)scu->abilityChars->rawData + i * 0x900;
        scu->abilityWins[i].bitmap = GFL_BitmapCreate(18, 4, 0x20, scu->heapId);
        scu->abilityWins[i].ability = 0;
        scu->abilityWins[i].monId = 0x1F;
        scu->abilityWins[i].charOffset = charOffset + i * 0x48;
        scu->abilityWins[i].shown = FALSE;
    }
}

static void func_ov167_021d3b74(BtlvScu *scu) {
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

static void func_ov167_021d3bb4(BtlvScuAbilityWin *win, u8 pos, BOOL flash) {
    BattleMon *mon = func_ov167_0219d188(win->scu->pokeCon, pos);
    u16 ability = GetBattleMonStat(mon, BATTLEMON_ABILITY);
    u8 monId = GetMonID(mon);

    if (win->shown) {
        if (monId == win->monId) {
            return;
        }
        func_ov167_021d3ed4(win);
    }
    if (ability != win->ability || monId != win->monId) {
        win->ability = ability;
        win->monId = monId;
        func_ov167_021d3ddc(win);
    }
    win->flash = flash;
    win->seq = 0;
    BtlvEffect_SetAbility(pos, ability);
}

static BOOL func_ov167_021d3c34(BtlvScuAbilityWin *win) {
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

static void func_ov167_021d3ddc(BtlvScuAbilityWin *win) {
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
    MIi_CpuCopy32((u8 *)win->scu->abilityChars->rawData + win->side * 0x900, GFL_BitmapGetPixelData(win->bitmap), size);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    func_ov167_021d4ec0(scu->abilityStrbuf, 0x67, 2, win->monId, win->ability);
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

static void func_ov167_021d3ed4(BtlvScuAbilityWin *win) {
    if (win->side == BTLV_SCU_SIDE_PLAYER) {
        GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, 0x90);
    } else {
        GFL_BGSysMoveBG(win->bg, BG_MOVE_SET_X, -0x90);
    }
    win->shown = FALSE;
}

static BOOL func_ov167_021d3f00(BtlvScuAbilityWin *win) {
    return TRUE;
}

static void func_ov167_021d3f04(BtlvScuAbilityWin *win, u8 pos) {
    BattleMon *mon = func_ov167_0219d188(win->scu->pokeCon, pos);
    u16 ability = GetBattleMonStat(mon, BATTLEMON_ABILITY);
    u8 monId = GetMonID(mon);

    if (ability != win->ability || monId != win->monId) {
        win->monId = monId;
        win->ability = ability;
        func_ov167_021d3ddc(win);
    }
    win->seq = 0;
    BtlvEffect_SetAbility(pos, ability);
}

static BOOL func_ov167_021d3f4c(BtlvScuAbilityWin *win) {
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

static void func_ov167_021d408c(BtlvScu *scu, BtlvBGaugeParam *status, u8 clientId1, u8 clientId2, s32 arg4) {
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
    status->mode = func_ov167_0219c988(scu->mainModule);
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

void func_ov167_021d4194(BtlvScu *scu) {
    func_ov167_021d575c(scu->strbuf, 0xF);
    func_ov167_021d2e20(scu, scu->strbuf, 0, NULL);
}

BOOL func_ov167_021d41b0(BtlvScu *scu) {
    return func_ov167_021d2edc(scu);
}

void func_ov167_021d41b8(BtlvScu *scu) {
}

void func_ov167_021d41bc(BtlvScu *scu) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK, 0, 16, -3);
}

void func_ov167_021d41d0(BtlvScu *scu) {
    GFL_BitmapFill(scu->msgBitmap, 12);
    BmpWin_FlushChar(scu->msgWin);
    GFL_FadeSet(FADE_ENGINE_A_BLACK, 16, 0, -3);
}

BOOL func_ov167_021d41f0(BtlvScu *scu) {
    return func_ov167_021d1070(scu);
}

void func_ov167_021d41f8(BtlvScu *scu, BattleMon *mon, const BattleMonLevelUp *levelUp) {
    scu->levelUpMon = mon;
    scu->levelUp = *levelUp;
    BtlvScu_SetSubProc(scu, func_ov167_021d437c);
}

BOOL func_ov167_021d4234(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

void func_ov167_021d428c(BtlvScu *scu) {
    BtlvScu_SetSubProc(scu, func_ov167_021d4448);
}

BOOL func_ov167_021d42ac(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

void func_ov167_021d4304(BtlvScu *scu) {
    BtlvScu_SetSubProc(scu, func_ov167_021d44d8);
}

BOOL func_ov167_021d4324(BtlvScu *scu) {
    return BtlvScu_RunSubProc(scu);
}

// Opens the stat window with the gains of the level-up
static BOOL func_ov167_021d437c(s32 *seq, void *arg) {
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
        func_ov167_021d57e4(scu->strbuf, scu->levelUp.hp, scu->levelUp.attack, scu->levelUp.defense,
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
static BOOL func_ov167_021d4448(s32 *seq, void *arg) {
    BtlvScu *scu = arg;

    switch (*seq) {
    case 0:
        func_ov167_021bb10c(scu->levelUpMon, &scu->levelUp);
        func_ov167_021d5874(scu->strbuf, scu->levelUp.hp, scu->levelUp.attack, scu->levelUp.defense,
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
static BOOL func_ov167_021d44d8(s32 *seq, void *arg) {
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
static u16 func_ov167_021d4578(BtlvScu *scu, u16 color) {
    if (func_ov167_0219c988(scu->mainModule) == 2) {
        color = 0;
    }
    return color;
}

static void func_ov167_021d4590(BtlvScu *scu) {
    GFL_BitmapFill(scu->msgBitmap, func_ov167_021d4578(scu, 0xC));
    BmpWin_FlushChar(scu->msgWin);
}
