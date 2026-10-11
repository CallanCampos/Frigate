#include "dpw/dpw_tr.h"

#include "dpw_internal.h"
#include "dwc/gamestats.h"
#include "nitro/os.h"

// The Global Trade Station's client, see dpw_tr.h. Every call fills the request that the game statistics client sends to
// one page of the server, and DpwTr's process runs it a frame at a time. The state is even while a call waits for its
// answer, and the answer of the server (a code, and for some calls the Pokémon) ends the call with a result: 0 or more
// when it went well, and a negative number for the ways it failed. A call that could not start ends in state 0x18 with
// the result -13, and the other states after an answer or a failure of the game statistics client are 0x18 too, until
// the game asks for the end (DpwTr_IsFinished)

// The state of the library: where the call is, what it ended with, the player's ID and friend key, and the request
// that is sent: the Pokémon, the signature that goes with it and the size of the signature. The answer of the server
// starts with two codes
typedef struct DpwTrWork {
    int state;
    int result;
    s32 pid;
    DpwFriendKey friendKey;
    u8 request[0x128];
    u32 requestId;
    u8 signature[0x80];
    u32 signatureSize;
    u8 code[2];
    void *resultData;
    u32 unk1CC;
} DpwTrWork;

static int DpwTrMapGamestatsError(int group);

static int sDpwTrServerType;
static DpwTrWork sDpwTrWork;

void DpwTr_Init(s32 pid, u64 friendKey, int server) {
    sDpwTrWork.state = 1;
    sDpwTrWork.result = 0;
    sDpwTrWork.pid = pid;
    sDpwTrWork.friendKey.low = (u32)friendKey;
    sDpwTrWork.friendKey.high = friendKey >> 32;
    switch (server) {
    case GAMESTATS_SERVER_LIVE:
        sDpwTrServerType = GAMESTATS_SERVER_LIVE;
        break;
    case GAMESTATS_SERVER_DEVELOPMENT:
        sDpwTrServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    case GAMESTATS_SERVER_TEST:
        sDpwTrServerType = GAMESTATS_SERVER_TEST;
        break;
    default:
        sDpwTrServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    }
}

