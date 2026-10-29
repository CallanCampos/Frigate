#include "types.h"
#include "app/un_select.h"
#include "field/field_script.h"
#include "field/unity_tower.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The script commands of the Unity Tower (a descriptive name). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The floor select's parameter, and what the event writes its result to
typedef struct {
    UnSelectParam param;
    u32 unk118;
    u16 *floorResult;
    u16 *countryResult;
    u16 *selectedResult;
} UnityTowerFloorSelectEvent;

static u8 UnityTowerState_GetTrainerClass(const UnityTowerFloor *state, u8 index);
static u8 UnityTowerState_GetParam(const UnityTowerFloor *state, u16 param);
static GameEvent *EventUnityTowerFloorSelect_Create(GameSystem *gsys, u16 floor, u16 *floorResult, u16 *countryResult,
                                                    u16 *selectedResult);
static GameEventReturnCode EventUnityTowerFloorSelect_Callback(GameEvent *event, u32 *state, void *data);

static u8 UnityTowerState_GetTrainerClass(const UnityTowerFloor *state, u8 index) {
    if (index >= state->count) {
        return 11;
    }
    return state->trainerClasses[index];
}

static u8 UnityTowerState_GetParam(const UnityTowerFloor *state, u16 param) {
    switch (param) {
    case 0:
        return state->count;
    case 1:
        return state->floor;
    case 2:
        return state->value;
    }
    return 0;
}

static GameEvent *EventUnityTowerFloorSelect_Create(GameSystem *gsys, u16 floor, u16 *floorResult, u16 *countryResult,
                                                    u16 *selectedResult) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventUnityTowerFloorSelect_Callback, sizeof(UnityTowerFloorSelectEvent));
    UnityTowerFloorSelectEvent *work = GameEvent_GetData(event);
    UnityTowerSurveySave *survey;
    int country;
    int i;
    int count;

    work->param.floor = floor;
    work->selectedResult = selectedResult;
    work->floorResult = floorResult;
    work->countryResult = countryResult;
    survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    for (country = 1; country < UNITY_TOWER_COUNTRY_MAX + 1; country++) {
        i = country - 1;
        if (UnityTower_HasMetCountry(survey, country)) {
            work->param.countryVisited[i] = TRUE;
        } else {
            work->param.countryVisited[i] = FALSE;
        }
    }
    count = UnityTower_CountVisitors(survey);
    if (count > UNITY_TOWER_VISITOR_MAX) {
        count = UNITY_TOWER_VISITOR_MAX;
    }
    for (i = 0; i < count; i++) {
        work->param.visitorCountries[i] = UnityTowerVisitor_GetCountry(UnityTower_GetVisitor(survey, i));
    }
    for (; i < UNITY_TOWER_VISITOR_MAX; i++) {
        work->param.visitorCountries[i] = 0;
    }
    return event;
}

static GameEventReturnCode EventUnityTowerFloorSelect_Callback(GameEvent *event, u32 *state, void *data) {
    UnityTowerFloorSelectEvent *work = GameEvent_GetData(event);

    switch (*state) {
    case 0:
        GSYS_QueueProcAsEvent(event, OVERLAY_UN_SELECT, &UN_SELECT_PROC_FUNCTIONS, &work->param);
        (*state)++;
        break;
    case 1:
        if (work->param.selected == TRUE) {
            *work->selectedResult = TRUE;
            *work->floorResult = work->param.selectedFloor;
            *work->countryResult = work->param.selectedCountry;
        } else {
            *work->selectedResult = FALSE;
            *work->floorResult = 0;
            *work->countryResult = 0;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL Scrcmd_UnityTower_UnityTowerGetTrainerClass(VM *vm, FieldScriptEnv *env) {
    const UnityTowerFloor *state;
    u8 index;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    state = GameData_GetUnityTowerSave(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    index = VM_Read16(vm);
    result = ScriptReadVar(vm, env);
    *result = UnityTowerState_GetTrainerClass(state, index);
    return FALSE;
}

BOOL Scrcmd_UnityTower_UnityTowerGetStateParam(VM *vm, FieldScriptEnv *env) {
    const UnityTowerFloor *state;
    u16 param;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    state = GameData_GetUnityTowerSave(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    param = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    *result = UnityTowerState_GetParam(state, param);
    return FALSE;
}

BOOL Scrcmd_UnityTower_UnityTowerCallFloorSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 floor;
    u16 *floorResult;
    u16 *countryResult;
    u16 *selectedResult;
    GameEvent *event;

    GameData_GetUnityTowerSave(GSYS_GetGameData(gsys));
    floor = ScriptReadAny(vm, env);
    floorResult = ScriptReadVar(vm, env);
    countryResult = ScriptReadVar(vm, env);
    selectedResult = ScriptReadVar(vm, env);
    event = EventUnityTowerFloorSelect_Create(gsys, floor, floorResult, countryResult, selectedResult);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}
