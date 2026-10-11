#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/c_gear_power_onoff.h"
#include "field/subscreen.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "save/player_info.h"
#include "system/app_keycursor.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The touch screen that asks whether to turn the C-Gear on or off, named after the ROM's "c_gear_power_onoff.c". The
// mode picks the sequence: with the C-Gear off it offers to turn it on, with it on it offers to turn it off, and with
// wireless communication disabled in the system settings it only says so. Once the sequence ends, the field returns
// to subscreen 14. The names are ours

// The sequences, by the mode
#define MODE_POWER_ON 0
#define MODE_POWER_OFF 1
#define MODE_NO_WIRELESS 2

// The messages of system message file 25
#define MSG_ASK_POWER_ON 0
#define MSG_POWER_ON 1
#define MSG_ASK_POWER_OFF 2
#define MSG_EXPLANATION 3
#define MSG_NO_WIRELESS 4
#define MSG_YES 5
#define MSG_NO 6

// The yes/no menu's items
#define YESNO_YES 0

// The color the windows are cleared to
#define WINDOW_BG_COLOR 15

struct CGearPowerOnOff {
    u16 heapId;
    GameSystem *gsys;
    GameCommSys *comm;
    FieldSubscreen *subscreen;
    // The characters of BG 6 that the background takes, as GFL_BGSysLoadArcNCGRDynamic returns them
    u32 bgChars;
    // The window frame's characters
    u32 frameChars;
    // The question, at the top
    BmpWin *messageWindow;
    // The explanation below it
    BmpWin *explanationWindow;
    TCBExManager *tcbExMgr;
    // Never set, but freed with its string if it were
    PrintStream *printStream;
    StrBuf *printStr;
    u32 unk_2c;
    KeyCursor *keyCursor;
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *yesNoMenu;
    u16 mode;
    u16 seq;
    // Whether the C-Gear is being turned off, once the answer is in
    u16 powerOff;
    u16 finished;
};

typedef BOOL (*CGearPowerOnOffSeqFunc)(CGearPowerOnOff *work);

static void CGearPowerOnOff_Init(CGearPowerOnOff *work, HeapID heapId);
static void CGearPowerOnOff_Exit(CGearPowerOnOff *work);
static void CGearPowerOnOff_InitBG(CGearPowerOnOff *work, HeapID heapId);
static void CGearPowerOnOff_ExitBG(CGearPowerOnOff *work);
static void CGearPowerOnOff_CreateMenuRes(CGearPowerOnOff *work, HeapID heapId);
static void CGearPowerOnOff_FreeMenuRes(CGearPowerOnOff *work);
static void CGearPowerOnOff_CreateWindows(CGearPowerOnOff *work, HeapID heapId);
static void CGearPowerOnOff_FreeWindows(CGearPowerOnOff *work);
static void CGearPowerOnOff_PrintMessage(CGearPowerOnOff *work, BmpWin *window, u32 msgId);
static void CGearPowerOnOff_PrintMessageResized(CGearPowerOnOff *work, BmpWin *window, u32 msgId, u32 height);
static void CGearPowerOnOff_CreateYesNo(CGearPowerOnOff *work, HeapID heapId);
static void CGearPowerOnOff_FreeYesNo(CGearPowerOnOff *work);
static void CGearPowerOnOff_UpdateYesNo(CGearPowerOnOff *work);
static u8 CGearPowerOnOff_GetYesNoPos(CGearPowerOnOff *work);
static BOOL CGearPowerOnOff_IsYesNoFinished(CGearPowerOnOff *work);
static BOOL CGearPowerOnOff_SeqPowerOn(CGearPowerOnOff *work);
static BOOL CGearPowerOnOff_SeqPowerOff(CGearPowerOnOff *work);
static BOOL CGearPowerOnOff_SeqNoWireless(CGearPowerOnOff *work);

// The palette, by the player's gender
static const u32 sPaletteFileIds[2] = { 0, 1 };

static const BGSetup sBG5Setup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x00000), 0x6800,
    GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sBG6Setup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x00000), 0x6800,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sBG4Setup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x00000), 0x6800,
    GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE,
};

static CGearPowerOnOffSeqFunc sSeqFuncs[] = {
    [MODE_POWER_ON] = CGearPowerOnOff_SeqPowerOn,
    [MODE_POWER_OFF] = CGearPowerOnOff_SeqPowerOff,
    [MODE_NO_WIRELESS] = CGearPowerOnOff_SeqNoWireless,
};

