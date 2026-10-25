#include "dwc/gstats.h"
#include "dwc/ext.h"
#include "dwc/md5.h"

// GameSpy's stats and persistent data (gstats.c). The ROM names neither the file nor its functions: the file,
// CloseStatsConnection, IsStatsConnected, PersistThink, ProcessGetData, ProcessStatement, CallReqCallback and the
// operations ServerOpInt to PlayerOpString are GameSpy's, as pret's pokerevo Wii build has them (gstats.s), and the
// other functions and the data are the later SDK's. The functions that the game doesn't call, which the original link
// dead-stripped, are reconstructions from GameSpy's SDK (UniSpySDK's copy), listed in config/deadstrip.toml: the
// statics they read and their local initializers and function-local statics give the file's data the ROM's layout

// What a request is
typedef enum {
    rt_authcb,
    rt_datacb,
    rt_savecb,
    rt_profilecb,
} reqtype_t;

// A request that waits for the server's answer: its type, the player's local id and profile, the persistent data's
// type and index, and the callback with its argument
typedef struct StatsServerRequest {
    reqtype_t reqtype;
    int localid;
    int profileid;
    persisttype_t pdtype;
    int pdindex;
    void *instance;
    void *callback;
} StatsServerRequest;

static void InternalInit(void);
static int SendChallengeResponse(const char *indata, int theGameport);
static int RecvSessionKey(void);
static int DoSend(char *data, int len);
static void xcode_buf(char *buf, int len);
static int g_crc32(char *s, int len);
static void create_challenge(int challenge, char chstr[9]);
static char *value_for_key(const char *s, const char *key);
static char *value_for_key_safe(const char *s, const char *key);
static int get_sockaddrin(const char *host, int port, SOCKADDR_IN *saddr, HOSTENT **savehent);
static void AddRequestCallback(reqtype_t reqtype, int localid, int profileid, persisttype_t pdtype, int pdindex,
                               void *callback, void *instance);
static void SendPlayerAuthRequest(char *data, int len, int localid, PersAuthCallbackFn callback, void *instance);
static void SetPersistDataHelper(int localid, int profileid, persisttype_t type, int index, const char *data, int len,
                                 PersDataSaveCallbackFn callback, void *instance, int kvset);
static int SocketReadable(SOCKET s);
static char *FindFinal(char *buff, int len);
static int FindRequest(reqtype_t reqtype, int localid, int profileid);
static void ProcessPlayerAuth(const char *buf, int len);
static void ProcessGetPid(const char *buf, int len);
static void ProcessGetData(const char *buf, int len);
static void ProcessSetData(const char *buf, int len);
static void ProcessStatement(char *buff, int len);
static int ProcessInBuffer(char *buff, int len);
static void CallReqCallback(int reqindex, int success, gsi_u32 modified, char *data, int length);
static void ClosePendingCallbacks(void);
static int ServerOpInt(statsgame_t game, char *name, BucketFunc func, int value, int index);
static double ServerOpFloat(statsgame_t game, char *name, BucketFunc func, double value, int index);
static char *ServerOpString(statsgame_t game, char *name, BucketFunc func, char *value, int index);
static int TeamOpInt(statsgame_t game, char *name, BucketFunc func, int value, int index);
static double TeamOpFloat(statsgame_t game, char *name, BucketFunc func, double value, int index);
static char *TeamOpString(statsgame_t game, char *name, BucketFunc func, char *value, int index);
static int PlayerOpInt(statsgame_t game, char *name, BucketFunc func, int value, int index);
static double PlayerOpFloat(statsgame_t game, char *name, BucketFunc func, double value, int index);
static char *PlayerOpString(statsgame_t game, char *name, BucketFunc func, char *value, int index);
static char *CreateBucketSnapShot(bucketset_t buckets);

