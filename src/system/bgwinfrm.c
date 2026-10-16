#include "system/bgwinfrm.h"
#include "types.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// Frames of BG screen data that are put on a BG, cleared and slid across it. The file name is the ROM's (its heap
// allocations' assert text); names are ours

// The default area a frame shows in: the whole screen, in tiles
#define SCREEN_WIDTH_TILES 32
#define SCREEN_HEIGHT_TILES 24

typedef struct {
    u16 *screen;
    // In tiles
    u16 width : 6;
    u16 height : 6;
    // Not on the BG: set up, or cleared
    u16 hidden : 1;
    u16 moving : 3;
    s8 x;
    s8 y;
    s8 moveX;
    s8 moveY;
    u8 bg;
    u8 moveCount;
    // The area of the BG it shows in
    s8 left;
    s8 right;
    s8 top;
    s8 bottom;
} BGWinFrameEntry;

struct BGWinFrame {
    BGWinFrameEntry *entries;
    u16 count;
    u16 transferMode : 15;
    // Set while any frame moves
    u16 moving : 1;
    u16 heapId;
};

static void BGWinFrame_NoTransfer(u8 bg);

static void (*const sBGWinFrameTransferFuncs[])(u8 bg) = {
    [BGWINFRAME_TRANSFER_NONE] = BGWinFrame_NoTransfer,
    [BGWINFRAME_TRANSFER_NOW] = GFL_BGSysLoadScr,
    [BGWINFRAME_TRANSFER_VBLANK] = GFL_BGSysQueueScrLoad,
};

BGWinFrame *BGWinFrame_Create(u32 transferMode, u32 count, HeapID heapId) {
    BGWinFrame *frames = GFL_HeapAllocate(heapId, sizeof(BGWinFrame), FALSE, "bgwinfrm.c", 86);

    frames->count = count;
    frames->heapId = heapId;
    frames->transferMode = transferMode;
    frames->moving = FALSE;
    frames->entries = GFL_HeapAllocate(heapId, count * sizeof(BGWinFrameEntry), TRUE, "bgwinfrm.c", 93);
    return frames;
}

void BGWinFrame_Delete(BGWinFrame *frames) {
    u32 i;

    for (i = 0; i < frames->count; i++) {
        if (frames->entries[i].screen != NULL) {
            GFL_HeapFree(frames->entries[i].screen);
        }
    }
    GFL_HeapFree(frames->entries);
    GFL_HeapFree(frames);
}

void BGWinFrame_InitFrame(BGWinFrame *frames, u32 index, u32 bg, u32 width, u32 height) {
    BGWinFrameEntry *frame = &frames->entries[index];

    frame->screen = GFL_HeapAllocate(frames->heapId, width * height * sizeof(u16), TRUE, "bgwinfrm.c", 138);
    frame->width = width;
    frame->height = height;
    frame->bg = bg;
    frame->x = 0;
    frame->y = 0;
    frame->moving = 0;
    frame->hidden = TRUE;
    frame->left = 0;
    frame->right = SCREEN_WIDTH_TILES;
    frame->top = 0;
    frame->bottom = SCREEN_HEIGHT_TILES;
}

void BGWinFrame_SetScreen(BGWinFrame *frames, u32 index, u16 *screen) {
    BGWinFrameEntry *frame = &frames->entries[index];

    MIi_CpuCopy16(screen, frame->screen, frame->width * frame->height * sizeof(u16));
}

void BGWinFrame_LoadScreen(BGWinFrame *frames, u32 index, u32 arcId, u32 fileId, BOOL compressed) {
    NNSG2dScreenData *screen;
    void *file = GFL_G2DIOReadNSCR(arcId, fileId, compressed, &screen, frames->heapId);

    BGWinFrame_SetScreen(frames, index, (u16 *)screen->rawData);
    GFL_HeapFree(file);
}

void BGWinFrame_LoadScreenArc(BGWinFrame *frames, u32 index, ArcTool *arc, u32 fileId, BOOL compressed) {
    NNSG2dScreenData *screen;
    void *file = GFL_G2DIOReadNSCRArc(arc, fileId, compressed, &screen, frames->heapId);

    BGWinFrame_SetScreen(frames, index, (u16 *)screen->rawData);
    GFL_HeapFree(file);
}

void BGWinFrame_Put(BGWinFrame *frames, u32 index, s8 x, s8 y) {
    BGWinFrameEntry *frame = &frames->entries[index];
    u8 putX, putY, sizeX, sizeY, srcX, srcY;

    frame->x = x;
    frame->y = y;
    if (x >= frame->right || y >= frame->bottom || x + frame->width < frame->left || y + frame->height < frame->top) {
        return;
    }

    putX = x;
    sizeX = frame->width;
    srcX = 0;
    if (x < frame->left) {
        putX = frame->left;
        srcX = frame->left - x;
        sizeX -= srcX;
    }
    if (x + frame->width >= frame->right) {
        sizeX -= x + frame->width - frame->right;
    }

    putY = y;
    sizeY = frame->height;
    srcY = 0;
    if (y < frame->top) {
        putY = frame->top;
        srcY = frame->top - y;
        sizeY -= srcY;
    }
    if (y + frame->height >= frame->bottom) {
        sizeY -= y + frame->height - frame->bottom;
    }

    GFL_BGSysLoadScrArea(frame->bg, putX, putY, sizeX, sizeY, frame->screen, srcX, srcY, frame->width, frame->height);
    sBGWinFrameTransferFuncs[frames->transferMode](frame->bg);
    frame->hidden = FALSE;
}

