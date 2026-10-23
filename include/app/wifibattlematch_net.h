#ifndef POKEBW2_APP_WIFIBATTLEMATCH_NET_H
#define POKEBW2_APP_WIFIBATTLEMATCH_NET_H

#include "types.h"
#include "app/wifibattlematch_link.h"
#include "dwc/competition.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 260's wifibattlematch_net.c (named by its embedded string): the net part of the Wi-Fi battle competitions.
// Its functions have no names yet, only their addresses; what they do is said at each

typedef struct WifiBattleMatchNet WifiBattleMatchNet;

// What the machines' and the requests' main functions return: 0 while they run, 1 once done and 2 when they failed.
// The HTTP requests and the check of Pokémon return 3 when the server answered with an error
enum {
    WBM_NET_RUNNING,
    WBM_NET_SUCCESS,
    WBM_NET_FAILED,
    WBM_NET_SERVER_ERROR,
};

// What WifiBattleMatchNet_DownloadMain returns: 0 while it runs, 1 once the file is in, 2 when there was no file or
// its CRC was wrong, and 3 on an error
enum {
    WBM_DOWNLOAD_RUNNING,
    WBM_DOWNLOAD_SUCCESS,
    WBM_DOWNLOAD_NO_FILE,
    WBM_DOWNLOAD_ERROR,
};

// How an error ends for the player, as WifiBattleMatchNet_CheckError returns it: nothing, the Wi-Fi connection lost,
// the competition failed or the game's network error. They are what DWCRapCommon_CheckError and the error converters
// answer
typedef enum {
    WBM_RESULT_OK,
    WBM_RESULT_DISCONNECTED,
    WBM_RESULT_FAILED,
    WBM_RESULT_NET_ERROR,
    WBM_RESULT_TIMEOUT,
} WifiBattleMatchNetResult;

// The modes of WifiBattleMatchNet_StartMatch, which are the value of the matching key "mod": the Wi-Fi competition's
// and the rating battle's match on the players' ratings, disconnects and cup, and the free battle's takes anybody.
// Mode 1 asserts. WifiBattleMatchNet_StartReport's kind is the competition's or the rating battle's
enum {
    WBM_MATCH_MODE_COMPETITION,
    WBM_MATCH_MODE_UNUSED,
    WBM_MATCH_MODE_RATING,
    WBM_MATCH_MODE_FREE,
};

// The rules of a battle, the matching key "rul" and the format of a report: the report of each has the fields of its
// numbers of wins and losses and of its rating
enum {
    WBM_RULE_SINGLE,
    WBM_RULE_DOUBLE,
    WBM_RULE_TRIPLE,
    WBM_RULE_ROTATION,
    WBM_RULE_SHOOTER,
};

// The reports of WifiBattleMatchNet_StartReport: the one when a battle starts, and the one with its result
enum {
    WBM_REPORT_MODE_START,
    WBM_REPORT_MODE_RESULT,
};

// What WifiBattleMatchNet_StartPokemonCheck checks: one Pokémon's data, or a PokeParty
enum {
    WBM_POKEMON_CHECK_ONE,
    WBM_POKEMON_CHECK_PARTY,
};

// What a player offers in a match, which is sent to the other player whole
typedef struct {
    const GSLoginCertificate *certificate;
    SCIReport *connection;
    u8 unk8[0x28];
    u8 unk30[0x28];
    u8 unk58[0x10];
} WifiBattleMatchNetInfo;

// What overlay 290 shares with this file: the player's ID in the Sake database, the player's side and the other
// player's
typedef struct {
    s32 sake_recordID;
    WifiBattleMatchNetInfo mine;
    WifiBattleMatchNetInfo theirs;
} WifiBattleMatchNetData;

// What the matching of two players is looking for: the rating, the disconnects, the cup and the number of matches
typedef struct {
    s32 rating;
    s32 disconnects;
    s32 cup;
    s32 matches;
} WifiBattleMatchNetKeys;

