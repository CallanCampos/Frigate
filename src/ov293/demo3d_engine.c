#include "types.h"
#include "app/demo3d/demo3d_engine.h"
#include "app/demo3d/demo3d_capture.h"
#include "app/demo3d/demo3d_cmdsys.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_graphic.h"
#include "field/field_light_status.h"
#include "gfl/double3Ddisp.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/rtc.h"

// What plays a 3D demo, named after the ROM's "demo3d_engine.c": it steps the camera's animation and the models'
// by the same frames, runs the commands of each frame and draws the models

static void Demo3DEngine_VBlank(void *data);
static void Demo3DEngine_FreeCamera(G3DCamera *camera);
static void Demo3DEngine_SetDual3DCamera(Demo3DEngine *engine, G3DCamera *camera);
static void Demo3DEngine_Setup3D(Demo3DEngine *engine, Demo3DParam *param, const Demo3DSetup *setup, HeapID heapId);

static const VecFx32 sDemo3DCameraPosition = { 0, 0, 0 };
static const VecFx32 sDemo3DCameraUpVector = { 0, FX32_ONE, 0 };
static const VecFx32 sDemo3DCameraTarget = { 0, 0, 0 };

static void Demo3DEngine_VBlank(void *data) {
    GFL_G3DDual3DExecVBlank();
}

static void Demo3DEngine_FreeCamera(G3DCamera *camera) {
    GFL_G3DCameraFree(camera);
}

Demo3DEngine *Demo3DEngine_Create(Demo3DGraphic *graphic, Demo3DParam *param, HeapID heapId) {
    Demo3DEngine *engine = GFL_HeapAllocate(heapId, sizeof(Demo3DEngine), TRUE, "demo3d_engine.c", 121);
    G3DSceneSetup *sceneSetup;
    u32 i;
    int j;

    engine->graphic = graphic;
    engine->demoId = param->demoId;
    engine->heapId = heapId;
    engine->scene.demoId = engine->demoId;
    engine->scene.param = param->param;
    engine->scene.startFrame = param->unk08;
    engine->scene.dayPeriod = GetDayPeriod(param->season, param->hour);
    engine->scene.playerSex = param->playerSex;
    engine->scene.hour = param->hour;
    engine->scene.minute = param->minute;
    engine->scene.season = param->season;
    engine->setup = Demo3DData_GetSetup(engine->demoId);
    Demo3DEngine_SetCamera(engine, engine->setup->fovySin, engine->setup->fovyCos, engine->setup->mainOffset,
                           engine->setup->subOffset);
    Demo3DEngine_Setup3D(engine, param, engine->setup, heapId);
    engine->frameStep = Demo3DData_GetFrameStep(engine->demoId);
    engine->dual3D = Demo3DData_IsDual3D(engine->demoId);
    if (engine->dual3D) {
        GFL_G3DDual3DInit(heapId);
        GFL_VBlankSetCallback(Demo3DEngine_VBlank, NULL);
    }
    VEC_Set(&engine->srt.translation, 0, 0, 0);
    VEC_Set(&engine->srt.scale, FX32_ONE, FX32_ONE, FX32_ONE);
    MTX_Identity33_(&engine->srt.rotation);
    engine->unitCount = Demo3DData_GetUnitCount(engine->demoId);
    engine->scenes = GFL_HeapAllocate(heapId, engine->unitCount * sizeof(u16), TRUE, "demo3d_engine.c", 192);
    engine->cameraAnm = Demo3DData_LoadCamera(engine->demoId, heapId, 10);
    GFL_G3DCurveFrameSet(engine->cameraAnm, FX32_CONST(engine->scene.startFrame));
    engine->g3dMgr = GFL_G3DMgrCreate(engine->unitCount * 6, engine->unitCount * 6, heapId);
    sceneSetup = Demo3DData_CreateSceneSetup(heapId);
    for (i = 0; i < engine->unitCount; i++) {
        Demo3DData_SetSceneSetup(sceneSetup, &engine->scene, i);
        engine->scenes[i] = GFL_G3DMgrNewScene(engine->g3dMgr, sceneSetup);
    }
    Demo3DData_FreeSceneSetup(sceneSetup);
    for (i = 0; i < engine->unitCount; i++) {
        G3DActor *actor = GFL_G3DMgrGetActor(engine->g3dMgr, engine->scenes[i]);
        int count = GFL_G3DActorGetAnmCount(actor);

        for (j = 0; j < count; j++) {
            fx32 frame = FX32_CONST(engine->scene.startFrame);

            GFL_G3DActorBindAnm(actor, j);
            GFL_G3DActorSetAnmFrame(actor, j, &frame);
        }
    }
    engine->cmdSys = Demo3DCmdSys_Create(engine, heapId);
    return engine;
}

