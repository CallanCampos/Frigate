#ifndef POKEBW2_FIELD_FIELD_WEATHER_H
#define POKEBW2_FIELD_FIELD_WEATHER_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The field's weathers. Overlay 36 keeps the weather system, which loads an overlay with each weather's data:
// overlay 74 for weather 0, which the weather names call sunny, and overlays 75 to 78 for the others

// What a weather's functions get
typedef struct WeatherTask WeatherTask;

// One of a weather's particles, such as overlay 77's grains of sand
typedef struct WeatherObj WeatherObj;

// Called from FieldWeather_Update with the heap it was given
typedef BOOL (*WeatherFunc)(WeatherTask *task, u32 arg, HeapID heapId);
// Adds count particles
typedef void (*WeatherSpawnFunc)(WeatherTask *task, int count, HeapID heapId);

// The weather's graphics: its archive, whether it has particles, how many of the two background layers it uses, the
// files of the particles' characters, palette, cells and cell animations, and each layer's texture, as a
// field/field_3dbg.h Field3DBgRes
typedef struct {
    u16 arcId;
    u16 hasObjs;
    u16 bgCount;
    u16 charsFileId;
    u16 paletteFileId;
    u16 cellFileId;
    u16 cellAnimFileId;
    u16 bgRes[2][6];
    u16 unk26;
} WeatherGraphics;

typedef struct {
    WeatherGraphics graphics;
    // The size of the work that func_ov036_021997e4 returns
    u32 workSize;
    WeatherFunc funcs[7];
    // Moves one particle, every frame
    void (*objFunc)(WeatherObj *obj);
} WeatherData;

// Main: an entry of the u16 table at sWeatherLightIds, which the weather overlays pass as a sound
u16 FieldLightStatus_GetWeatherLight(u32 index);

// Overlay 36's weather_task.c: a weather's task, which runs the functions of its data. weather.c creates and frees
// two, updates them, starts a weather (mode 1 at once, 2 over the other task's) and ends one, and asks whether one is
// fading
WeatherTask *func_ov036_021993e0(ClActUnit *unit, FieldCamera *camera, FieldLight *light, FieldFog *fog,
                                 FieldFogCtrl *fogCtrl, Field3DBg **bgs, FieldSound *fieldSound, u16 season,
                                 HeapID heapId);
void func_ov036_02199424(WeatherTask *task);
void func_ov036_02199434(WeatherTask *task, HeapID heapId);
void func_ov036_02199618(WeatherTask *task, const WeatherData *data, u8 unk25, u32 fade, u32 mode, u32 ambience,
                         HeapID heapId);
void func_ov036_0219967c(WeatherTask *task, u32 fade, u32 mode);
void func_ov036_021996f4(WeatherTask *task);
u8 func_ov036_021997dc(WeatherTask *task);
// The index of the camera's defaults
u32 func_ov036_02199774(WeatherTask *task);
void *func_ov036_021997e4(WeatherTask *task);
void func_ov036_0219978c(WeatherTask *task, u32 a1, u32 index);
void func_ov036_021997bc(WeatherTask *task, int value, u32 index);
void func_ov036_021997cc(WeatherTask *task, int value, u32 index);
BOOL func_ov036_021997e8(WeatherTask *task);
WeatherObj *func_ov036_021997f4(WeatherTask *task, HeapID heapId);
void func_ov036_0219980c(WeatherObj *obj);
void func_ov036_02199824(WeatherTask *task, u32 a1, u32 a2, u32 a3, int a4, int a5, int a6, int a7,
                         WeatherSpawnFunc spawn);
void func_ov036_02199854(WeatherTask *task, u32 a1, u32 a2, u32 a3, int a4);
BOOL func_ov036_02199868(WeatherTask *task, HeapID heapId);
BOOL func_ov036_02199874(WeatherTask *task, HeapID heapId);
void func_ov036_02199880(WeatherTask *task, WeatherSpawnFunc spawn, int count, int a3, int a4, HeapID heapId);
void func_ov036_02199734(WeatherTask *task, u32 a1, u32 a2);
void func_ov036_0219975c(WeatherTask *task, u16 a1, u32 a2, u32 a3);
BOOL func_ov036_02199768(WeatherTask *task);
void func_ov036_02199780(WeatherTask *task, u16 a1, u16 a2);
void func_ov036_0219979c(WeatherTask *task, int value, u32 index);
void func_ov036_021997ac(WeatherTask *task, int value, u32 index);
void func_ov036_02199740(WeatherTask *task, u32 a1, u32 a2);
void func_ov036_0219974c(WeatherTask *task, u32 a1);
void func_ov036_0219990c(WeatherTask *task, u32 a1, u32 a2, u32 a3);
void func_ov036_02199948(WeatherTask *task, u32 a1);
void func_ov036_02199958(WeatherTask *task, u32 a1, u32 a2, int a3, u32 a4);
void func_ov036_021999bc(WeatherTask *task, u32 a1, u32 a2, u32 a3);
BOOL func_ov036_021999dc(WeatherTask *task);
BOOL func_ov036_02199a00(WeatherTask *task);
BOOL func_ov036_02199a14(WeatherTask *task);
BOOL func_ov036_02199a20(WeatherTask *task);
u32 func_ov036_02199a2c(WeatherTask *task);
u32 func_ov036_02199a38(WeatherTask *task);
u32 func_ov036_02199a44(WeatherTask *task);
void func_ov036_02199a50(WeatherTask *task);
// The camera's movement since the last frame, as a scroll
void func_ov036_02199a78(WeatherTask *task, int *x, int *y);
void func_ov036_02199b90(WeatherTask *task, int x, int y);
void func_ov036_02199be8(WeatherTask *task, u32 a1);
void func_ov036_02199bf8(WeatherTask *task);
void func_ov036_02199c08(WeatherTask *task, u32 sound);
void func_ov036_02199c30(WeatherTask *task, u32 sound, u32 a2);
void *func_ov036_02199c5c(WeatherObj *obj);
ClActor *func_ov036_02199c64(WeatherObj *obj);
void func_ov036_02199c6c(WeatherObj *obj, ClActorPos *pos);
void func_ov036_02199c80(WeatherObj *obj, const ClActorPos *pos);
void func_ov036_02199cf0(WeatherObj *obj, const ClActorPos *pos);

// Overlay 74
extern WeatherData SUNNY_WEATHER;
BOOL Sunny_Init(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_Main(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_Exit(WeatherTask *task, u32 arg, HeapID heapId);
void *func_ov036_02199004(FieldCamera *camera, void *light, FieldFog *fog, void *fogCtrl, FieldSound *sound, u16 season,
                          HeapID heapId);
void func_ov036_021990d8(FieldWeather *weather);
void FieldWeather_Update(void *weather, HeapID heapId);
void FieldWeather_Draw(FieldWeather *weather);
BOOL func_ov036_0219917c(FieldWeather *weather);
void SetWeatherInit(FieldWeather *weather, u16 weatherId, HeapID heapId);
void func_ov036_02199208(void *weather, u32 weatherId);
// Sets the weather at once, without a transition
void func_ov036_02199210(FieldWeather *weather, u32 weatherId);

#endif // POKEBW2_FIELD_FIELD_WEATHER_H
