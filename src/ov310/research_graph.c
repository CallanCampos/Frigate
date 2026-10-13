#include "types.h"
#include "app/research_radar/arrow.h"
#include "app/research_radar/bg_font.h"
#include "app/research_radar/circle_graph.h"
#include "app/research_radar/palette_anime.h"
#include "app/research_radar/percentage.h"
#include "app/research_radar/queue.h"
#include "app/research_radar/research_common.h"
#include "app/research_radar/research_data.h"
#include "app/research_radar/research_graph.h"
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
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "save/save_control.h"
#include "system/bmp_oam.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The Research Radar's graph of the survey being run: a circle graph of the answers to each of its three questions,
// today's or in all, with the answer chosen pointed at by an arrow, and the player's own answer marked. The results are
// hidden until the player opens them, and they are updated when new ones come in by beacon. The screen is used by touch
// or with a cursor, which moves between the question, the answer, the player's answer, the count and the button that
// opens the results

// The sequences of the screen, which run one after another from a queue
enum {
    SEQ_SETUP,
    // Restarts the beacon communication, which brings in new results
    SEQ_RESTART_COMM,
    SEQ_TOUCH,
    SEQ_CURSOR,
    // Shows the results for the first time, growing the graph
    SEQ_OPEN,
    // Shows the percentages of the answers one after another
    SEQ_SHOW_PERCENTAGES,
    // Flashes the screen white and back
    SEQ_FLASH_IN,
    SEQ_FLASH_OUT,
    // Shows the results updated by new ones, growing the new graph over the old one
    SEQ_UPDATE_TOUCH,
    SEQ_UPDATE_CURSOR,
    SEQ_FADE_OUT,
    SEQ_WAIT,
    SEQ_TEARDOWN,
    SEQ_END,
};

// The BGs: the graph on the main engine's BG 0, in 3D, its frame on BG 2 and its text on BG 3, and the sub engine's
// title on BG 6 and its text on BG 7, in front of the proc's BGs
#define BG_MAIN_3D 0
#define BG_MAIN_GRAPH 2
#define BG_MAIN_TEXT 3
#define BG_SUB_TITLE 6
#define BG_SUB_TEXT 7

// The palettes of the graph's BG for the areas of the cursor, and for the one the cursor is on
#define BG_PALETTE_NORMAL 6
#define BG_PALETTE_CURSOR 10

// The objects' resources: the main engine's, the palette of the button, which comes from the graphics the apps share,
// and the sub engine's
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

// The units: the actors', the bitmaps' and the percentages'
#define UNIT_COUNT 4
#define UNIT_ACTORS 1
#define UNIT_BMP_OAM 2
#define UNIT_PERCENTAGES 3

// The actors: the arrows on either side of the cursor, and the mark of the player's answer in the graph, with the one
// that explains it
enum {
    ACTOR_LEFT,
    ACTOR_RIGHT,
    ACTOR_LEGEND,
    ACTOR_MARKER,
    ACTOR_COUNT,
};

// The bitmaps shown as actors: the message shown while the graph grows, and the button that opens the results
enum {
    BMP_MESSAGE,
    BMP_BUTTON,
    BMP_COUNT,
};

// The texts: the sub screen's title, survey and question, and the main screen's question, answer, player's answer,
// count of people, the message that nobody has answered, and the one that the results are being updated
enum {
    BG_FONT_TITLE,
    BG_FONT_SURVEY,
    BG_FONT_QUESTION,
    BG_FONT_QUESTION_TITLE,
    BG_FONT_ANSWER,
    BG_FONT_PLAYER_ANSWER,
    BG_FONT_TOTAL,
    BG_FONT_NO_ANSWERS,
    BG_FONT_UPDATING,
    BG_FONT_COUNT,
};

// The palette animations: the cursor, a cursor move and the button pressed, the cursor on the button and moved to
// it, the button disabled and enabled, and the frame
enum {
    ANIME_CURSOR,
    ANIME_MOVE,
    ANIME_DECIDE,
    ANIME_BUTTON_CURSOR,
    ANIME_BUTTON_MOVE,
    ANIME_BUTTON_DISABLE,
    ANIME_BUTTON_ENABLE,
    ANIME_FRAME,
    ANIME_COUNT,
};

// The places of the cursor, in its order
enum {
    CURSOR_QUESTION,
    CURSOR_ANSWER,
    CURSOR_PLAYER_ANSWER,
    CURSOR_MODE,
    CURSOR_BUTTON,
    CURSOR_COUNT,
};

// The touch rectangles: the arrows on either side of the cursor, which move with it, the places of the cursor, the
// graph and the button
enum {
    TOUCH_LEFT,
    TOUCH_RIGHT,
    TOUCH_QUESTION,
    TOUCH_ANSWER,
    TOUCH_PLAYER_ANSWER,
    TOUCH_MODE,
    TOUCH_GRAPH,
    TOUCH_BUTTON,
    TOUCH_END,
    TOUCH_RECT_COUNT,
};

// The counts the graph shows: today's answers or all of them
enum {
    MODE_TODAY,
    MODE_TOTAL,
    MODE_COUNT,
};

#define MSG_DATA_COUNT 3
#define MSG_DATA_ANSWERS 2
#define PERCENTAGE_COUNT 10
#define SURVEY_COUNT 10

// The answers' IDs, which index the colors of their slices
#define ANSWER_ID_COUNT 148

// The counts are shown up to this
#define COUNT_MAX 999999

// A percentage under this isn't shown on the graph
#define PERCENTAGE_MIN 10

// The question of the time played, whose answer the player doesn't give: it is worked out from the hours played, one
// answer for each 10 hours up to 100
#define QUESTION_PLAY_TIME 29
#define ANSWER_PLAY_TIME_FIRST 0x87

// The frames the graph grows for when it is opened and updated, playing a drum roll
#define OPEN_FRAMES 120
#define UPDATE_FRAMES 60

// The frames after the results are updated before new results are shown
#define UPDATE_WAIT_FRAMES 240

// The frames between two percentages being shown
#define PERCENTAGE_INTERVAL 10

