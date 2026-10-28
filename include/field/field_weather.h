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
    // The size of the work that WeatherTask_GetWorkBuffer returns
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
WeatherTask *WeatherTask_Create(ClActUnit *unit, FieldCamera *camera, FieldLight *light, FieldFog *fog,
                                FieldFogCtrl *fogCtrl, Field3DBg **bgs, FieldSound *fieldSound, u16 season,
                                HeapID heapId);
void WeatherTask_Delete(WeatherTask *task);
void WeatherTask_Update(WeatherTask *task, HeapID heapId);
void WeatherTask_Start(WeatherTask *task, const WeatherData *data, u8 unk25, u32 fade, u32 mode, u32 ambience,
                       HeapID heapId);
void WeatherTask_RequestEnd(WeatherTask *task, u32 fade, u32 mode);
void WeatherTask_ForceEnd(WeatherTask *task);
u8 WeatherTask_GetPhase(WeatherTask *task);
// The index of the camera's defaults
u32 WeatherTask_GetCameraSettingId(WeatherTask *task);
void *WeatherTask_GetWorkBuffer(WeatherTask *task);
void WeatherTask_SetBillboardVisible(WeatherTask *task, u32 a1, u32 index);
void WeatherTask_SetBillboardTexOffsetX(WeatherTask *task, int value, u32 index);
void WeatherTask_SetBillboardTexOffsetY(WeatherTask *task, int value, u32 index);
BOOL WeatherTask_GetParticleCount(WeatherTask *task);
WeatherObj *WeatherTask_AddParticle(WeatherTask *task, HeapID heapId);
void WeatherTask_RemoveParticle(WeatherObj *obj);
void WeatherTask_SetupSpawner(WeatherTask *task, u32 a1, u32 a2, u32 a3, int a4, int a5, int a6, int a7,
                              WeatherSpawnFunc spawn);
void WeatherTask_RetargetSpawner(WeatherTask *task, u32 a1, u32 a2, u32 a3, int a4);
BOOL WeatherTask_StepSpawner(WeatherTask *task, HeapID heapId);
BOOL WeatherTask_StepSpawnTimer(WeatherTask *task, HeapID heapId);
void WeatherTask_PrewarmParticles(WeatherTask *task, WeatherSpawnFunc spawn, int count, int a3, int a4, HeapID heapId);
void WeatherTask_FadeLight(WeatherTask *task, u32 a1, u32 a2);
void WeatherTask_FlashLight(WeatherTask *task, u16 a1, u32 a2, u32 a3);
BOOL WeatherTask_IsLightFlashing(WeatherTask *task);
void WeatherTask_SetLightColor(WeatherTask *task, u16 a1, u16 a2);
void WeatherTask_SetBillboardScaleX(WeatherTask *task, int value, u32 index);
void WeatherTask_SetBillboardScaleY(WeatherTask *task, int value, u32 index);
void WeatherTask_SetLight(WeatherTask *task, u32 a1, u32 a2);
void WeatherTask_ResetLight(WeatherTask *task, u32 a1);
void WeatherTask_InitFog(WeatherTask *task, u32 a1, u32 a2, u32 a3);
void WeatherTask_DisableFog(WeatherTask *task, u32 a1);
void WeatherTask_ChangeFog(WeatherTask *task, u32 a1, u32 a2, int a3, u32 a4);
void WeatherTask_ChangeFogOffset(WeatherTask *task, u32 a1, u32 a2, u32 a3);
BOOL WeatherTask_IsFogIdle(WeatherTask *task);
BOOL WeatherTask_IsZoneFogActive(WeatherTask *task);
BOOL WeatherTask_IsZoneFog(WeatherTask *task);
BOOL WeatherTask_IsZoneFogLight(WeatherTask *task);
u32 WeatherTask_GetZoneFogOffset(WeatherTask *task);
u32 WeatherTask_GetZoneFogSlope(WeatherTask *task);
u32 WeatherTask_GetZoneFogLightId(WeatherTask *task);
void WeatherTask_SaveCameraPos(WeatherTask *task);
// The camera's movement since the last frame, as a scroll
void WeatherTask_GetCameraScroll(WeatherTask *task, int *x, int *y);
void WeatherTask_ShiftParticles(WeatherTask *task, int x, int y);
void WeatherTask_PlaySe(WeatherTask *task, u32 a1);
void WeatherTask_StopSe(WeatherTask *task);
void WeatherTask_PlaySeDirect(WeatherTask *task, u32 sound);
void WeatherTask_PlaySeWithVoice(WeatherTask *task, u32 sound, u32 a2);
void *WeatherTask_GetParticleWork(WeatherObj *obj);
ClActor *WeatherTask_GetParticleActor(WeatherObj *obj);
void WeatherTask_GetParticlePos(WeatherObj *obj, ClActorPos *pos);
void WeatherTask_SetParticlePosWrapped(WeatherObj *obj, const ClActorPos *pos);
void WeatherTask_SetParticlePos(WeatherObj *obj, const ClActorPos *pos);

// Overlay 74
extern WeatherData SUNNY_WEATHER;
BOOL Sunny_Init(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_FadeIn(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_InitNoFade(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_Main(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_InitFadeOut(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_FadeOut(WeatherTask *task, u32 arg, HeapID heapId);
BOOL Sunny_Exit(WeatherTask *task, u32 arg, HeapID heapId);
FieldWeather *FieldWeather_Create(FieldCamera *camera, FieldLight *light, FieldFog *fog, FieldFogCtrl *fogCtrl,
                                  FieldSound *sound, u16 season, HeapID heapId);
void FieldWeather_Delete(FieldWeather *weather);
void FieldWeather_Update(FieldWeather *weather, HeapID heapId);
void FieldWeather_Draw(FieldWeather *weather);
BOOL FieldWeather_IsTaskActive(FieldWeather *weather);
void SetWeatherInit(FieldWeather *weather, u16 weatherId, HeapID heapId);
void FieldWeather_RequestChange(FieldWeather *weather, u32 weatherId);
// Sets the weather at once, without a transition
void FieldWeather_RequestChangeNoFade(FieldWeather *weather, u32 weatherId);

#endif // POKEBW2_FIELD_FIELD_WEATHER_H
