#ifndef POKEBW2_DWC_NATNEG_H
#define POKEBW2_DWC_NATNEG_H

#include "dwc/NATify.h"
#include "dwc/nonport.h"
#include "nitrowifi/soc.h"

// GameSpy's NAT negotiation (natneg.c), in overlay 11: two consoles behind NATs find each other's public address
// through the servers natneg1, natneg2 and natneg3 of the game's domain, which match them by a cookie that both give,
// and then ping each other until a packet gets through. The packets start with the six magic bytes, the protocol's
// version (4) and a type: 0 init, 1 its ack, 2 an ERT test, 3 its ack, 5 connect, 6 its ack, 7 a ping, 10 an address
// check, 11 its reply, 12 a NAT detection request, 13 a report of the result, 14 its ack, 15 the pre-init and 16 its
// ack.
//
// A negotiation's state goes 0 (the pre-init sent), 1 (its ack received), 2 (the init packets sent), 3 (all of them
// acked: waiting for the partner), 4 (pinging the partner), then 7 (the report sent), 8 (its ack received), 5
// (finished) and 6 (canceled). Its result is 0 (success), 1 (the partner never came), 2 (the servers never answered),
// 3 (the pings timed out), 4 (an unknown error) or 5 (no result yet); the errors of a start are 0 (none), 1 (out of
// memory), 2 (no socket, or the backend is down) and 3 (the servers' names can't be resolved)

// The byte order of the network
#define NN_SWAP16(x) ((gsi_u16)((gsi_u8)((x) >> 8) | ((x) << 8 & 0xff00)))
#define NN_SWAP32(x)                                                                                                   \
    ((((gsi_u32)(x) >> 24) & 0xff) | (((gsi_u32)(x) >> 8) & 0xff00) | (((gsi_u32)(x) << 8) & 0xff0000) |               \
     (((gsi_u32)(x) << 24) & 0xff000000))

#define NATNEG_MAGIC_LEN 6

// The packets' fields after the header, by type. The sizes sent are the header's 12 bytes and 9 for an init
// packet (and the game's name after them), 8 for a connect packet or a ping, 61 for a report and 6 for a pre-init
typedef struct _InitPacket {
    unsigned char portType; // 0 sent from the game's socket, 1 to 3 to each server from the negotiation's
    unsigned char clientIndex;
    unsigned char useGamePort;
    unsigned int localIp;
    unsigned short localPort;
} InitPacket;

typedef struct _ReportPacket {
    unsigned char portType;
    unsigned char clientIndex;
    unsigned char negResult; // whether it succeeded
    NatType natType;
    NatMappingScheme natMappingScheme;
    char gamename[50];
} ReportPacket;

typedef struct _ConnectPacket {
    unsigned int remoteIp;
    unsigned short remotePort;
    unsigned char gotYourData;
    unsigned char finished; // 0 while it goes on, else the reason it failed: 1 a deadbeat partner, 2 the init timed out
} ConnectPacket;

typedef struct PreInitPacket {
    gsi_u8 clientIndex;
    gsi_u8 state; // in the ack: 2 when the init packets can be sent
    gsi_u32 clientId;
} PreInitPacket;

typedef struct _NatNegPacket {
    unsigned char magic[NATNEG_MAGIC_LEN];
    unsigned char version;
    unsigned char packetType;
    int cookie;
    union {
        InitPacket init;
        ConnectPacket connect;
        ReportPacket report;
        PreInitPacket preInit;
    } packet;
} NatNegPacket;

typedef enum {
    ns_initsent,
    ns_initack,
    ns_connectping,
    ns_finished,
    ns_canceled,
    ns_reportsent,
    ns_reportack
} NegotiateState;

typedef enum {
    nr_success,
    nr_deadbeatpartner,
    nr_inittimeout,
    nr_pingtimeout,
    nr_unknownerror,
    nr_noresult
} NegotiateResult;

typedef enum { ne_noerror, ne_allocerror, ne_socketerror, ne_dnserror } NegotiateError;

typedef void (*NegotiateProgressFunc)(NegotiateState state, void *userData);
typedef void (*NegotiateCompletedFunc)(NegotiateResult result, SOCKET gameSocket, SOCKADDR_IN *remoteAddr,
                                       void *userData);
typedef void (*NatDetectionResultsFunc)(gsi_bool success, NAT nat);

extern unsigned char NNMagicData[NATNEG_MAGIC_LEN];

void NNFreeNegotiateList(void);
// Starts a negotiation that shares the game's socket, or uses its own when it is -1; the game passes the packets
// that come in on its socket with the magic bytes to NNProcessData
NegotiateError NNBeginNegotiationWithSocket(SOCKET gameSocket, int cookie, int clientIndex,
                                            NegotiateProgressFunc progress, NegotiateCompletedFunc completed,
                                            void *userData);
void NNCancel(int cookie);
void NNThink(void);
void NNProcessData(char *data, int length, SOCKADDR_IN *from);

#endif // POKEBW2_DWC_NATNEG_H
