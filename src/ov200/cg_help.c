#include "types.h"
#include "app/cg_help.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/player_info.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "text/system/cg_help.h"

#define PAGE_COUNT 15
// The titles of the pages are the first messages of the bank and their texts the ones after
#define TITLE_MESSAGE(page) (CgHelp_Text_AboutInfrared + (page))
#define TEXT_MESSAGE(page) (CgHelp_Text_UsingInfraredCanBattle + (page))

// The sub screen's BGs, which show the help
enum {
    BG_TEXT = 4,
    BG_MESSAGE = 5,
    BG_FRAME = 6,
    BG_SCROLLING = 7,
};

enum {
    STATE_WIPE_IN,
    STATE_WAIT_WIPE_IN,
    STATE_CLOSE,
    STATE_WAIT_WIPE_OUT,
    STATE_PAGES,
};

// The touch screen's buttons
enum {
    BUTTON_PREVIOUS,
    BUTTON_NEXT,
    BUTTON_QUIT,
};

// The two cell actors, whose animations the pages show
#define ACTOR_COUNT 2

typedef struct {
    u16 heapId;
    CGHelpParam *param;
    BOOL netError;
    BOOL redraw;
    u32 state;
    TCB *vblankTask;
    u8 page;
    // Whether the page's text is still being printed
    BOOL printing;
    BmpWin *textWindow;
    BmpWin *titleWindow;
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    AppTaskMenuRes *taskMenuRes;
    AppTaskMenuWin *previousButton;
    AppTaskMenuWin *nextButton;
    AppTaskMenuWin *quitButton;
    ClActUnit *clActUnit;
    u32 palette;
    u32 chars[ACTOR_COUNT];
    u32 cellAnims[ACTOR_COUNT];
    ClActor *actors[ACTOR_COUNT];
} CGHelp;

static BOOL CGHelp_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CGHelp_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CGHelp_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void CGHelp_Setup(CGHelp *help);
static void CGHelp_Cleanup(CGHelp *help);
static BOOL CGHelp_Update(CGHelp *help);
static void CGHelp_VBlank(TCB *tcb, void *data);
static void CGHelp_InitBGs(CGHelp *help);
static void CGHelp_FreeBGs(CGHelp *help);
static void CGHelp_CreateBG(const BGSetup *setup, u8 bg, u8 mode);
static void CGHelp_LoadGraphics(CGHelp *help);
static void CGHelp_FreeGraphics(CGHelp *help);
static void CGHelp_InitText(CGHelp *help);
static void CGHelp_FreeText(CGHelp *help);
static void CGHelp_HandleInput(CGHelp *help);
static void CGHelp_DrawPage(CGHelp *help, u8 page);
static void CGHelp_ShowAnimation(CGHelp *help, u8 page);

GameProcFunctions CG_HELP_PROC_FUNCTIONS = {
    CGHelp_Init,
    CGHelp_Main,
    CGHelp_Exit,
};

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

// What a page shows: the animation of one of the two actors
typedef struct {
    u16 sequence : 8;
    u16 actor : 1;
    u16 : 7;
} PageAnimation;

static const PageAnimation sPageAnimations[8] = {
    { 0, 1 }, { 2, 1 }, { 1, 1 }, { 0x1f, 0 }, { 0xc, 0 }, { 0xa, 0 }, { 6, 0 }, { 0, 0 },
};

static const BGSetup sBGSetupMessage = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6800),
    GX_BG_CHARBASE(0x00000),
    0x6000,
    GX_BG_EXTPLTT_23,
    1,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sBGSetupFrame = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7000),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_23,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sBGSetupScrolling = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7800),
    GX_BG_CHARBASE(0x18000),
    0x8000,
    GX_BG_EXTPLTT_23,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sBGSetupText = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6000),
    GX_BG_CHARBASE(0x08000),
    0x8000,
    GX_BG_EXTPLTT_23,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// The picture of each page, as an index of sPageAnimations
static const u32 sPageAnimationIndex[PAGE_COUNT] = { 0, 1, 1, 2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6 };

static void CGHelp_Setup(CGHelp *help) {
    help->state = STATE_WIPE_IN;
    CGHelp_InitBGs(help);
    CGHelp_LoadGraphics(help);
    CGHelp_InitText(help);
    help->vblankTask = GFL_VBlankTCBAdd(CGHelp_VBlank, help, 8);
    help->page = 0;
    help->redraw = TRUE;
    CGHelp_DrawPage(help, help->page);
    GFL_NetSetIconScreen(FALSE, help->heapId);
}