struct ResearchGraph {
    ResearchCommon *common;
    HeapID heapId;
    Font *font;
    MsgData *msgData[MSG_DATA_COUNT];
    WordSet *wordSet;
    Queue *queue;
    u32 seq;
    u32 seqState;
    u32 seqFrames;
    u32 waitFrames;
    // The frames since the results were last updated, up to UPDATE_WAIT_FRAMES
    u32 updateFrames;
    int cursor;
    ResearchData data;
    // The question shown and its answer chosen, by their indices in the data
    u8 question;
    u8 answer;
    // The graph of each mode, and the graphs that grow over them when the results are updated
    CircleGraph *graphs[MODE_COUNT];
    CircleGraph *nextGraphs[MODE_COUNT];
    u32 mode;
    Arrow *arrow;
    Percentage *percentages[PERCENTAGE_COUNT];
    // How many percentages are set up, and how many of them are shown
    u8 percentageCount;
    u8 percentagesShown;
    TouchRect touchRects[TOUCH_RECT_COUNT];
    PaletteAnime *animes[ANIME_COUNT];
    BGFont *bgFonts[BG_FONT_COUNT];
    u32 objRes[OBJ_RES_COUNT];
    ClActUnit *units[UNIT_COUNT];
    ClActor *actors[ACTOR_COUNT];
    BmpOamSys *bmpOamSys;
    BmpOamActor *bmpOamActors[BMP_COUNT];
    GFLBitmap *bitmaps[BMP_COUNT];
    PaletteFade *paletteFade;
    TCBManager *tcbMgr;
    TCB *vblankTask;
    // Whether the graphs are drawn and animated, while the screen is set up
    BOOL active;
    // Whether the results have been opened
    BOOL resultsShown;
    // Whether they were opened by touch
    BOOL touchOpened;
    // Whether the results are being updated
    BOOL updating;
    // Whether UPDATE_WAIT_FRAMES have passed since the last update, and whether new results came in
    BOOL updateReady;
    BOOL newResults;
    BOOL seqDone;
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

// A place of the cursor: the area of the graph's BG it highlights, in tiles, its rectangle in pixels, and where the
// arrows on either side of it go, from the rectangle's corner
typedef struct {
    u8 tileX;
    u8 tileY;
    u8 tileWidth;
    u8 tileHeight;
    int x;
    int y;
    int width;
    int height;
    int leftX;
    int leftY;
    int rightX;
    int rightY;
} CursorEntry;

static void ResearchGraph_SeqSetup(ResearchGraph *wk);
static void ResearchGraph_SeqRestartComm(ResearchGraph *wk);
static void ResearchGraph_SeqTouch(ResearchGraph *wk);
static void ResearchGraph_SeqCursor(ResearchGraph *wk);
static void ResearchGraph_SeqOpen(ResearchGraph *wk);
static void ResearchGraph_SeqShowPercentages(ResearchGraph *wk);
static void ResearchGraph_SeqFlashIn(ResearchGraph *wk);
static void ResearchGraph_SeqFlashOut(ResearchGraph *wk);
static void ResearchGraph_SeqUpdateTouch(ResearchGraph *wk);
static void ResearchGraph_SeqUpdateCursor(ResearchGraph *wk);
static void ResearchGraph_SeqFadeOut(ResearchGraph *wk);
static void ResearchGraph_SeqWait(ResearchGraph *wk);
static void ResearchGraph_SeqTeardown(ResearchGraph *wk);
static void ResearchGraph_EndSeq(ResearchGraph *wk);
static void ResearchGraph_SetTouchMode(ResearchGraph *wk, BOOL touch);
static void ResearchGraph_SetNext(ResearchGraph *wk, u32 next);
static u32 ResearchGraph_GetWait(ResearchGraph *wk);
static u32 ResearchGraph_GetStartSeq(ResearchGraph *wk);
static void ResearchGraph_CountFrame(ResearchGraph *wk);
static void ResearchGraph_PushSeq(ResearchGraph *wk, u32 seq);
static void ResearchGraph_UpdateSeq(ResearchGraph *wk);
static void ResearchGraph_SetSeq(ResearchGraph *wk, u32 seq);
static u32 ResearchGraph_GetSeqState(ResearchGraph *wk);
static void ResearchGraph_NextSeqState(ResearchGraph *wk);
static void ResearchGraph_CursorUp(ResearchGraph *wk);
static void ResearchGraph_CursorDown(ResearchGraph *wk);
static void ResearchGraph_SelectCursor(ResearchGraph *wk, int pos);
static void ResearchGraph_MoveCursorTo(ResearchGraph *wk, int pos);
static void ResearchGraph_NextQuestion(ResearchGraph *wk);
static void ResearchGraph_PrevQuestion(ResearchGraph *wk);
static void ResearchGraph_NextAnswer(ResearchGraph *wk);
static void ResearchGraph_PrevAnswer(ResearchGraph *wk);
static void ResearchGraph_ResetAnswer(ResearchGraph *wk);
static void ResearchGraph_SelectFirstAnswer(ResearchGraph *wk);
static void ResearchGraph_ToggleMode(ResearchGraph *wk);
static void ResearchGraph_FadeIn(void);
static void ResearchGraph_FadeOut(void);
static void ResearchGraph_MoveCursor(ResearchGraph *wk, int dir);
static void ResearchGraph_SetCursor(ResearchGraph *wk, int pos);
static void ResearchGraph_HighlightCursor(ResearchGraph *wk);
static void ResearchGraph_UnhighlightCursor(ResearchGraph *wk);
static void ResearchGraph_ChangeQuestion(ResearchGraph *wk, int dir);
static void ResearchGraph_SetFirstAnswer(ResearchGraph *wk);
static void ResearchGraph_ChangeAnswer(ResearchGraph *wk, int dir);
static void ResearchGraph_SetFirstAnswerIndex(ResearchGraph *wk);
static void ResearchGraph_UpdateButton(ResearchGraph *wk);
static void ResearchGraph_PressButton(ResearchGraph *wk);
static void ResearchGraph_FadeButtons(ResearchGraph *wk, BOOL restore);
static void ResearchGraph_HighlightButton(ResearchGraph *wk);
static void ResearchGraph_UnhighlightButton(ResearchGraph *wk);
static void ResearchGraph_MoveToButton(ResearchGraph *wk);
static void ResearchGraph_EnableButton(ResearchGraph *wk);
static void ResearchGraph_DisableButton(ResearchGraph *wk);
static void ResearchGraph_StartReturnAnime(ResearchGraph *wk);
static void ResearchGraph_SwapGraphs(ResearchGraph *wk);
static void ResearchGraph_SetCenterColor(CircleGraphData *item);
static void ResearchGraph_SetGraphDepths(ResearchGraph *wk);
static void ResearchGraph_SetGraphData(ResearchGraph *wk, u32 mode);
static void ResearchGraph_SetNextGraphData(ResearchGraph *wk, u32 mode);
static void ResearchGraph_UpdateGraphs(ResearchGraph *wk);
static void ResearchGraph_DrawGraphs(ResearchGraph *wk);
static void ResearchGraph_DrawAnswerColor(ResearchGraph *wk);
static void ResearchGraph_ShowArrow(ResearchGraph *wk);
static void ResearchGraph_SetupPercentages(ResearchGraph *wk);
static void ResearchGraph_HidePercentages(ResearchGraph *wk);
static void ResearchGraph_ShowPercentage(ResearchGraph *wk, u8 index);
static void ResearchGraph_ShowPercentages(ResearchGraph *wk);
static void ResearchGraph_UpdateArrowRects(ResearchGraph *wk);
static void ResearchGraph_LoadGraphBG(ResearchGraph *wk);
static void ResearchGraph_PrintSurvey(ResearchGraph *wk);
static void ResearchGraph_PrintQuestion(ResearchGraph *wk);
static void ResearchGraph_PrintQuestionTitle(ResearchGraph *wk);
static void ResearchGraph_PrintAnswer(ResearchGraph *wk);
static void ResearchGraph_PrintPlayerAnswer(ResearchGraph *wk);
static void ResearchGraph_PrintTotal(ResearchGraph *wk);
static void ResearchGraph_ShowNoAnswers(ResearchGraph *wk);
static void ResearchGraph_ShowUpdating(ResearchGraph *wk);
static void ResearchGraph_UpdateArrows(ResearchGraph *wk);
static void ResearchGraph_SetArrowsVisible(ResearchGraph *wk, BOOL visible);
static void ResearchGraph_UpdateLegend(ResearchGraph *wk);
static void ResearchGraph_UpdateMarker(ResearchGraph *wk);
static void ResearchGraph_ShowButton(ResearchGraph *wk);
static void ResearchGraph_ShowMessage(ResearchGraph *wk);
static void ResearchGraph_HideMessage(ResearchGraph *wk);
static void ResearchGraph_SetBmpOamVisible(ResearchGraph *wk, u32 index, BOOL visible);
static void ResearchGraph_StartPaletteAnime(ResearchGraph *wk, u32 index);
static void ResearchGraph_StopPaletteAnime(ResearchGraph *wk, u32 index);
static BOOL ResearchGraph_IsPaletteAnimeActive(ResearchGraph *wk, u32 index);
static void ResearchGraph_UpdatePaletteAnimes(ResearchGraph *wk);
static void ResearchGraph_UpdateCommonPaletteAnime(ResearchGraph *wk);
static void ResearchGraph_FlashIn(ResearchGraph *wk);
static void ResearchGraph_FlashOut(ResearchGraph *wk);
static BOOL ResearchGraph_IsPaletteFadeDone(ResearchGraph *wk);
static void ResearchGraph_VBlank(TCB *tcb, void *data);
static void ResearchGraph_CheckNewResults(ResearchGraph *wk);
static void ResearchGraph_ClearNewResults(ResearchGraph *wk);
static void ResearchGraph_CountUpdateFrames(ResearchGraph *wk);
static void ResearchGraph_ResetUpdateFrames(ResearchGraph *wk);
static GameSystem *ResearchGraph_GetGameSystem(ResearchGraph *wk);
static GameData *ResearchGraph_GetGameData(ResearchGraph *wk);
static void ResearchGraph_SetHeapID(ResearchGraph *wk, HeapID heapId);
static ResearchCommon *ResearchGraph_GetCommon(ResearchGraph *wk);
static void ResearchGraph_SetCommon(ResearchGraph *wk, ResearchCommon *common);
static BOOL ResearchGraph_IsForceExit(ResearchGraph *wk);
static void ResearchGraph_SetMode(ResearchGraph *wk, u32 mode);
static CircleGraph *ResearchGraph_GetGraph(ResearchGraph *wk);
static CircleGraph *ResearchGraph_GetNextGraph(ResearchGraph *wk);
static BOOL ResearchGraph_AreGraphsStill(ResearchGraph *wk);
static u8 ResearchGraph_GetQuestionID(ResearchGraph *wk);
static u8 ResearchGraph_GetAnswerCount(ResearchGraph *wk);
static u16 ResearchGraph_GetAnswerID(ResearchGraph *wk);
static u32 ResearchGraph_GetQuestionTotal(ResearchGraph *wk);
static u32 ResearchGraph_GetQuestionTodayTotal(ResearchGraph *wk);
static u32 ResearchGraph_GetQuestionAllTotal(ResearchGraph *wk);
static u32 ResearchGraph_GetAnswerTotal(ResearchGraph *wk);
static u32 ResearchGraph_GetAnswerTodayTotal(ResearchGraph *wk);
static u32 ResearchGraph_GetAnswerAllTotal(ResearchGraph *wk);
static u8 ResearchGraph_GetSurveyID(ResearchGraph *wk);
static u8 ResearchGraph_GetPlayerAnswer(ResearchGraph *wk);
static u8 ResearchGraph_GetPlayTimeAnswer(ResearchGraph *wk);
static u32 ResearchGraph_GetSavedTodayTotal(ResearchGraph *wk);
static u32 ResearchGraph_GetObjRes(ResearchGraph *wk, u32 index);
static ClActUnit *ResearchGraph_GetUnit(ResearchGraph *wk, u32 index);
static ClActor *ResearchGraph_GetActor(ResearchGraph *wk, u32 index);
static ResearchGraph *ResearchGraph_Alloc(HeapID heapId);
static void ResearchGraph_InitWork(ResearchGraph *wk);
static void ResearchGraph_Free(ResearchGraph *wk);
static void ResearchGraph_ClearQueue(ResearchGraph *wk);
static void ResearchGraph_CreateQueue(ResearchGraph *wk);
static void ResearchGraph_DeleteQueue(ResearchGraph *wk);
static void ResearchGraph_ClearFont(ResearchGraph *wk);
static void ResearchGraph_CreateFont(ResearchGraph *wk);
static void ResearchGraph_DeleteFont(ResearchGraph *wk);
static void ResearchGraph_ClearMsgData(ResearchGraph *wk);
static void ResearchGraph_LoadMsgData(ResearchGraph *wk);
static void ResearchGraph_FreeMsgData(ResearchGraph *wk);
static void ResearchGraph_ClearWordSet(ResearchGraph *wk);
static void ResearchGraph_CreateWordSet(ResearchGraph *wk);
static void ResearchGraph_DeleteWordSet(ResearchGraph *wk);
static void ResearchGraph_ClearGraphs(ResearchGraph *wk);
static void ResearchGraph_CreateGraphs(ResearchGraph *wk);
static void ResearchGraph_DeleteGraphs(ResearchGraph *wk);
static void ResearchGraph_ClearData(ResearchGraph *wk);
static void ResearchGraph_LoadData(ResearchGraph *wk);
static void ResearchGraph_InitTouchRects(ResearchGraph *wk);
static void ResearchGraph_ClearArrow(ResearchGraph *wk);
static void ResearchGraph_CreateArrow(ResearchGraph *wk);
static void ResearchGraph_DeleteArrow(ResearchGraph *wk);
static void ResearchGraph_ClearPercentages(ResearchGraph *wk);
static void ResearchGraph_CreatePercentages(ResearchGraph *wk);
static void ResearchGraph_DeletePercentages(ResearchGraph *wk);
static void ResearchGraph_Init3D(void);
static void ResearchGraph_InitBG(ResearchGraph *wk);
static void ResearchGraph_ExitBG(ResearchGraph *wk);
static void ResearchGraph_LoadSubBG(ResearchGraph *wk);
static void ResearchGraph_UnloadSubBG(ResearchGraph *wk);
static void ResearchGraph_InitSubText(ResearchGraph *wk);
static void ResearchGraph_ExitSubText(ResearchGraph *wk);
static void ResearchGraph_LoadMainBG(ResearchGraph *wk);
static void ResearchGraph_UnloadMainBG(ResearchGraph *wk);
static void ResearchGraph_InitMainText(ResearchGraph *wk);
static void ResearchGraph_ExitMainText(ResearchGraph *wk);
static void ResearchGraph_ClearBGFonts(ResearchGraph *wk);
static void ResearchGraph_CreateBGFonts(ResearchGraph *wk);
static void ResearchGraph_DeleteBGFonts(ResearchGraph *wk);
static void ResearchGraph_LoadSubObjRes(ResearchGraph *wk);
static void ResearchGraph_FreeSubObjRes(ResearchGraph *wk);
static void ResearchGraph_LoadMainObjRes(ResearchGraph *wk);
static void ResearchGraph_FreeMainObjRes(ResearchGraph *wk);
static void ResearchGraph_ClearUnits(ResearchGraph *wk);
static void ResearchGraph_CreateUnits(ResearchGraph *wk);
static void ResearchGraph_DeleteUnits(ResearchGraph *wk);
static void ResearchGraph_ClearActors(ResearchGraph *wk);
static void ResearchGraph_CreateActors(ResearchGraph *wk);
static void ResearchGraph_DeleteActors(ResearchGraph *wk);
static void ResearchGraph_ClearBitmaps(ResearchGraph *wk);
static void ResearchGraph_CreateBitmaps(ResearchGraph *wk);
static void ResearchGraph_DrawBitmaps(ResearchGraph *wk);
static void ResearchGraph_DrawButton(ResearchGraph *wk);
static void ResearchGraph_FreeBitmaps(ResearchGraph *wk);
static void ResearchGraph_CreateBmpOam(ResearchGraph *wk);
static void ResearchGraph_DeleteBmpOam(ResearchGraph *wk);
static void ResearchGraph_CreateBmpOamActors(ResearchGraph *wk);
static void ResearchGraph_DeleteBmpOamActors(ResearchGraph *wk);
static void ResearchGraph_ClearPaletteFade(ResearchGraph *wk);
static void ResearchGraph_CreatePaletteFade(ResearchGraph *wk);
static void ResearchGraph_DeletePaletteFade(ResearchGraph *wk);
static void ResearchGraph_ClearPaletteAnimes(ResearchGraph *wk);
static void ResearchGraph_CreatePaletteAnimes(ResearchGraph *wk);
static void ResearchGraph_DeletePaletteAnimes(ResearchGraph *wk);
static void ResearchGraph_SetupPaletteAnimes(ResearchGraph *wk);
static void ResearchGraph_RestorePalettes(ResearchGraph *wk);
static void ResearchGraph_ShowCommIcon(ResearchGraph *wk);
static void ResearchGraph_SetVBlank(ResearchGraph *wk);
static void ResearchGraph_ResetVBlank(ResearchGraph *wk);
static u8 ResearchGraph_ClampU8(int value);
static void ResearchGraph_RequestCommRestart(ResearchGraph *wk);
static BOOL ResearchGraph_RestartComm(ResearchGraph *wk);
static void ResearchGraph_EndComm(ResearchGraph *wk);

// Not referenced
const u32 ResearchGraph_Unused1 = 31;

static const u8 sGraphUnitPriorities[UNIT_COUNT] = { 0, 1, 0, 0 };

static const u16 sGraphUnitCounts[UNIT_COUNT] = { 1, 4, 34, 68 };

// Not referenced; see research_graph.h
const u32 ResearchGraph_Unused2[2] = { 3, 0 };

// The names of the surveys
static const u8 sGraphSurveyNames[SURVEY_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

// The three questions of each survey
static const u8 sGraphSurveyQuestions1[SURVEY_COUNT] = { 0, 1, 2, 3, 4, 12, 20, 10, 9, 15 };

static const u8 sGraphSurveyQuestions2[SURVEY_COUNT] = { 8, 25, 7, 5, 6, 13, 21, 16, 19, 18 };

static const u8 sGraphSurveyQuestions3[SURVEY_COUNT] = { 29, 26, 28, 11, 14, 24, 22, 17, 23, 27 };

static const u32 sGraphMsgFiles[MSG_DATA_COUNT] = { 0x168, 0x163, 0 };

static const GXRgb sGraphEdgeColors[8] = { 0 };

static const BGSysLCDConfig sGraphLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

// The titles of the questions, and their texts, by their IDs
static const u8 sGraphQuestionTitles[30] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
                                             35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49 };

static const u8 sGraphQuestionTexts[30] = { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
                                            65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 };

static const BGSetup sGraphBGSubTitle = { 0,
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

static const BGSetup sGraphBGSubText = { 0,
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

static const BGSetup sGraphBGMainGraph = { 0,
                                           0,
                                           0x800,
                                           0,
                                           BGRES_256x256,
                                           GX_BG_COLORMODE_16,
                                           GX_BG_SCRBASE(0x0800),
                                           GX_BG_CHARBASE(0x04000),
                                           0x8000,
                                           GX_BG_EXTPLTT_01,
                                           1,
                                           GX_BG_AREAOVER_XLU,
                                           FALSE };

static const BGSetup sGraphBGMainText = { 0,
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

// The arrows' rectangles are set from the cursor
static const ResearchRect sGraphTouchRects[TOUCH_RECT_COUNT] = {
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 16, 240, 0, 32 },
    { 104, 255, 32, 88 },
    { 136, 255, 88, 128 },
    { 104, 255, 128, 168 },
    { 8, 112, 48, 152 },
    { 0, 160, 160, 192 },
#ifdef BUGFIX
    { 0, 0, TOUCH_RECT_END, 0 },
#else
    // BUG: The end of the table is written as a TouchRect, with TOUCH_RECT_END in the first field, which is left here,
    // so the copy in the work has no end and the touch panel goes on reading the work after it
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

static const BmpOamEntry sGraphBmpOams[BMP_COUNT] = {
    { 0, 76, OBJ_RES_MAIN_PALETTE, 2, 0, 0, 0, CLACT_VRAM_MAIN },
    { 0, 168, OBJ_RES_BUTTON_PALETTE, 1, 0, 0, 0, CLACT_VRAM_MAIN },
};

static const BitmapEntry sGraphBitmaps[BMP_COUNT] = {
    { 32, 5, 0x20, 0, 11, 0, 9, 15, 14, 0, ARCID_RESEARCH_RADAR, 21, TRUE },
    { 20, 3, 0x20, 0, 20, 0, 4, 14, 15, 0, ARCID_APP_MENU_COMMON, 32, FALSE },
};

static const ResearchActorSetup sGraphActors[ACTOR_COUNT] = {
    { 0, 0, 9, 0, 0, UNIT_ACTORS, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
    { 0, 0, 8, 0, 0, UNIT_ACTORS, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
    { 138, 108, 6, 0, 0, UNIT_ACTORS, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
    { 0, 0, 6, 0, 0, UNIT_ACTORS, OBJ_RES_MAIN_CHARS, OBJ_RES_MAIN_PALETTE, OBJ_RES_MAIN_CELL_ANIMS, 0 },
};

// The colors of the answers' slices, by the answers' IDs
static const u8 sGraphAnswerReds[ANSWER_ID_COUNT] = {
    31, 7,  21, 0,  31, 31, 0,  0,  16, 16, 0,  0,  16, 0,  31, 0,  31, 16, 31, 0,  0,  31, 0,  31, 21,
    7,  31, 31, 0,  0,  31, 0,  31, 0,  21, 7,  31, 31, 31, 0,  31, 31, 31, 0,  31, 0,  31, 0,  31, 0,
    31, 0,  31, 0,  20, 14, 9,  3,  0,  31, 31, 0,  0,  0,  31, 0,  29, 18, 18, 0,  31, 0,  29, 31, 0,
    29, 18, 31, 5,  31, 0,  31, 0,  0,  29, 31, 0,  31, 0,  18, 31, 0,  31, 16, 0,  0,  31, 0,  16, 0,
    0,  0,  31, 0,  31, 29, 16, 16, 16, 0,  16, 17, 0,  0,  0,  31, 31, 31, 16, 31, 0,  16, 0,  0,  16,
    16, 17, 0,  29, 0,  10, 3,  0,  3,  18, 20, 18, 16, 14, 12, 10, 9,  7,  5,  3,  0,  0,  0,
};

static const u8 sGraphAnswerGreens[ANSWER_ID_COUNT] = {
    31, 7,  21, 0,  0,  16, 16, 0,  31, 0,  16, 16, 16, 0,  16, 0,  0,  0,  0,  0,  31, 0,  0,  31, 21,
    7,  31, 16, 31, 0,  0,  0,  22, 31, 21, 7,  31, 31, 0,  0,  17, 22, 0,  0,  22, 16, 0,  0,  22, 31,
    0,  0,  22, 31, 20, 14, 9,  3,  0,  0,  22, 31, 16, 31, 0,  0,  0,  18, 18, 16, 0,  0,  0,  0,  0,
    0,  18, 22, 5,  0,  0,  22, 31, 16, 0,  0,  0,  22, 31, 18, 31, 14, 17, 0,  16, 0,  0,  16, 16, 31,
    16, 16, 0,  0,  17, 0,  16, 16, 0,  16, 16, 16, 14, 0,  0,  31, 31, 31, 16, 0,  0,  16, 31, 16, 0,
    0,  16, 14, 0,  16, 10, 9,  0,  3,  18, 20, 18, 16, 14, 12, 10, 9,  7,  5,  3,  0,  0,  0,
};

static const u8 sGraphAnswerBlues[ANSWER_ID_COUNT] = {
    31, 7,  21, 31, 0,  0,  0,  16, 0,  0,  31, 31, 0,  31, 16, 31, 31, 31, 0,  31, 0,  0,  31, 31, 21,
    7,  31, 16, 0,  31, 0,  31, 0,  0,  21, 7,  31, 31, 0,  31, 17, 0,  0,  31, 0,  0,  0,  31, 0,  0,
    0,  31, 0,  0,  20, 14, 9,  3,  31, 0,  0,  0,  31, 0,  0,  31, 31, 18, 18, 31, 0,  31, 31, 0,  31,
    31, 18, 0,  5,  0,  31, 0,  0,  31, 31, 0,  31, 0,  0,  18, 31, 31, 17, 0,  16, 31, 0,  31, 16, 0,
    0,  31, 0,  31, 17, 31, 16, 16, 16, 0,  0,  11, 31, 16, 31, 31, 31, 31, 16, 0,  31, 0,  0,  31, 0,
    16, 11, 31, 31, 0,  10, 10, 16, 3,  18, 20, 18, 16, 14, 12, 10, 9,  7,  5,  3,  0,  0,  0,
};

static const ResearchPaletteAnimeSetup sGraphPaletteAnimes[ANIME_COUNT] = {
    { (u16 *)(HW_BG_PLTT + 0x146), (const u16 *)(HW_BG_PLTT + 0x146), 3, 0, 0xffff },
    { (u16 *)(HW_BG_PLTT + 0x140), (const u16 *)(HW_BG_PLTT + 0x140), 16, 4, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0x80), (const u16 *)(HW_OBJ_PLTT + 0x80), 16, 3, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0x8c), (const u16 *)(HW_OBJ_PLTT + 0x8c), 1, 0, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0x8c), (const u16 *)(HW_OBJ_PLTT + 0x8c), 1, 4, 0xffff },
    { (u16 *)(HW_OBJ_PLTT + 0xa0), (const u16 *)(HW_OBJ_PLTT + 0xa0), 16, 7, 0 },
    { (u16 *)(HW_OBJ_PLTT + 0xa0), (const u16 *)(HW_OBJ_PLTT + 0xa0), 16, 8, 0 },
    { (u16 *)(HW_BG_PLTT + 0x160), (const u16 *)(HW_BG_PLTT + 0x160), 16, 1, 0xffff },
};

static const CursorEntry sGraphCursors[CURSOR_COUNT] = {
    { 2, 0, 28, 4, 16, 0, 224, 32, -8, 12, 232, 12 },
    { 13, 4, 19, 7, 104, 32, 152, 56, 6, 32, 150, 32 },
    { 17, 11, 15, 5, 136, 88, 120, 40, 0, 20, 120, 20 },
    { 13, 16, 19, 5, 104, 128, 152, 40, 4, 19, 150, 19 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

static const BGFontEntry sGraphBGFonts[BG_FONT_COUNT] = {
    { { BG_SUB_TEXT, 2, 0, 28, 3, 0, 4, 15, 3, 4, 0 }, 0, 12, FALSE },
    { { BG_SUB_TEXT, 0, 18, 32, 2, 0, 0, 15, 3, 4, 0 }, 1, 0, TRUE },
    { { BG_SUB_TEXT, 0, 20, 32, 4, 0, 0, 15, 3, 4, 0 }, 1, 0, TRUE },
    { { BG_MAIN_TEXT, 2, 0, 28, 4, 0, 5, 15, 1, 2, 0 }, 1, 0, TRUE },
    { { BG_MAIN_TEXT, 13, 4, 19, 7, 0, 4, 15, 1, 2, 0 }, 1, 13, FALSE },
    { { BG_MAIN_TEXT, 17, 11, 15, 5, 248, 4, 15, 1, 2, 0 }, 1, 14, FALSE },
    { { BG_MAIN_TEXT, 13, 16, 19, 5, 38, 4, 15, 1, 2, 0 }, 0, 15, FALSE },
    { { BG_MAIN_TEXT, 2, 10, 11, 4, 2, 0, 15, 1, 2, 0 }, 0, 10, FALSE },
    { { BG_MAIN_TEXT, 12, 5, 20, 5, 15, 4, 15, 1, 2, 0 }, 0, 19, TRUE },
};

ResearchGraph *ResearchGraph_Create(ResearchCommon *common) {
    HeapID heapId = ResearchCommon_GetHeapID(common);
    ResearchGraph *wk = ResearchGraph_Alloc(heapId);

    ResearchGraph_InitWork(wk);
    ResearchGraph_SetHeapID(wk, heapId);
    ResearchGraph_SetCommon(wk, common);
    return wk;
}

void ResearchGraph_Delete(ResearchGraph *wk) {
    ResearchGraph_DeleteQueue(wk);
    ResearchGraph_Free(wk);
}

void ResearchGraph_Main(ResearchGraph *wk) {
    switch (wk->seq) {
    case SEQ_SETUP:
        ResearchGraph_SeqSetup(wk);
        break;
    case SEQ_RESTART_COMM:
        ResearchGraph_SeqRestartComm(wk);
        break;
    case SEQ_TOUCH:
        ResearchGraph_SeqTouch(wk);
        break;
    case SEQ_CURSOR:
        ResearchGraph_SeqCursor(wk);
        break;
    case SEQ_OPEN:
        ResearchGraph_SeqOpen(wk);
        break;
    case SEQ_SHOW_PERCENTAGES:
        ResearchGraph_SeqShowPercentages(wk);
        break;
    case SEQ_FLASH_IN:
        ResearchGraph_SeqFlashIn(wk);
        break;
    case SEQ_FLASH_OUT:
        ResearchGraph_SeqFlashOut(wk);
        break;
    case SEQ_UPDATE_TOUCH:
        ResearchGraph_SeqUpdateTouch(wk);
        break;
    case SEQ_UPDATE_CURSOR:
        ResearchGraph_SeqUpdateCursor(wk);
        break;
    case SEQ_FADE_OUT:
        ResearchGraph_SeqFadeOut(wk);
        break;
    case SEQ_WAIT:
        ResearchGraph_SeqWait(wk);
        break;
    case SEQ_TEARDOWN:
        ResearchGraph_SeqTeardown(wk);
        break;
    case SEQ_END:
        return;
    }

    ResearchGraph_CheckNewResults(wk);
    ResearchGraph_CountUpdateFrames(wk);
    if (wk->active) {
        func_0204b794();
        ResearchGraph_UpdateGraphs(wk);
        Arrow_Update(wk->arrow);
        ResearchGraph_UpdateCommonPaletteAnime(wk);
        ResearchGraph_UpdatePaletteAnimes(wk);
        ResearchGraph_DrawGraphs(wk);
        ResearchGraph_DrawAnswerColor(wk);
        G3_SwapBuffers(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
    }
    ResearchGraph_CountFrame(wk);
    ResearchGraph_UpdateSeq(wk);
}

BOOL ResearchGraph_IsEnd(ResearchGraph *wk) {
    return wk->isEnd;
}

u32 ResearchGraph_GetNext(ResearchGraph *wk) {
    return wk->next;
}

static void ResearchGraph_SeqSetup(ResearchGraph *wk) {
    u32 startSeq;

    ResearchGraph_CreateQueue(wk);
    ResearchGraph_CreateFont(wk);
    ResearchGraph_LoadMsgData(wk);
    ResearchGraph_CreateWordSet(wk);
    ResearchGraph_InitTouchRects(wk);
    ResearchGraph_UpdateArrowRects(wk);
    ResearchGraph_Init3D();
    ResearchGraph_InitBG(wk);
    ResearchGraph_LoadSubBG(wk);
    ResearchGraph_InitSubText(wk);
    ResearchGraph_LoadMainBG(wk);
    ResearchGraph_InitMainText(wk);
    ResearchGraph_CreateBGFonts(wk);
    ResearchGraph_LoadSubObjRes(wk);
    ResearchGraph_LoadMainObjRes(wk);
    ResearchGraph_CreateUnits(wk);
    ResearchGraph_CreateActors(wk);
    ResearchGraph_CreateBitmaps(wk);
    ResearchGraph_DrawBitmaps(wk);
    ResearchGraph_DrawButton(wk);
    ResearchGraph_CreateBmpOam(wk);
    ResearchGraph_CreateBmpOamActors(wk);
    ResearchGraph_LoadData(wk);
    ResearchGraph_CreateGraphs(wk);
    ResearchGraph_CreateArrow(wk);
    ResearchGraph_CreatePercentages(wk);
    ResearchGraph_CreatePaletteFade(wk);
    ResearchGraph_CreatePaletteAnimes(wk);
    ResearchGraph_SetupPaletteAnimes(wk);
    ResearchGraph_SetVBlank(wk);
    ResearchGraph_ShowCommIcon(wk);
    ResearchGraph_PrintSurvey(wk);
    ResearchGraph_PrintQuestion(wk);
    ResearchGraph_PrintQuestionTitle(wk);
    ResearchGraph_PrintAnswer(wk);
    ResearchGraph_PrintPlayerAnswer(wk);
    ResearchGraph_PrintTotal(wk);
    ResearchGraph_ShowNoAnswers(wk);
    ResearchGraph_ShowUpdating(wk);
    ResearchGraph_UpdateArrows(wk);
    ResearchGraph_LoadGraphBG(wk);
    ResearchGraph_UpdateLegend(wk);
    ResearchGraph_ShowButton(wk);
    ResearchGraph_StartPaletteAnime(wk, ANIME_CURSOR);
    ResearchGraph_StartPaletteAnime(wk, ANIME_FRAME);
    ResearchGraph_UpdateButton(wk);
    wk->active = TRUE;
    ResearchGraph_FadeIn();

    startSeq = ResearchGraph_GetStartSeq(wk);
    ResearchGraph_PushSeq(wk, SEQ_RESTART_COMM);
    ResearchGraph_PushSeq(wk, startSeq);
    if (startSeq == SEQ_CURSOR) {
        ResearchGraph_HighlightCursor(wk);
    }
    ResearchGraph_EndSeq(wk);
}

static void ResearchGraph_SeqRestartComm(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        ResearchGraph_RequestCommRestart(wk);
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (ResearchGraph_RestartComm(wk)) {
            ResearchGraph_EndSeq(wk);
        }
        break;
    }
}

static void ResearchGraph_SeqTouch(ResearchGraph *wk) {
    u32 keys;
    s32 rect;
    s32 commonButton;

    GCTX_HIDGetHeldKeys();
    keys = GCTX_HIDGetPressedKeys();
    rect = func_0203da0c(wk->touchRects);
    commonButton = func_0203da0c(ResearchCommon_GetTouchRects(wk->common));

    if (wk->resultsShown && wk->newResults && wk->updateReady) {
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_UPDATE_TOUCH);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
        ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
        ResearchGraph_PushSeq(wk, SEQ_TOUCH);
        return;
    }

    if (ResearchGraph_IsForceExit(wk) || commonButton == RESEARCH_COMMON_BUTTON_RETURN) {
        ResearchGraph_SetTouchMode(wk, TRUE);
        ResearchGraph_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_WAIT);
        ResearchGraph_PushSeq(wk, SEQ_FADE_OUT);
        ResearchGraph_PushSeq(wk, SEQ_TEARDOWN);
    } else if ((keys & PAD_KEY_UP) || (keys & PAD_KEY_DOWN) || (keys & PAD_KEY_LEFT) || (keys & PAD_KEY_RIGHT) ||
               (keys & PAD_BUTTON_A)) {
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_UpdateButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
    } else if (rect == TOUCH_GRAPH && !wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        wk->touchOpened = TRUE;
        ResearchGraph_PressButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_OPEN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
        ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
        ResearchGraph_PushSeq(wk, SEQ_TOUCH);
    } else if (rect == TOUCH_BUTTON) {
        if (!wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
            wk->touchOpened = TRUE;
            ResearchGraph_PressButton(wk);
            ResearchGraph_EndSeq(wk);
            ResearchGraph_PushSeq(wk, SEQ_OPEN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
            ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
            ResearchGraph_PushSeq(wk, SEQ_TOUCH);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    } else if (rect == TOUCH_QUESTION) {
        if (!wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
            wk->touchOpened = TRUE;
            ResearchGraph_PressButton(wk);
            ResearchGraph_EndSeq(wk);
            ResearchGraph_PushSeq(wk, SEQ_OPEN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
            ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
            ResearchGraph_PushSeq(wk, SEQ_TOUCH);
        } else {
            ResearchGraph_SelectCursor(wk, CURSOR_QUESTION);
        }
    } else if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk) && rect == TOUCH_ANSWER) {
        ResearchGraph_SelectCursor(wk, CURSOR_ANSWER);
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_UpdateButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
    } else if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk) && rect == TOUCH_PLAYER_ANSWER) {
        ResearchGraph_SelectCursor(wk, CURSOR_PLAYER_ANSWER);
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_UpdateButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
    } else if (rect == TOUCH_MODE) {
        ResearchGraph_SelectCursor(wk, CURSOR_MODE);
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_UpdateButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
    } else if (rect == TOUCH_LEFT) {
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_UpdateButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
        switch (wk->cursor) {
        case CURSOR_QUESTION:
            ResearchGraph_PrevQuestion(wk);
            break;
        case CURSOR_ANSWER:
            ResearchGraph_PrevAnswer(wk);
            break;
        case CURSOR_MODE:
            ResearchGraph_ToggleMode(wk);
            break;
        }
    } else if (rect == TOUCH_RIGHT) {
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_UpdateButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
        switch (wk->cursor) {
        case CURSOR_QUESTION:
            ResearchGraph_NextQuestion(wk);
            break;
        case CURSOR_ANSWER:
            ResearchGraph_NextAnswer(wk);
            break;
        case CURSOR_MODE:
            ResearchGraph_ToggleMode(wk);
            break;
        }
    } else if (keys & PAD_BUTTON_B) {
        ResearchGraph_SetTouchMode(wk, FALSE);
        ResearchGraph_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_WAIT);
        ResearchGraph_PushSeq(wk, SEQ_FADE_OUT);
        ResearchGraph_PushSeq(wk, SEQ_TEARDOWN);
    }
}

static void ResearchGraph_SeqCursor(ResearchGraph *wk) {
    u32 keys;
    s32 rect;
    s32 commonButton;

    GCTX_HIDGetHeldKeys();
    keys = GCTX_HIDGetPressedKeys();
    rect = func_0203da0c(wk->touchRects);
    commonButton = func_0203da0c(ResearchCommon_GetTouchRects(wk->common));

    if (wk->resultsShown && wk->newResults && wk->updateReady) {
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_UPDATE_CURSOR);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
        ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
        return;
    }

    if (ResearchGraph_IsForceExit(wk) || commonButton == RESEARCH_COMMON_BUTTON_RETURN) {
        ResearchGraph_SetTouchMode(wk, TRUE);
        ResearchGraph_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_WAIT);
        ResearchGraph_PushSeq(wk, SEQ_FADE_OUT);
        ResearchGraph_PushSeq(wk, SEQ_TEARDOWN);
    } else if (keys & PAD_KEY_UP) {
        ResearchGraph_CursorUp(wk);
    } else if (keys & PAD_KEY_DOWN) {
        ResearchGraph_CursorDown(wk);
    } else if (rect == TOUCH_GRAPH && !wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        wk->touchOpened = TRUE;
        ResearchGraph_PressButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_OPEN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
        ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
        ResearchGraph_PushSeq(wk, SEQ_TOUCH);
    } else if (rect == TOUCH_BUTTON) {
        if (!wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
            wk->touchOpened = TRUE;
            ResearchGraph_PressButton(wk);
            ResearchGraph_EndSeq(wk);
            ResearchGraph_PushSeq(wk, SEQ_OPEN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
            ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
            ResearchGraph_PushSeq(wk, SEQ_TOUCH);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    } else if (rect == TOUCH_QUESTION) {
        if (!wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
            wk->touchOpened = TRUE;
            ResearchGraph_PressButton(wk);
            ResearchGraph_EndSeq(wk);
            ResearchGraph_PushSeq(wk, SEQ_OPEN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
            ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
            ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
            ResearchGraph_PushSeq(wk, SEQ_TOUCH);
        } else {
            ResearchGraph_SelectCursor(wk, CURSOR_QUESTION);
        }
    } else if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk) && rect == TOUCH_ANSWER) {
        ResearchGraph_SelectCursor(wk, CURSOR_ANSWER);
    } else if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk) && rect == TOUCH_PLAYER_ANSWER) {
        ResearchGraph_SelectCursor(wk, CURSOR_PLAYER_ANSWER);
    } else if (rect == TOUCH_MODE) {
        ResearchGraph_SelectCursor(wk, CURSOR_MODE);
    } else if ((keys & PAD_KEY_LEFT) || rect == TOUCH_LEFT) {
        switch (wk->cursor) {
        case CURSOR_QUESTION:
            ResearchGraph_PrevQuestion(wk);
            break;
        case CURSOR_ANSWER:
            ResearchGraph_PrevAnswer(wk);
            break;
        case CURSOR_MODE:
            ResearchGraph_ToggleMode(wk);
            break;
        }
    } else if ((keys & PAD_KEY_RIGHT) || rect == TOUCH_RIGHT) {
        switch (wk->cursor) {
        case CURSOR_QUESTION:
            ResearchGraph_NextQuestion(wk);
            break;
        case CURSOR_ANSWER:
            ResearchGraph_NextAnswer(wk);
            break;
        case CURSOR_MODE:
            ResearchGraph_ToggleMode(wk);
            break;
        }
    } else if ((keys & PAD_BUTTON_A) && !wk->resultsShown && ResearchGraph_GetQuestionTotal(wk) &&
               (wk->cursor == CURSOR_QUESTION || wk->cursor == CURSOR_BUTTON)) {
        wk->touchOpened = FALSE;
        ResearchGraph_PressButton(wk);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_OPEN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_IN);
        ResearchGraph_PushSeq(wk, SEQ_FLASH_OUT);
        ResearchGraph_PushSeq(wk, SEQ_SHOW_PERCENTAGES);
        ResearchGraph_PushSeq(wk, SEQ_CURSOR);
    } else if (keys & PAD_BUTTON_B) {
        ResearchGraph_SetTouchMode(wk, FALSE);
        ResearchGraph_StartReturnAnime(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ResearchGraph_EndSeq(wk);
        ResearchGraph_PushSeq(wk, SEQ_WAIT);
        ResearchGraph_PushSeq(wk, SEQ_FADE_OUT);
        ResearchGraph_PushSeq(wk, SEQ_TEARDOWN);
    }
}

