#ifndef POKEBW2_DWC_SB_QUERYENGINE_H
#define POKEBW2_DWC_SB_QUERYENGINE_H

#include "types.h"
#include "dwc/sb_server.h"

// The query engine of GameSpy's server browser (sb_queryengine.c), in overlay 11: it sends the queries to the game
// servers of a list over UDP, a few at a time, and reads their answers into the servers

// The kinds of query: the keys that the browser asked for, all of them, or only the ping
#define QTYPE_BASIC 0
#define QTYPE_FULL 1
#define QTYPE_ICMP 2

// How long a server has to answer, in milliseconds
#define MAX_QUERY_MSEC 2500
#define MAX_QUERY_KEYS 40

typedef struct SBQueryEngine SBQueryEngine;

typedef enum SBQueryEngineCallbackReason {
    qe_updatesuccess,
    qe_updatefailed,
    qe_engineidle,
    qe_challengereceived
} SBQueryEngineCallbackReason;

typedef void (*SBEngineCallbackFn)(SBQueryEngine *engine, SBQueryEngineCallbackReason reason, SBServer *server,
                                   void *instance);

// A queue of servers, linked by their next fields
typedef struct SBServerFIFO {
    SBServer *first;
    SBServer *last;
    int count;
} SBServerFIFO;

struct SBQueryEngine {
    int queryversion;         // 1 for QR2's queries, else the old protocol's "\basic\" and "\status\"
    int maxupdates;           // how many servers are queried at once
    SBServerFIFO querylist;   // the servers being queried
    SBServerFIFO pendinglist; // the servers that wait their turn
    int querysock;
    int icmpsock;
    u32 mypublicip;
    u8 serverkeys[MAX_QUERY_KEYS]; // the keys of a basic query
    int numserverkeys;
    SBEngineCallbackFn ListCallback;
    void *instance;
};

// Starts the engine unless the game's services are down (a LAN browse starts it anyway)
void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, BOOL lanBrowse,
                       SBEngineCallbackFn callback, void *instance);
void SBQueryEngineSetPublicIP(SBQueryEngine *engine, u32 mypublicip);
void SBEngineHaltUpdates(SBQueryEngine *engine);
void SBEngineCleanup(SBQueryEngine *engine);
// Queries a server, at once or once a query ends, at the front of the queue or at its end
void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer *server, BOOL addfront, int querytype,
                               BOOL usequerychallenge);
void SBQueryEngineThink(SBQueryEngine *engine);
void SBQueryEngineAddQueryKey(SBQueryEngine *engine, u8 keyid);
void SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer *server);

#endif // POKEBW2_DWC_SB_QUERYENGINE_H
