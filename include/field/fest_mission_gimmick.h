#ifndef POKEBW2_FIELD_FEST_MISSION_GIMMICK_H
#define POKEBW2_FIELD_FEST_MISSION_GIMMICK_H

#include "types.h"
#include "field/fest_mission_data.h"
#include "field/zone.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The field side of the Funfest missions of type 5, overlay 72: the people they place on the field, the item sale,
// the Pokémon quiz and the level boost of a mission's battle. Overlay 36's Funfest gimmick (FesGimmick_*) and the
// scripts of overlay 33 call it. No string names the file; the name is a guess.

// A zone the mission places people in
typedef struct {
    u16 zoneId;
    // How many people the mission placed in it
    u16 count;
    // Whether each person was taken away
    u8 removed[4];
} FesMissionZone;

// The work of a type 5 mission, kept by the festival (func_02014864)
typedef struct {
    LinkFestival *festival;
    u16 zoneId;
    // How many places the current zone has for people
    u8 placeCount;
    // How many people the current zone has
    u8 actorCount;
    // Where the people can stand
    ZoneBGEntity places[20];
    // The place of each person
    u8 placeIndex[4];
    // What each person is told by the mission's target: three values, for the actor's user parameters
    u16 params[4][3];
    FieldActor *actors[4];
    s32 zoneCount;
    FesMissionZone zones[6];
    u32 speciesCount;
    u16 species[640];
} FesMissionWork;

// The Pokémon quiz
typedef struct {
    // How many Pokémon were seen
    u8 count;
    // Which of them is the answer
    u8 answer;
    // Which choice is the answer
    u8 answerChoice;
    u16 species[10];
    u16 choices[5];
} FesMissionQuiz;

// Overlay 36's Funfest gimmick, as Field_GetFesGimmick returns it (overlay 73's RivalSelectContext is the same one)
typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    LinkFestival *festival;
    FestMission *mission;
    u8 unk10[8];
    Field *field;
    MMSys *actors;
    FieldPlayer *player;
    FieldActor *playerActor;
    EventData *eventData;
    EventWork *eventWork;
    void *effects;
    void *railSystem;
    u32 unk38;
    void *unk3C;
    FesMissionWork *work;
    FesMissionQuiz quiz;
} FesGimmick;

// The festival's total, at most 9999
int LinkFestival_GetTotalScore(LinkFestival *festival);

// The first actor on a rail position, other than exclude
FieldActor *func_ov036_0219584c(MMSys *system, const RailPosition *position, BOOL checkInit, FieldActor *exclude);

void func_ov072_021e8be0(FesGimmick *gimmick, FesMissionWork *work);
void func_ov072_021e8bf8(FesGimmick *gimmick, FesMissionWork *work);
void func_ov072_021e8c08(FesGimmick *gimmick, FesMissionWork *work, u16 zoneId, int index);
BOOL func_ov072_021e8c3c(FesGimmick *gimmick, FesMissionWork *work);
void func_ov072_021e8c74(FesGimmick *gimmick, PokeParty *party);
void func_ov072_021e8d08(FesGimmick *gimmick, FieldActor *actor, u16 arg0, u16 arg1, u16 *out0, u16 *out1);
void func_ov072_021e8d70(FesGimmick *gimmick, FieldActor *actor, u16 *item, u16 *price);
void func_ov072_021e8ddc(FesGimmick *gimmick, u16 *count, u16 *answer, u16 *answerChoice);
u16 func_ov072_021e8ee8(FesGimmick *gimmick, u8 index);
u16 func_ov072_021e8ef4(FesGimmick *gimmick, u8 index);

#endif // POKEBW2_FIELD_FEST_MISSION_GIMMICK_H
