#ifndef POKEBW2_FIELD_ENCOUNTER_H
#define POKEBW2_FIELD_ENCOUNTER_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Names and layouts from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
struct EncountSystem {
    Field *field;
    GameSystem *gsys;
    GameData *gameData;
    EncData *encData;
    EffectEncountState *effectEncountState;
};

// Where a phenomenon (shaking grass, a dust cloud, rippling water or a flying shadow) is
typedef struct {
    u16 zoneId;
    s16 x;
    s16 y;
    s16 z;
    fx32 height;
    // The phenomenon, an ENCOUNT_EFFECT_ value that MapTile_GetEncounterEffectKind gives
    u8 kind;
    u8 active : 4;
    u8 suspended : 2;
    u8 triggered : 2;
} RareEncountSpot;

struct EncountState {
    s16 x;
    s16 y;
    s16 z;
    // The encounter rate climbs by its config's increase each time the block counter, which grows by the step
    // increment, reaches its threshold
    u8 rateBlockCounter;
    u8 rateStepIncrement;
    u32 rateStepCounter;
    u32 terrain;
    u16 encountRate;
    u8 unk12[2];
    // The steps since the last phenomenon
    u32 phenomenonSteps;
    RareEncountSpot rareSpot;
};

// How the encounter rate of a kind of encounter climbs while the player walks
typedef struct {
    u8 increase;
    u8 threshold;
    u8 max;
    u8 steps;
} EncountRateConfig;

// Names and layouts from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
typedef struct {
    u16 species : 11;
    u16 form : 5;
    u8 minLevel;
    u8 maxLevel;
} WildEncSlot;

// A slot's form that picks one of the species' forms at random
#define WILD_ENC_FORM_RANDOM 0x1f

// A zone's wild encounters for a season, 0xe8 bytes of archive 0x7f
struct EncData {
    // The encounter rate of each kind of encounter (ENCTYPE_*), 0 for none
    u8 userData[7];
    u8 flags : 7;
    u8 fishingEnable : 1;
    WildEncSlot grass[12];
    WildEncSlot grassRare[12];
    WildEncSlot grassShaking[12];
    WildEncSlot surf[5];
    WildEncSlot surfRare[5];
    WildEncSlot fishing[5];
    WildEncSlot fishingRare[5];
};

extern const char data_ov012_0216e240[];

BOOL EncData_Load(EncData *encData, ArcTool *arc, u16 zoneId, u8 season);
EncountState *EncountState_Create(HeapID heapId);
void EncountState_Free(EncountState *state);
void EncountState_SetTerrain(EncountState *state, u32 terrain);
void func_ov012_0215917c(GameData *gameData, Field *field);
void GameData_ResetPhenomenonSteps(GameData *gameData);
void func_ov012_021591f4(GameData *gameData);
u16 EncountSave_GetRoamingPkmZone(EncountSave *save, u8 slot);
u16 getSwarmLevelRangeFromData(GameData *gameData);
u32 EncountSave_HasActiveEntry(EncountSave *save);
void EncountSave_SpawnEntry(GameData *gameData, u8 slot);
void EncountSave_SpawnPendingEntries(GameData *gameData);
// A weather that overrides the zone's, which is always WEATHER_NONE
u32 GetDefaultWeatherValue(GameData *gameData, u16 zoneId);
// Whether weathers 6 and 7 go back to the zone's own, which is always FALSE
BOOL EncountSave_IsAnyEntryActive(GameData *gameData);
// The phenomena (shaking grass, dust clouds, rippling water and flying shadows) of effect_encount.c
EffectEncountState *EffectEncountState_Create(HeapID heapId);
void EffectEncountState_Free(EffectEncountState *state);
void PrepareFieldEncountSystem(EncountSystem *system, EffectEncountState *state);
void EffectEncount_Exit(EncountSystem *system, EffectEncountState *state);
void EffectEncount_LoadSetting(EncountSystem *system, EffectEncountState *state);
void UpdatePhenomenon(EncountSystem *system);
BOOL EncountSystem_CancelPhenomenonIfActorHit(EncountSystem *system);
GameEvent *CreateRandomPhenomenonEvent(EncountSystem *system);
// The distance to the phenomenon, along the axis it is farther on, and whether there is one
BOOL EffectEncount_GetSpotDistance(EncountSystem *system, u16 *distance);
// Hides or shows the phenomenon's effect
void EffectEncount_SetEffectHidden(EncountSystem *system, u32 hidden);
// The item the phenomenon gave
u16 EffectEncount_GetItemId(EncountSystem *system);
// The wild encounters of field_encount.c
EncountSystem *EncSys_Create(Field *field);
void EncSys_Free(EncountSystem *system);
BOOL EncSys_IsActive(EncountSystem *system, u32 unused);
void EncSys_Update(EncountSystem *system, BOOL moved, BOOL turned);
// The flags of a static encounter that make its Pokémon: shiny, without its held item, with its hidden ability, never
// shiny, male or female. The others change the battle
#define STATIC_ENC_FLAG_SHINY 0x2
#define STATIC_ENC_FLAG_NO_ITEM 0x4
#define STATIC_ENC_FLAG_HIDDEN_ABILITY 0x8
#define STATIC_ENC_FLAG_NOT_SHINY 0x10
#define STATIC_ENC_FLAG_MALE 0x20
#define STATIC_ENC_FLAG_FEMALE 0x40

GameEvent *EventWildBattleCall_CreateStatic(EncountSystem *system, u16 species, u8 level, u8 form, u16 flags);
u32 ConvFieldWeatherToBtl(Field *field);
void EffectEncount_ConsumeSpot(EncountSystem *system);
// Sets the setup up for a battle against the trainer, here
// The style is 0 to 3 for single, double, triple and rotation; a double battle is against two trainers with a
// partner, and a single one with a second trainer
void BtlSetup_SetTrainerLocal(EncountSystem *encountSystem, BtlSetup *setup, u32 style, u32 partnerId, u32 trainerId,
                              u32 trainerId2, HeapID heapId);
// The battle's encounter effect and music, against the wild species or the trainer
void EventBattleCall_DecideEnvWild(s32 species, s32 form, s32 encountMode, BOOL isDouble, Field *field, u32 *encEffect,
                                   u16 *bgm);
void EventBattleCall_DecideEnvTrainer(u32 trainerId, Field *field, u32 *encEffect, u16 *bgm);

void EncountSystem_CancelPhenomenon(EncountSystem *encountSystem);
BOOL EncountState_CheckSpecialEncountPos(EncountSystem *encounter, const s16 *gridPos);
// The setup of the wild battle on the fishing rod, or NULL for none; rare is for rippling water
BtlSetup *BtlSetup_CreateFishing(EncountSystem *encounter, BOOL rare);

#endif // POKEBW2_FIELD_ENCOUNTER_H
