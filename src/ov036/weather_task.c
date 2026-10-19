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

// The weather's sound: state 1 plays it, state 2 waits for the BGM to finish loading
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
    u8 unk25;
    u8 state;
    u8 loaded;
    WeatherRes res;
    void *work;
    WeatherSound sound;
    WeatherTimer timer;
    VecFx32 lookAt;
    WeatherObj objs[32];
    WeatherObj *objHead;
    int objCount;
    u8 fogPending;
    u8 fogDepthShift;
    u8 fogDuration;
    u32 fogColor;
};

#define WEATHER_OBJ_COUNT 32

static const ClActorSetup WEATHER_OBJ_SETUP = { 0, 0, 0, 0x40, 3 };
static const u8 WEATHER_STATE_KINDS[13] = { 0, 1, 1, 1, 1, 2, 2, 1, 3, 4, 4, 5, 5 };

static BOOL func_ov036_0219a324(WeatherTask *task);
static void func_ov036_0219a330(WeatherTask *task);
static void func_ov036_0219a318(WeatherTask *task);
static void func_ov036_02199e58(WeatherTask *task, HeapID heapId);
static void func_ov036_02199e8c(WeatherTask *task, HeapID heapId);
static void func_ov036_02199ef8(WeatherTask *task, HeapID heapId);
static void func_ov036_02199f94(WeatherTask *task);
static BOOL func_ov036_02199d50(WeatherTask *task, WeatherFunc func, HeapID heapId);
static void func_ov036_0219a2c4(WeatherSound *sound, FieldSound *fieldSound);
static void func_ov036_02199e24(WeatherTask *task);
static u8 func_ov036_02199f9c(WeatherTask *task);
static void func_ov036_02199f50(WeatherTask *task);
static void func_ov036_02199ecc(WeatherTask *task);
static void func_ov036_02199e78(WeatherTask *task);
static void func_ov036_02199d04(WeatherTask *task);
static void func_ov036_0219a258(WeatherSound *sound, u32 ambience);
static u8 func_ov036_02199d68(WeatherTask *task);
static WeatherObj *func_ov036_02199d78(WeatherTask *task);
static void func_ov036_02199da8(WeatherTask *task, WeatherObj *obj);
static void func_ov036_02199de4(WeatherTask *task, WeatherObj *obj);
static BOOL func_ov036_0219a1f0(WeatherObj *obj);
static void func_ov036_0219a2f4(WeatherTask *task, u32 depthShift, u32 color, int duration);
static BOOL func_ov036_0219a0e4(WeatherTimer *timer, HeapID heapId);
static BOOL func_ov036_0219a1b8(WeatherTimer *timer, HeapID heapId);
static void func_ov036_0219a0b0(WeatherTimer *timer, WeatherTask *owner, int interval, int value, int valueTarget,
                                int intervalTarget, int intervalStep, int stepsMax, int valueStep,
                                WeatherSpawnFunc func);
static void func_ov036_0219a0d8(WeatherTimer *timer, int valueTarget, int intervalTarget, int intervalStep,
                                int valueStep);
static void func_ov036_0219a35c(const MtxFx44 *proj, fx32 distance, fx32 *x, fx32 *y);
static ClActor *func_ov036_0219a254(WeatherObj *obj);
static void func_ov036_02199fa4(WeatherRes *res, const WeatherData *data, HeapID heapId);
static void func_ov036_02199fd4(WeatherRes *res, const WeatherData *data, HeapID heapId);
static void func_ov036_0219a020(WeatherRes *res);
static void func_ov036_02199fc0(WeatherRes *res);
static void func_ov036_0219a044(WeatherRes *res, int index, Field3DBg *bg, const void *config, HeapID heapId);
static void func_ov036_0219a094(WeatherRes *res, int index, Field3DBg *bg);
static void func_ov036_0219a200(WeatherObj *obj, WeatherTask *task, WeatherRes *res, ClActUnit *unit, HeapID heapId);
static void func_ov036_0219a23c(WeatherObj *obj);
static void func_ov036_0219a1e4(WeatherObj *obj);
static void *func_ov036_0219a250(WeatherObj *obj);
static void func_ov036_0219a25c(WeatherSound *sound, FieldSound *fieldSound, u32 se);
static void func_ov036_0219a298(WeatherSound *sound, FieldSound *fieldSound);

