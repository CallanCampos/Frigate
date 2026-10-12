#include "types.h"
#include "constants/sound.h"
#include "field/field_weather.h"
#include "field/weather_rain.h"
#include "gfl/clact.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"

// The rain weathers. The ROM doesn't name the file; the name is a guess. Weather 2 is plain rain: drops that fall
// at a slant and splash. Weather 6 is a thunderstorm: faster drops over a scrolling background, with lightning
// that flashes the screen and rumbles. Weather 7 is rain that gusts: a lull, then a gust that sweeps three kinds of
// streak across two scrolling backgrounds. Overlay 36's weather table (WEATHER_OVERLAYS) loads them

#define WEATHER_SOUND_TYPE 0x3d
#define RAIN_START_COLOR 0x7fdf
#define RAIN_FADE_IN_FRAMES 0x5a
#define RAIN_FADE_OUT_FRAMES 0x32

// Weather 2's work
typedef struct {
    // 1 when the weather starts
    int timer;
    // The weather asks for 0x28 bytes of work but uses only these
    int unused[9];
} RainWork;

// One of weather 2's drops
typedef struct {
    int frame;
    // The frame (in steps of 2) after which the drop splashes
    int splashFrame;
    int speedY;
    // 0 falling, 1 splashing, 2 gone
    int state;
    int speedX;
    // Random: drops with 7 or more splash on the ground
    int splash;
} RainDrop;

// One of lightning's flashes
typedef struct {
    u16 flashSpeed;
    u16 flashFrames;
    // The flash's shade
    u16 shade;
    // Frames until the next flash
    s16 delay;
    s16 soundDelay;
    u16 sound;
} LightningFlash;

// The lightning of weather 6
typedef struct {
    u8 state;
    // 0, 1 or 2: how the lightning flickers
    u8 kind;
    // The shape of the strike that was chosen
    u16 type;
    u16 count;
    u16 index;
    LightningFlash flashes[4];
    int timer;
    s16 soundDelay;
    s16 sound;
} Lightning;

// Weather 6's work
typedef struct {
    s16 state;
    s16 timer;
    int scroll;
    int frame;
    Lightning lightning;
} ThunderWork;

// One of weather 6's drops
typedef struct {
    int unused00;
    int unused04;
    int speedY;
    // 0 falling, 1 gone
    int state;
    int speedX;
} ThunderDrop;

// The speeds of the gusts of weather 7, 10 times too big
typedef struct {
    int x;
    int y;
    int unused;
} GustWind;

// Weather 7's work
typedef struct {
    s16 state;
    s16 timer;
    // The two backgrounds' scroll
    s16 scrollX1;
    s16 scrollY1;
    s16 scrollX2;
    s16 scrollY2;
    // Frames into the mode and where it ends
    s16 counter;
    s16 limit;
    // 0 off, 1 lull, 2 gust, 3 the gust begins, 4 the gust ends
    s16 mode;
    // Frames into the next rumble, and where it comes
    s16 rumbleCounter;
    s16 rumbleLimit;
    // The speed that the particles blow at
    s16 windX;
    s16 windY;
    int bgScale;
} GustWork;

typedef struct {
    s16 x;
    s16 y;
    s16 speedX;
    s16 speedY;
    s16 baseX;
    s16 baseY;
} GustParticleData;

// One of weather 7's streaks
typedef struct {
    int kind;
    GustWork *work;
    GustParticleData data;
} GustParticle;

// Both kinds of function get the particle's actor, and the move functions ignore it
typedef void (*GustParticleFunc)(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);

