#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLDATA_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLDATA_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_calldata.c: the table of the calls and the save data of the calls made

LocalTvtCallData *LocalTvtCallData_Create(GameSystem *gsys, GameData *gameData, HeapID heapId);
void LocalTvtCallData_Free(LocalTvtCallData *data);
// The background and the object of the face of a person the player calls, for the calls made so far
u32 LocalTvtCallData_GetBackground(LocalTvtCallData *data, u16 person);
u32 LocalTvtCallData_GetObject(LocalTvtCallData *data, u16 person);
u32 LocalTvtCallData_GetBadgeCount(LocalTvtCallData *data);
GameData *LocalTvtCallData_GetGameData(LocalTvtCallData *data);
// The messages of the call with a person, for the calls made so far
u16 LocalTvtCallData_GetGreetingMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetChatMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetByeMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetExtraMsg(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_CanCall(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_IsListed(LocalTvtCallData *data, u16 person);
// The background and the object of a face in a zone, by its battle background and weather
u32 LocalTvtCallData_GetZoneBackground(LocalTvtCallData *data, u16 zoneId);
u32 LocalTvtCallData_GetZoneObject(LocalTvtCallData *data, u16 zoneId);
// The calls made with a person, which count up to a maximum
u16 LocalTvtCallData_GetCallCount(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_IsPlayerMale(LocalTvtCallData *data);
void LocalTvtCallData_AddCall(LocalTvtCallData *data, u16 person, u32 max);
void LocalTvtCallData_SetMeeting(LocalTvtCallData *data, u32 value);
void LocalTvtCallData_SetMeetingToday(LocalTvtCallData *data);
BOOL LocalTvtCallData_IsMeetingToday(LocalTvtCallData *data);
void LocalTvtCallData_ClearMeetingIfMet(LocalTvtCallData *data);
BOOL LocalTvtCallData_IsSecretUnknown(LocalTvtCallData *data);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLDATA_H
