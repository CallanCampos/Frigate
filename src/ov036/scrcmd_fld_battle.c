// The script commands that check the party and the Battle Box against a battle's regulation, list the Pokémon it
// allows, and let the player pick which team enters. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "battle/regulation.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/scrcmd_fld_battle.h"
#include "gfl/heap.h"
#include "save/box.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

#define REGULATION_SPECIES_COUNT 686
#define REGULATION_LIST_MAX 32

// The team that enters, as PartySelectList_Delete returns its row: the party, the Battle Box, cancel, or neither when
// neither can enter or the team picked fails the regulation
enum {
    TEAM_SELECT_PARTY,
    TEAM_SELECT_BATTLE_BOX,
    TEAM_SELECT_CANCEL,
    TEAM_SELECT_NONE,
};

typedef struct {
    GameSystem *gsys;
    Field *field;
    PartySelectList *select;
    PokeParty *party;
    PokeParty *battleBoxParty;
    // Set when the team fails the regulation, which PartySelectList refuses
    u8 partyIneligible;
    u8 battleBoxIneligible;
    u16 heapId;
    u16 *result;
} TeamSelectEvent;

static u16 Regulation_SetCaughtSpeciesWords(GameData *gameData, WordSet *wordSet, u16 regulationId, HeapID heapId);
static u16 *Regulation_ListCaughtSpecies(Regulation *regulation, GameData *gameData, u16 *count, HeapID heapId);
static BOOL TeamSelect_CheckTeams(GameSystem *gsys, u32 pickFromParty, Regulation *regulation, u32 *partyResult,
                                  u32 *battleBoxResult, u16 *result, HeapID heapId);
static GameEvent *TeamSelectEvent_Create(GameSystem *gsys, u32 partyResult, u32 battleBoxResult, u16 *result,
                                         HeapID heapId);
static GameEventReturnCode TeamSelectEvent_Callback(GameEvent *event, u32 *state, void *data);

// Set the words to the species of the regulation that the player has caught; their count
BOOL Scrcmd_FldBattle_RegisterAllowedSpecies(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 regulationId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = Regulation_SetCaughtSpeciesWords(gameData, wordSet, regulationId, FieldScriptEnv_GetHeapID(env));
    return FALSE;
}

// Let the player pick the team for a battle under the regulation
BOOL Scrcmd_FldBattle_CheckPartyRegulation(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 regulationId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    Regulation *regulation = PokeRegulation_LoadNew(regulationId, heapId);
    GameEvent *event = Regulation_CreatePartyCheckTask(
        gsys, regulationId == 20 || regulationId == 21 || regulationId == 22, regulation, result, heapId);

    GFL_HeapFree(regulation);
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static u16 Regulation_SetCaughtSpeciesWords(GameData *gameData, WordSet *wordSet, u16 regulationId, HeapID heapId) {
    u16 i;
    u16 *species;
    u16 count = 0;
    Regulation *regulation = PokeRegulation_LoadNew(regulationId, heapId);

    species = Regulation_ListCaughtSpecies(regulation, gameData, &count, heapId);
    for (i = 0; i < count; i++) {
        WordSet_LoadSpeciesName(wordSet, i, species[i]);
    }
    GFL_HeapFree(species);
    GFL_HeapFree(regulation);
    return count;
}

// The species the regulation lists that the player has caught, at most 32
static u16 *Regulation_ListCaughtSpecies(Regulation *regulation, GameData *gameData, u16 *count, HeapID heapId) {
    int i = 0;
    PokeDexSave *pokedex;
    u16 *species;

    *count = 0;
    pokedex = GameData_GetPokedex(gameData);
    species =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), REGULATION_LIST_MAX * sizeof(u16), TRUE, "scrcmd_fld_battle.c", 172);
    for (; i < REGULATION_SPECIES_COUNT; i++) {
        if ((1 << (i % 8)) & regulation->bannedSpecies[i / 8] && PokeDex_IsCaught(pokedex, i)) {
            if (*count >= REGULATION_LIST_MAX) {
                break;
            }
            species[(*count)++] = i;
        }
    }
    return species;
}

GameEvent *Regulation_CreatePartyCheckTask(GameSystem *gsys, u32 pickFromParty, Regulation *regulation, u16 *result,
                                           HeapID heapId) {
    u32 partyResult;
    u32 battleBoxResult;

    if (TeamSelect_CheckTeams(gsys, pickFromParty, regulation, &partyResult, &battleBoxResult, result, heapId) ==
        TRUE) {
        return TeamSelectEvent_Create(gsys, partyResult, battleBoxResult, result, heapId);
    }
    return NULL;
}

