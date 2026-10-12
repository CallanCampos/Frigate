#include "types.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/pms.h"
#include "system/vm.h"

// The script command that lets the player pick a phrase (a descriptive name). s01DA_CallPhraseSelect is swan's name

typedef struct {
    u32 mode;
    u16 *result;
    u16 *result2;
    u16 *decided;
    void *pms;
    // Which of the C-Gear's phrases the select starts from and keeps, when it does
    u32 cgearIndex;
    BOOL useCGear;
} PhraseSelectEvent;

static GameEventReturnCode func_ov012_02161114(GameEvent *event, u32 *state, void *data);

BOOL s01DA_CallPhraseSelect(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 mode = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u16 *result2 = ScriptReadVar(vm, env);
    u16 *decided = ScriptReadVar(vm, env);
    u32 kind = 0;
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov012_02161114, sizeof(PhraseSelectEvent));
    PhraseSelectEvent *data = GameEvent_GetData(event);
    PMSData sentence;

    sys_memset(data, 0, sizeof(PhraseSelectEvent));
    data->mode = mode;
    data->result = result;
    data->result2 = result2;
    data->decided = decided;
    *decided = FALSE;
    switch (mode) {
    case 0:
        data->cgearIndex = 1;
        data->useCGear = TRUE;
        kind = 2;
        break;
    case 1:
        data->cgearIndex = 2;
        data->useCGear = TRUE;
        kind = 2;
        break;
    case 2:
        data->cgearIndex = 3;
        data->useCGear = TRUE;
        kind = 2;
        break;
    case 3:
        data->cgearIndex = 4;
        data->useCGear = TRUE;
        kind = 2;
        break;
    case 4:
        data->useCGear = FALSE;
        break;
    case 5:
        data->useCGear = FALSE;
        kind = 1;
        break;
    default:
        data->pms = NULL;
        ScriptWork_CallEvent(work, event);
        return TRUE;
    }
    data->pms = PMSIParam_Create(kind, 0, 0, 0, GameData_GetSaveControl(gameData), HEAPID_USER);
    if (data->useCGear) {
        CGearPhrases_Get(getCGearDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), data->cgearIndex,
                      &sentence);
        PMSIParam_SetSentence(data->pms, &sentence);
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode func_ov012_02161114(GameEvent *event, u32 *state, void *data_) {
    PhraseSelectEvent *data = GameEvent_GetData(event);
    void *pms = data->pms;
    u16 words[2];
    PMSData sentence;

    switch (*state) {
    case 0:
        if (pms == NULL) {
            return GAMEEVENT_DONE;
        }
        GSYS_QueueProcAsEvent(event, OVERLAY_ID(185), &PMS_INPUT_PROC_FUNCTIONS, pms);
        (*state)++;
        break;
    case 1:
        if (PMSIParam_IsCanceled(pms)) {
            *data->decided = FALSE;
        } else if (data->mode == 5) {
            PMSIParam_GetWords(pms, words);
            *data->result = words[0];
            *data->result2 = words[1];
            *data->decided = TRUE;
        } else if (data->mode == 4) {
            *data->result = PMSIParam_GetWord(pms);
            *data->result2 = 0;
            *data->decided = TRUE;
        } else {
            PMSIParam_GetSentence(pms, &sentence);
            if (data->useCGear) {
                CGearPhrases_Set(
                    getCGearDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(GameEvent_GetGameSystem(event)))),
                    data->cgearIndex, &sentence);
            }
            *data->result = sentence.words[0];
            *data->result2 = sentence.words[1];
            *data->decided = TRUE;
        }
        PMSIParam_Free(data->pms);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