WeatherTask *func_ov036_021993e0(ClActUnit *unit, FieldCamera *camera, FieldLight *light, FieldFog *fog,
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

void func_ov036_02199424(WeatherTask *task) {
    func_ov036_021996f4(task);
    GFL_HeapFree(task);
}

void func_ov036_02199434(WeatherTask *task, HeapID heapId) {
    if (func_ov036_0219a324(task) && !FieldFog_IsAnimationRunning(task->fog)) {
        func_ov036_0219a330(task);
        func_ov036_0219a318(task);
    }
    switch (task->state) {
    case 1:
        func_ov036_02199e58(task, heapId);
        func_ov036_02199e8c(task, heapId);
        func_ov036_02199ef8(task, heapId);
        func_ov036_02199f94(task);
        task->state = 5;
        break;
    case 2:
        func_ov036_02199e58(task, heapId);
        task->state = 3;
        break;
    case 3:
        func_ov036_02199e8c(task, heapId);
        task->state = 4;
        break;
    case 4:
        func_ov036_02199ef8(task, heapId);
        task->state = 5;
        break;
    case 5:
        if (func_ov036_02199d50(task, task->data->funcs[0], heapId)) {
            if (task->fade != 0) {
                task->state = 6;
            } else {
                task->state = 7;
            }
        }
        func_ov036_0219a2c4(&task->sound, task->fieldSound);
        func_ov036_02199f94(task);
        break;
    case 6:
        if (func_ov036_02199d50(task, task->data->funcs[1], heapId)) {
            task->state = 8;
        }
        func_ov036_02199e24(task);
        func_ov036_0219a2c4(&task->sound, task->fieldSound);
        break;
    case 7:
        if (func_ov036_02199d50(task, task->data->funcs[2], heapId)) {
            task->state = 8;
        }
        func_ov036_0219a2c4(&task->sound, task->fieldSound);
        break;
    case 8:
        func_ov036_02199d50(task, task->data->funcs[3], heapId);
        func_ov036_02199e24(task);
        func_ov036_0219a2c4(&task->sound, task->fieldSound);
        break;
    case 9:
        if (func_ov036_02199d50(task, task->data->funcs[4], heapId)) {
            task->state = 10;
        }
        func_ov036_02199e24(task);
        func_ov036_0219a2c4(&task->sound, task->fieldSound);
        break;
    case 10:
        if (func_ov036_02199d50(task, task->data->funcs[5], heapId)) {
            task->state = 11;
        }
        func_ov036_02199e24(task);
        func_ov036_0219a2c4(&task->sound, task->fieldSound);
        break;
    case 11:
        if (func_ov036_02199f9c(task)) {
            func_ov036_02199d50(task, task->data->funcs[6], heapId);
        }
        task->state = 12;
        break;
    case 12:
        func_ov036_02199f50(task);
        func_ov036_02199ecc(task);
        func_ov036_02199e78(task);
        func_ov036_02199d04(task);
        task->state = 0;
        break;
    }
}

void func_ov036_02199618(WeatherTask *task, const WeatherData *data, u8 unk25, u32 fade, u32 mode, u32 ambience,
                         HeapID heapId) {
    task->data = data;
    task->mode = mode;
    task->fade = fade;
    task->unk25 = unk25;
    if (task->unk25 == 0) {
        task->state = 1;
    } else if (task->unk25 == 1) {
        task->state = 2;
    }
    func_ov036_0219a258(&task->sound, ambience);
}

void func_ov036_0219967c(WeatherTask *task, u32 fade, u32 mode) {
    u8 kind = func_ov036_02199d68(task);
    if (task->state != 0 && (u32)(kind - 4) > 1) {
        task->fade = fade;
        task->mode = mode;
        if (task->state <= 5) {
            task->state = 11;
        } else if (task->fade != 0) {
            task->state = 9;
        } else {
            task->state = 11;
        }
    }
}

void func_ov036_021996f4(WeatherTask *task) {
    if (task->state != 0) {
        if (task->state <= 11) {
            func_ov036_02199d50(task, task->data->funcs[6], 0);
        }
        if (task->state <= 12) {
            func_ov036_02199f50(task);
            func_ov036_02199ecc(task);
            func_ov036_02199e78(task);
        }
        func_ov036_02199d04(task);
    }
}

void func_ov036_02199734(WeatherTask *task, u32 a1, u32 a2) {
    FieldLight_ChangeData(task->light, a1, a2, 0x3c);
}

void func_ov036_02199740(WeatherTask *task, u32 a1, u32 a2) {
    FieldLight_ChangeData(task->light, a1, a2, 1);
}

void func_ov036_0219974c(WeatherTask *task, u32 a1) {
    if (a1 == 1) {
        FieldLight_ChangeDataDefault(task->light);
    }
}

void func_ov036_0219975c(WeatherTask *task, u16 a1, u32 a2, u32 a3) {
    FieldLight_StartFlash(task->light, a1, a2, a3);
}

BOOL func_ov036_02199768(WeatherTask *task) {
    return FieldLight_IsFlashRunning(task->light);
}

u32 func_ov036_02199774(WeatherTask *task) {
    return FieldCamera_GetDefaultsIndex(task->camera);
}

void func_ov036_02199780(WeatherTask *task, u16 a1, u16 a2) {
    FieldLight_StartFlashOneWay(task->light, a1, a2);
}

void func_ov036_0219978c(WeatherTask *task, u32 a1, u32 index) {
    func_ov036_02198cf0(task->bgs[index], a1);
}

void func_ov036_0219979c(WeatherTask *task, int value, u32 index) {
    func_ov036_02198d00(task->bgs[index], value);
}

void func_ov036_021997ac(WeatherTask *task, int value, u32 index) {
    func_ov036_02198d04(task->bgs[index], value);
}

void func_ov036_021997bc(WeatherTask *task, int value, u32 index) {
    func_ov036_02198cf8(task->bgs[index], value);
}

void func_ov036_021997cc(WeatherTask *task, int value, u32 index) {
    func_ov036_02198cfc(task->bgs[index], value);
}

u8 func_ov036_021997dc(WeatherTask *task) {
    return func_ov036_02199d68(task);
}

void *func_ov036_021997e4(WeatherTask *task) {
    return task->work;
}

BOOL func_ov036_021997e8(WeatherTask *task) {
    return task->objCount;
}

WeatherObj *func_ov036_021997f4(WeatherTask *task, HeapID heapId) {
    WeatherObj *obj = func_ov036_02199d78(task);
    if (obj != NULL) {
        func_ov036_02199da8(task, obj);
    }
    return obj;
}

void func_ov036_0219980c(WeatherObj *obj) {
    if (func_ov036_0219a1f0(obj)) {
        func_ov036_02199de4(obj->task, obj);
    }
}

void func_ov036_02199824(WeatherTask *task, u32 a1, u32 a2, u32 a3, int a4, int a5, int a6, int a7,
                         WeatherSpawnFunc spawn) {
    func_ov036_0219a0b0(&task->timer, task, a2, a1, a3, a4, a5, a6, a7, spawn);
}

void func_ov036_02199854(WeatherTask *task, u32 a1, u32 a2, u32 a3, int a4) {
    func_ov036_0219a0d8(&task->timer, a1, a2, a3, a4);
}

BOOL func_ov036_02199868(WeatherTask *task, HeapID heapId) {
    return func_ov036_0219a0e4(&task->timer, heapId);
}

BOOL func_ov036_02199874(WeatherTask *task, HeapID heapId) {
    return func_ov036_0219a1b8(&task->timer, heapId);
}

void func_ov036_02199880(WeatherTask *task, WeatherSpawnFunc spawn, int count, int a3, int a4, HeapID heapId) {
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
                if (!func_ov036_0219a1f0(obj)) {
                    break;
                }
            }
            obj = next;
            next = obj->next;
            if (i >= a3 && i % a3 == 0) {
                rate += a4;
            }
            if (obj == task->objHead) {
                break;
            }
        }
    }
}

