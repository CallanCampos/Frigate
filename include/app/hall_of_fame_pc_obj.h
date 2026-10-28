#ifndef POKEBW2_APP_HALL_OF_FAME_PC_OBJ_H
#define POKEBW2_APP_HALL_OF_FAME_PC_OBJ_H

#include "types.h"
#include "app/hall_of_fame_pc_main.h"

// The Hall of Fame PC's actors, in hall_of_fame_pc_obj.c (a guessed name). Our names

void HallOfFamePCObj_Init(HallOfFamePCWork *wk);
void HallOfFamePCObj_Exit(HallOfFamePCWork *wk);
void HallOfFamePCObj_Animate(HallOfFamePCWork *wk);
void HallOfFamePCObj_SetVisible(HallOfFamePCWork *wk, u32 id, BOOL visible);
void HallOfFamePCObj_SetAnime(HallOfFamePCWork *wk, u32 id, u32 sequence);
BOOL HallOfFamePCObj_IsAnimating(HallOfFamePCWork *wk, u32 id);
void HallOfFamePCObj_SetPos(HallOfFamePCWork *wk, u32 id, s16 x, s16 y);
void HallOfFamePCObj_GetPos(HallOfFamePCWork *wk, u32 id, s16 *x, s16 *y);
// Loads the page's Pokémon into the free set of actors and frees the other set
void HallOfFamePCObj_LoadPokemon(HallOfFamePCWork *wk);
// The first actor of the set that is shown
u32 HallOfFamePCObj_GetPokeBase(HallOfFamePCWork *wk);
// Puts a Pokémon's actor on the ring at an angle in degrees
void HallOfFamePCObj_SetPokePos(HallOfFamePCWork *wk, u32 id, s32 angle);
// Draws the Pokémon at the front of the ring over those behind
void HallOfFamePCObj_SortPriority(HallOfFamePCWork *wk);
// Sets how dark each Pokémon is by its place behind the front one, or lights them all
void HallOfFamePCObj_SetPokeFade(HallOfFamePCWork *wk, BOOL on);

#endif // POKEBW2_APP_HALL_OF_FAME_PC_OBJ_H
