#include <string.h>
// Before dwc/ext.h: MWCC builds the memcpys of the addresses, ports and lengths in place, as the ROM does, only when
// <string.h> declares memcpy first
#include "dwc/ext.h"
#include "dwc/sb_serverlist.h"

// The list of GameSpy's server browser (sb_serverlist.c). The ROM names neither the file nor its functions: the file
// and the functions are GameSpy's, as pret's pokerevo Wii build has them (sb_serverlist.s), and the ones that the
// Wii's build doesn't name are named after the later SDK (SBServerListAppendServer, SBServerListFindServerByIP,
// AddServerToDeadlist, SBFreeDeadList, SBAllocateServerList, ErrorDisconnect, the Buffer* functions,
// SetupListChallenge, FreePopularValues, FreeKeyList, InitCryptKey, ServerSizeForFlags, ParseServerIPPort,
// SBSetLastListErrorPtr and ProcessDeleteServer), as are SBOverrideMasterServer, g_sortserverlist and
// ErrorDisconnect's QUERY_ERROR; ListStringHash, the later SDK's StringHash, is ours, since sb_server.c has its own
// StringHash

// The master servers' port, and how many master servers a game's name picks from
#define MSPORT2 28910
#define NUM_MASTER_SERVERS 20
#define INCOMING_BUFFER_SIZE 4096
#define MAX_MAPLOOP_LENGTH 16

// The messages of a request: the list, and a message for a server
#define SERVER_LIST_REQUEST 0
#define SEND_MESSAGE_REQUEST 2
#define LIST_PROTOCOL_VERSION 1
#define LIST_ENCODING_VERSION 3
// An option of the request: the source address of the player follows
#define ALTERNATE_SOURCE_IP 8

// The messages that the master server sends after the list
#define PUSH_KEYS_MESSAGE 1
#define PUSH_SERVER_MESSAGE 2
#define KEEPALIVE_MESSAGE 3
#define DELETE_SERVER_MESSAGE 4
#define MAPLOOP_MESSAGE 5
#define PLAYERSEARCH_MESSAGE 6

#define KEYTYPE_STRING 0
#define KEYTYPE_BYTE 1
#define KEYTYPE_SHORT 2

// The size of the master server's address and port of the player
#define FIXED_HEADER_LEN 6
// How long the servers of the local network have to answer, in milliseconds
#define SL_LAN_SEARCH_TIME 2000

char *SBOverrideMasterServer;
// The list that a sort sorts, which its compare functions read the keys from
static SBServerList *g_sortserverlist;

static int prevKeyCompare(SBServer *server1, SBServer *server2);

// Compares two servers by the key of the sort before, which keeps the order of the servers that the key of this one
// can't tell apart
static int prevKeyCompare(SBServer *server1, SBServer *server2) {
    const char *prevsortkey = g_sortserverlist->prevsortinfo.sortkey;
    int diff;
    double f;

    switch (g_sortserverlist->prevsortinfo.comparemode) {
    case sbcm_int:
        diff = SBServerGetIntValueA(server1, prevsortkey, 0) - SBServerGetIntValueA(server2, prevsortkey, 0);
        break;
    case sbcm_float:
        f = SBServerGetFloatValueA(server1, prevsortkey, 0) - SBServerGetFloatValueA(server2, prevsortkey, 0);
        if (!g_sortserverlist->sortascending) {
            f = -f;
        }
        if ((float)f > 0.0f) {
            return 1;
        } else if ((float)f < 0.0f) {
            return -1;
        } else {
            return 0;
        }
    case sbcm_strcase:
        diff = strcmp(SBServerGetStringValueA(server1, prevsortkey, ""),
                      SBServerGetStringValueA(server2, prevsortkey, ""));
        break;
    case sbcm_stricase:
        diff = strcmpi(SBServerGetStringValueA(server1, prevsortkey, ""),
                       SBServerGetStringValueA(server2, prevsortkey, ""));
        break;
    default:
        return 0;
    }
    if (!g_sortserverlist->sortascending) {
        diff = -diff;
    }
    return diff;
}

static int IntKeyCompare(const void *entry1, const void *entry2) {
    SBServer *server1 = *(SBServer **)entry1;
    SBServer *server2 = *(SBServer **)entry2;
    int diff;
    const char *currsortkey = g_sortserverlist->currsortinfo.sortkey;

    diff = SBServerGetIntValueA(server1, currsortkey, 0) - SBServerGetIntValueA(server2, currsortkey, 0);
    if (diff == 0) {
        return prevKeyCompare(server1, server2);
    }
    if (!g_sortserverlist->sortascending) {
        diff = -diff;
    }
    return diff;
}

static int FloatKeyCompare(const void *entry1, const void *entry2) {
    SBServer *server1 = *(SBServer **)entry1;
    SBServer *server2 = *(SBServer **)entry2;
    double f;
    const char *currsortkey = g_sortserverlist->currsortinfo.sortkey;

    f = SBServerGetFloatValueA(server1, currsortkey, 0) - SBServerGetFloatValueA(server2, currsortkey, 0);
    if (!((float)f > 0.0f) && !((float)f < 0.0f)) {
        return prevKeyCompare(server1, server2);
    }
    if (!g_sortserverlist->sortascending) {
        f = -f;
    }
    if ((float)f > 0.0f) {
        return 1;
    } else if ((float)f < 0.0f) {
        return -1;
    } else {
        return 0;
    }
}

static int StrCaseKeyCompare(const void *entry1, const void *entry2) {
    SBServer *server1 = *(SBServer **)entry1;
    SBServer *server2 = *(SBServer **)entry2;
    int diff;
    const char *currsortkey = g_sortserverlist->currsortinfo.sortkey;

    diff = strcmp(SBServerGetStringValueA(server1, currsortkey, ""), SBServerGetStringValueA(server2, currsortkey, ""));
    if (diff == 0) {
        return prevKeyCompare(server1, server2);
    }
    if (!g_sortserverlist->sortascending) {
        diff = -diff;
    }
    return diff;
}

