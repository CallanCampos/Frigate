#include "dwc/gsPlatformSocket.h"
#include "dwc/gsDebug.h"
// MWCC copies the address in sendto in place, as the ROM does, only when <string.h> declares memcpy
#include <string.h>

// GameSpy's sockets (gsPlatformSocket.c). The ROM names neither the file nor its functions: the file is the later
// SDK's, and the functions are GameSpy's, as pret's pokerevo Wii build has them (nonport.s), or the later SDK's
// (CheckRcode, closesocketsure and socket, of the nitro/gsSocketNitro.c that it includes)

static int CheckRcode(int result, int error);

// The state of the sockets: the aliases of the console's host entry (none), the error of the last call that failed,
// and the host entry with the console's address. Each is a static of its own defined with a zero initializer, which
// the functions reach from the section's base while getlocalhost takes each address through a literal of its own
static char *sAliases = NULL;
static int sLastError = 0;
static HOSTENT sHostEnt = { 0 };
static gsi_u32 sIps[5] = { 0 };
static gsi_u8 *sIpPtrs[17] = { 0 };

// Keeps the error of a call that failed and returns the error value, or returns the result
static int CheckRcode(int result, int error) {
    if (result < 0) {
        sLastError = result;
        return error;
    }
    return result;
}

int socket(int family, int type, int protocol) {
    return CheckRcode(SOC_Socket(family, type, protocol), -1);
}

int closesocket(SOCKET sock) {
    return CheckRcode(SOC_Close(sock), -1);
}

gsi_bool closesocketsure(SOCKET sock, gsi_bool *secure) {
    int result = SOC_Close(sock);

    if (result == 0) {
        *secure = *secure == gsi_false;
        return *secure;
    }
    if (!SOCL_IsNotClosing(sock)) {
        return gsi_false;
    }
    CheckRcode(result, -1);
    *secure = gsi_true;
    return gsi_true;
}

int shutdown(SOCKET sock, int how) {
    return CheckRcode(SOC_Shutdown(sock, how), -1);
}

int bind(SOCKET sock, const SOCKADDR *address, int size) {
    SOCKADDR local;

    if (((const SOCKADDR_IN *)address)->port == 0) {
        return 0;
    }
    local = *address;
    local.len = size;
    return CheckRcode(SOC_Bind(sock, &local), -1);
}

int connect(SOCKET sock, const SOCKADDR *address, int size) {
    SOCKADDR remote = *address;

    remote.len = size;
    return CheckRcode(SOC_Connect(sock, &remote), -1);
}

int recv(SOCKET sock, char *buffer, int size, int flags) {
    return CheckRcode(SOC_Recv(sock, buffer, size, flags), -1);
}

int recvfrom(SOCKET sock, char *buffer, int size, int flags, SOCKADDR *address, int *addressSize) {
    address->len = *addressSize;
    int result = SOC_RecvFrom(sock, buffer, size, flags, address);
    *addressSize = address->len;
    return CheckRcode(result, -1);
}

int send(SOCKET sock, const char *buffer, int size, int flags) {
    return CheckRcode(SOC_Send(sock, buffer, size, flags), -1);
}

int sendto(SOCKET sock, const char *buffer, int size, int flags, const SOCKADDR *address, int addressSize) {
    SOCKADDR remote;

    memcpy(&remote, address, sizeof(remote));
    remote.len = addressSize;
    return CheckRcode(SOC_SendTo(sock, buffer, size, flags, &remote), -1);
}

int setsockopt(SOCKET sock, int level, int name, const char *value, int size) {
    return CheckRcode(0, -1);
}

int getsockname(SOCKET sock, SOCKADDR *address, int *size) {
    address->len = *size;
    int result = SOC_GetSockName(sock, address);
    *size = address->len;
    return CheckRcode(result, -1);
}

unsigned long inet_addr(const char *host) {
    IN_ADDR address;

    if (SOC_InetAtoN(host, &address) == 0) {
        return -1;
    }
    return address.addr;
}

int GOAGetLastError(SOCKET sock) {
    return sLastError;
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
    sHostEnt.name = "localhost";
    sHostEnt.aliases = &sAliases;
    sHostEnt.addrType = 2;
    sHostEnt.addrLength = 0;
    sHostEnt.addrList = sIpPtrs;
    sIps[0] = 0;
    SOC_U32to4U8(SOCL_GetHostID(), (gsi_u8 *)&sIps[0]);
    if (sIps[0] == 0) {
        return NULL;
    }
    sIpPtrs[0] = (gsi_u8 *)&sIps[0];
    sHostEnt.addrLength = 4;
    sIpPtrs[1] = NULL;
    return &sHostEnt;
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
