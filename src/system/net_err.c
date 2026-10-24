#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/dwc_rap.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "gfl/wih.h"
#include "gfl/wm_icon.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "system/dsi.h"
#include "system/gf_font.h"
#include "system/main.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/startmenu_gfl_net_err_disp_message.h"
#include "text/system/wifi_error.h"

// The network error screen: a window on BG1 of the main screen with the error's message, until A or B is pressed, over
// whatever the game was showing, which it saves and puts back. A fatal error stops the game on the screen instead.
// The file's name is the ROM's string, from GFL_HeapAllocate's calls. The names of the functions are swan's

enum {
    ERROR_WINDOW_NET,
    ERROR_WINDOW_ASSERT,
    ERROR_WINDOW_COUNT,
};

// The size of the heap for the network, and of what the screen saves of BG1: its characters, screen and palette
#define NET_HEAP_SIZE 0x4920
#define SAVED_CHAR_SIZE 0x4000
#define SAVED_SCREEN_SIZE 0x800
#define SAVED_PLTT_SIZE 0x20

// The text area: the tiles from FIRST_TEXT_TILE, laid out in rows of the screen from TEXT_TOP, TEXT_LEFT
#define TILE_SIZE 0x20
#define FIRST_TEXT_TILE 0x20
#define TEXT_LEFT 1
#define TEXT_TOP 4
#define TEXT_WIDTH 30
#define TEXT_HEIGHT 16
#define TEXT_BACKGROUND 7

// The keys that close the screen
#define CLOSE_KEYS (PAD_BUTTON_A | PAD_BUTTON_B)

// The messages a Wi-Fi error can have without a table: the largest is NET_ERR_MESSAGE_TABLE_MAX - 1
#define NET_ERR_MESSAGE_TABLE_MAX 16

// The state of the error: none, found, and ready to be shown
enum {
    NET_ERR_STATE_NONE,
    NET_ERR_STATE_FOUND,
    NET_ERR_STATE_SHOW,
};

// What GFL_NetErrCheck tells the game to do about the error
enum {
    NET_ERR_CHECK_NONE,
    NET_ERR_CHECK_RETRY,
    NET_ERR_CHECK_END,
};

// A network type that only the error screen has: an assertion failed
#define NET_ERR_TYPE_ASSERT 0x60

// The first of the types of GFLNetErrorInfo that have a message of their own
#define NET_ERR_INFO_TYPE_FIRST 0x3ee

// The buttons and keys that PAD_Read reports, X and Y among them
#define PAD_ALL_MASK 0x2fff

// NitroSDK's GXDispCnt, the display control register's fields, with GX_GetDispCnt, GXS_GetDispCnt and
// GXS_GetVisibleWnd, and the readers of the OBJ window's planes, which nitro/gx.h lacks
typedef union {
    u32 raw;
    struct {
        u32 bgMode : 3;
        u32 bg0_2d3d : 1;
        u32 objMapChar : 1;
        u32 objMapBmp : 2;
        u32 blankScr : 1;
        u32 visiblePlane : 5;
        u32 visibleWnd : 3;
        u32 dispMode : 4;
        u32 extObjMapChar : 2;
        u32 extObjMapBmp : 1;
        u32 hBlankObjProc : 1;
        u32 bgCharOffset : 3;
        u32 bgScrOffset : 3;
        u32 bgExtPltt : 1;
        u32 objExtPltt : 1;
    };
} GXDispCnt;

static inline GXDispCnt GX_GetDispCnt(void) {
    return *(volatile GXDispCnt *)&reg_GX_DISPCNT;
}

static inline GXDispCnt GXS_GetDispCnt(void) {
    return *(volatile GXDispCnt *)&reg_GXS_DB_DISPCNT;
}

static inline int GXS_GetVisibleWnd(void) {
    return (reg_GXS_DB_DISPCNT & (REG_GX_DISPCNT_W0_MASK | REG_GX_DISPCNT_W1_MASK | REG_GX_DISPCNT_OW_MASK)) >>
           REG_GX_DISPCNT_W0_SHIFT;
}

static inline GXWndPlane G2_GetWndOBJInsidePlane(void) {
    return *(volatile GXWndPlane *)((u32)&reg_G2_WINOUT + 1);
}

