#include "types.h"
#include "app/research_radar/bg_font.h"
#include "app/research_radar/palette_anime.h"
#include "app/research_radar/queue.h"
#include "app/research_radar/research_common.h"
#include "app/research_radar/research_top.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "save/event_work.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/gf_font.h"

// The Research Radar's top screen, with a button for the list of surveys and one for the graph of the survey being
// run. The list's button is disabled until a second survey is unlocked. It is used by touch or with a cursor, and the
// screen it goes on to starts in the same mode

// The sequences of the screen, which run one after another from a queue
enum {
    SEQ_SETUP,
    SEQ_TOUCH,
    SEQ_CURSOR,
    SEQ_WAIT,
    SEQ_FADE_OUT,
    SEQ_TEARDOWN,
    SEQ_END,
};

// The buttons, in the order of the cursor
enum {
    BUTTON_LIST,
    BUTTON_GRAPH,
    BUTTON_COUNT,
};

// The BGs: the buttons on the main engine's BG 2 and their labels on BG 3, and the sub engine's title on BG 6 and its
// text on BG 7, in front of the proc's BGs
#define BG_MAIN_BUTTONS 2
#define BG_MAIN_TEXT 3
#define BG_SUB_TITLE 6
#define BG_SUB_TEXT 7

// The objects' resources, of the main engine and then of the sub engine
enum {
    OBJ_RES_MAIN_CHARS,
    OBJ_RES_MAIN_PALETTE,
    OBJ_RES_MAIN_CELL_ANIMS,
    OBJ_RES_SUB_CHARS,
    OBJ_RES_SUB_PALETTE,
    OBJ_RES_SUB_CELL_ANIMS,
    OBJ_RES_COUNT,
};

#define UNIT_COUNT 2
#define ACTOR_COUNT 1
#define MSG_DATA_COUNT 2
#define BG_FONT_COUNT 4
#define BUTTON_RECT_COUNT 3

// The palette animations: the cursor's button, a cursor move, a button pressed, and the list's disabled button
enum {
    ANIME_CURSOR,
    ANIME_MOVE,
    ANIME_DECIDE,
    ANIME_DISABLED_1,
    ANIME_DISABLED_2,
    ANIME_COUNT,
};

// The frames a pressed button flashes before the screen fades out
#define DECIDE_WAIT_FRAMES 15

// The first survey is always there, and each of these event flags unlocks one more
#define FIRST_SURVEY_FLAG 0xbf
#define LAST_SURVEY_FLAG 0xc7

struct ResearchTop {
    ResearchCommon *common;
    HeapID heapId;
    Font *font;
    MsgData *msgData[MSG_DATA_COUNT];
    Queue *queue;
    u32 seq;
    u32 seqState;
    u32 seqFrames;
    u32 waitFrames;
    int cursor;
    TouchRect buttonRects[BUTTON_RECT_COUNT];
    PaletteAnime *animes[ANIME_COUNT];
    BGFont *bgFonts[BG_FONT_COUNT];
    u32 objRes[OBJ_RES_COUNT];
    ClActUnit *units[UNIT_COUNT];
    ClActor *actors[ACTOR_COUNT];
    BOOL seqDone;
    // Whether the icon of a new survey result is shown
    BOOL newIconShown;
    BOOL isEnd;
    u32 next;
};

typedef struct {
    BGFontWindow window;
    u32 msgDataIndex;
    u32 strId;
    BOOL centered;
} BGFontEntry;

