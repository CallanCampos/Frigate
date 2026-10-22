#include "ghttp_internal.h"

// ghttpMain: starting the client, a request and the walk of the requests' states

// How many times the client was started: it is stopped when the count comes back to 0
static int ghiReferenceCount;

// Gives a connection that the server answered with an error its result, by the status code
void ghiHandleStatus(GHIConnection *connection) {
    switch (connection->statusCode / 100) {
    case 1:
    case 2:
    case 3:
        return;
    case 4:
        switch (connection->statusCode) {
        case 401:
            connection->result = 9;
            break;
        case 403:
            connection->result = 10;
            break;
        case 404:
        case 410:
            connection->result = 11;
            break;
        case 402:
        case 405:
        case 406:
        case 407:
        case 408:
        case 409:
        default:
            connection->result = 8;
            break;
        }
        break;
    case 5:
        connection->result = 12;
        break;
    }
}

// Does what the state of the connection has to do. Returns gsi_true once it has ended
GHTTPBool ghiProcessConnection(GHIConnection *connection) {
    gsi_bool completed;

    if (connection->processing) {
        return gsi_false;
    }
    connection->processing = gsi_true;

    if (connection->state == 0) {
        ghiDoSocketInit(connection);
    }
    if (connection->state == 1) {
        ghiDoHostLookup(connection);
    }
    if (connection->state == 2) {
        ghiDoLookupPending(connection);
    }
    if (connection->state == 3) {
        ghiDoConnecting(connection);
    }
    if (connection->state == 4) {
        ghiDoSecuringSession(connection);
    }
    if (connection->state == 5) {
        ghiDoSendingRequest(connection);
    }
    if (connection->state == 6) {
        ghiDoPosting(connection);
    }
    if (connection->state == 7) {
        ghiDoWaiting(connection);
    }
    if (connection->state == 8) {
        ghiDoReceivingStatus(connection);
    }
    if (connection->state == 9) {
        ghiDoReceivingHeaders(connection);
    }
    if (connection->state == 10) {
        ghiDoReceivingFile(connection);
    }
    if (connection->state == 11) {
        gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment, "Connection Closing\n");
    }

    if (connection->redirectURL != NULL) {
        ghiRedirectConnection(connection);
    }

    completed = connection->completed;
    if (connection->result == 0x12 && !completed) {
        if (!CanReceiveOnSocket(connection->socket)) {
            connection->completed = gsi_true;
        }
    }

    if (connection->completed) {
        if (connection->encryptor.mNotInCloseSocket == gsi_true) {
            ghiHandleStatus(connection);
            if (!ghiCloseConnection(connection)) {
                connection->processing = gsi_false;
                connection->state = 11;
                return gsi_false;
            }
            ghiCallCompletedCallback(connection);
            if (!ghiFreeConnectionData(connection)) {
                connection->processing = gsi_false;
                return gsi_false;
            }
        } else {
            if (!ghiCloseConnection(connection)) {
                connection->processing = gsi_false;
                connection->state = 11;
                return gsi_false;
            }
            ghiCallCompletedCallback(connection);
            if (!ghiFreeConnectionData(connection)) {
                connection->processing = gsi_false;
                return gsi_false;
            }
        }
    } else {
        connection->processing = gsi_false;
    }
    return completed;
}

void ghttpStartup(void) {
    ghiLock();
    ghiReferenceCount++;
    if (ghiReferenceCount == 1) {
        ghiCreateLock();
        ghiThrottleBufferSize = 125;
        ghiThrottleTimeDelay = 250;
        return;
    }
    ghiUnlock();
}

void ghttpCleanup(void) {
    ghiLock();
    ghiReferenceCount--;
    if (ghiReferenceCount == 0) {
        ghiCleanupConnections();
        if (ghiProxyAddress != NULL) {
            gsifree(ghiProxyAddress);
            ghiProxyAddress = NULL;
        }
        ghiUnlock();
        ghiFreeLock();
    } else {
        ghiUnlock();
    }
}

