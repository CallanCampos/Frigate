#include "types.h"
#include "app/wbt_list.h"
#include "app/ui/print_msg.h"
#include "app/ui/ui_scene.h"
#include "app/wbt_list_graphic.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/wbt.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nnsys/gfd.h"
#include "system/app_keycursor.h"
#include "system/app_menu_common.h"
#include "system/app_printsys_common.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/nsbtx_to_clwk.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/0406.h"

// The Pokémon World Tournament's list of Trainers: the upper screen's bracket of the eight entrants, which an
// announcer's messages go through on the lower screen. The ROM has no name for this file; "wbt_list.c" is a guess from
// its graphic file, "wbt_list_graphic.c"

#define ENTRANT_COUNT 8
#define COLUMN_SIZE 4
#define MATCH_COUNT 11
#define LINE_COUNT 6
#define POSITION_COUNT 0x5c
#define MESSAGE_LINE_COUNT 4
#define PALETTE_COUNT 8
// The first OBJ palette of the Trainers' sprites
#define TRAINER_PALETTE 6
#define TEXT_COLOR 0x3dc0

// What the screen shows, the unk4 of its WbtSetup
enum {
    MODE_OPENING,
    MODE_FIRST_ROUND,
    MODE_SECOND_ROUND,
};

enum {
    STATE_START = 0,
    STATE_OPENING_MESSAGE = 10,
    STATE_SLIDE_IN = 100,
    STATE_MESSAGE = 200,
    STATE_SLIDE_OUT = 300,
    STATE_OPENING_SLIDE_OUT = 310,
    STATE_OPENING_END = 320,
    STATE_LOSE = 400,
    STATE_LINE = 500,
    STATE_LINE_WAIT = 510,
    STATE_NEXT_SLIDE_IN = 600,
    STATE_NEXT_MESSAGE = 700,
    STATE_END = 10000,
};

enum {
    WINDOW_TITLE,
    WINDOW_SUBTITLE,
    WINDOW_MESSAGE,
    WINDOW_COUNT,
};

// The cell actors: each entrant's panel and name plate, and the bracket's lines
enum {
    ACTOR_PANEL = 0,
    ACTOR_PLATE = ACTOR_PANEL + ENTRANT_COUNT,
    ACTOR_LINE = ACTOR_PLATE + ENTRANT_COUNT,
    ACTOR_COUNT = ACTOR_LINE + LINE_COUNT,
};

enum {
    OBJ_RES_PALETTE,
    OBJ_RES_CHARS,
    OBJ_RES_CELL,
    OBJ_RES_ANIM,
    OBJ_RES_COUNT,
};

enum {
    TIMER_0,
    TIMER_LINE,
    TIMER_LOSE,
    TIMER_3,
    TIMER_COUNT,
};

enum {
    MATCH_LOSER,
    MATCH_WINNER,
    MATCH_MASK,
    MATCH_LINE,
    MATCH_SIDE,
    MATCH_MESSAGE,
    MATCH_FIRST,
    MATCH_SECOND,
};

#define SLIDE_ACTIVE 0x1
#define SLIDE_DONE 0x2

#define MESSAGE_FLAG_LAST 0x1
#define MESSAGE_FLAG_RESULT 0x2
#define MESSAGE_FLAG_TITLE 0x4
#define MESSAGE_FLAG_ENTRANTS 0x8
#define MESSAGE_FLAG_END 0x10

#define ACTOR_FLAG_ANIMATE 0x1
#define ACTOR_FLAG_FLIP_Y 0x2
#define ACTOR_FLAG_FLIP_X 0x4

// A column of entrants sliding in or out, in hundredths of a pixel
typedef struct {
    s16 start;
    s16 end;
    s16 flags;
    s32 step;
    s32 x;
} WbtListSlide;

// A step of the list: the match it shows, or the Trainers it introduces
typedef struct {
    u8 loser;
    u8 winner;
    // The entrants that slide in, a bit each
    u8 mask;
    // The bracket's line that the winner goes on along, and which of its two ends the winner is at
    u8 line;
    u8 side;
    // The first of the five messages about the match
    u8 message;
    u8 first;
    u8 second;
} WbtListMatch;

typedef struct {
    u16 heapId;
    u32 unk4;
    u32 objRes[OBJ_RES_COUNT - 1];
    ClActor *actors[ACTOR_COUNT];
    WbtListGraphic *graphic;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    MsgData *unk7C;
    u8 trainerActorCount;
    UIObjRes trainerRes;
    ClActor *trainerActors[ENTRANT_COUNT];
    WbtSetup *setup;
    BmpOamSys *bmpOam;
    GFLBitmap *nameBitmaps[ENTRANT_COUNT];
    BmpOamActor *nameActors[ENTRANT_COUNT];
    PrintWindow windows[WINDOW_COUNT];
    PrintStream *stream;
    TCBExManager *tcbManager;
    StrBuf *strbuf;
    KeyCursor *keyCursor;
    AppPrintsysCommon printCommon;
    s32 mode;
    s32 state;
    s32 step;
    s32 playerIndex;
    u32 mask;
    u32 messageFlags;
    u32 unk144;
    u32 unk148;
    // How many of the shuffled messages about a match have been shown
    s32 lineCount;
    s32 lineOrder[MESSAGE_LINE_COUNT];
    s32 timers[TIMER_COUNT];
    WbtListSlide slides[2];
    WbtListMatch matches[MATCH_COUNT];
    // The OBJ palettes of the Trainers' sprites, and their gray versions for the ones who lost
    u16 palettes[PALETTE_COUNT][16];
    u16 grayPalettes[PALETTE_COUNT][16];
    // The positions of everything on the screen, x and y, from the archive
    s16 *positions;
} WbtList;

typedef struct {
    u8 title;
    u8 last;
    u8 first;
    u8 second;
} WbtListMessages;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} WbtListWindowSetup;

typedef struct {
    s16 fileId;
    s16 index;
    u16 offset;
    u16 count;
} WbtListObjResSetup;

typedef struct {
    u8 unk0;
    u8 unk1;
    u8 palette;
    u8 position;
    u8 priority;
    u8 unk5;
} WbtListTrainerSetup;

typedef struct {
    u8 width;
    u8 height;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 paletteOffset;
    u8 position;
    u8 pad7;
} WbtListNameSetup;

// What sWbtListLines has for each end of a line
enum {
    LINE_POSITION,
    LINE_FLAGS,
    // Drawing itself, and drawn
    LINE_SEQUENCE,
    LINE_SEQUENCE_DRAWN,
    LINE_VALUE_COUNT,
};

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unk4;
    u8 flags;
    u8 position;
    u8 pad7;
} WbtListActorSetup;

static BOOL WbtListProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WbtListProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WbtListProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static void WbtList_LoadBG(u32 *unk, HeapID heapId);
static void WbtList_CreateTrainerActors(WbtList *work, const WbtListTrainerSetup *setups);
static void WbtList_FreeTrainerActors(WbtList *work);
static void WbtList_InitView(WbtList *work);
static void WbtList_Main(WbtList *work);
static void WbtList_FreeView(WbtList *work);
static void WbtList_FlushWindows(WbtList *work);
static void WbtList_MainSequence(WbtList *work);
static void WbtList_SequenceStep(WbtList *work);
static void WbtList_Sequence(WbtList *work);
static void WbtList_SetTimer(WbtList *work, s32 timer, s32 frames);
static s32 WbtList_TickTimer(WbtList *work, s32 timer);
static void WbtList_CreateWindows(WbtList *work);
static void WbtList_PrintTitle(WbtList *work, s32 window, s32 x, s32 y, StrBuf *strbuf);
static void WbtList_FreeWindows(WbtList *work);
static void WbtList_InitObjects(WbtList *work);
static void WbtList_LoadObjResources(WbtList *work, u32 vramType, const WbtListObjResSetup *resSetups);
static void WbtList_CreateActors(WbtList *work, u32 vramType, const WbtListActorSetup *setups,
                                 const WbtListObjResSetup *resSetups);
