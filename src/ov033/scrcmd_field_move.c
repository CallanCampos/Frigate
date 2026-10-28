#include "types.h"
#include "field/event_dive.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/field_move_scripts.h"
#include "field/field_move_tcb.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL ScriptNative_SurfTCBWait(VM *vm, void *env) {
    TCB *tcb;

    tcb = FieldScriptEnv_GetPlayerGridEventTCB(env);
    if (FieldSurfTCB_CheckEnd(tcb) == TRUE) {
        FieldSurfTCB_Free(tcb);
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_FieldMove_CallSurf(VM *vm, FieldScriptEnv *env) {
    VecFx32 position;
    Field *field;
    FieldPlayer *player;
    FieldG3DMapper *mapper;
    u16 direction;
    HeapID heapId;
    u32 tileType;
    TCB *tcb;

    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    player = Field_GetPlayer(field);
    direction = GetActorFaceDir(FieldPlayer_GetActor(player));
    mapper = Field_GetG3DMapper(field);
    heapId = FieldScriptEnv_GetHeapID(env);
    FieldPlayer_GetWPosInDir(player, direction, &position);
    tileType = GetTileTypeAtPos(mapper, &position);
    tcb = FieldSurfTCB_Create(player, direction, tileType, heapId);
    FieldScriptEnv_SetPlayerGridEventTCB(env, tcb);
    VM_SetNativeCallback(vm, ScriptNative_SurfTCBWait);
    return TRUE;
}

BOOL ScriptNative_WaterfallTCBWait(VM *vm, void *env) {
    TCB *tcb;

    tcb = FieldScriptEnv_GetPlayerGridEventTCB(env);
    if (FieldWaterfallTCB_CheckEnd(tcb) == TRUE) {
        FieldWaterfallTCB_Free(tcb);
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_FieldMove_CallWaterfall(VM *vm, FieldScriptEnv *env) {
    FieldPlayer *player;
    HeapID heapId;
    u32 direction;
    u32 param;
    TCB *tcb;

    heapId = FieldScriptEnv_GetHeapID(env);
    player = Field_GetPlayer(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    direction = FieldPlayer_GetFaceDir(player);
    param = ScriptReadAny(vm, env);
    tcb = FieldWaterfallTCB_Create(player, direction, param, heapId);
    FieldScriptEnv_SetPlayerGridEventTCB(env, tcb);
    VM_SetNativeCallback(vm, ScriptNative_WaterfallTCBWait);
    return TRUE;
}

BOOL Scrcmd_FieldMove_CallCut(VM *vm, FieldScriptEnv *env) {
    Field *field;
    FieldActor *actor;

    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    actor = FieldPlayer_GetActor(Field_GetPlayer(field));
    FldeffCut_Start(actor, Field_GetFieldEffects(field));
    return FALSE;
}

BOOL Scrcmd_FieldMove_CallDiving(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 mode = ScriptReadAny(vm, env);

    if (mode == 0) {
        ScriptWork_CallEvent(work, EventDiveIn_Create(gsys, field));
    } else if (mode == 1) {
        ScriptWork_CallEvent(work, CreateDiveOutEvent(gsys, field, FALSE));
    } else {
        ScriptWork_CallEvent(work, CreateDiveOutEvent(gsys, field, TRUE));
    }
    return TRUE;
}
