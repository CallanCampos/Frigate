// The move codes that follow another actor or the player, hide an actor under a field effect, turn it and hop it:
// the functions that fldmmdl_movedata.c's tables list for each, and the steps they run. The ROM gives this file no
// name, so fldmmdl_movecode_pair.c is a guess after the first of them (FindPairedTrainerActor is swan's). Its .text
// starts at 0x02192a4c and its .rodata, sorted by size on its own, at 0x021cbeec (Black 2). The code was first
// written by mariano54 on their parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "field/event_trainer_eye.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/fldmmdl_movecode_pair.h"
#include "nitro/fx.h"

// A position in fx32 and the grid square that holds it
#define FX32_TO_GRID(pos) ((s16)(((pos) >> 4) / FX32_ONE))
#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))

// The block of the codes that follow another actor or the player
typedef struct {
    u8 state;
    u8 active;
    s16 x;
    s16 z;
    s16 unk6;
    FieldActor *target;
} ActorFollowWork;

typedef struct {
    u8 state;
    u8 variant;
} ActorBobWork;

typedef struct {
    u8 state;
    u8 kind;
    u8 hasEffect;
    FieldEffectTask *task;
} ActorEffectWork;

typedef struct {
    u8 state;
    s8 dir;
    u8 flag;
} ActorTurnWork;

typedef struct {
    u8 state;
    s32 mode;
    s32 variant;
} ActorHopWork;

static BOOL func_ov036_02192aac(FieldActor *actor, ActorFollowWork *work);
static BOOL func_ov036_02192afc(FieldActor *actor, ActorFollowWork *work);
static BOOL func_ov036_02192b18(FieldActor *actor, ActorFollowWork *work);
static void func_ov036_02192b44(FieldActor *actor, ActorFollowWork *work);
static BOOL func_ov036_02192b6c(FieldActor *actor, ActorFollowWork *work);
static void func_ov036_02192ba0(FieldActor *actor, ActorFollowWork *work);
static u32 func_ov036_02192bc0(FieldActor *actor);
static BOOL func_ov036_02192c28(FieldActor *actor);
static BOOL func_ov036_02192d10(FieldActor *actor, ActorFollowWork *work);
static BOOL func_ov036_02192d4c(FieldActor *actor, ActorFollowWork *work);
static BOOL func_ov036_02192e10(FieldActor *actor, ActorFollowWork *work);
static void func_ov036_02192e88(FieldActor *actor, ActorFollowWork *work, FieldActor *other);
static BOOL func_ov036_02192eac(FieldActor *actor, ActorFollowWork *work);
static BOOL func_ov036_02192ef8(FieldActor *actor, ActorFollowWork *work);
static void func_ov036_02192f8c(FieldActor *actor, u32 variant);
static FieldEffectTask *func_ov036_02193088(FieldActor *actor, u32 kind);
static BOOL func_ov036_021930a4(FieldActor *actor, ActorEffectWork *work);
static BOOL func_ov036_021930d4(FieldActor *actor, ActorEffectWork *work);
static void func_ov036_02193134(FieldActor *actor, u32 unused, u32 flag);
static BOOL func_ov036_021931d4(FieldActor *actor, ActorTurnWork *work);
static BOOL func_ov036_02193208(FieldActor *actor, ActorTurnWork *work);
static BOOL func_ov036_02193220(FieldActor *actor, ActorTurnWork *work);
static BOOL func_ov036_02193254(FieldActor *actor, ActorTurnWork *work);
static BOOL func_ov036_02193280(FieldActor *actor, ActorTurnWork *work);
static void func_ov036_021932a8(FieldActor *actor, s32 mode, s32 variant);
static BOOL func_ov036_02193310(FieldActor *actor, ActorHopWork *work);
static BOOL func_ov036_02193324(FieldActor *actor, ActorHopWork *work);
static BOOL func_ov036_02193338(FieldActor *actor, ActorHopWork *work);
static BOOL func_ov036_02193354(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant);
static BOOL func_ov036_021933e0(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant);
static BOOL func_ov036_0219346c(FieldActor *actor, u32 dir, u32 variant);
static BOOL func_ov036_02193490(FieldActor *actor, u32 dir, u32 variant);
static u32 func_ov036_021934b4(FieldActor *actor, u32 dir, u32 variant);
static u32 func_ov036_021934e8(FieldActor *actor, u32 *dir, u32 variant);
static BOOL func_ov036_0219350c(FieldActor *actor, ActorHopWork *work, u32 speed);

