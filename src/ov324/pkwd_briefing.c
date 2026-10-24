#include "types.h"
#include "app/pkwd_briefing.h"
#include "battle/btl_setup.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/pokewood.h"
#include "system/gf_font.h"
#include "system/infowin.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/event_pass_power_startmenu.h"
#include "text/system/pkwd_briefing_common.h"

// Pokéstar Studios' theater: the movie's set in 3D on the upper screen and the audience in its seats on the lower
// one. Before the battle it shows the briefing of the scene a line at a time; after it, the movie's credits scroll
// by while the audience reacts to how the movie went. Named after the ROM's "pkwd_briefing.c"

// The archive of the movies' data, as in event_pokewood.c
#define ARCID_PKWD_MOVIE 269

#define MOVIE_COUNT 41
// The movies from here on credit an actor rather than the player
#define MOVIE_NO_PLAYER 40

#define SEAT_COUNT PKWD_BRIEFING_SEAT_COUNT
#define PARTY_SIZE 6

// The lines of a briefing, which each bank has for a boy and then for a girl, and the message that ends them
#define LINE_COUNT 10
#define LINE_END (LINE_COUNT * 2)

#define WINDOW_COUNT 36

// The files of ARCID_PKWD_COUNT
enum {
    FILE_PALETTE = 0,
    FILE_CREDITS_PALETTE = 3,
    FILE_TEXT_CHARS = 8,
    FILE_CREDITS_CHARS = 12,
    FILE_TEXT_SCREEN = 19,
    FILE_CREDITS_END_SCREEN = 22,
    FILE_CAMERA = 35,
};

// The files of the audience in ARCID_BTLV
enum {
    FILE_AUDIENCE_BG_PALETTE = 560,
    FILE_AUDIENCE_BG_CHARS = 561,
    FILE_AUDIENCE_BG_SCREEN = 565,
    FILE_AUDIENCE_CHARS = 567,
    FILE_AUDIENCE_CELLS = 568,
    FILE_AUDIENCE_ANIMS = 569,
    FILE_AUDIENCE_PALETTE = 570,
};

// The animations of a member of the audience
enum {
    AUDIENCE_SIT = 0,
    AUDIENCE_GOOD = 5,
    AUDIENCE_STRANGE = 7,
    AUDIENCE_BAD = 8,
};

// The frames of the audience's palette animation, and its patterns
#define AUDIENCE_PALETTE_FRAMES 3
#define AUDIENCE_PATTERN_COUNT 3
#define AUDIENCE_OBJ_PALETTE_SIZE 0x80
#define AUDIENCE_BG_PALETTE_SIZE 0xa0

// The strings of the credits
enum {
    // The 15 messages of the movie
    CREDIT_MOVIE,
    CREDIT_SCREENPLAY = 15,
    CREDIT_VFX,
    CREDIT_ART,
    CREDIT_DIRECTOR,
    CREDIT_DIRECTOR_NAME,
    CREDIT_PLAYER_ROLE,
    CREDIT_PLAYER_NAME,
    CREDIT_ACTOR_NAME,
    CREDIT_ACTOR_NAME_2,
    CREDIT_PLAYER_POKEMON,
    CREDIT_ENEMY_POKEMON = CREDIT_PLAYER_POKEMON + PARTY_SIZE,
    CREDIT_COUNT = CREDIT_ENEMY_POKEMON + PARTY_SIZE,
};

// What a line of the credits reads: a string below CREDIT_PLAYER_ROLE, which is left out when it is empty, or one of
// these
enum {
    SOURCE_PLAYER_ROLE = 30,
    SOURCE_PLAYER_NAME,
    SOURCE_ACTOR_NAME,
    SOURCE_ACTOR_NAME_2,
    // The player's name, or the actor's when they swapped places
    SOURCE_STAR_NAME,
    SOURCE_PLAYER_POKEMON,
    SOURCE_ENEMY_POKEMON = SOURCE_PLAYER_POKEMON + PARTY_SIZE,
};

// The lines of the credits that are not text
enum {
    CREDITS_SPACE = 35,
    CREDITS_END,
};

// How the credits are laid out
enum {
    CREDITS_BOTH,
    CREDITS_RIGHT,
    CREDITS_LEFT,
    CREDITS_LAYOUT_COUNT,
};

enum {
    STATE_INIT,
    STATE_LOAD,
    STATE_WAIT_PRINT,
    STATE_WAIT_FADE_IN,
    STATE_RUN,
    STATE_WAIT_VOICE,
    STATE_FADE_OUT,
    STATE_WAIT_FADE_OUT,
    STATE_FREE,
    STATE_DONE,
};

// The steps of the briefing
enum {
    BRIEFING_START,
    BRIEFING_WAIT,
    BRIEFING_FADE_OUT_BLANK,
    BRIEFING_WAIT_BLANK,
    BRIEFING_START_LINE,
    BRIEFING_FADE_IN_LINE,
    BRIEFING_SHOW_LINE,
    BRIEFING_FADE_OUT_LINE,
    BRIEFING_NEXT_LINE,
    BRIEFING_UNUSED_9,
    BRIEFING_UNUSED_10,
    BRIEFING_WAIT_PRINT,
};

// The steps of the credits
enum {
    CREDITS_START,
    CREDITS_START_2,
    CREDITS_FADE_OUT_BLANK,
    CREDITS_WAIT_BLANK,
    CREDITS_SHOW,
    CREDITS_FADE_IN,
    CREDITS_SCROLL,
    CREDITS_UNUSED_7,
    CREDITS_FINISH,
    CREDITS_STOP_MUSIC,
    CREDITS_REACT,
    CREDITS_WAIT_REACTION,
};

typedef struct {
    TCBExManager *tcbManager;
    TCB *vblankTask;
} PkwdBriefingSystem;

typedef struct {
    BmpWin *windows[WINDOW_COUNT];
    u32 unk90;
    PrintWindow printWindows[WINDOW_COUNT];
    u32 unk1B4;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData;
    WordSet *wordSet;
    // The briefing's line and the next one, by turns
    StrBuf *lines[2];
    // What a message with nothing to show reads
    StrBuf *empty;
    StrBuf *unk1D4;
    StrBuf *credits[CREDIT_COUNT];
    u32 unk268[2];
    s32 alpha;
    // The briefing's line
    s32 line;
    s32 timer;
    // How far the credits have scrolled, in pixels
    s32 scroll;
    // How far the picture at the end of the credits has scrolled in
    s32 endScroll;
    s32 windowCount;
    u32 unk288;
    // The height of the credits, in tiles
    s32 height;
    PokeParty *playerParty;
    PokeParty *enemyParty;
    // How many lines of text each line of the briefing has
    u8 lineHeights[LINE_COUNT];
    // Whether the player's role and the actor's swapped places in the credits
    BOOL swapped;
} PkwdBriefingText;

typedef struct {
    G3DManager *manager;
    u16 scene;
    G3DLight *light;
    G3DCurve *curve;
    G3DCamera *camera;
} PkwdBriefingSet;

typedef struct {
    ClActUnit *unit;
    u32 unk4[13];
} PkwdBriefingClAct;

typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
    ClActUnit *unit;
    ClActor *actors[SEAT_COUNT];
    // The frames until each member of the audience starts to move
    u16 delays[SEAT_COUNT];
    ArcTool *arc;
    // The frames of the palettes' animation
    u8 *objPalettes;
    u8 *bgPalettes;
    s32 frame;
    s32 timer;
    s32 pattern;
    // How many more times the pattern plays
    s32 repeat;
} PkwdBriefingAudience;

typedef struct {
    PkwdBriefingParam *param;
    u16 state;
    u16 heapId;
    PkwdBriefingSystem system;
    PkwdBriefingText text;
    PkwdBriefingSet set;
    PkwdBriefingClAct clact;
    PkwdBriefingAudience audience;
    u32 unk3B4;
    u32 voice;
    u32 unk3BC;
    u32 lastVBlank;
    u32 step;
    u32 sex;
    // The layout of the credits
    u32 layout;
    // How long a line of the briefing stays, in frames
    s32 lineWait;
} PkwdBriefingWork;

// A seat of the audience: where it is, and its palette
typedef struct {
    u8 x;
    u8 y;
    u8 palette;
    u8 padding;
} PkwdBriefingSeat;

// A line of the credits: its entry in the table of sources or CREDITS_SPACE, the column of its window, how many tiles
// the credits go down after it, and whether it ends at the middle of the screen
typedef struct {
    u8 line;
    u8 x;
    u8 height;
    u8 rightAligned;
} PkwdBriefingCreditsLine;

// A pattern of the audience's palette animation: how many frames it has, and how many times it may repeat
typedef struct {
    s16 length;
    s16 repeatMax;
} PkwdBriefingPattern;

// The fields of a movie's data that the credits read
typedef struct {
    u8 unk0[8];
    s16 director;
    u8 unkA[0x24];
    // The level of the studio above which the star is the player
    s16 starLevel;
} PkwdBriefingMovieFile;

static BOOL PkwdBriefing_Init(GameProc *proc, u32 *state, void *param, void *work);
static void PkwdBriefing_CheckSkip(PkwdBriefingWork *wk);
static BOOL PkwdBriefing_Main(GameProc *proc, u32 *state, void *param, void *work);
static void PkwdBriefing_GetParties(PkwdBriefingWork *wk);
static void PkwdBriefing_FreeParties(PkwdBriefingWork *wk);
static BOOL PkwdBriefing_CreditsMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdBriefing_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void PkwdBriefing_ResetDisplay(void);
static void PkwdBriefing_SetVRAM(void);
static void PkwdBriefing_Update(PkwdBriefingWork *wk);
static void PkwdBriefing_VBlank(TCB *tcb, void *data);
static void PkwdBriefing_CreditsVBlank(TCB *tcb, void *data);
static void PkwdBriefingSystem_Init(PkwdBriefingSystem *system, PkwdBriefingWork *wk, HeapID heapId);
static void PkwdBriefingSystem_InitCredits(PkwdBriefingSystem *system, PkwdBriefingWork *wk, HeapID heapId);
static void PkwdBriefingSystem_Update(PkwdBriefingSystem *system, HeapID heapId);
static void PkwdBriefingSystem_Free(PkwdBriefingSystem *system);
static void PkwdBriefingText_InitBG(PkwdBriefingText *text, s32 movie, HeapID heapId);
static void PkwdBriefingText_LoadCreditsBG(PkwdBriefingText *text, HeapID heapId, u32 layout);
static void PkwdBriefingText_LoadCreditsScreen(PkwdBriefingText *text, HeapID heapId, u32 layout);
static void PkwdBriefingText_LoadCreditsEndScreen(PkwdBriefingText *text, HeapID heapId);
static void PkwdBriefingText_FreeBG(PkwdBriefingText *text);
static void PkwdBriefingText_LoadBriefing(PkwdBriefingText *text, s32 movie, PlayerInfo *playerInfo, HeapID heapId);
static void PkwdBriefingText_FreeBriefing(PkwdBriefingText *text);
static void PkwdBriefingText_SwapCredits(PkwdBriefingText *text, s32 a, s32 b);
static void PkwdBriefingText_SetStar(PkwdBriefingText *text, s32 level, s32 starLevel);
static StrBuf *PkwdBriefing_CreatePlayerName(PkwdBriefingParam *param, HeapID heapId);
static void PkwdBriefing_LoadCredits(PkwdBriefingParam *param, PkwdBriefingText *text, s32 movie, HeapID heapId);
static void PkwdBriefingText_FreeCredits(PkwdBriefingText *text);
static void PkwdBriefingText_CreateBriefingWindows(PkwdBriefingText *text, HeapID heapId);
static void PkwdBriefingText_FreeBriefingWindows(PkwdBriefingText *text);
static void PkwdBriefingText_Print(PkwdBriefingText *text, s32 window, StrBuf *strbuf, u32 rightAligned);
static StrBuf *PkwdBriefingText_GetCredit(PkwdBriefingText *text, u32 line);
static void PkwdBriefingText_PrintCredit(PkwdBriefingText *text, s32 window, u32 line, u32 rightAligned, HeapID heapId);
static void PkwdBriefingText_CreateCreditsWindows(PkwdBriefingText *text, HeapID heapId, u32 layout);
static void PkwdBriefingText_FreeCreditsWindows(PkwdBriefingText *text);
static void PkwdBriefingSet_Init(PkwdBriefingSet *set, HeapID heapId, s32 movie);
static void PkwdBriefingSet_StepCamera(PkwdBriefingSet *set, HeapID heapId);
static void PkwdBriefingSet_Draw(PkwdBriefingSet *set, HeapID heapId, int movie);
static void PkwdBriefingSet_Free(PkwdBriefingSet *set);
static void PkwdBriefingClAct_Init(PkwdBriefingClAct *clact, HeapID heapId);
static void PkwdBriefingClAct_Update(PkwdBriefingClAct *clact, HeapID heapId);
static void PkwdBriefingClAct_Free(PkwdBriefingClAct *clact);
static void PkwdBriefing_RunBriefing(PkwdBriefingWork *wk);
static void PkwdBriefing_InitWindow(void);
static void PkwdBriefing_SetCreditsWindow(u32 side);
static void PkwdBriefingText_PrintLine(PkwdBriefingText *text, s32 line, u32 sex);
static void PkwdBriefingText_CountLines(PkwdBriefingText *text, u32 sex);
static BOOL PkwdBriefing_IsLastLine(PkwdBriefingWork *wk);
static void PkwdBriefingAudience_Init(PkwdBriefingAudience *audience, HeapID heapId);
static void PkwdBriefingAudience_Free(PkwdBriefingAudience *audience);
static u32 PkwdBriefingAudience_IsSeatTaken(u32 seats, u32 seat);
static void PkwdBriefingAudience_TakeSeat(u32 *seats, u32 seat);
static void PkwdBriefingAudience_HideEmptySeats(PkwdBriefingAudience *audience, u32 seats);
static u32 PkwdBriefingAudience_PickSeats(PkwdBriefingAudience *audience, s32 count);
static void PkwdBriefingAudience_SetDelays(PkwdBriefingAudience *audience, u32 seats);
static void PkwdBriefing_InitAudience(PkwdBriefingWork *wk, BOOL pickSeats, HeapID heapId);
static void PkwdBriefingAudience_SetVisible(PkwdBriefingAudience *audience, s32 seat, BOOL visible);
static void PkwdBriefingAudience_SetSequence(PkwdBriefingAudience *audience, s32 seat, u32 sequence);
static void PkwdBriefingAudience_SetAllSequences(PkwdBriefingAudience *audience, u32 sequence, u32 seats);
static void PkwdBriefingAudience_PlaySE(u32 count, s32 sequence);
static void PkwdBriefingAudience_LoadPalettes(PkwdBriefingAudience *audience, HeapID heapId);
static void PkwdBriefingAudience_FreePalettes(PkwdBriefingAudience *audience);
static void PkwdBriefingAudience_SetPalettes(PkwdBriefingAudience *audience, s32 frame);
static void PkwdBriefingAudience_UpdatePalettes(PkwdBriefingAudience *audience);
static void PkwdBriefingAudience_SetPattern(PkwdBriefingAudience *audience, s32 pattern);
static s32 PkwdBriefingAudience_PickPattern(PkwdBriefingAudience *audience, s32 pattern);
static u32 PkwdBriefingAudience_Random(PkwdBriefingAudience *audience, u32 max);
static void PkwdBriefing_UpdateAudience(PkwdBriefingWork *wk);
static void PkwdBriefing_RunCredits(PkwdBriefingWork *wk);
static void PkwdBriefing_React(PkwdBriefingWork *wk, BtlSetup *setup);
static void PkwdBriefing_CheckCreditsEnd(PkwdBriefingWork *wk);
static void PkwdBriefingText_ShowCreditsWindows(PkwdBriefingText *text);
static void PkwdBriefing_ScrollCredits(PkwdBriefingWork *wk, s32 scroll, u32 layout, HeapID heapId);
static s32 PkwdBriefing_GetLineWait(PkwdBriefingWork *wk);

