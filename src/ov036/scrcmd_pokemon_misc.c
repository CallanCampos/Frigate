// Script commands of the party Pokémon that follow the item commands: the Pokémon selection screen, the Trainer Card's
// favourite Pokémon, IVs, the Pokédex, the move tutors, Pokérus, name input, move replacement, egg hatching and
// in-game trades. The ROM has no name for the file; scrcmd_pokemon_misc.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/bag.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "field/event_egg_demo.h"
#include "field/event_field_trade.h"
#include "field/event_poke_status.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/fld_trade.h"
#include "field/scrcmd_pokemon.h"
#include "field/scrcmd_pokemon_fld.h"
#include "field/scrcmd_pokemon_misc.h"
#include "gfl/overlay.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/traded_pokemon.h"
#include "save/trainer_game_info.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// What Scrcmd_PokemonMisc_PokePartyGetIV gives: the IVs' total, the highest, or one of them
enum {
    IV_QUERY_TOTAL,
    IV_QUERY_HIGHEST,
    IV_QUERY_HP,
    IV_QUERY_ATTACK,
    IV_QUERY_DEFENSE,
    IV_QUERY_SPEED,
    IV_QUERY_SP_ATTACK,
    IV_QUERY_SP_DEFENSE,
};

// The tutors of the move tutor commands, by the moves they teach: Draco Meteor, the starters' ultimate moves and
// their pledges
enum {
    MOVE_TUTOR_DRACO_METEOR,
    MOVE_TUTOR_ULTIMATE,
    MOVE_TUTOR_PLEDGE,
};

// The moves of PokeParty_CheckMoveTutorNormal's list
enum {
    TUTOR_MOVE_GRASS_PLEDGE,
    TUTOR_MOVE_FIRE_PLEDGE,
    TUTOR_MOVE_WATER_PLEDGE,
    TUTOR_MOVE_FRENZY_PLANT,
    TUTOR_MOVE_BLAST_BURN,
    TUTOR_MOVE_HYDRO_CANNON,
    TUTOR_MOVE_DRACO_METEOR,
};

// The move that the party list shows for the tutors whose move depends on the Pokémon
#define TUTOR_LIST_MOVE_ULTIMATE 0xfffe
#define TUTOR_LIST_MOVE_PLEDGE 0xfffd

// Whether a Pokémon, or any in the party, can learn the tutor's move: it can, it can't, it isn't friendly enough, or
// it knows the move
enum {
    MOVE_TUTOR_CHECK_OK,
    MOVE_TUTOR_CHECK_INCOMPATIBLE,
    MOVE_TUTOR_CHECK_NOT_FRIENDLY,
    MOVE_TUTOR_CHECK_KNOWN,
};

// What Scrcmd_PokemonMisc_PokePartyGetHiddenPowerType gives for a Pokémon that can't learn Hidden Power
#define HIDDEN_POWER_TYPE_NONE 0xffff

// Whether the Pokémon can be traded for the offer: it can, it is an egg, it is another species or sex, or it can't
// be traded at all (an egg or a fused Kyurem)
enum {
    FIELD_TRADE_CHECK_OK,
    FIELD_TRADE_CHECK_EGG,
    FIELD_TRADE_CHECK_SPECIES,
    FIELD_TRADE_CHECK_SEX,
    FIELD_TRADE_CHECK_UNTRADEABLE,
};

// What Scrcmd_PokemonMisc_ControlLegendMon does: check whether the party has the legendary Pokémon, move it to the
// front, or swap a party Pokémon with it from the box first
enum {
    LEGEND_MON_CHECK_PARTY,
    LEGEND_MON_MOVE_TO_FRONT,
    LEGEND_MON_SWAP_FROM_BOX,
};

static u16 PokeParty_GetTutorMoveID(PartyPkm *pkm, u32 tutor);
static u32 PML_PkmGetHapinessMax(u32 unused);
static BOOL PokeParty_CheckTutorMoveCompat(PartyPkm *pkm, u32 tutor);
static u32 CheckPartyAllMoveTutor(VM *vm, FieldScriptEnv *env, u32 tutor, u8 *learnable, BOOL needMaxHappiness);

