#ifndef POKEBW2_SYSTEM_FEST_MISSION_H
#define POKEBW2_SYSTEM_FEST_MISSION_H

#include "types.h"
#include "field/fest_mission_data.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A Funfest mission's config (fest_mission.c, named by the ROM's string). ClearFesMissionCfg, isFesMissionAvailable,
// allocateForFestMission and UpdateFesMissionConfigSecondsTime are swan's names
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#define FEST_MISSION_NONE 0x7f
#define FEST_MISSION_SCORE_MAX 9999
#define FEST_MISSION_SECONDS_MAX 3599

// The part of a Funfest mission that its beacons send: the first 0x14 bytes of a FestMissionConfig, laid out as
// FestMission's (field/fest_mission_data.h)
typedef struct {
    // FEST_MISSION_NONE when there is no mission
    u32 index : 7;
    u32 level : 3;
    u32 target : 10;
    u32 unk0_20 : 10;
    u32 unk0_30 : 1;
    u32 unk0_31 : 1;
    // The item of the mission's reward
    u32 unk4_0 : 10;
    u32 unk4_10 : 7;
    // The total score the mission wants
    u32 count : 14;
    u32 unk4_31 : 1;
    u32 unk8_0 : 18;
    u32 unk8_18 : 12;
    u32 unk8_30 : 2;
    // What a log entry of the mission's beacon is about: an item, a move, a Pass Power or a species
    u32 unkC_0 : 10;
    // The mission's seconds left
    u32 seconds : 12;
    // A second item, or 0
    u32 unkC_22 : 10;
    u16 unk10_0 : 6;
    u16 unk10_6 : 3;
    // Which of FestMission_ExtraSeconds a gain of time counts
    u16 unk10_9 : 2;
    u16 unk10_11 : 5;
    u8 messageId;
    u8 resultMessageId;
} FestMissionInfo;

// A Funfest mission's config, as GetFestMissionCfg returns it. A mission's beacons send its first 0x14 bytes
struct FestMissionConfig {
    // unk00[0] and unk1C together tell one mission from another
    union {
        FestMissionInfo info;
        u32 unk00[5];
    };
    // header.kind is the kind of mission, which picks the overlay that runs it on the field
    union {
        FestMissionHeader header;
        struct {
            u8 type;
            u8 unk15[3];
        };
    };
    // The player's score in the mission, at most FEST_MISSION_SCORE_MAX
    u16 missionId;
    u8 unk1A;
    u32 unk1C;
    u32 unk20;
    // When the mission's time started, in the RTC's seconds, or -1 until it starts (func_02014dac)
    s64 unk24;
};

FestMissionConfig *allocateForFestMission(HeapID heapId);
void ClearFesMissionCfg(FestMissionConfig *config);
// Copies a mission to start it, or to join it when joined is set
void FestMissionConfig_Start(FestMissionConfig *config, const FestMissionConfig *src, u8 joined);
// Starts the mission's time
void func_02014dac(FestMissionConfig *config);
void func_02014db4(FestMissionConfig *config);
void UpdateFesMissionConfigSecondsTime(FestMissionConfig *config);
// The mission's seconds left
u32 FestMissionConfig_GetSecondsLeft(FestMissionConfig *config);
// Whether the mission has 30 seconds left
BOOL FestMissionConfig_HasTimeLeft(FestMissionConfig *config);
BOOL isFesMissionAvailable(FestMissionConfig *config);
// Whether a log entry of this kind and value is of the mission's target
BOOL FestMissionConfig_IsTarget(FestMissionConfig *config, u32 kind, u32 value);
// Adds count to the score when the entry is of the mission's target
BOOL FestMissionConfig_AddScore(FestMissionConfig *config, u32 kind, u32 value, int count);
// Gives the mission more time, gain times, and returns the seconds given
u32 FestMissionConfig_AddSeconds(FestMissionConfig *config, u16 gain);
// Whether the two are the same mission
BOOL FestMissionConfig_IsSame(const FestMissionConfig *config, const FestMissionConfig *other);
// The mission of a beacon; FALSE when the beacon's type has none
BOOL FestMissionConfig_FromBeacon(FestMissionConfig *config, const GameBeacon *beacon);

#endif // POKEBW2_SYSTEM_FEST_MISSION_H
