#include "types.h"
#include "field/field_weather.h"
#include "field/weather_particles.h"
#include "gfl/clact.h"
#include "gfl/random.h"

// Four weathers with particles, 1, 4, 5 and 8, in the order of their code. The ROM doesn't name the file; the name is
// a guess, and so are the weathers' names, which only say what the particles do: flakes that sway down, a blizzard of
// flakes that rush across a scrolling background, flakes that flutter, and specks that drift. Overlay 36's weather
// table (WEATHER_OVERLAYS) loads them. Like overlay 77's, they share the shape of WeatherData's functions

#define FLAKE_START_COLOR 0x7fdf
#define WEATHER_SOUND_TYPE 0x3d

// Weather 1's work
typedef struct {
    // Frames until the fade starts, when appearing
    int timer;
    // 1 for the second half of the flakes of the first batch
    int mode;
    // Set when the fade-out ends; the flakes then leave
    int done;
    int unused[9];
} FlakeWork;

// One of weather 1's flakes
typedef struct {
    // Points to FlakeWork's mode
    int *mode;
    int unused04[2];
    // The y step is taken when acc passes it
    int rate;
    int acc;
    // The index of sFlakeSway
    int swayIdx;
    int unused18;
    // Points to FlakeWork's done
    int *done;
    // 0 until the weather is done, then 1
    int state;
    // Frames left before the flake goes, once the weather is done
    int timer;
} Flake;

// Weather 4's work
typedef struct {
    int timer;
    // Counts the calls to Blizzard_AddFlakes, 0 to 1023: every 256 the wind changes
    int cycle;
    // The background's scroll, 0 to 255
    int scrollX;
    int unused0C[5];
    // The step of the background's scroll, 0 to 251: sBlizzardScrollX's and sBlizzardScrollY's entry every 28
    int scrollStep;
    int scrollY;
} BlizzardWork;

// One of weather 4's flakes
typedef struct {
    int frame;
    int lifetime;
    int speedY;
    // 0 while it moves, 1 once its lifetime is over
    int state;
    int speedX;
    // Added to speedX every 4 frames
    int accelX;
} BlizzardFlake;

// Weather 5's work
typedef struct {
    int timer;
    int unused[9];
} FlutterWork;

// One of weather 5's flakes
typedef struct {
    int frame;
    int lifetime;
    int speedY;
    // 0 while it falls, 1 while it flutters, 2 once it is gone
    int phase;
    int speedX;
    int unused14;
    int seed;
} FlutterFlake;

// Weather 8's work
typedef struct {
    int timer;
    int unused[9];
} SpeckWork;

// One of weather 8's specks
typedef struct {
    int frame;
    int lifetime;
    int stepX;
    int stepY;
    // The specks move every periodX frames in x and every periodY frames in y
    int periodX;
    int periodY;
} Speck;

static BOOL Flake_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flake_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flake_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flake_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flake_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flake_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flake_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void Flake_Move(WeatherObj *obj);
static void Flake_Add(WeatherTask *task, int count, HeapID heapId);
static BOOL Blizzard_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Blizzard_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Blizzard_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Blizzard_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Blizzard_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Blizzard_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Blizzard_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void BlizzardFlake_Move(WeatherObj *obj);
static void Blizzard_AddFlakes(WeatherTask *task, int count, HeapID heapId);
static void Blizzard_ScrollBg(WeatherTask *task, BlizzardWork *work);
static BOOL Flutter_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flutter_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flutter_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flutter_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flutter_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flutter_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Flutter_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void FlutterFlake_Move(WeatherObj *obj);
static void Flutter_AddFlakes(WeatherTask *task, int count, HeapID heapId);
static BOOL Speck_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Speck_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Speck_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Speck_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Speck_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Speck_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Speck_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void Speck_Move(WeatherObj *obj);
static void Speck_Add(WeatherTask *task, int count, HeapID heapId);