static void ResearchGraph_SeqOpen(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        ResearchGraph_LoadData(wk);
        ResearchGraph_ClearNewResults(wk);
        ResearchGraph_ShowMessage(wk);
        ResearchGraph_SetGraphData(wk, MODE_TODAY);
        ResearchGraph_SetGraphData(wk, MODE_TOTAL);
        CircleGraph_SetVisible(ResearchGraph_GetGraph(wk), TRUE);
        CircleGraph_RequestGrowSlow(ResearchGraph_GetGraph(wk));
        ResearchGraph_FadeButtons(wk, FALSE);
        ResearchGraph_SetArrowsVisible(wk, FALSE);
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (!GFL_SndPlayerIsActiveAny()) {
            if (wk->seqFrames >= OPEN_FRAMES) {
                ResearchGraph_NextSeqState(wk);
            } else {
                GFL_SndSEPlay(SEQ_SE_SYS_81);
            }
        }
        break;
    case 2:
        wk->resultsShown = TRUE;
        ResearchGraph_FadeButtons(wk, TRUE);
        ResearchGraph_SetCursor(wk, CURSOR_ANSWER);
        ResearchGraph_UpdateArrows(wk);
        ResearchGraph_UpdateArrowRects(wk);
        ResearchGraph_SelectFirstAnswer(wk);
        ResearchGraph_HideMessage(wk);
        ResearchGraph_LoadGraphBG(wk);
        ResearchGraph_PrintAnswer(wk);
        ResearchGraph_PrintPlayerAnswer(wk);
        ResearchGraph_ShowUpdating(wk);
        ResearchGraph_PrintTotal(wk);
        ResearchGraph_ShowArrow(wk);
        ResearchGraph_UpdateLegend(wk);
        if (!wk->touchOpened) {
            ResearchGraph_HighlightCursor(wk);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_82);
        ResearchGraph_EndSeq(wk);
        break;
    }
}

