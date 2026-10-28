#ifndef POKEBW2_FIELD_FIELD_EFFECT_H
#define POKEBW2_FIELD_FIELD_EFFECT_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/billboard_actor.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "struct_decls.h"

BillboardActorSys *Field_GetEffectBillboardActor(Field *field);
BillboardActorSys *Field_GetWildEffectBillboardActor(Field *field);
// The saving icon, and showing, hiding and freeing it
FieldSaveAnime *FieldSaveanime_Create(HeapID heapId, Field *field);
void FieldSaveanime_Start(FieldSaveAnime *effect);
void FieldSaveanime_Stop(FieldSaveAnime *effect);
void FieldSaveanime_Delete(FieldSaveAnime *effect);

// fldeff_emotion_bubble.c: an emotion bubble over an actor's head, by kind: "!", "?", a music note and "...", which has
// no sound
enum {
    EMOTION_BUBBLE_EXCLAMATION,
    EMOTION_BUBBLE_QUESTION,
    EMOTION_BUBBLE_NOTE,
    EMOTION_BUBBLE_ELLIPSIS,
    EMOTION_BUBBLE_KIND_COUNT,
};
// playSE plays the bubble's sound. The first function's bubble stays when it is done, until its owner ends it, and
// the second's ends itself. Then whether a bubble is done, which a NULL one is
FieldEffectTask *FldeffEmotionBubble_SetActor(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE);
FieldEffectTask *FldeffEmotionBubble_StartHeld(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE);
BOOL FldeffEmotionBubble_IsFinished(FieldEffectTask *task);
// Chargestone Cave's stones (field_gimmick_d06.c): the effect over a stone at pos for actor, and the one at a mark
// at pos. Then lighting a stone's effect, starting and the state of its other animation, and stopping a mark's
FieldEffectTask *StoneTask_StartFollow(FieldEffects *effects, const VecFx32 *pos, FieldActor *actor);
FieldEffectTask *StoneTask_StartPulse(FieldEffects *effects, const VecFx32 *pos);
void StoneTask_FollowFreezeLoop(FieldEffectTask *task);
void StoneTask_FollowStartEnding(FieldEffectTask *task);
u8 StoneTask_FollowIsEnded(FieldEffectTask *task);
void StoneTask_PulseTrigger(FieldEffectTask *task);
// Break the rock in front of an actor with Rock Smash
void FldeffRockSmash_Start(FieldActor *actor, FieldEffects *effects);
// The ripples of a fishing line cast in dir from pos, and how fast they play
FieldEffectTask *FldeffFishing_Start(FieldEffects *effects, const VecFx32 *pos, u32 dir, u32 sameHeight);
void FldeffFishing_SetAnimeSpeed(FieldEffectTask *task, u16 speed);
// The festival's sparkles: start their task, show one at pos, hide one or all, and whether one shows and its value
FieldEffectTask *FldeffFesSparkle_Start(FieldEffects *effects);
void FldeffFesSparkle_Show(FieldEffectTask *task, u8 idx, u16 value, const VecFx32 *pos);
void FldeffFesSparkle_Hide(FieldEffectTask *task, u8 idx);
void FldeffFesSparkle_HideAll(FieldEffectTask *task);
u16 FldeffFesSparkle_IsShown(FieldEffectTask *task, u8 idx);
u16 FldeffFesSparkle_GetValue(FieldEffectTask *task, u8 idx);
// The field effect of a phenomenon, which fldeff_encount.c plays
FieldEffectTask *EncountTask_Start(EncountSystem *system, FieldEffects *effects, u16 x, u16 z, fx32 height, u32 kind);
// Pause and hide a phenomenon
void EncountTask_SetPaused(FieldEffectTask *task, BOOL paused);
void EncountTask_SetDrawHidden(FieldEffectTask *task, BOOL hidden);
// The effects that actors make on the terrain
void FldeffShadow_Start(FieldActor *actor, FieldEffects *effects);
// The dust of an actor landing
void FldeffDust_Start(FieldActor *actor, FieldEffects *effects);
// The dust in front of an actor
void FldeffDust_StartFacing(FieldActor *actor, FieldEffects *effects);
void GrassTask_Start(FieldEffects *effects, FieldActor *actor, BOOL animate, int kind);
// fldeff_footmark.c: the marks an actor leaves, by kind: footprints in sand and snow, the tracks of actors whose
// footprint type is 2 in sand and snow, and tracks in deep sand. Tracks turn with the actor. The actor's map
// attributes pass FOOTMARK_KIND_MAX for no mark
enum {
    FOOTMARK_KIND_SAND,
    FOOTMARK_KIND_SAND_TRACKS,
    FOOTMARK_KIND_SNOW,
    FOOTMARK_KIND_SNOW_TRACKS,
    FOOTMARK_KIND_DEEP_SAND_TRACKS,
    FOOTMARK_KIND_MAX,
};
void Footmark_SetActor(FieldActor *actor, FieldEffects *effects, u32 kind);
// fldeff_reflect.c: an actor's reflection, by kind: in water, whose width swings, and in a mirror floor or ice, which
// stays still
enum {
    REFLECT_KIND_WATER = 1,
    REFLECT_KIND_MIRROR,
};
void ReflectTask_SetActor(MMSys *system, FieldActor *actor, FieldEffects *effects, u32 kind);
void FldeffRipple_Start(FieldEffects *effects, FieldActor *actor, u32 big, u32 atInit);
void FldeffSplash_AddModel(FieldEffects *effects, FieldActor *actor);
void D06Electric_StartSpark(FieldActor *actor, FieldEffects *effects);
void FldeffLeaves_Start(FieldEffects *effects, FieldActor *actor);

