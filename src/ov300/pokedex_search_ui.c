#include "types.h"
#include "app/pokedex_search_main.h"
#include "app/pokedex_search_ui.h"
#include "app/ui/frame_list.h"
#include "gfl/bg_sys.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/blink_palanm.h"
#include "system/cursor_move.h"

// The keys and touches of the Pokédex's search screen: the main page's cursor and the buttons around a list. The ROM
// doesn't name this file; pokedex_search_ui.c is a guess after pokedex_search_main.c

// How long the search's result stays without a key or a touch
#define RESULT_FRAMES 90

static void PokedexSearchUI_MoveCursor(PokedexSearchWork *wk, u32 pos);
static void PokedexSearchUI_CursorOn(void *work, int pos, int prevPos);
static void PokedexSearchUI_CursorOff(void *work, int pos, int prevPos);
static void PokedexSearchUI_CursorMove(void *work, int pos, int prevPos);
static void PokedexSearchUI_CursorTouch(void *work, int pos, int prevPos);

static const CursorMoveCallbacks sPokedexSearchCursorCallbacks = {
    PokedexSearchUI_CursorOn,
    PokedexSearchUI_CursorOff,
    PokedexSearchUI_CursorMove,
    PokedexSearchUI_CursorTouch,
};

// The return button and the rows of the shapes' list
static const TouchRect sPokedexSearchShapeListTouch[] = {
    { 168, 191, 232, 255 }, { 0, 39, 128, 143 },    { 40, 79, 128, 143 },
    { 80, 119, 128, 143 },  { 120, 159, 128, 143 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The return button and the rows of the other lists
static const TouchRect sPokedexSearchListTouch[] = {
    { 168, 191, 232, 255 }, { 0, 23, 128, 143 },    { 24, 47, 128, 143 },
    { 48, 71, 128, 143 },   { 72, 95, 128, 143 },   { 96, 119, 128, 143 },
    { 120, 143, 128, 143 }, { 144, 167, 128, 143 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const CursorMoveData sPokedexSearchCursorData[] = {
    { 16,
      0,
      16,
      3,
      POKEDEX_SEARCH_UI_START,
      POKEDEX_SEARCH_UI_INITIAL,
      POKEDEX_SEARCH_UI_ORDER,
      POKEDEX_SEARCH_UI_ORDER,
      { 0, 23, 128, 255 } },
    { 16,
      3,
      16,
      3,
      POKEDEX_SEARCH_UI_ORDER,
      POKEDEX_SEARCH_UI_TYPE,
      POKEDEX_SEARCH_UI_INITIAL,
      POKEDEX_SEARCH_UI_INITIAL,
      { 24, 47, 128, 255 } },
    { 16,
      6,
      16,
      3,
      POKEDEX_SEARCH_UI_INITIAL,
      POKEDEX_SEARCH_UI_COLOR,
      POKEDEX_SEARCH_UI_TYPE,
      POKEDEX_SEARCH_UI_TYPE,
      { 48, 71, 128, 255 } },
    { 16,
      9,
      16,
      3,
      POKEDEX_SEARCH_UI_TYPE,
      POKEDEX_SEARCH_UI_SHAPE,
      POKEDEX_SEARCH_UI_COLOR,
      POKEDEX_SEARCH_UI_COLOR,
      { 72, 95, 128, 255 } },
    { 16,
      12,
      16,
      5,
      POKEDEX_SEARCH_UI_COLOR,
      POKEDEX_SEARCH_UI_START,
      POKEDEX_SEARCH_UI_SHAPE,
      POKEDEX_SEARCH_UI_SHAPE,
      { 96, 135, 128, 255 } },
    { 14,
      18,
      18,
      3,
      POKEDEX_SEARCH_UI_SHAPE,
      POKEDEX_SEARCH_UI_ORDER,
      POKEDEX_SEARCH_UI_RESET,
      POKEDEX_SEARCH_UI_RESET,
      { 144, 167, 113, 255 } },
    { 0,
      18,
      7,
      3,
      POKEDEX_SEARCH_UI_SHAPE,
      POKEDEX_SEARCH_UI_ORDER,
      POKEDEX_SEARCH_UI_START,
      POKEDEX_SEARCH_UI_START,
      { 144, 167, 0, 50 } },
    { 0,
      0,
      0,
      0,
      POKEDEX_SEARCH_UI_SHORTCUT,
      POKEDEX_SEARCH_UI_SHORTCUT,
      POKEDEX_SEARCH_UI_SHORTCUT,
      POKEDEX_SEARCH_UI_SHORTCUT,
      { 168, 191, 184, 207 } },
    { 0,
      0,
      0,
      0,
      POKEDEX_SEARCH_UI_EXIT,
      POKEDEX_SEARCH_UI_EXIT,
      POKEDEX_SEARCH_UI_EXIT,
      POKEDEX_SEARCH_UI_EXIT,
      { 168, 191, 208, 231 } },
    { 0,
      0,
      0,
      0,
      POKEDEX_SEARCH_UI_RETURN,
      POKEDEX_SEARCH_UI_RETURN,
      POKEDEX_SEARCH_UI_RETURN,
      POKEDEX_SEARCH_UI_RETURN,
      { 168, 191, 232, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

void PokedexSearchUI_InitCursor(PokedexSearchWork *wk, u32 pos) {
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        wk->cursorMove = CursorMove_Create(sPokedexSearchCursorData, &sPokedexSearchCursorCallbacks, wk, FALSE, pos,
                                           HEAPID_POKEDEX_SEARCH);
    } else {
        wk->cursorMove = CursorMove_Create(sPokedexSearchCursorData, &sPokedexSearchCursorCallbacks, wk, TRUE, pos,
                                           HEAPID_POKEDEX_SEARCH);
        PokedexSearchUI_SetCursorPalette(wk, pos, POKEDEX_SEARCH_UI_PALETTE_ON);
    }
    CursorMove_SetHideOnTouch(wk->cursorMove);
}

void PokedexSearchUI_ExitCursor(PokedexSearchWork *wk) {
    if (wk->cursorMove != NULL) {
        CursorMove_Delete(wk->cursorMove);
        wk->cursorMove = NULL;
    }
}

// Sets the palette of a position's frame: the criteria and the two buttons under them
void PokedexSearchUI_SetCursorPalette(PokedexSearchWork *wk, u32 pos, u32 palette) {
    if (pos <= POKEDEX_SEARCH_UI_RESET) {
        const CursorMoveData *data = CursorMove_GetData(wk->cursorMove, pos);

        GFL_BGSysSetScrPaletteNo(0, data->x, data->y, data->width, data->height, palette);
        GFL_BGSysQueueScrLoad(0);
    }
}

u32 PokedexSearchUI_Main(PokedexSearchWork *wk) {
    u32 ret = CursorMove_Update(wk->cursorMove);

    if (ret != CURSOR_MOVE_NONE) {
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        GCTX_HIDSetUsingTouch(FALSE);
        return POKEDEX_SEARCH_UI_EXIT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        GCTX_HIDSetUsingTouch(FALSE);
        return POKEDEX_SEARCH_UI_SHORTCUT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        GCTX_HIDSetUsingTouch(FALSE);
        PokedexSearchUI_MoveCursor(wk, POKEDEX_SEARCH_UI_START);
        return POKEDEX_SEARCH_UI_START;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        GCTX_HIDSetUsingTouch(FALSE);
        PokedexSearchUI_MoveCursor(wk, POKEDEX_SEARCH_UI_RESET);
        return POKEDEX_SEARCH_UI_RESET;
    }
    // Any other key only shows the cursor again
    if (GCTX_HIDGetPressedKeys() != 0 && GCTX_HIDIsUsingTouch() == TRUE) {
        GCTX_HIDSetUsingTouch(FALSE);
        CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        return CURSOR_MOVE_CURSOR_ON;
    }
    return ret;
}

static void PokedexSearchUI_MoveCursor(PokedexSearchWork *wk, u32 pos) {
    PokedexSearchUI_SetCursorPalette(wk, CursorMove_GetPos(wk->cursorMove), POKEDEX_SEARCH_UI_PALETTE_OFF);
    PokedexSearchUI_SetCursorPalette(wk, pos, POKEDEX_SEARCH_UI_PALETTE_ON);
    CursorMove_SetPos(wk->cursorMove, pos);
}

static void PokedexSearchUI_CursorOn(void *work, int pos, int prevPos) {
    PokedexSearchWork *wk = work;
    u32 keys = GCTX_HIDGetPressedKeys();

    // B alone only shows the cursor
    if (!(keys & PAD_BUTTON_B) || (keys & (PAD_BUTTON_A | PAD_KEY_RIGHT | PAD_KEY_LEFT | PAD_KEY_UP | PAD_KEY_DOWN))) {
        PokedexSearchUI_SetCursorPalette(wk, pos, POKEDEX_SEARCH_UI_PALETTE_ON);
        BlinkPalAnm_InitAnime(wk->blinkAnm);
    }
}

static void PokedexSearchUI_CursorOff(void *work, int pos, int prevPos) {
    PokedexSearchWork *wk = work;

    PokedexSearchUI_SetCursorPalette(wk, prevPos, POKEDEX_SEARCH_UI_PALETTE_OFF);
    PokedexSearchUI_SetCursorPalette(wk, pos, POKEDEX_SEARCH_UI_PALETTE_OFF);
}

static void PokedexSearchUI_CursorMove(void *work, int pos, int prevPos) {
    PokedexSearchWork *wk = work;

    PokedexSearchUI_SetCursorPalette(wk, prevPos, POKEDEX_SEARCH_UI_PALETTE_OFF);
    PokedexSearchUI_SetCursorPalette(wk, pos, POKEDEX_SEARCH_UI_PALETTE_ON);
    BlinkPalAnm_InitAnime(wk->blinkAnm);
}

// The shortcut button doesn't take the cursor
static void PokedexSearchUI_CursorTouch(void *work, int pos, int prevPos) {
    PokedexSearchWork *wk = work;

    PokedexSearchUI_SetCursorPalette(wk, prevPos, POKEDEX_SEARCH_UI_PALETTE_OFF);
    if (pos == POKEDEX_SEARCH_UI_SHORTCUT) {
        CursorMove_SetPos(wk->cursorMove, prevPos);
    }
}

BOOL PokedexSearchUI_WaitResult(PokedexSearchWork *wk) {
    if (wk->timer == RESULT_FRAMES) {
        wk->timer = 0;
        return TRUE;
    }
    wk->timer++;
    if (GFL_HIDIsTouchPressed()) {
        GCTX_HIDSetUsingTouch(TRUE);
        wk->timer = 0;
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GCTX_HIDSetUsingTouch(FALSE);
        wk->timer = 0;
        return TRUE;
    }
    return FALSE;
}

int PokedexSearchUI_ListMain(PokedexSearchWork *wk) {
    int ret = GFL_HIDGetTouchPressedRect(sPokedexSearchListTouch);

    if (ret != -1) {
        GCTX_HIDSetUsingTouch(TRUE);
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GCTX_HIDSetUsingTouch(FALSE);
        return POKEDEX_SEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        GCTX_HIDSetUsingTouch(FALSE);
        return POKEDEX_SEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            GCTX_HIDSetUsingTouch(FALSE);
            FrameList_ShowCursor(wk->frameList);
        }
        return POKEDEX_SEARCH_UI_LIST_RESET;
    }
    return ret;
}

int PokedexSearchUI_ShapeListMain(PokedexSearchWork *wk) {
    int ret = GFL_HIDGetTouchPressedRect(sPokedexSearchShapeListTouch);

    if (ret != -1) {
        GCTX_HIDSetUsingTouch(TRUE);
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GCTX_HIDSetUsingTouch(FALSE);
        return POKEDEX_SEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        GCTX_HIDSetUsingTouch(FALSE);
        return POKEDEX_SEARCH_UI_LIST_CANCEL;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            GCTX_HIDSetUsingTouch(FALSE);
            FrameList_ShowCursor(wk->frameList);
        }
        return POKEDEX_SEARCH_UI_LIST_RESET;
    }
    return ret;
}
