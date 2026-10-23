// The field player's movement on a rail: each frame the held direction and the rail ahead decide a command, by
// whether the player walks, cycles or surfs, and the command sets the actor's next animation command. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/obj_codes.h"
#include "constants/sound.h"
#include "field/event_data.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_player_nogrid.h"
#include "field/field_rail.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "nitro/mi.h"
#include "system/game_data.h"
#include "system/game_system.h"

// What the player is doing, which the next command depends on
enum {
    NOGRID_STATE_IDLE,
    NOGRID_STATE_MOVE,
    NOGRID_STATE_TURN,
    NOGRID_STATE_BRAKE,
};

// What the player does next
enum {
    NOGRID_COMMAND_NONE,
    NOGRID_COMMAND_IDLE,
    NOGRID_COMMAND_MOVE,
    NOGRID_COMMAND_TURN,
    NOGRID_COMMAND_BRAKE,
    NOGRID_COMMAND_JUMP,
};

#define NOGRID_FLAG_SLIDING 0x1
#define NOGRID_FLAG_SLIDE_END_SPOT 0x2
#define NOGRID_FLAG_4 0x4
#define NOGRID_FLAG_SLIDE_A 0x8
#define NOGRID_FLAG_SLIDE_B 0x10
#define NOGRID_FLAG_SLIDE_C 0x20
#define NOGRID_FLAG_SLIDE_PENDING 0x40

#define NOGRID_MOVE_CODE 0x45 // Guessed name: the player rail movement callback set.

#define ACMD_FACE 0x0
#define ACMD_WALK_8F 0x8
#define ACMD_WALK_8F_DASH 0xc
#define ACMD_WALK_4F 0x10
#define ACMD_WALK_2F 0x14
#define ACMD_STAY_WALK_16F 0x1c
#define ACMD_STAY_WALK_2F 0x28
#define ACMD_JUMP_2 0x38
#define ACMD_DASH 0x58

typedef struct {
    u8 active;
    u8 kind;
    s16 step;
} StepSound;

typedef struct {
    u8 active : 1;
    u8 held : 1;
    u8 pending : 1;
    u8 mode : 5;
    s16 time;
    u32 dir;
} SlideState;

struct FieldPlayerNoGrid {
    u32 state;
    u32 flags;
    StepSound sound;
    SlideState slide;
    FieldPlayerCore *core;
    Field *field;
    FieldActor *actor;
    RailUnit *railUnit;
};

typedef u16 (*SlideFunc)(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);

static BOOL FieldPlayerRail_CheckAllowUpdate(FieldPlayerNoGrid *nogrid, u16 dir);
static u32 func_ov036_0219d094(FieldPlayerNoGrid *nogrid, u32 dir, u32 pressedKeys, u32 heldKeys, BOOL a4);
static void func_ov036_0219d100(FieldPlayerNoGrid *nogrid, u32 command, u32 dir, u32 pressedKeys, u32 heldKeys,
                                BOOL a5);