// field_effect.c: the effects a map has loaded, by ID, and the tasks that play them. An ID is the effect's slot in
// field_effect.c's table, named after the file whose load and free functions fill it
enum {
    FLDEFF_SHADOW,
    FLDEFF_DUST,
    FLDEFF_GRASS,
    FLDEFF_SURF_POKEMON,
    FLDEFF_EMOTION_BUBBLE,
    FLDEFF_FOOTMARK,
    FLDEFF_REFLECT,
    FLDEFF_RIPPLE,
    FLDEFF_SURF_POKEMON_SPLASH,
    FLDEFF_ROCK_SMASH,
    FLDEFF_D06_ELECTRIC,
    FLDEFF_CUT,
    FLDEFF_FISHING,
    FLDEFF_BTRAIN,
    FLDEFF_HIDE,
    FLDEFF_BUBBLE,
    FLDEFF_SPLASH,
    FLDEFF_LEAVES,
    FLDEFF_FES_SPARKLE,
    // fldeff_encount.c's phenomena, in the order of field_map.h's ENCOUNT_EFFECT_* that play them
    FLDEFF_ENCOUNT_GRASS,
    FLDEFF_ENCOUNT_REALLY_TALL_GRASS,
    FLDEFF_ENCOUNT_DUST_CLOUD,
    FLDEFF_ENCOUNT_WATER_0,
    FLDEFF_ENCOUNT_WATER_1,
    FLDEFF_ENCOUNT_FLYING_SHADOW,
    FLDEFF_MAX,
    // The ID of an empty slot, and of the empty last entry of the effects' table
    FLDEFF_NONE = FLDEFF_MAX,
};

typedef struct FieldEffectTaskStore FieldEffectTaskStore;

typedef void (*FieldEffectTaskFunc)(FieldEffectTask *task, void *work);

// What a task does when it starts, ends, updates and draws
typedef struct {
    // The size of the task's work
    u32 workSize;
    FieldEffectTaskFunc init;
    FieldEffectTaskFunc delete;
    FieldEffectTaskFunc update;
    FieldEffectTaskFunc draw;
} FieldEffectTaskVTable;

