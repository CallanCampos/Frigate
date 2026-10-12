#ifndef POKEBW2_SYSTEM_GAME_BEACON_H
#define POKEBW2_SYSTEM_GAME_BEACON_H

#include "types.h"
#include "gfl/heap.h"
#include "save/join_avenue.h"
#include "struct_decls.h"

// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except the GameBeaconSys_*, GameBeacon_* and
// GameBeaconSendSlot_* names and the types' names, which are ours. The global GameBeaconSys is named after the ROM's
// assertion "GameBeaconSys == NULL"; swan calls it g_GameBeaconSys

// The game's beacons (game_beacon.c): the beacon this game sends, which tells other games what the player is doing,
// and a log of the beacons received from them

// How many received beacons the log keeps
#define GAME_BEACON_LOG_MAX 30
// The characters of a beacon's free text
#define GAME_BEACON_TEXT_LEN 10
// The beacon types are below this
#define GAME_BEACON_TYPE_MAX 0x68
// A beacon's passPower when no Pass Power is active
#define GAME_BEACON_PASS_POWER_NONE 0x30
// A beacon's recentKind when it has no recent event
#define GAME_BEACON_RECENT_NONE 5

// What a beacon's type sends
typedef union {
    // A value (an item, a move, a Pass Power, a number of hours)
    u16 value;
    // Free text, such as a nickname
    u16 text[GAME_BEACON_TEXT_LEN];
    // Type 0x18: a message to one trainer
    struct {
        u16 text[9];
        u16 trainerId;
    } message;
    // Types 0x19, 0x1a, 0x1b and 0x32: a distribution, which key identifies
    struct {
        u16 species;
        u16 item;
        u16 passPower;
        u16 unk06;
        u32 key;
    } gift;
    // Types 0x3c to 0x67: the Funfest mission's data (func_02014594), and for type 0x67 a text
    struct {
        u32 unk00;
        u16 text[8];
    } mission;
    // A Join Avenue visitor's shop choice and extra data, of the type's argument (func_02037970)
    JoinAvenueBeaconPayload avenue;
    // The other views overlay 12's game_beacon_set.c fills its messages through
    u32 value32;
    u8 value8;
    struct {
        u8 a;
        u8 b;
    } pair;
    struct {
        u16 value;
        u8 extra;
    } withExtra;
    struct {
        u16 name[9];
        u16 value;
    } named;
    struct {
        u16 name[9];
        u8 value;
    } namedByte;
} GameBeaconPayload;

struct GameBeacon {
    // A bit for each game version that may receive it, (version - VERSION_WHITE)
    u8 targetVersions;
    u8 country;
    u8 region;
    u8 passPower : 7;
    u8 gender : 1;
    u32 unk04 : 17;
    u32 zoneId : 10;
    u32 language : 5;
    u32 unk08 : 17;
    // Incremented each time the beacon changes, so that receivers tell a new beacon from one already seen
    u32 serial : 5;
    u32 parentZoneId : 10;
    u32 version : 6;
    u32 trainerView : 3;
    u32 surveyRank : 3;
    u32 favoriteColor : 4;
    u32 playHours : 10;
    u32 playMinutes : 6;
    u16 trainerId;
    u16 name[7];
    u16 greeting[8];
    u16 type;
    union {
        u16 value;
        struct {
            u16 missionId : 14;
            u16 unk : 2;
        } mission;
    } arg;
    GameBeaconPayload payload;
    // The last notable event: a species (kinds 0 and 1), an item (kinds 2 to 4), or GAME_BEACON_RECENT_NONE
    u16 recentKind : 8;
    u16 cgear0 : 3;
    u16 cgear1 : 5;
    u16 cgear2;
    u16 cgear3;
    u16 recentValue;
    u8 surveyAnswers[10];
    u8 medalCount;
    u8 unk5B;
    u32 missionWord;
};

// When a received beacon arrived; copied as one halfword
typedef union {
    u16 raw;
    struct {
        u8 minute;
        u8 hour : 7;
        // Whether the sender's survey answers were counted
        u8 surveyCounted : 1;
    };
} GameBeaconTime;