BOOL Scrcmd_PokemonMisc_CallPokeSelect(VM *vm, FieldScriptEnv *env) {
    PokeSelectArgs args;
    u16 flag = VM_Read16(vm);
    u16 *picked = ScriptReadVar(vm, env);
    u16 *index = ScriptReadVar(vm, env);
    ScriptWork *work;
    GameSystem *gsys;
    u32 mode;

    ScriptReadAny(vm, env);
    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    MI_CpuFill8(&args, 0, sizeof(args));
    args.picked = picked;
    args.index = index;
    mode = 3;
    if (flag != 0) {
        mode = 0x1b;
    }
    args.mode = mode;
    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKE_STATUS, EventPokeSelect_Create, &args));
    return TRUE;
}

BOOL Scrcmd_PokemonMisc_TrainerCardSetFavePokemon(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    u32 index = ScriptReadAny(vm, env);

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        TrainerGameInfoSave *info = getTrainerCardDataBlkAddress(FieldScriptEnv_GetGameData(env));
        u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        u16 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        u16 isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        u16 sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);

        SetFavePokemonData(info, species, form, isEgg, sex);
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_TrainerCardSaveGymVictoryParty(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokeParty *party = GameData_GetParty(gameData);
    u16 gym = ScriptReadAny(vm, env);
    u16 species[6];
    int i;
    int count;

    for (i = 0; i < 6; i++) {
        species[i] = 0;
    }
    for (i = 0, count = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            species[count++] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        }
    }
    SetGymDefeatPartyData(getTrainerCardDataBlkAddress(gameData), gym, species);
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_WordSetGymVictoryParty(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u16 gym = ScriptReadAny(vm, env);
    u16 *count = ScriptReadVar(vm, env);
    u16 species[6];
    int i;
    int loaded;

    GetGymDefeatPartyData(getTrainerCardDataBlkAddress(gameData), gym, species);
    i = 0;
    loaded = 0;
    for (; i < 6; i++) {
        if (species[i] != 0) {
            WordSet_LoadSpeciesName(wordSet, loaded, species[i]);
            loaded++;
        }
    }
    *count = loaded;
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_PokePartyGetIV(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    u32 index = ScriptReadAny(vm, env);
    u32 type = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u16 ivs[6];
    u16 total;
    u16 highest;
    u32 i;

    if (!CheckGetPartyPokemon(env, index, &pkm)) {
        *result = 0;
        return FALSE;
    }
    total = 0;
    ivs[0] = PokeParty_GetParam(pkm, PKM_PARAM_IV_HP, NULL);
    ivs[1] = PokeParty_GetParam(pkm, PKM_PARAM_IV_ATTACK, NULL);
    ivs[2] = PokeParty_GetParam(pkm, PKM_PARAM_IV_DEFENSE, NULL);
    ivs[3] = PokeParty_GetParam(pkm, PKM_PARAM_IV_SPEED, NULL);
    ivs[4] = PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_ATTACK, NULL);
    ivs[5] = PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, NULL);
    highest = 0;
    for (i = 0; i < 6; i++) {
        total += ivs[i];
        if (highest < ivs[i]) {
            highest = ivs[i];
        }
    }
    switch (type) {
    case IV_QUERY_TOTAL:
        *result = total;
        break;
    case IV_QUERY_HIGHEST:
        *result = highest;
        break;
    case IV_QUERY_HP:
        *result = ivs[0];
        break;
    case IV_QUERY_ATTACK:
        *result = ivs[1];
        break;
    case IV_QUERY_DEFENSE:
        *result = ivs[2];
        break;
    case IV_QUERY_SPEED:
        *result = ivs[3];
        break;
    case IV_QUERY_SP_ATTACK:
        *result = ivs[4];
        break;
    case IV_QUERY_SP_DEFENSE:
        *result = ivs[5];
        break;
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_PokeDexGetEvaluationParams(VM *vm, FieldScriptEnv *env) {
    u16 mode = ScriptReadAny(vm, env);
    u16 *message = ScriptReadVar(vm, env);
    u16 *count = ScriptReadVar(vm, env);
    // The jingle of the evaluation, one of SEQ_ME_HYOUKA1 to 6
    u16 *jingle = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));

    PokeDex_GetEvaluationParams(pokedex, mode, heapId, message, count, jingle);
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_PokeDexIsComplete(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 *result = ScriptReadVar(vm, env);
    u16 kind = ScriptReadAny(vm, env);

    switch (kind) {
    case POKEDEX_EVALUATION_NATIONAL_CAUGHT:
        *result = PokeDex_IsCompleteNational(pokedex);
        break;
    case POKEDEX_EVALUATION_REGIONAL_CAUGHT:
        *result = PokeDex_IsCompleteUnova(pokedex, Field_GetHeapID(field));
        break;
    case POKEDEX_EVALUATION_REGIONAL_SEEN:
        *result = PokeDex_IsSeenAllUnova(pokedex, Field_GetHeapID(field));
        break;
    }
    return FALSE;
}

