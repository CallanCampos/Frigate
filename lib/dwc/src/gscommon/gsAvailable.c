#include "dwc/gsAvailable.h"
#include "dwc/ext.h"

// GameSpy's availability check (gsAvailable.c). The ROM names neither the file nor its functions: the file,
// GSIStartAvailableCheckA and GSIAvailableCheckThink are GameSpy's, as pret's pokerevo Wii build has them
// (gsAvailable.s), and so are __GSIACResult, __GSIACGamename and GSIACHostname after the later SDK; so are the
// static functions and data

// The check in progress: the socket, the server's address, the packet, its size, when it was last sent and how many
// times it was sent again
typedef struct AvailableCheck {
    int sock;
    SOCSockAddrIn address;
    char packet[0x40];
    int packetSize;
    gsi_u32 sendTime;
    int sendCount;
} AvailableCheck;

static gsi_bool get_sockaddrin(const char *host, int port, SOCSockAddrIn *address);
static void SendPacket(void);
static int HandlePacket(const char *buffer, int size, const SOCSockAddrIn *from, gsi_u32 *flags);

// MWCC lays out the .bss in the reverse order of the definitions, and the ROM has __GSIACResult first
static AvailableCheck AC;
char GSIACHostname[0x40];
char __GSIACGamename[0x40];
GSIACResult __GSIACResult;

// The address of a host (a text of four numbers or a name to look up) at a port
static gsi_bool get_sockaddrin(const char *host, int port, SOCSockAddrIn *address) {
    address->family = 2;
    // The port in network order
    address->port = (gsi_u8)((gsi_u16)port >> 8) | (((gsi_u16)port << 8) & 0xff00);
    address->addr.addr = inet_addr(host);
    if (address->addr.addr == -1) {
        SOCHostEnt *entry = SOC_GetHostByName(host);

        if (entry == NULL) {
            return gsi_false;
        }
        address->addr.addr = *(gsi_u32 *)entry->addrList[0];
    }
    return gsi_true;
}

static void SendPacket(void) {
    sendto(AC.sock, AC.packet, AC.packetSize, 0, (SOCKADDR *)&AC.address, sizeof(SOCSockAddrIn));
    AC.sendTime = current_time();
}

void GSIStartAvailableCheckA(const char *gamename) {
    char host[0x40];
    char hostSet;
    const char *useHost;

    gsiSafeStrcpyA(__GSIACGamename, gamename, sizeof(__GSIACGamename));
    AC.sock = -1;
    SocketStartUp();
    hostSet = GSIACHostname[0];
    if (hostSet == '\0') {
        OS_SPrintf(host, "%s.available.gs.nintendowifi.net", gamename);
    }
    useHost = GSIACHostname;
    if (hostSet == '\0') {
        useHost = host;
    }
    if (get_sockaddrin(useHost, 27900, &AC.address)) {
        AC.sock = socket(2, 2, 0);
        if (AC.sock != -1) {
            gsi_u32 length;

            AC.packet[0] = 9;
            length = strlen(gamename);
            memcpy(&AC.packet[5], gamename, length + 1);
            AC.packetSize = length + 6;
            SendPacket();
            AC.sendCount = 0;
        }
    }
}

// Reads the server's answer: 0 with its flags for an answer from the server, 1 for anything else
static int HandlePacket(const char *buffer, int size, const SOCSockAddrIn *from, gsi_u32 *flags) {
    if (size < 7) {
        return 1;
    }
    if (memcmp(&from->addr, &AC.address.addr, 4) != 0) {
        return 1;
    }
    if (from->port != AC.address.port) {
        return 1;
    }
    if (memcmp(buffer, "\xfe\xfd\x09", 3) != 0) {
        return 1;
    }
    {
        gsi_u32 value = (buffer[3] << 24) & 0xff000000;

        value |= (buffer[4] << 16) & 0xff0000;
        value |= (buffer[5] << 8) & 0xff00;
        value |= (gsi_u8)buffer[6];
        *flags = value;
    }
    return 0;
}

GSIACResult GSIAvailableCheckThink(void) {
    char buffer[0x40];
    SOCSockAddrIn from;
    int fromSize = sizeof(SOCSockAddrIn);
    gsi_u32 flags;

    if (AC.sock == -1) {
        __GSIACResult = GSIACAvailable;
        return __GSIACResult;
    }
    if (CanReceiveOnSocket(AC.sock)) {
        int size = recvfrom(AC.sock, buffer, sizeof(buffer), 0, (SOCKADDR *)&from, &fromSize);

        if (HandlePacket(buffer, size, &from, &flags) == 0) {
            closesocket(AC.sock);
            if (flags & 1) {
                __GSIACResult = GSIACUnavailable;
            } else if (flags & 2) {
                __GSIACResult = GSIACTemporarilyUnavailable;
            } else {
                __GSIACResult = GSIACAvailable;
            }
            return __GSIACResult;
        }
    }
    if (current_time() > AC.sendTime + 2000) {
        if (AC.sendCount == 1) {
            closesocket(AC.sock);
            __GSIACResult = GSIACAvailable;
            return __GSIACResult;
        }
        SendPacket();
        AC.sendCount++;
    }
    return GSIACWaiting;
}
