#include "types.h"
#include "app/compatible_irc_sys.h"
#include "app/irc_appbar.h"
#include "app/irc_compat_result.h"
#include "app/irc_compatible.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "save/bag.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_log.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The compatibility result of the infrared link: its graphics, its windows, the three digits of the score, the flash of
// the background and the steps of the screen. The ROM has no string for the file, so the name is a guess. Names are
// ours

#define BG_COUNT 8
#define DIGIT_COUNT 3
#define CLACT_COUNT 38
#define FLASH_COLOR_COUNT 16
#define SWEET_HEART_MAX 999

// The actor of the sprite behind the digits that is not one of the three
#define ACTOR_BACKGROUND (CLACT_COUNT - 1)

// The medal for a score of 80 and over
#define MEDAL_COMPAT 180

// The record that counts the checks
#define RECORD_COMPAT_CHECKS 0x71

// The steps of IrcCompatResult_ShowStep
enum {
    STEP_WAIT_START,
    STEP_START_FLASH,
    STEP_WAIT_FLASH,
    STEP_WAIT_DIGITS,
    STEP_PRINT_SCORE,
    STEP_JINGLE,
    STEP_WAIT_JINGLE,
    STEP_REWARD,
    STEP_WAIT_BUTTON_DELAY,
    STEP_SHOW_BUTTON,
    STEP_WAIT_SAVE,
};

// What IrcCompatResult_GiveReward did
enum {
    REWARD_GIVEN,
    // Not given: the player was logged today, or there is no link
    REWARD_NONE,
    // The bag is full
    REWARD_NO_ROOM,
};

typedef struct {
    ClActUnit *unit;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    ClActor *actors[CLACT_COUNT];
} IrcCompatClAct;

typedef struct {
    // The characters of the window frame
    u32 frameChars;
    IrcCompatClAct clact;
    TCB *vblankTask;
} IrcCompatGraphic;

typedef struct {
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    WordSet *wordSet;
} IrcCompatText;

typedef struct {
    BmpWin *window;
    PrintWindow printWindow;
    StrBuf *strbuf;
    u16 color;
} IrcCompatWindow;

// The score in three digit sprites, which appear one after the other
typedef struct {
    ClActor *actors[DIGIT_COUNT];
    u16 unkC[DIGIT_COUNT];
    u16 unk12[DIGIT_COUNT];
    int unk18[DIGIT_COUNT];
    u8 unk24[12];
    u8 digits[DIGIT_COUNT];
    u8 unk33;
    // Whether each digit has appeared
    int shown[DIGIT_COUNT];
} IrcCompatDigits;

// The flash of colors of palette 8 of BG 3, a tier at a time, and how it fades back
typedef struct {
    u16 unk0;
    u8 unk2[6];
    u16 target[FLASH_COLOR_COUNT];
    u16 colors[FLASH_COLOR_COUNT];
    u32 frame;
    BOOL done;
    BOOL active;
    u16 lastStep;
    u16 step;
    int phase;
    // Whether the score is close enough over its tier to flash one more
    BOOL carry;
} IrcCompatFlash;

// The colors that flash at each step: the step's colors and how many
typedef struct {
    u8 colors[2];
    u8 count;
} IrcCompatFlashStep;

typedef struct IrcCompatResultWork IrcCompatResultWork;
typedef void (*IrcCompatStepFunc)(IrcCompatResultWork *wk, u16 *state);

struct IrcCompatResultWork {
    IrcCompatGraphic graphic;
    IrcCompatText text;
    // The message of the players, then of the result
    IrcCompatWindow windowA;
    // The title
    IrcCompatWindow windowB;
    IrcCompatDigits digits;
    IrcCompatFlash flash;
    IrcAppBar *button;
    IrcCompatStepFunc step;
    u16 stepState;
    BOOL stepDone;
    u32 counter;
    // Whether both players have logs
    BOOL bothLogs;
    // How many Sweet Hearts the score earns
    int reward;
    IrcCompatibleParam3 *param;
};

