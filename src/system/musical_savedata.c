#include "types.h"
#include "gfl/std.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"

// The musical's save block: the photo of the last finale, the props owned and the ones not yet looked at, and the
// musical's counters. The ROM has no string for the file, so the name is a guess (swan's
// getAddressOfMusicalDataInfo). Block 0x2a.

#define SAVE_BLOCK_MUSICAL 0x2a

u32 func_0200ac58(void) {
    return sizeof(MusicalSave);
}

void func_0200ac60(MusicalSave *musical) {
    u8 i;

    MI_CpuFill8(musical, 0, sizeof(MusicalSave));
    func_0200ad20(musical);
    for (i = 0; i < 10; i++) {
        musical->unk1E0[i].unk0 = 4;
    }
    musical->unk29D = 0;
    musical->unk29E = 0;
    func_0200add8(musical, 0x30);
    func_0200add8(musical, 0x46);
    func_0200add8(musical, 0x53);
    func_0200add8(musical, 0x4b);
    func_0200add8(musical, 0x0);
    func_0200add8(musical, 0x1);
    func_0200add8(musical, 0xa);
    func_0200add8(musical, 0x1c);
    func_0200add8(musical, 0x8);
    func_0200add8(musical, 0x3b);
    func_0200add8(musical, 0x3f);
    func_0200add8(musical, 0x42);
    func_0200add8(musical, 0x10);
    func_0200add8(musical, 0x59);
    func_0200add8(musical, 0x4c);
}

MusicalSave *getAddressOfMusicalDataInfo(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_MUSICAL);
}

void func_0200ad20(MusicalSave *musical) {
    u8 i;

    for (i = 0; i < 8; i++) {
        musical->unk1B0[i].unk0 = 10;
    }
}

MusicalSaveUnk1B0 *func_0200ad44(MusicalSave *musical) {
    return musical->unk1B0;
}

BOOL func_0200ad4c(MusicalSave *musical) {
    if (musical->shot.month) {
        return TRUE;
    }
    return FALSE;
}

MusicalShot *func_0200ad5c(MusicalSave *musical) {
    return &musical->shot;
}

BOOL func_0200ad60(MusicalSave *musical, u8 prop) {
    if (prop < MUSICAL_PROP_MAX) {
        u8 byte = prop / 8;
        u8 bit = prop % 8;
        if ((musical->props[byte] & (1 << bit)) > 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL func_0200ad9c(MusicalSave *musical, u8 prop) {
    if (prop < MUSICAL_PROP_MAX) {
        u8 byte = prop / 8;
        u8 bit = prop % 8;
        if ((musical->newProps[byte] & (1 << bit)) > 0) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_0200add8(MusicalSave *musical, u8 prop) {
    if (prop < MUSICAL_PROP_MAX) {
        u8 shift = prop % 8;
        u32 mask = 1 << shift;
        u8 byte = prop / 8;
        musical->props[byte] |= mask;
        musical->newProps[byte] |= mask;
    }
}

void func_0200ae14(MusicalSave *musical) {
    u8 i;

    for (i = 0; i < MUSICAL_PROP_BYTES; i++) {
        musical->newProps[i] = 0;
    }
}

u32 func_0200ae30(MusicalSave *musical) {
    u32 count = 0;
    s32 i;

    for (i = 0; i < 256; i++) {
        if (func_0200ad60(musical, i) == TRUE) {
            count++;
        }
    }
    return count;
}

BOOL func_0200ae58(MusicalSave *musical) {
    if (func_0200ae30(musical) == MUSICAL_PROP_MAX) {
        return TRUE;
    }
    return FALSE;
}

MusicalSaveUnk1E0 *func_0200ae6c(MusicalSave *musical, u8 index) {
    return &musical->unk1E0[index];
}

u16 func_0200ae78(MusicalSave *musical) {
    return musical->unk252;
}

void func_0200ae84(MusicalSave *musical) {
    if (musical->unk252 < 0xffff) {
        musical->unk252++;
    }
}

u16 func_0200ae9c(MusicalSave *musical) {
    return musical->unk254;
}

void func_0200aea4(MusicalSave *musical) {
    if (musical->unk254 < 0xffff) {
        musical->unk254++;
    }
}

u8 func_0200aebc(MusicalSave *musical, u8 index) {
    return musical->unk298[index];
}

void func_0200aec8(MusicalSave *musical, u8 index, u8 value) {
    musical->unk298[index] = value;
}

u8 func_0200aed4(MusicalSave *musical) {
    return musical->unk29C;
}

void func_0200aedc(MusicalSave *musical, u8 value) {
    musical->unk29C = value;
}

u8 func_0200aee4(MusicalSave *musical) {
    return musical->unk29D;
}

void func_0200aef0(MusicalSave *musical, u8 value) {
    musical->unk29D = value;
}

u8 func_0200aefc(MusicalSave *musical) {
    return musical->unk29E;
}

void func_0200af08(MusicalSave *musical, u8 value) {
    musical->unk29E = value;
}

const u16 *func_0200af14(MusicalSave *musical) {
    return musical->name;
}

void func_0200af1c(MusicalSave *musical, u16 value) {
    s32 sum = musical->unk256 + value;
    if (sum < 0xffff) {
        musical->unk256 = sum;
    } else {
        musical->unk256 = 0xffff;
    }
}

u16 func_0200af38(MusicalSave *musical) {
    return musical->unk256;
}

BOOL func_0200af44(MusicalSave *musical) {
    return musical->dressingRoomVisited;
}

void func_0200af50(MusicalSave *musical, BOOL value) {
    musical->dressingRoomVisited = value;
}

u8 func_0200af5c(MusicalSave *musical) {
    return musical->downloadedProps;
}

void func_0200af64(MusicalSave *musical, u8 value) {
    musical->downloadedProps = value;
}

u8 *func_0200af6c(MusicalSave *musical) {
    return musical->props;
}
