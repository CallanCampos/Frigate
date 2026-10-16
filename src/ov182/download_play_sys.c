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
// one here. The overlay's file before this one, src/ov182/download_play_wih.c, owns the wireless work.

static void func_ov182_021ba8bc(void);
BOOL func_ov182_021ba950(void);
static void func_ov182_021bac20(void);
static void func_ov182_021bac9c(void);
static void func_ov182_021bacbc(void);
void func_ov182_021badd4(void);
static void func_ov182_021badf8(void);
static void func_ov182_021baf38(void);
static BOOL func_ov182_021baf4c(void);
static BOOL func_ov182_021baf70(void *info);
static void func_ov182_021baf9c(void);
static void func_ov182_021bafc8(u16 unk, u16 peer, u8 *data);
static void func_ov182_021bb0c0(void);
static u16 func_ov182_021bb17c(void);

// The work the parent's flow reads the helper's through, which the function that starts the flow stores
DownloadPlaySysWork *gDownloadPlaySysWork;

// Takes the work the overlay's screens file built and starts the flow out at the wireless bring-up, with the game
// the children download. The state and the state before it are cleared; the two fields that say the parent is the
// host and the session's own first word are set; the data sharing's own areas are cleared; then the game registry
// is copied into the work's own, the player count comes from it, and the console's backup pages are read into the
// copy the data sharing sends, with the two halves of that read timed and their own id timed into the work's address
void func_ov182_021ba768(DownloadPlaySysWork *loaderContext, const MBGameRegistry *registry) {
    DownloadPlaySysWork *dat;

    DownloadPlay_SetWork((DownloadPlayWork *)loaderContext);
    gDownloadPlaySysWork = (DownloadPlaySysWork *)loaderContext;
    dat = gDownloadPlaySysWork;
    dat->state = DOWNLOAD_PLAY_SYS_STATE_STOP;
    dat->prevState = DOWNLOAD_PLAY_SYS_STATE_STOP;
    dat->unk_a92 = 1;
    dat->host = 1;
    dat->timer = 0;
    func_ov182_021baf9c();
    dat->registry.romFilePathp = registry->romFilePathp;
    dat->registry.gameNamep = registry->gameNamep;
    dat->registry.gameIntroductionp = registry->gameIntroductionp;
    dat->registry.iconCharPathp = registry->iconCharPathp;
    dat->registry.iconPalettePathp = registry->iconPalettePathp;
    dat->registry.ggid = registry->ggid;
    dat->unk_a92 = registry->maxPlayerNum != 0;
    dat->maxPlayerNum = 2;
    OS_GetTick();
    DWCBm_ReadAllNtrPages(&dat->unk_a98);
    OS_GetTick();
    dat->unk_aa8 = (void *)func_ov182_021b7e6c();
}

// Maps the state the flow is in to the state it should end in, and says whether the flow may move. The states that
// have ended the parent ask to end the parent, the ones that were sending ask to end the children, and the
// remaining ones ask to end the key sharing. The overlay's screens file calls this
BOOL func_ov182_021ba81c(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    switch (dat->state) {
    case DOWNLOAD_PLAY_SYS_STATE_STOP:
    case DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA_SHARING:
    case DOWNLOAD_PLAY_SYS_STATE_RECEIVE_COMPLETE:
    case DOWNLOAD_PLAY_SYS_STATE_KEY_COMPLETE:
    case DOWNLOAD_PLAY_SYS_STATE_END_CHILDREN:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_PARENT;
        dat->timer = 0;
        return TRUE;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_CHILDREN:
    case DOWNLOAD_PLAY_SYS_STATE_SEND_DATA:
    case DOWNLOAD_PLAY_SYS_STATE_WAIT_SEND:
    case DOWNLOAD_PLAY_SYS_STATE_START_END_CONNECT:
        if (dat->state == DOWNLOAD_PLAY_SYS_STATE_WAIT_CHILDREN && dat->timer < DOWNLOAD_PLAY_SYS_TIMEOUT) {
            return FALSE;
        }
        MB_End();
        dat->state = DOWNLOAD_PLAY_SYS_STATE_RESET;
        dat->timer = 2;
        return TRUE;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_KEY;
        return TRUE;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_KEY;
        return TRUE;

    case DOWNLOAD_PLAY_SYS_STATE_END_KEY:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_PARENT;
        return TRUE;

    default:
        return FALSE;
    }
}

