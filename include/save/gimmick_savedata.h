#ifndef POKEBW2_SAVE_GIMMICK_SAVEDATA_H
#define POKEBW2_SAVE_GIMMICK_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// The state of the zone's gimmick, which GameData keeps and save block 0x30 stores. The header's name is a guess

#define GIMMICK_WORK_MAX 0x20

struct GimmickState {
    // -1 for none
    s32 id;
    // The gimmick's own data
    u32 work[GIMMICK_WORK_MAX];
    u8 unk84[0x5];
    u8 unk89[0xb];
}; // size: 0x94

u32 GimmickSave_GetBlockSize(void);
void GimmickSave_Init(GimmickState *gimmick);
void GimmickState_Reset(GimmickState *gimmick);
// Resets the state for a gimmick and returns its user data
void *GimmickState_SetID(GimmickState *gimmick, u32 gimmickId);
// The gimmick's own data. The ID is not checked
void *GimmickState_GetUserData(GimmickState *gimmick, u32 gimmickId);
// The two areas after the user data. The ID is not checked
void *func_0200bac8(GimmickState *gimmick, u32 gimmickId);
void *func_0200bacc(GimmickState *gimmick);
u32 GimmickState_GetID(GimmickState *gimmick);
void GimmickState_StoreSave(const GimmickState *gimmick, SaveControl *save);
void GimmickState_LoadSave(SaveControl *save, GimmickState *gimmick);

#endif // POKEBW2_SAVE_GIMMICK_SAVEDATA_H
