#ifndef POKEBW2_FIELD_RIVAL_SELECT_H
#define POKEBW2_FIELD_RIVAL_SELECT_H

#include "types.h"
#include "struct_decls.h"

struct RivalEntry {
    u8 id;
    // Whether the Pokémon is female
    u8 female;
    u8 selected;
    u8 active;
    // The Pokémon's species and form, from overlay 26's table of the mission's people
    u16 species;
    u8 form;
};

u8 getHollowNum(RivalDataSave *save);

RivalEntry *RivalSelect_GetEntry(FesGimmick *gimmick, u32 id);
void RivalSelect_SelectOtherRandom(FesGimmick *gimmick, u32 id);
BOOL RivalSelect_RemoveHollowActorIfMission(FesGimmick *gimmick);
BOOL RivalSelect_RemoveHollowActor(FesGimmick *gimmick);
BOOL RivalSelect_IsSelectedAndActive(FesGimmick *gimmick, u32 id);
void RivalSelect_SetActive(FesGimmick *gimmick, u32 id);
// The index of the mission's entry for id among its 10, or RIVAL_ENTRY_NONE when none has it
#define RIVAL_ENTRY_NONE 10
u8 RivalSelect_FindEntryIndex(FesGimmick *gimmick, u32 id);

#endif // POKEBW2_FIELD_RIVAL_SELECT_H