// What the background's scroll adds each step
static const s8 sBlizzardScrollX[9] = { 7, 8, 8, 8, 8, 8, 7, 7, 7 };
static const s8 sBlizzardScrollY[9] = { 3, 2, 1, 2, 1, 2, 3, 2, 3 };
static const int sBlizzardSpeedY[4] = { 3, 6, 3, 6 };
static const int sBlizzardSpeedX[4] = { 7, 11, 15, 19 };

// How far a flake moves in x at each step
static const s8 sFlakeSway[0x151] = {
    0, 0, 0,  0,  1, 0, 0, 0, 1, 0,  0,  0, 0, 1,  0,  0, 0,  0,  0, 1, 0,  0,  0, 0, 0, 0,  1,  0, 0, 0,  0,  0, 0, 0,
    1, 0, 0,  0,  0, 0, 0, 0, 0, 1,  0,  0, 0, 0,  0,  0, 0,  0,  0, 1, 0,  0,  0, 0, 0, 0,  0,  0, 0, 0,  1,  0, 0, 0,
    0, 0, 0,  0,  0, 0, 0, 0, 1, 0,  0,  0, 0, 0,  0,  0, 0,  0,  0, 0, 0,  -1, 0, 0, 0, 0,  0,  0, 0, 0,  0,  0, 0, -1,
    0, 0, 0,  0,  0, 0, 0, 0, 0, 0,  -1, 0, 0, 0,  0,  0, 0,  0,  0, 0, -1, 0,  0, 0, 0, 0,  0,  0, 0, -1, 0,  0, 0, 0,
    0, 0, 0,  -1, 0, 0, 0, 0, 0, 0,  -1, 0, 0, 0,  0,  0, -1, 0,  0, 0, 0,  -1, 0, 0, 0, -1, 0,  0, 0, 0,  -1, 0, 0, 0,
    0, 0, -1, 0,  0, 0, 0, 0, 0, -1, 0,  0, 0, 0,  0,  0, 0,  -1, 0, 0, 0,  0,  0, 0, 0, 0,  -1, 0, 0, 0,  0,  0, 0, 0,
    0, 0, -1, 0,  0, 0, 0, 0, 0, 0,  0,  0, 0, -1, 0,  0, 0,  0,  0, 0, 0,  0,  0, 0, 0, -1, 0,  0, 0, 0,  0,  0, 0, 0,
    0, 0, -1, 0,  0, 0, 0, 0, 0, 0,  0,  0, 0, 0,  -1, 0, 0,  0,  0, 0, 0,  0,  0, 0, 0, 0,  0,  1, 0, 0,  0,  0, 0, 0,
    0, 0, 0,  0,  0, 1, 0, 0, 0, 0,  0,  0, 0, 0,  0,  0, 1,  0,  0, 0, 0,  0,  0, 0, 0, 0,  1,  0, 0, 0,  0,  0, 0, 0,
    0, 1, 0,  0,  0, 0, 0, 0, 0, 1,  0,  0, 0, 0,  0,  0, 1,  0,  0, 0, 0,  0,  1, 0, 0, 0,  0,  1, 0, 0,  0,
};

WeatherData SPECK_WEATHER = {
    { 0x37, 1, 0, 0x1b, 0x1c, 0x1a, 0x19 },
    sizeof(SpeckWork),
    {
        Speck_Init,
        Speck_FadeIn,
        Speck_InitNoFade,
        Speck_Main,
        Speck_InitFadeOut,
        Speck_FadeOut,
        Speck_Exit,
    },
    Speck_Move,
};

WeatherData BLIZZARD_WEATHER = {
    { 0x37, 1, 1, 0x1f, 0x18, 0x1e, 0x1d, 0x2a, 0x303, 3, 0x102 },
    sizeof(BlizzardWork),
    {
        Blizzard_Init,
        Blizzard_FadeIn,
        Blizzard_InitNoFade,
        Blizzard_Main,
        Blizzard_InitFadeOut,
        Blizzard_FadeOut,
        Blizzard_Exit,
    },
    BlizzardFlake_Move,
};

