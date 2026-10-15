#include "types.h"
#include "app/th_award.h"
#include "app/th_award_graphic.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "save/trial_house.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/script/global_10440.h"

// The Trial House's results screen: the rank that the points of a challenge earn, the points, the kind of battle and
// the team's Pokémon, with a jingle for the rank. A, B or a touch closes it. Named after the ROM's "th_award_graphic.c"
// (the other file of the overlay is th_award_graphic.c)

#define BG_FRAME 1
#define BG_BACKDROP 2
#define BG_TEXT 3
#define BG_SUB_C_GEAR 4

#define WINDOW_COUNT 4
#define WINDOW_BATTLE_KIND 1
#define WINDOW_POINTS 2
#define WINDOW_RANK 3

#define ICON_COUNT 4
#define RANK_COUNT 7
// The rank index of a challenge that has no result
#define RANK_NONE 7

// The steps of the process
enum {
    STEP_START,
    STEP_WAIT_FADE_IN,
    STEP_WAIT_JINGLE,
    STEP_WAIT_INPUT,
    STEP_WAIT_FADE_OUT,
    STEP_END,
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ThAwardBGSetup;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 fromEnd;
} ThAwardWindowSetup;

typedef struct {
    u32 msgId;
    u8 iconCount;
    u8 unk5;
    u16 rank;
    u32 bgm;
} ThAwardRankInfo;

typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
} ThAwardIconRes;

typedef struct {
    u16 heapId;
    ThAwardParam *param;
    ThAwardGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    TCB *vblankTask;
    u8 iconCount;
    ThAwardIconRes icons[ICON_COUNT];
    ClActor *iconActors[ICON_COUNT];
    MsgData *msgData;
    BmpWin *windows[WINDOW_COUNT];
    BOOL printed[WINDOW_COUNT];
    BOOL transferred;
    u8 rankIconCount;
    u32 rankChars;
    u32 rankPalette;
    u32 rankCellAnims;
    ClActor *rankActors[RANK_COUNT];
    u32 rankIdx;
} ThAwardWork;

static BOOL ThAward_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ThAward_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ThAward_Main(GameProc *proc, u32 *state, void *param, void *work);
static void ThAward_VBlank(TCB *tcb, void *data);
static void ThAward_LoadGraphics(ThAwardWork *wk);
static void ThAward_Unused(ThAwardWork *wk);
static void ThAward_InitIcons(ThAwardWork *wk);
static void ThAward_FreeIcons(ThAwardWork *wk);
static void ThAward_InitRankIcons(ThAwardWork *wk);
static void ThAward_FreeRankIcons(ThAwardWork *wk);
static void ThAward_InitText(ThAwardWork *wk);
static void ThAward_FreeText(ThAwardWork *wk);
static void ThAward_TransferPrinted(ThAwardWork *wk);
static u32 ThAward_GetRankIdx(u16 points);

static inline TrialHouseRecord *ThAward_GetRecord(ThAwardParam *param) {
    TrialHouseRecord *record;

    if (param->downloaded) {
        record = &param->save->records[1];
    } else {
        record = &param->save->records[0];
    }
    return record;
}

static const u8 sThAwardTextY[4] = { 0, 0, 0, 0 };

static const ClActorPos sThAwardIconPositionsSingle[4] = { { 0x58, 0x98 }, { 0x80, 0x98 }, { 0xa8, 0x98 } };

static const ClActorPos sThAwardIconPositionsDouble[4] = {
    { 0x50, 0x98 }, { 0x70, 0x98 }, { 0x90, 0x98 }, { 0xb0, 0x98 }
};

static const ClActorPos sThAwardRankIconPositions[RANK_COUNT] = {
    { 0x30, 0x72 }, { 0x45, 0x6b }, { 0x5c, 0x67 }, { 0x74, 0x65 }, { 0x8c, 0x67 }, { 0xa3, 0x6b }, { 0xb8, 0x72 },
};

static const ThAwardWindowSetup sThAwardWindowSetups[WINDOW_COUNT] = {
    { BG_TEXT, 0, 0, 1, 1, 2, 0 },
    { BG_TEXT, 1, 4, 30, 2, 2, 0 },
    { BG_TEXT, 4, 8, 24, 2, 2, 0 },
    { BG_TEXT, 4, 14, 24, 2, 2, 0 },
};