static int StrNoCaseKeyCompare(const void *entry1, const void *entry2) {
    SBServer *server1 = *(SBServer **)entry1;
    SBServer *server2 = *(SBServer **)entry2;
    int diff;
    const char *currsortkey = g_sortserverlist->currsortinfo.sortkey;

    diff =
        strcmpi(SBServerGetStringValueA(server1, currsortkey, ""), SBServerGetStringValueA(server2, currsortkey, ""));
    if (diff == 0) {
        return prevKeyCompare(server1, server2);
    }
    if (!g_sortserverlist->sortascending) {
        diff = -diff;
    }
    return diff;
}

void SBServerListSort(SBServerList *slist, BOOL ascending, SortInfo sortinfo) {
    DArrayCompareFunc comparator;

    switch (sortinfo.comparemode) {
    case sbcm_int:
        comparator = IntKeyCompare;
        break;
    case sbcm_float:
        comparator = FloatKeyCompare;
        break;
    case sbcm_strcase:
        comparator = StrCaseKeyCompare;
        break;
    case sbcm_stricase:
        comparator = StrNoCaseKeyCompare;
        break;
    default:
        comparator = StrNoCaseKeyCompare;
    }
    // The key of this sort becomes the one before, unless it is the same
    if (strlen(slist->prevsortinfo.sortkey) == 0) {
        slist->prevsortinfo = sortinfo;
    } else if (strcmp(sortinfo.sortkey, slist->currsortinfo.sortkey) != 0) {
        slist->prevsortinfo = slist->currsortinfo;
    }
    slist->currsortinfo = sortinfo;
    slist->sortascending = ascending;
    g_sortserverlist = slist;
    ArraySort(slist->servers, comparator);
}

void SBServerListAppendServer(SBServerList *slist, SBServer *server) {
    func_ov011_021715a4(slist->servers, &server);
    slist->ListCallback(slist, slc_serveradded, server, slist->instance);
}

int SBServerListFindServer(SBServerList *slist, SBServer *findserver) {
    int numservers = func_ov011_02171588(slist->servers);
    int i;

    for (i = 0; i < numservers; i++) {
        if (findserver == *(SBServer **)func_ov011_0217158c(slist->servers, i)) {
            return i;
        }
    }
    return -1;
}

int SBServerListFindServerByIP(SBServerList *slist, u32 ip, u16 port) {
    int numservers;
    SBServer *server;
    int i;

    numservers = func_ov011_02171588(slist->servers);
    for (i = 0; i < numservers; i++) {
        server = *(SBServer **)func_ov011_0217158c(slist->servers, i);
        if (SBServerGetPublicInetAddress(server) == ip && SBServerGetPublicQueryPortNBO(server) == port) {
            return i;
        }
    }
    return -1;
}

// The servers taken off the list wait in a list of their own until SBListThink frees them
static void AddServerToDeadlist(SBServerList *slist, SBServer *server) {
    if (slist->deadlist == NULL) {
        SBServerSetNext(server, NULL);
    } else {
        SBServerSetNext(server, slist->deadlist);
    }
    slist->deadlist = server;
}

void SBServerListRemoveAt(SBServerList *slist, int index) {
    SBServer *server = *(SBServer **)func_ov011_0217158c(slist->servers, index);

    slist->ListCallback(slist, slc_serverdeleted, server, slist->instance);
    func_ov011_02171668(slist->servers, index);
    AddServerToDeadlist(slist, server);
}

int SBServerListCount(SBServerList *slist) {
    return func_ov011_02171588(slist->servers);
}

SBServer *SBServerListNth(SBServerList *slist, int i) {
    return *(SBServer **)func_ov011_0217158c(slist->servers, i);
}

void SBFreeDeadList(SBServerList *slist) {
    SBServer *server;
    SBServer *next;

    if (slist->deadlist == NULL) {
        return;
    }
    server = slist->deadlist;
    while (server != NULL) {
        next = SBServerGetNext(server);
        SBServerFree(&server);
        server = next;
    }
    slist->deadlist = NULL;
}

void SBServerListClear(SBServerList *slist) {
    int i;
    int nservers = func_ov011_02171588(slist->servers);

    for (i = 0; i < nservers; i++) {
        AddServerToDeadlist(slist, *(SBServer **)func_ov011_0217158c(slist->servers, i));
    }
    ArrayClear(slist->servers);
    SBFreeDeadList(slist);
}

void SBAllocateServerList(SBServerList *slist) {
    slist->servers = func_ov011_02171514(sizeof(SBServer *), 100, NULL);
    slist->deadlist = NULL;
}

const char *SBRefStr(SBServerList *slist, const char *str) {
    SBRefString ref;
    SBRefString *val;

    ref.str = str;
    val = TableLookup(SBRefStrHash(slist), &ref);
    if (val != NULL) {
        val->refcount++;
        return val->str;
    }
    ref.str = goastrdup(str);
    ref.refcount = 1;
    TableEnter(SBRefStrHash(slist), &ref);
    return ref.str;
}

void SBReleaseStr(SBServerList *slist, const char *str) {
    SBRefString ref;
    SBRefString *val;

    ref.str = str;
    val = TableLookup(SBRefStrHash(slist), &ref);
    if (val == NULL) {
        return;
    }
    val->refcount--;
    if (val->refcount == 0) {
        TableRemove(SBRefStrHash(slist), &ref);
    }
}

int NTSLengthSB(char *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (buf[i] == '\0') {
            return i + 1;
        }
    }
    return -1;
}

