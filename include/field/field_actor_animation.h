#ifndef POKEBW2_FIELD_FIELD_ACTOR_ANIMATION_H
#define POKEBW2_FIELD_FIELD_ACTOR_ANIMATION_H

#include "types.h"
#include "field/field_async_proc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

// Function names from swan.
// Only the fields needed by the current C accessors are known.
struct FieldActorAnmProc {
    FieldActor *actor;
    u8 unk4[0x180];
    G3DCurve *curve;
    FieldAsyncProc *asyncProc;
    VecFx32 pos;
    u8 unk198;
    u8 animation;
    u8 finished;
    u8 unk19b;
    HeapID heapId;
};

FieldActorAnmProc *FieldActorAnmProc_Create(Field *field, u16 actorId, const VecFx32 *pos, HeapID heapId);
void FieldActorAnmProc_Free(FieldActorAnmProc *proc);
void FieldActorAnmProc_Play(FieldActorAnmProc *proc, u16 animation);
BOOL FieldActorAnmProc_IsPlaying(FieldActorAnmProc *proc);
void FieldActorAnmProc_Update(FieldAsyncProc *asyncProc, Field *field, void *data);
void FieldActorAnmProc_Draw(FieldAsyncProc *asyncProc, Field *field, void *data);
void FieldActorAnmProc_CommitTransform(G3DCurve *curve, FieldActor *actor, VecFx32 *pos);
BOOL Scrcmd_FieldActorAnimation_ActorAnimationInit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActorAnimation_ActorAnimationFree(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActorAnimation_ActorAnimationPlay(VM *vm, FieldScriptEnv *env);
GameEventReturnCode EventActorAnmProcWait_Callback(GameEvent *event, u32 *state, void *data);
BOOL Scrcmd_FieldActorAnimation_ActorAnimationWait(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_FIELD_ACTOR_ANIMATION_H
