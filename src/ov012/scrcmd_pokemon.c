#include "types.h"
#include "constants/pokemon.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/player_state.h"
#include "field/scrcmd_pokemon.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/met_data.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/version.h"
#include "system/vm.h"

BOOL CheckGetPartyPokemon(FieldScriptEnv *env, u32 index, PartyPkm **pkm) {
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    BOOL found;

    if (index < PokeParty_GetPkmCount(party)) {
        found = TRUE;
    } else {
        found = FALSE;
        index = 0;
    }
    *pkm = PokeParty_GetPkm(party, index);
    return found;
}

u32 GetScrPokeStat(FieldScriptEnv *env, u32 index, u32 param) {
    PartyPkm *pkm;

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        return PokeParty_GetParam(pkm, param, NULL);
    }
    return 0;
}

BOOL CheckPokeMoveLearned_NonEgg(PartyPkm *pkm, u16 move) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != 0) {
        return FALSE;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1, NULL) == move ||
        PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 1, NULL) == move ||
        PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 2, NULL) == move ||
        PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 3, NULL) == move) {
        return TRUE;
    }
    return FALSE;
}

BOOL s011B_PokePartyGetTypes(VM *vm, FieldScriptEnv *env) {
    u16 *type1 = ScriptReadVar(vm, env);
    u16 *type2 = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        *type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
        *type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
    } else {
        *type1 = 0;
        *type2 = 0;
    }
    return FALSE;
}

BOOL s0104_PokePartyRecoverAll(VM *vm, FieldScriptEnv *env) {
    PokeParty_RecoverAll(GameData_GetParty(FieldScriptEnv_GetGameData(env)));
    return FALSE;
}

BOOL s0112_PokePartyGetEVTotal(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;
    u16 total = 0;

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        total += (u16)PokeParty_GetParam(pkm, PKM_PARAM_EV_HP, NULL);
        total += (u16)PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 1, NULL);
        total += (u16)PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 2, NULL);
        total += (u16)PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 3, NULL);
        total += (u16)PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 4, NULL);
        total += (u16)PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 5, NULL);
    }
    *result = total;
    return FALSE;
}

BOOL s0101_PokePartyIsFullHP(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;

    *result = FALSE;
    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        u16 hp = (u16)PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
        u16 maxHp = (u16)PokeParty_GetParam(pkm, PKM_PARAM_HP + 1, NULL);
        if (hp == maxHp || PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
            *result = TRUE;
        }
    }
    return FALSE;
}

BOOL s024E_PokePartyIsFullPP(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;
    u16 pp;
    u16 i;
    u32 full;

    *result = FALSE;
    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        full = TRUE;

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE || PokeParty_GetParam(pkm, 3, NULL) == TRUE) {
            *result = TRUE;
            return FALSE;
        }

        for (i = 0; i < 4; i++) {
            if ((u16)PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL) == 0) {
                continue;
            }
            pp = (u16)PokeParty_GetParam(pkm, 0x3a + i, NULL);
            u16 maxPp = (u16)PokeParty_GetParam(pkm, 0x42 + i, NULL);
            if (pp != maxPp) {
                full = FALSE;
                break;
            }
        }
        *result = full;
    }
    return FALSE;
}

BOOL s0102_PokePartyIsEgg(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;

    *result = FALSE;
    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            *result = FALSE;
        } else {
            *result = TRUE;
        }
    }
    return FALSE;
}

BOOL s00FC_PokePartyGetHappiness(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;

    *result = 0;
    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            *result = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
        }
    }
    return FALSE;
}

BOOL s00FD_PokePartyAdjustHappiness(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 delta = ScriptReadAny(vm, env);
    u16 mode = VM_Read16(vm);
    PartyPkm *pkm = NULL;
    u8 happiness;

    if (!CheckGetPartyPokemon(env, index, &pkm)) {
        return FALSE;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
        return FALSE;
    }
    happiness = (u8)PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
    switch (mode) {
    case 0:
        if (delta > 255) {
            happiness = 255;
        } else {
            happiness = (u8)delta;
        }
        break;
    case 1:
        if (happiness + delta > 255) {
            happiness = 255;
        } else {
            happiness += (u8)delta;
        }
        break;
    case 2:
        if (happiness < delta) {
            happiness = 0;
        } else {
            happiness -= (u8)delta;
        }
        break;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, happiness);
    return FALSE;
}