void SBServerListInit(SBServerList *slist, const char *queryForGamename, const char *queryFromGamename,
                      const char *queryFromKey, int queryFromVersion, BOOL lanBrowse, SBListCallBackFn callback,
                      void *instance) {
    if (lanBrowse == FALSE && __GSIACResult != 1) {
        return;
    }
    slist->state = sl_disconnected;
    SBAllocateServerList(slist);
    SBRefStrHash(slist);
    gsiSafeStrcpyA(slist->queryforgamename, queryForGamename, sizeof(slist->queryforgamename));
    gsiSafeStrcpyA(slist->queryfromgamename, queryFromGamename, sizeof(slist->queryfromgamename));
    gsiSafeStrcpyA(slist->queryfromkey, queryFromKey, sizeof(slist->queryfromkey));
    slist->ListCallback = callback;
    slist->MaploopCallback = NULL;
    slist->instance = instance;
    slist->mypublicip = 0;
    slist->slsocket = -1;
    slist->inbuffer = NULL;
    slist->inbufferlen = 0;
    slist->keylist = NULL;
    slist->expectedelements = -1;
    slist->numpopularvalues = 0;
    slist->srcip = 0;
    slist->fromgamever = queryFromVersion;
    strcpy(slist->currsortinfo.sortkey, "");
    strcpy(slist->prevsortinfo.sortkey, "");
    SBSetLastListErrorPtr(slist, "");
    slist->mLanAdapterOverride = NULL;
    slist->backendgameflags = QR2_USE_QUERY_CHALLENGE;
    srand(current_time());
    SocketStartUp();
}

// Tells the list's callback of the master server's error text, if it sent one, and that the list is disconnected
static void ErrorDisconnect(SBServerList *slist) {
    static char *QUERY_ERROR = "Query Error: ";

    if (slist->inbufferlen > 0 && slist->inbufferlen > strlen(QUERY_ERROR) &&
        strncmp(slist->inbuffer, QUERY_ERROR, strlen(QUERY_ERROR)) == 0) {
        SBSetLastListErrorPtr(slist, slist->inbuffer + strlen(QUERY_ERROR));
        slist->ListCallback(slist, slc_queryerror, SBNullServer, slist->instance);
    }
    slist->ListCallback(slist, slc_disconnected, SBNullServer, slist->instance);
    SBServerListDisconnect(slist);
}

// The hash of a game's name, which picks its master server
static int ListStringHash(const char *s, int numbuckets) {
    u32 hashcode = 0;

    while (*s != '\0') {
        hashcode = hashcode * 0x9ccf9319 + tolower(*s);
        s++;
    }
    return hashcode % numbuckets;
}

static SBError ServerListConnect(SBServerList *slist) {
    SocAddrIn saddr;
    SocHostEnt *hent;
    char masterHostname[128];
    int masterIndex;

    masterIndex = ListStringHash(slist->queryforgamename, NUM_MASTER_SERVERS);
    if (SBOverrideMasterServer != NULL) {
        gsiSafeStrcpyA(masterHostname, SBOverrideMasterServer, sizeof(masterHostname));
    } else {
        OS_SPrintf(masterHostname, "%s.ms%d.gs.nintendowifi.net", slist->queryforgamename, masterIndex);
    }
    saddr.family = 2;
    saddr.port = SB_SWAP16(MSPORT2);
    saddr.addr = inet_addr(masterHostname);
    if (saddr.addr == -1) {
        hent = SOC_GetHostByName(masterHostname);
        if (hent == NULL) {
            return sbe_dnserror;
        }
        memcpy(&saddr.addr, hent->addrList[0], sizeof(saddr.addr));
    }
    if (slist->slsocket == -1) {
        slist->slsocket = gsiSocket(2, 1, 0);
        if (slist->slsocket == -1) {
            return sbe_socketerror;
        }
    }
    if (connect(slist->slsocket, &saddr, sizeof(saddr)) != 0) {
        closesocket(slist->slsocket);
        slist->slsocket = -1;
        return sbe_connecterror;
    }
    return sbe_noerror;
}

// The writers of a request, which move the buffer and the length on: a text with its end, a byte, a number as it is
// in memory, a number in the byte order of a little-endian machine, and bytes
static void BufferAddNTS(char **buffer, const char *str, int *len) {
    int slen;

    if (str == NULL) {
        str = "";
    }
    slen = strlen(str) + 1;
    memcpy(*buffer, str, slen);
    *len += slen;
    *buffer += slen;
}

static void BufferAddByte(char **buffer, u8 bval, int *len) {
    **buffer = bval;
    *len += 1;
    *buffer += 1;
}

static void BufferAddInt(char **buffer, int ival, int *len) {
    memcpy(*buffer, &ival, 4);
    *len += 4;
    *buffer += 4;
}

static void BufferAddIntLE(char **buffer, int ival, int *len) {
    u32 ivalNBO = SB_SWAP32(ival);
    u32 ivalBE = 0;

    ivalBE |= (ivalNBO & 0xff) << 24;
    ivalBE |= (ivalNBO & 0xff00) << 8;
    ivalBE |= (ivalNBO & 0xff0000) >> 8;
    ivalBE |= (ivalNBO & 0xff000000) >> 24;
    BufferAddInt(buffer, ivalBE, len);
}

static void BufferAddData(char **buffer, char *data, int dlen, int *len) {
    memcpy(*buffer, data, dlen);
    *len += dlen;
    *buffer += dlen;
}

// The challenge that the master server's answer is encrypted with: 8 characters from 33 to 125, each odd or even as
// the ones before give
static void SetupListChallenge(SBServerList *slist) {
    int i;
    int oddmode;

    slist->mychallenge[0] = 33 + rand() % 93;
    oddmode = 0;
    for (i = 1; i < LIST_CHALLENGE_LEN; i++) {
        oddmode = (slist->mychallenge[i - 1] & 1) ^ (i & 1) ^ oddmode ^ (slist->mychallenge[0] & 1) ^
                  ((slist->mychallenge[0] < 79) ? 1 : 0) ^
                  ((slist->mychallenge[i - 1] < slist->mychallenge[0]) ? 1 : 0);
        slist->mychallenge[i] = 33 + rand() % 93;
        if ((oddmode && (slist->mychallenge[i] & 1) == 0) || (!oddmode && (slist->mychallenge[i] & 1) == 1)) {
            slist->mychallenge[i]++;
        }
    }
}

