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
};

typedef void *(*BucketFunc)(bucketset_t set, char *name, void *value);
typedef int (*SetIntFunc)(statsgame_t game, char *name, BucketFunc func, int value, int index);
typedef double (*SetFloatFunc)(statsgame_t game, char *name, BucketFunc func, double value, int index);
typedef char *(*SetStringFunc)(statsgame_t game, char *name, BucketFunc func, char *value, int index);

// The bucket functions of the operations, set, add, sub, mult, div, concat and avg, and the functions that apply one
// to a bucket of the game, a team or a player (int, float and string for each)
extern BucketFunc bucketfuncs[7];
extern void *bopfuncs[3][3];

void CloseStatsConnection(void);
gsi_bool IsStatsConnected(void);
// Reads and handles what the server sent; returns gsi_false once the connection is closed or not set up
gsi_bool PersistThink(void);
// The index of a team's or a player's buckets in the game
int GetTeamIndex(statsgame_t game, int tnum);
int GetPlayerIndex(statsgame_t game, int pnum);

#endif // POKEBW2_DWC_GSTATS_H
