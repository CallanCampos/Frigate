#ifndef POKEBW2_DWC_GPI_H
#define POKEBW2_DWC_GPI_H

#include "dwc/darray.h"
#include "dwc/gp.h"
#include "dwc/gsPlatformSocket.h"
#include "dwc/hashtable.h"
#include "dwc/nonport.h"

// The state of a connection of GameSpy's GP (gpi.c) and the layouts that the files of GP share, in overlay 11, after
// GameSpy's gpi.h and the headers of its gpi*.c (see dwc/gp.h for the names). The functions of each file are in its
// header: dwc/gpiBuddy.h, dwc/gpiBuffer.h and the others.

// The states of a connection: not connected, connecting to the connection manager, logging in, connected,
// disconnected, and deleting the profile
#define GPI_NOT_CONNECTED 0
#define GPI_CONNECTING 1
#define GPI_NEGOTIATING 2
#define GPI_CONNECTED 3
#define GPI_DISCONNECTED 4
#define GPI_PROFILE_DELETING 5

// The kinds of operation (gpiOperation.c), and their states: started, connecting (GPI_CONNECTING), logging in,
// requested, waiting for the answer, and finishing (answered)
#define GPI_CONNECT 0
#define GPI_NEW_PROFILE 1
#define GPI_GET_INFO 2
#define GPI_PROFILE_SEARCH 3
#define GPI_REGISTER_UNIQUENICK 4
#define GPI_DELETE_PROFILE 5
#define GPI_REGISTER_CDKEY 6

#define GPI_START 0
#define GPI_LOGIN 2
#define GPI_REQUESTING 3
#define GPI_WAITING 4
#define GPI_FINISHING 5

// The kinds of callback that wait to be called (gpiCallback.c), by what the call frees of their argument
#define GPI_ADD_NORMAL 0
#define GPI_ADD_ERROR 1
#define GPI_ADD_MESSAGE 2
#define GPI_ADD_NICKS 3
#define GPI_ADD_PMATCH 4
#define GPI_ADD_STATUS 5
#define GPI_ADD_BUDDDYREQUEST 6
#define GPI_ADD_TRANSFER_CALLBACK 7
#define GPI_ADD_REVERSE_BUDDIES 8
#define GPI_ADD_SUGGESTED_UNIQUE 9
#define GPI_ADD_BUDDYAUTH 10
#define GPI_ADD_BUDDYUTM 11
#define GPI_ADD_BUDDYREVOKE 12
#define GPI_ADD_REVERSE_BUDDIES_LIST 13
#define GPI_ADD_BUDDYKEYS 14
#define GPI_ADD_PROFILE_LIST 15

// The kinds of buddy message (gpiBuddy.c): a message, a buddy request, its reply, its authorization, a UTM, the
// revocation of a buddy, then the ones of the peers: a status, a game invite, a ping and its pong, a request for the
// keys of the status and its reply, and the file transfers from 200 on
#define GPI_BM_MESSAGE 1
#define GPI_BM_REQUEST 2
#define GPI_BM_REPLY 3
#define GPI_BM_AUTH 4
#define GPI_BM_UTM 5
#define GPI_BM_REVOKE 6
#define GPI_BM_STATUS 100
#define GPI_BM_INVITE 101
#define GPI_BM_PING 102
#define GPI_BM_PONG 103
#define GPI_BM_KEYS_REQUEST 104
#define GPI_BM_KEYS_REPLY 105
#define GPI_BM_FILE_SEND_REQUEST 200
#define GPI_BM_FILE_SEND_REPLY 201

// The states of a peer (gpiPeer.c): not connected, waiting for the server to give the profile's signature, given it,
// connecting, waiting for the other side's authorization, connected, and disconnected (to be removed)
#define GPI_PEER_NOT_CONNECTED 100
#define GPI_PEER_GETTING_SIG 101
#define GPI_PEER_GOT_SIG 102
#define GPI_PEER_CONNECTING 103
#define GPI_PEER_WAITING 104
#define GPI_PEER_CONNECTED 105
#define GPI_PEER_DISCONNECTED 106

typedef struct GPIBuddyStatus GPIBuddyStatus;
typedef struct GPIBuddyStatusInfo GPIBuddyStatusInfo;
typedef struct GPIInfoCache GPIInfoCache;
typedef struct GPIPeer GPIPeer;