// Sends to the master server, and connects again once if the connection broke
static SBError SendWithRetry(SBServerList *slist, char *data, int len) {
    SBError err;
    int ret = 0;
    int retryCount = 1;

    while (retryCount >= 0) {
        retryCount--;
        ret = send(slist->slsocket, data, len, 0);
        if (ret <= 0 && retryCount >= 0) {
            if (slist->inbufferlen > 0) {
                break;
            }
            SBServerListDisconnect(slist);
            err = SBServerListConnectAndQuery(slist, NULL, NULL, NO_SERVER_LIST, 0);
            if (err != sbe_noerror) {
                ErrorDisconnect(slist);
                return err;
            }
        } else {
            break;
        }
    }
    if (ret <= 0) {
        return sbe_connecterror;
    }
    return sbe_noerror;
}

// The request: its length, the request's type, the versions of the protocol and of the encoding, the version and the
// names of the games, the challenge, the filter, the keys and the options, and the source address and the most
// servers if the options have them
SBError SBServerListConnectAndQuery(SBServerList *slist, const char *fieldList, const char *serverFilter, int options,
                                    int maxServers) {
    SBError err;
    int ret;
    int requestLen;
    u16 netLen;
    char *requestBuf;
    char outgoingRequest[MAX_FIELD_LIST_LEN + MAX_FILTER_LEN + 255 + 1];

    if (fieldList == NULL) {
        fieldList = "";
    }
    if (serverFilter == NULL) {
        serverFilter = "";
    }
    if (strlen(fieldList) > MAX_FIELD_LIST_LEN) {
        return sbe_paramerror;
    }
    if (strlen(serverFilter) > MAX_FILTER_LEN) {
        return sbe_paramerror;
    }
    err = ServerListConnect(slist);
    if (err != sbe_noerror) {
        return err;
    }
    slist->queryoptions = options;
    SetupListChallenge(slist);
    requestLen = 2;
    requestBuf = outgoingRequest + 2;
    BufferAddByte(&requestBuf, SERVER_LIST_REQUEST, &requestLen);
    BufferAddByte(&requestBuf, LIST_PROTOCOL_VERSION, &requestLen);
    BufferAddByte(&requestBuf, LIST_ENCODING_VERSION, &requestLen);
    BufferAddIntLE(&requestBuf, slist->fromgamever, &requestLen);
    BufferAddNTS(&requestBuf, slist->queryforgamename, &requestLen);
    BufferAddNTS(&requestBuf, slist->queryfromgamename, &requestLen);
    BufferAddData(&requestBuf, slist->mychallenge, LIST_CHALLENGE_LEN, &requestLen);
    BufferAddNTS(&requestBuf, serverFilter, &requestLen);
    BufferAddNTS(&requestBuf, fieldList, &requestLen);
    options = SB_SWAP32(options);
    BufferAddInt(&requestBuf, options, &requestLen);
    if (slist->queryoptions & ALTERNATE_SOURCE_IP) {
        BufferAddInt(&requestBuf, slist->srcip, &requestLen);
    }
    if (slist->queryoptions & LIMIT_RESULT_COUNT) {
        BufferAddIntLE(&requestBuf, maxServers, &requestLen);
    }
    netLen = SB_SWAP16((u16)requestLen);
    memcpy(outgoingRequest, &netLen, 2);
    ret = send(slist->slsocket, outgoingRequest, requestLen, 0);
    if (ret <= 0) {
        SBServerListDisconnect(slist);
        return sbe_connecterror;
    }
    slist->state = sl_mainlist;
    slist->pstate = pi_cryptheader;
    if (slist->inbuffer == NULL) {
        slist->inbuffer = gsimalloc(INCOMING_BUFFER_SIZE);
        if (slist->inbuffer == NULL) {
            return sbe_allocerror;
        }
        slist->inbufferlen = 0;
    }
    return sbe_noerror;
}

static void FreePopularValues(SBServerList *slist) {
    int i;

    for (i = 0; i < slist->numpopularvalues; i++) {
        SBReleaseStr(slist, slist->popularvalues[i]);
    }
    slist->numpopularvalues = 0;
}

static void FreeKeyList(SBServerList *slist) {
    int i;

    if (slist->keylist == NULL) {
        return;
    }
    for (i = 0; i < func_ov011_02171588(slist->keylist); i++) {
        SBReleaseStr(slist, ((KeyInfo *)func_ov011_0217158c(slist->keylist, i))->keyName);
    }
    func_ov011_02171558(slist->keylist);
    slist->keylist = NULL;
}

void SBServerListDisconnect(SBServerList *slist) {
    if (slist->inbuffer != NULL) {
        gsifree(slist->inbuffer);
    }
    slist->inbuffer = NULL;
    slist->inbufferlen = 0;
    if (slist->slsocket != -1) {
        closesocket(slist->slsocket);
    }
    slist->slsocket = -1;
    slist->state = sl_disconnected;
    FreeKeyList(slist);
    slist->expectedelements = -1;
    FreePopularValues(slist);
}

void SBServerListCleanup(SBServerList *slist) {
    SBServerListDisconnect(slist);
    SBServerListClear(slist);
    SBRefStrHashCleanup(slist);
    if (slist->servers != NULL) {
        func_ov011_02171558(slist->servers);
    }
    slist->servers = NULL;
}

// The key of the cipher: the master server's challenge mixed into ours with the game's secret key
static void InitCryptKey(SBServerList *slist, char *key, int keylen) {
    int i;
    int seckeylen = strlen(slist->queryfromkey);
    char *seckey = slist->queryfromkey;

    for (i = 0; i < keylen; i++) {
        slist->mychallenge[(i * seckey[i % seckeylen]) % LIST_CHALLENGE_LEN] ^=
            (slist->mychallenge[i % LIST_CHALLENGE_LEN] ^ key[i]) & 0xff;
    }
    GOACryptInit(&slist->cryptkey, (u8 *)slist->mychallenge, LIST_CHALLENGE_LEN);
}

// The size of a server of the list before its keys: its flags and address, and what the flags add
static int ServerSizeForFlags(int flags) {
    int size = 5;

    if (flags & PRIVATE_IP_FLAG) {
        size += 4;
    }
    if (flags & ICMP_IP_FLAG) {
        size += 4;
    }
    if (flags & NONSTANDARD_PORT_FLAG) {
        size += 2;
    }
    if (flags & NONSTANDARD_PRIVATE_PORT_FLAG) {
        size += 2;
    }
    return size;
}

