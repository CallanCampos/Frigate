#include "types.h"
#include "app/pkwd_record_monitor.h"
#include "app/pkwd_record_monitor_graphic.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/pokewood.h"
#include "system/app_menu_common.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/pokewood_lines.h"

// Pokéstar Studios' record monitor: the list of the movies the player has filmed on the lower screen and, on the
// upper one, the box office record of the movie under the cursor, its cast and its stars. The file has no name
// string; it is named after the overlay's other file, pkwd_record_monitor_graphic.c (a guess)

// The archive of the screen's graphics, which pkwd_selectscreen.c takes its marks from
#define ARCID_PKWD_RECORD_MONITOR_GRA 100

// The archive's file of the actors' positions, an x and a y each
#define POSITIONS_FILE 43
#define POSITION_COUNT 33
// The first of the positions of the list's marks
#define POSITION_LIST_MARK 17

#define MOVIE_COUNT 41
#define CAST_COUNT 6
#define STAR_COUNT 3
#define WINDOW_MAX 13
#define LOAD_MAX 8
#define LIST_ROWS 7
#define LIST_ROW_HEIGHT 24

#define MOVIE_NONE (-1)
#define MODE_END (-1)

#define SCORE_MAX 9999
#define SCORE_MIN (-9998)

// The groups of cell actors
enum {
    // The return button and the arrows of the list
    ACTOR_GROUP_BUTTON,
    ACTOR_GROUP_ICON,
    // The scroll bar and the marks of the list's rows
    ACTOR_GROUP_LIST,
    ACTOR_GROUP_MARK,
};

enum {
    BUTTON_RETURN,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_COUNT,
};

enum {
    LIST_ACTOR_BAR,
    // The marks of the rows shown, and of the row before them and the one after them
    LIST_ACTOR_MARK,
    LIST_MARK_COUNT = LIST_ROWS + 2,
    LIST_ACTOR_COUNT = LIST_ACTOR_MARK + LIST_MARK_COUNT,
};

enum {
    // The two halves of the mark of the highest grossing movie
    MARK_BEST,
    MARK_CAST = 2,
    // The three that cover the upper screen while the movie's details are put up
    MARK_COVER = MARK_CAST + CAST_COUNT,
    MARK_STAR = 11,
    MARK_COUNT = MARK_STAR + STAR_COUNT,
};

// The resources of the cell actors
enum {
    RES_COMMON_PALETTE,
    RES_COMMON_CHARS,
    RES_COMMON_CELL,
    RES_ICON_PALETTE,
    RES_ICON_CHARS,
    RES_ICON_CELL = RES_ICON_CHARS + CAST_COUNT,
    RES_MARK_PALETTE,
    RES_MARK_CHARS,
    RES_MARK_CELL,
    RES_COUNT,
};

// The screens that the windows are on
enum {
    SCREEN_MAIN,
    SCREEN_SUB,
};

enum {
    WINDOW_HEADING,
    WINDOW_GROSS_LABEL,
    WINDOW_CAST_LABEL,
    WINDOW_TITLE,
    WINDOW_TAGLINE,
    WINDOW_GROSS,
    WINDOW_PLAYER,
};

// The messages of the upper screen
enum {
    MOVIE_MESSAGE_TITLE,
    MOVIE_MESSAGE_TAGLINE_BOY,
    MOVIE_MESSAGE_TAGLINE_GIRL,
    MOVIE_MESSAGE_GROSS,
    MOVIE_MESSAGE_PLAYER,
    MOVIE_MESSAGE_COUNT,
};

enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
};

// The states of the screen
enum {
    STATE_START = 0,
    STATE_WAIT_FADE_IN = 10,
    STATE_RUN = 100,
    STATE_END = 10000,
};

enum {
    MODE_INIT,
    MODE_MAIN,
    MODE_EXIT,
    MODE_COUNT,
};

// The states of the list
enum {
    LIST_STATE_DRAW = 0,
    LIST_STATE_OPEN_DETAILS = 1,
    LIST_STATE_WAIT_COVER = 2,
    LIST_STATE_CHECK_KEYS = 10,
    LIST_STATE_INPUT = 100,
    LIST_STATE_WAIT_RETURN = 200,
    LIST_STATE_WAIT_DOWN = 210,
    LIST_STATE_WAIT_UP = 220,
    LIST_STATE_WAIT_LIST = 300,
    LIST_STATE_WAIT_ARROW = 305,
    LIST_STATE_WAIT_PRINT = 310,
    LIST_STATE_END = 10000,
};

// What the list's result makes the screen do
enum {
    ACTION_NONE,
    ACTION_MOVE,
    ACTION_DECIDE,
    ACTION_DECIDE_TOUCH,
};

#define FLAG_FADED_IN 0x1
#define FLAG_LIST_READY 0x2
// Moves the BG of the upper screen's frame to the page once the text is printed
#define FLAG_MOVE_BG 0x4
#define FLAG_PAGE_UP 0x10
#define FLAG_PAGE_DOWN 0x20
#define FLAG_CURSOR_MOVED 0x40
#define FLAG_REDRAW 0x100
#define FLAG_LIST_DRAWN 0x1000
#define FLAG_CURSOR_SHOWN 0x2000
#define FLAG_DECIDED 0x10000
#define FLAG_DETAILS_OPEN 0x20000
#define FLAG_SHOW_DETAILS 0x40000

// What the list holds of a movie, after a bit for each of its stars
#define ENTRY_NEW 0x8
#define ENTRY_NEW_RECORD 0x10
#define ENTRY_SEEN 0x20

typedef struct {
    u8 movie;
    u8 series;
    u8 flags;
    u8 padding;
} PkwdRecordMonitorEntry;

// The resources of the cell actors, which the tables give by number
typedef union {
    u32 all[RES_COUNT];
    struct {
        u32 commonPalette;
        u32 commonChars;
        u32 commonCell;
        u32 iconPalette;
        u32 iconChars[CAST_COUNT];
        u32 iconCell;
        u32 markPalette;
        u32 markChars;
        u32 markCell;
    } of;
} PkwdRecordMonitorRes;

typedef struct {
    u16 heapId;
    PkwdRecordMonitorRes res;
    ClActor *buttons[BUTTON_COUNT];
    ClActor *icons[CAST_COUNT];
    ClActor *listActors[LIST_ACTOR_COUNT];
    ClActor *marks[MARK_COUNT];
    PkwdRecordMonitorGraphic *graphic;
    PkwdRecordMonitorParam *param;
    Font *font;
    PrintQueue *printQueue;
    PrintWindow subWindows[WINDOW_MAX];
    PrintWindow windows[WINDOW_MAX];
    MsgData *msgData;
    WordSet *wordSet;
    FrameList *list;
    s32 state;
    u32 mode;
    s32 listState;
    u32 page;
    s32 nextMode;
    u32 flags;
    // A bit for each window whose screen has been sent
    u32 subWindowsShown;
    u32 windowsShown;
    s32 selected;
    s32 count;
    s32 cursor;
    s32 scroll;
    // Only cleared
    u32 unk1DC;
    // The list's scroll that the marks of its rows were last put for
    s32 markScroll;
    s16 *positions;
    PkwdRecordMonitorEntry entries[POKEWOOD_MOVIE_COUNT];
    // Whether each movie is the highest grossing one
    u8 best[POKEWOOD_MOVIE_COUNT];
    // Whether the keys have been used, which puts up the details of the movie under the cursor
    BOOL keyMode;
} PkwdRecordMonitorWork;

// A file of the BGs: where it goes, the file, and as in pkwd_count.c the offset and the size
typedef struct {
    u8 target;
    u8 fileId;
    u8 offset;
    u8 size;
} PkwdRecordMonitorLoad;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} PkwdRecordMonitorWindowSetup;

// A message of a page's labels and the window it goes in
typedef struct {
    u16 message;
    u16 window;
} PkwdRecordMonitorLabel;

typedef struct {
    s16 x;
    s16 y;
} PkwdRecordMonitorBGPos;

// A file of the cell actors' resources and where the work keeps it
typedef struct {
    s16 fileId;
    s16 res;
    u16 offset;
    u16 count;
} PkwdRecordMonitorResFile;

typedef struct {
    PkwdRecordMonitorResFile palette;
    PkwdRecordMonitorResFile chars;
    PkwdRecordMonitorResFile cell;
    PkwdRecordMonitorResFile anim;
} PkwdRecordMonitorResSetup;

#define ACTOR_ANIMATE 0x1
#define ACTOR_FLIP_V 0x2
#define ACTOR_FLIP_H 0x4

#define ACTOR_END 0xff

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unk4;
    u8 flags;
    u8 position;
    u8 charsRes;
} PkwdRecordMonitorActorSetup;

// A group of cell actors: its resources, how many it has and which of the work's groups it is
typedef struct {
    s16 paletteRes;
    s16 charsRes;
    s16 cellRes;
    s16 count;
    s16 group;
} PkwdRecordMonitorActorGroup;

typedef void (*PkwdRecordMonitorModeFunc)(PkwdRecordMonitorWork *wk);