BOOL s00FE_PokePartyGetSpecies(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);

    if (GetScrPokeStat(env, index, PKM_PARAM_IS_EGG) != 0) {
        *result = SPECIES_EGG;
    } else {
        *result = GetScrPokeStat(env, index, PKM_PARAM_SPECIES);
    }
    return FALSE;
}

BOOL s00FF_PokePartyGetForme(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    *result = GetScrPokeStat(env, index, PKM_PARAM_FORM);
    return FALSE;
}

BOOL s010D_PokePartyGetMemberByType(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 kind = ScriptReadAny(vm, env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    u16 index;

    switch (kind) {
    case 2:
        index = PokeParty_GetFirstBattleReady(party);
        break;
    case 1:
        index = isEggInParty(party);
        break;
    default:
        index = 0;
        break;
    }
    *result = index;
    return FALSE;
}

BOOL s0103_PokePartyGetCount(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 kind = ScriptReadAny(vm, env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    int count;

    switch (kind) {
    case 0:
        count = PokeParty_GetPkmCount(party);
        break;
    case 1:
        count = howManyPartyPokesAreNotEggs(party);
        break;
    case 2:
        count = howManyPokesAreAbleToFight(party);
        break;
    case 3:
        count = countAllEggsInParty(party);
        break;
    case 4:
        count = countSanityEggsInParty(party);
        break;
    case 5:
        count = PokeParty_GetCapacity(party);
        int partyCount = PokeParty_GetPkmCount(party);
        if (count < partyCount) {
            count = 0;
        } else {
            count -= partyCount;
        }
        break;
    default:
        count = 0;
        break;
    }
    *result = count;
    return FALSE;
}

BOOL s0114_PokePartyGetCountBySpecies(VM *vm, FieldScriptEnv *env) {
    u16 species = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    u16 partyCount = PokeParty_GetPkmCount(party);
    u16 count = 0;
    int i;

    if (species != 0 && species <= SPECIES_EGG - 1) {
        for (i = 0; i < partyCount; i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);
            if (pkm != NULL && PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
                u16 foundSpecies = (u16)PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
                if (foundSpecies == species) {
                    count++;
                }
            }
        }
        *result = count;
    } else {
        *result = 0;
    }
    return FALSE;
}

BOOL s0121_BoxGetCount(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 kind = ScriptReadAny(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(gameData);
    GameData_GetParty(gameData);
    int count;

    switch (kind) {
    case 0:
        count = howManyTotalPokesAreInBoxes(boxes);
        break;
    case 1:
    case 2:
        count = howManyNormalPokesAreInAllBoxes(boxes);
        break;
    case 3:
        count = howManyTotalPokesAreInBoxes(boxes) - howManyNormalPokesAreInAllBoxes(boxes);
        break;
    case 5:
        count = howManyTotalPokesAreInBoxes(boxes);
        if (count > 720) {
            count = 0;
        } else {
            count = 720 - count;
        }
        break;
    default:
        count = 0;
        break;
    }
    *result = count;
    return FALSE;
}

BOOL s0118_PokePartyFindBySpecies(VM *vm, FieldScriptEnv *env) {
    u16 species = ScriptReadAny(vm, env);
    u16 *found = ScriptReadVar(vm, env);
    u16 *indexOut = ScriptReadVar(vm, env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    u16 count = PokeParty_GetPkmCount(party);
    int i;

    if (species != 0 && species <= SPECIES_EGG - 1) {
        for (i = 0; i < count; i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);
            if (pkm != NULL && PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
                u16 pkmSpecies = (u16)PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
                if (pkmSpecies == species) {
                    break;
                }
            }
        }
        if (i < count) {
            *found = TRUE;
            *indexOut = i;
        } else {
            *found = FALSE;
        }
    } else {
        *found = FALSE;
    }
    return FALSE;
}

BOOL s0115_PokePartyHasMove(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 move = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;

    *result = FALSE;
    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        if (CheckPokeMoveLearned_NonEgg(pkm, move) == TRUE) {
            *result = TRUE;
        }
    }
    return FALSE;
}

BOOL s0116_PokePartyHasMoveAny(VM *vm, FieldScriptEnv *env) {
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    u16 *result = ScriptReadVar(vm, env);
    u16 move = ScriptReadAny(vm, env);
    int i;
    int count = PokeParty_GetPkmCount(party);

    *result = 6;
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        if (CheckPokeMoveLearned_NonEgg(pkm, move) == TRUE) {
            *result = i;
            break;
        }
    }
    return FALSE;
}

