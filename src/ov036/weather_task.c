// weather_task.c: the field's weather task, which runs one weather's data (the functions of its WeatherData) and keeps
// what they use: its archive and graphics, its particles, the weather's sound, the fog and the timer. The name is the
// ROM's own, from GFL_HeapAllocate's file argument. The layouts are ours
#include "types.h"
#include "field/field_3dbg.h"
#include "field/field_camera.h"
#include "field/field_environment.h"
#include "field/field_fog.h"
#include "field/field_sound.h"
#include "field/field_weather.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"

// The weather's graphics, which the particles use
typedef struct {
    u16 loaded;
    u16 chars;
    u16 palette;
    u16 cells;
    u32 bgActive[2];
    ArcTool *arc;
} WeatherRes;

// WeatherSound's state: no sound, the sound playing, or waiting for the BGM to finish loading to play it
enum {
    WEATHER_SOUND_NONE,
    WEATHER_SOUND_PLAYING,
    WEATHER_SOUND_WAIT_LOAD,
};

// The weather's sound
typedef struct {
    u16 state;
    u16 se;
    u32 unk04;
    u32 ambience;
} WeatherSound;

// Moves a value and an interval toward their targets, and calls func with the value whenever the interval is up
typedef struct {
    WeatherTask *owner;
    s16 value;
    s16 counter;
    s16 interval;
    s16 stepCount;
    s16 valueTarget;
    s16 intervalTarget;
    s16 intervalStep;
    s16 stepsMax;
    s32 valueStep;
    WeatherSpawnFunc func;
} WeatherTimer;

struct WeatherObj {
    WeatherTask *task;
    ClActor *actor;
    u8 work[0x28];
    WeatherObj *prev;
    WeatherObj *next;
};

#define WEATHER_OBJ_COUNT 32

struct WeatherTask {
    ClActUnit *unit;
    FieldFog *fog;
    FieldLight *light;
    FieldSound *fieldSound;
    FieldFogCtrl *fogCtrl;
    FieldCamera *camera;
    Field3DBg **bgs;
    u16 season;
    const WeatherData *data;
    u8 mode : 4;
    u8 fade : 4;
    u8 stagedLoad;
    u8 state;
    u8 loaded;
    WeatherRes res;
    void *work;
    WeatherSound sound;
    WeatherTimer timer;
    VecFx32 lookAt;
    WeatherObj objs[WEATHER_OBJ_COUNT];
    WeatherObj *objHead;
    int objCount;
    u8 fogPending;
    u8 fogDepthShift;
    u8 fogDuration;
    u32 fogColor;
};

// WeatherTask's state: load the weather's resources at once or over three frames, run its data's functions (funcs[0]
// to funcs[6]: init, fade in or start without one, the weather, fade out, the end of the fade and exit) and free
// what it loaded
enum {
    WEATHER_TASK_NONE,
    WEATHER_TASK_LOAD_ALL,
    WEATHER_TASK_LOAD_RESOURCES,
    WEATHER_TASK_INIT_BILLBOARDS,
    WEATHER_TASK_ALLOC_PARTICLES,
    WEATHER_TASK_INIT,
    WEATHER_TASK_FADE_IN,
    WEATHER_TASK_START,
    WEATHER_TASK_MAIN,
    WEATHER_TASK_FADE_OUT,
    WEATHER_TASK_FADE_OUT_END,
    WEATHER_TASK_EXIT,
    WEATHER_TASK_FREE,
    WEATHER_TASK_STATE_MAX,
};

static const ClActorSetup WEATHER_OBJ_SETUP = { 0, 0, 0, 0x40, 3 };
static const u8 WEATHER_STATE_KINDS[WEATHER_TASK_STATE_MAX] = { 0, 1, 1, 1, 1, 2, 2, 1, 3, 4, 4, 5, 5 };

static BOOL WeatherTask_HasFogChange(WeatherTask *task);
static void WeatherTask_ApplyFogChange(WeatherTask *task);
static void WeatherTask_ClearFogChange(WeatherTask *task);
static void WeatherTask_LoadResources(WeatherTask *task, HeapID heapId);
static void WeatherTask_InitBillboards(WeatherTask *task, HeapID heapId);
static void WeatherTask_AllocParticles(WeatherTask *task, HeapID heapId);
static void WeatherTask_MarkStarted(WeatherTask *task);
static BOOL WeatherTask_CallStageFunc(WeatherTask *task, WeatherFunc func, HeapID heapId);
static void WeatherTask_ResumeSound(WeatherSound *sound, FieldSound *fieldSound);
static void WeatherTask_UpdateParticles(WeatherTask *task);
static u8 WeatherTask_IsStarted(WeatherTask *task);
static void WeatherTask_FreeParticles(WeatherTask *task);
static void WeatherTask_FreeBillboards(WeatherTask *task);
static void WeatherTask_UnloadResources(WeatherTask *task);
static void WeatherTask_Reset(WeatherTask *task);
static void WeatherTask_SetSoundUseSoundSys(WeatherSound *sound, u32 ambience);
static u8 WeatherTask_GetStagePhase(WeatherTask *task);
static WeatherObj *WeatherTask_FindFreeParticle(WeatherTask *task);
static void WeatherTask_LinkParticle(WeatherTask *task, WeatherObj *obj);
static void WeatherTask_UnlinkParticle(WeatherTask *task, WeatherObj *obj);
static BOOL WeatherTask_IsParticleLinked(WeatherObj *obj);
static void WeatherTask_QueueFogChange(WeatherTask *task, u32 depthShift, u32 color, int duration);
static BOOL WeatherTask_TickSpawner(WeatherTimer *timer, HeapID heapId);
static BOOL WeatherTask_TickSpawnTimer(WeatherTimer *timer, HeapID heapId);
static void WeatherTask_InitSpawner(WeatherTimer *timer, WeatherTask *owner, int interval, int value, int valueTarget,
                                    int intervalTarget, int intervalStep, int stepsMax, int valueStep,
                                    WeatherSpawnFunc func);