static BOOL Rain_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Rain_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Rain_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Rain_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Rain_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Rain_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Rain_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void RainDrop_Move(WeatherObj *obj);
static void Rain_AddDrops(WeatherTask *task, int count, HeapID heapId);
static BOOL Thunder_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Thunder_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Thunder_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Thunder_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Thunder_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Thunder_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL Thunder_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void ThunderDrop_Move(WeatherObj *obj);
static void Thunder_AddDrops(WeatherTask *task, int count, HeapID heapId);
static void Thunder_ScrollBg(WeatherTask *task, ThunderWork *work);
static BOOL GustyRain_Init(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL GustyRain_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL GustyRain_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL GustyRain_Main(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL GustyRain_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL GustyRain_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
static BOOL GustyRain_Exit(WeatherTask *task, u32 arg, HeapID heapId);
static void GustParticle_Move(WeatherObj *obj);
static void Gust_AddParticles(WeatherTask *task, int count, HeapID heapId);
static void GustSlant_Move(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);
static void GustSlant_Init(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);
static void GustStreak_Move(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);
static void GustStreak_Init(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);
static void GustDriven_Move(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);
static void GustDriven_Init(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor);
static void Gust_StartLull(WeatherTask *task, GustWork *work, u32 arg, BOOL noFade);
static void Gust_Update(WeatherTask *task, GustWork *work, u32 arg, BOOL noFade);
static void Gust_ScrollBgs(WeatherTask *task, GustWork *work);
static void Gust_PlayGustSound(WeatherTask *task, GustWork *work, u32 arg, u32 sound);
static void Gust_ResetRumble(GustWork *work);
static void Gust_PlayRumble(WeatherTask *task, GustWork *work, u32 arg);
static void Lightning_Init(Lightning *lightning, int kind);
static void Lightning_Exit(Lightning *lightning);
static void Lightning_Update(Lightning *lightning, WeatherTask *task);
static void Lightning_SetupFlickers(Lightning *lightning);
static void Lightning_SetupBolt(Lightning *lightning);
static void Lightning_SetupForked(Lightning *lightning);
static u16 Lightning_GetShade(Lightning *lightning);

static const int sThunderSpeedScale[5] = { 10, 10, 13, 10, 16 };

static const u8 sLightningFlashFrames[32] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 20, 20, 30, 30, 35,
    35, 35, 40, 40,
};

static const int sGustBgScale[14] = {
    0x1100, 0x1200, 0x1300, 0x1300, 0x1300, 0x1200, 0x1100, 0x1000, 0x1000, 0x1200, 0x1300, 0x1200, 0x1200, 0x1100,
};

static const GustWind sGustWind[6] = {
    { 0xd0, 0x18, 0 }, { 0x120, 0x10, 0 }, { 0xf0, 0x18, 0 }, { 0x130, 0x18, 0 }, { 0xc0, 0x18, 0 }, { 0x100, 0x10, 0 },
};

static GustParticleFunc sGustParticleInit[3] = { GustSlant_Init, GustStreak_Init, GustDriven_Init };
static GustParticleFunc sGustParticleMove[3] = { GustSlant_Move, GustStreak_Move, GustDriven_Move };

WeatherData THUNDERSTORM_WEATHER = {
    { 0x37, 1, 1, 0x11, 0xe, 0x10, 0xf, 0x29, 0x202, 3, 0x102 },
    sizeof(ThunderWork),
    {
        Thunder_Init,
        Thunder_FadeIn,
        Thunder_InitNoFade,
        Thunder_Main,
        Thunder_InitFadeOut,
        Thunder_FadeOut,
        Thunder_Exit,
    },
    ThunderDrop_Move,
};

WeatherData GUSTY_RAIN_WEATHER = {
    { 0x37, 1, 2, 6, 7, 5, 4, 0x28, 0x202, 3, 0x102, 0, 0, 0x27, 0x202, 3, 0x102 },
    sizeof(GustWork),
    {
        GustyRain_Init,
        GustyRain_FadeIn,
        GustyRain_InitNoFade,
        GustyRain_Main,
        GustyRain_InitFadeOut,
        GustyRain_FadeOut,
        GustyRain_Exit,
    },
    GustParticle_Move,
};

WeatherData RAIN_WEATHER = {
    { 0x37, 1, 0, 0xd, 0xe, 0xc, 0xb },
    sizeof(RainWork),
    {
        Rain_Init,
        Rain_FadeIn,
        Rain_InitNoFade,
        Rain_Main,
        Rain_InitFadeOut,
        Rain_FadeOut,
        Rain_Exit,
    },
    RainDrop_Move,
};

static BOOL Rain_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    RainWork *work = func_ov036_021997e4(task);

    func_ov036_02199824(task, 1, 8, 2, 0, -4, 2, 1, Rain_AddDrops);
    func_ov036_0219990c(task, 8, RAIN_START_COLOR, arg);
    work->timer = 1;
    func_ov036_02199a50(task);
    return TRUE;
}

static BOOL Rain_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    RainWork *work = func_ov036_021997e4(task);
    BOOL done = func_ov036_02199868(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            func_ov036_02199958(task, 8, 0x7f5f, RAIN_FADE_IN_FRAMES, arg);
            func_ov036_02199734(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(2));
            func_ov036_02199be8(task, SEQ_SE_FLD_32);
        }
    } else if (func_ov036_021999dc(task) && done) {
        return TRUE;
    }
    func_ov036_02199a78(task, &x, &y);
    func_ov036_02199b90(task, x, y);
    return FALSE;
}

static BOOL Rain_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199824(task, 2, 0, 2, 0, -4, 2, 1, Rain_AddDrops);
    func_ov036_0219990c(task, 8, 0x7f5f, arg);
    func_ov036_02199880(task, Rain_AddDrops, 20, 10, 1, heapId);
    func_ov036_02199be8(task, SEQ_SE_FLD_32);
    func_ov036_02199740(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(2));
    return TRUE;
}

static BOOL Rain_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    int x;
    int y;

    func_ov036_021997e4(task);
    func_ov036_02199874(task, heapId);
    func_ov036_02199a78(task, &x, &y);
    func_ov036_02199b90(task, x, y);
    return FALSE;
}

