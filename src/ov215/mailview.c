#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/mailview.h"
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
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "text/system/btl_server_flow_3.h"

// The mail screen's graphics (overlay 215's mailview.c): its BGs, palettes, windows, buttons and the mail's text,
// loaded a step at a time by MailView_Load. The names are ours

// The mail graphics' archive, which no header names yet. A design's files are its palette, its characters
// MAIL_FILE_CHAR_BASE files on, and its screen MAIL_FILE_SCREEN_BASE files on
#define ARCID_MAIL 0x99
#define MAIL_FILE_CHAR_BASE 12
#define MAIL_FILE_SCREEN_BASE 24
// The screen of the buttons, with the write phase's frame
#define MAIL_FILE_WRITE_SCREEN 36

// The BGs of the screen: the mail's, the sentences, the text and the buttons' frame, and the sub screen's
#define MAIL_BG_COUNT 5

// The steps of the loading
enum {
    MAIL_LOAD_MESSAGES,
    MAIL_LOAD_GRAPHICS,
    MAIL_LOAD_WINDOWS,
    MAIL_LOAD_OBJECTS,
};

static void MailView_InitFade(MailSysWork *work);
static void MailView_CreatePMSDraw(MailSysWork *work);
static void MailView_DeletePMSDraw(MailSysWork *work);
static void MailView_SetVRAMBanks(MailSysWork *work);
static void MailView_InitBGs(MailSysWork *work);
static void MailView_ReleaseBGs(MailSysWork *work);
static void MailView_LoadGraphics(MailSysWork *work);
static void MailView_ExitFade(MailSysWork *work);
static void MailView_CreateWindows(MailSysWork *work);
static void MailView_CreateButtons(MailSysWork *work);
static void MailView_DeleteWindows(MailSysWork *work);
static void MailView_PrintMail(MailSysWork *work);
static void MailView_CreateClAct(MailSysWork *work);
static void MailView_DeleteClAct(MailSysWork *work);

BOOL MailView_Load(MailSysWork *work) {
    switch (work->subSeq) {
    case MAIL_LOAD_MESSAGES:
        work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_3, work->heapId);
        work->msgBuf = GFL_StrBufCreate(128, work->heapId);
        MailView_InitBGs(work);
        break;
    case MAIL_LOAD_GRAPHICS:
        MailView_LoadGraphics(work);
        break;
    case MAIL_LOAD_WINDOWS:
        MailView_CreateWindows(work);
        break;
    case MAIL_LOAD_OBJECTS:
        MailView_CreateClAct(work);
        work->menuRes = AppTaskMenuRes_Create(0, 8, work->font, work->printQueue, work->heapId);
        work->subSeq = 0;
        MailView_CreatePMSDraw(work);
        MailView_PrintMail(work);
        MailView_CreateButtons(work);
        MailView_InitFade(work);
        return TRUE;
    }
    work->subSeq++;
    return FALSE;
}

static void MailView_InitFade(MailSysWork *work) {
    PaletteFade_BlendBuffer(work->palFade, 0, 0, 0x70, 16, 0);
    PaletteFade_SetTransferAll(work->palFade, TRUE);
    PaletteFade_Transfer(work->palFade);
    work->blinkLevel = 0;
    work->blinkFalling = 0;
    work->tcbBuffer = GFL_HeapAllocate(work->heapId, GFL_TCBMgrCalcAllocSize(3), FALSE, "mailview.c", 0x4fd);
    work->tcbMgr = GFL_TCBMgrCreate(3, work->tcbBuffer);
    work->blinkTask = GFL_TCBMgrAddTask(work->tcbMgr, MailSys_UpdateBlink, work, 0);
}

BOOL MailView_Unload(MailSysWork *work) {
    MailView_DeletePMSDraw(work);
    MailView_DeleteClAct(work);
    if (work->unk28) {
        GFL_MsgDataFree(work->msgData);
    }
    GFL_StrBufFree(work->msgBuf);
    AppTaskMenuRes_Free(work->menuRes);
    MailView_DeleteWindows(work);
    MailView_ExitFade(work);
    MailView_ReleaseBGs(work);
    return TRUE;
}

static void MailView_CreatePMSDraw(MailSysWork *work) {
    work->pmsQueue = func_02021998(work->heapId);
    work->pmsClUnit = func_0204bf1c(8, 1, work->heapId);
    work->pmsDraw = PMSDraw_Create(work->pmsClUnit, 0, work->pmsQueue, work->font, 0, 4, work->heapId);
    PMSDraw_SetBackColor(work->pmsDraw, 0);
}