// The strings are loaded when the menu is created
static AppTaskMenuItem sYesNoItems[2] = {
    { NULL, 0x39e3, 0 },
    { NULL, 0x39e3, 0 },
};

// The heap is set when the menu is created
static AppTaskMenuInit sYesNoInit = {
    0, 2, sYesNoItems, APP_TASKMENU_POS_TOP_LEFT, 21, 6, 0, 0,
};

CGearPowerOnOff *CGearPowerOnOff_Create(FieldSubscreen *subscreen, GameSystem *gsys, HeapID heapId) {
    CGearPowerOnOff *work = GFL_HeapAllocate(heapId, sizeof(CGearPowerOnOff), TRUE, "c_gear_power_onoff.c", 301);

    work->gsys = gsys;
    work->comm = GSYS_GetGameCommSystem(gsys);
    work->subscreen = subscreen;
    work->heapId = heapId;
    CGearPowerOnOff_Init(work, heapId);

    if (!isWirelessEnabled()) {
        work->mode = MODE_NO_WIRELESS;
    } else if (func_02016b34(gsys) == TRUE) {
        work->mode = MODE_POWER_OFF;
    } else {
        work->mode = MODE_POWER_ON;
    }
    CGearPowerOnOff_Update(work, TRUE);
    return work;
}

void CGearPowerOnOff_Free(CGearPowerOnOff *work) {
    CGearPowerOnOff_Exit(work);
    GFL_HeapFree(work);
}

void CGearPowerOnOff_Update(CGearPowerOnOff *work, BOOL active) {
    if (!work->finished && sSeqFuncs[work->mode](work)) {
        FieldSubscreen_SaveReturnSubscreen(work->subscreen, 14);
        work->finished = TRUE;
    }
    func_02021a3c(work->printQueue);
}

static void CGearPowerOnOff_Init(CGearPowerOnOff *work, HeapID heapId) {
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    func_020232d8();
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_S019A_CGEAR_CONTROL_WARNING, heapId);
    work->printQueue = func_02021998(heapId);
    CGearPowerOnOff_InitBG(work, heapId);
    CGearPowerOnOff_CreateMenuRes(work, heapId);
    CGearPowerOnOff_CreateWindows(work, heapId);
    func_02042ba8(FALSE, heapId);
    func_ov036_02198884(work->subscreen, heapId);
}

static void CGearPowerOnOff_Exit(CGearPowerOnOff *work) {
    func_ov036_021984f0(work->subscreen, work->heapId);
    CGearPowerOnOff_FreeWindows(work);
    CGearPowerOnOff_FreeMenuRes(work);
    CGearPowerOnOff_ExitBG(work);
    func_02021c44(work->printQueue);
    func_02021a18(work->printQueue);
    GFL_MsgDataFree(work->msgData);
    GFL_FontFree(work->font);
}

static void CGearPowerOnOff_InitBG(CGearPowerOnOff *work, HeapID heapId) {
    int bg;
    ArcTool *arc;
    u32 gender;

    for (bg = 4; bg <= 7; bg++) {
        GFL_BGSysSetBGEnabled(bg, FALSE);
    }

    GFL_BGSysCreateBG(6, &sBG6Setup, BGMODE_TEXT);
    GFL_BGSysClearScr(6);
    GFL_BGSysLoadScr(6);
    GFL_BGSysSetBGEnabled(6, TRUE);

    GFL_BGSysCreateBG(5, &sBG5Setup, BGMODE_TEXT);
    GFL_BGSysClearScr(5);
    GFL_BGSysLoadScr(5);
    GFL_BGSysSetBGEnabled(5, TRUE);

    GFL_BGSysCreateBG(4, &sBG4Setup, BGMODE_TEXT);
    GFL_BGSysClearScr(4);
    GFL_BGSysLoadScr(4);
    GFL_BGSysSetBGEnabled(4, TRUE);

    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, heapId);
    // The gender is masked to one bit
    gender = getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(work->gsys))) & 1;
    GFL_G2DIOLoadArcNCLRDefault(arc, sPaletteFileIds[gender], PALTYPE_SUB_BG, 0, 0x20, heapId);
    work->bgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 2, 6, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 3, 6, 0, (u16)work->bgChars, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static void CGearPowerOnOff_ExitBG(CGearPowerOnOff *work) {
    GFL_BGSysFreeCharMemory(6, (u16)work->bgChars, (u16)(work->bgChars >> 16));
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(4, FALSE);
}