// Whether the keys and values of a server's rules are all in the buffer, up to their end
static int FullRulesPresent(char *buf, int len) {
    int i;

    while (len > 0 && *buf != '\0') {
        i = NTSLengthSB(buf, len);
        if (i < 0) {
            return 0;
        }
        buf += i;
        len -= i;
        i = NTSLengthSB(buf, len);
        if (i < 0) {
            return 0;
        }
        buf += i;
        len -= i;
    }
    if (len == 0) {
        return 0;
    }
    if (*buf == '\0') {
        return 1;
    }
    return 0;
}

// Whether the values of all the list's keys are in the buffer: a byte, a short, or a text's index among the popular
// values or 0xff and the text
static int AllKeysPresent(SBServerList *slist, char *buf, int len) {
    int numkeys;
    int i;
    int strindex;

    numkeys = func_ov011_02171588(slist->keylist);
    for (i = 0; i < numkeys; i++) {
        switch (((KeyInfo *)func_ov011_0217158c(slist->keylist, i))->keyType) {
        case KEYTYPE_BYTE:
            buf++;
            len--;
            break;
        case KEYTYPE_SHORT:
            buf += 2;
            len -= 2;
            break;
        case KEYTYPE_STRING:
            if (len < 1) {
                return 0;
            }
            strindex = (u8)buf[0];
            buf++;
            len--;
            if (strindex == 0xff) {
                strindex = NTSLengthSB(buf, len);
                if (strindex == -1) {
                    return 0;
                }
                buf += strindex;
                len -= strindex;
            }
            break;
        default:
            return 0;
        }
        if (len < 0) {
            return 0;
        }
    }
    return 1;
}

static void ParseServerIPPort(SBServerList *slist, char *buf, int len, u32 *ip, u16 *port) {
    u8 flags;

    if (len < 5) {
        return;
    }
    flags = buf[0];
    buf++;
    len--;
    memcpy(ip, buf, 4);
    buf += 4;
    len -= 4;
    if (flags & NONSTANDARD_PORT_FLAG) {
        if (len < 2) {
            return;
        }
        memcpy(port, buf, 2);
    } else {
        *port = slist->defaultport;
    }
}

// Reads a server of the list, or of the master server's message of one, which has no popular values: its flags,
// addresses, keys and rules. Returns its size
static int ParseServer(SBServerList *slist, SBServer *server, char *buf, int len, int usepopularlist) {
    int numkeys;
    int i;
    short sval;
    int strindex;
    int holdlen = len;
    u32 ip;
    u16 port;
    u8 flags;

    flags = buf[0];
    SBServerSetFlags(server, flags);
    buf++;
    len--;
    // The public address and port are the server's already
    buf += 4;
    len -= 4;
    if (flags & NONSTANDARD_PORT_FLAG) {
        buf += 2;
        len -= 2;
    }
    if (flags & PRIVATE_IP_FLAG) {
        memcpy(&ip, buf, 4);
        buf += 4;
        len -= 4;
    } else {
        ip = 0;
    }
    if (flags & NONSTANDARD_PRIVATE_PORT_FLAG) {
        memcpy(&port, buf, 2);
        buf += 2;
        len -= 2;
    } else {
        port = slist->defaultport;
    }
    SBServerSetPrivateAddr(server, ip, port);
    if (flags & ICMP_IP_FLAG) {
        memcpy(&ip, buf, 4);
        buf += 4;
        len -= 4;
        SBServerSetICMPIP(server, ip);
    }
    if (flags & HAS_KEYS_FLAG) {
        numkeys = func_ov011_02171588(slist->keylist);
        for (i = 0; i < numkeys; i++) {
            KeyInfo *ki = func_ov011_0217158c(slist->keylist, i);

            switch (ki->keyType) {
            case KEYTYPE_BYTE:
                SBServerAddIntKeyValue(server, ki->keyName, (u8)buf[0]);
                buf++;
                len--;
                break;
            case KEYTYPE_SHORT:
                memcpy(&sval, buf, 2);
                SBServerAddIntKeyValue(server, ki->keyName, SB_SWAP16((u16)sval));
                buf += 2;
                len -= 2;
                break;
            case KEYTYPE_STRING:
                if (usepopularlist) {
                    strindex = (u8)buf[0];
                    buf++;
                    len--;
                } else {
                    strindex = 0xff;
                }
                if (strindex == 0xff) {
                    SBServerAddKeyValue(server, ki->keyName, buf);
                    strindex = strlen(buf) + 1;
                    buf += strindex;
                    len -= strindex;
                } else {
                    SBServerAddKeyValue(server, ki->keyName, slist->popularvalues[strindex]);
                }
                break;
            }
        }
        SBServerSetState(server, SBServerGetState(server) | STATE_BASICKEYS);
    }
    if (flags & HAS_FULL_RULES_FLAG) {
        while (*buf != '\0' && len > 0) {
            char *k = buf;

            i = strlen(k) + 1;
            buf += i;
            len -= i;
            SBServerAddKeyValue(server, k, buf);
            i = strlen(buf) + 1;
            buf += i;
            len -= i;
        }
        buf++;
        len--;
        SBServerSetState(server, SBServerGetState(server) | STATE_FULLKEYS);
    }
    // A server with neither has no keys any more
    {
        u8 state = SBServerGetState(server);

        if ((flags & (HAS_FULL_RULES_FLAG | HAS_KEYS_FLAG)) == 0 &&
            ((state & STATE_BASICKEYS) | (state & STATE_FULLKEYS))) {
            state &= ~(STATE_BASICKEYS | STATE_FULLKEYS);
            SBServerSetState(server, state);
        }
    }
    return holdlen - len;
}

