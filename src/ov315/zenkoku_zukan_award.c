#include "types.h"
#include "app/zenkoku_zukan_award.h"
#include "app/zenkoku_zukan_award_graphic.h"
#include "app/zukan_award.h"
#include "constants/arc.h"
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
#include "save/config.h"
#include "save/player_info.h"
#include "save/trainer_data_savedata.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/script/title.h"

// The national Pokédex diploma: the certificate for completing the national Pokédex. It prints the player's name and
// the text on the top screen, slides a ribbon in and plays a jingle, and A, B or a touch closes it.
// Named after the ROM's "zenkoku_zukan_award.c" (the other file of the overlay is zenkoku_zukan_award_graphic.c)

#define BG_MAIN_BACKDROP 2
#define BG_MAIN_DIPLOMA 1
#define BG_SUB_C_GEAR 4

#define WINDOW_COUNT 5
#define WINDOW_PLAYER_NAME 2
#define WINDOW_CERTIFIES 3
#define WINDOW_STAFF 4

// The steps of the process
enum {
    STEP_WAIT_TEXT,
    STEP_WAIT_FADE_IN,
    STEP_WAIT_SLIDE,
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
} ZenkokuZukanAwardBGSetup;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 fromEnd;
    u8 unk7;
    u8 unk8;
} ZenkokuZukanAwardWindowSetup;

typedef struct {
    u8 x;
    u8 y;
    u8 unk2;
    u8 unk3;
} ZenkokuZukanAwardWindowPos;

typedef struct {
    u16 heapId;
    ZukanAwardParam *param;
    ZenkokuZukanAwardGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    TCB *vblankTask;
    MsgData *msgData;
    BmpWin *windows[WINDOW_COUNT];
    BOOL printed[WINDOW_COUNT];
    u8 slideWait;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    ClActor *ribbon;
} ZenkokuZukanAwardWork;

