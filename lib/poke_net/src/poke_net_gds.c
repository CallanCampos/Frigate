// The GDS client's own functions: starting and ending it, the state of the request, and the requests themselves, each
// of which builds its data and hands it to the request engine. The ROM names nothing here; the file's name and its
// functions' are those of the Generation 4 library, with our own for the requests that Generation 5 changed.

#include "types.h"
#include "poke_net/poke_net_gds.h"
#include "poke_net_internal.h"
#include "dwc/ext.h"

BOOL POKE_NET_GDS_Initialize(POKE_NET_REQUESTCOMMON_AUTH *auth, const char *url, u16 port) {
    return POKE_NET_Initialize(url, port, auth);
}

void POKE_NET_GDS_Release(void) {
    POKE_NET_Release();
}

int POKE_NET_GDS_GetStatus(void) {
    int status;

    POKE_NET_LockMutex();
    status = POKE_NET_GetStatus();
    POKE_NET_UnlockMutex();
    return status;
}

int POKE_NET_GDS_GetLastErrorCode(void) {
    return POKE_NET_GetLastErrorCode();
}

void *POKE_NET_GDS_GetResponse(void) {
    return POKE_NET_GetResponse();
}

s32 POKE_NET_GDS_GetResponseSize(void) {
    return POKE_NET_GetResponseSize();
}

s32 POKE_NET_GDS_GetResponseMaxSize(s32 reqno) {
    return POKE_NET_GetResponseMaxSize(reqno);
}

// Uploads a musical photo. The answer is a POKE_NET_GDS_RESPONSE_REGIST
BOOL POKE_NET_GDS_MusicalShotRegist(POKE_NET_GDS_MUSICALSHOT_SEND *data, void *response) {
    POKE_NET_GDS_MUSICALSHOT_SEND request;

    request = *data;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST, &request, response, -1);
}

typedef struct {
    u16 SearchType;
    u16 SearchOpt;
    s64 SearchParam;
} POKE_NET_GDS_REQUEST_MUSICALSHOT_GET;

// Downloads the newest photos of a species. The answer is a POKE_NET_GDS_RESPONSE_MUSICALSHOT_GET
BOOL POKE_NET_GDS_MusicalShotGet(s32 monsno, void *response) {
    POKE_NET_GDS_REQUEST_MUSICALSHOT_GET request;

    request.SearchType = 0;
    request.SearchOpt = 0;
    request.SearchParam = monsno;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_MUSICALSHOT_GET, &request, response, -1);
}

// Uploads a battle video with the signature of its Pokémon. The answer is a POKE_NET_GDS_RESPONSE_REGIST
BOOL POKE_NET_GDS_BattleDataRegist(BattleRecVideo *data, u8 *sign, s32 signSize, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_REGIST request = {0};

    if (signSize > POKE_NET_GDS_BATTLEDATA_SIGN_SIZE) {
        return FALSE;
    }
    request.Data = *data;
    request.SignSize = signSize;
    memcpy(request.Sign, sign, signSize);
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST, &request, response, -1);
}

// Searches the battle videos by a condition. The answer to the searches is a POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH
BOOL POKE_NET_GDS_BattleDataSearchCondition(POKE_NET_GDS_BATTLEDATA_SEARCHCONDITION *condition, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_SEARCH request;

    request.SearchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_CONDITION;
    request.ConditionParam = *condition;
    request.SearchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_CONDITION;
    request.SearchOpt = 0;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH, &request, response, -1);
}

// Download the server's three lists of battle videos
BOOL POKE_NET_GDS_BattleDataSearchList1(u32 serverVersion, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_SEARCH request = {0};

    request.SearchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST1;
    request.ConditionParam.ServerVersion = serverVersion;
    request.SearchParam = 0;
    request.SearchOpt = 0;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH, &request, response, -1);
}

BOOL POKE_NET_GDS_BattleDataSearchList3(u32 serverVersion, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_SEARCH request = {0};

    request.SearchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST3;
    request.ConditionParam.ServerVersion = serverVersion;
    request.SearchParam = 0;
    request.SearchOpt = 0;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH, &request, response, -1);
}

