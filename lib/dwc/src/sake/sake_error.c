#include "sake_internal.h"

// How an answer from the storage server is taken in, and how a request becomes a message: the server names its
// error in the answer's result element, and every message starts with the game's ID, secret key and login ticket.

static SAKERequestResult Sake_GetServerError(const char *name);

// The declaration of the message's namespace
static const char *sSakeNamespaceDeclarations[] = { "ns1=\"http://gamespy.net/sake\"" };

static SAKERequestResult Sake_GetServerError(const char *name) {
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

void Sake_OnResponse(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData) {
    SAKERequest request = userData;
    char result[0x20];
    void *output = NULL;
    int error;

    if (request == NULL || request->sake == NULL || request->type == NULL) {
        return;
    }
    error = Sake_GetHttpResultError(status);
    if (error == SAKERequestResult_SUCCESS) {
        if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, request->type->responseName) ||
            !gsXmlReadChildAsStringNT(reader, request->type->resultName, result, sizeof(result))) {
            error = SAKERequestResult_MALFORMED_RESPONSE;
        } else {
            error = Sake_GetServerError(result);
            if (error == SAKERequestResult_SUCCESS && request->type->outputSize != 0) {
                request->output = gsimalloc(request->type->outputSize);
                if (request->output != NULL) {
                    request->response = reader;
                    error = request->type->parse(request);
                    if (error == SAKERequestResult_SUCCESS) {
                        output = request->output;
                    }
                } else {
                    error = SAKERequestResult_OUT_OF_MEMORY;
                }
            }
        }
    }
    if (request->callback != NULL) {
        request->callback(request->sake, request, error, request->input, output, request->userData);
    }
    if (request->type->freeOutput != NULL) {
        request->type->freeOutput(request);
    }
    gsifree(request->output);
    request->output = NULL;
    Sake_FreeRequest(request);
}

SAKEStartRequestResult Sake_BuildMessage(SAKERequest request) {
    SAKEIRequestInfo *type = request->type;
    int error;

    error = type->validate(request);
    if (error != 0) {
        return error;
    }
    request->writer = gsXmlCreateStreamWriter(sSakeNamespaceDeclarations, 1);
    if (request->writer == NULL) {
        return SAKEStartRequestResult_OUT_OF_MEMORY;
    }
    gsXmlWriteOpenTag(request->writer, "ns1", request->type->name);
    gsXmlWriteIntElement(request->writer, "ns1", "gameid", request->sake->gameId);
    gsXmlWriteStringElement(request->writer, "ns1", "secretKey", request->sake->secretKey);
    gsXmlWriteStringElement(request->writer, "ns1", "loginTicket", request->sake->loginTicket.data);
    error = type->build(request);
    if (error != 0) {
        gsXmlFreeWriter(request->writer);
        request->writer = NULL;
        if (type->freeOutput != NULL) {
            type->freeOutput(request);
        }
        return error;
    }
    gsXmlWriteCloseTag(request->writer, "ns1", request->type->name);
    gsXmlCloseWriter(request->writer);
    return 0;
}
