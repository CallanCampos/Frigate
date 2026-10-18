#include "dwc/gstats.h"
#include "dwc/ext.h"

// GameSpy's stats and persistent data (gstats.c). The ROM names neither the file nor its functions: the file,
// CloseStatsConnection, IsStatsConnected, PersistThink, ProcessGetData, ProcessStatement, CallReqCallback and the
// operations ServerOpInt to PlayerOpString are GameSpy's, as pret's pokerevo Wii build has them (gstats.s), and the
// other functions and the data are the later SDK's

// What a request is
typedef enum StatsRequestType {
    rt_authcb,
    rt_datacb,
    rt_savecb,
    rt_profilecb,
} StatsRequestType;

// A request that waits for the server's answer: its type, the player's local id and profile, the persistent data's
// type and index, and the callback with its argument
typedef struct StatsServerRequest {
    StatsRequestType reqtype;
    int localid;
    int profileid;
    int pdtype;
    int pdindex;
    void *instance;
    void *callback;
} StatsServerRequest;

// The callbacks of the four requests
typedef void (*PersAuthCallbackFn)(int localid, int profileid, int authenticated, char *errmsg, void *instance);
typedef void (*PersDataCallbackFn)(int localid, int profileid, int type, int index, int success, u32 modified,
                                   char *data, int len, void *instance);
typedef void (*PersDataSaveCallbackFn)(int localid, int profileid, int type, int index, int success, u32 modified,
                                       void *instance);
typedef void (*ProfileCallbackFn)(int localid, int profileid, int success, void *instance);

static void xcode_buf(char *buf, int len);
static char *value_for_key(const char *s, const char *key);
static char *value_for_key_safe(const char *s, const char *key);
static BOOL SocketReadable(int s);
static char *FindFinal(char *buff, int len);
static int FindRequest(StatsRequestType reqtype, int localid, int profileid);
static void ProcessPlayerAuth(const char *buf, int len);
static void ProcessGetPid(const char *buf, int len);
static void ProcessGetData(const char *buf, int len);
static void ProcessSetData(const char *buf, int len);
static void ProcessStatement(char *buff, int len);
static int ProcessInBuffer(char *buff, int len);
static void CallReqCallback(int reqindex, int success, u32 modified, char *data, int length);
static void ClosePendingCallbacks(void);
static int ServerOpInt(StatsGame *game, char *name, BucketFunc func, int value, int index);
static double ServerOpFloat(StatsGame *game, char *name, BucketFunc func, double value, int index);
static char *ServerOpString(StatsGame *game, char *name, BucketFunc func, char *value, int index);
static int TeamOpInt(StatsGame *game, char *name, BucketFunc func, int value, int index);
static double TeamOpFloat(StatsGame *game, char *name, BucketFunc func, double value, int index);
static char *TeamOpString(StatsGame *game, char *name, BucketFunc func, char *value, int index);
static int PlayerOpInt(StatsGame *game, char *name, BucketFunc func, int value, int index);
static double PlayerOpFloat(StatsGame *game, char *name, BucketFunc func, double value, int index);
static char *PlayerOpString(StatsGame *game, char *name, BucketFunc func, char *value, int index);

// MWCC lays out a file's initialized data, which the zeroed variables of the .bss are part of, with a heap sort by size
// of the reverse of the order of their definitions, so these are defined in an order that gives the ROM's layout.
// sesskeystr and challengestr, the keys "sesskey" and "challenge" with their first letters left out as in enc1 and
// enc3, are the later SDK's statics of RecvSessionKey and SendChallengeResponse, which the ROM doesn't have; nor does
// it have the code that uses connid, sesskey, internal_init, gameport, initstart and the three words that are only
// named here (sUnused0 to sUnused2)
char gcd_gamename[256] = "";
static int connid = 0;
char gcd_secret_key[256] = "";
static int sUnused1 = 0;
static StatsGame *g_statsgame = NULL;
static int sock = -1;
// The keys that the messages are scrambled with, "GameSpy3D" and "ProjectAphex", whose first letters the code sets,
// so that the texts aren't in the ROM as they are
static char enc1[16] = { '\0', 'a', 'm', 'e', 'S', 'p', 'y', '3', 'D', '\0' };
static char enc3[16] = { '\0', 'r', 'o', 'j', 'e', 'c', 't', 'A', 'p', 'h', 'e', 'x', '\0' };
static int sesskey = 0;
static char challengestr[10] = { '\0', 'h', 'a', 'l', 'l', 'e', 'n', 'g', 'e', '\0' };
static int internal_init = 0;
static char sesskeystr[8] = { '\0', 'e', 's', 's', 'k', 'e', 'y', '\0' };
static char finalstr[10] = { '\0', 'f', 'i', 'n', 'a', 'l', '\\', '\0' };
static char *rcvbuffer = NULL;
static char *enc = enc1;
static u32 inittimeout = 20000;
static int sUnused0 = 0;
void *bopfuncs[3][3] = {
    { ServerOpInt, ServerOpFloat, ServerOpString },
    { TeamOpInt, TeamOpFloat, TeamOpString },
    { PlayerOpInt, PlayerOpFloat, PlayerOpString },
};
static int rcvlen = 0;
static u32 initstart = 0;
static int stats_initstate = 0;
static int gameport = 0;
// value_for_key's, which takes its two buffers in turn
static int valueindex = 0;
static DArray *serverreqs = NULL;
static int sUnused2 = 0;
char StatsServerHostname[64] = "gamestats.gs.nintendowifi.net";
static int rcvmax = 0;
BucketFunc bucketfuncs[7] = { BucketSet, BucketAdd, BucketSub, BucketMult, BucketDiv, BucketConcat, BucketAvg };

