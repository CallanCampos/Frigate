#include "types.h"
#include "app/pkwd_book.h"
#include "app/pkwd_book_graphic.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/event_pokewood.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
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
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "save/player_info.h"
#include "save/pokewood.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/blink_palanm.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/event_pass_power_startmenu.h"
#include "text/system/pokewood_lines.h"

// Pokéstar Studios' scenario book: the list of the movies that the player can film on the lower screen, and the
// poster, the title, the plot and the mission of the one under the cursor on the upper screen. Picking a movie asks
// whether to film it or to see its details, which are a page for the cast and a page for each of its Pokémon. The
// ROM names only the overlay's other file, "pkwd_book_graphic.c", so the name of this one is our guess

// The archives of the screen's graphics and of the movies' data, as in pkwd_selectscreen.c and event_pokewood.c
#define ARCID_PKWD_BOOK_GRA 100
#define ARCID_POKEWOOD_MOVIE 269

#define MOVIE_COUNT 41
// The movies that the book can list
#define LIST_MOVIE_COUNT 40
#define LIST_ROWS 7
#define CAST_COUNT 6
#define MOVE_COUNT 4
#define WINDOW_COUNT 16
#define MSG_DATA_COUNT 4
#define TEXT_ACTOR_COUNT 3
#define MENU_ITEM_COUNT 3
#define LOAD_COUNT_MAX 8
#define POSITION_COUNT 34
// The first of the new marks' positions
#define POSITION_NEW_MARKS 25
#define NEW_MARK_COUNT 9

#define RESOURCE_COUNT 27
#define BAR_ACTOR_COUNT 3
#define POKE_ICON_COUNT 7
#define TYPE_ICON_COUNT 6
#define TRAINER_ACTOR_COUNT 1
#define MARK_ACTOR_COUNT 11

// The slots of the work's resources
enum {
    RES_BAR_PALETTE,
    RES_BAR_CHARS,
    RES_BAR_CELLS,
    RES_POKE_ICON_PALETTE,
    RES_POKE_ICON_CHARS,
    RES_POKE_ICON_CELLS = RES_POKE_ICON_CHARS + POKE_ICON_COUNT,
    RES_TYPE_ICON_PALETTE,
    RES_TYPE_ICON_CHARS,
    RES_TYPE_ICON_CELLS = RES_TYPE_ICON_CHARS + TYPE_ICON_COUNT,
    RES_TRAINER_PALETTE,
    RES_TRAINER_CHARS,
    RES_TRAINER_CELLS,
    RES_MARK_PALETTE,
    RES_MARK_CHARS,
    RES_MARK_CELLS,
    RES_FONT_PALETTE,
};

// The groups of cell actors
enum {
    GROUP_BAR,
    GROUP_POKE_ICON,
    GROUP_TYPE_ICON,
    GROUP_TRAINER,
    GROUP_MARK,
};

// The touch bar's buttons
enum {
    BAR_RETURN,
    BAR_LEFT,
    BAR_RIGHT,
};

// The marks: the page number's plate, the list's scroll bar, and the marks of the movies that were not filmed yet,
// one for each row of the list and for the rows above and below
enum {
    MARK_PAGE,
    MARK_SCROLL_BAR,
    MARK_NEW,
};

// The message files of the work
enum {
    MSG_LINES,
    // A movie's script, while its role name is read
    MSG_SCRIPT,
    MSG_TRAINER_CLASSES,
    MSG_COSTARS,
};

// The ways to align a text on its x
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ALIGN_CENTER 2

// The flags of the work
#define FLAG_WIPE_IN_DONE 0x1
// The list page was left, so entering it draws it again
#define FLAG_LIST_LEFT 0x2
// The lower BG scrolls to the page once its text is printed
#define FLAG_SCROLL_PENDING 0x4
#define FLAG_DETAILS 0x8
#define FLAG_PAGE_UP 0x10
#define FLAG_PAGE_DOWN 0x20
#define FLAG_MOVIE_CHANGED 0x40
#define FLAG_LIST_DRAWN 0x1000
#define FLAG_CURSOR_SHOWN 0x2000
#define FLAG_SHOW_MOVIE 0x10000

// The steps of the screen
enum {
    STEP_START = 0,
    STEP_WAIT_WIPE = 10,
    STEP_RUN = 100,
    STEP_END = 10000,
};

// The pages of the book
enum {
    PAGE_LIST,
    PAGE_CAST,
    PAGE_POKEMON,
};

// The states of the pages: each page has one to enter it, one to run it and one to leave it
enum {
    PAGE_STATE_ENTER_LIST,
    PAGE_STATE_LIST,
    PAGE_STATE_LEAVE_LIST,
    PAGE_STATE_ENTER_CAST,
    PAGE_STATE_CAST,
    PAGE_STATE_LEAVE_CAST,
    PAGE_STATE_ENTER_POKEMON,
    PAGE_STATE_POKEMON,
    PAGE_STATE_LEAVE_POKEMON,
    PAGE_STATE_COUNT,
};

#define PAGE_STATE_NONE (-1)

// The steps of a page
enum {
    PAGE_STEP_START = 0,
    PAGE_STEP_INPUT = 100,
    PAGE_STEP_WAIT_RETURN = 200,
    PAGE_STEP_WAIT_RIGHT = 210,
    PAGE_STEP_WAIT_LEFT = 220,
    PAGE_STEP_LIST_SCROLL = 300,
    PAGE_STEP_LIST_WAIT_ARROW = 305,
    PAGE_STEP_LIST_WAIT_PRINT = 310,
    PAGE_STEP_LIST_BLINK = 320,
    PAGE_STEP_LIST_MENU = 400,
    PAGE_STEP_END = 10000,
};

// The series of a movie, which gives the color of its row and of the upper screen
typedef enum {
    SERIES_0,
    SERIES_1,
    SERIES_2,
    SERIES_3,
    SERIES_COUNT,
} PkwdBookSeries;

// What the menu of a movie returns
enum {
    MENU_FILM,
    MENU_DETAILS,
    MENU_QUIT,
};

// A file to load to a palette or BG: where, which file, the offset and, for the palettes, the size
typedef struct {
    u8 target;
    u8 fileId;
    u8 offset;
    u8 extra;
} PkwdBookLoad;

// A file of the cell actors' resources and where its load goes in the work
typedef struct {
    s16 fileId;
    s16 slot;
    u16 offset;
    u16 count;
} PkwdBookActorResFile;

typedef struct {
    PkwdBookActorResFile palette;
    PkwdBookActorResFile chars;
    PkwdBookActorResFile cells;
    PkwdBookActorResFile anims;
} PkwdBookActorRes;

// The resources that a group of cell actors share, and the group that holds them
typedef struct {
    s16 paletteSlot;
    s16 charsSlot;
    s16 cellSlot;
    s16 count;
    s16 group;
} PkwdBookActorGroup;

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unk4;
    u8 flags;
    u8 positionIdx;
    u8 charsSlot;
} PkwdBookActorEntry;

// A text that is drawn as cell actors
typedef struct {
    u8 width;
    u8 height;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 paletteOffset;
    u8 positionIdx;
    u8 unk7;
} PkwdBookTextActorEntry;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} PkwdBookWindowSetup;

// A heading of a page: its message in sPkwdBookLabelMessages, its window, whether it is dark and its message file
typedef struct {
    u16 message;
    u16 window;
    u16 dark;
    u16 msgData;
} PkwdBookLabel;

// A movie's data in arc 269, of which only the trainer classes matter here: those of the side that films and of the
// one that watches, for each sex of the player
typedef struct {
    u8 unk0[0x18];
    s16 trainerClass[2][2];
} PkwdBookMovieFile;

// A movie of the list
typedef struct {
    u16 movie : 8;
    u16 series : 7;
    // Not filmed yet
    u16 isNew : 1;
} PkwdBookMovie;

// A Pokémon of a movie's cast
typedef struct {
    u32 species;
    u32 sex;
    u32 level;
    u32 item;
    u32 ability;
    u32 type1;
    u32 type2;
    u32 moves[MOVE_COUNT];
    u32 moveTypes[MOVE_COUNT];
} PkwdBookCastMember;

// The resources of the cell actors, by name and by slot
typedef union {
    struct {
        u32 barPalette;
        u32 barChars;
        u32 barCells;
        u32 pokeIconPalette;
        u32 pokeIconChars[POKE_ICON_COUNT];
        u32 pokeIconCells;
        u32 typeIconPalette;
        u32 typeIconChars[TYPE_ICON_COUNT];
        u32 typeIconCells;
        u32 trainerPalette;
        u32 trainerChars;
        u32 trainerCells;
        u32 markPalette;
        u32 markChars;
        u32 markCells;
        u32 fontPalette;
    } named;
    u32 slots[RESOURCE_COUNT];
} PkwdBookResources;

typedef struct PkwdBookWork PkwdBookWork;

struct PkwdBookWork {
    u16 heapId;
    PkwdBookResources resources;
    ClActor *barActors[BAR_ACTOR_COUNT];
    ClActor *pokeIcons[POKE_ICON_COUNT];
    ClActor *typeIcons[TYPE_ICON_COUNT];
    ClActor *trainerActors[TRAINER_ACTOR_COUNT];
    ClActor *markActors[MARK_ACTOR_COUNT];
    PkwdBookGraphic *graphic;
    PkwdBookParam *param;
    Font *font;
    Font *smallFont;
    PrintQueue *printQueue;
    // The windows of the lower screen, then those of the upper screen
    PrintWindow subWindows[WINDOW_COUNT];
    PrintWindow mainWindows[WINDOW_COUNT];
    MsgData *msgData[MSG_DATA_COUNT];
    WordSet *wordSet;
    BmpOamSys *bmpOam;
    GFLBitmap *textBitmaps[TEXT_ACTOR_COUNT];
    BmpOamActor *textActors[TEXT_ACTOR_COUNT];
    FrameList *list;
    AppTaskMenu *menu;
    AppTaskMenuItem menuItems[MENU_ITEM_COUNT];
    AppTaskMenuRes *menuRes;
    s32 step;
    u32 pageState;
    s32 pageStep;
    s32 page;
    s32 nextPageState;
    u32 flags;
    // The windows that were put on their BG, a bit each
    u32 subShown;
    u32 mainShown;
    // The list's item under the cursor, the count of its items, its cursor's row and its first row
    s32 selected;
    s32 movieCount;
    s32 cursorPos;
    s32 scroll;
    // The page of the details: 0 for the cast, then a page for each Pokémon
    s32 castPage;
    s32 result;
    s32 trainerClass;
    // The new marks that show, a bit each
    u32 newMarks;
    s32 lastScroll;
    // The positions of the actors, an x and a y each
    s16 *positions;
    s32 castCount;
    s32 castIdx;
    PokeParty *party;
    PkwdBookCastMember cast[CAST_COUNT];
    PkwdBookMovie movies[LIST_MOVIE_COUNT];
    // The name of the player's role in each movie
    StrBuf *roleNames[LIST_MOVIE_COUNT];
    // The palettes of each series of movies, for the upper screen
    void *paletteFiles[SERIES_COUNT];
    NNSG2dPaletteData *palettes[SERIES_COUNT];
    void *textPaletteFiles[SERIES_COUNT];
    NNSG2dPaletteData *textPalettes[SERIES_COUNT];
    s32 blendAlpha1;
    s32 blendAlpha2;
    u32 unk548;
};

typedef void (*PkwdBookPageFunc)(PkwdBookWork *wk);

static BOOL PkwdBook_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdBook_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdBook_Main(GameProc *proc, u32 *state, void *param, void *work);
static void PkwdBook_InitScreen(PkwdBookWork *wk);
static void PkwdBook_Step(PkwdBookWork *wk);
static void PkwdBook_FreeScreen(PkwdBookWork *wk);
static void PkwdBook_StepSequence(PkwdBookWork *wk);
static void PkwdBook_RunSequence(PkwdBookWork *wk);
static s16 PkwdBook_GetPosition(PkwdBookWork *wk, u32 index);
static void PkwdBook_LoadGraphics(PkwdBookWork *wk);
static void PkwdBook_LoadTouchBarBG(PkwdBookWork *wk, u32 bg, u32 palette);
static void PkwdBook_LoadPoster(PkwdBookWork *wk, u32 graphicIdx, BOOL deferred);
static void PkwdBook_LoadFontPalette(PkwdBookWork *wk, u32 surface);
static void PkwdBook_LoadBarRes(PkwdBookWork *wk, u32 surface);
static void PkwdBook_LoadPokeIconRes(PkwdBookWork *wk);
static void PkwdBook_SetPokeIcon(PkwdBookWork *wk, u32 slot, u32 species, u32 sex);
static void PkwdBook_LoadTypeIconRes(PkwdBookWork *wk);
static void PkwdBook_SetTypeIcon(PkwdBookWork *wk, u32 slot, u32 unused, u32 type);
static void PkwdBook_LoadTrainerRes(PkwdBookWork *wk, u32 surface, u32 trainerClass);
static void PkwdBook_SetTrainer(PkwdBookWork *wk, u32 trainerClass);
static void PkwdBook_LoadActorResources(PkwdBookWork *wk, u32 surface, const PkwdBookActorRes *res);
static void PkwdBook_CreateActors(PkwdBookWork *wk, u32 surface, const PkwdBookActorEntry *entries,
                                  const PkwdBookActorGroup *group);
static void PkwdBook_InitActors(PkwdBookWork *wk);
static void PkwdBook_FreeActors(PkwdBookWork *wk);
static void PkwdBook_PlayAnim(PkwdBookWork *wk, int group, int index, u32 sequence);
static void PkwdBook_SetActorVisible(PkwdBookWork *wk, int group, int index, BOOL visible);
static BOOL PkwdBook_IsActorAnimating(PkwdBookWork *wk, int group, int index);
static void PkwdBook_CreateTextActors(PkwdBookWork *wk, u32 surface, const PkwdBookTextActorEntry *entries,
                                      u32 paletteSlot);
static void PkwdBook_PrintTextActor(PkwdBookWork *wk, int index, const StrBuf *strbuf, s32 x, s32 y);
static void PkwdBook_SetTextActorVisible(PkwdBookWork *wk, int index, BOOL visible);
static void PkwdBook_CreateWindows(PkwdBookWork *wk, BOOL isSub, u32 page);
static void PkwdBook_FreeWindows(PkwdBookWork *wk, BOOL isSub);
static void PkwdBook_UpdateText(PkwdBookWork *wk);
static void PkwdBook_PrintText(PkwdBookWork *wk, BOOL isSub, u32 window, s32 x, s32 y, const StrBuf *strbuf, u32 align,
                               u16 color, BOOL keep);