static void func_ov036_0219d178(FieldPlayerNoGrid *nogrid, u32 heldKeys, u32 command);
static u32 func_ov036_0219d198(FieldPlayerNoGrid *nogrid, FieldActor *actor, u16 dir, u32 *tileType);
static void func_ov036_0219d1a8(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219d230(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static u32 func_ov036_0219d2ac(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir, BOOL a4);
static u32 func_ov036_0219d314(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219d374(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219d448(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219d498(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static void func_ov036_0219d510(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d514(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d54c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d584(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d5a8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d5e4(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d608(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static u32 func_ov036_0219d684(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir, BOOL a4);
static u32 func_ov036_0219d6ec(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219d74c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219d820(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219d870(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static void func_ov036_0219d8e8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d8ec(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d924(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d948(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d96c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d9a8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219d9cc(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219da5c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219da60(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219da98(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219dac4(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static void func_ov036_0219dae8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5);
static u32 func_ov036_0219db24(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir, BOOL a4);
static u32 func_ov036_0219db9c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219dbfc(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219dc8c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static u32 func_ov036_0219dcdc(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5);
static void func_ov036_0219dd54(FieldPlayerNoGrid *nogrid, u32 flag);
static void func_ov036_0219dd5c(FieldPlayerNoGrid *nogrid, u32 flag);
static BOOL func_ov036_0219dd68(FieldPlayerNoGrid *nogrid, u32 flag);
static void func_ov036_0219dd78(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219dd84(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219dd90(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219dd9c(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219dda8(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219ddb4(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219ddc0(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219ddcc(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219ddd8(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219dde4(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219ddf0(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219ddfc(FieldPlayerNoGrid *nogrid);
static void func_ov036_0219de08(FieldPlayerNoGrid *nogrid);
static u32 func_ov036_0219de14(FieldPlayerNoGrid *nogrid);
static u16 func_ov036_0219de9c(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static void func_ov036_0219dee0(FieldPlayerNoGrid *nogrid);
static u16 func_ov036_0219df78(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static u16 func_ov036_0219dfe4(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static u16 func_ov036_0219e048(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static u16 func_ov036_0219e0ac(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static u16 func_ov036_0219e0d4(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static u16 func_ov036_0219e0fc(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static u16 func_ov036_0219e110(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2);
static void func_ov036_0219e178(StepSound *sound, u32 kind);
static void func_ov036_0219e1a0(StepSound *sound);
static void func_ov036_0219e1c0(StepSound *sound, FieldActor *actor);
static void func_ov036_0219e1f8(SlideState *slide);
static void func_ov036_0219e20c(SlideState *slide);
static void func_ov036_0219e21c(SlideState *slide);
static void func_ov036_0219e224(SlideState *slide, u16 dir, FieldActor *actor);
static void func_ov036_0219e2a0(SlideState *slide);
static u32 func_ov036_0219e2bc(SlideState *slide, u16 dir);
static u32 func_ov036_0219e2d4(SlideState *slide);
static BOOL func_ov036_0219e2dc(FieldPlayerNoGrid *nogrid, u16 dir);

// The directions the player faces in turn while it spins, by the way it spins
static const u8 SPIN_FACE_DIRS[2][4] = {
    { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT },
    { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT },
};

// What the player does on each kind of slide, by func_ov036_0219de14's result
static const SlideFunc SLIDE_FUNCS[8] = {
    NULL,
    func_ov036_0219df78,
    func_ov036_0219dfe4,
    func_ov036_0219e048,
    func_ov036_0219e0ac,
    func_ov036_0219e0d4,
    func_ov036_0219e0fc,
    func_ov036_0219e110,
};

// The player's actor on a rail, whose object code is filled in
static ZoneNPC sPlayerTemplate = { 0xff, OBJ_CODE_HERO, NOGRID_MOVE_CODE, 0, 0, 0, 0, 0, 0, 0, 0xffff, 0xffff, TRUE };

FieldPlayerNoGrid *func_ov036_0219cea0(FieldPlayerCore *core, HeapID heapId) {
    FieldPlayerNoGrid *nogrid = GFL_HeapAllocate(heapId, sizeof(FieldPlayerNoGrid), TRUE, "field_player_nogrid.c", 416);

    nogrid->core = core;
    nogrid->field = FieldPlayerCore_GetField(core);
    nogrid->actor = FieldPlayerCore_GetActor(nogrid->core);
    if (GetActorMoveCode(nogrid->actor) != NOGRID_MOVE_CODE) {
        sPlayerTemplate.modelId = FldAct_GetObjCode(nogrid->actor);
        FldAct_Transplant(nogrid->actor, &sPlayerTemplate);
    }
    nogrid->railUnit = FldAct_GetRailUnit(nogrid->actor);
    {
        RailPosition pos = { 0 };

        SetActorInitedPositionRail(nogrid->actor, &pos);
    }
    func_ov036_0219e1f8(&nogrid->slide);

    switch (FieldPlayerCore_GetExState(nogrid->core)) {
    case 2:
        FieldPlayerCore_SetSpecialSeq(nogrid->core, 4);
        func_ov036_0219b350(nogrid->core);
        break;
    case 3:
        FieldPlayerCore_SetSpecialSeq(nogrid->core, 0x100);
        func_ov036_0219b350(nogrid->core);
        break;
    }
    return nogrid;
}

void func_ov036_0219cf40(FieldPlayerNoGrid *nogrid) {
    GFL_HeapFree(nogrid);
}

void func_ov036_0219cf48(FieldPlayerNoGrid *nogrid, const RailPosition *pos) {
    if (GetActorMoveCode(nogrid->actor) != NOGRID_MOVE_CODE) {
        sPlayerTemplate.modelId = FldAct_GetObjCode(nogrid->actor);
        FldAct_Transplant(nogrid->actor, &sPlayerTemplate);
    }
    nogrid->railUnit = FldAct_GetRailUnit(nogrid->actor);
    SetActorInitedPositionRail(nogrid->actor, pos);
}

void func_ov036_0219cf84(FieldPlayerNoGrid *nogrid) {
    nogrid->railUnit = NULL;
}

void FieldPlayerRail_UpdateMovement(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys) {
    u16 dir = FieldPlayerCore_GetMoveDirByKey(nogrid->core, heldKeys);

    if (FieldPlayerRail_CheckAllowUpdate(nogrid, dir)) {
        u32 command;

        func_ov036_0219b328(nogrid->core);
        func_ov036_0219e224(&nogrid->slide, dir, nogrid->actor);
        dir = func_ov036_0219de9c(nogrid, dir, 0);
        command = func_ov036_0219d094(nogrid, dir, pressedKeys, heldKeys, FALSE);
        func_ov036_0219d178(nogrid, heldKeys, command);
        func_ov036_0219d100(nogrid, command, dir, pressedKeys, heldKeys, FALSE);
        func_ov036_0219d1a8(nogrid);
        func_ov036_0219e2a0(&nogrid->slide);
    }
}

BOOL func_ov036_0219d008(FieldPlayerNoGrid *nogrid) {
    if (nogrid->state == NOGRID_STATE_BRAKE) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov036_0219d018(FieldPlayerNoGrid *nogrid) {
    return func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDING) != 0;
}

void FieldPlayerRail_SetRailPos(FieldPlayerNoGrid *nogrid, const RailPosition *pos) {
    SetActorInitedPositionRail(nogrid->actor, pos);
}

void FieldPlayerCore_GetRailPos(FieldPlayerNoGrid *nogrid, RailPosition *pos) {
    GetActorRailPos(nogrid->actor, pos);
}

void func_ov036_0219d044(FieldPlayerNoGrid *nogrid, u16 dir, RailPosition *pos) {
    GetActorRailPosForDir(nogrid->actor, dir, pos);
}

void FieldPlayerRail_GetWorldPos(FieldPlayerNoGrid *nogrid, VecFx32 *pos) {
    RailUnit_GetCalcPos(nogrid->railUnit, pos);
}

RailUnit *func_ov036_0219d05c(FieldPlayerNoGrid *nogrid) {
    return nogrid->railUnit;
}

void func_ov036_0219d060(FieldPlayerNoGrid *nogrid) {
    if (nogrid->state == NOGRID_STATE_BRAKE) {
        FieldActor *actor = nogrid->actor;
        u16 dir = GetActorFaceDir(actor);

        FldAct_FreeAcmd(actor);
        CheckSetActorFaceDir(actor, dir);
        FldAct_SetDrawStatus(actor, 0);
        FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    }
}

static u32 func_ov036_0219d094(FieldPlayerNoGrid *nogrid, u32 dir, u32 pressedKeys, u32 heldKeys, BOOL a4) {
    u32 command;

    switch (FieldPlayerCore_GetExState(nogrid->core)) {
    case 0:
        command = func_ov036_0219d2ac(nogrid, pressedKeys, heldKeys, dir, a4);
        break;
    case 1:
        command = func_ov036_0219d684(nogrid, pressedKeys, heldKeys, dir, a4);
        break;
    case 2:
    case 3:
        command = func_ov036_0219db24(nogrid, pressedKeys, heldKeys, dir, a4);
        break;
    }
    return command;
}

static void func_ov036_0219d100(FieldPlayerNoGrid *nogrid, u32 command, u32 dir, u32 pressedKeys, u32 heldKeys,
                                BOOL a5) {
    switch (FieldPlayerCore_GetExState(nogrid->core)) {
    case 0:
        func_ov036_0219d230(nogrid, command, pressedKeys, heldKeys, dir, a5);
        break;
    case 1:
        func_ov036_0219d608(nogrid, command, pressedKeys, heldKeys, dir, a5);
        break;
    case 2:
    case 3:
        func_ov036_0219d9cc(nogrid, command, pressedKeys, heldKeys, dir, a5);
        break;
    }
}

static void func_ov036_0219d178(FieldPlayerNoGrid *nogrid, u32 heldKeys, u32 command) {
    FieldPlayerCore_CalcSeparateKeyDirs(nogrid->core, heldKeys);
    func_ov036_0219dda8(nogrid);
    if (command == NOGRID_COMMAND_MOVE) {
        func_ov036_0219dd9c(nogrid);
    }
}

static u32 func_ov036_0219d198(FieldPlayerNoGrid *nogrid, FieldActor *actor, u16 dir, u32 *tileType) {
    return func_ov036_021956f4(actor, dir, tileType);
}

static void func_ov036_0219d1a8(FieldPlayerNoGrid *nogrid) {
    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_A)) {
        func_ov036_0219e178(&nogrid->sound, 0);
    } else if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_B)) {
        func_ov036_0219e178(&nogrid->sound, 1);
    } else {
        func_ov036_0219e1a0(&nogrid->sound);
    }
    func_ov036_0219e1c0(&nogrid->sound, nogrid->actor);
}

static BOOL FieldPlayerRail_CheckAllowUpdate(FieldPlayerNoGrid *nogrid, u16 dir) {
    FieldActor *actor = nogrid->actor;

    if (IsAllActorAcmdFinished(actor) == TRUE) {
        return TRUE;
    }
    if (dir != DIR_NONE && nogrid->state == NOGRID_STATE_BRAKE && dir != GetActorFaceDir(actor)) {
        return TRUE;
    }
    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_A | NOGRID_FLAG_SLIDE_B) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0219d230(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FieldActor *actor = nogrid->actor;

    switch (command) {
    case NOGRID_COMMAND_NONE:
        func_ov036_0219d510(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_IDLE:
        func_ov036_0219d514(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_MOVE:
        func_ov036_0219d54c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_TURN:
        func_ov036_0219d584(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_BRAKE:
        func_ov036_0219d5a8(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_JUMP:
        func_ov036_0219d5e4(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    }
}

static u32 func_ov036_0219d2ac(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir, BOOL a4) {
    u32 command = NOGRID_COMMAND_NONE;
    FieldActor *actor = nogrid->actor;

    switch (nogrid->state) {
    case NOGRID_STATE_IDLE:
        command = func_ov036_0219d314(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_MOVE:
        command = func_ov036_0219d374(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_TURN:
        command = func_ov036_0219d448(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_BRAKE:
        command = func_ov036_0219d498(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    }
    return command;
}

static u32 func_ov036_0219d314(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (dir != DIR_NONE) {
            if (dir != GetActorFaceDir(actor) && a5 == FALSE) {
                return NOGRID_COMMAND_TURN;
            }
            return func_ov036_0219d374(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        }
        return NOGRID_COMMAND_IDLE;
    }
    return NOGRID_COMMAND_NONE;
}

static u32 func_ov036_0219d374(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    u32 hit;

    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219d314(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }

    hit = func_ov036_0219d198(nogrid, actor, dir, NULL);
    if (a5 == TRUE && (hit & 2)) {
        hit = 0;
    }
    if (hit == 0) {
        return NOGRID_COMMAND_MOVE;
    }
    if (hit & 2) {
        RailPosition pos;
        u32 tile;
        u32 tileClass;
        u32 ledgeDir;

        GetActorRailPosForDir(actor, dir, &pos);
        tile = func_ov036_02194990(actor, &pos);
        ledgeDir = DIR_NONE;
        tileClass = GetTileClass(tile);
        GetTileFlags(tile);
        if (MapTile_IsLedgeU(tileClass)) {
            ledgeDir = DIR_UP;
        } else if (MapTile_IsLedgeD(tileClass)) {
            ledgeDir = DIR_DOWN;
        } else if (MapTile_IsLedgeL(tileClass)) {
            ledgeDir = DIR_LEFT;
        } else if (MapTile_IsLedgeR(tileClass)) {
            ledgeDir = DIR_RIGHT;
        }
        if (ledgeDir != DIR_NONE && ledgeDir == dir) {
            return NOGRID_COMMAND_JUMP;
        }
    }
    return NOGRID_COMMAND_BRAKE;
}

static u32 func_ov036_0219d448(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219d314(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    return func_ov036_0219d374(nogrid, actor, pressedKeys, heldKeys, dir, a5);
}

static u32 func_ov036_0219d498(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    u16 faceDir = GetActorFaceDir(actor);

    if (dir != DIR_NONE && dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return func_ov036_0219d374(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219d314(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    return func_ov036_0219d374(nogrid, actor, pressedKeys, heldKeys, dir, a5);
}

static void func_ov036_0219d510(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
}

static void func_ov036_0219d514(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    nogrid->state = NOGRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
}

static void func_ov036_0219d54c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    u16 acmd;

    if (a5 == TRUE) {
        acmd = ACMD_WALK_2F;
    } else if (heldKeys & PAD_BUTTON_B) {
        acmd = ACMD_DASH;
    } else {
        acmd = ACMD_WALK_8F_DASH;
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, acmd));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void func_ov036_0219d584(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_2F));
    nogrid->state = NOGRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_TURN);
}

static void func_ov036_0219d5a8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_16F));
    nogrid->state = NOGRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    if (!func_ov036_0219e2dc(nogrid, dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static void func_ov036_0219d5e4(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_JUMP_2));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void func_ov036_0219d608(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FieldActor *actor = nogrid->actor;

    switch (command) {
    case NOGRID_COMMAND_NONE:
        func_ov036_0219d8e8(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_IDLE:
        func_ov036_0219d8ec(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_MOVE:
        func_ov036_0219d924(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_TURN:
        func_ov036_0219d948(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_BRAKE:
        func_ov036_0219d96c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_JUMP:
        func_ov036_0219d9a8(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    }
}

static u32 func_ov036_0219d684(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir, BOOL a4) {
    u32 command = NOGRID_COMMAND_NONE;
    FieldActor *actor = nogrid->actor;

    switch (nogrid->state) {
    case NOGRID_STATE_IDLE:
        command = func_ov036_0219d6ec(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_MOVE:
        command = func_ov036_0219d74c(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_TURN:
        command = func_ov036_0219d820(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_BRAKE:
        command = func_ov036_0219d870(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    }
    return command;
}

static u32 func_ov036_0219d6ec(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (dir != DIR_NONE) {
            if (dir != GetActorFaceDir(actor) && a5 == FALSE) {
                return NOGRID_COMMAND_TURN;
            }
            return func_ov036_0219d74c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        }
        return NOGRID_COMMAND_IDLE;
    }
    return NOGRID_COMMAND_NONE;
}

static u32 func_ov036_0219d74c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    u32 hit;

    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219d6ec(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }

    hit = func_ov036_0219d198(nogrid, actor, dir, NULL);
    if (a5 == TRUE && (hit & 2)) {
        hit = 0;
    }
    if (hit == 0) {
        return NOGRID_COMMAND_MOVE;
    }
    if (hit & 2) {
        RailPosition pos;
        u32 tile;
        u32 tileClass;
        u32 ledgeDir;

        GetActorRailPosForDir(actor, dir, &pos);
        tile = func_ov036_02194990(actor, &pos);
        ledgeDir = DIR_NONE;
        tileClass = GetTileClass(tile);
        GetTileFlags(tile);
        if (MapTile_IsLedgeU(tileClass)) {
            ledgeDir = DIR_UP;
        } else if (MapTile_IsLedgeD(tileClass)) {
            ledgeDir = DIR_DOWN;
        } else if (MapTile_IsLedgeL(tileClass)) {
            ledgeDir = DIR_LEFT;
        } else if (MapTile_IsLedgeR(tileClass)) {
            ledgeDir = DIR_RIGHT;
        }
        if (ledgeDir != DIR_NONE && ledgeDir == dir) {
            return NOGRID_COMMAND_JUMP;
        }
    }
    return NOGRID_COMMAND_BRAKE;
}

static u32 func_ov036_0219d820(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219d6ec(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    return func_ov036_0219d74c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
}

static u32 func_ov036_0219d870(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    u16 faceDir = GetActorFaceDir(actor);

    if (dir != DIR_NONE && dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return func_ov036_0219d74c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219d6ec(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    return func_ov036_0219d74c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
}

static void func_ov036_0219d8e8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
}

static void func_ov036_0219d8ec(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    nogrid->state = NOGRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
}

static void func_ov036_0219d924(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_WALK_2F));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void func_ov036_0219d948(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_2F));
    nogrid->state = NOGRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_TURN);
}

static void func_ov036_0219d96c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_16F));
    nogrid->state = NOGRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    if (!func_ov036_0219e2dc(nogrid, dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static void func_ov036_0219d9a8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_JUMP_2));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void func_ov036_0219d9cc(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FieldActor *actor = FieldPlayerCore_GetActor(nogrid->core);

    switch (command) {
    case NOGRID_COMMAND_NONE:
        func_ov036_0219da5c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_IDLE:
        func_ov036_0219da60(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_MOVE:
        func_ov036_0219da98(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_TURN:
        func_ov036_0219dac4(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    case NOGRID_COMMAND_BRAKE:
        func_ov036_0219dae8(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        break;
    }
}

static void func_ov036_0219da5c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
}

static void func_ov036_0219da60(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    nogrid->state = NOGRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
}

static void func_ov036_0219da98(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    u16 acmd;

    if (a5 == TRUE) {
        acmd = ACMD_WALK_2F;
    } else {
        acmd = ACMD_WALK_4F;
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, acmd));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void func_ov036_0219dac4(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_2F));
    nogrid->state = NOGRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_TURN);
}

static void func_ov036_0219dae8(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                                BOOL a5) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_16F));
    nogrid->state = NOGRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    if (!func_ov036_0219e2dc(nogrid, dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static u32 func_ov036_0219db24(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir, BOOL a4) {
    FieldActor *actor = FieldPlayerCore_GetActor(nogrid->core);
    u32 command = NOGRID_COMMAND_NONE;

    switch (nogrid->state) {
    case NOGRID_STATE_IDLE:
        command = func_ov036_0219db9c(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_MOVE:
        command = func_ov036_0219dbfc(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_TURN:
        command = func_ov036_0219dc8c(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    case NOGRID_STATE_BRAKE:
        command = func_ov036_0219dcdc(nogrid, actor, pressedKeys, heldKeys, dir, a4);
        break;
    }
    return command;
}

static u32 func_ov036_0219db9c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (dir != DIR_NONE) {
            if (dir != GetActorFaceDir(actor) && a5 == FALSE) {
                return NOGRID_COMMAND_TURN;
            }
            return func_ov036_0219dbfc(nogrid, actor, pressedKeys, heldKeys, dir, a5);
        }
        return NOGRID_COMMAND_IDLE;
    }
    return NOGRID_COMMAND_NONE;
}

static u32 func_ov036_0219dbfc(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    u32 hit;
    u32 tile;

    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219db9c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }

    hit = func_ov036_0219d198(nogrid, actor, dir, &tile);
    if (a5 == TRUE && hit != 0 && !(hit & 0x10)) {
        hit = 0;
    }
    if (tile != -1) {
        u32 flags;

        GetTileClass(tile);
        flags = GetTileFlags(tile);
        if (hit == 0) {
            if (flags & 2) {
                return NOGRID_COMMAND_MOVE;
            }
            if (a5 == TRUE) {
                return NOGRID_COMMAND_MOVE;
            }
        }
    }
    return NOGRID_COMMAND_BRAKE;
}

static u32 func_ov036_0219dc8c(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219db9c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    return func_ov036_0219dbfc(nogrid, actor, pressedKeys, heldKeys, dir, a5);
}

static u32 func_ov036_0219dcdc(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys, u16 dir,
                               BOOL a5) {
    u16 faceDir = GetActorFaceDir(actor);

    if (dir != DIR_NONE && dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return func_ov036_0219dbfc(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return func_ov036_0219db9c(nogrid, actor, pressedKeys, heldKeys, dir, a5);
    }
    return func_ov036_0219dbfc(nogrid, actor, pressedKeys, heldKeys, dir, a5);
}

static void func_ov036_0219dd54(FieldPlayerNoGrid *nogrid, u32 flag) {
    nogrid->flags |= flag;
}

static void func_ov036_0219dd5c(FieldPlayerNoGrid *nogrid, u32 flag) {
    nogrid->flags &= ~flag;
}

static BOOL func_ov036_0219dd68(FieldPlayerNoGrid *nogrid, u32 flag) {
    if (nogrid->flags & flag) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0219dd78(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd54(nogrid, NOGRID_FLAG_SLIDING);
}

static void func_ov036_0219dd84(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_SLIDING);
}

static void func_ov036_0219dd90(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd54(nogrid, NOGRID_FLAG_SLIDE_END_SPOT);
}

static void func_ov036_0219dd9c(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_SLIDE_END_SPOT);
}

static void func_ov036_0219dda8(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_4);
}

static void func_ov036_0219ddb4(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd54(nogrid, NOGRID_FLAG_SLIDE_A);
}

static void func_ov036_0219ddc0(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_SLIDE_A);
}

static void func_ov036_0219ddcc(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd54(nogrid, NOGRID_FLAG_SLIDE_B);
}

static void func_ov036_0219ddd8(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_SLIDE_B);
}

static void func_ov036_0219dde4(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd54(nogrid, NOGRID_FLAG_SLIDE_C);
}

static void func_ov036_0219ddf0(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_SLIDE_C);
}

static void func_ov036_0219ddfc(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd54(nogrid, NOGRID_FLAG_SLIDE_PENDING);
}

static void func_ov036_0219de08(FieldPlayerNoGrid *nogrid) {
    func_ov036_0219dd5c(nogrid, NOGRID_FLAG_SLIDE_PENDING);
}

static u32 func_ov036_0219de14(FieldPlayerNoGrid *nogrid) {
    u32 tile = GetCachedTileUnderActor(nogrid->actor);
    u32 tileClass;

    if (!func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_END_SPOT)) {
        tileClass = GetTileClass(tile);
        if (IsTileSlidingIce(tileClass) == TRUE) {
            return 1;
        }
        if (func_ov036_021b3b64(tileClass) == TRUE) {
            return 2;
        }
        if (func_ov036_021b3b70(tileClass) == TRUE) {
            return 3;
        }
        if (func_ov036_021b3b7c(tileClass) == TRUE) {
            return 4;
        }
        if (func_ov036_021b3b88(tileClass) == TRUE) {
            return 5;
        }
        if (func_ov036_021b3b94(tileClass) == TRUE) {
            return 6;
        }
        if (func_ov036_021b3ba0(tileClass) == TRUE) {
            return 7;
        }
    }
    return 0;
}

static u16 func_ov036_0219de9c(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    u32 slide;

    if (!IsAllActorAcmdFinished(nogrid->actor)) {
        return dir;
    }
    if (a2 == 0 && (slide = func_ov036_0219de14(nogrid)) != 0) {
        return SLIDE_FUNCS[slide](nogrid, dir, a2);
    }
    func_ov036_0219dee0(nogrid);
    return dir;
}

static void func_ov036_0219dee0(FieldPlayerNoGrid *nogrid) {
    FieldActor *actor = nogrid->actor;

    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDING | NOGRID_FLAG_SLIDE_A | NOGRID_FLAG_SLIDE_B |
                                        NOGRID_FLAG_SLIDE_PENDING)) {
        ClearActorFlag(actor, 0x18);
    }
    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDING)) {
        func_ov036_0219dd84(nogrid);
        func_ov036_0219dd90(nogrid);
        FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    }
    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_A | NOGRID_FLAG_SLIDE_B)) {
        func_ov036_0219ddc0(nogrid);
        func_ov036_0219ddd8(nogrid);
        ChangeActorDirection(actor, GetInverseDirection(GetActorMotionDir(actor)));
    }
    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_C)) {
        func_ov036_0219ddf0(nogrid);
        ChangeActorDirection(actor, DIR_RIGHT);
    }
    if (func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_PENDING)) {
        func_ov036_0219de08(nogrid);
        func_ov036_0219e21c(&nogrid->slide);
    }
}

static u16 func_ov036_0219df78(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    FieldActor *actor = nogrid->actor;
    u16 motionDir = GetActorMotionDir(actor);

    if (func_ov036_0219d198(nogrid, actor, motionDir, NULL) != 0) {
        func_ov036_0219dee0(nogrid);
        return dir;
    }
    func_ov036_0219dd78(nogrid);
    func_ov036_0219ddc0(nogrid);
    func_ov036_0219ddd8(nogrid);
    CheckSetActorFaceDir(actor, motionDir);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
    return motionDir;
}

static u16 func_ov036_0219dfe4(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    FieldActor *actor = nogrid->actor;
    u16 motionDir = GetActorMotionDir(actor);

    if (func_ov036_0219d198(nogrid, actor, motionDir, NULL) != 0) {
        func_ov036_0219dee0(nogrid);
        return dir;
    }
    func_ov036_0219dd78(nogrid);
    func_ov036_0219ddb4(nogrid);
    func_ov036_0219ddd8(nogrid);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
    return motionDir;
}

static u16 func_ov036_0219e048(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    FieldActor *actor = nogrid->actor;
    u16 motionDir = GetActorMotionDir(actor);

    if (func_ov036_0219d198(nogrid, actor, motionDir, NULL) != 0) {
        func_ov036_0219dee0(nogrid);
        return dir;
    }
    func_ov036_0219dd78(nogrid);
    func_ov036_0219ddcc(nogrid);
    func_ov036_0219ddc0(nogrid);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
    return motionDir;
}

static u16 func_ov036_0219e0ac(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    if (GetActorMotionDir(nogrid->actor) == DIR_LEFT) {
        GFL_SndSEPlay(SEQ_SE_DANSA);
    }
    return func_ov036_0219df78(nogrid, dir, a2);
}

static u16 func_ov036_0219e0d4(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    if (GetActorMotionDir(nogrid->actor) == DIR_RIGHT) {
        GFL_SndSEPlay(SEQ_SE_DANSA);
    }
    return func_ov036_0219df78(nogrid, dir, a2);
}

static u16 func_ov036_0219e0fc(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    u16 result = func_ov036_0219df78(nogrid, dir, a2);

    func_ov036_0219dde4(nogrid);
    return result;
}

static u16 func_ov036_0219e110(FieldPlayerNoGrid *nogrid, u16 dir, u32 a2) {
    u16 result;

    if (!func_ov036_0219dd68(nogrid, NOGRID_FLAG_SLIDE_PENDING)) {
        func_ov036_0219ddfc(nogrid);
        func_ov036_0219e20c(&nogrid->slide);
    }
    result = func_ov036_0219e2bc(&nogrid->slide, dir);
    if (func_ov036_0219e2d4(&nogrid->slide)) {
        SetActorFlag(nogrid->actor, 0x18);
        FldAct_SetAcmd(nogrid->actor, GetAcmdForDir(result, ACMD_WALK_8F));
        FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
        result = DIR_NONE;
    } else {
        ClearActorFlag(nogrid->actor, 0x18);
    }
    return result;
}

static void func_ov036_0219e178(StepSound *sound, u32 kind) {
    if (sound->active != 1 || kind != sound->kind) {
        GFL_SndSEPlay(SEQ_SE_FLD_66);
        sound->active = 1;
        sound->kind = kind;
        sound->step = 0;
    }
}

static void func_ov036_0219e1a0(StepSound *sound) {
    if (sound->active != 0) {
        sound->active = 0;
        sound->step = 0;
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_66));
    }
}

static void func_ov036_0219e1c0(StepSound *sound, FieldActor *actor) {
    if (sound->active != 0) {
        sound->step = (sound->step + 1) % 4;
        SetActorFaceDir(actor, SPIN_FACE_DIRS[sound->kind][sound->step]);
    }
}

static void func_ov036_0219e1f8(SlideState *slide) {
    MI_CpuFill8(slide, 0, sizeof(SlideState));
    slide->dir = DIR_NONE;
}

static void func_ov036_0219e20c(SlideState *slide) {
    slide->active = 1;
}

static void func_ov036_0219e21c(SlideState *slide) {
    func_ov036_0219e1f8(slide);
}

static void func_ov036_0219e224(SlideState *slide, u16 dir, FieldActor *actor) {
    if (slide->active) {
        if (slide->mode != 0) {
            if (dir != DIR_NONE) {
                slide->time = 0;
                slide->mode = 0;
                if (slide->held) {
                    slide->dir = dir;
                }
            }
        } else if (dir == DIR_NONE) {
            if (slide->time < 30) {
                slide->time++;
            } else {
                slide->mode = 1;
            }
        }
        if (IsAllActorAcmdFinished(actor) == TRUE) {
            if (slide->mode == 0) {
                slide->held = 0;
                slide->pending = 0;
            } else {
                slide->held = 1;
                slide->pending = 1;
            }
        }
    }
}

static void func_ov036_0219e2a0(SlideState *slide) {
    if (slide->pending) {
        slide->pending = 0;
    }
    if (!slide->held) {
        slide->dir = DIR_NONE;
    }
}

static u32 func_ov036_0219e2bc(SlideState *slide, u16 dir) {
    u32 ret;

    if (slide->pending) {
        return 1;
    }
    ret = slide->dir;
    if (ret == DIR_NONE) {
        ret = dir;
    }
    return ret;
}

static u32 func_ov036_0219e2d4(SlideState *slide) {
    return slide->held;
}

static BOOL func_ov036_0219e2dc(FieldPlayerNoGrid *nogrid, u16 dir) {
    FieldActor *actor = FieldPlayerCore_GetActor(nogrid->core);
    EventData *eventData = GameData_GetEventData(GSYS_GetGameData(Field_GetGameSystem(nogrid->field)));
    RailPosition pos;

    GetActorRailPosForDir(actor, dir, &pos);
    if (GetWarpIDByPlayerPosRail(eventData, &pos) != WARP_ID_NONE) {
        return TRUE;
    }
    return FALSE;
}