static BOOL Rain_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    RainWork *work = func_ov036_021997e4(task);

    func_ov036_02199854(task, 0, 8, 4, -1);
    work->timer = 1;
    func_ov036_02199bf8(task);
    return TRUE;
}

static BOOL Rain_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    RainWork *work = func_ov036_021997e4(task);
    BOOL done = func_ov036_02199868(task, heapId);
    int x;
    int y;

    if (work->timer > 0) {
        if (--work->timer == 0) {
            func_ov036_021999bc(task, RAIN_START_COLOR, RAIN_FADE_OUT_FRAMES, arg);
        }
    } else if ((arg ? func_ov036_021999dc(task) : TRUE) && done && !func_ov036_021997e8(task)) {
        return TRUE;
    }
    func_ov036_02199a78(task, &x, &y);
    func_ov036_02199b90(task, x, y);
    return FALSE;
}

static BOOL Rain_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199948(task, arg);
    func_ov036_0219974c(task, arg);
    return TRUE;
}

static void RainDrop_Move(WeatherObj *obj) {
    RainDrop *drop = func_ov036_02199c5c(obj);
    ClActor *actor = func_ov036_02199c64(obj);
    ClActorPos pos;

    func_ov036_02199c6c(obj, &pos);
    switch (drop->state) {
    case 0:
        pos.x += (s16)drop->speedX;
        pos.y += (s16)drop->speedY;
        drop->frame += 2;
        if (drop->frame > drop->splashFrame) {
            if (drop->splash < 7) {
                drop->state = 2;
            } else {
                drop->state = 1;
                drop->frame = 4;
                func_0204c504(actor, 3);
            }
        }
        func_ov036_02199cf0(obj, &pos);
        break;
    case 1:
        if (drop->frame-- <= 0) {
            drop->state = 2;
        }
        break;
    case 2:
        func_ov036_0219980c(obj);
        break;
    }
}

static void Rain_AddDrops(WeatherTask *task, int count, HeapID heapId) {
    int i;

    func_ov036_021997e4(task);
    for (i = 0; i < count; i++) {
        WeatherObj *obj;
        RainDrop *drop;
        ClActor *actor;
        u32 random;
        u32 size;
        int slant;
        ClActorPos pos;

        obj = func_ov036_021997f4(task, heapId);
        if (obj == NULL) {
            return;
        }
        drop = func_ov036_02199c5c(obj);
        actor = func_ov036_02199c64(obj);
        random = GFL_RandomLC(0);
        drop->frame = 0;
        size = random % 3;
        func_0204c504(actor, size);
        slant = random % 20;
        drop->speedY = slant + (size + 1) * 16;
        if (size == 2) {
            drop->speedY += 16;
        }
        drop->speedX = slant / 8 + (size + 1) * 8;
        if (size == 2) {
            drop->speedX += 8;
        }
        drop->state = 0;
        drop->splashFrame = (random & 1) + 1;
        pos.x = (u8)random + (size * 15 - 0x48);
        pos.y = -0x60;
        func_ov036_02199cf0(obj, &pos);
        drop->speedX *= 2;
        drop->speedY *= 2;
        drop->splash = GFL_RandomLC(10);
    }
}

static BOOL Thunder_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    ThunderWork *work = func_ov036_021997e4(task);

    Lightning_Init(&work->lightning, 2);
    func_ov036_02199824(task, 1, 15, 2, 0, -3, 1, 1, Thunder_AddDrops);
    func_ov036_0219990c(task, 9, RAIN_START_COLOR, arg);
    work->timer = 0;
    work->state = 0;
    func_ov036_02199a50(task);
    return TRUE;
}

static BOOL Thunder_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    ThunderWork *work = func_ov036_021997e4(task);

    switch (work->state) {
    case 0:
        func_ov036_02199734(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(6));
        work->state++;
        work->timer = RAIN_FADE_IN_FRAMES;
        break;
    case 1:
        if (--work->timer < 0) {
            work->state++;
        }
        break;
    case 2:
        func_ov036_02199780(task, 0x28c6, 8);
        work->timer = 16;
        work->state++;
        break;
    case 3:
        if (--work->timer < 0) {
            func_ov036_0219975c(task, 0x23de, 4, 60);
            func_ov036_02199c08(task, SEQ_SE_FLD_33);
            work->state++;
        }
        break;
    case 4:
        if (!func_ov036_02199768(task)) {
            func_ov036_02199958(task, 9, 0x7f80, 0x50, arg);
            work->state++;
            work->timer = 0x30;
        }
        break;
    case 5: {
        BOOL done;
        BOOL flashed;

        if (--work->timer == 0) {
            func_ov036_02199be8(task, SEQ_SE_FLD_31);
            func_ov036_0219978c(task, 1, 0);
        }
        done = func_ov036_02199868(task, heapId);
        flashed = func_ov036_021999dc(task);
        if (done && flashed) {
            return TRUE;
        }
        break;
    }
    }
    Thunder_ScrollBg(task, work);
    return FALSE;
}

