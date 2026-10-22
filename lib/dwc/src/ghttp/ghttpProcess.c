#include "ghttp_internal.h"
#include "nitro/std.h"

// ghttpProcess: what each state of a connection does, from reading its URL to the end of the file it receives

// Whether a character is white space, as the C library's isspace is done: from the classification of the characters
// below 0x80
static inline int ghiIsSpace(int c) {
    return (c < 0 || c >= 0x80) ? 0 : (__ctype_map[c] & 0x100);
}

// Reads the URL of a connection: whether it is secure, the address, the port and the path (with spaces made +).
// Returns gsi_false if it isn't a URL
GHTTPBool ghiParseURL(GHIConnection *connection) {
    char *url;
    gsi_u32 length;
    char saved;
    char *path;
    char *space;

    if (connection == NULL) {
        return gsi_false;
    }
    url = connection->URL;
    if (url == NULL) {
        return gsi_false;
    }
    if (strncmp(url, "http://", 7) == 0) {
        connection->protocol = gsi_false;
        url += 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        connection->protocol = gsi_true;
        url += 8;
    } else {
        return gsi_false;
    }

    length = strcspn(url, ":/");
    saved = url[length];
    url[length] = '\0';
    connection->serverAddress = goastrdup(url);
    if (connection->serverAddress == NULL) {
        return gsi_false;
    }
    url[length] = saved;
    url += length;

    if (url[0] == ':') {
        url++;
        connection->serverPort = atol(url);
        if (connection->serverPort == 0) {
            return gsi_false;
        }
        do {
            url++;
        } while (url[0] != '\0' && url[0] != '/');
    } else if (connection->protocol == gsi_true) {
        connection->serverPort = 443;
    } else {
        connection->serverPort = 80;
    }

    if (url[0] == '\0') {
        url = "/";
    }
    path = goastrdup(url);
    connection->requestPath = path;
    space = strchr(path, ' ');
    while (space != NULL) {
        *space = '+';
        path = connection->requestPath;
        space = strchr(path, ' ');
    }
    {
        gsi_bool valid = gsi_false;

        if (path != NULL) {
            valid = gsi_true;
        }
        return valid;
    }
}

// Starts a connection: reads its URL and, for a secure one, makes sure that there is an encryption engine
void ghiDoSocketInit(GHIConnection *connection) {
    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Socket Initialization\n");
    ghiCallProgressCallback(connection, NULL, 0);
    SocketStartUp();
    if (!ghiParseURL(connection)) {
        connection->completed = gsi_true;
        connection->result = 3;
        return;
    }

    if (connection->protocol == gsi_true && connection->encryptor.mEngine == 0) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "Encryption engine not set for HTTPS.  Using default engine\r\n");
        ghttpSetRequestEncryptionEngine(connection->request, 5);
    } else if (connection->protocol != gsi_true && connection->encryptor.mEngine != 0) {
        ghttpSetRequestEncryptionEngine(connection->request, 0);
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "Encryption engine set for unsecured URL. Removing encryption.\r\n");
    }

    if (connection->protocol == gsi_true && connection->encryptor.mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Debug, "Initializing SSL engine\n");
        if (connection->encryptor.mInitFunc(connection, &connection->encryptor) == 3) {
            gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_WarmError,
                          "Failed to initialize SSL engine\n");
            connection->completed = gsi_true;
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

    if (connection->handle != NULL) {
        connection->state = 2;
        ghiCallProgressCallback(connection, NULL, 0);
        return;
    }
    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Host Lookup\n");
    host = connection->proxyOverrideServer;
    if (host == NULL) {
        host = ghiProxyAddress;
        if (host == NULL) {
            host = connection->serverAddress;
        }
    }
    connection->serverIP = inet_addr(host);
    if (connection->serverIP == -1) {
        if (gsiStartResolvingHostname(host, &connection->handle) == -1) {
            gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_HotError, "Thread Creation Failed\n");
            connection->handle = NULL;
            connection->completed = gsi_true;
            connection->result = 4;
            return;
        }
    }
    if (connection->serverIP == -1) {
        connection->state = 2;
        ghiCallProgressCallback(connection, NULL, 0);
    } else {
        connection->state = 3;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Waits for the lookup to end
void ghiDoLookupPending(GHIConnection *connection) {
    connection->serverIP = gsiGetResolvedIP(connection->handle);
    if (connection->serverIP == -1) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_HotError, "Error resolving hostname\n");
        connection->handle = NULL;
        connection->completed = gsi_true;
        connection->result = 4;
        return;
    }
    if (connection->serverIP == 0) {
        connection->state = 2;
        ghiCallProgressCallback(connection, NULL, 0);
        return;
    }
    connection->handle = NULL;
    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "DNS lookup complete\n");
    connection->state = 3;
    ghiCallProgressCallback(connection, NULL, 0);
}