static void ResearchTop_SeqSetup(ResearchTop *wk);
static void ResearchTop_SeqTouch(ResearchTop *wk);
static void ResearchTop_SeqCursor(ResearchTop *wk);
static void ResearchTop_SeqWait(ResearchTop *wk);
static void ResearchTop_SeqFadeOut(ResearchTop *wk);
static void ResearchTop_SeqTeardown(ResearchTop *wk);
static void ResearchTop_PushSeq(ResearchTop *wk, u32 seq);
static void ResearchTop_EndSeq(ResearchTop *wk);
static void ResearchTop_UpdateSeq(ResearchTop *wk);
static void ResearchTop_SetSeq(ResearchTop *wk, u32 seq);
static u32 ResearchTop_GetSeqState(ResearchTop *wk);
static void ResearchTop_NextSeqState(ResearchTop *wk);
static u32 ResearchTop_GetStartSeq(ResearchTop *wk);
static void ResearchTop_SetTouchMode(ResearchTop *wk, BOOL touch);
static void ResearchTop_SetNext(ResearchTop *wk, u32 next);
static void ResearchTop_CheckNewResult(ResearchTop *wk);
static void ResearchTop_CursorUp(ResearchTop *wk);
static void ResearchTop_CursorDown(ResearchTop *wk);
static void ResearchTop_MoveCursorTo(ResearchTop *wk, int pos);
static void ResearchTop_HighlightButton(int button);
static void ResearchTop_UnhighlightButton(int button);
static void ResearchTop_DisableListButton(ResearchTop *wk);
static void ResearchTop_StartReturnAnime(ResearchTop *wk);
static void ResearchTop_ShowNewIcon(ResearchTop *wk);
static void ResearchTop_FadeIn(void);
static void ResearchTop_FadeOut(void);
static void ResearchTop_CountFrame(ResearchTop *wk);
static u32 ResearchTop_GetSeqFrames(ResearchTop *wk);
static void ResearchTop_SetWait(ResearchTop *wk, u32 frames);
static u32 ResearchTop_GetWait(ResearchTop *wk);
static void ResearchTop_MoveCursor(ResearchTop *wk, int dir);
static void ResearchTop_SetCursor(ResearchTop *wk, int pos);
static BOOL ResearchTop_IsButtonEnabled(ResearchTop *wk, int button);
static u32 ResearchTop_GetObjRes(ResearchTop *wk, u32 index);
static ClActUnit *ResearchTop_GetUnit(ResearchTop *wk, u32 index);
static ClActor *ResearchTop_GetActor(ResearchTop *wk, u32 index);
static void ResearchTop_StartPaletteAnime(ResearchTop *wk, u32 index);
static void ResearchTop_StopPaletteAnime(ResearchTop *wk, u32 index);
static void ResearchTop_UpdatePaletteAnimes(ResearchTop *wk);
static u8 ResearchTop_CountSurveys(ResearchTop *wk);
static ResearchCommon *ResearchTop_GetCommon(ResearchTop *wk);
static void ResearchTop_SetCommon(ResearchTop *wk, ResearchCommon *common);
static void ResearchTop_SetHeapID(ResearchTop *wk, HeapID heapId);
static BOOL ResearchTop_IsForceExit(ResearchTop *wk);
static ResearchTop *ResearchTop_Alloc(HeapID heapId);
static void ResearchTop_Free(ResearchTop *wk);
static void ResearchTop_InitWork(ResearchTop *wk);
static void ResearchTop_InitCursor(ResearchTop *wk);
static void ResearchTop_ClearFont(ResearchTop *wk);
static void ResearchTop_CreateFont(ResearchTop *wk);
static void ResearchTop_DeleteFont(ResearchTop *wk);
static void ResearchTop_ClearMsgData(ResearchTop *wk);
static void ResearchTop_LoadMsgData(ResearchTop *wk);
static void ResearchTop_FreeMsgData(ResearchTop *wk);
static void ResearchTop_InitButtonRects(ResearchTop *wk);
static void ResearchTop_ClearQueue(ResearchTop *wk);
static void ResearchTop_CreateQueue(ResearchTop *wk);
static void ResearchTop_DeleteQueue(ResearchTop *wk);
static void ResearchTop_InitBG(ResearchTop *wk);
static void ResearchTop_ExitBG(ResearchTop *wk);
static void ResearchTop_LoadSubBG(ResearchTop *wk);
static void ResearchTop_UnloadSubBG(ResearchTop *wk);
static void ResearchTop_InitSubText(ResearchTop *wk);
static void ResearchTop_ExitSubText(ResearchTop *wk);
static void ResearchTop_LoadMainBG(ResearchTop *wk);
static void ResearchTop_UnloadMainBG(ResearchTop *wk);
static void ResearchTop_InitMainText(ResearchTop *wk);
static void ResearchTop_ExitMainText(ResearchTop *wk);
static void ResearchTop_ClearBGFonts(ResearchTop *wk);
static void ResearchTop_CreateBGFonts(ResearchTop *wk);
static void ResearchTop_DeleteBGFonts(ResearchTop *wk);
static void ResearchTop_ClearObjRes(ResearchTop *wk);
static void ResearchTop_LoadSubObjRes(ResearchTop *wk);
static void ResearchTop_FreeSubObjRes(ResearchTop *wk);
static void ResearchTop_LoadMainObjRes(ResearchTop *wk);
static void ResearchTop_FreeMainObjRes(ResearchTop *wk);
static void ResearchTop_ClearUnits(ResearchTop *wk);
static void ResearchTop_CreateUnits(ResearchTop *wk);
static void ResearchTop_DeleteUnits(ResearchTop *wk);
static void ResearchTop_ClearActors(ResearchTop *wk);
static void ResearchTop_CreateActors(ResearchTop *wk);
static void ResearchTop_DeleteActors(ResearchTop *wk);
static void ResearchTop_ClearPaletteAnimes(ResearchTop *wk);
static void ResearchTop_CreatePaletteAnimes(ResearchTop *wk);
static void ResearchTop_DeletePaletteAnimes(ResearchTop *wk);
static void ResearchTop_SetupPaletteAnimes(ResearchTop *wk);
static void ResearchTop_RestorePalettes(ResearchTop *wk);
static void ResearchTop_VBlank(void *data);
static void ResearchTop_SetVBlank(ResearchTop *wk);
static void ResearchTop_ResetVBlank(ResearchTop *wk);
static void ResearchTop_ShowCommIcon(ResearchTop *wk);

static const u8 sTopUnitPriorities[UNIT_COUNT] = { 0, 0 };

static const u16 sTopUnitCounts[UNIT_COUNT] = { 10, 10 };

static const u32 sTopMsgFiles[MSG_DATA_COUNT] = { 0x168, 0x163 };

