#include "sake_internal.h"

// Posting a request's message to the storage server, whose URL is made once from the game's name.

static void Sake_PostMessage(SAKERequest request);

// The URL of the storage server, which the Wi-Fi library sets up (in gdb.c) once it knows the server it uses
char gSakeStorageUrl[128];

static void Sake_PostMessage(SAKERequest request) {
    GSSoapTask *http;

    if (gSakeStorageUrl[0] == '\0') {
        OS_SNPrintf(gSakeStorageUrl, 128, "http://%s.sake.gs.nintendowifi.net/SakeStorageServer/StorageServer.asmx",
                    request->sake->gameName);
    }
    http = gsiExecuteSoap(gSakeStorageUrl, request->type->soapAction, request->writer, Sake_OnResponse, request);
    if (http != NULL) {
        request->type->http = http->handle;
    } else {
        request->type->http = NULL;
    }
}

SAKEStartRequestResult Sake_Submit(SAKERequest request, SAKEIRequestInfo *type) {
    int error;

    request->type = type;
    error = Sake_BuildMessage(request);
    if (error != 0) {
        return error;
    }
    Sake_PostMessage(request);
    return 0;
}
