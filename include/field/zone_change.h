#ifndef POKEBW2_FIELD_ZONE_CHANGE_H
#define POKEBW2_FIELD_ZONE_CHANGE_H

// Overlay 12's zone_change.c, what the game resets when the player enters a zone

#include "types.h"
#include "struct_decls.h"

void ZoneChange_OnSeamless(GameData *gameData, Field *field);
void ZoneChange_OnMapChange(GameData *gameData, u16 zoneId);
void ZoneChange_OnFly(GameData *gameData, u16 zoneId);
void ZoneChange_OnTeleport(GameData *gameData, u16 zoneId);
void ZoneChange_OnEscape(GameData *gameData, u16 zoneId);
void ZoneChange_OnBlackout(GameData *gameData, u16 zoneId);
void ZoneChange_OnGameStart(GameData *gameData, u16 zoneId);
void ZoneChange_OnFieldLoad(GameData *gameData, u16 zoneId);
// Resets the strength boulders of zone 0x1e5
void ZoneChange_ResetStrengthRocks(MMSys *system);

#endif // POKEBW2_FIELD_ZONE_CHANGE_H
