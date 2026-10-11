#include "app/pms_input_view.h"
#include "types.h"
#include "app/pms_input.h"
#include "app/pmsiv_category.h"
#include "app/pmsiv_edit.h"
#include "app/pmsiv_menu.h"
#include "app/pmsiv_wordwin.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "system/gf_font.h"
#include "system/pmsi_param.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The phrase input's screens: the input sends commands, each of which runs as a task until the screens' parts have
// finished moving, and the parts share the graphics and the message window set up here. The names are ours, guessed

// How many commands can run at once
#define PMSIV_COMMAND_MAX 4

// The message file of the phrase input's texts
#define PMSIV_MSG_FILE TEXT_BANK_PMS_INPUT

struct PMSInputView {
    TCB *mainTask;
    TCB *vintrTask;
    TCB *cmdTask[PMSIV_COMMAND_MAX];
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    ClActUnit *actUnit;
    // The OBJ graphics, for each screen
    PMSIVObjRes objRes[2];
    PMSIVObjRes objRes2[2];
    PMSIVEdit *edit;
    PMSIVCategory *category;
    PMSIVWordWin *wordWin;
    void *unk60;
    PMSIVMenu *menu;
    // The two colors of the edit area's text
    GXRgb colors[2];
    BmpWin *msgWin;
    BOOL msgPrinting;
    // 1 while the cursor is on the buttons
    u8 status;
    int *keyMode;
    PrintQueue *printQueue;
    Font *font;
    BOOL lowerScreenChanged;
    BOOL lowerScreenCategories;
    ArcTool *arc;
};

typedef struct {
    PMSInputView *vwk;
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    u32 cmd;
    int storePos;
    int seq;
} PMSIVCommandWork;

static void PMSIView_MainTask(TCB *tcb, void *data);
static void PMSIView_VintrTask(TCB *tcb, void *data);
static void PMSIView_DeleteCommand(PMSIVCommandWork *cwk);
static void PMSIView_CmdInit(TCB *tcb, void *data);
static void PMSIView_SetupObjGraphic(PMSIVCommandWork *cwk, ArcTool *arc);
static void PMSIView_CmdQuit(TCB *tcb, void *data);
static void PMSIView_SetupBG(PMSIVCommandWork *cwk);
static void PMSIView_CmdFadeIn(TCB *tcb, void *data);
static void PMSIView_CmdUpdateEditArea(TCB *tcb, void *data);
static void PMSIView_CmdChangeKTEditArea(TCB *tcb, void *data);
static void PMSIView_CmdChangeKTCategory(TCB *tcb, void *data);
static void PMSIView_CmdChangeKTWordWin(TCB *tcb, void *data);
static void PMSIView_CmdEditAreaToButton(TCB *tcb, void *data);
static void PMSIView_CmdButtonToEditArea(TCB *tcb, void *data);
static void PMSIView_CmdButtonToEditAreaSelect(TCB *tcb, void *data);
static void PMSIView_CmdEditAreaToCategory(TCB *tcb, void *data);
static void PMSIView_CmdCategoryToEditArea(TCB *tcb, void *data);
static void PMSIView_CmdCategoryToWordWin(TCB *tcb, void *data);
static void PMSIView_CmdWordWinToCategory(TCB *tcb, void *data);
static void PMSIView_CmdWordWinToEditArea(TCB *tcb, void *data);
static void PMSIView_CmdWordWinToButton(TCB *tcb, void *data);
static void PMSIView_CmdMoveEditAreaCursor(TCB *tcb, void *data);
static void PMSIView_CmdMoveButtonCursor(TCB *tcb, void *data);
static void PMSIView_CmdMoveCategoryCursor(TCB *tcb, void *data);
static void PMSIView_CmdMoveWordWinCursor(TCB *tcb, void *data);
static void PMSIView_CmdScrollWordWin(TCB *tcb, void *data);
static void PMSIView_CmdPushButton(TCB *tcb, void *data);
static void PMSIView_CmdNop22(TCB *tcb, void *data);
static void PMSIView_CmdUpdateEditAreaCursor(TCB *tcb, void *data);
static void PMSIView_CmdNop26(TCB *tcb, void *data);
static void PMSIView_CmdNop27(TCB *tcb, void *data);
static void PMSIView_CmdNop28(TCB *tcb, void *data);
static void PMSIView_CmdNop29(TCB *tcb, void *data);
static void PMSIView_CmdChangeCategoryMode(TCB *tcb, void *data);
static void PMSIView_CmdSetWordWinArrows(TCB *tcb, void *data);
static void PMSIView_CmdShowMenu(TCB *tcb, void *data);
static void PMSIView_CmdMoveCategory(TCB *tcb, void *data);
static void PMSIView_CmdMoveWordWin(TCB *tcb, void *data);
static void PMSIView_CmdChangeCategoryModeDisable(TCB *tcb, void *data);
static void PMSIView_CmdChangeCategoryModeEnable(TCB *tcb, void *data);
static void PMSIView_PrintMessage(PMSIVCommandWork *cwk, u32 msg);
static void PMSIView_FlushMessage(PMSInputView *vwk);

static const BGSysVRAMConfig sPMSIViewVRAMConfig = {
    GX_VRAM_BG_128_B, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,  GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_64K,
};

