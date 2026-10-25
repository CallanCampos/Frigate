#include "app/download_play_sys.h"
#include "dwc/bm.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// The parent's own flow for the DS Download Play range of overlay 182 (download_play_sys.c is a descriptive name: the
// ROM embeds no file name for this file). It drives the parent as a state machine: it takes the work the overlay's
// screens file hands it, starts the wireless, steps the download-play parent and the data sharing that the children
// read their records from, and ends both. Overlay 181's src/ov181/mb_parent_sys.c drives the same kind of flow over
// the MBComm_* layer with a different state machine; it is a role reference only, and no function of it is a copy of
// one here. The overlay's file before this one, lib/dwcutil/src/dwc_mov_wh.c, owns the wireless work.

static void func_ov182_021ba8bc(void);
static void func_ov182_021bac20(void);
static void func_ov182_021bac9c(void);
static void func_ov182_021bacbc(void);
static void func_ov182_021badd4(void);
static void func_ov182_021badf8(void);
static void func_ov182_021baf38(void);
static BOOL func_ov182_021baf4c(void);
static BOOL func_ov182_021baf70(WMStartParentCallback *callback);
static void func_ov182_021baf9c(void);
static void func_ov182_021bafc8(u16 code, u16 arg, const u8 *data);
static void func_ov182_021bb0c0(void);
static u16 func_ov182_021bb17c(void);

// The size of each of the two areas the data sharing's frames sit in
#define FRAME_AREA_SIZE 0x100

static DownloadPlaySysWork *sDownloadPlaySysWork;

// Takes the block the overlay's screens file built, hands the download-play parent its part, and starts the flow
// out ready: the session's ids and the timer are cleared with the frame areas and the state is set ready, the game
// registry is copied with the player count it carries, and the console's backup pages are read
void func_ov182_021ba768(DownloadPlaySysContext *context, const DownloadPlaySysGame *registry) {
    sDownloadPlaySysWork = &context->sys;
    DWCi_MOV_MBP_MemInit(&context->play);
    sDownloadPlaySysWork->tgid = 0;
    sDownloadPlaySysWork->channel = 0;
    sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_READY;
    sDownloadPlaySysWork->prevState = DOWNLOAD_PLAY_SYS_STATE_READY;
    sDownloadPlaySysWork->timer = 0;
    func_ov182_021baf9c();
    sDownloadPlaySysWork->registry.romFilePathp = registry->romFilePathp;
    sDownloadPlaySysWork->registry.gameNamep = registry->gameNamep;
    sDownloadPlaySysWork->registry.gameIntroductionp = registry->gameIntroductionp;
    sDownloadPlaySysWork->registry.iconCharPathp = registry->iconCharPathp;
    sDownloadPlaySysWork->registry.iconPalettePathp = registry->iconPalettePathp;
    sDownloadPlaySysWork->registry.ggid = registry->ggid;
    sDownloadPlaySysWork->playerCount = registry->maxPlayerNum;
    sDownloadPlaySysWork->registry.maxPlayerNum = 2;
    // The original reads the tick on both sides of the page read and keeps neither
    OS_GetTick();
    DWCBm_ReadAllNtrPages(sDownloadPlaySysWork->backupPages);
    OS_GetTick();
    sDownloadPlaySysWork->sendData = DWCi_SETTINGlGetMemMap();
}

// Asks the flow to end from the state it is in, and says whether it took the request. The states that ended the
// wireless end the flow, the ones of the download-play parent end the parent first, and the ones of the data sharing
// end the connection; the entry is left alone for its first six frames
BOOL func_ov182_021ba81c(void) {
    u8 state = sDownloadPlaySysWork->state;

    if (state == DOWNLOAD_PLAY_SYS_STATE_READY || state == DOWNLOAD_PLAY_SYS_STATE_ENDED_WIRELESS ||
        state == DOWNLOAD_PLAY_SYS_STATE_ENDED_PEER || state == DOWNLOAD_PLAY_SYS_STATE_ENDED_CANCEL ||
        state == DOWNLOAD_PLAY_SYS_STATE_ENDED_ERROR) {
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED;
        sDownloadPlaySysWork->endRequest = DOWNLOAD_PLAY_SYS_REQUEST_NONE;
        return TRUE;
    }

    if (state == DOWNLOAD_PLAY_SYS_STATE_ENTRY || state == DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED ||
        state == DOWNLOAD_PLAY_SYS_STATE_SENDING || state == DOWNLOAD_PLAY_SYS_STATE_CHILD_LEFT) {
        if (state == DOWNLOAD_PLAY_SYS_STATE_ENTRY && sDownloadPlaySysWork->entryFrames < 6) {
            return FALSE;
        }
        MB_End();
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_END_PARENT;
        sDownloadPlaySysWork->endRequest = DOWNLOAD_PLAY_SYS_REQUEST_END;
        return TRUE;
    }

    if (state == DOWNLOAD_PLAY_SYS_STATE_CONNECT_PEER || state == DOWNLOAD_PLAY_SYS_STATE_CONNECT_ASKED) {
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_END_CONNECT;
        return TRUE;
    }

    if (state == DOWNLOAD_PLAY_SYS_STATE_DONE) {
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED;
        return TRUE;
    }

    return FALSE;
}

