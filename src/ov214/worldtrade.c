#include "types.h"
#include "app/worldtrade.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "dpw/dpw_tr.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/wm_icon.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "save/player_info.h"
#include "system/app_taskmenu.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/0415.h"
#include "worldtrade_local.h"

// The Global Trade Station's proc: its work, the screens it switches between, and the graphics, menus and helpers
// they share. The names are ours, guesses but for the asserted ones

typedef int (*WorldTradeSubProcFunc)(WorldTradeWork *wk, int seq);

static BOOL WorldTradeProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WorldTradeProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WorldTradeProc_End(GameProc *proc, u32 *state, void *param, void *work);
static void WorldTrade_VBlankFunc(TCB *tcb, void *data);
static void WorldTrade_VramBankSet(const BGSysVRAMConfig *config);
static void WorldTrade_InitWork(WorldTradeWork *wk, WorldTradeParam *param);
static void WorldTrade_FreeWork(WorldTradeWork *wk);
static void WorldTrade_WndSetting(void);
static void WorldTrade_InitCellActor(WorldTradeWork *wk, const BGSysVRAMConfig *vramConfig);
static void WorldTrade_ShowOBJ(WorldTradeWork *wk);
static int WorldTrade_WifiLinkLevel(void);
static void WorldTrade_InitCLACT(WorldTradeWork *wk);
static void WorldTrade_FreeCLACT(WorldTradeWork *wk);
static void WorldTrade_ServerWaitTimeFunc(WorldTradeWork *wk);
static void WorldTrade_SetPassive(BOOL main);

const GameProcFunctions WORLDTRADE_PROC_FUNCTIONS = {
    WorldTradeProc_Init,
    WorldTradeProc_Main,
    WorldTradeProc_End,
};

static WorldTradeWork *sWorldTradeWork;

static WorldTradeSubProcFunc sSubProcessTable[][3] = {
    { WorldTrade_Enter_Init, WorldTrade_Enter_Main, WorldTrade_Enter_End },
    { WorldTrade_Title_Init, WorldTrade_Title_Main, WorldTrade_Title_End },
    { WorldTrade_MyPoke_Init, WorldTrade_MyPoke_Main, WorldTrade_MyPoke_End },
    { WorldTrade_Partner_Init, WorldTrade_Partner_Main, WorldTrade_Partner_End },
    { WorldTrade_Search_Init, WorldTrade_Search_Main, WorldTrade_Search_End },
    { WorldTrade_Box_Init, WorldTrade_Box_Main, WorldTrade_Box_End },
    { WorldTrade_Deposit_Init, WorldTrade_Deposit_Main, WorldTrade_Deposit_End },
    { WorldTrade_Upload_Init, WorldTrade_Upload_Main, WorldTrade_Upload_End },
    { WorldTrade_Status_Init, WorldTrade_Status_Main, WorldTrade_Status_End },
    { WorldTrade_Demo_Init, WorldTrade_Demo_Main, WorldTrade_Demo_End },
};

static BOOL WorldTradeProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WorldTradeParam *wtParam = param;
    WorldTradeWork *wk;

    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WORLDTRADE, 0x30100);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WorldTradeWork), HEAPID_WORLDTRADE);
    sys_memset(wk, 0, sizeof(WorldTradeWork));
    sWorldTradeWork = wk;

    wk->demoPokemon = WorldTrade_AllocPartyPkm(HEAPID_WORLDTRADE);
    wk->procManager = CreateGameProcManager(HEAPID_WORLDTRADE);
    wk->tcbBuffer = GFL_HeapAllocate(HEAPID_WORLDTRADE, GFL_TCBMgrCalcAllocSize(8), FALSE, "worldtrade.c", 161);
    wk->tcbManager = GFL_TCBMgrCreate(8, wk->tcbBuffer);
    WorldTrade_PrintInit(&wk->print, wtParam->config);

    wk->wordSet = GFL_WordSetSystemCreate(11, 64, HEAPID_WORLDTRADE);
    wk->msgManager = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0415, HEAPID_WORLDTRADE);
    wk->lobbyMsgManager = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0416, HEAPID_WORLDTRADE);
    wk->systemMsgManager = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_ERROR, HEAPID_WORLDTRADE);
    wk->monsNameManager = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SPECIES_NAMES, HEAPID_WORLDTRADE);
    wk->countryNameManager = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE_LOAD_COUNTRY_TO_STRBUF, HEAPID_WORLDTRADE);

    WorldTrade_InitWork(wk, wtParam);
    GFL_BGSysInitVRAM(0);
    WorldTrade_InitGraphics(wk);
    GFL_SndBGMSetPaused(FALSE);
    func_02005d8c();
    return TRUE;
}

