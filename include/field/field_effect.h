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
void *func_ov036_021c6cc8(HeapID heapId, Field *field);
void func_ov036_021c6d14(FieldSaveAnime *effect);
void func_ov036_021c6d3c(FieldSaveAnime *effect);
void func_ov036_021c6cf8(FieldSaveAnime *effect);

// fldeff_gyoe.c: an emotion bubble over an actor's head, by kind: "!", "?", a music note and "...", which has no
// sound. playSE plays the bubble's sound. The first function's bubble stays when it is done, until its owner ends it,
// and the second's ends itself. Then whether a bubble is done, which a NULL one is
FieldEffectTask *func_ov036_021b3f14(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE);
FieldEffectTask *func_ov036_021b3f64(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE);
BOOL func_ov036_021b3fb4(FieldEffectTask *task);
// Chargestone Cave's stones (field_gimmick_d06.c): the effect over a stone at pos for actor, and the one at a mark
// at pos. Then lighting a stone's effect, starting and the state of its other animation, and stopping a mark's
FieldEffectTask *func_ov036_021c2984(FieldEffects *effects, const VecFx32 *pos, FieldActor *actor);
FieldEffectTask *func_ov036_021c2b78(FieldEffects *effects, const VecFx32 *pos);
void func_ov036_021c29bc(FieldEffectTask *task);
void func_ov036_021c29cc(FieldEffectTask *task);
u8 func_ov036_021c29f0(FieldEffectTask *task);
void func_ov036_021c2bac(FieldEffectTask *task);
// Break the rock in front of an actor with Rock Smash
void func_ov036_021a56c8(FieldActor *actor, FieldEffects *effects);
// The ripples of a fishing line cast in dir from pos, and how fast they play
FieldEffectTask *func_ov036_021a58e0(FieldEffects *effects, const VecFx32 *pos, u32 dir, u32 sameHeight);
void func_ov036_021a5968(FieldEffectTask *task, u16 speed);
// The festival's sparkles: start their task, show one at pos, hide one or all, and whether one shows and its value
FieldEffectTask *func_ov036_021a5bb4(FieldEffects *effects);
void func_ov036_021a5c04(FieldEffectTask *task, u8 idx, u16 value, const VecFx32 *pos);
void func_ov036_021a5c2c(FieldEffectTask *task, u8 idx);
void func_ov036_021a5c44(FieldEffectTask *task);
u16 func_ov036_021a5c5c(FieldEffectTask *task, u8 idx);
u16 func_ov036_021a5c74(FieldEffectTask *task, u8 idx);
// The field effect of a phenomenon, which fldeff_encount.c plays
FieldEffectTask *func_ov036_021a53f8(EncountSystem *system, FieldEffects *effects, u16 x, u16 z, fx32 height, u32 kind);
// Pause and hide a phenomenon
void func_ov036_021a5498(FieldEffectTask *task, BOOL paused);
void func_ov036_021a54a8(FieldEffectTask *task, BOOL hidden);
// The effects that actors make on the terrain
void func_ov036_021a3bf0(FieldActor *actor, FieldEffects *effects);
// The dust of an actor landing
void func_ov036_021a3e74(FieldActor *actor, FieldEffects *effects);
// The dust in front of an actor
void func_ov036_021a3ec4(FieldActor *actor, FieldEffects *effects);
void func_ov036_021a40ac(FieldEffects *effects, FieldActor *actor, BOOL animate, int kind);
// fldeff_footmark.c: the marks an actor leaves, by kind: footprints in sand (0) and snow (2), the tracks of actors
// whose footprint type is 2 in sand (1) and snow (3), and tracks in deep sand (4). Tracks turn with the actor
void func_ov036_021b47c8(FieldActor *actor, FieldEffects *effects, u32 kind);
void func_ov036_021b49ac(MMSys *system, FieldActor *actor, void *effects, u32 kind);
void func_ov036_021be828(void *effects, FieldActor *actor, u32 arg2, u32 arg3);
void func_ov036_021bea3c(void *effects, FieldActor *actor);
void func_ov036_021c289c(FieldActor *actor, void *effects);
void func_ov036_021c94e0(void *effects, FieldActor *actor);

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
#ifdef PLATFORM_PC
    // The tasks' work holds pointers, which a host aligns to 8 bytes; after param2 and pos it would start 4 bytes past
    // such a boundary. Every task's work fits the 0xa0 bytes on a host too (the largest, FesKiraTask, is 0x98)
    u8 work[0xa0] __attribute__((aligned(8)));
#else
    u8 work[0xa0];
#endif
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
struct FieldG3DObjSystem *func_ov036_021a3724(FieldEffects *effects);
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
void func_ov036_021a3a70(FieldEffectTask *task);
void func_ov036_021a3a94(FieldEffectTask *task);
u32 func_ov036_021a3abc(FieldEffectTask *task);
void *func_ov036_021a3ac8(FieldEffectTask *task);
void func_ov036_021a3ad4(FieldEffectTask *task, VecFx32 *pos);
void func_ov036_021a3ae8(FieldEffectTask *task, const VecFx32 *pos);
void *func_ov036_021a3afc(FieldEffectTask *task);
void FieldEffects_SetLuminanceTable(FieldEffects *effects, const u8 *table);
void FieldEffects_ApplyLuminanceTable(FieldEffects *effects, void *resource);

