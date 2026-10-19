#include "types.h"
#include "app/manual.h"
#include "app/manual_common.h"
#include "app/manual_top.h"
#include "app/manual_touchbar.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"
#include "text/system/title_7.h"

// The manual's first screen, named after the ROM's "manual_top.c": two buttons, for the categories and for all the
// titles

#define BUTTON_COUNT 2
#define BUTTON_COLOR 0x39e3

enum {
    BUTTON_CATEGORY,
    BUTTON_TITLE,
};

enum {
    STATE_IDLE,
    STATE_END,
    STATE_TOUCH_BAR,
    STATE_DECIDE,
};

struct ManualTop {
    ManualTopParam *param;
    ManualCommon *common;
    AppTaskMenuRes *res;
    AppTaskMenuWin *wins[BUTTON_COUNT];
    TCB *vblankTask;
    u32 state;
    // Counts down to the end once the screen is left
    s8 endTimer;
};

static void ManualTop_VBlank(TCB *tcb, void *data);
static void ManualTop_LoadGraphics(ManualTop *top);
static void ManualTop_CreateMenu(ManualTop *top);
static void ManualTop_QueueScreen(ManualTop *top);
static void ManualTop_FreeGraphics(ManualTop *top);
static void ManualTop_FreeMenu(ManualTop *top);
static BOOL ManualTop_UpdateCursor(ManualTop *top);

