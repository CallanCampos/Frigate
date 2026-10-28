#include "types.h"
#include "constants/sound.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_gimmick_d06.h"
#include "field/field_map.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define GIMMICK_ID 13
#define STONE_ACTOR_ID 0xfd
#define STONE_COUNT 12
#define MARK_COUNT 14

// How far a stone can go in a push
#define MAX_PUSH_STEPS 0x20

// A position on the grid. The stones ignore y
typedef struct {
    s32 x;
    s32 y;
    s32 z;
} StoneGridPos;

// The stones and the marks of a zone
typedef struct {
    u32 zone;
    u32 count;
    const StoneGridPos *positions;
} ZoneTable;

// What the save keeps: whether the stones exist already
typedef struct {
    u8 initialized;
    struct StoneWork *work;
} D06State;

// A stone and its effect
typedef struct {
    FieldActor *actor;
    FieldEffectTask *effect;
} StoneSlot;

typedef struct StoneWork {
    u32 zone;
    D06State *state;
    Field *field;
    StoneSlot stones[STONE_COUNT];
    FieldEffectTask *marks[MARK_COUNT];
} StoneWork;

// The event of a push
typedef struct {
    u16 dir;
    u16 slotIndex;
    int distance;
    StoneSlot *slot;
    StoneWork *work;
} PushData;

static const ZoneTable *FindStoneTable(u32 zone);
static const StoneGridPos *GetStonePositions(u32 zone);
static u32 GetStoneCount(u32 zone);
static const ZoneTable *FindMarkTable(u32 zone);
static const StoneGridPos *GetMarkPositions(u32 zone);
static u32 GetMarkCount(u32 zone);
static GameEvent *StartPush(Field *field, FieldActor *stone, u16 dir);
static GameEventReturnCode PushEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode BlockedEvent(GameEvent *event, u32 *state, void *data);
static void InitStoneAt(StoneWork *work, StoneSlot *slot, int x, int z);
static void InitStoneActor(StoneWork *work, StoneSlot *slot, FieldActor *actor);
static BOOL ProbeElectricRock(StoneWork *work, StoneSlot *slot, u16 dir, u16 maxSteps, u32 *steps);
static u32 GetDistanceToElectricRock(StoneWork *work, StoneSlot *slot, u16 dir);
static BOOL IsNextToElectricRock(StoneWork *work, StoneSlot *slot);
static void InitMark(StoneWork *work, FieldEffectTask **mark, int x, int y, int z);
static void StopMark(Field *field, u32 index);
static FieldActor *CreateStone(Field *field, int x, int z);
static StoneWork *GetWork(Field *field);
static u32 GetGimmickId(u32 zone);

static const StoneGridPos sStonesZone3[5] = {
    { 43, 1, 49 }, { 47, 1, 45 }, { 45, 0, 14 }, { 43, 0, 12 }, { 36, 0, 18 },
};

static const StoneGridPos sMarksZone3[11] = {
    { 54, -1, 51 }, { 31, -1, 48 }, { 31, -1, 49 }, { 31, -1, 50 }, { 61, 0, 37 },  { 46, 0, 41 },
    { 50, 0, 26 },  { 56, 0, 17 },  { 41, -1, 17 }, { 46, -1, 5 },  { 52, -1, 10 },
};

static const StoneGridPos sStonesZone4[3] = {
    { 9, 0, 23 },
    { 21, 0, 9 },
    { 24, 0, 3 },
};

static const StoneGridPos sMarksZone4[11] = {
    { 8, -1, 14 },  { 8, -3, 35 },  { 12, -3, 42 }, { 17, -3, 49 }, { 15, -5, 53 }, { 11, -6, 50 },
    { 20, -3, 35 }, { 24, -1, 25 }, { 25, -1, 14 }, { 27, -1, 6 },  { 23, -1, 12 },
};

static const StoneGridPos sStonesZone2[12] = {
    { 15, 0, 39 }, { 24, 0, 46 }, { 9, 0, 26 },  { 20, 1, 27 }, { 21, 1, 26 }, { 24, 1, 16 },
    { 24, 1, 17 }, { 34, 0, 26 }, { 40, 1, 12 }, { 60, 0, 42 }, { 61, 0, 52 }, { 61, 0, 53 },
};

