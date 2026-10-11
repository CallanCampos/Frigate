#include "nhttp_internal.h"

// d_nhttp: the library's interface for the game (the calls of dwc/nhttp.h). A connection is what NHTTPCreateConnection
// gives, its handle is the address of the NHttpConnection

static int NHTTPAddHeaderFieldToRequest(NHttpRequest *request, const char *name, const char *value);
static int NHTTPi_SetRootCA(NHttpRequest *request, void *certificates, int count);

int NHTTPStartup(NHttpAllocFunc alloc, NHttpFreeFunc free, u32 priority) {
    return -(NHTTPi_Startup(NHTTPi_GetSystemInfoP(), alloc, free, priority) == 0);
}

void NHTTPCleanupAsync(void (*callback)(void)) {
    NHTTPi_CleanupAsync(NHTTPi_GetSystemInfoP(), callback);
}

void NHTTPCleanup(void) {
    NHTTPi_CleanupAsync(NHTTPi_GetSystemInfoP(), NULL);
}

int NHTTPGetError(void) {
    return NHTTPi_GetError(&NHTTPi_GetSystemInfoP()->state);
}

int NHTTPAddHeaderField(int connection, const char *name, const char *value) {
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());

    return NHTTPAddHeaderFieldToRequest(NHTTPi_GetRequest(mutexInfo, (NHttpConnection *)connection), name, value);
}

static int NHTTPAddHeaderFieldToRequest(NHttpRequest *request, const char *name, const char *value) {
    NHttpState *state = NHTTPi_GetBgnEndInfoP(NHTTPi_GetSystemInfoP());

    if (request == NULL) {
        return -1;
    }
    if (request->sent != 0) {
        return -1;
    }
    return -(NHTTP_AddHeaderField(request, state, name, value) == 0);
}

int NHTTPAddPostDataAscii(int connection, const char *name, const char *value) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpRequest *request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(root), (NHttpConnection *)connection);

    return -(NHTTP_AddPostDataAscii(request, state, name, value) == 0);
}

// Sets the data that a request posts as it is, instead of as parts
int NHTTPSetPostDataRaw(int connection, const void *data, u32 size) {
    NHttpRequest *request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)connection);

    if (request == NULL) {
        return -1;
    }
    if (request->sent != 0) {
        return -1;
    }
    if (request->postData != NULL) {
        return -1;
    }
    request->rawPost = TRUE;
    request->rawData = data;
    request->rawSize = size;
    return 0;
}

int NHTTPGetHeaderField(int connection, const char *name, char **value) {
    NHttpResponse *response =
        NHTTPi_GetResponse(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)connection);

    if (response == NULL) {
        return -1;
    }
    return NHTTP_GetHeaderField(response, name, value);
}

int NHTTPGetHeaderAll(int connection, char **header) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpResponse *response = NHTTPi_GetResponse(NHTTPi_GetMutexInfoP(root), (NHttpConnection *)connection);

    if (response == NULL) {
        return -1;
    }
    return NHTTP_GetHeaderAll(state, response, header);
}

// The status of the answer, once its header has come, or -1
int NHTTPGetResultCode(int connection) {
    NHttpResponse *response =
        NHTTPi_GetResponse(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)connection);

    if (response == NULL) {
        return -1;
    }
    if (response->headerDone) {
        return response->statusCode;
    }
    return -1;
}

int NHTTPSetVerifyOption(int connection, int option) {
    NHttpRequest *request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)connection);

    if (request == NULL) {
        return -1;
    }
    request->verifyOption = option;
    return 0;
}

// Sets the user and the password of the request's basic authorization (32 characters at most each)
int NHTTPSetBasicAuthorization(int connection, const char *user, const char *password) {
    NHttpRequest *request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)connection);
    char text[0x41];
    int userLength;
    int passwordLength;

    if (request == NULL || user == NULL || password == NULL) {
        return -1;
    }
    userLength = NHTTPi_strnlen(user, 0x21);
    passwordLength = NHTTPi_strnlen(password, 0x21);
    if (userLength <= 0x20 && passwordLength <= 0x20) {
        NHTTPi_memclr(text, sizeof(text));
        NHTTPi_memcpy(text, user, userLength);
        NHTTPi_memcpy(text + userLength, ":", 1);
        NHTTPi_memcpy(text + (userLength + 1), password, passwordLength);
        request->authorizationSize = NHTTPi_Base64Encode(request->authorization, text);
        return 0;
    }
    return -1;
}

int NHTTPSetRootCA(int connection, void *certificates, int count) {
    NHttpRequest *request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)connection);

    return NHTTPi_SetRootCA(request, certificates, count);
}

static int NHTTPi_SetRootCA(NHttpRequest *request, void *certificates, int count) {
    if (request == NULL) {
        return -1;
    }
    request->unk_C8 = 0;
    request->unk_DC = 0;
    request->rootCerts = certificates;
    request->rootCertCount = count;
    return 0;
}
