#include "app/pmsiv_category.h"
#include "types.h"
#include "app/pms_input.h"
#include "app/pms_input_data.h"
#include "app/pms_input_view.h"
#include "app/pmsi_initial_data.h"
#include "app/pmsiv_menu.h"
#include "app/pmsiv_tool.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The phrase input's categories: the buttons of the word groups, three to a row, or of the initials, on BG1, the
// letters the player types for the search and the words it finds on BG6, and the cursor over them. The names are
// ours, guessed

// The message file of the groups' names
#define PMSIV_CATEGORY_MSG_FILE TEXT_BANK_PMS_CATEGORIES
// The name shown for a group with no word unlocked
#define PMSIV_CATEGORY_MSG_EMPTY 13

// The cursor's animations
#define CURSOR_ANIM_GROUP 4
#define CURSOR_ANIM_INITIAL 6
#define CURSOR_ANIM_BUTTON 16
#define CURSOR_ANIM_INITIAL_DECIDE 30
#define CURSOR_ANIM_GROUP_DECIDE 31

// How many results the list shows
#define PMSIV_CATEGORY_RESULT_MAX 22

struct PMSIVCategory {
    PMSInputView *vwk;
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    u32 unkC;
    int seq;
    u32 unk14;
    PMSIVToolBlendWork blend;
    PMSIVToolScrollWork scroll;
    ClActor *cursor;
    // Whether the cursor was shown before its animation of a category chosen
    BOOL cursorVisible;
    int *keyMode;
    BmpWin *groupWin[PMSI_CATEGORY_COUNT];
    BmpWin *initialWin;
    BmpWin *searchWin;
    BmpWin *resultWin;
    PrintWindow resultPrint;
};

static u32 PMSIVCategory_SetupGroupWindow(PMSIVCategory *wk, u32 charPos);
static u32 PMSIVCategory_SetupInitialWindow(PMSIVCategory *wk, u32 charPos);
static u32 PMSIVCategory_SetupSearchWindow(PMSIVCategory *wk, u32 charPos);
static void PMSIVCategory_SetupResultWindow(PMSIVCategory *wk);
static void PMSIVCategory_SetupActor(PMSIVCategory *wk);
static void PMSIVCategory_HideResultList(PMSIVCategory *wk);
static void PMSIVCategory_ShowResultList(PMSIVCategory *wk, u32 count);

const u32 PMSIV_CATEGORY_UNK_7198[3] = { 0x1f, 0x3f, 0x3f };

// The message of each group's name
static const u8 sPMSIVCategoryGroupMsgs[PMSI_CATEGORY_COUNT] = { 0, 1, 2, 3, 11, 5, 12, 6, 7, 8, 10, 9 };

PMSIVCategory *PMSIVCategory_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSIVCategory *wk = GFL_HeapAllocate(HEAPID_PMS_INPUT, sizeof(PMSIVCategory), FALSE, "pmsiv_category.c", 222);

    wk->vwk = vwk;
    wk->mwk = mwk;
    wk->dwk = dwk;
    wk->unkC = 0;
    wk->unk14 = 0;
    wk->cursor = NULL;
    wk->cursorVisible = FALSE;
    wk->keyMode = PMSInput_GetKeyModePtr(mwk);
    return wk;
}

void PMSIVCategory_Delete(PMSIVCategory *wk) {
    u8 i;

    if (wk->cursor) {
        func_0204c108(wk->cursor);
    }
    for (i = 0; i < PMSI_CATEGORY_COUNT; i++) {
        if (wk->groupWin[i]) {
            BmpWin_Free(wk->groupWin[i]);
        }
    }
    if (wk->initialWin) {
        BmpWin_Free(wk->initialWin);
    }
    if (wk->searchWin) {
        BmpWin_Free(wk->searchWin);
    }
    if (wk->resultWin) {
        BmpWin_Free(wk->resultWin);
    }
    GFL_HeapFree(wk);
}