static const ResearchRect sTopButtonRects[BUTTON_RECT_COUNT] = {
    { 24, 232, 40, 80 },
    { 24, 232, 96, 136 },
#ifdef BUGFIX
    { 0, 0, TOUCH_RECT_END, 0 },
#else
    // BUG: The end of the table is written as a TouchRect, with TOUCH_RECT_END in the first field, which is left here,
    // so the copy in the work has no end and the touch panel goes on reading the work after it
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

static const BGSysLCDConfig sTopLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

// The icon of a new result
static const ResearchActorSetup sTopActors[ACTOR_COUNT] = { { 32, 104, 5, 0, 0, 1, OBJ_RES_MAIN_CHARS,
                                                              OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 } };

static const BGSetup sTopBGSubText = { 0,
                                       0,
                                       0x800,
                                       0,
                                       BGRES_256x256,
                                       GX_BG_COLORMODE_16,
                                       GX_BG_SCRBASE(0x1800),
                                       GX_BG_CHARBASE(0x10000),
                                       0x8000,
                                       GX_BG_EXTPLTT_01,
                                       0,
                                       GX_BG_AREAOVER_XLU,
                                       FALSE };

static const BGSetup sTopBGMainText = { 0,
                                        0,
                                        0x800,
                                        0,
                                        BGRES_256x256,
                                        GX_BG_COLORMODE_16,
                                        GX_BG_SCRBASE(0x1000),
                                        GX_BG_CHARBASE(0x10000),
                                        0x8000,
                                        GX_BG_EXTPLTT_01,
                                        1,
                                        GX_BG_AREAOVER_XLU,
                                        FALSE };

static const BGSetup sTopBGSubTitle = { 0,
                                        0,
                                        0x800,
                                        0,
                                        BGRES_256x256,
                                        GX_BG_COLORMODE_16,
                                        GX_BG_SCRBASE(0x1000),
                                        GX_BG_CHARBASE(0x04000),
                                        0x8000,
                                        GX_BG_EXTPLTT_01,
                                        1,
                                        GX_BG_AREAOVER_XLU,
                                        FALSE };

static const BGSetup sTopBGMainButtons = { 0,
                                           0,
                                           0x800,
                                           0,
                                           BGRES_256x256,
                                           GX_BG_COLORMODE_16,
                                           GX_BG_SCRBASE(0x0800),
                                           GX_BG_CHARBASE(0x04000),
                                           0x8000,
                                           GX_BG_EXTPLTT_01,
                                           2,
                                           GX_BG_AREAOVER_XLU,
                                           FALSE };

static const BGFontEntry sTopBGFonts[BG_FONT_COUNT] = {
    { { BG_SUB_TEXT, 0, 17, 32, 6, 0, 16, 15, 3, 4, 0 }, 0, 1, FALSE },
    { { BG_MAIN_TEXT, 2, 0, 28, 4, 0, 4, 14, 3, 4, 0 }, 0, 0, FALSE },
    { { BG_MAIN_TEXT, 3, 5, 26, 5, 0, 16, 14, 1, 2, 0 }, 0, 2, FALSE },
    { { BG_MAIN_TEXT, 3, 12, 26, 5, 0, 16, 14, 1, 2, 0 }, 0, 3, FALSE },
};

static const ResearchPaletteAnimeSetup sTopPaletteAnimes[ANIME_COUNT] = {
    { (u16 *)(HW_BG_PLTT + 0x146), (const u16 *)(HW_BG_PLTT + 0x146), 3, 0, 0xffff },
    { (u16 *)(HW_BG_PLTT + 0x14c), (const u16 *)(HW_BG_PLTT + 0x14c), 1, 4, 0xffff },
    { (u16 *)(HW_BG_PLTT + 0x14c), (const u16 *)(HW_BG_PLTT + 0x14c), 1, 3, 0xffff },
    { (u16 *)(HW_BG_PLTT + 0xa0), (const u16 *)(HW_BG_PLTT + 0xa0), 16, 9, 0 },
    { (u16 *)(HW_BG_PLTT + 0x1e0), (const u16 *)(HW_BG_PLTT + 0x1e0), 16, 9, 0 },
};

ResearchTop *ResearchTop_Create(ResearchCommon *common) {
    HeapID heapId = ResearchCommon_GetHeapID(common);
    ResearchTop *wk = ResearchTop_Alloc(heapId);

    ResearchTop_InitWork(wk);
    ResearchTop_SetHeapID(wk, heapId);
    ResearchTop_SetCommon(wk, common);
    return wk;
}

void ResearchTop_Delete(ResearchTop *wk) {
    ResearchTop_DeleteQueue(wk);
    ResearchTop_Free(wk);
}

void ResearchTop_Main(ResearchTop *wk) {
    switch (wk->seq) {
    case SEQ_SETUP:
        ResearchTop_SeqSetup(wk);
        break;
    case SEQ_TOUCH:
        ResearchTop_SeqTouch(wk);
        break;
    case SEQ_CURSOR:
        ResearchTop_SeqCursor(wk);
        break;
    case SEQ_WAIT:
        ResearchTop_SeqWait(wk);
        break;
    case SEQ_FADE_OUT:
        ResearchTop_SeqFadeOut(wk);
        break;
    case SEQ_TEARDOWN:
        ResearchTop_SeqTeardown(wk);
        break;
    case SEQ_END:
        return;
    }

    if (!ResearchTop_IsEnd(wk)) {
        ResearchTop_CheckNewResult(wk);
        ResearchTop_UpdatePaletteAnimes(wk);
        ResearchCommon_UpdatePaletteAnime(wk->common);
        func_0204b794();
    }
    ResearchTop_CountFrame(wk);
    ResearchTop_UpdateSeq(wk);
}

BOOL ResearchTop_IsEnd(ResearchTop *wk) {
    if (wk->isEnd) {
        return TRUE;
    }
    return FALSE;
}

u32 ResearchTop_GetNext(ResearchTop *wk) {
    return wk->next;
}

static void ResearchTop_SeqSetup(ResearchTop *wk) {
    u32 startSeq;

    ResearchTop_CreateQueue(wk);
    ResearchTop_CreateFont(wk);
    ResearchTop_LoadMsgData(wk);
    ResearchTop_InitButtonRects(wk);
    ResearchTop_InitCursor(wk);
    ResearchTop_InitBG(wk);
    ResearchTop_LoadSubBG(wk);
    ResearchTop_InitSubText(wk);
    ResearchTop_LoadMainBG(wk);
    ResearchTop_InitMainText(wk);
    ResearchTop_CreateBGFonts(wk);
    ResearchTop_LoadSubObjRes(wk);
    ResearchTop_LoadMainObjRes(wk);
    ResearchTop_CreateUnits(wk);
    ResearchTop_CreateActors(wk);
    ResearchTop_CreatePaletteAnimes(wk);
    ResearchTop_SetupPaletteAnimes(wk);
    ResearchTop_ShowCommIcon(wk);
    ResearchTop_SetVBlank(wk);

    if (!ResearchTop_IsButtonEnabled(wk, BUTTON_LIST)) {
        ResearchTop_DisableListButton(wk);
    }

    startSeq = ResearchTop_GetStartSeq(wk);
    ResearchTop_PushSeq(wk, ResearchTop_GetStartSeq(wk));
    if (startSeq == SEQ_CURSOR) {
        ResearchTop_HighlightButton(wk->cursor);
        ResearchTop_StartPaletteAnime(wk, ANIME_CURSOR);
    }

    ResearchTop_FadeIn();
    ResearchTop_EndSeq(wk);
}

static void ResearchTop_SeqTouch(ResearchTop *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 button = func_0203da0c(wk->buttonRects);
    s32 commonButton = func_0203da0c(ResearchCommon_GetTouchRects(wk->common));

    if (ResearchTop_IsForceExit(wk) || commonButton == RESEARCH_COMMON_BUTTON_RETURN) {
        ResearchTop_SetTouchMode(wk, TRUE);
        ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_EXIT);
        ResearchTop_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
        ResearchTop_EndSeq(wk);
        ResearchTop_PushSeq(wk, SEQ_WAIT);
        ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
        ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
    } else if (keys & PAD_BUTTON_B) {
        ResearchTop_SetTouchMode(wk, FALSE);
        ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_EXIT);
        ResearchTop_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
        ResearchTop_EndSeq(wk);
        ResearchTop_PushSeq(wk, SEQ_WAIT);
        ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
        ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
    } else if ((keys & PAD_KEY_UP) || (keys & PAD_KEY_DOWN) || (keys & PAD_KEY_LEFT) || (keys & PAD_KEY_RIGHT) ||
               (keys & PAD_BUTTON_A)) {
        ResearchTop_HighlightButton(wk->cursor);
        ResearchTop_StartPaletteAnime(wk, ANIME_CURSOR);
        ResearchTop_EndSeq(wk);
        ResearchTop_PushSeq(wk, SEQ_CURSOR);
    } else if (button == BUTTON_LIST) {
        if (ResearchTop_IsButtonEnabled(wk, BUTTON_LIST)) {
            ResearchTop_SetTouchMode(wk, TRUE);
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_LIST);
            ResearchTop_MoveCursorTo(wk, BUTTON_LIST);
            ResearchTop_HighlightButton(wk->cursor);
            ResearchTop_StopPaletteAnime(wk, ANIME_CURSOR);
            ResearchTop_StartPaletteAnime(wk, ANIME_DECIDE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
            ResearchTop_EndSeq(wk);
            ResearchTop_PushSeq(wk, SEQ_WAIT);
            ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
            ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    } else if (button == BUTTON_GRAPH) {
        if (ResearchTop_IsButtonEnabled(wk, BUTTON_GRAPH)) {
            ResearchTop_SetTouchMode(wk, TRUE);
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_GRAPH);
            ResearchTop_MoveCursorTo(wk, BUTTON_GRAPH);
            ResearchTop_HighlightButton(wk->cursor);
            ResearchTop_StopPaletteAnime(wk, ANIME_CURSOR);
            ResearchTop_StartPaletteAnime(wk, ANIME_DECIDE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
            ResearchTop_EndSeq(wk);
            ResearchTop_PushSeq(wk, SEQ_WAIT);
            ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
            ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    }
}

static void ResearchTop_SeqCursor(ResearchTop *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 button = func_0203da0c(wk->buttonRects);
    s32 commonButton = func_0203da0c(ResearchCommon_GetTouchRects(wk->common));

    if (ResearchTop_IsForceExit(wk) || commonButton == RESEARCH_COMMON_BUTTON_RETURN) {
        ResearchTop_SetTouchMode(wk, TRUE);
        ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_EXIT);
        ResearchTop_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
        ResearchTop_EndSeq(wk);
        ResearchTop_PushSeq(wk, SEQ_WAIT);
        ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
        ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
        return;
    }

    if (keys & PAD_KEY_UP) {
        ResearchTop_CursorUp(wk);
    }
    if (keys & PAD_KEY_DOWN) {
        ResearchTop_CursorDown(wk);
    }

    if (keys & PAD_BUTTON_A) {
        ResearchTop_SetTouchMode(wk, FALSE);
        ResearchTop_StopPaletteAnime(wk, ANIME_CURSOR);
        ResearchTop_StartPaletteAnime(wk, ANIME_DECIDE);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
        ResearchTop_EndSeq(wk);
        ResearchTop_PushSeq(wk, SEQ_WAIT);
        ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
        ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
        switch (wk->cursor) {
        case BUTTON_LIST:
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_LIST);
            break;
        case BUTTON_GRAPH:
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_GRAPH);
            break;
        default:
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_EXIT);
            break;
        }
    } else if (button == BUTTON_LIST) {
        if (ResearchTop_IsButtonEnabled(wk, BUTTON_LIST)) {
            ResearchTop_SetTouchMode(wk, TRUE);
            ResearchTop_MoveCursorTo(wk, BUTTON_LIST);
            ResearchTop_StopPaletteAnime(wk, ANIME_CURSOR);
            ResearchTop_StartPaletteAnime(wk, ANIME_DECIDE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_LIST);
            ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
            ResearchTop_EndSeq(wk);
            ResearchTop_PushSeq(wk, SEQ_WAIT);
            ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
            ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    } else if (button == BUTTON_GRAPH) {
        if (ResearchTop_IsButtonEnabled(wk, BUTTON_GRAPH)) {
            ResearchTop_SetTouchMode(wk, TRUE);
            ResearchTop_MoveCursorTo(wk, BUTTON_GRAPH);
            ResearchTop_StopPaletteAnime(wk, ANIME_CURSOR);
            ResearchTop_StartPaletteAnime(wk, ANIME_DECIDE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_GRAPH);
            ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
            ResearchTop_EndSeq(wk);
            ResearchTop_PushSeq(wk, SEQ_WAIT);
            ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
            ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    } else if (keys & PAD_BUTTON_B) {
        ResearchTop_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchTop_SetNext(wk, RESEARCH_TOP_NEXT_EXIT);
        ResearchTop_SetWait(wk, DECIDE_WAIT_FRAMES);
        ResearchTop_EndSeq(wk);
        ResearchTop_PushSeq(wk, SEQ_WAIT);
        ResearchTop_PushSeq(wk, SEQ_FADE_OUT);
        ResearchTop_PushSeq(wk, SEQ_TEARDOWN);
    }
}

static void ResearchTop_SeqWait(ResearchTop *wk) {
    if (ResearchTop_GetWait(wk) < ResearchTop_GetSeqFrames(wk)) {
        ResearchTop_EndSeq(wk);
    }
}

static void ResearchTop_SeqFadeOut(ResearchTop *wk) {
    switch (ResearchTop_GetSeqState(wk)) {
    case 0:
        ResearchTop_FadeOut();
        ResearchTop_NextSeqState(wk);
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            ResearchTop_EndSeq(wk);
        }
        break;
    }
}