static void ResearchGraph_SeqShowPercentages(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        ResearchGraph_SetupPercentages(wk);
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (wk->seqFrames % PERCENTAGE_INTERVAL == 0) {
            ResearchGraph_ShowPercentage(wk, wk->percentagesShown++);
        }
        if (wk->percentageCount <= wk->percentagesShown) {
            ResearchGraph_EndSeq(wk);
        }
        break;
    }
}

static void ResearchGraph_SeqFlashIn(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        ResearchGraph_StopPaletteAnime(wk, ANIME_BUTTON_DISABLE);
        ResearchGraph_FlashIn(wk);
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (ResearchGraph_IsPaletteFadeDone(wk)) {
            ResearchGraph_EndSeq(wk);
        }
        break;
    }
}

static void ResearchGraph_SeqFlashOut(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        ResearchGraph_FlashOut(wk);
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (ResearchGraph_IsPaletteFadeDone(wk)) {
            ResearchGraph_UpdateButton(wk);
            ResearchGraph_EndSeq(wk);
        }
        break;
    }
}

static void ResearchGraph_SeqUpdateTouch(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        wk->updating = TRUE;
        ResearchGraph_ClearNewResults(wk);
        ResearchGraph_ResetUpdateFrames(wk);
        ResearchGraph_LoadData(wk);
        ResearchGraph_FadeButtons(wk, FALSE);
        ResearchGraph_SetArrowsVisible(wk, FALSE);
        ResearchGraph_LoadGraphBG(wk);
        ResearchGraph_PrintAnswer(wk);
        ResearchGraph_ShowUpdating(wk);
        ResearchGraph_ShowNoAnswers(wk);
        ResearchGraph_ShowArrow(wk);
        ResearchGraph_UpdateArrows(wk);
        ResearchGraph_HidePercentages(wk);
        ResearchGraph_SetNextGraphData(wk, MODE_TODAY);
        ResearchGraph_SetNextGraphData(wk, MODE_TOTAL);
        ResearchGraph_SetGraphDepths(wk);
        CircleGraph_SetVisible(ResearchGraph_GetNextGraph(wk), TRUE);
        CircleGraph_RequestGrow(ResearchGraph_GetNextGraph(wk));
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (!GFL_SndPlayerIsActiveAny()) {
            if (wk->seqFrames >= UPDATE_FRAMES) {
                ResearchGraph_NextSeqState(wk);
            } else {
                GFL_SndSEPlay(SEQ_SE_SYS_81);
            }
        }
        break;
    case 2:
        wk->updating = FALSE;
        ResearchGraph_SwapGraphs(wk);
        CircleGraph_SetVisible(ResearchGraph_GetNextGraph(wk), FALSE);
        ResearchGraph_LoadGraphBG(wk);
        ResearchGraph_ShowUpdating(wk);
        ResearchGraph_PrintAnswer(wk);
        ResearchGraph_PrintPlayerAnswer(wk);
        ResearchGraph_PrintTotal(wk);
        ResearchGraph_ShowArrow(wk);
        ResearchGraph_UpdateArrows(wk);
        ResearchGraph_UpdateMarker(wk);
        ResearchGraph_UpdateLegend(wk);
        GFL_SndSEPlay(SEQ_SE_SYS_82);
        ResearchGraph_EndSeq(wk);
        break;
    }
}

static void ResearchGraph_SeqUpdateCursor(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        wk->updating = TRUE;
        ResearchGraph_ClearNewResults(wk);
        ResearchGraph_ResetUpdateFrames(wk);
        ResearchGraph_LoadData(wk);
        ResearchGraph_FadeButtons(wk, FALSE);
        ResearchGraph_SetArrowsVisible(wk, FALSE);
        ResearchGraph_UnhighlightCursor(wk);
        ResearchGraph_LoadGraphBG(wk);
        ResearchGraph_PrintAnswer(wk);
        ResearchGraph_ShowUpdating(wk);
        ResearchGraph_ShowNoAnswers(wk);
        ResearchGraph_ShowArrow(wk);
        ResearchGraph_UpdateArrows(wk);
        ResearchGraph_HidePercentages(wk);
        ResearchGraph_SetNextGraphData(wk, MODE_TODAY);
        ResearchGraph_SetNextGraphData(wk, MODE_TOTAL);
        ResearchGraph_SetGraphDepths(wk);
        CircleGraph_SetVisible(ResearchGraph_GetNextGraph(wk), TRUE);
        CircleGraph_RequestGrow(ResearchGraph_GetNextGraph(wk));
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (!GFL_SndPlayerIsActiveAny()) {
            if (wk->seqFrames >= UPDATE_FRAMES) {
                ResearchGraph_NextSeqState(wk);
            } else {
                GFL_SndSEPlay(SEQ_SE_SYS_81);
            }
        }
        break;
    case 2:
        wk->updating = FALSE;
        ResearchGraph_SwapGraphs(wk);
        CircleGraph_SetVisible(ResearchGraph_GetNextGraph(wk), FALSE);
        ResearchGraph_LoadGraphBG(wk);
        ResearchGraph_HighlightCursor(wk);
        ResearchGraph_ShowUpdating(wk);
        ResearchGraph_PrintAnswer(wk);
        ResearchGraph_PrintPlayerAnswer(wk);
        ResearchGraph_PrintTotal(wk);
        ResearchGraph_ShowArrow(wk);
        ResearchGraph_UpdateArrows(wk);
        ResearchGraph_UpdateMarker(wk);
        ResearchGraph_UpdateLegend(wk);
        GFL_SndSEPlay(SEQ_SE_SYS_82);
        ResearchGraph_EndSeq(wk);
        break;
    }
}

static void ResearchGraph_SeqFadeOut(ResearchGraph *wk) {
    switch (ResearchGraph_GetSeqState(wk)) {
    case 0:
        ResearchGraph_FadeOut();
        ResearchGraph_NextSeqState(wk);
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            ResearchGraph_EndSeq(wk);
        }
        break;
    }
}

static void ResearchGraph_SeqWait(ResearchGraph *wk) {
    if (ResearchGraph_GetWait(wk) < wk->seqFrames) {
        ResearchGraph_EndSeq(wk);
    }
}

static void ResearchGraph_SeqTeardown(ResearchGraph *wk) {
    ResearchGraph_RestorePalettes(wk);
    ResearchGraph_DeletePaletteAnimes(wk);
    ResearchCommon_StopPaletteAnime(wk->common);
    ResearchCommon_RestorePalette(wk->common);
    ResearchGraph_ResetVBlank(wk);
    ResearchGraph_DeletePercentages(wk);
    ResearchGraph_DeleteArrow(wk);
    ResearchGraph_DeleteGraphs(wk);
    ResearchGraph_DeletePaletteFade(wk);
    ResearchGraph_FreeBitmaps(wk);
    ResearchGraph_DeleteBmpOamActors(wk);
    ResearchGraph_DeleteBmpOam(wk);
    ResearchGraph_DeleteActors(wk);
    ResearchGraph_DeleteUnits(wk);
    ResearchGraph_FreeSubObjRes(wk);
    ResearchGraph_FreeMainObjRes(wk);
    ResearchGraph_DeleteBGFonts(wk);
    ResearchGraph_ExitMainText(wk);
    ResearchGraph_UnloadMainBG(wk);
    ResearchGraph_ExitSubText(wk);
    ResearchGraph_UnloadSubBG(wk);
    ResearchGraph_ExitBG(wk);
    ResearchGraph_DeleteWordSet(wk);
    ResearchGraph_FreeMsgData(wk);
    ResearchGraph_DeleteFont(wk);
    ResearchGraph_SetNext(wk, RESEARCH_GRAPH_NEXT_TOP);
    ResearchGraph_PushSeq(wk, SEQ_END);
    ResearchGraph_EndSeq(wk);
    ResearchGraph_EndComm(wk);
    wk->active = FALSE;
    wk->isEnd = TRUE;
}

static void ResearchGraph_EndSeq(ResearchGraph *wk) {
    wk->seqDone = TRUE;
}

static void ResearchGraph_SetTouchMode(ResearchGraph *wk, BOOL touch) {
    ResearchCommon_SetTouchMode(wk->common, touch);
}

static void ResearchGraph_SetNext(ResearchGraph *wk, u32 next) {
    wk->next = next;
}

static u32 ResearchGraph_GetWait(ResearchGraph *wk) {
    return wk->waitFrames;
}

// The cursor is shown from the start when the screen before was left with it
static u32 ResearchGraph_GetStartSeq(ResearchGraph *wk) {
    ResearchCommon *common = wk->common;
    u32 prevSeq = ResearchCommon_GetPrevSeq(common);
    BOOL touch = ResearchCommon_GetTouchMode(common);

    if (prevSeq != RESEARCH_SEQ_INIT && !touch) {
        return SEQ_CURSOR;
    }
    return SEQ_TOUCH;
}

static void ResearchGraph_CountFrame(ResearchGraph *wk) {
    wk->seqFrames++;
}

static void ResearchGraph_PushSeq(ResearchGraph *wk, u32 seq) {
    Queue_Push(wk->queue, seq);
}

static void ResearchGraph_UpdateSeq(ResearchGraph *wk) {
    if (wk->seqDone && !Queue_IsEmpty(wk->queue)) {
        ResearchGraph_SetSeq(wk, Queue_Pop(wk->queue));
    }
}

static void ResearchGraph_SetSeq(ResearchGraph *wk, u32 seq) {
    wk->seq = seq;
    wk->seqState = 0;
    wk->seqFrames = 0;
    wk->seqDone = FALSE;
}

static u32 ResearchGraph_GetSeqState(ResearchGraph *wk) {
    return wk->seqState;
}

static void ResearchGraph_NextSeqState(ResearchGraph *wk) {
    wk->seqState++;
}

// Moves the cursor up, past the places that can't be chosen: the answer and the player's answer until the results are
// shown, and the button after they are
static void ResearchGraph_CursorUp(ResearchGraph *wk) {
    BOOL moving = TRUE;

    ResearchGraph_UnhighlightCursor(wk);
    ResearchGraph_MoveCursor(wk, -1);
    while (moving) {
        if (wk->cursor == CURSOR_ANSWER || wk->cursor == CURSOR_PLAYER_ANSWER) {
            if (!wk->resultsShown || !ResearchGraph_GetQuestionTotal(wk)) {
                ResearchGraph_MoveCursor(wk, -1);
            } else {
                moving = FALSE;
            }
        } else if (wk->cursor == CURSOR_BUTTON) {
            if (wk->resultsShown == TRUE || !ResearchGraph_GetQuestionTotal(wk)) {
                ResearchGraph_MoveCursor(wk, -1);
            } else {
                moving = FALSE;
            }
        } else {
            moving = FALSE;
        }
    }
    ResearchGraph_HighlightCursor(wk);
    ResearchGraph_UpdateArrows(wk);
    ResearchGraph_UpdateMarker(wk);
    ResearchGraph_StartPaletteAnime(wk, ANIME_MOVE);
    if (wk->cursor == CURSOR_BUTTON) {
        ResearchGraph_MoveToButton(wk);
    }
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchGraph_UpdateArrowRects(wk);
}

static void ResearchGraph_CursorDown(ResearchGraph *wk) {
    BOOL moving = TRUE;

    ResearchGraph_UnhighlightCursor(wk);
    ResearchGraph_MoveCursor(wk, 1);
    while (moving) {
        if (wk->cursor == CURSOR_ANSWER || wk->cursor == CURSOR_PLAYER_ANSWER) {
            if (!wk->resultsShown || !ResearchGraph_GetQuestionTotal(wk)) {
                ResearchGraph_MoveCursor(wk, 1);
            } else {
                moving = FALSE;
            }
        } else if (wk->cursor == CURSOR_BUTTON) {
            if (wk->resultsShown == TRUE || !ResearchGraph_GetQuestionTotal(wk)) {
                ResearchGraph_MoveCursor(wk, 1);
            } else {
                moving = FALSE;
            }
        } else {
            moving = FALSE;
        }
    }
    ResearchGraph_HighlightCursor(wk);
    ResearchGraph_UpdateArrows(wk);
    ResearchGraph_UpdateMarker(wk);
    ResearchGraph_StartPaletteAnime(wk, ANIME_MOVE);
    if (wk->cursor == CURSOR_BUTTON) {
        ResearchGraph_MoveToButton(wk);
    }
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    ResearchGraph_UpdateArrowRects(wk);
}

