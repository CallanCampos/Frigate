#include "app/pms_select.h"
#include "types.h"
#include "app/ui/ui_scene.h"
#include "app/ui/touchbar.h"
#include "app/pms_input.h"
#include "app/pms_select_graphic.h"
#include "constants/arc.h"
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
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/brightness.h"
#include "system/gf_font.h"
#include "system/pms.h"
#include "system/pms_data.h"
#include "system/pms_draw.h"
#include "system/pms_word.h"
#include "system/pmsi_param.h"
#include "system/printsys.h"
#include "text/system/pms_input.h"

// The phrase select: a list of the saved phrases on plates, from which the trainer card picks its greeting, or opens
// the phrase input to edit one. Named descriptively, after its graphics' "pms_select_graphic.c". The names are ours

// The plates shown at once, and the phrases saved
#define PLATE_COUNT 4
#define SENTENCE_MAX 20

// No phrase under the cursor
#define POS_NONE 0xff

// The scroll bar's range of y
#define SCROLL_BAR_TOP 16
#define SCROLL_BAR_BOTTOM 152
#define SCROLL_BAR_RANGE 137

enum {
    SEQ_START,
    SEQ_TOUCHBAR_ON,
    SEQ_FADE_IN,
    SEQ_FADE_IN_WAIT,
    SEQ_SELECT,
    SEQ_FADE_OUT,
    SEQ_FADE_OUT_WAIT,
    SEQ_TOUCHBAR_OFF,
    SEQ_MENU_OPEN,
    SEQ_MENU,
    SEQ_MENU_CLOSE,
    SEQ_INPUT_FADE_OUT,
    SEQ_INPUT_FADE_OUT_WAIT,
    SEQ_INPUT_RELEASE,
    SEQ_INPUT_CALL,
    SEQ_INPUT_RETURN,
    SEQ_END,
};

// The BGs: the plates the phrases are on, whose colors fade back and forth under the cursor, and the title window
typedef struct {
    u32 chars;
    void *screenFile;
    NNSG2dScreenData *screen;
    u16 fadeAngle;
    u16 fadeColors[3];
    u16 colors[4];
    // Set once a phrase is chosen, when the plate blinks
    BOOL decided;
    u8 blinkCount;
    BmpWin *titleWindow;
    BOOL titleFlushPending;
} PMSSelectBG;

typedef struct {
    PMSSelectParam *param;
    HeapID heapId;
    ClActor *scrollBar;
    UIObjRes objRes;
    SaveControl *save;
    PMSWordSave *wordSave;
    PMSSelectBG bg;
    PMSSelectGraphic *graphic;
    TouchBar *touchBar;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    BmpWin *windows[PLATE_COUNT];
    BOOL flushPending[PLATE_COUNT];
    PMSDraw *pmsDraw;
    PMSIParam *inputParam;
    u8 prevPos;
    u8 pos;
    // The phrase on the first plate
    u8 top;
    u8 sentenceCount;
    // The plates of the list, with the empty one after the phrases when there is room
    u8 lineCount;
    u8 scrollTouch : 1;
    u32 subSeq;
    u32 nextSeq;
    BOOL listActive;
    BOOL touchMode;
    GameProcManager *procMgr;
} PMSSelectWork;

// A color of a palette that fades from start to end and back
typedef struct {
    u16 palette;
    u16 color;
    GXRgb start;
    GXRgb end;
} PMSSelectColorFade;