static BOOL PkwdRecordMonitor_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdRecordMonitor_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdRecordMonitor_Main(GameProc *proc, u32 *state, void *param, void *work);
static void PkwdRecordMonitor_Setup(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_Update(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_Cleanup(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_RunState(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_RunStateCore(PkwdRecordMonitorWork *wk);
static s16 PkwdRecordMonitor_GetPosition(PkwdRecordMonitorWork *wk, u32 index);
static void PkwdRecordMonitor_LoadBG(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_LoadTouchBarBG(PkwdRecordMonitorWork *wk, u32 bg, u32 palette);
static void PkwdRecordMonitor_LoadCommonRes(PkwdRecordMonitorWork *wk, u32 vramType);
static void PkwdRecordMonitor_LoadIconRes(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_SetIcon(PkwdRecordMonitorWork *wk, u32 index, u32 species, u32 sex, const u32 form);
static void PkwdRecordMonitor_LoadRes(PkwdRecordMonitorWork *wk, u32 vramType, const PkwdRecordMonitorResSetup *setup);
static void PkwdRecordMonitor_CreateActors(PkwdRecordMonitorWork *wk, u32 surface,
                                           const PkwdRecordMonitorActorSetup *setup,
                                           const PkwdRecordMonitorActorGroup *group);
static void PkwdRecordMonitor_InitActors(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_FreeActors(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_SetActorSequence(PkwdRecordMonitorWork *wk, u32 group, s32 index, u32 sequence);
static void PkwdRecordMonitor_SetActorVisible(PkwdRecordMonitorWork *wk, u32 group, s32 index, BOOL visible);
static BOOL PkwdRecordMonitor_IsActorAnimating(PkwdRecordMonitorWork *wk, u32 group, s32 index);
static void PkwdRecordMonitor_CreateWindows(PkwdRecordMonitorWork *wk, u32 screen, u32 page);
static void PkwdRecordMonitor_FreeWindows(PkwdRecordMonitorWork *wk, u32 screen);
static void PkwdRecordMonitor_FlushWindows(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_Print(PkwdRecordMonitorWork *wk, u32 screen, u32 window, s32 x, s32 y, StrBuf *strbuf,
                                    u32 align, u16 color);
static u16 PkwdRecordMonitor_GetAlignedX(PkwdRecordMonitorWork *wk, StrBuf *strbuf, Font *font, u32 align, u32 window);
static void PkwdRecordMonitor_FillWindow(PkwdRecordMonitorWork *wk, u32 screen, u32 window, u32 color);
static void PkwdRecordMonitor_SetBGPos(PkwdRecordMonitorWork *wk, u32 page);
static void PkwdRecordMonitor_PrintLabels(PkwdRecordMonitorWork *wk, u32 screen, u32 page);
static void PkwdRecordMonitor_UpdateBGPos(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_InitMode(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_MainMode(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_ExitMode(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_Input(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_PrintMovie(PkwdRecordMonitorWork *wk, BOOL movieOnly);
static void PkwdRecordMonitor_SetArrows(PkwdRecordMonitorWork *wk, s32 first, s32 last, s32 selected);
static void PkwdRecordMonitor_ShowMovie(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_HideActors(PkwdRecordMonitorWork *wk, u32 unused);
static BOOL PkwdRecordMonitor_RunMode(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_ChangeMode(PkwdRecordMonitorWork *wk);
static s32 PkwdRecordMonitor_ReadTouch(PkwdRecordMonitorWork *wk, u32 page);
static void PkwdRecordMonitor_LoadMovies(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_MarkRecordsSeen(PkwdRecordMonitorWork *wk);
static s32 PkwdRecordMonitor_GetMovie(PkwdRecordMonitorWork *wk, s32 index);
static void PkwdRecordMonitor_ShowDetails(PkwdRecordMonitorWork *wk, BOOL show);
static BOOL PkwdRecordMonitor_CheckKeyMode(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_CreateList(PkwdRecordMonitorWork *wk);
static u32 PkwdRecordMonitor_UpdateList(PkwdRecordMonitorWork *wk);
static u32 PkwdRecordMonitor_HandleListResult(PkwdRecordMonitorWork *wk, u32 result);
static void PkwdRecordMonitor_FreeList(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_ClearList(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_StoreListPos(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_PrintListItem(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PkwdRecordMonitor_OnListSelect(void *work, u32 index, BOOL moved);
static void PkwdRecordMonitor_OnListScroll(void *work, s16 delta);
static void PkwdRecordMonitor_PrintItem(PkwdRecordMonitorWork *wk, u32 index, PrintWindow *window, MsgData *msgData);
static void PkwdRecordMonitor_PrintCentered(PkwdRecordMonitorWork *wk, PrintWindow *window, PrintQueue *queue,
                                            MsgData *msgData, u32 message, u16 color, s32 y);
static void PkwdRecordMonitor_PrintTitle(PkwdRecordMonitorWork *wk, PrintWindow *window, PrintQueue *queue,
                                         MsgData *msgData, u32 message);
static void PkwdRecordMonitor_ShowScrollBar(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_UpdateScrollBar(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_UpdateListMarks(PkwdRecordMonitorWork *wk, BOOL reset);
static void PkwdRecordMonitor_MoveListMarks(PkwdRecordMonitorWork *wk, s16 delta);
static void PkwdRecordMonitor_SetStar(PkwdRecordMonitorWork *wk, BOOL visible, s32 sequence, s32 star);
static void PkwdRecordMonitor_HideDetails(PkwdRecordMonitorWork *wk);
static void PkwdRecordMonitor_Dummy(PkwdRecordMonitorWork *wk);

static const s16 sPkwdRecordMonitorLabelMessages[] = {
    PokewoodLines_Text_HighestGrossingFilm,
    PokewoodLines_Text_RecordBoxOfficeGross,
    PokewoodLines_Text_Featuring_2,
    PokewoodLines_Text_Empty_3,
};

static const PkwdRecordMonitorActorGroup sPkwdRecordMonitorButtonGroup = {
    RES_COMMON_PALETTE, RES_COMMON_CHARS, RES_COMMON_CELL, BUTTON_COUNT, ACTOR_GROUP_BUTTON,
};

static const PkwdRecordMonitorActorGroup sPkwdRecordMonitorListGroup = {
    RES_MARK_PALETTE, RES_MARK_CHARS, RES_MARK_CELL, LIST_ACTOR_COUNT, ACTOR_GROUP_LIST,
};

// The messages of the upper screen: the first of the titles, of the taglines for a boy and for a girl, the gross and
// the player's name. The reads are folded, so the table is only in the ROM's data, as in pkwd_selectscreen.c
static const u16 sPkwdRecordMonitorMovieMessages[MOVIE_MESSAGE_COUNT] = {
    PokewoodLines_Text_BrycenMan, PokewoodLines_Text_KidVsBrycenMan, PokewoodLines_Text_GirlVsBrycenMan,
    PokewoodLines_Text_Billion,   PokewoodLines_Text_Empty_13,
};

static const PkwdRecordMonitorActorGroup sPkwdRecordMonitorIconGroup = {
    RES_ICON_PALETTE, 0, RES_ICON_CELL, CAST_COUNT, ACTOR_GROUP_ICON,
};

static const PkwdRecordMonitorActorGroup sPkwdRecordMonitorMarkGroup = {
    RES_MARK_PALETTE, RES_MARK_CHARS, RES_MARK_CELL, MARK_COUNT, ACTOR_GROUP_MARK,
};

// The return button
static const TouchRect sPkwdRecordMonitorTouchRects[] = {
    { 168, 184, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// Where the BG of the upper screen's frame is for each page
static const PkwdRecordMonitorBGPos sPkwdRecordMonitorBGPos[] = {
    { 0, 192 },
    { 256, 0 },
    { 0, 0 },
};

static const PkwdRecordMonitorLoad sPkwdRecordMonitorChars[] = {
    { 1, 22, 0, 0 },
    { 7, 24, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const PkwdRecordMonitorLoad sPkwdRecordMonitorPalettes[] = {
    { 0, 3, 0, 0 },
    { 4, 5, 6, 2 },
    { 0xff, 0, 0, 0 },
};

static const PkwdRecordMonitorWindowSetup sPkwdRecordMonitorSubWindows[] = {
    { 4, 2, 1, 28, 4, 10 },
    { 0xff, 0, 0, 0, 0, 0 },
};

// The windows of the upper screen's pages
static const PkwdRecordMonitorWindowSetup *sPkwdRecordMonitorSubWindowPages[] = { sPkwdRecordMonitorSubWindows };

static const TouchRect *sPkwdRecordMonitorTouchRectPages[] = { sPkwdRecordMonitorTouchRects };

static const FrameListCallbacks sPkwdRecordMonitorListCallbacks = {
    PkwdRecordMonitor_PrintListItem,
    PkwdRecordMonitor_OnListSelect,
    PkwdRecordMonitor_OnListScroll,
};

static const PkwdRecordMonitorLabel sPkwdRecordMonitorLabels[] = {
    { 0, WINDOW_HEADING },
    { 1, WINDOW_GROSS_LABEL },
    { 2, WINDOW_CAST_LABEL },
    { 0xffff, 0 },
};

static const PkwdRecordMonitorLabel *sPkwdRecordMonitorLabelPages[] = { sPkwdRecordMonitorLabels };

static PkwdRecordMonitorModeFunc sPkwdRecordMonitorModes[MODE_COUNT] = {
    PkwdRecordMonitor_InitMode,
    PkwdRecordMonitor_MainMode,
    PkwdRecordMonitor_ExitMode,
};

static const PkwdRecordMonitorLoad sPkwdRecordMonitorScreens[] = {
    { 1, 33, 0, 0 }, { 2, 34, 0, 0 }, { 3, 35, 0, 0 }, { 7, 37, 0, 0 }, { 0xff, 0, 0, 0 },
};

static const PkwdRecordMonitorActorSetup sPkwdRecordMonitorButtonActors[] = {
    { 1, 48, 1, 1, 0, ACTOR_ANIMATE, 3, RES_COMMON_CHARS },
    { 4, 48, 1, 1, 0, ACTOR_ANIMATE, 0, RES_COMMON_CHARS },
    { 5, 48, 1, 1, 0, ACTOR_ANIMATE, 1, RES_COMMON_CHARS },
    { ACTOR_END, 0, 0, 0, 0, 0, 0, 0 },
};

static const PkwdRecordMonitorResSetup sPkwdRecordMonitorMarkRes = {
    { 2, RES_MARK_PALETTE, 0x100, 6 },
    { 21, RES_MARK_CHARS, 0, 0 },
    { 27, RES_MARK_CELL, 0, 0 },
    { 39, -1, 0, 0 },
};

static const u8 sPkwdRecordMonitorSeries[MOVIE_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
};

static const PkwdRecordMonitorWindowSetup sPkwdRecordMonitorWindows[] = {
    { 0, 2, 1, 27, 2, 1 },  { 0, 2, 11, 28, 2, 2 },  { 0, 2, 17, 18, 2, 3 },   { 0, 3, 4, 27, 2, 10 },
    { 0, 3, 6, 27, 4, 10 }, { 0, 9, 14, 21, 2, 10 }, { 0, 20, 17, 10, 2, 10 }, { 0xff, 0, 0, 0, 0, 0 },
};

static const PkwdRecordMonitorActorSetup sPkwdRecordMonitorIconActors[] = {
    { 1, 40, 1, 0, 0, 0, 4, RES_ICON_CHARS + 0 }, { 1, 40, 1, 0, 0, 0, 5, RES_ICON_CHARS + 1 },
    { 1, 40, 1, 0, 0, 0, 6, RES_ICON_CHARS + 2 }, { 1, 40, 1, 0, 0, 0, 7, RES_ICON_CHARS + 3 },
    { 1, 40, 1, 0, 0, 0, 8, RES_ICON_CHARS + 4 }, { 1, 40, 1, 0, 0, 0, 9, RES_ICON_CHARS + 5 },
    { ACTOR_END, 0, 0, 0, 0, 0, 0, 0 },
};

const GameProcFunctions PKWD_RECORD_MONITOR_PROC_FUNCTIONS = {
    PkwdRecordMonitor_Init,
    PkwdRecordMonitor_Main,
    PkwdRecordMonitor_Exit,
};

// The list's rows and buttons when it has no more movies than rows, and so no scroll bar
static const FrameListTouch sPkwdRecordMonitorListTouchShort[] = {
    { { 0, 23, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },    { { 24, 47, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 48, 71, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },   { { 72, 95, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 96, 119, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },  { { 120, 143, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 144, 167, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE }, { { 168, 192, 136, 160 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 192, 168, 192 }, FRAMELIST_TOUCH_PAGE_DOWN },  { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

static const FrameListTouch sPkwdRecordMonitorListTouch[] = {
    { { 0, 23, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 24, 47, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 48, 71, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 72, 95, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 96, 119, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 120, 143, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 144, 167, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 4, 164, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 192, 136, 160 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 192, 168, 192 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

// The list, which is given its callbacks, the work and the movies when it is created
static const FrameListSetup sPkwdRecordMonitorListSetup = {
    { 6, 0xff, 2, 0, 28, LIST_ROW_HEIGHT / 8, 2, 0, 26, 3, 0, { 24, 12, 8, 6, 4, 3 }, 1, 8, 0 },
    LIST_ROWS,
    4,
    0,
    LIST_ROWS,
    0,
    sPkwdRecordMonitorListTouch,
    NULL,
    NULL,
};

static const PkwdRecordMonitorActorSetup sPkwdRecordMonitorListActors[] = {
    { 0, 48, 1, 0, 0, ACTOR_ANIMATE, 16, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 17, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 18, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 19, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 20, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 21, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 22, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 23, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 24, RES_MARK_CHARS },
    { 1, 48, 2, 0, 0, ACTOR_ANIMATE, 25, RES_MARK_CHARS },
    { ACTOR_END, 0, 0, 0, 0, 0, 0, 0 },
};

static const PkwdRecordMonitorActorSetup sPkwdRecordMonitorMarkActors[] = {
    { 4, 48, 1, 0, 0, ACTOR_ANIMATE, 26, RES_MARK_CHARS },
    { 5, 48, 0, 0, 0, ACTOR_ANIMATE, 26, RES_MARK_CHARS },
    { 6, 48, 0, 0, 0, ACTOR_ANIMATE, 10, RES_MARK_CHARS },
    { 6, 48, 0, 0, 0, ACTOR_ANIMATE, 11, RES_MARK_CHARS },
    { 6, 48, 0, 0, 0, ACTOR_ANIMATE, 12, RES_MARK_CHARS },
    { 6, 48, 0, 0, 0, ACTOR_ANIMATE, 13, RES_MARK_CHARS },
    { 6, 48, 0, 0, 0, ACTOR_ANIMATE, 14, RES_MARK_CHARS },
    { 6, 48, 0, 0, 0, ACTOR_ANIMATE, 15, RES_MARK_CHARS },
    { 7, 48, 2, 0, 0, 0, 27, RES_MARK_CHARS },
    { 8, 48, 2, 0, 0, 0, 28, RES_MARK_CHARS },
    { 9, 48, 2, 0, 0, 0, 29, RES_MARK_CHARS },
    { 10, 48, 0, 0, 0, ACTOR_ANIMATE, 30, RES_MARK_CHARS },
    { 11, 48, 0, 0, 0, ACTOR_ANIMATE, 31, RES_MARK_CHARS },
    { 12, 48, 0, 0, 0, ACTOR_ANIMATE, 32, RES_MARK_CHARS },
    { ACTOR_END, 0, 0, 0, 0, 0, 0, 0 },
};

static BOOL PkwdRecordMonitor_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdRecordMonitorWork *wk;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PKWD_RECORD_MONITOR, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PkwdRecordMonitorWork), HEAPID_PKWD_RECORD_MONITOR);
    sys_memset(wk, 0, sizeof(PkwdRecordMonitorWork));
    wk->heapId = HEAPID_PKWD_RECORD_MONITOR;
    wk->param = param;
    wk->graphic = PkwdRecordMonitorGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    PkwdRecordMonitor_Setup(wk);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    return TRUE;
}

static BOOL PkwdRecordMonitor_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdRecordMonitorWork *wk = work;
    HeapID heapId;

    PkwdRecordMonitor_Cleanup(wk);
    GFL_WordSetSystemFree(wk->wordSet);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    PkwdRecordMonitorGraphic_Free(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL PkwdRecordMonitor_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdRecordMonitorWork *wk = work;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
            wk->flags |= FLAG_FADED_IN;
        }
        break;
    case 1:
        if (wk->state == STATE_END) {
            (*state)++;
        }
        break;
    case 2:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    PkwdRecordMonitor_Update(wk);
    PkwdRecordMonitor_FlushWindows(wk);
    PkwdRecordMonitorGraphic_Update(wk->graphic);
    PkwdRecordMonitorGraphic_Begin3D(wk->graphic);
    PkwdRecordMonitorGraphic_End3D(wk->graphic);
    return FALSE;
}

static void PkwdRecordMonitor_Setup(PkwdRecordMonitorWork *wk) {
    s32 i;
    ArcTool *arc;

    wk->state = STATE_START;
    wk->mode = MODE_INIT;
    wk->listState = LIST_STATE_DRAW;
    wk->page = 0;
    wk->nextMode = MODE_INIT;
    wk->flags = 0;
    wk->subWindowsShown = 0;
    wk->windowsShown = 0;
    wk->selected = 0;
    wk->count = 20;
    wk->cursor = 0;
    wk->scroll = 0;
    wk->unk1DC = 0;
    wk->markScroll = -1;
    wk->list = NULL;
    wk->keyMode = FALSE;
    for (i = 0; i < POKEWOOD_MOVIE_COUNT; i++) {
        wk->entries[i].movie = 0xff;
        wk->entries[i].series = 0;
        wk->entries[i].flags = 0;
        wk->best[i] = FALSE;
    }
    for (i = 0; i < WINDOW_MAX; i++) {
        wk->subWindows[i].window = NULL;
        wk->windows[i].window = NULL;
    }
    wk->positions = GFL_ArcSysReadHeapNew(ARCID_PKWD_RECORD_MONITOR_GRA, POSITIONS_FILE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEWOOD_LINES, wk->heapId);
    PkwdRecordMonitor_LoadMovies(wk);
    PkwdRecordMonitor_GetMovie(wk, wk->selected);
    PkwdRecordMonitor_LoadBG(wk);
    PkwdRecordMonitor_LoadTouchBarBG(wk, 5, 8);
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 4, 10 * 0x20, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_RECORD_MONITOR_GRA, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 10 * 0x20, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
    PkwdRecordMonitor_InitActors(wk);
    PkwdRecordMonitor_CreateWindows(wk, SCREEN_MAIN, 0);
    PkwdRecordMonitor_CreateWindows(wk, SCREEN_SUB, 0);
    PkwdRecordMonitor_PrintLabels(wk, SCREEN_MAIN, 0);
    PkwdRecordMonitor_PrintMovie(wk, FALSE);
    PkwdRecordMonitor_CreateList(wk);
    PkwdRecordMonitor_SetArrows(wk, 0, wk->count - 1, wk->selected);
    PkwdRecordMonitor_SetBGPos(wk, 0);
    GFL_BGSysSetBGEnabled(0, FALSE);
}

static void PkwdRecordMonitor_Update(PkwdRecordMonitorWork *wk) {
    PkwdRecordMonitor_RunState(wk);
    PkwdRecordMonitor_Dummy(wk);
}

static void PkwdRecordMonitor_Cleanup(PkwdRecordMonitorWork *wk) {
    PkwdRecordMonitor_MarkRecordsSeen(wk);
    GFL_HeapFree(wk->positions);
    PkwdRecordMonitor_FreeWindows(wk, SCREEN_MAIN);
    PkwdRecordMonitor_FreeWindows(wk, SCREEN_SUB);
    if (wk->msgData != NULL) {
        GFL_MsgDataFree(wk->msgData);
        wk->msgData = NULL;
    }
    PkwdRecordMonitor_FreeList(wk);
    PkwdRecordMonitor_FreeActors(wk);
}

static void PkwdRecordMonitor_RunState(PkwdRecordMonitorWork *wk) {
    PkwdRecordMonitor_RunStateCore(wk);
}

static void PkwdRecordMonitor_RunStateCore(PkwdRecordMonitorWork *wk) {
    switch (wk->state) {
    case STATE_START:
        wk->state = STATE_WAIT_FADE_IN;
        break;
    case STATE_WAIT_FADE_IN:
        if (wk->flags & FLAG_FADED_IN) {
            wk->state = STATE_RUN;
        }
        break;
    case STATE_RUN:
        if (!PkwdRecordMonitor_RunMode(wk)) {
            wk->state = STATE_END;
        }
        break;
    case STATE_END:
        break;
    }
}

// A coordinate from the file of the actors' positions
static s16 PkwdRecordMonitor_GetPosition(PkwdRecordMonitorWork *wk, u32 index) {
    if (index >= POSITION_COUNT * 2) {
        return 0;
    }
    return wk->positions[index];
}

static void PkwdRecordMonitor_LoadBG(PkwdRecordMonitorWork *wk) {
    const PkwdRecordMonitorLoad *palette = sPkwdRecordMonitorPalettes;
    const PkwdRecordMonitorLoad *chars = sPkwdRecordMonitorChars;
    const PkwdRecordMonitorLoad *screen = sPkwdRecordMonitorScreens;
    u16 *buffer;
    s32 i;
    s32 page;
    s32 j;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_RECORD_MONITOR_GRA, wk->heapId);

    for (i = 0; i < LOAD_MAX; i++) {
        if (palette->target == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, palette->fileId, palette->target, palette->offset * 0x20, palette->size * 0x20,
                                    wk->heapId);
        palette++;
    }
    for (i = 0; i < LOAD_MAX; i++) {
        if (chars->target == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->fileId, chars->target, chars->offset, 0, FALSE, wk->heapId);
        chars++;
    }
    for (i = 0; i < LOAD_MAX; i++) {
        if (screen->target == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screen->fileId, screen->target, screen->offset, 0, FALSE,
                                               wk->heapId);
        screen++;
    }
    GFL_ArcToolFree(arc);
    // Gives the three pages of the upper screen's frame their palette
    buffer = GFL_BGSysIsScrHeapExists(7);
    for (page = 0; page < 3; page++) {
        for (i = 0; i < 32; i++) {
            for (j = 0; j < 32; j++) {
                buffer[page * 0x400 + i * 32 + j] += 6 << 12;
            }
        }
    }
    GFL_BGSysLoadScr(7);
}

// Loads the touch bar of the lower screen and gives it its palette
static void PkwdRecordMonitor_LoadTouchBarBG(PkwdRecordMonitorWork *wk, u32 bg, u32 palette) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    u16 *buffer;
    u32 tiles;
    s32 x;
    s32 y;

    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), 4, palette * 0x20, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), bg, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), bg, 0, 0, FALSE, wk->heapId);
    buffer = GFL_BGSysIsScrHeapExists(bg);
    tiles = (u16)(palette << 12);
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 32; x++) {
            buffer[y * 32 + x] += tiles;
        }
    }
    GFL_BGSysLoadScr(bg);
    GFL_ArcToolFree(arc);
}

// Loads the touch bar's buttons
static void PkwdRecordMonitor_LoadCommonRes(PkwdRecordMonitorWork *wk, u32 vramType) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    u32 paletteFile;
    u32 charsFile;
    u32 cellFile;
    u32 animFile;

    PkwdRecordMonitorGraphic_GetClActUnit(wk->graphic);
    paletteFile = func_0202d810();
    charsFile = func_0202d814();
    cellFile = func_0202d818(2);
    animFile = func_0202d81c(2);
    wk->res.of.commonPalette = func_0204bbb8(arc, paletteFile, vramType, 4 * 0x20, 0, 4, wk->heapId);
    wk->res.of.commonChars = func_0204b81c(arc, charsFile, FALSE, vramType, wk->heapId);
    wk->res.of.commonCell = func_0204bde0(arc, cellFile, animFile, wk->heapId);
    GFL_ArcToolFree(arc);
}

// Loads what the icons of the cast share, and a first icon for each to have its characters replaced
static void PkwdRecordMonitor_LoadIconRes(PkwdRecordMonitorWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(wk->heapId));
    u32 i;

    wk->res.of.iconPalette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 5 * 0x20, wk->heapId);
    wk->res.of.iconCell = func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), wk->heapId);
    for (i = 0; i < CAST_COUNT; i++) {
        wk->res.of.iconChars[i] =
            func_0204b81c(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, CLACT_VRAM_MAIN, wk->heapId);
    }
    GFL_ArcToolFree(arc);
}

// Gives an icon of the cast the characters and the palette of a Pokémon
static void PkwdRecordMonitor_SetIcon(PkwdRecordMonitorWork *wk, u32 index, u32 species, u32 sex, const u32 form) {
    u32 palette = PokeIcon_GetPalette(species, form, sex, FALSE);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(wk->heapId));
    NNSG2dCharacterData *character;
    void *buffer = GFL_G2DIOReadOBJNCGRArc(arc, PokeParty_GetIconIndex(species, form, sex, FALSE), FALSE, &character,
                                           HEAPID_TAIL(wk->heapId));

    func_0204ba40(wk->res.of.iconChars[index], character);
    GFL_HeapFree(buffer);
    GFL_ArcToolFree(arc);
    func_0204c378(wk->icons[index], palette, 1);
}

static void PkwdRecordMonitor_LoadRes(PkwdRecordMonitorWork *wk, u32 vramType, const PkwdRecordMonitorResSetup *setup) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_RECORD_MONITOR_GRA, wk->heapId);

    PkwdRecordMonitorGraphic_GetClActUnit(wk->graphic);
    wk->res.all[setup->palette.res] =
        func_0204bbb8(arc, setup->palette.fileId, vramType, setup->palette.offset, 0, setup->palette.count, wk->heapId);
    wk->res.all[setup->chars.res] = func_0204b81c(arc, setup->chars.fileId, FALSE, vramType, wk->heapId);
    wk->res.all[setup->cell.res] = func_0204bde0(arc, setup->cell.fileId, setup->anim.fileId, wk->heapId);
    GFL_ArcToolFree(arc);
}

// Creates a group's actors at their positions from the file
static void PkwdRecordMonitor_CreateActors(PkwdRecordMonitorWork *wk, u32 surface,
                                           const PkwdRecordMonitorActorSetup *setup,
                                           const PkwdRecordMonitorActorGroup *group) {
    s32 i;
    ClActUnit *unit = PkwdRecordMonitorGraphic_GetClActUnit(wk->graphic);
    ClActorSetup actorSetup;
    ClActor **actors;
    s16 paletteRes;
    s16 cellRes;
    s16 count;
    s16 kind;
    u32 position;
    s16 x;
    s16 y;
    BOOL flag;

    sys_memset(&actorSetup, 0, sizeof(ClActorSetup));
    paletteRes = group->paletteRes;
    cellRes = group->cellRes;
    count = group->count;
    kind = group->group;
    if (kind == ACTOR_GROUP_BUTTON) {
        actors = wk->buttons;
    } else if (kind == ACTOR_GROUP_ICON) {
        actors = wk->icons;
    } else if (kind == ACTOR_GROUP_LIST) {
        actors = wk->listActors;
    } else if (kind == ACTOR_GROUP_MARK) {
        actors = wk->marks;
    }
    for (i = 0; i < count; i++) {
        if (setup->sequence == ACTOR_END) {
            break;
        }
        position = setup->position * 2;
        x = PkwdRecordMonitor_GetPosition(wk, position);
        y = PkwdRecordMonitor_GetPosition(wk, position + 1);
        actorSetup.x = x;
        actorSetup.y = y;
        actorSetup.sequence = setup->sequence;
        actorSetup.priority = setup->priority;
        actorSetup.bgPriority = setup->bgPriority;
        actors[i] = func_0204c040(unit, wk->res.all[setup->charsRes], wk->res.all[paletteRes], wk->res.all[cellRes],
                                  &actorSetup, surface, wk->heapId);
        flag = FALSE;
        if (setup->visible == TRUE) {
            flag = TRUE;
        }
        func_0204c124(actors[i], flag);
        flag = FALSE;
        if (setup->flags & ACTOR_ANIMATE) {
            flag = TRUE;
        }
        func_0204c520(actors[i], flag);
        if (setup->flags & ACTOR_FLIP_V) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (setup->flags & ACTOR_FLIP_H) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        setup++;
    }
}

static void PkwdRecordMonitor_InitActors(PkwdRecordMonitorWork *wk) {
    PkwdRecordMonitorGraphic_GetClActUnit(wk->graphic);
    PkwdRecordMonitor_LoadCommonRes(wk, CLACT_VRAM_SUB);
    PkwdRecordMonitor_LoadIconRes(wk);
    PkwdRecordMonitor_LoadRes(wk, CLACT_VRAM_BOTH, &sPkwdRecordMonitorMarkRes);
    PkwdRecordMonitor_CreateActors(wk, 1, sPkwdRecordMonitorButtonActors, &sPkwdRecordMonitorButtonGroup);
    PkwdRecordMonitor_CreateActors(wk, 0, sPkwdRecordMonitorIconActors, &sPkwdRecordMonitorIconGroup);
    PkwdRecordMonitor_CreateActors(wk, 1, sPkwdRecordMonitorListActors, &sPkwdRecordMonitorListGroup);
    PkwdRecordMonitor_CreateActors(wk, 0, sPkwdRecordMonitorMarkActors, &sPkwdRecordMonitorMarkGroup);
    func_02042ba8(TRUE, wk->heapId);
    func_0204c318(wk->marks[MARK_COVER + 0], 1);
    func_0204c318(wk->marks[MARK_COVER + 1], 1);
    func_0204c318(wk->marks[MARK_COVER + 2], 1);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3,
                        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                            GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                        8, 8);
}

static void PkwdRecordMonitor_FreeActors(PkwdRecordMonitorWork *wk) {
    s32 i;

    for (i = 0; i < BUTTON_COUNT; i++) {
        func_0204c108(wk->buttons[i]);
    }
    for (i = 0; i < CAST_COUNT; i++) {
        func_0204c108(wk->icons[i]);
    }
    for (i = 0; i < LIST_ACTOR_COUNT; i++) {
        func_0204c108(wk->listActors[i]);
    }
    for (i = 0; i < MARK_COUNT; i++) {
        func_0204c108(wk->marks[i]);
    }
    func_0204bcd0(wk->res.of.commonPalette);
    func_0204b98c(wk->res.of.commonChars);
    func_0204be64(wk->res.of.commonCell);
    func_0204bcd0(wk->res.of.iconPalette);
    for (i = 0; i < CAST_COUNT; i++) {
        func_0204b98c(wk->res.of.iconChars[i]);
    }
    func_0204be64(wk->res.of.iconCell);
    func_0204bcd0(wk->res.of.markPalette);
    func_0204b98c(wk->res.of.markChars);
    func_0204be64(wk->res.of.markCell);
}

// Plays an animation of an actor
static void PkwdRecordMonitor_SetActorSequence(PkwdRecordMonitorWork *wk, u32 group, s32 index, u32 sequence) {
    ClActor **actors;

    if (group == ACTOR_GROUP_BUTTON) {
        if (index < BUTTON_COUNT) {
            actors = wk->buttons;
            func_0204c488(actors[index], sequence);
            func_0204c520(actors[index], TRUE);
        }
    } else if (group == ACTOR_GROUP_ICON) {
        if (index < CAST_COUNT) {
            actors = wk->icons;
            func_0204c488(actors[index], sequence);
            func_0204c520(actors[index], TRUE);
        }
    } else if (group == ACTOR_GROUP_LIST) {
        if (index < LIST_ACTOR_COUNT) {
            actors = wk->listActors;
            func_0204c488(actors[index], sequence);
            func_0204c520(actors[index], TRUE);
        }
    } else if (group == ACTOR_GROUP_MARK) {
        if (index < MARK_COUNT) {
            actors = wk->marks;
            func_0204c488(actors[index], sequence);
            func_0204c520(actors[index], TRUE);
            func_0204c56c(actors[index]);
        }
    }
}

static void PkwdRecordMonitor_SetActorVisible(PkwdRecordMonitorWork *wk, u32 group, s32 index, BOOL visible) {
    if (group == ACTOR_GROUP_BUTTON) {
        if (index < BUTTON_COUNT) {
            func_0204c124(wk->buttons[index], visible);
        }
    } else if (group == ACTOR_GROUP_ICON) {
        if (index < CAST_COUNT) {
            func_0204c124(wk->icons[index], visible);
        }
    } else if (group == ACTOR_GROUP_LIST) {
        if (index < LIST_ACTOR_COUNT) {
            func_0204c124(wk->listActors[index], visible);
        }
    } else if (group == ACTOR_GROUP_MARK) {
        if (index < MARK_COUNT) {
            func_0204c124(wk->marks[index], visible);
        }
    }
}

static BOOL PkwdRecordMonitor_IsActorAnimating(PkwdRecordMonitorWork *wk, u32 group, s32 index) {
    BOOL animating = TRUE;

    if (group == ACTOR_GROUP_BUTTON) {
        if (index >= BUTTON_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->buttons[index])) {
            animating = FALSE;
        }
    } else if (group == ACTOR_GROUP_ICON) {
        if (index >= CAST_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->icons[index])) {
            animating = FALSE;
        }
    } else if (group == ACTOR_GROUP_LIST) {
        if (index >= LIST_ACTOR_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->listActors[index])) {
            animating = FALSE;
        }
    } else if (group == ACTOR_GROUP_MARK) {
        if (index >= MARK_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->marks[index])) {
            animating = FALSE;
        }
    }
    return animating;
}

// Creates the windows of a screen, for the upper one those of a page
static void PkwdRecordMonitor_CreateWindows(PkwdRecordMonitorWork *wk, u32 screen, u32 page) {
    s32 i;
    const PkwdRecordMonitorWindowSetup *setup;

    if (screen == SCREEN_MAIN) {
        setup = sPkwdRecordMonitorWindows;
        wk->windowsShown = 0;
    } else {
        setup = sPkwdRecordMonitorSubWindowPages[page];
        wk->subWindowsShown = 0;
        PkwdRecordMonitor_FreeWindows(wk, SCREEN_SUB);
    }
    for (i = 0; i < WINDOW_MAX; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        if (screen == SCREEN_MAIN) {
            wk->windows[i].window =
                BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, 1);
        } else {
            wk->subWindows[i].window =
                BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, 1);
        }
        PkwdRecordMonitor_FillWindow(wk, screen, i, 0);
        setup++;
    }
}

static void PkwdRecordMonitor_FreeWindows(PkwdRecordMonitorWork *wk, u32 screen) {
    s32 i;

    if (screen == SCREEN_MAIN) {
        for (i = 0; i < WINDOW_MAX; i++) {
            if (wk->windows[i].window != NULL) {
                GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i].window), 0);
                BmpWin_ClearScreen(wk->windows[i].window);
                BmpWin_Free(wk->windows[i].window);
                wk->windows[i].window = NULL;
            }
        }
    } else {
        for (i = 0; i < WINDOW_MAX; i++) {
            if (wk->subWindows[i].window != NULL) {
                GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindows[i].window), 0);
                BmpWin_ClearScreen(wk->subWindows[i].window);
                BmpWin_Free(wk->subWindows[i].window);
                wk->subWindows[i].window = NULL;
            }
        }
    }
}

// Sends the windows whose text is printed to VRAM, and their screens the first time
static void PkwdRecordMonitor_FlushWindows(PkwdRecordMonitorWork *wk) {
    s32 i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < WINDOW_MAX; i++) {
        if (wk->subWindows[i].window != NULL) {
            PrintWindow_Flush(&wk->subWindows[i], wk->printQueue);
            if (PrintWindow_IsPrinted(&wk->subWindows[i]) == TRUE) {
                if (!(wk->subWindowsShown & (1 << i))) {
                    BmpWin_FlushMap(wk->subWindows[i].window);
                    GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->subWindows[i].window));
                    wk->subWindowsShown |= 1 << i;
                }
            }
        }
        if (wk->windows[i].window != NULL) {
            PrintWindow_Flush(&wk->windows[i], wk->printQueue);
            if (PrintWindow_IsPrinted(&wk->windows[i]) == TRUE) {
                if (!(wk->windowsShown & (1 << i))) {
                    BmpWin_FlushMap(wk->windows[i].window);
                    GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[i].window));
                    wk->windowsShown |= 1 << i;
                }
            }
        }
    }
    PkwdRecordMonitor_UpdateBGPos(wk);
}