static void CGearPowerOnOff_CreateMenuRes(CGearPowerOnOff *work, HeapID heapId) {
    work->menuRes = AppTaskMenuRes_Create(4, 3, work->font, work->printQueue, heapId);
}

static void CGearPowerOnOff_FreeMenuRes(CGearPowerOnOff *work) {
    AppTaskMenuRes_Free(work->menuRes);
}

static void CGearPowerOnOff_CreateWindows(CGearPowerOnOff *work, HeapID heapId) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x20, 0x20, heapId);
    work->frameChars = LoadCursorImageEndOfHeap(5, 2, 0, heapId);
    work->messageWindow = BmpWin_CreateDynamic(5, 1, 1, 30, 6, 1, TRUE);
    work->explanationWindow = BmpWin_CreateDynamic(5, 1, 13, 30, 10, 1, TRUE);

    BmpWin_SetHeight2(work->messageWindow, 4);
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), WINDOW_BG_COLOR);
    BmpWin_FlushChar(work->messageWindow);
    BmpWin_FlushMap(work->messageWindow);

    GFL_BitmapFill(BmpWin_GetBitmap(work->explanationWindow), WINDOW_BG_COLOR);
    BmpWin_FlushChar(work->explanationWindow);
    BmpWin_FlushMap(work->explanationWindow);

    work->tcbExMgr = GFL_TCBExMgrCreate(heapId, heapId, 1, 4);
    work->keyCursor = KeyCursor_Create(WINDOW_BG_COLOR, TRUE, TRUE, heapId);
}

static void CGearPowerOnOff_FreeWindows(CGearPowerOnOff *work) {
    if (work->printStream != NULL) {
        func_020223cc(work->printStream);
        work->printStream = NULL;
        GFL_StrBufFree(work->printStr);
        work->printStr = NULL;
    }
    KeyCursor_Free(work->keyCursor);
    GFL_TCBExMgrFree(work->tcbExMgr);
    BmpWin_Free(work->explanationWindow);
    BmpWin_Free(work->messageWindow);
    FreeCursorImageEndOfHeap(5, work->frameChars);
}

static void CGearPowerOnOff_PrintMessage(CGearPowerOnOff *work, BmpWin *window, u32 msgId) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, msgId);

    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, str, work->font);
    BmpWin_DrawFrameEndOfHeap(window, FALSE, work->frameChars, 2);
    BmpWin_FlushChar(window);
    GFL_StrBufFree(str);
}

// Prints the message in the window at a new height, clearing its old frame first
static void CGearPowerOnOff_PrintMessageResized(CGearPowerOnOff *work, BmpWin *window, u32 msgId, u32 height) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, msgId);

    BmpWin_SetHeight2(window, height);
    GFL_BitmapFill(BmpWin_GetBitmap(window), WINDOW_BG_COLOR);
    BmpWin_ClearFrame(window, 2);
    BmpWin_FlushMap(window);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, str, work->font);
    BmpWin_DrawFrameEndOfHeap(window, FALSE, work->frameChars, 2);
    BmpWin_FlushChar(window);
    GFL_StrBufFree(str);
}

static void CGearPowerOnOff_CreateYesNo(CGearPowerOnOff *work, HeapID heapId) {
    sYesNoInit.heapId = heapId;
    sYesNoItems[0].str = GFL_MsgDataLoadStrbufNew(work->msgData, MSG_YES);
    sYesNoItems[1].str = GFL_MsgDataLoadStrbufNew(work->msgData, MSG_NO);
    work->yesNoMenu = AppTaskMenu_CreateFastFlash(&sYesNoInit, work->menuRes);
    AppTaskMenu_SetLocked(work->yesNoMenu, TRUE);
    GFL_StrBufFree(sYesNoItems[0].str);
    GFL_StrBufFree(sYesNoItems[1].str);
}

static void CGearPowerOnOff_FreeYesNo(CGearPowerOnOff *work) {
    if (work->yesNoMenu != NULL) {
        AppTaskMenu_Free(work->yesNoMenu);
        work->yesNoMenu = NULL;
    }
}

static void CGearPowerOnOff_UpdateYesNo(CGearPowerOnOff *work) {
    if (work->yesNoMenu != NULL) {
        AppTaskMenu_Update(work->yesNoMenu);
    }
}

static u8 CGearPowerOnOff_GetYesNoPos(CGearPowerOnOff *work) {
    return AppTaskMenu_GetCursorPos(work->yesNoMenu);
}

static BOOL CGearPowerOnOff_IsYesNoFinished(CGearPowerOnOff *work) {
    return AppTaskMenu_IsFlashFinished(work->yesNoMenu);
}

