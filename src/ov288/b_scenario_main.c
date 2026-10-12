#include "types.h"
#include "battle/b_scenario_graphic.h"
#include "battle/b_scenario_main.h"
#include "battle/btlv_effect.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/pokewood_lines.h"

// The scenario screen's entry, task and pages (b_scenario_main.c, named by the ROM's embedded string): a page runs
// its three functions of sBScenarioPageFuncs in turn, the last taking the next page's first, until a page asks for
// no next one, and the screen fades out. Where this file ends and b_scenario_graphic.c starts is a guess: the
// functions from BScenarioMain_CountScriptLines to BScenarioMain_LoadBG refer to no data of either.

// The battle's graphics archive, and its files
#define ARCID_BATTLE_SCENARIO 100
#define FILE_LABEL_PALETTE 15
#define FILE_POSITIONS 41

// The BGs of the lower screen
#define BG_FRAME 4
#define BG_BUTTONS 5
#define BG_FLASH 6
#define BG_PAGES 7

#define TEXT_X_CENTER 0x70
#define TEXT_COLOR 0x440
#define TEXT_COLOR_LABEL 0x3c40
#define TEXT_COLOR_MALE 0x18a0
#define TEXT_COLOR_FEMALE 0x1060
#define ALIGN_CENTER 2

// The lines of a script page, and the most a script has
#define SCRIPT_PAGE_LINES 2
#define SCRIPT_LINE_MAX 20
// The text that ends what the scenes ask for, in a film's script
#define SCRIPT_MSG_END 41

// The states of the scenario screen
enum {
    STEP_INIT = 0,
    STEP_WAIT_WIPE = 10,
    STEP_PAGES = 100,
    STEP_FADE_OUT = 200,
    STEP_END = 10000,
};

// The states of a page
enum {
    PAGE_STEP_INIT = 0,
    PAGE_STEP_INPUT = 100,
    PAGE_STEP_BACK = 200,
    PAGE_STEP_PREV = 210,
    PAGE_STEP_NEXT = 220,
    PAGE_STEP_FLASH = 300,
    PAGE_STEP_FLASH_CAST = 310,
    PAGE_STEP_IDLE = 400,
};

// The page functions, three a page
enum {
    PAGE_SEQ_SUMMARY = 0,
    PAGE_SEQ_SCRIPT = 3,
    PAGE_SEQ_CAST = 6,
    PAGE_SEQ_COUNT = 9,
    PAGE_SEQ_END = -1,
};

enum {
    PAGE_SUMMARY,
    PAGE_SCRIPT,
    PAGE_CAST,
};

// What the player does on a page
enum {
    INPUT_NONE,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_DECIDE,
    INPUT_BACK,
    // The page's touch rectangles, in order
    INPUT_TOUCH,
};

// A label of a page: the index of its text in sBScenarioLabelMessages, its window, whether it is in the text's
// color, and the message file
typedef struct {
    u16 label;
    u16 window;
    u16 dark;
    u16 msgData;
} BScenarioLabel;

typedef void (*BScenarioPageFunc)(BScenarioWork *work);

static void BScenarioMain_Task(TCB *tcb, void *data);
static BOOL BScenarioMain_Finish(TCB *tcb, BScenarioWork *work);
static void BScenarioMain_Step(BScenarioWork *work);
static void BScenarioMain_Init(BScenarioWork *work);
static void BScenarioMain_StartWipe(BScenarioWork *work);
static BOOL BScenarioMain_IsWiping(void);
static void BScenarioMain_StartFadeOut(BScenarioWork *work);
static BOOL BScenarioMain_IsFading(BScenarioWork *work);
static void BScenarioMain_InitBG(BScenarioWork *work);
static void BScenarioMain_FreeBG(BScenarioWork *work);
static void BScenarioMain_InitPalettes(BScenarioWork *work);
static void BScenarioMain_ApplyPage(BScenarioWork *work);
static void BScenarioMain_SummaryInit(BScenarioWork *work);
static void BScenarioMain_SummaryUpdate(BScenarioWork *work);
static void BScenarioMain_SummaryExit(BScenarioWork *work);
static void BScenarioMain_SummaryInput(BScenarioWork *work);
static void BScenarioMain_PrintSummary(BScenarioWork *work, BOOL update);
static void BScenarioMain_ScriptInit(BScenarioWork *work);
static void BScenarioMain_ScriptUpdate(BScenarioWork *work);
static void BScenarioMain_ScriptExit(BScenarioWork *work);
static void BScenarioMain_ScriptInput(BScenarioWork *work);
static void BScenarioMain_PrintScript(BScenarioWork *work, BOOL update);
static void BScenarioMain_CastInit(BScenarioWork *work);
static void BScenarioMain_CastUpdate(BScenarioWork *work);
static void BScenarioMain_CastExit(BScenarioWork *work);
static void BScenarioMain_CastInput(BScenarioWork *work);
static void BScenarioMain_PrintCast(BScenarioWork *work, BOOL update);
static void BScenarioMain_UpdateCastActors(BScenarioWork *work, BOOL update);
static void BScenarioMain_HideCastActors(BScenarioWork *work);
static BOOL BScenarioMain_UpdatePage(BScenarioWork *work);
static void BScenarioMain_NextPage(BScenarioWork *work);
static void BScenarioMain_ReadCast(BScenarioWork *work);
static void BScenarioMain_PrintLabels(BScenarioWork *work, int page);
static void BScenarioMain_CountScriptLines(BScenarioWork *work);
static void BScenarioMain_UpdateArrows(BScenarioWork *work, int page, BOOL apply);
static void BScenarioMain_UpdateArrowAnims(BScenarioWork *work);
static void BScenarioMain_UpdateCursor(BScenarioWork *work, int page, int cursor);
static void BScenarioMain_InitScriptPage(BScenarioWork *work);
static BOOL BScenarioMain_IsCloseRequested(BScenarioWork *work);
static void BScenarioMain_AfterPage(BScenarioWork *work);
static void BScenarioMain_LoadBG(BScenarioWork *work);

// The summary's texts from the film's number: its title, its plot and its mission
static const s16 sBScenarioSummaryMessages[3] = {
    PokewoodLines_Text_BrycenMan,
    PokewoodLines_Text_HeroBattlesMaskedMan,
    PokewoodLines_Text_KnockOutOpponentEvery,
};

// The palette files of the films' series
static const s16 sBScenarioSeriesPalettes[4] = { 16, 17, 18, 19 };

static const s16 sBScenarioLabelMessages[9] = {
    PokewoodLines_Text_Title,
    PokewoodLines_Text_Plot,
    PokewoodLines_Text_Mission,
    PokewoodLines_Text_Script,
    PokewoodLines_Text_Costars,
    PokewoodLines_Text_Cast,
    PokewoodLines_Text_HeldItems,
    PokewoodLines_Text_Ability,
    // The film's title, in its script
    0,
};

enum {
    CAST_MSG_SPECIES,
    CAST_MSG_MALE,
    CAST_MSG_FEMALE,
    CAST_MSG_LEVEL,
    CAST_MSG_ITEM,
    CAST_MSG_ABILITY,
    CAST_MSG_MOVES,
    CAST_MSG_SLASH = 10,
};

