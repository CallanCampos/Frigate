// wifilogout.c, in overlay 190: the process that logs out of the Nintendo Wi-Fi Connection. The ROM has no file name
// for it, so it is named after its process (our name)

#include "types.h"
#include "app/wifi_login.h"
#include "app/wifilogin_disp.h"
#include "app/wifilogin_message.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/app_taskmenu.h"
#include "system/bmp_menu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/wifi_error.h"

typedef struct WifiLogoutWork WifiLogoutWork;
typedef void (*WifiLogoutStateFunc)(WifiLogoutWork *work);

struct WifiLogoutWork {
    // The yes/no menu, which the logout never opens
    WifiLoginMessageMenu *menu;
    WifiLoginDisp *disp;
    WifiLoginMessage *msg;
    WifiLogoutStateFunc state;
    WifiLogoutParam *param;
    HeapID heapId;
    // Set by the network library's callback once the connection is closed
    BOOL disconnected;
    u8 unk1C[4];
};

static void WifiLogout_SetState(WifiLogoutWork *work, WifiLogoutStateFunc state);
static void WifiLogout_ChangeState(WifiLogoutWork *work, WifiLogoutStateFunc state, int line);
static BOOL WifiLogout_RunState(WifiLogoutWork *work);
static void WifiLogout_StateStartFadeIn(WifiLogoutWork *work);
static void WifiLogout_StateWaitFadeIn(WifiLogoutWork *work);
static void WifiLogout_StatePrintDisconnecting(WifiLogoutWork *work);
static void WifiLogout_StateWaitDisconnecting(WifiLogoutWork *work);
static void WifiLogout_StateDisconnect(WifiLogoutWork *work);
static void WifiLogout_StateWaitDisconnected(WifiLogoutWork *work);
static void WifiLogout_StatePrintDisconnected(WifiLogoutWork *work);
static void WifiLogout_StateWaitDisconnectedMessage(WifiLogoutWork *work);
static void WifiLogout_StateStartFadeOut(WifiLogoutWork *work);
static void WifiLogout_StateWaitFadeOut(WifiLogoutWork *work);
static void WifiLogout_OnDisconnected(void *work);
static BOOL WifiLogoutProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiLogoutProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiLogoutProc_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions WIFILOGOUT_PROC_FUNCTIONS = {
    WifiLogoutProc_Init,
    WifiLogoutProc_Main,
    WifiLogoutProc_Exit,
};

static BOOL WifiLogoutProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WifiLogoutParam *logoutParam = param;
    WifiLogoutWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFILOGOUT, 0x18000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiLogoutWork), HEAPID_WIFILOGOUT);
    sys_memset(wk, 0, sizeof(WifiLogoutWork));
    wk->param = logoutParam;
    wk->heapId = HEAPID_WIFILOGOUT;
    if (logoutParam->unk10 == 0) {
        logoutParam->unk10 = 6;
    }
    wk->disp = WifiLoginDisp_Create(wk->heapId, logoutParam->unk4, logoutParam->unk8);
    wk->msg = WifiLoginMessage_Create(wk->heapId, TEXT_BANK_WIFI_ERROR, logoutParam->unk8);
    WifiLogout_ChangeState(wk, WifiLogout_StateStartFadeIn, 173);
    if (logoutParam->unk4 == 0) {
        WifiLoginMessage_PrintTitle(wk->msg, WifiError_Text_DisconnectFromNintendoWfc);
        if (logoutParam->unkC == 0) {
            GFL_SndBGMPlay(SEQ_BGM_WIFI_ACCESS, SND_CHANNEL_MASK_ALL);
        }
    }
    if (logoutParam->unk8 == 0) {
        func_02042ba8(FALSE, wk->heapId);
    } else {
        func_02042ba8(TRUE, wk->heapId);
    }
    return TRUE;
}

static BOOL WifiLogoutProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiLogoutParam *logoutParam = param;
    WifiLogoutWork *wk = work;

    if (logoutParam->unk4 == 0) {
        WifiLoginMessage_ClearTitle(wk->msg);
    }
    WifiLoginMessage_Free(wk->msg);
    WifiLoginDisp_Free(wk->disp);
    func_02012144();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFILOGOUT);
    return TRUE;
}

