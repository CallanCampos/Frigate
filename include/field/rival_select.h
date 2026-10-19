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

RivalEntry *func_ov073_021e8be0(RivalSelectContext *context, u32 id);
void func_ov073_021e8bfc(RivalSelectContext *context, u32 id);
BOOL func_ov073_021e8c4c(RivalSelectContext *context);
BOOL func_ov073_021e8c64(RivalSelectContext *context);
BOOL func_ov073_021e8cac(RivalSelectContext *context, u32 id);
void func_ov073_021e8cdc(RivalSelectContext *context, u32 id);
u8 func_ov073_021e8d00(RivalSelectContext *context, u32 id);

#endif // POKEBW2_FIELD_RIVAL_SELECT_H
