// The Black Tower and White Treehollow's creator: from the dungeon's trainer seed it picks the trainers of each floor,
// the goal's room and trainer, the reward and the special trainers that stand on the floors. Named after the ROM's
// "infinity_dungeon_creator.c"
#include "field/infinity_dungeon_creator.h"
#include "types.h"
#include "constants/version.h"
#include "field/infinity_dungeon.h"
#include "field/infinity_dungeon_rooms.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// The dungeon's own trainers, apart from the game's trainer table
#define INFINITY_DUNGEON_TRAINER_COUNT 640
// The ten special trainers of Black 2 come first, then White 2's, and a rank has five of them
#define INFINITY_DUNGEON_SPECIAL_BLACK2 640
#define INFINITY_DUNGEON_SPECIAL_WHITE2 650
#define INFINITY_DUNGEON_SPECIAL_RANK_COUNT 5
#define INFINITY_DUNGEON_TRAINER_CLASS_COUNT 30

static int InfinityDungeonCreator_PickTrainers(InfinityDungeon *dungeon, u32 rank, u32 *trainers, InfinityDungeonRandom *rnd, HeapID heapId);
static int InfinityDungeonCreator_ListTrainerRooms(InfinityDungeon *dungeon, int floor, u32 *rooms, u32 *counts);
static void InfinityDungeonCreator_PlaceSpecialTrainer(InfinityDungeon *dungeon, const u16 *specials, int floor, u32 rank, const u32 *rooms,
                                int roomCount, const u32 *counts, InfinityDungeonRandom *rnd, int *specialCount);
static int InfinityDungeonCreator_ListGoalCandidates(InfinityDungeon *dungeon, u8 floor, u32 *slots, u32 *rooms);
static void InfinityDungeonCreator_PickSpecialTrainers(InfinityDungeon *dungeon, u32 rank, u16 *specials, InfinityDungeonRandom *rnd);

static inline u32 InfinityDungeonRandom_Next(InfinityDungeonRandom *rnd, u32 max) {
    rnd->state = rnd->multiplier * rnd->state + rnd->increment;
    if (max == 0) {
        return rnd->state >> 32;
    }
    return ((rnd->state >> 32) * max) >> 32;
}

void InfinityDungeonCreator_Create(InfinityDungeon *dungeon, u32 rank, u32 seed, u8 floorCount, HeapID heapId) {
    InfinityDungeonRandom rnd;
    u32 goalSlots[INFINITY_DUNGEON_FLOOR_TRAINER_MAX];
    u32 goalRooms[INFINITY_DUNGEON_FLOOR_TRAINER_MAX];
    u16 specials[INFINITY_DUNGEON_SPECIAL_RANK_COUNT];
    u32 rooms[INFINITY_DUNGEON_ROOM_COUNT];
    u32 counts[INFINITY_DUNGEON_ROOM_COUNT];
    int specialCount;
    int goalCount;
    int roomCount;
    int floor;
    u8 pick;
    int i;
    u32 *trainers;

    rnd.multiplier = 0x5d588b656c078965ULL;
    rnd.state = seed;
    rnd.increment = 0x269ec3;
    trainers = GFL_HeapAllocate(HEAPID_TAIL(heapId), 2000, TRUE, "infinity_dungeon_creator.c", 915);

    for (floor = 0; floor < floorCount; floor++) {
        int count;

        sys_memset(trainers, 0xff, 2000);
        count = InfinityDungeonCreator_PickTrainers(dungeon, rank, trainers, &rnd, heapId);
        sys_memset(dungeon->trainerIds[floor], 0xff, sizeof(dungeon->trainerIds[floor]));
        func_ov127_021f0bfc(trainers, count, &rnd);
        for (i = 0; i < INFINITY_DUNGEON_FLOOR_TRAINER_MAX; i++) {
            dungeon->trainerIds[floor][i] = trainers[i];
        }
        dungeon->specialRoom[floor] = INFINITY_DUNGEON_ROOM_COUNT;
    }

    dungeon->targetFloor = InfinityDungeonRandom_Next(&rnd, floorCount);
    goalCount = InfinityDungeonCreator_ListGoalCandidates(dungeon, dungeon->targetFloor, goalSlots, goalRooms);
    if (goalCount <= 0) {
        return;
    }
    pick = InfinityDungeonRandom_Next(&rnd, goalCount);
    dungeon->targetRoom = goalRooms[pick];
    dungeon->targetSlot = goalSlots[pick];

    specialCount = 0;
    InfinityDungeonCreator_PickSpecialTrainers(dungeon, rank, specials, &rnd);
    for (floor = 0; floor < floorCount; floor++) {
        sys_memset(rooms, 0, sizeof(rooms));
        sys_memset(counts, 0, sizeof(counts));
        roomCount = InfinityDungeonCreator_ListTrainerRooms(dungeon, floor, rooms, counts);
        if (roomCount <= 0) {
            return;
        }
        InfinityDungeonCreator_PlaceSpecialTrainer(dungeon, specials, floor, rank, rooms, roomCount, counts, &rnd, &specialCount);
    }
    GFL_HeapFree(trainers);
}