static u16 PokeParty_GetTutorMoveID(PartyPkm *pkm, u32 tutor) {
    switch (tutor) {
    case MOVE_TUTOR_ULTIMATE:
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_FRENZY_PLANT)) {
            return MOVE_FRENZY_PLANT;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_BLAST_BURN)) {
            return MOVE_BLAST_BURN;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_HYDRO_CANNON)) {
            return MOVE_HYDRO_CANNON;
        }
        break;
    case MOVE_TUTOR_DRACO_METEOR:
        return MOVE_DRACO_METEOR;
    case MOVE_TUTOR_PLEDGE:
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_GRASS_PLEDGE)) {
            return MOVE_GRASS_PLEDGE;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_FIRE_PLEDGE)) {
            return MOVE_FIRE_PLEDGE;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_WATER_PLEDGE)) {
            return MOVE_WATER_PLEDGE;
        }
        break;
    }
    return MOVE_BLAST_BURN;
}

static u32 PML_PkmGetHapinessMax(u32 unused) {
    return 0xff;
}

static BOOL PokeParty_CheckTutorMoveCompat(PartyPkm *pkm, u32 tutor) {
    switch (tutor) {
    case MOVE_TUTOR_ULTIMATE:
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_FRENZY_PLANT)) {
            return TRUE;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_BLAST_BURN)) {
            return TRUE;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_HYDRO_CANNON)) {
            return TRUE;
        }
        return FALSE;
    case MOVE_TUTOR_DRACO_METEOR:
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_DRACO_METEOR)) {
            return TRUE;
        }
        return FALSE;
    case MOVE_TUTOR_PLEDGE:
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_GRASS_PLEDGE)) {
            return TRUE;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_FIRE_PLEDGE)) {
            return TRUE;
        }
        if (PokeParty_CheckMoveTutorNormal(pkm, TUTOR_MOVE_WATER_PLEDGE)) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

static u32 CheckPartyAllMoveTutor(VM *vm, FieldScriptEnv *env, u32 tutor, u8 *learnable, BOOL needMaxHappiness) {
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    int count = PokeParty_GetPkmCount(party);
    BOOL anyCompatible = FALSE;
    BOOL anyReady = FALSE;
    int i;

    *learnable = 0;
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != 1 && PokeParty_CheckTutorMoveCompat(pkm, tutor)) {
            anyCompatible = TRUE;
            if (needMaxHappiness && PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL) < PML_PkmGetHapinessMax(tutor)) {
                continue;
            }
            anyReady = TRUE;
            *learnable |= 1 << i;
        }
    }
    if (!anyCompatible) {
        return MOVE_TUTOR_CHECK_INCOMPATIBLE;
    }
    if (anyReady == 0) {
        return MOVE_TUTOR_CHECK_NOT_FRIENDLY;
    }
    return MOVE_TUTOR_CHECK_OK;
}