static const StoneGridPos sMarksZone2[14] = {
    { 8, -1, 47 },  { 8, -1, 37 },  { 13, 0, 18 }, { 40, -1, 24 }, { 27, 0, 13 },  { 12, 0, 11 },  { 38, 0, 15 },
    { 55, -1, 50 }, { 62, -1, 45 }, { 50, 0, 16 }, { 66, -1, 25 }, { 63, -1, 24 }, { 64, -1, 26 }, { 25, -1, 54 },
};

static const ZoneTable sStoneTables[3] = {
    { ZONE_CHARGESTONE_CAVE_2, NELEMS(sStonesZone2), sStonesZone2 },
    { ZONE_CHARGESTONE_CAVE_3, NELEMS(sStonesZone3), sStonesZone3 },
    { ZONE_CHARGESTONE_CAVE_4, NELEMS(sStonesZone4), sStonesZone4 },
};

static const ZoneTable sMarkTables[3] = {
    { ZONE_CHARGESTONE_CAVE_2, NELEMS(sMarksZone2), sMarksZone2 },
    { ZONE_CHARGESTONE_CAVE_3, NELEMS(sMarksZone3), sMarksZone3 },
    { ZONE_CHARGESTONE_CAVE_4, NELEMS(sMarksZone4), sMarksZone4 },
};

static const ZoneTable *FindStoneTable(u32 zone) {
    int i;
    const ZoneTable *table = sStoneTables;

    for (i = 0; i < NELEMS(sStoneTables); i++, table++) {
        if (table->zone == zone) {
            return table;
        }
    }
    return &sStoneTables[0];
}

static const StoneGridPos *GetStonePositions(u32 zone) {
    return FindStoneTable(zone)->positions;
}

static u32 GetStoneCount(u32 zone) {
    return FindStoneTable(zone)->count;
}

static const ZoneTable *FindMarkTable(u32 zone) {
    int i;
    const ZoneTable *table = sMarkTables;

    for (i = 0; i < NELEMS(sMarkTables); i++, table++) {
        if (table->zone == zone) {
            return table;
        }
    }
    return &sMarkTables[0];
}

static const StoneGridPos *GetMarkPositions(u32 zone) {
    return FindMarkTable(zone)->positions;
}

static u32 GetMarkCount(u32 zone) {
    return FindMarkTable(zone)->count;
}

void GimmickD06_Setup(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    GimmickState *gimmickState = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    void *effects = Field_GetFieldEffects(field);
    u32 zone = Field_GetPlayerStateZoneID(field);
    D06State *state = GimmickState_GetUserData(gimmickState, GetGimmickId(zone));
    StoneWork *work;
    u32 effectId;
    u32 count;
    const StoneGridPos *positions;
    u32 i;

    work = GFL_HeapAllocate(heapId, sizeof(StoneWork), TRUE, "field_gimmick_d06.c", 202);
    state->work = work;
    work->state = state;
    work->field = field;
    work->zone = zone;
    effectId = FLDEFF_D06_ELECTRIC;
    FieldEffects_Load(effects, &effectId, 1);
    if (!state->initialized) {
        count = GetStoneCount(work->zone);
        positions = GetStonePositions(work->zone);
        for (i = 0; i < count; i++, positions++) {
            InitStoneAt(work, &work->stones[i], positions->x, positions->z);
        }
    } else {
        MMSys *actorSystem;
        u32 index = 0;
        FieldActor *actor;

        i = 0;
        actorSystem = Field_GetActorSystem(field);
        while (NextActor(actorSystem, &actor, &index)) {
            if (GetActorUID(actor) == STONE_ACTOR_ID) {
                InitStoneActor(work, &work->stones[i], actor);
                i++;
                if (i >= STONE_COUNT) {
                    break;
                }
            }
        }
    }
    count = GetMarkCount(work->zone);
    positions = GetMarkPositions(work->zone);
    for (i = 0; i < count; i++, positions++) {
        InitMark(work, &work->marks[i], positions->x, positions->y, positions->z);
    }
    state->initialized = TRUE;
}

void GimmickD06_End(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    GimmickState *gimmickState = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    void *effects = Field_GetFieldEffects(field);
    D06State *state;

    state = GimmickState_GetUserData(gimmickState, GetGimmickId(Field_GetPlayerStateZoneID(field)));
    GFL_HeapFree(state->work);
}

void GimmickD06_Update(Field *field) {
}