void CloseStatsConnection(void) {
    if (sock != -1) {
        shutdown(sock, 2);
        closesocket(sock);
    }
    sock = -1;
    ClosePendingCallbacks();
    if (rcvbuffer != NULL) {
        gsifree(rcvbuffer);
        rcvbuffer = NULL;
        rcvmax = 0;
        rcvlen = 0;
    }
}

BOOL IsStatsConnected(void) {
    return sock != -1;
}

BOOL PersistThink(void) {
    int len;
    int processed;

    if (sock == -1) {
        return FALSE;
    }
    if (stats_initstate != 5) {
        return FALSE;
    }
    while (SocketReadable(sock)) {
        // Keeps at least 128 bytes free in the buffer
        if (rcvmax - rcvlen < 128) {
            if (rcvmax < 256) {
                rcvmax = 256;
            } else {
                rcvmax *= 2;
            }
            rcvbuffer = gsirealloc(rcvbuffer, rcvmax + 1);
            if (rcvbuffer == NULL) {
                return FALSE;
            }
        }
        len = recv(sock, rcvbuffer + rcvlen, rcvmax - rcvlen, 0);
        if (len <= 0) {
            CloseStatsConnection();
            return FALSE;
        }
        rcvlen += len;
        rcvbuffer[rcvlen] = '\0';
        processed = ProcessInBuffer(rcvbuffer, rcvlen);
        if (processed == rcvlen) {
            rcvlen = 0;
        } else {
            memmove(rcvbuffer, rcvbuffer + processed, rcvlen - processed);
            rcvlen -= processed;
        }
    }
    if (sock == -1) {
        return FALSE;
    }
    return TRUE;
}

// Scrambles or unscrambles a buffer with the key that enc points to
static void xcode_buf(char *buf, int len) {
    int i;
    char *pos = enc;

    for (i = 0; i < len; i++) {
        buf[i] ^= *pos++;
        if (*pos == '\0') {
            pos = enc;
        }
    }
}

// The value of a key in a message, in one of two buffers that the calls take in turn, or NULL
static char *value_for_key(const char *s, const char *key) {
    char *pos;
    char *pos2;
    char keyspec[256] = "\\";
    static char value[2][256];

    valueindex ^= 1;
    strcat(keyspec, key);
    strcat(keyspec, "\\");
    pos = strstr(s, keyspec);
    if (pos == NULL) {
        return NULL;
    }
    pos += strlen(keyspec);
    pos2 = value[valueindex];
    while (*pos != '\0' && *pos != '\\') {
        *pos2++ = *pos++;
    }
    *pos2 = '\0';
    return value[valueindex];
}

static char *value_for_key_safe(const char *s, const char *key) {
    char *temp = value_for_key(s, key);

    if (temp == NULL) {
        return "";
    }
    return temp;
}

static BOOL SocketReadable(int s) {
    return CanReceiveOnSocket(s);
}

// The "\final\" that ends a message, or NULL
static char *FindFinal(char *buff, int len) {
    char *pos = buff;

    while (pos - buff < len - 6) {
        if (pos[0] == '\\' && pos[1] == 'f' && pos[2] == 'i' && pos[3] == 'n' && pos[4] == 'a' && pos[5] == 'l' &&
            pos[6] == '\\') {
            return pos;
        }
        pos++;
    }
    return NULL;
}