static void WbtList_CreateNameActors(WbtList *work, u32 vramType, const WbtListNameSetup *setups,
                                     const WbtListObjResSetup *resSetups);
static void WbtList_FreeObjects(WbtList *work);
static void WbtList_SetActorAnim(WbtList *work, s32 actor, s32 sequence);
static void WbtList_SetActorVisible(WbtList *work, s32 actor, BOOL visible);
static BOOL WbtList_IsActorAnimating(WbtList *work, s32 actor);
static void WbtList_SavePalettes(WbtList *work, s32 count, s32 start, s32 sub);
static void WbtList_SetPaletteGray(WbtList *work, s32 palette, s32 start, BOOL restore);
static void WbtList_StartSlide(WbtList *work, s32 column, s16 start, s16 end, s16 frames);
static s16 WbtList_UpdateSlide(WbtList *work, s32 column);
static BOOL WbtList_IsSliding(WbtList *work, s32 column);
static void WbtList_SlideColumns(WbtList *work, BOOL out);
static void WbtList_PlaceEntrant(WbtList *work, s32 entrant, BOOL hidden);
static void WbtList_MoveColumns(WbtList *work);
static void WbtList_PrintMessage(WbtList *work, u32 messageId);
static BOOL WbtList_UpdateMessage(WbtList *work);
static u32 WbtList_GetMessage(WbtList *work);
static void WbtList_ClearMessage(WbtList *work);
static s32 WbtList_GetPosition(WbtList *work, u32 index);
static void WbtList_ShowLine(WbtList *work, s32 line, s32 drawn, s32 side);
static BOOL WbtList_IsLineAnimating(WbtList *work, s32 line);
static void WbtList_ShowLineJoint(WbtList *work);
static void WbtList_ResetBracket(WbtList *work);
static void WbtList_InitEntrants(WbtList *work);
static void WbtList_InitMatches(WbtList *work);
static s32 WbtList_GetMatchValue(WbtList *work, s32 field, s32 offset);
static BOOL WbtList_NextStep(WbtList *work);
static BOOL WbtList_IsLastStep(WbtList *work);
static void WbtList_ShuffleLines(WbtList *work);
static void WbtList_Stub(WbtList *work);

// The x of the two titles
static const s16 sWbtListTitleX[2] = { 112, 112 };

// The x of the two columns of entrants, hidden and shown
static const s16 sWbtListColumnX[2][2] = {
    { -72, 0 },
    { 220, 148 }
};

static const WbtListMessages sWbtListMessages[4] = {
    { Bank0406_Text_The, Bank0406_Text_StartsNow, Bank0406_Text_The, Bank0406_Text_ThesePokemonTrainersWho },
    { Bank0406_Text_FirstRoundVs, Bank0406_Text_LetsMoveNextBattle, 0, 0 },
    { Bank0406_Text_SecondRoundVs, Bank0406_Text_WhichTrainerWillWin, 0, 0 },
    { 0, 0, 0, 0 }
};

static const WbtListWindowSetup sWbtListWindows[WINDOW_COUNT + 1] = {
    { 4, 2, 4, 28, 2, 2 },
    { 4, 2, 17, 28, 2, 2 },
    { 0, 1, 19, 30, 4, 9 },
    { 0xff, 0, 0, 0, 0, 0 }
};

static const WbtListObjResSetup sWbtListObjRes[OBJ_RES_COUNT] = {
    { 2, OBJ_RES_PALETTE, 0, 6 },
    { 5, OBJ_RES_CHARS, 0, 0 },
    { 6, OBJ_RES_CELL, 0, 0 },
    { 11, -1, 0, 0 }
};

static const WbtListTrainerSetup sWbtListTrainers[ENTRANT_COUNT + 1] = {
    { 0x07, 0x03, TRAINER_PALETTE + 0, 0, 64, 0 },
    { 0x0d, 0x03, TRAINER_PALETTE + 1, 1, 64, 0 },
    { 0x88, 0x03, TRAINER_PALETTE + 2, 2, 64, 0 },
    { 0x26, 0x02, TRAINER_PALETTE + 3, 3, 64, 0 },
    { 0x5e, 0x03, TRAINER_PALETTE + 4, 4, 64, 0 },
    { 0x25, 0x02, TRAINER_PALETTE + 5, 5, 64, 0 },
    { 0x24, 0x02, TRAINER_PALETTE + 6, 6, 64, 0 },
    { 0x5d, 0x03, TRAINER_PALETTE + 7, 7, 64, 0 },
    { 0xff, 0, 0, 0, 0, 0 }
};

const GameProcFunctions WBT_LIST_PROC_FUNCTIONS = { WbtListProc_Init, WbtListProc_Main, WbtListProc_Exit };

static const WbtListNameSetup sWbtListNames[ENTRANT_COUNT + 1] = {
    { 8, 2, 56, 1, TRUE, 0, 16, 0 },
    { 8, 2, 56, 1, TRUE, 0, 17, 0 },
    { 8, 2, 56, 1, TRUE, 0, 18, 0 },
    { 8, 2, 56, 1, TRUE, 0, 19, 0 },
    { 8, 2, 56, 1, TRUE, 0, 20, 0 },
    { 8, 2, 56, 1, TRUE, 0, 21, 0 },
    { 8, 2, 56, 1, TRUE, 0, 22, 0 },
    { 8, 2, 56, 1, TRUE, 0, 23, 0 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 }
};

// The bracket's lines: the four of the first round and the two of the second, each from either of its two entrants
static const u16 sWbtListLines[LINE_COUNT * 2 * LINE_VALUE_COUNT] = {
    32, ACTOR_FLAG_ANIMATE, 8, 9,
    33, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_X, 8, 9,
    34, ACTOR_FLAG_ANIMATE, 8, 9,
    35, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_X, 8, 9,
    36, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y, 8, 9,
    37, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y | ACTOR_FLAG_FLIP_X, 8, 9,
    38, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y, 8, 9,
    39, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y | ACTOR_FLAG_FLIP_X, 8, 9,
    40, ACTOR_FLAG_ANIMATE, 10, 11,
    41, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_X, 10, 11,
    42, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y, 10, 11,
    43, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y | ACTOR_FLAG_FLIP_X, 10, 11,
};

static const WbtListActorSetup sWbtListActors[ACTOR_COUNT + 1] = {
    { 0, 48, 1, TRUE, 0, 0, 8, 0 },
    { 0, 48, 1, TRUE, 0, 0, 9, 0 },
    { 0, 48, 1, TRUE, 0, 0, 10, 0 },
    { 0, 48, 1, TRUE, 0, 0, 11, 0 },
    { 4, 48, 1, TRUE, 0, 0, 12, 0 },
    { 4, 48, 1, TRUE, 0, 0, 13, 0 },
    { 4, 48, 1, TRUE, 0, 0, 14, 0 },
    { 4, 48, 1, TRUE, 0, 0, 15, 0 },
    { 2, 64, 1, TRUE, 0, 0, 16, 0 },
    { 2, 64, 1, TRUE, 0, 0, 17, 0 },
    { 2, 64, 1, TRUE, 0, 0, 18, 0 },
    { 2, 64, 1, TRUE, 0, 0, 19, 0 },
    { 6, 64, 1, TRUE, 0, 0, 20, 0 },
    { 6, 64, 1, TRUE, 0, 0, 21, 0 },
    { 6, 64, 1, TRUE, 0, 0, 22, 0 },
    { 6, 64, 1, TRUE, 0, 0, 23, 0 },
    { 8, 80, 1, FALSE, 3, ACTOR_FLAG_ANIMATE, 32, 0 },
    { 8, 80, 1, FALSE, 3, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_X, 35, 0 },
    { 8, 80, 1, FALSE, 3, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y, 36, 0 },
    { 8, 80, 1, FALSE, 3, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y | ACTOR_FLAG_FLIP_X, 39, 0 },
    { 10, 80, 1, FALSE, 3, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_X, 41, 0 },
    { 10, 80, 1, FALSE, 3, ACTOR_FLAG_ANIMATE | ACTOR_FLAG_FLIP_Y | ACTOR_FLAG_FLIP_X, 43, 0 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 }
};

