#ifndef POKEBW2_DWC_SB_SERVER_H
#define POKEBW2_DWC_SB_SERVER_H

#include "types.h"
#include "dwc/hashtable.h"

// A game server of GameSpy's server browser (sb_server.c), in overlay 11: its addresses, what the queries found out
// about it and the table of the keys and values it answered with. The texts of every table are shared, counted, in
// one table of the file (SBRefStr and SBReleaseStr of dwc/sb_serverlist.h)

// The byte order of the network, which the addresses and ports are kept in
#define SB_SWAP16(x) ((u16)((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00)))
#define SB_SWAP32(x)                                                                                                   \
    ((((u32)(x)) >> 24 & 0xff) | (((u32)(x)) >> 8 & 0xff00) | (((u32)(x)) << 8 & 0xff0000) |                           \
     (((u32)(x)) << 24 & 0xff000000))

typedef struct SBServer SBServer;
// The list of the servers, laid out in dwc/sb_serverlist.h
typedef struct SBServerList SBServerList;

// What the queries know of a server (its state)
#define STATE_BASICKEYS (1 << 0)
#define STATE_FULLKEYS (1 << 1)
#define STATE_PENDINGBASICQUERY (1 << 2)
#define STATE_PENDINGFULLQUERY (1 << 3)
#define STATE_QUERYFAILED (1 << 4)
#define STATE_PENDINGICMPQUERY (1 << 5)
#define STATE_VALIDPING (1 << 6)
#define STATE_PENDINGQUERYCHALLENGE (1 << 7)

// What the master server tells of a server (its flags)
#define UNSOLICITED_UDP_FLAG 1
#define PRIVATE_IP_FLAG 2
#define CONNECT_NEGOTIATE_FLAG 4
#define ICMP_IP_FLAG 8
#define NONSTANDARD_PORT_FLAG 16
#define NONSTANDARD_PRIVATE_PORT_FLAG 32
#define HAS_KEYS_FLAG 64
#define HAS_FULL_RULES_FLAG 128

// A key of a server and its value, both shared texts
typedef struct SBKeyValuePair {
    const char *key;
    const char *value;
} SBKeyValuePair;

// A shared text and how many hold it
typedef struct SBRefString {
    const char *str;
    int refcount;
} SBRefString;

// The addresses and ports are in network order. updatetime is when the query was sent, and its round trip (the ping)
// once it is answered
struct SBServer {
    u32 publicip;
    u16 publicport;
    u32 privateip;
    u16 privateport;
    u32 icmpip;
    u8 state;
    u8 flags;
    HashTable *keyvals; // of SBKeyValuePair
    u32 updatetime;
    u32 querychallenge;
    SBServer *next;
    u8 splitResponseBitmap; // the parts of a split answer that have come
};

// The server that stands for none, which the list's callbacks get for events of the list itself
extern SBServer *SBNullServer;

// The table of the shared texts, which the first call creates (the list is unused), and its end once it is empty
HashTable *SBRefStrHash(SBServerList *slist);
void SBRefStrHashCleanup(SBServerList *slist);
// Frees the server that an element of the list's array points to
void SBServerFree(void *elem);
void SBServerAddKeyValue(SBServer *server, const char *keyname, const char *value);
void SBServerAddIntKeyValue(SBServer *server, const char *keyname, int value);
// A key's value, or the default when the server doesn't have it (NULL for no server); the number of a value that
// starts with a digit, where "ping" gives the ping
const char *SBServerGetStringValueA(SBServer *server, const char *keyname, const char *def);
int SBServerGetIntValueA(SBServer *server, const char *key, int idefault);
double SBServerGetFloatValueA(SBServer *server, const char *key, double fdefault);
u32 SBServerGetPublicInetAddress(SBServer *server);
u16 SBServerGetPublicQueryPort(SBServer *server);
u16 SBServerGetPublicQueryPortNBO(SBServer *server);
BOOL SBServerHasPrivateAddress(SBServer *server);
u32 SBServerGetPrivateInetAddress(SBServer *server);
u16 SBServerGetPrivateQueryPort(SBServer *server);
void SBServerSetNext(SBServer *server, SBServer *next);
SBServer *SBServerGetNext(SBServer *server);
// Reads the answers of a server: the "\key\value" texts of the old protocol, and QR2's answer whole or in parts
void SBServerParseKeyVals(SBServer *server, char *keyvals);
void SBServerParseQR2FullKeysSingle(SBServer *server, char *data, int len);
void SBServerParseQR2FullKeysSplit(SBServer *server, char *data, int len);
int SBServerGetPing(SBServer *server);
// A new server with no keys, or NULL when out of memory (the list is unused)
SBServer *SBAllocServer(SBServerList *slist, u32 publicip, u16 publicport);
void SBServerSetFlags(SBServer *server, u8 flags);
void SBServerSetPrivateAddr(SBServer *server, u32 ip, u16 port);
void SBServerSetICMPIP(SBServer *server, u32 icmpip);
void SBServerSetState(SBServer *server, u8 state);
u8 SBServerGetState(SBServer *server);
BOOL SBIsNullServer(SBServer *server);

#endif // POKEBW2_DWC_SB_SERVER_H
