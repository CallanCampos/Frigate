#include <string.h>
// Before dwc/ext.h: MWCC builds the bound address's 8-byte memset in place, as the ROM does, only when
// <string.h> declares them first
#include "dwc/ext.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's sockets (gt2Socket.c). The functions' names, from pret/pokerevo's Wii build, are in dwc/gt2Socket.h
// and beside the static ones

static int func_ov011_0217e768(const void *element, int bucketCount);
static int func_ov011_0217e77c(const void *a, const void *b);
static void func_ov011_0217e798(void *element);
static GTI2Connection *func_ov011_0217e978(void);
static int func_ov011_0217ec68(void *element, void *clientData);

// gti2ConnectionHash and gti2ConnectionCompare: the hash table of a socket's connections holds GTI2Connection
// pointers, by address
static int func_ov011_0217e768(const void *element, int bucketCount) {
    const GTI2Connection *connection = *(GTI2Connection **)element;

    return (connection->ip * connection->port) % bucketCount;
}

static int func_ov011_0217e77c(const void *a, const void *b) {
    const GTI2Connection *first = *(GTI2Connection **)a;
    const GTI2Connection *second = *(GTI2Connection **)b;

    if (first->ip != second->ip) {
        return first->ip - second->ip;
    }
    return (s16)(first->port - second->port);
}

// gti2ConnectionFree, the free function of the closed connections
static void func_ov011_0217e798(void *element) {
    func_ov011_0217d0e8(*(GTI2Connection **)element);
}

GTI2Connection *func_ov011_0217e7a4(GTI2Socket *socket, u32 ip, u16 port) {
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

int func_ov011_0217e7cc(GTI2Socket **socketOut, const char *localAddress, int outgoingBufferSize,
                        int incomingBufferSize, GTI2SocketErrorCallback onSocketError, int protocol) {
    u16 port;
    SocAddrIn address;
    GTI2Socket *socket;
    u32 ip;
    int addressSize;

    SocketStartUp();
    if (incomingBufferSize == 0) {
        incomingBufferSize = 0x10000;
    }
    if (outgoingBufferSize == 0) {
        outgoingBufferSize = 0x10000;
    }
    if (!func_ov011_0217ed7c(localAddress, &ip, &port)) {
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
    socket->connections = TableNew2(sizeof(GTI2Connection *), 0x20, 2, func_ov011_0217e768, func_ov011_0217e77c, NULL);
    if (socket->connections == NULL) {
        gsifree(socket);
        return 1;
    }
    socket->closedConnections = func_ov011_02171514(sizeof(GTI2Connection *), 4, func_ov011_0217e798);
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
    address.addr = ip;
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
    socket->ip = address.addr;
    socket->port = GTI2_SWAP16(address.port);
    *socketOut = socket;
    return 0;
}

void func_ov011_0217e944(GTI2Socket *socket) {
    if (socket->callbackLevel != 0) {
        socket->close = TRUE;
        return;
    }
    closesocket(socket->handle);
    TableFree(socket->connections);
    func_ov011_02171558(socket->closedConnections);
    gsifree(socket);
    SocketShutDown();
}

void func_ov011_0217e974(GTI2Socket *socket, GTI2ConnectAttemptCallback callback) {
    socket->onConnectAttempt = callback;
}

// Allocates a connection
static GTI2Connection *func_ov011_0217e978(void) {
    return gsimalloc(sizeof(GTI2Connection));
}

int func_ov011_0217e984(GTI2Socket *socket, GTI2Connection **connectionOut, u32 ip, u16 port) {
    GTI2Connection *connection = NULL;

    if (func_ov011_0217e7a4(socket, ip, port) != NULL) {
        return 5;
    }
    connection = func_ov011_0217e978();
    if (connection != NULL) {
        memset(connection, 0, sizeof(GTI2Connection));
        connection->ip = ip;
        connection->port = port;
        connection->socket = socket;
        connection->startTime = current_time();
        connection->lastSendTime = connection->startTime;
        connection->serialNumber = 0;
        connection->expectedSerialNumber = 0;
        if (!func_ov011_0217c944(&connection->incoming, socket->incomingBufferSize)) {
            goto fail;
        }
        if (!func_ov011_0217c944(&connection->outgoing, socket->outgoingBufferSize)) {
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
        *connectionOut = func_ov011_0217e7a4(socket, ip, port);
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

void func_ov011_0217eac8(GTI2Connection *connection) {
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

BOOL func_ov011_0217eb34(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length) {
    int result;
    int error;

    func_ov011_0217ee60(&message, &length);
    if (socket->protocol != 3 && !CanSendOnSocket(socket->handle)) {
        return TRUE;
    }
    SocAddrIn address = { 0 };

    address.family = 2;
    address.addr = ip;
    address.port = GTI2_SWAP16(port);
    result = sendto(socket->handle, message, length, 0, &address, sizeof(address));
    if (result == -1) {
        error = GOAGetLastError(socket->handle);
        if (error == -15) {
            if (!func_ov011_0217df34(socket, ip, port)) {
                return FALSE;
            }
        } else if (error == -23) {
            if (!func_ov011_0217dfc4(socket, ip, port, TRUE)) {
                return FALSE;
            }
        } else if (error == -42 || error == -6) {
            return TRUE;
        } else if (error != -35) {
            func_ov011_0217ecf0(socket);
            return FALSE;
        }
    } else if (socket->onSendDump != NULL) {
        if (!func_ov011_0217cd0c(socket, func_ov011_0217e7a4(socket, ip, port), ip, port, FALSE, message, length,
                                 TRUE)) {
            return FALSE;
        }
    }
    return TRUE;
}

// gti2SocketConnectionsThinkMap: runs a connection's timers and frees it once it has closed; returns FALSE to stop
// when a callback closed the socket
static int func_ov011_0217ec68(void *element, void *clientData) {
    GTI2Connection *connection = *(GTI2Connection **)element;
    u32 now = *(u32 *)clientData;

    if (connection->state != 7 && !func_ov011_0217d038(connection, now)) {
        return FALSE;
    }
    if (connection->state == 7 && !connection->freeAtAcceptReject && connection->callbackLevel == 0) {
        func_ov011_0217eac8(connection);
    }
    return TRUE;
}

BOOL func_ov011_0217eca0(GTI2Socket *socket) {
    u32 now = current_time();

    if (TableMapSafe2(socket->connections, func_ov011_0217ec68, &now)) {
        return FALSE;
    }
    return TRUE;
}

void func_ov011_0217eccc(GTI2Socket *socket) {
    int i;

    for (i = func_ov011_02171588(socket->closedConnections) - 1; i >= 0; i--) {
        func_ov011_0217eac8(*(GTI2Connection **)func_ov011_0217158c(socket->closedConnections, i));
    }
}

void func_ov011_0217ecf0(GTI2Socket *socket) {
    if (!socket->error) {
        socket->error = TRUE;
        func_ov011_0217d350(socket);
        if (func_ov011_0217c9f4(socket)) {
            func_ov011_0217e944(socket);
        }
    }
}