BOOL Scrcmd_PokemonMisc_MoveTutorCheckParty(VM *vm, FieldScriptEnv *env) {
    u8 learnable;
    u16 tutor = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = CheckPartyAllMoveTutor(vm, env, tutor, &learnable, TRUE);
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_MoveTutorGetMoveID(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    u16 tutor = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    if (CheckGetPartyPokemon(env, index, &pkm) == TRUE) {
        *result = PokeParty_GetTutorMoveID(pkm, tutor);
    } else {
        *result = 0;
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_MoveTutorCheckPkm(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    u32 tutor = ScriptReadAny(vm, env);
    u32 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = MOVE_TUTOR_CHECK_INCOMPATIBLE;
    if (!CheckGetPartyPokemon(env, index, &pkm)) {
        return FALSE;
    }
    if (!PokeParty_CheckTutorMoveCompat(pkm, tutor)) {
        *result = MOVE_TUTOR_CHECK_INCOMPATIBLE;
    } else if (CheckPokeMoveLearned_NonEgg(pkm, PokeParty_GetTutorMoveID(pkm, tutor)) == TRUE) {
        *result = MOVE_TUTOR_CHECK_KNOWN;
    } else if (PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL) < PML_PkmGetHapinessMax(tutor)) {
        *result = MOVE_TUTOR_CHECK_NOT_FRIENDLY;
    } else {
        *result = MOVE_TUTOR_CHECK_OK;
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_MoveTutorCallPokeSelect(VM *vm, FieldScriptEnv *env) {
    u8 learnable;
    MoveTutorPokeSelectArgs args;
    u16 tutor = ScriptReadAny(vm, env);
    u16 *picked = ScriptReadVar(vm, env);
    u16 *index = ScriptReadVar(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    if (CheckPartyAllMoveTutor(vm, env, tutor, &learnable, FALSE) != MOVE_TUTOR_CHECK_OK) {
        *picked = 0;
    } else {
        MI_CpuFill8(&args, 0, sizeof(args));
        switch (tutor) {
        case MOVE_TUTOR_ULTIMATE:
            args.move = TUTOR_LIST_MOVE_ULTIMATE;
            break;
        case MOVE_TUTOR_PLEDGE:
            args.move = TUTOR_LIST_MOVE_PLEDGE;
            break;
        case MOVE_TUTOR_DRACO_METEOR:
            args.move = MOVE_DRACO_METEOR;
            break;
        default:
            args.move = MOVE_STRUGGLE;
            break;
        }
        args.picked = picked;
        args.index = index;
        args.learnable = learnable;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKE_STATUS,
                                                                   EventMoveTutorPokeSelect_Create, &args));
    }
    return TRUE;
}

BOOL Scrcmd_PokemonMisc_PokePartyCheckPokerus(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    PokeParty *party;
    u32 count;
    u32 i;

    i = 0;
    *result = 0;
    party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    count = PokeParty_GetPkmCount(party);
    for (; i < count; i++) {
        if (PokeParty_GetParam(PokeParty_GetPkm(party, i), PKM_PARAM_POKERUS, NULL) != 0) {
            *result = 1;
            break;
        }
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_PokePartyGetHiddenPowerType(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    u16 *result = ScriptReadVar(vm, env);
    u32 index = ScriptReadAny(vm, env);
    u8 tm = PML_ItemGetTMBitMask(ITEM_TM10);

    if (!CheckGetPartyPokemon(env, index, &pkm)) {
        *result = HIDDEN_POWER_TYPE_NONE;
    } else if (!canPkmLearnTM_Wrapper(pkm, tm)) {
        *result = HIDDEN_POWER_TYPE_NONE;
    } else {
        *result = getHiddenPowerType(pkm);
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_CallPokeNameInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 *result = ScriptReadVar(vm, env);
    u32 slot = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, prepareForPokemonNameEntryData(gsys, result, slot, ScriptReadAny(vm, env)));
    return TRUE;
}

BOOL Scrcmd_PokemonMisc_CallPokeMoveReplace(VM *vm, FieldScriptEnv *env) {
    PokeMoveReplaceArgs args;
    u16 *forgot = ScriptReadVar(vm, env);
    u16 *slot = ScriptReadVar(vm, env);
    u16 partyIndex = ScriptReadAny(vm, env);
    u16 move = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GSYS_GetField(gsys);
    MI_CpuFill8(&args, 0, sizeof(args));
    args.forgot = forgot;
    args.slot = slot;
    args.partyIndex = partyIndex;
    args.forgetHm = move == 0;
    args.move = move;
    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKE_STATUS, EventPokeMoveReplace_Create, &args));
    return TRUE;
}

BOOL Scrcmd_PokemonMisc_CallEggHatch(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData;
    PokeParty *party;
    int count;
    int i;

    Field_GetHeapID(GSYS_GetField(gsys));
    gameData = GSYS_GetGameData(gsys);
    GetGameDataPlayerInfo(gameData);
    party = GameData_GetParty(gameData);
    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        u32 isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        u32 isBadEgg = PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL);
        u32 happiness = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);

        if (isEgg != 0 && isBadEgg == 0 && happiness == 0) {
            ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(16),
                                                                       (GameEventProvider)EventEggDemo_Create, pkm));
            *result = i;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_FieldTradeCheck(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    FieldTradeInput *input;
    GameSystem *gsys;
    GameData *gameData;
    HeapID heapId;
    u16 *result;
    u32 offerIndex;
    u32 partyIndex;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = GSYS_GetGameData(gsys);
    heapId = Field_GetHeapID(GSYS_GetField(gsys));
    result = ScriptReadVar(vm, env);
    offerIndex = ScriptReadAny(vm, env);
    partyIndex = ScriptReadAny(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(gameData), partyIndex);
    input = FieldTradeInput_Create(heapId, offerIndex);

    *result = FIELD_TRADE_CHECK_OK;
    if (*result == FIELD_TRADE_CHECK_OK) {
        u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 1) {
            *result = FIELD_TRADE_CHECK_UNTRADEABLE;
        }
        if (isKyuremTransformed(species, form) == 1) {
            *result = FIELD_TRADE_CHECK_UNTRADEABLE;
        }
    }
    if (*result == FIELD_TRADE_CHECK_OK && PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 1) {
        *result = FIELD_TRADE_CHECK_EGG;
    }
    if (*result == FIELD_TRADE_CHECK_OK &&
        PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) != FieldTradeInput_GetWantedSpecies(input)) {
        *result = FIELD_TRADE_CHECK_SPECIES;
    }
    if (*result == FIELD_TRADE_CHECK_OK && FieldTradeInput_GetWantedSex(input) != GENDER_UNKNOWN &&
        PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL) != FieldTradeInput_GetWantedSex(input)) {
        *result = FIELD_TRADE_CHECK_SEX;
    }
    FieldTradeInput_Free(input);
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_FieldTradeStart(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u32 offerIndex = ScriptReadAny(vm, env);
    u32 partyIndex = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, EventFieldTrade_Create(gsys, offerIndex, partyIndex));
    return TRUE;
}

