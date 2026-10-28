// The NPC who follows the player, such as an ally on a route, and its script commands. The name is the ROM's own, from
// GFL_HeapAllocate's file argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/pair_sys.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/vm.h"

// The flag set while the follower follows, and the work that holds its index
#define FLAG_FOLLOWER 0x966
#define WORK_FOLLOWER 0x4043
// The follower's actor ID
#define FOLLOWER_ACTOR_ID 0xfe

static void ClearFollowNpcData(FieldFollowWk *wk);

FieldFollowWk *InitFieldFollowWk(HeapID heapId) {
    FieldFollowWk *wk = GFL_HeapAllocate(heapId, sizeof(FieldFollowWk), TRUE, "pair_sys.c", 39);

    wk->heapId = heapId;
    ClearFollowNpcData(wk);
    return wk;
}

void FreeFieldFollowWk(FieldFollowWk *wk) {
    MI_CpuFill8(wk, 0, sizeof(FieldFollowWk));
    GFL_HeapFree(wk);
}

BOOL GameData_CheckPairFlag(GameData *gameData) {
    return EventWork_FlagGet(GameData_GetEventWork(gameData), FLAG_FOLLOWER);
}

static BOOL setupNpcFollower(GameData *gameData, u8 index, u8 isPersistent, u32 originActorId, u16 objCode, u16 scrId,
                             u16 trainerId) {
    FieldFollowWk *wk = GetFieldFollowerCfg(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);

    if (GameData_CheckPairFlag(gameData)) {
        return FALSE;
    }
    if (index >= 8) {
        return FALSE;
    }
    ClearFollowNpcData(wk);
    wk->index = index;
    wk->isPersistent = isPersistent;
    wk->originActorId = originActorId;
    wk->objCode = objCode;
    wk->scrId = scrId;
    wk->trainerId = trainerId;
    EventWork_FlagSet(eventWork, FLAG_FOLLOWER);
    *EventWork_GetWkPtr(eventWork, WORK_FOLLOWER) = index;
    return TRUE;
}

void ShutdownFollowWork(GameData *gameData) {
    FieldFollowWk *wk = GetFieldFollowerCfg(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);

    if (GameData_CheckPairFlag(gameData)) {
        EventWork_FlagReset(eventWork, FLAG_FOLLOWER);
        *EventWork_GetWkPtr(eventWork, WORK_FOLLOWER) = 0xff;
        ClearFollowNpcData(wk);
    }
}

