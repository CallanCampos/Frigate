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
    SandstormWork *work = func_ov036_021997e4(task);

    func_ov036_02199824(task, 1, 15, 1, 2, -2, 4, 1, Sandstorm_AddGrains);
    func_ov036_0219990c(task, 8, 0x7fdf, arg);
    work->timer = 1;
    work->cycle = 0;
    work->scroll = 0;
    work->windSpeed = 0;
    func_ov036_02199a50(task);
    func_ov036_02199be8(task, 0x6e3);
    return TRUE;
}

static BOOL Sandstorm_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = func_ov036_021997e4(task);
    BOOL done = func_ov036_02199868(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            func_ov036_02199958(task, 8, 0x7f5e, 0x5a, arg);
            func_ov036_02199734(task, 0x3d, FieldLightStatus_GetWeatherLight(3));
        }
    } else if (func_ov036_021999dc(task) && done) {
        func_ov036_0219978c(task, 1, 0);
        return TRUE;
    }
    Sandstorm_ScrollBg(task, work);
    return FALSE;
}

static BOOL Sandstorm_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199824(task, 1, 2, 1, 2, -2, 4, 1, Sandstorm_AddGrains);
    func_ov036_0219990c(task, 8, 0x7f5e, arg);
    func_ov036_02199880(task, Sandstorm_AddGrains, 24, 2, 2, heapId);
    func_ov036_0219978c(task, 1, 0);
    func_ov036_02199740(task, 0x3d, FieldLightStatus_GetWeatherLight(3));
    return TRUE;
}

static BOOL Sandstorm_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = func_ov036_021997e4(task);

    if (func_ov036_02199874(task, heapId)) {
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
    SandstormWork *work = func_ov036_021997e4(task);

    func_ov036_02199854(task, 0, 15, 2, -1);
    work->timer = 31;
    func_ov036_0219978c(task, 0, 0);
    func_ov036_02199bf8(task);
    return TRUE;
}

static BOOL Sandstorm_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    SandstormWork *work = func_ov036_021997e4(task);
    BOOL done = func_ov036_02199868(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            func_ov036_021999bc(task, 0x7fdf, 0x5a, arg);
        }
    } else if ((arg ? func_ov036_021999dc(task) : TRUE) && done && !func_ov036_021997e8(task)) {
        return TRUE;
    }
    Sandstorm_ScrollBg(task, work);
    return FALSE;
}

static BOOL Sandstorm_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199948(task, arg);
    func_ov036_0219974c(task, arg);
    return TRUE;
}

static void SandGrain_Move(WeatherObj *obj) {
    SandGrain *grain = func_ov036_02199c5c(obj);
    ClActorPos pos;

    func_ov036_02199c64(obj);
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
        func_ov036_02199c80(obj, &pos);
    } else {
        func_ov036_0219980c(obj);
    }
}

static void Sandstorm_AddGrains(WeatherTask *task, int count, HeapID heapId) {
    SandstormWork *work = func_ov036_021997e4(task);
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

        obj = func_ov036_021997f4(task, heapId);
        if (obj == NULL) {
            return;
        }
        grain = func_ov036_02199c5c(obj);
        actor = func_ov036_02199c64(obj);
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
        func_0204c504(actor, size);
        pos.x = -6 - GFL_RandomLC(24);
        pos.y = GFL_RandomLC(192) - 64;
        func_ov036_02199c80(obj, &pos);
    }
}

static void Sandstorm_ScrollBg(WeatherTask *task, SandstormWork *work) {
    int x;
    int y;

    func_ov036_02199a78(task, &x, &y);
    func_ov036_02199b90(task, x, y);
    work->scroll = (work->scroll + 6) % 256;
    func_ov036_021997bc(task, -work->scroll * 2 - x, 0);
    func_ov036_021997cc(task, -work->scroll * 2 + y, 0);
}
