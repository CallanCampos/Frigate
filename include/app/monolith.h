#ifndef POKEBW2_APP_MONOLITH_H
#define POKEBW2_APP_MONOLITH_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 143, the Entralink monolith (monolith_main.c and the files in app/monolith/)
#define OVERLAY_MONOLITH OVERLAY_ID(143)

typedef struct {
    GameSystem *gsys;
    u8 unk04[0x28];
    // The special pass powers' flags, the two bytes of HighLinkSave_GetSpecialFlags, then byte 2 set
    u8 powerFlags[4];
    // The net ID passed to func_0202bf68 once a pass power is received
    u8 netId;
    u8 unk31;
} MonolithParam;

extern const GameProcFunctions MONOLITH_PROC_FUNCTIONS;

#endif // POKEBW2_APP_MONOLITH_H