static const TouchRect sManualTopTouchRects[] = {
    { 24, 47, 8, 247 },
    { 48, 71, 8, 247 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

ManualTop *ManualTop_Create(ManualTopParam *param, ManualCommon *common) {
    ManualTop *top = GFL_HeapAllocate(common->heapId, sizeof(ManualTop), TRUE, "manual_top.c", 157);

    top->param = param;
    param->result = MANUAL_TOP_RESULT_RETURN;
    top->common = common;
    ManualTop_LoadGraphics(top);
    ManualTop_QueueScreen(top);
    top->vblankTask = GFL_VBlankTCBAdd(ManualTop_VBlank, top, 1);
    top->state = STATE_IDLE;
    top->endTimer = -1;
    ManualTouchBar_SetMode(top->common->touchBar, MANUAL_TOUCHBAR_MODE_TOP);
    return top;
}

void ManualTop_Free(ManualTop *top) {
    GFL_TCBRemove(top->vblankTask);
    GFL_HeapFree(top);
}

// Returns whether the screen has ended
BOOL ManualTop_Main(ManualTop *top) {
    BOOL ended = FALSE;
    u8 i;

    if (top->endTimer == 0) {
        ended = TRUE;
    } else if (top->endTimer > 0) {
        if (top->endTimer == 1) {
            ManualTop_FreeGraphics(top);
        }
        top->endTimer--;
    }
    if (top->state == STATE_IDLE || top->state == STATE_TOUCH_BAR) {
        ManualTouchBar_Main(top->common->touchBar);
        if (top->state == STATE_IDLE) {
            if (ManualTouchBar_GetTouched(top->common->touchBar) != -1) {
                top->state = STATE_TOUCH_BAR;
            }
        } else {
            u32 decided = ManualTouchBar_GetDecided(top->common->touchBar);

            if (decided != -1) {
                switch (decided) {
                case MANUAL_TOUCHBAR_ICON_RETURN:
                    top->param->result = MANUAL_TOP_RESULT_RETURN;
                    break;
                case MANUAL_TOUCHBAR_ICON_CLOSE:
                    top->param->result = MANUAL_TOP_RESULT_CLOSE;
                    break;
                }
                top->state = STATE_END;
                top->endTimer = 1;
            }
        }
    }
    if (top->state == STATE_IDLE || top->state == STATE_DECIDE) {
        for (i = 0; i < BUTTON_COUNT; i++) {
            AppTaskMenuWin_Update(top->wins[i]);
        }
        if (top->state == STATE_IDLE) {
            if (ManualTop_UpdateCursor(top)) {
                top->state = STATE_DECIDE;
            }
        } else {
            if (AppTaskMenuWin_IsFlashFinished(top->wins[top->param->cursor])) {
                if (top->param->cursor == BUTTON_CATEGORY) {
                    top->param->result = MANUAL_TOP_RESULT_CATEGORY;
                } else if (top->param->cursor == BUTTON_TITLE) {
                    top->param->result = MANUAL_TOP_RESULT_TITLE;
                }
                top->state = STATE_END;
                top->endTimer = 1;
            }
        }
    }
    return ended;
}

static void ManualTop_VBlank(TCB *tcb, void *data) {
}

static void ManualTop_LoadGraphics(ManualTop *top) {
    GFL_G2DIOLoadArcNCLRDefault(top->common->arc, MANUAL_ARC_BG_NCLR, PALTYPE_SUB_BG, 0, 2 * 0x20, top->common->heapId);
    GFL_BGSysLoadArcNCGRStatic(top->common->arc, MANUAL_ARC_BG_NCGR, 4, 0, 0, FALSE, top->common->heapId);
    GFL_G2DIOLoadNSCRAsync(top->common->arc, MANUAL_ARC_TOP_NSCR, 4, 0, 0, 0, FALSE, top->common->heapId);
    ManualTop_CreateMenu(top);
}

static void ManualTop_CreateMenu(ManualTop *top) {
    AppTaskMenuItem items[BUTTON_COUNT];
    u8 i;

    top->res = AppTaskMenuRes_Create(5, 4, top->common->font, top->common->queue, top->common->heapId);
    items[BUTTON_CATEGORY].str = GFL_MsgDataLoadStrbufNew(top->common->msgSystem, Title7_Text_ViewCategories);
    items[BUTTON_CATEGORY].color = BUTTON_COLOR;
    items[BUTTON_CATEGORY].type = 0;
    items[BUTTON_TITLE].str = GFL_MsgDataLoadStrbufNew(top->common->msgSystem, Title7_Text_ShowAll);
    items[BUTTON_TITLE].color = BUTTON_COLOR;
    items[BUTTON_TITLE].type = 0;
    for (i = 0; i < BUTTON_COUNT; i++) {
        top->wins[i] = AppTaskMenuWin_Create(top->res, &items[i], 1, (i + 1) * 3, 30, top->common->heapId);
        GFL_StrBufFree(items[i].str);
    }
    if (!GCTX_HIDIsUsingTouch()) {
        AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
    }
}

static void ManualTop_QueueScreen(ManualTop *top) {
    GFL_BGSysQueueScrLoad(4);
}

static void ManualTop_FreeGraphics(ManualTop *top) {
    ManualTop_FreeMenu(top);
}

static void ManualTop_FreeMenu(ManualTop *top) {
    u8 i;

    PrintQueue_Clear(top->common->queue);
    for (i = 0; i < BUTTON_COUNT; i++) {
        AppTaskMenuWin_Free(top->wins[i]);
    }
    AppTaskMenuRes_Free(top->res);
    GFL_BGSysQueueScrLoad(5);
}

// Moves the cursor with the keys, or picks a button by touch. Returns whether a button was picked
static BOOL ManualTop_UpdateCursor(ManualTop *top) {
    s32 touched = GFL_HIDGetTouchPressedRect(sManualTopTouchRects);

    if (!GCTX_HIDIsUsingTouch()) {
        if (touched >= 0 && touched <= BUTTON_COUNT - 1) {
            GCTX_HIDSetUsingTouch(TRUE);
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], FALSE);
        }
    } else if (GCTX_HIDGetPressedKeys() != 0) {
        if (!(GCTX_HIDGetPressedKeys() & (PAD_BUTTON_B | PAD_BUTTON_X))) {
            GCTX_HIDSetUsingTouch(FALSE);
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return FALSE;
        }
    }
    if (!GCTX_HIDIsUsingTouch()) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            AppTaskMenuWin_SetFlashing(top->wins[top->param->cursor], TRUE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return TRUE;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], FALSE);
            if (top->param->cursor == BUTTON_CATEGORY) {
                top->param->cursor = BUTTON_TITLE;
            } else {
                top->param->cursor = BUTTON_CATEGORY;
            }
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], FALSE);
            if (top->param->cursor == BUTTON_TITLE) {
                top->param->cursor = BUTTON_CATEGORY;
            } else {
                top->param->cursor = BUTTON_TITLE;
            }
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        } else if (GCTX_HIDGetTypedKeys() & (PAD_BUTTON_L | PAD_KEY_LEFT)) {
            if (top->param->cursor == BUTTON_TITLE) {
                AppTaskMenuWin_SetActive(top->wins[top->param->cursor], FALSE);
                top->param->cursor = BUTTON_CATEGORY;
                AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else if (GCTX_HIDGetTypedKeys() & (PAD_BUTTON_R | PAD_KEY_RIGHT)) {
            if (top->param->cursor == BUTTON_CATEGORY) {
                AppTaskMenuWin_SetActive(top->wins[top->param->cursor], FALSE);
                top->param->cursor = BUTTON_TITLE;
                AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else {
        if (touched >= 0 && touched <= BUTTON_COUNT - 1) {
            top->param->cursor = touched;
            AppTaskMenuWin_SetActive(top->wins[top->param->cursor], TRUE);
            AppTaskMenuWin_SetFlashing(top->wins[top->param->cursor], TRUE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return TRUE;
        }
    }
    return FALSE;
}