BOOL Scrcmd_PokemonMisc_FieldTradeGetSpecies(VM *vm, FieldScriptEnv *env) {
    FieldTradeInput *input;
    GameSystem *gsys;
    HeapID heapId;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetGameData(gsys);
    heapId = Field_GetHeapID(GSYS_GetField(gsys));
    result = ScriptReadVar(vm, env);
    input = FieldTradeInput_Create(heapId, ScriptReadAny(vm, env));
    *result = FieldTradeInput_GetSpecies(input);
    FieldTradeInput_Free(input);
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_ItemGetRandomOwnedTMMove(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    s32 count = BagSave_GetPocketItemCount(bag, BAG_POCKET_TMS_HMS);
    u16 move;
    u32 index = GFL_RandomLC(count);

    if (count == 0) {
        move = 0;
    } else {
        move = PML_ItemGetTMMoveID(BagSave_GetItemIndexHandle(bag, BAG_POCKET_TMS_HMS, index)->item);
    }
    *result = move;
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_FieldTradeSavePokemon(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    TradedPokemonSave *block;
    PokeParty *party;
    u32 partyIndex;
    u32 slot;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    block = GetTradedPokemonBlock(gameData);
    party = GameData_GetParty(gameData);
    partyIndex = ScriptReadAny(vm, env);
    slot = ScriptReadAny(vm, env);
    SetTradedPokemonData(block, slot, PokeParty_GetPkm(party, partyIndex));
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_WordSetLoadPastTradePkmName(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    TradedPokemonSave *block = GetTradedPokemonBlock(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u32 tradeIndex = ScriptReadAny(vm, env);
    u32 wordIndex = ScriptReadAny(vm, env);
    StrBuf *buf = ScriptWork_GetAltStrBuf(work);
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    if (TradedPokemonSave_IsTraded(block, tradeIndex)) {
        GFL_StrBufLoadString(buf, TradedPokemonSave_GetNickname(block, tradeIndex));
    } else {
        u16 empty[1] = { 0 };

        empty[0] = GFL_StrBufGetTerminator();
        GFL_StrBufLoadString(buf, empty);
    }
    GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 0, 1, 2);
    return FALSE;
}

BOOL Scrcmd_PokemonMisc_ControlLegendMon(VM *vm, FieldScriptEnv *env) {
    u16 kind = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    switch (kind) {
    case LEGEND_MON_CHECK_PARTY:
        if (GetOTZekromPartyIdx(gameData) < 0) {
            *result = 0;
        } else {
            *result = 1;
        }
        break;
    case LEGEND_MON_MOVE_TO_FRONT:
        ScrcmdPokemonFld_MoveLegendToFront(env);
        break;
    case LEGEND_MON_SWAP_FROM_BOX:
        ScrcmdPokemonFld_SwapPartyMonWithBoxedLegend(env, *result);
        ScrcmdPokemonFld_MoveLegendToFront(env);
        break;
    }
    return FALSE;
}
