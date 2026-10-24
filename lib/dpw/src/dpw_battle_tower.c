#include "dpw/dpw_bt.h"
#include "dpw_internal.h"
#include "dwc/dwc_encsession.h"
#include "nitro/os.h"

// The Battle Subway's client, see dpw_bt.h. It works like dpw_tr.c: the states are even while a request is out and the
// answer of the server decides the result, which is 0 or more when it went well and negative otherwise. The states
// end at 0xc, which is also where a request that failed stops. As there, the copies of a few bytes go through the
// compiler's built-in memcpy (__memcpy), which copies a constant size in place

// The team that is uploaded, with what the server needs to know about it. It is built here and then copied to the
// request, the same bytes as the request holds
typedef struct DpwBtUpload {
    u8 team[0xf0];
    u8 kind;
    u8 room;
    u8 win;
    u8 unkF3[5];
    DpwFriendKey key;
    u8 sign[0x80];
    u32 id;
} DpwBtUpload;

typedef struct DpwBtWork {
    int state;
    int result;
    s32 pid;
    DpwFriendKey friendKey;
    u8 request[0x184];
    u8 code[2];
    void *resultData;
    u32 unk1A0;
} DpwBtWork;

static int DpwBtMapGamestatsError(int group);

static DpwBtWork sDpwBtWork;
static DpwBtUpload sDpwBtUpload;
static int sDpwBtServerType;

void DpwBt_Init(s32 pid, u64 friendKey, int server) {
    sDpwBtWork.state = 1;
    sDpwBtWork.result = 0;
    sDpwBtWork.pid = pid;
    sDpwBtWork.friendKey.low = (u32)friendKey;
    sDpwBtWork.friendKey.high = friendKey >> 32;
    switch (server) {
    case GAMESTATS_SERVER_LIVE:
        sDpwBtServerType = GAMESTATS_SERVER_LIVE;
        break;
    case GAMESTATS_SERVER_DEVELOPMENT:
        sDpwBtServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    case GAMESTATS_SERVER_TEST:
        sDpwBtServerType = GAMESTATS_SERVER_TEST;
        break;
    default:
        sDpwBtServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    }
}