// Prints a string into a window of a screen, aligned in it
static void PkwdRecordMonitor_Print(PkwdRecordMonitorWork *wk, u32 screen, u32 window, s32 x, s32 y, StrBuf *strbuf,
                                    u32 align, u16 color) {
    u16 offset;

    PkwdRecordMonitor_FillWindow(wk, screen, window, 0);
    offset = PkwdRecordMonitor_GetAlignedX(wk, strbuf, wk->font, align, window);
    if (screen == SCREEN_MAIN) {
        PrintWindow_Print(&wk->windows[window], wk->printQueue, x + offset, y, strbuf, wk->font, color);
    } else {
        PrintWindow_Print(&wk->subWindows[window], wk->printQueue, x + offset, y, strbuf, wk->font, color);
    }
}

// Where a string starts in a window for an alignment. It measures the lower screen's window for either screen, and
// the upper screen's strings are never aligned
static u16 PkwdRecordMonitor_GetAlignedX(PkwdRecordMonitorWork *wk, StrBuf *strbuf, Font *font, u32 align, u32 window) {
    u32 spacing = 0;
    u32 x = 0;

    if (align == ALIGN_RIGHT) {
        x = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[window].window));
        x -= GFL_FontGetBlockWidth(strbuf, font, spacing);
    } else if (align == ALIGN_CENTER) {
        x = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[window].window)) / 2;
        x -= GFL_FontGetBlockWidth(strbuf, font, spacing) / 2;
    }
    return x;
}

