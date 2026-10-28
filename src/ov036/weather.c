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
#include "system/zone_weather.h"

struct FieldWeather {
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
};

// FieldWeather's transition: end the weather, then start the next, or start the next over it as it ends; the index of
// WEATHER_TRANSITION_FUNCS
enum {
    WEATHER_TRANSITION_SEQUENTIAL,
    WEATHER_TRANSITION_CROSSFADE,
};

// FieldWeather's state through a change of weather: a crossfade goes from WEATHER_STATE_START to WEATHER_STATE_WAIT_END
// and back to WEATHER_STATE_IDLE, a sequential change through all of them
enum {
    WEATHER_STATE_IDLE,
    WEATHER_STATE_START,
    WEATHER_STATE_WAIT_END,
    WEATHER_STATE_LOAD_OVERLAY,
    WEATHER_STATE_START_NEXT,
};

typedef struct WeatherOverlay {
    WeatherData *data;
    u32 overlayId;
} WeatherOverlay;

typedef void (*WeatherTransitionFunc)(FieldWeather *weather, HeapID heapId);

static void FieldWeather_LoadOverlay(FieldWeather *weather, u32 overlayId);
static void FieldWeather_UnloadOverlay(FieldWeather *weather);
static void FieldWeather_ReleaseBillboardVramTask(TCB *tcb, void *data);
static BOOL FieldWeather_StartTransition(FieldWeather *weather, u32 weatherId);
static void FieldWeather_UpdateSequential(FieldWeather *weather, HeapID heapId);
static void FieldWeather_UpdateCrossfade(FieldWeather *weather, HeapID heapId);
static u32 FieldWeather_GetTransitionMode(u16 current, u16 next);

static const Field3DBgInit WEATHER_BG_SETUPS[2] = {
    { 0x100, 0xc0, FX32_CONST(80), 0 },
    { 0x100, 0xc0, FX32_CONST(88), 0 },
};

static const ClActSurfaceSetup WEATHER_SURFACE_SETUP = { 0, 0, 0x100, 0xc0, 0, 1 };

static const WeatherOverlay WEATHER_OVERLAYS[WEATHER_MAX] = {
    { &SUNNY_WEATHER, OVERLAY_ID(74) },        { &FLAKE_WEATHER, OVERLAY_ID(75) },
    { &RAIN_WEATHER, OVERLAY_ID(76) },         { &SANDSTORM_WEATHER, OVERLAY_ID(77) },
    { &BLIZZARD_WEATHER, OVERLAY_ID(75) },     { &FLUTTER_WEATHER, OVERLAY_ID(75) },
    { &THUNDERSTORM_WEATHER, OVERLAY_ID(76) }, { &GUSTY_RAIN_WEATHER, OVERLAY_ID(76) },
    { &SPECK_WEATHER, OVERLAY_ID(75) },        { &TINT_WEATHER_9, OVERLAY_ID(78) },
    { &TINT_WEATHER_10, OVERLAY_ID(78) },      { &TINT_WEATHER_11, OVERLAY_ID(78) },
    { &SANDSTORM_WEATHER_2, OVERLAY_ID(77) },  { &TINT_WEATHER_13, OVERLAY_ID(78) },
    { &TINT_WEATHER_14, OVERLAY_ID(78) },
};

static WeatherTransitionFunc WEATHER_TRANSITION_FUNCS[2] = {
    FieldWeather_UpdateSequential,
    FieldWeather_UpdateCrossfade,
};