static void ResearchTop_SeqTeardown(ResearchTop *wk) {
    ResearchTop_ResetVBlank(wk);
    ResearchCommon_StopPaletteAnime(wk->common);
    ResearchCommon_RestorePalette(wk->common);
    ResearchTop_RestorePalettes(wk);
    ResearchTop_DeletePaletteAnimes(wk);
    ResearchTop_DeleteActors(wk);
    ResearchTop_DeleteUnits(wk);
    ResearchTop_FreeSubObjRes(wk);
    ResearchTop_FreeMainObjRes(wk);
    ResearchTop_DeleteBGFonts(wk);
    ResearchTop_ExitMainText(wk);
    ResearchTop_UnloadMainBG(wk);
    ResearchTop_ExitSubText(wk);
    ResearchTop_UnloadSubBG(wk);
    ResearchTop_ExitBG(wk);
    ResearchTop_FreeMsgData(wk);
    ResearchTop_DeleteFont(wk);
    wk->isEnd = TRUE;
    ResearchTop_PushSeq(wk, SEQ_END);
    ResearchTop_EndSeq(wk);
}

static void ResearchTop_PushSeq(ResearchTop *wk, u32 seq) {
    Queue_Push(wk->queue, seq);
}

static void ResearchTop_EndSeq(ResearchTop *wk) {
    wk->seqDone = TRUE;
}