static u16 PkwdBook_GetAlignOffset(const StrBuf *strbuf, Font *font, u32 align);
static void PkwdBook_FillWindow(PkwdBookWork *wk, BOOL isSub, u32 window, u32 color);
static void PkwdBook_ScrollBG(PkwdBookWork *wk, u32 page);
static void PkwdBook_PrintLabels(PkwdBookWork *wk, BOOL isSub, u32 page);
static void PkwdBook_UpdateScroll(PkwdBookWork *wk);
static void PkwdBook_EnterList(PkwdBookWork *wk);
static void PkwdBook_RunList(PkwdBookWork *wk);
static void PkwdBook_LeaveList(PkwdBookWork *wk);
static void PkwdBook_ListInput(PkwdBookWork *wk);
static void PkwdBook_PrintMovie(PkwdBookWork *wk, BOOL keepLabels);
static void PkwdBook_SetArrows(PkwdBookWork *wk, s32 first, s32 last, s32 current);
static void PkwdBook_EnterCast(PkwdBookWork *wk);
static void PkwdBook_RunCast(PkwdBookWork *wk);
static void PkwdBook_LeaveCast(PkwdBookWork *wk);
static void PkwdBook_CastInput(PkwdBookWork *wk);
static void PkwdBook_PrintCast(PkwdBookWork *wk, BOOL keepLabels);
static void PkwdBook_ShowCast(PkwdBookWork *wk, u32 unused);
static void PkwdBook_EnterPokemon(PkwdBookWork *wk);
static void PkwdBook_RunPokemon(PkwdBookWork *wk);
static void PkwdBook_LeavePokemon(PkwdBookWork *wk);
static void PkwdBook_PokemonInput(PkwdBookWork *wk);
static void PkwdBook_PrintPokemon(PkwdBookWork *wk, BOOL keepLabels);
static void PkwdBook_ShowPokemon(PkwdBookWork *wk, u32 unused);
static void PkwdBook_HideActors(PkwdBookWork *wk, BOOL keepText);
static BOOL PkwdBook_RunPage(PkwdBookWork *wk);
static void PkwdBook_ChangePage(PkwdBookWork *wk);
static void PkwdBook_TurnCastPage(PkwdBookWork *wk, BOOL back);
static void PkwdBook_PrintPageNumber(PkwdBookWork *wk);
static void PkwdBook_PlaceArrows(PkwdBookWork *wk, u32 page);
static s32 PkwdBook_ReadTouch(PkwdBookWork *wk, u32 page);
static void PkwdBook_LoadCast(PkwdBookWork *wk, u32 movie);
static void PkwdBook_FreeCast(PkwdBookWork *wk);
static void PkwdBook_ReadCast(PkwdBookWork *wk);
static void PkwdBook_ListMovies(PkwdBookWork *wk);
static s32 PkwdBook_GetMovie(PkwdBookWork *wk, s32 index);
static void PkwdBook_TransferSeriesPalette(PkwdBookWork *wk, u32 series);
static void PkwdBook_LoadSeriesPalettes(PkwdBookWork *wk);
static void PkwdBook_FreeSeriesPalettes(PkwdBookWork *wk);
static void PkwdBook_ShowMovie(PkwdBookWork *wk, BOOL force);
static void PkwdBook_CreateList(PkwdBookWork *wk);
static s32 PkwdBook_UpdateList(PkwdBookWork *wk);
static s32 PkwdBook_HandleListResult(PkwdBookWork *wk, u32 result);
static void PkwdBook_FreeList(PkwdBookWork *wk);
static void PkwdBook_ClearList(PkwdBookWork *wk);
static void PkwdBook_RecreateList(PkwdBookWork *wk);
static void PkwdBook_SaveListPos(PkwdBookWork *wk);
static void PkwdBookList_Print(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PkwdBookList_Select(void *work, u32 index, BOOL moved);
static void PkwdBookList_Scroll(void *work, s16 delta);
static void PkwdBook_PrintListItem(PkwdBookWork *wk, u32 index, PrintWindow *window, MsgData *msgData);
static void PkwdBook_PrintCentered(PkwdBookWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                   u32 messageId, u16 color, s32 y);
static void PkwdBook_PrintItemTitle(PkwdBookWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                    u32 messageId);
static void PkwdBook_PrintQuestion(PkwdBookWork *wk);
static void PkwdBook_ClearQuestion(PkwdBookWork *wk);
static void PkwdBook_CreateMenu(PkwdBookWork *wk);
static void PkwdBook_FreeMenu(PkwdBookWork *wk);
static s32 PkwdBook_UpdateMenu(PkwdBookWork *wk);
static void PkwdBook_LoadTrainerClass(PkwdBookWork *wk, u32 movie);
static void PkwdBook_ShowScrollBar(PkwdBookWork *wk);
static void PkwdBook_UpdateScrollBar(PkwdBookWork *wk);
static void PkwdBook_UpdateNewMarks(PkwdBookWork *wk, BOOL reset);
static void PkwdBook_ScrollNewMarks(PkwdBookWork *wk, s16 delta);
static void PkwdBook_LoadRoleNames(PkwdBookWork *wk);
static void PkwdBook_FreeRoleNames(PkwdBookWork *wk);
static u32 PkwdBook_GetPosterGraphicIdx(PkwdBookWork *wk, u32 movie);
static void PkwdBook_Unused1(PkwdBookWork *wk);
static void PkwdBook_Unused2(PkwdBookWork *wk);

// The list's setup names its touch table, which is defined after it
static const FrameListTouch sPkwdBookListTouch[11];

// The message of a script that is the name of the player's role
static const u32 sPkwdBookRoleNameMessage[1] = { 1 };

// The first movie's title, plot and mission, which the others follow
static const s16 sPkwdBookMovieMessages[3] = {
    PokewoodLines_Text_BrycenMan,
    PokewoodLines_Text_HeroBattlesMaskedMan,
    PokewoodLines_Text_KnockOutOpponentEvery,
};

// The positions of the touch bar's arrows on the list page and on the others
static const s16 sPkwdBookArrowPositions[2][2] = { { 0, 1 }, { 18, 19 } };

static const TouchRect sPkwdBookListTouchRects[2] = { { 168, 184, 232, 248 }, { TOUCH_RECT_END, 0, 0, 0 } };

// The question of the menu, then its items
static const s16 sPkwdBookMenuMessages[1 + MENU_ITEM_COUNT] = {
    PokewoodLines_Text_FilmScript,
    PokewoodLines_Text_Film,
    PokewoodLines_Text_SeeDetails,
    PokewoodLines_Text_Quit,
};

static const PkwdBookActorGroup sPkwdBookPokeIconGroup = {
    RES_POKE_ICON_PALETTE, 0, RES_POKE_ICON_CELLS, POKE_ICON_COUNT, GROUP_POKE_ICON,
};

static const PkwdBookActorGroup sPkwdBookTypeIconGroup = {
    RES_TYPE_ICON_PALETTE, 0, RES_TYPE_ICON_CELLS, TYPE_ICON_COUNT, GROUP_TYPE_ICON,
};

static const PkwdBookActorGroup sPkwdBookBarGroup = {
    RES_BAR_PALETTE, RES_BAR_CHARS, RES_BAR_CELLS, BAR_ACTOR_COUNT, GROUP_BAR,
};

static const PkwdBookActorGroup sPkwdBookMarkGroup = {
    RES_MARK_PALETTE, RES_MARK_CHARS, RES_MARK_CELLS, MARK_ACTOR_COUNT, GROUP_MARK,
};

static const PkwdBookActorGroup sPkwdBookTrainerGroup = {
    RES_TRAINER_PALETTE, 0, RES_TRAINER_CELLS, TRAINER_ACTOR_COUNT, GROUP_TRAINER,
};

static const PkwdBookWindowSetup sPkwdBookListWindows[2] = {
    { 4, 2, 1, 28, 4, 10 },
    { 0xff, 0, 0, 0, 0, 0 },
};

// Where the lower screen's BG 3 goes for each page
static const ClActorPos sPkwdBookPageScrolls[3] = { { 0, 192 }, { 256, 0 }, { 0, 0 } };

static const PkwdBookLoad sPkwdBookScreenLoads[3] = {
    { 3, 38, 0, 0 },
    { 7, 37, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const PkwdBookLoad sPkwdBookCharLoads[3] = {
    { 3, 25, 0, 0 },
    { 7, 24, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const FrameListCallbacks sPkwdBookListCallbacks = {
    PkwdBookList_Print,
    PkwdBookList_Select,
    PkwdBookList_Scroll,
};

// The files of each series' palettes: the upper screen's, then its text's
static const s16 sPkwdBookSeriesPalettes[SERIES_COUNT][2] = { { 6, 16 }, { 7, 17 }, { 8, 18 }, { 9, 19 } };

static const PkwdBookLabel sPkwdBookListLabels[2] = {
    { 7, 0, FALSE, MSG_LINES },
    { 0xffff, 0, 0, 0 },
};

static const PkwdBookActorEntry sPkwdBookTrainerActors[2] = {
    { 0, 0x30, 1, 0, 0, 1, 17, RES_TRAINER_CHARS },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const TouchRect sPkwdBookCastTouchRects[4] = {
    { 168, 184, 96, 112 },
    { 168, 184, 148, 164 },
    { 168, 184, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sPkwdBookPokemonTouchRects[4] = {
    { 168, 184, 96, 112 },
    { 168, 184, 148, 164 },
    { 168, 184, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const s16 sPkwdBookLabelMessages[8] = {
    PokewoodLines_Text_Title,     PokewoodLines_Text_Plot,    PokewoodLines_Text_Mission, PokewoodLines_Text_Cast,
    PokewoodLines_Text_HeldItems, PokewoodLines_Text_Ability, PokewoodLines_Text_Role,    PokewoodLines_Text_Empty_3,
};

static const PkwdBookLoad sPkwdBookPaletteLoads[4] = {
    { PALTYPE_MAIN_BG, 9, 0, 0 },
    { PALTYPE_MAIN_BG, 44, 9, 1 },
    { PALTYPE_SUB_BG, 5, 6, 2 },
    { 0xff, 0, 0, 0 },
};

// The messages of a Pokémon's page: its name, the marks of its sex, its level, its item, its Ability, its moves and
// the slash of the page number
static const s16 sPkwdBookPokemonMessages[11] = {
    PokewoodLines_Text_Empty_3,  PokewoodLines_Text_Empty_4,  PokewoodLines_Text_Empty_5,  PokewoodLines_Text_Lv,
    PokewoodLines_Text_Empty_6,  PokewoodLines_Text_Empty_7,  PokewoodLines_Text_Empty_8,  PokewoodLines_Text_Empty_9,
    PokewoodLines_Text_Empty_10, PokewoodLines_Text_Empty_11, PokewoodLines_Text_Empty_12,
};

static const PkwdBookLabel sPkwdBookCastLabels[3] = {
    { 3, 0, FALSE, MSG_LINES },
    { 6, 1, TRUE, MSG_LINES },
    { 0xffff, 0, 0, 0 },
};

static const PkwdBookWindowSetup sPkwdBookCastWindows[5] = {
    { 4, 2, 1, 28, 2, 10 },  { 4, 1, 15, 15, 2, 13 }, { 4, 1, 17, 15, 2, 13 },
    { 4, 1, 19, 15, 2, 13 }, { 0xff, 0, 0, 0, 0, 0 },
};

static const PkwdBookLabel sPkwdBookMovieLabels[4] = {
    { 0, 0, FALSE, MSG_LINES },
    { 1, 1, FALSE, MSG_LINES },
    { 2, 2, FALSE, MSG_LINES },
    { 0xffff, 0, 0, 0 },
};

static const PkwdBookLabel sPkwdBookPokemonLabels[4] = {
    { 3, 0, FALSE, MSG_LINES },
    { 4, 3, TRUE, MSG_LINES },
    { 5, 9, TRUE, MSG_LINES },
    { 0xffff, 0, 0, 0 },
};

static const PkwdBookActorEntry sPkwdBookBarActors[4] = {
    { 1, 0x30, 1, 1, 0, 1, 3, RES_BAR_CHARS },
    { 4, 0x30, 1, 1, 0, 1, 0, RES_BAR_CHARS },
    { 5, 0x30, 1, 1, 0, 1, 1, RES_BAR_CHARS },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const PkwdBookActorRes sPkwdBookMarkRes = {
    { 2, RES_MARK_PALETTE, 0x1a0, 2 },
    { 21, RES_MARK_CHARS, 0, 0 },
    { 27, RES_MARK_CELLS, 0, 0 },
    { 39, -1, 0, 0 },
};

static const PkwdBookTextActorEntry sPkwdBookTextActors[TEXT_ACTOR_COUNT + 1] = {
    { 2, 2, 0x38, 1, 1, 0, 20, 0 },
    { 2, 2, 0x38, 1, 1, 0, 21, 0 },
    { 2, 2, 0x38, 1, 1, 0, 22, 0 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const FrameListSetup sPkwdBookListSetup = {
    { 6, 0xff, 2, 0, 28, 3, 2, 0, 26, 3, 0, { 24, 12, 8, 6, 4, 3 }, 1, 8, 0 },
    LIST_ROWS,
    4,
    0,
    LIST_ROWS,
    0,
    sPkwdBookListTouch,
    NULL,
    NULL,
};

// The series of each movie, which gives the color of its row and of the upper screen
static const u8 sPkwdBookSeries[MOVIE_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
};

// The graphics of each movie's poster
static const u8 sPkwdBookPosterGraphics[MOVIE_COUNT] = {
    0, 11, 11, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,  4,  4,  4,  5,  5, 5,
    6, 6,  6,  7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 12, 0,
};

static const PkwdBookWindowSetup sPkwdBookMainWindows[7] = {
    { 1, 2, 1, 28, 2, 5 },  { 1, 2, 5, 28, 2, 6 },  { 1, 2, 12, 28, 2, 6 },  { 1, 2, 3, 28, 2, 7 },
    { 1, 3, 7, 28, 4, 7 },  { 1, 3, 14, 28, 4, 7 }, { 0xff, 0, 0, 0, 0, 0 },
};

static const PkwdBookLoad sPkwdBookPosterChars[14] = {
    { 2, 0x2d, 0, 0 }, { 2, 0x2e, 0, 0 }, { 2, 0x2f, 0, 0 }, { 2, 0x30, 0, 0 }, { 2, 0x31, 0, 0 },
    { 2, 0x32, 0, 0 }, { 2, 0x33, 0, 0 }, { 2, 0x34, 0, 0 }, { 2, 0x35, 0, 0 }, { 2, 0x36, 0, 0 },
    { 2, 0x37, 0, 0 }, { 2, 0x38, 0, 0 }, { 2, 0x39, 0, 0 }, { 0xff, 0, 0, 0 },
};

static const PkwdBookLoad sPkwdBookPosterScreens[14] = {
    { 2, 0x3a, 0, 0 }, { 2, 0x3b, 0, 0 }, { 2, 0x3c, 0, 0 }, { 2, 0x3d, 0, 0 }, { 2, 0x3e, 0, 0 },
    { 2, 0x3f, 0, 0 }, { 2, 0x40, 0, 0 }, { 2, 0x41, 0, 0 }, { 2, 0x42, 0, 0 }, { 2, 0x43, 0, 0 },
    { 2, 0x44, 0, 0 }, { 2, 0x45, 0, 0 }, { 2, 0x46, 0, 0 }, { 0xff, 0, 0, 0 },
};

static const PkwdBookActorEntry sPkwdBookTypeIconActors[TYPE_ICON_COUNT + 1] = {
    { 0, 0x30, 1, 0, 0, 1, 11, RES_TYPE_ICON_CHARS },     { 0, 0x30, 1, 0, 0, 1, 12, RES_TYPE_ICON_CHARS + 1 },
    { 0, 0x30, 1, 0, 0, 1, 13, RES_TYPE_ICON_CHARS + 2 }, { 0, 0x30, 1, 0, 0, 1, 14, RES_TYPE_ICON_CHARS + 3 },
    { 0, 0x30, 1, 0, 0, 1, 15, RES_TYPE_ICON_CHARS + 4 }, { 0, 0x30, 1, 0, 0, 1, 16, RES_TYPE_ICON_CHARS + 5 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

const GameProcFunctions PKWD_BOOK_PROC_FUNCTIONS = { PkwdBook_Init, PkwdBook_Main, PkwdBook_Exit };

static const PkwdBookActorEntry sPkwdBookPokeIconActors[POKE_ICON_COUNT + 1] = {
    { 1, 0x28, 1, 0, 0, 0, 4, RES_POKE_ICON_CHARS },      { 1, 0x28, 1, 0, 0, 0, 5, RES_POKE_ICON_CHARS + 1 },
    { 1, 0x28, 1, 0, 0, 0, 6, RES_POKE_ICON_CHARS + 2 },  { 1, 0x28, 1, 0, 0, 0, 7, RES_POKE_ICON_CHARS + 3 },
    { 1, 0x28, 1, 0, 0, 0, 8, RES_POKE_ICON_CHARS + 4 },  { 1, 0x28, 1, 0, 0, 0, 9, RES_POKE_ICON_CHARS + 5 },
    { 1, 0x28, 1, 0, 0, 0, 10, RES_POKE_ICON_CHARS + 6 }, { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const PkwdBookWindowSetup sPkwdBookPokemonWindows[12] = {
    { 4, 2, 1, 28, 2, 10 },  { 4, 1, 6, 10, 2, 13 },  { 4, 2, 11, 10, 2, 13 }, { 4, 4, 15, 12, 2, 13 },
    { 4, 13, 15, 12, 2, 13 }, { 4, 16, 4, 12, 2, 13 }, { 4, 16, 7, 12, 2, 13 }, { 4, 16, 10, 12, 2, 13 },
    { 4, 16, 13, 12, 2, 13 }, { 4, 4, 17, 12, 2, 13 }, { 4, 13, 17, 12, 2, 13 }, { 0xff, 0, 0, 0, 0, 0 },
};

// The list's touch table when all its movies fit, with no scroll bar
static const FrameListTouch sPkwdBookShortListTouch[10] = {
    { { 0, 23, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 24, 47, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 48, 71, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 72, 95, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 96, 119, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 120, 143, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 144, 167, 16, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 168, 192, 136, 160 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 192, 168, 192 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

static const FrameListTouch sPkwdBookListTouch[11] = {
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

static const PkwdBookActorEntry sPkwdBookMarkActors[MARK_ACTOR_COUNT + 1] = {
    { 2, 0x30, 1, 0, 0, 1, 23, RES_MARK_CHARS }, { 0, 0x30, 1, 0, 0, 1, 24, RES_MARK_CHARS },
    { 1, 0x30, 2, 0, 0, 1, 25, RES_MARK_CHARS }, { 1, 0x30, 2, 0, 0, 1, 26, RES_MARK_CHARS },
    { 1, 0x30, 2, 0, 0, 1, 27, RES_MARK_CHARS }, { 1, 0x30, 2, 0, 0, 1, 28, RES_MARK_CHARS },
    { 1, 0x30, 2, 0, 0, 1, 29, RES_MARK_CHARS }, { 1, 0x30, 2, 0, 0, 1, 30, RES_MARK_CHARS },
    { 1, 0x30, 2, 0, 0, 1, 31, RES_MARK_CHARS }, { 1, 0x30, 2, 0, 0, 1, 32, RES_MARK_CHARS },
    { 1, 0x30, 2, 0, 0, 1, 33, RES_MARK_CHARS }, { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

// The scripts of each movie, for a boy and then for a girl
static const u16 sPkwdBookScriptBanks[MOVIE_COUNT * 2] = {
    TEXT_BANK_0182, TEXT_BANK_0181,
    TEXT_BANK_0184, TEXT_BANK_0183,
    TEXT_BANK_0186, TEXT_BANK_0185,
    TEXT_BANK_0188, TEXT_BANK_0187,
    TEXT_BANK_0190, TEXT_BANK_0189,
    TEXT_BANK_0192, TEXT_BANK_0191,
    TEXT_BANK_0194, TEXT_BANK_0193,
    TEXT_BANK_0196, TEXT_BANK_0195,
    TEXT_BANK_0198, TEXT_BANK_0197,
    TEXT_BANK_0200, TEXT_BANK_0199,
    TEXT_BANK_0202, TEXT_BANK_0201,
    TEXT_BANK_0204, TEXT_BANK_0203,
    TEXT_BANK_0206, TEXT_BANK_0205,
    TEXT_BANK_0208, TEXT_BANK_0207,
    TEXT_BANK_0210, TEXT_BANK_0209,
    TEXT_BANK_0212, TEXT_BANK_0211,
    TEXT_BANK_0214, TEXT_BANK_0213,
    TEXT_BANK_0216, TEXT_BANK_0215,
    TEXT_BANK_0218, TEXT_BANK_0217,
    TEXT_BANK_0220, TEXT_BANK_0219,
    TEXT_BANK_0222, TEXT_BANK_0221,
    TEXT_BANK_0224, TEXT_BANK_0223,
    TEXT_BANK_0226, TEXT_BANK_0225,
    TEXT_BANK_0228, TEXT_BANK_0227,
    TEXT_BANK_0230, TEXT_BANK_0229,
    TEXT_BANK_0232, TEXT_BANK_0231,
    TEXT_BANK_0234, TEXT_BANK_0233,
    TEXT_BANK_0236, TEXT_BANK_0235,
    TEXT_BANK_0238, TEXT_BANK_0237,
    TEXT_BANK_0240, TEXT_BANK_0239,
    TEXT_BANK_0242, TEXT_BANK_0241,
    TEXT_BANK_0244, TEXT_BANK_0243,
    TEXT_BANK_0246, TEXT_BANK_0245,
    TEXT_BANK_0248, TEXT_BANK_0247,
    TEXT_BANK_0250, TEXT_BANK_0249,
    TEXT_BANK_0252, TEXT_BANK_0251,
    TEXT_BANK_0254, TEXT_BANK_0253,
    TEXT_BANK_0256, TEXT_BANK_0255,
    TEXT_BANK_0258, TEXT_BANK_0257,
    TEXT_BANK_0260, TEXT_BANK_0259,
    TEXT_BANK_0262, TEXT_BANK_0261,
};

static const PkwdBookWindowSetup *sPkwdBookSubWindows[3] = {
    sPkwdBookListWindows,
    sPkwdBookCastWindows,
    sPkwdBookPokemonWindows,
};

static const TouchRect *sPkwdBookTouchRects[3] = {
    sPkwdBookListTouchRects,
    sPkwdBookCastTouchRects,
    sPkwdBookPokemonTouchRects,
};

// The headings of the upper screen, then those of each page of the lower screen
static const PkwdBookLabel *sPkwdBookLabels[4] = {
    sPkwdBookMovieLabels,
    sPkwdBookListLabels,
    sPkwdBookCastLabels,
    sPkwdBookPokemonLabels,
};

static PkwdBookPageFunc sPkwdBookPageFuncs[PAGE_STATE_COUNT] = {
    PkwdBook_EnterList,    PkwdBook_RunList,    PkwdBook_LeaveList,
    PkwdBook_EnterCast,    PkwdBook_RunCast,    PkwdBook_LeaveCast,
    PkwdBook_EnterPokemon, PkwdBook_RunPokemon, PkwdBook_LeavePokemon,
};

static BOOL PkwdBook_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdBookWork *wk;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PKWD_BOOK, 0x38000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PkwdBookWork), HEAPID_PKWD_BOOK);
    sys_memset(wk, 0, sizeof(PkwdBookWork));
    wk->heapId = HEAPID_PKWD_BOOK;
    wk->param = param;
    wk->graphic = PkwdBookGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreate(8, 64, wk->heapId);
    wk->menu = NULL;
    wk->menuRes = AppTaskMenuRes_Create(4, 11, wk->font, wk->printQueue, wk->heapId);
    PkwdBook_InitScreen(wk);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    return TRUE;
}

static BOOL PkwdBook_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdBookWork *wk = work;
    HeapID heapId;

    if (!func_02021c0c(wk->printQueue)) {
        return FALSE;
    }
    if (wk->param->result != NULL) {
        *wk->param->result = wk->result;
    }
    PkwdBook_FreeScreen(wk);
    AppTaskMenuRes_Free(wk->menuRes);
    GFL_WordSetSystemFree(wk->wordSet);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    GFL_FontFree(wk->smallFont);
    PkwdBookGraphic_Free(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL PkwdBook_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdBookWork *wk = work;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
            wk->flags |= FLAG_WIPE_IN_DONE;
        }
        break;
    case 1:
        if (wk->step == STEP_END) {
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
    PkwdBook_Step(wk);
    PkwdBook_UpdateText(wk);
    PkwdBookGraphic_Update(wk->graphic);
    PkwdBookGraphic_Begin3D(wk->graphic);
    PkwdBookGraphic_End3D(wk->graphic);
    return FALSE;
}

static void PkwdBook_InitScreen(PkwdBookWork *wk) {
    int i;
    ArcTool *arc;

    wk->step = STEP_START;
    wk->pageState = PAGE_STATE_ENTER_LIST;
    wk->pageStep = PAGE_STEP_START;
    wk->page = PAGE_LIST;
    wk->nextPageState = 0;
    wk->flags = 0;
    wk->subShown = 0;
    wk->mainShown = 0;
    wk->selected = 0;
    wk->movieCount = 20;
    wk->cursorPos = 0;
    wk->scroll = 0;
    wk->castPage = 0;
    wk->result = PKWD_BOOK_CANCEL;
    wk->trainerClass = 0;
    wk->newMarks = 0;
    wk->lastScroll = -1;
    wk->castCount = 1;
    wk->castIdx = 0;
    wk->party = NULL;
    wk->list = NULL;
    wk->unk548 = 0;
    for (i = 0; i < LIST_MOVIE_COUNT; i++) {
        wk->movies[i].movie = 0xff;
        wk->movies[i].series = 0;
        wk->movies[i].isNew = FALSE;
    }
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->subWindows[i].window = NULL;
        wk->mainWindows[i].window = NULL;
    }
    for (i = 0; i < SERIES_COUNT; i++) {
        wk->paletteFiles[i] = NULL;
        wk->textPaletteFiles[i] = NULL;
    }
    wk->positions = GFL_ArcSysReadHeapNew(ARCID_PKWD_BOOK_GRA, 42, wk->heapId);
    wk->msgData[MSG_LINES] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEWOOD_LINES, wk->heapId);
    wk->msgData[MSG_SCRIPT] = NULL;
    wk->msgData[MSG_TRAINER_CLASSES] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TRAINER_CLASS_NAMES, wk->heapId);
    wk->msgData[MSG_COSTARS] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_PASS_POWER_STARTMENU, wk->heapId);
    PkwdBook_LoadRoleNames(wk);
    PkwdBook_Unused2(wk);
    PkwdBook_ListMovies(wk);
    PkwdBook_LoadCast(wk, PkwdBook_GetMovie(wk, wk->selected));
    PkwdBook_LoadGraphics(wk);
    PkwdBook_LoadTouchBarBG(wk, 5, 8);
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_SUB_BG, 10 * 0x20, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 15, PALTYPE_SUB_BG, 13 * 0x20, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
    PkwdBook_CreateWindows(wk, FALSE, PAGE_LIST);
    PkwdBook_CreateWindows(wk, TRUE, PAGE_LIST);
    PkwdBook_PrintLabels(wk, FALSE, PAGE_LIST);
    PkwdBook_PrintMovie(wk, FALSE);
    PkwdBook_InitActors(wk);
    PkwdBook_LoadSeriesPalettes(wk);
    PkwdBook_TransferSeriesPalette(wk, wk->movies[wk->selected].series);
    PkwdBook_CreateList(wk);
    PkwdBook_SetArrows(wk, 0, wk->movieCount - 1, wk->selected);
    PkwdBook_ScrollBG(wk, PAGE_LIST);
    PkwdBook_ShowMovie(wk, FALSE);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, 0x3f, 8, 3);
    wk->blendAlpha1 = 3;
    wk->blendAlpha2 = 15;
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, 0x3a, wk->blendAlpha1, wk->blendAlpha2);
}

static void PkwdBook_Step(PkwdBookWork *wk) {
    PkwdBook_StepSequence(wk);
    PkwdBook_Unused1(wk);
}

static void PkwdBook_FreeScreen(PkwdBookWork *wk) {
    int i;

    PkwdBook_FreeSeriesPalettes(wk);
    PkwdBook_FreeCast(wk);
    PkwdBook_FreeRoleNames(wk);
    GFL_HeapFree(wk->positions);
    PkwdBook_FreeWindows(wk, FALSE);
    PkwdBook_FreeWindows(wk, TRUE);
    for (i = 0; i < MSG_DATA_COUNT; i++) {
        if (wk->msgData[i] != NULL) {
            GFL_MsgDataFree(wk->msgData[i]);
            wk->msgData[i] = NULL;
        }
    }
    PkwdBook_FreeList(wk);
    PkwdBook_FreeActors(wk);
}

static void PkwdBook_StepSequence(PkwdBookWork *wk) {
    PkwdBook_RunSequence(wk);
}

static void PkwdBook_RunSequence(PkwdBookWork *wk) {
    switch (wk->step) {
    case STEP_START:
        wk->step = STEP_WAIT_WIPE;
        PkwdBook_LoadPoster(wk, 0, TRUE);
        break;
    case STEP_WAIT_WIPE:
        if (wk->flags & FLAG_WIPE_IN_DONE) {
            wk->step = STEP_RUN;
        }
        break;
    case STEP_RUN:
        if (!PkwdBook_RunPage(wk)) {
            wk->step = STEP_END;
        }
        break;
    case STEP_END:
        break;
    }
}

// The x or the y of a position that the screen's file holds, or 0 past its positions
static s16 PkwdBook_GetPosition(PkwdBookWork *wk, u32 index) {
    if (index >= POSITION_COUNT * 2) {
        return 0;
    }
    return wk->positions[index];
}

static void PkwdBook_LoadGraphics(PkwdBookWork *wk) {
    const PkwdBookLoad *palette = sPkwdBookPaletteLoads;
    const PkwdBookLoad *chars = sPkwdBookCharLoads;
    const PkwdBookLoad *screen = sPkwdBookScreenLoads;
    int col;
    u16 *rowBuf;
    int row;
    u16 *pageBuf;
    u16 *screenBuf;
    int i;
    int page;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);

    for (i = 0; i < LOAD_COUNT_MAX; palette++, i++) {
        if (palette->target == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, palette->fileId, palette->target, palette->offset << 5, palette->extra << 5,
                                    wk->heapId);
    }
    for (i = 0; i < LOAD_COUNT_MAX; i++, chars++) {
        if (chars->target == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->fileId, chars->target, chars->offset, 0, FALSE, wk->heapId);
    }
    for (i = 0; i < LOAD_COUNT_MAX; i++, screen++) {
        if (screen->target == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screen->fileId, screen->target, screen->offset, 0, FALSE,
                                               wk->heapId);
    }
    GFL_ArcToolFree(arc);
    // The three pages of the lower screen's BG 3 are in palette 6
    screenBuf = GFL_BGSysIsScrHeapExists(7);
    for (page = 0; page < 3; page++) {
        pageBuf = &screenBuf[page * 0x400];
        for (row = 0; row < 32; row++) {
            for (col = 0, rowBuf = &pageBuf[row * 32]; col < 32; col++) {
                rowBuf[col] += 6 << 12;
            }
        }
    }
    GFL_BGSysLoadScr(7);
}

// Loads the touch bar's BG in a palette
static void PkwdBook_LoadTouchBarBG(PkwdBookWork *wk, u32 bg, u32 palette) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    u16 *screenBuf;
    int row;
    int col;

    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), PALTYPE_SUB_BG, palette << 5, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), bg, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), bg, 0, 0, FALSE, wk->heapId);
    screenBuf = GFL_BGSysIsScrHeapExists(bg);
    for (row = 0; row < 24; row++) {
        for (col = 0; col < 32; col++) {
            screenBuf[row * 32 + col] += (palette & 0xf) << 12;
        }
    }
    GFL_BGSysLoadScr(bg);
    GFL_ArcToolFree(arc);
}

// Loads a poster to the upper screen, at once or in the next VBlank
static void PkwdBook_LoadPoster(PkwdBookWork *wk, u32 graphicIdx, BOOL deferred) {
    const PkwdBookLoad *chars = sPkwdBookPosterChars;
    const PkwdBookLoad *screen = sPkwdBookPosterScreens;
    ArcTool *arc;

    if (deferred == TRUE) {
        NNSG2dCharacterData *charData;
        NNSG2dScreenData *screenData;
        void *screenFile;
        void *charFile;

        arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);
        screen += graphicIdx;
        screenFile = GFL_G2DIOReadNSCRArc(arc, screen->fileId, FALSE, &screenData, wk->heapId);
        GFL_BGSysBufferScrDefault(2, screenData->rawData, 0x800);
        GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 24, 9);
        GFL_BGSysQueueScrLoad(2);
        chars += graphicIdx;
        charFile = GFL_G2DIOReadBGNCGRArc(arc, chars->fileId, FALSE, &charData, wk->heapId);
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG2_CHAR_MAIN, 0, charData->rawData, 0x4000);
        GFL_ArcToolFree(arc);
        GFL_HeapFree(screenFile);
        GFL_HeapFree(charFile);
    } else {
        arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);
        chars += graphicIdx;
        GFL_BGSysLoadArcNCGRStatic(arc, chars->fileId, chars->target, chars->offset, 0, FALSE, wk->heapId);
        screen += graphicIdx;
        loadBGScrToVramByFileNoReserveNegAlign(arc, screen->fileId, screen->target, screen->offset, 0, FALSE,
                                               wk->heapId);
        GFL_ArcToolFree(arc);
        GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 24, 9);
        GFL_BGSysLoadScr(2);
    }
}

// Loads the font's palette for the texts that are drawn as cell actors
static void PkwdBook_LoadFontPalette(PkwdBookWork *wk, u32 surface) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);

    wk->resources.named.fontPalette = func_0204bc48(arc, 6, surface, 12 * 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
}

// Loads the resources of the touch bar's buttons
static void PkwdBook_LoadBarRes(PkwdBookWork *wk, u32 surface) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    u32 paletteFile;
    u32 charsFile;
    u32 cellFile;
    u32 animFile;

    PkwdBookGraphic_GetClActUnit(wk->graphic);
    paletteFile = func_0202d810();
    charsFile = func_0202d814();
    cellFile = func_0202d818(2);
    animFile = func_0202d81c(2);
    wk->resources.named.barPalette = func_0204bbb8(arc, paletteFile, surface, 4 * 0x20, 0, 4, wk->heapId);
    wk->resources.named.barChars = func_0204b81c(arc, charsFile, FALSE, surface, wk->heapId);
    wk->resources.named.barCells = func_0204bde0(arc, cellFile, animFile, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void PkwdBook_LoadPokeIconRes(PkwdBookWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(wk->heapId));
    u32 i;

    wk->resources.named.pokeIconPalette =
        func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0, wk->heapId);
    wk->resources.named.pokeIconCells =
        func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), wk->heapId);
    for (i = 0; i < POKE_ICON_COUNT; i++) {
        wk->resources.named.pokeIconChars[i] =
            func_0204b81c(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, CLACT_VRAM_SUB, wk->heapId);
    }
    GFL_ArcToolFree(arc);
}

