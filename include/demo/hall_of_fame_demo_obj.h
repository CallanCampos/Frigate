#ifndef POKEBW2_DEMO_HALL_OF_FAME_DEMO_OBJ_H
#define POKEBW2_DEMO_HALL_OF_FAME_DEMO_OBJ_H

#include "types.h"
#include "demo/hall_of_fame_demo_main.h"

// The Hall of Fame entry demo's actors and the texts drawn on sprites, in hall_of_fame_demo_obj.c (a guessed name). Our
// names

void HallOfFameDemoObj_Init(HallOfFameDemoWork *wk, u32 scene);
void HallOfFameDemoObj_Exit(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_SetPos(HallOfFameDemoWork *wk, u32 id, s16 x, s16 y);
void HallOfFameDemoObj_GetPos(HallOfFameDemoWork *wk, u32 id, s16 *x, s16 *y);
void HallOfFameDemoObj_Move(HallOfFameDemoWork *wk, u32 id, s16 x, s16 y);
void HallOfFameDemoObj_SetVisible(HallOfFameDemoWork *wk, u32 id, BOOL visible);
void HallOfFameDemoObj_SetAnime(HallOfFameDemoWork *wk, u32 id, u32 sequence);
BOOL HallOfFameDemoObj_IsAnimating(HallOfFameDemoWork *wk, u32 id);
// Loads the Pokémon being shown, in place of the last one
void HallOfFameDemoObj_AddPoke(HallOfFameDemoWork *wk);
// Gives the actors' palette the colors of the Pokémon's type
void HallOfFameDemoObj_LoadTypePalette(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_PrintPokeInfo(HallOfFameDemoWork *wk);
// Keeps the first scene's texts on their plates
void HallOfFameDemoObj_MoveFontOam(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_SetFontOamVisible(HallOfFameDemoWork *wk, u32 id, BOOL visible);
void HallOfFameDemoObj_Init1st(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_Exit1st(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_Main1st(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_Init2nd(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_Exit2nd(HallOfFameDemoWork *wk);
void HallOfFameDemoObj_Main2nd(HallOfFameDemoWork *wk);
// Removes the confetti that have fallen, and drops a new one every other frame or so
void HallOfFameDemoObj_MainConfetti(HallOfFameDemoWork *wk);

#endif // POKEBW2_DEMO_HALL_OF_FAME_DEMO_OBJ_H
