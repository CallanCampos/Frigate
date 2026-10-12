#ifndef POKEBW2_APP_DEMO3D_DEMO3D_CMDSYS_H
#define POKEBW2_APP_DEMO3D_DEMO3D_CMDSYS_H

// What runs a 3D demo's commands at their frames (overlay 293), named after the ROM's "demo3d_cmdsys.c"

#include "types.h"
#include "app/demo3d/demo3d_capture.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_engine.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "system/gf_font.h"

struct Demo3DCmdSys {
    HeapID heapId;
    HeapID tailHeapId;
    u32 demoId;
    // No commands are left
    BOOL finished;
    // The demo is over: the commands' tasks end at once
    BOOL ending;
    s32 lastFrame;
    s32 cmdIndex;
    s8 textSpeed;
    // DEMO3D_BGM_CHANGE_PUSH has pushed the BGM
    u8 bgmPushed;
    u8 droppedFrame;
    Demo3DEngine *engine;
    TCBExManager *tcbMgr;
    Font *font;
    ArcTool *arc;
    Demo3DCapture *capture;
};

Demo3DCmdSys *Demo3DCmdSys_Create(Demo3DEngine *engine, HeapID heapId);
void Demo3DCmdSys_Free(Demo3DCmdSys *sys);
// Runs the commands of a frame of the camera's animation, or none for -FX32_ONE. droppedFrame is nonzero for a
// frame that is played to catch up
void Demo3DCmdSys_Main(Demo3DCmdSys *sys, fx32 frame, u32 droppedFrame);

#endif // POKEBW2_APP_DEMO3D_DEMO3D_CMDSYS_H
