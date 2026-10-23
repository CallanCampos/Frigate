#include "types.h"
#include "gfl/std.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"

// The musical's save block: the photo of the last finale, the props owned and the ones not yet looked at, and the
// musical's counters. The ROM has no string for the file, so the name is a guess (swan's
// getAddressOfMusicalDataInfo). Block 0x2a.

u32 MusicalSave_GetSize(void) {
    return sizeof(MusicalSave);
}

void MusicalSave_Init(MusicalSave *musical) {
    u8 i;

    MI_CpuFill8(musical, 0, sizeof(MusicalSave));
    MusicalSave_ClearEquips(musical);
    for (i = 0; i < 10; i++) {
        musical->audienceGifts[i].topKind = 4;
    }
    musical->program = 0;
    musical->hasDistProgram = 0;
    MusicalSave_AddProp(musical, 0x30);
    MusicalSave_AddProp(musical, 0x46);
    MusicalSave_AddProp(musical, 0x53);
    MusicalSave_AddProp(musical, 0x4b);
    MusicalSave_AddProp(musical, 0x0);
    MusicalSave_AddProp(musical, 0x1);
    MusicalSave_AddProp(musical, 0xa);
    MusicalSave_AddProp(musical, 0x1c);
    MusicalSave_AddProp(musical, 0x8);
    MusicalSave_AddProp(musical, 0x3b);
    MusicalSave_AddProp(musical, 0x3f);
    MusicalSave_AddProp(musical, 0x42);
    MusicalSave_AddProp(musical, 0x10);
    MusicalSave_AddProp(musical, 0x59);
    MusicalSave_AddProp(musical, 0x4c);
}

MusicalSave *getAddressOfMusicalDataInfo(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_MUSICAL);
}

void MusicalSave_ClearEquips(MusicalSave *musical) {
    u8 i;

    for (i = 0; i < 8; i++) {
        musical->equips[i].pos = 10;
    }
}

MusicalSaveEquip *MusicalSave_GetEquips(MusicalSave *musical) {
    return musical->equips;
}

BOOL MusicalSave_HasShot(MusicalSave *musical) {
    if (musical->shot.month) {
        return TRUE;
    }
    return FALSE;
}

MusicalShot *MusicalSave_GetShot(MusicalSave *musical) {
    return &musical->shot;
}

BOOL MusicalSave_HasProp(MusicalSave *musical, u8 prop) {
    if (prop < MUSICAL_PROP_MAX) {
        u8 byte = prop / 8;
        u8 bit = prop % 8;
        if ((musical->props[byte] & (1 << bit)) > 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL MusicalSave_IsPropNew(MusicalSave *musical, u8 prop) {
    if (prop < MUSICAL_PROP_MAX) {
        u8 byte = prop / 8;
        u8 bit = prop % 8;
        if ((musical->newProps[byte] & (1 << bit)) > 0) {
            return TRUE;
        }
    }
    return FALSE;
}

void MusicalSave_AddProp(MusicalSave *musical, u8 prop) {
    if (prop < MUSICAL_PROP_MAX) {
        u8 shift = prop % 8;
        u32 mask = 1 << shift;
        u8 byte = prop / 8;
        musical->props[byte] |= mask;
        musical->newProps[byte] |= mask;
    }
}

void MusicalSave_ClearNewProps(MusicalSave *musical) {
    u8 i;

    for (i = 0; i < MUSICAL_PROP_BYTES; i++) {
        musical->newProps[i] = 0;
    }
}

u32 MusicalSave_CountProps(MusicalSave *musical) {
    u32 count = 0;
    s32 i;

    for (i = 0; i < 256; i++) {
        if (MusicalSave_HasProp(musical, i) == TRUE) {
            count++;
        }
    }
    return count;
}

BOOL MusicalSave_HasAllProps(MusicalSave *musical) {
    if (MusicalSave_CountProps(musical) == MUSICAL_PROP_MAX) {
        return TRUE;
    }
    return FALSE;
}

MusicalAudienceGift *MusicalSave_GetAudienceGift(MusicalSave *musical, u8 index) {
    return &musical->audienceGifts[index];
}

u16 MusicalSave_GetShowCount(MusicalSave *musical) {
    return musical->showCount;
}

void MusicalSave_IncShowCount(MusicalSave *musical) {
    if (musical->showCount < 0xffff) {
        musical->showCount++;
    }
}

u16 MusicalSave_GetWinCount(MusicalSave *musical) {
    return musical->winCount;
}

void MusicalSave_IncWinCount(MusicalSave *musical) {
    if (musical->winCount < 0xffff) {
        musical->winCount++;
    }
}

u8 MusicalSave_GetPropKindCount(MusicalSave *musical, u8 index) {
    return musical->propKindCounts[index];
}

void MusicalSave_SetPropKindCount(MusicalSave *musical, u8 index, u8 value) {
    musical->propKindCounts[index] = value;
}

u8 MusicalSave_GetLastPoints(MusicalSave *musical) {
    return musical->lastPoints;
}

void MusicalSave_SetLastPoints(MusicalSave *musical, u8 value) {
    musical->lastPoints = value;
}

u8 MusicalSave_GetProgram(MusicalSave *musical) {
    return musical->program;
}

void MusicalSave_SetProgram(MusicalSave *musical, u8 value) {
    musical->program = value;
}

u8 MusicalSave_HasDistProgram(MusicalSave *musical) {
    return musical->hasDistProgram;
}

void MusicalSave_SetHasDistProgram(MusicalSave *musical, u8 value) {
    musical->hasDistProgram = value;
}

const u16 *MusicalSave_GetName(MusicalSave *musical) {
    return musical->name;
}

void MusicalSave_AddTotalPoints(MusicalSave *musical, u16 value) {
    s32 sum = musical->totalPoints + value;
    if (sum < 0xffff) {
        musical->totalPoints = sum;
    } else {
        musical->totalPoints = 0xffff;
    }
}

u16 MusicalSave_GetTotalPoints(MusicalSave *musical) {
    return musical->totalPoints;
}

BOOL MusicalSave_GetDressingRoomVisited(MusicalSave *musical) {
    return musical->dressingRoomVisited;
}

void MusicalSave_SetDressingRoomVisited(MusicalSave *musical, BOOL value) {
    musical->dressingRoomVisited = value;
}

u8 MusicalSave_GetDownloadedProps(MusicalSave *musical) {
    return musical->downloadedProps;
}

void MusicalSave_SetDownloadedProps(MusicalSave *musical, u8 value) {
    musical->downloadedProps = value;
}

u8 *MusicalSave_GetProps(MusicalSave *musical) {
    return musical->props;
}