static BOOL Thunder_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199824(task, 2, 0, 2, 0, -3, 1, 1, Thunder_AddDrops);
    func_ov036_0219990c(task, 9, 0x7f80, arg);
    func_ov036_02199880(task, Thunder_AddDrops, 20, 5, 1, heapId);
    func_ov036_02199be8(task, SEQ_SE_FLD_31);
    func_ov036_02199740(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(6));
    func_ov036_0219978c(task, 1, 0);
    return TRUE;
}

static BOOL Thunder_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    ThunderWork *work = func_ov036_021997e4(task);

    work->frame = (work->frame + 1) % 300;
    func_ov036_02199874(task, heapId);
    Lightning_Update(&work->lightning, task);
    Thunder_ScrollBg(task, work);
    return FALSE;
}

static BOOL Thunder_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    ThunderWork *work = func_ov036_021997e4(task);

    func_ov036_02199854(task, 0, 15, 3, -1);
    work->timer = 1;
    func_ov036_02199bf8(task);
    func_ov036_0219978c(task, 0, 0);
    return TRUE;
}

static BOOL Thunder_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    ThunderWork *work = func_ov036_021997e4(task);
    BOOL done = func_ov036_02199868(task, heapId);

    if (work->timer > 0) {
        if (--work->timer == 0) {
            func_ov036_021999bc(task, RAIN_START_COLOR, RAIN_FADE_OUT_FRAMES, arg);
        }
    } else if ((arg ? func_ov036_021999dc(task) : TRUE) && done && !func_ov036_021997e8(task)) {
        return TRUE;
    }
    Thunder_ScrollBg(task, work);
    return FALSE;
}

static BOOL Thunder_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    ThunderWork *work = func_ov036_021997e4(task);

    func_ov036_02199948(task, arg);
    func_ov036_0219974c(task, arg);
    Lightning_Exit(&work->lightning);
    return TRUE;
}

static void ThunderDrop_Move(WeatherObj *obj) {
    ThunderDrop *drop = func_ov036_02199c5c(obj);
    ClActorPos pos;

    func_ov036_02199c64(obj);
    func_ov036_02199c6c(obj, &pos);
    switch (drop->state) {
    case 0:
        pos.x += (s16)drop->speedX;
        pos.y += (s16)drop->speedY;
        if (pos.y >= 192) {
            drop->state = 1;
        }
        func_ov036_02199cf0(obj, &pos);
        break;
    case 1:
        func_ov036_0219980c(obj);
        break;
    }
}

static void Thunder_AddDrops(WeatherTask *task, int count, HeapID heapId) {
    int i;
    ThunderWork *work = func_ov036_021997e4(task);

    for (i = 0; i < count; i++) {
        WeatherObj *obj;
        ThunderDrop *drop;
        ClActor *actor;
        u32 random;
        u32 size;
        ClActorPos pos;

        obj = func_ov036_021997f4(task, heapId);
        if (obj == NULL) {
            return;
        }
        drop = func_ov036_02199c5c(obj);
        actor = func_ov036_02199c64(obj);
        random = GFL_RandomLC(0);
        size = random % 3;
        func_0204c504(actor, size);
        drop->speedX = (random & 15) + (size + 1) * 14;
        drop->speedY = drop->speedX;
        drop->speedX = sThunderSpeedScale[work->frame / 60] * drop->speedX / 10;
        drop->speedY = sThunderSpeedScale[work->frame / 60] * drop->speedY / 10;
        drop->state = 0;
        pos.x = random % 0x180 - 0x80;
        pos.y = random % 40 - 0x50;
        func_ov036_02199cf0(obj, &pos);
    }
}

static void Thunder_ScrollBg(WeatherTask *task, ThunderWork *work) {
    int x;
    int y;

    func_ov036_02199a78(task, &x, &y);
    func_ov036_02199b90(task, x, y);
    work->scroll = (work->scroll + 9) % 256;
    func_ov036_021997bc(task, -(work->scroll + x), 0);
    func_ov036_021997cc(task, y - work->scroll, 0);
}

static BOOL GustyRain_Init(WeatherTask *task, u32 arg, HeapID heapId) {
    GustWork *work = func_ov036_021997e4(task);

    func_ov036_02199824(task, 1, 4, 1, 1, -4, 1, 1, Gust_AddParticles);
    func_ov036_0219990c(task, 9, RAIN_START_COLOR, arg);
    func_ov036_02199a50(task);
    Gust_ResetRumble(work);
    return TRUE;
}

