// The Dream World account screens, which the start menu's Game Sync settings start: they create the Game Sync account
// and show its ID, or only show the ID. The ROM doesn't name this file; the name is pdwacc_message.c's and
// pdwacc_disp.c's counterpart of gsync_state.c. Function names are ours.

#include "types.h"
#include "app/gsync.h"
#include "app/gsync/pdwacc_disp.h"
#include "app/gsync/pdwacc_message.h"
#include "constants/sound.h"
#include "gfl/nhttp_rap.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/app_taskmenu.h"
#include "system/game_data.h"
#include "system/wipe.h"

// Errors of the account screens' own, beside the server's
#define PDWACC_ERROR_SERVICE_UNAVAILABLE 0xfff1
#define PDWACC_ERROR_BAD_GATEWAY 0xfff2
#define PDWACC_ERROR_HTTP 0xfff3

// The request that creates the account, sending the save data
#define PDWACC_REQUEST_CREATE 9

typedef struct PdwAccWork PdwAccWork;
typedef void (*PdwAccStateFunc)(PdwAccWork *wk);

struct PdwAccWork {
    HeapID heapId;
    void *unk4;
    NHttpRap *http;
    PdwAccDisp *disp;
    PdwAccMessage *msg;
    AppTaskMenu *yesNo;
    u32 unk18;
    SaveControl *save;
    GameData *gameData;
    void *unk24;
    s32 profileId;
    u32 unk2C;
    PdwAccStateFunc state;
    u8 unk34[0x38];
    BOOL saving;
    u32 error;
};

static BOOL PdwAcc_CheckHttpStatus(PdwAccWork *wk, int status);
static void PdwAcc_SetState(PdwAccWork *wk, PdwAccStateFunc state);
static void PdwAcc_ChangeState(PdwAccWork *wk, PdwAccStateFunc state, int line);
static void PdwAcc_StateWaitFadeOut(PdwAccWork *wk);
static void PdwAcc_StateFadeOut(PdwAccWork *wk);
static void PdwAcc_StateWaitSavedMessage(PdwAccWork *wk);
static void PdwAcc_StateWaitSave(PdwAccWork *wk);
static void PdwAcc_StateStartSave(PdwAccWork *wk);
static void PdwAcc_StateSave(PdwAccWork *wk);
static void PdwAcc_StateIdNext(PdwAccWork *wk);
static void PdwAcc_StateIdWait(PdwAccWork *wk);
static void PdwAcc_StateShowId(PdwAccWork *wk);
static void PdwAcc_StateErrorWait(PdwAccWork *wk);
static void PdwAcc_StateError(PdwAccWork *wk);
static void PdwAcc_StateWaitCreateMessage(PdwAccWork *wk);
static void PdwAcc_StateCreateStart(PdwAccWork *wk);
static void PdwAcc_StateCreateAnswer(PdwAccWork *wk);
static void PdwAcc_StateCreateAsk(PdwAccWork *wk);
static void PdwAcc_StateStart(PdwAccWork *wk);
static void PdwAcc_StateWaitCreate(PdwAccWork *wk);
static void PdwAcc_StateCreate(PdwAccWork *wk);
static void PdwAcc_StateIdOnlyWait(PdwAccWork *wk);
static void PdwAcc_StateShowIdOnly(PdwAccWork *wk);
static void PdwAcc_OnDisconnect(void *work, int a1, int code, int error);
static BOOL PdwAcc_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PdwAcc_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PdwAcc_ProcExit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions PDWACC_PROC_FUNCTIONS = {
    PdwAcc_ProcInit,
    PdwAcc_ProcMain,
    PdwAcc_ProcExit,
};

