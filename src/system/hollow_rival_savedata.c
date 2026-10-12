#include "types.h"
#include "save/hollow_rival_savedata.h"
#include "save/misc_savedata.h"
#include "save/save_control.h"

// A part of save block 0x42, which the script commands of scrcmd_hollow_rival.c read and write: a value and a flag
// of each of eight indices, a counter, three flags and a value. The ROM has no string for the file, so the name is a
// guess after swan's getHollow_RivalBlk.

#define SAVE_BLOCK_MISC 0x42

void *getHollow_RivalBlk(SaveControl *save) {
    return func_0200f7d8(SaveControl_GetBlockPtr(save, SAVE_BLOCK_MISC));
}

void func_0200ff50(void *block, u32 index, u16 value) {
    HollowRivalSave *hollowRival = block;

    hollowRival->values[index] = value;
}

u16 func_0200ff54(void *block, u16 index) {
    HollowRivalSave *hollowRival = block;

    return hollowRival->values[index];
}

void func_0200ff58(void *block, u16 index, u32 value) {
    HollowRivalSave *hollowRival = block;

    hollowRival->flags[index] = value;
}

u8 func_0200ff60(HollowRivalSave *hollowRival, u16 index) {
    return hollowRival->flags[index];
}

void func_0200ff68(HollowRivalSave *hollowRival, u8 value) {
    hollowRival->unk10 = value;
}

void func_0200ff6c(void *block) {
    HollowRivalSave *hollowRival = block;

    hollowRival->unk10 = 0;
}

u16 func_0200ff74(void *block) {
    HollowRivalSave *hollowRival = block;

    return hollowRival->unk10;
}

void func_0200ff78(void *rival, u32 value) {
    HollowRivalSave *hollowRival = rival;

    hollowRival->unk11_0 = value;
}

BOOL func_0200ff8c(HollowRivalSave *hollowRival) {
    return hollowRival->unk11_0;
}

void func_0200ff94(void *block, u32 value) {
    HollowRivalSave *hollowRival = block;

    hollowRival->unk11_1 = value;
}

BOOL func_0200ffa8(HollowRivalSave *hollowRival) {
    return hollowRival->unk11_1;
}

void func_0200ffb0(void *block, u16 value) {
    HollowRivalSave *hollowRival = block;

    hollowRival->unk12 = value;
}

u16 func_0200ffb4(HollowRivalSave *hollowRival) {
    return hollowRival->unk12;
}

void func_0200ffb8(void *block, u32 value) {
    HollowRivalSave *hollowRival = block;

    hollowRival->unk11_2 = value;
}

BOOL func_0200ffcc(HollowRivalSave *hollowRival) {
    return hollowRival->unk11_2;
}

BOOL func_0200ffd4(void *block, int index) {
    HollowRivalSave *hollowRival = block;

    // BUG: the mask is whether the bit's number isn't 0, not the bit, so this is bit 0 of the byte for every index
    // but the first of each byte, which gives FALSE
#ifdef BUGFIX
    return (hollowRival->unk14[index / 8] >> (index % 8)) & 1;
#else
    return hollowRival->unk14[index / 8] & (index % 8 != 0);
#endif
}