void DpwBt_Process(void) {
    const u8 *answer;
    int size;
    int type;
    DWCErrorType group;

    switch (sDpwBtWork.state) {
    case 2:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.code, answer, sizeof(sDpwBtWork.code));
            if (size != 0) {
                sDpwBtWork.result = sDpwBtWork.code[0];
            } else {
                sDpwBtWork.result = -2;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case 4:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = DWCi_EncSessionGetResponse(&size);
            memcpy(sDpwBtWork.resultData, answer, 0xa8c);
            if (size == 0xa8c) {
                sDpwBtWork.result = 0;
            } else {
                switch (*(u8 *)sDpwBtWork.resultData) {
                case 2:
                    sDpwBtWork.result = -3;
                    break;
                case 5:
                    sDpwBtWork.result = -2;
                    break;
                default:
                    sDpwBtWork.result = -5;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case 6:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.code, answer, sizeof(sDpwBtWork.code));
            switch (sDpwBtWork.code[0]) {
            case 1:
                sDpwBtWork.result = 0;
                break;
            case 2:
                sDpwBtWork.result = -3;
                break;
            case 4:
                sDpwBtWork.result = -1;
                break;
            case 5:
                sDpwBtWork.result = -2;
                break;
            default:
                sDpwBtWork.result = -5;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case 8:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.code, answer, sizeof(sDpwBtWork.code));
            switch (sDpwBtWork.code[0]) {
            case 1:
                sDpwBtWork.result = 0;
                break;
            case 3:
                sDpwBtWork.result = 1;
                break;
            case 4:
                sDpwBtWork.result = 2;
                break;
            case 5:
                sDpwBtWork.result = -2;
                break;
            default:
                sDpwBtWork.result = -5;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case 10:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.resultData, answer, 8);
            if (size == 8) {
                sDpwBtWork.result = 0;
            } else {
                switch (*(u8 *)sDpwBtWork.resultData) {
                case 1:
                    sDpwBtWork.result = 0;
                    break;
                case 3:
                    sDpwBtWork.result = 1;
                    break;
                case 4:
                    sDpwBtWork.result = 2;
                    break;
                case 5:
                    sDpwBtWork.result = -2;
                    break;
                default:
                    sDpwBtWork.result = -5;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -4;
        DWCi_EncSessionShutdown();
        break;
    case 0:
    case 1:
    case 0xc:
        break;
    }
}

void DpwBt_End(void) {
    sDpwBtWork.state = 0;
}

BOOL DpwBt_IsFinished(void) {
    switch (sDpwBtWork.state) {
    case 1:
        return TRUE;
    case 0xc:
        sDpwBtWork.state = 1;
        return TRUE;
    default:
        return FALSE;
    }
}

s32 DpwBt_GetResult(void) {
    return sDpwBtWork.result;
}

void DpwBt_GetRoomNumber(int kind) {
    sDpwBtWork.request[0] = kind - 1;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/roomnum.asp", sDpwBtWork.pid, sDpwBtWork.request, 1, NULL) == 0) {
        sDpwBtWork.state = 2;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_Download(int kind, int room, void *result) {
    sDpwBtWork.resultData = result;
    sDpwBtWork.request[0] = kind - 1;
    sDpwBtWork.request[1] = room - 1;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/download.asp", sDpwBtWork.pid, sDpwBtWork.request, 2, NULL) == 0) {
        sDpwBtWork.state = 4;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_Upload(int kind, int room, u8 win, const void *team, const void *sign, u32 id) {
    memcpy(sDpwBtUpload.team, team, 0xf0);
    sDpwBtUpload.kind = kind - 1;
    sDpwBtUpload.room = room - 1;
    sDpwBtUpload.win = win;
    __memcpy(&sDpwBtUpload.key, &sDpwBtWork.friendKey, sizeof(sDpwBtUpload.key));
    memcpy(sDpwBtUpload.sign, sign, 0x80);
    sDpwBtUpload.id = id;
    *(DpwBtUpload *)sDpwBtWork.request = sDpwBtUpload;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/upload.asp", sDpwBtWork.pid, sDpwBtWork.request, 0x184, NULL) == 0) {
        sDpwBtWork.state = 6;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_Cancel(void) {
    switch (sDpwBtWork.state) {
    case 2:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = 3;
        break;
    case 4:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = 5;
        break;
    case 6:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = 7;
        break;
    case 8:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = 9;
        break;
    case 10:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = 11;
        break;
    }
}

void DpwBt_GetServerState(void) {
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/info.asp", sDpwBtWork.pid, sDpwBtWork.request, 0, NULL) == 0) {
        sDpwBtWork.state = 8;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result) {
    OS_GetMacAddress(profile->macAddress);
    memcpy(sDpwBtWork.request, profile, 0x64);
    sDpwBtWork.resultData = result;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/common/setProfile.asp", sDpwBtWork.pid, sDpwBtWork.request, 0x64, NULL) == 0) {
        sDpwBtWork.state = 10;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        DWCi_EncSessionShutdown();
    }
}

// The result of a failure of the game statistics client, from its group of errors
static int DpwBtMapGamestatsError(int group) {
    int result;

    switch (group) {
    case 0:
    case 1:
    case 2:
        result = -7;
        break;
    case 3:
        result = -6;
        break;
    case 5:
        result = -6;
        break;
    case 4:
        result = -7;
        break;
    case 6:
        result = -6;
        break;
    case 7:
        result = -5;
        break;
    default:
        result = -6;
        break;
    }
    if (result != -5) {
        if (DWC_UpdateConnection()) {
            result = -6;
        }
        DWC_ClearError();
    }
    return result;
}
