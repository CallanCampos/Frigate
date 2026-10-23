#include "sake_internal.h"

// A request to the storage server: making and freeing it, taking in the server's answer, and writing and posting its
// message. The server names its error in the answer's result element, and every message starts with the game's ID,
// secret key and login ticket. The file and the names in it are GameSpy's own, as its sakeRequest.c (UniSpySDK) has
// them: its eight functions are sakeRequest.c's, in its order, from sakeiInitRequest to sakeiStartRequest, and its data
// starts with sakeRequest.c's namespace table GSI_SAKE_SERVICE_NAMESPACES. The ROM doesn't name the file.

static SAKERequestResult sakeiCheckHttpResult(GHTTPResult status);
static SAKERequestResult sakeiCheckSakeResult(const char *name);
static void sakeiSoapCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData);
static SAKEStartRequestResult sakeiSetupRequest(SAKERequest request);
static void sakeiExecuteRequest(SAKERequest request);

// The declaration of the message's namespace
static const char *GSI_SAKE_SERVICE_NAMESPACES[] = { "ns1=\"http://gamespy.net/sake\"" };

// The URL of the storage server, which the Wi-Fi library sets up (in gdb.c) once it knows the server it uses
char sakeiSoapUrl[128];

SAKERequest sakeiInitRequest(SAKE sake, SAKEIRequestType kind, void *input, SAKERequestCallback callback,
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

static SAKERequestResult sakeiCheckHttpResult(GHTTPResult status) {
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

static SAKERequestResult sakeiCheckSakeResult(const char *name) {
    if (strcmp(name, "Success") == 0) {
        return SAKERequestResult_SUCCESS;
    }
    if (strcmp(name, "SecretKeyInvalid") == 0) {
        return SAKERequestResult_SECRET_KEY_INVALID;
    }
    if (strcmp(name, "ServiceDisabled") == 0) {
        return SAKERequestResult_SERVICE_DISABLED;
    }
    if (strcmp(name, "DatabaseUnavailable") == 0) {
        return SAKERequestResult_DATABASE_UNAVAILABLE;
    }
    if (strcmp(name, "LoginTicketInvalid") == 0) {
        return SAKERequestResult_LOGIN_TICKET_INVALID;
    }
    if (strcmp(name, "LoginTicketExpired") == 0) {
        return SAKERequestResult_LOGIN_TICKET_EXPIRED;
    }
    if (strcmp(name, "TableNotFound") == 0) {
        return SAKERequestResult_TABLE_NOT_FOUND;
    }
    if (strcmp(name, "RecordNotFound") == 0) {
        return SAKERequestResult_RECORD_NOT_FOUND;
    }
    if (strcmp(name, "FieldNotFound") == 0) {
        return SAKERequestResult_FIELD_NOT_FOUND;
    }
    if (strcmp(name, "FieldTypeInvalid") == 0) {
        return SAKERequestResult_FIELD_TYPE_INVALID;
    }
    if (strcmp(name, "NoPermission") == 0) {
        return SAKERequestResult_NO_PERMISSION;
    }
    if (strcmp(name, "RecordLimitReached") == 0) {
        return SAKERequestResult_RECORD_LIMIT_REACHED;
    }
    if (strcmp(name, "AlreadyRated") == 0) {
        return SAKERequestResult_ALREADY_RATED;
    }
    if (strcmp(name, "NotRateable") == 0) {
        return SAKERequestResult_NOT_RATEABLE;
    }
    if (strcmp(name, "NotOwned") == 0) {
        return SAKERequestResult_NOT_OWNED;
    }
    if (strcmp(name, "FilterInvalid") == 0) {
        return SAKERequestResult_FILTER_INVALID;
    }
    if (strcmp(name, "SortInvalid") == 0) {
        return SAKERequestResult_SORT_INVALID;
    }
    if (strcmp(name, "TargetFilterInvalid") == 0) {
        return SAKERequestResult_TARGET_FILTER_INVALID;
    }
    return SAKERequestResult_UNKNOWN_ERROR;
}

static void sakeiSoapCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData) {
    SAKERequest request = userData;
    char result[0x20];
    void *output = NULL;
    int error;

    if (request == NULL || request->mSake == NULL || request->mInfo == NULL) {
        return;
    }
    error = sakeiCheckHttpResult(status);
    if (error == SAKERequestResult_SUCCESS) {
        if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, request->mInfo->mResponseTag) ||
            !gsXmlReadChildAsStringNT(reader, request->mInfo->mResultTag, result, sizeof(result))) {
            error = SAKERequestResult_MALFORMED_RESPONSE;
        } else {
            error = sakeiCheckSakeResult(result);
            if (error == SAKERequestResult_SUCCESS && request->mInfo->mSakeOutputSize != 0) {
                request->mOutput = gsimalloc(request->mInfo->mSakeOutputSize);
                if (request->mOutput != NULL) {
                    request->mSoapResponse = reader;
                    error = request->mInfo->mProcessSoapResponseFunc(request);
                    if (error == SAKERequestResult_SUCCESS) {
                        output = request->mOutput;
                    }
                } else {
                    error = SAKERequestResult_OUT_OF_MEMORY;
                }
            }
        }
    }
    if (request->mCallback != NULL) {
        request->mCallback(request->mSake, request, error, request->mInput, output, request->mUserData);
    }
    if (request->mInfo->mFreeDataFunc != NULL) {
        request->mInfo->mFreeDataFunc(request);
    }
    gsifree(request->mOutput);
    request->mOutput = NULL;
    sakeiFreeRequest(request);
}

static SAKEStartRequestResult sakeiSetupRequest(SAKERequest request) {
    SAKEIRequestInfo *type = request->mInfo;
    int error;

    error = type->mValidateInputFunc(request);
    if (error != 0) {
        return error;
    }
    request->mSoapRequest = gsXmlCreateStreamWriter(GSI_SAKE_SERVICE_NAMESPACES, 1);
    if (request->mSoapRequest == NULL) {
        return SAKEStartRequestResult_OUT_OF_MEMORY;
    }
    gsXmlWriteOpenTag(request->mSoapRequest, "ns1", request->mInfo->mFuncName);
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "gameid", request->mSake->mGameId);
    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "secretKey", request->mSake->mSecretKey);
    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "loginTicket", request->mSake->mLoginTicket.data);
    error = type->mFillSoapRequestFunc(request);
    if (error != 0) {
        gsXmlFreeWriter(request->mSoapRequest);
        request->mSoapRequest = NULL;
        if (type->mFreeDataFunc != NULL) {
            type->mFreeDataFunc(request);
        }
        return error;
    }
    gsXmlWriteCloseTag(request->mSoapRequest, "ns1", request->mInfo->mFuncName);
    gsXmlCloseWriter(request->mSoapRequest);
    return 0;
}

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