static BOOL IrcCompatResult_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcCompatResult_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcCompatResult_Main(GameProc *proc, u32 *state, void *param, void *work);
static void IrcCompatGraphic_Create(IrcCompatGraphic *graphic, HeapID heapId);
static void IrcCompatGraphic_Free(IrcCompatGraphic *graphic);
static void IrcCompatGraphic_Update(IrcCompatGraphic *graphic);
static ClActor *IrcCompatGraphic_GetActor(IrcCompatGraphic *graphic, int index);
static ClActUnit *IrcCompatGraphic_GetUnit(IrcCompatGraphic *graphic);
static void IrcCompatGraphic_VBlank(TCB *tcb, void *data);
static void IrcCompatGraphic_InitBG(IrcCompatGraphic *graphic, HeapID heapId);
static void IrcCompatGraphic_FreeBG(IrcCompatGraphic *graphic);
static void IrcCompatGraphic_VBlankBG(IrcCompatGraphic *graphic);
static void IrcCompatClAct_Init(IrcCompatClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void IrcCompatClAct_Free(IrcCompatClAct *clact);
static void IrcCompatClAct_Update(IrcCompatClAct *clact);
static void IrcCompatClAct_VBlank(IrcCompatClAct *clact);
static ClActor *IrcCompatClAct_GetActor(IrcCompatClAct *clact, int index);
static ClActUnit *IrcCompatClAct_GetUnit(IrcCompatClAct *clact);
static void IrcCompatText_Init(IrcCompatText *text, BOOL keepFont, HeapID heapId);
static void IrcCompatText_Free(IrcCompatText *text);
static BOOL IrcCompatText_Update(IrcCompatText *text);
static Font *IrcCompatText_GetFont(IrcCompatText *text);
static PrintQueue *IrcCompatText_GetQueue(IrcCompatText *text);
static MsgData *IrcCompatText_GetMsgData(IrcCompatText *text);
static WordSet *IrcCompatText_GetWordSet(IrcCompatText *text);
static void IrcCompatWindow_Init(IrcCompatWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                 HeapID heapId);
static void IrcCompatWindow_InitWithColor(IrcCompatWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                          u16 color, u8 fromEnd, HeapID heapId);
static void IrcCompatWindow_Free(IrcCompatWindow *win);
static BOOL IrcCompatWindow_Flush(IrcCompatWindow *win, IrcCompatText *text);
static void IrcCompatWindow_PrintMsg(IrcCompatWindow *win, IrcCompatText *text, u32 msgId, int x, s16 y);
static void IrcCompatWindow_PrintMsgCentered(IrcCompatWindow *win, IrcCompatText *text, u32 msgId);
static void IrcCompatWindow_PrintMsgNumber(IrcCompatWindow *win, IrcCompatText *text, u8 msgId, u8 number, u16 index);
static void IrcCompatWindow_PrintMsgPlayers(IrcCompatWindow *win, IrcCompatText *text, u32 msgId,
                                            const CompatibleIrcProfile *own, const CompatibleIrcProfile *other,
                                            HeapID heapId);
static void IrcCompatWindow_PrintMsgReward(IrcCompatWindow *win, IrcCompatText *text, const CompatibleIrcProfile *own,
                                           u32 count, u32 msgId);
static void IrcCompatResult_SetStep(IrcCompatResultWork *wk, IrcCompatStepFunc step);
static void IrcCompatResult_SetStepDone(IrcCompatResultWork *wk);
static void IrcCompatResult_StartStep(IrcCompatResultWork *wk, u16 *state);
static void IrcCompatResult_ShowStep(IrcCompatResultWork *wk, u16 *state);
static void IrcCompatResult_FinishStep(IrcCompatResultWork *wk, u16 *state);
static void IrcCompatDigits_Init(IrcCompatDigits *digits, IrcCompatGraphic *graphic, int score);
static void IrcCompatDigits_Show(IrcCompatDigits *digits, int index);
static BOOL IrcCompatDigits_Update(IrcCompatDigits *digits);
static int IrcCompatDigits_IsShown(IrcCompatDigits *digits, int index);
static void IrcCompatDigits_SetValue(IrcCompatDigits *digits, u32 value);
static void IrcCompatFlash_Init(IrcCompatFlash *flash, u16 unk0, u8 score, HeapID heapId);
static void IrcCompatFlash_Free(IrcCompatFlash *flash);
static void IrcCompatFlash_Update(IrcCompatFlash *flash);
static void IrcCompatFlash_Start(IrcCompatFlash *flash);
static BOOL IrcCompatFlash_IsDone(IrcCompatFlash *flash);
static void IrcCompatFlash_BlendColor(u16 *dest, u16 frame, u16 duration, u8 palette, u8 index, u16 from, u16 to);
static u32 IrcCompat_GetTier(u8 score, BOOL *carry);
static u32 IrcCompat_GetRewardCount(u32 score, BOOL bothLogs);
static u32 IrcCompatResult_GiveReward(IrcCompatResultWork *wk);

const GameProcFunctions IRC_COMPAT_RESULT_PROC_FUNCTIONS = {
    IrcCompatResult_Init,
    IrcCompatResult_Main,
    IrcCompatResult_Exit,
};

static const BGSysLCDConfig sIrcCompatLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_1, GX_BG0_AS_2D };

// The divisors that cut a score into digits
static const u32 sIrcCompatDigitDivisors[4] = { 1, 10, 100, 1000 };

static const u32 sIrcCompatBGModes[BG_COUNT] = { BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT,
                                                 BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT, BGMODE_AFFINE };

static const u32 sIrcCompatBGs[BG_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static const IrcCompatFlashStep sIrcCompatFlashSteps[6] = {
    { { 0, 0 }, 0 }, { { 12, 11 }, 2 }, { { 9, 10 }, 2 }, { { 8, 0 }, 1 }, { { 7, 6 }, 2 }, { { 3, 5 }, 2 },
};

static const BGSysVRAMConfig sIrcCompatVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const BGSetup sIrcCompatBGSetups[BG_COUNT] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x14000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x14000), 0x4000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
};