static BOOL PMSSelect_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PMSSelect_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PMSSelect_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PMSSelect_IsPrintEnd(PMSSelectWork *wk);
static void PMSSelect_SetSeq(u32 *state, PMSSelectWork *wk, u32 seq);
static void PMSSelect_SetupScreen(PMSSelectWork *wk);
static void PMSSelect_ReleaseScreen(PMSSelectWork *wk);
static void PMSSelect_BGLoad(PMSSelectBG *bg, HeapID heapId, Font *font, PrintQueue *queue, MsgData *msgData);
static void PMSSelect_BGUnload(PMSSelectBG *bg);
static void PMSSelect_BGDrawPlate(PMSSelectBG *bg, u8 line, u32 palette, BOOL active);
static void PMSSelect_FadeColors(u16 *angle, GXRgb *colors, const PMSSelectColorFade *fades, u8 count);
static void PMSSelect_BGUpdatePlateFade(PMSSelectBG *bg);
static void PMSSelect_BGResetPlateFade(PMSSelectBG *bg);
static void PMSSelect_BGDrawPlateEdge(PMSSelectBG *bg, u32 palette);
static TouchBar *PMSSelect_CreateTouchBar(PMSSelectWork *wk, ClActUnit *unit, HeapID heapId);
static void PMSSelect_DeleteTouchBar(PMSSelectWork *wk);
static void PMSSelect_UpdateTouchBar(TouchBar *touchBar);
static AppTaskMenu *PMSSelect_CreateMenu(AppTaskMenuRes *res, MsgData *msgData, HeapID heapId);
static void PMSSelect_DeleteMenu(AppTaskMenu *menu);
static void PMSSelect_UpdateMenu(AppTaskMenu *menu);
static void PMSSelect_SetupList(PMSSelectWork *wk);
static void PMSSelect_DeleteList(PMSSelectWork *wk);
static void PMSSelect_UpdateList(PMSSelectWork *wk);
static void PMSSelect_DrawList(PMSSelectWork *wk);
static void PMSSelect_RedrawList(PMSSelectWork *wk);
static void PMSSelect_DrawListEx(PMSSelectWork *wk, BOOL waitPrint);
static void PMSSelect_UpdateScrollBar(PMSSelectWork *wk, BOOL force);
static BOOL PMSSelect_CheckScrollBarTouch(PMSSelectWork *wk);
static void PMSSelect_GetScrollPos(u32 y, u8 lineCount, u16 *top, s16 *barY);
static BOOL PMSSelect_ScrollByTouch(PMSSelectWork *wk);
static BOOL PMSSelect_ListInput(PMSSelectWork *wk);
static void PMSSelect_FlushWindows(PMSSelectWork *wk);
static BOOL PMSSelect_FlushWindow(PrintQueue *queue, BmpWin *window, BOOL *flushPending);
static void PMSSelect_DrawNewPlate(PMSSelectWork *wk, u8 line, u8 index, BOOL active);
static void PMSSelect_DrawSentencePlate(PMSSelectWork *wk, u8 line, u8 index, BOOL active);
static int PMSSelect_GetTouchedPlate(void);
static BOOL PMSSelect_SeqTouchBarOn(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqSelect(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqTouchBarOff(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqMenuOpen(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqMenu(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqMenuClose(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqCallInput(u32 *seq, PMSSelectWork *wk);
static BOOL PMSSelect_SeqInputReturn(u32 *seq, PMSSelectWork *wk);

const GameProcFunctions PMS_SELECT_PROC_FUNCTIONS = {
    PMSSelect_Init,
    PMSSelect_Main,
    PMSSelect_Exit,
};

// The colors of the plate under the cursor
static const PMSSelectColorFade sPMSSelectPlateFades[] = {
    { 7, 5, 0x7e80, 0x7fef },
    { 7, 9, 0x5164, 0x7f22 },
    { 7, 10, 0x69ea, 0x7fa5 },
};

// The menu of a phrase: choose it, edit it, or back
static u8 sPMSSelectMenuMessages[] = { 13, 16, 15 };

static BOOL PMSSelect_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PMSSelectWork *wk;
    PMSSelectParam *selectParam = param;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PMS_SELECT, 0x1c000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PMSSelectWork), HEAPID_PMS_SELECT);
    sys_memset(wk, 0, sizeof(PMSSelectWork));
    wk->heapId = HEAPID_PMS_SELECT;
    wk->save = selectParam->save;
    wk->wordSave = getDexBlkAddress(selectParam->save);
    wk->param = selectParam;
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PMS_INPUT, wk->heapId);
    wk->queue = func_02021998(wk->heapId);
    wk->procMgr = CreateGameProcManager(wk->heapId);
    wk->prevPos = POS_NONE;
    wk->pos = POS_NONE;
    PMSSelect_SetupScreen(wk);
    return TRUE;
}

static BOOL PMSSelect_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PMSSelectWork *wk = work;

    PMSSelect_ReleaseScreen(wk);
    FreeGameProcManager(wk->procMgr);
    func_02021c44(wk->queue);
    func_02021a18(wk->queue);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_PMS_SELECT);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL PMSSelect_IsPrintEnd(PMSSelectWork *wk) {
    if (func_02021c0c(wk->queue) && PMSDraw_IsPrintEnd(wk->pmsDraw)) {
        return TRUE;
    }
    return FALSE;
}

static void PMSSelect_SetSeq(u32 *state, PMSSelectWork *wk, u32 seq) {
    *state = seq;
    wk->subSeq = 0;
}

static BOOL PMSSelect_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PMSSelectWork *wk = work;
    BOOL running = GFL_ProcMgrUpdate(wk->procMgr);

    if (running == TRUE) {
        return FALSE;
    }

    switch (*state) {
    case SEQ_START:
        if (PMSData_IsComplete(PMSWordSave_GetSentence(wk->wordSave, 0), wk->heapId) == FALSE) {
            wk->pos = 0;
            PMSSelect_SetSeq(state, wk, SEQ_INPUT_RELEASE);
        } else {
            PMSSelect_SetSeq(state, wk, SEQ_TOUCHBAR_ON);
        }
        break;
    case SEQ_TOUCHBAR_ON:
        if (PMSSelect_SeqTouchBarOn(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, SEQ_FADE_IN);
        }
        break;
    case SEQ_FADE_IN:
        GFL_FadeSet(3, 16, 0, 0);
        PMSSelect_SetSeq(state, wk, SEQ_FADE_IN_WAIT);
        break;
    case SEQ_FADE_IN_WAIT:
        if (!GFL_FadeIsRunning()) {
            PMSSelect_SetSeq(state, wk, SEQ_SELECT);
        }
        break;
    case SEQ_SELECT:
        if (PMSSelect_IsPrintEnd(wk) && PMSSelect_SeqSelect(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, wk->nextSeq);
        }
        break;
    case SEQ_FADE_OUT:
        GFL_FadeSet(3, 0, 16, 0);
        PMSSelect_SetSeq(state, wk, SEQ_FADE_OUT_WAIT);
        break;
    case SEQ_FADE_OUT_WAIT:
        if (!GFL_FadeIsRunning()) {
            PMSSelect_SetSeq(state, wk, SEQ_TOUCHBAR_OFF);
        }
        break;
    case SEQ_TOUCHBAR_OFF:
        if (PMSSelect_SeqTouchBarOff(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, SEQ_END);
        }
        break;
    case SEQ_MENU_OPEN:
        if (PMSSelect_SeqMenuOpen(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, SEQ_MENU);
        }
        break;
    case SEQ_MENU:
        if (PMSSelect_SeqMenu(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, SEQ_MENU_CLOSE);
        }
        break;
    case SEQ_MENU_CLOSE:
        if (PMSSelect_SeqMenuClose(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, wk->nextSeq);
        }
        break;
    case SEQ_INPUT_FADE_OUT:
        if (PMSSelect_IsPrintEnd(wk)) {
            GFL_FadeSet(3, 0, 16, 0);
            PMSSelect_SetSeq(state, wk, SEQ_INPUT_FADE_OUT_WAIT);
        }
        break;
    case SEQ_INPUT_FADE_OUT_WAIT:
        if (!GFL_FadeIsRunning()) {
            PMSSelect_SetSeq(state, wk, SEQ_INPUT_RELEASE);
        }
        break;
    case SEQ_INPUT_RELEASE:
        PMSSelect_ReleaseScreen(wk);
        GFL_OvlUnload(OVERLAY_APP_UI);
        PMSSelect_SetSeq(state, wk, SEQ_INPUT_CALL);
        return FALSE;
    case SEQ_INPUT_CALL:
        if (PMSSelect_SeqCallInput(&wk->subSeq, wk)) {
            PMSSelect_SetSeq(state, wk, SEQ_INPUT_RETURN);
        }
        return FALSE;
    case SEQ_INPUT_RETURN:
        if (running != TRUE && PMSSelect_SeqInputReturn(&wk->subSeq, wk)) {
            GFL_OvlLoad(OVERLAY_APP_UI);
            PMSSelect_SetupScreen(wk);
            PMSSelect_SetSeq(state, wk, SEQ_TOUCHBAR_ON);
        }
        return FALSE;
    case SEQ_END:
        return TRUE;
    }

    func_02021a3c(wk->queue);
    if (wk->bg.titleFlushPending && !func_02021c1c(wk->queue, BmpWin_GetBitmap(wk->bg.titleWindow))) {
        BmpWin_Transfer(wk->bg.titleWindow);
        wk->bg.titleFlushPending = FALSE;
    }
    PMSSelect_FlushWindows(wk);
    PMSSelect_UpdateList(wk);
    PMSSelectGraphic_Main(wk->graphic);
    return FALSE;
}

