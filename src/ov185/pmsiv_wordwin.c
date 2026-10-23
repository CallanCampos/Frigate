#include "app/pmsiv_wordwin.h"
#include "types.h"
#include "app/pms_input.h"
#include "app/pms_input_view.h"
#include "app/pmsiv_tool.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The phrase input's word window: a category's words two to a row on BG2, drawn into a window that scrolls around
// as the list scrolls, with a cursor, a scroll bar and the number words as actors. The names are ours, guessed

// How many number words there are, shown as actors
#define WORDWIN_NUMBER_COUNT 10
// How many words the window draws at a time, and its rows' height
#define WORDWIN_DRAW_WORDS 16
#define WORDWIN_LINE_HEIGHT 24

// The group of the number words
#define GROUP_NUMBERS 11

// The scroll bar's range
#define SCROLL_BAR_TOP 18
#define SCROLL_BAR_BOTTOM 149
#define SCROLL_BAR_HEIGHT 132

// The cursor's animations
enum {
    CURSOR_ANIM = 8,
    CURSOR_ANIM_DECIDE = 28,
};

struct PMSIVWordWin {
    PMSInputView *vwk;
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    BmpWin *win;
    // A word that crosses the bottom of the window is drawn here first
    BmpWin *tmpWin;
    ClActor *cursor;
    BOOL cursorVisible;
    ClActor *scrollBar;
    ClActor *numberAct[WORDWIN_NUMBER_COUNT];
    StrBuf *str;
    PMSIVToolBlendWork blend;
    PMSIVToolScrollWork scroll;
    int seq;
    GXWndPlane savedOutside;
    int savedWnd;
    // Where the next words go in the window, and the first word drawn
    u32 y;
    u32 top;
    int *keyMode;
};

static void PMSIVWordWin_SetupActors(PMSIVWordWin *wk);
static void PMSIVWordWin_ClearScrollArea(PMSIVWordWin *wk, int vector);
static void PMSIVWordWin_ResetScroll(PMSIVWordWin *wk);
static void PMSIVWordWin_PrintWord(PMSIVWordWin *wk, unsigned int index, u32 y);

PMSIVWordWin *PMSIVWordWin_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSIVWordWin *wk = GFL_HeapAllocate(HEAPID_PMS_INPUT, sizeof(PMSIVWordWin), FALSE, "pmsiv_wordwin.c", 154);

    wk->vwk = vwk;
    wk->mwk = mwk;
    wk->dwk = dwk;
    wk->str = GFL_StrBufCreate(32, HEAPID_PMS_INPUT);
    wk->win = BmpWin_CreateDynamic(2, 3, 0, 26, 32, 13, TRUE);
    wk->tmpWin = BmpWin_CreateDynamic(2, 0, 0, 12, 4, 13, TRUE);
    wk->cursor = NULL;
    wk->cursorVisible = FALSE;
    wk->keyMode = PMSInput_GetKeyModePtr(wk->mwk);
    return wk;
}

void PMSIVWordWin_Delete(PMSIVWordWin *wk) {
    if (wk->cursor) {
        ClActor_Delete(wk->cursor);
    }
    if (wk->scrollBar) {
        ClActor_Delete(wk->scrollBar);
    }
    if (wk->str) {
        GFL_StrBufFree(wk->str);
    }
    BmpWin_Free(wk->tmpWin);
    BmpWin_Free(wk->win);
    GFL_HeapFree(wk);
}

void PMSIVWordWin_SetupGraphicDatas(PMSIVWordWin *wk) {
    GFL_BGSysClearCharCore(2, 0x20, 0, HEAPID_PMS_INPUT);
    GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 32, 13);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->win), 0);
    BmpWin_FlushMap(wk->win);
    BmpWin_FlushChar(wk->win);
    GFL_BGSysLoadScr(2);
    PMSIVWordWin_SetupActors(wk);
    GFL_BGSysSetBGEnabled(2, FALSE);
}