// Gives an icon the graphics of a species
static void PkwdBook_SetPokeIcon(PkwdBookWork *wk, u32 slot, u32 species, u32 sex) {
    u32 palette = PokeIcon_GetPalette(species, 0, sex, FALSE);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(wk->heapId));
    NNSG2dCharacterData *charData;
    void *file =
        GFL_G2DIOReadOBJNCGRArc(arc, PokeParty_GetIconIndex(species, 1, sex, FALSE), FALSE, &charData,
                                HEAPID_TAIL(wk->heapId));

    func_0204ba40(wk->resources.named.pokeIconChars[slot], charData);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    func_0204c378(wk->pokeIcons[slot], palette, 1);
}

static void PkwdBook_LoadTypeIconRes(PkwdBookWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->heapId));
    u32 i;

    wk->resources.named.typeIconPalette = func_0204bbb8(arc, 33, CLACT_VRAM_SUB, 8 * 0x20, 0, 3, wk->heapId);
    wk->resources.named.typeIconCells = func_0204bde0(arc, 61, 64, wk->heapId);
    for (i = 0; i < TYPE_ICON_COUNT; i++) {
        wk->resources.named.typeIconChars[i] = func_0204b81c(arc, func_0202d7f4(0), FALSE, CLACT_VRAM_SUB, wk->heapId);
    }
    GFL_ArcToolFree(arc);
}