// Adds the next server of the list once it is all in the buffer: its size, 0 when it isn't all there yet, -1 for the
// list's end (an address of 0xffffffff) and -2 when out of memory
static int IncomingListParseServer(SBServerList *slist, char *buf, int len) {
    int i;
    u32 ip;
    u16 port;
    SBServer *server;
    u8 flags;

    if (len < 1) {
        return 0;
    }
    flags = buf[0];
    i = ServerSizeForFlags(flags);
    if (len < i) {
        return 0;
    }
    if ((flags & HAS_KEYS_FLAG) && !AllKeysPresent(slist, buf + i, len - i)) {
        return 0;
    }
    if ((flags & HAS_FULL_RULES_FLAG) && !FullRulesPresent(buf + i, len - i)) {
        return 0;
    }
    if (memcmp(buf + 1, "\xff\xff\xff\xff", 4) == 0) {
        return -1;
    }
    ParseServerIPPort(slist, buf, len, &ip, &port);
    server = SBAllocServer(slist, ip, port);
    if (SBIsNullServer(server)) {
        return -2;
    }
    i = ParseServer(slist, server, buf, len, 1);
    SBServerListAppendServer(slist, server);
    return i;
}

void SBSetLastListErrorPtr(SBServerList *slist, char *theError) {
    slist->lasterror = theError;
}

// Reads the master server's answer as it comes, from where the reading got to: the header of the cipher, the
// address of the player and the port of the game's servers, the keys of the list, its popular values and its
// servers. The data after the header comes encrypted
static SBError ProcessMainListData(SBServerList *slist) {
    int reqlen;
    int keyoffset;
    int keylen;
    char *inbuf = slist->inbuffer;
    int inlen = slist->inbufferlen;

    switch (slist->pstate) {
    case pi_cryptheader:
        if (inlen < 1) {
            break;
        }
        reqlen = ((u8)inbuf[0] ^ 0xec) + 2;
        if (inlen < reqlen) {
            break;
        }
        keyoffset = reqlen;
        keylen = (u8)inbuf[reqlen - 1] ^ 0xea;
        reqlen += keylen;
        if (inlen < reqlen) {
            break;
        }
        InitCryptKey(slist, inbuf + keyoffset, keylen);
        slist->pstate = pi_fixedheader;
        // The random bytes start with the game's flags
        memcpy(&slist->backendgameflags, &inbuf[1], 2);
        slist->backendgameflags = SB_SWAP16(slist->backendgameflags);
        inbuf += reqlen;
        inlen -= reqlen;
        GOADecrypt(&slist->cryptkey, (u8 *)inbuf, inlen);
        // fallthrough
    case pi_fixedheader:
        if (inlen < FIXED_HEADER_LEN) {
            break;
        }
        memcpy(&slist->mypublicip, inbuf, 4);
        slist->ListCallback(slist, slc_publicipdetermined, SBNullServer, slist->instance);
        memcpy(&slist->defaultport, inbuf + 4, 2);
        // A port of 0xffff is an error, with its text
        if (slist->defaultport == 0xffff) {
            if (NTSLengthSB(inbuf + FIXED_HEADER_LEN, inlen - FIXED_HEADER_LEN) == -1) {
                break;
            }
            SBSetLastListErrorPtr(slist, inbuf + FIXED_HEADER_LEN);
            slist->ListCallback(slist, slc_queryerror, SBNullServer, slist->instance);
            if (slist->inbuffer == NULL) {
                break;
            }
        }
        inbuf += FIXED_HEADER_LEN;
        inlen -= FIXED_HEADER_LEN;
        if ((slist->queryoptions & NO_SERVER_LIST) || slist->defaultport == 0xffff) {
            slist->pstate = pi_finished;
            slist->state = sl_connected;
            break;
        }
        slist->pstate = pi_keylist;
        slist->expectedelements = -1;
        // fallthrough
    case pi_keylist:
        if (slist->expectedelements == -1) {
            if (inlen < 1) {
                break;
            }
            slist->expectedelements = (u8)inbuf[0];
            slist->keylist = func_ov011_02171514(sizeof(KeyInfo), slist->expectedelements, NULL);
            if (slist->keylist == NULL) {
                return sbe_allocerror;
            }
            inbuf++;
            inlen--;
        }
        while (slist->expectedelements > func_ov011_02171588(slist->keylist)) {
            KeyInfo ki;
            int keylen;

            if (inlen < 2) {
                break;
            }
            keylen = NTSLengthSB(inbuf + 1, inlen - 1);
            if (keylen == -1) {
                break;
            }
            ki.keyType = (u8)inbuf[0];
            ki.keyName = SBRefStr(slist, inbuf + 1);
            func_ov011_021715a4(slist->keylist, &ki);
            inbuf += keylen + 1;
            inlen -= keylen + 1;
        }
        if (slist->expectedelements > func_ov011_02171588(slist->keylist)) {
            break;
        }
        slist->pstate = pi_uniquevaluelist;
        slist->expectedelements = -1;
        // fallthrough
    case pi_uniquevaluelist:
        if (slist->expectedelements == -1) {
            if (inlen < 1) {
                break;
            }
            slist->expectedelements = (u8)inbuf[0];
            slist->numpopularvalues = 0;
            inbuf++;
            inlen--;
        }
        while (slist->expectedelements > slist->numpopularvalues) {
            int keylen = NTSLengthSB(inbuf, inlen);

            if (keylen == -1) {
                break;
            }
            slist->popularvalues[slist->numpopularvalues++] = SBRefStr(slist, inbuf);
            inbuf += keylen;
            inlen -= keylen;
        }
        if (slist->expectedelements > slist->numpopularvalues) {
            break;
        }
        slist->pstate = pi_servers;
        // fallthrough
    case pi_servers:
        if (inlen < 5) {
            break;
        }
        do {
            reqlen = IncomingListParseServer(slist, inbuf, inlen);
            if (reqlen == -2) {
                return sbe_allocerror;
            } else if (reqlen == -1) {
                inlen -= 5;
                inbuf += 5;
                slist->pstate = pi_finished;
                slist->state = sl_connected;
                slist->ListCallback(slist, slc_initiallistcomplete, SBNullServer, slist->instance);
                break;
            }
            inbuf += reqlen;
            inlen -= reqlen;
            // The callback disconnected
            if (slist->inbuffer == NULL) {
                reqlen = 0;
            }
        } while (reqlen != 0);
        break;
    default:
        break;
    }
    if (slist->inbuffer == NULL) {
        return sbe_noerror;
    }
    if (inlen != 0) {
        memmove(slist->inbuffer, inbuf, inlen);
    }
    slist->inbufferlen = inlen;
    return sbe_noerror;
}

