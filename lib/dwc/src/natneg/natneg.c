#include <string.h>
// Before dwc/ext.h: MWCC builds the copies of the magic bytes and of the packets in place, as the ROM does, only when
// <string.h> declares memcpy
#include "dwc/ext.h"
#include "dwc/natneg.h"

// GameSpy's NAT negotiation (natneg.c). The ROM names neither the file nor its functions: the file,
// NNFreeNegotiateList, StartReport, SendInitPackets, CheckNatifyStatus, NNBeginNegotiationWithSocket, NNCancel,
// NegotiateThink, NNThink, ProcessConnectPacket, ProcessPingPacket, ProcessInitPacket and NNProcessData are GameSpy's,
// as pret's pokerevo Wii build has them (natneg.s), and FindNegotiatorForCookie, AddNegotiator, RemoveNegotiator,
// CheckMagic, SendReportPacket, SendPingPacket, NameToIp, ResolveServer, ResolveServers, SendConnectAck, NNMagicData
// and the statics are named after the later SDK. The pre-init packets (the cookie's client ID, SendPreInitPacket and
// ProcessPreInitPacket) are newer than both, and those names are ours; RemoveFinishedNegotiator, SendPacket,
// GetLocalIP and GetLocalPort are the later SDK's too (gsAvailable.c and NATify.c have statics of the same names)

// One negotiation with a partner. The guessed address is the partner's public one, as the server or its pings give
// it, and remoteAddr the one that the negotiation ended with
typedef struct NATNegotiator {
    SOCKET gameSock;
    int cookie;
    int clientIndex;
    NegotiateProgressFunc progressCallback;
    NegotiateCompletedFunc completedCallback;
    void *userData;
    SOCKET negotiateSock;
    NegotiateState state;
    int initAckRecv[4];
    int retryCount;
    int maxRetryCount;
    unsigned long retryTime;
    unsigned int guessedIp;
    unsigned short guessedPort;
    unsigned char gotRemoteData;
    unsigned char sendGotRemoteData; // whether the partner has this side's data, so that the negotiation can finish
    NegotiateResult result;
    SOCKET connectedSocket;
    SOCKADDR_IN remoteAddr;
} NATNegotiator;

gsi_u8 NNMagicData[NATNEG_MAGIC_LEN] = { 0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2 };

// The NAT detection's state, which NNThink checks while a detection runs, and the servers' names (to use instead of
// natneg1 to natneg3 of the game's domain) and addresses. With -ipa file, MWCC lays a file's .bss out in an order of
// its own; this order of the definitions gives the ROM's, which starts with negotiateList
static NatDetectionResultsFunc natifyCallback;
static char *Matchup3Hostname;
static gsi_u32 natifyStartTime;
static char *Matchup2Hostname;
static int natMappingScheme;
static gsi_u32 matchup2ip;
static gsi_u32 matchup1ip;
static int clientId;
static DArray negotiateList;
static char *Matchup1Hostname;
static gsi_bool activeNatify;
static gsi_u32 matchup3ip;
static NAT nat;
static int natType = 6;
static int mappingSock = -1;
static int ertSock = -1;

static int GetClientID(void);
static NATNegotiator *FindNegotiatorForCookie(int cookie);
static NATNegotiator *AddNegotiator(void);
static void RemoveNegotiator(NATNegotiator *neg);
static int CheckMagic(const char *data);
static int SendPacket(SOCKET sock, unsigned int toAddr, unsigned short toPort, void *data, int length);
static unsigned int GetLocalIP(void);
static unsigned short GetLocalPort(SOCKET sock);
static void SendReportPacket(NATNegotiator *neg);
static void StartReport(NATNegotiator *neg, NegotiateResult result, SOCKET socket, SOCKADDR_IN *remoteAddr);
static void SendPreInitPacket(NATNegotiator *neg);
static void SendInitPackets(NATNegotiator *neg);
static void SendPingPacket(NATNegotiator *neg);
static gsi_bool CheckNatifyStatus(SOCKET sock);
static unsigned int NameToIp(const char *name);
static unsigned int ResolveServer(const char *overrideHostname, const char *defaultHostname);
static int ResolveServers(void);
static void RemoveFinishedNegotiator(NATNegotiator *neg);
static void NegotiateThink(NATNegotiator *neg);
static void SendConnectAck(NATNegotiator *neg, SOCKADDR_IN *toAddr);
static void ProcessConnectPacket(NATNegotiator *neg, NatNegPacket *packet, SOCKADDR_IN *fromAddr);
static void ProcessPingPacket(NATNegotiator *neg, NatNegPacket *packet, SOCKADDR_IN *fromAddr);
static void ProcessPreInitPacket(NATNegotiator *neg, NatNegPacket *packet);
static void ProcessInitPacket(NATNegotiator *neg, NatNegPacket *packet, SOCKADDR_IN *fromAddr);

