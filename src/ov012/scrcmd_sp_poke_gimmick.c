// Script commands of the special Pokémon: overlay 129's events for them (sp_poke_gimmick.c), finding one given at an
// event in the party, and the roaming Pokémon. The name is a guess after overlay 129's file.
// Scrcmd_SpPokeGimmick_PokePartyFindEx is swan's name (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/scrcmd_sp_poke_gimmick.h"
#include "field/sp_poke_event.h"
#include "nitro/fx.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/encounter.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

#define MOVE_RELIC_SONG 547
// Any form
#define FORM_ANY 0xff

// The center of a grid square, in fx32, and a height in half squares
#define GRID_CENTER(n) ((n) * FX32_CONST(16) + FX32_CONST(8))
#define GRID_HEIGHT(n) ((n) * FX32_CONST(8))

static BOOL FindSpecialTransferPkm(PokeParty *party, u16 species, u16 move, u8 form, u32 kind, u16 *slot,
                                   PlayerInfo *playerInfo);

BOOL Scrcmd_SpPokeGimmick_SpPokeJump(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 mode = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 height = ScriptReadAny(vm, env);
    u16 a5 = ScriptReadAny(vm, env);
    VecFx32 playerPos;
    VecFx32 pos;
    VecFx32 *start;
    VecFx32 *end;
    GameEvent *event;

    FieldPlayer_GetWPos(Field_GetPlayer(field), &playerPos);
    pos.x = GRID_CENTER(x);
    pos.y = GRID_HEIGHT(y);
    pos.z = GRID_CENTER(z);
    if (mode == 0) {
        start = &playerPos;
        end = &pos;
    } else {
        start = &pos;
        end = &playerPos;
    }
    event = SpPokeEvent_CreateJump(gsys, mode, start, end, GRID_HEIGHT(height), a5);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_SpPokeGimmick_SpPokeAppear(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a1 = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    VecFx32 pos;

    pos.x = GRID_CENTER(x);
    pos.y = GRID_HEIGHT(y);
    pos.z = GRID_CENTER(z);
    SpPokeEvent_Appear(gsys, a1, &pos);
    return FALSE;
}

BOOL Scrcmd_SpPokeGimmick_SpPokeWaitAnimEnd(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = SpPokeEvent_CreateWaitAnimEnd(gsys, ScriptReadAny(vm, env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_SpPokeGimmick_SpPokeWaitFrame(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = SpPokeEvent_CreateWaitFrame(gsys, ScriptReadAny(vm, env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_SpPokeGimmick_PokePartyFindEx(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokeParty *party = GameData_GetParty(gameData);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    u16 species = ScriptReadAny(vm, env);
    u16 checkMove = ScriptReadAny(vm, env);
    u16 *slot = ScriptReadVar(vm, env);
    u16 *found = ScriptReadVar(vm, env);
    u32 kind;
    u16 move;
    u8 form;

    switch (species) {
    case SPECIES_KELDEO:
        kind = 4;
        move = 0;
        form = 0;
        break;
    case SPECIES_MELOETTA:
        kind = 5;
        move = MOVE_RELIC_SONG;
        form = FORM_ANY;
        break;
    case SPECIES_CELEBI:
        kind = 0;
        move = 0;
        form = FORM_ANY;
        break;
    case SPECIES_RAIKOU:
    case SPECIES_ENTEI:
    case SPECIES_SUICUNE:
        kind = 2;
        move = 0;
        form = FORM_ANY;
        break;
    case SPECIES_GENESECT:
        kind = 6;
        move = 0;
        form = FORM_ANY;
        break;
    case SPECIES_SHAYMIN:
        kind = 7;
        move = 0;
        form = FORM_ANY;
        break;
    case SPECIES_LANDORUS:
        kind = 8;
        move = 0;
        form = 1;
        break;
    default:
        *found = FALSE;
        return FALSE;
    }
    if (checkMove == FALSE) {
        move = 0;
    }
    *found = FindSpecialTransferPkm(party, species, move, form, kind, slot, playerInfo);
    return FALSE;
}

// Finds the first Pokémon of the species given at the event of the kind, that doesn't know the move and is of the form
static BOOL FindSpecialTransferPkm(PokeParty *party, u16 species, u16 move, u8 form, u32 kind, u16 *slot,
                                   PlayerInfo *playerInfo) {
    BOOL found;
    int count = PokeParty_GetPkmCount(party);
    int i;
    PartyPkm *pkm;

    found = FALSE;
    *slot = 0;
    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_IsSpecialTransfer(pkm, kind, playerInfo) == FALSE ||
            species != PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL)) {
            continue;
        }
        if (move != 0 && (move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE1, NULL) ||
                          move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 1, NULL) ||
                          move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 2, NULL) ||
                          move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 3, NULL))) {
            continue;
        }
        if (form == FORM_ANY || form == PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL)) {
            found = TRUE;
            *slot = i;
            break;
        }
    }
    return found;
}

BOOL Scrcmd_SpPokeGimmick_SetSpecialTransferUsed(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokeParty *party = GameData_GetParty(gameData);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    u16 species = ScriptReadAny(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    u32 kind;
    PartyPkm *pkm;

    switch (species) {
    case SPECIES_CELEBI:
        kind = 0;
        break;
    case SPECIES_RAIKOU:
    case SPECIES_ENTEI:
    case SPECIES_SUICUNE:
        kind = 2;
        break;
    default:
        return FALSE;
    }
    pkm = PokeParty_GetPkm(party, slot);
    if (PokeParty_IsSpecialTransfer(pkm, kind, playerInfo) == FALSE) {
        return FALSE;
    }
    PokeParty_SetSpecialTransferUsed(pkm, kind, playerInfo);
    return FALSE;
}

BOOL Scrcmd_SpPokeGimmick_ReadSaveByte(VM *vm, FieldScriptEnv *env) {
    EncountSave *encountSave = SaveControl_GetEncountSave(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u8 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = EncountSave_GetEntryState(encountSave, index);
    return FALSE;
}
