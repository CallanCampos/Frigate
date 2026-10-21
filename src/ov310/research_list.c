#include "types.h"
#include "app/research_radar/bg_font.h"
#include "app/research_radar/palette_anime.h"
#include "app/research_radar/queue.h"
#include "app/research_radar/research_common.h"
#include "app/research_radar/research_list.h"
#include "app/research_radar/research_list_recovery.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/survey.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nnsys/g2d.h"
#include "save/event_work.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// The Research Radar's list of surveys, one for each that is unlocked, which scrolls by dragging its bar or with the
// cursor. Choosing a survey other than the one being run asks whether to run it instead, and running it makes it the
// survey of the save. The survey being run is drawn a tile to the left of the others, with an icon

// The sequences of the screen, which run one after another from a queue
enum {
    SEQ_SETUP,
    SEQ_TOUCH,
    SEQ_CURSOR,
    // Moves the cursor to the next survey, scrolling the list to it
    SEQ_CURSOR_SCROLL,
    // Scrolls the list while its bar is held, and on after it is released
    SEQ_DRAG,
    SEQ_RETURN_TOUCH,
    SEQ_RETURN_CURSOR,
    // Scrolls the chosen survey to the middle and asks whether to run it
    SEQ_CONFIRM_OPEN_TOUCH,
    SEQ_CONFIRM_TOUCH,
    SEQ_CONFIRM_OPEN_CURSOR,
    SEQ_CONFIRM_CURSOR,
    SEQ_CONFIRM_CANCEL,
    // Runs the chosen survey
    SEQ_DECIDE,
    SEQ_TEARDOWN,
    SEQ_END,
};

// The BGs: the main engine's frame on BG 0, the list on BG 2 and its text on BG 3, and the sub engine's title on BG 6
// and its text on BG 7, in front of the proc's BGs
#define BG_MAIN_FRAME 0
#define BG_MAIN_LIST 2
#define BG_MAIN_TEXT 3
#define BG_SUB_TITLE 6
#define BG_SUB_TEXT 7

// The objects' resources: the main engine's, the palette of the buttons, which comes from the graphics the apps
// share, and the sub engine's
enum {
    OBJ_RES_MAIN_CHARS,
    OBJ_RES_MAIN_PALETTE,
    OBJ_RES_MAIN_CELL_ANIMS,
    OBJ_RES_BUTTON_PALETTE,
    OBJ_RES_SUB_CHARS,
    OBJ_RES_SUB_PALETTE,
    OBJ_RES_SUB_CELL_ANIMS,
    OBJ_RES_COUNT,
};

// The units of the actors, and the one of the bitmaps' actors
#define UNIT_COUNT 3
#define UNIT_BMP_OAM 2

// The actors: the scroll bar's knob, an animation at the middle of the screen, and the icon of the survey being run
enum {
    ACTOR_SCROLL_BAR,
    ACTOR_CENTER,
    ACTOR_CURRENT,
    ACTOR_COUNT,
};

// The bitmaps shown as actors: the question whether to run a survey, its answers, and the message that it runs
enum {
    BMP_QUESTION,
    BMP_YES,
    BMP_NO,
    BMP_DECIDED,
    BMP_COUNT,
};

// The buttons of the question, in the order of its cursor
enum {
    BUTTON_YES,
    BUTTON_NO,
    BUTTON_COUNT,
};

// The palette animations: the list's cursor, a cursor move and a survey chosen, and the same for the question's
// buttons
enum {
    ANIME_CURSOR,
    ANIME_MOVE,
    ANIME_DECIDE,
    ANIME_BUTTON_CURSOR,
    ANIME_BUTTON_MOVE,
    ANIME_BUTTON_DECIDE,
    ANIME_COUNT,
};

// The texts of the sub screen: its title and help, and the chosen survey's name, description and three questions
enum {
    BG_FONT_TITLE,
    BG_FONT_HELP,
    BG_FONT_NAME,
    BG_FONT_DESCRIPTION,
    BG_FONT_QUESTION_1,
    BG_FONT_QUESTION_2,
    BG_FONT_QUESTION_3,
    BG_FONT_COUNT,
};

#define MSG_DATA_COUNT 2
#define SURVEY_COUNT 10
#define QUESTION_COUNT 3
#define BUTTON_RECT_COUNT 3
#define LIST_RECT_COUNT 11
#define SCROLL_RECT_COUNT 2

// The value of a survey index when there is none
#define SURVEY_NONE 0xff

// The list's layout: each survey is 24 pixels high, 3 rows of tiles, and the list shows from y 24 to 168
#define ITEM_HEIGHT 24
#define ITEM_ROWS 3
#define VIEW_TOP 24
#define VIEW_BOTTOM 168
// The BG scroll that shows the end of the list of all the surveys, which the saved scroll is kept within
#define MAX_BG_SCROLL (SURVEY_COUNT * ITEM_HEIGHT - VIEW_BOTTOM)

// The scroll bar's knob moves between these y
#define BAR_TOP 32
#define BAR_BOTTOM 160

// The fastest the list goes on scrolling after its bar is released
#define DRAG_SPEED_MAX 9

// The frames a direction is held before the cursor repeats
#define KEY_REPEAT_FRAMES 10

// The frames after a survey is run before the screen fades out
#define DECIDE_WAIT_FRAMES 60

// The first survey is always there, and each of these event flags unlocks one more
#define FIRST_SURVEY_FLAG 0xbf
#define LAST_SURVEY_FLAG 0xc7

// A text color of the text renderer, from each color's index

struct ResearchList {
    ResearchCommon *common;
    ResearchListRecovery *recovery;
    HeapID heapId;
    Font *font;
    MsgData *msgData[MSG_DATA_COUNT];
    Queue *queue;
    u32 seq;
    u32 seqState;
    u32 seqFrames;
    u32 waitFrames;
    // The cursor of the question whether to run a survey
    int buttonCursor;
    u8 surveyCount;
    // The survey chosen, and the one being run once the screen ends
    u8 selected;
    u8 cursor;
    // Where the cursor goes when it scrolls the list
    u8 cursorTarget;
    // The y in the list that the view follows, and its last value
    int scrollY;
    int prevScrollY;
    // A scroll in progress, from scrollFrom to scrollTo over scrollFrames frames
    int scrollFrom;
    int scrollTo;
    int scrollFrames;
    int scrollFrame;
    // The speed the list goes on scrolling at after its bar is released, and how it slows down
    int dragSpeed;
    int dragAccel;
    u32 keyRepeatFrames;
    TouchRect buttonRects[BUTTON_RECT_COUNT];
    TouchRect listRects[LIST_RECT_COUNT];
    TouchRect scrollRects[SCROLL_RECT_COUNT];
    PaletteFade *paletteFade;
    PaletteAnime *animes[ANIME_COUNT];
    BGFont *itemFonts[SURVEY_COUNT];
    BGFont *bgFonts[BG_FONT_COUNT];
    u32 objRes[OBJ_RES_COUNT];
    ClActUnit *units[UNIT_COUNT];
    ClActor *actors[ACTOR_COUNT];
    BmpOamSys *bmpOamSys;
    BmpOamActor *bmpOamActors[BMP_COUNT];
    GFLBitmap *bitmaps[BMP_COUNT];
    TCBManager *tcbMgr;
    TCB *vblankTask;
    BOOL seqDone;
    // Whether the rest of the screen is darkened, behind the question
    BOOL darkened;
    BOOL cursorShown;
    BOOL isEnd;
    u32 next;
};

typedef struct {
    BGFontWindow window;
    u32 msgDataIndex;
    u32 strId;
    BOOL centered;
} BGFontEntry;

// A bitmap shown as an actor, with the text drawn into it, over a picture loaded from an archive when hasBase is set
typedef struct {
    u32 width;
    u32 height;
    u32 tileSize;
    u32 msgDataIndex;
    u32 strId;
    u32 textX;
    u32 textY;
    u8 letterColor;
    u8 shadowColor;
    u8 backColor;
    u32 arcId;
    u32 fileId;
    BOOL hasBase;
} BitmapEntry;

// The actor of a bitmap: where it is, and its palette resource and offset
typedef struct {
    s16 x;
    s16 y;
    u32 palette;
    u32 palOffset;
    u8 priority;
    u8 bgPriority;
    u16 surface;
    u32 vramType;
} BmpOamEntry;