static void CGHelp_Cleanup(CGHelp *help) {
    GFL_TCBRemove(help->vblankTask);
    CGHelp_FreeText(help);
    CGHelp_FreeGraphics(help);
    CGHelp_FreeBGs(help);
}

static BOOL CGHelp_Update(CGHelp *help) {
    switch (help->state) {
    case STATE_WIPE_IN:
        GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, help->heapId);
        help->state = STATE_WAIT_WIPE_IN;
        break;
    case STATE_WAIT_WIPE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            help->state = STATE_PAGES;
        }
        break;
    case STATE_CLOSE:
        if (AppTaskMenuWin_IsFlashFinished(help->quitButton) == TRUE) {
            GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, help->heapId);
            help->state = STATE_WAIT_WIPE_OUT;
        }
        break;
    case STATE_WAIT_WIPE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    case STATE_PAGES:
        if (help->printing == FALSE) {
            CGHelp_HandleInput(help);
        }
        break;
    }
    PrintQueue_Main(help->printQueue);
    ClActSys_Draw();
    if (help->nextButton != NULL) {
        AppTaskMenuWin_Update(help->nextButton);
    }
    if (help->previousButton != NULL) {
        AppTaskMenuWin_Update(help->previousButton);
    }
    AppTaskMenuWin_Update(help->quitButton);
    if (help->printing == TRUE && PrintQueue_IsEmpty(help->printQueue) == TRUE) {
        if (AppTaskMenuWin_IsFlashFinished(help->nextButton) == TRUE ||
            AppTaskMenuWin_IsFlashFinished(help->previousButton) == TRUE || help->redraw == TRUE) {
            BmpWin_FlushChar(help->textWindow);
            BmpWin_FlushChar(help->titleWindow);
            help->printing = FALSE;
            help->redraw = FALSE;
            CGHelp_ShowAnimation(help, help->page);
            if (help->page == PAGE_COUNT - 1) {
                AppTaskMenuWin_ResetFlash(help->nextButton);
                AppTaskMenuWin_ClearScreen(help->nextButton);
            } else {
                AppTaskMenuWin_FlushMap(help->nextButton);
                AppTaskMenuWin_ResetFlash(help->nextButton);
            }
            if (help->page == 0) {
                AppTaskMenuWin_ResetFlash(help->previousButton);
                AppTaskMenuWin_ClearScreen(help->previousButton);
            } else {
                AppTaskMenuWin_FlushMap(help->previousButton);
                AppTaskMenuWin_ResetFlash(help->previousButton);
            }
        }
    }
    GFL_BGSysMoveBG(BG_SCROLLING, BG_MOVE_UP, 1);
    return FALSE;
}

static void CGHelp_VBlank(TCB *tcb, void *data) {
    ClActSys_VBlank();
}

static void CGHelp_InitBGs(CGHelp *help) {
    ClActSysSetup clActSetup;

    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    GFL_BGSysCreate(help->heapId);
    BmpWin_InitAllocator(help->heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    CGHelp_CreateBG(&sBGSetupText, BG_TEXT, BGMODE_TEXT);
    CGHelp_CreateBG(&sBGSetupMessage, BG_MESSAGE, BGMODE_TEXT);
    CGHelp_CreateBG(&sBGSetupFrame, BG_FRAME, BGMODE_TEXT);
    CGHelp_CreateBG(&sBGSetupScrolling, BG_SCROLLING, BGMODE_TEXT);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 15, 4);
    clActSetup = data_02093f08;
    clActSetup.charCount = 0x40;
    ClActSys_Create(&clActSetup, &sVRAMConfig, help->heapId);
    help->clActUnit = ClActUnit_Create(0x30, 0, help->heapId);
    ClActUnit_UseSystemRenderer(help->clActUnit);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void CGHelp_FreeBGs(CGHelp *help) {
    ClActUnit_Delete(help->clActUnit);
    ClActSys_Delete();
    GFL_BGSysReleaseBG(BG_TEXT);
    GFL_BGSysReleaseBG(BG_MESSAGE);
    GFL_BGSysReleaseBG(BG_FRAME);
    GFL_BGSysReleaseBG(BG_SCROLLING);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void CGHelp_CreateBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void CGHelp_LoadGraphics(CGHelp *help) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, help->heapId);
    ClActorSetup actorSetup;
    u8 gender;
    int i;

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0, help->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, BG_FRAME, 0, 0, FALSE, help->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 10, BG_FRAME, 0, 0, FALSE, help->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, BG_SCROLLING, 0, 0, FALSE, help->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 9, BG_SCROLLING, 0, 0, FALSE, help->heapId);
    GFL_ArcToolFree(arc);
    gender = getTrainerGender(help->param->playerInfo) & 1;
    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, help->heapId);
    help->palette = ClActSys_LoadPlttRange(arc, 14 + gender, CLACT_VRAM_SUB, 0, 0, 15, help->heapId);
    help->chars[0] = ClActSys_LoadChars(arc, 16, FALSE, CLACT_VRAM_SUB, help->heapId);
    help->cellAnims[0] = ClActSys_LoadCellAnims(arc, 17, 18, help->heapId);
    help->chars[1] = ClActSys_LoadChars(arc, 19, FALSE, CLACT_VRAM_SUB, help->heapId);
    help->cellAnims[1] = ClActSys_LoadCellAnims(arc, 29, 30, help->heapId);
    GFL_ArcToolFree(arc);
    actorSetup.x = 32;
    actorSetup.y = 48;
    actorSetup.sequence = 0;
    actorSetup.bgPriority = 0;
    for (i = 0; i < ACTOR_COUNT; i++) {
        actorSetup.priority = i;
        help->actors[i] = ClActor_Create(help->clActUnit, help->chars[i], help->palette, help->cellAnims[i],
                                         &actorSetup, CLACT_SURFACE_SUB, help->heapId);
        ClActor_SetVisible(help->actors[i], FALSE);
        func_0204c520(help->actors[i], FALSE);
    }
}