static inline GXWndPlane G2S_GetWndOBJInsidePlane(void) {
    return *(volatile GXWndPlane *)((u32)&reg_G2S_DB_WINOUT + 1);
}

// NitroSDK's PAD_Read, as in key.c
static inline u16 PAD_Read(void) {
    return (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ PAD_ALL_MASK) & PAD_ALL_MASK);
}

// The keys held, up cancelling down and left cancelling right, as key.c reads them
static inline u16 ReadKeys(void) {
    u16 keys = PAD_Read();

    keys = keys & ~((keys & PAD_KEY_UP) << 1) & ~((keys & PAD_KEY_LEFT) >> 1);
    return keys;
}

typedef struct {
    // What BG1 of the main screen had before the error screen
    void *charBuf;
    void *screenBuf;
    void *plttBuf;
    GXDispCnt dispCnt;
    GXBg01Control bg1Control;
    s32 brightness;
    s32 subBrightness;
    s32 hBlankIrq;
    s32 vBlankIrq;
    u32 bgBanks;
    u32 subBgBanks;
    // A NET_ERR_STATE_*
    int state;
    u32 unk30;
    u16 blend;
    u16 subBlend;
    u32 subEnabledBGs;
    int subVisibleWnd;
    u16 subBackdrop;
    // Whether the screens stay dark after the error screen, for GFL_NetErrShowScreen
    BOOL keepDark;
    GFLNetErrorInfo info;
    u8 stateError;
    // A GFL_NET_TYPE_*, or NET_ERR_TYPE_ASSERT
    u8 netType;
    // Whether message is shown as it is, without the tables
    u8 messageGiven;
    // A NET_ERR_CHECK_*
    int check;
    // The message of a Wi-Fi error, in TEXT_BANK_WIFI_ERROR
    u32 message;
} GFLNetErrWork;

// The files of an error window's graphics
typedef struct {
    u16 arcId;
    u16 charId;
    u16 screenId;
    u16 plttId;
} ErrorWindowGraphics;

static void GFL_NetErrWaitVBlank(void);
static void GFL_NetErrAbortCore(void);
static BOOL GFL_NetErrShowCore(u32 fatal);
static void GFL_NetErrDispInit(u32 fatal, BOOL disconnecting);
static void GFL_NetErrRestore(BOOL keepDark);
static BOOL GFL_NetErrIsReady(void);
static void loadErrorBackground(int window);
static u32 GFL_NetErrGetWifiMessage(GFLNetErrorInfo *info, u32 message);
static void GFL_NetErrDispMessage(u32 fatal, BOOL disconnecting);
static void GFL_NetErrGameUpdate(void);

static const ErrorWindowGraphics FATAL_ERROR_G2D_RESIDS[ERROR_WINDOW_COUNT] = {
    { ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_CHAR, ERROR_WINDOW_GRA_SCRN, ERROR_WINDOW_GRA_PLTT },
    { ARCID_ASSERT_WINDOW_GRA, ERROR_WINDOW_GRA_CHAR, ERROR_WINDOW_GRA_SCRN, ERROR_WINDOW_GRA_PLTT },
};

// The message of each Wi-Fi error on a DS, and on a DSi, whose messages name the system's own settings
static const u16 sWifiErrorMessages[NET_ERR_MESSAGE_TABLE_MAX] = {
    WifiError_Text_UnableConnectNintendoWi,    WifiError_Text_EitherNintendoWiFi,
    WifiError_Text_NintendoWiFiConnection,     WifiError_Text_NintendoWiFiConnection_2,
    WifiError_Text_AccessPointBusyUnable,      WifiError_Text_NoAccessPointRange,
    WifiError_Text_NoCompatibleAccessPoint,    WifiError_Text_UnableConnectNintendoWi_2,
    WifiError_Text_UnableConnectNintendoWi_3,  WifiError_Text_ThereNoResponseEither,
    WifiError_Text_CommunicationErrorHaveBeen, WifiError_Text_CommunicationErrorHelpVisit,
    WifiError_Text_DownloadFailedHelpVisit,    WifiError_Text_CommunicationErrorHaveBeen_2,
    WifiError_Text_CommunicationError,         WifiError_Text_NintendoWiFiConnection_3,
};

