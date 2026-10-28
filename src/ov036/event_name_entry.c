// The events that ask the player for a name through the name entry (overlay 280): a Pokémon's nickname, the Hidden
// Hollow rival's and a Join Avenue passerby's. The name is descriptive: the ROM embeds none for this file
#include "types.h"
#include "app/name_entry.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/shortcut_menu.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/str_tool.h"
#include "system/wordset.h"

// The nickname of a Pokémon in the party
typedef struct {
    GameSystem *gsys;
    Field *field;
    NameEntryParam *nameEntry;
    PartyPkm *pkm;
    u16 *result;
    BOOL transition;
    StrBuf *name;
} EventPokeNameEntry;

// The name of the player on the avenue, or of one of the passerbys' messages
typedef struct {
    GameSystem *gsys;
    Field *field;
    NameEntryParam *nameEntry;
    u16 *result;
    u32 target;
} EventJoinAvenueNameEntry;

// The name of the rival in the Hidden Hollow
typedef struct {
    GameSystem *gsys;
    Field *field;
    NameEntryParam *nameEntry;
    u16 *result;
} EventRivalNameEntry;

// The name of a Pokémon the entry offers as a set of words to choose from
typedef struct {
    GameSystem *gsys;
    Field *field;
    NameEntryParam *nameEntry;
    u16 *result;
    u16 species;
    u16 form;
    WordSet *wordSet;
} EventPokeNameWordSetInput;

// The records' entry that counts the Pokémon nicknamed
#define RECORD_NICKNAMES 30

// The steps of each event: call the name entry, then keep what it returns
enum {
    NAME_ENTRY_STATE_CALL,
    NAME_ENTRY_STATE_RESULT,
};

static GameEventReturnCode PokeNameEntry_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode JoinAvenueNameEntry_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode RivalNameEntry_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPokeNameWordSetInput_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *prepareForPokemonNameEntryData(GameSystem *gsys, u16 *result, int slot, BOOL transition) {
    PartyPkm *pkm;
    TrainerGameInfoSave *gameInfo;
    EventPokeNameEntry *work;
    GameEvent *event;
    GameData *gameData;
    u32 species;
    u32 sexAndForm;

    if (PokeParty_GetPkmCount(GameData_GetParty(GSYS_GetGameData(gsys))) <= slot) {
        slot = 0;
    }
    event = GameEvent_Create(gsys, 0, PokeNameEntry_Callback, sizeof(EventPokeNameEntry));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->result = result;
    work->transition = transition;
    gameData = GSYS_GetGameData(work->gsys);
    pkm = PokeParty_GetPkm(GameData_GetParty(gameData), slot);
    gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(gameData));
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    sexAndForm = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL) | (PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL) << 8);
    work->name = GFL_StrBufCreate(32, HEAPID_GAMEEVENT);
    PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, work->name);
    work->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, NAME_ENTRY_POKEMON, species, sexAndForm, 10, NULL, gameInfo);
    work->pkm = pkm;
    return event;
}

static GameEventReturnCode PokeNameEntry_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeNameEntry *work = data;

    switch (*state) {
    case NAME_ENTRY_STATE_CALL:
        GameEvent_ChainNext(
            event, ReturnToField_CreateNameInputEvent(work->gsys, work->field, work->nameEntry, work->transition));
        (*state)++;
        break;
    case NAME_ENTRY_STATE_RESULT:
        if (work->nameEntry->canceled != TRUE) {
            if (GFL_StrBufCmp(work->nameEntry->name, work->name) == 0) {
                PokeParty_SetParam(work->pkm, PKM_PARAM_NICKNAME, (u32)work->nameEntry->name);
            } else {
                work->nameEntry->canceled = TRUE;
            }
        }
        *work->result = (work->nameEntry->canceled != TRUE);
        if (*work->result == TRUE) {
            RecordAddOne(GameData_GetRecords(GSYS_GetGameData(work->gsys)), RECORD_NICKNAMES);
        }
        GFL_StrBufFree(work->name);
        NameEntryParam_Free(work->nameEntry);
        return TRUE;
    }
    return FALSE;
}