// The cast page's texts, and the slash of the script page's page number
static const s16 sBScenarioCastMessages[11] = {
    PokewoodLines_Text_Empty_3,  PokewoodLines_Text_Empty_4,  PokewoodLines_Text_Empty_5,  PokewoodLines_Text_Lv,
    PokewoodLines_Text_Empty_6,  PokewoodLines_Text_Empty_7,  PokewoodLines_Text_Empty_8,  PokewoodLines_Text_Empty_9,
    PokewoodLines_Text_Empty_10, PokewoodLines_Text_Empty_11, PokewoodLines_Text_Empty_12,
};

static const BScenarioLabel sBScenarioScriptLabels[3] = {
    { 4, 5, FALSE, 0 },
    { 8, 0, FALSE, 1 },
    { 0xffff, 0, 0, 0 },
};

static const BScenarioLabel sBScenarioCastLabels[5] = {
    { 5, 0, FALSE, 0 }, { 6, 3, TRUE, 0 }, { 7, 9, TRUE, 0 }, { 3, 11, FALSE, 0 }, { 0xffff, 0, 0, 0 },
};

// The series of each film, which gives the screen its colors
static const u8 sBScenarioSeries[BSCENARIO_SCENARIO_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
};

enum {
    SCRIPT_MSG_SCENES,
    SCRIPT_MSG_ORDERS,
};

// In a film's script, the first of the headings of its scenes and of what the scenes ask for. As with
// sBScenarioScriptWindowIndices, the code has only the values
static const s16 sBScenarioScriptMessages[2] = { 1, 21 };

enum {
    SCRIPT_WINDOW_SLASH,
    SCRIPT_WINDOW_ORDERS,
};

// The script page's window of the slash of the page number, and the first of those of what the scenes ask for. The
// ROM has the table, and the code only its values: which of the page's constants were read from it is a guess
static const s16 sBScenarioScriptWindowIndices[2] = { 8, 3 };

static const BScenarioLabel sBScenarioSummaryLabels[6] = {
    { 0, 0, TRUE, 0 },  { 1, 1, TRUE, 0 },  { 2, 2, TRUE, 0 },
    { 3, 6, FALSE, 0 }, { 4, 7, FALSE, 0 }, { 0xffff, 0, 0, 0 },
};

static const BScenarioLabel *sBScenarioLabelTables[3] = { sBScenarioSummaryLabels, sBScenarioScriptLabels,
                                                          sBScenarioCastLabels };

static BScenarioPageFunc sBScenarioPageFuncs[PAGE_SEQ_COUNT] = {
    BScenarioMain_SummaryInit, BScenarioMain_SummaryUpdate, BScenarioMain_SummaryExit,
    BScenarioMain_ScriptInit,  BScenarioMain_ScriptUpdate,  BScenarioMain_ScriptExit,
    BScenarioMain_CastInit,    BScenarioMain_CastUpdate,    BScenarioMain_CastExit,
};

// The script's pages, of two scenes each
static inline int BScenarioMain_GetScriptPageCount(BScenarioWork *work) {
    int pages = work->scriptLines / SCRIPT_PAGE_LINES;

    pages += work->scriptLines % SCRIPT_PAGE_LINES;
    return pages;
}

void BScenarioMain_Start(BScenarioParam *param) {
    int i;
    BScenarioWork *work = GFL_HeapAllocate(param->heapId, sizeof(BScenarioWork), TRUE, "b_scenario_main.c", 356);

    GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BScenarioMain_Task, work, 100);
    work->param = param;
    work->paletteFade = BtlvEffect_GetPaletteFade();
    work->step = STEP_INIT;
    work->pageSeq = PAGE_SEQ_SUMMARY;
    work->nextPageSeq = -2;
    work->flags = 0;
    work->pageStep = PAGE_STEP_INIT;
    work->page = PAGE_SUMMARY;
    work->flashFlags = 0;
    work->flashStep = 0;
    work->flashButton = 0;
    work->shownWindows = 0;
    work->scenario = param->scenario;
    work->summaryCursor = 0;
    work->scriptCursor = 0;
    work->castIndex = 0;
    work->castCount = PokeParty_GetPkmCount(work->param->party);
    work->castCursor = 0;
    for (i = 0; i < 2; i++) {
        work->flashFrame[i] = 0;
    }
    for (i = 0; i < BSCENARIO_WINDOW_MAX; i++) {
        work->windows[i].window = NULL;
    }
    for (i = 0; i < BSCENARIO_RES_COUNT; i++) {
        work->palettes[i] = -1;
    }
    for (i = 0; i < BSCENARIO_RES_COUNT; i++) {
        work->cells[i] = -1;
    }
    for (i = 0; i < BSCENARIO_CHARS_COUNT; i++) {
        work->chars[i] = -1;
    }
    work->positions = NULL;
    BScenarioMain_Step(work);
}

static void BScenarioMain_Task(TCB *tcb, void *data) {
    BScenarioWork *work = data;

    if (work->param->abort == 1) {
        work->step = STEP_END;
    }
    BScenarioMain_Step(work);
    if (work->step == STEP_END) {
        if (BScenarioMain_Finish(tcb, work) == TRUE) {
            return;
        }
    }
    GFL_TCBExMgrUpdate(work->tcbManager);
    BScenarioGraphic_UpdatePrint(work);
    BScenarioMain_ApplyPage(work);
}

static BOOL BScenarioMain_Finish(TCB *tcb, BScenarioWork *work) {
    int i;

    if (!func_02021c0c(work->printQueue)) {
        if (work->param->abort == 1) {
            func_02021c44(work->printQueue);
        } else {
            return FALSE;
        }
    }
    if (!GFL_WipeIsFinished()) {
        if (work->param->abort == 1) {
            GFL_WipeForceEnd();
        } else {
            return FALSE;
        }
    }
    BScenarioGraphic_FreeActors(work);
    BScenarioMain_FreeBG(work);
    GFL_TCBExMgrFree(work->tcbManager);
    for (i = 0; i < 2; i++) {
        if (work->msgData[i] != NULL) {
            GFL_MsgDataFree(work->msgData[i]);
            work->msgData[i] = NULL;
        }
    }
    GFL_HeapFree(work->positions);
    GFL_WordSetSystemFree(work->wordSet);
    BScenarioGraphic_FreeWindows(work);
    func_02021a18(work->printQueue);
    GFL_FontFree(work->smallFont);
    work->param->done = TRUE;
    func_0203d564(!(work->flags & BSCENARIO_FLAG_KEY_CLOSE));
    GFL_TCBRemove(tcb);
    GFL_HeapFree(work);
    return TRUE;
}

static void BScenarioMain_Step(BScenarioWork *work) {
    switch (work->step) {
    case STEP_INIT:
        BScenarioMain_Init(work);
        work->step = STEP_WAIT_WIPE;
        break;
    case STEP_WAIT_WIPE:
        if (!BScenarioMain_IsWiping()) {
            work->step = STEP_PAGES;
        }
        break;
    case STEP_PAGES:
        if (!BScenarioMain_UpdatePage(work)) {
            work->step = STEP_FADE_OUT;
            BScenarioMain_StartFadeOut(work);
        }
        break;
    case STEP_FADE_OUT:
        if (!BScenarioMain_IsFading(work)) {
            work->step = STEP_END;
        }
        break;
    case STEP_END:
        break;
    }
}

