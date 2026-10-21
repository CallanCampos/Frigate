#include "ghttp_internal.h"

// ghttpMain: starting the client, a request and the walk of the requests' states

// How many times the client was started: it is stopped when the count comes back to 0
static int sStartCount;

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
gsi_bool ghiProcessConnection(GHIConnection *connection) {
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
        gsDebugFormat(8, 3, 0x10, "Connection Closing\n");
    }

    if (connection->redirectUrl != NULL) {
        ghiRedirectConnection(connection);
    }

    completed = connection->completed;
    if (connection->result == 0x12 && !completed) {
        if (!CanReceiveOnSocket(connection->socket)) {
            connection->completed = gsi_true;
        }
    }

    if (connection->completed) {
        if (connection->encryptor.secure == gsi_true) {
            ghiHandleStatus(connection);
            if (!ghiCloseSocket(connection)) {
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
            if (!ghiCloseSocket(connection)) {
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
    sStartCount++;
    if (sStartCount == 1) {
        ghiCreateLock();
        ghiThrottleBytes = 125;
        ghiThrottleInterval = 250;
        return;
    }
    ghiUnlock();
}

void ghttpCleanup(void) {
    ghiLock();
    sStartCount--;
    if (sStartCount == 0) {
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
int ghttpGetExA(const char *url, const char *headers, char *buffer, int bufferSize, GHIPost *post, gsi_bool throttle,
                gsi_bool blocking, GHIProgressFunc progress, GHICompletedFunc completed, void *param) {
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
    if (sStartCount == 0) {
        ghttpStartup();
    }

    connection = ghiNewConnection();
    if (connection == NULL) {
        return -1;
    }
    connection->type = 0;
    connection->url = goastrdup(url);
    if (connection->url == NULL) {
        ghiFreeConnection(connection);
        return -1;
    }
    if (headers != NULL && headers[0] != '\0') {
        connection->headers = goastrdup(headers);
        if (connection->headers == NULL) {
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
    connection->userBuffer = buffer != NULL;
    if (connection->userBuffer) {
        ready = ghiInitFixedBuffer(connection, &connection->fileBuffer, buffer, bufferSize);
    } else {
        ready = ghiInitBuffer(connection, &connection->fileBuffer, 0x800, 0x800);
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

gsi_bool ghttpThinkRequest(int request) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection == NULL) {
        return gsi_false;
    }
    ghiProcessConnection(connection);
    return gsi_true;
}

// Ends a request, as cancelled (0x12)
void ghttpCancelRequest(int request) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        connection->result = 0x12;
        connection->state = 11;
        connection->completed = gsi_true;
        ghiCloseSocket(connection);
    }
}

int ghttpGetState(int request) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        return connection->state;
    }
    return 0;
}

void ghttpSetMaxRecvTime(int request, int time) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        connection->maxRecvTime = time;
    }
}

GHIPost *ghttpNewPost(void) {
    return ghiNewPost();
}

void ghttpPostSetAutoFree(GHIPost *post, gsi_bool autoFree) {
    if (post != NULL) {
        ghiPostSetAutoFree(post, autoFree);
    }
}

void ghttpFreePost(int request, GHIPost *post) {
    if (post != NULL) {
        ghiFreePostOfRequest(request, post);
    }
}

gsi_bool ghttpPostAddXml(GHIPost *post, SoapWriter *writer) {
    return ghiPostAddXml(post, writer);
}

gsi_bool ghttpSetRootCAList(const char *url, GHICertList *rootCA) {
    return ghiEncyptorSetRootCAList(url, rootCA);
}

gsi_bool ghttpRemoveRootCAList(const char *url) {
    return ghiEncryptorRemoveRootCAList(url);
}
