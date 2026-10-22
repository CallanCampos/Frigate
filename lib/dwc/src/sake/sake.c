#include "sake_internal.h"

// The Sake itself: setting it up for a game and a player, starting the requests, and the URLs of the file server.
// The names of the calls whose texts the ROM keeps ("sakeStartup", "sakeCreateRecord", "sakeUpdateRecord",
// "sakeGetMyRecords" and "sakeGetSpecificRecords") are the SDK's; the others are named by what they do.

static SAKERequest Sake_AllocRequest(SAKE sake, SAKEIRequestType kind, void *input, SAKERequestCallback callback,
                                     void *userData);
static SAKERequest Sake_StartRequest(SAKE sake, void *input, SAKERequestCallback callback, void *userData,
                                     SAKEIRequestType kind, SAKEStartRequestResult (*submit)(SAKERequest request));
static int Sake_StopHttp(SAKE sake, SAKERequest request);

// The URLs of a file server that the game asked for, in place of the standard
static char sSakeUploadUrl[128];
static char sSakeDownloadUrl[128];

SAKEStartupResult sakeStartup(SAKE *sake) {
    SAKE created;

    if (__GSIACResult != GSIACAvailable) {
        return 1;
    }
    if (gsCoreIsShutdown() != GSCore_IN_USE) {
        return 2;
    }
    created = gsimalloc(sizeof(struct SAKEInternal));
    if (created == NULL) {
        return 3;
    }
    memset(created, 0, sizeof(struct SAKEInternal));
    *sake = created;
    return 0;
}

void Sake_Free(SAKE sake) {
    gsifree(sake);
}

void Sake_SetGame(SAKE sake, const char *gameName, int gameId, const char *secretKey) {
    strcpy(sake->gameName, gameName);
    strcpy(sake->secretKey, secretKey);
    sake->gameId = gameId;
    sake->hasGame = gsi_true;
}

void Sake_SetLoginTicket(SAKE sake, int profileId, const SakeLoginTicket *loginTicket) {
    sake->profileId = profileId;
    sake->loginTicket = *loginTicket;
    sake->hasTicket = gsi_true;
}

SAKEStartRequestResult Sake_GetLastError(SAKE sake) {
    return sake->lastError;
}

static SAKERequest Sake_StartRequest(SAKE sake, void *input, SAKERequestCallback callback, void *userData,
                                     SAKEIRequestType kind, SAKEStartRequestResult (*submit)(SAKERequest request)) {
    SAKERequest request = Sake_AllocRequest(sake, kind, input, callback, userData);

    if (request == NULL) {
        return NULL;
    }
    sake->lastError = submit(request);
    if (sake->lastError != 0) {
        Sake_FreeRequest(request);
        return NULL;
    }
    return request;
}

static int Sake_StopHttp(SAKE sake, SAKERequest request) {
    if (sake == NULL || request == NULL || request->type == NULL || request->type->http == NULL) {
        return SAKEStartRequestResult_BAD_INPUT;
    }
    gsiCoreCancelTask(request->type->http);
    return 0;
}

SAKERequest sakeCreateRecord(SAKE sake, SAKECreateRecordInput *input, SAKERequestCallback callback, void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKEIRequestType_CREATE_RECORD, Sake_SubmitCreateRecord);
}

SAKERequest sakeUpdateRecord(SAKE sake, SAKEUpdateRecordInput *input, SAKERequestCallback callback, void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKEIRequestType_UPDATE_RECORD, Sake_SubmitUpdateRecord);
}

SAKERequest sakeGetMyRecords(SAKE sake, SAKEGetMyRecordsInput *input, SAKERequestCallback callback, void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKEIRequestType_GET_MY_RECORDS, Sake_SubmitGetMyRecords);
}

SAKERequest sakeGetSpecificRecords(SAKE sake, SAKEGetSpecificRecordsInput *input, SAKERequestCallback callback,
                                   void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKEIRequestType_GET_SPECIFIC_RECORDS,
                             Sake_SubmitGetSpecificRecords);
}

gsi_bool Sake_SetDownloadUrl(SAKE sake, const char *url) {
    if (sake == NULL || url == NULL) {
        return gsi_false;
    }
    strcpy(sSakeDownloadUrl, url);
    return gsi_true;
}

gsi_bool Sake_GetDownloadUrl(SAKE sake, int fileId, char *url) {
    int length;

    if (sake == NULL || url == NULL || !sake->hasGame || !sake->hasTicket) {
        return gsi_false;
    }
    if (sSakeDownloadUrl[0] != '\0') {
        length = OS_SNPrintf(url, 128, "%s?fileid=%d&gameid=%d&pid=%d", sSakeDownloadUrl, fileId, sake->gameId,
                             sake->profileId);
    } else {
        length =
            OS_SNPrintf(url, 128, "http://%s.sake.gamespy.com/SakeFileServer/download.aspx?fileid=%d&gameid=%d&pid=%d",
                        sake->gameName, fileId, sake->gameId, sake->profileId);
    }
    if (length < 0) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool Sake_SetUploadUrl(SAKE sake, const char *url) {
    if (sake == NULL || url == NULL) {
        return gsi_false;
    }
    strcpy(sSakeUploadUrl, url);
    return gsi_true;
}

gsi_bool Sake_GetUploadUrl(SAKE sake, char *url) {
    int length;

    if (sake == NULL || url == NULL || !sake->hasGame || !sake->hasTicket) {
        return gsi_false;
    }
    if (sSakeUploadUrl[0] != '\0') {
        length = OS_SNPrintf(url, 128, "%s?gameid=%d&pid=%d", sSakeUploadUrl, sake->gameId, sake->profileId);
    } else {
        length = OS_SNPrintf(url, 128, "http://%s.sake.gamespy.com/SakeFileServer/upload.aspx?gameid=%d&pid=%d",
                             sake->gameName, sake->gameId, sake->profileId);
    }
    if (length < 0) {
        return gsi_false;
    }
    return gsi_true;
}

int Sake_CancelRequest(SAKE sake, SAKERequest request) {
    return Sake_StopHttp(sake, request);
}

static SAKERequest Sake_AllocRequest(SAKE sake, SAKEIRequestType kind, void *input, SAKERequestCallback callback,
                                     void *userData) {
    SAKERequest request;

    sake->lastError = 0;
    if (input == NULL) {
        sake->lastError = SAKEStartRequestResult_BAD_INPUT;
        return NULL;
    }
    if (!sake->hasGame || !sake->hasTicket) {
        sake->lastError = SAKEStartRequestResult_NOT_AUTHENTICATED;
        return NULL;
    }
    request = gsimalloc(sizeof(struct SAKERequestInternal));
    if (request == NULL) {
        sake->lastError = SAKEStartRequestResult_OUT_OF_MEMORY;
        return NULL;
    }
    memset(request, 0, sizeof(struct SAKERequestInternal));
    request->sake = sake;
    request->kind = kind;
    request->input = input;
    request->callback = callback;
    request->userData = userData;
    return request;
}

void Sake_FreeRequest(SAKERequest request) {
    gsifree(request);
}

SAKERequestResult Sake_GetHttpResultError(GHTTPResult status) {
    switch (status) {
    case 0:
        return SAKERequestResult_SUCCESS;
    case 1:
        return SAKERequestResult_OUT_OF_MEMORY;
    case 0x12:
        return SAKERequestResult_CANCELED;
    default:
        return SAKERequestResult_CONNECTION_ERROR;
    }
}
