// The field actors' action commands (fldmmdl_acmd.c, after the file name that its allocation passes): the task that
// runs a list of them, and the start, update and end functions of each command with the tables of them that
// fldmmdl_movedata.c lists. Names such as FieldAcmdTCB_Create, GetAcmdForDir and the ACMD_ tables are swan's
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). The code was first written by mariano54 on their
// parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "constants/obj_codes.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_effect.h"
#include "field/fldmmdl_movecode_pair.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"

// The state of an action command task, which is also the index of its function in ACMD_TCB_FUNCS
enum {
    ACMD_TCB_STATE_BEGIN,
    ACMD_TCB_STATE_WAIT_ACMD_READY,
    ACMD_TCB_STATE_COMMIT_ACMD,
    ACMD_TCB_STATE_CHECK_SEQ_PERMS,
    ACMD_TCB_STATE_ADVANCE_QUEUE,
    ACMD_TCB_STATE_END,
};

// A command of an action list, as ACMD builds it
typedef struct {
    u16 code;
    u16 count;
} FieldAcmdEntry;

typedef struct {
    u32 state;
    BOOL ended;
    s32 repeat;
    FieldActor *actor;
    const FieldAcmdEntry *cursor;
    const u32 *queue;
} FieldAcmdWork;

// The work of the moves that last some frames
typedef struct {
    u16 state;
    s16 frames;
    u32 dir;
    fx32 speed;
} FieldAcmdWalkWork;

typedef struct {
    fx32 speed;
    fx32 progress;
    u16 heightStep;
    u16 height;
    s8 dir;
    s8 frames;
    u8 state;
    s8 heightTable;
} FieldAcmdJumpWork;

typedef struct {
    s32 pos;
    s32 speed;
} AcmdPosSpeedWork;

typedef struct {
    s32 kind;
    FieldEffectTask *task;
} AcmdEffectWork;

typedef struct {
    s16 dir;
    u16 state;
    s16 steps;
    s16 index;
} AcmdDirStepsWork;

typedef struct {
    s32 frames;
    VecFx32 delta;
} AcmdShiftWork;

typedef struct {
    u16 count;
    u16 limit;
} AcmdWaitWork;

// The work of a move that slides the actor over frames while a model bobs along an axis
typedef struct {
    u8 frames;
    u8 bobAxis;
    u8 axis;
    u8 bobNegate;
    s32 step;
    s32 distance;
    u16 angle;
    u16 angleStep;
} SlideBobWork;

// The work of a move that walks the actor over a few frames in a direction
typedef struct {
    u16 anim;
    s16 frames;
    s32 dir;
    fx32 distance;
} WalkFramesWork;

// The work of a move that lifts the actor
typedef struct {
    fx32 height;
    fx32 speed;
    u16 time;
    u8 lastFrame;
} LiftWork;

// The work of a move that shifts the model while a sound plays
typedef struct {
    u16 time;
    u8 lastFrame;
} ShiftWork;