static BOOL IrcCompatResult_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatResultWork *wk;
    CompatibleIrcProfile own;
    CompatibleIrcProfile profile;
    IrcCompatibleParam3 *resultParam = param;
    ClActUnit *unit;
    Font *font;
    PrintQueue *queue;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_IRC_COMPAT, 0x20000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcCompatResultWork), HEAPID_IRC_COMPAT);
    sys_memset(wk, 0, sizeof(IrcCompatResultWork));
    wk->param = resultParam;
    IrcCompatGraphic_Create(&wk->graphic, HEAPID_IRC_COMPAT);
    IrcCompatText_Init(&wk->text, FALSE, HEAPID_IRC_COMPAT);
    if (wk->param->gsys != NULL) {
        GSYS_GetGameCommSystem(wk->param->gsys);
    }
    IrcCompatWindow_Init(&wk->windowA, 5, 1, 18, 30, 5, 5, HEAPID_IRC_COMPAT);
    BmpWin_DrawFrame(wk->windowA.window, 0, (u16)wk->graphic.frameChars, 6);
    IrcCompatWindow_InitWithColor(&wk->windowB, 7, 6, 3, 20, 5, 8, 1, TRUE, HEAPID_IRC_COMPAT);
    IrcCompatWindow_PrintMsgCentered(&wk->windowB, &wk->text, 1);
    GFL_BGSysScaleBGReq(7, 3, 0x1000);
    GFL_BGSysScaleBGReq(7, 6, 0x1000);
    GFL_BGSysAdjustBGOriginReq(7, 9, 0x80);
    GFL_BGSysAdjustBGOriginReq(7, 12, 0x28);
    IrcCompatDigits_Init(&wk->digits, &wk->graphic, wk->param->result);
    if (wk->param->gsys != NULL) {
        CompatibleIrcSys_BuildProfile(wk->param->gsys, &own);
        IrcCompatWindow_PrintMsgPlayers(&wk->windowA, &wk->text, 0, &own, wk->param->profile, HEAPID_IRC_COMPAT);
    }
    unit = IrcCompatGraphic_GetUnit(&wk->graphic);
    font = IrcCompatText_GetFont(&wk->text);
    queue = IrcCompatText_GetQueue(&wk->text);
    wk->button = IrcAppBar_Create(2, unit, 0, 13, 13, 2, font, queue, HEAPID_IRC_COMPAT);
    IrcAppBar_SetVisible(wk->button, FALSE);
    IrcCompatFlash_Init(&wk->flash, 3, wk->param->result, HEAPID_IRC_COMPAT);
    if (wk->param->gsys != NULL) {
        CompatibleIrcSys_BuildProfile(wk->param->gsys, &profile);
        if (profile.isFourteenth != 0 && wk->param->profile->isFourteenth != 0) {
            wk->bothLogs = TRUE;
        } else {
            wk->bothLogs = FALSE;
        }
        wk->reward = IrcCompat_GetRewardCount(wk->param->result, wk->bothLogs);
    }
    IrcCompatResult_SetStep(wk, IrcCompatResult_StartStep);
    return TRUE;
}

static BOOL IrcCompatResult_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatResultWork *wk = work;

    IrcCompatFlash_Free(&wk->flash);
    IrcCompatWindow_Free(&wk->windowB);
    IrcAppBar_Free(wk->button);
    IrcCompatWindow_Free(&wk->windowA);
    IrcCompatGraphic_Free(&wk->graphic);
    IrcCompatText_Free(&wk->text);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_IRC_COMPAT);
    return TRUE;
}

static BOOL IrcCompatResult_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatResultWork *wk = work;

    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        GFL_FadeSet(3, 16, 0, 0);
        *state = 2;
        break;
    case 2:
        if (GFL_FadeIsRunning()) {
            break;
        }
        *state = 3;
        break;
    case 3:
        wk->step(wk, &wk->stepState);
        if (wk->stepDone) {
            *state = 4;
        }
        break;
    case 4:
        GFL_FadeSet(3, 0, 16, 0);
        *state = 5;
        break;
    case 5:
        if (GFL_FadeIsRunning()) {
            break;
        }
        *state = 6;
        break;
    case 6:
        return TRUE;
    }
    if (IrcCompatText_Update(&wk->text)) {
        IrcCompatWindow_Flush(&wk->windowA, &wk->text);
        IrcCompatWindow_Flush(&wk->windowB, &wk->text);
    }
    IrcCompatGraphic_Update(&wk->graphic);
    IrcCompatFlash_Update(&wk->flash);
    IrcAppBar_Update(wk->button);
    if (wk->param->sys != NULL) {
        CompatibleIrcSys_Update(wk->param->sys);
    }
    return FALSE;
}