WeatherData FLUTTER_WEATHER = {
    { 0x37, 1, 0, 2, 3, 1 },
    sizeof(FlutterWork),
    {
        Flutter_Init,
        Flutter_FadeIn,
        Flutter_InitNoFade,
        Flutter_Main,
        Flutter_InitFadeOut,
        Flutter_FadeOut,
        Flutter_Exit,
    },
    FlutterFlake_Move,
};

WeatherData FLAKE_WEATHER = {
    { 0x37, 1, 0, 0x17, 0x18, 0x16, 0x15 },
    sizeof(FlakeWork),
    {
        Flake_Init,
        Flake_FadeIn,
        Flake_InitNoFade,
        Flake_Main,
        Flake_InitFadeOut,
        Flake_FadeOut,
        Flake_Exit,
    },
    Flake_Move,
};

static BOOL Flake_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    FlakeWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 1, 18, 1, 16, -3, 1, 0, Flake_Add);
    work->mode = 0;
    work->done = 0;
    WeatherTask_InitFog(task, 8, FLAKE_START_COLOR, arg);
    work->timer = 8;
    WeatherTask_SaveCameraPos(task);
    return TRUE;
}

static BOOL Flake_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    FlakeWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFog(task, 8, 0x7f5f, 0x5a, arg);
            WeatherTask_FadeLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(1));
        }
    } else if (WeatherTask_IsFogIdle(task) && fading) {
        return TRUE;
    }
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Flake_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    FlakeWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 1, 16, 1, 16, -3, 1, 0, Flake_Add);
    work->mode = 1;
    work->done = 0;
    WeatherTask_InitFog(task, 8, 0x7f5f, arg);
    WeatherTask_PrewarmParticles(task, Flake_Add, 20, 1, 18, heapId);
    WeatherTask_SetLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(1));
    return TRUE;
}

static BOOL Flake_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    int x;
    int y;

    WeatherTask_StepSpawnTimer(task, heapId);
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Flake_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    FlakeWork *work = WeatherTask_GetWorkBuffer(task);
    int x;
    int y;

    WeatherTask_RetargetSpawner(task, 0, 18, 3, -1);
    work->timer = 16;
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return TRUE;
}

static BOOL Flake_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    FlakeWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFogOffset(task, FLAKE_START_COLOR, 0x5a, arg);
        }
    } else if ((arg ? WeatherTask_IsFogIdle(task) : TRUE) && fading) {
        work->done = TRUE;
        if (!WeatherTask_GetParticleCount(task)) {
            return TRUE;
        }
    }
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Flake_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_DisableFog(task, arg);
    WeatherTask_ResetLight(task, arg);
    return TRUE;
}

static void Flake_Move(WeatherObj *obj) {
    Flake *flake = WeatherTask_GetParticleWork(obj);
    ClActorPos pos;
    int *done;

    WeatherTask_GetParticleActor(obj);
    done = flake->done;
    if (flake->state == 0) {
        if (*done != 0) {
            flake->state++;
        }
    } else if (--flake->timer < 0) {
        WeatherTask_RemoveParticle(obj);
        return;
    }
    func_ov036_02199c6c(obj, &pos);
    pos.x += sFlakeSway[flake->swayIdx];
    if (flake->acc > flake->rate) {
        pos.y++;
        flake->acc %= flake->rate;
    }
    WeatherTask_SetParticlePosWrapped(obj, &pos);
    flake->acc += 8;
    flake->swayIdx = (flake->swayIdx + 1) % NELEMS(sFlakeSway);
    if ((pos.y < -0x11c && pos.y > -0x128) || (pos.y > 0xd4 && pos.y < 0xe8)) {
        *flake->mode = TRUE;
        WeatherTask_RemoveParticle(obj);
    }
}

