#include <string.h>
// Before dwc/ext.h: MWCC builds Think's copies of the servers' packets in place, as the ROM does, only when <string.h>
// declares memcpy
#include "dwc/NATify.h"
#include "dwc/ext.h"
#include "dwc/natneg.h"

// GameSpy's NAT detection (NATify.c). The ROM names neither the file nor its functions: the file, AddressToString,
// GetLocalIP, Think, NatifyThink and DetermineNatType are GameSpy's, as pret's pokerevo Wii build has them
// (NATify.s), and GetLocalPort, OutputMapping and the statics are named after the later SDK

static gsi_bool gotERT1;
static gsi_bool gotERT2;
static gsi_bool gotERT3;
static gsi_bool gotADDRESS1a;
static gsi_bool gotADDRESS1b;
static gsi_bool gotADDRESS2;
static gsi_bool gotADDRESS3;

static unsigned int GetLocalIP(void);
static unsigned short GetLocalPort(SOCKET sock);
static int Think(SOCKET sock, NAT *nat);

const char *AddressToString(unsigned int ip, unsigned short port, char string[22]) {
    static char strAddressArray[2][22];
    static int nIndex;
    char *strAddress;

    if (string != NULL) {
        strAddress = string;
    } else {
        nIndex ^= 1;
        strAddress = strAddressArray[nIndex];
    }
    if (ip != 0) {
        SOCInAddr inAddr;

        inAddr.addr = ip;
        if (port != 0) {
            OS_SPrintf(strAddress, "%s:%d", SOC_InetNtoA(inAddr), port);
        } else {
            OS_SPrintf(strAddress, "%s", SOC_InetNtoA(inAddr));
        }
    } else if (port != 0) {
        OS_SPrintf(strAddress, ":%d", port);
    } else {
        strAddress[0] = '\0';
    }
    return strAddress;
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

void OutputMapping(const AddressMapping *mapping) {
    if (mapping == NULL) {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "ERROR: Port mapping not available.");
    } else {
        gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment, "Discovered map: [%s] -> [%s]\n",
                      AddressToString(mapping->privateIp, mapping->privatePort, NULL),
                      AddressToString(mapping->publicIp, mapping->publicPort, NULL));
    }
}

static int Think(SOCKET sock, NAT *nat) {
    static char data[0x200];
    int length;
    gsi_u8 type;
    NatNegPacket packet;
    SOCSockAddrIn from;
    int fromSize = sizeof(SOCSockAddrIn);

    if (gotERT1 && gotERT2 && gotERT3 && gotADDRESS1a && gotADDRESS1b && gotADDRESS2 && gotADDRESS3) {
        return gsi_false;
    }
    if (sock != -1) {
        while (CanReceiveOnSocket(sock)) {
            length = recvfrom(sock, data, sizeof(data), 0, (SOCKADDR *)&from, &fromSize);
            if (length == -1) {
                gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice, "RECV SOCKET ERROR: %d\n",
                              GOAGetLastError(sock));
                break;
            }
            if (memcmp(data, NNMagicData, NATNEG_MAGIC_LEN) != 0) {
                return gsi_true;
            }
            type = data[7];
            if (length < 21) {
                return gsi_true;
            }
            if (type == 2) {
                // The packet's fields after the port type are not aligned on the wire
                memcpy(&packet, data, 21);
                memcpy(&packet.packet.init.localIp, &data[15], 4);
                memcpy(&packet.packet.init.localPort, &data[19], 2);
                switch (packet.packet.init.portType) {
                case 1:
                    gotERT1 = gsi_true;
                    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
                                  "(1) Got the solicited ERT from: %s\n",
                                  AddressToString(from.addr.addr, NN_SWAP16(from.port), NULL));
                    break;
                case 2:
                    nat->ipRestricted = gsi_false;
                    gotERT2 = gsi_true;
                    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
                                  "(2) Got the unsolicited (address) ERT from: %s\n",
                                  AddressToString(from.addr.addr, NN_SWAP16(from.port), NULL));
                    break;
                case 3:
                    nat->portRestricted = gsi_false;
                    gotERT3 = gsi_true;
                    gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
                                  "(3) Got the unsolicited (port) ERT from: %s\n",
                                  AddressToString(from.addr.addr, NN_SWAP16(from.port), NULL));
                    break;
                }
            } else if (type == 11) {
                memcpy(&packet, data, 21);
                memcpy(&packet.packet.init.localIp, &data[15], 4);
                memcpy(&packet.packet.init.localPort, &data[19], 2);
                packet.cookie = NN_SWAP32(packet.cookie);
                switch (packet.cookie) {
                case 0:
                    gotADDRESS1a = gsi_true;
                    break;
                case 3:
                    gotADDRESS1b = gsi_true;
                    break;
                case 1:
                    gotADDRESS2 = gsi_true;
                    break;
                case 2:
                    gotADDRESS3 = gsi_true;
                    break;
                }
                nat->mappings[packet.cookie].privateIp = GetLocalIP();
                nat->mappings[packet.cookie].privatePort = NN_SWAP16(GetLocalPort(sock));
                nat->mappings[packet.cookie].publicIp = packet.packet.init.localIp;
                nat->mappings[packet.cookie].publicPort = NN_SWAP16(packet.packet.init.localPort);
                gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
                              "Got ADDRESS REPLY %d from: %s\n", packet.cookie,
                              AddressToString(from.addr.addr, NN_SWAP16(from.port), NULL));
                OutputMapping(&nat->mappings[packet.cookie]);
            }
            if (sock == -1) {
                break;
            }
        }
    }
    return gsi_true;
}

