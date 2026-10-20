#include "types.h"
#include "app/box2.h"
#include "app/box2_bgwfrm.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "constants/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "system/bgwinfrm.h"

// The PC box's frames of BG screen data that slide in and out (bgwinfrm.c): the party, the box menu and the
// marking frame. The ROM doesn't name this file; box2_bgwfrm.c is a guess after box2_main.c. None of these functions
// has a name yet

static void LoadFrameScreen(BGWinFrame *frames, u32 index, u32 datId);
static void PutMenuOffscreen(BGWinFrame *frames);
static void InitMessageFrame(Box2SysWork *syswk);

void Box2Frame_Init(Box2SysWork *syswk) {
    Box2AppWork *app = syswk->app;

    app->bgWinFrame = BGWinFrame_Create(BGWINFRAME_TRANSFER_VBLANK, 12, HEAPID_BOX2_APP);
    Box2Bmp_InitMenuFrames(app);
    InitMessageFrame(syswk);
    Box2Bmp_InitPartyButton(syswk);
    Box2Bmp_InitBoxListButton(syswk);

    BGWinFrame_InitFrame(app->bgWinFrame, 8, 1, 11, 15);
    if (syswk->param->mode == 4) {
        LoadFrameScreen(app->bgWinFrame, 8, 8);
    } else {
        LoadFrameScreen(app->bgWinFrame, 8, 9);
    }

    BGWinFrame_InitFrame(app->bgWinFrame, 7, 1, 11, 15);
    LoadFrameScreen(app->bgWinFrame, 7, 10);
    Box2Bmp_InitMarkingFrame(syswk);

    BGWinFrame_InitFrame(app->bgWinFrame, 9, 1, 11, 21);
    LoadFrameScreen(app->bgWinFrame, 9, 11);

    PutMenuOffscreen(app->bgWinFrame);
}

void Box2Frame_Delete(Box2AppWork *app) {
    BGWinFrame_Delete(app->bgWinFrame);
}

static void LoadFrameScreen(BGWinFrame *frames, u32 index, u32 datId) {
    BGWinFrame_LoadScreen(frames, index, ARCID_BOX2, datId, TRUE);
}

void Box2Frame_PutPartyLeft(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 8, 2, 6);
}

void Box2Frame_PutPartyRight(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 8, 21, 6);
}

void Box2Frame_PutPartyLeftBelow(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 8, 2, 24);
}

void Box2Frame_PutPartyRightBelow(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 8, 21, 24);
}

void Box2Frame_SlidePartyUp(BGWinFrame *frames) {
    s8 x, y;

    BGWinFrame_GetPos(frames, 8, &x, &y);
    if (y != 6) {
        BGWinFrame_StartMove(frames, 8, 0, -1, y - 6);
    }
}

void Box2Frame_SlidePartyDown(BGWinFrame *frames) {
    s8 x, y;

    BGWinFrame_GetPos(frames, 8, &x, &y);
    if (y != 24) {
        BGWinFrame_StartMove(frames, 8, 0, 1, 24 - y);
    }
}

void Box2Frame_SlidePartyRight(BGWinFrame *frames) {
    BGWinFrame_StartMove(frames, 8, 1, 0, 19);
}

void Box2Frame_SlidePartyLeft(BGWinFrame *frames) {
    BGWinFrame_StartMove(frames, 8, -1, 0, 19);
}

BOOL Box2Frame_PartyMoveStep(Box2SysWork *syswk) {
    s8 prevX, prevY, x, y;
    BOOL moving;
    BOOL ret;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, 8, &prevX, &prevY);
    moving = BGWinFrame_MoveStep(syswk->app->bgWinFrame, 8);
    BGWinFrame_GetPos(syswk->app->bgWinFrame, 8, &x, &y);
    if (prevX != x || prevY != y) {
        Box2Obj_MovePartyIconsWithFrame(syswk);
    }
    ret = FALSE;
    if (moving) {
        ret = TRUE;
    }
    return ret;
}

BOOL Box2Frame_IsPartyAtRight(BGWinFrame *frames) {
    s8 x, y;

    BGWinFrame_GetPos(frames, 8, &x, &y);
    if (x == 21 && y == 6) {
        return TRUE;
    }
    return FALSE;
}

BOOL Box2Frame_IsPartyAtLeft(BGWinFrame *frames) {
    s8 x, y;

    BGWinFrame_GetPos(frames, 8, &x, &y);
    if (x == 2 && y == 6) {
        return TRUE;
    }
    return FALSE;
}

BOOL Box2Frame_IsPartyUp(BGWinFrame *frames) {
    s8 x, y;

    BGWinFrame_GetPos(frames, 8, &x, &y);
    if (y == 6) {
        return TRUE;
    }
    return FALSE;
}

