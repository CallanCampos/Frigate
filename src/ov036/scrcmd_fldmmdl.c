// The script commands of field actors: action commands and walked routes, adding, removing and placing actors, their
// positions, directions and rail positions, and pausing the actors that a script doesn't move. The name is the ROM's
// own, from GFL_HeapAllocate's file argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
#include "field/event_action_call.h"
#include "field/event_actor_move.h"
#include "field/event_dive.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_rail.h"
#include "field/field_script.h"
#include "field/field_task.h"
#include "field/player_state.h"
#include "field/scrcmd_fldmmdl.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

u8 g_ScrEventActorFlags;

// The world position of a tile's corner, and of its center
#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))
#define GRID_CENTER(n) ((n) * FX32_CONST(16) + FX32_CONST(8))
// The tile of a world position
#define FX32_TO_GRID(x) (((x) >> FX32_SHIFT) / 16)
#define HEIGHT_TO_GRID(height) ((s16)(((height) >> 4) / FX32_ONE))

// The actor IDs of the follower and of the script's own actor
#define SCRIPT_ACTOR_FOLLOWER 0xf2
#define SCRIPT_ACTOR_SELF 0xf1
// The move code of the Pokémon that follows the player
#define MOVE_CODE_FOLLOWER 0x30

// The actors that DisableAllButEventActorsMovement left moving, which WaitAndDisableEventMovement stops once they
// finish
#define EVENT_ACTOR_PLAYER (1 << 0)
#define EVENT_ACTOR_FOLLOWER (1 << 1)
#define EVENT_ACTOR_PARENT (1 << 2)
#define EVENT_ACTOR_PAIR (1 << 3)

// An action command as the walked route writes it
typedef struct {
    u16 code;
    u16 count;
} WalkRouteAcmd;

// The route that s024F_ActorWalkRoute has an actor walk: to the tile (x, z), first along x if axis is 0
typedef struct {
    FieldScriptEnv *env;
    Field *field;
    u16 x;
    u16 z;
    // Bit 0 walks through actors, bit 1 through blocked tiles, bit 2 checks only the map's bounds
    u16 flags;
    // Frames per tile
    u16 speed;
    u16 axis;
} WalkRoute;

static FieldActor *FindScrEnvPlayerActor(FieldScriptEnv *env);

static const u32 ACMD_QUEUE_TURN_N_8F[2] = { ACMD(0x20, 1), ACMD_END };
static const u32 ACMD_QUEUE_TURN_S_8F[2] = { ACMD(0x21, 1), ACMD_END };
static const u32 ACMD_QUEUE_TURN_W_8F[2] = { ACMD(0x22, 1), ACMD_END };
static const u32 ACMD_QUEUE_TURN_E_8F[2] = { ACMD(0x23, 1), ACMD_END };

static FieldActor *FindScriptActor(FieldScriptEnv *env, u16 id) {
    FieldActor *actor;
    MMSys *mmSys = GetScrEnvMMdlSys(env);

    if (id == SCRIPT_ACTOR_FOLLOWER) {
        actor = FindActorByMoveCode(mmSys, MOVE_CODE_FOLLOWER);
    } else if (id == SCRIPT_ACTOR_SELF) {
        ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
        // BUG: The script's own actor is never looked up, so the actor returned is garbage
#ifdef BUGFIX
        actor = ScriptWork_GetParentActor(work);
#endif
    } else {
        actor = FindFieldActor(mmSys, id);
    }
    return actor;
}

BOOL s0064_ActorCmdExec(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    u16 id = ScriptReadAny(vm, env);
    u32 offset = VM_Read32(vm);
    FieldActor *actor = FindScriptActor(env, id);

    if (actor == NULL) {
        return FALSE;
    }
    FieldScriptEnv_AddAcmdTask(env, FieldAcmdTCB_Create(actor, (const u32 *)(vm->pc + offset)));
    return FALSE;
}