GameEvent *GimmickD06_CheckPush(Field *field, u16 dir) {
    GameEvent *event = NULL;
    StoneWork *work = GetWork(field);
    VecFx32 pos;
    s16 gridX;
    s16 gridZ;
    u32 count;
    u32 i;
    const StoneGridPos *positions;
    FieldActor *stone;

    FieldPlayer_GetWPos(Field_GetPlayer(field), &pos);
    ExpandVecInGridDir(dir, &pos, FX32_ONE << 4);
    gridX = (s16)((pos.x >> 4) / FX32_ONE);
    gridZ = (s16)((pos.z >> 4) / FX32_ONE);
    count = GetMarkCount(work->zone);
    positions = GetMarkPositions(work->zone);
    for (i = 0; i < count; i++, positions++) {
        if (positions->x == gridX && positions->z == gridZ) {
            StopMark(field, i);
        }
    }
    stone = GetFirstActorOnGPos(Field_GetActorSystem(field), gridX, gridZ, FALSE);
    if (stone != NULL && GetActorUID(stone) == STONE_ACTOR_ID) {
        event = StartPush(field, stone, dir);
    }
    return event;
}

static GameEvent *StartPush(Field *field, FieldActor *stone, u16 dir) {
    u16 i;
    StoneWork *work = GetWork(field);
    u32 distance;
    GameSystem *gsys = Field_GetGameSystem(field);
    StoneSlot *slot;
    GameEvent *event;
    PushData *data;

    for (i = 0; i < STONE_COUNT; i++) {
        if (work->stones[i].actor == stone) {
            break;
        }
    }
    if (i >= STONE_COUNT) {
        i = 0;
    }
    slot = &work->stones[i];
    if (IsNextToElectricRock(work, slot) == TRUE) {
        return NULL;
    }
    distance = GetDistanceToElectricRock(work, slot, dir);
    if (distance == 0) {
        event = GameEvent_Create(gsys, NULL, BlockedEvent, sizeof(PushData));
    } else {
        event = GameEvent_Create(gsys, NULL, PushEvent, sizeof(PushData));
    }
    data = GameEvent_GetData(event);
    MI_CpuFill(data, 0, sizeof(PushData));
    data->dir = dir;
    data->work = work;
    data->slotIndex = i;
    data->slot = slot;
    data->distance = distance;
    return event;
}