// The index of a request in the list, or -1
static int FindRequest(StatsRequestType reqtype, int localid, int profileid) {
    int i;
    StatsServerRequest *req;

    if (serverreqs == NULL) {
        return -1;
    }
    for (i = 0; i < func_ov011_02171588(serverreqs); i++) {
        req = func_ov011_0217158c(serverreqs, i);
        if (req->reqtype == reqtype && req->localid == localid && req->profileid == profileid) {
            return i;
        }
    }
    return -1;
}

// "\pauthr\<profile>\lid\<local id>", the answer to an authentication
static void ProcessPlayerAuth(const char *buf, int len) {
    int reqindex;
    char *errmsg;
    int pid;
    int lid;

    pid = atol(value_for_key_safe(buf, "pauthr"));
    lid = atol(value_for_key_safe(buf, "lid"));
    errmsg = value_for_key_safe(buf, "errmsg");
    reqindex = FindRequest(rt_authcb, lid, 0);
    if (reqindex == -1) {
        return;
    }
    ((StatsServerRequest *)func_ov011_0217158c(serverreqs, reqindex))->profileid = pid;
    CallReqCallback(reqindex, pid > 0, 0, errmsg, 0);
}

// "\getpidr\<profile>\lid\<local id>"
static void ProcessGetPid(const char *buf, int len) {
    int reqindex;
    int pid;
    int lid;

    pid = atol(value_for_key_safe(buf, "getpidr"));
    lid = atol(value_for_key_safe(buf, "lid"));
    reqindex = FindRequest(rt_profilecb, lid, 0);
    if (reqindex == -1) {
        return;
    }
    ((StatsServerRequest *)func_ov011_0217158c(serverreqs, reqindex))->profileid = pid;
    CallReqCallback(reqindex, pid > 0, 0, NULL, 0);
}

// "\getpdr\<success>\lid\<local id>\pid\<profile>\mod\<time>\length\<length>\data\<data>"
static void ProcessGetData(const char *buf, int len) {
    int reqindex;
    int pid;
    int lid;
    int success;
    int length;
    u32 modified;
    char *data;

    success = atol(value_for_key_safe(buf, "getpdr"));
    lid = atol(value_for_key_safe(buf, "lid"));
    pid = atol(value_for_key_safe(buf, "pid"));
    modified = atol(value_for_key_safe(buf, "mod"));
    reqindex = FindRequest(rt_datacb, lid, pid);
    if (reqindex == -1) {
        return;
    }
    length = atol(value_for_key_safe(buf, "length"));
    data = strstr(buf, "\\data\\");
    if (data == NULL) {
        length = 0;
        data = "";
    } else {
        data += 6;
    }
    CallReqCallback(reqindex, success, modified, data, length);
}

// "\setpdr\<success>\lid\<local id>\pid\<profile>\mod\<time>"
static void ProcessSetData(const char *buf, int len) {
    int reqindex;
    int pid;
    int lid;
    int success;
    int modified;

    success = atol(value_for_key_safe(buf, "setpdr"));
    pid = atol(value_for_key_safe(buf, "pid"));
    lid = atol(value_for_key_safe(buf, "lid"));
    modified = atol(value_for_key_safe(buf, "mod"));
    reqindex = FindRequest(rt_savecb, lid, pid);
    if (reqindex == -1) {
        return;
    }
    CallReqCallback(reqindex, success, modified, NULL, 0);
}

static void ProcessStatement(char *buff, int len) {
    buff[len] = '\0';
    if (strncmp(buff, "\\pauthr\\", 8) == 0) {
        ProcessPlayerAuth(buff, len);
    } else if (strncmp(buff, "\\getpidr\\", 9) == 0) {
        ProcessGetPid(buff, len);
    } else if (strncmp(buff, "\\getpidr\\", 9) == 0) {
        ProcessGetPid(buff, len);
    } else if (strncmp(buff, "\\getpdr\\", 8) == 0) {
        ProcessGetData(buff, len);
    } else if (strncmp(buff, "\\setpdr\\", 8) == 0) {
        ProcessSetData(buff, len);
    }
}

// Handles the messages in a buffer, and returns how many bytes they took
static int ProcessInBuffer(char *buff, int len) {
    char *pos;
    int oldlen = len;

    pos = FindFinal(buff, len);
    while (len > 0 && pos != NULL) {
        enc = enc1;
        xcode_buf(buff, pos - buff);
        ProcessStatement(buff, pos - buff);
        len -= pos - buff + 7;
        buff = pos + 7;
        if (len > 0) {
            pos = FindFinal(buff, len);
        }
    }
    return oldlen - len;
}

