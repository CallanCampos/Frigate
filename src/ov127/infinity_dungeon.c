// The Black Tower and White Treehollow's dungeon: the event that moves the player between its rooms, the dungeon's
// start, the battle against a trainer, the scripts of the NPCs that stand in the rooms, the trainers they have and
// the reward at the end. Named after the ROM's "infinity_dungeon.c". The tables and the layout are in the next file
// of the overlay, which the ROM has no name for
#include "field/infinity_dungeon.h"
#include "types.h"
#include "battle/trainer_data.h"
#include "constants/items.h"
#include "constants/version.h"
#include "field/battle_facility.h"
#include "field/bsubway_scr.h"
#include "field/event_battle.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_prop.h"
#include "field/infinity_dungeon_creator.h"
#include "field/infinity_dungeon_gimmick.h"
#include "field/infinity_dungeon_rooms.h"
#include "gfl/calctool.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The dungeon's trainer data and the Pokémon they use are files of these archives
#define INFINITY_DUNGEON_ARC_POKEMON 0x105
#define INFINITY_DUNGEON_ARC_TRAINERS 0x106
// The trainer id of a slot with no trainer
#define INFINITY_DUNGEON_TRAINER_NONE 660
// The script that a trainer of the dungeon talks with
#define INFINITY_DUNGEON_TRAINER_SCRIPT 0x29f9
// The grid a floor's rooms are in is 4 rooms wide
#define INFINITY_DUNGEON_GRID_WIDTH 4

// What the script of an NPC is chosen from
typedef struct {
    InfinityDungeon *dungeon;
    KeyDataSave *keyData;
    u8 npcIndex;
} InfinityDungeonNpcScriptArg;

typedef u32 (*InfinityDungeonNpcScriptFunc)(InfinityDungeonNpcScriptArg *arg);

static const u16 *InfinityDungeon_GetWallProps(InfinityDungeon *dungeon);
static void InfinityDungeonRoomChange_CreateTrainerActor(const InfinityDungeonRoomChange *roomChange,
                                                         const InfinityDungeonPos *pos);
