#ifndef POKEBW2_BATTLE_POKEWOOD_CUTIN_H
#define POKEBW2_BATTLE_POKEWOOD_CUTIN_H

// Overlay 167's pokewood_cutin.c (named by its string): the cut-ins of the Pokestar Studios movies

#include "types.h"
#include "battle/btl_main.h"
#include "struct_decls.h"

PokewoodCutin *PokewoodCutin_Create(HeapID heapId);
void PokewoodCutin_Delete(PokewoodCutin *cutin);
void PokewoodCutin_Main(PokewoodCutin *cutin);
BOOL PokewoodCutin_IsDone(PokewoodCutin *cutin);
void PokewoodCutin_Start(PokewoodCutin *cutin, const BtlScriptedRules *rules, s8 scene, u16 choice, u32 gender);
void PokewoodCutin_SetViewCore(PokewoodCutin *cutin, BtlvCore *viewCore);
// unused is a debug flag the client passes, which the function ignores
BOOL PokewoodCutin_HasCutin(const BtlScriptedRules *rules, s8 scene, u8 result, u32 unused);

#endif // POKEBW2_BATTLE_POKEWOOD_CUTIN_H
