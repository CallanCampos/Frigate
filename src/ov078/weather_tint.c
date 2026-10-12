#include "types.h"
#include "field/field_weather.h"
#include "field/weather_tint.h"

// The weathers that only tint the screen. The ROM doesn't name the file; the name is a guess. Each fades a level and an
// RGB555 colour in and out and plays a sound. Weather 9 is fixed. The others (10, 11, 13 and 14) pick the level and the
// colour by the camera's defaults index, and share all but their first function

#define TINT_START_COLOR 0x7fdf
#define TINT_FADE_IN_FRAMES 0x5a
#define TINT_FADE_OUT_FRAMES 0x3c
#define TINT_SOUND_TYPE 0x3d
#define TINT_DEFAULTS_COUNT 34

// Weather 9's work
typedef struct {
    // 0 until the fade-in starts
    int state;
    int unused[9];
    u32 level;
    u32 color;
} Tint9Work;

// The work of the other weathers
typedef struct {
    // The index of the sound
    int sound;
    // 0 until the fade-in starts
    int state;
    int unused[9];
    u32 level;
    u32 color;
} TintWork;

static BOOL Tint9_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint9_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint9_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint9_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint9_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint9_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint9_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void Tint9_ObjFunc(WeatherObj *obj);
static BOOL Tint10_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint11_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint13_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint14_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Tint_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void Tint_ObjFunc(WeatherObj *obj);

// The level of the tint by the camera's defaults index
static const u8 sLevels[TINT_DEFAULTS_COUNT] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 1, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

// The colour of the tint by the camera's defaults index
static const u32 sColors[TINT_DEFAULTS_COUNT] = {
    0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x0000,
    0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f,
    0x7f5f, 0x7f5f, 0x0a08, 0x7f5f, 0x7f6f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f, 0x7f5f,
    0x7f5f,
};

WeatherData TINT_WEATHER_9 = {
    { 0x37 },
    sizeof(Tint9Work),
    {
        Tint9_Init,
        Tint9_FadeIn,
        Tint9_InitNoFade,
        Tint9_Main,
        Tint9_InitFadeOut,
        Tint9_FadeOut,
        Tint9_Exit,
    },
    Tint9_ObjFunc,
};

WeatherData TINT_WEATHER_10 = {
    { 0x37 },
    sizeof(TintWork),
    {
        Tint10_Init,
        Tint_FadeIn,
        Tint_InitNoFade,
        Tint_Main,
        Tint_InitFadeOut,
        Tint_FadeOut,
        Tint_Exit,
    },
    Tint_ObjFunc,
};

WeatherData TINT_WEATHER_11 = {
    { 0x37 },
    sizeof(TintWork),
    {
        Tint11_Init,
        Tint_FadeIn,
        Tint_InitNoFade,
        Tint_Main,
        Tint_InitFadeOut,
        Tint_FadeOut,
        Tint_Exit,
    },
    Tint_ObjFunc,
};

WeatherData TINT_WEATHER_13 = {
    { 0x37 },
    sizeof(TintWork),
    {
        Tint13_Init,
        Tint_FadeIn,
        Tint_InitNoFade,
        Tint_Main,
        Tint_InitFadeOut,
        Tint_FadeOut,
        Tint_Exit,
    },
    Tint_ObjFunc,
};

WeatherData TINT_WEATHER_14 = {
    { 0x37 },
    sizeof(TintWork),
    {
        Tint14_Init,
        Tint_FadeIn,
        Tint_InitNoFade,
        Tint_Main,
        Tint_InitFadeOut,
        Tint_FadeOut,
        Tint_Exit,
    },
    Tint_ObjFunc,
};

static BOOL Tint9_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    Tint9Work *work = func_ov036_021997e4(task);

    func_ov036_0219990c(task, 9, TINT_START_COLOR, arg);
    work->state = 0;
    work->level = 9;
    work->color = TINT_START_COLOR - 0x67;
    return TRUE;
}

