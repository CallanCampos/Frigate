#include "types.h"
#include "field/comm_player.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_comm_actor.h"
#include "field/field_effect.h"
#include "field/field_player.h"
#include "field/field_status.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_system.h"

// Game Freak's name for GSYS_CheckField, which the asserts keep
#define GAMESYSTEM_CheckFieldMapWork GSYS_CheckField

static void func_ov012_02161514(CommPlayerSys *cps, int index);
static void func_ov012_02161808(GameSystem *gsys, CommPlayerSys *cps);

CommPlayerSys *func_ov012_021613d0(u8 a0, GameSystem *gsys, HeapID heapId, u8 a3) {
    CommPlayerSys *cps = GFL_HeapAllocate(heapId, sizeof(CommPlayerSys), TRUE, "comm_player.c", 97);

    cps->unk8E = a0;
    cps->gsys = gsys;
    cps->heapId = heapId;
    cps->unk90 = a3;
    return cps;
}

void func_ov012_02161404(GameSystem *gsys, CommPlayerSys *cps) {
    int i;

    if (cps->act_ctrl != NULL) {
        for (i = 0; i < COMM_PLAYER_MAX; i++) {
            func_ov012_02161514(cps, i);
        }
        if (GSYS_CheckField(gsys) == TRUE) {
            FldCommActSys_Free(cps->act_ctrl);
        } else {
            GFL_ASSERT(0);
        }
    }
    GFL_HeapFree(cps);
}

void func_ov012_0216144c(GameSystem *gsys, CommPlayerSys *cps) {
    int i;

    func_ov012_02161808(gsys, cps);
    if (GSYS_CheckField(cps->gsys) == TRUE) {
        for (i = 0; i < COMM_PLAYER_MAX; i++) {
            if (cps->players[i].active == TRUE && cps->players[i].actor != NULL &&
                func_ov036_021b3fb4(cps->players[i].actor) == TRUE) {
                cps->players[i].actor = NULL;
            }
        }
    }
}

void func_ov012_0216148c(CommPlayerSys *cps, int index, u16 a2, const CommPlayerStatus *status) {
    if (cps->act_ctrl == NULL || cps->players[index].active == TRUE) {
        return;
    }
    cps->players[index].pos = status->pos;
    cps->players[index].dir = status->dir;
    cps->players[index].unkC = status->unk0;
    cps->players[index].unk12 = a2;
    cps->players[index].hidden = FALSE;
    cps->players[index].active = TRUE;
    cps->players[index].actor = NULL;
    FldCommActSys_CreateActor(cps->act_ctrl, index, a2, &cps->players[index].dir, &cps->players[index].pos,
                              &cps->players[index].unkC);
}

static void func_ov012_02161514(CommPlayerSys *cps, int index) {
    if (cps->players[index].active && cps->act_ctrl != NULL) {
        cps->players[index].actor = NULL;
        FldCommActSys_DeleteActor(cps->act_ctrl, index);
        cps->players[index].active = FALSE;
    }
}

void func_ov012_02161544(CommPlayerSys *cps) {
    int i;

    GFL_ASSERT(GAMESYSTEM_CheckFieldMapWork(cps->gsys) == TRUE);
    if (cps->act_ctrl != NULL) {
        for (i = 0; i < COMM_PLAYER_MAX; i++) {
            if (cps->players[i].active == TRUE) {
                func_ov012_02161514(cps, i);
                cps->players[i].hidden = TRUE;
            }
        }
        FldCommActSys_Free(cps->act_ctrl);
        cps->act_ctrl = NULL;
    }
}

void func_ov012_021615a4(CommPlayerSys *cps) {
    CommPlayerStatus status;
    int i;

    GFL_ASSERT(GAMESYSTEM_CheckFieldMapWork(cps->gsys) == TRUE);
    GFL_ASSERT(cps->act_ctrl == NULL);
    cps->act_ctrl =
        FldCommActSys_Create(cps->unk8E, Field_GetActorSystem(GSYS_GetField(cps->gsys)), cps->heapId, cps->unk90);
    MI_CpuFill8(&status, 0, sizeof(CommPlayerStatus));
    for (i = 0; i < COMM_PLAYER_MAX; i++) {
        if (cps->players[i].hidden == TRUE) {
            status.pos = cps->players[i].pos;
            status.dir = cps->players[i].dir;
            status.unk0 = 0;
            status.exState = cps->players[i].exState;
            func_ov012_0216148c(cps, i, cps->players[i].unk12, &status);
        }
    }
}

BOOL func_ov012_0216165c(CommPlayerSys *cps, int index) {
    GFL_ASSERT(cps != NULL && index < COMM_PLAYER_MAX);
    return cps->players[index].active;
}

void func_ov012_0216168c(CommPlayerSys *cps, int index, const CommPlayerStatus *status) {
    cps->players[index].pos = status->pos;
    cps->players[index].dir = status->dir;
    cps->players[index].unkC = status->unk0;
}

BOOL func_ov012_021616b0(CommPlayerSys *cps, CommPlayerStatus *status) {
    GameData *gameData = GSYS_GetGameData(cps->gsys);
    CommPlayerSent *last = &cps->last;
    BOOL busy = FALSE;
    PlayerState *state;
    u32 exState;
    u32 dir;
    VecFx32 pos;
    FieldPlayer *player;
    FieldActor *actor;

    if (cps->unk8F == TRUE) {
        return FALSE;
    }
    state = GameData_GetPlayerState(gameData);
    exState = FieldPlayerState_GetExState(state);
    if (FieldStatus_GetBusyFlag(GameData_GetFieldStatus(gameData)) == 2) {
        dir = last->dir;
        pos = last->pos;
        busy = TRUE;
        exState = last->exState;
    } else if (GSYS_CheckField(cps->gsys) == TRUE && Field_CheckMapLoadFinished(GSYS_GetField(cps->gsys)) == TRUE) {
        player = Field_GetPlayer(GSYS_GetField(cps->gsys));
        actor = FieldPlayer_GetActor(player);
        if (FieldPlayer_DeriveExState(player) == 4) {
            FieldPlayer_GetWPos(player, &pos);
        } else {
            CopyActorPosAllAdd(actor, &pos);
        }
        dir = FieldPlayer_GetFaceDir(player);
    } else {
        pos = *PlayerState_GetWPos(state);
        dir = PlayerState_CalcDirection(state);
    }
    status->dir = dir;
    status->pos = pos;
    status->unk0 = busy;
    status->exState = exState;
    if (dir != last->dir || exState != last->exState || busy != last->busy ||
        GFL_STD_MemCmp(&pos, &last->pos, sizeof(VecFx32)) != 0) {
        last->dir = dir;
        last->pos = pos;
        last->exState = (u8)exState;
        last->busy = (u8)busy;
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_021617e8(CommPlayerSys *cps, s16 x, s16 y, s16 z, u32 *netId, u32 *index) {
    if (cps->act_ctrl == NULL) {
        return FALSE;
    }
    return FldCommActSys_FindActor(cps->act_ctrl, x, y, z, netId, index);
}

static void func_ov012_02161808(GameSystem *gsys, CommPlayerSys *cps) {
    Field *field;

    if (cps->act_ctrl == NULL) {
        field = GSYS_GetField(gsys);
        if (field != NULL && Field_CheckMapLoadFinished(field)) {
            cps->act_ctrl = FldCommActSys_Create(cps->unk8E, Field_GetActorSystem(field), cps->heapId, cps->unk90);
        }
    }
}