BOOL s010C_PokePartyAdd(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    HeapID heapId;
    u16 *result;
    u16 species;
    u16 form;
    u16 level;
    BoxPkmCreateParams params;

    FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    GameData_GetParty(gameData);
    GetGameDataPlayerInfo(gameData);
    result = ScriptReadVar(vm, env);
    species = ScriptReadAny(vm, env);
    form = ScriptReadAny(vm, env);
    level = ScriptReadAny(vm, env);
    sys_memset(&params, 0, sizeof(BoxPkmCreateParams));
    params.heapId = heapId;
    params.species = species;
    params.form = form;
    params.level = level;
    params.item = 0;
    params.ability = 2;
    params.sex = 2;
    params.param1C = 2;
    params.ball = 4;
    *result = addPkmToParty(gameData, &params);
    return FALSE;
}

BOOL s010E_PokePartyAddEx(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    HeapID heapId;
    u16 *result;
    u16 species;
    u16 form;
    u16 level;
    u16 ability;
    u16 sex;
    u16 param1C;
    u16 item;
    u16 ball;
    BoxPkmCreateParams params;

    FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    GameData_GetParty(gameData);
    GetGameDataPlayerInfo(gameData);
    result = ScriptReadVar(vm, env);
    species = ScriptReadAny(vm, env);
    form = ScriptReadAny(vm, env);
    level = ScriptReadAny(vm, env);
    ability = ScriptReadAny(vm, env);
    sex = ScriptReadAny(vm, env);
    param1C = ScriptReadAny(vm, env);
    item = ScriptReadAny(vm, env);
    ball = ScriptReadAny(vm, env);
    sys_memset(&params, 0, sizeof(BoxPkmCreateParams));
    params.heapId = heapId;
    params.species = species;
    params.form = form;
    params.level = level;
    params.item = item;
    params.ability = ability;
    params.hiddenAbility = params.ability == 3 ? TRUE : FALSE;
    if (params.ability >= 3) {
        params.ability = 2;
    }
    params.sex = sex;
    if (params.sex >= 3) {
        params.sex = 2;
    }
    params.param1C = param1C;
    if (params.param1C >= 3) {
        params.param1C = 2;
    }
    params.ball = ball;
    *result = addPkmToParty(gameData, &params);
    return FALSE;
}

BOOL s02EA_PokePartyAddNPoke(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    HeapID heapId;
    PokeParty *party;
    PlayerInfo *playerInfo;
    u16 *result;
    u16 species;
    u16 level;
    u16 unk5;
    u16 unk7;
    u16 unk6;
    PartyPkm *pkm;
    NPokeSpec spec;

    FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    party = GameData_GetParty(gameData);
    playerInfo = GetGameDataPlayerInfo(gameData);
    result = ScriptReadVar(vm, env);
    species = ScriptReadAny(vm, env);
    level = ScriptReadAny(vm, env);
    unk5 = ScriptReadAny(vm, env);
    unk7 = ScriptReadAny(vm, env);
    unk6 = ScriptReadAny(vm, env);
    pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), TRUE, "scrcmd_pokemon.c", 0x3bd);
    sys_memset(&spec, 0, sizeof(NPokeSpec));
    spec.species = species;
    spec.level = level;
    spec.nature = unk5;
    spec.sex = unk6;
    spec.ability = unk7;
    createNPkm(pkm, &spec);
    PokeParty_SetParam(pkm, PKM_PARAM_POKEBALL, PML_ItemGetMonsBallID(4));
    PokeParty_SetupMetData(pkm, 7, playerInfo,
                           ZoneData_GetPlaceNameID(PlayerState_GetZoneID(GameData_GetPlayerState(gameData))), heapId);
    PokeParty_RecalcStats(pkm);
    *result = PokeParty_AddPkm(party, pkm);
    addPkmToDex(GameData_GetPokedex(gameData), pkm);
    GFL_HeapFree(pkm);
    return FALSE;
}

BOOL s0108_PokePartyGetMoveCount(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;
    int i;
    int count = 0;

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        for (i = 0; i < 4; i++) {
            if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL) != 0) {
                count++;
            }
        }
    }
    *result = count;
    return FALSE;
}

