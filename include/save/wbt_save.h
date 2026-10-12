#ifndef POKEBW2_SAVE_WBT_SAVE_H
#define POKEBW2_SAVE_WBT_SAVE_H

#include "types.h"
#include "struct_decls.h"

// The Pokémon World Tournament's save data, from func_020179f8. It counts to 9999 for each of 29 tournaments
// (wbt_savedata.c, a descriptive name). It is 0x5c bytes into the Trial House's save block, 0x3f

#define WBT_TOURNAMENT_COUNT 29
#define WBT_COUNT_MAX 9999
#define WBT_SLOT_COUNT 3
#define WBT_SLOT_NONE 0xff

typedef struct {
    u16 counts[WBT_TOURNAMENT_COUNT];
    // A bit for each Trainer beaten
    u8 beaten[8];
    u16 slots[WBT_SLOT_COUNT];
    u8 padding48[4];
} WbtSave;

// func_0200fe80, which clears it, is declared in save/trial_house.h, whose block it is part of
void *func_0200fea0(SaveControl *save);
u16 func_0200feac(void *save, u32 tournament);
void func_0200feb4(void *save, u32 tournament);
// Sets the count of a tournament's slot, and a bit of the table of Trainers beaten
void func_0200ff28(void *save, u8 index, u16 value);
BOOL func_0200fec8(WbtSave *wbt, int trainer);
void func_0200fefc(void *save, int trainer);
u16 func_0200ff34(void *save, u8 index);
// Whether the checksum of a downloaded tournament's extra save block holds
BOOL func_02010428(void *data);

#endif // POKEBW2_SAVE_WBT_SAVE_H
