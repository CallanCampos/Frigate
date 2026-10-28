#ifndef POKEBW2_FIELD_FIELD_EFFECT_H
#define POKEBW2_FIELD_FIELD_EFFECT_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/blact.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "struct_decls.h"

BlActSys *Field_GetEffectBlAct(Field *field);
BlActSys *Field_GetWildEffectBlAct(Field *field);
// The saving icon, and showing, hiding and freeing it
FieldSaveAnime *FieldSaveanime_Create(HeapID heapId, Field *field);
void FieldSaveanime_Start(FieldSaveAnime *effect);
void FieldSaveanime_Stop(FieldSaveAnime *effect);
void FieldSaveanime_Delete(FieldSaveAnime *effect);

// fldeff_gyoe.c: an emotion bubble over an actor's head, by kind: "!", "?", a music note and "...", which has no
// sound. playSE plays the bubble's sound. The first function's bubble stays when it is done, until its owner ends it,
// and the second's ends itself. Then whether a bubble is done, which a NULL one is
FieldEffectTask *FldeffGyoe_SetMMdl(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE);
FieldEffectTask *FldeffGyoe_StartHeld(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE);
BOOL FldeffGyoe_IsFinished(FieldEffectTask *task);
// Chargestone Cave's stones (field_gimmick_d06.c): the effect over a stone at pos for actor, and the one at a mark
// at pos. Then lighting a stone's effect, starting and the state of its other animation, and stopping a mark's
FieldEffectTask *StoneTask_StartFollow(FieldEffects *effects, const VecFx32 *pos, FieldActor *actor);
FieldEffectTask *StoneTask_StartPulse(FieldEffects *effects, const VecFx32 *pos);
void StoneTask_FollowFreezeLoop(FieldEffectTask *task);
void StoneTask_FollowStartEnding(FieldEffectTask *task);
u8 StoneTask_FollowIsEnded(FieldEffectTask *task);
void StoneTask_PulseTrigger(FieldEffectTask *task);
// Break the rock in front of an actor with Rock Smash
void FldeffIwakudaki_Start(FieldActor *actor, FieldEffects *effects);
// The ripples of a fishing line cast in dir from pos, and how fast they play
FieldEffectTask *FldeffFishing_Start(FieldEffects *effects, const VecFx32 *pos, u32 dir, u32 sameHeight);
void FldeffFishing_SetAnimeSpeed(FieldEffectTask *task, u16 speed);
// The festival's sparkles: start their task, show one at pos, hide one or all, and whether one shows and its value
FieldEffectTask *FldeffFesKira_Start(FieldEffects *effects);
void FldeffFesKira_Show(FieldEffectTask *task, u8 idx, u16 value, const VecFx32 *pos);
void FldeffFesKira_Hide(FieldEffectTask *task, u8 idx);
void FldeffFesKira_HideAll(FieldEffectTask *task);
u16 FldeffFesKira_IsShown(FieldEffectTask *task, u8 idx);
u16 FldeffFesKira_GetValue(FieldEffectTask *task, u8 idx);
// The field effect of a phenomenon, which fldeff_encount.c plays
FieldEffectTask *EncountTask_Start(EncountSystem *system, FieldEffects *effects, u16 x, u16 z, fx32 height, u32 kind);
// Pause and hide a phenomenon
void EncountTask_SetPaused(FieldEffectTask *task, BOOL paused);
void EncountTask_SetDrawHidden(FieldEffectTask *task, BOOL hidden);
// The effects that actors make on the terrain
void FldeffShadow_Start(FieldActor *actor, FieldEffects *effects);
// The dust of an actor landing
void FldeffKemuri_Start(FieldActor *actor, FieldEffects *effects);
// The dust in front of an actor
void FldeffKemuri_StartFacing(FieldActor *actor, FieldEffects *effects);
void GrassTask_Start(FieldEffects *effects, FieldActor *actor, BOOL animate, int kind);
// fldeff_footmark.c: the marks an actor leaves, by kind: footprints in sand (0) and snow (2), the tracks of actors
// whose footprint type is 2 in sand (1) and snow (3), and tracks in deep sand (4). Tracks turn with the actor
void Footmark_SetMMdl(FieldActor *actor, FieldEffects *effects, u32 kind);
void ReflectTask_SetMMdl(MMSys *system, FieldActor *actor, FieldEffects *effects, u32 kind);
void FldeffRipple_Start(FieldEffects *effects, FieldActor *actor, u32 arg2, u32 arg3);
void func_ov036_021bea3c(FieldEffects *effects, FieldActor *actor);
void func_ov036_021c289c(FieldActor *actor, FieldEffects *effects);
void func_ov036_021c94e0(FieldEffects *effects, FieldActor *actor);

// field_effect.c: the effects a map has loaded, by ID, and the tasks that play them
enum {
    FLDEFF_MAX = 25,
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
    u32 param1;
    void *param2;
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
                                        u32 param1, void *param2, u32 priority);
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