// Returns whether the HTTP status is an error, and shows it
static BOOL PdwAcc_CheckHttpStatus(PdwAccWork *wk, int status) {
    switch (status) {
    case 503:
        wk->error = PDWACC_ERROR_SERVICE_UNAVAILABLE;
        PdwAcc_ChangeState(wk, PdwAcc_StateError, 153);
        return TRUE;
    case 502:
        wk->error = PDWACC_ERROR_BAD_GATEWAY;
        PdwAcc_ChangeState(wk, PdwAcc_StateError, 157);
        return TRUE;
    default:
        if (status >= 400) {
            wk->error = PDWACC_ERROR_HTTP;
            PdwAcc_ChangeState(wk, PdwAcc_StateError, 163);
            return TRUE;
        }
        return FALSE;
    }
}

static void PdwAcc_SetState(PdwAccWork *wk, PdwAccStateFunc state) {
    wk->state = state;
}

// Changes the state, from a line that the debug build reported
static void PdwAcc_ChangeState(PdwAccWork *wk, PdwAccStateFunc state, int line) {
    PdwAcc_SetState(wk, state);
}

static void PdwAcc_StateWaitFadeOut(PdwAccWork *wk) {
    if (GFL_WipeIsFinished()) {
        PdwAcc_ChangeState(wk, NULL, 213);
    }
}

static void PdwAcc_StateFadeOut(PdwAccWork *wk) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, 6, 1, wk->heapId);
    GFL_SndBGMFadeOut(6);
    PdwAcc_ChangeState(wk, PdwAcc_StateWaitFadeOut, 231);
}

static void PdwAcc_StateWaitSavedMessage(PdwAccWork *wk) {
    if (PdwAccMessage_IsPrintFinished(wk->msg) && GCTX_HIDGetPressedKeys()) {
        PdwAcc_ChangeState(wk, PdwAcc_StateFadeOut, 241);
    }
}

static void PdwAcc_StateWaitSave(PdwAccWork *wk) {
    int result = func_02017850(wk->gameData);

    if (result == 2 || result == 3) {
        wk->saving = FALSE;
        PdwAccMessage_PrintStream(wk->msg, 11);
        PdwAcc_ChangeState(wk, PdwAcc_StateWaitSavedMessage, 254);
    }
}

static void PdwAcc_StateStartSave(PdwAccWork *wk) {
    if (PdwAccMessage_IsPrintFinished(wk->msg)) {
        wk->saving = TRUE;
        func_0201782c(wk->gameData);
        PdwAcc_ChangeState(wk, PdwAcc_StateWaitSave, 265);
    }
}

static void PdwAcc_StateSave(PdwAccWork *wk) {
    PdwAccMessage_PrintStream(wk->msg, 6);
    PdwAccMessage_StartWaitIcon(wk->msg);
    PdwAcc_ChangeState(wk, PdwAcc_StateStartSave, 274);
}

static void PdwAcc_StateIdNext(PdwAccWork *wk) {
    if (GCTX_HIDGetPressedKeys()) {
        PdwAccMessage_ClearInfo(wk->msg);
        func_020099e8(getDreamWorldStuffAddress(wk->save), TRUE);
        PdwAcc_ChangeState(wk, PdwAcc_StateSave, 282);
    }
}

static void PdwAcc_StateIdWait(PdwAccWork *wk) {
    if (GCTX_HIDGetPressedKeys()) {
        PdwAccMessage_ShowGSyncId(wk->msg, wk->profileId, wk->profileId);
        PdwAccMessage_PrintInfo(wk->msg, 9);
        PdwAcc_ChangeState(wk, PdwAcc_StateIdNext, 298);
    }
}

static void PdwAcc_StateShowId(PdwAccWork *wk) {
    PdwAccMessage_ShowGSyncId(wk->msg, wk->profileId, wk->profileId);
    PdwAccMessage_PrintInfo(wk->msg, 8);
    PdwAcc_ChangeState(wk, PdwAcc_StateIdWait, 316);
}

static void PdwAcc_StateErrorWait(PdwAccWork *wk) {
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        PdwAcc_ChangeState(wk, PdwAcc_StateFadeOut, 331);
    }
}

