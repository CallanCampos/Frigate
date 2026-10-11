#include "nhttp_internal.h"

// NHTTP_request: making a request out of a URL, sending it and cancelling it

#define NHTTP_URL_LENGTH_MAX 0x1ff
#define NHTTP_URL_BUFFER_SIZE 0x200
#define NHTTP_ERROR_NO_MEMORY 1
#define NHTTP_ERROR_BAD_URL 4
#define NHTTP_ERROR_BAD_STATE 11

static void NHTTPi_freeHdrList(NHttpHeader *list);

// The length of the scheme in front of the host: http:// or https://
static inline int NHTTPi_schemeLength(NHttpRequest *request) {
    return request->https ? 8 : 7;
}

void NHTTPi_InitRequestInfo(NHttpReqInfo *info) {
    info->current = NULL;
}

// Makes a request for a URL, a method and the buffer that the body of the answer goes in. The URL is checked and
// kept with its %XX decoded, and the scheme (http:// or https://), the host, the port and the path are found in it
NHttpRequest *NHTTP_CreateRequest(NHttpState *state, const char *url, int method, void *buffer, int bufferSize,
                                  void *param, int unused, NHttpReleaseFunc release) {
    char text[NHTTP_URL_BUFFER_SIZE];
    int length;
    int schemeLength;
    int restLength;
    int sawSlash;
    const char *rest;
    int decodedLength;
    int escapes;
    int escapeState;
    int in;
    int out;
    char c;
    NHttpRequest *request = NULL;

    if (method != NHTTP_METHOD_GET && method != NHTTP_METHOD_POST && method != NHTTP_METHOD_HEAD) {
        NHTTPi_SetError(state, NHTTP_ERROR_BAD_STATE);
        goto fail;
    }
    request = NHTTPi_alloc(sizeof(NHttpRequest), 4);
    if (request == NULL) {
        NHTTPi_SetError(state, NHTTP_ERROR_NO_MEMORY);
        goto fail;
    }
    NHTTPi_memclr(request, sizeof(NHttpRequest));
    request->response = NHTTPi_alloc(sizeof(NHttpResponse), 4);
    if (request->response == NULL) {
        NHTTPi_SetError(state, NHTTP_ERROR_NO_MEMORY);
        goto fail;
    }
    NHTTPi_memclr(request->response, sizeof(NHttpResponse));
    request->response->buffer = buffer;
    request->response->bufferSize = bufferSize;
    request->response->unk_2C = unused;
    request->response->release = release;
    length = NHTTPi_strnlen(url, NHTTP_URL_LENGTH_MAX);
    if (length <= 7) {
        NHTTPi_SetError(state, NHTTP_ERROR_BAD_URL);
        goto fail;
    }
    NHTTPi_memclr(text, NHTTP_URL_BUFFER_SIZE);
    NHTTPi_memcpy(text, url, length);
    request->port = 80;
    schemeLength = 7;
    if (NHTTPi_strnicmp(text, "http://", 7) != 0) {
        schemeLength = 8;
        if (NHTTPi_strnicmp(text, "https://", 8) != 0) {
            NHTTPi_SetError(state, NHTTP_ERROR_BAD_URL);
            goto fail;
        }
        request->https = 1;
        request->port = 443;
    }
    rest = text + schemeLength;
    restLength = length - schemeLength;
    if (restLength <= 0) {
        NHTTPi_SetError(state, NHTTP_ERROR_BAD_URL);
        goto fail;
    }
    // The size of the URL without its escapes: a %XX is three bytes and becomes one. Past a slash that is a %2F, the
    // escapes are left as they are
    in = 0;
    escapes = 0;
    escapeState = 0;
    while (in < restLength && rest[in] != '/') {
        if (escapeState == 2) {
            escapeState--;
        } else if (escapeState == 1) {
            c = NHTTPi_strToHex(rest + (in - 1), 2);
            escapeState--;
            if (c < 0) {
                NHTTPi_SetError(state, NHTTP_ERROR_BAD_URL);
                goto fail;
            }
            if (c == '/') {
                escapes--;
                break;
            }
        } else if (rest[in] == '%') {
            escapeState = 2;
            escapes++;
        }
        in++;
    }
    if (escapeState != 0) {
        NHTTPi_SetError(state, NHTTP_ERROR_BAD_URL);
        goto fail;
    }
    decodedLength = schemeLength + restLength - escapes * 2;
    request->url = NHTTPi_alloc(decodedLength + 1, 4);
    if (request->url == NULL) {
        NHTTPi_SetError(state, NHTTP_ERROR_NO_MEMORY);
        goto fail;
    }
    NHTTPi_memclr(request->url, decodedLength + 1);
    NHTTPi_memcpy(request->url, text, schemeLength);
    sawSlash = 0;
    in = 0;
    out = 0;
    escapeState = 0;
    if (restLength > 0) {
        do {
            if (escapeState == 2) {
                escapeState--;
            } else if (escapeState == 1) {
                c = NHTTPi_strToHex(rest + (in - 1), 2);
                request->url[schemeLength + out - 1] = c;
                escapeState--;
                if (c == '/') {
                    sawSlash = 1;
                }
            } else {
                c = rest[in];
                if (c == '/') {
                    sawSlash = 1;
                }
                if (!sawSlash && c == '%') {
                    escapeState = 2;
                } else {
                    request->url[schemeLength + out] = c;
                }
                out++;
            }
            in++;
        } while (in < restLength);
    }
    request->url[schemeLength + out] = 0;
    // Where the host ends, and where the path starts, with the port between them if there is one
    rest = request->url + schemeLength;
    for (in = 0; in < out; in++) {
        if (rest[in] == '/' || rest[in] == ':') {
            request->hostEnd = in + schemeLength;
            break;
        }
    }
    if (in == out) {
        request->hostEnd = in + schemeLength;
        request->pathStart = request->hostEnd;
    } else if (rest[in] == '/') {
        request->pathStart = request->hostEnd;
    } else if (rest[in] == ':') {
        for (; in < out; in++) {
            if (rest[in] == '/') {
                request->pathStart = in + schemeLength;
                break;
            }
        }
        if (in == out) {
            request->pathStart = in + schemeLength;
        } else {
            int portStart = request->hostEnd + 1;
            int port = NHTTPi_strtonum(request->url + portStart, request->pathStart - portStart);

            if (port < 0) {
                port = request->port;
            } else if (port > 0xffff) {
                NHTTPi_SetError(state, NHTTP_ERROR_BAD_URL);
                goto fail;
            }
            request->port = (u16)port;
        }
    }
    decodedLength = request->hostEnd - NHTTPi_schemeLength(request);
    request->host = NHTTPi_alloc(decodedLength + 1, 4);
    if (request->host == NULL) {
        NHTTPi_SetError(state, NHTTP_ERROR_NO_MEMORY);
        goto fail;
    }
    NHTTPi_memclr(request->host, decodedLength + 1);
    NHTTPi_memcpy(request->host, request->url + NHTTPi_schemeLength(request), decodedLength);
    NHTTPi_memcpy(request->boundary, NHTTPi_boundary, NHTTP_BOUNDARY_SIZE);
    request->method = method;
    request->ssl = NULL;
    request->unk_B0 = 0;
    request->unk_B4 = 0;
    request->unk_B8 = 0;
    request->unk_BC = 0;
    request->rootCerts = NULL;
    request->rootCertCount = 0;
    request->unk_CC = 0;
    request->verifyOption = 0;
    request->skipVerify = 0;
    request->response->userParam = param;
    request->useProxy = 0;
    request->unk_248 = 0;
    return request;

fail:
    if (request != NULL) {
        if (request->url != NULL) {
            NHTTPi_free(request->url);
        }
        if (request->host != NULL) {
            NHTTPi_free(request->host);
        }
        if (request->response != NULL) {
            NHTTPi_free(request->response);
        }
        NHTTPi_free(request);
    }
    return NULL;
}

