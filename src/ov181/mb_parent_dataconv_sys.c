#include "types.h"
#include "app/mb_parent.h"
#include "app/mb_parent/mb_comm_sys.h"
#include "app/mb_parent/mbp.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "gfl/wih.h"
#include "gfl/wm_icon.h"
#include "nitro/mb.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "system/text_speed.h"

// The DS Download Play parent of Unova Link's Memory Link (mb_parent_dataconv_sys.c): it sends a program to a system
// with Black or White in it and receives that game's save data. It has no screens of its own: Unova Link's network
// code starts its steps with MBDataConv_Request and polls it each frame

#define MB_DATACONV_GGID 0x1380

// A step: returns TRUE once done
typedef BOOL (*MBDataConvStep)(MBDataConv *conv, u32 *seq);

struct MBDataConv {
    HeapID heapId;
    MBCommSys *comm;
    MBGameRegistry *registry;
    u32 connectFrames;
    u32 seq;
    // Keeps the children from rebooting into the program, and cancels the distribution
    BOOL holdReboot;
    BOOL cancel;
    BOOL distributed;
    BOOL wirelessStarted;
    MBCommParentInfo parentInfo;
    MBDataConvStep step;
    u32 result;
};

static BOOL MBDataConv_Distribute(MBDataConv *conv, u32 *seq);
static BOOL MBDataConv_Connect(MBDataConv *conv, u32 *seq);
static MBGameRegistry *MBDataConv_CreateRegistry(StrBuf *name, StrBuf *intro, HeapID heapId);
static void MBDataConv_FreeRegistry(MBGameRegistry *registry);
static int MBDataConv_UpdateEntry(MBDataConv *conv);
static BOOL MBDataConv_OnWirelessDone(BOOL success);
static void MBDataConv_SoftResetCallback(void *work);

static BOOL sMBDataConvWirelessDone;

MBDataConv *MBDataConv_Create(HeapID heapId) {
    MBDataConv *conv = GFL_HeapAllocate(heapId, sizeof(MBDataConv), TRUE, "mb_parent_dataconv_sys.c", 116);

    conv->heapId = heapId;
    conv->comm = MBComm_Create(heapId);
    GCTX_HIDBlockSleep(0x10);
    return conv;
}

void MBDataConv_Delete(MBDataConv *conv) {
    GCTX_HIDUnblockSleep(0x10);
    func_0203e7dc();
    if (conv->registry != NULL) {
        MBDataConv_FreeRegistry(conv->registry);
        conv->registry = NULL;
    }
    MBComm_Delete(conv->comm);
    GFL_HeapFree(conv);
}

void MBDataConv_Update(MBDataConv *conv) {
    MBComm_Update(conv->comm);
    if (conv->step != NULL && conv->step(conv, &conv->seq)) {
        conv->step = NULL;
        conv->seq = 0;
    }
}

BOOL MBDataConv_Request(MBDataConv *conv, u32 request) {
    switch (request) {
    case MB_DATACONV_REQUEST_DISTRIBUTE:
        conv->step = MBDataConv_Distribute;
        break;
    case MB_DATACONV_REQUEST_HOLD_REBOOT:
        if (conv->step == MBDataConv_Distribute) {
            conv->holdReboot = TRUE;
        }
        break;
    case MB_DATACONV_REQUEST_CANCEL:
        if (conv->step == MBDataConv_Distribute) {
            conv->cancel = TRUE;
        }
        break;
    case MB_DATACONV_REQUEST_CONNECT:
        conv->step = MBDataConv_Connect;
        break;
    }
    return TRUE;
}

BOOL MBDataConv_IsIdle(MBDataConv *conv) {
    if (conv->step == NULL) {
        return TRUE;
    }
    return FALSE;
}

void MBDataConv_SetGameInfo(MBDataConv *conv, StrBuf *name, StrBuf *intro) {
    conv->registry = MBDataConv_CreateRegistry(name, intro, conv->heapId);
}

u32 MBDataConv_GetResult(MBDataConv *conv) {
    return conv->result;
}

void *MBDataConv_GetReceivedData(MBDataConv *conv) {
    return MBComm_GetData(conv->comm);
}

