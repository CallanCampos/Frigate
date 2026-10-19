#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "app/zknsearch_bmp.h"
#include "app/zknsearch_list.h"
#include "app/zknsearch_main.h"
#include "app/zknsearch_obj.h"
#include "app/zknsearch_seq.h"
#include "app/zknsearch_ui.h"
#include "app/zukan_common.h"
#include "app/zukan_search_engine.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/pokedex.h"
#include "system/blink_palanm.h"
#include "system/cursor_move.h"
#include "system/game_data.h"
#include "system/palanm.h"

// The sequences of the Pokédex's search screen: the main page with the criteria, a page with a list for each
// criterion, and the search. The ROM doesn't name this file; zknsearch_seq.c is a guess after zknsearch_main.c

enum {
    SEQ_INIT,
    SEQ_EXIT,
    SEQ_WAIT_FADE,
    SEQ_MAIN_INIT,
    SEQ_MAIN,
    SEQ_PAGE_END,
    SEQ_RESET,
    SEQ_SEARCH,
    SEQ_SEARCH_RESULT,
    SEQ_ORDER_INIT,
    SEQ_ORDER,
    SEQ_ORDER_EXIT,
    SEQ_INITIAL_INIT,
    SEQ_INITIAL,
    SEQ_INITIAL_EXIT,
    SEQ_TYPE_INIT,
    SEQ_TYPE,
    SEQ_TYPE_EXIT,
    SEQ_COLOR_INIT,
    SEQ_COLOR,
    SEQ_COLOR_EXIT,
    SEQ_SHAPE_INIT,
    SEQ_SHAPE,
    SEQ_SHAPE_EXIT,
    SEQ_LIST_SCROLL,
    SEQ_BUTTON_ANM,
    SEQ_FINISH,
    SEQ_END,
};

// How long the search takes at least, and when its sound stops
#define SEARCH_FRAMES 121
#define SEARCH_SOUND_FRAMES 100

// No button to wait for while the list jumps
#define BUTTON_NONE ZKNSEARCH_OBJ_COUNT

typedef int (*ZknSearchSeqFunc)(ZknSearchWork *wk);

static int ZknSearchSeq_Init(ZknSearchWork *wk);
static int ZknSearchSeq_Exit(ZknSearchWork *wk);
static int ZknSearchSeq_WaitFade(ZknSearchWork *wk);
static int ZknSearchSeq_MainInit(ZknSearchWork *wk);
static int ZknSearchSeq_MainPage(ZknSearchWork *wk);
static int ZknSearchSeq_PageEnd(ZknSearchWork *wk);
static int ZknSearchSeq_Reset(ZknSearchWork *wk);
static int ZknSearchSeq_Search(ZknSearchWork *wk);
static int ZknSearchSeq_SearchResult(ZknSearchWork *wk);
static int ZknSearchSeq_OrderInit(ZknSearchWork *wk);
static int ZknSearchSeq_Order(ZknSearchWork *wk);
static int ZknSearchSeq_OrderExit(ZknSearchWork *wk);
static int ZknSearchSeq_InitialInit(ZknSearchWork *wk);
static int ZknSearchSeq_Initial(ZknSearchWork *wk);
static int ZknSearchSeq_InitialExit(ZknSearchWork *wk);
static int ZknSearchSeq_TypeInit(ZknSearchWork *wk);
static int ZknSearchSeq_Type(ZknSearchWork *wk);
static int ZknSearchSeq_TypeExit(ZknSearchWork *wk);
static int ZknSearchSeq_ColorInit(ZknSearchWork *wk);
static int ZknSearchSeq_Color(ZknSearchWork *wk);
static int ZknSearchSeq_ColorExit(ZknSearchWork *wk);
static int ZknSearchSeq_ShapeInit(ZknSearchWork *wk);
static int ZknSearchSeq_Shape(ZknSearchWork *wk);
static int ZknSearchSeq_ShapeExit(ZknSearchWork *wk);
static int ZknSearchSeq_ListScroll(ZknSearchWork *wk);
static int ZknSearchSeq_ButtonAnm(ZknSearchWork *wk);
static int ZknSearchSeq_Finish(ZknSearchWork *wk);
static void ZknSearchSeq_SetOrder(ZknSearchWork *wk, u32 row);
static void ZknSearchSeq_ResetOrder(ZknSearchWork *wk);
static void ZknSearchSeq_SetInitial(ZknSearchWork *wk, u32 row);
static void ZknSearchSeq_ResetInitial(ZknSearchWork *wk);
static void ZknSearchSeq_SetType(ZknSearchWork *wk, u32 row);
static void ZknSearchSeq_ResetType(ZknSearchWork *wk);
static void ZknSearchSeq_SetColor(ZknSearchWork *wk, u32 row);
static void ZknSearchSeq_ResetColor(ZknSearchWork *wk);
static void ZknSearchSeq_SetShape(ZknSearchWork *wk, u32 row);
static void ZknSearchSeq_ResetShape(ZknSearchWork *wk);
static int ZknSearchSeq_SetButtonAnm(ZknSearchWork *wk, u32 isActor, u32 id, int next);
static int ZknSearchSeq_SetPageEnd(ZknSearchWork *wk, int next);
static void ZknSearchSeq_ToggleShortcut(ZknSearchWork *wk);

