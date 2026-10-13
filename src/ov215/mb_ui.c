#include "types.h"
#include "app/mailbox_local.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "system/cursor_move.h"

// The mailbox screen's input: the touch screen's buttons, the cursor over the list and the keys. The ROM doesn't name
// this file; the name is a guess, after box2_ui.c

static void MailBox_CursorOn(void *work, int pos, int prevPos);
static void MailBox_CursorOff(void *work, int pos, int prevPos);
static void MailBox_CursorMoved(void *work, int pos, int prevPos);
static void MailBox_CursorTouched(void *work, int pos, int prevPos);

// The touch screen's page and return buttons, and the return button of the mail
static const TouchRect sButtonRects[] = {
    { 168, 191, 8, 32 },
    { 168, 191, 32, 56 },
    { 168, 191, 232, 255 },
    { 168, 191, 194, 231 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sReturnRects[] = {
    { 160, 191, 230, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const CursorMoveCallbacks sCursorCallbacks = {
    MailBox_CursorOn,
    MailBox_CursorOff,
    MailBox_CursorMoved,
    MailBox_CursorTouched,
};

static const CursorMoveData sCursorData[] = {
    { 64, 16, 0, 0, 0, 2, 0, 1, { 0x0, 0x1f, 0x8, 0x77 } },
    { 192, 16, 0, 0, 1, 3, 0, 1, { 0x0, 0x1f, 0x88, 0xf4 } },
    { 64, 48, 0, 0, 0, 4, 2, 3, { 0x20, 0x3f, 0x8, 0x77 } },
    { 192, 48, 0, 0, 1, 5, 2, 3, { 0x20, 0x3f, 0x88, 0xf4 } },
    { 64, 80, 0, 0, 2, 6, 4, 5, { 0x40, 0x5f, 0x8, 0x77 } },
    { 192, 80, 0, 0, 3, 7, 4, 5, { 0x40, 0x5f, 0x88, 0xf4 } },
    { 64, 112, 0, 0, 4, 8, 6, 7, { 0x60, 0x7f, 0x8, 0x77 } },
    { 192, 112, 0, 0, 5, 9, 6, 7, { 0x60, 0x7f, 0x88, 0xf4 } },
    { 64, 144, 0, 0, 6, 8, 8, 9, { 0x80, 0x9f, 0x8, 0x77 } },
    { 192, 144, 0, 0, 7, 9, 8, 9, { 0x80, 0x9f, 0x88, 0xf4 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

s32 MailBox_GetTouch(MailBoxWork *work) {
    return func_0203da0c(sButtonRects);
}

void MailBox_CreateCursor(MailBoxWork *work) {
    u32 cursor;

    if (work->page > work->view->maxPage) {
        work->page = work->view->maxPage;
        cursor = 0;
    } else {
        cursor = work->cursor;
    }
    work->view->cursor = CursorMove_Create(sCursorData, &sCursorCallbacks, work, TRUE, cursor, HEAPID_MAILBOX_VIEW);
    MailBox_UpdateCursor(work, cursor);
}

void MailBox_FreeCursor(MailBoxWork *work) {
    CursorMove_Delete(work->view->cursor);
}

static void MailBox_CursorOn(void *work, int pos, int prevPos) {
}

static void MailBox_CursorOff(void *work, int pos, int prevPos) {
}

static void MailBox_CursorMoved(void *work, int pos, int prevPos) {
    MailBox_UpdateCursor(work, pos);
}

static void MailBox_CursorTouched(void *work, int pos, int prevPos) {
    MailBox_UpdateCursor(work, pos);
}

void MailBox_UpdateCursor(MailBoxWork *work, u32 pos) {
    const CursorMoveData *data = CursorMove_GetData(work->view->cursor, pos);

    MailBox_SetActorPos(work->view, MAILBOX_ACTOR_CURSOR, data->x, data->y);
    if (pos == 10) {
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CURSOR, 4);
    } else {
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CURSOR, 5);
    }
}

BOOL MailBox_IsReturnPressed(void) {
    if (func_0203da0c(sReturnRects) != -1) {
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        return TRUE;
    }
    return FALSE;
}

BOOL MailBox_IsCloseKeyPressed(void) {
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        return TRUE;
    }
    return FALSE;
}