void DpwTr_Process(void) {
    const u8 *answer;
    int size;
    int type;
    int group;

    switch (sDpwTrWork.state) {
    case 2:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            sDpwTrWork.code[0] = answer[0];
            sDpwTrWork.code[1] = answer[1];
            switch (sDpwTrWork.code[0]) {
            case 1:
                sDpwTrWork.result = 0;
                break;
            case 2:
                sDpwTrWork.result = -5;
                break;
            case 3:
                sDpwTrWork.result = -4;
                break;
            case 7:
                sDpwTrWork.result = -1;
                break;
            case 8:
                sDpwTrWork.result = -8;
                break;
            case 9:
                sDpwTrWork.result = -9;
                break;
            case 10:
                sDpwTrWork.result = -10;
                break;
            case 11:
                sDpwTrWork.result = -11;
                break;
            case 12:
                sDpwTrWork.result = -6;
                break;
            case 13:
                sDpwTrWork.result = -7;
                break;
            case 14:
                sDpwTrWork.result = -2;
                break;
            default:
                sDpwTrWork.result = -13;
                break;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 4:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            sDpwTrWork.code[0] = answer[0];
            sDpwTrWork.code[1] = answer[1];
            switch (sDpwTrWork.code[0]) {
            case 1:
                sDpwTrWork.result = 0;
                break;
            case 2:
                sDpwTrWork.result = -5;
                break;
            case 3:
                sDpwTrWork.result = -4;
                break;
            case 5:
                sDpwTrWork.result = -3;
                break;
            case 14:
                sDpwTrWork.result = -2;
                break;
            default:
                sDpwTrWork.result = -13;
                break;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 6:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            memcpy(sDpwTrWork.resultData, answer, 0x128);
            if (size == 0x128) {
                sDpwTrWork.result = 0;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case 5:
                    sDpwTrWork.result = -3;
                    break;
                case 3:
                    sDpwTrWork.result = -4;
                    break;
                case 14:
                    sDpwTrWork.result = -2;
                    break;
                default:
                    sDpwTrWork.result = -13;
                    break;
                }
            }
            Gamestats_Close();
            break;
        }
        break;
    case 8:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            memcpy(sDpwTrWork.resultData, answer, 0x128);
            if (size == 0x128) {
                sDpwTrWork.result = 1;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case 5:
                    sDpwTrWork.result = -3;
                    break;
                case 3:
                    sDpwTrWork.result = -4;
                    break;
                case 4:
                    sDpwTrWork.result = 0;
                    break;
                case 14:
                    sDpwTrWork.result = -2;
                    break;
                default:
                    sDpwTrWork.result = -13;
                    break;
                }
            }
            Gamestats_Close();
            break;
        }
        break;
    case 10:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            sDpwTrWork.code[0] = answer[0];
            sDpwTrWork.code[1] = answer[1];
            switch (sDpwTrWork.code[0]) {
            case 1:
                sDpwTrWork.result = 0;
                break;
            case 5:
                sDpwTrWork.result = -3;
                break;
            case 3:
                sDpwTrWork.result = -4;
                break;
            case 14:
                sDpwTrWork.result = -2;
                break;
            default:
                sDpwTrWork.result = -13;
                break;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 12:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            sDpwTrWork.code[0] = answer[0];
            sDpwTrWork.code[1] = answer[1];
            switch (sDpwTrWork.code[0]) {
            case 1:
                sDpwTrWork.result = 0;
                break;
            case 5:
                sDpwTrWork.result = -3;
                break;
            case 2:
                sDpwTrWork.result = -5;
                break;
            case 3:
                sDpwTrWork.result = -4;
                break;
            case 14:
                sDpwTrWork.result = -2;
                break;
            default:
                sDpwTrWork.result = -13;
                break;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 14:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            ((u8 *)sDpwTrWork.resultData)[0] = answer[0];
            ((u8 *)sDpwTrWork.resultData)[1] = answer[1];
            switch (*(u8 *)sDpwTrWork.resultData) {
            case 1:
                if ((u32)(size - 2) >= 0x128) {
                    sDpwTrWork.result = (u32)(size - 2) / 0x128;
                    memcpy(sDpwTrWork.resultData, answer + 2, size - 2);
                } else if (size == 2) {
                    sDpwTrWork.result = 0;
                }
                break;
            case 14:
                sDpwTrWork.result = -2;
                break;
            default:
                sDpwTrWork.result = -13;
                break;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 16:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            memcpy(sDpwTrWork.resultData, answer, 0x128);
            if (size == 0x128) {
                sDpwTrWork.result = 0;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case 2:
                    sDpwTrWork.result = -5;
                    break;
                case 12:
                    sDpwTrWork.result = -6;
                    break;
                case 13:
                    sDpwTrWork.result = -7;
                    break;
                case 8:
                    sDpwTrWork.result = -8;
                    break;
                case 9:
                    sDpwTrWork.result = -9;
                    break;
                case 10:
                    sDpwTrWork.result = -10;
                    break;
                case 11:
                    sDpwTrWork.result = -11;
                    break;
                case 14:
                    sDpwTrWork.result = -2;
                    break;
                default:
                    sDpwTrWork.result = -13;
                    break;
                }
            }
            Gamestats_Close();
            break;
        }
        break;
    case 18:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            sDpwTrWork.code[0] = answer[0];
            sDpwTrWork.code[1] = answer[1];
            if (sDpwTrWork.code[0] == 1) {
                sDpwTrWork.result = 0;
            } else if (sDpwTrWork.code[0] == 2) {
                sDpwTrWork.result = -5;
            } else if (sDpwTrWork.code[0] == 14) {
                sDpwTrWork.result = -2;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 20:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            sDpwTrWork.code[0] = answer[0];
            sDpwTrWork.code[1] = answer[1];
            switch (sDpwTrWork.code[0]) {
            case 1:
                sDpwTrWork.result = 0;
                break;
            case 6:
                sDpwTrWork.result = 1;
                break;
            case 7:
                sDpwTrWork.result = 2;
                break;
            case 14:
                sDpwTrWork.result = -2;
                break;
            default:
                sDpwTrWork.result = -13;
                break;
            }
            Gamestats_Close();
            break;
        }
        break;
    case 22:
        switch (Gamestats_Poll()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = 0x18;
            func_020583b0(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            Gamestats_Close();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = 0x18;
            answer = Gamestats_GetBody(&size);
            ((u8 *)sDpwTrWork.resultData)[0] = answer[0];
            ((u8 *)sDpwTrWork.resultData)[1] = answer[1];
            if (size == 8) {
                sDpwTrWork.result = 0;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case 1:
                    sDpwTrWork.result = 0;
                    break;
                case 6:
                    sDpwTrWork.result = 1;
                    break;
                case 7:
                    sDpwTrWork.result = 2;
                    break;
                case 14:
                    sDpwTrWork.result = -2;
                    break;
                default:
                    sDpwTrWork.result = -13;
                    break;
                }
            }
            Gamestats_Close();
            break;
        }
        break;
    case 0:
    case 1:
    case 0x18:
        break;
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
    case 13:
    case 15:
    case 17:
    case 19:
    case 21:
    case 23:
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -12;
        Gamestats_Close();
        break;
    }
}

void DpwTr_End(void) {
    sDpwTrWork.state = 0;
}

BOOL DpwTr_IsFinished(void) {
    switch (sDpwTrWork.state) {
    case 1:
        return TRUE;
    case 0x18:
        sDpwTrWork.state = 1;
        return TRUE;
    default:
        return FALSE;
    }
}

