#ifndef POKEBW2_SAVE_POKEDEX_SKIN_SAVE_H
#define POKEBW2_SAVE_POKEDEX_SKIN_SAVE_H

#include "types.h"

// The downloaded Pokédex skin, extra save block 7 (pokedex_skin_save.c, a descriptive name). Its size and its
// accessors, which take the block as a void pointer, are declared in save/save_control.h

typedef struct {
    u8 chars[0x6000];
    u8 palette[0x200];
    BOOL valid;
} PokedexSkinSave; // size: 0x6204

void func_0200f16c(void *block);

#endif // POKEBW2_SAVE_POKEDEX_SKIN_SAVE_H