static void IrcCompatGraphic_Create(IrcCompatGraphic *graphic, HeapID heapId) {
    sys_memset(graphic, 0, sizeof(IrcCompatGraphic));
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sIrcCompatVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    IrcCompatClAct_Init(&graphic->clact, &sIrcCompatVRAMConfig, heapId);
    IrcCompatGraphic_InitBG(graphic, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(IrcCompatGraphic_VBlank, graphic, 0);
}

static void IrcCompatGraphic_Free(IrcCompatGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    IrcCompatGraphic_FreeBG(graphic);
    IrcCompatClAct_Free(&graphic->clact);
}

static void IrcCompatGraphic_Update(IrcCompatGraphic *graphic) {
    IrcCompatClAct_Update(&graphic->clact);
}

static ClActor *IrcCompatGraphic_GetActor(IrcCompatGraphic *graphic, int index) {
    return IrcCompatClAct_GetActor(&graphic->clact, index);
}

static ClActUnit *IrcCompatGraphic_GetUnit(IrcCompatGraphic *graphic) {
    return IrcCompatClAct_GetUnit(&graphic->clact);
}

static void IrcCompatGraphic_VBlank(TCB *tcb, void *data) {
    IrcCompatGraphic *graphic = data;

    IrcCompatGraphic_VBlankBG(graphic);
    IrcCompatClAct_VBlank(&graphic->clact);
}

static void IrcCompatGraphic_InitBG(IrcCompatGraphic *graphic, HeapID heapId) {
    ArcTool *arc;
    int i;

    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sIrcCompatLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysCreateBG(sIrcCompatBGs[i], &sIrcCompatBGSetups[i], sIrcCompatBGModes[i]);
        GFL_BGSysClearBG(sIrcCompatBGs[i]);
        GFL_BGSysSetBGEnabled(sIrcCompatBGs[i], TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 16, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 16, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 2, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 6, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 48, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 13, 1, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 2, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 46, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 14, 1, 0, 0, FALSE, heapId);
    GFL_BGSysFillChar(5, 0, 1, 0);
    graphic->frameChars = GFL_BGSysLoadArcNCGRDynamic(arc, 49, 5, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static void IrcCompatGraphic_FreeBG(IrcCompatGraphic *graphic) {
    int i;

    GFL_BGSysFreeCharMemory(5, (u16)graphic->frameChars, (u16)(graphic->frameChars >> 16));
    GFL_BGSysFreeFilledChar(5, 1, 0);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sIrcCompatBGs[i]);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void IrcCompatGraphic_VBlankBG(IrcCompatGraphic *graphic) {
    GFL_BGSysUpdate();
}

static void IrcCompatClAct_Init(IrcCompatClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    ArcTool *arc;
    ClActorSetup setup;
    ClActorSetup background;
    int i;

    sys_memset(clact, 0, sizeof(IrcCompatClAct));
    ClActSys_Create(&data_02093f08, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    clact->palette = func_0204bba0(arc, 12, CLACT_VRAM_MAIN, 0, heapId);
    clact->chars = func_0204b81c(arc, 11, FALSE, CLACT_VRAM_MAIN, heapId);
    clact->cellAnims = func_0204bde0(arc, 10, 9, heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    for (i = 0; i < 3; i++) {
        clact->actors[i] = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup,
                                         CLACT_SURFACE_MAIN, heapId);
    }
    sys_memset(&background, 0, sizeof(ClActorSetup));
    background.x = 128;
    background.y = 96;
    clact->actors[ACTOR_BACKGROUND] = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims,
                                                    &background, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(clact->actors[ACTOR_BACKGROUND], FALSE);
    func_0204c468(clact->actors[ACTOR_BACKGROUND], 0);
    func_0204c438(clact->actors[ACTOR_BACKGROUND], 0);
    func_0204c520(clact->actors[ACTOR_BACKGROUND], TRUE);
    func_0204c488(clact->actors[ACTOR_BACKGROUND], 11);
    for (i = 0; i < 3; i++) {
        func_0204c124(clact->actors[i], FALSE);
        func_0204c468(clact->actors[i], 0);
        func_0204c438(clact->actors[i], 0);
    }
}

static void IrcCompatClAct_Free(IrcCompatClAct *clact) {
    int i;

    for (i = 0; i < CLACT_COUNT; i++) {
        if (clact->actors[i] != NULL) {
            func_0204c108(clact->actors[i]);
        }
    }
    func_0204bcd0(clact->palette);
    func_0204b98c(clact->chars);
    func_0204be64(clact->cellAnims);
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(IrcCompatClAct));
}

static void IrcCompatClAct_Update(IrcCompatClAct *clact) {
    func_0204b794();
}

static void IrcCompatClAct_VBlank(IrcCompatClAct *clact) {
    func_0204b7c8();
}

static ClActor *IrcCompatClAct_GetActor(IrcCompatClAct *clact, int index) {
    return clact->actors[index];
}

static ClActUnit *IrcCompatClAct_GetUnit(IrcCompatClAct *clact) {
    return clact->unit;
}

static void IrcCompatText_Init(IrcCompatText *text, BOOL keepFont, HeapID heapId) {
    sys_memset(text, 0, sizeof(IrcCompatText));
    func_020232d0();
    if (!keepFont) {
        text->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    }
    text->queue = func_02021998(heapId);
    text->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_6, heapId);
    text->wordSet = GFL_WordSetSystemCreateDefault(heapId);
}

static void IrcCompatText_Free(IrcCompatText *text) {
    GFL_WordSetSystemFree(text->wordSet);
    GFL_MsgDataFree(text->msgData);
    func_02021a18(text->queue);
    GFL_FontFree(text->font);
    sys_memset(text, 0, sizeof(IrcCompatText));
}

static BOOL IrcCompatText_Update(IrcCompatText *text) {
    return func_02021a3c(text->queue);
}

static Font *IrcCompatText_GetFont(IrcCompatText *text) {
    return text->font;
}

static PrintQueue *IrcCompatText_GetQueue(IrcCompatText *text) {
    return text->queue;
}

static MsgData *IrcCompatText_GetMsgData(IrcCompatText *text) {
    return text->msgData;
}

static WordSet *IrcCompatText_GetWordSet(IrcCompatText *text) {
    return text->wordSet;
}

static void IrcCompatWindow_Init(IrcCompatWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                 HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcCompatWindow));
    win->color = 15;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_TransferNow(win->window);
}

