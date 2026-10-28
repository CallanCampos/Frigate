// The script commands of build models, the map's props: the Hall of Fame machine, the Pokémon Center's healing and
// PC, and finding, animating, showing and swapping props by their tile. The ROM has no name for the file;
// scrcmd_buildmodel.c is descriptive, after field_buildmodel.c. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/event_hall_of_fame_machine.h"
#include "field/event_pokemon_center.h"
#include "field/field.h"
#include "field/field_g3d_mapper.h"
#include "field/field_prop.h"
#include "field/field_script.h"
#include "field/pc_sound.h"
#include "field/scrcmd_buildmodel.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_system.h"
#include "system/vm.h"

u16 g_ScrBMAnmWaitHandleID;

// The world position of a tile
#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))

static FieldPropSystem *GetScrEnvBMSystem(FieldScriptEnv *env);
static FieldChunkPropHolder *FieldPropSystem_FindPropAtGPos(FieldPropSystem *system, u32 propId, u16 x, u16 z);
static BOOL ScriptNative_BMHndAnmWait(VM *vm, void *env);

BOOL Scrcmd_Buildmodel_BMPlayHOFMachineSeq(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventHallOfFameMachine_Create(gsys, ScriptWork_GetEvent(work)));
    return TRUE;
}

BOOL Scrcmd_Buildmodel_PokecenPlayHealingSequence(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 ballCount = ScriptReadAny(vm, env);

    Field_GetPlayer(GSYS_GetField(gsys));
    ScriptWork_CallEvent(work, EventPokeCenHeal_Create(gsys, ScriptWork_GetEvent(work), ballCount));
    return TRUE;
}

BOOL Scrcmd_Buildmodel_PokecenPCOpen(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, CreatePCSoundCallEvent(ScriptWork_GetEvent(work), gsys, field));
    return TRUE;
}

BOOL Scrcmd_Buildmodel_PokecenPCIdle(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, func_ov033_02179a58(ScriptWork_GetEvent(work), gsys, field));
    return TRUE;
}

BOOL Scrcmd_Buildmodel_PokecenPCClose(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);
    u16 skipSound = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, FieldDoorAnime_StartClose(ScriptWork_GetEvent(work), gsys, field, skipSound));
    return TRUE;
}

static FieldPropSystem *GetScrEnvBMSystem(FieldScriptEnv *env) {
    return FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
}

static FieldChunkPropHolder *FieldPropSystem_FindPropAtGPos(FieldPropSystem *system, u32 propId, u16 x, u16 z) {
    VecFx32 pos;

    pos.x = GRID_TO_FX32(x);
    pos.y = 0;
    pos.z = GRID_TO_FX32(z);
    return FieldPropSystem_FindPropAtPos(system, propId, &pos);
}

BOOL Scrcmd_Buildmodel_BMAnmPlayInv(VM *vm, FieldScriptEnv *env) {
    u16 propId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    FieldPropSystem *system = GetScrEnvBMSystem(env);
    FieldChunkPropHolder *prop = FieldPropSystem_FindPropAtGPos(system, propId, x, z);

    if (prop != NULL) {
        FieldChunkPropHolder_CallAnmCmd(system, prop, 0, 1);
        FieldChunkPropHolder_CallAnmCmd(system, prop, 0, 3);
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMAnmPlayLoop(VM *vm, FieldScriptEnv *env) {
    u16 propId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    FieldPropSystem *system = GetScrEnvBMSystem(env);
    FieldChunkPropHolder *prop = FieldPropSystem_FindPropAtGPos(system, propId, x, z);

    if (prop != NULL) {
        FieldChunkPropHolder_CallAnmCmd(system, prop, 0, 2);
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMSetVisible(VM *vm, FieldScriptEnv *env) {
    u16 propId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 visible = ScriptReadAny(vm, env);
    FieldChunkPropHolder *prop = FieldPropSystem_FindPropAtGPos(GetScrEnvBMSystem(env), propId, x, z);

    if (prop != NULL) {
        FieldChunkPropHolder_SetVisible(prop, visible);
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMChangeMdlID(VM *vm, FieldScriptEnv *env) {
    u16 propId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 resId = ScriptReadAny(vm, env);
    FieldPropSystem *system = GetScrEnvBMSystem(env);
    FieldChunkPropHolder *prop = FieldPropSystem_FindPropAtGPos(system, propId, x, z);

    if (prop != NULL) {
        FieldChunkPropHolder_ChangeResID(system, prop, resId);
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMCreateHandleByGPos(VM *vm, FieldScriptEnv *env) {
    VecFx32 pos;
    u16 *result = ScriptReadVar(vm, env);
    u16 propId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    FieldPropSystem *system =
        FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));

    pos.x = GRID_TO_FX32(x);
    pos.y = 0;
    pos.z = GRID_TO_FX32(z);
    *result = FieldPropSystem_GetHandleID(system, FieldPropSystem_CreateHandleAtPos(system, propId, &pos));
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMReleaseHandle(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    FieldPropHandle *handle = FieldPropSystem_FindHandleByID(GetScrEnvBMSystem(env), id);

    if (handle != NULL) {
        FieldPropHandle_Free(handle);
    }
    return FALSE;
}

// Play the handle's animation and its sound
BOOL Scrcmd_Buildmodel_BMHndAudioVisualAnmPlay(VM *vm, FieldScriptEnv *env) {
    u16 soundId;
    u16 id = ScriptReadAny(vm, env);
    u16 animation = ScriptReadAny(vm, env);
    FieldPropHandle *handle = FieldPropSystem_FindHandleByID(GetScrEnvBMSystem(env), id);

    if (handle != NULL) {
        FieldPropHandle_CallAnmCmd(handle, animation, 0);
        if (FieldPropHandle_GetAnimSoundID(handle, &soundId)) {
            GFL_SndSEPlay(soundId);
        }
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMHndAnmPlay(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    u16 animation = ScriptReadAny(vm, env);
    FieldPropHandle *handle = FieldPropSystem_FindHandleByID(GetScrEnvBMSystem(env), id);

    if (handle != NULL) {
        FieldPropHandle_CallAnmCmd(handle, animation, 2);
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMHndAnmPause(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    FieldPropHandle *handle = FieldPropSystem_FindHandleByID(GetScrEnvBMSystem(env), id);

    if (handle != NULL) {
        FieldPropHandle_CallAnmCmdSilent(handle, 3);
    }
    return FALSE;
}

BOOL Scrcmd_Buildmodel_BMHndAnmWait(VM *vm, FieldScriptEnv *env) {
    g_ScrBMAnmWaitHandleID = ScriptReadAny(vm, env);
    VM_SetNativeCallback(vm, ScriptNative_BMHndAnmWait);
    return TRUE;
}

// Wait for the handle's animation and its sound to end
static BOOL ScriptNative_BMHndAnmWait(VM *vm, void *env) {
    FieldPropHandle *handle = FieldPropSystem_FindHandleByID(GetScrEnvBMSystem(env), g_ScrBMAnmWaitHandleID);

    if (handle == NULL) {
        return TRUE;
    }
    if (!FieldPropHandle_IsAnmFinished(handle)) {
        return FALSE;
    }
    if (!FieldPropHandle_IsAnimSoundFinished(handle)) {
        return TRUE;
    }
    return FALSE;
}
