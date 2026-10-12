// The script commands that check the party and the Battle Box against a battle's regulation, list the Pokémon it
// allows, and let the player pick which team enters. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "battle/regulation.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/scrcmd_fld_battle.h"
#include "gfl/heap.h"
#include "save/box.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

#define REGULATION_SPECIES_COUNT 686
#define REGULATION_LIST_MAX 32

// The result when neither team can enter
#define TEAM_SELECT_NONE 3

typedef struct {
    GameSystem *gsys;
    Field *field;
    void *select;
    PokeParty *party;
    PokeParty *battleBoxParty;
    u8 partyOk;
    u8 battleBoxOk;
    u16 heapId;
    u16 *result;
} TeamSelectEvent;

static u16 func_ov036_021aeb0c(GameData *gameData, WordSet *wordSet, u16 regulationId, HeapID heapId);
static u16 *func_ov036_021aeb64(Regulation *regulation, GameData *gameData, u16 *count, HeapID heapId);
static BOOL func_ov036_021aec28(GameSystem *gsys, u32 a1, Regulation *regulation, u32 *partyResult,
                                u32 *battleBoxResult, u16 *result, HeapID heapId);
static GameEvent *func_ov036_021aecf8(GameSystem *gsys, u32 partyResult, u32 battleBoxResult, u16 *result,
                                      HeapID heapId);
static GameEventReturnCode func_ov036_021aed8c(GameEvent *event, u32 *state, void *data);

// Set the words to the species of the regulation that the player has caught; their count
BOOL func_ov036_021aea40(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 regulationId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = func_ov036_021aeb0c(gameData, wordSet, regulationId, FieldScriptEnv_GetHeapID(env));
    return FALSE;
}

// Let the player pick the team for a battle under the regulation
BOOL func_ov036_021aea8c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 regulationId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    Regulation *regulation = PokeRegulation_LoadNew(regulationId, heapId);
    GameEvent *event = func_ov036_021aebf0(gsys, regulationId == 20 || regulationId == 21 || regulationId == 22,
                                           regulation, result, heapId);

    GFL_HeapFree(regulation);
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static u16 func_ov036_021aeb0c(GameData *gameData, WordSet *wordSet, u16 regulationId, HeapID heapId) {
    u16 i;
    u16 *species;
    u16 count = 0;
    Regulation *regulation = PokeRegulation_LoadNew(regulationId, heapId);

    species = func_ov036_021aeb64(regulation, gameData, &count, heapId);
    for (i = 0; i < count; i++) {
        WordSet_LoadSpeciesName(wordSet, i, species[i]);
    }
    GFL_HeapFree(species);
    GFL_HeapFree(regulation);
    return count;
}

// The species the regulation lists that the player has caught, at most 32
static u16 *func_ov036_021aeb64(Regulation *regulation, GameData *gameData, u16 *count, HeapID heapId) {
    int i = 0;
    PokeDexSave *pokedex;
    u16 *species;

    *count = 0;
    pokedex = GameData_GetPokedex(gameData);
    species = GFL_HeapAllocate(HEAPID_TAIL(heapId), REGULATION_LIST_MAX * sizeof(u16), TRUE, "scrcmd_fld_battle.c",
                               172);
    for (; i < REGULATION_SPECIES_COUNT; i++) {
        if ((1 << (i % 8)) & regulation->unkA[i / 8] && PokeDex_IsCaught(pokedex, i)) {
            if (*count >= REGULATION_LIST_MAX) {
                break;
            }
            species[(*count)++] = i;
        }
    }
    return species;
}

GameEvent *func_ov036_021aebf0(GameSystem *gsys, u32 a1, Regulation *regulation, u16 *result, HeapID heapId) {
    u32 partyResult;
    u32 battleBoxResult;

    if (func_ov036_021aec28(gsys, a1, regulation, &partyResult, &battleBoxResult, result, heapId) == TRUE) {
        return func_ov036_021aecf8(gsys, partyResult, battleBoxResult, result, heapId);
    }
    return NULL;
}