static void IrcCompatWindow_InitWithColor(IrcCompatWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                          u16 color, u8 fromEnd, HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcCompatWindow));
    win->color = color;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, fromEnd);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_TransferNow(win->window);
}

static void IrcCompatWindow_Free(IrcCompatWindow *win) {
    GFL_StrBufFree(win->strbuf);
    BmpWin_Free(win->window);
    sys_memset(win, 0, sizeof(IrcCompatWindow));
}

static BOOL IrcCompatWindow_Flush(IrcCompatWindow *win, IrcCompatText *text) {
    PrintWindow_Flush(&win->printWindow, text->queue);
    return PrintWindow_IsPrinted(&win->printWindow);
}

static void IrcCompatWindow_PrintMsg(IrcCompatWindow *win, IrcCompatText *text, u32 msgId, int x, s16 y) {
    MsgData *msgData = IrcCompatText_GetMsgData(text);
    PrintQueue *queue = IrcCompatText_GetQueue(text);
    Font *font = IrcCompatText_GetFont(text);

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    PrintWindow_PrintNoColor(&win->printWindow, queue, x, y, win->strbuf, font);
}

static void IrcCompatWindow_PrintMsgCentered(IrcCompatWindow *win, IrcCompatText *text, u32 msgId) {
    MsgData *msgData = IrcCompatText_GetMsgData(text);
    PrintQueue *queue = IrcCompatText_GetQueue(text);
    Font *font = IrcCompatText_GetFont(text);
    u16 x;
    u16 y;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    x = BmpWin_GetSizeX(win->window) * 4;
    y = BmpWin_GetSizeY(win->window) * 4;
    x -= (u16)(GFL_FontGetBlockWidth(win->strbuf, font, 0) / 2);
    y -= (u16)(GFL_FontGetBlockHeight(win->strbuf, font) / 2);
    PrintWindow_Print(&win->printWindow, queue, x, y, win->strbuf, font, PRINT_COLOR(15, 14, 4));
}

static void IrcCompatWindow_PrintMsgNumber(IrcCompatWindow *win, IrcCompatText *text, u8 msgId, u8 number,
                                           u16 index) {
    WordSet *wordSet;
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    StrBuf *format;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    wordSet = IrcCompatText_GetWordSet(text);
    msgData = IrcCompatText_GetMsgData(text);
    WordSetNumber(wordSet, index, number, 3, 0, TRUE);
    format = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    GFL_WordSetFormatStrbuf(wordSet, win->strbuf, format);
    GFL_StrBufFree(format);
    queue = IrcCompatText_GetQueue(text);
    font = IrcCompatText_GetFont(text);
    PrintWindow_PrintNoColor(&win->printWindow, queue, 0, 0, win->strbuf, font);
}

static void IrcCompatWindow_PrintMsgPlayers(IrcCompatWindow *win, IrcCompatText *text, u32 msgId,
                                            const CompatibleIrcProfile *own, const CompatibleIrcProfile *other,
                                            HeapID heapId) {
    WordSet *wordSet;
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    StrBuf *format;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    wordSet = IrcCompatText_GetWordSet(text);
    msgData = IrcCompatText_GetMsgData(text);
    copyVarForText(wordSet, 0, (PlayerInfo *)&other->trainer);
    copyVarForText(wordSet, 1, (PlayerInfo *)&own->trainer);
    format = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    GFL_WordSetFormatStrbuf(wordSet, win->strbuf, format);
    GFL_StrBufFree(format);
    queue = IrcCompatText_GetQueue(text);
    font = IrcCompatText_GetFont(text);
    PrintWindow_PrintNoColor(&win->printWindow, queue, 0, 0, win->strbuf, font);
}

static void IrcCompatWindow_PrintMsgReward(IrcCompatWindow *win, IrcCompatText *text, const CompatibleIrcProfile *own,
                                           u32 count, u32 msgId) {
    WordSet *wordSet;
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    StrBuf *format;
    BOOL plural;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    wordSet = IrcCompatText_GetWordSet(text);
    msgData = IrcCompatText_GetMsgData(text);
    copyVarForText(wordSet, 0, (PlayerInfo *)&own->trainer);
    plural = FALSE;
    if (count > 1) {
        plural = TRUE;
    }
    loadItemText(wordSet, 1, ITEM_SWEET_HEART, plural, FALSE);
    WordSetNumber(wordSet, 2, count, 1, 0, TRUE);
    format = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    GFL_WordSetFormatStrbuf(wordSet, win->strbuf, format);
    GFL_StrBufFree(format);
    queue = IrcCompatText_GetQueue(text);
    font = IrcCompatText_GetFont(text);
    PrintWindow_PrintNoColor(&win->printWindow, queue, 0, 0, win->strbuf, font);
}

