#ifndef POKEBW2_DWC_GSTATS_H
#define POKEBW2_DWC_GSTATS_H

#include "dwc/darray.h"
#include "dwc/gbucket.h"
#include "dwc/nonport.h"

// GameSpy's stats and persistent data (gstats.c), in overlay 11: the connection to the stats server
// ("gamestats.gs.nintendowifi.net"), whose messages are texts of "\key\value" pairs scrambled with a key and ended by
// "\final\", the requests that wait for its answers, and the buckets of a game, its teams and its players

// The game's name and secret key, which DWC's start copies in and its Sake and competition clients send
extern char gcd_gamename[256];
extern char gcd_secret_key[256];
// The host of the stats server
extern char StatsServerHostname[64];

typedef struct statsgame_s *statsgame_t;

// A game whose stats are reported: its buckets, and the indexes of its players' and teams' buckets by their numbers
struct statsgame_s {
    int connid;
    int sesskey;
    gsi_bool usebuckets;
    bucketset_t buckets;
    char challenge[9];
    DArray playernums; // of int
    DArray teamnums;   // of int
    // The counts of players and teams that NewPlayerA and NewTeamA number, and the game's start, from GameSpy's SDK
    // (UniSpySDK's copy): only the dead-stripped code uses them
    int totalplayers;
    int totalteams;
    gsi_time sttime;
};

// The results of InitStatsAsync, InitStatsThink and InitStatsConnection
#define GE_NOERROR 0
#define GE_NOSOCKET 1
#define GE_NODNS 2
#define GE_NOCONNECT 3
#define GE_BUSY 4
#define GE_DATAERROR 5
#define GE_CONNECTING 6
#define GE_TIMEDOUT 7

// The steps of InitStatsAsync's connection, which InitStatsThink takes
typedef enum {
    init_none,
    init_failed,
    init_connecting,
    init_awaitchallenge,
    init_awaitsessionkey,
    init_complete,
} initstate_t;

// Who may read and write a player's persistent data
typedef enum {
    pd_private_ro,
    pd_private_rw,
    pd_public_ro,
    pd_public_rw,
} persisttype_t;

// The callbacks of the requests: an authentication, data read, data saved and a profile found
typedef void (*PersAuthCallbackFn)(int localid, int profileid, int authenticated, char *errmsg, void *instance);
typedef void (*PersDataCallbackFn)(int localid, int profileid, persisttype_t type, int index, int success,
                                   gsi_u32 modified, char *data, int len, void *instance);
typedef void (*PersDataSaveCallbackFn)(int localid, int profileid, persisttype_t type, int index, int success,
                                       gsi_u32 modified, void *instance);
typedef void (*ProfileCallbackFn)(int localid, int profileid, int success, void *instance);

// The operations and the levels of the bucket macros
typedef enum {
    bo_set,
    bo_add,
    bo_sub,
    bo_mult,
    bo_div,
    bo_concat,
    bo_avg,
} bucketop_t;

typedef enum {
    bl_server,
    bl_team,
    bl_player,
} bucketlevel_t;

typedef void *(*BucketFunc)(bucketset_t set, char *name, void *value);
typedef int (*SetIntFunc)(statsgame_t game, char *name, BucketFunc func, int value, int index);
typedef double (*SetFloatFunc)(statsgame_t game, char *name, BucketFunc func, double value, int index);
typedef char *(*SetStringFunc)(statsgame_t game, char *name, BucketFunc func, char *value, int index);

// The bucket functions of the operations, set, add, sub, mult, div, concat and avg, and the functions that apply one
// to a bucket of the game, a team or a player (int, float and string for each)
extern BucketFunc bucketfuncs[7];
extern void *bopfuncs[3][3];

// Applies an operation to an int or a text bucket of the game, a team or a player
#define BucketIntOp(game, name, operation, value, bucketlevel, index)                                                  \
    (((SetIntFunc)bopfuncs[bucketlevel][bt_int])(game, name, bucketfuncs[operation], value, index))
#define BucketStringOp(game, name, operation, value, bucketlevel, index)                                               \
    (((SetStringFunc)bopfuncs[bucketlevel][bt_string])(game, name, bucketfuncs[operation], value, index))

// GameSpy's stats and persistent data functions that the game doesn't call, which the original link dead-stripped.
// Reconstructed from GameSpy's SDK (UniSpySDK's copy), with no code in the ROM to check them against
// (config/deadstrip.toml)
char *GenerateAuthA(const char *challenge, const char *password, char response[33]);
int InitStatsAsync(int theGamePort, gsi_time theInitTimeout);
int InitStatsThink(void);
int InitStatsConnection(int gameport_);
char *GetChallenge(statsgame_t game);
statsgame_t NewGame(int usebuckets);
void FreeGame(statsgame_t game);
int SendGameSnapShotA(statsgame_t game, const char *snapshot, int final);
void NewPlayerA(statsgame_t game, int pnum, char *name);
void RemovePlayer(statsgame_t game, int pnum);
void NewTeamA(statsgame_t game, int tnum, char *name);
void RemoveTeam(statsgame_t game, int tnum);
void PreAuthenticatePlayerPartner(int localid, const char *authtoken, const char *challengeresponse,
                                  PersAuthCallbackFn callback, void *instance);
void PreAuthenticatePlayerPM(int localid, int profileid, const char *challengeresponse, PersAuthCallbackFn callback,
                             void *instance);
void PreAuthenticatePlayerCDA(int localid, const char *nick, const char *keyhash, const char *challengeresponse,
                              PersAuthCallbackFn callback, void *instance);
void GetProfileIDFromCDA(int localid, const char *nick, const char *keyhash, ProfileCallbackFn callback,
                         void *instance);
void GetPersistData(int localid, int profileid, persisttype_t type, int index, PersDataCallbackFn callback,
                    void *instance);
void GetPersistDataModified(int localid, int profileid, persisttype_t type, int index, gsi_u32 modifiedsince,
                            PersDataCallbackFn callback, void *instance);
void SetPersistData(int localid, int profileid, persisttype_t type, int index, const char *data, int len,
                    PersDataSaveCallbackFn callback, void *instance);
void GetPersistDataValuesModifiedA(int localid, int profileid, persisttype_t type, int index, gsi_u32 modifiedsince,
                                   char *keys, PersDataCallbackFn callback, void *instance);
void GetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, char *keys,
                           PersDataCallbackFn callback, void *instance);
void SetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, const char *keyvalues,
                           PersDataSaveCallbackFn callback, void *instance);
int StatsThink(void);

void CloseStatsConnection(void);
gsi_bool IsStatsConnected(void);
// Reads and handles what the server sent; returns gsi_false once the connection is closed or not set up
gsi_bool PersistThink(void);
// The index of a team's or a player's buckets in the game
int GetTeamIndex(statsgame_t game, int tnum);
int GetPlayerIndex(statsgame_t game, int pnum);

#endif // POKEBW2_DWC_GSTATS_H