// Calls a request's callback and removes the request
static void CallReqCallback(int reqindex, int success, u32 modified, char *data, int length) {
    StatsServerRequest *req;

    if (reqindex < 0 || reqindex >= func_ov011_02171588(serverreqs)) {
        return;
    }
    req = func_ov011_0217158c(serverreqs, reqindex);
    if (req->callback != NULL) {
        switch (req->reqtype) {
        case rt_authcb:
            ((PersAuthCallbackFn)req->callback)(req->localid, req->profileid, success, data, req->instance);
            break;
        case rt_datacb:
            ((PersDataCallbackFn)req->callback)(req->localid, req->profileid, req->pdtype, req->pdindex, success,
                                                modified, data, length, req->instance);
            break;
        case rt_savecb:
            ((PersDataSaveCallbackFn)req->callback)(req->localid, req->profileid, req->pdtype, req->pdindex, success,
                                                    modified, req->instance);
            break;
        case rt_profilecb:
            ((ProfileCallbackFn)req->callback)(req->localid, req->profileid, success, req->instance);
            break;
        }
    }
    func_ov011_02171668(serverreqs, reqindex);
}

// Calls the callbacks of the requests left, when the connection is lost, with "Connection Lost"
static void ClosePendingCallbacks(void) {
    int i;

    if (serverreqs == NULL) {
        return;
    }
    for (i = func_ov011_02171588(serverreqs) - 1; i >= 0; i--) {
        char connerror[] = "\x13\x1D\x1\x4\x0\x0\x0\x28\x1F\x6\x45\x34\x3F\x1\x1B";

        enc = enc3;
        xcode_buf(connerror, sizeof(connerror) - 1);
        CallReqCallback(i, 0, 0, connerror, 0);
    }
    func_ov011_02171558(serverreqs);
    serverreqs = NULL;
}

int GetTeamIndex(StatsGame *game, int tnum) {
    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return tnum;
    }
    return *(int *)func_ov011_0217158c(game->teamnums, tnum);
}

int GetPlayerIndex(StatsGame *game, int pnum) {
    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return pnum;
    }
    return *(int *)func_ov011_0217158c(game->playernums, pnum);
}

// Applies an operation to a bucket of the game, or creates the bucket with the value
#define DO_FUNC(func, game, name, value, type, ret)                                                                    \
    if (game == NULL) {                                                                                                \
        game = g_statsgame;                                                                                            \
    }                                                                                                                  \
    if (game == NULL) {                                                                                                \
        ret = value;                                                                                                   \
    } else {                                                                                                           \
        ret = func(game->buckets, name, value);                                                                        \
        if (ret == NULL) {                                                                                             \
            ret = BucketNew(game->buckets, name, type, value);                                                         \
        }                                                                                                              \
    }

static int ServerOpInt(StatsGame *game, char *name, BucketFunc func, int value, int index) {
    int *ret;

    DO_FUNC(func, game, name, &value, bt_int, ret);
    return *ret;
}

static double ServerOpFloat(StatsGame *game, char *name, BucketFunc func, double value, int index) {
    double *ret;

    DO_FUNC(func, game, name, &value, bt_float, ret);
    return *ret;
}

static char *ServerOpString(StatsGame *game, char *name, BucketFunc func, char *value, int index) {
    char *ret;

    DO_FUNC(func, game, name, value, bt_string, ret);
    return ret;
}

static int TeamOpInt(StatsGame *game, char *name, BucketFunc func, int value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_t%d", name, GetTeamIndex(game, index));
    return ServerOpInt(game, fullname, func, value, index);
}

static double TeamOpFloat(StatsGame *game, char *name, BucketFunc func, double value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_t%d", name, GetTeamIndex(game, index));
    return ServerOpFloat(game, fullname, func, value, index);
}

static char *TeamOpString(StatsGame *game, char *name, BucketFunc func, char *value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_t%d", name, GetTeamIndex(game, index));
    return ServerOpString(game, fullname, func, value, index);
}

static int PlayerOpInt(StatsGame *game, char *name, BucketFunc func, int value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_%d", name, GetPlayerIndex(game, index));
    return ServerOpInt(game, fullname, func, value, index);
}

static double PlayerOpFloat(StatsGame *game, char *name, BucketFunc func, double value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_%d", name, GetPlayerIndex(game, index));
    return ServerOpFloat(game, fullname, func, value, index);
}

static char *PlayerOpString(StatsGame *game, char *name, BucketFunc func, char *value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_%d", name, GetPlayerIndex(game, index));
    return ServerOpString(game, fullname, func, value, index);
}