static void PkwdRecordMonitor_FillWindow(PkwdRecordMonitorWork *wk, u32 screen, u32 window, u32 color) {
    if (screen == SCREEN_MAIN) {
        if (wk->windows[window].window != NULL) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[window].window), color);
            wk->windows[window].flushPending = TRUE;
        }
    } else {
        if (wk->subWindows[window].window != NULL) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindows[window].window), color);
            wk->subWindows[window].flushPending = TRUE;
        }
    }
}

// Moves the BG of the upper screen's frame to a page
static void PkwdRecordMonitor_SetBGPos(PkwdRecordMonitorWork *wk, u32 page) {
    GFL_BGSysMoveBG(7, BG_MOVE_SET_X, sPkwdRecordMonitorBGPos[page].x);
    GFL_BGSysMoveBG(7, BG_MOVE_SET_Y, sPkwdRecordMonitorBGPos[page].y);
}

// Prints the labels of a screen's page
static void PkwdRecordMonitor_PrintLabels(PkwdRecordMonitorWork *wk, u32 screen, u32 page) {
    const PkwdRecordMonitorLabel *labels = sPkwdRecordMonitorLabelPages[screen + page];
    s32 i;

    for (i = 0; i < WINDOW_MAX; i++) {
        u16 message;
        u16 window;
        StrBuf *strbuf;

        if (labels[i].message == 0xffff) {
            break;
        }
        message = labels[i].message;
        window = labels[i].window;
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, sPkwdRecordMonitorLabelMessages[message]);
        if (message == 0) {
            PkwdRecordMonitor_Print(wk, screen, window, 0, 0, strbuf, ALIGN_CENTER, PRINT_COLOR(1, 2, 0));
        } else {
            PkwdRecordMonitor_Print(wk, screen, window, 0, 0, strbuf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0));
        }
        GFL_StrBufFree(strbuf);
    }
}