static const ThAwardBGSetup sThAwardSubBGSetups[1] = {
    { BG_SUB_C_GEAR,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x8000), GX_BG_CHARBASE(0x0000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

const GameProcFunctions TH_AWARD_PROC_FUNCTIONS = { ThAward_Init, ThAward_Main, ThAward_Exit };

// From the best rank down, the text, the number of icons, the points' rank and the jingle
static const ThAwardRankInfo sThAwardRankInfos[RANK_COUNT] = {
    { Global10440_Text_MasterRank, 7, 0, 6, SEQ_ME_HYOUKA6 },
    { Global10440_Text_EliteRank, 6, 0, 5, SEQ_ME_HYOUKA5 },
    { Global10440_Text_HyperRank, 5, 0, 4, SEQ_ME_HYOUKA4 },
    { Global10440_Text_SuperRank, 4, 0, 3, SEQ_ME_HYOUKA3 },
    { Global10440_Text_NormalRank, 3, 0, 2, SEQ_ME_HYOUKA2 },
    { Global10440_Text_NoviceRank, 2, 0, 1, SEQ_ME_HYOUKA1 },
    { Global10440_Text_BeginnerRank, 1, 0, 0, SEQ_ME_HYOUKA1 },
};

static BOOL ThAward_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ThAwardWork *wk;
    TrialHouseRecord *record;
    ArcTool *arc;
    HeapID heapId;
    u32 palette;
    int i;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_AWARD, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ThAwardWork), HEAPID_AWARD);
    sys_memset(wk, 0, sizeof(ThAwardWork));
    wk->heapId = HEAPID_AWARD;
    wk->param = param;
    wk->graphic = ThAwardGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    GFL_BGSysSetBGPriority(1, 2);
    GFL_BGSysSetBGPriority(2, 1);
    GFL_BGSysSetBGPriority(3, 0);
    wk->vblankTask = GFL_VBlankTCBAdd(ThAward_VBlank, wk, 1);
    record = ThAward_GetRecord(wk->param);
    if (record->valid == 0) {
        wk->rankIdx = RANK_NONE;
    } else {
        wk->rankIdx = ThAward_GetRankIdx(record->points);
    }
    ThAward_LoadGraphics(wk);
    ThAward_InitIcons(wk);
    ThAward_InitRankIcons(wk);
    ThAward_InitText(wk);
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_DispOn();
    GFL_BGSysDisableAllB();
    gfxSetBGModeB(0);
    GFL_BGSysDisableBGsB();
    for (i = 0; i < NELEMS(sThAwardSubBGSetups); i++) {
        GFL_BGSysCreateBG(sThAwardSubBGSetups[i].bg, &sThAwardSubBGSetups[i].setup, sThAwardSubBGSetups[i].mode);
        GFL_BGSysClearBG(sThAwardSubBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sThAwardSubBGSetups[i].bg, sThAwardSubBGSetups[i].enabled);
    }
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    heapId = wk->heapId;
    palette = TRUE;
    if (wk->param->gender != GENDER_FEMALE) {
        palette = FALSE;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, BG_SUB_C_GEAR, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, BG_SUB_C_GEAR, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_SUB_C_GEAR);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 16, 0);
    func_02042ba8(FALSE, wk->heapId);
    if (wk->param->playJingle && wk->rankIdx != RANK_NONE) {
        GFL_SndBGMFadeOut(6);
    }
    return TRUE;
}

static BOOL ThAward_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ThAwardWork *wk = work;
    int i;

    for (i = 0; i < NELEMS(sThAwardSubBGSetups); i++) {
        GFL_BGSysReleaseBG(sThAwardSubBGSetups[i].bg);
    }
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    ThAward_FreeText(wk);
    ThAward_FreeRankIcons(wk);
    ThAward_FreeIcons(wk);
    ThAward_Unused(wk);
    GFL_TCBRemove(wk->vblankTask);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    ThAwardGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_AWARD);
    return TRUE;
}