static void PMSIVWordWin_SetupActors(PMSIVWordWin *wk) {
    PMSIVObjRes res;
    PMSIVObjRes numberRes;
    int i;

    PMSIView_GetObjRes(wk->vwk, &res, 0, 0);
    wk->cursor = PMSIView_AddActor(wk->vwk, &res, 62, 24, 4, NNS_G2D_VRAM_TYPE_2DMAIN);
    ClActor_SetSequence(wk->cursor, CURSOR_ANIM);
    ClActor_SetVisible(wk->cursor, FALSE);
    wk->scrollBar = PMSIView_AddActor(wk->vwk, &res, 244, SCROLL_BAR_TOP, 5, NNS_G2D_VRAM_TYPE_2DMAIN);
    ClActor_SetSequence(wk->scrollBar, 18);
    ClActor_SetVisible(wk->scrollBar, FALSE);
    for (i = 0; i < WORDWIN_NUMBER_COUNT; i++) {
        PMSIView_GetObjRes2(wk->vwk, &numberRes, 0);
        wk->numberAct[i] = PMSIView_AddActor(wk->vwk, &numberRes, 0, 0, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
        ClActor_SetSequence(wk->numberAct[i], i);
        ClActor_SetVisible(wk->numberAct[i], FALSE);
        ClActor_SetBgPriority(wk->numberAct[i], 1);
    }
}

void PMSIVWordWin_SetupWords(PMSIVWordWin *wk) {
    u32 count, y, i;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->win), 0);
    PMSIVWordWin_ResetScroll(wk);
    count = PMSInput_GetCategoryWordMax(wk->mwk);
    if (count > WORDWIN_DRAW_WORDS) {
        count = WORDWIN_DRAW_WORDS;
    }
    y = wk->y;
    for (i = 0; i < count; i++) {
        PMSIVWordWin_PrintWord(wk, i, y);
        if (i & 1) {
            y += WORDWIN_LINE_HEIGHT;
        }
    }
    BmpWin_FlushChar(wk->win);
}

void PMSIVWordWin_RedrawWords(PMSIVWordWin *wk, u32 top) {
    u32 count, y, i;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->win), 0);
    PMSIVWordWin_ResetScroll(wk);
    wk->top = top * 2;
    count = PMSInput_GetCategoryWordMax(wk->mwk);
    if (count > WORDWIN_DRAW_WORDS) {
        count = WORDWIN_DRAW_WORDS;
    }
    y = wk->y;
    for (i = 0; i < count; i++) {
        PMSIVWordWin_PrintWord(wk, wk->top + i, y);
        if (i & 1) {
            y += WORDWIN_LINE_HEIGHT;
        }
    }
    BmpWin_FlushChar(wk->win);
}

void PMSIVWordWin_StartFadeIn(PMSIVWordWin *wk) {
    PMSIView_SetLowerScreen(wk->vwk, TRUE);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, 0x3f, 0, 16);
    GFL_BGSysSetBGEnabled(2, TRUE);
    G2_SetWnd1InsidePlane(GX_PLANEMASK_ALL, TRUE);
    wk->savedOutside = G2_GetWndOutsidePlane();
    wk->savedWnd = GX_GetVisibleWnd();
    G2_SetWndOutsidePlane(GX_PLANEMASK_ALL & ~GX_PLANEMASK_BG2, TRUE);
    G2_SetWnd1Position(0, 0, 255, 168);
    GX_SetVisibleWnd(GX_WNDMASK_W1);
    wk->seq = 0;
    PMSIVTool_SetupBlendWork(&wk->blend, GX_BLEND_PLANEMASK_BG2, 0x3f, 0, PMSIV_BLEND_MAX, 12);
}

