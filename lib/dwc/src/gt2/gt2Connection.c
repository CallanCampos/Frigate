#include "dwc/gt2Connection.h"
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's connections (gt2Connection.c)

static BOOL gti2CheckTimeout(GTI2Connection *connection, u32 now);
static BOOL gti2SendRetries(GTI2Connection *connection, u32 now);
static BOOL gti2CheckPendingAck(GTI2Connection *connection, u32 now);
static BOOL gti2CheckKeepAlive(GTI2Connection *connection, u32 now);

int gti2NewOutgoingConnection(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port) {
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = 0;
    (*connection)->initiated = TRUE;
    return 0;
}

int gti2NewIncomingConnection(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port) {
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = 2;
    (*connection)->initiated = FALSE;
    return 0;
}

int gti2StartConnectionAttempt(GTI2Connection *connection, const void *message, int length,
                               const GTI2ConnectionCallbacks *callbacks) {
    u8 challenge[0x20];

    gti2MessageCheck(&message, &length);
    if (length > 0) {
        connection->initialMessage = gsimalloc(length);
        if (connection->initialMessage == NULL) {
            return 1;
        }
        memcpy(connection->initialMessage, message, length);
        connection->initialMessageLength = length;
    }
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    gti2SendClientChallenge(connection, challenge);
    connection->state = 0;
    return 0;
}

BOOL gti2AcceptConnection(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks) {
    if (connection->freeAtAcceptReject) {
        connection->freeAtAcceptReject = FALSE;
        return FALSE;
    }
    connection->freeAtAcceptReject = FALSE;
    if (connection->state != 4) {
        return FALSE;
    }
    gti2SendAccept(connection);
    connection->state = 5;
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    return TRUE;
}

void gti2RejectConnection(GTI2Connection *connection, const void *message, int length) {
    connection->freeAtAcceptReject = FALSE;
    if (connection->state == 4) {
        gti2MessageCheck(&message, &length);
        gti2SendReject(connection, message, length);
        connection->state = 6;
    }
}

BOOL gti2ConnectionSendData(GTI2Connection *connection, const void *data, int length) {
    if (!gti2SocketSend(connection->socket, connection->ip, connection->port, data, length)) {
        return FALSE;
    }
    connection->lastSendTime = current_time();
    return TRUE;
}

// A connection that is still connecting times out after its own timeout, or a minute on the side
// that listens
static BOOL gti2CheckTimeout(GTI2Connection *connection, u32 now) {
    BOOL timedOut;

    if (connection->state < 5) {
        timedOut = FALSE;
        if (connection->initiated) {
            if (connection->timeout != 0 && now - connection->startTime > connection->timeout) {
                timedOut = TRUE;
            }
        } else if (connection->state < 4 && now - connection->startTime > 60000) {
            timedOut = TRUE;
        }
        if (timedOut) {
            gti2SendClosed(connection);
            gti2ConnectionClosed(connection);
            if (!gti2ConnectedCallback(connection, 6, NULL, 0)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Resends the reliable messages the peer hasn't acked for a second
static BOOL gti2SendRetries(GTI2Connection *connection, u32 now) {
    int count = func_ov011_02171588(connection->outgoingMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *message = func_ov011_0217158c(connection->outgoingMessages, i);

        if (now - message->sendTime > 1000) {
            if (!gti2ResendMessage(connection, message)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Sends the ack that has waited for 100 milliseconds
static BOOL gti2CheckPendingAck(GTI2Connection *connection, u32 now) {
    if (!connection->pendingAck) {
        return TRUE;
    }
    if (now - connection->pendingAckTime > 100) {
        if (!gti2SendAck(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Sends a keep-alive after 30 seconds without sending
static BOOL gti2CheckKeepAlive(GTI2Connection *connection, u32 now) {
    if (now - connection->lastSendTime > 30000) {
        if (!gti2SendKeepAlive(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL gti2ConnectionThink(GTI2Connection *connection, u32 now) {
    if (!gti2CheckTimeout(connection, now)) {
        return FALSE;
    }
    if (!gti2CheckKeepAlive(connection, now)) {
        return FALSE;
    }
    if (!gti2SendRetries(connection, now)) {
        return FALSE;
    }
    if (gti2CheckPendingAck(connection, now)) {
        return TRUE;
    }
    return FALSE;
}

void gti2CloseConnection(GTI2Connection *connection, BOOL hard) {
    if (hard) {
        if (connection->state < 7) {
            gti2ConnectionClosed(connection);
            gti2SendClosed(connection);
            gti2ClosedCallback(connection, 0);
            gti2FreeSocketConnection(connection);
        }
    } else {
        connection->state = 6;
        gti2SendClose(connection);
    }
}

void gti2ConnectionClosed(GTI2Connection *connection) {
    if (connection->state != 7) {
        connection->state = 7;
        TableRemove(connection->socket->connections, &connection);
        func_ov011_021715a4(connection->socket->closedConnections, &connection);
    }
}

void gti2ConnectionCleanup(GTI2Connection *connection) {
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
    }
    if (connection->incoming.data != NULL) {
        gsifree(connection->incoming.data);
    }
    if (connection->outgoing.data != NULL) {
        gsifree(connection->outgoing.data);
    }
    if (connection->incomingMessages != NULL) {
        func_ov011_02171558(connection->incomingMessages);
    }
    if (connection->outgoingMessages != NULL) {
        func_ov011_02171558(connection->outgoingMessages);
    }
    if (connection->sendFilters != NULL) {
        func_ov011_02171558(connection->sendFilters);
    }
    if (connection->receiveFilters != NULL) {
        func_ov011_02171558(connection->receiveFilters);
    }
    gsifree(connection);
}
