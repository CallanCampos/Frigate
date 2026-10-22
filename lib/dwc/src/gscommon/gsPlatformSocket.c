#include "dwc/gsPlatformSocket.h"
#include "dwc/gsDebug.h"

// GameSpy's sockets (gsPlatformSocket.c). The ROM names neither the file nor its functions: the file is the later
// SDK's, and the functions are GameSpy's as pret's pokerevo Wii build has them (nonport.s) but for three of ours:
// gsiSocketCheckResult, gsiCloseSocketSecure, and gsiSocket for socket, whose name GT2's own `socket` variables hide

static int gsiSocketCheckResult(int result, int error);

// The state of the sockets: the aliases of the console's host entry (none), the error of the last call that failed,
// and the host entry with the console's address
static struct {
    char *aliases;
    int lastError;
    HOSTENT hostEnt;
    gsi_u32 ips[5];
    gsi_u8 *ipPtrs[17];
} sSocketState;

// Keeps the error of a call that failed and returns the error value, or returns the result
static int gsiSocketCheckResult(int result, int error) {
    if (result < 0) {
        sSocketState.lastError = result;
        return error;
    }
    return result;
}

int gsiSocket(int family, int type, int protocol) {
    return gsiSocketCheckResult(SOC_Socket(family, type, protocol), -1);
}

int closesocket(SOCKET sock) {
    return gsiSocketCheckResult(SOC_Close(sock), -1);
}

gsi_bool gsiCloseSocketSecure(SOCKET sock, gsi_bool *secure) {
    int result = SOC_Close(sock);

    if (result == 0) {
        *secure = *secure == gsi_false;
        return *secure;
    }
    if (!SOCL_IsNotClosing(sock)) {
        return gsi_false;
    }
    gsiSocketCheckResult(result, -1);
    *secure = gsi_true;
    return gsi_true;
}

int shutdown(SOCKET sock, int how) {
    return gsiSocketCheckResult(SOC_Shutdown(sock, how), -1);
}

int bind(SOCKET sock, const SOCKADDR *address, int size) {
    SOCKADDR local;

    if (((const SOCKADDR_IN *)address)->port == 0) {
        return 0;
    }
    local = *address;
    local.len = size;
    return gsiSocketCheckResult(SOC_Bind(sock, &local), -1);
}

int connect(SOCKET sock, const SOCKADDR *address, int size) {
    SOCKADDR remote = *address;

    remote.len = size;
    return gsiSocketCheckResult(SOC_Connect(sock, &remote), -1);
}

int recv(SOCKET sock, char *buffer, int size, int flags) {
    return gsiSocketCheckResult(SOC_Recv(sock, buffer, size, flags), -1);
}

int recvfrom(SOCKET sock, char *buffer, int size, int flags, SOCKADDR *address, int *addressSize) {
    address->len = *addressSize;
    int result = SOC_RecvFrom(sock, buffer, size, flags, address);
    *addressSize = address->len;
    return gsiSocketCheckResult(result, -1);
}

int send(SOCKET sock, const char *buffer, int size, int flags) {
    return gsiSocketCheckResult(SOC_Send(sock, buffer, size, flags), -1);
}

int sendto(SOCKET sock, const char *buffer, int size, int flags, const SOCKADDR *address, int addressSize) {
    SOCKADDR remote;

    remote = *address;
    remote.len = addressSize;
    return gsiSocketCheckResult(SOC_SendTo(sock, buffer, size, flags, &remote), -1);
}

int setsockopt(SOCKET sock, int level, int name, const char *value, int size) {
    return gsiSocketCheckResult(0, -1);
}

int getsockname(SOCKET sock, SOCKADDR *address, int *size) {
    address->len = *size;
    int result = SOC_GetSockName(sock, address);
    *size = address->len;
    return gsiSocketCheckResult(result, -1);
}

unsigned long inet_addr(const char *host) {
    IN_ADDR address;

    if (SOC_InetAtoN(host, &address) == 0) {
        return -1;
    }
    return address.addr;
}