static BOOL Tint9_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    Tint9Work *work = func_ov036_021997e4(task);

    if (work->state == 0) {
        func_ov036_02199958(task, work->level, work->color, TINT_FADE_IN_FRAMES, arg);
        func_ov036_02199734(task, TINT_SOUND_TYPE, FieldLightStatus_GetWeatherLight(9));
        work->state--;
    } else if (func_ov036_021999dc(task)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Tint9_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    Tint9Work *work = func_ov036_021997e4(task);

    func_ov036_0219990c(task, work->level, work->color, arg);
    func_ov036_02199740(task, TINT_SOUND_TYPE, FieldLightStatus_GetWeatherLight(9));
    return TRUE;
}

static BOOL Tint9_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    return FALSE;
}

static BOOL Tint9_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    Tint9Work *work = func_ov036_021997e4(task);

    work->state = 0;
    return TRUE;
}

static BOOL Tint9_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    Tint9Work *work = func_ov036_021997e4(task);

    if (work->state == 0) {
        work->state--;
        func_ov036_021999bc(task, TINT_START_COLOR, TINT_FADE_OUT_FRAMES, arg);
    } else if ((arg != 0 ? func_ov036_021999dc(task) : TRUE) && !func_ov036_021997e8(task)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Tint9_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_02199948(task, arg);
    func_ov036_0219974c(task, arg);
    return TRUE;
}

static void Tint9_ObjFunc(WeatherObj *obj) {
}

static BOOL Tint10_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    u32 index = func_ov036_02199774(task);
    TintWork *work = func_ov036_021997e4(task);

    work->state = 0;
    work->sound = 10;
    work->level = sLevels[index];
    work->color = sColors[index];
    func_ov036_0219990c(task, work->level, TINT_START_COLOR, arg);
    return TRUE;
}

static BOOL Tint11_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    u32 index = func_ov036_02199774(task);
    TintWork *work = func_ov036_021997e4(task);

    work->state = 0;
    work->sound = 11;
    work->level = sLevels[index];
    work->color = sColors[index];
    func_ov036_0219990c(task, work->level, TINT_START_COLOR, arg);
    return TRUE;
}

static BOOL Tint13_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    u32 index = func_ov036_02199774(task);
    TintWork *work = func_ov036_021997e4(task);

    work->state = 0;
    work->sound = 13;
    work->level = sLevels[index];
    work->color = sColors[index];
    func_ov036_0219990c(task, work->level, TINT_START_COLOR, arg);
    return TRUE;
}

static BOOL Tint14_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    u32 index = func_ov036_02199774(task);
    TintWork *work = func_ov036_021997e4(task);

    work->state = 0;
    work->sound = 14;
    work->level = sLevels[index];
    work->color = sColors[index];
    func_ov036_0219990c(task, work->level, TINT_START_COLOR, arg);
    return TRUE;
}

static BOOL Tint_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    TintWork *work = func_ov036_021997e4(task);

    if (work->state == 0) {
        func_ov036_02199958(task, work->level, work->color, TINT_FADE_IN_FRAMES, arg);
        func_ov036_02199734(task, TINT_SOUND_TYPE, FieldLightStatus_GetWeatherLight(work->sound));
        work->state--;
    } else if (func_ov036_021999dc(task)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Tint_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    TintWork *work = func_ov036_021997e4(task);

    func_ov036_0219990c(task, work->level, work->color, arg);
    func_ov036_02199740(task, TINT_SOUND_TYPE, FieldLightStatus_GetWeatherLight(work->sound));
    return TRUE;
}

static BOOL Tint_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    return FALSE;
}

static BOOL Tint_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    TintWork *work = func_ov036_021997e4(task);

    work->state = 0;
    return TRUE;
}

static BOOL Tint_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    TintWork *work = func_ov036_021997e4(task);

    if (work->state == 0) {
        work->state--;
        func_ov036_021999bc(task, TINT_START_COLOR, TINT_FADE_OUT_FRAMES, arg);
    } else if ((arg != 0 ? func_ov036_021999dc(task) : TRUE) && !func_ov036_021997e8(task)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Tint_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_02199948(task, arg);
    func_ov036_0219974c(task, arg);
    return TRUE;
}

static void Tint_ObjFunc(WeatherObj *obj) {
}
