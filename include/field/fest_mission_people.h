#ifndef POKEBW2_FIELD_FEST_MISSION_PEOPLE_H
#define POKEBW2_FIELD_FEST_MISSION_PEOPLE_H

#include "types.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_gimmick.h"
#include "gfl/arc.h"
#include "struct_decls.h"

// Overlay 25: the Funfest missions of type 5, on entering a zone. It picks which of the zone's places get a person,
// what each of them is and says, and creates them. No string names the file; the name is a guess from what it does.

// A type 5 mission's settings: what was asked of each person, by the person and the question
typedef struct {
    u16 values[4][6];
} FesMissionAnswers;

extern const FesMissionAnswers FesMissionPeople_Answers[];

// The festival's archive of the missions' data
ArcTool *LinkFestival_GetArchive(LinkFestival *festival);
// The highest three bits of a zone's encounter id: which of the game's 6 stages the zone is in
int GetZoneEncIDHighest3Bits(u16 zoneId);

// On entering a zone: picks the places of the mission's people, then creates them
void FesMissionPeople_OnZoneEnter(RivalEntry *entry, GameSystem *gsys, LinkFestival *festival,
                                  FestMissionConfig *mission, u16 zoneId);
// Reads the species the mission's quiz or people use from the festival's archive into the work
void FesMissionPeople_LoadSpecies(FesGimmick *gimmick, FesMissionWork *work);
// Overlay 25's mission functions: the object code of the people, whether the items are on sale, and a zone's entry
u8 FesMissionPeople_IsOnSale(FestMissionConfig *mission);
u16 FesMissionPeople_GetObjCode(FesMissionWork *work, FestMissionConfig *mission);
FesMissionZone *FesMissionPeople_GetZone(FesMissionWork *work, u16 zoneId);

#endif // POKEBW2_FIELD_FEST_MISSION_PEOPLE_H