// The random ID that the pre-init packets give the servers, picked once
static int GetClientID(void) {
    int id = clientId;

    if (id == 0) {
        Util_RandSeed(current_time());
        id = Util_RandInt(1, 0x7fffffff);
        clientId = id;
    }
    return id;
}

static NATNegotiator *FindNegotiatorForCookie(int cookie) {
    int i;

    if (negotiateList == NULL) {
        return NULL;
    }
    for (i = 0; i < ArrayLength(negotiateList); i++) {
        NATNegotiator *neg = ArrayNth(negotiateList, i);

        if (neg->cookie == cookie) {
            return neg;
        }
    }
    return NULL;
}

static NATNegotiator *AddNegotiator(void) {
    NATNegotiator neg = { 0 };

    if (negotiateList == NULL) {
        negotiateList = ArrayNew(sizeof(NATNegotiator), 4, NULL);
    }
    ArrayAppend(negotiateList, &neg);
    return ArrayNth(negotiateList, ArrayLength(negotiateList) - 1);
}

static void RemoveNegotiator(NATNegotiator *neg) {
    int i;

    for (i = 0; i < ArrayLength(negotiateList); i++) {
        if (neg == ArrayNth(negotiateList, i)) {
            ArrayRemoveAt(negotiateList, i);
            return;
        }
    }
}

void NNFreeNegotiateList(void) {
    if (negotiateList != NULL) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_State, GSIDebugLevel_Notice,
                      "NNFreeNegotiateList: Freeing %d negotiators\n", ArrayLength(negotiateList));
        ArrayFree(negotiateList);
        negotiateList = NULL;
    }
}

static int CheckMagic(const char *data) {
    return memcmp(data, NNMagicData, NATNEG_MAGIC_LEN) == 0;
}

static int SendPacket(SOCKET sock, unsigned int toAddr, unsigned short toPort, void *data, int length) {
    SOCSockAddrIn address;

    address.family = 2;
    address.port = NN_SWAP16(toPort);
    address.addr.addr = toAddr;
    return sendto(sock, data, length, 0, (SOCKADDR *)&address, sizeof(SOCSockAddrIn));
}

// The first address of the console that isn't the loopback one and is a private one, else the last one
static unsigned int GetLocalIP(void) {
    int i;
    HOSTENT *host;
    IN_ADDR *addr;
    gsi_u32 localIp = 0;

    host = getlocalhost();
    if (host == NULL) {
        return 0;
    }
    for (i = 0;; i++) {
        if (host->addrList[i] == NULL) {
            break;
        }
        addr = (IN_ADDR *)host->addrList[i];
        if (addr->addr == NN_SWAP32(0x7f000001)) {
            continue;
        }
        localIp = addr->addr;
        if (IsPrivateIP(addr)) {
            return localIp;
        }
    }
    return localIp;
}

// The port (in network order) that a socket is bound to, or 0
static unsigned short GetLocalPort(SOCKET sock) {
    SOCSockAddrIn address;
    int size = sizeof(SOCSockAddrIn);

    if (getsockname(sock, (SOCKADDR *)&address, &size) == -1) {
        return 0;
    }
    return address.port;
}