static void PMSSelect_SetupScreen(PMSSelectWork *wk) {
    ClActUnit *unit;
    UIObjResSetup setup;

    wk->scrollTouch = FALSE;
    wk->listActive = TRUE;
    wk->touchMode = func_0203d554() == TRUE;
    wk->prevPos = wk->pos;
    if (wk->pos == POS_NONE) {
        if (wk->touchMode) {
            wk->top = 0;
        } else {
            wk->pos = 0;
            wk->top = 0;
        }
    } else if (wk->touchMode) {
        wk->pos = POS_NONE;
    }

    wk->graphic = PMSSelectGraphic_Create(0, wk->heapId);
    unit = PMSSelectGraphic_GetClActUnit(wk->graphic);
    setup.arcId = ARCID_PMSI;
    setup.paletteFile = 7;
    setup.charFile = 13;
    setup.cellFile = 35;
    setup.animFile = 41;
    setup.vramType = CLACT_VRAM_MAIN;
    setup.flags = 0;
    setup.paletteOffset = 0;
    setup.paletteStart = 0;
    setup.paletteCount = 5;
    UIObjRes_Load(&wk->objRes, &setup, unit, wk->heapId);
    wk->scrollBar = UIObjRes_CreateActor(&wk->objRes, unit, 240, SCROLL_BAR_TOP, 16, wk->heapId);
    func_0204c468(wk->scrollBar, 1);
    func_0204c520(wk->scrollBar, TRUE);
    PMSSelect_BGLoad(&wk->bg, wk->heapId, wk->font, wk->queue, wk->msgData);
    PMSSelect_BGResetPlateFade(&wk->bg);
    wk->bg.blinkCount = 0;
    wk->bg.decided = FALSE;
    wk->touchBar = PMSSelect_CreateTouchBar(wk, PMSSelectGraphic_GetClActUnit(wk->graphic), wk->heapId);
    wk->menuRes = AppTaskMenuRes_Create(0, 12, wk->font, wk->queue, wk->heapId);
    PMSSelect_SetupList(wk);
    func_02042ba8(TRUE, wk->heapId);
}