static void FieldAcmdTCB_Callback(TCB *tcb, void *data);
static BOOL FieldAcmdTCB_Begin(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_WaitAcmdReady(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_CommitAcmd(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_CheckSeqPerms(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_AdvanceQueue(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_End(FieldAcmdWork *work);
static BOOL FldAct_ExecAcmd(FieldActor *actor, u32 acmd, u32 state);
static BOOL func_ov036_0218f09c(FieldActor *actor);
static void func_ov036_0218f0ac(FieldActor *actor, u32 dir);
static BOOL func_ov036_0218f0d0(FieldActor *actor);
static BOOL func_ov036_0218f0dc(FieldActor *actor);
static BOOL func_ov036_0218f0e8(FieldActor *actor);
static BOOL func_ov036_0218f0f4(FieldActor *actor);
static void func_ov036_0218f100(FieldActor *actor, u32 dir, fx32 speed, s16 frames, u16 state);
static BOOL func_ov036_0218f14c(FieldActor *actor);
static BOOL func_ov036_0218f1a0(FieldActor *actor);
static BOOL func_ov036_0218f1b4(FieldActor *actor);
static BOOL func_ov036_0218f1c8(FieldActor *actor);
static BOOL func_ov036_0218f1dc(FieldActor *actor);
static BOOL func_ov036_0218f1f0(FieldActor *actor);
static BOOL func_ov036_0218f204(FieldActor *actor);
static BOOL func_ov036_0218f21c(FieldActor *actor);
static BOOL func_ov036_0218f230(FieldActor *actor);
static BOOL func_ov036_0218f244(FieldActor *actor);
static BOOL func_ov036_0218f25c(FieldActor *actor);
static BOOL func_ov036_0218f274(FieldActor *actor);
static BOOL func_ov036_0218f288(FieldActor *actor);
static BOOL func_ov036_0218f29c(FieldActor *actor);
static BOOL func_ov036_0218f2b0(FieldActor *actor);
static BOOL func_ov036_0218f2c4(FieldActor *actor);
static BOOL func_ov036_0218f2d8(FieldActor *actor);
static BOOL func_ov036_0218f2ec(FieldActor *actor);
static BOOL func_ov036_0218f304(FieldActor *actor);
static BOOL func_ov036_0218f31c(FieldActor *actor);
static BOOL func_ov036_0218f330(FieldActor *actor);
static BOOL func_ov036_0218f348(FieldActor *actor);
static BOOL func_ov036_0218f35c(FieldActor *actor);
static BOOL func_ov036_0218f374(FieldActor *actor);
static BOOL func_ov036_0218f388(FieldActor *actor);
static BOOL func_ov036_0218f3a0(FieldActor *actor);
static BOOL func_ov036_0218f3b8(FieldActor *actor);
static BOOL func_ov036_0218f3d0(FieldActor *actor);
static BOOL func_ov036_0218f3e4(FieldActor *actor);
static BOOL func_ov036_0218f3fc(FieldActor *actor);
static BOOL func_ov036_0218f414(FieldActor *actor);
static BOOL func_ov036_0218f42c(FieldActor *actor);
static BOOL func_ov036_0218f440(FieldActor *actor);
static void func_ov036_0218f458(FieldActor *actor, u32 dir, s16 frames, u16 state);
static BOOL func_ov036_0218f48c(FieldActor *actor);
static BOOL func_ov036_0218f4c0(FieldActor *actor);
static BOOL func_ov036_0218f4d0(FieldActor *actor);
static BOOL func_ov036_0218f4e0(FieldActor *actor);
static BOOL func_ov036_0218f4f0(FieldActor *actor);
static BOOL func_ov036_0218f500(FieldActor *actor);
static BOOL func_ov036_0218f510(FieldActor *actor);
static BOOL func_ov036_0218f520(FieldActor *actor);
static BOOL func_ov036_0218f530(FieldActor *actor);
static BOOL func_ov036_0218f540(FieldActor *actor);
static BOOL func_ov036_0218f550(FieldActor *actor);
static BOOL func_ov036_0218f560(FieldActor *actor);
static BOOL func_ov036_0218f570(FieldActor *actor);
static BOOL func_ov036_0218f580(FieldActor *actor);
static BOOL func_ov036_0218f590(FieldActor *actor);
static BOOL func_ov036_0218f5a0(FieldActor *actor);
static BOOL func_ov036_0218f5b0(FieldActor *actor);
static BOOL func_ov036_0218f5c0(FieldActor *actor);
static BOOL func_ov036_0218f5d0(FieldActor *actor);
static BOOL func_ov036_0218f5e0(FieldActor *actor);
static BOOL func_ov036_0218f5f0(FieldActor *actor);

static void func_ov036_0218f600(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                u16 heightStep, u32 se);

static void func_ov036_0218f66c(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                u16 heightStep);

static BOOL func_ov036_0218f690(FieldActor *actor);
static BOOL func_ov036_0218f76c(FieldActor *actor);
static BOOL func_ov036_0218f790(FieldActor *actor);
static BOOL func_ov036_0218f7b4(FieldActor *actor);
static BOOL func_ov036_0218f7d4(FieldActor *actor);
static BOOL func_ov036_0218f7f8(FieldActor *actor);
static BOOL func_ov036_0218f818(FieldActor *actor);
static BOOL func_ov036_0218f838(FieldActor *actor);
static BOOL func_ov036_0218f858(FieldActor *actor);
static BOOL func_ov036_0218f878(FieldActor *actor);
static BOOL func_ov036_0218f898(FieldActor *actor);
static BOOL func_ov036_0218f8bc(FieldActor *actor);
static BOOL func_ov036_0218f8e0(FieldActor *actor);
static BOOL func_ov036_0218f900(FieldActor *actor);
static BOOL func_ov036_0218f920(FieldActor *actor);
static BOOL func_ov036_0218f944(FieldActor *actor);
static BOOL func_ov036_0218f964(FieldActor *actor);
static BOOL func_ov036_0218f988(FieldActor *actor);
static BOOL func_ov036_0218f9a8(FieldActor *actor);
static BOOL func_ov036_0218f9c8(FieldActor *actor);
static BOOL func_ov036_0218f9ec(FieldActor *actor);
static BOOL func_ov036_0218fa10(FieldActor *actor);
static BOOL func_ov036_0218fa34(FieldActor *actor);
static BOOL func_ov036_0218fa58(FieldActor *actor);
static BOOL func_ov036_0218fa7c(FieldActor *actor);
static void func_ov036_0218faa0(FieldActor *actor, u32 count);
static BOOL func_ov036_0218fab8(FieldActor *actor);
static BOOL func_ov036_0218fad8(FieldActor *actor);
static BOOL func_ov036_0218fae4(FieldActor *actor);
static BOOL func_ov036_0218faf0(FieldActor *actor);
static BOOL func_ov036_0218fafc(FieldActor *actor);
static BOOL func_ov036_0218fb08(FieldActor *actor);
static BOOL func_ov036_0218fb14(FieldActor *actor);
static BOOL func_ov036_0218fb20(FieldActor *actor);
static BOOL func_ov036_0218fb2c(FieldActor *actor);
static BOOL func_ov036_0218fb50(FieldActor *actor);
static BOOL func_ov036_0218fb98(FieldActor *actor);
static BOOL func_ov036_0218fbcc(FieldActor *actor);
static BOOL func_ov036_0218fc14(FieldActor *actor);
static BOOL func_ov036_0218fc28(FieldActor *actor);
static BOOL func_ov036_0218fc3c(FieldActor *actor);
static BOOL func_ov036_0218fc50(FieldActor *actor);
static BOOL func_ov036_0218fc64(FieldActor *actor);
static BOOL func_ov036_0218fc78(FieldActor *actor);
static void func_ov036_0218fc8c(FieldActor *actor, u32 kind, u32 unused, BOOL playSE);
static BOOL func_ov036_0218fcc0(FieldActor *actor);
static BOOL func_ov036_0218fce8(FieldActor *actor);
static BOOL func_ov036_0218fcf8(FieldActor *actor);
static BOOL func_ov036_0218fd08(FieldActor *actor);
static BOOL func_ov036_0218fd18(FieldActor *actor);
static BOOL func_ov036_0218fd28(FieldActor *actor);
static BOOL func_ov036_0218fd38(FieldActor *actor);
static BOOL func_ov036_0218fd48(FieldActor *actor);
static BOOL func_ov036_0218fd58(FieldActor *actor);
static void func_ov036_0218fd68(FieldActor *actor, u32 dir, u32 steps, u32 state);
static BOOL func_ov036_0218fdac(FieldActor *actor, const s32 *distances);
static BOOL func_ov036_0218fe04(FieldActor *actor);
static BOOL func_ov036_0218fe14(FieldActor *actor);
static BOOL func_ov036_0218fe24(FieldActor *actor);
static BOOL func_ov036_0218fe34(FieldActor *actor);
static BOOL func_ov036_0218fe44(FieldActor *actor);
static BOOL func_ov036_0218fe54(FieldActor *actor);
static BOOL func_ov036_0218fe64(FieldActor *actor);
static BOOL func_ov036_0218fe74(FieldActor *actor);
static BOOL func_ov036_0218fe84(FieldActor *actor);
static BOOL func_ov036_0218fe9c(FieldActor *actor);
static BOOL func_ov036_0218feac(FieldActor *actor);
static BOOL func_ov036_0218febc(FieldActor *actor);
static BOOL func_ov036_0218fecc(FieldActor *actor);
static BOOL func_ov036_0218fedc(FieldActor *actor);
static BOOL func_ov036_0218fef4(FieldActor *actor);
static BOOL func_ov036_0218ff04(FieldActor *actor);
static BOOL func_ov036_0218ff14(FieldActor *actor);
static BOOL func_ov036_0218ff24(FieldActor *actor);
static BOOL func_ov036_0218ff34(FieldActor *actor);
static BOOL func_ov036_0218ff4c(FieldActor *actor);
static BOOL func_ov036_0218ff5c(FieldActor *actor);
static BOOL func_ov036_0218ff6c(FieldActor *actor);
static BOOL func_ov036_0218ff7c(FieldActor *actor);
static BOOL func_ov036_0218ff8c(FieldActor *actor);
static BOOL func_ov036_0218ffa4(FieldActor *actor);
static BOOL func_ov036_0218ffc0(FieldActor *actor);
static BOOL func_ov036_0218ffdc(FieldActor *actor);
static BOOL func_ov036_0218fff8(FieldActor *actor);
static BOOL func_ov036_02190010(FieldActor *actor);
static BOOL func_ov036_02190028(FieldActor *actor);
static BOOL func_ov036_02190080(FieldActor *actor);
static BOOL func_ov036_021900c8(FieldActor *actor);
static BOOL func_ov036_02190110(FieldActor *actor);
static void func_ov036_02190194(FieldActor *actor, u32 value, u16 limit);
static BOOL func_ov036_021901b0(FieldActor *actor);
static BOOL func_ov036_021901d0(FieldActor *actor);
static BOOL func_ov036_021901e8(FieldActor *actor);
static BOOL func_ov036_02190200(FieldActor *actor);
static BOOL func_ov036_02190218(FieldActor *actor);
static BOOL func_ov036_02190234(FieldActor *actor);

static void func_ov036_0219028c(FieldActor *actor, const VecFx32 *delta, u32 faceDir, u32 motionDir, u32 value,
                                u8 param);

static BOOL func_ov036_02190350(FieldActor *actor);
static BOOL func_ov036_02190394(FieldActor *actor);
static BOOL func_ov036_021903c8(FieldActor *actor);
static BOOL func_ov036_021903f8(FieldActor *actor);
static BOOL func_ov036_0219042c(FieldActor *actor);
static BOOL func_ov036_0219045c(FieldActor *actor);
static BOOL func_ov036_0219048c(FieldActor *actor);
static BOOL func_ov036_021904c0(FieldActor *actor);
static BOOL func_ov036_021904f4(FieldActor *actor);
static BOOL func_ov036_02190528(FieldActor *actor);
static BOOL func_ov036_0219055c(FieldActor *actor);
static BOOL func_ov036_02190590(FieldActor *actor);
static BOOL func_ov036_021905c0(FieldActor *actor);
static BOOL func_ov036_021905f0(FieldActor *actor);
static BOOL func_ov036_02190620(FieldActor *actor);
static BOOL func_ov036_02190650(FieldActor *actor);
static BOOL func_ov036_02190680(FieldActor *actor);
static BOOL func_ov036_021906b0(FieldActor *actor);
static BOOL func_ov036_021906e4(FieldActor *actor);
static BOOL func_ov036_02190718(FieldActor *actor);
static BOOL func_ov036_02190748(FieldActor *actor);
static BOOL func_ov036_02190778(FieldActor *actor);
static BOOL func_ov036_021907ac(FieldActor *actor);
static BOOL func_ov036_021907e0(FieldActor *actor);
static BOOL func_ov036_02190814(FieldActor *actor);
static BOOL func_ov036_02190848(FieldActor *actor);
static BOOL func_ov036_0219087c(FieldActor *actor);
static BOOL func_ov036_021908b0(FieldActor *actor);
static BOOL func_ov036_021908e4(FieldActor *actor);
static BOOL func_ov036_02190918(FieldActor *actor);
static BOOL func_ov036_0219094c(FieldActor *actor);
static BOOL func_ov036_02190980(FieldActor *actor);
static BOOL func_ov036_021909b4(FieldActor *actor);

static void func_ov036_021909e8(FieldActor *actor, s32 step, u32 faceDir, u32 motionDir, u8 frames, u8 anim, u8 axis,
                                u8 bobAxis, u8 bobNegate);

static BOOL func_ov036_02190ad0(FieldActor *actor);
static BOOL func_ov036_02190c4c(FieldActor *actor);
static BOOL func_ov036_02190c70(FieldActor *actor);
static BOOL func_ov036_02190c98(FieldActor *actor);
static BOOL func_ov036_02190cc0(FieldActor *actor);
static BOOL func_ov036_02190ce8(FieldActor *actor);
static BOOL func_ov036_02190d0c(FieldActor *actor);
static BOOL func_ov036_02190d38(FieldActor *actor);
static BOOL func_ov036_02190d60(FieldActor *actor);
static BOOL func_ov036_02190d88(FieldActor *actor);
static BOOL func_ov036_02190db0(FieldActor *actor);
static BOOL func_ov036_02190ddc(FieldActor *actor);
static BOOL func_ov036_02190e04(FieldActor *actor);
static BOOL func_ov036_02190e28(FieldActor *actor);
static BOOL func_ov036_02190e44(FieldActor *actor);
static BOOL func_ov036_02190e60(FieldActor *actor);
static BOOL func_ov036_02190e74(FieldActor *actor);
static BOOL func_ov036_02190eac(FieldActor *actor);
static BOOL func_ov036_02190eec(FieldActor *actor);
static BOOL func_ov036_02190f14(FieldActor *actor);
static BOOL func_ov036_0219100c(FieldActor *actor);
static BOOL func_ov036_02191020(FieldActor *actor);
static BOOL func_ov036_02191034(FieldActor *actor);
static BOOL func_ov036_02191090(FieldActor *actor);
static BOOL func_ov036_021910b0(FieldActor *actor);
static BOOL func_ov036_021910f4(FieldActor *actor);
static BOOL func_ov036_02191174(FieldActor *actor);
static BOOL func_ov036_02191188(FieldActor *actor);
static BOOL func_ov036_0219119c(FieldActor *actor);
static BOOL func_ov036_021911b0(FieldActor *actor);
static BOOL func_ov036_021911cc(FieldActor *actor);
static BOOL func_ov036_021911e8(FieldActor *actor);
static BOOL func_ov036_0219120c(FieldActor *actor);

static BOOL (*const ACMD_TCB_FUNCS[])(FieldAcmdWork *work) = { FieldAcmdTCB_Begin,        FieldAcmdTCB_WaitAcmdReady,
                                                               FieldAcmdTCB_CommitAcmd,   FieldAcmdTCB_CheckSeqPerms,
                                                               FieldAcmdTCB_AdvanceQueue, FieldAcmdTCB_End };

static const s32 data_ov036_021cb5b0[];
static const s32 data_ov036_021cb5d4[];
static const s32 data_ov036_021cb5ec[];
static const s32 data_ov036_021cb608[];
static const fx32 *data_ov036_021d4d34[];

void func_ov036_0218ed18(MMSys *system) {
    if (FldActSys_GetFieldBlAct(system) != NULL) {
        func_ov036_021937a4(system);
    }
    if (FldActSys_GetActorG3DSystem(system) != NULL) {
        func_ov036_021946f4(system);
    }
    FldActSys_SetDrawInited(system, FALSE);
}

void func_ov036_0218ed44(FieldActor *actor) {
    if (!CheckMMSysFlag(GetActorMModelSystem(actor), 4) && FldAct_IsDrawCreated(actor)) {
        FldAct_InvokeUpdateCallback(actor);
    }
}

BOOL func_ov036_0218ed68(FieldActor *actor) {
    if (IsActorMovementDisabled(actor) && !FldAct_CheckAcmdSet(actor)) {
        return TRUE;
    }
    if (CheckActorFlag(actor, 0x10)) {
        return TRUE;
    }
    return FALSE;
}

void CopyActorPosAllAdd(FieldActor *actor, VecFx32 *dest) {
    VecFx32 wPos;
    VecFx32 pos1;
    VecFx32 pos2;
    VecFx32 pos3;
    VecFx32 offset;

    CopyActorWPos(actor, &wPos);
    CopyActorWPosOffset(actor, &pos1);
    FldAct_GetOuterDrawOffset(actor, &pos2);
    FldAct_GetAttrDrawOffset(actor, &pos3);
    CopyActorPosOffset(actor, &offset);
    dest->x = wPos.x + pos1.x + pos2.x + pos3.x + offset.x;
    dest->y = wPos.y + pos1.y + pos2.y + pos3.y + offset.y;
    dest->z = wPos.z + pos1.z + pos2.z + pos3.z + offset.z;
}

void func_ov036_0218ee08(FieldActor *actor) {
    VecFx32 pos;
    u32 idx = FldAct_GetBlActIdx(actor, 0);

    if (idx != 0xffff) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));

        CopyActorPosAllAdd(actor, &pos);
        pos.y += Field_GetActorYOffset(GetMMSysField(GetActorMModelSystem(actor)));
        BlActScene_SetActorPos(BlActSys_GetScene(sys), idx, &pos);
    }
}

FieldAcmdTCB *FieldAcmdTCB_Create(FieldActor *actor, const u32 *action) {
    MMSys *system = GetActorMModelSystem(actor);
    FieldAcmdWork *work = GFL_HeapAllocate(HEAPID_TAIL(FldActSys_GetFieldHeapID(system)), sizeof(FieldAcmdWork), TRUE,
                                           "fldmmdl_acmd.c", 278);
    u32 priority = FldActSys_GetTaskPriority(GetActorMModelSystem(actor)) - 2;
    TCB *tcb = GFL_TCBMgrAddTask(GetMMSysTCBMgr(system), FieldAcmdTCB_Callback, work, priority);

    work->actor = actor;
    work->cursor = (const FieldAcmdEntry *)action;
    work->queue = action;
    return (FieldAcmdTCB *)tcb;
}

BOOL FieldAcmdTCB_CheckEnded(FieldAcmdTCB *task) {
    FieldAcmdWork *work = GFL_TCBGetData((TCB *)task);

    return work->ended;
}

void FieldAcmdTCB_Remove(FieldAcmdTCB *task) {
    FieldAcmdWork *work = GFL_TCBGetData((TCB *)task);

    FldAct_EndAcmd(work->actor);
    if (work->queue != NULL) {
        GFL_HeapFree((void *)work->queue);
    }
    GFL_HeapFree(work);
    GFL_TCBRemove((TCB *)task);
}

static void FieldAcmdTCB_Callback(TCB *tcb, void *data) {
    FieldAcmdWork *work = data;

    while (ACMD_TCB_FUNCS[work->state](work) == TRUE) {
    }
}

static BOOL FieldAcmdTCB_Begin(FieldAcmdWork *work) {
    work->repeat = 0;
    work->state = ACMD_TCB_STATE_WAIT_ACMD_READY;
    return TRUE;
}

static BOOL FieldAcmdTCB_WaitAcmdReady(FieldAcmdWork *work) {
    if (!IsAllActorAcmdFinished(work->actor)) {
        return FALSE;
    }
    work->state = ACMD_TCB_STATE_COMMIT_ACMD;
    return TRUE;
}

static BOOL FieldAcmdTCB_CommitAcmd(FieldAcmdWork *work) {
    FldAct_SetAcmd(work->actor, work->cursor->code);
    work->state = ACMD_TCB_STATE_CHECK_SEQ_PERMS;
    return FALSE;
}

static BOOL FieldAcmdTCB_CheckSeqPerms(FieldAcmdWork *work) {
    if (!FldAct_CheckAcmdEnd(work->actor)) {
        return FALSE;
    }
    work->state = ACMD_TCB_STATE_ADVANCE_QUEUE;
    return TRUE;
}

static BOOL FieldAcmdTCB_AdvanceQueue(FieldAcmdWork *work) {
    const FieldAcmdEntry *cursor = work->cursor;

    work->repeat++;
    if (work->repeat < cursor->count) {
        work->state = ACMD_TCB_STATE_WAIT_ACMD_READY;
        return TRUE;
    }
    work->cursor = cursor + 1;
    if (cursor[1].code == 0xfd) {
        work->ended = TRUE;
        work->state = ACMD_TCB_STATE_END;
        return FALSE;
    }
    if (cursor[1].code != ACMD_END) {
        work->state = ACMD_TCB_STATE_BEGIN;
        return TRUE;
    }
    work->queue = NULL;
    work->ended = TRUE;
    work->state = ACMD_TCB_STATE_END;
    return FALSE;
}

static BOOL FieldAcmdTCB_End(FieldAcmdWork *work) {
    return FALSE;
}

u16 GetAcmdForDir(u16 dir, u16 acmd) {
    int j;
    const u32 *const *lut = ACMD_DIR_LUT;
    const u32 *table;

    do {
        table = *lut;
        for (j = 0; j < 4; j++) {
            if (acmd == table[j]) {
                return table[dir];
            }
        }
        lut++;
    } while (*lut != NULL);
    return acmd;
}

void func_ov036_0218eff4(FieldActor *actor) {
    u16 acmd;

    do {
        acmd = FldAct_GetAcmd(actor);
        if (acmd == 0xff) {
            break;
        }
    } while (FldAct_ExecAcmd(actor, acmd, FldAct_GetAcmdState(actor)));
}

BOOL func_ov036_0218f01c(FieldActor *actor) {
    if (FldAct_GetAcmd(actor) != 0xff) {
        func_ov036_0218eff4(actor);
        if (!CheckActorMovementFlag(actor, 0x400)) {
            return FALSE;
        }
    }
    ClearActorMovementFlag(actor, 0x400);
    SetNextActorAcmd(actor, 0xff);
    FldAct_SetAcmdState(actor, 0);
    return TRUE;
}

static BOOL FldAct_ExecAcmd(FieldActor *actor, u32 acmd, u32 state) {
    if (CheckActorFlag(actor, 0x2000)) {
        return ACTOR_ACTION_COMMAND_TABLES_RAIL[acmd][state](actor);
    }
    return ACTOR_ACTION_COMMAND_TABLES[acmd][state](actor);
}

static BOOL func_ov036_0218f09c(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x400);
    return FALSE;
}

static void func_ov036_0218f0ac(FieldActor *actor, u32 dir) {
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, 0);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_0218f0d0(FieldActor *actor) {
    func_ov036_0218f0ac(actor, DIR_UP);
    return TRUE;
}

static BOOL func_ov036_0218f0dc(FieldActor *actor) {
    func_ov036_0218f0ac(actor, DIR_DOWN);
    return TRUE;
}

static BOOL func_ov036_0218f0e8(FieldActor *actor) {
    func_ov036_0218f0ac(actor, DIR_LEFT);
    return TRUE;
}

static BOOL func_ov036_0218f0f4(FieldActor *actor) {
    func_ov036_0218f0ac(actor, DIR_RIGHT);
    return TRUE;
}

static void func_ov036_0218f100(FieldActor *actor, u32 dir, fx32 speed, s16 frames, u16 state) {
    FieldAcmdWalkWork *work = func_ov012_0216713c(actor, sizeof(FieldAcmdWalkWork));

    work->state = state;
    work->frames = frames;
    work->dir = dir;
    work->speed = speed;
    FldAct_StepGPosInDir(actor, dir);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    SetActorMovementFlag(actor, 0x20);
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_0218f14c(FieldActor *actor) {
    FieldAcmdWalkWork *work = func_ov012_02167154(actor);

    FldAct_MoveWPosInDir(actor, work->dir, work->speed);
    CheckRecalcActorY(actor);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_0218f1a0(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x800, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f1b4(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x800, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f1c8(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x800, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f1dc(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x800, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f1f0(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x1000, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f204(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x1000, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f21c(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x1000, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f230(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x1000, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f244(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x2000, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f25c(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x2000, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f274(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x2000, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f288(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x2000, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f29c(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x4000, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f2b0(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x4000, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f2c4(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x4000, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f2d8(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x4000, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f2ec(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x8000, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f304(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x8000, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f31c(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x8000, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f330(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x8000, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f348(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x10000, 1, 0);
    return TRUE;
}

static BOOL func_ov036_0218f35c(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x10000, 1, 0);
    return TRUE;
}

static BOOL func_ov036_0218f374(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x10000, 1, 0);
    return TRUE;
}

static BOOL func_ov036_0218f388(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x10000, 1, 0);
    return TRUE;
}

static BOOL func_ov036_0218f3a0(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x4000, 4, 10);
    return TRUE;
}

static BOOL func_ov036_0218f3b8(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x4000, 4, 10);
    return TRUE;
}

static BOOL func_ov036_0218f3d0(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x4000, 4, 10);
    return TRUE;
}

static BOOL func_ov036_0218f3e4(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x4000, 4, 10);
    return TRUE;
}

static BOOL func_ov036_0218f3fc(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_UP, 0x1000, 16, 5);
    return TRUE;
}

static BOOL func_ov036_0218f414(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_DOWN, 0x1000, 16, 5);
    return TRUE;
}

static BOOL func_ov036_0218f42c(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_LEFT, 0x1000, 16, 5);
    return TRUE;
}

static BOOL func_ov036_0218f440(FieldActor *actor) {
    func_ov036_0218f100(actor, DIR_RIGHT, 0x1000, 16, 5);
    return TRUE;
}

static void func_ov036_0218f458(FieldActor *actor, u32 dir, s16 frames, u16 state) {
    FieldAcmdWalkWork *work = func_ov012_0216713c(actor, sizeof(FieldAcmdWalkWork));

    work->state = state;
    work->frames = frames;
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_0218f48c(FieldActor *actor) {
    FieldAcmdWalkWork *work = func_ov012_02167154(actor);

    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x400);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_0218f4c0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_UP, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f4d0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_DOWN, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f4e0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_LEFT, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f4f0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_RIGHT, 32, 1);
    return TRUE;
}

static BOOL func_ov036_0218f500(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_UP, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f510(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_DOWN, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f520(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_LEFT, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f530(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_RIGHT, 16, 2);
    return TRUE;
}

static BOOL func_ov036_0218f540(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f550(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f560(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f570(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_RIGHT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0218f580(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_UP, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f590(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_DOWN, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f5a0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_LEFT, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f5b0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_RIGHT, 4, 4);
    return TRUE;
}

static BOOL func_ov036_0218f5c0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_UP, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f5d0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_DOWN, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f5e0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_LEFT, 2, 5);
    return TRUE;
}

static BOOL func_ov036_0218f5f0(FieldActor *actor) {
    func_ov036_0218f458(actor, DIR_RIGHT, 2, 5);
    return TRUE;
}

static void func_ov036_0218f600(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                u16 heightStep, u32 se) {
    FieldAcmdJumpWork *work = func_ov012_0216713c(actor, sizeof(FieldAcmdJumpWork));

    work->dir = dir;
    work->speed = speed;
    work->frames = frames;
    work->state = state;
    work->heightTable = heightTable;
    work->heightStep = heightStep;
    if (speed == 0) {
        SetActorInitialGPosToNowGPos(actor);
    } else {
        FldAct_StepGPosInDir(actor, dir);
    }
    SetActorMovementFlag(actor, 0xa0);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    FieldActor_NextAcmdState(actor);
    if (se != 0) {
        GFL_SndSEPlay(se);
    }
}

static void func_ov036_0218f66c(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                u16 heightStep) {
    func_ov036_0218f600(actor, dir, speed, frames, state, heightTable, heightStep, SEQ_SE_DANSA);
}

static BOOL func_ov036_0218f690(FieldActor *actor) {
    FieldAcmdJumpWork *work = func_ov012_02167154(actor);
    VecFx32 offset;
    fx32 speed;
    const fx32 *row;
    u16 index;

    if (work->speed != 0) {
        FldAct_MoveWPosInDir(actor, work->dir, work->speed);
        CheckRecalcActorY(actor);
        if (work->progress >= 0x10000) {
            work->progress = 0;
            FldAct_StepGPosInDir(actor, work->dir);
            SetActorMovementFlag(actor, 0x20);
        }
        speed = work->speed;
        if (speed < 0) {
            speed = -speed;
        }
        work->progress += speed;
    }
    work->height += work->heightStep;
    if (work->height > 0xf00) {
        work->height = 0xf00;
    }
    index = work->height / 256;
    row = data_ov036_021d4d34[work->heightTable];
    offset.x = 0;
    offset.y = row[index];
    offset.z = 0;
    SetActorWPosOffset(actor, &offset);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    VecFx32 zero = { 0, 0, 0 };
    SetActorWPosOffset(actor, &zero);
    SetActorMovementFlag(actor, 0x540);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    GFL_SndSEPlay(SEQ_SE_FLD_10);
    return TRUE;
}

static BOOL func_ov036_0218f76c(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_UP, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f790(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_DOWN, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f7b4(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_LEFT, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f7d4(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_RIGHT, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f7f8(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_UP, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f818(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_DOWN, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f838(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_LEFT, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f858(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_RIGHT, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f878(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_UP, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f898(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_DOWN, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f8bc(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_LEFT, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f8e0(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_RIGHT, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_0218f900(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_UP, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f920(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_DOWN, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f944(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_LEFT, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f964(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_RIGHT, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_0218f988(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_LEFT, 0x1000, 16, 2, 0, 0xf0);
    return TRUE;
}

static BOOL func_ov036_0218f9a8(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_RIGHT, 0x1000, 16, 2, 0, 0xf0);
    return TRUE;
}

static BOOL func_ov036_0218f9c8(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_LEFT, 0x4000, 12, 3, 0, 0x140);
    return TRUE;
}

static BOOL func_ov036_0218f9ec(FieldActor *actor) {
    func_ov036_0218f66c(actor, DIR_RIGHT, 0x4000, 12, 3, 0, 0x140);
    return TRUE;
}

static BOOL func_ov036_0218fa10(FieldActor *actor) {
    func_ov036_0218f600(actor, DIR_UP, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static BOOL func_ov036_0218fa34(FieldActor *actor) {
    func_ov036_0218f600(actor, DIR_DOWN, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static BOOL func_ov036_0218fa58(FieldActor *actor) {
    func_ov036_0218f600(actor, DIR_LEFT, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static BOOL func_ov036_0218fa7c(FieldActor *actor) {
    func_ov036_0218f600(actor, DIR_RIGHT, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static void func_ov036_0218faa0(FieldActor *actor, u32 count) {
    u32 *work = func_ov012_0216713c(actor, 4);

    *work = count;
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_0218fab8(FieldActor *actor) {
    u32 *work = func_ov012_02167154(actor);

    if (*work != 0) {
        (*work)--;
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fad8(FieldActor *actor) {
    func_ov036_0218faa0(actor, 1);
    return TRUE;
}

static BOOL func_ov036_0218fae4(FieldActor *actor) {
    func_ov036_0218faa0(actor, 2);
    return TRUE;
}

static BOOL func_ov036_0218faf0(FieldActor *actor) {
    func_ov036_0218faa0(actor, 4);
    return TRUE;
}

static BOOL func_ov036_0218fafc(FieldActor *actor) {
    func_ov036_0218faa0(actor, 8);
    return TRUE;
}

static BOOL func_ov036_0218fb08(FieldActor *actor) {
    func_ov036_0218faa0(actor, 15);
    return TRUE;
}

static BOOL func_ov036_0218fb14(FieldActor *actor) {
    func_ov036_0218faa0(actor, 16);
    return TRUE;
}

static BOOL func_ov036_0218fb20(FieldActor *actor) {
    func_ov036_0218faa0(actor, 32);
    return TRUE;
}

static BOOL func_ov036_0218fb2c(FieldActor *actor) {
    AcmdPosSpeedWork *work = func_ov012_0216713c(actor, 8);

    work->speed = 8 << 13;
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fb50(FieldActor *actor) {
    AcmdPosSpeedWork *work = func_ov012_02167154(actor);

    work->pos += work->speed;
    VecFx32 offset = { 0, 0, 0 };

    offset.y = work->pos;
    SetActorWPosOffset(actor, &offset);
    if (work->pos / 0x8000 < 40) {
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fb98(FieldActor *actor) {
    AcmdPosSpeedWork *work = func_ov012_0216713c(actor, 8);

    work->pos = 5 << 18;
    work->speed = -0x10000;
    GFL_SndSEPlay(SEQ_SE_FLD_05);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fbcc(FieldActor *actor) {
    AcmdPosSpeedWork *work = func_ov012_02167154(actor);

    work->pos += work->speed;
    if (work->pos < 0) {
        work->pos = 0;
    }
    VecFx32 offset = { 0, 0, 0 };

    offset.y = work->pos;
    SetActorWPosOffset(actor, &offset);
    if (work->pos > 0) {
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fc14(FieldActor *actor) {
    SetActorFlag(actor, 4);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fc28(FieldActor *actor) {
    ClearActorFlag(actor, 4);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fc3c(FieldActor *actor) {
    SetActorFlag(actor, 8);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fc50(FieldActor *actor) {
    ClearActorFlag(actor, 8);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fc64(FieldActor *actor) {
    SetActorFlag(actor, 16);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fc78(FieldActor *actor) {
    ClearActorFlag(actor, 16);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static void func_ov036_0218fc8c(FieldActor *actor, u32 kind, u32 unused, BOOL playSE) {
    AcmdEffectWork *work = func_ov012_0216713c(actor, 8);

    work->kind = kind;
    work->task =
        func_ov036_021b3f14(Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor))), actor, kind, playSE);
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_0218fcc0(FieldActor *actor) {
    AcmdEffectWork *work = func_ov012_02167154(actor);

    if (func_ov036_021b3fb4(work->task) == TRUE) {
        func_ov036_021a3a70(work->task);
        FieldActor_NextAcmdState(actor);
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218fce8(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 0, 0, TRUE);
    return FALSE;
}

static BOOL func_ov036_0218fcf8(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 0, 0, FALSE);
    return FALSE;
}

static BOOL func_ov036_0218fd08(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 2, 0, FALSE);
    return FALSE;
}

static BOOL func_ov036_0218fd18(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 0, 0, TRUE);
    return FALSE;
}

static BOOL func_ov036_0218fd28(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 0, 0, TRUE);
    return FALSE;
}

static BOOL func_ov036_0218fd38(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 1, 0, TRUE);
    return FALSE;
}

static BOOL func_ov036_0218fd48(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 2, 0, TRUE);
    return FALSE;
}

static BOOL func_ov036_0218fd58(FieldActor *actor) {
    func_ov036_0218fc8c(actor, 3, 0, TRUE);
    return FALSE;
}

static void func_ov036_0218fd68(FieldActor *actor, u32 dir, u32 steps, u32 state) {
    AcmdDirStepsWork *work = func_ov012_0216713c(actor, sizeof(AcmdDirStepsWork));

    work->dir = dir;
    work->state = state;
    work->steps = steps;
    FldAct_StepGPosInDir(actor, dir);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    SetActorMovementFlag(actor, 0x20);
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_0218fdac(FieldActor *actor, const s32 *distances) {
    AcmdDirStepsWork *work = func_ov012_02167154(actor);

    FldAct_MoveWPosInDir(actor, work->dir, distances[work->index]);
    CheckRecalcActorY(actor);
    work->index++;
    if (work->index < work->steps) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0218fe04(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_UP, 6, 6);
    return TRUE;
}

static BOOL func_ov036_0218fe14(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_DOWN, 6, 6);
    return TRUE;
}

static BOOL func_ov036_0218fe24(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_LEFT, 6, 6);
    return TRUE;
}

static BOOL func_ov036_0218fe34(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_RIGHT, 6, 6);
    return TRUE;
}

static BOOL func_ov036_0218fe44(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_UP, 6, 11);
    return TRUE;
}

static BOOL func_ov036_0218fe54(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_DOWN, 6, 11);
    return TRUE;
}

static BOOL func_ov036_0218fe64(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_LEFT, 6, 11);
    return TRUE;
}

static BOOL func_ov036_0218fe74(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_RIGHT, 6, 11);
    return TRUE;
}

static BOOL func_ov036_0218fe84(FieldActor *actor) {
    if (func_ov036_0218fdac(actor, data_ov036_021cb5d4) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218fe9c(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_UP, 3, 7);
    return TRUE;
}

static BOOL func_ov036_0218feac(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_DOWN, 3, 7);
    return TRUE;
}

static BOOL func_ov036_0218febc(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_LEFT, 3, 7);
    return TRUE;
}

static BOOL func_ov036_0218fecc(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_RIGHT, 3, 7);
    return TRUE;
}

static BOOL func_ov036_0218fedc(FieldActor *actor) {
    if (func_ov036_0218fdac(actor, data_ov036_021cb5b0) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218fef4(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_UP, 7, 8);
    return TRUE;
}

static BOOL func_ov036_0218ff04(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_DOWN, 7, 8);
    return TRUE;
}

static BOOL func_ov036_0218ff14(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_LEFT, 7, 8);
    return TRUE;
}

static BOOL func_ov036_0218ff24(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_RIGHT, 7, 8);
    return TRUE;
}

static BOOL func_ov036_0218ff34(FieldActor *actor) {
    if (func_ov036_0218fdac(actor, data_ov036_021cb5ec) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218ff4c(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_UP, 12, 9);
    return TRUE;
}

static BOOL func_ov036_0218ff5c(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_DOWN, 12, 9);
    return TRUE;
}

static BOOL func_ov036_0218ff6c(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_LEFT, 12, 9);
    return TRUE;
}

static BOOL func_ov036_0218ff7c(FieldActor *actor) {
    func_ov036_0218fd68(actor, DIR_RIGHT, 12, 9);
    return TRUE;
}

static BOOL func_ov036_0218ff8c(FieldActor *actor) {
    if (func_ov036_0218fdac(actor, data_ov036_021cb608) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218ffa4(FieldActor *actor) {
    func_ov012_0216713c(actor, 4);
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_0218ffc0(FieldActor *actor) {
    func_ov012_0216713c(actor, 4);
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_0218ffdc(FieldActor *actor) {
    func_ov012_0216713c(actor, 4);
    FldAct_SetDrawStatus(actor, 12);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_0218fff8(FieldActor *actor) {
    GFL_SndSEPlay(SEQ_SE_FLD_152);
    return func_ov036_0218ffa4(actor);
}

static BOOL func_ov036_02190010(FieldActor *actor) {
    GFL_SndSEPlay(SEQ_SE_FLD_152);
    return func_ov036_0218ffc0(actor);
}

static BOOL func_ov036_02190028(FieldActor *actor) {
    u16 cmd;
    u32 index;

    func_ov012_02167154(actor);
    index = FldAct_GetBlActIdx(actor, 0);
    if (index != 0xffff) {
        if (BlActSys_GetActorLastCmd(FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor))),
                                     (u16)index, &cmd) == TRUE) {
            FldAct_SetDrawStatus(actor, 0);
            FieldActor_NextAcmdState(actor);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_02190080(FieldActor *actor) {
    u16 cmd;
    u32 index;

    func_ov012_02167154(actor);
    index = FldAct_GetBlActIdx(actor, 0);
    if (index != 0xffff) {
        if (BlActSys_GetActorLastCmd(FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor))),
                                     (u16)index, &cmd) == TRUE) {
            FieldActor_NextAcmdState(actor);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_021900c8(FieldActor *actor) {
    FieldEffectTask *task;

    func_ov012_0216713c(actor, 4);
    task = func_ov036_0219311c(actor);
    if (task != NULL) {
        func_ov036_021c6800(task);
    }
    VecFx32 offset = { 0, 0, 0 };

    SetActorWPosOffset(actor, &offset);
    SetActorMovementFlag(actor, 0xa0);
    ClearActorMovementFlag(actor, 0x8000);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_02190110(FieldActor *actor) {
    u32 *work = func_ov012_02167154(actor);
    VecFx32 offset = { 0, 0, 0 };
    FieldEffectTask *task;

    offset.y = data_ov036_021d4d34[0][*work];
    SetActorWPosOffset(actor, &offset);
    task = func_ov036_0219311c(actor);
    if (task != NULL) {
        if (func_ov036_021c6820(task) == TRUE) {
            func_ov036_021a3a70(task);
            func_ov036_02193110(actor, 0);
        }
    }
    *work += 2;
    if (*work < 16) {
        return FALSE;
    }
    offset.y = 0;
    SetActorWPosOffset(actor, &offset);
    SetActorMovementFlag(actor, 0x540);
    func_ov036_02193128(actor);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static void func_ov036_02190194(FieldActor *actor, u32 value, u16 limit) {
    AcmdWaitWork *work = func_ov012_0216713c(actor, 4);

    work->limit = limit;
    FldAct_SetDrawStatus(actor, value);
}

static BOOL func_ov036_021901b0(FieldActor *actor) {
    AcmdWaitWork *work = func_ov012_02167154(actor);

    work->count++;
    if (work->count >= work->limit) {
        FieldActor_NextAcmdState(actor);
    }
    return FALSE;
}

static BOOL func_ov036_021901d0(FieldActor *actor) {
    func_ov036_02190194(actor, 1, 40);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_021901e8(FieldActor *actor) {
    func_ov036_02190194(actor, 2, 41);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_02190200(FieldActor *actor) {
    func_ov036_02190194(actor, 0, 8);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_02190218(FieldActor *actor) {
    func_ov012_0216713c(actor, 4);
    FldAct_SetDrawStatus(actor, 1);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190234(FieldActor *actor) {
    VecFx32 offset = { 0, 0, 0 };
    s32 heights[10] = { 0, 0, 0, 0, 0, 0x4000, 0x4000, 0x3000, 0, 0 };
    s32 *work;
    s32 index;

    work = func_ov012_02167154(actor);
    index = *work;
    if (index >= 10) {
        index = 9;
    }
    offset.y = heights[index];
    SetActorWPosOffset(actor, &offset);
    (*work)++;
    if (*work >= 9) {
        FieldActor_NextAcmdState(actor);
    }
    return FALSE;
}

static void func_ov036_0219028c(FieldActor *actor, const VecFx32 *delta, u32 faceDir, u32 motionDir, u32 value,
                                u8 param) {
    AcmdShiftWork *work = func_ov012_0216713c(actor, 16);

    work->frames = value;
    work->delta = *delta;
    CheckSetActorFaceDir(actor, faceDir);
    SetActorMotionDir(actor, motionDir);
    FldAct_SetDrawStatus(actor, param);
    FldAct_SetMoveStart(actor);
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    if (delta->x < 0) {
        adjustXPos(actor, -1);
    } else if (delta->x > 0) {
        adjustXPos(actor, 1);
    }
    if (delta->y < 0) {
        adjustZPos(actor, -1);
    } else if (delta->y > 0) {
        adjustZPos(actor, 1);
    }
    if (delta->z < 0) {
        adjustYPos(actor, -1);
    } else if (delta->z > 0) {
        adjustYPos(actor, 1);
    }
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_02190350(FieldActor *actor) {
    AcmdShiftWork *work = func_ov012_02167154(actor);

    FldAct_AddWPos(actor, &work->delta);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190394(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(2), 0 };

    func_ov036_0219028c(actor, &delta, DIR_LEFT, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021903c8(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(-2), 0 };

    func_ov036_0219028c(actor, &delta, DIR_RIGHT, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021903f8(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(2) };

    func_ov036_0219028c(actor, &delta, DIR_DOWN, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0219042c(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-2) };

    func_ov036_0219028c(actor, &delta, DIR_UP, DIR_RIGHT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0219045c(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(2), 0 };

    func_ov036_0219028c(actor, &delta, DIR_RIGHT, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0219048c(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(-2), 0 };

    func_ov036_0219028c(actor, &delta, DIR_LEFT, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021904c0(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-2) };

    func_ov036_0219028c(actor, &delta, DIR_UP, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021904f4(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(2) };

    func_ov036_0219028c(actor, &delta, DIR_DOWN, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_02190528(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(2) };

    func_ov036_0219028c(actor, &delta, DIR_DOWN, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL func_ov036_0219055c(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-2) };

    func_ov036_0219028c(actor, &delta, DIR_UP, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL func_ov036_02190590(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(-2), 0, 0 };

    func_ov036_0219028c(actor, &delta, DIR_RIGHT, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021905c0(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(2), 0, 0 };

    func_ov036_0219028c(actor, &delta, DIR_LEFT, DIR_RIGHT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021905f0(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(4) };

    func_ov036_0219028c(actor, &delta, DIR_DOWN, DIR_UP, 4, 4);
    return TRUE;
}

static BOOL func_ov036_02190620(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-4) };

    func_ov036_0219028c(actor, &delta, DIR_UP, DIR_DOWN, 4, 4);
    return TRUE;
}

static BOOL func_ov036_02190650(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(-4), 0, 0 };

    func_ov036_0219028c(actor, &delta, DIR_RIGHT, DIR_LEFT, 4, 4);
    return TRUE;
}

static BOOL func_ov036_02190680(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(4), 0, 0 };

    func_ov036_0219028c(actor, &delta, DIR_LEFT, DIR_RIGHT, 4, 4);
    return TRUE;
}

static BOOL func_ov036_021906b0(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(8) };

    func_ov036_0219028c(actor, &delta, DIR_DOWN, DIR_UP, 2, 5);
    return TRUE;
}

static BOOL func_ov036_021906e4(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-8) };

    func_ov036_0219028c(actor, &delta, DIR_UP, DIR_DOWN, 2, 5);
    return TRUE;
}

static BOOL func_ov036_02190718(FieldActor *actor) {
    VecFx32 step = { -0x8000, 0, 0 };
    func_ov036_0219028c(actor, &step, DIR_RIGHT, DIR_LEFT, 2, 5);
    return TRUE;
}

static BOOL func_ov036_02190748(FieldActor *actor) {
    VecFx32 step = { 0x8000, 0, 0 };
    func_ov036_0219028c(actor, &step, DIR_LEFT, DIR_RIGHT, 2, 5);
    return TRUE;
}

static BOOL func_ov036_02190778(FieldActor *actor) {
    VecFx32 step = { 0, 0x4000, 0 };
    func_ov036_0219028c(actor, &step, DIR_LEFT, DIR_UP, 4, 10);
    return TRUE;
}

static BOOL func_ov036_021907ac(FieldActor *actor) {
    VecFx32 step = { 0, -0x4000, 0 };
    func_ov036_0219028c(actor, &step, DIR_RIGHT, DIR_DOWN, 4, 10);
    return TRUE;
}

static BOOL func_ov036_021907e0(FieldActor *actor) {
    VecFx32 step = { 0, 0, 0x4000 };
    func_ov036_0219028c(actor, &step, DIR_DOWN, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL func_ov036_02190814(FieldActor *actor) {
    VecFx32 step = { 0, 0, -0x4000 };
    func_ov036_0219028c(actor, &step, DIR_UP, DIR_RIGHT, 4, 10);
    return TRUE;
}

static BOOL func_ov036_02190848(FieldActor *actor) {
    VecFx32 step = { 0, 0x4000, 0 };
    func_ov036_0219028c(actor, &step, DIR_RIGHT, DIR_UP, 4, 10);
    return TRUE;
}

static BOOL func_ov036_0219087c(FieldActor *actor) {
    VecFx32 step = { 0, -0x4000, 0 };
    func_ov036_0219028c(actor, &step, DIR_LEFT, DIR_DOWN, 4, 10);
    return TRUE;
}

static BOOL func_ov036_021908b0(FieldActor *actor) {
    VecFx32 step = { 0, 0, -0x4000 };
    func_ov036_0219028c(actor, &step, DIR_UP, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL func_ov036_021908e4(FieldActor *actor) {
    VecFx32 step = { 0, 0, 0x4000 };
    func_ov036_0219028c(actor, &step, DIR_DOWN, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL func_ov036_02190918(FieldActor *actor) {
    VecFx32 step = { 0, 0, 0x4000 };
    func_ov036_0219028c(actor, &step, DIR_DOWN, DIR_UP, 4, 10);
    return TRUE;
}

static BOOL func_ov036_0219094c(FieldActor *actor) {
    VecFx32 step = { 0, 0, -0x4000 };
    func_ov036_0219028c(actor, &step, DIR_UP, DIR_DOWN, 4, 10);
    return TRUE;
}

static BOOL func_ov036_02190980(FieldActor *actor) {
    VecFx32 step = { -0x4000, 0, 0 };
    func_ov036_0219028c(actor, &step, DIR_RIGHT, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL func_ov036_021909b4(FieldActor *actor) {
    VecFx32 step = { 0x4000, 0, 0 };
    func_ov036_0219028c(actor, &step, DIR_LEFT, DIR_RIGHT, 4, 10);
    return TRUE;
}

static void func_ov036_021909e8(FieldActor *actor, s32 step, u32 faceDir, u32 motionDir, u8 frames, u8 anim, u8 axis,
                                u8 bobAxis, u8 bobNegate) {
    s32 sign = 1;
    SlideBobWork *work = func_ov012_0216713c(actor, sizeof(SlideBobWork));
    work->frames = frames;
    work->step = step;
    work->axis = axis;
    work->bobAxis = bobAxis;
    work->bobNegate = bobNegate;
    work->angleStep = 0x1000 / work->frames;
    CheckSetActorFaceDir(actor, faceDir);
    SetActorMotionDir(actor, motionDir);
    FldAct_SetDrawStatus(actor, anim);
    FldAct_SetMoveStart(actor);
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    if (step != 0) {
        switch (axis) {
        case 0:
            if (step < 0) {
                sign = -1;
            }
            adjustXPos(actor, sign);
            break;
        case 1:
            if (step < 0) {
                sign = -1;
            }
            adjustZPos(actor, sign * 2);
            break;
        case 2:
            if (step < 0) {
                sign = -1;
            }
            adjustYPos(actor, sign);
            break;
        }
    }
    FieldActor_NextAcmdState(actor);
}

static BOOL func_ov036_02190ad0(FieldActor *actor) {
    SlideBobWork *work = func_ov012_02167154(actor);
    VecFx32 position;
    s32 bobValue;
    CopyActorWPos(actor, &position);
    switch (work->axis) {
    case 0:
        position.x += work->step;
        break;
    case 1:
        position.y += work->step;
        break;
    case 2:
        position.z += work->step;
        break;
    }
    SetActorWPosValue(actor, &position);
    work->distance += work->step < 0 ? -work->step : work->step;
    work->angle += work->angleStep;
    if (work->angle > 0xf00) {
        work->angle = 0xf00;
    }
    VecFx32 bob = { 0, 0, 0 };
    u16 index = work->angle / 256;
    bobValue = data_ov036_021d4d34[1][index];
    if (work->bobNegate == 1) {
        bobValue = -bobValue;
    }
    switch (work->bobAxis) {
    case 0:
        bob.x = bobValue;
        break;
    case 1:
        bob.y = bobValue;
        break;
    case 2:
        bob.z = bobValue;
        break;
    }
    SetActorWPosOffset(actor, &bob);
    work->frames--;
    if (work->distance >= 0x10000 && work->frames != 0) {
        s32 step;
        s32 sign;
        work->distance -= 0x10000;
        sign = 1;
        step = work->step;
        SetActorInitialGPosX(actor, GetGPosX(actor));
        SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
        SetActorInitialGPosZ(actor, GetGPosZ(actor));
        switch (work->axis) {
        case 0:
            if (step < 0) {
                sign -= 2;
            }
            adjustXPos(actor, sign);
            break;
        case 1:
            if (step < 0) {
                sign -= 2;
            }
            adjustZPos(actor, sign * 2);
            break;
        case 2:
            if (step < 0) {
                sign -= 2;
            }
            adjustYPos(actor, sign);
            break;
        }
    }
    if (work->frames != 0) {
        return FALSE;
    }
    VecFx32 zero = { 0, 0, 0 };
    SetActorWPosOffset(actor, &zero);
    SetActorMovementFlag(actor, 0x540);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190c4c(FieldActor *actor) {
    func_ov036_021909e8(actor, 0x2000, DIR_LEFT, DIR_UP, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190c70(FieldActor *actor) {
    func_ov036_021909e8(actor, -0x2000, DIR_RIGHT, DIR_DOWN, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190c98(FieldActor *actor) {
    func_ov036_021909e8(actor, 0x2000, DIR_DOWN, DIR_LEFT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190cc0(FieldActor *actor) {
    func_ov036_021909e8(actor, -0x2000, DIR_UP, DIR_RIGHT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190ce8(FieldActor *actor) {
    func_ov036_021909e8(actor, 0x2000, DIR_RIGHT, DIR_UP, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190d0c(FieldActor *actor) {
    func_ov036_021909e8(actor, -0x2000, DIR_LEFT, DIR_DOWN, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190d38(FieldActor *actor) {
    func_ov036_021909e8(actor, -0x2000, DIR_UP, DIR_LEFT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190d60(FieldActor *actor) {
    func_ov036_021909e8(actor, 0x2000, DIR_DOWN, DIR_LEFT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190d88(FieldActor *actor) {
    func_ov036_021909e8(actor, 0x2000, DIR_DOWN, DIR_UP, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190db0(FieldActor *actor) {
    func_ov036_021909e8(actor, -0x2000, DIR_UP, DIR_DOWN, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190ddc(FieldActor *actor) {
    func_ov036_021909e8(actor, -0x2000, DIR_RIGHT, DIR_LEFT, 8, 3, 0, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190e04(FieldActor *actor) {
    func_ov036_021909e8(actor, 0x2000, DIR_LEFT, DIR_RIGHT, 8, 3, 0, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190e28(FieldActor *actor) {
    func_ov012_0216713c(actor, 4);
    FldAct_SetDrawStatus(actor, 1);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_02190e44(FieldActor *actor) {
    s32 *count = func_ov012_02167154(actor);
    (*count)++;
    if (*count >= 22) {
        FieldActor_NextAcmdState(actor);
    }
    return FALSE;
}

static BOOL func_ov036_02190e60(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190e74(FieldActor *actor) {
    WalkFramesWork *work = func_ov012_0216713c(actor, sizeof(WalkFramesWork));
    work->anim = 12;
    work->frames = 2;
    work->dir = 2;
    work->distance = 0x8000;
    FldAct_StepGPosInDir(actor, work->dir);
    FldAct_SetDrawStatus(actor, work->anim);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190eac(FieldActor *actor) {
    WalkFramesWork *work = func_ov012_02167154(actor);
    FldAct_MoveWPosInDir(actor, work->dir, work->distance);
    CheckRecalcActorY(actor);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190eec(FieldActor *actor) {
    LiftWork *work = func_ov012_0216713c(actor, 0x10);
    work->speed = 0x2000;
    work->time = 0;
    work->lastFrame = 0;
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02190f14(FieldActor *actor) {
    LiftWork *work = func_ov012_02167154(actor);
    u16 anim;
    u16 step;
    if (func_ov036_02197060(actor, &anim, &step) != TRUE) {
        return FALSE;
    }
    work->time++;
    step--;
    if (FldAct_GetObjCode(actor) == OBJ_CODE_211) {
        if (anim == 2 && work->lastFrame != step) {
            work->lastFrame = step;
            switch (step) {
            case 7:
            case 12:
            case 17:
                GFL_SndSEPlay(SEQ_SE_FLD_167);
                break;
            case 10:
            case 15:
            case 20:
                GFL_SndSEPlay(SEQ_SE_FLD_168);
                break;
            }
        }
    } else if (work->time == 0x5f) {
        GFL_SndSEPlay(SEQ_SE_FLD_169);
    }
    if (work->time < 0x5f) {
        return FALSE;
    }
    work->height += work->speed;
    VecFx32 offset = { 0, 0, 0 };
    offset.y = work->height;
    SetActorWPosOffset(actor, &offset);
    if (work->height / 0x8000 < 16) {
        return FALSE;
    }
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0219100c(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02191020(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_02191034(FieldActor *actor) {
    BlActSys *blAct = FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor)));
    u32 blActIdx;
    u16 cmd;
    blActIdx = FldAct_GetBlActIdx(actor, 0);
    if (blActIdx != 0xffff && !BlActSys_GetActorLastCmd(blAct, (u16)blActIdx, &cmd)) {
        return FALSE;
    }
    CheckSetActorFaceDir(actor, DIR_DOWN);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02191090(FieldActor *actor) {
    VecFx32 offset = { 0, 0, 0 };
    SetActorWPosOffset(actor, &offset);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_021910b0(FieldActor *actor) {
    VecFx32 offset = { 0, 0x8000, 0 };
    ShiftWork *work = func_ov012_0216713c(actor, 6);
    work->time = 0;
    work->lastFrame = 0;
    SetActorWPosOffset(actor, &offset);
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_021910f4(FieldActor *actor) {
    ShiftWork *work = func_ov012_02167154(actor);
    u16 anim;
    u16 step;
    if (func_ov036_021970b8(actor, &anim, &step) != TRUE) {
        return FALSE;
    }
    work->time++;
    step--;
    if (FldAct_GetObjCode(actor) == OBJ_CODE_221 && anim == 0 && work->lastFrame != step) {
        work->lastFrame = step;
        switch (step) {
        case 0:
            GFL_SndSEPlay(SEQ_SE_FLD_167);
            break;
        case 3:
            GFL_SndSEPlay(SEQ_SE_FLD_168);
            break;
        }
    }
    if (work->time < 0x5f) {
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02191174(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02191188(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_0219119c(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_021911b0(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 12);
    func_ov036_02197288(actor);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_021911cc(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 13);
    func_ov036_02197288(actor);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL func_ov036_021911e8(FieldActor *actor) {
    if (func_ov036_02197238(actor) == TRUE) {
        FldAct_SetDrawStatus(actor, 0);
        FieldActor_NextAcmdState(actor);
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0219120c(FieldActor *actor) {
    if (func_ov036_02197238(actor) == TRUE) {
        FieldActor_NextAcmdState(actor);
        return TRUE;
    }
    return FALSE;
}

// The command tables, after the functions. MWCC lays a file's data out sorted by size, and equal sizes come out in an
// order set by where every object is declared, so these are in the order the game's layout needs: that of their start
// functions (then update functions, for a shared start), except commands 88-91 (their index in
// ACTOR_ACTION_COMMAND_TABLES), which follow command 83's table, 117-120, which follow 152's, and 153-158, which follow
// 144's. The tables the functions read come last, declared at the top.
const ActorAcmdFunc ACMD_DIR_TABLE[] = { func_ov036_0218f0d0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad0c[] = { func_ov036_0218f0dc, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad14[] = { func_ov036_0218f0e8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caccc[] = { func_ov036_0218f0f4, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_WALK32_TABLE[] = { func_ov036_0218f1a0, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb580[] = { func_ov036_0218f1b4, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb574[] = { func_ov036_0218f1c8, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb568[] = { func_ov036_0218f1dc, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_WALK16_TABLE[] = { func_ov036_0218f1f0, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb550[] = { func_ov036_0218f204, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb544[] = { func_ov036_0218f21c, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb538[] = { func_ov036_0218f230, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_WALK8_TABLE[] = { func_ov036_0218f244, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb520[] = { func_ov036_0218f25c, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb514[] = { func_ov036_0218f274, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb508[] = { func_ov036_0218f288, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_WALK4_TABLE[] = { func_ov036_0218f29c, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb4f0[] = { func_ov036_0218f2b0, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb4e4[] = { func_ov036_0218f2c4, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb4d8[] = { func_ov036_0218f2d8, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_WALK2_TABLE[] = { func_ov036_0218f2ec, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb4c0[] = { func_ov036_0218f304, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb4b4[] = { func_ov036_0218f31c, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb4a8[] = { func_ov036_0218f330, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb49c[] = { func_ov036_0218f348, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb490[] = { func_ov036_0218f35c, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb484[] = { func_ov036_0218f374, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb478[] = { func_ov036_0218f388, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_TURN32_TABLE[] = { func_ov036_0218f4c0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb460[] = { func_ov036_0218f4d0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb454[] = { func_ov036_0218f4e0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb448[] = { func_ov036_0218f4f0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_TURN16_TABLE[] = { func_ov036_0218f500, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb430[] = { func_ov036_0218f510, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb424[] = { func_ov036_0218f520, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb418[] = { func_ov036_0218f530, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_TURN8_TABLE[] = { func_ov036_0218f540, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb400[] = { func_ov036_0218f550, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb3f4[] = { func_ov036_0218f560, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb3e8[] = { func_ov036_0218f570, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_TURN4_TABLE[] = { func_ov036_0218f580, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb010[] = { func_ov036_0218f590, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb3c4[] = { func_ov036_0218f5a0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb3b8[] = { func_ov036_0218f5b0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_TURN2_TABLE[] = { func_ov036_0218f5c0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb3a0[] = { func_ov036_0218f5d0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb394[] = { func_ov036_0218f5e0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb388[] = { func_ov036_0218f5f0, func_ov036_0218f48c, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_JUMP16F[] = { func_ov036_0218f76c, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb370[] = { func_ov036_0218f790, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb364[] = { func_ov036_0218f7b4, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb358[] = { func_ov036_0218f7d4, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_JUMP8F[] = { func_ov036_0218f7f8, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb340[] = { func_ov036_0218f818, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb334[] = { func_ov036_0218f838, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb328[] = { func_ov036_0218f858, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb31c[] = { func_ov036_0218f878, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb310[] = { func_ov036_0218f898, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb304[] = { func_ov036_0218f8bc, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2f8[] = { func_ov036_0218f8e0, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2ec[] = { func_ov036_0218f900, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2e0[] = { func_ov036_0218f920, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2d4[] = { func_ov036_0218f944, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2c8[] = { func_ov036_0218f964, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_JUMP3G16F_TABLE[] = { func_ov036_0218f988, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1e4[] = { func_ov036_0218f9a8, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2a4[] = { func_ov036_0218f9c8, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb298[] = { func_ov036_0218f9ec, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb28c[] = { func_ov036_0218fad8, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb280[] = { func_ov036_0218fae4, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb250[] = { func_ov036_0218faf0, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb25c[] = { func_ov036_0218fafc, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb274[] = { func_ov036_0218fb08, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb2b0[] = { func_ov036_0218fb14, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb244[] = { func_ov036_0218fb20, func_ov036_0218fab8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb238[] = { func_ov036_0218fb2c, func_ov036_0218fb50, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb22c[] = { func_ov036_0218fb98, func_ov036_0218fbcc, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad04[] = { func_ov036_0218fc14, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacec[] = { func_ov036_0218fc28, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacac[] = { func_ov036_0218fc3c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacdc[] = { func_ov036_0218fc50, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacd4[] = { func_ov036_0218fc64, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacfc[] = { func_ov036_0218fc78, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cadb8[] = { func_ov036_0218fce8, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad28[] = { func_ov036_0218fcf8, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1c0[] = { func_ov036_0218fd08, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1b4[] = { func_ov036_0218fd18, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1a8[] = { func_ov036_0218fd38, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb19c[] = { func_ov036_0218fd48, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb190[] = { func_ov036_0218fd58, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb184[] = { func_ov036_0218fe04, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb178[] = { func_ov036_0218fe14, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb16c[] = { func_ov036_0218fe24, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb160[] = { func_ov036_0218fe34, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb154[] = { func_ov036_0218fe44, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb148[] = { func_ov036_0218fe54, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb13c[] = { func_ov036_0218fe64, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caec0[] = { func_ov036_0218fe74, func_ov036_0218fe84, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb124[] = { func_ov036_0218fe9c, func_ov036_0218fedc, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb118[] = { func_ov036_0218feac, func_ov036_0218fedc, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb10c[] = { func_ov036_0218febc, func_ov036_0218fedc, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb100[] = { func_ov036_0218fecc, func_ov036_0218fedc, func_ov036_0218f09c };

const ActorAcmdFunc ACMD_RUN4F_TABLE[] = { func_ov036_0218f3a0, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0e8[] = { func_ov036_0218f3b8, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0dc[] = { func_ov036_0218f3d0, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0d0[] = { func_ov036_0218f3e4, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0c4[] = { func_ov036_0218fef4, func_ov036_0218ff34, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0b8[] = { func_ov036_0218ff04, func_ov036_0218ff34, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0ac[] = { func_ov036_0218ff14, func_ov036_0218ff34, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb0a0[] = { func_ov036_0218ff24, func_ov036_0218ff34, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb094[] = { func_ov036_0218ff4c, func_ov036_0218ff8c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb088[] = { func_ov036_0218ff5c, func_ov036_0218ff8c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb07c[] = { func_ov036_0218ff6c, func_ov036_0218ff8c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb070[] = { func_ov036_0218ff7c, func_ov036_0218ff8c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb064[] = { func_ov036_0218ffa4, func_ov036_02190028, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb058[] = { func_ov036_0218ffa4, func_ov036_02190080, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb04c[] = { func_ov036_0218ffc0, func_ov036_02190028, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb040[] = { func_ov036_0218ffc0, func_ov036_02190080, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb034[] = { func_ov036_0218ffdc, func_ov036_02190028, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb028[] = { func_ov036_0218ffdc, func_ov036_02190080, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb01c[] = { func_ov036_0218fff8, func_ov036_02190028, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae30[] = { func_ov036_02190010, func_ov036_02190028, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb004[] = { func_ov036_021900c8, func_ov036_02190110, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caff8[] = { func_ov036_021901d0, func_ov036_021901b0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafec[] = { func_ov036_021901e8, func_ov036_021901b0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafe0[] = { func_ov036_02190200, func_ov036_021901b0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafd4[] = { func_ov036_02190394, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafc8[] = { func_ov036_021903c8, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafbc[] = { func_ov036_021903f8, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafb0[] = { func_ov036_0219042c, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf80[] = { func_ov036_0219045c, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf8c[] = { func_ov036_0219048c, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cafa4[] = { func_ov036_021904c0, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1f0[] = { func_ov036_021904f4, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb208[] = { func_ov036_02190528, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb214[] = { func_ov036_0219055c, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb268[] = { func_ov036_02190590, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb3d0[] = { func_ov036_021905c0, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf44[] = { func_ov036_021905f0, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf38[] = { func_ov036_02190620, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf2c[] = { func_ov036_02190650, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cadc4[] = { func_ov036_02190680, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1d8[] = { func_ov036_021906b0, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1cc[] = { func_ov036_021906e4, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caefc[] = { func_ov036_02190718, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caef0[] = { func_ov036_02190748, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caee4[] = { func_ov036_0218fa10, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caed8[] = { func_ov036_0218fa34, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caecc[] = { func_ov036_0218fa58, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad94[] = { func_ov036_0218fa7c, func_ov036_0218f690, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caeb4[] = { func_ov036_02190778, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caea8[] = { func_ov036_021907ac, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae9c[] = { func_ov036_021907e0, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae90[] = { func_ov036_02190814, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae84[] = { func_ov036_02190848, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae78[] = { func_ov036_0219087c, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae6c[] = { func_ov036_021908b0, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae60[] = { func_ov036_021908e4, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae54[] = { func_ov036_02190918, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae48[] = { func_ov036_0219094c, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae3c[] = { func_ov036_02190980, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae00[] = { func_ov036_021909b4, func_ov036_02190350, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae18[] = { func_ov036_02190c4c, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae24[] = { func_ov036_02190c70, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf50[] = { func_ov036_02190c98, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf5c[] = { func_ov036_02190cc0, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf98[] = { func_ov036_02190ce8, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb1fc[] = { func_ov036_02190d0c, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb220[] = { func_ov036_02190d38, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb598[] = { func_ov036_02190d60, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cadd0[] = { func_ov036_02190d88, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad1c[] = { func_ov036_02190db0, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf08[] = { func_ov036_02190ddc, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cadac[] = { func_ov036_02190e04, func_ov036_02190ad0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cada0[] = { func_ov036_0218fd28, func_ov036_0218fcc0, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad40[] = { func_ov036_02190218, func_ov036_02190234, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad88[] = { func_ov036_0218f3fc, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad7c[] = { func_ov036_0218f414, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad70[] = { func_ov036_0218f42c, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caddc[] = { func_ov036_0218f440, func_ov036_0218f14c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cadf4[] = { func_ov036_02190e28, func_ov036_02190e44, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacbc[] = { func_ov036_02190e60, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf20[] = { func_ov036_02190e74, func_ov036_02190eac, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf68[] = { func_ov036_02190eec, func_ov036_02190f14, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacc4[] = { func_ov036_0219100c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cb5a4[] = { func_ov036_02191020, func_ov036_02191034, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cace4[] = { func_ov036_02191090, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf14[] = { func_ov036_021910b0, func_ov036_021910f4, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cacb4[] = { func_ov036_02191174, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cad4c[] = { func_ov036_02191188, func_ov036_021911e8, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cade8[] = { func_ov036_0219119c, func_ov036_0219120c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021cae0c[] = { func_ov036_021911b0, func_ov036_0219120c, func_ov036_0218f09c };

const ActorAcmdFunc data_ov036_021caf74[] = { func_ov036_021911cc, func_ov036_0219120c, func_ov036_0218f09c };

static const s32 data_ov036_021cb5b0[] = { 0x5000, 0x6000, 0x5000 };

static const fx32 data_ov036_021cb7bc[] = { 0,
                                            FX32_CONST(2),
                                            FX32_CONST(3),
                                            FX32_CONST(4),
                                            FX32_CONST(5),
                                            FX32_CONST(6),
                                            FX32_CONST(6),
                                            FX32_CONST(6),
                                            FX32_CONST(5),
                                            FX32_CONST(5),
                                            FX32_CONST(4),
                                            FX32_CONST(3),
                                            FX32_CONST(2),
                                            0,
                                            0,
                                            0 };

static const s32 data_ov036_021cb5d4[] = { 0x2000, 0x3000, 0x3000, 0x2000, 0x3000, 0x3000 };

static const s32 data_ov036_021cb608[] = { 0x1000, 0x1000, 0x2000, 0x1000, 0x1000, 0x2000,
                                           0x1000, 0x1000, 0x2000, 0x1000, 0x1000, 0x2000 };

static const s32 data_ov036_021cb5ec[] = { 0x2500, 0x2480, 0x2480, 0x2480, 0x2480, 0x2480, 0x2480 };

static const fx32 data_ov036_021cb7fc[] = { FX32_CONST(2),
                                            FX32_CONST(4),
                                            FX32_CONST(6),
                                            FX32_CONST(8),
                                            FX32_CONST(9),
                                            FX32_CONST(10),
                                            FX32_CONST(10),
                                            FX32_CONST(10),
                                            FX32_CONST(9),
                                            FX32_CONST(8),
                                            FX32_CONST(6),
                                            FX32_CONST(5),
                                            FX32_CONST(3),
                                            FX32_CONST(2),
                                            0,
                                            0 };

static const fx32 data_ov036_021cb83c[] = { FX32_CONST(4),
                                            FX32_CONST(6),
                                            FX32_CONST(8),
                                            FX32_CONST(10),
                                            FX32_CONST(11),
                                            FX32_CONST(12),
                                            FX32_CONST(12),
                                            FX32_CONST(12),
                                            FX32_CONST(11),
                                            FX32_CONST(10),
                                            FX32_CONST(9),
                                            FX32_CONST(8),
                                            FX32_CONST(6),
                                            FX32_CONST(4),
                                            0,
                                            0 };

static const fx32 *data_ov036_021d4d34[] = { data_ov036_021cb83c, data_ov036_021cb7bc, data_ov036_021cb7fc };
