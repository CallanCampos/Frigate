// The events that ask the player for a name through the name entry (overlay 280): a Pokémon's nickname, the Hidden
// Hollow rival's and a Join Avenue passerby's. The name is descriptive: the ROM embeds none for this file
#include "types.h"
#include "app/name_entry.h"
#include "constants/pokemon.h"
#include "field/field.h"
#include "field/field_event.h"
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
    GameEventReturnCode (*callback)(GameEvent *event, u32 *state, void *data);
    StrBuf *name;
} EventPokeNameEntry;

// The name of the player on the avenue, or of one of the passerbys' messages
typedef struct {
    GameSystem *gsys;
    Field *field;
    NameEntryParam *nameEntry;
    u16 *result;
    u16 target;
    GameEventReturnCode (*callback)(GameEvent *event, u32 *state, void *data);
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

static GameEventReturnCode PokeNameEntry_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPokeNameWordSetInput_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *prepareForPokemonNameEntryData(GameSystem *gsys, u16 *result, u32 slot,
                                          GameEventReturnCode (*callback)(GameEvent *event, u32 *state, void *data)) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PartyPkm *pkm;
    StrBuf *name;
    TrainerGameInfoSave *gameInfo;
    EventPokeNameEntry *work;
    GameEvent *event;
    u32 species;
    u32 sexAndForm;

    if (PokeParty_GetPkmCount(GameData_GetParty(gameData)) <= slot) {
        slot = 0;
    }
    event = GameEvent_Create(gsys, 0, PokeNameEntry_Callback, sizeof(EventPokeNameEntry));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->result = result;
    work->callback = callback;
    gameData = GSYS_GetGameData(work->gsys);
    pkm = PokeParty_GetPkm(GameData_GetParty(gameData), slot);
    gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(gameData));
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    sexAndForm = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL) | (PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL) << 8);
    name = GFL_StrBufCreate(32, HEAPID_GAMEEVENT);
    work->name = name;
    PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, name);
    work->pkm = pkm;
    work->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, NAME_ENTRY_POKEMON, species, sexAndForm, 10, NULL, gameInfo);
    return event;
}

static GameEventReturnCode PokeNameEntry_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeNameEntry *work = data;

    if (*state == 0) {
        GameEvent_ChainNext(event, prepareForPokemonNameEntryData(work->gsys, work->result, 0, work->callback));
        (*state)++;
        return FALSE;
    }
    if (work->nameEntry->unk1C != 1) {
        if (GFL_StrBufCmp(work->nameEntry->name, work->name) == 0) {
            PokeParty_SetParam(work->pkm, PKM_PARAM_NICKNAME, (uptr)work->nameEntry->name);
        } else {
            work->nameEntry->unk1C = 1;
        }
    }
    *work->result = (work->nameEntry->unk1C != 1);
    if (*work->result == 1) {
        RecordAddOne(GameData_GetRecords(GSYS_GetGameData(work->gsys)), 0x1e);
    }
    GFL_StrBufFree(work->name);
    func_ov012_02165ae8(work->nameEntry);
    return TRUE;
}

GameEvent *JoinAvenueNameEntry_Create(u16 target, GameSystem *gsys, u16 *result, u16 useCurrent) {
    GameData *gameData = GSYS_GetGameData(gsys);
    JoinAvenueSave *save = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    JoinAvenueInfo *info = JoinAvenue_GetInfo(save);
    JoinAvenuePerson *person = func_02038a18(getAddressOfBeginningOfOccupants(save));
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(gameData));
    StrBuf *name;
    EventJoinAvenueNameEntry *work;
    GameEvent *event;
    u32 mode;
    u32 maxLength;
    u16 buffer[16];

    switch (target) {
    case 1:
        mode = NAME_ENTRY_AVENUE_TITLE;
        maxLength = 10;
        JoinAvenue_GetParam(info, 1, buffer);
        break;
    case 2:
        mode = NAME_ENTRY_FAVORITE_PHRASE;
        maxLength = 8;
        joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE1, buffer);
        break;
    case 3:
        mode = NAME_ENTRY_IMPRESSED;
        maxLength = 8;
        joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE2, buffer);
        break;
    default:
        mode = NAME_ENTRY_AVENUE_NAME;
        maxLength = 10;
        JoinAvenue_GetParam(info, 0, buffer);
        break;
    }
    name = GFL_StrBufCreate(64, 0x8004);
    GFL_StrBufLoadString(name, buffer);
    if (GFL_StrBufGetCharCount(name) > 10) {
        GFL_StrBufClear(name);
    }
    event = GameEvent_Create(gsys, 0, JoinAvenueNameEntry_Callback, sizeof(EventJoinAvenueNameEntry));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->result = result;
    work->target = target;
    work->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, mode, getTrainerGender(GetGameDataPlayerInfo(gameData)), 0,
                                     maxLength, useCurrent ? name : NULL, gameInfo);
    GFL_StrBufFree(name);
    return event;
}

