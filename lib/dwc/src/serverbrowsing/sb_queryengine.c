#include <string.h>
// Before dwc/ext.h: MWCC builds the memcpy of the request key and of the challenge in place, as the ROM does, only
// when <string.h> declares memcpy first
#include "dwc/ext.h"
#include "dwc/qr2regkeys.h"
#include "dwc/sb_queryengine.h"
#include "dwc/sb_serverlist.h"

// The query engine of GameSpy's server browser (sb_queryengine.c). The ROM names neither the file nor its functions:
// the file and the functions are GameSpy's, as pret's pokerevo Wii build has them (sb_queryengine.s), and FIFOAddRear,
// FIFOAddFront, FIFOGetFirst, FIFOClear, ParseSingleGOAReply, ParseSingleICMPReply, TimeoutOldQueries and
// QueueNextQueries, which the Wii's build doesn't name, are named after the later SDK. The DS has no ICMP: a ping query
// sends nothing, and its socket is never opened

static void FIFOAddRear(SBServerFIFO *fifo, SBServer *server) {
    if (fifo->last != NULL) {
        fifo->last->next = server;
    }
    fifo->last = server;
    server->next = NULL;
    if (fifo->first == NULL) {
        fifo->first = server;
    }
    fifo->count++;
}

static void FIFOAddFront(SBServerFIFO *fifo, SBServer *server) {
    server->next = fifo->first;
    fifo->first = server;
    if (fifo->last == NULL) {
        fifo->last = server;
    }
    fifo->count++;
}

static SBServer *FIFOGetFirst(SBServerFIFO *fifo) {
    SBServer *hold = fifo->first;

    if (hold != NULL) {
        fifo->first = hold->next;
        if (fifo->first == NULL) {
            fifo->last = NULL;
        }
        fifo->count--;
    }
    return hold;
}

static gsi_bool FIFORemove(SBServerFIFO *fifo, SBServer *server) {
    SBServer *hold;
    SBServer *prev = NULL;

    hold = fifo->first;
    while (hold != NULL) {
        if (hold == server) {
            if (prev != NULL) {
                prev->next = hold->next;
            }
            if (fifo->first == hold) {
                fifo->first = hold->next;
            }
            if (fifo->last == hold) {
                fifo->last = prev;
            }
            fifo->count--;
            return gsi_true;
        }
        prev = hold;
        hold = hold->next;
    }
    return gsi_false;
}

static void FIFOClear(SBServerFIFO *fifo) {
    fifo->first = fifo->last = NULL;
    fifo->count = 0;
}

// Sends a server its query: QR2's challenge request, or its query of the basic keys or of all, or the old protocol's
static void QEStartQuery(SBQueryEngine *engine, SBServer *server) {
    gsi_u8 queryBuffer[256];
    int queryLen;
    gsi_bool querySuccess = gsi_false;
    SocAddrIn saddr;

    saddr.family = 2;
    server->updatetime = current_time();
    if (!(server->state & STATE_PENDINGICMPQUERY)) {
        if (engine->queryversion == 1) {
            if (server->state & STATE_PENDINGQUERYCHALLENGE) {
                int pos = 0;

                queryBuffer[pos++] = 0xfe;
                queryBuffer[pos++] = 0xfd;
                queryBuffer[pos++] = 9;
                memcpy(&queryBuffer[pos], &server->updatetime, 4);
                pos += 4;
                queryLen = pos;
            } else {
                gsi_u32 challengeNBO = SB_SWAP32(server->querychallenge);
                int pos = 0;

                queryBuffer[pos++] = 0xfe;
                queryBuffer[pos++] = 0xfd;
                queryBuffer[pos++] = 0;
                memcpy(&queryBuffer[pos], &server->updatetime, 4);
                pos += 4;
                if (challengeNBO != 0) {
                    memcpy(&queryBuffer[pos], &challengeNBO, 4);
                    pos += 4;
                }
                if (server->state & STATE_PENDINGBASICQUERY) {
                    int i;

                    queryBuffer[pos++] = engine->numserverkeys;
                    for (i = 0; i < engine->numserverkeys; i++) {
                        queryBuffer[pos++] = engine->serverkeys[i];
                    }
                    // No keys of the players or the teams
                    queryBuffer[pos++] = 0;
                    queryBuffer[pos++] = 0;
                    queryLen = pos;
                } else {
                    queryBuffer[pos++] = 0xff;
                    queryBuffer[pos++] = 0xff;
                    queryBuffer[pos++] = 0xff;
                    // The answer may come split in parts
                    queryBuffer[pos++] = 1;
                    queryLen = pos;
                }
            }
        } else if (server->state & STATE_PENDINGBASICQUERY) {
            memcpy(queryBuffer, "\\basic\\\\info\\", 13);
            queryLen = 13;
        } else {
            memcpy(queryBuffer, "\\status\\", 8);
            queryLen = 8;
        }
        if (server->publicip == engine->mypublicip && (server->flags & PRIVATE_IP_FLAG)) {
            saddr.addr.addr = server->privateip;
            saddr.port = server->privateport;
        } else {
            saddr.addr.addr = server->publicip;
            saddr.port = server->publicport;
        }
        sendto(engine->querysock, queryBuffer, queryLen, 0, &saddr, sizeof(saddr));
        querySuccess = gsi_true;
    }
    if (querySuccess) {
        FIFOAddRear(&engine->querylist, server);
    } else {
        server->updatetime = 0;
    }
}

