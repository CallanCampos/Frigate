#ifndef POKEBW2_APP_GDS_RAP_H
#define POKEBW2_APP_GDS_RAP_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/nhttp_rap.h"
#include "poke_net/poke_net_gds.h"
#include "save/battle_rec.h"
#include "save/gds_profile.h"
#include "save/musical_savedata.h"
#include "struct_decls.h"

// The game's side of the Global Data Server (GDS) client, in overlay 201: gds_rap.c, which the ROM's string names, and
// gds_rap_response.c after it (a guess, after the Generation 4 games' file of the same functions). The Battle
// Recorder's battle videos and musical photos go through it: a request is sent with one of the GdsRap_Send functions,
// GdsRap_Main runs it, and its end calls the callback of the request, after which the getters give what was received.
// Our names

typedef struct GdsRapWork GdsRapWork;

// The result of the last request, valid when it failed
typedef struct {
    BOOL isValid;
    // GDS_RAP_RESULT_TYPE_*
    u32 type;
    // For a server error, the request, POKE_NET_GDS_REQCODE_*
    s32 code;
    // The library's error (POKE_NET_GDS_LASTERROR_*), the server's result, or GDS_RAP_HTTP_ERROR_*
    u32 detail;
} GdsRapResult;

enum {
    // The library could not finish the request
    GDS_RAP_RESULT_TYPE_LIB_ERROR,
    // The request was aborted
    GDS_RAP_RESULT_TYPE_ABORTED,
    // The server answered with an error, or the check of a video's Pokémon failed
    GDS_RAP_RESULT_TYPE_SERVER_ERROR,
};

// The details of a failed check of a video's Pokémon, by the HTTP status of its answer: 400, 401, 408 and any other
enum {
    GDS_RAP_HTTP_ERROR_BAD_REQUEST = 150,
    GDS_RAP_HTTP_ERROR_UNAUTHORIZED,
    GDS_RAP_HTTP_ERROR_TIMEOUT,
    GDS_RAP_HTTP_ERROR_OTHER,
};

// A function the client calls with the caller's work once a request has finished
typedef void (*GdsRapCallback)(void *work, const GdsRapResult *result);

// The callbacks of the requests, and the work they are called with
typedef struct {
    void *work;
    GdsRapCallback onUploadMusicalShotEnd;
    GdsRapCallback onDownloadMusicalShotEnd;
    GdsRapCallback onUploadVideoEnd;
    GdsRapCallback onDownloadVideoListEnd;
    GdsRapCallback onDownloadVideoEnd;
    GdsRapCallback onFavoriteVideoEnd;
} GdsRapCallbacks;

// What GdsRap_Init is started with
typedef struct {
    // The player's GameSpy profile ID
    s32 profileId;
    HeapID heapId;
    GameData *gameData;
    // The buffer of the player's login to the Nintendo Wi-Fi Connection, which the check of a video's Pokémon takes
    void *loginBuffer;
    GdsRapCallbacks callbacks;
} GdsRapInit;

// What runs after a request's answer, until it returns TRUE: then the request's callback is called
typedef struct GdsRapSubProc GdsRapSubProc;
typedef BOOL (*GdsRapSubProcFunc)(GdsRapWork *gdsrap, GdsRapSubProc *sub);

struct GdsRapSubProc {
    GdsRapCallback callback;
    u16 seq;
    // The step of the save of the video
    u16 saveSeq;
    u32 unk8;
    GdsRapSubProcFunc func;
};

#define GDS_RAP_SIGNATURE_SIZE 0x80

struct GdsRapWork {
    GameData *gameData;
    POKE_NET_REQUESTCOMMON_AUTH auth;
    u32 heapId;
    // The library's status, POKE_NET_GDS_STATUS_*, now and when it was last looked at
    int status;
    int lastStatus;
    GdsRapCallbacks callbacks;
    GdsRapResult result;
    // What the request sends
    union {
        POKE_NET_GDS_MUSICALSHOT_SEND musicalShot;
        BattleRecVideo *video;
        POKE_NET_GDS_BATTLEDATA_SEARCHCONDITION condition;
    } send;
    union {
        // The species whose musical photos are downloaded
        u16 species;
        // The number of the video that is downloaded or marked
        u64 dataNumber;
    };
    // The buffer for the library's answer
    void *response;
    BOOL unk3B0;
    BOOL isActive;
    u8 unk3B8[0x20];
    // The request to send, and the one whose answer is waited for; POKE_NET_GDS_REQCODE_LAST for none. The second
    // name and p_nhttp are the original ones, which the assertions' strings give
    int sendReq;
    int recv_wait_req;
    // The kind of the search, POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_*
    u32 searchType;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strBuf;
    GdsRapSubProc sub;
    u8 unk400;
    // Frames to wait before the request is sent
    u8 sendWait;
    // The step of a video's upload
    u8 uploadSeq;
    // How often the video's Pokémon were checked
    u8 checkCount;
    // The check of the video's Pokémon
    NHttpRap *p_nhttp;
    u8 unk408[2];
    u8 signature[GDS_RAP_SIGNATURE_SIZE];
    u32 checkResult;
    // The Pokémon in the video
    u8 pkmCount;
    u8 unk491;
    // Set once the Pokémon failed the check a second time
    u8 checkFailed;
    // GDS_RAP_HTTP_ERROR_*
    u8 httpError;
    void *loginBuffer;
    // The nicknames of the video's Pokémon, kept while the video is uploaded with the species' names in their place
    u16 nicknames[BATTLE_REC_HEADER_PKM_MAX][11];
    BOOL nicknamesReplaced;
};

