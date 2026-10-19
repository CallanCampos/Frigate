// Script commands from the end of overlay 12: swapping the Join Avenue's overlays for its store, the Pokéstar
// Studios' level and the lens flare. The name is a guess after the first commands; s02C6_JoinAvenueStoreStart,
// s02C6_JoinAvenueStoreEnd and s02E3_LensFlareRequest are swan's names
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_join_avenue_store.h"
#include "gfl/overlay.h"
#include "save/pokewood.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL s02C6_JoinAvenueStoreStart(VM *vm, FieldScriptEnv *env) {
    GFL_OvlUnload(OVERLAY_ID(59));
    GFL_OvlLoad(OVERLAY_ID(60));
    return FALSE;
}

BOOL s02C6_JoinAvenueStoreEnd(VM *vm, FieldScriptEnv *env) {
    GFL_OvlUnload(OVERLAY_ID(60));
    GFL_OvlLoad(OVERLAY_ID(59));
    return FALSE;
}

BOOL s02CB_PokewoodGetLevel(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u32 level = func_020111ec(func_020111b0(PokewoodSave_Get(FieldScriptEnv_GetGameData(env))));

    if (level >= 5) {
        level = 0;
    }
    *result = level;
    return FALSE;
}

BOOL s02E3_LensFlareRequest(VM *vm, FieldScriptEnv *env) {
    GameData_SetLensFlareRequested(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)), TRUE);
    return FALSE;
}