static void Flake_Add(WeatherTask *task, int count, HeapID heapId) {
    FlakeWork *work = WeatherTask_GetWorkBuffer(task);
    int i = 0;

    if (count > 0) {
        int *mode = &work->mode;
        int *done = &work->done;

        do {
            WeatherObj *obj = WeatherTask_AddParticle(task, heapId);
            Flake *flake;
            ClActor *actor;
            ClActorPos pos;

            if (obj == NULL) {
                break;
            }
            flake = WeatherTask_GetParticleWork(obj);
            actor = WeatherTask_GetParticleActor(obj);
            ClActor_SetAnimFrame(actor, GFL_RandomLC(4));
            flake->acc = 0;
            flake->swayIdx = 0;
            flake->state = 0;
            flake->timer = (int)GFL_RandomLC(0) % 30 + 2;
            flake->rate = GFL_RandomLC(8) + 12;
            flake->mode = mode;
            flake->done = done;
            pos.x = GFL_RandomLC(0x19e) - 0x20;
            if (work->mode == 1 && i >= count / 2) {
                pos.y = -8 - 0x20 - GFL_RandomLC(20);
            } else {
                pos.y = -8 - GFL_RandomLC(20);
            }
            WeatherTask_SetParticlePosWrapped(obj, &pos);
        } while (++i < count);
    }
}

static BOOL Blizzard_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 1, 6, 1, 2, -2, 2, 1, Blizzard_AddFlakes);
    WeatherTask_InitFog(task, 4, 0x7fff, arg);
    work->timer = 8;
    work->cycle = 0;
    work->scrollX = 0;
    work->scrollY = 0;
    work->scrollStep = 0;
    WeatherTask_SaveCameraPos(task);
    WeatherTask_PlaySe(task, 0x6e3);
    return TRUE;
}

static BOOL Blizzard_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFog(task, 4, 0x7b18, 0x5a, arg);
            WeatherTask_FadeLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(4));
        }
    } else if (WeatherTask_IsFogIdle(task) && fading) {
        WeatherTask_SetBillboardVisible(task, 1, 0);
        return TRUE;
    }
    Blizzard_ScrollBg(task, work);
    return FALSE;
}

static BOOL Blizzard_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 1, 2, 1, 2, -2, 2, 1, Blizzard_AddFlakes);
    WeatherTask_InitFog(task, 4, 0x7b18, arg);
    work->cycle = 0;
    work->scrollX = 0;
    work->scrollY = 0;
    WeatherTask_PrewarmParticles(task, Blizzard_AddFlakes, 20, 2, 2, heapId);
    WeatherTask_SetBillboardVisible(task, 1, 0);
    WeatherTask_SetLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(4));
    return TRUE;
}

static BOOL Blizzard_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);

    if (WeatherTask_StepSpawnTimer(task, heapId)) {
        // Never true, since sBlizzardSpeedX holds no negative speed
        if (sBlizzardSpeedX[work->cycle / 256] <= -8) {
            Blizzard_AddFlakes(task, 1, heapId);
        }
    }
    Blizzard_ScrollBg(task, work);
    return FALSE;
}

static BOOL Blizzard_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_RetargetSpawner(task, 0, 3, 1, -1);
    work->timer = 10;
    Blizzard_ScrollBg(task, work);
    WeatherTask_SetBillboardVisible(task, 0, 0);
    WeatherTask_StopSe(task);
    return TRUE;
}

static BOOL Blizzard_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFogOffset(task, 0x7fff, 0x3c, arg);
        }
    } else if ((arg ? WeatherTask_IsFogIdle(task) : TRUE) && fading) {
        // Weather 1's code, where this is the flag that makes the flakes leave, but here it is the background's scroll
        work->scrollX = TRUE;
        if (!WeatherTask_GetParticleCount(task)) {
            return TRUE;
        }
    }
    Blizzard_ScrollBg(task, work);
    return FALSE;
}

static BOOL Blizzard_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_DisableFog(task, arg);
    WeatherTask_ResetLight(task, arg);
    return TRUE;
}

