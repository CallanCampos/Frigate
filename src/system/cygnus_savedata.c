#include <stddef.h>
#include "types.h"
#include "save/cygnus_save_control.h"
#include "gfl/std.h"
#include "save/dream_world.h"
#include "save/save_control.h"

// Pokémon Dream Radar's save block, and the checks and the encoding of the data that Dream Radar writes to the card,
// which cygnus_save_control.c reads and writes. The ROM has no string for the file, so the name is a guess; the
// function names are swan's except getNextSeed's callers. Block 0x48.

#define SAVE_BLOCK_DREAM_RADAR 0x48

static u64 getNextSeed(u64 seed);

u32 getDreamRadarSaveSize(void) {
    return sizeof(DreamRadarSave);
}

void initDreamRadarSave(DreamRadarSave *save) {
    sys_memset(save, 0, getDreamRadarSaveSize());
}

u32 GetDreamRadarFlag(DreamRadarSave *save, u32 flag) {
    switch (flag) {
    case DREAM_RADAR_FLAG_READ:
        return save->read;
    case DREAM_RADAR_FLAG_SEED:
        return save->seed;
    case DREAM_RADAR_FLAG_RECEIVED:
        return save->received;
    }
    return 0;
}

void SetDreamRadarFlag(DreamRadarSave *save, u32 flag, u32 value) {
    switch (flag) {
    case DREAM_RADAR_FLAG_READ:
        save->read = value;
        break;
    case DREAM_RADAR_FLAG_SEED:
        save->seed = value;
        break;
    case DREAM_RADAR_FLAG_RECEIVED:
        save->received = value;
        break;
    }
}

DreamRadarSave *GetDreamRadarSaveBlock(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_DREAM_RADAR);
}

static u64 getNextSeed(u64 seed) {
    return seed * 0x5d588b656c078965 + 0x269ec3;
}

u32 ValidateDreamRadarSave(CygnusSaveData *data) {
    if (data->magic != CYGNUS_DATA_MAGIC) {
        return CYGNUS_DATA_INVALID;
    }
    if (data->headerCrc != getCRC16(data, offsetof(CygnusSaveData, headerCrc))) {
        return CYGNUS_DATA_INVALID;
    }
    if (data->state == 1) {
        return CYGNUS_DATA_READ;
    }
    if (data->crc == getCRC16(data->words, sizeof(data->words))) {
        return CYGNUS_DATA_VALID;
    }
    return CYGNUS_DATA_BROKEN;
}

void func_020113c4(void *data, u32 seed, u32 received) {
    CygnusSaveData *cygnus = data;
    u32 mask = getNextSeed(seed) >> 32;

    if (cygnus->seed == 0) {
        cygnus->seed = CYGNUS_DATA_MAGIC;
    }
    cygnus->state = 1;
    cygnus->seed = seed;
    cygnus->magic = CYGNUS_DATA_MAGIC;
    cygnus->received = received ^ mask;
    cygnus->headerCrc = getCRC16(cygnus, offsetof(CygnusSaveData, headerCrc));
}

u32 func_02011400(void *data, u32 seed, u32 *words) {
    CygnusSaveData *cygnus = data;
    u32 i = 0;
    u32 mask = getNextSeed(seed) >> 32;
    u32 result = ValidateDreamRadarSave(cygnus);

    for (; i < CYGNUS_DATA_WORD_MAX; i++) {
        words[i] = cygnus->words[i] ^ mask;
    }
    return result;
}
