#ifndef POKEBW2_SAVE_WBT_SAVE_H
#define POKEBW2_SAVE_WBT_SAVE_H

#include "types.h"
#include "struct_decls.h"

// The Pokémon World Tournament's save data, from func_020179f8. It counts to 9999 for each of 29 tournaments

void *func_0200fea0(SaveControl *save);
u16 func_0200feac(void *save, u32 tournament);
void func_0200feb4(void *save, u32 tournament);
// Sets the count of a tournament's slot, and a bit of the table of Trainers beaten
void func_0200ff28(void *save, u8 index, u16 value);
void func_0200fefc(void *save, u32 trainer);
u16 func_0200ff34(void *save, u8 index);
// Whether the checksum of a downloaded tournament's extra save block holds
BOOL func_02010428(void *data);

#endif // POKEBW2_SAVE_WBT_SAVE_H
