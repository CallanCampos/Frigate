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
    connections = gsirealloc(sConnections.connections, newCount * sizeof(GHIConnection *));
    if (connections == NULL) {
        return -1;
    }
    sConnections.connections = connections;
    for (i = count; i < newCount; i++) {
        sConnections.connections[i] = gsimalloc(sizeof(GHIConnection));
        if (sConnections.connections[i] == NULL) {
            for (i--; i >= count; i--) {
                gsifree(sConnections.connections[i]);
            }
            return -1;
        }
        sConnections.connections[i]->inUse = gsi_false;
    }
    sConnections.count = newCount;
    return count;
}

// Makes a connection in a free slot, with nothing set but its buffers. Returns NULL if it can't
GHIConnection *ghiNewConnection(void) {
    GHTTPBool result;
    GHIConnection *connection;
    int slot;

    ghiLock();
    slot = ghiFindFreeSlot();
    if (slot == -1) {
        ghiUnlock();
        return NULL;
    }
    connection = sConnections.connections[slot];
    memset(connection, 0, sizeof(GHIConnection));
    connection->inUse = gsi_true;
    connection->request = slot;
    connection->uniqueID = sConnections.nextUniqueId++;
    connection->type = 0;
    connection->state = 0;
    connection->URL = NULL;
    connection->serverAddress = NULL;
    connection->serverIP = 0;
    connection->serverPort = 0;
    connection->requestPath = NULL;
    connection->sendHeaders = NULL;
    connection->unk_30 = 0;
    connection->blocking = gsi_false;
    connection->persistConnection = gsi_false;
    connection->result = 0;
    connection->progressCallback = NULL;
    connection->completedCallback = NULL;
    connection->callbackParam = NULL;
    connection->socket = -1;
    connection->socketError = 0;
    connection->userBufferSupplied = gsi_false;
    connection->statusMajorVersion = 0;
    connection->statusMinorVersion = 0;
    connection->statusCode = 0;
    connection->statusStringIndex = 0;
    connection->headerStringIndex = 0;
    connection->completed = gsi_false;
    connection->fileBytesReceived = 0;
    connection->totalSize = -1;
    connection->redirectURL = NULL;
    connection->redirectCount = 0;
    connection->chunkedTransfer = 0;
    connection->processing = 0;
    connection->throttle = 0;
    connection->lastThrottleRecv = 0;
    connection->post = NULL;
    connection->maxRecvTime = 500;
    connection->proxyOverridePort = 80;
    connection->proxyOverrideServer = NULL;
    connection->encryptor.mInterface = NULL;
    connection->encryptor.mInitialized = gsi_false;
    connection->receiveFileIdleTime = 0;
    connection->handle = NULL;

    result = ghiInitBuffer(connection, &connection->sendBuffer, 0x800, 0x1000);
    if (result) {
        result = ghiInitBuffer(connection, &connection->encodeBuffer, 0x800, 0x400);
    }
    if (result) {
        result = ghiInitBuffer(connection, &connection->recvBuffer, 0x800, 0x800);
    }
    if (result) {
        result = ghiInitBuffer(connection, &connection->decodeBuffer, 0x800, 0x400);
    }
    if (!result) {
        ghiFreeConnection(connection);
        ghiUnlock();
        return NULL;
    }
    sConnections.activeCount++;
    ghiUnlock();
    return connection;
}

// Ends the connection's lookup of its host and its socket, and says if that went well
GHTTPBool ghiCloseConnection(GHIConnection *connection) {
    if (connection == NULL) {
        return gsi_false;
    }
    if (!connection->inUse) {
        return gsi_false;
    }
    if (connection->request < 0) {
        return gsi_false;
    }
    if (connection->request >= sConnections.count) {
        return gsi_false;
    }

    ghiLock();
    if (connection->handle != NULL) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment,
                      "Cancelling Thread and freeing memory\n");
        gsiCancelResolvingHostname(connection->handle);
        connection->handle = NULL;
    }
    if (connection->socket != -1) {
        shutdown(connection->socket, 2);
        if (!closesocketsure(connection->socket, &connection->encryptor.mNotInCloseSocket)) {
            ghiUnlock();
            return gsi_false;
        }
        connection->socket = -1;
    }
    ghiUnlock();
    return gsi_true;
}