// The check of a team that the server answers: its status, the result of each Pokémon and the signature
typedef struct {
    u8 status;
    u32 result[6];
    u8 signature[0x80];
} WifiBattleMatchNetCheck;

// The fields of the arena's table, by the number that WifiBattleMatchNet_GetArenaFieldName gives their names for
typedef enum {
    ARENA_DISCONNECTS_COUNTER = 3,
    ARENA_COMPLETE_MATCHES_COUNTER = 4,
    ARENA_NUM_SINGLE_WIN = 9,
    ARENA_NUM_SINGLE_LOSE = 10,
    ARENA_NUM_DOUBLE_LOSE = 11,
    ARENA_NUM_DOUBLE_WIN = 12,
    ARENA_NUM_ROTATE_WIN = 13,
    ARENA_NUM_ROTATE_LOSE = 14,
    ARENA_NUM_TRIPLE_WIN = 15,
    ARENA_NUM_TRIPLE_LOSE = 16,
    ARENA_NUM_SHOOTER_WIN = 17,
    ARENA_NUM_SHOOTER_LOSE = 18,
    ARENA_ELO_RATING_1V1_SINGLE = 19,
    ARENA_ELO_RATING_1V1_DOUBLE = 20,
    ARENA_ELO_RATING_1V1_ROTATE = 21,
    ARENA_ELO_RATING_1V1_TRIPLE = 22,
    ARENA_ELO_RATING_1V1_SHOOTER = 23,
    ARENA_CHEATS_COUNTER = 24,
    ARENA_ELO_RATING_MAXIMUM_STAT = 25,
    ARENA_ELO_RATING_MINIMUM_STAT = 26,
    ARENA_SCORE_WEIGHT_NORMAL = 27,
    ARENA_FLOOR_MODIFIER = 28,
    ARENA_INTERMED_MODIFIER = 29,
    ARENA_ADVANCED_MODIFIER = 30,
    ARENA_MASTER_MODIFIER = 31,
    ARENA_RATING_DIFF_HIGH = 32,
    ARENA_RATING_DIFF_LOW = 33,
    ARENA_NUM_WINLOSE_COUNTER_MINIMUM_STAT = 34,
    ARENA_INITIAL_RATING = 35,
    ARENA_ELO_RATING_1V1_WIFICUP = 36,
    ARENA_NUM_WIFICUP_WIN = 37,
    ARENA_NUM_WIFICUP_LOSE = 38,
    ARENA_CHEATS_WIFICUP_COUNTER = 39,
    ARENA_DISCONNECTS_WIFICUP_COUNTER = 40,
} WifiBattleMatchNetArenaField;

// The kinds of read that WifiBattleMatchNet_StartRead starts: it fills a WifiBattleMatchNetStats, a
// WifiBattleMatchNetCupStats, a record ID, the team of the Wi-Fi cup and the date of the last login
enum {
    WBM_READ_ARENA,
    WBM_READ_WIFICUP,
    WBM_READ_RECORD_ID,
    WBM_READ_WIFICUP_PARTY,
    WBM_READ_LAST_LOGIN,
};

// The kinds of record that WifiBattleMatchNet_StartWrite writes, with the data it takes: the team of the Wi-Fi cup,
// nothing (the date is the current one), nothing, and a WifiBattleMatchNetRecordData
enum {
    WBM_WRITE_WIFICUP,
    WBM_WRITE_LAST_LOGIN,
    WBM_WRITE_PROFILE,
    WBM_WRITE_RECORD_DATA,
};

// The stats of a player in the arena
typedef struct {
    s32 eloRatingDouble;
    s32 eloRatingRotate;
    s32 eloRatingShooter;
    s32 eloRatingSingle;
    s32 eloRatingTriple;
    s32 unk14;
    s32 cheatsCounter;
    s32 unk1C;
    s32 disconnectsCounter;
    s32 numDoubleLose;
    s32 numDoubleWin;
    s32 numRotateLose;
    s32 numRotateWin;
    s32 numShooterLose;
    s32 numShooterWin;
    s32 numSingleLose;
    s32 numSingleWin;
    s32 numTripleLose;
    s32 numTripleWin;
    s32 initialProfileId;
    s32 nowProfileId;
    s32 recordSaveIdx;
    s32 recordId;
} WifiBattleMatchNetStats;

