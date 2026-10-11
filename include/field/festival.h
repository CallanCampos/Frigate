#ifndef POKEBW2_FIELD_FESTIVAL_H
#define POKEBW2_FIELD_FESTIVAL_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "struct_decls.h"

// A Funfest mission's config, as GetFestMissionCfg returns it. A mission's beacons send its first 0x14 bytes
struct FestMissionConfig {
    u32 unk00[5];
    // The kind of mission, which picks the overlay that runs it on the field
    u8 type;
    u8 unk15[3];
    u16 missionId;
    u16 unk1A;
    u32 unk1C;
    u32 unk20;
    s32 unk24;
    s32 unk28;
};

// A mission taken from a beacon (func_02014c78, func_02014fb0, func_02014c94)
struct FestMissionEntry {
    FestMissionConfig config;
    u16 trainerId;
    u16 zoneId;
    u32 missionId;
};

void *Field_GetFesGimmick(Field *field);
BOOL FesGimmick_IsCurrent(void *gimmick, u32 type);
// Whether the Funfest mission keeps phenomena off the tile
BOOL func_ov036_021b6758(void *gimmick, u16 x, u16 z, fx32 height);
// Changes the party of a Funfest mission's battle, when gimmick 5 is current
void func_ov036_021b67d8(void *gimmick, PokeParty *party);
// The number the salesman's messages are offset by, when gimmick 5 is current
u32 func_ov036_021b67bc(void *gimmick);
void DeleteFunfestActor(void *gimmick, u16 zoneId, u8 actorIndex);
void func_ov036_021b65e8(void *gimmick);
void func_ov036_021b6690(void *gimmick);
u32 LinkFestival_GetNormalChangeBGMID(LinkFestival *festival);
void *GetFestMissionCfg(LinkFestival *festival);
// Overlay 12's fest_mission_field.c: the zone a mission uses in place of a zone, a random state seeded for the
// mission, a random number below max that the list doesn't have, and the mission's work on entering a zone
u16 func_ov012_02168320(u16 zoneId);
void func_ov012_02168348(u32 seed, int skip, MATHRandContext32 *rand);
u32 func_ov012_021683a8(MATHRandContext32 *rand, u32 max, const u32 *list, int count);
BOOL isFesMissionAvailable(void *missionCfg);
// Copies the current mission's beacon data to mission, with value
void func_02014594(LinkFestival *festival, void *mission, u16 value);
void *func_020146fc(LinkFestival *festival);
void func_0201472c(LinkFestival *festival, void *mission, u8 joined);
void func_02014964(LinkFestival *festival, void *mission);
// The mission's seconds left
s32 func_02014dcc(void *mission);
void func_020150dc(void *work, GameData *gameData, u8 joined);
void func_020153b8(LinkFestival *festival, void *mission);
u16 func_020145d8(LinkFestival *festival);
void *func_02014710(LinkFestival *festival);
int func_020147bc(LinkFestival *festival);
BOOL func_02014844(LinkFestival *festival, u16 zoneId);
u32 func_02014920(GameSystem *gsys, u32 a1, u32 a2);
void func_02014a00(LinkFestival *festival, u16 trainerId, u16 value, StrBuf *name);
void func_02014c78(FestMissionEntry *entry);
int func_02014c94(LinkFestival *festival, FestMissionEntry *entry, u32 *out);
void func_02014fb0(FestMissionEntry *entry, const GameBeacon *beacon);
void func_020150a8(LinkFestival *festival, const GameBeacon *beacon, u32 a2, u32 a3, u32 a4);
BOOL func_020151c0(void *work, const GameBeacon *beacon);
void func_0201528c(LinkFestival *festival, const GameBeacon *beacon);
BOOL func_02015458(LinkFestival *festival, const GameBeacon *beacon);
void func_02014774(LinkFestival *festival, u32 arg1);
u32 GetTrainerCardTextMSGID(u32 type);
void *func_ov036_021b5b7c(GameSystem *gsys, HeapID heapId);
void func_ov036_021b5bfc(void *work);
void func_ov036_021b5c28(void *work);
void func_ov036_021b5c78(void *work);
void *FesGimmick_Create(GameSystem *gsys, Field *field, HeapID heapId);
void FesGimmick_Free(void *gimmick);
void FesGimmick_BindActorSystem(void *gimmick, MMSys *actorSystem);
void FesGimmick_BindPlayer(void *gimmick, void *effects, FieldPlayer *player);
void func_ov036_021b65d4(void *gimmick);
void func_ov036_021b6660(void *gimmick);

#endif // POKEBW2_FIELD_FESTIVAL_H
