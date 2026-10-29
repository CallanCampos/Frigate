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
static const u8 TILE_CLASS_BATTLE_TERRAINS[40][2] = {
    { TILE_CLASS_NORMAL, 4 },
    { TILE_CLASS_PUDDLE, 2 },
    { TILE_CLASS_03, 1 },
    { TILE_CLASS_GRASS, 5 },
    { TILE_CLASS_GRASS_B, 5 },
    { TILE_CLASS_GRASS_DOUBLE, 5 },
    { TILE_CLASS_GRASS_DOUBLE_B, 5 },
    { TILE_CLASS_REALLY_TALL_GRASS, 5 },
    { TILE_CLASS_REALLY_TALL_GRASS_DOUBLE, 5 },
    { TILE_CLASS_CAVE, 10 },
    { TILE_CLASS_SAND, 8 },
    { TILE_CLASS_DEEP_SAND, 8 },
    { TILE_CLASS_SNOW, 7 },
    { TILE_CLASS_SNOW_NO_CYCLING, 7 },
    { TILE_CLASS_MIRROR_ICE, 13 },
    { TILE_CLASS_11, 4 },
    { TILE_CLASS_13, 4 },
    { TILE_CLASS_RIPPLE_WATER, 11 },
    { TILE_CLASS_RIPPLE_WATER_15, 11 },
    { TILE_CLASS_16, 14 },
    { TILE_CLASS_SPLASH_WATER, 12 },
    { TILE_CLASS_ICE, 13 },
    { TILE_CLASS_SAND_19, 8 },
    { TILE_CLASS_MIRROR_FLOOR, 4 },
    { TILE_CLASS_SWAMP, 9 },
    { TILE_CLASS_MUD, 3 },
    { TILE_CLASS_1F, 0 },
    { TILE_CLASS_BRIDGE, 4 },
    { TILE_CLASS_GRASS_C, 5 },
    { TILE_CLASS_GRASS_DOUBLE_C, 5 },
    { TILE_CLASS_SAND_23, 8 },
    { TILE_CLASS_FALLEN_LEAVES, 17 },
    { TILE_CLASS_26, 4 },
    { TILE_CLASS_ELECTRIC_FIELD, 10 },
    { TILE_CLASS_WATER_3D, 6 },
    { TILE_CLASS_WATER, 6 },
    { TILE_CLASS_WATER_43, 6 },
    { TILE_CLASS_QUICKSAND, 8 },
    { TILE_CLASS_SLIP_DOWN_EDGE_A1, 1 },
    { TILE_CLASS_CATWALK, 1 },
};

u32 GetTileTypeAtPos(FieldG3DMapper *mapper, const VecFx32 *position) {
    MapTerrainBuf terrain;

    if (FieldG3DMapper_GetTerrain(mapper, position, &terrain) == TRUE) {
        return terrain.tileType;
    }
    return TILE_TYPE_NONE;
}

u16 GetTileClass(u32 tileType) {
    return tileType & 0xffff;
}

u32 GetTileFlags(u32 tileType) {
    return (u16)(tileType >> 16);
}

BOOL MapTile_IsValid(u32 tileType) {
    u16 tileClass = GetTileClass(tileType);

    if (tileType == TILE_TYPE_NONE || tileClass == TILE_CLASS_NONE) {
        return FALSE;
    }
    return TRUE;
}