static void ResearchGraph_SelectCursor(ResearchGraph *wk, int pos) {
    ResearchGraph_MoveCursorTo(wk, pos);
    ResearchGraph_StartPaletteAnime(wk, ANIME_MOVE);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void ResearchGraph_MoveCursorTo(ResearchGraph *wk, int pos) {
    if ((pos == CURSOR_ANSWER || pos == CURSOR_PLAYER_ANSWER) &&
        (!wk->resultsShown || !ResearchGraph_GetQuestionTotal(wk))) {
        return;
    }
    ResearchGraph_UnhighlightCursor(wk);
    ResearchGraph_SetCursor(wk, pos);
    ResearchGraph_HighlightCursor(wk);
    ResearchGraph_UpdateArrows(wk);
    ResearchGraph_UpdateMarker(wk);
    ResearchGraph_UpdateArrowRects(wk);
}

static void ResearchGraph_NextQuestion(ResearchGraph *wk) {
    if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        CircleGraph_RequestShrink(ResearchGraph_GetGraph(wk));
    }
    ResearchGraph_LoadData(wk);
    ResearchGraph_ClearNewResults(wk);
    ResearchGraph_ChangeQuestion(wk, 1);
    wk->resultsShown = FALSE;
    ResearchGraph_LoadGraphBG(wk);
    ResearchGraph_HighlightCursor(wk);
    ResearchGraph_PrintQuestion(wk);
    ResearchGraph_PrintQuestionTitle(wk);
    ResearchGraph_ResetAnswer(wk);
    ResearchGraph_PrintPlayerAnswer(wk);
    ResearchGraph_PrintTotal(wk);
    ResearchGraph_ShowNoAnswers(wk);
    ResearchGraph_HidePercentages(wk);
    ResearchGraph_UpdateLegend(wk);
    ResearchGraph_UpdateButton(wk);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void ResearchGraph_PrevQuestion(ResearchGraph *wk) {
    if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        CircleGraph_RequestShrink(ResearchGraph_GetGraph(wk));
    }
    ResearchGraph_LoadData(wk);
    ResearchGraph_ClearNewResults(wk);
    ResearchGraph_ChangeQuestion(wk, -1);
    wk->resultsShown = FALSE;
    ResearchGraph_LoadGraphBG(wk);
    ResearchGraph_HighlightCursor(wk);
    ResearchGraph_PrintQuestion(wk);
    ResearchGraph_PrintQuestionTitle(wk);
    ResearchGraph_ResetAnswer(wk);
    ResearchGraph_PrintPlayerAnswer(wk);
    ResearchGraph_PrintTotal(wk);
    ResearchGraph_ShowNoAnswers(wk);
    ResearchGraph_HidePercentages(wk);
    ResearchGraph_UpdateLegend(wk);
    ResearchGraph_UpdateButton(wk);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void ResearchGraph_NextAnswer(ResearchGraph *wk) {
    ResearchGraph_ChangeAnswer(wk, 1);
    ResearchGraph_PrintAnswer(wk);
    ResearchGraph_ShowArrow(wk);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void ResearchGraph_PrevAnswer(ResearchGraph *wk) {
    ResearchGraph_ChangeAnswer(wk, -1);
    ResearchGraph_PrintAnswer(wk);
    ResearchGraph_ShowArrow(wk);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void ResearchGraph_ResetAnswer(ResearchGraph *wk) {
    ResearchGraph_SetFirstAnswerIndex(wk);
    ResearchGraph_PrintAnswer(wk);
    ResearchGraph_ShowArrow(wk);
}

static void ResearchGraph_SelectFirstAnswer(ResearchGraph *wk) {
    ResearchGraph_SetFirstAnswer(wk);
    ResearchGraph_PrintAnswer(wk);
    ResearchGraph_ShowArrow(wk);
}

// Switches the graph between today's answers and all of them, shrinking the one shown and growing the other
static void ResearchGraph_ToggleMode(ResearchGraph *wk) {
    BOOL shrunk = FALSE;
    u32 mode;

    if (!ResearchGraph_AreGraphsStill(wk)) {
        return;
    }

    if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        CircleGraph_RequestShrink(ResearchGraph_GetGraph(wk));
        shrunk = TRUE;
    }

    switch (wk->mode) {
    case MODE_TODAY:
        mode = MODE_TOTAL;
        break;
    case MODE_TOTAL:
        mode = MODE_TODAY;
        break;
    }
    ResearchGraph_SetMode(wk, mode);
    ResearchGraph_LoadGraphBG(wk);
    ResearchGraph_HighlightCursor(wk);
    ResearchGraph_PrintAnswer(wk);
    ResearchGraph_PrintPlayerAnswer(wk);
    ResearchGraph_PrintTotal(wk);
    ResearchGraph_ShowNoAnswers(wk);
    ResearchGraph_ShowUpdating(wk);
    ResearchGraph_UpdateLegend(wk);
    ResearchGraph_UpdateButton(wk);
    ResearchGraph_ShowArrow(wk);
    ResearchGraph_HidePercentages(wk);

    if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        ResearchGraph_SetupPercentages(wk);
        ResearchGraph_ShowPercentages(wk);
    }

    if (wk->resultsShown && ResearchGraph_GetQuestionTotal(wk)) {
        if (shrunk) {
            CircleGraph_SetWait(ResearchGraph_GetGraph(wk), 20);
        }
        CircleGraph_RequestGrowFast(ResearchGraph_GetGraph(wk));
        CircleGraph_SetVisible(ResearchGraph_GetGraph(wk), TRUE);
    }
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void ResearchGraph_FadeIn(void) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
}

static void ResearchGraph_FadeOut(void) {
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
}

static void ResearchGraph_MoveCursor(ResearchGraph *wk, int dir) {
    wk->cursor = (wk->cursor + dir + CURSOR_COUNT) % CURSOR_COUNT;
}

static void ResearchGraph_SetCursor(ResearchGraph *wk, int pos) {
    wk->cursor = pos;
}

// The answer's and the count's areas take the cursor's palette in two parts, around their labels
static void ResearchGraph_HighlightCursor(ResearchGraph *wk) {
    switch (wk->cursor) {
    case CURSOR_QUESTION:
    case CURSOR_PLAYER_ANSWER:
    case CURSOR_MODE:
        GFL_BGSysSetScrPaletteNo(BG_MAIN_GRAPH, sGraphCursors[wk->cursor].tileX, sGraphCursors[wk->cursor].tileY,
                                 sGraphCursors[wk->cursor].tileWidth, sGraphCursors[wk->cursor].tileHeight,
                                 BG_PALETTE_CURSOR);
        break;
    case CURSOR_ANSWER:
        GFL_BGSysSetScrPaletteNo(BG_MAIN_GRAPH, 13, 4, 19, 4, BG_PALETTE_CURSOR);
        GFL_BGSysSetScrPaletteNo(BG_MAIN_GRAPH, 16, 8, 16, 3, BG_PALETTE_CURSOR);
        break;
    case CURSOR_BUTTON:
        ResearchGraph_HighlightButton(wk);
        break;
    }
    GFL_BGSysLoadScr(BG_MAIN_GRAPH);
}

static void ResearchGraph_UnhighlightCursor(ResearchGraph *wk) {
    switch (wk->cursor) {
    case CURSOR_QUESTION:
    case CURSOR_PLAYER_ANSWER:
    case CURSOR_MODE:
        GFL_BGSysSetScrPaletteNo(BG_MAIN_GRAPH, sGraphCursors[wk->cursor].tileX, sGraphCursors[wk->cursor].tileY,
                                 sGraphCursors[wk->cursor].tileWidth, sGraphCursors[wk->cursor].tileHeight,
                                 BG_PALETTE_NORMAL);
        break;
    case CURSOR_ANSWER:
        GFL_BGSysSetScrPaletteNo(BG_MAIN_GRAPH, 13, 4, 19, 4, BG_PALETTE_NORMAL);
        GFL_BGSysSetScrPaletteNo(BG_MAIN_GRAPH, 16, 8, 16, 3, BG_PALETTE_NORMAL);
        break;
    case CURSOR_BUTTON:
        ResearchGraph_UnhighlightButton(wk);
        break;
    }
    GFL_BGSysLoadScr(BG_MAIN_GRAPH);
}

static void ResearchGraph_ChangeQuestion(ResearchGraph *wk, int dir) {
    wk->question = (wk->question + dir + RESEARCH_QUESTION_COUNT) % RESEARCH_QUESTION_COUNT;
}

// Chooses the answer of the graph's first slice, the most given
static void ResearchGraph_SetFirstAnswer(ResearchGraph *wk) {
    CircleGraph *graph = ResearchGraph_GetGraph(wk);
    int questionId = ResearchGraph_GetQuestionID(wk);
    int id = CircleGraph_GetItemId(graph, 0);
    u8 answer = ResearchData_GetAnswerIndex(&wk->data, questionId, id);

    wk->answer = answer;
}

// Chooses the answer of the slice before or after the chosen one's in the graph
static void ResearchGraph_ChangeAnswer(ResearchGraph *wk, int dir) {
    CircleGraph *graph = ResearchGraph_GetGraph(wk);
    int questionId = ResearchGraph_GetQuestionID(wk);
    int count = ResearchGraph_GetAnswerCount(wk);
    int index = CircleGraph_GetItemIndex(graph, ResearchGraph_GetAnswerID(wk));
    int id;

    index = (count + (index + dir)) % count;
    id = CircleGraph_GetItemId(graph, index);
    wk->answer = ResearchData_GetAnswerIndex(&wk->data, questionId, id);
}

static void ResearchGraph_SetFirstAnswerIndex(ResearchGraph *wk) {
    wk->answer = 0;
}

// The button can be pressed until the results are shown, and only if somebody answered the question
static void ResearchGraph_UpdateButton(ResearchGraph *wk) {
    if (wk->resultsShown == TRUE || wk->updating == TRUE || !ResearchGraph_GetQuestionTotal(wk)) {
        if (!ResearchGraph_IsPaletteAnimeActive(wk, ANIME_BUTTON_DISABLE)) {
            ResearchGraph_DisableButton(wk);
        }
    } else {
        if (ResearchGraph_IsPaletteAnimeActive(wk, ANIME_BUTTON_DISABLE) == TRUE) {
            ResearchGraph_EnableButton(wk);
        }
    }
}

static void ResearchGraph_PressButton(ResearchGraph *wk) {
    BmpOam_ActorSetPaletteOffset(wk->bmpOamActors[BMP_BUTTON], 0);
    ResearchGraph_StopPaletteAnime(wk, ANIME_BUTTON_DISABLE);
    ResearchGraph_StopPaletteAnime(wk, ANIME_BUTTON_ENABLE);
    ResearchGraph_StartPaletteAnime(wk, ANIME_DECIDE);
}

// Darkens the objects' first palette while the graph grows, and brings it back
static void ResearchGraph_FadeButtons(ResearchGraph *wk, BOOL restore) {
    if (restore) {
        PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 1, 0, 0, 0, 0, wk->tcbMgr);
    } else {
        PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 1, 0, 0, 10, 0, wk->tcbMgr);
    }
}

static void ResearchGraph_HighlightButton(ResearchGraph *wk) {
    BmpOam_ActorSetPaletteOffset(wk->bmpOamActors[BMP_BUTTON], 0);
    ResearchGraph_StartPaletteAnime(wk, ANIME_BUTTON_CURSOR);
}

static void ResearchGraph_UnhighlightButton(ResearchGraph *wk) {
    BmpOam_ActorSetPaletteOffset(wk->bmpOamActors[BMP_BUTTON], 1);
    ResearchGraph_StopPaletteAnime(wk, ANIME_BUTTON_CURSOR);
}

static void ResearchGraph_MoveToButton(ResearchGraph *wk) {
    BmpOam_ActorSetPaletteOffset(wk->bmpOamActors[BMP_BUTTON], 0);
    ResearchGraph_StartPaletteAnime(wk, ANIME_BUTTON_MOVE);
}

static void ResearchGraph_EnableButton(ResearchGraph *wk) {
    BmpOam_ActorSetPaletteOffset(wk->bmpOamActors[BMP_BUTTON], 1);
    ResearchGraph_StopPaletteAnime(wk, ANIME_BUTTON_DISABLE);
    ResearchGraph_StartPaletteAnime(wk, ANIME_BUTTON_ENABLE);
}

static void ResearchGraph_DisableButton(ResearchGraph *wk) {
    BmpOam_ActorSetPaletteOffset(wk->bmpOamActors[BMP_BUTTON], 1);
    ResearchGraph_StartPaletteAnime(wk, ANIME_BUTTON_DISABLE);
}

static void ResearchGraph_StartReturnAnime(ResearchGraph *wk) {
    ResearchCommon_StartPaletteAnime(wk->common, 0);
}

static void ResearchGraph_SwapGraphs(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MODE_COUNT; i++) {
        CircleGraph *graph = wk->graphs[i];

        wk->graphs[i] = wk->nextGraphs[i];
        wk->nextGraphs[i] = graph;
    }
}

// The centre of a slice is its color blended a third of the way toward white
static void ResearchGraph_SetCenterColor(CircleGraphData *item) {
    u8 green = (item->color[1] + 62) / 3;
    u8 blue = (item->color[2] + 62) / 3;

    item->centerColor[0] = (item->color[0] + 62) / 3;
    item->centerColor[1] = green;
    item->centerColor[2] = blue;
}

// The old graph is drawn behind the new one that grows over it
static void ResearchGraph_SetGraphDepths(ResearchGraph *wk) {
    CircleGraph_SetDepth(ResearchGraph_GetGraph(wk), -2 * FX16_ONE);
    CircleGraph_SetDepth(ResearchGraph_GetNextGraph(wk), 0);
}

static void ResearchGraph_SetGraphData(ResearchGraph *wk, u32 mode) {
    CircleGraphData data[RESEARCH_ANSWER_MAX];
    CircleGraph *graph = wk->graphs[mode];
    int count = ResearchGraph_GetAnswerCount(wk);
    ResearchQuestion *question = &wk->data.questions[wk->question];
    int i;

    for (i = 0; i < count; i++) {
        ResearchAnswer *answer = &question->answers[i];
        CircleGraphData *item = &data[i];

        item->id = answer->id;
        item->color[0] = answer->red;
        item->color[1] = answer->green;
        item->color[2] = answer->blue;
        ResearchGraph_SetCenterColor(item);
        switch (mode) {
        case MODE_TODAY:
            item->count = answer->todayCount;
            break;
        case MODE_TOTAL:
            item->count = answer->totalCount;
            break;
        }
    }
    CircleGraph_SetData(graph, data, count);
}

static void ResearchGraph_SetNextGraphData(ResearchGraph *wk, u32 mode) {
    CircleGraphData data[RESEARCH_ANSWER_MAX];
    CircleGraph *graph = wk->nextGraphs[mode];
    int count = ResearchGraph_GetAnswerCount(wk);
    ResearchQuestion *question = &wk->data.questions[wk->question];
    int i;

    for (i = 0; i < count; i++) {
        ResearchAnswer *answer = &question->answers[i];
        CircleGraphData *item = &data[i];

        item->id = answer->id;
        item->color[0] = answer->red;
        item->color[1] = answer->green;
        item->color[2] = answer->blue;
        ResearchGraph_SetCenterColor(item);
        switch (mode) {
        case MODE_TODAY:
            item->count = answer->todayCount;
            break;
        case MODE_TOTAL:
            item->count = answer->totalCount;
            break;
        }
    }
    CircleGraph_SetData(graph, data, count);
}

static void ResearchGraph_UpdateGraphs(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MODE_COUNT; i++) {
        CircleGraph_Main(wk->graphs[i]);
        CircleGraph_Main(wk->nextGraphs[i]);
    }
}

static void ResearchGraph_DrawGraphs(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MODE_COUNT; i++) {
        CircleGraph_Draw(wk->graphs[i]);
        CircleGraph_Draw(wk->nextGraphs[i]);
    }
}

