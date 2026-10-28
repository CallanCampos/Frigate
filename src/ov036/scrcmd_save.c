// The script command that saves the game, with the saving icon, through the Pokéwood system when it runs. The ROM has
// no name for the file; scrcmd_save.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_script.h"
#include "field/pokewood_system.h"
#include "field/scrcmd_save.h"
#include "field/subscreen.h"
#include "gfl/backup_system.h"
#include "save/records.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The save's steps: start it with the saving icon, wait for it, stop the report screen's bar if it failed, wait for the
// bar to fill, and take the icon away
enum {
    SAVE_STATE_START,
    SAVE_STATE_WAIT_SAVE,
    SAVE_STATE_STOP_BAR,
    SAVE_STATE_WAIT_BAR,
    SAVE_STATE_END,
    SAVE_STATE_DONE,
};

typedef struct {
    GameSystem *gsys;
    FieldSaveAnime *saveIcon;
    // Set to whether the save succeeded
    u16 *result;
    BOOL pokewood;
} SaveEvent;

static GameEventReturnCode EventSaveData_Callback(GameEvent *event, u32 *state, void *data) {
    SaveEvent *save = data;
    GameData *gameData = GSYS_GetGameData(save->gsys);
    Field *field = GSYS_GetField(save->gsys);
    FieldSubscreen *subscreen = Field_GetSubscreen(field);
    HeapID heapId = Field_GetHeapID(field);

    switch (*state) {
    case SAVE_STATE_START:
        save->saveIcon = FieldSaveanime_Create(heapId, field);
        FieldSaveanime_Start(save->saveIcon);
        if (FieldSubscreen_GetScreenID(subscreen) == FLD_SUBSCREEN_ID_SAVE_REPORT) {
            FieldSubscreen_Report_InitSave(subscreen);
            FieldSubscreen_Report_StartBar(subscreen);
        }
        RecordAddOne(GameData_GetRecords(gameData), 1);
        if (!save->pokewood) {
            GameData_StartSave(gameData);
            *state = SAVE_STATE_WAIT_SAVE;
        } else {
            GameEvent_ChainNext(event, EventPokewoodSave_Create(save->gsys, save->result));
            *state = SAVE_STATE_STOP_BAR;
        }
        break;
    case SAVE_STATE_WAIT_SAVE:
        switch (GameData_UpdateSave(gameData)) {
        case SAVE_RESULT_CONTINUE:
        case SAVE_RESULT_LAST:
            // Still saving
            break;
        case SAVE_RESULT_OK:
            *save->result = TRUE;
            *state = SAVE_STATE_STOP_BAR;
            break;
        case SAVE_RESULT_NG:
            *save->result = FALSE;
            *state = SAVE_STATE_STOP_BAR;
            break;
        }
        break;
    case SAVE_STATE_STOP_BAR:
        if (*save->result == FALSE && FieldSubscreen_GetScreenID(subscreen) == FLD_SUBSCREEN_ID_SAVE_REPORT) {
            FieldSubscreen_Report_StopBar(subscreen);
        }
        *state = SAVE_STATE_WAIT_BAR;
        break;
    case SAVE_STATE_WAIT_BAR:
        if (FieldSubscreen_GetScreenID(subscreen) != FLD_SUBSCREEN_ID_SAVE_REPORT ||
            FieldSubscreen_Report_IsBarFull(subscreen)) {
            *state = SAVE_STATE_END;
        }
        break;
    case SAVE_STATE_END:
        FieldSaveanime_Stop(save->saveIcon);
        FieldSaveanime_Delete(save->saveIcon);
        *state = SAVE_STATE_DONE;
        break;
    case SAVE_STATE_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventSaveData_Create(GameSystem *gsys, u16 *result) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventSaveData_Callback, sizeof(SaveEvent));
    SaveEvent *save = GameEvent_GetData(event);
    PokewoodSystem **pokewood;

    save->gsys = gsys;
    save->saveIcon = NULL;
    save->result = result;
    save->pokewood = FALSE;
    pokewood = GameData_GetPokewoodSystemPtr(GSYS_GetGameData(gsys));
    if (pokewood != NULL && *pokewood != NULL) {
        save->pokewood = TRUE;
    }
    return event;
}

BOOL Scrcmd_Save_SaveDataWrite(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    FieldScriptEnv_GetGameData(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    u16 *result = ScriptReadVar(vm, env);

    ScriptWork_CallEvent(work, EventSaveData_Create(gsys, result));
    return TRUE;
}