BOOL s010A_PokePartyGetMove(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    u16 slot;
    u16 index;
    u16 *result;

    result = ScriptReadVar(vm, env);
    index = ScriptReadAny(vm, env);
    slot = ScriptReadAny(vm, env);

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        *result = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL);
    } else {
        *result = 0;
    }
    return FALSE;
}

BOOL s010B_PokePartyLearnMove(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    u16 move = ScriptReadAny(vm, env);
    PartyPkm *pkm;
    BOOL encrypted;
    u8 i;
    u32 moveId;
    u32 pp;
    u32 ppUps;

    if (!CheckGetPartyPokemon(env, index, &pkm)) {
        return FALSE;
    }
    encrypted = PokeParty_DecryptPkm(pkm);
    if (slot > 4) {
        PokeParty_SetLastMove(pkm, move);
    } else if (move == 0) {
        i = slot;
        if (slot < 3) {
            for (; i < 3; i++) {
                moveId = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i + 1, NULL);
                if (moveId == 0) {
                    break;
                }
                ppUps = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i + 1, NULL);
                pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i + 1, NULL);
                PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + i, moveId);
                PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, ppUps);
                PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, pp);
            }
        }
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + i, 0);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, 0);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, 0);
    } else {
        PokeParty_SetMove(pkm, move, (u8)slot);
    }
    PokeParty_EncryptPkm(pkm, encrypted);
    return FALSE;
}

BOOL s0113_PokePartyIsOriginGame(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    PartyPkm *pkm = NULL;
    HeapID heapId;
    StrBuf *otName;
    StrBuf *playerName;
    u32 origin;

    if (!CheckGetPartyPokemon(env, index, &pkm)) {
        *result = FALSE;
        return FALSE;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL) != getIDAsUInt(playerInfo)) {
        *result = FALSE;
        return FALSE;
    }
    heapId = FieldScriptEnv_GetHeapID(env);
    otName = GFL_StrBufCreate(16, heapId);
    playerName = GFL_StrBufCreate(16, heapId);
    if (otName != NULL && playerName != NULL) {
        PokeParty_GetParam(pkm, PKM_PARAM_OT_NAME, otName);
        textCopy(playerInfo->name, playerName);
        if (!GFL_StrBufCmp(otName, playerName)) {
            GFL_StrBufFree(otName);
            GFL_StrBufFree(playerName);
            *result = FALSE;
            return FALSE;
        }
    } else {
        *result = FALSE;
        return FALSE;
    }
    GFL_StrBufFree(otName);
    GFL_StrBufFree(playerName);
    if (PokeParty_GetParam(pkm, PKM_PARAM_OT_GENDER, NULL) != getTrainerGender(playerInfo)) {
        *result = FALSE;
        return FALSE;
    }
    origin = PokeParty_GetParam(pkm, PKM_PARAM_ORIGIN_GAME, NULL);
    if (origin != game_version) {
        *result = FALSE;
        return FALSE;
    }
    *result = TRUE;
    return FALSE;
}

BOOL s0117_PokePartySetForme(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 forme = ScriptReadAny(vm, env);
    FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    PartyPkm *pkm = PokeParty_GetPkm(GameData_GetParty(gameData), index);
    PokeParty_ChangeForme(pkm, forme);
    PokeDex_RegistPkm(GameData_GetPokedex(gameData), pkm);
    return FALSE;
}

BOOL s011C_PokePartyChangeRotomForme(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 forme = ScriptReadAny(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    PartyPkm *pkm = PokeParty_GetPkm(GameData_GetParty(gameData), index);
    PML_PkmChangeRotomForme(pkm, slot, forme);
    PokeDex_RegistPkm(GameData_GetPokedex(gameData), pkm);
    return FALSE;
}

BOOL s01D5_MoveReminderCheckPkm(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    PartyPkm *pkm;
    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        u16 *moves = PokeParty_GetRememberableMoves(pkm, heapId);
        *result = doesPkmHaveLevelMoveToLearn(moves);
        GFL_HeapFree(moves);
    }
    return FALSE;
}

BOOL s0119_PokePartyIsFromWhiteForest(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u16 city = ScriptReadAny(vm, env);
    u16 metLocation = GetScrPokeStat(env, index, 0x95);
    if (metLocation == data_ov012_0216ca04[city]) {
        *result = TRUE;
    } else {
        *result = FALSE;
    }
    return FALSE;
}

