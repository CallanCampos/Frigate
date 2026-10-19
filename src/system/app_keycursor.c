#include "types.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "system/app_keycursor.h"
#include "system/printsys.h"

// The cursor that bobs in the bottom right corner of a window while its message waits for a button

// The cursor's size and where it goes, from the window's bottom right corner
#define CURSOR_WIDTH 10
#define CURSOR_HEIGHT 7
#define CURSOR_RIGHT 11
#define CURSOR_BOTTOM 9

// The cursor moves down a pixel every 8 frames, and back up after 3
#define CURSOR_FRAME_TIME 8
#define CURSOR_FRAMES 3

struct KeyCursor {
    GFLBitmap *bitmap;
    u8 frame;
    u8 timer;
    u8 erased;
    u8 keys : 1;
    u8 touch : 1;
    u8 showWhenDone : 1;
    u16 bgColor;
};

// The cursor: a down arrow at the top left of 2 by 2 tiles of 4 bits per pixel, 0x20 bytes each
static const u8 sCursorBitmap[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x21, 0x22, 0x22,
    0x22, 0x10, 0x22, 0x22, 0x12, 0x00, 0x21, 0x22, 0x11, 0x00, 0x10, 0x12, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

KeyCursor *KeyCursor_Create(u16 bgColor, BOOL keys, BOOL touch, HeapID heapId) {
    return KeyCursor_CreateEx(bgColor, keys, touch, heapId, sCursorBitmap);
}

KeyCursor *KeyCursor_CreateEx(u16 bgColor, BOOL keys, BOOL touch, HeapID heapId, const u8 *bitmap) {
    KeyCursor *cursor = GFL_HeapAllocate(heapId, sizeof(KeyCursor), TRUE, "app_keycursor.c", 150);

    cursor->bgColor = bgColor;
    cursor->keys = keys;
    cursor->touch = touch;
    cursor->bitmap = GFL_BitmapWrap((void *)bitmap, 2, 2, 0x20, heapId);
    return cursor;
}

void KeyCursor_Free(KeyCursor *cursor) {
    GFL_BitmapFree(cursor->bitmap);
    GFL_HeapFree(cursor);
}

void KeyCursor_Erase(KeyCursor *cursor, GFLBitmap *bitmap, u16 bgColor) {
    u16 x = GFL_BitmapGetWidth(bitmap) - CURSOR_RIGHT;
    u16 y = GFL_BitmapGetHeight(bitmap) - CURSOR_BOTTOM;

    GFL_BitmapFillArea(bitmap, x, y + cursor->frame, CURSOR_WIDTH, CURSOR_HEIGHT, bgColor);
}

void KeyCursor_Draw(KeyCursor *cursor, GFLBitmap *bitmap, u16 bgColor) {
    u16 x;
    u16 y;

    KeyCursor_Erase(cursor, bitmap, bgColor);
    cursor->timer++;
    if (cursor->timer >= CURSOR_FRAME_TIME) {
        cursor->timer = 0;
        cursor->frame++;
        cursor->frame %= CURSOR_FRAMES;
    }

    x = GFL_BitmapGetWidth(bitmap) - CURSOR_RIGHT;
    y = GFL_BitmapGetHeight(bitmap) - CURSOR_BOTTOM;
    GFL_BitmapCopyArea(cursor->bitmap, bitmap, 0, 2, x, y + cursor->frame, CURSOR_WIDTH, CURSOR_HEIGHT, 0);
}

void KeyCursor_Update(KeyCursor *cursor, PrintStream *stream, BmpWin *window) {
    BOOL pressed;

    switch (PrintStream_GetState(stream)) {
    case PRINT_STREAM_DONE:
        if (cursor->showWhenDone == FALSE) {
            cursor->erased = FALSE;
            break;
        }
        // fallthrough
    case PRINT_STREAM_PAUSED:
        pressed = FALSE;
        if (cursor->keys) {
            pressed = GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B);
        }
        if (pressed == FALSE && cursor->touch) {
            pressed = func_0203da48();
        }

        if (pressed) {
            cursor->showWhenDone = FALSE;
            cursor->erased = TRUE;
            KeyCursor_Erase(cursor, BmpWin_GetBitmap(window), cursor->bgColor);
            BmpWin_FlushChar(window);
        } else if (cursor->erased == FALSE) {
            KeyCursor_Draw(cursor, BmpWin_GetBitmap(window), cursor->bgColor);
            BmpWin_FlushChar(window);
        }
        break;
    case PRINT_STREAM_RUNNING:
        cursor->erased = FALSE;
        break;
    }
}

BOOL KeyCursor_UpdateWait(KeyCursor *cursor, BmpWin *window) {
    BOOL pressed = FALSE;

    if (cursor->keys) {
        pressed = GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B);
    }
    if (pressed == FALSE && cursor->touch) {
        pressed = func_0203da48();
    }

    if (pressed) {
        KeyCursor_Erase(cursor, BmpWin_GetBitmap(window), cursor->bgColor);
    } else {
        KeyCursor_Draw(cursor, BmpWin_GetBitmap(window), cursor->bgColor);
    }
    BmpWin_FlushChar(window);
    return pressed;
}

void KeyCursor_ShowWhenDone(KeyCursor *cursor) {
    cursor->showWhenDone = TRUE;
}
