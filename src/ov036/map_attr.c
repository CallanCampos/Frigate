// The map's tile attributes: each tile's class and flags, and which classes are grass, water, ledges, furniture and
// the rest. The name is descriptive; the ROM has no string for this file. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"

static BOOL MapTile_IsNormalTallGrassSingleBtl(u32 tileClass);
static BOOL MapTile_IsTallGrassSingleBtl(u32 tileClass);
static BOOL MapTile_IsSandD(u32 tileClass);

// The battle terrain of each tile class
// clang-format off
static const u8 data_ov036_021d0324[40][2] = {
    {0x00, 4},  {0x02, 2},  {0x03, 1}, {0x04, 5},  {0x05, 5},  {0x06, 5},  {0x07, 5}, {0x08, 5},
    {0x09, 5},  {0x0a, 10}, {0x0b, 8}, {0x0c, 8},  {0x0e, 7},  {0x0f, 7},  {0x10, 13}, {0x11, 4},
    {0x13, 4},  {0x14, 11}, {0x15, 11}, {0x16, 14}, {0x17, 12}, {0x18, 13}, {0x19, 8}, {0x1b, 4},
    {0x1c, 9},  {0x1e, 3},  {0x1f, 0}, {0x20, 4},  {0x21, 5},  {0x22, 5},  {0x23, 8}, {0x25, 17},
    {0x26, 4},  {0x30, 10}, {0x3d, 6}, {0x3f, 6},  {0x43, 6},  {0x7c, 8},  {0xa1, 1}, {0xbe, 1},
};

// clang-format on

u32 GetTileTypeAtPos(FieldG3DMapper *mapper, const VecFx32 *position) {
    MapTerrainBuf terrain;

    if (FieldG3DMapper_GetTerrain(mapper, position, &terrain) == TRUE) {
        return terrain.tileType;
    }
    return 0xffffffff;
}

u16 GetTileClass(u32 tileType) {
    return tileType & 0xffff;
}

u32 GetTileFlags(u32 tileType) {
    return (u16)(tileType >> 16);
}

BOOL MapTile_IsValid(u32 tileType) {
    u16 tileClass = GetTileClass(tileType);

    if (tileType == 0xffffffff || tileClass == 0xff) {
        return FALSE;
    }
    return TRUE;
}

