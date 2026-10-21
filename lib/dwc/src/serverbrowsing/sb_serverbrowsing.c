#include "dwc/sb_serverbrowsing.h"
#include "dwc/ext.h"
#include "dwc/natneg.h"
#include "dwc/qr2regkeys.h"

// GameSpy's server browser (sb_serverbrowsing.c). The ROM names neither the file nor its functions: the file and the
// functions are GameSpy's, as pret's pokerevo Wii build has them (sb_serverbrowsing.s), and ServerBrowserHalt, which
// the Wii's build doesn't name, is named after the later SDK

static void ListCallback(SBServerList *serverlist, SBListCallbackReason reason, SBServer *server, void *instance) {
    ServerBrowser *sb = instance;

    switch (reason) {
    case slc_serveradded:
        sb->BrowserCallback(sb, sbc_serveradded, server, sb->instance);
        // Queries a server that the list doesn't give all the keys of
        if ((server->state & (STATE_BASICKEYS | STATE_FULLKEYS)) == 0 || (server->state & STATE_VALIDPING) == 0) {
            if (server->state & (STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY | STATE_PENDINGICMPQUERY)) {
                break;
            }
            if (!sb->dontUpdate) {
                int qtype;

                if (server->flags & UNSOLICITED_UDP_FLAG) {
                    if (sb->list.state == sl_lanbrowse || sb->engine.numserverkeys == 0) {
                        qtype = QTYPE_FULL;
                    } else {
                        qtype = QTYPE_BASIC;
                    }
                } else {
                    qtype = QTYPE_ICMP;
                }
                if (serverlist->backendgameflags & QR2_USE_QUERY_CHALLENGE) {
                    SBQueryEngineUpdateServer(&sb->engine, server, gsi_false, qtype, gsi_true);
                } else {
                    SBQueryEngineUpdateServer(&sb->engine, server, gsi_false, qtype, gsi_false);
                }
            }
        }
        break;
    case slc_serverupdated:
        if ((server->state & (STATE_BASICKEYS | STATE_FULLKEYS | STATE_VALIDPING)) == 0) {
            sb->BrowserCallback(sb, sbc_serverupdatefailed, server, sb->instance);
        } else {
            sb->BrowserCallback(sb, sbc_serverupdated, server, sb->instance);
        }
        break;
    case slc_serverdeleted:
        if ((server->state & (STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY | STATE_PENDINGICMPQUERY)) != 0) {
            SBQueryEngineRemoveServerFromFIFOs(&sb->engine, server);
        }
        sb->BrowserCallback(sb, sbc_serverdeleted, server, sb->instance);
        break;
    case slc_initiallistcomplete:
        if (sb->disconnectFlag) {
            SBServerListDisconnect(serverlist);
        }
        if (func_ov011_02171588(serverlist->servers) == 0 || sb->engine.querylist.count == 0) {
            sb->BrowserCallback(sb, sbc_updatecomplete, NULL, sb->instance);
        }
        break;
    case slc_disconnected:
        break;
    case slc_queryerror:
        sb->BrowserCallback(sb, sbc_queryerror, NULL, sb->instance);
        break;
    case slc_publicipdetermined:
        SBQueryEngineSetPublicIP(&sb->engine, sb->list.mypublicip);
        break;
    case slc_serverchallengereceived:
        break;
    }
    if (server != NULL && server->publicip == sb->triggerIP && server->publicport == sb->triggerPort) {
        sb->triggerIP = 0;
    }
}

static void EngineCallback(SBQueryEngine *engine, SBQueryEngineCallbackReason reason, SBServer *server,
                           void *instance) {
    ServerBrowser *sb = instance;

    switch (reason) {
    case qe_updatefailed:
        sb->BrowserCallback(sb, sbc_serverupdatefailed, server, sb->instance);
        break;
    case qe_updatesuccess:
        sb->BrowserCallback(sb, sbc_serverupdated, server, sb->instance);
        break;
    case qe_engineidle:
        sb->BrowserCallback(sb, sbc_updatecomplete, server, sb->instance);
        break;
    case qe_challengereceived:
        sb->BrowserCallback(sb, sbc_serverchallengereceived, server, sb->instance);
        return;
    }
    if (server != NULL && server->publicip == sb->triggerIP && server->publicport == sb->triggerPort) {
        sb->triggerIP = 0;
    }
}

ServerBrowser *ServerBrowserNewA(const char *queryForGamename, const char *queryFromGamename, const char *queryFromKey,
                                 int queryFromVersion, int maxConcUpdates, int queryVersion, gsi_bool lanBrowse,
                                 ServerBrowserCallback callback, void *instance) {
    ServerBrowser *sb;

    if (lanBrowse == gsi_false && __GSIACResult != 1) {
        return NULL;
    }
    sb = gsimalloc(sizeof(ServerBrowser));
    if (sb == NULL) {
        return NULL;
    }
    sb->BrowserCallback = callback;
    sb->ConnectCallback = NULL;
    sb->instance = instance;
    sb->dontUpdate = gsi_false;
    SBServerListInit(&sb->list, queryForGamename, queryFromGamename, queryFromKey, queryFromVersion, lanBrowse,
                     ListCallback, sb);
    SBQueryEngineInit(&sb->engine, maxConcUpdates, queryVersion, lanBrowse, EngineCallback, sb);
    return sb;
}