static BOOL WorldTradeProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WorldTradeWork *wk = work;

    if (func_02042788()) {
        func_0205b5ec();
        func_0203e7f8(WorldTrade_WifiLinkLevel());
    }
    func_ov189_021a6d00();
    wk->procResult = GFL_ProcMgrUpdate(wk->procManager);
    GFL_TCBMgrUpdate(wk->tcbManager);
    WorldTrade_PrintMain(&wk->print);

    switch (*state) {
    case WT_SEQ_INIT:
        *state = sSubProcessTable[wk->subProcess][0](wk, *state);
        WorldTrade_WndSetting();
        break;
    case WT_SEQ_FADEIN:
        if (GFL_WipeIsFinished()) {
            *state = WT_SEQ_MAIN;
        }
        break;
    case WT_SEQ_MAIN:
        *state = sSubProcessTable[wk->subProcess][1](wk, *state);
        break;
    case WT_SEQ_FADEOUT:
        if (GFL_WipeIsFinished()) {
            *state = sSubProcessTable[wk->subProcess][2](wk, *state);
            if (wk->subprocFlag) {
                WorldTrade_SubLcdActorAdd(wk);
                WorldTrade_SubLcdMatchObjAppear(wk, wk->searchResult, 0);
                wk->subprocFlag = 0;
            }
        }
        break;
    case WT_SEQ_OUT:
        *state = WT_SEQ_END;
        break;
    case WT_SEQ_END:
        return TRUE;
    }

    WorldTrade_ServerWaitTimeFunc(wk);
    if (wk->clactUnit != NULL) {
        func_0204b794();
    }
    if (wk->unk12E8 > 0) {
        wk->unk12E8--;
    }
    return FALSE;
}

static BOOL WorldTradeProc_End(GameProc *proc, u32 *state, void *param, void *work) {
    WorldTradeWork *wk = work;

    func_02005d8c();
    WorldTrade_ExitGraphics(wk);
    WorldTrade_FreeWork(wk);

    GFL_MsgDataFree(wk->monsNameManager);
    GFL_MsgDataFree(wk->systemMsgManager);
    GFL_MsgDataFree(wk->lobbyMsgManager);
    GFL_MsgDataFree(wk->msgManager);
    GFL_MsgDataFree(wk->countryNameManager);
    GFL_WordSetSystemFree(wk->wordSet);

    func_0203a610(wk->tcbManager);
    GFL_HeapFree(wk->tcbBuffer);
    FreeGameProcManager(wk->procManager);
    GFL_HeapFree(wk->demoPokemon);
    WorldTrade_PrintExit(&wk->print);

    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WORLDTRADE);
    GFL_OvlUnload(OVERLAY_ID(139));
    GFL_OvlUnload(OVERLAY_ID(189));
    return TRUE;
}

static void WorldTrade_VBlankFunc(TCB *tcb, void *data) {
    WorldTradeWork *wk = data;

    GFL_BGSysUpdate();
    func_0204b7c8();
    if (wk->vfunc != NULL) {
        wk->vfunc(wk);
        wk->vfunc = NULL;
    }
    if (wk->vfunc2 != NULL) {
        wk->vfunc2(wk);
    }
}

static void WorldTrade_VramBankSet(const BGSysVRAMConfig *config) {
    GFL_BGSysSetVRAMBanks(config);
}

static void WorldTrade_InitWork(WorldTradeWork *wk, WorldTradeParam *param) {
    GFL_ASSERT(param->savedata != NULL);
    GFL_ASSERT(param->worldtrade_data != NULL);
    GFL_ASSERT(param->systemdata != NULL);
    GFL_ASSERT(param->myparty != NULL);
    GFL_ASSERT(param->mybox != NULL);
    GFL_ASSERT(param->wifilist != NULL);
    GFL_ASSERT(param->wifihistory != NULL);
    GFL_ASSERT(param->mystatus != NULL);
    GFL_ASSERT(param->config != NULL);
    GFL_ASSERT(param->record != NULL);

    wk->param = param;
    wk->subProcess = WORLDTRADE_ENTER;
    WorldTrade_SubProcessChange(wk, WORLDTRADE_ENTER, 0x13);
    wk->titleCursorPos = 0;
    wk->search.characterNo = 0;
    wk->search.gender = 3;
    wk->search.level_min = 0;
    wk->search.level_max = 0;
    wk->searchBackup.characterNo = 0;
    wk->demoEnd = 0;
    wk->boxTrayNo = 0xff;
    wk->boxPokeNum = 0;
    wk->boxSearchFlag = 0;
    wk->subLcdTouchOK = 0;
    wk->timeWaitWork = NULL;
    wk->countryCode = 0;
}

