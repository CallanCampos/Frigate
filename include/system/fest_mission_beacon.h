#ifndef POKEBW2_SYSTEM_FEST_MISSION_BEACON_H
#define POKEBW2_SYSTEM_FEST_MISSION_BEACON_H

#include "types.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/fest_mission.h"

// The lists the festival keeps of a Funfest mission (fest_mission_beacon.c, a guessed name): the log of what its
// players did, the Pass Powers they gave and the missions that other players offer

// A line of the mission's log: what a player did
typedef struct {
    u16 name[8];
    // The text of a LOG_KIND_TEXT
    u16 text[9];
    u16 trainerId;
    // Whether the player is the one who started the mission, or this game's player
    u8 own : 1;
    u8 gender : 1;
    u8 look : 6;
    // A LOG_KIND_*: the beacon's type - 0x3a for the rest
    u8 kind;
    // What the line is about, by FestMission_LogCategories
    u16 value;
    u16 score;
} FestMissionLogEntry;

// The kinds of a log entry: the player started or joined the mission, a player was met, a Pass Power was used, a
// player's words
enum {
    LOG_KIND_NONE,
    LOG_KIND_START,
    LOG_KIND_JOIN,
    LOG_KIND_MEET,
    LOG_KIND_PASS_POWER = 15,
    LOG_KIND_TEXT = 45,
    LOG_KIND_MAX,
};

#define FEST_MISSION_LOG_MAX 30

typedef struct {
    FestMissionLogEntry entries[FEST_MISSION_LOG_MAX];
    u8 unk4EC;
    u8 count;
    u8 unk4EE;
} FestMissionLog;

// A Pass Power a player gave during the mission
typedef struct {
    u16 name[8];
    u16 passPower;
    // Whether a beacon gave it
    u16 received;
} FestMissionPassPower;

#define FEST_MISSION_PASS_POWER_MAX 2

typedef struct {
    FestMissionPassPower own;
    FestMissionPassPower others[FEST_MISSION_PASS_POWER_MAX];
    s32 count;
} FestMissionPassPowers;

// A mission another player is running, which the player can join
typedef struct {
    u16 name[8];
    FestMissionConfig config;
} FestMissionOffer;

#define FEST_MISSION_OFFER_MAX 3

typedef struct {
    FestMissionOffer entries[FEST_MISSION_OFFER_MAX];
    s32 count;
} FestMissionOffers;

void FestMissionLogEntry_Clear(FestMissionLogEntry *entry);
void FestMissionLogEntry_GetName(const FestMissionLogEntry *entry, StrBuf *dest);
void FestMissionLog_Clear(FestMissionLog *log);
// Logs what a beacon says its player did
void FestMissionLog_AddBeacon(LinkFestival *festival, const GameBeacon *beacon, u32 score, u32 mode, u32 own);
// Logs that the player started or joined the mission
void FestMissionLog_AddPlayer(FestMissionLog *log, GameData *gameData, u8 joined);

void FestMissionPassPower_Clear(FestMissionPassPower *passPower);
void FestMissionPassPowers_Clear(FestMissionPassPowers *passPowers);
// Takes the Pass Power of a beacon of type 0x32, once
BOOL FestMissionPassPowers_AddBeacon(FestMissionPassPowers *passPowers, const GameBeacon *beacon);
// The Pass Power of a log entry
u16 FestMissionPassPower_FromLogEntry(FestMissionPassPower *passPower, const FestMissionLogEntry *entry);

// Keeps the mission of a beacon as an offer
void FestMissionOffers_AddBeacon(LinkFestival *festival, const GameBeacon *beacon);
// Drops the offers that are running out of time
void FestMissionOffers_DropExpired(LinkFestival *festival);
// Drops the offer of a mission
void FestMissionOffers_Remove(LinkFestival *festival, FestMissionConfig *config);
// What a log entry's value is, by its kind
u8 FestMission_GetLogCategory(u32 kind);
// Whether a beacon's mission is not a valid one
BOOL FestMission_IsBeaconInvalid(LinkFestival *festival, const GameBeacon *beacon);

#endif // POKEBW2_SYSTEM_FEST_MISSION_BEACON_H