static BOOL MBDataConv_Distribute(MBDataConv *conv, u32 *seq) {
    if (conv->wirelessStarted &&
        (WH_GetSystemState() == WH_SYSSTATE_ERROR || WH_GetSystemState() == WH_SYSSTATE_FATAL)) {
        conv->distributed = FALSE;
        if (WH_Finalize() == TRUE) {
            MBP_FreeBuffers();
            WH_Release();
            return TRUE;
        }
        return FALSE;
    }
    switch (*seq) {
    case 0:
        conv->wirelessStarted = TRUE;
        conv->cancel = FALSE;
        conv->holdReboot = FALSE;
        conv->distributed = FALSE;
        GFL_OvlLoad(OVERLAY_ID(30));
        sMBDataConvWirelessDone = FALSE;
        WH_Initialize(conv->heapId, MBDataConv_OnWirelessDone, 0);
        func_0203e76c(240, 0, 0, conv->heapId);
        func_0203e810(TRUE, conv->heapId);
        GCTX_HIDSetSoftResetCallback(MBDataConv_SoftResetCallback, conv);
        (*seq)++;
        break;
    case 1:
        if (WH_GetSystemState() == WH_SYSSTATE_IDLE && WH_StartMeasureChannel() == TRUE) {
            (*seq)++;
        }
        break;
    case 2:
        if (WH_GetSystemState() == WH_SYSSTATE_MEASURECHANNEL) {
            (*seq)++;
        }
        break;
    case 3:
        MBP_Init(conv->heapId, MB_DATACONV_GGID, MB_TGID_AUTO);
        MBP_Start(conv->registry, WH_GetMeasureChannel());
        (*seq)++;
        break;
    case 4:
        switch (MBDataConv_UpdateEntry(conv)) {
        case 0:
            break;
        case 1:
            conv->distributed = TRUE;
            (*seq)++;
            break;
        case 2:
            (*seq)++;
            break;
        }
        break;
    case 5:
        WH_End(MBDataConv_OnWirelessDone);
        (*seq)++;
        break;
    case 6:
        if (sMBDataConvWirelessDone == TRUE) {
            GCTX_HIDSetSoftResetCallback(NULL, NULL);
            WH_Release();
            (*seq)++;
        }
        break;
    case 7:
        GFL_OvlUnload(OVERLAY_ID(30));
        conv->wirelessStarted = FALSE;
        return TRUE;
    }
    func_0203e7f8(WM_LINK_LEVEL_3 - func_020810fc());
    func_0203e838();
    return FALSE;
}

static BOOL MBDataConv_Connect(MBDataConv *conv, u32 *seq) {
    switch (*seq) {
    case 0:
        MBComm_StartNet(conv->comm);
        func_02042ba8(TRUE, conv->heapId);
        (*seq)++;
        break;
    case 1:
        if (MBComm_IsNetReady(conv->comm)) {
            (*seq)++;
        }
        break;
    case 2:
        MBComm_Connect(conv->comm);
        (*seq)++;
        break;
    case 3:
        if (conv->connectFrames++ >= 1800) {
            conv->result = MB_DATACONV_RESULT_TIMEOUT;
            MBComm_EndNet(conv->comm);
            *seq = 9;
        } else if (MBComm_IsConnected(conv->comm)) {
            (*seq)++;
        }
        break;
    case 4:
        conv->parentInfo.textSpeed = func_02017bcc();
        conv->parentInfo.language = GFL_MsgDataGetDefaultLangID();
        if (MBComm_SendParentInfo(conv->comm, &conv->parentInfo)) {
            (*seq)++;
        }
        break;
    case 5:
        if (MBComm_GetState(conv->comm) == 10) {
            conv->result = MB_DATACONV_RESULT_CHILD_ERROR_1;
            MBComm_StartDisconnect(conv->comm);
            *seq = 7;
        } else if (MBComm_GetState(conv->comm) == 11) {
            conv->result = MB_DATACONV_RESULT_CHILD_ERROR_2;
            MBComm_StartDisconnect(conv->comm);
            *seq = 7;
        } else if (MBComm_GetState(conv->comm) == 12) {
            conv->result = MB_DATACONV_RESULT_CHILD_ERROR_3;
            MBComm_StartDisconnect(conv->comm);
            *seq = 7;
        } else if (MBComm_IsDataReceived(conv->comm) && MBComm_SendCommand(conv->comm, MB_COMM_CMD_CLOSE, 0) == TRUE) {
            (*seq)++;
        }
        break;
    case 6:
        if (MBComm_SendCommand(conv->comm, MB_COMM_CMD_END, 0) == TRUE) {
            MBComm_StartDisconnect(conv->comm);
            (*seq)++;
        }
        break;
    case 7:
        if (MBComm_IsDisconnected(conv->comm)) {
            MBComm_EndNet(conv->comm);
            (*seq)++;
        }
        break;
    case 8:
        if (MBComm_IsNetEnded(conv->comm)) {
            (*seq)++;
        }
        break;
    case 9:
        return TRUE;
    }
    return FALSE;
}

