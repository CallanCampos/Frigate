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

struct RivalSelectContext {
    void *unk0;
    GameData *gameData;
    void *entryOwner;
    u8 padding[0xc];
    Field *field;
    MMSys *actors;
};

RivalEntry *LinkFestival_GetEntries(void *owner);
u8 getHollowNum(RivalDataSave *save);

RivalEntry *RivalSelect_GetEntry(RivalSelectContext *context, u32 id);
void RivalSelect_SelectOtherRandom(RivalSelectContext *context, u32 id);
BOOL RivalSelect_RemoveHollowActorIfMission(RivalSelectContext *context);
BOOL RivalSelect_RemoveHollowActor(RivalSelectContext *context);
BOOL RivalSelect_IsSelectedAndActive(RivalSelectContext *context, u32 id);
void RivalSelect_SetActive(RivalSelectContext *context, u32 id);
u8 RivalSelect_FindEntryIndex(RivalSelectContext *context, u32 id);

#endif // POKEBW2_FIELD_RIVAL_SELECT_H