static void MailView_DeletePMSDraw(MailSysWork *work) {
    PMSDraw_Delete(work->pmsDraw);
    func_02021a18(work->pmsQueue);
    work->pmsQueue = NULL;
    func_0204bf98(work->pmsClUnit);
}

static void MailView_SetVRAMBanks(MailSysWork *work) {
    GFL_BGSysSetVRAMBanks(&MailSys_VRAMConfig);
}

static void MailView_InitBGs(MailSysWork *work) {
    MailView_SetVRAMBanks(work);
    BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
    GFL_BGSysSetLCDConfig(&lcdConfig);
    GFL_BGSysSetDisplayLayout(0);
    BGSetup bgSetups[MAIL_BG_COUNT] = {
        { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x8000), 0x4000,
          GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
        { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000), 0x8000,
          GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
        { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x0000), 0x4000,
          GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
        { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe000), GX_BG_CHARBASE(0x0000), 0x4000,
          GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
        { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x0000), 0x4000,
          GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    };

    GFL_BGSysCreateBG(0, &bgSetups[0], BGMODE_TEXT);
    GFL_BGSysCreateBG(1, &bgSetups[1], BGMODE_TEXT);
    GFL_BGSysCreateBG(2, &bgSetups[2], BGMODE_TEXT);
    GFL_BGSysCreateBG(3, &bgSetups[3], BGMODE_TEXT);
    GFL_BGSysCreateBG(4, &bgSetups[4], BGMODE_TEXT);
    GFL_BGSysClearScr(0);
    GFL_BGSysClearScr(1);
    GFL_BGSysClearScr(2);
    GFL_BGSysClearScr(3);
    GFL_BGSysClearScr(4);
    GFL_BGSysClearCharCore(0, 0x20, 0, work->heapId);
    GFL_BGSysClearCharCore(1, 0x20, 0, work->heapId);
    GFL_BGSysClearCharCore(2, 0x20, 0, work->heapId);
    GFL_BGSysClearCharCore(3, 0x20, 0, work->heapId);
    GFL_BGSysClearCharCore(4, 0x20, 0, work->heapId);
}

static void MailView_ReleaseBGs(MailSysWork *work) {
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysSetDisplayLayout(1);
}

static void MailView_LoadGraphics(MailSysWork *work) {
    u32 design = work->edit->design;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAIL, work->heapId);

    LoadSysMsgBox(0, 1, 4, 0, work->heapId);
    LoadSysMsgBox(0, 13, 6, 0, work->heapId);
    u32 charFile = design + MAIL_FILE_CHAR_BASE;
    GFL_BGSysLoadArcNCGRStatic(arc, charFile, 3, 0, 0, TRUE, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, charFile, 4, 0, 0, TRUE, work->heapId);
    u32 screenFile = design + MAIL_FILE_SCREEN_BASE;
    u32 screenSize = 0x800;
    loadBGScrToVramByFileNoReserveNegAlign(arc, screenFile, 3, 0, screenSize, TRUE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, screenFile, 4, 0, screenSize, TRUE, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, design, PALTYPE_SUB_BG, 0, 0, work->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0xa0, 0x20, work->heapId);
    work->palFade = PaletteFade_Create(work->heapId);
    PaletteFade_AllocBuffer(work->palFade, 0, 0xe0, work->heapId);
    PaletteFade_LoadArcNCLR(work->palFade, arc, design, work->heapId, 0, 0x60, 0);
    PaletteFade_LoadNCLR(work->palFade, ARCID_FONT, 7, work->heapId, 0, 0x20, 0x30);
    PaletteFade_LoadNCLR(work->palFade, ARCID_FONT, 5, work->heapId, 0, 0x20, 0x50);
    PaletteFade_LoadNCLR(work->palFade, ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), work->heapId, 0, 0x20, 0x40);
    PaletteFade_LoadNCLR(work->palFade, ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(1), work->heapId, 0, 0x20, 0x60);
    func_020232d8();
    GFL_ArcToolFree(arc);
    GFL_BGSysFillScrArea(4, 0x2001, 0, 0, 32, 32, 0x11);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
    if (work->phase != MAIL_PHASE_VIEW) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_MAIL, work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, MAIL_FILE_WRITE_SCREEN, 2, 0, screenSize, TRUE, work->heapId);
        GFL_ArcToolFree(arc);
        work->onKeyMode = MailSys_OnKeyMode;
        work->onTouchMode = MailSys_OnTouchMode;
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, TRUE);
    }
}

static void MailView_ExitFade(MailSysWork *work) {
    PaletteFade_FreeBuffer(work->palFade, 0);
    PaletteFade_Free(work->palFade);
    work->palFade = NULL;
    GFL_TCBRemove(work->blinkTask);
    func_0203a610(work->tcbMgr);
    GFL_HeapFree(work->tcbBuffer);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, 0, 31, 0);
}

