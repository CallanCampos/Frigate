#ifndef POKEBW2_FIELD_FESTIVAL_H
#define POKEBW2_FIELD_FESTIVAL_H

#include "types.h"
#include "field/rival_select.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "struct_decls.h"
#include "system/fest_mission_beacon.h"

// 8 bytes that the festival keeps in three ways
typedef union {
    // Unused: index is FEST_MISSION_NONE
    struct {
        u32 index : 7;
    } header;
    // A mission the player has seen: its config's unk00[0] and unk1C
    struct {
        u32 unk0;
        u32 unk4;
    } mission;
    // Another player in the current mission
    struct {
        u16 trainerId;
        u16 zoneId;
        u32 score;
    } member;
} FestMissionRecord;

#define LINK_FESTIVAL_RECORD_COUNT 99

// The player with the highest score of the current mission
typedef struct {
    StrBuf *name;
    u16 score;
    u16 trainerId;
} LinkFestivalBest;

// What LinkFestival_GetRanking tells of the current mission
typedef struct {
    // The players in the mission, with the player
    u8 count;
    // The player's place among them, from 1
    u8 rank;
    u16 score;
    u16 total;
} LinkFestivalRanking;

// The Funfest's work, which the game system holds (GSYS_GetLinkFestival)
struct LinkFestival {
    GameSystem *gsys;
    GameData *gameData;
    // 0 with no mission, 1 once one starts, 2 while its time runs, 3 when it is over
    u8 state : 7;
    // Set when best changes
    u8 bestChanged : 1;
    u8 recordCount;
    // Where the next seen mission goes in records
    u8 recordIndex;
    // The frames between two calls of GameBeaconSys_SendMission, and those left until the next
    u16 beaconInterval;
    u16 beaconTimer;
    // The total and the player's score that the field's mission panel shows, and the seconds gained that it has not
    // shown yet
    u16 shownTotal;
    u16 shownScore;
    u16 secondsGained;
    u16 zoneId;
    // The total that secondsGained was last raised for
    u16 lastTotal;
    ArcTool *archive;
    // File 0 of the archive: every mission's FestMissionHeader (field/fest_mission_data.h)
    FestMissionHeader *headers;
    FestMissionConfig config;
    // The two parts of config
    FestMissionInfo *info;
    // config's FestMissionHeader
    FestMissionHeader *missionHeader;
    FestMissionRecord records[LINK_FESTIVAL_RECORD_COUNT];
    // The current mission
    FestMissionRecord current;
    LinkFestivalBest *best;
    // The log that the mission viewer lists, which it moves the entries of log into
    FestMissionLog viewLog;
    FestMissionLog log;
    FestMissionPassPowers passPowers;
    FestMissionOffers offers;
    // What LinkFestival_GetEntries (field/rival_select.h) gives
    RivalEntry entries[224];
};

// A mission taken from a beacon (FestMissionEntry_Clear, FestMissionConfig_FromBeacon, LinkFestival_UpdateMember)
struct FestMissionEntry {
    FestMissionConfig config;
    // The sender, as a member
    FestMissionRecord record;
};

