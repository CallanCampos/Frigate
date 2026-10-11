#ifndef POKEBW2_APP_POKEMONTRADE_IRCDEMO_H
#define POKEBW2_APP_POKEMONTRADE_IRCDEMO_H

#include "types.h"
#include "app/pokemon_trade_local.h"

// Overlay 192, the trade demo of a trade over the infrared port. Its two files, pokemontrade_demo_common.c (the
// ROM's name for the template's first half, which overlay 193's demos share) and pokemontrade_ircdemo.c, are named
// from the ROM's strings; the function names are descriptive guesses

// pokemontrade_demo_common.c

// Creates the sprite that the scene's effects use, or restarts it
void PokemonTradeIrcDemo_ShowActor(PokemonTradeWork *wk);
// Loads the sprite's resources
void PokemonTradeIrcDemo_LoadResources(PokemonTradeWork *wk);
// Does nothing and returns its argument. The frame that a step happens at goes through it
int PokemonTradeIrcDemo_Frames(int frame);
// Sets the frame counter of the scene
void PokemonTradeIrcDemo_SetTimer(PokemonTradeWork *wk, int timer);
// Frees the effects and goes on to the save
void PokemonTradeIrcDemo_Finish(PokemonTradeWork *wk);
// Shows a sprite still, in front of the scene
void PokemonTradeIrcDemo_ShowSprite(MCSS *mcss, PokemonTradeWork *wk);
// Plays the voice of the other machine's Pokémon and makes a sprite hop or bounce where it is
void PokemonTradeIrcDemo_StartMove(PokemonTradeWork *wk, int index);

// pokemontrade_ircdemo.c

// The step that sets the scene up, after the common file's steps
void PokemonTradeIrcDemo_Init(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMONTRADE_IRCDEMO_H
