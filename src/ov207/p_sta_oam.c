#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/bmp.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "p_status_local.h"

// Sprites of a bitmap for the summary screen's pages: the bitmap, which text is printed into, is cut into 64x32
// actors whose characters are uploaded from it. The names of the fields and functions are guesses

#define OAM_CELL_WIDTH 64
#define OAM_CELL_HEIGHT 32
#define OAM_CELL_COUNT 3
// The size of one 8-pixel line of a cell's characters, 8 tiles
#define CELL_LINE_SIZE 0x100
#define CELL_LINES (OAM_CELL_HEIGHT / 8)
#define CELL_ANIMS_NONE 0xffffffff

// The 1D OBJ character mappings, each with its own cells and animations
enum {
    CELL_MAPPING_1D_128K,
    CELL_MAPPING_1D_64K,
    CELL_MAPPING_1D_32K,
    CELL_MAPPING_COUNT,
};

struct PStaOam {
    ClActUnit *unit;
    HeapID heapId;
    u32 cellAnims[CELL_MAPPING_COUNT];
};

struct PStaOamActor {
    GFLBitmap *bitmap;
    ClActor *actors[OAM_CELL_COUNT];
    u32 vramType;
    u32 chars[OAM_CELL_COUNT];
    u16 surface;
    u8 cols;
    u8 rows;
};

static u32 PStaOam_LoadCellAnims(PStaOam *oam, u32 vramType, ArcTool *arc);
static void PStaOam_FreeCellAnims(PStaOam *oam);

PStaOam *PStaOam_Create(HeapID heapId, ClActUnit *unit) {
    PStaOam *oam = GFL_HeapAllocate(heapId, sizeof(PStaOam), TRUE, "p_sta_oam.c", 93);
    int i;

    oam->heapId = heapId;
    oam->unit = unit;
    for (i = 0; i < CELL_MAPPING_COUNT; i++) {
        oam->cellAnims[i] = CELL_ANIMS_NONE;
    }
    return oam;
}

void PStaOam_Free(PStaOam *oam) {
    PStaOam_FreeCellAnims(oam);
    GFL_HeapFree(oam);
}

PStaOamActor *PStaOam_CreateActor(PStaOam *oam, const PStaOamSetup *setup) {
    ClActorSetup actorSetup = { 0, 0, 0, 0, 0 };
    PStaOamActor *actor = GFL_HeapAllocate(oam->heapId, sizeof(PStaOamActor), TRUE, "p_sta_oam.c", 138);
    u8 cols;
    u8 rows;
    u8 row;
    ArcTool *arc;
    u8 col;
    u32 mapping;

    actor->bitmap = setup->bitmap;
    cols = GFL_BitmapGetWidth(setup->bitmap) / OAM_CELL_WIDTH;
    rows = GFL_BitmapGetHeight(setup->bitmap) / OAM_CELL_HEIGHT;
    if (GFL_BitmapGetWidth(setup->bitmap) % OAM_CELL_WIDTH != 0) {
        cols++;
    }
    if (GFL_BitmapGetHeight(setup->bitmap) % OAM_CELL_HEIGHT != 0) {
        rows++;
    }
    actor->cols = cols;
    actor->rows = rows;
    actor->surface = setup->surface;
    actor->vramType = setup->vramType;
    arc = GFL_ArcSysCreateFileHandle(ARCID_P_STATUS, oam->heapId);
    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            mapping = PStaOam_LoadCellAnims(oam, setup->vramType, arc);
            actor->chars[row * cols + col] = func_0204b81c(arc, 9, FALSE, setup->vramType, oam->heapId);
            actorSetup.x = setup->x + col * OAM_CELL_WIDTH;
            actorSetup.y = setup->y + row * OAM_CELL_HEIGHT;
            actorSetup.priority = setup->priority;
            actorSetup.bgPriority = setup->bgPriority;
            actor->actors[row * cols + col] =
                func_0204c040(oam->unit, actor->chars[row * cols + col], setup->palette, oam->cellAnims[mapping],
                              &actorSetup, setup->surface, oam->heapId);
            // The palette is set on the actor of the column in the first row, whatever the row. Every bitmap of the
            // summary screen is one row tall, so it is the same actor, and a second row would overflow the arrays
            func_0204c378(actor->actors[col], (u8)setup->paletteOffset, 0);
        }
    }
    GFL_ArcToolFree(arc);
    return actor;
}

