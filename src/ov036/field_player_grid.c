// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
#include "field/event_data.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_player_grid.h"
#include "field/gym_gimmick.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/season.h"

// The field player's movement on the grid: each frame the held direction and the tile ahead decide a command, by
// whether the player walks, cycles or surfs, and the command sets the actor's next animation command

// What the player is doing, which the next command depends on
enum {
    GRID_STATE_IDLE,
    GRID_STATE_MOVE,
    GRID_STATE_TURN,
    GRID_STATE_BRAKE,
    GRID_STATE_CATWALK_BALANCE,
    GRID_STATE_CATWALK_EXIT,
    GRID_STATE_CATWALK_EXIT_WAIT,
    GRID_STATE_FALL,
};

// What the player does next
enum {
    GRID_COMMAND_NONE,
    GRID_COMMAND_IDLE,
    GRID_COMMAND_MOVE,
    GRID_COMMAND_TURN,
    GRID_COMMAND_BRAKE,
    GRID_COMMAND_JUMP,
    GRID_COMMAND_CATWALK_BALANCE,
    GRID_COMMAND_CATWALK_EXIT,
    GRID_COMMAND_CATWALK_EXIT_WAIT,
    GRID_COMMAND_FALL,
};

#define GRID_FLAG_CONST_MOVE 0x1
// The slide ended on this tile, so it doesn't start again until the player moves
#define GRID_FLAG_CONST_MOVE_END_SPOT 0x2
#define GRID_FLAG_4 0x4
#define GRID_FLAG_CATWALK_BALANCING 0x8
#define GRID_FLAG_CATWALK_EXITING 0x10
#define GRID_FLAG_RUNNING 0x20

// What PlayerMoveCollCheckCatwalk adds to ActorRouteCollCheck's result: the tile ahead is a catwalk
#define COLL_CATWALK 0x100

enum {
    CONST_MOVE_NONE,
    CONST_MOVE_STRAIGHT,
};

// How a step's sound repeats
enum {
    SFX_MODE_WALK,
    SFX_MODE_RUN,
    SFX_MODE_BIKE,
};

#define SEASON_ANY 4

#define GRID_SIZE FX32_CONST(16)
#define HEIGHT_TO_GRID(height) ((s16)(((height) >> 4) / FX32_ONE))

// How far the player falls from a catwalk each frame
#define FALL_SPEED FX32_CONST(8)

// How long the player balances on a catwalk before falling off
#define CATWALK_BALANCE_TIME 60

#define KEY_B 0x2

struct FieldPlayerGrid {
    u32 state;
    u32 command;
    u32 flags;
    FieldPlayerCore *core;
    Field *field;
    u16 balanceTime;
    u8 balanceKept;
    u16 stepSE;
    u16 stepCount;
    BOOL verticalMoveOnly;
};

typedef struct {
    u32 pressedKeys;
    u32 heldKeys;
    u16 dir;
    u32 flags;
    // Moves through everything but the map's edge. Never set
    BOOL force;
} GridMoveInput;

// A step's sound by tile flag, and after how many steps it repeats when running and cycling
typedef struct {
    u32 flag;
    u32 se;
    s16 runInterval;
    s16 bikeInterval;
} TileFlagSFX;

// The same by tile class and season
typedef struct {
    BOOL (*check)(u32 tileClass);
    u16 se;
    s8 runInterval;
    s8 bikeInterval;
    u8 season;
} TileClassSFX;

typedef BOOL (*ConstMoveFunc)(FieldPlayerGrid *grid, u16 dir);

static void FieldPlayerGrid_SetCommonStatus(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input);
static BOOL FieldPlayerGrid_CheckAllowUpdate(FieldPlayerGrid *grid, u16 dir);
static BOOL FieldPlayerGrid_PlaySFXByTile(FieldPlayerGrid *grid, u32 command, const VecFx32 *pos, u32 layer,
                                          u32 season);