// The master server's new keys of the list: their count, then each key's type and name
static SBError ProcessPushKeyList(SBServerList *slist, char *buf, int len) {
    int i;
    int numkeys;

    numkeys = (u8)buf[0];
    buf++;
    len--;
    if (slist->keylist != NULL) {
        FreeKeyList(slist);
    }
    slist->keylist = func_ov011_02171514(sizeof(KeyInfo), numkeys, NULL);
    if (slist->keylist == NULL) {
        return sbe_allocerror;
    }
    for (i = 0; i < numkeys; i++) {
        int keylen;
        KeyInfo ki;

        if (len < 2) {
            return sbe_dataerror;
        }
        keylen = NTSLengthSB(buf + 1, len - 1);
        if (keylen == -1) {
            return sbe_dataerror;
        }
        ki.keyType = (u8)buf[0];
        ki.keyName = SBRefStr(slist, buf + 1);
        func_ov011_021715a4(slist->keylist, &ki);
        buf += keylen + 1;
        len -= keylen + 1;
    }
    return sbe_noerror;
}

// The players that a search found: whether it is the last message, the count, and for each the nick, the server's
// address, when the player was last seen and the game's name
static SBError ProcessPlayerSearch(SBServerList *slist, char *buf, int len) {
    u8 isFinal;
    u8 resultCount;
    u32 ip;
    u16 port;
    char *nick;
    u32 lastSeen;
    int i;

    if (len < 2) {
        return sbe_dataerror;
    }
    isFinal = buf[0];
    resultCount = buf[1];
    buf += 2;
    len -= 2;
    for (i = 0; i < resultCount; i++) {
        int slen;

        nick = buf;
        slen = NTSLengthSB(buf, len);
        if (slen == -1) {
            return sbe_dataerror;
        }
        buf += slen;
        len -= slen;
        if (len < 11) {
            return sbe_dataerror;
        }
        memcpy(&ip, buf, 4);
        memcpy(&port, buf + 4, 2);
        memcpy(&lastSeen, buf + 6, 4);
        lastSeen = SB_SWAP32(lastSeen);
        buf += 10;
        len -= 10;
        slen = NTSLengthSB(buf, len);
        if (slen == -1) {
            return sbe_dataerror;
        }
        slist->PlayerSearchCallback(slist, nick, ip, port, lastSeen, buf, slist->instance);
        buf += slen;
        len -= slen;
    }
    if (isFinal) {
        slist->PlayerSearchCallback(slist, NULL, 0, 0, 0, NULL, slist->instance);
    }
    return sbe_noerror;
}

// A server's map loop: its address, when the map changed, the count of the maps and their names
static SBError ProcessMaploop(SBServerList *slist, char *buf, int len) {
    u32 changeTime;
    u32 ip;
    u16 port;
    SBServer *server;
    u8 mapCount;
    int i;
    char *mapList[MAX_MAPLOOP_LENGTH];

    if (len < 11) {
        return sbe_dataerror;
    }
    memcpy(&ip, buf, 4);
    memcpy(&port, buf + 4, 2);
    i = SBServerListFindServerByIP(slist, ip, port);
    if (i == -1) {
        return sbe_noerror;
    }
    server = SBServerListNth(slist, i);
    memcpy(&changeTime, buf + 6, 4);
    changeTime = SB_SWAP32(changeTime);
    mapCount = buf[10];
    buf += 11;
    len -= 11;
    for (i = 0; i < mapCount && i < MAX_MAPLOOP_LENGTH; i++) {
        int maplen;

        if (len < 1) {
            break;
        }
        maplen = NTSLengthSB(buf, len);
        if (maplen == -1) {
            return sbe_dataerror;
        }
        mapList[i] = buf;
        buf += maplen;
        len -= maplen;
    }
    if (slist->MaploopCallback == NULL) {
        return sbe_noerror;
    }
    slist->MaploopCallback(slist, server, changeTime, i, mapList, slist->instance);
    return sbe_noerror;
}

static SBError ProcessDeleteServer(SBServerList *slist, char *buf, int len) {
    u32 ip;
    u16 port;
    int i;

    if (len < 6) {
        return sbe_dataerror;
    }
    memcpy(&ip, buf, 4);
    memcpy(&port, buf + 4, 2);
    i = SBServerListFindServerByIP(slist, ip, port);
    if (i == -1) {
        return sbe_noerror;
    }
    SBServerListRemoveAt(slist, i);
    return sbe_noerror;
}

// A server that the master server sends after the list, as the list has them but without popular values
static SBError ProcessPushServer(SBServerList *slist, char *buf, int len) {
    SBServer *server;
    u32 ip;
    u16 port;
    int foundexisting;
    int i;

    if (len < 5) {
        return sbe_dataerror;
    }
    ParseServerIPPort(slist, buf, len, &ip, &port);
    foundexisting = SBServerListFindServerByIP(slist, ip, port);
    if (foundexisting == -1) {
        server = SBAllocServer(slist, ip, port);
        if (SBIsNullServer(server)) {
            return sbe_allocerror;
        }
    } else {
        server = SBServerListNth(slist, foundexisting);
    }
    i = ParseServer(slist, server, buf, len, 0);
    if (i < 0) {
        return sbe_dataerror;
    }
    if (foundexisting == -1) {
        SBServerListAppendServer(slist, server);
    }
    slist->ListCallback(slist, slc_serverupdated, server, slist->instance);
    return sbe_noerror;
}