static void WeatherTask_SetSpawnerTarget(WeatherTimer *timer, int valueTarget, int intervalTarget, int intervalStep,
                                         int valueStep);
static void WeatherTask_GetViewScale(const MtxFx44 *proj, fx32 distance, fx32 *x, fx32 *y);
static ClActor *WeatherTask_GetParticleActorPtr(WeatherObj *obj);
static void WeatherTask_LoadArchive(WeatherRes *res, const WeatherData *data, HeapID heapId);
static void WeatherTask_LoadSpriteResources(WeatherRes *res, const WeatherData *data, HeapID heapId);
static void WeatherTask_FreeSpriteResources(WeatherRes *res);
static void WeatherTask_FreeArchive(WeatherRes *res);
static void WeatherTask_InitBillboard(WeatherRes *res, int index, Field3DBg *bg, const void *config, HeapID heapId);
static void WeatherTask_FreeBillboard(WeatherRes *res, int index, Field3DBg *bg);
static void WeatherTask_CreateParticleActor(WeatherObj *obj, WeatherTask *task, const WeatherRes *res, ClActUnit *unit,
                                            HeapID heapId);
static void WeatherTask_DestroyParticleActor(WeatherObj *obj);
static void WeatherTask_ClearParticle(WeatherObj *obj);
static void *WeatherTask_GetParticleWorkPtr(WeatherObj *obj);
static void WeatherTask_PlaySound(WeatherSound *sound, FieldSound *fieldSound, u32 se);
static void WeatherTask_StopSound(WeatherSound *sound, FieldSound *fieldSound);

WeatherTask *WeatherTask_Create(ClActUnit *unit, FieldCamera *camera, FieldLight *light, FieldFog *fog,
                                FieldFogCtrl *fogCtrl, Field3DBg **bgs, FieldSound *fieldSound, u16 season,
                                HeapID heapId) {
    WeatherTask *task = GFL_HeapAllocate(heapId, sizeof(WeatherTask), TRUE, "weather_task.c", 0x17e);
    task->unit = unit;
    task->camera = camera;
    task->fog = fog;
    task->light = light;
    task->fogCtrl = fogCtrl;
    task->bgs = bgs;
    task->fieldSound = fieldSound;
    task->season = season;
    return task;
}

void WeatherTask_Delete(WeatherTask *task) {
    WeatherTask_ForceEnd(task);
    GFL_HeapFree(task);
}