static void IrcCompatResult_SetStep(IrcCompatResultWork *wk, IrcCompatStepFunc step) {
    wk->step = step;
    wk->stepState = 0;
}

static void IrcCompatResult_SetStepDone(IrcCompatResultWork *wk) {
    wk->stepDone = TRUE;
}

static void IrcCompatResult_StartStep(IrcCompatResultWork *wk, u16 *state) {
    IrcCompatResult_SetStep(wk, IrcCompatResult_ShowStep);
}

static void IrcCompatResult_ShowStep(IrcCompatResultWork *wk, u16 *state) {
    CompatibleIrcProfile profile;
    u32 result;

    switch (*state) {
    case STEP_WAIT_START:
        if (wk->counter++ > 30) {
            wk->counter = 0;
            *state = STEP_START_FLASH;
        }
        break;
    case STEP_START_FLASH:
        IrcCompatFlash_Start(&wk->flash);
        *state = STEP_WAIT_FLASH;
        break;
    case STEP_WAIT_FLASH:
        if (!IrcCompatFlash_IsDone(&wk->flash)) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_SYS_20);
        IrcCompatDigits_Show(&wk->digits, 0);
        IrcCompatDigits_Show(&wk->digits, 1);
        IrcCompatDigits_Show(&wk->digits, 2);
        *state = STEP_WAIT_DIGITS;
        break;
    case STEP_WAIT_DIGITS:
        if (!IrcCompatDigits_IsShown(&wk->digits, 0)) {
            break;
        }
        if (!IrcCompatDigits_IsShown(&wk->digits, 1)) {
            break;
        }
        if (!IrcCompatDigits_IsShown(&wk->digits, 2)) {
            break;
        }
        *state = STEP_PRINT_SCORE;
        break;
    case STEP_PRINT_SCORE: {
        u8 score = wk->param->result;
        u16 msgId = (u8)(score / 10 + 2);

        if (msgId > 12) {
            msgId = 12;
        } else if (msgId < 2) {
            msgId = 2;
        }
        IrcCompatWindow_PrintMsgNumber(&wk->windowA, &wk->text, msgId, score, 2);
        *state = STEP_JINGLE;
        break;
    }
    case STEP_JINGLE:
        if (wk->counter++ > 50) {
            MedalBox *medalBox = SaveControl_GetMedalBox(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys)));

            wk->counter = 0;
            switch (IrcCompat_GetTier(wk->param->result, NULL)) {
            case 5:
                GFL_SndBGMPlay(SEQ_ME_AISHOU_H, SND_CHANNEL_MASK_ALL);
                MedalBox_GiveMedal(medalBox, MEDAL_COMPAT);
                break;
            case 4:
                MedalBox_GiveMedal(medalBox, MEDAL_COMPAT);
                GFL_SndBGMPlay(SEQ_ME_AISHOU_M, SND_CHANNEL_MASK_ALL);
                break;
            case 1:
            case 2:
            case 3:
                GFL_SndBGMPlay(SEQ_ME_AISHOU_L, SND_CHANNEL_MASK_ALL);
                break;
            case 0:
                GFL_SndBGMPlay(SEQ_ME_AISHOU_L, SND_CHANNEL_MASK_ALL);
                break;
            }
            *state = STEP_WAIT_JINGLE;
        }
        break;
    case STEP_WAIT_JINGLE:
        if (GFL_SndBGMIsPlaying()) {
            break;
        }
        if (wk->counter++ > 30) {
            wk->counter = 0;
            *state = STEP_REWARD;
        }
        break;
    case STEP_REWARD:
        if (!func_0203da48()) {
            break;
        }
        if (wk->param->gsys != NULL) {
            CompatibleIrcSys_BuildProfile(wk->param->gsys, &profile);
        }
        result = IrcCompatResult_GiveReward(wk);
        if (result == REWARD_GIVEN) {
            if (wk->bothLogs == FALSE) {
                IrcCompatWindow_PrintMsgReward(&wk->windowA, &wk->text, &profile, (u8)wk->reward, 13);
            } else {
                IrcCompatWindow_PrintMsgReward(&wk->windowA, &wk->text, &profile, (u8)wk->reward, 14);
            }
            *state = STEP_WAIT_BUTTON_DELAY;
        } else if (result == REWARD_NO_ROOM) {
            IrcCompatWindow_PrintMsg(&wk->windowA, &wk->text, 15, 0, 0);
            *state = STEP_WAIT_BUTTON_DELAY;
        } else {
            *state = STEP_SHOW_BUTTON;
        }
        break;
    case STEP_WAIT_BUTTON_DELAY:
        if (wk->counter++ > 30) {
            IrcAppBar_SetVisible(wk->button, TRUE);
            wk->counter = 0;
            *state = STEP_SHOW_BUTTON;
        }
        break;
    case STEP_SHOW_BUTTON:
        IrcAppBar_SetVisible(wk->button, TRUE);
        *state = STEP_WAIT_SAVE;
        break;
    case STEP_WAIT_SAVE:
        if (func_0203da48()) {
            IrcCompatResult_SetStep(wk, IrcCompatResult_FinishStep);
        }
        break;
    }
    if (IrcAppBar_GetResult(wk->button) == 1) {
        IrcCompatResult_SetStepDone(wk);
    }
    IrcCompatDigits_Update(&wk->digits);
}