// Makes the socket, connects it, and waits for the connection to be made
void ghiDoConnecting(GHIConnection *connection) {
    int result;

    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Connecting\n");
    if (connection->socket == -1) {
        connection->socket = socket(2, 1, 0);
        if (connection->socket == -1) {
            connection->completed = gsi_true;
            connection->result = 5;
            connection->socketError = GOAGetLastError(connection->socket);
            return;
        }
        if (!SetSockBlocking(connection->socket, 0)) {
            connection->completed = gsi_true;
            connection->result = 5;
            connection->socketError = GOAGetLastError(connection->socket);
            return;
        }
        if (connection->throttle != 0) {
            SetReceiveBufferSize(connection->socket, ghiThrottleBufferSize);
        }
        if (connection->encryptor.mEngine == 4) {
            if (connection->encryptor.mStartFunc(connection, &connection->encryptor) == 3) {
                connection->completed = gsi_true;
                connection->result = 0x11;
                return;
            }
            connection->encryptor.mEngine = 0;
        }

        {
            SOCSockAddrIn address = { 0 };

            address.family = 2;
            if (connection->proxyOverrideServer != NULL) {
                address.port = GHI_SWAP16(connection->proxyOverridePort);
            } else if (ghiProxyAddress != NULL) {
                address.port = GHI_SWAP16(ghiProxyPort);
            } else {
                address.port = GHI_SWAP16(connection->serverPort);
            }
            address.addr.addr = connection->serverIP;
            result = connect(connection->socket, (SOCKADDR *)&address, 8);
        }
        if (result == -1) {
            int error = GOAGetLastError(connection->socket);

            if (error != -6 && error != -26 && error != -76) {
                connection->completed = gsi_true;
                connection->result = 6;
                connection->socketError = error;
                return;
            }
        }
    }

    {
        int ready;
        int failed;

        result = GSISocketSelect(connection->socket, NULL, &ready, &failed);
        if (result == -1 || (result == 1 && failed != 0)) {
            connection->completed = gsi_true;
            connection->result = 6;
            if (result == -1) {
                connection->socketError = GOAGetLastError(connection->socket);
            } else {
                connection->socketError = 0;
            }
            return;
        }
        if (result == 1 && ready != 0) {
            if (connection->encryptor.mEngine == 0) {
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

    if (connection->encryptor.mSessionStarted == 0) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Securing Session\n");
        if (connection->encryptor.mStartFunc != NULL) {
            if (connection->encryptor.mStartFunc(connection, &connection->encryptor) == 3) {
                connection->completed = gsi_true;
                connection->result = 0x11;
                return;
            }
        }
        if (connection->encryptor.mSessionEstablished != 0) {
            connection->state = 5;
            ghiCallProgressCallback(connection, NULL, 0);
            return;
        }
    }

    if (connection->encryptor.mLibSendsHandshakeMessages != 0) {
        if (connection->encryptor.mStartFunc != NULL) {
            if (connection->encryptor.mStartFunc(connection, &connection->encryptor) == 3) {
                connection->completed = gsi_true;
                connection->result = 0x11;
                return;
            }
        }
        if (connection->encryptor.mSessionEstablished != 0) {
            connection->state = 5;
            ghiCallProgressCallback(connection, NULL, 0);
        }
        return;
    }

    if (connection->sendBuffer.pos < connection->sendBuffer.len) {
        if (!ghiSendBufferedData(connection)) {
            return;
        }
        if (connection->sendBuffer.pos < connection->sendBuffer.len) {
            return;
        }
        ghiResetBuffer(&connection->sendBuffer);
    }

    size = sizeof(buffer);
    result = ghiDoReceive(connection, buffer, &size);
    if (result == 2 || result == 3) {
        connection->completed = gsi_true;
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
        connection->completed = gsi_true;
        connection->result = 0x11;
        return;
    }
    if (connection->encryptor.mSessionEstablished != 0) {
        connection->state = 5;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Builds the request, a line and its headers, and sends it
void ghiDoSendingRequest(GHIConnection *connection) {
    GHIBuffer *buffer;
    const char *method;
    char length[16];

    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Sending Request\n");
    if (connection->sendBuffer.len == 0) {
        if (connection->encryptor.mEngine == 0 || connection->encryptor.mEncryptOnBuffer == 0) {
            buffer = &connection->sendBuffer;
        } else {
            buffer = &connection->encodeBuffer;
        }

        if (connection->post != NULL && !connection->postingState.completed) {
            method = "POST ";
        } else {
            method = "HEAD ";
            if (connection->type != 3) {
                method = "GET ";
            }
        }
        ghiAppendDataToBuffer(buffer, method, 0);
        if (connection->proxyOverrideServer != NULL || ghiProxyAddress != NULL) {
            ghiAppendDataToBuffer(buffer, connection->URL, 0);
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

        if (connection->sendHeaders == NULL || strstr(connection->sendHeaders, "User-Agent") == NULL) {
            ghiAppendHeaderToBuffer(buffer, "User-Agent", "GameSpyHTTP/1.0");
        }
        if (connection->persistConnection) {
            ghiAppendHeaderToBuffer(buffer, "Connection", "Keep-Alive");
        } else {
            ghiAppendHeaderToBuffer(buffer, "Connection", "close");
        }

        if (connection->post != NULL && !connection->postingState.completed) {
            OS_SPrintf(length, "%d", connection->postingState.totalBytes);
            ghiAppendHeaderToBuffer(buffer, "Content-Length", length);
            ghiAppendHeaderToBuffer(buffer, "Content-Type", ghiPostGetContentType(connection));
        }
        if (connection->sendHeaders != NULL) {
            ghiAppendDataToBuffer(buffer, connection->sendHeaders, 0);
        }
        ghiAppendDataToBuffer(buffer, "\r\n", 2);

        if (connection->encryptor.mEngine != 0 && connection->encryptor.mEncryptOnBuffer == 1) {
            if (!ghiEncryptDataToBuffer(&connection->sendBuffer, buffer->data, buffer->len)) {
                connection->completed = gsi_true;
                connection->result = 0x11;
                return;
            }
            ghiResetBuffer(buffer);
        }
    }

    if (ghiSendBufferedData(connection) && connection->sendBuffer.pos >= connection->sendBuffer.len) {
        ghiResetBuffer(&connection->sendBuffer);
        if (connection->post != NULL && !connection->postingState.completed) {
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

    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Posting\n");
    bytesPosted = connection->postingState.bytesPosted;
    result = ghiPostDoPosting(connection);
    if (result == 0) {
        readable = 0;
        ghiPostCleanupState(connection);
        if (GSISocketSelect(connection->socket, &readable, NULL, NULL) == 1 && readable != 0) {
            connection->state = 8;
            ghiCallProgressCallback(connection, NULL, 0);
        }
        return;
    }
    if (result == 3) {
        connection->postingState.waitPostContinue = gsi_false;
        return;
    }
    if (bytesPosted != connection->postingState.bytesPosted) {
        ghiCallPostCallback(connection);
    }
    if (result == 1) {
        ghiPostCleanupState(connection);
        connection->postingState.completed = gsi_true;
        connection->state = 7;
        ghiCallProgressCallback(connection, NULL, 0);
    }
}

// Waits for the server to answer
void ghiDoWaiting(GHIConnection *connection) {
    int readable;
    int failed;
    int result;

    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Waiting\n");
    result = GSISocketSelect(connection->socket, &readable, NULL, &failed);
    if (result == -1 || (result == 1 && failed != 0)) {
        connection->completed = gsi_true;
        connection->result = 5;
        if (result == -1) {
            connection->socketError = GOAGetLastError(connection->socket);
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
static GHTTPBool ghiParseStatus(GHIConnection *connection) {
    int major;
    int minor;
    int code;
    int textEnd;
    char c;

    if (STD_TSScanf(connection->recvBuffer.data, "HTTP/%d.%d %d%n", &major, &minor, &code, &textEnd) != 3 ||
        major < 1 || code < 100 || code >= 600) {
        connection->completed = gsi_true;
        connection->result = 7;
        return gsi_false;
    }

    for (; (c = connection->recvBuffer.data[textEnd]) != '\0' && ghiIsSpace(c); textEnd++) {
    }
    connection->statusMajorVersion = major;
    connection->statusMinorVersion = minor;
    connection->statusCode = code;
    connection->statusStringIndex = textEnd;
    return gsi_true;
}

// Reads what the socket has until the status line is whole, and then moves on to the headers
void ghiDoReceivingStatus(GHIConnection *connection) {
    int size;
    char buffer[0x400];
    int result;
    char *lineEnd;
    int lineLength;

    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Receiving Status\n");
    size = sizeof(buffer);
    result = ghiDoReceive(connection, buffer, &size);
    if (result == 3 || result == 1) {
        return;
    }
    if (result == 0) {
        if (connection->encryptor.mEngine != 0 && connection->encryptor.mEncryptOnBuffer == 1) {
            if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
                return;
            }
            if (!ghiDecryptReceivedData(connection)) {
                connection->completed = gsi_true;
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
        connection->headerStringIndex = lineLength + 2;
        if (connection->statusCode == 100 && connection->postingState.waitPostContinue != 0) {
            connection->postingState.waitPostContinue = gsi_false;
            ghiResetBuffer(&connection->recvBuffer);
            connection->state = 6;
            ghiCallProgressCallback(connection, NULL, 0);
            gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_Comment, "Got HTTP continue\r\n");
            return;
        }
        connection->state = 9;
        ghiCallProgressCallback(connection, NULL, 0);
    } else if (result == 2) {
        connection->completed = gsi_true;
        connection->result = 7;
        connection->socketError = GOAGetLastError(connection->socket);
    }
}

// Hands what was received of the file to the game, and ends the request if all of it came
static GHTTPBool ghiDeliverIncomingFileData(GHIConnection *connection, const char *data, int size) {
    const char *delivered = NULL;
    int deliveredSize = 0;

    connection->fileBytesReceived += size;
    if (connection->fileBytesReceived == connection->totalSize || connection->connectionClosed != 0) {
        connection->completed = gsi_true;
    }
    if (connection->type == GHIGET) {
        if (!ghiAppendDataToBuffer(&connection->getFileBuffer, data, size)) {
            return gsi_false;
        }
        delivered = connection->getFileBuffer.data;
        deliveredSize = connection->getFileBuffer.len;
    } else if (connection->type == GHISAVE) {
        if (size != 0) {
            connection->completed = gsi_true;
            connection->result = 13;
            return gsi_false;
        }
        delivered = data;
        deliveredSize = size;
    } else if (connection->type == GHISTREAM) {
        delivered = data;
        deliveredSize = size;
    }
    ghiCallProgressCallback(connection, delivered, deliveredSize);
    return gsi_true;
}

// The size of the chunk that a chunk header tells, or -1
static int ghiParseChunkSize(GHIConnection *connection) {
    int size;

    if (STD_TSScanf(connection->chunkHeader, "%x", &size) != 1) {
        return -1;
    }
    return size;
}

// Adds to the header of the chunk that is being read
static void ghiAppendToChunkHeaderBuffer(GHIConnection *connection, const char *data, int size) {
    int count;

    if (size != 0 && connection->chunkHeaderLen < 10) {
        count = 10 - connection->chunkHeaderLen;
        if (count >= size) {
            count = size;
        }
        memcpy(connection->chunkHeader + connection->chunkHeaderLen, data, count);
        connection->chunkHeaderLen += count;
        connection->chunkHeader[connection->chunkHeaderLen] = '\0';
    }
}

// Sends the file's bytes that came to the game, taking them out of their chunks if the file comes in chunks. Returns
// gsi_false if the request failed
GHTTPBool ghiProcessIncomingFileData(GHIConnection *connection, const char *data, int size) {
    if (connection->chunkedTransfer) {
        while (size > 0) {
            if (connection->chunkReadingState == 0) {
                const char *lineEnd = strchr(data, '\n');
                int chunkSize;

                if (lineEnd != NULL) {
                    ghiAppendToChunkHeaderBuffer(connection, data, lineEnd - data);
                    size -= lineEnd + 1 - data;
                    data = lineEnd + 1;
                    chunkSize = ghiParseChunkSize(connection);
                    connection->chunkBytesLeft = chunkSize;
                    if (chunkSize == -1) {
                        connection->completed = gsi_true;
                        connection->result = 7;
                        return gsi_false;
                    }
                    if (chunkSize == 0) {
                        connection->chunkReadingState = 3;
                        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_RawDump,
                                      "Reading footer\n");
                    } else {
                        connection->chunkReadingState = 1;
                        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_RawDump,
                                      "Reading %d byte chunk\n", chunkSize);
                    }
                } else {
                    ghiAppendToChunkHeaderBuffer(connection, data, size);
                    return gsi_true;
                }
            } else if (connection->chunkReadingState == 1) {
                int count = connection->chunkBytesLeft;

                if (count >= size) {
                    count = size;
                }
                gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_RawDump, "Read %d bytes of chunk\n",
                              count);
                if (!ghiDeliverIncomingFileData(connection, data, count)) {
                    return gsi_false;
                }
                data += count;
                size -= count;
                connection->chunkBytesLeft -= count;
                if (connection->chunkBytesLeft == 0) {
                    connection->chunkReadingState = 2;
                }
            } else if (connection->chunkReadingState == 2) {
                const char *lineEnd = strchr(data, '\n');

                if (lineEnd == NULL) {
                    return gsi_true;
                }
                size -= lineEnd + 1 - data;
                data = lineEnd + 1;
                connection->chunkHeader[0] = '\0';
                connection->chunkHeaderLen = 0;
                connection->chunkBytesLeft = 0;
                connection->chunkReadingState = 0;
                gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_RawDump, "Read chunk footer\n");
            } else if (connection->chunkReadingState == 3) {
                connection->completed = gsi_true;
                gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_RawDump,
                              "Finished reading chunks\n");
                return gsi_true;
            } else {
                return gsi_false;
            }
        }
        return gsi_true;
    }
    return ghiDeliverIncomingFileData(connection, data, size);
}

static void ghiFreeIfSet(void *pointer) {
    if (pointer != NULL) {
        gsifree(pointer);
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
    char *headers;
    gsi_bool skipCR = gsi_true;

    buffer = gsimalloc(0x1000);
    if (buffer == NULL) {
        connection->completed = gsi_true;
        connection->result = 1;
        return;
    }
    size = 0x1000;
    result = ghiDoReceive(connection, buffer, &size);
    if (result == 3) {
        ghiFreeIfSet(buffer);
        return;
    }
    if (result == 1) {
        ghiFreeIfSet(buffer);
        return;
    }
    if (result == 0) {
        if (connection->encryptor.mEngine != 0 && connection->encryptor.mEncryptOnBuffer == 1) {
            if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
                ghiFreeIfSet(buffer);
                return;
            }
            if (!ghiDecryptReceivedData(connection)) {
                connection->completed = gsi_true;
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
    headersPos = connection->headerStringIndex;
    headers = data + headersPos;
    if (connection->statusCode / 100 == 1 && (strncmp(headers, "\r\n", 2) == 0 || strncmp(headers, "\n\n", 2) == 0)) {
        skipCR = gsi_false;
        headersEnd = headers;
    } else {
        headersEnd = strstr(headers, "\r\n\r\n");
    }
    if (headersEnd == NULL) {
        headersEnd = strstr(headers, "\n\n");
    }
    if (headersEnd != NULL) {
        if (skipCR == gsi_true) {
            headersEnd += 2;
        }
        headersEnd[0] = '\0';
        body = headersEnd + 2;
        bodyLength = connection->recvBuffer.len - (body - connection->recvBuffer.data);
        connection->recvBuffer.len = headersEnd - connection->recvBuffer.data + 1;
        connection->recvBuffer.pos = connection->recvBuffer.len;

        if (connection->statusCode / 100 == 1) {
            if (bodyLength != 0) {
                memmove(connection->recvBuffer.data, body, bodyLength + 1);
                connection->recvBuffer.len = bodyLength;
            } else {
                ghiResetBuffer(&connection->recvBuffer);
            }
            if (connection->postingState.waitPostContinue != 0) {
                connection->postingState.waitPostContinue = gsi_false;
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
                connection->completed = gsi_true;
                connection->result = 11;
                ghiFreeIfSet(buffer);
                return;
            }
            location = strstr(headers, "Location:");
            if (location != NULL) {
                char *end;

                location += 9;
                while (ghiIsSpace(location[0])) {
                    location++;
                }
                end = location;
                while (end[0] != '\0' && !ghiIsSpace(end[0])) {
                    end++;
                }
                end[0] = '\0';
                if (location[0] == '/') {
                    connection->redirectURL = gsimalloc(strlen(connection->serverAddress) + 14 + strlen(location));
                    if (connection->redirectURL == NULL) {
                        connection->completed = gsi_true;
                        connection->result = 1;
                    }
                    OS_SPrintf(connection->redirectURL, "http://%s:%d%s", connection->serverAddress,
                               connection->serverPort, location);
                } else {
                    connection->redirectURL = goastrdup(location);
                    if (connection->redirectURL == NULL) {
                        connection->completed = gsi_true;
                        connection->result = 1;
                    }
                }
                ghiFreeIfSet(buffer);
                return;
            }
        }

        contentLength = strstr(headers, "Content-Length:");
        if (contentLength != NULL) {
            char maxValue[11] = "2147483647";
            char *digits = contentLength + 16;
            char *digitsEnd = digits;
            int maxLength = strlen(maxValue);
            int digitsLength;

            for (; digitsEnd != NULL && digitsEnd[0] != '\0' && digitsEnd[0] != '\n' && digitsEnd[0] != '\r' &&
                   digitsEnd[0] != ' ';
                 digitsEnd++) {
            }
            digitsLength = digitsEnd - digits;
            if (digitsLength > maxLength) {
                connection->completed = gsi_true;
                connection->result = 16;
                ghiFreeIfSet(buffer);
                return;
            }
            if (maxLength == digitsLength && strncmp(digits, maxValue, digitsLength) >= 0) {
                connection->completed = gsi_true;
                connection->result = 16;
                ghiFreeIfSet(buffer);
                return;
            }
            connection->totalSize = atol(digits);
        }

        connection->chunkedTransfer = strstr(headers, "Transfer-Encoding: chunked") != NULL ? gsi_true : gsi_false;
        if (connection->chunkedTransfer) {
            connection->chunkHeader[0] = '\0';
            connection->chunkHeaderLen = 0;
            connection->chunkBytesLeft = 0;
            connection->chunkReadingState = 0;
        }
        if (connection->type == GHIHEAD || connection->type == GHIPOST) {
            connection->completed = gsi_true;
            ghiFreeIfSet(buffer);
            return;
        }
        connection->state = 10;
        if (contentLength != NULL && connection->totalSize == 0) {
            connection->completed = gsi_true;
            ghiFreeIfSet(buffer);
            return;
        }
        if (bodyLength > 0) {
            ghiProcessIncomingFileData(connection, body, bodyLength);
        }
    } else if (result == 2) {
        connection->completed = gsi_true;
        connection->result = 7;
        connection->socketError = GOAGetLastError(connection->socket);
    }
    ghiFreeIfSet(buffer);
}

// Reads the file the request asks for, as long as it can for a while, and gives it to the game
void ghiDoReceivingFile(GHIConnection *connection) {
    gsi_u32 startTime = current_time();
    int size = 0;
    char *buffer;
    gsi_u32 elapsed = 0;
    int result;

    buffer = gsimalloc(0x2000);
    if (buffer == NULL) {
        connection->completed = gsi_true;
        connection->result = 1;
        return;
    }
    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Receiving File\n");

    while (!connection->completed && elapsed < connection->maxRecvTime) {
        size = 0x2000;
        result = ghiDoReceive(connection, buffer, &size);
        if (result == 1) {
            ghiFreeIfSet(buffer);
            if (connection->receiveFileIdleTime == 0) {
                connection->receiveFileIdleTime = current_time();
                return;
            }
            if (current_time() - connection->receiveFileIdleTime < 30000) {
                return;
            }
            gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_HotError,
                          "Receive File Idle Timeout %d ===== \r\n", current_time() - connection->receiveFileIdleTime);
            connection->receiveFileIdleTime = 0;
            connection->completed = gsi_true;
            connection->result = 19;
            return;
        }
        connection->receiveFileIdleTime = 0;
        if (result == 3) {
            ghiFreeIfSet(buffer);
            return;
        }
        if (result == 2) {
            connection->completed = gsi_true;
            if (connection->totalSize > 0 && connection->fileBytesReceived < connection->totalSize) {
                connection->result = 15;
            }
            ghiFreeIfSet(buffer);
            return;
        }

        if (connection->encryptor.mEngine != 0 && connection->encryptor.mEncryptOnBuffer == 1) {
            int received;

            if (!ghiAppendDataToBuffer(&connection->decodeBuffer, buffer, size)) {
                ghiFreeIfSet(buffer);
                return;
            }
            connection->recvBuffer.len = connection->recvBuffer.pos;
            if (!ghiDecryptReceivedData(connection)) {
                connection->completed = gsi_true;
                connection->result = 0x11;
                ghiFreeIfSet(buffer);
                return;
            }
            received = connection->recvBuffer.len - connection->recvBuffer.pos;
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
        elapsed = current_time() - startTime;
    }
    ghiFreeIfSet(buffer);
}