// Check the party and the Battle Box against the regulation; whether the player has to pick between them
static BOOL func_ov036_021aec28(GameSystem *gsys, u32 a1, Regulation *regulation, u32 *partyResult,
                                u32 *battleBoxResult, u16 *result, HeapID heapId) {
    u32 unk = 0;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party;
    BattleBoxSave *battleBox;
    PokeParty *battleBoxParty;

    unk = 0;
    party = GameData_GetParty(gameData);
    *result = TEAM_SELECT_NONE;
    if (a1) {
        *partyResult = PokeRegulation_CheckPartyCanEnter(regulation, party);
    } else {
        *partyResult = PokeRegulation_CheckParty(regulation, party, &unk);
    }
    battleBox = getBattleBox(GameData_GetSaveControl(gameData));
    if (BattleBoxSave_HasPkm(battleBox) == TRUE) {
        unk = 0;
        battleBoxParty = convertBoxedPokeSetToParty(battleBox, heapId);
        if (a1) {
            *battleBoxResult = PokeRegulation_CheckPartyCanEnter(regulation, battleBoxParty);
        } else {
            *battleBoxResult = PokeRegulation_CheckParty(regulation, battleBoxParty, &unk);
        }
        if (battleBoxParty != NULL) {
            GFL_HeapFree(battleBoxParty);
        }
    } else {
        if (*partyResult != 0) {
            *result = TEAM_SELECT_NONE;
        } else {
            *result = 0;
        }
        return FALSE;
    }
    if (*partyResult != 0 && *battleBoxResult != 0) {
        *result = TEAM_SELECT_NONE;
        return FALSE;
    }
    return TRUE;
}

u32 func_ov036_021aece0(GameSystem *gsys, u32 a1, Regulation *regulation, HeapID heapId) {
    u32 partyResult;
    u32 battleBoxResult;
    u16 result;

    return func_ov036_021aec28(gsys, a1, regulation, &partyResult, &battleBoxResult, &result, heapId);
}

static GameEvent *func_ov036_021aecf8(GameSystem *gsys, u32 partyResult, u32 battleBoxResult, u16 *result,
                                      HeapID heapId) {
    GameEvent *event;
    TeamSelectEvent *select;
    PokeParty *battleBoxParty;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party = GameData_GetParty(gameData);
    BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(gameData));

    if (BattleBoxSave_HasPkm(battleBox) == TRUE) {
        u8 partyOk;

        battleBoxParty = convertBoxedPokeSetToParty(battleBox, heapId);
        partyOk = FALSE;
        event = GameEvent_Create(gsys, NULL, func_ov036_021aed8c, sizeof(TeamSelectEvent));
        select = GameEvent_GetData(event);
        select->gsys = gsys;
        select->field = GSYS_GetField(gsys);
        select->result = result;
        select->heapId = heapId;
        select->party = party;
        select->battleBoxParty = battleBoxParty;
        if (partyResult != 0) {
            partyOk = TRUE;
        }
        select->partyOk = partyOk;
        select->battleBoxOk = battleBoxResult != 0 ? TRUE : FALSE;
    } else {
        return NULL;
    }
    return event;
}

static GameEventReturnCode func_ov036_021aed8c(GameEvent *event, u32 *state, void *data) {
    TeamSelectEvent *select = data;
    u32 cancelled;

    switch (*state) {
    case 0:
        select->select = func_ov036_021c3180(select->field, select->party, select->battleBoxParty, select->partyOk,
                                             select->battleBoxOk, select->heapId);
        (*state)++;
        break;
    case 1:
        if (func_ov036_021c3278(select->select)) {
            *select->result = func_ov036_021c3218(select->select, &cancelled);
            if (cancelled == TRUE) {
                *select->result = TEAM_SELECT_NONE;
            }
            GFL_HeapFree(select->battleBoxParty);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