static void BlizzardFlake_Move(WeatherObj *obj) {
    BlizzardFlake *flake = WeatherTask_GetParticleWork(obj);
    ClActor *actor = WeatherTask_GetParticleActor(obj);
    ClActorPos pos;

    ClActor_GetSurfacePos(actor, &pos, 0);
    switch (flake->state) {
    case 0:
        pos.x += (s16)flake->speedX;
        pos.y += (s16)flake->speedY;
        if (flake->frame++ > flake->lifetime) {
            flake->state = 1;
        }
        if (flake->frame % 4 == 0) {
            flake->speedX += flake->accelX;
            if (flake->speedY > 1) {
                flake->speedY--;
            }
        }
        ClActor_SetSurfacePos(actor, &pos, 0);
        break;
    case 1:
        WeatherTask_RemoveParticle(obj);
        break;
    }
}

static void Blizzard_AddFlakes(WeatherTask *task, int count, HeapID heapId) {
    BlizzardWork *work = WeatherTask_GetWorkBuffer(task);
    int index;
    int i;

    if (++work->cycle >= 1024) {
        work->cycle = 0;
    }
    index = work->cycle / 256;
    for (i = 0; i < count; i++) {
        WeatherObj *obj = WeatherTask_AddParticle(task, heapId);
        BlizzardFlake *flake;
        ClActor *actor;
        int size;
        ClActorPos pos;

        if (obj == NULL) {
            break;
        }
        flake = WeatherTask_GetParticleWork(obj);
        actor = WeatherTask_GetParticleActor(obj);
        flake->frame = 0;
        flake->lifetime = GFL_RandomLC(3) + 9;
        size = GFL_RandomLC(4);
        ClActor_SetAnimFrame(actor, size);
        flake->speedX = (size + 1) * sBlizzardSpeedX[index];
        flake->speedY = (size + 1) * sBlizzardSpeedY[index];
        flake->state = 0;
        if (size == 3) {
            flake->speedX += sBlizzardSpeedX[index];
            flake->speedY += sBlizzardSpeedY[index];
        }
        flake->accelX = sBlizzardSpeedX[index];
        pos.x = -GFL_RandomLC(24);
        pos.y = GFL_RandomLC(168) - 0x20;
        ClActor_SetSurfacePos(actor, &pos, 0);
    }
}

static void Blizzard_ScrollBg(WeatherTask *task, BlizzardWork *work) {
    int x;
    int y;
    int scrollStep;
    int step;

    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    scrollStep = work->scrollStep;
    step = scrollStep / 28;
    scrollStep = (scrollStep + 1) % (NELEMS(sBlizzardScrollX) * 28);
    work->scrollStep = scrollStep;
    work->scrollX = work->scrollX + sBlizzardScrollX[step] - x / 2;
    work->scrollY = work->scrollY + sBlizzardScrollY[step] - y / 2;
    if (work->scrollX < 0) {
        work->scrollX += 256;
    }
    if (work->scrollY < 0) {
        work->scrollY += 256;
    }
    work->scrollX %= 256;
    work->scrollY %= 256;
    WeatherTask_SetBillboardTexOffsetX(task, -work->scrollX, 0);
    WeatherTask_SetBillboardTexOffsetY(task, -work->scrollY, 0);
}

static BOOL Flutter_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    FlutterWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 0, 8, 1, 1, -4, 2, 1, Flutter_AddFlakes);
    WeatherTask_InitFog(task, 9, FLAKE_START_COLOR, arg);
    work->timer = 1;
    WeatherTask_SaveCameraPos(task);
    WeatherTask_PlaySe(task, 0x6e4);
    return TRUE;
}

static BOOL Flutter_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    FlutterWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFog(task, 9, 0x7f80, 0x3c, arg);
            WeatherTask_FadeLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(5));
        }
    } else if (WeatherTask_IsFogIdle(task) && fading) {
        return TRUE;
    }
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Flutter_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_SetupSpawner(task, 1, 1, 1, 1, -4, 2, 1, Flutter_AddFlakes);
    WeatherTask_InitFog(task, 9, 0x7f80, arg);
    WeatherTask_PrewarmParticles(task, Flutter_AddFlakes, 20, 10, 1, heapId);
    WeatherTask_SetLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(5));
    return TRUE;
}

