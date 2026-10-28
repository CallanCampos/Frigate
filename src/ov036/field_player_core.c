#include "types.h"
#include "constants/obj_codes.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_sound.h"
#include "field/player_state.h"
#include "gfl/billboard_actor.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The part of the field's player that grid and rail movement share: the player's actor, how it moves and which model
// it wears, and the steps that change the model when the player starts or stops cycling, surfing or diving

#define POS_TO_GRID_RAW(pos) (((pos) >> 4) / FX32_ONE)
#define POS_TO_GRID(pos) ((s16)POS_TO_GRID_RAW(pos))

#define OBJ_CODE_NONE 0xffff
#define PLAYER_ACTOR_ID 0xff
#define SEQ_NONE 10

typedef BOOL (*ExStateChangeFunc)(FieldPlayerCore *core);

struct FieldPlayerCore {
    u32 unk0;
    GameSystem *gameSystem;
    Field *field;
    PlayerState *playerState;
    u32 observeStatus;
    u32 moveStatus;
    u32 sex;
    FieldActor *actor;
    u16 keyDirH;
    u16 keyDirV;
    u32 specialSeq;
    FieldEffectTask *terrainEffectTCB;
    u16 modelReadyFrames;
    // How many times the special sequence has changed the player's object code, which nothing reads
    u16 objCodeChanges;
    s16 seqStep;
    u16 seqDone;
};

