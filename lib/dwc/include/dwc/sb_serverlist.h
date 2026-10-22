#ifndef POKEBW2_DWC_SB_SERVERLIST_H
#define POKEBW2_DWC_SB_SERVERLIST_H

#include "dwc/darray.h"
#include "dwc/nonport.h"
#include "dwc/sb_crypt.h"
#include "dwc/sb_server.h"

// The list of GameSpy's server browser (sb_serverlist.c), in overlay 11: the servers that the master server of a game
// ("<game>.ms<n>.gs.nintendowifi.net", over TCP) sends, encrypted, with the keys that it knows of them, its messages
// after the list, the sort of the servers, and the servers of the local network

#define LIST_CHALLENGE_LEN 8
#define MAX_POPULAR_VALUES 255
#define SORTKEY_LENGTH 255
// The longest list of keys and filter of a request
#define MAX_FIELD_LIST_LEN 256
#define MAX_FILTER_LEN 511

// The options of a request to the master server
#define SEND_FIELDS_FOR_ALL 1
#define NO_SERVER_LIST 2
#define PUSH_UPDATES 4
#define SEND_GROUPS 32
#define NO_LIST_CACHE 64
#define LIMIT_RESULT_COUNT 128

// The games' flag that their servers want a challenge in the queries
#define QR2_USE_QUERY_CHALLENGE 128

typedef enum SBServerListState { sl_lanbrowse, sl_disconnected, sl_connected, sl_mainlist } SBServerListState;

// Where the reading of the list has got to
typedef enum SBListParseState {
    pi_cryptheader,
    pi_fixedheader,
    pi_keylist,
    pi_uniquevaluelist,
    pi_servers,
    pi_finished
} SBListParseState;

typedef enum SBError {
    sbe_noerror,
    sbe_socketerror,
    sbe_dnserror,
    sbe_connecterror,
    sbe_dataerror,
    sbe_allocerror,
    sbe_paramerror,
    sbe_duplicateupdateerror
} SBError;

typedef enum SBListCallbackReason {
    slc_serveradded,
    slc_serverupdated,
    slc_serverdeleted,
    slc_initiallistcomplete,
    slc_disconnected,
    slc_queryerror,
    slc_publicipdetermined,
    slc_serverchallengereceived
} SBListCallbackReason;

// How a sort compares a key: as a number, a float, a text, or a text without case
typedef enum SBCompareMode { sbcm_int, sbcm_float, sbcm_strcase, sbcm_stricase } SBCompareMode;

typedef void (*SBListCallBackFn)(SBServerList *serverlist, SBListCallbackReason reason, SBServer server,
                                 void *instance);
// The answer of a request for a server's map loop: the server, when the map changed, and the maps
typedef void (*SBMaploopCallbackFn)(SBServerList *serverlist, SBServer server, gsi_u32 mapChangeTime, int numMaps,
                                    char *mapList[], void *instance);
// One player that a search found (all NULL after the last): the nick, the server and the game
typedef void (*SBPlayerSearchCallbackFn)(SBServerList *serverlist, char *nick, goa_uint32 serverIP,
                                         unsigned short serverPort, gsi_u32 lastSeenTime, char *gamename,
                                         void *instance);

// A key that the master server sends for every server: its name and type (0 a text, 1 a byte, 2 a short)
typedef struct _KeyInfo {
    const char *keyName;
    int keyType;
} KeyInfo;

typedef struct _SortInfo {
    char sortkey[SORTKEY_LENGTH];
    SBCompareMode comparemode;
} SortInfo;

struct _SBServerList {
    SBServerListState state;
    DArray servers; // of SBServer
    DArray keylist; // of KeyInfo
    char queryforgamename[36];
    char queryfromgamename[36];
    char queryfromkey[32];
    char mychallenge[LIST_CHALLENGE_LEN];
    char *inbuffer;
    int inbufferlen;
    const char *popularvalues[MAX_POPULAR_VALUES]; // the texts that the servers' keys refer to by index
    int numpopularvalues;
    int expectedelements; // how many keys or texts are coming, or -1 before their count
    SBListCallBackFn ListCallback;
    SBMaploopCallbackFn MaploopCallback;
    SBPlayerSearchCallbackFn PlayerSearchCallback;
    void *instance;
    SortInfo currsortinfo;
    SortInfo prevsortinfo;
    SBBool sortascending;
    goa_uint32 mypublicip;
    goa_uint32 srcip;
    unsigned short defaultport;
    char *lasterror;
    SOCKET slsocket;
    gsi_time lanstarttime;
    int fromgamever;
    GOACryptState cryptkey;
    int queryoptions;
    SBListParseState pstate;
    gsi_u16 backendgameflags;
    const char *mLanAdapterOverride;
    SBServer deadlist; // the servers taken off the list, which SBListThink frees
};

// A host to ask instead of the game's master server, unless it is NULL
extern char *SBOverrideMasterServer;

void SBServerListSort(SBServerList *slist, SBBool ascending, SortInfo sortinfo);
// Adds a server, and tells the list's callback
void SBServerListAppendServer(SBServerList *slist, SBServer server);
// The index of a server, or of the server of an address, or -1
int SBServerListFindServer(SBServerList *slist, SBServer findserver);
int SBServerListFindServerByIP(SBServerList *slist, goa_uint32 ip, unsigned short port);
void SBServerListRemoveAt(SBServerList *slist, int index);
int SBServerListCount(SBServerList *slist);
SBServer SBServerListNth(SBServerList *slist, int i);
void SBFreeDeadList(SBServerList *slist);
void SBServerListClear(SBServerList *slist);
void SBAllocateServerList(SBServerList *slist);
// The shared copy of a text, which it counts, and its release (the list is unused)
const char *SBRefStr(SBServerList *slist, const char *str);
void SBReleaseStr(SBServerList *slist, const char *str);
// The length of the text at the start of a buffer, with its end, or -1 when it doesn't end in the buffer
int NTSLengthSB(char *buf, int len);
// Starts the list unless the game's services are down (a LAN browse starts it anyway)
void SBServerListInit(SBServerList *slist, const char *queryForGamename, const char *queryFromGamename,
                      const char *queryFromKey, int queryFromVersion, SBBool lanBrowse, SBListCallBackFn callback,
                      void *instance);
// Connects to the master server and asks for the servers that pass a filter, with the keys of a list of "\key"
SBError SBServerListConnectAndQuery(SBServerList *slist, const char *fieldList, const char *serverFilter, int options,
                                    int maxServers);
void SBServerListDisconnect(SBServerList *slist);
void SBServerListCleanup(SBServerList *slist);
void SBSetLastListErrorPtr(SBServerList *slist, char *theError);
// Sends a message through the master server to a server, and the cookie of a NAT negotiation
SBError SBSendMessageToServer(SBServerList *slist, goa_uint32 ip, unsigned short port, const char *data, int len);
SBError SBSendNatNegotiateCookieToServer(SBServerList *slist, goa_uint32 ip, unsigned short port, int cookie);
SBError SBListThink(SBServerList *slist);

#endif // POKEBW2_DWC_SB_SERVERLIST_H