static void BScenarioMain_Init(BScenarioWork *work) {
    G2S_BlendNone();
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    work->tcbManager = GFL_TCBExMgrCreate(work->param->heapId, work->param->heapId, 1, 4);
    work->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, work->param->heapId);
    work->printQueue = func_020219a8(0x800, work->param->heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(work->param->heapId);
    work->positions = GFL_ArcSysReadHeapNew(ARCID_BATTLE_SCENARIO, FILE_POSITIONS, work->param->heapId);
    work->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEWOOD_LINES, work->param->heapId);
    // The films' scripts are the 41 message files from 314
    work->msgData[1] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0314 + work->scenario, work->param->heapId);
    BScenarioMain_InitBG(work);
    BScenarioMain_LoadBG(work);
    BScenarioMain_CountScriptLines(work);
    BScenarioMain_InitScriptPage(work);
    BScenarioGraphic_CreateWindows(work, PAGE_SUMMARY);
    BScenarioMain_ReadCast(work);
    BScenarioMain_PrintSummary(work, FALSE);
    BScenarioGraphic_InitActors(work);
    BScenarioMain_StartWipe(work);
    BScenarioMain_InitPalettes(work);
    work->flags |= BSCENARIO_FLAG_REFRESH;
    if (!func_0203d554()) {
        work->flags |= BSCENARIO_FLAG_KEYS;
    }
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, 0x3f, 8, 3);
}

static void BScenarioMain_StartWipe(BScenarioWork *work) {
    GFL_WipeSet(4, 1, 1, 0, 6, 1, work->param->heapId);
}

static BOOL BScenarioMain_IsWiping(void) {
    BOOL wiping = TRUE;

    if (GFL_WipeIsFinished() == TRUE) {
        wiping = FALSE;
    }
    return wiping;
}

static void BScenarioMain_StartFadeOut(BScenarioWork *work) {
    PaletteFade_StartFade(work->paletteFade, 10, 0xffff, 0, 0, 16, 0, BtlvEffect_GetTCBManager());
}

static BOOL BScenarioMain_IsFading(BScenarioWork *work) {
    BOOL fading = TRUE;

    if (PaletteFade_GetActiveMask(work->paletteFade) == 0) {
        fading = FALSE;
    }
    return fading;
}

static void BScenarioMain_InitBG(BScenarioWork *work) {
    BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

    GFL_BGSysSetLCDConfigForEngine(&lcdConfig, BGSYS_ENGINE_SUB);
    BGSetup bgPages = { 0,
                        0,
                        0x2000,
                        0,
                        BGRES_512x512,
                        GX_BG_COLORMODE_16,
                        GX_BG_SCRBASE(0xc800),
                        GX_BG_CHARBASE(0x0000),
                        0x8000,
                        GX_BG_EXTPLTT_23,
                        3,
                        GX_BG_AREAOVER_XLU,
                        FALSE };
    GFL_BGSysCreateBG(BG_PAGES, &bgPages, BGMODE_TEXT);
    BGSetup bgFlash = { 0,
                        0,
                        0x800,
                        0,
                        BGRES_256x256,
                        GX_BG_COLORMODE_16,
                        GX_BG_SCRBASE(0xe800),
                        GX_BG_CHARBASE(0x18000),
                        0x8000,
                        GX_BG_EXTPLTT_23,
                        2,
                        GX_BG_AREAOVER_XLU,
                        FALSE };
    GFL_BGSysCreateBG(BG_FLASH, &bgFlash, BGMODE_TEXT);
    BGSetup bgButtons = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x10000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
    GFL_BGSysCreateBG(BG_BUTTONS, &bgButtons, BGMODE_TEXT);
    GFL_BGSysClearScr(BG_BUTTONS);
    BGSetup bgFrame = { 0,
                        0,
                        0x800,
                        0,
                        BGRES_256x256,
                        GX_BG_COLORMODE_16,
                        GX_BG_SCRBASE(0xf000),
                        GX_BG_CHARBASE(0x8000),
                        0x4000,
                        GX_BG_EXTPLTT_01,
                        0,
                        GX_BG_AREAOVER_XLU,
                        FALSE };
    GFL_BGSysCreateBG(BG_FRAME, &bgFrame, BGMODE_TEXT);
    GFL_BGSysClearScr(BG_FRAME);
    GFL_BGSysClearCharCore(BG_BUTTONS, 0x20, 0, work->param->heapId);
    GFL_BGSysClearCharCore(BG_FRAME, 0x20, 0, work->param->heapId);
    GFL_BGSysQueueScrLoad(BG_BUTTONS);
    GFL_BGSysQueueScrLoad(BG_FRAME);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_ALL, TRUE);
}

static void BScenarioMain_FreeBG(BScenarioWork *work) {
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_ALL, FALSE);
    GFL_BGSysReleaseBG(BG_FRAME);
    GFL_BGSysReleaseBG(BG_BUTTONS);
    GFL_BGSysReleaseBG(BG_FLASH);
    GFL_BGSysReleaseBG(BG_PAGES);
}

static void BScenarioMain_InitPalettes(BScenarioWork *work) {
    u32 series = sBScenarioSeries[work->param->scenario];

    PaletteFade_LoadFromVRAM(work->paletteFade, 1, 0, 0x100);
    PaletteFade_LoadFromVRAM(work->paletteFade, 1, 0xa0, 0x60);
    PaletteFade_LoadNCLR(work->paletteFade, ARCID_FONT, 6, work->param->heapId, 1, 0x20, 0x80);
    PaletteFade_LoadNCLR(work->paletteFade, ARCID_BATTLE_SCENARIO, FILE_LABEL_PALETTE, work->param->heapId, 1, 0x20,
                         0x90);
    PaletteFade_LoadNCLR(work->paletteFade, ARCID_BATTLE_SCENARIO, sBScenarioSeriesPalettes[series % 4],
                         work->param->heapId, 1, 0x60, 0xa0);
    func_02042ba8(FALSE, work->param->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, 0, 0x1e0);
    PaletteFade_StartFade(work->paletteFade, 10, 0xffff, 0, 16, 0, 0, BtlvEffect_GetTCBManager());
}

// Sets the BG and the actors of a page that was entered
static void BScenarioMain_ApplyPage(BScenarioWork *work) {
    int cursor;

    if (!(work->flags & BSCENARIO_FLAG_PAGE)) {
        return;
    }
    BScenarioGraphic_ScrollBG(work, work->page);
    BScenarioGraphic_SetButtonsVisible(work, work->page, TRUE);
    BScenarioGraphic_SetPlateVisible(work, work->page);
    if (work->page != PAGE_CAST) {
        BScenarioMain_HideCastActors(work);
    }
    cursor = work->summaryCursor;
    if (work->page == PAGE_SCRIPT) {
        cursor = work->scriptCursor;
    } else if (work->page == PAGE_CAST) {
        cursor = work->castCursor;
    }
    BScenarioMain_UpdateCursor(work, work->page, cursor);
    work->flags &= ~BSCENARIO_FLAG_PAGE;
}

static void BScenarioMain_SummaryInit(BScenarioWork *work) {
    work->pageSeq = PAGE_SEQ_SUMMARY + 1;
    work->pageStep = PAGE_STEP_INIT;
    work->page = PAGE_SUMMARY;
    if (work->flags & BSCENARIO_FLAG_REENTER) {
        BScenarioGraphic_CreateWindows(work, PAGE_SUMMARY);
        BScenarioMain_PrintSummary(work, FALSE);
        work->flags |= BSCENARIO_FLAG_REFRESH;
    }
    work->flags |= BSCENARIO_FLAG_REENTER;
}