static BOOL WbtListProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WbtList *list;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WBT_LIST, 0x30000);
    list = GFL_ProcInitSubsystem(proc, sizeof(WbtList), HEAPID_WBT_LIST);
    sys_memset(list, 0, sizeof(WbtList));
    list->heapId = HEAPID_WBT_LIST;
    list->setup = param;
    list->graphic = WbtListGraphic_Create(1, list->heapId);
    list->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, list->heapId);
    list->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0406, list->heapId);
    list->unk7C = NULL;
    list->queue = func_02021998(list->heapId);
    list->tcbManager = GFL_TCBExMgrCreate(list->heapId, list->heapId, 32, 32);
    list->stream = NULL;
    list->strbuf = NULL;
    WbtList_LoadBG(&list->unk4, list->heapId);
    WbtList_InitView(list);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, list->heapId);
    return TRUE;
}

static BOOL WbtListProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WbtList *list = work;
    HeapID heapId;

    WbtList_FreeView(list);
    WbtList_FreeTrainerActors(list);
    if (list->msgData != NULL) {
        GFL_MsgDataFree(list->msgData);
    }
    if (list->unk7C != NULL) {
        GFL_MsgDataFree(list->unk7C);
    }
    func_02021a18(list->queue);
    GFL_FontFree(list->font);
    GFL_TCBExMgrFree(list->tcbManager);
    WbtListGraphic_Free(list->graphic);
    heapId = list->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL WbtListProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WbtList *list = work;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 1:
        if (list->state == STATE_END && func_02021c0c(list->queue) == TRUE) {
            (*state)++;
        }
        break;
    case 2:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, list->heapId);
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    WbtList_Main(list);
    GFL_TCBExMgrUpdate(list->tcbManager);
    WbtList_FlushWindows(list);
    WbtListGraphic_Update(list->graphic);
    WbtListGraphic_Begin3D(list->graphic);
    WbtListGraphic_End3D(list->graphic);
    return FALSE;
}

static void WbtList_LoadBG(u32 *unk, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WBT_SUBSCREEN, heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0xa0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_SUB_BG, 0, 0x60, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 8, 5, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 9, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 2, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

// The entrants' overworld sprites, each with the app menu's resources and its own palette
static void WbtList_CreateTrainerActors(WbtList *work, const WbtListTrainerSetup *setups) {
    ClActUnit *unit = WbtListGraphic_GetClActUnit(work->graphic);
    u32 i;

    for (i = 0; i < ENTRANT_COUNT; i++) {
        UIObjResSetup resSetup;
        u32 position;
        s32 x;
        s32 y;

        if (setups->unk0 == 0xff) {
            break;
        }
        resSetup.vramType = CLACT_VRAM_MAIN;
        resSetup.flags = 0;
        resSetup.arcId = getUINarcIdx();
        resSetup.paletteFile = func_0202d8a0();
        resSetup.charFile = func_0202d8a4();
        resSetup.cellFile = func_0202d8a8(2);
        resSetup.animFile = func_0202d8ac(2);
        resSetup.paletteOffset = setups->palette;
        resSetup.paletteStart = 0;
        resSetup.paletteCount = 1;
        UIObjRes_Load(&work->trainerRes, &resSetup, unit, work->heapId);
        position = setups->position * 2;
        x = WbtList_GetPosition(work, position);
        y = WbtList_GetPosition(work, position + 1);
        work->trainerActors[i] = UIObjRes_CreateActor(&work->trainerRes, unit, x, y, 0, work->heapId);
        NsbtxToClwk_LoadTexture(work->trainerActors[i], ARCID_MMODEL_GRA, work->setup->entrants[i].unk4,
                                work->setup->entrants[i].unk6, 4, 4, 0, resSetup.vramType, work->heapId);
        work->trainerActorCount++;
        func_0204c438(work->trainerActors[i], setups->priority);
        setups++;
    }
}

static void WbtList_FreeTrainerActors(WbtList *work) {
    u32 i;

    for (i = 0; i < work->trainerActorCount; i++) {
        func_0204c108(work->trainerActors[i]);
    }
    UIObjRes_Free(&work->trainerRes);
}

static void WbtList_InitView(WbtList *work) {
    s32 i;
    ArcTool *arc;

    WbtList_Stub(work);
    work->strbuf = GFL_StrBufCreate(200, work->heapId);
    work->mode = work->setup->unk4;
    work->state = STATE_START;
    work->step = 0;
    work->playerIndex = 0;
    work->mask = 0;
    work->messageFlags = 0;
    work->unk144 = 0;
    work->unk148 = 0;
    work->lineCount = 0;
    for (i = 0; i < MESSAGE_LINE_COUNT; i++) {
        work->lineOrder[i] = -1;
    }
    for (i = 0; i < TIMER_COUNT; i++) {
        work->timers[i] = 0;
    }
    work->positions = GFL_ArcSysReadHeapNew(ARCID_WBT_SUBSCREEN, 12, work->heapId);
    WbtList_CreateWindows(work);
    WbtList_PrintTitle(work, WINDOW_TITLE, sWbtListTitleX[WINDOW_TITLE], 0, work->setup->tournamentName);
    WbtList_PrintTitle(work, WINDOW_SUBTITLE, sWbtListTitleX[WINDOW_SUBTITLE], 0, work->setup->unk9C);
    WbtList_InitObjects(work);
    for (i = 0; i < ENTRANT_COUNT; i++) {
        GFL_BitmapFill(work->nameBitmaps[i], 0);
        GFL_StrBufClear(work->strbuf);
        GFL_StrBufUncompress(work->strbuf, work->setup->entrants[i].name);
        GFL_TextRendererDrawToBitmapEx(work->nameBitmaps[i], 0, 0, work->strbuf, work->font, TEXT_COLOR);
        BmpOam_ActorBmpTrans(work->nameActors[i]);
    }
    WbtList_InitEntrants(work);
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_MAIN_BG, 9 * 0x20, 0x20, work->heapId);
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(0, 1, 8, 0, work->heapId);
    BmpWin_DrawFrame(work->windows[WINDOW_MESSAGE].window, FALSE, 1, 8);
    BmpWin_FlushMap(work->windows[WINDOW_MESSAGE].window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[WINDOW_MESSAGE].window));
    WbtList_ClearMessage(work);
    AppPrintsysCommon_Init(&work->printCommon, APP_PRINTSYS_COMMON_KEYS);
    work->keyCursor = KeyCursor_Create(15, TRUE, FALSE, work->heapId);
}

static void WbtList_Main(WbtList *work) {
    WbtList_MainSequence(work);
}

static void WbtList_FreeView(WbtList *work) {
    GFL_HeapFree(work->positions);
    WbtList_FreeWindows(work);
    WbtList_FreeObjects(work);
}

static void WbtList_FlushWindows(WbtList *work) {
    s32 i;

    func_02021a3c(work->queue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintWindow_Flush(&work->windows[i], work->queue);
    }
}

static void WbtList_MainSequence(WbtList *work) {
    WbtList_Sequence(work);
}