int NatifyThink(SOCKET sock, NAT *nat) {
    return Think(sock, nat);
}

gsi_bool DetermineNatType(NAT *nat) {
    nat->natType = 6;
    nat->promiscuity = 4;
    nat->qr2Compatible = gsi_true;
    if (nat->mappings[0].publicIp == 0 || nat->mappings[1].publicIp == 0 || nat->mappings[2].publicIp == 0) {
        return gsi_false;
    }
    if (!nat->portRestricted && !nat->ipRestricted && nat->mappings[0].publicIp == nat->mappings[0].privateIp) {
        nat->natType = 0;
    } else if (nat->mappings[0].publicIp == nat->mappings[0].privateIp) {
        nat->natType = 1;
    } else if (!nat->ipRestricted && !nat->portRestricted &&
               abs(nat->mappings[2].publicPort - nat->mappings[1].publicPort) >= 1) {
        nat->natType = 5;
        nat->promiscuity = 0;
    } else if (nat->ipRestricted && !nat->portRestricted &&
               abs(nat->mappings[2].publicPort - nat->mappings[1].publicPort) >= 1) {
        nat->natType = 5;
        nat->promiscuity = 2;
    } else if (!nat->ipRestricted && nat->portRestricted &&
               abs(nat->mappings[2].publicPort - nat->mappings[1].publicPort) >= 1) {
        nat->natType = 5;
        nat->promiscuity = 3;
    } else if (nat->ipRestricted && nat->portRestricted &&
               abs(nat->mappings[2].publicPort - nat->mappings[1].publicPort) >= 1) {
        nat->natType = 5;
        nat->promiscuity = 1;
    } else if (nat->portRestricted) {
        nat->natType = 4;
    } else if (nat->ipRestricted && !nat->portRestricted) {
        nat->natType = 3;
    } else if (!nat->ipRestricted && !nat->portRestricted) {
        nat->natType = 2;
    } else {
        nat->natType = 6;
    }
    if (nat->mappings[0].publicPort == nat->mappings[0].privatePort &&
        nat->mappings[1].publicPort == nat->mappings[1].privatePort &&
        nat->mappings[2].publicPort == nat->mappings[2].privatePort) {
        nat->mappingScheme = 1;
    } else if (nat->mappings[0].publicPort == nat->mappings[1].publicPort &&
               nat->mappings[1].publicPort == nat->mappings[2].publicPort) {
        nat->mappingScheme = 2;
    } else if (nat->mappings[0].publicPort == nat->mappings[0].privatePort &&
               nat->mappings[2].publicPort - nat->mappings[1].publicPort == 1) {
        nat->mappingScheme = 4;
    } else if (nat->mappings[2].publicPort - nat->mappings[1].publicPort == 1) {
        nat->mappingScheme = 3;
    } else {
        nat->mappingScheme = 0;
    }
    if (nat->mappings[3].publicPort != 0 && nat->mappings[0].publicPort != nat->mappings[3].publicPort) {
        nat->qr2Compatible = gsi_false;
    }
    return gsi_true;
}