static BOOL Flutter_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    int x;
    int y;

    WeatherTask_GetWorkBuffer(task);
    WeatherTask_StepSpawnTimer(task, heapId);
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Flutter_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    FlutterWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_RetargetSpawner(task, 0, 8, 4, -1);
    work->timer = 1;
    WeatherTask_StopSe(task);
    return TRUE;
}

static BOOL Flutter_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    FlutterWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFogOffset(task, FLAKE_START_COLOR, 0x3c, arg);
        }
    } else if ((arg ? WeatherTask_IsFogIdle(task) : TRUE) && fading && !WeatherTask_GetParticleCount(task)) {
        return TRUE;
    }
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Flutter_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_DisableFog(task, arg);
    WeatherTask_ResetLight(task, arg);
    return TRUE;
}

static void FlutterFlake_Move(WeatherObj *obj) {
    FlutterFlake *flake = WeatherTask_GetParticleWork(obj);
    ClActor *actor = WeatherTask_GetParticleActor(obj);
    ClActorPos pos;

    func_ov036_02199c6c(obj, &pos);
    switch (flake->phase) {
    case 0:
        if (++flake->frame > flake->lifetime) {
            flake->phase = 1;
            flake->frame = flake->seed % 4 * 2 + 8;
            flake->speedX = 2;
            flake->speedY = 2;
            ClActor_SetAnimFrame(actor, ClActor_GetAnimFrame(actor) + 3);
            return;
        }
        pos.x += (s16)flake->speedX;
        pos.y += (s16)flake->speedY;
        WeatherTask_SetParticlePosWrapped(obj, &pos);
        break;
    case 1:
        pos.x += (s16)flake->speedX;
        if (flake->frame % 2 == 0) {
            pos.y += (s16)flake->speedY;
        } else {
            pos.y -= (s16)flake->speedY;
        }
        if (flake->frame % 4 == 0) {
            if (flake->speedY - 1 >= 0) {
                flake->speedY--;
            }
            if (flake->speedX - 1 >= 0) {
                flake->speedX--;
            }
        }
        WeatherTask_SetParticlePos(obj, &pos);
        if (flake->frame-- <= 0) {
            flake->phase = 2;
        }
        break;
    case 2:
        WeatherTask_RemoveParticle(obj);
        break;
    }
}

static void Flutter_AddFlakes(WeatherTask *task, int count, HeapID heapId) {
    int i;

    WeatherTask_GetWorkBuffer(task);
    for (i = 0; i < count; i++) {
        WeatherObj *obj = WeatherTask_AddParticle(task, heapId);
        FlutterFlake *flake;
        ClActor *actor;
        u32 seed;
        int kind;
        int m;
        ClActorPos pos;

        if (obj == NULL) {
            break;
        }
        flake = WeatherTask_GetParticleWork(obj);
        actor = WeatherTask_GetParticleActor(obj);
        seed = GFL_RandomLC(0);
        flake->frame = 0;
        if (seed % 100 < 30) {
            kind = 0;
        } else if (seed % 100 < 95) {
            kind = 1;
        } else if (seed % 100 < 100) {
            kind = 2;
        }
        ClActor_SetAnimFrame(actor, 2 - kind);
        m = seed % 23;
        flake->speedY = m + (kind + 1) * 10;
        if (kind == 2) {
            flake->speedY += 3;
        }
        flake->speedX = m / 4 + (kind + 1) * 4;
        if (kind == 2) {
            flake->speedX -= 2;
        }
        flake->phase = 0;
        flake->lifetime = (seed & 3) + 1;
        pos.x = seed % 0x10e + (kind * 15 - 0x40);
        pos.y = (seed & 0xf) - 8;
        WeatherTask_SetParticlePosWrapped(obj, &pos);
        flake->seed = seed;
    }
}

static BOOL Speck_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    SpeckWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_SetupSpawner(task, 2, 16, 20, 2, -2, 4, 2, Speck_Add);
    WeatherTask_InitFog(task, 8, 0x7fff, arg);
    work->timer = 0;
    return TRUE;
}

