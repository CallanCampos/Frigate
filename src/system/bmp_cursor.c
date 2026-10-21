#include "types.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "system/bmp_cursor.h"
#include "system/printsys.h"

// The cursor menus draw before their selected option: an 8x8 bitmap, or a string, which nothing in the ROM sets

#define BMPCURSOR_BITMAP 0
#define BMPCURSOR_STRING 1

struct BmpCursor {
    u32 type;
    StrBuf *string;
    GFLBitmap *bitmap;
};

// The cursor's 16-color pixels, a triangle pointing right
static const u8 pixelData[] = {
    0xff, 0xff, 0xff, 0xff, 0x1f, 0x11, 0xff, 0xff, 0x1f, 0x11, 0x11, 0xff, 0x1f, 0x11, 0x11, 0xf1,
    0x1f, 0x11, 0x11, 0xf1, 0x1f, 0x11, 0x11, 0xf2, 0x1f, 0x11, 0x22, 0xff, 0x2f, 0x22, 0xff, 0xff,
};

BmpCursor *BmpCursor_Create(u32 heapId) {
    BmpCursor *cursor = GFL_HeapAllocate(heapId, sizeof(BmpCursor), TRUE, "bmp_cursor.c", 58);

    if (cursor != NULL) {
        cursor->type = BMPCURSOR_BITMAP;
    }
    return cursor;
}

void BmpCursor_Free(BmpCursor *cursor) {
    if (cursor->string != NULL) {
        GFL_StrBufFree(cursor->string);
    }
    if (cursor->bitmap != NULL) {
        GFL_BitmapFree(cursor->bitmap);
    }
    GFL_HeapFree(cursor);
}

void BmpCursor_Print(BmpCursor *cursor, u16 x, u16 y, PrintWindow *printWindow, PrintQueue *queue, Font *font) {
    if (cursor->type == BMPCURSOR_STRING) {
        PrintQueue_Print(queue, BmpWin_GetBitmap(printWindow->window), x, y, cursor->string, font);
        printWindow->flushPending = TRUE;
    } else if (cursor->bitmap != NULL) {
        GFL_BitmapCopyArea(cursor->bitmap, BmpWin_GetBitmap(printWindow->window), 0, 0, x + 2, y + 2, 8, 8, 0xf);
        BmpWin_FlushChar(printWindow->window);
    }
}

void BmpCursor_LoadBitmap(BmpCursor *cursor, u32 heapId) {
    cursor->bitmap = GFL_BitmapWrap((void *)pixelData, 1, 1, 0x20, heapId);
}
