#ifndef POKEBW2_FIELD_EVENT_MAPCHANGE_H
#define POKEBW2_FIELD_EVENT_MAPCHANGE_H

#include "types.h"
#include "field/zone.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

GameEvent *CreateGameEntryPointEvent(GameSystem *gsys, GameSystemProcData *procData);
GameEvent *EventGameOpening_Create(GameSystem *gsys, GameSystemProcData *procData);
GameEvent *EventFieldFirst_Create(GameSystem *gsys, GameSystemProcData *procData);
GameEvent *EventFieldContinue_Create(GameSystem *gsys, GameSystemProcData *procData);
GameEvent *EventMapChangeWarp_CreateGrid(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir,
                                         BOOL unk40);
GameEvent *EventMapChangeWarp_CreateRail(GameSystem *gsys, Field *field, u16 zoneId, const RailPosition *pos, u16 dir,
                                         BOOL unk40);
GameEvent *EventMapChange_CreateRail(GameSystem *gsys, Field *field, u16 zoneId, const RailPosition *pos, u16 dir,
                                     BOOL unk40);
GameEvent *EventMapChangeQuicksand_Create(GameSystem *gsys, Field *field, const VecFx32 *effectPos, u16 zoneId,
                                          VecFx32 *pos);
GameEvent *EventMapChangeEscapeRope_Create(Field *field, GameSystem *gsys);
GameEvent *EventMapChangeDig_Create(GameSystem *gsys);
GameEvent *EventMapChangeTeleport_Create(GameSystem *gsys);
GameEvent *EventMapChangeDiveOut_Create(GameSystem *gsys);
GameEvent *EventMapChangeDiveIn_Create(GameSystem *gsys, u16 zoneId);
GameEvent *EventMapChangeWarpPad_Create(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir);
GameEvent *EventUnionRoomWarp_Create(GameSystem *gsys, Field *field);
GameEvent *EventMapChangeUnionRoomExit_Create(GameSystem *gsys, Field *field);
GameEvent *EventEntralinkWarpIn_Create(GameSystem *gsys, u16 zoneId, const VecFx32 *pos, u32 a3);
GameEvent *EventEntralinkWarpIn_CreateCore(GameSystem *gsys, Field *field, ZoneSpawnInfo *spawn, u32 a3, u32 a4);
GameEvent *EventEntralinkWarp_Create(GameSystem *gsys, Field *field, ZoneSpawnInfo *spawn);
GameEvent *EventEntralinkWarp_CreateOut(GameSystem *gsys);
GameEventReturnCode func_ov033_02177370(GameEvent *event, u32 *state, void *data);
void EventEntralinkWarp_CreateReturnLocation(ZoneSpawnInfo *spawn, Field *field);
GameEvent *EventMapChangeWarp_CreateFromEntity(GameSystem *gsys, Field *field, ZoneWarp *warp, u32 a3);
GameEvent *EventMapChange_CreateGrid(GameSystem *gsys, Field *field, u8 mode, u16 zoneId, VecFx32 *pos, u16 dir);
GameEvent *EventMapChange_CreateGridDefault(GameSystem *gsys, Field *field, u16 zoneId, VecFx32 *pos, u16 dir);
GameEvent *EventMapChange_CreateForFly(GameSystem *gsys, Field *field, u32 unused, ZoneSpawnInfo *spawn, u16 warpDir);
GameEvent *EventMapChangeFakeWarp_Create(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir);
GameEvent *EventMapChangeEnding_Create(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir);
GameEvent *EventMapChangeBlackout_Create(GameSystem *gsys);
void FieldMapControl_LoadBlackoutZone(GameSystem *gsys);
void FieldMapControl_LoadZone(GameSystem *gsys, u16 zoneId);
void FieldMapControl_InitSpawn(GameSystem *gsys, ZoneSpawnInfo *next);
void FieldMapControl_DeleteAllActors(GameSystem *gsys);
u16 ConvWarpDirToAngle(s32 warpDir);
void SetupZoneChangeSpawn(EventData *eventData, ZoneSpawnInfo *next, ZoneSpawnInfo *spawn);
void CallSpawnAllZoneNPCs(GameData *gameData, const ZoneSpawnInfo *spawn);
void Field_SetPlayerHidden(Field *field, BOOL hidden);
void GameData_DeleteAllActors(GameData *gameData);
void GameData_UpdateJoinAvenueForZone(GameData *gameData, u16 zoneId);
void GameData_UpdateZoneChangeFlag(GameData *gameData, u16 zoneId, u16 prevZoneId);
void GameData_UpdateFlashStatus(GameData *gameData, u16 zoneId);
void GameData_SetGimmickByZone(GameData *gameData, s32 zoneId);
void GameData_UpdateEscapeRopeZone(GameData *gameData, const ZoneSpawnInfo *spawn);
void AdjustEscapeRopeSpawn(GameData *gameData, ZoneSpawnInfo *spawn);
void GameData_AdjustPlayerStateOnDiveOut(GameData *gameData);
void GameData_UpdatePartyForTimeOfDay(GameData *gameData);
void CityState_InitFromSave(CityState *state, PlayerInfo *player, SaveControl *save, u32 unused);
BOOL CityState_IsCityValid(CityState *state);
BOOL CityState_IsSet(CityState *state);

#endif // POKEBW2_FIELD_EVENT_MAPCHANGE_H