static void PMSSelect_ReleaseScreen(PMSSelectWork *wk) {
    u8 i;

    func_02021c44(wk->queue);
    for (i = 0; i < PLATE_COUNT; i++) {
        wk->flushPending[i] = FALSE;
    }
    wk->bg.titleFlushPending = FALSE;
    UIObjRes_Free(&wk->objRes);
    PMSSelect_DeleteTouchBar(wk);
    AppTaskMenuRes_Free(wk->menuRes);
    PMSSelect_DeleteList(wk);
    PMSSelect_BGUnload(&wk->bg);
    PMSSelectGraphic_Delete(wk->graphic);
    func_0203d564(wk->touchMode ? TRUE : FALSE);
}

static void PMSSelect_BGLoad(PMSSelectBG *bg, HeapID heapId, Font *font, PrintQueue *queue, MsgData *msgData) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PMSI, heapId);
    StrBuf *strbuf;

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 9 * 0x20, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 8 * 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 9 * 0x20, 0x20, heapId);
    bg->colors[0] = 0x7fff;
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 9 * 0x20 + 14 * sizeof(GXRgb), &bg->colors[0],
                                       sizeof(GXRgb));
    bg->colors[1] = 0x7fff;
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 9 * 0x20 + 13 * sizeof(GXRgb), &bg->colors[1],
                                       sizeof(GXRgb));
    GFL_BGSysLoadArcNCGRStatic(arc, 10, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 21, 4, 0, 0, FALSE, heapId);
    bg->chars = GFL_BGSysLoadArcNCGRDynamic(arc, 10, 3, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 22, 3, 0, CHAR_POS(bg->chars), 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 11, 2, 0, 0, FALSE, heapId);
    bg->screenFile = GFL_G2DIOReadNSCRArc(arc, 23, FALSE, &bg->screen, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 4, 4, 0x80, 8 * 0x20, 0x20, heapId);
    bg->colors[2] = 0x294b;
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 8 * 0x20 + 14 * sizeof(GXRgb), &bg->colors[2],
                                       sizeof(GXRgb));
    bg->colors[3] = 0x5694;
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 8 * 0x20 + 15 * sizeof(GXRgb), &bg->colors[3],
                                       sizeof(GXRgb));
    GFL_BGSysLoadArcNCGRStatic(arc, 18, 5, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 24, 5, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(5, 0, 0, 32, 32, 8);
    bg->titleWindow = BmpWin_CreateDynamic(5, 3, 5, 26, 7, 8, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(bg->titleWindow), 7);
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, 21);
    func_02021c7c(queue, BmpWin_GetBitmap(bg->titleWindow), 2, 4, strbuf, font, PRINT_COLOR(14, 15, 7));
    bg->titleFlushPending = TRUE;
    GFL_StrBufFree(strbuf);
    GFL_ArcToolFree(arc);
}

static void PMSSelect_BGUnload(PMSSelectBG *bg) {
    BmpWin_Free(bg->titleWindow);
    GFL_BGSysFreeCharMemory(3, CHAR_POS(bg->chars), CHAR_SIZE(bg->chars));
    GFL_HeapFree(bg->screenFile);
}

static void PMSSelect_BGDrawPlate(PMSSelectBG *bg, u8 line, u32 palette, BOOL active) {
    int i;
    NNSG2dScreenData *screen = bg->screen;
    int width = screen->width / 8;
    int height = screen->height / 8;
    u16 *entries = (u16 *)screen->rawData;

    if (active) {
        palette = 5;
    }
    for (i = 0; i < width * height; i++) {
        entries[i] = (entries[i] & 0xfff) + (palette + 2) * 0x1000;
    }
    GFL_BGSysLoadScrArea(2, 0, line * 6 + 1, width, 6, bg->screen->rawData, 0, 0, width, height);
    GFL_BGSysQueueScrLoad(2);
}

