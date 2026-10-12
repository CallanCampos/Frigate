#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zknsearch_main.h"
#include "app/zknsearch_ui.h"
#include "gfl/bg_sys.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/blink_palanm.h"
#include "system/cursor_move.h"

// The keys and touches of the Pokédex's search screen: the main page's cursor and the buttons around a list. The ROM
// doesn't name this file; zknsearch_ui.c is a guess after zknsearch_main.c

// How long the search's result stays without a key or a touch
#define RESULT_FRAMES 90

static void ZknSearchUI_MoveCursor(ZknSearchWork *wk, u32 pos);
static void ZknSearchUI_CursorOn(void *work, int pos, int prevPos);
static void ZknSearchUI_CursorOff(void *work, int pos, int prevPos);
static void ZknSearchUI_CursorMove(void *work, int pos, int prevPos);
static void ZknSearchUI_CursorTouch(void *work, int pos, int prevPos);

static const CursorMoveCallbacks sZknSearchCursorCallbacks = {
    ZknSearchUI_CursorOn,
    ZknSearchUI_CursorOff,
    ZknSearchUI_CursorMove,
    ZknSearchUI_CursorTouch,
};

// The return button and the rows of the shapes' list
static const TouchRect sZknSearchShapeListTouch[] = {
    { 168, 191, 232, 255 }, { 0, 39, 128, 143 },    { 40, 79, 128, 143 },
    { 80, 119, 128, 143 },  { 120, 159, 128, 143 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The return button and the rows of the other lists
static const TouchRect sZknSearchListTouch[] = {
    { 168, 191, 232, 255 }, { 0, 23, 128, 143 },    { 24, 47, 128, 143 },
    { 48, 71, 128, 143 },   { 72, 95, 128, 143 },   { 96, 119, 128, 143 },
    { 120, 143, 128, 143 }, { 144, 167, 128, 143 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const CursorMoveData sZknSearchCursorData[] = {
    { 16,
      0,
      16,
      3,
      ZKNSEARCH_UI_START,
      ZKNSEARCH_UI_INITIAL,
      ZKNSEARCH_UI_ORDER,
      ZKNSEARCH_UI_ORDER,
      { 0, 23, 128, 255 } },
    { 16,
      3,
      16,
      3,
      ZKNSEARCH_UI_ORDER,
      ZKNSEARCH_UI_TYPE,
      ZKNSEARCH_UI_INITIAL,
      ZKNSEARCH_UI_INITIAL,
      { 24, 47, 128, 255 } },
    { 16,
      6,
      16,
      3,
      ZKNSEARCH_UI_INITIAL,
      ZKNSEARCH_UI_COLOR,
      ZKNSEARCH_UI_TYPE,
      ZKNSEARCH_UI_TYPE,
      { 48, 71, 128, 255 } },
    { 16,
      9,
      16,
      3,
      ZKNSEARCH_UI_TYPE,
      ZKNSEARCH_UI_SHAPE,
      ZKNSEARCH_UI_COLOR,
      ZKNSEARCH_UI_COLOR,
      { 72, 95, 128, 255 } },
    { 16,
      12,
      16,
      5,
      ZKNSEARCH_UI_COLOR,
      ZKNSEARCH_UI_START,
      ZKNSEARCH_UI_SHAPE,
      ZKNSEARCH_UI_SHAPE,
      { 96, 135, 128, 255 } },
    { 14,
      18,
      18,
      3,
      ZKNSEARCH_UI_SHAPE,
      ZKNSEARCH_UI_ORDER,
      ZKNSEARCH_UI_RESET,
      ZKNSEARCH_UI_RESET,
      { 144, 167, 113, 255 } },
    { 0,
      18,
      7,
      3,
      ZKNSEARCH_UI_SHAPE,
      ZKNSEARCH_UI_ORDER,
      ZKNSEARCH_UI_START,
      ZKNSEARCH_UI_START,
      { 144, 167, 0, 50 } },
    { 0,
      0,
      0,
      0,
      ZKNSEARCH_UI_SHORTCUT,
      ZKNSEARCH_UI_SHORTCUT,
      ZKNSEARCH_UI_SHORTCUT,
      ZKNSEARCH_UI_SHORTCUT,
      { 168, 191, 184, 207 } },
    { 0, 0, 0, 0, ZKNSEARCH_UI_EXIT, ZKNSEARCH_UI_EXIT, ZKNSEARCH_UI_EXIT, ZKNSEARCH_UI_EXIT, { 168, 191, 208, 231 } },
    { 0,
      0,
      0,
      0,
      ZKNSEARCH_UI_RETURN,
      ZKNSEARCH_UI_RETURN,
      ZKNSEARCH_UI_RETURN,
      ZKNSEARCH_UI_RETURN,
      { 168, 191, 232, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

void ZknSearchUI_InitCursor(ZknSearchWork *wk, u32 pos) {
    if (func_0203d554() == TRUE) {
        wk->cursorMove =
            CursorMove_Create(sZknSearchCursorData, &sZknSearchCursorCallbacks, wk, FALSE, pos, HEAPID_ZUKAN_SEARCH);
    } else {
        wk->cursorMove =
            CursorMove_Create(sZknSearchCursorData, &sZknSearchCursorCallbacks, wk, TRUE, pos, HEAPID_ZUKAN_SEARCH);
        ZknSearchUI_SetCursorPalette(wk, pos, ZKNSEARCH_UI_PALETTE_ON);
    }
    CursorMove_SetHideOnTouch(wk->cursorMove);
}

void ZknSearchUI_ExitCursor(ZknSearchWork *wk) {
    if (wk->cursorMove != NULL) {
        CursorMove_Delete(wk->cursorMove);
        wk->cursorMove = NULL;
    }
}

// Sets the palette of a position's frame: the criteria and the two buttons under them
void ZknSearchUI_SetCursorPalette(ZknSearchWork *wk, u32 pos, u32 palette) {
    if (pos <= ZKNSEARCH_UI_RESET) {
        const CursorMoveData *data = CursorMove_GetData(wk->cursorMove, pos);

        GFL_BGSysSetScrPaletteNo(0, data->x, data->y, data->width, data->height, palette);
        GFL_BGSysQueueScrLoad(0);
    }
}

u32 ZknSearchUI_Main(ZknSearchWork *wk) {
    u32 ret = CursorMove_Update(wk->cursorMove);

    if (ret != CURSOR_MOVE_NONE) {
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        func_0203d564(FALSE);
        return ZKNSEARCH_UI_EXIT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
        if (func_0203d554() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        func_0203d564(FALSE);
        return ZKNSEARCH_UI_SHORTCUT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (func_0203d554() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        func_0203d564(FALSE);
        ZknSearchUI_MoveCursor(wk, ZKNSEARCH_UI_START);
        return ZKNSEARCH_UI_START;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (func_0203d554() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        func_0203d564(FALSE);
        ZknSearchUI_MoveCursor(wk, ZKNSEARCH_UI_RESET);
        return ZKNSEARCH_UI_RESET;
    }
    // Any other key only shows the cursor again
    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0203d564(FALSE);
        CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        return CURSOR_MOVE_CURSOR_ON;
    }
    return ret;
}

static void ZknSearchUI_MoveCursor(ZknSearchWork *wk, u32 pos) {
    ZknSearchUI_SetCursorPalette(wk, CursorMove_GetPos(wk->cursorMove), ZKNSEARCH_UI_PALETTE_OFF);
    ZknSearchUI_SetCursorPalette(wk, pos, ZKNSEARCH_UI_PALETTE_ON);
    CursorMove_SetPos(wk->cursorMove, pos);
}

static void ZknSearchUI_CursorOn(void *work, int pos, int prevPos) {
    ZknSearchWork *wk = work;
    u32 keys = GCTX_HIDGetPressedKeys();

    // B alone only shows the cursor
    if (!(keys & PAD_BUTTON_B) || (keys & (PAD_BUTTON_A | PAD_KEY_RIGHT | PAD_KEY_LEFT | PAD_KEY_UP | PAD_KEY_DOWN))) {
        ZknSearchUI_SetCursorPalette(wk, pos, ZKNSEARCH_UI_PALETTE_ON);
        BlinkPalAnm_InitAnime(wk->blinkAnm);
    }
}

static void ZknSearchUI_CursorOff(void *work, int pos, int prevPos) {
    ZknSearchWork *wk = work;

    ZknSearchUI_SetCursorPalette(wk, prevPos, ZKNSEARCH_UI_PALETTE_OFF);
    ZknSearchUI_SetCursorPalette(wk, pos, ZKNSEARCH_UI_PALETTE_OFF);
}

static void ZknSearchUI_CursorMove(void *work, int pos, int prevPos) {
    ZknSearchWork *wk = work;

    ZknSearchUI_SetCursorPalette(wk, prevPos, ZKNSEARCH_UI_PALETTE_OFF);
    ZknSearchUI_SetCursorPalette(wk, pos, ZKNSEARCH_UI_PALETTE_ON);
    BlinkPalAnm_InitAnime(wk->blinkAnm);
}

// The shortcut button doesn't take the cursor
static void ZknSearchUI_CursorTouch(void *work, int pos, int prevPos) {
    ZknSearchWork *wk = work;

    ZknSearchUI_SetCursorPalette(wk, prevPos, ZKNSEARCH_UI_PALETTE_OFF);
    if (pos == ZKNSEARCH_UI_SHORTCUT) {
        CursorMove_SetPos(wk->cursorMove, prevPos);
    }
}

BOOL ZknSearchUI_WaitResult(ZknSearchWork *wk) {
    if (wk->timer == RESULT_FRAMES) {
        wk->timer = 0;
        return TRUE;
    }
    wk->timer++;
    if (func_0203da48()) {
        func_0203d564(TRUE);
        wk->timer = 0;
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        func_0203d564(FALSE);
        wk->timer = 0;
        return TRUE;
    }
    return FALSE;
}

int ZknSearchUI_ListMain(ZknSearchWork *wk) {
    int ret = func_0203da0c(sZknSearchListTouch);

    if (ret != -1) {
        func_0203d564(TRUE);
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        return ZKNSEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        func_0203d564(FALSE);
        return ZKNSEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (func_0203d554() == TRUE) {
            func_0203d564(FALSE);
            FrameList_ShowCursor(wk->frameList);
        }
        return ZKNSEARCH_UI_LIST_RESET;
    }
    return ret;
}

int ZknSearchUI_ShapeListMain(ZknSearchWork *wk) {
    int ret = func_0203da0c(sZknSearchShapeListTouch);

    if (ret != -1) {
        func_0203d564(TRUE);
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        return ZKNSEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        func_0203d564(FALSE);
        return ZKNSEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (func_0203d554() == TRUE) {
            func_0203d564(FALSE);
            FrameList_ShowCursor(wk->frameList);
        }
        return ZKNSEARCH_UI_LIST_RESET;
    }
    return ret;
}
