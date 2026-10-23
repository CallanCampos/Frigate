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
#include "save/records.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The C-Gear screen, which shows the saving
#define SUBSCREEN_CGEAR 7

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
    case 0:
        save->saveIcon = func_ov036_021c6cc8(heapId, field);
        func_ov036_021c6d14(save->saveIcon);
        if (FieldSubscreen_GetScreenID(subscreen) == SUBSCREEN_CGEAR) {
            func_ov036_02198b10(subscreen);
            func_ov036_02198b1c(subscreen);
        }
        RecordAddOne(GameData_GetRecords(gameData), 1);
        if (!save->pokewood) {
            GameData_StartSave(gameData);
            *state = 1;
        } else {
            GameEvent_ChainNext(event, EventPokewoodSave_Create(save->gsys, save->result));
            *state = 2;
        }
        break;
    case 1:
        switch (GameData_UpdateSave(gameData)) {
        case 0:
        case 1:
            // Still saving
            break;
        case 2:
            *save->result = TRUE;
            *state = 2;
            break;
        case 3:
            *save->result = FALSE;
            *state = 2;
            break;
        }
        break;
    case 2:
        if (*save->result == FALSE && FieldSubscreen_GetScreenID(subscreen) == SUBSCREEN_CGEAR) {
            func_ov036_02198b34(subscreen);
        }
        *state = 3;
        break;
    case 3:
        if (FieldSubscreen_GetScreenID(subscreen) != SUBSCREEN_CGEAR || func_ov036_02198b28(subscreen)) {
            *state = 4;
        }
        break;
    case 4:
        func_ov036_021c6d3c(save->saveIcon);
        func_ov036_021c6cf8(save->saveIcon);
        *state = 5;
        break;
    case 5:
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

BOOL s0137_SaveDataWrite(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    FieldScriptEnv_GetGameData(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    u16 *result = ScriptReadVar(vm, env);

    ScriptWork_CallEvent(work, EventSaveData_Create(gsys, result));
    return TRUE;
}