static void ResearchList_SeqSetup(ResearchList *wk);
static void ResearchList_SeqTouch(ResearchList *wk);
static void ResearchList_SeqCursor(ResearchList *wk);
static void ResearchList_SeqCursorScroll(ResearchList *wk);
static void ResearchList_SeqDrag(ResearchList *wk);
static void ResearchList_SeqConfirmTouch(ResearchList *wk);
static void ResearchList_SeqConfirmCursor(ResearchList *wk);
static void ResearchList_SeqConfirmOpenTouch(ResearchList *wk);
static void ResearchList_SeqConfirmOpenCursor(ResearchList *wk);
static void ResearchList_SeqDecide(ResearchList *wk);
static void ResearchList_SeqReturnCursor(ResearchList *wk);
static void ResearchList_SeqReturnTouch(ResearchList *wk);
static void ResearchList_SeqConfirmCancel(ResearchList *wk);
static void ResearchList_SeqTeardown(ResearchList *wk);
static void ResearchList_CountFrame(ResearchList *wk);
static void ResearchList_PushSeq(ResearchList *wk, u32 seq);
static void ResearchList_EndSeq(ResearchList *wk);
static BOOL ResearchList_GetTouchMode(ResearchList *wk);
static void ResearchList_SetTouchMode(ResearchList *wk, BOOL touch);
static void ResearchList_SetNext(ResearchList *wk, u32 next);
static u32 ResearchList_GetWait(ResearchList *wk);
static void ResearchList_UpdateSeq(ResearchList *wk);
static u32 ResearchList_GetSeq(ResearchList *wk);
static void ResearchList_SetSeq(ResearchList *wk, u32 seq);
static u32 ResearchList_GetSeqState(ResearchList *wk);
static void ResearchList_NextSeqState(ResearchList *wk);
static void ResearchList_ResetSeqState(ResearchList *wk);
static void ResearchList_ButtonCursorUp(ResearchList *wk);
static void ResearchList_ButtonCursorDown(ResearchList *wk);
static void ResearchList_MoveButtonCursorTo(ResearchList *wk, int pos);
static void ResearchList_FollowScroll(ResearchList *wk);
static void ResearchList_MoveCursorTo(ResearchList *wk, u8 pos);
static void ResearchList_StartReturnAnime(ResearchList *wk);
static void ResearchList_StartCursorScroll(ResearchList *wk);
static void ResearchList_StartScrollBack(ResearchList *wk);
static void ResearchList_StartScrollToCursor(ResearchList *wk);
static u8 ResearchList_GetCurrentSurvey(ResearchList *wk);
static void ResearchList_SetCurrentSurvey(ResearchList *wk);
static void ResearchList_MoveButtonCursor(ResearchList *wk, int dir);
static void ResearchList_SetButtonCursor(ResearchList *wk, int pos);
static void ResearchList_HighlightButton(ResearchList *wk, u32 button);
static void ResearchList_UnhighlightButton(ResearchList *wk, u32 button);
static void ResearchList_SetCursor(ResearchList *wk, u8 pos);
static void ResearchList_SetCursorTarget(ResearchList *wk, int dir);
static void ResearchList_ShowCursor(ResearchList *wk);
static void ResearchList_HideCursor(ResearchList *wk);
static void ResearchList_IndentItem(ResearchList *wk, u8 index);
static void ResearchList_UnindentItem(ResearchList *wk, u8 index);
static u8 ResearchList_GetItemColumn(ResearchList *wk, u8 index);
static u8 ResearchList_GetItemRow(ResearchList *wk, u8 index);
static int ResearchList_GetItemScreenX(ResearchList *wk, u8 index);
static int ResearchList_GetItemScreenTop(ResearchList *wk, u8 index);
static int ResearchList_GetItemScreenBottom(ResearchList *wk, u8 index);
static void ResearchList_UpdateWindow(ResearchList *wk);
static void ResearchList_UpdateCurrentIcon(ResearchList *wk);
static void ResearchList_PrintCursorSurvey(ResearchList *wk);
static void ResearchList_PrintSurvey(ResearchList *wk, u8 index);
static void ResearchList_HideSurvey(ResearchList *wk);
static void ResearchList_ShowSurvey(ResearchList *wk);
static void ResearchList_UpdateScrollBar(ResearchList *wk);
static int ResearchList_GetBarY(ResearchList *wk);
static int ResearchList_BarYToScroll(ResearchList *wk, int y);
static void ResearchList_SlowDrag(ResearchList *wk);
static BOOL ResearchList_IsDragging(ResearchList *wk);
static void ResearchList_SetDragSpeed(ResearchList *wk);
static void ResearchList_ReleaseDrag(ResearchList *wk);
static void ResearchList_SetDragAccel(ResearchList *wk);
static void ResearchList_StartScroll(ResearchList *wk, int from, int to, int frames);
static void ResearchList_UpdateScroll(ResearchList *wk);
static BOOL ResearchList_IsScrollEnd(ResearchList *wk);
static void ResearchList_DragTo(ResearchList *wk, int y);
static void ResearchList_SetScrollY(ResearchList *wk, int y);
static void ResearchList_ScrollToView(ResearchList *wk);
static void ResearchList_ScrollToDrag(ResearchList *wk);
static int ResearchList_GetBGScroll(void);
static void ResearchList_SetBGScroll(int y);
static int ResearchList_GetViewTop(void);
static int ResearchList_GetViewBottom(void);
static int ResearchList_GetItemAt(int y);
static int ResearchList_GetItemTop(int index);
static int ResearchList_GetItemBottom(int index);
static int ResearchList_GetMaxBGScroll(ResearchList *wk);
static int ResearchList_GetListHeight(ResearchList *wk);
static BOOL ResearchList_CanScroll(ResearchList *wk);
static void ResearchList_UpdateListRects(ResearchList *wk);
static void ResearchList_FadeIn(void);
static void ResearchList_FadeOut(void);
static BOOL ResearchList_IsFadeDone(void);
static void ResearchList_Darken(ResearchList *wk);
static void ResearchList_Brighten(ResearchList *wk);
static void ResearchList_BrightenCurrentIcon(ResearchList *wk);
static BOOL ResearchList_IsPaletteFadeDone(ResearchList *wk);
static void ResearchList_StartCommonPaletteAnime(ResearchList *wk, u32 index);
static void ResearchList_StartPaletteAnime(ResearchList *wk, u32 index);
static void ResearchList_StopPaletteAnime(ResearchList *wk, u32 index);
static BOOL ResearchList_IsPaletteAnimeEnd(ResearchList *wk, u32 index);
static void ResearchList_UpdatePaletteAnimes(ResearchList *wk);
static void ResearchList_UpdateCommonPaletteAnime(ResearchList *wk);
static u32 ResearchList_GetKeyRepeatFrames(ResearchList *wk);
static void ResearchList_CountKeyRepeat(ResearchList *wk);
static void ResearchList_ResetKeyRepeat(ResearchList *wk);
static void ResearchList_VBlank(TCB *tcb, void *data);
static GameData *ResearchList_GetGameData(ResearchList *wk);
static void ResearchList_SetHeapID(ResearchList *wk, HeapID heapId);
static void ResearchList_SetCommon(ResearchList *wk, ResearchCommon *common);
static ResearchCommon *ResearchList_GetCommon(ResearchList *wk);
static void ResearchList_SetRecovery(ResearchList *wk, ResearchListRecovery *recovery);
static void ResearchList_LoadRecovery(ResearchList *wk);
static void ResearchList_SaveRecovery(ResearchList *wk);
static BOOL ResearchList_IsForceExit(ResearchList *wk);
static u8 ResearchList_GetSurveyCount(ResearchList *wk);
static u8 ResearchList_GetSelected(ResearchList *wk);
static void ResearchList_SetSelected(ResearchList *wk, u8 index);
static BOOL ResearchList_IsSurvey(ResearchList *wk, u8 index);
static u32 ResearchList_GetObjRes(ResearchList *wk, u32 index);
static ClActUnit *ResearchList_GetUnit(ResearchList *wk, u32 index);
static ClActor *ResearchList_GetActor(ResearchList *wk, u32 index);
static void ResearchList_SetBmpOamVisible(ResearchList *wk, u32 index, BOOL visible);
static BmpOamActor *ResearchList_GetButtonBmpOam(ResearchList *wk, u32 button);
static ResearchList *ResearchList_Alloc(HeapID heapId);
static void ResearchList_InitWork(ResearchList *wk);
static void ResearchList_Free(ResearchList *wk);
static void ResearchList_CountSurveys(ResearchList *wk);
static void ResearchList_CreateFont(ResearchList *wk);
static void ResearchList_ClearFont(ResearchList *wk);
static void ResearchList_DeleteFont(ResearchList *wk);
static void ResearchList_LoadMsgData(ResearchList *wk);
static void ResearchList_ClearMsgData(ResearchList *wk);
static void ResearchList_FreeMsgData(ResearchList *wk);
static void ResearchList_CreateQueue(ResearchList *wk);
static void ResearchList_ClearQueue(ResearchList *wk);
static void ResearchList_DeleteQueue(ResearchList *wk);
static void ResearchList_InitTouchRects(ResearchList *wk);
static void ResearchList_InitBG(ResearchList *wk);
static void ResearchList_ExitBG(ResearchList *wk);
static void ResearchList_LoadSubBG(ResearchList *wk);
static void ResearchList_UnloadSubBG(ResearchList *wk);
static void ResearchList_InitSubText(ResearchList *wk);
static void ResearchList_ExitSubText(ResearchList *wk);
static void ResearchList_LoadFrameBG(ResearchList *wk);
static void ResearchList_UnloadFrameBG(ResearchList *wk);
static void ResearchList_LoadListBG(ResearchList *wk);
static void ResearchList_UnloadListBG(ResearchList *wk);
static void ResearchList_InitMainText(ResearchList *wk);
static void ResearchList_ExitMainText(ResearchList *wk);
static void ResearchList_CreateBGFonts(ResearchList *wk);
static void ResearchList_ClearBGFonts(ResearchList *wk);
static void ResearchList_DeleteBGFonts(ResearchList *wk);
static void ResearchList_ClearObjRes(ResearchList *wk);
static void ResearchList_LoadSubObjRes(ResearchList *wk);
static void ResearchList_FreeSubObjRes(ResearchList *wk);
static void ResearchList_LoadMainObjRes(ResearchList *wk);
static void ResearchList_FreeMainObjRes(ResearchList *wk);
static void ResearchList_ClearUnits(ResearchList *wk);
static void ResearchList_CreateUnits(ResearchList *wk);
static void ResearchList_DeleteUnits(ResearchList *wk);
static void ResearchList_ClearActors(ResearchList *wk);
static void ResearchList_CreateActors(ResearchList *wk);
static void ResearchList_DeleteActors(ResearchList *wk);
static void ResearchList_ClearBitmaps(ResearchList *wk);
static void ResearchList_CreateBitmaps(ResearchList *wk);
static void ResearchList_DrawBitmaps(ResearchList *wk);
static void ResearchList_DrawYesButton(ResearchList *wk);
static void ResearchList_DrawNoButton(ResearchList *wk);
static void ResearchList_FreeBitmaps(ResearchList *wk);
static void ResearchList_CreateBmpOam(ResearchList *wk);
static void ResearchList_DeleteBmpOam(ResearchList *wk);
static void ResearchList_CreateBmpOamActors(ResearchList *wk);
static void ResearchList_DeleteBmpOamActors(ResearchList *wk);
static void ResearchList_ClearPaletteFade(ResearchList *wk);
static void ResearchList_CreatePaletteFade(ResearchList *wk);
static void ResearchList_DeletePaletteFade(ResearchList *wk);
static void ResearchList_CreatePaletteAnimes(ResearchList *wk);
static void ResearchList_ClearPaletteAnimes(ResearchList *wk);
static void ResearchList_DeletePaletteAnimes(ResearchList *wk);
static void ResearchList_SetupPaletteAnimes(ResearchList *wk);
static void ResearchList_RestorePalettes(ResearchList *wk);
static void ResearchList_ShowCenterActor(ResearchList *wk);
static void ResearchList_ShowCommIcon(ResearchList *wk);
static void ResearchList_SetVBlank(ResearchList *wk);
static void ResearchList_ResetVBlank(ResearchList *wk);

static const u8 sListUnitPriorities[UNIT_COUNT] = { 0, 0, 0 };

static const u16 sListUnitCounts[UNIT_COUNT] = { 10, 10, 60 };

