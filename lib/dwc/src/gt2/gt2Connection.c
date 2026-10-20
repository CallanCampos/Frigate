#include "dwc/gt2Connection.h"
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's connections (gt2Connection.c). The functions' names, from pret/pokerevo's Wii build, are in
// dwc/gt2Connection.h

static BOOL func_ov011_0217cf44(GTI2Connection *connection, u32 now);
static BOOL func_ov011_0217cfa4(GTI2Connection *connection, u32 now);
static BOOL func_ov011_0217cfe8(GTI2Connection *connection, u32 now);
static BOOL func_ov011_0217d014(GTI2Connection *connection, u32 now);

int func_ov011_0217ce00(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port) {
    int result = func_ov011_0217e984(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = 0;
    (*connection)->initiated = TRUE;
    return 0;
}

int func_ov011_0217ce1c(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port) {
    int result = func_ov011_0217e984(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = 2;
    (*connection)->initiated = FALSE;
    return 0;
}

int func_ov011_0217ce38(GTI2Connection *connection, const void *message, int length,
                        const GTI2ConnectionCallbacks *callbacks) {
    u8 challenge[0x20];

    func_ov011_0217ee60(&message, &length);
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
    func_ov011_0217e2c0(connection, challenge);
    connection->state = 0;
    return 0;
}

BOOL func_ov011_0217ceac(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks) {
    if (connection->freeAtAcceptReject) {
        connection->freeAtAcceptReject = FALSE;
        return FALSE;
    }
    connection->freeAtAcceptReject = FALSE;
    if (connection->state != 4) {
        return FALSE;
    }
    func_ov011_0217e3cc(connection);
    connection->state = 5;
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    return TRUE;
}

void func_ov011_0217cee8(GTI2Connection *connection, const void *message, int length) {
    connection->freeAtAcceptReject = FALSE;
    if (connection->state == 4) {
        func_ov011_0217ee60(&message, &length);
        func_ov011_0217e40c(connection, message, length);
        connection->state = 6;
    }
}

BOOL func_ov011_0217cf18(GTI2Connection *connection, const void *data, int length) {
    if (!func_ov011_0217eb34(connection->socket, connection->ip, connection->port, data, length)) {
        return FALSE;
    }
    connection->lastSendTime = current_time();
    return TRUE;
}

// gti2CheckTimeout: a connection that is still connecting times out after its own timeout, or a minute on the side
// that listens
static BOOL func_ov011_0217cf44(GTI2Connection *connection, u32 now) {
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
            func_ov011_0217e694(connection);
            func_ov011_0217d0b4(connection);
            if (!func_ov011_0217cab0(connection, 6, NULL, 0)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Resends the reliable messages the peer hasn't acked for a second
static BOOL func_ov011_0217cfa4(GTI2Connection *connection, u32 now) {
    int count = func_ov011_02171588(connection->outgoingMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *message = func_ov011_0217158c(connection->outgoingMessages, i);

        if (now - message->sendTime > 1000) {
            if (!func_ov011_0217e6f8(connection, message)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Sends the ack that has waited for 100 milliseconds
static BOOL func_ov011_0217cfe8(GTI2Connection *connection, u32 now) {
    if (!connection->pendingAck) {
        return TRUE;
    }
    if (now - connection->pendingAckTime > 100) {
        if (!func_ov011_0217e598(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Sends a keep-alive after 30 seconds without sending
static BOOL func_ov011_0217d014(GTI2Connection *connection, u32 now) {
    if (now - connection->lastSendTime > 30000) {
        if (!func_ov011_0217e4a0(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL func_ov011_0217d038(GTI2Connection *connection, u32 now) {
    if (!func_ov011_0217cf44(connection, now)) {
        return FALSE;
    }
    if (!func_ov011_0217d014(connection, now)) {
        return FALSE;
    }
    if (!func_ov011_0217cfa4(connection, now)) {
        return FALSE;
    }
    if (func_ov011_0217cfe8(connection, now)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov011_0217d080(GTI2Connection *connection, BOOL hard) {
    if (hard) {
        if (connection->state < 7) {
            func_ov011_0217d0b4(connection);
            func_ov011_0217e694(connection);
            func_ov011_0217cb6c(connection, 0);
            func_ov011_0217eac8(connection);
        }
    } else {
        connection->state = 6;
        func_ov011_0217e460(connection);
    }
}

void func_ov011_0217d0b4(GTI2Connection *connection) {
    if (connection->state != 7) {
        connection->state = 7;
        func_ov011_02171920(connection->socket->connections, &connection);
        func_ov011_021715a4(connection->socket->closedConnections, &connection);
    }
}

void func_ov011_0217d0e8(GTI2Connection *connection) {
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