static void SendReportPacket(NATNegotiator *neg) {
    NatNegPacket packet;

    memcpy(packet.magic, NNMagicData, NATNEG_MAGIC_LEN);
    packet.version = 4;
    packet.packettype = 13;
    packet.cookie = NN_SWAP32(neg->cookie);
    packet.Packet.report.clientindex = neg->clientIndex;
    packet.Packet.report.negResult = neg->result == 0 ? gsi_true : gsi_false;
    packet.Packet.report.natType = natType;
    packet.Packet.report.natMappingScheme = natMappingScheme;
    if (strlen(__GSIACGamename) > 0) {
        memcpy(packet.Packet.report.gamename, __GSIACGamename, sizeof(packet.Packet.report.gamename));
    }
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "SendReportPacket(cookie=%d): Sending REPORT to %s:%d (result: %d)\n", neg->cookie,
                  SOC_InetNtoA(*(SOCInAddr *)&matchup1ip), 27901, packet.Packet.report.negResult);
    SendPacket(neg->negotiateSock, matchup1ip, 27901, &packet, 73);
}

static void StartReport(NATNegotiator *neg, NegotiateResult result, SOCKET socket, SOCKADDR_IN *remoteAddr) {
    neg->result = result;
    neg->connectedSocket = socket;
    if (remoteAddr != NULL) {
        memcpy(&neg->remoteAddr, remoteAddr, sizeof(neg->remoteAddr));
    }
    if (result == 2 || result == 1) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_State, GSIDebugLevel_Notice,
                      "StartReport(cookie=%d): Negotiation failed, cancelling\n", neg->cookie);
        neg->state = 5;
        neg->completedCallback(neg->result, neg->connectedSocket, &neg->remoteAddr, neg->userData);
        NNCancel(neg->cookie);
    } else {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_State, GSIDebugLevel_Notice,
                      "StartReport(cookie=%d): Negotiation finished, sending report\n", neg->cookie);
        SendReportPacket(neg);
        neg->state = 7;
        neg->retryTime = current_time() + 500;
        neg->retryCount = 0;
        neg->maxRetryCount = 4;
        neg->completedCallback(neg->result, neg->connectedSocket, &neg->remoteAddr, neg->userData);
    }
}

static void SendPreInitPacket(NATNegotiator *neg) {
    NatNegPacket packet;

    memcpy(packet.magic, NNMagicData, NATNEG_MAGIC_LEN);
    packet.version = 4;
    packet.packettype = 15;
    packet.cookie = NN_SWAP32(neg->cookie);
    packet.Packet.preInit.clientindex = neg->clientIndex;
    packet.Packet.preInit.clientID = GetClientID();
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "Sending PREINIT to %s:%d\n",
                  SOC_InetNtoA(*(SOCInAddr *)&matchup1ip), 27901);
    SendPacket(neg->negotiateSock, matchup1ip, 27901, &packet, 18);
    if (neg->state == 1) {
        neg->retryTime = current_time() + 10000;
        neg->maxRetryCount = 12;
    } else {
        neg->retryTime = current_time() + 500;
        neg->maxRetryCount = 10;
        neg->state = 0;
    }
}

