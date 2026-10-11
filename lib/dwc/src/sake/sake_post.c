#include "sake_internal.h"

// Posting a request's message to the storage server, whose URL is made once from the game's name.

static void Sake_PostMessage(SakeRequest *request);

// The URL of the storage server, which the Wi-Fi library sets up (in gdb.c) once it knows the server it uses
char gSakeStorageUrl[128];

static void Sake_PostMessage(SakeRequest *request) {
    GsHttpRequest *http;

    if (gSakeStorageUrl[0] == '\0') {
        func_0207a2c0(gSakeStorageUrl, 128, "http://%s.sake.gs.nintendowifi.net/SakeStorageServer/StorageServer.asmx",
                      request->sake->gameName);
    }
    http = func_ov011_0216ea4c(gSakeStorageUrl, request->type->soapAction, request->writer, Sake_OnResponse, request);
    if (http != NULL) {
        request->type->http = http->handle;
    } else {
        request->type->http = NULL;
    }
}

int Sake_Submit(SakeRequest *request, SakeRequestType *type) {
    int error;

    request->type = type;
    error = Sake_BuildMessage(request);
    if (error != 0) {
        return error;
    }
    Sake_PostMessage(request);
    return 0;
}