GameEventReturnCode JoinAvenueNameEntry_Callback(GameEvent *event, u32 *state, void *data) {
    EventJoinAvenueNameEntry *work = data;
    u16 name[16];

    if (*state == 0) {
        GameEvent_ChainNext(event, JoinAvenueNameEntry_Create(work->target, work->gsys, work->result, 0));
        (*state)++;
        return FALSE;
    }
    if (work->nameEntry->unk1C != 1) {
        JoinAvenueSave *save = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(work->gsys)));
        JoinAvenueInfo *info = JoinAvenue_GetInfo(save);
        JoinAvenuePerson *person = func_02038a18(getAddressOfBeginningOfOccupants(save));
        const u16 *str = GFL_StrBufGetStringPtr(work->nameEntry->name);

        switch (work->target) {
        case 1:
            func_02039064(info, 1, (u32)str);
            break;
        case 2:
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_MESSAGE1, (u32)str);
            break;
        case 3:
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_MESSAGE2, (u32)str);
            break;
        default:
            func_02039064(info, 0, (u32)str);
            break;
        }
    } else if (work->target == 0) {
        JoinAvenueSave *save = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(work->gsys)));
        MsgData *msgData = GFL_MsgSysLoadData(FALSE, 0x16d, 0x2, HEAPID_GAMEEVENT);

        GFL_MsgDataLoadRawStr(msgData, 0x79, name, 0x15);
        GFL_MsgDataFree(msgData);
        func_02039064(JoinAvenue_GetInfo(save), 0, (u32)name);
    }
    if (work->result != NULL) {
        *work->result = (work->nameEntry->unk1C != 1);
    }
    func_ov012_02165ae8(work->nameEntry);
    return TRUE;
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

GameEventReturnCode RivalNameEntry_Callback(GameEvent *event, u32 *state, void *data) {
    EventRivalNameEntry *work = data;

    if (*state == 0) {
        GameEvent_ChainNext(event, RivalNameEntry_Create(work->gsys, work->result));
        (*state)++;
        return FALSE;
    }
    if (work->nameEntry->unk1C != 1) {
        SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(work->gsys));

        copyRivalNameIntoHollowBlock(getHollow_RivalData(save), GFL_StrBufGetStringPtr(work->nameEntry->name));
    }
    if (work->result != NULL) {
        *work->result = (work->nameEntry->unk1C != 1);
    }
    func_ov012_02165ae8(work->nameEntry);
    return TRUE;
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
    work->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, NAME_ENTRY_PLAYER, species, 0, 10, NULL, gameInfo);
    return event;
}

static GameEventReturnCode EventPokeNameWordSetInput_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeNameWordSetInput *work = data;
    StrBuf *speciesName;

    if (*state == 0) {
        GameEvent_ChainNext(event, EventPokeNameWordSetInput_Create(work->gsys, work->species, work->form, work->result,
                                                                    work->wordSet));
        (*state)++;
        return FALSE;
    }
    speciesName = GFL_StrBufCreate(32, HEAPID_GAMEEVENT);
    GFL_MsgDataLoadStrbuf(g_PMLSpeciesNamesResident, work->species, speciesName);
    if (work->nameEntry->unk1C == 1) {
        *work->result = 0;
    } else if (GFL_StrBufCmpIgnoreAccents(work->nameEntry->name, speciesName) == 1) {
        *work->result = 1;
        func_0202437c(work->wordSet, work->form, speciesName, 0, 1, 2);
    } else {
        *work->result = 2;
        func_0202437c(work->wordSet, work->form, work->nameEntry->name, 0, 1, 2);
    }
    GFL_StrBufFree(speciesName);
    func_ov012_02165ae8(work->nameEntry);
    return TRUE;
}