static void SendInitPackets(NATNegotiator *neg) {
    char buffer[21 + sizeof(__GSIACGamename)];
    NatNegPacket *packet = (NatNegPacket *)buffer;
    gsi_u32 localIp;
    gsi_u16 localPort;
    int packetLength;

    memcpy(packet->magic, NNMagicData, NATNEG_MAGIC_LEN);
    packet->packettype = 0;
    packet->version = 4;
    packet->cookie = NN_SWAP32(neg->cookie);
    packet->Packet.init.clientindex = neg->clientIndex;
    packet->Packet.init.usegameport = neg->gameSock == -1 ? 0 : 1;
    localIp = NN_SWAP32(GetLocalIP());
    // The address and the port follow the port type and the flags on the wire, unaligned
    buffer[15] = localIp >> 24;
    buffer[16] = localIp >> 16;
    buffer[17] = localIp >> 8;
    buffer[18] = localIp;
    buffer[19] = 0;
    buffer[20] = 0;
    strcpy(buffer + 21, __GSIACGamename);
    packetLength = 21 + strlen(__GSIACGamename) + 1;
    if (packet->Packet.init.usegameport && !neg->initAckRecv[0]) {
        packet->Packet.init.porttype = 0;
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "Sending INIT (GP) to %s:%d...\n",
                      SOC_InetNtoA(*(SOCInAddr *)&matchup1ip), 27901);
        SendPacket(neg->gameSock, matchup1ip, 27901, packet, packetLength);
    }
    if (!neg->initAckRecv[1]) {
        packet->Packet.init.porttype = 1;
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "SendInitPackets(cookie=%d): Sending INIT (NN1) to %s:%d...\n", neg->cookie,
                      SOC_InetNtoA(*(SOCInAddr *)&matchup1ip), 27901);
        SendPacket(neg->negotiateSock, matchup1ip, 27901, packet, packetLength);
    }
    localPort = NN_SWAP16(GetLocalPort(packet->Packet.init.usegameport ? neg->gameSock : neg->negotiateSock));
    buffer[19] = localPort >> 8;
    buffer[20] = localPort;
    if (!neg->initAckRecv[2]) {
        packet->Packet.init.porttype = 2;
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "SendInitPackets(cookie=%d): Sending INIT (NN2) to %s:%d...\n", neg->cookie,
                      SOC_InetNtoA(*(SOCInAddr *)&matchup2ip), 27901);
        SendPacket(neg->negotiateSock, matchup2ip, 27901, packet, packetLength);
    }
    if (!neg->initAckRecv[3]) {
        packet->Packet.init.porttype = 3;
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "SendInitPackets(cookie=%d): Sending INIT (NN3) to %s:%d...\n", neg->cookie,
                      SOC_InetNtoA(*(SOCInAddr *)&matchup3ip), 27901);
        SendPacket(neg->negotiateSock, matchup3ip, 27901, packet, packetLength);
    }
    neg->state = 2;
    neg->retryTime = current_time() + 500;
    neg->maxRetryCount = 10;
}

static void SendPingPacket(NATNegotiator *neg) {
    NatNegPacket packet;

    memcpy(packet.magic, NNMagicData, NATNEG_MAGIC_LEN);
    packet.version = 4;
    packet.packettype = 7;
    packet.cookie = NN_SWAP32(neg->cookie);
    packet.Packet.connect.remoteIP = neg->guessedIp;
    packet.Packet.connect.remotePort = NN_SWAP16(neg->guessedPort);
    packet.Packet.connect.gotyourdata = neg->gotRemoteData;
    packet.Packet.connect.finished = neg->gotRemoteData && neg->sendGotRemoteData ? 1 : 0;
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "SendPingPacket(cookie=%d): Sending PING to %s:%d (got remote data: %d)\n", neg->cookie,
                  SOC_InetNtoA(*(SOCInAddr *)&neg->guessedIp), neg->guessedPort, neg->gotRemoteData);
    SendPacket(neg->gameSock != -1 ? neg->gameSock : neg->negotiateSock, neg->guessedIp, neg->guessedPort, &packet, 20);
    neg->retryTime = current_time() + 700;
    neg->maxRetryCount = 7;
}

static gsi_bool CheckNatifyStatus(SOCKET sock) {
    gsi_bool active = gsi_true;
    gsi_bool success = gsi_false;

    if (sock != -1) {
        if (current_time() - natifyStartTime < 10000) {
            active = NatifyThink(sock, &nat);
        } else {
            active = gsi_false;
        }
        if (!active) {
            success = DetermineNatType(&nat);
            natifyCallback(success, nat);
            natType = nat.natType;
            natMappingScheme = nat.mappingScheme;
            if (mappingSock != -1) {
                closesocket(mappingSock);
                mappingSock = -1;
            }
            if (ertSock != -1) {
                closesocket(ertSock);
                ertSock = -1;
            }
        }
    }
    return active;
}