static const u16 sWifiErrorMessagesDSi[NET_ERR_MESSAGE_TABLE_MAX] = {
    WifiError_Text_UnableConnectNintendoWi_6,  WifiError_Text_EitherNintendoWiFi,
    WifiError_Text_NintendoWiFiConnection_6,   WifiError_Text_NintendoWiFiConnection_2,
    WifiError_Text_AccessPointBusyUnable,      WifiError_Text_NoAccessPointRange,
    WifiError_Text_NoCompatibleAccessPoint_2,  WifiError_Text_UnableConnectNintendoWi_2,
    WifiError_Text_UnableConnectNintendoWi_7,  WifiError_Text_ThereNoResponseEither,
    WifiError_Text_CommunicationErrorHaveBeen, WifiError_Text_CommunicationErrorHelpVisit,
    WifiError_Text_DownloadFailedHelpVisit,    WifiError_Text_CommunicationErrorHaveBeen_2,
    WifiError_Text_CommunicationError,         WifiError_Text_NintendoWiFiConnection_7,
};

static GFLNetErrWork g_GflNetErr;

void GFL_NetHeapInit(void) {
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_NET, NET_HEAP_SIZE);
    MI_CpuFill8(&g_GflNetErr, 0, sizeof(GFLNetErrWork));
}

void GFL_NetErrInit(u32 heapId) {
    GFLNetErrWork *work = &g_GflNetErr;

    MI_CpuFill8(work, 0, sizeof(GFLNetErrWork));
    work->charBuf = GFL_HeapAllocate(heapId, SAVED_CHAR_SIZE, FALSE, "net_err.c", 188);
    work->screenBuf = GFL_HeapAllocate(heapId, SAVED_SCREEN_SIZE, FALSE, "net_err.c", 189);
    work->plttBuf = GFL_HeapAllocate(heapId, SAVED_PLTT_SIZE, FALSE, "net_err.c", 190);
}

// Finds the network's error, each frame
void GFL_NetErrUpdate(void) {
    u8 netType;

    if (g_GflNetErr.state == NET_ERR_STATE_NONE && GFL_NetIsInitialized() == TRUE) {
        if (GFL_NetHasError() || NetSystem_HasError() || NetState_GetError()) {
            GFL_NetClearCallbacks();
            GFL_NetErrRecord();
            netType = GFL_NetGetInitData()->bNetType;
            if (netType == GFL_NET_TYPE_WIFI || netType == GFL_NET_TYPE_WIFI_LOBBY ||
                netType == GFL_NET_TYPE_WIFI_GTS) {
                GFLNetErrorInfo *info = NetState_GetErrorInfo();

                if (info->type == NET_ERR_INFO_TYPE_FIRST + 2 || info->type == NET_ERR_INFO_TYPE_FIRST + 3) {
                    g_GflNetErr.message = WifiError_Text_ConnectionOtherPersonHas;
                } else if (info->type == NET_ERR_INFO_TYPE_FIRST + 4 || info->type == NET_ERR_INFO_TYPE_FIRST + 5 ||
                           info->type == NET_ERR_INFO_TYPE_FIRST + 6 || info->type == NET_ERR_INFO_TYPE_FIRST + 7 ||
                           info->type == NET_ERR_INFO_TYPE_FIRST + 8) {
                    g_GflNetErr.message = WifiError_Text_CommunicationError;
                } else {
                    g_GflNetErr.message = DWCRap_GetErrorKind(info->code, info->errorType);
                }
            }
        }
    }
}

BOOL GFL_NetErrShow(u32 a0) {
    return GFL_NetErrShowCore(a0);
}

void GFL_NetErrShowWifi(u32 message) {
    g_GflNetErr.message = message;
    g_GflNetErr.messageGiven = TRUE;
    g_GflNetErr.netType = GFL_NET_TYPE_WIFI;
    GFL_NetErrShow(TRUE);
}

void GFL_NetErrShowFatal(void) {
    GFL_NetErrShow(TRUE);
}