// A text that grows as it is written to (gpiBuffer.c): len of size bytes are used, and pos is how far it was sent or
typedef enum _GPIBool { GPIFalse, GPITrue } GPIBool;

// read
typedef struct GPIBuffer {
    char *buffer;
    int size;
    int len;
    int pos;
} GPIBuffer;

// A callback of the app and its parameter, and one that waits to be called (gpiCallback.c), with the argument made for
// it, its kind (GPI_ADD_*) and the operation it ends
typedef struct GPICallback {
    GPCallback callback;
    void *param;
} GPICallback;

typedef struct GPICallbackData {
    GPICallback callback;
    void *arg;
    int type;
    int operationID;
    struct GPICallbackData *pnext;
} GPICallbackData;

// An operation in progress (gpiOperation.c), in the connection's list: its kind and state, its data (GPIConnectData,
// GPISearchData or nothing), the app's callback for its end, and the id of the server's messages that answer it
typedef struct GPIOperation {
    int type;
    void *data;
    GPIBool blocking;
    GPICallback callback;
    int state;
    int id;
    GPResult result;
    struct GPIOperation *pnext;
} GPIOperation;

// What a connect operation keeps (gpiConnect.c): the server's challenge and ours, the hash of the password, and the
// auth token and partner challenge of a login without a password
typedef struct GPIConnectData {
    char serverChallenge[128];
    char userChallenge[33];
    char passwordHash[33];
    char authtoken[256];
    char partnerchallenge[256];
    char cdkey[65];
    GPIBool newuser;
} GPIConnectData;

// A key of a status and its value (gpiKeys.c)
typedef struct GPIKey {
    char *keyName;
    char *keyValue;
} GPIKey;

// What the profiles keep (gpiProfile.c): the status of a buddy (from a "\bm\" status message), and the newer status
// info of one (from "\bsi\"), with the addresses of its game and the keys of its status
struct GPIBuddyStatus {
    int buddyIndex;
    GPEnum status;
    char *statusString;
    char *locationString;
    unsigned int ip;
    unsigned short port;
    GPEnum quietModeFlags;
};

struct GPIBuddyStatusInfo {
    int buddyIndex;
    GPEnum statusState;
    char *richStatus;
    char *gameType;
    char *gameVariant;
    char *gameMapName;
    unsigned int sessionFlags;
    unsigned int buddyIp;
    unsigned short buddyPort;
    unsigned int hostIp;
    unsigned int hostPrivateIp;
    unsigned short queryPort;
    unsigned short hostPort;
    GPEnum quietModeFlags;
    int productId;
    DArray extendedInfoKeys;
};

// The info of a profile that the server gave (gpiInfo.c), which the profile keeps when the connection caches info
struct GPIInfoCache {
    char *nick;
    char *uniquenick;
    char *email;
    char *firstname;
    char *lastname;
    char *homepage;
    int icquin;
    char zipcode[11];
    char countrycode[3];
    float longitude;
    float latitude;
    char place[128];
    int birthday;
    int birthmonth;
    int birthyear;
    GPEnum sex;
    GPEnum publicmask;
    char *aimname;
    int pic;
    int occupationid;
    int industryid;
    int incomeid;
    int marriedid;
    int childcount;
    int interests1;
    int ownership1;
    int conntypeid;
};

// A profile the connection knows of (gpiProfile.c): a buddy, a blocked player, one that asked to be a buddy or the
// player of a peer. authSig is the signature of a buddy request, and peerSig the one a peer authorizes with;
// buddyOrBlockCache keeps a cached profile when only buddies and blocked players are cached, and deleted drops the
// status messages of a buddy that was deleted
typedef struct GPIProfile {
    int profileId;
    int userId;
    GPIBuddyStatus *buddyStatus;
    GPIBuddyStatusInfo *buddyStatusInfo;
    GPIInfoCache *cache;
    char *authSig;
    int requestCount;
    char *peerSig;
    gsi_bool blocked;
    int blockIndex;
    gsi_bool buddyOrBlockCache;
    gsi_bool deleted;
} GPIProfile;

// The table of the profiles, by their ids, with how many there are, and how many buddies and blocked players
typedef struct GPIProfileList {
    HashTable profileTable;
    int num;
    int numBuddies;
    int numBlocked;
} GPIProfileList;

// An operation that a peer waits on (gpiPeer.c), with the app's callback and when it times out, and the queue of them
typedef struct GPIPeerOp {
    int type;
    void *userData;
    GPCallback callback;
    struct GPIPeerOp *next;
    int state;
    gsi_time timeout;
} GPIPeerOp;