BOOL s011A_PokePartyGetMetDate(VM *vm, FieldScriptEnv *env) {
    u16 *year = ScriptReadVar(vm, env);
    u16 *month = ScriptReadVar(vm, env);
    u16 *day = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    PartyPkm *pkm;

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        *year = PokeParty_GetParam(pkm, 0x92, NULL);
        *month = PokeParty_GetParam(pkm, 0x93, NULL);
        *day = PokeParty_GetParam(pkm, 0x94, NULL);
    } else {
        *year = 0;
        *month = 0;
        *day = 0;
    }
    return FALSE;
}

BOOL s0111_PokePartySetIV(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 param = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);
    u32 i;

    for (i = 0; i < 6; i++) {
        if (param == data_ov012_0216ca1a[2 * i]) {
            if (value <= data_ov012_0216ca1c[2 * i]) {
                PartyPkm *pkm;
                if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
                    PokeParty_SetParam(pkm, param, value);
                    PokeParty_RecalcStats(pkm);
                }
            }
            return FALSE;
        }
    }
    return FALSE;
}

BOOL s0110_PokePartyGetParam(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u16 param = ScriptReadAny(vm, env);
    u32 i;

    for (i = 0; i < 20; i++) {
        if (param == data_ov012_0216ca06[i]) {
            *result = GetScrPokeStat(env, index, param);
            return FALSE;
        }
    }
    *result = 0;
    return FALSE;
}

BOOL s0122_BoxAdd(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    HeapID heapId;
    u16 *result;
    u16 species;
    u16 form;
    u16 level;
    BoxPkmCreateParams params;

    FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    GameData_GetParty(gameData);
    GetGameDataPlayerInfo(gameData);
    result = ScriptReadVar(vm, env);
    species = ScriptReadAny(vm, env);
    form = ScriptReadAny(vm, env);
    level = ScriptReadAny(vm, env);
    params.heapId = heapId;
    params.species = species;
    params.form = form;
    params.level = level;
    params.item = 0;
    params.ability = 2;
    params.sex = 2;
    params.param1C = 2;
    params.ball = 4;
    *result = GameData_AddBoxPkm(gameData, &params);
    return FALSE;
}

BOOL s0123_BoxAddEx(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    HeapID heapId;
    u16 *result;
    u16 species;
    u16 form;
    u16 level;
    u16 ability;
    u16 sex;
    u16 param1C;
    u16 item;
    u16 ball;
    BoxPkmCreateParams params;

    FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    GameData_GetParty(gameData);
    GetGameDataPlayerInfo(gameData);
    result = ScriptReadVar(vm, env);
    species = ScriptReadAny(vm, env);
    form = ScriptReadAny(vm, env);
    level = ScriptReadAny(vm, env);
    ability = ScriptReadAny(vm, env);
    sex = ScriptReadAny(vm, env);
    param1C = ScriptReadAny(vm, env);
    item = ScriptReadAny(vm, env);
    ball = ScriptReadAny(vm, env);
    params.heapId = heapId;
    params.species = species;
    params.form = form;
    params.level = level;
    params.item = item;
    params.ability = ability;
    if (params.ability >= 3)
        params.ability = 2;
    params.sex = sex;
    if (params.sex >= 3)
        params.sex = 2;
    params.param1C = param1C;
    if (params.param1C >= 3)
        params.param1C = 2;
    params.ball = ball;
    *result = GameData_AddBoxPkm(gameData, &params);
    return FALSE;
}

BOOL s010F_PokePartyAddEgg(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    HeapID heapId;
    PokeParty *party;
    PlayerInfo *playerInfo;
    u16 *result;
    u16 species;
    u16 form;
    PartyPkm *pkm;
    StrBuf *name;
    void *personal;
    u32 cycles;

    FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    party = GameData_GetParty(gameData);
    playerInfo = GetGameDataPlayerInfo(gameData);
    result = ScriptReadVar(vm, env);
    species = ScriptReadAny(vm, env);
    form = ScriptReadAny(vm, env);
    if (PokeParty_GetCapacity(party) <= PokeParty_GetPkmCount(party)) {
        *result = FALSE;
        return FALSE;
    }
    pkm = PokeParty_NewTempPkm(species, 1, (u64)-1, heapId);
    PokeParty_SetParam(pkm, PKM_PARAM_FORM, form);
    name = copyTrainerNameToNewStrbuf((const u16 *)GetGameDataPlayerInfo(gameData), heapId);
    PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME, (u32)name);
    GFL_StrBufFree(name);
    personal = PML_PersonalLoad(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), heapId);
    cycles = PML_PersonalGetParam(personal, PERSONAL_HATCH_CYCLES);
    PML_PersonalFree(personal);
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, cycles);
    PokeParty_SetParam(pkm, PKM_PARAM_IS_EGG, 1);
    name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);
    PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)name);
    GFL_StrBufFree(name);
    PokeParty_RecalcStats(pkm);
    PokeParty_SetupMetData(pkm, 5, playerInfo, 0xea63, heapId);
    PokeParty_AddPkm(party, pkm);
    GFL_HeapFree(pkm);
    *result = TRUE;
    return FALSE;
}

