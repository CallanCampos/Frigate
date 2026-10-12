#include "types.h"
#include "save/encounter.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The wild encounters' save: the roaming Pokémon, the swarm's location, the repel and the special Pokémon that were
// caught. The ROM has no string for the file, so the name is a guess. Block 0x37.

#define SAVE_BLOCK_ENCOUNT 0x37

u32 EncountSave_GetBlockSize(void) {
    return sizeof(EncountSave);
}

EncountSave *SaveControl_GetEncountSave(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_ENCOUNT);
}

void EncountSave_Init(EncountSave *save) {
    sys_memset(save, 0, sizeof(EncountSave));
    save->swarmLocation = 0;
    save->repelSteps = 0;
    save->repelItem = 0;
}

void EncountSave_RerollSwarmLocation(SaveControl *save) {
    EncountSave *encount = SaveControl_GetEncountSave(save);

    encount->swarmLocation = GFL_RandomLCAlt(ENCOUNT_SAVE_SWARM_LOCATION_COUNT);
}

u8 EncountSave_GetSwarmLocation(EncountSave *save) {
    return save->swarmLocation;
}

u8 func_0200dd38(EncountSave *save, u8 index) {
    return save->unk2A[index];
}

u8 EncountSave_GetRoamingPkmZoneClock(EncountSave *save, u8 slot) {
    return save->roamingZoneClocks[slot];
}

BOOL EncountSave_GetRoamingPkmStatus(EncountSave *save, u8 slot) {
    return save->roamingPkm[slot].active;
}

RoamingPkm *EncountSave_GetRoamingPkm(EncountSave *save, u8 slot) {
    return &save->roamingPkm[slot];
}

u32 EncountSave_GetRoamingPkmParam(const RoamingPkm *roamingPkm, u32 param) {
    switch (param) {
        case ROAMING_PKM_PARAM_ZONE:
            return roamingPkm->zone;
        case ROAMING_PKM_PARAM_IVS:
            return roamingPkm->ivs;
        case ROAMING_PKM_PARAM_PERSONALITY:
            return roamingPkm->personality;
        case ROAMING_PKM_PARAM_NATURE:
            return roamingPkm->nature;
        case ROAMING_PKM_PARAM_SPECIES:
            return roamingPkm->species;
        case ROAMING_PKM_PARAM_HP:
            return roamingPkm->hp;
        case ROAMING_PKM_PARAM_LEVEL:
            return roamingPkm->level;
        case ROAMING_PKM_PARAM_STATUS:
            return roamingPkm->status;
        case ROAMING_PKM_PARAM_ACTIVE:
            return roamingPkm->active;
        case ROAMING_PKM_PARAM_UNK13:
            return roamingPkm->unk13;
    }
    return 0;
}

void EncountSave_SetRepelSteps(EncountSave *save, u8 steps) {
    save->repelSteps = steps;
}

BOOL EncountSave_DecrementRepelSteps(EncountSave *save) {
    if (save->repelSteps != 0) {
        save->repelSteps--;
        if (save->repelSteps == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL EncountSave_IsRepelDepleted(EncountSave *save) {
    if (save->repelSteps == 0) {
        return TRUE;
    }
    return FALSE;
}

void EncountSave_SetUsedRepelItemID(EncountSave *save, u16 item) {
    save->repelItem = item;
}

u16 EncountSave_GetUsedRepelItemID(EncountSave *save) {
    return save->repelItem;
}

// BUG: adds up the bits of the special Pokémon that were caught instead of counting them, so the count is far too
// high once any but the first was caught
int GetAlreadyCaughtNPokeCount(EncountSave *save) {
    int i;
    int count = 0;
    u16 flags = save->caughtFlags;

    for (i = 0; i < ENCOUNT_SAVE_N_POKE_COUNT; i++) {
#ifdef BUGFIX
        if (flags & (1 << i)) {
            count++;
        }
#else
        count += flags & (1 << i);
#endif
    }
    return count;
}

BOOL IsNPokeAlreadyCaught(EncountSave *save, u8 index) {
    return save->caughtFlags & (1 << index);
}

void SetNPokeCaught(EncountSave *save, u8 index) {
    save->caughtFlags |= 1 << index;
}
