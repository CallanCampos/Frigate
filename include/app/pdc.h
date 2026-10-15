#ifndef POKEBW2_APP_PDC_H
#define POKEBW2_APP_PDC_H

// Overlay 172's pdc.c, after its embedded name: catching a Pokémon of the Entree Forest. Overlay 330's
// event_pdc_return.c (app/event_pdc_return.h) shows the result. The process is started by overlay 12's
// scrcmd_entree_forest.c; its other files, pdc_main.c and pdc_msg.c, read the work through the getters

#include "types.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// What the scene is given, 0x20 bytes. Only pdc.c reads the fields
typedef struct PdcWork PdcWork;

// Overlay 172
PdcWork *PdcWork_Create(GameData *gameData, PartyPkm *pkm, BtlFieldStatus *status, PlayerInfo *playerInfo, BagSave *bag,
                        TrainerDataSave *trainerData, PokeDexSave *pokedex, HeapID heapId);
GameData *PdcWork_GetGameData(const PdcWork *work);
PartyPkm *PdcWork_GetPkm(const PdcWork *work);
BtlFieldStatus *PdcWork_GetFieldStatus(const PdcWork *work);
PlayerInfo *PdcWork_GetPlayerInfo(const PdcWork *work);
BagSave *PdcWork_GetBag(const PdcWork *work);
TrainerDataSave *PdcWork_GetTrainerData(const PdcWork *work);
PokeDexSave *PdcWork_GetPokedex(const PdcWork *work);
// Whether the Pokémon was caught
BOOL PdcWork_IsCaught(const PdcWork *work);

// The process's functions: init, main and exit
extern const GameProcFunctions PDC_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PDC_H