void WeatherTask_Update(WeatherTask *task, HeapID heapId) {
    if (WeatherTask_HasFogChange(task) && !FieldFog_IsAnimationRunning(task->fog)) {
        WeatherTask_ApplyFogChange(task);
        WeatherTask_ClearFogChange(task);
    }
    switch (task->state) {
    case WEATHER_TASK_LOAD_ALL:
        WeatherTask_LoadResources(task, heapId);
        WeatherTask_InitBillboards(task, heapId);
        WeatherTask_AllocParticles(task, heapId);
        WeatherTask_MarkStarted(task);
        task->state = WEATHER_TASK_INIT;
        break;
    case WEATHER_TASK_LOAD_RESOURCES:
        WeatherTask_LoadResources(task, heapId);
        task->state = WEATHER_TASK_INIT_BILLBOARDS;
        break;
    case WEATHER_TASK_INIT_BILLBOARDS:
        WeatherTask_InitBillboards(task, heapId);
        task->state = WEATHER_TASK_ALLOC_PARTICLES;
        break;
    case WEATHER_TASK_ALLOC_PARTICLES:
        WeatherTask_AllocParticles(task, heapId);
        task->state = WEATHER_TASK_INIT;
        break;
    case WEATHER_TASK_INIT:
        if (WeatherTask_CallStageFunc(task, task->data->funcs[0], heapId)) {
            if (task->fade != 0) {
                task->state = WEATHER_TASK_FADE_IN;
            } else {
                task->state = WEATHER_TASK_START;
            }
        }
        WeatherTask_ResumeSound(&task->sound, task->fieldSound);
        WeatherTask_MarkStarted(task);
        break;
    case WEATHER_TASK_FADE_IN:
        if (WeatherTask_CallStageFunc(task, task->data->funcs[1], heapId)) {
            task->state = WEATHER_TASK_MAIN;
        }
        WeatherTask_UpdateParticles(task);
        WeatherTask_ResumeSound(&task->sound, task->fieldSound);
        break;
    case WEATHER_TASK_START:
        if (WeatherTask_CallStageFunc(task, task->data->funcs[2], heapId)) {
            task->state = WEATHER_TASK_MAIN;
        }
        WeatherTask_ResumeSound(&task->sound, task->fieldSound);
        break;
    case WEATHER_TASK_MAIN:
        WeatherTask_CallStageFunc(task, task->data->funcs[3], heapId);
        WeatherTask_UpdateParticles(task);
        WeatherTask_ResumeSound(&task->sound, task->fieldSound);
        break;
    case WEATHER_TASK_FADE_OUT:
        if (WeatherTask_CallStageFunc(task, task->data->funcs[4], heapId)) {
            task->state = WEATHER_TASK_FADE_OUT_END;
        }
        WeatherTask_UpdateParticles(task);
        WeatherTask_ResumeSound(&task->sound, task->fieldSound);
        break;
    case WEATHER_TASK_FADE_OUT_END:
        if (WeatherTask_CallStageFunc(task, task->data->funcs[5], heapId)) {
            task->state = WEATHER_TASK_EXIT;
        }
        WeatherTask_UpdateParticles(task);
        WeatherTask_ResumeSound(&task->sound, task->fieldSound);
        break;
    case WEATHER_TASK_EXIT:
        if (WeatherTask_IsStarted(task)) {
            WeatherTask_CallStageFunc(task, task->data->funcs[6], heapId);
        }
        task->state = WEATHER_TASK_FREE;
        break;
    case WEATHER_TASK_FREE:
        WeatherTask_FreeParticles(task);
        WeatherTask_FreeBillboards(task);
        WeatherTask_UnloadResources(task);
        WeatherTask_Reset(task);
        task->state = WEATHER_TASK_NONE;
        break;
    }
}

void WeatherTask_Start(WeatherTask *task, const WeatherData *data, u8 stagedLoad, u32 fade, u32 mode, u32 ambience,
                       HeapID heapId) {
    task->data = data;
    task->mode = mode;
    task->fade = fade;
    task->stagedLoad = stagedLoad;
    if (!task->stagedLoad) {
        task->state = WEATHER_TASK_LOAD_ALL;
    } else if (task->stagedLoad == TRUE) {
        task->state = WEATHER_TASK_LOAD_RESOURCES;
    }
    WeatherTask_SetSoundUseSoundSys(&task->sound, ambience);
}

void WeatherTask_RequestEnd(WeatherTask *task, u32 fade, u32 mode) {
    u8 kind = WeatherTask_GetStagePhase(task);
    if (task->state != WEATHER_TASK_NONE && (u32)(kind - 4) > 1) {
        task->fade = fade;
        task->mode = mode;
        if (task->state <= WEATHER_TASK_INIT) {
            task->state = WEATHER_TASK_EXIT;
        } else if (task->fade != 0) {
            task->state = WEATHER_TASK_FADE_OUT;
        } else {
            task->state = WEATHER_TASK_EXIT;
        }
    }
}

void WeatherTask_ForceEnd(WeatherTask *task) {
    if (task->state != WEATHER_TASK_NONE) {
        if (task->state <= WEATHER_TASK_EXIT) {
            WeatherTask_CallStageFunc(task, task->data->funcs[6], 0);
        }
        if (task->state <= WEATHER_TASK_FREE) {
            WeatherTask_FreeParticles(task);
            WeatherTask_FreeBillboards(task);
            WeatherTask_UnloadResources(task);
        }
        WeatherTask_Reset(task);
    }
}

void WeatherTask_FadeLight(WeatherTask *task, u32 arcId, u32 fileId) {
    FieldLight_ChangeData(task->light, arcId, fileId, 0x3c);
}

void WeatherTask_SetLight(WeatherTask *task, u32 arcId, u32 fileId) {
    FieldLight_ChangeData(task->light, arcId, fileId, 1);
}

void WeatherTask_ResetLight(WeatherTask *task, u32 mode) {
    if (mode == WEATHER_MODE_ALONE) {
        FieldLight_ChangeDataDefault(task->light);
    }
}

void WeatherTask_FlashLight(WeatherTask *task, u16 color, u32 frames, u32 returnFrames) {
    FieldLight_StartFlash(task->light, color, frames, returnFrames);
}

BOOL WeatherTask_IsLightFlashing(WeatherTask *task) {
    return FieldLight_IsFlashRunning(task->light);
}

u32 WeatherTask_GetCameraSettingId(WeatherTask *task) {
    return FieldCamera_GetDefaultsIndex(task->camera);
}

void WeatherTask_SetLightColor(WeatherTask *task, u16 color, u16 frames) {
    FieldLight_StartFlashOneWay(task->light, color, frames);
}

