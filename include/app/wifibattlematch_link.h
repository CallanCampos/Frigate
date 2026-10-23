#ifndef POKEBW2_APP_WIFIBATTLEMATCH_LINK_H
#define POKEBW2_APP_WIFIBATTLEMATCH_LINK_H

#include "types.h"
#include "app/wifibattlematch.h"
#include "gfl/heap.h"
#include "gfl/proc.h"

// Overlay 260's wifibattlematch_link.c, wifibattlematch_link_random.c and wifibattlematch_link_competition.c (all
// descriptive names: the ROM names only the next file, wifibattlematch_net.c): the link of the Wi-Fi battle
// competitions, which overlay 290 chains. wifibattlematch_link.c is the proc that overlay 290 runs, which runs one of
// the other two: the random matchups of the field, or the Wi-Fi competition of the title screen's menu. The files are
// three because each has a table of procs, which wifibattlematch_link.c reaches by its symbol, and a string of its
// own for its assert

// The heap of the link, and the one that the exchange of the match results gives the Wi-Fi library
#define HEAPID_WIFIBATTLEMATCH_LINK 0x57
#define HEAPID_WIFIBATTLEMATCH_LINK_DWC 0x86

// What the link leaves for overlay 290 to do next (WifiBattleMatchLinkParam.action); overlay 290 also knows a 4,
// which logs out like the 0
enum {
    WIFIBATTLEMATCH_LINK_ACTION_LOGOUT,
    WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST,
    WIFIBATTLEMATCH_LINK_ACTION_RESULT,
    WIFIBATTLEMATCH_LINK_ACTION_RELOGIN,
};

// What the link writes to the server after a battle: overlay 290's report, and the word that
// WifiBattleMatchLinkParam.unk28 points to
typedef struct {
    WifiBattleMatchReport report;
    u32 unk50;
} WifiBattleMatchLinkReportBuf;

// What the server says of the player's account: the Game Sync ID in the first word, and in the bytes after it the
// state of the competition and whether the Game Sync ID is registered
typedef struct {
    s32 profileId;
    u8 unk4;
    u8 unk5;
    u8 registered;
    u8 unk7;
} WifiBattleMatchLinkAccount;

// What the server says of the ratings, as 5 ratings, 5 pairs of results in the order that the server has the battle
// types in, and the data of the Wi-Fi competition's ranking
typedef struct {
    u32 rating[5];
    u32 unk14[3];
    u32 unk20;
    struct {
        u32 second;
        u32 first;
    } results[5];
    u32 unk4C;
    u32 unk50;
    u32 unk54;
    u32 unk58;
} WifiBattleMatchLinkRatings;

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
    u32 unk14;
    u8 body[0x534];
} WifiBattleMatchLinkRanking;

// What overlay 290 gives the link
typedef struct {
    u32 mode;
    // What the link leaves for overlay 290 to do next, a WIFIBATTLEMATCH_LINK_ACTION_* or 4
    u32 action;
    WifiBattleMatchParam *param;
    u32 result;
    RndMatchData *record;
    WifiBattleMatchPlayer *player;
    WifiBattleMatchPlayer *otherPlayer;
    void *unk1C;
    u32 *unk20;
    WifiBattleMatchReport *report;
    u32 *unk28;
    WifiBattleMatchBattleResult *battleResult;
    WifiBattleMatchLinkAccount *account;
    WifiBattleMatchLinkRanking *unk34;
    WifiBattleMatchLinkRatings *ratings;
    void *unk3C;
    BOOL *unk40;
    u32 unk44;
} WifiBattleMatchLinkParam;

// The proc of the link, which overlay 290 chains
extern const GameProcFunctions WIFIBATTLEMATCH_LINK_PROC_FUNCTIONS;
// The procs that it runs: the random matchups, and the Wi-Fi competition
extern const GameProcFunctions WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS;
extern const GameProcFunctions WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_LINK_H
