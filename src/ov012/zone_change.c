#include "types.h"
#include "field/encounter.h"
#include "field/event_data.h"
#include "field/field_actor.h"
#include "field/field_status.h"
#include "field/hidden_item.h"
#include "field/zone.h"
#include "field/zone_change.h"
#include "save/event_work.h"
#include "save/save_control.h"
#include "system/game_data.h"

void ZoneChange_OnSeamless(GameData *gameData, Field *field) {
    SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));
    func_ov012_021591f4(gameData);
    EventWork_FlagReset(GameData_GetEventWork(gameData), 0x964);
    func_ov012_0215917c(gameData, field);
}

void ZoneChange_OnMapChange(GameData *gameData, u16 zoneId) {
    FieldStatus *status = GameData_GetFieldStatus(gameData);
    EventWork *eventWork;

    if (!(GetZoneFlashFlags(zoneId) & 3)) {
        FieldStatus_SetFlashUsed(status, FALSE);
    }
    eventWork = GameData_GetEventWork(gameData);
    EventWork_FlagReset(eventWork, 0x964);
    TryClearRepeatableHiddenItemFlags(eventWork);
    if (IsZoneAbyssalRuinsInside(zoneId) == FALSE) {
        PlayerSave_SetAbyssalRuinsStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)), 0);
    }
}

void ZoneChange_OnFly(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnTeleport(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnEscape(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnBlackout(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnGameStart(GameData *gameData, u16 zoneId) {
}

void ZoneChange_OnFieldLoad(GameData *gameData, u16 zoneId) {
}

void ZoneChange_ResetStrengthRocks(MMSys *system) {
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 0);
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 1);
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 2);
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 3);
}