BOOL POKE_NET_GDS_BattleDataSearchList2(u32 serverVersion, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_SEARCH request = {0};

    request.SearchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST2;
    request.ConditionParam.ServerVersion = serverVersion;
    request.SearchParam = 0;
    request.SearchOpt = 0;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH, &request, response, -1);
}

typedef struct {
    u64 Code;
    u32 ServerVersion;
} POKE_NET_GDS_REQUEST_BATTLEDATA_GET;

// Downloads the battle video of a number. The answer is a POKE_NET_GDS_RESPONSE_BATTLEDATA_GET
BOOL POKE_NET_GDS_BattleDataGet(u64 code, u32 serverVersion, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_GET request;

    request.Code = code;
    request.ServerVersion = serverVersion;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_GET, &request, response, -1);
}

typedef struct {
    u64 Code;
} POKE_NET_GDS_REQUEST_BATTLEDATA_FAVORITE;

// Marks the battle video of a number as a favorite. The answer has no data
BOOL POKE_NET_GDS_BattleDataFavorite(u64 code, void *response) {
    POKE_NET_GDS_REQUEST_BATTLEDATA_FAVORITE request;

    request.Code = code;
    return POKE_NET_Request(POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE, &request, response, -1);
}

#define POKE_NET_GDS_DEBUG_MESSAGE_SIZE 0x400

s32 POKE_NET_GetRequestSize(s32 reqno) {
    s32 size;

    switch (reqno) {
    case POKE_NET_GDS_REQCODE_DEBUG_MESSAGE:
        size = POKE_NET_GDS_DEBUG_MESSAGE_SIZE;
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST:
        size = sizeof(POKE_NET_GDS_MUSICALSHOT_SEND);
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_GET:
        size = sizeof(POKE_NET_GDS_REQUEST_MUSICALSHOT_GET);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST:
        size = sizeof(POKE_NET_GDS_REQUEST_BATTLEDATA_REGIST);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH:
        size = sizeof(POKE_NET_GDS_REQUEST_BATTLEDATA_SEARCH);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_GET:
        size = sizeof(POKE_NET_GDS_REQUEST_BATTLEDATA_GET);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE:
        size = sizeof(POKE_NET_GDS_REQUEST_BATTLEDATA_FAVORITE);
        break;
    default:
        return -1;
    }
    return size + POKE_NET_REQUEST_HEADER_SIZE;
}

s32 POKE_NET_GetResponseMaxSize(s32 reqno) {
    s32 reqnos[] = {
        POKE_NET_GDS_REQCODE_DEBUG_MESSAGE,
        POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST,
        POKE_NET_GDS_REQCODE_MUSICALSHOT_GET,
        POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST,
        POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH,
        POKE_NET_GDS_REQCODE_BATTLEDATA_GET,
        POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE,
        -1,
    };
    s32 size;
    s32 i;

    switch (reqno) {
    case POKE_NET_GDS_REQCODE_ADMIN:
        // The largest of all, each of which has its header counted already
        size = -1;
        for (i = 0;; i++) {
            s32 max;

            if (reqnos[i] == -1) {
                break;
            }
            max = POKE_NET_GetResponseMaxSize(reqnos[i]);
            if (max > size) {
                size = max;
            }
        }
        if (size == -1) {
            return -1;
        }
        break;
    case POKE_NET_GDS_REQCODE_DEBUG_MESSAGE:
        size = POKE_NET_GDS_DEBUG_MESSAGE_SIZE;
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST:
        size = sizeof(POKE_NET_GDS_RESPONSE_REGIST);
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_GET:
        size = sizeof(POKE_NET_GDS_RESPONSE_MUSICALSHOT_GET);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST:
        size = sizeof(POKE_NET_GDS_RESPONSE_REGIST);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH:
        size = sizeof(POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_GET:
        size = sizeof(POKE_NET_GDS_RESPONSE_BATTLEDATA_GET);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE:
        size = 0;
        break;
    default:
        return -1;
    }
    return size + POKE_NET_RESPONSE_HEADER_SIZE;
}