FieldWeather *FieldWeather_Create(FieldCamera *camera, FieldLight *light, FieldFog *fog, FieldFogCtrl *fogCtrl,
                                  FieldSound *sound, u16 season, HeapID heapId) {
    FieldWeather *weather;
    ClActRenderer *renderer;
    s32 i;

    weather = GFL_HeapAllocate(heapId, sizeof(FieldWeather), TRUE, "weather.c", 0x158);
    for (i = 0; i < 2; i++) {
        weather->layers[i] = Field3DBG_CreateBillboard(&WEATHER_BG_SETUPS[i], heapId);
    }
    weather->transition = WEATHER_TRANSITION_SEQUENTIAL;
    weather->state = WEATHER_STATE_IDLE;
    weather->current = WEATHER_NONE;
    weather->next = WEATHER_NONE;
    weather->pending = WEATHER_NONE;
    weather->overlayId = -1;
    weather->actUnit = ClActUnit_Create(0x40, 8, heapId);
    renderer = ClActRenderer_Create(&WEATHER_SURFACE_SETUP, 1, heapId);
    weather->renderer = renderer;
    ClActUnit_SetRenderer(weather->actUnit, renderer);
    ClActUnit_SetVisibleAnimOnly(weather->actUnit, TRUE);
    ClActUnit_SetSharedParams(weather->actUnit, TRUE);
    for (i = 0; i < 2; i++) {
        weather->tasks[i] =
            WeatherTask_Create(weather->actUnit, camera, light, fog, fogCtrl, weather->layers, sound, season, heapId);
    }
    weather->tcb = GFL_VBlankTCBAdd(FieldWeather_ReleaseBillboardVramTask, weather, 0x40);
    return weather;
}

void FieldWeather_Delete(FieldWeather *weather) {
    s32 i;

    GFL_TCBRemove(weather->tcb);
    for (i = 0; i < 2; i++) {
        WeatherTask_Delete(weather->tasks[i]);
    }
    FieldWeather_UnloadOverlay(weather);
    ClActRenderer_Delete(weather->renderer);
    ClActUnit_Delete(weather->actUnit);
    for (i = 0; i < 2; i++) {
        Field3DBG_DeleteBillboard(weather->layers[i]);
    }
    GFL_HeapFree(weather);
}

void FieldWeather_Update(FieldWeather *weather, HeapID heapId) {
    s32 i;

    WEATHER_TRANSITION_FUNCS[weather->transition](weather, heapId);
    if (weather->state == WEATHER_STATE_IDLE && weather->pending != WEATHER_NONE) {
        FieldWeather_RequestChange(weather, weather->pending);
        weather->pending = WEATHER_NONE;
    }
    for (i = 0; i < 2; i++) {
        WeatherTask_Update(weather->tasks[i], heapId);
    }
}

void FieldWeather_Draw(FieldWeather *weather) {
    s32 i;

    if (weather) {
        for (i = 0; i < 2; i++) {
            Field3DBG_DrawBillboard(weather->layers[i]);
        }
    }
}