static BOOL GustyRain_FadeIn(WeatherTask *task, u32 arg, HeapID heapId) {
    GustWork *work = func_ov036_021997e4(task);
    BOOL done;
    BOOL flashed;

    switch (work->state) {
    case 0:
        func_ov036_02199734(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(7));
        work->timer = RAIN_FADE_IN_FRAMES;
        work->state++;
        break;
    case 1:
        if (--work->timer <= 0) {
            Gust_StartLull(task, work, arg, TRUE);
            work->limit = 0x78;
            work->timer = 8;
            work->state++;
        }
        break;
    case 2:
        if (--work->timer == 0) {
            func_ov036_02199be8(task, SEQ_SE_FLD_31);
            func_ov036_0219978c(task, 1, 0);
            func_ov036_02199958(task, 9, 0x7f80, 10, arg);
        }
        done = func_ov036_02199868(task, heapId);
        flashed = func_ov036_021999dc(task);
        if (done == TRUE && flashed == TRUE && work->timer <= 0) {
            return TRUE;
        }
        break;
    }
    Gust_Update(task, work, arg, FALSE);
    Gust_ScrollBgs(task, work);
    return FALSE;
}

static BOOL GustyRain_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId) {
    GustWork *work = func_ov036_021997e4(task);

    func_ov036_02199824(task, 1, 1, 1, 1, -4, 1, 1, Gust_AddParticles);
    func_ov036_0219990c(task, 9, 0x7f80, arg);
    func_ov036_02199880(task, Gust_AddParticles, 20, 5, 1, heapId);
    func_ov036_02199be8(task, SEQ_SE_FLD_31);
    Gust_StartLull(task, work, arg, FALSE);
    func_ov036_0219978c(task, 1, 0);
    func_ov036_02199740(task, WEATHER_SOUND_TYPE, FieldLightStatus_GetWeatherLight(7));
    return TRUE;
}

static BOOL GustyRain_Main(WeatherTask *task, u32 arg, HeapID heapId) {
    GustWork *work = func_ov036_021997e4(task);

    func_ov036_02199874(task, heapId);
    Gust_Update(task, work, arg, FALSE);
    Gust_ScrollBgs(task, work);
    return FALSE;
}

static BOOL GustyRain_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199854(task, 0, 4, 4, -1);
    func_ov036_02199bf8(task);
    func_ov036_021999bc(task, RAIN_START_COLOR, RAIN_FADE_OUT_FRAMES, arg);
    func_ov036_0219978c(task, 0, 0);
    func_ov036_0219978c(task, 0, 1);
    return TRUE;
}

static BOOL GustyRain_FadeOut(WeatherTask *task, u32 arg, HeapID heapId) {
    GustWork *work = func_ov036_021997e4(task);
    BOOL done = func_ov036_02199868(task, heapId);

    if ((arg ? func_ov036_021999dc(task) : TRUE) && done && !func_ov036_021997e8(task)) {
        return TRUE;
    }
    Gust_Update(task, work, arg, TRUE);
    Gust_ScrollBgs(task, work);
    return FALSE;
}

static BOOL GustyRain_Exit(WeatherTask *task, u32 arg, HeapID heapId) {
    func_ov036_021997e4(task);
    func_ov036_02199948(task, arg);
    func_ov036_0219974c(task, arg);
    return TRUE;
}

static void GustParticle_Move(WeatherObj *obj) {
    GustParticle *particle = func_ov036_02199c5c(obj);
    ClActor *actor = func_ov036_02199c64(obj);

    sGustParticleMove[particle->kind](obj, particle->work, &particle->data, actor);
}

static void Gust_AddParticles(WeatherTask *task, int count, HeapID heapId) {
    u32 random = GFL_RandomLC(0);
    GustWork *work = func_ov036_021997e4(task);
    int i;

    if (work->mode == 0) {
        return;
    }
    if (work->mode == 2 && (random & 1)) {
        return;
    }
    for (i = 0; i < count; i++) {
        WeatherObj *obj;
        GustParticle *particle;
        ClActor *actor;

        obj = func_ov036_021997f4(task, heapId);
        if (obj == NULL) {
            return;
        }
        particle = func_ov036_02199c5c(obj);
        actor = func_ov036_02199c64(obj);
        particle->work = work;
        switch (work->mode) {
        case 1:
            particle->kind = 1;
            break;
        case 2:
            if (random % 100 <= 50) {
                particle->kind = 2;
            } else {
                particle->kind = 0;
            }
            break;
        case 3:
        case 4:
            if (random % 100 <= 80) {
                particle->kind = 1;
            } else {
                particle->kind = 2;
            }
            break;
        }
        sGustParticleInit[particle->kind](obj, particle->work, &particle->data, actor);
    }
}

static void GustSlant_Move(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor) {
    s16 windX = work->windX;
    s16 windY;
    ClActorPos pos;

    if (windX == 0) {
        windX = data->baseX;
        windY = data->baseY;
    } else {
        windY = work->windY;
        data->baseX = windX;
        data->baseY = windY;
    }
    func_ov036_02199c6c(obj, &pos);
    data->x += data->speedX + windX;
    data->y += data->speedY + windY;
    pos.x = data->x / 8;
    pos.y = data->y / 8;
    if (pos.x >= 256 || pos.y >= 192) {
        func_ov036_0219980c(obj);
        return;
    }
    func_ov036_02199cf0(obj, &pos);
}