BOOL MapTile_BlocksCollision(u32 tileType) {
    u32 flags = GetTileFlags(tileType);

    if (!MapTile_IsValid(tileType) || (flags & TILE_FLAG_COLLISION)) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReallyTallGrass(u16 tileClass) {
    if (tileClass == TILE_CLASS_REALLY_TALL_GRASS || tileClass == TILE_CLASS_REALLY_TALL_GRASS_DOUBLE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeU(u32 tileClass) {
    if (tileClass == TILE_CLASS_LEDGE_UP) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeD(u32 tileClass) {
    if (tileClass == TILE_CLASS_LEDGE_DOWN) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeL(u32 tileClass) {
    if (tileClass == TILE_CLASS_LEDGE_LEFT) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsLedgeR(u32 tileClass) {
    if (tileClass == TILE_CLASS_LEDGE_RIGHT) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsQuicksand(u32 tileClass) {
    if (tileClass == TILE_CLASS_QUICKSAND) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReceptionCounter(u32 tileClass) {
    if (tileClass == TILE_CLASS_RECEPTION_COUNTER) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsPC(u32 tileClass) {
    if (tileClass == TILE_CLASS_PC) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsUnused0xD7(u32 tileClass) {
    if (tileClass == TILE_CLASS_UNUSED_D7) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTV(u32 tileClass) {
    if (tileClass == TILE_CLASS_TV) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsVendingMachine(u32 tileClass) {
    if (tileClass == TILE_CLASS_VENDING_MACHINE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseA(u32 tileClass) {
    if (tileClass == TILE_CLASS_BOOKCASE_A) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseB(u32 tileClass) {
    if (tileClass == TILE_CLASS_BOOKCASE_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseC(u32 tileClass) {
    if (tileClass == TILE_CLASS_BOOKCASE_C) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsBookcaseD(u32 tileClass) {
    if (tileClass == TILE_CLASS_BOOKCASE_D) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTrashCanA(u32 tileClass) {
    if (tileClass == TILE_CLASS_TRASH_CAN_A) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsTrashCanB(u32 tileClass) {
    if (tileClass == TILE_CLASS_TRASH_CAN_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAssortmentA(u32 tileClass) {
    if (tileClass == TILE_CLASS_ASSORTMENT_A) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAssortmentB(u32 tileClass) {
    if (tileClass == TILE_CLASS_ASSORTMENT_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsAssortmentC(u32 tileClass) {
    if (tileClass == TILE_CLASS_ASSORTMENT_C) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MapTile_IsNormalTallGrassSingleBtl(u32 tileClass) {
    if (tileClass == TILE_CLASS_GRASS || tileClass == TILE_CLASS_GRASS_C || tileClass == TILE_CLASS_GRASS_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsNormalTallGrassDoubleBtl(u32 tileClass) {
    if (tileClass == TILE_CLASS_GRASS_DOUBLE || tileClass == TILE_CLASS_GRASS_DOUBLE_C ||
        tileClass == TILE_CLASS_GRASS_DOUBLE_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReallyTallGrassSingleBtl(u32 tileClass) {
    if (tileClass == TILE_CLASS_REALLY_TALL_GRASS) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsReallyTallGrassDoubleBtl(u32 tileClass) {
    if (tileClass == TILE_CLASS_REALLY_TALL_GRASS_DOUBLE) {
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
    if (tileClass == TILE_CLASS_STRENGTH_HOLE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSurfEdge(u32 tileClass) {
    if (tileClass == TILE_CLASS_SURF_EDGE || tileClass == TILE_CLASS_SURF_EDGE_44) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsRippleWater(u32 tileClass) {
    switch (tileClass) {
    case TILE_CLASS_RIPPLE_WATER:
    case TILE_CLASS_RIPPLE_WATER_15:
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSplashWater(u32 tileClass) {
    if (tileClass == TILE_CLASS_SPLASH_WATER) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsTileWaterfall(u32 tileClass) {
    if (tileClass == TILE_CLASS_WATERFALL) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSnow_(u32 tileClass) {
    if (tileClass == TILE_CLASS_SNOW) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSnowNoCycling(u32 tileClass) {
    if (tileClass == TILE_CLASS_SNOW_NO_CYCLING) {
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
    if (tileClass == TILE_CLASS_ICE || tileClass == TILE_CLASS_MIRROR_ICE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsIce(u32 tileClass) {
    if (tileClass == TILE_CLASS_MIRROR_ICE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsSwamp(u32 tileClass) {
    if (tileClass == TILE_CLASS_SWAMP) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MapTile_IsSandD(u32 tileClass) {
    if (tileClass == TILE_CLASS_SAND_19) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsTileSand(u32 tileClass) {
    if (tileClass == TILE_CLASS_DEEP_SAND || tileClass == TILE_CLASS_SAND || tileClass == TILE_CLASS_SAND_23) {
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
    if (tileClass == TILE_CLASS_WATER_3D || tileClass == TILE_CLASS_WATER_3E || tileClass == TILE_CLASS_WATER ||
        tileClass == TILE_CLASS_WATER_42 || tileClass == TILE_CLASS_WATER_43) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsFallenLeaves(u32 tileClass) {
    if (tileClass == TILE_CLASS_FALLEN_LEAVES) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsPuddle(u32 tileClass) {
    if (tileClass == TILE_CLASS_PUDDLE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsMud(u32 tileClass) {
    if (tileClass == TILE_CLASS_MUD) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsElectricField(u32 tileClass) {
    if (tileClass == TILE_CLASS_ELECTRIC_FIELD) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsElectricRock(u32 tileClass) {
    if (tileClass == TILE_CLASS_ELECTRIC_ROCK) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsCatwalkBody_(u32 tileClass) {
    if (tileClass == TILE_CLASS_CATWALK) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsCatwalkEntryPoint(u32 tileClass) {
    if (tileClass == TILE_CLASS_CATWALK_ENTRY) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsDeepSand(u32 tileClass) {
    if (tileClass == TILE_CLASS_DEEP_SAND) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassSingleB(u32 tileClass) {
    if (tileClass == TILE_CLASS_GRASS_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassDoubleB(u32 tileClass) {
    if (tileClass == TILE_CLASS_GRASS_DOUBLE_B) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsMirrorFloor(u32 tileClass) {
    if (tileClass == TILE_CLASS_MIRROR_FLOOR) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Up(u16 tileClass) {
    if (tileClass == TILE_CLASS_BLOCK_UP || tileClass == TILE_CLASS_BLOCK_UP_RIGHT ||
        tileClass == TILE_CLASS_BLOCK_UP_LEFT) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Down(u16 tileClass) {
    if (tileClass == TILE_CLASS_BLOCK_DOWN || tileClass == TILE_CLASS_BLOCK_DOWN_RIGHT ||
        tileClass == TILE_CLASS_BLOCK_DOWN_LEFT) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Left(u16 tileClass) {
    if (tileClass == TILE_CLASS_BLOCK_LEFT || tileClass == TILE_CLASS_BLOCK_UP_LEFT ||
        tileClass == TILE_CLASS_BLOCK_DOWN_LEFT) {
        return TRUE;
    }
    return FALSE;
}

BOOL TileExitBlockCheck_Right(u16 tileClass) {
    if (tileClass == TILE_CLASS_BLOCK_RIGHT || tileClass == TILE_CLASS_BLOCK_UP_RIGHT ||
        tileClass == TILE_CLASS_BLOCK_DOWN_RIGHT) {
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
    if (tileClass == TILE_CLASS_GRASS_C) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsGrassDoubleC(u32 tileClass) {
    if (tileClass == TILE_CLASS_GRASS_DOUBLE_C) {
        return TRUE;
    }
    return FALSE;
}

BOOL MapTile_IsHiddenGrottoEntranceGrass(u32 tileClass) {
    if (tileClass == TILE_CLASS_HIDDEN_GROTTO_GRASS) {
        return TRUE;
    }
    return FALSE;
}

// The kind of phenomenon the tile can have: 0 to 3 for grass, 4 for dust clouds, 5 and 6 for water, 7 for flying
// shadows and 8 for none
u32 MapTile_GetEncounterEffectKind(u32 tileType) {
    u16 tileClass = tileType;
    u16 flags = tileType >> 16;

    if (tileClass == TILE_CLASS_BRIDGE) {
        return ENCOUNT_EFFECT_FLYING_SHADOW;
    }
    if (!(flags & TILE_FLAG_ENCOUNT)) {
        return ENCOUNT_EFFECT_NONE;
    }
    switch (tileClass) {
    case TILE_CLASS_GRASS:
        return ENCOUNT_EFFECT_GRASS;
    case TILE_CLASS_GRASS_B:
        return ENCOUNT_EFFECT_GRASS_B;
    case TILE_CLASS_GRASS_C:
        return ENCOUNT_EFFECT_GRASS_C;
    case TILE_CLASS_REALLY_TALL_GRASS:
        return ENCOUNT_EFFECT_REALLY_TALL_GRASS;
    case TILE_CLASS_CAVE:
        return ENCOUNT_EFFECT_DUST_CLOUD;
    case TILE_CLASS_WATER_3D:
        return ENCOUNT_EFFECT_WATER_0;
    case TILE_CLASS_WATER:
    case TILE_CLASS_WATER_43:
        return ENCOUNT_EFFECT_WATER_1;
    default:
        return ENCOUNT_EFFECT_NONE;
    }
}

u32 GetTileEncountType(u32 tileClass) {
    u32 i;

    for (i = 0; i < NELEMS(TILE_CLASS_BATTLE_TERRAINS); i++) {
        if (tileClass == TILE_CLASS_BATTLE_TERRAINS[i][0]) {
            return TILE_CLASS_BATTLE_TERRAINS[i][1];
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
    if (((flags & TILE_FLAG_WATER) && !MapTile_BlocksCollision(frontTileType)) || MapTile_IsSurfEdge(tileClass)) {
        return TRUE;
    }
    return FALSE;
}
