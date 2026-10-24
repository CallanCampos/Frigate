#include "types.h"
#include "field/field_weather.h"
#include "field/weather_sandstorm.h"
#include "gfl/clact.h"
#include "gfl/random.h"

// The sandstorm weathers. The ROM doesn't name the file; the name is a guess. Grains of sand blow in from the left at
// a speed that changes with a 320-frame cycle, over a background that scrolls with them and the camera

typedef struct {
    // Frames until the fade starts, when appearing or leaving
    int timer;
    // Counts the calls to Sandstorm_AddGrains, 0 to 319: every 40 the wind moves on to the next of sWindSpeedX and
    // sWindSpeedY
    int cycle;
    // The background's scroll, 0 to 255
    int scroll;
    int windSpeed;
    // The weather asks for 0x28 bytes of work but uses only these
    int unused[6];
} SandstormWork;

// A grain of sand
typedef struct {
    int frame;
    int lifetime;
    int speedY;
    BOOL done;
    int speedX;
    // Added to speedX every 5 frames
    int accelX;
} SandGrain;

static BOOL Sandstorm_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Sandstorm_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Sandstorm_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Sandstorm_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Sandstorm_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Sandstorm_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Sandstorm_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void SandGrain_Move(WeatherObj *obj);
static void Sandstorm_AddGrains(WeatherTask *task, int count, HeapID heapId);
static void Sandstorm_ScrollBg(WeatherTask *task, SandstormWork *work);

static const int sWindSpeedX[8] = { 6, 10, 10, 6, 10, 12, 20, 12 };
static const int sWindSpeedY[8] = { 4, 4, 4, 8, 8, 4, 4, 4 };

WeatherData SANDSTORM_WEATHER = {
    { 0x37, 1, 0, 0x22, 0x23, 0x21, 0x20, 0x2b, 0x303, 3, 0x102 },
    sizeof(SandstormWork),
    {
        Sandstorm_Init,
        Sandstorm_FadeIn,
        Sandstorm_InitNoFade,
        Sandstorm_Main,
        Sandstorm_InitFadeOut,
        Sandstorm_FadeOut,
        Sandstorm_Exit,
    },
    SandGrain_Move,
};

WeatherData SANDSTORM_WEATHER_2 = {
    { 0x37, 1, 1, 0x22, 0x23, 0x21, 0x20, 0x2b, 0x303, 3, 0x102 },
    sizeof(SandstormWork),
    {
        Sandstorm_Init,
        Sandstorm_FadeIn,
        Sandstorm_InitNoFade,
        Sandstorm_Main,
        Sandstorm_InitFadeOut,
        Sandstorm_FadeOut,
        Sandstorm_Exit,
    },
    SandGrain_Move,
};

static BOOL Sandstorm_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 1, 15, 1, 2, -2, 4, 1, Sandstorm_AddGrains);
    WeatherTask_InitFog(task, 8, 0x7fdf, arg);
    work->timer = 1;
    work->cycle = 0;
    work->scroll = 0;
    work->windSpeed = 0;
    WeatherTask_SaveCameraPos(task);
    WeatherTask_PlaySe(task, 0x6e3);
    return TRUE;
}

static BOOL Sandstorm_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL done = WeatherTask_StepSpawner(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFog(task, 8, 0x7f5e, 0x5a, arg);
            WeatherTask_FadeLight(task, 0x3d, FieldLightStatus_GetWeatherLight(3));
        }
    } else if (WeatherTask_IsFogIdle(task) && done) {
        WeatherTask_SetBillboardVisible(task, 1, 0);
        return TRUE;
    }
    Sandstorm_ScrollBg(task, work);
    return FALSE;
}

static BOOL Sandstorm_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_SetupSpawner(task, 1, 2, 1, 2, -2, 4, 1, Sandstorm_AddGrains);
    WeatherTask_InitFog(task, 8, 0x7f5e, arg);
    WeatherTask_PrewarmParticles(task, Sandstorm_AddGrains, 24, 2, 2, heapId);
    WeatherTask_SetBillboardVisible(task, 1, 0);
    WeatherTask_SetLight(task, 0x3d, FieldLightStatus_GetWeatherLight(3));
    return TRUE;
}

