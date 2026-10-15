#include "nhttp_internal.h"

// NHTTP_thread: what the thread does to run a request: it looks the host up, connects (through the proxy if there is
// one), sends the header and the post data, reads the header and then the body

// The texts of the headers of a post, which go to the read-only data
static const char sContentDisposition[] = "Content-Disposition: form-data; name=\"";
static const char sContentTypeMultipart[] = "Content-Type: multipart/form-data; boundary=";
static const char sContentTypeUrlEncoded[] = "Content-Type: application/x-www-form-urlencoded\r\n";
static const char sContentTypeOctetStream[] =
    "Content-Type: application/octet-stream\r\nContent-Transfer-Encoding: binary\r\n";

static int NHTTPi_SendHeaderData(NHttpThreadWork *work, const void *data, int size);

void NHTTPi_InitThreadInfo(NHttpThreadInfo *info) {
    NHTTPi_IsCreateCommThreadMessageQueueOff(info);
}

void NHTTPi_IsCreateCommThreadMessageQueueOn(NHttpThreadInfo *info) {
    info->queueCreated = TRUE;
}

void NHTTPi_IsCreateCommThreadMessageQueueOff(NHttpThreadInfo *info) {
    info->queueCreated = FALSE;
}

BOOL NHTTPi_IsCreateCommThreadMessageQueue(NHttpThreadInfo *info) {
    return info->queueCreated;
}

// Whether the last four bytes that were read (kept in a ring of four) end the header: an empty line, as CR CR, LF LF or
// CR LF CR LF
BOOL NHTTPi_CheckHeaderEnd(const char *tail, int count) {
    char second = tail[(count - 2) & 3];

    if (second == '\r' && tail[(count - 1) & 3] == '\r') {
        return TRUE;
    }
    if (second == '\n' && tail[(count - 1) & 3] == '\n') {
        return TRUE;
    }
    if (tail[(count - 4) & 3] == '\r' && tail[(count - 3) & 3] == '\n' && second == '\r' &&
        tail[(count - 1) & 3] == '\n') {
        return TRUE;
    }
    return FALSE;
}

// Puts data in the thread's send buffer, and sends the buffer when it is full. Returns the size of the data, or what
// the socket library returned if that was not more than 0
int NHTTPi_SaveBuf(NHttpRequest *request, u8 *sendBuffer, int sock, int *count, const u8 *data, const int size) {
    int room;
    int result;
    int rest = size;

    while (rest > 0) {
        if (request->canceled != 0) {
            return -1;
        }
        room = NHTTP_SEND_BUFFER_SIZE - *count;
        if (rest <= room) {
            room = rest;
        }
        NHTTPi_memcpy(sendBuffer + *count, data, room);
        data += room;
        rest -= room;
        *count += room;
        if (*count == NHTTP_SEND_BUFFER_SIZE) {
            result = NHTTPi_SocSend(request, sock, sendBuffer, NHTTP_SEND_BUFFER_SIZE, 0);
            if (result <= 0) {
                return result;
            }
            *count -= result;
        }
    }
    return size;
}

// Reads one byte at a time up to the end of a line, which is CR LF. Returns the number of bytes, or what the socket
// returned if that was not more than 0
static int NHTTPi_SkipLine(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock) {
    char tail[2];
    int count;
    int total = 0;

    tail[0] = 0;
    tail[1] = 0;
    count = 0;
    while (!(tail[count & 1] == '\r' && tail[(count - 1) & 1] == '\n')) {
        int result = NHTTPi_SocRecv(mutexInfo, request, sock, &tail[count & 1], 1, 0);

        if (result <= 0) {
            return result;
        }
        total += result;
        count++;
    }
    return total;
}

// Adds the size of what the connection's callback gives for a part of the post data, as it will be sent (a part sent
// as text is longer when it is encoded). Returns FALSE if the callback can't be called
static BOOL NHTTPi_GetPostContentlength(NHttpMutexInfo *mutexInfo, NHttpRequest *request, const char *name,
                                        int *length, int mode) {
    int total = 0;
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);

    if (connection == NULL) {
        return FALSE;
    }
    connection->userBuffer = NULL;
    for (;;) {
        int size;
        const char *buffer;

        if (request->canceled != 0) {
            return FALSE;
        }
        connection->userBufferSize = 0;
        if (NHTTPi_PostSendCallback(mutexInfo, connection, name, total) < 0) {
            return FALSE;
        }
        size = connection->userBufferSize;
        buffer = connection->userBuffer;
        if (size == 0) {
            break;
        }
        if (buffer == NULL) {
            return FALSE;
        }
        total += size;
        switch (mode) {
        case NHTTP_SEND_RAW:
        case NHTTP_SEND_PART:
            *length += size;
            break;
        case NHTTP_SEND_ENCODED:
            *length += NHTTPi_getUrlEncodedSize2(buffer, size);
            break;
        }
    }
    return TRUE;
}