static GameEventReturnCode PushEvent(GameEvent *event, u32 *state, void *data) {
    PushData *push = data;
    FieldActor *player = FieldPlayer_GetActor(Field_GetPlayer(push->work->field));

    switch (*state) {
    case 0:
        if (IsAllActorAcmdFinished(player) == TRUE) {
            FldAct_SetAcmd(player, GetAcmdForDir(push->dir, ACMD_STAY_WALK_16F));
            GFL_SndSEPlay(SEQ_SE_FLD_63);
            (*state)++;
        }
        break;
    case 1:
        if (IsAllActorAcmdFinished(push->slot->actor) == TRUE) {
            if (push->distance > 0) {
                u16 acmd = ACMD_WALK_16F;

                if (push->distance <= 1) {
                    acmd = ACMD_WALK_4F;
                } else if (push->distance <= 2) {
                    acmd = ACMD_WALK_4F;
                } else if (push->distance <= 4) {
                    acmd = ACMD_WALK_8F;
                }
                FldAct_SetAcmd(push->slot->actor, GetAcmdForDir(push->dir, acmd));
                push->distance--;
            } else {
                GFL_SndSEPlay(SEQ_SE_FLD_50);
                (*state)++;
            }
        }
        break;
    case 2:
        if (FldAct_CheckAcmdEnd(player) == TRUE) {
            FldAct_EndAcmd(push->slot->actor);
            FldAct_EndAcmd(player);
            StoneTask_FollowFreezeLoop(push->slot->effect);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode BlockedEvent(GameEvent *event, u32 *state, void *data) {
    PushData *push = data;
    FieldActor *player = FieldPlayer_GetActor(Field_GetPlayer(push->work->field));

    switch (*state) {
    case 0:
        if (IsAllActorAcmdFinished(player) == TRUE) {
            FldAct_SetAcmd(player, GetAcmdForDir(push->dir, ACMD_STAY_WALK_16F));
            GFL_SndSEPlay(SEQ_SE_WALL_HIT);
            (*state)++;
        }
    case 1:
        StoneTask_FollowStartEnding(push->slot->effect);
        (*state)++;
        break;
    case 2:
        if (!StoneTask_FollowIsEnded(push->slot->effect)) {
            break;
        }
        (*state)++;
    case 3:
        if (FldAct_CheckAcmdEnd(player) == TRUE) {
            FldAct_EndAcmd(player);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static void InitStoneAt(StoneWork *work, StoneSlot *slot, int x, int z) {
    VecFx32 pos;

    slot->actor = CreateStone(work->field, x, z);
    CopyActorWPos(slot->actor, &pos);
    slot->effect = StoneTask_StartFollow(Field_GetFieldEffects(work->field), &pos, slot->actor);
    if (IsNextToElectricRock(work, slot) == TRUE) {
        StoneTask_FollowFreezeLoop(slot->effect);
    }
}

static void InitStoneActor(StoneWork *work, StoneSlot *slot, FieldActor *actor) {
    VecFx32 pos;

    slot->actor = actor;
    CopyActorWPos(actor, &pos);
    slot->effect = StoneTask_StartFollow(Field_GetFieldEffects(work->field), &pos, slot->actor);
    if (IsNextToElectricRock(work, slot) == TRUE) {
        StoneTask_FollowFreezeLoop(slot->effect);
    }
}

// Whether the stone, moving in dir over free tiles, reaches an electric rock tile within maxSteps; the steps it takes
// then, or 0
static BOOL ProbeElectricRock(StoneWork *work, StoneSlot *slot, u16 dir, u16 maxSteps, u32 *steps) {
    FieldActor *actor = slot->actor;
    u16 i = 0;
    BOOL found = FALSE;
    FieldG3DMapper *mapper = Field_GetG3DMapper(work->field);
    s16 x;
    s16 y;
    s16 z;
    u32 collision;
    VecFx32 pos;
    u32 tileType;
    u32 walkable;

    x = GetGPosX(actor);
    y = FldAct_GetGPosY(actor);
    z = GetGPosZ(actor);
    walkable = 1;
    do {
        AdjusGridXZByDir(dir, &x, &z, 1);
        collision = ActorRouteCollCheck(actor, x, y, z, dir) & ~walkable;
        if (collision) {
            if (collision & 2) {
                ConvGXZToVector(x, z, &pos);
                pos.y = GetActorPosY(actor);
                tileType = GetTileTypeAtPos(mapper, &pos);
                if (tileType != TILE_TYPE_NONE && MapTile_IsElectricRock(GetTileClass(tileType)) == TRUE) {
                    found = TRUE;
                }
            }
            break;
        }
        i++;
    } while (i < maxSteps);
    if (!found) {
        i = 0;
    }
    *steps = i;
    return found;
}

static u32 GetDistanceToElectricRock(StoneWork *work, StoneSlot *slot, u16 dir) {
    u32 steps;

    ProbeElectricRock(work, slot, dir, MAX_PUSH_STEPS, &steps);
    return steps;
}

static BOOL IsNextToElectricRock(StoneWork *work, StoneSlot *slot) {
    u16 dir;
    u32 steps;

    for (dir = 0; dir < 4; dir++) {
        if (ProbeElectricRock(work, slot, dir, 1, &steps) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

static void InitMark(StoneWork *work, FieldEffectTask **mark, int x, int y, int z) {
    VecFx32 pos;

    ConvGXZToVector(x, z, &pos);
    pos.y = (y << 16) + 0x8000;
    *mark = StoneTask_StartPulse(Field_GetFieldEffects(work->field), &pos);
}

static void StopMark(Field *field, u32 index) {
    GimmickState *gimmickState = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    D06State *state = GimmickState_GetUserData(gimmickState, GetGimmickId(Field_GetPlayerStateZoneID(field)));

    StoneTask_PulseTrigger(state->work->marks[index]);
}

static FieldActor *CreateStone(Field *field, int x, int z) {
    MMSys *actorSystem = Field_GetActorSystem(field);

    return CreateNewActorByParam(actorSystem, x, z, 0, STONE_ACTOR_ID, 0, MOVE_CODE_NONE,
                                 Field_GetPlayerStateZoneID(field));
}

static StoneWork *GetWork(Field *field) {
    GimmickState *gimmickState = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    D06State *state = GimmickState_GetUserData(gimmickState, GetGimmickId(Field_GetPlayerStateZoneID(field)));

    return state->work;
}

// The gimmick is the same in every zone of the cave
static u32 GetGimmickId(u32 zone) {
    return GIMMICK_ID;
}
