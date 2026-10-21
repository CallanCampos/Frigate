#include "sake_internal.h"

// The Sake itself: setting it up for a game and a player, starting the requests, and the URLs of the file server.
// The names of the calls whose texts the ROM keeps ("sakeStartup", "sakeCreateRecord", "sakeUpdateRecord",
// "sakeGetMyRecords" and "sakeGetSpecificRecords") are the SDK's; the others are named by what they do.

static SakeRequest *Sake_AllocRequest(Sake *sake, int kind, void *input, SakeCallback callback, void *userData);
static SakeRequest *Sake_StartRequest(Sake *sake, void *input, SakeCallback callback, void *userData, int kind,
                                      int (*submit)(SakeRequest *request));
static int Sake_StopHttp(Sake *sake, SakeRequest *request);

// The URLs of a file server that the game asked for, in place of the standard
static char sSakeUploadUrl[128];
static char sSakeDownloadUrl[128];

int sakeStartup(Sake **sake) {
    Sake *created;

    if (__GSIACResult != 1) {
        return 1;
    }
    if (gsCoreIsShutdown() != 0) {
        return 2;
    }
    created = gsimalloc(sizeof(Sake));
    if (created == NULL) {
        return 3;
    }
    memset(created, 0, sizeof(Sake));
    *sake = created;
    return 0;
}

void Sake_Free(Sake *sake) {
    gsifree(sake);
}

void Sake_SetGame(Sake *sake, const char *gameName, int gameId, const char *secretKey) {
    strcpy(sake->gameName, gameName);
    strcpy(sake->secretKey, secretKey);
    sake->gameId = gameId;
    sake->hasGame = gsi_true;
}

void Sake_SetLoginTicket(Sake *sake, int profileId, const SakeLoginTicket *loginTicket) {
    sake->profileId = profileId;
    sake->loginTicket = *loginTicket;
    sake->hasTicket = gsi_true;
}

int Sake_GetLastError(const Sake *sake) {
    return sake->lastError;
}

static SakeRequest *Sake_StartRequest(Sake *sake, void *input, SakeCallback callback, void *userData, int kind,
                                      int (*submit)(SakeRequest *request)) {
    SakeRequest *request = Sake_AllocRequest(sake, kind, input, callback, userData);

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

static int Sake_StopHttp(Sake *sake, SakeRequest *request) {
    if (sake == NULL || request == NULL || request->type == NULL || request->type->http == NULL) {
        return SAKE_REQUEST_ERROR_NO_INPUT;
    }
    gsiCoreCancelTask(request->type->http);
    return 0;
}

SakeRequest *sakeCreateRecord(Sake *sake, SakeCreateRecordInput *input, SakeCallback callback, void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKE_REQUEST_CREATE_RECORD, Sake_SubmitCreateRecord);
}

SakeRequest *sakeUpdateRecord(Sake *sake, SakeUpdateRecordInput *input, SakeCallback callback, void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKE_REQUEST_UPDATE_RECORD, Sake_SubmitUpdateRecord);
}

SakeRequest *sakeGetMyRecords(Sake *sake, SakeGetMyRecordsInput *input, SakeCallback callback, void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKE_REQUEST_GET_MY_RECORDS, Sake_SubmitGetMyRecords);
}

SakeRequest *sakeGetSpecificRecords(Sake *sake, SakeGetSpecificRecordsInput *input, SakeCallback callback,
                                    void *userData) {
    return Sake_StartRequest(sake, input, callback, userData, SAKE_REQUEST_GET_SPECIFIC_RECORDS,
                             Sake_SubmitGetSpecificRecords);
}

gsi_bool Sake_SetDownloadUrl(Sake *sake, const char *url) {
    if (sake == NULL || url == NULL) {
        return gsi_false;
    }
    strcpy(sSakeDownloadUrl, url);
    return gsi_true;
}

gsi_bool Sake_GetDownloadUrl(const Sake *sake, int fileId, char *url) {
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

gsi_bool Sake_SetUploadUrl(Sake *sake, const char *url) {
    if (sake == NULL || url == NULL) {
        return gsi_false;
    }
    strcpy(sSakeUploadUrl, url);
    return gsi_true;
}

gsi_bool Sake_GetUploadUrl(const Sake *sake, char *url) {
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

int Sake_CancelRequest(Sake *sake, SakeRequest *request) {
    return Sake_StopHttp(sake, request);
}

static SakeRequest *Sake_AllocRequest(Sake *sake, int kind, void *input, SakeCallback callback, void *userData) {
    SakeRequest *request;

    sake->lastError = 0;
    if (input == NULL) {
        sake->lastError = SAKE_REQUEST_ERROR_NO_INPUT;
        return NULL;
    }
    if (!sake->hasGame || !sake->hasTicket) {
        sake->lastError = SAKE_REQUEST_ERROR_NOT_SET_UP;
        return NULL;
    }
    request = gsimalloc(sizeof(SakeRequest));
    if (request == NULL) {
        sake->lastError = SAKE_REQUEST_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    memset(request, 0, sizeof(SakeRequest));
    request->sake = sake;
    request->kind = kind;
    request->input = input;
    request->callback = callback;
    request->userData = userData;
    return request;
}

void Sake_FreeRequest(SakeRequest *request) {
    gsifree(request);
}

int Sake_GetHttpResultError(int status) {
    switch (status) {
    case 0:
        return SAKE_RESULT_SUCCESS;
    case 1:
        return SAKE_RESULT_OUT_OF_MEMORY;
    case 0x12:
        return SAKE_RESULT_CANCELED;
    default:
        return SAKE_RESULT_CONNECTION_ERROR;
    }
}