u32 GFL_NetErrGetServerMessage(int result) {
    switch (result) {
    case 0:
        GFL_ASSERT(0);
        return 0;
    case 1:
        return WifiError_Text_ErrorOccurredServerPlease;
    case 2:
        return WifiError_Text_ErrorOccurredServerPlease_2;
    case 3:
        return WifiError_Text_CommunicationErrorOccurredPlease;
    case 4:
        return WifiError_Text_CommunicationErrorOccurredPlease_2;
    case 5:
        return WifiError_Text_ErrorOccurredServerPlease_3;
    case 6:
        return WifiError_Text_ServiceHasBeenDiscontinued;
    case 7:
        return WifiError_Text_CommunicationErrorOccurredPlease_3;
    case 8:
        return WifiError_Text_ErrorOccurredServerPlease_4;
    case 9:
    default:
        return WifiError_Text_CommunicationErrorOccurredPlease_4;
    }
}

void GFL_NetErrSetKeepDark(BOOL keepDark) {
    g_GflNetErr.keepDark = keepDark;
}

void GFL_NetErrClearKeepDark(void) {
    GFL_NetErrSetKeepDark(FALSE);
}

// Sets the brightness the screens had before the error screen
void GFL_NetErrApplyBrightness(void) {
    GFLNetErrWork *work = &g_GflNetErr;

    GX_SetMasterBrightness(work->brightness);
    GXS_SetMasterBrightness(work->subBrightness);
}

static void GFL_NetErrWaitVBlank(void) {
    if (GFL_NetIsInitialized() == TRUE) {
        GFL_NetMainWhileWaiting();
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
}

// Shows the error screen while the network goes on, on the screen that has the wireless icon, with a window that
// leaves the icon's OBJ visible
void GFL_NetErrShowScreen(void) {
    GXDispCnt subDispCnt = GXS_GetDispCnt();
    GXWndPlane outside = G2_GetWndOutsidePlane();
    GXWndPlane subOutside = G2S_GetWndOutsidePlane();
    GXWndPlane objInside = G2_GetWndOBJInsidePlane();
    GXWndPlane subObjInside = G2S_GetWndOBJInsidePlane();

    if (!GFL_NetErrIsReady()) {
        return;
    }
    GFL_SndStop();
    GFL_NetErrDispInit(FALSE, FALSE);
    GX_VBlankIntr(TRUE);
    WmIcon_Show();
    if (WmIcon_GetEngine() == 1) {
        G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG1, FALSE);
        G2_SetWndOBJInsidePlane(GX_WND_PLANEMASK_OBJ, FALSE);
        GX_SetVisibleWnd(GX_WNDMASK_OW);
        GX_SetVisiblePlane(GX_PLANEMASK_BG1 | GX_PLANEMASK_OBJ);
    } else {
        G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG1, FALSE);
        G2S_SetWndOBJInsidePlane(GX_WND_PLANEMASK_OBJ, FALSE);
        GXS_SetVisibleWnd(GX_WNDMASK_OW);
        GXS_SetVisiblePlane(GX_PLANEMASK_BG1 | GX_PLANEMASK_OBJ);
    }
    // Wait for the keys to be released, then pressed, with the lid open
    while (PAD_DetectFold() == TRUE || (ReadKeys() & CLOSE_KEYS)) {
        GFL_NetErrGameUpdate();
        GFL_NetErrWaitVBlank();
    }
    while (PAD_DetectFold() == TRUE || !(ReadKeys() & CLOSE_KEYS)) {
        GFL_NetErrGameUpdate();
        GFL_NetErrWaitVBlank();
    }
    GFL_NetErrRestore(g_GflNetErr.keepDark == TRUE ? TRUE : FALSE);
    WmIcon_Hide();
    if (WmIcon_GetEngine() == 2) {
        GXS_SetGraphicsMode(subDispCnt.bgMode);
        GXS_SetVisiblePlane(subDispCnt.visiblePlane);
        GXS_SetVisibleWnd(subDispCnt.visibleWnd);
        G2S_SetWndOutsidePlane(subOutside.planeMask, subOutside.effect);
        G2S_SetWndOBJInsidePlane(subObjInside.planeMask, subObjInside.effect);
    } else {
        G2_SetWndOutsidePlane(outside.planeMask, outside.effect);
        G2_SetWndOBJInsidePlane(objInside.planeMask, objInside.effect);
    }
}

void GFL_NetErrAbort(void) {
    if (GFL_NetErrIsReady()) {
        GFL_NetErrAbortCore();
    }
}

