#include "ghttp_internal.h"

// ghttpConnection: the list of the connections of the requests, and the making, ending and walking of them

typedef struct GHIConnectionList {
    int count;
    int activeCount;
    int nextUniqueId;
    GHIConnection **connections;
} GHIConnectionList;

static GHIConnectionList sConnections;

// How many slots the list grows by
#define GHI_CONNECTION_GROWTH 4

// Gives the index of a free slot, growing the list if none is. Returns -1 if it can't
int ghiFindFreeSlot(void) {
    int i;
    int count;
    int newCount;
    GHIConnection **connections;

    for (i = 0; i < sConnections.count; i++) {
        if (!sConnections.connections[i]->inUse) {
            return i;
        }
    }

    count = sConnections.count;
    newCount = count + GHI_CONNECTION_GROWTH;
    connections = func_ov011_0216ddec(sConnections.connections, newCount * sizeof(GHIConnection *));
    if (connections == NULL) {
        return -1;
    }
    sConnections.connections = connections;
    for (i = count; i < newCount; i++) {
        sConnections.connections[i] = func_ov011_0216dddc(sizeof(GHIConnection));
        if (sConnections.connections[i] == NULL) {
            for (i--; i >= count; i--) {
                func_ov011_0216ddfc(sConnections.connections[i]);
            }
            return -1;
        }
        sConnections.connections[i]->inUse = FALSE;
    }
    sConnections.count = newCount;
    return count;
}

// Makes a connection in a free slot, with nothing set but its buffers. Returns NULL if it can't
GHIConnection *ghiNewConnection(void) {
    int slot;
    GHIConnection *connection;

    ghiLock();
    slot = ghiFindFreeSlot();
    if (slot == -1) {
        ghiUnlock();
        return NULL;
    }
    connection = sConnections.connections[slot];
    memset(connection, 0, sizeof(GHIConnection));
    connection->inUse = TRUE;
    connection->request = slot;
    connection->uniqueId = sConnections.nextUniqueId++;
    connection->type = 0;
    connection->state = 0;
    connection->url = NULL;
    connection->serverAddress = NULL;
    connection->serverIp = 0;
    connection->serverPort = 0;
    connection->requestPath = NULL;
    connection->headers = NULL;
    connection->unk_30 = 0;
    connection->blocking = FALSE;
    connection->keepAlive = FALSE;
    connection->result = 0;
    connection->progressCallback = NULL;
    connection->completedCallback = NULL;
    connection->callbackParam = NULL;
    connection->socket = -1;
    connection->socketError = 0;
    connection->userBuffer = FALSE;
    connection->httpMajor = 0;
    connection->httpMinor = 0;
    connection->statusCode = 0;
    connection->statusTextPos = 0;
    connection->headersPos = 0;
    connection->completed = FALSE;
    connection->bytesReceived = 0;
    connection->totalSize = -1;
    connection->redirectUrl = NULL;
    connection->redirectCount = 0;
    connection->chunked = 0;
    connection->processing = 0;
    connection->throttle = 0;
    connection->lastThrottleTime = 0;
    connection->post = NULL;
    connection->maxRecvTime = 500;
    connection->proxyPort = 80;
    connection->proxyAddress = NULL;
    connection->encryptor.state = NULL;
    connection->encryptor.open = FALSE;
    connection->hostLookup = NULL;
    connection->lastRecvTime = 0;

    if (ghiInitBuffer(connection, &connection->sendBuffer, 0x800, 0x1000) &&
        ghiInitBuffer(connection, &connection->encodeBuffer, 0x800, 0x400) &&
        ghiInitBuffer(connection, &connection->recvBuffer, 0x800, 0x800) &&
        ghiInitBuffer(connection, &connection->decodeBuffer, 0x800, 0x400)) {
        sConnections.activeCount++;
        ghiUnlock();
        return connection;
    }
    ghiFreeConnection(connection);
    ghiUnlock();
    return NULL;
}

// Ends the connection's lookup of its host and its socket, and says if that went well
BOOL ghiCloseSocket(GHIConnection *connection) {
    if (connection == NULL) {
        return FALSE;
    }
    if (!connection->inUse) {
        return FALSE;
    }
    if (connection->request < 0) {
        return FALSE;
    }
    if (connection->request >= sConnections.count) {
        return FALSE;
    }

    ghiLock();
    if (connection->hostLookup != NULL) {
        func_ov011_0216cca8(8, 3, 0x10, "Cancelling Thread and freeing memory\n");
        func_ov011_0216e2b4(connection->hostLookup);
        connection->hostLookup = NULL;
    }
    if (connection->socket != -1) {
        func_ov011_0216de7c(connection->socket, 2);
        if (!func_ov011_0216de40(connection->socket, &connection->encryptor.secure)) {
            ghiUnlock();
            return FALSE;
        }
        connection->socket = -1;
    }
    ghiUnlock();
    return TRUE;
}