static void WorldTrade_FreeWork(WorldTradeWork *wk) {
}

static void WorldTrade_WndSetting(void) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    // The wireless strength icon sits in window 0, out of the fades
    GX_SetVisibleWnd(GX_WNDMASK_W0);
    G2_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                              GX_WND_PLANEMASK_OBJ,
                          FALSE);
    G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                              GX_WND_PLANEMASK_OBJ,
                          TRUE);
    G2_SetWnd0Position(256 - 16, 0, 255, 16);
}

static const ClActSysSetup sClActSysSetup = {
    0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16,
};

static void WorldTrade_InitCellActor(WorldTradeWork *wk, const BGSysVRAMConfig *vramConfig) {
    void *plttBuf;
    int animFile;
    int cellFile;
    int plttFile;
    int charFile;
    ArcTool *arc;
    NNSG2dPaletteData *pltt;
    u16 *colors;
    int i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);
    ClActSys_Create(&sClActSysSetup, vramConfig, HEAPID_WORLDTRADE);
    wk->clactUnit = func_0204bf1c(96, 0, HEAPID_WORLDTRADE);

    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR] =
        func_0204b81c(arc, 0x11, TRUE, CLACT_VRAM_MAIN, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT] = func_0204bba0(arc, 8, CLACT_VRAM_MAIN, 0, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL] = func_0204bde0(arc, 0x12, 0x13, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR] =
        func_0204b81c(arc, 0x27, TRUE, CLACT_VRAM_MAIN, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL] = func_0204bde0(arc, 0x28, 0x29, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CHAR] =
        func_0204b81c(arc, 0x24, TRUE, CLACT_VRAM_SUB, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_PLTT] = func_0204bba0(arc, 7, CLACT_VRAM_SUB, 0, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CELL] = func_0204bde0(arc, 0x25, 0x26, HEAPID_WORLDTRADE);
    GFL_ArcToolFree(arc);

    // The Pokémon icons' palettes, and a copy at half brightness for icons that can't be chosen
    plttBuf = GFL_G2DIOReadNCLR(7, 0, &pltt, HEAPID_WORLDTRADE);
    cp15_flushDC(pltt->rawData, 0x60);
    gfxUploadStdPaletteObjA(pltt->rawData, 0xa0, 0x60);
    colors = pltt->rawData;
    for (i = 0; i < 48; i++) {
        colors[i] =
            (((colors[i] >> 10) & 0x1f) / 2 << 10) | (((colors[i] >> 5) & 0x1f) / 2 << 5) | (colors[i] & 0x1f) / 2;
    }
    cp15_flushDC(pltt->rawData, 0x60);
    gfxUploadStdPaletteObjA(pltt->rawData, 0x100, 0x60);
    GFL_HeapFree(plttBuf);

    if ((u8)getTrainerGender(wk->param->mystatus) == 1) {
        charFile = 0x11;
        animFile = 0x10;
        cellFile = 0xf;
        plttFile = 6;
    } else {
        charFile = 0xe;
        animFile = 0xd;
        cellFile = 0xc;
        plttFile = 4;
    }
    arc = GFL_ArcSysCreateFileHandle(30, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_CHAR] =
        func_0204b81c(arc, charFile, FALSE, CLACT_VRAM_SUB, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_PLTT] =
        func_0204bbb8(arc, plttFile, CLACT_VRAM_SUB, 0, 0, 1, HEAPID_WORLDTRADE);
    wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_CELL] = func_0204bde0(arc, cellFile, animFile, HEAPID_WORLDTRADE);
    GFL_ArcToolFree(arc);
}

static void WorldTrade_ShowOBJ(WorldTradeWork *wk) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void WorldTrade_TouchWinYesNoMake(WorldTradeWork *wk, int y, int cgx, int palette, u8 passive) {
    WorldTrade_TouchWinYesNoMakeEx(wk, y, cgx, palette, 0, passive);
}