PMSInputView *PMSIView_Create(const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSInputView *vwk;
    int i;

    vwk = GFL_HeapAllocate(HEAPID_PMS_INPUT, sizeof(PMSInputView), FALSE, "pms_input_view.c", 223);
    if (vwk) {
        vwk->menu = NULL;
        vwk->printQueue = NULL;
        vwk->mwk = mwk;
        vwk->dwk = dwk;
        vwk->keyMode = PMSInput_GetKeyModePtr(mwk);
        ClActSys_Create(&data_02093f08, &sPMSIViewVRAMConfig, HEAPID_PMS_INPUT);
        vwk->actUnit = func_0204bf1c(128, 0, HEAPID_PMS_INPUT);
        GFL_BGSysCreate(HEAPID_PMS_INPUT);
        BmpWin_InitAllocator(HEAPID_PMS_INPUT);
        GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
        GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
        vwk->mainTask = GFL_TCBMgrAddTask(PMSInput_GetTCBManager(mwk), PMSIView_MainTask, vwk, 2);
        vwk->vintrTask = PMSIView_AddVTask(PMSIView_VintrTask, vwk, 1);
        for (i = 0; i < PMSIV_COMMAND_MAX; i++) {
            vwk->cmdTask[i] = NULL;
        }
        vwk->msgPrinting = FALSE;
        vwk->status = 0;
        vwk->lowerScreenChanged = FALSE;
        vwk->lowerScreenCategories = FALSE;
        vwk->arc = GFL_ArcSysCreateFileHandle(ARCID_PMSI, HEAPID_PMS_INPUT);
    }
    return vwk;
}

void PMSIView_Delete(PMSInputView *vwk) {
    int i;

    if (vwk) {
        GFL_ArcToolFree(vwk->arc);
        for (i = 0; i < PMSIV_COMMAND_MAX; i++) {
            if (vwk->cmdTask[i]) {
                GFL_TCBRemove(vwk->cmdTask[i]);
            }
        }
        GFL_TCBRemove(vwk->mainTask);
        GFL_TCBRemove(vwk->vintrTask);
        PMSIVMenu_Delete(vwk->menu);
        func_0204bf98(vwk->actUnit);
        for (i = 0; i < 2; i++) {
            func_0204bcd0(vwk->objRes[i].palette);
            func_0204b98c(vwk->objRes[i].chars);
            func_0204be64(vwk->objRes[i].cellAnims);
            func_0204bcd0(vwk->objRes2[i].palette);
            func_0204b98c(vwk->objRes2[i].chars);
            func_0204be64(vwk->objRes2[i].cellAnims);
        }
        func_0204b758();
        BmpWin_FreeAllocator();
        GFL_BGSysFree();
        GFL_HeapFree(vwk);
    }
}

TCB *PMSIView_AddVTask(TCBFunc func, void *wk, u32 priority) {
    return GFL_VBlankTCBAdd(func, wk, priority);
}

static void PMSIView_MainTask(TCB *tcb, void *data) {
    PMSInputView *vwk = data;

    func_0204b794();
    if (vwk->menu != NULL) {
        PMSIVMenu_Main(vwk->menu);
    }
    if (vwk->printQueue != NULL) {
        func_02021a3c(vwk->printQueue);
        PMSIView_FlushMessage(vwk);
    }
}

static void PMSIView_VintrTask(TCB *tcb, void *data) {
    PMSInputView *vwk = data;

    if (vwk->lowerScreenChanged) {
        vwk->lowerScreenChanged = FALSE;
    }
    GFL_BGSysUpdate();
    func_0204b7c8();
}

void PMSIView_SetCommand(PMSInputView *vwk, int cmd) {
    TCBFunc funcs[PMSIV_CMD_COUNT] = {
        PMSIView_CmdInit,
        PMSIView_CmdQuit,
        PMSIView_CmdFadeIn,
        PMSIView_CmdUpdateEditArea,
        PMSIView_CmdChangeKTEditArea,
        PMSIView_CmdChangeKTCategory,
        PMSIView_CmdChangeKTWordWin,
        PMSIView_CmdEditAreaToButton,
        PMSIView_CmdButtonToEditArea,
        PMSIView_CmdButtonToEditAreaSelect,
        PMSIView_CmdEditAreaToCategory,
        PMSIView_CmdCategoryToEditArea,
        PMSIView_CmdCategoryToWordWin,
        PMSIView_CmdWordWinToCategory,
        PMSIView_CmdWordWinToEditArea,
        PMSIView_CmdWordWinToButton,
        PMSIView_CmdMoveEditAreaCursor,
        PMSIView_CmdMoveButtonCursor,
        PMSIView_CmdMoveCategoryCursor,
        PMSIView_CmdMoveWordWinCursor,
        PMSIView_CmdScrollWordWin,
        PMSIView_CmdPushButton,
        PMSIView_CmdNop22,
        PMSIView_CmdUpdateEditAreaCursor,
        PMSIView_CmdChangeCategoryModeDisable,
        PMSIView_CmdChangeCategoryModeEnable,
        PMSIView_CmdNop26,
        PMSIView_CmdNop27,
        PMSIView_CmdNop28,
        PMSIView_CmdNop29,
        PMSIView_CmdChangeCategoryMode,
        PMSIView_CmdSetWordWinArrows,
        PMSIView_CmdShowMenu,
        PMSIView_CmdMoveCategory,
        PMSIView_CmdMoveWordWin,
    };

    if (cmd < PMSIV_CMD_COUNT) {
        PMSIVCommandWork *cwk =
            GFL_HeapAllocate(HEAPID_PMS_INPUT, sizeof(PMSIVCommandWork), FALSE, "pms_input_view.c", 493);
        if (cwk) {
            int i;

            cwk->seq = 0;
            cwk->cmd = cmd;
            cwk->vwk = vwk;
            cwk->mwk = vwk->mwk;
            cwk->dwk = vwk->dwk;
            for (i = 0; i < PMSIV_COMMAND_MAX; i++) {
                if (vwk->cmdTask[i] == NULL) {
                    cwk->storePos = i;
                    vwk->cmdTask[i] = GFL_TCBMgrAddTask(PMSInput_GetTCBManager(vwk->mwk), funcs[cmd], cwk, 1);
                    break;
                }
            }
        }
    }
}

