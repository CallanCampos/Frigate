#include "types.h"
#include "gfl/std.h"
#include "save/pokedex_skin_save.h"
#include "save/save_control.h"

// The downloaded Pokédex skin, extra save block 7: the characters and the palette of the Pokédex's cover. The ROM has
// no string for the file, so the name is a guess

u32 PokedexSkinSave_GetSize(void) {
    return sizeof(PokedexSkinSave);
}

void PokedexSkinSave_Init(void *block) {
    MI_CpuFill8(block, 0, PokedexSkinSave_GetSize());
}

void *PokedexSkinSave_GetChars(PokedexSkinSave *skin) {
    return skin->chars;
}

void *PokedexSkinSave_GetPalette(PokedexSkinSave *skin) {
    return skin->palette;
}

BOOL PokedexSkinSave_IsValid(PokedexSkinSave *skin) {
    return skin->valid;
}

void PokedexSkinSave_SetValid(PokedexSkinSave *skin, u32 valid) {
    skin->valid = valid;
}
