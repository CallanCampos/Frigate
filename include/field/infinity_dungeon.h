#ifndef POKEBW2_FIELD_INFINITY_DUNGEON_H
#define POKEBW2_FIELD_INFINITY_DUNGEON_H

#include "types.h"
#include "struct_decls.h"

// infinity_dungeon.c, overlay 127, which the ROM names: the Black Tower and White Treehollow, five floors of 4x4 rooms
// that infinity_dungeon_creator.c lays out from a seed, with their trainers and the reward at the end. Script plugin
// 61 drives it

#define INFINITY_DUNGEON_FLOOR_COUNT 5
#define INFINITY_DUNGEON_ROOM_COUNT 16
#define INFINITY_DUNGEON_FLOOR_TRAINER_MAX 32
// A room's type that isn't set yet, past the 19 templates
#define INFINITY_DUNGEON_ROOM_TYPE_NONE 19

// The room kinds that the room templates give
#define ROOM_KIND_START 0
#define ROOM_KIND_GOAL 1

// A room of a floor's grid, x + y * 4
typedef struct {
    u8 type;
    u8 kind; // the template's class: 1 for the floor's goal room, 4 for an ordinary room not laid out yet
    u32 open[4]; // the passages north, south, east and west
    u8 index;
} InfinityDungeonRoom;

typedef struct {
    InfinityDungeonRoom rooms[INFINITY_DUNGEON_ROOM_COUNT];
} InfinityDungeonFloor;

// The work of the whole dungeon, which the gimmick allocates
struct InfinityDungeon {
    u32 seed; // from the save, and the three derived from it
    u32 trainerSeed;
    u32 layoutSeed;
    u32 rewardSeed;
    u16 battleSeeds[INFINITY_DUNGEON_FLOOR_COUNT][INFINITY_DUNGEON_FLOOR_TRAINER_MAX];
    InfinityDungeonFloor floors[INFINITY_DUNGEON_FLOOR_COUNT];
    u16 trainerIds[INFINITY_DUNGEON_FLOOR_COUNT][INFINITY_DUNGEON_FLOOR_TRAINER_MAX]; // 0xffff for none
    u16 specialTrainers[INFINITY_DUNGEON_FLOOR_TRAINER_MAX];
    u16 specialRoom[INFINITY_DUNGEON_FLOOR_COUNT]; // the room of each floor's special trainer, 16 for none
    u8 targetFloor;
    u8 targetRoom;
    u8 targetSlot;
    u16 rewardItem;
    u8 rewardCount;
    u8 version; // the save's, VERSION_BLACK2 or VERSION_WHITE2
};

// A 64-bit linear congruential generator
typedef struct {
    u64 state;
    u64 multiplier;
    u64 increment;
} InfinityDungeonRandom;

// A position in tiles of a room, which the room's tables give for its doors, passages and entrances
typedef struct {
    s32 x;
    s32 y;
    s32 z;
} InfinityDungeonPos;

// An NPC of the dungeon: how it moves and acts, how large the area it notices the player in is, and its script
typedef struct {
    u16 moveCode;
    u16 evType;
    u16 direction;
    u16 userParam0;
    u16 userParam1;
    s32 areaW;
    s32 areaH;
    u16 script;
} InfinityDungeonNpc;

// An NPC of a room template, and where it stands
typedef struct {
    u16 npc;
    s32 x;
    s32 y;
} InfinityDungeonRoomNpc;

// One of the 19 kinds of room, with the NPCs that stand in it
typedef struct {
    u8 group;
    u8 npcCount;
    u32 kind;
    InfinityDungeonRoomNpc npcs[4];
} InfinityDungeonRoomTemplate;

// The levels of the dungeon's trainers' Pokémon in a rank
typedef struct {
    u16 minLevel;
    u16 maxLevel;
} InfinityDungeonLevelRange;

void func_ov127_021efd60(GameSystem *gsys, InfinityDungeon *gimmick);
GameEvent *func_ov127_021efeec(GameSystem *gsys, Field *field, u8 a2);
u32 func_ov127_021f02e0(InfinityDungeon *gimmick);
u32 InfinityDungeon_GetNpcScript(KeyDataSave *keyData, InfinityDungeon *gimmick, u16 objCode, u8 a3);
u32 func_ov127_021f03dc(KeyDataSave *keyData, InfinityDungeon *gimmick, u16 objCode, u16 a3);
u32 func_ov127_021f08b4(InfinityDungeon *gimmick, u16 a1);
u32 func_ov127_021f08e8(InfinityDungeon *gimmick);
BOOL func_ov127_021f093c(InfinityDungeon *gimmick, u32 floor, u32 room);
u16 func_ov127_021efdac(u16 direction);
void func_ov127_021efde0(InfinityDungeonRoomChange *roomChange, InfinityDungeonRoom *room);
void func_ov127_021f0040(KeyDataSave *keyData, InfinityDungeon *gimmick);
BOOL func_ov127_021f08d0(InfinityDungeonRoomChange *roomChange);
BOOL func_ov127_021f096c(InfinityDungeon *gimmick, KeyDataSave *keyData);

// One of the dungeon's own 640 trainers, which stands from minRank to maxRank for a version, 0 for both
typedef struct {
    u8 minRank;
    u8 maxRank;
    u8 version;
    u8 trainerClass;
} InfinityDungeonTrainer;

#endif // POKEBW2_FIELD_INFINITY_DUNGEON_H