static void MailView_CreateWindows(MailSysWork *work) {
    int i;

    BmpWin_InitAllocator(work->heapId);
    work->windows[0] = BmpWin_CreateDynamic(1, 0, 0, 1, 1, 5, 0);
    work->windows[1] = BmpWin_CreateDynamic(1, 3, 3, 26, 4, 1, 0);
    work->windows[2] = BmpWin_CreateDynamic(1, 3, 8, 26, 4, 1, 0);
    work->windows[3] = BmpWin_CreateDynamic(1, 3, 13, 26, 4, 1, 0);
    work->windows[4] = BmpWin_CreateDynamic(1, 8, 19, 11, 2, 1, 0);
    work->windows[5] = BmpWin_CreateDynamic(1, 21, 19, 8, 2, 1, 0);
    work->windows[6] = BmpWin_CreateDynamic(0, 2, 1, 28, 4, 5, 1);
    for (i = 0; i < 6; i++) {
        work->entries[i].win = work->windows[i];
        work->entries[i].drawn = FALSE;
    }
    work->printQueue = func_02021998(work->heapId);
    for (i = 0; i < 7; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i]), 0);
        if (i < 5) {
            BmpWin_TransferNow(work->windows[i]);
        }
    }
}

static void MailView_CreateButtons(MailSysWork *work) {
    if (work->phase == MAIL_PHASE_WRITE) {
        work->buttonItems[0].str = GFL_MsgDataLoadStrbufNew(work->msgData, BtlServerFlow3_Text_Confirm);
        work->buttonItems[0].color = PRINT_COLOR(14, 15, 3);
        work->buttonItems[0].type = 0;
        work->buttonItems[1].str = GFL_MsgDataLoadStrbufNew(work->msgData, BtlServerFlow3_Text_Cancel);
        work->buttonItems[1].color = PRINT_COLOR(14, 15, 3);
        work->buttonItems[1].type = APP_TASKMENU_ITEM_RETURN;
        work->confirmButton = AppTaskMenuWin_Create(work->menuRes, &work->buttonItems[0], 21, 18, 10, work->heapId);
        work->cancelButton = AppTaskMenuWin_Create(work->menuRes, &work->buttonItems[1], 21, 21, 10, work->heapId);
    }
}

static void MailView_DeleteWindows(MailSysWork *work) {
    int i;

    if (work->phase == MAIL_PHASE_WRITE) {
        AppTaskMenuWin_Free(work->confirmButton);
        AppTaskMenuWin_Free(work->cancelButton);
        GFL_StrBufFree(work->buttonItems[0].str);
        GFL_StrBufFree(work->buttonItems[1].str);
    }
    func_02021a18(work->printQueue);
    work->printQueue = NULL;
    for (i = 0; i < 7; i++) {
        BmpWin_ClearScreenNow(work->windows[i]);
        BmpWin_Free(work->windows[i]);
    }
    BmpWin_FreeAllocator();
}

static void MailView_PrintMail(MailSysWork *work) {
    int i;

    for (i = 0; i < MAIL_WORD_COUNT; i++) {
        if (PMSData_IsNotEmpty(&work->edit->words[i])) {
            PMSDraw_Print(work->pmsDraw, work->windows[i + 1], &work->edit->words[i], i);
        }
    }
    if (work->edit->word != 0xffff) {
        work->pmsWord.type = 6;
        work->pmsWord.id = 0;
        work->pmsWord.words[0] = work->edit->word;
        PMSDraw_Print(work->pmsDraw, work->windows[4], &work->pmsWord, 3);
    }
    if (work->phase == MAIL_PHASE_VIEW) {
        PrintQueue *queue = work->printQueue;

        func_02021c7c(queue, BmpWin_GetBitmap(work->entries[5].win), 0, 2, work->edit->name, work->font,
                      PRINT_COLOR(1, 2, 0));
        work->entries[5].drawn = TRUE;
        BmpWin_TransferNow(work->windows[5]);
    }
}

static void MailView_CreateClAct(MailSysWork *work) {
    ClActSys_Create(&data_02093f08, &MailSys_VRAMConfig, work->heapId);
    work->clUnit = func_0204bf1c(4, 0, work->heapId);
    func_0204c028(work->clUnit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    func_02042ba8(FALSE, work->heapId);
}

// The counterpart of MailView_CreateClAct, which the ROM places after it (0x021a8d58). Whether it belongs to this file
// or starts mb_seq.c is a guess: it frees what MailView_CreateClAct made
static void MailView_DeleteClAct(MailSysWork *work) {
    func_0204bf98(work->clUnit);
    func_0204b758();
}