static const ZknSearchSeqFunc sZknSearchSeqFuncs[] = {
    ZknSearchSeq_Init,         ZknSearchSeq_Exit,      ZknSearchSeq_WaitFade,    ZknSearchSeq_MainInit,
    ZknSearchSeq_MainPage,     ZknSearchSeq_PageEnd,   ZknSearchSeq_Reset,       ZknSearchSeq_Search,
    ZknSearchSeq_SearchResult, ZknSearchSeq_OrderInit, ZknSearchSeq_Order,       ZknSearchSeq_OrderExit,
    ZknSearchSeq_InitialInit,  ZknSearchSeq_Initial,   ZknSearchSeq_InitialExit, ZknSearchSeq_TypeInit,
    ZknSearchSeq_Type,         ZknSearchSeq_TypeExit,  ZknSearchSeq_ColorInit,   ZknSearchSeq_Color,
    ZknSearchSeq_ColorExit,    ZknSearchSeq_ShapeInit, ZknSearchSeq_Shape,       ZknSearchSeq_ShapeExit,
    ZknSearchSeq_ListScroll,   ZknSearchSeq_ButtonAnm, ZknSearchSeq_Finish,
};

BOOL ZknSearchSeq_Main(ZknSearchWork *wk) {
    wk->seq = sZknSearchSeqFuncs[wk->seq](wk);
    if (wk->seq == SEQ_END) {
        return FALSE;
    }
    ZknSearchObj_Update(wk);
    ZknSearchBmp_Update(wk);
    ZukanCommon_ScrollBGs(2, 6, &wk->scrollTimer);
    return TRUE;
}

static int ZknSearchSeq_Init(ZknSearchWork *wk) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
    if (PokeDex_GetMode(wk->param->pokedex) == 1) {
        wk->param->criteria->regional = 0;
    } else {
        wk->param->criteria->regional = 1;
    }
    ZknSearchMain_InitVRAM();
    ZknSearchMain_InitBG();
    ZknSearchMain_LoadGraphics();
    ZknSearchMain_InitMsg(wk);
    ZknSearchBmp_Init(wk);
    ZknSearchObj_Init(wk);
    ZknSearchMain_InitBlinkAnm(wk);
    GFL_NetSetIconScreen(TRUE, HEAPID_ZUKAN_SEARCH);
    ZknSearchMain_InitBGWinFrame(wk);
    ZknSearchMain_InitFramePos(wk);
    ZknSearchMain_InitPaletteFade(wk);
    ZknSearchMain_StartFade(wk, 16, 16);
    ZknSearchMain_LoadLoadingScreen(wk);
    ZknSearchMain_InitWindow();
    ZknSearchMain_InitVBlank(wk);
    ZknSearchMain_InitHBlank(wk);
    wk->page = ZKNSEARCH_NONE;
    ZknSearchMain_ShapeToListPos(wk);
    return SEQ_MAIN_INIT;
}

static int ZknSearchSeq_Exit(ZknSearchWork *wk) {
    if (ZknSearchMain_UpdateFrameMove(wk) == TRUE || PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_EXIT;
    }
    ZknSearchMain_ExitHBlank(wk);
    ZknSearchMain_ExitVBlank(wk);
    ZknSearchMain_ExitWindow();
    ZknSearchMain_FreeLoadingScreen(wk);
    ZknSearchMain_ExitBGWinFrame(wk);
    ZknSearchMain_ExitPaletteFade(wk);
    ZknSearchMain_ExitBlinkAnm(wk);
    ZknSearchObj_Exit(wk);
    ZknSearchBmp_Exit(wk);
    ZknSearchMain_ExitMsg(wk);
    ZknSearchMain_ExitBG();
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return SEQ_END;
}

