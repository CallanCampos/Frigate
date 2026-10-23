#ifndef POKEBW2_FIELD_RIVAL_SELECT_H
#define POKEBW2_FIELD_RIVAL_SELECT_H

#include "types.h"
#include "struct_decls.h"

struct RivalEntry {
    u8 id;
    u8 unk1;
    u8 selected;
    u8 active;
    // From overlay 26's table of the mission's people
    u16 unk4;
    u8 unk6;
};

u8 getHollowNum(RivalDataSave *save);

RivalEntry *RivalSelect_GetEntry(FesGimmick *gimmick, u32 id);
void RivalSelect_SelectOtherRandom(FesGimmick *gimmick, u32 id);
BOOL RivalSelect_RemoveHollowActorIfMission(FesGimmick *gimmick);
BOOL RivalSelect_RemoveHollowActor(FesGimmick *gimmick);
BOOL RivalSelect_IsSelectedAndActive(FesGimmick *gimmick, u32 id);
void RivalSelect_SetActive(FesGimmick *gimmick, u32 id);
u8 RivalSelect_FindEntryIndex(FesGimmick *gimmick, u32 id);

#endif // POKEBW2_FIELD_RIVAL_SELECT_H