static void GustSlant_Init(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor) {
    u32 random = GFL_RandomLC(0);
    u32 r = random % 24;
    ClActorPos pos;
    s16 windX;

    data->speedX = r + 16;
    windX = work->windX;
    data->speedY = r + 8;
    data->baseX = windX;
    data->baseY = work->windY;
    pos.x = (random & 7) - 8;
    pos.y = random % 160 - 16;
    func_ov036_02199cf0(obj, &pos);
    data->x = pos.x << 3;
    data->y = pos.y << 3;
    func_0204c488(actor, 0);
    func_0204c520(actor, 1);
    func_0204c53c(actor, 1 << 13);
}

static void GustStreak_Move(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor) {
    ClActorPos pos;

    func_ov036_02199c6c(obj, &pos);
    data->x += data->speedX + work->windX;
    data->y += data->speedY + work->windY;
    pos.x = data->x / 8;
    pos.y = data->y / 8;
    if (pos.x >= 256 || pos.y >= 160) {
        func_ov036_0219980c(obj);
        return;
    }
    func_ov036_02199cf0(obj, &pos);
}

static void GustStreak_Init(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor) {
    u32 random = GFL_RandomLC(0);
    u32 r = random % 43;
    ClActorPos pos;

    data->speedX = r + 86;
    data->speedY = r * 2 + 258;
    pos.x = random % 336 - 80;
    pos.y = random % 40 - 80;
    func_ov036_02199cf0(obj, &pos);
    data->x = pos.x << 3;
    data->y = pos.y << 3;
    func_0204c488(actor, 1);
    func_0204c520(actor, 1);
    func_0204c53c(actor, 1 << 13);
}

static void GustDriven_Move(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor) {
    ClActorPos pos;
    s16 windX;

    func_ov036_02199c6c(obj, &pos);
    windX = work->windX;
    if (windX != 0) {
        s16 dx = data->speedX + windX;
        s16 dy = data->speedY + work->windY;

        data->x += dx;
        data->y += dy;
        pos.x = data->x / 8;
        pos.y = data->y / 8;
        data->baseX = dx;
        data->baseY = dy;
    } else {
        data->x += data->baseX;
        data->y += data->baseY;
        pos.x = data->x / 8;
        pos.y = data->y / 8;
    }
    if (pos.x >= 256 || pos.y >= 192) {
        func_ov036_0219980c(obj);
        return;
    }
    func_ov036_02199cf0(obj, &pos);
}

static void GustDriven_Init(WeatherObj *obj, GustWork *work, GustParticleData *data, ClActor *actor) {
    u32 random = GFL_RandomLC(0);
    u32 r = random % 24;
    ClActorPos pos;

    data->speedX = r + 80;
    data->speedY = r + 14;
    data->baseX = data->speedX + work->windX;
    data->baseY = data->speedY + work->windY;
    pos.x = (random & 7) - 8;
    pos.y = random % 160 - 16;
    func_ov036_02199cf0(obj, &pos);
    data->x = pos.x << 3;
    data->y = pos.y << 3;
    func_0204c488(actor, 2);
    func_0204c520(actor, 1);
    func_0204c53c(actor, 1 << 13);
}

static void Gust_StartLull(WeatherTask *task, GustWork *work, u32 arg, BOOL noFade) {
    u32 random = GFL_RandomLC(0);

    work->mode = 1;
    work->limit = random % 400 + 60;
    work->counter = 0;
    if (!noFade) {
        func_ov036_02199958(task, 9, 0x7f80, 10, arg);
        func_ov036_0219978c(task, 0, 1);
        func_ov036_02199be8(task, SEQ_SE_FLD_31);
    }
}