void WeatherTask_SetBillboardVisible(WeatherTask *task, u32 enabled, u32 index) {
    Field3DBG_SetBillboardVisible(task->bgs[index], enabled);
}

void WeatherTask_SetBillboardScaleX(WeatherTask *task, int value, u32 index) {
    Field3DBG_SetBillboardScaleX(task->bgs[index], value);
}

void WeatherTask_SetBillboardScaleY(WeatherTask *task, int value, u32 index) {
    Field3DBG_SetBillboardScaleY(task->bgs[index], value);
}

void WeatherTask_SetBillboardTexOffsetX(WeatherTask *task, int value, u32 index) {
    Field3DBG_SetBillboardTexOffsetX(task->bgs[index], value);
}

void WeatherTask_SetBillboardTexOffsetY(WeatherTask *task, int value, u32 index) {
    Field3DBG_SetBillboardTexOffsetY(task->bgs[index], value);
}

u8 WeatherTask_GetPhase(WeatherTask *task) {
    return WeatherTask_GetStagePhase(task);
}

void *WeatherTask_GetWorkBuffer(WeatherTask *task) {
    return task->work;
}

BOOL WeatherTask_GetParticleCount(WeatherTask *task) {
    return task->objCount;
}

WeatherObj *WeatherTask_AddParticle(WeatherTask *task, HeapID heapId) {
    WeatherObj *obj = WeatherTask_FindFreeParticle(task);
    if (obj != NULL) {
        WeatherTask_LinkParticle(task, obj);
    }
    return obj;
}

void WeatherTask_RemoveParticle(WeatherObj *obj) {
    if (WeatherTask_IsParticleLinked(obj)) {
        WeatherTask_UnlinkParticle(obj->task, obj);
    }
}

void WeatherTask_SetupSpawner(WeatherTask *task, u32 value, u32 interval, u32 valueTarget, int intervalTarget,
                              int intervalStep, int stepsMax, int valueStep, WeatherSpawnFunc spawn) {
    WeatherTask_InitSpawner(&task->timer, task, interval, value, valueTarget, intervalTarget, intervalStep, stepsMax,
                            valueStep, spawn);
}

void WeatherTask_RetargetSpawner(WeatherTask *task, u32 valueTarget, u32 intervalTarget, u32 intervalStep,
                                 int valueStep) {
    WeatherTask_SetSpawnerTarget(&task->timer, valueTarget, intervalTarget, intervalStep, valueStep);
}

BOOL WeatherTask_StepSpawner(WeatherTask *task, HeapID heapId) {
    return WeatherTask_TickSpawner(&task->timer, heapId);
}

BOOL WeatherTask_StepSpawnTimer(WeatherTask *task, HeapID heapId) {
    return WeatherTask_TickSpawnTimer(&task->timer, heapId);
}

void WeatherTask_PrewarmParticles(WeatherTask *task, WeatherSpawnFunc spawn, int count, int rateInterval, int rateStep,
                                  HeapID heapId) {
    int j;
    int i;
    WeatherObj *obj;
    WeatherObj *next;
    int rate;

    spawn(task, count, heapId);
    rate = 0;
    obj = task->objHead;
    if (obj != NULL) {
        next = obj->next;
        for (i = 0; i < count; i++) {
            for (j = 0; j < rate; j++) {
                task->data->objFunc(obj);
                if (!WeatherTask_IsParticleLinked(obj)) {
                    break;
                }
            }
            obj = next;
            next = obj->next;
            if (i >= rateInterval && i % rateInterval == 0) {
                rate += rateStep;
            }
            if (obj == task->objHead) {
                break;
            }
        }
    }
}

void WeatherTask_InitFog(WeatherTask *task, u32 depthShift, u32 offset, u32 mode) {
    if (mode == WEATHER_MODE_ALONE) {
        FieldFog_SetAlphaMode(task->fog, 0);
        FieldFog_SetAlpha(task->fog, 0x1f);
        FieldFog_SetDepthShift(task->fog, depthShift);
        FieldFog_SetOffset(task->fog, offset);
        FieldFog_SetDefaultTable(task->fog);
        FieldFog_SetEnabled(task->fog, TRUE);
    }
}

void WeatherTask_DisableFog(WeatherTask *task, u32 mode) {
    if (mode == WEATHER_MODE_ALONE) {
        FieldFog_SetEnabled(task->fog, FALSE);
    }
}

void WeatherTask_ChangeFog(WeatherTask *task, u32 depthShift, u32 color, int duration, u32 mode) {
    int half;
    if (mode == WEATHER_MODE_ALONE) {
        FieldFog_Animate(task->fog, color, depthShift, duration);
        return;
    }
    if (mode == WEATHER_MODE_CROSSFADE) {
        if (depthShift == FieldFog_GetDepthShift(task->fog)) {
            FieldFog_Animate(task->fog, color, depthShift, duration);
            return;
        }
        half = duration / 2;
        FieldFog_Animate(task->fog, 0x7fdf, FieldFog_GetDepthShift(task->fog), half);
        WeatherTask_QueueFogChange(task, depthShift, color, half);
    }
}