// Sends what the connection's callback gives for a part of the post data. Returns 0 if it all went into the send buffer,
// 1 if the socket failed, 2 if the server closed it and 3 if the connection is gone or the request canceled
static int NHTTPi_SendPostData(NHttpMutexInfo *mutexInfo, NHttpRequest *request, u8 *sendBuffer, const char *name,
                               int sock, int *count, int mode) {
    char encoded[3];
    int total = 0;
    NHttpConnection *connection;

    connection = NHTTPi_Request2Connection(mutexInfo, request);
    if (connection == NULL) {
        return 3;
    }
    connection->userBuffer = NULL;
    for (;;) {
        u32 size;
        const char *buffer;
        int result;

        if (request->canceled != 0) {
            return 3;
        }
        connection->userBufferSize = 0;
        if (NHTTPi_PostSendCallback(mutexInfo, connection, name, total) < 0) {
            return 3;
        }
        buffer = connection->userBuffer;
        size = connection->userBufferSize;
        if (size == 0) {
            break;
        }
        if (buffer == NULL) {
            return 3;
        }
        total += size;
        switch (mode) {
        case NHTTP_SEND_RAW:
        case NHTTP_SEND_PART:
            result = NHTTPi_SaveBuf(request, sendBuffer, sock, count, buffer, size);
            if (result < 0) {
                return 1;
            }
            if (result == 0) {
                return 2;
            }
            break;
        case NHTTP_SEND_ENCODED: {
            u32 i;

            for (i = 0; i < size; i++) {
                NHTTPi_memclr(encoded, 3);
                result = NHTTPi_SaveBuf(request, sendBuffer, sock, count, encoded,
                                        NHTTPi_encodeUrlChar(encoded, buffer[i]));
                if (result < 0) {
                    return 1;
                }
                if (result == 0) {
                    return 2;
                }
            }
            break;
        }
        }
    }
    return 0;
}

// Whether the response has room for what is read, asking the connection's callback for a buffer when it has none or it
// is full
static BOOL NHTTPi_BufFull(NHttpMutexInfo *mutexInfo, NHttpResponse *response) {
    BOOL ready = FALSE;
    BOOL full = NHTTPi_isRecvBufFull(response, response->bufferUsed);

    if (response->bufferSize == 0 || response->buffer == NULL || full) {
        NHttpConnection *connection = NHTTPi_Response2Connection(mutexInfo, response);

        if (connection != NULL) {
            NHTTPi_BufferFullCallback(mutexInfo, connection);
            if (response->buffer != NULL && response->bufferSize != 0) {
                if (!NHTTPi_isRecvBufFull(response, response->bufferUsed)) {
                    ready = TRUE;
                }
            }
        }
    } else if (!full) {
        ready = TRUE;
    }
    return ready;
}

// Puts data in the send buffer for the request that runs. Returns 0, or 1 if the socket failed and 2 if the server
// closed it
static int NHTTPi_SendHeaderData(NHttpThreadWork *work, const void *data, int size) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpThreadInfo *thread = NHTTPi_GetThreadInfoP(root);
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    u8 *sendBuffer = thread->sendBuffer;
    int result;

    if (size == 0) {
        return 0;
    }
    result = NHTTPi_SaveBuf(request, sendBuffer, state->socket, &work->sendCount, data, size);
    if (result < 0) {
        return 1;
    }
    if (result == 0) {
        return 2;
    }
    return 0;
}

// Sends the Proxy-Authorization header field if the request has the user and the password of the proxy
static int NHTTPi_SendProxyAuthorization(NHttpThreadWork *work) {
    NHttpRequest *request = NHTTPi_GetReqInfoP(NHTTPi_GetSystemInfoP())->current->request;
    int result;

    if (request->proxyAuthorizationSize == 0) {
        return 0;
    }
    result = NHTTPi_SendHeaderData(work, "Proxy-Authorization: Basic ", 27);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, request->proxyAuthorization, request->proxyAuthorizationSize);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    return 0;
}

static int NHTTPi_SendAuthorization(NHttpThreadWork *work) {
    NHttpRequest *request = NHTTPi_GetReqInfoP(NHTTPi_GetSystemInfoP())->current->request;
    int result;

    if (request->authorizationSize == 0) {
        return 0;
    }
    result = NHTTPi_SendHeaderData(work, "Authorization: Basic ", 21);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, request->authorization, request->authorizationSize);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    return 0;
}

// Sends the request that asks the proxy to connect to the server (CONNECT), for a secure connection
static int NHTTPi_SendProxyConnectMethod(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    u8 *sendBuffer = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    char port[8];
    int portLength = NHTTPi_intToStr(port, request->port);
    int result;

    result = NHTTPi_SendHeaderData(work, "CONNECT ", 8);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, request->url + 8, request->hostEnd - 8);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, ":", 1);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, port, portLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, " HTTP/1.1\r\n", 11);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "Host: ", 6);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, request->url + 8, request->hostEnd - 8);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, ":", 1);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, port, portLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "Content-Length: 0\r\nPragma: no-cache\r\n", 37);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendProxyAuthorization(work);
    if (result != 0) {
        return result;
    }
    // The result of the empty line that ends the header is not checked
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (work->sendCount > 0) {
        result = NHTTPi_SocSend(request, state->socket, sendBuffer, work->sendCount, 0);
        if (result < 0) {
            return 1;
        }
        if (result == 0) {
            return 2;
        }
    }
    work->sendCount = 0;
    NHTTPi_memclr(sendBuffer, NHTTP_SEND_BUFFER_SIZE);
    return 0;
}