// Adds the action command that walks count tiles in dir at the route's speed
static void WriteWalkRouteAcmdCore(WalkRouteAcmd *acmd, u32 dir, u16 *index, u16 count, WalkRoute *route) {
    u32 code;

    // The command that walks up at the speed: the next three walk down, left and right
    switch (route->speed) {
    case 32:
        code = 0x4;
        break;
    case 16:
        code = 0x8;
        break;
    case 12:
        code = 0xa7;
        break;
    case 8:
        code = 0xc;
        break;
    case 7:
        code = 0x60;
        break;
    case 6:
        code = 0x4c;
        break;
    case 4:
        code = 0x10;
        break;
    case 3:
        code = 0x50;
        break;
    case 2:
        code = 0x14;
        break;
    default:
        code = 0x8;
        break;
    }
    switch (dir) {
    case 0:
        acmd[*index].code = code;
        acmd[*index].count = count;
        break;
    case 1:
        acmd[*index].code = code + 1;
        acmd[*index].count = count;
        break;
    case 2:
        acmd[*index].code = code + 2;
        acmd[*index].count = count;
        break;
    case 3:
        acmd[*index].code = code + 3;
        acmd[*index].count = count;
        break;
    }
    (*index)++;
}

static void AdjustVecByDir(u32 dir, VecFx32 *pos, u16 tiles) {
    switch (dir) {
    case 0:
        pos->z -= GRID_TO_FX32(tiles);
        break;
    case 1:
        pos->z += GRID_TO_FX32(tiles);
        break;
    case 2:
        pos->x -= GRID_TO_FX32(tiles);
        break;
    case 3:
        pos->x += GRID_TO_FX32(tiles);
        break;
    }
}

static BOOL WalkRouteDestPosBlockCheck(WalkRoute *route, VecFx32 pos) {
    FieldG3DMapper *mapper;
    u32 tileFlags;

    if (route->flags & 4) {
        mapper = Field_GetG3DMapper(route->field);
        tileFlags = GetTileFlags(GetTileTypeAtPos(mapper, &pos));
        if (FieldG3DMapper_IsPosOutOfBounds(mapper, &pos) == TRUE) {
            return TRUE;
        }
        return FALSE;
    }
    mapper = Field_GetG3DMapper(route->field);
    if (!(route->flags & 2)) {
        tileFlags = GetTileFlags(GetTileTypeAtPos(mapper, &pos));
        if ((tileFlags & 1) || (tileFlags & 2)) {
            return TRUE;
        }
    }
    if (FieldG3DMapper_IsPosOutOfBounds(mapper, &pos) == TRUE) {
        return TRUE;
    }
    if (!(route->flags & 1)) {
        if (GetFirstActorOnGPos(Field_GetActorSystem(route->field), FX32_TO_GRID(pos.x), FX32_TO_GRID(pos.z), TRUE) !=
            NULL) {
            return TRUE;
        }
    }
    return FALSE;
}

// Steps around the blocked tile in dir: one tile and more to the side, then one tile in dir
static BOOL WalkRoute_Detour(u32 dir, VecFx32 *pos, WalkRouteAcmd *acmd, u16 *index, WalkRoute *route) {
    VecFx32 start;
    VecFx32 sideZ;
    VecFx32 sideX;
    u16 i;

    start.x = pos->x;
    start.y = pos->y;
    start.z = pos->z;

    switch (dir) {
    case 2:
    case 3:
        for (i = 1;; i++) {
            sideZ = start;
            AdjustVecByDir(0, &sideZ, i);
            AdjustVecByDir(dir, &sideZ, 1);
            if (!WalkRouteDestPosBlockCheck(route, sideZ)) {
                WriteWalkRouteAcmdCore(acmd, 0, index, i, route);
                WriteWalkRouteAcmdCore(acmd, dir, index, 1, route);
                VEC_Set(pos, sideZ.x, sideZ.y, sideZ.z);
                break;
            }
            sideZ = start;
            AdjustVecByDir(1, &sideZ, i);
            AdjustVecByDir(dir, &sideZ, 1);
            if (!WalkRouteDestPosBlockCheck(route, sideZ)) {
                WriteWalkRouteAcmdCore(acmd, 1, index, i, route);
                WriteWalkRouteAcmdCore(acmd, dir, index, 1, route);
                VEC_Set(pos, sideZ.x, sideZ.y, sideZ.z);
                break;
            }
            if (i > 10) {
                return FALSE;
            }
        }
        break;
    case 0:
    case 1:
        for (i = 1;; i++) {
            sideX = start;
            AdjustVecByDir(2, &sideX, i);
            AdjustVecByDir(dir, &sideX, 1);
            if (!WalkRouteDestPosBlockCheck(route, sideX)) {
                WriteWalkRouteAcmdCore(acmd, 2, index, i, route);
                WriteWalkRouteAcmdCore(acmd, dir, index, 1, route);
                VEC_Set(pos, sideX.x, sideX.y, sideX.z);
                break;
            }
            sideX = start;
            AdjustVecByDir(3, &sideX, i);
            AdjustVecByDir(dir, &sideX, 1);
            if (!WalkRouteDestPosBlockCheck(route, sideX)) {
                WriteWalkRouteAcmdCore(acmd, 3, index, i, route);
                WriteWalkRouteAcmdCore(acmd, dir, index, 1, route);
                VEC_Set(pos, sideX.x, sideX.y, sideX.z);
                break;
            }
            if (i > 10) {
                return FALSE;
            }
        }
        break;
    }
    return TRUE;
}