static void WbtList_SequenceStep(WbtList *work) {
    s32 loser;
    s32 winner;
    s32 line;
    s32 side;

    switch (work->state) {
    case STATE_START:
        work->state = STATE_SLIDE_IN;
        WbtList_SlideColumns(work, FALSE);
        WbtList_ResetBracket(work);
        if (work->mode == MODE_OPENING) {
            work->state = STATE_OPENING_MESSAGE;
            WbtList_PrintMessage(work, WbtList_GetMessage(work));
        } else {
            GFL_SndSEPlay(SEQ_SE_SW_WBT_06);
        }
        break;
    case STATE_OPENING_MESSAGE:
        if (!WbtList_UpdateMessage(work)) {
            WbtList_NextStep(work);
            work->mask = WbtList_GetMatchValue(work, MATCH_MASK, 0);
            work->state = STATE_SLIDE_IN;
            WbtList_SlideColumns(work, FALSE);
            WbtList_ClearMessage(work);
            GFL_SndSEPlay(SEQ_SE_SW_WBT_06);
        }
        break;
    case STATE_SLIDE_IN:
        WbtList_MoveColumns(work);
        if (!WbtList_IsSliding(work, 0)) {
            work->state = STATE_MESSAGE;
            WbtList_PrintMessage(work, WbtList_GetMessage(work));
        }
        break;
    case STATE_MESSAGE:
        if (WbtList_UpdateMessage(work)) {
            break;
        }
        if (work->mode == MODE_OPENING) {
            WbtList_NextStep(work);
            work->mask = WbtList_GetMatchValue(work, MATCH_MASK, 0);
            work->state = STATE_OPENING_SLIDE_OUT;
            WbtList_SlideColumns(work, TRUE);
            WbtList_ClearMessage(work);
            GFL_SndSEPlay(SEQ_SE_SW_WBT_09);
        } else {
            work->state = STATE_SLIDE_OUT;
            WbtList_SlideColumns(work, TRUE);
            WbtList_ClearMessage(work);
            GFL_SndSEPlay(SEQ_SE_SW_WBT_09);
        }
        break;
    case STATE_SLIDE_OUT:
        WbtList_MoveColumns(work);
        if (!WbtList_IsSliding(work, 0)) {
            WbtList_NextStep(work);
            work->mask = WbtList_GetMatchValue(work, MATCH_MASK, 0);
            loser = WbtList_GetMatchValue(work, MATCH_LOSER, -1);
            WbtList_SetActorAnim(work, ACTOR_PANEL + loser, loser >= COLUMN_SIZE ? 13 : 12);
            work->state = STATE_LOSE;
            WbtList_SetTimer(work, TIMER_LOSE, 10);
        }
        break;
    case STATE_OPENING_SLIDE_OUT:
        WbtList_MoveColumns(work);
        if (!WbtList_IsSliding(work, 0)) {
            work->state = STATE_OPENING_END;
            WbtList_PrintMessage(work, WbtList_GetMessage(work));
        }
        break;
    case STATE_OPENING_END:
        if (!WbtList_UpdateMessage(work)) {
            work->state = STATE_END;
        }
        break;
    case STATE_LOSE:
        if (WbtList_IsActorAnimating(work, ACTOR_PANEL + WbtList_GetMatchValue(work, MATCH_LOSER, -1))) {
            break;
        }
        WbtList_SetPaletteGray(work, WbtList_GetMatchValue(work, MATCH_LOSER, -1), TRAINER_PALETTE, FALSE);
        loser = WbtList_GetMatchValue(work, MATCH_LOSER, -1);
        WbtList_SetActorAnim(work, ACTOR_PANEL + loser, loser >= COLUMN_SIZE ? 5 : 1);
        work->state = STATE_LINE;
        WbtList_SetTimer(work, TIMER_LINE, 60);
        WbtList_ShowLine(work, WbtList_GetMatchValue(work, MATCH_LINE, -1), 0,
                         WbtList_GetMatchValue(work, MATCH_SIDE, -1));
        WbtList_ShowLineJoint(work);
        winner = WbtList_GetMatchValue(work, MATCH_WINNER, -1);
        WbtList_SetActorAnim(work, ACTOR_PANEL + winner, winner >= COLUMN_SIZE ? 16 : 15);
        GFL_SndSEPlay(SEQ_SE_SW_WBT_05);
        break;
    case STATE_LINE:
        WbtList_TickTimer(work, TIMER_LINE);
        line = WbtList_GetMatchValue(work, MATCH_LINE, -1);
        side = WbtList_GetMatchValue(work, MATCH_SIDE, -1);
        if (WbtList_IsLineAnimating(work, line)) {
            break;
        }
        work->state = STATE_LINE_WAIT;
        WbtList_ShowLine(work, line, 1, side);
        winner = WbtList_GetMatchValue(work, MATCH_WINNER, -1);
        WbtList_SetActorAnim(work, ACTOR_PANEL + winner, winner >= COLUMN_SIZE ? 4 : 0);
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_SW_WBT_05));
        break;
    case STATE_LINE_WAIT:
        if (WbtList_TickTimer(work, TIMER_LINE) > 0) {
            break;
        }
        work->state = STATE_NEXT_SLIDE_IN;
        WbtList_SlideColumns(work, FALSE);
        GFL_SndSEPlay(SEQ_SE_SW_WBT_06);
        break;
    case STATE_NEXT_SLIDE_IN:
        WbtList_MoveColumns(work);
        if (!WbtList_IsSliding(work, 0)) {
            work->state = STATE_NEXT_MESSAGE;
            WbtList_PrintMessage(work, WbtList_GetMessage(work));
        }
        break;
    case STATE_NEXT_MESSAGE:
        if (WbtList_UpdateMessage(work)) {
            break;
        }
        work->state = STATE_SLIDE_OUT;
        if (WbtList_IsLastStep(work) == TRUE) {
            work->state = STATE_END;
        } else {
            WbtList_SlideColumns(work, TRUE);
            WbtList_ClearMessage(work);
            GFL_SndSEPlay(SEQ_SE_SW_WBT_09);
        }
        break;
    case STATE_END:
        break;
    }
}

static void WbtList_Sequence(WbtList *work) {
    WbtList_SequenceStep(work);
}

static void WbtList_SetTimer(WbtList *work, s32 timer, s32 frames) {
    work->timers[timer] = frames;
}

static s32 WbtList_TickTimer(WbtList *work, s32 timer) {
    work->timers[timer]--;
    if (work->timers[timer] <= 0) {
        work->timers[timer] = 0;
    }
    return work->timers[timer];
}

static void WbtList_CreateWindows(WbtList *work) {
    s32 i;
    const WbtListWindowSetup *setup = sWbtListWindows;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        work->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        setup++;
    }
}

static void WbtList_PrintTitle(WbtList *work, s32 window, s32 x, s32 y, StrBuf *strbuf) {
    PrintStrAligned(&work->windows[window], work->queue, x, y, strbuf, work->font, TEXT_COLOR, PRINT_ALIGN_CENTER);
    BmpWin_FlushMap(work->windows[window].window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[window].window));
}

static void WbtList_FreeWindows(WbtList *work) {
    s32 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        BmpWin_Free(work->windows[i].window);
    }
    if (work->strbuf != NULL) {
        GFL_StrBufFree(work->strbuf);
    }
    if (work->stream != NULL) {
        func_020223cc(work->stream);
    }
    KeyCursor_Free(work->keyCursor);
}

static void WbtList_InitObjects(WbtList *work) {
    ClActUnit *unit = WbtListGraphic_GetClActUnit(work->graphic);

    WbtList_LoadObjResources(work, CLACT_VRAM_MAIN, sWbtListObjRes);
    WbtList_CreateActors(work, CLACT_VRAM_MAIN, sWbtListActors, sWbtListObjRes);
    work->bmpOam = BmpOam_Init(work->heapId, unit);
    WbtList_CreateNameActors(work, CLACT_VRAM_MAIN, sWbtListNames, sWbtListObjRes);
    WbtList_CreateTrainerActors(work, sWbtListTrainers);
    WbtList_SavePalettes(work, PALETTE_COUNT, TRAINER_PALETTE, CLACT_VRAM_MAIN);
    func_02042ba8(FALSE, work->heapId);
}

