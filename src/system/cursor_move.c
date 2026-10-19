#include "system/cursor_move.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/heapsys.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"

// A cursor that moves between the positions of a table with the keys or the touch screen. A touch hides the cursor;
// a key shows it again

struct CursorMove {
    const CursorMoveData *data;
    BOOL cursorVisible;
    BOOL bOnlyShowsCursor;
    u8 hideOnTouch;
    u8 pos;
    u8 prevPos;
    // The position to go back to along a CURSOR_MOVE_LINK_RETURN link, or CURSOR_MOVE_NO_LINK
    u8 returnPos;
    // A bit per position, set while it is enabled
    u32 enabled[2];
    const CursorMoveCallbacks *callbacks;
    void *work;
};

static int CursorMove_Touch(CursorMove *cursor, int *pos);
static u8 CursorMoveData_GetLink(const CursorMoveData *data, u8 pos, u8 dir);
static int CursorMove_ShowCursor(CursorMove *cursor);
static int CursorMove_ShowCursorByKey(CursorMove *cursor);
static int CursorMove_Move(CursorMove *cursor, u8 dir, u8 next);
static BOOL CursorMoveData_IsReturnLink(const CursorMoveData *data, u8 dir);
static BOOL CursorMove_IsPosEnabled(const u32 *enabled, u32 pos);

CursorMove *CursorMove_Create(const CursorMoveData *data, const CursorMoveCallbacks *callbacks, void *work,
                              BOOL cursorVisible, u8 pos, HeapID heapId) {
    CursorMove *cursor = GFL_HeapAllocate(heapId, sizeof(CursorMove), FALSE, "cursor_move.c", 106);

    cursor->data = data;
    cursor->callbacks = callbacks;
    cursor->work = work;
    cursor->hideOnTouch = FALSE;
    cursor->cursorVisible = cursorVisible;
    cursor->bOnlyShowsCursor = FALSE;
    cursor->pos = pos;
    cursor->prevPos = CURSOR_MOVE_NO_LINK;
    cursor->returnPos = CURSOR_MOVE_NO_LINK;
    cursor->enabled[0] = 0xffffffff;
    cursor->enabled[1] = 0xffffffff;
    return cursor;
}

void CursorMove_Delete(CursorMove *cursor) {
    // The only free in the game that skips GFL_HeapFree's debug trace
    GFL_HeapFreeCore(cursor);
}

void CursorMove_SetHideOnTouch(CursorMove *cursor) {
    cursor->hideOnTouch = TRUE;
}

void CursorMove_SetBOnlyShowsCursor(CursorMove *cursor) {
    cursor->bOnlyShowsCursor = TRUE;
}

int CursorMove_UpdatePressed(CursorMove *cursor) {
    int pos;
    u8 dir;
    u8 next;

    if (CursorMove_Touch(cursor, &pos) == TRUE) {
        return pos;
    }
    if (cursor->cursorVisible == FALSE && cursor->hideOnTouch == TRUE) {
        return CursorMove_ShowCursorByKey(cursor);
    }

    if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_UP);
        dir = CURSOR_MOVE_UP;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_DOWN);
        dir = CURSOR_MOVE_DOWN;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_LEFT);
        dir = CURSOR_MOVE_LEFT;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_RIGHT);
        dir = CURSOR_MOVE_RIGHT;
    } else {
        next = CURSOR_MOVE_NO_LINK;
    }
    if (next != CURSOR_MOVE_NO_LINK) {
        return CursorMove_Move(cursor, dir, next);
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GCTX_HIDSetUsingTouch(FALSE);
        return cursor->pos;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GCTX_HIDSetUsingTouch(FALSE);
        return CURSOR_MOVE_CANCEL;
    }
    return CURSOR_MOVE_NONE;
}