// Walks one tile toward the destination, or around the tile if it is blocked
static void WalkRoute_WriteStep(u32 step, VecFx32 *pos, WalkRouteAcmd *acmd, u16 *index, WalkRoute *route) {
    VecFx32 next;
    u32 dir;

    VEC_Set(&next, pos->x, pos->y, pos->z);
    switch (step) {
    case 1:
        dir = 2;
        break;
    case 0:
        dir = 3;
        break;
    case 2:
        dir = 0;
        break;
    case 3:
        dir = 1;
        break;
    }
    AdjustVecByDir(dir, &next, 1);
    if (WalkRouteDestPosBlockCheck(route, next)) {
        WalkRoute_Detour(dir, pos, acmd, index, route);
        return;
    }
    WriteWalkRouteAcmdCore(acmd, dir, index, 1, route);
    AdjustVecByDir(dir, pos, 1);
}

// Walks back to the line the route started on, after a step around a blocked tile
static BOOL WalkRoute_ReturnToLine(u32 step, fx32 line, fx32 now, VecFx32 *pos, WalkRouteAcmd *acmd, u16 *index,
                                   WalkRoute *route) {
    VecFx32 next;
    s16 diff = FX32_TO_GRID(now) - FX32_TO_GRID(line);
    u32 dir;
    u16 count;

    VEC_Set(&next, pos->x, pos->y, pos->z);
    switch (step) {
    case 0:
    case 1:
        if (diff > 0) {
            dir = 0;
        } else {
            dir = 1;
        }
        break;
    case 2:
    case 3:
        dir = 2;
        if (diff <= 0) {
            dir = 3;
        }
        break;
    }
    count = diff < 0 ? -diff : diff;
    AdjustVecByDir(dir, &next, count);
    if (WalkRouteDestPosBlockCheck(route, next)) {
        return FALSE;
    }
    WriteWalkRouteAcmdCore(acmd, dir, index, count, route);
    AdjustVecByDir(dir, pos, count);
    return TRUE;
}

// Walks along one axis until pos reaches dest on it
static void WalkRoute_WriteAxis(WalkRouteAcmd *acmd, u16 *index, VecFx32 *pos, VecFx32 *dest, WalkRoute *route) {
    fx32 *posOther;
    u32 step;
    fx32 *posAxis;
    fx32 *destAxis;
    fx32 line;

    if (route->axis == 0) {
        posOther = &pos->z;
        line = pos->z;
        if (dest->x > pos->x) {
            step = 0;
        } else {
            step = 1;
        }
        posAxis = &pos->x;
        destAxis = &dest->x;
    } else {
        line = pos->x;
        posOther = &pos->x;
        if (dest->z > pos->z) {
            step = 3;
        } else {
            step = 2;
        }
        posAxis = &pos->z;
        destAxis = &dest->z;
    }
    while (FX32_TO_GRID(*posAxis) != FX32_TO_GRID(*destAxis)) {
        WalkRoute_WriteStep(step, pos, acmd, index, route);
        if (*posOther != line && FX32_TO_GRID(*posAxis) != FX32_TO_GRID(*destAxis)) {
            if (WalkRoute_ReturnToLine(step, line, *posOther, pos, acmd, index, route) == TRUE) {
                line = *posOther;
            }
        }
    }
}

static u16 CreateRouteActionQueue(WalkRouteAcmd *acmd, FieldActor *actor, WalkRoute *route) {
    VecFx32 pos;
    VecFx32 dest;
    u16 index = 0;

    CopyActorWPos(actor, &pos);
    dest.x = GRID_TO_FX32(route->x);
    dest.z = GRID_TO_FX32(route->z);
    // BUG: The original passes an unset destination Y to the tile and bounds checks
#ifdef BUGFIX
    dest.y = pos.y;
    WalkRouteDestPosBlockCheck(route, dest);
#else
    WalkRouteDestPosBlockCheck(route, dest);
#endif
    WalkRoute_WriteAxis(acmd, &index, &pos, &dest, route);
    route->axis = route->axis == 0 ? 1 : 0;
    WalkRoute_WriteAxis(acmd, &index, &pos, &dest, route);
    // The route ends with 0xfd, not ACMD_END
    acmd[index].code = 0xfd;
    acmd[index].count = 0;
    return index;
}