static void IrcCompatResult_FinishStep(IrcCompatResultWork *wk, u16 *state) {
    IrcCompatResult_SetStepDone(wk);
}

static void IrcCompatDigits_Init(IrcCompatDigits *digits, IrcCompatGraphic *graphic, int score) {
    int i;

    sys_memset(digits, 0, sizeof(IrcCompatDigits));
    for (i = 0; i < DIGIT_COUNT; i++) {
        ClActorPos pos;

        digits->actors[i] = IrcCompatGraphic_GetActor(graphic, i);
        if (score < 100) {
            pos.x = 36 * i + 74;
        } else if (score >= 0 && score < 10) {
            pos.x = 36 * i + 56;
        } else {
            pos.x = 36 * i + 92;
        }
        pos.y = 84;
        func_0204c140(digits->actors[i], &pos, 0);
        func_0204c124(digits->actors[i], FALSE);
        digits->unkC[i] = 0;
        digits->unk12[i] = 60;
        digits->unk18[i] = 0xfef / digits->unk12[i];
    }
    IrcCompatDigits_SetValue(digits, score);
}

static void IrcCompatDigits_Show(IrcCompatDigits *digits, int index) {
    if (digits->shown[index] == 0) {
        if (index == 0 && digits->digits[2 - index] == 0) {
            digits->shown[index] = TRUE;
            return;
        }
        if (index == 0 && digits->digits[2 - index] == 0 && index == 1 && digits->digits[2 - index] == 0) {
            digits->shown[index] = TRUE;
            return;
        }
        digits->shown[index] = TRUE;
        func_0204c124(digits->actors[index], TRUE);
    }
}

static BOOL IrcCompatDigits_Update(IrcCompatDigits *digits) {
    return FALSE;
}

static int IrcCompatDigits_IsShown(IrcCompatDigits *digits, int index) {
    return digits->shown[index];
}

static void IrcCompatDigits_SetValue(IrcCompatDigits *digits, u32 value) {
    int i;
    u8 rest = value;

    // A count of the digits that nothing uses
    if (rest != 0) {
        for (;;) {
            if (rest == 0) {
                break;
            }
            rest = rest / 10;
        }
    }
    for (i = 0; i < DIGIT_COUNT; i++) {
        func_0204c488(digits->actors[i], 0);
    }
    for (i = 0; i < DIGIT_COUNT; i++) {
        digits->digits[i] = value % sIrcCompatDigitDivisors[i + 1] / sIrcCompatDigitDivisors[i];
        func_0204c488(digits->actors[2 - i], digits->digits[i] + 1);
    }
}

static void IrcCompatFlash_Init(IrcCompatFlash *flash, u16 unk0, u8 score, HeapID heapId) {
    ArcTool *arc;
    NNSG2dPaletteData *palette;
    void *file;
    u16 *src;
    int i;

    sys_memset(flash, 0, sizeof(IrcCompatFlash));
    flash->unk0 = unk0;
    flash->lastStep = IrcCompat_GetTier(score, &flash->carry);
    if (flash->carry) {
        flash->lastStep++;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 15, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 17, 3, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    file = GFL_G2DIOReadNCLR(ARCID_IRC_COMPAT, 16, &palette, heapId);
    src = palette->rawData;
    sys_memcpy(src, flash->target, sizeof(flash->target));
    sys_memcpy(src + 0x80, flash->target, sizeof(flash->target));
    GFL_HeapFree(file);
    sys_memcpy(flash->target, flash->colors, sizeof(flash->colors));
    for (i = 0; i < 13; i++) {
        flash->colors[i] = GX_RGB(31, 31, 31);
    }
    for (i = 0; i < FLASH_COLOR_COUNT; i++) {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 8 * 0x20 + i * 2, &flash->colors[i], 2);
    }
}

static void IrcCompatFlash_Free(IrcCompatFlash *flash) {
    sys_memset(flash, 0, sizeof(IrcCompatFlash));
}

