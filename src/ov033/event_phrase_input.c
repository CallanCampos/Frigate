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

// The longest name of modes 5 to 7
static const u8 sMaxLengths[3] = { 8, 8, 8 };

GameEvent *EventPhraseInput_Create(GameSystem *gsys, Field *field, GameEvent *parent, u32 mode, u16 *result) {
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
    data->saveBlock = func_020114f0(save);
    data->playerInfo = GetGameDataPlayerInfo(data->gameData);
    data->unk18 = func_020174d4(data->gameData);
    data->nameEntry.mode = mode;
    data->nameEntry.gender = getTrainerGender(data->playerInfo);
    data->nameEntry.gameInfo = data->trainerInfo;
    data->nameEntry.unk30 = data->saveBlock;
    if (mode == 14 || mode == 15) {
        data->nameEntry.maxLength = 8;
    } else {
        data->nameEntry.maxLength = sMaxLengths[mode - 5];
    }
    data->nameEntry.name = GFL_StrBufCreate(data->nameEntry.maxLength + 1, data->heapId);
    data->nameEntry.unk2C = 0;
    switch (mode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        break;
    case 5:
        GFL_StrBufLoadString(data->nameEntry.name, TrainerGameInfo_GetGreeting(data->trainerInfo));
        break;
    case 6:
        GFL_StrBufLoadString(data->nameEntry.name, TrainerGameInfo_GetThanks(data->trainerInfo));
        break;
    case 15:
        GFL_StrBufLoadString(data->nameEntry.name, func_0201150c(data->saveBlock));
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
        func_ov033_02177734(data, &data->nameEntry);
        GFL_StrBufFree(data->nameEntry.name);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void func_ov033_02177734(struct EventPhraseInputData *data, NameEntryParam *param) {
    if (data->result != NULL) {
        *data->result = param->unk1C == 0;
    }
    if (param->unk1C == 1) {
        return;
    }
    switch (data->mode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    case 5:
        TrainerGameInfo_SetGreeting(data->trainerInfo, param->name);
        GameBeaconSys_UpdateGreeting();
        break;
    case 6:
        TrainerGameInfo_SetThanks(data->trainerInfo, param->name);
        break;
    case 15:
        func_020114fc(data->saveBlock, param->name);
        break;
    case 7:
        GFL_StrBufCopy(BeaconStatus_GetGreeting(data->unk18), data->nameEntry.name);
        func_ov012_021603ec(param->name, param->unk34);
        break;
    case 14:
        GFL_StrBufCopy(BeaconStatus_GetGreeting(data->unk18), data->nameEntry.name);
        func_ov012_021603ec(param->name, param->unk34);
        break;
    }
}

BOOL func_ov033_021777dc(VM *vm, FieldScriptEnv *env) {
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
    save = func_020179e4(gameData);
    if (input == 0) {
        if (func_020099f4(save) == 1 && func_020099e0(save) == 1) {
            *result = 1;
        } else {
            *result = 0;
        }
    }
    return FALSE;
}

BOOL func_ov033_02177844(VM *vm, FieldScriptEnv *env) {
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
    save = func_020179e4(gameData);
    switch (input) {
    case 0:
        *result = 1;
        for (i = 0; i < 5; i++) {
            value = func_02009a98(save, i);
            if (value == NULL) {
                *result = 0;
            } else if (*value == 0x7e || *value == 0) {
                *result = 0;
            }
        }
        break;
    case 1:
        if (func_02009ae0(save) == 0x7f) {
            *result = 0;
        } else {
            *result = 1;
        }
        break;
    case 2:
        if (func_02009b20(save) == 1) {
            *result = 1;
        } else {
            *result = 0;
        }
        break;
    case 3:
        func_02009b30(save, 0);
        break;
    case 4:
        func_02009af8(save, input2);
        break;
    }
    return FALSE;
}

BOOL func_ov033_02177908(VM *vm, FieldScriptEnv *env) {
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
    save = func_020179e4(gameData);
    input = ScriptReadAny(vm, env);
    index = ScriptReadAny(vm, env);
    phrase = func_02009a98(save, index);
    if (phrase != NULL && *phrase != 0x7e && *phrase != 0) {
        strbuf = GFL_StrBufCreate(14, heapId);
        GFL_StrBufLoadFixedString(strbuf, phrase + 1, 13);
        func_0202437c(wordSet, input, strbuf, 0, 0, 2);
        GFL_StrBufFree(strbuf);
    }
    return FALSE;
}