void PMSIVCategory_SetupGraphicDatas(PMSIVCategory *wk, ArcTool *arc) {
    u32 charPos;

    loadBGScrToVramByFileNoReserveNegAlign(arc, 27, 1, 0, 0, FALSE, HEAPID_PMS_INPUT);
    charPos = GFL_BGSysLoadArcNCGRStatic(arc, 15, 1, 0, 0, FALSE, HEAPID_PMS_INPUT);
    charPos = PMSIVCategory_SetupGroupWindow(wk, charPos / 0x20);
    charPos = PMSIVCategory_SetupInitialWindow(wk, charPos);
    charPos = PMSIVCategory_SetupSearchWindow(wk, charPos);
    PMSIVCategory_SetupResultWindow(wk);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, -4);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, 16);
    PMSIVCategory_SetupActor(wk);
    gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, -10);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_ALL, TRUE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_ALL & ~GX_PLANEMASK_BG1, TRUE);
    G2_SetWnd0Position(0, 0, 255, 168);
    GX_SetVisibleWnd(GX_WNDMASK_W0);
    GFL_BGSysLoadScr(1);
}

static u32 PMSIVCategory_SetupGroupWindow(PMSIVCategory *wk, u32 charPos) {
    int i;
    int x, y;
    MsgData *msgData;
    Font *font;

    font = PMSIView_GetFont(wk->vwk);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PMSIV_CATEGORY_MSG_FILE, HEAPID_PMS_INPUT);
    x = 1;
    y = 9;

    for (i = 0; i < PMSI_CATEGORY_COUNT; i++) {
        BmpWin *win;
        StrBuf *str;
        u32 width;

        if (i != 0 && i % 3 == 0) {
            x = 1;
            y += 3;
        }
        if (PMSIData_GetCategoryWordCount(wk->dwk, i)) {
            str = GFL_MsgDataLoadStrbufNew(msgData, sPMSIVCategoryGroupMsgs[i]);
            GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
        } else {
            str = GFL_MsgDataLoadStrbufNew(msgData, PMSIV_CATEGORY_MSG_EMPTY);
            GFL_TextRndUpdateColorIndexLUT(3, 4, 15);
        }
        win = BmpWin_CreateDynamic(1, x, y, 9, 2, 12, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
        width = 72 - GFL_FontGetBlockWidth(str, font, 0);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), width / 2, 0, str, font);
        BmpWin_FlushMap(win);
        BmpWin_FlushChar(win);
        GFL_StrBufFree(str);
        wk->groupWin[i] = win;
        charPos += 9 * 2;
        x += 10;
    }
    GFL_MsgDataFree(msgData);
    return charPos;
}

static u32 PMSIVCategory_SetupInitialWindow(PMSIVCategory *wk, u32 charPos) {
    u32 count;
    Font *font = PMSIView_GetFont(wk->vwk);
    BmpWin *win = BmpWin_CreateDynamic(1, 1, 8, 30, 14, 12, TRUE);
    StrBuf *str = GFL_StrBufCreate(4, HEAPID_PMS_INPUT);
    u32 i;
    u32 x, y;

    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    count = PMSIInitial_GetCount();
    for (i = 0; i < count; i++) {
        PMSIInitial_GetString(i, str);
        PMSIInitial_GetPos(i, &x, &y);
        GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), x + 5, y, str, font);
    }
    BmpWin_FlushChar(win);
    GFL_StrBufFree(str);
    wk->initialWin = win;
    return charPos + 30 * 14;
}

static u32 PMSIVCategory_SetupSearchWindow(PMSIVCategory *wk, u32 charPos) {
    BmpWin *win = BmpWin_CreateDynamic(1, 10, 3, 11, 2, 12, TRUE);

    GFL_BitmapFill(BmpWin_GetBitmap(win), 1);
    BmpWin_FlushChar(win);
    wk->searchWin = win;
    return charPos + 11 * 2;
}

static void PMSIVCategory_SetupResultWindow(PMSIVCategory *wk) {
    BmpWin *win = BmpWin_CreateDynamic(6, 1, 1, 30, 22, 0, TRUE);

    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(6);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, -192);
    wk->resultWin = win;
}

