#include "nitrowifi/soc.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/std.h"
#include "socl_internal.h"

// soc.c: the BSD-style SOC_ calls over SOCL, which NitroWiFi's SOC_ API is (pokediamond's soc.o: the same calls under
// the same names, in the reverse order, except that pokediamond calls the host name lookup SOC_GetHostByAddr, the
// name of the reverse lookup; pokeheartgold's SOC_GetHostByName is the one for it). Their addresses and ports are in
// network order, so they swap them. The three tables of socket parameters (SOCLSocketParamUDP and the others) are its
// data.

// How SOC_Socket's sockets are made. A TCP socket gets no receive buffer of its own: it reads from CPS's.
// clang-format off
SOCLSocketParam SOCLSocketParamUDP = {
    1, 1,
    { 0x5c0, 1, 0, 0, 0, 0, 0x800 },
    { 0x800, 12, 32 },
    { 0, 0, 0 },
};

SOCLSocketParam SOCLSocketParamTCP = {
    0, 1,
    { 0, 0, 0xb9e, 0, 0, 0x16d1, 0 },
    { 0x800, 12, 32 },
    { 0x800, 13, 32 },
};

SOCLSocketParam SOCLSocketParamUDPSend = {
    2, 0,
    { 0, 0, 0x5ea, 0, 0, 0x6eb, 0 },
    { 0, 0, 0 },
    { 0x800, 13, 32 },
};
// clang-format on

// The allocator and deallocator that SOC_Startup was given
static SocFreeFunc FreeFunc_SOC;
static SocAllocFunc AllocFunc_SOC;
static SOCHostEnt sHostEnt;
// What SOC_GetHostByName gives: the address, the list of addresses (one, and NULL), and the name
static u32 sHostAddr;
static u8 *sHostAddrList[2];
static char sInetNtoABuffer[16];
static SOCLConfig sSoclConfig;
static char sHostName[0x104];

static void *AllocFunc_SOCL(u32 size);
static void FreeFunc_SOCL(void *ptr);

int SOC_Socket(int domain, int type, int protocol) {
    if (type == SOC_SOCK_STREAM) {
        return SOCL_CreateSocket(&SOCLSocketParamTCP);
    }
    return SOCL_CreateSocket(&SOCLSocketParamUDP);
}

int SOC_Bind(int sock, const void *sockAddr) {
    const SOCSockAddrIn *address = (const SOCSockAddrIn *)sockAddr;

    return SOCL_Bind(sock, SOC_NtoHs(address->port));
}

int SOC_Connect(int sock, const void *sockAddr) {
    const SOCSockAddrIn *address = (const SOCSockAddrIn *)sockAddr;

    return SOCL_Connect(sock, SOC_NtoHs(address->port), SOC_NtoHl(address->addr.addr));
}

int SOC_Recv(int sock, void *buffer, int size, int flags) {
    return SOCL_ReadFrom(sock, buffer, size, NULL, NULL, flags);
}

int SOC_RecvFrom(int sock, void *buffer, int size, int flags, void *from) {
    u16 port;
    CPSInAddr ip;
    int result;

    result = SOCL_ReadFrom(sock, buffer, size, &port, &ip, flags);
    if (result >= 0 && from != NULL) {
        SOCSockAddrIn *addr = (SOCSockAddrIn *)from;

        addr->port = SOC_HtoNs(port);
        addr->addr.addr = SOC_HtoNl(ip);
    }
    return result;
}

int SOC_Send(int sock, const void *buffer, int size, int flags) {
    return SOCL_WriteTo(sock, buffer, size, 0, 0, flags);
}

int SOC_SendTo(int sock, const void *buffer, int size, int flags, const void *sockTo) {
    const SOCSockAddrIn *to;
    u16 port;
    CPSInAddr ip;

    if (sockTo != NULL) {
        to = (const SOCSockAddrIn *)sockTo;
        port = SOC_NtoHs(to->port);
        ip = SOC_NtoHl(to->addr.addr);
    } else {
        port = 0;
        ip = 0;
    }
    return SOCL_WriteTo(sock, buffer, size, port, ip, flags);
}

int SOC_Shutdown(int sock, int how) {
    return SOCL_Shutdown(sock);
}

int SOC_Close(int sock) {
    return SOCL_Close(sock);
}

SOCHostEnt *SOC_GetHostByName(const char *host) {
    CPSInAddr addr = SOCL_Resolve(host);

    if (addr == 0) {
        return NULL;
    }
    MI_CpuFill8(sHostName, 0, 0x101);
    STD_CopyLString(sHostName, host, 0x101);
    sHostEnt.name = sHostName;
    sHostEnt.aliases = NULL;
    sHostEnt.addrType = SOC_AF_INET;
    sHostEnt.addrLength = 4;
    sHostEnt.addrList = sHostAddrList;
    sHostAddrList[0] = (u8 *)&sHostAddr;
    sHostAddrList[1] = NULL;
    sHostAddr = SOC_HtoNl(addr);
    return &sHostEnt;
}

