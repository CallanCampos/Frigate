#include "types.h"
#include "save/key_data_savedata.h"
#include "gfl/std.h"
#include "save/key_info.h"
#include "save/save_control.h"

// The save block that swan calls the key data: the run through the Black Tower or the White Treehollow (the seed of
// its floors, the Trainers defeated, the floor and room reached) and the key information of the Key System. The ROM
// has no string for the file, so the name is descriptive. Block 0x45.

#define SAVE_BLOCK_KEY_DATA 0x45

u32 func_02010230(void) {
    return sizeof(KeyDataSave);
}

void func_02010234(KeyDataSave *keyData) {
    sys_memset(keyData, 0, sizeof(KeyDataSave));
    func_02010474(func_020103f0(keyData));
}

// Clears the run, but not its seed, its money or the areas reached
void func_0201024c(KeyDataSave *keyData) {
    sys_memset(keyData->trainerFlags, 0, sizeof(keyData->trainerFlags));
    keyData->flags = 0;
    keyData->unk1C = 0;
    keyData->floor = 0;
    keyData->room = 0;
}

KeyDataSave *getKeyDataBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_KEY_DATA);
}

u8 func_02010274(KeyDataSave *keyData, u32 a1) {
    if (a1 == 0) {
        return keyData->progress;
    }
    return keyData->progressOther;
}

u32 func_02010284(KeyDataSave *keyData) {
    return keyData->seed;
}

BOOL func_02010288(KeyDataSave *keyData, u32 a1, u32 bit) {
    return (keyData->trainerFlags[a1] & (1 << bit)) != 0;
}

// How many Trainers have been defeated
u16 func_020102a4(KeyDataSave *keyData) {
    u32 i, j;
    u32 count = 0;

    for (i = 0; i < KEY_DATA_TRAINER_FLAG_WORDS; i++) {
        for (j = 0; j < 32; j++) {
            if (func_02010288(keyData, i, j) == TRUE) {
                count++;
            }
        }
    }
    return count;
}

u8 func_020102d0(KeyDataSave *keyData) {
    return keyData->unk1C;
}

// The floor within the area reached
u8 func_020102d4(KeyDataSave *keyData) {
    return keyData->floor - func_020103a0(func_02010378(keyData));
}

u8 func_020102ec(KeyDataSave *keyData) {
    return keyData->room;
}

void func_020102f0(KeyDataSave *keyData, u8 value, u32 a2) {
    if (a2 == 0) {
        keyData->progress = value;
    } else {
        keyData->progressOther = value;
    }
}

void func_02010300(KeyDataSave *keyData, u32 value) {
    keyData->seed = value;
}

void func_02010304(KeyDataSave *keyData, u32 a1, u32 bit, BOOL set) {
    u32 mask;

    if (set == FALSE) {
        mask = 0xffffffff;
        mask ^= 1 << bit;
        keyData->trainerFlags[a1] &= mask;
    } else {
        mask = 1;
        mask <<= bit;
        keyData->trainerFlags[a1] |= mask;
    }
}

void func_02010334(KeyDataSave *keyData, u8 value) {
    keyData->unk1C = value;
}

void func_02010338(KeyDataSave *keyData, u8 value) {
    keyData->floor = value;
}

void func_0201033c(KeyDataSave *keyData, u8 value) {
    keyData->room = value;
}

BOOL func_02010340(KeyDataSave *keyData, u32 bit) {
    return ((1 << bit) & keyData->flags) != 0;
}

void func_02010354(KeyDataSave *keyData, u32 bit, BOOL set) {
    u32 mask;

    if (set == FALSE) {
        mask = 0xffffffff;
        mask ^= 1 << bit;
        keyData->flags &= mask;
    } else {
        mask = 1;
        mask <<= bit;
        keyData->flags |= mask;
    }
}

// The area that the floor reached lies in
u8 func_02010378(KeyDataSave *keyData) {
    u8 area;

    for (area = 1; area < KEY_DATA_AREA_COUNT; area++) {
        if ((int)func_020103a0(area) > keyData->floor) {
            return area - 1;
        }
    }
    return KEY_DATA_AREA_COUNT - 1;
}

// The floors of the areas before one
u8 func_020103a0(int a0) {
    int i;
    int floors = 0;

    for (i = 0; i < a0; i++) {
        floors += func_020103c4(i);
    }
    return floors;
}

// The floors of an area
u8 func_020103c4(u8 a0) {
    u8 floors[KEY_DATA_AREA_COUNT] = { 1, 1, 1, 1, 1, 2, 3, 2, 3, 4 };

    return floors[a0];
}

u32 func_020103e8(KeyDataSave *keyData) {
    return keyData->money;
}

void func_020103ec(KeyDataSave *keyData, u32 value) {
    keyData->money = value;
}

KeyInfoSave *func_020103f0(KeyDataSave *keyData) {
    return &keyData->keyInfo;
}