s32 DpwTr_GetResult(void) {
    return sDpwTrWork.result;
}

void DpwTr_Upload(Dpw_Tr_Data *data, const void *sign, u32 size) {
    memcpy(sDpwTrWork.request, data, 0x128);
    sDpwTrWork.requestId = 0;
    memcpy(sDpwTrWork.signature, sign, size);
    sDpwTrWork.signatureSize = size;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/post.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = 2;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_UploadFinish(void) {
    *(DpwFriendKey *)sDpwTrWork.request = sDpwTrWork.friendKey;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/post_finish.asp", sDpwTrWork.pid, sDpwTrWork.request, 8, NULL) == 0) {
        sDpwTrWork.state = 4;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_Download(Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/get.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = 6;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_GetTradeResult(Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/result.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = 8;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_DeleteFinish(void) {
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/delete.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = 10;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_ReturnFinish(void) {
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/return.asp", sDpwTrWork.pid, sDpwTrWork.request, 0, NULL) == 0) {
        sDpwTrWork.state = 12;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

// Cancels the call that is running, and waits for the next state
void DpwTr_Cancel(void) {
    switch (sDpwTrWork.state) {
    case 2:
        Gamestats_Abort();
        sDpwTrWork.state = 3;
        break;
    case 4:
        Gamestats_Abort();
        sDpwTrWork.state = 5;
        break;
    case 6:
        Gamestats_Abort();
        sDpwTrWork.state = 7;
        break;
    case 8:
        Gamestats_Abort();
        sDpwTrWork.state = 9;
        break;
    case 10:
        Gamestats_Abort();
        sDpwTrWork.state = 11;
        break;
    case 12:
        Gamestats_Abort();
        sDpwTrWork.state = 13;
        break;
    case 14:
        Gamestats_Abort();
        sDpwTrWork.state = 15;
        break;
    case 16:
        Gamestats_Abort();
        sDpwTrWork.state = 17;
        break;
    case 18:
        Gamestats_Abort();
        sDpwTrWork.state = 19;
        break;
    case 20:
        Gamestats_Abort();
        sDpwTrWork.state = 21;
        break;
    case 22:
        Gamestats_Abort();
        sDpwTrWork.state = 23;
        break;
    }
}

void DpwTr_Search(const Dpw_Tr_PokemonSearchData *search, s32 maxNum, Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    *(Dpw_Tr_PokemonSearchData *)sDpwTrWork.request = *search;
    sDpwTrWork.request[6] = maxNum;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/search.asp", sDpwTrWork.pid, sDpwTrWork.request, 7, NULL) == 0) {
        sDpwTrWork.state = 14;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_SearchEx(const Dpw_Tr_PokemonSearchDataEx *search, Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    *(Dpw_Tr_PokemonSearchDataEx *)sDpwTrWork.request = *search;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/search.asp", sDpwTrWork.pid, sDpwTrWork.request, 8, NULL) == 0) {
        sDpwTrWork.state = 14;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_Exchange(s32 id, Dpw_Tr_Data *upload, Dpw_Tr_Data *result, const void *sign, u32 size) {
    sDpwTrWork.resultData = result;
    memcpy(sDpwTrWork.request, upload, 0x128);
    sDpwTrWork.requestId = id;
    memcpy(sDpwTrWork.signature, sign, size);
    sDpwTrWork.signatureSize = size;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/exchange.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = 16;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_ExchangeFinish(void) {
    *(DpwFriendKey *)sDpwTrWork.request = sDpwTrWork.friendKey;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/exchange_finish.asp", sDpwTrWork.pid, sDpwTrWork.request, 8, NULL) == 0) {
        sDpwTrWork.state = 18;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_GetServerState(void) {
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/worldexchange/info.asp", sDpwTrWork.pid, sDpwTrWork.request, 0, NULL) == 0) {
        sDpwTrWork.state = 20;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

void DpwTr_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result) {
    func_0207c33c(profile->unk1C);
    memcpy(sDpwTrWork.request, profile, 0x64);
    sDpwTrWork.resultData = result;
    Gamestats_Start(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (Gamestats_Send("/web/common/setProfile.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x64, NULL) == 0) {
        sDpwTrWork.state = 22;
    } else {
        sDpwTrWork.state = 0x18;
        sDpwTrWork.result = -13;
        Gamestats_Close();
    }
}

// The result of a failure of the game statistics client, from its group of errors
static int DpwTrMapGamestatsError(int group) {
    int result;

    switch (group) {
    case 0:
    case 1:
    case 2:
        result = -15;
        break;
    case 3:
        result = -14;
        break;
    case 5:
        result = -14;
        break;
    case 4:
        result = -15;
        break;
    case 6:
        result = -14;
        break;
    case 7:
        result = -13;
        break;
    default:
        result = -14;
        break;
    }
    if (result != -13) {
        if (func_0205b5ec()) {
            result = -14;
        }
        func_02058490();
    }
    return result;
}
