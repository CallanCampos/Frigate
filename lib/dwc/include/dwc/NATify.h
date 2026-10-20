#ifndef POKEBW2_DWC_NATIFY_H
#define POKEBW2_DWC_NATIFY_H

#include "types.h"

// GameSpy's NAT detection (NATify.c), in overlay 11: the NAT negotiation code sends probes to its servers from two
// sockets, and the servers' answers (the ERT tests, which say whether an unsolicited packet comes through, and the
// address replies, which say where a packet was seen from) give the type of the NAT and how it maps ports.
//
// The NAT's type is 0 (no NAT), 1 (a firewall only), 2 (a full cone), 3 (a restricted cone), 4 (a port restricted
// cone), 5 (symmetric) or 6 (unknown); its promiscuity 0 (promiscuous), 1 (not), 2 (for ports), 3 (for addresses) or 4
// (not applicable); its mapping scheme 0 (unrecognized), 1 (the private port as the public one), 2 (a consistent
// port), 3 (incremental) or 4 (mixed)

// Where an address was seen from here and from the server, in network order but for the ports
typedef struct AddressMapping {
    u32 privateIp;
    u16 privatePort;
    u32 publicIp;
    u16 publicPort;
} AddressMapping;

// What was found of the NAT. The mappings are those of the address checks 0 (the first), 1 and 2 (to the other
// servers) and 3 (to the first again)
typedef struct NAT {
    char brand[32];
    char model[32];
    char firmware[64];
    BOOL ipRestricted;
    BOOL portRestricted;
    int promiscuity;
    int natType;
    int mappingScheme;
    AddressMapping mappings[4];
    BOOL qr2Compatible;
} NAT;

// "ip:port" (or just the ip or the port) in string, or in one of two buffers that it uses in turn when it is NULL
const char *AddressToString(u32 ip, u16 port, char *string);
void OutputMapping(const AddressMapping *mapping);
// Reads the servers' answers on a socket; returns FALSE once every answer has come
BOOL NatifyThink(int sock, NAT *nat);
// Works the NAT's type out from the answers; FALSE if the mappings it needs are missing
BOOL DetermineNatType(NAT *nat);

#endif // POKEBW2_DWC_NATIFY_H