static void PkwdRecordMonitor_UpdateBGPos(PkwdRecordMonitorWork *wk) {
    if (wk->flags & FLAG_MOVE_BG) {
        if (func_02021c0c(wk->printQueue) == TRUE) {
            PkwdRecordMonitor_SetBGPos(wk, wk->page);
            wk->flags &= ~FLAG_MOVE_BG;
        }
    }
}

static void PkwdRecordMonitor_InitMode(PkwdRecordMonitorWork *wk) {
    wk->mode = MODE_MAIN;
    wk->page = 0;
    PkwdRecordMonitor_HideActors(wk, 0);
    PkwdRecordMonitor_ShowScrollBar(wk);
    PkwdRecordMonitor_UpdateListMarks(wk, TRUE);
    PkwdRecordMonitor_MoveListMarks(wk, 0);
    wk->flags |= FLAG_LIST_READY;
    wk->flags |= FLAG_CURSOR_MOVED;
}

static void PkwdRecordMonitor_MainMode(PkwdRecordMonitorWork *wk) {
    switch (wk->listState) {
    case LIST_STATE_DRAW:
        if (!(wk->flags & FLAG_LIST_DRAWN)) {
            if (!FrameList_Draw(wk->list)) {
                wk->listState = LIST_STATE_CHECK_KEYS;
                wk->flags |= FLAG_LIST_DRAWN;
            }
        } else {
            wk->listState = LIST_STATE_CHECK_KEYS;
        }
        break;
    case LIST_STATE_CHECK_KEYS:
        if (PkwdRecordMonitor_CheckKeyMode(wk) == TRUE) {
            wk->listState = LIST_STATE_OPEN_DETAILS;
        } else {
            wk->listState = LIST_STATE_INPUT;
        }
        break;
    case LIST_STATE_OPEN_DETAILS:
        GFL_BGSysSetBGEnabled(0, FALSE);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(2, FALSE);
        GFL_BGSysSetBGEnabled(3, FALSE);
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_COVER + 0, TRUE);
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_COVER + 1, TRUE);
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_COVER + 2, TRUE);
        PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_MARK, MARK_COVER + 0, 7);
        PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_MARK, MARK_COVER + 1, 8);
        PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_MARK, MARK_COVER + 2, 9);
        wk->listState = LIST_STATE_WAIT_COVER;
        break;
    case LIST_STATE_WAIT_COVER:
        if (!PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_MARK, MARK_COVER + 0) &&
            !PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_MARK, MARK_COVER + 1) &&
            !PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_MARK, MARK_COVER + 2)) {
            GFL_BGSysSetBGEnabled(1, FALSE);
            GFL_BGSysSetBGEnabled(2, TRUE);
            GFL_BGSysSetBGEnabled(3, TRUE);
            PkwdRecordMonitor_PrintMovie(wk, TRUE);
            PkwdRecordMonitor_ShowMovie(wk);
            wk->entries[wk->selected].flags |= ENTRY_SEEN;
            wk->listState = LIST_STATE_WAIT_PRINT;
            wk->flags |= FLAG_SHOW_DETAILS;
            wk->flags |= FLAG_DETAILS_OPEN;
        }
        break;
    case LIST_STATE_INPUT:
        PkwdRecordMonitor_Input(wk);
        break;
    case LIST_STATE_WAIT_RETURN:
        if (!PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_BUTTON, BUTTON_RETURN)) {
            wk->mode++;
            wk->nextMode = MODE_END;
        }
        break;
    case LIST_STATE_WAIT_DOWN:
        if (!PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_BUTTON, BUTTON_DOWN)) {
            wk->listState = LIST_STATE_INPUT;
            wk->mode++;
        }
        break;
    case LIST_STATE_WAIT_UP:
        if (!PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_BUTTON, BUTTON_UP)) {
            wk->listState = LIST_STATE_INPUT;
            wk->mode++;
        }
        break;
    case LIST_STATE_WAIT_LIST:
        if (PkwdRecordMonitor_UpdateList(wk) == FRAMELIST_NONE) {
            PkwdRecordMonitor_StoreListPos(wk);
            wk->flags |= FLAG_CURSOR_MOVED;
            wk->listState = LIST_STATE_WAIT_ARROW;
        }
        break;
    case LIST_STATE_WAIT_ARROW:
        if (wk->flags & FLAG_PAGE_UP) {
            if (!PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_BUTTON, BUTTON_UP)) {
                wk->listState = LIST_STATE_INPUT;
                wk->flags &= ~(FLAG_PAGE_UP | FLAG_PAGE_DOWN);
                PkwdRecordMonitor_SetArrows(wk, 0, wk->count - 1, wk->selected);
            }
        } else if (wk->flags & FLAG_PAGE_DOWN) {
            if (!PkwdRecordMonitor_IsActorAnimating(wk, ACTOR_GROUP_BUTTON, BUTTON_DOWN)) {
                wk->listState = LIST_STATE_INPUT;
                wk->flags &= ~(FLAG_PAGE_UP | FLAG_PAGE_DOWN);
                PkwdRecordMonitor_SetArrows(wk, 0, wk->count - 1, wk->selected);
            }
        } else {
            wk->listState = LIST_STATE_INPUT;
            PkwdRecordMonitor_SetArrows(wk, 0, wk->count - 1, wk->selected);
        }
        break;
    case LIST_STATE_WAIT_PRINT:
        if (func_02021c0c(wk->printQueue) == TRUE) {
            if (wk->flags & FLAG_SHOW_DETAILS) {
                PkwdRecordMonitor_ShowDetails(wk, TRUE);
                wk->flags &= ~FLAG_SHOW_DETAILS;
            }
            wk->listState = LIST_STATE_INPUT;
        }
        break;
    case LIST_STATE_END:
        break;
    }
}