// The stats of a player in the Wi-Fi cup, followed by the team that was registered
typedef struct {
    s32 cheatsCounter;
    s32 disconnectsCounter;
    s32 numWin;
    s32 numLose;
    s32 eloRating;
    s32 recordSaveIdx;
    u8 party[1];
} WifiBattleMatchNetCupStats;

// A record that the competition keeps in one of six slots
typedef struct {
    u8 data[0x50];
    u32 slot;
} WifiBattleMatchNetRecordData;

WifiBattleMatchNet *WifiBattleMatchNet_Create(WifiBattleMatchNetData *data, GameData *gameData, void *loginBuffer,
                                              HeapID heapId);
void WifiBattleMatchNet_Delete(WifiBattleMatchNet *p_wk);
// Does nothing
void WifiBattleMatchNet_Nop(WifiBattleMatchNet *p_wk);
// Starts a timing sync with the number, and checks whether the other machine reached it too
void WifiBattleMatchNet_StartSync(WifiBattleMatchNet *p_wk, u32 syncId);
BOOL WifiBattleMatchNet_CheckSync(WifiBattleMatchNet *p_wk);
// Checks the network's state and the error that was found, returning how it ends (a WifiBattleMatchNetResult)
u32 WifiBattleMatchNet_CheckError(WifiBattleMatchNet *p_wk, u32 a1, u32 a2);
// Resets the state of the machines that follow, whose main functions return WBM_NET_*
void WifiBattleMatchNet_ResetLogin(WifiBattleMatchNet *p_wk);
u32 WifiBattleMatchNet_LoginMain(WifiBattleMatchNet *p_wk);
// Sets the match query and its keys for the mode (WBM_MATCH_MODE_*) and the rule (WBM_RULE_*), and the machine that
// runs it; WBM_NET_SUCCESS once matched
void WifiBattleMatchNet_StartMatch(WifiBattleMatchNet *p_wk, u32 mode, u32 a2, u32 rule,
                                   const WifiBattleMatchNetKeys *keys);