void Demo3DEngine_Free(Demo3DEngine *engine) {
    u32 i;

    Demo3DCmdSys_Free(engine->cmdSys);
    Demo3DEngine_FreeCamera(engine->camera);
    if (engine->dual3D) {
        GFL_VBlankResetCallback();
        GFL_G3DDual3DFree();
    }
    GFL_G3DCurveFree(engine->cameraAnm);
    for (i = 0; i < engine->unitCount; i++) {
        GFL_G3DMgrDeleteScene(engine->g3dMgr, engine->scenes[i]);
    }
    GFL_G3DMgrFree(engine->g3dMgr);
    GFL_HeapFree(engine->scenes);
    GFL_HeapFree(engine);
}

// Moves the view of the screen that is being drawn up or down by its offset
static void Demo3DEngine_SetDual3DCamera(Demo3DEngine *engine, G3DCamera *camera) {
    fx16 offset = GFL_G3DDual3DGetRenderDisp() == 0 ? engine->mainOffset : engine->subOffset;

    GFL_G3DCameraOrthoSetTop(camera, offset + engine->top);
    GFL_G3DCameraOrthoSetBottom(camera, offset + engine->bottom);
}

BOOL Demo3DEngine_Main(Demo3DEngine *engine, u32 droppedFrames) {
    BOOL ended;
    fx32 frame;
    u32 i;
    int j;
    fx32 step;
    G3DActor *actor;

    step = 0;
    frame = GFL_G3DCurveGetNowFrame(engine->cameraAnm);
    for (i = 0; i < droppedFrames + 1; i++) {
        if (engine->stopped == 0) {
            Demo3DCmdSys_Main(engine->cmdSys, frame + step, i);
        } else {
            Demo3DCmdSys_Main(engine->cmdSys, -FX32_ONE, i);
        }
        step += engine->frameStep;
    }
    ended = GFL_G3DCurveFrameStep(engine->cameraAnm, step);
    GFL_G3DCurveApplyCamera(engine->camera, engine->cameraAnm);
    for (i = 0; i < engine->unitCount; i++) {
        int count;

        actor = GFL_G3DMgrGetActor(engine->g3dMgr, engine->scenes[i]);
        count = GFL_G3DActorGetAnmCount(actor);
        for (j = 0; j < count; j++) {
            GFL_G3DActorStepAnmFrameLoop(actor, j, step);
        }
    }
    if (engine->dual3D) {
        Demo3DEngine_SetDual3DCamera(engine, engine->camera);
    }
    GFL_G3DCameraFlush(engine->camera);
    Demo3DGraphic_Begin3D(engine->graphic);
    for (i = 0; i < engine->unitCount; i++) {
        GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(engine->g3dMgr, engine->scenes[i]), &engine->srt);
    }
    Demo3DGraphic_End3D(engine->graphic);
    if (engine->dual3D) {
        GFL_G3DDual3DRaiseNeedRenderFlag();
    }
    return ended;
}

fx32 Demo3DEngine_GetNowFrame(Demo3DEngine *engine) {
    return GFL_G3DCurveGetNowFrame(engine->cameraAnm);
}

u32 Demo3DEngine_GetFrameCount(Demo3DEngine *engine) {
    return GFL_G3DCurveGetFrameCount(engine->cameraAnm);
}

