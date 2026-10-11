#ifndef POKEBW2_APP_POKEMONTRADE_DEMO_COMMON_H
#define POKEBW2_APP_POKEMONTRADE_DEMO_COMMON_H

#include "types.h"
#include "app/pokemon_trade_local.h"

// pokemontrade_demo_common.c, the part of the trade demo that its three overlay-193 files share. Overlay 193 holds
// one demo's three scenes, each file a copy of the same template, so this file's names are descriptive guesses

// Does nothing and returns its argument. The frame that a scene's step happens at goes through it
int PokemonTradeDemo_Frames(int frame);
// Creates the sprite that the scenes' effects use, or restarts it
void PokemonTradeDemo_ShowActor(PokemonTradeWork *wk);
// Loads the sprite's resources
void PokemonTradeDemo_LoadResources(PokemonTradeWork *wk);
// Sets the frame counter of the scene
void PokemonTradeDemo_SetTimer(PokemonTradeWork *wk, int timer);
// Frees the effects and goes on to the save
void PokemonTradeDemo_Finish(PokemonTradeWork *wk);
// The emitters' callbacks, which make them follow the paths of the 3D effects
void PokemonTradeDemo_InitEmitter0(SPLEmitter *emitter);
void PokemonTradeDemo_InitEmitter1(SPLEmitter *emitter);

#endif // POKEBW2_APP_POKEMONTRADE_DEMO_COMMON_H