static void PkwdRecordMonitor_ExitMode(PkwdRecordMonitorWork *wk) {
    PkwdRecordMonitor_ChangeMode(wk);
    PkwdRecordMonitor_ClearList(wk);
}

// Runs the list and the return button
static void PkwdRecordMonitor_Input(PkwdRecordMonitorWork *wk) {
    u32 redraw = 0;
    BOOL cancel = FALSE;
    u32 input = 0;
    u32 action;
    s32 touch;

    if (!func_02021c0c(wk->printQueue)) {
        return;
    }
    action = PkwdRecordMonitor_HandleListResult(wk, PkwdRecordMonitor_UpdateList(wk));
    if (action == ACTION_MOVE) {
        redraw = 2;
        input = 1;
    } else if (action == ACTION_DECIDE || action == ACTION_DECIDE_TOUCH) {
        redraw = 1;
        input = 2;
    }
    if (wk->flags & FLAG_CURSOR_MOVED) {
        redraw = 1;
        wk->flags &= ~FLAG_CURSOR_MOVED;
    }
    if (redraw == 0) {
        touch = PkwdRecordMonitor_ReadTouch(wk, wk->page);
        if (touch == 0) {
            cancel = TRUE;
        }
        if (cancel == FALSE) {
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                cancel = TRUE;
            }
        }
        if (cancel == TRUE) {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_RETURN, 9);
            wk->listState = LIST_STATE_WAIT_RETURN;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            if (touch == 0) {
                func_0203d564(TRUE);
            } else {
                func_0203d564(FALSE);
            }
        }
    }
    if (redraw == 1) {
        wk->flags |= FLAG_REDRAW;
    }
    if (!wk->keyMode) {
        if (GCTX_HIDGetPressedKeys() != 0 || input == 2) {
            wk->listState = LIST_STATE_OPEN_DETAILS;
            wk->keyMode = TRUE;
        }
    }
    if (wk->flags & FLAG_REDRAW) {
        if (wk->flags & FLAG_DETAILS_OPEN) {
            BOOL show = FALSE;

            if (!func_0203d554()) {
                show = TRUE;
            }
            if (wk->flags & FLAG_DECIDED) {
                show = TRUE;
            }
            if (show == TRUE) {
                PkwdRecordMonitor_PrintMovie(wk, TRUE);
                PkwdRecordMonitor_ShowMovie(wk);
                wk->entries[wk->selected].flags |= ENTRY_SEEN;
                wk->listState = LIST_STATE_WAIT_PRINT;
                wk->flags |= FLAG_SHOW_DETAILS;
                wk->flags &= ~FLAG_DECIDED;
            }
        }
        wk->flags &= ~FLAG_REDRAW;
    }
    if (!(wk->flags & FLAG_CURSOR_SHOWN)) {
        FrameList_ShowCursor(wk->list);
        wk->flags |= FLAG_CURSOR_SHOWN;
    }
    PkwdRecordMonitor_UpdateScrollBar(wk);
}

// Prints the title, the tagline and the record of the movie under the cursor, and with them the player's name
static void PkwdRecordMonitor_PrintMovie(PkwdRecordMonitorWork *wk, BOOL movieOnly) {
    s32 movie = PkwdRecordMonitor_GetMovie(wk, wk->selected);
    PokewoodRecord *record;
    StrBuf *strbuf;
    StrBuf *format;
    s32 score;
    s32 tenths;
    u32 message;
    u32 species;
    u32 form;
    u32 sex;

    if (!movieOnly) {
        format = GFL_MsgDataLoadStrbufNew(wk->msgData, sPkwdRecordMonitorMovieMessages[MOVIE_MESSAGE_PLAYER]);
        strbuf = GFL_StrBufCreate(8, wk->heapId);
        copyVarForText(wk->wordSet, 1, wk->param->playerInfo);
        GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
        GFL_StrBufFree(format);
        PkwdRecordMonitor_Print(wk, SCREEN_MAIN, WINDOW_PLAYER, 0, 0, strbuf, ALIGN_RIGHT, PRINT_COLOR(13, 10, 11));
        GFL_StrBufFree(strbuf);
    }
    record = func_02011194(wk->param->save, movie);
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, movie + sPkwdRecordMonitorMovieMessages[MOVIE_MESSAGE_TITLE]);
    PkwdRecordMonitor_Print(wk, SCREEN_MAIN, WINDOW_TITLE, 0, 0, strbuf, ALIGN_LEFT, PRINT_COLOR(13, 10, 11));
    GFL_StrBufFree(strbuf);
    if (getTrainerGender(wk->param->playerInfo) == 0) {
        message = movie + sPkwdRecordMonitorMovieMessages[MOVIE_MESSAGE_TAGLINE_BOY];
    } else {
        message = movie + sPkwdRecordMonitorMovieMessages[MOVIE_MESSAGE_TAGLINE_GIRL];
    }
    strbuf = GFL_StrBufCreate(0x80, wk->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, message);
    PokeIcon_UnpackSpecies(record->cast[0], &species, &form, &sex);
    WordSet_LoadSpeciesName(wk->wordSet, 0, (u16)species);
    copyVarForText(wk->wordSet, 1, wk->param->playerInfo);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    GFL_StrBufFree(format);
    PkwdRecordMonitor_Print(wk, SCREEN_MAIN, WINDOW_TAGLINE, 0, 0, strbuf, ALIGN_LEFT, PRINT_COLOR(13, 10, 11));
    GFL_StrBufFree(strbuf);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, sPkwdRecordMonitorMovieMessages[MOVIE_MESSAGE_GROSS]);
    strbuf = GFL_StrBufCreate(0x80, wk->heapId);
    score = record->score;
    if (score > SCORE_MAX) {
        score = SCORE_MAX;
    } else if (score < SCORE_MIN) {
        score = SCORE_MIN;
    }
    // The gross in tenths of a billion: its whole part, its tenths, and all of it
    WordSetNumber(wk->wordSet, 3, score / 10, 3, 0, TRUE);
    if (score >= 0) {
        tenths = score % 10;
    } else {
        tenths = -score % 10;
    }
    WordSetNumber(wk->wordSet, 4, tenths, 1, 2, TRUE);
    WordSetNumber(wk->wordSet, 2, score, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    GFL_StrBufFree(format);
    PkwdRecordMonitor_Print(wk, SCREEN_MAIN, WINDOW_GROSS, 0, 0, strbuf, ALIGN_RIGHT, PRINT_COLOR(13, 10, 11));
    GFL_StrBufFree(strbuf);
}