// The trainers of a rank, in the order of the rank's classes, each class's share shuffled
static int InfinityDungeonCreator_PickTrainers(InfinityDungeon *dungeon, u32 rank, u32 *trainers, InfinityDungeonRandom *rnd, HeapID heapId) {
    int trainerCount = 0;
    u32 *candidates;
    u32 *group;
    u8 version;
    u32 candidateCount;
    u8 trainerClass;
    const u8 *classCounts;
    int i;

    version = func_ov127_021f0dd8(dungeon);
    candidateCount = 0;
    candidates = GFL_HeapAllocate(HEAPID_TAIL(heapId), 2000, TRUE, "infinity_dungeon_creator.c", 1022);
    group = GFL_HeapAllocate(HEAPID_TAIL(heapId), 2000, TRUE, "infinity_dungeon_creator.c", 1023);
    for (i = 0; i < INFINITY_DUNGEON_TRAINER_COUNT; i++) {
        const InfinityDungeonTrainer *trainer = InfinityDungeon_GetTrainer(i);

        if (rank >= trainer->minRank && rank <= trainer->maxRank &&
            (trainer->version == version || trainer->version == 0)) {
            candidates[candidateCount] = i;
            candidateCount++;
        }
    }
    classCounts = func_ov127_021f0b74(rank, version);
    for (trainerClass = 0; trainerClass < INFINITY_DUNGEON_TRAINER_CLASS_COUNT; trainerClass++) {
        int count = classCounts[trainerClass];

        if (count != 0) {
            int picked = 0;
            int groupCount = 0;
            int j;

            sys_memset(group, 0, sizeof(u32));
            for (j = 0; j < candidateCount; j++) {
                if (trainerClass == func_ov127_021f0a90(candidates[j])) {
                    group[groupCount] = candidates[j];
                    groupCount++;
                }
            }
            func_ov127_021f0bfc(group, groupCount, rnd);
            while (count > 0) {
                trainers[trainerCount] = group[picked];
                trainerCount++;
                count--;
                picked++;
            }
        }
    }
    GFL_HeapFree(candidates);
    GFL_HeapFree(group);
    return trainerCount;
}

// The rooms of a floor that hold trainers, except the goal's, and how many each holds
static int InfinityDungeonCreator_ListTrainerRooms(InfinityDungeon *dungeon, int floor, u32 *rooms, u32 *counts) {
    BOOL isTargetFloor = FALSE;
    int room;
    int count = 0;

    if (dungeon->targetFloor == floor) {
        isTargetFloor = TRUE;
    }
    for (room = 0; room < INFINITY_DUNGEON_ROOM_COUNT; room++) {
        if (room != dungeon->targetRoom || !isTargetFloor) {
            u8 type = dungeon->floors[floor].rooms[room].type;

            if (InfinityDungeon_GetRoomNpcCount(type) != 0) {
                rooms[count] = room;
                counts[count] = InfinityDungeon_GetRoomNpcCount(type);
                count++;
            }
        }
    }
    return count;
}

