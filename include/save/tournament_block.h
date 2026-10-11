#ifndef POKEBW2_SAVE_TOURNAMENT_BLOCK_H
#define POKEBW2_SAVE_TOURNAMENT_BLOCK_H

#include "types.h"
#include "struct_decls.h"

// The save block that swan calls the tournament block, of entries of 0x160 bytes whose fields a number picks

void *getTournamentBlockAddress(SaveControl *save);
// An entry of the block
void *func_0200b8f4(void *block, u32 index);
// An entry's field, and setting it. The Wi-Fi login reads field 2 and field 10 and sets field 10
u32 func_0200b798(void *entry, u32 field);
void func_0200b830(void *entry, u32 field, u32 value);

#endif // POKEBW2_SAVE_TOURNAMENT_BLOCK_H