// Gives an icon the graphics of a type
static void PkwdBook_SetTypeIcon(PkwdBookWork *wk, u32 slot, u32 unused, u32 type) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->heapId));
    NNSG2dCharacterData *charData;
    void *file = GFL_G2DIOReadOBJNCGRArc(arc, func_0202d7f4(type), FALSE, &charData, HEAPID_TAIL(wk->heapId));

    func_0204ba40(wk->resources.named.typeIconChars[slot], charData);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    func_0204c378(wk->typeIcons[slot], func_0202d7e8(type), 1);
}

static void PkwdBook_LoadTrainerRes(PkwdBookWork *wk, u32 surface, u32 trainerClass) {
    ArcTool *arc = MakeTrGraArcHandle(wk->heapId);

    wk->resources.named.trainerPalette = TrGra_LoadClActPalette(arc, trainerClass, surface, 11 * 0x20, wk->heapId);
    wk->resources.named.trainerChars = TrGra_LoadClActChars(arc, trainerClass, surface, wk->heapId);
    wk->resources.named.trainerCells = TrGra_LoadClActCellAnims(trainerClass, 2, surface, wk->heapId);
    GFL_ArcToolFree(arc);
}

// Gives the trainer's actor the graphics of a trainer class
static void PkwdBook_SetTrainer(PkwdBookWork *wk, u32 trainerClass) {
    ArcTool *arc = MakeTrGraArcHandle(wk->heapId);

    TrGra_ReplaceClActCharsAndPalette(arc, trainerClass, wk->resources.named.trainerChars,
                                      wk->resources.named.trainerPalette, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void PkwdBook_LoadActorResources(PkwdBookWork *wk, u32 surface, const PkwdBookActorRes *res) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);

    PkwdBookGraphic_GetClActUnit(wk->graphic);
    wk->resources.slots[res->palette.slot] =
        func_0204bbb8(arc, res->palette.fileId, surface, res->palette.offset, 0, res->palette.count, wk->heapId);
    wk->resources.slots[res->chars.slot] = func_0204b81c(arc, res->chars.fileId, FALSE, surface, wk->heapId);
    wk->resources.slots[res->cells.slot] = func_0204bde0(arc, res->cells.fileId, res->anims.fileId, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void PkwdBook_CreateActors(PkwdBookWork *wk, u32 surface, const PkwdBookActorEntry *entries,
                                  const PkwdBookActorGroup *group) {
    int i;
    ClActUnit *unit = PkwdBookGraphic_GetClActUnit(wk->graphic);
    ClActorSetup setup;
    ClActor **actors;
    s16 count;
    s16 groupIdx;
    s16 paletteSlot;
    s16 cellSlot;

    sys_memset(&setup, 0, sizeof(setup));
    paletteSlot = group->paletteSlot;
    cellSlot = group->cellSlot;
    count = group->count;
    groupIdx = group->group;
    if (groupIdx == GROUP_TRAINER) {
        actors = wk->trainerActors;
    } else if (groupIdx == GROUP_BAR) {
        actors = wk->barActors;
    } else if (groupIdx == GROUP_POKE_ICON) {
        actors = wk->pokeIcons;
    } else if (groupIdx == GROUP_TYPE_ICON) {
        actors = wk->typeIcons;
    } else if (groupIdx == GROUP_MARK) {
        actors = wk->markActors;
    }
    for (i = 0; i < count; entries++, i++) {
        u32 positionIdx;
        s16 x;
        s16 y;
        BOOL visible;
        BOOL animated;

        if (entries->sequence == 0xff) {
            break;
        }
        positionIdx = entries->positionIdx * 2;
        x = PkwdBook_GetPosition(wk, positionIdx);
        y = PkwdBook_GetPosition(wk, positionIdx + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = entries->sequence;
        setup.priority = entries->priority;
        setup.bgPriority = entries->bgPriority;
        actors[i] = func_0204c040(unit, wk->resources.slots[entries->charsSlot], wk->resources.slots[paletteSlot],
                                  wk->resources.slots[cellSlot], &setup, surface, wk->heapId);
        visible = FALSE;
        if (entries->visible == 1) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        animated = FALSE;
        if (entries->flags & 1) {
            animated = TRUE;
        }
        func_0204c520(actors[i], animated);
        if (entries->flags & 2) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (entries->flags & 4) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
    }
}

static void PkwdBook_InitActors(PkwdBookWork *wk) {
    ClActUnit *unit = PkwdBookGraphic_GetClActUnit(wk->graphic);

    PkwdBook_LoadBarRes(wk, CLACT_VRAM_SUB);
    PkwdBook_LoadPokeIconRes(wk);
    PkwdBook_LoadTypeIconRes(wk);
    PkwdBook_LoadTrainerRes(wk, CLACT_VRAM_SUB, 1);
    PkwdBook_LoadFontPalette(wk, CLACT_VRAM_SUB);
    PkwdBook_LoadActorResources(wk, CLACT_VRAM_SUB, &sPkwdBookMarkRes);
    PkwdBook_CreateActors(wk, CLACT_SURFACE_SUB, sPkwdBookBarActors, &sPkwdBookBarGroup);
    PkwdBook_CreateActors(wk, CLACT_SURFACE_SUB, sPkwdBookPokeIconActors, &sPkwdBookPokeIconGroup);
    PkwdBook_CreateActors(wk, CLACT_SURFACE_SUB, sPkwdBookTypeIconActors, &sPkwdBookTypeIconGroup);
    PkwdBook_CreateActors(wk, CLACT_SURFACE_SUB, sPkwdBookTrainerActors, &sPkwdBookTrainerGroup);
    PkwdBook_CreateActors(wk, CLACT_SURFACE_SUB, sPkwdBookMarkActors, &sPkwdBookMarkGroup);
    func_02042ba8(TRUE, wk->heapId);
    wk->bmpOam = BmpOam_Init(wk->heapId, unit);
    PkwdBook_CreateTextActors(wk, CLACT_SURFACE_SUB, sPkwdBookTextActors, RES_FONT_PALETTE);
    func_0204c318(wk->markActors[MARK_PAGE], 1);
}

static void PkwdBook_FreeActors(PkwdBookWork *wk) {
    int i;

    for (i = 0; i < BAR_ACTOR_COUNT; i++) {
        func_0204c108(wk->barActors[i]);
    }
    for (i = 0; i < POKE_ICON_COUNT; i++) {
        func_0204c108(wk->pokeIcons[i]);
    }
    for (i = 0; i < TYPE_ICON_COUNT; i++) {
        func_0204c108(wk->typeIcons[i]);
    }
    func_0204c108(wk->trainerActors[0]);
    for (i = 0; i < MARK_ACTOR_COUNT; i++) {
        func_0204c108(wk->markActors[i]);
    }
    func_0204bcd0(wk->resources.named.barPalette);
    func_0204b98c(wk->resources.named.barChars);
    func_0204be64(wk->resources.named.barCells);
    func_0204bcd0(wk->resources.named.pokeIconPalette);
    for (i = 0; i < POKE_ICON_COUNT; i++) {
        func_0204b98c(wk->resources.named.pokeIconChars[i]);
    }
    func_0204be64(wk->resources.named.pokeIconCells);
    func_0204bcd0(wk->resources.named.typeIconPalette);
    for (i = 0; i < TYPE_ICON_COUNT; i++) {
        func_0204b98c(wk->resources.named.typeIconChars[i]);
    }
    func_0204be64(wk->resources.named.typeIconCells);
    func_0204bcd0(wk->resources.named.trainerPalette);
    func_0204b98c(wk->resources.named.trainerChars);
    func_0204be64(wk->resources.named.trainerCells);
    func_0204bcd0(wk->resources.named.markPalette);
    func_0204b98c(wk->resources.named.markChars);
    func_0204be64(wk->resources.named.markCells);
    func_0204bcd0(wk->resources.named.fontPalette);
    for (i = 0; i < TEXT_ACTOR_COUNT; i++) {
        BmpOam_ActorDel(wk->textActors[i]);
        GFL_BitmapFree(wk->textBitmaps[i]);
    }
    BmpOam_Exit(wk->bmpOam);
}

static void PkwdBook_PlayAnim(PkwdBookWork *wk, int group, int index, u32 sequence) {
    if (group == GROUP_TRAINER) {
        if (index < TRAINER_ACTOR_COUNT) {
            func_0204c488(wk->trainerActors[index], (u16)sequence);
            func_0204c520(wk->trainerActors[index], TRUE);
        }
    } else if (group == GROUP_BAR) {
        if (index < BAR_ACTOR_COUNT) {
            func_0204c488(wk->barActors[index], (u16)sequence);
            func_0204c520(wk->barActors[index], TRUE);
        }
    } else if (group == GROUP_POKE_ICON) {
        if (index < POKE_ICON_COUNT) {
            func_0204c488(wk->pokeIcons[index], (u16)sequence);
            func_0204c520(wk->pokeIcons[index], TRUE);
        }
    } else if (group == GROUP_TYPE_ICON) {
        if (index < TYPE_ICON_COUNT) {
            func_0204c488(wk->typeIcons[index], (u16)sequence);
            func_0204c520(wk->typeIcons[index], TRUE);
        }
    } else if (group == GROUP_MARK) {
        if (index < MARK_ACTOR_COUNT) {
            func_0204c488(wk->markActors[index], (u16)sequence);
            func_0204c520(wk->markActors[index], TRUE);
        }
    }
}

static void PkwdBook_SetActorVisible(PkwdBookWork *wk, int group, int index, BOOL visible) {
    if (group == GROUP_TRAINER) {
        if (index < TRAINER_ACTOR_COUNT) {
            func_0204c124(wk->trainerActors[index], visible);
        }
    } else if (group == GROUP_BAR) {
        if (index < BAR_ACTOR_COUNT) {
            func_0204c124(wk->barActors[index], visible);
        }
    } else if (group == GROUP_POKE_ICON) {
        if (index < POKE_ICON_COUNT) {
            func_0204c124(wk->pokeIcons[index], visible);
        }
    } else if (group == GROUP_TYPE_ICON) {
        if (index < TYPE_ICON_COUNT) {
            func_0204c124(wk->typeIcons[index], visible);
        }
    } else if (group == GROUP_MARK) {
        if (index < MARK_ACTOR_COUNT) {
            func_0204c124(wk->markActors[index], visible);
        }
    }
}

static BOOL PkwdBook_IsActorAnimating(PkwdBookWork *wk, int group, int index) {
    BOOL result = TRUE;

    if (group == GROUP_TRAINER) {
        if (index >= TRAINER_ACTOR_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->trainerActors[index])) {
            result = FALSE;
        }
    } else if (group == GROUP_BAR) {
        if (index >= BAR_ACTOR_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->barActors[index])) {
            result = FALSE;
        }
    } else if (group == GROUP_POKE_ICON) {
        if (index >= POKE_ICON_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->pokeIcons[index])) {
            result = FALSE;
        }
    } else if (group == GROUP_TYPE_ICON) {
        if (index >= TYPE_ICON_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->typeIcons[index])) {
            result = FALSE;
        }
    } else if (group == GROUP_MARK) {
        if (index >= MARK_ACTOR_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->markActors[index])) {
            result = FALSE;
        }
    }
    return result;
}