static void BScenarioMain_SummaryUpdate(BScenarioWork *work) {
    switch (work->pageStep) {
    case PAGE_STEP_INIT:
        work->pageStep = PAGE_STEP_INPUT;
        // fallthrough
    case PAGE_STEP_INPUT:
        BScenarioMain_SummaryInput(work);
        break;
    case PAGE_STEP_BACK:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_BACK)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_END;
        }
        break;
    case PAGE_STEP_FLASH:
        BScenarioGraphic_UpdateFlash(work);
        if (!BScenarioGraphic_IsFlashing(work)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_SCRIPT;
        }
        break;
    case PAGE_STEP_FLASH_CAST:
        BScenarioGraphic_UpdateFlash(work);
        if (!BScenarioGraphic_IsFlashing(work)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_CAST;
        }
        break;
    }
}

static void BScenarioMain_SummaryExit(BScenarioWork *work) {
    BScenarioMain_NextPage(work);
}

// The summary's choices are the script, the cast and back
static void BScenarioMain_SummaryInput(BScenarioWork *work) {
    int input = INPUT_NONE;
    BOOL touched = FALSE;
    BOOL refresh = FALSE;

    if (!func_02021c0c(work->printQueue)) {
        return;
    }
    if (work->flags & BSCENARIO_FLAG_REFRESH) {
        refresh = TRUE;
        work->flags &= ~BSCENARIO_FLAG_REFRESH;
    }
    if (!refresh) {
        s32 touch = BScenarioGraphic_GetTouched(work, PAGE_SUMMARY);

        if (touch == 1) {
            input = INPUT_TOUCH;
            touched = TRUE;
        } else if (touch == 0) {
            input = INPUT_TOUCH + 1;
            touched = TRUE;
        } else if (touch == 2) {
            input = INPUT_BACK;
            touched = TRUE;
        }
        if (input == INPUT_NONE) {
            if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
                input = INPUT_LEFT;
            } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
                input = INPUT_RIGHT;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                input = INPUT_DECIDE;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                input = INPUT_BACK;
            }
        }
    }
    if (input != INPUT_NONE) {
        if (!touched) {
            if (work->flags & BSCENARIO_FLAG_KEYS) {
                if (input == INPUT_LEFT) {
                    if (work->summaryCursor != 0) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->summaryCursor--;
                    if (work->summaryCursor < 0) {
                        work->summaryCursor = 0;
                    }
                    refresh = TRUE;
                } else if (input == INPUT_RIGHT) {
                    if (work->summaryCursor != 2) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->summaryCursor++;
                    if (work->summaryCursor > 2) {
                        work->summaryCursor = 2;
                    }
                    refresh = TRUE;
                } else if (input == INPUT_DECIDE) {
                    if (work->summaryCursor == 0) {
                        BScenarioGraphic_StartFlash(work, 0);
                        work->pageStep = PAGE_STEP_FLASH;
                        work->scriptCursor = 0;
                        GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    } else if (work->summaryCursor == 1) {
                        BScenarioGraphic_StartFlash(work, 1);
                        work->pageStep = PAGE_STEP_FLASH_CAST;
                        // The touch screen's way clears the cast page's cursor instead
                        work->scriptCursor = 0;
                        GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    } else {
                        BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                        work->pageStep = PAGE_STEP_BACK;
                        work->flags |= BSCENARIO_FLAG_KEY_CLOSE;
                        GFL_SndSEPlay(SEQ_SE_CANCEL2);
                    }
                } else if (input == INPUT_BACK) {
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                    work->pageStep = PAGE_STEP_BACK;
                    work->flags |= BSCENARIO_FLAG_KEY_CLOSE;
                    GFL_SndSEPlay(SEQ_SE_CANCEL2);
                }
            } else {
                work->flags |= BSCENARIO_FLAG_KEYS;
                work->summaryCursor = 0;
                refresh = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else {
            work->flags &= ~BSCENARIO_FLAG_KEYS;
            refresh = TRUE;
            if (input == INPUT_BACK) {
                BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                work->pageStep = PAGE_STEP_BACK;
                GFL_SndSEPlay(SEQ_SE_CANCEL2);
            } else if (input == INPUT_TOUCH) {
                BScenarioGraphic_StartFlash(work, 1);
                work->pageStep = PAGE_STEP_FLASH_CAST;
                work->castCursor = 0;
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            } else if (input == INPUT_TOUCH + 1) {
                BScenarioGraphic_StartFlash(work, 0);
                work->pageStep = PAGE_STEP_FLASH;
                work->scriptCursor = 0;
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            }
        }
    }
    if (refresh == TRUE) {
        BScenarioMain_UpdateCursor(work, PAGE_SUMMARY, work->summaryCursor);
    }
}

static void BScenarioMain_PrintSummary(BScenarioWork *work, BOOL update) {
    int scenario = work->scenario;
    int i;

    if (update) {
        return;
    }
    BScenarioMain_PrintLabels(work, PAGE_SUMMARY);
    for (i = 0; i < 3; i++) {
        u32 align = 0;
        int x = 0;
        StrBuf *str;

        if (i == 0) {
            align = ALIGN_CENTER;
            x = TEXT_X_CENTER;
        }
        str = GFL_MsgDataLoadStrbufNew(work->msgData[0], scenario + sBScenarioSummaryMessages[i]);
        BScenarioGraphic_Print(work, i + 3, x, 0, str, align, TEXT_COLOR, FALSE, FALSE);
        GFL_StrBufFree(str);
    }
    work->flags |= BSCENARIO_FLAG_PAGE;
}

static void BScenarioMain_ScriptInit(BScenarioWork *work) {
    work->pageSeq = PAGE_SEQ_SCRIPT + 1;
    work->pageStep = PAGE_STEP_INIT;
    work->page = PAGE_SCRIPT;
    BScenarioGraphic_SetMarker(work, 0, 0, FALSE);
    BScenarioGraphic_CreateWindows(work, PAGE_SCRIPT);
    BScenarioMain_PrintScript(work, FALSE);
    BScenarioMain_UpdateArrows(work, PAGE_SCRIPT, TRUE);
    work->flags |= BSCENARIO_FLAG_REFRESH;
}

static void BScenarioMain_ScriptUpdate(BScenarioWork *work) {
    switch (work->pageStep) {
    case PAGE_STEP_INIT:
        work->pageStep = PAGE_STEP_INPUT;
        // fallthrough
    case PAGE_STEP_INPUT:
        BScenarioMain_ScriptInput(work);
        break;
    case PAGE_STEP_BACK:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_BACK)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_SUMMARY;
            BScenarioGraphic_SetMarker(work, 0, 0, FALSE);
        }
        break;
    case PAGE_STEP_PREV:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_PREV)) {
            work->pageStep = PAGE_STEP_INPUT;
            if (work->flags & BSCENARIO_FLAG_PREV_END) {
                BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, 5);
            }
        }
        break;
    case PAGE_STEP_NEXT:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_NEXT)) {
            work->pageStep = PAGE_STEP_INPUT;
            if (work->flags & BSCENARIO_FLAG_NEXT_END) {
                BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, 8);
            }
        }
        break;
    case PAGE_STEP_FLASH:
        BScenarioGraphic_UpdateFlash(work);
        if (!BScenarioGraphic_IsFlashing(work)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_CAST;
            BScenarioGraphic_SetMarker(work, 0, 0, FALSE);
        }
        break;
    case PAGE_STEP_IDLE:
        break;
    }
}

static void BScenarioMain_ScriptExit(BScenarioWork *work) {
    BScenarioMain_NextPage(work);
}

