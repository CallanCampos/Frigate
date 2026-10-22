#include <string.h>
// Before dwc/ext.h: MWCC builds the bound address's 8-byte memset in place, as the ROM does, only when
// <string.h> declares them first
#include "dwc/ext.h"
#include "dwc/gt2Buffer.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's sockets (gt2Socket.c)

static int gti2ConnectionHash(const void *element, int bucketCount);
static int gti2ConnectionCompare(const void *a, const void *b);
static void gti2ConnectionFree(void *element);
static GT2Connection gti2CreateConnectionObject(void);
static int gti2SocketConnectionsThinkMap(void *element, void *clientData);

// The hash table of a socket's connections holds GTI2Connection
// pointers, by address
static int gti2ConnectionHash(const void *element, int bucketCount) {
    GT2Connection connection = *(GT2Connection *)element;

    return (connection->ip * connection->port) % bucketCount;
}

static int gti2ConnectionCompare(const void *a, const void *b) {
    GT2Connection first = *(GT2Connection *)a;
    GT2Connection second = *(GT2Connection *)b;

    if (first->ip != second->ip) {
        return first->ip - second->ip;
    }
    return (gsi_i16)(first->port - second->port);
}

// The free function of the closed connections
static void gti2ConnectionFree(void *element) {
    gti2ConnectionCleanup(*(GT2Connection *)element);
}

GT2Connection gti2SocketFindConnection(GT2Socket socket, unsigned int ip, unsigned short port) {
    GTI2Connection key;
    GT2Connection keyPointer;
    GT2Connection *found;

    key.ip = ip;
    key.port = port;
    keyPointer = &key;
    found = TableLookup(socket->connections, &keyPointer);
    if (found != NULL) {
        return *found;
    }
    return NULL;
}

GT2Result gti2CreateSocket(GT2Socket *socketOut, const char *localAddress, int outgoingBufferSize,
                           int incomingBufferSize, gt2SocketErrorCallback onSocketError, GTI2ProtocolType protocol) {
    gsi_u16 port;
    SOCSockAddrIn address;
    GTI2Socket *socketTemp;
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
    socketTemp = gsimalloc(sizeof(GTI2Socket));
    if (socketTemp == NULL) {
        return 1;
    }
    memset(socketTemp, 0, sizeof(GTI2Socket));
    socketTemp->socket = -1;
    socketTemp->incomingBufferSize = incomingBufferSize;
    socketTemp->outgoingBufferSize = outgoingBufferSize;
    socketTemp->socketErrorCallback = onSocketError;
    socketTemp->connections =
        TableNew2(sizeof(GT2Connection), 0x20, 2, gti2ConnectionHash, gti2ConnectionCompare, NULL);
    if (socketTemp->connections == NULL) {
        gsifree(socketTemp);
        return 1;
    }
    socketTemp->closedConnections = ArrayNew(sizeof(GT2Connection), 4, gti2ConnectionFree);
    if (socketTemp->closedConnections == NULL) {
        TableFree(socketTemp->connections);
        gsifree(socketTemp);
        return 1;
    }
    socketTemp->socket = socket(2, 2, 0);
    socketTemp->protocolType = protocol;
    if (protocol == GTI2AdHocProtocol) {
        socketTemp->protocolOffset = 0;
    } else {
        socketTemp->protocolOffset = protocol;
    }
    if (socketTemp->socket == -1) {
        TableFree(socketTemp->connections);
        ArrayFree(socketTemp->closedConnections);
        gsifree(socketTemp);
        return 3;
    }
    memset(&address, 0, sizeof(address));
    address.family = 2;
    address.addr.addr = ip;
    address.port = GTI2_SWAP16(port);
    if (protocol != GTI2AdHocProtocol) {
        if (bind(socketTemp->socket, (SOCKADDR *)&address, sizeof(address)) == -1) {
            closesocket(socketTemp->socket);
            TableFree(socketTemp->connections);
            ArrayFree(socketTemp->closedConnections);
            gsifree(socketTemp);
            return 3;
        }
    }
    addressSize = sizeof(address);
    getsockname(socketTemp->socket, (SOCKADDR *)&address, &addressSize);
    socketTemp->ip = address.addr.addr;
    socketTemp->port = GTI2_SWAP16(address.port);
    *socketOut = socketTemp;
    return 0;
}

void gti2CloseSocket(GT2Socket socket) {
    if (socket->callbackLevel != 0) {
        socket->close = gsi_true;
        return;
    }
    closesocket(socket->socket);
    TableFree(socket->connections);
    ArrayFree(socket->closedConnections);
    gsifree(socket);
    SocketShutDown();
}

void gti2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback) {
    socket->connectAttemptCallback = callback;
}