// The scroll bar
static const ResearchRect sListScrollRects[SCROLL_RECT_COUNT] = {
    { 232, 255, VIEW_TOP, VIEW_BOTTOM },
#ifdef BUGFIX
    { 0, 0, TOUCH_RECT_END, 0 },
#else
    // BUG: As in research_top.c, the end of the table is written as a TouchRect, so the copy in the work has no end
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

static const u32 sListMsgFiles[MSG_DATA_COUNT] = { 0x168, 0x163 };

static const u32 sListButtonBmpOams[BUTTON_COUNT] = { BMP_YES, BMP_NO };

// The three questions of each survey
static const u8 sListSurveyQuestions2[SURVEY_COUNT] = { 8, 25, 7, 5, 6, 13, 21, 16, 19, 18 };

static const u8 sListSurveyQuestions1[SURVEY_COUNT] = { 0, 1, 2, 3, 4, 12, 20, 10, 9, 15 };

static const u8 sListSurveyNames[SURVEY_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

static const u8 sListSurveyQuestions3[SURVEY_COUNT] = { 29, 26, 28, 11, 14, 24, 22, 17, 23, 27 };

static const u8 sListSurveyDescriptions[SURVEY_COUNT] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };

// The buttons of the question
static const ResearchRect sListButtonRects[BUTTON_RECT_COUNT] = {
    { 160, 240, 142, 166 },
    { 160, 240, 168, 192 },
#ifdef BUGFIX
    { 0, 0, TOUCH_RECT_END, 0 },
#else
    // BUG: The same as sListScrollRects' end
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

static const BGSysLCDConfig sListLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

// The message of each question
static const u8 sListQuestionMsgs[30] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
                                          35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49 };

static const BGSetup sListBGMainFrame = { 0,
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

static const BGSetup sListBGSubTitle = { 0,
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

static const BGSetup sListBGSubText = { 0,
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

static const BGSetup sListBGMainList = { 0,
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

static const BGSetup sListBGMainText = { 0,
                                         0,
                                         0x800,
                                         0,
                                         BGRES_256x256,
                                         GX_BG_COLORMODE_16,
                                         GX_BG_SCRBASE(0x1800),
                                         GX_BG_CHARBASE(0x10000),
                                         0x8000,
                                         GX_BG_EXTPLTT_01,
                                         1,
                                         GX_BG_AREAOVER_XLU,
                                         FALSE };

// The surveys, which ResearchList_UpdateListRects fills in as the list scrolls
static const ResearchRect sListItemRects[LIST_RECT_COUNT] = {
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
#ifdef BUGFIX
    { 0, 0, TOUCH_RECT_END, 0 },
#else
    // BUG: The same as sListScrollRects' end
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

static const BmpOamEntry sListBmpOams[BMP_COUNT] = {
    { 0, 144, OBJ_RES_MAIN_PALETTE, 2, 0, 0, 0, CLACT_VRAM_MAIN },
    { 160, 142, OBJ_RES_BUTTON_PALETTE, 1, 0, 0, 0, CLACT_VRAM_MAIN },
    { 160, 168, OBJ_RES_BUTTON_PALETTE, 1, 0, 0, 0, CLACT_VRAM_MAIN },
    { 0, 76, OBJ_RES_MAIN_PALETTE, 2, 0, 0, 0, CLACT_VRAM_MAIN },
};

static const ResearchActorSetup sListActors[ACTOR_COUNT] = {
    { 0, 0, 7, 3, 0, 1, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
    { 0, 0, 14, 3, 0, 1, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
    { 0, 0, 4, 0, 2, 1, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
};

static const ResearchPaletteAnimeSetup sListPaletteAnimes[ANIME_COUNT] = {
    { (u16 *)(HW_BG_PLTT + 0x146), (const u16 *)(HW_BG_PLTT + 0x146), 3, 0, 0xffff },
    { (u16 *)(HW_BG_PLTT + 0x140), (const u16 *)(HW_BG_PLTT + 0x140), 16, 4, 0xffff },
    { (u16 *)(HW_BG_PLTT + 0x140), (const u16 *)(HW_BG_PLTT + 0x140), 16, 3, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0x8c), (const u16 *)(HW_OBJ_PLTT + 0x8c), 1, 0, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0x80), (const u16 *)(HW_OBJ_PLTT + 0x80), 16, 4, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0x80), (const u16 *)(HW_OBJ_PLTT + 0x80), 16, 3, 0xffff },
};

static const BGFontEntry sListBGFonts[BG_FONT_COUNT] = {
    { { BG_SUB_TEXT, 2, 0, 28, 3, 0, 3, 15, 3, 4, 0 }, 0, 4, FALSE },
    { { BG_SUB_TEXT, 0, 18, 32, 6, 0, 5, 15, 3, 4, 0 }, 0, 5, FALSE },
    { { BG_SUB_TEXT, 0, 3, 32, 3, 0, 0, 15, 3, 4, 0 }, 1, 0, TRUE },
    { { BG_SUB_TEXT, 0, 6, 32, 4, 0, 0, 15, 3, 4, 0 }, 1, 0, TRUE },
    { { BG_SUB_TEXT, 3, 11, 30, 2, 0, 0, 15, 1, 2, 0 }, 1, 0, FALSE },
    { { BG_SUB_TEXT, 3, 13, 30, 2, 0, 0, 15, 1, 2, 0 }, 1, 0, FALSE },
    { { BG_SUB_TEXT, 3, 15, 30, 2, 0, 0, 15, 1, 2, 0 }, 1, 0, FALSE },
};

static const BitmapEntry sListBitmaps[BMP_COUNT] = {
    { 20, 6, 0x20, 0, 6, 10, 8, 15, 14, 0, ARCID_RESEARCH_RADAR, 18, TRUE },
    { 10, 3, 0x20, 0, 7, 10, 5, 14, 15, 0, ARCID_APP_MENU_COMMON, 32, FALSE },
    { 10, 3, 0x20, 0, 8, 10, 5, 14, 15, 0, ARCID_APP_MENU_COMMON, 32, FALSE },
    { 32, 4, 0x20, 0, 9, 0, 9, 15, 14, 0, ARCID_RESEARCH_RADAR, 21, TRUE },
};

// The names of the surveys in the list
static const BGFontEntry sListItemFonts[SURVEY_COUNT] = {
    { { BG_MAIN_TEXT, 2, 0, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 0, FALSE },
    { { BG_MAIN_TEXT, 2, 3, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 1, FALSE },
    { { BG_MAIN_TEXT, 2, 6, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 2, FALSE },
    { { BG_MAIN_TEXT, 2, 9, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 3, FALSE },
    { { BG_MAIN_TEXT, 2, 12, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 4, FALSE },
    { { BG_MAIN_TEXT, 2, 15, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 5, FALSE },
    { { BG_MAIN_TEXT, 2, 18, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 6, FALSE },
    { { BG_MAIN_TEXT, 2, 21, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 7, FALSE },
    { { BG_MAIN_TEXT, 2, 24, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 8, FALSE },
    { { BG_MAIN_TEXT, 2, 27, 28, 3, 8, 5, 15, 1, 2, 0 }, 1, 9, FALSE },
};

ResearchList *ResearchList_Create(ResearchCommon *common, ResearchListRecovery *recovery) {
    HeapID heapId = ResearchCommon_GetHeapID(common);
    ResearchList *wk = ResearchList_Alloc(heapId);

    ResearchList_InitWork(wk);
    ResearchList_SetHeapID(wk, heapId);
    ResearchList_SetCommon(wk, common);
    ResearchList_SetRecovery(wk, recovery);
    return wk;
}

void ResearchList_Delete(ResearchList *wk) {
    ResearchList_DeleteQueue(wk);
    ResearchList_Free(wk);
}

void ResearchList_Main(ResearchList *wk) {
    switch (ResearchList_GetSeq(wk)) {
    case SEQ_SETUP:
        ResearchList_SeqSetup(wk);
        break;
    case SEQ_TOUCH:
        ResearchList_SeqTouch(wk);
        break;
    case SEQ_CURSOR:
        ResearchList_SeqCursor(wk);
        break;
    case SEQ_CURSOR_SCROLL:
        ResearchList_SeqCursorScroll(wk);
        break;
    case SEQ_DRAG:
        ResearchList_SeqDrag(wk);
        break;
    case SEQ_RETURN_CURSOR:
        ResearchList_SeqReturnCursor(wk);
        break;
    case SEQ_RETURN_TOUCH:
        ResearchList_SeqReturnTouch(wk);
        break;
    case SEQ_CONFIRM_OPEN_TOUCH:
        ResearchList_SeqConfirmOpenTouch(wk);
        break;
    case SEQ_CONFIRM_TOUCH:
        ResearchList_SeqConfirmTouch(wk);
        break;
    case SEQ_CONFIRM_OPEN_CURSOR:
        ResearchList_SeqConfirmOpenCursor(wk);
        break;
    case SEQ_CONFIRM_CURSOR:
        ResearchList_SeqConfirmCursor(wk);
        break;
    case SEQ_CONFIRM_CANCEL:
        ResearchList_SeqConfirmCancel(wk);
        break;
    case SEQ_DECIDE:
        ResearchList_SeqDecide(wk);
        break;
    case SEQ_TEARDOWN:
        ResearchList_SeqTeardown(wk);
        break;
    case SEQ_END:
        break;
    }

    if (!ResearchList_IsEnd(wk)) {
        ResearchList_UpdateCommonPaletteAnime(wk);
        ResearchList_UpdatePaletteAnimes(wk);
        ClActSys_Draw();
    }
    ResearchList_CountFrame(wk);
    ResearchList_UpdateSeq(wk);
}

BOOL ResearchList_IsEnd(ResearchList *wk) {
    return wk->isEnd;
}

u32 ResearchList_GetNext(ResearchList *wk) {
    return wk->next;
}

static void ResearchList_SeqSetup(ResearchList *wk) {
    u8 current;

    ResearchList_CreateQueue(wk);
    ResearchList_CreateFont(wk);
    ResearchList_LoadMsgData(wk);
    ResearchList_InitTouchRects(wk);
    ResearchList_UpdateListRects(wk);
    ResearchList_CountSurveys(wk);
    ResearchList_InitBG(wk);
    ResearchList_LoadSubBG(wk);
    ResearchList_InitSubText(wk);
    ResearchList_LoadFrameBG(wk);
    ResearchList_LoadListBG(wk);
    ResearchList_InitMainText(wk);
    ResearchList_CreateBGFonts(wk);
    ResearchList_LoadSubObjRes(wk);
    ResearchList_LoadMainObjRes(wk);
    ResearchList_CreateUnits(wk);
    ResearchList_CreateActors(wk);
    ResearchList_CreateBitmaps(wk);
    ResearchList_DrawBitmaps(wk);
    ResearchList_DrawYesButton(wk);
    ResearchList_DrawNoButton(wk);
    ResearchList_CreateBmpOam(wk);
    ResearchList_CreateBmpOamActors(wk);
    ResearchList_CreatePaletteFade(wk);
    ResearchList_CreatePaletteAnimes(wk);
    ResearchList_SetupPaletteAnimes(wk);
    ResearchList_SetVBlank(wk);
    ResearchList_ShowCommIcon(wk);
    ResearchList_ShowCenterActor(wk);
    ResearchList_StartPaletteAnime(wk, ANIME_CURSOR);
    ResearchList_SetBmpOamVisible(wk, BMP_QUESTION, FALSE);
    ResearchList_SetBmpOamVisible(wk, BMP_YES, FALSE);
    ResearchList_SetBmpOamVisible(wk, BMP_NO, FALSE);
    ResearchList_LoadRecovery(wk);

    current = ResearchList_GetCurrentSurvey(wk);
    if (current != SURVEY_NONE) {
        ResearchList_IndentItem(wk, current);
        ResearchList_SetSelected(wk, current);
        ResearchList_UpdateCurrentIcon(wk);
    }

    ResearchList_UpdateListRects(wk);
    ResearchList_UpdateScrollBar(wk);
    ResearchList_UpdateCurrentIcon(wk);
    ResearchList_UpdateWindow(wk);
    ResearchList_PrintCursorSurvey(wk);

    if (!ResearchList_GetTouchMode(wk)) {
        ResearchList_ShowCursor(wk);
        ResearchList_ShowSurvey(wk);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CURSOR);
    } else {
        ResearchList_HideCursor(wk);
        ResearchList_HideSurvey(wk);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_TOUCH);
    }

    ResearchList_FadeIn();
}

static void ResearchList_SeqTouch(ResearchList *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 item = GFL_HIDGetTouchPressedRect(wk->listRects);
    s32 commonButton = GFL_HIDGetTouchPressedRect(ResearchCommon_GetTouchRects(wk->common));
    u32 x, y;

    GFL_HIDGetTouchHeldPoint(&x, &y);

    if (ResearchList_IsForceExit(wk) || commonButton == RESEARCH_COMMON_BUTTON_RETURN) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchList_StartReturnAnime(wk);
        ResearchList_SetTouchMode(wk, TRUE);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_RETURN_TOUCH);
    } else if (keys & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchList_StartReturnAnime(wk);
        ResearchList_SetTouchMode(wk, FALSE);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_RETURN_TOUCH);
    } else if ((keys & PAD_KEY_UP) || (keys & PAD_KEY_DOWN) || (keys & PAD_KEY_LEFT) || (keys & PAD_KEY_RIGHT) ||
               (keys & PAD_BUTTON_A)) {
        ResearchList_ShowCursor(wk);
        ResearchList_ShowSurvey(wk);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CURSOR);
    } else if (item >= 0 && item <= SURVEY_COUNT - 1) {
        if (y >= VIEW_TOP && y <= VIEW_BOTTOM && ResearchList_IsSurvey(wk, item) == TRUE) {
            ResearchList_MoveCursorTo(wk, item);
            ResearchList_SetSelected(wk, wk->cursor);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ResearchList_StartPaletteAnime(wk, ANIME_DECIDE);
            ResearchList_PrintCursorSurvey(wk);
            ResearchList_ShowSurvey(wk);
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_CONFIRM_OPEN_TOUCH);
        }
    } else if ((GFL_HIDGetTouchHeldRect(wk->scrollRects) == 0 || GFL_HIDGetTouchPressedRect(wk->scrollRects) == 0) &&
               ResearchList_CanScroll(wk) == TRUE) {
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_DRAG);
        ResearchList_PushSeq(wk, SEQ_TOUCH);
    }
}

static void ResearchList_SeqCursor(ResearchList *wk) {
    u32 held = GCTX_HIDGetHeldKeys();
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 item = GFL_HIDGetTouchPressedRect(wk->listRects);
    s32 commonButton = GFL_HIDGetTouchPressedRect(ResearchCommon_GetTouchRects(wk->common));
    u32 x, y;

    GFL_HIDGetTouchHeldPoint(&x, &y);

    if (held & (PAD_KEY_UP | PAD_KEY_DOWN)) {
        ResearchList_CountKeyRepeat(wk);
    } else {
        ResearchList_ResetKeyRepeat(wk);
    }

    if (ResearchList_IsForceExit(wk) || commonButton == RESEARCH_COMMON_BUTTON_RETURN) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchList_StartReturnAnime(wk);
        ResearchList_SetTouchMode(wk, TRUE);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_RETURN_CURSOR);
    } else if ((keys & PAD_KEY_UP) ||
               ((held & PAD_KEY_UP) && wk->cursor != 0 && ResearchList_GetKeyRepeatFrames(wk) > KEY_REPEAT_FRAMES)) {
        ResearchList_HideCursor(wk);
        ResearchList_SetCursorTarget(wk, -1);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CURSOR_SCROLL);
        ResearchList_PushSeq(wk, SEQ_CURSOR);
    } else if ((keys & PAD_KEY_DOWN) || ((held & PAD_KEY_DOWN) && wk->cursor < ResearchList_GetSurveyCount(wk) - 1 &&
                                         ResearchList_GetKeyRepeatFrames(wk) > KEY_REPEAT_FRAMES)) {
        ResearchList_HideCursor(wk);
        ResearchList_SetCursorTarget(wk, 1);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CURSOR_SCROLL);
        ResearchList_PushSeq(wk, SEQ_CURSOR);
    } else if (keys & PAD_BUTTON_A) {
        ResearchList_StartPaletteAnime(wk, ANIME_DECIDE);
        ResearchList_SetSelected(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ResearchList_PushSeq(wk, SEQ_CONFIRM_OPEN_CURSOR);
        ResearchList_EndSeq(wk);
    } else if (item >= 0 && item <= SURVEY_COUNT - 1) {
        if (y >= VIEW_TOP && y <= VIEW_BOTTOM && ResearchList_IsSurvey(wk, item) == TRUE) {
            ResearchList_MoveCursorTo(wk, item);
            ResearchList_SetSelected(wk, wk->cursor);
            ResearchList_StartPaletteAnime(wk, ANIME_DECIDE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ResearchList_PrintCursorSurvey(wk);
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_CONFIRM_OPEN_TOUCH);
        }
    } else if (keys & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchList_StartReturnAnime(wk);
        ResearchList_SetTouchMode(wk, FALSE);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_RETURN_CURSOR);
    } else if ((GFL_HIDGetTouchHeldRect(wk->scrollRects) == 0 || GFL_HIDGetTouchPressedRect(wk->scrollRects) == 0) &&
               ResearchList_CanScroll(wk) == TRUE) {
        ResearchList_HideCursor(wk);
        ResearchList_HideSurvey(wk);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_DRAG);
        ResearchList_PushSeq(wk, SEQ_TOUCH);
    }
}

static void ResearchList_SeqCursorScroll(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        ResearchList_StartCursorScroll(wk);
        ResearchList_MoveCursorTo(wk, wk->cursorTarget);
        ResearchList_PrintCursorSurvey(wk);
        ResearchList_NextSeqState(wk);
        break;
    case 1:
        ResearchList_UpdateScroll(wk);
        ResearchList_ScrollToView(wk);
        ResearchList_UpdateListRects(wk);
        ResearchList_UpdateScrollBar(wk);
        ResearchList_UpdateCurrentIcon(wk);
        ResearchList_UpdateWindow(wk);
        if (ResearchList_IsScrollEnd(wk)) {
            ResearchList_UpdateWindow(wk);
            ResearchList_EndSeq(wk);
        }
        break;
    }
}

static void ResearchList_SeqDrag(ResearchList *wk) {
    u32 x, y;
    BOOL touching = GFL_HIDGetTouchHeldPoint(&x, &y);

    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        if (!touching) {
            ResearchList_ReleaseDrag(wk);
            ResearchList_SetDragAccel(wk);
            ResearchList_NextSeqState(wk);
        } else {
            ResearchList_DragTo(wk, ResearchList_BarYToScroll(wk, y));
            ResearchList_ScrollToDrag(wk);
            ResearchList_UpdateListRects(wk);
            ResearchList_UpdateScrollBar(wk);
            ResearchList_UpdateCurrentIcon(wk);
            ResearchList_SetDragSpeed(wk);
        }
        break;
    case 1:
        ResearchList_DragTo(wk, wk->scrollY + wk->dragSpeed);
        ResearchList_ScrollToDrag(wk);
        ResearchList_UpdateListRects(wk);
        ResearchList_UpdateScrollBar(wk);
        ResearchList_UpdateCurrentIcon(wk);
        ResearchList_SlowDrag(wk);
        if (!ResearchList_IsDragging(wk)) {
            ResearchList_PrintCursorSurvey(wk);
            ResearchList_EndSeq(wk);
        }
        break;
    }

    ResearchList_FollowScroll(wk);
}

