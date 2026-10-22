#include <string.h>
// Before dwc/ext.h: MWCC builds the bound address's 8-byte memset in place, as the ROM does, only when
// <string.h> declares them first
#include "dwc/ext.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's sockets (gt2Socket.c)

static int gti2ConnectionHash(const void *element, int bucketCount);
static int gti2ConnectionCompare(const void *a, const void *b);
static void gti2ConnectionFree(void *element);
static GTI2Connection *gti2CreateConnectionObject(void);
static int gti2SocketConnectionsThinkMap(void *element, void *clientData);

// The hash table of a socket's connections holds GTI2Connection
// pointers, by address
static int gti2ConnectionHash(const void *element, int bucketCount) {
    const GTI2Connection *connection = *(GTI2Connection **)element;

    return (connection->ip * connection->port) % bucketCount;
}

static int gti2ConnectionCompare(const void *a, const void *b) {
    const GTI2Connection *first = *(GTI2Connection **)a;
    const GTI2Connection *second = *(GTI2Connection **)b;

    if (first->ip != second->ip) {
        return first->ip - second->ip;
    }
    return (gsi_i16)(first->port - second->port);
}

// The free function of the closed connections
static void gti2ConnectionFree(void *element) {
    gti2ConnectionCleanup(*(GTI2Connection **)element);
}

GTI2Connection *gti2SocketFindConnection(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port) {
    GTI2Connection key;
    GTI2Connection *keyPointer;
    GTI2Connection **found;

    key.ip = ip;
    key.port = port;
    keyPointer = &key;
    found = TableLookup(socket->connections, &keyPointer);
    if (found != NULL) {
        return *found;
    }
    return NULL;
}

int gti2CreateSocket(GTI2Socket **socketOut, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                     GTI2SocketErrorCallback onSocketError, int protocol) {
    gsi_u16 port;
    SocAddrIn address;
    GTI2Socket *socket;
    gsi_u32 ip;
    int addressSize;

    SocketStartUp();
    if (incomingBufferSize == 0) {
        incomingBufferSize = 0x10000;
    }
    if (outgoingBufferSize == 0) {
        outgoingBufferSize = 0x10000;
    }
    if (!gt2StringToAddress(localAddress, &ip, &port)) {
        return 4;
    }
    socket = gsimalloc(sizeof(GTI2Socket));
    if (socket == NULL) {
        return 1;
    }
    memset(socket, 0, sizeof(GTI2Socket));
    socket->handle = -1;
    socket->incomingBufferSize = incomingBufferSize;
    socket->outgoingBufferSize = outgoingBufferSize;
    socket->onSocketError = onSocketError;
    socket->connections = TableNew2(sizeof(GTI2Connection *), 0x20, 2, gti2ConnectionHash, gti2ConnectionCompare, NULL);
    if (socket->connections == NULL) {
        gsifree(socket);
        return 1;
    }
    socket->closedConnections = func_ov011_02171514(sizeof(GTI2Connection *), 4, gti2ConnectionFree);
    if (socket->closedConnections == NULL) {
        TableFree(socket->connections);
        gsifree(socket);
        return 1;
    }
    socket->handle = gsiSocket(2, 2, 0);
    socket->protocol = protocol;
    if (protocol == 3) {
        socket->protocolOffset = 0;
    } else {
        socket->protocolOffset = protocol;
    }
    if (socket->handle == -1) {
        TableFree(socket->connections);
        func_ov011_02171558(socket->closedConnections);
        gsifree(socket);
        return 3;
    }
    memset(&address, 0, sizeof(address));
    address.family = 2;
    address.addr.addr = ip;
    address.port = GTI2_SWAP16(port);
    if (protocol != 3) {
        if (bind(socket->handle, &address, sizeof(address)) == -1) {
            closesocket(socket->handle);
            TableFree(socket->connections);
            func_ov011_02171558(socket->closedConnections);
            gsifree(socket);
            return 3;
        }
    }
    addressSize = sizeof(address);
    getsockname(socket->handle, &address, &addressSize);
    socket->ip = address.addr.addr;
    socket->port = GTI2_SWAP16(address.port);
    *socketOut = socket;
    return 0;
}

void gti2CloseSocket(GTI2Socket *socket) {
    if (socket->callbackLevel != 0) {
        socket->close = gsi_true;
        return;
    }
    closesocket(socket->handle);
    TableFree(socket->connections);
    func_ov011_02171558(socket->closedConnections);
    gsifree(socket);
    SocketShutDown();
}

void gti2Listen(GTI2Socket *socket, GTI2ConnectAttemptCallback callback) {
    socket->onConnectAttempt = callback;
}

// Allocates a connection
static GTI2Connection *gti2CreateConnectionObject(void) {
    return gsimalloc(sizeof(GTI2Connection));
}