// The tables are declared in this order because MWCC lays out objects of the same size by where each was declared:
// it is one that gives the ROM's .rodata (found with tools/decomp/rodata_order.py's model)
static BOOL (*const data_ov036_021cbeec[])(FieldActor *actor, ActorFollowWork *work) = { func_ov036_02192aac,
                                                                                         func_ov036_02192afc };

static BOOL (*const data_ov036_021cbef4[])(FieldActor *actor, ActorEffectWork *work) = { func_ov036_021930a4,
                                                                                         func_ov036_021930d4 };

static BOOL (*const data_ov036_021cbf18[])(FieldActor *actor, ActorTurnWork *work) = {
    func_ov036_021931d4, func_ov036_02193208, func_ov036_02193220, func_ov036_02193254, func_ov036_02193280
};

static BOOL (*const data_ov036_021cbf04[])(FieldActor *actor, ActorFollowWork *work) = { func_ov036_02192d10,
                                                                                         func_ov036_02192d4c };

static BOOL (*const data_ov036_021cbf0c[])(FieldActor *actor, ActorHopWork *work) = { func_ov036_02193310,
                                                                                      func_ov036_02193324,
                                                                                      func_ov036_02193338 };

static const s32 data_ov036_021cbfac[][2] = { { 0x1, 0x1 }, { -0x1, -0x1 }, { 0x1, -0x1 }, { -0x1, 0x1 } };

static const u32 data_ov036_021cbefc[] = { 0x1, 0 };

static const s32 data_ov036_021cbf2c[][2] = { { -0x1, 0x1 }, { 0x1, -0x1 }, { 0, 0 }, { 0, 0 } };

static const s32 data_ov036_021cbf4c[][2] = { { 0, 0 }, { 0, 0 }, { 0x1, -0x1 }, { -0x1, 0x1 } };

static const s32 data_ov036_021cbf8c[][2] = { { -0x1, 0x1 }, { 0x1, -0x1 }, { 0x1, 0x1 }, { -0x1, -0x1 } };

static const s32 data_ov036_021cbfcc[][2] = {
    { DIR_RIGHT, DIR_LEFT }, { DIR_LEFT, DIR_RIGHT }, { DIR_UP, DIR_DOWN }, { DIR_DOWN, DIR_UP }
};

static const s32 data_ov036_021cbf6c[][2] = {
    { DIR_LEFT, DIR_RIGHT }, { DIR_RIGHT, DIR_LEFT }, { DIR_DOWN, DIR_UP }, { DIR_UP, DIR_DOWN }
};

void func_ov036_02192a4c(FieldActor *actor) {
    ActorFollowWork *work = ClearActorPositionBlock(actor, 8);

    func_ov036_02192b18(actor, work);
    func_ov012_021670f4(actor, 0);
    func_ov012_021674bc(actor);
    SetActorFlag8000(actor, 0);
}

void func_ov036_02192a78(FieldActor *actor) {
    ActorFollowWork *work = GetNPCRailPosPtrAddr(actor);

    if (func_ov036_02192b18(actor, work)) {
        while (data_ov036_021cbeec[work->state](actor, work) == TRUE) {
        }
    }
}

void func_ov036_02192aa8(FieldActor *actor) {
}