BOOL s024F_ActorWalkRoute(VM *vm, FieldScriptEnv *env) {
    WalkRoute route;
    u16 id = ScriptReadAny(vm, env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    FieldActor *actor = FindScriptActor(env, id);
    WalkRouteAcmd *acmd;

    if (actor == NULL) {
        return FALSE;
    }
    acmd = GFL_HeapAllocate(heapId, sizeof(WalkRouteAcmd) * 30, TRUE, "scrcmd_fldmmdl.c", 793);
    route.env = env;
    route.field = fieldWork->field;
    route.x = ScriptReadAny(vm, env);
    route.z = ScriptReadAny(vm, env);
    route.flags = ScriptReadAny(vm, env);
    route.speed = ScriptReadAny(vm, env);
    route.axis = ScriptReadAny(vm, env);
    CreateRouteActionQueue(acmd, actor, &route);
    FieldScriptEnv_AddAcmdTask(env, FieldAcmdTCB_Create(actor, (const u32 *)acmd));
    return FALSE;
}

static BOOL ScriptNative_AcmdWait(VM *vm, void *data) {
    if (!FieldScriptEnv_CheckAcmdQueueRunning(data)) {
        return TRUE;
    }
    return FALSE;
}

BOOL s0065_ActorCmdWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_AcmdWait);
    return TRUE;
}

BOOL s0066_ActorGetMoveCode(VM *vm, FieldScriptEnv *env) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    u16 *result = ScriptReadVar(vm, env);

    *result = 0;
    *result = GetActorMoveCode(FindFieldActor(mmSys, ScriptReadAny(vm, env)));
    return FALSE;
}

BOOL s0073_ActorSetMoveCode(VM *vm, FieldScriptEnv *env) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    u16 id = ScriptReadAny(vm, env);
    u16 moveCode = ScriptReadAny(vm, env);

    ChangeActorMoveCodeSeq(FindFieldActor(mmSys, id), moveCode);
    return FALSE;
}

BOOL s0067_ActorGetGPos(VM *vm, FieldScriptEnv *env) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *actor = FindFieldActor(mmSys, ScriptReadAny(vm, env));
    u16 *x = ScriptReadVar(vm, env);
    u16 *z = ScriptReadVar(vm, env);

    *x = GetGPosX(actor);
    *z = GetGPosZ(actor);
    return FALSE;
}

BOOL s0068_PlayerGetGPos(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    FieldActor *actor = FieldPlayer_GetActor(Field_GetPlayer(fieldWork->field));
    u16 *x = ScriptReadVar(vm, env);
    u16 *z = ScriptReadVar(vm, env);

    *x = GetGPosX(actor);
    *z = GetGPosZ(actor);
    return FALSE;
}

BOOL s006E_PlayerGetDir(VM *vm, FieldScriptEnv *env) {
    FieldActor *actor = FindScrEnvPlayerActor(env);
    u16 *result = ScriptReadVar(vm, env);

    *result = GetActorFaceDir(actor);
    return FALSE;
}

BOOL s0069_ActorNew(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 dir = ScriptReadAny(vm, env);
    u16 id = ScriptReadAny(vm, env);
    u16 objCode = ScriptReadAny(vm, env);
    u16 moveCode = ScriptReadAny(vm, env);
    u16 zoneId = Field_GetPlayerStateZoneID(fieldWork->field);
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *actor = CreateNewActorByParam(
        mmSys, x, z, dir, id, ResolvePossibleWKOBJCODE(GameData_GetEventWork(FieldScriptEnv_GetGameData(env)), objCode),
        moveCode, zoneId);

    if (actor != NULL) {
        DisableActorMovement(actor);
    }
    return FALSE;
}

BOOL s006C_ActorDelete(VM *vm, FieldScriptEnv *env) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *actor = FindFieldActor(mmSys, ScriptReadAny(vm, env));

    if (actor != NULL) {
        DeleteActor(actor);
    }
    return FALSE;
}