// Starts the parent's session: gives the wireless the game's own id, takes the id the parent's session goes under
// and advances it, copies the game data the parent's own address holds into the frame the children read, and clears
// the parent's host flag and the count of the frames it has sent
static void func_ov182_021ba8bc(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    func_ov182_021ba180(dat->registry.ggid);
    dat->state = DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS;
    dat->ids.ids.tgid = WM_GetNextTgid();
    MI_CpuCopy8(dat->unk_aa8, dat->dataShareFrame, 0x40);
    dat->host = 0;
    dat->msgCounter = 0;
    dat->ids.ids.tgid++;
}

// Brings the wireless up and moves to the state that measures the channels, once the parent is at either end of the
// session
void func_ov182_021ba918(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    if (dat->state == DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS || dat->state == DOWNLOAD_PLAY_SYS_STATE_END_CONNECT ||
        dat->state == DOWNLOAD_PLAY_SYS_STATE_END_PARENT) {
        func_ov182_021ba8bc();
        func_ov182_021ba42c();
        dat->state = DOWNLOAD_PLAY_SYS_STATE_MEASURE_CHANNEL;
    }
}

// Starts sending the game to the children, once the parent's flow is waiting to
BOOL func_ov182_021ba950(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    if (dat->state != DOWNLOAD_PLAY_SYS_STATE_SEND_DATA) {
        return FALSE;
    }

    dat->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_SEND;
    DownloadPlay_StartSending();
    return TRUE;
}