u32 CursorMove_Update(CursorMove *cursor) {
    int pos;
    u8 dir;
    u8 next;

    if (CursorMove_Touch(cursor, &pos) == TRUE) {
        return pos;
    }
    if (cursor->cursorVisible == FALSE && cursor->hideOnTouch == TRUE) {
        return CursorMove_ShowCursorByKey(cursor);
    }

    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_UP);
        dir = CURSOR_MOVE_UP;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_DOWN);
        dir = CURSOR_MOVE_DOWN;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_LEFT);
        dir = CURSOR_MOVE_LEFT;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        next = CursorMoveData_GetLink(cursor->data, cursor->pos, CURSOR_MOVE_RIGHT);
        dir = CURSOR_MOVE_RIGHT;
    } else {
        next = CURSOR_MOVE_NO_LINK;
    }
    if (next != CURSOR_MOVE_NO_LINK) {
        return CursorMove_Move(cursor, dir, next);
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GCTX_HIDSetUsingTouch(FALSE);
        return cursor->pos;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GCTX_HIDSetUsingTouch(FALSE);
        return CURSOR_MOVE_CANCEL;
    }
    return CURSOR_MOVE_NONE;
}

// Whether the screen was touched this frame; pos is the enabled position touched, or -1
static int CursorMove_Touch(CursorMove *cursor, int *pos) {
    TouchRect rects[2];
    BOOL touched;
    u16 i;

    rects[1] = (TouchRect){ TOUCH_RECT_END, 0, 0, 0 };
    touched = FALSE;
    *pos = -1;
    for (i = 0;; i++) {
        if (cursor->data[i].rect.top == TOUCH_RECT_END) {
            break;
        }
        rects[0] = cursor->data[i].rect;
        if (GFL_HIDGetTouchPressedRect(rects) != TOUCH_RECT_NONE) {
            touched = TRUE;
            if (CursorMove_IsPosEnabled(cursor->enabled, i) == TRUE) {
                cursor->prevPos = cursor->pos;
                cursor->pos = i;
                if (cursor->cursorVisible == TRUE && cursor->hideOnTouch == TRUE) {
                    cursor->cursorVisible = FALSE;
                    cursor->callbacks->cursorOff(cursor->work, cursor->pos, cursor->prevPos);
                }
                cursor->callbacks->touch(cursor->work, cursor->pos, cursor->prevPos);
                cursor->prevPos = CURSOR_MOVE_NO_LINK;
                cursor->returnPos = CURSOR_MOVE_NO_LINK;
                *pos = i;
                GCTX_HIDSetUsingTouch(TRUE);
                break;
            }
        }
    }
    return touched;
}

static u8 CursorMoveData_GetLink(const CursorMoveData *data, u8 pos, u8 dir) {
    u8 next = pos;

    switch (dir) {
    case CURSOR_MOVE_UP:
        next = data[pos].up;
        break;
    case CURSOR_MOVE_DOWN:
        next = data[pos].down;
        break;
    case CURSOR_MOVE_LEFT:
        next = data[pos].left;
        break;
    case CURSOR_MOVE_RIGHT:
        next = data[pos].right;
        break;
    }
    return next;
}

static int CursorMove_ShowCursor(CursorMove *cursor) {
    GCTX_HIDSetUsingTouch(FALSE);
    cursor->cursorVisible = TRUE;
    cursor->prevPos = CURSOR_MOVE_NO_LINK;
    cursor->returnPos = CURSOR_MOVE_NO_LINK;
    cursor->callbacks->cursorOn(cursor->work, cursor->pos, cursor->prevPos);
    return CURSOR_MOVE_CURSOR_ON;
}

static int CursorMove_ShowCursorByKey(CursorMove *cursor) {
    if (cursor->bOnlyShowsCursor == TRUE) {
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_PLUS_KEY_MASK)) {
            return CursorMove_ShowCursor(cursor);
        }
    } else {
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_PLUS_KEY_MASK)) {
            return CursorMove_ShowCursor(cursor);
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            CursorMove_ShowCursor(cursor);
            return CURSOR_MOVE_CANCEL;
        }
    }
    return CURSOR_MOVE_NONE;
}

