#include "types.h"
#include "app/name_entry.h"
#include "field/event_phrase_input.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/beacon_status.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The longest name of modes NAME_ENTRY_GREETING to NAME_ENTRY_PASSERBY_MESSAGE
static const u8 sMaxLengths[3] = { 8, 8, 8 };

GameEvent *EventPhraseInput_Create(GameSystem *gsys, Field *field, GameEvent *parent, unsigned int mode, u16 *result) {
    GameEvent *event;
    struct EventPhraseInputData *data;
    SaveControl *save;

    event = GameEvent_Create(gsys, parent, EventPhraseInput_Callback, sizeof(struct EventPhraseInputData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->gameData = GSYS_GetGameData(gsys);
    data->field = field;
    data->result = result;
    data->mode = mode;
    data->heapId = 4;
    save = GameData_GetSaveControl(data->gameData);
    data->trainerInfo = getTrainerGameInfoAddress(save);
    save = GameData_GetSaveControl(data->gameData);
    data->saveBlock = CGearName_GetFromSave(save);
    data->playerInfo = GetGameDataPlayerInfo(data->gameData);
    data->unk18 = GameData_GetBeaconStatus(data->gameData);
    data->nameEntry.mode = mode;
    data->nameEntry.gender = getTrainerGender(data->playerInfo);
    data->nameEntry.gameInfo = data->trainerInfo;
    data->nameEntry.unk30 = data->saveBlock;
    if (mode == NAME_ENTRY_TRAINER_MESSAGE || mode == NAME_ENTRY_HAPPY_MESSAGE) {
        data->nameEntry.maxLength = 8;
    } else {
        data->nameEntry.maxLength = sMaxLengths[mode - NAME_ENTRY_GREETING];
    }
    data->nameEntry.name = GFL_StrBufCreate(data->nameEntry.maxLength + 1, data->heapId);
    data->nameEntry.unk2C = 0;
    switch (mode) {
    case NAME_ENTRY_PLAYER:
    case NAME_ENTRY_POKEMON:
    case NAME_ENTRY_BOX:
    case NAME_ENTRY_RIVAL:
    case NAME_ENTRY_FRIEND:
    case NAME_ENTRY_PASSERBY_MESSAGE:
    case NAME_ENTRY_AVENUE_NAME:
    case NAME_ENTRY_AVENUE_TITLE:
    case NAME_ENTRY_FAVORITE_PHRASE:
    case NAME_ENTRY_PLASMA_PASSWORD:
    case NAME_ENTRY_GAME_SYNC_ID:
    case NAME_ENTRY_IMPRESSED:
    case NAME_ENTRY_TRAINER_MESSAGE:
        break;
    case NAME_ENTRY_GREETING:
        GFL_StrBufLoadString(data->nameEntry.name, TrainerGameInfo_GetGreeting(data->trainerInfo));
        break;
    case NAME_ENTRY_THANKS:
        GFL_StrBufLoadString(data->nameEntry.name, TrainerGameInfo_GetThanks(data->trainerInfo));
        break;
    case NAME_ENTRY_HAPPY_MESSAGE:
        GFL_StrBufLoadString(data->nameEntry.name, CGearName_Get(data->saveBlock));
        break;
    }
    return event;
}

GameEventReturnCode EventPhraseInput_Callback(GameEvent *event, u32 *state, void *eventData) {
    struct EventPhraseInputData *data = eventData;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(data->gsys, data->field, OVERLAY_ID(280),
                                                                         &NAME_ENTRY_PROC_FUNCTIONS, &data->nameEntry));
        (*state)++;
        break;
    case 1:
        EventPhraseInput_StoreResult(data, &data->nameEntry);
        GFL_StrBufFree(data->nameEntry.name);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void EventPhraseInput_StoreResult(struct EventPhraseInputData *data, NameEntryParam *param) {
    if (data->result != NULL) {
        *data->result = param->canceled == FALSE;
    }
    if (param->canceled == TRUE) {
        return;
    }
    switch (data->mode) {
    case NAME_ENTRY_PLAYER:
    case NAME_ENTRY_POKEMON:
    case NAME_ENTRY_BOX:
    case NAME_ENTRY_RIVAL:
    case NAME_ENTRY_FRIEND:
    case NAME_ENTRY_AVENUE_NAME:
    case NAME_ENTRY_AVENUE_TITLE:
    case NAME_ENTRY_FAVORITE_PHRASE:
    case NAME_ENTRY_PLASMA_PASSWORD:
    case NAME_ENTRY_GAME_SYNC_ID:
    case NAME_ENTRY_IMPRESSED:
        break;
    case NAME_ENTRY_GREETING:
        TrainerGameInfo_SetGreeting(data->trainerInfo, param->name);
        GameBeaconSys_UpdateGreeting();
        break;
    case NAME_ENTRY_THANKS:
        TrainerGameInfo_SetThanks(data->trainerInfo, param->name);
        break;
    case NAME_ENTRY_HAPPY_MESSAGE:
        CGearName_Set(data->saveBlock, param->name);
        break;
    case NAME_ENTRY_PASSERBY_MESSAGE:
        GFL_StrBufCopy(BeaconStatus_GetGreeting(data->unk18), data->nameEntry.name);
        GameBeacon_BroadcastPhrase(param->name, param->unk34);
        break;
    case NAME_ENTRY_TRAINER_MESSAGE:
        GFL_StrBufCopy(BeaconStatus_GetGreeting(data->unk18), data->nameEntry.name);
        GameBeacon_BroadcastPhrase(param->name, param->unk34);
        break;
    }
}

BOOL Scrcmd_PhraseInput_CheckWifiListFlags(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    GameData *gameData;
    u16 input;
    u16 *result;
    DreamWorldSave *save;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    input = ScriptReadAny(vm, env);
    ScriptReadAny(vm, env);
    ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    save = GameData_GetDreamWorldSave(gameData);
    if (input == 0) {
        if (DreamWorldSave_GetIDShown(save) == 1 && DreamWorldSave_GetMode(save) == 1) {
            *result = 1;
        } else {
            *result = 0;
        }
    }
    return FALSE;
}

BOOL Scrcmd_PhraseInput_WifiListOperation(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    GameData *gameData;
    u16 input;
    u16 input2;
    u16 *result;
    DreamWorldSave *save;
    u8 i;
    u16 *value;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    input = ScriptReadAny(vm, env);
    input2 = ScriptReadAny(vm, env);
    ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    save = GameData_GetDreamWorldSave(gameData);
    switch (input) {
    case 0:
        *result = 1;
        for (i = 0; i < 5; i++) {
            value = DreamWorldSave_GetResult(save, i);
            if (value == NULL) {
                *result = 0;
            } else if (*value == 0x7e || *value == 0) {
                *result = 0;
            }
        }
        break;
    case 1:
        if (DreamWorldSave_GetResultIndex(save) == 0x7f) {
            *result = 0;
        } else {
            *result = 1;
        }
        break;
    case 2:
        if (DreamWorldSave_GetFlag(save) == 1) {
            *result = 1;
        } else {
            *result = 0;
        }
        break;
    case 3:
        DreamWorldSave_SetFlag(save, 0);
        break;
    case 4:
        DreamWorldSave_SetResultIndex(save, input2);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_PhraseInput_SetWifiFriendNameWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    GameData *gameData;
    HeapID heapId;
    WordSet *wordSet;
    DreamWorldSave *save;
    u16 input;
    u16 index;
    u16 *phrase;
    StrBuf *strbuf;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    wordSet = ScriptWork_GetWordSet(work);
    save = GameData_GetDreamWorldSave(gameData);
    input = ScriptReadAny(vm, env);
    index = ScriptReadAny(vm, env);
    phrase = DreamWorldSave_GetResult(save, index);
    if (phrase != NULL && *phrase != 0x7e && *phrase != 0) {
        strbuf = GFL_StrBufCreate(14, heapId);
        GFL_StrBufLoadFixedString(strbuf, phrase + 1, 13);
        GFL_WordSetSetStrbuf(wordSet, input, strbuf, 0, 0, 2);
        GFL_StrBufFree(strbuf);
    }
    return FALSE;
}