// The script's choices are the cast, the previous and the next page, and back
static void BScenarioMain_ScriptInput(BScenarioWork *work) {
    BOOL refresh = FALSE;
    int input = INPUT_NONE;
    BOOL touched = FALSE;
    BOOL redraw = FALSE;

    if (!func_02021c0c(work->printQueue)) {
        return;
    }
    if (work->flags & BSCENARIO_FLAG_REFRESH) {
        refresh = TRUE;
        work->flags &= ~BSCENARIO_FLAG_REFRESH;
    }
    if (!refresh) {
        s32 touch = BScenarioGraphic_GetTouched(work, PAGE_SCRIPT);

        if (touch == 0) {
            input = INPUT_TOUCH;
            touched = TRUE;
        } else if (touch == 1) {
            input = INPUT_TOUCH + 1;
            touched = TRUE;
        } else if (touch == 2) {
            input = INPUT_TOUCH + 2;
            touched = TRUE;
        } else if (touch == 3) {
            input = INPUT_BACK;
            touched = TRUE;
        }
        if (input == INPUT_NONE) {
            if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
                input = INPUT_LEFT;
            } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
                input = INPUT_RIGHT;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                input = INPUT_DECIDE;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                input = INPUT_BACK;
            }
        }
    }
    if (input != INPUT_NONE) {
        if (!touched) {
            if (work->flags & BSCENARIO_FLAG_KEYS) {
                if (input == INPUT_LEFT) {
                    if (work->scriptCursor != 0) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->scriptCursor--;
                    if (work->scriptCursor < 0) {
                        work->scriptCursor = 0;
                    }
                    refresh = TRUE;
                } else if (input == INPUT_RIGHT) {
                    if (work->scriptCursor != 3) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->scriptCursor++;
                    if (work->scriptCursor > 3) {
                        work->scriptCursor = 3;
                    }
                    refresh = TRUE;
                } else if (input == INPUT_DECIDE) {
                    if (work->scriptCursor == 0) {
                        BScenarioGraphic_StartFlash(work, 2);
                        work->pageStep = PAGE_STEP_FLASH;
                        GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    } else if (work->scriptCursor == 1) {
                        if (work->scriptPage != 0) {
                            GFL_SndSEPlay(SEQ_SE_DECIDE2);
                        }
                        work->scriptPage--;
                        if (work->scriptPage < 0) {
                            work->scriptPage = 0;
                            work->flags |= BSCENARIO_FLAG_PREV_BLOCKED;
                            work->flags |= BSCENARIO_FLAG_PREV_ANIM;
                        } else {
                            redraw = TRUE;
                            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, 4);
                            work->pageStep = PAGE_STEP_PREV;
                            work->flags &= ~BSCENARIO_FLAG_NEXT_BLOCKED;
                            work->flags |= BSCENARIO_FLAG_NEXT_ANIM;
                        }
                        BScenarioMain_UpdateArrows(work, PAGE_SCRIPT, FALSE);
                    } else if (work->scriptCursor == 2) {
                        int pages = BScenarioMain_GetScriptPageCount(work);

                        if (work->scriptPage != pages - 1) {
                            GFL_SndSEPlay(SEQ_SE_DECIDE2);
                        }
                        work->scriptPage++;
                        if (work->scriptPage > pages - 1) {
                            work->scriptPage = pages - 1;
                            work->flags |= BSCENARIO_FLAG_NEXT_BLOCKED;
                            work->flags |= BSCENARIO_FLAG_NEXT_ANIM;
                        } else {
                            redraw = TRUE;
                            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, 7);
                            work->pageStep = PAGE_STEP_NEXT;
                            work->flags &= ~BSCENARIO_FLAG_PREV_BLOCKED;
                            work->flags |= BSCENARIO_FLAG_PREV_ANIM;
                        }
                        BScenarioMain_UpdateArrows(work, PAGE_SCRIPT, FALSE);
                    } else {
                        BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                        work->pageStep = PAGE_STEP_BACK;
                        work->scriptCursor = 0;
                        work->summaryCursor = 0;
                        GFL_SndSEPlay(SEQ_SE_CANCEL2);
                    }
                } else if (input == INPUT_BACK) {
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                    work->pageStep = PAGE_STEP_BACK;
                    work->scriptCursor = 0;
                    work->summaryCursor = 0;
                    GFL_SndSEPlay(SEQ_SE_CANCEL2);
                }
            } else {
                work->flags |= BSCENARIO_FLAG_KEYS;
                work->scriptCursor = 0;
                refresh = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else {
            if (input == INPUT_BACK) {
                BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                work->pageStep = PAGE_STEP_BACK;
                work->flags &= ~BSCENARIO_FLAG_KEYS;
                work->scriptCursor = 0;
                work->summaryCursor = 0;
                GFL_SndSEPlay(SEQ_SE_CANCEL2);
            } else if (input == INPUT_TOUCH) {
                BScenarioGraphic_StartFlash(work, 2);
                work->pageStep = PAGE_STEP_FLASH;
                work->flags &= ~BSCENARIO_FLAG_KEYS;
                work->scriptCursor = 0;
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            } else if (input == INPUT_TOUCH + 1) {
                if (work->scriptPage != 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                }
                work->scriptPage--;
                if (work->scriptPage < 0) {
                    work->scriptPage = 0;
                    work->flags |= BSCENARIO_FLAG_PREV_BLOCKED;
                    work->flags |= BSCENARIO_FLAG_PREV_ANIM;
                } else {
                    work->flags &= ~BSCENARIO_FLAG_KEYS;
                    refresh = TRUE;
                    redraw = TRUE;
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, 4);
                    work->pageStep = PAGE_STEP_PREV;
                    work->flags &= ~BSCENARIO_FLAG_NEXT_BLOCKED;
                    work->flags |= BSCENARIO_FLAG_NEXT_ANIM;
                }
                BScenarioMain_UpdateArrows(work, PAGE_SCRIPT, FALSE);
            } else if (input == INPUT_TOUCH + 2) {
                int pages = BScenarioMain_GetScriptPageCount(work);

                if (work->scriptPage != pages - 1) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                }
                work->scriptPage++;
                if (work->scriptPage > pages - 1) {
                    work->scriptPage = pages - 1;
                    work->flags |= BSCENARIO_FLAG_NEXT_BLOCKED;
                    work->flags |= BSCENARIO_FLAG_NEXT_ANIM;
                } else {
                    work->flags &= ~BSCENARIO_FLAG_KEYS;
                    refresh = TRUE;
                    redraw = TRUE;
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, 7);
                    work->pageStep = PAGE_STEP_NEXT;
                    work->flags &= ~BSCENARIO_FLAG_PREV_BLOCKED;
                    work->flags |= BSCENARIO_FLAG_PREV_ANIM;
                }
                BScenarioMain_UpdateArrows(work, PAGE_SCRIPT, FALSE);
            }
        }
    }
    if (refresh == TRUE) {
        BScenarioMain_UpdateCursor(work, PAGE_SCRIPT, work->scriptCursor);
    }
    if (redraw == TRUE) {
        BScenarioMain_PrintScript(work, TRUE);
    }
    BScenarioMain_UpdateArrowAnims(work);
}