// Shows the fatal error and stops the game there
static void GFL_NetErrAbortCore(void) {
    GFL_NetErrDispInit(TRUE, FALSE);
    GCTX_HIDUnblockSoftReset(2);
    while (TRUE) {
        Machine_ProcessExitRequest();
    }
}

// Records the network's error, and what the game is to do about it
void GFL_NetErrRecord(void) {
    GFLNetErrWork *work = &g_GflNetErr;

    if (GFL_NetErrIsReady() && work->state == NET_ERR_STATE_NONE) {
        work->state = NET_ERR_STATE_FOUND;
        if (!GFL_NetIsEnded()) {
            work->netType = GFL_NetGetInitData()->bNetType;
            work->info = *NetState_GetErrorInfo();
            work->stateError = NetState_GetError();
            if (work->netType == GFL_NET_TYPE_WIFI || work->netType == GFL_NET_TYPE_WIFI_LOBBY ||
                work->netType == GFL_NET_TYPE_WIFI_GTS) {
                if (work->info.errorType == 1 || work->info.errorType == 2) {
                    work->check = NET_ERR_CHECK_END;
                } else {
                    work->check = NET_ERR_CHECK_RETRY;
                }
            } else {
                work->check = NET_ERR_CHECK_END;
            }
        }
    }
}

BOOL GFL_NetErrCheck(void) {
    GFLNetErrWork *work = &g_GflNetErr;

    if (work->state != NET_ERR_STATE_NONE) {
        return work->check;
    }
    return NET_ERR_CHECK_NONE;
}

void GFL_NetErrMarkShown(void) {
    if (g_GflNetErr.state == NET_ERR_STATE_FOUND) {
        g_GflNetErr.state = NET_ERR_STATE_SHOW;
    }
}

void GFL_NetErrRecordWifi(u32 message) {
    if (g_GflNetErr.state != NET_ERR_STATE_SHOW) {
        g_GflNetErr.message = message;
        g_GflNetErr.messageGiven = TRUE;
        g_GflNetErr.netType = GFL_NET_TYPE_WIFI;
        g_GflNetErr.state = NET_ERR_STATE_SHOW;
    }
}

void AssertFailErrorDisp(void) {
    GFLNetErrWork *work = &g_GflNetErr;

    GFL_HeapFree(work->charBuf);
    GFL_HeapFree(work->screenBuf);
    GFL_HeapFree(work->plttBuf);
    GFL_HeapDelete(HEAPID_NET);
    g_GflNetErr.netType = NET_ERR_TYPE_ASSERT;
    GFL_NetErrShow(TRUE);
}

void GFL_NetErrClear(void) {
    g_GflNetErr.state = NET_ERR_STATE_NONE;
    g_GflNetErr.check = NET_ERR_CHECK_NONE;
}

BOOL GFL_NetErrShutdown(void) {
    if (GFL_NetIsInitialized()) {
        GFLNetInitData *init = GFL_NetGetInitData();

        GFL_NetCancelEnd();
        GFL_NetExit(NULL);
        GFL_NetIwFree();
        do {
            if (init != NULL &&
                (init->bNetType == GFL_NET_TYPE_WIRELESS || init->bNetType == GFL_NET_TYPE_WIRELESS_LISTEN)) {
                if (WH_GetSystemState() == 10) {
                    return FALSE;
                }
            }
            GFL_NetUpdateDevice();
            GFL_NetErrGameUpdate();
            OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        } while (!GFL_NetIsEnded());
    }
    return TRUE;
}

