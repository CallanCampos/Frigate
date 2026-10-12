#ifndef POKEBW2_DEMO_DDEMO_OBJ_H
#define POKEBW2_DEMO_DDEMO_OBJ_H

#include "types.h"
#include "demo/ddemo_main.h"

// The Hall of Fame entry demo's actors and the texts drawn on sprites, in ddemo_obj.c (a guessed name). Our names

void DDemoObj_Init(DDemoWork *wk, u32 scene);
void DDemoObj_Exit(DDemoWork *wk);
void DDemoObj_SetPos(DDemoWork *wk, u32 id, s16 x, s16 y);
void DDemoObj_GetPos(DDemoWork *wk, u32 id, s16 *x, s16 *y);
void DDemoObj_Move(DDemoWork *wk, u32 id, s16 x, s16 y);
void DDemoObj_SetVisible(DDemoWork *wk, u32 id, BOOL visible);
void DDemoObj_SetAnime(DDemoWork *wk, u32 id, u32 sequence);
BOOL DDemoObj_IsAnimating(DDemoWork *wk, u32 id);
// Loads the Pokémon being shown, in place of the last one
void DDemoObj_AddPoke(DDemoWork *wk);
// Gives the actors' palette the colors of the Pokémon's type
void DDemoObj_LoadTypePalette(DDemoWork *wk);
void DDemoObj_PrintPokeInfo(DDemoWork *wk);
// Keeps the first scene's texts on their plates
void DDemoObj_MoveFontOam(DDemoWork *wk);
void DDemoObj_SetFontOamVisible(DDemoWork *wk, u32 id, BOOL visible);
void DDemoObj_Init1st(DDemoWork *wk);
void DDemoObj_Exit1st(DDemoWork *wk);
void DDemoObj_Main1st(DDemoWork *wk);
void DDemoObj_Init2nd(DDemoWork *wk);
void DDemoObj_Exit2nd(DDemoWork *wk);
void DDemoObj_Main2nd(DDemoWork *wk);
// Removes the confetti that have fallen, and drops a new one every other frame or so
void DDemoObj_MainConfetti(DDemoWork *wk);

#endif // POKEBW2_DEMO_DDEMO_OBJ_H