BOOL PMSIView_WaitCommandAll(PMSInputView *vwk) {
    int i;

    for (i = 0; i < PMSIV_COMMAND_MAX; i++) {
        if (vwk->cmdTask[i] != NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL PMSIView_WaitCommand(PMSInputView *vwk, int cmd) {
    int i;

    for (i = 0; i < PMSIV_COMMAND_MAX; i++) {
        if (vwk->cmdTask[i] != NULL) {
            PMSIVCommandWork *cwk = GFL_TCBGetData(vwk->cmdTask[i]);
            if (cwk->cmd == cmd) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static void PMSIView_DeleteCommand(PMSIVCommandWork *cwk) {
    GFL_TCBRemove(cwk->vwk->cmdTask[cwk->storePos]);
    cwk->vwk->cmdTask[cwk->storePos] = NULL;
    GFL_HeapFree(cwk);
}

static void PMSIView_CmdInit(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    ArcTool *arc;

    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);

    arc = GFL_ArcSysCreateFileHandle(ARCID_PMSI, HEAPID_PMS_INPUT);
    PMSIView_SetupBG(cwk);
    PMSIView_SetupObjGraphic(cwk, arc);

    cwk->vwk->edit = PMSIVEdit_Create(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIVEdit_SetupGraphicDatas(cwk->vwk->edit, arc);
    cwk->vwk->category = PMSIVCategory_Create(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIVCategory_SetupGraphicDatas(cwk->vwk->category, arc);
    cwk->vwk->wordWin = PMSIVWordWin_Create(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIVWordWin_SetupGraphicDatas(cwk->vwk->wordWin);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 30, 5, 0, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_BGSysLoadArcNCGRStatic(arc, 18, 5, 0, 0, FALSE, HEAPID_PMS_INPUT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 6, 0, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_BGSysLoadArcNCGRStatic(arc, 18, 6, 0, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_G2DIOLoadArcNCLR(arc, 4, 4, 0x80, 0xa0, 0x20, HEAPID_PMS_INPUT);

    cwk->vwk->colors[0] = GX_RGB(11, 10, 10);
    NNS_GfdRegisterNewVramTransferTask(31, 0xbc, &cwk->vwk->colors[0], sizeof(GXRgb));
    cwk->vwk->colors[1] = GX_RGB(20, 20, 21);
    NNS_GfdRegisterNewVramTransferTask(31, 0xbe, &cwk->vwk->colors[1], sizeof(GXRgb));

    GFL_BGSysLoadArcNCGRStatic(arc, 18, 7, 0, 0, FALSE, HEAPID_PMS_INPUT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 24, 7, 0, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_BGSysSetScrPaletteNo(7, 0, 0, 32, 32, 5);
    cwk->vwk->msgWin = BmpWin_CreateDynamic(7, 3, 5, 26, 7, 5, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(cwk->vwk->msgWin), 7);
    cwk->vwk->msgPrinting = FALSE;
    if (PMSInput_HasStartSentence(cwk->mwk) || PMSInput_GetInputMode(cwk->mwk) != PMSI_MODE_SENTENCE) {
        PMSIView_PrintMessage(cwk, 1);
    } else {
        PMSIView_PrintMessage(cwk, 0);
    }
    PMSIView_SetLowerScreen(cwk->vwk, FALSE);

    cwk->vwk->menu = PMSIVMenu_Create(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIVMenu_SetupEditButtons(cwk->vwk->menu);

    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    gfxEngineEnableA();
    GFL_BGSysLoadArcNCGRStatic(arc, 16, 3, 0, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_BGSysLoadArcNCGRStatic(arc, 17, 3, 0x60, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_ArcToolFree(arc);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_SetupObjGraphic(PMSIVCommandWork *cwk, ArcTool *arc) {
    PMSInputView *vwk = cwk->vwk;
    int i;

    for (i = 0; i < 2; i++) {
        vwk->objRes[i].palette = func_0204bba0(arc, 7, i, 0, HEAPID_PMS_INPUT);
        vwk->objRes[i].chars = func_0204b81c(arc, 19, FALSE, i, HEAPID_PMS_INPUT);
        vwk->objRes[i].cellAnims = func_0204bde0(arc, 36, 42, HEAPID_PMS_INPUT);
        vwk->objRes2[i].palette = func_0204bba0(arc, 1, i, 0xe0, HEAPID_PMS_INPUT);
        vwk->objRes2[i].chars = func_0204b81c(arc, 46, FALSE, i, HEAPID_PMS_INPUT);
        vwk->objRes2[i].cellAnims = func_0204bde0(arc, 34, 40, HEAPID_PMS_INPUT);
    }
}

static void PMSIView_CmdQuit(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        GFL_WipeSet(0, 0, 0, 0, 5, 1, HEAPID_PMS_INPUT);
        cwk->seq++;
        break;
    case 1:
        if (GFL_WipeIsFinished()) {
            BmpWin_Free(cwk->vwk->msgWin);
            PMSIVEdit_Delete(cwk->vwk->edit);
            PMSIVCategory_Delete(cwk->vwk->category);
            PMSIVWordWin_Delete(cwk->vwk->wordWin);
            GFL_FontFree(vwk->font);
            func_02021c44(vwk->printQueue);
            func_02021a18(vwk->printQueue);
            vwk->printQueue = NULL;
            GFL_BGSysReleaseBG(0);
            GFL_BGSysReleaseBG(1);
            GFL_BGSysReleaseBG(2);
            GFL_BGSysReleaseBG(3);
            GFL_BGSysReleaseBG(4);
            GFL_BGSysReleaseBG(5);
            GFL_BGSysReleaseBG(6);
            GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static const BGSysLCDConfig sPMSIViewLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSetup sPMSIViewBG0Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0xd800),
    GX_BG_CHARBASE(0x00000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG1Setup = {
    0,
    0,
    0x1000,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0xe000),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_01,
    1,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG2Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0xf000),
    GX_BG_CHARBASE(0x18000),
    0x8000,
    GX_BG_EXTPLTT_01,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG3Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0xf800),
    GX_BG_CHARBASE(0x08000),
    0x5000,
    GX_BG_EXTPLTT_01,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG4Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_01,
    1,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG5Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0800),
    GX_BG_CHARBASE(0x0c000),
    0x8000,
    GX_BG_EXTPLTT_01,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG6Setup = {
    0,
    0,
    0x1000,
    0,
    BGRES_256x512,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x1000),
    GX_BG_CHARBASE(0x14000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sPMSIViewBG7Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x2000),
    GX_BG_CHARBASE(0x1c000),
    0x4000,
    GX_BG_EXTPLTT_01,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static void PMSIView_SetupBG(PMSIVCommandWork *cwk) {
    PMSInputView *vwk = cwk->vwk;
    int i;

    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GFL_BGSysSetVRAMBanks(&sPMSIViewVRAMConfig);
    GFL_BGSysSetLCDConfig(&sPMSIViewLCDConfig);
    GFL_BGSysCreateBG(0, &sPMSIViewBG0Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(1, &sPMSIViewBG1Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(2, &sPMSIViewBG2Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(3, &sPMSIViewBG3Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(4, &sPMSIViewBG4Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(5, &sPMSIViewBG5Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(6, &sPMSIViewBG6Setup, BGMODE_TEXT);
    GFL_BGSysCreateBG(7, &sPMSIViewBG7Setup, BGMODE_TEXT);
    for (i = 0; i <= 7; i++) {
        GFL_BGSysClearScr(i);
        GFL_BGSysSetBGEnabled(i, TRUE);
    }
    vwk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_PMS_INPUT);
    vwk->printQueue = func_020219a8(0x500, HEAPID_PMS_INPUT);
}

static void PMSIView_CmdFadeIn(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;

    switch (cwk->seq) {
    case 0:
        GFL_WipeSet(0, 1, 1, 0, 5, 1, HEAPID_PMS_INPUT);
        cwk->seq++;
        break;
    case 1:
        if (GFL_WipeIsFinished()) {
            cwk->seq++;
        }
        break;
    default:
        PMSIView_DeleteCommand(cwk);
        break;
    }
}

static void PMSIView_CmdUpdateEditArea(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    vwk->status = 0;
    PMSIVEdit_UpdateEditArea(vwk->edit);
    PMSIVEdit_MoveCursor(vwk->edit, PMSInput_GetEditAreaCursorPos(cwk->mwk));
    PMSIVMenu_UpdateSentenceType(vwk->menu);
    PMSIVMenu_UpdateEditButtons(vwk->menu);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdChangeKTEditArea(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;

    PMSIView_ChangeKTEditArea(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdChangeKTCategory(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;

    PMSIView_ChangeKTCategory(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdChangeKTWordWin(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;

    PMSIView_ChangeKTWordWin(cwk->vwk, cwk->mwk, cwk->dwk);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdEditAreaToButton(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    vwk->status = 1;
    PMSIVEdit_VisibleCursor(vwk->edit, FALSE);
    PMSIVEdit_StopArrow(vwk->edit);
    PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), TRUE);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdButtonToEditArea(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    vwk->status = 1;
    PMSIVEdit_VisibleCursor(vwk->edit, FALSE);
    PMSIVEdit_StopArrow(vwk->edit);
    PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), FALSE);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdButtonToEditAreaSelect(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    vwk->status = 0;
    PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), FALSE);
    PMSIVEdit_ActiveArrow(vwk->edit);
    PMSIVEdit_VisibleCursor(vwk->edit, TRUE);
    PMSIVEdit_MoveCursor(vwk->edit, PMSInput_GetEditAreaCursorPos(cwk->mwk));
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdEditAreaToCategory(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    BOOL flag1, flag2;

    switch (cwk->seq) {
    case 0:
        PMSIVEdit_VisibleCursor(vwk->edit, TRUE);
        PMSIVEdit_MoveCursor(vwk->edit, PMSInput_GetEditAreaCursorPos(cwk->mwk));
        cwk->seq++;
        break;
    case 1:
        PMSInput_ResetSearch(cwk->mwk);
        PMSIVMenu_SetupCategoryButtons(vwk->menu);
        PMSIVEdit_StopCursor(vwk->edit);
        PMSIVEdit_StopArrow(vwk->edit);
        PMSIVCategory_StartEnableBG(vwk->category);
        PMSIVEdit_ScrollSet(vwk->edit, FALSE);
        if (PMSInput_GetCategoryMode(cwk->mwk) == 1) {
            PMSIVCategory_PrintSearchInput(vwk->category);
        }
        if (PMSInput_GetCategoryMode(cwk->mwk) == 0) {
            PMSIView_PrintMessage(cwk, 2);
        } else {
            PMSIView_PrintMessage(cwk, 4);
        }
        cwk->seq++;
        break;
    case 2:
        flag1 = PMSIVCategory_WaitEnableBG(vwk->category);
        flag2 = PMSIVEdit_ScrollWait(vwk->edit);
        if (flag1 && flag2) {
            PMSIVCategory_MoveCursor(vwk->category, PMSInput_GetCategoryCursorPos(vwk->mwk));
            PMSIVCategory_VisibleCursor(vwk->category, TRUE);
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdChangeCategoryModeDisable(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        cwk->seq++;
        // fallthrough
    case 1:
        PMSIVMenu_SetupCategoryButtons(vwk->menu);
        PMSIVCategory_ChangeModeScreen(vwk->category);
        PMSIVCategory_ChangeModeBG(vwk->category);
        if (PMSInput_GetCategoryMode(cwk->mwk) == 1) {
            PMSIVCategory_PrintSearchInput(vwk->category);
        }
        cwk->seq++;
        break;
    case 2:
        if (PMSIVCategory_WaitModeChange(vwk->category)) {
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdChangeCategoryModeEnable(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    BOOL flag1, flag2;

    switch (cwk->seq) {
    case 0:
        PMSIVMenu_StartButton(vwk->menu, 1);
        PMSIVCategory_StartResultList(vwk->category, FALSE);
        cwk->seq++;
        break;
    case 1:
        flag1 = PMSIVCategory_WaitResultList(vwk->category, FALSE);
        flag2 = PMSIVMenu_WaitButton(vwk->menu, 1);
        if (flag1 && flag2) {
            PMSIVMenu_EndButton(vwk->menu, 1);
            PMSInput_ResetSearch(cwk->mwk);
            PMSIVMenu_SetupCategoryButtons(vwk->menu);
            PMSIVCategory_VisibleCursor(vwk->category, FALSE);
            PMSIVCategory_ChangeModeScreen(vwk->category);
            PMSIVCategory_ChangeModeBG(vwk->category);
            if (PMSInput_GetCategoryMode(cwk->mwk) == 1) {
                PMSIVCategory_PrintSearchInput(vwk->category);
            }
            if (PMSInput_GetCategoryMode(cwk->mwk) == 0) {
                PMSIView_PrintMessage(cwk, 2);
            } else {
                PMSIView_PrintMessage(cwk, 4);
            }
            cwk->seq++;
        }
        break;
    case 2:
        if (PMSIVCategory_WaitModeChange(vwk->category)) {
            cwk->seq++;
        }
        break;
    case 3:
        PMSIVCategory_MoveCursor(vwk->category, PMSInput_GetCategoryCursorPos(vwk->mwk));
        PMSIVCategory_VisibleCursor(vwk->category, TRUE);
        PMSIView_DeleteCommand(cwk);
        break;
    }
}

static void PMSIView_CmdCategoryToEditArea(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    BOOL flag1, flag2;

    switch (cwk->seq) {
    case 0:
        if (PMSInput_GetCategoryMode(cwk->mwk) == 1) {
            PMSIVMenu_StartButton(vwk->menu, 0);
            PMSIVCategory_StartResultList(vwk->category, FALSE);
        } else {
            PMSIVMenu_StartReturn(vwk->menu);
        }
        cwk->seq++;
        break;
    case 1:
        if (PMSInput_GetCategoryMode(cwk->mwk) == 1) {
            flag1 = PMSIVCategory_WaitResultList(vwk->category, FALSE);
            flag2 = PMSIVMenu_WaitButton(vwk->menu, 0);
        } else {
            flag1 = PMSIVMenu_WaitReturn(vwk->menu);
            flag2 = TRUE;
        }
        if (flag1 && flag2) {
            cwk->seq++;
        }
        break;
    case 2:
        PMSIVMenu_SetupEditButtons(vwk->menu);
        PMSIVCategory_VisibleCursor(vwk->category, FALSE);
        PMSIVCategory_StartDisableBG(vwk->category);
        PMSIVEdit_ScrollSet(vwk->edit, TRUE);
        if (PMSInput_HasStartSentence(cwk->mwk) || PMSInput_GetInputMode(cwk->mwk) != PMSI_MODE_SENTENCE) {
            PMSIView_PrintMessage(cwk, 1);
        } else {
            PMSIView_PrintMessage(cwk, 0);
        }
        cwk->seq++;
        break;
    case 3:
        flag1 = PMSIVCategory_WaitDisableBG(vwk->category);
        flag2 = PMSIVEdit_ScrollWait(vwk->edit);
        if (flag1 && flag2) {
            PMSIVEdit_ActiveArrow(vwk->edit);
            PMSIVEdit_ActiveCursor(vwk->edit);
            vwk->status = 0;
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdCategoryToWordWin(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    BOOL flag1, flag2;

    switch (cwk->seq) {
    case 0:
        cwk->seq++;
        break;
    case 1:
        if (PMSIVCategory_WaitCursorDecide(vwk->category)) {
            PMSIVCategory_VisibleCursor(vwk->category, FALSE);
            PMSIVMenu_StartButton(vwk->menu, 3);
            cwk->seq++;
        }
        break;
    case 2:
        if (PMSIVMenu_WaitButton(vwk->menu, 3)) {
            PMSIVCategory_StartFadeOut(vwk->category);
            PMSIVCategory_StartResultList(vwk->category, FALSE);
            cwk->seq++;
        }
        break;
    case 3:
        flag1 = PMSIVCategory_WaitResultList(vwk->category, FALSE);
        flag2 = PMSIVCategory_WaitFadeOut(vwk->category);
        if (flag1 && flag2) {
            PMSIVMenu_SetupWordWinButtons(vwk->menu);
            PMSIVWordWin_SetupWords(vwk->wordWin);
            PMSIVWordWin_StartFadeIn(vwk->wordWin);
            if (PMSInput_GetCategoryMode(cwk->mwk) == 0) {
                PMSIView_PrintMessage(cwk, 3);
            } else {
                PMSIView_PrintMessage(cwk, 5);
            }
            cwk->seq++;
        }
        break;
    case 4:
        if (PMSIVWordWin_WaitFadeIn(vwk->wordWin)) {
            PMSIVWordWin_MoveCursor(vwk->wordWin, PMSInput_GetWordWinCursorPos(vwk->mwk));
            PMSIVWordWin_SetScrollBar(vwk->wordWin, PMSInput_GetWordWinUpArrowVisible(vwk->mwk),
                                PMSInput_GetWordWinDownArrowVisible(vwk->mwk));
            PMSIVWordWin_VisibleCursor(vwk->wordWin, TRUE);
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdWordWinToCategory(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    u32 searchCount = PMSInput_GetSearchResultCount(cwk->mwk);
    BOOL flag1, flag2;

    switch (cwk->seq) {
    case 0:
        PMSIVMenu_StartReturn(vwk->menu);
        cwk->seq++;
        break;
    case 1:
        if (PMSIVMenu_WaitReturn(vwk->menu)) {
            cwk->seq++;
        }
        break;
    case 2:
        PMSIVMenu_SetupCategoryButtons(vwk->menu);
        cwk->seq++;
        break;
    case 3:
        PMSIVWordWin_VisibleCursor(vwk->wordWin, FALSE);
        PMSIVWordWin_StartFadeOut(vwk->wordWin);
        cwk->seq++;
        break;
    case 4:
        if (PMSIVWordWin_WaitFadeOut(vwk->wordWin)) {
            PMSIVCategory_ChangeModeBG(vwk->category);
            cwk->seq++;
        }
        break;
    case 5: {
        BOOL flag = FALSE;

        PMSIView_SetLowerScreen(cwk->vwk, FALSE);
        PMSIVCategory_StartFadeIn(vwk->category);
        if (PMSInput_GetCategoryMode(cwk->mwk) == 0) {
            PMSIVCategory_StartResultList(vwk->category, flag);
        } else {
            if (searchCount) {
                flag = TRUE;
            }
            PMSIVCategory_StartResultList(vwk->category, flag);
        }
        if (PMSInput_GetCategoryMode(cwk->mwk) == 0) {
            PMSIView_PrintMessage(cwk, 2);
        } else {
            PMSIView_PrintMessage(cwk, 4);
        }
        cwk->seq++;
        break;
    }
    case 6:
        flag1 = PMSIVCategory_WaitFadeIn(vwk->category);
        flag2 = PMSIVCategory_WaitResultList(vwk->category, searchCount ? TRUE : FALSE);
        if (flag1 && flag2) {
            PMSIVCategory_MoveCursor(vwk->category, PMSInput_GetCategoryCursorPos(vwk->mwk));
            PMSIVCategory_VisibleCursor(vwk->category, TRUE);
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdWordWinToEditArea(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    BOOL flag1, flag2;

    switch (cwk->seq) {
    case 0:
        cwk->seq++;
        break;
    case 1:
        if (PMSIVWordWin_WaitCursorDecide(vwk->wordWin)) {
            PMSIVMenu_SetupEditButtons(vwk->menu);
            PMSIVWordWin_VisibleCursor(vwk->wordWin, FALSE);
            PMSIVWordWin_StartFadeOut(vwk->wordWin);
            PMSIVEdit_ScrollSet(vwk->edit, TRUE);
            cwk->seq++;
        }
        break;
    case 2:
        flag1 = PMSIVWordWin_WaitFadeOut(vwk->wordWin);
        flag2 = PMSIVEdit_ScrollWait(vwk->edit);
        if (flag1 && flag2) {
            PMSIVCategory_SetDisableBG(vwk->category);
            PMSIView_SetLowerScreen(cwk->vwk, FALSE);
            PMSIVCategory_StartFadeIn(vwk->category);
            if (PMSInput_HasStartSentence(cwk->mwk) || PMSInput_GetInputMode(cwk->mwk) != PMSI_MODE_SENTENCE) {
                PMSIView_PrintMessage(cwk, 1);
            } else {
                PMSIView_PrintMessage(cwk, 0);
            }
            cwk->seq++;
        }
        break;
    case 3:
        if (PMSIVCategory_WaitFadeIn(vwk->category)) {
            PMSIVCategory_StartBrightDown(vwk->category);
            cwk->seq++;
        }
        break;
    case 4:
        if (PMSIVCategory_WaitBrightDown(vwk->category)) {
            PMSIVEdit_UpdateEditArea(vwk->edit);
            PMSIVEdit_VisibleCursor(vwk->edit, TRUE);
            PMSIVEdit_ActiveArrow(vwk->edit);
            PMSIVEdit_ActiveCursor(vwk->edit);
            vwk->status = 0;
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdWordWinToButton(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        PMSIVWordWin_VisibleCursor(vwk->wordWin, FALSE);
        PMSIVWordWin_StartFadeOut(vwk->wordWin);
        cwk->seq++;
        break;
    case 1:
        if (PMSIVWordWin_WaitFadeOut(vwk->wordWin)) {
            PMSIVCategory_SetDisableBG(vwk->category);
            PMSIView_SetLowerScreen(cwk->vwk, FALSE);
            PMSIVCategory_StartFadeIn(vwk->category);
            cwk->seq++;
        }
        break;
    case 2:
        if (PMSIVCategory_WaitFadeIn(vwk->category)) {
            PMSIVCategory_StartBrightDown(vwk->category);
            cwk->seq++;
        }
        break;
    case 3:
        if (PMSIVCategory_WaitBrightDown(vwk->category)) {
            PMSIVEdit_UpdateEditArea(vwk->edit);
            PMSIVEdit_VisibleCursor(vwk->edit, FALSE);
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdMoveEditAreaCursor(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    PMSIVEdit_MoveCursor(vwk->edit, PMSInput_GetEditAreaCursorPos(vwk->mwk));
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdMoveButtonCursor(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), TRUE);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdMoveCategoryCursor(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    PMSIVCategory_MoveCursor(vwk->category, PMSInput_GetCategoryCursorPos(vwk->mwk));
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdMoveWordWinCursor(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    PMSIVWordWin_MoveCursor(vwk->wordWin, PMSInput_GetWordWinCursorPos(vwk->mwk));
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdScrollWordWin(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        PMSIVWordWin_StartScroll(vwk->wordWin, PMSInput_GetWordWinScrollVector(vwk->mwk));
        cwk->seq++;
        break;
    case 1:
        if (PMSIVWordWin_WaitScroll(vwk->wordWin)) {
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdPushButton(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    PMSIVMenu_FlashButton(vwk->menu, PMSInput_GetButtonCursorPos(cwk->mwk), TRUE);
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdNop22(TCB *tcb, void *data) {
    PMSIView_DeleteCommand(data);
}

static void PMSIView_CmdUpdateEditAreaCursor(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    if (vwk->status != 1) {
        PMSIVEdit_ActiveCursor(vwk->edit);
    }
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdNop26(TCB *tcb, void *data) {
    PMSIView_DeleteCommand(data);
}

static void PMSIView_CmdNop27(TCB *tcb, void *data) {
    PMSIView_DeleteCommand(data);
}

static void PMSIView_CmdNop28(TCB *tcb, void *data) {
    PMSIView_DeleteCommand(data);
}

static void PMSIView_CmdNop29(TCB *tcb, void *data) {
    PMSIView_DeleteCommand(data);
}

static void PMSIView_CmdChangeCategoryMode(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;
    u32 searchCount = PMSInput_GetSearchResultCount(cwk->mwk);

    switch (cwk->seq) {
    case 0:
        PMSIVMenu_UpdateSearchButtons(vwk->menu);
        PMSIVCategory_PrintSearchInput(vwk->category);
        PMSIVCategory_StartResultList(vwk->category, searchCount ? TRUE : FALSE);
        cwk->seq++;
        break;
    case 1:
        if (PMSIVCategory_WaitResultList(vwk->category, searchCount ? TRUE : FALSE)) {
            cwk->seq++;
        }
        break;
    case 2:
        PMSIView_DeleteCommand(cwk);
        break;
    }
}

static void PMSIView_CmdSetWordWinArrows(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    PMSIVWordWin_RedrawWords(vwk->wordWin, PMSInput_GetWordWinUpArrowVisible(vwk->mwk));
    PMSIView_DeleteCommand(cwk);
}

static void PMSIView_CmdShowMenu(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        PMSIVMenu_StartButton(vwk->menu, 2);
        cwk->seq++;
        break;
    case 1:
        if (PMSIVMenu_WaitButton(vwk->menu, 2)) {
            if (*vwk->keyMode == 0 && PMSInput_GetCategoryCursorPos(vwk->mwk) == 0xfd &&
                PMSInput_GetCategoryPosSaved(vwk->mwk) == 0xfd) {
                PMSIVMenu_SetCursor(vwk->menu, 1, TRUE);
            } else {
                PMSIVMenu_SetCursor(vwk->menu, 0, FALSE);
            }
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdMoveCategory(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        PMSIVCategory_StartCursorDecide(vwk->category, PMSInput_GetCategoryCursorPos(vwk->mwk));
        cwk->seq++;
        break;
    case 1:
        if (PMSIVCategory_WaitCursorDecide(vwk->category)) {
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

static void PMSIView_CmdMoveWordWin(TCB *tcb, void *data) {
    PMSIVCommandWork *cwk = data;
    PMSInputView *vwk = cwk->vwk;

    switch (cwk->seq) {
    case 0:
        PMSIVWordWin_StartCursorDecide(vwk->wordWin, PMSInput_GetWordWinCursorPos(vwk->mwk));
        cwk->seq++;
        break;
    case 1:
        if (PMSIVWordWin_WaitCursorDecide(vwk->wordWin)) {
            PMSIView_DeleteCommand(cwk);
        }
        break;
    }
}

u32 PMSIView_GetSentenceEditPosMax(PMSInputView *vwk) {
    return PMSIVEdit_GetWordCount(vwk->edit);
}

u16 PMSIView_GetSentenceWord(PMSInputView *vwk, u32 index) {
    return PMSIVEdit_GetWordIndex(vwk->edit, index);
}

void PMSIView_GetSentenceWordArea(PMSInputView *vwk, TouchRect *rect, u8 index) {
    PMSIVEdit_GetWordArea(vwk->edit, rect, index);
}

int PMSIView_GetMenuButton(PMSInputView *vwk) {
    if (PMSIVMenu_IsFlashFinished(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk))) {
        return 1;
    }
    return -1;
}

void PMSIView_SetMenuButton(PMSInputView *vwk, u32 which) {
    PMSIVMenu_SetEditButton(vwk->menu, which);
}

PMSIVMenu *PMSIView_GetMenu(PMSInputView *vwk) {
    return vwk->menu;
}

ClActUnit *PMSIView_GetActUnit(PMSInputView *vwk) {
    return vwk->actUnit;
}

Font *PMSIView_GetFont(PMSInputView *vwk) {
    return vwk->font;
}

PrintQueue *PMSIView_GetPrintQueue(PMSInputView *vwk) {
    return vwk->printQueue;
}

void PMSIView_GetObjRes2(PMSInputView *vwk, PMSIVObjRes *res, u32 lcd) {
    *res = vwk->objRes2[lcd];
}

void PMSIView_GetObjRes(PMSInputView *vwk, PMSIVObjRes *res, u32 lcd, u32 bgPriority) {
    *res = vwk->objRes[lcd];
}

ClActor *PMSIView_AddActor(PMSInputView *vwk, const PMSIVObjRes *res, u32 x, u32 y, u32 priority, int drawArea) {
    ClActorSetup setup;
    ClActor *actor;
    u32 irq;
    u16 surface;

    setup.x = x;
    setup.y = y;
    setup.priority = priority;
    setup.bgPriority = 0;
    setup.sequence = 0;
    if (drawArea == NNS_G2D_VRAM_TYPE_2DSUB) {
        surface = CLACT_SURFACE_SUB;
    } else {
        surface = CLACT_SURFACE_MAIN;
    }
    irq = CPU_IRQDisable();
    actor = func_0204c040(vwk->actUnit, res->chars, res->palette, res->cellAnims, &setup, surface, HEAPID_PMS_INPUT);
    CPU_SetIRQMask(irq);
    if (actor) {
        func_0204c520(actor, TRUE);
        func_0204c53c(actor, FX32_ONE);
    }
    return actor;
}

int PMSIView_GetWordWinScrollDir(PMSInputView *vwk, u32 unused, u32 pos) {
    ClActorPos barPos;

    if (!PMSIVWordWin_GetScrollBarPos(vwk->wordWin, &barPos)) {
        return 3;
    }
    if (pos < barPos.y) {
        return 1;
    }
    if (pos > barPos.y) {
        return 2;
    }
    if (pos == barPos.y) {
        return 0;
    }
    return 3;
}

void PMSIView_SetWordWinScrollBarY(PMSInputView *vwk, u32 y) {
    PMSIVWordWin_SetScrollBarY(vwk->wordWin, y);
}

u32 PMSIView_GetWordWinScrollBarPos(PMSInputView *vwk, u32 count) {
    return PMSIVWordWin_GetScrollBarLine(vwk->wordWin, count);
}

void PMSIView_SetLowerScreen(PMSInputView *vwk, BOOL categories) {
    u32 fileId, palOffset;

    if (categories) {
        fileId = 29;
        palOffset = 0x60;
    } else {
        fileId = 28;
        palOffset = 0;
    }
    GFL_G2DIOLoadNSCRAsync(vwk->arc, fileId, 3, 0, palOffset, 0, FALSE, HEAPID_PMS_INPUT);
    vwk->lowerScreenChanged = TRUE;
    vwk->lowerScreenCategories = categories;
}

static void PMSIView_PrintMessage(PMSIVCommandWork *cwk, u32 msg) {
    u32 msgIds[] = { 22, 23, 27, 24, 25, 26 };
    MsgData *msgData;
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(cwk->vwk->msgWin), 7);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PMSIV_MSG_FILE, HEAPID_PMS_INPUT);
    str = GFL_MsgDataLoadStrbufNew(msgData, msgIds[msg]);
    func_02021c7c(cwk->vwk->printQueue, BmpWin_GetBitmap(cwk->vwk->msgWin), 2, 4, str, cwk->vwk->font,
                  PRINT_COLOR(14, 15, 7));
    cwk->vwk->msgPrinting = TRUE;
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
}

static void PMSIView_FlushMessage(PMSInputView *vwk) {
    if (vwk->msgPrinting) {
        if (!func_02021c1c(vwk->printQueue, BmpWin_GetBitmap(vwk->msgWin))) {
            BmpWin_Transfer(vwk->msgWin);
            vwk->msgPrinting = FALSE;
        }
    }
}

void PMSIView_ChangeKTEditArea(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    if (*vwk->keyMode == 1) {
        PMSIVEdit_VisibleCursor(vwk->edit, FALSE);
        PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), FALSE);
    } else if (vwk->status == 1) {
        PMSIVEdit_VisibleCursor(vwk->edit, FALSE);
        PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), TRUE);
    } else {
        PMSIVEdit_VisibleCursor(vwk->edit, TRUE);
        PMSIVMenu_SetCursor(vwk->menu, PMSInput_GetButtonCursorPos(vwk->mwk), FALSE);
    }
}

void PMSIView_ChangeKTCategory(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSIVCategory_VisibleCursor(vwk->category, TRUE);
    if (*vwk->keyMode == 1) {
        if (PMSInput_GetCategoryMode(mwk) == 1) {
            PMSIVMenu_SetCursor(vwk->menu, 0, FALSE);
        }
    } else {
        PMSIVCategory_MoveCursor(vwk->category, PMSInput_GetCategoryCursorPos(vwk->mwk));
    }
}

void PMSIView_ChangeKTWordWin(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSIVWordWin_VisibleCursor(vwk->wordWin, TRUE);
}