// Destroys a request after the connection that has it lets go of its response
void NHTTP_DestroyRequest(NHttpRoot *root, NHttpRequest *request) {
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);

    if (connection != NULL) {
        connection->response = NULL;
    }
    NHTTPi_free(request->response);
    NHTTPi_destroyRequestObject(mutexInfo, request);
}

// Frees a ring of headers or post data from its last part
static void NHTTPi_freeHdrList(NHttpHeader *list) {
    if (list != NULL) {
        do {
            if (list != list->prev) {
                NHttpHeader *last = list->prev;
                NHttpHeader *before = last->prev;

                NHTTPi_free(last);
                list->prev = before;
            } else {
                NHTTPi_free(list);
                list = NULL;
            }
        } while (list != NULL);
    }
}

BOOL NHTTPi_destroyRequestObject(NHttpMutexInfo *mutexInfo, NHttpRequest *request) {
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);

    if (connection != NULL) {
        connection->request = NULL;
    }
    NHTTPi_freeHdrList(request->headers);
    NHTTPi_freeHdrList(request->postData);
    NHTTPi_free(request->url);
    NHTTPi_free(request->host);
    NHTTPi_free(request);
    return TRUE;
}

// Sends a request to the thread, which takes it when the requests before it are done. Returns its id, or -1
int NHTTP_SendRequestAsync(NHttpRoot *root, NHttpRequest *request) {
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpThreadInfo *thread = NHTTPi_GetThreadInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    int id;

    if (request->sent != 0) {
        NHTTPi_SetError(state, NHTTP_ERROR_BAD_STATE);
        return -1;
    }
    NHTTPi_lockReqList(mutexInfo);
    id = NHTTPi_setReqQueue(NHTTPi_GetListInfoP(root), request);
    if (id >= 0) {
        request->sent = 1;
        NHTTPi_kickCommThread(&thread->queue);
    } else {
        NHTTPi_SetError(state, NHTTP_ERROR_NO_MEMORY);
    }
    NHTTPi_unlockReqList(mutexInfo);
    return id;
}

// Cancels the request that has an id: the one that runs is told to stop and its socket is shut down, one that waits is
// taken out of the list
BOOL NHTTP_CancelRequestAsync(NHttpRoot *root, int id) {
    BOOL canceled = FALSE;
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpQueueNode *current = reqInfo->current;

    NHTTPi_lockReqList(mutexInfo);
    if (current != NULL && current->id == id && current->request->canceled == 0) {
        current->request->canceled = 1;
        NHTTPi_SocCancel(mutexInfo, current->request, current->socket);
        canceled = TRUE;
    }
    if (!canceled) {
        canceled = NHTTPi_freeReqQueue(NHTTPi_GetListInfoP(root), mutexInfo, id);
    }
    NHTTPi_unlockReqList(mutexInfo);
    return canceled;
}

void NHTTPi_cancelAllRequests(NHttpRoot *root) {
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    NHttpListInfo *list = NHTTPi_GetListInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpQueueNode *current = reqInfo->current;

    NHTTPi_lockReqList(mutexInfo);
    if (current != NULL && current->request->canceled == 0) {
        current->request->canceled = 1;
        NHTTPi_SocCancel(mutexInfo, current->request, current->socket);
    }
    NHTTPi_allFreeReqQueue(list, mutexInfo);
    NHTTPi_unlockReqList(mutexInfo);
}