static BOOL ThAward_Main(GameProc *proc, u32 *state, void *param, void *work) {
    ThAwardWork *wk = work;
    BOOL done;
    u32 x;
    u32 y;
    u8 i;

    switch (*state) {
    case STEP_START:
        done = TRUE;
        if (wk->param->playJingle && wk->rankIdx != RANK_NONE) {
            if (GFL_SndBGMIsFading()) {
                done = FALSE;
            } else {
                GFL_SndBGMSetPaused(TRUE);
                GFL_SndBGMPush();
                GFL_SndBGMPlay(sThAwardRankInfos[wk->rankIdx].bgm, SND_CHANNEL_MASK_ALL);
            }
        }
        if (done) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
            *state = STEP_WAIT_FADE_IN;
        }
        break;
    case STEP_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            if (wk->param->playJingle && wk->rankIdx != RANK_NONE) {
                *state = STEP_WAIT_JINGLE;
            } else {
                *state = STEP_WAIT_INPUT;
            }
        }
        break;
    case STEP_WAIT_JINGLE:
        if (!GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            GFL_SndBGMFadeIn(6);
            *state = STEP_WAIT_INPUT;
        }
        break;
    case STEP_WAIT_INPUT:
        done = FALSE;
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            func_0203d564(FALSE);
            done = TRUE;
        } else if (func_0203dac8(&x, &y)) {
            done = TRUE;
            func_0203d564(TRUE);
        }
        if (done) {
            *state = STEP_WAIT_FADE_OUT;
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        }
        break;
    case STEP_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            *state = STEP_END;
        }
        break;
    case STEP_END:
        return TRUE;
    }
    if (func_02021a3c(wk->printQueue) && !wk->transferred) {
        for (i = 1; i < WINDOW_COUNT; i++) {
            BmpWin_Transfer(wk->windows[i]);
        }
        wk->transferred = TRUE;
    }
    ThAward_TransferPrinted(wk);
    ThAwardGraphic_Update(wk->graphic);
    return FALSE;
}

static void ThAward_VBlank(TCB *tcb, void *data) {
}

static void ThAward_LoadGraphics(ThAwardWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_TH_AWARD, wk->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0x40, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, BG_BACKDROP, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, BG_BACKDROP, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 5, BG_FRAME, 0, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_BACKDROP);
    GFL_BGSysLoadScr(BG_FRAME);
}

static void ThAward_Unused(ThAwardWork *wk) {
}

// The icons of the team's Pokémon
static void ThAward_InitIcons(ThAwardWork *wk) {
    TrialHouseRecord *record;
    const ClActorPos *positions;
    ArcTool *arc;
    ClActorSetup setup;
    u32 species;
    u32 form;
    u32 sex;
    u8 i;

    wk->iconCount = 0;
    record = ThAward_GetRecord(wk->param);
    if (record->valid == 0) {
        return;
    }
    if (record->isDouble == 0) {
        wk->iconCount = 3;
        positions = sThAwardIconPositionsSingle;
    } else {
        wk->iconCount = 4;
        positions = sThAwardIconPositionsDouble;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);
    for (i = 0; i < wk->iconCount; i++) {
        species = record->pokemon[i].species;
        form = record->pokemon[i].form;
        sex = record->pokemon[i].sex;
        if (i == 0) {
            wk->icons[i].palette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), 0, 0x20, wk->heapId);
        }
        wk->icons[i].chars =
            func_0204b81c(arc, PokeParty_GetIconIndex(species, form, sex, FALSE), FALSE, 0, wk->heapId);
        wk->icons[i].cellAnims = func_0204bde0(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), wk->heapId);
        sys_memset(&setup, 0, sizeof(setup));
        setup.x = positions[i].x;
        setup.y = positions[i].y;
        wk->iconActors[i] = func_0204c040(ThAwardGraphic_GetClActUnit(wk->graphic), wk->icons[i].chars,
                                          wk->icons[0].palette, wk->icons[i].cellAnims, &setup, 0, wk->heapId);
        func_0204c488(wk->iconActors[i], 1);
        func_0204c520(wk->iconActors[i], FALSE);
        func_0204c378(wk->iconActors[i], PokeIcon_GetPalette(species, form, sex, FALSE), 0);
    }
    GFL_ArcToolFree(arc);
}

