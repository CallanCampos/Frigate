// The tile attributes of the rail maps: a tilemap per rail line, from archive 78, and the tile classes of the rails'
// special tiles. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/arc.h"
#include "field/field_map.h"
#include "field/field_rail.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"

#define RAIL_ATTR_NONE 0xffffffff

// A rail line's tiles, across it and along it
typedef struct {
    u16 width;
    u16 length;
    u32 tiles[];
} RailTilemap;

struct RailAttr {
    u8 *data;
    u32 size;
    u32 count;
    RailTilemap **tilemaps;
};

RailAttr *AllocateRailAttrBlock(u32 heapId) {
    return GFL_HeapAllocate(heapId, sizeof(RailAttr), TRUE, "rail_attr.c", 92);
}

void FieldRailTilemap_Delete(RailAttr *attr) {
    FieldRailTilemap_Release(attr);
    GFL_HeapFree(attr);
}

void FieldRailTilemap_Load(RailAttr *attr, u32 fileId, u32 heapId) {
    u32 i = 0;
    u32 offset;

    attr->data = GFL_ArcSysReadHeapNewLZGetLen(ARCID_RAIL_HEADERS, fileId, FALSE, heapId, &attr->size);
    attr->count = *(u32 *)attr->data;
    attr->tilemaps = GFL_HeapAllocate(heapId, attr->count * sizeof(RailTilemap *), TRUE, "rail_attr.c", 134);
    offset = sizeof(u32);
    for (; i < attr->count; i++) {
        attr->tilemaps[i] = (RailTilemap *)(attr->data + offset);
        offset += attr->tilemaps[i]->width * attr->tilemaps[i]->length * sizeof(u32) + sizeof(RailTilemap);
    }
}

void FieldRailTilemap_Release(RailAttr *attr) {
    if (attr->data != NULL) {
        GFL_HeapFree(attr->tilemaps);
        GFL_HeapFree(attr->data);
        attr->count = 0;
        attr->tilemaps = NULL;
        attr->data = NULL;
    }
}

// The attributes of the tile at a position on a line, or RAIL_ATTR_NONE
u32 FieldRailTilemap_GetTileAtPos(RailAttr *attr, const RailPosition *pos) {
    u32 tile = RAIL_ATTR_NONE;

    if (pos->componentIsLine == TRUE && attr->count > pos->componentId) {
        RailTilemap *tilemap = attr->tilemaps[pos->componentId];
        int x = pos->posSide + tilemap->width / 2;
        int z = pos->posFront;

        if (tilemap->width > x && tilemap->length > z && x >= 0 && z >= 0) {
            tile = tilemap->tiles[x + tilemap->width * z];
        }
    }
    return tile;
}

BOOL FieldRailTilemap_IsSignBoard(u16 tileClass) {
    if (tileClass == TILE_CLASS_SLIP_DOWN_EDGE || tileClass == TILE_CLASS_SLIP_DOWN_EDGE_A1) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsJumpStart(u32 tileClass) {
    if (tileClass == TILE_CLASS_GRID_RAIL_SWITCH || tileClass == TILE_CLASS_GRID_RAIL_SWITCH_28) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsForceMoveUp(u32 tileClass) {
    if (tileClass == TILE_CLASS_FORCE_MOVE_UP) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsForceMoveDown(u32 tileClass) {
    if (tileClass == TILE_CLASS_FORCE_MOVE_DOWN) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsForceMoveLeft(u32 tileClass) {
    if (tileClass == TILE_CLASS_FORCE_MOVE_LEFT) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsForceMoveRight(u32 tileClass) {
    if (tileClass == TILE_CLASS_FORCE_MOVE_RIGHT) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsForceMoveStop(u32 tileClass) {
    if (tileClass == TILE_CLASS_FORCE_MOVE_STOP) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldRailTilemap_IsSlipDown(u32 tileClass) {
    if (tileClass == TILE_CLASS_SLIP_DOWN) {
        return TRUE;
    }
    return FALSE;
}