void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, gsi_bool lanBrowse,
                       SBEngineCallbackFn callback, void *instance) {
    if (lanBrowse == gsi_false && __GSIACResult != 1) {
        return;
    }
    SocketStartUp();
    engine->queryversion = queryversion;
    engine->maxupdates = maxupdates;
    engine->numserverkeys = 0;
    engine->ListCallback = callback;
    engine->instance = instance;
    engine->mypublicip = 0;
    engine->querysock = gsiSocket(2, 2, 0);
    FIFOClear(&engine->pendinglist);
    FIFOClear(&engine->querylist);
}

void SBQueryEngineSetPublicIP(SBQueryEngine *engine, gsi_u32 mypublicip) {
    engine->mypublicip = mypublicip;
}

void SBEngineHaltUpdates(SBQueryEngine *engine) {
    FIFOClear(&engine->pendinglist);
    FIFOClear(&engine->querylist);
}

void SBEngineCleanup(SBQueryEngine *engine) {
    closesocket(engine->querysock);
    engine->querysock = -1;
    FIFOClear(&engine->pendinglist);
    FIFOClear(&engine->querylist);
}

void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer *server, gsi_bool addfront, int querytype,
                               gsi_bool usequerychallenge) {
    server->state &= ~(STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY | STATE_PENDINGICMPQUERY |
                       STATE_PENDINGQUERYCHALLENGE | STATE_QUERYFAILED);
    server->splitResponseBitmap = 0;
    server->querychallenge = 0;
    // No ICMP on the DS
    if (querytype == QTYPE_ICMP) {
        return;
    }
    if (querytype == QTYPE_BASIC) {
        server->state |= STATE_PENDINGBASICQUERY;
    } else if (querytype == QTYPE_FULL) {
        server->state |= STATE_PENDINGFULLQUERY;
    } else if (querytype == QTYPE_ICMP) {
        server->state |= STATE_PENDINGICMPQUERY;
    } else {
        return;
    }
    if (usequerychallenge && (querytype == QTYPE_FULL || querytype == QTYPE_BASIC)) {
        server->state |= STATE_PENDINGQUERYCHALLENGE;
    }
    if (engine->querylist.count < engine->maxupdates) {
        QEStartQuery(engine, server);
        return;
    }
    if (addfront) {
        FIFOAddFront(&engine->pendinglist, server);
    } else {
        FIFOAddRear(&engine->pendinglist, server);
    }
}

// QR2's answer of a server (0) or of its challenge (9), after the request key
static void ParseSingleQR2Reply(SBQueryEngine *engine, SBServer *server, char *data, int len) {
    int i;
    int dlen;

    if (data[0] != 0 && data[0] != 9) {
        return;
    }
    data += 5;
    len -= 5;
    if (server->state & STATE_PENDINGQUERYCHALLENGE) {
        server->state &= ~STATE_PENDINGQUERYCHALLENGE;
        if (len > 0) {
            // Queries the server again, with the challenge
            server->querychallenge = atol(data);
            FIFORemove(&engine->querylist, server);
            QEStartQuery(engine, server);
            engine->ListCallback(engine, qe_challengereceived, server, engine->instance);
            return;
        }
    } else if (server->state & STATE_PENDINGBASICQUERY) {
        for (i = 0; i < engine->numserverkeys; i++) {
            dlen = NTSLengthSB(data, len);
            if (dlen < 0) {
                break;
            }
            if (!qr2_internal_is_master_only_key(qr2_registered_key_list[engine->serverkeys[i]])) {
                SBServerAddKeyValue(server, qr2_registered_key_list[engine->serverkeys[i]], data);
            }
            data += dlen;
            len -= dlen;
        }
        server->state |= STATE_BASICKEYS | STATE_VALIDPING;
    } else if (*data != '\0' && strncmp("splitnum", data, 8) == 0) {
        SBServerParseQR2FullKeysSplit(server, data, len);
        if (server->splitResponseBitmap != 0xff) {
            return;
        }
        server->state |= STATE_FULLKEYS | STATE_BASICKEYS | STATE_VALIDPING;
    } else {
        SBServerParseQR2FullKeysSingle(server, data, len);
        server->state |= STATE_FULLKEYS | STATE_BASICKEYS | STATE_VALIDPING;
    }
    server->state &= ~(STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY);
    server->updatetime = current_time() - server->updatetime;
    FIFORemove(&engine->querylist, server);
    engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
}

