#ifndef POKEBW2_NITROWIFI_CPS_INTERNAL_H
#define POKEBW2_NITROWIFI_CPS_INTERNAL_H

#include "nitrowifi/cps.h"

// The byte order of the network, which is big-endian, and the frames the stack builds and reads. A received packet is
// read through the headers below; a packet to send is built backwards from where its data begins, each layer writing
// its header in front of the one above.

static inline u16 CPS_SWAP16(u16 x) {
    return (x >> 8) | (x << 8);
}
// A value of 16 or 32 bits in network order, at an address that is only 2-aligned
#define CPS_GET16(p) CPS_SWAP16(*(u16 *)(p))
#define CPS_GET32(p) (((u32)CPS_GET16(p) << 16) | CPS_GET16((u8 *)(p) + 2))
#define CPS_PUT16(p, v) (*(u16 *)(p) = CPS_SWAP16(v))
#define CPS_PUT32(p, v)                                                                                                \
    do {                                                                                                               \
        CPS_PUT16((p), (u16)((v) >> 16));                                                                              \
        CPS_PUT16((u8 *)(p) + 2, (u16)(v));                                                                            \
    } while (0)
// The same at an address of any alignment, byte by byte
#define CPS_BE16(p) ((u16)(((p)[0] << 8) | (p)[1]))
#define CPS_BE32(p) (((u32)CPS_BE16(p) << 16) | CPS_BE16((p) + 2))
#define CPS_PUTBE16(p, v)                                                                                              \
    do {                                                                                                               \
        (p)[0] = (v) >> 8;                                                                                             \
        (p)[1] = (v);                                                                                                  \
    } while (0)
#define CPS_PUTBE32(p, v)                                                                                              \
    do {                                                                                                               \
        CPS_PUTBE16((p), (u16)((v) >> 16));                                                                            \
        CPS_PUTBE16((p) + 2, (u16)(v));                                                                                \
    } while (0)

#define CPS_LOOPBACK_ADDR 0x7f000001
#define CPS_BROADCAST_ADDR 0xffffffff

#define CPS_ETHERTYPE_IP 0x800
#define CPS_ETHERTYPE_ARP 0x806

#define CPS_PROTO_ICMP 1
#define CPS_PROTO_TCP 6
#define CPS_PROTO_UDP 17

// The most an IP packet carries past its header
#define CPS_IP_MTU 1480

// The stack's clock, in units of 65536 ticks (about 0.125 s)
#define CPS_NOW() ((s32)(clock() >> 16))
// How long an ACK may wait for more data to go with it
#define CPS_ACK_DELAY OS_MilliSecondsToTicks(200)

// TCP's flags
#define CPS_TCP_FIN 0x01
#define CPS_TCP_SYN 0x02
#define CPS_TCP_RST 0x04
#define CPS_TCP_PSH 0x08
#define CPS_TCP_ACK 0x10
#define CPS_TCP_URG 0x20

// Where a packet's data begins in a socket's send buffer: past the Ethernet and IP headers
#define CPS_SNDBUF_HEADER (sizeof(CPSEtherHeader) + sizeof(CPSIpHeader))

typedef struct CPSEtherHeader {
    u8 dst[6];
    u8 src[6];
    u16 type;
} CPSEtherHeader;

typedef struct CPSArpPacket {
    u16 hardware;
    u16 protocol;
    u16 lengths;
    u16 op;
    u8 src_mac[6];
    u16 src_ip[2];
    u8 dst_mac[6];
    u16 dst_ip[2];
} CPSArpPacket;

typedef struct CPSIpHeader {
    u8 ver_len;
    u8 tos;
    u16 len;
    u16 id;
    u16 frag;
    u8 ttl;
    u8 proto;
    u16 sum;
    u16 src[2];
    u16 dst[2];
} CPSIpHeader;

typedef struct CPSIcmpHeader {
    u8 type;
    u8 code;
    u16 sum;
    u16 id;
    u16 seq;
} CPSIcmpHeader;

typedef struct CPSUdpHeader {
    u16 src_port;
    u16 dst_port;
    u16 len;
    u16 sum;
} CPSUdpHeader;

typedef struct CPSTcpHeader {
    u16 src_port;
    u16 dst_port;
    u16 seq[2];
    u16 ack[2];
    u8 offset; // The header's length in words, in the high nibble
    u8 flags;
    u16 window;
    u16 sum;
    u16 urgent;
} CPSTcpHeader;

// The length of a TCP header, and its data
#define CPS_TCP_HEADER_LEN(tcp) ((u32)((tcp)->offset & 0xf0) >> 2)
#define CPS_TCP_DATA(tcp) ((u8 *)(tcp) + CPS_TCP_HEADER_LEN(tcp))

// One entry of the ARP cache. time is when it was last used, in units of 65536 ticks
typedef struct CPSArpEntry {
    CPSInAddr ip;
    u8 mac[6];
    u16 time;
} CPSArpEntry;

// An IP packet being put together from its fragments: where each fragment begins and ends, in units of 8 bytes, and
// the end of the last one once it came
typedef struct CPSReassembly {
    CPSInAddr src;
    u16 count;
    u16 id;
    u16 last;
    u16 len;
    u16 start[8];
    u16 end[8];
    s32 time;
    u8 *data;
    u8 *buf;
} CPSReassembly;

#endif // POKEBW2_NITROWIFI_CPS_INTERNAL_H