void WeatherTask_ChangeFogOffset(WeatherTask *task, u32 color, u32 duration, u32 mode) {
    if (mode == WEATHER_MODE_ALONE) {
        FieldFog_Animate(task->fog, color, FieldFog_GetDepthShift(task->fog), duration);
    }
}

BOOL WeatherTask_IsFogIdle(WeatherTask *task) {
    if (FieldFog_IsAnimationRunning(task->fog)) {
        return FALSE;
    }
    if (WeatherTask_HasFogChange(task)) {
        return FALSE;
    }
    return TRUE;
}

BOOL WeatherTask_IsZoneFogActive(WeatherTask *task) {
    if (FieldFogCtrl_IsLoadRequest(task->fogCtrl) == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL WeatherTask_IsZoneFog(WeatherTask *task) {
    return FieldFogCtrl_CheckArc(task->fogCtrl);
}

BOOL WeatherTask_IsZoneFogLight(WeatherTask *task) {
    return FieldFogCtrl_IsStaticLightExists(task->fogCtrl);
}

u32 WeatherTask_GetZoneFogOffset(WeatherTask *task) {
    return FieldFogCtrl_GetFogParam0(task->fogCtrl);
}

u32 WeatherTask_GetZoneFogSlope(WeatherTask *task) {
    return FieldFogCtrl_GetFogParam1(task->fogCtrl);
}

u32 WeatherTask_GetZoneFogLightId(WeatherTask *task) {
    return FieldFogCtrl_GetStaticLightIndex(task->fogCtrl);
}

void WeatherTask_SaveCameraPos(WeatherTask *task) {
    if (task->camera != NULL) {
        VecFx32 target;
        GFL_G3DCameraGetLookatTarget(FieldCamera_GetG3DCamera(task->camera), &target);
        task->lookAt = target;
    }
}

void WeatherTask_GetCameraScroll(WeatherTask *task, int *x, int *y) {
    G3DCamera *camera;
    VecFx32 target;
    VecFx32 pos;
    int dx;
    int dz;
    fx32 scaleX;
    fx32 scaleY;
    int signX;
    int signY;
    int outX;
    int outY;

    if (task->camera == NULL) {
        *x = 0;
        *y = 0;
        return;
    }
    camera = FieldCamera_GetG3DCamera(task->camera);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    GFL_G3DCameraGetLookatPos(camera, &pos);
    dx = target.x - task->lookAt.x;
    dz = target.z - task->lookAt.z;
    if (MATH_ABS(dx) > 0x3e8000 || MATH_ABS(dz) > 0x64000) {
        task->lookAt = target;
        *x = 0;
        *y = 0;
        return;
    }
    WeatherTask_GetViewScale(&NNS_G3dGlb.projMtx, VEC_Distance(&target, &pos), &scaleX, &scaleY);
    scaleX = FX_Div(scaleX, 0x100000);
    scaleY = FX_Div(scaleY, 0xc0000);
    signX = FX32_ONE;
    if (dx < 0) {
        dx = -dx;
        signX = -FX32_ONE;
    }
    outX = FX_Div(dx, scaleX);
    if (signX < 0) {
        outX = -outX;
        outX += FX32_ONE;
    }
    signY = FX32_ONE;
    if (dz < 0) {
        dz = -dz;
        signY = -FX32_ONE;
    }
    outY = FX_Div(dz, scaleY);
    if (signY < 0) {
        outY = -outY;
        outY += FX32_ONE;
    }
    if (outX + outY != 0) {
        task->lookAt = target;
    }
    *x = outX >> 12;
    *y = outY >> 12;
}

void WeatherTask_ShiftParticles(WeatherTask *task, int x, int y) {
    WeatherObj *obj = task->objHead;
    if (obj != NULL) {
        do {
            ClActor *actor = WeatherTask_GetParticleActorPtr(obj);
            ClActorPos pos;
            ClActor_GetPos(actor, &pos);
            pos.x -= (s16)x;
            pos.y -= (s16)y;
            ClActor_SetPos(actor, &pos);
            obj = obj->next;
        } while (obj != task->objHead);
    }
}

void WeatherTask_PlaySe(WeatherTask *task, u32 se) {
    WeatherTask_PlaySound(&task->sound, task->fieldSound, se);
}

void WeatherTask_StopSe(WeatherTask *task) {
    WeatherTask_StopSound(&task->sound, task->fieldSound);
}

void WeatherTask_PlaySeDirect(WeatherTask *task, u32 sound) {
    if (!GFL_SndIsLoadThreadRunning()) {
        if (task->sound.ambience != 0) {
            FieldSnd_PlayAmbience(task->fieldSound, sound);
        } else {
            GFL_SndSEPlay(sound);
        }
    }
}

void WeatherTask_PlaySeWithVolume(WeatherTask *task, u32 sound, u32 volume) {
    if (!GFL_SndIsLoadThreadRunning()) {
        if (task->sound.ambience != 0) {
            FieldSnd_PlayAmbienceEx(task->fieldSound, sound, volume);
        } else {
            GFL_SndSEPlayEx(sound, volume);
        }
    }
}

void *WeatherTask_GetParticleWork(WeatherObj *obj) {
    return WeatherTask_GetParticleWorkPtr(obj);
}

ClActor *WeatherTask_GetParticleActor(WeatherObj *obj) {
    return WeatherTask_GetParticleActorPtr(obj);
}

void WeatherTask_GetParticlePos(WeatherObj *obj, ClActorPos *pos) {
    ClActor_GetSurfacePos(WeatherTask_GetParticleActorPtr(obj), pos, 0);
}

void WeatherTask_SetParticlePosWrapped(WeatherObj *obj, const ClActorPos *pos) {
    ClActorPos wrapped = *pos;
    if (wrapped.x > 0x140) {
        wrapped.x %= 0x140;
    } else if (wrapped.x < -0x40) {
        wrapped.x += 0x140;
    }
    if (wrapped.y > 0x100) {
        wrapped.y %= 0x100;
    } else if (wrapped.y < -0x40) {
        wrapped.y += 0x100;
    }
    ClActor_SetSurfacePos(WeatherTask_GetParticleActorPtr(obj), &wrapped, 0);
}

void WeatherTask_SetParticlePos(WeatherObj *obj, const ClActorPos *pos) {
    ClActor_SetSurfacePos(WeatherTask_GetParticleActorPtr(obj), pos, 0);
}

static void WeatherTask_Reset(WeatherTask *task) {
    ClActUnit *unit = task->unit;
    FieldFog *fog = task->fog;
    FieldFogCtrl *fogCtrl = task->fogCtrl;
    FieldLight *light = task->light;
    Field3DBg **bgs = task->bgs;
    FieldCamera *camera = task->camera;
    FieldSound *fieldSound = task->fieldSound;
    u16 season = task->season;
    MI_CpuFill8(task, 0, sizeof(WeatherTask));
    task->unit = unit;
    task->camera = camera;
    task->fieldSound = fieldSound;
    task->fog = fog;
    task->light = light;
    task->season = season;
    task->bgs = bgs;
    task->fogCtrl = fogCtrl;
}

static BOOL WeatherTask_CallStageFunc(WeatherTask *task, WeatherFunc func, HeapID heapId) {
    if (func != NULL) {
        return func(task, task->mode, heapId);
    }
    return TRUE;
}

static u8 WeatherTask_GetStagePhase(WeatherTask *task) {
    return WEATHER_STATE_KINDS[task->state];
}

static WeatherObj *WeatherTask_FindFreeParticle(WeatherTask *task) {
    WeatherObj *obj;
    int i = 0;
    obj = task->objs;
    for (; i < WEATHER_OBJ_COUNT; i++) {
        if (!WeatherTask_IsParticleLinked(obj + i)) {
            return task->objs + i;
        }
    }
    return NULL;
}

static void WeatherTask_LinkParticle(WeatherTask *task, WeatherObj *obj) {
    WeatherObj *last;

    task->objCount++;
    if (task->objHead == NULL) {
        task->objHead = obj;
        obj->next = obj;
        task->objHead->prev = obj;
        return;
    }
    last = task->objHead->prev;
    obj->prev = last;
    obj->next = last->next;
    last->next->prev = obj;
    last->next = obj;
    ClActor_SetVisible(obj->actor, TRUE);
}

static void WeatherTask_UnlinkParticle(WeatherTask *task, WeatherObj *obj) {
    task->objCount--;
    if (task->objHead == obj) {
        if (obj->next == obj) {
            task->objHead = NULL;
        } else {
            task->objHead = obj->next;
        }
    }
    obj->next->prev = obj->prev;
    obj->prev->next = obj->next;
    obj->next = NULL;
    obj->prev = NULL;
    ClActor_SetVisible(obj->actor, FALSE);
}

static void WeatherTask_UpdateParticles(WeatherTask *task) {
    if (task->objHead != NULL && task->data->objFunc != NULL) {
        WeatherObj *obj = task->objHead;
        WeatherObj *next = obj->next;
        do {
            task->data->objFunc(obj);
            obj = next;
            next = next->next;
            if (obj == task->objHead) {
                return;
            }
        } while (task->objHead != NULL);
    }
}

static void WeatherTask_LoadResources(WeatherTask *task, HeapID heapId) {
    WeatherTask_LoadArchive(&task->res, task->data, heapId);
    WeatherTask_LoadSpriteResources(&task->res, task->data, heapId);
}

static void WeatherTask_UnloadResources(WeatherTask *task) {
    WeatherTask_FreeSpriteResources(&task->res);
    WeatherTask_FreeArchive(&task->res);
}

static void WeatherTask_InitBillboards(WeatherTask *task, HeapID heapId) {
    int i;
    for (i = 0; i < task->data->graphics.bgCount; i++) {
        WeatherTask_InitBillboard(&task->res, i, task->bgs[i], task->data->graphics.bgRes[i], heapId);
    }
}

static void WeatherTask_FreeBillboards(WeatherTask *task) {
    int i;
    for (i = 0; i < task->data->graphics.bgCount; i++) {
        WeatherTask_FreeBillboard(&task->res, i, task->bgs[i]);
    }
}

static void WeatherTask_AllocParticles(WeatherTask *task, HeapID heapId) {
    task->work = GFL_HeapAllocate(heapId, task->data->workSize, TRUE, "weather_task.c", 0x874);
    if (task->data->graphics.hasObjs != 0) {
        int i;
        for (i = 0; i < WEATHER_OBJ_COUNT; i++) {
            WeatherTask_CreateParticleActor(&task->objs[i], task, &task->res, task->unit, heapId);
        }
    }
}

static void WeatherTask_FreeParticles(WeatherTask *task) {
    if (task->work != NULL) {
        GFL_HeapFree(task->work);
        task->work = NULL;
    }
    if (task->data->graphics.hasObjs != 0) {
        int i;
        for (i = 0; i < WEATHER_OBJ_COUNT; i++) {
            WeatherTask_DestroyParticleActor(&task->objs[i]);
            WeatherTask_ClearParticle(&task->objs[i]);
        }
    }
    WeatherTask_StopSe(task);
}

static void WeatherTask_MarkStarted(WeatherTask *task) {
    task->loaded = 1;
}

static u8 WeatherTask_IsStarted(WeatherTask *task) {
    return task->loaded;
}

static void WeatherTask_LoadArchive(WeatherRes *res, const WeatherData *data, HeapID heapId) {
    if (data->graphics.hasObjs != 0 || data->graphics.bgCount != 0) {
        res->arc = GFL_ArcSysCreateFileHandle(data->graphics.arcId, heapId);
    }
}

static void WeatherTask_FreeArchive(WeatherRes *res) {
    if (res->arc != NULL) {
        GFL_ArcToolFree(res->arc);
        res->arc = NULL;
    }
}

static void WeatherTask_LoadSpriteResources(WeatherRes *res, const WeatherData *data, HeapID heapId) {
    if (data->graphics.hasObjs != 0) {
        res->chars = ClActSys_LoadChars(res->arc, data->graphics.charsFileId, FALSE, 0, heapId);
        res->palette = ClActSys_LoadPlttRange(res->arc, data->graphics.paletteFileId, 0, 0, 0, 1, heapId);
        res->cells = ClActSys_LoadCellAnims(res->arc, data->graphics.cellFileId, data->graphics.cellAnimFileId, heapId);
        res->loaded = 1;
    }
}

static void WeatherTask_FreeSpriteResources(WeatherRes *res) {
    if (res->loaded != 0) {
        ClActSys_FreeChars(res->chars);
        ClActSys_FreePltt(res->palette);
        ClActSys_FreeCellAnims(res->cells);
        res->loaded = 0;
    }
}

// The layer's texture, from the weather's data, at full alpha
static void WeatherTask_InitBillboard(WeatherRes *res, int index, Field3DBg *bg, const void *config, HeapID heapId) {
    static Field3DBgRes bgRes;

    MI_CpuCopy8(config, &bgRes, sizeof(Field3DBgRes));
    if (bgRes.alpha != 0x1f) {
        bgRes.alpha = 0x1f;
    }
    Field3DBG_LoadBillboardTexture(bg, res->arc, &bgRes, heapId);
    Field3DBG_SetBillboardVisible(bg, 0);
    Field3DBG_SetBillboardPolygonId(bg, 0x20);
    res->bgActive[index] = 1;
}

static void WeatherTask_FreeBillboard(WeatherRes *res, int index, Field3DBg *bg) {
    if (res->bgActive[index] != 0) {
        Field3DBG_FreeBillboardTexture(bg);
        res->bgActive[index] = 0;
    }
}

static void WeatherTask_InitSpawner(WeatherTimer *timer, WeatherTask *owner, int interval, int value, int valueTarget,
                                    int intervalTarget, int intervalStep, int stepsMax, int valueStep,
                                    WeatherSpawnFunc func) {
    timer->owner = owner;
    timer->counter = 0;
    timer->stepCount = 0;
    timer->value = value;
    timer->valueTarget = valueTarget;
    timer->interval = interval;
    timer->intervalTarget = intervalTarget;
    timer->intervalStep = intervalStep;
    timer->stepsMax = stepsMax;
    timer->valueStep = valueStep;
    timer->func = func;
}

static void WeatherTask_SetSpawnerTarget(WeatherTimer *timer, int valueTarget, int intervalTarget, int intervalStep,
                                         int valueStep) {
    timer->valueTarget = valueTarget;
    timer->intervalTarget = intervalTarget;
    timer->intervalStep = intervalStep;
    timer->valueStep = valueStep;
}

static BOOL WeatherTask_TickSpawner(WeatherTimer *timer, HeapID heapId) {
    u32 done = 0;
    u32 neg;
    s16 target;

    if (timer->intervalStep == 0) {
        done |= 1;
    } else {
        neg = timer->intervalStep > 0 ? 0 : 1;
        target = timer->intervalTarget;
        if ((timer->interval >= timer->intervalTarget && neg == 0) || (timer->interval <= target && neg == 1)) {
            done |= 1;
            timer->interval = target;
        }
    }
    if (timer->valueStep == 0) {
        done |= 2;
    } else {
        neg = timer->valueStep > 0 ? 0 : 1;
        target = timer->valueTarget;
        if ((timer->value >= target && neg == 0) || (timer->value <= target && neg == 1)) {
            done |= 2;
            timer->value = target;
        }
    }
    timer->counter--;
    if (timer->counter <= 0) {
        timer->func(timer->owner, timer->value, heapId);
        timer->counter = timer->interval;
        if (!(done & 1)) {
            timer->interval += timer->intervalStep;
        }
        if (!(done & 2)) {
            timer->stepCount++;
            if (timer->stepCount >= timer->stepsMax) {
                timer->stepCount = 0;
                timer->value += (s16)timer->valueStep;
            }
        }
    }
    return (done & 1) != 0;
}

static BOOL WeatherTask_TickSpawnTimer(WeatherTimer *timer, HeapID heapId) {
    timer->counter--;
    if (timer->counter <= 0) {
        timer->func(timer->owner, timer->value, heapId);
        timer->counter = timer->interval;
        return TRUE;
    }
    return FALSE;
}

static void WeatherTask_ClearParticle(WeatherObj *obj) {
    MI_CpuFill8(obj, 0, sizeof(WeatherObj));
}

static BOOL WeatherTask_IsParticleLinked(WeatherObj *obj) {
    if (obj->prev != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void WeatherTask_CreateParticleActor(WeatherObj *obj, WeatherTask *task, const WeatherRes *res, ClActUnit *unit,
                                            HeapID heapId) {
    obj->task = task;
    obj->actor = ClActor_Create(unit, res->chars, res->palette, res->cells, &WEATHER_OBJ_SETUP, 0, heapId);
    ClActor_SetVisible(obj->actor, FALSE);
    ClActor_SetDrawAnim(obj->actor, FALSE);
}

static void WeatherTask_DestroyParticleActor(WeatherObj *obj) {
    if (obj->actor != NULL) {
        ClActor_Delete(obj->actor);
        obj->actor = NULL;
    }
}

static void *WeatherTask_GetParticleWorkPtr(WeatherObj *obj) {
    return obj->work;
}

static ClActor *WeatherTask_GetParticleActorPtr(WeatherObj *obj) {
    return obj->actor;
}

static void WeatherTask_SetSoundUseSoundSys(WeatherSound *sound, u32 ambience) {
    sound->ambience = ambience;
}

static void WeatherTask_PlaySound(WeatherSound *sound, FieldSound *fieldSound, u32 se) {
    WeatherTask_StopSound(sound, fieldSound);
    if (!GFL_SndIsLoadThreadRunning()) {
        sound->state = WEATHER_SOUND_PLAYING;
        sound->se = se;
        if (sound->ambience != 0) {
            FieldSnd_PlayAmbience(fieldSound, sound->se);
        } else {
            GFL_SndSEPlay(sound->se);
        }
    } else {
        sound->state = WEATHER_SOUND_WAIT_LOAD;
        sound->se = se;
    }
}

static void WeatherTask_StopSound(WeatherSound *sound, FieldSound *fieldSound) {
    if (sound->state == WEATHER_SOUND_PLAYING) {
        sound->state = WEATHER_SOUND_NONE;
        if (sound->ambience != 0) {
            FieldSnd_StopAmbience(fieldSound, sound->se);
        } else {
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(sound->se));
        }
    }
}

static void WeatherTask_ResumeSound(WeatherSound *sound, FieldSound *fieldSound) {
    if (sound->state == WEATHER_SOUND_WAIT_LOAD && !GFL_SndIsLoadThreadRunning()) {
        sound->state = WEATHER_SOUND_PLAYING;
        if (sound->ambience != 0) {
            FieldSnd_PlayAmbience(fieldSound, sound->se);
        } else {
            GFL_SndSEPlay(sound->se);
        }
    }
}

static void WeatherTask_QueueFogChange(WeatherTask *task, u32 depthShift, u32 color, int duration) {
    task->fogPending = 1;
    task->fogDepthShift = depthShift;
    task->fogColor = color;
    task->fogDuration = duration / 2;
}

static void WeatherTask_ClearFogChange(WeatherTask *task) {
    task->fogPending = 0;
}

static BOOL WeatherTask_HasFogChange(WeatherTask *task) {
    return task->fogPending;
}

static void WeatherTask_ApplyFogChange(WeatherTask *task) {
    FieldFog_AnimateEx(task->fog, 0x7fdf, task->fogColor, task->fogDepthShift, task->fogDepthShift, task->fogDuration);
}

static void WeatherTask_GetViewScale(const MtxFx44 *proj, fx32 distance, fx32 *x, fx32 *y) {
    *y = FX_MUL(FX_Div(distance, proj->m[1][1]), 2 * FX32_ONE);
    *x = FX_MUL(FX_Div(distance, proj->m[0][0]), 2 * FX32_ONE);
}