typedef struct GPIPeerOpQueue {
    GPIPeerOp *first;
    GPIPeerOp *opList;
    GPIPeerOp *last;
} GPIPeerOpQueue;

// A message that waits to be sent to a peer: the text with its header, its kind and where its own text starts
typedef struct GPIMessage {
    GPIBuffer buffer;
    int type;
    int start;
} GPIMessage;

// What names a file transfer (gpiTransfer.c): the profile that sends it, a count and a time
typedef struct GPITransferID {
    int profileid;
    unsigned int count;
    unsigned int time;
} GPITransferID;

// A peer (gpiPeer.c): a buddy that messages go to directly, through the Udp Engine (dwc/gt2.h), in the connection's
// list. initiated is set on the side that connected
struct GPIPeer {
    int state;
    GPIBool initiated;
    unsigned int ip;
    unsigned short port;
    GPProfile profile;
    int timeout;
    int nackCount;
    GPIBuffer inputBuffer;
    GPIBuffer outputBuffer;
    DArray messages; // of GPIMessage
    GPIPeerOpQueue peerOpQueue;
    struct GPIPeer *pnext;
};

// A search of the search manager (gpiSearch.c): its kind, the socket and its buffers, what it looks for, and whether it
// is being processed or is to be removed. The last two are the "\profilelist\" search's, which GameSpy's GP doesn't
// have (the names are ours)
typedef struct GPISearchData {
    int type;
    SOCKET sock;
    GPIBuffer inputBuffer;
    GPIBuffer outputBuffer;
    char nick[31];
    char uniquenick[21];
    int namespaceIDs[16];
    int numNamespaces;
    char email[51];
    char firstname[31];
    char lastname[31];
    char password[31];
    char cdkey[65];
    int partnerID;
    int icquin;
    int skip;
    int productID;
    GPIBool processing;
    GPIBool remove;
    gsi_time searchStartTime;
    int *revBuddyProfileIds;
    int numOfRevBuddyProfiles;
    int searchProfileId;
    int maxResults;
} GPISearchData;

// The state of a connection, which GPConnection points to. The status info and the transfers aren't used here
typedef struct GPIConnection {
    char errorString[256];
    GPIBool infoCaching;
    GPIBool infoCachingBuddyAndBlockOnly;
    GPIBool simulation;
    GPIBool firewall;
    char nick[31];
    char uniquenick[21];
    char email[51];
    char password[31];
    int sessKey;
    int userid;
    int profileid;
    int partnerID;
    GPICallback callbacks[GPI_NUM_CALLBACKS];
    SOCKET cmSocket;
    int connectState;
    GPIBuffer socketBuffer;
    char *inputBuffer;
    int inputBufferSize;
    GPIBuffer outputBuffer;
    char mHeader[16]; // the header of the Udp Engine's messages
    unsigned short peerPort;
    int nextOperationID;
    int numSearches;
    GPEnum lastStatusState;
    unsigned int hostIp;
    unsigned int hostPrivateIp;
    unsigned short queryPort;
    unsigned short hostPort;
    unsigned int sessionFlags;
    char richStatus[256];
    char gameType[33];
    char gameVariant[33];
    char gameMapName[33];
    DArray extendedInfoKeys; // of GPIKey
    char lastStatusString[256];
    char lastLocationString[256];
    GPErrorCode errorCode;
    GPIBool fatalError;
    void *diskCache;
    GPIOperation *operationList;
    GPIProfileList profileList;
    GPIPeer *peerList;
    GPICallbackData *callbackList;
    GPICallbackData *lastCallback;
    GPIBuffer updateproBuffer;
    GPIBuffer updateuiBuffer;
    DArray transfers;
    unsigned int nextTransferID;
    int productID;
    int namespaceID;
    char loginTicket[GP_LOGIN_TICKET_LEN];
    GPEnum quietModeFlags;
    gsi_time kaTransmit; // when the last message was sent or received, for the keep-alives
} GPIConnection;

GPResult gpiInitialize(GPConnection *connection, int productID, int namespaceID, int partnerID);
void gpiDestroy(GPConnection *connection);
GPResult gpiReset(GPConnection *connection);
GPResult gpiProcessConnectionManager(GPConnection *connection);
GPResult gpiProcess(GPConnection *connection, int blockingOperationID);

#endif // POKEBW2_DWC_GPI_H