// MWCC sorts a file's data by size with a heapsort, which leaves objects of one size in an order that depends on
// where every object is declared. The declarations below are in an order that gives the ROM's layout, found by
// search: the sets' resources, animations and actors each together and in the movies' order, and the other tables
// around them where the sort needs them
const GameProcFunctions PKWD_BRIEFING_PROC_FUNCTIONS = {
    PkwdBriefing_Init,
    PkwdBriefing_Main,
    PkwdBriefing_Exit,
};

const GameProcFunctions PKWD_BRIEFING_CREDITS_PROC_FUNCTIONS = {
    PkwdBriefing_Init,
    PkwdBriefing_CreditsMain,
    PkwdBriefing_Exit,
};

// The lines of the credits for each layout. The windows are 15 tiles wide
// clang-format off
static const PkwdBriefingCreditsLine sPkwdBriefingCreditsBoth[] = {
    { 0, 1, 0, TRUE },
    { 12, 17, 3, FALSE },
    { 1, 1, 0, TRUE },
    { 13, 17, 2, FALSE },
    { 14, 17, 2, FALSE },
    { 15, 17, 2, FALSE },
    { 16, 17, 2, FALSE },
    { 17, 17, 2, FALSE },
    { 18, 17, 2, FALSE },
    { CREDITS_SPACE, 0, 2, FALSE },
    { 4, 1, 0, TRUE },
    { 26, 17, 4, FALSE },
    { 5, 1, 0, TRUE },
    { 27, 17, 4, FALSE },
    { 6, 1, 0, TRUE },
    { 28, 17, 4, FALSE },
    { 7, 1, 0, TRUE },
    { 29, 17, 4, FALSE },
    { 8, 1, 0, TRUE },
    { 30, 17, 4, FALSE },
    { 2, 1, 0, TRUE },
    { 19, 17, 3, FALSE },
    { 3, 1, 0, TRUE },
    { 20, 17, 2, FALSE },
    { 21, 17, 2, FALSE },
    { 22, 17, 2, FALSE },
    { 23, 17, 2, FALSE },
    { 24, 17, 2, FALSE },
    { 25, 17, 2, FALSE },
    { CREDITS_SPACE, 1, 2, FALSE },
    { 9, 1, 0, TRUE },
    { 31, 17, 4, FALSE },
    { 10, 1, 0, TRUE },
    { 32, 17, 2, FALSE },
    { 33, 17, 4, FALSE },
    { 11, 1, 0, TRUE },
    { 34, 17, 2, FALSE },
    { CREDITS_END, 0, 0, FALSE },
};

// clang-format on

// The frames of the palettes that the steps of each pattern show
static const s16 sPkwdBriefingPatternFrames0[] = { 1, 2, 1, 0 };

// clang-format off
static const PkwdBriefingCreditsLine sPkwdBriefingCreditsRight[] = {
    { 0, 1, 2, FALSE },
    { 12, 2, 3, FALSE },
    { 1, 1, 2, FALSE },
    { 13, 2, 2, FALSE },
    { 14, 2, 2, FALSE },
    { 15, 2, 2, FALSE },
    { 16, 2, 2, FALSE },
    { 17, 2, 2, FALSE },
    { 18, 2, 2, FALSE },
    { CREDITS_SPACE, 0, 2, FALSE },
    { 4, 1, 2, FALSE },
    { 26, 2, 3, FALSE },
    { 5, 1, 2, FALSE },
    { 27, 2, 3, FALSE },
    { 6, 1, 2, FALSE },
    { 28, 2, 3, FALSE },
    { 7, 1, 2, FALSE },
    { 29, 2, 3, FALSE },
    { 8, 1, 2, FALSE },
    { 30, 2, 4, FALSE },
    { 2, 1, 2, FALSE },
    { 19, 2, 3, FALSE },
    { 3, 1, 2, FALSE },
    { 20, 2, 2, FALSE },
    { 21, 2, 2, FALSE },
    { 22, 2, 2, FALSE },
    { 23, 2, 2, FALSE },
    { 24, 2, 2, FALSE },
    { 25, 2, 2, FALSE },
    { CREDITS_SPACE, 1, 2, FALSE },
    { 9, 1, 2, FALSE },
    { 31, 2, 3, FALSE },
    { 10, 1, 2, FALSE },
    { 32, 2, 2, FALSE },
    { 33, 2, 3, FALSE },
    { 11, 1, 2, FALSE },
    { 34, 2, 2, FALSE },
    { CREDITS_END, 0, 0, FALSE },
};

// clang-format on

// clang-format off
static const PkwdBriefingCreditsLine sPkwdBriefingCreditsLeft[] = {
    { 0, 16, 2, FALSE },
    { 12, 17, 3, FALSE },
    { 1, 16, 2, FALSE },
    { 13, 17, 2, FALSE },
    { 14, 17, 2, FALSE },
    { 15, 17, 2, FALSE },
    { 16, 17, 2, FALSE },
    { 17, 17, 2, FALSE },
    { 18, 17, 2, FALSE },
    { CREDITS_SPACE, 0, 2, FALSE },
    { 4, 16, 2, FALSE },
    { 26, 17, 3, FALSE },
    { 5, 16, 2, FALSE },
    { 27, 17, 3, FALSE },
    { 6, 16, 2, FALSE },
    { 28, 17, 3, FALSE },
    { 7, 16, 2, FALSE },
    { 29, 17, 3, FALSE },
    { 8, 16, 2, FALSE },
    { 30, 17, 4, FALSE },
    { 2, 16, 2, FALSE },
    { 19, 17, 3, FALSE },
    { 3, 16, 2, FALSE },
    { 20, 17, 2, FALSE },
    { 21, 17, 2, FALSE },
    { 22, 17, 2, FALSE },
    { 23, 17, 2, FALSE },
    { 24, 17, 2, FALSE },
    { 25, 17, 2, FALSE },
    { CREDITS_SPACE, 16, 2, FALSE },
    { 9, 16, 2, FALSE },
    { 31, 17, 3, FALSE },
    { 10, 16, 2, FALSE },
    { 32, 17, 2, FALSE },
    { 33, 17, 3, FALSE },
    { 11, 16, 2, FALSE },
    { 34, 17, 2, FALSE },
    { CREDITS_END, 0, 0, FALSE },
};

// clang-format on

static const BGSysLCDConfig sPkwdBriefingLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

// BGs 2 and 3: the second BG of the text, and the frame around the set
static const BGSetup sPkwdBriefingText2BGSetup = { 0,
                                                   0,
                                                   0x1000,
                                                   0,
                                                   BGRES_256x512,
                                                   GX_BG_COLORMODE_16,
                                                   GX_BG_SCRBASE(0x1000),
                                                   GX_BG_CHARBASE(0x18000),
                                                   0x8000,
                                                   GX_BG_EXTPLTT_01,
                                                   1,
                                                   GX_BG_AREAOVER_XLU,
                                                   0 };

static const BGSetup sPkwdBriefingFrameBGSetup = { 0,
                                                   0,
                                                   0x800,
                                                   0,
                                                   BGRES_256x256,
                                                   GX_BG_COLORMODE_16,
                                                   GX_BG_SCRBASE(0x2000),
                                                   GX_BG_CHARBASE(0x8000),
                                                   0x8000,
                                                   GX_BG_EXTPLTT_01,
                                                   2,
                                                   GX_BG_AREAOVER_XLU,
                                                   0 };

// The screen of the credits' frame for each layout
static const u32 sPkwdBriefingCreditsScreens[CREDITS_LAYOUT_COUNT] = { 25, 26, 27 };

// The screen that a layout with the credits on one side changes to at their end
static const u32 sPkwdBriefingCreditsSideScreens[CREDITS_LAYOUT_COUNT] = { 0, 23, 24 };

static const s16 sPkwdBriefingPatternFrames2[] = { 1, 0 };

// The patterns of the audience's palette animation
static const PkwdBriefingPattern sPkwdBriefingPatterns[AUDIENCE_PATTERN_COUNT] = { { 4, 4 }, { 4, 4 }, { 2, 2 } };

// The sounds of the audience's reactions, by animation, for a small audience and a large one
static const u32 sPkwdBriefingAudienceSE[][2] = {
    { 0, 0 },
    { SEQ_SE_SW_PKWOOD_AUD_03, SEQ_SE_SW_PKWOOD_AUD_03 },
    { SEQ_SE_SW_PKWOOD_AUD_07, SEQ_SE_SW_PKWOOD_AUD_08 },
    { SEQ_SE_SW_PKWOOD_AUD_09, SEQ_SE_SW_PKWOOD_AUD_10 },
    { SEQ_SE_SW_PKWOOD_AUD_11, SEQ_SE_SW_PKWOOD_AUD_12 },
    { SEQ_SE_SW_PKWOOD_AUD_01, SEQ_SE_SW_PKWOOD_AUD_02 },
    { SEQ_SE_SW_PKWOOD_AUD_05, SEQ_SE_SW_PKWOOD_AUD_06 },
    { SEQ_SE_SW_PKWOOD_AUD_13, SEQ_SE_SW_PKWOOD_AUD_14 },
    { SEQ_SE_SW_PKWOOD_AUD_15, SEQ_SE_SW_PKWOOD_AUD_16 },
};

