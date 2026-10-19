#include "types.h"
#include "gfl/std.h"
#include "save/pokewood.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The save block of Pokéstar Studios, which the game calls Pokewood: the flag lists with a flag for each movie, each
// movie's best result and the last series filmed. The ROM has no string for the file, so the name is a guess; the
// movie that is loaded is pokewood_rec.c's. Block 0x47.

#define SAVE_BLOCK_POKEWOOD 0x47
#define POKEWOOD_SCORE_NONE (-9999)
#define POKEWOOD_UNK348_MAX 99

static void getFlagPos(int movie, int *word, int *bit);
static u32 *getFlagList(PokewoodSave *save, u32 list);
static void func_0201110c(PokewoodSave *save, u32 movie);

u32 PokewoodSave_GetSize(void) {
    return sizeof(PokewoodSave);
}

void PokewoodSave_Init(PokewoodSave *save) {
    int i;

    MI_CpuFill8(save, 0, sizeof(PokewoodSave));
    for (i = 0; i < POKEWOOD_MOVIE_COUNT; i++) {
        save->records[i].score = POKEWOOD_SCORE_NONE;
    }
    func_0201110c(save, 0);
    PokewoodSave_SetFlag(save, 3, 0);
    PokewoodSave_SetLevelBit(save, 0, 1);
}

PokewoodSave *PokewoodSave_Get(GameData *gameData) {
    return SaveControl_GetBlockPtr(GameData_GetSaveControl(gameData), SAVE_BLOCK_POKEWOOD);
}

static void getFlagPos(int movie, int *word, int *bit) {
    *word = movie / 32;
    *bit = movie - *word * 32;
}

static u32 *getFlagList(PokewoodSave *save, u32 list) {
    switch (list) {
    case 0:
        return save->flags0;
    case 1:
        return save->flags1;
    case 2:
        return save->flags2;
    case 3:
        return save->flags3;
    case 4:
        return save->flags4;
    case 5:
        return save->flags5;
    case 6:
        return save->flags6;
    }
    return NULL;
}

BOOL PokewoodSave_CheckFlag(PokewoodSave *save, u32 list, u32 movie) {
    u32 *flags = getFlagList(save, list);
    int word;
    int bit;

    getFlagPos(movie, &word, &bit);
    return (flags[word] >> bit) & 1;
}

void PokewoodSave_SetFlag(PokewoodSave *save, u32 list, u32 movie) {
    u32 *flags = getFlagList(save, list);
    int word;
    int bit;

    getFlagPos(movie, &word, &bit);
    flags[word] |= 1 << bit;
}

BOOL func_02011100(PokewoodSave *save, u32 movie) {
    return PokewoodSave_CheckFlag(save, 0, movie);
}

static void func_0201110c(PokewoodSave *save, u32 movie) {
    PokewoodSave_SetFlag(save, 0, movie);
}

BOOL func_02011118(PokewoodSave *save, u32 movie) {
    return PokewoodSave_CheckFlag(save, 1, movie);
}

void func_02011124(PokewoodSave *save, u32 movie) {
    PokewoodSave_SetFlag(save, 1, movie);
}

void PokewoodSave_UpdateRecord(PokewoodSave *save, u32 movie, const PokewoodRecord *record) {
    if (record->score >= save->records[movie].score) {
        if (!save->records[movie].hasRecord) {
            save->records[movie] = *record;
        } else {
            save->records[movie] = *record;
            save->records[movie].isNewRecord = TRUE;
            save->records[movie].hasRecord = TRUE;
        }
    }
}

void PokewoodSave_MarkRecordSeen(PokewoodSave *save, u32 movie) {
    save->records[movie].hasRecord = TRUE;
    save->records[movie].isNewRecord = FALSE;
}

PokewoodRecord *PokewoodSave_GetRecord(PokewoodSave *save, u32 movie) {
    return &save->records[movie];
}

u8 func_020111a0(PokewoodSave *save, u32 slot) {
    return save->slots[slot];
}

void func_020111a8(PokewoodSave *save, u32 slot, u8 value) {
    save->slots[slot] = value;
}

s16 func_020111b0(PokewoodSave *save) {
    if (save->unk348 > POKEWOOD_UNK348_MAX || save->unk348 < 0) {
        save->unk348 = 0;
    }
    return save->unk348;
}

void func_020111d0(PokewoodSave *save, int amount) {
    save->unk348 += (s16)amount;
    if (save->unk348 > POKEWOOD_UNK348_MAX) {
        save->unk348 = POKEWOOD_UNK348_MAX;
    }
}

int func_020111ec(int value) {
    if (value >= 0 && value < 20) {
        return 0;
    }
    if (value >= 20 && value < 40) {
        return 1;
    }
    if (value >= 40 && value < 70) {
        return 2;
    }
    if (value >= 70 && value < 95) {
        return 3;
    }
    if (value >= 95 && value <= POKEWOOD_UNK348_MAX) {
        return 4;
    }
    return 0;
}

BOOL PokewoodSave_GetLevelBit(PokewoodSave *save, u32 bit) {
    return (save->levelBits >> bit) & 1;
}

void PokewoodSave_SetLevelBit(PokewoodSave *save, u32 bit, u32 value) {
    save->levelBits &= 0xffff ^ (1 << bit);
    save->levelBits |= value << bit;
}

u8 PokewoodSave_GetLastSeries(PokewoodSave *save, u32 index) {
    return save->lastSeries[index];
}

void PokewoodSave_PushLastSeries(PokewoodSave *save, u32 a1, u32 a2, u8 *a3) {
    int i;
    int j;
    int count;

    save->lastSeries[3] = save->lastSeries[2];
    save->lastSeries[2] = save->lastSeries[1];
    save->lastSeries[1] = save->lastSeries[0];
    save->lastSeries[0] = a2;
    a3[a1] = 0;
    for (i = 1; i < POKEWOOD_LAST_SERIES_COUNT; i++) {
        count = 0;
        for (j = 0; j < POKEWOOD_SLOT_COUNT; j++) {
            if (save->lastSeries[i] == a3[j]) {
                count++;
            }
        }
        if (count == 0) {
            save->lastSeries[i] = 0;
        }
    }
}

u32 PokewoodSave_CountFlags(PokewoodSave *save, u32 list) {
    int i;
    u32 count = 0;

    for (i = 0; i < POKEWOOD_MOVIE_COUNT; i++) {
        if (PokewoodSave_CheckFlag(save, list, i)) {
            count++;
        }
    }
    return count;
}