// Check the party and the Battle Box against the regulation, only whether a team can be picked from each when
// pickFromParty is set; whether the player has to pick between them
static BOOL TeamSelect_CheckTeams(GameSystem *gsys, u32 pickFromParty, Regulation *regulation, u32 *partyResult,
                                  u32 *battleBoxResult, u16 *result, HeapID heapId) {
    u32 failFlags = 0;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party;
    BattleBoxSave *battleBox;
    PokeParty *battleBoxParty;

    failFlags = 0;
    party = GameData_GetParty(gameData);
    *result = TEAM_SELECT_NONE;
    if (pickFromParty) {
        *partyResult = PokeRegulation_CheckPartyCanEnter(regulation, party);
    } else {
        *partyResult = PokeRegulation_CheckParty(regulation, party, &failFlags);
    }
    battleBox = getBattleBox(GameData_GetSaveControl(gameData));
    if (BattleBoxSave_HasPkm(battleBox) == TRUE) {
        failFlags = 0;
        battleBoxParty = convertBoxedPokeSetToParty(battleBox, heapId);
        if (pickFromParty) {
            *battleBoxResult = PokeRegulation_CheckPartyCanEnter(regulation, battleBoxParty);
        } else {
            *battleBoxResult = PokeRegulation_CheckParty(regulation, battleBoxParty, &failFlags);
        }
        if (battleBoxParty != NULL) {
            GFL_HeapFree(battleBoxParty);
        }
    } else {
        if (*partyResult != REGULATION_RESULT_OK) {
            *result = TEAM_SELECT_NONE;
        } else {
            *result = TEAM_SELECT_PARTY;
        }
        return FALSE;
    }
    if (*partyResult != REGULATION_RESULT_OK && *battleBoxResult != REGULATION_RESULT_OK) {
        *result = TEAM_SELECT_NONE;
        return FALSE;
    }
    return TRUE;
}

u32 Regulation_CheckTeams(GameSystem *gsys, u32 pickFromParty, Regulation *regulation, HeapID heapId) {
    u32 partyResult;
    u32 battleBoxResult;
    u16 result;

    return TeamSelect_CheckTeams(gsys, pickFromParty, regulation, &partyResult, &battleBoxResult, &result, heapId);
}

static GameEvent *TeamSelectEvent_Create(GameSystem *gsys, u32 partyResult, u32 battleBoxResult, u16 *result,
                                         HeapID heapId) {
    GameEvent *event;
    TeamSelectEvent *select;
    PokeParty *battleBoxParty;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party = GameData_GetParty(gameData);
    BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(gameData));

    if (BattleBoxSave_HasPkm(battleBox) == TRUE) {
        battleBoxParty = convertBoxedPokeSetToParty(battleBox, heapId);
        event = GameEvent_Create(gsys, NULL, TeamSelectEvent_Callback, sizeof(TeamSelectEvent));
        select = GameEvent_GetData(event);
        select->gsys = gsys;
        select->field = GSYS_GetField(gsys);
        select->result = result;
        select->heapId = heapId;
        select->party = party;
        select->battleBoxParty = battleBoxParty;
        select->partyIneligible = partyResult != REGULATION_RESULT_OK;
        select->battleBoxIneligible = battleBoxResult != REGULATION_RESULT_OK ? TRUE : FALSE;
    } else {
        return NULL;
    }
    return event;
}

// The steps of the team select: open the party select list, then wait for the choice
enum {
    TEAM_SELECT_STEP_OPEN,
    TEAM_SELECT_STEP_WAIT,
};

static GameEventReturnCode TeamSelectEvent_Callback(GameEvent *event, u32 *state, void *data) {
    TeamSelectEvent *select = data;
    u32 ineligible;

    switch (*state) {
    case TEAM_SELECT_STEP_OPEN:
        select->select = PartySelectList_Create(select->field, select->party, select->battleBoxParty,
                                                select->partyIneligible, select->battleBoxIneligible, select->heapId);
        (*state)++;
        break;
    case TEAM_SELECT_STEP_WAIT:
        if (PartySelectList_Main(select->select)) {
            *select->result = PartySelectList_Delete(select->select, &ineligible);
            if (ineligible == TRUE) {
                *select->result = TEAM_SELECT_NONE;
            }
            GFL_HeapFree(select->battleBoxParty);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
