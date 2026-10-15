#include "nhttp_internal.h"

// NHTTP_response: what a response has of the server's header, as the user asks for it

// Frees a response: the blocks of its header, the copies that were made of it, the buffer of the body (by the user's
// function) and the response itself
void NHTTP_DestroyResponse(NHttpMutexInfo *mutexInfo, NHttpResponse *response) {
    NHttpHeaderBlock *block = response->blocks;
    NHttpConnection *connection;

    if (block != NULL) {
        do {
            NHttpHeaderBlock *next = block->next;

            NHTTPi_free(block);
            block = next;
            response->blocks = next;
        } while (block != NULL);
    }
    if (response->headerCopy != NULL) {
        NHTTPi_free(response->headerCopy);
    }
    if (response->valueCopy != NULL) {
        NHTTPi_free(response->valueCopy);
    }
    if (response->release != NULL) {
        response->release(response->buffer, NHTTPi_free, response->userParam);
        response->buffer = NULL;
        response->bufferSize = 0;
    }
    connection = NHTTPi_Response2Connection(mutexInfo, response);
    if (connection != NULL) {
        connection->response = NULL;
    }
    NHTTPi_free(response);
}

// Finds a header field by its name. Gives the place of its value in valueStart and returns the value's length, or -1
int NHTTPi_getHeaderValue(NHttpResponse *response, const char *name, int *valueStart) {
    int colon;
    int lineEnd = 0;
    int line;
    int next;

    line = NHTTPi_findNextLineHdrRecvBuf(response, NHTTP_FIRST_FIELD, response->headerSize, &colon, &lineEnd);
    while (line > 0) {
        next = NHTTPi_findNextLineHdrRecvBuf(response, line, response->headerSize, &colon, &lineEnd);
        if (colon > 0 && NHTTPi_compareTokenN_HdrRecvBuf(response, line, colon, name, 0) == 0) {
            if (colon + 1 < response->headerSize) {
                int valueEnd;
                int start;

                next = NHTTPi_findNextLineHdrRecvBuf(response, colon + 1, response->headerSize, NULL, &lineEnd);
                if (next <= 0) {
                    valueEnd = response->headerSize;
                } else {
                    if (next < lineEnd) {
                        return -1;
                    }
                    valueEnd = next - lineEnd;
                }
                start = NHTTPi_skipSpaceHdrRecvBuf(response, colon + 1, valueEnd);
                if (start < 0) {
                    start = valueEnd;
                }
                *valueStart = start;
                return valueEnd - start;
            }
            return 0;
        }
        line = next;
    }
    return -1;
}

// Copies the value of a header field (a text that stays until the next call), and gives its length. The name
// HTTPSTATUSCODE gives the status code of the answer
int NHTTP_GetHeaderField(NHttpResponse *response, const char *name, char **value) {
    int start;
    int length;

    if (!response->headerDone) {
        return -1;
    }
    if (response->valueCopy != NULL) {
        NHTTPi_free(response->valueCopy);
        response->valueCopy = NULL;
    }
    length = NHTTPi_getHeaderValue(response, name, &start);
    if (length >= 0) {
        response->valueCopy = NHTTPi_alloc(length + 1, 4);
        if (response->valueCopy == NULL) {
            return -1;
        }
        response->valueCopy[length] = 0;
        NHTTPi_loadFromHdrRecvBuf(response, response->valueCopy, start, length);
        *value = response->valueCopy;
        return length;
    }
    if (NHTTPi_strcmp("HTTPSTATUSCODE", name) == 0) {
        response->valueCopy = NHTTPi_alloc(4, 4);
        if (response->valueCopy == NULL) {
            return -1;
        }
        response->valueCopy[3] = 0;
        NHTTPi_loadFromHdrRecvBuf(response, response->valueCopy, 9, 3);
        *value = response->valueCopy;
        return 3;
    }
    return -1;
}

// Gives the whole header: the part of the response if it fits there, or a copy
int NHTTP_GetHeaderAll(NHttpState *state, NHttpResponse *response, char **header) {
    if (!response->headerDone || response->headerSize == 0) {
        return -1;
    }
    if (response->headerSize < NHTTP_HEADER_SIZE) {
        *header = response->header;
    } else {
        if (response->headerCopy == NULL) {
            response->headerCopy = NHTTPi_alloc(response->headerSize, 4);
            if (response->headerCopy == NULL) {
                NHTTPi_SetError(state, 1);
                return -1;
            }
            NHTTPi_loadFromHdrRecvBuf(response, response->headerCopy, 0, response->headerSize);
        }
        *header = response->headerCopy;
    }
    return response->headerSize;
}
