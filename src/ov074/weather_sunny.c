#include "types.h"
#include "field/field_weather.h"

WeatherData SUNNY_WEATHER = {
    { 0 },
    sizeof(u32),
    {
        Sunny_Init,
        Sunny_FadeIn,
        Sunny_InitNoFade,
        Sunny_Main,
        Sunny_InitFadeOut,
        Sunny_FadeOut,
        Sunny_Exit,
    },
    NULL,
};

BOOL Sunny_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    u32 *state;

    if (WeatherTask_IsZoneFogActive(task)) {
        return FALSE;
    }
    state = WeatherTask_GetWorkBuffer(task);
    *state = 0;
    if (WeatherTask_IsZoneFog(task)) {
        WeatherTask_GetZoneFogOffset(task);
        WeatherTask_InitFog(task, WeatherTask_GetZoneFogSlope(task), 0x7fff, arg);
    }
    return TRUE;
}

BOOL Sunny_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    u32 *state = WeatherTask_GetWorkBuffer(task);
    if (*state == 0) {
        if (WeatherTask_IsZoneFog(task)) {
            u32 value = WeatherTask_GetZoneFogOffset(task);
            WeatherTask_ChangeFog(task, WeatherTask_GetZoneFogSlope(task), value, 0xa0, arg);
        }
        if (WeatherTask_IsZoneFogLight(task)) {
            WeatherTask_FadeLight(task, 0x51, WeatherTask_GetZoneFogLightId(task));
        }
        *state = 1;
    }
    if (WeatherTask_IsFogIdle(task)) {
        return TRUE;
    }
    return FALSE;
}

BOOL Sunny_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    if (WeatherTask_IsZoneFog(task)) {
        u32 value = WeatherTask_GetZoneFogOffset(task);
        WeatherTask_InitFog(task, WeatherTask_GetZoneFogSlope(task), value, arg);
    }
    if (WeatherTask_IsZoneFogLight(task)) {
        WeatherTask_SetLight(task, 0x51, WeatherTask_GetZoneFogLightId(task));
    }
    return TRUE;
}

BOOL Sunny_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    return FALSE;
}

BOOL Sunny_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    if (WeatherTask_IsZoneFog(task)) {
        WeatherTask_ChangeFogOffset(task, 0x7fff, 0x50, arg);
    }
    return TRUE;
}

BOOL Sunny_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    if (WeatherTask_IsFogIdle(task)) {
        return TRUE;
    }
    return FALSE;
}

BOOL Sunny_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    if (WeatherTask_IsZoneFog(task)) {
        WeatherTask_DisableFog(task, arg);
    }
    if (WeatherTask_IsZoneFogLight(task)) {
        WeatherTask_ResetLight(task, arg);
    }
    return TRUE;
}