// Shows the error that was marked, shuts the connection down, and waits for A or B. A Wi-Fi error shows a message
// while the connection shuts down
static BOOL GFL_NetErrShowCore(u32 fatal) {
    GFLNetErrWork *work = &g_GflNetErr;
    BOOL wifi;

    if (!GFL_NetErrIsReady()) {
        return FALSE;
    }
    if (fatal == TRUE) {
        GFL_NetErrAbortCore();
    }
    if (work->state == NET_ERR_STATE_SHOW) {
        wifi = FALSE;
        if (work->netType == GFL_NET_TYPE_WIFI || work->netType == GFL_NET_TYPE_WIFI_LOBBY ||
            work->netType == GFL_NET_TYPE_WIFI_GTS) {
            wifi = TRUE;
        }
        if (wifi == TRUE) {
            GFL_NetErrDispInit(fatal, TRUE);
        }
        if (!GFL_NetErrShutdown()) {
            GFL_NetErrAbortCore();
        }
        if (wifi == TRUE) {
            work->hBlankIrq = GX_HBlankIntr(FALSE);
            work->vBlankIrq = GX_VBlankIntr(FALSE);
            GFL_NetErrDispMessage(fatal, FALSE);
        } else {
            GFL_NetErrDispInit(fatal, FALSE);
        }
        // Wait for the keys to be released, then pressed, with the lid open
        while (PAD_DetectFold() == TRUE || (ReadKeys() & CLOSE_KEYS)) {
            GFL_NetErrGameUpdate();
        }
        while (PAD_DetectFold() == TRUE || !(ReadKeys() & CLOSE_KEYS)) {
            GFL_NetErrGameUpdate();
        }
        GFL_NetErrRestore(FALSE);
        GFL_NetErrClear();
        work->unk30 = 0;
        return TRUE;
    }
    return FALSE;
}

// Saves what the screens show and shows the error window, with its message unless an assertion failed
static void GFL_NetErrDispInit(u32 fatal, BOOL disconnecting) {
    GFLNetErrWork *work = &g_GflNetErr;

    if (!GFL_NetErrIsReady()) {
        return;
    }
    work->brightness = GX_GetMasterBrightness();
    work->subBrightness = GXS_GetMasterBrightness();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    if (!disconnecting) {
        work->hBlankIrq = GX_HBlankIntr(FALSE);
        work->vBlankIrq = GX_VBlankIntr(FALSE);
    }
    work->bgBanks = GX_ResetBankForBG();
    work->subBgBanks = GX_ResetBankForSubBG();
    GX_SetBankForBG(GX_VRAM_BG_128_C);
    work->dispCnt = GX_GetDispCnt();
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
    GX_SetVisiblePlane(GX_PLANEMASK_BG1);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    work->subVisibleWnd = GXS_GetVisibleWnd();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    work->bg1Control = G2_GetBG1Control();
    G2_SetBG1Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x0000, GX_BG_CHARBASE_0x04000,
                     work->bg1Control.bgExtPltt);
    G2_BG1Mosaic(FALSE);
    G2_SetBG1Offset(0, 0);
    if (work->netType != NET_ERR_TYPE_ASSERT) {
        MI_CpuCopy16(G2_GetBG1CharPtr(), work->charBuf, SAVED_CHAR_SIZE);
        MI_CpuCopy16(G2_GetBG1ScrPtr(), work->screenBuf, SAVED_SCREEN_SIZE);
        MI_CpuCopy16((void *)HW_BG_PLTT, work->plttBuf, SAVED_PLTT_SIZE);
    }
    work->blend = reg_G2_BLDCNT;
    // BUG: the sub screen's blend control is saved over the main screen's, so GFL_NetErrRestore sets the main screen's
    // to the sub screen's and the sub screen's to subBlend, which nothing sets
#ifdef BUGFIX
    work->subBlend = reg_G2S_DB_BLDCNT;
#else
    work->blend = reg_G2S_DB_BLDCNT;
#endif
    G2_BlendNone();
    G2S_BlendNone();
    work->subEnabledBGs = GFL_BGSysGetEnabledBGsB();
    GFL_BGSysSetEnabledBGsB(0);
    work->subBackdrop = *(u16 *)HW_DB_BG_PLTT;
    *(u16 *)HW_DB_BG_PLTT = GX_RGB(10, 23, 31);
    if (g_GflNetErr.netType == NET_ERR_TYPE_ASSERT) {
        loadErrorBackground(ERROR_WINDOW_ASSERT);
    } else {
        loadErrorBackground(ERROR_WINDOW_NET);
        GFL_NetErrDispMessage(fatal, disconnecting);
    }
    GX_SetMasterBrightness(0);
    GXS_SetMasterBrightness(0);
}