static void WbtList_LoadObjResources(WbtList *work, u32 vramType, const WbtListObjResSetup *resSetups) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WBT_SUBSCREEN, work->heapId);

    WbtListGraphic_GetClActUnit(work->graphic);
    work->objRes[resSetups[OBJ_RES_PALETTE].index] =
        func_0204bbb8(arc, resSetups[OBJ_RES_PALETTE].fileId, vramType, resSetups[OBJ_RES_PALETTE].offset, 0,
                      resSetups[OBJ_RES_PALETTE].count, work->heapId);
    work->objRes[resSetups[OBJ_RES_CHARS].index] =
        func_0204b81c(arc, resSetups[OBJ_RES_CHARS].fileId, FALSE, vramType, work->heapId);
    work->objRes[resSetups[OBJ_RES_CELL].index] =
        func_0204bde0(arc, resSetups[OBJ_RES_CELL].fileId, resSetups[OBJ_RES_ANIM].fileId, work->heapId);
    GFL_ArcToolFree(arc);
}

static void WbtList_CreateActors(WbtList *work, u32 vramType, const WbtListActorSetup *setups,
                                 const WbtListObjResSetup *resSetups) {
    s32 i;
    ClActUnit *unit = WbtListGraphic_GetClActUnit(work->graphic);
    ClActorSetup setup;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    for (i = 0; i < ACTOR_COUNT; i++) {
        u32 position;
        s32 x;
        s32 y;
        BOOL visible;
        BOOL animate;

        if (setups->sequence == 0xff) {
            break;
        }
        position = setups->position * 2;
        x = WbtList_GetPosition(work, position);
        y = WbtList_GetPosition(work, position + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = setups->sequence;
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        work->actors[i] = func_0204c040(unit, work->objRes[resSetups[OBJ_RES_CHARS].index],
                                        work->objRes[resSetups[OBJ_RES_PALETTE].index],
                                        work->objRes[resSetups[OBJ_RES_CELL].index], &setup, vramType, work->heapId);
        visible = FALSE;
        if (setups->visible == TRUE) {
            visible = TRUE;
        }
        func_0204c124(work->actors[i], visible);
        animate = FALSE;
        if (setups->flags & ACTOR_FLAG_ANIMATE) {
            animate = TRUE;
        }
        func_0204c520(work->actors[i], animate);
        if (setups->flags & ACTOR_FLAG_FLIP_Y) {
            func_0204c2b0(work->actors[i], 1, TRUE);
        }
        if (setups->flags & ACTOR_FLAG_FLIP_X) {
            func_0204c2b0(work->actors[i], 0, TRUE);
        }
        setups++;
    }
}

// The entrants' names, each a bitmap shown as OBJs
static void WbtList_CreateNameActors(WbtList *work, u32 vramType, const WbtListNameSetup *setups,
                                     const WbtListObjResSetup *resSetups) {
    s32 i;

    for (i = 0; i < ENTRANT_COUNT; i++) {
        BmpOamActorSetup setup;
        u32 position;
        s32 x;
        s32 y;

        if (setups->width == 0xff) {
            break;
        }
        work->nameBitmaps[i] = GFL_BitmapCreate(setups->width, setups->height, 0x20, work->heapId);
        position = setups->position * 2;
        x = WbtList_GetPosition(work, position);
        y = WbtList_GetPosition(work, position + 1);
        sys_memset(&setup, 0, sizeof(BmpOamActorSetup));
        setup.bitmap = work->nameBitmaps[i];
        setup.x = x;
        setup.y = y;
        setup.palette = work->objRes[resSetups[OBJ_RES_PALETTE].index];
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        setup.paletteOffset = setups->paletteOffset;
        setup.surface = vramType;
        setup.vramType = vramType;
        work->nameActors[i] = BmpOam_ActorAdd(work->bmpOam, &setup);
        if (!setups->visible) {
            BmpOam_ActorSetDrawEnable(work->nameActors[i], FALSE);
        }
        // BUG: the loop doesn't go on to the next setup, so every name is created with the first one's. They only
        // differ in their positions, which WbtList_PlaceEntrant sets again
#ifdef BUGFIX
        setups++;
#endif
    }
}

static void WbtList_FreeObjects(WbtList *work) {
    s32 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (work->actors[i] != NULL) {
            func_0204c108(work->actors[i]);
        }
    }
    func_0204bcd0(work->objRes[OBJ_RES_PALETTE]);
    func_0204b98c(work->objRes[OBJ_RES_CHARS]);
    func_0204be64(work->objRes[OBJ_RES_CELL]);
    for (i = 0; i < ENTRANT_COUNT; i++) {
        BmpOam_ActorDel(work->nameActors[i]);
        GFL_BitmapFree(work->nameBitmaps[i]);
    }
    BmpOam_Exit(work->bmpOam);
}

static void WbtList_SetActorAnim(WbtList *work, s32 actor, s32 sequence) {
    func_0204c488(work->actors[actor], sequence);
    func_0204c520(work->actors[actor], TRUE);
}

static void WbtList_SetActorVisible(WbtList *work, s32 actor, BOOL visible) {
    func_0204c124(work->actors[actor], visible);
}

static BOOL WbtList_IsActorAnimating(WbtList *work, s32 actor) {
    BOOL animating = TRUE;

    if (!func_0204c560(work->actors[actor])) {
        animating = FALSE;
    }
    return animating;
}

// Keeps the OBJ palettes from `start` on
static void WbtList_SavePalettes(WbtList *work, s32 count, s32 start, s32 sub) {
    u16 *src = (u16 *)HW_OBJ_PLTT;

    if (sub == 1) {
        src += 0x400 / sizeof(u16);
    }
    sys_memcpy16(src + start * 16, work->palettes, count * 0x20);
}

// Turns one of the kept palettes gray, or back; with palette -1, all back
static void WbtList_SetPaletteGray(WbtList *work, s32 palette, s32 start, BOOL restore) {
    s32 i;

    if (palette == -1) {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, start * 0x20, work->palettes,
                                           sizeof(work->palettes));
        return;
    }
    for (i = 0; i < 16; i++) {
        u16 color = work->palettes[palette][i];
        u16 b = (color >> 10) & 0x1f;
        u16 r = color & 0x1f;
        u16 g = (color >> 5) & 0x1f;
        u16 gray = (r + g + b) / 3;

        work->grayPalettes[palette][i] = (gray << 10) + (gray << 5) + gray;
    }
    if (!restore) {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, (palette + start) * 0x20,
                                           work->grayPalettes[palette], sizeof(work->grayPalettes[palette]));
    } else {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, (palette + start) * 0x20,
                                           work->palettes[palette], sizeof(work->palettes[palette]));
    }
}

static void WbtList_StartSlide(WbtList *work, s32 column, s16 start, s16 end, s16 frames) {
    if (column < 2 && frames != 0) {
        work->slides[column].start = start;
        work->slides[column].end = end;
        work->slides[column].step = (end - start) * 100 / frames;
        work->slides[column].x = start * 100;
        work->slides[column].flags = SLIDE_ACTIVE;
    }
}

// Returns the column's x
static s16 WbtList_UpdateSlide(WbtList *work, s32 column) {
    s16 x = 0;

    if (work->slides[column].flags & SLIDE_ACTIVE) {
        if (!(work->slides[column].flags & SLIDE_DONE)) {
            BOOL left = FALSE;
            s16 next;

            work->slides[column].x += work->slides[column].step;
            next = work->slides[column].x / 100;
            if (work->slides[column].step < 0) {
                left = TRUE;
            }
            if (!left) {
                if (next >= work->slides[column].end) {
                    work->slides[column].flags |= SLIDE_DONE;
                }
            } else {
                if (next <= work->slides[column].end) {
                    work->slides[column].flags |= SLIDE_DONE;
                }
            }
            x = next;
        } else {
            x = work->slides[column].end;
        }
    }
    return x;
}