BOOL FieldWeather_IsTaskActive(FieldWeather *weather) {
    s32 i;

    if (weather) {
        for (i = 0; i < 2; i++) {
            if (WeatherTask_GetPhase(weather->tasks[i]) == TRUE) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void SetWeatherInit(FieldWeather *weather, u16 weatherId, HeapID heapId) {
    s32 i;

    if (weatherId == weather->current && weatherId != WEATHER_SUNNY) {
        return;
    }
    for (i = 0; i < 2; i++) {
        WeatherTask_ForceEnd(weather->tasks[i]);
    }
    FieldWeather_UnloadOverlay(weather);
    weather->state = WEATHER_STATE_IDLE;
    FieldWeather_LoadOverlay(weather, WEATHER_OVERLAYS[weatherId].overlayId);
    WeatherTask_Start(weather->tasks[0], WEATHER_OVERLAYS[weatherId].data, FALSE, FALSE, WEATHER_MODE_ALONE, TRUE,
                      heapId);
    weather->current = weatherId;
    weather->next = WEATHER_NONE;
}

void FieldWeather_RequestChange(FieldWeather *weather, u32 weatherId) {
    FieldWeather_StartTransition(weather, weatherId);
}

void FieldWeather_RequestChangeNoFade(FieldWeather *weather, u32 weatherId) {
    FieldWeather_StartTransition(weather, weatherId);
    weather->fadeIn = FALSE;
}

u16 FieldWeather_GetWeatherId(FieldWeather *weather) {
    return weather->current;
}

static void FieldWeather_LoadOverlay(FieldWeather *weather, u32 overlayId) {
    weather->overlayId = overlayId;
    GFL_OvlLoad(overlayId);
}

static void FieldWeather_UnloadOverlay(FieldWeather *weather) {
    if (weather->overlayId != -1) {
        GFL_OvlUnload(weather->overlayId);
        weather->overlayId = -1;
    }
}

static void FieldWeather_ReleaseBillboardVramTask(TCB *tcb, void *data) {
    FieldWeather *weather = data;
    s32 i;

    for (i = 0; i < 2; i++) {
        Field3DBG_ReleaseBillboardTextureVram(weather->layers[i]);
    }
}

static BOOL FieldWeather_StartTransition(FieldWeather *weather, u32 weatherId) {
    if (weather->state != WEATHER_STATE_IDLE) {
        weather->pending = weatherId;
        return FALSE;
    }
    if (weather->current == weatherId && weatherId != WEATHER_SUNNY) {
        return FALSE;
    }
    weather->next = weatherId;
    weather->fadeIn = TRUE;
    weather->transition = FieldWeather_GetTransitionMode(weather->current, weather->next);
    weather->state = WEATHER_STATE_START;
    return TRUE;
}

static void FieldWeather_UpdateSequential(FieldWeather *weather, HeapID heapId) {
    switch (weather->state) {
    case WEATHER_STATE_START:
        WeatherTask_RequestEnd(weather->tasks[0], TRUE, WEATHER_MODE_ALONE);
        weather->state = WEATHER_STATE_WAIT_END;
        break;
    case WEATHER_STATE_WAIT_END:
        if (!WeatherTask_GetPhase(weather->tasks[0])) {
            FieldWeather_UnloadOverlay(weather);
            weather->state = WEATHER_STATE_LOAD_OVERLAY;
        }
        break;
    case WEATHER_STATE_LOAD_OVERLAY:
        FieldWeather_LoadOverlay(weather, WEATHER_OVERLAYS[weather->next].overlayId);
        weather->state = WEATHER_STATE_START_NEXT;
        break;
    case WEATHER_STATE_START_NEXT:
        WeatherTask_Start(weather->tasks[0], WEATHER_OVERLAYS[weather->next].data, TRUE, TRUE, WEATHER_MODE_ALONE,
                          weather->fadeIn, heapId);
        weather->current = weather->next;
        weather->fadeIn = TRUE;
        weather->next = WEATHER_NONE;
        weather->state = WEATHER_STATE_IDLE;
        break;
    }
}

static void FieldWeather_UpdateCrossfade(FieldWeather *weather, HeapID heapId) {
    WeatherTask *task;

    switch (weather->state) {
    case WEATHER_STATE_IDLE:
        break;
    case WEATHER_STATE_START:
        WeatherTask_Start(weather->tasks[1], WEATHER_OVERLAYS[weather->next].data, TRUE, TRUE, WEATHER_MODE_CROSSFADE,
                          weather->fadeIn, heapId);
        WeatherTask_RequestEnd(weather->tasks[0], TRUE, WEATHER_MODE_CROSSFADE);
        weather->state = WEATHER_STATE_WAIT_END;
        break;
    case WEATHER_STATE_WAIT_END:
        if (!WeatherTask_GetPhase(weather->tasks[0])) {
            task = weather->tasks[0];
            weather->tasks[0] = weather->tasks[1];
            weather->current = weather->next;
            weather->tasks[1] = task;
            weather->next = WEATHER_NONE;
            weather->fadeIn = TRUE;
            weather->state = WEATHER_STATE_IDLE;
        }
        break;
    }
}

static u32 FieldWeather_GetTransitionMode(u16 current, u16 next) {
    u32 result = WEATHER_TRANSITION_SEQUENTIAL;

    switch (current) {
    case WEATHER_FLAKE:
        if (next == WEATHER_BLIZZARD || next == WEATHER_FLUTTER) {
            result = WEATHER_TRANSITION_CROSSFADE;
        }
        break;
    case WEATHER_BLIZZARD:
        if (next == WEATHER_FLAKE || next == WEATHER_FLUTTER) {
            result = WEATHER_TRANSITION_CROSSFADE;
        }
        break;
    case WEATHER_FLUTTER:
        if (next == WEATHER_BLIZZARD || next == WEATHER_FLAKE) {
            result = WEATHER_TRANSITION_CROSSFADE;
        }
        break;
    }
    return result;
}