// Reads the proxy's answer to CONNECT. TRUE if the status is 200
static BOOL NHTTPi_RecvProxyConnectHeader(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    u8 *sendBuffer = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpRequest *request = reqInfo->current->request;
    NHttpResponse *response = request->response;
    int count;
    BOOL ok = FALSE;
    int received = 0;
    char buffer[0x200];

    for (;;) {
        int i;
        BOOL found;

        count = NHTTPi_SocRecv(mutexInfo, request, state->socket, buffer + received, 0x200 - received, 0);
        received += count;
        response->statusCode = NHTTPi_strToInt(buffer + 9, 3);
        if (NHTTPi_strnicmp(buffer, "HTTP/", 5) == 0 && buffer[8] == ' ' && response->statusCode == 200) {
            ok = TRUE;
        }
        for (i = 0, found = FALSE; i < received; i++) {
            if (i > 1 && buffer[i - 1] == '\r' && buffer[i] == '\r') {
                found = TRUE;
            } else if (i > 1 && buffer[i - 1] == '\n' && buffer[i] == '\n') {
                found = TRUE;
            } else if (i > 3 && buffer[i - 3] == '\r' && buffer[i - 2] == '\n' && buffer[i - 1] == '\r' &&
                       buffer[i] == '\n') {
                found = TRUE;
            }
        }
        if (found) {
            return ok ? TRUE : FALSE;
        }
        if (count < 0) {
            return FALSE;
        }
        if (received >= 0x200) {
            count = NHTTPi_SocRecv(mutexInfo, request, state->socket, sendBuffer, 1, 0);
            if (count < 0) {
                return FALSE;
            }
            if (count != 0) {
                return FALSE;
            }
        }
    }
}

// Sends the header fields that the request was given, taking each off the list
static int NHTTPi_SendHeaderList(NHttpThreadWork *work) {
    NHttpRequest *request = NHTTPi_GetReqInfoP(NHTTPi_GetSystemInfoP())->current->request;
    NHttpHeader *header = NHTTPi_getHdrFromList(&request->headers);
    int result;

    if (header != NULL) {
        do {
            result = NHTTPi_SendHeaderData(work, header->name, NHTTPi_strlen(header->name));
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, ": ", 2);
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, header->value, NHTTPi_strlen(header->value));
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, "\r\n", 2);
            if (result != 0) {
                return result;
            }
            NHTTPi_free(header);
            header = NHTTPi_getHdrFromList(&request->headers);
        } while (header != NULL);
    }
    return 0;
}

// Sends a post that is one block of data (the body of the request), with its Content-Length
static int NHTTPi_SendProcPostDataRaw(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    u8 *sendBuffer = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    int length = 0;
    char text[12];
    int textLength;
    int result;

    if (request->rawData == NULL) {
        if (!NHTTPi_GetPostContentlength(mutexInfo, request, NULL, &length, NHTTP_SEND_RAW)) {
            return 3;
        }
    } else {
        length = request->rawSize;
    }
    textLength = NHTTPi_intToStr(text, length);
    result = NHTTPi_SendHeaderData(work, "Content-Length: ", 16);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, text, textLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    if (request->rawData == NULL) {
        result =
            NHTTPi_SendPostData(mutexInfo, request, sendBuffer, NULL, state->socket, &work->sendCount, NHTTP_SEND_RAW);
        if (result != 0) {
            return result;
        }
    } else {
        result = NHTTPi_SendHeaderData(work, request->rawData, request->rawSize);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

// Sends a post as multipart/form-data: each part goes between boundaries with its name, its content type if it is
// binary and its data
static int NHTTPi_SendProcPostDataBinary(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    NHttpThreadInfo *thread = NHTTPi_GetThreadInfoP(root);
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    int length = 0;
    NHttpHeader *part;
    char text[12];
    int textLength;
    int result;

    part = request->postData;
    if (part != NULL) {
        do {
            length += NHTTP_BOUNDARY_SIZE + 2;
            length += NHTTPi_strlen(part->name) + 41;
            if (part->binary) {
                length += 75;
            }
            length += 2;
            if (part->value == NULL) {
                if (!NHTTPi_GetPostContentlength(mutexInfo, request, part->name, &length, NHTTP_SEND_PART)) {
                    return 3;
                }
            } else {
                length += part->length;
            }
            length += 2;
            if (part == request->postData->prev) {
                break;
            }
            part = part->next;
        } while (part != NULL);
    }
    length += NHTTP_BOUNDARY_SIZE + 4;
    textLength = NHTTPi_intToStr(text, length);
    result = NHTTPi_SendHeaderData(work, sContentTypeMultipart, sizeof(sContentTypeMultipart) - 1);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, request->boundary + 2, NHTTP_BOUNDARY_SIZE - 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "Content-Length: ", 16);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, text, textLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    part = request->postData;
    if (part != NULL) {
        do {
            result = NHTTPi_SendHeaderData(work, request->boundary, NHTTP_BOUNDARY_SIZE);
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, "\r\n", 2);
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, sContentDisposition, sizeof(sContentDisposition) - 1);
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, part->name, NHTTPi_strlen(part->name));
            if (result != 0) {
                return result;
            }
            result = NHTTPi_SendHeaderData(work, "\"\r\n", 3);
            if (result != 0) {
                return result;
            }
            if (part->binary) {
                result = NHTTPi_SendHeaderData(
                    work, sContentTypeOctetStream, sizeof(sContentTypeOctetStream) - 1);
                if (result != 0) {
                    return result;
                }
            }
            result = NHTTPi_SendHeaderData(work, "\r\n", 2);
            if (result != 0) {
                return result;
            }
            if (part->value == NULL) {
                result = NHTTPi_SendPostData(mutexInfo, request, thread->sendBuffer, part->name, state->socket,
                                             &work->sendCount, NHTTP_SEND_PART);
                if (result != 0) {
                    return result;
                }
            } else {
                result = NHTTPi_SendHeaderData(work, part->value, part->length);
                if (result != 0) {
                    return result;
                }
            }
            result = NHTTPi_SendHeaderData(work, "\r\n", 2);
            if (result != 0) {
                return result;
            }
            if (part == request->postData->prev) {
                break;
            }
            part = part->next;
        } while (part != NULL);
    }
    result = NHTTPi_SendHeaderData(work, request->boundary, NHTTP_BOUNDARY_SIZE);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "--\r\n", 4);
    if (result != 0) {
        return result;
    }
    return 0;
}