GameEvent *JoinAvenueNameEntry_Create(u16 target, GameSystem *gsys, u16 *result, u16 useCurrent) {
    GameData *gameData = GSYS_GetGameData(gsys);
    TrainerGameInfoSave *gameInfo;
    PlayerInfo *playerInfo;
    JoinAvenueOccupants *occupants;
    JoinAvenueInfo *info;
    JoinAvenueSave *save;
    JoinAvenuePerson *person;
    StrBuf *name;
    u32 mode;
    u32 maxLength;
    EventJoinAvenueNameEntry *work;
    GameEvent *event;
    u16 buffer[64];

    gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(gameData));
    playerInfo = GetGameDataPlayerInfo(gameData);
    save = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    occupants = getAddressOfBeginningOfOccupants(save);
    info = JoinAvenue_GetInfo(save);
    person = JoinAvenueOccupants_GetPlayerPerson(occupants);
    name = GFL_StrBufCreate(64, HEAPID_TAIL(HEAPID_GAMEEVENT));

    switch (target) {
    case JOIN_AVENUE_ENTRY_NAME:
    default:
        mode = NAME_ENTRY_AVENUE_NAME;
        maxLength = 10;
        JoinAvenue_GetParam(info, JOIN_AVE_INFO_NAME, buffer);
        break;
    case JOIN_AVENUE_ENTRY_TITLE:
        mode = NAME_ENTRY_AVENUE_TITLE;
        maxLength = 10;
        JoinAvenue_GetParam(info, JOIN_AVE_INFO_TITLE, buffer);
        break;
    case JOIN_AVENUE_ENTRY_PHRASE:
        mode = NAME_ENTRY_FAVORITE_PHRASE;
        maxLength = 8;
        joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE1, buffer);
        break;
    case JOIN_AVENUE_ENTRY_IMPRESSED:
        mode = NAME_ENTRY_IMPRESSED;
        maxLength = 8;
        joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE2, buffer);
        break;
    }
    GFL_StrBufLoadString(name, buffer);
    if (GFL_StrBufGetCharCount(name) > 10) {
        GFL_StrBufClear(name);
    }
    event = GameEvent_Create(gsys, 0, JoinAvenueNameEntry_Callback, sizeof(EventJoinAvenueNameEntry));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->result = result;
    if (useCurrent) {
        work->nameEntry =
            setupNameEntry(HEAPID_GAMEEVENT, mode, getTrainerGender(playerInfo), 0, maxLength, name, gameInfo);
    } else {
        work->nameEntry =
            setupNameEntry(HEAPID_GAMEEVENT, mode, getTrainerGender(playerInfo), 0, maxLength, NULL, gameInfo);
    }
    work->target = target;
    GFL_StrBufFree(name);
    return event;
}

static GameEventReturnCode JoinAvenueNameEntry_Callback(GameEvent *event, u32 *state, void *data) {
    EventJoinAvenueNameEntry *work = data;
    u16 name[22];

    switch (*state) {
    case NAME_ENTRY_STATE_CALL:
        GameEvent_ChainNext(event, ReturnToField_CreateNameInputEvent(work->gsys, work->field, work->nameEntry, TRUE));
        (*state)++;
        break;
    case NAME_ENTRY_STATE_RESULT:
        if (work->nameEntry->canceled != TRUE) {
            JoinAvenueSave *save = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(work->gsys)));
            JoinAvenueOccupants *occupants = getAddressOfBeginningOfOccupants(save);
            JoinAvenueInfo *info = JoinAvenue_GetInfo(save);
            JoinAvenuePerson *person = JoinAvenueOccupants_GetPlayerPerson(occupants);
            const u16 *str = GFL_StrBufGetStringPtr(work->nameEntry->name);

            switch (work->target) {
            case JOIN_AVENUE_ENTRY_NAME:
                JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_NAME, (u32)str);
                break;
            case JOIN_AVENUE_ENTRY_TITLE:
                JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_TITLE, (u32)str);
                break;
            case JOIN_AVENUE_ENTRY_PHRASE:
                JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_MESSAGE1, (u32)str);
                break;
            case JOIN_AVENUE_ENTRY_IMPRESSED:
                JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_MESSAGE2, (u32)str);
                break;
            }
        } else if (work->target == JOIN_AVENUE_ENTRY_NAME) {
            JoinAvenueSave *save = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(work->gsys)));
            JoinAvenueInfo *info;
            MsgData *msgData;

            getAddressOfBeginningOfOccupants(save);
            info = JoinAvenue_GetInfo(save);
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, HEAPID_GAMEEVENT);
            GFL_MsgDataLoadRawStr(msgData, 0x79, name, 0x15);
            GFL_MsgDataFree(msgData);
            JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_NAME, (u32)name);
        }
        if (work->result != NULL) {
            *work->result = (work->nameEntry->canceled != TRUE);
        }
        NameEntryParam_Free(work->nameEntry);
        return TRUE;
    }
    return FALSE;
}