static BOOL WbtList_IsSliding(WbtList *work, s32 column) {
    BOOL sliding = FALSE;

    if ((work->slides[column].flags & SLIDE_ACTIVE) && !(work->slides[column].flags & SLIDE_DONE)) {
        sliding = TRUE;
    }
    return sliding;
}

static void WbtList_SlideColumns(WbtList *work, BOOL out) {
    s32 frames;
    s32 leftStart;
    s32 leftEnd;
    s32 rightStart;
    s32 rightEnd;

    if (!out) {
        leftStart = sWbtListColumnX[0][0];
        leftEnd = sWbtListColumnX[0][1];
        rightStart = sWbtListColumnX[1][0];
        rightEnd = sWbtListColumnX[1][1];
        frames = 9;
    } else {
        leftStart = sWbtListColumnX[0][1];
        leftEnd = sWbtListColumnX[0][0];
        rightStart = sWbtListColumnX[1][1];
        rightEnd = sWbtListColumnX[1][0];
        frames = 9;
    }
    WbtList_StartSlide(work, 0, leftStart, leftEnd, frames);
    WbtList_StartSlide(work, 1, rightStart, rightEnd, frames);
}

// Puts an entrant's plate and name where its column is shown, or hidden
static void WbtList_PlaceEntrant(WbtList *work, s32 entrant, BOOL hidden) {
    s32 left;
    s32 right;
    s32 nameX;
    s32 nameY;
    ClActor **plates;
    ClActorPos pos;

    if (!hidden) {
        left = sWbtListColumnX[0][1];
        right = sWbtListColumnX[1][1];
        nameX = WbtList_GetPosition(work, 88);
        nameY = WbtList_GetPosition(work, 89);
    } else {
        left = sWbtListColumnX[0][0];
        right = sWbtListColumnX[1][0];
        nameX = WbtList_GetPosition(work, 90);
        nameY = WbtList_GetPosition(work, 91);
    }
    plates = &work->actors[ACTOR_PLATE];
    func_0204c178(plates[entrant], &pos, 0);
    pos.x = right;
    if (entrant < COLUMN_SIZE) {
        pos.x = left;
    }
    func_0204c140(plates[entrant], &pos, 0);
    BmpOam_ActorSetPos(work->nameActors[entrant], pos.x + nameX, pos.y + nameY);
}

// Moves the plates and names of the step's entrants with their columns
static void WbtList_MoveColumns(WbtList *work) {
    s16 i;
    s16 leftX;
    s32 rightNameY;
    s32 rightNameX;
    s32 leftNameY;
    s32 leftNameX;
    s16 rightX;
    ClActorPos rightPos;
    ClActorPos leftPos;

    leftX = WbtList_UpdateSlide(work, 0);
    leftNameX = WbtList_GetPosition(work, 88);
    leftNameY = WbtList_GetPosition(work, 89);
    for (i = 0; i < COLUMN_SIZE; i++) {
        if (work->mask & (1 << i)) {
            func_0204c178(work->actors[ACTOR_PLATE + i], &leftPos, 0);
            leftPos.x = leftX;
            func_0204c140(work->actors[ACTOR_PLATE + i], &leftPos, 0);
            BmpOam_ActorSetPos(work->nameActors[i], leftPos.x + leftNameX, leftPos.y + leftNameY);
        }
    }
    rightX = WbtList_UpdateSlide(work, 1);
    rightNameX = WbtList_GetPosition(work, 90);
    rightNameY = WbtList_GetPosition(work, 91);
    for (i = 0; i < COLUMN_SIZE; i++) {
        if (work->mask & (1 << (i + COLUMN_SIZE))) {
            func_0204c178(work->actors[ACTOR_PLATE + COLUMN_SIZE + i], &rightPos, 0);
            rightPos.x = rightX;
            func_0204c140(work->actors[ACTOR_PLATE + COLUMN_SIZE + i], &rightPos, 0);
            BmpOam_ActorSetPos(work->nameActors[COLUMN_SIZE + i], rightPos.x + rightNameX, rightPos.y + rightNameY);
        }
    }
}

// Starts a message, with the tournament's two names or the step's two Trainers as its words
static void WbtList_PrintMessage(WbtList *work, u32 messageId) {
    WordSet *wordSet = GFL_WordSetSystemCreate(8, 40, work->heapId);
    s32 words = 0;
    s32 first;
    s32 second;
    StrBuf *src;
    BmpWin *window;

    if ((work->messageFlags & MESSAGE_FLAG_TITLE) || (work->messageFlags & MESSAGE_FLAG_ENTRANTS)) {
        words = 1;
    }
    if (work->messageFlags & MESSAGE_FLAG_END) {
        words = 2;
    }
    if (work->mode == MODE_OPENING && words == 1) {
        func_0202437c(wordSet, 0, work->setup->tournamentName, 0, 1, 2);
        func_0202437c(wordSet, 1, work->setup->unk9C, 0, 1, 2);
    } else {
        first = WbtList_GetMatchValue(work, MATCH_FIRST, 0);
        second = WbtList_GetMatchValue(work, MATCH_SECOND, 0);
        if (work->messageFlags & MESSAGE_FLAG_RESULT) {
            first = WbtList_GetMatchValue(work, MATCH_WINNER, 0);
        }
        if (words == 2) {
            func_0202437c(wordSet, 0, work->setup->tournamentName, 0, 1, 2);
            func_0202437c(wordSet, 1, work->setup->unk9C, 0, 1, 2);
        } else {
            func_0202437c(wordSet, 0, work->setup->entrants[first].name, 0, 1, 2);
            func_0202437c(wordSet, 1, work->setup->entrants[second].name, 0, 1, 2);
        }
    }
    if (work->stream != NULL) {
        func_020223cc(work->stream);
    }
    if (work->strbuf != NULL) {
        GFL_StrBufFree(work->strbuf);
        work->strbuf = GFL_StrBufCreate(200, work->heapId);
    }
    src = GFL_MsgDataLoadStrbufNew(work->msgData, messageId);
    GFL_WordSetFormatStrbuf(wordSet, work->strbuf, src);
    work->stream = func_02022268(work->windows[WINDOW_MESSAGE].window, 0, 0, work->strbuf, work->font,
                                 func_02017bcc(), work->tcbManager, 4, work->heapId, 15);
    window = work->windows[WINDOW_MESSAGE].window;
    BmpWin_TransferNow(window);
    GFL_WordSetSystemFree(wordSet);
    GFL_StrBufFree(src);
}

// Returns whether the step's messages are still going. After a message that isn't the step's last comes one of the
// shuffled messages about the match
static BOOL WbtList_UpdateMessage(WbtList *work) {
    s32 line;
    u16 message;

    if (work->stream == NULL) {
        return FALSE;
    }
    KeyCursor_Update(work->keyCursor, work->stream, work->windows[WINDOW_MESSAGE].window);
    if (AppPrintsysCommon_Update(&work->printCommon, work->stream) == TRUE) {
        if (work->messageFlags & MESSAGE_FLAG_LAST) {
            return FALSE;
        }
        work->messageFlags |= MESSAGE_FLAG_LAST;
        work->messageFlags |= MESSAGE_FLAG_RESULT;
        WbtList_ClearMessage(work);
        line = work->lineOrder[work->lineCount];
        work->lineCount++;
        if (work->lineCount >= MESSAGE_LINE_COUNT - 1) {
            work->lineCount = MESSAGE_LINE_COUNT - 1;
        }
        message = WbtList_GetMatchValue(work, MATCH_MESSAGE, 0);
        WbtList_PrintMessage(work, message + line);
        return TRUE;
    }
    return TRUE;
}