void ServerBrowserFree(ServerBrowser *sb) {
    SBServerListCleanup(&sb->list);
    SBEngineCleanup(&sb->engine);
    if (sb->ConnectCallback != NULL) {
        NNFreeNegotiateList();
    }
    gsifree(sb);
}

SBError ServerBrowserBeginUpdate2(ServerBrowser *sb, gsi_bool async, gsi_bool disconnectOnComplete,
                                  const gsi_u8 *basicFields, int numBasicFields, const char *serverFilter,
                                  int updateOptions, int maxServers) {
    char keyList[MAX_FIELD_LIST_LEN] = "";
    int listLen = 0;
    int i;
    int keylen;
    SBError err;

    if (numBasicFields > MAX_QUERY_KEYS) {
        gsDebugFormat(4, 4, 2, "numBasicFields passed to ServerBrowserUpdate is too high!\r\n");
        return sbe_paramerror;
    }
    sb->disconnectFlag = disconnectOnComplete;
    sb->engine.numserverkeys = 0;
    for (i = 0; i < numBasicFields; i++) {
        const char *keyname = qr2_registered_key_list[basicFields[i]];

        keylen = strlen(keyname);
        if (listLen + keylen + 1 >= MAX_FIELD_LIST_LEN) {
            break;
        }
        listLen += OS_SPrintf(keyList + listLen, "\\%s", keyname);
        SBQueryEngineAddQueryKey(&sb->engine, basicFields[i]);
    }
    err = SBServerListConnectAndQuery(&sb->list, keyList, serverFilter, updateOptions, maxServers);
    if (err != sbe_noerror) {
        return err;
    }
    if (!async) {
        while (sb->list.state == sl_mainlist || (sb->engine.querylist.count > 0 && err == sbe_noerror)) {
            msleep(10);
            err = ServerBrowserThink(sb);
        }
    }
    return err;
}

SBError ServerBrowserLimitUpdateA(ServerBrowser *sb, gsi_bool async, gsi_bool disconnectOnComplete,
                                  const gsi_u8 *basicFields, int numBasicFields, const char *serverFilter,
                                  int maxServers) {
    return ServerBrowserBeginUpdate2(sb, async, disconnectOnComplete, basicFields, numBasicFields, serverFilter,
                                     LIMIT_RESULT_COUNT, maxServers);
}

SBError ServerBrowserSendMessageToServerA(ServerBrowser *sb, const char *ip, gsi_u16 port, const char *data, int len) {
    return SBSendMessageToServer(&sb->list, inet_addr(ip), SB_SWAP16(port), data, len);
}

SBError ServerBrowserSendNatNegotiateCookieToServerA(ServerBrowser *sb, const char *ip, gsi_u16 port, int cookie) {
    return SBSendNatNegotiateCookieToServer(&sb->list, inet_addr(ip), SB_SWAP16(port), cookie);
}

void ServerBrowserRemoveServer(ServerBrowser *sb, SBServer *server) {
    int i = SBServerListFindServer(&sb->list, server);

    if (i != -1) {
        SBServerListRemoveAt(&sb->list, i);
    }
}

SBError ServerBrowserThink(ServerBrowser *sb) {
    NNThink();
    SBQueryEngineThink(&sb->engine);
    return SBListThink(&sb->list);
}

void ServerBrowserHalt(ServerBrowser *sb) {
    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
}

void ServerBrowserClear(ServerBrowser *sb) {
    ServerBrowserHalt(sb);
    SBServerListClear(&sb->list);
}

SBState ServerBrowserState(ServerBrowser *sb) {
    if (sb->engine.querylist.count > 0) {
        return sb_querying;
    }
    if (sb->list.state == sl_mainlist || sb->list.state == sl_lanbrowse) {
        return sb_listxfer;
    }
    if (sb->list.state == sl_disconnected) {
        return sb_disconnected;
    }
    return sb_connected;
}

SBServer *ServerBrowserGetServer(ServerBrowser *sb, int index) {
    return SBServerListNth(&sb->list, index);
}

int ServerBrowserCount(ServerBrowser *sb) {
    return SBServerListCount(&sb->list);
}

void ServerBrowserSortA(ServerBrowser *sb, gsi_bool ascending, const char *sortkey, SBCompareMode comparemode) {
    SortInfo info;

    info.comparemode = comparemode;
    strcpy(info.sortkey, sortkey);
    SBServerListSort(&sb->list, ascending, info);
}

gsi_u32 ServerBrowserGetMyPublicIPAddr(ServerBrowser *sb) {
    return sb->list.mypublicip;
}
