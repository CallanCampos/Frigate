#ifndef POKEBW2_SYSTEM_GAME_SYSTEM_H
#define POKEBW2_SYSTEM_GAME_SYSTEM_H

#include "types.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "struct_decls.h"

typedef enum {
    GAME_ENTRYPOINT_OPENING,
    GAME_ENTRYPOINT_FIELD_CONTINUE,
    GAME_ENTRYPOINT_DEBUG,
} GameEntryPoint;

struct GameSystemProcData {
    GameEntryPoint entryPoint;
    VecFx32 spawnPos;
    u16 zoneId;
    u16 unk12;
};

extern const GameProcFunctions GAMESYSTEM_PROC_FUNCTIONS;

GameSystemProcData *GameSystem_CreateProcData(GameEntryPoint entryPoint, u16 zoneId, const VecFx32 *spawnPos, s16 unk12);
Field *GSYS_GetField(GameSystem *gsys);
// A VBlank task that draws from the random generator, which the anti-piracy checks add when they fail, and how many
// they added
void get_mt(TCB *tcb, void *data);
extern u32 data_021410f8;
// Whether the field map is up, which Game Freak's asserts call GAMESYSTEM_CheckFieldMapWork
BOOL GSYS_CheckField(GameSystem *gsys);
PlayerState *GSYS_GetPlayerState(GameSystem *gsys);
GameCommSys *GSYS_GetGameCommSystem(GameSystem *gsys);
GameData *GSYS_GetGameData(GameSystem *gsys);
LinkFestival *GSYS_GetLinkFestival(GameSystem *gsys);
BOOL GSYS_GetProcMgrState(GameSystem *gsys);
BOOL GSYS_GetEventRunningFlag(GameSystem *gsys);
// Whether an event is running
BOOL GSYS_CheckNowEvent(GameSystem *gsys);
void GSYS_QueueProc(GameSystem *gsys, s32 overlayId, const GameProcFunctions *functions, void *param);
void GSYS_QueueProcAsEvent(GameEvent *event, s32 overlayId, const GameProcFunctions *functions, void *param);
BOOL GSYS_TryBootGameComm(GameSystem *gsys);
void func_02016b0c(GameSystem *gsys, u32 a1);
u8 func_02016b14(GameSystem *gsys);
void func_02016b24(GameSystem *gsys, u32 value);
u8 func_02016b2c(GameSystem *gsys);
u32 func_02016b34(GameSystem *gsys);
void func_02016b40(GameSystem *gsys, u32 value);
BOOL func_02016bec(GameSystem *gsys);
ISS *GameSystem_GetISS(GameSystem *gsys);
u32 getStatusOfFesMission(LinkFestival *festival);

void GSYS_SetEventProvider(GameSystem *gsys, void *provider, void *data);
GameEvent *GSYS_GetNowEvent(GameSystem *gsys);
void GSYS_SetField(GameSystem *gsys, Field *field);

#endif // POKEBW2_SYSTEM_GAME_SYSTEM_H