static void PMSSelect_FadeColors(u16 *angle, GXRgb *colors, const PMSSelectColorFade *fades, u8 count) {
    u8 i;
    fx16 t;

    if (*angle + 0x400 >= 0x10000) {
        *angle = *angle - 0x10000 + 0x400;
    } else {
        *angle += 0x400;
    }
    t = (FX_CosIdx(*angle) + FX16_ONE) / 2;
    for (i = 0; i < count; i++) {
        u8 r1 = (fades[i].start & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
        u8 g1 = (fades[i].start & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 b1 = (fades[i].start & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 r2 = (fades[i].end & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
        u8 g2 = (fades[i].end & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 b2 = (fades[i].end & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 r;
        u8 g;
        u8 b;

        r = r1 + ((r2 - r1) * t >> FX32_SHIFT);
        g = g1 + ((g2 - g1) * t >> FX32_SHIFT);
        b = b1 + ((b2 - b1) * t >> FX32_SHIFT);
        colors[i] = GX_RGB(r, g, b);
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN,
                                           fades[i].palette * 0x20 + fades[i].color * sizeof(GXRgb), &colors[i],
                                           sizeof(GXRgb));
    }
}

static void PMSSelect_BGUpdatePlateFade(PMSSelectBG *bg) {
    if (bg->decided) {
        BOOL on = TRUE;

        if (bg->blinkCount < 16) {
            on = (bg->blinkCount / 4) % 2;
            bg->blinkCount++;
        }
        bg->fadeAngle = (on ? 0x3f : 0x1f) << 10;
    }
    PMSSelect_FadeColors(&bg->fadeAngle, bg->fadeColors, sPMSSelectPlateFades, NELEMS(sPMSSelectPlateFades));
}

static void PMSSelect_BGResetPlateFade(PMSSelectBG *bg) {
    bg->fadeAngle = 0;
}

static void PMSSelect_BGDrawPlateEdge(PMSSelectBG *bg, u32 palette) {
    int i;
    NNSG2dScreenData *screen = bg->screen;
    int width = screen->width / 8;
    int height = screen->height / 8;
    u16 *entries = (u16 *)screen->rawData;

    for (i = 0; i < width * height; i++) {
        entries[i] = (entries[i] & 0xfff) + (palette + 2) * 0x1000;
    }
    GFL_BGSysLoadScrArea(2, 0, 0, width, 1, bg->screen->rawData, 0, 5, width, height);
    GFL_BGSysQueueScrLoad(2);
}

static TouchBar *PMSSelect_CreateTouchBar(PMSSelectWork *wk, ClActUnit *unit, HeapID heapId) {
    TouchBarSetup setup = { 0 };
    TouchBarItem items[] = {
        { 1, { 232, 168 } },
    };
    TouchBar *touchBar;

    setup.items = items;
    setup.count = NELEMS(items);
    setup.unit = unit;
    setup.bg = 1;
    setup.bgPalette = 14;
    setup.objPalette = 5;
    setup.vramType = CLACT_VRAM_SUB;
    touchBar = TouchBar_Create(&setup, heapId);
    TouchBar_SetIconBgPriority(touchBar, 1, TRUE);
    return touchBar;
}

static void PMSSelect_DeleteTouchBar(PMSSelectWork *wk) {
    TouchBar_Free(wk->touchBar);
}

static void PMSSelect_UpdateTouchBar(TouchBar *touchBar) {
    TouchBar_Main(touchBar);
}

static AppTaskMenu *PMSSelect_CreateMenu(AppTaskMenuRes *res, MsgData *msgData, HeapID heapId) {
    AppTaskMenuInit init;
    AppTaskMenuItem items[3];
    AppTaskMenu *menu;
    u32 i;

    for (i = 0; i < 3; i++) {
        items[i].str = GFL_MsgDataLoadStrbufNew(msgData, sPMSSelectMenuMessages[i]);
        items[i].color = PRINT_COLOR(14, 15, 3);
        items[i].type = 0;
    }
    items[2].type = APP_TASKMENU_ITEM_RETURN;
    init.heapId = heapId;
    init.itemCount = 3;
    init.items = items;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 13;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, res);
    for (i = 0; i < 3; i++) {
        GFL_StrBufFree(items[i].str);
    }
    return menu;
}

static void PMSSelect_DeleteMenu(AppTaskMenu *menu) {
    AppTaskMenu_Free(menu);
}

static void PMSSelect_UpdateMenu(AppTaskMenu *menu) {
    AppTaskMenu_Update(menu);
}

static void PMSSelect_SetupList(PMSSelectWork *wk) {
    int i;

    wk->pmsDraw = PMSDraw_Create(PMSSelectGraphic_GetClActUnit(wk->graphic), CLACT_VRAM_MAIN, wk->queue, wk->font, 8,
                                 PLATE_COUNT, wk->heapId);
    wk->sentenceCount = 0;
    for (i = 0; i < SENTENCE_MAX; i++) {
        if (PMSData_IsComplete(PMSWordSave_GetSentence(wk->wordSave, i), wk->heapId) == FALSE) {
            break;
        }
        wk->sentenceCount++;
    }
    wk->lineCount = wk->sentenceCount;
    if (wk->sentenceCount < SENTENCE_MAX) {
        wk->lineCount++;
    }
    func_0204c124(wk->scrollBar, wk->lineCount > 3);
    for (i = 0; i < PLATE_COUNT; i++) {
        wk->windows[i] = BmpWin_CreateDynamic(2, 2, i * 6 + 2, 25, 4, 9, TRUE);
    }
    PMSSelect_DrawList(wk);
    PMSSelect_UpdateScrollBar(wk, TRUE);
}

static void PMSSelect_DeleteList(PMSSelectWork *wk) {
    int i;

    PMSDraw_Delete(wk->pmsDraw);
    for (i = 0; i < PLATE_COUNT; i++) {
        BmpWin_Free(wk->windows[i]);
    }
}

static void PMSSelect_UpdateList(PMSSelectWork *wk) {
    PMSDraw_Main(wk->pmsDraw);
}

static void PMSSelect_DrawList(PMSSelectWork *wk) {
    PMSSelect_DrawListEx(wk, FALSE);
}

static void PMSSelect_RedrawList(PMSSelectWork *wk) {
    PMSSelect_DrawListEx(wk, TRUE);
    PMSSelect_BGResetPlateFade(&wk->bg);
}

static void PMSSelect_DrawListEx(PMSSelectWork *wk, BOOL waitPrint) {
    int i;
    u8 index;

    if (waitPrint && !PMSDraw_IsPrintEnd(wk->pmsDraw)) {
        return;
    }
    GFL_BGSysFillScrAsync(2, 0);
    if (wk->top != 0) {
        PMSSelect_BGDrawPlateEdge(&wk->bg, PMSData_GetType(PMSWordSave_GetSentence(wk->wordSave, (u8)(wk->top - 1))));
    }
    for (i = 0; i < PLATE_COUNT; i++) {
        if (PMSDraw_IsDrawn(wk->pmsDraw, i)) {
            PMSDraw_Clear(wk->pmsDraw, i, FALSE);
        }
    }
    for (i = 0; wk->top + i < wk->sentenceCount && i < PLATE_COUNT; i++) {
        index = wk->top + i;
        PMSSelect_DrawSentencePlate(wk, i, index, index == wk->pos);
    }
    if (wk->sentenceCount < SENTENCE_MAX && i < PLATE_COUNT) {
        index = wk->top + i;
        PMSSelect_DrawNewPlate(wk, i, 0, index == wk->pos);
    }
}

static void PMSSelect_UpdateScrollBar(PMSSelectWork *wk, BOOL force) {
    int max;
    int top;
    s16 y;

    if (!func_0204c138(wk->scrollBar)) {
        return;
    }
    max = wk->lineCount - 2;
    top = wk->top;
    if (!force) {
        int barY = func_0204c234(wk->scrollBar, 1);
        int start = top * SCROLL_BAR_RANGE / max + SCROLL_BAR_TOP;
        int end = (top + 1) * SCROLL_BAR_RANGE / max + SCROLL_BAR_TOP;

        if (top == 0) {
            if (barY < end) {
                return;
            }
        } else if (top == max - 1) {
            if (barY >= start) {
                return;
            }
        } else if (start <= barY && barY < end) {
            return;
        }
    }
    if (top == 0) {
        y = SCROLL_BAR_TOP;
    } else if (top == max - 1) {
        y = SCROLL_BAR_BOTTOM;
    } else {
        y = wk->top * SCROLL_BAR_RANGE / max + SCROLL_BAR_TOP + SCROLL_BAR_RANGE / (max * 2);
        if (y > SCROLL_BAR_BOTTOM) {
            y = SCROLL_BAR_BOTTOM;
        }
    }
    func_0204c228(wk->scrollBar, y, 1);
}

static BOOL PMSSelect_CheckScrollBarTouch(PMSSelectWork *wk) {
    u32 x, y;

    if (!func_0204c138(wk->scrollBar)) {
        return FALSE;
    }
    if (!func_0203dac8(&x, &y)) {
        return FALSE;
    }
    if (x >= 240 && x <= 247 && y <= 159 && y >= 8) {
        wk->scrollTouch = TRUE;
        return TRUE;
    }
    return FALSE;
}

static void PMSSelect_GetScrollPos(u32 y, u8 lineCount, u16 *top, s16 *barY) {
    int max = lineCount - 2;
    int i;

    for (i = 0; i < max; i++) {
        u32 start = i * SCROLL_BAR_RANGE / max + SCROLL_BAR_TOP;
        u32 end = (i * SCROLL_BAR_RANGE + SCROLL_BAR_RANGE) / max + SCROLL_BAR_TOP;

        if (i == 0) {
            if (y < end) {
                *top = i;
                break;
            }
        } else if (i == max - 1) {
            if (y >= start) {
                *top = i;
                break;
            }
        } else if (start <= y && y < end) {
            *top = i;
            break;
        }
    }
    if (y > SCROLL_BAR_BOTTOM) {
        y = SCROLL_BAR_BOTTOM;
    } else if (y < SCROLL_BAR_TOP) {
        y = SCROLL_BAR_TOP;
    }
    *barY = y;
}

static BOOL PMSSelect_ScrollByTouch(PMSSelectWork *wk) {
    u32 x, y;
    u16 top;
    s16 barY;

    if (!func_0203da84(&x, &y)) {
        wk->scrollTouch = FALSE;
        return FALSE;
    }
    PMSSelect_GetScrollPos(y, wk->lineCount, &top, &barY);
    func_0204c228(wk->scrollBar, barY, 1);
    if (top != wk->top) {
        wk->top = top;
        return TRUE;
    }
    return FALSE;
}

static BOOL PMSSelect_ListInput(PMSSelectWork *wk) {
    BOOL decided = FALSE;
    BOOL cont = TRUE;

    if (!wk->scrollTouch) {
        BOOL key = FALSE;

        if (GCTX_HIDGetPressedKeys() & ~PAD_BUTTON_B) {
            key = TRUE;
        }
        if (key && wk->pos == POS_NONE) {
            if (wk->prevPos == POS_NONE) {
                wk->pos = wk->top;
            } else if (wk->top <= wk->prevPos && wk->prevPos < wk->top + 3) {
                wk->pos = wk->prevPos;
            } else {
                wk->pos = wk->top;
            }
            wk->prevPos = POS_NONE;
            cont = FALSE;
            wk->touchMode = FALSE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            PMSSelect_RedrawList(wk);
        }
    }

    if (cont && wk->pos != POS_NONE && !wk->scrollTouch) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            decided = TRUE;
            cont = FALSE;
            wk->touchMode = FALSE;
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            if (wk->pos != 0) {
                wk->prevPos = wk->pos;
                wk->pos--;
                if (wk->pos + 1 == wk->top) {
                    wk->top--;
                }
                PMSSelect_UpdateScrollBar(wk, FALSE);
                PMSSelect_RedrawList(wk);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            cont = FALSE;
            wk->touchMode = FALSE;
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            if (wk->pos < wk->lineCount - 1) {
                wk->prevPos = wk->pos;
                wk->pos++;
                if (wk->pos == wk->top + 3) {
                    wk->top++;
                }
                PMSSelect_UpdateScrollBar(wk, FALSE);
                PMSSelect_RedrawList(wk);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            cont = FALSE;
            wk->touchMode = FALSE;
        }
    }

    if (cont) {
        if (wk->scrollTouch) {
            if (PMSSelect_ScrollByTouch(wk)) {
                PMSSelect_RedrawList(wk);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            wk->touchMode = TRUE;
        } else {
            int plate = PMSSelect_GetTouchedPlate();

            if (plate != -1 && plate > wk->lineCount - 1) {
                plate = -1;
            }
            if (plate != -1) {
                wk->prevPos = wk->pos;
                wk->pos = wk->top + plate;
                PMSSelect_RedrawList(wk);
                decided = TRUE;
                wk->touchMode = TRUE;
            } else {
                if (!wk->scrollTouch) {
                    PMSSelect_CheckScrollBarTouch(wk);
                }
                if (wk->scrollTouch == TRUE) {
                    if (PMSSelect_ScrollByTouch(wk) || wk->pos != POS_NONE) {
                        wk->prevPos = wk->pos;
                        wk->pos = POS_NONE;
                        PMSSelect_RedrawList(wk);
                    }
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    wk->touchMode = TRUE;
                }
            }
        }
    }
    return decided;
}

static void PMSSelect_FlushWindows(PMSSelectWork *wk) {
    int i;

    for (i = 0; i < PLATE_COUNT; i++) {
        PMSSelect_FlushWindow(wk->queue, wk->windows[i], &wk->flushPending[i]);
    }
}

static BOOL PMSSelect_FlushWindow(PrintQueue *queue, BmpWin *window, BOOL *flushPending) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(window);

    if (*flushPending == TRUE && !func_02021c1c(queue, bitmap)) {
        BmpWin_Transfer(window);
        *flushPending = FALSE;
        return TRUE;
    }
    return FALSE;
}

static void PMSSelect_DrawNewPlate(PMSSelectWork *wk, u8 line, u8 index, BOOL active) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PmsInput_Text_AddNewMessages);
    u16 color;

    PMSSelect_BGDrawPlate(&wk->bg, line, 6, active);
    if (active) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[line]), 14);
        color = PRINT_COLOR(1, 2, 14);
    } else {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[line]), 13);
        color = PRINT_COLOR(1, 2, 13);
    }
    func_02021c7c(wk->queue, BmpWin_GetBitmap(wk->windows[line]), 0, 0, strbuf, wk->font, color);
    GFL_StrBufFree(strbuf);
    BmpWin_FlushMap(wk->windows[line]);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(wk->windows[line]));
    wk->flushPending[line] = TRUE;
}