static BOOL Speck_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    SpeckWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);
    int x;
    int y;

    if (work->timer == 0) {
        work->timer--;
        WeatherTask_ChangeFog(task, 8, 0x7f57, 0x5a, arg);
        WeatherTask_FadeLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(8));
    } else if (WeatherTask_IsFogIdle(task) && fading) {
        return TRUE;
    }
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Speck_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_SetupSpawner(task, 20, 2, 20, 2, -2, 4, 2, Speck_Add);
    WeatherTask_InitFog(task, 8, 0x7f57, arg);
    WeatherTask_PrewarmParticles(task, Speck_Add, 20, 10, 1, heapId);
    WeatherTask_SetLight(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(8));
    return TRUE;
}

static BOOL Speck_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    int x;
    int y;

    WeatherTask_GetWorkBuffer(task);
    WeatherTask_StepSpawnTimer(task, heapId);
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Speck_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    SpeckWork *work = WeatherTask_GetWorkBuffer(task);

    WeatherTask_RetargetSpawner(task, 0, 16, 2, -2);
    work->timer = 100;
    return TRUE;
}

static BOOL Speck_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    SpeckWork *work = WeatherTask_GetWorkBuffer(task);
    BOOL fading = WeatherTask_StepSpawner(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            WeatherTask_ChangeFogOffset(task, 0x7fff, 0x78, arg);
        }
    } else if ((arg ? WeatherTask_IsFogIdle(task) : TRUE) && fading && !WeatherTask_GetParticleCount(task)) {
        return TRUE;
    }
    WeatherTask_GetCameraScroll(task, &x, &y);
    WeatherTask_ShiftParticles(task, x, y);
    return FALSE;
}

static BOOL Speck_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    WeatherTask_GetWorkBuffer(task);
    WeatherTask_DisableFog(task, arg);
    WeatherTask_ResetLight(task, arg);
    return TRUE;
}

static void Speck_Move(WeatherObj *obj) {
    Speck *speck = WeatherTask_GetParticleWork(obj);
    ClActorPos pos;

    WeatherTask_GetParticleActor(obj);
    func_ov036_02199c6c(obj, &pos);
    speck->frame++;
    if (speck->frame % speck->periodX == 0) {
        pos.x += (s16)speck->stepX;
    }
    if (speck->frame % speck->periodY == 0) {
        pos.y += (s16)speck->stepY;
    }
    // BUG: The new position is never set, so the specks stay where they appear
#ifdef BUGFIX
    WeatherTask_SetParticlePosWrapped(obj, &pos);
#endif
    if (speck->frame >= speck->lifetime) {
        WeatherTask_RemoveParticle(obj);
    }
}

static void Speck_Add(WeatherTask *task, int count, HeapID heapId) {
    int i;

    WeatherTask_GetWorkBuffer(task);
    for (i = 0; i < count; i++) {
        WeatherObj *obj = WeatherTask_AddParticle(task, heapId);
        int a;
        int b;
        Speck *speck;
        ClActor *actor;
        int frame;
        int low;
        int high;
        int range;
        ClActorPos pos;

        if (obj == NULL) {
            break;
        }
        speck = WeatherTask_GetParticleWork(obj);
        actor = WeatherTask_GetParticleActor(obj);
        a = GFL_RandomLC(0);
        b = GFL_RandomLC(0);
        speck->frame = 0;
        speck->lifetime = a % 5 + 7;
        speck->stepX = a % 2 == 0 ? 1 : -1;
        speck->stepY = 1;
        speck->periodX = a % 6 + 3;
        speck->periodY = b % 5 + 4;
        frame = a % 20;
        pos.x = a % 0x180 - 0x40;
        pos.y = b % 256 - 8;
        WeatherTask_SetParticlePosWrapped(obj, &pos);
        low = 50 - pos.x / 3;
        range = 0xce - pos.x / 3;
        if (range < 0) {
            high = low - b % -range;
        } else {
            high = low + b % range;
        }
        if (low <= pos.y && high >= pos.y) {
            speck->lifetime *= 2;
        } else {
            frame = a % 4;
        }
        ClActor_SetAnimFrame(actor, frame);
    }
}
