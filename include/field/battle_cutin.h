#ifndef POKEBW2_FIELD_BATTLE_CUTIN_H
#define POKEBW2_FIELD_BATTLE_CUTIN_H

#include "types.h"
#include "struct_decls.h"

struct BattleCutinParam {
    u32 cutin;
    u32 animation;
    u16 color;
    u16 color2;
};

extern const BattleCutinParam BATTLE_CUTIN_DB[];

const BattleCutinParam *BattleCutinDB_GetParam(u32 index);

#endif // POKEBW2_FIELD_BATTLE_CUTIN_H