PartyPkm *GameData_MakeBoxPkm(GameData *gameData, BoxPkmCreateParams *params) {
    PlayerInfo *playerInfo;
    u32 trainerId;
    u32 pid;
    PartyPkm *pkm;
    u16 zoneId;
    u16 placeName;

    playerInfo = GetGameDataPlayerInfo(gameData);
    trainerId = getIDAsUInt(GetGameDataPlayerInfo(gameData));
    pid = PML_GenPID(trainerId, (u16)params->species, (u16)params->form, params->sex, params->ability,
                     params->param1C);
    pkm = PokeParty_NewPkm((u16)params->species, (u16)params->level, trainerId, PKM_IVS_RANDOM, pid, params->heapId);
    PokeParty_ChangeForme(pkm, params->form);
    PokeParty_SetParam(pkm, 6, params->item);
    if (params->hiddenAbility != 0) {
        PokeParty_SetHiddenAbil(pkm, params->species, params->form);
    }
    if (GetItemParam((u16)params->ball, 15, params->heapId) == 4) {
        PokeParty_SetParam(pkm, 0x98, PML_ItemGetMonsBallID((u16)params->ball));
    }
    zoneId = PlayerState_GetZoneID(GameData_GetPlayerState(gameData));
    placeName = ZoneData_GetPlaceNameID(zoneId);
    PokeParty_SetupMetData(pkm, 0, playerInfo, placeName, params->heapId);
    PokeParty_RecalcStats(pkm);
    return pkm;
}

BOOL GameData_AddBoxPkm(GameData *gameData, BoxPkmCreateParams *params) {
    BoxSaveAccessor *boxes;
    PartyPkm *pkm;
    BoxPkm *boxPkm;
    PokeDexSave *dex;

    boxes = GameData_GetBoxSaveAccessor(gameData);
    if ((int)howManyTotalPokesAreInBoxes(boxes) >= 720) {
        return FALSE;
    }
    pkm = GameData_MakeBoxPkm(gameData, params);
    boxPkm = func_0201d624(pkm);
    BoxSaveAccessor_InsertPkm(boxes, boxPkm);
    dex = GameData_GetPokedex(gameData);
    addPkmToDex(dex, pkm);
    GFL_HeapFree(pkm);
    return TRUE;
}

BOOL addPkmToParty(GameData *gameData, BoxPkmCreateParams *params) {
    PokeParty *party;
    int capacity;
    PartyPkm *pkm;
    PokeDexSave *dex;

    party = GameData_GetParty(gameData);
    capacity = PokeParty_GetCapacity(party);
    if (capacity <= PokeParty_GetPkmCount(party)) {
        return FALSE;
    }
    pkm = GameData_MakeBoxPkm(gameData, params);
    PokeParty_AddPkm(party, pkm);
    dex = GameData_GetPokedex(gameData);
    addPkmToDex(dex, pkm);
    GFL_HeapFree(pkm);
    return TRUE;
}

BOOL s00F9_MoneyAdd(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    GameData_GetPlayerState(gameData);
    u32 amount = ScriptReadAny(vm, env);
    TrainerGameInfoSave *info = getTrainerCardDataBlkAddress(gameData);
    addCashToTotal(info, amount);
    return FALSE;
}

BOOL s00FA_MoneySub(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    GameData_GetPlayerState(gameData);
    u32 amount = ScriptReadAny(vm, env);
    TrainerGameInfoSave *info = getTrainerCardDataBlkAddress(gameData);
    subCashFromTotal(info, amount);
    return FALSE;
}

BOOL s00FB_MoneyCheck(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u32 amount = ScriptReadAny(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    GameData_GetPlayerState(gameData);
    TrainerGameInfoSave *info = getTrainerCardDataBlkAddress(gameData);
    *result = amount <= getCash(info);
    return FALSE;
}
