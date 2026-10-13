// The Black Tower and White Treehollow's dungeon, its tables and its layout: the templates of the 19 kinds of rooms and
// of the NPCs in them, the 640 trainers and the tables by rank, the shuffles of the generator, the seeds, the trainer
// slots of a room, and the maze that digs the passages of each floor and puts the rooms in it. The ROM has no name for
// this file, which is the one after infinity_dungeon.c: named after what it holds
#include "field/infinity_dungeon_rooms.h"
#include "types.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "field/infinity_dungeon.h"
#include "gfl/std.h"
#include "nitro/math.h"

// The grid a floor's rooms are in is 4 rooms wide, and a walk can go back at most this many rooms
#define INFINITY_DUNGEON_GRID_WIDTH 4
#define INFINITY_DUNGEON_WALK_DEPTH 100

typedef struct {
    u32 width;
    u32 height;
} InfinityDungeonSize;

typedef struct {
    u32 objCode;
    u8 scriptIndex;
    u8 npcIndex;
} InfinityDungeonObjScript;

// A room of a floor's grid
typedef struct {
    int x;
    int y;
} InfinityDungeonGridPos;

// A walk through the grid that digs the passages, with the rooms it can go back to
typedef struct {
    int top;
    InfinityDungeonGridPos cur;
    InfinityDungeonGridPos stack[INFINITY_DUNGEON_WALK_DEPTH];
} InfinityDungeonWalk;


static void func_ov127_021f0c54(u8 *array, int count, MATHRandContext32 *rand);
static const u8 *func_ov127_021f0d88(u16 rank, u8 floor, u8 version);
static u8 func_ov127_021f1308(u32 rank, u8 floor, u8 version, u8 *types);
static u8 func_ov127_021f1348(u32 rank, u8 floor, u8 version, u8 *types);
static int func_ov127_021f1388(InfinityDungeonRoom *rooms, int *list, int width, int height);
static InfinityDungeonRoom *func_ov127_021f13e8(const InfinityDungeonGridPos *pos, InfinityDungeonRoom *rooms);
static const InfinityDungeonRoom *func_ov127_021f13f8(const InfinityDungeonGridPos *pos, const InfinityDungeonRoom *rooms);
static void func_ov127_021f1408(u8 direction, InfinityDungeonWalk *walk, InfinityDungeonRoom *rooms);
static int func_ov127_021f14c0(const InfinityDungeonGridPos *pos, const InfinityDungeonRoom *rooms, int *directions,
                               int width, int height, BOOL allowVisited);
static BOOL func_ov127_021f15d8(const InfinityDungeonGridPos *pos, const InfinityDungeonRoom *rooms);
static void func_ov127_021f15f8(InfinityDungeonWalk *walk, const InfinityDungeonGridPos *pos);
static void func_ov127_021f160c(InfinityDungeonWalk *walk);
static BOOL func_ov127_021f1634(const InfinityDungeonRoom *rooms, int width, int height);

static const u8 sIvs[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static const u16 sBlack2Zones[6] = { ZONE_BLACK_TOWER_2, ZONE_BLACK_TOWER_3, ZONE_BLACK_TOWER_5, ZONE_BLACK_TOWER_4, ZONE_BLACK_TOWER_6, ZONE_BLACK_TOWER };

static const u16 sWhite2Zones[6] = { ZONE_WHITE_TREEHOLLOW_2, ZONE_WHITE_TREEHOLLOW_3, ZONE_WHITE_TREEHOLLOW_4, ZONE_WHITE_TREEHOLLOW_5, ZONE_WHITE_TREEHOLLOW_6, ZONE_WHITE_TREEHOLLOW };

static const InfinityDungeonLevelRange sLevelRanges[10] = {
    { 47, 50 },
    { 50, 53 },
    { 53, 56 },
    { 56, 59 },
    { 59, 62 },
    { 62, 65 },
    { 65, 68 },
    { 68, 71 },
    { 71, 74 },
    { 74, 77 },
};

static const InfinityDungeonPos sPos2c20 = { 6, 0, 0 };

static const InfinityDungeonPos sPos2c38[2] = {
    { 7, 0, 2 },
    { 11, 0, 2 },
};

static const InfinityDungeonPos sPos2c2c = { 6, 0, 0 };

static const InfinityDungeonSize sSizes[10] = {
    { 3, 3 },
    { 3, 3 },
    { 3, 3 },
    { 4, 4 },
    { 4, 4 },
    { 3, 3 },
    { 3, 3 },
    { 4, 4 },
    { 4, 4 },
    { 4, 4 },
};

static const InfinityDungeonObjScript sObjScripts[30] = {
    { 0x18, 2, 4 },
    { 0x1a, 4, 5 },
    { 0x42, 3, 5 },
    { 0x17, 4, 3 },
    { 0x2e, 0, 2 },
    { 0x2f, 3, 3 },
    { 0x3d, 1, 2 },
    { 0x48, 0, 4 },
    { 0x40, 2, 4 },
    { 0x45, 2, 6 },
    { 0x43, 1, 2 },
    { 0x9b, 0, 2 },
    { 0xb7, 0, 2 },
    { 0xb8, 4, 1 },
    { 0x49, 1, 2 },
    { 0x4a, 3, 3 },
    { 0xc, 2, 0 },
    { 0x10, 3, 1 },
    { 0x2a, 1, 2 },
    { 0x2b, 4, 3 },
    { 0xb, 2, 0 },
    { 0xf, 3, 1 },
    { 0x30, 0, 2 },
    { 0x31, 4, 3 },
    { 0xd, 5, 0 },
    { 0x11, 6, 1 },
    { 0x2d, 6, 7 },
    { 0x2c, 5, 6 },
    { 0x22, 5, 2 },
    { 0x23, 6, 3 },
};

static const u8 sTrainerClassSharesWhite2[10][30] = {
    { 0, 2, 1, 0, 2, 0, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 2, 1, 1, 1, 2, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 0, 2, 3, 1, 0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 1, 2, 2, 2, 2, 0, 2, 2, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 2, 2, 2, 2, 0, 1, 2, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 2, 2, 2, 3, 2, 1, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 2, 3, 3, 2, 2, 2, 2, 2, 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 3, 3, 2, 2, 3, 1, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
};

static const u8 sTrainerClassSharesBlack2[10][30] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 2, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 2, 1, 1, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 2, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 0, 1, 2, 1, 0, 1, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 1, 2, 2, 0, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 1, 2, 2, 2, 1, 3, 2, 3, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 2, 2, 3, 1, 3, 2, 2, 2, 2, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 3, 2, 2, 1, 2, 2, 3, 2, 3, 2, 0, 0, 1, 1, 0, 0 },
};

static const InfinityDungeonPos sPosBlack2B[6][5] = {
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 14 },
        { 6, 0, 14 },
        { 6, 0, 14 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
    },
};

static const InfinityDungeonPos sPosWhite2B[6][5] = {
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 14 },
        { 6, 0, 14 },
        { 6, 0, 14 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 2 },
        { 6, 0, 13 },
        { 1, 0, 7 },
        { 11, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
    },
};

static const InfinityDungeonPos sPosBlack2A[6][5] = {
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 13 },
        { 6, 0, 13 },
        { 6, 0, 13 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
    },
};

static const InfinityDungeonPos sPosWhite2A[6][5] = {
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 13 },
        { 6, 0, 13 },
        { 6, 0, 13 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 6, 0, 14 },
        { 6, 0, 1 },
        { 12, 0, 7 },
        { 0, 0, 7 },
        { 0, 0, 0 },
    },
    {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
    },
};

static const InfinityDungeonNpc sNpcs[21] = {
    { 2, 1, 1, 1, 0, 0, 0, 0x130 },
    { 3, 1, 1, 2, 2, 2, 2, 0x138 },
    { 3, 4, 1, 2, 2, 2, 2, 0x148 },
    { 3, 5, 1, 2, 2, 2, 2, 0x140 },
    { 0x2c, 1, 3, 2, 0, 4, 4, 0x138 },
    { 0x2b, 1, 2, 2, 0, 4, 4, 0x138 },
    { 0x2a, 1, 1, 2, 0, 4, 4, 0x138 },
    { 0x2b, 1, 0, 2, 0, 4, 4, 0x138 },
    { 0x2c, 4, 3, 2, 2, 4, 4, 0x148 },
    { 0x2b, 4, 2, 2, 2, 4, 4, 0x148 },
    { 0x2a, 4, 1, 2, 2, 4, 4, 0x148 },
    { 0x2b, 4, 0, 2, 2, 4, 4, 0x148 },
    { 0x2c, 5, 3, 2, 2, 4, 4, 0x140 },
    { 0x2b, 6, 2, 2, 2, 4, 4, 0x140 },
    { 0x2a, 5, 1, 2, 2, 4, 4, 0x140 },
    { 0x2b, 6, 0, 2, 2, 4, 4, 0x140 },
    { 0x2b, 1, 2, 2, 0, 2, 2, 0x138 },
    { 0x2b, 4, 2, 2, 2, 2, 2, 0x148 },
    { 0x2b, 5, 2, 2, 2, 2, 2, 0x140 },
    { 0x2b, 6, 2, 2, 2, 2, 2, 0x140 },
    { 0x2c, 4, 3, 2, 2, 6, 6, 0x148 },
};