static BOOL WifiLogoutProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WifiLogoutWork *wk = work;
    BOOL done;

    if (wk->menu != NULL) {
        WifiLoginMessage_UpdateMenu(wk->menu);
    }
    WifiLoginDisp_Main(wk->disp);
    WifiLoginMessage_Update(wk->msg);
    done = FALSE;
    if (WifiLogout_RunState(wk)) {
        done = TRUE;
    }
    return done;
}

static void WifiLogout_SetState(WifiLogoutWork *work, WifiLogoutStateFunc state) {
    work->state = state;
}

// Every state change gives the line it is on
static void WifiLogout_ChangeState(WifiLogoutWork *work, WifiLogoutStateFunc state, int line) {
    WifiLogout_SetState(work, state);
}

// Runs the state, returning whether it has ended
static BOOL WifiLogout_RunState(WifiLogoutWork *work) {
    if (work->state != NULL) {
        work->state(work);
        return FALSE;
    }
    return TRUE;
}

static void WifiLogout_StateStartFadeIn(WifiLogoutWork *work) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN,
                (work->param->unk10 & 8) ? WIPE_COLOR_WHITE : WIPE_COLOR_BLACK, 6, 1, work->heapId);
    WifiLogout_ChangeState(work, WifiLogout_StateWaitFadeIn, 352);
}

static void WifiLogout_StateWaitFadeIn(WifiLogoutWork *work) {
    if (GFL_WipeIsFinished()) {
        WifiLogout_ChangeState(work, WifiLogout_StatePrintDisconnecting, 366);
    }
}

static void WifiLogout_StatePrintDisconnecting(WifiLogoutWork *work) {
    WifiLoginMessage_PrintWaiting(work->msg, WifiError_Text_DisconnectingFromNintendoWi);
    WifiLogout_ChangeState(work, WifiLogout_StateWaitDisconnecting, 409);
}

static void WifiLogout_StateWaitDisconnecting(WifiLogoutWork *work) {
    if (WifiLoginMessage_IsDone(work->msg)) {
        WifiLogout_ChangeState(work, WifiLogout_StateDisconnect, 423);
    }
}

static void WifiLogout_StateDisconnect(WifiLogoutWork *work) {
    if (GFL_NetErrCheck()) {
        func_020428a0();
        func_020429f0();
        func_02042478();
        func_020424e4();
        func_02012144();
    }
    if (func_020427a4()) {
        WifiLogout_ChangeState(work, WifiLogout_StatePrintDisconnected, 452);
    } else if (func_02042860(WifiLogout_OnDisconnected)) {
        WifiLogout_ChangeState(work, WifiLogout_StateWaitDisconnected, 460);
    }
}

static void WifiLogout_StateWaitDisconnected(WifiLogoutWork *work) {
    if (work->disconnected) {
        WifiLogout_ChangeState(work, WifiLogout_StatePrintDisconnected, 477);
    }
}

static void WifiLogout_StatePrintDisconnected(WifiLogoutWork *work) {
    WifiLoginMessage_PrintWaiting(work->msg, WifiError_Text_HaveBeenDisconnectedFrom);
    WifiLogout_ChangeState(work, WifiLogout_StateWaitDisconnectedMessage, 491);
}

static void WifiLogout_StateWaitDisconnectedMessage(WifiLogoutWork *work) {
    if (WifiLoginMessage_IsDone(work->msg)) {
        WifiLogout_ChangeState(work, WifiLogout_StateStartFadeOut, 505);
    }
}

static void WifiLogout_StateStartFadeOut(WifiLogoutWork *work) {
    u32 color = (work->param->unk10 & 0x10) ? WIPE_COLOR_WHITE : WIPE_COLOR_BLACK;

    if (work->param->unkC == 0) {
        GFL_SndBGMFadeOut(6);
    }
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, color, 6, 1, work->heapId);
    WifiLogout_ChangeState(work, WifiLogout_StateWaitFadeOut, 536);
}

static void WifiLogout_StateWaitFadeOut(WifiLogoutWork *work) {
    if (GFL_WipeIsFinished()) {
        WifiLogout_ChangeState(work, NULL, 550);
    }
}

static void WifiLogout_OnDisconnected(void *work) {
    WifiLogoutWork *wk = work;

    wk->disconnected = TRUE;
}
