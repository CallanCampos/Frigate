#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// soc.c: the BSD-style SOC_ calls over SOCL, which NitroWiFi's SOC_ API is (pokediamond's soc.o: the same calls under
// the same names, in the reverse order, except that pokediamond calls the host name lookup SOC_GetHostByAddr, the
// name of the reverse lookup; pokeheartgold's SOC_GetHostByName is the one for it). Their addresses and ports are in network order, so they swap them. The three
// tables of socket parameters (SOCLSocketParamUDP and the others) are its data.

// How SOC_Socket's sockets are made. A TCP socket gets no receive buffer of its own: it reads from CPS's.
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

// The allocator and deallocator that SOC_Startup was given
static SocFreeFunc FreeFunc_SOC;
static SocAllocFunc AllocFunc_SOC;
static SocHostEnt sHostEnt;
// What SOC_GetHostByName gives: the address, the list of addresses (one, and NULL), and the name
static u32 sHostAddr;
static u8 *sHostAddrList[2];
static char sInetNtoABuffer[16];
static SOCLConfig sSoclConfig;
static char sHostName[0x104];

static void *AllocFunc_SOCL(u32 size);
static void FreeFunc_SOCL(void *ptr);

static inline u16 SOC_Swap16(u16 value) {
    return ((value >> 8) & 0xff) | ((value << 8) & 0xff00);
}

static inline u32 SOC_Swap32(u32 value) {
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

int SOC_Socket(int domain, int type, int protocol) {
    if (type == POKE_NET_SOC_SOCK_STREAM) {
        return SOCL_CreateSocket(&SOCLSocketParamTCP);
    }
    return SOCL_CreateSocket(&SOCLSocketParamUDP);
}

int SOC_Bind(int sock, const SocAddrIn *address) {
    return SOCL_Bind((SOCLSocket *)sock, SOC_Swap16(address->port));
}

int SOC_Connect(int sock, const SocAddrIn *address) {
    return SOCL_Connect((SOCLSocket *)sock, SOC_Swap16(address->port), SOC_Swap32(address->addr));
}

int SOC_Read(int sock, void *buffer, int size, int flags) {
    return SOCLi_ReadFrom((SOCLSocket *)sock, buffer, size, NULL, NULL, flags);
}

int SOC_RecvFrom(int sock, void *buffer, int size, int flags, SocAddrIn *from) {
    CPSInAddr ip;
    u16 port;
    int result;

    result = SOCLi_ReadFrom((SOCLSocket *)sock, buffer, size, &port, &ip, flags);
    if (result >= 0) {
        if (from != NULL) {
            from->port = SOC_Swap16(port);
            from->addr = SOC_Swap32(ip);
        }
    }
    return result;
}

int SOC_Send(int sock, const void *buffer, int size, int flags) {
    return SOCL_WriteTo((SOCLSocket *)sock, buffer, size, 0, 0, flags);
}

int SOC_SendTo(int sock, const void *buffer, int size, int flags, const SocAddrIn *to) {
    u16 port;
    CPSInAddr ip;

    if (to != NULL) {
        port = SOC_Swap16(to->port);
        ip = SOC_Swap32(to->addr);
    } else {
        port = 0;
        ip = 0;
    }
    return SOCL_WriteTo((SOCLSocket *)sock, buffer, size, port, ip, flags);
}

int SOC_Shutdown(int sock, int how) {
    return SOCL_Shutdown((SOCLSocket *)sock);
}

int SOC_Close(int sock) {
    return SOCL_Close(sock);
}

SocHostEnt *SOC_GetHostByName(const char *host) {
    CPSInAddr addr = SOCL_Resolve(host);

    if (addr == 0) {
        return NULL;
    }
    sys_memset(sHostName, 0, 0x101);
    func_0207f7cc(sHostName, host, 0x101);
    sHostEnt.name = sHostName;
    sHostEnt.aliases = NULL;
    sHostEnt.addrType = POKE_NET_SOC_AF_INET;
    sHostEnt.addrLength = 4;
    sHostEnt.addrList = sHostAddrList;
    sHostAddrList[0] = (u8 *)&sHostAddr;
    sHostAddrList[1] = NULL;
    sHostAddr = SOC_Swap32(addr);
    return &sHostEnt;
}

int SOC_GetSockName(int sock, SocAddrIn *address) {
    SOCLSocket *socket = (SOCLSocket *)sock;
    CPSInAddr host_ip;
    u32 port;

    if (socket == NULL) {
        return SOCL_ENETRESET;
    }
    host_ip = SOCL_GetHostID();
    if (socket != NULL) {
        port = socket->local_port;
    } else {
        port = 0;
    }
    if (host_ip == 0) {
        port = 0;
    }
    address->len = 8;
    address->family = POKE_NET_SOC_AF_INET;
    address->port = SOC_Swap16(port);
    address->addr = SOC_Swap32(host_ip);
    return 0;
}

u32 SOC_GetHostID(void) {
    return SOC_Swap32(SOCL_GetHostID());
}

// The SDK's name, which is wrong: it sets the DNS servers
int SOC_GetResolver(const u32 *dns1, const u32 *dns2) {
    return SOCL_SetResolver(SOC_Swap32(*dns1), SOC_Swap32(*dns2));
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

int SOC_Startup(const SocConfig *config) {
    sSoclConfig.use_dhcp = config->useDhcp == 1 ? TRUE : FALSE;
    sSoclConfig.host_ip.my_ip = SOC_Swap32(config->addr);
    sSoclConfig.host_ip.net_mask = SOC_Swap32(config->netmask);
    sSoclConfig.host_ip.gateway_ip = SOC_Swap32(config->gateway);
    sSoclConfig.host_ip.dns_ip[0] = SOC_Swap32(config->dns1);
    sSoclConfig.host_ip.dns_ip[1] = SOC_Swap32(config->dns2);
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

char *SOC_InetNtoA(SocInAddr addr) {
    SOC_InetNtoP(POKE_NET_SOC_AF_INET, &addr, sInetNtoABuffer, 16);
    return sInetNtoABuffer;
}

int SOC_InetAtoN(const char *text, u32 *address) {
    CPSInAddr ip = SOCL_InetAtoH(text);

    if (ip == 0) {
        return 0;
    }
    if (address != NULL) {
        *address = SOC_Swap32(ip);
    }
    return 1;
}

char *SOC_InetNtoP(int family, const void *address, char *buffer, u32 size) {
    u32 ip;
    u8 octets[4];

    if (family != POKE_NET_SOC_AF_INET) {
        return NULL;
    }
    if (size < 16) {
        return NULL;
    }
    sys_memcpy(address, &ip, 4);
    SOC_U32to4U8(ip, octets);
    func_0207a2c0(buffer, 16, "%d.%d.%d.%d", octets[3], octets[2], octets[1], octets[0]);
    return buffer;
}

void SOC_U32to4U8(u32 value, u8 *bytes) {
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
}

int SOC_Poll(SocPollFd *fds, u32 count, s64 ticks) {
    BOOL timed;
    int ready;
    SocPollFd *fd;
    u32 i;

    if (count == 0 || fds == NULL) {
        return 0;
    }
    timed = ticks != -1;
    while (1) {
        fd = fds;
        ready = 0;
        for (i = 0; i < count; i++, fd++) {
            int mask = fd->events | 0xe0;
            int status = SOCL_GetStatus((SOCLSocket *)fd->fd) & mask;

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
        func_0207aa04(1);
        ticks -= 523; // One millisecond of OS ticks
    }
    return ready;
}