// Creates the texts that are drawn as cell actors, which are the page number's
static void PkwdBook_CreateTextActors(PkwdBookWork *wk, u32 surface, const PkwdBookTextActorEntry *entries,
                                      u32 paletteSlot) {
    int i;

    // BUG: the entries are never stepped through, so the three texts are created from the first one. It only shows
    // in where they are until PkwdBook_PrintTextActor places them
    for (i = 0; i < TEXT_ACTOR_COUNT; i++) {
        BmpOamActorSetup setup;
        u32 positionIdx;
        s16 x;
        s16 y;

        if (entries->width == 0xff) {
            break;
        }
        wk->textBitmaps[i] = GFL_BitmapCreate(entries->width, entries->height, 0x20, wk->heapId);
        positionIdx = entries->positionIdx;
        x = PkwdBook_GetPosition(wk, positionIdx * 2);
        y = PkwdBook_GetPosition(wk, positionIdx * 2 + 1);
        sys_memset(&setup, 0, sizeof(setup));
        setup.bitmap = wk->textBitmaps[i];
        setup.x = x;
        setup.y = y;
        setup.palette = wk->resources.slots[paletteSlot];
        setup.priority = entries->priority;
        setup.bgPriority = entries->bgPriority;
        setup.paletteOffset = entries->paletteOffset;
        setup.surface = surface;
        setup.vramType = surface;
        wk->textActors[i] = BmpOam_ActorAdd(wk->bmpOam, &setup);
        if (entries->visible == 0) {
            BmpOam_ActorSetDrawEnable(wk->textActors[i], FALSE);
        }
#ifdef BUGFIX
        entries++;
#endif
    }
}

// Draws a text of the page number, off its position by x and y
static void PkwdBook_PrintTextActor(PkwdBookWork *wk, int index, const StrBuf *strbuf, s32 x, s32 y) {
    if (index < TEXT_ACTOR_COUNT) {
        const PkwdBookTextActorEntry *entry;
        u32 positionIdx;
        s16 baseX;
        s16 baseY;

        GFL_BitmapFill(wk->textBitmaps[index], 0);
        GFL_TextRendererDrawToBitmapEx(wk->textBitmaps[index], 0, 0, strbuf, wk->smallFont, PRINT_COLOR(15, 2, 0));
        BmpOam_ActorBmpTrans(wk->textActors[index]);
        entry = &sPkwdBookTextActors[index];
        positionIdx = entry->positionIdx * 2;
        baseX = PkwdBook_GetPosition(wk, positionIdx);
        baseY = PkwdBook_GetPosition(wk, positionIdx + 1);
        baseX += (s16)x;
        baseY += (s16)y;
        BmpOam_ActorSetPos(wk->textActors[index], baseX, baseY);
        BmpOam_ActorSetDrawEnable(wk->textActors[index], TRUE);
    }
}

static void PkwdBook_SetTextActorVisible(PkwdBookWork *wk, int index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(wk->textActors[index], visible);
}

// Creates the windows of the upper screen, or those of a page of the lower screen
static void PkwdBook_CreateWindows(PkwdBookWork *wk, BOOL isSub, u32 page) {
    int i;
    const PkwdBookWindowSetup *setup;

    if (!isSub) {
        setup = sPkwdBookMainWindows;
        wk->mainShown = 0;
    } else {
        setup = sPkwdBookSubWindows[page];
        wk->subShown = 0;
        PkwdBook_FreeWindows(wk, TRUE);
    }
    for (i = 0; i < WINDOW_COUNT; i++, setup++) {
        if (setup->bg == 0xff) {
            break;
        }
        if (!isSub) {
            wk->mainWindows[i].window =
                BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        } else {
            wk->subWindows[i].window =
                BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        }
        PkwdBook_FillWindow(wk, isSub, i, 0);
    }
}

static void PkwdBook_FreeWindows(PkwdBookWork *wk, BOOL isSub) {
    int i;

    if (!isSub) {
        for (i = 0; i < WINDOW_COUNT; i++) {
            if (wk->mainWindows[i].window != NULL) {
                GFL_BitmapFill(BmpWin_GetBitmap(wk->mainWindows[i].window), 0);
                BmpWin_ClearScreen(wk->mainWindows[i].window);
                BmpWin_Free(wk->mainWindows[i].window);
                wk->mainWindows[i].window = NULL;
            }
        }
    } else {
        for (i = 0; i < WINDOW_COUNT; i++) {
            if (wk->subWindows[i].window != NULL) {
                GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindows[i].window), 0);
                BmpWin_ClearScreen(wk->subWindows[i].window);
                BmpWin_Free(wk->subWindows[i].window);
                wk->subWindows[i].window = NULL;
            }
        }
    }
}

// Sends the windows whose text is printed to VRAM, and puts them on their BG the first time
static void PkwdBook_UpdateText(PkwdBookWork *wk) {
    int i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        if (wk->subWindows[i].window != NULL) {
            PrintWindow_Flush(&wk->subWindows[i], wk->printQueue);
            if (PrintWindow_IsPrinted(&wk->subWindows[i]) == TRUE) {
                if (!(wk->subShown & (1 << i))) {
                    BmpWin_FlushMap(wk->subWindows[i].window);
                    GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->subWindows[i].window));
                    wk->subShown |= 1 << i;
                }
            }
        }
        if (wk->mainWindows[i].window != NULL) {
            PrintWindow_Flush(&wk->mainWindows[i], wk->printQueue);
            if (PrintWindow_IsPrinted(&wk->mainWindows[i]) == TRUE) {
                if (!(wk->mainShown & (1 << i))) {
                    BmpWin_FlushMap(wk->mainWindows[i].window);
                    GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->mainWindows[i].window));
                    wk->mainShown |= 1 << i;
                }
            }
        }
    }
    PkwdBook_UpdateScroll(wk);
}

// Prints a text so that its left, right or center is at x, after clearing the window unless keep is set
static void PkwdBook_PrintText(PkwdBookWork *wk, BOOL isSub, u32 window, s32 x, s32 y, const StrBuf *strbuf, u32 align,
                               u16 color, BOOL keep) {
    u16 offset;

    if (!keep) {
        PkwdBook_FillWindow(wk, isSub, window, 0);
    }
    offset = PkwdBook_GetAlignOffset(strbuf, wk->font, align);
    if (!isSub) {
        PrintWindow_Print(&wk->mainWindows[window], wk->printQueue, x - offset, y, strbuf, wk->font, color);
    } else {
        PrintWindow_Print(&wk->subWindows[window], wk->printQueue, x - offset, y, strbuf, wk->font, color);
    }
}

static u16 PkwdBook_GetAlignOffset(const StrBuf *strbuf, Font *font, u32 align) {
    if (align == ALIGN_RIGHT) {
        return GFL_FontGetBlockWidth(strbuf, font, 0);
    } else if (align == ALIGN_CENTER) {
        return GFL_FontGetBlockWidth(strbuf, font, 0) / 2;
    }
    return 0;
}

static void PkwdBook_FillWindow(PkwdBookWork *wk, BOOL isSub, u32 window, u32 color) {
    if (!isSub) {
        if (wk->mainWindows[window].window != NULL) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->mainWindows[window].window), color);
            wk->mainWindows[window].flushPending = TRUE;
        }
    } else {
        if (wk->subWindows[window].window != NULL) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindows[window].window), color);
            wk->subWindows[window].flushPending = TRUE;
        }
    }
}

// Scrolls the lower screen's BG 3 to a page
static void PkwdBook_ScrollBG(PkwdBookWork *wk, u32 page) {
    GFL_BGSysMoveBG(7, BG_MOVE_SET_X, sPkwdBookPageScrolls[page].x);
    GFL_BGSysMoveBG(7, BG_MOVE_SET_Y, sPkwdBookPageScrolls[page].y);
}

// Prints the headings of the upper screen, or those of a page of the lower screen
static void PkwdBook_PrintLabels(PkwdBookWork *wk, BOOL isSub, u32 page) {
    const PkwdBookLabel *labels = sPkwdBookLabels[isSub + page];
    u16 color;
    int i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        u16 message = labels[i].message;
        u32 align;
        u16 window;
        u16 msgData;
        s32 x;
        StrBuf *strbuf;

        if (message == 0xffff) {
            break;
        }
        window = labels[i].window;
        msgData = labels[i].msgData;
        align = ALIGN_LEFT;
        x = 0;
        color = PRINT_COLOR(15, 2, 0);
        if (labels[i].dark == TRUE) {
            color = PRINT_COLOR(1, 2, 0);
        }
        if (!isSub) {
            color = PRINT_COLOR(1, 2, 0);
            if (i == 0) {
                align = ALIGN_CENTER;
                x = 112;
            }
        }
        if (message == 6) {
            x = 1;
        }
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[msgData], sPkwdBookLabelMessages[message]);
        PkwdBook_PrintText(wk, isSub, window, x, 0, strbuf, align, color, FALSE);
        GFL_StrBufFree(strbuf);
    }
}

// Scrolls the lower screen's BG to the page once its text is printed
static void PkwdBook_UpdateScroll(PkwdBookWork *wk) {
    if (wk->flags & FLAG_SCROLL_PENDING) {
        if (func_02021c0c(wk->printQueue) == TRUE) {
            PkwdBook_ScrollBG(wk, wk->page);
            wk->flags &= ~FLAG_SCROLL_PENDING;
        }
    }
}

static void PkwdBook_EnterList(PkwdBookWork *wk) {
    wk->pageState = PAGE_STATE_LIST;
    wk->pageStep = PAGE_STEP_START;
    wk->page = PAGE_LIST;
    if (wk->flags & FLAG_LIST_LEFT) {
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, FALSE);
        PkwdBook_CreateWindows(wk, TRUE, PAGE_LIST);
        PkwdBook_PrintMovie(wk, FALSE);
        wk->flags |= FLAG_SCROLL_PENDING;
        PkwdBook_PlaceArrows(wk, PAGE_LIST);
        PkwdBook_SetArrows(wk, 0, wk->movieCount - 1, wk->selected);
        PkwdBook_RecreateList(wk);
        PkwdBook_ScrollBG(wk, wk->page);
        wk->flags &= ~FLAG_SCROLL_PENDING;
    }
    PkwdBook_HideActors(wk, FALSE);
    PkwdBook_ShowScrollBar(wk);
    PkwdBook_UpdateNewMarks(wk, TRUE);
    PkwdBook_ScrollNewMarks(wk, 0);
    PkwdBook_ShowMovie(wk, FALSE);
    wk->flags = (wk->flags | FLAG_LIST_LEFT) & ~FLAG_CURSOR_SHOWN;
}

