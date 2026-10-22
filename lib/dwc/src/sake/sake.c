#include "sake_internal.h"

// The Sake itself: setting it up for a game and a player, starting the requests, and the URLs of the file server.
// The names are GameSpy's own, as its sakeMain.c has them (the ROM keeps the texts of "sakeStartup",
// "sakeCreateRecord", "sakeUpdateRecord", "sakeGetMyRecords" and "sakeGetSpecificRecords"); the last three functions,
// sakeiInitRequest, sakeiFreeRequest and sakeiCheckHttpResult, are the first of GameSpy's sakeRequest.c.

static SAKERequest sakeiInitRequest(SAKE sake, SAKEIRequestType kind, void *input, SAKERequestCallback callback,
                                    void *userData);
static SAKERequest sakeiRunRequest(SAKE sake, void *input, SAKERequestCallback callback, void *userData,
                                   SAKEIRequestType kind, SAKEStartRequestResult (*submit)(SAKERequest request));
static int Sake_StopHttp(SAKE sake, SAKERequest request);

// The URLs of a file server that the game asked for, in place of the standard
static char gSakeUploadUrlOverride[128];
static char gSakeDownloadUrlOverride[128];

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

void sakeShutdown(SAKE sake) {
    gsifree(sake);
}

void sakeSetGame(SAKE sake, const char *gameName, int gameId, const char *secretKey) {
    strcpy(sake->mGameName, gameName);
    strcpy(sake->mSecretKey, secretKey);
    sake->mGameId = gameId;
    sake->mIsGameAuthenticated = gsi_true;
}

void sakeSetProfile(SAKE sake, int profileId, const SakeLoginTicket *loginTicket) {
    sake->mProfileId = profileId;
    sake->mLoginTicket = *loginTicket;
    sake->mIsProfileAuthenticated = gsi_true;
}

SAKEStartRequestResult sakeGetStartRequestResult(SAKE sake) {
    return sake->mStartRequestResult;
}

static SAKERequest sakeiRunRequest(SAKE sake, void *input, SAKERequestCallback callback, void *userData,
                                   SAKEIRequestType kind, SAKEStartRequestResult (*submit)(SAKERequest request)) {
    SAKERequest request = sakeiInitRequest(sake, kind, input, callback, userData);

    if (request == NULL) {
        return NULL;
    }
    sake->mStartRequestResult = submit(request);
    if (sake->mStartRequestResult != 0) {
        sakeiFreeRequest(request);
        return NULL;
    }
    return request;
}

static int Sake_StopHttp(SAKE sake, SAKERequest request) {
    if (sake == NULL || request == NULL || request->mInfo == NULL || request->mInfo->http == NULL) {
        return SAKEStartRequestResult_BAD_INPUT;
    }
    gsiCoreCancelTask(request->mInfo->http);
    return 0;
}

SAKERequest sakeCreateRecord(SAKE sake, SAKECreateRecordInput *input, SAKERequestCallback callback, void *userData) {
    return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_CREATE_RECORD,
                           sakeiStartCreateRecordRequest);
}

SAKERequest sakeUpdateRecord(SAKE sake, SAKEUpdateRecordInput *input, SAKERequestCallback callback, void *userData) {
    return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_UPDATE_RECORD,
                           sakeiStartUpdateRecordRequest);
}

SAKERequest sakeGetMyRecords(SAKE sake, SAKEGetMyRecordsInput *input, SAKERequestCallback callback, void *userData) {
    return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_MY_RECORDS,
                           sakeiStartGetMyRecordsRequest);
}

SAKERequest sakeGetSpecificRecords(SAKE sake, SAKEGetSpecificRecordsInput *input, SAKERequestCallback callback,
                                   void *userData) {
    return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_SPECIFIC_RECORDS,
                           sakeiStartGetSpecificRecordsRequest);
}

gsi_bool sakeSetFileDownloadURL(SAKE sake, const char *url) {
    if (sake == NULL || url == NULL) {
        return gsi_false;
    }
    strcpy(gSakeDownloadUrlOverride, url);
    return gsi_true;
}

gsi_bool sakeGetFileDownloadURL(SAKE sake, int fileId, char *url) {
    int length;

    if (sake == NULL || url == NULL || !sake->mIsGameAuthenticated || !sake->mIsProfileAuthenticated) {
        return gsi_false;
    }
    if (gSakeDownloadUrlOverride[0] != '\0') {
        length = OS_SNPrintf(url, 128, "%s?fileid=%d&gameid=%d&pid=%d", gSakeDownloadUrlOverride, fileId, sake->mGameId,
                             sake->mProfileId);
    } else {
        length =
            OS_SNPrintf(url, 128, "http://%s.sake.gamespy.com/SakeFileServer/download.aspx?fileid=%d&gameid=%d&pid=%d",
                        sake->mGameName, fileId, sake->mGameId, sake->mProfileId);
    }
    if (length < 0) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool sakeSetFileUploadURL(SAKE sake, const char *url) {
    if (sake == NULL || url == NULL) {
        return gsi_false;
    }
    strcpy(gSakeUploadUrlOverride, url);
    return gsi_true;
}

gsi_bool sakeGetFileUploadURL(SAKE sake, char *url) {
    int length;

    if (sake == NULL || url == NULL || !sake->mIsGameAuthenticated || !sake->mIsProfileAuthenticated) {
        return gsi_false;
    }
    if (gSakeUploadUrlOverride[0] != '\0') {
        length = OS_SNPrintf(url, 128, "%s?gameid=%d&pid=%d", gSakeUploadUrlOverride, sake->mGameId, sake->mProfileId);
    } else {
        length = OS_SNPrintf(url, 128, "http://%s.sake.gamespy.com/SakeFileServer/upload.aspx?gameid=%d&pid=%d",
                             sake->mGameName, sake->mGameId, sake->mProfileId);
    }
    if (length < 0) {
        return gsi_false;
    }
    return gsi_true;
}

int Sake_CancelRequest(SAKE sake, SAKERequest request) {
    return Sake_StopHttp(sake, request);
}

static SAKERequest sakeiInitRequest(SAKE sake, SAKEIRequestType kind, void *input, SAKERequestCallback callback,
                                    void *userData) {
    SAKERequest request;

    sake->mStartRequestResult = 0;
    if (input == NULL) {
        sake->mStartRequestResult = SAKEStartRequestResult_BAD_INPUT;
        return NULL;
    }
    if (!sake->mIsGameAuthenticated || !sake->mIsProfileAuthenticated) {
        sake->mStartRequestResult = SAKEStartRequestResult_NOT_AUTHENTICATED;
        return NULL;
    }
    request = gsimalloc(sizeof(struct SAKERequestInternal));
    if (request == NULL) {
        sake->mStartRequestResult = SAKEStartRequestResult_OUT_OF_MEMORY;
        return NULL;
    }
    memset(request, 0, sizeof(struct SAKERequestInternal));
    request->mSake = sake;
    request->mType = kind;
    request->mInput = input;
    request->mCallback = callback;
    request->mUserData = userData;
    return request;
}

void sakeiFreeRequest(SAKERequest request) {
    gsifree(request);
}

SAKERequestResult sakeiCheckHttpResult(GHTTPResult status) {
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
