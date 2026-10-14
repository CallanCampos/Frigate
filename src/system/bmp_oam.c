#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/bmp_oam.h"

// Bitmaps shown as OAM sprites. The bitmap is cut into cells of 32x16 pixels, one cell actor each, and its pixels are
// uploaded to the actors' characters. Our names; swan has none for this file

// The size of a cell's characters, 32x16 pixels at 4 bits
#define CELL_CHAR_SIZE 0x100
// The size of a cell's row of 8 pixels
#define CELL_ROW_SIZE 0x80

#define CELL_ANIM_NONE 0xffffffff

// The OBJ character mappings that the cells come in
enum {
    CELL_MAPPING_1D_128K,
    CELL_MAPPING_1D_64K,
    CELL_MAPPING_1D_32K,
    CELL_MAPPING_COUNT,
};

struct BmpOamSys {
    ClActUnit *unit;
    HeapID heapId;
    ArcTool *arc;
    u32 cellAnims[CELL_MAPPING_COUNT];
};

#define BMP_OAM_CELL_MAX 24

struct BmpOamActor {
    GFLBitmap *bitmap;
    ClActor *actors[BMP_OAM_CELL_MAX];
    u32 vramType;
    u32 chars[BMP_OAM_CELL_MAX];
    u16 surface;
    // In cells
    u8 width;
    u8 height;
};

typedef struct {
    u32 charSize;
    u32 cellFileId;
    u32 animFileId;
} BmpOamCellRes;

static u32 BmpOam_LoadCellAnim(BmpOamSys *sys, u32 vramType, ArcTool *arc);
static void BmpOam_FreeCellAnims(BmpOamSys *sys);

static const BmpOamCellRes sCellRes[CELL_MAPPING_COUNT] = {
    [CELL_MAPPING_1D_128K] = { CELL_CHAR_SIZE, 0, 1 },
    [CELL_MAPPING_1D_64K] = { CELL_CHAR_SIZE, 3, 4 },
    [CELL_MAPPING_1D_32K] = { CELL_CHAR_SIZE, 6, 7 },
};

BmpOamSys *BmpOam_Init(HeapID heapId, ClActUnit *unit) {
    int i;
    BmpOamSys *sys = GFL_HeapAllocate(heapId, sizeof(BmpOamSys), TRUE, "bmp_oam.c", 115);

    sys->heapId = heapId;
    sys->unit = unit;
    for (i = 0; i < CELL_MAPPING_COUNT; i++) {
        sys->cellAnims[i] = CELL_ANIM_NONE;
    }
    sys->arc = GFL_ArcSysCreateFileHandle(ARCID_BMP_OAM, HEAPID_TAIL(heapId));
    return sys;
}

void BmpOam_Exit(BmpOamSys *sys) {
    GFL_ArcToolFree(sys->arc);
    BmpOam_FreeCellAnims(sys);
    GFL_HeapFree(sys);
}

BmpOamActor *BmpOam_ActorAdd(BmpOamSys *sys, const BmpOamActorSetup *setup) {
    ClActorSetup clSetup = { 0 };
    BmpOamActor *actor = GFL_HeapAllocate(sys->heapId, sizeof(BmpOamActor), TRUE, "bmp_oam.c", 163);
    u8 width, height;
    u8 x, y;
    ArcTool *arc;
    u32 mapping;

    actor->bitmap = setup->bitmap;
    width = GFL_BitmapGetWidth(setup->bitmap) / 32;
    height = GFL_BitmapGetHeight(setup->bitmap) / 16;
    if (GFL_BitmapGetWidth(setup->bitmap) % 32 != 0) {
        width++;
    }
    if (GFL_BitmapGetHeight(setup->bitmap) % 16 != 0) {
        height++;
    }
    actor->width = width;
    actor->height = height;
    actor->surface = setup->surface;
    actor->vramType = setup->vramType;

    arc = sys->arc;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            mapping = BmpOam_LoadCellAnim(sys, setup->vramType, arc);
            actor->chars[y * width + x] = func_0204b8bc(sCellRes[mapping].charSize, setup->vramType, sys->heapId);
            clSetup.x = setup->x + x * 32;
            clSetup.y = setup->y + y * 16;
            clSetup.priority = setup->priority;
            clSetup.bgPriority = setup->bgPriority;
            actor->actors[y * width + x] =
                func_0204c040(sys->unit, actor->chars[y * width + x], setup->palette, sys->cellAnims[mapping], &clSetup,
                              setup->surface, sys->heapId);
            func_0204c378(actor->actors[y * width + x], (u8)setup->paletteOffset, 0);
            func_0204c5c8(actor->actors[y * width + x], FALSE);
        }
    }
    return actor;
}

void BmpOam_ActorDel(BmpOamActor *actor) {
    u8 x, y;

    for (y = 0; y < actor->height; y++) {
        for (x = 0; x < actor->width; x++) {
            func_0204b98c(actor->chars[y * actor->width + x]);
            func_0204c108(actor->actors[y * actor->width + x]);
        }
    }
    GFL_HeapFree(actor);
}

void BmpOam_ActorSetDrawEnable(BmpOamActor *actor, BOOL enable) {
    u8 x, y;

    for (y = 0; y < actor->height; y++) {
        for (x = 0; x < actor->width; x++) {
            func_0204c124(actor->actors[y * actor->width + x], enable);
        }
    }
}

BOOL BmpOam_ActorGetDrawEnable(BmpOamActor *actor) {
    return func_0204c138(actor->actors[0]);
}

