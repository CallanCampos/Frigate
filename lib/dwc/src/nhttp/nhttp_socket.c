#include "nhttp_internal.h"

// NHTTP_socket: the sockets that connect to the server (overlay 11's socket library), with the buffered reads

// The bytes that seed the random numbers of a secure connection
static u32 sEntropy[8];

static u32 NHttpSocSslFilterDropBit15(u32 flags);
static u32 NHttpSocSslFilterNone(u32 flags);
static void NHttpSocSeedRandom(void);

static u32 NHttpSocSslFilterDropBit15(u32 flags) {
    if (flags & 0x8000) {
        flags &= ~0x8000;
    }
    return flags;
}

static u32 NHttpSocSslFilterNone(u32 flags) {
    return 0;
}

static void NHttpSocSeedRandom(void) {
    OS_GetLowEntropyData(sEntropy);
    func_ov011_02159d64(sEntropy, sizeof(sEntropy));
}

// Whether the socket still has its connection: 0 if reading would wait, -1 if not
int NHTTPi_SocCheck(int sock) {
    int result = 0;
    u8 byte;

    if (sock == -1) {
        result = -1;
    } else if (func_ov011_02157c50(sock, &byte, 1, 4) != -6) {
        result = -1;
    }
    return result;
}

int NHTTPi_SocOpen(NHttpRequest *request) {
    return func_ov011_02157bd0(2, 1, 0);
}

// Closes the socket, trying again while the library says it is busy, and frees the secure connection's settings
int NHTTPi_SocClose(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock) {
    int result = func_ov011_02157d38(sock);

    if (result == 0 || result == -26) {
        int waited = 0;

        while ((result = func_ov011_02157d38(sock)) == -26 && waited <= 10000) {
            func_0207aa04(500);
            func_ov011_021575f8();
            waited += 500;
        }
    }
    if (result != -26 && request != NULL && request->ssl != NULL) {
        NHTTPi_free(request->ssl);
        request->ssl = NULL;
    }
    return result;
}

int NHTTPi_SocConnect(NHttpState *state, NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, u32 addr,
                      int port) {
    SocAddrIn address;
    u16 hostPort;

    address.len = sizeof(address);
    address.family = 2;
    hostPort = port;
    address.port = ((hostPort >> 8) & 0xff) | ((hostPort << 8) & 0xff00);
    address.addr = addr;
    if (request->https != 0 && request->useProxy == 0) {
        int result = NHTTPi_SocSSLConnect(state, mutexInfo, request, sock);

        if (result < 0) {
            return result;
        }
    }
    if (func_ov011_02157c10(sock, &address) < 0) {
        if (request->canceled != 0) {
            return NHTTP_SOCKET_CANCELED;
        }
        return NHTTP_SOCKET_ERROR;
    }
    return 0;
}

// Makes the socket's connection secure, with the request's root certificates and its host to check the server's
// against
int NHTTPi_SocSSLConnect(NHttpState *state, NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock) {
    NHttpSslInfo *ssl = request->ssl;

    if (ssl == NULL) {
        ssl = NHTTPi_alloc(sizeof(NHttpSslInfo), 4);
        request->ssl = ssl;
        NHTTPi_memclr(ssl, sizeof(NHttpSslInfo));
    }
    ssl->rootCerts = request->rootCerts;
    ssl->rootCertCount = request->rootCertCount;
    ssl->hostName = request->host;
    if (request->skipVerify != 0) {
        ssl->filter = NHttpSocSslFilterNone;
    } else {
        ssl->filter = NHttpSocSslFilterDropBit15;
    }
    if (func_ov011_02158174(sock, ssl) < 0) {
        return NHTTP_SOCKET_ERROR;
    }
    NHttpSocSeedRandom();
    return 0;
}

// Reads from the socket into the connection's buffer, and gives the caller what it asked for out of it
int NHTTPi_SocRecv_sub(NHttpConnection *connection, int sock, void *buffer, int size, int flags) {
    u8 *recvBuffer = connection->recvBuffer;
    int copied = 0;

    if (size > 0) {
        u32 count;

        if (connection->recvSize == 0) {
            int result = func_ov011_02157c50(sock, recvBuffer, NHTTP_RECV_BUFFER_SIZE, flags);

            if (result <= 0) {
                return result;
            }
            connection->recvSize = result;
            connection->recvPos = 0;
        }
        count = connection->recvSize;
        if (count != 0) {
            if (size <= count) {
                count = size;
            }
            NHTTPi_memcpy(buffer, recvBuffer + connection->recvPos, count);
            connection->recvSize -= count;
            if (connection->recvSize == 0) {
                NHTTPi_memclr(recvBuffer, NHTTP_RECV_BUFFER_SIZE);
                connection->recvPos = 0;
            } else {
                connection->recvPos += count;
            }
            copied = count;
        }
    }
    return copied;
}

int NHTTPi_SocRecv(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, void *buffer, int size, int flags) {
    NHttpConnection *connection = NHTTPi_Request2Connection(mutexInfo, request);
    int result;

    if (connection != NULL) {
        result = NHTTPi_SocRecv_sub(connection, sock, buffer, size, flags);
    } else {
        return NHTTP_SOCKET_ERROR;
    }
    if (result < 0) {
        if (request->canceled != 0) {
            return NHTTP_SOCKET_CANCELED;
        }
        if (result == -56) {
            return 0;
        }
        return NHTTP_SOCKET_ERROR;
    }
    return result;
}

int NHTTPi_SocSend_sub(int sock, const void *buffer, int size, int flags) {
    return func_ov011_02157cbc(sock, buffer, size, flags);
}

int NHTTPi_SocSend(NHttpRequest *request, int sock, const void *buffer, int size, int flags) {
    int result = NHTTPi_SocSend_sub(sock, buffer, size, flags);

    if (result < 0) {
        if (request->canceled != 0) {
            return NHTTP_SOCKET_CANCELED;
        }
        if (result == -56) {
            return 0;
        }
        return NHTTP_SOCKET_ERROR;
    }
    return result;
}

// Shuts the socket down, which ends a read or a write that waits on it
void NHTTPi_SocCancel(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock) {
    NHTTPi_lockReqList(mutexInfo);
    if (sock >= 0) {
        func_ov011_02157d30(sock, 2);
    }
    NHTTPi_unlockReqList(mutexInfo);
}

// Looks a host up, as an address or as a name, and returns its address (0 if there is none)
u32 NHTTPi_resolveHostname(NHttpRequest *request, const char *host) {
    u32 address = 0;
    u32 parsed;

    if (func_ov011_02158034(host, &parsed) != 0) {
        address = parsed;
    } else {
        SocHostEnt *entry = func_ov011_02157d40(host);

        if (entry != NULL && entry->addrLength > 0) {
            NHTTPi_memcpy(&address, entry->addrList[0], 4);
        }
    }
    return address;
}