// Brings the wireless up under the game's id and a new session id, with the data the parent sends ready to go
static void func_ov182_021ba8bc(void) {
    DWCi_MOV_WH_SetGgid(sDownloadPlaySysWork->registry.ggid);
    sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_READY;
    sDownloadPlaySysWork->tgid = WM_GetNextTgid();
    MI_CpuCopy8(sDownloadPlaySysWork->sendData, sDownloadPlaySysWork->gameData, sizeof(sDownloadPlaySysWork->gameData));
    sDownloadPlaySysWork->host = FALSE;
    sDownloadPlaySysWork->msgCounter = 0;
    sDownloadPlaySysWork->tgid++;
}

// Starts the session from a state the flow can start from, and says whether it did
BOOL func_ov182_021ba918(void) {
    if (sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_READY ||
        sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_ENDED_CANCEL ||
        sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_ENDED_ERROR) {
        func_ov182_021ba8bc();
        DWCi_MOV_WH_Initialize();
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS;
        return TRUE;
    }

    return FALSE;
}

// Starts sending the game once a child has entered, and says whether it did; the overlay's screens file calls this
BOOL func_ov182_021ba950(void) {
    if (sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED) {
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_SENDING;
        DWCi_MOV_MBP_StartDownloadAll();
        return TRUE;
    }

    return FALSE;
}