static void ResearchList_SeqConfirmTouch(ResearchList *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 button = GFL_HIDGetTouchPressedRect(wk->buttonRects);

    if ((keys & PAD_KEY_UP) || (keys & PAD_KEY_DOWN) || (keys & PAD_KEY_LEFT) || (keys & PAD_KEY_RIGHT) ||
        (keys & PAD_BUTTON_A)) {
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CONFIRM_OPEN_CURSOR);
    } else if (button == BUTTON_YES) {
        ResearchList_MoveButtonCursorTo(wk, BUTTON_YES);
        ResearchList_StopPaletteAnime(wk, ANIME_BUTTON_CURSOR);
        ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_DECIDE);
        ResearchList_SetTouchMode(wk, TRUE);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_DECIDE);
    } else if (ResearchList_IsForceExit(wk) || (keys & PAD_BUTTON_B) || button == BUTTON_NO) {
        ResearchList_MoveButtonCursorTo(wk, BUTTON_NO);
        ResearchList_StopPaletteAnime(wk, ANIME_BUTTON_CURSOR);
        ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_DECIDE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CONFIRM_CANCEL);
        ResearchList_PushSeq(wk, SEQ_CURSOR);
    }
}

static void ResearchList_SeqConfirmCursor(ResearchList *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 button = GFL_HIDGetTouchPressedRect(wk->buttonRects);

    if (keys & PAD_KEY_UP) {
        ResearchList_ButtonCursorUp(wk);
    }
    if (keys & PAD_KEY_DOWN) {
        ResearchList_ButtonCursorDown(wk);
    }

    if (keys & PAD_BUTTON_A) {
        ResearchList_StopPaletteAnime(wk, ANIME_BUTTON_CURSOR);
        ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_DECIDE);
        switch (wk->buttonCursor) {
        case BUTTON_YES:
            ResearchList_SetTouchMode(wk, FALSE);
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_DECIDE);
            break;
        case BUTTON_NO:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_CONFIRM_CANCEL);
            ResearchList_PushSeq(wk, SEQ_CURSOR);
            break;
        }
    } else if (button == BUTTON_YES) {
        ResearchList_MoveButtonCursorTo(wk, BUTTON_YES);
        ResearchList_StopPaletteAnime(wk, ANIME_BUTTON_CURSOR);
        ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_DECIDE);
        ResearchList_SetTouchMode(wk, TRUE);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_DECIDE);
    } else if (ResearchList_IsForceExit(wk) || (keys & PAD_BUTTON_B) || button == BUTTON_NO) {
        ResearchList_MoveButtonCursorTo(wk, BUTTON_NO);
        ResearchList_StopPaletteAnime(wk, ANIME_BUTTON_CURSOR);
        ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_DECIDE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchList_EndSeq(wk);
        ResearchList_PushSeq(wk, SEQ_CONFIRM_CANCEL);
        ResearchList_PushSeq(wk, SEQ_CURSOR);
    }
}

static void ResearchList_SeqConfirmOpenTouch(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        ResearchList_UnhighlightButton(wk, BUTTON_NO);
        ResearchList_UnhighlightButton(wk, BUTTON_YES);
        ResearchList_SetBmpOamVisible(wk, BMP_QUESTION, TRUE);
        ResearchList_SetBmpOamVisible(wk, BMP_YES, TRUE);
        ResearchList_SetBmpOamVisible(wk, BMP_NO, TRUE);
        if (!wk->darkened) {
            BGFont_SetPalette(wk->itemFonts[wk->cursor], 14);
            ResearchList_Darken(wk);
        }
        ResearchList_StartScrollToCursor(wk);
        ResearchList_NextSeqState(wk);
        break;
    case 1:
        ResearchList_UpdateScroll(wk);
        ResearchList_ScrollToView(wk);
        ResearchList_UpdateListRects(wk);
        ResearchList_UpdateScrollBar(wk);
        ResearchList_UpdateCurrentIcon(wk);
        ResearchList_UpdateWindow(wk);
        if (ResearchList_IsScrollEnd(wk)) {
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_CONFIRM_TOUCH);
        }
        break;
    }
}

static void ResearchList_SeqConfirmOpenCursor(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        ResearchList_SetButtonCursor(wk, BUTTON_YES);
        ResearchList_UnhighlightButton(wk, BUTTON_NO);
        ResearchList_HighlightButton(wk, BUTTON_YES);
        ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_CURSOR);
        ResearchList_SetBmpOamVisible(wk, BMP_QUESTION, TRUE);
        ResearchList_SetBmpOamVisible(wk, BMP_YES, TRUE);
        ResearchList_SetBmpOamVisible(wk, BMP_NO, TRUE);
        if (!wk->darkened) {
            BGFont_SetPalette(wk->itemFonts[wk->cursor], 14);
            ResearchList_Darken(wk);
        }
        ResearchList_StartScrollToCursor(wk);
        ResearchList_NextSeqState(wk);
        break;
    case 1:
        ResearchList_UpdateScroll(wk);
        ResearchList_ScrollToView(wk);
        ResearchList_UpdateListRects(wk);
        ResearchList_UpdateScrollBar(wk);
        ResearchList_UpdateCurrentIcon(wk);
        ResearchList_UpdateWindow(wk);
        if (ResearchList_IsScrollEnd(wk)) {
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_CONFIRM_CURSOR);
        }
        break;
    }
}

static void ResearchList_SeqDecide(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        ResearchList_BrightenCurrentIcon(wk);
        ResearchList_UnindentItem(wk, ResearchList_GetCurrentSurvey(wk));
        ResearchList_SetCurrentSurvey(wk);
        ResearchList_IndentItem(wk, ResearchList_GetSelected(wk));
        ResearchList_UpdateCurrentIcon(wk);
        ResearchList_SetBmpOamVisible(wk, BMP_DECIDED, TRUE);
        GFL_SndSEPlay(SEQ_SE_SYS_80);
        ResearchList_NextSeqState(wk);
        break;
    case 1:
        if (wk->seqFrames >= DECIDE_WAIT_FRAMES) {
            ResearchList_FadeOut();
            ResearchList_NextSeqState(wk);
        }
        break;
    case 2:
        if (ResearchList_IsFadeDone()) {
            ResearchList_Brighten(wk);
            ResearchList_NextSeqState(wk);
        }
        break;
    case 3:
        if (ResearchList_IsPaletteFadeDone(wk)) {
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_TEARDOWN);
        }
        break;
    }
}

static void ResearchList_SeqReturnCursor(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        if (ResearchList_GetWait(wk) < wk->seqFrames) {
            ResearchList_FadeOut();
            ResearchList_NextSeqState(wk);
        }
        break;
    case 1:
        if (ResearchList_IsFadeDone()) {
            ResearchList_Brighten(wk);
            ResearchList_NextSeqState(wk);
        }
        break;
    case 2:
        if (ResearchList_IsPaletteFadeDone(wk)) {
            BGFont_SetPalette(wk->itemFonts[wk->cursor], 15);
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_TEARDOWN);
        }
        break;
    }
}

static void ResearchList_SeqReturnTouch(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        if (ResearchList_GetWait(wk) < wk->seqFrames) {
            ResearchList_FadeOut();
            ResearchList_NextSeqState(wk);
        }
        break;
    case 1:
        if (ResearchList_IsFadeDone()) {
            ResearchList_Brighten(wk);
            ResearchList_NextSeqState(wk);
        }
        break;
    case 2:
        if (ResearchList_IsPaletteFadeDone(wk)) {
            BGFont_SetPalette(wk->itemFonts[wk->cursor], 15);
            ResearchList_EndSeq(wk);
            ResearchList_PushSeq(wk, SEQ_TEARDOWN);
        }
        break;
    }
}

static void ResearchList_SeqConfirmCancel(ResearchList *wk) {
    switch (ResearchList_GetSeqState(wk)) {
    case 0:
        ResearchList_StartScrollBack(wk);
        ResearchList_NextSeqState(wk);
        break;
    case 1:
        ResearchList_UpdateScroll(wk);
        ResearchList_ScrollToView(wk);
        ResearchList_UpdateListRects(wk);
        ResearchList_UpdateScrollBar(wk);
        ResearchList_UpdateCurrentIcon(wk);
        ResearchList_UpdateWindow(wk);
        if (ResearchList_IsScrollEnd(wk)) {
            ResearchList_NextSeqState(wk);
        }
        break;
    case 2:
        if (ResearchList_IsPaletteAnimeEnd(wk, ANIME_BUTTON_DECIDE)) {
            ResearchList_SetBmpOamVisible(wk, BMP_QUESTION, FALSE);
            ResearchList_SetBmpOamVisible(wk, BMP_YES, FALSE);
            ResearchList_SetBmpOamVisible(wk, BMP_NO, FALSE);
            ResearchList_UpdateWindow(wk);
            ResearchList_Brighten(wk);
            ResearchList_NextSeqState(wk);
        }
        break;
    case 3:
        if (ResearchList_IsPaletteFadeDone(wk)) {
            BGFont_SetPalette(wk->itemFonts[wk->cursor], 15);
            ResearchList_EndSeq(wk);
        }
        break;
    }
}

static void ResearchList_SeqTeardown(ResearchList *wk) {
    ResearchList_SaveRecovery(wk);
    ResearchList_ResetVBlank(wk);
    ResearchList_RestorePalettes(wk);
    ResearchList_DeletePaletteAnimes(wk);
    ResearchCommon_StopPaletteAnime(wk->common);
    ResearchCommon_RestorePalette(wk->common);
    ResearchList_DeletePaletteFade(wk);
    ResearchList_FreeBitmaps(wk);
    ResearchList_DeleteBmpOamActors(wk);
    ResearchList_DeleteBmpOam(wk);
    ResearchList_DeleteActors(wk);
    ResearchList_DeleteUnits(wk);
    ResearchList_FreeSubObjRes(wk);
    ResearchList_FreeMainObjRes(wk);
    ResearchList_DeleteBGFonts(wk);
    ResearchList_ExitMainText(wk);
    ResearchList_UnloadFrameBG(wk);
    ResearchList_UnloadListBG(wk);
    ResearchList_ExitSubText(wk);
    ResearchList_UnloadSubBG(wk);
    ResearchList_ExitBG(wk);
    ResearchList_FreeMsgData(wk);
    ResearchList_DeleteFont(wk);
    ResearchList_SetNext(wk, RESEARCH_LIST_NEXT_TOP);
    ResearchList_EndSeq(wk);
    ResearchList_PushSeq(wk, SEQ_END);
    wk->isEnd = TRUE;
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
}

static void ResearchList_CountFrame(ResearchList *wk) {
    wk->seqFrames++;
}

static void ResearchList_PushSeq(ResearchList *wk, u32 seq) {
    Queue_Push(wk->queue, seq);
}

static void ResearchList_EndSeq(ResearchList *wk) {
    wk->seqDone = TRUE;
}

static BOOL ResearchList_GetTouchMode(ResearchList *wk) {
    return ResearchCommon_GetTouchMode(wk->common);
}

static void ResearchList_SetTouchMode(ResearchList *wk, BOOL touch) {
    ResearchCommon_SetTouchMode(wk->common, touch);
}

static void ResearchList_SetNext(ResearchList *wk, u32 next) {
    wk->next = next;
}

static u32 ResearchList_GetWait(ResearchList *wk) {
    return wk->waitFrames;
}

static void ResearchList_UpdateSeq(ResearchList *wk) {
    if (wk->seqDone && !Queue_IsEmpty(wk->queue)) {
        ResearchList_SetSeq(wk, Queue_Pop(wk->queue));
    }
}

static u32 ResearchList_GetSeq(ResearchList *wk) {
    return wk->seq;
}

static void ResearchList_SetSeq(ResearchList *wk, u32 seq) {
    wk->seq = seq;
    wk->seqFrames = 0;
    wk->seqDone = FALSE;
    ResearchList_ResetSeqState(wk);
}

