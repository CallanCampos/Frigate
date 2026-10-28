#ifndef POKEBW2_FIELD_FIELD_MAP_H
#define POKEBW2_FIELD_FIELD_MAP_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "save/gimmick_savedata.h"
#include "struct_decls.h"

struct MapMatrix {
    u16 heapId;
    u16 zoneId;
    u32 matrixId;
    u16 width;
    u16 height;
    u16 format;
    u16 unk0E;
    u32 chunkIdCount;
    u16 zoneIds[900];
    u32 chunkIds[900];
};

// The terrain of a layer at a position. Layout from swan, which names the normal's components SlopeX, HeightDiv and
// SlopeZ
struct MapTerrainBuf {
    VecFx16 normal;
    u16 pad06;
    u32 tileType;
    fx32 height;
};

// The terrain at a position in a chunk, a layer for each height. Layout from swan
struct MapTerrainSamplerOutput {
    MapTerrainBuf layers[16];
    u32 layerCount;
};

struct MapMatrixFileHeader {
    u16 format;
    u16 unk02;
    u16 width;
    u16 height;
    // The chunks, then the zones if the format is 1
    u32 chunkIds[];
};

// An entry of the map replacements, file 0 of archive 10
typedef struct {
    u16 id;
    u8 kind;
    // What picks the value: the season, the version, both, or an event of EVENT_MAP_REPLACE_TABLE
    u8 condition;
    // The original value, then the replacements
    u16 values[6];
} MapReplaceEntry;

// The values that MapReplaceEntry.condition picks from
typedef struct {
    u8 season;
    // 0 in Black and Black 2, 1 in White and White 2
    u8 version;
    // 0 in Black and Black 2, the season plus 1 in White and White 2
    u8 versionSeason;
    // Whether each event of EVENT_MAP_REPLACE_TABLE happened
    u8 events[10];
} MapReplaceVariables;

struct MapReplace {
    u16 heapId;
    ArcTool *arc;
    u32 entryCount;
    MapReplaceVariables variables;
    MapReplaceEntry entry;
};

struct MapReplaceEvent {
    u8 condition;
    u8 uid;
    u16 workId;
    u16 expectedValue;
};

extern const MapReplaceEvent EVENT_MAP_REPLACE_TABLE[];

extern const char data_ov012_0216e1c4[];
extern const char data_ov012_0216e1d4[];

MapMatrix *InitMapMatrix(HeapID heapId);
void MapMatrix_Load(MapMatrix *matrix, u16 matrixId, u16 zoneId, HeapID heapId);
void ProcessMapMatrix(MapMatrix *matrix, const MapMatrixFileHeader *data, u32 matrixId, u16 zoneId);
void FreeMapMatrix(MapMatrix *matrix);
u16 GetZoneIDAtMatrixXZ(MapMatrix *matrix, s32 x, s32 z);
u16 GetZoneIDAtMatrixXZWorld(MapMatrix *matrix, s32 x, s32 z);
// What a matrix's chunk without a zone has
#define MATRIX_ZONE_NONE 0xffff
u16 GetMapMatrixWidth(MapMatrix *matrix);
u16 GetMapMatrixHeight(MapMatrix *matrix);
u32 GetMapMatrixChunkIDCount(MapMatrix *matrix);
u32 *GetMapMatrixChunkIDs(MapMatrix *matrix);
BOOL RangeCheckChunkCoordinate(MapMatrix *matrix, s32 x, s32 z);
BOOL RangeCheckChunkCoordinateWorld(MapMatrix *matrix, s32 x, s32 z);
u32 GetChunkCoordOfWorld(s32 coordinate);
MapReplace *MapReplace_Create(HeapID heapId, GameSystem *gsys);
s32 MapReplace_GetEntryCount(MapReplace *replace);
s32 MapReplace_LoadEntry(MapReplace *replace, u32 index);
void MapReplace_Free(MapReplace *replace);
void MapReplace_Patch(MapMatrix *matrix, MapReplace *replace, HeapID heapId);
u32 MapReplace_ResolvePatch(MapReplace *replace, u32 *oldValue, u32 *newValue);
int MapReplace_GetEventByCond(u8 condition);
const MapReplaceEvent *MapReplace_GetEventByUID(GameData *gameData, u16 uid);
void GameData_SetEventMapReplace(GameData *gameData, u16 uid, BOOL set);
BOOL GameData_IsMapReplaceEventSet(GameData *gameData, u16 uid);
void MapReplace_LoadVariables(MapReplaceVariables *variables, GameSystem *gsys);
void MapMatrix_Patch(MapMatrix *matrix, GameSystem *gsys, HeapID heapId);
// What GetTileTypeAtPos returns where there is no terrain
#define TILE_TYPE_NONE 0xffffffff

