#ifndef POKEBW2_DWC_QR2_H
#define POKEBW2_DWC_QR2_H

#include "dwc/gsPlatformSocket.h"
#include "dwc/nonport.h"
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
#define QR2_SWAP16(x) ((gsi_u16)((gsi_u8)((x) >> 8) | ((x) << 8 & 0xff00)))
#define QR2_SWAP32(x)                                                                                                  \
    ((gsi_u8)((gsi_u32)(x) >> 24) | ((gsi_u32)(x) >> 8 & 0xff00) | ((gsi_u32)(x) << 8 & 0xff0000) |                    \
     ((gsi_u32)(x) << 24 & 0xff000000))

#define QR2_MAX_DATA_SIZE 0x578
#define QR2_REQUEST_KEY_LEN 4
#define QR2_RECENT_CLIENT_MESSAGES_TO_TRACK 10
#define QR2_IPVERIFY_ARRAY_SIZE 200

// A packet as it is built: its bytes and how many are used
typedef struct qr2_implementation_s *qr2_t;
typedef struct qr2_keybuffer_s *qr2_keybuffer_t;
typedef struct qr2_buffer_s *qr2_buffer_t;

struct qr2_buffer_s {
    char buffer[QR2_MAX_DATA_SIZE];
    int len;
};

// The keys of a type that the game has values for
struct qr2_keybuffer_s {
    gsi_u8 keys[MAX_REGISTERED_KEYS];
    int numKeys;
};

// The types of keys, the later SDK's qr2_key_type
typedef enum {
    key_server,
    key_player,
    key_team,
    key_type_count,
} qr2_key_type;

// The errors that qr2_init_socketA and the add-error callback give
typedef enum {
    e_qrnoerror,
    e_qrwsockerror,
    e_qrbinderror,
    e_qrdnserror,
    e_qrconnerror,
    e_qrnochallengeerror,
    qr2_error_t_count
} qr2_error_t;

typedef void (*qr2_serverkeycallback_t)(int keyId, qr2_buffer_t outBuf, void *userData);
typedef void (*qr2_playerteamkeycallback_t)(int keyId, int index, qr2_buffer_t outBuf, void *userData);
typedef void (*qr2_keylistcallback_t)(qr2_key_type keyType, qr2_keybuffer_t keyBuffer, void *userData);
typedef int (*qr2_countcallback_t)(qr2_key_type keyType, void *userData);
typedef void (*qr2_adderrorcallback_t)(qr2_error_t error, gsi_char *errorMessage, void *userData);
typedef void (*qr2_natnegcallback_t)(int cookie, void *userData);
typedef void (*qr2_clientmessagecallback_t)(gsi_char *data, int len, void *userData);
typedef void (*qr2_publicaddresscallback_t)(unsigned int ip, unsigned short port, void *userData);
typedef void (*qr2_clientconnectedcallback_t)(SOCKET gameSocket, SOCKADDR_IN *remoteAddr, void *userData);
typedef void (*qr2_hostregisteredcallback_t)(void *userData);
typedef void (*cdkey_process_t)(char *data, int len, SOCKADDR *sender);

// A host that asked for an IP verify challenge, with the challenge and when it was sent (addr is 0 when unused)
struct qr2_ipverify_info_s {
    SOCKADDR_IN addr;
    gsi_u32 challenge;
    gsi_time createTime;
};

// A reporting game
struct qr2_implementation_s {
    SOCKET hbSock;
    char gamename[64];
    char secretKey[64];
    char instanceKey[QR2_REQUEST_KEY_LEN];
    qr2_serverkeycallback_t serverKeyCallback;
    qr2_playerteamkeycallback_t playerKeyCallback;
    qr2_playerteamkeycallback_t teamKeyCallback;
    qr2_keylistcallback_t keyListCallback;
    qr2_countcallback_t playerTeamCountCallback;
    qr2_adderrorcallback_t addErrorCallback;
    qr2_natnegcallback_t nnCallback;
    qr2_clientmessagecallback_t cmCallback;
    qr2_publicaddresscallback_t paCallback;
    qr2_clientconnectedcallback_t ccCallback;
    qr2_hostregisteredcallback_t crCallback;
    gsi_time lastHeartbeat;
    gsi_time lastKa;
    int userStateChangeRequested;
    int listedState; // 1 to 4 while it waits for the master server's challenge, -1 after an error
    int isPublic;
    int qport;
    int readSocket; // whether qr2_think reads the socket, which isn't the game's
    int natNegotiate;
    SOCKADDR_IN hbAddr;
    cdkey_process_t cdkeyProcess;
    int clientMessageKeys[QR2_RECENT_CLIENT_MESSAGES_TO_TRACK];
    int curMessageKey;
    unsigned int publicIp;
    unsigned short publicPort;
    void *userData;
    gsi_u8 backendOptions; // from the master server's challenge: 0x80 asks for IP verify challenges
    struct qr2_ipverify_info_s ipVerify[QR2_IPVERIFY_ARRAY_SIZE];
};

// The library's own reporting game, which the calls given NULL use
extern struct qr2_implementation_s static_qr2_rec;
// The master server's host to use instead of the game's, unless it is empty
extern char qr2_hostname[64];

// Starts reporting on a socket that the game made (so that the game reads it unless readSocket); a NULL qrec uses
// the library's own one
qr2_error_t qr2_init_socketA(qr2_t *qrec, SOCKET sock, int qport, const char *gamename, const char *secretKey,
                             int isPublic, int natNegotiate, qr2_serverkeycallback_t serverKeyCallback,
                             qr2_playerteamkeycallback_t playerKeyCallback, qr2_playerteamkeycallback_t teamKeyCallback,
                             qr2_keylistcallback_t keyListCallback, qr2_countcallback_t playerTeamCountCallback,
                             qr2_adderrorcallback_t addErrorCallback, void *userData);
void qr2_register_natneg_callback(qr2_t qrec, qr2_natnegcallback_t callback);
void qr2_register_clientmessage_callback(qr2_t qrec, qr2_clientmessagecallback_t callback);
void qr2_register_publicaddress_callback(qr2_t qrec, qr2_publicaddresscallback_t callback);
void qr2_think(qr2_t qrec);
void qr2_check_queries(qr2_t qrec);
void qr2_check_send_heartbeat(qr2_t qrec);
void qr2_send_statechanged(qr2_t qrec);
void qr2_shutdown(qr2_t qrec);
gsi_bool qr2_keybuffer_add(qr2_keybuffer_t keyBuffer, int keyId);
gsi_bool qr2_buffer_add_int(qr2_buffer_t outBuf, int value);
gsi_bool qr2_buffer_addA(qr2_buffer_t outBuf, const char *value);
// Takes a packet that came in on the game's socket
void qr2_parse_queryA(qr2_t qrec, char *query, int len, SOCKADDR *sender);

#endif // POKEBW2_DWC_QR2_H