static u32 ResearchList_GetSeqState(ResearchList *wk) {
    return wk->seqState;
}

static void ResearchList_NextSeqState(ResearchList *wk) {
    wk->seqState++;
}

static void ResearchList_ResetSeqState(ResearchList *wk) {
    wk->seqState = 0;
}

static void ResearchList_ButtonCursorUp(ResearchList *wk) {
    ResearchList_UnhighlightButton(wk, wk->buttonCursor);
    ResearchList_MoveButtonCursor(wk, -1);
    ResearchList_HighlightButton(wk, wk->buttonCursor);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_MOVE);
}

static void ResearchList_ButtonCursorDown(ResearchList *wk) {
    ResearchList_UnhighlightButton(wk, wk->buttonCursor);
    ResearchList_MoveButtonCursor(wk, 1);
    ResearchList_HighlightButton(wk, wk->buttonCursor);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_MOVE);
}

static void ResearchList_MoveButtonCursorTo(ResearchList *wk, int pos) {
    ResearchList_UnhighlightButton(wk, wk->buttonCursor);
    ResearchList_SetButtonCursor(wk, pos);
    ResearchList_HighlightButton(wk, wk->buttonCursor);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchList_StartPaletteAnime(wk, ANIME_BUTTON_MOVE);
}

// Keeps the cursor on a survey in the view, moving it to the next one in when the view scrolls past it
static void ResearchList_FollowScroll(ResearchList *wk) {
    int top = ResearchList_GetViewTop();
    int bottom = ResearchList_GetViewBottom();
    int itemTop = ResearchList_GetItemTop(wk->cursor);
    int itemBottom = ResearchList_GetItemBottom(wk->cursor);
    u8 pos;

    if (itemBottom <= top) {
        pos = ResearchList_GetItemAt(itemBottom + 1);
        if (wk->cursorShown) {
            ResearchList_MoveCursorTo(wk, pos);
        } else {
            ResearchList_SetCursor(wk, pos);
        }
    }

    if (bottom <= itemTop) {
        pos = ResearchList_GetItemAt(itemTop - 1);
        if (wk->cursorShown) {
            ResearchList_MoveCursorTo(wk, pos);
        } else {
            ResearchList_SetCursor(wk, pos);
        }
    }
}

static void ResearchList_MoveCursorTo(ResearchList *wk, u8 pos) {
    ResearchList_HideCursor(wk);
    ResearchList_SetCursor(wk, pos);
    ResearchList_ShowCursor(wk);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchList_StartPaletteAnime(wk, ANIME_MOVE);
}

static void ResearchList_StartReturnAnime(ResearchList *wk) {
    ResearchList_StartCommonPaletteAnime(wk, 0);
}

// Scrolls from the view's edge to the survey the cursor moves to, at once when both are in the view
static void ResearchList_StartCursorScroll(ResearchList *wk) {
    int from = wk->scrollY;
    int to;
    int top;
    int bottom;
    int frames;

    if (wk->cursor < wk->cursorTarget) {
        to = ResearchList_GetItemBottom(wk->cursorTarget) + 1;
    } else {
        to = ResearchList_GetItemTop(wk->cursorTarget);
    }

    top = ResearchList_GetViewTop();
    bottom = ResearchList_GetViewBottom();
    if (to < top || bottom < to) {
        if (from < to) {
            from = bottom;
        } else {
            from = top;
        }
    }

    frames = (MATH_MAX(from, to) - MATH_MIN(from, to)) / 4;
    top = ResearchList_GetViewTop();
    bottom = ResearchList_GetViewBottom();
    if (top <= from && from <= bottom && top <= to && to <= bottom) {
        frames = 0;
    }
    ResearchList_StartScroll(wk, from, to, frames);
}

// Scrolls the list back inside its ends after the question
static void ResearchList_StartScrollBack(ResearchList *wk) {
    int from = wk->scrollY;
    int to = from;
    int top;
    int bottom;
    int frames;

    if (ResearchList_GetListHeight(wk) < from) {
        to = ResearchList_GetMaxBGScroll(wk) + VIEW_TOP;
    } else if (from < 0) {
        to = VIEW_BOTTOM - VIEW_TOP;
    }

    top = ResearchList_GetViewTop();
    bottom = ResearchList_GetViewBottom();
    if (to < top || bottom < to) {
        if (from < to) {
            from = bottom;
        } else {
            from = top;
        }
    }

    frames = (MATH_MAX(from, to) - MATH_MIN(from, to)) / 3;
    top = ResearchList_GetViewTop();
    bottom = ResearchList_GetViewBottom();
    if (top <= from && from <= bottom && top <= to && to <= bottom) {
        frames = 1;
    }
    ResearchList_StartScroll(wk, from, to, frames);
}

// Scrolls the survey of the cursor to the middle of the view
static void ResearchList_StartScrollToCursor(ResearchList *wk) {
    int center = (ResearchList_GetViewTop() + ResearchList_GetViewBottom()) / 2;
    int y = ResearchList_GetItemRow(wk, wk->cursor) * 8;
    int from;
    int to;

    if (center < y) {
        from = ResearchList_GetViewBottom();
        to = ResearchList_GetViewBottom() + (y - center);
    } else {
        from = ResearchList_GetViewTop();
        to = ResearchList_GetViewTop() - (center - y);
    }
    ResearchList_StartScroll(wk, from, to, (MATH_MAX(from, to) - MATH_MIN(from, to)) / 3);
}

// The survey being run, from its questions in the save
static u8 ResearchList_GetCurrentSurvey(ResearchList *wk) {
    u8 questions[QUESTION_COUNT];
    int i;
    void *survey = SurveySave_Get(GameData_GetSaveControl(ResearchList_GetGameData(wk)));
    u8 index;

    for (i = 0; i < QUESTION_COUNT; i++) {
        questions[i] = SurveySave_GetQuestion(survey, i);
    }

    for (index = 0; index < SURVEY_COUNT; index++) {
        if (questions[0] == sListSurveyQuestions1[index] && questions[1] == sListSurveyQuestions2[index] &&
            questions[2] == sListSurveyQuestions3[index]) {
            return index;
        }
    }
    // Questions of no survey give the first, though some callers check for SURVEY_NONE. Returning SURVEY_NONE instead
    // would not fix it: ResearchList_SeqDecide passes the result on to ResearchList_UnindentItem unchecked
    return 0;
}

// Makes the chosen survey the one being run
static void ResearchList_SetCurrentSurvey(ResearchList *wk) {
    u8 index = ResearchList_GetSelected(wk);
    void *survey = SurveySave_Get(GameData_GetSaveControl(ResearchList_GetGameData(wk)));

    SurveySave_SetQuestion(survey, sListSurveyQuestions1[index], 0);
    SurveySave_SetQuestion(survey, sListSurveyQuestions2[index], 1);
    SurveySave_SetQuestion(survey, sListSurveyQuestions3[index], 2);
}

static void ResearchList_MoveButtonCursor(ResearchList *wk, int dir) {
    wk->buttonCursor = (wk->buttonCursor + dir + BUTTON_COUNT) % BUTTON_COUNT;
}

static void ResearchList_SetButtonCursor(ResearchList *wk, int pos) {
    wk->buttonCursor = pos;
}

static void ResearchList_HighlightButton(ResearchList *wk, u32 button) {
    BmpOam_ActorSetPaletteOffset(ResearchList_GetButtonBmpOam(wk, button), 0);
}

static void ResearchList_UnhighlightButton(ResearchList *wk, u32 button) {
    BmpOam_ActorSetPaletteOffset(ResearchList_GetButtonBmpOam(wk, button), 1);
}

static void ResearchList_SetCursor(ResearchList *wk, u8 pos) {
    wk->cursor = pos;
}

static void ResearchList_SetCursorTarget(ResearchList *wk, int dir) {
    u8 cursor = wk->cursor;

    wk->cursorTarget = (cursor + dir + ResearchList_GetSurveyCount(wk)) % ResearchList_GetSurveyCount(wk);
}

static void ResearchList_ShowCursor(ResearchList *wk) {
    u8 cursor = wk->cursor;

    GFL_BGSysSetScrPaletteNo(BG_MAIN_LIST, ResearchList_GetItemColumn(wk, cursor), ResearchList_GetItemRow(wk, cursor),
                             28, ITEM_ROWS, 10);
    GFL_BGSysQueueScrLoad(BG_MAIN_LIST);
    wk->cursorShown = TRUE;
}

static void ResearchList_HideCursor(ResearchList *wk) {
    u8 cursor = wk->cursor;

    GFL_BGSysSetScrPaletteNo(BG_MAIN_LIST, ResearchList_GetItemColumn(wk, cursor), ResearchList_GetItemRow(wk, cursor),
                             28, ITEM_ROWS, 9);
    GFL_BGSysQueueScrLoad(BG_MAIN_LIST);
    wk->cursorShown = FALSE;
}

// Moves a survey's tiles a tile to the left, as the survey being run is drawn
static void ResearchList_IndentItem(ResearchList *wk, u8 index) {
    u16 *listScreen = GFL_BGSysIsScrHeapExists(BG_MAIN_LIST);
    u16 *textScreen = GFL_BGSysIsScrHeapExists(BG_MAIN_TEXT);
    int top;
    int i;
    int x;
    int src;
    int dst;

    for (i = 0; i < ITEM_ROWS; i++) {
        for (x = 0; x < 28; x++) {
            top = ResearchList_GetItemRow(wk, index);
            src = (x + 2) + (top + i) * 32;
            dst = src - 1;
            listScreen[dst] = listScreen[src];
            textScreen[dst] = textScreen[src];
        }
    }

    top = ResearchList_GetItemRow(wk, index);
    listScreen[top * 32 + 29] = listScreen[0];
    textScreen[top * 32 + 29] = textScreen[0];
    listScreen[(top + 1) * 32 + 29] = listScreen[0];
    textScreen[(top + 1) * 32 + 29] = textScreen[0];
    listScreen[(top + 2) * 32 + 29] = listScreen[0];
    textScreen[(top + 2) * 32 + 29] = textScreen[0];
    GFL_BGSysQueueScrLoad(BG_MAIN_LIST);
    GFL_BGSysQueueScrLoad(BG_MAIN_TEXT);
}

static void ResearchList_UnindentItem(ResearchList *wk, u8 index) {
    u16 *listScreen = GFL_BGSysIsScrHeapExists(BG_MAIN_LIST);
    u16 *textScreen = GFL_BGSysIsScrHeapExists(BG_MAIN_TEXT);
    int top;
    int i;
    int x;
    int src;
    int dst;

    for (i = ITEM_ROWS - 1; i >= 0; i--) {
        for (x = 27; x >= 0; x--) {
            top = ResearchList_GetItemRow(wk, index);
            src = (x + 1) + (top + i) * 32;
            dst = src + 1;
            listScreen[dst] = listScreen[src];
            textScreen[dst] = textScreen[src];
        }
    }

    top = ResearchList_GetItemRow(wk, index);
    listScreen[top * 32 + 1] = listScreen[0];
    textScreen[top * 32 + 1] = textScreen[0];
    listScreen[(top + 1) * 32 + 1] = listScreen[0];
    textScreen[(top + 1) * 32 + 1] = textScreen[0];
    listScreen[(top + 2) * 32 + 1] = listScreen[0];
    textScreen[(top + 2) * 32 + 1] = textScreen[0];
    GFL_BGSysQueueScrLoad(BG_MAIN_LIST);
    GFL_BGSysQueueScrLoad(BG_MAIN_TEXT);
}

// The tile column of a survey in the list, a tile to the left for the survey being run
static u8 ResearchList_GetItemColumn(ResearchList *wk, u8 index) {
    u8 x = 2;

    if (index == ResearchList_GetCurrentSurvey(wk)) {
        x--;
    }
    return x;
}

static u8 ResearchList_GetItemRow(ResearchList *wk, u8 index) {
    return index * ITEM_ROWS;
}

static int ResearchList_GetItemScreenX(ResearchList *wk, u8 index) {
    int x = ResearchList_GetItemColumn(wk, index) * 8;

    return x - GFL_BGSysGetBGOffsetX2(BG_MAIN_LIST);
}

static int ResearchList_GetItemScreenTop(ResearchList *wk, u8 index) {
    int y = ResearchList_GetItemRow(wk, index) * 8;

    return y - ResearchList_GetBGScroll();
}

static int ResearchList_GetItemScreenBottom(ResearchList *wk, u8 index) {
    int y = ResearchList_GetItemRow(wk, index) * 8 + ITEM_HEIGHT - 1;

    return y - ResearchList_GetBGScroll();
}