static unsigned int NameToIp(const char *name) {
    gsi_u32 ip;
    SOCHostEnt *host;

    ip = inet_addr(name);
    if (ip == -1) {
        host = SOC_GetHostByName(name);
        if (host == NULL) {
            return 0;
        }
        ip = *(gsi_u32 *)host->addrList[0];
    }
    return ip;
}

static unsigned int ResolveServer(const char *overrideHostname, const char *defaultHostname) {
    const char *hostname;
    char hostnameBuffer[64];

    if (overrideHostname == NULL) {
        OS_SNPrintf(hostnameBuffer, sizeof(hostnameBuffer), "%s.%s", __GSIACGamename, defaultHostname);
        hostname = hostnameBuffer;
    } else {
        hostname = overrideHostname;
    }
    return NameToIp(hostname);
}

static int ResolveServers(void) {
    if (matchup1ip == 0) {
        matchup1ip = ResolveServer(Matchup1Hostname, "natneg1.gs.nintendowifi.net");
    }
    if (matchup2ip == 0) {
        matchup2ip = ResolveServer(Matchup2Hostname, "natneg2.gs.nintendowifi.net");
    }
    if (matchup3ip == 0) {
        matchup3ip = ResolveServer(Matchup3Hostname, "natneg3.gs.nintendowifi.net");
    }
    if (matchup1ip == 0 || matchup2ip == 0 || matchup3ip == 0) {
        return gsi_false;
    }
    return gsi_true;
}

NegotiateError NNBeginNegotiationWithSocket(SOCKET gameSocket, int cookie, int clientIndex,
                                            NegotiateProgressFunc progress, NegotiateCompletedFunc completed,
                                            void *userData) {
    NATNegotiator *neg;

    if (__GSIACResult != GSIACAvailable) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                      "NNBeginNegotiationWithSocket(cookie=%d): Backend not available\n", cookie);
        return 2;
    }
    if (!ResolveServers()) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                      "NNBeginNegotiationWithSocket(cookie=%d): DNS resolve to NN servers failed\n", cookie);
        return 3;
    }
    neg = AddNegotiator();
    if (neg == NULL) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "NNBeginNegotiationWithSocket: Failed to allocate negotiator\n");
        return 1;
    }
    neg->negotiateSock = socket(2, 2, 0);
    if (neg->negotiateSock == -1) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_HotError,
                      "NNBeginNegotiationWithSocket: Failed to create negotiate socket\n");
        RemoveNegotiator(neg);
        return 2;
    }
    neg->gameSock = gameSocket;
    neg->clientIndex = clientIndex;
    neg->cookie = cookie;
    neg->progressCallback = progress;
    neg->completedCallback = completed;
    neg->userData = userData;
    neg->gotRemoteData = 0;
    neg->sendGotRemoteData = 0;
    neg->guessedIp = 0;
    neg->guessedPort = 0;
    neg->result = 5;
    neg->retryCount = 0;
    neg->maxRetryCount = 0;
    SendPreInitPacket(neg);
    {
        SOCSockAddrIn address;
        int size = sizeof(SOCSockAddrIn);

        getsockname(neg->negotiateSock, (SOCKADDR *)&address, &size);
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "Negotiate Socket: %d\n",
                      NN_SWAP16(address.port));
    }
    return 0;
}

void NNCancel(int cookie) {
    NATNegotiator *neg = FindNegotiatorForCookie(cookie);

    if (neg == NULL) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Misc, GSIDebugLevel_Warning,
                      "NNCancel(cookie=%d): Failed to find negotiator\n", cookie);
        return;
    }
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Misc, GSIDebugLevel_Notice,
                  "NNCancel(cookie=%d): Cancelling negotiator\n", cookie);
    if (neg->negotiateSock != -1) {
        closesocket(neg->negotiateSock);
        neg->negotiateSock = -1;
    }
    neg->state = 6;
}

// Cancels a negotiation that is done; one without the game's socket keeps its own, which it ended with
static void RemoveFinishedNegotiator(NATNegotiator *neg) {
    if (neg->gameSock == -1) {
        neg->negotiateSock = -1;
    }
    NNCancel(neg->cookie);
}