void WorldTrade_TouchWinYesNoMakeEx(WorldTradeWork *wk, int y, int cgx, int palette, int frame, u8 passive) {
    GFL_ASSERT(wk->task_res == NULL);
    GFL_ASSERT(wk->task_work == NULL);

    wk->task_res = AppTaskMenuRes_Create((u8)frame, (u8)palette, wk->print.font, wk->print.printQueue, HEAPID_WORLDTRADE);
    {
        AppTaskMenuItem items[2] = {
            { NULL, 0x39e3, 0 },
            { NULL, 0x39e3, 0 },
        };
        AppTaskMenuInit init;

        sys_memset(&init, 0, sizeof(AppTaskMenuInit));
        items[0].str = GFL_MsgDataLoadStrbufNew(wk->msgManager, Bank0415_Text_Yes);
        items[1].str = GFL_MsgDataLoadStrbufNew(wk->msgManager, Bank0415_Text_No);
        init.heapId = HEAPID_WORLDTRADE;
        init.itemCount = 2;
        init.items = items;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        init.x = 32;
        init.y = y;
        init.width = 8;
        init.height = 3;
        wk->task_work = AppTaskMenu_Create(&init, wk->task_res);
        GFL_StrBufFree(items[0].str);
        GFL_StrBufFree(items[1].str);
    }

    if (passive) {
        BOOL isMain = FALSE;

        if (frame <= 3) {
            isMain = TRUE;
        }
        WorldTrade_SetPassive(isMain);
    }
}

void WorldTrade_TouchWinYesNoDel(WorldTradeWork *wk) {
    if (wk->task_work != NULL) {
        AppTaskMenu_Free(wk->task_work);
        wk->task_work = NULL;
    }
    if (wk->task_res != NULL) {
        AppTaskMenuRes_Free(wk->task_res);
        wk->task_res = NULL;
    }
}

u32 WorldTrade_TouchSwMain(WorldTradeWork *wk) {
    u32 ret = 0;

    if (wk->task_work != NULL) {
        AppTaskMenu_Update(wk->task_work);
        if (AppTaskMenu_IsFlashFinished(wk->task_work)) {
            ret = AppTaskMenu_GetCursorPos(wk->task_work);
            if (ret == 0) {
                ret = 1;
            } else if (ret == 1) {
                ret = 2;
            }
            WorldTrade_ClearPassive();
        }
    }
    return ret;
}

void WorldTrade_SelBoxInit(WorldTradeWork *wk, u8 frame, int count, int y) {
    int i;

    GFL_ASSERT(wk->task_res == NULL);
    GFL_ASSERT(wk->task_work == NULL);

    wk->task_res = AppTaskMenuRes_Create(frame, 10, wk->print.font, wk->print.printQueue, HEAPID_WORLDTRADE);
    {
        AppTaskMenuItem itemWork[3] = {
            { NULL, 0x39e3, 0 },
            { NULL, 0x39e3, 0 },
            { NULL, 0x39e3, 0 },
        };
        AppTaskMenuInit init;

        sys_memset(&init, 0, sizeof(AppTaskMenuInit));
        // The assert prints its expression with these spaces
        // clang-format off
        GFL_ASSERT(count <= NELEMS( itemWork ));
        // clang-format on
        for (i = 0; i < count; i++) {
            itemWork[i].str = wk->menuList[i].text;
        }
        init.heapId = HEAPID_WORLDTRADE;
        init.itemCount = count;
        init.items = itemWork;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        init.x = 32;
        init.y = y;
        init.width = 13;
        init.height = 3;
        wk->task_work = AppTaskMenu_Create(&init, wk->task_res);
    }
    WorldTrade_SetPassive(TRUE);
}

int WorldTrade_SelBoxMain(WorldTradeWork *wk) {
    int ret = -1;

    if (wk->task_work != NULL) {
        AppTaskMenu_Update(wk->task_work);
        if (AppTaskMenu_IsFlashFinished(wk->task_work)) {
            ret = AppTaskMenu_GetCursorPos(wk->task_work) + 1;
            WorldTrade_ClearPassive();
        }
    }
    return ret;
}

void WorldTrade_SelBoxEnd(WorldTradeWork *wk) {
    if (wk->task_work != NULL) {
        AppTaskMenu_Free(wk->task_work);
        wk->task_work = NULL;
    }
    if (wk->task_res != NULL) {
        AppTaskMenuRes_Free(wk->task_res);
        wk->task_res = NULL;
    }
    WorldTrade_ClearPassive();
}

void WorldTrade_SetNextSeq(WorldTradeWork *wk, int toSeq, int nextSeq) {
    wk->subprocessSeq = toSeq;
    wk->subprocessNextSeq = nextSeq;
}

void WorldTrade_ActPos(ClActor *act, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(act, &pos, 1);
}

static int WorldTrade_WifiLinkLevel(void) {
    return 3 - func_0205b250();
}

