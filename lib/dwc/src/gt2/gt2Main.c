#include "dwc/gt2Main.h"
#include "dwc/ext.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's API (gt2Main.c). The names of the functions that keep their addresses are in dwc/gt2Main.h

static void gti2CloseAllConnectionsHardMap(void *element, void *clientData);

int func_ov011_0217d144(GTI2Socket **socket, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                        GTI2SocketErrorCallback onSocketError) {
    return gti2CreateSocket(socket, localAddress, outgoingBufferSize, incomingBufferSize, onSocketError, 0);
}

void func_ov011_0217d158(GTI2Socket *socket) {
    gt2CloseAllConnectionsHard(socket);
    gti2CloseSocket(socket);
}

void func_ov011_0217d168(GTI2Socket *socket) {
    if (gti2ReceiveMessages(socket) && gti2SocketConnectionsThink(socket)) {
        gti2FreeClosedConnections(socket);
    }
}

void func_ov011_0217d188(GTI2Socket *socket, GTI2ConnectAttemptCallback callback) {
    gti2Listen(socket, callback);
}

BOOL func_ov011_0217d190(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks) {
    return gti2AcceptConnection(connection, callbacks);
}

void func_ov011_0217d198(GTI2Connection *connection, const void *message, int length) {
    gti2RejectConnection(connection, message, length);
}

int func_ov011_0217d1a0(GTI2Socket *socket, GTI2Connection **connection, const char *remoteAddress, const void *message,
                        int length, u32 timeout, const GTI2ConnectionCallbacks *callbacks, BOOL blocking) {
    GTI2Connection *created;
    u32 ip;
    u16 port;
    int result;
    BOOL done;

    if (!gt2StringToAddress(remoteAddress, &ip, &port) || ip == 0 || port == 0) {
        return 4;
    }
    // A multicast address
    if ((GTI2_SWAP32(ip) & 0xe0000000) == 0xe0000000) {
        return 4;
    }
    result = gti2NewOutgoingConnection(socket, &created, ip, port);
    if (result != 0) {
        return result;
    }
    created->timeout = timeout;
    result = gti2StartConnectionAttempt(created, message, length, callbacks);
    if (result != 0) {
        gti2FreeSocketConnection(created);
        return result;
    }
    if (!blocking) {
        if (connection != NULL) {
            *connection = created;
        }
        return 0;
    }
    created->callbackLevel++;
    do {
        func_ov011_0217d168(socket);
        done = created->state >= 5;
        if (!done) {
            msleep(1);
        }
    } while (!done);
    created->callbackLevel--;
    if (created->state == 5) {
        *connection = created;
    }
    return created->result;
}

int func_ov011_0217d290(GTI2Connection *connection, const void *message, int length, BOOL reliable) {
    if (connection->state != 5) {
        return 8;
    }
    gti2MessageCheck(&message, &length);
    // A reliable message of protocol 2 starts with its own length, copied out as it may be unaligned
    if (reliable && connection->socket->protocol == 2) {
        u16 size;
        u8 *dest = (u8 *)&size;
        const u8 *src = message;

        dest[0] = src[0];
        dest[1] = src[1];
        if (length != size + connection->socket->protocolOffset) {
            return 9;
        }
    }
    if (func_ov011_02171588(connection->sendFilters) != 0) {
        gti2SendFilterCallback(connection, 0, message, length, reliable);
        return 0;
    }
    if (gti2Send(connection, message, length, reliable)) {
        return 0;
    }
    return 10;
}

void gt2CloseConnectionHard(GTI2Connection *connection) {
    gti2CloseConnection(connection, TRUE);
}

static void gti2CloseAllConnectionsHardMap(void *element, void *clientData) {
    gt2CloseConnectionHard(*(GTI2Connection **)element);
}

void gt2CloseAllConnectionsHard(GTI2Socket *socket) {
    TableMapSafe(socket->connections, gti2CloseAllConnectionsHardMap, NULL);
}

int func_ov011_0217d364(GTI2Connection *connection) {
    if (connection->state < 5) {
        return 0;
    }
    if (connection->state == 5) {
        return 1;
    }
    if (connection->state == 6) {
        return 2;
    }
    return 3;
}

u32 func_ov011_0217d380(GTI2Connection *connection) {
    return connection->ip;
}

u16 func_ov011_0217d384(GTI2Connection *connection) {
    return connection->port;
}

u32 func_ov011_0217d388(GTI2Socket *socket) {
    return socket->ip;
}

u16 func_ov011_0217d38c(GTI2Socket *socket) {
    return socket->port;
}

int func_ov011_0217d390(GTI2Connection *connection) {
    return connection->outgoing.capacity;
}

int func_ov011_0217d394(GTI2Connection *connection) {
    return connection->outgoing.capacity - connection->outgoing.length;
}

int gt2GetSocketSOCKET(GTI2Socket *socket) {
    return socket->handle;
}

void func_ov011_0217d3a0(GTI2Socket *socket, GTI2UnrecognizedMessageCallback callback) {
    socket->onUnrecognizedMessage = callback;
}

void gt2SetConnectionData(GTI2Connection *connection, void *userData) {
    connection->userData = userData;
}

void *gt2GetConnectionData(GTI2Connection *connection) {
    return connection->userData;
}