int GOAGetLastError(SOCKET sock) {
    return sSocketState.lastError;
}

int GSISocketSelect(SOCKET sock, int *readable, int *writable, int *failed) {
    SOCPollFD fd;
    int result;

    fd.fd = sock;
    fd.events = 0;
    if (readable != NULL) {
        fd.events |= 1;
    }
    if (writable != NULL) {
        fd.events |= 8;
    }
    fd.revents = 0;
    result = SOC_Poll(&fd, 1, 0);
    if (result < 0) {
        return -1;
    }
    if (readable != NULL) {
        if (result > 0 && (fd.revents & 0x41)) {
            *readable = gsi_true;
        } else {
            *readable = gsi_false;
        }
    }
    if (writable != NULL) {
        if (result > 0 && (fd.revents & 8)) {
            *writable = gsi_true;
        } else {
            *writable = gsi_false;
        }
    }
    if (failed != NULL) {
        if (result > 0 && (fd.revents & 0x20)) {
            *failed = gsi_true;
        } else {
            *failed = gsi_false;
        }
    }
    return result;
}

int SetSockBlocking(SOCKET sock, int blocking) {
    int flags = SOC_Fcntl(sock, 3, 0);

    if (blocking) {
        flags &= ~4;
    } else {
        flags |= 4;
    }
    if (SOC_Fcntl(sock, 4, flags) == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "SetSockBlocking: Set socket %d to %s\r\n", sock, blocking ? "blocking" : "non-blocking");
        return gsi_true;
    }
    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "SetSockBlocking failed: tried to set socket %d to %s\r\n", sock,
                  blocking ? "blocking" : "non-blocking");
    return gsi_false;
}

int SetReceiveBufferSize(SOCKET sock, int size) {
    if (setsockopt(sock, 0xffff, 0x1002, (const char *)&size, sizeof(size)) != -1) {
        return gsi_true;
    }
    return gsi_false;
}

int CanReceiveOnSocket(SOCKET sock) {
    int result = gsi_false;
    int readable = gsi_false;

    if (GSISocketSelect(sock, &readable, NULL, NULL) == 1) {
        result = readable;
    }
    return result;
}

int CanSendOnSocket(SOCKET sock) {
    int result = gsi_false;
    int writable = gsi_false;

    if (GSISocketSelect(sock, NULL, &writable, NULL) == 1) {
        result = writable;
    }
    return result;
}

HOSTENT *getlocalhost(void) {
    sSocketState.hostEnt.name = "localhost";
    sSocketState.hostEnt.aliases = &sSocketState.aliases;
    sSocketState.hostEnt.addrType = 2;
    sSocketState.hostEnt.addrLength = 0;
    sSocketState.hostEnt.addrList = sSocketState.ipPtrs;
    sSocketState.ips[0] = 0;
    SOC_U32to4U8(SOCL_GetHostID(), (gsi_u8 *)&sSocketState.ips[0]);
    if (sSocketState.ips[0] == 0) {
        return NULL;
    }
    sSocketState.ipPtrs[0] = (gsi_u8 *)&sSocketState.ips[0];
    sSocketState.hostEnt.addrLength = 4;
    sSocketState.ipPtrs[1] = NULL;
    return &sSocketState.hostEnt;
}

int IsPrivateIP(IN_ADDR *address) {
    gsi_u32 ip = address->addr;
    int a;
    int b;

    // The address in host order
    ip = ((ip << 24) & 0xff000000) | (((ip << 8) & 0x00ff0000) | ((gsi_u8)(ip >> 24) | ((ip >> 8) & 0x0000ff00)));
    a = (gsi_u8)(ip >> 24);
    b = (gsi_u8)(ip >> 16);
    if (a == 10) {
        return gsi_true;
    }
    if (a == 172 && b >= 16 && b <= 31) {
        return gsi_true;
    }
    if (a == 192 && b == 168) {
        return gsi_true;
    }
    return gsi_false;
}