int SOC_GetSockName(int sock, void *sockAddr) {
    SOCSockAddrIn *address = (SOCSockAddrIn *)sockAddr;
    CPSInAddr host_ip;
    u32 port;

    if (sock == 0) {
        return SOCL_ENETRESET;
    }
    host_ip = SOCL_GetHostID();
    port = SOCL_GetHostPort(sock);
    if (host_ip == 0) {
        port = 0;
    }
    address->len = 8;
    address->family = SOC_AF_INET;
    address->port = SOC_HtoNs(port);
    address->addr.addr = SOC_HtoNl(host_ip);
    return 0;
}

long SOC_GetHostID(void) {
    u32 hostId = SOCL_GetHostID();

    return (long)SOC_HtoNl(hostId);
}

// The SDK's name, which is wrong: it sets the DNS servers
int SOC_SetResolver(const SOCInAddr *dns1, const SOCInAddr *dns2) {
    return SOCL_SetResolver(SOC_NtoHl(dns1->addr), SOC_NtoHl(dns2->addr));
}

int SOC_Fcntl(int sock, int command, int flags) {
    SOCLSocket *socket = (SOCLSocket *)sock;

    if (socket == NULL) {
        return -1;
    }
    switch (command) {
    case 3:
        if (socket->flag_block == 1) {
            return 0;
        }
        return 4;
    case 4:
        if (flags & 4) {
            socket->flag_block = 0;
        } else {
            socket->flag_block = 1;
        }
        break;
    }
    return 0;
}

// SOCL's allocator and deallocator, which keep the size of a block in the word before it and hand the user's
// SOC_Startup allocator that size
static void *AllocFunc_SOCL(u32 size) {
    u32 total = size + 4;
    u32 *block = AllocFunc_SOC(0, total);

    if (block != NULL) {
        *block = total;
        block++;
    }
    return block;
}

static void FreeFunc_SOCL(void *ptr) {
    u32 *block = ptr;

    if (block != NULL) {
        block--;
        FreeFunc_SOC(0, block, *block);
    }
}

int SOC_Startup(const SOCConfig *config) {
    sSoclConfig.use_dhcp = config->flag == SOC_FLAG_DHCP ? TRUE : FALSE;
    sSoclConfig.host_ip.my_ip = SOC_NtoHl(config->addr.addr);
    sSoclConfig.host_ip.net_mask = SOC_NtoHl(config->netmask.addr);
    sSoclConfig.host_ip.gateway_ip = SOC_NtoHl(config->router.addr);
    sSoclConfig.host_ip.dns_ip[0] = SOC_NtoHl(config->dns1.addr);
    sSoclConfig.host_ip.dns_ip[1] = SOC_NtoHl(config->dns2.addr);
    sSoclConfig.alloc = AllocFunc_SOCL;
    sSoclConfig.free = FreeFunc_SOCL;
    AllocFunc_SOC = config->alloc;
    FreeFunc_SOC = config->free;
    sSoclConfig.cmd_packet_max = 0x40;
    sSoclConfig.mtu = config->mtu;
    sSoclConfig.rwin = config->rwin;
    return SOCL_Startup(&sSoclConfig);
}

int SOC_Cleanup(void) {
    return SOCL_Cleanup();
}

char *SOC_InetNtoA(SOCInAddr addr) {
    SOC_InetNtoP(SOC_AF_INET, &addr, sInetNtoABuffer, 16);
    return sInetNtoABuffer;
}

int SOC_InetAtoN(const char *text, SOCInAddr *address) {
    CPSInAddr ip = SOCL_InetAtoH(text);

    if (ip == 0) {
        return 0;
    }
    if (address != NULL) {
        address->addr = SOC_HtoNl(ip);
    }
    return 1;
}

const char *SOC_InetNtoP(int family, const void *address, char *buffer, unsigned int size) {
    u32 ip;
    u8 octets[4];

    if (family != SOC_AF_INET) {
        return NULL;
    }
    if (size < 16) {
        return NULL;
    }
    MI_CpuCopy8(address, &ip, 4);
    SOC_U32to4U8(ip, octets);
    OS_SNPrintf(buffer, 16, "%d.%d.%d.%d", octets[3], octets[2], octets[1], octets[0]);
    return buffer;
}

void SOC_U32to4U8(u32 value, u8 *bytes) {
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
}

int SOC_Poll(SOCPollFD *fds, unsigned int count, OSTick timeout) {
    BOOL timed;
    s64 ticks;
    int ready;
    SOCPollFD *fd;
    u32 i;

    if (count == 0 || fds == NULL) {
        return 0;
    }
    timed = timeout != (OSTick)-1;
    ticks = (s64)timeout;
    while (1) {
        fd = fds;
        ready = 0;
        for (i = 0; i < count; i++, fd++) {
            int mask = fd->events | 0xe0;
            int status = SOCL_GetStatus(fd->fd) & mask;

            if (status != 0) {
                ready++;
            }
            fd->revents = status;
        }
        if (ready > 0) {
            break;
        }
        if (timed && ticks <= 0) {
            break;
        }
        OS_Sleep(1);
        ticks -= 523; // One millisecond of OS ticks
    }
    return ready;
}