// The data of GameSpy's SDK (UniSpySDK's copy), in its order. The file is built with -ipa file, under which MWCC lays
// out the statics that some code reads, with the local initializers and the function-local statics, in a heap sort by
// size; many of them are read only by the code that the original link dead-stripped, which is rebuilt below
char gcd_gamename[256] = "";
char gcd_secret_key[256] = "";
static statsgame_t g_statsgame = NULL;
static int connid = 0;
static int sesskey = 0;
static SOCKET sock = -1;
// The keys that the messages are scrambled with, "GameSpy3D" and "ProjectAphex", whose first letters InternalInit
// sets, so that the texts aren't in the ROM as they are
static char enc1[16] = { '\0', 'a', 'm', 'e', 'S', 'p', 'y', '3', 'D', '\0' };
static char enc3[16] = { '\0', 'r', 'o', 'j', 'e', 'c', 't', 'A', 'p', 'h', 'e', 'x', '\0' };
static char finalstr[10] = { '\0', 'f', 'i', 'n', 'a', 'l', '\\', '\0' };
static char *enc = enc1;
static int internal_init = 0;
static char *rcvbuffer = NULL;
static int rcvmax = 0;
static int rcvlen = 0;
static int stats_initstate = init_none;
static int gameport = 0;
static gsi_u32 initstart = 0;
static gsi_u32 inittimeout = 20000;
char StatsServerHostname[64] = "gamestats.gs.nintendowifi.net";
static DArray serverreqs = NULL;
BucketFunc bucketfuncs[7] = { BucketSet, BucketAdd, BucketSub, BucketMult, BucketDiv, BucketConcat, BucketAvg };
void *bopfuncs[3][3] = {
    { ServerOpInt, ServerOpFloat, ServerOpString },
    { TeamOpInt, TeamOpFloat, TeamOpString },
    { PlayerOpInt, PlayerOpFloat, PlayerOpString },
};

// What a connection's or a game's id is XORed with for its challenge
#define CHALLENGEXOR 0x38f371e6

// The functions that the ROM doesn't have (GenerateAuthA to InitStatsConnection, GetChallenge to SetPersistDataValuesA,
// StatsThink, and the static functions that only they call) are reconstructions from GameSpy's SDK (UniSpySDK's copy),
// with no code in the ROM to check them against (config/deadstrip.toml)

// The response to a challenge: the MD5 digest of the password and the challenge
char *GenerateAuthA(const char *challenge, const char *password, char response[33]) {
    char rawout[128];

    if (strlen(password) + strlen(challenge) + 20 >= sizeof(rawout)) {
        strcpy(response, "CD Key or challenge too long");
        return response;
    }
    sprintf(rawout, "%s%s", password, challenge);
    MD5Digest((unsigned char *)rawout, strlen(rawout), response);
    return response;
}

// Starts connecting to the stats server
int InitStatsAsync(int theGamePort, gsi_time theInitTimeout) {
    SOCKADDR_IN saddr;
    char tempHostname[sizeof(gcd_gamename) + 2 + sizeof(StatsServerHostname)];
    int ret;

    gameport = theGamePort;
    if (theInitTimeout != 0) {
        inittimeout = theInitTimeout;
    }
    if (__GSIACResult != GSIACAvailable) {
        return GE_NOSOCKET;
    }
    if (!internal_init) {
        InternalInit();
    }
    SocketStartUp();
    sesskey = current_time();
    if (sock != -1) {
        CloseStatsConnection();
    }
    rcvlen = 0;
    if (inet_addr(StatsServerHostname) == -1) {
        strcpy(tempHostname, gcd_gamename);
        strcat(tempHostname, ".");
        strcat(tempHostname, StatsServerHostname);
    } else {
        strcpy(tempHostname, StatsServerHostname);
    }
    if (get_sockaddrin(tempHostname, 29920, &saddr, NULL) == 0) {
        return GE_NODNS;
    }
    sock = socket(SOC_AF_INET, SOC_SOCK_STREAM, 0);
    if (sock == -1) {
        return GE_NOSOCKET;
    }
    SetSockBlocking(sock, 0);
    ret = connect(sock, (SOCKADDR *)&saddr, sizeof(saddr));
    if (ret == -1) {
        int anError = GOAGetLastError(sock);

        if (anError != SOC_EWOULDBLOCK && anError != SOC_ETIMEDOUT && anError != SOC_EINPROGRESS) {
            stats_initstate = init_failed;
            closesocket(sock);
            return GE_NOCONNECT;
        }
    }
    rcvbuffer = gsimalloc(64);
    if (rcvbuffer == NULL) {
        return GE_NOCONNECT;
    }
    rcvmax = 64;
    rcvlen = 0;
    initstart = current_time();
    stats_initstate = init_connecting;
    return GE_CONNECTING;
}