static BOOL InfinityDungeon_SetFixedFloors(KeyDataSave *keyData, InfinityDungeon *dungeon);
static void InfinityDungeon_SetFixedTrainers(KeyDataSave *keyData, InfinityDungeon *dungeon);
static u32 InfinityDungeon_GetGoalRoom(InfinityDungeon *dungeon);
static u32 InfinityDungeonNpcScript_TargetFloor(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_GoalFloor(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_GoalDirection(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeon_GetSideOfRoom(u8 x, u8 y, u8 targetX, u8 targetY);
static u32 InfinityDungeonNpcScript_Plain(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_SpecialDirection(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_TargetDirection(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_TargetTrainerCount(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_TargetTrainerSex(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_TargetTrainerTalk(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_Goal(InfinityDungeonNpcScriptArg *arg);
static u32 InfinityDungeonNpcScript_Default(InfinityDungeonNpcScriptArg *arg);
static u16 InfinityDungeon_GetBattleSeed(InfinityDungeon *dungeon, u8 floor, u8 slot);
static void InfinityDungeon_PickReward(InfinityDungeon *dungeon, u32 rank, u32 seed);

// The props that close a room's passages, in White 2 and in Black 2
static const u16 sWhiteWallProps[INFINITY_DUNGEON_WALL_COUNT] = { 0x97, 0x9d, 0x9e, 0x9c };
static const u16 sBlackWallProps[INFINITY_DUNGEON_WALL_COUNT] = { 0x96, 0x9a, 0x99, 0x9b };

static const u16 sBlack2Trainers0[32] = {
    0x38, 0x271, 0x280, 0x1e, 0x28, 0x26, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const u16 sBlack2Trainers1[32] = {
    0x67, 0x6c, 0x74, 0x5d, 0x62, 0x75, 0x281, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1,    1,    1,    1,    1,    1,    1,     1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const u16 sBlack2Specials1[32] = {
    0x281, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282,
    0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282, 0x282,
};

static const u16 sWhite2Trainers0[32] = {
    0xd, 0x262, 0x28a, 0x17, 6, 0x12, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const u16 sWhite2Specials0[32] = {
    0x28a, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b,
    0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b, 0x28b,
};

static const u16 sBlack2Specials0[32] = {
    0x280, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281,
    0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281, 0x281,
};

static const u16 sWhite2Trainers1[32] = {
    0x42, 0x3f, 0x4e, 0x44, 0x50, 0x46, 0x28b, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1,    1,    1,    1,    1,    1,    1,     1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const u16 sWhite2Specials1[32] = {
    0x28b, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c,
    0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c, 0x28c,
};

static const InfinityDungeonFloor sFloorBlack2Mode1 = {
    {
        { 0x13, 0, { 0, 0, 1, 0 }, 0 },
        { 0, 4, { 0, 0, 1, 1 }, 1 },
        { 7, 4, { 0, 1, 0, 1 }, 2 },
        { 0x13, 4, { 0, 0, 0, 0 }, 3 },
        { 0x13, 1, { 0, 1, 0, 0 }, 4 },
        { 4, 4, { 0, 1, 1, 0 }, 5 },
        { 0x13, 4, { 1, 0, 0, 1 }, 6 },
        { 0x13, 4, { 0, 0, 0, 0 }, 7 },
        { 0x13, 4, { 1, 0, 1, 0 }, 8 },
        { 1, 4, { 1, 0, 1, 1 }, 9 },
        { 8, 4, { 0, 0, 0, 1 }, 10 },
        { 0x13, 4, { 0, 0, 0, 0 }, 11 },
        { 0x13, 4, { 0, 0, 0, 0 }, 12 },
        { 0x13, 4, { 0, 0, 0, 0 }, 13 },
        { 0x13, 4, { 0, 0, 0, 0 }, 14 },
        { 0x13, 4, { 0, 0, 0, 0 }, 15 },
    },
};

static const InfinityDungeonFloor sFloorWhite2Mode0 = {
    {
        { 0x13, 0, { 0, 1, 0, 0 }, 0 },
        { 0x13, 1, { 0, 1, 0, 0 }, 1 },
        { 0, 4, { 0, 1, 0, 0 }, 2 },
        { 0x13, 4, { 0, 0, 0, 0 }, 3 },
        { 0, 4, { 1, 0, 1, 0 }, 4 },
        { 0x13, 4, { 1, 1, 0, 1 }, 5 },
        { 0, 4, { 1, 1, 0, 0 }, 6 },
        { 0x13, 4, { 0, 0, 0, 0 }, 7 },
        { 3, 4, { 0, 0, 1, 0 }, 8 },
        { 7, 4, { 1, 0, 1, 1 }, 9 },
        { 0x13, 4, { 1, 0, 0, 1 }, 10 },
        { 0x13, 4, { 0, 0, 0, 0 }, 11 },
        { 0x13, 4, { 0, 0, 0, 0 }, 12 },
        { 0x13, 4, { 0, 0, 0, 0 }, 13 },
        { 0x13, 4, { 0, 0, 0, 0 }, 14 },
        { 0x13, 4, { 0, 0, 0, 0 }, 15 },
    },
};

static const InfinityDungeonFloor sFloorWhite2Mode1 = {
    {
        { 0x13, 0, { 0, 0, 1, 0 }, 0 },
        { 0, 4, { 0, 0, 1, 1 }, 1 },
        { 7, 4, { 0, 1, 0, 1 }, 2 },
        { 0x13, 4, { 0, 0, 0, 0 }, 3 },
        { 0x13, 1, { 0, 1, 0, 0 }, 4 },
        { 4, 4, { 0, 1, 1, 0 }, 5 },
        { 0x13, 4, { 1, 0, 0, 1 }, 6 },
        { 0x13, 4, { 0, 0, 0, 0 }, 7 },
        { 0x13, 4, { 1, 0, 1, 0 }, 8 },
        { 1, 4, { 1, 0, 1, 1 }, 9 },
        { 8, 4, { 0, 0, 0, 1 }, 10 },
        { 0x13, 4, { 0, 0, 0, 0 }, 11 },
        { 0x13, 4, { 0, 0, 0, 0 }, 12 },
        { 0x13, 4, { 0, 0, 0, 0 }, 13 },
        { 0x13, 4, { 0, 0, 0, 0 }, 14 },
        { 0x13, 4, { 0, 0, 0, 0 }, 15 },
    },
};

static const InfinityDungeonFloor sFloorBlack2Mode0 = {
    {
        { 0x13, 0, { 0, 1, 0, 0 }, 0 },
        { 0x13, 1, { 0, 1, 0, 0 }, 1 },
        { 0, 4, { 0, 1, 0, 0 }, 2 },
        { 0x13, 4, { 0, 0, 0, 0 }, 3 },
        { 0, 4, { 1, 0, 1, 0 }, 4 },
        { 0x13, 4, { 1, 1, 0, 1 }, 5 },
        { 0, 4, { 1, 1, 0, 0 }, 6 },
        { 0x13, 4, { 0, 0, 0, 0 }, 7 },
        { 3, 4, { 0, 0, 1, 0 }, 8 },
        { 7, 4, { 1, 0, 1, 1 }, 9 },
        { 0x13, 4, { 1, 0, 0, 1 }, 10 },
        { 0x13, 4, { 0, 0, 0, 0 }, 11 },
        { 0x13, 4, { 0, 0, 0, 0 }, 12 },
        { 0x13, 4, { 0, 0, 0, 0 }, 13 },
        { 0x13, 4, { 0, 0, 0, 0 }, 14 },
        { 0x13, 4, { 0, 0, 0, 0 }, 15 },
    },
};

static const u16 *InfinityDungeon_GetWallProps(InfinityDungeon *dungeon) {
    if (InfinityDungeon_GetVersion(dungeon) == VERSION_BLACK2) {
        return sBlackWallProps;
    }
    return sWhiteWallProps;
}

// Puts a wall prop in each passage of the room that is closed
void InfinityDungeonRoomChange_CreateWalls(InfinityDungeonRoomChange *roomChange, InfinityDungeonRoom *room, u32 floor,
                                           u32 roomIndex) {
    const u16 *props = InfinityDungeon_GetWallProps(roomChange->dungeon);
    InfinityDungeonGimmick *gimmick = Field_GetGimmickWorkBlock(roomChange->field, 0);
    SRTMatrix matrix;

    matrix.translation.x = 0;
    matrix.translation.y = 0;
    matrix.translation.z = 0;
    matrix.scale.x = FX32_ONE;
    matrix.scale.y = FX32_ONE;
    matrix.scale.z = FX32_ONE;
    MAT3_RotationEulerZYX(0, 0, 0, &matrix.rotation);
    if (room->kind != ROOM_KIND_GOAL) {
        if (room->kind != ROOM_KIND_START) {
            if (!InfinityDungeon_IsBelowGoal(roomChange->dungeon, floor, roomIndex) && room->open[0] == 0) {
                InfinityDungeonGimmick_SetWall(
                    gimmick, FieldPropSystem_CreateHandleNew(roomChange->propSystem, props[0], &matrix), 0);
            }
        }
        if (room->open[1] == 0) {
            InfinityDungeonGimmick_SetWall(
                gimmick, FieldPropSystem_CreateHandleNew(roomChange->propSystem, props[1], &matrix), 1);
        }
        if (room->open[3] == 0) {
            InfinityDungeonGimmick_SetWall(
                gimmick, FieldPropSystem_CreateHandleNew(roomChange->propSystem, props[3], &matrix), 3);
        }
        if (room->open[2] == 0) {
            InfinityDungeonGimmick_SetWall(
                gimmick, FieldPropSystem_CreateHandleNew(roomChange->propSystem, props[2], &matrix), 2);
        }
    }
}

// Puts the trainers of the room on both sides of each passage that is open
void InfinityDungeonRoomChange_CreateTrainers(InfinityDungeonRoomChange *roomChange, InfinityDungeonRoom *room) {
    u8 version = InfinityDungeon_GetVersion(roomChange->dungeon);

    if (room->kind == ROOM_KIND_GOAL) {
        return;
    }
    if (room->kind != ROOM_KIND_START) {
        BOOL visited = InfinityDungeon_IsBelowGoal(roomChange->dungeon, roomChange->nextFloor, roomChange->nextRoom);
        BOOL cleared = InfinityDungeon_IsTargetDefeated(roomChange->dungeon, roomChange->keyData);

        if (room->open[0] == 0 || (visited == TRUE && cleared == FALSE)) {
            const InfinityDungeonPos *src = InfinityDungeon_GetTrainerPos(version, room->kind, 0);
            InfinityDungeonPos pos = *src;

            InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
            pos.x = src->x + 1;
            InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
            pos.x = src->x - 1;
            InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
        } else if (visited == TRUE && cleared == TRUE) {
            const InfinityDungeonPos *src = InfinityDungeon_GetTrainerPos(version, room->kind, 0);
            InfinityDungeonPos pos = *src;

            pos.x = src->x + 1;
            InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
            pos.x = src->x - 1;
            InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
        }
    }
    if (room->open[1] == 0) {
        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, InfinityDungeon_GetTrainerPos(version, room->kind, 1));
    }
    if (room->open[3] == 0) {
        const InfinityDungeonPos *src = InfinityDungeon_GetTrainerPos(version, room->kind, 2);
        InfinityDungeonPos pos = *src;

        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
        pos.z = src->z - 1;
        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
        pos.z = src->z + 1;
        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
    }
    if (room->open[2] == 0) {
        const InfinityDungeonPos *src = InfinityDungeon_GetTrainerPos(version, room->kind, 3);
        InfinityDungeonPos pos = *src;

        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
        pos.z = src->z - 1;
        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
        pos.z = src->z + 1;
        InfinityDungeonRoomChange_CreateTrainerActor(roomChange, &pos);
    }
}

static void InfinityDungeonRoomChange_CreateTrainerActor(const InfinityDungeonRoomChange *roomChange,
                                                         const InfinityDungeonPos *pos) {
    FieldActor *actor = CreateNewActorByParam(roomChange->actorSystem, pos->x, pos->z, DIR_UP, 0xd1, 0xb9,
                                              MOVE_CODE_NONE, roomChange->destination.zoneId);
    SetActorSCRID(actor, 2000);
}

void InfinityDungeon_CreateCurrentWalls(GameSystem *gsys, InfinityDungeon *dungeon) {
    InfinityDungeonRoomChange *roomChange = GFL_HeapAllocate(
        HEAPID_TAIL(HEAPID_FIELDMAP), sizeof(InfinityDungeonRoomChange), TRUE, "infinity_dungeon.c", 1095);

    roomChange->dungeon = dungeon;
    InfinityDungeonRoomChange_Init(roomChange, gsys);
    InfinityDungeonRoomChange_CreateWalls(roomChange, InfinityDungeonRoomChange_GetRoom(roomChange), roomChange->floor,
                                          roomChange->room);
    GFL_HeapFree(roomChange);
}

u16 InfinityDungeon_GetEntryFacing(u16 direction) {
    switch (direction) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 1;
    default:
        return 1;
    }
}

void InfinityDungeonRoomChange_CreateNpcs(InfinityDungeonRoomChange *roomChange, InfinityDungeonRoom *room) {
    const InfinityDungeonRoomTemplate *roomTemplate;
    int i;

    if (InfinityDungeon_GetRoomNpcCount(room->type) != 0) {
        roomTemplate = InfinityDungeon_GetRoomTemplate(room->type);
        for (i = 0; i < InfinityDungeon_GetRoomNpcCount(room->type); i++) {
            const InfinityDungeonRoomNpc *roomNpc = &roomTemplate->npcs[i];
            const InfinityDungeonNpc *npc = InfinityDungeon_GetNpc(roomNpc->npc);
            const InfinityDungeonDestination *destination = &roomChange->destination;
            u8 slot = InfinityDungeon_GetTrainerSlot(roomChange->dungeon, roomChange->floor, room->index, i);
            u16 trainerId = InfinityDungeon_GetTrainerId(roomChange->dungeon, roomChange->floor, slot);

            if (trainerId != INFINITY_DUNGEON_TRAINER_NONE) {
                u16 objCode =
                    BtlInst_GetRomTrainerObjCode(INFINITY_DUNGEON_ARC_TRAINERS, trainerId, roomChange->heapId);
                FieldActor *actor =
                    CreateNewActorByParam(roomChange->actorSystem, roomNpc->x, roomNpc->y, npc->direction, 176 + slot,
                                          objCode, npc->moveCode, destination->zoneId);

                SetActorEvType(actor, npc->evType);
                SetActorUserParam(actor, npc->userParam0, 0);
                SetActorUserParam(actor, npc->userParam1, 1);
                SetActorAreaW(actor, npc->areaW);
                SetActorAreaH(actor, npc->areaH);
                SetActorSCRID(actor, INFINITY_DUNGEON_TRAINER_SCRIPT);
                ChangeActorMoveCodeSeq(actor, npc->moveCode);
            }
        }
    }
}

GameEvent *InfinityDungeon_CreateBattleEvent(GameSystem *gsys, Field *field, u8 a2) {
    KeyDataSave *keyData = getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    u32 mode = KeyDataSave_GetArea(keyData);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(field);
    u32 floor = KeyDataSave_GetFloorInArea(keyData);
    u16 seed = InfinityDungeon_GetBattleSeed(dungeon, floor, a2);
    u16 trainerId = InfinityDungeon_GetTrainerId(dungeon, floor, a2);
    u8 version = InfinityDungeon_GetVersion(dungeon);
    MATHRandContext32 rand;
    BSubwayTrainer trainer;
    void *trainerData;
    const InfinityDungeonLevelRange *levels;
    PokeParty *party;
    PokeParty *battleParty;
    BtlSetup *setup;
    int i;
    u8 partyCount;

    MATH_InitRand32(&rand, seed);
    trainerData = BtlInst_MakeRomTrainer(&trainer, INFINITY_DUNGEON_ARC_TRAINERS, trainerId, 0x35, HEAPID_FIELDMAP);
    BtlInst_MakeRomTeam(trainerData, trainerId, trainer.pokemon, 2, INFINITY_DUNGEON_ARC_POKEMON, NULL, NULL, NULL,
                        &rand, InfinityDungeon_GetIvs(mode), HEAPID_FIELDMAP);
    GFL_HeapFree(trainerData);
    levels = InfinityDungeon_GetLevelRange(mode);
    party = GameData_GetParty(GSYS_GetGameData(gsys));
    partyCount = PokeParty_GetPkmCount(party);
    battleParty = PokeParty_Create(HEAPID_TAIL(HEAPID_FIELDMAP));
    PokeParty_InitCore(battleParty, partyCount);
    for (i = 0; i < partyCount; i++) {
        PokeParty_AddPkm(battleParty, PokeParty_GetPkm(party, i));
    }
    setup = SetupInfinityDungeonBattle(gsys, battleParty, partyCount, 0, 2, &trainer, 0, 0x500, levels->maxLevel,
                                       levels->minLevel, 0, version, mode, field, &rand);
    GFL_HeapFree(battleParty);
    return EventInfinityDungeonBattleCall_Create(gsys, field, setup);
}

void InfinityDungeon_Setup(KeyDataSave *keyData, InfinityDungeon *dungeon) {
    u32 width;
    u32 height;
    u8 rank = KeyDataSave_GetArea(keyData);
    u8 floorCount;

    if (!InfinityDungeon_SetFixedFloors(keyData, dungeon)) {
        InfinityDungeon_GetFloorSize(rank, &width, &height);
        floorCount = KeyDataSave_GetAreaFloorCount(rank);
        InfinityDungeon_LayoutFloors(dungeon, rank, dungeon->layoutSeed, width, height, floorCount);
        InfinityDungeonCreator_Create(dungeon, rank, dungeon->trainerSeed, floorCount, HEAPID_FIELDMAP);
    } else {
        InfinityDungeon_SetFixedTrainers(keyData, dungeon);
    }
    InfinityDungeon_PickReward(dungeon, rank, dungeon->rewardSeed);
}

static BOOL InfinityDungeon_SetFixedFloors(KeyDataSave *keyData, InfinityDungeon *dungeon) {
    u32 progress0 = KeyDataSave_GetProgress(keyData, 0);
    u32 progress1 = KeyDataSave_GetProgress(keyData, 1);
    u32 rank = KeyDataSave_GetArea(keyData);
    u32 mode;
    int floor;
    int i;

    if (progress0 >= progress1) {
        progress1 = progress0;
    }
    if (progress1 >= 2 || progress1 > rank) {
        return FALSE;
    }

    switch (dungeon->version) {
    case VERSION_BLACK2:
        mode = 0;
        break;
    default:
        mode = 1;
        break;
    }
    mode = KeyDataSave_GetProgress(keyData, mode);
    {
        const InfinityDungeonFloor *white2Floors[2] = { &sFloorWhite2Mode0, &sFloorWhite2Mode1 };
        const InfinityDungeonFloor *black2Floors[2] = { &sFloorBlack2Mode0, &sFloorBlack2Mode1 };

        for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
            for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
                InfinityDungeonRoom *room = &dungeon->floors[floor].rooms[i];

                MI_CpuFill8(room, 0, sizeof(InfinityDungeonRoom));
                room->kind = 4;
                room->type = INFINITY_DUNGEON_ROOM_TYPE_NONE;
            }
        }
        if (dungeon->version == VERSION_BLACK2) {
            MI_CpuCopy8(black2Floors[mode], &dungeon->floors[0], sizeof(InfinityDungeonFloor));
        } else {
            MI_CpuCopy8(white2Floors[mode], &dungeon->floors[0], sizeof(InfinityDungeonFloor));
        }
    }
    return TRUE;
}

static void InfinityDungeon_SetFixedTrainers(KeyDataSave *keyData, InfinityDungeon *dungeon) {
    u32 mode;
    int i;
    int j;
    u32 index;

    switch (dungeon->version) {
    case VERSION_BLACK2:
        index = 0;
        break;
    default:
        index = 1;
        break;
    }
    mode = KeyDataSave_GetProgress(keyData, index);
    {
        const u16 *white2Lists[2][2] = { { sWhite2Trainers0, sWhite2Specials0 },
                                         { sWhite2Trainers1, sWhite2Specials1 } };
        const u16 *black2Lists[2][2] = { { sBlack2Trainers0, sBlack2Specials0 },
                                         { sBlack2Trainers1, sBlack2Specials1 } };

        MI_CpuFill8(dungeon->trainerIds, 0, sizeof(dungeon->trainerIds));
        MI_CpuFill8(dungeon->specialTrainers, 0, sizeof(dungeon->specialTrainers));
        if (dungeon->version == VERSION_BLACK2) {
            MI_CpuCopy8(black2Lists[mode][0], dungeon->trainerIds, sizeof(dungeon->specialTrainers));
            MI_CpuCopy16(black2Lists[mode][1], dungeon->specialTrainers, sizeof(dungeon->specialTrainers));
        } else {
            MI_CpuCopy8(white2Lists[mode][0], dungeon->trainerIds, sizeof(dungeon->specialTrainers));
            MI_CpuCopy16(white2Lists[mode][1], dungeon->specialTrainers, sizeof(dungeon->specialTrainers));
        }
    }
    MI_CpuFill8(dungeon->specialRoom, 0, sizeof(dungeon->specialRoom));
    for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
        InfinityDungeonRoom *room = &dungeon->floors[0].rooms[i];

        for (j = 0; j < InfinityDungeon_GetRoomNpcCount(room->type); j++) {
            u16 trainerId = dungeon->trainerIds[0][InfinityDungeon_GetTrainerSlot(dungeon, 0, room->index, j)];

            if (trainerId >= 640 && trainerId < INFINITY_DUNGEON_TRAINER_NONE) {
                dungeon->specialRoom[0] = room->index;
            }
        }
    }
    if (mode == 0) {
        dungeon->targetFloor = 0;
        dungeon->targetRoom = 2;
        dungeon->targetSlot = 0;
    } else {
        dungeon->targetFloor = 0;
        dungeon->targetRoom = 9;
        dungeon->targetSlot = 4;
    }
}

u32 InfinityDungeon_GetGoalFloor(InfinityDungeon *dungeon) {
    int i;
    int floor;

    for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
        for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
            if (dungeon->floors[floor].rooms[i].kind == 1) {
                return floor;
            }
        }
    }
    return 0;
}

static u32 InfinityDungeon_GetGoalRoom(InfinityDungeon *dungeon) {
    int i;
    int floor;

    for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
        for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
            if (dungeon->floors[floor].rooms[i].kind == 1) {
                return i;
            }
        }
    }
    return 0;
}

u32 InfinityDungeon_GetNpcScript(KeyDataSave *keyData, InfinityDungeon *dungeon, u16 objCode, u8 a3) {
    InfinityDungeonNpcScriptArg arg;
    u8 floor = KeyDataSave_GetFloorInArea(keyData);
    InfinityDungeonNpcScriptFunc firstTimeScripts[9] = {
        InfinityDungeonNpcScript_TargetDirection,
        InfinityDungeonNpcScript_TargetFloor,
        InfinityDungeonNpcScript_TargetTrainerCount,
        InfinityDungeonNpcScript_TargetTrainerSex,
        InfinityDungeonNpcScript_TargetTrainerTalk,
        InfinityDungeonNpcScript_SpecialDirection,
        InfinityDungeonNpcScript_Plain,
        InfinityDungeonNpcScript_Goal,
        InfinityDungeonNpcScript_Default,
    };
    InfinityDungeonNpcScriptFunc defeatedScripts[9] = {
        InfinityDungeonNpcScript_Default, InfinityDungeonNpcScript_Default, InfinityDungeonNpcScript_Default,
        InfinityDungeonNpcScript_Default, InfinityDungeonNpcScript_Default, InfinityDungeonNpcScript_SpecialDirection,
        InfinityDungeonNpcScript_Plain,   InfinityDungeonNpcScript_Goal,    InfinityDungeonNpcScript_Default,
    };
    BOOL defeated = InfinityDungeon_IsTargetDefeated(dungeon, keyData);
    u8 npcIndex = InfinityDungeon_GetNpcIndex(objCode);
    u32 scriptIndex = InfinityDungeon_GetNpcScriptIndex(dungeon, objCode, floor, a3);

    arg.dungeon = dungeon;
    arg.npcIndex = npcIndex;
    arg.keyData = keyData;
    if (defeated == 0) {
        return firstTimeScripts[scriptIndex](&arg);
    } else {
        return defeatedScripts[scriptIndex](&arg);
    }
}

u32 InfinityDungeon_GetNpcScriptByResult(KeyDataSave *keyData, InfinityDungeon *dungeon, u16 objCode, u16 a3) {
    u32 npcIndex = InfinityDungeon_GetNpcIndex(objCode);

    if (a3 == 1) {
        return 0x1ec + npcIndex;
    }
    return 0x1f6 + npcIndex;
}

static u32 InfinityDungeonNpcScript_TargetFloor(InfinityDungeonNpcScriptArg *arg) {
    u32 floor = KeyDataSave_GetFloorInArea(arg->keyData);
    u32 script;

    if (floor < arg->dungeon->targetFloor) {
        if (InfinityDungeon_GetVersion(arg->dungeon) == VERSION_BLACK2) {
            script = 0xa0;
        } else {
            script = 0xa8;
        }
    } else if (floor > arg->dungeon->targetFloor) {
        if (InfinityDungeon_GetVersion(arg->dungeon) == VERSION_BLACK2) {
            script = 0xa8;
        } else {
            script = 0xa0;
        }
    } else {
        script = 0xb0;
    }
    return script + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_GoalFloor(InfinityDungeonNpcScriptArg *arg) {
    u32 floor = KeyDataSave_GetFloorInArea(arg->keyData);
    u8 goalFloor = InfinityDungeon_GetGoalFloor(arg->dungeon);

    if (floor != goalFloor) {
        return 0x200 + arg->npcIndex;
    }
    return 0x208 + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_GoalDirection(InfinityDungeonNpcScriptArg *arg) {
    u32 floor = KeyDataSave_GetFloorInArea(arg->keyData);
    u8 goalFloor = InfinityDungeon_GetGoalFloor(arg->dungeon);
    u8 goalRoom = InfinityDungeon_GetGoalRoom(arg->dungeon);
    u8 room = KeyDataSave_GetRoom(arg->keyData);
    u32 script = 0x1d8;

    if (floor == goalFloor) {
        u8 x = 0;
        u8 y = 0;
        u8 goalX = 0;
        u8 goalY = 0;

        if (room != 0) {
            x = room % INFINITY_DUNGEON_GRID_WIDTH;
            y = room / INFINITY_DUNGEON_GRID_WIDTH;
        }
        if (goalRoom != 0) {
            goalX = goalRoom % INFINITY_DUNGEON_GRID_WIDTH;
            goalY = goalRoom / INFINITY_DUNGEON_GRID_WIDTH;
        }
        switch (InfinityDungeon_GetSideOfRoom(x, y, goalX, goalY)) {
        case 0:
            script = 0x210;
            break;
        case 1:
            script = 0x218;
            break;
        case 2:
            script = 0x228;
            break;
        case 3:
            script = 0x220;
            break;
        case 4:
            script = 0x240;
            break;
        case 5:
            script = 0x230;
            break;
        case 6:
            script = 0x248;
            break;
        case 7:
            script = 0x238;
            break;
        default:
            script = 0x210;
            break;
        }
    }
    return script + arg->npcIndex;
}

// Which of the eight sides of (targetX, targetY) the room (x, y) is on, 0 for the same room
static u32 InfinityDungeon_GetSideOfRoom(u8 x, u8 y, u8 targetX, u8 targetY) {
    if (x == targetX) {
        return (u8)(y <= targetY);
    }
    if (y == targetY) {
        return x < targetX ? 3 : 2;
    }
    if (y > targetY) {
        return x < targetX ? 5 : 4;
    }
    return x < targetX ? 7 : 6;
}

static u32 InfinityDungeonNpcScript_Plain(InfinityDungeonNpcScriptArg *arg) {
    return 0x150 + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_SpecialDirection(InfinityDungeonNpcScriptArg *arg) {
    u32 floor = KeyDataSave_GetFloorInArea(arg->keyData);
    u8 room = KeyDataSave_GetRoom(arg->keyData);
    u8 specialRoom = arg->dungeon->specialRoom[floor];
    u32 version = InfinityDungeon_GetVersion(arg->dungeon);
    u8 x = 0;
    u8 y = 0;
    u8 specialX = 0;
    u8 specialY = 0;
    u32 black2Scripts[8] = { 0x198, 0x1a0, 0x1b0, 0x1a8, 0x1c8, 0x1b8, 0x1d0, 0x1c0 };
    u32 white2Scripts[8] = { 0x158, 0x160, 0x170, 0x168, 0x188, 0x178, 0x190, 0x180 };
    u32 *scripts = black2Scripts;
    u32 script;

    if (version != VERSION_BLACK2) {
        scripts = white2Scripts;
    }
    if (room == specialRoom) {
        return 0x1e0 + arg->npcIndex;
    }
    if (room != 0) {
        x = room % INFINITY_DUNGEON_GRID_WIDTH;
        y = room / INFINITY_DUNGEON_GRID_WIDTH;
    }
    if (specialRoom != 0) {
        specialX = specialRoom % INFINITY_DUNGEON_GRID_WIDTH;
        specialY = specialRoom / INFINITY_DUNGEON_GRID_WIDTH;
    }
    script = scripts[InfinityDungeon_GetSideOfRoom(x, y, specialX, specialY)];
    return script + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_TargetDirection(InfinityDungeonNpcScriptArg *arg) {
    u32 floor = KeyDataSave_GetFloorInArea(arg->keyData);
    u8 room = KeyDataSave_GetRoom(arg->keyData);
    u8 targetRoom = arg->dungeon->targetRoom;
    u8 targetFloor = arg->dungeon->targetFloor;
    u8 x = 0;
    u8 y = 0;
    u8 targetX = 0;
    u8 targetY = 0;
    u32 script;

    if (targetFloor != floor) {
        return 0x1e0 + arg->npcIndex;
    }
    if (room == targetRoom) {
        return 0x1e0 + arg->npcIndex;
    }
    if (room != 0) {
        x = room % INFINITY_DUNGEON_GRID_WIDTH;
        y = room / INFINITY_DUNGEON_GRID_WIDTH;
    }
    if (targetRoom != 0) {
        targetX = targetRoom % INFINITY_DUNGEON_GRID_WIDTH;
        targetY = targetRoom / INFINITY_DUNGEON_GRID_WIDTH;
    }
    switch (InfinityDungeon_GetSideOfRoom(x, y, targetX, targetY)) {
    case 0:
        script = 0xc0;
        break;
    case 1:
        script = 0xc8;
        break;
    case 2:
        script = 0xd8;
        break;
    case 3:
        script = 0xd0;
        break;
    case 4:
        script = 0xf0;
        break;
    case 5:
        script = 0xe0;
        break;
    case 6:
        script = 0xf8;
        break;
    case 7:
        script = 0xe8;
        break;
    default:
        script = 0xc0;
        break;
    }
    return script + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_TargetTrainerCount(InfinityDungeonNpcScriptArg *arg) {
    InfinityDungeon *dungeon = arg->dungeon;

    switch (InfinityDungeon_GetRoomNpcCount(dungeon->floors[dungeon->targetFloor].rooms[dungeon->targetRoom].type)) {
    case 1:
        return 0x100 + arg->npcIndex;
    case 2:
        return 0x108 + arg->npcIndex;
    case 3:
        return 0x110 + arg->npcIndex;
    case 4:
        return 0x118 + arg->npcIndex;
    default:
        return 0x1e0 + arg->npcIndex;
    }
}

static u32 InfinityDungeonNpcScript_TargetTrainerSex(InfinityDungeonNpcScriptArg *arg) {
    BSubwayTrainer trainer;
    u16 *trainerData = BtlInst_MakeRomTrainer(
        &trainer, INFINITY_DUNGEON_ARC_TRAINERS,
        InfinityDungeon_GetTrainerId(arg->dungeon, arg->dungeon->targetFloor, arg->dungeon->targetSlot), 0x35,
        HEAPID_FIELDMAP);
    u16 trainerClass = trainerData[0];

    GFL_HeapFree(trainerData);
    if (TrainerClass_GetSex(trainerClass) == 0) {
        return 0x120 + arg->npcIndex;
    }
    return 0x128 + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_TargetTrainerTalk(InfinityDungeonNpcScriptArg *arg) {
    u8 floor = arg->dungeon->targetFloor;
    u8 room = arg->dungeon->targetRoom;
    InfinityDungeonRoom *rooms = arg->dungeon->floors[floor].rooms;
    const InfinityDungeonRoomTemplate *roomTemplate = InfinityDungeon_GetRoomTemplate(rooms[room].type);
    int i;

    for (i = 0; i < InfinityDungeon_GetRoomNpcCount(rooms[room].type); i++) {
        if (InfinityDungeon_IsTargetSlot(arg->dungeon, floor,
                                         InfinityDungeon_GetTrainerSlot(arg->dungeon, floor, room, i)) == TRUE) {
            const InfinityDungeonNpc *npc = InfinityDungeon_GetNpc(roomTemplate->npcs[i].npc);

            return npc->script + arg->npcIndex;
        }
    }
    return 0x1e0 + arg->npcIndex;
}

static u32 InfinityDungeonNpcScript_Goal(InfinityDungeonNpcScriptArg *arg) {
    if (KeyDataSave_GetAreaFloorCount(KeyDataSave_GetArea(arg->keyData)) <= 1) {
        return InfinityDungeonNpcScript_GoalDirection(arg);
    }
    return InfinityDungeonNpcScript_GoalFloor(arg);
}

static u32 InfinityDungeonNpcScript_Default(InfinityDungeonNpcScriptArg *arg) {
    return 0x1e0 + arg->npcIndex;
}

static u16 InfinityDungeon_GetBattleSeed(InfinityDungeon *dungeon, u8 floor, u8 slot) {
    return dungeon->battleSeeds[floor][slot];
}

u32 InfinityDungeon_FindSpecialTrainer(InfinityDungeon *dungeon, u16 trainerId) {
    u32 i;
    const u16 *specials = dungeon->specialTrainers;

    for (i = 0; i < INFINITY_DUNGEON_FLOOR_TRAINER_MAX; i++) {
        if (trainerId == specials[i]) {
            return i;
        }
    }
    return 0;
}

BOOL InfinityDungeonRoomChange_UsesFade(InfinityDungeonRoomChange *roomChange) {
    if (roomChange->mode == 1 || roomChange->direction == 4) {
        return TRUE;
    }
    return FALSE;
}

u32 InfinityDungeon_CountTrainers(InfinityDungeon *dungeon) {
    u32 total = 0;
    int floor;
    int i;

    for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
        for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
            if (dungeon->floors[floor].rooms[i].type != INFINITY_DUNGEON_ROOM_TYPE_NONE) {
                total += InfinityDungeon_GetRoomNpcCount(dungeon->floors[floor].rooms[i].type);
            }
        }
    }
    return total;
}

BOOL InfinityDungeon_IsBelowGoal(InfinityDungeon *dungeon, u32 floor, u32 room) {
    int above = room - 4;

    if (above < 0) {
        return 0;
    }
    if (dungeon->floors[floor].rooms[above].kind == 1) {
        return 1;
    }
    return 0;
}

BOOL InfinityDungeon_IsTargetDefeated(InfinityDungeon *dungeon, KeyDataSave *keyData) {
    return KeyDataSave_IsTrainerDefeated(keyData, dungeon->targetFloor, dungeon->targetSlot);
}

static const u16 sRewardItemsRank0to4[6] = {
    ITEM_PROTEIN, ITEM_IRON, ITEM_CALCIUM, ITEM_ZINC, ITEM_CARBOS, ITEM_HP_UP,
};

static const u16 sRewardWeightsRank0to4[6] = {
    0x11, 0x11, 0x11, 0x11, 0x10, 0x10,
};

static const u16 sRewardItemsRank9[7] = {
    ITEM_POMEG_BERRY, ITEM_KELPSY_BERRY, ITEM_QUALOT_BERRY, ITEM_HONDEW_BERRY,
    ITEM_GREPA_BERRY, ITEM_TAMATO_BERRY, ITEM_RARE_CANDY,
};

static const u16 sRewardItemsRank5to8[7] = {
    ITEM_HEALTH_WING, ITEM_MUSCLE_WING, ITEM_RESIST_WING, ITEM_GENIUS_WING,
    ITEM_CLEVER_WING, ITEM_SWIFT_WING,  ITEM_RARE_CANDY,
};

static const u16 sRewardWeightsRank5to8[7] = {
    0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xa,
};

static const u16 sRewardWeightsRank9[7] = {
    0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xa,
};

static void InfinityDungeon_PickReward(InfinityDungeon *dungeon, u32 rank, u32 seed) {
    u8 total = 0;
    u8 counts[10] = { 1, 1, 1, 2, 2, 1, 1, 2, 2, 1 };
    const u16 *items[10] = { sRewardItemsRank0to4, sRewardItemsRank0to4, sRewardItemsRank0to4, sRewardItemsRank0to4,
                             sRewardItemsRank0to4, sRewardItemsRank5to8, sRewardItemsRank5to8, sRewardItemsRank5to8,
                             sRewardItemsRank5to8, sRewardItemsRank9 };
    const u16 *weights[10] = { sRewardWeightsRank0to4, sRewardWeightsRank0to4, sRewardWeightsRank0to4,
                               sRewardWeightsRank0to4, sRewardWeightsRank0to4, sRewardWeightsRank5to8,
                               sRewardWeightsRank5to8, sRewardWeightsRank5to8, sRewardWeightsRank5to8,
                               sRewardWeightsRank9 };
    const u16 itemCounts[10] = { 6, 6, 6, 6, 6, 7, 7, 7, 7, 7 };
    MATHRandContext32 rand;
    u8 roll;
    int i;

    MATH_InitRand32(&rand, seed);
    roll = MATH_Rand32(&rand, 100) + 1;
    for (i = 0; i < itemCounts[rank]; i++) {
        total += weights[rank][i];
        if (roll <= total) {
            dungeon->rewardItem = items[rank][i];
            break;
        }
    }
    dungeon->rewardCount = counts[rank];
}