static void ThAward_FreeIcons(ThAwardWork *wk) {
    u8 i;

    for (i = 0; i < wk->iconCount; i++) {
        func_0204c108(wk->iconActors[i]);
        func_0204be64(wk->icons[i].cellAnims);
        func_0204b98c(wk->icons[i].chars);
        if (i == 0) {
            func_0204bcd0(wk->icons[i].palette);
        }
    }
}

// The row of icons that stand for the rank
static void ThAward_InitRankIcons(ThAwardWork *wk) {
    ArcTool *arc;
    ClActorSetup setup;
    u8 i;

    wk->rankIconCount = 0;
    if (wk->rankIdx == RANK_NONE) {
        return;
    }
    wk->rankIconCount = sThAwardRankInfos[wk->rankIdx].iconCount;
    if (wk->rankIconCount == 0) {
        return;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_TH_AWARD, wk->heapId);
    wk->rankPalette = func_0204bbb8(arc, 1, 0, 0, 0, 1, wk->heapId);
    wk->rankChars = func_0204b81c(arc, 3, FALSE, 0, wk->heapId);
    wk->rankCellAnims = func_0204bde0(arc, 6, 7, wk->heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(setup));
    for (i = 0; i < wk->rankIconCount; i++) {
        setup.x = sThAwardRankIconPositions[i].x;
        setup.y = sThAwardRankIconPositions[i].y;
        wk->rankActors[i] = func_0204c040(ThAwardGraphic_GetClActUnit(wk->graphic), wk->rankChars, wk->rankPalette,
                                          wk->rankCellAnims, &setup, 0, wk->heapId);
        func_0204c488(wk->rankActors[i], 0);
        func_0204c520(wk->rankActors[i], TRUE);
    }
}

static void ThAward_FreeRankIcons(ThAwardWork *wk) {
    u8 i;

    if (wk->rankIconCount != 0) {
        for (i = 0; i < wk->rankIconCount; i++) {
            func_0204c108(wk->rankActors[i]);
        }
        func_0204be64(wk->rankCellAnims);
        func_0204b98c(wk->rankChars);
        func_0204bcd0(wk->rankPalette);
    }
}