static int CursorMove_Move(CursorMove *cursor, u8 dir, u8 next) {
    BOOL direct;

    GCTX_HIDSetUsingTouch(FALSE);
    if (next & CURSOR_MOVE_LINK_RETURN) {
        if (cursor->returnPos != CURSOR_MOVE_NO_LINK) {
            next = cursor->returnPos;
        } else {
            next ^= CURSOR_MOVE_LINK_RETURN;
        }
    }

    // Skip the disabled positions along the direction, or stay put
    direct = TRUE;
    while (TRUE) {
        u8 link;

        if (cursor->enabled[next / 32] & (1 << (next % 32))) {
            break;
        }
        direct = FALSE;
        link = CursorMoveData_GetLink(cursor->data, next, dir) & CURSOR_MOVE_LINK_POS;
        if (link == next || link == cursor->pos) {
            next = cursor->pos;
            break;
        }
        next = link;
    }

    if (cursor->pos != next) {
        if (CursorMoveData_IsReturnLink(&cursor->data[next], dir) == TRUE && direct) {
            cursor->returnPos = cursor->pos;
        } else {
            cursor->returnPos = CURSOR_MOVE_NO_LINK;
        }
        cursor->prevPos = cursor->pos;
        cursor->pos = next;
        cursor->callbacks->move(cursor->work, cursor->pos, cursor->prevPos);
        return CURSOR_MOVE_MOVED;
    }

    if (dir == CURSOR_MOVE_UP) {
        return CURSOR_MOVE_EDGE_UP;
    }
    if (dir == CURSOR_MOVE_DOWN) {
        return CURSOR_MOVE_EDGE_DOWN;
    }
    if (dir == CURSOR_MOVE_LEFT) {
        return CURSOR_MOVE_EDGE_LEFT;
    }
    if (dir == CURSOR_MOVE_RIGHT) {
        return CURSOR_MOVE_EDGE_RIGHT;
    }
    return CURSOR_MOVE_NONE;
}

u8 CursorMove_GetPos(CursorMove *cursor) {
    return cursor->pos;
}

void CursorMove_SetPos(CursorMove *cursor, u8 pos) {
    cursor->pos = pos;
    cursor->prevPos = CURSOR_MOVE_NO_LINK;
    cursor->returnPos = CURSOR_MOVE_NO_LINK;
}

BOOL CursorMove_IsCursorVisible(CursorMove *cursor) {
    return cursor->cursorVisible;
}

void CursorMove_SetCursorVisible(CursorMove *cursor, BOOL visible) {
    cursor->cursorVisible = visible;
    if (visible == TRUE) {
        cursor->prevPos = CURSOR_MOVE_NO_LINK;
        cursor->returnPos = CURSOR_MOVE_NO_LINK;
        cursor->callbacks->cursorOn(cursor->work, cursor->pos, -1);
    } else {
        cursor->callbacks->cursorOff(cursor->work, cursor->pos, -1);
    }
}

void CursorMove_DisablePos(CursorMove *cursor, u32 pos) {
    if (cursor->enabled[pos / 32] & (1 << (pos % 32))) {
        cursor->enabled[pos / 32] &= (0xffffffff ^ (1 << (pos % 32)));
    }
}

void CursorMove_EnablePos(CursorMove *cursor, u32 pos) {
    if (!(cursor->enabled[pos / 32] & (1 << (pos % 32)))) {
        cursor->enabled[pos / 32] ^= (1 << (pos % 32));
    }
}

const CursorMoveData *CursorMove_GetData(CursorMove *cursor, u32 pos) {
    return &cursor->data[pos];
}

// Whether moving dir arrives at data along a link that goes back the way it came
static BOOL CursorMoveData_IsReturnLink(const CursorMoveData *data, u8 dir) {
    switch (dir) {
    case CURSOR_MOVE_UP:
        if (data->down & CURSOR_MOVE_LINK_RETURN) {
            return TRUE;
        }
        break;
    case CURSOR_MOVE_DOWN:
        if (data->up & CURSOR_MOVE_LINK_RETURN) {
            return TRUE;
        }
        break;
    case CURSOR_MOVE_LEFT:
        if (data->right & CURSOR_MOVE_LINK_RETURN) {
            return TRUE;
        }
        break;
    case CURSOR_MOVE_RIGHT:
        if (data->left & CURSOR_MOVE_LINK_RETURN) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL CursorMove_IsPosEnabled(const u32 *enabled, u32 pos) {
    return (enabled[pos / 32] & (1 << (pos % 32))) ? TRUE : FALSE;
}