// Takes InitStatsAsync's connection a step further
int InitStatsThink(void) {
    switch (stats_initstate) {
    case init_failed:
        return GE_NOCONNECT;
    case init_connecting: {
        int aWriteFlag = 0;
        int aExceptFlag = 0;
        int aResult = GSISocketSelect(sock, NULL, &aWriteFlag, &aExceptFlag);

        if (aResult == -1 || (aResult == 1 && aExceptFlag == 1)) {
            stats_initstate = init_failed;
            CloseStatsConnection();
            return GE_NOCONNECT;
        } else if (aResult == 0) {
            if (current_time() - initstart > inittimeout) {
                stats_initstate = init_failed;
                CloseStatsConnection();
                return GE_TIMEDOUT;
            } else {
                return GE_CONNECTING;
            }
        }
        stats_initstate = init_awaitchallenge;
    }
    // fall through
    case init_awaitchallenge: {
        int ret = 0;

        if (!CanReceiveOnSocket(sock)) {
            if (current_time() - initstart > inittimeout) {
                stats_initstate = init_failed;
                CloseStatsConnection();
                return GE_TIMEDOUT;
            }
            return GE_CONNECTING;
        }
        ret = recv(sock, rcvbuffer + rcvlen, rcvmax - rcvlen, 0);
        if (ret == -1) {
            stats_initstate = init_failed;
            CloseStatsConnection();
            return GE_NOCONNECT;
        }
        rcvlen += ret;
        rcvmax -= ret;
        if (rcvlen < 38) {
            return GE_CONNECTING;
        }
        rcvbuffer[rcvlen] = '\0';
        stats_initstate = init_awaitsessionkey;
        enc = enc1;
        xcode_buf(rcvbuffer, rcvlen);
        ret = SendChallengeResponse(rcvbuffer, gameport);
        if (ret != GE_NOERROR) {
            stats_initstate = init_failed;
            CloseStatsConnection();
            return ret;
        }
        stats_initstate = init_awaitsessionkey;
        rcvmax += rcvlen;
        rcvlen = 0;
        memset(rcvbuffer, 0, rcvmax);
    }
    // fall through
    case init_awaitsessionkey: {
        int ret = 0;

        if (!CanReceiveOnSocket(sock)) {
            if (current_time() - initstart > inittimeout) {
                stats_initstate = init_failed;
                CloseStatsConnection();
                return GE_TIMEDOUT;
            }
            return GE_CONNECTING;
        }
        ret = RecvSessionKey();
        if (ret != GE_NOERROR) {
            stats_initstate = init_failed;
            CloseStatsConnection();
            return ret;
        }
        rcvmax += rcvlen;
        rcvlen = 0;
        memset(rcvbuffer, 0, rcvmax);
        stats_initstate = init_complete;
    }
    // fall through
    case init_complete:
        return GE_NOERROR;
    default:
        return GE_NOCONNECT;
    }
}

// InitStatsAsync and InitStatsThink until the connection is made or fails
int InitStatsConnection(int gameport_) {
    int aResult = InitStatsAsync(gameport_, 0);

    while (aResult == GE_CONNECTING) {
        aResult = InitStatsThink();
        msleep(5);
    }
    return aResult;
}

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

gsi_bool IsStatsConnected(void) {
    return sock != -1;
}

// The challenge of a game, or the connection's when there is no game
char *GetChallenge(statsgame_t game) {
    static char challenge[9];

    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        create_challenge(connid ^ CHALLENGEXOR, challenge);
        return challenge;
    }
    return game->challenge;
}

// Starts a game, and reports it to the server if connected
statsgame_t NewGame(int usebuckets) {
    statsgame_t game = gsimalloc(sizeof(struct statsgame_s));
    char data[256];
    int len;

    if (!internal_init) {
        InternalInit();
    }
    game->connid = connid;
    game->sesskey = sesskey++;
    game->buckets = NULL;
    game->playernums = NULL;
    game->teamnums = NULL;
    game->usebuckets = usebuckets;
    if (sock != -1) {
        // "\newgame\\connid\%d\sesskey\%d"
        char respformat[] = "\xC\x1C\xA\x1D\x2\x2\x19\x24\x2C\x34\x6\x17\x3E\x1C\x6\xE\x39\x46\x10\x1D\x3\xD\x16\xB\x3B"
                            "\x17\x16\x36\x40\x7";

        enc = enc3;
        xcode_buf(respformat, sizeof(respformat) - 1);
        len = sprintf(data, respformat, game->connid, game->sesskey);
        len = DoSend(data, len);
        if (len <= 0) {
            CloseStatsConnection();
        }
        create_challenge(game->connid ^ CHALLENGEXOR, game->challenge);
    }
    if (sock == -1) {
        gsifree(game);
        game = NULL;
    }
    if (game != NULL && game->usebuckets) {
        game->buckets = NewBucketSet();
        game->playernums = ArrayNew(sizeof(int), 32, NULL);
        game->teamnums = ArrayNew(sizeof(int), 2, NULL);
        game->totalplayers = game->totalteams = 0;
    }
    if (game != NULL) {
        game->sttime = current_time();
    }
    g_statsgame = game;
    return game;
}