static void PdwAcc_StateError(PdwAccWork *wk) {
    u32 msgId = wk->error + 11;

    if (wk->http != NULL) {
        NHttpRap_EndRequest(wk->http);
    }
    if (wk->error == PDWACC_ERROR_SERVICE_UNAVAILABLE) {
        msgId = 22;
    } else if (wk->error == PDWACC_ERROR_BAD_GATEWAY) {
        msgId = 23;
    } else if (wk->error == PDWACC_ERROR_HTTP) {
        msgId = 20;
    } else if (wk->error >= 11) {
        msgId = 20;
    }
    PdwAccMessage_PrintInfo(wk->msg, msgId);
    PdwAcc_ChangeState(wk, PdwAcc_StateErrorWait, 359);
}

static void PdwAcc_StateWaitCreateMessage(PdwAccWork *wk) {
    if (PdwAccMessage_IsPrintFinished(wk->msg)) {
        PdwAcc_ChangeState(wk, PdwAcc_StateCreate, 370);
    }
}

static void PdwAcc_StateCreateStart(PdwAccWork *wk) {
    PdwAccMessage_PrintStream(wk->msg, 3);
    PdwAccMessage_StartWaitIcon(wk->msg);
    PdwAcc_ChangeState(wk, PdwAcc_StateWaitCreateMessage, 379);
}

static void PdwAcc_StateCreateAnswer(PdwAccWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->yesNo)) {
        if (AppTaskMenu_GetCursorPos(wk->yesNo) == 0) {
            PdwAcc_ChangeState(wk, PdwAcc_StateCreateStart, 397);
        } else {
            PdwAcc_ChangeState(wk, PdwAcc_StateFadeOut, 400);
        }
        PdwAccMessage_ClearMessage(wk->msg);
        AppTaskMenu_Free(wk->yesNo);
        wk->yesNo = NULL;
    }
}

static void PdwAcc_StateCreateAsk(PdwAccWork *wk) {
    if (PdwAccMessage_IsPrintFinished(wk->msg)) {
        wk->yesNo = PdwAccMessage_CreateYesNo(wk->msg, PDWACC_YESNO_POS_UPPER);
        PdwAcc_ChangeState(wk, PdwAcc_StateCreateAnswer, 422);
    }
}

static void PdwAcc_StateStart(PdwAccWork *wk) {
    PdwAccMessage_PrintStream(wk->msg, 2);
    PdwAcc_ChangeState(wk, PdwAcc_StateCreateAsk, 439);
}

static void PdwAcc_StateWaitCreate(PdwAccWork *wk) {
    if (func_02042788()) {
        if (!PdwAcc_CheckHttpStatus(wk, NHttpRap_GetStatus(wk->http)) && NHttpRap_Poll(wk->http) == 0) {
            u32 *response = NHttpRap_GetAnswer(wk->http);

            PdwAccMessage_ClearMessage(wk->msg);
            if (*response == 2) {
                PdwAcc_ChangeState(wk, PdwAcc_StateShowId, 468);
            } else if (*response == 0) {
                PdwAcc_ChangeState(wk, PdwAcc_StateShowId, 471);
            } else {
                wk->error = *response;
                PdwAcc_ChangeState(wk, PdwAcc_StateError, 475);
            }
        }
    }
}

static void PdwAcc_StateCreate(PdwAccWork *wk) {
    u32 size;

    if (func_02042788()) {
        if (NHttpRap_SendRequest(PDWACC_REQUEST_CREATE, wk->http)) {
            void *data = func_02007454(wk->save, &size);

            func_ov189_021a0854(NHttpRap_GetConnection(wk->http), data, 0x80000);
            if (NHttpRap_StartRequest(wk->http) == 0) {
                PdwAcc_ChangeState(wk, PdwAcc_StateWaitCreate, 502);
            }
        }
    } else if (GCTX_HIDGetPressedKeys()) {
        PdwAcc_ChangeState(wk, PdwAcc_StateWaitCreate, 508);
    }
}

static void PdwAcc_StateIdOnlyWait(PdwAccWork *wk) {
    if (GFL_WipeIsFinished() && GCTX_HIDGetPressedKeys()) {
        PdwAccMessage_ClearGSyncId(wk->msg);
        PdwAcc_ChangeState(wk, PdwAcc_StateFadeOut, 556);
    }
}