// Puts back what the screens showed
static void GFL_NetErrRestore(BOOL keepDark) {
    GFLNetErrWork *work = &g_GflNetErr;
    int x;
    int y;

    if (!GFL_NetErrIsReady()) {
        return;
    }
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    *(u16 *)HW_DB_BG_PLTT = work->subBackdrop;
    GFL_BGSysSetEnabledBGsB(work->subEnabledBGs);
    reg_G2_BLDCNT = work->blend;
    reg_G2S_DB_BLDCNT = work->subBlend;
    MI_CpuCopy16(work->charBuf, G2_GetBG1CharPtr(), SAVED_CHAR_SIZE);
    MI_CpuCopy16(work->screenBuf, G2_GetBG1ScrPtr(), SAVED_SCREEN_SIZE);
    MI_CpuCopy16(work->plttBuf, (void *)HW_BG_PLTT, SAVED_PLTT_SIZE);
    G2_SetBG1Control(work->bg1Control.screenSize, work->bg1Control.colorMode, work->bg1Control.screenBase,
                     work->bg1Control.charBase, work->bg1Control.bgExtPltt);
    G2_BG1Mosaic(work->bg1Control.mosaic);
    GFL_NetGetBG1Offset(&x, &y);
    G2_SetBG1Offset(x, y);
    GX_SetGraphicsMode(work->dispCnt.dispMode, work->dispCnt.bgMode, work->dispCnt.bg0_2d3d);
    GX_SetVisiblePlane(work->dispCnt.visiblePlane);
    GX_SetVisibleWnd(work->dispCnt.visibleWnd);
    GXS_SetVisibleWnd(work->subVisibleWnd);
    GX_ResetBankForSubBG();
    GX_ResetBankForBG();
    GX_SetBankForBG(work->bgBanks);
    GX_SetBankForSubBG(work->subBgBanks);
    GX_HBlankIntr(work->hBlankIrq);
    GX_VBlankIntr(work->vBlankIrq);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    if (!keepDark) {
        GX_SetMasterBrightness(work->brightness);
        GXS_SetMasterBrightness(work->subBrightness);
    }
}