static void PMSIVCategory_SetupActor(PMSIVCategory *wk) {
    PMSIVObjRes res;

    PMSIView_GetObjRes(wk->vwk, &res, 0, 0);
    wk->cursor = PMSIView_AddActor(wk->vwk, &res, 48, 64, 3, NNS_G2D_VRAM_TYPE_2DMAIN);
    func_0204c488(wk->cursor, CURSOR_ANIM_GROUP);
    func_0204c124(wk->cursor, FALSE);
}

void PMSIVCategory_VisibleCursor(PMSIVCategory *wk, BOOL visible) {
    if (visible) {
        if (*wk->keyMode == 0) {
            func_0204c124(wk->cursor, TRUE);
            if (PMSInput_GetCategoryMode(wk->mwk) == 0) {
                func_0204c488(wk->cursor, CURSOR_ANIM_GROUP);
            } else {
                func_0204c488(wk->cursor, CURSOR_ANIM_INITIAL);
            }
        } else {
            func_0204c124(wk->cursor, FALSE);
        }
    } else {
        func_0204c124(wk->cursor, FALSE);
    }
}

void PMSIVCategory_MoveCursor(PMSIVCategory *wk, u32 pos) {
    ClActorPos actPos = PMSIV_CURSOR_HIDE_POS;
    u32 anim;

    if (PMSInput_GetCategoryMode(wk->mwk) == 0 && pos != PMSIV_CATEGORY_POS_BACK) {
        actPos.x = (pos % 3) * 80 + 48;
        actPos.y = (pos / 3) * 24 + 64;
        anim = CURSOR_ANIM_GROUP;
    } else {
        PMSIVMenu *menu = PMSIView_GetMenu(wk->vwk);

        anim = CURSOR_ANIM_BUTTON;
        if (pos == PMSIV_CATEGORY_POS_BUTTON_0) {
            if (*wk->keyMode == 0) {
                PMSIVMenu_SetCursor(menu, 0, TRUE);
            } else {
                PMSIVMenu_SetCursor(menu, 0, FALSE);
            }
        } else if (pos == PMSIV_CATEGORY_POS_BUTTON_1) {
            if (*wk->keyMode == 0) {
                PMSIVMenu_SetCursor(menu, 1, TRUE);
            } else {
                PMSIVMenu_SetCursor(menu, 0, FALSE);
            }
        } else if (pos == PMSIV_CATEGORY_POS_BACK) {
            if (*wk->keyMode == 0) {
                PMSIVMenu_SetCursor(menu, 2, TRUE);
            } else {
                PMSIVMenu_SetCursor(menu, 0, FALSE);
            }
        } else {
            u32 x, y;

            PMSIVMenu_SetCursor(menu, 0, FALSE);
            PMSIInitial_GetPos(pos, &x, &y);
            actPos.x = x + 18;
            actPos.y = y + 56;
            anim = CURSOR_ANIM_INITIAL;
        }
    }
    func_0204c140(wk->cursor, &actPos, CLACT_SURFACE_MAIN);
    func_0204c488(wk->cursor, anim);
}

void PMSIVCategory_StartEnableBG(PMSIVCategory *wk) {
    wk->seq = 0;
    PMSIVTool_SetupBrightWork(&wk->blend, GX_BLEND_PLANEMASK_BG1, -10, 0, 16);
}

BOOL PMSIVCategory_WaitEnableBG(PMSIVCategory *wk) {
    if (wk->seq == 0 && PMSIVTool_WaitBright(&wk->blend)) {
        return TRUE;
    }
    return FALSE;
}

void PMSIVCategory_StartDisableBG(PMSIVCategory *wk) {
    wk->seq = 0;
    PMSIVTool_SetupScrollWork(&wk->scroll, 1, PMSIV_TOOL_SCROLL_Y, 0, 6);
}

BOOL PMSIVCategory_WaitDisableBG(PMSIVCategory *wk) {
    switch (wk->seq) {
    case 0:
        if (PMSIVTool_WaitScroll(&wk->scroll)) {
            PMSIVTool_SetupBrightWork(&wk->blend, GX_BLEND_PLANEMASK_BG1, 0, -10, 16);
            wk->seq++;
        }
        break;
    case 1:
        return PMSIVTool_WaitBright(&wk->blend);
    }
    return FALSE;
}