static BOOL CGearPowerOnOff_SeqPowerOn(CGearPowerOnOff *work) {
    switch (work->seq) {
    case 0:
        CGearPowerOnOff_PrintMessage(work, work->messageWindow, MSG_ASK_POWER_ON);
        CGearPowerOnOff_PrintMessage(work, work->explanationWindow, MSG_EXPLANATION);
        CGearPowerOnOff_CreateYesNo(work, work->heapId);
        func_0201740c(GSYS_GetGameData(work->gsys), 0);
        work->seq++;
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            work->seq++;
        }
        break;
    case 2:
        if (CGearPowerOnOff_IsYesNoFinished(work)) {
            if (CGearPowerOnOff_GetYesNoPos(work) == YESNO_YES) {
                func_02016b40(work->gsys, 1);
                GSYS_TryBootGameComm(work->gsys);
                work->seq = 3;
            } else {
                work->seq = 5;
            }
            CGearPowerOnOff_FreeYesNo(work);
        }
        break;
    case 3:
        CGearPowerOnOff_PrintMessageResized(work, work->messageWindow, MSG_POWER_ON, 6);
        KeyCursor_Draw(work->keyCursor, BmpWin_GetBitmap(work->messageWindow), WINDOW_BG_COLOR);
        work->seq++;
        break;
    case 4:
        if (KeyCursor_UpdateWait(work->keyCursor, work->messageWindow) || func_02016bec(work->gsys)) {
            KeyCursor_Erase(work->keyCursor, BmpWin_GetBitmap(work->messageWindow), WINDOW_BG_COLOR);
            work->seq++;
        }
        break;
    case 5:
        if (func_02016bec(work->gsys)) {
            if (GameCommSys_BootCheck(work->comm)) {
                GameCommSys_ExitReq(work->comm);
            }
            work->seq = 6;
        } else {
            return TRUE;
        }
        break;
    case 6:
        if (!GameCommSys_BootCheck(work->comm)) {
            return TRUE;
        }
        break;
    }
    CGearPowerOnOff_UpdateYesNo(work);
    return FALSE;
}

static BOOL CGearPowerOnOff_SeqPowerOff(CGearPowerOnOff *work) {
    switch (work->seq) {
    case 0:
        CGearPowerOnOff_PrintMessage(work, work->messageWindow, MSG_ASK_POWER_OFF);
        CGearPowerOnOff_PrintMessage(work, work->explanationWindow, MSG_EXPLANATION);
        CGearPowerOnOff_CreateYesNo(work, work->heapId);
        func_0201740c(GSYS_GetGameData(work->gsys), 0);
        work->seq++;
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            work->seq++;
        }
        break;
    case 2:
        if (CGearPowerOnOff_IsYesNoFinished(work) || func_02016bec(work->gsys) == TRUE) {
            if (CGearPowerOnOff_GetYesNoPos(work) == YESNO_YES || func_02016bec(work->gsys) == TRUE) {
                if (GameCommSys_BootCheck(work->comm)) {
                    GameCommSys_ExitReq(work->comm);
                }
                work->powerOff = TRUE;
            } else {
                work->powerOff = FALSE;
            }
            work->seq = 3;
            CGearPowerOnOff_FreeYesNo(work);
        }
        break;
    case 3:
        if (work->powerOff) {
            if (GameCommSys_BootCheck(work->comm)) {
                break;
            }
            func_02016b40(work->gsys, 0);
        }
        work->seq++;
        break;
    case 4:
        return TRUE;
    }
    CGearPowerOnOff_UpdateYesNo(work);
    return FALSE;
}

static BOOL CGearPowerOnOff_SeqNoWireless(CGearPowerOnOff *work) {
    switch (work->seq) {
    case 0:
        BmpWin_ClearScreen(work->messageWindow);
        BmpWin_ClearScreen(work->explanationWindow);
        BmpWin_SetPosY(work->explanationWindow, 6);
        BmpWin_FlushChar(work->explanationWindow);
        BmpWin_FlushMap(work->explanationWindow);
        CGearPowerOnOff_PrintMessage(work, work->explanationWindow, MSG_NO_WIRELESS);
        func_0201740c(GSYS_GetGameData(work->gsys), 0);
        work->seq++;
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            work->seq++;
        }
        break;
    case 2:
        if (func_0203da48()) {
            work->seq++;
        }
        break;
    case 3:
        return TRUE;
    }
    return FALSE;
}