static void FieldPlayerGrid_UpdateTileSFX(FieldPlayerGrid *grid, u32 command, u16 dir);
static u32 FieldPlayerGrid_DecideCommand(FieldPlayerGrid *grid, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandNormal(FieldPlayerGrid *grid, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandNormalIdle(FieldPlayerGrid *grid, FieldActor *actor,
                                                   const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandNormalMoving(FieldPlayerGrid *grid, FieldActor *actor,
                                                     const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandNormalTurning(FieldPlayerGrid *grid, FieldActor *actor,
                                                      const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandNormalBraking(FieldPlayerGrid *grid, FieldActor *actor,
                                                      const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandCatwalk(FieldPlayerGrid *grid, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandCatwalkIdle(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandCatwalkMove(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandCatwalkBalance(FieldPlayerGrid *grid, FieldActor *actor,
                                                       const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandCatwalkExit(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandFall(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandBike(FieldPlayerGrid *grid, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandBikeIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static BOOL MapTile_IsBlocksCycling2(u16 tileClass);
static u32 FieldPlayerGrid_DecideCommandBikeMoving(FieldPlayerGrid *grid, FieldActor *actor,
                                                   const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandBikeTurning(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandBikeBraking(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandSurf(FieldPlayerGrid *grid, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandSurfIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandSurfMoving(FieldPlayerGrid *grid, FieldActor *actor,
                                                   const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandSurfTurning(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static u32 FieldPlayerGrid_DecideCommandSurfBraking(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static void FieldPlayerGrid_UpdateCore(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormal(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalBusy(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalMove(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalTurn(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalBrake(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalJump(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalCatwalkBalance(FieldPlayerGrid *grid, FieldActor *actor,
                                                       const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalCatwalkExit(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalCatwalkExitWait(FieldPlayerGrid *grid, FieldActor *actor,
                                                        const GridMoveInput *input);
static void FieldPlayerGrid_UpdateNormalFall(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBike(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBikeBusy(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBikeIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBikeMove(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBikeTurn(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBikeBrake(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateBikeJump(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateSurf(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateSurfBusy(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateSurfIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateSurfMove(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateSurfTurn(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static void FieldPlayerGrid_UpdateSurfBrake(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input);
static u32 PlayerMoveCollCheck(FieldPlayerGrid *grid, FieldActor *actor, u16 dir, u32 *tileType);
static void FieldPlayerGrid_SetFlag(FieldPlayerGrid *grid, u32 flag);
static void FieldPlayerGrid_ClearFlag(FieldPlayerGrid *grid, u32 flag);
static BOOL FieldPlayerGrid_CheckFlag(FieldPlayerGrid *grid, u32 flag);
static void FieldPlayerGrid_SetConstMoveFlag(FieldPlayerGrid *grid);
static void FieldPlayerGrid_ClearConstMoveFlag(FieldPlayerGrid *grid);
static BOOL FieldPlayerGrid_CheckConstMoveFlag(FieldPlayerGrid *grid);
static void FieldPlayerGrid_SetConstMoveEndSpotFlag(FieldPlayerGrid *grid);
static void FieldPlayerGrid_ClearConstMoveEndSpotFlag(FieldPlayerGrid *grid);
static void FieldPlayerGrid_ClearFlag4(FieldPlayerGrid *grid);
static void FieldPlayerGrid_SetCatwalkBalancingFlag(FieldPlayerGrid *grid, BOOL set);
static BOOL FieldPlayerGrid_CheckCatwalkBalancingFlag(FieldPlayerGrid *grid);
static void FieldPlayerGrid_SetCatwalkExitingFlag(FieldPlayerGrid *grid, BOOL set);
static BOOL FieldPlayerGrid_CheckCatwalkExitingFlag(FieldPlayerGrid *grid);
static u32 FieldPlayerGrid_GetConstMoveType(FieldPlayerGrid *grid, u16 dir);
static void FieldPlayerGrid_ClearConstMove(FieldPlayerGrid *grid);
static BOOL FieldPlayerGrid_ConstMoveUpdate_NONE(FieldPlayerGrid *grid, u16 dir);
static BOOL FieldPlayerGrid_ConstMoveUpdate_STRAIGHT(FieldPlayerGrid *grid, u16 dir);
static BOOL FieldPlayerGrid_ConstMoveUpdate(FieldPlayerGrid *grid, u16 dir, BOOL disabled);
static BOOL MapTile_IsCatwalk(u32 tileClass);
static BOOL MapTile_IsCatwalkBody(u32 tileClass);
static BOOL FieldPlayerGrid_CheckOnCatwalk(FieldPlayerGrid *grid);
static u16 FieldPlayerGrid_CalcCatwalkFallDir(FieldPlayerGrid *grid);
static u32 PlayerMoveCollCheckCatwalk(FieldPlayerGrid *grid, FieldActor *actor, u16 dir, u32 *tileType);
static BOOL FieldPlayerGrid_CheckModelHeightMismatch(FieldPlayerGrid *grid);
static void FieldPlayerGrid_BeginCatwalkBalance(FieldPlayerGrid *grid);
static void FieldPlayerGrid_ResetCatwalkBalance(FieldPlayerGrid *grid);
static BOOL FieldPlayerGrid_GetTileInDir(FieldPlayerGrid *grid, u16 dir, u32 *tileType);
static u32 FieldPlayerGrid_CompareHeightChange(FieldPlayerGrid *grid, u16 dir);
static BOOL FieldPlayerGrid_CheckWarpAtHitWall(FieldPlayerGrid *grid, u16 dir);

// The sounds of steps by the tile's flags and by its class: of the tile ahead (LAY1), and of the player's own
// (LAY0), which come first and are none. Each list ends with an empty entry
static const TileFlagSFX TILE_FLAG_SFX_LAY1[] = {
    { 0x20, SEQ_SE_FLD_09, 2, 4 },
    { 0, SEQ_SE_DUMMY, 0, 0 },
};

static const TileFlagSFX TILE_FLAG_SFX_LAY0[] = {
    { 0, SEQ_SE_DUMMY, 0, 0 },
};

static const TileClassSFX TILE_CLASS_SFX_LAY0[] = {
    { NULL, 0, 0, 0, SEASON_ANY },
};

// clang-format off
static const TileClassSFX TILE_CLASS_SFX_LAY1[] = {
    {(BOOL (*)(u32))MapTile_IsReallyTallGrass, SEQ_SE_FLD_08, 2, 4, SEASON_ANY},
    {MapTile_IsSnow_, SEQ_SE_FLD_11, 2, 4, SEASON_ANY},
    {MapTile_IsSnowNoCycling, SEQ_SE_FLD_11, 2, 4, SEASON_ANY},
    {MapTile_IsSplashWater, SEQ_SE_FLD_13, 2, 4, SEASON_ANY},
    {MapTile_IsRippleWater, SEQ_SE_FLD_13, 2, 4, SEASON_ANY},
    {MapTile_IsSwamp, SEQ_SE_FLD_13, 2, 4, SEASON_ANY},
    {MapTile_IsDeepSand, SEQ_SE_FLD_91, 2, 4, SEASON_ANY},
    {MapTile_IsAnySand, SEQ_SE_FLD_14, 2, 4, SEASON_ANY},
    {MapTile_IsPuddle, SEQ_SE_FLD_11, 2, 4, 3},
    {MapTile_IsMud, SEQ_SE_FLD_11, 2, 4, 3},
    {MapTile_IsHiddenGrottoEntranceGrass, SEQ_SE_FLD_08, 2, 4, SEASON_ANY},
    {MapTile_IsFallenLeaves, SEQ_SE_FLD_12, 2, 4, 2},
    {NULL, 0, 0, 0, SEASON_ANY},
};

// clang-format on

FieldPlayerGrid *FieldPlayerGrid_Create(FieldPlayerCore *core, HeapID heapId) {
    FieldPlayerGrid *grid = GFL_HeapAllocate(heapId, sizeof(FieldPlayerGrid), TRUE, "field_player_grid.c", 384);

    grid->core = core;
    grid->field = FieldPlayerCore_GetField(core);
    grid->stepSE = SEQ_SE_DUMMY;
    FieldPlayerGrid_SetConstMoveEndSpotFlag(grid);
    switch (FieldPlayerCore_GetExState(grid->core)) {
    case FLD_PLAYER_EXSTATE_SURF:
        FieldPlayerCore_SetSpecialSeq(grid->core, 4);
        FieldPlayerCore_FinishFormChange(grid->core);
        break;
    case FLD_PLAYER_EXSTATE_DIVE:
        FieldPlayerCore_SetSpecialSeq(grid->core, 0x100);
        FieldPlayerCore_FinishFormChange(grid->core);
        break;
    }
    return grid;
}

void FieldPlayerGrid_Free(FieldPlayerGrid *grid) {
    GFL_HeapFree(grid);
}

void FieldPlayerGrid_UpdateMovement(FieldPlayerGrid *grid, u32 pressedKeys, u32 heldKeys, u32 flags) {
    GridMoveInput input;
    u32 command;
    u16 dir = FieldPlayerCore_GetMoveDirByKey(grid->core, heldKeys);

    if (FieldPlayerGrid_CheckAllowUpdate(grid, dir) == TRUE) {
        FieldPlayerCore_StepFormChange(grid->core);
        if (!FieldPlayerGrid_ConstMoveUpdate(grid, dir, FALSE)) {
            input.pressedKeys = pressedKeys;
            input.heldKeys = heldKeys;
            input.dir = dir;
            input.flags = flags;
            input.force = FALSE;
            command = FieldPlayerGrid_DecideCommand(grid, &input);
            FieldPlayerGrid_SetCommonStatus(grid, command, &input);
            FieldPlayerGrid_UpdateCore(grid, command, &input);
            FieldPlayerGrid_UpdateTileSFX(grid, command, dir);
        }
    }
}

static void FieldPlayerGrid_SetCommonStatus(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input) {
    grid->command = command;
    FieldPlayerCore_CalcSeparateKeyDirs(grid->core, input->heldKeys);
    FieldPlayerGrid_ClearFlag4(grid);
    FieldPlayerGrid_ClearFlag(grid, GRID_FLAG_RUNNING);
    if (command == GRID_COMMAND_MOVE) {
        FieldPlayerGrid_ClearConstMoveEndSpotFlag(grid);
    }
    grid->verticalMoveOnly = FALSE;
}

// Whether the player can take a command: the last one is done, or they are walking into a wall and another direction
// is held
static BOOL FieldPlayerGrid_CheckAllowUpdate(FieldPlayerGrid *grid, u16 dir) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    if (IsAllActorAcmdFinished(actor) == TRUE) {
        return TRUE;
    }
    if (dir != PLAYER_DIR_NONE && grid->state == GRID_STATE_BRAKE && dir != GetActorFaceDir(actor)) {
        return TRUE;
    }
    return FALSE;
}

// Plays the sound of a step on the tile at a position, every step when walking and at the sound's interval otherwise.
// FALSE if the tile has none
static BOOL FieldPlayerGrid_PlaySFXByTile(FieldPlayerGrid *grid, u32 command, const VecFx32 *pos, u32 layer,
                                          u32 season) {
    u32 tileType;
    int interval;
    u32 se;
    int mode;

    se = SEQ_SE_DUMMY;
    interval = 0;
    mode = SFX_MODE_WALK;
    if (FieldPlayerCore_GetExState(grid->core) == FLD_PLAYER_EXSTATE_CYCLING) {
        mode = SFX_MODE_BIKE;
    } else if (FieldPlayerGrid_CheckFlag(grid, GRID_FLAG_RUNNING)) {
        mode = SFX_MODE_RUN;
    }
    if (mode == SFX_MODE_WALK) {
        grid->stepSE = SEQ_SE_DUMMY;
        grid->stepCount = 0;
    }
    if (GetTileTypeAtPosByActor(FieldPlayerCore_GetActor(grid->core), pos, &tileType) == TRUE) {
        u32 tileFlags = GetTileFlags(tileType);
        const TileFlagSFX *flagSFX;
        const TileClassSFX *classSFX;

        if (layer == 0) {
            flagSFX = TILE_FLAG_SFX_LAY0;
        } else {
            flagSFX = TILE_FLAG_SFX_LAY1;
        }
        for (; flagSFX->flag != 0; flagSFX++) {
            if (flagSFX->flag & tileFlags) {
                se = flagSFX->se;
                if (mode == SFX_MODE_RUN) {
                    interval = flagSFX->runInterval;
                } else if (mode == SFX_MODE_BIKE) {
                    interval = flagSFX->bikeInterval;
                }
                break;
            }
        }
        if (se == SEQ_SE_DUMMY) {
            classSFX = TILE_CLASS_SFX_LAY0;
            if (layer != 0) {
                classSFX = TILE_CLASS_SFX_LAY1;
            }
            for (; classSFX->check != NULL; classSFX++) {
                // The class is the low half of the tile type
                if (classSFX->check((u16)tileType) == TRUE &&
                    (classSFX->season == SEASON_ANY || classSFX->season == season)) {
                    se = classSFX->se;
                    if (mode == SFX_MODE_RUN) {
                        interval = classSFX->runInterval;
                    } else if (mode == SFX_MODE_BIKE) {
                        interval = classSFX->bikeInterval;
                    }
                    break;
                }
            }
        }
        if (se == SEQ_SE_DUMMY) {
            if (layer == 1) {
                grid->stepCount = 0;
            }
        } else if (mode != SFX_MODE_WALK) {
            if (grid->stepSE != se) {
                grid->stepCount = 0;
            }
            if (grid->stepCount != 0 && grid->stepCount < interval) {
                se = SEQ_SE_DUMMY;
            }
            if (grid->stepCount >= interval) {
                grid->stepCount = 0;
            }
            grid->stepSE = se;
            grid->stepCount++;
        }
        if (se != SEQ_SE_DUMMY) {
            GFL_SndSEPlay(se);
            return TRUE;
        }
    }
    return FALSE;
}

static void FieldPlayerGrid_UpdateTileSFX(FieldPlayerGrid *grid, u32 command, u16 dir) {
    VecFx32 pos;
    FieldActor *actor;
    u32 season;

    if (command != GRID_COMMAND_MOVE) {
        grid->stepSE = SEQ_SE_DUMMY;
        grid->stepCount = 0;
        return;
    }
    actor = FieldPlayerCore_GetActor(grid->core);
    season = GameSystem_GetSeason(Field_GetGameSystem(grid->field));
    CopyActorWPos(actor, &pos);
    if (!FieldPlayerGrid_PlaySFXByTile(grid, command, &pos, 0, season)) {
        ExpandVecInGridDir(dir, &pos, GRID_SIZE);
        FieldPlayerGrid_PlaySFXByTile(grid, command, &pos, 1, season);
    }
}

static u32 FieldPlayerGrid_DecideCommand(FieldPlayerGrid *grid, const GridMoveInput *input) {
    u32 command = GRID_COMMAND_NONE;

    switch (FieldPlayerCore_GetExState(grid->core)) {
    case FLD_PLAYER_EXSTATE_NONE:
        command = FieldPlayerGrid_DecideCommandNormal(grid, input);
        break;
    case FLD_PLAYER_EXSTATE_CYCLING:
        command = FieldPlayerGrid_DecideCommandBike(grid, input);
        break;
    case FLD_PLAYER_EXSTATE_SURF:
    case FLD_PLAYER_EXSTATE_DIVE:
        command = FieldPlayerGrid_DecideCommandSurf(grid, input);
        break;
    }
    return command;
}

u32 FieldPlayerGrid_GetMoveDirContType(FieldPlayerGrid *grid, u16 dir) {
    u16 faceDir = GetActorFaceDir(FieldPlayerCore_GetActor(grid->core));

    if (dir == PLAYER_DIR_NONE) {
        return MOVE_DIR_CONT_NONE;
    }
    if (dir != faceDir && grid->state != GRID_STATE_MOVE && grid->state != GRID_STATE_BRAKE) {
        return MOVE_DIR_CONT_TURN;
    }
    return MOVE_DIR_CONT_MOVE;
}

BOOL FieldPlayerGrid_IsAcmdFinished(FieldPlayerGrid *grid, u16 dir) {
    return FieldPlayerCore_IsAcmdFinished(grid->core);
}

static u32 FieldPlayerGrid_DecideCommandNormal(FieldPlayerGrid *grid, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
    u32 command = GRID_COMMAND_NONE;

    if (FieldPlayerGrid_CheckOnCatwalk(grid) == TRUE || FieldPlayerGrid_CheckCatwalkExitingFlag(grid) == TRUE) {
        return FieldPlayerGrid_DecideCommandCatwalk(grid, input);
    }
    switch (grid->state) {
    case GRID_STATE_IDLE:
        command = FieldPlayerGrid_DecideCommandNormalIdle(grid, actor, input);
        break;
    case GRID_STATE_MOVE:
        command = FieldPlayerGrid_DecideCommandNormalMoving(grid, actor, input);
        break;
    case GRID_STATE_TURN:
        command = FieldPlayerGrid_DecideCommandNormalTurning(grid, actor, input);
        break;
    case GRID_STATE_BRAKE:
        command = FieldPlayerGrid_DecideCommandNormalBraking(grid, actor, input);
        break;
    }
    return command;
}

static u32 FieldPlayerGrid_DecideCommandNormalIdle(FieldPlayerGrid *grid, FieldActor *actor,
                                                   const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (input->dir != PLAYER_DIR_NONE) {
            // Running turns without stopping
            if (input->dir != GetActorFaceDir(actor) && input->force == FALSE) {
                if (!(input->flags & MOVE_FLAG_CAN_RUN)) {
                    return GRID_COMMAND_TURN;
                }
                if (!(input->heldKeys & KEY_B)) {
                    return GRID_COMMAND_TURN;
                }
            }
            return FieldPlayerGrid_DecideCommandNormalMoving(grid, actor, input);
        }
        return GRID_COMMAND_IDLE;
    }
    return GRID_COMMAND_NONE;
}

// Walking and running: a step if nothing at all is in the way, a jump over a ledge that faces the same way, and
// otherwise a bump into the wall
static u32 FieldPlayerGrid_DecideCommandNormalMoving(FieldPlayerGrid *grid, FieldActor *actor,
                                                     const GridMoveInput *input) {
    u32 tileType;
    u32 tileFlags;
    u32 hit;
    u16 tileClass;
    u16 ledgeDir;

    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandNormalIdle(grid, actor, input);
    }
    hit = PlayerMoveCollCheck(grid, actor, input->dir, &tileType);
    if (input->force == TRUE && hit != 0 && !(hit & 0x10)) {
        hit = 0;
    }
    if (tileType != TILE_TYPE_NONE) {
        tileClass = GetTileClass(tileType);
        tileFlags = GetTileFlags(tileType);
        if (hit & 2) {
            ledgeDir = PLAYER_DIR_NONE;
            if (MapTile_IsLedgeU(tileClass)) {
                ledgeDir = DIR_UP;
            } else if (MapTile_IsLedgeD(tileClass)) {
                ledgeDir = DIR_DOWN;
            } else if (MapTile_IsLedgeL(tileClass)) {
                ledgeDir = DIR_LEFT;
            } else if (MapTile_IsLedgeR(tileClass)) {
                ledgeDir = DIR_RIGHT;
            }
            if (ledgeDir != PLAYER_DIR_NONE && ledgeDir == input->dir) {
                return GRID_COMMAND_JUMP;
            }
        }
        if (hit == 0) {
            if (!(tileFlags & 2)) {
                return GRID_COMMAND_MOVE;
            }
            if (input->force == TRUE) {
                return GRID_COMMAND_MOVE;
            }
        }
    }
    return GRID_COMMAND_BRAKE;
}

static u32 FieldPlayerGrid_DecideCommandNormalTurning(FieldPlayerGrid *grid, FieldActor *actor,
                                                      const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandNormalIdle(grid, actor, input);
    }
    return FieldPlayerGrid_DecideCommandNormalMoving(grid, actor, input);
}

static u32 FieldPlayerGrid_DecideCommandNormalBraking(FieldPlayerGrid *grid, FieldActor *actor,
                                                      const GridMoveInput *input) {
    u16 faceDir = GetActorFaceDir(actor);

    if (input->dir != PLAYER_DIR_NONE && input->dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return FieldPlayerGrid_DecideCommandNormalMoving(grid, actor, input);
    }
    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandNormalIdle(grid, actor, input);
    }
    return FieldPlayerGrid_DecideCommandNormalMoving(grid, actor, input);
}

static u32 FieldPlayerGrid_DecideCommandCatwalk(FieldPlayerGrid *grid, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
    u32 command = GRID_COMMAND_NONE;

    switch (grid->state) {
    case GRID_STATE_IDLE:
        command = FieldPlayerGrid_DecideCommandCatwalkIdle(grid, actor, input);
        break;
    case GRID_STATE_MOVE:
        command = FieldPlayerGrid_DecideCommandCatwalkMove(grid, actor, input);
        break;
    case GRID_STATE_TURN:
        command = FieldPlayerGrid_DecideCommandNormalTurning(grid, actor, input);
        break;
    case GRID_STATE_BRAKE:
        command = FieldPlayerGrid_DecideCommandNormalBraking(grid, actor, input);
        break;
    case GRID_STATE_CATWALK_BALANCE:
        command = FieldPlayerGrid_DecideCommandCatwalkBalance(grid, actor, input);
        break;
    case GRID_STATE_CATWALK_EXIT:
    case GRID_STATE_CATWALK_EXIT_WAIT:
        command = FieldPlayerGrid_DecideCommandCatwalkExit(grid, actor, input);
        break;
    case GRID_STATE_FALL:
        command = FieldPlayerGrid_DecideCommandFall(grid, actor, input);
        break;
    }
    return command;
}

static u32 FieldPlayerGrid_DecideCommandCatwalkIdle(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (input->dir != PLAYER_DIR_NONE) {
            if (input->dir != GetActorFaceDir(actor) && input->force == FALSE) {
                return GRID_COMMAND_TURN;
            }
            return FieldPlayerGrid_DecideCommandCatwalkMove(grid, actor, input);
        }
        return FieldPlayerGrid_DecideCommandCatwalkBalance(grid, actor, input);
    }
    return GRID_COMMAND_NONE;
}

// On a catwalk: a step onto more catwalk if nothing is in the way, and a fall off it onto any tile that isn't a wall,
// whatever else ActorRouteCollCheck found there
static u32 FieldPlayerGrid_DecideCommandCatwalkMove(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    u32 tileType;
    u32 hit;

    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandCatwalkIdle(grid, actor, input);
    }
    hit = PlayerMoveCollCheckCatwalk(grid, actor, input->dir, &tileType);
    if (tileType != TILE_TYPE_NONE) {
        if (hit & COLL_CATWALK) {
            if (!(hit & ~COLL_CATWALK)) {
                return GRID_COMMAND_MOVE;
            }
        } else if (!MapTile_BlocksCollision(tileType)) {
            FieldPlayerGrid_SetCatwalkExitingFlag(grid, TRUE);
            return GRID_COMMAND_CATWALK_EXIT;
        }
    }
    return GRID_COMMAND_BRAKE;
}

static u32 FieldPlayerGrid_DecideCommandCatwalkBalance(FieldPlayerGrid *grid, FieldActor *actor,
                                                       const GridMoveInput *input) {
    if (input->dir == PLAYER_DIR_NONE) {
        if (!FieldPlayerGrid_CheckCatwalkBalancingFlag(grid)) {
            FieldPlayerGrid_BeginCatwalkBalance(grid);
        }
        if (grid->balanceKept == TRUE && RailActorAnim_SyncFrameGroup(actor, grid->balanceTime)) {
            grid->balanceKept = FALSE;
        }
        grid->balanceTime++;
        if (grid->balanceTime == CATWALK_BALANCE_TIME) {
            FieldPlayerGrid_ResetCatwalkBalance(grid);
            FieldPlayerGrid_SetCatwalkExitingFlag(grid, TRUE);
            return GRID_COMMAND_CATWALK_EXIT;
        }
    } else {
        FieldPlayerGrid_ResetCatwalkBalance(grid);
        return FieldPlayerGrid_DecideCommandCatwalkMove(grid, actor, input);
    }
    return GRID_COMMAND_CATWALK_BALANCE;
}

static u32 FieldPlayerGrid_DecideCommandCatwalkExit(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        return GRID_COMMAND_FALL;
    }
    return GRID_COMMAND_CATWALK_EXIT_WAIT;
}

static u32 FieldPlayerGrid_DecideCommandFall(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    if (FieldPlayerGrid_CheckModelHeightMismatch(grid) == TRUE) {
        FieldPlayerGrid_SanitizeHeightMismatch(grid);
        FieldPlayerGrid_SetCatwalkExitingFlag(grid, FALSE);
        return GRID_COMMAND_IDLE;
    }
    return GRID_COMMAND_FALL;
}

static u32 FieldPlayerGrid_DecideCommandBike(FieldPlayerGrid *grid, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
    u32 command = GRID_COMMAND_NONE;

    switch (grid->state) {
    case GRID_STATE_IDLE:
        command = FieldPlayerGrid_DecideCommandBikeIdle(grid, actor, input);
        break;
    case GRID_STATE_MOVE:
        command = FieldPlayerGrid_DecideCommandBikeMoving(grid, actor, input);
        break;
    case GRID_STATE_TURN:
        command = FieldPlayerGrid_DecideCommandBikeTurning(grid, actor, input);
        break;
    case GRID_STATE_BRAKE:
        command = FieldPlayerGrid_DecideCommandBikeBraking(grid, actor, input);
        break;
    }
    return command;
}

static u32 FieldPlayerGrid_DecideCommandBikeIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (input->dir != PLAYER_DIR_NONE) {
            if (input->dir != GetActorFaceDir(actor) && input->force == FALSE) {
                return GRID_COMMAND_TURN;
            }
            return FieldPlayerGrid_DecideCommandBikeMoving(grid, actor, input);
        }
        return GRID_COMMAND_IDLE;
    }
    return GRID_COMMAND_NONE;
}

static BOOL MapTile_IsBlocksCycling2(u16 tileClass) {
    if (MapTile_IsReallyTallGrass(tileClass) || MapTile_IsCatwalkBody_(tileClass) ||
        MapTile_IsCatwalkEntryPoint(tileClass) || MapTile_IsSnowNoCycling(tileClass) ||
        MapTile_IsHiddenGrottoEntranceGrass(tileClass)) {
        return TRUE;
    }
    return FALSE;
}

// Cycling: as walking, and not onto the tiles that a bicycle can't cross
static u32 FieldPlayerGrid_DecideCommandBikeMoving(FieldPlayerGrid *grid, FieldActor *actor,
                                                   const GridMoveInput *input) {
    u32 tileType;
    u32 tileFlags;
    u32 hit;
    u16 tileClass;
    u16 ledgeDir;

    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandBikeIdle(grid, actor, input);
    }
    hit = PlayerMoveCollCheck(grid, actor, input->dir, &tileType);
    if (input->force == TRUE && hit != 0 && !(hit & 0x10)) {
        hit = 0;
    }
    if (tileType != TILE_TYPE_NONE) {
        tileClass = GetTileClass(tileType);
        tileFlags = GetTileFlags(tileType);
        if (hit & 2) {
            ledgeDir = PLAYER_DIR_NONE;
            if (MapTile_IsLedgeU(tileClass)) {
                ledgeDir = DIR_UP;
            } else if (MapTile_IsLedgeD(tileClass)) {
                ledgeDir = DIR_DOWN;
            } else if (MapTile_IsLedgeL(tileClass)) {
                ledgeDir = DIR_LEFT;
            } else if (MapTile_IsLedgeR(tileClass)) {
                ledgeDir = DIR_RIGHT;
            }
            if (ledgeDir != PLAYER_DIR_NONE && ledgeDir == input->dir) {
                return GRID_COMMAND_JUMP;
            }
        }
        if (hit == 0) {
            if (!(tileFlags & 2) && !MapTile_IsBlocksCycling2(tileClass)) {
                return GRID_COMMAND_MOVE;
            }
            if (input->force == TRUE) {
                return GRID_COMMAND_MOVE;
            }
        }
    }
    return GRID_COMMAND_BRAKE;
}

static u32 FieldPlayerGrid_DecideCommandBikeTurning(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandBikeIdle(grid, actor, input);
    }
    return FieldPlayerGrid_DecideCommandBikeMoving(grid, actor, input);
}

static u32 FieldPlayerGrid_DecideCommandBikeBraking(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    u16 faceDir = GetActorFaceDir(actor);

    if (input->dir != PLAYER_DIR_NONE && input->dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return FieldPlayerGrid_DecideCommandBikeMoving(grid, actor, input);
    }
    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandBikeIdle(grid, actor, input);
    }
    return FieldPlayerGrid_DecideCommandBikeMoving(grid, actor, input);
}

static u32 FieldPlayerGrid_DecideCommandSurf(FieldPlayerGrid *grid, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
    u32 command = GRID_COMMAND_NONE;

    switch (grid->state) {
    case GRID_STATE_IDLE:
        command = FieldPlayerGrid_DecideCommandSurfIdle(grid, actor, input);
        break;
    case GRID_STATE_MOVE:
        command = FieldPlayerGrid_DecideCommandSurfMoving(grid, actor, input);
        break;
    case GRID_STATE_TURN:
        command = FieldPlayerGrid_DecideCommandSurfTurning(grid, actor, input);
        break;
    case GRID_STATE_BRAKE:
        command = FieldPlayerGrid_DecideCommandSurfBraking(grid, actor, input);
        break;
    }
    return command;
}

static u32 FieldPlayerGrid_DecideCommandSurfIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (input->dir != PLAYER_DIR_NONE) {
            if (input->dir != GetActorFaceDir(actor) && input->force == FALSE) {
                return GRID_COMMAND_TURN;
            }
            return FieldPlayerGrid_DecideCommandSurfMoving(grid, actor, input);
        }
        return GRID_COMMAND_IDLE;
    }
    return GRID_COMMAND_NONE;
}

// Surfing: only onto water with nothing in the way
static u32 FieldPlayerGrid_DecideCommandSurfMoving(FieldPlayerGrid *grid, FieldActor *actor,
                                                   const GridMoveInput *input) {
    u32 tileType;
    u32 tileFlags;
    u32 hit;

    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandSurfIdle(grid, actor, input);
    }
    hit = PlayerMoveCollCheck(grid, actor, input->dir, &tileType);
    if (input->force == TRUE && hit != 0 && !(hit & 0x10)) {
        hit = 0;
    }
    if (tileType != TILE_TYPE_NONE) {
        GetTileClass(tileType);
        tileFlags = GetTileFlags(tileType);
        if (hit == 0) {
            if (tileFlags & 2) {
                return GRID_COMMAND_MOVE;
            }
            if (input->force == TRUE) {
                return GRID_COMMAND_MOVE;
            }
        }
    }
    return GRID_COMMAND_BRAKE;
}

static u32 FieldPlayerGrid_DecideCommandSurfTurning(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandSurfIdle(grid, actor, input);
    }
    return FieldPlayerGrid_DecideCommandSurfMoving(grid, actor, input);
}

static u32 FieldPlayerGrid_DecideCommandSurfBraking(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    u16 faceDir = GetActorFaceDir(actor);

    if (input->dir != PLAYER_DIR_NONE && input->dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return FieldPlayerGrid_DecideCommandSurfMoving(grid, actor, input);
    }
    if (IsAllActorAcmdFinished(actor) == FALSE) {
        return GRID_COMMAND_NONE;
    }
    if (input->dir == PLAYER_DIR_NONE) {
        return FieldPlayerGrid_DecideCommandSurfIdle(grid, actor, input);
    }
    return FieldPlayerGrid_DecideCommandSurfMoving(grid, actor, input);
}

static void FieldPlayerGrid_UpdateCore(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input) {
    switch (FieldPlayerCore_GetExState(grid->core)) {
    case FLD_PLAYER_EXSTATE_NONE:
        FieldPlayerGrid_UpdateNormal(grid, command, input);
        break;
    case FLD_PLAYER_EXSTATE_CYCLING:
        FieldPlayerGrid_UpdateBike(grid, command, input);
        break;
    case FLD_PLAYER_EXSTATE_SURF:
    case FLD_PLAYER_EXSTATE_DIVE:
        FieldPlayerGrid_UpdateSurf(grid, command, input);
        break;
    }
}

static void FieldPlayerGrid_UpdateNormal(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    switch (command) {
    case GRID_COMMAND_NONE:
        FieldPlayerGrid_UpdateNormalBusy(grid, actor, input);
        break;
    case GRID_COMMAND_IDLE:
        FieldPlayerGrid_UpdateNormalIdle(grid, actor, input);
        break;
    case GRID_COMMAND_MOVE:
        FieldPlayerGrid_UpdateNormalMove(grid, actor, input);
        break;
    case GRID_COMMAND_TURN:
        FieldPlayerGrid_UpdateNormalTurn(grid, actor, input);
        break;
    case GRID_COMMAND_BRAKE:
        FieldPlayerGrid_UpdateNormalBrake(grid, actor, input);
        break;
    case GRID_COMMAND_JUMP:
        FieldPlayerGrid_UpdateNormalJump(grid, actor, input);
        break;
    case GRID_COMMAND_CATWALK_BALANCE:
        FieldPlayerGrid_UpdateNormalCatwalkBalance(grid, actor, input);
        break;
    case GRID_COMMAND_CATWALK_EXIT:
        FieldPlayerGrid_UpdateNormalCatwalkExit(grid, actor, input);
        break;
    case GRID_COMMAND_CATWALK_EXIT_WAIT:
        FieldPlayerGrid_UpdateNormalCatwalkExitWait(grid, actor, input);
        break;
    case GRID_COMMAND_FALL:
        FieldPlayerGrid_UpdateNormalFall(grid, actor, input);
        break;
    }
}

static void FieldPlayerGrid_UpdateNormalBusy(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
}

static void FieldPlayerGrid_UpdateNormalIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    u16 dir = input->dir;

    if (dir == PLAYER_DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    grid->state = GRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
}

// A step, slower when the tile ahead is higher
static void FieldPlayerGrid_UpdateNormalMove(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    u16 acmd;
    BOOL checkHeight = TRUE;

    if (input->force == TRUE) {
        acmd = ACMD_WALK_2F;
        checkHeight = FALSE;
    } else if ((input->flags & MOVE_FLAG_CAN_RUN) && (input->heldKeys & KEY_B)) {
        acmd = ACMD_DASH;
        FieldPlayerGrid_SetFlag(grid, GRID_FLAG_RUNNING);
    } else {
        acmd = ACMD_WALK_8F;
    }
    if (checkHeight == TRUE && FieldPlayerGrid_CompareHeightChange(grid, input->dir) == 1) {
        switch (acmd) {
        case ACMD_WALK_4F:
            acmd = ACMD_WALK_UPHILL_4F;
            break;
        case ACMD_DASH:
            acmd = ACMD_DASH_UPHILL;
            break;
        case ACMD_WALK_8F:
            acmd = ACMD_WALK_UPHILL_8F;
            break;
        }
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, acmd));
    grid->state = GRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerGrid_UpdateNormalTurn(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_STAY_WALK_4F));
    grid->state = GRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_TURN);
}

static void FieldPlayerGrid_UpdateNormalBrake(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_STAY_WALK_16F));
    grid->state = GRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
    if (!FieldPlayerGrid_CheckWarpAtHitWall(grid, input->dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static void FieldPlayerGrid_UpdateNormalJump(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_JUMP_2));
    grid->state = GRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_MOVE);
    GFL_SndSEPlay(SEQ_SE_DANSA);
}

static void FieldPlayerGrid_UpdateNormalCatwalkBalance(FieldPlayerGrid *grid, FieldActor *actor,
                                                       const GridMoveInput *input) {
    grid->state = GRID_STATE_CATWALK_BALANCE;
}

// The jump off a catwalk, in the held direction or the first one free. The actor stops following the map's height,
// so it lands in the air over the tile below
static void FieldPlayerGrid_UpdateNormalCatwalkExit(FieldPlayerGrid *grid, FieldActor *actor,
                                                    const GridMoveInput *input) {
    u16 dir = input->dir;

    if (dir == PLAYER_DIR_NONE) {
        dir = FieldPlayerGrid_CalcCatwalkFallDir(grid);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_JUMP_1));
    FldAct_SetHeightCalcDisabled(actor, TRUE);
    FldAct_SetLandingDustDisabled(actor, TRUE);
    SetActorMovementFlag(actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW);
    grid->state = GRID_STATE_CATWALK_EXIT;
}

static void FieldPlayerGrid_UpdateNormalCatwalkExitWait(FieldPlayerGrid *grid, FieldActor *actor,
                                                        const GridMoveInput *input) {
    grid->state = GRID_STATE_CATWALK_EXIT_WAIT;
}

// The fall after the jump, down to the map's height
static void FieldPlayerGrid_UpdateNormalFall(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    VecFx32 pos;
    fx32 height;

    actor = FieldPlayerCore_GetActor(grid->core);
    CopyActorWPos(actor, &pos);
    GetHeightFromMap(actor, &pos, &height);
    if (pos.y > height) {
        pos.y -= FALL_SPEED;
        if (pos.y < height) {
            pos.y = height;
            ClearActorMovementFlag(actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW);
            grid->verticalMoveOnly = TRUE;
        }
        SetActorWPosValue(actor, &pos);
        grid->state = GRID_STATE_FALL;
    }
}

static void FieldPlayerGrid_UpdateBike(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    switch (command) {
    case GRID_COMMAND_NONE:
        FieldPlayerGrid_UpdateBikeBusy(grid, actor, input);
        break;
    case GRID_COMMAND_IDLE:
        FieldPlayerGrid_UpdateBikeIdle(grid, actor, input);
        break;
    case GRID_COMMAND_MOVE:
        FieldPlayerGrid_UpdateBikeMove(grid, actor, input);
        break;
    case GRID_COMMAND_TURN:
        FieldPlayerGrid_UpdateBikeTurn(grid, actor, input);
        break;
    case GRID_COMMAND_BRAKE:
        FieldPlayerGrid_UpdateBikeBrake(grid, actor, input);
        break;
    case GRID_COMMAND_JUMP:
        FieldPlayerGrid_UpdateBikeJump(grid, actor, input);
        break;
    }
}

static void FieldPlayerGrid_UpdateBikeBusy(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
}

static void FieldPlayerGrid_UpdateBikeIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    u16 dir = input->dir;

    if (dir == PLAYER_DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    grid->state = GRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
}

// A step, slower in deep sand and when the tile ahead is higher
static void FieldPlayerGrid_UpdateBikeMove(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    u32 tileType;
    u16 acmd;
    BOOL checkHeight = TRUE;

    if (input->force == TRUE) {
        acmd = ACMD_WALK_2F;
        checkHeight = FALSE;
    } else {
        acmd = ACMD_WALK_2F;
        if (FieldPlayerGrid_GetTileInDir(grid, PLAYER_DIR_NONE, &tileType) == TRUE &&
            MapTile_IsDeepSand(GetTileClass(tileType)) == TRUE) {
            acmd = ACMD_BIKE_DEEP_SAND;
            FldeffDust_Start(actor, Field_GetFieldEffects(grid->field));
        }
    }
    if (checkHeight == TRUE && FieldPlayerGrid_CompareHeightChange(grid, input->dir) == 1) {
        switch (acmd) {
        case ACMD_WALK_2F:
            acmd = ACMD_WALK_4F;
            break;
        }
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, acmd));
    grid->state = GRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerGrid_UpdateBikeTurn(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_STAY_WALK_4F));
    grid->state = GRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_TURN);
}

static void FieldPlayerGrid_UpdateBikeBrake(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_STAY_WALK_16F));
    grid->state = GRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
    if (!FieldPlayerGrid_CheckWarpAtHitWall(grid, input->dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static void FieldPlayerGrid_UpdateBikeJump(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_JUMP_2));
    grid->state = GRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_MOVE);
    GFL_SndSEPlay(SEQ_SE_DANSA);
}

static void FieldPlayerGrid_UpdateSurf(FieldPlayerGrid *grid, u32 command, const GridMoveInput *input) {
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    switch (command) {
    case GRID_COMMAND_NONE:
        FieldPlayerGrid_UpdateSurfBusy(grid, actor, input);
        break;
    case GRID_COMMAND_IDLE:
        FieldPlayerGrid_UpdateSurfIdle(grid, actor, input);
        break;
    case GRID_COMMAND_MOVE:
        FieldPlayerGrid_UpdateSurfMove(grid, actor, input);
        break;
    case GRID_COMMAND_TURN:
        FieldPlayerGrid_UpdateSurfTurn(grid, actor, input);
        break;
    case GRID_COMMAND_BRAKE:
        FieldPlayerGrid_UpdateSurfBrake(grid, actor, input);
        break;
    }
}

static void FieldPlayerGrid_UpdateSurfBusy(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
}

static void FieldPlayerGrid_UpdateSurfIdle(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    u16 dir = input->dir;

    if (dir == PLAYER_DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    grid->state = GRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
}

static void FieldPlayerGrid_UpdateSurfMove(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    u16 acmd;

    if (input->force == TRUE) {
        acmd = ACMD_WALK_2F;
    } else {
        acmd = ACMD_WALK_4F;
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, acmd));
    grid->state = GRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerGrid_UpdateSurfTurn(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_STAY_WALK_4F));
    grid->state = GRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_TURN);
}

static void FieldPlayerGrid_UpdateSurfBrake(FieldPlayerGrid *grid, FieldActor *actor, const GridMoveInput *input) {
    FldAct_SetAcmd(actor, GetAcmdForDir(input->dir, ACMD_STAY_WALK_16F));
    grid->state = GRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
    if (!FieldPlayerGrid_CheckWarpAtHitWall(grid, input->dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

// ActorRouteCollCheck for a step in a direction, and the tile type there, TILE_TYPE_NONE if there is none. A boulder in
// a hole ahead makes the hole passable
static u32 PlayerMoveCollCheck(FieldPlayerGrid *grid, FieldActor *actor, u16 dir, u32 *tileType) {
    VecFx32 pos;
    u32 hit;
    s16 x = GetGPosX(actor);
    s16 y = FldAct_GetGPosY(actor);
    s16 z = GetGPosZ(actor);

    x += GetDirectionVectorCompX(dir);
    z += GetDirectionVectorCompZ(dir);
    hit = ActorRouteCollCheck(actor, x, y, z, dir);
    CopyActorWPos(actor, &pos);
    ExpandVecInGridDir(dir, &pos, GRID_SIZE);
    if (GetTileTypeAtPosByActor(actor, &pos, tileType) == FALSE) {
        *tileType = TILE_TYPE_NONE;
    }
    if ((hit & 2) && MapTile_IsStrengthHole(GetTileClass(*tileType)) == TRUE &&
        IsSavedStrengthRockAtPos(GetActorMModelSystem(actor), &pos) == TRUE) {
        hit &= ~2;
    }
    return hit;
}

static void FieldPlayerGrid_SetFlag(FieldPlayerGrid *grid, u32 flag) {
    grid->flags |= flag;
}

static void FieldPlayerGrid_ClearFlag(FieldPlayerGrid *grid, u32 flag) {
    grid->flags &= ~flag;
}

static BOOL FieldPlayerGrid_CheckFlag(FieldPlayerGrid *grid, u32 flag) {
    if (grid->flags & flag) {
        return TRUE;
    }
    return FALSE;
}

static void FieldPlayerGrid_SetConstMoveFlag(FieldPlayerGrid *grid) {
    FieldPlayerGrid_SetFlag(grid, GRID_FLAG_CONST_MOVE);
}

static void FieldPlayerGrid_ClearConstMoveFlag(FieldPlayerGrid *grid) {
    FieldPlayerGrid_ClearFlag(grid, GRID_FLAG_CONST_MOVE);
}

static BOOL FieldPlayerGrid_CheckConstMoveFlag(FieldPlayerGrid *grid) {
    BOOL result = TRUE;

    if (!FieldPlayerGrid_CheckFlag(grid, GRID_FLAG_CONST_MOVE)) {
        result = FALSE;
    }
    return result;
}

static void FieldPlayerGrid_SetConstMoveEndSpotFlag(FieldPlayerGrid *grid) {
    FieldPlayerGrid_SetFlag(grid, GRID_FLAG_CONST_MOVE_END_SPOT);
}

static void FieldPlayerGrid_ClearConstMoveEndSpotFlag(FieldPlayerGrid *grid) {
    FieldPlayerGrid_ClearFlag(grid, GRID_FLAG_CONST_MOVE_END_SPOT);
}

static void FieldPlayerGrid_ClearFlag4(FieldPlayerGrid *grid) {
    FieldPlayerGrid_ClearFlag(grid, GRID_FLAG_4);
}

static void FieldPlayerGrid_SetCatwalkBalancingFlag(FieldPlayerGrid *grid, BOOL set) {
    if (set == TRUE) {
        FieldPlayerGrid_SetFlag(grid, GRID_FLAG_CATWALK_BALANCING);
    } else {
        FieldPlayerGrid_ClearFlag(grid, GRID_FLAG_CATWALK_BALANCING);
    }
}

static BOOL FieldPlayerGrid_CheckCatwalkBalancingFlag(FieldPlayerGrid *grid) {
    if (FieldPlayerGrid_CheckFlag(grid, GRID_FLAG_CATWALK_BALANCING)) {
        return TRUE;
    }
    return FALSE;
}

static void FieldPlayerGrid_SetCatwalkExitingFlag(FieldPlayerGrid *grid, BOOL set) {
    if (set == TRUE) {
        FieldPlayerGrid_SetFlag(grid, GRID_FLAG_CATWALK_EXITING);
    } else {
        FieldPlayerGrid_ClearFlag(grid, GRID_FLAG_CATWALK_EXITING);
    }
}

static BOOL FieldPlayerGrid_CheckCatwalkExitingFlag(FieldPlayerGrid *grid) {
    if (FieldPlayerGrid_CheckFlag(grid, GRID_FLAG_CATWALK_EXITING)) {
        return TRUE;
    }
    return FALSE;
}

void FieldPlayerGrid_ForceBrake(FieldPlayerGrid *grid) {
    if (grid->state == GRID_STATE_BRAKE) {
        FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
        u16 dir = GetActorFaceDir(actor);

        FldAct_FreeAcmd(actor);
        CheckSetActorFaceDir(actor, dir);
        FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
        FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
    }
}

BOOL FieldPlayerGrid_StopTurnOrBlocked(FieldPlayerGrid *grid) {
    switch (grid->state) {
    case GRID_STATE_IDLE:
        break;
    case GRID_STATE_MOVE:
        return FALSE;
    case GRID_STATE_TURN:
    case GRID_STATE_BRAKE: {
        FieldActor *actor = FieldPlayerGrid_GetPlayerActor(grid);
        u16 dir = GetActorFaceDir(actor);

        FldAct_FreeAcmd(actor);
        CheckSetActorFaceDir(actor, dir);
        FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
        FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
        break;
    }
    }
    return TRUE;
}

// The movement that the tile under the player forces: a slide on ice, unless one just ended here
static u32 FieldPlayerGrid_GetConstMoveType(FieldPlayerGrid *grid, u16 dir) {
    u32 tileType = GetCachedTileUnderActor(FieldPlayerCore_GetActor(grid->core));

    if (!FieldPlayerGrid_CheckFlag(grid, GRID_FLAG_CONST_MOVE_END_SPOT) &&
        IsTileSlidingIce(GetTileClass(tileType)) == TRUE) {
        return CONST_MOVE_STRAIGHT;
    }
    return CONST_MOVE_NONE;
}

BOOL FieldPlayerGrid_IsConstMoving(FieldPlayerGrid *grid) {
    if (FieldPlayerGrid_GetConstMoveType(grid, PLAYER_DIR_NONE) != CONST_MOVE_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldPlayerGrid_IsCatwalkExiting(FieldPlayerGrid *grid) {
    if (FieldPlayerGrid_CheckCatwalkExitingFlag(grid) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

// After the fall from a catwalk: the actor follows the map's height again, at the grid height it landed on
void FieldPlayerGrid_SanitizeHeightMismatch(FieldPlayerGrid *grid) {
    VecFx32 pos;
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    FldAct_SetHeightCalcDisabled(actor, FALSE);
    FldAct_SetLandingDustDisabled(actor, FALSE);
    SetActorMovementFlag(actor, 0x140);
    CheckRecalcActorY(actor);
    CopyActorWPos(actor, &pos);
    SetActorGPosY(actor, HEIGHT_TO_GRID(pos.y));
    FldAct_CacheTerrainInfo(actor);
    SetActorInitialGPosToNowGPos(actor);
}

static void FieldPlayerGrid_ClearConstMove(FieldPlayerGrid *grid) {
    if (FieldPlayerGrid_CheckConstMoveFlag(grid) == TRUE) {
        ClearActorFlag(FieldPlayerCore_GetActor(grid->core), 0x18);
        FieldPlayerGrid_ClearConstMoveFlag(grid);
    }
}

static BOOL FieldPlayerGrid_ConstMoveUpdate_NONE(FieldPlayerGrid *grid, u16 dir) {
    FieldPlayerGrid_ClearConstMove(grid);
    return FALSE;
}

// A slide: one more tile the way the actor is moving, until anything at all is in the way
static BOOL FieldPlayerGrid_ConstMoveUpdate_STRAIGHT(FieldPlayerGrid *grid, u16 dir) {
    u32 tileType;
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
    u16 motionDir = GetActorMotionDir(actor);

    if (PlayerMoveCollCheck(grid, actor, motionDir, &tileType) != 0) {
        FieldPlayerGrid_ClearConstMove(grid);
        FieldPlayerGrid_SetConstMoveEndSpotFlag(grid);
        FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_NONE);
        return FALSE;
    }
    FieldPlayerGrid_SetConstMoveFlag(grid);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    grid->state = GRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(grid->core, PLAYER_MOVE_STATUS_MOVE);
    return TRUE;
}

static const ConstMoveFunc FIELD_PLAYER_GRID_CONST_MOVE_FUNCS[] = {
    FieldPlayerGrid_ConstMoveUpdate_NONE,
    FieldPlayerGrid_ConstMoveUpdate_STRAIGHT,
};

// TRUE if the tile moved the player, so the keys don't
static BOOL FieldPlayerGrid_ConstMoveUpdate(FieldPlayerGrid *grid, u16 dir, BOOL disabled) {
    if (!disabled) {
        u32 type = FieldPlayerGrid_GetConstMoveType(grid, dir);

        if (FIELD_PLAYER_GRID_CONST_MOVE_FUNCS[type](grid, dir) == TRUE) {
            return TRUE;
        }
    }
    FieldPlayerGrid_ClearConstMoveFlag(grid);
    return FALSE;
}

static BOOL MapTile_IsCatwalk(u32 tileClass) {
    if (MapTile_IsCatwalkBody_(tileClass) == TRUE || MapTile_IsCatwalkEntryPoint(tileClass) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MapTile_IsCatwalkBody(u32 tileClass) {
    if (MapTile_IsCatwalkBody_(tileClass) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldPlayerGrid_CheckOnCatwalk(FieldPlayerGrid *grid) {
    u32 tileType;

    FieldPlayerGrid_GetTileInDir(grid, PLAYER_DIR_NONE, &tileType);
    if (MapTile_IsCatwalkBody(GetTileClass(tileType)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

// The first direction with a tile to fall onto, from the one the player faces
static u16 FieldPlayerGrid_CalcCatwalkFallDir(FieldPlayerGrid *grid) {
    u32 tileType;
    u16 nextDirs[4] = { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN };
    u16 i;
    u16 dir = FieldPlayerCore_GetFaceDir(grid->core);

    for (i = 0; i < 4; i++, dir = nextDirs[dir]) {
        if (FieldPlayerGrid_GetTileInDir(grid, dir, &tileType) == TRUE && !MapTile_IsCatwalk(GetTileClass(tileType)) &&
            !MapTile_BlocksCollision(tileType)) {
            return dir;
        }
    }
    return PLAYER_DIR_NONE;
}

static u32 PlayerMoveCollCheckCatwalk(FieldPlayerGrid *grid, FieldActor *actor, u16 dir, u32 *tileType) {
    u32 hit = PlayerMoveCollCheck(grid, actor, dir, tileType);

    if (MapTile_IsCatwalk(GetTileClass(*tileType)) == TRUE) {
        hit |= COLL_CATWALK;
    }
    return hit;
}

// Whether the falling player has reached the map's height
static BOOL FieldPlayerGrid_CheckModelHeightMismatch(FieldPlayerGrid *grid) {
    VecFx32 pos;
    fx32 height;
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    CopyActorWPos(actor, &pos);
    GetHeightFromMap(actor, &pos, &height);
    if (pos.y <= height) {
        return TRUE;
    }
    return FALSE;
}

static void FieldPlayerGrid_BeginCatwalkBalance(FieldPlayerGrid *grid) {
    FieldPlayerGrid_SetCatwalkBalancingFlag(grid, TRUE);
    FieldPlayerCore_SetFormSlotModel(grid->core, 7);
}

static void FieldPlayerGrid_ResetCatwalkBalance(FieldPlayerGrid *grid) {
    grid->balanceTime = 0;
    grid->balanceKept = FALSE;
    FieldPlayerGrid_SetCatwalkExitingFlag(grid, FALSE);
    FieldPlayerGrid_SetCatwalkBalancingFlag(grid, FALSE);
    FieldPlayerCore_InitModel(grid->core);
}

FieldPlayerCore *FieldPlayerGrid_GetPlayerCore(FieldPlayerGrid *grid) {
    return grid->core;
}

FieldActor *FieldPlayerGrid_GetPlayerActor(FieldPlayerGrid *grid) {
    return FieldPlayerCore_GetActor(grid->core);
}

void FieldPlayerGrid_InterruptCatwalkBalance(FieldPlayerGrid *grid, BOOL keep) {
    if (FieldPlayerGrid_CheckCatwalkBalancingFlag(grid) == TRUE) {
        u16 time = grid->balanceTime;

        FieldPlayerGrid_ResetCatwalkBalance(grid);
        if (keep == TRUE) {
            grid->balanceTime = time;
            grid->balanceKept = TRUE;
        }
    }
}

void FieldPlayerGrid_StopForcedMove(FieldPlayerGrid *grid) {
    FieldPlayerGrid_ClearConstMove(grid);
}

BOOL FieldPlayerGrid_GetVerticalMoveOnlyFlag(FieldPlayerGrid *grid) {
    return grid->verticalMoveOnly;
}

void FieldPlayerGrid_SetVerticalMoveOnlyFlag(FieldPlayerGrid *grid) {
    grid->verticalMoveOnly = TRUE;
}

BOOL FieldPlayerGrid_IsBraking(FieldPlayerGrid *grid) {
    if (grid->state == GRID_STATE_BRAKE) {
        return TRUE;
    }
    return FALSE;
}

// The tile type under the player (PLAYER_DIR_NONE) or a step in the direction. FALSE if there is no terrain
static BOOL FieldPlayerGrid_GetTileInDir(FieldPlayerGrid *grid, u16 dir, u32 *tileType) {
    VecFx32 pos;

    FieldPlayerCore_GetWPos(grid->core, &pos);
    if (dir != PLAYER_DIR_NONE) {
        ExpandVecInGridDir(dir, &pos, GRID_SIZE);
    }
    return GetTileTypeAtPosByActor(FieldPlayerCore_GetActor(grid->core), &pos, tileType);
}

// Whether the map a step in the direction is higher than the player (1), lower (2) or level (0)
static u32 FieldPlayerGrid_CompareHeightChange(FieldPlayerGrid *grid, u16 dir) {
    VecFx32 pos;
    fx32 height;
    fx32 y;
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);

    CopyActorWPos(actor, &pos);
    y = pos.y;
    ExpandVecInGridDir(dir, &pos, GRID_SIZE);
    if (GetHeightFromMap(actor, &pos, &height) == TRUE) {
        if (y < height) {
            return 1;
        }
        if (y > height) {
            return 2;
        }
    }
    return 0;
}

// Whether the wall the player walks into is a way out, which makes no sound: a warp, or the next ride of the
// Castelia City gym
static BOOL FieldPlayerGrid_CheckWarpAtHitWall(FieldPlayerGrid *grid, u16 dir) {
    VecFx32 pos;
    FieldActor *actor = FieldPlayerCore_GetActor(grid->core);
    GameSystem *gsys = Field_GetGameSystem(grid->field);
    EventData *eventData = GameData_GetEventData(GSYS_GetGameData(gsys));

    CopyActorWPos(actor, &pos);
    if (GetWarpIDByPlayerPos(eventData, &pos, dir) != WARP_ID_NONE) {
        return TRUE;
    }
    if (Field_CheckGimmickID(grid->field, GIMMICK_CASTELIA_GYM) && GymInsect_IsRideAhead(gsys, dir)) {
        return TRUE;
    }
    return FALSE;
}