void PMSIVCategory_SetDisableBG(PMSIVCategory *wk) {
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, 16);
}

void PMSIVCategory_StartBrightDown(PMSIVCategory *wk) {
    PMSIVTool_SetupBrightWork(&wk->blend, GX_BLEND_PLANEMASK_BG1, 0, -10, 16);
}

BOOL PMSIVCategory_WaitBrightDown(PMSIVCategory *wk) {
    return PMSIVTool_WaitBright(&wk->blend);
}

void PMSIVCategory_StartFadeOut(PMSIVCategory *wk) {
    PMSIVTool_SetupBlendWork(&wk->blend, GX_BLEND_PLANEMASK_BG1, 0x3f, PMSIV_BLEND_MAX, 0, 12);
}

BOOL PMSIVCategory_WaitFadeOut(PMSIVCategory *wk) {
    if (PMSIVTool_WaitBlend(&wk->blend)) {
        GFL_BGSysSetBGEnabled(1, FALSE);
        return TRUE;
    }
    return FALSE;
}

void PMSIVCategory_StartFadeIn(PMSIVCategory *wk) {
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, 0x3f, 0, 16);
    GFL_BGSysSetBGEnabled(1, TRUE);
    PMSIVTool_SetupBlendWork(&wk->blend, GX_BLEND_PLANEMASK_BG1, 0x3f, 0, PMSIV_BLEND_MAX, 12);
}

BOOL PMSIVCategory_WaitFadeIn(PMSIVCategory *wk) {
    return PMSIVTool_WaitBlend(&wk->blend);
}

void PMSIVCategory_ChangeModeBG(PMSIVCategory *wk) {
    int x;

    if (PMSInput_GetCategoryMode(wk->mwk) == 0) {
        x = -4;
    } else {
        x = 0;
    }
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, x);
}

void PMSIVCategory_ChangeModeScreen(PMSIVCategory *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PMSI, HEAPID_PMS_INPUT);

    if (PMSInput_GetCategoryMode(wk->mwk) == 0) {
        int i;

        GFL_G2DIOLoadNSCRAsync(arc, 27, 1, 0, 0, 0, FALSE, HEAPID_PMS_INPUT);
        for (i = 0; i < PMSI_CATEGORY_COUNT; i++) {
            BmpWin_FlushMap(wk->groupWin[i]);
        }
    } else {
        GFL_G2DIOLoadNSCRAsync(arc, 26, 1, 0, 0, 0, FALSE, HEAPID_PMS_INPUT);
        BmpWin_FlushMap(wk->initialWin);
        BmpWin_FlushMap(wk->searchWin);
    }
    GFL_ArcToolFree(arc);
    GFL_BGSysQueueScrLoad(1);
}

BOOL PMSIVCategory_WaitModeChange(PMSIVCategory *wk) {
    return TRUE;
}

void PMSIVCategory_PrintSearchInput(PMSIVCategory *wk) {
    Font *font = PMSIView_GetFont(wk->vwk);
    BmpWin *win = wk->searchWin;
    StrBuf *str = GFL_StrBufCreate(14, HEAPID_PMS_INPUT);

    PMSInput_GetSearchInputStr(wk->mwk, str);
    GFL_BitmapFill(BmpWin_GetBitmap(win), 1);
    GFL_TextRndUpdateColorIndexLUT(15, 14, 1);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, str, font);
    GFL_StrBufFree(str);
    BmpWin_FlushMap(win);
    BmpWin_FlushChar(win);
    GFL_BGSysQueueScrLoad(1);
}

void PMSIVCategory_StartResultList(PMSIVCategory *wk, BOOL show) {
    u32 count = PMSInput_GetSearchResultCount(wk->mwk);

    if (count > PMSIV_CATEGORY_RESULT_MAX) {
        count = PMSIV_CATEGORY_RESULT_MAX;
    }
    if (show) {
        PMSIVCategory_ShowResultList(wk, count);
    } else {
        PMSIVCategory_HideResultList(wk);
    }
}

static void PMSIVCategory_HideResultList(PMSIVCategory *wk) {
    int y = -192 - GFL_BGSysGetBGOffsetY(6);

    PMSIVTool_SetupScrollWork(&wk->scroll, 6, PMSIV_TOOL_SCROLL_Y, y, 8);
}

