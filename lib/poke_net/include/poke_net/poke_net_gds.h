#ifndef POKEBW2_POKE_NET_POKE_NET_GDS_H
#define POKEBW2_POKE_NET_POKE_NET_GDS_H

#include "types.h"
#include "save/battle_rec.h"
#include "save/gds_profile.h"
#include "save/musical_savedata.h"

// The client of the Global Data Server (GDS), which keeps the battle videos and the musical photos of the Global Link.
// It is ARM code at the end of overlay 201, after the game's gds_rap.c that drives it. The ROM names none of it: the
// names are those of the same library in the Generation 4 games (poke_net), which this one corresponds to function for
// function, and our own where Generation 5 differs (the musical photos, the lists of videos and the SSL connection).
// A request runs in a thread of its own: start one, then poll the status until it is finished, aborted or in error

// What every request starts with: who asks. The token is the one of the Nintendo Wi-Fi Connection's service locator,
// which the client fetches itself before its first request
#define POKE_NET_SVL_TOKEN_LENGTH 300

typedef struct {
    s32 PID;
    u8 ROMCode;
    u8 LangCode;
    char SvlToken[POKE_NET_SVL_TOKEN_LENGTH + 1];
    u8 padding133;
    u32 unk134;
} POKE_NET_REQUESTCOMMON_AUTH;

// The requests
enum {
    POKE_NET_GDS_REQCODE_ADMIN = -1,
    POKE_NET_GDS_REQCODE_DEBUG_MESSAGE = 0,
    POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST = 21000,
    POKE_NET_GDS_REQCODE_MUSICALSHOT_GET,
    POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST = 22000,
    POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH,
    POKE_NET_GDS_REQCODE_BATTLEDATA_GET,
    POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE,
    POKE_NET_GDS_REQCODE_LAST,
};

// What POKE_NET_GDS_GetStatus gives
enum {
    POKE_NET_GDS_STATUS_INACTIVE,
    POKE_NET_GDS_STATUS_INITIALIZED,
    POKE_NET_GDS_STATUS_REQUEST,
    POKE_NET_GDS_STATUS_NETSETTING,
    POKE_NET_GDS_STATUS_CONNECTING,
    POKE_NET_GDS_STATUS_SENDING,
    POKE_NET_GDS_STATUS_RECEIVING,
    POKE_NET_GDS_STATUS_ABORTED,
    POKE_NET_GDS_STATUS_FINISHED,
    POKE_NET_GDS_STATUS_ERROR,
};

// What POKE_NET_GDS_GetLastErrorCode gives once the status is POKE_NET_GDS_STATUS_ERROR
enum {
    POKE_NET_GDS_LASTERROR_NONE,
    POKE_NET_GDS_LASTERROR_NOTINITIALIZED,
    POKE_NET_GDS_LASTERROR_ILLEGALREQUEST,
    POKE_NET_GDS_LASTERROR_CREATESOCKET,
    POKE_NET_GDS_LASTERROR_IOCTRLSOCKET,
    POKE_NET_GDS_LASTERROR_NETWORKSETTING,
    POKE_NET_GDS_LASTERROR_CREATETHREAD,
    POKE_NET_GDS_LASTERROR_CONNECT,
    POKE_NET_GDS_LASTERROR_SENDREQUEST,
    POKE_NET_GDS_LASTERROR_RECVRESPONSE,
    POKE_NET_GDS_LASTERROR_CONNECTTIMEOUT,
    POKE_NET_GDS_LASTERROR_SENDTIMEOUT,
    POKE_NET_GDS_LASTERROR_RECVTIMEOUT,
    POKE_NET_GDS_LASTERROR_ABORTED,
    POKE_NET_GDS_LASTERROR_GETSVL,
    POKE_NET_GDS_LASTERROR_SSL,
};

// The answer to a request: the request it answers, the server's result (0 for success) and the request's own data
typedef struct {
    u16 ReqCode;
    u16 Result;
    u8 Param[0];
} POKE_NET_RESPONSE;

// A musical photo with the profile of the player who sends it
typedef struct {
    GdsProfile Profile;
    MusicalShot Data;
} POKE_NET_GDS_MUSICALSHOT_SEND;

// A musical photo as the server gives it: the sender's profile ID and the photo's number before it
typedef struct {
    s32 PID;
    u64 Code;
    POKE_NET_GDS_MUSICALSHOT_SEND Data;
} POKE_NET_GDS_RESPONSE_MUSICALSHOT_RECVDATA;

// The photos that POKE_NET_GDS_MusicalShotGet can give
#define POKE_NET_GDS_MUSICALSHOT_GET_MAX 5

