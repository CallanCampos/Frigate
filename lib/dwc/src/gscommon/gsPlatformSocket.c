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
    SocHostEnt hostEnt;
    u32 ips[5];
    u8 *ipPtrs[17];
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

int closesocket(int sock) {
    return gsiSocketCheckResult(SOC_Close(sock), -1);
}

BOOL gsiCloseSocketSecure(int sock, BOOL *secure) {
    int result = SOC_Close(sock);

    if (result == 0) {
        *secure = *secure == FALSE;
        return *secure;
    }
    if (!SOCL_IsNotClosing(sock)) {
        return FALSE;
    }
    gsiSocketCheckResult(result, -1);
    *secure = TRUE;
    return TRUE;
}

int shutdown(int sock, int how) {
    return gsiSocketCheckResult(SOC_Shutdown(sock, how), -1);
}

int bind(int sock, const SocAddrIn *address, int size) {
    SocSockAddr local;

    if (address->port == 0) {
        return 0;
    }
    local = *(const SocSockAddr *)address;
    local.len = size;
    return gsiSocketCheckResult(SOC_Bind(sock, (const SocAddrIn *)&local), -1);
}

int connect(int sock, const SocAddrIn *address, int size) {
    SocSockAddr remote = *(const SocSockAddr *)address;

    remote.len = size;
    return gsiSocketCheckResult(SOC_Connect(sock, (const SocAddrIn *)&remote), -1);
}

int recv(int sock, void *buffer, int size, int flags) {
    return gsiSocketCheckResult(SOC_Read(sock, buffer, size, flags), -1);
}

int recvfrom(int sock, void *buffer, int size, int flags, SocAddrIn *address, int *addressSize) {
    address->len = *addressSize;
    int result = SOC_RecvFrom(sock, buffer, size, flags, address);
    *addressSize = address->len;
    return gsiSocketCheckResult(result, -1);
}

int send(int sock, const void *buffer, int size, int flags) {
    return gsiSocketCheckResult(SOC_Send(sock, buffer, size, flags), -1);
}

int sendto(int sock, const void *buffer, int size, int flags, const SocAddrIn *address, int addressSize) {
    SocSockAddr remote;

    remote = *(const SocSockAddr *)address;
    remote.len = addressSize;
    return gsiSocketCheckResult(SOC_SendTo(sock, buffer, size, flags, (const SocAddrIn *)&remote), -1);
}

int setsockopt(int sock, int level, int name, const void *value, int size) {
    return gsiSocketCheckResult(0, -1);
}

int getsockname(int sock, SocAddrIn *address, int *size) {
    address->len = *size;
    int result = SOC_GetSockName(sock, address);
    *size = address->len;
    return gsiSocketCheckResult(result, -1);
}

int inet_addr(const char *host) {
    u32 address;

    if (SOC_InetAtoN(host, &address) == 0) {
        return -1;
    }
    return address;
}

int GOAGetLastError(int sock) {
    return sSocketState.lastError;
}

int GSISocketSelect(int sock, BOOL *readable, BOOL *writable, BOOL *failed) {
    SocPollFd fd;
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
            *readable = TRUE;
        } else {
            *readable = FALSE;
        }
    }
    if (writable != NULL) {
        if (result > 0 && (fd.revents & 8)) {
            *writable = TRUE;
        } else {
            *writable = FALSE;
        }
    }
    if (failed != NULL) {
        if (result > 0 && (fd.revents & 0x20)) {
            *failed = TRUE;
        } else {
            *failed = FALSE;
        }
    }
    return result;
}

BOOL SetSockBlocking(int sock, BOOL blocking) {
    int flags = SOC_Fcntl(sock, 3, 0);

    if (blocking) {
        flags &= ~4;
    } else {
        flags |= 4;
    }
    if (SOC_Fcntl(sock, 4, flags) == 0) {
        gsDebugFormat(12, 0, 0x10, "SetSockBlocking: Set socket %d to %s\r\n", sock,
                      blocking ? "blocking" : "non-blocking");
        return TRUE;
    }
    gsDebugFormat(12, 0, 0x10, "SetSockBlocking failed: tried to set socket %d to %s\r\n", sock,
                  blocking ? "blocking" : "non-blocking");
    return FALSE;
}

BOOL SetReceiveBufferSize(int sock, int size) {
    if (setsockopt(sock, 0xffff, 0x1002, &size, sizeof(size)) != -1) {
        return TRUE;
    }
    return FALSE;
}

BOOL CanReceiveOnSocket(int sock) {
    BOOL result = FALSE;
    BOOL readable = FALSE;

    if (GSISocketSelect(sock, &readable, NULL, NULL) == 1) {
        result = readable;
    }
    return result;
}

BOOL CanSendOnSocket(int sock) {
    BOOL result = FALSE;
    BOOL writable = FALSE;

    if (GSISocketSelect(sock, NULL, &writable, NULL) == 1) {
        result = writable;
    }
    return result;
}

SocHostEnt *getlocalhost(void) {
    sSocketState.hostEnt.name = "localhost";
    sSocketState.hostEnt.aliases = &sSocketState.aliases;
    sSocketState.hostEnt.addrType = 2;
    sSocketState.hostEnt.addrLength = 0;
    sSocketState.hostEnt.addrList = sSocketState.ipPtrs;
    sSocketState.ips[0] = 0;
    SOC_U32to4U8(SOCL_GetHostID(), (u8 *)&sSocketState.ips[0]);
    if (sSocketState.ips[0] == 0) {
        return NULL;
    }
    sSocketState.ipPtrs[0] = (u8 *)&sSocketState.ips[0];
    sSocketState.hostEnt.addrLength = 4;
    sSocketState.ipPtrs[1] = NULL;
    return &sSocketState.hostEnt;
}

BOOL IsPrivateIP(const u32 *address) {
    u32 ip = *address;
    int a;
    int b;

    // The address in host order
    ip = ((ip << 24) & 0xff000000) | (((ip << 8) & 0x00ff0000) | ((u8)(ip >> 24) | ((ip >> 8) & 0x0000ff00)));
    a = (u8)(ip >> 24);
    b = (u8)(ip >> 16);
    if (a == 10) {
        return TRUE;
    }
    if (a == 172 && b >= 16 && b <= 31) {
        return TRUE;
    }
    if (a == 192 && b == 168) {
        return TRUE;
    }
    return FALSE;
}