// Sends a post as application/x-www-form-urlencoded: name=value pairs with & between them, URL encoded
static int NHTTPi_SendProcPostDataAscii(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    u8 *sendBuffer = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    int length = 0;
    NHttpHeader *part;
    char text[12];
    int textLength;
    int result;

    part = request->postData;
    if (part != NULL) {
        do {
            length = length + NHTTPi_getUrlEncodedSize(part->name) + 1;
            if (part->value == NULL) {
                if (!NHTTPi_GetPostContentlength(mutexInfo, request, part->name, &length, NHTTP_SEND_ENCODED)) {
                    return 3;
                }
            } else {
                length += NHTTPi_getUrlEncodedSize(part->value);
            }
            if (part == request->postData->prev) {
                break;
            }
            length += 1;
            part = part->next;
        } while (part != NULL);
    }
    textLength = NHTTPi_intToStr(text, length);
    result = NHTTPi_SendHeaderData(work, sContentTypeUrlEncoded, sizeof(sContentTypeUrlEncoded) - 1);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "Content-Length: ", 16);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, text, textLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    part = request->postData;
    if (part != NULL) {
        do {
            int i;

            for (i = 0; part->name[i] != 0; i++) {
                result = NHTTPi_SendHeaderData(work, text, NHTTPi_encodeUrlChar(text, part->name[i]));
                if (result != 0) {
                    return result;
                }
            }
            result = NHTTPi_SendHeaderData(work, "=", 1);
            if (result != 0) {
                return result;
            }
            if (part->value == NULL) {
                result = NHTTPi_SendPostData(mutexInfo, request, sendBuffer, part->name, state->socket,
                                             &work->sendCount, NHTTP_SEND_ENCODED);
                if (result != 0) {
                    return result;
                }
            } else {
                for (i = 0; part->value[i] != 0; i++) {
                    result = NHTTPi_SendHeaderData(work, text, NHTTPi_encodeUrlChar(text, part->value[i]));
                    if (result != 0) {
                        return result;
                    }
                }
            }
            if (part == request->postData->prev) {
                break;
            }
            result = NHTTPi_SendHeaderData(work, "&", 1);
            if (result != 0) {
                return result;
            }
            part = part->next;
        } while (part != NULL);
    }
    return 0;
}

static void NHTTPi_InitThreadWork(NHttpThreadWork *work) {
    work->id = -1;
    NHTTPi_memclr(work->hostName, sizeof(work->hostName));
    NHTTPi_memclr(work->unk_104, sizeof(work->unk_104));
    work->address = -1;
    work->prevAddress = -1;
    work->sendCount = 0;
    work->keepAlive = FALSE;
    work->chunked = FALSE;
    work->retry = 0;
    work->contentLength = 0;
    work->error = 0;
}

// Ends the request that runs: closes its socket unless the connection is kept, records how it ended, destroys the
// request and tells the user's callback
static void NHTTPi_ThreadReqEnd(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpRequest *request = reqInfo->current->request;
    NHttpResponse *response = request->response;
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);

    if (request->canceled != 0) {
        work->error = 8;
        work->keepAlive = FALSE;
    }
    if (work->keepAlive == FALSE) {
        if (state->socket >= 0) {
            if (NHTTPi_SocClose(mutexInfo, request, state->socket) < 0) {
                work->error = 10;
            }
            state->socket = -1;
        }
    }
    if (work->error == 0) {
        response->succeeded = TRUE;
    } else {
        response->succeeded = FALSE;
        NHTTPi_SetError(state, work->error);
        if (response->buffer == work->unk_104) {
            response->buffer = NULL;
            response->bufferSize = 0;
        }
    }
    if (connection != NULL) {
        connection->status = work->error;
    }
    NHTTPi_lockReqList(mutexInfo);
    NHTTPi_free(reqInfo->current);
    reqInfo->current = NULL;
    NHTTPi_unlockReqList(mutexInfo);
    NHTTPi_destroyRequestObject(mutexInfo, request);
    if (connection != NULL && response->succeeded) {
        connection->stage = NHTTP_STAGE_DONE;
    }
    NHTTPi_CompleteCallback(mutexInfo, connection);
    if (connection != NULL) {
        NHTTPi_WakeConnection(connection);
    }
    NHTTPi_DestroyConnection(connection);
}