BOOL PMSIVWordWin_WaitFadeIn(PMSIVWordWin *wk) {
    if (wk->seq == 0) {
        if (PMSIVTool_WaitBlend(&wk->blend)) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void PMSIVWordWin_StartFadeOut(PMSIVWordWin *wk) {
    wk->seq = 0;
    PMSIVTool_SetupBlendWork(&wk->blend, GX_BLEND_PLANEMASK_BG2, 0x3f, PMSIV_BLEND_MAX, 0, 12);
}

BOOL PMSIVWordWin_WaitFadeOut(PMSIVWordWin *wk) {
    int i;

    if (wk->seq == 0) {
        if (PMSIVTool_WaitBlend(&wk->blend)) {
            for (i = 0; i < WORDWIN_NUMBER_COUNT; i++) {
                ClActor_SetVisible(wk->numberAct[i], FALSE);
            }
            GFL_BGSysSetBGEnabled(2, FALSE);
            G2_SetWndOutsidePlane(wk->savedOutside.planeMask, wk->savedOutside.effect);
            GX_SetVisibleWnd(wk->savedWnd);
            return TRUE;
        }
    } else {
        return TRUE;
    }
    return FALSE;
}

void PMSIVWordWin_VisibleCursor(PMSIVWordWin *wk, BOOL visible) {
    if (visible) {
        if (*wk->keyMode == 0) {
            ClActor_SetVisible(wk->cursor, TRUE);
        } else {
            ClActor_SetVisible(wk->cursor, FALSE);
        }
        ClActor_SetVisible(wk->scrollBar, PMSInput_GetCategoryWordMax(wk->mwk) > 14 ? TRUE : FALSE);
    } else {
        ClActor_SetVisible(wk->cursor, visible);
        ClActor_SetVisible(wk->scrollBar, FALSE);
    }
}

void PMSIVWordWin_MoveCursor(PMSIVWordWin *wk, u32 pos) {
    ClActorPos actPos;

    if (pos != -1) {
        actPos.x = (pos & 1) * 112 + 62;
        actPos.y = (pos / 2 + 1) * WORDWIN_LINE_HEIGHT;
        ClActor_SetSurfacePos(wk->cursor, &actPos, CLACT_SURFACE_MAIN);
        ClActor_SetSequence(wk->cursor, CURSOR_ANIM);
    }
}

void PMSIVWordWin_StartScroll(PMSIVWordWin *wk, int vector) {
    int count, i;
    u8 y;
    u8 newY;
    u32 newTop;
    u32 start;
    int dy;

    PMSIVWordWin_ClearScrollArea(wk, vector);
    count = vector * 2;
    start = wk->top;
    newTop = start + count;
    dy = vector * WORDWIN_LINE_HEIGHT;
    newY = wk->y + dy;
    if (vector < 0) {
        start = newTop;
        y = newY;
        count = -count;
        vector = -vector;
    } else {
        y = wk->y + 192;
        start += WORDWIN_DRAW_WORDS;
        if (count + start > PMSInput_GetCategoryWordMax(wk->mwk)) {
            count--;
        }
    }
    for (i = 0; i < count; i++) {
        PMSIVWordWin_PrintWord(wk, start + i, y);
        if (i & 1) {
            y += WORDWIN_LINE_HEIGHT;
        }
    }
    wk->y = newY;
    wk->top = newTop;
    BmpWin_FlushChar(wk->win);
    BmpWin_FlushMap(wk->win);
    PMSIVTool_SetupScrollWork(&wk->scroll, 2, PMSIV_TOOL_SCROLL_Y, dy, vector);
}

BOOL PMSIVWordWin_WaitScroll(PMSIVWordWin *wk) {
    u16 top, scrollMax;

    if (PMSIVTool_WaitScroll(&wk->scroll)) {
        PMSInput_GetWordWinScroll(wk->mwk, &top, &scrollMax);
        PMSIVWordWin_SetScrollBar(wk, top, scrollMax);
        return TRUE;
    }
    return FALSE;
}

BOOL PMSIVWordWin_GetScrollBarPos(PMSIVWordWin *wk, ClActorPos *pos) {
    BOOL visible = ClActor_IsVisible(wk->scrollBar);

    ClActor_GetSurfacePos(wk->scrollBar, pos, CLACT_SURFACE_MAIN);
    return visible;
}

void PMSIVWordWin_SetScrollBar(PMSIVWordWin *wk, u32 top, u32 scrollMax) {
    ClActorPos pos;

    pos.x = 244;
    if (top == 0) {
        pos.y = SCROLL_BAR_TOP;
    } else if (top == scrollMax) {
        pos.y = SCROLL_BAR_BOTTOM;
    } else if (scrollMax != 0) {
        s16 start = top * SCROLL_BAR_HEIGHT / (scrollMax + 1) + SCROLL_BAR_TOP;
        s16 end = (top + 1) * SCROLL_BAR_HEIGHT / (scrollMax + 1) + SCROLL_BAR_TOP - 1;

        pos.y = (start + end) / 2;
        if (pos.y < SCROLL_BAR_TOP) {
            pos.y = SCROLL_BAR_TOP;
        } else if (pos.y > SCROLL_BAR_BOTTOM) {
            pos.y = SCROLL_BAR_BOTTOM;
        }
    } else {
        pos.y = SCROLL_BAR_TOP;
    }
    ClActor_SetSurfacePos(wk->scrollBar, &pos, CLACT_SURFACE_MAIN);
}

void PMSIVWordWin_SetScrollBarY(PMSIVWordWin *wk, u32 y) {
    ClActorPos pos;

    ClActor_GetSurfacePos(wk->scrollBar, &pos, CLACT_SURFACE_MAIN);
    pos.y = y;
    if (pos.y < SCROLL_BAR_TOP) {
        pos.y = SCROLL_BAR_TOP;
    }
    if (pos.y > SCROLL_BAR_BOTTOM) {
        pos.y = SCROLL_BAR_BOTTOM;
    }
    ClActor_SetSurfacePos(wk->scrollBar, &pos, CLACT_SURFACE_MAIN);
}

u32 PMSIVWordWin_GetScrollBarLine(PMSIVWordWin *wk, u32 scrollMax) {
    ClActorPos pos;
    u32 line;
    s16 y, i;

    PMSIVWordWin_GetScrollBarPos(wk, &pos);
    if (scrollMax != 0) {
        y = pos.y;
        if (y == SCROLL_BAR_TOP) {
            line = 0;
        } else if (y == SCROLL_BAR_BOTTOM) {
            line = scrollMax;
        } else {
            for (i = 0; i <= scrollMax; i++) {
                s16 start = i * SCROLL_BAR_HEIGHT / (scrollMax + 1) + SCROLL_BAR_TOP;
                s16 end = (i + 1) * SCROLL_BAR_HEIGHT / (scrollMax + 1) + SCROLL_BAR_TOP - 1;

                if (start > end) {
                    end = start;
                }
                if (start <= y && y <= end) {
                    line = i;
                    break;
                }
            }
        }
    } else {
        line = 0;
    }
    return line;
}

static void PMSIVWordWin_ClearScrollArea(PMSIVWordWin *wk, int vector) {
    int start, end;

    if (vector > 0) {
        start = (wk->y + 192) & 0xff;
        end = (start + vector * WORDWIN_LINE_HEIGHT) & 0xff;
    } else {
        end = wk->y;
        start = (end + vector * WORDWIN_LINE_HEIGHT) & 0xff;
    }
    if (start < end) {
        GFL_BitmapFillArea(BmpWin_GetBitmap(wk->win), 0, start, 208, end - start, 0);
    } else {
        GFL_BitmapFillArea(BmpWin_GetBitmap(wk->win), 0, start, 208, 256 - start, 0);
        GFL_BitmapFillArea(BmpWin_GetBitmap(wk->win), 0, 0, 208, end, 0);
    }
}

static void PMSIVWordWin_ResetScroll(PMSIVWordWin *wk) {
    wk->y = 0;
    wk->top = 0;
    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, 8);
}

static void PMSIVWordWin_PrintWord(PMSIVWordWin *wk, unsigned int index, u32 y) {
    Font *font = PMSIView_GetFont(wk->vwk);
    int height;
    u32 x;

    if (index < 2 || index >= PMSInput_GetCategoryWordMax(wk->mwk)) {
        return;
    }
    if (PMSInput_GetCategoryMode(wk->mwk) == 0 && PMSInput_GetCategoryCursorPos(wk->mwk) == GROUP_NUMBERS) {
        ClActorPos pos;

        pos.x = ((index - 2) & 1) * 112 + 24;
        pos.y = y - 8;
        ClActor_SetPos(wk->numberAct[index - 2], &pos);
        ClActor_SetVisible(wk->numberAct[index - 2], TRUE);
        return;
    }
    PMSInput_GetCategoryWord(wk->mwk, index - 2, wk->str);
    GFL_TextRndResetColorIndexLUT();
    if (y <= 240) {
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->win), ((index - 2) & 1) * 112, y, wk->str, font);
        return;
    }
    height = 256 - y;
    GFL_BitmapFill(BmpWin_GetBitmap(wk->tmpWin), 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->tmpWin), 0, 0, wk->str, font);
    x = ((index - 2) & 1) * 112;
    GFL_BitmapCopyArea(BmpWin_GetBitmap(wk->tmpWin), BmpWin_GetBitmap(wk->win), 0, 0, x, y, 96, height, 0xffff);
    GFL_BitmapCopyArea(BmpWin_GetBitmap(wk->tmpWin), BmpWin_GetBitmap(wk->win), 0, height, x, 0, 96, 16 - height,
                       0xffff);
}

void PMSIVWordWin_StartCursorDecide(PMSIVWordWin *wk, u32 pos) {
    ClActorPos actPos;

    wk->cursorVisible = ClActor_IsVisible(wk->cursor);
    ClActor_SetVisible(wk->cursor, TRUE);
    if (pos != -1) {
        actPos.x = (pos & 1) * 112 + 62;
        actPos.y = (pos / 2 + 1) * WORDWIN_LINE_HEIGHT;
        ClActor_SetSurfacePos(wk->cursor, &actPos, CLACT_SURFACE_MAIN);
        ClActor_SetSequence(wk->cursor, CURSOR_ANIM_DECIDE);
    }
}

BOOL PMSIVWordWin_WaitCursorDecide(PMSIVWordWin *wk) {
    if (ClActor_GetSequence(wk->cursor) != CURSOR_ANIM_DECIDE) {
        return TRUE;
    }
    if (!ClActor_IsAnimActive(wk->cursor)) {
        ClActor_SetVisible(wk->cursor, wk->cursorVisible);
        ClActor_SetSequence(wk->cursor, CURSOR_ANIM);
        wk->cursorVisible = FALSE;
        return TRUE;
    }
    return FALSE;
}