static BOOL ZenkokuZukanAward_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZenkokuZukanAward_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZenkokuZukanAward_Main(GameProc *proc, u32 *state, void *param, void *work);
static void ZenkokuZukanAward_VBlank(TCB *tcb, void *data);
static void ZenkokuZukanAward_LoadGraphics(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_Unused(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_InitText(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_FreeText(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_TransferPrinted(ZenkokuZukanAwardWork *wk);
static BOOL ZenkokuZukanAward_IsAllPrinted(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_InitBackdrop(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_InitRibbon(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_FreeRibbon(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_StartSlide(ZenkokuZukanAwardWork *wk);
static void ZenkokuZukanAward_UpdateSlide(ZenkokuZukanAwardWork *wk);
static BOOL ZenkokuZukanAward_IsSlideDone(ZenkokuZukanAwardWork *wk);

static const ClActorSetup sZenkokuZukanAwardRibbonSetup[1] = { { 18, 17, 0, 0, 1 } };

static const ZenkokuZukanAwardBGSetup sZenkokuZukanAwardSubBGSetups[1] = {
    { BG_SUB_C_GEAR,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x8000), GX_BG_CHARBASE(0x0000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static const ZenkokuZukanAwardWindowSetup sZenkokuZukanAwardWindowSetups[WINDOW_COUNT] = {
    { BG_MAIN_BACKDROP, 0, 0, 1, 1, 15, 0, 0, 0 },   { BG_MAIN_BACKDROP, 0, 24, 1, 1, 15, 0, 0, 0 },
    { BG_MAIN_BACKDROP, 7, 4, 20, 3, 15, 0, 0, 0 },  { BG_MAIN_BACKDROP, 7, 8, 20, 9, 15, 0, 0, 0 },
    { BG_MAIN_BACKDROP, 6, 17, 20, 5, 15, 0, 0, 0 },
};

// Where the windows go when the text is in kanji (the Japanese version's setting)
static const ZenkokuZukanAwardWindowPos sZenkokuZukanAwardKanjiWindowPositions[WINDOW_COUNT] = {
    { 0, 0, 0, 0 }, { 0, 24, 0, 0 }, { 7, 4, 0, 0 }, { 6, 8, 0, 0 }, { 6, 17, 0, 0 },
};

const GameProcFunctions ZENKOKU_ZUKAN_AWARD_PROC_FUNCTIONS = { ZenkokuZukanAward_Init, ZenkokuZukanAward_Main,
                                                               ZenkokuZukanAward_Exit };

static BOOL ZenkokuZukanAward_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ZenkokuZukanAwardWork *wk;
    PlayerInfo *playerInfo;
    ArcTool *arc;
    HeapID heapId;
    u32 palette;
    int i;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_AWARD, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZenkokuZukanAwardWork), HEAPID_AWARD);
    sys_memset(wk, 0, sizeof(ZenkokuZukanAwardWork));
    wk->heapId = HEAPID_AWARD;
    wk->param = param;
    wk->graphic = ZenkokuZukanAwardGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    GFL_BGSysSetBGPriority(1, 1);
    GFL_BGSysSetBGPriority(2, 0);
    wk->vblankTask = GFL_VBlankTCBAdd(ZenkokuZukanAward_VBlank, wk, 1);
    ZenkokuZukanAward_LoadGraphics(wk);
    ZenkokuZukanAward_InitText(wk);
    ZenkokuZukanAward_InitBackdrop(wk);
    ZenkokuZukanAward_InitRibbon(wk);
    playerInfo = GetGameDataPlayerInfo(wk->param->gameData);
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_DispOn();
    GFL_BGSysDisableAllB();
    gfxSetBGModeB(0);
    GFL_BGSysDisableBGsB();
    for (i = 0; i < NELEMS(sZenkokuZukanAwardSubBGSetups); i++) {
        GFL_BGSysCreateBG(sZenkokuZukanAwardSubBGSetups[i].bg, &sZenkokuZukanAwardSubBGSetups[i].setup,
                          sZenkokuZukanAwardSubBGSetups[i].mode);
        GFL_BGSysClearBG(sZenkokuZukanAwardSubBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sZenkokuZukanAwardSubBGSetups[i].bg, sZenkokuZukanAwardSubBGSetups[i].enabled);
    }
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    heapId = wk->heapId;
    palette = TRUE;
    if (playerInfo->gender != GENDER_FEMALE) {
        palette = FALSE;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, BG_SUB_C_GEAR, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, BG_SUB_C_GEAR, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_SUB_C_GEAR);
    ZenkokuZukanAward_StartSlide(wk);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 16, 0);
    func_02042ba8(FALSE, wk->heapId);
    if (wk->param->unk04 == 0) {
        GFL_SndBGMFadeOut(6);
    }
    return TRUE;
}

static BOOL ZenkokuZukanAward_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ZenkokuZukanAwardWork *wk = work;
    int i;

    for (i = 0; i < NELEMS(sZenkokuZukanAwardSubBGSetups); i++) {
        GFL_BGSysReleaseBG(sZenkokuZukanAwardSubBGSetups[i].bg);
    }
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    ZenkokuZukanAward_FreeRibbon(wk);
    ZenkokuZukanAward_FreeText(wk);
    ZenkokuZukanAward_Unused(wk);
    GFL_TCBRemove(wk->vblankTask);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    ZenkokuZukanAwardGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_AWARD);
    return TRUE;
}

static BOOL ZenkokuZukanAward_Main(GameProc *proc, u32 *state, void *param, void *work) {
    ZenkokuZukanAwardWork *wk = work;
    BOOL done;
    u32 x;
    u32 y;

    switch (*state) {
    case STEP_WAIT_TEXT:
        done = TRUE;
        if (!ZenkokuZukanAward_IsAllPrinted(wk)) {
            done = FALSE;
        }
        if (done && wk->param->unk04 == 0) {
            if (GFL_SndBGMIsFading()) {
                done = FALSE;
            } else {
                GFL_SndBGMSetPaused(TRUE);
                GFL_SndBGMPush();
                GFL_SndBGMPlay(SEQ_ME_HYOUKA6, SND_CHANNEL_MASK_ALL);
            }
        }
        if (done) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
            *state = STEP_WAIT_FADE_IN;
        }
        break;
    case STEP_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            if (wk->param->unk04 != 0) {
                *state = STEP_WAIT_INPUT;
            } else {
                *state = STEP_WAIT_SLIDE;
            }
        }
        break;
    case STEP_WAIT_SLIDE:
        if (ZenkokuZukanAward_IsSlideDone(wk)) {
            *state = STEP_WAIT_JINGLE;
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
    func_02021a3c(wk->printQueue);
    ZenkokuZukanAward_TransferPrinted(wk);
    if ((s32)*state >= STEP_WAIT_FADE_IN && (s32)*state <= STEP_WAIT_FADE_OUT) {
        ZenkokuZukanAward_UpdateSlide(wk);
    }
    ZenkokuZukanAwardGraphic_Update(wk->graphic);
    return FALSE;
}

static void ZenkokuZukanAward_VBlank(TCB *tcb, void *data) {
}

static void ZenkokuZukanAward_LoadGraphics(ZenkokuZukanAwardWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_AWARD, wk->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_MAIN_BG, 0, 0x1e0, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, BG_MAIN_DIPLOMA, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, BG_MAIN_DIPLOMA, 0, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_MAIN_DIPLOMA);
}

static void ZenkokuZukanAward_Unused(ZenkokuZukanAwardWork *wk) {
}

static void ZenkokuZukanAward_InitText(ZenkokuZukanAwardWork *wk) {
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);
    u32 kanji = func_02008a84((Config *)getTrainerDataBlkAddress(GameData_GetSaveControl(wk->param->gameData)));
    StrBuf *strbuf;
    StrBuf *certifies;
    StrBuf *staff;
    WordSet *wordSet;
    StrBuf *format;
    u16 textWidth;
    u16 windowWidth;
    s16 x;
    u8 i;

    wk->msgData = NULL;
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->windows[i] = NULL;
        wk->printed[i] = FALSE;
    }
    wk->printed[0] = TRUE;
    wk->printed[1] = TRUE;
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0, 0x1e0, 0x20, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_TITLE, wk->heapId);
    for (i = 0; i < WINDOW_COUNT; i++) {
        const ZenkokuZukanAwardWindowSetup *setup = &sZenkokuZukanAwardWindowSetups[i];

        wk->windows[i] = BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height,
                                              setup->palette, setup->fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
        BmpWin_FlushChar(wk->windows[i]);
    }
    if (kanji == 1) {
        for (i = 0; i < WINDOW_COUNT; i++) {
            BmpWin_SetPosX(wk->windows[i], sZenkokuZukanAwardKanjiWindowPositions[i].x);
            BmpWin_SetPosY(wk->windows[i], sZenkokuZukanAwardKanjiWindowPositions[i].y);
        }
    }

    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScriptTitle_Text_Player_2);
    strbuf = GFL_StrBufCreate(32, wk->heapId);
    x = 0;
    copyVarForText(wordSet, 0, playerInfo);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, format);
    GFL_FontGetBlockWidth(strbuf, wk->font, 0);
    GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_PLAYER_NAME]));
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_PLAYER_NAME]), 0, 0, strbuf, wk->font,
                  PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(format);
    GFL_WordSetSystemFree(wordSet);

    certifies = GFL_MsgDataLoadStrbufNew(wk->msgData, ScriptTitle_Text_DocumentCertifiesHaveSuccessfully);
    textWidth = GFL_FontGetBlockWidth(certifies, wk->font, 0);
    windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_CERTIFIES]));
    if (windowWidth >= textWidth && kanji == 1) {
        x = (windowWidth - textWidth) / 2;
    }
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_CERTIFIES]), x, 0, certifies, wk->font,
                  PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(certifies);

    staff = GFL_MsgDataLoadStrbufNew(wk->msgData, ScriptTitle_Text_GameFreakAllStaff_2);
    GFL_FontGetBlockWidth(staff, wk->font, 0);
    GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_STAFF]));
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->windows[WINDOW_STAFF]), 0, 0, staff, wk->font,
                  PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(staff);

    for (i = 1; i < WINDOW_COUNT; i++) {
        BmpWin_Transfer(wk->windows[i]);
    }
    ZenkokuZukanAward_TransferPrinted(wk);
}

