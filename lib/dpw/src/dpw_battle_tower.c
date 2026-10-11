#include "dpw/dpw_bt.h"

#include "dpw_internal.h"
#include "dwc/gamestats.h"
#include "nitro/os.h"

// The Battle Subway's client, see dpw_bt.h. It works like dpw_tr.c: the states are even while a request is out and the
// answer of the server decides the result, which is 0 or more when it went well and negative otherwise. The states
// end at 0xc, which is also where a request that failed stops

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
    int group;

    switch (sDpwBtWork.state) {
    case 2:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            func_020583b0(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = Gamestats_GetBody(&size);
            sDpwBtWork.code[0] = answer[0];
            sDpwBtWork.code[1] = answer[1];
            if (size == 0) {
                sDpwBtWork.result = -2;
            } else {
                sDpwBtWork.result = sDpwBtWork.code[0];
            }
            Gamestats_Close();
            break;
        }
        break;
    case 4:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            func_020583b0(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = Gamestats_GetBody(&size);
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
            Gamestats_Close();
            break;
        }
        break;
    case 6:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            func_020583b0(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = Gamestats_GetBody(&size);
            sDpwBtWork.code[0] = answer[0];
            sDpwBtWork.code[1] = answer[1];
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
            Gamestats_Close();
            break;
        }
        break;
    case 8:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            func_020583b0(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = Gamestats_GetBody(&size);
            sDpwBtWork.code[0] = answer[0];
            sDpwBtWork.code[1] = answer[1];
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
            Gamestats_Close();
            break;
        }
        break;
    case 10:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = 0xc;
            func_020583b0(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = 0xc;
            answer = Gamestats_GetBody(&size);
            *(DpwFriendKey *)sDpwBtWork.resultData = *(const DpwFriendKey *)answer;
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
            Gamestats_Close();
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
        Gamestats_Close();
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
    Gamestats_Start(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/battletower/roomnum.asp", sDpwBtWork.pid, sDpwBtWork.request, 1, NULL) == 0) {
        sDpwBtWork.state = 2;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        Gamestats_Close();
    }
}

void DpwBt_Download(int kind, int room, void *result) {
    sDpwBtWork.resultData = result;
    sDpwBtWork.request[0] = kind - 1;
    sDpwBtWork.request[1] = room - 1;
    Gamestats_Start(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/battletower/download.asp", sDpwBtWork.pid, sDpwBtWork.request, 2, NULL) == 0) {
        sDpwBtWork.state = 4;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        Gamestats_Close();
    }
}

void DpwBt_Upload(int kind, int room, u8 win, const void *team, const void *sign, u32 id) {
    memcpy(sDpwBtUpload.team, team, 0xf0);
    sDpwBtUpload.kind = kind - 1;
    sDpwBtUpload.room = room - 1;
    sDpwBtUpload.win = win;
    sDpwBtUpload.key = sDpwBtWork.friendKey;
    memcpy(sDpwBtUpload.sign, sign, 0x80);
    sDpwBtUpload.id = id;
    *(DpwBtUpload *)sDpwBtWork.request = sDpwBtUpload;
    Gamestats_Start(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/battletower/upload.asp", sDpwBtWork.pid, sDpwBtWork.request, 0x184, NULL) == 0) {
        sDpwBtWork.state = 6;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        Gamestats_Close();
    }
}

void DpwBt_Cancel(void) {
    switch (sDpwBtWork.state) {
    case 2:
        Gamestats_Abort();
        sDpwBtWork.state = 3;
        break;
    case 4:
        Gamestats_Abort();
        sDpwBtWork.state = 5;
        break;
    case 6:
        Gamestats_Abort();
        sDpwBtWork.state = 7;
        break;
    case 8:
        Gamestats_Abort();
        sDpwBtWork.state = 9;
        break;
    case 10:
        Gamestats_Abort();
        sDpwBtWork.state = 11;
        break;
    }
}

void DpwBt_GetServerState(void) {
    Gamestats_Start(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/battletower/info.asp", sDpwBtWork.pid, sDpwBtWork.request, 0, NULL) == 0) {
        sDpwBtWork.state = 8;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        Gamestats_Close();
    }
}

void DpwBt_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result) {
    func_0207c33c(profile->unk1C);
    memcpy(sDpwBtWork.request, profile, 0x64);
    sDpwBtWork.resultData = result;
    Gamestats_Start(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/common/setProfile.asp", sDpwBtWork.pid, sDpwBtWork.request, 0x64, NULL) == 0) {
        sDpwBtWork.state = 10;
    } else {
        sDpwBtWork.state = 0xc;
        sDpwBtWork.result = -5;
        Gamestats_Close();
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
        if (func_0205b5ec()) {
            result = -6;
        }
        func_02058490();
    }
    return result;
}
