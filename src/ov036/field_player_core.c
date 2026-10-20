#include "types.h"
#include "constants/obj_codes.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_sound.h"
#include "field/player_state.h"
#include "gfl/blact.h"
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
    u16 unk2C;
    u16 unk2E;
    s16 seqStep;
    u16 seqDone;
};

static void FieldPlayerCore_ChangeObjCode(FieldPlayerCore *core, u16 objCode);
static void FieldPlayerCore_InitModelCore(FieldPlayerCore *core, u32 exState);
static BOOL FieldPlayerCore_IsBaseFormeObjCode(FieldPlayerCore *core);
static BOOL func_ov036_0219b1fc(FieldPlayerCore *core);
static u32 GetMoveDirHByKey(u32 keys);
static u32 GetMoveDirVByKey(u32 keys);
static u16 FieldPlayerCore_GetMoveDirByKey_(FieldPlayerCore *core, u16 heldKeys);
static void FieldPlayerCore_CalcSeparateKeyDirs_(FieldPlayerCore *core, u16 heldKeys);
static BOOL FieldPlayerCore_CheckSpecialObjCode(FieldPlayerCore *core, u16 objCode);
static void FieldPlayerCore_ReqSyncBGM(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_NONE(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_CYCLING(FieldPlayerCore *core);
static BOOL func_ov036_0219b4b0(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_SURF(FieldPlayerCore *core);
static BOOL func_ov036_0219b548(FieldPlayerCore *core);
static BOOL func_ov036_0219b578(FieldPlayerCore *core);
static BOOL func_ov036_0219b5b0(FieldPlayerCore *core);
static BOOL func_ov036_0219b5cc(FieldPlayerCore *core);
static BOOL func_ov036_0219b5d8(FieldPlayerCore *core);
static BOOL func_ov036_0219b5f4(FieldPlayerCore *core);
static BOOL func_ov036_0219b600(FieldPlayerCore *core);
static BOOL func_ov036_0219b61c(FieldPlayerCore *core);
static BOOL func_ov036_0219b628(FieldPlayerCore *core);
static BOOL func_ov036_0219b644(FieldPlayerCore *core);
static BOOL FieldPlayerCore_ExStateChangeFunc_DIVE(FieldPlayerCore *core);
static BOOL func_ov036_0219b6f4(FieldPlayerCore *core);
static BOOL func_ov036_0219b710(FieldPlayerCore *core);

// MWCC lays these out in a different order than they are defined; this definition order gives the original layout
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_DIVE[] = { FieldPlayerCore_ExStateChangeFunc_DIVE };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_NONE[] = { FieldPlayerCore_ExStateChangeFunc_NONE };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_SURF[] = { FieldPlayerCore_ExStateChangeFunc_SURF };
static const ExStateChangeFunc data_ov036_021cfc8c[] = { func_ov036_0219b628, func_ov036_0219b644 };
static const ExStateChangeFunc PLAYER_EXSTATE_CHANGE_VTABLE_CYCLING[] = {
    FieldPlayerCore_ExStateChangeFunc_CYCLING,
    func_ov036_0219b4b0,
};
static const ExStateChangeFunc data_ov036_021cfc7c[] = { func_ov036_0219b5b0, func_ov036_0219b5cc };
static const ExStateChangeFunc data_ov036_021cfc74[] = { func_ov036_0219b600, func_ov036_0219b61c };
static const ExStateChangeFunc data_ov036_021cfc84[] = { func_ov036_0219b548, func_ov036_0219b578 };
static const ExStateChangeFunc data_ov036_021cfc9c[] = { func_ov036_0219b6f4, func_ov036_0219b710 };
static const ExStateChangeFunc data_ov036_021cfc6c[] = { func_ov036_0219b5d8, func_ov036_0219b5f4 };

static const ZoneNPC PLAYER_NPC_TEMPLATE = {
    PLAYER_ACTOR_ID, OBJ_CODE_HERO, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff, 0xffff, FALSE,
};

static const ExStateChangeFunc *const PLAYER_EXSTATE_CHANGE_VTABLES[] = {
    PLAYER_EXSTATE_CHANGE_VTABLE_NONE,
    PLAYER_EXSTATE_CHANGE_VTABLE_CYCLING,
    PLAYER_EXSTATE_CHANGE_VTABLE_SURF,
    data_ov036_021cfc84,
    data_ov036_021cfc7c,
    data_ov036_021cfc6c,
    data_ov036_021cfc74,
    data_ov036_021cfc8c,
    PLAYER_EXSTATE_CHANGE_VTABLE_DIVE,
    data_ov036_021cfc9c,
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
    core->observeStatus = 0;
    moveStatus = core->moveStatus;
    if (!IsAllActorAcmdFinished(actor)) {
        switch (moveStatus) {
        case 0:
            return;
        case 1:
            if (prevObserve == 0 || prevObserve == 3) {
                core->observeStatus = 1;
                return;
            }
            core->observeStatus = 2;
            return;
        case 2:
            core->observeStatus = 2;
            return;
        default:
            return;
        }
    }
    if (FldAct_CheckAcmdEnd(actor) == TRUE) {
        switch (moveStatus) {
        case 0:
            return;
        case 1:
            if (prevObserve == 0) {
                return;
            }
            if (prevObserve == 3) {
                core->observeStatus = 0;
                return;
            }
            core->observeStatus = 3;
            return;
        case 2:
            if (prevObserve == 0) {
                return;
            }
            if (prevObserve == 3) {
                core->observeStatus = 0;
                return;
            }
            core->observeStatus = 3;
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

u32 func_ov036_0219afd8(FieldPlayerCore *core) {
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

void func_ov036_0219b014(FieldPlayerCore *core, u32 forme) {
    u32 sex = FieldPlayerCore_GetSex(core);
    FieldActor *actor = FieldPlayerCore_GetActor(core);
    u16 objCode = FieldPlayer_GetObjCodeByForme(sex, forme);

    if (objCode != FldAct_GetObjCode(actor)) {
        FieldPlayerCore_ChangeObjCode(core, objCode);
    }
    core->unk2C = 0;
}

BOOL func_ov036_0219b04c(FieldPlayerCore *core) {
    if (FldAct_GetBlActIdx(FieldPlayerCore_GetActor(core), 0) != BLACT_NONE) {
        if (core->unk2C == 1) {
            return TRUE;
        }
        core->unk2C++;
    }
    return FALSE;
}

u32 FieldPlayerCore_DeriveExState(FieldPlayerCore *core) {
    return FieldPlayer_GetExStateByObjCode(FldAct_GetObjCode(FieldPlayerCore_GetActor(core)));
}

u32 func_ov036_0219b088(FieldPlayerCore *core) {
    if (FieldPlayer_GetExStateByObjCode(FldAct_GetObjCode(FieldPlayerCore_GetActor(core))) == 10) {
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov036_0219b0a4(FieldPlayerCore *core) {
    BlActSys *blActSys = FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(core->actor)));
    u32 idx;
    u16 cmd;

    if (func_ov036_0218ed68(core->actor) == TRUE && func_ov036_0219381c(core->actor) == TRUE) {
        return TRUE;
    }
    idx = FldAct_GetBlActIdx(core->actor, 0);
    if (idx != BLACT_NONE && BlActSys_GetActorLastCmd(blActSys, idx, &cmd) && cmd == BLACT_ANIM_CMD_END) {
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

void func_ov036_0219b138(FieldPlayerCore *core, FieldEffectTask *tcb) {
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

static BOOL func_ov036_0219b1fc(FieldPlayerCore *core) {
    u16 idx = FldAct_GetBlActIdx(core->actor, 0);

    if (idx != BLACT_NONE) {
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

static u16 func_ov036_0219b2d4(FieldPlayerCore *core, s32 index) {
    if (core->seqDone == 0) {
        const ExStateChangeFunc *funcs = PLAYER_EXSTATE_CHANGE_VTABLES[index];

        while (funcs[core->seqStep](core) == TRUE) {
        }
    }
    return core->seqDone;
}

static s32 func_ov036_0219b2fc(u32 mask) {
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
    core->unk2C = 0;
    core->unk2E = 0;
}

BOOL func_ov036_0219b328(FieldPlayerCore *core) {
    s32 index = func_ov036_0219b2fc(core->specialSeq);

    if (index >= SEQ_NONE) {
        return TRUE;
    }
    if (func_ov036_0219b2d4(core, index) == 1) {
        return TRUE;
    }
    return FALSE;
}

void func_ov036_0219b350(FieldPlayerCore *core) {
    s32 index = func_ov036_0219b2fc(core->specialSeq);

    if (index < SEQ_NONE) {
        while (func_ov036_0219b2d4(core, index) == 0) {
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

static BOOL func_ov036_0219b3b8(FieldPlayerCore *core, u32 forme) {
    return FieldPlayerCore_CheckSpecialObjCode(core,
                                               FieldPlayer_GetObjCodeByForme(FieldPlayerCore_GetSex(core), forme));
}

static BOOL func_ov036_0219b3d4(FieldPlayerCore *core, u32 forme) {
    if (func_ov036_0219b1fc(core) == TRUE) {
        core->unk2E++;
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
        func_ov036_021a3a70(tcb);
        func_ov036_0219b138(core, NULL);
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
    if (func_ov036_0219b3b8(core, 1) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b4b0(FieldPlayerCore *core) {
    func_ov036_0219b3d4(core, 1);
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
        func_ov036_0219b138(core, func_ov036_021a4484(effects, dir, &pos, actor, 1));
    }
    core->seqDone = 1;
    return FALSE;
}

static BOOL func_ov036_0219b548(FieldPlayerCore *core) {
    u32 exState = FieldPlayerCore_GetExState(core);

    if (FieldPlayerCore_CheckSpecialObjCode(
            core, FieldPlayer_GetObjCodeByExState(FieldPlayerCore_GetSex(core), exState)) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b578(FieldPlayerCore *core) {
    if (func_ov036_0219b1fc(core) == TRUE) {
        u32 exState;

        core->unk2E++;
        exState = FieldPlayerCore_GetExState(core);
        FieldPlayerCore_ChangeObjCode(core, FieldPlayer_GetObjCodeByExState(FieldPlayerCore_GetSex(core), exState));
        core->seqDone = 1;
    }
    return FALSE;
}

static BOOL func_ov036_0219b5b0(FieldPlayerCore *core) {
    if (func_ov036_0219b3b8(core, 4) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b5cc(FieldPlayerCore *core) {
    func_ov036_0219b3d4(core, 4);
    return FALSE;
}

static BOOL func_ov036_0219b5d8(FieldPlayerCore *core) {
    if (func_ov036_0219b3b8(core, 5) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b5f4(FieldPlayerCore *core) {
    func_ov036_0219b3d4(core, 5);
    return FALSE;
}

static BOOL func_ov036_0219b600(FieldPlayerCore *core) {
    if (func_ov036_0219b3b8(core, 6) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b61c(FieldPlayerCore *core) {
    func_ov036_0219b3d4(core, 6);
    return FALSE;
}

static BOOL func_ov036_0219b628(FieldPlayerCore *core) {
    if (func_ov036_0219b3b8(core, 9) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b644(FieldPlayerCore *core) {
    func_ov036_0219b3d4(core, 9);
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
        func_ov036_0219b138(core, func_ov036_021a4484(fieldEffects, dir, &pos, actor, 1));
    }
    func_ov036_021c6934(core->actor, effects);
    if (core->terrainEffectTCB != NULL) {
        func_ov036_021a4514(core->terrainEffectTCB, FALSE);
    }
    core->seqDone = 1;
    return FALSE;
}

static BOOL func_ov036_0219b6f4(FieldPlayerCore *core) {
    if (func_ov036_0219b3b8(core, 8) != TRUE) {
        core->seqStep++;
    }
    return FALSE;
}

static BOOL func_ov036_0219b710(FieldPlayerCore *core) {
    func_ov036_0219b3d4(core, 8);
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
        func_ov036_0219690c(actor, blocked);
    }
    return FALSE;
}

static void FieldPlayerCore_ReqSyncBGM(FieldPlayerCore *core) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(core->field));
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u32 zoneId = Field_GetPlayerStateZoneID(core->field);

    FieldSnd_SetBGMOnPlayerExStateChange(fieldSound, gameData, zoneId);
}
