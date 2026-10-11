#include "ghttp_internal.h"

// ghttpProcess: what each state of a connection does, from reading its URL to the end of the file it receives

// Whether a character is white space, as the C library's isspace is done: from the classification of the characters
// below 0x80
static inline int ghiIsSpace(int c) {
    return (c < 0 || c >= 0x80) ? 0 : (__ctype_map[c] & 0x100);
}

// The same, for a character from a text
#define GHI_IS_SPACE(c) ((u32)(c) < 0x80 ? (__ctype_map[c] & 0x100) : 0)

// Reads the URL of a connection: whether it is secure, the address, the port and the path (with spaces made +).
// Returns FALSE if it isn't a URL
BOOL ghiParseURL(GHIConnection *connection) {
    char *url;
    u32 length;
    char saved;
    char *path;
    char *space;

    if (connection == NULL) {
        return FALSE;
    }
    url = connection->url;
    if (url == NULL) {
        return FALSE;
    }
    if (strncmp(url, "http://", 7) == 0) {
        connection->secure = FALSE;
        url += 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        connection->secure = TRUE;
        url += 8;
    } else {
        return FALSE;
    }

    length = strcspn(url, ":/");
    saved = url[length];
    url[length] = '\0';
    connection->serverAddress = func_ov011_0216e2c8(url);
    if (connection->serverAddress == NULL) {
        return FALSE;
    }
    url[length] = saved;
    url += length;

    if (url[0] == ':') {
        url++;
        connection->serverPort = atol(url);
        if (connection->serverPort == 0) {
            return FALSE;
        }
        do {
            url++;
        } while (url[0] != '\0' && url[0] != '/');
    } else if (connection->secure == TRUE) {
        connection->serverPort = 443;
    } else {
        connection->serverPort = 80;
    }

    if (url[0] == '\0') {
        url = "/";
    }
    path = func_ov011_0216e2c8(url);
    connection->requestPath = path;
    space = strchr(path, ' ');
    while (space != NULL) {
        *space = '+';
        path = connection->requestPath;
        space = strchr(path, ' ');
    }
    {
        BOOL valid = FALSE;

        if (path != NULL) {
            valid = TRUE;
        }
        return valid;
    }
}

// Starts a connection: reads its URL and, for a secure one, makes sure that there is an encryption engine
void ghiDoSocketInit(GHIConnection *connection) {
    func_ov011_0216cca8(8, 3, 0x10, "Socket Initialization\n");
    ghiCallProgressCallback(connection, NULL, 0);
    func_ov011_0216e31c();
    if (!ghiParseURL(connection)) {
        connection->completed = TRUE;
        connection->result = 3;
        return;
    }

    if (connection->secure == TRUE && connection->encryptor.engine == 0) {
        func_ov011_0216cca8(8, 0, 2, "Encryption engine not set for HTTPS.  Using default engine\r\n");
        ghttpSetRequestEncryptionEngine(connection->request, 5);
    } else if (connection->secure != TRUE && connection->encryptor.engine != 0) {
        ghttpSetRequestEncryptionEngine(connection->request, 0);
        func_ov011_0216cca8(8, 0, 2, "Encryption engine set for unsecured URL. Removing encryption.\r\n");
    }

    if (connection->secure == TRUE && connection->encryptor.open == 0) {
        func_ov011_0216cca8(8, 3, 15, "Initializing SSL engine\n");
        if (connection->encryptor.init(connection, &connection->encryptor) == 3) {
            func_ov011_0216cca8(8, 3, 2, "Failed to initialize SSL engine\n");
            connection->completed = TRUE;
            connection->result = 0x11;
            return;
        }
    }
    connection->state = 1;
    ghiCallProgressCallback(connection, NULL, 0);
}