// The flow itself, one state per frame. The states that end the wireless wait DOWNLOAD_PLAY_SYS_TIMEOUT frames
// before they ask it whether it is idle
void func_ov182_021ba974(void) {
    switch (sDownloadPlaySysWork->state) {
    case DOWNLOAD_PLAY_SYS_STATE_READY:
        if (sDownloadPlaySysWork->endRequest == DOWNLOAD_PLAY_SYS_REQUEST_RESTART) {
            sDownloadPlaySysWork->endRequest = DOWNLOAD_PLAY_SYS_REQUEST_NONE;
            func_ov182_021ba918();
        } else if (sDownloadPlaySysWork->endRequest == DOWNLOAD_PLAY_SYS_REQUEST_END) {
            sDownloadPlaySysWork->endRequest = DOWNLOAD_PLAY_SYS_REQUEST_NONE;
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS:
        func_ov182_021bac20();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT:
        func_ov182_021bac9c();
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENTRY;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_ENTRY:
        sDownloadPlaySysWork->entryFrames++;
        func_ov182_021bacbc();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED:
    case DOWNLOAD_PLAY_SYS_STATE_SENDING:
        func_ov182_021bacbc();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_START_CONNECT:
        func_ov182_021badd4();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_CONNECT:
    case DOWNLOAD_PLAY_SYS_STATE_CONNECT_PEER:
    case DOWNLOAD_PLAY_SYS_STATE_CONNECT_ASKED:
        func_ov182_021badf8();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_PEER_DONE:
        func_ov182_021baf38();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_END_PARENT:
        sDownloadPlaySysWork->timer = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PARENT;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PARENT:
        if (sDownloadPlaySysWork->timer++ > DOWNLOAD_PLAY_SYS_TIMEOUT) {
            func_ov182_021baf4c();
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_SENT:
        MB_End();
        sDownloadPlaySysWork->timer = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PEER;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_WIRELESS:
        if (sDownloadPlaySysWork->timer++ > DOWNLOAD_PLAY_SYS_TIMEOUT &&
            DWCi_MOV_WH_GetSystemState() == WIH_STATE_IDLE) {
            DWCi_MOV_WH_End();
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED_WIRELESS;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_PEER_END:
        DWCi_MOV_WH_Finalize();
        sDownloadPlaySysWork->timer = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PEER;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PEER:
        if (sDownloadPlaySysWork->timer++ > DOWNLOAD_PLAY_SYS_TIMEOUT &&
            DWCi_MOV_WH_GetSystemState() == WIH_STATE_IDLE) {
            DWCi_MOV_WH_End();
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED_PEER;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_PEER_CANCEL:
        DWCi_MOV_WH_Finalize();
        sDownloadPlaySysWork->timer = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CANCEL;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CANCEL:
        if (sDownloadPlaySysWork->timer++ > DOWNLOAD_PLAY_SYS_TIMEOUT &&
            DWCi_MOV_WH_GetSystemState() == WIH_STATE_IDLE) {
            DWCi_MOV_WH_End();
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED_CANCEL;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_ERROR:
        DWCi_MOV_WH_Finalize();
        sDownloadPlaySysWork->timer = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_ERROR;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_ERROR:
        if (sDownloadPlaySysWork->timer++ > DOWNLOAD_PLAY_SYS_TIMEOUT) {
            if (DWCi_MOV_WH_GetSystemState() == WIH_STATE_IDLE) {
                DWCi_MOV_WH_End();
                sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED_ERROR;
            } else if (sDownloadPlaySysWork->timer % DOWNLOAD_PLAY_SYS_TIMEOUT == 1 &&
                       sDownloadPlaySysWork->timer > 55) {
                DWCi_MOV_WH_Finalize();
            }
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_END_CONNECT:
        DWCi_MOV_WH_Finalize();
        sDownloadPlaySysWork->timer = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CONNECT;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CONNECT:
        if (sDownloadPlaySysWork->timer++ > DOWNLOAD_PLAY_SYS_TIMEOUT &&
            DWCi_MOV_WH_GetSystemState() == WIH_STATE_IDLE) {
            DWCi_MOV_WH_End();
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ENDED;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_DISCONNECT:
        DWCi_MOV_WH_Finalize();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_ENDED:
        break;
    }
}

// Reports the state the flow is in and whether it moved since the last call, which the overlay's screens file polls
void func_ov182_021babdc(u8 *state, u8 *changed) {
    *state = sDownloadPlaySysWork->state;
    *changed = sDownloadPlaySysWork->state != sDownloadPlaySysWork->prevState;
    sDownloadPlaySysWork->prevState = sDownloadPlaySysWork->state;
}

// The user the parent's first child reported, which the overlay's screens file shows
const MBUserInfo *func_ov182_021bac14(void) {
    return MB_CommGetChildUser(1);
}

// Brings the wireless up: it measures the channels from idle, and the parent may start once it has the channel
static void func_ov182_021bac20(void) {
    switch (DWCi_MOV_WH_GetSystemState()) {
    case WIH_STATE_IDLE:
        DWCi_MOV_WH_StartMeasureChannel();
        break;

    case WIH_STATE_MEASURECHANNEL:
        sDownloadPlaySysWork->channel = DWCi_MOV_WH_GetMeasureChannel();
        sDownloadPlaySysWork->entryFrames = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT;
        break;

    case WIH_STATE_STOP:
        sDownloadPlaySysWork->entryFrames = 0;
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT;
        break;

    case WIH_STATE_ERROR:
        DWCi_MOV_WH_Reset();
        break;

    case WIH_STATE_BUSY:
        break;

    default:
        OS_Terminate();
    }
}

// Hands the download-play parent the game's id and the session's
static void func_ov182_021bac9c(void) {
    DWCi_MOV_MBP_Init(sDownloadPlaySysWork->registry.ggid, sDownloadPlaySysWork->tgid);
}

// Steps the download-play parent: it starts the parent, follows the children through the entry and the download,
// reboots them once all can, and ends the wireless once the parent has stopped
static void func_ov182_021bacbc(void) {
    switch (DWCi_MOV_MBP_GetState()) {
    case DWCi_MOV_MBP_STATE_IDLE:
        DWCi_MOV_MBP_Start(&sDownloadPlaySysWork->registry, sDownloadPlaySysWork->channel);
        break;

    case DWCi_MOV_MBP_STATE_ENTRY:
        if (DWCi_MOV_MBP_GetChildBmp(MOV_MBP_BMP_ENTRY)) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED;
        } else if (DWCi_MOV_MBP_GetChildBmp(MOV_MBP_BMP_DOWNLOAD) || DWCi_MOV_MBP_GetChildBmp(MOV_MBP_BMP_BOOTABLE)) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_SENDING;
        } else if (sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED &&
                   !DWCi_MOV_MBP_GetChildBmp(MOV_MBP_BMP_ENTRY)) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_CHILD_LEFT;
        }
        break;

    case DWCi_MOV_MBP_STATE_DATASENDING:
        if (DWCi_MOV_MBP_IsBootableAll()) {
            DWCi_MOV_MBP_StartRebootAll();
        } else if ((sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_SENDING ||
                    sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_START_CONNECT) &&
                   !DWCi_MOV_MBP_GetChildBmp(MOV_MBP_BMP_DOWNLOAD)) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_SENT;
        }
        break;

    case DWCi_MOV_MBP_STATE_COMPLETE:
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_START_CONNECT;
        break;

    case DWCi_MOV_MBP_STATE_ERROR:
        DWCi_MOV_MBP_Cancel();
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_READY;
        break;

    case DWCi_MOV_MBP_STATE_STOP:
        switch (DWCi_MOV_WH_GetSystemState()) {
        case WIH_STATE_IDLE:
            DWCi_MOV_WH_End();
            break;

        case WIH_STATE_STOP:
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_FAILED;
            break;

        case WIH_STATE_BUSY:
            break;

        default:
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_FAILED;
            break;
        }
        break;
    }
}

// Clears the frame areas and puts the flow in the state that connects to the child, with the scan filter that
// takes only the children the parent knows
static void func_ov182_021badd4(void) {
    func_ov182_021baf9c();
    DWCi_MOV_WH_SetJudgeAcceptFunc(func_ov182_021baf70);
    sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_CONNECT;
}

// Connects to the child and runs the data sharing with it: it asks the wireless to connect when it is idle, and
// once connected it sends the step's data, reads the frames, gives up after 480 frames without an answer, and moves
// on by the code the peer's frame carries
static void func_ov182_021badf8(void) {
    switch (DWCi_MOV_WH_GetSystemState()) {
    case WIH_STATE_IDLE:
        DWCi_MOV_WH_ParentConnect(WIH_CONNECTMODE_DS_PARENT, sDownloadPlaySysWork->tgid + 1,
                                  sDownloadPlaySysWork->channel);
        break;

    case WIH_STATE_CONNECTED:
    case WIH_STATE_DATASHARING:
    case WIH_STATE_KEYSHARING:
        func_ov182_021bafc8(0, sDownloadPlaySysWork->dataShareStep,
                            sDownloadPlaySysWork->sendData +
                                (sDownloadPlaySysWork->dataShareStep % 16) * sizeof(sDownloadPlaySysWork->gameData));
        func_ov182_021bb0c0();
        if (sDownloadPlaySysWork->state == DOWNLOAD_PLAY_SYS_STATE_ERROR) {
            break;
        }
        if (sDownloadPlaySysWork->msgCounter > 480) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ERROR;
            break;
        }
        if (func_ov182_021bb17c() == 0x10 || func_ov182_021bb17c() == 0x20) {
            // The original reads the code once more here and drops it
            func_ov182_021bb17c();
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_CONNECT_ASKED;
        } else if (func_ov182_021bb17c() == 0x40) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_PEER_DONE;
        } else if (func_ov182_021bb17c() == 0xff) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ERROR;
        } else if (func_ov182_021bb17c() == 0x50) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_PEER_END;
        } else if (func_ov182_021bb17c() == 0x60) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_PEER_CANCEL;
        } else if (func_ov182_021bb17c() == 0x70) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ERROR;
        } else if (func_ov182_021bb17c() == 0) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_CONNECT;
        } else if (func_ov182_021bb17c() == 0xbd) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_CONNECT_PEER;
        } else {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_FAILED;
        }
        break;
    }
}