void PStaOam_FreeActor(PStaOamActor *actor) {
    u8 row;
    u8 col;

    for (row = 0; row < actor->rows; row++) {
        for (col = 0; col < actor->cols; col++) {
            func_0204b98c(actor->chars[row * actor->cols + col]);
            func_0204c108(actor->actors[row * actor->cols + col]);
        }
    }
    GFL_HeapFree(actor);
}

void PStaOam_SetVisible(PStaOamActor *actor, BOOL visible) {
    u8 row;
    u8 col;

    for (row = 0; row < actor->rows; row++) {
        for (col = 0; col < actor->cols; col++) {
            func_0204c124(actor->actors[row * actor->cols + col], visible);
        }
    }
}

void PStaOam_Upload(PStaOamActor *actor) {
    u8 *pixels = GFL_BitmapGetPixelData(actor->bitmap);
    int col;
    int row;
    u32 offset;
    u32 size;
    u32 lastSize;
    int tileCols;
    int lastLines;
    u32 dest;
    u8 line;
    void (*upload)(const void *src, u32 offset, u32 size);

    DC_FlushRange(pixels, GFL_BitmapCalcPixelDataSize(actor->bitmap));
    if (actor->vramType == CLACT_VRAM_MAIN) {
        upload = GX_LoadOBJ;
    } else {
        upload = GXS_LoadOBJ;
    }
    tileCols = GFL_BitmapGetWidth(actor->bitmap) / 8;
    lastLines = GFL_BitmapGetHeight(actor->bitmap) / 8 % 4;
    lastSize = GFL_BitmapGetWidth(actor->bitmap) % OAM_CELL_WIDTH / 8 * 32;
    if (lastSize == 0) {
        lastSize = CELL_LINE_SIZE;
    }
    offset = 0;
    for (row = 0; row < actor->rows; row++) {
        for (col = 0; col < actor->cols; col++) {
            if (col == actor->cols - 1) {
                size = lastSize;
            } else {
                size = CELL_LINE_SIZE;
            }
            dest = func_0204bb80(actor->chars[row * actor->cols + col], actor->vramType);
            for (line = 0; line < CELL_LINES; line++) {
                if (row != actor->rows - 1 || lastLines == 0 || line < lastLines) {
                    upload(pixels + (offset + line * (tileCols * 32)), dest, size);
                    dest += CELL_LINE_SIZE;
                }
            }
            offset += size;
        }
        offset += tileCols * 32;
    }
}

void PStaOam_SetPosition(PStaOamActor *actor, s16 x, s16 y) {
    int row;
    int col;
    ClActorPos pos;

    for (row = 0; row < actor->rows; row++) {
        for (col = 0; col < actor->cols; col++) {
            pos.x = x + col * OAM_CELL_WIDTH;
            pos.y = y + row * OAM_CELL_HEIGHT;
            func_0204c140(actor->actors[row * actor->cols + col], &pos, actor->surface);
        }
    }
}

void PStaOam_SwapBitmaps(PStaOamActor *a, PStaOamActor *b) {
    GFLBitmap *bitmap = a->bitmap;

    a->bitmap = b->bitmap;
    b->bitmap = bitmap;
}

static u32 PStaOam_LoadCellAnims(PStaOam *oam, u32 vramType, ArcTool *arc) {
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
    if (oam->cellAnims[mapping] != CELL_ANIMS_NONE) {
        return mapping;
    }
    oam->cellAnims[mapping] = func_0204bde0(arc, 79, 130, oam->heapId);
    return mapping;
}

static void PStaOam_FreeCellAnims(PStaOam *oam) {
    int i;

    for (i = 0; i < CELL_MAPPING_COUNT; i++) {
        if (oam->cellAnims[i] != CELL_ANIMS_NONE) {
            func_0204be64(oam->cellAnims[i]);
            oam->cellAnims[i] = CELL_ANIMS_NONE;
        }
    }
}