struct FieldEffectTask {
    BOOL active;
    u32 mode;
    void *params;
    VecFx32 pos;
    u8 work[0xa0];
    FieldEffectTaskVTable vtable;
    FieldEffectTaskStore *store;
    TCB *tcb;
};

FieldEffects *FieldEffects_Create(Field *field, int count, HeapID heapId);
void FieldEffects_Free(FieldEffects *effects);
void FieldEffects_Update(FieldEffects *effects);
void FieldEffects_Draw(FieldEffects *effects);
Field *FieldEffects_GetField(FieldEffects *effects);
// The field effects' 3D object system, archive, season and whether the area is outdoors
struct FieldG3DObjSystem *FieldEffects_GetFieldMapData(FieldEffects *effects);
ArcTool *FieldEffects_GetArc(FieldEffects *effects);
u32 FieldEffects_GetSeason(FieldEffects *effects);
BOOL FieldEffects_GetAreaIsExterior(FieldEffects *effects);
// Load count effects by ID, free one, and whether one is loaded and its data
void FieldEffects_Load(FieldEffects *effects, const u32 *ids, u32 count);
void FieldEffects_FreeEffect(FieldEffects *effects, u32 id);
BOOL FieldEffects_IsLoaded(FieldEffects *effects, u32 id);
void *FieldEffects_GetHandleData(FieldEffects *effects, u32 id);
void FieldEffects_TCBManagerInit(FieldEffects *effects, u32 count);
// Start a task, at pos if it isn't NULL
FieldEffectTask *FieldEffects_TCBCreate(FieldEffects *effects, const FieldEffectTaskVTable *vtable, const VecFx32 *pos,
                                        u32 mode, void *params, u32 priority);
// End a task, update it, and its parameters, position and work
void FieldEffects_DeleteObject(FieldEffectTask *task);
void FieldEffects_UpdateObject(FieldEffectTask *task);
u32 FieldEffects_GetObjectMode(FieldEffectTask *task);
void *FieldEffects_GetObjectParam(FieldEffectTask *task);
void FieldEffects_GetObjectPos(FieldEffectTask *task, VecFx32 *pos);
void FieldEffects_SetObjectPos(FieldEffectTask *task, const VecFx32 *pos);
void *FieldEffects_GetObjectWork(FieldEffectTask *task);
void FieldEffects_SetLuminanceTable(FieldEffects *effects, const u8 *table);
void FieldEffects_ApplyLuminanceTable(FieldEffects *effects, G3DResource *resource);

// fldeff_surf_pokemon.c: the Pokémon an actor surfs on, facing dir at pos, and how it follows the actor, which can
// change; and whether its wake shows
enum {
    // It stays where it is
    SURF_POKEMON_FOLLOW_NONE,
    // It bobs under the actor, which it lifts by its draw offset
    SURF_POKEMON_FOLLOW_BOB,
    // It follows the actor and leaves the actor's draw offset alone, without its wake
    SURF_POKEMON_FOLLOW_KEEP_OFFSET,
};
FieldEffectTask *FldeffSurfPokemon_Start(FieldEffects *effects, u16 dir, const VecFx32 *pos, FieldActor *actor,
                                         u32 mode);
void FldeffSurfPokemon_SetFollowMode(FieldEffectTask *task, u8 mode);
void FldeffSurfPokemon_SetWakeEnabled(FieldEffectTask *task, BOOL showWake);
// A splash of a kind that follows the surfed Pokémon's task, or one at pos that ends with its animation, and
// whether a splash's animation has ended
FieldEffectTask *FldeffSurfPokemonSplash_StartOnEffect(FieldEffects *effects, u32 kind, FieldEffectTask *parent);
FieldEffectTask *FldeffSurfPokemonSplash_StartAtPos(FieldEffects *effects, u32 kind, const VecFx32 *pos);
BOOL FldeffSurfPokemonSplash_IsFinished(FieldEffectTask *task);
// The effects that every map loads, and their count
extern const u32 STATIC_LOADED_FIELD_EFFECT_IDS[21];
extern const u32 STATIC_LOADED_FIELD_EFFECT_COUNT;