u32 GetTileTypeAtPos(FieldG3DMapper *mapper, const VecFx32 *position);
// The tile flags that GetTileFlags returns: the one that MapTile_BlocksCollision tests, the one of water, which the
// player can't jump onto from the water and which gives battles their water music, the one of the tiles where wild
// Pokémon appear, and the one of the tiles that show an actor's reflection, on them or on the tile below
#define TILE_FLAG_COLLISION 0x1
#define TILE_FLAG_WATER 0x2
#define TILE_FLAG_ENCOUNT 0x4
#define TILE_FLAG_REFLECT 0x40

// The tile classes that GetTileClass returns, as map_attr.c's and rail_attr.c's predicates, the slope and water
// current tables, the encounter phenomena and the battle terrain table test them. A class that the code tells apart
// from the others only by its number keeps it: those of the battle terrain table alone, and the second of two classes
// that every check treats alike (the first being the one the maps use most)
enum {
    TILE_CLASS_NORMAL = 0x00,
    TILE_CLASS_PUDDLE = 0x02,
    TILE_CLASS_03 = 0x03,
    // Grass, a single and a double battle's, each in three kinds (swan's A, B and C, the first unlettered); B's
    // effect changes in winter
    TILE_CLASS_GRASS = 0x04,
    TILE_CLASS_GRASS_B = 0x05,
    TILE_CLASS_GRASS_DOUBLE = 0x06,
    TILE_CLASS_GRASS_DOUBLE_B = 0x07,
    TILE_CLASS_REALLY_TALL_GRASS = 0x08,
    TILE_CLASS_REALLY_TALL_GRASS_DOUBLE = 0x09,
    // The caves' floor, where dust clouds rise
    TILE_CLASS_CAVE = 0x0a,
    TILE_CLASS_SAND = 0x0b,
    TILE_CLASS_DEEP_SAND = 0x0c,
    TILE_CLASS_SNOW = 0x0e,
    TILE_CLASS_SNOW_NO_CYCLING = 0x0f,
    // Ice that the player slides on and that reflects like a mirror floor
    TILE_CLASS_MIRROR_ICE = 0x10,
    TILE_CLASS_11 = 0x11,
    TILE_CLASS_13 = 0x13,
    TILE_CLASS_RIPPLE_WATER = 0x14,
    TILE_CLASS_RIPPLE_WATER_15 = 0x15,
    TILE_CLASS_16 = 0x16,
    TILE_CLASS_SPLASH_WATER = 0x17,
    TILE_CLASS_ICE = 0x18,
    TILE_CLASS_SAND_19 = 0x19,
    TILE_CLASS_MIRROR_FLOOR = 0x1b,
    TILE_CLASS_SWAMP = 0x1c,
    // A hole that Strength's boulders fill
    TILE_CLASS_STRENGTH_HOLE = 0x1d,
    TILE_CLASS_MUD = 0x1e,
    TILE_CLASS_1F = 0x1f,
    // The bridges, where flying Pokémon's shadows appear
    TILE_CLASS_BRIDGE = 0x20,
    TILE_CLASS_GRASS_C = 0x21,
    TILE_CLASS_GRASS_DOUBLE_C = 0x22,
    TILE_CLASS_SAND_23 = 0x23,
    TILE_CLASS_HIDDEN_GROTTO_GRASS = 0x24,
    TILE_CLASS_FALLEN_LEAVES = 0x25,
    TILE_CLASS_26 = 0x26,
    // Where the player steps between the grid and a rail by walking into a wall, as on TILE_CLASS_GRID_RAIL_SWITCH
    TILE_CLASS_GRID_RAIL_SWITCH_28 = 0x28,
    TILE_CLASS_ELECTRIC_FIELD = 0x30,
    TILE_CLASS_ELECTRIC_ROCK = 0x32,
    // Water to surf on: the sea's and the rivers' (TILE_CLASS_WATER), the caves' and ponds' (0x3d) and others
    TILE_CLASS_WATER_3D = 0x3d,
    TILE_CLASS_WATER_3E = 0x3e,
    TILE_CLASS_WATER = 0x3f,
    TILE_CLASS_WATERFALL = 0x40,
    // A shore that the player can start surfing or fishing from
    TILE_CLASS_SURF_EDGE = 0x41,
    TILE_CLASS_WATER_42 = 0x42,
    TILE_CLASS_WATER_43 = 0x43,
    TILE_CLASS_SURF_EDGE_44 = 0x44,
    // A tile with a wall on a side or two, which actors can't cross in either direction
    TILE_CLASS_BLOCK_RIGHT = 0x51,
    TILE_CLASS_BLOCK_LEFT = 0x52,
    TILE_CLASS_BLOCK_UP = 0x53,
    TILE_CLASS_BLOCK_DOWN = 0x54,
    TILE_CLASS_BLOCK_UP_RIGHT = 0x55,
    TILE_CLASS_BLOCK_UP_LEFT = 0x56,
    TILE_CLASS_BLOCK_DOWN_RIGHT = 0x57,
    TILE_CLASS_BLOCK_DOWN_LEFT = 0x58,
    // A ledge that the player jumps off in its direction
    TILE_CLASS_LEDGE_RIGHT = 0x72,
    TILE_CLASS_LEDGE_LEFT = 0x73,
    TILE_CLASS_LEDGE_UP = 0x74,
    TILE_CLASS_LEDGE_DOWN = 0x75,
    // A slope that the player slides down in its direction
    TILE_CLASS_SLOPE_SLIP_RIGHT = 0x76,
    TILE_CLASS_SLOPE_SLIP_LEFT = 0x77,
    TILE_CLASS_SLOPE_SLIP_UP = 0x78,
    TILE_CLASS_SLOPE_SLIP_DOWN = 0x79,
    TILE_CLASS_QUICKSAND = 0x7c,
    // A current that carries the surfing player in its direction
    TILE_CLASS_WATER_CURRENT_RIGHT = 0x94,
    TILE_CLASS_WATER_CURRENT_LEFT = 0x95,
    TILE_CLASS_WATER_CURRENT_UP = 0x96,
    TILE_CLASS_WATER_CURRENT_DOWN = 0x97,
    // A rail's edge that the player slips down off by walking down into it
    TILE_CLASS_SLIP_DOWN_EDGE = 0xa0,
    TILE_CLASS_SLIP_DOWN_EDGE_A1 = 0xa1,
    TILE_CLASS_GRID_RAIL_SWITCH = 0xa3,
    // A rail's tile that moves the player on, in its direction, until one that stops it
    TILE_CLASS_FORCE_MOVE_DOWN = 0xa4,
    TILE_CLASS_FORCE_MOVE_UP = 0xa5,
    TILE_CLASS_FORCE_MOVE_RIGHT = 0xa6,
    TILE_CLASS_FORCE_MOVE_LEFT = 0xa7,
    TILE_CLASS_FORCE_MOVE_STOP = 0xa8,
    TILE_CLASS_SLIP_DOWN = 0xb0,
    TILE_CLASS_CATWALK = 0xbe,
    TILE_CLASS_CATWALK_ENTRY = 0xbf,
    // The furniture that runs a script when the player faces it and presses A
    TILE_CLASS_RECEPTION_COUNTER = 0xd4,
    TILE_CLASS_PC = 0xd6,
    TILE_CLASS_UNUSED_D7 = 0xd7,
    TILE_CLASS_TV = 0xd8,
    TILE_CLASS_BOOKCASE_A = 0xd9,
    TILE_CLASS_BOOKCASE_B = 0xda,
    TILE_CLASS_BOOKCASE_C = 0xdb,
    TILE_CLASS_BOOKCASE_D = 0xdc,
    TILE_CLASS_TRASH_CAN_A = 0xdd,
    TILE_CLASS_TRASH_CAN_B = 0xde,
    TILE_CLASS_ASSORTMENT_A = 0xdf,
    TILE_CLASS_ASSORTMENT_B = 0xe0,
    TILE_CLASS_ASSORTMENT_C = 0xe1,
    TILE_CLASS_VENDING_MACHINE = 0xe2,
    // The class of a tile that isn't one
    TILE_CLASS_NONE = 0xff,
};

