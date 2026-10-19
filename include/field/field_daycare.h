#ifndef POKEBW2_FIELD_FIELD_DAYCARE_H
#define POKEBW2_FIELD_FIELD_DAYCARE_H

#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The event that picks a party Pokémon to leave at the day care
typedef struct {
    GameSystem *gsys;
    Field *field;
    PokeListParam *partyParam;
    PStatusParam *summaryParam;
    u16 *result;
} DayCarePokeSelectWork;

DayCareSave *Field_GetDayCare(Field *field);
BOOL DayCare_CheckSpawnFlag(DayCareSave *dayCare);
void DayCare_Breed(DayCareSave *dayCare, PokeParty *party);
void DayCare_CheckResetSeed(DayCareSave *dayCare);
GameEvent *EventDayCarePokeSelect_Create(GameSystem *gsys, Field *field, u16 *result);
GameEventReturnCode EventDayCarePokeSelect_Callback(GameEvent *event, u32 *state, void *data);
void DayCare_AddPkm(DayCareSave *dayCare, PokeParty *party, u32 slot);
void DayCare_RemovePkm(DayCareSave *dayCare, u32 slot, PokeParty *party);
u32 DayCare_GetPkmCount(DayCareSave *dayCare);
u32 DayCare_CalcEggSpawnChance(DayCareSave *dayCare);
PartyPkm *DayCare_GetPkm(DayCareSave *dayCare, u32 slot);
u32 DayCare_CalcNewLevel(DayCareSave *dayCare, u32 slot);
u32 DayCare_CalcLevelGain(DayCareSave *dayCare, u32 slot);
u32 DayCare_CalcWithdrawCost(DayCareSave *dayCare, u32 slot);
u32 getNameGenderStatus(PartyPkm *pkm);
void *DayCare_Create(HeapID heapId, Field *field, DayCareSave *save);
void DayCare_Free(void *dayCare);
// Counts a step of the Pokémon left there, and returns TRUE when an egg appears
BOOL DayCare_Update(DayCareSave *dayCare);

#endif // POKEBW2_FIELD_FIELD_DAYCARE_H
