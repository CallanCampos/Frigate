#include "types.h"
#include "save/pokedex_skin_save.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The downloaded Pokédex skin, extra save block 7: the characters and the palette of the Pokédex's cover. The ROM has
// no string for the file, so the name is a guess

u32 func_0200f164(void) {
    return sizeof(PokedexSkinSave);
}

void func_0200f16c(void *block) {
    sys_memset(block, 0, func_0200f164());
}

void *func_0200f180(void *block) {
    PokedexSkinSave *skin = block;

    return skin->chars;
}

void *func_0200f184(void *block) {
    PokedexSkinSave *skin = block;

    return skin->palette;
}

BOOL func_0200f18c(void *block) {
    PokedexSkinSave *skin = block;

    return skin->valid;
}

void func_0200f194(void *block, u32 valid) {
    PokedexSkinSave *skin = block;

    skin->valid = valid;
}