// Takes the request that was sent first. FALSE (after waiting for a message) when there is none
static BOOL NHTTPi_ThreadExecReqQueue(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpQueueNode *node;

    NHTTPi_lockReqList(mutexInfo);
    node = NHTTPi_getReqFromReqQueue(NHTTPi_GetListInfoP(root));
    if (node != NULL) {
        NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);

        work->id = node->id;
        reqInfo->current = node;
    } else {
        work->id = -1;
    }
    NHTTPi_unlockReqList(mutexInfo);
    if (work->id < 0) {
        NHTTPi_idleCommThread(&NHTTPi_GetThreadInfoP(root)->queue);
        return FALSE;
    }
    return TRUE;
}

// Looks the host of the request up, or its proxy, unless it is the host of the last request
static BOOL NHTTPi_ThreadHostAddrProc(NHttpThreadWork *work) {
    NHttpRequest *request = NHTTPi_GetReqInfoP(NHTTPi_GetSystemInfoP())->current->request;
    char *host = request->host;

    if (request->useProxy) {
        host = request->proxyHost;
    }
    if (NHTTPi_strlen(host) == 0 || NHTTPi_strcmp(host, work->hostName) != 0) {
        work->address = NHTTPi_resolveHostname(request, host);
        if (work->address == 0) {
            if (request->useProxy) {
                work->error = 12;
                return FALSE;
            }
            work->error = 4;
            return FALSE;
        }
    } else {
        work->address = work->prevAddress;
    }
    NHTTPi_memclr(work->hostName, sizeof(work->hostName));
    NHTTPi_memcpy(work->hostName, host, NHTTPi_strnlen(host, 0xff));
    work->port = request->port;
    if (request->useProxy) {
        work->port = request->proxyPort;
    }
    if (work->address != work->prevAddress || work->port != work->prevPort || request->https == 1) {
        work->keepAlive = FALSE;
    }
    work->prevAddress = work->address;
    work->prevPort = work->port;
    return TRUE;
}

// Opens the socket and connects it to the host or the proxy, unless the connection of the last request is kept
static BOOL NHTTPi_ThreadConnectProc(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    NHttpRequest *request = reqInfo->current->request;
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);

    if (work->keepAlive == TRUE) {
        if (NHTTPi_SocCheck(state->socket) == -1) {
            work->keepAlive = FALSE;
        }
    }
    if (work->keepAlive == FALSE) {
        if (state->socket >= 0) {
            if (NHTTPi_SocClose(mutexInfo, request, state->socket) < 0) {
                state->socket = -1;
                work->error = 10;
                return FALSE;
            }
        }
        state->socket = NHTTPi_SocOpen(request);
        if (state->socket < 0) {
            work->error = 3;
            return FALSE;
        }
        NHTTPi_lockReqList(mutexInfo);
        reqInfo->current->socket = state->socket;
        NHTTPi_unlockReqList(mutexInfo);
        if (request->canceled != 0) {
            return FALSE;
        }
        if (NHTTPi_SocConnect(state, mutexInfo, request, state->socket, work->address, work->port) < 0) {
            if (request->useProxy) {
                work->error = 13;
                return FALSE;
            }
            if (NHTTPi_GetSSLError(state) != 0) {
                work->error = 14;
                return FALSE;
            }
            work->error = 5;
            return FALSE;
        }
    } else {
        NHTTPi_lockReqList(mutexInfo);
        reqInfo->current->socket = state->socket;
        NHTTPi_unlockReqList(mutexInfo);
    }
    return TRUE;
}

// For a secure connection through a proxy: asks the proxy to connect and makes the connection secure. 0 to go on, 1
// when it failed, 2 when the proxy closed the connection
static int NHTTPi_ThreadProxyProc(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpRequest *request = reqInfo->current->request;
    int result;

    work->error = 10;
    work->sendCount = 0;
    if (request->https && request->useProxy) {
        result = NHTTPi_SendProxyConnectMethod(work);
        if (result != 0) {
            return result;
        }
        if (!NHTTPi_RecvProxyConnectHeader(work)) {
            return 1;
        }
        result = NHTTPi_SocSSLConnect(state, mutexInfo, request, state->socket);
        if (result != 0) {
            if (result == -1004) {
                if (NHTTPi_GetSSLError(state) != 0) {
                    work->error = 16;
                }
                return 1;
            }
            if (result == -1005) {
                if (NHTTPi_GetSSLError(state) != 0) {
                    work->error = 17;
                }
                return 1;
            }
            if (NHTTPi_GetSSLError(state) != 0) {
                work->error = 14;
            }
            return 1;
        }
    }
    return 0;
}