// The message that starts the step, and what its words are
static u32 WbtList_GetMessage(WbtList *work) {
    s32 mode = work->mode;
    u32 messageId = sWbtListMessages[mode].title;

    work->messageFlags = 0;
    if (mode == MODE_OPENING) {
        work->messageFlags = MESSAGE_FLAG_LAST;
        if (work->step == 0) {
            messageId = sWbtListMessages[mode].first;
            work->messageFlags |= MESSAGE_FLAG_TITLE;
        } else if (work->step == 1) {
            messageId = sWbtListMessages[mode].second;
            work->messageFlags |= MESSAGE_FLAG_ENTRANTS;
        } else if (work->step == 2) {
            messageId = sWbtListMessages[mode].last;
            work->messageFlags |= MESSAGE_FLAG_END;
        }
    } else if (mode == MODE_FIRST_ROUND) {
        if (work->step == 4) {
            work->messageFlags = MESSAGE_FLAG_LAST;
            messageId = sWbtListMessages[mode].last;
            work->messageFlags |= MESSAGE_FLAG_END;
        }
    } else if (mode == MODE_SECOND_ROUND) {
        if (work->step == 2) {
            work->messageFlags = MESSAGE_FLAG_LAST;
            messageId = sWbtListMessages[mode].last;
            work->messageFlags |= MESSAGE_FLAG_END;
        }
    }
    return messageId;
}

static void WbtList_ClearMessage(WbtList *work) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_MESSAGE].window), 15);
    BmpWin_FlushChar(work->windows[WINDOW_MESSAGE].window);
}

static s32 WbtList_GetPosition(WbtList *work, u32 index) {
    if (index >= POSITION_COUNT) {
        return 0;
    }
    return work->positions[index];
}

// Shows one of the bracket's lines from the winner's end, drawing itself or drawn
static void WbtList_ShowLine(WbtList *work, s32 line, s32 drawn, s32 side) {
    s32 index = line * 2 * LINE_VALUE_COUNT + side * LINE_VALUE_COUNT;
    s16 position;
    s16 flags;
    ClActorPos pos;

    func_0204c488(work->actors[ACTOR_LINE + line], sWbtListLines[index + LINE_SEQUENCE + drawn]);
    func_0204c124(work->actors[ACTOR_LINE + line], TRUE);
    func_0204c520(work->actors[ACTOR_LINE + line], TRUE);
    position = sWbtListLines[index + LINE_POSITION];
    flags = sWbtListLines[index + LINE_FLAGS];
    if (flags & ACTOR_FLAG_FLIP_Y) {
        func_0204c2b0(work->actors[ACTOR_LINE + line], 1, TRUE);
    } else {
        func_0204c2b0(work->actors[ACTOR_LINE + line], 1, FALSE);
    }
    if (flags & ACTOR_FLAG_FLIP_X) {
        func_0204c2b0(work->actors[ACTOR_LINE + line], 0, TRUE);
    } else {
        func_0204c2b0(work->actors[ACTOR_LINE + line], 0, FALSE);
    }
    pos.x = WbtList_GetPosition(work, position * 2);
    pos.y = WbtList_GetPosition(work, position * 2 + 1);
    func_0204c140(work->actors[ACTOR_LINE + line], &pos, 0);
}

static BOOL WbtList_IsLineAnimating(WbtList *work, s32 line) {
    return WbtList_IsActorAnimating(work, ACTOR_LINE + line);
}

// In the second round, the piece that joins the first round's line to the second's
static void WbtList_ShowLineJoint(WbtList *work) {
    s32 joint;
    s32 line;
    s32 side;

    if (work->mode == MODE_SECOND_ROUND) {
        joint = 3;
        line = WbtList_GetMatchValue(work, MATCH_LINE, -1);
        side = WbtList_GetMatchValue(work, MATCH_SIDE, -1);
        if (line == 4) {
            if (side == 0) {
                joint = 1;
            } else {
                joint = 0;
            }
        } else if (side != 0) {
            joint = 2;
        }
        WbtList_SetActorAnim(work, ACTOR_LINE + joint, 14);
    }
}

// Shows the bracket as it is before the screen's first step: nothing, or in the second round the first round's
// losers and lines
static void WbtList_ResetBracket(WbtList *work) {
    s32 i;
    s32 j;

    if (work->mode == MODE_FIRST_ROUND) {
        for (i = 0; i < ENTRANT_COUNT; i++) {
            WbtList_SetActorAnim(work, ACTOR_PANEL + i, i >= COLUMN_SIZE ? 4 : 0);
        }
        WbtList_SetPaletteGray(work, -1, TRAINER_PALETTE, TRUE);
        for (j = 0; j < LINE_COUNT; j++) {
            WbtList_SetActorVisible(work, ACTOR_LINE + j, FALSE);
        }
        for (i = 0; i < ENTRANT_COUNT; i++) {
            WbtList_PlaceEntrant(work, i, TRUE);
        }
    } else if (work->mode == MODE_SECOND_ROUND) {
        for (i = 0; i < ENTRANT_COUNT; i++) {
            WbtList_SetActorAnim(work, ACTOR_PANEL + i, i >= COLUMN_SIZE ? 4 : 0);
        }
        WbtList_SetPaletteGray(work, -1, TRAINER_PALETTE, TRUE);
        for (i = 0; i < 4; i++) {
            s32 loser = work->matches[3 + i].loser;

            WbtList_SetActorAnim(work, ACTOR_PANEL + loser, i >= 2 ? 5 : 1);
            WbtList_SetPaletteGray(work, loser, TRAINER_PALETTE, FALSE);
        }
        for (j = 0; j < LINE_COUNT; j++) {
            WbtList_SetActorVisible(work, ACTOR_LINE + j, FALSE);
        }
        for (i = 0; i < 4; i++) {
            WbtList_ShowLine(work, work->matches[3 + i].line, 1, work->matches[3 + i].side);
        }
        for (i = 0; i < ENTRANT_COUNT; i++) {
            WbtList_PlaceEntrant(work, i, TRUE);
        }
    } else {
        for (i = 0; i < ENTRANT_COUNT; i++) {
            WbtList_SetActorAnim(work, ACTOR_PANEL + i, i >= COLUMN_SIZE ? 4 : 0);
        }
        WbtList_SetPaletteGray(work, -1, TRAINER_PALETTE, TRUE);
        for (j = 0; j < LINE_COUNT; j++) {
            WbtList_SetActorVisible(work, ACTOR_LINE + j, FALSE);
        }
        for (i = 0; i < ENTRANT_COUNT; i++) {
            WbtList_PlaceEntrant(work, i, TRUE);
        }
    }
}

static void WbtList_InitEntrants(WbtList *work) {
    BOOL found = FALSE;
    s16 i;

    for (i = 0; i < ENTRANT_COUNT; i++) {
        s32 sequence;
        s32 palette;

        if (i < COLUMN_SIZE) {
            sequence = 2;
            if (work->setup->entrants[i].unk7 == 1) {
                sequence = 3;
            }
        } else {
            sequence = 6;
            if (work->setup->entrants[i].unk7 == 1) {
                sequence = 7;
            }
        }
        WbtList_SetActorAnim(work, ACTOR_PLATE + i, sequence);
        palette = 0;
        if (work->setup->entrants[i].unk7 == 1) {
            palette = 1;
        }
        BmpOam_ActorSetPaletteOffset(work->nameActors[i], (u8)palette);
        if (work->setup->entrants[i].isPlayer == TRUE && !found) {
            found = TRUE;
            work->playerIndex = i;
        }
    }
    WbtList_InitMatches(work);
    work->mask = WbtList_GetMatchValue(work, MATCH_MASK, 0);
    WbtList_ShuffleLines(work);
}