// Ends the exchange once the peer said it is done with it, which func_ov182_021ba81c then ends the flow from
static void func_ov182_021baf38(void) {
    sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_DONE;
}

// Ends the parent's wireless once it is idle and takes the flow back to ready, and says whether it did
static BOOL func_ov182_021baf4c(void) {
    if (DWCi_MOV_WH_GetSystemState() == WIH_STATE_IDLE) {
        DWCi_MOV_WH_End();
        sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_READY;
        return TRUE;
    }

    return FALSE;
}

// The scan filter the flow installs: the child that connected is looked up by its MAC address, kept by its player
// number, and let in only if the parent knows it
static BOOL func_ov182_021baf70(WMStartParentCallback *callback) {
    u16 playerNo = DWCi_MOV_MBP_GetPlayerNo(callback->macAddress);

    if (playerNo == 0) {
        return FALSE;
    }

    sDownloadPlaySysWork->childInfo[playerNo - 1] = DWCi_MOV_MBP_GetChildInfo(playerNo);
    return TRUE;
}

// Clears both frame areas and points the frame the data sharing sends at this work's own
static void func_ov182_021baf9c(void) {
    MI_CpuFill8(sDownloadPlaySysWork->recvFrames, 0, FRAME_AREA_SIZE);
    MI_CpuFill8(&sDownloadPlaySysWork->sendFrame, 0, FRAME_AREA_SIZE);
    sDownloadPlaySysWork->sendFramep = &sDownloadPlaySysWork->sendFrame;
}

