#include "types.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_player.h"
#include "field/zone.h"
#include "gfl/net.h"
#include "struct_decls.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct EntralinkWarpReturnWork {
    GameSystem *gsys;
    Field *field;
};

static const VecFx32 sWarpInPos = { FX32_CONST(511), FX32_CONST(32), FX32_CONST(584) };

GameEventReturnCode func_ov033_02177370(GameEvent *event, u32 *state, void *data) {
    EntralinkWarpReturnWork *work;
    GameSystem *gsys;
    GameCommSys *comm;
    GameData *gameData;
    Field *field;
    GameEvent *next;

    work = data;
    gsys = work->gsys;
    comm = GSYS_GetGameCommSystem(gsys);
    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    switch (*state) {
    case 0:
        if (GameCommSys_IsTransitioning(comm)) {
            break;
        }
        if (GameCommSys_BootCheck(comm)) {
            GameCommSys_ExitReq(comm);
            break;
        }
        next = EventEntralinkWarpIn_Create(gsys, 0x117, &sWarpInPos, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    default:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov033_021773e4(GameSystem *gsys, void *args) {
    GameEvent *event;
    EntralinkWarpReturnWork *work;

    event = GameEvent_Create(gsys, NULL, func_ov033_02177370, sizeof(EntralinkWarpReturnWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = args;
    return event;
}

BOOL EventEntralinkWarpIn_CheckAllowed(GameSystem *gsys) {
    GameData *gameData;
    EventData *eventData;
    ZoneNPC *npcs;
    Field *field;
    MMSys *actorSystem;
    ZoneNPCGridPosition *npcPosition;
    s32 npcCount;
    ZoneNPCRailPosition *npcRail;
    FieldPlayer *player;
    u32 zoneId;
    EventWork *eventWork;
    FieldActor *playerActor;
    VecFx32 worldPosition;
    s16 playerX;
    s16 playerY;
    s16 playerZ;
    RailPosition railPosition;
    FieldActorConfig modelInfo;
    s32 i;
    ZoneNPC *npc;

    gameData = GSYS_GetGameData(gsys);
    eventData = GameData_GetEventData(gameData);
    npcs = GetZoneNPCs(eventData);
    npcCount = GetZoneNPCsCount(eventData);
    field = GSYS_GetField(gsys);
    actorSystem = Field_GetActorSystem(field);
    player = Field_GetPlayer(field);
    zoneId = Field_GetPlayerStateZoneID(field);
    if (!GetZoneFlagsEnableEntralinkWarp(zoneId)) {
        return FALSE;
    }
    if ((u32)(FieldPlayer_GetExState(player) - 2) <= 1) {
        return FALSE;
    }
    if (Field_GetResolvedControllerTypeID(field) == 0) {
        playerActor = FieldPlayer_GetActor(player);
        eventWork = GameData_GetEventWork(gameData);
        CopyActorWPos(playerActor, &worldPosition);
        if (FindCollidingZoneTriggerAtLocation(eventData, eventWork, &worldPosition)) {
            return FALSE;
        }
    }
    if ((npcs == NULL || npcCount == 0) && zoneId != 0 && zoneId != 0x1a8) {
        return TRUE;
    }
    for (i = 0; i < npcCount; i++) {
        npc = &npcs[i];
        if (npc->isRail == FALSE) {
            npcPosition = &npc->pos.grid;
            GetNPCMdlInfoForOBJCODE(actorSystem, npc->modelId, &modelInfo);
            FieldPlayer_GetGPos(player, &playerX, &playerY, &playerZ);
            if (npcPosition->x <= playerX && playerX < npcPosition->x + modelInfo.collWidth &&
                npcPosition->z - modelInfo.collHeight < playerZ && playerZ <= npcPosition->z) {
                return FALSE;
            }
        } else if (npc->isRail == TRUE) {
            npcRail = &npc->pos.rail;
            if (Field_GetResolvedControllerTypeID(field) == 1) {
                FieldPlayer_GetNogridNextRailLocation(player, &railPosition);
                if (npcRail->railIndex == railPosition.componentId && npcRail->frontPos == railPosition.posFront &&
                    npcRail->sidePos == railPosition.posSide) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}
