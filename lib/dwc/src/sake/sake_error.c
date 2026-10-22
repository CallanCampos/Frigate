#include "sake_internal.h"

// How an answer from the storage server is taken in, and how a request becomes a message: the server names its
// error in the answer's result element, and every message starts with the game's ID, secret key and login ticket. With
// sake_post.c it is GameSpy's sakeRequest.c, whose names it has where the code shows them.

static SAKERequestResult sakeiCheckSakeResult(const char *name);

// The declaration of the message's namespace
static const char *GSI_SAKE_SERVICE_NAMESPACES[] = { "ns1=\"http://gamespy.net/sake\"" };

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

void sakeiSoapCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData) {
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

SAKEStartRequestResult sakeiSetupRequest(SAKERequest request) {
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