// Lights the arrows of the list that have movies to go to
static void PkwdRecordMonitor_SetArrows(PkwdRecordMonitorWork *wk, s32 first, s32 last, s32 selected) {
    if (last >= 0) {
        if (first >= selected) {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_UP, 18);
        } else {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_UP, 4);
        }
        if (selected >= last) {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_DOWN, 19);
        } else {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_DOWN, 5);
        }
    }
}

// Shows the cast of the movie under the cursor, with the marks of its famous Pokémon, and the movie's marks
static void PkwdRecordMonitor_ShowMovie(PkwdRecordMonitorWork *wk) {
    s32 movie = PkwdRecordMonitor_GetMovie(wk, wk->selected);
    PokewoodRecord *record = func_02011194(wk->param->save, movie);
    s32 i;
    u32 species;
    u32 form;
    u32 sex;

    for (i = 0; i < CAST_COUNT; i++) {
        PokeIcon_UnpackSpecies(record->cast[i], &species, &form, &sex);
        if (species != 0) {
            PkwdRecordMonitor_SetIcon(wk, i, species, sex, form);
            PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_ICON, i, TRUE);
            if (record->famousCast & (1 << i)) {
                PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_CAST + i, TRUE);
            } else {
                PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_CAST + i, FALSE);
            }
        } else {
            PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_ICON, i, FALSE);
            PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_CAST + i, FALSE);
        }
    }
    if (wk->best[movie] == TRUE) {
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_BEST + 0, TRUE);
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_BEST + 1, TRUE);
    } else {
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_BEST + 0, FALSE);
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_BEST + 1, FALSE);
    }
    for (i = 0; i < STAR_COUNT; i++) {
        BOOL visible = FALSE;

        if (wk->entries[wk->selected].flags & (1 << i)) {
            visible = TRUE;
        }
        PkwdRecordMonitor_SetStar(wk, visible, i, i);
    }
}

static void PkwdRecordMonitor_HideActors(PkwdRecordMonitorWork *wk, u32 unused) {
    s32 i;

    for (i = 0; i < CAST_COUNT; i++) {
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_ICON, i, FALSE);
    }
    for (i = 0; i < LIST_ACTOR_COUNT; i++) {
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_LIST, i, FALSE);
    }
    for (i = 0; i < MARK_COUNT; i++) {
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, i, FALSE);
    }
}

// Runs the mode, and gives whether the screen goes on
static BOOL PkwdRecordMonitor_RunMode(PkwdRecordMonitorWork *wk) {
    BOOL running = TRUE;

    if (wk->mode >= MODE_COUNT) {
        return FALSE;
    }
    sPkwdRecordMonitorModes[wk->mode](wk);
    if (wk->nextMode == MODE_END) {
        running = FALSE;
    }
    return running;
}

static void PkwdRecordMonitor_ChangeMode(PkwdRecordMonitorWork *wk) {
    if (wk->nextMode != MODE_END) {
        wk->mode = wk->nextMode;
    }
}

static s32 PkwdRecordMonitor_ReadTouch(PkwdRecordMonitorWork *wk, u32 page) {
    return func_0203da0c(sPkwdRecordMonitorTouchRectPages[page]);
}

// Lists the movies that have been filmed, and finds the highest grossing ones
static void PkwdRecordMonitor_LoadMovies(PkwdRecordMonitorWork *wk) {
    u16 best[POKEWOOD_MOVIE_COUNT];
    s32 count;
    u16 bestScore = 0;
    u8 bestCount = 0;
    s32 movie;
    s32 i;

    count = 0;

    for (movie = 0; movie < POKEWOOD_MOVIE_COUNT; movie++) {
        if (func_020110ac(wk->param->save, 5, movie) == TRUE) {
            PokewoodRecord *record = func_02011194(wk->param->save, movie);

            wk->entries[count].movie = movie;
            if (!record->hasRecord) {
                wk->entries[count].flags |= ENTRY_NEW;
            }
            if (record->isNewRecord == TRUE) {
                wk->entries[count].flags |= ENTRY_NEW_RECORD;
            }
            if (record->score >= bestScore) {
                if (record->score > bestScore) {
                    bestCount = 0;
                }
                bestScore = record->score;
                best[bestCount] = movie;
                bestCount++;
            }
            if (func_020110ac(wk->param->save, 2, movie) == TRUE) {
                wk->entries[count].flags |= 1 << 0;
            }
            if (func_020110ac(wk->param->save, 6, movie) == TRUE) {
                wk->entries[count].flags |= 1 << 1;
            }
            if (func_020110ac(wk->param->save, 4, movie) == TRUE) {
                wk->entries[count].flags |= 1 << 2;
            }
            wk->entries[count].series = sPkwdRecordMonitorSeries[movie] & 3;
            count++;
        }
    }
    wk->count = count;
    for (i = 0; i < bestCount; i++) {
        wk->best[best[i]] = TRUE;
    }
}

// Marks the records of the movies that were looked at as seen
static void PkwdRecordMonitor_MarkRecordsSeen(PkwdRecordMonitorWork *wk) {
    s32 i;

    for (i = 0; i < wk->count; i++) {
        if (wk->entries[i].flags & ENTRY_SEEN) {
            func_0201117c(wk->param->save, wk->entries[i].movie);
        }
    }
}

// The movie of an item of the list
static s32 PkwdRecordMonitor_GetMovie(PkwdRecordMonitorWork *wk, s32 index) {
    if (index >= wk->count) {
        return MOVIE_NONE;
    }
    return wk->entries[index].movie;
}

// Shows the text of the movie's details, or hides it and the actors when the cursor moves in touch mode
static void PkwdRecordMonitor_ShowDetails(PkwdRecordMonitorWork *wk, BOOL show) {
    u8 enabled = TRUE;

    if (!show) {
        if (func_0203d554() == TRUE) {
            enabled = FALSE;
        }
        GFL_BGSysSetBGEnabled(0, enabled);
    } else {
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    if (!enabled) {
        PkwdRecordMonitor_HideDetails(wk);
    }
}

// Whether the screen starts with the keys in use, which it only checks once
static BOOL PkwdRecordMonitor_CheckKeyMode(PkwdRecordMonitorWork *wk) {
    BOOL keyMode = FALSE;

    if (wk->keyMode == TRUE) {
        return FALSE;
    }
    if (!func_0203d554()) {
        keyMode = TRUE;
        wk->keyMode = TRUE;
    }
    return keyMode;
}

static void PkwdRecordMonitor_CreateList(PkwdRecordMonitorWork *wk) {
    FrameListSetup setup = sPkwdRecordMonitorListSetup;
    ArcTool *arc;
    s32 i;

    setup.work = wk;
    PkwdRecordMonitor_FreeList(wk);
    arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_RECORD_MONITOR_GRA, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 20, 6, 0, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 0x20, wk->heapId);
    setup.callbacks = &sPkwdRecordMonitorListCallbacks;
    setup.count = wk->count;
    setup.cursorPos = wk->cursor;
    setup.scroll = wk->scroll;
    if (wk->count <= LIST_ROWS) {
        setup.touch = sPkwdRecordMonitorListTouchShort;
    }
    wk->list = FrameList_Create(&setup, wk->heapId);
    FrameList_LoadScreen(wk->list, arc, 29, FALSE, 0);
    FrameList_LoadScreen(wk->list, arc, 30, FALSE, 1);
    FrameList_LoadScreen(wk->list, arc, 31, FALSE, 2);
    FrameList_LoadScreen(wk->list, arc, 32, FALSE, 3);
    FrameList_LoadCursorPalette(wk->list, arc, 0, 0, 1);
    GFL_ArcToolFree(arc);
    // Each movie is drawn in the frame of its series and holds the message of its title
    for (i = 0; i < setup.count; i++) {
        FrameList_AddItem(wk->list, wk->entries[i].series, wk->entries[i].movie + PokewoodLines_Text_BrycenMan);
    }
    wk->flags &= ~FLAG_LIST_DRAWN;
    if (!FrameList_Draw(wk->list)) {
        wk->flags |= FLAG_LIST_DRAWN;
    }
    GFL_BGSysResetStdPalette(4, GX_RGB(29, 28, 25));
}

static u32 PkwdRecordMonitor_UpdateList(PkwdRecordMonitorWork *wk) {
    u32 result = FRAMELIST_NONE;

    if (wk->list != NULL) {
        result = FrameList_Main(wk->list);
    }
    return result;
}

