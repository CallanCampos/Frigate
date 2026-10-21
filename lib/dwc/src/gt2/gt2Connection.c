#include "dwc/gt2Connection.h"
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's connections (gt2Connection.c)

static gsi_bool gti2CheckTimeout(GTI2Connection *connection, gsi_u32 now);
static gsi_bool gti2SendRetries(GTI2Connection *connection, gsi_u32 now);
static gsi_bool gti2CheckPendingAck(GTI2Connection *connection, gsi_u32 now);
static gsi_bool gti2CheckKeepAlive(GTI2Connection *connection, gsi_u32 now);

int gti2NewOutgoingConnection(GTI2Socket *socket, GTI2Connection **connection, gsi_u32 ip, gsi_u16 port) {
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = 0;
    (*connection)->initiated = gsi_true;
    return 0;
}

int gti2NewIncomingConnection(GTI2Socket *socket, GTI2Connection **connection, gsi_u32 ip, gsi_u16 port) {
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = 2;
    (*connection)->initiated = gsi_false;
    return 0;
}

int gti2StartConnectionAttempt(GTI2Connection *connection, const void *message, int length,
                               const GTI2ConnectionCallbacks *callbacks) {
    gsi_u8 challenge[0x20];

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

gsi_bool gti2AcceptConnection(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks) {
    if (connection->freeAtAcceptReject) {
        connection->freeAtAcceptReject = gsi_false;
        return gsi_false;
    }
    connection->freeAtAcceptReject = gsi_false;
    if (connection->state != 4) {
        return gsi_false;
    }
    gti2SendAccept(connection);
    connection->state = 5;
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    return gsi_true;
}

void gti2RejectConnection(GTI2Connection *connection, const void *message, int length) {
    connection->freeAtAcceptReject = gsi_false;
    if (connection->state == 4) {
        gti2MessageCheck(&message, &length);
        gti2SendReject(connection, message, length);
        connection->state = 6;
    }
}

gsi_bool gti2ConnectionSendData(GTI2Connection *connection, const void *data, int length) {
    if (!gti2SocketSend(connection->socket, connection->ip, connection->port, data, length)) {
        return gsi_false;
    }
    connection->lastSendTime = current_time();
    return gsi_true;
}

// A connection that is still connecting times out after its own timeout, or a minute on the side
// that listens
static gsi_bool gti2CheckTimeout(GTI2Connection *connection, gsi_u32 now) {
    gsi_bool timedOut;

    if (connection->state < 5) {
        timedOut = gsi_false;
        if (connection->initiated) {
            if (connection->timeout != 0 && now - connection->startTime > connection->timeout) {
                timedOut = gsi_true;
            }
        } else if (connection->state < 4 && now - connection->startTime > 60000) {
            timedOut = gsi_true;
        }
        if (timedOut) {
            gti2SendClosed(connection);
            gti2ConnectionClosed(connection);
            if (!gti2ConnectedCallback(connection, 6, NULL, 0)) {
                return gsi_false;
            }
        }
    }
    return gsi_true;
}

// Resends the reliable messages the peer hasn't acked for a second
static gsi_bool gti2SendRetries(GTI2Connection *connection, gsi_u32 now) {
    int count = func_ov011_02171588(connection->outgoingMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *message = func_ov011_0217158c(connection->outgoingMessages, i);

        if (now - message->sendTime > 1000) {
            if (!gti2ResendMessage(connection, message)) {
                return gsi_false;
            }
        }
    }
    return gsi_true;
}

// Sends the ack that has waited for 100 milliseconds
static gsi_bool gti2CheckPendingAck(GTI2Connection *connection, gsi_u32 now) {
    if (!connection->pendingAck) {
        return gsi_true;
    }
    if (now - connection->pendingAckTime > 100) {
        if (!gti2SendAck(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// Sends a keep-alive after 30 seconds without sending
static gsi_bool gti2CheckKeepAlive(GTI2Connection *connection, gsi_u32 now) {
    if (now - connection->lastSendTime > 30000) {
        if (!gti2SendKeepAlive(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

gsi_bool gti2ConnectionThink(GTI2Connection *connection, gsi_u32 now) {
    if (!gti2CheckTimeout(connection, now)) {
        return gsi_false;
    }
    if (!gti2CheckKeepAlive(connection, now)) {
        return gsi_false;
    }
    if (!gti2SendRetries(connection, now)) {
        return gsi_false;
    }
    if (gti2CheckPendingAck(connection, now)) {
        return gsi_true;
    }
    return gsi_false;
}

void gti2CloseConnection(GTI2Connection *connection, gsi_bool hard) {
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