static void PMSSelect_DrawSentencePlate(PMSSelectWork *wk, u8 line, u8 index, BOOL active) {
    PMSData *sentence = PMSWordSave_GetSentence(wk->wordSave, index);

    PMSSelect_BGDrawPlate(&wk->bg, line, PMSData_GetType(sentence), active);
    if (active) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[line]), 14);
        PMSDraw_SetColor(wk->pmsDraw, PRINT_COLOR(1, 2, 14));
        PMSDraw_SetBackColor(wk->pmsDraw, 14);
    } else {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[line]), 13);
        PMSDraw_SetColor(wk->pmsDraw, PRINT_COLOR(1, 2, 13));
        PMSDraw_SetBackColor(wk->pmsDraw, 13);
    }
    if (PMSData_IsComplete(sentence, wk->heapId) == TRUE) {
        PMSDraw_Print(wk->pmsDraw, wk->windows[line], sentence, line);
        wk->flushPending[line] = TRUE;
    }
}

static int PMSSelect_GetTouchedPlate(void) {
    u32 x, y;

    if (func_0203dac8(&x, &y) && x < 232 && y >= 8) {
        u32 plate = (y / 8 - 1) / 6;

        if (plate < 3) {
            return plate;
        }
    }
    return -1;
}

static BOOL PMSSelect_SeqTouchBarOn(u32 *seq, PMSSelectWork *wk) {
    TouchBar_SetVisible(wk->touchBar, TRUE);
    return TRUE;
}

