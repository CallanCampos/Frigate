#include "dwc/gt2Main.h"
#include "dwc/ext.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's API (gt2Main.c). The names of the functions that keep their addresses are in dwc/gt2Main.h

static void gti2CloseAllConnectionsHardMap(void *element, void *clientData);

GT2Result func_ov011_0217d144(GT2Socket *socket, const char *localAddress, int outgoingBufferSize,
                              int incomingBufferSize, gt2SocketErrorCallback onSocketError) {
    return gti2CreateSocket(socket, localAddress, outgoingBufferSize, incomingBufferSize, onSocketError,
                            GTI2UdpProtocol);
}

void func_ov011_0217d158(GT2Socket socket) {
    gt2CloseAllConnectionsHard(socket);
    gti2CloseSocket(socket);
}

void func_ov011_0217d168(GT2Socket socket) {
    if (gti2ReceiveMessages(socket) && gti2SocketConnectionsThink(socket)) {
        gti2FreeClosedConnections(socket);
    }
}

void func_ov011_0217d188(GT2Socket socket, gt2ConnectAttemptCallback callback) {
    gti2Listen(socket, callback);
}

GT2Bool func_ov011_0217d190(GT2Connection connection, GT2ConnectionCallbacks *callbacks) {
    return gti2AcceptConnection(connection, callbacks);
}

void func_ov011_0217d198(GT2Connection connection, const GT2Byte *message, int length) {
    gti2RejectConnection(connection, message, length);
}

GT2Result func_ov011_0217d1a0(GT2Socket socket, GT2Connection *connection, const char *remoteAddress,
                              const GT2Byte *message, int length, int timeout, GT2ConnectionCallbacks *callbacks,
                              GT2Bool blocking) {
    GT2Connection created;
    gsi_u32 ip;
    gsi_u16 port;
    int result;
    gsi_bool done;

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

GT2Result func_ov011_0217d290(GT2Connection connection, const GT2Byte *message, int length, GT2Bool reliable) {
    if (connection->state != 5) {
        return 8;
    }
    gti2MessageCheck(&message, &length);
    // A reliable message of protocol 2 starts with its own length, copied out as it may be unaligned
    if (reliable && connection->socket->protocol == GTI2VdpProtocol) {
        gsi_u16 size;
        gsi_u8 *dest = (gsi_u8 *)&size;
        const gsi_u8 *src = message;

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

void gt2CloseConnectionHard(GT2Connection connection) {
    gti2CloseConnection(connection, gsi_true);
}

static void gti2CloseAllConnectionsHardMap(void *element, void *clientData) {
    gt2CloseConnectionHard(*(GT2Connection *)element);
}

void gt2CloseAllConnectionsHard(GT2Socket socket) {
    TableMapSafe(socket->connections, gti2CloseAllConnectionsHardMap, NULL);
}

GT2ConnectionState func_ov011_0217d364(GT2Connection connection) {
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

unsigned int func_ov011_0217d380(GT2Connection connection) {
    return connection->ip;
}

unsigned short func_ov011_0217d384(GT2Connection connection) {
    return connection->port;
}

unsigned int func_ov011_0217d388(GT2Socket socket) {
    return socket->ip;
}

unsigned short func_ov011_0217d38c(GT2Socket socket) {
    return socket->port;
}

int func_ov011_0217d390(GT2Connection connection) {
    return connection->outgoing.capacity;
}

int func_ov011_0217d394(GT2Connection connection) {
    return connection->outgoing.capacity - connection->outgoing.length;
}

SOCKET gt2GetSocketSOCKET(GT2Socket socket) {
    return socket->handle;
}

void func_ov011_0217d3a0(GT2Socket socket, gt2UnrecognizedMessageCallback callback) {
    socket->onUnrecognizedMessage = callback;
}

void gt2SetConnectionData(GT2Connection connection, void *userData) {
    connection->userData = userData;
}

void *gt2GetConnectionData(GT2Connection connection) {
    return connection->userData;
}