u32 WifiBattleMatchNet_MatchMain(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_CancelMatch(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_ResetEnd(WifiBattleMatchNet *p_wk);
BOOL WifiBattleMatchNet_EndMain(WifiBattleMatchNet *p_wk);
BOOL WifiBattleMatchNet_SendCmd2402(WifiBattleMatchNet *p_wk);
BOOL WifiBattleMatchNet_TakeCmd2402(WifiBattleMatchNet *p_wk);
BOOL WifiBattleMatchNet_SendPerson(WifiBattleMatchNet *p_wk);
BOOL WifiBattleMatchNet_TakePerson(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_SetGuest(WifiBattleMatchNet *p_wk, u32 value);
// Starts a report (WBM_REPORT_MODE_*) of a kind (WBM_MATCH_MODE_COMPETITION or WBM_MATCH_MODE_RATING), with the
// battle's rule (WBM_RULE_*) and result for WBM_REPORT_MODE_RESULT
void WifiBattleMatchNet_StartReport(WifiBattleMatchNet *p_wk, u32 mode, u32 kind, u32 rule,
                                    const WifiBattleMatchBattleResult *battleResult, u32 a5);
u32 WifiBattleMatchNet_ReportMain(WifiBattleMatchNet *p_wk, u32 *out);
u32 WifiBattleMatchNet_GetReceivedValue(WifiBattleMatchNet *p_wk);
// Reads the table of fields of a kind (WBM_READ_*) into out
void WifiBattleMatchNet_StartRead(WifiBattleMatchNet *p_wk, int recordId, int kind, void *out);
u32 WifiBattleMatchNet_ReadMain(WifiBattleMatchNet *p_wk);
// Writes a record of a kind (WBM_WRITE_*)
void WifiBattleMatchNet_StartWrite(WifiBattleMatchNet *p_wk, int kind, void *data);
u32 WifiBattleMatchNet_WriteMain(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_StartCreateRecord(WifiBattleMatchNet *p_wk);
u32 WifiBattleMatchNet_CreateRecordMain(WifiBattleMatchNet *p_wk);
u32 WifiBattleMatchNet_GetRecordId(WifiBattleMatchNet *p_wk);
BOOL WifiBattleMatchNet_SendPlayerInfo(WifiBattleMatchNet *p_wk, const void *data);
BOOL WifiBattleMatchNet_TakePlayerInfo(WifiBattleMatchNet *p_wk, void **out);
// Sends the team picked, and then is TRUE once it is sent
BOOL WifiBattleMatchNet_SendParty(WifiBattleMatchNet *p_wk, PokeParty *party);
// Copies the other player's team into party, and then is TRUE, once it has arrived
BOOL WifiBattleMatchNet_TakeParty(WifiBattleMatchNet *p_wk, PokeParty *party);
BOOL WifiBattleMatchNet_SendValue4(WifiBattleMatchNet *p_wk, const void *data);
BOOL WifiBattleMatchNet_TakeValue4(WifiBattleMatchNet *p_wk, void *out);
BOOL WifiBattleMatchNet_SendReportValue(WifiBattleMatchNet *p_wk, const void *data);
BOOL WifiBattleMatchNet_TakeReportValue(WifiBattleMatchNet *p_wk, u32 *out);
void WifiBattleMatchNet_InitDownload(WifiBattleMatchNet *p_wk, s32 profileId);
u32 WifiBattleMatchNet_DownloadMain(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_GetDownload(WifiBattleMatchNet *p_wk, void *dest);
void WifiBattleMatchNet_StartAccountRequest(WifiBattleMatchNet *p_wk, HeapID heapId);
u32 WifiBattleMatchNet_AccountRequestMain(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_GetAccountValues(WifiBattleMatchNet *p_wk, void *out);
void WifiBattleMatchNet_StartPostRequest(WifiBattleMatchNet *p_wk, const void *in, HeapID heapId);
u32 WifiBattleMatchNet_PostRequestMain(WifiBattleMatchNet *p_wk);
void WifiBattleMatchNet_StartNameCheck(WifiBattleMatchNet *p_wk, PlayerInfo *info, u16 a2);
BOOL WifiBattleMatchNet_NameCheckMain(WifiBattleMatchNet *p_wk, u32 *out);
// Checks a Pokémon or a team, by the kind (WBM_POKEMON_CHECK_*)
void WifiBattleMatchNet_StartPokemonCheck(WifiBattleMatchNet *p_wk, void *data, int kind);
u32 WifiBattleMatchNet_PokemonCheckMain(WifiBattleMatchNet *p_wk, WifiBattleMatchNetCheck *out);
void WifiBattleMatchNet_ShowIcon(WifiBattleMatchNet *p_wk, int a1);
s32 WifiBattleMatchNet_GetGSID(WifiBattleMatchNet *p_wk);

// wifibattlematch_net_fields.c, which follows: the names of the Sake fields. The first two give the arena's table of
// them and its length, and the next two the Wi-Fi cup's; the last gives the name of one of the numbered fields
const char **WifiBattleMatchNet_GetArenaFieldNames(void);
int WifiBattleMatchNet_GetArenaFieldCount(void);
const char **WifiBattleMatchNet_GetWifiCupFieldNames(void);
int WifiBattleMatchNet_GetWifiCupFieldCount(void);
const char *WifiBattleMatchNet_GetArenaFieldName(WifiBattleMatchNetArenaField field);

#endif // POKEBW2_APP_WIFIBATTLEMATCH_NET_H
