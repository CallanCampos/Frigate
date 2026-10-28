// The Trainers who see the player: finding them, and the event that walks them up to the player for the battle. The
// name is the ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/battle.h"
#include "field/encounter.h"
#include "field/event_trainer_eye.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_comm_actor.h"
#include "field/field_effect.h"
#include "field/field_event.h"
#include "field/field_script_event.h"
#include "field/pair_sys.h"
#include "field/trainer_script.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The script of a Trainer's battle, and of the Trainer whose actor uses SCRID_SPECIAL_TRAINER
#define SCRID_TRAINER_CLASH 0xee8
#define SCRID_SPECIAL_TRAINER_CLASH 0x29f8

enum {
    TRAINER_EYE_SEQ_START_MOVE,
    TRAINER_EYE_SEQ_FORCE_TRAINER_DIR,
    TRAINER_EYE_SEQ_DECIDE_JUMP_OUT,
    TRAINER_EYE_SEQ_TURN_TRAINER,
    TRAINER_EYE_SEQ_WAIT_TURN_TRAINER,
    TRAINER_EYE_SEQ_START_TERRAIN_EFFECT,
    TRAINER_EYE_SEQ_WAIT_TERRAIN_EFFECT,
    TRAINER_EYE_SEQ_JUMP_OUT,
    TRAINER_EYE_SEQ_WAIT_JUMP_OUT,
    TRAINER_EYE_SEQ_WAIT_30F,
    TRAINER_EYE_SEQ_DECIDE_WALK_OR_STOP,
    TRAINER_EYE_SEQ_WALK_ONE_STEP_TO_PLAYER,
    TRAINER_EYE_SEQ_DECREMENT_CLASH_DIST,
    TRAINER_EYE_SEQ_WAIT_8F,
    TRAINER_EYE_SEQ_TURN_PLAYER,
    TRAINER_EYE_SEQ_WAIT_TURN_PLAYER,
    TRAINER_EYE_SEQ_SET_IDLE_MOVE_CODE,
    TRAINER_EYE_SEQ_FINISH,
};

struct EventTrainerEyeWork {
    int seq;
    BOOL finished;
    TrainerClashData data;
    u32 unk20;
    // Set for the second Trainer of two, who leaves the player facing the first unless they battle at once
    BOOL isPair;
    int timer;
    FieldEffectTask *emote;
    Field *field;
    FieldPlayer *player;
};

typedef struct {
    EventTrainerEyeWork *eyes[2];
} EventTrainerEyeEvent;

typedef int (*TrainerLineOfSightFunc)(s16 range, const GridPos *trainer, const GridPos *player);
typedef BOOL (*EventTrainerEyeRoutine)(EventTrainerEyeWork *work);

static BOOL EventTrainerEye_FindClashActor(Field *field, FieldActor *exclude, TrainerClashData *data);
static int EventTrainerEye_CheckActorClash(EncountSystem *encounter, FieldActor *actor, FieldActor *player,
                                           u16 *dirOut);