// Whether GFL_NetErrInit has run
static BOOL GFL_NetErrIsReady(void) {
    if (g_GflNetErr.charBuf != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void loadErrorBackground(int window) {
    void *file;
    NNSG2dCharacterData *chars;
    NNSG2dScreenData *screen;
    NNSG2dPaletteData *palette;
    NNSG2dPaletteCompressInfo *compressInfo;
    u32 arcId;

    MIi_CpuClear32(0, G2_GetBG1CharPtr(), SAVED_CHAR_SIZE);

    arcId = FATAL_ERROR_G2D_RESIDS[window].arcId;
    file = GFL_ArcSysReadHeapNewLZ(arcId, FATAL_ERROR_G2D_RESIDS[window].charId, FALSE, HEAPID_USER);
    if (NNS_G2dGetUnpackedBGCharacterData(file, &chars)) {
        DC_FlushRange(chars->pRawData, chars->szByte);
        MI_CpuCopy16(chars->pRawData, G2_GetBG1CharPtr(), chars->szByte);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(arcId, FATAL_ERROR_G2D_RESIDS[window].screenId, FALSE, HEAPID_USER);
    if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
        DC_FlushRange(screen->rawData, screen->szByte);
        MI_CpuCopy16(screen->rawData, G2_GetBG1ScrPtr(), screen->szByte);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(arcId, FATAL_ERROR_G2D_RESIDS[window].plttId, FALSE, HEAPID_USER);
    NNS_G2dGetUnpackedPaletteCompressInfo(file, &compressInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &palette)) {
        DC_FlushRange(palette->pRawData, SAVED_PLTT_SIZE);
        MI_CpuCopy16(palette->pRawData, (void *)HW_BG_PLTT, SAVED_PLTT_SIZE);
    }
    GFL_HeapFree(file);
}

// The message of a Wi-Fi error. The second argument is unused
static u32 GFL_NetErrGetWifiMessage(GFLNetErrorInfo *info, u32 message) {
    switch (info->type) {
    case NET_ERR_INFO_TYPE_FIRST:
    case NET_ERR_INFO_TYPE_FIRST + 9:
        return WifiError_Text_CommunicationErrorHaveBeen_2;
    case NET_ERR_INFO_TYPE_FIRST + 3:
        return WifiError_Text_ConnectionOtherPersonHas;
    case NET_ERR_INFO_TYPE_FIRST + 4:
    case NET_ERR_INFO_TYPE_FIRST + 5:
    case NET_ERR_INFO_TYPE_FIRST + 6:
    case NET_ERR_INFO_TYPE_FIRST + 7:
    case NET_ERR_INFO_TYPE_FIRST + 8:
        return WifiError_Text_CommunicationError;
    case NET_ERR_INFO_TYPE_FIRST + 2:
        return WifiError_Text_ConnectionOtherPersonHas;
    }
    if (isRunningOnDSi()) {
        if (g_GflNetErr.message >= NET_ERR_MESSAGE_TABLE_MAX) {
            return WifiError_Text_CommunicationErrorHelpVisit;
        }
        return sWifiErrorMessagesDSi[g_GflNetErr.message];
    }
    if (g_GflNetErr.message >= NET_ERR_MESSAGE_TABLE_MAX) {
        return WifiError_Text_CommunicationErrorHelpVisit;
    }
    return sWifiErrorMessages[g_GflNetErr.message];
}

// Lays the text area's tiles out on the screen, and prints the error's message in them
static void GFL_NetErrDispMessage(u32 fatal, BOOL disconnecting) {
    GFLNetErrWork *work = &g_GflNetErr;
    int x;
    int y;
    u16 *screen;
    u16 tile;
    GFLBitmap *bitmap;
    WordSet *wordSet;
    Font *font;
    MsgData *msgData;
    u32 message;
    StrBuf *src;
    StrBuf *str;

    screen = G2_GetBG1ScrPtr();
    tile = FIRST_TEXT_TILE;
    for (y = TEXT_TOP; y < TEXT_TOP + TEXT_HEIGHT; y++) {
        for (x = TEXT_LEFT; x < TEXT_LEFT + TEXT_WIDTH; x++) {
            screen[x + y * 32] = tile;
            tile++;
        }
    }
    MIi_CpuClear16(TEXT_BACKGROUND * 0x1111, (u8 *)G2_GetBG1CharPtr() + FIRST_TEXT_TILE * TILE_SIZE,
                   TEXT_WIDTH * TEXT_HEIGHT * TILE_SIZE);
    bitmap = GFL_BitmapWrapVRAM((u8 *)G2_GetBG1CharPtr() + FIRST_TEXT_TILE * TILE_SIZE, TEXT_WIDTH, TEXT_HEIGHT,
                                TILE_SIZE, HEAPID_USER);
    wordSet = GFL_WordSetSystemCreateDefault(HEAPID_USER);
    font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_USER);
    if (disconnecting == TRUE) {
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_ERROR, HEAPID_USER);
        message = WifiError_Text_CommunicationErrorDisconnectingFrom;
    } else if (work->netType == GFL_NET_TYPE_WIFI || work->netType == GFL_NET_TYPE_WIFI_LOBBY ||
               work->netType == GFL_NET_TYPE_WIFI_GTS) {
        WordSetNumber(wordSet, 0, work->info.code, 5, 2, TRUE);
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_ERROR, HEAPID_USER);
        if (work->messageGiven) {
            message = work->message;
            work->messageGiven = FALSE;
        } else {
            message = GFL_NetErrGetWifiMessage(&work->info, work->message);
        }
    } else {
        WordSetNumber(wordSet, 0, work->stateError, 5, 2, TRUE);
        msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU_GFL_NET_ERR_DISP_MESSAGE, HEAPID_USER);
        if (fatal == TRUE) {
            message = StartmenuGflNetErrDispMessage_Text_CommunicationErrorPleaseTurn;
        } else {
            message = StartmenuGflNetErrDispMessage_Text_CommunicationError;
        }
    }
    src = GFL_MsgDataLoadStrbufNew(msgData, message);
    str = GFL_StrBufCreate(0x189, HEAPID_USER);
    GFL_WordSetFormatStrbuf(wordSet, str, src);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, str, font, PRINT_COLOR(4, 11, TEXT_BACKGROUND));
    GFL_StrBufFree(str);
    GFL_StrBufFree(src);
    GFL_MsgDataFree(msgData);
    GFL_FontFree(font);
    GFL_WordSetSystemFree(wordSet);
    GFL_BitmapFree(bitmap);
}

// Keeps the game's systems running while the error screen waits
static void GFL_NetErrGameUpdate(void) {
    Machine_ProcessExitRequest();
    GCTX_HIDUpdate();
    GFL_SndUpdate();
    PokeVoice_Update();
}