// Acts on what the list did, and gives the ACTION_* of it
static u32 PkwdRecordMonitor_HandleListResult(PkwdRecordMonitorWork *wk, u32 result) {
    s16 action = ACTION_NONE;
    BOOL jumped = FALSE;

    switch (result) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        action = ACTION_DECIDE;
        break;
    case -7:
    case -5:
        wk->flags |= FLAG_PAGE_UP;
        action = ACTION_MOVE;
        break;
    case -6:
    case -4:
        wk->flags |= FLAG_PAGE_DOWN;
        action = ACTION_MOVE;
        break;
    case -3:
    case -2:
        action = ACTION_MOVE;
        jumped = TRUE;
        break;
    case -11:
    case -10:
    case -9:
        action = ACTION_MOVE;
        break;
    case -12:
        action = ACTION_DECIDE_TOUCH;
        break;
    }
    if (action == ACTION_MOVE) {
        wk->listState = LIST_STATE_WAIT_LIST;
        if (wk->flags & FLAG_PAGE_UP) {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_UP, 12);
        } else if (wk->flags & FLAG_PAGE_DOWN) {
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_BUTTON, BUTTON_DOWN, 13);
        }
        if (jumped == TRUE) {
            FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
            PkwdRecordMonitor_UpdateListMarks(wk, TRUE);
        }
    } else if (action == ACTION_DECIDE || action == ACTION_DECIDE_TOUCH) {
        PkwdRecordMonitor_StoreListPos(wk);
        if (func_0203d554() == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        FrameList_SetRowPalette(wk->list, (u16)wk->cursor, 1);
        if (action == ACTION_DECIDE) {
            func_0203d564(FALSE);
        }
        if (wk->keyMode == TRUE) {
            wk->flags |= FLAG_DECIDED;
        }
        PkwdRecordMonitor_SetArrows(wk, 0, wk->count - 1, wk->selected);
    }
    if (action == ACTION_MOVE) {
        PkwdRecordMonitor_ShowDetails(wk, FALSE);
    }
    return action;
}

static void PkwdRecordMonitor_FreeList(PkwdRecordMonitorWork *wk) {
    if (wk->list != NULL) {
        FrameList_Free(wk->list);
        wk->list = NULL;
    }
}

static void PkwdRecordMonitor_ClearList(PkwdRecordMonitorWork *wk) {
    GFL_BGSysClearScr(6);
    GFL_BGSysQueueScrLoad(6);
    PkwdRecordMonitor_StoreListPos(wk);
}

static void PkwdRecordMonitor_StoreListPos(PkwdRecordMonitorWork *wk) {
    if (wk->list != NULL) {
        wk->selected = FrameList_GetSelected(wk->list);
        wk->cursor = FrameList_GetCursor(wk->list);
        wk->scroll = FrameList_GetScroll(wk->list);
    }
}

// The list's callback that prints an item, the title of its movie
static void PkwdRecordMonitor_PrintListItem(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PkwdRecordMonitorWork *wk = work;

    PkwdRecordMonitor_PrintItem(wk, index, window, wk->msgData);
}

static void PkwdRecordMonitor_OnListSelect(void *work, u32 index, BOOL moved) {
}

// The list's callback for its scrolling, which the marks of its rows follow
static void PkwdRecordMonitor_OnListScroll(void *work, s16 delta) {
    PkwdRecordMonitorWork *wk = work;

    PkwdRecordMonitor_UpdateListMarks(wk, FALSE);
    PkwdRecordMonitor_MoveListMarks(wk, delta);
}

static void PkwdRecordMonitor_PrintItem(PkwdRecordMonitorWork *wk, u32 index, PrintWindow *window, MsgData *msgData) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    PkwdRecordMonitor_PrintTitle(wk, window, queue, msgData, FrameList_GetValue(wk->list, index));
}

// Prints a message in the middle of a window
static void PkwdRecordMonitor_PrintCentered(PkwdRecordMonitorWork *wk, PrintWindow *window, PrintQueue *queue,
                                            MsgData *msgData, u32 message, u16 color, s32 y) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, message);
    s32 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
    s32 windowWidth = BmpWin_GetSizeX(window->window) * 8;

    PrintWindow_Print(window, queue, (windowWidth - width) / 2 + 3, y, strbuf, wk->font, color);
    GFL_StrBufFree(strbuf);
}

static void PkwdRecordMonitor_PrintTitle(PkwdRecordMonitorWork *wk, PrintWindow *window, PrintQueue *queue,
                                         MsgData *msgData, u32 message) {
    PkwdRecordMonitor_PrintCentered(wk, window, queue, msgData, message, PRINT_COLOR(15, 14, 0), 4);
}

// Shows the scroll bar when the list has more movies than rows
static void PkwdRecordMonitor_ShowScrollBar(PkwdRecordMonitorWork *wk) {
    BOOL visible = TRUE;

    if (wk->count <= LIST_ROWS) {
        visible = FALSE;
    }
    PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_LIST, LIST_ACTOR_BAR, visible);
}

static void PkwdRecordMonitor_UpdateScrollBar(PkwdRecordMonitorWork *wk) {
    ClActorPos pos;

    if (wk->count > LIST_ROWS) {
        func_0204c178(wk->listActors[LIST_ACTOR_BAR], &pos, 1);
        pos.y = FrameList_ClampBarPos(wk->list, pos.y);
        if (pos.y < 8) {
            pos.y = 8;
        } else if (pos.y > 160) {
            pos.y = 160;
        }
        func_0204c140(wk->listActors[LIST_ACTOR_BAR], &pos, 1);
    }
}

// Puts the marks of the new movies and the new records by the list's rows after it scrolled, a row off when it is
// still scrolling there
static void PkwdRecordMonitor_UpdateListMarks(PkwdRecordMonitorWork *wk, BOOL reset) {
    BOOL visible;
    u32 newBits = 0;
    u32 recordBits = 0;
    s32 scroll;
    s32 direction;
    s32 i;

    if (reset == TRUE) {
        wk->markScroll = -1;
    }
    scroll = FrameList_GetScroll(wk->list);
    if (scroll == wk->markScroll) {
        return;
    }
    if (scroll > wk->markScroll) {
        direction = 1;
    } else {
        direction = -1;
    }
    if (reset == TRUE) {
        direction = 0;
    }
    for (i = 0; i < LIST_ROWS; i++) {
        u8 flags = wk->entries[scroll + i].flags;

        if (flags & ENTRY_NEW) {
            newBits |= 1 << i;
        }
        if (flags & ENTRY_NEW_RECORD) {
            recordBits |= 1 << i;
        }
    }
    if (scroll > 0) {
        u8 flags = (wk->entries + scroll - 1)->flags;

        if (flags & ENTRY_NEW) {
            newBits |= 1 << LIST_ROWS;
        }
        if (flags & ENTRY_NEW_RECORD) {
            recordBits |= 1 << LIST_ROWS;
        }
    }
    if (scroll + LIST_ROWS < wk->count) {
        u8 flags = (wk->entries + scroll + LIST_ROWS)->flags;

        if (flags & ENTRY_NEW) {
            newBits |= 1 << (LIST_ROWS + 1);
        }
        if (flags & ENTRY_NEW_RECORD) {
            recordBits |= 1 << (LIST_ROWS + 1);
        }
    }
    for (i = 0; i < LIST_MARK_COUNT; i++) {
        visible = FALSE;
        if (newBits & (1 << i)) {
            visible = TRUE;
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_LIST, LIST_ACTOR_MARK + i, 1);
        } else if (recordBits & (1 << i)) {
            visible = TRUE;
            PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_LIST, LIST_ACTOR_MARK + i, 3);
        }
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_LIST, LIST_ACTOR_MARK + i, visible);
    }
    for (i = 0; i < LIST_MARK_COUNT; i++) {
        ClActorPos pos;

        func_0204c178(wk->listActors[LIST_ACTOR_MARK + i], &pos, 1);
        pos.x = PkwdRecordMonitor_GetPosition(wk, (POSITION_LIST_MARK + i) * 2);
        // The two indexes differ in signedness: the original computes the product twice
        pos.y = PkwdRecordMonitor_GetPosition(wk, (u32)(POSITION_LIST_MARK + i) * 2 + 1);
        pos.y += (s16)(direction * LIST_ROW_HEIGHT);
        func_0204c140(wk->listActors[LIST_ACTOR_MARK + i], &pos, 1);
    }
    wk->markScroll = scroll;
}

static void PkwdRecordMonitor_MoveListMarks(PkwdRecordMonitorWork *wk, s16 delta) {
    s32 i;

    for (i = 0; i < LIST_MARK_COUNT; i++) {
        ClActorPos pos;

        func_0204c178(wk->listActors[LIST_ACTOR_MARK + i], &pos, 1);
        pos.y -= delta;
        func_0204c140(wk->listActors[LIST_ACTOR_MARK + i], &pos, 1);
    }
}

// Shows a star of the movie in an animation, or hides it
static void PkwdRecordMonitor_SetStar(PkwdRecordMonitorWork *wk, BOOL visible, s32 sequence, s32 star) {
    PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_STAR + star, visible);
    if (visible == TRUE) {
        PkwdRecordMonitor_SetActorSequence(wk, ACTOR_GROUP_MARK, MARK_STAR + star, sequence + 10);
    }
}

static void PkwdRecordMonitor_HideDetails(PkwdRecordMonitorWork *wk) {
    s32 i;

    for (i = 0; i < CAST_COUNT; i++) {
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_ICON, i, FALSE);
        PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_CAST + i, FALSE);
    }
    PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_BEST + 0, FALSE);
    PkwdRecordMonitor_SetActorVisible(wk, ACTOR_GROUP_MARK, MARK_BEST + 1, FALSE);
    for (i = 0; i < STAR_COUNT; i++) {
        PkwdRecordMonitor_SetStar(wk, FALSE, i, i);
    }
}

static void PkwdRecordMonitor_Dummy(PkwdRecordMonitorWork *wk) {
}