void BGWinFrame_Show(BGWinFrame *frames, u32 index) {
    BGWinFrame_Put(frames, index, frames->entries[index].x, frames->entries[index].y);
}

void BGWinFrame_Hide(BGWinFrame *frames, u32 index) {
    BGWinFrameEntry *frame = &frames->entries[index];
    s8 x, y, sizeX, sizeY;

    frame->hidden = TRUE;
    if (frame->x >= frame->right || frame->y >= frame->bottom || frame->x + frame->width < frame->left ||
        frame->y + frame->height < frame->top) {
        return;
    }

    x = frame->x;
    sizeX = frame->width;
    if (frame->x < frame->left) {
        x = frame->left;
        sizeX -= frame->left - frame->x;
    }
    if (frame->x + frame->width >= frame->right) {
        sizeX -= frame->x + frame->width - frame->right;
    }

    y = frame->y;
    sizeY = frame->height;
    if (frame->y < frame->top) {
        y = frame->top;
        sizeY -= frame->top - frame->y;
    }
    if (frame->y + frame->height >= frame->bottom) {
        sizeY -= frame->y + frame->height - frame->bottom;
    }

    GFL_BGSysFillScrArea(frame->bg, 0, x, y, sizeX, sizeY, BGSYS_FILL_KEEP_PALETTE);
    sBGWinFrameTransferFuncs[frames->transferMode](frame->bg);
}

void BGWinFrame_StartMove(BGWinFrame *frames, u32 index, s8 moveX, s8 moveY, u8 count) {
    BGWinFrameEntry *frame = &frames->entries[index];

    frame->moveX = moveX;
    frame->moveY = moveY;
    frame->moveCount = count;
    frame->moving = 1;
    frames->moving = TRUE;
}

void BGWinFrame_UpdateMoves(BGWinFrame *frames) {
    u32 i;

    if (frames->moving == FALSE) {
        return;
    }
    frames->moving = FALSE;
    for (i = 0; i < frames->count; i++) {
        if (BGWinFrame_MoveStep(frames, i) == TRUE) {
            frames->moving = TRUE;
        }
    }
}

BOOL BGWinFrame_MoveStep(BGWinFrame *frames, u32 index) {
    if (frames->entries[index].moving == 0) {
        return FALSE;
    }
    BGWinFrame_Hide(frames, index);
    frames->entries[index].x += frames->entries[index].moveX;
    frames->entries[index].y += frames->entries[index].moveY;
    BGWinFrame_Show(frames, index);
    frames->entries[index].moveCount--;
    if (frames->entries[index].moveCount == 0) {
        frames->entries[index].moving = 0;
        return FALSE;
    }
    return TRUE;
}

BOOL BGWinFrame_IsMoving(BGWinFrame *frames, u32 index) {
    return frames->entries[index].moving;
}

void BGWinFrame_SetPalette(BGWinFrame *frames, u32 index, u8 x, u8 y, u8 width, u8 height, u8 palette) {
    u16 *screen = frames->entries[index].screen;
    u32 frameWidth = frames->entries[index].width;
    u16 i, j;

    for (i = y; i < y + height; i++) {
        for (j = x; j < x + width; j++) {
            screen[i * frameWidth + j] = (screen[i * frameWidth + j] & 0xfff) | (palette << 12);
        }
    }
}

void BGWinFrame_WriteBmpWin(BGWinFrame *frames, u32 index, BmpWin *window) {
    u16 *screen = frames->entries[index].screen;
    u32 frameWidth = frames->entries[index].width;
    u16 charPos = BmpWin_GetCharPos(window);
    u16 palette = BmpWin_GetPalette(window) << 12;
    u8 sizeX = BmpWin_GetSizeX(window);
    u8 sizeY = BmpWin_GetSizeY(window);
    u8 posX = BmpWin_GetPosX(window);
    u8 posY = BmpWin_GetPosY(window);
    u16 i, j;

    for (i = 0; i < sizeY; i++) {
        if (posY + i >= frames->entries[index].height) {
            break;
        }
        for (j = 0; j < sizeX; j++) {
            if (posX + j >= frames->entries[index].width) {
                break;
            }
            screen[(posY + i) * frameWidth + posX + j] = palette + (charPos + j);
        }
        charPos += sizeX;
    }
}

u16 *BGWinFrame_GetScreen(BGWinFrame *frames, u32 index) {
    return frames->entries[index].screen;
}

u8 BGWinFrame_GetBG(BGWinFrame *frames, u32 index) {
    return frames->entries[index].bg;
}

void BGWinFrame_GetPos(BGWinFrame *frames, u32 index, s8 *x, s8 *y) {
    if (x != NULL) {
        *x = frames->entries[index].x;
    }
    if (y != NULL) {
        *y = frames->entries[index].y;
    }
}

void BGWinFrame_GetSize(BGWinFrame *frames, u32 index, u16 *width, u16 *height) {
    if (width != NULL) {
        *width = frames->entries[index].width;
    }
    if (height != NULL) {
        *height = frames->entries[index].height;
    }
}

void BGWinFrame_SetArea(BGWinFrame *frames, u32 index, s8 left, s8 right, s8 top, s8 bottom) {
    frames->entries[index].left = left;
    frames->entries[index].right = right;
    frames->entries[index].top = top;
    frames->entries[index].bottom = bottom;
}

static void BGWinFrame_NoTransfer(u8 bg) {
}