void func_ov036_0219990c(WeatherTask *task, u32 a1, u32 a2, u32 a3) {
    if (a3 == 1) {
        FieldFog_SetAlphaMode(task->fog, 0);
        FieldFog_SetAlpha(task->fog, 0x1f);
        FieldFog_SetDepthShift(task->fog, a1);
        FieldFog_SetOffset(task->fog, a2);
        FieldFog_SetDefaultTable(task->fog);
        FieldFog_SetEnabled(task->fog, TRUE);
    }
}

void func_ov036_02199948(WeatherTask *task, u32 a1) {
    if (a1 == 1) {
        FieldFog_SetEnabled(task->fog, FALSE);
    }
}

void func_ov036_02199958(WeatherTask *task, u32 a1, u32 a2, int a3, u32 a4) {
    int half;
    if (a4 == 1) {
        FieldFog_Animate(task->fog, a2, a1, a3);
        return;
    }
    if (a4 == 2) {
        if (a1 == FieldFog_GetDepthShift(task->fog)) {
            FieldFog_Animate(task->fog, a2, a1, a3);
            return;
        }
        half = a3 / 2;
        FieldFog_Animate(task->fog, 0x7fdf, FieldFog_GetDepthShift(task->fog), half);
        func_ov036_0219a2f4(task, a1, a2, half);
    }
}