static MBGameRegistry *MBDataConv_CreateRegistry(StrBuf *name, StrBuf *intro, HeapID heapId) {
    MBGameRegistry *registry = GFL_HeapAllocate(heapId, sizeof(MBGameRegistry), FALSE, "mb_parent_dataconv_sys.c", 530);
    u16 length;
    int i;

    registry->romFilePathp = "/dl_rom/child2_r_eng.srl";
    registry->gameNamep = GFL_HeapAllocate(heapId, 0x60, TRUE, "mb_parent_dataconv_sys.c", 537);
    length = GFL_StrBufGetCharCount(name);
    sys_memcpy16(GFL_StrBufGetStringPtr(name), registry->gameNamep, length * 2);
    registry->gameNamep[length] = 0;
    registry->gameIntroductionp = GFL_HeapAllocate(heapId, 0xc0, TRUE, "mb_parent_dataconv_sys.c", 545);
    length = GFL_StrBufGetCharCount(intro);
    sys_memcpy16(GFL_StrBufGetStringPtr(intro), registry->gameIntroductionp, length * 2);
    registry->gameIntroductionp[length] = 0;
    // The child's font has the characters of the game's charset at other codes
    for (i = 0; i < 0x60; i++) {
        if (registry->gameIntroductionp[i] == 0xfffe) {
            registry->gameIntroductionp[i] = 0xa;
        } else if (registry->gameIntroductionp[i] >= 0xff10 && registry->gameIntroductionp[i] <= 0xff19) {
            registry->gameIntroductionp[i] -= 0xfee0;
        }
    }
    for (i = 0; i < 0x30; i++) {
        if (registry->gameNamep[i] == 0xff29) {
            registry->gameNamep[i] = 0x49;
        } else if (registry->gameNamep[i] == 0xff24) {
            registry->gameNamep[i] = 0x44;
        } else if (registry->gameNamep[i] >= 0xff10 && registry->gameNamep[i] <= 0xff19) {
            registry->gameNamep[i] -= 0xfee0;
        }
    }
#ifdef BLACK2
    registry->iconCharPathp = "/dl_rom/icon_b.char";
    registry->iconPalettePathp = "/dl_rom/icon_b.plt";
#else
    registry->iconCharPathp = "/dl_rom/icon_w.char";
    registry->iconPalettePathp = "/dl_rom/icon_w.plt";
#endif
    registry->ggid = MB_DATACONV_GGID;
    registry->maxPlayerNum = 2;
    return registry;
}

static void MBDataConv_FreeRegistry(MBGameRegistry *registry) {
    GFL_HeapFree(registry->gameIntroductionp);
    GFL_HeapFree(registry->gameNamep);
    GFL_HeapFree(registry);
}

// Returns 1 once the children have booted the program, 2 once the distribution has stopped
static int MBDataConv_UpdateEntry(MBDataConv *conv) {
    switch (MBP_GetState()) {
    case MBP_STATE_ENTRY:
        if (conv->cancel == TRUE) {
            MBP_Cancel();
        } else if (conv->holdReboot == FALSE && MBP_IsBootableAll()) {
            MBP_StartRebootAll();
        }
        break;
    case MBP_STATE_COMPLETE:
        return 1;
    case MBP_STATE_ERROR:
        MBP_Cancel();
        break;
    case MBP_STATE_STOP:
        return 2;
    }
    return 0;
}

static BOOL MBDataConv_OnWirelessDone(BOOL success) {
    sMBDataConvWirelessDone = TRUE;
    return TRUE;
}

static void MBDataConv_SoftResetCallback(void *work) {
    MBDataConv *conv = work;

    if (MBP_IsStarted() == TRUE) {
        MBP_Cancel();
    }
    while (MBP_GetState() != MBP_STATE_STOP && MBP_GetState() != MBP_STATE_COMPLETE) {
        irq_waitFor(TRUE, OS_IE_V_BLANK);
    }
    if (conv->wirelessStarted == TRUE) {
        sMBDataConvWirelessDone = FALSE;
        while (WH_End(MBDataConv_OnWirelessDone)) {
            irq_waitFor(TRUE, OS_IE_V_BLANK);
        }
        while (sMBDataConvWirelessDone == FALSE) {
            irq_waitFor(TRUE, OS_IE_V_BLANK);
        }
    }
}