// Draws a square of the chosen answer's color next to its name
static void ResearchGraph_DrawAnswerColor(ResearchGraph *wk) {
    if (wk->resultsShown && wk->updating != TRUE && ResearchGraph_GetQuestionTotal(wk)) {
        ResearchData *data = &wk->data;
        u8 red = ResearchData_GetAnswerRed(data, wk->question, wk->answer);
        u8 green = ResearchData_GetAnswerGreen(data, wk->question, wk->answer);
        u8 blue = ResearchData_GetAnswerBlue(data, wk->question, wk->answer);

        G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, 0);
        G3_Begin(GX_BEGIN_QUADS);
        G3_Color(GX_RGB(red, green, blue));
        G3_Vtx(-0x398, 0x96c, 0);
        G3_Vtx(-0x398, 0xa7a, 0);
        G3_Vtx(-0x25f, 0xa7a, 0);
        G3_Vtx(-0x25f, 0x96c, 0);
        G3_End();
    }
}

// Points the arrow from the answer's name to its slice
static void ResearchGraph_ShowArrow(ResearchGraph *wk) {
    int x, y;

    Arrow_Hide(wk->arrow);
    // resultsShown is tested twice, as the original reads it again at the end
    if (wk->updating != TRUE && wk->resultsShown && ResearchGraph_GetQuestionTotal(wk) &&
        CircleGraph_GetPercentById(ResearchGraph_GetGraph(wk), ResearchGraph_GetAnswerID(wk)) && wk->resultsShown) {
        CircleGraph_GetLabelScreenPosById(ResearchGraph_GetGraph(wk), ResearchGraph_GetAnswerID(wk), &x, &y);
        Arrow_SetPath(wk->arrow, 104, 34, x, y);
        Arrow_Start(wk->arrow);
    }
}

// Sets up a percentage on each slice of PERCENTAGE_MIN or more, from the last
static void ResearchGraph_SetupPercentages(ResearchGraph *wk) {
    // The count goes in the loop's counter first: a loop that starts from a call's result is longer
    int i = ResearchGraph_GetAnswerCount(wk);
    CircleGraph *graph = ResearchGraph_GetGraph(wk);
    int count = 0;
    int x, y;

    for (i = i - 1; i >= 0; i--) {
        int percent = CircleGraph_GetPercent(graph, i);

        CircleGraph_GetLabelScreenPos(graph, i, &x, &y);
        if (percent >= PERCENTAGE_MIN) {
            Percentage *percentage = wk->percentages[count];

            Percentage_SetValue(percentage, percent);
            Percentage_SetPos(percentage, x, y);
            count++;
        }
    }
    wk->percentageCount = count;
}

static void ResearchGraph_HidePercentages(ResearchGraph *wk) {
    int i;

    for (i = 0; i < PERCENTAGE_COUNT; i++) {
        Percentage_SetVisible(wk->percentages[i], FALSE);
    }
    wk->percentagesShown = 0;
}

static void ResearchGraph_ShowPercentage(ResearchGraph *wk, u8 index) {
    Percentage_SetVisible(wk->percentages[index], TRUE);
}

static void ResearchGraph_ShowPercentages(ResearchGraph *wk) {
    int i;
    int count = wk->percentageCount;

    for (i = 0; i < count; i++) {
        ResearchGraph_ShowPercentage(wk, i);
    }
}

// The arrows' touch rectangles are 32 pixels square around them
static void ResearchGraph_UpdateArrowRects(ResearchGraph *wk) {
    const CursorEntry *entry = &sGraphCursors[wk->cursor];
    TouchRect *rect = &wk->touchRects[TOUCH_LEFT];

    rect->left = ResearchGraph_ClampU8(entry->x + entry->leftX - 16);
    rect->top = ResearchGraph_ClampU8(entry->y + entry->leftY - 16);
    rect->right = ResearchGraph_ClampU8(rect->left + 32);
    rect->bottom = ResearchGraph_ClampU8(rect->top + 32);

    // The original looks the entry up again for the right arrow
    entry = &sGraphCursors[wk->cursor];
    rect = &wk->touchRects[TOUCH_RIGHT];
    rect->left = ResearchGraph_ClampU8(entry->x + entry->rightX - 16);
    rect->top = ResearchGraph_ClampU8(entry->y + entry->rightY - 16);
    rect->right = ResearchGraph_ClampU8(rect->left + 32);
    rect->bottom = ResearchGraph_ClampU8(rect->top + 32);
}

// The graph's frame: without the labels of the results until they are shown or while they are updated
static void ResearchGraph_LoadGraphBG(ResearchGraph *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, wk->heapId);
    u32 fileId;
    void *file;
    NNSG2dScreenData *screen;

    if (wk->updating) {
        fileId = 8;
    } else if (!ResearchGraph_GetQuestionTotal(wk)) {
        fileId = 6;
    } else if (!wk->resultsShown) {
        fileId = 6;
    } else {
        fileId = 7;
    }
    file = GFL_ArcToolReadHeapNew(handle, fileId, wk->heapId);
    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_MAIN_GRAPH, screen->rawData, 0, 0, 32, 32);
    GFL_BGSysLoadScr(BG_MAIN_GRAPH);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchGraph_PrintSurvey(ResearchGraph *wk) {
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_SURVEY], sGraphSurveyNames[wk->data.surveyId]);
}

static void ResearchGraph_PrintQuestion(ResearchGraph *wk) {
    BGFont_PrintMsg(wk->bgFonts[BG_FONT_QUESTION], sGraphQuestionTexts[ResearchGraph_GetQuestionID(wk)]);
}

static void ResearchGraph_PrintQuestionTitle(ResearchGraph *wk) {
    BGFont *bgFont = wk->bgFonts[BG_FONT_QUESTION_TITLE];

    BGFont_PrintMsg(bgFont, sGraphQuestionTitles[ResearchGraph_GetQuestionID(wk)]);
}

// The chosen answer's rank in the graph, name, count and percentage
static void ResearchGraph_PrintAnswer(ResearchGraph *wk) {
    BGFont *bgFont = wk->bgFonts[BG_FONT_ANSWER];
    CircleGraph *graph = ResearchGraph_GetGraph(wk);
    u16 answerId;
    u8 index;
    u32 total;
    u8 percent;
    StrBuf *format;
    StrBuf *name;
    StrBuf *str;

    if (!wk->resultsShown || wk->updating == TRUE) {
        BGFont_SetVisible(bgFont, FALSE);
        return;
    }
    if (!ResearchGraph_GetQuestionTotal(wk)) {
        BGFont_SetVisible(bgFont, FALSE);
        return;
    }

    answerId = ResearchGraph_GetAnswerID(wk);
    index = CircleGraph_GetItemIndex(graph, answerId);
    total = ResearchGraph_GetAnswerTotal(wk);
    percent = CircleGraph_GetPercentById(graph, answerId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], 13);
    name = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_DATA_ANSWERS], answerId);
    str = GFL_StrBufCreate(0x80, wk->heapId);
    WordSetNumber(wk->wordSet, 0, index + 1, 2, 1, 1);
    func_0202437c(wk->wordSet, 1, name, 0, 1, 2);
    WordSetNumber(wk->wordSet, 2, total, 5, 1, 1);
    WordSetNumber(wk->wordSet, 3, percent, 3, 1, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
    BGFont_PrintStr(bgFont, str);
    BGFont_SetVisible(bgFont, TRUE);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
    GFL_StrBufFree(name);
}

static void ResearchGraph_PrintPlayerAnswer(ResearchGraph *wk) {
    u8 answerId;
    StrBuf *format;
    StrBuf *name;
    StrBuf *str;

    if (!wk->resultsShown || !ResearchGraph_GetQuestionTotal(wk)) {
        BGFont_SetVisible(wk->bgFonts[BG_FONT_PLAYER_ANSWER], FALSE);
        return;
    }

    answerId = ResearchGraph_GetPlayerAnswer(wk);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], 14);
    name = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_DATA_ANSWERS], answerId);
    str = GFL_StrBufCreate(0x80, wk->heapId);
    func_0202437c(wk->wordSet, 0, name, 0, 1, 2);
    GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
    BGFont_SetVisible(wk->bgFonts[BG_FONT_PLAYER_ANSWER], TRUE);
    BGFont_PrintStr(wk->bgFonts[BG_FONT_PLAYER_ANSWER], str);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
    GFL_StrBufFree(name);
}

// How many people answered, today or in all
static void ResearchGraph_PrintTotal(ResearchGraph *wk) {
    u32 strId;
    u32 total;
    WordSet *wordSet;
    StrBuf *format;
    StrBuf *str;

    if (wk->resultsShown) {
        switch (wk->mode) {
        case MODE_TODAY:
            strId = 15;
            break;
        case MODE_TOTAL:
            strId = 17;
            break;
        }
    } else {
        switch (wk->mode) {
        case MODE_TODAY:
            strId = 16;
            break;
        case MODE_TOTAL:
            strId = 18;
            break;
        }
    }

    total = ResearchGraph_GetQuestionTotal(wk);
    wordSet = wk->wordSet;
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], strId);
    str = GFL_StrBufCreate(0x80, wk->heapId);
    WordSetNumber(wordSet, 0, total, 6, 1, 1);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    BGFont_PrintStr(wk->bgFonts[BG_FONT_TOTAL], str);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
}

static void ResearchGraph_ShowNoAnswers(ResearchGraph *wk) {
    if (!ResearchGraph_GetQuestionTotal(wk)) {
        BGFont_SetVisible(wk->bgFonts[BG_FONT_NO_ANSWERS], TRUE);
    } else {
        BGFont_SetVisible(wk->bgFonts[BG_FONT_NO_ANSWERS], FALSE);
    }
}

static void ResearchGraph_ShowUpdating(ResearchGraph *wk) {
    if (wk->updating) {
        BGFont_SetVisible(wk->bgFonts[BG_FONT_UPDATING], TRUE);
    } else {
        BGFont_SetVisible(wk->bgFonts[BG_FONT_UPDATING], FALSE);
    }
}

// The arrows on either side of the cursor, except where there is nothing to choose
static void ResearchGraph_UpdateArrows(ResearchGraph *wk) {
    const CursorEntry *entry = &sGraphCursors[wk->cursor];
    ClActor *left = ResearchGraph_GetActor(wk, ACTOR_LEFT);
    ClActor *right = ResearchGraph_GetActor(wk, ACTOR_RIGHT);
    ClActorPos pos;

    if (wk->updating == TRUE || wk->cursor == CURSOR_PLAYER_ANSWER || wk->cursor == CURSOR_BUTTON) {
        func_0204c124(left, FALSE);
        func_0204c124(right, FALSE);
        return;
    }

    pos.x = entry->x + entry->leftX;
    pos.y = entry->y + entry->leftY;
    func_0204c140(left, &pos, 0);
    pos.x = entry->x + entry->rightX;
    pos.y = entry->y + entry->rightY;
    func_0204c140(right, &pos, 0);
    func_0204c124(left, TRUE);
    func_0204c124(right, TRUE);
    func_0204c520(left, TRUE);
    func_0204c520(right, TRUE);
}

static void ResearchGraph_SetArrowsVisible(ResearchGraph *wk, BOOL visible) {
    ClActor *left = ResearchGraph_GetActor(wk, ACTOR_LEFT);
    ClActor *right = ResearchGraph_GetActor(wk, ACTOR_RIGHT);

    func_0204c124(left, visible);
    func_0204c124(right, visible);
}

static void ResearchGraph_UpdateLegend(ResearchGraph *wk) {
    if (!wk->resultsShown || !ResearchGraph_GetQuestionTotal(wk)) {
        func_0204c124(wk->actors[ACTOR_LEGEND], FALSE);
        return;
    }
    func_0204c124(wk->actors[ACTOR_LEGEND], TRUE);
}

// Marks the player's answer in the graph while the cursor is on it
static void ResearchGraph_UpdateMarker(ResearchGraph *wk) {
    ClActor *marker = wk->actors[ACTOR_MARKER];
    const ResearchActorSetup *entry = &sGraphActors[ACTOR_MARKER];
    u32 answerId;
    int x, y;
    ClActorPos pos;

    if (!wk->resultsShown) {
        func_0204c124(marker, FALSE);
        return;
    }
    if (wk->cursor != CURSOR_PLAYER_ANSWER) {
        func_0204c124(marker, FALSE);
        return;
    }

    answerId = ResearchGraph_GetPlayerAnswer(wk);
    if (!CircleGraph_GetPercentById(ResearchGraph_GetGraph(wk), answerId)) {
        func_0204c124(marker, FALSE);
        return;
    }
    if (answerId == 0) {
        func_0204c124(marker, FALSE);
        return;
    }

    CircleGraph_GetLabelScreenPosById(ResearchGraph_GetGraph(wk), answerId, &x, &y);
    pos.x = x;
    pos.y = y;
    func_0204c140(marker, &pos, entry->surface);
    func_0204c124(marker, TRUE);
    func_0204c520(marker, TRUE);
    func_0204c584(marker, 2);
}

static void ResearchGraph_ShowButton(ResearchGraph *wk) {
    ResearchGraph_SetBmpOamVisible(wk, BMP_BUTTON, TRUE);
}

static void ResearchGraph_ShowMessage(ResearchGraph *wk) {
    ResearchGraph_SetBmpOamVisible(wk, BMP_MESSAGE, TRUE);
}

static void ResearchGraph_HideMessage(ResearchGraph *wk) {
    ResearchGraph_SetBmpOamVisible(wk, BMP_MESSAGE, FALSE);
}

static void ResearchGraph_SetBmpOamVisible(ResearchGraph *wk, u32 index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(wk->bmpOamActors[index], visible);
}

static void ResearchGraph_StartPaletteAnime(ResearchGraph *wk, u32 index) {
    PaletteAnime_Start(wk->animes[index], sGraphPaletteAnimes[index].mode, sGraphPaletteAnimes[index].color);
}

static void ResearchGraph_StopPaletteAnime(ResearchGraph *wk, u32 index) {
    PaletteAnime_Stop(wk->animes[index]);
}

static BOOL ResearchGraph_IsPaletteAnimeActive(ResearchGraph *wk, u32 index) {
    return PaletteAnime_IsActive(wk->animes[index]);
}

static void ResearchGraph_UpdatePaletteAnimes(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Update(wk->animes[i]);
    }
}

static void ResearchGraph_UpdateCommonPaletteAnime(ResearchGraph *wk) {
    ResearchCommon_UpdatePaletteAnime(wk->common);
}

static void ResearchGraph_FlashIn(ResearchGraph *wk) {
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xffff, 3, 0, 7, 0xffff, wk->tcbMgr);
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0xffff, 3, 0, 7, 0xffff, wk->tcbMgr);
}

static void ResearchGraph_FlashOut(ResearchGraph *wk) {
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xffff, 3, 7, 0, 0xffff, wk->tcbMgr);
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0xffff, 3, 7, 0, 0xffff, wk->tcbMgr);
}

static BOOL ResearchGraph_IsPaletteFadeDone(ResearchGraph *wk) {
    if (!PaletteFade_GetActiveMask(wk->paletteFade)) {
        return TRUE;
    }
    return FALSE;
}

static void ResearchGraph_VBlank(TCB *tcb, void *data) {
    ResearchGraph *wk = data;

    GFL_BGSysUpdate();
    func_0204b7c8();
    PaletteFade_Transfer(wk->paletteFade);
}

// New results came in by beacon if today's count of answers saved is no longer the one shown
static void ResearchGraph_CheckNewResults(ResearchGraph *wk) {
    if (GameBeaconSys_PopSurveyUpdated() == TRUE) {
        u16 shown = ResearchGraph_GetQuestionTodayTotal(wk);
        u16 saved = ResearchGraph_GetSavedTodayTotal(wk);

        if (shown != saved) {
            wk->newResults = TRUE;
        }
    }
}

static void ResearchGraph_ClearNewResults(ResearchGraph *wk) {
    wk->newResults = FALSE;
}

static void ResearchGraph_CountUpdateFrames(ResearchGraph *wk) {
    if (wk->updateFrames < UPDATE_WAIT_FRAMES) {
        wk->updateFrames++;
    } else {
        wk->updateReady = TRUE;
    }
}