// The master server's messages after the list, each its length, its type and its data
static SBError ProcessAdHocData(SBServerList *slist) {
    u16 msglen;
    int i;
    SBError ret = sbe_noerror;

    while (slist->inbufferlen >= 3) {
        memcpy(&msglen, slist->inbuffer, 2);
        msglen = SB_SWAP16(msglen);
        if (msglen > INCOMING_BUFFER_SIZE) {
            ret = sbe_dataerror;
            break;
        }
        if (slist->inbufferlen < msglen) {
            return sbe_noerror;
        }
        switch (slist->inbuffer[2]) {
        case PUSH_KEYS_MESSAGE:
            ret = ProcessPushKeyList(slist, slist->inbuffer + 3, msglen - 3);
            break;
        case PUSH_SERVER_MESSAGE:
            ret = ProcessPushServer(slist, slist->inbuffer + 3, msglen - 3);
            break;
        case KEEPALIVE_MESSAGE:
            // Sent back as it is
            i = send(slist->slsocket, slist->inbuffer, msglen, 0);
            if (i <= 0) {
                return sbe_connecterror;
            }
            break;
        case DELETE_SERVER_MESSAGE:
            ret = ProcessDeleteServer(slist, slist->inbuffer + 3, msglen - 3);
            break;
        case MAPLOOP_MESSAGE:
            ret = ProcessMaploop(slist, slist->inbuffer + 3, msglen - 3);
            break;
        case PLAYERSEARCH_MESSAGE:
            ret = ProcessPlayerSearch(slist, slist->inbuffer + 3, msglen - 3);
            break;
        }
        slist->inbufferlen -= msglen;
        if (slist->inbufferlen != 0 && slist->inbuffer != NULL) {
            memmove(slist->inbuffer, slist->inbuffer + msglen, slist->inbufferlen);
        }
        if (ret != sbe_noerror) {
            break;
        }
    }
    if (ret != sbe_noerror) {
        ErrorDisconnect(slist);
    }
    return ret;
}

static SBError ProcessIncomingData(SBServerList *slist) {
    SBError err;
    int len;
    int oldlen;

    if (!CanReceiveOnSocket(slist->slsocket)) {
        return sbe_noerror;
    }
    oldlen = slist->inbufferlen;
    len = recv(slist->slsocket, slist->inbuffer + slist->inbufferlen, INCOMING_BUFFER_SIZE - slist->inbufferlen, 0);
    if (len == -1 || len == 0) {
        ErrorDisconnect(slist);
        return sbe_connecterror;
    }
    slist->inbufferlen += len;
    err = sbe_noerror;
    if (slist->state == sl_connected || slist->pstate > pi_cryptheader) {
        GOADecrypt(&slist->cryptkey, (u8 *)(slist->inbuffer + oldlen), slist->inbufferlen - oldlen);
    }
    if (slist->state == sl_mainlist) {
        err = ProcessMainListData(slist);
    }
    if (err != sbe_noerror) {
        return err;
    }
    if (slist->state == sl_connected && slist->inbufferlen > 0) {
        return ProcessAdHocData(slist);
    }
    return sbe_noerror;
}

// The message's length, its type and the server's address, then its data
SBError SBSendMessageToServer(SBServerList *slist, u32 ip, u16 port, const char *data, int len) {
    char requestBuffer[16];
    u16 tmp;
    u16 msgLen = 9;
    SBError ret;
    int i;

    if (slist->state == sl_disconnected) {
        SBServerListConnectAndQuery(slist, NULL, NULL, NO_SERVER_LIST, 0);
    }
    if (slist->state == sl_disconnected) {
        return sbe_connecterror;
    }
    tmp = SB_SWAP16((u16)(msgLen + len));
    memcpy(requestBuffer, &tmp, 2);
    requestBuffer[2] = SEND_MESSAGE_REQUEST;
    memcpy(requestBuffer + 3, &ip, sizeof(ip));
    memcpy(requestBuffer + 7, &port, sizeof(port));
    ret = SendWithRetry(slist, requestBuffer, msgLen);
    if (ret != sbe_noerror) {
        return ret;
    }
    i = send(slist->slsocket, data, len, 0);
    if (i < 0) {
        return sbe_connecterror;
    }
    return sbe_noerror;
}

// The cookie after NAT negotiation's magic bytes
SBError SBSendNatNegotiateCookieToServer(SBServerList *slist, u32 ip, u16 port, int cookie) {
    u8 negotiateBuffer[6 + 4];

    negotiateBuffer[0] = 0xfd;
    negotiateBuffer[1] = 0xfc;
    negotiateBuffer[2] = 0x1e;
    negotiateBuffer[3] = 0x66;
    negotiateBuffer[4] = 0x6a;
    negotiateBuffer[5] = 0xb2;
    cookie = SB_SWAP32(cookie);
    memcpy(negotiateBuffer + 6, &cookie, 4);
    return SBSendMessageToServer(slist, ip, port, (char *)negotiateBuffer, 6 + 4);
}

// Adds the servers of the local network that answer, until the time of the search runs out
static SBError ProcessLanData(SBServerList *slist) {
    char indata[1500];
    SocAddrIn saddr;
    int saddrlen = sizeof(saddr);
    int error;
    int foundexisting;
    SBServer *server;

    while (CanReceiveOnSocket(slist->slsocket)) {
        error = recvfrom(slist->slsocket, indata, sizeof(indata) - 1, 0, &saddr, &saddrlen);
        if (error == -1) {
            continue;
        }
        foundexisting = SBServerListFindServerByIP(slist, saddr.addr, saddr.port);
        if (foundexisting != -1) {
            continue;
        }
        server = SBAllocServer(slist, saddr.addr, saddr.port);
        if (SBIsNullServer(server)) {
            return sbe_allocerror;
        }
        SBServerSetFlags(server, UNSOLICITED_UDP_FLAG | NONSTANDARD_PORT_FLAG);
        SBServerListAppendServer(slist, server);
    }
    if (current_time() - slist->lanstarttime > SL_LAN_SEARCH_TIME) {
        closesocket(slist->slsocket);
        slist->slsocket = -1;
        slist->state = sl_disconnected;
        slist->ListCallback(slist, slc_initiallistcomplete, SBNullServer, slist->instance);
    }
    return sbe_noerror;
}

SBError SBListThink(SBServerList *slist) {
    SBFreeDeadList(slist);
    switch (slist->state) {
    case sl_disconnected:
        break;
    case sl_connected:
    case sl_mainlist:
        return ProcessIncomingData(slist);
    case sl_lanbrowse:
        return ProcessLanData(slist);
    }
    return sbe_noerror;
}