static void PMSIVCategory_ShowResultList(PMSIVCategory *wk, u32 count) {
    PrintQueue *queue = PMSIView_GetPrintQueue(wk->vwk);
    Font *font = PMSIView_GetFont(wk->vwk);
    StrBuf *str = GFL_StrBufCreate(15, HEAPID_PMS_INPUT);
    int i;
    int height, offset;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->resultWin), 15);
    PrintWindow_Init(&wk->resultPrint, wk->resultWin);
    for (i = 0; i < count; i++) {
        int x;

        if (i % 2 == 0) {
            x = 8;
        } else {
            x = 128;
        }
        PMSInput_GetSearchResultStr(wk->mwk, i, str);
        PrintWindow_Print(&wk->resultPrint, queue, x, (i / 2) * 16, str, font, PRINT_COLOR(1, 2, 15));
    }
    GFL_StrBufFree(str);
    offset = GFL_BGSysGetBGOffsetY(6) + 192;
    height = ((count + 1) / 2) * 16 + 8;
    if (height > 192) {
        height = 192;
    } else if (height < 48) {
        height = 48;
    }
    PMSIVTool_SetupScrollWork(&wk->scroll, 6, PMSIV_TOOL_SCROLL_Y, height - offset, 8);
    BmpWin_FlushMap(wk->resultWin);
    GFL_BGSysQueueScrLoad(6);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

BOOL PMSIVCategory_WaitResultList(PMSIVCategory *wk, BOOL show) {
    PrintQueue *queue = PMSIView_GetPrintQueue(wk->vwk);
    BOOL printed = TRUE;
    BOOL scrolled;

    if (show) {
        PrintWindow_Flush(&wk->resultPrint, queue);
        printed = TRUE;
        if (wk->resultPrint.flushPending) {
            printed = FALSE;
        }
    }
    scrolled = PMSIVTool_WaitScroll(&wk->scroll);
    if (printed && scrolled) {
        if (!show) {
            GFL_BGSysSetBGEnabled(6, FALSE);
        }
        return TRUE;
    }
    return FALSE;
}

void PMSIVCategory_StartCursorDecide(PMSIVCategory *wk, u32 pos) {
    ClActorPos actPos;
    u32 anim;

    wk->cursorVisible = func_0204c138(wk->cursor);
    func_0204c124(wk->cursor, TRUE);
    if (PMSInput_GetCategoryMode(wk->mwk) == 0 && pos != PMSIV_CATEGORY_POS_BACK) {
        actPos.x = (pos % 3) * 80 + 48;
        actPos.y = (pos / 3) * 24 + 64;
        anim = CURSOR_ANIM_GROUP_DECIDE;
    } else {
        anim = CURSOR_ANIM_BUTTON;
        if (pos != PMSIV_CATEGORY_POS_BUTTON_0 && pos != PMSIV_CATEGORY_POS_BUTTON_1 &&
            pos != PMSIV_CATEGORY_POS_BACK) {
            u32 x, y;

            PMSIInitial_GetPos(pos, &x, &y);
            actPos.x = x + 18;
            actPos.y = y + 56;
            anim = CURSOR_ANIM_INITIAL_DECIDE;
        }
    }
    func_0204c140(wk->cursor, &actPos, CLACT_SURFACE_MAIN);
    func_0204c488(wk->cursor, anim);
}

BOOL PMSIVCategory_WaitCursorDecide(PMSIVCategory *wk) {
    u16 anim = func_0204c4a0(wk->cursor);

    if (anim != CURSOR_ANIM_GROUP_DECIDE && anim != CURSOR_ANIM_INITIAL_DECIDE) {
        return TRUE;
    }
    if (!func_0204c560(wk->cursor)) {
        func_0204c124(wk->cursor, wk->cursorVisible);
        func_0204c488(wk->cursor, PMSInput_GetCategoryMode(wk->mwk) == 0 ? CURSOR_ANIM_GROUP : CURSOR_ANIM_INITIAL);
        wk->cursorVisible = FALSE;
        return TRUE;
    }
    return FALSE;
}
