// The field's weather system (weather.c, after the file name that its allocation passes): two weather tasks of
// weather_task.c and their two background layers, the overlay of the weather that plays, and the change from one
// weather to the next, at once or over each other. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0); the layout is ours
#include "types.h"
#include "field/field_3dbg.h"
#include "field/field_environment.h"
#include "field/field_fog.h"
#include "field/field_weather.h"
#include "field/weather_particles.h"
#include "field/weather_rain.h"
#include "field/weather_sandstorm.h"
#include "field/weather_tint.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/tcb.h"

typedef struct {
    Field3DBg *layers[2];
    u16 transition;
    u16 state;
    u16 current;
    u16 next;
    BOOL fadeIn;
    u16 pending;
    ClActUnit *actUnit;
    ClActRenderer *renderer;
    WeatherTask *tasks[2];
    s32 overlayId;
    TCB *tcb;
} FieldWeather;

typedef struct WeatherOverlay {
    WeatherData *data;
    u32 overlayId;
} WeatherOverlay;

typedef void (*WeatherTransitionFunc)(FieldWeather *weather, HeapID heapId);

static void func_ov036_02199224(FieldWeather *weather, u32 overlayId);
static void func_ov036_02199230(FieldWeather *weather);
static void func_ov036_02199248(TCB *tcb, void *data);
static BOOL func_ov036_02199260(FieldWeather *weather, u32 weatherId);
static void func_ov036_02199294(FieldWeather *weather, HeapID heapId);
static void func_ov036_02199330(FieldWeather *weather, HeapID heapId);
static u32 func_ov036_021993a4(u32 current, u32 next);

static const Field3DBgInit WEATHER_BG_SETUPS[2] = {
    { 0x100, 0xc0, FX32_CONST(80), 0 },
    { 0x100, 0xc0, FX32_CONST(88), 0 },
};

static const ClActSurfaceSetup WEATHER_SURFACE_SETUP = { 0, 0, 0x100, 0xc0, 0, 1 };

static const WeatherOverlay WEATHER_OVERLAYS[15] = {
    { &SUNNY_WEATHER, 0x4a },        { &FLAKE_WEATHER, 0x4b },      { &RAIN_WEATHER, 0x4c },
    { &SANDSTORM_WEATHER, 0x4d },    { &BLIZZARD_WEATHER, 0x4b },   { &FLUTTER_WEATHER, 0x4b },
    { &THUNDERSTORM_WEATHER, 0x4c }, { &GUSTY_RAIN_WEATHER, 0x4c }, { &SPECK_WEATHER, 0x4b },
    { &TINT_WEATHER_9, 0x4e },       { &TINT_WEATHER_10, 0x4e },    { &TINT_WEATHER_11, 0x4e },
    { &SANDSTORM_WEATHER_2, 0x4d },  { &TINT_WEATHER_13, 0x4e },    { &TINT_WEATHER_14, 0x4e },
};

static WeatherTransitionFunc WEATHER_TRANSITION_FUNCS[2] = {
    func_ov036_02199294,
    func_ov036_02199330,
};

void *func_ov036_02199004(FieldCamera *camera, void *light, FieldFog *fog, void *fogCtrl, FieldSound *sound, u32 season,
                          HeapID heapId) {
    FieldWeather *weather;
    ClActRenderer *renderer;
    s32 i;

    weather = GFL_HeapAllocate(heapId, sizeof(FieldWeather), TRUE, "weather.c", 0x158);
    for (i = 0; i < 2; i++) {
        weather->layers[i] = func_ov036_02198bc0(&WEATHER_BG_SETUPS[i], heapId);
    }
    weather->transition = 0;
    weather->state = 0;
    weather->current = 0xffff;
    weather->next = 0xffff;
    weather->pending = 0xffff;
    weather->overlayId = -1;
    weather->actUnit = ClActUnit_Create(0x40, 8, heapId);
    renderer = ClActRenderer_Create(&WEATHER_SURFACE_SETUP, 1, heapId);
    weather->renderer = renderer;
    ClActUnit_SetRenderer(weather->actUnit, renderer);
    func_0204bff0(weather->actUnit, TRUE);
    func_0204c004(weather->actUnit, TRUE);
    for (i = 0; i < 2; i++) {
        weather->tasks[i] =
            func_ov036_021993e0(weather->actUnit, camera, light, fog, fogCtrl, weather->layers, sound, season, heapId);
    }
    weather->tcb = GFL_VBlankTCBAdd(func_ov036_02199248, weather, 0x40);
    return weather;
}

void func_ov036_021990d8(void *weatherPtr) {
    FieldWeather *weather = weatherPtr;
    s32 i;

    GFL_TCBRemove(weather->tcb);
    for (i = 0; i < 2; i++) {
        func_ov036_02199424(weather->tasks[i]);
    }
    func_ov036_02199230(weather);
    ClActRenderer_Delete(weather->renderer);
    ClActUnit_Delete(weather->actUnit);
    for (i = 0; i < 2; i++) {
        func_ov036_02198bf0(weather->layers[i]);
    }
    GFL_HeapFree(weather);
}