void TryRespawnFollowActor(GameData *gameData, u32 behind) {
    FieldFollowWk *wk = GetFieldFollowerCfg(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    MMSys *mmSys = GameData_GetMMSys(gameData);
    FieldActor *player;
    FieldActor *actor;
    s16 x, z;
    u32 dir;
    u32 zoneId;
    BOOL hidden;

    if (!EventWork_FlagGet(eventWork, FLAG_FOLLOWER)) {
        return;
    }
    if (wk->isPersistent != TRUE) {
        ShutdownFollowWork(gameData);
        return;
    }
    player = FindPlayerFieldActor(mmSys);
    x = GetGPosX(player);
    z = GetGPosZ(player);
    dir = GetActorFaceDir(player);
    zoneId = GetActorZoneID(player);
    hidden = TRUE;
    if (behind == 1) {
        hidden = FALSE;
        AdjusGridXZByDir(GetInverseDirection((u16)dir), &x, &z, 1);
    }
    actor = CreateNewActorByParam(mmSys, x, z, dir, FOLLOWER_ACTOR_ID, wk->objCode, MOVE_CODE_FOLLOW_PLAYER, zoneId);
    SetActorSCRID(actor, wk->scrId);
    SetActorFlag32(actor, TRUE);
    SetActorHidden(actor, hidden);
    SetActorFlag256(actor, hidden);
}

void updateFollowerModel(GameData *gameData) {
    EventWork *eventWork;
    MMSys *mmSys;
    FieldActor *actor;

    GetFieldFollowerCfg(gameData);
    eventWork = GameData_GetEventWork(gameData);
    mmSys = GameData_GetMMSys(gameData);
    if (!EventWork_FlagGet(eventWork, FLAG_FOLLOWER)) {
        return;
    }
    actor = FindFieldActor(mmSys, FOLLOWER_ACTOR_ID);
    if (actor == NULL) {
        ShutdownFollowWork(gameData);
        return;
    }
    if (IsActorHidden(actor)) {
        FieldActor *player = FindPlayerFieldActor(mmSys);
        GridPos pos;

        GetActorInitGPos(player, &pos);
        SetActorGPos(actor, pos.x, pos.y, pos.z, GetActorFaceDir(player));
        SetActorHidden(actor, FALSE);
        SetActorFlag256(actor, FALSE);
    }
}

static u16 getFollowFlagStatus(GameData *gameData) {
    EventWork *eventWork = GameData_GetEventWork(gameData);

    if (EventWork_FlagGet(eventWork, FLAG_FOLLOWER)) {
        return *EventWork_GetWkPtr(eventWork, WORK_FOLLOWER);
    }
    return 0xff;
}

u16 GetNowFollowerAllyTrID(GameData *gameData) {
    u16 status = getFollowFlagStatus(gameData);
    FieldFollowWk *wk = GetFieldFollowerCfg(gameData);

    if (status == 0xff) {
        return 0;
    }
    return wk->trainerId;
}

static u8 GetFollowActorOriginActorID(GameData *gameData) {
    return GetFieldFollowerCfg(gameData)->originActorId;
}

static BOOL GetFollowWkIsPersistent(GameData *gameData) {
    return GetFieldFollowerCfg(gameData)->isPersistent;
}

static void ClearFollowNpcData(FieldFollowWk *wk) {
    wk->index = 0xff;
    wk->isPersistent = FALSE;
    wk->originActorId = 0;
    wk->objCode = 0;
    wk->scrId = 0;
    wk->trainerId = 0;
}

BOOL Scrcmd_PairSys_ActorPairSet(VM *vm, FieldScriptEnv *env) {
    u16 actorId = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u16 isPersistent = ScriptReadAny(vm, env);
    u16 trainerId = ScriptReadAny(vm, env);
    u16 scrId = ScriptReadAny(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    FieldActor *actor = FindFieldActor(GameData_GetMMSys(gameData), actorId);

    if (actor == NULL) {
        return TRUE;
    }
    if (scrId == 0) {
        scrId = FldAct_GetSCRID(actor);
    }
    setupNpcFollower(gameData, index, isPersistent, actorId, FldAct_GetObjCode(actor), scrId, trainerId);
    ChangeActorMoveCodeSeq(actor, MOVE_CODE_FOLLOW_PLAYER);
    ChangeActorUID(actor, FOLLOWER_ACTOR_ID);
    SetActorSCRID(actor, scrId);
    if (isPersistent == TRUE) {
        SetActorFlag32(actor, TRUE);
    }
    return TRUE;
}

BOOL Scrcmd_PairSys_ActorPairEnd(VM *vm, FieldScriptEnv *env) {
    u16 scrId = ScriptReadAny(vm, env);
    u16 moveCode = ScriptReadAny(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    FieldActor *actor = FindFieldActor(GameData_GetMMSys(gameData), FOLLOWER_ACTOR_ID);

    if (actor != NULL) {
        SetActorSCRID(actor, scrId);
        ChangeActorMoveCodeSeq(actor, moveCode);
        if (!GetFollowWkIsPersistent(gameData)) {
            ChangeActorUID(actor, GetFollowActorOriginActorID(gameData));
        }
        SetActorFlag32(actor, FALSE);
        SetActorFlag8000(actor, TRUE);
    }
    ShutdownFollowWork(gameData);
    return TRUE;
}

BOOL Scrcmd_PairSys_ActorPairGetTrID(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    GameData_CheckPairFlag(gameData);
    *result = GetNowFollowerAllyTrID(gameData);
    return TRUE;
}

BOOL Scrcmd_PairSys_ActorPairSetMoveEnable(VM *vm, FieldScriptEnv *env) {
    u8 enable = VM_Read8(vm);
    FieldActor *actor = FindFieldActor(GameData_GetMMSys(FieldScriptEnv_GetGameData(env)), FOLLOWER_ACTOR_ID);

    if (actor == NULL) {
        return TRUE;
    }
    if (enable) {
        EnableActorMovement(actor);
    } else {
        DisableActorMovement(actor);
    }
    return TRUE;
}