static BOOL PMSSelect_SeqSelect(u32 *seq, PMSSelectWork *wk) {
    BOOL end;
    int touch;

    touch = -1;
    end = FALSE;
    PMSSelect_BGUpdatePlateFade(&wk->bg);
    if (wk->bg.decided == FALSE) {
        BOOL decided = FALSE;

        if (!wk->scrollTouch) {
            func_0203d564(wk->touchMode ? TRUE : FALSE);
            PMSSelect_UpdateTouchBar(wk->touchBar);
            if (TouchBar_GetTouched(wk->touchBar) != -1) {
                wk->listActive = FALSE;
            }
            touch = TouchBar_GetDecided(wk->touchBar);
            if (touch != -1) {
                wk->listActive = TRUE;
            }
            wk->touchMode = func_0203d554() == TRUE;
        }
        if (touch != -1) {
            switch (touch) {
            case 1:
                wk->param->cancel = TRUE;
                wk->param->result = NULL;
                wk->nextSeq = SEQ_FADE_OUT;
                return TRUE;
            case 7:
                break;
            }
        }
        if (wk->listActive) {
            decided = PMSSelect_ListInput(wk);
        }
        if (decided) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            wk->bg.decided = TRUE;
        }
    } else if (wk->bg.blinkCount == 16) {
        end = TRUE;
    }
    if (end) {
        if (PMSData_IsComplete(PMSWordSave_GetSentence(wk->wordSave, wk->pos), wk->heapId)) {
            wk->nextSeq = SEQ_MENU_OPEN;
        } else {
            wk->nextSeq = SEQ_INPUT_FADE_OUT;
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL PMSSelect_SeqTouchBarOff(u32 *seq, PMSSelectWork *wk) {
    TouchBar_SetVisible(wk->touchBar, FALSE);
    return TRUE;
}

static BOOL PMSSelect_SeqMenuOpen(u32 *seq, PMSSelectWork *wk) {
    func_0203d564(wk->touchMode ? TRUE : FALSE);
    wk->menu = PMSSelect_CreateMenu(wk->menuRes, wk->msgData, wk->heapId);
    BrightnessController_SetScreenBrightness(-8, 30, 1);
    return TRUE;
}

static BOOL PMSSelect_SeqMenu(u32 *seq, PMSSelectWork *wk) {
    PMSSelect_UpdateMenu(wk->menu);
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        switch (AppTaskMenu_GetCursorPos(wk->menu)) {
        case 0:
            wk->nextSeq = SEQ_FADE_OUT;
            wk->param->result = PMSWordSave_GetSentence(wk->wordSave, wk->pos);
            wk->param->cancel = FALSE;
            break;
        case 1:
            wk->nextSeq = SEQ_INPUT_FADE_OUT;
            break;
        case 2:
            wk->nextSeq = SEQ_SELECT;
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL PMSSelect_SeqMenuClose(u32 *seq, PMSSelectWork *wk) {
    PMSSelect_DeleteMenu(wk->menu);
    wk->touchMode = func_0203d554() == TRUE;
    if (wk->nextSeq == SEQ_SELECT) {
        BrightnessController_SetScreenBrightness(0, 30, 1);
        PMSSelect_BGResetPlateFade(&wk->bg);
        wk->bg.blinkCount = 0;
        wk->bg.decided = FALSE;
        if (wk->touchMode) {
            wk->prevPos = wk->pos;
            wk->pos = POS_NONE;
            PMSSelect_RedrawList(wk);
        }
    }
    return TRUE;
}

static BOOL PMSSelect_SeqCallInput(u32 *seq, PMSSelectWork *wk) {
    PMSIParam *param;

    switch (*seq) {
    case 0:
        (*seq)++;
        break;
    case 1:
        param = PMSIParam_Create(2, 0, NULL, TRUE, wk->save, wk->heapId);
        PMSIParam_SetSentence(param, PMSWordSave_GetSentence(wk->wordSave, wk->pos));
        QueueGameProc(wk->procMgr, OVERLAY_NONE, &PMS_INPUT_PROC_FUNCTIONS, param);
        wk->inputParam = param;
        return TRUE;
    }
    return FALSE;
}

static BOOL PMSSelect_SeqInputReturn(u32 *seq, PMSSelectWork *wk) {
    PMSData sentence;

    switch (*seq) {
    case 0:
        if (PMSIParam_IsChanged(wk->inputParam)) {
            PMSIParam_GetSentence(wk->inputParam, &sentence);
            PMSWordSave_SetSentence(wk->wordSave, wk->pos, &sentence);
        }
        PMSIParam_Free(wk->inputParam);
        (*seq)++;
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}