// fldeff_namipoke.c: the Pokémon an actor surfs on, facing dir at pos, and how it follows the actor (0: it doesn't, 1:
// it bobs under the actor, 2: the actor's offset is left alone), which can change; and whether its wake shows
FieldEffectTask *FldeffNamipoke_Start(FieldEffects *effects, u16 dir, const VecFx32 *pos, FieldActor *actor, u32 mode);
void FldeffNamipoke_SetFollowMode(FieldEffectTask *task, u8 mode);
void FldeffNamipoke_SetWakeEnabled(FieldEffectTask *task, BOOL showWake);
// A splash of a kind that follows the surfed Pokémon's task, or one at pos that ends with its animation, and
// whether a splash's animation has ended
FieldEffectTask *FldeffNamipokeSplash_StartOnEffect(FieldEffects *effects, u32 kind, FieldEffectTask *parent);
FieldEffectTask *FldeffNamipokeSplash_StartAtPos(FieldEffects *effects, u32 kind, const VecFx32 *pos);
BOOL FldeffNamipokeSplash_IsFinished(FieldEffectTask *task);
// The effects that every map loads, and their count
extern const u32 STATIC_LOADED_FIELD_EFFECT_IDS[21];
extern const u32 data_ov036_021d0388;

// Each effect's load and free functions, in their own files
void *FieldEffect_Shadow_Create(FieldEffects *effects, HeapID heapId);
void FieldEffect_Shadow_Free(FieldEffects *effects, void *data);
void *playSmokeEffect(FieldEffects *effects, HeapID heapId);
void freeSmokeEffect(FieldEffects *effects, void *data);
void *loadGrassEffects(FieldEffects *effects, HeapID heapId);
void freeGrassEffects(FieldEffects *effects, void *data);
void *FldeffNamipoke_Create(FieldEffects *effects, HeapID heapId);
void FldeffNamipoke_Delete(FieldEffects *effects, void *data);
void *FldeffGyoe_Create(FieldEffects *effects, HeapID heapId);
void FldeffGyoe_Delete(FieldEffects *effects, void *data);
void *Footmark_Init(FieldEffects *effects, HeapID heapId);
void Footmark_Delete(FieldEffects *effects, void *data);
void *FldeffReflect_Create(FieldEffects *effects, HeapID heapId);
void FldeffReflect_Delete(FieldEffects *effects, void *data);
void *FldeffRipple_Create(FieldEffects *effects, HeapID heapId);
void FldeffRipple_Delete(FieldEffects *effects, void *data);
void *FldeffNamipokeSplash_Create(FieldEffects *effects, HeapID heapId);
void FldeffNamipokeSplash_Delete(FieldEffects *effects, void *data);
void *FldeffIwakudaki_Create(FieldEffects *effects, HeapID heapId);
void FldeffIwakudaki_Delete(FieldEffects *effects, void *data);
void *StoneTask_Create(FieldEffects *effects, HeapID heapId);
void func_ov036_021c27ac(FieldEffects *effects, void *data);
void *FldeffIaigiri_Create(FieldEffects *effects, HeapID heapId);
void func_ov036_021c2e00(FieldEffects *effects, void *data);
void *FldeffFishing_Create(FieldEffects *effects, HeapID heapId);
void FldeffFishing_Delete(FieldEffects *effects, void *data);
void *FieldEffect_BTrain_Create(FieldEffects *effects, HeapID heapId);
void FieldEffect_BTrain_Free(FieldEffects *effects, void *data);
void *func_ov036_021c66e8(FieldEffects *effects, HeapID heapId);
void func_ov036_021c670c(FieldEffects *effects, void *data);
// fldeff_hide.c: the object an actor hides in, of a kind (0 to 2); the start of its animation and sound; and whether
// its animation has ended, which it has for a NULL task
FieldEffectTask *HideTask_Start(FieldEffects *effects, FieldActor *actor, u32 kind);
void HideTask_RequestEnd(FieldEffectTask *task);
u16 HideTask_IsFinished(FieldEffectTask *task);
// The bubbles that rise from an actor in the water
void FldeffBubble_Start(FieldActor *actor, FieldEffects *effects);
void *FldeffBubble_CreateRes(FieldEffects *effects, HeapID heapId);
void func_ov036_021c6990(FieldEffects *effects, void *data);
void *func_ov036_021be988(FieldEffects *effects, HeapID heapId);
void func_ov036_021be9ac(FieldEffects *effects, void *data);
void *func_ov036_021c944c(FieldEffects *effects, HeapID heapId);
void func_ov036_021c9470(FieldEffects *effects, void *data);
void *FldeffFesKira_Create(FieldEffects *effects, HeapID heapId);
void FldeffFesKira_Delete(FieldEffects *effects, void *data);
void *EncountTask_CreateGrass(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateReallyTallGrass(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateDustCloud(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateWater0(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateWater1(FieldEffects *effects, HeapID heapId);
void *EncountTask_CreateFlyingShadow(FieldEffects *effects, HeapID heapId);
void EncountTask_Delete(FieldEffects *effects, void *data);

#endif // POKEBW2_FIELD_FIELD_EFFECT_H