// Sends the request line, the header fields and the post data. 0 when it is all sent, 1 or 3 when it failed and 2
// when the server closed the connection
static int NHTTPi_ThreadSendProc(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);
    u8 *sendBuffer = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    int urlLength = NHTTPi_strlen(request->url);
    int result = 0;
    int schemeLength;

    work->error = 10;
    if (connection != NULL) {
        connection->stage = NHTTP_STAGE_SENDING;
    }
    work->sendCount = 0;
    switch (request->method) {
    case NHTTP_METHOD_GET:
        result = NHTTPi_SendHeaderData(work, "GET ", 4);
        break;
    case NHTTP_METHOD_POST:
        result = NHTTPi_SendHeaderData(work, "POST ", 5);
        break;
    case NHTTP_METHOD_HEAD:
        result = NHTTPi_SendHeaderData(work, "HEAD ", 5);
        break;
    }
    if (result != 0) {
        return result;
    }
    if (request->useProxy && !request->https) {
        result = NHTTPi_SendHeaderData(work, request->url, urlLength);
        if (result != 0) {
            return result;
        }
    } else if (urlLength > request->pathStart) {
        result = NHTTPi_SendHeaderData(work, request->url + request->pathStart, urlLength - request->pathStart);
        if (result != 0) {
            return result;
        }
    } else {
        result = NHTTPi_SendHeaderData(work, "/", 1);
        if (result != 0) {
            return result;
        }
    }
    result = NHTTPi_SendHeaderData(work, " HTTP/1.1\r\n", 11);
    if (result != 0) {
        return result;
    }
    schemeLength = request->https ? 8 : 7;
    result = NHTTPi_SendHeaderData(work, "Host: ", 6);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, request->url + schemeLength, request->hostEnd - schemeLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderData(work, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    if (request->useProxy && !request->https) {
        result = NHTTPi_SendProxyAuthorization(work);
        if (result != 0) {
            return result;
        }
    }
    result = NHTTPi_SendAuthorization(work);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_SendHeaderList(work);
    if (result != 0) {
        return result;
    }
    if (request->method == NHTTP_METHOD_POST) {
        BOOL multipart;

        if (request->rawPost) {
            result = NHTTPi_SendProcPostDataRaw(work);
        } else {
            if (request->postMode == NHTTP_POST_AUTO) {
                NHttpHeader *part = request->postData;

                multipart = FALSE;
                if (part != NULL) {
                    do {
                        if (part->binary) {
                            multipart = TRUE;
                            break;
                        }
                        if (part == request->postData->prev) {
                            break;
                        }
                        part = part->next;
                    } while (part != NULL);
                }
            } else if (request->postMode == NHTTP_POST_MULTIPART) {
                multipart = TRUE;
            } else {
                multipart = FALSE;
            }
            if (multipart) {
                result = NHTTPi_SendProcPostDataBinary(work);
            } else {
                result = NHTTPi_SendProcPostDataAscii(work);
            }
        }
        if (result != 0) {
            if (result == 3) {
                work->error = 3;
                return result;
            }
            return result;
        }
    } else {
        result = NHTTPi_SendHeaderData(work, "\r\n", 2);
        if (result != 0) {
            return result;
        }
    }
    result = 0;
    if (work->sendCount > 0) {
        int sent = NHTTPi_SocSend(request, state->socket, sendBuffer, work->sendCount, 0);

        work->sendCount = 0;
        NHTTPi_memclr(sendBuffer, NHTTP_SEND_BUFFER_SIZE);
        if (sent < 0) {
            result = 1;
        }
        if (sent == 0) {
            result = 2;
        }
    }
    work->sendCount = 0;
    NHTTPi_memclr(sendBuffer, NHTTP_SEND_BUFFER_SIZE);
    return result;
}

// Reads the header of the answer a byte at a time, up to its end, into the response (its first part and then blocks)
static BOOL NHTTPi_ThreadRecvHeaderProc(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    NHttpResponse *response = request->response;
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    char tail[4] = { 0, 0, 0, 0 };
    NHttpHeaderBlock *block;

    if (connection != NULL) {
        connection->stage = NHTTP_STAGE_RECV_HEADER;
    }
    response->headerSize = 0;
    NHTTPi_memclr(work->statusLine, sizeof(work->statusLine));
    block = response->blocks;
    work->recvCount = 0;
    for (;;) {
        int count;

        if (request->canceled != 0) {
            return FALSE;
        }
        if (work->recvCount < NHTTP_HEADER_SIZE) {
            count = NHTTPi_SocRecv(mutexInfo, request, state->socket, response->header + work->recvCount, 1, 0);
            tail[work->recvCount & 3] = response->header[work->recvCount];
        } else {
            int offset = work->recvCount & (NHTTP_HEADER_BLOCK_SIZE - 1);

            if (offset == 0) {
                if (block != NULL) {
                    block->next = NHTTPi_alloc(sizeof(NHttpHeaderBlock), 4);
                    block = block->next;
                } else {
                    block = NHTTPi_alloc(sizeof(NHttpHeaderBlock), 4);
                    response->blocks = block;
                }
                if (block == NULL) {
                    work->error = 1;
                    return FALSE;
                }
                block->next = NULL;
            }
            count = NHTTPi_SocRecv(mutexInfo, request, state->socket, block->data + offset, 1, 0);
            tail[work->recvCount & 3] = block->data[offset];
        }
        if (count <= 0) {
            work->error = 10;
            return FALSE;
        }
        work->recvCount += count;
        if (NHTTPi_CheckHeaderEnd(tail, work->recvCount)) {
            break;
        }
    }
    response->headerSize = work->recvCount;
    if (response->headerSize == 0) {
        work->error = 7;
        return FALSE;
    }
    return TRUE;
}