static BOOL Sandstorm_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = WeatherTask_GetWorkBuffer(task);

    if (WeatherTask_StepSpawnTimer(task, heapId)) {
        work->windSpeed = sWindSpeedX[work->cycle / 40];
        // Never true, since sWindSpeedX holds no negative speed
        if (work->windSpeed <= -6) {
            Sandstorm_AddGrains(task, 1, heapId);
        }
    }
    Sandstorm_ScrollBg(task, work);
    return FALSE;
}

static BOOL Sandstorm_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_RetargetSpawner(task, 0, 15, 2, -1);
    work->timer = 31;
    WeatherTask_SetBillboardVisible(task, 0, 0);
    WeatherTask_StopSe(task);
    return TRUE;
}

static BOOL Sandstorm_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL done = WeatherTask_StepSpawner(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFogOffset(task, 0x7fdf, 0x5a, arg);
        }
    } else if ((arg ? WeatherTask_IsFogIdle(task) : TRUE) && done && !WeatherTask_GetParticleCount(task)) {
        return TRUE;
    }
    Sandstorm_ScrollBg(task, work);
    return FALSE;
}

static BOOL Sandstorm_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_DisableFog(task, arg);
    WeatherTask_ResetLight(task, arg);
    return TRUE;
}

static void SandGrain_Move(WeatherObj *obj) {
    SandGrain *grain = WeatherTask_GetParticleWork(obj);
    ClActorPos pos;

    WeatherTask_GetParticleActor(obj);
    func_ov036_02199c6c(obj, &pos);
    if (!grain->done) {
        pos.x += (s16)grain->speedX;
        pos.y += (s16)grain->speedY;
        if (grain->frame % 5 == 0) {
            grain->speedX += grain->accelX;
        }
        if (grain->frame++ > grain->lifetime) {
            grain->done = TRUE;
        }
        WeatherTask_SetParticlePosWrapped(obj, &pos);
    } else {
        WeatherTask_RemoveParticle(obj);
    }
}

static void Sandstorm_AddGrains(WeatherTask *task, int count, HeapID heapId) {
    SandstormWork *work = WeatherTask_GetWorkBuffer(task);
    int index;
    int i;

    work->cycle = (work->cycle + 1) % 320;
    index = work->cycle / 40;
    for (i = 0; i < count; i++) {
        WeatherObj *obj;
        SandGrain *grain;
        ClActor *actor;
        int size;
        ClActorPos pos;

        obj = WeatherTask_AddParticle(task, heapId);
        if (obj == NULL) {
            return;
        }
        grain = WeatherTask_GetParticleWork(obj);
        actor = WeatherTask_GetParticleActor(obj);
        grain->frame = 0;
        grain->lifetime = GFL_RandomLC(20) + 15;
        size = 3 - (grain->lifetime - 15) / 6;
        grain->speedY = sWindSpeedY[index] * (size + 1);
        grain->speedX = sWindSpeedX[index] * (size + 1);
        grain->done = FALSE;
        grain->accelX = sWindSpeedX[index];
        if (GFL_RandomLC(1000) == 777) {
            size = 4;
            grain->speedY += grain->speedY / 2;
        }
        ClActor_SetAnimFrame(actor, size);
        pos.x = -6 - GFL_RandomLC(24);
        pos.y = GFL_RandomLC(192) - 64;
        WeatherTask_SetParticlePosWrapped(obj, &pos);
    }
}

static void Sandstorm_ScrollBg(WeatherTask *task, SandstormWork *work) {
    int x;
    int y;

    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    work->scroll = (work->scroll + 6) % 256;
    WeatherTask_SetBillboardTexOffsetX(task, -work->scroll * 2 - x, 0);
    WeatherTask_SetBillboardTexOffsetY(task, -work->scroll * 2 + y, 0);
}
