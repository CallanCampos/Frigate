#include "sake_internal.h"

// Posting a request's message to the storage server, whose URL is made once from the game's name. With sake_error.c
// it is GameSpy's sakeRequest.c, whose names it has where the code shows them.

static void sakeiExecuteRequest(SAKERequest request);

// The URL of the storage server, which the Wi-Fi library sets up (in gdb.c) once it knows the server it uses
char sakeiSoapUrl[128];

static void sakeiExecuteRequest(SAKERequest request) {
    GSSoapTask *http;

    if (sakeiSoapUrl[0] == '\0') {
        OS_SNPrintf(sakeiSoapUrl, 128, "http://%s.sake.gs.nintendowifi.net/SakeStorageServer/StorageServer.asmx",
                    request->mSake->mGameName);
    }
    http = gsiExecuteSoap(sakeiSoapUrl, request->mInfo->mSoapAction, request->mSoapRequest, sakeiSoapCallback, request);
    if (http != NULL) {
        request->mInfo->http = http->mCoreTask;
    } else {
        request->mInfo->http = NULL;
    }
}

SAKEStartRequestResult sakeiStartRequest(SAKERequest request, SAKEIRequestInfo *type) {
    int error;

    request->mInfo = type;
    error = sakeiSetupRequest(request);
    if (error != 0) {
        return error;
    }
    sakeiExecuteRequest(request);
    return 0;
}