// Prints the script page's two scenes with what each asks for, and the page's number
static void BScenarioMain_PrintScript(BScenarioWork *work, BOOL update) {
    StrBuf *str;
    int scene;
    int line;

    if (!update) {
        BScenarioMain_PrintLabels(work, PAGE_SCRIPT);
        str = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[CAST_MSG_SLASH]);
        BScenarioGraphic_Print(work, sBScenarioScriptWindowIndices[SCRIPT_WINDOW_SLASH], 0, 0, str, 0, TEXT_COLOR, TRUE,
                               FALSE);
        GFL_StrBufFree(str);
        work->flags |= BSCENARIO_FLAG_PAGE;
    }
    str = GFL_MsgDataLoadStrbufNew(work->msgData[1],
                                   work->scriptPage * SCRIPT_PAGE_LINES + sBScenarioScriptMessages[SCRIPT_MSG_SCENES]);
    BScenarioGraphic_Print(work, 1, 0, 0, str, 0, TEXT_COLOR, FALSE, FALSE);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(work->msgData[1],
                                   work->scriptPage * SCRIPT_PAGE_LINES + sBScenarioScriptMessages[SCRIPT_MSG_ORDERS]);
    BScenarioGraphic_Print(work, sBScenarioScriptWindowIndices[SCRIPT_WINDOW_ORDERS], 0, 0, str, 0, TEXT_COLOR, FALSE,
                           FALSE);
    GFL_StrBufFree(str);
    if (work->scriptPage * SCRIPT_PAGE_LINES + 1 < work->scriptLines) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData[1], work->scriptPage * SCRIPT_PAGE_LINES +
                                                             sBScenarioScriptMessages[SCRIPT_MSG_SCENES] + 1);
        BScenarioGraphic_Print(work, 2, 0, 0, str, 0, TEXT_COLOR, FALSE, FALSE);
        GFL_StrBufFree(str);
        str = GFL_MsgDataLoadStrbufNew(work->msgData[1], work->scriptPage * SCRIPT_PAGE_LINES +
                                                             sBScenarioScriptMessages[SCRIPT_MSG_ORDERS] + 1);
        BScenarioGraphic_Print(work, sBScenarioScriptWindowIndices[SCRIPT_WINDOW_ORDERS] + 1, 0, 0, str, 0, TEXT_COLOR,
                               FALSE, FALSE);
        GFL_StrBufFree(str);
    } else {
        BScenarioGraphic_ClearWindow(work, 2);
        BScenarioGraphic_ClearWindow(work, sBScenarioScriptWindowIndices[SCRIPT_WINDOW_ORDERS] + 1);
    }
    scene = work->param->scene - 1;
    line = scene % SCRIPT_PAGE_LINES;
    if (work->scriptPage == scene / SCRIPT_PAGE_LINES) {
        BScenarioGraphic_SetMarker(work, line, 0, TRUE);
    } else {
        BScenarioGraphic_SetMarker(work, 0, 0, FALSE);
    }
    BScenarioGraphic_PrintNumber(work, 6, 0, 0, work->scriptPage + 1, 2, 0, TRUE);
    BScenarioGraphic_PrintNumber(work, 7, 0, 0, BScenarioMain_GetScriptPageCount(work), 2, 0, TRUE);
}

static void BScenarioMain_CastInit(BScenarioWork *work) {
    work->pageSeq = PAGE_SEQ_CAST + 1;
    work->pageStep = PAGE_STEP_INIT;
    work->page = PAGE_CAST;
    BScenarioGraphic_CreateWindows(work, PAGE_CAST);
    BScenarioMain_PrintCast(work, FALSE);
    BScenarioMain_UpdateCastActors(work, FALSE);
    BScenarioMain_UpdateArrows(work, PAGE_CAST, TRUE);
    work->flags |= BSCENARIO_FLAG_REFRESH;
}

static void BScenarioMain_CastUpdate(BScenarioWork *work) {
    switch (work->pageStep) {
    case PAGE_STEP_INIT:
        work->pageStep = PAGE_STEP_INPUT;
        // fallthrough
    case PAGE_STEP_INPUT:
        BScenarioMain_CastInput(work);
        break;
    case PAGE_STEP_BACK:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_BACK)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_SUMMARY;
        }
        break;
    case PAGE_STEP_PREV:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_PREV)) {
            work->pageStep = PAGE_STEP_INPUT;
        }
        break;
    case PAGE_STEP_NEXT:
        if (!BScenarioGraphic_IsButtonAnimating(work, BSCENARIO_BUTTON_NEXT)) {
            work->pageStep = PAGE_STEP_INPUT;
        }
        break;
    case PAGE_STEP_FLASH:
        BScenarioGraphic_UpdateFlash(work);
        if (!BScenarioGraphic_IsFlashing(work)) {
            work->pageSeq++;
            work->nextPageSeq = PAGE_SEQ_SCRIPT;
        }
        break;
    case PAGE_STEP_IDLE:
        break;
    }
}

static void BScenarioMain_CastExit(BScenarioWork *work) {
    BScenarioMain_NextPage(work);
}

// The cast's choices are the script, the previous and the next Pokémon, and back
static void BScenarioMain_CastInput(BScenarioWork *work) {
    BOOL redraw = FALSE;
    int input = INPUT_NONE;
    BOOL touched = FALSE;
    BOOL refresh = FALSE;

    if (!func_02021c0c(work->printQueue)) {
        return;
    }
    if (work->flags & BSCENARIO_FLAG_REFRESH) {
        refresh = TRUE;
        work->flags &= ~BSCENARIO_FLAG_REFRESH;
    }
    if (!refresh) {
        s32 touch = BScenarioGraphic_GetTouched(work, PAGE_CAST);

        if (touch == 0) {
            input = INPUT_TOUCH;
            touched = TRUE;
        } else if (touch == 1) {
            input = INPUT_TOUCH + 1;
            touched = TRUE;
        } else if (touch == 2) {
            input = INPUT_TOUCH + 2;
            touched = TRUE;
        } else if (touch == 3) {
            input = INPUT_BACK;
            touched = TRUE;
        }
        if (input == INPUT_NONE) {
            if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
                input = INPUT_LEFT;
            } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
                input = INPUT_RIGHT;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                input = INPUT_DECIDE;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                input = INPUT_BACK;
            }
        }
    }
    if (input != INPUT_NONE) {
        if (!touched) {
            if (work->flags & BSCENARIO_FLAG_KEYS) {
                if (input == INPUT_LEFT) {
                    if (work->castCursor != 0) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->castCursor--;
                    if (work->castCursor < 0) {
                        work->castCursor = 0;
                    }
                    refresh = TRUE;
                } else if (input == INPUT_RIGHT) {
                    if (work->castCursor != 3) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->castCursor++;
                    if (work->castCursor > 3) {
                        work->castCursor = 3;
                    }
                    refresh = TRUE;
                } else if (input == INPUT_DECIDE) {
                    if (work->castCursor == 0) {
                        BScenarioGraphic_StartFlash(work, 3);
                        work->pageStep = PAGE_STEP_FLASH;
                        GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    } else if (work->castCursor == 1) {
                        if (!(work->flags & BSCENARIO_FLAG_PREV_END)) {
                            GFL_SndSEPlay(SEQ_SE_DECIDE2);
                            work->castIndex--;
                            if (work->castIndex < 0) {
                                work->castIndex = work->castCount - 1;
                            }
                            redraw = TRUE;
                            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, 4);
                            work->pageStep = PAGE_STEP_PREV;
                        }
                    } else if (work->castCursor == 2) {
                        if (!(work->flags & BSCENARIO_FLAG_NEXT_END)) {
                            GFL_SndSEPlay(SEQ_SE_DECIDE2);
                            work->castIndex++;
                            if (work->castIndex >= work->castCount) {
                                work->castIndex = 0;
                            }
                            redraw = TRUE;
                            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, 7);
                            work->pageStep = PAGE_STEP_NEXT;
                        }
                    } else {
                        BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                        work->pageStep = PAGE_STEP_BACK;
                        work->castCursor = 0;
                        work->summaryCursor = 0;
                        GFL_SndSEPlay(SEQ_SE_CANCEL2);
                    }
                } else if (input == INPUT_BACK) {
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                    work->pageStep = PAGE_STEP_BACK;
                    work->castCursor = 0;
                    work->summaryCursor = 0;
                    GFL_SndSEPlay(SEQ_SE_CANCEL2);
                }
            } else {
                work->flags |= BSCENARIO_FLAG_KEYS;
                work->castCursor = 0;
                refresh = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else {
            if (input == INPUT_BACK) {
                BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_BACK, 1);
                work->pageStep = PAGE_STEP_BACK;
                work->flags &= ~BSCENARIO_FLAG_KEYS;
                work->castCursor = 0;
                work->summaryCursor = 0;
                GFL_SndSEPlay(SEQ_SE_CANCEL2);
            } else if (input == INPUT_TOUCH) {
                BScenarioGraphic_StartFlash(work, 3);
                work->pageStep = PAGE_STEP_FLASH;
                work->flags &= ~BSCENARIO_FLAG_KEYS;
                work->castCursor = 0;
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            } else if (input == INPUT_TOUCH + 1) {
                if (!(work->flags & BSCENARIO_FLAG_PREV_END)) {
                    work->castIndex--;
                    if (work->castIndex < 0) {
                        work->castIndex = work->castCount - 1;
                    }
                    redraw = TRUE;
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    work->flags &= ~BSCENARIO_FLAG_KEYS;
                    refresh = TRUE;
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, 4);
                    work->pageStep = PAGE_STEP_PREV;
                }
            } else if (input == INPUT_TOUCH + 2) {
                if (!(work->flags & BSCENARIO_FLAG_NEXT_END)) {
                    work->castIndex++;
                    if (work->castIndex >= work->castCount) {
                        work->castIndex = 0;
                    }
                    redraw = TRUE;
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    work->flags &= ~BSCENARIO_FLAG_KEYS;
                    refresh = TRUE;
                    BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, 7);
                    work->pageStep = PAGE_STEP_NEXT;
                }
            }
        }
    }
    if (refresh == TRUE) {
        BScenarioMain_UpdateCursor(work, PAGE_CAST, work->castCursor);
    }
    if (redraw == TRUE) {
        BScenarioMain_PrintCast(work, TRUE);
        BScenarioMain_UpdateCastActors(work, TRUE);
    }
}