static BOOL func_ov036_02192aac(FieldActor *actor, ActorFollowWork *work) {
    func_ov012_021674bc(actor);
    func_ov012_021674e8(actor);
    if (func_ov012_02167520(actor)) {
        return FALSE;
    }
    if (func_ov036_02192b6c(actor, work) == TRUE) {
        func_ov036_02192ba0(actor, work);
        if (func_ov036_02192c28(actor) == TRUE) {
            func_ov012_021674b0(actor);
            work->state++;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_02192afc(FieldActor *actor, ActorFollowWork *work) {
    if (func_ov036_0218f01c(actor) == TRUE) {
        func_ov012_021674bc(actor);
        work->state = 0;
    }
    return FALSE;
}

static BOOL func_ov036_02192b18(FieldActor *actor, ActorFollowWork *work) {
    if (FindPlayerFieldActor(GetActorMModelSystem(actor)) == NULL) {
        work->active = 0;
        return FALSE;
    }
    if (work->active == 0) {
        func_ov036_02192b44(actor, work);
    }
    return TRUE;
}

static void func_ov036_02192b44(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    work->active = 1;
    work->x = GetGPosX(player);
    work->z = GetGPosZ(player);
    work->unk6 = 0xff;
}

static BOOL func_ov036_02192b6c(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    if (player != NULL) {
        s32 x = GetGPosX(player);
        s32 z = GetGPosZ(player);

        if (x != work->x || z != work->z) {
            return TRUE;
        }
    }
    return FALSE;
}

static void func_ov036_02192ba0(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    work->x = GetGPosX(player);
    work->z = GetGPosZ(player);
}

static u32 func_ov036_02192bc0(FieldActor *actor) {
    u16 acmd = FldAct_GetAcmd(FindPlayerFieldActor(GetActorMModelSystem(actor)));

    switch (acmd) {
    case 0x58:
        return 0x10;
    case 0x59:
        return 0x11;
    case 0x5a:
        return 0x12;
    case 0x5b:
        return 0x13;
    case 0xa3:
        return 0x4c;
    case 0xa4:
        return 0x4d;
    case 0xa5:
        return 0x4e;
    case 0xa6:
        return 0x4f;
    }
    return acmd;
}

static BOOL func_ov036_02192c28(FieldActor *actor) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));
    s32 x = GetGPosX(actor);
    s32 z = GetGPosZ(actor);
    s32 playerInitX = FldAct_GetInitGPosX(player);
    s32 playerInitZ = FldAct_GetInitGPosZ(player);
    u32 acmd;

    if (x != playerInitX || z != playerInitZ) {
        acmd = func_ov036_02192bc0(actor);
        if (acmd == 0xff) {
            return FALSE;
        }
        StartDirAcmd(actor, GetDirFromPosToPos(x, z, playerInitX, playerInitZ), acmd);
        return TRUE;
    }
    return FALSE;
}

void func_ov036_02192ca8(FieldActor *actor) {
    ActorFollowWork *work = ClearActorPositionBlock(actor, 12);

    func_ov036_02192e10(actor, work);
    func_ov012_021670f4(actor, 0);
    func_ov012_021674bc(actor);
    work->active = 0;
}

void func_ov036_02192cd0(FieldActor *actor) {
    ActorFollowWork *work = GetNPCRailPosPtrAddr(actor);

    if (func_ov036_02192e10(actor, work)) {
        while (data_ov036_021cbf04[work->state](actor, work) == TRUE) {
        }
    }
}

void func_ov036_02192d00(FieldActor *actor) {
}

void func_ov036_02192d04(FieldActor *actor) {
    ActorFollowWork *work = GetNPCRailPosPtrAddr(actor);

    work->active = 0;
}