// What each line of the credits reads
// clang-format off
static const u32 sPkwdBriefingCreditSources[] = {
    SOURCE_PLAYER_ROLE,       CREDIT_MOVIE + 1,         SOURCE_ACTOR_NAME,        CREDIT_MOVIE + 0,
    CREDIT_MOVIE + 4,         CREDIT_MOVIE + 6,         CREDIT_MOVIE + 8,         CREDIT_MOVIE + 10,
    CREDIT_MOVIE + 12,        CREDIT_VFX,               CREDIT_SCREENPLAY,        CREDIT_DIRECTOR,
    SOURCE_PLAYER_NAME,       SOURCE_PLAYER_POKEMON,    SOURCE_PLAYER_POKEMON + 1, SOURCE_PLAYER_POKEMON + 2,
    SOURCE_PLAYER_POKEMON + 3, SOURCE_PLAYER_POKEMON + 4, SOURCE_PLAYER_POKEMON + 5, SOURCE_ACTOR_NAME_2,
    SOURCE_ENEMY_POKEMON,     SOURCE_ENEMY_POKEMON + 1, SOURCE_ENEMY_POKEMON + 2, SOURCE_ENEMY_POKEMON + 3,
    SOURCE_ENEMY_POKEMON + 4, SOURCE_ENEMY_POKEMON + 5, CREDIT_MOVIE + 5,         CREDIT_MOVIE + 7,
    CREDIT_MOVIE + 9,         CREDIT_MOVIE + 11,        CREDIT_MOVIE + 13,        CREDIT_ART,
    CREDIT_MOVIE + 14,        SOURCE_STAR_NAME,         CREDIT_DIRECTOR_NAME,
};

// clang-format on

// BG 5, the seats
static const BGSetup sPkwdBriefingSeatsBGSetup = { 0,
                                                   0,
                                                   0x800,
                                                   0,
                                                   BGRES_256x256,
                                                   GX_BG_COLORMODE_16,
                                                   GX_BG_SCRBASE(0x0000),
                                                   GX_BG_CHARBASE(0x4000),
                                                   0x4000,
                                                   GX_BG_EXTPLTT_01,
                                                   1,
                                                   GX_BG_AREAOVER_XLU,
                                                   0 };

// What every member of the audience starts from
static const ClActorSetup sPkwdBriefingAudienceSetup = { 0, 0, AUDIENCE_SIT, 2, 0 };