static void ZenkokuZukanAward_FreeText(ZenkokuZukanAwardWork *wk) {
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
static void ZenkokuZukanAward_TransferPrinted(ZenkokuZukanAwardWork *wk) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (!wk->printed[i] && !func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->windows[i]))) {
            BmpWin_Transfer(wk->windows[i]);
            wk->printed[i] = TRUE;
        }
    }
}

static BOOL ZenkokuZukanAward_IsAllPrinted(ZenkokuZukanAwardWork *wk) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (!wk->printed[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

// Fills the BG behind the diploma's text with one color
static void ZenkokuZukanAward_InitBackdrop(ZenkokuZukanAwardWork *wk) {
    u16 *palette = GFL_HeapAllocate(wk->heapId, 0x20, TRUE, "zenkoku_zukan_award.c", 987);
    u16 *screen;
    u16 entry;
    u8 x;
    u8 y;
    u16 i = 0;

    palette[0] = GX_RGB(0, 0, 0);
    palette[1] = GX_RGB(9, 9, 7);
    palette[2] = GX_RGB(16, 16, 12);
    palette[3] = GX_RGB(0, 0, 0);
    GFL_BGSysUploadStdPalette(BG_MAIN_BACKDROP, palette, 0x20, 0x1e0);
    GFL_HeapFree(palette);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[1]), 3);
    BmpWin_FlushChar(wk->windows[1]);
    screen = GFL_HeapAllocate(wk->heapId, 32 * 24 * sizeof(u16), TRUE, "zenkoku_zukan_award.c", 1004);
    entry = BmpWin_GetCharPos(wk->windows[1]);
    entry |= 15 << 12;
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 32; x++) {
            screen[i] = entry;
            i++;
        }
    }
    GFL_BGSysLoadScrAreaAll(BG_MAIN_BACKDROP, screen, 0, 24, 32, 24);
    GFL_BGSysLoadScr(BG_MAIN_BACKDROP);
    GFL_HeapFree(screen);
}