static int ZknSearchSeq_WaitFade(ZknSearchWork *wk) {
    if (PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
        return wk->buttonNextSeq;
    }
    return SEQ_WAIT_FADE;
}

static int ZknSearchSeq_MainInit(ZknSearchWork *wk) {
    if (PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_MAIN_INIT;
    }
    ZknSearchMain_SetBlend(TRUE);
    ZknSearchMain_LoadMainScreen(wk);
    ZknSearchBmp_PutMain(wk);
    ZknSearchObj_SetMain(wk);
    ZknSearchMain_SetMainBGPriority();
    ZknSearchMain_RequestBGOn(wk);
    if (wk->page == ZKNSEARCH_NONE) {
        ZknSearchUI_InitCursor(wk, ZKNSEARCH_UI_ORDER);
        GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, 0);
        GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, 0);
        ZknSearchMain_StartFrameMove(wk, -8);
        ZknSearchMain_StartFade(wk, 16, 0);
        return SEQ_MAIN;
    }
    ZknSearchUI_InitCursor(wk, wk->page);
    return SEQ_MAIN;
}

static int ZknSearchSeq_MainPage(ZknSearchWork *wk) {
    if (ZknSearchMain_UpdateFrameMove(wk) == TRUE || PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_MAIN;
    }
    switch (ZknSearchUI_Main(wk)) {
    case ZKNSEARCH_UI_ORDER:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_ORDER, ZknSearchSeq_SetPageEnd(wk, SEQ_ORDER_INIT));
    case ZKNSEARCH_UI_INITIAL:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_INITIAL,
                                         ZknSearchSeq_SetPageEnd(wk, SEQ_INITIAL_INIT));
    case ZKNSEARCH_UI_TYPE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_TYPE, ZknSearchSeq_SetPageEnd(wk, SEQ_TYPE_INIT));
    case ZKNSEARCH_UI_COLOR:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_COLOR, ZknSearchSeq_SetPageEnd(wk, SEQ_COLOR_INIT));
    case ZKNSEARCH_UI_SHAPE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_SHAPE, ZknSearchSeq_SetPageEnd(wk, SEQ_SHAPE_INIT));
    case ZKNSEARCH_UI_START:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_START, SEQ_SEARCH);
    case ZKNSEARCH_UI_RESET:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return ZknSearchSeq_SetButtonAnm(wk, FALSE, ZKNSEARCH_UI_RESET, SEQ_RESET);
    case ZKNSEARCH_UI_SHORTCUT:
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        ZknSearchSeq_ToggleShortcut(wk);
        break;
    case ZKNSEARCH_UI_EXIT:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->param->result = ZUKAN_SEARCH_RESULT_CLOSE;
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_EXIT, 8);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_EXIT, SEQ_FINISH);
    case ZKNSEARCH_UI_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ZknSearchMain_ListPosToShape(wk);
        wk->param->result = ZUKAN_SEARCH_RESULT_CANCEL;
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_FINISH);
    case CURSOR_MOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_MOVED:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ZknSearchMain_ListPosToShape(wk);
        wk->param->result = ZUKAN_SEARCH_RESULT_CANCEL;
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_FINISH);
    }
    BlinkPalAnm_Main(wk->blinkAnm);
    return SEQ_MAIN;
}

static int ZknSearchSeq_PageEnd(ZknSearchWork *wk) {
    ZknSearchMain_ClearPage(wk);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, FALSE);
    ZknSearchUI_ExitCursor(wk);
    return wk->nextSeq;
}

static int ZknSearchSeq_Reset(ZknSearchWork *wk) {
    ZukanCommon_InitCriteria(wk->param->pokedex, wk->param->criteria);
    ZknSearchBmp_PutMain(wk);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, FALSE);
    return SEQ_MAIN;
}