// Prints the shown Pokémon's name, sex, level, item, moves and Ability
static void BScenarioMain_PrintCast(BScenarioWork *work, BOOL update) {
    StrBuf *buffer;
    int sexMsg;
    int i;

    if (!update) {
        BScenarioMain_PrintLabels(work, PAGE_CAST);
        work->flags |= BSCENARIO_FLAG_PAGE;
    }
    PokeParty_GetPkm(work->param->party, work->castIndex);
    buffer = GFL_StrBufCreate(0x80, work->param->heapId);

    int species = work->mons[work->castIndex].species;
    StrBuf *speciesStr = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[CAST_MSG_SPECIES]);
    WordSet_LoadSpeciesName(work->wordSet, 0, (u16)species);
    GFL_WordSetFormatStrbuf(work->wordSet, buffer, speciesStr);
    sexMsg = CAST_MSG_MALE;
    BScenarioGraphic_Print(work, 1, 0, 0, buffer, 0, TEXT_COLOR, FALSE, FALSE);
    GFL_StrBufFree(speciesStr);

    int sex = work->mons[work->castIndex].sex;
    u32 color = TEXT_COLOR_MALE;
    if (sex == GENDER_FEMALE) {
        sexMsg = CAST_MSG_FEMALE;
        color = TEXT_COLOR_FEMALE;
    }
    if (sex != GENDER_UNKNOWN) {
        StrBuf *sexStr = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[sexMsg]);

        BScenarioGraphic_Print(work, 1, 0x3e, 0, sexStr, 0, color, FALSE, TRUE);
        GFL_StrBufFree(sexStr);
    }

    int level = work->mons[work->castIndex].level;
    StrBuf *levelStr = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[CAST_MSG_LEVEL]);
    WordSetNumber(work->wordSet, 0, level, 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, buffer, levelStr);
    BScenarioGraphic_Print(work, 2, 0, 0, buffer, 0, TEXT_COLOR, FALSE, FALSE);
    GFL_StrBufFree(levelStr);

    int item = work->mons[work->castIndex].item;
    StrBuf *itemStr = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[CAST_MSG_ITEM]);
    loadItemNameToStrbuf(work->wordSet, 0, item);
    GFL_WordSetFormatStrbuf(work->wordSet, buffer, itemStr);
    BScenarioGraphic_Print(work, 4, 0, 0, buffer, 0, TEXT_COLOR, FALSE, FALSE);
    GFL_StrBufFree(itemStr);

    for (i = 0; i < 4; i++) {
        int move = work->mons[work->castIndex].moves[i];

        if (move != 0) {
            StrBuf *moveStr = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[CAST_MSG_MOVES + i]);

            loadMoveNameToStrbuf(work->wordSet, 0, move);
            GFL_WordSetFormatStrbuf(work->wordSet, buffer, moveStr);
            BScenarioGraphic_Print(work, i + 5, 0, 0, buffer, 0, TEXT_COLOR, FALSE, FALSE);
            GFL_StrBufFree(moveStr);
        } else {
            BScenarioGraphic_ClearWindow(work, i + 5);
        }
    }

    int ability = work->mons[work->castIndex].ability;
    StrBuf *abilityStr = GFL_MsgDataLoadStrbufNew(work->msgData[0], sBScenarioCastMessages[CAST_MSG_ABILITY]);
    loadAbilityNameToStrbuf(work->wordSet, 0, ability);
    GFL_WordSetFormatStrbuf(work->wordSet, buffer, abilityStr);
    BScenarioGraphic_Print(work, 10, 0, 0, buffer, 0, TEXT_COLOR, FALSE, FALSE);
    GFL_StrBufFree(abilityStr);
    GFL_StrBufFree(buffer);
}

// Shows the icon of the shown Pokémon, and the icons of its types and of its moves' types
static void BScenarioMain_UpdateCastActors(BScenarioWork *work, BOOL update) {
    int i;
    int type1;
    BOOL visible;

    PokeParty_GetPkm(work->param->party, work->castIndex);
    for (i = 0; i < 6; i++) {
        BOOL shown;

        if (i == work->castIndex) {
            BScenarioGraphic_SetIconPalette(work, i, work->mons[i].species, work->mons[i].sex);
            shown = TRUE;
        } else {
            shown = FALSE;
        }
        BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_ICON, i, shown);
    }
    type1 = work->mons[work->castIndex].type1;
    BScenarioGraphic_SetTypeIcon(work, 0, 0, type1);
    BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_TYPE, 0, TRUE);
    visible = TRUE;
    if (type1 != work->mons[work->castIndex].type2) {
        BScenarioGraphic_SetTypeIcon(work, 1, 0, work->mons[work->castIndex].type2);
    } else {
        visible = FALSE;
    }
    BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_TYPE, 1, visible);
    for (i = 0; i < 4; i++) {
        BOOL shown;

        if (work->mons[work->castIndex].moves[i] != 0) {
            BScenarioGraphic_SetTypeIcon(work, i + 2, 1, work->mons[work->castIndex].moveTypes[i]);
            shown = TRUE;
        } else {
            shown = FALSE;
        }
        BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_TYPE, i + 2, shown);
    }
}