static void PdwAcc_StateShowIdOnly(PdwAccWork *wk) {
    PdwAccMessage_ShowGSyncId(wk->msg, wk->profileId, wk->profileId);
    PdwAccMessage_PrintInfo(wk->msg, 7);
    PdwAcc_ChangeState(wk, PdwAcc_StateIdOnlyWait, 572);
}

static void PdwAcc_OnDisconnect(void *work, int a1, int code, int error) {
    PdwAccWork *wk = work;

    if (wk->http != NULL) {
        NHttpRap_EndRequest(wk->http);
        NHttpRap_Destroy(wk->http);
        wk->http = NULL;
    }
}

static BOOL PdwAcc_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PdwAccParam *pdwParam = param;
    PdwAccWork *wk;

    GFL_OvlLoad(OVERLAY_ID(189));
    wk = GFL_ProcInitSubsystem(proc, sizeof(PdwAccWork), pdwParam->heapId);
    sys_memset(wk, 0, sizeof(PdwAccWork));
    wk->heapId = pdwParam->heapId;
    wk->save = GameData_GetSaveControl(pdwParam->gameData);
    wk->gameData = pdwParam->gameData;
    wk->profileId = func_02008bdc(GetGameDataPlayerInfo(pdwParam->gameData));
    if (func_02042788()) {
        wk->http = NHttpRap_Create(pdwParam->heapId, wk->profileId, pdwParam->loginBuffer);
        DWCRap_SetErrorFunc(PdwAcc_OnDisconnect, wk);
    }
    wk->disp = PdwAccDisp_Create(wk->heapId);
    wk->msg = PdwAccMessage_Create(wk->heapId, 107);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    GFL_SndBGMPlay(SEQ_BGM_GAME_SYNC, SND_CHANNEL_MASK_ALL);
    switch (pdwParam->mode) {
    case PDWACC_MODE_CREATE:
        PdwAcc_ChangeState(wk, PdwAcc_StateStart, 630);
        break;
    case PDWACC_MODE_SHOW_ID:
        PdwAcc_ChangeState(wk, PdwAcc_StateShowIdOnly, 633);
        break;
    }
    pdwParam->disconnected = FALSE;
    return TRUE;
}

static BOOL PdwAcc_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    PdwAccParam *pdwParam = param;
    PdwAccWork *wk = work;
    PdwAccStateFunc func = wk->state;
    BOOL done = TRUE;

    if (func != NULL) {
        if (!GFL_NetErrCheck()) {
            func(wk);
        }
        done = FALSE;
    }
    if (wk->yesNo != NULL) {
        AppTaskMenu_Update(wk->yesNo);
    }
    PdwAccDisp_Main(wk->disp);
    PdwAccMessage_Main(wk->msg);
    if (GFL_WipeIsFinished()) {
        if (GFL_NetErrCheck()) {
            NHttpRap_EndRequest(wk->http);
            if (wk->saving) {
                func_02017884(wk->gameData);
                wk->saving = FALSE;
            }
            Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
            Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
            DWCRapCommon_CheckError(1, 0);
            pdwParam->disconnected = TRUE;
            done = TRUE;
        }
    } else if (GFL_NetErrCheck()) {
        done = TRUE;
    }
    return done;
}

static BOOL PdwAcc_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    PdwAccWork *wk = work;

    if (wk->yesNo != NULL) {
        AppTaskMenu_Free(wk->yesNo);
    }
    PdwAccMessage_Free(wk->msg);
    PdwAccDisp_Free(wk->disp);
    if (wk->unk4 != NULL) {
        GFL_HeapFree(wk->unk4);
    }
    if (wk->unk24 != NULL) {
        GFL_HeapFree(wk->unk24);
    }
    if (wk->http != NULL) {
        NHttpRap_Destroy(wk->http);
        wk->http = NULL;
    }
    if (func_02042788()) {
        DWCRap_SetErrorFunc(NULL, NULL);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_OvlUnload(OVERLAY_ID(189));
    return TRUE;
}
