#ifndef POKEBW2_APP_OV306_H
#define POKEBW2_APP_OV306_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 306's screen, which the Battle Subway and the battle proc show

#define OVERLAY_OV306 OVERLAY_ID(306)

typedef struct {
    GameData *gameData;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
    s32 unk14;
} Ov306Param;

extern const GameProcFunctions BtlRecSelProcFunctions;

#endif // POKEBW2_APP_OV306_H