static const u8 sRoomCountsWhite2[10][5][20] = {
    {
        { 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 1, 2, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 3, 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 3, 0, 0, 3, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 2, 0, 0, 0, 3, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 },
        { 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
        { 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 1, 0 },
        { 2, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 0, 4, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 1, 2 },
        { 0, 3, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 1, 2, 0, 0, 2, 1, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 0, 3, 0, 0, 3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 2, 2 },
        { 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 0, 0, 1, 2, 2 },
        { 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 2, 0, 0, 1, 2, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 2, 0, 0, 0, 2, 1, 2 },
        { 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2, 0, 0, 2, 1, 2 },
        { 0, 0, 2, 0, 0, 0, 3, 0, 0, 3, 0, 0, 0, 0, 2, 0, 0, 2, 1, 2 },
        { 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 3, 0, 2, 0, 0, 0, 0, 2, 1, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
};

static const u8 sRoomCountsBlack2[10][5][20] = {
    {
        { 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 1, 2, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 3, 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 3, 0, 0, 3, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 2, 0, 0, 0, 3, 3, 0, 3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0 },
        { 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 1, 1, 0, 0, 0 },
        { 2, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 0, 4, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 2, 1, 0, 0, 2 },
        { 0, 3, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 0, 3, 0, 0, 3, 0, 0, 0, 0, 0, 2, 0, 1, 2, 0, 0, 2 },
        { 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 1, 2, 0, 0, 2 },
        { 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 2, 1, 2, 0, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 2, 0, 2, 1, 0, 0, 2 },
        { 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2, 2, 1, 0, 0, 2 },
        { 0, 0, 2, 0, 0, 0, 3, 0, 0, 3, 0, 0, 0, 0, 2, 2, 1, 0, 0, 2 },
        { 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 3, 0, 2, 0, 0, 2, 1, 0, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
};

static const InfinityDungeonRoomTemplate sRoomTemplates[19] = {
    { 0, 1, 4, { { 0, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 2, 4, { { 0, 8, 5 }, { 0, 4, 9 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 3, 4, { { 0, 6, 5 }, { 0, 4, 8 }, { 0, 8, 8 }, { 0, 0, 0 } } },
    { 0, 1, 4, { { 1, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 1, 4, { { 2, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 1, 4, { { 3, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 1, 4, { { 4, 4, 5 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 2, 4, { { 4, 4, 5 }, { 5, 8, 9 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 1, 4, { { 8, 4, 5 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 2, 4, { { 8, 4, 5 }, { 9, 8, 9 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 1, 4, { { 0xc, 4, 5 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 2, 4, { { 0xc, 4, 5 }, { 0xd, 8, 9 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 0, 3, 4, { { 0x10, 7, 6 }, { 0x10, 9, 0xa }, { 0x10, 5, 0xa }, { 0, 0, 0 } } },
    { 0, 3, 4, { { 0x11, 7, 6 }, { 0x11, 9, 0xa }, { 0x11, 5, 0xa }, { 0, 0, 0 } } },
    { 0, 3, 4, { { 0x12, 7, 6 }, { 0x13, 9, 0xa }, { 0x13, 5, 0xa }, { 0, 0, 0 } } },
    { 1, 1, 2, { { 0x14, 3, 4 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 1, 1, 3, { { 0, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 2, 1, 2, { { 0, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
    { 2, 1, 3, { { 0, 6, 7 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
};

static const InfinityDungeonTrainer sTrainers[640] = {
    { 0, 0, VERSION_WHITE2, 0 },
    { 0, 0, VERSION_WHITE2, 0 },
    { 0, 0, VERSION_WHITE2, 0 },
    { 0, 0, VERSION_WHITE2, 1 },
    { 0, 0, VERSION_WHITE2, 1 },
    { 0, 0, VERSION_WHITE2, 1 },
    { 0, 0, VERSION_WHITE2, 2 },
    { 0, 0, VERSION_WHITE2, 2 },
    { 0, 0, VERSION_WHITE2, 3 },
    { 0, 0, VERSION_WHITE2, 3 },
    { 0, 0, VERSION_WHITE2, 11 },
    { 0, 0, VERSION_WHITE2, 11 },
    { 0, 0, VERSION_WHITE2, 11 },
    { 0, 0, VERSION_WHITE2, 4 },
    { 0, 0, VERSION_WHITE2, 4 },
    { 0, 0, VERSION_WHITE2, 4 },
    { 0, 0, VERSION_WHITE2, 5 },
    { 0, 0, VERSION_WHITE2, 5 },
    { 0, 0, VERSION_WHITE2, 6 },
    { 0, 0, VERSION_WHITE2, 6 },
    { 0, 0, VERSION_WHITE2, 7 },
    { 0, 0, VERSION_WHITE2, 7 },
    { 0, 0, VERSION_WHITE2, 7 },
    { 0, 0, VERSION_WHITE2, 10 },
    { 0, 0, VERSION_WHITE2, 10 },
    { 0, 0, VERSION_WHITE2, 10 },
    { 0, 0, VERSION_WHITE2, 8 },
    { 0, 0, VERSION_WHITE2, 8 },
    { 0, 0, VERSION_WHITE2, 9 },
    { 0, 0, VERSION_WHITE2, 9 },
    { 0, 0, VERSION_BLACK2, 12 },
    { 0, 0, VERSION_BLACK2, 12 },
    { 0, 0, VERSION_BLACK2, 12 },
    { 0, 0, VERSION_BLACK2, 13 },
    { 0, 0, VERSION_BLACK2, 13 },
    { 0, 0, VERSION_BLACK2, 13 },
    { 0, 0, VERSION_BLACK2, 14 },
    { 0, 0, VERSION_BLACK2, 14 },
    { 0, 0, VERSION_BLACK2, 15 },
    { 0, 0, VERSION_BLACK2, 15 },
    { 0, 0, VERSION_BLACK2, 16 },
    { 0, 0, VERSION_BLACK2, 16 },
    { 0, 0, VERSION_BLACK2, 16 },
    { 0, 0, VERSION_BLACK2, 17 },
    { 0, 0, VERSION_BLACK2, 17 },
    { 0, 0, VERSION_BLACK2, 17 },
    { 0, 0, VERSION_BLACK2, 18 },
    { 0, 0, VERSION_BLACK2, 18 },
    { 0, 0, VERSION_BLACK2, 19 },
    { 0, 0, VERSION_BLACK2, 19 },
    { 0, 0, VERSION_BLACK2, 20 },
    { 0, 0, VERSION_BLACK2, 20 },
    { 0, 0, VERSION_BLACK2, 20 },
    { 0, 0, VERSION_BLACK2, 21 },
    { 0, 0, VERSION_BLACK2, 21 },
    { 0, 0, VERSION_BLACK2, 21 },
    { 0, 0, VERSION_BLACK2, 22 },
    { 0, 0, VERSION_BLACK2, 22 },
    { 0, 0, VERSION_BLACK2, 23 },
    { 0, 0, VERSION_BLACK2, 23 },
    { 1, 1, VERSION_WHITE2, 0 },
    { 1, 1, VERSION_WHITE2, 0 },
    { 1, 1, VERSION_WHITE2, 0 },
    { 1, 1, VERSION_WHITE2, 1 },
    { 1, 1, VERSION_WHITE2, 1 },
    { 1, 1, VERSION_WHITE2, 1 },
    { 1, 1, VERSION_WHITE2, 2 },
    { 1, 1, VERSION_WHITE2, 2 },
    { 1, 1, VERSION_WHITE2, 3 },
    { 1, 1, VERSION_WHITE2, 3 },
    { 1, 1, VERSION_WHITE2, 11 },
    { 1, 1, VERSION_WHITE2, 11 },
    { 1, 1, VERSION_WHITE2, 11 },
    { 1, 1, VERSION_WHITE2, 4 },
    { 1, 1, VERSION_WHITE2, 4 },
    { 1, 1, VERSION_WHITE2, 4 },
    { 1, 1, VERSION_WHITE2, 5 },
    { 1, 1, VERSION_WHITE2, 5 },
    { 1, 1, VERSION_WHITE2, 6 },
    { 1, 1, VERSION_WHITE2, 6 },
    { 1, 1, VERSION_WHITE2, 7 },
    { 1, 1, VERSION_WHITE2, 7 },
    { 1, 1, VERSION_WHITE2, 7 },
    { 1, 1, VERSION_WHITE2, 10 },
    { 1, 1, VERSION_WHITE2, 10 },
    { 1, 1, VERSION_WHITE2, 10 },
    { 1, 1, VERSION_WHITE2, 8 },
    { 1, 1, VERSION_WHITE2, 8 },
    { 1, 1, VERSION_WHITE2, 9 },
    { 1, 1, VERSION_WHITE2, 9 },
    { 1, 1, VERSION_BLACK2, 12 },
    { 1, 1, VERSION_BLACK2, 12 },
    { 1, 1, VERSION_BLACK2, 12 },
    { 1, 1, VERSION_BLACK2, 13 },
    { 1, 1, VERSION_BLACK2, 13 },
    { 1, 1, VERSION_BLACK2, 13 },
    { 1, 1, VERSION_BLACK2, 14 },
    { 1, 1, VERSION_BLACK2, 14 },
    { 1, 1, VERSION_BLACK2, 15 },
    { 1, 1, VERSION_BLACK2, 15 },
    { 1, 1, VERSION_BLACK2, 16 },
    { 1, 1, VERSION_BLACK2, 16 },
    { 1, 1, VERSION_BLACK2, 16 },
    { 1, 1, VERSION_BLACK2, 17 },
    { 1, 1, VERSION_BLACK2, 17 },
    { 1, 1, VERSION_BLACK2, 17 },
    { 1, 1, VERSION_BLACK2, 18 },
    { 1, 1, VERSION_BLACK2, 18 },
    { 1, 1, VERSION_BLACK2, 19 },
    { 1, 1, VERSION_BLACK2, 19 },
    { 1, 1, VERSION_BLACK2, 20 },
    { 1, 1, VERSION_BLACK2, 20 },
    { 1, 1, VERSION_BLACK2, 20 },
    { 1, 1, VERSION_BLACK2, 21 },
    { 1, 1, VERSION_BLACK2, 21 },
    { 1, 1, VERSION_BLACK2, 21 },
    { 1, 1, VERSION_BLACK2, 22 },
    { 1, 1, VERSION_BLACK2, 22 },
    { 1, 1, VERSION_BLACK2, 23 },
    { 1, 1, VERSION_BLACK2, 23 },
    { 2, 3, VERSION_WHITE2, 0 },
    { 2, 3, VERSION_WHITE2, 0 },
    { 2, 3, VERSION_WHITE2, 0 },
    { 2, 3, VERSION_WHITE2, 1 },
    { 2, 3, VERSION_WHITE2, 1 },
    { 2, 3, VERSION_WHITE2, 1 },
    { 2, 3, VERSION_WHITE2, 2 },
    { 2, 3, VERSION_WHITE2, 2 },
    { 2, 3, VERSION_WHITE2, 3 },
    { 2, 3, VERSION_WHITE2, 3 },
    { 2, 3, VERSION_WHITE2, 11 },
    { 2, 3, VERSION_WHITE2, 11 },
    { 2, 3, VERSION_WHITE2, 11 },
    { 2, 3, VERSION_WHITE2, 4 },
    { 2, 3, VERSION_WHITE2, 4 },
    { 2, 3, VERSION_WHITE2, 4 },
    { 2, 3, VERSION_WHITE2, 5 },
    { 2, 3, VERSION_WHITE2, 5 },
    { 2, 3, VERSION_WHITE2, 6 },
    { 2, 3, VERSION_WHITE2, 6 },
    { 2, 3, VERSION_WHITE2, 7 },
    { 2, 3, VERSION_WHITE2, 7 },
    { 2, 3, VERSION_WHITE2, 7 },
    { 2, 3, VERSION_WHITE2, 10 },
    { 2, 3, VERSION_WHITE2, 10 },
    { 2, 3, VERSION_WHITE2, 10 },
    { 2, 3, VERSION_WHITE2, 8 },
    { 2, 3, VERSION_WHITE2, 8 },
    { 2, 3, VERSION_WHITE2, 9 },
    { 2, 3, VERSION_WHITE2, 9 },
    { 2, 3, VERSION_BLACK2, 12 },
    { 2, 3, VERSION_BLACK2, 12 },
    { 2, 3, VERSION_BLACK2, 12 },
    { 2, 3, VERSION_BLACK2, 13 },
    { 2, 3, VERSION_BLACK2, 13 },
    { 2, 3, VERSION_BLACK2, 13 },
    { 2, 3, VERSION_BLACK2, 14 },
    { 2, 3, VERSION_BLACK2, 14 },
    { 2, 3, VERSION_BLACK2, 15 },
    { 2, 3, VERSION_BLACK2, 15 },
    { 2, 3, VERSION_BLACK2, 16 },
    { 2, 3, VERSION_BLACK2, 16 },
    { 2, 3, VERSION_BLACK2, 16 },
    { 2, 3, VERSION_BLACK2, 17 },
    { 2, 3, VERSION_BLACK2, 17 },
    { 2, 3, VERSION_BLACK2, 17 },
    { 2, 3, VERSION_BLACK2, 18 },
    { 2, 3, VERSION_BLACK2, 18 },
    { 2, 3, VERSION_BLACK2, 19 },
    { 2, 3, VERSION_BLACK2, 19 },
    { 2, 3, VERSION_BLACK2, 20 },
    { 2, 3, VERSION_BLACK2, 20 },
    { 2, 3, VERSION_BLACK2, 20 },
    { 2, 3, VERSION_BLACK2, 21 },
    { 2, 3, VERSION_BLACK2, 21 },
    { 2, 3, VERSION_BLACK2, 21 },
    { 2, 3, VERSION_BLACK2, 22 },
    { 2, 3, VERSION_BLACK2, 22 },
    { 2, 3, VERSION_BLACK2, 23 },
    { 2, 3, VERSION_BLACK2, 23 },
    { 2, 3, VERSION_WHITE2, 0 },
    { 2, 3, VERSION_WHITE2, 0 },
    { 2, 3, VERSION_WHITE2, 0 },
    { 2, 3, VERSION_WHITE2, 1 },
    { 2, 3, VERSION_WHITE2, 1 },
    { 2, 3, VERSION_WHITE2, 1 },
    { 2, 3, VERSION_WHITE2, 2 },
    { 2, 3, VERSION_WHITE2, 2 },
    { 2, 3, VERSION_WHITE2, 3 },
    { 2, 3, VERSION_WHITE2, 3 },
    { 2, 3, VERSION_WHITE2, 11 },
    { 2, 3, VERSION_WHITE2, 11 },
    { 2, 3, VERSION_WHITE2, 11 },
    { 2, 3, VERSION_WHITE2, 4 },
    { 2, 3, VERSION_WHITE2, 4 },
    { 2, 3, VERSION_WHITE2, 4 },
    { 2, 3, VERSION_WHITE2, 5 },
    { 2, 3, VERSION_WHITE2, 5 },
    { 2, 3, VERSION_WHITE2, 6 },
    { 2, 3, VERSION_WHITE2, 6 },
    { 2, 3, VERSION_WHITE2, 7 },
    { 2, 3, VERSION_WHITE2, 7 },
    { 2, 3, VERSION_WHITE2, 7 },
    { 2, 3, VERSION_WHITE2, 10 },
    { 2, 3, VERSION_WHITE2, 10 },
    { 2, 3, VERSION_WHITE2, 10 },
    { 2, 3, VERSION_WHITE2, 8 },
    { 2, 3, VERSION_WHITE2, 8 },
    { 2, 3, VERSION_WHITE2, 9 },
    { 2, 3, VERSION_WHITE2, 9 },
    { 2, 3, VERSION_BLACK2, 12 },
    { 2, 3, VERSION_BLACK2, 12 },
    { 2, 3, VERSION_BLACK2, 12 },
    { 2, 3, VERSION_BLACK2, 13 },
    { 2, 3, VERSION_BLACK2, 13 },
    { 2, 3, VERSION_BLACK2, 13 },
    { 2, 3, VERSION_BLACK2, 14 },
    { 2, 3, VERSION_BLACK2, 14 },
    { 2, 3, VERSION_BLACK2, 15 },
    { 2, 3, VERSION_BLACK2, 15 },
    { 2, 3, VERSION_BLACK2, 16 },
    { 2, 3, VERSION_BLACK2, 16 },
    { 2, 3, VERSION_BLACK2, 16 },
    { 2, 3, VERSION_BLACK2, 17 },
    { 2, 3, VERSION_BLACK2, 17 },
    { 2, 3, VERSION_BLACK2, 17 },
    { 2, 3, VERSION_BLACK2, 18 },
    { 2, 3, VERSION_BLACK2, 18 },
    { 2, 3, VERSION_BLACK2, 19 },
    { 2, 3, VERSION_BLACK2, 19 },
    { 2, 3, VERSION_BLACK2, 20 },
    { 2, 3, VERSION_BLACK2, 20 },
    { 2, 3, VERSION_BLACK2, 20 },
    { 2, 3, VERSION_BLACK2, 21 },
    { 2, 3, VERSION_BLACK2, 21 },
    { 2, 3, VERSION_BLACK2, 21 },
    { 2, 3, VERSION_BLACK2, 22 },
    { 2, 3, VERSION_BLACK2, 22 },
    { 2, 3, VERSION_BLACK2, 23 },
    { 2, 3, VERSION_BLACK2, 23 },
    { 4, 5, VERSION_WHITE2, 0 },
    { 4, 5, VERSION_WHITE2, 0 },
    { 4, 5, VERSION_WHITE2, 0 },
    { 4, 5, VERSION_WHITE2, 1 },
    { 4, 5, VERSION_WHITE2, 1 },
    { 4, 5, VERSION_WHITE2, 1 },
    { 4, 5, VERSION_WHITE2, 2 },
    { 4, 5, VERSION_WHITE2, 2 },
    { 4, 5, VERSION_WHITE2, 3 },
    { 4, 5, VERSION_WHITE2, 3 },
    { 4, 5, VERSION_WHITE2, 11 },
    { 4, 5, VERSION_WHITE2, 11 },
    { 4, 5, VERSION_WHITE2, 11 },
    { 4, 5, VERSION_WHITE2, 4 },
    { 4, 5, VERSION_WHITE2, 4 },
    { 4, 5, VERSION_WHITE2, 4 },
    { 4, 5, VERSION_WHITE2, 5 },
    { 4, 5, VERSION_WHITE2, 5 },
    { 4, 5, VERSION_WHITE2, 6 },
    { 4, 5, VERSION_WHITE2, 6 },
    { 4, 5, VERSION_WHITE2, 7 },
    { 4, 5, VERSION_WHITE2, 7 },
    { 4, 5, VERSION_WHITE2, 7 },
    { 4, 5, VERSION_WHITE2, 10 },
    { 4, 5, VERSION_WHITE2, 10 },
    { 4, 5, VERSION_WHITE2, 10 },
    { 4, 5, VERSION_WHITE2, 8 },
    { 4, 5, VERSION_WHITE2, 8 },
    { 4, 5, VERSION_WHITE2, 9 },
    { 4, 5, VERSION_WHITE2, 9 },
    { 4, 5, VERSION_BLACK2, 12 },
    { 4, 5, VERSION_BLACK2, 12 },
    { 4, 5, VERSION_BLACK2, 12 },
    { 4, 5, VERSION_BLACK2, 13 },
    { 4, 5, VERSION_BLACK2, 13 },
    { 4, 5, VERSION_BLACK2, 13 },
    { 4, 5, VERSION_BLACK2, 14 },
    { 4, 5, VERSION_BLACK2, 14 },
    { 4, 5, VERSION_BLACK2, 15 },
    { 4, 5, VERSION_BLACK2, 15 },
    { 4, 5, VERSION_BLACK2, 16 },
    { 4, 5, VERSION_BLACK2, 16 },
    { 4, 5, VERSION_BLACK2, 16 },
    { 4, 5, VERSION_BLACK2, 17 },
    { 4, 5, VERSION_BLACK2, 17 },
    { 4, 5, VERSION_BLACK2, 17 },
    { 4, 5, VERSION_BLACK2, 18 },
    { 4, 5, VERSION_BLACK2, 18 },
    { 4, 5, VERSION_BLACK2, 19 },
    { 4, 5, VERSION_BLACK2, 19 },
    { 4, 5, VERSION_BLACK2, 20 },
    { 4, 5, VERSION_BLACK2, 20 },
    { 4, 5, VERSION_BLACK2, 20 },
    { 4, 5, VERSION_BLACK2, 21 },
    { 4, 5, VERSION_BLACK2, 21 },
    { 4, 5, VERSION_BLACK2, 21 },
    { 4, 5, VERSION_BLACK2, 22 },
    { 4, 5, VERSION_BLACK2, 22 },
    { 4, 5, VERSION_BLACK2, 23 },
    { 4, 5, VERSION_BLACK2, 23 },
    { 4, 5, VERSION_WHITE2, 0 },
    { 4, 5, VERSION_WHITE2, 0 },
    { 4, 5, VERSION_WHITE2, 0 },
    { 4, 5, VERSION_WHITE2, 1 },
    { 4, 5, VERSION_WHITE2, 1 },
    { 4, 5, VERSION_WHITE2, 1 },
    { 4, 5, VERSION_WHITE2, 2 },
    { 4, 5, VERSION_WHITE2, 2 },
    { 4, 5, VERSION_WHITE2, 3 },
    { 4, 5, VERSION_WHITE2, 3 },
    { 4, 5, VERSION_WHITE2, 11 },
    { 4, 5, VERSION_WHITE2, 11 },
    { 4, 5, VERSION_WHITE2, 11 },
    { 4, 5, VERSION_WHITE2, 4 },
    { 4, 5, VERSION_WHITE2, 4 },
    { 4, 5, VERSION_WHITE2, 4 },
    { 4, 5, VERSION_WHITE2, 5 },
    { 4, 5, VERSION_WHITE2, 5 },
    { 4, 5, VERSION_WHITE2, 6 },
    { 4, 5, VERSION_WHITE2, 6 },
    { 4, 5, VERSION_WHITE2, 7 },
    { 4, 5, VERSION_WHITE2, 7 },
    { 4, 5, VERSION_WHITE2, 7 },
    { 4, 5, VERSION_WHITE2, 10 },
    { 4, 5, VERSION_WHITE2, 10 },
    { 4, 5, VERSION_WHITE2, 10 },
    { 4, 5, VERSION_WHITE2, 8 },
    { 4, 5, VERSION_WHITE2, 8 },
    { 4, 5, VERSION_WHITE2, 9 },
    { 4, 5, VERSION_WHITE2, 9 },
    { 4, 5, VERSION_BLACK2, 12 },
    { 4, 5, VERSION_BLACK2, 12 },
    { 4, 5, VERSION_BLACK2, 12 },
    { 4, 5, VERSION_BLACK2, 13 },
    { 4, 5, VERSION_BLACK2, 13 },
    { 4, 5, VERSION_BLACK2, 13 },
    { 4, 5, VERSION_BLACK2, 14 },
    { 4, 5, VERSION_BLACK2, 14 },
    { 4, 5, VERSION_BLACK2, 15 },
    { 4, 5, VERSION_BLACK2, 15 },
    { 4, 5, VERSION_BLACK2, 16 },
    { 4, 5, VERSION_BLACK2, 16 },
    { 4, 5, VERSION_BLACK2, 16 },
    { 4, 5, VERSION_BLACK2, 17 },
    { 4, 5, VERSION_BLACK2, 17 },
    { 4, 5, VERSION_BLACK2, 17 },
    { 4, 5, VERSION_BLACK2, 18 },
    { 4, 5, VERSION_BLACK2, 18 },
    { 4, 5, VERSION_BLACK2, 19 },
    { 4, 5, VERSION_BLACK2, 19 },
    { 4, 5, VERSION_BLACK2, 20 },
    { 4, 5, VERSION_BLACK2, 20 },
    { 4, 5, VERSION_BLACK2, 20 },
    { 4, 5, VERSION_BLACK2, 21 },
    { 4, 5, VERSION_BLACK2, 21 },
    { 4, 5, VERSION_BLACK2, 21 },
    { 4, 5, VERSION_BLACK2, 22 },
    { 4, 5, VERSION_BLACK2, 22 },
    { 4, 5, VERSION_BLACK2, 23 },
    { 4, 5, VERSION_BLACK2, 23 },
    { 6, 7, VERSION_WHITE2, 0 },
    { 6, 7, VERSION_WHITE2, 0 },
    { 6, 7, VERSION_WHITE2, 0 },
    { 6, 7, VERSION_WHITE2, 1 },
    { 6, 7, VERSION_WHITE2, 1 },
    { 6, 7, VERSION_WHITE2, 23 },
    { 6, 7, VERSION_WHITE2, 2 },
    { 6, 7, VERSION_WHITE2, 2 },
    { 6, 7, VERSION_WHITE2, 3 },
    { 6, 7, VERSION_WHITE2, 3 },
    { 6, 7, VERSION_WHITE2, 11 },
    { 6, 7, VERSION_WHITE2, 11 },
    { 6, 7, VERSION_WHITE2, 11 },
    { 6, 7, VERSION_WHITE2, 4 },
    { 6, 7, VERSION_WHITE2, 4 },
    { 6, 7, VERSION_WHITE2, 4 },
    { 6, 7, VERSION_WHITE2, 5 },
    { 6, 7, VERSION_WHITE2, 5 },
    { 6, 7, VERSION_WHITE2, 6 },
    { 6, 7, VERSION_WHITE2, 6 },
    { 6, 7, VERSION_WHITE2, 7 },
    { 6, 7, VERSION_WHITE2, 7 },
    { 6, 7, VERSION_WHITE2, 7 },
    { 6, 7, VERSION_WHITE2, 10 },
    { 6, 7, VERSION_WHITE2, 10 },
    { 6, 7, VERSION_WHITE2, 10 },
    { 6, 7, VERSION_WHITE2, 8 },
    { 6, 7, VERSION_WHITE2, 8 },
    { 6, 7, VERSION_WHITE2, 9 },
    { 6, 7, VERSION_WHITE2, 9 },
    { 6, 7, VERSION_BLACK2, 12 },
    { 6, 7, VERSION_BLACK2, 12 },
    { 6, 7, VERSION_BLACK2, 12 },
    { 6, 7, VERSION_BLACK2, 13 },
    { 6, 7, VERSION_BLACK2, 13 },
    { 6, 7, VERSION_BLACK2, 13 },
    { 6, 7, VERSION_BLACK2, 14 },
    { 6, 7, VERSION_BLACK2, 14 },
    { 6, 7, VERSION_BLACK2, 15 },
    { 6, 7, VERSION_BLACK2, 15 },
    { 6, 7, VERSION_BLACK2, 16 },
    { 6, 7, VERSION_BLACK2, 16 },
    { 6, 7, VERSION_BLACK2, 16 },
    { 6, 7, VERSION_BLACK2, 17 },
    { 6, 7, VERSION_BLACK2, 17 },
    { 6, 7, VERSION_BLACK2, 17 },
    { 6, 7, VERSION_BLACK2, 18 },
    { 6, 7, VERSION_BLACK2, 18 },
    { 6, 7, VERSION_BLACK2, 19 },
    { 6, 7, VERSION_BLACK2, 19 },
    { 6, 7, VERSION_BLACK2, 20 },
    { 6, 7, VERSION_BLACK2, 20 },
    { 6, 7, VERSION_BLACK2, 20 },
    { 6, 7, VERSION_BLACK2, 21 },
    { 6, 7, VERSION_BLACK2, 21 },
    { 6, 7, VERSION_BLACK2, 21 },
    { 6, 7, VERSION_BLACK2, 22 },
    { 6, 7, VERSION_BLACK2, 22 },
    { 6, 7, VERSION_BLACK2, 23 },
    { 6, 7, VERSION_BLACK2, 23 },
    { 6, 7, VERSION_WHITE2, 0 },
    { 6, 7, VERSION_WHITE2, 0 },
    { 6, 7, VERSION_WHITE2, 0 },
    { 6, 7, VERSION_WHITE2, 1 },
    { 6, 7, VERSION_WHITE2, 1 },
    { 6, 7, VERSION_WHITE2, 1 },
    { 6, 7, VERSION_WHITE2, 2 },
    { 6, 7, VERSION_WHITE2, 2 },
    { 6, 7, VERSION_WHITE2, 3 },
    { 6, 7, VERSION_WHITE2, 3 },
    { 6, 7, VERSION_WHITE2, 11 },
    { 6, 7, VERSION_WHITE2, 11 },
    { 6, 7, VERSION_WHITE2, 11 },
    { 6, 7, VERSION_WHITE2, 4 },
    { 6, 7, VERSION_WHITE2, 4 },
    { 6, 7, VERSION_WHITE2, 4 },
    { 6, 7, VERSION_WHITE2, 5 },
    { 6, 7, VERSION_WHITE2, 5 },
    { 6, 7, VERSION_WHITE2, 6 },
    { 6, 7, VERSION_WHITE2, 6 },
    { 6, 7, VERSION_WHITE2, 7 },
    { 6, 7, VERSION_WHITE2, 7 },
    { 6, 7, VERSION_WHITE2, 7 },
    { 6, 7, VERSION_WHITE2, 10 },
    { 6, 7, VERSION_WHITE2, 10 },
    { 6, 7, VERSION_WHITE2, 10 },
    { 6, 7, VERSION_WHITE2, 8 },
    { 6, 7, VERSION_WHITE2, 8 },
    { 6, 7, VERSION_WHITE2, 9 },
    { 6, 7, VERSION_WHITE2, 9 },
    { 6, 7, VERSION_BLACK2, 12 },
    { 6, 7, VERSION_BLACK2, 12 },
    { 6, 7, VERSION_BLACK2, 12 },
    { 6, 7, VERSION_BLACK2, 13 },
    { 6, 7, VERSION_BLACK2, 13 },
    { 6, 7, VERSION_BLACK2, 13 },
    { 6, 7, VERSION_BLACK2, 14 },
    { 6, 7, VERSION_BLACK2, 14 },
    { 6, 7, VERSION_BLACK2, 15 },
    { 6, 7, VERSION_BLACK2, 15 },
    { 6, 7, VERSION_BLACK2, 16 },
    { 6, 7, VERSION_BLACK2, 16 },
    { 6, 7, VERSION_BLACK2, 16 },
    { 6, 7, VERSION_BLACK2, 17 },
    { 6, 7, VERSION_BLACK2, 17 },
    { 6, 7, VERSION_BLACK2, 17 },
    { 6, 7, VERSION_BLACK2, 18 },
    { 6, 7, VERSION_BLACK2, 18 },
    { 6, 7, VERSION_BLACK2, 19 },
    { 6, 7, VERSION_BLACK2, 19 },
    { 6, 7, VERSION_BLACK2, 20 },
    { 6, 7, VERSION_BLACK2, 20 },
    { 6, 7, VERSION_BLACK2, 20 },
    { 6, 7, VERSION_BLACK2, 21 },
    { 6, 7, VERSION_BLACK2, 21 },
    { 6, 7, VERSION_BLACK2, 21 },
    { 6, 7, VERSION_BLACK2, 22 },
    { 6, 7, VERSION_BLACK2, 22 },
    { 6, 7, VERSION_BLACK2, 23 },
    { 6, 7, VERSION_BLACK2, 23 },
    { 8, 9, VERSION_WHITE2, 0 },
    { 8, 9, VERSION_WHITE2, 0 },
    { 8, 9, VERSION_WHITE2, 0 },
    { 8, 9, VERSION_WHITE2, 1 },
    { 8, 9, VERSION_WHITE2, 1 },
    { 8, 9, VERSION_WHITE2, 1 },
    { 8, 9, VERSION_WHITE2, 2 },
    { 8, 9, VERSION_WHITE2, 2 },
    { 8, 9, VERSION_WHITE2, 3 },
    { 8, 9, VERSION_WHITE2, 3 },
    { 8, 9, VERSION_WHITE2, 11 },
    { 8, 9, VERSION_WHITE2, 11 },
    { 8, 9, VERSION_WHITE2, 11 },
    { 8, 9, VERSION_WHITE2, 4 },
    { 8, 9, VERSION_WHITE2, 4 },
    { 8, 9, VERSION_WHITE2, 4 },
    { 8, 9, VERSION_WHITE2, 5 },
    { 8, 9, VERSION_WHITE2, 5 },
    { 8, 9, VERSION_WHITE2, 6 },
    { 8, 9, VERSION_WHITE2, 6 },
    { 8, 9, VERSION_WHITE2, 7 },
    { 8, 9, VERSION_WHITE2, 7 },
    { 8, 9, VERSION_WHITE2, 7 },
    { 8, 9, VERSION_WHITE2, 10 },
    { 8, 9, VERSION_WHITE2, 10 },
    { 8, 9, VERSION_WHITE2, 10 },
    { 8, 9, VERSION_WHITE2, 8 },
    { 8, 9, VERSION_WHITE2, 8 },
    { 8, 9, VERSION_WHITE2, 9 },
    { 8, 9, VERSION_WHITE2, 9 },
    { 8, 9, VERSION_BLACK2, 12 },
    { 8, 9, VERSION_BLACK2, 12 },
    { 8, 9, VERSION_BLACK2, 12 },
    { 8, 9, VERSION_BLACK2, 13 },
    { 8, 9, VERSION_BLACK2, 13 },
    { 8, 9, VERSION_BLACK2, 13 },
    { 8, 9, VERSION_BLACK2, 14 },
    { 8, 9, VERSION_BLACK2, 14 },
    { 8, 9, VERSION_BLACK2, 15 },
    { 8, 9, VERSION_BLACK2, 15 },
    { 8, 9, VERSION_BLACK2, 16 },
    { 8, 9, VERSION_BLACK2, 16 },
    { 8, 9, VERSION_BLACK2, 16 },
    { 8, 9, VERSION_BLACK2, 17 },
    { 8, 9, VERSION_BLACK2, 17 },
    { 8, 9, VERSION_BLACK2, 17 },
    { 8, 9, VERSION_BLACK2, 18 },
    { 8, 9, VERSION_BLACK2, 18 },
    { 8, 9, VERSION_BLACK2, 19 },
    { 8, 9, VERSION_BLACK2, 19 },
    { 8, 9, VERSION_BLACK2, 20 },
    { 8, 9, VERSION_BLACK2, 20 },
    { 8, 9, VERSION_BLACK2, 20 },
    { 8, 9, VERSION_BLACK2, 21 },
    { 8, 9, VERSION_BLACK2, 21 },
    { 8, 9, VERSION_BLACK2, 21 },
    { 8, 9, VERSION_BLACK2, 22 },
    { 8, 9, VERSION_BLACK2, 22 },
    { 8, 9, VERSION_BLACK2, 23 },
    { 8, 9, VERSION_BLACK2, 23 },
    { 8, 9, VERSION_WHITE2, 0 },
    { 8, 9, VERSION_WHITE2, 0 },
    { 8, 9, VERSION_WHITE2, 0 },
    { 8, 9, VERSION_WHITE2, 1 },
    { 8, 9, VERSION_WHITE2, 1 },
    { 8, 9, VERSION_WHITE2, 1 },
    { 8, 9, VERSION_WHITE2, 2 },
    { 8, 9, VERSION_WHITE2, 2 },
    { 8, 9, VERSION_WHITE2, 3 },
    { 8, 9, VERSION_WHITE2, 3 },
    { 8, 9, VERSION_WHITE2, 11 },
    { 8, 9, VERSION_WHITE2, 11 },
    { 8, 9, VERSION_WHITE2, 11 },
    { 8, 9, VERSION_WHITE2, 4 },
    { 8, 9, VERSION_WHITE2, 4 },
    { 8, 9, VERSION_WHITE2, 4 },
    { 8, 9, VERSION_WHITE2, 5 },
    { 8, 9, VERSION_WHITE2, 5 },
    { 8, 9, VERSION_WHITE2, 6 },
    { 8, 9, VERSION_WHITE2, 6 },
    { 8, 9, VERSION_WHITE2, 7 },
    { 8, 9, VERSION_WHITE2, 7 },
    { 8, 9, VERSION_WHITE2, 7 },
    { 8, 9, VERSION_WHITE2, 10 },
    { 8, 9, VERSION_WHITE2, 10 },
    { 8, 9, VERSION_WHITE2, 10 },
    { 8, 9, VERSION_WHITE2, 8 },
    { 8, 9, VERSION_WHITE2, 8 },
    { 8, 9, VERSION_WHITE2, 9 },
    { 8, 9, VERSION_WHITE2, 9 },
    { 8, 9, VERSION_BLACK2, 12 },
    { 8, 9, VERSION_BLACK2, 12 },
    { 8, 9, VERSION_BLACK2, 12 },
    { 8, 9, VERSION_BLACK2, 13 },
    { 8, 9, VERSION_BLACK2, 13 },
    { 8, 9, VERSION_BLACK2, 13 },
    { 8, 9, VERSION_BLACK2, 14 },
    { 8, 9, VERSION_BLACK2, 14 },
    { 8, 9, VERSION_BLACK2, 15 },
    { 8, 9, VERSION_BLACK2, 15 },
    { 8, 9, VERSION_BLACK2, 16 },
    { 8, 9, VERSION_BLACK2, 16 },
    { 8, 9, VERSION_BLACK2, 16 },
    { 8, 9, VERSION_BLACK2, 17 },
    { 8, 9, VERSION_BLACK2, 17 },
    { 8, 9, VERSION_BLACK2, 17 },
    { 8, 9, VERSION_BLACK2, 18 },
    { 8, 9, VERSION_BLACK2, 18 },
    { 8, 9, VERSION_BLACK2, 19 },
    { 8, 9, VERSION_BLACK2, 19 },
    { 8, 9, VERSION_BLACK2, 20 },
    { 8, 9, VERSION_BLACK2, 20 },
    { 8, 9, VERSION_BLACK2, 20 },
    { 8, 9, VERSION_BLACK2, 21 },
    { 8, 9, VERSION_BLACK2, 21 },
    { 8, 9, VERSION_BLACK2, 21 },
    { 8, 9, VERSION_BLACK2, 22 },
    { 8, 9, VERSION_BLACK2, 22 },
    { 8, 9, VERSION_BLACK2, 23 },
    { 8, 9, VERSION_BLACK2, 23 },
    { 0, 2, VERSION_WHITE2, 28 },
    { 0, 2, VERSION_WHITE2, 28 },
    { 0, 2, VERSION_WHITE2, 28 },
    { 3, 5, VERSION_WHITE2, 28 },
    { 3, 5, VERSION_WHITE2, 28 },
    { 3, 5, VERSION_WHITE2, 28 },
    { 6, 9, VERSION_WHITE2, 28 },
    { 6, 9, VERSION_WHITE2, 28 },
    { 6, 9, VERSION_WHITE2, 28 },
    { 6, 9, VERSION_WHITE2, 28 },
    { 0, 2, VERSION_WHITE2, 29 },
    { 0, 2, VERSION_WHITE2, 29 },
    { 0, 2, VERSION_WHITE2, 29 },
    { 3, 5, VERSION_WHITE2, 29 },
    { 3, 5, VERSION_WHITE2, 29 },
    { 3, 5, VERSION_WHITE2, 29 },
    { 6, 9, VERSION_WHITE2, 29 },
    { 6, 9, VERSION_WHITE2, 29 },
    { 6, 9, VERSION_WHITE2, 29 },
    { 6, 9, VERSION_WHITE2, 29 },
    { 0, 2, VERSION_BLACK2, 24 },
    { 0, 2, VERSION_BLACK2, 24 },
    { 0, 2, VERSION_BLACK2, 24 },
    { 3, 5, VERSION_BLACK2, 24 },
    { 3, 5, VERSION_BLACK2, 24 },
    { 0, 2, VERSION_BLACK2, 25 },
    { 0, 2, VERSION_BLACK2, 25 },
    { 0, 2, VERSION_BLACK2, 25 },
    { 3, 5, VERSION_BLACK2, 25 },
    { 3, 5, VERSION_BLACK2, 25 },
    { 3, 5, VERSION_BLACK2, 27 },
    { 6, 9, VERSION_BLACK2, 27 },
    { 6, 9, VERSION_BLACK2, 27 },
    { 6, 9, VERSION_BLACK2, 27 },
    { 6, 9, VERSION_BLACK2, 27 },
    { 3, 5, VERSION_BLACK2, 26 },
    { 6, 9, VERSION_BLACK2, 26 },
    { 6, 9, VERSION_BLACK2, 26 },
    { 6, 9, VERSION_BLACK2, 26 },
    { 6, 9, VERSION_BLACK2, 26 },
};

static inline u32 InfinityDungeonRandom_Next(InfinityDungeonRandom *rnd, u32 max) {
    rnd->state = rnd->multiplier * rnd->state + rnd->increment;
    if (max == 0) {
        return rnd->state >> 32;
    }
    return ((rnd->state >> 32) * max) >> 32;
}

const InfinityDungeonNpc *InfinityDungeon_GetNpc(u16 npc) {
    return &sNpcs[npc];
}

const InfinityDungeonRoomTemplate *InfinityDungeon_GetRoomTemplate(u16 type) {
    return &sRoomTemplates[type];
}

const InfinityDungeonTrainer *InfinityDungeon_GetTrainer(u16 trainer) {
    return &sTrainers[trainer];
}

u8 func_ov127_021f0a90(u16 trainer) {
    return sTrainers[trainer].trainerClass;
}

u8 InfinityDungeon_GetRoomNpcCount(u8 type) {
    if (type != INFINITY_DUNGEON_ROOM_TYPE_NONE) {
        return sRoomTemplates[type].npcCount;
    }
    return 0;
}

void InfinityDungeon_GetFloorSize(u32 rank, u32 *width, u32 *height) {
    *width = sSizes[rank].width;
    *height = sSizes[rank].height;
}

const u16 *func_ov127_021f0acc(u8 version) {
    if (version == VERSION_BLACK2) {
        return sBlack2Zones;
    }
    return sWhite2Zones;
}

const InfinityDungeonPos *func_ov127_021f0ae0(u8 version) {
    if (version == VERSION_BLACK2) {
        return &sPos2c2c;
    }
    return &sPos2c20;
}

const InfinityDungeonPos *func_ov127_021f0af4(u32 index) {
    return &sPos2c38[index];
}

const InfinityDungeonPos *func_ov127_021f0b04(u8 version, u8 kind, u16 direction) {
    if (version == VERSION_BLACK2) {
        return &sPosBlack2A[kind][direction];
    }
    return &sPosWhite2A[kind][direction];
}

const InfinityDungeonPos *func_ov127_021f0b30(u8 version, u8 kind, u8 index) {
    if (version == VERSION_BLACK2) {
        return &sPosBlack2B[kind][index];
    }
    return &sPosWhite2B[kind][index];
}

const InfinityDungeonLevelRange *func_ov127_021f0b5c(u16 mode) {
    return &sLevelRanges[mode];
}

u8 func_ov127_021f0b68(u16 mode) {
    return sIvs[mode];
}

const u8 *func_ov127_021f0b74(u16 rank, u8 version) {
    if (version == VERSION_BLACK2) {
        return sTrainerClassSharesBlack2[rank];
    }
    return sTrainerClassSharesWhite2[rank];
}

u32 InfinityDungeon_GetNpcScriptIndex(InfinityDungeon *dungeon, u32 objCode, u8 floor, u8 slot) {
    int i;

    if (!func_ov127_021f0db4(dungeon, floor, slot)) {
        for (i = 0; i < 30; i++) {
            if (objCode == sObjScripts[i].objCode) {
                return sObjScripts[i].scriptIndex;
            }
        }
    } else {
        return 7;
    }
    return 0;
}

u8 func_ov127_021f0bd0(u32 objCode) {
    int i;

    for (i = 0; i < 30; i++) {
        if (objCode == sObjScripts[i].objCode) {
            return sObjScripts[i].npcIndex;
        }
    }
    return 0;
}

void func_ov127_021f0bfc(u32 *array, int count, InfinityDungeonRandom *rnd) {
    int i;

    for (i = 0; i < count; i++) {
        u32 j = InfinityDungeonRandom_Next(rnd, count);
        u32 temp = array[j];

        array[j] = array[i];
        array[i] = temp;
    }
}

static void func_ov127_021f0c54(u8 *array, int count, MATHRandContext32 *rand) {
    int i;

    for (i = 0; i < count; i++) {
        u32 j = MATH_Rand32(rand, count);
        u8 temp = array[j];

        array[j] = array[i];
        array[i] = temp;
    }
}

void func_ov127_021f0ca8(u16 *array, int count, InfinityDungeonRandom *rnd) {
    int i;

    for (i = 0; i < count; i++) {
        u32 j = InfinityDungeonRandom_Next(rnd, count);
        u16 temp = array[j];

        array[j] = array[i];
        array[i] = temp;
    }
}

void InfinityDungeon_Seed(InfinityDungeon *dungeon, s32 seed) {
    MATHRandContext32 rand;
    int floor;
    int i;

    dungeon->seed = seed;
    MATH_InitRand32(&rand, seed);
    dungeon->trainerSeed = MATH_Rand32(&rand, 0);
    dungeon->layoutSeed = MATH_Rand32(&rand, 0);
    dungeon->rewardSeed = MATH_Rand32(&rand, 0);
    for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
        for (i = 0; i < INFINITY_DUNGEON_FLOOR_TRAINER_MAX; i++) {
            dungeon->battleSeeds[floor][i] = MATH_Rand32(&rand, 0);
        }
    }
}

static const u8 *func_ov127_021f0d88(u16 rank, u8 floor, u8 version) {
    if (version == VERSION_BLACK2) {
        return sRoomCountsBlack2[rank][floor];
    }
    return sRoomCountsWhite2[rank][floor];
}

u16 func_ov127_021f0db4(InfinityDungeon *dungeon, u8 floor, u8 slot) {
    if (dungeon->targetSlot == slot && dungeon->targetFloor == floor) {
        return 1;
    }
    return 0;
}

u8 func_ov127_021f0dd8(InfinityDungeon *dungeon) {
    return dungeon->version;
}

u32 func_ov127_021f0de4(InfinityDungeon *dungeon, u32 floor, int slot) {
    int i;
    int total = 0;

    for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
        u8 count = InfinityDungeon_GetRoomNpcCount(dungeon->floors[floor].rooms[i].type);

        if (count != 0) {
            total += count;
            if (slot < total) {
                return i;
            }
        }
    }
    return 0;
}

u32 InfinityDungeon_GetTrainerSlot(InfinityDungeon *dungeon, u32 floor, u32 room, u8 trainer) {
    int total = 0;
    int i;

    for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
        u32 type = dungeon->floors[floor].rooms[i].type;

        if (room <= i) {
            break;
        }
        if (InfinityDungeon_GetRoomNpcCount(type) != 0) {
            total += InfinityDungeon_GetRoomNpcCount(type);
        }
    }
    return total + trainer;
}

u16 InfinityDungeon_GetTrainerId(InfinityDungeon *dungeon, u8 floor, u8 slot) {
    return dungeon->trainerIds[floor][slot];
}

void InfinityDungeon_LayoutFloors(InfinityDungeon *dungeon, u32 rank, u32 seed, int width, int height, u8 floorCount) {
    InfinityDungeonWalk walk;
    InfinityDungeonWalk extraWalk;
    InfinityDungeonGridPos starts[INFINITY_DUNGEON_FLOOR_COUNT];
    MATHRandContext32 rand;
    int rooms[INFINITY_DUNGEON_ROOM_COUNT];
    int directions[4];
    int extraDirections[4];
    u8 types[INFINITY_DUNGEON_ROOM_COUNT];
    u8 goalTypes[INFINITY_DUNGEON_ROOM_COUNT];
    InfinityDungeonGridPos goal;
    InfinityDungeonGridPos pos;
    int goalFloor;
    u8 goalRoom;
    int count;
    int floor;
    int kind;
    int i;
    int j;

    MATH_InitRand32(&rand, seed);
    for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
        for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
            dungeon->floors[floor].rooms[i].index = i;
            dungeon->floors[floor].rooms[i].kind = 4;
            dungeon->floors[floor].rooms[i].type = INFINITY_DUNGEON_ROOM_TYPE_NONE;
            for (j = 0; j < 4; j++) {
                dungeon->floors[floor].rooms[i].open[j] = 0;
            }
        }
    }
    for (floor = 0; floor < INFINITY_DUNGEON_FLOOR_COUNT; floor++) {
        InfinityDungeonRoom *first = dungeon->floors[floor].rooms;

        first->kind = 0;
        first->type = INFINITY_DUNGEON_ROOM_TYPE_NONE;
        starts[floor].x = 0;
        starts[floor].y = 0;
    }

    // The floor of the goal, and its room among those that have another below them
    goalFloor = MATH_Rand32(&rand, floorCount);
    {
        InfinityDungeonRoom *goalRooms = dungeon->floors[goalFloor].rooms;
        InfinityDungeonRoom *room;

        count = func_ov127_021f1388(goalRooms, rooms, width, height);
        goalRoom = rooms[MATH_Rand32(&rand, count)];
        room = &goalRooms[goalRoom];
        room->kind = 1;
        room->type = INFINITY_DUNGEON_ROOM_TYPE_NONE;
    }
    goal.x = goalRoom % INFINITY_DUNGEON_GRID_WIDTH;
    goal.y = goalRoom / INFINITY_DUNGEON_GRID_WIDTH;

    for (floor = 0; floor < floorCount; floor++) {
        InfinityDungeonRoom *floorRooms = dungeon->floors[floor].rooms;

        walk.cur = starts[floor];
        walk.top = -1;
        for (i = 0; i < INFINITY_DUNGEON_WALK_DEPTH; i++) {
            count = func_ov127_021f14c0(&walk.cur, floorRooms, directions, width, height, FALSE);
            if (count > 0) {
                func_ov127_021f1408(directions[(u8)MATH_Rand32(&rand, count)], &walk, floorRooms);
            } else {
                func_ov127_021f160c(&walk);
                if (func_ov127_021f1634(floorRooms, width, height) == TRUE) {
                    break;
                }
            }
        }
        if (goalFloor == floor) {
            walk.cur = goal;
            func_ov127_021f1408(1, &walk, floorRooms);
        }

        extraWalk.top = -1;
        for (pos.y = 0; pos.y < height; pos.y++) {
            for (pos.x = 0; pos.x < width; pos.x++) {
                extraWalk.cur = pos;
                kind = func_ov127_021f13e8(&pos, floorRooms)->kind;
                if (kind != 0 && kind != 1) {
                    count = func_ov127_021f14c0(&pos, floorRooms, extraDirections, width, height, TRUE);
                    func_ov127_021f1408(extraDirections[(u8)MATH_Rand32(&rand, count)], &extraWalk,
                                        floorRooms);
                }
            }
        }
    }

    for (i = 0; i < floorCount; i++) {
        int placed = 0;

        count = func_ov127_021f1308(rank, i, dungeon->version, types);
        func_ov127_021f0c54(types, count, &rand);
        for (j = 0; j < INFINITY_DUNGEON_ROOM_COUNT; j++) {
            if (dungeon->floors[i].rooms[j].kind == 4) {
                int x = j % INFINITY_DUNGEON_GRID_WIDTH;
                int y = j / INFINITY_DUNGEON_GRID_WIDTH;

                if (x < width && y < height) {
                    if (goal.x == x && goal.y + 1 == y && i == goalFloor) {
                        const InfinityDungeonRoomTemplate *roomTemplate;
                        u8 type;

                        count = func_ov127_021f1348(rank, i, dungeon->version, goalTypes);
                        type = goalTypes[MATH_Rand32(&rand, count)];
                        roomTemplate = InfinityDungeon_GetRoomTemplate(type);
                        dungeon->floors[i].rooms[j].type = type;
                        dungeon->floors[i].rooms[j].kind = roomTemplate->kind;
                    } else {
                        u8 type = types[placed];

                        if (type != INFINITY_DUNGEON_ROOM_TYPE_NONE) {
                            const InfinityDungeonRoomTemplate *roomTemplate = InfinityDungeon_GetRoomTemplate(type);

                            dungeon->floors[i].rooms[j].type = type;
                            dungeon->floors[i].rooms[j].kind = roomTemplate->kind;
                        }
                        placed++;
                    }
                }
            }
        }
    }
}

static u8 func_ov127_021f1308(u32 rank, u8 floor, u8 version, u8 *types) {
    int count = 0;
    const u8 *counts = func_ov127_021f0d88(rank, floor, version);
    int i;

    sys_memset(types, INFINITY_DUNGEON_ROOM_TYPE_NONE, INFINITY_DUNGEON_ROOM_COUNT);
    for (i = 0; i < 20; i++) {
        u8 n = counts[i];

        if (n != 0) {
            do {
                types[count++] = i;
            } while (--n != 0);
        }
    }
    return count;
}

static u8 func_ov127_021f1348(u32 rank, u8 floor, u8 version, u8 *types) {
    int count = 0;
    const u8 *counts = func_ov127_021f0d88(rank, floor, version);
    int i;

    sys_memset(types, INFINITY_DUNGEON_ROOM_TYPE_NONE, INFINITY_DUNGEON_ROOM_COUNT);
    for (i = 0; i < INFINITY_DUNGEON_ROOM_TYPE_NONE; i++) {
        if (counts[i] != 0 && InfinityDungeon_GetRoomTemplate(i)->kind == 4) {
            types[count++] = i;
        }
    }
    return count;
}

// The rooms of the floor but its last row that have been set apart, by their number
static int func_ov127_021f1388(InfinityDungeonRoom *rooms, int *list, int width, int height) {
    int count = 0;
    int x;
    int y;
    InfinityDungeonGridPos pos;

    sys_memset(list, 16, 16 * sizeof(int));
    for (y = 0; y < height - 1; y++) {
        for (x = 0; x < width; x++) {
            pos.x = x;
            pos.y = y;
            if (func_ov127_021f13e8(&pos, rooms)->kind != 0) {
                list[count] = x + y * INFINITY_DUNGEON_GRID_WIDTH;
                count++;
            }
        }
    }
    return count;
}

static InfinityDungeonRoom *func_ov127_021f13e8(const InfinityDungeonGridPos *pos, InfinityDungeonRoom *rooms) {
    return &rooms[pos->x + pos->y * INFINITY_DUNGEON_GRID_WIDTH];
}

static const InfinityDungeonRoom *func_ov127_021f13f8(const InfinityDungeonGridPos *pos, const InfinityDungeonRoom *rooms) {
    return &rooms[pos->x + pos->y * INFINITY_DUNGEON_GRID_WIDTH];
}

// Opens a passage from the walk's room in the direction (up, down, left or right) and moves into the room beyond
static void func_ov127_021f1408(u8 direction, InfinityDungeonWalk *walk, InfinityDungeonRoom *rooms) {
    switch (direction) {
    case 0:
        func_ov127_021f13e8(&walk->cur, rooms)->open[0] = 1;
        func_ov127_021f15f8(walk, &walk->cur);
        walk->cur.y--;
        func_ov127_021f13e8(&walk->cur, rooms)->open[1] = 1;
        break;
    case 1:
        func_ov127_021f13e8(&walk->cur, rooms)->open[1] = 1;
        func_ov127_021f15f8(walk, &walk->cur);
        walk->cur.y++;
        func_ov127_021f13e8(&walk->cur, rooms)->open[0] = 1;
        break;
    case 3:
        func_ov127_021f13e8(&walk->cur, rooms)->open[3] = 1;
        func_ov127_021f15f8(walk, &walk->cur);
        walk->cur.x--;
        func_ov127_021f13e8(&walk->cur, rooms)->open[2] = 1;
        break;
    case 2:
        func_ov127_021f13e8(&walk->cur, rooms)->open[2] = 1;
        func_ov127_021f15f8(walk, &walk->cur);
        walk->cur.x++;
        func_ov127_021f13e8(&walk->cur, rooms)->open[3] = 1;
        break;
    }
}

// The directions that the walk can dig into from a room, which are rooms nobody has dug into yet unless allowVisited
static int func_ov127_021f14c0(const InfinityDungeonGridPos *pos, const InfinityDungeonRoom *rooms, int *directions,
                               int width, int height, BOOL allowVisited) {
    int count = 0;
    InfinityDungeonGridPos east;
    InfinityDungeonGridPos west;
    InfinityDungeonGridPos south;
    InfinityDungeonGridPos north;
    u8 kind;

    sys_memset(directions, 4, 4 * sizeof(int));
    east = *pos;
    east.x++;
    if (east.x < width && func_ov127_021f13f8(&east, rooms)->kind != 1) {
        if (allowVisited == 0) {
            if (func_ov127_021f15d8(&east, rooms)) {
                directions[count] = 2;
                count++;
            }
        } else {
            directions[count] = 2;
            count++;
        }
    }
    west = *pos;
    west.x--;
    if (west.x >= 0 && func_ov127_021f13f8(&west, rooms)->kind != 1) {
        if (allowVisited == 0) {
            if (func_ov127_021f15d8(&west, rooms)) {
                directions[count] = 3;
                count++;
            }
        } else {
            directions[count] = 3;
            count++;
        }
    }
    south = *pos;
    south.y++;
    if (south.y < height) {
        kind = func_ov127_021f13f8(&south, rooms)->kind;
        if (kind != 1 && kind != 0) {
            if (allowVisited == 0) {
                if (func_ov127_021f15d8(&south, rooms)) {
                    directions[count] = 1;
                    count++;
                }
            } else {
                directions[count] = 1;
                count++;
            }
        }
    }
    north = *pos;
    if (func_ov127_021f13f8(&north, rooms)->kind != 0) {
        north.y--;
        if (func_ov127_021f13f8(&north, rooms)->kind != 1 && north.y >= 0) {
            if (allowVisited == 0) {
                if (func_ov127_021f15d8(&north, rooms)) {
                    directions[count] = 0;
                    count++;
                }
            } else {
                directions[count] = 0;
                count++;
            }
        }
    }
    return count;
}

// Whether no passage has been opened into or out of the room
static BOOL func_ov127_021f15d8(const InfinityDungeonGridPos *pos, const InfinityDungeonRoom *rooms) {
    const InfinityDungeonRoom *room = func_ov127_021f13f8(pos, rooms);
    int i;

    for (i = 0; i < 4; i++) {
        if (room->open[i] == 1) {
            return FALSE;
        }
    }
    return TRUE;
}

static void func_ov127_021f15f8(InfinityDungeonWalk *walk, const InfinityDungeonGridPos *pos) {
    walk->top++;
    walk->stack[walk->top] = *pos;
}

static void func_ov127_021f160c(InfinityDungeonWalk *walk) {
    if (walk->top >= 0) {
        walk->cur = walk->stack[walk->top];
        walk->stack[walk->top].x = 0;
        walk->stack[walk->top].y = 0;
        walk->top--;
    }
}

// Whether every room that isn't the goal has been dug into
static BOOL func_ov127_021f1634(const InfinityDungeonRoom *rooms, int width, int height) {
    int x;
    int y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            InfinityDungeonGridPos pos = { x, y };

            if (func_ov127_021f13f8(&pos, rooms)->kind != 1 && func_ov127_021f15d8(&pos, rooms) == TRUE) {
                return FALSE;
            }
        }
    }
    return TRUE;
}