// Frees what a connection owns and gives its slot back
BOOL ghiFreeConnectionData(GHIConnection *connection) {
    if (connection == NULL) {
        return FALSE;
    }
    if (!connection->inUse) {
        return FALSE;
    }
    if (connection->request < 0) {
        return FALSE;
    }
    if (connection->request >= sConnections.count) {
        return FALSE;
    }

    ghiLock();
    func_ov011_0216ddfc(connection->url);
    connection->url = NULL;
    func_ov011_0216ddfc(connection->unk_34);
    connection->unk_34 = NULL;
    func_ov011_0216ddfc(connection->serverAddress);
    connection->serverAddress = NULL;
    func_ov011_0216ddfc(connection->requestPath);
    connection->requestPath = NULL;
    func_ov011_0216ddfc(connection->headers);
    connection->headers = NULL;
    func_ov011_0216ddfc(connection->redirectUrl);
    // The redirect URL is not cleared, the headers are cleared a second time instead. The slot's memory is cleared when
    // it is used again
    connection->headers = NULL;
    func_ov011_0216ddfc(connection->proxyAddress);
    connection->proxyAddress = NULL;
    ghiFreeBuffer(&connection->sendBuffer);
    ghiFreeBuffer(&connection->encodeBuffer);
    ghiFreeBuffer(&connection->recvBuffer);
    ghiFreeBuffer(&connection->decodeBuffer);
    ghiFreeBuffer(&connection->fileBuffer);
    if (connection->postingState.states != NULL) {
        ghiPostCleanupState(connection);
    }
    if (connection->encryptor.open) {
        if (connection->encryptor.cleanup != NULL) {
            connection->encryptor.cleanup(connection, &connection->encryptor);
        }
        connection->encryptor.open = FALSE;
    }
    connection->lastRecvTime = 0;
    connection->inUse = FALSE;
    sConnections.activeCount--;
    ghiUnlock();
    return TRUE;
}

BOOL ghiFreeConnection(GHIConnection *connection) {
    if (connection == NULL) {
        return FALSE;
    }
    if (!connection->inUse) {
        return FALSE;
    }
    if (connection->request < 0) {
        return FALSE;
    }
    if (connection->request >= sConnections.count) {
        return FALSE;
    }
    if (ghiCloseSocket(connection) != TRUE) {
        return FALSE;
    }
    return ghiFreeConnectionData(connection);
}

// Gives the connection of a request, or NULL if the request has none
GHIConnection *ghiRequestToConnection(int request) {
    GHIConnection *connection;

    ghiLock();
    if (request < 0 || request >= sConnections.count) {
        ghiUnlock();
        return NULL;
    }
    connection = sConnections.connections[request];
    if (!connection->inUse) {
        connection = NULL;
    }
    ghiUnlock();
    return connection;
}

// Calls the function with each connection that is in use
void ghiEnumConnections(GHIEnumFunc func) {
    int i;

    if (sConnections.activeCount > 0) {
        ghiLock();
        for (i = 0; i < sConnections.count; i++) {
            if (sConnections.connections[i]->inUse) {
                func(sConnections.connections[i]);
            }
        }
        ghiUnlock();
    }
}

// Starts a connection over at the address that it was redirected to
void ghiRedirectConnection(GHIConnection *connection) {
    func_ov011_0216cca8(8, 3, 0x10, "Redirecting Connection\n");
    if (connection->hostLookup != NULL) {
        func_ov011_0216cca8(8, 3, 0x10, "Cancelling Thread and freeing memory\n");
        func_ov011_0216e2b4(connection->hostLookup);
        connection->hostLookup = NULL;
    }
    func_ov011_0216de7c(connection->socket, 2);
    if (func_ov011_0216de40(connection->socket, &connection->encryptor.secure)) {
        func_ov011_0216ddfc(connection->url);
        connection->url = connection->redirectUrl;
        connection->redirectUrl = NULL;
        func_ov011_0216ddfc(connection->serverAddress);
        connection->serverAddress = NULL;
        connection->serverIp = 0;
        connection->serverPort = 0;
        func_ov011_0216ddfc(connection->requestPath);
        connection->socket = -1;
        connection->requestPath = NULL;
        connection->state = 0;
        ghiResetBuffer(&connection->sendBuffer);
        ghiResetBuffer(&connection->encodeBuffer);
        ghiResetBuffer(&connection->recvBuffer);
        ghiResetBuffer(&connection->decodeBuffer);
        connection->httpMajor = 0;
        connection->httpMinor = 0;
        connection->statusCode = 0;
        connection->statusTextPos = 0;
        connection->headersPos = 0;
        connection->closing = FALSE;
        if (connection->encryptor.open) {
            if (connection->encryptor.cleanup != NULL) {
                connection->encryptor.cleanup(connection, &connection->encryptor);
            }
            connection->encryptor.open = FALSE;
            if (strncmp("https://", connection->url, 8) != 0) {
                connection->encryptor.engine = 0;
                connection->encryptor.state = NULL;
            }
        }
        connection->redirectCount++;
    }
}

// Ends every connection and frees the list
void ghiCleanupConnections(void) {
    int i;

    if (sConnections.connections != NULL) {
        ghiEnumConnections(ghiFreeConnection);
        for (i = 0; i < sConnections.count; i++) {
            func_ov011_0216ddfc(sConnections.connections[i]);
        }
        func_ov011_0216ddfc(sConnections.connections);
        sConnections.connections = NULL;
        sConnections.count = 0;
        sConnections.activeCount = 0;
    }
}

// Copies the name of the host of a URL, which is what is between its scheme and the first colon or slash. Returns NULL
// if the URL has no scheme
char *ghiCopyHost(char *url) {
    char *host;
    u32 length;
    char saved;
    char *copy;

    if (strncmp(url, "http://", 7) == 0) {
        host = url + 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        host = url + 8;
    } else {
        return NULL;
    }
    length = strcspn(host, ":/");
    saved = host[length];
    host[length] = '\0';
    copy = func_ov011_0216e2c8(host);
    host[length] = saved;
    return copy;
}
