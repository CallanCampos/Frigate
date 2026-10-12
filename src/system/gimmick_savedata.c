#include "types.h"
#include "save/gimmick_savedata.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The state of the zone's gimmick and its save block. The ROM has no string for the file, so the name is a guess
// (swan's GimmickSave_Init). Block 0x30.

#define SAVE_BLOCK_GIMMICK 0x30

u32 GimmickSave_GetBlockSize(void) {
    return sizeof(GimmickState);
}

void GimmickSave_Init(GimmickState *gimmick) {
    sys_memset(gimmick, 0, sizeof(GimmickState));
    gimmick->id = -1;
}

void GimmickState_Reset(GimmickState *gimmick) {
    s32 i;

    gimmick->id = -1;
    for (i = 0; i < GIMMICK_WORK_MAX; i++) {
        gimmick->work[i] = 0;
    }
}

void *GimmickState_SetID(GimmickState *gimmick, u32 gimmickId) {
    GimmickState_Reset(gimmick);
    gimmick->id = gimmickId;
    return gimmick->work;
}

void *GimmickState_GetUserData(GimmickState *gimmick, u32 gimmickId) {
    return gimmick->work;
}

void *func_0200bac8(GimmickState *gimmick, u32 gimmickId) {
    return gimmick->unk84;
}

void *func_0200bacc(GimmickState *gimmick) {
    return gimmick->unk89;
}

u32 GimmickState_GetID(GimmickState *gimmick) {
    return gimmick->id;
}

void GimmickState_StoreSave(const GimmickState *gimmick, SaveControl *save) {
    sys_memcpy32(gimmick, SaveControl_GetBlockPtr(save, SAVE_BLOCK_GIMMICK), sizeof(GimmickState));
}

void GimmickState_LoadSave(SaveControl *save, GimmickState *gimmick) {
    sys_memcpy32(SaveControl_GetBlockPtr(save, SAVE_BLOCK_GIMMICK), gimmick, sizeof(GimmickState));
}