static int EventTrainerEye_CalcLineOfSight(u16 dir, s16 range, const GridPos *trainer, const GridPos *player);
static int EventTrainerEye_CalcLineOfSight_U(s16 range, const GridPos *trainer, const GridPos *player);
static int EventTrainerEye_CalcLineOfSight_D(s16 range, const GridPos *trainer, const GridPos *player);
static int EventTrainerEye_CalcLineOfSight_L(s16 range, const GridPos *trainer, const GridPos *player);
static int EventTrainerEye_CalcLineOfSight_R(s16 range, const GridPos *trainer, const GridPos *player);
static BOOL EventTrainerEyeSub_StartMove(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_ForceTrainerDir(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_DecideJumpOut(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_TurnTrainer(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_WaitTurnTrainer(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_StartTerrainEffect(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_WaitTerrainEffect(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_JumpOut(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_WaitJumpOut(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_Wait30F(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_DecideWalkOrStop(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_WalkOneStepToPlayer(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_DecrementClashDist(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_Wait8F(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_TurnPlayer(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_WaitTurnPlayer(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_SetIdleMoveCode(EventTrainerEyeWork *work);
static BOOL EventTrainerEyeSub_Finish(EventTrainerEyeWork *work);
static BOOL EventTrainerEye_Update(EventTrainerEyeWork *work);
static GameEventReturnCode EventTrainerEye_Callback(GameEvent *event, u32 *state, void *data);
static u16 EventTrainerEye_GetSightType(FieldActor *actor);
static BOOL EventTrainerEye_CheckPathObstacle(FieldActor *actor, u16 dir, int range, EncountSystem *encounter);
static u16 EventTrainerEye_UsesSpecialScript(FieldActor *actor);
static BOOL CheckTrainerAlreadyDefeated(Field *field, FieldActor *actor);
static void InitFieldScriptTrainerSetup(TrainerClashData *data, FieldActor *actor, int range, u16 dir);
static FieldActor *EventTrainerEye_FindPairedTrainer(FieldActor *trainer, u16 trainerId);
static GameEvent *CreateTrainerClashEvent(Field *field, FieldActor *actor);

static const TrainerLineOfSightFunc TRAINER_LOS_DIST_FUNCTIONS[4] = {
    EventTrainerEye_CalcLineOfSight_U,
    EventTrainerEye_CalcLineOfSight_D,
    EventTrainerEye_CalcLineOfSight_L,
    EventTrainerEye_CalcLineOfSight_R,
};

// clang-format off
static const EventTrainerEyeRoutine EVENT_TRAINER_EYE_ROUTINES[] = {
    EventTrainerEyeSub_StartMove,
    EventTrainerEyeSub_ForceTrainerDir,
    EventTrainerEyeSub_DecideJumpOut,
    EventTrainerEyeSub_TurnTrainer,
    EventTrainerEyeSub_WaitTurnTrainer,
    EventTrainerEyeSub_StartTerrainEffect,
    EventTrainerEyeSub_WaitTerrainEffect,
    EventTrainerEyeSub_JumpOut,
    EventTrainerEyeSub_WaitJumpOut,
    EventTrainerEyeSub_Wait30F,
    EventTrainerEyeSub_DecideWalkOrStop,
    EventTrainerEyeSub_WalkOneStepToPlayer,
    EventTrainerEyeSub_DecrementClashDist,
    EventTrainerEyeSub_Wait8F,
    EventTrainerEyeSub_TurnPlayer,
    EventTrainerEyeSub_WaitTurnPlayer,
    EventTrainerEyeSub_SetIdleMoveCode,
    EventTrainerEyeSub_Finish,
};

// clang-format on

GameEvent *EventTrainerEye_CheckAll(Field *field) {
    TrainerClashData data;
    TrainerClashData partner;
    u32 battleCount;
    int battleType;
    GameEvent *event = NULL;
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));

    if (EventTrainerEye_FindClashActor(field, NULL, &data) == TRUE) {
        battleCount = FieldEventProvider_CountAlivePartyMembers(Field_GetGameSystem(field));
        if (EventTrainerEye_UsesSpecialScript(data.actor) == TRUE) {
            battleType = BTL_STYLE_SINGLE;
        } else {
            battleType = getBattleType(data.trainerId);
        }
        if (battleType == BTL_STYLE_SINGLE) {
            if (battleCount >= 1 && GetNowFollowerAllyTrID(gameData) != 0 &&
                EventTrainerEye_FindClashActor(field, data.actor, &partner) == TRUE) {
                event = CreateTrainerClashEvent(field, data.actor);
                data.kind = TRAINER_CLASH_WITH_PARTNER;
                SetupTrainerClashSlot(event, 0, &data);
                partner.kind = TRAINER_CLASH_WITH_PARTNER;
                SetupTrainerClashSlot(event, 1, &partner);
            } else if (battleCount < 2 || EventTrainerEye_FindClashActor(field, data.actor, &partner) == FALSE) {
                event = CreateTrainerClashEvent(field, data.actor);
                data.kind = TRAINER_CLASH_SINGLE;
                SetupTrainerClashSlot(event, 0, &data);
            } else {
                event = CreateTrainerClashEvent(field, data.actor);
                data.kind = TRAINER_CLASH_TWO_TRAINERS;
                SetupTrainerClashSlot(event, 0, &data);
                partner.kind = TRAINER_CLASH_TWO_TRAINERS;
                SetupTrainerClashSlot(event, 1, &partner);
            }
        } else if (battleType == BTL_STYLE_DOUBLE) {
            if (battleCount >= 2) {
                InitFieldScriptTrainerSetup(&partner, EventTrainerEye_FindPairedTrainer(data.actor, data.trainerId),
                                            data.range, data.dir);
                event = CreateTrainerClashEvent(field, data.actor);
                data.kind = TRAINER_CLASH_DOUBLE_PAIR;
                SetupTrainerClashSlot(event, 0, &data);
                partner.kind = TRAINER_CLASH_DOUBLE_PAIR;
                SetupTrainerClashSlot(event, 1, &partner);
            }
        } else if (battleType == BTL_STYLE_TRIPLE || battleType == BTL_STYLE_ROTATION) {
            if (battleCount >= 3) {
                event = CreateTrainerClashEvent(field, data.actor);
                data.kind = TRAINER_CLASH_SINGLE;
                SetupTrainerClashSlot(event, 0, &data);
            }
        }
    }
    return event;
}

int EventTrainerEyeSub_GetPlayerDistance(FieldActor *actor, u16 dir, int range, EncountSystem *encounter) {
    GridPos pos;
    GridPos playerPos;
    int dist;
    FieldActor *player = FindFieldActor(GetActorMModelSystem(actor), ACTOR_UID_PLAYER);

    if (player == NULL) {
        return -1;
    }
    FldAct_GetGPos(player, &playerPos);
    FldAct_GetGPos(actor, &pos);
    dist = EventTrainerEye_CalcLineOfSight(dir, range, &pos, &playerPos);
    if (dist != -1 && EventTrainerEye_CheckPathObstacle(actor, dir, dist, encounter) == TRUE) {
        dist = -1;
    }
    return dist;
}

// The first actor other than exclude that sees the player and is a Trainer still to battle
static BOOL EventTrainerEye_FindClashActor(Field *field, FieldActor *exclude, TrainerClashData *data) {
    FieldActor *actor;
    u32 index;
    u16 dir;
    int dist;
    EncountSystem *encounter = Field_GetEncountSystem(field);
    FieldActor *player = FieldPlayer_GetActor(Field_GetPlayer(field));
    MMSys *actorSys = Field_GetActorSystem(field);

    actor = NULL;
    index = 0;
    while (NextActor(actorSys, &actor, &index) == TRUE) {
        if (actor == player || (exclude != NULL && exclude == actor)) {
            continue;
        }
        dist = EventTrainerEye_CheckActorClash(encounter, actor, player, &dir);
        if (dist != -1 && !CheckTrainerAlreadyDefeated(field, actor)) {
            InitFieldScriptTrainerSetup(data, actor, dist, dir);
            return TRUE;
        }
    }
    return FALSE;
}

// How far a Trainer sees the player, facing its way or, for one that looks around, any way; -1 if it doesn't
static int EventTrainerEye_CheckActorClash(EncountSystem *encounter, FieldActor *actor, FieldActor *player,
                                           u16 *dirOut) {
    GridPos pos;
    GridPos playerPos;
    u16 faceDir;
    int dist;
    s16 range;
    u16 dir;
    u16 eyeType = EventTrainerEye_GetSightType(actor);

    if (eyeType == TRAINER_SIGHT_FACING || eyeType == TRAINER_SIGHT_ALL_DIRS) {
        dir = DIR_UP;
        range = GetActorUserParam(actor, 0);
        FldAct_GetGPos(actor, &pos);
        FldAct_GetGPos(player, &playerPos);
        if (eyeType == TRAINER_SIGHT_FACING) {
            faceDir = GetActorFaceDir(actor);
            dist = EventTrainerEye_CalcLineOfSight(faceDir, range, &pos, &playerPos);
            if (dist != -1 && !EventTrainerEye_CheckPathObstacle(actor, faceDir, dist, encounter)) {
                *dirOut = faceDir;
                return dist;
            }
        } else {
            do {
                dist = EventTrainerEye_CalcLineOfSight(dir, range, &pos, &playerPos);
                if (dist != -1 && !EventTrainerEye_CheckPathObstacle(actor, dir, dist, encounter)) {
                    *dirOut = dir;
                    return dist;
                }
                dir++;
            } while (dir < 4);
        }
    }
    return -1;
}

static int EventTrainerEye_CalcLineOfSight(u16 dir, s16 range, const GridPos *trainer, const GridPos *player) {
    return TRAINER_LOS_DIST_FUNCTIONS[dir](range, trainer, player);
}

static int EventTrainerEye_CalcLineOfSight_U(s16 range, const GridPos *trainer, const GridPos *player) {
    if (trainer->x == player->x && trainer->y == player->y && player->z < trainer->z &&
        player->z >= trainer->z - range) {
        return trainer->z - player->z;
    }
    return -1;
}

static int EventTrainerEye_CalcLineOfSight_D(s16 range, const GridPos *trainer, const GridPos *player) {
    if (trainer->x == player->x && trainer->y == player->y && player->z > trainer->z &&
        player->z <= trainer->z + range) {
        return player->z - trainer->z;
    }
    return -1;
}

static int EventTrainerEye_CalcLineOfSight_L(s16 range, const GridPos *trainer, const GridPos *player) {
    if (trainer->z == player->z && trainer->y == player->y && player->x < trainer->x &&
        player->x >= trainer->x - range) {
        return trainer->x - player->x;
    }
    return -1;
}

static int EventTrainerEye_CalcLineOfSight_R(s16 range, const GridPos *trainer, const GridPos *player) {
    if (trainer->z == player->z && trainer->y == player->y && player->x > trainer->x &&
        player->x <= trainer->x + range) {
        return player->x - trainer->x;
    }
    return -1;
}

GameEvent *EventTrainerEye_Create(GameSystem *gsys, EventTrainerEyeWork *eye1, EventTrainerEyeWork *eye2) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventTrainerEye_Callback, sizeof(EventTrainerEyeEvent));
    EventTrainerEyeEvent *data = GameEvent_GetData(event);

    data->eyes[0] = eye1;
    data->eyes[1] = eye2;
    return event;
}

EventTrainerEyeWork *EventTrainerEye_CreateData(HeapID heapId, Field *field, const TrainerClashData *data,
                                                BOOL isPair) {
    EventTrainerEyeWork *work = GFL_HeapAllocate(heapId, sizeof(EventTrainerEyeWork), TRUE, "event_trainer_eye.c", 541);

    work->data = *data;
    work->unk20 = 0;
    work->isPair = isPair;
    work->field = field;
    work->player = Field_GetPlayer(field);
    return work;
}

static BOOL EventTrainerEyeSub_StartMove(EventTrainerEyeWork *work) {
    if (IsActorFlag16(work->data.actor) == TRUE) {
        EnableActorMovement(work->data.actor);
    }
    work->seq = TRAINER_EYE_SEQ_FORCE_TRAINER_DIR;
    return TRUE;
}

static BOOL EventTrainerEyeSub_ForceTrainerDir(EventTrainerEyeWork *work) {
    if (IsActorFlag16(work->data.actor) == TRUE) {
        return FALSE;
    }
    CheckSetActorFaceDir(work->data.actor, work->data.dir);
    DisableActorMovement(work->data.actor);
    work->seq = TRAINER_EYE_SEQ_DECIDE_JUMP_OUT;
    return TRUE;
}

// Trainers hidden in the terrain jump out; the others turn to the player
static BOOL EventTrainerEyeSub_DecideJumpOut(EventTrainerEyeWork *work) {
    u32 moveCode;

    if (!FldAct_CheckAcmdEnd(FieldPlayer_GetActor(work->player))) {
        return FALSE;
    }
    work->seq = TRAINER_EYE_SEQ_TURN_TRAINER;
    moveCode = GetActorMoveCode(work->data.actor);
    switch (moveCode) {
    case MOVE_CODE_HIDE_33:
    case MOVE_CODE_HIDE_34:
    case MOVE_CODE_HIDE_35:
    case MOVE_CODE_HIDE_36:
    case MOVE_CODE_HIDE_LEAVES:
        work->seq = TRAINER_EYE_SEQ_JUMP_OUT;
        break;
    }
    return TRUE;
}

static BOOL EventTrainerEyeSub_TurnTrainer(EventTrainerEyeWork *work) {
    if (IsAllActorAcmdFinished(work->data.actor) == TRUE) {
        FldAct_SetAcmd(work->data.actor, GetAcmdForDir(work->data.dir, ACMD_FACE));
        work->seq = TRAINER_EYE_SEQ_WAIT_TURN_TRAINER;
    }
    return FALSE;
}

static BOOL EventTrainerEyeSub_WaitTurnTrainer(EventTrainerEyeWork *work) {
    if (!FldAct_CheckAcmdEnd(work->data.actor)) {
        return FALSE;
    }
    work->seq = TRAINER_EYE_SEQ_START_TERRAIN_EFFECT;
    return TRUE;
}

// The exclamation mark over the Trainer
static BOOL EventTrainerEyeSub_StartTerrainEffect(EventTrainerEyeWork *work) {
    work->emote = FldeffEmotionBubble_SetActor(Field_GetFieldEffects(work->field), work->data.actor, 0, 0);
    work->seq = TRAINER_EYE_SEQ_WAIT_TERRAIN_EFFECT;
    return FALSE;
}

static BOOL EventTrainerEyeSub_WaitTerrainEffect(EventTrainerEyeWork *work) {
    if (FldeffEmotionBubble_IsFinished(work->emote) == TRUE) {
        FieldEffects_DeleteObject(work->emote);
        work->seq = TRAINER_EYE_SEQ_WAIT_30F;
    }
    return FALSE;
}

static BOOL EventTrainerEyeSub_JumpOut(EventTrainerEyeWork *work) {
    FldAct_SetAcmd(work->data.actor, 0x65);
    work->seq = TRAINER_EYE_SEQ_WAIT_JUMP_OUT;
    return FALSE;
}

static BOOL EventTrainerEyeSub_WaitJumpOut(EventTrainerEyeWork *work) {
    if (FldAct_CheckAcmdEnd(work->data.actor) == TRUE) {
        work->seq = TRAINER_EYE_SEQ_WAIT_30F;
    }
    return FALSE;
}

static BOOL EventTrainerEyeSub_Wait30F(EventTrainerEyeWork *work) {
    if (++work->timer >= 30) {
        work->timer = 0;
        work->seq = TRAINER_EYE_SEQ_DECIDE_WALK_OR_STOP;
    }
    return FALSE;
}

static BOOL EventTrainerEyeSub_DecideWalkOrStop(EventTrainerEyeWork *work) {
    if (work->data.range <= 1) {
        work->seq = TRAINER_EYE_SEQ_WAIT_8F;
        return TRUE;
    }
    work->seq = TRAINER_EYE_SEQ_WALK_ONE_STEP_TO_PLAYER;
    return TRUE;
}

static BOOL EventTrainerEyeSub_WalkOneStepToPlayer(EventTrainerEyeWork *work) {
    if (IsAllActorAcmdFinished(work->data.actor) == TRUE) {
        FldAct_SetAcmd(work->data.actor, GetAcmdForDir(work->data.dir, ACMD_WALK_8F));
        work->seq = TRAINER_EYE_SEQ_DECREMENT_CLASH_DIST;
    }
    return FALSE;
}

static BOOL EventTrainerEyeSub_DecrementClashDist(EventTrainerEyeWork *work) {
    if (!FldAct_CheckAcmdEnd(work->data.actor)) {
        return FALSE;
    }
    work->data.range--;
    work->seq = TRAINER_EYE_SEQ_DECIDE_WALK_OR_STOP;
    return TRUE;
}

static BOOL EventTrainerEyeSub_Wait8F(EventTrainerEyeWork *work) {
    if (++work->timer < 8) {
        return FALSE;
    }
    work->timer = 0;
    work->seq = TRAINER_EYE_SEQ_TURN_PLAYER;
    return TRUE;
}

static BOOL EventTrainerEyeSub_TurnPlayer(EventTrainerEyeWork *work) {
    FieldActor *player = FieldPlayer_GetActor(work->player);
    s16 playerX = GetGPosX(player);
    s16 playerZ = GetGPosZ(player);
    u16 dir = GetDirFromPosToPos(playerX, playerZ, GetGPosX(work->data.actor), GetGPosZ(work->data.actor));

    if (dir != GetActorFaceDir(player) && (!work->isPair || work->data.kind == TRAINER_CLASH_TWO_TRAINERS)) {
        if (IsAllActorAcmdFinished(player) == TRUE) {
            FldAct_SetAcmd(player, GetAcmdForDir(dir, ACMD_FACE));
            work->seq = TRAINER_EYE_SEQ_WAIT_TURN_PLAYER;
        }
    } else {
        work->seq = TRAINER_EYE_SEQ_SET_IDLE_MOVE_CODE;
    }
    return FALSE;
}

static BOOL EventTrainerEyeSub_WaitTurnPlayer(EventTrainerEyeWork *work) {
    FieldActor *player = FieldPlayer_GetActor(work->player);

    if (!FldAct_CheckAcmdEnd(player)) {
        return FALSE;
    }
    FldAct_EndAcmd(player);
    work->seq = TRAINER_EYE_SEQ_SET_IDLE_MOVE_CODE;
    return TRUE;
}

static BOOL EventTrainerEyeSub_SetIdleMoveCode(EventTrainerEyeWork *work) {
    FldAct_EndAcmd(work->data.actor);
    ChangeActorMoveCodeSeq(work->data.actor, MOVE_CODE_NONE);
    work->seq = TRAINER_EYE_SEQ_FINISH;
    return TRUE;
}

static BOOL EventTrainerEyeSub_Finish(EventTrainerEyeWork *work) {
    work->finished = TRUE;
    return FALSE;
}

// Run the routines until one waits; whether the Trainer is done
static BOOL EventTrainerEye_Update(EventTrainerEyeWork *work) {
    while (EVENT_TRAINER_EYE_ROUTINES[work->seq](work) == TRUE) {
    }
    if (work->finished == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static GameEventReturnCode EventTrainerEye_Callback(GameEvent *event, u32 *state, void *data) {
    EventTrainerEyeEvent *eyeEvent = data;

    if (eyeEvent->eyes[0] != NULL && EventTrainerEye_Update(eyeEvent->eyes[0]) == TRUE) {
        GFL_HeapFree(eyeEvent->eyes[0]);
        eyeEvent->eyes[0] = NULL;
    }
    if (eyeEvent->eyes[1] != NULL && EventTrainerEye_Update(eyeEvent->eyes[1]) == TRUE) {
        GFL_HeapFree(eyeEvent->eyes[1]);
        eyeEvent->eyes[1] = NULL;
    }
    if (eyeEvent->eyes[0] == NULL && eyeEvent->eyes[1] == NULL) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// How the actor looks for the player: TRAINER_SIGHT_FACING or TRAINER_SIGHT_ALL_DIRS for a Trainer
static u16 EventTrainerEye_GetSightType(FieldActor *actor) {
    return EvType_GetSightType(GetActorEvType(actor));
}

// Whether anything stands between the Trainer and the tile range tiles away in dir
static BOOL EventTrainerEye_CheckPathObstacle(FieldActor *actor, u16 dir, int range, EncountSystem *encounter) {
    GridPos pos;
    int i;
    s16 dx;
    s16 dz;

    if (range != 0) {
        i = 0;
        FldAct_GetGPos(actor, &pos);
        dx = GetDirectionVectorCompX(dir);
        dz = GetDirectionVectorCompZ(dir);
        pos.x += dx;
        pos.z += dz;
        for (; i < range - 1; i++) {
            if (ActorRouteCollCheck(actor, pos.x, pos.y, pos.z, dir) & ~ACTOR_COLL_WALK_AREA) {
                return TRUE;
            }
            if (encounter != NULL && EncountState_CheckSpecialEncountPos(encounter, &pos.x)) {
                return TRUE;
            }
            pos.x += dx;
            pos.z += dz;
        }
        if ((ActorRouteCollCheck(actor, pos.x, pos.y, pos.z, dir) & ~ACTOR_COLL_WALK_AREA) != ACTOR_COLL_ACTOR) {
            return TRUE;
        }
        if (encounter != NULL && EncountState_CheckSpecialEncountPos(encounter, &pos.x)) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 GetNPCTrainerID(FieldActor *actor) {
    if (EventTrainerEye_UsesSpecialScript(actor) != TRUE) {
        return GetNPCTrainerIDFromSCRID(FldAct_GetSCRID(actor));
    }
    return GetActorUID(actor);
}

// Whether the actor uses the special Trainer script, whose Trainer ID is its UID
static u16 EventTrainerEye_UsesSpecialScript(FieldActor *actor) {
    return FldAct_GetSCRID(actor) == SCRID_SPECIAL_TRAINER;
}

static BOOL CheckTrainerAlreadyDefeated(Field *field, FieldActor *actor) {
    u32 index;
    KeyDataSave *keyData;
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    EventWork *eventWork = GameData_GetEventWork(gameData);

    if (EventTrainerEye_UsesSpecialScript(actor) != TRUE) {
        return TrainerFlagGet(eventWork, GetNPCTrainerID(actor));
    }
    index = GetNPCTrainerID(actor) - 0xb0;
    keyData = getKeyDataBlkAddress(GameData_GetSaveControl(gameData));
    return KeyDataSave_IsTrainerDefeated(keyData, KeyDataSave_GetFloorInArea(keyData), index);
}

static void InitFieldScriptTrainerSetup(TrainerClashData *data, FieldActor *actor, int range, u16 dir) {
    data->range = range;
    data->dir = dir;
    data->scrId = FldAct_GetSCRID(actor);
    data->trainerId = GetNPCTrainerID(actor);
    data->actor = actor;
}

// The other Trainer of a double battle pair
static FieldActor *EventTrainerEye_FindPairedTrainer(FieldActor *trainer, u16 trainerId) {
    FieldActor *actor;
    u32 index = 0;
    MMSys *actorSys = GetActorMModelSystem(trainer);
    u16 eyeType;

    while (NextActor(actorSys, &actor, &index) == TRUE) {
        if (actor != trainer) {
            eyeType = EventTrainerEye_GetSightType(actor);
            if ((eyeType == TRAINER_SIGHT_FACING || eyeType == TRAINER_SIGHT_ALL_DIRS) &&
                trainerId == GetNPCTrainerID(actor)) {
                return actor;
            }
        }
    }
    return NULL;
}

static GameEvent *CreateTrainerClashEvent(Field *field, FieldActor *actor) {
    GameSystem *gsys = Field_GetGameSystem(field);

    if (EventTrainerEye_UsesSpecialScript(actor) != TRUE) {
        return EventScriptCall_Create(gsys, SCRID_TRAINER_CLASH, actor, 1);
    }
    return EventScriptCall_Create(gsys, SCRID_SPECIAL_TRAINER_CLASH, actor, 1);
}