static void PkwdBook_RunList(PkwdBookWork *wk) {
    switch (wk->pageStep) {
    case PAGE_STEP_START:
        if (!(wk->flags & FLAG_LIST_DRAWN)) {
            if (!FrameList_Draw(wk->list)) {
                wk->flags |= FLAG_LIST_DRAWN;
                wk->pageStep = PAGE_STEP_INPUT;
                GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, TRUE);
            }
        } else {
            wk->pageStep = PAGE_STEP_INPUT;
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, TRUE);
        }
        break;
    case PAGE_STEP_INPUT:
        PkwdBook_ListInput(wk);
        break;
    case PAGE_STEP_WAIT_RETURN:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RETURN)) {
            wk->pageState++;
            wk->nextPageState = PAGE_STATE_NONE;
        }
        break;
    case PAGE_STEP_WAIT_RIGHT:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RIGHT)) {
            wk->pageStep = PAGE_STEP_INPUT;
            wk->pageState++;
        }
        break;
    case PAGE_STEP_WAIT_LEFT:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_LEFT)) {
            wk->pageStep = PAGE_STEP_INPUT;
            wk->pageState++;
        }
        break;
    case PAGE_STEP_LIST_SCROLL:
        if (PkwdBook_UpdateList(wk) == FRAMELIST_NONE) {
            PkwdBook_SaveListPos(wk);
            wk->flags |= FLAG_MOVIE_CHANGED;
            wk->pageStep = PAGE_STEP_LIST_WAIT_ARROW;
        }
        break;
    case PAGE_STEP_LIST_WAIT_ARROW:
        if (wk->flags & FLAG_PAGE_UP) {
            if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_LEFT)) {
                wk->pageStep = PAGE_STEP_INPUT;
                wk->flags &= ~(FLAG_PAGE_UP | FLAG_PAGE_DOWN);
                PkwdBook_SetArrows(wk, 0, wk->movieCount - 1, wk->selected);
            }
        } else if (wk->flags & FLAG_PAGE_DOWN) {
            if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RIGHT)) {
                wk->pageStep = PAGE_STEP_INPUT;
                wk->flags &= ~(FLAG_PAGE_UP | FLAG_PAGE_DOWN);
                PkwdBook_SetArrows(wk, 0, wk->movieCount - 1, wk->selected);
            }
        } else {
            wk->pageStep = PAGE_STEP_INPUT;
            PkwdBook_SetArrows(wk, 0, wk->movieCount - 1, wk->selected);
        }
        break;
    case PAGE_STEP_LIST_WAIT_PRINT:
        if (func_02021c0c(wk->printQueue) == TRUE) {
            if (wk->flags & FLAG_SHOW_MOVIE) {
                PkwdBook_ShowMovie(wk, TRUE);
                wk->flags &= ~FLAG_SHOW_MOVIE;
            }
            wk->pageStep = PAGE_STEP_LIST_BLINK;
            BlinkPalAnm_StartBlink(FrameList_GetBlinkAnm(wk->list));
        }
        break;
    case PAGE_STEP_LIST_BLINK:
        BlinkPalAnm_Main(FrameList_GetBlinkAnm(wk->list));
        if (BlinkPalAnm_IsBlinkEnd(FrameList_GetBlinkAnm(wk->list)) == TRUE) {
            BlinkPalAnm_SetAnimeCount(FrameList_GetBlinkAnm(wk->list), 0);
            BlinkPalAnm_Main(FrameList_GetBlinkAnm(wk->list));
            PkwdBook_PrintQuestion(wk);
            PkwdBook_CreateMenu(wk);
            wk->pageStep = PAGE_STEP_LIST_MENU;
        }
        break;
    case PAGE_STEP_LIST_MENU: {
        s32 result = PkwdBook_UpdateMenu(wk);

        if (result != -1) {
            if (result == MENU_FILM) {
                wk->pageState++;
                wk->nextPageState = PAGE_STATE_NONE;
                wk->result = PkwdBook_GetMovie(wk, wk->selected);
            } else if (result == MENU_DETAILS) {
                s32 movie;

                wk->nextPageState = PAGE_STATE_ENTER_CAST;
                wk->pageState++;
                wk->flags |= FLAG_DETAILS;
                movie = PkwdBook_GetMovie(wk, wk->selected);
                PkwdBook_LoadCast(wk, movie);
                wk->castPage = 0;
                PkwdBook_LoadTrainerClass(wk, movie);
            } else {
                wk->pageStep = PAGE_STEP_INPUT;
                PkwdBook_ShowMovie(wk, FALSE);
                BlinkPalAnm_InitAnime(FrameList_GetBlinkAnm(wk->list));
                if (func_0203d554() == TRUE) {
                    FrameList_SetRowPalette(wk->list, (u16)wk->cursorPos, 0);
                }
            }
            PkwdBook_ClearQuestion(wk);
            PkwdBook_FreeMenu(wk);
        }
        break;
    }
    case PAGE_STEP_END:
        break;
    }
}

static void PkwdBook_LeaveList(PkwdBookWork *wk) {
    PkwdBook_ChangePage(wk);
    PkwdBook_ClearList(wk);
}

static void PkwdBook_ListInput(PkwdBookWork *wk) {
    s32 action = 0;
    BOOL cancel = FALSE;
    s32 result;

    if (!func_02021c0c(wk->printQueue)) {
        return;
    }
    result = PkwdBook_HandleListResult(wk, PkwdBook_UpdateList(wk));
    if (result == 1) {
        action = 2;
    } else if (result == 2) {
        action = 1;
    }
    if (wk->flags & FLAG_MOVIE_CHANGED) {
        action = 1;
        wk->flags &= ~FLAG_MOVIE_CHANGED;
    }
    if (action == 0) {
        s32 touch = PkwdBook_ReadTouch(wk, wk->page);

        if (touch == 0) {
            cancel = TRUE;
        }
        if (!cancel) {
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                cancel = TRUE;
            }
        }
        if (cancel == TRUE) {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RETURN, 9);
            wk->pageStep = PAGE_STEP_WAIT_RETURN;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            if (touch == 0) {
                func_0203d564(TRUE);
            } else {
                func_0203d564(FALSE);
            }
        }
    }
    if (action == 1) {
        PkwdBookSeries series;

        PkwdBook_PrintLabels(wk, FALSE, PAGE_LIST);
        PkwdBook_PrintMovie(wk, TRUE);
        series = wk->movies[wk->selected].series;
        PkwdBook_TransferSeriesPalette(wk, series);
        PkwdBook_LoadPoster(wk, PkwdBook_GetPosterGraphicIdx(wk, wk->movies[wk->selected].movie), TRUE);
    }
    if (!(wk->flags & FLAG_CURSOR_SHOWN)) {
        FrameList_ShowCursor(wk->list);
        wk->flags |= FLAG_CURSOR_SHOWN;
    }
    PkwdBook_UpdateScrollBar(wk);
}

// Prints the title, the plot and the mission of the movie under the cursor on the upper screen
static void PkwdBook_PrintMovie(PkwdBookWork *wk, BOOL keepLabels) {
    s32 movie = PkwdBook_GetMovie(wk, wk->selected);
    int i;

    if (!keepLabels) {
        PkwdBook_PrintLabels(wk, TRUE, PAGE_LIST);
    }
    for (i = 0; i < 3; i++) {
        StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], movie + sPkwdBookMovieMessages[i]);
        u32 align;
        s32 x;
        u16 color;

        if (i == 0) {
            align = ALIGN_CENTER;
            x = 112;
            color = PRINT_COLOR(1, 3, 0);
        } else {
            align = ALIGN_LEFT;
            x = 0;
            color = PRINT_COLOR(1, 2, 0);
        }
        PkwdBook_PrintText(wk, FALSE, i + 3, x, 0, strbuf, align, color, FALSE);
        GFL_StrBufFree(strbuf);
    }
}

// Has the touch bar's arrows show whether the current item is the first or the last
static void PkwdBook_SetArrows(PkwdBookWork *wk, s32 first, s32 last, s32 current) {
    if (last >= 0) {
        if (first >= current) {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_LEFT, 18);
        } else {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_LEFT, 4);
        }
        if (current >= last) {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RIGHT, 19);
        } else {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RIGHT, 5);
        }
    }
}

static void PkwdBook_EnterCast(PkwdBookWork *wk) {
    wk->pageState = PAGE_STATE_CAST;
    wk->pageStep = PAGE_STEP_INPUT;
    wk->page = PAGE_CAST;
    PkwdBook_CreateWindows(wk, TRUE, PAGE_CAST);
    PkwdBook_PrintCast(wk, FALSE);
    PkwdBook_HideActors(wk, TRUE);
    PkwdBook_ShowCast(wk, 0);
    PkwdBook_PlaceArrows(wk, PAGE_CAST);
    PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_LEFT, 4);
    PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RIGHT, 5);
    wk->flags |= FLAG_SCROLL_PENDING;
}

static void PkwdBook_RunCast(PkwdBookWork *wk) {
    switch (wk->pageStep) {
    case PAGE_STEP_START:
        wk->pageStep = PAGE_STEP_INPUT;
        // fallthrough
    case PAGE_STEP_INPUT:
        PkwdBook_CastInput(wk);
        break;
    case PAGE_STEP_WAIT_RETURN:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RETURN)) {
            wk->pageState++;
            wk->pageStep = PAGE_STEP_INPUT;
            wk->flags &= ~FLAG_DETAILS;
        }
        break;
    case PAGE_STEP_WAIT_RIGHT:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RIGHT)) {
            wk->pageStep = PAGE_STEP_INPUT;
            wk->pageState++;
            PkwdBook_TurnCastPage(wk, FALSE);
        }
        break;
    case PAGE_STEP_WAIT_LEFT:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_LEFT)) {
            wk->pageStep = PAGE_STEP_INPUT;
            wk->pageState++;
            PkwdBook_TurnCastPage(wk, TRUE);
        }
        break;
    case PAGE_STEP_END:
        break;
    }
}

static void PkwdBook_LeaveCast(PkwdBookWork *wk) {
    PkwdBook_ChangePage(wk);
}

static void PkwdBook_CastInput(PkwdBookWork *wk) {
    s32 action = 0;
    s32 touch;

    if (!func_02021c0c(wk->printQueue)) {
        return;
    }
    touch = PkwdBook_ReadTouch(wk, wk->page);
    if (touch == 0) {
        action = 1;
    } else if (touch == 1) {
        action = 2;
    } else if (touch == 2) {
        action = 3;
    }
    if (action == 0) {
        if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            action = 1;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            action = 2;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            action = 3;
        }
    }
    if (action == 1) {
        wk->nextPageState = PAGE_STATE_ENTER_POKEMON;
        PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_LEFT, 12);
        wk->pageStep = PAGE_STEP_WAIT_LEFT;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    } else if (action == 2) {
        wk->nextPageState = PAGE_STATE_ENTER_POKEMON;
        PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RIGHT, 13);
        wk->pageStep = PAGE_STEP_WAIT_RIGHT;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    } else if (action == 3) {
        wk->nextPageState = PAGE_STATE_ENTER_LIST;
        PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RETURN, 9);
        wk->pageStep = PAGE_STEP_WAIT_RETURN;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (touch == 2) {
            func_0203d564(TRUE);
        } else {
            func_0203d564(FALSE);
        }
    }
}

// Prints the cast page's texts: the names of the player's role and of the costar
static void PkwdBook_PrintCast(PkwdBookWork *wk, BOOL keepLabels) {
    u32 gender = getTrainerGender(wk->param->playerInfo);

    if (!keepLabels) {
        s32 movie;
        StrBuf *costar;
        StrBuf *buf;

        PkwdBook_PrintLabels(wk, TRUE, PAGE_CAST);
        movie = PkwdBook_GetMovie(wk, wk->selected);
        PkwdBook_PrintText(wk, TRUE, 2, 1, 0, wk->roleNames[movie], ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
        if (gender == 0) {
            costar = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_COSTARS], movie + EventPassPowerStartmenu_Text_BrycenMan);
        } else {
            costar =
                GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_COSTARS], movie + EventPassPowerStartmenu_Text_BrycenMan_5);
        }
        buf = GFL_StrBufCreate(16, wk->heapId);
        GFL_StrBufUncompress(buf, costar);
        PkwdBook_PrintText(wk, TRUE, 3, 1, 0, buf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
        GFL_StrBufFree(buf);
        GFL_StrBufFree(costar);
    }
}

// Shows the cast page's actors: the icons of the cast, the costar and the page number
static void PkwdBook_ShowCast(PkwdBookWork *wk, u32 unused) {
    int i;

    for (i = 0; i < CAST_COUNT; i++) {
        BOOL visible;

        if (i < wk->castCount) {
            PkwdBook_SetPokeIcon(wk, i, wk->cast[i].species, wk->cast[i].sex);
            visible = TRUE;
        } else {
            visible = FALSE;
        }
        PkwdBook_SetActorVisible(wk, GROUP_POKE_ICON, i, visible);
    }
    PkwdBook_SetTrainer(wk, wk->trainerClass);
    PkwdBook_SetActorVisible(wk, GROUP_TRAINER, 0, TRUE);
    PkwdBook_PrintPageNumber(wk);
    PkwdBook_SetActorVisible(wk, GROUP_MARK, MARK_PAGE, TRUE);
}

static void PkwdBook_EnterPokemon(PkwdBookWork *wk) {
    wk->pageState = PAGE_STATE_POKEMON;
    wk->pageStep = PAGE_STEP_INPUT;
    wk->page = PAGE_POKEMON;
    PkwdBook_CreateWindows(wk, TRUE, PAGE_POKEMON);
    PkwdBook_PrintPokemon(wk, FALSE);
    PkwdBook_HideActors(wk, TRUE);
    PkwdBook_ShowPokemon(wk, 0);
    wk->flags |= FLAG_SCROLL_PENDING;
}

static void PkwdBook_RunPokemon(PkwdBookWork *wk) {
    switch (wk->pageStep) {
    case PAGE_STEP_START:
        wk->pageStep = PAGE_STEP_INPUT;
        // fallthrough
    case PAGE_STEP_INPUT:
        PkwdBook_PokemonInput(wk);
        break;
    case PAGE_STEP_WAIT_RETURN:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RETURN)) {
            wk->pageState++;
            wk->pageStep = PAGE_STEP_INPUT;
            wk->flags &= ~FLAG_DETAILS;
        }
        break;
    case PAGE_STEP_WAIT_RIGHT:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_RIGHT)) {
            wk->pageStep = PAGE_STEP_INPUT;
            wk->pageState++;
            PkwdBook_TurnCastPage(wk, FALSE);
            if (wk->castPage != 0) {
                wk->nextPageState = PAGE_STATE_ENTER_POKEMON;
            }
        }
        break;
    case PAGE_STEP_WAIT_LEFT:
        if (!PkwdBook_IsActorAnimating(wk, GROUP_BAR, BAR_LEFT)) {
            wk->pageStep = PAGE_STEP_INPUT;
            wk->pageState++;
            PkwdBook_TurnCastPage(wk, TRUE);
            if (wk->castPage != 0) {
                wk->nextPageState = PAGE_STATE_ENTER_POKEMON;
            }
        }
        break;
    case PAGE_STEP_END:
        break;
    }
}

