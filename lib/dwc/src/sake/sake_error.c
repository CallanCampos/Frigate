#include "sake_internal.h"

// How an answer from the storage server is taken in, and how a request becomes a message: the server names its
// error in the answer's result element, and every message starts with the game's ID, secret key and login ticket.

static int Sake_GetServerError(const char *name);

// The declaration of the message's namespace
static const char *sSakeNamespaceDeclarations[] = {"ns1=\"http://gamespy.net/sake\""};

static int Sake_GetServerError(const char *name) {
    if (strcmp(name, "Success") == 0) {
        return SAKE_RESULT_SUCCESS;
    }
    if (strcmp(name, "SecretKeyInvalid") == 0) {
        return SAKE_RESULT_SECRET_KEY_INVALID;
    }
    if (strcmp(name, "ServiceDisabled") == 0) {
        return SAKE_RESULT_SERVICE_DISABLED;
    }
    if (strcmp(name, "DatabaseUnavailable") == 0) {
        return SAKE_RESULT_DATABASE_UNAVAILABLE;
    }
    if (strcmp(name, "LoginTicketInvalid") == 0) {
        return SAKE_RESULT_LOGIN_TICKET_INVALID;
    }
    if (strcmp(name, "LoginTicketExpired") == 0) {
        return SAKE_RESULT_LOGIN_TICKET_EXPIRED;
    }
    if (strcmp(name, "TableNotFound") == 0) {
        return SAKE_RESULT_TABLE_NOT_FOUND;
    }
    if (strcmp(name, "RecordNotFound") == 0) {
        return SAKE_RESULT_RECORD_NOT_FOUND;
    }
    if (strcmp(name, "FieldNotFound") == 0) {
        return SAKE_RESULT_FIELD_NOT_FOUND;
    }
    if (strcmp(name, "FieldTypeInvalid") == 0) {
        return SAKE_RESULT_FIELD_TYPE_INVALID;
    }
    if (strcmp(name, "NoPermission") == 0) {
        return SAKE_RESULT_NO_PERMISSION;
    }
    if (strcmp(name, "RecordLimitReached") == 0) {
        return SAKE_RESULT_RECORD_LIMIT_REACHED;
    }
    if (strcmp(name, "AlreadyRated") == 0) {
        return SAKE_RESULT_ALREADY_RATED;
    }
    if (strcmp(name, "NotRateable") == 0) {
        return SAKE_RESULT_NOT_RATEABLE;
    }
    if (strcmp(name, "NotOwned") == 0) {
        return SAKE_RESULT_NOT_OWNED;
    }
    if (strcmp(name, "FilterInvalid") == 0) {
        return SAKE_RESULT_FILTER_INVALID;
    }
    if (strcmp(name, "SortInvalid") == 0) {
        return SAKE_RESULT_SORT_INVALID;
    }
    if (strcmp(name, "TargetFilterInvalid") == 0) {
        return SAKE_RESULT_TARGET_FILTER_INVALID;
    }
    return SAKE_RESULT_UNKNOWN;
}

void Sake_OnResponse(int status, int unused, SoapReader *reader, void *userData) {
    SakeRequest *request = userData;
    char result[0x20];
    void *output = NULL;
    int error;

    if (request == NULL || request->sake == NULL || request->type == NULL) {
        return;
    }
    error = Sake_GetHttpResultError(status);
    if (error == SAKE_RESULT_SUCCESS) {
        if (!func_ov011_02170d18(reader) || !func_ov011_02170d28(reader, request->type->responseName)
            || !func_ov011_02170e90(reader, request->type->resultName, result, sizeof(result))) {
            error = SAKE_RESULT_MALFORMED_RESPONSE;
        } else {
            error = Sake_GetServerError(result);
            if (error == SAKE_RESULT_SUCCESS && request->type->outputSize != 0) {
                request->output = func_ov011_0216dddc(request->type->outputSize);
                if (request->output != NULL) {
                    request->response = reader;
                    error = request->type->parse(request);
                    if (error == SAKE_RESULT_SUCCESS) {
                        output = request->output;
                    }
                } else {
                    error = SAKE_RESULT_OUT_OF_MEMORY;
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
    func_ov011_0216ddfc(request->output);
    request->output = NULL;
    Sake_FreeRequest(request);
}

int Sake_BuildMessage(SakeRequest *request) {
    SakeRequestType *type = request->type;
    int error;

    error = type->validate(request);
    if (error != 0) {
        return error;
    }
    request->writer = func_ov011_0216fbf8(sSakeNamespaceDeclarations, 1);
    if (request->writer == NULL) {
        return SAKE_REQUEST_ERROR_OUT_OF_MEMORY;
    }
    func_ov011_02170614(request->writer, "ns1", request->type->name);
    func_ov011_02170818(request->writer, "ns1", "gameid", request->sake->gameId);
    func_ov011_021706b8(request->writer, "ns1", "secretKey", request->sake->secretKey);
    func_ov011_021706b8(request->writer, "ns1", "loginTicket", request->sake->loginTicket.data);
    error = type->build(request);
    if (error != 0) {
        func_ov011_0216fe64(request->writer);
        request->writer = NULL;
        if (type->freeOutput != NULL) {
            type->freeOutput(request);
        }
        return error;
    }
    func_ov011_02170660(request->writer, "ns1", request->type->name);
    func_ov011_0216fea8(request->writer);
    return 0;
}