int gti2NewSocketConnection(GTI2Socket *socket, GTI2Connection **connectionOut, gsi_u32 ip, gsi_u16 port) {
    GTI2Connection *connection = NULL;

    if (gti2SocketFindConnection(socket, ip, port) != NULL) {
        return 5;
    }
    connection = gti2CreateConnectionObject();
    if (connection != NULL) {
        memset(connection, 0, sizeof(GTI2Connection));
        connection->ip = ip;
        connection->port = port;
        connection->socket = socket;
        connection->startTime = current_time();
        connection->lastSendTime = connection->startTime;
        connection->serialNumber = 0;
        connection->expectedSerialNumber = 0;
        if (!gti2AllocateBuffer(&connection->incoming, socket->incomingBufferSize)) {
            goto fail;
        }
        if (!gti2AllocateBuffer(&connection->outgoing, socket->outgoingBufferSize)) {
            goto fail;
        }
        connection->incomingMessages = func_ov011_02171514(sizeof(GTI2IncomingMessage), 0x40, NULL);
        if (connection->incomingMessages == NULL) {
            goto fail;
        }
        connection->outgoingMessages = func_ov011_02171514(sizeof(GTI2OutgoingMessage), 0x40, NULL);
        if (connection->outgoingMessages == NULL) {
            goto fail;
        }
        connection->sendFilters = func_ov011_02171514(sizeof(GTI2FilterCallback), 2, NULL);
        if (connection->sendFilters == NULL) {
            goto fail;
        }
        connection->receiveFilters = func_ov011_02171514(sizeof(GTI2FilterCallback), 2, NULL);
        if (connection->receiveFilters == NULL) {
            goto fail;
        }
        TableEnter(socket->connections, &connection);
        *connectionOut = gti2SocketFindConnection(socket, ip, port);
        if (*connectionOut != NULL) {
            return 0;
        }
    }
fail:
    if (connection != NULL) {
        gsifree(connection->incoming.data);
        gsifree(connection->outgoing.data);
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
    return 1;
}

void gti2FreeSocketConnection(GTI2Connection *connection) {
    int count;
    int i;

    if (connection->freeAtAcceptReject || connection->callbackLevel != 0) {
        return;
    }
    if (connection->state == 7) {
        count = func_ov011_02171588(connection->socket->closedConnections);
        for (i = 0; i < count; i++) {
            if (connection == *(GTI2Connection **)func_ov011_0217158c(connection->socket->closedConnections, i)) {
                func_ov011_02171668(connection->socket->closedConnections, i);
                return;
            }
        }
    } else {
        TableRemove(connection->socket->connections, &connection);
    }
}

gsi_bool gti2SocketSend(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port, const void *message, int length) {
    int result;
    int error;

    gti2MessageCheck(&message, &length);
    if (socket->protocol != 3 && !CanSendOnSocket(socket->handle)) {
        return gsi_true;
    }
    SocAddrIn address = { 0 };

    address.family = 2;
    address.addr.addr = ip;
    address.port = GTI2_SWAP16(port);
    result = sendto(socket->handle, message, length, 0, &address, sizeof(address));
    if (result == -1) {
        error = GOAGetLastError(socket->handle);
        if (error == -15) {
            if (!gti2HandleConnectionReset(socket, ip, port)) {
                return gsi_false;
            }
        } else if (error == -23) {
            if (!gti2HandleHostUnreachable(socket, ip, port, gsi_true)) {
                return gsi_false;
            }
        } else if (error == -42 || error == -6) {
            return gsi_true;
        } else if (error != -35) {
            gti2SocketError(socket);
            return gsi_false;
        }
    } else if (socket->onSendDump != NULL) {
        if (!gti2DumpCallback(socket, gti2SocketFindConnection(socket, ip, port), ip, port, gsi_false, message, length,
                              gsi_true)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// Runs a connection's timers and frees it once it has closed; returns gsi_false to stop
// when a callback closed the socket
static int gti2SocketConnectionsThinkMap(void *element, void *clientData) {
    GTI2Connection *connection = *(GTI2Connection **)element;
    gsi_u32 now = *(gsi_u32 *)clientData;

    if (connection->state != 7 && !gti2ConnectionThink(connection, now)) {
        return gsi_false;
    }
    if (connection->state == 7 && !connection->freeAtAcceptReject && connection->callbackLevel == 0) {
        gti2FreeSocketConnection(connection);
    }
    return gsi_true;
}

gsi_bool gti2SocketConnectionsThink(GTI2Socket *socket) {
    gsi_u32 now = current_time();

    if (TableMapSafe2(socket->connections, gti2SocketConnectionsThinkMap, &now)) {
        return gsi_false;
    }
    return gsi_true;
}

void gti2FreeClosedConnections(GTI2Socket *socket) {
    int i;

    for (i = func_ov011_02171588(socket->closedConnections) - 1; i >= 0; i--) {
        gti2FreeSocketConnection(*(GTI2Connection **)func_ov011_0217158c(socket->closedConnections, i));
    }
}

void gti2SocketError(GTI2Socket *socket) {
    if (!socket->error) {
        socket->error = gsi_true;
        gt2CloseAllConnectionsHard(socket);
        if (gti2SocketErrorCallback(socket)) {
            gti2CloseSocket(socket);
        }
    }
}