static void PkwdBook_LeavePokemon(PkwdBookWork *wk) {
    PkwdBook_ChangePage(wk);
}

static void PkwdBook_PokemonInput(PkwdBookWork *wk) {
    s32 action = 0;
    s32 touch;

    if (!func_02021c0c(wk->printQueue)) {
        return;
    }
    touch = PkwdBook_ReadTouch(wk, wk->page);
    if (touch == 0) {
        action = 1;
    } else if (touch == 1) {
        action = 2;
    } else if (touch == 2) {
        action = 3;
    }
    if (action == 0) {
        if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            action = 1;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            action = 2;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            action = 3;
        }
    }
    if (action == 1) {
        wk->nextPageState = PAGE_STATE_ENTER_CAST;
        PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_LEFT, 12);
        wk->pageStep = PAGE_STEP_WAIT_LEFT;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    } else if (action == 2) {
        wk->nextPageState = PAGE_STATE_ENTER_CAST;
        PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RIGHT, 13);
        wk->pageStep = PAGE_STEP_WAIT_RIGHT;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    } else if (action == 3) {
        wk->nextPageState = PAGE_STATE_ENTER_LIST;
        PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RETURN, 9);
        wk->pageStep = PAGE_STEP_WAIT_RETURN;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (touch == 2) {
            func_0203d564(TRUE);
        } else {
            func_0203d564(FALSE);
        }
    }
}

// Prints the texts of the page of the cast's current Pokémon
static void PkwdBook_PrintPokemon(PkwdBookWork *wk, BOOL keepLabels) {
    StrBuf *buf;
    u32 value;
    s32 sexMessage;
    u32 color;
    StrBuf *nameFormat;
    StrBuf *levelFormat;
    StrBuf *itemFormat;
    StrBuf *abilityFormat;
    int i;

    if (!keepLabels) {
        PkwdBook_PrintLabels(wk, TRUE, PAGE_POKEMON);
    }
    PokeParty_GetPkm(wk->party, wk->castIdx);
    buf = GFL_StrBufCreate(0x80, wk->heapId);
    value = wk->cast[wk->castIdx].species;
    nameFormat = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[0]);
    WordSet_LoadSpeciesName(wk->wordSet, 0, (u16)value);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, nameFormat);
    PkwdBook_PrintText(wk, TRUE, 1, 0, 0, buf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
    GFL_StrBufFree(nameFormat);
    sexMessage = 1;
    color = PRINT_COLOR(6, 5, 0);
    if (wk->cast[wk->castIdx].sex == 1) {
        sexMessage = 2;
        color = PRINT_COLOR(4, 3, 0);
    }
    if (wk->cast[wk->castIdx].sex != 2) {
        StrBuf *mark = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[sexMessage]);

        PkwdBook_PrintText(wk, TRUE, 1, 62, 0, mark, ALIGN_LEFT, (u16)color, TRUE);
        GFL_StrBufFree(mark);
    }
    value = wk->cast[wk->castIdx].level;
    levelFormat = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[3]);
    WordSetNumber(wk->wordSet, 0, value, 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, levelFormat);
    PkwdBook_PrintText(wk, TRUE, 2, 0, 0, buf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
    GFL_StrBufFree(levelFormat);
    value = wk->cast[wk->castIdx].item;
    itemFormat = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[4]);
    loadItemNameToStrbuf(wk->wordSet, 0, value);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, itemFormat);
    PkwdBook_PrintText(wk, TRUE, 4, 0, 0, buf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
    GFL_StrBufFree(itemFormat);
    for (i = 0; i < MOVE_COUNT; i++) {
        u32 move = wk->cast[wk->castIdx].moves[i];

        if (move != 0) {
            StrBuf *moveFormat = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[6 + i]);

            loadMoveNameToStrbuf(wk->wordSet, 0, move);
            GFL_WordSetFormatStrbuf(wk->wordSet, buf, moveFormat);
            PkwdBook_PrintText(wk, TRUE, i + 5, 0, 0, buf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
            GFL_StrBufFree(moveFormat);
        } else {
            PkwdBook_FillWindow(wk, TRUE, i + 5, 0);
        }
    }
    value = wk->cast[wk->castIdx].ability;
    abilityFormat = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[5]);
    loadAbilityNameToStrbuf(wk->wordSet, 0, value);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, abilityFormat);
    PkwdBook_PrintText(wk, TRUE, 10, 0, 0, buf, ALIGN_LEFT, PRINT_COLOR(1, 2, 0), FALSE);
    GFL_StrBufFree(abilityFormat);
    GFL_StrBufFree(buf);
}

// Shows the actors of the page of the cast's current Pokémon: its icon, its types, those of its moves and the page
// number
static void PkwdBook_ShowPokemon(PkwdBookWork *wk, u32 unused) {
    u32 type1;
    u32 type2;
    BOOL visible;
    int i;

    PokeParty_GetPkm(wk->party, wk->castIdx);
    PkwdBook_SetPokeIcon(wk, CAST_COUNT, wk->cast[wk->castIdx].species, wk->cast[wk->castIdx].sex);
    PkwdBook_SetActorVisible(wk, GROUP_POKE_ICON, CAST_COUNT, TRUE);
    type1 = wk->cast[wk->castIdx].type1;
    PkwdBook_SetTypeIcon(wk, 0, 0, type1);
    PkwdBook_SetActorVisible(wk, GROUP_TYPE_ICON, 0, TRUE);
    visible = TRUE;
    type2 = wk->cast[wk->castIdx].type2;
    if (type1 != type2) {
        PkwdBook_SetTypeIcon(wk, 1, 0, type2);
    } else {
        visible = FALSE;
    }
    PkwdBook_SetActorVisible(wk, GROUP_TYPE_ICON, 1, visible);
    for (i = 0; i < MOVE_COUNT; i++) {
        if (wk->cast[wk->castIdx].moves[i] != 0) {
            PkwdBook_SetTypeIcon(wk, i + 2, 1, wk->cast[wk->castIdx].moveTypes[i]);
            visible = TRUE;
        } else {
            visible = FALSE;
        }
        PkwdBook_SetActorVisible(wk, GROUP_TYPE_ICON, i + 2, visible);
    }
    PkwdBook_PrintPageNumber(wk);
    PkwdBook_SetActorVisible(wk, GROUP_MARK, MARK_PAGE, TRUE);
}

// Hides the actors of the pages, and the page number unless keepText is set
static void PkwdBook_HideActors(PkwdBookWork *wk, BOOL keepText) {
    int i;

    for (i = 0; i < POKE_ICON_COUNT; i++) {
        PkwdBook_SetActorVisible(wk, GROUP_POKE_ICON, i, FALSE);
    }
    for (i = 0; i < TYPE_ICON_COUNT; i++) {
        PkwdBook_SetActorVisible(wk, GROUP_TYPE_ICON, i, FALSE);
    }
    PkwdBook_SetActorVisible(wk, GROUP_TRAINER, 0, FALSE);
    for (i = 0; i < MARK_ACTOR_COUNT; i++) {
        PkwdBook_SetActorVisible(wk, GROUP_MARK, i, FALSE);
    }
    if (!keepText) {
        for (i = 0; i < TEXT_ACTOR_COUNT; i++) {
            PkwdBook_SetTextActorVisible(wk, i, FALSE);
        }
    }
}

// Runs the page's state. Returns whether the book is still open
static BOOL PkwdBook_RunPage(PkwdBookWork *wk) {
    BOOL running = TRUE;

    if (wk->pageState >= PAGE_STATE_COUNT) {
        return FALSE;
    }
    sPkwdBookPageFuncs[wk->pageState](wk);
    if (wk->nextPageState == PAGE_STATE_NONE) {
        running = FALSE;
    }
    return running;
}

static void PkwdBook_ChangePage(PkwdBookWork *wk) {
    if (wk->nextPageState != PAGE_STATE_NONE) {
        wk->pageState = wk->nextPageState;
    }
}

// Turns to the next page of the details, or to the one before
static void PkwdBook_TurnCastPage(PkwdBookWork *wk, BOOL back) {
    if (!back) {
        wk->castPage++;
        if (wk->castPage >= wk->castCount + 1) {
            wk->castPage = 0;
        }
    } else {
        wk->castPage--;
        if (wk->castPage < 0) {
            wk->castPage = wk->castCount;
        }
    }
    if (wk->castPage == 0) {
        wk->castIdx = 0;
    } else {
        wk->castIdx = wk->castPage - 1;
    }
}

// Draws the page number of the details and the count of their pages, with a slash between
static void PkwdBook_PrintPageNumber(PkwdBookWork *wk) {
    StrBuf *buf = GFL_StrBufCreate(0x40, wk->heapId);
    StrBuf *slash;

    GFL_WordSetFormatNumber(buf, wk->castPage + 1, 1, 2, TRUE);
    PkwdBook_PrintTextActor(wk, 0, buf, 0, 0);
    GFL_WordSetFormatNumber(buf, wk->castCount + 1, 1, 2, TRUE);
    PkwdBook_PrintTextActor(wk, 2, buf, 0, 0);
    GFL_StrBufFree(buf);
    slash = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookPokemonMessages[10]);
    PkwdBook_PrintTextActor(wk, 1, slash, 0, 0);
    GFL_StrBufFree(slash);
}

// Puts the touch bar's arrows where a page has them
static void PkwdBook_PlaceArrows(PkwdBookWork *wk, u32 page) {
    s16 positions[2];
    ClActorPos pos;
    s16 i;

    positions[0] = sPkwdBookArrowPositions[page][0];
    positions[1] = sPkwdBookArrowPositions[page][1];
    for (i = 0; i < 2; i++) {
        u32 positionIdx = positions[i] * 2;

        pos.x = PkwdBook_GetPosition(wk, positionIdx);
        pos.y = PkwdBook_GetPosition(wk, positionIdx + 1);
        func_0204c140(wk->barActors[BAR_LEFT + i], &pos, CLACT_SURFACE_SUB);
    }
}

// The button of a page's touch bar that was touched this frame, or TOUCH_RECT_NONE
static s32 PkwdBook_ReadTouch(PkwdBookWork *wk, u32 page) {
    return func_0203da0c(sPkwdBookTouchRects[page]);
}

// Makes the party of a movie's cast
static void PkwdBook_LoadCast(PkwdBookWork *wk, u32 movie) {
    PkwdBook_FreeCast(wk);
    wk->party = PokeParty_Create(wk->heapId);
    EventPokewood_CreateCastPartyFromSave(wk->param->playerInfo, wk->party, wk->heapId, movie, 1);
    PkwdBook_ReadCast(wk);
}

static void PkwdBook_FreeCast(PkwdBookWork *wk) {
    if (wk->party != NULL) {
        GFL_HeapFree(wk->party);
        wk->party = NULL;
    }
}

// Reads what the pages show of the cast's Pokémon
static void PkwdBook_ReadCast(PkwdBookWork *wk) {
    int i;
    int j;

    wk->castCount = PokeParty_GetPkmCount(wk->party);
    for (i = 0; i < CAST_COUNT; i++) {
        wk->cast[i].species = 0;
        wk->cast[i].sex = 0;
        wk->cast[i].level = 0;
        wk->cast[i].item = 0;
        wk->cast[i].type1 = 0;
        wk->cast[i].type2 = 0;
        for (j = 0; j < MOVE_COUNT; j++) {
            wk->cast[i].moves[j] = 0;
            wk->cast[i].moveTypes[j] = 0;
        }
    }
    for (i = 0; i < wk->castCount; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(wk->party, i);

        wk->cast[i].species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        wk->cast[i].sex = PokeParty_GetSex(pkm);
        wk->cast[i].level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
        wk->cast[i].item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
        wk->cast[i].ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
        wk->cast[i].type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
        wk->cast[i].type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
        for (j = 0; j < MOVE_COUNT; j++) {
            wk->cast[i].moves[j] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + j, NULL);
            if (wk->cast[i].moves[j] != 0) {
                wk->cast[i].moveTypes[j] = PML_MoveGetParam(wk->cast[i].moves[j], MOVE_PARAM_TYPE);
            }
        }
    }
}

// Lists the movies that the player can film
static void PkwdBook_ListMovies(PkwdBookWork *wk) {
    int count = 0;
    int i;

    for (i = 0; i < LIST_MOVIE_COUNT; i++) {
        if (func_02011100(wk->param->save, i) == TRUE) {
            wk->movies[count].movie = i;
            if (!func_02011118(wk->param->save, i)) {
                wk->movies[count].isNew = TRUE;
            }
            wk->movies[count].series = sPkwdBookSeries[i] & 3;
            count++;
        }
    }
    wk->movieCount = count;
}

// The movie of an item of the list, or PKWD_BOOK_CANCEL past its end
static s32 PkwdBook_GetMovie(PkwdBookWork *wk, s32 index) {
    if (index >= wk->movieCount) {
        return PKWD_BOOK_CANCEL;
    }
    return wk->movies[index].movie;
}

// Has the VBlank send the palettes of a series to the upper screen
static void PkwdBook_TransferSeriesPalette(PkwdBookWork *wk, u32 series) {
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 0, wk->palettes[series]->rawData, 5 * 0x20);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 5 * 0x20, wk->textPalettes[series]->rawData,
                                       3 * 0x20);
}

static void PkwdBook_LoadSeriesPalettes(PkwdBookWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);
    int i;

    for (i = 0; i < SERIES_COUNT; i++) {
        wk->paletteFiles[i] = GFL_G2DIOReadNCLRArc(arc, sPkwdBookSeriesPalettes[i][0], &wk->palettes[i], wk->heapId);
    }
    for (i = 0; i < SERIES_COUNT; i++) {
        wk->textPaletteFiles[i] =
            GFL_G2DIOReadNCLRArc(arc, sPkwdBookSeriesPalettes[i][1], &wk->textPalettes[i], wk->heapId);
    }
    GFL_ArcToolFree(arc);
}

static void PkwdBook_FreeSeriesPalettes(PkwdBookWork *wk) {
    int i;

    for (i = 0; i < SERIES_COUNT; i++) {
        if (wk->paletteFiles[i] != NULL) {
            GFL_HeapFree(wk->paletteFiles[i]);
            wk->paletteFiles[i] = NULL;
        }
        if (wk->textPaletteFiles[i] != NULL) {
            GFL_HeapFree(wk->textPaletteFiles[i]);
            wk->textPaletteFiles[i] = NULL;
        }
    }
}