typedef struct {
    GameBeacon beacon;
    // Frames since it was received, up to 0xffff
    u16 age;
    GameBeaconTime time;
} GameBeaconLogEntry;

// The beacon this game sends
typedef struct {
    GameBeacon beacon;
    // Frames in the current type, which reverts to 0 after its lifetime
    s16 timer;
    // Set by every change; GameBeaconSys_SendIfUpdated sends the beacon and clears it
    u8 updated;
} GameBeaconSendSlot;

struct GameBeaconSystem {
    GameSystem *gsys;
    GameData *gameData;
    GameBeaconSendSlot mine;
    GameBeaconLogEntry log[GAME_BEACON_LOG_MAX];
    // A bit for each log entry not yet shown
    u32 newMask;
    u32 recvCount;
    s8 logOldest;
    s8 logNewest;
    s8 logCount;
    u8 surveyUpdated;
    u16 playHours;
    u8 enabled;
    // The Join Avenue advertisement that the beacon sends next, 0 to 6
    u8 avenueAdIndex;
    JoinAvenuePersonList *avenuePeople;
    u8 avenueAdTimer;
    u8 avenueAdRequest;
    u8 noticeFlags;
    u8 avenueVisitorPending;
    // Whether the beacon is sent to Black 2 and White 2 only
    BOOL b2w2Only;
    StrBuf *nameBuf;
    void *avenueWork;
};

extern GameBeaconSystem *GameBeaconSys;