// Reads the status line and the header fields that matter: the status, how long the body is, whether the connection
// stays open and whether the body is in chunks
static BOOL NHTTPi_ThreadParseHeaderProc(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    NHttpResponse *response = request->response;
    u8 *sendBuffer = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    int colon;
    int start;
    int length;

    if (!NHTTPi_loadFromHdrRecvBuf(response, work->statusLine, 0, sizeof(work->statusLine))) {
        work->error = 7;
        return FALSE;
    }
    if (NHTTPi_strnicmp(work->statusLine, "HTTP/", 5) != 0) {
        work->error = 7;
        return FALSE;
    }
    if (work->statusLine[8] != ' ') {
        work->error = 7;
        return FALSE;
    }
    response->statusCode = NHTTPi_strToInt(work->statusLine + 9, 3);
    if (response->statusCode < 0) {
        work->error = 7;
        return FALSE;
    }
    if (NHTTPi_findNextLineHdrRecvBuf(response, NHTTP_FIRST_FIELD, response->headerSize, &colon, NULL) < 0) {
        work->error = 7;
        return FALSE;
    }
    length = NHTTPi_getHeaderValue(response, "Content-Length", &start);
    work->contentLength = length;
    if (length == 0) {
        work->error = 0;
        return FALSE;
    }
    if (length > NHTTP_SEND_BUFFER_SIZE) {
        work->error = 7;
        return FALSE;
    }
    if (length > 0) {
        if (!NHTTPi_loadFromHdrRecvBuf(response, (char *)sendBuffer, start, length)) {
            work->error = 7;
            return FALSE;
        }
        work->contentLength = NHTTPi_strToInt((char *)sendBuffer, work->contentLength);
        if (work->contentLength < 0) {
            work->error = 7;
            return FALSE;
        }
        response->contentLength = work->contentLength;
    } else {
        response->contentLength = -1;
    }
    if (request->https != 0) {
        work->keepAlive = FALSE;
    } else {
        length = NHTTPi_getHeaderValue(response, "Connection", &start);
        if (length == 0) {
            work->error = 7;
            work->keepAlive = FALSE;
            return FALSE;
        }
        if (length > NHTTP_SEND_BUFFER_SIZE) {
            work->keepAlive = FALSE;
        } else if (length > 0) {
            if (NHTTPi_compareTokenN_HdrRecvBuf(response, start, start + length, "Keep-Alive", 0) == 0) {
                work->keepAlive = TRUE;
            } else {
                work->keepAlive = FALSE;
            }
        } else {
            work->keepAlive = FALSE;
        }
    }
    length = NHTTPi_getHeaderValue(response, "Transfer-Encoding", &start);
    work->chunked = length;
    if (length == 0) {
        work->error = 7;
        return FALSE;
    }
    if (length > NHTTP_SEND_BUFFER_SIZE) {
        work->chunked = FALSE;
    } else {
        work->chunked =
            length > 0 ? NHTTPi_compareTokenN_HdrRecvBuf(response, start, start + length, "chunked", ';') == 0 : FALSE;
    }
    work->error = 0;
    response->headerDone = TRUE;
    return TRUE;
}