// Starts a request for a URL. With a buffer the result is received into it, without one into a buffer of the client's.
// Returns the request, or -1 if it couldn't be made. If blocking it only returns when the request has ended, with 0
GHTTPRequest ghttpGetExA(const char *url, const char *headers, char *buffer, int bufferSize, GHTTPPost post,
                         GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progress,
                         ghttpCompletedCallback completed, void *param) {
    GHIConnection *connection;
    gsi_bool ready;

    if (url == NULL || url[0] == '\0') {
        return -1;
    }
    if (bufferSize < 0) {
        return -1;
    }
    if (buffer != NULL && bufferSize == 0) {
        return -1;
    }
    if (ghiReferenceCount == 0) {
        ghttpStartup();
    }

    connection = ghiNewConnection();
    if (connection == NULL) {
        return -1;
    }
    connection->type = 0;
    connection->URL = goastrdup(url);
    if (connection->URL == NULL) {
        ghiFreeConnection(connection);
        return -1;
    }
    if (headers != NULL && headers[0] != '\0') {
        connection->sendHeaders = goastrdup(headers);
        if (connection->sendHeaders == NULL) {
            ghiFreeConnection(connection);
            return -1;
        }
    }
    connection->post = post;
    connection->blocking = blocking;
    connection->progressCallback = progress;
    connection->completedCallback = completed;
    connection->callbackParam = param;
    connection->throttle = throttle;
    connection->userBufferSupplied = buffer != NULL;
    if (connection->userBufferSupplied) {
        ready = ghiInitFixedBuffer(connection, &connection->getFileBuffer, buffer, bufferSize);
    } else {
        ready = ghiInitBuffer(connection, &connection->getFileBuffer, 0x800, 0x800);
    }
    if (!ready) {
        ghiFreeConnection(connection);
        return -1;
    }
    if (post != NULL) {
        if (!ghiPostInitState(connection)) {
            ghiFreeConnection(connection);
            return -1;
        }
    }

    if (blocking) {
        if (!ghiProcessConnection(connection)) {
            do {
                msleep(10);
            } while (!ghiProcessConnection(connection));
        }
        return 0;
    }
    return connection->request;
}

// Walks all the requests
void ghttpThink(void) {
    ghiEnumConnections(ghiProcessConnection);
}

GHTTPBool ghttpRequestThink(GHTTPRequest request) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection == NULL) {
        return gsi_false;
    }
    ghiProcessConnection(connection);
    return gsi_true;
}

// Ends a request, as cancelled (0x12)
void ghttpCancelRequest(GHTTPRequest request) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        connection->result = 0x12;
        connection->state = 11;
        connection->completed = gsi_true;
        ghiCloseConnection(connection);
    }
}

GHTTPState ghttpGetState(GHTTPRequest request) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        return connection->state;
    }
    return 0;
}

void ghttpSetMaxRecvTime(GHTTPRequest request, gsi_time time) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        connection->maxRecvTime = time;
    }
}

GHTTPPost ghttpNewPost(void) {
    return ghiNewPost();
}

void ghttpPostSetAutoFree(GHTTPPost post, GHTTPBool autoFree) {
    if (post != NULL) {
        ghiPostSetAutoFree(post, autoFree);
    }
}

void ghttpFreePost(GHTTPRequest request, GHTTPPost post) {
    if (post != NULL) {
        ghiFreePostAndUpdateConnection(request, post);
    }
}

GHTTPBool ghttpPostAddXml(GHTTPPost post, GSXmlStreamWriter writer) {
    return ghiPostAddXml(post, writer);
}

GHTTPBool ghttpSetRootCAList(const char *url, GHICertList *rootCA) {
    return ghiEncyptorSetRootCAList(url, rootCA);
}

GHTTPBool ghttpCleanupRootCAList(const char *url) {
    return ghiEncyptorCleanupRootCAList(url);
}