BOOL MapTile_BlocksCollision(u32 tileType) {
    u32 flags = GetTileFlags(tileType);

    if (!MapTile_IsValid(tileType) || (flags & 1)) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReallyTallGrass(u16 tileClass) {
    if (tileClass == 8 || tileClass == 9) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeU(u32 tileClass) {
    if (tileClass == 0x74) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeD(u32 tileClass) {
    if (tileClass == 0x75) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeL(u32 tileClass) {
    if (tileClass == 0x73) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeR(u32 tileClass) {
    if (tileClass == 0x72) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsQuicksand(u32 tileClass) {
    if (tileClass == 0x7c) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReceptionCounter(u32 tileClass) {
    if (tileClass == 0xd4) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsPC(u32 tileClass) {
    if (tileClass == 0xd6) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsUnused0xD7(u32 tileClass) {
    if (tileClass == 0xd7) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTV(u32 tileClass) {
    if (tileClass == 0xd8) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsVendingMachine(u32 tileClass) {
    if (tileClass == 0xe2) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseA(u32 tileClass) {
    if (tileClass == 0xd9) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseB(u32 tileClass) {
    if (tileClass == 0xda) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseC(u32 tileClass) {
    if (tileClass == 0xdb) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseD(u32 tileClass) {
    if (tileClass == 0xdc) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTrashCanA(u32 tileClass) {
    if (tileClass == 0xdd) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTrashCanB(u32 tileClass) {
    if (tileClass == 0xde) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAssortmentA(u32 tileClass) {
    if (tileClass == 0xdf) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAssortmentB(u32 tileClass) {
    if (tileClass == 0xe0) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAssortmentC(u32 tileClass) {
    if (tileClass == 0xe1) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MapTile_IsNormalTallGrassSingleBtl(u32 tileClass) {
    if (tileClass == 4 || tileClass == 0x21 || tileClass == 5) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsNormalTallGrassDoubleBtl(u32 tileClass) {
    if (tileClass == 6 || tileClass == 0x22 || tileClass == 7) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReallyTallGrassSingleBtl(u32 tileClass) {
    if (tileClass == 0x8) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReallyTallGrassDoubleBtl(u32 tileClass) {
    if (tileClass == 0x9) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MapTile_IsTallGrassSingleBtl(u32 tileClass) {
    if (MapTile_IsNormalTallGrassSingleBtl(tileClass) == TRUE ||
        MapTile_IsReallyTallGrassSingleBtl(tileClass) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTallGrassDoubleBtl(u32 tileClass) {
    if (MapTile_IsNormalTallGrassDoubleBtl(tileClass) == TRUE ||
        MapTile_IsReallyTallGrassDoubleBtl(tileClass) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTallGrass(u32 tileClass) {
    if (MapTile_IsTallGrassSingleBtl(tileClass) == TRUE || MapTile_IsTallGrassDoubleBtl(tileClass) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsStrengthHole(u32 tileClass) {
    if (tileClass == 0x1d) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSurfEdge(u32 tileClass) {
    if (tileClass == 0x41 || tileClass == 0x44) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov036_021a2bf4(u32 tileClass) {
    switch (tileClass) {
    case 0x14:
    case 0x15:
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSplashWater(u32 tileClass) {
    if (tileClass == 0x17) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsTileWaterfall(u32 tileClass) {
    if (tileClass == 0x40) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSnow_(u32 tileClass) {
    if (tileClass == 0xe) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSnowNoCycling(u32 tileClass) {
    if (tileClass == 0xf) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSnow(u32 tileClass) {
    if (MapTile_IsSnow_(tileClass)) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsTileSlidingIce(u32 tileClass) {
    if (tileClass == 0x18 || tileClass == 0x10) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsIce(u32 tileClass) {
    if (tileClass == 0x10) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSwamp(u32 tileClass) {
    if (tileClass == 0x1c) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MapTile_IsSandD(u32 tileClass) {
    if (tileClass == 0x19) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsTileSand(u32 tileClass) {
    if (tileClass == 0xc || tileClass == 0xb || tileClass == 0x23) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAnySand(u32 tileClass) {
    if (MapTile_IsSandD(tileClass) || IsTileSand(tileClass)) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsTileSurfWater(u16 tileClass) {
    if (tileClass == 0x3d || tileClass == 0x3e || tileClass == 0x3f || tileClass == 0x42 || tileClass == 0x43) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov036_021a2cd4(u32 tileClass) {
    if (tileClass == 0x25) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsPuddle(u32 tileClass) {
    if (tileClass == 0x2) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsMud(u32 tileClass) {
    if (tileClass == 0x1e) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsElectricField(u32 tileClass) {
    if (tileClass == 0x30) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsElectricRock(u32 tileClass) {
    if (tileClass == 0x32) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsCatwalkBody_(u32 tileClass) {
    if (tileClass == 0xbe) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsCatwalkEntryPoint(u32 tileClass) {
    if (tileClass == 0xbf) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsDeepSand(u32 tileClass) {
    if (tileClass == 0xc) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassSingleB(u32 tileClass) {
    if (tileClass == 0x5) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassDoubleB(u32 tileClass) {
    if (tileClass == 0x7) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsMirrorFloor(u32 tileClass) {
    if (tileClass == 0x1b) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Up(u16 tileClass) {
    if (tileClass == 0x53 || tileClass == 0x55 || tileClass == 0x56) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Down(u16 tileClass) {
    if (tileClass == 0x54 || tileClass == 0x57 || tileClass == 0x58) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Left(u16 tileClass) {
    if (tileClass == 0x52 || tileClass == 0x56 || tileClass == 0x58) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Right(u16 tileClass) {
    if (tileClass == 0x51 || tileClass == 0x55 || tileClass == 0x57) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBlocksCycling(u16 tileClass) {
    if (MapTile_IsReallyTallGrass(tileClass) || MapTile_IsCatwalkBody_(tileClass) ||
        MapTile_IsCatwalkEntryPoint(tileClass) || MapTile_IsSnowNoCycling(tileClass) ||
        MapTile_IsHiddenGrottoEntranceGrass(tileClass)) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassSingleC(u32 tileClass) {
    if (tileClass == 0x21) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassDoubleC(u32 tileClass) {
    if (tileClass == 0x22) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsHiddenGrottoEntranceGrass(u32 tileClass) {
    if (tileClass == 0x24) {
        return TRUE;
    }
    return FALSE;
}

// The kind of phenomenon the tile can have: 0 to 3 for grass, 4 for dust clouds, 5 and 6 for water, 7 for flying
// shadows and 8 for none
u32 MapTile_GetEncounterEffectKind(u32 tileType) {
    u16 tileClass = tileType;
    u16 flags = tileType >> 16;

    if (tileClass == 0x20) {
        return 7;
    }
    if (!(flags & 4)) {
        return 8;
    }
    switch (tileClass) {
    case 4:
        return 0;
    case 5:
        return 1;
    case 0x21:
        return 2;
    case 8:
        return 3;
    case 10:
        return 4;
    case 0x3d:
        return 5;
    case 0x3f:
    case 0x43:
        return 6;
    default:
        return 8;
    }
}

u32 GetTileEncountType(u32 tileClass) {
    u32 i;

    for (i = 0; i < 40; i++) {
        if (tileClass == data_ov036_021d0324[i][0]) {
            return data_ov036_021d0324[i][1];
        }
    }
    return 0;
}

// Whether the player can start surfing from the tile onto the tile in front
BOOL CheckSurfBeginTiles(u32 tileType, u32 frontTileType) {
    u32 flags = GetTileFlags(frontTileType);
    u16 tileClass = GetTileClass(frontTileType);

    if (MapTile_BlocksCollision(tileType)) {
        return FALSE;
    }
    if (((flags & 2) && !MapTile_BlocksCollision(frontTileType)) || MapTile_IsSurfEdge(tileClass)) {
        return TRUE;
    }
    return FALSE;
}