void Demo3DEngine_SetStopped(Demo3DEngine *engine, u32 stopped) {
    engine->stopped = stopped;
}

// The demos' view is 4:3
#define DEMO3D_ASPECT ((u32)FX32_CONST(4.0 / 3.0))

// Makes the camera again from the projection's angles, as a frustum
void Demo3DEngine_SetCamera(Demo3DEngine *engine, fx32 fovySin, fx32 fovyCos, s32 mainOffset, s32 subOffset) {
    fx32 right;
    fx32 left;
    fx32 scale;
    fx32 width;
    fx32 top;
    fx32 height;
    fx16 sin;
    u16 angle;
    fx32 far;
    fx32 scaleW;
    HeapID heapId;
    fx32 bottom;
    fx32 cos;
    fx32 near;

    engine->fovySin = fovySin;
    engine->fovyCos = fovyCos;
    engine->mainOffset = mainOffset;
    engine->subOffset = subOffset;
    if (engine->camera != NULL) {
        Demo3DEngine_FreeCamera(engine->camera);
    }
    angle = FX_DEG_TO_IDX(FX_Div(engine->fovySin, FX32_CONST(2)));
    cos = FX_CosIdx(FX_DEG_TO_IDX(FX_Div(engine->fovyCos, FX32_CONST(2))));
    heapId = engine->heapId;
    scaleW = engine->setup->scaleW;
    near = engine->setup->near;
    far = engine->setup->far;
    sin = FX_SinIdx(angle);
    height = FX_Mul(near, sin);
    top = FX_Div(height, cos);
    bottom = -FX_Div(height, cos);
    width = FX_Mul(near, DEMO3D_ASPECT);
    scale = FX32_ONE;
    right = FX_Div(FX_Mul(FX_Div(width, scale), sin), cos);
    left = -FX_Div(FX_Mul(FX_Div(width, scale), sin), cos);
    engine->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_FRUSTUM, top, bottom, left, right, near, far, scaleW,
                                         &sDemo3DCameraPosition, &sDemo3DCameraUpVector, &sDemo3DCameraTarget, heapId);
    GFL_G3DCameraOrthoGetTop(engine->camera, &engine->top);
    GFL_G3DCameraOrthoGetBottom(engine->camera, &engine->bottom);
    GFL_G3DCameraFlush(engine->camera);
}

// Sets the 3D display up as the demo's data has it, with the light of the demo's zone at the time of day
static void Demo3DEngine_Setup3D(Demo3DEngine *engine, Demo3DParam *param, const Demo3DSetup *setup, HeapID heapId) {
    GXRgb colors[10];
    u32 fogTable[8];
    u32 i;

    FieldLightStatus_Get(setup->zoneId, param->hour, param->minute, 0, param->season, colors, heapId);
    Demo3DGraphic_SetLightColors(engine->graphic, colors, 0);
    G3X_AntiAlias(setup->antiAlias);
    G3X_AlphaTest(setup->alphaTest, engine->setup->alphaTestRef);
    G3X_AlphaBlend(setup->alphaBlend);
    G3X_EdgeMarking(setup->edgeMarking);
    if (setup->edgeMarking) {
        gfxSetEdgeColorTable(setup->edgeColors);
    }
    gfxSetFog(setup->fog, setup->fogAlphaMode, setup->fogShift, setup->fogOffset);
    if (setup->fog) {
        G3X_SetFogColor(setup->fogColor, setup->fogAlpha);
        for (i = 0; i < NELEMS(fogTable); i++) {
            fogTable[i] = setup->fogTable[i];
        }
        gfxSetFogTable(fogTable);
    }
    gfxClearColor(setup->clearColor, setup->clearAlpha, setup->clearDepth, setup->clearPolygonId, setup->fog);
    gfxSet1DotDepth(setup->polygon1DotDepth);
    GFL_G3DSysSetSwapBufferParams(setup->sortMode, setup->bufferMode);
}