static void ResearchTop_UpdateSeq(ResearchTop *wk) {
    if (wk->seqDone && !Queue_IsEmpty(wk->queue)) {
        ResearchTop_SetSeq(wk, Queue_Pop(wk->queue));
    }
}

static void ResearchTop_SetSeq(ResearchTop *wk, u32 seq) {
    wk->seq = seq;
    wk->seqState = 0;
    wk->seqFrames = 0;
    wk->seqDone = FALSE;
}

static u32 ResearchTop_GetSeqState(ResearchTop *wk) {
    return wk->seqState;
}

static void ResearchTop_NextSeqState(ResearchTop *wk) {
    wk->seqState++;
}

// The sequence the screen starts with: the cursor's, when the screen was entered from another screen without touch
static u32 ResearchTop_GetStartSeq(ResearchTop *wk) {
    ResearchCommon *common = wk->common;
    u32 prevSeq = ResearchCommon_GetPrevSeq(common);
    BOOL touch = ResearchCommon_GetTouchMode(common);

    if (prevSeq != RESEARCH_SEQ_INIT && !touch) {
        return SEQ_CURSOR;
    }
    return SEQ_TOUCH;
}

static void ResearchTop_SetTouchMode(ResearchTop *wk, BOOL touch) {
    ResearchCommon_SetTouchMode(wk->common, touch);
}

static void ResearchTop_SetNext(ResearchTop *wk, u32 next) {
    wk->next = next;
}

static void ResearchTop_CheckNewResult(ResearchTop *wk) {
    if (!wk->newIconShown && GameBeaconSys_PopSurveyUpdated() == TRUE) {
        wk->newIconShown = TRUE;
        ResearchTop_ShowNewIcon(wk);
    }
}

static void ResearchTop_CursorUp(ResearchTop *wk) {
    ResearchTop_UnhighlightButton(wk->cursor);
    ResearchTop_MoveCursor(wk, -1);
    ResearchTop_HighlightButton(wk->cursor);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchTop_StartPaletteAnime(wk, ANIME_MOVE);
}

static void ResearchTop_CursorDown(ResearchTop *wk) {
    ResearchTop_UnhighlightButton(wk->cursor);
    ResearchTop_MoveCursor(wk, 1);
    ResearchTop_HighlightButton(wk->cursor);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchTop_StartPaletteAnime(wk, ANIME_MOVE);
}

static void ResearchTop_MoveCursorTo(ResearchTop *wk, int pos) {
    ResearchTop_UnhighlightButton(wk->cursor);
    ResearchTop_SetCursor(wk, pos);
    ResearchTop_HighlightButton(wk->cursor);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchTop_StartPaletteAnime(wk, ANIME_MOVE);
}

static void ResearchTop_HighlightButton(int button) {
    u8 bg, x, y, width, height, palette;

    switch (button) {
    case BUTTON_LIST:
        bg = BG_MAIN_BUTTONS;
        x = 3;
        y = 5;
        width = 26;
        height = 5;
        palette = 10;
        break;
    case BUTTON_GRAPH:
        bg = BG_MAIN_BUTTONS;
        x = 3;
        y = 12;
        width = 26;
        height = 5;
        palette = 10;
        break;
    default:
        return;
    }
    GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, palette);
    GFL_BGSysLoadScr(bg);
}

