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

// The work of a type 5 mission, kept by the festival (LinkFestival_GetEntries)
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
    int zoneCount;
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

// Overlay 36's Funfest gimmick, as Field_GetFesGimmick returns it. Overlays 26, 71 and 73 take it too
struct FesGimmick {
    GameSystem *gsys;
    GameData *gameData;
    LinkFestival *festival;
    FestMission *mission;
    // Whether overlay 71, 72 or 73 is loaded for the mission, and which
    BOOL overlayLoaded;
    u32 overlayId;
    Field *field;
    MMSys *actors;
    FieldPlayer *player;
    FieldActor *playerActor;
    EventData *eventData;
    EventWork *eventWork;
    FieldEffects *effects;
    FieldRailSystem *railSystem;
    u32 unk38;
    // The sparkles of a mission of type 4, which overlay 71 shows
    FieldEffectTask *sparkles;
    // The mission's work in the festival's entries, by the mission's kind: a FesMissionWork, a FestSparkleWork or
    // the hollows' RivalEntry list
    void *work;
    FesMissionQuiz quiz;
};

// The festival's total, at most 9999
int LinkFestival_GetTotalScore(LinkFestival *festival);

void FesMissionGimmick_LoadActorResource(FesGimmick *gimmick, FesMissionWork *work);
void FesMissionGimmick_RemoveBlockedPeople(FesGimmick *gimmick, FesMissionWork *work);
void FesMissionGimmick_RemovePerson(FesGimmick *gimmick, FesMissionWork *work, u16 zoneId, int index);
BOOL FesMissionGimmick_End(FesGimmick *gimmick, FesMissionWork *work);
void FesMissionGimmick_BoostPartyLevels(FesGimmick *gimmick, PokeParty *party);
void FesMissionGimmick_GetItemExchangeInfo(FesGimmick *gimmick, FieldActor *actor, u16 arg0, u16 arg1, u16 *out0,
                                           u16 *out1);
void FesMissionGimmick_GetItemSaleInfo(FesGimmick *gimmick, FieldActor *actor, u16 *item, u16 *price);
void FesMissionGimmick_MakePokemonQuiz(FesGimmick *gimmick, u16 *count, u16 *answer, u16 *answerChoice);
u16 FesMissionGimmick_GetQuizSpecies(FesGimmick *gimmick, u8 index);
u16 FesMissionGimmick_GetQuizChoice(FesGimmick *gimmick, u8 index);

#endif // POKEBW2_FIELD_FEST_MISSION_GIMMICK_H