// Allocates a connection
static GT2Connection gti2CreateConnectionObject(void) {
    return gsimalloc(sizeof(GTI2Connection));
}

GT2Result gti2NewSocketConnection(GT2Socket socket, GT2Connection *connectionOut, unsigned int ip,
                                  unsigned short port) {
    GT2Connection connection = NULL;

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
        connection->lastSend = connection->startTime;
        connection->serialNumber = 0;
        connection->expectedSerialNumber = 0;
        if (!gti2AllocateBuffer(&connection->incomingBuffer, socket->incomingBufferSize)) {
            goto fail;
        }
        if (!gti2AllocateBuffer(&connection->outgoingBuffer, socket->outgoingBufferSize)) {
            goto fail;
        }
        connection->incomingBufferMessages = ArrayNew(sizeof(GTI2IncomingBufferMessage), 0x40, NULL);
        if (connection->incomingBufferMessages == NULL) {
            goto fail;
        }
        connection->outgoingBufferMessages = ArrayNew(sizeof(GTI2OutgoingBufferMessage), 0x40, NULL);
        if (connection->outgoingBufferMessages == NULL) {
            goto fail;
        }
        connection->sendFilters = ArrayNew(sizeof(gt2SendFilterCallback), 2, NULL);
        if (connection->sendFilters == NULL) {
            goto fail;
        }
        connection->receiveFilters = ArrayNew(sizeof(gt2ReceiveFilterCallback), 2, NULL);
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
        gsifree(connection->incomingBuffer.buffer);
        gsifree(connection->outgoingBuffer.buffer);
        if (connection->incomingBufferMessages != NULL) {
            ArrayFree(connection->incomingBufferMessages);
        }
        if (connection->outgoingBufferMessages != NULL) {
            ArrayFree(connection->outgoingBufferMessages);
        }
        if (connection->sendFilters != NULL) {
            ArrayFree(connection->sendFilters);
        }
        if (connection->receiveFilters != NULL) {
            ArrayFree(connection->receiveFilters);
        }
        gsifree(connection);
    }
    return 1;
}

void gti2FreeSocketConnection(GT2Connection connection) {
    int count;
    int i;

    if (connection->freeAtAcceptReject || connection->callbackLevel != 0) {
        return;
    }
    if (connection->state == 7) {
        count = ArrayLength(connection->socket->closedConnections);
        for (i = 0; i < count; i++) {
            if (connection == *(GT2Connection *)ArrayNth(connection->socket->closedConnections, i)) {
                ArrayDeleteAt(connection->socket->closedConnections, i);
                return;
            }
        }
    } else {
        TableRemove(connection->socket->connections, &connection);
    }
}

GT2Bool gti2SocketSend(GT2Socket socket, unsigned int ip, unsigned short port, const GT2Byte *message, int length) {
    int result;
    int error;

    gti2MessageCheck(&message, &length);
    if (socket->protocolType != GTI2AdHocProtocol && !CanSendOnSocket(socket->socket)) {
        return gsi_true;
    }
    SOCSockAddrIn address = { 0 };

    address.family = 2;
    address.addr.addr = ip;
    address.port = GTI2_SWAP16(port);
    result = sendto(socket->socket, message, length, 0, (SOCKADDR *)&address, sizeof(address));
    if (result == -1) {
        error = GOAGetLastError(socket->socket);
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
    } else if (socket->sendDumpCallback != NULL) {
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
    GT2Connection connection = *(GT2Connection *)element;
    gsi_u32 now = *(gsi_u32 *)clientData;

    if (connection->state != 7 && !gti2ConnectionThink(connection, now)) {
        return gsi_false;
    }
    if (connection->state == 7 && !connection->freeAtAcceptReject && connection->callbackLevel == 0) {
        gti2FreeSocketConnection(connection);
    }
    return gsi_true;
}

GT2Bool gti2SocketConnectionsThink(GT2Socket socket) {
    gsi_u32 now = current_time();

    if (TableMapSafe2(socket->connections, gti2SocketConnectionsThinkMap, &now)) {
        return gsi_false;
    }
    return gsi_true;
}

void gti2FreeClosedConnections(GT2Socket socket) {
    int i;

    for (i = ArrayLength(socket->closedConnections) - 1; i >= 0; i--) {
        gti2FreeSocketConnection(*(GT2Connection *)ArrayNth(socket->closedConnections, i));
    }
}

void gti2SocketError(GT2Socket socket) {
    if (!socket->error) {
        socket->error = gsi_true;
        gt2CloseAllConnectionsHard(socket);
        if (gti2SocketErrorCallback(socket)) {
            gti2CloseSocket(socket);
        }
    }
}