// Searches a step a frame while the progress bar fills, which takes longer
static int ZknSearchSeq_Search(ZknSearchWork *wk) {
    u16 count;
    u16 *list;

    if (wk->timer == 0) {
        GFL_SndSEPlay(SEQ_SE_SYS_78);
        ZknSearchMain_ShowLoading(wk);
        ZknSearchBmp_PutSearching(wk);
        ZknSearchBmp_ClearInfo(wk);
    }
    if (wk->timer == SEARCH_SOUND_FRAMES) {
        GFL_SndStop();
    }
    if (wk->timer == 1) {
        ZknSearchMain_ListPosToShape(wk);
        wk->engine =
            ZukanSearchEngine_Create(wk->param->pokedex, wk->param->criteria, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
        wk->searchDone = FALSE;
    }
    if (wk->timer >= 2 && wk->searchDone == FALSE) {
        wk->searchDone = ZukanSearchEngine_Step(wk->engine, HEAPID_ZUKAN, &count, &list);
        if (wk->searchDone == TRUE) {
            wk->param->count = count;
            wk->param->list = list;
            ZukanSearchEngine_Free(wk->engine);
        }
    }
    if (wk->timer == SEARCH_FRAMES) {
        if (wk->searchDone == TRUE) {
            if (wk->param->count != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_84);
                ZknSearchBmp_PutFound(wk);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                ZknSearchBmp_PutNotFound(wk);
            }
            wk->timer = 0;
            return SEQ_SEARCH_RESULT;
        }
    } else {
        ZknSearchObj_MoveLoadingBar(wk, wk->timer);
        wk->timer++;
    }
    return SEQ_SEARCH;
}

static int ZknSearchSeq_SearchResult(ZknSearchWork *wk) {
    if (ZknSearchUI_WaitResult(wk) == TRUE) {
        if (wk->param->count != 0) {
            wk->param->result = ZUKAN_SEARCH_RESULT_FOUND;
            return SEQ_FINISH;
        }
        ZknSearchMain_HideLoading(wk);
        ZknSearchBmp_PutInfo(wk);
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, FALSE);
        } else {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        ZknSearchMain_ShapeToListPos(wk);
        return SEQ_MAIN;
    }
    return SEQ_SEARCH_RESULT;
}

static int ZknSearchSeq_OrderInit(ZknSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = ZKNSEARCH_PAGE_ORDER;
        ZknSearchMain_SetBlend(TRUE);
        ZknSearchBmp_PutOrder(wk);
        ZknSearchList_InitOrder(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            ZknSearchObj_SetOrder(wk);
            ZknSearchMain_LoadOrderScreen(wk);
            ZknSearchMain_SetListBGPriority();
            ZknSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_ORDER;
        }
        break;
    }
    return SEQ_ORDER_INIT;
}

