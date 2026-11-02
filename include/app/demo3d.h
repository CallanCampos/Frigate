#ifndef POKEBW2_APP_DEMO3D_H
#define POKEBW2_APP_DEMO3D_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The 3D demos, overlay 293 (demo3d.c; the headers of its other files are in app/demo3d/)
#define OVERLAY_DEMO3D OVERLAY_ID(293)

// What a demo is started with, which CreateSeqLoadParam fills in
typedef struct {
    GameSystem *gsys;
    u32 demoId;
    // The frame of the camera's animation that the demo starts at
    u32 startFrame;
    u8 param;
    u8 hour;
    u8 minute;
    u8 season : 7;
    u8 playerSex : 1;
    // The frame the demo ended at
    u32 endFrame;
    // What the demo left, a DEMO3D_RESULT_*. The Royal Unova's cruise is the one demo a key ends
    u32 result;
} Demo3DParam;

// How a demo ended: not yet, by a key, or at the end of its camera animation
enum {
    DEMO3D_RESULT_NONE,
    DEMO3D_RESULT_SKIPPED,
    DEMO3D_RESULT_ENDED,
};

// The same table: its symbol and its name in demo3d.c, until the symbol is renamed
extern const GameProcFunctions gDemo3DProcFunctions;
extern const GameProcFunctions gDemo3DProcFunctions;

#endif // POKEBW2_APP_DEMO3D_H