static void NegotiateThink(NATNegotiator *neg) {
    static char indata[0x200];
    SOCSockAddrIn saddr;
    int saddrlen = sizeof(SOCSockAddrIn);
    int error;

    if (neg == NULL) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Misc, GSIDebugLevel_Warning,
                      "NegotiateThink: NULL negotiator\n");
        return;
    }
    if (neg->state == 6) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Memory, GSIDebugLevel_Notice,
                      "NegotiateThink(cookie=%d): Removing canceled negotiator\n", neg->cookie);
        RemoveNegotiator(neg);
        return;
    }
    if (neg->negotiateSock != -1) {
        do {
            if (!CanReceiveOnSocket(neg->negotiateSock)) {
                break;
            }
            error = recvfrom(neg->negotiateSock, indata, sizeof(indata), 0, (SOCKADDR *)&saddr, &saddrlen);
            if (error == -1) {
                gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                              "NegotiateThink(cookie=%d): RECV SOCKET ERROR: %d\n", neg->cookie,
                              GOAGetLastError(neg->negotiateSock));
                break;
            }
            NNProcessData(indata, error, &saddr);
            if (neg->state == 6) {
                break;
            }
        } while (neg->negotiateSock != -1);
    }
    if (neg->state == 0 || neg->state == 1 || neg->state == 2 || neg->state == 4) {
        if (current_time() > neg->retryTime) {
            if (neg->retryCount > neg->maxRetryCount) {
                gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                              "NegotiateThink(cookie=%d): Last init packet resend timed out\n", neg->cookie);
                if (neg->state == 0 || neg->state == 1 || neg->state == 2) {
                    StartReport(neg, 2, -1, NULL);
                } else {
                    StartReport(neg, 3, -1, NULL);
                }
            } else {
                neg->retryCount++;
                gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                              "NegotiateThink(cookie=%d): Resending init packets (retryCount=%d)\n", neg->cookie,
                              neg->retryCount);
                if (neg->state == 0 || neg->state == 1) {
                    SendPreInitPacket(neg);
                } else if (neg->state == 2) {
                    SendInitPackets(neg);
                } else {
                    SendPingPacket(neg);
                }
            }
        }
    }
    if (neg->state == 3 && current_time() > neg->retryTime) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "NegotiateThink(cookie=%d): Failed to receive any pings from partner\n", neg->cookie);
        StartReport(neg, 1, -1, NULL);
    }
    if (neg->state == 4 && current_time() > neg->retryTime) {
        if (neg->retryCount > neg->maxRetryCount) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                          "NegotiateThink(cookie=%d): Last ping packet resend timed out\n", neg->cookie);
            StartReport(neg, 3, -1, NULL);
        } else {
            neg->retryCount++;
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                          "NegotiateThink(cookie=%d): Resending ping packet (retryCount=%d)\n", neg->cookie,
                          neg->retryCount);
            SendPingPacket(neg);
            neg->retryTime = current_time() + 700;
        }
    }
    if (neg->state == 7 && current_time() > neg->retryTime) {
        if (neg->retryCount > neg->maxRetryCount) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_State, GSIDebugLevel_Notice,
                          "NegotiateThink(cookie=%d): Last report packet resend timed out, finishing\n", neg->cookie);
            neg->state = 5;
        } else {
            SendReportPacket(neg);
            neg->retryCount++;
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                          "NegotiateThink(cookie=%d): Resending report packet (retryCount=%d)\n", neg->cookie,
                          neg->retryCount);
            neg->retryTime = current_time() + 500;
        }
    }
    if (neg->state == 5 || neg->state == 8) {
        if (neg->sendGotRemoteData) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_State, GSIDebugLevel_Notice,
                          "NegotiateThink(cookie=%d): Both sides are finished, no need to linger, cancelling\n",
                          neg->cookie);
        } else if (current_time() > neg->retryTime) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_State, GSIDebugLevel_Notice,
                          "NegotiateThink(cookie=%d): Finished negotiator is done lingering, cancelling\n",
                          neg->cookie);
        }
        RemoveFinishedNegotiator(neg);
    }
    if (neg->state == 6) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Memory, GSIDebugLevel_Notice,
                      "NegotiateThink(cookie=%d): Removing canceled negotiator post-update\n", neg->cookie);
        RemoveNegotiator(neg);
    }
}