static void ZenkokuZukanAward_InitRibbon(ZenkokuZukanAwardWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_AWARD, wk->heapId);

    wk->palette = func_0204bbb8(arc, 0, 0, 0, 0, 2, wk->heapId);
    wk->chars = func_0204b81c(arc, 3, FALSE, 0, wk->heapId);
    wk->cellAnims = func_0204bde0(arc, 8, 9, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->ribbon = func_0204c040(ZenkokuZukanAwardGraphic_GetClActUnit(wk->graphic), wk->chars, wk->palette,
                               wk->cellAnims, sZenkokuZukanAwardRibbonSetup, 0, wk->heapId);
    func_0204c124(wk->ribbon, TRUE);
    func_0204c520(wk->ribbon, TRUE);
}

static void ZenkokuZukanAward_FreeRibbon(ZenkokuZukanAwardWork *wk) {
    func_0204c108(wk->ribbon);
    func_0204be64(wk->cellAnims);
    func_0204b98c(wk->chars);
    func_0204bcd0(wk->palette);
}

// Puts the diploma's BGs and the ribbon below the screen, to slide up
static void ZenkokuZukanAward_StartSlide(ZenkokuZukanAwardWork *wk) {
    ClActorPos pos;
    u32 i;

    if (wk->param->unk04 == 0) {
        GFL_BGSysMoveBG(BG_MAIN_DIPLOMA, BG_MOVE_SET_Y, 64);
        GFL_BGSysMoveBG(BG_MAIN_BACKDROP, BG_MOVE_SET_Y, 64);
        for (i = 0; i < NELEMS(sZenkokuZukanAwardRibbonSetup); i++) {
            pos.x = sZenkokuZukanAwardRibbonSetup[i].x;
            pos.y = sZenkokuZukanAwardRibbonSetup[i].y - 64;
            func_0204c140(wk->ribbon, &pos, 0);
        }
        wk->slideWait = 0;
    }
}

static void ZenkokuZukanAward_UpdateSlide(ZenkokuZukanAwardWork *wk) {
    ClActorPos pos;
    int y;

    if (wk->param->unk04 == 0) {
        y = GFL_BGSysGetBGOffsetY(BG_MAIN_DIPLOMA);
        if (y != 0) {
            if (wk->slideWait == 0) {
                if (y >= 2) {
                    y = 2;
                }
                GFL_BGSysMoveBGReq(BG_MAIN_DIPLOMA, BG_MOVE_UP, y);
                GFL_BGSysMoveBGReq(BG_MAIN_BACKDROP, BG_MOVE_UP, y);
                func_0204c178(wk->ribbon, &pos, 0);
                pos.y += y;
                func_0204c140(wk->ribbon, &pos, 0);
                wk->slideWait = 0;
            } else {
                wk->slideWait--;
            }
        }
    }
}

static BOOL ZenkokuZukanAward_IsSlideDone(ZenkokuZukanAwardWork *wk) {
    BOOL done;
    BOOL result;

    if (wk->param->unk04 != 0) {
        return TRUE;
    }
    done = TRUE;
    if (GFL_BGSysGetBGOffsetY(BG_MAIN_DIPLOMA) != 0) {
        done = FALSE;
    }
    result = FALSE;
    if (done) {
        result = TRUE;
    }
    return result;
}