BOOL s006B_ActorAdd(VM *vm, FieldScriptEnv *env) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    EventData *eventData = GameData_GetEventData(gameData);
    u32 count = GetZoneNPCsCount(eventData);
    u16 zoneId = PlayerState_GetZoneID(GameData_GetPlayerState(gameData));
    u16 uid = ScriptReadAny(vm, env);

    if (count != 0) {
        EventWork *eventWork = GameData_GetEventWork(gameData);
        FieldActor *actor = SpawnZoneNPCByUID(mmSys, GetZoneNPCs(eventData), zoneId, count, eventWork, uid);

        if (actor != NULL) {
            DisableActorMovement(actor);
        }
    }
    return FALSE;
}

BOOL s006D_ActorSetGPos(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    s16 x = ScriptReadAny(vm, env);
    s16 y = ScriptReadAny(vm, env);
    s16 z = ScriptReadAny(vm, env);
    u16 dir = ScriptReadAny(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvMMdlSys(env), id);

    if (actor != NULL) {
        SetActorGPos(actor, x, y, z, dir);
    }
    return FALSE;
}

BOOL s006A_ActorGetSpawnFlag(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvMMdlSys(env), id);

    if (actor != NULL) {
        *result = GetActorSpawnFlag(actor);
    }
    return FALSE;
}

BOOL s0079_ActorGetUserParam(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvMMdlSys(env), id);

    if (index <= 2 && actor != NULL) {
        *result = GetActorUserParam(actor, index);
    }
    return FALSE;
}

BOOL ScriptNative_SetMmdlZoneDeleteOff(VM *vm, FieldScriptEnv *env) {
    u16 id = ScriptReadAny(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvMMdlSys(env), id);

    if (actor != NULL) {
        SetActorFlag32(actor, TRUE);
    }
    return FALSE;
}

// Stops each actor that DisableAllButEventActorsMovement left moving once it has finished its move
static BOOL WaitAndDisableEventMovement(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    FieldActor *parent = ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env));
    FieldActor *actor = FindScrEnvPlayerActor(env);
    MMSys *mmSys = GetScrEnvMMdlSys(env);

    if ((g_ScrEventActorFlags & EVENT_ACTOR_PLAYER) && FldAct_CheckAcmdEnd(actor) == TRUE) {
        DisableActorMovement(actor);
        g_ScrEventActorFlags &= ~EVENT_ACTOR_PLAYER;
    }
    if ((g_ScrEventActorFlags & EVENT_ACTOR_PARENT) && IsActorFlag16(parent) == FALSE) {
        DisableActorMovement(parent);
        g_ScrEventActorFlags &= ~EVENT_ACTOR_PARENT;
    }
    if (g_ScrEventActorFlags & EVENT_ACTOR_FOLLOWER) {
        actor = FindActorByMoveCode(mmSys, MOVE_CODE_FOLLOWER);
        if (IsActorFlag16(actor) == FALSE) {
            DisableActorMovement(actor);
            g_ScrEventActorFlags &= ~EVENT_ACTOR_FOLLOWER;
        }
    }
    if (g_ScrEventActorFlags & EVENT_ACTOR_PAIR) {
        parent = FindPairedTrainerActor(parent);
        if (parent == NULL) {
            g_ScrEventActorFlags &= ~EVENT_ACTOR_PAIR;
        } else if (IsActorFlag16(parent) == FALSE) {
            DisableActorMovement(parent);
            g_ScrEventActorFlags &= ~EVENT_ACTOR_PAIR;
        }
    }
    if (g_ScrEventActorFlags == 0) {
        return TRUE;
    }
    return FALSE;
}

// Stops every actor but the player, the script's actor, the follower and the paired Trainer, and waits for those to
// finish their moves
static BOOL DisableAllButEventActorsMovement(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *parent = ScriptWork_GetParentActor(work);
    FieldActor *player = FindScrEnvPlayerActor(env);
    FieldActor *follower = FindActorByMoveCode(mmSys, MOVE_CODE_FOLLOWER);
    FieldActor *pair = FindPairedTrainerActor(parent);

    g_ScrEventActorFlags = 0;
    DisableAllActorsMovement(mmSys);
    if (FldAct_CheckAcmdEnd(player) == FALSE) {
        g_ScrEventActorFlags |= EVENT_ACTOR_PLAYER;
        EnableActorMovement(player);
    }
    if (parent != NULL && IsActorFlag16(parent) == TRUE) {
        g_ScrEventActorFlags |= EVENT_ACTOR_PARENT;
        EnableActorMovement(parent);
    }
    if (follower != NULL && IsActorFlag16(follower) == TRUE) {
        g_ScrEventActorFlags |= EVENT_ACTOR_FOLLOWER;
        EnableActorMovement(follower);
    }
    if (pair != NULL && IsActorFlag16(pair) == TRUE) {
        g_ScrEventActorFlags |= EVENT_ACTOR_PAIR;
        EnableActorMovement(pair);
    }
    VM_SetNativeCallback(vm, WaitAndDisableEventMovement);
    return TRUE;
}