static void IrcCompatFlash_Update(IrcCompatFlash *flash) {
    int k;

    if (!flash->active) {
        return;
    }
    switch (flash->phase) {
    case 0:
        if (flash->step > flash->lastStep) {
            if (flash->carry) {
                flash->phase = 1;
                return;
            }
            flash->done = TRUE;
            flash->active = FALSE;
            return;
        }
        if (flash->frame == 0) {
            GFL_SndSEPlay(SEQ_SE_FLD_165 + flash->step);
        }
        for (k = 0; k < sIrcCompatFlashSteps[flash->step].count; k++) {
            u8 color = sIrcCompatFlashSteps[flash->step].colors[k];

            IrcCompatFlash_BlendColor(&flash->colors[color], flash->frame, 40, 8, color, GX_RGB(31, 31, 31),
                                      flash->target[color]);
        }
        if (flash->frame++ >= 40) {
            flash->frame = 0;
            flash->step++;
        }
        break;
    case 1:
        if (flash->frame++ >= 60) {
            flash->frame = 0;
            flash->phase = 2;
        }
        break;
    case 2:
        if (flash->frame == 0) {
            GFL_SndSEPlay(SEQ_SE_FLD_164 + flash->step);
        }
        for (k = 0; k < sIrcCompatFlashSteps[flash->step - 1].count; k++) {
            u8 color = sIrcCompatFlashSteps[flash->step - 1].colors[k];

            IrcCompatFlash_BlendColor(&flash->colors[color], flash->frame, 40, 8, color, flash->target[color],
                                      GX_RGB(31, 31, 31));
        }
        if (flash->frame++ >= 40) {
            flash->frame = 0;
            flash->done = TRUE;
            flash->active = FALSE;
        }
        break;
    }
}

static void IrcCompatFlash_Start(IrcCompatFlash *flash) {
    flash->active = TRUE;
    flash->done = FALSE;
    flash->step = 1;
    flash->phase = 0;
    flash->frame = 0;
}

static BOOL IrcCompatFlash_IsDone(IrcCompatFlash *flash) {
    return flash->done;
}

static void IrcCompatFlash_BlendColor(u16 *dest, u16 frame, u16 duration, u8 palette, u8 index, u16 from, u16 to) {
    u8 fromR = from & GX_RGB_R_MASK;
    u8 fromG = (from & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 fromB = (from & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    u8 toR = to & GX_RGB_R_MASK;
    u8 toG = (to & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 toB = (to & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    int stepB = (toB - fromB) * frame / duration;
    int stepR = (toR - fromR) * frame / duration;
    int stepG = (toG - fromG) * frame / duration;

    *dest = GX_RGB((u8)(fromR + stepR), (u8)(fromG + stepG), (u8)(fromB + stepB));
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, palette * 0x20 + index * 2, dest, 2);
}

static u32 IrcCompat_GetTier(u8 score, BOOL *carry) {
    int next;
    u32 tier;

    if (score == 100) {
        next = 0xffff;
        tier = 5;
    } else if (score >= 80) {
        next = 100;
        tier = 4;
    } else if (score >= 60) {
        next = 80;
        tier = 3;
    } else if (score >= 30) {
        next = 60;
        tier = 2;
    } else if (score != 0) {
        next = 30;
        tier = 1;
    } else if (score == 0) {
        next = 0;
        tier = 0;
    } else {
        next = 100;
        tier = 1;
    }
    if (carry != NULL) {
        *carry = score + 3 > next ? TRUE : FALSE;
    }
    return tier;
}

static u32 IrcCompat_GetRewardCount(u32 score, BOOL bothLogs) {
    u32 count = bothLogs ? 2 : 1;

    if (score == 100) {
        return count * 3;
    }
    if (score > 79) {
        count *= 2;
    }
    return count;
}

static u32 IrcCompatResult_GiveReward(IrcCompatResultWork *wk) {
    u32 result = REWARD_NONE;
    TrainerLogSave *log = func_0200afbc(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys)));
    CompatibleIrcProfile *other = wk->param->profile;
    PlayerInfo *info = &other->trainer;

    if (!func_0200b0c0(log, getIDAsUInt(info)) && wk->param->gsys != NULL) {
        BagSave *bag = GameData_GetBag(GSYS_GetGameData(wk->param->gsys));
        u32 owned = BagSave_GetItemCountByID(bag, ITEM_SWEET_HEART, HEAPID_IRC_COMPAT);

        if (owned == SWEET_HEART_MAX) {
            result = REWARD_NO_ROOM;
        } else {
            u32 count = wk->reward;

            if (SWEET_HEART_MAX - owned < count) {
                count = SWEET_HEART_MAX - owned;
            }
            if (BagSave_AddItem(bag, ITEM_SWEET_HEART, count, HEAPID_IRC_COMPAT)) {
                result = REWARD_GIVEN;
            } else {
                result = REWARD_NO_ROOM;
            }
        }
    }
    if (wk->param->profile != NULL) {
        u16 *name = GetPlayerName(info);
        u8 unk10_17 = getTrainerGender(info) != 0 ? 0 : 1;

        func_0200b184(log, name, wk->param->result, unk10_17, other->birthMonth, other->birthDay, getIDAsUInt(info),
                      other->species, other->form, other->sex, other->isEgg);
    }
    if (wk->param->gsys != NULL) {
        RecordAddOne(GameData_GetRecords(GSYS_GetGameData(wk->param->gsys)), RECORD_COMPAT_CHECKS);
    }
    return result;
}