// Puts the next special trainer on the floor, in the goal's trainer's place when the goal is on another floor and the
// trainer is on this one, or else in a trainer's place of a random room
static void InfinityDungeonCreator_PlaceSpecialTrainer(InfinityDungeon *dungeon, const u16 *specials, int floor, u32 rank, const u32 *rooms,
                                int roomCount, const u32 *counts, InfinityDungeonRandom *rnd, int *specialCount) {
    u16 special = specials[*specialCount];

    if (dungeon->targetFloor != floor) {
        u16 goalTrainer = InfinityDungeon_GetTrainerId(dungeon, dungeon->targetFloor, dungeon->targetSlot);
        int i;
        int total = 0;
        int slot;

        for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
            total += InfinityDungeon_GetRoomNpcCount(dungeon->floors[floor].rooms[i].type);
        }
        for (slot = 0; slot < total; slot++) {
            if (goalTrainer == dungeon->trainerIds[floor][slot]) {
                break;
            }
        }
        if (slot < total) {
            dungeon->trainerIds[floor][slot] = special;
            dungeon->specialTrainers[*specialCount] = special;
            dungeon->specialRoom[floor] = func_ov127_021f0de4(dungeon, floor, slot);
            (*specialCount)++;
            return;
        }
    }

    {
        u32 pick = InfinityDungeonRandom_Next(rnd, roomCount);
        u32 slotInRoom = InfinityDungeonRandom_Next(rnd, counts[pick]);
        u32 slot = InfinityDungeon_GetTrainerSlot(dungeon, floor, rooms[pick], slotInRoom);

        dungeon->trainerIds[floor][slot] = special;
        dungeon->specialTrainers[*specialCount] = special;
        (*specialCount)++;
        dungeon->specialRoom[floor] = rooms[pick];
    }
}

// The trainer slots and rooms where the goal can stand: every slot of an ordinary room that isn't the one four rooms
// before the floor's goal room
static int InfinityDungeonCreator_ListGoalCandidates(InfinityDungeon *dungeon, u8 floor, u32 *slots, u32 *rooms) {
    int count = 0;
    int goal = -1;
    int room;

    for (room = 0; room < INFINITY_DUNGEON_ROOM_COUNT; room++) {
        u8 kind = dungeon->floors[floor].rooms[room].kind;
        u8 type = dungeon->floors[floor].rooms[room].type;

        if (kind == 1) {
            goal = room;
        } else if (kind == 4) {
            if (goal == -1 || goal != room - 4) {
                u8 slot = InfinityDungeon_GetRoomNpcCount(type);

                while (slot != 0) {
                    slot--;
                    slots[count] = InfinityDungeon_GetTrainerSlot(dungeon, floor, room, slot);
                    rooms[count++] = room;
                }
            }
        }
    }
    return count;
}

// The five special trainers of a rank, shuffled
static void InfinityDungeonCreator_PickSpecialTrainers(InfinityDungeon *dungeon, u32 rank, u16 *specials, InfinityDungeonRandom *rnd) {
    u8 version = func_ov127_021f0dd8(dungeon);
    u16 first;
    int i;

    switch (rank) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        if (version == VERSION_BLACK2) {
            first = INFINITY_DUNGEON_SPECIAL_BLACK2;
        } else {
            first = INFINITY_DUNGEON_SPECIAL_WHITE2;
        }
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        if (version == VERSION_BLACK2) {
            first = INFINITY_DUNGEON_SPECIAL_BLACK2 + INFINITY_DUNGEON_SPECIAL_RANK_COUNT;
        } else {
            first = INFINITY_DUNGEON_SPECIAL_WHITE2 + INFINITY_DUNGEON_SPECIAL_RANK_COUNT;
        }
        break;
    default:
        first = INFINITY_DUNGEON_SPECIAL_BLACK2;
        break;
    }
    for (i = 0; i < INFINITY_DUNGEON_SPECIAL_RANK_COUNT; i++) {
        specials[i] = first + i;
    }
    func_ov127_021f0ca8(specials, INFINITY_DUNGEON_SPECIAL_RANK_COUNT, rnd);
}