typedef struct {
    s32 Count;
    POKE_NET_GDS_RESPONSE_MUSICALSHOT_RECVDATA Data[POKE_NET_GDS_MUSICALSHOT_GET_MAX];
} POKE_NET_GDS_RESPONSE_MUSICALSHOT_GET;

// The answer to both uploads: the number that the server gave the data
typedef struct {
    u64 Code;
} POKE_NET_GDS_RESPONSE_REGIST;

// The size of the signature of a battle video's Pokémon, which the upload carries
#define POKE_NET_GDS_BATTLEDATA_SIGN_SIZE 0x100

typedef struct {
    BattleRecVideo Data;
    s32 SignSize;
    u8 Sign[POKE_NET_GDS_BATTLEDATA_SIGN_SIZE];
} POKE_NET_GDS_REQUEST_BATTLEDATA_REGIST;

// What a search of battle videos asks for: a species, a battle mode with the mask of its bits that count, and the
// version of the server's data
typedef struct {
    u16 MonsNo;
    u16 BattleMode;
    u16 BattleModeMask;
    u8 unk6;
    u8 unk7;
    u32 ServerVersion;
    u32 unkC;
} POKE_NET_GDS_BATTLEDATA_SEARCHCONDITION;

// The kinds of search: by a condition, or one of the server's three lists
enum {
    POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_CONDITION,
    POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST1,
    POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST2,
    POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST3,
};

typedef struct {
    u16 SearchType;
    u16 SearchOpt;
    POKE_NET_GDS_BATTLEDATA_SEARCHCONDITION ConditionParam;
    u64 SearchParam;
} POKE_NET_GDS_REQUEST_BATTLEDATA_SEARCH;

// What a list shows of a video: the start of the video, its profile and header
typedef struct {
    GdsProfile Profile;
    BattleRecHeader Header;
} POKE_NET_GDS_BATTLEDATA_OUTLINE;

// A video of a list: the sender's profile ID, the video's number, and its profile and header
typedef struct {
    s32 PID;
    u64 Code;
    POKE_NET_GDS_BATTLEDATA_OUTLINE Data;
} POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH_RECVDATA;

// The videos that a search can give
#define POKE_NET_GDS_BATTLEDATA_SEARCH_MAX 30

typedef struct {
    s32 Count;
    POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH_RECVDATA Data[POKE_NET_GDS_BATTLEDATA_SEARCH_MAX];
} POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH;

// A downloaded video
typedef struct {
    s32 PID;
    u64 Code;
    BattleRecVideo Data;
} POKE_NET_GDS_RESPONSE_BATTLEDATA_GET;

// Starts the client for a server, and ends it
BOOL POKE_NET_GDS_Initialize(POKE_NET_REQUESTCOMMON_AUTH *auth, const char *url, u16 port);
void POKE_NET_GDS_Release(void);
// The state of the request, POKE_NET_GDS_STATUS_*
int POKE_NET_GDS_GetStatus(void);
// The error, POKE_NET_GDS_LASTERROR_*, once the status is POKE_NET_GDS_STATUS_ERROR
int POKE_NET_GDS_GetLastErrorCode(void);
// The answer, a POKE_NET_RESPONSE, and its size, once the status is POKE_NET_GDS_STATUS_FINISHED
void *POKE_NET_GDS_GetResponse(void);
s32 POKE_NET_GDS_GetResponseSize(void);
// The size of the buffer that the answer to a request needs, POKE_NET_GDS_REQCODE_ADMIN for the largest of all
s32 POKE_NET_GDS_GetResponseMaxSize(s32 reqno);
// The requests, each of which returns FALSE if it could not start. response is the buffer for the answer
BOOL POKE_NET_GDS_MusicalShotRegist(POKE_NET_GDS_MUSICALSHOT_SEND *data, void *response);
BOOL POKE_NET_GDS_MusicalShotGet(s32 monsno, void *response);
BOOL POKE_NET_GDS_BattleDataRegist(BattleRecVideo *data, u8 *sign, s32 signSize, void *response);
BOOL POKE_NET_GDS_BattleDataSearchCondition(POKE_NET_GDS_BATTLEDATA_SEARCHCONDITION *condition, void *response);
BOOL POKE_NET_GDS_BattleDataSearchList1(u32 serverVersion, void *response);
BOOL POKE_NET_GDS_BattleDataSearchList3(u32 serverVersion, void *response);
BOOL POKE_NET_GDS_BattleDataSearchList2(u32 serverVersion, void *response);
BOOL POKE_NET_GDS_BattleDataGet(u64 code, u32 serverVersion, void *response);
BOOL POKE_NET_GDS_BattleDataFavorite(u64 code, void *response);

#endif // POKEBW2_POKE_NET_POKE_NET_GDS_H