void NNThink(void) {
    int i;

    if (activeNatify) {
        activeNatify = CheckNatifyStatus(mappingSock) && CheckNatifyStatus(ertSock);
    }
    if (negotiateList != NULL) {
        for (i = ArrayLength(negotiateList) - 1; i >= 0; i--) {
            NegotiateThink(ArrayNth(negotiateList, i));
        }
    }
}

static void SendConnectAck(NATNegotiator *neg, SOCKADDR_IN *toAddr) {
    NatNegPacket packet;

    memcpy(packet.magic, NNMagicData, NATNEG_MAGIC_LEN);
    packet.version = 4;
    packet.packettype = 6;
    packet.cookie = NN_SWAP32(neg->cookie);
    packet.Packet.init.clientindex = neg->clientIndex;
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "SendConnectAck(cookie=%d): Sending connect ack...\n", neg->cookie);
    SendPacket(neg->negotiateSock, toAddr->addr.addr, NN_SWAP16(toAddr->port), &packet, 21);
}

static void ProcessConnectPacket(NATNegotiator *neg, NatNegPacket *packet, SOCKADDR_IN *fromAddr) {
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "ProcessConnectPacket(cookie=%d): Got connect packet (finish code: %d), guess: %s:%d\n", neg->cookie,
                  packet->Packet.connect.finished, SOC_InetNtoA(*(SOCInAddr *)&packet->Packet.connect.remoteIP),
                  NN_SWAP16(packet->Packet.connect.remotePort));
    if (packet->Packet.connect.finished == 0) {
        SendConnectAck(neg, fromAddr);
    }
    if (neg->state >= 4) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "ProcessConnectPacket(cookie=%d): Got connect packet while already connected, ignoring\n",
                      neg->cookie);
        return;
    }
    if (packet->Packet.connect.finished != 0) {
        int errcode = 4;

        if (packet->Packet.connect.finished == 1) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                          "ProcessConnectPacket(cookie=%d): deadbeat partner\n", neg->cookie);
            errcode = 1;
        } else if (packet->Packet.connect.finished == 2) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                          "ProcessConnectPacket(cookie=%d): init timeout\n", neg->cookie);
            errcode = 2;
        }
        StartReport(neg, errcode, -1, NULL);
        return;
    }
    neg->guessedIp = packet->Packet.connect.remoteIP;
    neg->guessedPort = NN_SWAP16(packet->Packet.connect.remotePort);
    neg->retryCount = 0;
    neg->state = 4;
    neg->progressCallback(neg->state, neg->userData);
    SendPingPacket(neg);
}

static void ProcessPingPacket(NATNegotiator *neg, NatNegPacket *packet, SOCKADDR_IN *fromAddr) {
    IN_ADDR senderIp;

    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "ProcessPingPacket(cookie=%d): Got ping from: %s:%d (gotmydata: %d, finished: %d)\n", neg->cookie,
                  SOC_InetNtoA(fromAddr->addr), NN_SWAP16(fromAddr->port), packet->Packet.connect.gotyourdata,
                  packet->Packet.connect.finished);
    if (neg->state < 4) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "ProcessPingPacket(cookie=%d): Received ping before ns_connectping state, ignoring\n",
                      neg->cookie);
        return;
    }
    senderIp = fromAddr->addr;
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "Got ping from: %s:%d (gotmydata: %d, finished: %d)\n", SOC_InetNtoA(senderIp),
                  NN_SWAP16(fromAddr->port), packet->Packet.connect.gotyourdata, packet->Packet.connect.finished);
    neg->guessedIp = fromAddr->addr.addr;
    neg->guessedPort = NN_SWAP16(fromAddr->port);
    neg->gotRemoteData = 1;
    neg->sendGotRemoteData = packet->Packet.connect.gotyourdata ? 1 : 0;
    SendPingPacket(neg);
    if (neg->state == 4 && neg->sendGotRemoteData) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "ProcessPingPacket(cookie=%d): CONNECT FINISHED, starting report\n", neg->cookie);
        StartReport(neg, 0, neg->gameSock != -1 ? neg->gameSock : neg->negotiateSock, fromAddr);
    }
}