static void Gust_Update(WeatherTask *task, GustWork *work, u32 arg, BOOL noFade) {
    switch (work->mode) {
    case 1:
        work->counter++;
        Gust_PlayGustSound(task, work, arg, SEQ_SE_FLD_99);
        if (work->counter >= work->limit) {
            work->mode = 3;
            work->limit = 16;
            work->counter = 0;
            if (!noFade) {
                func_ov036_02199958(task, 9, 0x7f70, 10, arg);
                func_ov036_02199be8(task, SEQ_SE_FLD_32);
            }
        }
        break;
    case 2:
        work->counter++;
        Gust_PlayGustSound(task, work, arg, SEQ_SE_FLD_99);
        Gust_PlayRumble(task, work, arg);
        if (work->counter >= work->limit) {
            work->mode = 4;
            work->limit = 16;
            work->counter = 0;
            if (!noFade) {
                func_ov036_0219978c(task, 0, 1);
                func_ov036_0219979c(task, 0x1000, 0);
                func_ov036_021997ac(task, 0x1000, 0);
            }
        }
        break;
    case 3:
        work->counter++;
        if (work->counter >= work->limit) {
            u32 random = GFL_RandomLC(0);

            work->mode = 2;
            work->limit = random % 400 + 60;
            work->counter = 0;
            if (!noFade) {
                func_ov036_0219978c(task, 1, 1);
                func_ov036_0219979c(task, 0x800, 0);
                func_ov036_021997ac(task, 0x1800, 0);
            }
        }
        break;
    case 4:
        work->counter++;
        if (work->counter >= work->limit) {
            Gust_StartLull(task, work, arg, noFade);
        }
        break;
    }

    switch (work->mode) {
    case 1:
        if (work->limit - work->counter >= 9) {
            work->windX = 0;
            work->windY = 0;
        } else {
            work->windX = 0xa0;
            work->windY = 16;
        }
        break;
    case 2: {
        int index = work->counter % 72 * 6 / 72;

        work->windX = sGustWind[index].x;
        work->windY = sGustWind[index].y;
        work->bgScale = sGustBgScale[(u32)work->counter % 224 * 14 / 224];
        break;
    }
    case 3:
    case 4:
        work->windX = 0xa0;
        work->windY = 16;
        work->bgScale = 0x800;
        break;
    }
}

static void Gust_ScrollBgs(WeatherTask *task, GustWork *work) {
    int x;
    int y;

    func_ov036_02199a78(task, &x, &y);
    func_ov036_02199b90(task, x, y);
    if (work->windX != 0) {
        work->scrollX1 = (work->scrollX1 + 9) % 256;
        work->scrollY1 = (work->scrollY1 + 3) % 256;
        work->scrollX2 = (work->scrollX2 + 9) % 256;
        work->scrollY2 = (work->scrollY2 + 2) % 256;
    } else {
        work->scrollX1 = (work->scrollX1 + 3) % 256;
        work->scrollY1 = (work->scrollY1 + 8) % 256;
    }
    func_ov036_021997bc(task, -(work->scrollX1 + x), 0);
    func_ov036_021997cc(task, y - work->scrollY1, 0);
    func_ov036_021997bc(task, -(work->scrollX2 + x), 1);
    func_ov036_021997cc(task, y - work->scrollY2, 1);
    func_ov036_0219979c(task, work->bgScale, 1);
    func_ov036_021997ac(task, work->bgScale, 1);
}

static void Gust_PlayGustSound(WeatherTask *task, GustWork *work, u32 arg, u32 sound) {
    if (arg != 0 && work->counter == work->limit - 24) {
        func_ov036_02199c08(task, sound);
    }
}

static void Gust_ResetRumble(GustWork *work) {
    work->rumbleCounter = 0;
    work->rumbleLimit = GFL_RandomLC(16) + 50;
}

static void Gust_PlayRumble(WeatherTask *task, GustWork *work, u32 arg) {
    if (arg != 0 && work->counter <= work->limit - 74) {
        work->rumbleCounter++;
        if (work->rumbleCounter >= work->rumbleLimit) {
            if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_99))) {
                u32 delay = GFL_RandomLC(27) + 100;

                func_ov036_02199c30(task, SEQ_SE_FLD_98, delay);
                Gust_ResetRumble(work);
            } else {
                work->rumbleCounter = work->rumbleLimit - 10;
            }
        }
    }
}

static void Lightning_Init(Lightning *lightning, int kind) {
    sys_memset(lightning, 0, sizeof(Lightning));
    lightning->timer = GFL_RandomLC(3);
    lightning->kind = kind;
}

static void Lightning_Exit(Lightning *lightning) {
    sys_memset(lightning, 0, sizeof(Lightning));
}