void FreeGame(statsgame_t game) {
    if (game == NULL || game == g_statsgame) {
        game = g_statsgame;
        g_statsgame = NULL;
    }
    if (game == NULL) {
        return;
    }
    if (game->usebuckets) {
        if (game->buckets != NULL) {
            FreeBucketSet(game->buckets);
        }
        if (game->playernums != NULL) {
            ArrayFree(game->playernums);
        }
        if (game->teamnums != NULL) {
            ArrayFree(game->teamnums);
        }
    }
    gsifree(game);
}

// Sends a game's data, its buckets' or the text given, with its backslashes escaped
int SendGameSnapShotA(statsgame_t game, const char *snapshot, int final) {
    int snaplen;
    int len;
    int ret = GE_NOERROR;
    char *snapcopy;
    char *data;

    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return GE_DATAERROR;
    }
    if (game->usebuckets) {
        snapcopy = CreateBucketSnapShot(game->buckets);
    } else {
        snapcopy = goastrdup(snapshot);
    }
    snaplen = strlen(snapcopy);
    data = gsimalloc(snaplen + 256);
    while (snaplen--) {
        if (snapcopy[snaplen] == '\\') {
            snapcopy[snaplen] = '\x1';
        }
    }
    if (sock != -1) {
        // "\updgame\\sesskey\%d\connid\%d\done\%d\gamedata\%s"
        char respformat[] = "\xC\x7\x1F\xE\x2\x2\x19\x24\x2C\x34\x16\x1D\x23\x1\x4\xF\x1C\x3F\x51\x25\x2C\xB\xA\x16\x3E"
                            "\x1B\xB\x36\x40\x7\x28\x25\x1F\x6\x00\x24\x75\x16\x33\xD\x4\xE\x11\x25\x11\x1C\x4\x24\x75"
                            "\x1";

        enc = enc3;
        xcode_buf(respformat, sizeof(respformat) - 1);
        len = sprintf(data, respformat, game->sesskey, game->connid, final, snapcopy);
        snaplen = DoSend(data, len);
        if (snaplen <= 0) {
            CloseStatsConnection();
        }
    }
    if (sock == -1) {
        ret = GE_NOCONNECT;
    }
    gsifree(snapcopy);
    gsifree(data);
    return ret;
}

// Numbers a new player and gives it its start time and name
void NewPlayerA(statsgame_t game, int pnum, char *name) {
    int i = -1;

    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return;
    }
    while (pnum >= ArrayLength(game->playernums)) {
        ArrayAppend(game->playernums, &i);
    }
    i = game->totalplayers++;
    ArrayReplaceAt(game->playernums, &i, pnum);
    BucketIntOp(game, "ctime", bo_set, (int)(current_time() - game->sttime) / 1000, bl_player, pnum);
    BucketStringOp(game, "player", bo_set, name, bl_player, pnum);
}

void RemovePlayer(statsgame_t game, int pnum) {
    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return;
    }
    BucketIntOp(game, "dtime", bo_set, (int)(current_time() - game->sttime) / 1000, bl_player, pnum);
}

void NewTeamA(statsgame_t game, int tnum, char *name) {
    int i = -1;

    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return;
    }
    while (tnum >= ArrayLength(game->teamnums)) {
        ArrayAppend(game->teamnums, &i);
    }
    i = game->totalteams++;
    ArrayReplaceAt(game->teamnums, &i, tnum);
    BucketIntOp(game, "ctime", bo_set, (int)(current_time() - game->sttime) / 1000, bl_team, tnum);
    BucketStringOp(game, "team", bo_set, name, bl_team, tnum);
}

void RemoveTeam(statsgame_t game, int tnum) {
    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return;
    }
    BucketIntOp(game, "dtime", bo_set, (int)(current_time() - game->sttime) / 1000, bl_team, tnum);
}

void PreAuthenticatePlayerPartner(int localid, const char *authtoken, const char *challengeresponse,
                                  PersAuthCallbackFn callback, void *instance) {
    // "\authp\\authtoken\%s\resp\%s\lid\%d"
    char respformat[] = "\xC\x13\x1A\x1E\xD\x13\x28\x1D\x11\x1D\x11\x10\x24\x1D\x04\x0F\x0B\x3F\x51\x32\x2C\x1A\x00\x0B"
                        "\x20\x2E\x4A\x19\x39\x0F\x1D\x25\x2C\x4D\x01";
    int len;
    char data[256];

    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    len = sprintf(data, respformat, authtoken, challengeresponse, localid);
    SendPlayerAuthRequest(data, len, localid, callback, instance);
}

void PreAuthenticatePlayerPM(int localid, int profileid, const char *challengeresponse, PersAuthCallbackFn callback,
                             void *instance) {
    // "\authp\\pid\%d\resp\%s\lid\%d"
    char respformat[] = "\xC\x13\x1A\x1E\xD\x13\x28\x1D\x0\x1\x1\x24\x75\x16\x33\x18\x0\x10\x4\x1D\x55\x1B\x39\x14\x39"
                        "\x16\x33\x4F\x1";
    int len;
    char data[256];

    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    len = sprintf(data, respformat, profileid, challengeresponse, localid);
    SendPlayerAuthRequest(data, len, localid, callback, instance);
}