static void BScenarioMain_HideCastActors(BScenarioWork *work) {
    int i;

    for (i = 0; i < 6; i++) {
        BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_TYPE, i, FALSE);
    }
    for (i = 0; i < 6; i++) {
        BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_ICON, i, FALSE);
    }
}

// Runs the page's function, and returns whether the screen goes on
static BOOL BScenarioMain_UpdatePage(BScenarioWork *work) {
    BOOL active = TRUE;

    if (BScenarioMain_IsCloseRequested(work) == TRUE) {
        work->nextPageSeq = PAGE_SEQ_END;
        return FALSE;
    }
    if (work->pageSeq >= PAGE_SEQ_COUNT) {
        return FALSE;
    }
    sBScenarioPageFuncs[work->pageSeq](work);
    BScenarioMain_AfterPage(work);
    if (work->nextPageSeq == PAGE_SEQ_END) {
        active = FALSE;
    }
    return active;
}

static void BScenarioMain_NextPage(BScenarioWork *work) {
    if (work->nextPageSeq != PAGE_SEQ_END) {
        work->pageSeq = work->nextPageSeq;
    }
}

static void BScenarioMain_ReadCast(BScenarioWork *work) {
    int i;
    int j;

    for (i = 0; i < 6; i++) {
        work->mons[i].species = 0;
        work->mons[i].sex = 0;
        work->mons[i].level = 0;
        work->mons[i].item = 0;
        work->mons[i].type1 = 0;
        work->mons[i].type2 = 0;
        for (j = 0; j < 4; j++) {
            work->mons[i].moves[j] = 0;
            work->mons[i].moveTypes[j] = 0;
        }
    }
    for (i = 0; i < work->castCount; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(work->param->party, i);

        work->mons[i].species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        work->mons[i].sex = PokeParty_GetSex(pkm);
        work->mons[i].level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
        work->mons[i].item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
        work->mons[i].ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
        work->mons[i].type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
        work->mons[i].type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
        for (j = 0; j < 4; j++) {
            work->mons[i].moves[j] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + j, NULL);
            if (work->mons[i].moves[j] != 0) {
                work->mons[i].moveTypes[j] = PML_MoveGetParam(work->mons[i].moves[j], MOVE_PARAM_TYPE);
            }
        }
    }
}

static void BScenarioMain_PrintLabels(BScenarioWork *work, int page) {
    const BScenarioLabel *labels = sBScenarioLabelTables[page];
    int i;

    for (i = 0; i < BSCENARIO_WINDOW_MAX; i++) {
        u16 label = labels[i].label;
        int x;
        u16 window;
        u16 msgData;
        u16 color;
        u32 align;
        StrBuf *str;

        if (label == 0xffff) {
            break;
        }
        window = labels[i].window;
        color = TEXT_COLOR_LABEL;
        msgData = labels[i].msgData;
        if (labels[i].dark == TRUE) {
            color = TEXT_COLOR;
        }
        align = 0;
        x = 0;
        if (label == 0) {
            align = ALIGN_CENTER;
            x = TEXT_X_CENTER;
        }
        str = GFL_MsgDataLoadStrbufNew(work->msgData[msgData], sBScenarioLabelMessages[label]);
        BScenarioGraphic_Print(work, window, x, 0, str, align, color, FALSE, FALSE);
        GFL_StrBufFree(str);
    }
}

// Counts the script's scenes, which end at the first that asks for nothing
static void BScenarioMain_CountScriptLines(BScenarioWork *work) {
    StrBuf *end = GFL_MsgDataLoadStrbufNew(work->msgData[1], SCRIPT_MSG_END);
    int i;
    int count = 0;

    for (i = 0; i < SCRIPT_LINE_MAX; i++, count++) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[1], i + sBScenarioScriptMessages[SCRIPT_MSG_ORDERS]);

        if (GFL_StrBufCmp(end, str) == TRUE) {
            GFL_StrBufFree(str);
            break;
        }
        GFL_StrBufFree(str);
    }
    work->scriptLines = count;
    GFL_StrBufFree(end);
}

// Greys out the arrows that lead nowhere from the page shown; apply sets their animations at once
static void BScenarioMain_UpdateArrows(BScenarioWork *work, int page, BOOL apply) {
    if (page == PAGE_SCRIPT) {
        int pages;

        work->flags &= ~(BSCENARIO_FLAG_PREV_END | BSCENARIO_FLAG_NEXT_END);
        if (work->scriptPage <= 0) {
            work->flags |= BSCENARIO_FLAG_PREV_END;
        }
        pages = BScenarioMain_GetScriptPageCount(work);
        if (work->scriptPage >= pages - 1) {
            work->flags |= BSCENARIO_FLAG_NEXT_END;
        }
        if (apply == TRUE) {
            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, work->flags & BSCENARIO_FLAG_PREV_END ? 5 : 3);
            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, work->flags & BSCENARIO_FLAG_NEXT_END ? 8 : 6);
        }
    } else if (page == PAGE_CAST) {
        work->flags &= ~(BSCENARIO_FLAG_PREV_END | BSCENARIO_FLAG_NEXT_END);
        if (work->castCount <= 1) {
            work->flags |= BSCENARIO_FLAG_PREV_END | BSCENARIO_FLAG_NEXT_END;
        }
        if (apply == TRUE) {
            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, work->flags & BSCENARIO_FLAG_PREV_END ? 5 : 3);
            BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, work->flags & BSCENARIO_FLAG_NEXT_END ? 8 : 6);
        }
    }
}

// Sets the animations of the arrows that the script page's input asked for
static void BScenarioMain_UpdateArrowAnims(BScenarioWork *work) {
    if (work->flags & BSCENARIO_FLAG_PREV_ANIM) {
        BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_PREV, work->flags & BSCENARIO_FLAG_PREV_BLOCKED ? 5 : 3);
        work->flags &= ~BSCENARIO_FLAG_PREV_ANIM;
    }
    if (work->flags & BSCENARIO_FLAG_NEXT_ANIM) {
        BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_NEXT, work->flags & BSCENARIO_FLAG_NEXT_BLOCKED ? 8 : 6);
        work->flags &= ~BSCENARIO_FLAG_NEXT_ANIM;
    }
}

static void BScenarioMain_UpdateCursor(BScenarioWork *work, int page, int cursor) {
    BScenarioGraphic_UpdateCursor(work, work->flags & BSCENARIO_FLAG_KEYS ? TRUE : FALSE, page, cursor);
}

// Opens the script on the page of the scene being filmed
static void BScenarioMain_InitScriptPage(BScenarioWork *work) {
    work->scriptPage = (work->param->scene - 1) / SCRIPT_PAGE_LINES;
    if (work->scriptPage < 0 || work->param->scene > work->scriptLines) {
        work->scriptPage = 0;
    }
}

static BOOL BScenarioMain_IsCloseRequested(BScenarioWork *work) {
    BOOL requested = FALSE;

    if (work->param->close == 1) {
        requested = TRUE;
    }
    return requested;
}

static void BScenarioMain_AfterPage(BScenarioWork *work) {
}

static void BScenarioMain_LoadBG(BScenarioWork *work) {
    BScenarioGraphic_LoadBG(work);
}