void func_ov036_021999bc(WeatherTask *task, u32 a1, u32 a2, u32 a3) {
    if (a3 == 1) {
        FieldFog_Animate(task->fog, a1, FieldFog_GetDepthShift(task->fog), a2);
    }
}

BOOL func_ov036_021999dc(WeatherTask *task) {
    if (FieldFog_IsAnimationRunning(task->fog)) {
        return FALSE;
    }
    if (func_ov036_0219a324(task)) {
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov036_02199a00(WeatherTask *task) {
    if (func_ov036_021977a4(task->fogCtrl) == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov036_02199a14(WeatherTask *task) {
    return FieldFogCtrl_CheckArc(task->fogCtrl);
}

BOOL func_ov036_02199a20(WeatherTask *task) {
    return FieldFogCtrl_IsStaticLightExists(task->fogCtrl);
}

u32 func_ov036_02199a2c(WeatherTask *task) {
    return FieldFogCtrl_GetFogParam0(task->fogCtrl);
}

u32 func_ov036_02199a38(WeatherTask *task) {
    return FieldFogCtrl_GetFogParam1(task->fogCtrl);
}

u32 func_ov036_02199a44(WeatherTask *task) {
    return FieldFogCtrl_GetStaticLightIndex(task->fogCtrl);
}

void func_ov036_02199a50(WeatherTask *task) {
    if (task->camera != NULL) {
        VecFx32 target;
        GFL_G3DCameraGetLookatTarget(FieldCamera_GetG3DCamera(task->camera), &target);
        task->lookAt = target;
    }
}

void func_ov036_02199a78(WeatherTask *task, int *x, int *y) {
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
    func_ov036_0219a35c(&NNS_G3dGlb.projMtx, VEC_Distance(&target, &pos), &scaleX, &scaleY);
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

void func_ov036_02199b90(WeatherTask *task, int x, int y) {
    WeatherObj *obj = task->objHead;
    if (obj != NULL) {
        do {
            ClActor *actor = func_ov036_0219a254(obj);
            ClActorPos pos;
            ClActor_GetPos(actor, &pos);
            pos.x -= (s16)x;
            pos.y -= (s16)y;
            ClActor_SetPos(actor, &pos);
            obj = obj->next;
        } while (obj != task->objHead);
    }
}

void func_ov036_02199be8(WeatherTask *task, u32 a1) {
    func_ov036_0219a25c(&task->sound, task->fieldSound, a1);
}

void func_ov036_02199bf8(WeatherTask *task) {
    func_ov036_0219a298(&task->sound, task->fieldSound);
}

void func_ov036_02199c08(WeatherTask *task, u32 sound) {
    if (!func_02005cbc()) {
        if (task->sound.ambience != 0) {
            FieldSnd_PlayAmbience(task->fieldSound, sound);
        } else {
            GFL_SndSEPlay(sound);
        }
    }
}

void func_ov036_02199c30(WeatherTask *task, u32 sound, u32 a2) {
    if (!func_02005cbc()) {
        if (task->sound.ambience != 0) {
            FieldSnd_PlayAmbienceEx(task->fieldSound, sound, a2);
        } else {
            GFL_SndSEPlayEx(sound, a2);
        }
    }
}

void *func_ov036_02199c5c(WeatherObj *obj) {
    return func_ov036_0219a250(obj);
}

ClActor *func_ov036_02199c64(WeatherObj *obj) {
    return func_ov036_0219a254(obj);
}

void func_ov036_02199c6c(WeatherObj *obj, ClActorPos *pos) {
    ClActor_GetSurfacePos(func_ov036_0219a254(obj), pos, 0);
}

void func_ov036_02199c80(WeatherObj *obj, const ClActorPos *pos) {
    ClActorPos wrapped = *pos;
    if (wrapped.x > 0x140) {
        wrapped.x = wrapped.x % 0x140;
    } else if (wrapped.x < -0x40) {
        wrapped.x += 0x140;
    }
    if (wrapped.y > 0x100) {
        wrapped.y = wrapped.y % 0x100;
    } else if (wrapped.y < -0x40) {
        wrapped.y += 0x100;
    }
    ClActor_SetSurfacePos(func_ov036_0219a254(obj), &wrapped, 0);
}

void func_ov036_02199cf0(WeatherObj *obj, const ClActorPos *pos) {
    ClActor_SetSurfacePos(func_ov036_0219a254(obj), pos, 0);
}

static void func_ov036_02199d04(WeatherTask *task) {
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

static BOOL func_ov036_02199d50(WeatherTask *task, WeatherFunc func, HeapID heapId) {
    if (func != NULL) {
        return func(task, task->mode, heapId);
    }
    return TRUE;
}

static u8 func_ov036_02199d68(WeatherTask *task) {
    return WEATHER_STATE_KINDS[task->state];
}

static WeatherObj *func_ov036_02199d78(WeatherTask *task) {
    WeatherObj *obj;
    int i = 0;
    obj = task->objs;
    for (; i < WEATHER_OBJ_COUNT; i++) {
        if (!func_ov036_0219a1f0(obj + i)) {
            return task->objs + i;
        }
    }
    return NULL;
}

static void func_ov036_02199da8(WeatherTask *task, WeatherObj *obj) {
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

static void func_ov036_02199de4(WeatherTask *task, WeatherObj *obj) {
    task->objCount--;
    if (task->objHead == obj) {
        WeatherObj *next = obj->next;
        if (next == obj) {
            next = NULL;
        }
        task->objHead = next;
    }
    obj->next->prev = obj->prev;
    obj->prev->next = obj->next;
    obj->next = NULL;
    obj->prev = NULL;
    ClActor_SetVisible(obj->actor, FALSE);
}

static void func_ov036_02199e24(WeatherTask *task) {
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

static void func_ov036_02199e58(WeatherTask *task, HeapID heapId) {
    func_ov036_02199fa4(&task->res, task->data, heapId);
    func_ov036_02199fd4(&task->res, task->data, heapId);
}

static void func_ov036_02199e78(WeatherTask *task) {
    func_ov036_0219a020(&task->res);
    func_ov036_02199fc0(&task->res);
}

static void func_ov036_02199e8c(WeatherTask *task, HeapID heapId) {
    int i;
    for (i = 0; i < task->data->graphics.bgCount; i++) {
        func_ov036_0219a044(&task->res, i, task->bgs[i], task->data->graphics.bgRes[i], heapId);
    }
}

static void func_ov036_02199ecc(WeatherTask *task) {
    int i;
    for (i = 0; i < task->data->graphics.bgCount; i++) {
        func_ov036_0219a094(&task->res, i, task->bgs[i]);
    }
}

static void func_ov036_02199ef8(WeatherTask *task, HeapID heapId) {
    task->work = GFL_HeapAllocate(heapId, task->data->workSize, TRUE, "weather_task.c", 0x874);
    if (task->data->graphics.hasObjs != 0) {
        int i;
        for (i = 0; i < WEATHER_OBJ_COUNT; i++) {
            func_ov036_0219a200(&task->objs[i], task, &task->res, task->unit, heapId);
        }
    }
}

static void func_ov036_02199f50(WeatherTask *task) {
    if (task->work != NULL) {
        GFL_HeapFree(task->work);
        task->work = NULL;
    }
    if (task->data->graphics.hasObjs != 0) {
        int i;
        for (i = 0; i < WEATHER_OBJ_COUNT; i++) {
            func_ov036_0219a23c(&task->objs[i]);
            func_ov036_0219a1e4(&task->objs[i]);
        }
    }
    func_ov036_02199bf8(task);
}

static void func_ov036_02199f94(WeatherTask *task) {
    task->loaded = 1;
}

static u8 func_ov036_02199f9c(WeatherTask *task) {
    return task->loaded;
}

static void func_ov036_02199fa4(WeatherRes *res, const WeatherData *data, HeapID heapId) {
    if (data->graphics.hasObjs != 0 || data->graphics.bgCount != 0) {
        res->arc = GFL_ArcSysCreateFileHandle(data->graphics.arcId, heapId);
    }
}

static void func_ov036_02199fc0(WeatherRes *res) {
    if (res->arc != NULL) {
        GFL_ArcToolFree(res->arc);
        res->arc = NULL;
    }
}

static void func_ov036_02199fd4(WeatherRes *res, const WeatherData *data, HeapID heapId) {
    if (data->graphics.hasObjs != 0) {
        res->chars = ClActSys_LoadChars(res->arc, data->graphics.charsFileId, FALSE, 0, heapId);
        res->palette = ClActSys_LoadPlttRange(res->arc, data->graphics.paletteFileId, 0, 0, 0, 1, heapId);
        res->cells = ClActSys_LoadCellAnims(res->arc, data->graphics.cellFileId, data->graphics.cellAnimFileId, heapId);
        res->loaded = 1;
    }
}

static void func_ov036_0219a020(WeatherRes *res) {
    if (res->loaded != 0) {
        ClActSys_FreeChars(res->chars);
        ClActSys_FreePltt(res->palette);
        ClActSys_FreeCellAnims(res->cells);
        res->loaded = 0;
    }
}

// The layer's texture, from the weather's data, at full alpha
static void func_ov036_0219a044(WeatherRes *res, int index, Field3DBg *bg, const void *config, HeapID heapId) {
    static Field3DBgRes bgRes;

    MI_CpuCopy8(config, &bgRes, sizeof(Field3DBgRes));
    if (bgRes.alpha != 0x1f) {
        bgRes.alpha = 0x1f;
    }
    func_ov036_02198cb4(bg, res->arc, &bgRes, heapId);
    func_ov036_02198cf0(bg, 0);
    func_ov036_02198cf4(bg, 0x20);
    res->bgActive[index] = 1;
}

static void func_ov036_0219a094(WeatherRes *res, int index, Field3DBg *bg) {
    if (res->bgActive[index] != 0) {
        func_ov036_02198c9c(bg);
        res->bgActive[index] = 0;
    }
}

static void func_ov036_0219a0b0(WeatherTimer *timer, WeatherTask *owner, int interval, int value, int valueTarget,
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

static void func_ov036_0219a0d8(WeatherTimer *timer, int valueTarget, int intervalTarget, int intervalStep,
                                int valueStep) {
    timer->valueTarget = valueTarget;
    timer->intervalTarget = intervalTarget;
    timer->intervalStep = intervalStep;
    timer->valueStep = valueStep;
}

static BOOL func_ov036_0219a0e4(WeatherTimer *timer, HeapID heapId) {
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

static BOOL func_ov036_0219a1b8(WeatherTimer *timer, HeapID heapId) {
    timer->counter--;
    if (timer->counter <= 0) {
        timer->func(timer->owner, timer->value, heapId);
        timer->counter = timer->interval;
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0219a1e4(WeatherObj *obj) {
    MI_CpuFill8(obj, 0, sizeof(WeatherObj));
}

static BOOL func_ov036_0219a1f0(WeatherObj *obj) {
    if (obj->prev != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0219a200(WeatherObj *obj, WeatherTask *task, WeatherRes *res, ClActUnit *unit, HeapID heapId) {
    obj->task = task;
    obj->actor = ClActor_Create(unit, res->chars, res->palette, res->cells, &WEATHER_OBJ_SETUP, 0, heapId);
    ClActor_SetVisible(obj->actor, FALSE);
    func_0204c5c8(obj->actor, FALSE);
}

static void func_ov036_0219a23c(WeatherObj *obj) {
    if (obj->actor != NULL) {
        ClActor_Delete(obj->actor);
        obj->actor = NULL;
    }
}

static void *func_ov036_0219a250(WeatherObj *obj) {
    return obj->work;
}

static ClActor *func_ov036_0219a254(WeatherObj *obj) {
    return obj->actor;
}

static void func_ov036_0219a258(WeatherSound *sound, u32 ambience) {
    sound->ambience = ambience;
}

static void func_ov036_0219a25c(WeatherSound *sound, FieldSound *fieldSound, u32 se) {
    func_ov036_0219a298(sound, fieldSound);
    if (!func_02005cbc()) {
        sound->state = 1;
        sound->se = se;
        if (sound->ambience != 0) {
            FieldSnd_PlayAmbience(fieldSound, sound->se);
        } else {
            GFL_SndSEPlay(sound->se);
        }
    } else {
        sound->state = 2;
        sound->se = se;
    }
}

static void func_ov036_0219a298(WeatherSound *sound, FieldSound *fieldSound) {
    if (sound->state == 1) {
        sound->state = 0;
        if (sound->ambience != 0) {
            FieldSnd_StopAmbience(fieldSound, sound->se);
        } else {
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(sound->se));
        }
    }
}

static void func_ov036_0219a2c4(WeatherSound *sound, FieldSound *fieldSound) {
    if (sound->state == 2 && !func_02005cbc()) {
        sound->state = 1;
        if (sound->ambience != 0) {
            FieldSnd_PlayAmbience(fieldSound, sound->se);
        } else {
            GFL_SndSEPlay(sound->se);
        }
    }
}

static void func_ov036_0219a2f4(WeatherTask *task, u32 depthShift, u32 color, int duration) {
    task->fogPending = 1;
    task->fogDepthShift = depthShift;
    task->fogColor = color;
    task->fogDuration = duration / 2;
}

static void func_ov036_0219a318(WeatherTask *task) {
    task->fogPending = 0;
}

static BOOL func_ov036_0219a324(WeatherTask *task) {
    return task->fogPending;
}

static void func_ov036_0219a330(WeatherTask *task) {
    FieldFog_AnimateEx(task->fog, 0x7fdf, task->fogColor, task->fogDepthShift, task->fogDepthShift, task->fogDuration);
}

static void func_ov036_0219a35c(const MtxFx44 *proj, fx32 distance, fx32 *x, fx32 *y) {
    *y = FX_MUL(FX_Div(distance, proj->m[1][1]), 2 * FX32_ONE);
    *x = FX_MUL(FX_Div(distance, proj->m[0][0]), 2 * FX32_ONE);
}