void BmpOam_ActorBmpTrans(BmpOamActor *actor) {
    u8 *pixels = GFL_BitmapGetPixelData(actor->bitmap);
    void (*upload)(const void *src, u32 offset, u32 size);
    int x;
    u32 tilesHigh;
    u32 vramOffset;
    int y;
    // Where the cell's upper and lower rows of 8 pixels are in the bitmap
    int offset;
    int lowerOffset;
    u32 tilesWide;
    u32 size;
    // The size of a row of the last cell of a row, which may be narrower
    u32 lastSize;

    cp15_flushDC(pixels, GFL_BitmapCalcPixelDataSize(actor->bitmap));
    upload = actor->vramType == CLACT_VRAM_MAIN ? gfxUploadObjCharA : gfxUploadObjCharB;
    tilesWide = GFL_BitmapGetWidth(actor->bitmap) / 8;
    tilesHigh = GFL_BitmapGetHeight(actor->bitmap) / 8;
    lastSize = GFL_BitmapGetWidth(actor->bitmap) % 32 / 8 * 32;
    if (lastSize == 0) {
        lastSize = CELL_ROW_SIZE;
    }

    offset = 0;
    lowerOffset = tilesWide * 32;
    for (y = 0; y < actor->height; y++) {
        for (x = 0; x < actor->width; x++) {
            if (x == actor->width - 1) {
                size = lastSize;
            } else {
                size = CELL_ROW_SIZE;
            }
            vramOffset = func_0204bb80(actor->chars[y * actor->width + x], actor->vramType);
            upload(pixels + offset, vramOffset, size);
            // Not when the bitmap ends halfway through the last row of cells
            if (y != actor->height - 1 || (tilesHigh & 1) == 0) {
                vramOffset += CELL_ROW_SIZE;
                upload(pixels + lowerOffset, vramOffset, size);
            }
            offset += size;
            lowerOffset += size;
        }
        // Skip the row of tiles that the lower rows came from
        offset += tilesWide * 32;
        lowerOffset += tilesWide * 32;
    }
}

void BmpOam_ActorGetPos(BmpOamActor *actor, s16 *x, s16 *y) {
    ClActorPos pos;

    func_0204c178(actor->actors[0], &pos, actor->surface);
    *x = pos.x;
    *y = pos.y;
}

void BmpOam_ActorSetPos(BmpOamActor *actor, s16 x, s16 y) {
    ClActorPos pos;
    int i, j;

    for (i = 0; i < actor->height; i++) {
        for (j = 0; j < actor->width; j++) {
            pos.x = x + j * 32;
            pos.y = y + i * 16;
            func_0204c140(actor->actors[i * actor->width + j], &pos, actor->surface);
        }
    }
}

void BmpOam_ActorSetObjMode(BmpOamActor *actor, u32 mode) {
    int i, j;

    for (i = 0; i < actor->height; i++) {
        for (j = 0; j < actor->width; j++) {
            func_0204c318(actor->actors[i * actor->width + j], mode);
        }
    }
}

void BmpOam_ActorSetPriority(BmpOamActor *actor, u8 priority) {
    int i, j;

    for (i = 0; i < actor->height; i++) {
        for (j = 0; j < actor->width; j++) {
            func_0204c438(actor->actors[i * actor->width + j], priority);
        }
    }
}

void BmpOam_ActorSetBgPriority(BmpOamActor *actor, u32 bgPriority) {
    int i, j;

    for (i = 0; i < actor->height; i++) {
        for (j = 0; j < actor->width; j++) {
            func_0204c468(actor->actors[i * actor->width + j], bgPriority);
        }
    }
}

void BmpOam_ActorSetPaletteOffset(BmpOamActor *actor, u32 offset) {
    int i, j;

    for (i = 0; i < actor->height; i++) {
        for (j = 0; j < actor->width; j++) {
            func_0204c378(actor->actors[i * actor->width + j], offset, 0);
        }
    }
}

// Loads the cells and animations for the OBJ character mapping of the actor's screen, once
static u32 BmpOam_LoadCellAnim(BmpOamSys *sys, u32 vramType, ArcTool *arc) {
    u32 mapping;
    GXOBJVRamModeChar mode;

    if (vramType == CLACT_VRAM_MAIN) {
        mode = GX_GetOBJVRamModeChar();
    } else {
        mode = GXS_GetOBJVRamModeChar();
    }
    switch (mode) {
    case GX_OBJVRAMMODE_CHAR_1D_128K:
        mapping = CELL_MAPPING_1D_128K;
        break;
    case GX_OBJVRAMMODE_CHAR_1D_64K:
        mapping = CELL_MAPPING_1D_64K;
        break;
    case GX_OBJVRAMMODE_CHAR_1D_32K:
        mapping = CELL_MAPPING_1D_32K;
        break;
    default:
        mapping = CELL_MAPPING_1D_128K;
        break;
    }
    if (sys->cellAnims[mapping] != CELL_ANIM_NONE) {
        return mapping;
    }
    sys->cellAnims[mapping] =
        func_0204bde0(arc, sCellRes[mapping].cellFileId, sCellRes[mapping].animFileId, sys->heapId);
    return mapping;
}

static void BmpOam_FreeCellAnims(BmpOamSys *sys) {
    int i;

    for (i = 0; i < CELL_MAPPING_COUNT; i++) {
        if (sys->cellAnims[i] != CELL_ANIM_NONE) {
            func_0204be64(sys->cellAnims[i]);
            sys->cellAnims[i] = CELL_ANIM_NONE;
        }
    }
}