// The steps of the three modes: the opening's three at 0, where the last has everyone but the player's first match
// slide out; the first round's four matches and the player's next match at 3; and the second round's two matches and
// the final at 8
static void WbtList_InitMatches(WbtList *work) {
    s32 i;

    for (i = 0; i < MATCH_COUNT; i++) {
        work->matches[i].loser = 0;
        work->matches[i].winner = 0;
        work->matches[i].mask = 0xff;
        work->matches[i].line = 0;
        work->matches[i].side = 0;
        work->matches[i].message = 0;
        work->matches[i].first = 0;
        work->matches[i].second = 0;
    }
    if (work->mode == MODE_OPENING) {
        work->matches[0].mask = 0;
        i = 2;
        if (work->playerIndex % 2 == 0) {
            work->matches[i].first = work->playerIndex;
            work->matches[i].second = work->playerIndex + 1;
            work->matches[i].mask = 0xff;
            work->matches[i].mask &= ~(1 << work->playerIndex);
            work->matches[i].mask &= ~(1 << (work->playerIndex + 1));
        } else {
            work->matches[i].first = work->playerIndex - 1;
            work->matches[i].second = work->playerIndex;
            work->matches[i].mask = 0xff;
            work->matches[i].mask &= ~(1 << (work->playerIndex - 1));
            work->matches[i].mask &= ~(1 << work->playerIndex);
        }
    }
    if (work->mode == MODE_FIRST_ROUND || work->mode == MODE_SECOND_ROUND) {
        for (i = 0; i < 4; i++) {
            if (work->setup->firstRound[i].unk0 == 1) {
                work->matches[3 + i].winner = i * 2;
                work->matches[3 + i].loser = i * 2 + 1;
                work->matches[3 + i].side = 0;
            } else {
                work->matches[3 + i].winner = i * 2 + 1;
                work->matches[3 + i].loser = i * 2;
                work->matches[3 + i].side = 1;
            }
            work->matches[3 + i].mask = 1 << (i * 2);
            work->matches[3 + i].mask |= 1 << (i * 2 + 1);
            work->matches[3 + i].line = i;
            work->matches[3 + i].message = work->setup->firstRound[i].unk4;
            work->matches[3 + i].first = i * 2;
            work->matches[3 + i].second = i * 2 + 1;
        }
        // The step after the four matches
        i = 7;
        if (work->playerIndex < COLUMN_SIZE) {
            work->matches[i].mask = 1 << work->matches[3].winner;
            work->matches[i].mask |= 1 << work->matches[4].winner;
            work->matches[i].first = work->matches[3].winner;
            work->matches[i].second = work->matches[4].winner;
        } else {
            work->matches[i].mask = 1 << work->matches[5].winner;
            work->matches[i].mask |= 1 << work->matches[6].winner;
            work->matches[i].first = work->matches[5].winner;
            work->matches[i].second = work->matches[6].winner;
        }
        work->matches[i].message = Bank0406_Text_LetsMoveNextBattle;
    }
    if (work->mode == MODE_SECOND_ROUND) {
        for (i = 0; i < 2; i++) {
            if (work->setup->secondRound[i].unk0 == 1) {
                work->matches[8 + i].winner = work->matches[3 + i * 2].winner;
                work->matches[8 + i].loser = work->matches[4 + i * 2].winner;
                work->matches[8 + i].side = 0;
            } else {
                work->matches[8 + i].winner = work->matches[4 + i * 2].winner;
                work->matches[8 + i].loser = work->matches[3 + i * 2].winner;
                work->matches[8 + i].side = 1;
            }
            work->matches[8 + i].mask = 1 << work->matches[3 + i * 2].winner;
            work->matches[8 + i].mask |= 1 << work->matches[4 + i * 2].winner;
            work->matches[8 + i].line = 4 + i;
            work->matches[8 + i].message = work->setup->secondRound[i].unk4;
            work->matches[8 + i].first = work->matches[3 + i * 2].winner;
            work->matches[8 + i].second = work->matches[4 + i * 2].winner;
        }
        // The step after the two matches
        i = 10;
        work->matches[i].mask = 1 << work->matches[8].winner;
        work->matches[i].mask |= 1 << work->matches[9].winner;
        work->matches[i].message = Bank0406_Text_WhichTrainerWillWin;
        work->matches[i].first = work->matches[8].winner;
        work->matches[i].second = work->matches[9].winner;
    }
}

// A MATCH_* value of the mode's current step, or of one at an offset from it
static s32 WbtList_GetMatchValue(WbtList *work, s32 field, s32 offset) {
    s32 value = 0;

    if (work->mode == MODE_FIRST_ROUND) {
        offset += work->step + 3;
        if (offset >= 8) {
            offset = 7;
        }
    } else if (work->mode == MODE_SECOND_ROUND) {
        offset += work->step + 8;
        if (offset >= 11) {
            offset = 10;
        }
    } else if (work->mode == MODE_OPENING) {
        offset += work->step;
        if (offset >= 3) {
            offset = 2;
        }
    }
    if (field == MATCH_LOSER) {
        value = work->matches[offset].loser;
    } else if (field == MATCH_WINNER) {
        value = work->matches[offset].winner;
    } else if (field == MATCH_MASK) {
        value = work->matches[offset].mask;
    } else if (field == MATCH_LINE) {
        value = work->matches[offset].line;
    } else if (field == MATCH_SIDE) {
        value = work->matches[offset].side;
    } else if (field == MATCH_MESSAGE) {
        value = work->matches[offset].message;
    } else if (field == MATCH_FIRST) {
        value = work->matches[offset].first;
    } else if (field == MATCH_SECOND) {
        value = work->matches[offset].second;
    }
    return value;
}

// Returns whether the step is the mode's last
static BOOL WbtList_NextStep(WbtList *work) {
    BOOL last = FALSE;

    work->step++;
    if (work->mode == MODE_FIRST_ROUND) {
        if (work->step >= 4) {
            work->step = 4;
            last = TRUE;
        }
    } else if (work->mode == MODE_SECOND_ROUND) {
        if (work->step >= 2) {
            work->step = 2;
            last = TRUE;
        }
    } else if (work->mode == MODE_OPENING) {
        if (work->step >= 2) {
            work->step = 2;
            last = TRUE;
        }
    }
    return last;
}

static BOOL WbtList_IsLastStep(WbtList *work) {
    BOOL last = FALSE;

    if (work->mode == MODE_FIRST_ROUND) {
        if (work->step >= 4) {
            last = TRUE;
        }
    } else if (work->mode == MODE_SECOND_ROUND) {
        if (work->step >= 2) {
            last = TRUE;
        }
    } else if (work->mode == MODE_OPENING) {
        if (work->step >= 2) {
            last = TRUE;
        }
    }
    return last;
}

// Picks which four of the five messages about a match are shown, and in which order
static void WbtList_ShuffleLines(WbtList *work) {
    s32 count = 0;
    s32 tries;
    s32 i;

    for (tries = 0; tries < 128; tries++) {
        BOOL used = FALSE;
        u32 line = GFL_RandomLC(5);

        for (i = 0; i < MESSAGE_LINE_COUNT; i++) {
            if (line == work->lineOrder[i]) {
                used = TRUE;
                break;
            }
        }
        if (!used) {
            work->lineOrder[count] = line;
            count++;
            if (count >= MESSAGE_LINE_COUNT) {
                break;
            }
        }
    }
    if (tries == 128) {
        for (i = 0; i < MESSAGE_LINE_COUNT; i++) {
            work->lineOrder[i] = i;
        }
    }
    work->lineCount = 0;
}

static void WbtList_Stub(WbtList *work) {
}