void FieldWeather_Update(void *weatherPtr, HeapID heapId) {
    FieldWeather *weather = weatherPtr;
    s32 i;

    WEATHER_TRANSITION_FUNCS[weather->transition](weather, heapId);
    if (weather->state == 0 && weather->pending != 0xffff) {
        func_ov036_02199208(weather, weather->pending);
        weather->pending = 0xffff;
    }
    for (i = 0; i < 2; i++) {
        func_ov036_02199434(weather->tasks[i], heapId);
    }
}

void FieldWeather_Draw(void *weatherPtr) {
    FieldWeather *weather = weatherPtr;
    s32 i;

    if (weather) {
        for (i = 0; i < 2; i++) {
            func_ov036_02198c10(weather->layers[i]);
        }
    }
}

BOOL func_ov036_0219917c(void *weatherPtr) {
    FieldWeather *weather = weatherPtr;
    s32 i;

    if (weather) {
        for (i = 0; i < 2; i++) {
            if (func_ov036_021997dc(weather->tasks[i]) == TRUE) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void SetWeatherInit(void *weatherPtr, u16 weatherId, HeapID heapId) {
    FieldWeather *weather = weatherPtr;
    s32 i;

    if (weatherId == weather->current && weatherId != 0) {
        return;
    }
    for (i = 0; i < 2; i++) {
        func_ov036_021996f4(weather->tasks[i]);
    }
    func_ov036_02199230(weather);
    weather->state = 0;
    func_ov036_02199224(weather, WEATHER_OVERLAYS[weatherId].overlayId);
    func_ov036_02199618(weather->tasks[0], WEATHER_OVERLAYS[weatherId].data, 0, 0, 1, 1, heapId);
    weather->current = weatherId;
    weather->next = 0xffff;
}

void func_ov036_02199208(void *weather, u32 weatherId) {
    func_ov036_02199260(weather, weatherId);
}

void func_ov036_02199210(void *weatherPtr, u32 weatherId) {
    FieldWeather *weather = weatherPtr;

    func_ov036_02199260(weather, weatherId);
    weather->fadeIn = FALSE;
}

u16 func_ov036_02199220(void *weatherPtr) {
    FieldWeather *weather = weatherPtr;

    return weather->current;
}

static void func_ov036_02199224(FieldWeather *weather, u32 overlayId) {
    weather->overlayId = overlayId;
    GFL_OvlLoad(overlayId);
}

static void func_ov036_02199230(FieldWeather *weather) {
    if (weather->overlayId != -1) {
        GFL_OvlUnload(weather->overlayId);
        weather->overlayId = -1;
    }
}

static void func_ov036_02199248(TCB *tcb, void *data) {
    FieldWeather *weather = data;
    s32 i;

    for (i = 0; i < 2; i++) {
        func_ov036_02198c8c(weather->layers[i]);
    }
}

static BOOL func_ov036_02199260(FieldWeather *weather, u32 weatherId) {
    if (weather->state != 0) {
        weather->pending = weatherId;
        return FALSE;
    }
    if (weather->current == weatherId && weatherId != 0) {
        return FALSE;
    }
    weather->next = weatherId;
    weather->fadeIn = TRUE;
    weather->transition = func_ov036_021993a4(weather->current, weather->next);
    weather->state = 1;
    return TRUE;
}

static void func_ov036_02199294(FieldWeather *weather, HeapID heapId) {
    switch (weather->state) {
    case 1:
        func_ov036_0219967c(weather->tasks[0], 1, 1);
        weather->state = 2;
        break;
    case 2:
        if (!func_ov036_021997dc(weather->tasks[0])) {
            func_ov036_02199230(weather);
            weather->state = 3;
        }
        break;
    case 3:
        func_ov036_02199224(weather, WEATHER_OVERLAYS[weather->next].overlayId);
        weather->state = 4;
        break;
    case 4:
        func_ov036_02199618(weather->tasks[0], WEATHER_OVERLAYS[weather->next].data, 1, 1, 1, weather->fadeIn, heapId);
        weather->current = weather->next;
        weather->fadeIn = TRUE;
        weather->next = 0xffff;
        weather->state = 0;
        break;
    }
}

static void func_ov036_02199330(FieldWeather *weather, HeapID heapId) {
    WeatherTask *task;

    switch (weather->state) {
    case 0:
        break;
    case 1:
        func_ov036_02199618(weather->tasks[1], WEATHER_OVERLAYS[weather->next].data, 1, 1, 2, weather->fadeIn, heapId);
        func_ov036_0219967c(weather->tasks[0], 1, 2);
        weather->state = 2;
        break;
    case 2:
        if (!func_ov036_021997dc(weather->tasks[0])) {
            task = weather->tasks[0];
            weather->tasks[0] = weather->tasks[1];
            weather->current = weather->next;
            weather->tasks[1] = task;
            weather->next = 0xffff;
            weather->fadeIn = TRUE;
            weather->state = 0;
        }
        break;
    }
}

static u32 func_ov036_021993a4(u32 current, u32 next) {
    u32 result = FALSE;

    switch (current) {
    case 1:
        if ((u16)(next + 0xfffc) <= 1) {
            result = TRUE;
        }
        break;
    case 4:
        if (next == 1 || next == 5) {
            result = TRUE;
        }
        break;
    case 5:
        if (next == 4 || next == 1) {
            result = TRUE;
        }
        break;
    }
    return result;
}