// The old protocol's answer, which is complete once it has "\final\"
static void ParseSingleGOAReply(SBQueryEngine *engine, SBServer *server, char *data, int len) {
    gsi_bool isfinal = strstr(data, "\\final\\") != NULL;

    SBServerParseKeyVals(server, data);
    if (isfinal) {
        if (server->state & STATE_PENDINGBASICQUERY) {
            server->state |= STATE_BASICKEYS | STATE_VALIDPING;
        } else {
            server->state |= STATE_FULLKEYS | STATE_VALIDPING;
        }
        server->state &= ~(STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY);
        server->updatetime = current_time() - server->updatetime;
        FIFORemove(&engine->querylist, server);
        engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
    }
}

// The answer of a ping, which the DS doesn't send
static gsi_bool ParseSingleICMPReply(SBQueryEngine *engine, SBServer *server, char *data, int len) {
    return gsi_true;
}

static void ProcessIncomingReplies(SBQueryEngine *engine, gsi_bool icmpSocket) {
    int i;
    char indata[2048];
    SocAddrIn saddr;
    int saddrlen = sizeof(saddr);
    SBServer *server;
    int recvSock = 0;

    if (!icmpSocket) {
        recvSock = engine->querysock;
    }
    while (CanReceiveOnSocket(recvSock)) {
        i = recvfrom(recvSock, indata, sizeof(indata) - 1, 0, &saddr, &saddrlen);
        if (i == -1) {
            break;
        }
        indata[i] = '\0';
        for (server = engine->querylist.first; server != NULL; server = server->next) {
            if ((icmpSocket && (server->flags & ICMP_IP_FLAG) && server->icmpip == saddr.addr.addr) ||
                (server->publicip == saddr.addr.addr && (server->publicport == saddr.port || icmpSocket)) ||
                (server->publicip == engine->mypublicip && (server->flags & PRIVATE_IP_FLAG) &&
                 server->privateip == saddr.addr.addr && server->privateport == saddr.port)) {
                if (icmpSocket) {
                    // Several pings may wait on one address
                    if (ParseSingleICMPReply(engine, server, indata, i)) {
                        break;
                    }
                } else {
                    if (engine->queryversion == 1) {
                        ParseSingleQR2Reply(engine, server, indata, i);
                    } else {
                        ParseSingleGOAReply(engine, server, indata, i);
                    }
                    break;
                }
            }
        }
    }
}

static void TimeoutOldQueries(SBQueryEngine *engine) {
    gsi_u32 ctime = current_time();

    while (engine->querylist.first != NULL) {
        if (ctime > engine->querylist.first->updatetime + MAX_QUERY_MSEC) {
            // BUG: the bits of the query's state are set in and cleared from the server's flags, so the server
            // stays pending and its flags get a nonstandard port
#ifdef BUGFIX
            engine->querylist.first->state |= STATE_QUERYFAILED;
            engine->querylist.first->updatetime = MAX_QUERY_MSEC;
            engine->querylist.first->state &=
                ~(STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY | STATE_PENDINGICMPQUERY);
#else
            engine->querylist.first->flags |= STATE_QUERYFAILED;
            engine->querylist.first->updatetime = MAX_QUERY_MSEC;
            engine->querylist.first->flags &=
                ~(STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY | STATE_PENDINGICMPQUERY);
#endif
            engine->ListCallback(engine, qe_updatefailed, engine->querylist.first, engine->instance);
            FIFOGetFirst(&engine->querylist);
        } else {
            break;
        }
    }
}

static void QueueNextQueries(SBQueryEngine *engine) {
    while (engine->querylist.count < engine->maxupdates && engine->pendinglist.count > 0) {
        SBServer *server = FIFOGetFirst(&engine->pendinglist);

        QEStartQuery(engine, server);
    }
}

void SBQueryEngineThink(SBQueryEngine *engine) {
    if (engine->querylist.count == 0) {
        return;
    }
    ProcessIncomingReplies(engine, gsi_false);
    TimeoutOldQueries(engine);
    if (engine->pendinglist.count > 0) {
        QueueNextQueries(engine);
    }
    if (engine->querylist.count == 0) {
        engine->ListCallback(engine, qe_engineidle, NULL, engine->instance);
    }
}

void SBQueryEngineAddQueryKey(SBQueryEngine *engine, gsi_u8 keyid) {
    if (engine->numserverkeys < MAX_QUERY_KEYS) {
        engine->serverkeys[engine->numserverkeys++] = keyid;
    }
}

void SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer *server) {
    if (FIFORemove(&engine->querylist, server)) {
        return;
    }
    FIFORemove(&engine->pendinglist, server);
}