static void CGHelp_FreeGraphics(CGHelp *help) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(help->actors[i]);
    }
    ClActSys_FreeCellAnims(help->cellAnims[1]);
    ClActSys_FreeChars(help->chars[1]);
    ClActSys_FreeCellAnims(help->cellAnims[0]);
    ClActSys_FreeChars(help->chars[0]);
    ClActSys_FreePltt(help->palette);
}

static void CGHelp_InitText(CGHelp *help) {
    AppTaskMenuItem item;

    help->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, help->heapId);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    help->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_CG_HELP, help->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 15 * 32, 0x20, help->heapId);
    LoadSysMsgBox(BG_MESSAGE, 1, 13, 0, help->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), 4, 13 * 32, 0x40, help->heapId);
    GFL_TextRndResetColorIndexLUT();
    help->printQueue = PrintQueue_CreateDefault(help->heapId);
    help->taskMenuRes = AppTaskMenuRes_Create(BG_TEXT, 11, help->font, help->printQueue, help->heapId);
    help->titleWindow = BmpWin_CreateDynamic(BG_MESSAGE, 2, 4, 28, 4, 15, 1);
    help->textWindow = BmpWin_CreateDynamic(BG_MESSAGE, 2, 9, 28, 12, 15, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(help->titleWindow), 1);
    BmpWin_FlushChar(help->titleWindow);
    BmpWin_FlushMap(help->titleWindow);
    BmpWin_DrawFrame(help->titleWindow, WINFRAME_TRANSFER_VBLANK, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(help->textWindow), 0);
    BmpWin_FlushChar(help->textWindow);
    BmpWin_FlushMap(help->textWindow);
    GFL_BGSysLoadScr(BG_MESSAGE);
    GFL_BGSysLoadScr(BG_TEXT);
    help->printing = FALSE;
    item.str = GFL_MsgDataLoadStrbufNew(help->msgData, CgHelp_Text_Next);
    item.color = 0x39e3;
    item.type = 0;
    help->nextButton = AppTaskMenuWin_Create(help->taskMenuRes, &item, 11, 21, 11, help->heapId);
    GFL_StrBufFree(item.str);
    item.str = GFL_MsgDataLoadStrbufNew(help->msgData, CgHelp_Text_Previous);
    help->previousButton = AppTaskMenuWin_Create(help->taskMenuRes, &item, 0, 21, 11, help->heapId);
    GFL_StrBufFree(item.str);
    item.str = GFL_MsgDataLoadStrbufNew(help->msgData, CgHelp_Text_Quit);
    item.type = APP_TASKMENU_ITEM_RETURN;
    help->quitButton = AppTaskMenuWin_Create(help->taskMenuRes, &item, 22, 21, 10, help->heapId);
    GFL_StrBufFree(item.str);
    AppTaskMenuWin_ClearScreen(help->previousButton);
    *(vu16 *)0x050005e2 = 0;
}