// Shows the parts of the screen above and below the list, where it has been scrolled off its ends, without the list
static void ResearchList_UpdateWindow(ResearchList *wk) {
    BOOL enable = FALSE;
    int top = ResearchList_GetItemScreenTop(wk, 0);
    int bottom = ResearchList_GetItemScreenBottom(wk, SURVEY_COUNT - 1);
    int x1, y1, x2, y2;

    if (top > 0) {
        enable = TRUE;
        x1 = 0;
        x2 = 240;
        y1 = 0;
        y2 = top - 1;
    } else if (bottom < 192) {
        enable = TRUE;
        x1 = 0;
        x2 = 240;
        y1 = bottom + 1;
        y2 = 192;
    }

    if (enable) {
        GX_SetVisibleWnd(GX_WNDMASK_W0);
        G2_SetWnd0Position(x1, y1, x2, y2);
        G2_SetWndOutsidePlane(GX_PLANEMASK_ALL, TRUE);
        G2_SetWnd0InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_OBJ, TRUE);
    } else {
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
    }
}

static void ResearchList_UpdateCurrentIcon(ResearchList *wk) {
    int current = ResearchList_GetCurrentSurvey(wk);
    ClActor *actor = ResearchList_GetActor(wk, ACTOR_CURRENT);
    ClActorPos pos;

    if (current == SURVEY_NONE) {
        ClActor_SetVisible(actor, FALSE);
        return;
    }

    pos.x = ResearchList_GetItemScreenX(wk, current);
    pos.y = ResearchList_GetItemScreenTop(wk, current) + 10;
    ClActor_SetSurfacePos(actor, &pos, 0);
    ClActor_SetAutoAnim(actor, TRUE);
    ClActor_SetVisible(actor, TRUE);
}

static void ResearchList_PrintCursorSurvey(ResearchList *wk) {
    ResearchList_PrintSurvey(wk, wk->cursor);
}

static void ResearchList_PrintSurvey(ResearchList *wk, u8 index) {
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_NAME], sListSurveyNames[index]);
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_DESCRIPTION], sListSurveyDescriptions[index]);
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_QUESTION_1], sListQuestionMsgs[sListSurveyQuestions1[index]]);
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_QUESTION_2], sListQuestionMsgs[sListSurveyQuestions2[index]]);
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_QUESTION_3], sListQuestionMsgs[sListSurveyQuestions3[index]]);
}

static void ResearchList_HideSurvey(ResearchList *wk) {
    BGFont_SetVisible(wk->bgFonts[BG_FONT_NAME], FALSE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_DESCRIPTION], FALSE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_QUESTION_1], FALSE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_QUESTION_2], FALSE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_QUESTION_3], FALSE);
}

static void ResearchList_ShowSurvey(ResearchList *wk) {
    BGFont_SetVisible(wk->bgFonts[BG_FONT_NAME], TRUE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_DESCRIPTION], TRUE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_QUESTION_1], TRUE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_QUESTION_2], TRUE);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_QUESTION_3], TRUE);
}

static void ResearchList_UpdateScrollBar(ResearchList *wk) {
    ClActor *actor = ResearchList_GetActor(wk, ACTOR_SCROLL_BAR);
    ClActorPos pos;

    if (!ResearchList_CanScroll(wk)) {
        ClActor_SetVisible(actor, FALSE);
        return;
    }

    pos.x = 248;
    pos.y = ResearchList_GetBarY(wk);
    ClActor_SetSurfacePos(actor, &pos, 0);
    ClActor_SetVisible(actor, TRUE);
}

static int ResearchList_GetBarY(ResearchList *wk) {
    int scroll = ResearchList_GetBGScroll();
    int max = ResearchList_GetMaxBGScroll(wk);
    f32 ratio;
    int y;

    scroll += VIEW_TOP;
    max += VIEW_TOP;
    ratio = (f32)scroll / (f32)max;
    y = ratio * (BAR_BOTTOM - BAR_TOP) + BAR_TOP;
    if (y < BAR_TOP) {
        y = BAR_TOP;
    }
    if (y > BAR_BOTTOM) {
        y = BAR_BOTTOM;
    }
    return y;
}

static int ResearchList_BarYToScroll(ResearchList *wk, int y) {
    f32 ratio;
    int min;
    int max;

    if (y < BAR_TOP) {
        y = BAR_TOP;
    }
    if (y > BAR_BOTTOM) {
        y = BAR_BOTTOM;
    }

    ratio = (f32)(y - BAR_TOP) / (BAR_BOTTOM - BAR_TOP);
    min = 0;
    max = ResearchList_GetMaxBGScroll(wk) + VIEW_TOP;
    return min + ratio * (max - min);
}

static void ResearchList_SlowDrag(ResearchList *wk) {
    int speed = wk->dragSpeed + wk->dragAccel;

    if (wk->dragSpeed * speed <= 0) {
        speed = 0;
    }
    wk->dragSpeed = speed;
}

static BOOL ResearchList_IsDragging(ResearchList *wk) {
    if (wk->dragSpeed != 0) {
        return TRUE;
    }
    return FALSE;
}

static void ResearchList_SetDragSpeed(ResearchList *wk) {
    wk->dragSpeed = wk->scrollY - wk->prevScrollY;
}

static void ResearchList_ReleaseDrag(ResearchList *wk) {
    wk->dragSpeed *= 1.5;
    if (wk->dragSpeed > DRAG_SPEED_MAX) {
        wk->dragSpeed = DRAG_SPEED_MAX;
    } else if (wk->dragSpeed < -DRAG_SPEED_MAX) {
        wk->dragSpeed = -DRAG_SPEED_MAX;
    }
}

static void ResearchList_SetDragAccel(ResearchList *wk) {
    if (wk->dragSpeed < 0) {
        wk->dragAccel = 1;
    } else {
        wk->dragAccel = -1;
    }
}

static void ResearchList_StartScroll(ResearchList *wk, int from, int to, int frames) {
    wk->scrollFrom = from;
    wk->scrollTo = to;
    wk->scrollFrames = frames;
    wk->scrollFrame = 0;
}

static void ResearchList_UpdateScroll(ResearchList *wk) {
    wk->scrollFrame++;
    ResearchList_SetScrollY(wk, wk->scrollFrom + (wk->scrollTo - wk->scrollFrom) * wk->scrollFrame / wk->scrollFrames);
}

static BOOL ResearchList_IsScrollEnd(ResearchList *wk) {
    if (wk->scrollFrames <= wk->scrollFrame) {
        return TRUE;
    }
    return FALSE;
}

static void ResearchList_DragTo(ResearchList *wk, int y) {
    if (y < 0) {
        y = 0;
    }
    if (ResearchList_GetListHeight(wk) < y) {
        y = ResearchList_GetListHeight(wk);
    }
    wk->prevScrollY = wk->scrollY;
    wk->scrollY = y;
}

static void ResearchList_SetScrollY(ResearchList *wk, int y) {
    wk->prevScrollY = y;
    wk->scrollY = y;
}

// Scrolls the view to scrollY when it is outside
static void ResearchList_ScrollToView(ResearchList *wk) {
    int y = wk->scrollY;
    int top = ResearchList_GetViewTop();
    int bottom = ResearchList_GetViewBottom();
    int bgY;

    if (y < top) {
        bgY = y - VIEW_TOP;
        GFL_BGSysMoveBGReq(BG_MAIN_LIST, BG_MOVE_SET_Y, bgY);
        GFL_BGSysMoveBGReq(BG_MAIN_TEXT, BG_MOVE_SET_Y, bgY);
    } else if (bottom < y) {
        bgY = y - VIEW_BOTTOM;
        GFL_BGSysMoveBGReq(BG_MAIN_LIST, BG_MOVE_SET_Y, bgY);
        GFL_BGSysMoveBGReq(BG_MAIN_TEXT, BG_MOVE_SET_Y, bgY);
    }
}

static void ResearchList_ScrollToDrag(ResearchList *wk) {
    int y = wk->scrollY - VIEW_TOP;

    if (y < -VIEW_TOP) {
        y = -VIEW_TOP;
    }
    if (ResearchList_GetMaxBGScroll(wk) < y) {
        y = ResearchList_GetMaxBGScroll(wk);
    }
    ResearchList_SetBGScroll(y);
}

static int ResearchList_GetBGScroll(void) {
    return GFL_BGSysGetBGOffsetY(BG_MAIN_LIST);
}

static void ResearchList_SetBGScroll(int y) {
    GFL_BGSysMoveBG(BG_MAIN_LIST, BG_MOVE_SET_Y, y);
    GFL_BGSysMoveBG(BG_MAIN_TEXT, BG_MOVE_SET_Y, y);
}

static int ResearchList_GetViewTop(void) {
    return ResearchList_GetBGScroll() + VIEW_TOP;
}

static int ResearchList_GetViewBottom(void) {
    return ResearchList_GetBGScroll() + VIEW_BOTTOM;
}

static int ResearchList_GetItemAt(int y) {
    return y / ITEM_HEIGHT;
}

static int ResearchList_GetItemTop(int index) {
    return index * ITEM_HEIGHT;
}

static int ResearchList_GetItemBottom(int index) {
    return ResearchList_GetItemTop(index) + ITEM_HEIGHT - 1;
}

static int ResearchList_GetMaxBGScroll(ResearchList *wk) {
    return ResearchList_GetSurveyCount(wk) * ITEM_HEIGHT - VIEW_BOTTOM;
}

static int ResearchList_GetListHeight(ResearchList *wk) {
    return ResearchList_GetMaxBGScroll(wk) + VIEW_BOTTOM;
}

static BOOL ResearchList_CanScroll(ResearchList *wk) {
    if (ResearchList_GetSurveyCount(wk) * ITEM_HEIGHT > VIEW_BOTTOM - VIEW_TOP) {
        return TRUE;
    }
    return FALSE;
}

// Moves the surveys' touch rectangles with the list, clipped at the screen's top and left
static void ResearchList_UpdateListRects(ResearchList *wk) {
    int i;

    for (i = 0; i <= SURVEY_COUNT - 1; i++) {
        int x = ResearchList_GetItemScreenX(wk, i);
        int y = ResearchList_GetItemScreenTop(wk, i);
        int left = x;
        int right;
        int top;
        int bottom;

        if (x < 0) {
            left = 0;
        }
        right = x + 216;
        if (right < 0) {
            right = 0;
        }
        top = y;
        if (y < 0) {
            top = 0;
        }
        bottom = y + ITEM_HEIGHT;
        if (bottom < 0) {
            bottom = 0;
        }
        wk->listRects[i].left = left;
        wk->listRects[i].right = right;
        wk->listRects[i].top = top;
        wk->listRects[i].bottom = bottom;
    }
}

static void ResearchList_FadeIn(void) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
}

static void ResearchList_FadeOut(void) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
}

static BOOL ResearchList_IsFadeDone(void) {
    if (!GFL_FadeIsRunning()) {
        return TRUE;
    }
    return FALSE;
}

// Darkens the main engine's BG palettes but the list's and its cursor's, and its OBJ palettes but the question's and
// the icon of the survey being run, unless that is the one chosen
static void ResearchList_Darken(ResearchList *wk) {
    u16 objMask;

    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbbff, 2, 0, 10, 0, wk->tcbMgr);
    if (ResearchList_GetCurrentSurvey(wk) == ResearchList_GetSelected(wk)) {
        objMask = 0xcf;
    } else {
        objMask = 0x4cf;
    }
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, objMask, 2, 0, 10, 0, wk->tcbMgr);
    wk->darkened = TRUE;
}

static void ResearchList_Brighten(ResearchList *wk) {
    u16 objMask;

    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbbff, 2, 10, 0, 0, wk->tcbMgr);
    if (ResearchList_GetCurrentSurvey(wk) == ResearchList_GetSelected(wk)) {
        objMask = 0xcf;
    } else {
        objMask = 0x4cf;
    }
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, objMask, 2, 10, 0, 0, wk->tcbMgr);
    wk->darkened = FALSE;
}

static void ResearchList_BrightenCurrentIcon(ResearchList *wk) {
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0x400, 0, 10, 0, 0, wk->tcbMgr);
}

static BOOL ResearchList_IsPaletteFadeDone(ResearchList *wk) {
    if (!PaletteFade_GetActiveMask(wk->paletteFade)) {
        return TRUE;
    }
    return FALSE;
}

static void ResearchList_StartCommonPaletteAnime(ResearchList *wk, u32 index) {
    ResearchCommon_StartPaletteAnime(wk->common, index);
}

static void ResearchList_StartPaletteAnime(ResearchList *wk, u32 index) {
    PaletteAnime_Start(wk->animes[index], sListPaletteAnimes[index].mode, sListPaletteAnimes[index].color);
}

static void ResearchList_StopPaletteAnime(ResearchList *wk, u32 index) {
    PaletteAnime_Stop(wk->animes[index]);
}

static BOOL ResearchList_IsPaletteAnimeEnd(ResearchList *wk, u32 index) {
    if (!PaletteAnime_IsActive(wk->animes[index])) {
        return TRUE;
    }
    return FALSE;
}

static void ResearchList_UpdatePaletteAnimes(ResearchList *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Update(wk->animes[i]);
    }
}

static void ResearchList_UpdateCommonPaletteAnime(ResearchList *wk) {
    ResearchCommon_UpdatePaletteAnime(wk->common);
}