void GameBeaconSys_Create(HeapID heapId);
void GameBeaconSys_SetGameSystem(GameSystem *gsys);
// Called once a frame
void GameBeaconSys_Update(void);
// Builds the game's beacon from the save
void GameBeaconSys_SetGameData(GameData *gameData);
// Copies the game's beacon as it is sent
void GameBeaconSys_GetSendBeacon(GameBeacon *dest);
void GameBeaconSys_ToggleB2W2Only(void);
void GameBeaconSys_SendIfUpdated(void);
void GameBeaconSys_SetNotice(u32 bit);
void GameBeaconSys_ClearNotice(u32 bit);
BOOL GameBeaconSys_CheckNotice(u32 bit);
// Logs a received beacon; FALSE if it was rejected or already seen
BOOL GameBeaconSys_Receive(const GameBeacon *beacon);
// The log's entry at index, and when it was received
GameBeacon *GameBeaconSys_GetLog(int index, GameBeaconTime *time);
// The beacon received n-th, counting from the first
GameBeacon *GameBeaconSys_GetRecent(u32 n);
u32 GameBeaconSys_GetReceiveCount(void);
// The first log entry at or after *index not yet shown, which it advances past that entry, or 30 when there is none
int GameBeaconSys_GetNextNew(int *index);
void GameBeaconSys_ClearNew(int index);
// Marks the log's entry from beacon's sender as not yet shown
BOOL GameBeaconSys_MarkNew(const GameBeacon *beacon);
// The age of the logged beacon of a trainer, or 0xffff
u16 GameBeaconSys_GetAge(u16 trainerId);
// Refresh the beacon's header for a new type, sent to all versions or to Black 2 and White 2 only
void GameBeaconSendSlot_Reset(GameBeaconSendSlot *slot);
void GameBeaconSendSlot_ResetB2W2Only(GameBeaconSendSlot *slot);
// Set a Funfest mission beacon of type, if its priority allows; for type 0x67, value is a StrBuf
void GameBeaconSendSlot_SetMission(GameBeaconSendSlot *slot, u16 type, u32 value);
void GameBeaconSendSlot_SetMissionEx(GameBeaconSendSlot *slot, u16 type, u32 value, u32 extra);
// Whether the beacon's type is a Funfest mission's, 0x3c to 0x67
BOOL GameBeacon_IsMissionType(const GameBeacon *beacon);
// Whether the beacon's type is 0x3c to 0x66
BOOL func_0202cf98(const GameBeacon *beacon);
// 1 + func_02014920's result while a Funfest mission is on, else 0
u32 func_0202cfac(u32 a0, u16 a1);
// Whether a beacon of type may replace the current one, by their priorities
BOOL GameBeaconSys_CanSendType(u16 type);
// Whether species is one of the special Pokémon (legendaries and such)
BOOL GameBeacon_IsSpecialSpecies(u16 species);
void GameBeacon_StoreText(const StrBuf *str, u16 *dest);
// Whether a received beacon changed the survey's results since the last call
u8 GameBeaconSys_PopSurveyUpdated(void);
void GameBeaconSys_SetSurveyAnswers(const void *answers);
void GameBeaconSys_SetCountryRegion(u8 country, u8 region);
void GameBeaconSys_SetSurveyRank(u8 rank);
// Sets the C-Gear record, a sentence (CGearPhrases_Get)
void GameBeaconSys_SetCGearRecord(const PMSData *record);
void GameBeaconSys_SetTrainerView(u32 trainerView);
// Sets the greeting from the save, after the player changed it
void GameBeaconSys_UpdateGreeting(void);
// Sets the medal count that the game's beacon sends
void GameBeaconSys_SetMedalCount(u8 count);
void func_0202d194(u8 value);
// Sends that the player caught species, of a wild battle of the kinds a1 and a2
void GameBeaconSys_SendCapture(u16 species, BOOL a1, BOOL a2);
// Sends a Funfest mission beacon for a defeated species, of a wild battle of the kinds a1 and a2
void func_0202d28c(u16 species, BOOL a1, BOOL a2);
// Sends a beacon of type 0x10 with a battle Pokémon's nickname
void func_0202d2c8(const StrBuf *nickname);
void GameBeaconSys_SendEvolution(u16 species, const StrBuf *nickname);
// Sends a beacon of type 0x23 with an item, or the Funfest mission's
void func_0202d384(u16 item);
void GameBeacon_SetZone(u16 zoneId, GameData *gameData);
// Set the beacon's recent event, of kinds 0 to 4
void func_0202d4c8(GameBeacon *beacon, u16 species);
void func_0202d4e0(GameBeacon *beacon, u16 species);
void func_0202d4fc(GameBeacon *beacon, u16 item);
void func_0202d518(GameBeacon *beacon, u16 item);
void func_0202d534(GameBeacon *beacon, u16 item);
void GameBeacon_ClearRecent(GameBeacon *beacon);
// Sends the Funfest mission beacon of type 0x3c
void func_0202d5cc(void);
void GameBeaconSys_SetAvenuePeople(JoinAvenuePersonList *list);
// Checks overlay 338; when the check fails, the received beacons are hidden every HBlank
void func_0202d6a8(void);

// In another file of ARM9 main
// Whether a received beacon is invalid
BOOL func_02013bd4(const GameBeacon *beacon);
// The beacon's type, or 1 for a type of 0 or of GAME_BEACON_TYPE_MAX or more
u16 func_02013eac(const GameBeacon *beacon);

// The log of received beacons that BeaconStatus keeps. Copies the beacon at index into dest and its time into time,
// and returns whether there is one
BOOL func_02013b44(BeaconStatus *status, GameBeacon *dest, u16 *time, u32 index);
// How many beacons the log holds
u8 func_02013b80(BeaconStatus *status);
// Allocates a beacon, cleared
GameBeacon *func_02013b8c(HeapID heapId);
// Whether the beacon's type is one of a distribution, 0x19, 0x1a, 0x1b or 0x32
BOOL func_02013c88(const GameBeacon *beacon);
// Loads the beacon's trainer name, 8 characters, into a StrBuf
void func_02013cac(const GameBeacon *beacon, StrBuf *dest);
// The beacon's kind, the low 6 bits of its word at 0xc
u32 func_02013cc0(const GameBeacon *beacon);
// The medal that the beacon's sender wears last, or 0xff, and how many medals it has
u8 func_02013dc8(const GameBeacon *beacon);
u8 func_02013dd0(const GameBeacon *beacon);

#endif // POKEBW2_SYSTEM_GAME_BEACON_H