// Frees what a connection owns and gives its slot back
GHTTPBool ghiFreeConnectionData(GHIConnection *connection) {
    if (connection == NULL) {
        return gsi_false;
    }
    if (!connection->inUse) {
        return gsi_false;
    }
    if (connection->request < 0) {
        return gsi_false;
    }
    if (connection->request >= sConnections.count) {
        return gsi_false;
    }

    ghiLock();
    gsifree(connection->URL);
    connection->URL = NULL;
    gsifree(connection->unk_34);
    connection->unk_34 = NULL;
    gsifree(connection->serverAddress);
    connection->serverAddress = NULL;
    gsifree(connection->requestPath);
    connection->requestPath = NULL;
    gsifree(connection->sendHeaders);
    connection->sendHeaders = NULL;
    gsifree(connection->redirectURL);
    // The redirect URL is not cleared, the headers are cleared a second time instead. The slot's memory is cleared when
    // it is used again
    connection->sendHeaders = NULL;
    gsifree(connection->proxyOverrideServer);
    connection->proxyOverrideServer = NULL;
    ghiFreeBuffer(&connection->sendBuffer);
    ghiFreeBuffer(&connection->encodeBuffer);
    ghiFreeBuffer(&connection->recvBuffer);
    ghiFreeBuffer(&connection->decodeBuffer);
    ghiFreeBuffer(&connection->getFileBuffer);
    if (connection->postingState.states != NULL) {
        ghiPostCleanupState(connection);
    }
    if (connection->encryptor.mInitialized) {
        if (connection->encryptor.mCleanupFunc != NULL) {
            connection->encryptor.mCleanupFunc(connection, &connection->encryptor);
        }
        connection->encryptor.mInitialized = gsi_false;
    }
    connection->receiveFileIdleTime = 0;
    connection->inUse = gsi_false;
    sConnections.activeCount--;
    ghiUnlock();
    return gsi_true;
}

GHTTPBool ghiFreeConnection(GHIConnection *connection) {
    if (connection == NULL) {
        return gsi_false;
    }
    if (!connection->inUse) {
        return gsi_false;
    }
    if (connection->request < 0) {
        return gsi_false;
    }
    if (connection->request >= sConnections.count) {
        return gsi_false;
    }
    if (ghiCloseConnection(connection) != gsi_true) {
        return gsi_false;
    }
    return ghiFreeConnectionData(connection);
}

// Gives the connection of a request, or NULL if the request has none
GHIConnection *ghiRequestToConnection(GHTTPRequest request) {
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
void ghiEnumConnections(GHTTPBool (*func)(GHIConnection *)) {
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
    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Redirecting Connection\n");
    if (connection->handle != NULL) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment,
                      "Cancelling Thread and freeing memory\n");
        gsiCancelResolvingHostname(connection->handle);
        connection->handle = NULL;
    }
    shutdown(connection->socket, 2);
    if (closesocketsure(connection->socket, &connection->encryptor.mNotInCloseSocket)) {
        gsifree(connection->URL);
        connection->URL = connection->redirectURL;
        connection->redirectURL = NULL;
        gsifree(connection->serverAddress);
        connection->serverAddress = NULL;
        connection->serverIP = 0;
        connection->serverPort = 0;
        gsifree(connection->requestPath);
        connection->socket = -1;
        connection->requestPath = NULL;
        connection->state = 0;
        ghiResetBuffer(&connection->sendBuffer);
        ghiResetBuffer(&connection->encodeBuffer);
        ghiResetBuffer(&connection->recvBuffer);
        ghiResetBuffer(&connection->decodeBuffer);
        connection->statusMajorVersion = 0;
        connection->statusMinorVersion = 0;
        connection->statusCode = 0;
        connection->statusStringIndex = 0;
        connection->headerStringIndex = 0;
        connection->connectionClosed = gsi_false;
        if (connection->encryptor.mInitialized) {
            if (connection->encryptor.mCleanupFunc != NULL) {
                connection->encryptor.mCleanupFunc(connection, &connection->encryptor);
            }
            connection->encryptor.mInitialized = gsi_false;
            if (strncmp("https://", connection->URL, 8) != 0) {
                connection->encryptor.mEngine = 0;
                connection->encryptor.mInterface = NULL;
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
            gsifree(sConnections.connections[i]);
        }
        gsifree(sConnections.connections);
        sConnections.connections = NULL;
        sConnections.count = 0;
        sConnections.activeCount = 0;
    }
}

// Copies the name of the host of a URL, which is what is between its scheme and the first colon or slash. Returns NULL
// if the URL has no scheme
char *ghiGetServerAddressFromUrl(char *url) {
    char *host;
    gsi_u32 length;
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
    copy = goastrdup(host);
    host[length] = saved;
    return copy;
}