static u32 ResearchList_GetKeyRepeatFrames(ResearchList *wk) {
    return wk->keyRepeatFrames;
}

static void ResearchList_CountKeyRepeat(ResearchList *wk) {
    wk->keyRepeatFrames++;
}

static void ResearchList_ResetKeyRepeat(ResearchList *wk) {
    wk->keyRepeatFrames = 0;
}

static void ResearchList_VBlank(TCB *tcb, void *data) {
    ResearchList *wk = data;

    GFL_BGSysUpdate();
    ClActSys_VBlank();
    PaletteFade_Transfer(wk->paletteFade);
}

static GameData *ResearchList_GetGameData(ResearchList *wk) {
    return ResearchCommon_GetGameData(wk->common);
}

static void ResearchList_SetHeapID(ResearchList *wk, HeapID heapId) {
    wk->heapId = heapId;
}

static void ResearchList_SetCommon(ResearchList *wk, ResearchCommon *common) {
    wk->common = common;
}

static ResearchCommon *ResearchList_GetCommon(ResearchList *wk) {
    return wk->common;
}

static void ResearchList_SetRecovery(ResearchList *wk, ResearchListRecovery *recovery) {
    wk->recovery = recovery;
}

// Puts the list back where it was when another screen was shown
static void ResearchList_LoadRecovery(ResearchList *wk) {
    int bgScroll;
    int scrollY;

    if (wk->recovery != NULL) {
        bgScroll = ResearchListRecovery_GetBGScroll(wk->recovery);
        if (bgScroll < -VIEW_TOP) {
            bgScroll = -VIEW_TOP;
        }
        if (bgScroll > MAX_BG_SCROLL) {
            bgScroll = MAX_BG_SCROLL;
        }
        ResearchList_SetBGScroll(bgScroll);

        scrollY = ResearchListRecovery_GetScrollY(wk->recovery);
        if (scrollY < 0) {
            scrollY = 0;
        }
        if (scrollY > SURVEY_COUNT * ITEM_HEIGHT) {
            scrollY = SURVEY_COUNT * ITEM_HEIGHT;
        }
        wk->scrollY = scrollY;
        wk->cursor = ResearchListRecovery_GetCursor(wk->recovery);
    }
}

static void ResearchList_SaveRecovery(ResearchList *wk) {
    int bgScroll;
    int scrollY;

    if (wk->recovery != NULL) {
        bgScroll = ResearchList_GetBGScroll();
        if (bgScroll < -VIEW_TOP) {
            bgScroll = -VIEW_TOP;
        }
        if (bgScroll > MAX_BG_SCROLL) {
            bgScroll = MAX_BG_SCROLL;
        }
        ResearchListRecovery_SetBGScroll(wk->recovery, bgScroll);

        scrollY = wk->scrollY;
        if (scrollY < 0) {
            scrollY = 0;
        }
        if (scrollY > SURVEY_COUNT * ITEM_HEIGHT) {
            scrollY = SURVEY_COUNT * ITEM_HEIGHT;
        }
        ResearchListRecovery_SetScrollY(wk->recovery, scrollY);
        ResearchListRecovery_SetCursor(wk->recovery, wk->cursor);
    }
}

static BOOL ResearchList_IsForceExit(ResearchList *wk) {
    return ResearchCommon_IsForceExit(ResearchList_GetCommon(wk));
}

static u8 ResearchList_GetSurveyCount(ResearchList *wk) {
    return wk->surveyCount;
}

static u8 ResearchList_GetSelected(ResearchList *wk) {
    return wk->selected;
}

static void ResearchList_SetSelected(ResearchList *wk, u8 index) {
    wk->selected = index;
}

static BOOL ResearchList_IsSurvey(ResearchList *wk, u8 index) {
    if (index < ResearchList_GetSurveyCount(wk)) {
        return TRUE;
    }
    return FALSE;
}

static u32 ResearchList_GetObjRes(ResearchList *wk, u32 index) {
    return wk->objRes[index];
}

static ClActUnit *ResearchList_GetUnit(ResearchList *wk, u32 index) {
    return wk->units[index];
}

static ClActor *ResearchList_GetActor(ResearchList *wk, u32 index) {
    return wk->actors[index];
}

static void ResearchList_SetBmpOamVisible(ResearchList *wk, u32 index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(wk->bmpOamActors[index], visible);
}

static BmpOamActor *ResearchList_GetButtonBmpOam(ResearchList *wk, u32 button) {
    return wk->bmpOamActors[sListButtonBmpOams[button]];
}

static ResearchList *ResearchList_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ResearchList), FALSE, "research_list.c", 3780);
}

static void ResearchList_InitWork(ResearchList *wk) {
    wk->recovery = NULL;
    wk->seq = SEQ_SETUP;
    wk->seqFrames = 0;
    wk->seqDone = FALSE;
    wk->waitFrames = 15;
    wk->cursor = 0;
    wk->cursorTarget = 0;
    wk->surveyCount = 0;
    wk->buttonCursor = BUTTON_YES;
    wk->selected = SURVEY_NONE;
    wk->tcbMgr = GFL_VBlankGetTCBMgr();
    wk->scrollY = 0;
    wk->prevScrollY = 0;
    wk->scrollFrom = 0;
    wk->scrollTo = 0;
    wk->scrollFrames = 0;
    wk->scrollFrame = 0;
    wk->darkened = FALSE;
    wk->cursorShown = FALSE;
    wk->next = RESEARCH_LIST_NEXT_TOP;
    wk->isEnd = FALSE;
    wk->vblankTask = NULL;
    ResearchList_ClearQueue(wk);
    ResearchList_ClearMsgData(wk);
    ResearchList_ClearFont(wk);
    ResearchList_ClearBGFonts(wk);
    ResearchList_ClearUnits(wk);
    ResearchList_ClearActors(wk);
    ResearchList_ClearObjRes(wk);
    ResearchList_ClearPaletteFade(wk);
    ResearchList_ClearBitmaps(wk);
    ResearchList_ClearPaletteAnimes(wk);
}

static void ResearchList_Free(ResearchList *wk) {
    GFL_HeapFree(wk);
}

static void ResearchList_CountSurveys(ResearchList *wk) {
    EventWork *eventWork = GameData_GetEventWork(ResearchList_GetGameData(wk));
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
    wk->surveyCount = count;
}

static void ResearchList_CreateFont(ResearchList *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
}

static void ResearchList_ClearFont(ResearchList *wk) {
    wk->font = NULL;
}

static void ResearchList_DeleteFont(ResearchList *wk) {
    GFL_FontFree(wk->font);
}

static void ResearchList_LoadMsgData(ResearchList *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        wk->msgData[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sListMsgFiles[i], wk->heapId);
    }
}

static void ResearchList_ClearMsgData(ResearchList *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        wk->msgData[i] = NULL;
    }
}

static void ResearchList_FreeMsgData(ResearchList *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        GFL_MsgDataFree(wk->msgData[i]);
    }
}

static void ResearchList_CreateQueue(ResearchList *wk) {
    wk->queue = Queue_Create(10, wk->heapId);
}

static void ResearchList_ClearQueue(ResearchList *wk) {
    wk->queue = NULL;
}

static void ResearchList_DeleteQueue(ResearchList *wk) {
    Queue_Delete(wk->queue);
}

static void ResearchList_InitTouchRects(ResearchList *wk) {
    int i;

    for (i = 0; i < BUTTON_RECT_COUNT; i++) {
        wk->buttonRects[i].left = sListButtonRects[i].left;
        wk->buttonRects[i].right = sListButtonRects[i].right;
        wk->buttonRects[i].top = sListButtonRects[i].top;
        wk->buttonRects[i].bottom = sListButtonRects[i].bottom;
    }
    for (i = 0; i < LIST_RECT_COUNT; i++) {
        wk->listRects[i].left = sListItemRects[i].left;
        wk->listRects[i].right = sListItemRects[i].right;
        wk->listRects[i].top = sListItemRects[i].top;
        wk->listRects[i].bottom = sListItemRects[i].bottom;
    }
    for (i = 0; i < SCROLL_RECT_COUNT; i++) {
        wk->scrollRects[i].left = sListScrollRects[i].left;
        wk->scrollRects[i].right = sListScrollRects[i].right;
        wk->scrollRects[i].top = sListScrollRects[i].top;
        wk->scrollRects[i].bottom = sListScrollRects[i].bottom;
    }
}

static void ResearchList_InitBG(ResearchList *wk) {
    GFL_BGSysSetLCDConfig(&sListLCDConfig);
    GFL_BGSysCreateBG(BG_SUB_TITLE, &sListBGSubTitle, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_SUB_TEXT, &sListBGSubText, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_FRAME, &sListBGMainFrame, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_LIST, &sListBGMainList, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_TEXT, &sListBGMainText, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_BACK, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_PATTERN, TRUE);
    GFL_BGSysSetBGEnabled(BG_SUB_TITLE, TRUE);
    GFL_BGSysSetBGEnabled(BG_SUB_TEXT, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_MAIN_FRAME, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_FRAME, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_LIST, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_TEXT, TRUE);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 16, 5);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG1, 16, 5);
    BmpWin_InitAllocator(wk->heapId);
}

static void ResearchList_ExitBG(ResearchList *wk) {
    BmpWin_FreeAllocator();
    GFL_BGSysReleaseBG(BG_MAIN_TEXT);
    GFL_BGSysReleaseBG(BG_MAIN_LIST);
    GFL_BGSysReleaseBG(BG_MAIN_FRAME);
    GFL_BGSysReleaseBG(BG_SUB_TEXT);
    GFL_BGSysReleaseBG(BG_SUB_TITLE);
}

static void ResearchList_LoadSubBG(ResearchList *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, HEAPID_TAIL(wk->heapId));
    void *file = GFL_ArcToolReadHeapNew(handle, 13, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_SUB_TITLE, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysQueueScrLoad(BG_SUB_TITLE);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchList_UnloadSubBG(ResearchList *wk) {
}

static void ResearchList_InitSubText(ResearchList *wk) {
    GFL_BGSysFillChar(BG_SUB_TEXT, 0, 1, 0);
    GFL_BGSysClearScr(BG_SUB_TEXT);
}

static void ResearchList_ExitSubText(ResearchList *wk) {
    GFL_BGSysFreeFilledChar(BG_SUB_TEXT, 1, 0);
}

static void ResearchList_LoadFrameBG(ResearchList *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, HEAPID_TAIL(wk->heapId));
    void *file = GFL_ArcToolReadHeapNew(handle, 3, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_MAIN_FRAME, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysQueueScrLoad(BG_MAIN_FRAME);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchList_UnloadFrameBG(ResearchList *wk) {
}

// Loads the rows of the list's screen for the surveys that are unlocked
static void ResearchList_LoadListBG(ResearchList *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, HEAPID_TAIL(wk->heapId));
    int rows = ResearchList_GetSurveyCount(wk) * ITEM_ROWS;
    void *file = GFL_ArcToolReadHeapNew(handle, 5, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_MAIN_LIST, screen->rawData, 0, 0, 32, rows);
    GFL_BGSysQueueScrLoad(BG_MAIN_LIST);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchList_UnloadListBG(ResearchList *wk) {
}

static void ResearchList_InitMainText(ResearchList *wk) {
    GFL_BGSysFillChar(BG_MAIN_TEXT, 0, 1, 0);
    GFL_BGSysClearScr(BG_MAIN_TEXT);
}

static void ResearchList_ExitMainText(ResearchList *wk) {
    GFL_BGSysFreeFilledChar(BG_MAIN_TEXT, 1, 0);
}

static void ResearchList_CreateBGFonts(ResearchList *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        BGFontSetup setup;

        setup.window.bg = sListBGFonts[i].window.bg;
        setup.window.x = sListBGFonts[i].window.x;
        setup.window.y = sListBGFonts[i].window.y;
        setup.window.width = sListBGFonts[i].window.width;
        setup.window.height = sListBGFonts[i].window.height;
        setup.window.textX = sListBGFonts[i].window.textX;
        setup.window.textY = sListBGFonts[i].window.textY;
        setup.window.palette = sListBGFonts[i].window.palette;
        setup.window.letterColor = sListBGFonts[i].window.letterColor;
        setup.window.shadowColor = sListBGFonts[i].window.shadowColor;
        setup.window.backColor = sListBGFonts[i].window.backColor;
        setup.centered = sListBGFonts[i].centered;
        wk->bgFonts[i] = BGFont_Create(&setup, wk->font, wk->msgData[sListBGFonts[i].msgDataIndex], wk->heapId);
        BGFont_PrintMsg(wk->bgFonts[i], sListBGFonts[i].strId);
    }

    for (i = 0; i < ResearchList_GetSurveyCount(wk); i++) {
        BGFontSetup setup;

        setup.window.bg = sListItemFonts[i].window.bg;
        setup.window.x = sListItemFonts[i].window.x;
        setup.window.y = sListItemFonts[i].window.y;
        setup.window.width = sListItemFonts[i].window.width;
        setup.window.height = sListItemFonts[i].window.height;
        setup.window.textX = sListItemFonts[i].window.textX;
        setup.window.textY = sListItemFonts[i].window.textY;
        setup.window.palette = sListItemFonts[i].window.palette;
        setup.window.letterColor = sListItemFonts[i].window.letterColor;
        setup.window.shadowColor = sListItemFonts[i].window.shadowColor;
        setup.window.backColor = sListItemFonts[i].window.backColor;
        setup.centered = sListItemFonts[i].centered;
        wk->itemFonts[i] = BGFont_Create(&setup, wk->font, wk->msgData[sListItemFonts[i].msgDataIndex], wk->heapId);
        BGFont_PrintMsg(wk->itemFonts[i], sListItemFonts[i].strId);
    }
}

static void ResearchList_ClearBGFonts(ResearchList *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        wk->bgFonts[i] = NULL;
    }
    for (i = 0; i < SURVEY_COUNT; i++) {
        wk->itemFonts[i] = NULL;
    }
}

