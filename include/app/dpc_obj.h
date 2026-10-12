#ifndef POKEBW2_APP_DPC_OBJ_H
#define POKEBW2_APP_DPC_OBJ_H

#include "types.h"
#include "app/dpc_main.h"

// The Hall of Fame PC's actors, in dpc_obj.c (a guessed name). Our names

void DpcObj_Init(DendouPCWork *wk);
void DpcObj_Exit(DendouPCWork *wk);
void DpcObj_Animate(DendouPCWork *wk);
void DpcObj_SetVisible(DendouPCWork *wk, u32 id, BOOL visible);
void DpcObj_SetAnime(DendouPCWork *wk, u32 id, u32 sequence);
BOOL DpcObj_IsAnimating(DendouPCWork *wk, u32 id);
void DpcObj_SetPos(DendouPCWork *wk, u32 id, s16 x, s16 y);
void DpcObj_GetPos(DendouPCWork *wk, u32 id, s16 *x, s16 *y);
// Loads the page's Pokémon into the free set of actors and frees the other set
void DpcObj_LoadPokemon(DendouPCWork *wk);
// The first actor of the set that is shown
u32 DpcObj_GetPokeBase(DendouPCWork *wk);
// Puts a Pokémon's actor on the ring at an angle in degrees
void DpcObj_SetPokePos(DendouPCWork *wk, u32 id, s32 angle);
// Draws the Pokémon at the front of the ring over those behind
void DpcObj_SortPriority(DendouPCWork *wk);
// Sets how dark each Pokémon is by its place behind the front one, or lights them all
void DpcObj_SetPokeFade(DendouPCWork *wk, BOOL on);

#endif // POKEBW2_APP_DPC_OBJ_H