static void ProcessPreInitPacket(NATNegotiator *neg, NatNegPacket *packet) {
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "Got preinit ack with state %d\n",
                  packet->Packet.preInit.state);
    if (neg->state != 1) {
        neg->state = 1;
        neg->progressCallback(neg->state, neg->userData);
        neg->retryTime = current_time() + 10000;
        neg->retryCount = 0;
        neg->maxRetryCount = 12;
    }
    switch (packet->Packet.preInit.state) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        SendInitPackets(neg);
        break;
    }
}

static void ProcessInitPacket(NATNegotiator *neg, NatNegPacket *packet, SOCKADDR_IN *fromAddr) {
    switch (packet->packettype) {
    case 1:
        if (packet->Packet.init.porttype > 3) {
            return;
        }
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "Got init ack for port %d\n",
                      packet->Packet.init.porttype);
        neg->initAckRecv[packet->Packet.init.porttype] = gsi_true;
        if (neg->state == 2) {
            if (neg->initAckRecv[1] && neg->initAckRecv[2] && neg->initAckRecv[3] &&
                (neg->gameSock == -1 || neg->initAckRecv[0])) {
                neg->state = 3;
                neg->retryTime = current_time() + 60000;
                neg->progressCallback(neg->state, neg->userData);
            }
        }
        break;
    case 2:
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "Got ERT\n");
        packet->packettype = 3;
        SendPacket(neg->negotiateSock, fromAddr->addr.addr, NN_SWAP16(fromAddr->port), packet, 21);
        break;
    case 14:
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "ProcessInitPacket(cookie=%d): Got REPORT_ACK, finishing\n", neg->cookie);
        neg->state = 8;
        break;
    }
}

void NNProcessData(char *data, int length, SOCKADDR_IN *fromAddr) {
    NatNegPacket packet;
    NATNegotiator *neg;
    gsi_u8 type;

    if (!CheckMagic(data)) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                      "NNProcessData: Invalid packet header, ignoring\n");
        return;
    }
    type = data[7];
    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
                  "NNProcessData: Packet type: %d, %d bytes (%s:%d)\n", type, length, SOC_InetNtoA(fromAddr->addr),
                  NN_SWAP16(fromAddr->port));
    if (type == 5 || type == 7) {
        if (length < 20) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                          "NNProcessData: CONNECT or CONNECT_PING packet too small, ignoring\n");
            return;
        }
        memcpy(&packet, data, 20);
        neg = FindNegotiatorForCookie(NN_SWAP32(packet.cookie));
        if (neg == NULL) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                          "NNProcessData(cookie=%d): Failed to find negotiator, ignoring\n", NN_SWAP32(packet.cookie));
            return;
        }
        if (type == 5) {
            ProcessConnectPacket(neg, &packet, fromAddr);
        } else {
            ProcessPingPacket(neg, &packet, fromAddr);
        }
    } else if (type == 16) {
        if (length < 18) {
            return;
        }
        memcpy(&packet, data, 18);
        neg = FindNegotiatorForCookie(NN_SWAP32(packet.cookie));
        if (neg != NULL) {
            ProcessPreInitPacket(neg, &packet);
        }
    } else {
        if (length < 21) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                          "NNProcessData: non-CONNECT/CONNECT_PING packet too small, ignoring\n");
            return;
        }
        memcpy(&packet, data, 21);
        neg = FindNegotiatorForCookie(NN_SWAP32(packet.cookie));
        if (neg == NULL) {
            gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Warning,
                          "NNProcessData(cookie=%d): Failed to find negotiator, ignoring\n", NN_SWAP32(packet.cookie));
            return;
        }
        ProcessInitPacket(neg, &packet, fromAddr);
    }
}
