#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/pokemon_league_gimmicks.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

// The script plugin of the Pokémon League's rooms (plugin 3), commands from 1000. The Elite Four's rooms are numbered
// from 1 in the order of their cameras

static void PokemonLeague_DisableRailLines(Field *field, u16 a1);
static u32 PokemonLeague_GetRoomCamera(u16 room);

static BOOL Scrcmd_PokemonLeague_DisableRailLines(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 room = ScriptReadAny(vm, env);

    PokemonLeague_DisableRailLines(field, room - 1);
    return FALSE;
}

// Sets the camera of an Elite Four's room
static BOOL Scrcmd_PokemonLeague_SetCamera(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 room = ScriptReadAny(vm, env);
    FieldCamera *camera = Field_GetCameraSystem(field);

    FieldCamera_SetDefaultsIndex(camera, PokemonLeague_GetRoomCamera(room - 1));
    return FALSE;
}

static void PokemonLeague_DisableRailLines(Field *field, u16 a1) {
    NoGridMapper *mapper = Field_GetNoGridMapper(field);

    FieldNoGridMapper_SetLineEnabled(mapper, 1, FALSE);
    FieldNoGridMapper_SetLineEnabled(mapper, 2, FALSE);
    if (a1 == 0) {
        FieldNoGridMapper_SetLineEnabled(mapper, 3, FALSE);
    }
}

static const u32 sRoomCameras[] = { 0x1c, 0x1d, 0x1e, 0x1f };

static u32 PokemonLeague_GetRoomCamera(u16 room) {
    return sRoomCameras[room];
}

// Grimsley's room
static BOOL Scrcmd_PokemonLeague_PlayGrimsleyGroup(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickGrimsley_PlayGroup(gsys, VM_Read16(vm));
    return FALSE;
}

// Grimsley's room
static BOOL Scrcmd_PokemonLeague_PlayGrimsleyModel0Animation(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickGrimsley_PlayModel0Animation(gsys, VM_Read16(vm));
    return FALSE;
}

static BOOL Scrcmd_PokemonLeague_PlayCaitlinAmbience(VM *vm, FieldScriptEnv *env) {
    FieldSnd_PlayAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))),
                          SEQ_SE_SW_CATTLEYA_03);
    return FALSE;
}

// Caitlin's room
static BOOL Scrcmd_PokemonLeague_PlayCaitlinModels34Or5(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickCaitlin_PlayModels34Or5(gsys, VM_Read16(vm));
    return FALSE;
}

// Caitlin's room
static BOOL Scrcmd_PokemonLeague_CallCaitlinModel1Event(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickCaitlin_CreateModel1Event(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Caitlin's room
static BOOL Scrcmd_PokemonLeague_PlayCaitlinModel0(VM *vm, FieldScriptEnv *env) {
    GimmickCaitlin_CallPlayModel0(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

// Caitlin's room
static BOOL Scrcmd_PokemonLeague_DropCaitlinActor(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 actorId = VM_Read16(vm);

    GimmickCaitlin_DropActor(gsys, actorId, VM_Read16(vm));
    return FALSE;
}

// Shauntal's room
static BOOL Scrcmd_PokemonLeague_CallShauntalModel8Event(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickShauntal_CallModel8Event(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Shauntal's room
static BOOL Scrcmd_PokemonLeague_PlayShauntalModel(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickShauntal_PlayModelFromIndex(gsys, VM_Read16(vm));
    return FALSE;
}

// Shauntal's room
static BOOL Scrcmd_PokemonLeague_CallShauntalRoomChangeEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GimmickShauntal_CallRoomChangeEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Shauntal's room
static BOOL Scrcmd_PokemonLeague_CallShauntalModels6And9Event(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickShauntal_CallModels6And9Event(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Marshal's room
static BOOL Scrcmd_PokemonLeague_PlayMarshalModel0(VM *vm, FieldScriptEnv *env) {
    MarshalRoom_PlayModel0Anims(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

// Marshal's room
static BOOL Scrcmd_PokemonLeague_PlayMarshalModel3Animation(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    MarshalRoom_PlayModel3Anim(gsys, VM_Read16(vm));
    return FALSE;
}

// Marshal's room
static BOOL Scrcmd_PokemonLeague_PlayMarshalModel1(VM *vm, FieldScriptEnv *env) {
    MarshalRoom_PlayModel1Anims(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

// Marshal's room
static BOOL Scrcmd_PokemonLeague_CallMarshalPlatformEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = MarshalRoom_LowerPlatform(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// The Champion's room
static BOOL Scrcmd_PokemonLeague_CallChampionChangeEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickChampion_CallChangeEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// The Champion's room
static BOOL Scrcmd_PokemonLeague_CallChampionOpenEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickChampion_CallOpenEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// The Champion's room
static BOOL Scrcmd_PokemonLeague_HideChampionActorShadow(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickChampion_HideActorShadow(gsys, VM_Read16(vm));
    return FALSE;
}

// The Champion's room
static BOOL Scrcmd_PokemonLeague_ShowChampionActorShadow(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickChampion_ShowActorShadow(gsys, VM_Read16(vm));
    return FALSE;
}

// The Champion's room
static BOOL Scrcmd_PokemonLeague_SetChampionRoomOpen(VM *vm, FieldScriptEnv *env) {
    GimmickChampion_CallSetOpen(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

const FieldScriptCommand POKEMON_LEAGUE_SCRIPT_COMMANDS[] = {
    Scrcmd_PokemonLeague_DisableRailLines,
    Scrcmd_PokemonLeague_SetCamera,
    Scrcmd_PokemonLeague_PlayGrimsleyGroup,
    Scrcmd_PokemonLeague_PlayGrimsleyModel0Animation,
    Scrcmd_PokemonLeague_PlayCaitlinAmbience,
    Scrcmd_PokemonLeague_PlayCaitlinModels34Or5,
    Scrcmd_PokemonLeague_CallCaitlinModel1Event,
    Scrcmd_PokemonLeague_PlayCaitlinModel0,
    Scrcmd_PokemonLeague_DropCaitlinActor,
    Scrcmd_PokemonLeague_CallShauntalModel8Event,
    Scrcmd_PokemonLeague_PlayShauntalModel,
    Scrcmd_PokemonLeague_CallShauntalRoomChangeEvent,
    Scrcmd_PokemonLeague_CallShauntalModels6And9Event,
    Scrcmd_PokemonLeague_PlayMarshalModel0,
    Scrcmd_PokemonLeague_PlayMarshalModel3Animation,
    Scrcmd_PokemonLeague_PlayMarshalModel1,
    Scrcmd_PokemonLeague_CallMarshalPlatformEvent,
    Scrcmd_PokemonLeague_CallChampionChangeEvent,
    Scrcmd_PokemonLeague_CallChampionOpenEvent,
    Scrcmd_PokemonLeague_HideChampionActorShadow,
    Scrcmd_PokemonLeague_ShowChampionActorShadow,
    Scrcmd_PokemonLeague_SetChampionRoomOpen,
    (FieldScriptCommand)0xFFFFFFFF,
};
