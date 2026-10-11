#ifndef POKEBW2_SAVE_RESH_ZEK_H
#define POKEBW2_SAVE_RESH_ZEK_H

#include "types.h"
#include "pml/poke_party.h"
#include "struct_decls.h"

// Save block 0x2b, which swan calls ReshZek: the Reshiram or Zekrom fused with Kyurem, kept while they are one
struct ReshZekSave {
    u32 unk0;
    PartyPkm pkm;
}; // size: 0xe0

// The size of the block and its init
u32 func_0200af74(void);
void func_0200af78(ReshZekSave *reshZek);
ReshZekSave *getReshZekBlkAddress(SaveControl *save);
// Copies the word at the start in and out
void func_0200af8c(ReshZekSave *reshZek, const u32 *src);
void func_0200af9c(ReshZekSave *reshZek, u32 *dest);
// The fused Pokémon, and storing one
PartyPkm *func_0200afa8(ReshZekSave *reshZek);
void func_0200afac(ReshZekSave *reshZek, const PartyPkm *pkm);

// The init tail-calls a function at 0x0215cd8c with the block, which dsd can't tell between overlays 11 and 12;
// neither overlay's function there inits a Pokémon, so the symbol is the linker's choice, overlay 11's
void func_ov011_0215cd8c(void *block);

#endif // POKEBW2_SAVE_RESH_ZEK_H