static int ZknSearchSeq_Order(ZknSearchWork *wk) {
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_SetOrder(wk, ret);
        }
        break;
    case FRAMELIST_NONE:
        ret = ZknSearchUI_ListMain(wk);
        switch (ret) {
        case ZKNSEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_ORDER_EXIT);
        case ZKNSEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_ORDER_EXIT);
        case ZKNSEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_ResetOrder(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_SetOrder(wk, ret - ZKNSEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    return SEQ_ORDER;
}

static int ZknSearchSeq_OrderExit(ZknSearchWork *wk) {
    ZknSearchMain_ClearPage(wk);
    ZknSearchObj_HideList(wk);
    ZknSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int ZknSearchSeq_InitialInit(ZknSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = ZKNSEARCH_PAGE_INITIAL;
        ZknSearchMain_SetBlend(TRUE);
        ZknSearchBmp_PutInitial(wk);
        ZknSearchList_InitInitial(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchObj_SetInitial(wk);
            ZknSearchMain_LoadInitialScreen(wk);
            ZknSearchMain_SetListBGPriority();
            ZknSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_INITIAL;
        }
        break;
    }
    return SEQ_INITIAL_INIT;
}

static int ZknSearchSeq_Initial(ZknSearchWork *wk) {
    int seq = SEQ_INITIAL;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_SetInitial(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        ZknSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_UP, seq);
    case -4:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        ZknSearchObj_HideMarks(wk);
        ZknSearchObj_PutItemMark(wk, 0, wk->param->criteria->initial);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = ZknSearchUI_ListMain(wk);
        switch (ret) {
        case ZKNSEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_INITIAL_EXIT);
        case ZKNSEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_INITIAL_EXIT);
        case ZKNSEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_ResetInitial(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchSeq_SetInitial(wk, ret - ZKNSEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    ZknSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int ZknSearchSeq_InitialExit(ZknSearchWork *wk) {
    ZknSearchMain_ClearPage(wk);
    ZknSearchObj_HideList(wk);
    ZknSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int ZknSearchSeq_TypeInit(ZknSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = ZKNSEARCH_PAGE_TYPE;
        ZknSearchMain_SetBlend(TRUE);
        ZknSearchBmp_PutType(wk);
        ZknSearchList_InitType(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchObj_SetType(wk);
            ZknSearchMain_LoadTypeScreen(wk);
            ZknSearchMain_SetListBGPriority();
            ZknSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_TYPE;
        }
        break;
    }
    return SEQ_TYPE_INIT;
}

static int ZknSearchSeq_Type(ZknSearchWork *wk) {
    int seq = SEQ_TYPE;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_SetType(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        ZknSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_UP, seq);
    case -4:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        ZknSearchObj_HideMarks(wk);
        ZknSearchObj_PutItemMark(wk, 0, ZknSearchMain_TypeToListPos(wk, wk->param->criteria->type1));
        ZknSearchObj_PutItemMark(wk, 1, ZknSearchMain_TypeToListPos(wk, wk->param->criteria->type2));
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = ZknSearchUI_ListMain(wk);
        switch (ret) {
        case ZKNSEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_TYPE_EXIT);
        case ZKNSEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_TYPE_EXIT);
        case ZKNSEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_ResetType(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchSeq_SetType(wk, ret - ZKNSEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    ZknSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int ZknSearchSeq_TypeExit(ZknSearchWork *wk) {
    ZknSearchMain_ClearPage(wk);
    ZknSearchObj_HideList(wk);
    ZknSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int ZknSearchSeq_ColorInit(ZknSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = ZKNSEARCH_PAGE_COLOR;
        ZknSearchMain_SetBlend(TRUE);
        ZknSearchBmp_PutColor(wk);
        ZknSearchList_InitColor(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchObj_SetColor(wk);
            ZknSearchMain_LoadColorScreen(wk);
            ZknSearchMain_SetListBGPriority();
            ZknSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_COLOR;
        }
        break;
    }
    return SEQ_COLOR_INIT;
}

static int ZknSearchSeq_Color(ZknSearchWork *wk) {
    int seq = SEQ_COLOR;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_SetColor(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        ZknSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_UP, seq);
    case -4:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        ZknSearchObj_HideMarks(wk);
        ZknSearchObj_PutItemMark(wk, 0, wk->param->criteria->color);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = ZknSearchUI_ListMain(wk);
        switch (ret) {
        case ZKNSEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_COLOR_EXIT);
        case ZKNSEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_COLOR_EXIT);
        case ZKNSEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_ResetColor(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchSeq_SetColor(wk, ret - ZKNSEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    ZknSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int ZknSearchSeq_ColorExit(ZknSearchWork *wk) {
    ZknSearchMain_ClearPage(wk);
    ZknSearchObj_HideList(wk);
    ZknSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int ZknSearchSeq_ShapeInit(ZknSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = ZKNSEARCH_PAGE_SHAPE;
        ZknSearchMain_SetBlend(TRUE);
        ZknSearchBmp_PutShape(wk);
        ZknSearchList_InitShape(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchObj_SetShape(wk);
            ZknSearchMain_LoadShapeScreen(wk);
            ZknSearchMain_SetListBGPriority();
            ZknSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_SHAPE;
        }
        break;
    }
    return SEQ_SHAPE_INIT;
}

static int ZknSearchSeq_Shape(ZknSearchWork *wk) {
    int seq = SEQ_SHAPE;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_SetShape(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        ZknSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = ZKNSEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 12);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_UP, seq);
    case -4:
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 13);
        return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        ZknSearchObj_PutShapeIcons(wk);
        ZknSearchObj_HideMarks(wk);
        ZknSearchObj_PutShapeItemMark(wk);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = ZknSearchUI_ShapeListMain(wk);
        switch (ret) {
        case ZKNSEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_SHAPE_EXIT);
        case ZKNSEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 9);
            return ZknSearchSeq_SetButtonAnm(wk, TRUE, ZKNSEARCH_OBJ_RETURN, SEQ_SHAPE_EXIT);
        case ZKNSEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchSeq_ResetShape(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknSearchObj_SetPageButtons(wk, FALSE);
            ZknSearchSeq_SetShape(wk, ret - ZKNSEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    ZknSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int ZknSearchSeq_ShapeExit(ZknSearchWork *wk) {
    ZknSearchMain_ClearPage(wk);
    ZknSearchObj_HideList(wk);
    ZknSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

// Waits for the list's jump or page scroll, and for the page button's animation
static int ZknSearchSeq_ListScroll(ZknSearchWork *wk) {
    switch (wk->subSeq) {
    case 0:
        if (FrameList_Main(wk->frameList) == FRAMELIST_NONE) {
            wk->subSeq++;
        }
        break;
    case 1:
        if (wk->buttonId == BUTTON_NONE || ZknSearchObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
            wk->subSeq = 0;
            ZknSearchObj_SetPageButtons(wk, FALSE);
            return wk->nextSeq;
        }
        break;
    }
    return SEQ_LIST_SCROLL;
}

// Flashes the cursor's position twice, or waits for a button's animation
static int ZknSearchSeq_ButtonAnm(ZknSearchWork *wk) {
    if (wk->buttonIsActor == FALSE) {
        switch (wk->buttonSeq) {
        case 0:
        case 2:
            if (wk->buttonTimer == 0) {
                ZknSearchUI_SetCursorPalette(wk, wk->buttonId, ZKNSEARCH_UI_PALETTE_OFF);
                wk->buttonTimer = 4;
                wk->buttonSeq++;
            } else {
                wk->buttonTimer--;
            }
            break;
        case 1:
        case 3:
            if (wk->buttonTimer == 0) {
                ZknSearchUI_SetCursorPalette(wk, wk->buttonId, ZKNSEARCH_UI_PALETTE_PRESSED);
                wk->buttonTimer = 4;
                wk->buttonSeq++;
            } else {
                wk->buttonTimer--;
            }
            break;
        case 4:
            if (GCTX_HIDIsUsingTouch() == TRUE) {
                ZknSearchUI_SetCursorPalette(wk, wk->buttonId, ZKNSEARCH_UI_PALETTE_OFF);
            } else {
                ZknSearchUI_SetCursorPalette(wk, wk->buttonId, ZKNSEARCH_UI_PALETTE_ON);
            }
            return wk->buttonNextSeq;
        }
    } else {
        if (ZknSearchObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
            if (wk->frameList != NULL) {
                ZknSearchObj_SetPageButtons(wk, FALSE);
            }
            return wk->buttonNextSeq;
        }
    }
    return SEQ_BUTTON_ANM;
}

static int ZknSearchSeq_Finish(ZknSearchWork *wk) {
    ZknSearchUI_ExitCursor(wk);
    ZknSearchMain_StartFrameMove(wk, 8);
    ZknSearchMain_StartFade(wk, 0, 16);
    return SEQ_EXIT;
}

static void ZknSearchSeq_SetOrder(ZknSearchWork *wk, u32 row) {
    wk->param->criteria->order = row;
    FrameList_SetCursor(wk->frameList, row);
    ZknSearchBmp_PutOrderValue(wk);
    ZknSearchObj_PutRowMark(wk, row, TRUE);
}

static void ZknSearchSeq_ResetOrder(ZknSearchWork *wk) {
    wk->param->criteria->order = 0;
    ZknSearchBmp_PutOrderValue(wk);
    ZknSearchObj_PutRowMark(wk, 0, TRUE);
}

// Chooses the row's initial, or none if it was the one chosen
static void ZknSearchSeq_SetInitial(ZknSearchWork *wk, u32 row) {
    u32 item = row + FrameList_GetScroll(wk->frameList);

    if (wk->param->criteria->initial == item) {
        wk->param->criteria->initial = ZKNSEARCH_NONE;
        ZknSearchObj_PutRowMark(wk, row, FALSE);
    } else {
        wk->param->criteria->initial = item;
        ZknSearchObj_PutRowMark(wk, row, TRUE);
    }
    FrameList_SetCursor(wk->frameList, row);
    ZknSearchBmp_PutInitialValue(wk);
}

static void ZknSearchSeq_ResetInitial(ZknSearchWork *wk) {
    wk->param->criteria->initial = ZKNSEARCH_NONE;
    ZknSearchBmp_PutInitialValue(wk);
    ZknSearchObj_HideMarks(wk);
}

// Chooses the row's type as the first or the second, dropping the first for a third, or drops it if it was chosen
static void ZknSearchSeq_SetType(ZknSearchWork *wk, u32 row) {
    u32 type = ZknSearchMain_ListPosToType(wk, row + FrameList_GetScroll(wk->frameList));

    if (wk->param->criteria->type1 == ZKNSEARCH_NONE) {
        wk->param->criteria->type1 = type;
    } else if (wk->param->criteria->type2 == ZKNSEARCH_NONE) {
        if (wk->param->criteria->type1 == type) {
            wk->param->criteria->type1 = ZKNSEARCH_NONE;
        } else {
            wk->param->criteria->type2 = type;
        }
    } else if (wk->param->criteria->type1 == type) {
        wk->param->criteria->type1 = wk->param->criteria->type2;
        wk->param->criteria->type2 = ZKNSEARCH_NONE;
    } else if (wk->param->criteria->type2 == type) {
        wk->param->criteria->type2 = ZKNSEARCH_NONE;
    } else {
        wk->param->criteria->type1 = wk->param->criteria->type2;
        wk->param->criteria->type2 = type;
    }
    FrameList_SetCursor(wk->frameList, row);
    ZknSearchBmp_PutTypeValue(wk);
    ZknSearchObj_PutTypeMarks(wk, wk->param->criteria->type1, wk->param->criteria->type2);
}

static void ZknSearchSeq_ResetType(ZknSearchWork *wk) {
    wk->param->criteria->type1 = ZKNSEARCH_NONE;
    wk->param->criteria->type2 = ZKNSEARCH_NONE;
    ZknSearchBmp_PutTypeValue(wk);
    ZknSearchObj_HideMarks(wk);
}

static void ZknSearchSeq_SetColor(ZknSearchWork *wk, u32 row) {
    u32 item = row + FrameList_GetScroll(wk->frameList);

    if (wk->param->criteria->color == item) {
        wk->param->criteria->color = ZKNSEARCH_NONE;
        ZknSearchObj_PutRowMark(wk, row, FALSE);
    } else {
        wk->param->criteria->color = item;
        ZknSearchObj_PutRowMark(wk, row, TRUE);
    }
    FrameList_SetCursor(wk->frameList, row);
    ZknSearchBmp_PutColorValue(wk);
}

static void ZknSearchSeq_ResetColor(ZknSearchWork *wk) {
    wk->param->criteria->color = ZKNSEARCH_NONE;
    ZknSearchBmp_PutColorValue(wk);
    ZknSearchObj_HideMarks(wk);
}

static void ZknSearchSeq_SetShape(ZknSearchWork *wk, u32 row) {
    u32 item = row + FrameList_GetScroll(wk->frameList);

    if (wk->param->criteria->shape == item) {
        wk->param->criteria->shape = ZKNSEARCH_NONE;
        ZknSearchObj_PutShapeRowMark(wk, row, FALSE);
    } else {
        wk->param->criteria->shape = item;
        ZknSearchObj_PutShapeRowMark(wk, row, TRUE);
    }
    FrameList_SetCursor(wk->frameList, row);
    ZknSearchObj_PutShape(wk);
}

static void ZknSearchSeq_ResetShape(ZknSearchWork *wk) {
    wk->param->criteria->shape = ZKNSEARCH_NONE;
    ZknSearchObj_PutShape(wk);
    ZknSearchObj_HideMarks(wk);
}

static int ZknSearchSeq_SetButtonAnm(ZknSearchWork *wk, u32 isActor, u32 id, int next) {
    wk->buttonIsActor = isActor;
    wk->buttonId = id;
    wk->buttonSeq = 0;
    wk->buttonTimer = 0;
    wk->buttonNextSeq = next;
    return SEQ_BUTTON_ANM;
}

static int ZknSearchSeq_SetPageEnd(ZknSearchWork *wk, int next) {
    wk->initSeq = 0;
    wk->nextSeq = next;
    return SEQ_PAGE_END;
}

static void ZknSearchSeq_ToggleShortcut(ZknSearchWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH, FALSE);
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH, TRUE);
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 7);
    }
}