static BOOL WaitAndDisableFollowerMovement(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    FieldScriptEnv_GetScriptWork(env);
    FieldActor *follower = FindActorByMoveCode(GetScrEnvMMdlSys(env), MOVE_CODE_FOLLOWER);

    if (IsActorFlag16(follower) == FALSE) {
        DisableActorMovement(follower);
        return TRUE;
    }
    return FALSE;
}

BOOL PauseEventMModels(VM *vm, FieldScriptEnv *env) {
    if (ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env)) == NULL) {
        MMSys *mmSys = GetScrEnvMMdlSys(env);
        FieldActor *follower;

        DisableAllActorsMovement(mmSys);
        follower = FindActorByMoveCode(mmSys, MOVE_CODE_FOLLOWER);
        if (follower != NULL && IsActorFlag16(follower) == TRUE) {
            EnableActorMovement(follower);
            VM_SetNativeCallback(vm, WaitAndDisableFollowerMovement);
            return TRUE;
        }
    } else {
        return DisableAllButEventActorsMovement(vm, env);
    }
    return FALSE;
}

void EnableAllActorsMovementScr(FieldScriptEnv *env) {
    EnableAllActorsMovement(GetScrEnvMMdlSys(env));
}

BOOL s0074_ActorSetEyeToEye(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 dir = GetInverseDirection(GetActorFaceDir(FindScrEnvPlayerActor(env)));
    FieldActor *actor = ScriptWork_GetParentActor(work);

    if (actor != NULL) {
        CheckSetActorFaceDir(actor, dir);
        FldAct_SetDrawStatus(actor, 0);
    }
    return FALSE;
}