static void FieldPlayerCore_ChangeObjCode(FieldPlayerCore *core, u16 objCode);
static void FieldPlayerCore_InitModelCore(FieldPlayerCore *core, u32 exState);
static BOOL FieldPlayerCore_IsBaseFormeObjCode(FieldPlayerCore *core);
static BOOL FieldPlayerCore_CheckDrawProcReady(FieldPlayerCore *core);
static u32 GetMoveDirHByKey(u32 keys);
static u32 GetMoveDirVByKey(u32 keys);
static u16 FieldPlayerCore_GetMoveDirByKey_(FieldPlayerCore *core, u16 heldKeys);
static void FieldPlayerCore_CalcSeparateKeyDirs_(FieldPlayerCore *core, u16 heldKeys);
static BOOL FieldPlayerCore_CheckSpecialObjCode(FieldPlayerCore *core, u16 objCode);
static void FieldPlayerCore_ReqSyncBGM(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_NONE(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_CYCLING(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepCycleDone(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_SURF(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepRestoreModel(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepRestoreModelDone(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot4(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot4Done(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot5(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot5Done(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot6(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot6Done(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot9(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot9Done(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_DIVE(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot8(FieldPlayerCore *core);
static BOOL FieldPlayerCore_FormStepSlot8Done(FieldPlayerCore *core);

// MWCC lays these out in a different order than they are defined; this definition order gives the original layout
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_DIVE[] = { FieldPlayerCore_ExStateChangeFunc_DIVE };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_NONE[] = { FieldPlayerCore_ExStateChangeFunc_NONE };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SURF[] = { FieldPlayerCore_ExStateChangeFunc_SURF };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SLOT9[] = { FieldPlayerCore_FormStepSlot9,
                                                                        FieldPlayerCore_FormStepSlot9Done };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_CYCLING[] = {
    FieldPlayerCore_ExStateChangeFunc_CYCLING,
    FieldPlayerCore_FormStepCycleDone,
};
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SLOT4[] = { FieldPlayerCore_FormStepSlot4,
                                                                        FieldPlayerCore_FormStepSlot4Done };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SLOT6[] = { FieldPlayerCore_FormStepSlot6,
                                                                        FieldPlayerCore_FormStepSlot6Done };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_RESTORE_MODEL[] = {
    FieldPlayerCore_FormStepRestoreModel, FieldPlayerCore_FormStepRestoreModelDone
};
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SLOT8[] = { FieldPlayerCore_FormStepSlot8,
                                                                        FieldPlayerCore_FormStepSlot8Done };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SLOT5[] = { FieldPlayerCore_FormStepSlot5,
                                                                        FieldPlayerCore_FormStepSlot5Done };

static const ZoneNPC PLAYER_NPC_TEMPLATE = {
    PLAYER_ACTOR_ID, OBJ_CODE_HERO, MOVE_CODE_NONE, ACTOR_EVENT_TYPE_NONE, 0, 0, 0, 0, 0, 0, 0xffff, 0xffff, FALSE,
};

static const ExStateChangeFunc *const PLAYER_EXSTATE_CHANGE_VTABLES[] = {
    PLAYER_EXSTATE_CHANGE_VTABLE_NONE,  PLAYER_EXSTATE_CHANGE_VTABLE_CYCLING,
    PLAYER_EXSTATE_CHANGE_VTABLE_SURF,  PLAYER_EXSTATE_CHANGE_VTABLE_RESTORE_MODEL,
    PLAYER_EXSTATE_CHANGE_VTABLE_SLOT4, PLAYER_EXSTATE_CHANGE_VTABLE_SLOT5,
    PLAYER_EXSTATE_CHANGE_VTABLE_SLOT6, PLAYER_EXSTATE_CHANGE_VTABLE_SLOT9,
    PLAYER_EXSTATE_CHANGE_VTABLE_DIVE,  PLAYER_EXSTATE_CHANGE_VTABLE_SLOT8,
};

FieldPlayerCore *FieldPlayerCore_Create(PlayerState *state, Field *field, const VecFx32 *pos, u32 sex, HeapID heapId) {
    FieldPlayerCore *core = GFL_HeapAllocate(heapId, sizeof(FieldPlayerCore), TRUE, "field_player_core.c", 151);
    u16 objCode;
    FieldActor *actor;
    MMSys *mmSys;

    core->playerState = state;
    core->field = field;
    core->gameSystem = Field_GetGameSystem(field);
    core->keyDirH = DIR_NONE;
    core->keyDirV = DIR_NONE;
    mmSys = Field_GetActorSystem(field);
    core->actor = FindFieldActor(mmSys, PLAYER_ACTOR_ID);
    core->sex = sex;
    objCode = FieldPlayerState_GetObjCode(core->playerState);
    actor = core->actor;
    if (actor == NULL) {
        ZoneNPC npc = PLAYER_NPC_TEMPLATE;

        ZoneNPCGridPosition *grid = &npc.pos.grid;

        grid->x = POS_TO_GRID(pos->x);
        grid->z = POS_TO_GRID(pos->z);
        grid->y = pos->y;
        if (objCode == OBJ_CODE_NONE) {
            npc.modelId = FieldPlayer_GetObjCodeByExState(sex, FieldPlayerCore_GetExState(core));
        } else {
            npc.modelId = objCode;
        }
        core->actor = CreateNewActorByEntityNoWKOBJCODE(mmSys, &npc, 0);
    } else {
        s32 y = POS_TO_GRID_RAW(pos->y);
        s32 z = POS_TO_GRID_RAW(pos->z);

        SetActorGPosX(actor, POS_TO_GRID(pos->x));
        SetActorGPosY(actor, y);
        SetActorGPosZ(actor, z);
        SetActorWPosValue(actor, pos);
        if (objCode == OBJ_CODE_NONE) {
            FieldPlayerCore_InitModel(core);
        }
    }
    SetActorFlag32(core->actor, TRUE);
    return core;
}

void FieldPlayerCore_Free(FieldPlayerCore *core) {
    GFL_HeapFree(core);
}

void FieldPlayerCore_UpdateActionStatusObserver(FieldPlayerCore *core) {
    FieldActor *actor = core->actor;
    u32 moveStatus;
    u32 prevObserve;

    prevObserve = core->observeStatus;
    core->observeStatus = PLAYER_OBSERVE_NONE;
    moveStatus = core->moveStatus;
    if (!IsAllActorAcmdFinished(actor)) {
        switch (moveStatus) {
        case PLAYER_MOVE_STATUS_NONE:
            return;
        case PLAYER_MOVE_STATUS_MOVE:
            if (prevObserve == PLAYER_OBSERVE_NONE || prevObserve == PLAYER_OBSERVE_END) {
                core->observeStatus = PLAYER_OBSERVE_START;
                return;
            }
            core->observeStatus = PLAYER_OBSERVE_MOVING;
            return;
        case PLAYER_MOVE_STATUS_TURN:
            core->observeStatus = PLAYER_OBSERVE_MOVING;
            return;
        default:
            return;
        }
    }
    if (FldAct_CheckAcmdEnd(actor) == TRUE) {
        switch (moveStatus) {
        case PLAYER_MOVE_STATUS_NONE:
            return;
        case PLAYER_MOVE_STATUS_MOVE:
            if (prevObserve == PLAYER_OBSERVE_NONE) {
                return;
            }
            if (prevObserve == PLAYER_OBSERVE_END) {
                core->observeStatus = PLAYER_OBSERVE_NONE;
                return;
            }
            core->observeStatus = PLAYER_OBSERVE_END;
            return;
        case PLAYER_MOVE_STATUS_TURN:
            if (prevObserve == PLAYER_OBSERVE_NONE) {
                return;
            }
            if (prevObserve == PLAYER_OBSERVE_END) {
                core->observeStatus = PLAYER_OBSERVE_NONE;
                return;
            }
            core->observeStatus = PLAYER_OBSERVE_END;
            return;
        default:
            return;
        }
    }
}

void FieldPlayerCore_GetWPos(FieldPlayerCore *core, VecFx32 *pos) {
    CopyActorWPos(core->actor, pos);
}

void FieldPlayerCore_SetWPos(FieldPlayerCore *core, const VecFx32 *pos) {
    s32 x = POS_TO_GRID_RAW(pos->x);
    s32 y = POS_TO_GRID_RAW(pos->y);
    s32 z = POS_TO_GRID_RAW(pos->z);
    FieldActor *actor = core->actor;

    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    SetActorGPosX(actor, x);
    SetActorGPosY(actor, y);
    SetActorGPosZ(actor, z);
    SetActorWPosValue(actor, pos);
}

u16 FieldPlayerCore_GetFaceDir(FieldPlayerCore *core) {
    return GetActorFaceDir(core->actor);
}

void FieldPlayerCore_SetDirection(FieldPlayerCore *core, u16 dir) {
    CheckSetActorFaceDir(core->actor, dir);
}

void FieldPlayerCore_ForceObjCode(FieldPlayerCore *core, u16 objCode) {
    FieldPlayerCore_ChangeObjCode(core, objCode);
    FieldPlayerState_SetObjCode(core->playerState, objCode);
}

void FieldPlayerCore_ClearObjCode(FieldPlayerCore *core) {
    FieldPlayerState_SetObjCode(core->playerState, OBJ_CODE_NONE);
}

Field *FieldPlayerCore_GetField(FieldPlayerCore *core) {
    return core->field;
}

FieldActor *FieldPlayerCore_GetActor(FieldPlayerCore *core) {
    return core->actor;
}

void FieldPlayerCore_SetMoveStatus(FieldPlayerCore *core, u32 status) {
    core->moveStatus = status;
}

u32 FieldPlayerCore_GetMoveStatus(FieldPlayerCore *core) {
    return core->moveStatus;
}

void FieldPlayerCore_ForceMoveObserveStatus(FieldPlayerCore *core, u32 status) {
    core->observeStatus = status;
}

u32 FieldPlayerCore_GetMoveObserveStatus(FieldPlayerCore *core) {
    return core->observeStatus;
}

u32 FieldPlayerCore_GetExState(FieldPlayerCore *core) {
    return FieldPlayerState_GetExState(core->playerState);
}

void FieldPlayerCore_SetExState(FieldPlayerCore *core, u32 state) {
    SetPlayerSpecialState(core->playerState, state);
}

u32 FieldPlayerCore_GetSex(FieldPlayerCore *core) {
    return core->sex;
}

GameSystem *FieldPlayerCore_GetGameSystem(FieldPlayerCore *core) {
    return core->gameSystem;
}

PlayerState *FieldPlayerCore_GetPlayerState(FieldPlayerCore *core) {
    return core->playerState;
}

void FieldPlayerCore_InitModel(FieldPlayerCore *core) {
    FieldPlayerCore_InitModelCore(core, FieldPlayerCore_GetExState(core));
}

void FieldPlayerCore_SetFormSlotModel(FieldPlayerCore *core, u32 forme) {
    u32 sex = FieldPlayerCore_GetSex(core);
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    u16 objCode = FieldPlayer_GetObjCodeByForme(sex, forme);

    if (objCode != FldAct_GetObjCode(actor)) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
    }
    core->modelReadyFrames = 0;
}

BOOL FieldPlayerCore_WaitModelReady(FieldPlayerCore *core) {
    if (FldAct_GetBillboardActorIdx(FieldPlayerCore_GetActor(core), 0) != BILLBOARD_ACTOR_NONE) {
        if (core->modelReadyFrames == 1) {
            return TRUE;
        }
        core->modelReadyFrames++;
    }
    return FALSE;
}

u32 FieldPlayerCore_DeriveExState(FieldPlayerCore *core) {
    return FieldPlayer_GetExStateByObjCode(FldAct_GetObjCode(FieldPlayerCore_GetActor(core)));
}

u32 FieldPlayerCore_CheckFormModel(FieldPlayerCore *core) {
    if (FieldPlayer_GetExStateByObjCode(FldAct_GetObjCode(FieldPlayerCore_GetActor(core))) == PLAYER_FORM_COUNT) {
        return FALSE;
    }
    return TRUE;
}

BOOL FieldPlayerCore_CheckAnimePaused(FieldPlayerCore *core) {
    BillboardActorSys *billboardActorSys =
        FieldBillboardActor_GetBillboardActorSys(FldActSys_GetFieldBillboardActor(GetActorMModelSystem(core->actor)));
    u32 idx;
    u16 cmd;

    if (FieldAcmdTCB_CheckAnimePaused(core->actor) == TRUE && FldBillboardActor_GetEntryHookFlag(core->actor) == TRUE) {
        return TRUE;
    }
    idx = FldAct_GetBillboardActorIdx(core->actor, 0);
    if (idx != BILLBOARD_ACTOR_NONE && BillboardActorSys_GetActorLastCmd(billboardActorSys, idx, &cmd) &&
        cmd == BILLBOARD_ACTOR_ANIM_CMD_END) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldPlayerCore_IsAcmdFinished(FieldPlayerCore *core) {
    if (IsAllActorAcmdFinished(FieldPlayerCore_GetActor(core)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

u16 FieldPlayerCore_GetMoveDirByKey(FieldPlayerCore *core, u32 heldKeys) {
    return FieldPlayerCore_GetMoveDirByKey_(core, heldKeys);
}

void FieldPlayerCore_CalcSeparateKeyDirs(FieldPlayerCore *core, u32 heldKeys) {
    FieldPlayerCore_CalcSeparateKeyDirs_(core, heldKeys);
}

void FieldPlayerCore_SetRideEffect(FieldPlayerCore *core, FieldEffectTask *tcb) {
    core->terrainEffectTCB = tcb;
}

FieldEffectTask *FieldPlayerCore_GetTerrainEffectTCB(FieldPlayerCore *core) {
    return core->terrainEffectTCB;
}

static void FieldPlayerCore_ChangeObjCode(FieldPlayerCore *core, u16 objCode) {
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    MMSys *mmSys = GetActorMModelSystem(actor);
    u16 currentObjCode = FldAct_GetObjCode(actor);

    if (objCode == OBJ_CODE_HERO || objCode == OBJ_CODE_HEROINE || currentObjCode == OBJ_CODE_HERO ||
        currentObjCode == OBJ_CODE_HEROINE) {
        FldAct_SwapObjCode(actor, objCode);
        FldActSys_ForceFullSync(mmSys);
        return;
    }
    FldAct_UpdateBlInfoForNewObjCode(
        actor, FieldPlayer_GetObjCodeByExState(FieldPlayerCore_GetSex(core), FLD_PLAYER_EXSTATE_NONE));
    FldActSys_ForceFullSync(mmSys);
    FldAct_SwapObjCode(actor, objCode);
    FldActSys_ForceFullSync(mmSys);
}

static void FieldPlayerCore_InitModelCore(FieldPlayerCore *core, u32 exState) {
    u32 sex = FieldPlayerCore_GetSex(core);
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    u16 objCode = FieldPlayer_GetObjCodeByExState(sex, exState);

    if (objCode != FldAct_GetObjCode(actor)) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
    }
    FieldPlayerCore_SetExState(core, exState);
}

static BOOL FieldPlayerCore_IsBaseFormeObjCode(FieldPlayerCore *core) {
    u16 objCode = FldAct_GetObjCode(FieldPlayerCore_GetActor(core));

    if (objCode == OBJ_CODE_HERO || objCode == OBJ_CODE_HEROINE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_CheckDrawProcReady(FieldPlayerCore *core) {
    u16 idx = FldAct_GetBillboardActorIdx(core->actor, 0);

    if (idx != BILLBOARD_ACTOR_NONE) {
        return TRUE;
    }
    return FALSE;
}

static u16 FieldPlayerCore_GetMoveDirByKey_(FieldPlayerCore *core, u16 heldKeys) {
    u32 dirH = GetMoveDirHByKey(heldKeys);
    u32 dirV = GetMoveDirVByKey(heldKeys);
    u16 motionDir;
    u16 prevH;
    u16 prevV;

    if (dirH == DIR_NONE) {
        return dirV;
    }
    if (dirV == DIR_NONE) {
        return dirH;
    }
    motionDir = GetActorMotionDir(FieldPlayerCore_GetActor(core));
    prevH = core->keyDirH;
    prevV = core->keyDirV;
    if (motionDir != DIR_NONE) {
        if (dirH == prevH && dirV == prevV) {
            if (dirV == motionDir) {
                return dirV;
            }
            if (dirH == motionDir) {
                return dirH;
            }
        }
        if (dirV != prevV) {
            return dirV;
        }
        return dirH;
    }
    return dirV;
}

static void FieldPlayerCore_CalcSeparateKeyDirs_(FieldPlayerCore *core, u16 heldKeys) {
    core->keyDirH = GetMoveDirHByKey(heldKeys);
    core->keyDirV = GetMoveDirVByKey(heldKeys);
}

static u32 GetMoveDirHByKey(u32 keys) {
    if (keys & PAD_KEY_LEFT) {
        return DIR_LEFT;
    }
    if (keys & PAD_KEY_RIGHT) {
        return DIR_RIGHT;
    }
    return DIR_NONE;
}

static u32 GetMoveDirVByKey(u32 keys) {
    if (keys & PAD_KEY_UP) {
        return DIR_UP;
    }
    if (keys & PAD_KEY_DOWN) {
        return DIR_DOWN;
    }
    return DIR_NONE;
}

static u16 FieldPlayerCore_RunFormSteps(FieldPlayerCore *core, s32 index) {
    if (core->seqDone == 0) {
        const ExStateChangeFunc *funcs = PLAYER_EXSTATE_CHANGE_VTABLES[index];

        while (funcs[core->seqStep](core) == TRUE) {
        }
    }
    return core->seqDone;
}

static s32 FieldPlayerCore_GetLowestFormSlot(u32 mask) {
    s32 i = SEQ_NONE;

    if (mask != 0) {
        for (i = 0; i < SEQ_NONE; i++) {
            if (mask & 1) {
                break;
            }
            mask >>= 1;
        }
    }
    return i;
}

void FieldPlayerCore_SetSpecialSeq(FieldPlayerCore *core, u32 seq) {
    core->specialSeq = seq;
    core->seqStep = 0;
    core->seqDone = 0;
    core->modelReadyFrames = 0;
    core->objCodeChanges = 0;
}

BOOL FieldPlayerCore_StepFormChange(FieldPlayerCore *core) {
    s32 index = FieldPlayerCore_GetLowestFormSlot(core->specialSeq);

    if (index >= SEQ_NONE) {
        return TRUE;
    }
    if (FieldPlayerCore_RunFormSteps(core, index) == 1) {
        return TRUE;
    }
    return FALSE;
}

void FieldPlayerCore_FinishFormChange(FieldPlayerCore *core) {
    s32 index = FieldPlayerCore_GetLowestFormSlot(core->specialSeq);

    if (index < SEQ_NONE) {
        while (FieldPlayerCore_RunFormSteps(core, index) == 0) {
        }
    }
}

static BOOL FieldPlayerCore_CheckSpecialObjCode(FieldPlayerCore *core, u16 objCode) {
    if (objCode == FldAct_GetObjCode(FieldPlayerCore_GetActor(core))) {
        core->seqDone = 1;
        return TRUE;
    }
    if (FieldPlayerCore_IsBaseFormeObjCode(core) == TRUE) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
        core->seqDone = 1;
        return TRUE;
    }
    FieldPlayerCore_ChangeObjCode(core, FieldPlayer_GetObjCodeByForme(FieldPlayerCore_GetSex(core), 0));
    return FALSE;
}

static BOOL FieldPlayerCore_ChangeToFormSlot(FieldPlayerCore *core, u32 forme) {
    return FieldPlayerCore_CheckSpecialObjCode(core,
                                               FieldPlayer_GetObjCodeByForme(FieldPlayerCore_GetSex(core), forme));
}

static BOOL FieldPlayerCore_FinishFormSlotChange(FieldPlayerCore *core, u32 forme) {
    if (FieldPlayerCore_CheckDrawProcReady(core) == TRUE) {
        core->objCodeChanges++;
        FieldPlayerCore_ChangeObjCode(core, FieldPlayer_GetObjCodeByForme(FieldPlayerCore_GetSex(core), forme));
        core->seqDone = 1;
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_ExStateChangeFunc_NONE(FieldPlayerCore *core) {
    VecFx32 offset = { 0, 0, 0 };
    u32 sex = FieldPlayerCore_GetSex(core);
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    u16 objCode = FieldPlayer_GetObjCodeByForme(sex, 0);
    FieldEffectTask *tcb;

    if (objCode != FldAct_GetObjCode(actor)) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
    }
    FieldPlayerCore_SetExState(core, FLD_PLAYER_EXSTATE_NONE);
    FieldPlayerCore_ReqSyncBGM(core);
    tcb = FieldPlayerCore_GetTerrainEffectTCB(core);
    if (tcb != NULL) {
        FieldEffects_DeleteObject(tcb);
        FieldPlayerCore_SetRideEffect(core, NULL);
    }
    FldAct_SetOuterDrawOffset(actor, &offset);
    core->seqDone = 1;
    return FALSE;
}

static BOOL FieldPlayerCore_ExStateChangeFunc_CYCLING(FieldPlayerCore *core) {
    FieldPlayerCore_GetSex(core);
    FieldPlayerCore_GetActor(core);
    FieldPlayerCore_SetExState(core, FLD_PLAYER_EXSTATE_CYCLING);
    FieldPlayerCore_ReqSyncBGM(core);
    if (FieldPlayerCore_ChangeToFormSlot(core, 1) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepCycleDone(FieldPlayerCore *core) {
    FieldPlayerCore_FinishFormSlotChange(core, 1);
    return FALSE;
}

static BOOL FieldPlayerCore_ExStateChangeFunc_SURF(FieldPlayerCore *core) {
    VecFx32 pos;
    u32 sex = FieldPlayerCore_GetSex(core);
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    u16 objCode = FieldPlayer_GetObjCodeByForme(sex, 2);

    if (objCode != FldAct_GetObjCode(actor)) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
    }
    if (FieldPlayerCore_GetExState(core) != FLD_PLAYER_EXSTATE_SURF) {
        FieldPlayerCore_SetExState(core, FLD_PLAYER_EXSTATE_SURF);
    }
    FieldPlayerCore_ReqSyncBGM(core);
    if (FieldPlayerCore_GetTerrainEffectTCB(core) == NULL) {
        FieldEffects *effects = Field_GetFieldEffects(core->field);
        u16 dir = GetActorFaceDir(actor);

        CopyActorWPos(actor, &pos);
        FieldPlayerCore_SetRideEffect(core,
                                      FldeffSurfPokemon_Start(effects, dir, &pos, actor, SURF_POKEMON_FOLLOW_BOB));
    }
    core->seqDone = 1;
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepRestoreModel(FieldPlayerCore *core) {
    u32 exState = FieldPlayerCore_GetExState(core);

    if (FieldPlayerCore_CheckSpecialObjCode(
            core, FieldPlayer_GetObjCodeByExState(FieldPlayerCore_GetSex(core), exState)) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepRestoreModelDone(FieldPlayerCore *core) {
    if (FieldPlayerCore_CheckDrawProcReady(core) == TRUE) {
        u32 exState;

        core->objCodeChanges++;
        exState = FieldPlayerCore_GetExState(core);
        FieldPlayerCore_ChangeObjCode(core, FieldPlayer_GetObjCodeByExState(FieldPlayerCore_GetSex(core), exState));
        core->seqDone = 1;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot4(FieldPlayerCore *core) {
    if (FieldPlayerCore_ChangeToFormSlot(core, 4) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot4Done(FieldPlayerCore *core) {
    FieldPlayerCore_FinishFormSlotChange(core, 4);
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot5(FieldPlayerCore *core) {
    if (FieldPlayerCore_ChangeToFormSlot(core, 5) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot5Done(FieldPlayerCore *core) {
    FieldPlayerCore_FinishFormSlotChange(core, 5);
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot6(FieldPlayerCore *core) {
    if (FieldPlayerCore_ChangeToFormSlot(core, 6) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot6Done(FieldPlayerCore *core) {
    FieldPlayerCore_FinishFormSlotChange(core, 6);
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot9(FieldPlayerCore *core) {
    if (FieldPlayerCore_ChangeToFormSlot(core, 9) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot9Done(FieldPlayerCore *core) {
    FieldPlayerCore_FinishFormSlotChange(core, 9);
    return FALSE;
}

static BOOL FieldPlayerCore_ExStateChangeFunc_DIVE(FieldPlayerCore *core) {
    FieldEffects *effects = Field_GetFieldEffects(core->field);
    u32 sex = FieldPlayerCore_GetSex(core);
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    u16 objCode = FieldPlayer_GetObjCodeByForme(sex, 3);
    VecFx32 pos;

    if (objCode != FldAct_GetObjCode(actor)) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
    }
    if (FieldPlayerCore_GetExState(core) != FLD_PLAYER_EXSTATE_DIVE) {
        FieldPlayerCore_SetExState(core, FLD_PLAYER_EXSTATE_DIVE);
    }
    if (FieldPlayerCore_GetTerrainEffectTCB(core) == NULL) {
        FieldEffects *fieldEffects = Field_GetFieldEffects(core->field);
        u16 dir = GetActorFaceDir(actor);

        CopyActorWPos(actor, &pos);
        FieldPlayerCore_SetRideEffect(core,
                                      FldeffSurfPokemon_Start(fieldEffects, dir, &pos, actor, SURF_POKEMON_FOLLOW_BOB));
    }
    FldeffBubble_Start(core->actor, effects);
    if (core->terrainEffectTCB != NULL) {
        FldeffSurfPokemon_SetWakeEnabled(core->terrainEffectTCB, FALSE);
    }
    core->seqDone = 1;
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot8(FieldPlayerCore *core) {
    if (FieldPlayerCore_ChangeToFormSlot(core, 8) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL FieldPlayerCore_FormStepSlot8Done(FieldPlayerCore *core) {
    FieldPlayerCore_FinishFormSlotChange(core, 8);
    if (core->seqDone == 1 && FieldPlayerCore_GetExState(core) == FLD_PLAYER_EXSTATE_SURF) {
        BOOL blocked = FALSE;
        FieldActor *actor = FieldPlayerCore_GetActor(core);
        MMSys *mmSys = GetActorMModelSystem(actor);
        s16 x;
        s16 z;

        x = GetGPosX(actor);
        z = GetGPosZ(actor);
        AdjusGridXZByDir(DIR_DOWN, &x, &z, 1);
        if (GetFirstActorOnGPos(mmSys, x, z, TRUE) != NULL) {
            blocked = TRUE;
        }
        RailActorAnim_SetState(actor, blocked);
    }
    return FALSE;
}

static void FieldPlayerCore_ReqSyncBGM(FieldPlayerCore *core) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(core->field));
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u32 zoneId = Field_GetPlayerStateZoneID(core->field);

    FieldSnd_SetBGMOnPlayerExStateChange(fieldSound, gameData, zoneId);
}