static void CGHelp_FreeText(CGHelp *help) {
    if (help->nextButton != NULL) {
        AppTaskMenuWin_Free(help->nextButton);
    }
    if (help->previousButton != NULL) {
        AppTaskMenuWin_Free(help->previousButton);
    }
    AppTaskMenuWin_Free(help->quitButton);
    AppTaskMenuRes_Free(help->taskMenuRes);
    PrintQueue_Free(help->printQueue);
    BmpWin_Free(help->textWindow);
    BmpWin_Free(help->titleWindow);
    GFL_MsgDataFree(help->msgData);
    GFL_FontFree(help->font);
    GFL_TextRndResetColorIndexLUT();
}

static void CGHelp_HandleInput(CGHelp *help) {
    TouchRect rects[4] = {
        { 168, 192, 0, 88 },
        { 168, 192, 88, 176 },
        { 168, 192, 176, 255 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    switch (GFL_HIDGetTouchPressedRect(rects)) {
    case BUTTON_PREVIOUS:
        if (help->page != 0) {
            help->page--;
            CGHelp_DrawPage(help, help->page);
            AppTaskMenuWin_SetFlashing(help->previousButton, TRUE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case BUTTON_NEXT:
        if (help->page < PAGE_COUNT - 1) {
            help->page++;
            CGHelp_DrawPage(help, help->page);
            AppTaskMenuWin_SetFlashing(help->nextButton, TRUE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case BUTTON_QUIT:
        AppTaskMenuWin_SetFlashing(help->quitButton, TRUE);
        help->state = STATE_CLOSE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        break;
    }
    if (func_02016bec(help->param->gsys) == TRUE) {
        AppTaskMenuWin_SetFlashing(help->quitButton, TRUE);
        help->state = STATE_CLOSE;
    }
}

static void CGHelp_DrawPage(CGHelp *help, u8 page) {
    StrBuf *title;
    StrBuf *text;

    GFL_BitmapFill(BmpWin_GetBitmap(help->titleWindow), 1);
    GFL_BitmapFill(BmpWin_GetBitmap(help->textWindow), 0);
    title = GFL_MsgDataLoadStrbufNew(help->msgData, TITLE_MESSAGE(page));
    PrintQueue_PrintColor(help->printQueue, BmpWin_GetBitmap(help->titleWindow), 40, 8, title, help->font,
                          PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(title);
    text = GFL_MsgDataLoadStrbufNew(help->msgData, TEXT_MESSAGE(page));
    PrintQueue_PrintColor(help->printQueue, BmpWin_GetBitmap(help->textWindow), 0, 0, text, help->font,
                          PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(text);
    help->printing = TRUE;
}

static void CGHelp_ShowAnimation(CGHelp *help, u8 page) {
    const PageAnimation *animation = &sPageAnimations[sPageAnimationIndex[page]];

    ClActor_SetSequence(help->actors[animation->actor], animation->sequence);
    ClActor_SetVisible(help->actors[animation->actor], TRUE);
    ClActor_SetVisible(help->actors[animation->actor ^ 1], FALSE);
}

static BOOL CGHelp_Init(GameProc *proc, u32 *state, void *param, void *work) {
    CGHelp *help;
    CGHelpParam *helpParam = param;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_CG_HELP, 0x60000);
    help = GFL_ProcInitSubsystem(proc, sizeof(CGHelp), HEAPID_CG_HELP);
    if (helpParam == NULL) {
        helpParam = GFL_HeapAllocate(HEAPID_CG_HELP, 8, TRUE, "cg_help.c", 801);
        helpParam->playerInfo = PlayerInfo_Create(HEAPID_CG_HELP);
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_R) {
            setTrainerGender(helpParam->playerInfo, 1);
        } else {
            setTrainerGender(helpParam->playerInfo, 0);
        }
    }
    help->param = helpParam;
    help->heapId = HEAPID_CG_HELP;
    CGHelp_Setup(help);
    return TRUE;
}

static BOOL CGHelp_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    CGHelp *help = work;

    CGHelp_Cleanup(help);
    if (param == NULL) {
        GFL_HeapFree(help->param->playerInfo);
        GFL_HeapFree(help->param);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_CG_HELP);
    return TRUE;
}

static BOOL CGHelp_Main(GameProc *proc, u32 *state, void *param, void *work) {
    CGHelp *help = work;
    BOOL done = CGHelp_Update(help);

    if (GFL_NetErrCheck() && !help->netError) {
        help->netError = TRUE;
        help->state = STATE_CLOSE;
    }
    if (done == TRUE) {
        return TRUE;
    }
    return FALSE;
}