// The flow itself, one state per frame. The wireless bring-up, the parent's own flow and the data sharing's are
// stepped in their own states, and the states in between wait on the wireless with a timeout: the ones that end a
// part of the session wait for the send to take its time, and the ones that only wait for the wireless ask it for
// its state first
void func_ov182_021ba974(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    switch (dat->state) {
    case DOWNLOAD_PLAY_SYS_STATE_STOP:
    case DOWNLOAD_PLAY_SYS_STATE_START_CONNECT:
        if (dat->state == DOWNLOAD_PLAY_SYS_STATE_START_CONNECT) {
            dat->timer = 0;
            func_ov182_021ba918();
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_MEASURE_CHANNEL:
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_PARENT;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT:
        func_ov182_021bac20();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_CHILDREN:
        func_ov182_021bac9c();
        dat->state = DOWNLOAD_PLAY_SYS_STATE_SEND_DATA;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_SEND_DATA:
        dat->msgCounter++;
        func_ov182_021bacbc();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_SEND:
        func_ov182_021bacbc();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_CONNECT:
    case DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA:
    case DOWNLOAD_PLAY_SYS_STATE_WAIT_KEY:
        func_ov182_021badf8();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_CONNECT_FAILED:
        func_ov182_021baf38();
        break;

    case DOWNLOAD_PLAY_SYS_STATE_END_KEY:
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_FINISH;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_START_END_CONNECT:
        if (++dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT) {
            func_ov182_021baf4c();
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CONNECT:
        MB_End();
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_RECEIVE;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_FINISH:
        if (++dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT && func_ov182_021ba19c() == WIH_STATE_IDLE) {
            func_ov182_021ba730();
            dat->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA_SHARING;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_START_DATA_SHARING:
        func_ov182_021ba67c();
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_RECEIVE;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA_SHARING:
        if (++dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT && func_ov182_021ba19c() == WIH_STATE_IDLE) {
            func_ov182_021ba730();
            dat->state = DOWNLOAD_PLAY_SYS_STATE_RECEIVE_COMPLETE;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_START_RECEIVE:
        func_ov182_021ba67c();
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_CONNECT_KEY;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_RECEIVE:
        if (++dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT && func_ov182_021ba19c() == WIH_STATE_IDLE) {
            func_ov182_021ba730();
            dat->state = DOWNLOAD_PLAY_SYS_STATE_KEY_COMPLETE;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_DATA_SHARING_FAILED:
        func_ov182_021ba67c();
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_CONNECT;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_END_DATA_SHARING:
        if (++dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT && func_ov182_021ba19c() == WIH_STATE_IDLE) {
            func_ov182_021ba730();
            dat->state = DOWNLOAD_PLAY_SYS_STATE_END_WIRELESS;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_START_CONNECT_KEY:
        if (++dat->timer / DOWNLOAD_PLAY_SYS_TIMEOUT == 1 && dat->timer > 0x37) {
            func_ov182_021ba67c();
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_WAIT_CONNECT_KEY:
        func_ov182_021ba67c();
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_CHILDREN;
        break;

    case DOWNLOAD_PLAY_SYS_STATE_KEY_COMPLETE:
        if (++dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT && func_ov182_021ba19c() == WIH_STATE_IDLE) {
            func_ov182_021ba730();
            dat->state = DOWNLOAD_PLAY_SYS_STATE_CANCEL;
        }
        break;

    case DOWNLOAD_PLAY_SYS_STATE_END_CONNECT:
        func_ov182_021ba67c();
        break;

    default:
        break;
    }
}

// Reports the state the flow is in and whether it moved since the last call, which the overlay's screens file polls
void func_ov182_021babdc(u8 *state, u8 *changed) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    *state = dat->state;
    *changed = dat->state != dat->prevState;
    dat->prevState = dat->state;
}

// The user the parent's first child reported, which the overlay's screens file shows
const MBUserInfo *func_ov182_021bac14(void) {
    return MB_CommGetChildUser(1);
}

// Brings the wireless up from idle: it is measured if it is still, and the channel it chose is taken and the parent
// may start if it has measured. The modes this flow does not drive leave the wireless up, which is what the library
// expects of a caller that got there
static void func_ov182_021bac20(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    switch (func_ov182_021ba19c()) {
    case WIH_STATE_STOP:
        func_ov182_021ba1a8();
        break;

    case WIH_STATE_IDLE:
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT;
        break;

    case WIH_STATE_MEASURECHANNEL:
        dat->ids.ids.channel = func_ov182_021ba350();
        dat->timer = 0;
        dat->state = DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT;
        break;

    case WIH_STATE_ERROR:
        func_ov182_021ba668();
        break;

    default:
        OS_Terminate();
        break;
    }
}

// Hands the parent and the wireless their session: the game's own id and the id the session goes under
static void func_ov182_021bac9c(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    DownloadPlay_Init(dat->registry.ggid, dat->ids.ids.tgid);
}

// Steps the download-play parent, one of its states per call: it starts the parent's part of the session, waits for
// the children to arrive and to be sent the game, and ends the session with the parent's own state
static void func_ov182_021bacbc(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);
    int i;

    switch (DownloadPlay_GetState()) {
    case DOWNLOAD_PLAY_STATE_IDLE:
        DownloadPlay_Start(&dat->registry, dat->ids.ids.channel);
        break;

    case DOWNLOAD_PLAY_STATE_ENTRY:
        i = 2;
        while (i < 5) {
            if (DownloadPlay_GetChildBmp(i) != 0) {
                dat->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_CHILDREN;
                return;
            }
            i++;
        }
        if (dat->state != DOWNLOAD_PLAY_SYS_STATE_WAIT_CHILDREN || DownloadPlay_GetChildBmp(i) != 0) {
            return;
        }
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_END_CONNECT;
        break;

    case DOWNLOAD_PLAY_STATE_DATASENDING:
        if (DownloadPlay_IsBootableAll()) {
            DownloadPlay_StartRebootAll();
        }
        break;

    case DOWNLOAD_PLAY_STATE_COMPLETE:
        i = dat->state;
        if (i <= DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA || i >= DOWNLOAD_PLAY_SYS_STATE_WAIT_CONNECT ||
            DownloadPlay_GetChildBmp(3) != 0) {
            return;
        }
        dat->state = DOWNLOAD_PLAY_SYS_STATE_END_CONNECT;
        break;

    case DOWNLOAD_PLAY_STATE_CANCEL:
        DownloadPlay_Cancel();
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS;
        break;

    case DOWNLOAD_PLAY_STATE_STOP:
        if (func_ov182_021ba19c() == WIH_STATE_IDLE) {
            func_ov182_021ba730();
            dat->state = DOWNLOAD_PLAY_SYS_STATE_END_DATA_SHARING;
        }
        break;

    default:
        break;
    }
}

// Puts the flow in the state that waits on a connection, with the scan filter that keeps the children the game
// allows
void func_ov182_021badd4(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    func_ov182_021baf9c();
    func_ov182_021ba5d0(func_ov182_021baf70);
    dat->scanFilter = func_ov182_021baf70;
    dat->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_CONNECT;
}

// Waits on the wireless for the children: it asks it to connect them when it is still, and steps the data sharing
// and reads the children's frames when it is connected or exchanging. The peer report in the frame says whether the
// exchange moved on, and the code the peer's frame carries says what to do next
static void func_ov182_021badf8(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);
    u8 *frame;

    switch (func_ov182_021ba19c()) {
    case WIH_STATE_IDLE:
        func_ov182_021ba4f8(WIH_CONNECTMODE_DS_PARENT, dat->ids.ids.tgid + 1, dat->ids.ids.channel);
        return;

    case WIH_STATE_CONNECTED:
    case WIH_STATE_DATASHARING:
    case WIH_STATE_KEYSHARING:
        frame = (u8 *)dat->unk_ab0;
        func_ov182_021bafc8(*(u16 *)frame, *((u16 *)frame + 1), frame + 4);
        func_ov182_021bb0c0();
        if (dat->state == DOWNLOAD_PLAY_SYS_STATE_RECEIVE_COMPLETE || dat->timer > DOWNLOAD_PLAY_SYS_TIMEOUT) {
            return;
        }
        break;

    default:
        break;
    }

    switch (func_ov182_021bb17c()) {
    case 0x10:
        break;

    case 0x20:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA;
        break;

    case 0x40:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_WAIT_KEY;
        break;

    case 0x50:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_DATA_SHARING;
        break;

    case 0xff:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_RECEIVE_COMPLETE;
        break;

    default:
        dat->state = DOWNLOAD_PLAY_SYS_STATE_DATA_SHARING_FAILED;
        break;
    }
}

// Moves the flow to the state that ends the key sharing
static void func_ov182_021baf38(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    dat->state = DOWNLOAD_PLAY_SYS_STATE_END_KEY;
}

// Ends the session's wireless once it is still, and moves the flow back to the state that brings it up
static BOOL func_ov182_021baf4c(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);

    if (func_ov182_021ba19c() != WIH_STATE_IDLE) {
        return FALSE;
    }

    func_ov182_021ba730();
    dat->state = DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS;
    return TRUE;
}

// The scan filter the flow installs: the child that connected is looked up by the MAC address its callback carries,
// and the player number it was given says whether it may stay and which player it is
static BOOL func_ov182_021baf70(void *info) {
    DOWNLOAD_PLAY_SYS_WORK(dat);
    WMStartParentCallback *callback = info;
    u16 playerNo;

    playerNo = DownloadPlay_GetPlayerNoByMac(callback->macAddress);
    if (playerNo == 0) {
        return FALSE;
    }

    dat->playerNo[playerNo - 1] = DownloadPlay_GetChildInfo(playerNo)->playerNo;
    return TRUE;
}

// Clears the two frame areas the data sharing exchanges, and points the frame the parent fills at the work itself
static void func_ov182_021baf9c(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);
    DownloadPlaySysWork *work = dat;

    MI_CpuFill8(dat->dataShareFrame, 0, DOWNLOAD_PLAY_SYS_FRAME_SIZE);
    MI_CpuFill8(dat->dataShareFrame + DOWNLOAD_PLAY_SYS_FRAME_SIZE, 0, DOWNLOAD_PLAY_SYS_FRAME_SIZE);
    dat->unk_ab0 = work;
}

// Fills the frame the host sends: the host writes its own two words and the game data into it, while a child writes
// the code that says it has nothing to send and the caller's own field. Then, once the wireless is sharing data,
// the flow steps the sharing and reads what each child wrote
static void func_ov182_021bafc8(u16 unk, u16 peer, u8 *data) {
    DOWNLOAD_PLAY_SYS_WORK(dat);
    u32 i;

    if (dat->host) {
        ((u16 *)dat->unk_ab0)[0] = unk;
        ((u16 *)dat->unk_ab0)[1] = peer;
        MI_CpuCopy8(data, (u8 *)dat->unk_ab0 + 4, 0x40);
    } else {
        dat->msgCounter++;
        ((u16 *)dat->unk_ab0)[0] = 0xbc;
        *((u8 *)dat->unk_ab0 + 4) = dat->unk_a92;
    }

    if (func_ov182_021ba19c() != WIH_STATE_DATASHARING) {
        return;
    }

    if (!func_ov182_021ba5fc((u8 *)dat->unk_ab0)) {
        dat->msgCounter++;
        return;
    }

    dat->msgCounter = 0;
    if (func_ov182_021ba18c() != 3) {
        dat->state = DOWNLOAD_PLAY_SYS_STATE_START_CONNECT_KEY;
        return;
    }

    for (i = 0; i < 2; i++) {
        u8 *address = func_ov182_021ba5dc(i);

        if (address != NULL) {
            MI_CpuCopy8(address, dat->dataShareFrame + DOWNLOAD_PLAY_SYS_FRAME_SIZE * i, DOWNLOAD_PLAY_SYS_FRAME_SIZE);
            dat->frameChanged[i] = 1;
        } else {
            dat->frameChanged[i] = 0;
        }
    }
}

// Reads the frames the children wrote: each of the sixteen slots that moved is examined, and the second child's
// frame decides. As the host, a frame that asks to go on is counted and the exchange moves every other one; as a
// child, a frame that says the peer is the host is taken over
static void func_ov182_021bb0c0(void) {
    DOWNLOAD_PLAY_SYS_WORK(dat);
    u32 *changed;
    int i;

    changed = dat->frameChanged;
    for (i = 0; i < DOWNLOAD_PLAY_SYS_FRAME_NO; i++) {
        u8 *frame;

        if (changed[i] == 0) {
            continue;
        }

        if (i != 1) {
            continue;
        }

        frame = dat->dataShareFrame + DOWNLOAD_PLAY_SYS_FRAME_SIZE * i;
        if (dat->host) {
            if (((u16 *)frame)[0] == 0x10) {
                dat->unk_a94++;
                if (!(dat->unk_a94 & 1)) {
                    dat->dataShareStep++;
                    if (dat->dataShareStep >= 0x24) {
                        dat->dataShareStep = 0;
                    }
                }
            }
        } else {
            ((u16 *)frame)[0] = 0xbc;
            if (((u16 *)frame)[0] == 0xbd) {
                dat->host = 1;
                dat->dataShareStep = 0;
                dat->unk_a94 = 0;
            }
        }
    }
}

// The code the peer's frame carries, which the flow reads to decide what to do next
static u16 func_ov182_021bb17c(void) {
    return gDownloadPlaySysWork->peerCode;
}