// Looks the server's address up: at once if it is a number, or else with a lookup that goes on
void ghiDoHostLookup(GHIConnection *connection) {
    char *host;

    if (connection->hostLookup != NULL) {
        connection->state = 2;
        ghiCallProgressCallback(connection, NULL, 0);
        return;
    }
    func_ov011_0216cca8(8, 3, 0x10, "Host Lookup\n");
    host = connection->proxyAddress;
    if (host == NULL) {
        host = ghiProxyAddress;
        if (host == NULL) {
            host = connection->serverAddress;
        }
    }
    connection->serverIp = func_ov011_0216df90(host);
    if (connection->serverIp == -1) {
        if (func_ov011_0216e26c(host, &connection->hostLookup) == -1) {
            func_ov011_0216cca8(8, 3, 1, "Thread Creation Failed\n");
            connection->hostLookup = NULL;
            connection->completed = TRUE;
            connection->result = 4;
            return;
        }
    }
    if (connection->serverIp == -1) {
        connection->state = 2;
        ghiCallProgressCallback(connection, NULL, 0);
    } else {
        connection->state = 3;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Waits for the lookup to end
void ghiDoLookupPending(GHIConnection *connection) {
    connection->serverIp = func_ov011_0216e2bc(connection->hostLookup);
    if (connection->serverIp == -1) {
        func_ov011_0216cca8(8, 3, 1, "Error resolving hostname\n");
        connection->hostLookup = NULL;
        connection->completed = TRUE;
        connection->result = 4;
        return;
    }
    if (connection->serverIp == 0) {
        connection->state = 2;
        ghiCallProgressCallback(connection, NULL, 0);
        return;
    }
    connection->hostLookup = NULL;
    func_ov011_0216cca8(8, 3, 0x10, "DNS lookup complete\n");
    connection->state = 3;
    ghiCallProgressCallback(connection, NULL, 0);
}

// Makes the socket, connects it, and waits for the connection to be made
void ghiDoConnecting(GHIConnection *connection) {
    int result;

    func_ov011_0216cca8(8, 3, 0x10, "Connecting\n");
    if (connection->socket == -1) {
        u16 port;

        connection->socket = func_ov011_0216de20(2, 1, 0);
        if (connection->socket == -1) {
            connection->completed = TRUE;
            connection->result = 5;
            connection->socketError = func_ov011_0216dfa8(connection->socket);
            return;
        }
        if (!func_ov011_0216e054(connection->socket, 0)) {
            connection->completed = TRUE;
            connection->result = 5;
            connection->socketError = func_ov011_0216dfa8(connection->socket);
            return;
        }
        if (connection->throttle != 0) {
            func_ov011_0216e0d0(connection->socket, ghiThrottleBytes);
        }
        if (connection->encryptor.engine == 4) {
            if (connection->encryptor.start(connection, &connection->encryptor) == 3) {
                connection->completed = TRUE;
                connection->result = 0x11;
                return;
            }
            connection->encryptor.engine = 0;
        }

        {
            SocAddrIn address = {0};

            address.family = 2;
            if (connection->proxyAddress != NULL) {
                port = connection->proxyPort;
            } else if (ghiProxyAddress != NULL) {
                port = ghiProxyPort;
            } else {
                port = connection->serverPort;
            }
            address.port = GHI_SWAP16(port);
            address.addr = connection->serverIp;
            result = func_ov011_0216dec4(connection->socket, &address, 8);
        }
        if (result == -1) {
            int error = func_ov011_0216dfa8(connection->socket);

            if (error != -6 && error != -26 && error != -76) {
                connection->completed = TRUE;
                connection->result = 6;
                connection->socketError = error;
                return;
            }
        }
    }

    {
        int ready;
        int failed;

        result = func_ov011_0216dfb4(connection->socket, NULL, &ready, &failed);
        if (result == -1 || (result == 1 && failed != 0)) {
            connection->completed = TRUE;
            connection->result = 6;
            if (result == -1) {
                connection->socketError = func_ov011_0216dfa8(connection->socket);
            } else {
                connection->socketError = 0;
            }
            return;
        }
        if (result == 1 && ready != 0) {
            if (connection->encryptor.engine == 0) {
                connection->state = 5;
            } else {
                connection->state = 4;
            }
            ghiCallProgressCallback(connection, NULL, 0);
        }
    }
}

// Ends the secure session's setup: the engine is started, and once it is active the request is sent
void ghiDoSecuringSession(GHIConnection *connection) {
    int size;
    char buffer[0x401];
    int result;

    if (connection->encryptor.unk_0C == 0) {
        func_ov011_0216cca8(8, 3, 0x10, "Securing Session\n");
        if (connection->encryptor.start != NULL) {
            if (connection->encryptor.start(connection, &connection->encryptor) == 3) {
                connection->completed = TRUE;
                connection->result = 0x11;
                return;
            }
        }
        if (connection->encryptor.active != 0) {
            connection->state = 5;
            ghiCallProgressCallback(connection, NULL, 0);
            return;
        }
    }

    if (connection->encryptor.unk_1C != 0) {
        if (connection->encryptor.start != NULL) {
            if (connection->encryptor.start(connection, &connection->encryptor) == 3) {
                connection->completed = TRUE;
                connection->result = 0x11;
                return;
            }
        }
        if (connection->encryptor.active != 0) {
            connection->state = 5;
            ghiCallProgressCallback(connection, NULL, 0);
        }
        return;
    }

    if (connection->sendBuffer.pos < connection->sendBuffer.length) {
        if (!ghiSendBufferedData(connection)) {
            return;
        }
        if (connection->sendBuffer.pos < connection->sendBuffer.length) {
            return;
        }
        ghiResetBuffer(&connection->sendBuffer);
    }

    size = sizeof(buffer);
    result = ghiDoReceive(connection, buffer, &size);
    if (result == 2 || result == 3) {
        connection->completed = TRUE;
        connection->result = 0x11;
        return;
    }
    if (result != 0) {
        return;
    }
    if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
        return;
    }
    if (!ghiDecryptReceivedData(connection)) {
        connection->completed = TRUE;
        connection->result = 0x11;
        return;
    }
    if (connection->encryptor.active != 0) {
        connection->state = 5;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Builds the request, a line and its headers, and sends it
void ghiDoSendingRequest(GHIConnection *connection) {
    GHIBuffer *buffer;
    const char *method;
    char length[16];

    func_ov011_0216cca8(8, 3, 0x10, "Sending Request\n");
    if (connection->sendBuffer.length == 0) {
        if (connection->encryptor.engine == 0 || connection->encryptor.mode == 0) {
            buffer = &connection->sendBuffer;
        } else {
            buffer = &connection->encodeBuffer;
        }

        if (connection->post != NULL && !connection->postingState.finished) {
            method = "POST ";
        } else {
            method = "HEAD ";
            if (connection->type != 3) {
                method = "GET ";
            }
        }
        ghiAppendDataToBuffer(buffer, method, 0);
        if (connection->proxyAddress != NULL || ghiProxyAddress != NULL) {
            ghiAppendDataToBuffer(buffer, connection->url, 0);
        } else {
            ghiAppendDataToBuffer(buffer, connection->requestPath, 0);
        }
        ghiAppendDataToBuffer(buffer, " HTTP/1.1\r\n", 0);

        if (connection->serverPort == 80) {
            ghiAppendHeaderToBuffer(buffer, "Host", connection->serverAddress);
        } else {
            ghiAppendDataToBuffer(buffer, "Host: ", 0);
            ghiAppendDataToBuffer(buffer, connection->serverAddress, 0);
            ghiAppendCharToBuffer(buffer, ':');
            ghiAppendIntToBuffer(buffer, connection->serverPort);
            ghiAppendDataToBuffer(buffer, "\r\n", 2);
        }

        if (connection->headers == NULL || strstr(connection->headers, "User-Agent") == NULL) {
            ghiAppendHeaderToBuffer(buffer, "User-Agent", "GameSpyHTTP/1.0");
        }
        if (connection->keepAlive) {
            ghiAppendHeaderToBuffer(buffer, "Connection", "Keep-Alive");
        } else {
            ghiAppendHeaderToBuffer(buffer, "Connection", "close");
        }

        if (connection->post != NULL && !connection->postingState.finished) {
            func_0207a290(length, "%d", connection->postingState.totalBytes);
            ghiAppendHeaderToBuffer(buffer, "Content-Length", length);
            ghiAppendHeaderToBuffer(buffer, "Content-Type", ghiPostGetContentType(connection));
        }
        if (connection->headers != NULL) {
            ghiAppendDataToBuffer(buffer, connection->headers, 0);
        }
        ghiAppendDataToBuffer(buffer, "\r\n", 2);

        if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
            if (!ghiEncryptDataToBuffer(&connection->sendBuffer, buffer->data, buffer->length)) {
                connection->completed = TRUE;
                connection->result = 0x11;
                return;
            }
            ghiResetBuffer(buffer);
        }
    }

    if (ghiSendBufferedData(connection) && connection->sendBuffer.pos >= connection->sendBuffer.length) {
        ghiResetBuffer(&connection->sendBuffer);
        if (connection->post != NULL && !connection->postingState.finished) {
            connection->state = 6;
        } else {
            connection->state = 7;
        }
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Sends the post of the request
void ghiDoPosting(GHIConnection *connection) {
    int bytesPosted;
    int result;
    int readable;

    func_ov011_0216cca8(8, 3, 0x10, "Posting\n");
    bytesPosted = connection->postingState.bytesPosted;
    result = ghiPostDoPosting(connection);
    if (result == 0) {
        readable = 0;
        ghiPostCleanupState(connection);
        if (func_ov011_0216dfb4(connection->socket, &readable, NULL, NULL) == 1 && readable != 0) {
            connection->state = 8;
            ghiCallProgressCallback(connection, NULL, 0);
        }
        return;
    }
    if (result == 3) {
        connection->postingState.isXml = FALSE;
        return;
    }
    if (bytesPosted != connection->postingState.bytesPosted) {
        ghiCallPostCallback(connection);
    }
    if (result == 1) {
        ghiPostCleanupState(connection);
        connection->postingState.finished = TRUE;
        connection->state = 7;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Waits for the server to answer
void ghiDoWaiting(GHIConnection *connection) {
    int readable;
    int failed;
    int result;

    func_ov011_0216cca8(8, 3, 0x10, "Waiting\n");
    result = func_ov011_0216dfb4(connection->socket, &readable, NULL, &failed);
    if (result == -1 || (result == 1 && failed != 0)) {
        connection->completed = TRUE;
        connection->result = 5;
        if (result == -1) {
            connection->socketError = func_ov011_0216dfa8(connection->socket);
        } else {
            connection->socketError = 0;
        }
        return;
    }
    if (result == 1 && readable != 0) {
        connection->state = 8;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Reads the status line of the answer: the version, the code and where its text ends
static BOOL ghiParseStatus(GHIConnection *connection) {
    int major;
    int minor;
    int code;
    int textEnd;
    char c;

    if (func_0207fa4c(connection->recvBuffer.data, "HTTP/%d.%d %d%n", &major, &minor, &code, &textEnd) != 3 ||
        major < 1 || code < 100 || code >= 600) {
        connection->completed = TRUE;
        connection->result = 7;
        return FALSE;
    }

    for (; (c = connection->recvBuffer.data[textEnd]) != '\0' && ghiIsSpace(c); textEnd++) {
    }
    connection->httpMajor = major;
    connection->httpMinor = minor;
    connection->statusCode = code;
    connection->statusTextPos = textEnd;
    return TRUE;
}

// Reads what the socket has until the status line is whole, and then moves on to the headers
void ghiDoReceivingStatus(GHIConnection *connection) {
    int size;
    char buffer[0x400];
    int result;
    char *lineEnd;
    int lineLength;

    func_ov011_0216cca8(8, 3, 0x10, "Receiving Status\n");
    size = sizeof(buffer);
    result = ghiDoReceive(connection, buffer, &size);
    if (result == 3 || result == 1) {
        return;
    }
    if (result == 0) {
        if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
            if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
                return;
            }
            if (!ghiDecryptReceivedData(connection)) {
                connection->completed = TRUE;
                connection->result = 0x11;
                return;
            }
        } else {
            if (!ghiAppendDataToBuffer(&connection->recvBuffer, buffer, size)) {
                return;
            }
        }
    }

    lineEnd = strstr(connection->recvBuffer.data, "\r\n");
    if (lineEnd != NULL) {
        lineEnd[0] = '\0';
        lineLength = lineEnd - connection->recvBuffer.data;
        if (!ghiParseStatus(connection)) {
            return;
        }
        connection->headersPos = lineLength + 2;
        if (connection->statusCode == 100 && connection->postingState.isXml != 0) {
            connection->postingState.isXml = FALSE;
            ghiResetBuffer(&connection->recvBuffer);
            connection->state = 6;
            ghiCallProgressCallback(connection, NULL, 0);
            func_ov011_0216cca8(8, 0, 0x10, "Got HTTP continue\r\n");
            return;
        }
        connection->state = 9;
        ghiCallProgressCallback(connection, NULL, 0);
    } else if (result == 2) {
        connection->completed = TRUE;
        connection->result = 7;
        connection->socketError = func_ov011_0216dfa8(connection->socket);
    }
}

// Hands what was received of the file to the game, and ends the request if all of it came
static BOOL ghiDeliverIncomingFileData(GHIConnection *connection, const char *data, int size) {
    const char *delivered = NULL;
    int deliveredSize = 0;

    connection->bytesReceived += size;
    if (connection->bytesReceived == connection->totalSize || connection->closing != 0) {
        connection->completed = TRUE;
    }
    if (connection->type == 0) {
        if (!ghiAppendDataToBuffer(&connection->fileBuffer, data, size)) {
            return FALSE;
        }
        delivered = connection->fileBuffer.data;
        deliveredSize = connection->fileBuffer.length;
    } else if (connection->type == 1) {
        if (size != 0) {
            connection->completed = TRUE;
            connection->result = 13;
            return FALSE;
        }
        delivered = data;
        deliveredSize = size;
    } else if (connection->type == 2) {
        delivered = data;
        deliveredSize = size;
    }
    ghiCallProgressCallback(connection, delivered, deliveredSize);
    return TRUE;
}

// The size of the chunk that a chunk header tells, or -1
static int ghiGetChunkSize(GHIConnection *connection) {
    int size;

    if (func_0207fa4c(connection->chunkHeader, "%x", &size) != 1) {
        return -1;
    }
    return size;
}

// Adds to the header of the chunk that is being read
static void ghiAppendChunkHeader(GHIConnection *connection, const char *data, int size) {
    int count;

    if (size != 0 && connection->chunkHeaderLength < 10) {
        count = 10 - connection->chunkHeaderLength;
        if (count >= size) {
            count = size;
        }
        memcpy(connection->chunkHeader + connection->chunkHeaderLength, data, count);
        connection->chunkHeaderLength += count;
        connection->chunkHeader[connection->chunkHeaderLength] = '\0';
    }
}

// Sends the file's bytes that came to the game, taking them out of their chunks if the file comes in chunks. Returns
// FALSE if the request failed
BOOL ghiProcessIncomingFileData(GHIConnection *connection, const char *data, int size) {
    if (connection->chunked) {
        while (size > 0) {
            if (connection->chunkState == 0) {
                const char *lineEnd = strchr(data, '\n');
                int chunkSize;

                if (lineEnd != NULL) {
                    ghiAppendChunkHeader(connection, data, lineEnd - data);
                    size -= lineEnd + 1 - data;
                    data = lineEnd + 1;
                    chunkSize = ghiGetChunkSize(connection);
                    connection->chunkRemaining = chunkSize;
                    if (chunkSize == -1) {
                        connection->completed = TRUE;
                        connection->result = 7;
                        return FALSE;
                    }
                    if (chunkSize == 0) {
                        connection->chunkState = 3;
                        func_ov011_0216cca8(8, 0, 0x20, "Reading footer\n");
                    } else {
                        connection->chunkState = 1;
                        func_ov011_0216cca8(8, 0, 0x20, "Reading %d byte chunk\n", chunkSize);
                    }
                } else {
                    ghiAppendChunkHeader(connection, data, size);
                    return TRUE;
                }
            } else if (connection->chunkState == 1) {
                int count = connection->chunkRemaining;

                if (count >= size) {
                    count = size;
                }
                func_ov011_0216cca8(8, 0, 0x20, "Read %d bytes of chunk\n", count);
                if (!ghiDeliverIncomingFileData(connection, data, count)) {
                    return FALSE;
                }
                data += count;
                size -= count;
                connection->chunkRemaining -= count;
                if (connection->chunkRemaining == 0) {
                    connection->chunkState = 2;
                }
            } else if (connection->chunkState == 2) {
                const char *lineEnd = strchr(data, '\n');

                if (lineEnd == NULL) {
                    return TRUE;
                }
                size -= lineEnd + 1 - data;
                data = lineEnd + 1;
                connection->chunkHeader[0] = '\0';
                connection->chunkHeaderLength = 0;
                connection->chunkRemaining = 0;
                connection->chunkState = 0;
                func_ov011_0216cca8(8, 0, 0x20, "Read chunk footer\n");
            } else if (connection->chunkState == 3) {
                connection->completed = TRUE;
                func_ov011_0216cca8(8, 0, 0x20, "Finished reading chunks\n");
                return TRUE;
            } else {
                return FALSE;
            }
        }
        return TRUE;
    }
    return ghiDeliverIncomingFileData(connection, data, size);
}

static void ghiFreeIfSet(void *pointer) {
    if (pointer != NULL) {
        func_ov011_0216ddfc(pointer);
    }
}

// Reads what the socket has until the headers are whole, and makes of them what is needed: the redirect, the length and
// the chunks
void ghiDoReceivingHeaders(GHIConnection *connection) {
    int size;
    int headersPos;
    int result;
    char *buffer;
    char *body;
    int bodyLength;
    char *contentLength;
    char *data;
    char *headersEnd;
    BOOL skipCR = TRUE;

    buffer = func_ov011_0216dddc(0x1000);
    size = 0x1000;
    if (buffer == NULL) {
        connection->completed = TRUE;
        connection->result = 1;
        return;
    }
    result = ghiDoReceive(connection, buffer, &size);
    if (result == 3 || result == 1) {
        ghiFreeIfSet(buffer);
        return;
    }
    if (result == 0) {
        if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
            if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
                ghiFreeIfSet(buffer);
                return;
            }
            if (!ghiDecryptReceivedData(connection)) {
                connection->completed = TRUE;
                connection->result = 0x11;
                ghiFreeIfSet(buffer);
                return;
            }
        } else {
            if (!ghiAppendDataToBuffer(&connection->recvBuffer, buffer, size)) {
                ghiFreeIfSet(buffer);
                return;
            }
        }
    }

    data = connection->recvBuffer.data;
    headersPos = connection->headersPos;
    if (connection->statusCode / 100 == 1 &&
        (strncmp(data + headersPos, "\r\n", 2) == 0 || strncmp(data + headersPos, "\n\n", 2) == 0)) {
        skipCR = FALSE;
        headersEnd = data + headersPos;
    } else {
        headersEnd = strstr(data + headersPos, "\r\n\r\n");
    }
    if (headersEnd == NULL) {
        headersEnd = strstr(data + headersPos, "\n\n");
    }
    if (headersEnd == NULL) {
        if (result == 2) {
            connection->completed = TRUE;
            connection->result = 7;
            connection->socketError = func_ov011_0216dfa8(connection->socket);
        }
        ghiFreeIfSet(buffer);
        return;
    }

    if (skipCR == TRUE) {
        headersEnd += 2;
    }
    headersEnd[0] = '\0';
    body = headersEnd + 2;
    bodyLength = connection->recvBuffer.length - (body - connection->recvBuffer.data);
    connection->recvBuffer.length = headersEnd - connection->recvBuffer.data + 1;
    connection->recvBuffer.pos = connection->recvBuffer.length;

    if (connection->statusCode / 100 == 1) {
        if (bodyLength != 0) {
            memmove(connection->recvBuffer.data, body, bodyLength + 1);
            connection->recvBuffer.length = bodyLength;
        } else {
            ghiResetBuffer(&connection->recvBuffer);
        }
        if (connection->postingState.isXml != 0) {
            connection->postingState.isXml = FALSE;
            connection->state = 6;
            ghiCallProgressCallback(connection, NULL, 0);
        }
        connection->state = 8;
        ghiCallProgressCallback(connection, NULL, 0);
        ghiFreeIfSet(buffer);
        return;
    }

    if (connection->statusCode / 100 == 3) {
        char *location;

        if (connection->redirectCount > 10) {
            connection->completed = TRUE;
            connection->result = 11;
            ghiFreeIfSet(buffer);
            return;
        }
        location = strstr(data + headersPos, "Location:");
        if (location != NULL) {
            char *end;

            location += 9;
            while (GHI_IS_SPACE(location[0])) {
                location++;
            }
            end = location;
            while (end[0] != '\0' && !GHI_IS_SPACE(end[0])) {
                end++;
            }
            end[0] = '\0';
            if (location[0] == '/') {
                connection->redirectUrl =
                    func_ov011_0216dddc(strlen(connection->serverAddress) + strlen(location) + 14);
                if (connection->redirectUrl == NULL) {
                    connection->completed = TRUE;
                    connection->result = 1;
                }
                func_0207a290(connection->redirectUrl, "http://%s:%d%s", connection->serverAddress,
                              connection->serverPort, location);
            } else {
                connection->redirectUrl = func_ov011_0216e2c8(location);
                if (connection->redirectUrl == NULL) {
                    connection->completed = TRUE;
                    connection->result = 1;
                }
            }
            ghiFreeIfSet(buffer);
            return;
        }
    }

    contentLength = strstr(data + headersPos, "Content-Length:");
    if (contentLength != NULL) {
        char maxValue[11] = "2147483647";
        char *digits = contentLength + 16;
        char *digitsEnd = digits;
        int digitsLength;

        for (; digitsEnd != NULL && digitsEnd[0] != '\0' && digitsEnd[0] != '\n' && digitsEnd[0] != '\r' &&
               digitsEnd[0] != ' ';
             digitsEnd++) {
        }
        digitsLength = digitsEnd - digits;
        if (digitsLength > strlen(maxValue)) {
            connection->completed = TRUE;
            connection->result = 16;
            ghiFreeIfSet(buffer);
            return;
        }
        if (strlen(maxValue) == digitsLength && strncmp(digits, maxValue, digitsLength) >= 0) {
            connection->completed = TRUE;
            connection->result = 16;
            ghiFreeIfSet(buffer);
            return;
        }
        connection->totalSize = atol(digits);
    }

    connection->chunked = strstr(data + headersPos, "Transfer-Encoding: chunked") != NULL ? TRUE : FALSE;
    if (connection->chunked) {
        connection->chunkHeader[0] = '\0';
        connection->chunkHeaderLength = 0;
        connection->chunkRemaining = 0;
        connection->chunkState = 0;
    }
    if (connection->type == 3 || connection->type == 4) {
        connection->completed = TRUE;
        ghiFreeIfSet(buffer);
        return;
    }
    connection->state = 10;
    if (contentLength != NULL && connection->totalSize == 0) {
        connection->completed = TRUE;
        ghiFreeIfSet(buffer);
        return;
    }
    if (bodyLength > 0) {
        ghiProcessIncomingFileData(connection, body, bodyLength);
    }
    ghiFreeIfSet(buffer);
}

// Reads the file the request asks for, as long as it can for a while, and gives it to the game
void ghiDoReceivingFile(GHIConnection *connection) {
    u32 startTime = func_ov011_0216e324();
    int size = 0;
    char *buffer;
    u32 elapsed = 0;
    int result;

    buffer = func_ov011_0216dddc(0x2000);
    if (buffer == NULL) {
        connection->completed = TRUE;
        connection->result = 1;
        return;
    }
    func_ov011_0216cca8(8, 3, 0x10, "Receiving File\n");

    while (!connection->completed && elapsed < connection->maxRecvTime) {
        size = 0x2000;
        result = ghiDoReceive(connection, buffer, &size);
        if (result == 1) {
            ghiFreeIfSet(buffer);
            if (connection->lastRecvTime == 0) {
                connection->lastRecvTime = func_ov011_0216e324();
                return;
            }
            if (func_ov011_0216e324() - connection->lastRecvTime < 30000) {
                return;
            }
            func_ov011_0216cca8(8, 0, 1, "Receive File Idle Timeout %d ===== \r\n",
                                func_ov011_0216e324() - connection->lastRecvTime);
            connection->lastRecvTime = 0;
            connection->completed = TRUE;
            connection->result = 19;
            return;
        }
        connection->lastRecvTime = 0;
        if (result == 3) {
            ghiFreeIfSet(buffer);
            return;
        }
        if (result == 2) {
            connection->completed = TRUE;
            if (connection->totalSize > 0 && connection->bytesReceived < connection->totalSize) {
                connection->result = 15;
            }
            ghiFreeIfSet(buffer);
            return;
        }

        if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
            int received;

            if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
                ghiFreeIfSet(buffer);
                return;
            }
            connection->recvBuffer.length = connection->recvBuffer.pos;
            if (!ghiDecryptReceivedData(connection)) {
                connection->completed = TRUE;
                connection->result = 0x11;
                ghiFreeIfSet(buffer);
                return;
            }
            received = connection->recvBuffer.length - connection->recvBuffer.pos;
            if (received != 0) {
                if (!ghiProcessIncomingFileData(connection, connection->recvBuffer.data + connection->recvBuffer.pos,
                                                received)) {
                    ghiFreeIfSet(buffer);
                    return;
                }
            }
        } else {
            if (!ghiProcessIncomingFileData(connection, buffer, size)) {
                ghiFreeIfSet(buffer);
                return;
            }
        }
        elapsed = func_ov011_0216e324() - startTime;
    }
    ghiFreeIfSet(buffer);
}