// fldeff_namipoke.c: the Pokémon an actor surfs on, facing dir at pos, and how it follows the actor (0: it doesn't, 1:
// it bobs under the actor, 2: the actor's offset is left alone), which can change; and whether its wake shows
FieldEffectTask *func_ov036_021a4484(FieldEffects *effects, u16 dir, const VecFx32 *pos, FieldActor *actor, u32 mode);
void func_ov036_021a4504(FieldEffectTask *task, u8 mode);
void func_ov036_021a4514(FieldEffectTask *task, BOOL showWake);
// A splash of a kind that follows the surfed Pokémon's task, or one at pos that ends with its animation, and
// whether a splash's animation has ended
FieldEffectTask *func_ov036_021a4eb0(FieldEffects *effects, u32 kind, FieldEffectTask *parent);
FieldEffectTask *func_ov036_021a4ee4(FieldEffects *effects, u32 kind, const VecFx32 *pos);
BOOL func_ov036_021a4f18(FieldEffectTask *task);
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
void *func_ov036_021a4384(FieldEffects *effects, HeapID heapId);
void func_ov036_021a43a8(FieldEffects *effects, void *data);
void *func_ov036_021b3e50(FieldEffects *effects, HeapID heapId);
void func_ov036_021b3e94(FieldEffects *effects, void *data);
void *func_ov036_021b46b0(FieldEffects *effects, HeapID heapId);
void func_ov036_021b46ec(FieldEffects *effects, void *data);
void *func_ov036_021b496c(FieldEffects *effects, HeapID heapId);
void func_ov036_021b4990(FieldEffects *effects, void *data);
void *func_ov036_021be790(FieldEffects *effects, HeapID heapId);
void func_ov036_021be7c4(FieldEffects *effects, void *data);
void *func_ov036_021a4d20(FieldEffects *effects, HeapID heapId);
void func_ov036_021a4d48(FieldEffects *effects, void *data);
void *func_ov036_021a5604(FieldEffects *effects, HeapID heapId);
void func_ov036_021a5628(FieldEffects *effects, void *data);
void *func_ov036_021c2788(FieldEffects *effects, HeapID heapId);
void func_ov036_021c27ac(FieldEffects *effects, void *data);
void *func_ov036_021c2ddc(FieldEffects *effects, HeapID heapId);
void func_ov036_021c2e00(FieldEffects *effects, void *data);
void *func_ov036_021a5828(FieldEffects *effects, HeapID heapId);
void func_ov036_021a584c(FieldEffects *effects, void *data);
void *FieldEffect_BTrain_Create(FieldEffects *effects, HeapID heapId);
void FieldEffect_BTrain_Free(FieldEffects *effects, void *data);
void *func_ov036_021c66e8(FieldEffects *effects, HeapID heapId);
void func_ov036_021c670c(FieldEffects *effects, void *data);
// fldeff_hide.c: the object an actor hides in, of a kind (0 to 2); the start of its animation and sound; and whether
// its animation has ended, which it has for a NULL task
FieldEffectTask *func_ov036_021c67c8(FieldEffects *effects, FieldActor *actor, u32 kind);
void func_ov036_021c6800(FieldEffectTask *task);
u16 func_ov036_021c6820(FieldEffectTask *task);
// The bubbles that rise from an actor in the water
void func_ov036_021c6934(FieldActor *actor, FieldEffects *effects);
void *func_ov036_021c696c(FieldEffects *effects, HeapID heapId);
void func_ov036_021c6990(FieldEffects *effects, void *data);
void *func_ov036_021be988(FieldEffects *effects, HeapID heapId);
void func_ov036_021be9ac(FieldEffects *effects, void *data);
void *func_ov036_021c944c(FieldEffects *effects, HeapID heapId);
void func_ov036_021c9470(FieldEffects *effects, void *data);
void *func_ov036_021a5a18(FieldEffects *effects, HeapID heapId);
void func_ov036_021a5a20(FieldEffects *effects, void *data);
void *func_ov036_021a50a4(FieldEffects *effects, HeapID heapId);
void *func_ov036_021a50b0(FieldEffects *effects, HeapID heapId);
void *func_ov036_021a50bc(FieldEffects *effects, HeapID heapId);
void *func_ov036_021a50c8(FieldEffects *effects, HeapID heapId);
void *func_ov036_021a50d4(FieldEffects *effects, HeapID heapId);
void *func_ov036_021a50e0(FieldEffects *effects, HeapID heapId);
void func_ov036_021a50ec(FieldEffects *effects, void *data);

#endif // POKEBW2_FIELD_FIELD_EFFECT_H