static void ResearchTop_UnhighlightButton(int button) {
    u8 bg, x, y, width, height, palette;

    switch (button) {
    case BUTTON_LIST:
        bg = BG_MAIN_BUTTONS;
        x = 3;
        y = 5;
        width = 26;
        height = 5;
        palette = 5;
        break;
    case BUTTON_GRAPH:
        bg = BG_MAIN_BUTTONS;
        x = 3;
        y = 12;
        width = 26;
        height = 5;
        palette = 6;
        break;
    default:
        return;
    }
    GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, palette);
    GFL_BGSysLoadScr(bg);
}

static void ResearchTop_DisableListButton(ResearchTop *wk) {
    BGFont_SetPalette(wk->bgFonts[2], 15);
    ResearchTop_StartPaletteAnime(wk, ANIME_DISABLED_1);
    ResearchTop_StartPaletteAnime(wk, ANIME_DISABLED_2);
}

static void ResearchTop_StartReturnAnime(ResearchTop *wk) {
    ResearchCommon_StartPaletteAnime(wk->common, 0);
}

static void ResearchTop_ShowNewIcon(ResearchTop *wk) {
    ClActor *actor = ResearchTop_GetActor(wk, 0);

    func_0204c124(actor, TRUE);
    func_0204c520(actor, TRUE);
    func_0204c53c(actor, FX32_ONE);
    func_0204c4d4(actor, 0);
}

static void ResearchTop_FadeIn(void) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
}

static void ResearchTop_FadeOut(void) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
}

static void ResearchTop_CountFrame(ResearchTop *wk) {
    wk->seqFrames++;
}

static u32 ResearchTop_GetSeqFrames(ResearchTop *wk) {
    return wk->seqFrames;
}

static void ResearchTop_SetWait(ResearchTop *wk, u32 frames) {
    wk->waitFrames = frames;
}

static u32 ResearchTop_GetWait(ResearchTop *wk) {
    return wk->waitFrames;
}

static void ResearchTop_MoveCursor(ResearchTop *wk, int dir) {
    int pos = wk->cursor;

    do {
        pos = (pos + dir + BUTTON_COUNT) % BUTTON_COUNT;
    } while (!ResearchTop_IsButtonEnabled(wk, pos));
    wk->cursor = pos;
}

static void ResearchTop_SetCursor(ResearchTop *wk, int pos) {
    wk->cursor = pos;
}

static BOOL ResearchTop_IsButtonEnabled(ResearchTop *wk, int button) {
    if (button == BUTTON_LIST) {
        if (ResearchTop_CountSurveys(wk) > 1) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static u32 ResearchTop_GetObjRes(ResearchTop *wk, u32 index) {
    return wk->objRes[index];
}

static ClActUnit *ResearchTop_GetUnit(ResearchTop *wk, u32 index) {
    return wk->units[index];
}

static ClActor *ResearchTop_GetActor(ResearchTop *wk, u32 index) {
    return wk->actors[index];
}

static void ResearchTop_StartPaletteAnime(ResearchTop *wk, u32 index) {
    PaletteAnime_Start(wk->animes[index], sTopPaletteAnimes[index].mode, sTopPaletteAnimes[index].color);
}

static void ResearchTop_StopPaletteAnime(ResearchTop *wk, u32 index) {
    PaletteAnime_Stop(wk->animes[index]);
}

static void ResearchTop_UpdatePaletteAnimes(ResearchTop *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Update(wk->animes[i]);
    }
}

static u8 ResearchTop_CountSurveys(ResearchTop *wk) {
    EventWork *eventWork = GameData_GetEventWork(ResearchCommon_GetGameData(wk->common));
    int count = 1;

    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 1) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 2) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 3) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 4) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 5) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 6) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, FIRST_SURVEY_FLAG + 7) == TRUE) {
        count++;
    }
    if (EventWork_FlagGet(eventWork, LAST_SURVEY_FLAG) == TRUE) {
        count++;
    }
    return count;
}

static ResearchCommon *ResearchTop_GetCommon(ResearchTop *wk) {
    return wk->common;
}

static void ResearchTop_SetCommon(ResearchTop *wk, ResearchCommon *common) {
    wk->common = common;
}

static void ResearchTop_SetHeapID(ResearchTop *wk, HeapID heapId) {
    wk->heapId = heapId;
}

static BOOL ResearchTop_IsForceExit(ResearchTop *wk) {
    return ResearchCommon_IsForceExit(ResearchTop_GetCommon(wk));
}

static ResearchTop *ResearchTop_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ResearchTop), FALSE, "research_top.c", 1486);
}

static void ResearchTop_Free(ResearchTop *wk) {
    GFL_HeapFree(wk);
}

static void ResearchTop_InitWork(ResearchTop *wk) {
    wk->seqDone = FALSE;
    wk->newIconShown = FALSE;
    wk->isEnd = FALSE;
    wk->seq = SEQ_SETUP;
    wk->seqFrames = 0;
    wk->waitFrames = 0;
    wk->cursor = BUTTON_LIST;
    wk->next = RESEARCH_TOP_NEXT_EXIT;
    wk->seqState = 0;
    ResearchTop_ClearObjRes(wk);
    ResearchTop_ClearQueue(wk);
    ResearchTop_ClearMsgData(wk);
    ResearchTop_ClearFont(wk);
    ResearchTop_ClearBGFonts(wk);
    ResearchTop_ClearUnits(wk);
    ResearchTop_ClearActors(wk);
    ResearchTop_ClearPaletteAnimes(wk);
}