static void PutMenuOffscreen(BGWinFrame *frames) {
    u32 i;

    for (i = 0; i < 6; i++) {
        BGWinFrame_Put(frames, i, 32, i * 3 + 2);
    }
}

void Box2Frame_ShowMenu(BGWinFrame *frames) {
    u32 i;

    for (i = 0; i < 6; i++) {
        BGWinFrame_Put(frames, i, 21, i * 3 + 2);
    }
}

void Box2Frame_HideMenu(BGWinFrame *frames) {
    u32 i;

    for (i = 0; i < 6; i++) {
        BGWinFrame_Hide(frames, i);
        BGWinFrame_Put(frames, i, 32, i * 3 + 2);
    }
}

void Box2Frame_SlideMenuIn(BGWinFrame *frames) {
    s8 x, y;
    u16 i = 0;

    BGWinFrame_GetPos(frames, 0, &x, &y);
    if (x != 21) {
        for (; i < 6; i++) {
            BGWinFrame_StartMove(frames, i, -1, 0, x - 21);
        }
    }
}

void Box2Frame_SlideMenuOut(BGWinFrame *frames) {
    s8 x, y;
    u16 i = 0;

    BGWinFrame_GetPos(frames, 0, &x, &y);
    if (x != 32) {
        for (; i < 6; i++) {
            BGWinFrame_StartMove(frames, i, 1, 0, 32 - x);
        }
    }
}

BOOL Box2Frame_MenuMoveStep(BGWinFrame *frames) {
    BOOL moving = FALSE;
    u32 i;

    for (i = 0; i < 6; i++) {
        if (BGWinFrame_MoveStep(frames, i) == TRUE) {
            moving = TRUE;
        }
    }
    return moving;
}

BOOL Box2Frame_IsMenuOnScreen(BGWinFrame *frames) {
    s8 x, y;
    BOOL ret = FALSE;

    BGWinFrame_GetPos(frames, 0, &x, &y);
    if (x != 32) {
        ret = TRUE;
    }
    return ret;
}

BOOL Box2Frame_IsMenuIn(BGWinFrame *frames) {
    s8 x, y;
    BOOL ret = FALSE;

    BGWinFrame_GetPos(frames, 0, &x, &y);
    if (x == 21) {
        ret = TRUE;
    }
    return ret;
}

static void InitMessageFrame(Box2SysWork *syswk) {
    u16 *screen = GFL_BGSysIsScrHeapExists(0);

    BGWinFrame_InitFrame(syswk->app->bgWinFrame, 6, 0, 32, 3);
    BGWinFrame_SetScreen(syswk->app->bgWinFrame, 6, screen + 32 * 21);
}

void Box2Frame_PutMessage(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 6, 0, 21);
}

void Box2Frame_ShowPartyButton(Box2AppWork *app) {
    BGWinFrame_Put(app->bgWinFrame, 10, 0, 21);
}

void Box2Frame_HidePartyButton(Box2AppWork *app) {
    BGWinFrame_Hide(app->bgWinFrame, 10);
}

void Box2Frame_ShowBoxListButton(Box2AppWork *app) {
    BGWinFrame_Put(app->bgWinFrame, 11, 0, 21);
}

void Box2Frame_HideBoxListButton(Box2AppWork *app) {
    BGWinFrame_Hide(app->bgWinFrame, 11);
}

void Box2Frame_SlideMarkingIn(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 7, 21, 24);
    BGWinFrame_StartMove(frames, 7, 0, -1, 19);
}

void Box2Frame_SlideMarkingOut(BGWinFrame *frames) {
    BGWinFrame_StartMove(frames, 7, 0, 1, 19);
}

BOOL Box2Frame_MarkingMoveStep(Box2SysWork *syswk) {
    s8 prevX, prevY, x, y;
    BOOL moving;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, 7, &prevX, &prevY);
    moving = BGWinFrame_MoveStep(syswk->app->bgWinFrame, 7);
    BGWinFrame_GetPos(syswk->app->bgWinFrame, 7, &x, &y);
    if (prevX != x || prevY != y) {
        Box2Obj_MoveMarkIcons(syswk);
    }
    return moving;
}

void Box2Frame_SlideBoxListIn(BGWinFrame *frames) {
    BGWinFrame_Put(frames, 9, 32, 0);
    BGWinFrame_StartMove(frames, 9, -1, 0, 11);
}

void Box2Frame_SlideBoxListOut(BGWinFrame *frames) {
    BGWinFrame_StartMove(frames, 9, 1, 0, 11);
}

BOOL Box2Frame_IsBoxListIn(BGWinFrame *frames) {
    s8 x, y;

    BGWinFrame_GetPos(frames, 9, &x, &y);
    if (x == 21) {
        return TRUE;
    }
    return FALSE;
}