static void ResearchList_DeleteBGFonts(ResearchList *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        BGFont_Delete(wk->bgFonts[i]);
        wk->bgFonts[i] = NULL;
    }
    for (i = 0; i < ResearchList_GetSurveyCount(wk); i++) {
        BGFont_Delete(wk->itemFonts[i]);
        wk->itemFonts[i] = NULL;
    }
}

static void ResearchList_ClearObjRes(ResearchList *wk) {
    int i;

    for (i = 0; i < OBJ_RES_COUNT; i++) {
        wk->objRes[i] = 0;
    }
}

static void ResearchList_LoadSubObjRes(ResearchList *wk) {
    HeapID heapId = wk->heapId;
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, HEAPID_TAIL(heapId));
    u32 chars = ClActSys_LoadChars(handle, 15, FALSE, CLACT_VRAM_SUB, heapId);
    u32 palette = ClActSys_LoadPltt(handle, 16, CLACT_VRAM_SUB, 0, heapId);
    u32 cellAnims = ClActSys_LoadCellAnims(handle, 14, 17, heapId);

    wk->objRes[OBJ_RES_SUB_CHARS] = chars;
    wk->objRes[OBJ_RES_SUB_PALETTE] = palette;
    wk->objRes[OBJ_RES_SUB_CELL_ANIMS] = cellAnims;
    GFL_ArcToolFree(handle);
}

static void ResearchList_FreeSubObjRes(ResearchList *wk) {
    ClActSys_FreeChars(wk->objRes[OBJ_RES_SUB_CHARS]);
    ClActSys_FreePltt(wk->objRes[OBJ_RES_SUB_PALETTE]);
    ClActSys_FreeCellAnims(wk->objRes[OBJ_RES_SUB_CELL_ANIMS]);
}

static void ResearchList_LoadMainObjRes(ResearchList *wk) {
    HeapID heapId = wk->heapId;
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, HEAPID_TAIL(heapId));
    u32 chars = ClActSys_LoadChars(handle, 15, FALSE, CLACT_VRAM_MAIN, heapId);
    u32 palette = ClActSys_LoadPlttRange(handle, 16, CLACT_VRAM_MAIN, 0xc0, 0, 5, heapId);
    u32 cellAnims = ClActSys_LoadCellAnims(handle, 14, 17, heapId);
    ArcTool *commonHandle;
    u32 buttonPalette;

    GFL_ArcToolFree(handle);

    commonHandle = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, heapId);
    buttonPalette = ClActSys_LoadPlttRange(commonHandle, 31, CLACT_VRAM_MAIN, 0x80, 0, 2, heapId);
    GFL_ArcToolFree(commonHandle);

    wk->objRes[OBJ_RES_MAIN_CHARS] = chars;
    wk->objRes[OBJ_RES_MAIN_PALETTE] = palette;
    wk->objRes[OBJ_RES_MAIN_CELL_ANIMS] = cellAnims;
    wk->objRes[OBJ_RES_BUTTON_PALETTE] = buttonPalette;
}

static void ResearchList_FreeMainObjRes(ResearchList *wk) {
    ClActSys_FreeChars(wk->objRes[OBJ_RES_MAIN_CHARS]);
    ClActSys_FreePltt(wk->objRes[OBJ_RES_MAIN_PALETTE]);
    ClActSys_FreeCellAnims(wk->objRes[OBJ_RES_MAIN_CELL_ANIMS]);
    ClActSys_FreePltt(wk->objRes[OBJ_RES_BUTTON_PALETTE]);
}

static void ResearchList_ClearUnits(ResearchList *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        wk->units[i] = NULL;
    }
}

static void ResearchList_CreateUnits(ResearchList *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        wk->units[i] = ClActUnit_Create(sListUnitCounts[i], sListUnitPriorities[i], wk->heapId);
    }
}

static void ResearchList_DeleteUnits(ResearchList *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        ClActUnit_Delete(wk->units[i]);
    }
}

static void ResearchList_ClearActors(ResearchList *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        wk->actors[i] = NULL;
    }
}

static void ResearchList_CreateActors(ResearchList *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        const ResearchActorSetup *entry = &sListActors[i];
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
        unit = ResearchList_GetUnit(wk, entry->unit);
        chars = ResearchList_GetObjRes(wk, entry->chars);
        palette = ResearchList_GetObjRes(wk, entry->palette);
        cellAnims = ResearchList_GetObjRes(wk, entry->cellAnims);
        wk->actors[i] = ClActor_Create(unit, chars, palette, cellAnims, &setup, entry->surface, wk->heapId);
        ClActor_SetVisible(wk->actors[i], FALSE);
    }
}

static void ResearchList_DeleteActors(ResearchList *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(wk->actors[i]);
    }
}

static void ResearchList_ClearBitmaps(ResearchList *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        wk->bitmaps[i] = NULL;
    }
}

static void ResearchList_CreateBitmaps(ResearchList *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        wk->bitmaps[i] =
            GFL_BitmapCreate(sListBitmaps[i].width, sListBitmaps[i].height, sListBitmaps[i].tileSize, wk->heapId);
    }
}

static void ResearchList_DrawBitmaps(ResearchList *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        const BitmapEntry *entry = &sListBitmaps[i];

        if (entry->hasBase) {
            GFLBitmap *base = GFL_G2DIOLoadBitmap(entry->arcId, entry->fileId, FALSE, wk->heapId);
            u16 color;
            StrBuf *str;

            GFL_BitmapCopyArea(base, wk->bitmaps[i], 0, 0, 0, 0, entry->width * 8, entry->height * 8, entry->backColor);
            color = PRINT_COLOR(entry->letterColor, entry->shadowColor, entry->backColor);
            str = GFL_MsgDataLoadStrbufNew(wk->msgData[entry->msgDataIndex], entry->strId);
            GFL_TextRendererDrawToBitmapEx(wk->bitmaps[i], entry->textX, entry->textY, str, wk->font, color);
            GFL_HeapFree(str);
            GFL_BitmapFree(base);
        }
    }
}

// Draws the Yes button from the tiles of the apps' button, 3 by 3 of them for its corners, edges and middle
static void ResearchList_DrawYesButton(ResearchList *wk) {
    int tiles[3][10] = {
        { 0, 1, 1, 1, 1, 1, 1, 1, 1, 2 },
        { 3, 4, 4, 4, 4, 4, 4, 4, 4, 5 },
        { 6, 7, 7, 7, 7, 7, 7, 7, 7, 8 },
    };
    const BitmapEntry *entry = &sListBitmaps[BMP_YES];
    int x, y;
    GFLBitmap *base;
    u16 color;
    StrBuf *str;

    base = GFL_G2DIOLoadBitmap(entry->arcId, entry->fileId, FALSE, wk->heapId);
    for (y = 0; y < 3; y++) {
        for (x = 0; x < 10; x++) {
            GFL_BitmapCopyArea(base, wk->bitmaps[BMP_YES], tiles[y][x] % 3 * 8, tiles[y][x] / 3 * 8, x * 8, y * 8, 8, 8,
                               0);
        }
    }
    GFL_BitmapFree(base);

    color = PRINT_COLOR(entry->letterColor, entry->shadowColor, entry->backColor);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData[entry->msgDataIndex], entry->strId);
    GFL_TextRendererDrawToBitmapEx(wk->bitmaps[BMP_YES], entry->textX, entry->textY, str, wk->font, color);
    GFL_HeapFree(str);
}

static void ResearchList_DrawNoButton(ResearchList *wk) {
    int tiles[3][10] = {
        { 0, 1, 1, 1, 1, 1, 1, 1, 1, 2 },
        { 3, 4, 4, 4, 4, 4, 4, 4, 4, 5 },
        { 6, 7, 7, 7, 7, 7, 7, 7, 7, 8 },
    };
    const BitmapEntry *entry = &sListBitmaps[BMP_NO];
    int x, y;
    GFLBitmap *base;
    u16 color;
    StrBuf *str;

    base = GFL_G2DIOLoadBitmap(entry->arcId, entry->fileId, FALSE, wk->heapId);
    for (y = 0; y < 3; y++) {
        for (x = 0; x < 10; x++) {
            GFL_BitmapCopyArea(base, wk->bitmaps[BMP_NO], tiles[y][x] % 3 * 8, tiles[y][x] / 3 * 8, x * 8, y * 8, 8, 8,
                               0);
        }
    }
    GFL_BitmapFree(base);

    color = PRINT_COLOR(entry->letterColor, entry->shadowColor, entry->backColor);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData[entry->msgDataIndex], entry->strId);
    GFL_TextRendererDrawToBitmapEx(wk->bitmaps[BMP_NO], entry->textX, entry->textY, str, wk->font, color);
    GFL_HeapFree(str);
}

static void ResearchList_FreeBitmaps(ResearchList *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        GFL_BitmapFree(wk->bitmaps[i]);
    }
}

static void ResearchList_CreateBmpOam(ResearchList *wk) {
    wk->bmpOamSys = BmpOam_Init(wk->heapId, wk->units[UNIT_BMP_OAM]);
}

static void ResearchList_DeleteBmpOam(ResearchList *wk) {
    BmpOam_Exit(wk->bmpOamSys);
}

static void ResearchList_CreateBmpOamActors(ResearchList *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        BmpOamActorSetup setup;

        setup.bitmap = wk->bitmaps[i];
        setup.x = sListBmpOams[i].x;
        setup.y = sListBmpOams[i].y;
        setup.palette = ResearchList_GetObjRes(wk, sListBmpOams[i].palette);
        setup.paletteOffset = sListBmpOams[i].palOffset;
        setup.priority = sListBmpOams[i].priority;
        setup.bgPriority = sListBmpOams[i].bgPriority;
        setup.surface = sListBmpOams[i].surface;
        setup.vramType = sListBmpOams[i].vramType;
        wk->bmpOamActors[i] = BmpOam_ActorAdd(wk->bmpOamSys, &setup);
        BmpOam_ActorSetDrawEnable(wk->bmpOamActors[i], FALSE);
        BmpOam_ActorBmpTrans(wk->bmpOamActors[i]);
    }
}

static void ResearchList_DeleteBmpOamActors(ResearchList *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        BmpOam_ActorDel(wk->bmpOamActors[i]);
    }
}

static void ResearchList_ClearPaletteFade(ResearchList *wk) {
    wk->paletteFade = NULL;
}

static void ResearchList_CreatePaletteFade(ResearchList *wk) {
    wk->paletteFade = PaletteFade_Create(wk->heapId);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, wk->heapId);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, wk->heapId);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_VRAM_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_VRAM_MAIN_OBJ, 0, 0x200);
}

static void ResearchList_DeletePaletteFade(ResearchList *wk) {
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_Free(wk->paletteFade);
}

static void ResearchList_CreatePaletteAnimes(ResearchList *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        wk->animes[i] = PaletteAnime_Create(wk->heapId);
    }
}

static void ResearchList_ClearPaletteAnimes(ResearchList *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        wk->animes[i] = NULL;
    }
}

static void ResearchList_DeletePaletteAnimes(ResearchList *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Delete(wk->animes[i]);
    }
}

static void ResearchList_SetupPaletteAnimes(ResearchList *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Setup(wk->animes[i], sListPaletteAnimes[i].dst, sListPaletteAnimes[i].src,
                           sListPaletteAnimes[i].count);
    }
}

static void ResearchList_RestorePalettes(ResearchList *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Restore(wk->animes[i]);
    }
}

static void ResearchList_ShowCenterActor(ResearchList *wk) {
    ClActor *actor = ResearchList_GetActor(wk, ACTOR_CENTER);
    ClActorPos pos;

    pos.x = 128;
    pos.y = 96;
    ClActor_SetSurfacePos(actor, &pos, 0);
    ClActor_SetVisible(actor, TRUE);
}

static void ResearchList_ShowCommIcon(ResearchList *wk) {
    GFL_NetSetIconScreen(TRUE, wk->heapId);
}

static void ResearchList_SetVBlank(ResearchList *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(ResearchList_VBlank, wk, 0);
}

static void ResearchList_ResetVBlank(ResearchList *wk) {
    GFL_TCBRemove(wk->vblankTask);
    wk->vblankTask = NULL;
}