// Shows the upper screen's poster and texts, which stay hidden in touch mode unless force is set
static void PkwdBook_ShowMovie(PkwdBookWork *wk, BOOL force) {
    BOOL visible = TRUE;

    if (!force) {
        if (func_0203d554() == TRUE) {
            visible = FALSE;
        }
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, visible);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, visible);
    } else {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, visible);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, visible);
    }
}

static void PkwdBook_CreateList(PkwdBookWork *wk) {
    FrameListSetup setup = sPkwdBookListSetup;
    ArcTool *arc;
    int i;

    setup.work = wk;
    PkwdBook_FreeList(wk);
    arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_BOOK_GRA, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 20, 6, 0, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_SUB_BG, 0, 0x20, wk->heapId);
    setup.callbacks = &sPkwdBookListCallbacks;
    setup.count = wk->movieCount;
    setup.cursorPos = wk->cursorPos;
    setup.scroll = wk->scroll;
    if (wk->movieCount <= LIST_ROWS) {
        setup.touch = sPkwdBookShortListTouch;
    }
    wk->list = FrameList_Create(&setup, wk->heapId);
    FrameList_LoadScreen(wk->list, arc, 29, FALSE, 0);
    FrameList_LoadScreen(wk->list, arc, 30, FALSE, 1);
    FrameList_LoadScreen(wk->list, arc, 31, FALSE, 2);
    FrameList_LoadScreen(wk->list, arc, 32, FALSE, 3);
    FrameList_LoadCursorPalette(wk->list, arc, 0, 0, 1);
    GFL_ArcToolFree(arc);
    for (i = 0; i < setup.count; i++) {
        FrameList_AddItem(wk->list, wk->movies[i].series, wk->movies[i].movie + PokewoodLines_Text_BrycenMan);
    }
    wk->flags &= ~FLAG_LIST_DRAWN;
    if (!FrameList_Draw(wk->list)) {
        wk->flags |= FLAG_LIST_DRAWN;
    }
    GFL_BGSysResetStdPalette(4, GX_RGB(29, 28, 25));
}

static s32 PkwdBook_UpdateList(PkwdBookWork *wk) {
    s32 result = FRAMELIST_NONE;

    if (wk->list != NULL) {
        result = FrameList_Main(wk->list);
    }
    return result;
}

// Acts on what the list returned. Returns 1 when the cursor moved or the list scrolled, 2 when a movie was picked
// and 3 for the list's -12, else 0
static s32 PkwdBook_HandleListResult(PkwdBookWork *wk, u32 result) {
    s32 action = 0;
    BOOL moved = FALSE;

    switch (result) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        action = 2;
        break;
    case -7:
    case -5:
        wk->flags |= FLAG_PAGE_UP;
        action = 1;
        break;
    case -6:
    case -4:
        wk->flags |= FLAG_PAGE_DOWN;
        action = 1;
        break;
    case -3:
    case -2:
        action = 1;
        moved = TRUE;
        break;
    case -9:
    case -10:
    case -11:
        action = 1;
        break;
    case -12:
        action = 3;
        break;
    }
    if (action == 1) {
        wk->pageStep = PAGE_STEP_LIST_SCROLL;
        if (wk->flags & FLAG_PAGE_UP) {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_LEFT, 12);
        } else if (wk->flags & FLAG_PAGE_DOWN) {
            PkwdBook_PlayAnim(wk, GROUP_BAR, BAR_RIGHT, 13);
        }
        if (moved == TRUE) {
            FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
            PkwdBook_UpdateNewMarks(wk, TRUE);
        }
    } else if (action == 2) {
        PkwdBook_SaveListPos(wk);
        wk->pageStep = PAGE_STEP_LIST_WAIT_PRINT;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        wk->flags |= FLAG_SHOW_MOVIE;
        FrameList_SetRowPalette(wk->list, (u16)wk->cursorPos, 1);
        PkwdBook_SetArrows(wk, 0, wk->movieCount - 1, wk->selected);
    }
    if (action != 0 && action != 2) {
        PkwdBook_ShowMovie(wk, FALSE);
    }
    return action;
}

static void PkwdBook_FreeList(PkwdBookWork *wk) {
    if (wk->list != NULL) {
        FrameList_Free(wk->list);
        wk->list = NULL;
    }
}

static void PkwdBook_ClearList(PkwdBookWork *wk) {
    GFL_BGSysClearScr(6);
    GFL_BGSysQueueScrLoad(6);
    PkwdBook_SaveListPos(wk);
}

static void PkwdBook_RecreateList(PkwdBookWork *wk) {
    if (wk->list != NULL) {
        PkwdBook_CreateList(wk);
    }
}

static void PkwdBook_SaveListPos(PkwdBookWork *wk) {
    if (wk->list != NULL) {
        wk->selected = FrameList_GetSelected(wk->list);
        wk->cursorPos = FrameList_GetCursor(wk->list);
        wk->scroll = FrameList_GetScroll(wk->list);
    }
}

static void PkwdBookList_Print(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PkwdBookWork *wk = work;

    PkwdBook_PrintListItem(wk, index, window, wk->msgData[MSG_LINES]);
}

static void PkwdBookList_Select(void *work, u32 index, BOOL moved) {
}

static void PkwdBookList_Scroll(void *work, s16 delta) {
    PkwdBookWork *wk = work;

    PkwdBook_UpdateNewMarks(wk, FALSE);
    PkwdBook_ScrollNewMarks(wk, delta);
}

// Prints an item of the list, whose value is the message of its movie's title
static void PkwdBook_PrintListItem(PkwdBookWork *wk, u32 index, PrintWindow *window, MsgData *msgData) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    PkwdBook_PrintItemTitle(wk, window, queue, msgData, FrameList_GetValue(wk->list, index));
}

// Prints a message in the middle of a window
static void PkwdBook_PrintCentered(PkwdBookWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                   u32 messageId, u16 color, s32 y) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, messageId);
    s32 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
    s32 windowWidth = BmpWin_GetSizeX(window->window) * 8;

    PrintWindow_Print(window, queue, (windowWidth - width) / 2 + 3, y, strbuf, wk->font, color);
    GFL_StrBufFree(strbuf);
}

static void PkwdBook_PrintItemTitle(PkwdBookWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                    u32 messageId) {
    PkwdBook_PrintCentered(wk, window, queue, msgData, messageId, PRINT_COLOR(15, 14, 0), 4);
}

// Asks what to do with the movie that was picked, in a message box on the lower screen
static void PkwdBook_PrintQuestion(PkwdBookWork *wk) {
    s32 movie;
    StrBuf *buf;
    StrBuf *format;

    LoadSysMsgBox(4, 1, 9, 0, wk->heapId);
    BmpWin_DrawFrame(wk->subWindows[0].window, FALSE, 1, 9);
    BmpWin_FlushMap(wk->subWindows[0].window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->subWindows[0].window));
    GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindows[0].window), 15);
    BmpWin_FlushChar(wk->subWindows[0].window);
    PkwdBook_FillWindow(wk, TRUE, 0, 15);
    movie = PkwdBook_GetMovie(wk, wk->selected);
    buf = GFL_StrBufCreate(0x80, wk->heapId);
    loadPokewoodLineToStrbuf(wk->wordSet, 0, movie);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_LINES], sPkwdBookMenuMessages[0]);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, format);
    PrintWindow_Print(&wk->subWindows[0], wk->printQueue, 0, 0, buf, wk->font, PRINT_COLOR(1, 2, 15));
    GFL_StrBufFree(format);
    GFL_StrBufFree(buf);
}

static void PkwdBook_ClearQuestion(PkwdBookWork *wk) {
    GFL_BGSysClearBG(4);
}

// Creates the menu of a movie: to film it, to see its details or to quit
static void PkwdBook_CreateMenu(PkwdBookWork *wk) {
    AppTaskMenuInit init;
    int i;

    init.heapId = wk->heapId;
    init.itemCount = MENU_ITEM_COUNT;
    init.items = wk->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 13;
    init.height = 3;
    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        wk->menuItems[i].str = GFL_StrBufCreate(100, wk->heapId);
        GFL_MsgDataLoadStrbuf(wk->msgData[MSG_LINES], sPkwdBookMenuMessages[1 + i], wk->menuItems[i].str);
        wk->menuItems[i].color = PRINT_COLOR(14, 15, 0);
        if (i == MENU_QUIT) {
            wk->menuItems[i].type = APP_TASKMENU_ITEM_RETURN;
        } else {
            wk->menuItems[i].type = 0;
        }
    }
    wk->menu = AppTaskMenu_Create(&init, wk->menuRes);
    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        GFL_StrBufFree(wk->menuItems[i].str);
    }
}

static void PkwdBook_FreeMenu(PkwdBookWork *wk) {
    if (wk->menu != NULL) {
        AppTaskMenu_Free(wk->menu);
        wk->menu = NULL;
    }
}

// The item of the menu that was picked, once it has flashed, or -1
static s32 PkwdBook_UpdateMenu(PkwdBookWork *wk) {
    if (wk->menu == NULL) {
        return -1;
    }
    AppTaskMenu_Update(wk->menu);
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        return AppTaskMenu_GetCursorPos(wk->menu);
    }
    return -1;
}

// Reads the trainer class of a movie's costar for the player's sex
static void PkwdBook_LoadTrainerClass(PkwdBookWork *wk, u32 movie) {
    s32 gender = 0;
    PkwdBookMovieFile *file;

    if (getTrainerGender(wk->param->playerInfo) == 1) {
        gender = 1;
    }
    file = GFL_ArcSysReadHeapNew(ARCID_POKEWOOD_MOVIE, movie, wk->heapId);
    wk->trainerClass = file->trainerClass[gender][0];
    GFL_HeapFree(file);
}

// Shows the list's scroll bar when the list scrolls
static void PkwdBook_ShowScrollBar(PkwdBookWork *wk) {
    BOOL visible = TRUE;

    if (wk->movieCount <= LIST_ROWS) {
        visible = FALSE;
    }
    PkwdBook_SetActorVisible(wk, GROUP_MARK, MARK_SCROLL_BAR, visible);
}

static void PkwdBook_UpdateScrollBar(PkwdBookWork *wk) {
    ClActorPos pos;

    if (wk->movieCount > LIST_ROWS) {
        func_0204c178(wk->markActors[MARK_SCROLL_BAR], &pos, CLACT_SURFACE_SUB);
        pos.y = FrameList_ClampBarPos(wk->list, pos.y);
        if (pos.y < 8) {
            pos.y = 8;
        } else if (pos.y > 160) {
            pos.y = 160;
        }
        func_0204c140(wk->markActors[MARK_SCROLL_BAR], &pos, CLACT_SURFACE_SUB);
    }
}

// Shows the marks of the list's rows whose movie was not filmed yet, a row off when the list starts scrolling. reset
// puts them on their rows
static void PkwdBook_UpdateNewMarks(PkwdBookWork *wk, BOOL reset) {
    ClActorPos pos;
    s32 scroll;
    s32 dir;
    int i;

    wk->newMarks = 0;
    if (reset == TRUE) {
        wk->lastScroll = -1;
    }
    scroll = FrameList_GetScroll(wk->list);
    if (scroll == wk->lastScroll) {
        return;
    }
    if (scroll > wk->lastScroll) {
        dir = 1;
    } else {
        dir = -1;
    }
    if (reset == TRUE) {
        dir = 0;
    }
    for (i = 0; i < LIST_ROWS; i++) {
        if (wk->movies[scroll + i].isNew) {
            wk->newMarks |= 1 << i;
        }
    }
    if (scroll > 0 && wk->movies[scroll - 1].isNew) {
        wk->newMarks |= 1 << LIST_ROWS;
    }
    if (scroll + LIST_ROWS < wk->movieCount && wk->movies[scroll + LIST_ROWS].isNew) {
        wk->newMarks |= 1 << (LIST_ROWS + 1);
    }
    for (i = 0; i < NEW_MARK_COUNT; i++) {
        BOOL visible = FALSE;

        if (wk->newMarks & (1 << i)) {
            visible = TRUE;
        }
        PkwdBook_SetActorVisible(wk, GROUP_MARK, MARK_NEW + i, visible);
    }
    for (i = 0; i < NEW_MARK_COUNT; i++) {
        func_0204c178(wk->markActors[MARK_NEW + i], &pos, CLACT_SURFACE_SUB);
        pos.x = PkwdBook_GetPosition(wk, (u32)(i + POSITION_NEW_MARKS) * 2);
        pos.y = PkwdBook_GetPosition(wk, (i + POSITION_NEW_MARKS) * 2 + 1);
        pos.y += (s16)(dir * 24);
        func_0204c140(wk->markActors[MARK_NEW + i], &pos, CLACT_SURFACE_SUB);
    }
    wk->lastScroll = scroll;
}

static void PkwdBook_ScrollNewMarks(PkwdBookWork *wk, s16 delta) {
    ClActorPos pos;
    int i;

    for (i = 0; i < NEW_MARK_COUNT; i++) {
        func_0204c178(wk->markActors[MARK_NEW + i], &pos, CLACT_SURFACE_SUB);
        pos.y -= delta;
        func_0204c140(wk->markActors[MARK_NEW + i], &pos, CLACT_SURFACE_SUB);
    }
}

// Reads the name of the player's role from each movie's script
static void PkwdBook_LoadRoleNames(PkwdBookWork *wk) {
    s32 gender = 0;
    int i;

    if (getTrainerGender(wk->param->playerInfo) == 1) {
        gender = 1;
    }
    for (i = 0; i < LIST_MOVIE_COUNT; i++) {
        wk->msgData[MSG_SCRIPT] =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sPkwdBookScriptBanks[i * 2 + gender], wk->heapId);
        wk->roleNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData[MSG_SCRIPT], sPkwdBookRoleNameMessage[0]);
        GFL_MsgDataFree(wk->msgData[MSG_SCRIPT]);
    }
    wk->msgData[MSG_SCRIPT] = NULL;
}

static void PkwdBook_FreeRoleNames(PkwdBookWork *wk) {
    int i;

    for (i = 0; i < LIST_MOVIE_COUNT; i++) {
        GFL_StrBufFree(wk->roleNames[i]);
        wk->roleNames[i] = NULL;
    }
}

static u32 PkwdBook_GetPosterGraphicIdx(PkwdBookWork *wk, u32 movie) {
    return sPkwdBookPosterGraphics[movie];
}

static void PkwdBook_Unused1(PkwdBookWork *wk) {
}

static void PkwdBook_Unused2(PkwdBookWork *wk) {
}