// Reads the body of the answer into the user's buffer: as many bytes as the Content-Length says, chunk by chunk when
// it is chunked, or until the server closes the connection. When the buffer is full the connection's callback is asked
// for another, and a scratch buffer takes what is read if it gives none (the error then says the buffer was too small)
static BOOL NHTTPi_ThreadRecvBodyProc(NHttpThreadWork *work) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpRequest *request = NHTTPi_GetReqInfoP(root)->current->request;
    NHttpResponse *response = request->response;
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);
    u8 *scratch = NHTTPi_GetThreadInfoP(root)->sendBuffer;
    int count;

    if (request->method == NHTTP_METHOD_HEAD || response->statusCode == 204 || response->statusCode == 304 ||
        (response->statusCode >= 100 && response->statusCode < 200)) {
        return TRUE;
    }
    NHTTPi_SetVirtualContentLength(connection, 0);
    if (connection != NULL) {
        connection->stage = NHTTP_STAGE_RECV_BODY;
    }
    if (work->contentLength >= 0) {
        NHTTPi_SetVirtualContentLength(connection, work->contentLength);
        if (work->contentLength > 0) {
            do {
                if (work->error != 6 && !NHTTPi_BufFull(mutexInfo, response)) {
                    work->error = 6;
                    response->buffer = work->unk_104;
                    response->bufferSize = sizeof(work->unk_104);
                }
                if (work->error == 6) {
                    count = NHTTPi_RecvBufN(mutexInfo, request, state->socket, 0, work->contentLength, 0);
                } else {
                    count = NHTTPi_RecvBufN(mutexInfo, request, state->socket, response->bufferUsed,
                                            work->contentLength, 0);
                }
                if (count < 0) {
                    return FALSE;
                }
                if (count == 0) {
                    break;
                }
                if (work->error != 6) {
                    response->bufferUsed += count;
                    response->bodyReceived += count;
                }
                work->contentLength -= count;
            } while (work->contentLength > 0);
        }
        if (work->error != 6) {
            if (work->contentLength != 0) {
                work->error = NHTTPi_isRecvBufFull(response, response->bufferUsed) ? 6 : 10;
            } else {
                work->error = 0;
            }
        }
    } else {
        int chunkSize;

        work->error = 10;
        chunkSize = 10;
        if (work->chunked) {
            chunkSize = -1;
            for (;;) {
                char ring[2] = { 0, 0 };

                work->recvCount = 0;
                while (work->recvCount < NHTTP_SEND_BUFFER_SIZE) {
                    int length;
                    char c;

                    count = NHTTPi_SocRecv(mutexInfo, request, state->socket, scratch + work->recvCount, 1, 0);
                    if (count < 0) {
                        return FALSE;
                    }
                    length = work->recvCount;
                    c = ring[length & 1] = scratch[length];
                    if (c == ';' || (c == '\n' && ring[(length - 1) & 1] == '\r')) {
                        if (c == '\n') {
                            length--;
                        } else if (NHTTPi_SkipLine(mutexInfo, request, state->socket) <= 0) {
                            return FALSE;
                        }
                        if (length == 0) {
                            return FALSE;
                        }
                        chunkSize = NHTTPi_strToHex((char *)scratch, length);
                        if (chunkSize < 0) {
                            return FALSE;
                        }
                        break;
                    }
                    work->recvCount++;
                }
                if (work->recvCount == NHTTP_SEND_BUFFER_SIZE) {
                    work->error = 7;
                    return FALSE;
                }
                if (chunkSize > 0) {
                    NHTTPi_SetVirtualContentLength(connection, chunkSize);
                    while (chunkSize > 0) {
                        if (work->error != 6 && !NHTTPi_BufFull(mutexInfo, response)) {
                            work->error = 6;
                            response->buffer = work->unk_104;
                            response->bufferSize = sizeof(work->unk_104);
                        }
                        if (work->error == 6) {
                            count = NHTTPi_RecvBufN(mutexInfo, request, state->socket, 0, chunkSize, 0);
                        } else {
                            count =
                                NHTTPi_RecvBufN(mutexInfo, request, state->socket, response->bufferUsed, chunkSize, 0);
                        }
                        if (count <= 0) {
                            return FALSE;
                        }
                        response->bufferUsed += count;
                        response->bodyReceived += count;
                        chunkSize -= count;
                        if (chunkSize == 0) {
                            if (NHTTPi_SocRecv(mutexInfo, request, state->socket, scratch, 2, 0) <= 0) {
                                return FALSE;
                            }
                        }
                    }
                } else {
                    NHTTPi_SkipLine(mutexInfo, request, state->socket);
                    work->error = 0;
                    break;
                }
            }
        } else {
            for (;;) {
                if (!NHTTPi_BufFull(mutexInfo, response)) {
                    work->error = 6;
                    response->buffer = work->unk_104;
                    response->bufferSize = sizeof(work->unk_104);
                }
                if (work->error == 6) {
                    count = NHTTPi_RecvBuf(mutexInfo, request, state->socket, 0, 0);
                } else {
                    count = NHTTPi_RecvBuf(mutexInfo, request, state->socket, response->bufferUsed, 0);
                }
                if (count < 0) {
                    return FALSE;
                }
                if (count == 0) {
                    if (work->error != 6) {
                        work->error = 0;
                    }
                    break;
                }
                response->bufferUsed += count;
                response->bodyReceived += count;
            }
        }
    }
    {
        NHttpConnection *owner = NHTTPi_Response2Connection(mutexInfo, response);

        if (work->error == 0 && owner != NULL) {
            NHTTPi_ReceivedCallback(mutexInfo, owner);
        }
    }
    return TRUE;
}

// The thread: takes each request that is sent and runs it, and sleeps when there is none, until the library ends
void NHTTPi_CommThreadProcMain(void) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpReqInfo *reqInfo = NHTTPi_GetReqInfoP(root);
    NHttpThreadWork work;

    NHTTPi_InitThreadWork(&work);
    if (state->quit == 0) {
        do {
            if (work.retry == 0) {
                if (!NHTTPi_ThreadExecReqQueue(&work)) {
                    continue;
                }
                if (reqInfo->current->request->canceled != 0) {
                    NHTTPi_ThreadReqEnd(&work);
                    continue;
                }
                if (!NHTTPi_ThreadHostAddrProc(&work)) {
                    NHTTPi_ThreadReqEnd(&work);
                    continue;
                }
            }
            if (work.retry == 1) {
                work.retry = 0;
            }
            if (!NHTTPi_ThreadConnectProc(&work)) {
                NHTTPi_ThreadReqEnd(&work);
                continue;
            }
            switch (NHTTPi_ThreadProxyProc(&work)) {
            case 0:
                break;
            case 2:
                work.retry = 1;
                continue;
            case 1:
                NHTTPi_ThreadReqEnd(&work);
                continue;
            }
            switch (NHTTPi_ThreadSendProc(&work)) {
            case 0:
                break;
            case 2:
                work.retry = 1;
                continue;
            case 1:
            case 3:
                NHTTPi_ThreadReqEnd(&work);
                continue;
            }
            if (reqInfo->current->request->canceled != 0) {
                NHTTPi_ThreadReqEnd(&work);
                continue;
            }
            if (!NHTTPi_ThreadRecvHeaderProc(&work)) {
                NHTTPi_ThreadReqEnd(&work);
                continue;
            }
            if (!NHTTPi_ThreadParseHeaderProc(&work)) {
                NHTTPi_ThreadReqEnd(&work);
                continue;
            }
            NHTTPi_ThreadRecvBodyProc(&work);
            NHTTPi_ThreadReqEnd(&work);
        } while (state->quit == 0);
    }
}