static const BGSysVRAMConfig sPkwdBriefingVRAMConfig = {
    GX_VRAM_BG_128_D,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_NONE,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0123_E,  GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// The sets: the model and the animation of each, the animation of its actor, and the actor
static const G3DSceneResourceSetup sPkwdBriefingSet00Resources[] = {
    { ARCID_BTLV, 141, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 146, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet01Resources[] = {
    { ARCID_BTLV, 205, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 209, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet02Resources[] = {
    { ARCID_BTLV, 313, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 316, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet03Resources[] = {
    { ARCID_BTLV, 273, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 278, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet04Resources[] = {
    { ARCID_BTLV, 189, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 193, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet05Resources[] = {
    { ARCID_BTLV, 293, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 298, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet06Resources[] = {
    { ARCID_BTLV, 314, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 317, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet07Resources[] = {
    { ARCID_BTLV, 161, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 166, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet08Resources[] = {
    { ARCID_BTLV, 189, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 193, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet09Resources[] = {
    { ARCID_BTLV, 122, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 127, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet10Resources[] = {
    { ARCID_BTLV, 207, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 211, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet11Resources[] = {
    { ARCID_BTLV, 181, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 183, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet12Resources[] = {
    { ARCID_BTLV, 191, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 195, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet13Resources[] = {
    { ARCID_BTLV, 163, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 168, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet14Resources[] = {
    { ARCID_BTLV, 142, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 147, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet15Resources[] = {
    { ARCID_BTLV, 141, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 146, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet16Resources[] = {
    { ARCID_BTLV, 253, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 258, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet17Resources[] = {
    { ARCID_BTLV, 237, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 241, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet18Resources[] = {
    { ARCID_BTLV, 239, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 243, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet19Resources[] = {
    { ARCID_BTLV, 294, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 299, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet20Resources[] = {
    { ARCID_BTLV, 295, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 300, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet21Resources[] = {
    { ARCID_BTLV, 223, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 227, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet22Resources[] = {
    { ARCID_BTLV, 257, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 262, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet23Resources[] = {
    { ARCID_BTLV, 276, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 281, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet24Resources[] = {
    { ARCID_BTLV, 273, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 278, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet25Resources[] = {
    { ARCID_BTLV, 274, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 279, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet26Resources[] = {
    { ARCID_BTLV, 255, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 260, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet27Resources[] = {
    { ARCID_BTLV, 296, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 301, 0, G3D_SCENE_RES_ARCSYS },
};
static const G3DSceneResourceSetup sPkwdBriefingSet28Resources[] = {
    { ARCID_BTLV, 125, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_BTLV, 130, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneAnimationSetup sPkwdBriefingSet00Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet01Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet02Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet03Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet04Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet05Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet06Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet07Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet08Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet09Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet10Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet11Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet12Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet13Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet14Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet15Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet16Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet17Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet18Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet19Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet20Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet21Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet22Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet23Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet24Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet25Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet26Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet27Anims[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sPkwdBriefingSet28Anims[] = { { 1, 0 } };

static const G3DSceneActorSetup sPkwdBriefingSet00Actor = {
    0, 0, 0, 0, sPkwdBriefingSet00Anims, NELEMS(sPkwdBriefingSet00Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet01Actor = {
    0, 0, 0, 0, sPkwdBriefingSet01Anims, NELEMS(sPkwdBriefingSet01Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet02Actor = {
    0, 0, 0, 0, sPkwdBriefingSet02Anims, NELEMS(sPkwdBriefingSet02Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet03Actor = {
    0, 0, 0, 0, sPkwdBriefingSet03Anims, NELEMS(sPkwdBriefingSet03Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet04Actor = {
    0, 0, 0, 0, sPkwdBriefingSet04Anims, NELEMS(sPkwdBriefingSet04Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet05Actor = {
    0, 0, 0, 0, sPkwdBriefingSet05Anims, NELEMS(sPkwdBriefingSet05Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet06Actor = {
    0, 0, 0, 0, sPkwdBriefingSet06Anims, NELEMS(sPkwdBriefingSet06Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet07Actor = {
    0, 0, 0, 0, sPkwdBriefingSet07Anims, NELEMS(sPkwdBriefingSet07Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet08Actor = {
    0, 0, 0, 0, sPkwdBriefingSet08Anims, NELEMS(sPkwdBriefingSet08Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet09Actor = {
    0, 0, 0, 0, sPkwdBriefingSet09Anims, NELEMS(sPkwdBriefingSet09Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet10Actor = {
    0, 0, 0, 0, sPkwdBriefingSet10Anims, NELEMS(sPkwdBriefingSet10Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet11Actor = {
    0, 0, 0, 0, sPkwdBriefingSet11Anims, NELEMS(sPkwdBriefingSet11Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet12Actor = {
    0, 0, 0, 0, sPkwdBriefingSet12Anims, NELEMS(sPkwdBriefingSet12Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet13Actor = {
    0, 0, 0, 0, sPkwdBriefingSet13Anims, NELEMS(sPkwdBriefingSet13Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet14Actor = {
    0, 0, 0, 0, sPkwdBriefingSet14Anims, NELEMS(sPkwdBriefingSet14Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet15Actor = {
    0, 0, 0, 0, sPkwdBriefingSet15Anims, NELEMS(sPkwdBriefingSet15Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet16Actor = {
    0, 0, 0, 0, sPkwdBriefingSet16Anims, NELEMS(sPkwdBriefingSet16Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet17Actor = {
    0, 0, 0, 0, sPkwdBriefingSet17Anims, NELEMS(sPkwdBriefingSet17Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet18Actor = {
    0, 0, 0, 0, sPkwdBriefingSet18Anims, NELEMS(sPkwdBriefingSet18Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet19Actor = {
    0, 0, 0, 0, sPkwdBriefingSet19Anims, NELEMS(sPkwdBriefingSet19Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet20Actor = {
    0, 0, 0, 0, sPkwdBriefingSet20Anims, NELEMS(sPkwdBriefingSet20Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet21Actor = {
    0, 0, 0, 0, sPkwdBriefingSet21Anims, NELEMS(sPkwdBriefingSet21Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet22Actor = {
    0, 0, 0, 0, sPkwdBriefingSet22Anims, NELEMS(sPkwdBriefingSet22Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet23Actor = {
    0, 0, 0, 0, sPkwdBriefingSet23Anims, NELEMS(sPkwdBriefingSet23Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet24Actor = {
    0, 0, 0, 0, sPkwdBriefingSet24Anims, NELEMS(sPkwdBriefingSet24Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet25Actor = {
    0, 0, 0, 0, sPkwdBriefingSet25Anims, NELEMS(sPkwdBriefingSet25Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet26Actor = {
    0, 0, 0, 0, sPkwdBriefingSet26Anims, NELEMS(sPkwdBriefingSet26Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet27Actor = {
    0, 0, 0, 0, sPkwdBriefingSet27Anims, NELEMS(sPkwdBriefingSet27Anims)
};
static const G3DSceneActorSetup sPkwdBriefingSet28Actor = {
    0, 0, 0, 0, sPkwdBriefingSet28Anims, NELEMS(sPkwdBriefingSet28Anims)
};

// The set of each movie
static const G3DSceneSetup sPkwdBriefingScenes[MOVIE_COUNT] = {
    { sPkwdBriefingSet00Resources, NELEMS(sPkwdBriefingSet00Resources), &sPkwdBriefingSet00Actor, 1 },
    { sPkwdBriefingSet01Resources, NELEMS(sPkwdBriefingSet01Resources), &sPkwdBriefingSet01Actor, 1 },
    { sPkwdBriefingSet02Resources, NELEMS(sPkwdBriefingSet02Resources), &sPkwdBriefingSet02Actor, 1 },
    { sPkwdBriefingSet03Resources, NELEMS(sPkwdBriefingSet03Resources), &sPkwdBriefingSet03Actor, 1 },
    { sPkwdBriefingSet04Resources, NELEMS(sPkwdBriefingSet04Resources), &sPkwdBriefingSet04Actor, 1 },
    { sPkwdBriefingSet05Resources, NELEMS(sPkwdBriefingSet05Resources), &sPkwdBriefingSet05Actor, 1 },
    { sPkwdBriefingSet06Resources, NELEMS(sPkwdBriefingSet06Resources), &sPkwdBriefingSet06Actor, 1 },
    { sPkwdBriefingSet07Resources, NELEMS(sPkwdBriefingSet07Resources), &sPkwdBriefingSet07Actor, 1 },
    { sPkwdBriefingSet08Resources, NELEMS(sPkwdBriefingSet08Resources), &sPkwdBriefingSet08Actor, 1 },
    { sPkwdBriefingSet07Resources, NELEMS(sPkwdBriefingSet07Resources), &sPkwdBriefingSet07Actor, 1 },
    { sPkwdBriefingSet09Resources, NELEMS(sPkwdBriefingSet09Resources), &sPkwdBriefingSet09Actor, 1 },
    { sPkwdBriefingSet08Resources, NELEMS(sPkwdBriefingSet08Resources), &sPkwdBriefingSet08Actor, 1 },
    { sPkwdBriefingSet10Resources, NELEMS(sPkwdBriefingSet10Resources), &sPkwdBriefingSet10Actor, 1 },
    { sPkwdBriefingSet08Resources, NELEMS(sPkwdBriefingSet08Resources), &sPkwdBriefingSet08Actor, 1 },
    { sPkwdBriefingSet10Resources, NELEMS(sPkwdBriefingSet10Resources), &sPkwdBriefingSet10Actor, 1 },
    { sPkwdBriefingSet03Resources, NELEMS(sPkwdBriefingSet03Resources), &sPkwdBriefingSet03Actor, 1 },
    { sPkwdBriefingSet11Resources, NELEMS(sPkwdBriefingSet11Resources), &sPkwdBriefingSet11Actor, 1 },
    { sPkwdBriefingSet12Resources, NELEMS(sPkwdBriefingSet12Resources), &sPkwdBriefingSet12Actor, 1 },
    { sPkwdBriefingSet13Resources, NELEMS(sPkwdBriefingSet13Resources), &sPkwdBriefingSet13Actor, 1 },
    { sPkwdBriefingSet14Resources, NELEMS(sPkwdBriefingSet14Resources), &sPkwdBriefingSet14Actor, 1 },
    { sPkwdBriefingSet15Resources, NELEMS(sPkwdBriefingSet15Resources), &sPkwdBriefingSet15Actor, 1 },
    { sPkwdBriefingSet16Resources, NELEMS(sPkwdBriefingSet16Resources), &sPkwdBriefingSet16Actor, 1 },
    { sPkwdBriefingSet17Resources, NELEMS(sPkwdBriefingSet17Resources), &sPkwdBriefingSet17Actor, 1 },
    { sPkwdBriefingSet18Resources, NELEMS(sPkwdBriefingSet18Resources), &sPkwdBriefingSet18Actor, 1 },
    { sPkwdBriefingSet05Resources, NELEMS(sPkwdBriefingSet05Resources), &sPkwdBriefingSet05Actor, 1 },
    { sPkwdBriefingSet19Resources, NELEMS(sPkwdBriefingSet19Resources), &sPkwdBriefingSet19Actor, 1 },
    { sPkwdBriefingSet02Resources, NELEMS(sPkwdBriefingSet02Resources), &sPkwdBriefingSet02Actor, 1 },
    { sPkwdBriefingSet08Resources, NELEMS(sPkwdBriefingSet08Resources), &sPkwdBriefingSet08Actor, 1 },
    { sPkwdBriefingSet20Resources, NELEMS(sPkwdBriefingSet20Resources), &sPkwdBriefingSet20Actor, 1 },
    { sPkwdBriefingSet21Resources, NELEMS(sPkwdBriefingSet21Resources), &sPkwdBriefingSet21Actor, 1 },
    { sPkwdBriefingSet22Resources, NELEMS(sPkwdBriefingSet22Resources), &sPkwdBriefingSet22Actor, 1 },
    { sPkwdBriefingSet23Resources, NELEMS(sPkwdBriefingSet23Resources), &sPkwdBriefingSet23Actor, 1 },
    { sPkwdBriefingSet03Resources, NELEMS(sPkwdBriefingSet03Resources), &sPkwdBriefingSet03Actor, 1 },
    { sPkwdBriefingSet24Resources, NELEMS(sPkwdBriefingSet24Resources), &sPkwdBriefingSet24Actor, 1 },
    { sPkwdBriefingSet25Resources, NELEMS(sPkwdBriefingSet25Resources), &sPkwdBriefingSet25Actor, 1 },
    { sPkwdBriefingSet26Resources, NELEMS(sPkwdBriefingSet26Resources), &sPkwdBriefingSet26Actor, 1 },
    { sPkwdBriefingSet27Resources, NELEMS(sPkwdBriefingSet27Resources), &sPkwdBriefingSet27Actor, 1 },
    { sPkwdBriefingSet27Resources, NELEMS(sPkwdBriefingSet27Resources), &sPkwdBriefingSet27Actor, 1 },
    { sPkwdBriefingSet28Resources, NELEMS(sPkwdBriefingSet28Resources), &sPkwdBriefingSet28Actor, 1 },
    { sPkwdBriefingSet07Resources, NELEMS(sPkwdBriefingSet07Resources), &sPkwdBriefingSet07Actor, 1 },
    { sPkwdBriefingSet00Resources, NELEMS(sPkwdBriefingSet00Resources), &sPkwdBriefingSet00Actor, 1 },
};

// BG 6, the info window
static const BGSetup sPkwdBriefingInfoBGSetup = { 0,
                                                  0,
                                                  0x800,
                                                  0,
                                                  BGRES_256x256,
                                                  GX_BG_COLORMODE_16,
                                                  GX_BG_SCRBASE(0x1000),
                                                  GX_BG_CHARBASE(0x4000),
                                                  0x4000,
                                                  GX_BG_EXTPLTT_01,
                                                  0,
                                                  GX_BG_AREAOVER_XLU,
                                                  0 };

static const SRTMatrix sPkwdBriefingSetMatrix = {
    { 0, 0, 0 },
    { FX32_ONE, FX32_ONE, FX32_ONE },
    { FX32_ONE, 0, 0, 0, FX32_ONE, 0, 0, 0, FX32_ONE },
};

static const VecFx32 sPkwdBriefingCameraTarget = { 0, 0x299a, 0 };

static const s16 sPkwdBriefingPatternFrames1[] = { 0, 0, 0, 0 };

static const LightSetup sPkwdBriefingLights[] = {
    { 0, { { 4095, -2047, -4095 }, GX_RGB(31, 31, 31) } },
    { 1, { { -4095, -4095, -4095 }, GX_RGB(31, 31, 31) } },
    { 2, { { -4095, -4095, -4095 }, GX_RGB(31, 31, 31) } },
    { 3, { { -4095, -4095, -4095 }, GX_RGB(31, 31, 31) } },
};

static const LightSetupList sPkwdBriefingLightList = { sPkwdBriefingLights, NELEMS(sPkwdBriefingLights) };

static const VecFx32 sPkwdBriefingCameraPos = { 0x6b33, 0x6b33, 0xc4cd };

static const VecFx32 sPkwdBriefingCameraUp = { 0, FX32_ONE, 0 };

// clang-format off
static const PkwdBriefingSeat sPkwdBriefingSeats[SEAT_COUNT] = {
    { 32, 104, 3 },
    { 80, 104, 3 },
    { 128, 104, 3 },
    { 176, 104, 3 },
    { 224, 104, 3 },
    { 8, 136, 2 },
    { 56, 136, 2 },
    { 104, 136, 2 },
    { 152, 136, 2 },
    { 200, 136, 2 },
    { 248, 136, 2 },
    { 32, 168, 1 },
    { 80, 168, 1 },
    { 128, 168, 1 },
    { 176, 168, 1 },
    { 224, 168, 1 },
    { 8, 200, 0 },
    { 56, 200, 0 },
    { 104, 200, 0 },
    { 152, 200, 0 },
    { 200, 200, 0 },
    { 248, 200, 0 },
};

// clang-format on

static const s16 *sPkwdBriefingPatternFrames[AUDIENCE_PATTERN_COUNT] = {
    sPkwdBriefingPatternFrames0,
    sPkwdBriefingPatternFrames1,
    sPkwdBriefingPatternFrames2,
};

// BG 1, the text
static const BGSetup sPkwdBriefingTextBGSetup = { 0,
                                                  0,
                                                  0x1000,
                                                  0,
                                                  BGRES_256x512,
                                                  GX_BG_COLORMODE_16,
                                                  GX_BG_SCRBASE(0x0000),
                                                  GX_BG_CHARBASE(0x10000),
                                                  0x8000,
                                                  GX_BG_EXTPLTT_01,
                                                  0,
                                                  GX_BG_AREAOVER_XLU,
                                                  0 };

static BOOL PkwdBriefing_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdBriefingParam *briefingParam = param;
    PkwdBriefingWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PKWD_BRIEFING, 0x120000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PkwdBriefingWork), HEAPID_PKWD_BRIEFING);
    MI_CpuFill8(wk, 0, sizeof(PkwdBriefingWork));
    wk->heapId = HEAPID_PKWD_BRIEFING;
    wk->param = briefingParam;
    briefingParam->skip = FALSE;
    wk->sex = getTrainerGender(wk->param->playerInfo);
    wk->lineWait = PkwdBriefing_GetLineWait(wk);
    GFL_OvlLoad(OVERLAY_ID(168));
    return TRUE;
}

// Ends the screen when the B Button is pressed
static void PkwdBriefing_CheckSkip(PkwdBriefingWork *wk) {
    if (wk->param->noSkip == FALSE && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
        wk->state = STATE_WAIT_VOICE;
        wk->param->skip = TRUE;
        GFL_BGSysSetBGEnabled(6, TRUE);
    }
}

static BOOL PkwdBriefing_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdBriefingWork *wk = work;

    switch (wk->state) {
    case STATE_INIT:
        PkwdBriefing_ResetDisplay();
        PkwdBriefing_SetVRAM();
        PkwdBriefing_InitWindow();
        wk->state = STATE_LOAD;
        break;
    case STATE_LOAD:
        PkwdBriefing_GetParties(wk);
        PkwdBriefingSystem_Init(&wk->system, wk, wk->heapId);
        PkwdBriefingText_InitBG(&wk->text, wk->param->movie, wk->heapId);
        PkwdBriefingSet_Init(&wk->set, wk->heapId, wk->param->movie);
        PkwdBriefingClAct_Init(&wk->clact, wk->heapId);
        PkwdBriefing_InitAudience(wk, TRUE, wk->heapId);
        PkwdBriefingText_LoadBriefing(&wk->text, wk->param->movie, wk->param->playerInfo, wk->heapId);
        PkwdBriefingText_CreateBriefingWindows(&wk->text, wk->heapId);
        PkwdBriefingText_CountLines(&wk->text, wk->sex);
        PkwdBriefingText_PrintLine(&wk->text, 0, wk->sex);
        InfoWin_Init(6, 15, wk->param->gameData, wk->heapId);
        GFL_NetSetIconScreen(FALSE, wk->heapId);
        GFL_FadeSet(3, 16, 0, 2);
        wk->state = STATE_WAIT_FADE_IN;
        wk->lastVBlank = OS_GetVBlankCount();
        break;
    case STATE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_RUN;
        }
        PkwdBriefing_Update(wk);
        break;
    case STATE_RUN:
        PkwdBriefing_Update(wk);
        PkwdBriefing_CheckSkip(wk);
        PkwdBriefing_RunBriefing(wk);
        break;
    case STATE_WAIT_VOICE:
        if (!PokeVoice_IsPlaying(wk->voice)) {
            wk->state = STATE_FADE_OUT;
        }
        PkwdBriefing_Update(wk);
        break;
    case STATE_FADE_OUT:
        GFL_FadeSet(3, 0, 16, 2);
        wk->state = STATE_WAIT_FADE_OUT;
        PkwdBriefing_Update(wk);
        break;
    case STATE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning() && !GFL_SndBGMIsFading()) {
            wk->state = STATE_FREE;
        }
        PkwdBriefing_Update(wk);
        break;
    case STATE_FREE:
        PkwdBriefingText_FreeBriefingWindows(&wk->text);
        PkwdBriefingText_FreeBriefing(&wk->text);
        PkwdBriefing_FreeParties(wk);
        InfoWin_Exit();
        PkwdBriefingAudience_Free(&wk->audience);
        PkwdBriefingClAct_Free(&wk->clact);
        PkwdBriefingSet_Free(&wk->set);
        PkwdBriefingText_FreeBG(&wk->text);
        PkwdBriefingSystem_Free(&wk->system);
        return TRUE;
    case STATE_DONE:
        return TRUE;
    }
    return FALSE;
}

// Takes the parties of the movie's battle
static void PkwdBriefing_GetParties(PkwdBriefingWork *wk) {
    if (wk->param->setup != NULL) {
        wk->text.playerParty = wk->param->setup->party[0];
        wk->text.enemyParty = wk->param->setup->party[1];
    }
}

static void PkwdBriefing_FreeParties(PkwdBriefingWork *wk) {
}

static BOOL PkwdBriefing_CreditsMain(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdBriefingWork *wk = work;
    s32 i;

    switch (wk->state) {
    case STATE_INIT:
        PkwdBriefing_ResetDisplay();
        PkwdBriefing_SetVRAM();
        PkwdBriefing_InitWindow();
        wk->state = STATE_LOAD;
        wk->layout = GFL_RandomLC(CREDITS_LAYOUT_COUNT);
        break;
    case STATE_LOAD:
        PkwdBriefing_GetParties(wk);
        PkwdBriefingSystem_InitCredits(&wk->system, wk, wk->heapId);
        PkwdBriefingText_InitBG(&wk->text, wk->param->movie, wk->heapId);
        PkwdBriefingSet_Init(&wk->set, wk->heapId, wk->param->movie);
        PkwdBriefingClAct_Init(&wk->clact, wk->heapId);
        PkwdBriefingText_LoadCreditsBG(&wk->text, wk->heapId, wk->layout);
        PkwdBriefing_InitAudience(wk, FALSE, wk->heapId);
        PkwdBriefing_LoadCredits(wk->param, &wk->text, wk->param->movie, wk->heapId);
        PkwdBriefingText_CreateCreditsWindows(&wk->text, wk->heapId, wk->layout);
        InfoWin_Init(6, 15, wk->param->gameData, wk->heapId);
        GFL_NetSetIconScreen(FALSE, wk->heapId);
        wk->state = STATE_WAIT_PRINT;
        wk->lastVBlank = OS_GetVBlankCount();
        wk->text.scroll = -192;
        PkwdBriefingText_ShowCreditsWindows(&wk->text);
        if ((wk->text.line & 1) == 0) {
            GFL_BGSysSetBGEnabled(1, TRUE);
            GFL_BGSysSetBGEnabled(2, FALSE);
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 0, 16);
        } else {
            GFL_BGSysSetBGEnabled(1, FALSE);
            GFL_BGSysSetBGEnabled(2, TRUE);
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG0, 0, 16);
        }
        break;
    case STATE_WAIT_PRINT:
        if (PrintQueue_Main(wk->text.printQueue) == TRUE) {
            GFL_FadeSet(3, 16, 0, 2);
            wk->state = STATE_WAIT_FADE_IN;
        }
        for (i = 0; i < wk->text.windowCount; i++) {
            PrintWindow_Flush(&wk->text.printWindows[i], wk->text.printQueue);
        }
        break;
    case STATE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_RUN;
        }
        PkwdBriefing_Update(wk);
        PrintQueue_Main(wk->text.printQueue);
        for (i = 0; i < wk->text.windowCount; i++) {
            PrintWindow_Flush(&wk->text.printWindows[i], wk->text.printQueue);
        }
        break;
    case STATE_RUN:
        PkwdBriefing_Update(wk);
        PkwdBriefing_CheckSkip(wk);
        PkwdBriefing_RunCredits(wk);
        PrintQueue_Main(wk->text.printQueue);
        for (i = 0; i < wk->text.windowCount; i++) {
            PrintWindow_Flush(&wk->text.printWindows[i], wk->text.printQueue);
        }
        break;
    case STATE_WAIT_VOICE:
        if (!PokeVoice_IsPlaying(wk->voice)) {
            wk->state = STATE_FADE_OUT;
        }
        PkwdBriefing_Update(wk);
        break;
    case STATE_FADE_OUT:
        GFL_FadeSet(3, 0, 16, 2);
        wk->state = STATE_WAIT_FADE_OUT;
        PkwdBriefing_Update(wk);
        break;
    case STATE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning() && !GFL_SndBGMIsFading()) {
            wk->state = STATE_FREE;
        }
        PkwdBriefing_Update(wk);
        break;
    case STATE_FREE:
        PkwdBriefingText_FreeCreditsWindows(&wk->text);
        PkwdBriefingText_FreeCredits(&wk->text);
        PkwdBriefing_FreeParties(wk);
        InfoWin_Exit();
        PkwdBriefingAudience_Free(&wk->audience);
        PkwdBriefingClAct_Free(&wk->clact);
        PkwdBriefingSet_Free(&wk->set);
        PkwdBriefingText_FreeBG(&wk->text);
        PkwdBriefingSystem_Free(&wk->system);
        return TRUE;
    case STATE_DONE:
        return TRUE;
    }
    return FALSE;
}

static BOOL PkwdBriefing_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_PKWD_BRIEFING);
    GFL_OvlUnload(OVERLAY_ID(168));
    return TRUE;
}

static void PkwdBriefing_ResetDisplay(void) {
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

static void PkwdBriefing_SetVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sPkwdBriefingVRAMConfig);
    GFL_BGSysSetLCDConfig(&sPkwdBriefingLCDConfig);
}

// Runs what does not depend on the state, with the camera catching up on the frames that were dropped
static void PkwdBriefing_Update(PkwdBriefingWork *wk) {
    u32 vblank = OS_GetVBlankCount();
    s32 frames = vblank - wk->lastVBlank;

    wk->lastVBlank = vblank;
    PkwdBriefingSystem_Update(&wk->system, wk->heapId);
    do {
        PkwdBriefingSet_StepCamera(&wk->set, wk->heapId);
    } while (--frames != 0);
    PkwdBriefingSet_Draw(&wk->set, wk->heapId, wk->param->movie);
    PkwdBriefingClAct_Update(&wk->clact, wk->heapId);
    PkwdBriefing_UpdateAudience(wk);
    InfoWin_Update();
}

static void PkwdBriefing_VBlank(TCB *tcb, void *data) {
    PkwdBriefingWork *wk = data;

    PrintQueue_Main(wk->text.printQueue);
    PrintWindow_Flush(&wk->text.printWindows[0], wk->text.printQueue);
    PrintWindow_Flush(&wk->text.printWindows[1], wk->text.printQueue);
    ClActSys_VBlank();
    GFL_BGSysUpdate();
}

static void PkwdBriefing_CreditsVBlank(TCB *tcb, void *data) {
    PkwdBriefingWork *wk = data;

    if (wk->text.endScroll != 0) {
        G2_SetBG3Offset(0, wk->text.endScroll);
    }
    G2_SetBG1Offset(0, wk->text.scroll);
    G2_SetBG2Offset(0, wk->text.scroll + 16);
    ClActSys_VBlank();
    GFL_BGSysUpdate();
}

static void PkwdBriefingSystem_Init(PkwdBriefingSystem *system, PkwdBriefingWork *wk, HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_G3DSysCreate(FALSE, 2, FALSE, 2, 0x1000, heapId, NULL);
    GFL_TextRndInitColorIndexLUT();
    system->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 4, 32);
    system->vblankTask = GFL_VBlankTCBAdd(PkwdBriefing_VBlank, wk, 5);
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
}

static void PkwdBriefingSystem_InitCredits(PkwdBriefingSystem *system, PkwdBriefingWork *wk, HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_G3DSysCreate(FALSE, 2, FALSE, 2, 0x1000, heapId, NULL);
    GFL_TextRndInitColorIndexLUT();
    system->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 4, 32);
    system->vblankTask = GFL_VBlankTCBAdd(PkwdBriefing_CreditsVBlank, wk, 5);
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
}

static void PkwdBriefingSystem_Update(PkwdBriefingSystem *system, HeapID heapId) {
    GFL_TCBExMgrUpdate(system->tcbManager);
}

static void PkwdBriefingSystem_Free(PkwdBriefingSystem *system) {
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
    GFL_TCBRemove(system->vblankTask);
    GFL_TCBExMgrFree(system->tcbManager);
    GFL_G3DSysFree();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void PkwdBriefingText_InitBG(PkwdBriefingText *text, s32 movie, HeapID heapId) {
    GFL_BGSysCreateBG(1, &sPkwdBriefingTextBGSetup, BGMODE_TEXT);
    GFL_BGSysCreateBG(2, &sPkwdBriefingText2BGSetup, BGMODE_TEXT);
    GFL_BGSysCreateBG(3, &sPkwdBriefingFrameBGSetup, BGMODE_TEXT);
    GFL_BGSysCreateBG(5, &sPkwdBriefingSeatsBGSetup, BGMODE_TEXT);
    GFL_BGSysCreateBG(6, &sPkwdBriefingInfoBGSetup, BGMODE_TEXT);
    GFL_BGSysLoadNCGRStatic(ARCID_PKWD_COUNT, FILE_TEXT_CHARS, 1, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_PKWD_COUNT, FILE_TEXT_SCREEN, 1, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_PKWD_COUNT, FILE_TEXT_CHARS, 2, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_PKWD_COUNT, FILE_TEXT_SCREEN, 2, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_PKWD_COUNT, FILE_PALETTE, 0, 0, 0, heapId);
    GFL_BGSysLoadScr(3);
    GFL_BGSysSetBGEnabled(3, FALSE);
    GFL_BGSysSetBGEnabled(1, FALSE);
    GFL_BGSysSetBGEnabled(2, FALSE);
    text->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    text->printQueue = PrintQueue_CreateDefault(heapId);
    text->unk1B4 = 1;
}

static void PkwdBriefingText_LoadCreditsBG(PkwdBriefingText *text, HeapID heapId, u32 layout) {
    GFL_BGSysClearCharCore(1, 0x20, 0, heapId);
    GFL_BGSysClearCharCore(2, 0x20, 0, heapId);
    GFL_BGSysClearScr(1);
    GFL_BGSysClearScr(2);
    GFL_BGSysLoadNCLRDefault(ARCID_PKWD_COUNT, FILE_CREDITS_PALETTE, 0, 0, 0, heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_PKWD_COUNT, FILE_CREDITS_CHARS, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_PKWD_COUNT, sPkwdBriefingCreditsScreens[layout], 3, 0, 0, FALSE,
                                           heapId);
    GFL_BGSysLoadScr(3);
    GFL_BGSysSetBGEnabled(3, TRUE);
}

static void PkwdBriefingText_LoadCreditsScreen(PkwdBriefingText *text, HeapID heapId, u32 layout) {
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_PKWD_COUNT, sPkwdBriefingCreditsSideScreens[layout], 3, 0, 0, FALSE,
                                           heapId);
    GFL_BGSysLoadScr(3);
}

static void PkwdBriefingText_LoadCreditsEndScreen(PkwdBriefingText *text, HeapID heapId) {
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_PKWD_COUNT, FILE_CREDITS_END_SCREEN, 3, 0, 0, FALSE, heapId);
}

static void PkwdBriefingText_FreeBG(PkwdBriefingText *text) {
    PrintQueue_Free(text->printQueue);
    GFL_FontFree(text->font);
}

static void PkwdBriefingText_LoadBriefing(PkwdBriefingText *text, s32 movie, PlayerInfo *playerInfo, HeapID heapId) {
    text->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PKWD_BRIEFING_MOVIE_00 + movie, heapId);
    text->lines[0] = GFL_StrBufCreate(304, heapId);
    text->lines[1] = GFL_StrBufCreate(304, heapId);
    text->empty = GFL_StrBufCreate(304, heapId);
    GFL_MsgDataLoadStrbuf(text->msgData, LINE_END, text->empty);
    text->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    setPartyPokemonSpeciesNameToStrbuf(text->wordSet, 0, PokeParty_GetPkm(text->playerParty, 0));
    copyVarForText(text->wordSet, 1, playerInfo);
}

static void PkwdBriefingText_FreeBriefing(PkwdBriefingText *text) {
    GFL_StrBufFree(text->empty);
    GFL_StrBufFree(text->lines[1]);
    GFL_StrBufFree(text->lines[0]);
    GFL_WordSetSystemFree(text->wordSet);
    GFL_MsgDataFree(text->msgData);
}

static void PkwdBriefingText_SwapCredits(PkwdBriefingText *text, s32 a, s32 b) {
    StrBuf *strbuf = text->credits[a];

    text->credits[a] = text->credits[b];
    text->credits[b] = strbuf;
}

// Makes the actor the star, with the player's role and Pokémon in the actor's place, unless the studio's level is
// above the one that the movie asks for
static void PkwdBriefingText_SetStar(PkwdBriefingText *text, s32 level, s32 starLevel) {
    if (level <= starLevel) {
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_ROLE, CREDIT_ACTOR_NAME);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_NAME, CREDIT_ACTOR_NAME_2);
        PkwdBriefingText_SwapCredits(text, CREDIT_MOVIE + 1, CREDIT_MOVIE + 0);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_POKEMON + 0, CREDIT_ENEMY_POKEMON + 0);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_POKEMON + 1, CREDIT_ENEMY_POKEMON + 1);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_POKEMON + 2, CREDIT_ENEMY_POKEMON + 2);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_POKEMON + 3, CREDIT_ENEMY_POKEMON + 3);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_POKEMON + 4, CREDIT_ENEMY_POKEMON + 4);
        PkwdBriefingText_SwapCredits(text, CREDIT_PLAYER_POKEMON + 5, CREDIT_ENEMY_POKEMON + 5);
        text->swapped = TRUE;
    } else {
        text->swapped = FALSE;
    }
}

// The name that the credits give the player: the player's own, or an actor's in the last movie
static StrBuf *PkwdBriefing_CreatePlayerName(PkwdBriefingParam *param, HeapID heapId) {
    StrBuf *name;

    if (param->movie < MOVIE_NO_PLAYER) {
        name = copyTrainerNameToNewStrbuf(param->playerInfo->name, heapId);
    } else {
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_PASS_POWER_STARTMENU, HEAPID_TAIL(heapId));
        StrBuf *compressed;

        name = GFL_StrBufCreate(16, heapId);
        compressed = GFL_MsgDataLoadStrbufNew(msgData, EventPassPowerStartmenu_Text_PopRoxie);
        GFL_StrBufUncompress(name, compressed);
        GFL_StrBufFree(compressed);
        GFL_MsgDataFree(msgData);
    }
    return name;
}

static void PkwdBriefing_LoadCredits(PkwdBriefingParam *param, PkwdBriefingText *text, s32 movie, HeapID heapId) {
    u32 sex = getTrainerGender(param->playerInfo);
    PokewoodSave *save = PokewoodSave_Get(param->gameData);
    s32 i;
    s32 starLevel;
    MsgData *msgData;
    s32 actorName2;
    s32 actorName;
    StrBuf *strbuf;
    PkwdBriefingMovieFile *file;

    text->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PKWD_BRIEFING_COMMON, heapId);
    text->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    text->empty = GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_Empty_327);
    text->unk1D4 = GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_Empty_328);
    for (i = 0; i < 15; i++) {
        text->credits[CREDIT_MOVIE + i] = GFL_MsgDataLoadStrbufNew(text->msgData, movie * 15 + i);
    }
    text->credits[CREDIT_SCREENPLAY] = GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_Screenplay);
    text->credits[CREDIT_VFX] = GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_Vfx);
    text->credits[CREDIT_ART] = GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_PokestarStudiosArt);
    text->credits[CREDIT_DIRECTOR] = GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_Director);
    text->credits[CREDIT_PLAYER_ROLE] =
        GFL_MsgDataLoadStrbufNew(text->msgData, movie * 15 + PkwdBriefingCommon_Text_Kid + sex);
    text->credits[CREDIT_PLAYER_NAME] = PkwdBriefing_CreatePlayerName(param, heapId);

    msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_PASS_POWER_STARTMENU, HEAPID_TAIL(heapId));
    actorName2 = 0;
    if (sex == 0) {
        actorName = EventPassPowerStartmenu_Text_BrycenMan;
    } else {
        actorName2 = EventPassPowerStartmenu_Text_Brycen_8;
        actorName = EventPassPowerStartmenu_Text_BrycenMan_5;
    }
    text->credits[CREDIT_ACTOR_NAME] = GFL_StrBufCreate(16, heapId);
    text->credits[CREDIT_ACTOR_NAME_2] = GFL_StrBufCreate(16, heapId);
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, actorName + movie);
    GFL_StrBufUncompress(text->credits[CREDIT_ACTOR_NAME], strbuf);
    GFL_StrBufFree(strbuf);
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, actorName2 + movie);
    GFL_StrBufUncompress(text->credits[CREDIT_ACTOR_NAME_2], strbuf);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);

    file = GFL_ArcSysReadHeapNew(ARCID_PKWD_MOVIE, movie, HEAPID_TAIL(heapId));
    text->credits[CREDIT_DIRECTOR_NAME] =
        GFL_MsgDataLoadStrbufNew(text->msgData, PkwdBriefingCommon_Text_DavidPokench + file->director);
    starLevel = file->starLevel;
    GFL_HeapFree(file);

    for (i = 0; i < PokeParty_GetPkmCount(text->playerParty); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(text->playerParty, i);

        text->credits[CREDIT_PLAYER_POKEMON + i] = GFL_StrBufCreate(12, heapId);
        PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, text->credits[CREDIT_PLAYER_POKEMON + i]);
    }
    setPartyPokemonSpeciesNameToStrbuf(text->wordSet, 0, PokeParty_GetPkm(text->playerParty, 0));
    for (i = 0; i < PokeParty_GetPkmCount(text->enemyParty); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(text->enemyParty, i);

        text->credits[CREDIT_ENEMY_POKEMON + i] = GFL_StrBufCreate(12, heapId);
        PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, text->credits[CREDIT_ENEMY_POKEMON + i]);
    }

    strbuf = GFL_StrBufCreate(25, heapId);
    GFL_WordSetFormatStrbuf(text->wordSet, strbuf, text->credits[CREDIT_PLAYER_ROLE]);
    GFL_StrBufFree(text->credits[CREDIT_PLAYER_ROLE]);
    text->credits[CREDIT_PLAYER_ROLE] = strbuf;
    PkwdBriefingText_SetStar(text, PokewoodSave_GetLevelOfPoints(PokewoodSave_GetLevelPoints(save)), starLevel);
}

static void PkwdBriefingText_FreeCredits(PkwdBriefingText *text) {
    s32 i;

    for (i = 0; i < CREDIT_COUNT; i++) {
        if (text->credits[i] != NULL) {
            GFL_StrBufFree(text->credits[i]);
        }
    }
    GFL_StrBufFree(text->unk1D4);
    GFL_StrBufFree(text->empty);
    GFL_WordSetSystemFree(text->wordSet);
    GFL_MsgDataFree(text->msgData);
}

static void PkwdBriefingText_CreateBriefingWindows(PkwdBriefingText *text, HeapID heapId) {
    s32 i;

    text->windows[0] = BmpWin_CreateDynamic(1, 2, 4, 28, 16, 0, TRUE);
    text->windows[1] = BmpWin_CreateDynamic(2, 2, 4, 28, 16, 0, TRUE);
    for (i = 0; i < 2; i++) {
        PrintWindow_Init(&text->printWindows[i], text->windows[i]);
        GFL_BitmapFill(BmpWin_GetBitmap(text->windows[i]), 0xee);
        BmpWin_FlushMap(text->windows[i]);
        BmpWin_FlushChar(text->windows[i]);
    }
    GFL_BGSysLoadScr(1);
    GFL_BGSysLoadScr(2);
}

static void PkwdBriefingText_FreeBriefingWindows(PkwdBriefingText *text) {
    BmpWin_Free(text->windows[1]);
    BmpWin_Free(text->windows[0]);
}

// Prints a string in a window, from its left or ending at the middle of the screen
static void PkwdBriefingText_Print(PkwdBriefingText *text, s32 window, StrBuf *strbuf, u32 rightAligned) {
    s32 x = 0;

    if (rightAligned == TRUE) {
        x = 120 - GFL_FontGetBlockWidth(strbuf, text->font, 0);
    }
    PrintWindow_Print(&text->printWindows[window], text->printQueue, x, 0, strbuf, text->font, PRINT_COLOR(1, 2, 0));
}

// The string of a line of the credits, or NULL if it has none
static StrBuf *PkwdBriefingText_GetCredit(PkwdBriefingText *text, u32 line) {
    u32 source = sPkwdBriefingCreditSources[line];
    StrBuf *strbuf;
    int slot;

    switch (source) {
    case SOURCE_PLAYER_ROLE:
        strbuf = text->credits[CREDIT_PLAYER_ROLE];
        break;
    case SOURCE_PLAYER_NAME:
        strbuf = text->credits[CREDIT_PLAYER_NAME];
        break;
    case SOURCE_ACTOR_NAME:
        strbuf = text->credits[CREDIT_ACTOR_NAME];
        break;
    case SOURCE_ACTOR_NAME_2:
        strbuf = text->credits[CREDIT_ACTOR_NAME_2];
        break;
    case SOURCE_PLAYER_POKEMON + 0:
    case SOURCE_PLAYER_POKEMON + 1:
    case SOURCE_PLAYER_POKEMON + 2:
    case SOURCE_PLAYER_POKEMON + 3:
    case SOURCE_PLAYER_POKEMON + 4:
    case SOURCE_PLAYER_POKEMON + 5:
        slot = source - SOURCE_PLAYER_POKEMON;
        strbuf = text->credits[CREDIT_PLAYER_POKEMON + slot];
        break;
    case SOURCE_ENEMY_POKEMON + 0:
    case SOURCE_ENEMY_POKEMON + 1:
    case SOURCE_ENEMY_POKEMON + 2:
    case SOURCE_ENEMY_POKEMON + 3:
    case SOURCE_ENEMY_POKEMON + 4:
    case SOURCE_ENEMY_POKEMON + 5:
        slot = source - SOURCE_ENEMY_POKEMON;
        strbuf = text->credits[CREDIT_ENEMY_POKEMON + slot];
        break;
    case SOURCE_STAR_NAME:
        if (text->swapped) {
            strbuf = text->credits[CREDIT_ACTOR_NAME_2];
        } else {
            strbuf = text->credits[CREDIT_PLAYER_NAME];
        }
        break;
    default:
        strbuf = text->credits[source];
        if (GFL_StrBufCmp(strbuf, text->empty) == TRUE) {
            strbuf = NULL;
        }
        break;
    }
    return strbuf;
}

static void PkwdBriefingText_PrintCredit(PkwdBriefingText *text, s32 window, u32 line, u32 rightAligned,
                                         HeapID heapId) {
    StrBuf *strbuf = PkwdBriefingText_GetCredit(text, line);

    if (strbuf != NULL) {
        PkwdBriefingText_Print(text, window, strbuf, rightAligned);
    }
}

// Makes a window for each line of the credits that has text and prints it: the first 62 tiles on BG 1, the rest on
// BG 2
static void PkwdBriefingText_CreateCreditsWindows(PkwdBriefingText *text, HeapID heapId, u32 layout) {
    int count = 0;
    s32 y = 5;
    s32 i;
    const PkwdBriefingCreditsLine *lines = sPkwdBriefingCreditsBoth;

    if (layout == CREDITS_RIGHT) {
        lines = sPkwdBriefingCreditsRight;
        PkwdBriefing_SetCreditsWindow(0);
    } else if (layout == CREDITS_LEFT) {
        lines = sPkwdBriefingCreditsLeft;
        PkwdBriefing_SetCreditsWindow(1);
    }
    for (i = 0; lines[i].line != CREDITS_END; i++) {
        if (lines[i].line < CREDITS_SPACE) {
            if (PkwdBriefingText_GetCredit(text, lines[i].line) != NULL) {
                if (y <= 62) {
                    text->windows[count] = BmpWin_CreateDynamic(1, lines[i].x, y, 15, 2, 0, TRUE);
                } else {
                    text->windows[count] = BmpWin_CreateDynamic(2, lines[i].x, y - 62, 15, 2, 0, TRUE);
                }
                PrintWindow_Init(&text->printWindows[count], text->windows[count]);
                GFL_BitmapFill(BmpWin_GetBitmap(text->windows[count]), 0);
                PkwdBriefingText_PrintCredit(text, count, lines[i].line, lines[i].rightAligned, heapId);
                count++;
                y += lines[i].height;
            }
        } else if (lines[i].line == CREDITS_SPACE) {
            y += lines[i].height;
        }
    }
    text->windowCount = count;
    text->height = y;
    GFL_BGSysLoadScr(1);
    GFL_BGSysLoadScr(2);
}

static void PkwdBriefingText_FreeCreditsWindows(PkwdBriefingText *text) {
    s32 i;

    for (i = 0; i < text->windowCount; i++) {
        BmpWin_Free(text->windows[i]);
    }
}

static void PkwdBriefingSet_Init(PkwdBriefingSet *set, HeapID heapId, s32 movie) {
    s32 first;

    set->manager =
        GFL_G3DMgrCreate(sPkwdBriefingScenes[movie].resourceCount, sPkwdBriefingScenes[movie].actorCount, heapId);
    set->scene = GFL_G3DMgrNewScene(set->manager, &sPkwdBriefingScenes[movie]);
    first = GFL_G3DMgrGetSceneFirstActorIdx(set->manager, set->scene);
    GFL_G3DActorBindAnm(GFL_G3DMgrGetActor(set->manager, first), 0);
    set->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, 0x399, 0xf97, 0x159a, 0, FX32_ONE, 512 * FX32_ONE, 0,
                            &sPkwdBriefingCameraPos, &sPkwdBriefingCameraUp, &sPkwdBriefingCameraTarget, heapId);
    set->curve = GFL_G3DCurveLoadFileAll(heapId, ARCID_PKWD_COUNT, FILE_CAMERA);
    set->light = GFL_G3DLightCreate(&sPkwdBriefingLightList, heapId);
    GFL_G3DLightFlush(set->light);
    GFL_BGSysSet3DBGPriority(3);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
}

static void PkwdBriefingSet_StepCamera(PkwdBriefingSet *set, HeapID heapId) {
    GFL_G3DCurveFrameStep(set->curve, FX32_ONE);
}

static void PkwdBriefingSet_Draw(PkwdBriefingSet *set, HeapID heapId, int movie) {
    s32 first = GFL_G3DMgrGetSceneFirstActorIdx(set->manager, set->scene);
    s32 i;

    GFL_G3DCurveApplyCamera(set->camera, set->curve);
    GFL_G3DCameraFlush(set->camera);
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    for (i = 0; i < sPkwdBriefingScenes[movie].actorCount; i++) {
        GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(set->manager, first + i), &sPkwdBriefingSetMatrix);
    }
    GFL_G3DSysReqSwapBuffers();
    for (i = 0; i < sPkwdBriefingScenes[movie].actorCount; i++) {
        GFL_G3DActorStepAnmFrameLoop(GFL_G3DMgrGetActor(set->manager, first + i), 0, FX32_ONE);
    }
}

static void PkwdBriefingSet_Free(PkwdBriefingSet *set) {
    GFL_G3DCurveFree(set->curve);
    GFL_G3DCameraFree(set->camera);
    GFL_G3DLightFree(set->light);
    GFL_G3DMgrDeleteScene(set->manager, set->scene);
    GFL_G3DMgrFree(set->manager);
}

static void PkwdBriefingClAct_Init(PkwdBriefingClAct *clact, HeapID heapId) {
    ClActSys_Create(&data_02093f08, &sPkwdBriefingVRAMConfig, heapId);
    clact->unit = ClActUnit_Create(64, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

static void PkwdBriefingClAct_Update(PkwdBriefingClAct *clact, HeapID heapId) {
    ClActSys_Draw();
}

static void PkwdBriefingClAct_Free(PkwdBriefingClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
}

// Shows the lines of the briefing one after the other. A line slides in as it fades in and waits for the A Button
// or for the text speed's time
static void PkwdBriefing_RunBriefing(PkwdBriefingWork *wk) {
    switch (wk->step) {
    case BRIEFING_START:
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(2, FALSE);
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 0, 16);
        wk->step = BRIEFING_WAIT;
        break;
    case BRIEFING_WAIT:
        wk->text.timer++;
        if (wk->text.timer == 60) {
            wk->step = BRIEFING_FADE_OUT_BLANK;
        }
        break;
    case BRIEFING_FADE_OUT_BLANK:
        wk->text.alpha++;
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 0, 16 - wk->text.alpha);
        if (wk->text.alpha == 8) {
            wk->step = BRIEFING_WAIT_BLANK;
            wk->text.alpha = 0;
            wk->text.timer = 0;
        }
        break;
    case BRIEFING_WAIT_BLANK:
        if (wk->text.timer++ > 30) {
            wk->step = BRIEFING_START_LINE;
        }
        break;
    case BRIEFING_START_LINE:
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 0, 8);
        wk->step = BRIEFING_FADE_IN_LINE;
        break;
    case BRIEFING_FADE_IN_LINE:
        wk->text.alpha++;
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, wk->text.alpha, 8);
        G2_SetBG1Offset((16 - wk->text.alpha) / 2, 0);
        if (wk->text.alpha == 16) {
            wk->step = BRIEFING_SHOW_LINE;
        }
        break;
    case BRIEFING_SHOW_LINE:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || ++wk->text.timer > wk->lineWait) {
            wk->step = BRIEFING_FADE_OUT_LINE;
            wk->text.timer = 0;
        }
        break;
    case BRIEFING_FADE_OUT_LINE:
        wk->text.alpha--;
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, wk->text.alpha, 8);
        G2_SetBG1Offset(-(16 - wk->text.alpha) / 2, 0);
        if (wk->text.alpha == 0) {
            wk->step = BRIEFING_NEXT_LINE;
        }
        break;
    case BRIEFING_NEXT_LINE:
        wk->text.line++;
        if (PkwdBriefing_IsLastLine(wk)) {
            wk->state = STATE_WAIT_VOICE;
        } else {
            wk->step = BRIEFING_WAIT_PRINT;
            PkwdBriefingText_PrintLine(&wk->text, wk->text.line, wk->sex);
        }
        break;
    case BRIEFING_WAIT_PRINT:
        if (PrintQueue_IsEmpty(wk->text.printQueue) == TRUE) {
            wk->step = BRIEFING_START_LINE;
        }
        break;
    }
}

// Has window 1 cover the screen between the letterbox bars, with every plane and the blending in it
static void PkwdBriefing_InitWindow(void) {
    G2_SetWnd1InsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                              GX_WND_PLANEMASK_OBJ,
                          TRUE);
    G2_SetWndOutsidePlane(0, FALSE);
    GX_SetVisibleWnd(GX_WNDMASK_W1);
    G2_SetWnd1Position(0, 16, 255, 176);
    G2_SetWnd0Position(0, 16, 255, 176);
}

// Has window 0 cover the half of the screen where the credits are, without BG 3 in it
static void PkwdBriefing_SetCreditsWindow(u32 side) {
    G2_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_OBJ,
                          TRUE);
    GX_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    if (side == 0) {
        G2_SetWnd0Position(136, 16, 255, 176);
    } else {
        G2_SetWnd0Position(0, 16, 120, 176);
    }
}

// Prints a line of the briefing in the middle of its window
static void PkwdBriefingText_PrintLine(PkwdBriefingText *text, s32 line, u32 sex) {
    StrBuf *strbuf;
    u8 height;

    if (sex == 0) {
        strbuf = GFL_MsgDataLoadStrbufNew(text->msgData, line);
    } else {
        strbuf = GFL_MsgDataLoadStrbufNew(text->msgData, line + LINE_COUNT);
    }
    GFL_WordSetFormatStrbuf(text->wordSet, text->lines[0], strbuf);
    GFL_StrBufFree(strbuf);
    height = text->lineHeights[line];
    GFL_BitmapFill(BmpWin_GetBitmap(text->windows[0]), 0xee);
    PrintWindow_Print(&text->printWindows[0], text->printQueue, 0, (128 - height * 16) / 2, text->lines[0], text->font,
                      PRINT_COLOR(1, 2, 14));
}

static void PkwdBriefingText_CountLines(PkwdBriefingText *text, u32 sex) {
    s32 i;

    for (i = 0; i < LINE_COUNT; i++) {
        StrBuf *strbuf;

        if (sex == 0) {
            strbuf = GFL_MsgDataLoadStrbufNew(text->msgData, i);
        } else {
            strbuf = GFL_MsgDataLoadStrbufNew(text->msgData, i + LINE_COUNT);
        }
        text->lineHeights[i] = GFL_FontGetLineCount(strbuf);
        GFL_StrBufFree(strbuf);
    }
}

// Whether the briefing has no line left, which the message that ends it shows
static BOOL PkwdBriefing_IsLastLine(PkwdBriefingWork *wk) {
    s32 line = wk->text.line;
    s32 buffer = line % 2;

    if (wk->sex == 0) {
        GFL_MsgDataLoadStrbuf(wk->text.msgData, line, wk->text.lines[buffer]);
    } else {
        GFL_MsgDataLoadStrbuf(wk->text.msgData, line + LINE_COUNT, wk->text.lines[buffer]);
    }
    return GFL_StrBufCmp(wk->text.lines[buffer], wk->text.empty);
}

static void PkwdBriefingAudience_Init(PkwdBriefingAudience *audience, HeapID heapId) {
    s32 i;

    audience->arc = GFL_ArcSysCreateFileHandle(ARCID_BTLV, heapId);
    audience->unit = ClActUnit_Create(SEAT_COUNT, 1, heapId);
    audience->chars = ClActSys_LoadChars(audience->arc, FILE_AUDIENCE_CHARS, FALSE, 1, heapId);
    audience->cellAnims = ClActSys_LoadCellAnims(audience->arc, FILE_AUDIENCE_CELLS, FILE_AUDIENCE_ANIMS, heapId);
    audience->palette = ClActSys_LoadPlttRange(audience->arc, FILE_AUDIENCE_PALETTE, 1, 0, 0, 4, heapId);
    for (i = 0; i < SEAT_COUNT; i++) {
        ClActorSetup base = sPkwdBriefingAudienceSetup;
        ClActorSetup setup = base;

        setup.x = sPkwdBriefingSeats[i].x;
        setup.y = sPkwdBriefingSeats[i].y;
        setup.priority = 128 - i;
        audience->actors[i] = ClActor_Create(audience->unit, audience->chars, audience->palette, audience->cellAnims,
                                             &setup, CLACT_SURFACE_SUB, heapId);
        ClActor_SetAutoAnim(audience->actors[i], FALSE);
        ClActor_SetVisible(audience->actors[i], TRUE);
        ClActor_SetPalette(audience->actors[i], sPkwdBriefingSeats[i].palette, 1);
    }
    PkwdBriefingAudience_LoadPalettes(audience, heapId);
}

static void PkwdBriefingAudience_Free(PkwdBriefingAudience *audience) {
    s32 i;

    // The characters are left to ClActSys's own teardown
    PkwdBriefingAudience_FreePalettes(audience);
    for (i = 0; i < SEAT_COUNT; i++) {
        if (audience->actors[i] != NULL) {
            ClActor_Delete(audience->actors[i]);
            audience->actors[i] = NULL;
        }
    }
    ClActSys_FreeCellAnims(audience->cellAnims);
    ClActSys_FreePltt(audience->palette);
    if (audience->unit != NULL) {
        ClActUnit_Delete(audience->unit);
    }
    GFL_ArcToolFree(audience->arc);
}

static u32 PkwdBriefingAudience_IsSeatTaken(u32 seats, u32 seat) {
    return (seats >> seat) & 1;
}

static void PkwdBriefingAudience_TakeSeat(u32 *seats, u32 seat) {
    *seats |= 1 << seat;
}

static void PkwdBriefingAudience_HideEmptySeats(PkwdBriefingAudience *audience, u32 seats) {
    s32 i;

    for (i = 0; i < SEAT_COUNT; i++) {
        if (!PkwdBriefingAudience_IsSeatTaken(seats, i)) {
            PkwdBriefingAudience_SetVisible(audience, i, FALSE);
            PkwdBriefingAudience_SetSequence(audience, i, AUDIENCE_SIT);
        }
    }
}

// Seats an audience of count at random, and returns its seats
static u32 PkwdBriefingAudience_PickSeats(PkwdBriefingAudience *audience, s32 count) {
    u32 seats = 0;
    s32 taken = 0;

    while (taken != count) {
        u32 seat = GFL_RandomLC(SEAT_COUNT);

        if (!PkwdBriefingAudience_IsSeatTaken(seats, seat)) {
            PkwdBriefingAudience_SetVisible(audience, seat, TRUE);
            PkwdBriefingAudience_SetSequence(audience, seat, AUDIENCE_SIT);
            PkwdBriefingAudience_TakeSeat(&seats, seat);
            taken++;
        }
    }
    PkwdBriefingAudience_HideEmptySeats(audience, seats);
    return seats;
}

static void PkwdBriefingAudience_SetDelays(PkwdBriefingAudience *audience, u32 seats) {
    s32 i;

    for (i = 0; i < SEAT_COUNT; i++) {
        if ((seats >> i) & 1) {
            audience->delays[i] = GFL_RandomLC(90);
        }
    }
}

static void PkwdBriefing_InitAudience(PkwdBriefingWork *wk, BOOL pickSeats, HeapID heapId) {
    PkwdBriefingAudience_Init(&wk->audience, heapId);
    if (pickSeats == TRUE) {
        wk->param->audience = PkwdBriefingAudience_PickSeats(&wk->audience, wk->param->audienceCount);
    } else {
        PkwdBriefingAudience_HideEmptySeats(&wk->audience, wk->param->audience);
    }
    PkwdBriefingAudience_SetDelays(&wk->audience, wk->param->audience);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysLoadArcNCGRStatic(wk->audience.arc, FILE_AUDIENCE_BG_CHARS, 5, 0, 0x400, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(wk->audience.arc, FILE_AUDIENCE_BG_SCREEN, 5, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(wk->audience.arc, FILE_AUDIENCE_BG_PALETTE, 4, 0, 0xe0, heapId);
}

static void PkwdBriefingAudience_SetVisible(PkwdBriefingAudience *audience, s32 seat, BOOL visible) {
    ClActor_SetVisible(audience->actors[seat], visible);
}

static void PkwdBriefingAudience_SetSequence(PkwdBriefingAudience *audience, s32 seat, u32 sequence) {
    ClActor_SetSequence(audience->actors[seat], sequence);
    ClActor_SetPalette(audience->actors[seat], sPkwdBriefingSeats[seat].palette, 1);
}

static void PkwdBriefingAudience_SetAllSequences(PkwdBriefingAudience *audience, u32 sequence, u32 seats) {
    s32 i;

    for (i = 0; i < SEAT_COUNT; i++) {
        if (PkwdBriefingAudience_IsSeatTaken(seats, i)) {
            PkwdBriefingAudience_SetSequence(audience, i, sequence);
        }
    }
}

// Plays the sound of a reaction, which depends on whether more than half of the seats are taken
static void PkwdBriefingAudience_PlaySE(u32 count, s32 sequence) {
    if (count < 12) {
        if (sPkwdBriefingAudienceSE[sequence][0] != 0) {
            GFL_SndSEPlay(sPkwdBriefingAudienceSE[sequence][0]);
        }
    } else {
        if (sPkwdBriefingAudienceSE[sequence][1] != 0) {
            GFL_SndSEPlay(sPkwdBriefingAudienceSE[sequence][1]);
        }
    }
}

static void PkwdBriefingAudience_LoadPalettes(PkwdBriefingAudience *audience, HeapID heapId) {
    NNSG2dPaletteData *objPalette;
    NNSG2dPaletteData *bgPalette;
    void *file;

    audience->objPalettes =
        GFL_HeapAllocate(heapId, AUDIENCE_OBJ_PALETTE_SIZE * AUDIENCE_PALETTE_FRAMES, FALSE, "pkwd_briefing.c", 2881);
    MI_CpuFill8(audience->objPalettes, 0, AUDIENCE_OBJ_PALETTE_SIZE * AUDIENCE_PALETTE_FRAMES);
    audience->bgPalettes =
        GFL_HeapAllocate(heapId, AUDIENCE_BG_PALETTE_SIZE * AUDIENCE_PALETTE_FRAMES, FALSE, "pkwd_briefing.c", 2884);
    MI_CpuFill8(audience->bgPalettes, 0, AUDIENCE_BG_PALETTE_SIZE * AUDIENCE_PALETTE_FRAMES);

    file = GFL_G2DIOReadNCLRArc(audience->arc, FILE_AUDIENCE_PALETTE, &objPalette, heapId);
    MI_CpuCopy8(objPalette->pRawData, audience->objPalettes, AUDIENCE_OBJ_PALETTE_SIZE * AUDIENCE_PALETTE_FRAMES);
    GFL_HeapFree(file);
    file = GFL_G2DIOReadNCLRArc(audience->arc, FILE_AUDIENCE_BG_PALETTE, &bgPalette, heapId);
    MI_CpuCopy8(bgPalette->pRawData, audience->bgPalettes, AUDIENCE_BG_PALETTE_SIZE * AUDIENCE_PALETTE_FRAMES);
    GFL_HeapFree(file);
}

static void PkwdBriefingAudience_FreePalettes(PkwdBriefingAudience *audience) {
    if (audience->objPalettes != NULL) {
        GFL_HeapFree(audience->objPalettes);
        audience->objPalettes = NULL;
    }
    if (audience->bgPalettes != NULL) {
        GFL_HeapFree(audience->bgPalettes);
        audience->bgPalettes = NULL;
    }
}

static void PkwdBriefingAudience_SetPalettes(PkwdBriefingAudience *audience, s32 frame) {
    GXS_LoadOBJPltt(&audience->objPalettes[frame * AUDIENCE_OBJ_PALETTE_SIZE], 0, AUDIENCE_OBJ_PALETTE_SIZE);
    GXS_LoadBGPltt(&audience->bgPalettes[frame * AUDIENCE_BG_PALETTE_SIZE], 0, AUDIENCE_BG_PALETTE_SIZE);
}

// Steps the palettes' animation every 5 frames. A pattern plays a random number of times, then another is picked
static void PkwdBriefingAudience_UpdatePalettes(PkwdBriefingAudience *audience) {
    audience->timer++;
    if (audience->timer > 4) {
        audience->timer = 0;
        audience->frame++;
        if (audience->frame >= sPkwdBriefingPatterns[audience->pattern].length) {
            audience->frame = 0;
            audience->repeat--;
            if (audience->repeat <= 0) {
                PkwdBriefingAudience_SetPattern(audience,
                                                PkwdBriefingAudience_PickPattern(audience, audience->pattern));
                audience->repeat =
                    PkwdBriefingAudience_Random(audience, sPkwdBriefingPatterns[audience->pattern].repeatMax) + 1;
            }
        }
        PkwdBriefingAudience_SetPalettes(audience, sPkwdBriefingPatternFrames[audience->pattern][audience->frame]);
    }
}

static void PkwdBriefingAudience_SetPattern(PkwdBriefingAudience *audience, s32 pattern) {
    audience->pattern = pattern;
    audience->timer = 0;
    audience->frame = 0;
}

// A pattern other than the given one: a random one, or after ten tries the next one
static s32 PkwdBriefingAudience_PickPattern(PkwdBriefingAudience *audience, s32 pattern) {
    BOOL found = FALSE;
    s32 i;
    s32 next;

    for (i = 0; i < 10; i++) {
        next = PkwdBriefingAudience_Random(audience, AUDIENCE_PATTERN_COUNT);
        if (next != pattern) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        next = pattern + 1;
        if (next > AUDIENCE_PATTERN_COUNT - 1) {
            next = 0;
        }
    }
    return next;
}

static u32 PkwdBriefingAudience_Random(PkwdBriefingAudience *audience, u32 max) {
    return GFL_RandomLC(max);
}

// Starts each member of the audience moving once its delay is over
static void PkwdBriefing_UpdateAudience(PkwdBriefingWork *wk) {
    s32 i;

    for (i = 0; i < SEAT_COUNT; i++) {
        if (PkwdBriefingAudience_IsSeatTaken(wk->param->audience, i)) {
            if (wk->audience.delays[i] != 0) {
                wk->audience.delays[i]--;
            } else {
                ClActor_SetAutoAnim(wk->audience.actors[i], TRUE);
            }
        }
    }
    PkwdBriefingAudience_UpdatePalettes(&wk->audience);
}

// Scrolls the credits by, then has the audience react to the movie
static void PkwdBriefing_RunCredits(PkwdBriefingWork *wk) {
    switch (wk->step) {
    case CREDITS_START:
        wk->step = CREDITS_START_2;
        break;
    case CREDITS_START_2:
        wk->step = CREDITS_FADE_OUT_BLANK;
        break;
    case CREDITS_FADE_OUT_BLANK:
        wk->text.alpha++;
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 0, 16 - wk->text.alpha);
        if (wk->text.alpha == 8) {
            wk->step = CREDITS_WAIT_BLANK;
            wk->text.alpha = 0;
            wk->text.timer = 0;
        }
        break;
    case CREDITS_WAIT_BLANK:
        if (wk->text.timer++ > 30) {
            wk->step = CREDITS_SHOW;
        }
        break;
    case CREDITS_SHOW:
        if ((wk->text.line & 1) == 0) {
            GFL_BGSysSetBGEnabled(1, TRUE);
            GFL_BGSysSetBGEnabled(2, FALSE);
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG0, 0, 8);
        } else {
            GFL_BGSysSetBGEnabled(1, FALSE);
            GFL_BGSysSetBGEnabled(2, TRUE);
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG0, 0, 8);
        }
        wk->step = CREDITS_FADE_IN;
        break;
    case CREDITS_FADE_IN:
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2,
                           wk->text.alpha, 8);
        wk->text.alpha++;
        if (wk->text.alpha == 16) {
            wk->step = CREDITS_SCROLL;
        }
        break;
    case CREDITS_SCROLL:
        PkwdBriefingText_ShowCreditsWindows(&wk->text);
        wk->text.scroll++;
        PkwdBriefing_ScrollCredits(wk, wk->text.scroll, wk->layout, wk->heapId);
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysSetBGEnabled(1, TRUE);
        PkwdBriefing_CheckCreditsEnd(wk);
        break;
    case CREDITS_STOP_MUSIC:
        if (wk->text.timer == 0) {
            GFL_SndBGMRequestChange(SEQ_BGM_SILENCE_FIELD, 0xffff, 100, 95);
            wk->text.timer++;
        } else if (wk->text.timer == 90) {
            wk->step = CREDITS_REACT;
            wk->text.timer = 0;
        } else {
            wk->text.timer++;
        }
        break;
    case CREDITS_REACT:
        PkwdBriefing_React(wk, wk->param->setup);
        PkwdBriefingText_LoadCreditsEndScreen(&wk->text, wk->heapId);
        GX_SetVisibleWnd(GX_WNDMASK_W1);
        G2_SetBG3Offset(0, 0);
        wk->step = CREDITS_WAIT_REACTION;
        break;
    case CREDITS_WAIT_REACTION:
        wk->text.timer++;
        if (wk->text.timer > 90) {
            wk->step = CREDITS_FINISH;
        }
        break;
    case CREDITS_FINISH:
        wk->state = STATE_WAIT_VOICE;
        break;
    }
}

// Has the audience react to how the movie's battle went
static void PkwdBriefing_React(PkwdBriefingWork *wk, BtlSetup *setup) {
    u32 result = 0;

    if (setup != NULL) {
        result = setup->studioEnding;
    }
    switch (result) {
    case 0:
        PkwdBriefingAudience_SetAllSequences(&wk->audience, AUDIENCE_GOOD, wk->param->audience);
        PkwdBriefingAudience_PlaySE(wk->param->audienceCount, AUDIENCE_GOOD);
        break;
    case 1:
        PkwdBriefingAudience_SetAllSequences(&wk->audience, AUDIENCE_BAD, wk->param->audience);
        PkwdBriefingAudience_PlaySE(wk->param->audienceCount, AUDIENCE_BAD);
        break;
    case 2:
        PkwdBriefingAudience_SetAllSequences(&wk->audience, AUDIENCE_STRANGE, wk->param->audience);
        PkwdBriefingAudience_PlaySE(wk->param->audienceCount, AUDIENCE_STRANGE);
        break;
    }
}

// Ends the scroll once the credits are past: at once when they are on one side, where the audience has reacted by
// then
static void PkwdBriefing_CheckCreditsEnd(PkwdBriefingWork *wk) {
    if (wk->layout == CREDITS_BOTH) {
        if (wk->text.scroll >= wk->text.height * 8 + 128) {
            wk->step = CREDITS_STOP_MUSIC;
            wk->text.timer = 0;
        }
    } else {
        if (wk->text.scroll >= wk->text.height * 8 + 320) {
            wk->step = CREDITS_FINISH;
            wk->text.timer = 0;
        }
    }
}

// Puts the windows of the credits that the scroll is coming to on their BGs' screens, which are two screens high
// and are filled anew at the points where the scroll wraps around
static void PkwdBriefingText_ShowCreditsWindows(PkwdBriefingText *text) {
    s32 part = 0;
    s32 i;

    switch (text->scroll) {
    case -192:
        part = 2;
        break;
    case 0:
        part = 3;
        break;
    case 256:
        part = 4;
        break;
    case 512:
        part = 5;
        break;
    case 768:
        part = 6;
        break;
    }
    if (part != 0) {
        for (i = 0; i < text->windowCount; i++) {
            BmpWin_FlushMap(text->windows[i]);
        }
        switch (part) {
        case 1:
            GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 64, 0);
            GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 64, 0);
            break;
        case 2:
            GFL_BGSysFillScrArea(1, 0, 0, 32, 32, 32, 0);
            GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 64, 0);
            break;
        case 3:
            GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 64, 0);
            break;
        case 4:
            GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 32, 0);
            GFL_BGSysFillScrArea(2, 0, 0, 32, 32, 32, 0);
            break;
        case 5:
            GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 64, 0);
            break;
        case 6:
            GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 64, 0);
            GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 32, 0);
            break;
        }
        GFL_BGSysLoadScr(1);
        GFL_BGSysLoadScr(2);
    }
}

// What the scroll of the credits sets off near their end: the picture of BG 3 scrolls along with them, and when
// the credits are on one side, the frame changes and the music stops, and then the audience reacts
static void PkwdBriefing_ScrollCredits(PkwdBriefingWork *wk, s32 scroll, u32 layout, HeapID heapId) {
    s32 end = wk->text.height * 8 - 128;

    if (scroll > end) {
        wk->text.endScroll = wk->text.scroll - end;
    }
    if (scroll == end + 234 && layout != CREDITS_BOTH) {
        PkwdBriefingText_LoadCreditsScreen(&wk->text, heapId, layout);
        GFL_SndBGMRequestChange(SEQ_BGM_SILENCE_FIELD, 0xffff, 100, 95);
    }
    if (scroll == end + 298 && layout != CREDITS_BOTH) {
        PkwdBriefing_React(wk, wk->param->setup);
    }
}

// How long a line of the briefing stays for the text speed: 12 seconds when it is slow, 8 for normal, 4 for fast
static s32 PkwdBriefing_GetLineWait(PkwdBriefingWork *wk) {
    s32 wait = TextSpeed_GetCurrentWait();

    if (wait >= TextSpeed_GetWait(0)) {
        return 720;
    }
    if (wait >= TextSpeed_GetWait(1)) {
        return 480;
    }
    return 240;
}