void WorldTrade_SubProcessChange(WorldTradeWork *wk, int subProcess, int mode) {
    wk->subNextProcess = subProcess;
    wk->subProcessMode = mode;
}

void WorldTrade_SubProcessUpdate(WorldTradeWork *wk) {
    wk->oldSubProcess = wk->subProcess;
    wk->subProcess = wk->subNextProcess;
}

int WorldTrade_GetTalkSpeed(WorldTradeWork *wk) {
    return func_02017bcc();
}

static const BGSysVRAMConfig sWorldTradeVRAMConfig = {
    GX_VRAM_BG_128_D,   GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_256_AB, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,   GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static void WorldTrade_InitCLACT(WorldTradeWork *wk) {
    WorldTrade_VramBankSet(&sWorldTradeVRAMConfig);
    WorldTrade_InitCellActor(wk, &sWorldTradeVRAMConfig);
    WorldTrade_ShowOBJ(wk);
}

static void WorldTrade_FreeCLACT(WorldTradeWork *wk) {
    WorldTrade_FreeFieldObjData(wk);
    func_0204bcd0(wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_PLTT]);
    func_0204b98c(wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_CHAR]);
    func_0204be64(wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_CELL]);
    func_0204bcd0(wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_PLTT]);
    func_0204b98c(wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CHAR]);
    func_0204be64(wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CELL]);
    func_0204bcd0(wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT]);
    func_0204b98c(wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR]);
    func_0204be64(wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL]);
    func_0204b98c(wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR]);
    func_0204be64(wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL]);
    func_0204bf98(wk->clactUnit);
    wk->clactUnit = NULL;
    func_0204b758();
}

static void WorldTrade_ServerWaitTimeFunc(WorldTradeWork *wk) {
    if (wk->serverWaitTime != 0) {
        wk->serverWaitTime--;
    }
}

void WorldTrade_BoxPokeNumGetStart(WorldTradeWork *wk) {
    wk->boxSearchFlag = 1;
    wk->boxPokeNum = 0;
}

void WorldTrade_TimeIconAdd(WorldTradeWork *wk) {
    wk->timeWaitWork = WaitIcon_Create(GFL_VBlankGetTCBMgr(), wk->msgWin, 15, 16, HEAPID_WORLDTRADE);
}

void WorldTrade_TimeIconDel(WorldTradeWork *wk) {
    if (wk->timeWaitWork != NULL) {
        WaitIcon_Free(wk->timeWaitWork);
        wk->timeWaitWork = NULL;
        BmpWin_FlushMap(wk->msgWin);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->msgWin));
    }
}

void WorldTrade_CLACT_PosChange(ClActor *act, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(act, &pos, 0);
}

// Darkens a screen behind a menu
static void WorldTrade_SetPassive(BOOL main) {
    if (main) {
        gfxRegSetBrightnessBlend(
            REG_BLDCNT_ADDR,
            GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -7);
    } else {
        gfxRegSetBrightnessBlend(
            REG_DB_BLDCNT_ADDR,
            GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -7);
    }
}

void WorldTrade_SetPassiveKeepBG2(BOOL main) {
    if (main) {
        gfxRegSetBrightnessBlend(
            REG_BLDCNT_ADDR,
            GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -7);
    } else {
        gfxRegSetBrightnessBlend(
            REG_DB_BLDCNT_ADDR,
            GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -7);
    }
}

void WorldTrade_ClearPassive(void) {
    G2_BlendNone();
    G2S_BlendNone();
}

void WorldTrade_InitGraphics(WorldTradeWork *wk) {
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    WorldTrade_InitCLACT(wk);
    GFL_BGSysCreate(HEAPID_WORLDTRADE);
    BmpWin_InitAllocator(HEAPID_WORLDTRADE);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
        GFL_BGSysSetLCDConfig(&config);
    }
    WorldTrade_WndSetting();
    G2_BlendNone();
    G2S_BlendNone();
    if (wk->vblankTask == NULL) {
        wk->vblankTask = GFL_VBlankTCBAdd(WorldTrade_VBlankFunc, wk, 0);
    }
}

void WorldTrade_ExitGraphics(WorldTradeWork *wk) {
    if (wk->vblankTask != NULL) {
        GFL_TCBRemove(wk->vblankTask);
        wk->vblankTask = NULL;
    }
    G2_BlendNone();
    G2S_BlendNone();
    WorldTrade_FreeCLACT(wk);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

void WorldTrade_ShowFatalError(WorldTradeWork *wk) {
    func_02017884(GSYS_GetGameData(wk->param->gsys));
    func_02011d04(0x29);
}