// gds_rap.c
// Starts the client, always returning TRUE
BOOL GdsRap_Init(GdsRapWork *gdsrap, const GdsRapInit *init);
void GdsRap_Exit(GdsRapWork *gdsrap);
// Start a request, returning FALSE if another one is running. Upload a musical photo with the player's profile
BOOL GdsRap_SendMusicalShotUpload(GdsRapWork *gdsrap, GdsProfile *profile, MusicalShot *shot);
// Download the newest musical photos of a species
BOOL GdsRap_SendMusicalShotDownload(GdsRapWork *gdsrap, u16 species);
// Upload the loaded battle video, with the player's profile
BOOL GdsRap_SendBattleVideoUpload(GdsRapWork *gdsrap, GdsProfile *profile);
// Download a list of the battle videos with a species, of a battle mode (an index of the client's table of modes)
BOOL GdsRap_SendBattleVideoSearch(GdsRapWork *gdsrap, u16 species, u32 searchModeNo, u8 a3, u8 a4);
// Download one of the server's three lists of battle videos
BOOL GdsRap_SendBattleVideoList1(GdsRapWork *gdsrap);
BOOL GdsRap_SendBattleVideoList2(GdsRapWork *gdsrap);
BOOL GdsRap_SendBattleVideoList3(GdsRapWork *gdsrap);
// Download the battle video of a number, and mark it as a favorite
BOOL GdsRap_SendBattleVideoDownload(GdsRapWork *gdsrap, u64 dataNumber);
BOOL GdsRap_SendBattleVideoFavorite(GdsRapWork *gdsrap, u64 dataNumber);
// Runs the requests, every frame
BOOL GdsRap_Main(GdsRapWork *gdsrap);
// Takes an event of the Wi-Fi connection: an error ends the check of a video's Pokémon
void GdsRap_OnNetEvent(GdsRapWork *gdsrap, int a1, int event, int a3);
// Gives the result of the last request in result, and whether there is one
BOOL GdsRap_GetResult(GdsRapWork *gdsrap, GdsRapResult **result);
// Clears the result
void GdsRap_ClearResult(GdsRapWork *gdsrap);
// Whether no request is running
BOOL GdsRap_IsIdle(GdsRapWork *gdsrap);

// gds_rap_response.c: whether the server's answer to each request is a success
BOOL GdsRapResponse_MusicalShotUpload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res);
BOOL GdsRapResponse_MusicalShotDownload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res);
BOOL GdsRapResponse_BattleVideoUpload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res);
BOOL GdsRapResponse_BattleVideoSearch(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res);
BOOL GdsRapResponse_BattleVideoDownload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res);
BOOL GdsRapResponse_BattleVideoFavorite(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res);
// What the last request received. Point to the downloaded musical photos, up to max of them, and give their number
int GdsRapResponse_GetMusicalShots(GdsRapWork *gdsrap, POKE_NET_GDS_MUSICALSHOT_SEND **list, int max);
// The number that the server gave the uploaded video
u64 GdsRapResponse_GetBattleVideoNumber(GdsRapWork *gdsrap);
// Copy the videos of the downloaded list to list, up to max of them, and give their number
int GdsRapResponse_GetBattleVideoList(GdsRapWork *gdsrap, POKE_NET_GDS_BATTLEDATA_OUTLINE *list, int max);
// Points to the downloaded video, and gives the low word of its number
u32 GdsRapResponse_GetBattleVideo(GdsRapWork *gdsrap, BattleRecVideo **video);

#endif // POKEBW2_APP_GDS_RAP_H
