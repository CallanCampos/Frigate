#ifndef POKEBW2_APP_DEMO3D_DEMO3D_ENGINE_H
#define POKEBW2_APP_DEMO3D_DEMO3D_ENGINE_H

// What plays a 3D demo (overlay 293): its camera animation, models and commands, named after the ROM's
// "demo3d_engine.c"

#include "types.h"
#include "app/demo3d.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_graphic.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct Demo3DCmdSys Demo3DCmdSys;

typedef struct {
    Demo3DGraphic *graphic;
    u32 demoId;
    // Set once the demo is ending: the commands no longer run
    u32 stopped;
    HeapID heapId;
    BOOL dual3D;
    fx32 frameStep;
    G3DCurve *cameraAnm;
    G3DManager *g3dMgr;
    G3DCamera *camera;
    fx32 fovySin;
    fx32 fovyCos;
    s32 mainOffset;
    s32 subOffset;
    fx32 top;
    fx32 bottom;
    u32 unitCount;
    SRTMatrix srt;
    Demo3DCmdSys *cmdSys;
    u16 *scenes;
    const Demo3DSetup *setup;
    Demo3DSceneParam scene;
} Demo3DEngine;

Demo3DEngine *Demo3DEngine_Create(Demo3DGraphic *graphic, Demo3DParam *param, HeapID heapId);
void Demo3DEngine_Free(Demo3DEngine *engine);
// Plays a frame, and the frames that were dropped before it. Returns whether the camera's animation has ended
BOOL Demo3DEngine_Main(Demo3DEngine *engine, u32 droppedFrames);
fx32 Demo3DEngine_GetNowFrame(Demo3DEngine *engine);
u32 Demo3DEngine_GetFrameCount(Demo3DEngine *engine);
void Demo3DEngine_SetStopped(Demo3DEngine *engine, u32 stopped);
void Demo3DEngine_SetCamera(Demo3DEngine *engine, fx32 fovySin, fx32 fovyCos, s32 mainOffset, s32 subOffset);

#endif // POKEBW2_APP_DEMO3D_DEMO3D_ENGINE_H