FesGimmick *Field_GetFesGimmick(Field *field);
BOOL FesGimmick_IsCurrent(FesGimmick *gimmick, u32 type);
// Whether the Hidden Grotto has a Funfest mission
BOOL hollowFesMissions(FesGimmick *gimmick, u32 id);
// Whether the Funfest mission keeps phenomena off the tile
BOOL func_ov036_021b6758(FesGimmick *gimmick, u16 x, u16 z, fx32 height);
// Changes the party of a Funfest mission's battle, when gimmick 5 is current
void func_ov036_021b67d8(FesGimmick *gimmick, PokeParty *party);
// The number the salesman's messages are offset by, when gimmick 5 is current
u32 func_ov036_021b67bc(FesGimmick *gimmick);
void DeleteFunfestActor(FesGimmick *gimmick, u16 zoneId, u8 actorIndex);
void func_ov036_021b65e8(FesGimmick *gimmick);
BOOL func_ov036_021b6690(FesGimmick *gimmick);
// The event of the Funfest mission at a grid or rail position, or NULL
GameEvent *func_ov036_021b6700(FesGimmick *gimmick, const VecFx32 *position);
GameEvent *func_ov036_021b672c(FesGimmick *gimmick, const RailPosition *position);
u32 LinkFestival_GetNormalChangeBGMID(LinkFestival *festival);
FestMissionConfig *GetFestMissionCfg(LinkFestival *festival);
// Overlay 12's fest_mission_field.c: the zone a mission uses in place of a zone, a random state seeded for the
// mission, a random number below max that the list doesn't have, and the mission's work on entering a zone
u16 FestMission_GetMissionZone(u16 zoneId);
void FestMission_InitRand(u32 seed, int skip, MATHRandContext32 *rand);
u32 FestMission_RandExcluding(MATHRandContext32 *rand, u32 max, const u32 *list, int count);
LinkFestival *CreateLinkFestival(HeapID heapId, GameSystem *gsys);
void FreeLinkFestival(LinkFestival *festival);
void LinkFestival_Update(LinkFestival *festival);
// Copies the current mission's beacon data to mission, with value
void LinkFestival_FillMissionInfo(LinkFestival *festival, void *mission, u16 value);
// The player's score in the mission
u16 LinkFestival_GetScore(LinkFestival *festival);
// The mission's seconds left
u16 LinkFestival_GetSecondsLeft(LinkFestival *festival);
void LinkFestival_GetRanking(LinkFestival *festival, LinkFestivalRanking *ranking);
FestMissionLog *LinkFestival_GetLog(LinkFestival *festival);
FestMissionLog *LinkFestival_GetViewLog(LinkFestival *festival);
FestMissionPassPowers *LinkFestival_GetPassPowers(LinkFestival *festival);
// Copies a mission's FestMissionHeader to dest
void FetchFestMissionCfgGimmickID(LinkFestival *festival, u8 index, FestMissionHeader *dest);
void LinkFestival_StartMission(LinkFestival *festival, const FestMissionConfig *mission, u8 joined);
void LinkFestival_SetState(LinkFestival *festival, u32 state);
int LinkFestival_GetState(LinkFestival *festival);
u16 LinkFestival_GetShownTotal(LinkFestival *festival);
u16 LinkFestival_GetShownScore(LinkFestival *festival);
void LinkFestival_SetShownTotal(LinkFestival *festival, u16 value);
u16 LinkFestival_GetSecondsGained(LinkFestival *festival);
void LinkFestival_ClearSecondsGained(LinkFestival *festival);
BOOL LinkFestival_IsNewZoneFromOutside(LinkFestival *festival, u16 zoneId);
// Whether the festival's total has reached what the mission wants; total gets it
BOOL LinkFestival_IsTargetReached(LinkFestival *festival, u32 *total);
// Ends the mission when it is cleared or out of time, and says whether it did
BOOL LinkFestival_IsMissionOver(LinkFestival *festival);
u32 LinkFestival_AddMissionScore(GameSystem *gsys, u32 a1, u32 a2);
// The players in the mission, or 0 without one
int LinkFestival_GetMemberCount(LinkFestival *festival);
// Remembers a mission seen while none is on
void LinkFestival_AddRecord(LinkFestival *festival, void *mission);
// Whether a mission of a beacon is not to be offered
BOOL LinkFestival_ShouldSkipOffer(LinkFestival *festival, FestMissionConfig *config);
// A score for best: the player's own when name is NULL
void LinkFestival_UpdateBest(LinkFestival *festival, u16 trainerId, u16 value, StrBuf *name);
BOOL LinkFestival_IsBestChanged(LinkFestival *festival);
BOOL LinkFestival_IsBestMine(LinkFestival *festival);
void LinkFestival_GetBest(LinkFestival *festival, LinkFestivalBest *dest);
LinkFestivalBest *LinkFestivalBest_Create(HeapID heapId);
void LinkFestivalBest_Free(LinkFestivalBest *best);
void FestMissionEntry_Clear(FestMissionEntry *entry);
int LinkFestival_UpdateMember(LinkFestival *festival, FestMissionEntry *entry, u32 *out);
void LinkFestival_StartTimer(LinkFestival *festival, u32 arg1);
u32 GetTrainerCardTextMSGID(u32 type);
FieldFMissionInfo *FieldFMissionInfo_Create(GameSystem *gsys, HeapID heapId);
void FieldFMissionInfo_Free(FieldFMissionInfo *work);
void FieldFMissionInfo_Update(FieldFMissionInfo *work);
void FieldFMissionInfo_UpdateDisplay(FieldFMissionInfo *work);
FesGimmick *FesGimmick_Create(GameSystem *gsys, Field *field, HeapID heapId);
void FesGimmick_Free(FesGimmick *gimmick);
void FesGimmick_BindActorSystem(FesGimmick *gimmick, MMSys *actorSystem);
void FesGimmick_BindPlayer(FesGimmick *gimmick, FieldEffects *effects, FieldPlayer *player);
void func_ov036_021b65d4(FesGimmick *gimmick);
void func_ov036_021b6660(FesGimmick *gimmick);

#endif // POKEBW2_FIELD_FESTIVAL_H