// Fills the frame this console sends and steps the data sharing: the host sends the code, its word and the data, a
// child the code that answers and its player count. A frame the sharing didn't take counts four, and once it is
// taken the frames of both consoles are read
static void func_ov182_021bafc8(u16 code, u16 arg, const u8 *data) {
    u16 i;

    if (sDownloadPlaySysWork->host == TRUE) {
        sDownloadPlaySysWork->sendFramep->code = code;
        sDownloadPlaySysWork->sendFramep->arg = arg;
        MI_CpuCopy8(data, sDownloadPlaySysWork->sendFramep->data, sizeof(sDownloadPlaySysWork->sendFramep->data));
    } else {
        sDownloadPlaySysWork->msgCounter++;
        sDownloadPlaySysWork->sendFramep->code = 0xbc;
        sDownloadPlaySysWork->sendFramep->data[0] = sDownloadPlaySysWork->playerCount;
    }

    if (DWCi_MOV_WH_GetSystemState() != WIH_STATE_DATASHARING) {
        return;
    }

    if (!DWCi_MOV_WH_StepDS((u8 *)&sDownloadPlaySysWork->sendFrame)) {
        sDownloadPlaySysWork->msgCounter += 4;
        return;
    }

    if (sDownloadPlaySysWork->host == FALSE) {
        sDownloadPlaySysWork->msgCounter++;
    } else {
        sDownloadPlaySysWork->msgCounter = 0;
        if (DWCi_MOV_WH_GetBitmap() != 3) {
            sDownloadPlaySysWork->state = DOWNLOAD_PLAY_SYS_STATE_ERROR;
            return;
        }
    }

    for (i = 0; i < 2; i++) {
        u8 *address = DWCi_MOV_WH_GetSharedDataAdr(i);

        if (address != NULL) {
            MI_CpuCopy8(address, &sDownloadPlaySysWork->recvFrames[i], sizeof(DownloadPlaySysFrame));
            sDownloadPlaySysWork->frameChanged[i] = TRUE;
        } else {
            sDownloadPlaySysWork->frameChanged[i] = FALSE;
        }
    }
}

// Reads the frame the other console sent. The host counts the peer's go-on codes and moves the step every second
// one, and stops at any other code; a child answers, and takes over as the host when the peer hands it over
static void func_ov182_021bb0c0(void) {
    int i;

    // Only the second slot, the peer's, is read: the frame of any other slot is taken but never looked at
    for (i = 0; i < DOWNLOAD_PLAY_SYS_FRAME_NO; i++) {
        if (sDownloadPlaySysWork->frameChanged[i] != FALSE) {
            DownloadPlaySysFrame *frame = &sDownloadPlaySysWork->recvFrames[i];

            if (i == 1) {
                if (sDownloadPlaySysWork->host == TRUE) {
                    if (frame->code != 0x10) {
                        return;
                    }
                    sDownloadPlaySysWork->hostCount++;
                    if (!(sDownloadPlaySysWork->hostCount & 1)) {
                        sDownloadPlaySysWork->dataShareStep++;
                        if (sDownloadPlaySysWork->dataShareStep >= 36) {
                            sDownloadPlaySysWork->dataShareStep = 0;
                        }
                    }
                } else {
                    sDownloadPlaySysWork->childCode = 0xbc;
                    if (frame->code == 0xbd) {
                        sDownloadPlaySysWork->host = TRUE;
                        sDownloadPlaySysWork->dataShareStep = 0;
                        sDownloadPlaySysWork->hostCount = 0;
                    }
                }
            }
        }
    }
}

// The code the peer's frame carries
static u16 func_ov182_021bb17c(void) {
    return sDownloadPlaySysWork->recvFrames[1].code;
}