// Each effect's load and free functions, in their own files
void *FieldEffect_Shadow_Create(FieldEffects *effects, HeapID heapId);
void FieldEffect_Shadow_Free(FieldEffects *effects, void *data);
void *playSmokeEffect(FieldEffects *effects, HeapID heapId);
void freeSmokeEffect(FieldEffects *effects, void *data);
void *loadGrassEffects(FieldEffects *effects, HeapID heapId);
void freeGrassEffects(FieldEffects *effects, void *data);
void *FldeffSurfPokemon_Create(FieldEffects *effects, HeapID heapId);
void FldeffSurfPokemon_Delete(FieldEffects *effects, void *data);
void *FldeffEmotionBubble_Create(FieldEffects *effects, HeapID heapId);
void FldeffEmotionBubble_Delete(FieldEffects *effects, void *data);
void *Footmark_Init(FieldEffects *effects, HeapID heapId);
void Footmark_Delete(FieldEffects *effects, void *data);
void *FldeffReflect_Create(FieldEffects *effects, HeapID heapId);
void FldeffReflect_Delete(FieldEffects *effects, void *data);
void *FldeffRipple_Create(FieldEffects *effects, HeapID heapId);
void FldeffRipple_Delete(FieldEffects *effects, void *data);
void *FldeffSurfPokemonSplash_Create(FieldEffects *effects, HeapID heapId);
void FldeffSurfPokemonSplash_Delete(FieldEffects *effects, void *data);
void *FldeffRockSmash_Create(FieldEffects *effects, HeapID heapId);
void FldeffRockSmash_Delete(FieldEffects *effects, void *data);
void *StoneTask_Create(FieldEffects *effects, HeapID heapId);
void D06Electric_Delete(FieldEffects *effects, void *data);
void *FldeffCut_Create(FieldEffects *effects, HeapID heapId);
void FldeffCut_Delete(FieldEffects *effects, void *data);
void *FldeffFishing_Create(FieldEffects *effects, HeapID heapId);
void FldeffFishing_Delete(FieldEffects *effects, void *data);
void *FieldEffect_BTrain_Create(FieldEffects *effects, HeapID heapId);
void FieldEffect_BTrain_Free(FieldEffects *effects, void *data);
void *Hide_CreateResources(FieldEffects *effects, HeapID heapId);
void Hide_DeleteResources(FieldEffects *effects, void *data);
// fldeff_hide.c: the object an actor hides in, of a kind (0 to 2); the start of its animation and sound; and whether
// its animation has ended, which it has for a NULL task
FieldEffectTask *HideTask_Start(FieldEffects *effects, FieldActor *actor, u32 kind);
void HideTask_RequestEnd(FieldEffectTask *task);
u16 HideTask_IsFinished(FieldEffectTask *task);
// The bubbles that rise from an actor in the water
void FldeffBubble_Start(FieldActor *actor, FieldEffects *effects);
void *FldeffBubble_CreateRes(FieldEffects *effects, HeapID heapId);
void FldeffBubble_DeleteRes(FieldEffects *effects, void *data);
void *FldeffSplash_CreateModelRes(FieldEffects *effects, HeapID heapId);
void FldeffSplash_DeleteModelRes(FieldEffects *effects, void *data);
void *FldeffLeaves_Create(FieldEffects *effects, HeapID heapId);
void FldeffLeaves_Delete(FieldEffects *effects, void *data);
void *FldeffFesSparkle_Create(FieldEffects *effects, HeapID heapId);
void FldeffFesSparkle_Delete(FieldEffects *effects, void *data);
void *EncountTask_CreateGrass(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateReallyTallGrass(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateDustCloud(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateWater0(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateWater1(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateFlyingShadow(FieldEffects *effects, HeapID heapId);
void EncountTask_Delete(FieldEffects *effects, void *data);

#endif // POKEBW2_FIELD_FIELD_EFFECT_H