// Puts the cursor on the button of the screen it came back from, when that wasn't by touch
static void ResearchTop_InitCursor(ResearchTop *wk) {
    ResearchCommon *common = wk->common;
    u32 prevSeq = ResearchCommon_GetPrevSeq(common);
    BOOL touch = ResearchCommon_GetTouchMode(common);
    int pos;

    if (prevSeq != RESEARCH_SEQ_INIT && !touch) {
        switch (prevSeq) {
        default:
        case RESEARCH_SEQ_LIST:
            pos = BUTTON_LIST;
            break;
        case RESEARCH_SEQ_GRAPH:
            pos = BUTTON_GRAPH;
            break;
        }
        ResearchTop_SetCursor(wk, pos);
        return;
    }

    if (ResearchTop_IsButtonEnabled(wk, BUTTON_LIST)) {
        ResearchTop_SetCursor(wk, BUTTON_LIST);
    } else {
        ResearchTop_SetCursor(wk, BUTTON_GRAPH);
    }
}

static void ResearchTop_ClearFont(ResearchTop *wk) {
    wk->font = NULL;
}

static void ResearchTop_CreateFont(ResearchTop *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
}

static void ResearchTop_DeleteFont(ResearchTop *wk) {
    GFL_FontFree(wk->font);
}

static void ResearchTop_ClearMsgData(ResearchTop *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        wk->msgData[i] = NULL;
    }
}

static void ResearchTop_LoadMsgData(ResearchTop *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        wk->msgData[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sTopMsgFiles[i], wk->heapId);
    }
}

static void ResearchTop_FreeMsgData(ResearchTop *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        GFL_MsgDataFree(wk->msgData[i]);
    }
}

static void ResearchTop_InitButtonRects(ResearchTop *wk) {
    int i;

    for (i = 0; i < BUTTON_RECT_COUNT; i++) {
        wk->buttonRects[i].left = sTopButtonRects[i].left;
        wk->buttonRects[i].right = sTopButtonRects[i].right;
        wk->buttonRects[i].top = sTopButtonRects[i].top;
        wk->buttonRects[i].bottom = sTopButtonRects[i].bottom;
    }
}

static void ResearchTop_ClearQueue(ResearchTop *wk) {
    wk->queue = NULL;
}

static void ResearchTop_CreateQueue(ResearchTop *wk) {
    wk->queue = Queue_Create(10, wk->heapId);
}

static void ResearchTop_DeleteQueue(ResearchTop *wk) {
    Queue_Delete(wk->queue);
}

static void ResearchTop_InitBG(ResearchTop *wk) {
    GFL_BGSysSetLCDConfig(&sTopLCDConfig);
    GFL_BGSysCreateBG(BG_SUB_TITLE, &sTopBGSubTitle, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_SUB_TEXT, &sTopBGSubText, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_BUTTONS, &sTopBGMainButtons, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_TEXT, &sTopBGMainText, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_BACK, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_PATTERN, TRUE);
    GFL_BGSysSetBGEnabled(BG_SUB_TITLE, TRUE);
    GFL_BGSysSetBGEnabled(BG_SUB_TEXT, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_MAIN_FRAME, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_BUTTONS, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_TEXT, TRUE);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 7, 15);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG1, 16, 5);
    BmpWin_InitAllocator(wk->heapId);
}

static void ResearchTop_ExitBG(ResearchTop *wk) {
    BmpWin_FreeAllocator();
    GFL_BGSysReleaseBG(BG_MAIN_TEXT);
    GFL_BGSysReleaseBG(BG_MAIN_BUTTONS);
    GFL_BGSysReleaseBG(BG_SUB_TEXT);
    GFL_BGSysReleaseBG(BG_SUB_TITLE);
}

static void ResearchTop_LoadSubBG(ResearchTop *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, wk->heapId);
    void *file = GFL_ArcToolReadHeapNew(handle, 13, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_SUB_TITLE, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysLoadScr(BG_SUB_TITLE);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchTop_UnloadSubBG(ResearchTop *wk) {
}

static void ResearchTop_InitSubText(ResearchTop *wk) {
    GFL_BGSysFillChar(BG_SUB_TEXT, 0, 1, 0);
    GFL_BGSysClearScr(BG_SUB_TEXT);
}

static void ResearchTop_ExitSubText(ResearchTop *wk) {
    GFL_BGSysFreeFilledChar(BG_SUB_TEXT, 1, 0);
}

static void ResearchTop_LoadMainBG(ResearchTop *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, wk->heapId);
    void *file = GFL_ArcToolReadHeapNew(handle, 4, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_MAIN_BUTTONS, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysLoadScr(BG_MAIN_BUTTONS);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchTop_UnloadMainBG(ResearchTop *wk) {
}

static void ResearchTop_InitMainText(ResearchTop *wk) {
    GFL_BGSysFillChar(BG_MAIN_TEXT, 0, 1, 0);
    GFL_BGSysClearScr(BG_MAIN_TEXT);
}

static void ResearchTop_ExitMainText(ResearchTop *wk) {
    GFL_BGSysFreeFilledChar(BG_MAIN_TEXT, 1, 0);
}

static void ResearchTop_ClearBGFonts(ResearchTop *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        wk->bgFonts[i] = NULL;
    }
}

static void ResearchTop_CreateBGFonts(ResearchTop *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        BGFontSetup setup;

        setup.window.bg = sTopBGFonts[i].window.bg;
        setup.window.x = sTopBGFonts[i].window.x;
        setup.window.y = sTopBGFonts[i].window.y;
        setup.window.width = sTopBGFonts[i].window.width;
        setup.window.height = sTopBGFonts[i].window.height;
        setup.window.textX = sTopBGFonts[i].window.textX;
        setup.window.textY = sTopBGFonts[i].window.textY;
        setup.window.palette = sTopBGFonts[i].window.palette;
        setup.window.letterColor = sTopBGFonts[i].window.letterColor;
        setup.window.shadowColor = sTopBGFonts[i].window.shadowColor;
        setup.window.backColor = sTopBGFonts[i].window.backColor;
        setup.centered = sTopBGFonts[i].centered;
        wk->bgFonts[i] = BGFont_Create(&setup, wk->font, wk->msgData[sTopBGFonts[i].msgDataIndex], wk->heapId);
        BGFont_PrintMsg(wk->bgFonts[i], sTopBGFonts[i].strId);
    }
}

