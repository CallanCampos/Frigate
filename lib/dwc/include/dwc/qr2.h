#ifndef POKEBW2_DWC_QR2_H
#define POKEBW2_DWC_QR2_H

#include "types.h"
#include "dwc/qr2regkeys.h"
#include "nitrowifi/soc.h"

// GameSpy's query and reporting (qr2.c), in overlay 11: a game that hosts reports itself to the master server
// "<game>.master.gs.nintendowifi.net" with heartbeats, answers the queries of the server browsers (its keys and their
// values, for the server, its players and its teams) and gets the master server's messages, such as the cookies of
// the NAT negotiations that clients ask for. A query packet starts with 0xfe 0xfd, its type and the instance key: 0 a
// query, 1 the master server's challenge, 2 an echo, 4 an error of the listing, 6 a client message, 8 a keepalive, 9
// a request of an IP verify challenge and 10 a client registered. The errors are 0 (none), 1 (a socket error), 2 (a
// bind error), 3 (a DNS error), 4 (a connection error) and 5 (no challenge from the master server)

// The byte order of the network
#define QR2_SWAP16(x) ((u16)((u8)((x) >> 8) | ((x) << 8 & 0xff00)))
#define QR2_SWAP32(x)                                                                                                  \
    ((u8)((u32)(x) >> 24) | ((u32)(x) >> 8 & 0xff00) | ((u32)(x) << 8 & 0xff0000) | ((u32)(x) << 24 & 0xff000000))

#define QR2_MAX_DATA_SIZE 0x578
#define QR2_REQUEST_KEY_LEN 4
#define QR2_RECENT_CLIENT_MESSAGES_TO_TRACK 10
#define QR2_IPVERIFY_ARRAY_SIZE 200

// A packet as it is built: its bytes and how many are used
typedef struct QR2Buffer {
    char buffer[QR2_MAX_DATA_SIZE];
    int len;
} QR2Buffer;

// The keys of a type that the game has values for
typedef struct QR2KeyBuffer {
    u8 keys[MAX_REGISTERED_KEYS];
    int numKeys;
} QR2KeyBuffer;

// The types of keys, the later SDK's qr2_key_type
typedef enum QR2KeyType {
    key_server,
    key_player,
    key_team,
    key_type_count,
} QR2KeyType;

typedef void (*QR2ServerKeyCallback)(int keyId, QR2Buffer *outBuf, void *userData);
typedef void (*QR2PlayerTeamKeyCallback)(int keyId, int index, QR2Buffer *outBuf, void *userData);
typedef void (*QR2KeyListCallback)(QR2KeyType keyType, QR2KeyBuffer *keyBuffer, void *userData);
typedef int (*QR2CountCallback)(QR2KeyType keyType, void *userData);
typedef void (*QR2AddErrorCallback)(int error, const char *errorMessage, void *userData);
typedef void (*QR2NatNegCallback)(int cookie, void *userData);
typedef void (*QR2ClientMessageCallback)(const char *data, int len, void *userData);
typedef void (*QR2PublicAddressCallback)(u32 ip, u16 port, void *userData);
typedef void (*QR2ClientConnectedCallback)(int gameSocket, SocAddrIn *remoteAddr, void *userData);
typedef void (*QR2ClientRegisteredCallback)(void *userData);
typedef void (*QR2CdKeyProcess)(const char *data, int len, const SocAddrIn *sender);

// A host that asked for an IP verify challenge, with the challenge and when it was sent (addr is 0 when unused)
typedef struct QR2IPVerifyInfo {
    SocAddrIn addr;
    u32 challenge;
    u32 createTime;
} QR2IPVerifyInfo;

// A reporting game
typedef struct QR2Implementation {
    int hbSock;
    char gamename[64];
    char secretKey[64];
    char instanceKey[QR2_REQUEST_KEY_LEN];
    QR2ServerKeyCallback serverKeyCallback;
    QR2PlayerTeamKeyCallback playerKeyCallback;
    QR2PlayerTeamKeyCallback teamKeyCallback;
    QR2KeyListCallback keyListCallback;
    QR2CountCallback playerTeamCountCallback;
    QR2AddErrorCallback addErrorCallback;
    QR2NatNegCallback nnCallback;
    QR2ClientMessageCallback cmCallback;
    QR2PublicAddressCallback paCallback;
    QR2ClientConnectedCallback ccCallback;
    QR2ClientRegisteredCallback crCallback;
    u32 lastHeartbeat;
    u32 lastKa;
    BOOL userStateChangeRequested;
    int listedState; // 1 to 4 while it waits for the master server's challenge, -1 after an error
    BOOL isPublic;
    int qport;
    BOOL readSocket; // whether qr2_think reads the socket, which isn't the game's
    BOOL natNegotiate;
    SocAddrIn hbAddr;
    QR2CdKeyProcess cdkeyProcess;
    int clientMessageKeys[QR2_RECENT_CLIENT_MESSAGES_TO_TRACK];
    int curMessageKey;
    u32 publicIp;
    u16 publicPort;
    void *userData;
    u8 backendOptions; // from the master server's challenge: 0x80 asks for IP verify challenges
    QR2IPVerifyInfo ipVerify[QR2_IPVERIFY_ARRAY_SIZE];
} QR2Implementation;

// The library's own reporting game, which the calls given NULL use
extern QR2Implementation static_qr2_rec;
// The master server's host to use instead of the game's, unless it is empty
extern char qr2_hostname[64];

// Starts reporting on a socket that the game made (so that the game reads it unless readSocket); a NULL qrec uses
// the library's own one
int qr2_init_socketA(QR2Implementation **qrec, int sock, int qport, const char *gamename, const char *secretKey,
                     BOOL isPublic, BOOL natNegotiate, QR2ServerKeyCallback serverKeyCallback,
                     QR2PlayerTeamKeyCallback playerKeyCallback, QR2PlayerTeamKeyCallback teamKeyCallback,
                     QR2KeyListCallback keyListCallback, QR2CountCallback playerTeamCountCallback,
                     QR2AddErrorCallback addErrorCallback, void *userData);
void qr2_register_natneg_callback(QR2Implementation *qrec, QR2NatNegCallback callback);
void qr2_register_clientmessage_callback(QR2Implementation *qrec, QR2ClientMessageCallback callback);
void qr2_register_publicaddress_callback(QR2Implementation *qrec, QR2PublicAddressCallback callback);
void qr2_think(QR2Implementation *qrec);
void qr2_check_queries(QR2Implementation *qrec);
void qr2_check_send_heartbeat(QR2Implementation *qrec);
void qr2_send_statechanged(QR2Implementation *qrec);
void qr2_shutdown(QR2Implementation *qrec);
BOOL qr2_keybuffer_add(QR2KeyBuffer *keyBuffer, int keyId);
BOOL qr2_buffer_add_int(QR2Buffer *outBuf, int value);
BOOL qr2_buffer_addA(QR2Buffer *outBuf, const char *value);
// Takes a packet that came in on the game's socket
void qr2_parse_queryA(QR2Implementation *qrec, char *query, int len, SocAddrIn *sender);

#endif // POKEBW2_DWC_QR2_H