static void ThAward_InitText(ThAwardWork *wk) {
    TrialHouseRecord *record;
    const u16 *name;
    StrBuf *kind;
    StrBuf *label;
    WordSet *wordSet;
    StrBuf *format;
    StrBuf *pointsLabel;
    StrBuf *pointsText;
    StrBuf *rank;
    u16 kindWidth;
    u16 pointsWidth;
    u16 labelWidth;
    u16 totalWidth;
    u16 windowWidth;
    u32 points;
    u16 x;
    u16 labelX;
    u8 i;

    wk->msgData = NULL;
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->windows[i] = NULL;
        wk->printed[i] = FALSE;
    }
    wk->printed[0] = TRUE;
    wk->transferred = FALSE;
    if (wk->param->downloaded) {
        record = &wk->param->save->records[1];
        name = (const u16 *)wk->param->save->unk38;
    } else {
        record = &wk->param->save->records[0];
    }
    if (record->valid == 0) {
        return;
    }
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0, 0x40, 0x20, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10440, wk->heapId);
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->windows[i] =
            BmpWin_CreateDynamic(sThAwardWindowSetups[i].bg, sThAwardWindowSetups[i].x, sThAwardWindowSetups[i].y,
                                 sThAwardWindowSetups[i].width, sThAwardWindowSetups[i].height,
                                 sThAwardWindowSetups[i].palette, sThAwardWindowSetups[i].fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
        BmpWin_FlushChar(wk->windows[i]);
    }

    // The kind of battle, or the name of the downloaded challenge, and "Test Result", centered together
    if (wk->param->downloaded) {
        kind = GFL_StrBufCreate(17, wk->heapId);
        GFL_StrBufLoadFixedString(kind, name, 17);
    } else {
        u32 messageId = Global10440_Text_SingleBattle;

        if (record->isDouble != 0) {
            messageId = Global10440_Text_DoubleBattle;
        }
        kind = GFL_MsgDataLoadStrbufNew(wk->msgData, messageId);
    }
    label = GFL_MsgDataLoadStrbufNew(wk->msgData, Global10440_Text_TestResult);
    x = 0;
    kindWidth = GFL_FontGetBlockWidth(kind, wk->font, 0);
    labelWidth = GFL_FontGetBlockWidth(label, wk->font, 0);
    totalWidth = kindWidth + 8 + labelWidth;
    windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_BATTLE_KIND]));
    if (windowWidth >= totalWidth) {
        x = (windowWidth - totalWidth) / 2;
    }
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_BATTLE_KIND]), x,
                  sThAwardTextY[WINDOW_BATTLE_KIND], kind, wk->font, PRINT_COLOR(1, 2, 0));
    labelX = x + kindWidth + 8;
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_BATTLE_KIND]), labelX,
                  sThAwardTextY[WINDOW_BATTLE_KIND], label, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(label);
    GFL_StrBufFree(kind);

    // The points
    points = record->points;
    if (points > 9999) {
        points = 9999;
    }
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, Global10440_Text_Empty);
    pointsText = GFL_StrBufCreate(5, wk->heapId);
    x = 0;
    WordSetNumber(wordSet, 0, (u16)points, 4, 0, 1);
    GFL_WordSetFormatStrbuf(wordSet, pointsText, format);
    pointsLabel = GFL_MsgDataLoadStrbufNew(wk->msgData, Global10440_Text_Points);
    pointsWidth = GFL_FontGetBlockWidth(pointsText, wk->font, 0);
    labelWidth = GFL_FontGetBlockWidth(pointsLabel, wk->font, 0);
    totalWidth = pointsWidth + 8 + labelWidth;
    windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_POINTS]));
    if (windowWidth >= totalWidth) {
        x = (windowWidth - totalWidth) / 2;
    }
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_POINTS]), x, sThAwardTextY[WINDOW_POINTS],
                  pointsText, wk->font, PRINT_COLOR(1, 2, 0));
    labelX = x + pointsWidth + 8;
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_POINTS]), labelX, sThAwardTextY[WINDOW_POINTS],
                  pointsLabel, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(pointsLabel);
    GFL_StrBufFree(pointsText);
    GFL_StrBufFree(format);
    GFL_WordSetSystemFree(wordSet);

    // The rank, centered
    rank = GFL_MsgDataLoadStrbufNew(wk->msgData, sThAwardRankInfos[ThAward_GetRankIdx(record->points)].msgId);
    totalWidth = GFL_FontGetBlockWidth(rank, wk->font, 0);
    windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_RANK]));
    x = (windowWidth - totalWidth) / 2;
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_RANK]), x, sThAwardTextY[WINDOW_RANK], rank,
                  wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(rank);

    for (i = 1; i < WINDOW_COUNT; i++) {
        BmpWin_Transfer(wk->windows[i]);
    }
    ThAward_TransferPrinted(wk);
}

static void ThAward_FreeText(ThAwardWork *wk) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (wk->windows[i] != NULL) {
            BmpWin_Free(wk->windows[i]);
        }
    }
    if (wk->msgData != NULL) {
        GFL_MsgDataFree(wk->msgData);
    }
}

// Sends the windows that have finished printing to the screen
static void ThAward_TransferPrinted(ThAwardWork *wk) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (!wk->printed[i] && !func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->windows[i]))) {
            BmpWin_Transfer(wk->windows[i]);
            wk->printed[i] = TRUE;
        }
    }
}

// The rank of a challenge's points, as an index of sThAwardRankInfos
static u32 ThAward_GetRankIdx(u16 points) {
    u32 rank;
    u8 i;

    if (points >= 6000) {
        rank = 6;
    } else if (points >= 5000) {
        rank = 5;
    } else if (points >= 4000) {
        rank = 4;
    } else if (points >= 3000) {
        rank = 3;
    } else if (points >= 2000) {
        rank = 2;
    } else if (points >= 1000) {
        rank = 1;
    } else {
        rank = 0;
    }
    for (i = 0; i < RANK_COUNT; i++) {
        if (rank == sThAwardRankInfos[i].rank) {
            return i;
        }
    }
    return 0;
}