static BOOL func_ov036_02192d10(FieldActor *actor, ActorFollowWork *work) {
    func_ov012_021674bc(actor);
    func_ov012_021674e8(actor);
    if (func_ov036_02192eac(actor, work) == TRUE) {
        if (func_ov036_02192ef8(actor, work) == TRUE) {
            func_ov012_021674b0(actor);
            work->state++;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_02192d4c(FieldActor *actor, ActorFollowWork *work) {
    if (!func_ov036_0218f01c(actor)) {
        return FALSE;
    }
    func_ov012_021674bc(actor);
    work->state = 0;
    return FALSE;
}

FieldActor *FindPairedTrainerActor(FieldActor *actor) {
    FieldActor *other;
    u32 index = 0;
    u16 evType = GetActorEvType(actor);
    u16 zoneId = GetActorZoneID(actor);
    MMSys *mmSys = GetActorMModelSystem(actor);
    u16 trainerId = GetNPCTrainerID(actor);

    switch (evType) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 20:
        while (NextActor(mmSys, &other, &index) == TRUE) {
            if (actor != other && zoneId == GetActorZoneID(other) && trainerId == GetNPCTrainerID(other)) {
                return other;
            }
        }
        break;
    }
    return NULL;
}

static BOOL func_ov036_02192e10(FieldActor *actor, ActorFollowWork *work) {
    u16 zoneId;
    u16 trainerId;
    u32 index;
    FieldActor *other;
    MMSys *mmSys;

    mmSys = GetActorMModelSystem(actor);
    index = 0;
    zoneId = GetActorZoneID(actor);
    trainerId = GetNPCTrainerID(actor);
    while (NextActor(mmSys, &other, &index) == TRUE) {
        if (actor != other && zoneId == GetActorZoneID(other) && trainerId == GetNPCTrainerID(other)) {
            if (work->active == 0) {
                func_ov036_02192e88(actor, work, other);
            }
            return TRUE;
        }
    }
    work->active = 0;
    return FALSE;
}

static void func_ov036_02192e88(FieldActor *actor, ActorFollowWork *work, FieldActor *other) {
    work->active = 1;
    work->x = GetGPosX(other);
    work->z = GetGPosZ(other);
    work->unk6 = 0xff;
    work->target = other;
}

static BOOL func_ov036_02192eac(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *target = work->target;
    s32 x = GetGPosX(actor);
    s32 z = GetGPosZ(actor);
    s32 targetInitX = FldAct_GetInitGPosX(target);
    s32 targetInitZ = FldAct_GetInitGPosZ(target);

    if (x != targetInitX || z != targetInitZ) {
        if (IsActorFlag16(target) == TRUE || !CheckActorMovementFlag(target, 0x1808)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_02192ef8(FieldActor *actor, ActorFollowWork *work) {
    s32 x = GetGPosX(actor);
    s32 z = GetGPosZ(actor);
    s32 targetX = GetGPosX(work->target);
    s32 targetZ = GetGPosZ(work->target);
    s32 targetInitX = FldAct_GetInitGPosX(work->target);
    s32 targetInitZ = FldAct_GetInitGPosZ(work->target);
    s32 dir;

    if (x == targetX && z == targetZ) {
        return FALSE;
    }
    dir = GetDirFromPosToPos(x, z, targetInitX, targetInitZ);
    x += GetDirectionVectorCompX(dir);
    z += GetDirectionVectorCompZ(dir);
    if (x != targetX || z != targetZ) {
        StartDirAcmd(actor, dir, 12);
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_02192f8c(FieldActor *actor, u32 variant) {
    ActorBobWork *work = ClearActorPositionBlock(actor, 8);

    work->variant = variant;
    func_ov012_021670f4(actor, 0);
    func_ov012_021674bc(actor);
    SetActorMovementFlag(actor, 0x8000);
    VecFx32 offset = { 0, -32 * FX32_ONE, 0 };

    SetActorWPosOffset(actor, &offset);
}

void func_ov036_02192fd4(FieldActor *actor) {
    func_ov036_02192f8c(actor, 0);
}

void func_ov036_02192fe0(FieldActor *actor) {
    func_ov036_02192f8c(actor, 1);
}

void func_ov036_02192fec(FieldActor *actor) {
    func_ov036_02192f8c(actor, 1);
}

void func_ov036_02192ff8(FieldActor *actor) {
    func_ov036_02192f8c(actor, 0);
}

void func_ov036_02193004(FieldActor *actor) {
    func_ov036_02192f8c(actor, 2);
}

void func_ov036_02193010(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cbef4[work->state](actor, work) == TRUE) {
    }
}

void func_ov036_02193034(FieldActor *actor) {
    FieldEffectTask *task = func_ov036_0219311c(actor);

    if (task != NULL) {
        func_ov036_021a3a70(task);
    }
}

void func_ov036_02193044(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    work->state = 0;
    func_ov036_02193110(actor, NULL);
    if (work->hasEffect == 0) {
        VecFx32 offset = { 0, -32 * FX32_ONE, 0 };

        SetActorWPosOffset(actor, &offset);
        SetActorMovementFlag(actor, 0x8000);
    }
}

static FieldEffectTask *func_ov036_02193088(FieldActor *actor, u32 kind) {
    return func_ov036_021c67c8(Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor))), actor, kind);
}

static BOOL func_ov036_021930a4(FieldActor *actor, ActorEffectWork *work) {
    if (work->hasEffect == 0) {
        func_ov036_02193110(actor, func_ov036_02193088(actor, work->kind));
    }
    func_ov012_021674bc(actor);
    func_ov012_021674e8(actor);
    work->state++;
    return FALSE;
}

static BOOL func_ov036_021930d4(FieldActor *actor, ActorEffectWork *work) {
    if (work->hasEffect == 0) {
        if (func_ov036_0219311c(actor) == NULL) {
            if (func_ov012_02167614(actor) == TRUE) {
                func_ov036_02193110(actor, func_ov036_02193088(actor, work->kind));
            }
        }
        SetActorMovementFlag(actor, 0x8000);
    }
    return FALSE;
}

void func_ov036_02193110(FieldActor *actor, FieldEffectTask *task) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    work->task = task;
}

FieldEffectTask *func_ov036_0219311c(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    return work->task;
}

void func_ov036_02193128(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    work->hasEffect = 1;
}

static void func_ov036_02193134(FieldActor *actor, u32 unused, u32 flag) {
    ActorTurnWork *work = ClearActorPositionBlock(actor, 4);

    work->dir = DIR_NONE;
    work->flag = flag;
    CheckSetActorFaceDir(actor, DIR_UP);
}

void func_ov036_02193150(FieldActor *actor) {
    func_ov036_02193134(actor, 0, 0);
}

void func_ov036_0219315c(FieldActor *actor) {
    func_ov036_02193134(actor, 1, 0);
}

void func_ov036_02193168(FieldActor *actor) {
    func_ov036_02193134(actor, 2, 0);
}

void func_ov036_02193174(FieldActor *actor) {
    func_ov036_02193134(actor, 3, 0);
}

void func_ov036_02193180(FieldActor *actor) {
    func_ov036_02193134(actor, 0, 1);
}

void func_ov036_0219318c(FieldActor *actor) {
    func_ov036_02193134(actor, 1, 1);
}

void func_ov036_02193198(FieldActor *actor) {
    func_ov036_02193134(actor, 2, 1);
}

void func_ov036_021931a4(FieldActor *actor) {
    func_ov036_02193134(actor, 3, 1);
}

void func_ov036_021931b0(FieldActor *actor) {
    ActorTurnWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cbf18[work->state](actor, work) == TRUE) {
    }
}

static BOOL func_ov036_021931d4(FieldActor *actor, ActorTurnWork *work) {
    u32 dir = GetActorFaceDir(actor);

    StartDirAcmd(actor, dir, 0);
    func_ov012_021674bc(actor);
    func_ov012_021674e8(actor);
    work->state = 1;
    return TRUE;
}

static BOOL func_ov036_02193208(FieldActor *actor, ActorTurnWork *work) {
    if (func_ov036_0218f01c(actor) == TRUE) {
        work->state = 2;
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_02193220(FieldActor *actor, ActorTurnWork *work) {
    if (work->dir == DIR_NONE) {
        FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

        if (player != NULL) {
            work->dir = GetActorMotionDir(player);
        }
    }
    func_ov012_021674bc(actor);
    func_ov012_021674e8(actor);
    work->state = 3;
    return TRUE;
}

static BOOL func_ov036_02193254(FieldActor *actor, ActorTurnWork *work) {
    u32 acmd = GetAcmdForDir(GetActorMotionDir(FindPlayerFieldActor(GetActorMModelSystem(actor))), 0);

    func_ov012_02166eb0(actor, acmd);
    work->state = 4;
    return TRUE;
}

static BOOL func_ov036_02193280(FieldActor *actor, ActorTurnWork *work) {
    if (!func_ov036_0218f01c(actor)) {
        return FALSE;
    }
    func_ov012_021674bc(actor);
    func_ov012_021674e8(actor);
    work->state = 2;
    return FALSE;
}

static void func_ov036_021932a8(FieldActor *actor, s32 mode, s32 variant) {
    ActorHopWork *work = ClearActorPositionBlock(actor, 12);

    work->mode = mode;
    work->variant = variant;
}

void func_ov036_021932bc(FieldActor *actor) {
    func_ov036_021932a8(actor, 0, 0);
}

void func_ov036_021932c8(FieldActor *actor) {
    func_ov036_021932a8(actor, 1, 1);
}

void func_ov036_021932d4(FieldActor *actor) {
    func_ov036_021932a8(actor, 2, 0);
}

void func_ov036_021932e0(FieldActor *actor) {
    func_ov036_021932a8(actor, 2, 1);
}

void func_ov036_021932ec(FieldActor *actor) {
    ActorHopWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cbf0c[work->state](actor, work) == TRUE) {
    }
}

static BOOL func_ov036_02193310(FieldActor *actor, ActorHopWork *work) {
    func_ov012_021674bc(actor);
    work->state++;
    return TRUE;
}

static BOOL func_ov036_02193324(FieldActor *actor, ActorHopWork *work) {
    func_ov036_0219350c(actor, work, 12);
    work->state++;
    return TRUE;
}

static BOOL func_ov036_02193338(FieldActor *actor, ActorHopWork *work) {
    if (func_ov036_0218f01c(actor) == TRUE) {
        func_ov012_021674bc(actor);
        work->state = 0;
    }
    return FALSE;
}

static BOOL func_ov036_02193354(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant) {
    u32 tileType;
    BOOL blocked = FALSE;
    VecFx32 target = *pos;
    s16 x = FX32_TO_GRID(pos->x);
    s16 z = FX32_TO_GRID(pos->z);

    x += data_ov036_021cbf2c[dir][variant];
    z += data_ov036_021cbf4c[dir][variant];
    target.x = GRID_TO_FX32(x);
    target.z = GRID_TO_FX32(z);
    if (GetTileTypeAtPosByActor(actor, &target, &tileType) == TRUE) {
        if (MapTile_BlocksCollision(tileType)) {
            blocked = TRUE;
        }
    }
    return blocked;
}

static BOOL func_ov036_021933e0(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant) {
    u32 tileType;
    BOOL blocked = FALSE;
    VecFx32 target = *pos;
    s16 x = FX32_TO_GRID(pos->x);
    s16 z = FX32_TO_GRID(pos->z);

    x += data_ov036_021cbf8c[dir][variant];
    z += data_ov036_021cbfac[dir][variant];
    target.x = GRID_TO_FX32(x);
    target.z = GRID_TO_FX32(z);
    if (GetTileTypeAtPosByActor(actor, &target, &tileType) == TRUE) {
        if (MapTile_BlocksCollision(tileType)) {
            blocked = TRUE;
        }
    }
    return blocked;
}

static BOOL func_ov036_0219346c(FieldActor *actor, u32 dir, u32 variant) {
    VecFx32 pos;

    CopyActorWPos(actor, &pos);
    return func_ov036_02193354(actor, &pos, dir, variant);
}

static BOOL func_ov036_02193490(FieldActor *actor, u32 dir, u32 variant) {
    VecFx32 pos;

    CopyActorWPos(actor, &pos);
    return func_ov036_021933e0(actor, &pos, dir, variant);
}

static u32 func_ov036_021934b4(FieldActor *actor, u32 dir, u32 variant) {
    if (!func_ov036_0219346c(actor, dir, variant)) {
        if (!func_ov036_02193490(actor, dir, variant)) {
            return DIR_NONE;
        }
        dir = data_ov036_021cbf6c[dir][variant];
    }
    return dir;
}

static u32 func_ov036_021934e8(FieldActor *actor, u32 *dir, u32 variant) {
    *dir = func_ov036_021934b4(actor, *dir, variant);
    if (*dir != DIR_NONE) {
        return ActorRouteCollCheckOneTileInDir(actor, *dir);
    }
    return 0;
}

static BOOL func_ov036_0219350c(FieldActor *actor, ActorHopWork *work, u32 speed) {
    u32 acmd;
    u32 variant = work->variant;
    u32 dir = GetActorMotionDir(actor);
    u32 result = func_ov036_021934e8(actor, &dir, variant);

    if (dir == DIR_NONE) {
        dir = GetActorMotionDir(actor);
        acmd = GetAcmdForDir(dir, 0x1c);
        func_ov012_02166eb0(actor, acmd);
        return FALSE;
    }
    if (result == 0) {
        acmd = GetAcmdForDir(dir, speed);
        func_ov012_021674b0(actor);
        func_ov012_02166eb0(actor, acmd);
        return TRUE;
    }
    if ((result & 1) && work->mode == 2) {
        dir = GetInverseDirection(GetActorMotionDir(actor));
        variant = data_ov036_021cbefc[variant];
        work->variant = variant;
        result = func_ov036_021934e8(actor, &dir, variant);
        if (dir == DIR_NONE) {
            dir = GetActorMotionDir(actor);
            acmd = GetAcmdForDir(dir, 0x1c);
            func_ov012_02166eb0(actor, acmd);
            return FALSE;
        }
        if (result == 0) {
            acmd = GetAcmdForDir(dir, speed);
            func_ov012_021674b0(actor);
            func_ov012_02166eb0(actor, acmd);
            return TRUE;
        }
    }
    if (result & 2) {
        dir = data_ov036_021cbfcc[dir][variant];
        result = func_ov036_021934e8(actor, &dir, variant);
        if (dir == DIR_NONE) {
            dir = GetActorMotionDir(actor);
            acmd = GetAcmdForDir(dir, 0x1c);
            func_ov012_02166eb0(actor, acmd);
            return FALSE;
        }
        if (result == 0) {
            acmd = GetAcmdForDir(dir, speed);
            func_ov012_021674b0(actor);
            func_ov012_02166eb0(actor, acmd);
            return TRUE;
        }
    }
    dir = GetActorMotionDir(actor);
    acmd = GetAcmdForDir(dir, 0x1c);
    func_ov012_02166eb0(actor, acmd);
    return FALSE;
}