static void ResearchGraph_ResetUpdateFrames(ResearchGraph *wk) {
    wk->updateFrames = 0;
    wk->updateReady = FALSE;
}

static GameSystem *ResearchGraph_GetGameSystem(ResearchGraph *wk) {
    return ResearchCommon_GetGameSystem(wk->common);
}

static GameData *ResearchGraph_GetGameData(ResearchGraph *wk) {
    return ResearchCommon_GetGameData(wk->common);
}

static void ResearchGraph_SetHeapID(ResearchGraph *wk, HeapID heapId) {
    wk->heapId = heapId;
}

static ResearchCommon *ResearchGraph_GetCommon(ResearchGraph *wk) {
    return wk->common;
}

static void ResearchGraph_SetCommon(ResearchGraph *wk, ResearchCommon *common) {
    wk->common = common;
}

static BOOL ResearchGraph_IsForceExit(ResearchGraph *wk) {
    return ResearchCommon_IsForceExit(ResearchGraph_GetCommon(wk));
}

static void ResearchGraph_SetMode(ResearchGraph *wk, u32 mode) {
    wk->mode = mode;
}

static CircleGraph *ResearchGraph_GetGraph(ResearchGraph *wk) {
    return wk->graphs[wk->mode];
}

static CircleGraph *ResearchGraph_GetNextGraph(ResearchGraph *wk) {
    return wk->nextGraphs[wk->mode];
}

static BOOL ResearchGraph_AreGraphsStill(ResearchGraph *wk) {
    if (CircleGraph_IsMoving(wk->graphs[MODE_TODAY]) || CircleGraph_IsMoving(wk->graphs[MODE_TOTAL]) ||
        CircleGraph_IsMoving(wk->nextGraphs[MODE_TODAY]) || CircleGraph_IsMoving(wk->nextGraphs[MODE_TOTAL])) {
        return FALSE;
    }
    return TRUE;
}

static u8 ResearchGraph_GetQuestionID(ResearchGraph *wk) {
    return wk->data.questions[wk->question].id;
}

static u8 ResearchGraph_GetAnswerCount(ResearchGraph *wk) {
    return wk->data.questions[wk->question].answerCount;
}

static u16 ResearchGraph_GetAnswerID(ResearchGraph *wk) {
    return wk->data.questions[wk->question].answers[wk->answer].id;
}

// How many people answered the question, today or in all
static u32 ResearchGraph_GetQuestionTotal(ResearchGraph *wk) {
    u32 total = 0;

    switch (wk->mode) {
    case MODE_TODAY:
        total = ResearchGraph_GetQuestionTodayTotal(wk);
        break;
    case MODE_TOTAL:
        total = ResearchGraph_GetQuestionAllTotal(wk);
        break;
    }
    return total;
}

static u32 ResearchGraph_GetQuestionTodayTotal(ResearchGraph *wk) {
    return wk->data.questions[wk->question].todayCount;
}

static u32 ResearchGraph_GetQuestionAllTotal(ResearchGraph *wk) {
    return wk->data.questions[wk->question].totalCount;
}

// How many people gave the chosen answer, today or in all
static u32 ResearchGraph_GetAnswerTotal(ResearchGraph *wk) {
    switch (wk->mode) {
    case MODE_TODAY:
        return ResearchGraph_GetAnswerTodayTotal(wk);
    case MODE_TOTAL:
        return ResearchGraph_GetAnswerAllTotal(wk);
    }
    return 0;
}

static u32 ResearchGraph_GetAnswerTodayTotal(ResearchGraph *wk) {
    return wk->data.questions[wk->question].answers[wk->answer].todayCount;
}

static u32 ResearchGraph_GetAnswerAllTotal(ResearchGraph *wk) {
    return wk->data.questions[wk->question].answers[wk->answer].totalCount;
}

// The survey being run, found from its questions, or the first if none matches
static u8 ResearchGraph_GetSurveyID(ResearchGraph *wk) {
    int i;
    u8 questions[RESEARCH_QUESTION_COUNT];
    u8 id;
    void *survey = func_0200ec2c(GameData_GetSaveControl(ResearchGraph_GetGameData(wk)));

    for (i = 0; i < RESEARCH_QUESTION_COUNT; i++) {
        questions[i] = func_0200ece4(survey, i);
    }
    for (id = 0; id < SURVEY_COUNT; id++) {
        if (questions[0] == sGraphSurveyQuestions1[id] && questions[1] == sGraphSurveyQuestions2[id] &&
            questions[2] == sGraphSurveyQuestions3[id]) {
            return id;
        }
    }
    return 0;
}

// The ID of the player's answer to the question, or 0 if the player hasn't answered it
static u8 ResearchGraph_GetPlayerAnswer(ResearchGraph *wk) {
    void *answers = func_0200ec38(func_0200ec2c(GameData_GetSaveControl(ResearchGraph_GetGameData(wk))));
    u8 questionId = ResearchGraph_GetQuestionID(wk);
    u16 answer;

    if (questionId == QUESTION_PLAY_TIME) {
        return ResearchGraph_GetPlayTimeAnswer(wk);
    }

    answer = func_0200ec3c(answers, questionId);
    if (answer == 0) {
        return 0;
    }
    answer--;
    return ResearchData_GetAnswerID(&wk->data, wk->question, answer);
}

static u8 ResearchGraph_GetPlayTimeAnswer(ResearchGraph *wk) {
    u16 hours = func_02008cec(func_02017a40(ResearchGraph_GetGameData(wk)));

    if (hours < 10) {
        return ANSWER_PLAY_TIME_FIRST;
    } else if (hours < 20) {
        return ANSWER_PLAY_TIME_FIRST + 1;
    } else if (hours < 30) {
        return ANSWER_PLAY_TIME_FIRST + 2;
    } else if (hours < 40) {
        return ANSWER_PLAY_TIME_FIRST + 3;
    } else if (hours < 50) {
        return ANSWER_PLAY_TIME_FIRST + 4;
    } else if (hours < 60) {
        return ANSWER_PLAY_TIME_FIRST + 5;
    } else if (hours < 70) {
        return ANSWER_PLAY_TIME_FIRST + 6;
    } else if (hours < 80) {
        return ANSWER_PLAY_TIME_FIRST + 7;
    } else if (hours < 90) {
        return ANSWER_PLAY_TIME_FIRST + 8;
    } else if (hours < 100) {
        return ANSWER_PLAY_TIME_FIRST + 9;
    }
    return ANSWER_PLAY_TIME_FIRST + 10;
}

static u32 ResearchGraph_GetSavedTodayTotal(ResearchGraph *wk) {
    void *survey = func_0200ec2c(GameData_GetSaveControl(ResearchGraph_GetGameData(wk)));

    return func_0200ecf0(survey, ResearchGraph_GetQuestionID(wk));
}

static u32 ResearchGraph_GetObjRes(ResearchGraph *wk, u32 index) {
    return wk->objRes[index];
}

static ClActUnit *ResearchGraph_GetUnit(ResearchGraph *wk, u32 index) {
    return wk->units[index];
}

static ClActor *ResearchGraph_GetActor(ResearchGraph *wk, u32 index) {
    return wk->actors[index];
}

static ResearchGraph *ResearchGraph_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ResearchGraph), FALSE, "research_graph.c", 4191);
}

static void ResearchGraph_InitWork(ResearchGraph *wk) {
    int i;

    wk->seq = SEQ_SETUP;
    wk->seqState = 0;
    wk->seqDone = FALSE;
    wk->waitFrames = 15;
    wk->seqFrames = 0;
    wk->updateFrames = 0;
    wk->cursor = CURSOR_QUESTION;
    wk->active = FALSE;
    wk->resultsShown = FALSE;
    wk->touchOpened = FALSE;
    wk->updating = FALSE;
    wk->updateReady = FALSE;
    wk->newResults = FALSE;
    wk->question = 0;
    wk->answer = 0;
    wk->mode = MODE_TODAY;
    wk->tcbMgr = GFL_VBlankGetTCBMgr();
    wk->percentageCount = 0;
    wk->percentagesShown = 0;
    wk->next = RESEARCH_GRAPH_NEXT_TOP;
    wk->isEnd = FALSE;
    for (i = 0; i < OBJ_RES_COUNT; i++) {
        wk->objRes[i] = 0;
    }
    ResearchGraph_ClearData(wk);
    ResearchGraph_ClearQueue(wk);
    ResearchGraph_ClearGraphs(wk);
    ResearchGraph_ClearArrow(wk);
    ResearchGraph_ClearPercentages(wk);
    ResearchGraph_ClearMsgData(wk);
    ResearchGraph_ClearWordSet(wk);
    ResearchGraph_ClearFont(wk);
    ResearchGraph_ClearBGFonts(wk);
    ResearchGraph_ClearUnits(wk);
    ResearchGraph_ClearActors(wk);
    ResearchGraph_ClearBitmaps(wk);
    ResearchGraph_ClearPaletteFade(wk);
    ResearchGraph_ClearPaletteAnimes(wk);
}

static void ResearchGraph_Free(ResearchGraph *wk) {
    GFL_HeapFree(wk);
}

static void ResearchGraph_ClearQueue(ResearchGraph *wk) {
    wk->queue = NULL;
}

static void ResearchGraph_CreateQueue(ResearchGraph *wk) {
    wk->queue = Queue_Create(10, wk->heapId);
}

static void ResearchGraph_DeleteQueue(ResearchGraph *wk) {
    Queue_Delete(wk->queue);
}

static void ResearchGraph_ClearFont(ResearchGraph *wk) {
    wk->font = NULL;
}

static void ResearchGraph_CreateFont(ResearchGraph *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
}

static void ResearchGraph_DeleteFont(ResearchGraph *wk) {
    GFL_FontFree(wk->font);
}

static void ResearchGraph_ClearMsgData(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        wk->msgData[i] = NULL;
    }
}

static void ResearchGraph_LoadMsgData(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        wk->msgData[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sGraphMsgFiles[i], wk->heapId);
    }
}

static void ResearchGraph_FreeMsgData(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MSG_DATA_COUNT; i++) {
        GFL_MsgDataFree(wk->msgData[i]);
    }
}

static void ResearchGraph_ClearWordSet(ResearchGraph *wk) {
    wk->wordSet = NULL;
}

static void ResearchGraph_CreateWordSet(ResearchGraph *wk) {
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
}

static void ResearchGraph_DeleteWordSet(ResearchGraph *wk) {
    GFL_WordSetSystemFree(wk->wordSet);
}

static void ResearchGraph_ClearGraphs(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MODE_COUNT; i++) {
        wk->graphs[i] = NULL;
        wk->nextGraphs[i] = NULL;
    }
}

static void ResearchGraph_CreateGraphs(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MODE_COUNT; i++) {
        wk->graphs[i] = CircleGraph_Create(wk->heapId);
        wk->nextGraphs[i] = CircleGraph_Create(wk->heapId);
    }
}

static void ResearchGraph_DeleteGraphs(ResearchGraph *wk) {
    int i;

    for (i = 0; i < MODE_COUNT; i++) {
        CircleGraph_Delete(wk->graphs[i]);
        CircleGraph_Delete(wk->nextGraphs[i]);
    }
}

static void ResearchGraph_ClearData(ResearchGraph *wk) {
    sys_memset(&wk->data, 0, sizeof(ResearchData));
}

// Reads the survey's questions and the counts of their answers from the save, adding today's to those of before
static void ResearchGraph_LoadData(ResearchGraph *wk) {
    void *survey = func_0200ec2c(GameData_GetSaveControl(ResearchGraph_GetGameData(wk)));
    int count;
    int q, i;
    u8 surveyId = ResearchGraph_GetSurveyID(wk);
    u16 answerIds[RESEARCH_QUESTION_COUNT][RESEARCH_ANSWER_MAX];
    u32 todayCounts[RESEARCH_QUESTION_COUNT][RESEARCH_ANSWER_MAX];
    u32 pastCounts[RESEARCH_QUESTION_COUNT][RESEARCH_ANSWER_MAX];
    u32 todayTotals[RESEARCH_QUESTION_COUNT];
    u32 pastTotals[RESEARCH_QUESTION_COUNT];
    u8 questions[RESEARCH_QUESTION_COUNT];
    u8 answerCounts[RESEARCH_QUESTION_COUNT];

    questions[0] = func_0200ece4(survey, 0);
    questions[1] = func_0200ece4(survey, 1);
    questions[2] = func_0200ece4(survey, 2);
    answerCounts[0] = GetSurveyAnswerMsgIDCount(questions[0]);
    answerCounts[1] = GetSurveyAnswerMsgIDCount(questions[1]);
    answerCounts[2] = GetSurveyAnswerMsgIDCount(questions[2]);
    todayTotals[0] = func_0200ecf0(survey, questions[0]);
    todayTotals[1] = func_0200ecf0(survey, questions[1]);
    todayTotals[2] = func_0200ecf0(survey, questions[2]);
    pastTotals[0] = func_0200ed14(survey, questions[0]);
    pastTotals[1] = func_0200ed14(survey, questions[1]);
    pastTotals[2] = func_0200ed14(survey, questions[2]);

    for (q = 0; q < RESEARCH_QUESTION_COUNT; q++) {
        u8 question = questions[q];

        count = answerCounts[q];
        for (i = 0; i < count; i++) {
            answerIds[q][i] = GetSurveyAnswerMsgID(question, i);
            todayCounts[q][i] = func_0200ed48(survey, question, i + 1);
            pastCounts[q][i] = func_0200ed90(survey, question, i + 1);
        }
    }

    wk->data.surveyId = surveyId;
    for (q = 0; q < RESEARCH_QUESTION_COUNT; q++) {
        wk->data.questions[q].id = questions[q];
        wk->data.questions[q].answerCount = answerCounts[q];
        wk->data.questions[q].todayCount = todayTotals[q];
        wk->data.questions[q].totalCount = todayTotals[q] + pastTotals[q];
        if (wk->data.questions[q].totalCount > COUNT_MAX) {
            wk->data.questions[q].totalCount = COUNT_MAX;
        }
#ifdef BUGFIX
        for (i = 0; i < answerCounts[q]; i++) {
#else
        // BUG: The answers past the question's count were never loaded, so their uninitialized IDs index the color
        // tables out of bounds. Nothing reads those answers afterwards
        for (i = 0; i < RESEARCH_ANSWER_MAX; i++) {
#endif
            u16 id = answerIds[q][i];

            wk->data.questions[q].answers[i].id = id;
            wk->data.questions[q].answers[i].red = sGraphAnswerReds[id];
            wk->data.questions[q].answers[i].green = sGraphAnswerGreens[id];
            wk->data.questions[q].answers[i].blue = sGraphAnswerBlues[id];
            wk->data.questions[q].answers[i].todayCount = todayCounts[q][i];
            wk->data.questions[q].answers[i].totalCount = todayCounts[q][i] + pastCounts[q][i];
        }
    }
}

static void ResearchGraph_InitTouchRects(ResearchGraph *wk) {
    int i;

    for (i = 0; i < TOUCH_RECT_COUNT; i++) {
        wk->touchRects[i].left = sGraphTouchRects[i].left;
        wk->touchRects[i].right = sGraphTouchRects[i].right;
        wk->touchRects[i].top = sGraphTouchRects[i].top;
        wk->touchRects[i].bottom = sGraphTouchRects[i].bottom;
    }
}

static void ResearchGraph_ClearArrow(ResearchGraph *wk) {
    wk->arrow = NULL;
}

static void ResearchGraph_CreateArrow(ResearchGraph *wk) {
    ArrowResources resources;

    resources.chars = ResearchGraph_GetObjRes(wk, OBJ_RES_MAIN_CHARS);
    resources.palette = ResearchGraph_GetObjRes(wk, OBJ_RES_MAIN_PALETTE);
    resources.cellAnims = ResearchGraph_GetObjRes(wk, OBJ_RES_MAIN_CELL_ANIMS);
    resources.surface = 0;
    resources.sequences[0] = 0;
    resources.sequences[1] = 1;
    resources.sequences[2] = 2;
    resources.sequences[3] = 3;
    wk->arrow = Arrow_Create(wk->heapId, &resources);
}

static void ResearchGraph_DeleteArrow(ResearchGraph *wk) {
    Arrow_Delete(wk->arrow);
}

static void ResearchGraph_ClearPercentages(ResearchGraph *wk) {
    int i;

    for (i = 0; i < PERCENTAGE_COUNT; i++) {
        wk->percentages[i] = NULL;
    }
}

static void ResearchGraph_CreatePercentages(ResearchGraph *wk) {
    int i;
    PercentageResources resources;

    resources.chars = ResearchGraph_GetObjRes(wk, OBJ_RES_MAIN_CHARS);
    resources.palette = ResearchGraph_GetObjRes(wk, OBJ_RES_MAIN_PALETTE);
    resources.cellAnims = ResearchGraph_GetObjRes(wk, OBJ_RES_MAIN_CELL_ANIMS);
    resources.surface = 0;
    resources.digitSequence = 12;
    resources.percentSequence = 13;
    for (i = 0; i < PERCENTAGE_COUNT; i++) {
        wk->percentages[i] = Percentage_Create(wk->heapId, &resources, ResearchGraph_GetUnit(wk, UNIT_PERCENTAGES));
        Percentage_SetVisible(wk->percentages[i], FALSE);
    }
}

static void ResearchGraph_DeletePercentages(ResearchGraph *wk) {
    int i;

    for (i = 0; i < PERCENTAGE_COUNT; i++) {
        Percentage_Delete(wk->percentages[i]);
    }
}

static void ResearchGraph_Init3D(void) {
    gfxInit3D();
    gfxResetMatrixStack();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Identity();
    G3X_AntiAlias(TRUE);
    G3X_EdgeMarking(TRUE);
    gfxSetEdgeColorTable(sGraphEdgeColors);
}

static void ResearchGraph_InitBG(ResearchGraph *wk) {
    GFL_BGSysSetLCDConfig(&sGraphLCDConfig);
    GFL_BGSysSet3DBGPriority(2);
    GFL_BGSysCreateBG(BG_SUB_TITLE, &sGraphBGSubTitle, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_SUB_TEXT, &sGraphBGSubText, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_GRAPH, &sGraphBGMainGraph, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_MAIN_TEXT, &sGraphBGMainText, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_BACK, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_PATTERN, TRUE);
    GFL_BGSysSetBGEnabled(BG_SUB_TITLE, TRUE);
    GFL_BGSysSetBGEnabled(BG_SUB_TEXT, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_3D, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_MAIN_FRAME, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_GRAPH, TRUE);
    GFL_BGSysSetBGEnabled(BG_MAIN_TEXT, TRUE);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 7, 15);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG1, 16, 5);
    BmpWin_InitAllocator(wk->heapId);
}