static void ResearchTop_DeleteBGFonts(ResearchTop *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        BGFont_Delete(wk->bgFonts[i]);
        wk->bgFonts[i] = NULL;
    }
}

static void ResearchTop_ClearObjRes(ResearchTop *wk) {
    int i;

    for (i = 0; i < OBJ_RES_COUNT; i++) {
        wk->objRes[i] = 0;
    }
}

static void ResearchTop_LoadSubObjRes(ResearchTop *wk) {
    HeapID heapId = wk->heapId;
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, heapId);
    u32 chars = func_0204b81c(handle, 15, FALSE, CLACT_VRAM_SUB, heapId);
    u32 palette = func_0204bba0(handle, 16, CLACT_VRAM_SUB, 0, heapId);
    u32 cellAnims = func_0204bde0(handle, 14, 17, heapId);

    wk->objRes[OBJ_RES_SUB_CHARS] = chars;
    wk->objRes[OBJ_RES_SUB_PALETTE] = palette;
    wk->objRes[OBJ_RES_SUB_CELL_ANIMS] = cellAnims;
    GFL_ArcToolFree(handle);
}

static void ResearchTop_FreeSubObjRes(ResearchTop *wk) {
    func_0204b98c(wk->objRes[OBJ_RES_SUB_CHARS]);
    func_0204bcd0(wk->objRes[OBJ_RES_SUB_PALETTE]);
    func_0204be64(wk->objRes[OBJ_RES_SUB_CELL_ANIMS]);
}

static void ResearchTop_LoadMainObjRes(ResearchTop *wk) {
    HeapID heapId = wk->heapId;
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, heapId);
    u32 chars = func_0204b81c(handle, 15, FALSE, CLACT_VRAM_MAIN, heapId);
    u32 palette = func_0204bbb8(handle, 16, CLACT_VRAM_MAIN, 0xc0, 0, 4, heapId);
    u32 cellAnims = func_0204bde0(handle, 14, 17, heapId);

    wk->objRes[OBJ_RES_MAIN_CHARS] = chars;
    wk->objRes[OBJ_RES_MAIN_PALETTE] = palette;
    wk->objRes[OBJ_RES_MAIN_CELL_ANIMS] = cellAnims;
    GFL_ArcToolFree(handle);
}

static void ResearchTop_FreeMainObjRes(ResearchTop *wk) {
    func_0204b98c(wk->objRes[OBJ_RES_MAIN_CHARS]);
    func_0204bcd0(wk->objRes[OBJ_RES_MAIN_PALETTE]);
    func_0204be64(wk->objRes[OBJ_RES_MAIN_CELL_ANIMS]);
}

static void ResearchTop_ClearUnits(ResearchTop *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        wk->units[i] = NULL;
    }
}

static void ResearchTop_CreateUnits(ResearchTop *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        wk->units[i] = func_0204bf1c(sTopUnitCounts[i], sTopUnitPriorities[i], wk->heapId);
    }
}

static void ResearchTop_DeleteUnits(ResearchTop *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        func_0204bf98(wk->units[i]);
    }
}

static void ResearchTop_ClearActors(ResearchTop *wk) {
    wk->actors[0] = NULL;
}

static void ResearchTop_CreateActors(ResearchTop *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        const ResearchActorSetup *entry = &sTopActors[i];
        ClActorSetup setup;
        ClActUnit *unit;
        u32 chars;
        u32 palette;
        u32 cellAnims;

        setup.x = entry->x;
        setup.y = entry->y;
        setup.sequence = entry->sequence;
        setup.priority = entry->priority;
        setup.bgPriority = entry->bgPriority;
        unit = ResearchTop_GetUnit(wk, entry->unit);
        chars = ResearchTop_GetObjRes(wk, entry->chars);
        palette = ResearchTop_GetObjRes(wk, entry->palette);
        cellAnims = ResearchTop_GetObjRes(wk, entry->cellAnims);
        wk->actors[i] = func_0204c040(unit, chars, palette, cellAnims, &setup, entry->surface, wk->heapId);
        func_0204c124(wk->actors[i], FALSE);
    }
}

static void ResearchTop_DeleteActors(ResearchTop *wk) {
    func_0204c108(wk->actors[0]);
}

static void ResearchTop_ClearPaletteAnimes(ResearchTop *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        wk->animes[i] = NULL;
    }
}

static void ResearchTop_CreatePaletteAnimes(ResearchTop *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        wk->animes[i] = PaletteAnime_Create(wk->heapId);
    }
}

static void ResearchTop_DeletePaletteAnimes(ResearchTop *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Delete(wk->animes[i]);
    }
}

static void ResearchTop_SetupPaletteAnimes(ResearchTop *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Setup(wk->animes[i], sTopPaletteAnimes[i].dst, sTopPaletteAnimes[i].src,
                           sTopPaletteAnimes[i].count);
    }
}

static void ResearchTop_RestorePalettes(ResearchTop *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Restore(wk->animes[i]);
    }
}

static void ResearchTop_VBlank(void *data) {
    func_0204b7c8();
}

static void ResearchTop_SetVBlank(ResearchTop *wk) {
    GFL_VBlankSetCallback(ResearchTop_VBlank, NULL);
}

static void ResearchTop_ResetVBlank(ResearchTop *wk) {
    GFL_VBlankResetCallback();
}

static void ResearchTop_ShowCommIcon(ResearchTop *wk) {
    func_02042ba8(TRUE, wk->heapId);
}