static void Lightning_Update(Lightning *lightning, WeatherTask *task) {
    // The three setups below read a column of each of these tables as constants, one entry per kind of strike (the
    // sound's delay, the shade's base and range, the delay's base and range), which leaves the tables in the ROM
    static const s16 sDelayBase[3] = { 0x78, 0xf0, 0xf0 };
    static const s16 sDelayRange[3] = { 0xf0, 300, 300 };
    static const u8 sShadeBase[3] = { 0x12, 0x18, 0x1e };
    static const u8 sSoundDelay[3] = { 0x1c, 0xd, 0xa };
    static const u8 sOdds[3] = { 30, 75, 100 };
    static const u8 sShadeRange[3] = { 3, 3, 1 };

    switch (lightning->state) {
    case 0:
        if (--lightning->timer < 0) {
            lightning->state = 1;
        }
        break;
    case 1: {
        u32 random = GFL_RandomLC(100);
        int odds;

        for (odds = 0; odds < 3; odds++) {
            if (sOdds[odds] >= random) {
                break;
            }
        }
        if (lightning->kind == 0) {
            Lightning_SetupFlickers(lightning);
        } else {
            switch (odds) {
            case 0:
                if (lightning->kind == 1) {
                    Lightning_SetupFlickers(lightning);
                } else {
                    Lightning_SetupBolt(lightning);
                }
                break;
            case 1:
                Lightning_SetupBolt(lightning);
                break;
            case 2:
                Lightning_SetupForked(lightning);
                break;
            }
        }
        lightning->index = 0;
        lightning->state = 2;
        break;
    }
    case 2:
        if (lightning->kind != 2) {
            lightning->state = 4;
        } else {
            func_ov036_02199780(task, 0x3129, 4);
            lightning->timer = 10;
            lightning->state = 3;
        }
        break;
    case 3:
        if (--lightning->timer <= 0) {
            lightning->state = 4;
        }
        break;
    case 4:
        if (lightning->kind != 0) {
            func_ov036_0219975c(task, Lightning_GetShade(lightning), lightning->flashes[lightning->index].flashSpeed,
                                lightning->flashes[lightning->index].flashFrames);
        }
        if (lightning->type == 2) {
            lightning->timer = GFL_RandomLC(lightning->flashes[lightning->index].flashFrames) +
                               lightning->flashes[lightning->index].flashSpeed / 2;
        }
        lightning->soundDelay = lightning->flashes[lightning->index].soundDelay;
        lightning->sound = lightning->flashes[lightning->index].sound;
        if (lightning->kind == 2) {
            if (lightning->index + 1 == lightning->count) {
                lightning->flashes[lightning->index].delay /= 3;
            }
        } else if (lightning->kind == 0) {
            lightning->flashes[lightning->index].delay *= 2;
        }
        lightning->state = 5;
        break;
    case 5:
        if (lightning->soundDelay > 0) {
            if (--lightning->soundDelay == 0) {
                func_ov036_02199c08(task, lightning->sound);
            }
        }
        if (!func_ov036_02199768(task)) {
            lightning->state = 6;
        }
        break;
    case 6:
        if (lightning->soundDelay > 0) {
            if (--lightning->soundDelay == 0) {
                func_ov036_02199c08(task, lightning->sound);
            }
        }
        lightning->flashes[lightning->index].delay--;
        if (lightning->flashes[lightning->index].delay <= 0) {
            if (lightning->count > lightning->index + 1) {
                lightning->index++;
                lightning->state = 4;
            } else {
                lightning->state = 1;
            }
        }
        break;
    }
}

static void Lightning_SetupFlickers(Lightning *lightning) {
    int i;

    lightning->type = 0;
    lightning->count = GFL_RandomLC(2) + 1;
    for (i = 0; i < lightning->count; i++) {
        lightning->flashes[i].shade = GFL_RandomLC(3) + 0x12;
        lightning->flashes[i].flashSpeed = 4;
        lightning->flashes[i].flashFrames = sLightningFlashFrames[lightning->flashes[i].shade];
        lightning->flashes[i].delay = GFL_RandomLC(0xf0) + 0x78;
        lightning->flashes[i].soundDelay = 0x1c;
        lightning->flashes[i].sound = SEQ_SE_FLD_34;
    }
}

static void Lightning_SetupBolt(Lightning *lightning) {
    lightning->type = 1;
    lightning->count = 1;
    lightning->flashes[0].shade = GFL_RandomLC(3) + 0x18;
    lightning->flashes[0].flashSpeed = 4;
    lightning->flashes[0].flashFrames = sLightningFlashFrames[lightning->flashes[0].shade];
    lightning->flashes[0].delay = GFL_RandomLC(300) + 0xf0;
    lightning->flashes[0].soundDelay = 0xd;
    lightning->flashes[0].sound = SEQ_SE_FLD_33;
}

static void Lightning_SetupForked(Lightning *lightning) {
    int i;

    lightning->type = 2;
    lightning->count = GFL_RandomLC(2) + 1;
    lightning->count++;
    for (i = 0; i < lightning->count - 1; i++) {
        lightning->flashes[i].shade = GFL_RandomLC(3) + 0x12;
        lightning->flashes[i].flashSpeed = 4;
        lightning->flashes[i].flashFrames = 8;
        lightning->flashes[i].delay = GFL_RandomLC(4);
        lightning->flashes[i].soundDelay = 10;
        lightning->flashes[i].sound = SEQ_SE_FLD_34;
    }
    lightning->flashes[i].shade = GFL_RandomLC(1) + 0x1e;
    lightning->flashes[i].flashSpeed = 4;
    lightning->flashes[i].flashFrames = sLightningFlashFrames[lightning->flashes[i].shade];
    lightning->flashes[i].delay = GFL_RandomLC(300) + 0xf0;
    lightning->flashes[i].soundDelay = 10;
    lightning->flashes[i].sound = SEQ_SE_FLD_33;
}

static u16 Lightning_GetShade(Lightning *lightning) {
    u16 shade = lightning->flashes[lightning->index].shade;

    return shade | shade << 5 | 0x2000;
}