static void ResearchGraph_ExitBG(ResearchGraph *wk) {
    BmpWin_FreeAllocator();
    GFL_BGSysReleaseBG(BG_MAIN_3D);
    GFL_BGSysReleaseBG(BG_MAIN_TEXT);
    GFL_BGSysReleaseBG(BG_MAIN_GRAPH);
    GFL_BGSysReleaseBG(BG_SUB_TEXT);
    GFL_BGSysReleaseBG(BG_SUB_TITLE);
}

static void ResearchGraph_LoadSubBG(ResearchGraph *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, wk->heapId);
    void *file = GFL_ArcToolReadHeapNew(handle, 13, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_SUB_TITLE, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysLoadScr(BG_SUB_TITLE);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchGraph_UnloadSubBG(ResearchGraph *wk) {
}

static void ResearchGraph_InitSubText(ResearchGraph *wk) {
    GFL_BGSysFillChar(BG_SUB_TEXT, 0, 1, 0);
    GFL_BGSysClearScr(BG_SUB_TEXT);
}

static void ResearchGraph_ExitSubText(ResearchGraph *wk) {
    GFL_BGSysFreeFilledChar(BG_SUB_TEXT, 1, 0);
}

static void ResearchGraph_LoadMainBG(ResearchGraph *wk) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, wk->heapId);
    void *file = GFL_ArcToolReadHeapNew(handle, 7, wk->heapId);
    NNSG2dScreenData *screen;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(BG_MAIN_GRAPH, screen->rawData, 0, 0, 32, 32);
    GFL_BGSysLoadScr(BG_MAIN_GRAPH);
    GFL_HeapFree(file);
    GFL_ArcToolFree(handle);
}

static void ResearchGraph_UnloadMainBG(ResearchGraph *wk) {
}

static void ResearchGraph_InitMainText(ResearchGraph *wk) {
    GFL_BGSysFillChar(BG_MAIN_TEXT, 0, 1, 0);
    GFL_BGSysClearScr(BG_MAIN_TEXT);
}

static void ResearchGraph_ExitMainText(ResearchGraph *wk) {
    GFL_BGSysFreeFilledChar(BG_MAIN_TEXT, 1, 0);
}

static void ResearchGraph_ClearBGFonts(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        wk->bgFonts[i] = NULL;
    }
}

static void ResearchGraph_CreateBGFonts(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        BGFontSetup setup;

        setup.window.bg = sGraphBGFonts[i].window.bg;
        setup.window.x = sGraphBGFonts[i].window.x;
        setup.window.y = sGraphBGFonts[i].window.y;
        setup.window.width = sGraphBGFonts[i].window.width;
        setup.window.height = sGraphBGFonts[i].window.height;
        setup.window.textX = sGraphBGFonts[i].window.textX;
        setup.window.textY = sGraphBGFonts[i].window.textY;
        setup.window.palette = sGraphBGFonts[i].window.palette;
        setup.window.letterColor = sGraphBGFonts[i].window.letterColor;
        setup.window.shadowColor = sGraphBGFonts[i].window.shadowColor;
        setup.window.backColor = sGraphBGFonts[i].window.backColor;
        setup.centered = sGraphBGFonts[i].centered;
        wk->bgFonts[i] = BGFont_Create(&setup, wk->font, wk->msgData[sGraphBGFonts[i].msgDataIndex], wk->heapId);
        BGFont_PrintMsg(wk->bgFonts[i], sGraphBGFonts[i].strId);
    }
}

static void ResearchGraph_DeleteBGFonts(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BG_FONT_COUNT; i++) {
        BGFont_Delete(wk->bgFonts[i]);
        wk->bgFonts[i] = NULL;
    }
}

static void ResearchGraph_LoadSubObjRes(ResearchGraph *wk) {
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

static void ResearchGraph_FreeSubObjRes(ResearchGraph *wk) {
    func_0204b98c(wk->objRes[OBJ_RES_SUB_CHARS]);
    func_0204bcd0(wk->objRes[OBJ_RES_SUB_PALETTE]);
    func_0204be64(wk->objRes[OBJ_RES_SUB_CELL_ANIMS]);
}

static void ResearchGraph_LoadMainObjRes(ResearchGraph *wk) {
    HeapID heapId = wk->heapId;
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, heapId);
    u32 chars = func_0204b81c(handle, 15, FALSE, CLACT_VRAM_MAIN, heapId);
    u32 palette = func_0204bbb8(handle, 16, CLACT_VRAM_MAIN, 0xc0, 0, 3, heapId);
    u32 cellAnims = func_0204bde0(handle, 14, 17, heapId);
    ArcTool *commonHandle;
    u32 buttonPalette;

    GFL_ArcToolFree(handle);

    commonHandle = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, heapId);
    buttonPalette = func_0204bbb8(commonHandle, 31, CLACT_VRAM_MAIN, 0x80, 0, 2, heapId);
    GFL_ArcToolFree(commonHandle);

    wk->objRes[OBJ_RES_MAIN_CHARS] = chars;
    wk->objRes[OBJ_RES_MAIN_PALETTE] = palette;
    wk->objRes[OBJ_RES_BUTTON_PALETTE] = buttonPalette;
    wk->objRes[OBJ_RES_MAIN_CELL_ANIMS] = cellAnims;
}

static void ResearchGraph_FreeMainObjRes(ResearchGraph *wk) {
    func_0204b98c(wk->objRes[OBJ_RES_MAIN_CHARS]);
    func_0204bcd0(wk->objRes[OBJ_RES_MAIN_PALETTE]);
    func_0204bcd0(wk->objRes[OBJ_RES_BUTTON_PALETTE]);
    func_0204be64(wk->objRes[OBJ_RES_MAIN_CELL_ANIMS]);
}

static void ResearchGraph_ClearUnits(ResearchGraph *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        wk->units[i] = NULL;
    }
}

static void ResearchGraph_CreateUnits(ResearchGraph *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        wk->units[i] = func_0204bf1c(sGraphUnitCounts[i], sGraphUnitPriorities[i], wk->heapId);
    }
}

static void ResearchGraph_DeleteUnits(ResearchGraph *wk) {
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        func_0204bf98(wk->units[i]);
    }
}

static void ResearchGraph_ClearActors(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        wk->actors[i] = NULL;
    }
}

static void ResearchGraph_CreateActors(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        const ResearchActorSetup *entry = &sGraphActors[i];
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
        unit = ResearchGraph_GetUnit(wk, entry->unit);
        chars = ResearchGraph_GetObjRes(wk, entry->chars);
        palette = ResearchGraph_GetObjRes(wk, entry->palette);
        cellAnims = ResearchGraph_GetObjRes(wk, entry->cellAnims);
        wk->actors[i] = func_0204c040(unit, chars, palette, cellAnims, &setup, entry->surface, wk->heapId);
        func_0204c124(wk->actors[i], FALSE);
    }
}

static void ResearchGraph_DeleteActors(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
}

static void ResearchGraph_ClearBitmaps(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        wk->bitmaps[i] = NULL;
    }
}

static void ResearchGraph_CreateBitmaps(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        wk->bitmaps[i] =
            GFL_BitmapCreate(sGraphBitmaps[i].width, sGraphBitmaps[i].height, sGraphBitmaps[i].tileSize, wk->heapId);
    }
}

static void ResearchGraph_DrawBitmaps(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        const BitmapEntry *entry = &sGraphBitmaps[i];

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

// Draws the button from the tiles of the apps' button, 3 by 3 of them for its corners, edges and middle
static void ResearchGraph_DrawButton(ResearchGraph *wk) {
    int tiles[3][20] = {
        { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 },
        { 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5 },
        { 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8 },
    };
    const BitmapEntry *entry = &sGraphBitmaps[BMP_BUTTON];
    int x, y;
    GFLBitmap *base;
    u16 color;
    StrBuf *str;

    base = GFL_G2DIOLoadBitmap(entry->arcId, entry->fileId, FALSE, wk->heapId);
    for (y = 0; y < 3; y++) {
        for (x = 0; x < 20; x++) {
            GFL_BitmapCopyArea(base, wk->bitmaps[BMP_BUTTON], tiles[y][x] % 3 * 8, tiles[y][x] / 3 * 8, x * 8, y * 8, 8,
                               8, 0);
        }
    }
    GFL_BitmapFree(base);

    color = PRINT_COLOR(entry->letterColor, entry->shadowColor, entry->backColor);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData[entry->msgDataIndex], entry->strId);
    GFL_TextRendererDrawToBitmapEx(wk->bitmaps[BMP_BUTTON], entry->textX, entry->textY, str, wk->font, color);
    GFL_HeapFree(str);
}

static void ResearchGraph_FreeBitmaps(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        GFL_BitmapFree(wk->bitmaps[i]);
    }
}

static void ResearchGraph_CreateBmpOam(ResearchGraph *wk) {
    wk->bmpOamSys = BmpOam_Init(wk->heapId, wk->units[UNIT_BMP_OAM]);
}

static void ResearchGraph_DeleteBmpOam(ResearchGraph *wk) {
    BmpOam_Exit(wk->bmpOamSys);
}

static void ResearchGraph_CreateBmpOamActors(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        BmpOamActorSetup setup;

        setup.bitmap = wk->bitmaps[i];
        setup.x = sGraphBmpOams[i].x;
        setup.y = sGraphBmpOams[i].y;
        setup.palette = ResearchGraph_GetObjRes(wk, sGraphBmpOams[i].palette);
        setup.paletteOffset = sGraphBmpOams[i].palOffset;
        setup.priority = sGraphBmpOams[i].priority;
        setup.bgPriority = sGraphBmpOams[i].bgPriority;
        setup.surface = sGraphBmpOams[i].surface;
        setup.vramType = sGraphBmpOams[i].vramType;
        wk->bmpOamActors[i] = BmpOam_ActorAdd(wk->bmpOamSys, &setup);
        BmpOam_ActorSetDrawEnable(wk->bmpOamActors[i], FALSE);
        BmpOam_ActorBmpTrans(wk->bmpOamActors[i]);
    }
}

static void ResearchGraph_DeleteBmpOamActors(ResearchGraph *wk) {
    int i;

    for (i = 0; i < BMP_COUNT; i++) {
        BmpOam_ActorDel(wk->bmpOamActors[i]);
    }
}

static void ResearchGraph_ClearPaletteFade(ResearchGraph *wk) {
    wk->paletteFade = NULL;
}

static void ResearchGraph_CreatePaletteFade(ResearchGraph *wk) {
    wk->paletteFade = PaletteFade_Create(wk->heapId);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, wk->heapId);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, wk->heapId);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_VRAM_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_VRAM_MAIN_OBJ, 0, 0x200);
}

static void ResearchGraph_DeletePaletteFade(ResearchGraph *wk) {
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_Free(wk->paletteFade);
}

static void ResearchGraph_ClearPaletteAnimes(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        wk->animes[i] = NULL;
    }
}

static void ResearchGraph_CreatePaletteAnimes(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        wk->animes[i] = PaletteAnime_Create(wk->heapId);
    }
}

static void ResearchGraph_DeletePaletteAnimes(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Delete(wk->animes[i]);
    }
}

static void ResearchGraph_SetupPaletteAnimes(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Setup(wk->animes[i], sGraphPaletteAnimes[i].dst, sGraphPaletteAnimes[i].src,
                           sGraphPaletteAnimes[i].count);
    }
}

static void ResearchGraph_RestorePalettes(ResearchGraph *wk) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Restore(wk->animes[i]);
    }
}

static void ResearchGraph_ShowCommIcon(ResearchGraph *wk) {
    func_02042ba8(TRUE, wk->heapId);
}

static void ResearchGraph_SetVBlank(ResearchGraph *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(ResearchGraph_VBlank, wk, 0);
}

static void ResearchGraph_ResetVBlank(ResearchGraph *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static u8 ResearchGraph_ClampU8(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return value;
}

// Asks the beacon communication to end, so that it starts again
static void ResearchGraph_RequestCommRestart(ResearchGraph *wk) {
    GameCommSys *comm = GSYS_GetGameCommSystem(ResearchGraph_GetGameSystem(wk));

    func_0202be14(comm, TRUE);
    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_UNK2) {
        GameCommSys_ExitReq(comm);
    }
}

// Starts the communication again once it has ended, and returns whether it has
static BOOL ResearchGraph_RestartComm(ResearchGraph *wk) {
    GameSystem *gsys = ResearchGraph_GetGameSystem(wk);

    switch (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
    case GAME_COMM_NO_NULL:
        GSYS_TryBootGameComm(gsys);
        return TRUE;
    default:
        return TRUE;
    case GAME_COMM_NO_UNK2:
        return FALSE;
    }
}

static void ResearchGraph_EndComm(ResearchGraph *wk) {
    func_0202be14(GSYS_GetGameCommSystem(ResearchGraph_GetGameSystem(wk)), FALSE);
}