void PreAuthenticatePlayerCDA(int localid, const char *nick, const char *keyhash, const char *challengeresponse,
                              PersAuthCallbackFn callback, void *instance) {
    // "\authp\\nick\%s\keyhash\%s\resp\%s\lid\%d"
    char respformat[] = "\xC\x13\x1A\x1E\xD\x13\x28\x1D\x1E\x1\x6\x13\xC\x57\x1C\x36\xE\x6\xD\x29\x11\x1B\xD\x24\x75"
                        "\x1\x33\x18\x0\x10\x4\x1D\x55\x1B\x39\x14\x39\x16\x33\x4F\x1";
    int len;
    char data[256];

    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    len = sprintf(data, respformat, nick, keyhash, challengeresponse, localid);
    SendPlayerAuthRequest(data, len, localid, callback, instance);
}

void GetProfileIDFromCDA(int localid, const char *nick, const char *keyhash, ProfileCallbackFn callback,
                         void *instance) {
    // "\getpid\\nick\%s\keyhash\%s\lid\%d"
    char respformat[] = "\xC\x15\xA\x1E\x15\xA\x10\x1D\x2C\x6\xC\x1B\x3B\x2E\x4A\x19\x39\x8\x11\x38\x18\x9\x16\x10\xC"
                        "\x57\x1C\x36\x9\xA\x10\x1D\x55\xC";
    int len;
    char data[512];

    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    len = sprintf(data, respformat, nick, keyhash, localid);
    if (sock != -1) {
        len = DoSend(data, len);
    }
    if (len <= 0) {
        CloseStatsConnection();
        if (callback != NULL) {
            callback(0, -1, 0, instance);
        }
    } else {
        AddRequestCallback(rt_profilecb, localid, 0, pd_private_ro, 0, callback, instance);
    }
}

void GetPersistData(int localid, int profileid, persisttype_t type, int index, PersDataCallbackFn callback,
                    void *instance) {
    GetPersistDataValuesModifiedA(localid, profileid, type, index, 0, "", callback, instance);
}

void GetPersistDataModified(int localid, int profileid, persisttype_t type, int index, gsi_u32 modifiedsince,
                            PersDataCallbackFn callback, void *instance) {
    GetPersistDataValuesModifiedA(localid, profileid, type, index, modifiedsince, "", callback, instance);
}

void SetPersistData(int localid, int profileid, persisttype_t type, int index, const char *data, int len,
                    PersDataSaveCallbackFn callback, void *instance) {
    SetPersistDataHelper(localid, profileid, type, index, data, len, callback, instance, 0);
}

void GetPersistDataValuesModifiedA(int localid, int profileid, persisttype_t type, int index, gsi_u32 modifiedsince,
                                   char *keys, PersDataCallbackFn callback, void *instance) {
    // "\getpd\\pid\%d\ptype\%d\dindex\%d\keys\%s\lid\%d"
    char respformat[] = "\xC\x15\xA\x1E\x15\x7\x28\x1D\x0\x1\x1\x24\x75\x16\x33\x1A\x11\x1A\x4\x24\x2C\x4D\x1\x24\x34"
                        "\x1B\x1\xE\x0\x1B\x28\x64\x14\x34\xE\x1D\x29\x1\x33\x4F\x16\x3F\x18\x28\x14\x34\x40\x1C";
    char modformat[] = { '\\', 'm', 'o', 'd', '\\', '%', 'd', '\0' };
    int len;
    char data[512];
    char tempkeys[256];
    char *p;

    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    strcpy(tempkeys, keys);
    // A backslash of the keys goes as a 1
    for (p = tempkeys; *p != '\0'; p++) {
        if (*p == '\\') {
            *p = '\x1';
        }
    }
    len = sprintf(data, respformat, profileid, type, index, tempkeys, localid);
    if (modifiedsince != 0) {
        len += sprintf(data + len, modformat, modifiedsince);
    }
    if (sock != -1) {
        len = DoSend(data, len);
    }
    if (len <= 0) {
        CloseStatsConnection();
        if (callback != NULL) {
            callback(localid, profileid, type, index, 0, 0, "", 0, instance);
        }
    } else {
        AddRequestCallback(rt_datacb, localid, profileid, type, index, callback, instance);
    }
}

void GetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, char *keys,
                           PersDataCallbackFn callback, void *instance) {
    GetPersistDataValuesModifiedA(localid, profileid, type, index, 0, keys, callback, instance);
}

void SetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, const char *keyvalues,
                           PersDataSaveCallbackFn callback, void *instance) {
    SetPersistDataHelper(localid, profileid, type, index, keyvalues, strlen(keyvalues) + 1, callback, instance, 1);
}

gsi_bool PersistThink(void) {
    int len;
    int processed;

    if (sock == -1) {
        return gsi_false;
    }
    if (stats_initstate != init_complete) {
        return gsi_false;
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
                return gsi_false;
            }
        }
        len = recv(sock, rcvbuffer + rcvlen, rcvmax - rcvlen, 0);
        if (len <= 0) {
            CloseStatsConnection();
            return gsi_false;
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
        return gsi_false;
    }
    return gsi_true;
}

int StatsThink(void) {
    return PersistThink();
}

// Sets the first letters of the hidden texts
static void InternalInit(void) {
    internal_init = 1;
    enc1[0] = 'G';
    enc3[0] = 'P';
    finalstr[0] = '\\';
}

// Answers the server's challenge with the game's name and the digest of it with the secret key
static int SendChallengeResponse(const char *indata, int theGameport) {
    static char challengestr[] = { '\0', 'h', 'a', 'l', 'l', 'e', 'n', 'g', 'e', '\0' };
    char *challenge;
    char resp[384];
    char md5val[33];
    // "\auth\\gamename\%s\response\%s\port\%d\id\1"
    char respformat[] = "\xC\x13\x1A\x1E\xD\x3F\x28\x26\x11\x5\x0\x16\x31\x1F\xA\x36\x40\x10\x28\x33\x15\x1B\x15\x17"
                        "\x3E\x1\xA\x36\x40\x10\x28\x31\x1F\x1A\x11\x24\x75\x16\x33\x3\x1\x3F\x45";
    int len;

    challengestr[0] = 'c';
    challenge = value_for_key(indata, challengestr);
    if (challenge == NULL) {
        closesocket(sock);
        return GE_DATAERROR;
    }
    len = sprintf(resp, "%d%s", g_crc32(challenge, strlen(challenge)), gcd_secret_key);
    MD5Digest((unsigned char *)resp, len, md5val);
    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    len = sprintf(resp, respformat, gcd_gamename, md5val, theGameport);
    if (DoSend(resp, len) <= 0) {
        closesocket(sock);
        return GE_NOCONNECT;
    }
    return GE_NOERROR;
}

// Reads the connection's id from the server's answer
static int RecvSessionKey(void) {
    static char sesskeystr[] = { '\0', 'e', 's', 's', 'k', 'e', 'y', '\0' };
    char resp[129];
    char *stext;
    int len = recv(sock, resp, sizeof(resp) - 1, 0);

    if (len == -1) {
        int anError = GOAGetLastError(sock);

        closesocket(sock);
        if (anError != SOC_EWOULDBLOCK && anError != SOC_ETIMEDOUT && anError != SOC_EINPROGRESS) {
            return GE_NOCONNECT;
        } else {
            return GE_DATAERROR;
        }
    }
    resp[len] = '\0';
    enc = enc1;
    xcode_buf(resp, len);
    sesskeystr[0] = 's';
    stext = value_for_key(resp, sesskeystr);
    if (stext == NULL) {
        closesocket(sock);
        return GE_DATAERROR;
    } else {
        connid = atol(stext);
    }
    return GE_NOERROR;
}

