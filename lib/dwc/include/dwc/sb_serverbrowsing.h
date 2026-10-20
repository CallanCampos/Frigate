#ifndef POKEBW2_DWC_SB_SERVERBROWSING_H
#define POKEBW2_DWC_SB_SERVERBROWSING_H

#include "types.h"
#include "dwc/sb_queryengine.h"
#include "dwc/sb_serverlist.h"

// GameSpy's server browser (sb_serverbrowsing.c), in overlay 11: a list of the game servers that a master server
// sends (dwc/sb_serverlist.h) and the engine that queries them (dwc/sb_queryengine.h), which DWC's matchmaking finds
// the other players' servers with

typedef struct ServerBrowser ServerBrowser;

typedef enum SBState { sb_disconnected, sb_listxfer, sb_querying, sb_connected } SBState;

typedef enum SBCallbackReason {
    sbc_serveradded,
    sbc_serverupdated,
    sbc_serverupdatefailed,
    sbc_serverdeleted,
    sbc_updatecomplete,
    sbc_queryerror,
    sbc_serverchallengereceived
} SBCallbackReason;

typedef void (*ServerBrowserCallback)(ServerBrowser *sb, SBCallbackReason reason, SBServer *server, void *instance);

struct ServerBrowser {
    SBQueryEngine engine;
    SBServerList list;
    BOOL disconnectFlag; // disconnects from the master server once the list is in
    BOOL dontUpdate;     // queries no servers as they are added
    u32 triggerIP;       // the address of a server that an update waits for
    u16 triggerPort;
    ServerBrowserCallback BrowserCallback;
    void *ConnectCallback; // set by a NAT negotiation's connection to a server, which the DS doesn't have
    void *instance;
};

// A browser of a game's servers, or NULL when the game's services are down (unless it browses the local network) or
// out of memory
ServerBrowser *ServerBrowserNewA(const char *queryForGamename, const char *queryFromGamename, const char *queryFromKey,
                                 int queryFromVersion, int maxConcUpdates, int queryVersion, BOOL lanBrowse,
                                 ServerBrowserCallback callback, void *instance);
void ServerBrowserFree(ServerBrowser *sb);
// Asks the master server for the servers that pass a filter, with the keys of a list of key numbers, and waits for
// them unless it is asynchronous
SBError ServerBrowserBeginUpdate2(ServerBrowser *sb, BOOL async, BOOL disconnectOnComplete, const u8 *basicFields,
                                  int numBasicFields, const char *serverFilter, int updateOptions, int maxServers);
SBError ServerBrowserLimitUpdateA(ServerBrowser *sb, BOOL async, BOOL disconnectOnComplete, const u8 *basicFields,
                                  int numBasicFields, const char *serverFilter, int maxServers);
SBError ServerBrowserSendMessageToServerA(ServerBrowser *sb, const char *ip, u16 port, const char *data, int len);
SBError ServerBrowserSendNatNegotiateCookieToServerA(ServerBrowser *sb, const char *ip, u16 port, int cookie);
void ServerBrowserRemoveServer(ServerBrowser *sb, SBServer *server);
SBError ServerBrowserThink(ServerBrowser *sb);
void ServerBrowserHalt(ServerBrowser *sb);
void ServerBrowserClear(ServerBrowser *sb);
SBState ServerBrowserState(ServerBrowser *sb);
SBServer *ServerBrowserGetServer(ServerBrowser *sb, int index);
int ServerBrowserCount(ServerBrowser *sb);
void ServerBrowserSortA(ServerBrowser *sb, BOOL ascending, const char *sortkey, SBCompareMode comparemode);
u32 ServerBrowserGetMyPublicIPAddr(ServerBrowser *sb);

#endif // POKEBW2_DWC_SB_SERVERBROWSING_H