GameEvent *RivalNameEntry_Create(GameSystem *gsys, u16 *result) {
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    EventRivalNameEntry *work;
    GameEvent *event;

    event = GameEvent_Create(gsys, 0, RivalNameEntry_Callback, sizeof(EventRivalNameEntry));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->result = result;
    work->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, NAME_ENTRY_RIVAL, 0, 0, 7, NULL, gameInfo);
    return event;
}

static GameEventReturnCode RivalNameEntry_Callback(GameEvent *event, u32 *state, void *data) {
    EventRivalNameEntry *work = data;

    switch (*state) {
    case NAME_ENTRY_STATE_CALL:
        GameEvent_ChainNext(event, ReturnToField_CreateNameInputEvent(work->gsys, work->field, work->nameEntry, TRUE));
        (*state)++;
        break;
    case NAME_ENTRY_STATE_RESULT:
        if (work->nameEntry->canceled != TRUE) {
            SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(work->gsys));

            copyRivalNameIntoHollowBlock(getHollow_RivalData(save), GFL_StrBufGetStringPtr(work->nameEntry->name));
        }
        if (work->result != NULL) {
            *work->result = (work->nameEntry->canceled != TRUE);
        }
        NameEntryParam_Free(work->nameEntry);
        return TRUE;
    }
    return FALSE;
}

GameEvent *EventPokeNameWordSetInput_Create(GameSystem *gsys, u16 species, u16 form, u16 *result, WordSet *wordSet) {
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    EventPokeNameWordSetInput *work;
    GameEvent *event;

    event = GameEvent_Create(gsys, 0, EventPokeNameWordSetInput_Callback, sizeof(EventPokeNameWordSetInput));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->result = result;
    work->form = form;
    work->species = species;
    work->wordSet = wordSet;
    work->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, NAME_ENTRY_POKEMON_RENAME, species, 0, 10, NULL, gameInfo);
    return event;
}

static GameEventReturnCode EventPokeNameWordSetInput_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeNameWordSetInput *work = data;
    StrBuf *speciesName;

    switch (*state) {
    case NAME_ENTRY_STATE_CALL:
        GameEvent_ChainNext(event, ReturnToField_CreateNameInputEvent(work->gsys, work->field, work->nameEntry, TRUE));
        (*state)++;
        break;
    case NAME_ENTRY_STATE_RESULT:
        speciesName = GFL_StrBufCreate(32, HEAPID_GAMEEVENT);
        GFL_MsgDataLoadStrbuf(g_PMLSpeciesNamesResident, work->species, speciesName);
        if (work->nameEntry->canceled != TRUE) {
            if (GFL_StrBufCmpIgnoreAccents(work->nameEntry->name, speciesName) == TRUE) {
                *work->result = POKE_NAME_INPUT_SPECIES;
                GFL_WordSetSetStrbuf(work->wordSet, work->form, speciesName, 0, 1, 2);
            } else {
                *work->result = POKE_NAME_INPUT_NICKNAME;
                GFL_WordSetSetStrbuf(work->wordSet, work->form, work->nameEntry->name, 0, 1, 2);
            }
        } else {
            *work->result = POKE_NAME_INPUT_CANCELED;
        }
        GFL_StrBufFree(speciesName);
        NameEntryParam_Free(work->nameEntry);
        return TRUE;
    }
    return FALSE;
}
