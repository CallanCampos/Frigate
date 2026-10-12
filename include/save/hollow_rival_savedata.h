#ifndef POKEBW2_SAVE_HOLLOW_RIVAL_SAVEDATA_H
#define POKEBW2_SAVE_HOLLOW_RIVAL_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// The data 0xb4 bytes into save block 0x42 (hollow_rival_savedata.c, a guessed name after swan's
// getHollow_RivalBlk). The other functions of the file are in save/save_control.h, which takes the data as a void *.

#define HOLLOW_RIVAL_INDEX_COUNT 8

typedef struct {
    u8 values[HOLLOW_RIVAL_INDEX_COUNT];
    u8 flags[HOLLOW_RIVAL_INDEX_COUNT];
    u8 unk10;
    u8 unk11_0 : 1;
    u8 unk11_1 : 1;
    u8 unk11_2 : 1;
    u16 unk12;
    u8 unk14[4];
} HollowRivalSave;

u8 func_0200ff60(HollowRivalSave *hollowRival, u16 index);
void func_0200ff68(HollowRivalSave *hollowRival, u8 value);
BOOL func_0200ff8c(HollowRivalSave *hollowRival);
BOOL func_0200ffa8(HollowRivalSave *hollowRival);
u16 func_0200ffb4(HollowRivalSave *hollowRival);
BOOL func_0200ffcc(HollowRivalSave *hollowRival);

#endif // POKEBW2_SAVE_HOLLOW_RIVAL_SAVEDATA_H