u32 GetTileFlags(u32 tileType);
u16 GetAbyssalRuinsDiveZoneID(Field *field, u16 *zoneId);
u16 GetTileClass(u32 tileType);
BOOL MapTile_IsValid(u32 tileType);
BOOL MapTile_BlocksCollision(u32 tileType);
BOOL MapTile_IsBlocksCycling(u16 tileClass);
BOOL MapTile_IsReallyTallGrass(u16 tileClass);
BOOL MapTile_IsNormalTallGrassDoubleBtl(u32 tileClass);
BOOL MapTile_IsTallGrassDoubleBtl(u32 tileClass);
BOOL MapTile_IsTallGrass(u32 tileClass);
// The battle terrain of the tile class
u32 GetTileEncountType(u32 tileClass);
BOOL MapTile_IsReallyTallGrassSingleBtl(u32 tileClass);
BOOL MapTile_IsReallyTallGrassDoubleBtl(u32 tileClass);
BOOL MapTile_IsRippleWater(u32 tileClass);
BOOL MapTile_IsSplashWater(u32 tileClass);
BOOL MapTile_IsSnowNoCycling(u32 tileClass);
BOOL MapTile_IsSnow(u32 tileClass);
BOOL MapTile_IsIce(u32 tileClass);
BOOL MapTile_IsSwamp(u32 tileClass);
BOOL MapTile_IsFallenLeaves(u32 tileClass);
BOOL MapTile_IsPuddle(u32 tileClass);
BOOL MapTile_IsMud(u32 tileClass);
BOOL MapTile_IsElectricField(u32 tileClass);
BOOL MapTile_IsDeepSand(u32 tileClass);
BOOL MapTile_IsGrassSingleB(u32 tileClass);
BOOL MapTile_IsGrassDoubleB(u32 tileClass);
BOOL MapTile_IsMirrorFloor(u32 tileClass);
// Whether a tile class keeps an actor from leaving it in a direction
BOOL TileExitBlockCheck_Up(u16 tileClass);
BOOL TileExitBlockCheck_Down(u16 tileClass);
BOOL TileExitBlockCheck_Left(u16 tileClass);
BOOL TileExitBlockCheck_Right(u16 tileClass);
BOOL MapTile_IsGrassSingleC(u32 tileClass);
BOOL MapTile_IsGrassDoubleC(u32 tileClass);
BOOL FieldRailTilemap_IsJumpStart(u32 tileClass);
BOOL MapTile_IsSurfEdge(u32 tileClass);
BOOL IsTileSurfWater(u16 tileClass);
// The rest of map_attr.c
BOOL MapTile_IsLedgeU(u32 tileClass);
BOOL MapTile_IsLedgeD(u32 tileClass);
BOOL MapTile_IsLedgeL(u32 tileClass);
BOOL MapTile_IsLedgeR(u32 tileClass);
BOOL MapTile_IsQuicksand(u32 tileClass);
BOOL MapTile_IsReceptionCounter(u32 tileClass);
BOOL MapTile_IsPC(u32 tileClass);
BOOL MapTile_IsUnused0xD7(u32 tileClass);
BOOL MapTile_IsTV(u32 tileClass);
BOOL MapTile_IsVendingMachine(u32 tileClass);
BOOL MapTile_IsBookcaseA(u32 tileClass);
BOOL MapTile_IsBookcaseB(u32 tileClass);
BOOL MapTile_IsBookcaseC(u32 tileClass);
BOOL MapTile_IsBookcaseD(u32 tileClass);
BOOL MapTile_IsTrashCanA(u32 tileClass);
BOOL MapTile_IsTrashCanB(u32 tileClass);
BOOL MapTile_IsAssortmentA(u32 tileClass);
BOOL MapTile_IsAssortmentB(u32 tileClass);
BOOL MapTile_IsAssortmentC(u32 tileClass);
BOOL MapTile_IsStrengthHole(u32 tileClass);
BOOL IsTileWaterfall(u32 tileClass);
BOOL MapTile_IsSnow_(u32 tileClass);
BOOL IsTileSlidingIce(u32 tileClass);
BOOL IsTileSand(u32 tileClass);
BOOL MapTile_IsAnySand(u32 tileClass);
BOOL MapTile_IsElectricRock(u32 tileClass);
BOOL MapTile_IsCatwalkBody_(u32 tileClass);
BOOL MapTile_IsCatwalkEntryPoint(u32 tileClass);
BOOL MapTile_IsHiddenGrottoEntranceGrass(u32 tileClass);
// The phenomena that MapTile_GetEncounterEffectKind finds on a tile, by the effects that fldeff_encount.c makes for
// them: three kinds of grass, which share one effect, really tall grass, a dust cloud, two kinds of water and a flying
// Pokémon's shadow, or none
enum {
    ENCOUNT_EFFECT_GRASS,
    ENCOUNT_EFFECT_GRASS_B,
    ENCOUNT_EFFECT_GRASS_C,
    ENCOUNT_EFFECT_REALLY_TALL_GRASS,
    ENCOUNT_EFFECT_DUST_CLOUD,
    ENCOUNT_EFFECT_WATER_0,
    ENCOUNT_EFFECT_WATER_1,
    ENCOUNT_EFFECT_FLYING_SHADOW,
    ENCOUNT_EFFECT_NONE,
};

u32 MapTile_GetEncounterEffectKind(u32 tileType);
BOOL CheckSurfBeginTiles(u32 tileType, u32 frontTileType);

// Overlay 36: patches of map land data from archive 0x9a, of which Join Avenue's shops are built
typedef struct LandDataPatch LandDataPatch;

LandDataPatch *ReadLandDataPatchA154Data(u16 fileId, HeapID heapId);
void FreeLandDataPatch(LandDataPatch *patch);
// Copies height rows of width cells, from srcX and srcY in the patch to destX and destY in the map
void LandDataPatch_OverwriteAttr(LandDataPatch *patch, LandPatchGrid *map, u32 srcX, u32 srcY, u32 destX, u32 destY,
                                 u32 width, u32 height);
// Adds the patch's buildings at x and y, after the count already added, and returns the new count
u32 LoadLandDataPatchBuildings(LandDataPatch *patch, FieldPropSystem *propSystem, FieldChunk *chunk, u32 count, u32 x,
                               u32 y);

#endif // POKEBW2_FIELD_FIELD_MAP_H