BOOL s0070_ActorFindByGPos(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    Field *field = fieldWork->field;
    u16 *uid = ScriptReadVar(vm, env);
    u16 *found = ScriptReadVar(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    FieldActor *actor = FindActorByGPos(Field_GetActorSystem(field), x, z, GRID_TO_FX32(y), FX32_CONST(8), FALSE);

    if (actor != NULL) {
        *found = TRUE;
        *uid = GetActorUID(actor);
    } else {
        *found = FALSE;
    }
    return FALSE;
}

BOOL s006F_PlayerGetActorInFront(VM *vm, FieldScriptEnv *env) {
    FieldPlayer *player = Field_GetPlayer(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    u16 *uid = ScriptReadVar(vm, env);
    u16 *found = ScriptReadVar(vm, env);
    FieldActor *actor = FieldPlayer_GetActorInFrontEx(player, FX32_CONST(8));

    if (actor != NULL) {
        *found = TRUE;
        *uid = GetActorUID(actor);
    } else {
        *found = FALSE;
    }
    return FALSE;
}

static BOOL ScriptNative_WaitPlayerSpecialSequence(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    if (FieldPlayer_StepFormChange(Field_GetPlayer(fieldWork->field)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL s0075_PlayerSetSpecialSequence(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    u16 seq = VM_Read16(vm);
    FieldPlayer *player = Field_GetPlayer(fieldWork->field);

    FieldPlayer_SetSpecialSeq(player, seq);
    if (FieldPlayer_StepFormChange(player) == TRUE) {
        return FALSE;
    }
    VM_SetNativeCallback(vm, ScriptNative_WaitPlayerSpecialSequence);
    return TRUE;
}

BOOL s0076_PlayerMoveToYAsync(VM *vm, FieldScriptEnv *env) {
    VecFx32 pos;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    Field *field = fieldWork->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    u16 negate = ScriptReadAny(vm, env);
    u16 frames = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 up = ScriptReadAny(vm, env);
    s32 duration = frames;

    if (negate) {
        duration = -frames;
    }
    if (up == 0) {
        VEC_Set(&pos, 0, -y * FX32_ONE, 0);
    } else {
        VEC_Set(&pos, 0, y * FX32_ONE, 0);
    }
    FieldTaskManager_AddTask(
        taskManager, FieldActorMoveTask_CreateAbs(field, duration, &pos, FieldPlayer_GetActor(Field_GetPlayer(field))),
        0);
    return FALSE;
}

BOOL s0248_PlayerMoveToYAsync_(VM *vm, FieldScriptEnv *env) {
    VecFx32 pos;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    Field *field = fieldWork->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    u16 negate = ScriptReadAny(vm, env);
    u16 frames = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 up = ScriptReadAny(vm, env);
    s32 duration = frames;

    if (negate) {
        duration = -frames;
    }
    if (up == 0) {
        VEC_Set(&pos, 0, -y * FX32_ONE, 0);
    } else {
        VEC_Set(&pos, 0, y * FX32_ONE, 0);
    }
    FieldTaskManager_AddTask(
        taskManager, FieldActorMoveTask_CreateAbs(field, duration, &pos, FieldPlayer_GetActor(Field_GetPlayer(field))),
        0);
    return FALSE;
}

static FieldActor *FindScrEnvPlayerActor(FieldScriptEnv *env) {
    return FindPlayerFieldActor(GetScrEnvMMdlSys(env));
}

// Turns the player to face the trigger the player stands on
BOOL s0077_PlayerTurnByTrigger(VM *vm, FieldScriptEnv *env) {
    int i;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    Field *field = fieldWork->field;
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 dirs[4] = { 0, 1, 2, 3 };
    EventData *eventData = GameData_GetEventData(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    FieldPlayer *player = Field_GetPlayer(field);
    VecFx32 pos;
    u32 dir;
    u32 trigDir;
    u32 faceDir;
    const u32 *action;

    FieldPlayer_GetWPos(player, &pos);
    for (i = 0; i < 4; i++) {
        trigDir = dirs[i];
        if (FindTriggerAtPosGrid(eventData, eventWork, &pos, trigDir) != NULL) {
            dir = trigDir;
            break;
        }
    }
    // BUG: With no trigger, the original compares an uninitialized direction.
#ifdef BUGFIX
    if (i == NELEMS(dirs)) {
        return FALSE;
    }
    faceDir = FieldPlayer_GetFaceDir(player);
#else
    faceDir = FieldPlayer_GetFaceDir(player);
#endif
    if (faceDir != dir) {
        if (dir == 0) {
            action = ACMD_QUEUE_TURN_N_8F;
        } else if (dir == 1) {
            action = ACMD_QUEUE_TURN_S_8F;
        } else if (dir == 2) {
            action = ACMD_QUEUE_TURN_W_8F;
        } else if (dir == 3) {
            action = ACMD_QUEUE_TURN_E_8F;
        } else {
            return FALSE;
        }
        ScriptWork_CallEvent(work, EventActionCall_Create(gsys, field, 0xff, action));
        return TRUE;
    }
    return FALSE;
}

BOOL s0078_PlayerGetExState(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    FieldPlayer *player = Field_GetPlayer(fieldWork->field);
    u16 *result = ScriptReadVar(vm, env);

    switch (FieldPlayer_GetExState(player)) {
    case 0:
        *result = 0;
        break;
    case 1:
        *result = 1;
        break;
    case 2:
        *result = 2;
        break;
    case 3:
        *result = 3;
        break;
    default:
        *result = 0;
        break;
    }
    return FALSE;
}

BOOL s007B_ActorJumpToGPos(VM *vm, FieldScriptEnv *env) {
    VecFx32 start;
    VecFx32 end;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 id = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FieldActor *actor = FindFieldActor(mmSys, id);

    CopyActorWPos(actor, &start);
    end.x = GRID_CENTER(x);
    end.y = GRID_TO_FX32(y);
    end.z = GRID_CENTER(z);
    ScriptWork_CallEvent(work, EventActorJump_Create(gsys, actor, &start, &end));
    return TRUE;
}

BOOL ScriptNative_CheckPlayerFormModel(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    FieldPlayer *player = Field_GetPlayer(fieldWork->field);
    u16 *result = ScriptReadVar(vm, env);

    *result = FieldPlayer_CheckFormModel(player);
    return FALSE;
}

BOOL s02BE_ActorMoveLinear(VM *vm, FieldScriptEnv *env) {
    VecFx32 start;
    VecFx32 end;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 id = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 frames = ScriptReadAny(vm, env);
    FieldActor *actor = FindScriptActor(env, id);
    GameEvent *event;

    if (actor == NULL) {
        return FALSE;
    }
    CopyActorWPos(actor, &start);
    end.x = GRID_CENTER(x);
    end.y = GRID_TO_FX32(y);
    end.z = GRID_CENTER(z);
    event = EventActorLinearMove_Create(gsys, actor, &start, &end, frames);
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s0071_PlayerGetRailPos(VM *vm, FieldScriptEnv *env) {
    FieldActor *actor = FindScrEnvPlayerActor(env);
    u16 *a = ScriptReadVar(vm, env);
    u16 *b = ScriptReadVar(vm, env);
    u16 *c = ScriptReadVar(vm, env);

    SetWkToActorRailPos(FldAct_GetRailUnit(actor), a, b, c);
    return FALSE;
}

BOOL s0072_ActorGetRailPos(VM *vm, FieldScriptEnv *env) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *actor = FindFieldActor(mmSys, ScriptReadAny(vm, env));
    u16 *a = ScriptReadVar(vm, env);
    u16 *b = ScriptReadVar(vm, env);
    u16 *c = ScriptReadVar(vm, env);

    SetWkToActorRailPos(FldAct_GetRailUnit(actor), a, b, c);
    return FALSE;
}

BOOL s007A_ActorPlayRailSlipdown(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *actor = FindFieldActor(mmSys, ScriptReadAny(vm, env));
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);

    ScriptWork_CallEvent(work, func_ov033_0217a148(gsys, fieldWork->field, actor));
    return TRUE;
}

BOOL s007C_PlayerSetRailPos(VM *vm, FieldScriptEnv *env) {
    RailPosition railPos;
    FieldActor *actor = FindScrEnvPlayerActor(env);
    u16 a = ScriptReadAny(vm, env);
    u16 b = ScriptReadAny(vm, env);
    u16 c = ScriptReadAny(vm, env);
    RailUnit *unit = FldAct_GetRailUnit(actor);

    RailUnit_SetLineLocation(unit, a, b, c);
    RailUnit_GetRailPos(unit, &railPos);
    SetActorInitedPositionRail(actor, &railPos);
    return FALSE;
}

BOOL s007D_ActorSetRailPos(VM *vm, FieldScriptEnv *env) {
    RailPosition railPos;
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *actor = FindFieldActor(mmSys, ScriptReadAny(vm, env));
    u16 a = ScriptReadAny(vm, env);
    u16 b = ScriptReadAny(vm, env);
    u16 c = ScriptReadAny(vm, env);
    RailUnit *unit = FldAct_GetRailUnit(actor);

    RailUnit_SetLineLocation(unit, a, b, c);
    RailUnit_GetRailPos(unit, &railPos);
    SetActorInitedPositionRail(actor, &railPos);
    return FALSE;
}

BOOL s007E_ActorPlayTeleportSeq(VM *vm, FieldScriptEnv *env) {
    VecFx32 offset;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 id = ScriptReadAny(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvMMdlSys(env), id);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    Field *field = fieldWork->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    FieldTask *moveTask;

    GFL_SndSEPlay(SEQ_SE_FLD_05);
    offset.x = 0;
    offset.y = FX32_CONST(150);
    offset.z = 0;
    moveTask = FieldActorMoveTask_CreateRel(field, 24, &offset, actor);
    FieldTaskManager_AddTask(taskManager, FieldActorSpinTask_Create(field, 24, 3, actor), 0);
    FieldTaskManager_AddTask(taskManager, moveTask, 0);
    return FALSE;
}

BOOL s0234_ActorFallDownToXZ(VM *vm, FieldScriptEnv *env) {
    VecFx32 pos;
    fx32 height;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 id = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 dir = ScriptReadAny(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvMMdlSys(env), id);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    Field *field = fieldWork->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    if (actor == NULL) {
        return FALSE;
    }
    height = 0;
    pos.x = GRID_CENTER(x);
    pos.y = FX32_CONST(250);
    pos.z = GRID_CENTER(z);
    GetHeightFromMap(actor, &pos, &height);
    pos.y = height;
    SetActorGPos(actor, x, HEIGHT_TO_GRID(pos.y), z, dir);
    SetActorWPosValue(actor, &pos);
    {
        VecFx32 offset = { 0, FX32_CONST(250), 0 };

        SetActorWPosOffset(actor, &offset);
    }
    FieldTaskManager_AddTask(taskManager, FieldActorFallTask_Create(field, actor, 40, 250), 0);
    return FALSE;
}