// Scrambles a message, ends it with "\final\" and sends it all; returns the bytes sent, or the error
static int DoSend(char *data, int len) {
    int sent = 0;

    enc = enc1;
    xcode_buf(data, len);
    strcpy(data + len, finalstr);
    while (sent < len + 7) {
        int ret = send(sock, data + sent, len + 7 - sent, 0);

        if (ret == -1) {
            int anError = GOAGetLastError(sock);

            if (anError != SOC_EWOULDBLOCK && anError != SOC_ETIMEDOUT && anError != SOC_EINPROGRESS) {
                return anError;
            }
        } else if (ret == 0) {
            return -1;
        } else {
            sent += ret;
        }
    }
    return sent;
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

// The hash of a challenge that the response gives
static int g_crc32(char *s, int len) {
    int i;
    int hashcode = 0;

    for (i = 0; i < len; i++) {
        hashcode = hashcode * -1664117991 + s[i];
    }
    return hashcode;
}

// Writes a challenge as 8 hex digits, each moved up into the letters by its place
static void create_challenge(int challenge, char chstr[9]) {
    char *p = chstr;

    sprintf(chstr, "%08x", challenge);
    while (*p != '\0') {
        *p = *p + ('A' - '0') + (p - chstr);
        p++;
    }
}

// The value of a key in a message, in one of two buffers that the calls take in turn, or NULL
static char *value_for_key(const char *s, const char *key) {
    static int valueindex;
    char *pos;
    char *pos2;
    gsi_u32 index;
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
    index = valueindex;
    pos2 = value[index];
    while (*pos != '\0' && *pos != '\\') {
        *pos2++ = *pos++;
    }
    *pos2 = '\0';
    return value[index];
}

static char *value_for_key_safe(const char *s, const char *key) {
    char *temp = value_for_key(s, key);

    if (temp == NULL) {
        return "";
    }
    return temp;
}

// The address of a host, a text of four numbers or a name to look up, at a port
static int get_sockaddrin(const char *host, int port, SOCKADDR_IN *saddr, HOSTENT **savehent) {
    HOSTENT *hent = NULL;

    memset(saddr, 0, sizeof(SOCKADDR_IN));
    saddr->family = SOC_AF_INET;
    saddr->port = SOC_HtoNs(port);
    if (host == NULL) {
        saddr->addr.addr = 0;
    } else {
        saddr->addr.addr = inet_addr(host);
    }
    if (saddr->addr.addr == -1) {
        hent = SOC_GetHostByName(host);
        if (hent == NULL) {
            return 0;
        }
        saddr->addr.addr = *(gsi_u32 *)hent->addrList[0];
    }
    if (savehent != NULL) {
        *savehent = hent;
    }
    return 1;
}

// Adds a request that waits for the server's answer
static void AddRequestCallback(reqtype_t reqtype, int localid, int profileid, persisttype_t pdtype, int pdindex,
                               void *callback, void *instance) {
    StatsServerRequest req;

    req.callback = callback;
    req.instance = instance;
    req.localid = localid;
    req.reqtype = reqtype;
    req.profileid = profileid;
    req.pdtype = pdtype;
    req.pdindex = pdindex;
    if (serverreqs == NULL) {
        serverreqs = ArrayNew(sizeof(StatsServerRequest), 2, NULL);
    }
    ArrayAppend(serverreqs, &req);
}

// Sends an authentication, and calls the callback with "Connection Lost" if it can't
static void SendPlayerAuthRequest(char *data, int len, int localid, PersAuthCallbackFn callback, void *instance) {
    int sentlen = 0;
    char connerror[] = "\x13\x1D\x1\x4\x0\x0\x0\x28\x1F\x6\x45\x34\x3F\x1\x1B";

    if (sock != -1) {
        sentlen = DoSend(data, len);
    }
    if (sentlen <= 0) {
        CloseStatsConnection();
        enc = enc3;
        xcode_buf(connerror, sizeof(connerror) - 1);
        if (callback != NULL) {
            callback(localid, 0, 0, connerror, instance);
        }
    } else {
        AddRequestCallback(rt_authcb, localid, 0, pd_private_ro, 0, callback, instance);
    }
}

// Sends a player's data to save, all of it or (kvset) only the keys given
static void SetPersistDataHelper(int localid, int profileid, persisttype_t type, int index, const char *data, int len,
                                 PersDataSaveCallbackFn callback, void *instance, int kvset) {
    // "\setpd\\pid\%d\ptype\%d\dindex\%d\kv\%d\lid\%d\length\%d\data\"
    char respformat[] =
        "\xC\x1\xA\x1E\x15\x7\x28\x1D\x0\x1\x1\x24\x75\x16\x33\x1A\x11\x1A\x4\x24\x2C\x4D\x1\x24\x34\x1B"
        "\x1\xE\x0\x1B\x28\x64\x14\x34\xE\xE\xC\x57\xB\x36\x9\xA\x10\x1D\x55\xC\x39\x14\x35\x1C\x8\x1E"
        "\xD\x3F\x51\x25\x2C\xC\x4\xC\x31\x2E";
    int tlen;
    char tdata[512];
    char *senddata;

    enc = enc3;
    xcode_buf(respformat, sizeof(respformat) - 1);
    if (type == pd_private_ro || type == pd_public_ro) {
        if (callback != NULL) {
            callback(localid, profileid, type, index, 0, 0, instance);
        }
        return;
    }
    tlen = sprintf(tdata, respformat, profileid, type, index, kvset, localid, len);
    if (tlen + len < 480) {
        memcpy(tdata + tlen, data, len);
        senddata = tdata;
    } else {
        senddata = gsimalloc(len + tlen + 256);
        memcpy(senddata, tdata, tlen);
        memcpy(senddata + tlen, data, len);
    }
    if (sock != -1) {
        tlen = DoSend(senddata, tlen + len);
    }
    if (tlen <= 0) {
        CloseStatsConnection();
        if (callback != NULL) {
            callback(localid, profileid, type, index, 0, 0, instance);
        }
    } else {
        AddRequestCallback(rt_savecb, localid, profileid, type, index, callback, instance);
    }
    if (senddata != tdata) {
        gsifree(senddata);
    }
}

static int SocketReadable(SOCKET s) {
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
static int FindRequest(reqtype_t reqtype, int localid, int profileid) {
    int i;
    StatsServerRequest *req;

    if (serverreqs == NULL) {
        return -1;
    }
    for (i = 0; i < ArrayLength(serverreqs); i++) {
        req = ArrayNth(serverreqs, i);
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
    ((StatsServerRequest *)ArrayNth(serverreqs, reqindex))->profileid = pid;
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
    ((StatsServerRequest *)ArrayNth(serverreqs, reqindex))->profileid = pid;
    CallReqCallback(reqindex, pid > 0, 0, NULL, 0);
}

// "\getpdr\<success>\lid\<local id>\pid\<profile>\mod\<time>\length\<length>\data\<data>"
static void ProcessGetData(const char *buf, int len) {
    int reqindex;
    int pid;
    int lid;
    int success;
    int length;
    gsi_u32 modified;
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
static void CallReqCallback(int reqindex, int success, gsi_u32 modified, char *data, int length) {
    StatsServerRequest *req;

    if (reqindex < 0 || reqindex >= ArrayLength(serverreqs)) {
        return;
    }
    req = ArrayNth(serverreqs, reqindex);
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
    ArrayDeleteAt(serverreqs, reqindex);
}

// Calls the callbacks of the requests left, when the connection is lost, with "Connection Lost"
static void ClosePendingCallbacks(void) {
    int i;

    if (serverreqs == NULL) {
        return;
    }
    for (i = ArrayLength(serverreqs) - 1; i >= 0; i--) {
        char connerror[] = "\x13\x1D\x1\x4\x0\x0\x0\x28\x1F\x6\x45\x34\x3F\x1\x1B";

        enc = enc3;
        xcode_buf(connerror, sizeof(connerror) - 1);
        CallReqCallback(i, 0, 0, connerror, 0);
    }
    ArrayFree(serverreqs);
    serverreqs = NULL;
}

int GetTeamIndex(statsgame_t game, int tnum) {
    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return tnum;
    }
    return *(int *)ArrayNth(game->teamnums, tnum);
}

int GetPlayerIndex(statsgame_t game, int pnum) {
    if (game == NULL) {
        game = g_statsgame;
    }
    if (game == NULL) {
        return pnum;
    }
    return *(int *)ArrayNth(game->playernums, pnum);
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

static int ServerOpInt(statsgame_t game, char *name, BucketFunc func, int value, int index) {
    int *ret;

    DO_FUNC(func, game, name, &value, bt_int, ret);
    return *ret;
}

static double ServerOpFloat(statsgame_t game, char *name, BucketFunc func, double value, int index) {
    double *ret;

    DO_FUNC(func, game, name, &value, bt_float, ret);
    return *ret;
}

static char *ServerOpString(statsgame_t game, char *name, BucketFunc func, char *value, int index) {
    char *ret;

    DO_FUNC(func, game, name, value, bt_string, ret);
    return ret;
}

static int TeamOpInt(statsgame_t game, char *name, BucketFunc func, int value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_t%d", name, GetTeamIndex(game, index));
    return ServerOpInt(game, fullname, func, value, index);
}

static double TeamOpFloat(statsgame_t game, char *name, BucketFunc func, double value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_t%d", name, GetTeamIndex(game, index));
    return ServerOpFloat(game, fullname, func, value, index);
}

static char *TeamOpString(statsgame_t game, char *name, BucketFunc func, char *value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_t%d", name, GetTeamIndex(game, index));
    return ServerOpString(game, fullname, func, value, index);
}

static int PlayerOpInt(statsgame_t game, char *name, BucketFunc func, int value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_%d", name, GetPlayerIndex(game, index));
    return ServerOpInt(game, fullname, func, value, index);
}

static double PlayerOpFloat(statsgame_t game, char *name, BucketFunc func, double value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_%d", name, GetPlayerIndex(game, index));
    return ServerOpFloat(game, fullname, func, value, index);
}

static char *PlayerOpString(statsgame_t game, char *name, BucketFunc func, char *value, int index) {
    char fullname[64];

    OS_SPrintf(fullname, "%s_%d", name, GetPlayerIndex(game, index));
    return ServerOpString(game, fullname, func, value, index);
}

static char *CreateBucketSnapShot(bucketset_t buckets) {
    return DumpBucketSet(buckets);
}
