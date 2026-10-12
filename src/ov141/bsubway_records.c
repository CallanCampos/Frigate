#include "types.h"
#include "app/bsubway_records.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/bsubway_save.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/cursor_move.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms_data.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/title_6.h"

// bsubway_records.c, a descriptive name: the Battle Subway's records screen, overlay 141. It lists the records that the
// Battle Subway's save holds, ten to a page on the main screen's frames, and shows the one under the cursor below

// The steps of the screen
enum {
    STATE_FADE_IN,
    STATE_INPUT,
    STATE_WAIT_BACK,
    STATE_FADE_OUT,
    STATE_WAIT_FADE_OUT,
};

#define RECORDS_PER_PAGE 10
#define MESSAGE_COUNT 7
#define WINDOW_COUNT 18
// The windows that draw the list's frames come first, then the ones for the record under the cursor
#define FRAME_WINDOW_COUNT RECORDS_PER_PAGE
#define INFO_WINDOW_COUNT 8
#define BG_COUNT 5

// The windows of the record under the cursor
enum {
    INFO_WINDOW_TITLE = FRAME_WINDOW_COUNT,
    INFO_WINDOW_RANK,
    INFO_WINDOW_TRAIN_NUMBER,
    INFO_WINDOW_NAME,
    INFO_WINDOW_LOCATION_LABEL,
    INFO_WINDOW_LOCATION,
    INFO_WINDOW_QUOTE_LABEL,
    INFO_WINDOW_QUOTE,
};

// The actors: the cursor, the back button and the page arrows
enum {
    ACTOR_CURSOR,
    ACTOR_BACK,
    ACTOR_ARROW_LEFT,
    ACTOR_ARROW_RIGHT,
    ACTOR_COUNT,
};

// The pages' arrows
#define PAGE_PREV 0
#define PAGE_NEXT 1

// What the touch and key input gives besides the cursor's choices
#define INPUT_PREV_PAGE 0
#define INPUT_NEXT_PAGE 1
#define INPUT_BACK 2
#define INPUT_NONE (-1)

// A record of the Battle Subway's save, which BSubwayWifiData_AllocLeaders copies: a player's name and place, and the message that
// they left
typedef struct {
    u16 name[8];
    u8 unk10[2];
    u8 country;
    u8 area;
    u8 unk14[4];
    PMSData message;
    u8 unk20_0 : 1;
    u8 female : 1;
    u8 unk20_2 : 6;
    u8 unk21;
} BSubwayRecord;

typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 baseChar;
} BSubwayRecordsWindow;

typedef struct {
    int x;
    int y;
    int sequence;
    u32 charIndex;
    u32 palIndex;
    u32 cellIndex;
} BSubwayRecordsActor;

typedef struct {
    ArcTool *arc;
    BGWinFrame *frames;
    BSubwayRecord *records;
    Font *font;
    BmpWin *windows[WINDOW_COUNT];
    PrintWindow printWindows[WINDOW_COUNT];
    PrintQueue *printQueue;
    u8 unkEC[0x8];
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *messages[MESSAGE_COUNT];
    ClActUnit *clUnits[2];
    ClActor *actors[ACTOR_COUNT];
    u32 objRes[6];
    CursorMove *cursor;
    TCB *vblankTcb;
    BSubwayRecordsParam *param;
    int state;
    u32 unk15C;
    u16 page;
    u16 pageCount;
    int count;
} BSubwayRecordsWork;

static BOOL BSubwayRecords_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BSubwayRecords_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BSubwayRecords_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void BSubwayRecords_VBlank(TCB *tcb, void *data);
static void BSubwayRecords_InitData(BSubwayRecordsWork *wk, BSubwayRecordsParam *param);
static void BSubwayRecords_FreeData(BSubwayRecordsWork *wk);
static void BSubwayRecords_InitVRAM(void);
static void BSubwayRecords_InitBG(BSubwayRecordsWork *wk);
static void BSubwayRecords_FreeBG(BSubwayRecordsWork *wk);
static void BSubwayRecords_LoadBG(BSubwayRecordsWork *wk);
static void BSubwayRecords_FreeFrames(BSubwayRecordsWork *wk);
static void BSubwayRecords_InitObjRes(BSubwayRecordsWork *wk);
static void BSubwayRecords_CreateActors(BSubwayRecordsWork *wk);
static void BSubwayRecords_FreeObjRes(BSubwayRecordsWork *wk);
static void BSubwayRecords_CreateWindows(BSubwayRecordsWork *wk);
static void BSubwayRecords_FreeWindows(BSubwayRecordsWork *wk);
static void BSubwayRecords_InitText(BSubwayRecordsWork *wk);
static void BSubwayRecords_FreeText(BSubwayRecordsWork *wk);
static void BSubwayRecords_FlushText(BSubwayRecordsWork *wk);
static BOOL BSubwayRecords_RunState(BSubwayRecordsWork *wk);
static BOOL BSubwayRecords_StateFadeIn(BSubwayRecordsWork *wk);
static BOOL BSubwayRecords_StateInput(BSubwayRecordsWork *wk);
static BOOL BSubwayRecords_StateWaitBack(BSubwayRecordsWork *wk);
static BOOL BSubwayRecords_StateFadeOut(BSubwayRecordsWork *wk);
static BOOL BSubwayRecords_StateWaitFadeOut(BSubwayRecordsWork *wk);
static void BSubwayRecords_CreateCursor(BSubwayRecordsWork *wk);
static void BSubwayRecords_FreeCursor(BSubwayRecordsWork *wk);
static void BSubwayRecords_OnCursorOn(void *work, int pos, int prevPos);
static void BSubwayRecords_OnCursorOff(void *work, int pos, int prevPos);
static void BSubwayRecords_OnCursorMove(void *work, int pos, int prevPos);
static void BSubwayRecords_OnCursorTouch(void *work, int pos, int prevPos);
static void BSubwayRecords_MoveCursorActor(BSubwayRecordsWork *wk, int pos);
static void BSubwayRecords_ShowRecord(BSubwayRecordsWork *wk, BSubwayRecord *records, int index);
static int BSubwayRecords_GetInput(BSubwayRecordsWork *wk);
static void BSubwayRecords_HandleInput(BSubwayRecordsWork *wk, int input);
static void BSubwayRecords_InitPages(BSubwayRecordsWork *wk);
static void BSubwayRecords_UpdateArrows(BSubwayRecordsWork *wk, int page, int pageCount);
static BOOL BSubwayRecords_StepPage(BSubwayRecordsWork *wk, int direction);
static void BSubwayRecords_ShowPage(BSubwayRecordsWork *wk, int page);
static int BSubwayRecords_GetPageSize(int page, int pageCount, int count);
static void BSubwayRecords_ShowFrames(BSubwayRecordsWork *wk);
static void BSubwayRecords_DrawFrame(BSubwayRecordsWork *wk, int index, StrBuf *name, BOOL female);

// Declared in an order that gives the original layout of the data (tools/decomp/rodata_order.py)

static const BSubwayRecordsWindow sBSubwayRecordsWindow5 = { 4, 2, 12, 28, 4, 1, 0xc8 };
static const BSubwayRecordsWindow sBSubwayRecordsWindow4 = { 4, 2, 10, 28, 2, 1, 0x90 };
static const BSubwayRecordsWindow sBSubwayRecordsWindow1 = { 4, 2, 3, 11, 2, 1, 0x44 };
static const BSubwayRecordsWindow sBSubwayRecordsWindow6 = { 4, 2, 16, 16, 2, 1, 0x138 };
static const BSubwayRecordsWindow sBSubwayRecordsWindow7 = { 4, 2, 18, 28, 4, 1, 0x158 };
static const BSubwayRecordsWindow sBSubwayRecordsWindow3 = { 4, 2, 8, 10, 2, 1, 0x7c };
static const BSubwayRecordsWindow sBSubwayRecordsWindow2 = { 4, 13, 3, 17, 2, 1, 0x5a };
static const BSubwayRecordsWindow sBSubwayRecordsWindow0 = { 4, 2, 1, 24, 2, 1, 0x14 };

static const CursorMoveCallbacks sBSubwayRecordsCursorCallbacks = {
    BSubwayRecords_OnCursorOn,
    BSubwayRecords_OnCursorOff,
    BSubwayRecords_OnCursorMove,
    BSubwayRecords_OnCursorTouch,
};

// The buttons under the list: the left and right arrows and the back button
static const TouchRect sBSubwayRecordsTouchRects[] = {
    { 168, 191, 8, 32 },
    { 168, 191, 32, 56 },
    { 168, 191, 232, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const u32 sBSubwayRecordsBGs[BG_COUNT] = { 4, 5, 0, 1, 2 };

static const BGSysVRAMConfig sBSubwayRecordsVRAMConfig = {
    GX_VRAM_BG_128_B, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,  GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

const GameProcFunctions BSUBWAY_RECORDS_PROC_FUNCTIONS = {
    BSubwayRecords_Init,
    BSubwayRecords_Main,
    BSubwayRecords_Exit,
};

// Where the list's frames go on their BG: two columns of five
static const int sBSubwayRecordsFramePositions[RECORDS_PER_PAGE][2] = {
    { 1, 0 }, { 17, 0 }, { 1, 4 }, { 17, 4 }, { 1, 8 }, { 17, 8 }, { 1, 12 }, { 17, 12 }, { 1, 16 }, { 17, 16 },
};

static const BSubwayRecordsActor sBSubwayRecordsActors[ACTOR_COUNT] = {
    { 64, 16, 0, 1, 0, 2 },
    { 224, 168, 1, 4, 4, 4 },
    { 8, 168, 4, 4, 4, 4 },
    { 32, 168, 5, 4, 4, 4 },
};

// The cursor's positions: the ten frames, then the end
static const CursorMoveData sBSubwayRecordsCursorData[RECORDS_PER_PAGE + 1] = {
    { 64, 16, 0, 0, 0, 2, 0, 1, { 0, 31, 8, 119 } },         { 192, 16, 0, 0, 1, 3, 0, 1, { 0, 31, 136, 244 } },
    { 64, 48, 0, 0, 0, 4, 2, 3, { 32, 63, 8, 119 } },        { 192, 48, 0, 0, 1, 5, 2, 3, { 32, 63, 136, 244 } },
    { 64, 80, 0, 0, 2, 6, 4, 5, { 64, 95, 8, 119 } },        { 192, 80, 0, 0, 3, 7, 4, 5, { 64, 95, 136, 244 } },
    { 64, 112, 0, 0, 4, 8, 6, 7, { 96, 127, 8, 119 } },      { 192, 112, 0, 0, 5, 9, 6, 7, { 96, 127, 136, 244 } },
    { 64, 144, 0, 0, 6, 8, 8, 9, { 128, 159, 8, 119 } },     { 192, 144, 0, 0, 7, 9, 8, 9, { 128, 159, 136, 244 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static BOOL (*sBSubwayRecordsStateFuncs[])(BSubwayRecordsWork *wk) = {
    BSubwayRecords_StateFadeIn,  BSubwayRecords_StateInput,       BSubwayRecords_StateWaitBack,
    BSubwayRecords_StateFadeOut, BSubwayRecords_StateWaitFadeOut,
};

static const BSubwayRecordsWindow *sBSubwayRecordsWindows[INFO_WINDOW_COUNT] = {
    &sBSubwayRecordsWindow0, &sBSubwayRecordsWindow1, &sBSubwayRecordsWindow2, &sBSubwayRecordsWindow3,
    &sBSubwayRecordsWindow4, &sBSubwayRecordsWindow5, &sBSubwayRecordsWindow6, &sBSubwayRecordsWindow7,
};

static BOOL BSubwayRecords_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BSubwayRecordsWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BSUBWAY_RECORDS, 0x20000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BSubwayRecordsWork), HEAPID_BSUBWAY_RECORDS);
    sys_memset32_fast(0, wk, sizeof(BSubwayRecordsWork));
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    BSubwayRecords_InitData(wk, param);
    BSubwayRecords_InitVRAM();
    BSubwayRecords_InitBG(wk);
    BSubwayRecords_LoadBG(wk);
    BSubwayRecords_InitObjRes(wk);
    BSubwayRecords_CreateActors(wk);
    BSubwayRecords_CreateWindows(wk);
    BSubwayRecords_InitText(wk);
    BSubwayRecords_CreateCursor(wk);
    BSubwayRecords_UpdateArrows(wk, wk->page, wk->pageCount);
    BSubwayRecords_ShowPage(wk, wk->page);
    GFL_WipeSet(0, 1, 1, 0, 16, TRUE, HEAPID_BSUBWAY_RECORDS);
    func_02042ba8(TRUE, HEAPID_BSUBWAY_RECORDS);
    return TRUE;
}

static BOOL BSubwayRecords_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BSubwayRecordsWork *wk = work;

    if (!BSubwayRecords_RunState(wk)) {
        return TRUE;
    }
    BSubwayRecords_FlushText(wk);
    BGWinFrame_UpdateMoves(wk->frames);
    func_0204b794();
    return FALSE;
}

static BOOL BSubwayRecords_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BSubwayRecordsWork *wk = work;

    BSubwayRecords_FreeCursor(wk);
    BSubwayRecords_FreeText(wk);
    BSubwayRecords_FreeWindows(wk);
    BSubwayRecords_FreeObjRes(wk);
    BSubwayRecords_FreeFrames(wk);
    BSubwayRecords_FreeBG(wk);
    BSubwayRecords_FreeData(wk);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapStatusValidate(HEAPID_BSUBWAY_RECORDS);
    GFL_HeapDelete(HEAPID_BSUBWAY_RECORDS);
    return TRUE;
}

static void BSubwayRecords_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
    GFL_BGSysUpdate();
}

static void BSubwayRecords_InitData(BSubwayRecordsWork *wk, BSubwayRecordsParam *param) {
    int i;

    wk->param = param;
    wk->unk15C = 0;
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_BSUBWAY_RECORDS, HEAPID_BSUBWAY_RECORDS);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_6, HEAPID_BSUBWAY_RECORDS);
    for (i = 0; i < MESSAGE_COUNT; i++) {
        wk->messages[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, i);
        GFL_StrBufGetStringPtr(wk->messages[i]);
    }
    wk->strbuf = GFL_StrBufCreate(0x40, HEAPID_BSUBWAY_RECORDS);
    wk->wordSet = GFL_WordSetSystemCreate(8, 0x40, HEAPID_BSUBWAY_RECORDS);
    wk->vblankTcb = GFL_VBlankTCBAdd(BSubwayRecords_VBlank, wk, 0);
    wk->records = BSubwayWifiData_AllocLeaders(func_02017968(wk->param->gameData), HEAPID_BSUBWAY_RECORDS);
    wk->count = BSubwayWifiLeader_CountList(wk->records);
    BSubwayRecords_InitPages(wk);
}

static void BSubwayRecords_FreeData(BSubwayRecordsWork *wk) {
    int i;

    GFL_HeapFree(wk->records);
    for (i = 0; i < MESSAGE_COUNT; i++) {
        GFL_StrBufFree(wk->messages[i]);
    }
    GFL_StrBufFree(wk->strbuf);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    GFL_TCBRemove(wk->vblankTcb);
    GFL_ArcToolFree(wk->arc);
}

static void BSubwayRecords_InitVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sBSubwayRecordsVRAMConfig);
    reg_GX_POWCNT = reg_GX_POWCNT & ~REG_GX_POWCNT_DSEL_MASK;
}

static void BSubwayRecords_InitBG(BSubwayRecordsWork *wk) {
    u32 i;

    GFL_BGSysCreate(HEAPID_BSUBWAY_RECORDS);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup setups[BG_COUNT] = {
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x00000), 0x8000,
              GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000), 0x4000,
              GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x00000), 0x4000,
              GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000), 0x8000,
              GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x08000), 0x4000,
              GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
        };

        for (i = 0; i < BG_COUNT; i++) {
            u32 bg = sBSubwayRecordsBGs[i];

            GFL_BGSysCreateBG(bg, &setups[i], BGMODE_TEXT);
            GFL_BGSysClearScr(bg);
            GFL_BGSysClearCharCore(bg, BGSYS_TILE_SIZE_16, 0, HEAPID_BSUBWAY_RECORDS);
            GFL_BGSysSetBGEnabled(bg, TRUE);
        }
    }
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(7, FALSE);
    GFL_BGSysSetBGEnabled(3, FALSE);
}

static void BSubwayRecords_FreeBG(BSubwayRecordsWork *wk) {
    u32 i;

    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sBSubwayRecordsBGs[i]);
    }
    GFL_BGSysFree();
}

static void BSubwayRecords_LoadBG(BSubwayRecordsWork *wk) {
    ArcTool *arc = wk->arc;
    ArcTool *uiArc;
    u32 offset;
    int i;

    GFL_BGSysLoadArcNCGRStatic(arc, 5, 5, 0, 0, TRUE, HEAPID_BSUBWAY_RECORDS);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 5, 0, 0, TRUE, HEAPID_BSUBWAY_RECORDS);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 4, 0, 0, HEAPID_BSUBWAY_RECORDS);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 2, 0, 0, TRUE, HEAPID_BSUBWAY_RECORDS);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0, 2, 0, 0, TRUE, HEAPID_BSUBWAY_RECORDS);
    GFL_G2DIOLoadArcNCLRDefault(arc, 2, 0, 0, 0, HEAPID_BSUBWAY_RECORDS);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 1, 0, 0, TRUE, HEAPID_BSUBWAY_RECORDS);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0, 1, 0, 0, TRUE, HEAPID_BSUBWAY_RECORDS);
    uiArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BSUBWAY_RECORDS);
    GFL_BGSysLoadArcNCGRStatic(uiArc, func_0202d824(), 1, 0, 0, FALSE, HEAPID_BSUBWAY_RECORDS);
    loadBGScrToVramByFileNoReserveNegAlign(uiArc, func_0202d828(), 1, 0, 0, FALSE, HEAPID_BSUBWAY_RECORDS);
    offset = 0x1c0;
    GFL_G2DIOLoadArcNCLR(uiArc, func_0202d820(), 0, 0, offset, 0x20, HEAPID_BSUBWAY_RECORDS);
    GFL_BGSysSetScrPaletteNo(1, 0, 21, 32, 3, 14);
    GFL_BGSysLoadScr(1);
    GFL_ArcToolFree(uiArc);
    offset += 0x20;
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, offset, 0x20, HEAPID_BSUBWAY_RECORDS);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, offset, 0x20, HEAPID_BSUBWAY_RECORDS);
    wk->frames = BGWinFrame_Create(BGWINFRAME_TRANSFER_VBLANK, RECORDS_PER_PAGE, HEAPID_BSUBWAY_RECORDS);
    for (i = 0; i < RECORDS_PER_PAGE; i++) {
        BGWinFrame_InitFrame(wk->frames, i, 1, 14, 4);
        BGWinFrame_LoadScreenArc(wk->frames, i, arc, 3, TRUE);
        BGWinFrame_Put(wk->frames, i, sBSubwayRecordsFramePositions[i][0], sBSubwayRecordsFramePositions[i][1]);
    }
}

static void BSubwayRecords_FreeFrames(BSubwayRecordsWork *wk) {
    BGWinFrame_Delete(wk->frames);
}

static void BSubwayRecords_InitObjRes(BSubwayRecordsWork *wk) {
    ArcTool *uiArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BSUBWAY_RECORDS);

    ClActSys_Create(&data_02093f08, &sBSubwayRecordsVRAMConfig, HEAPID_BSUBWAY_RECORDS);
    wk->clUnits[0] = func_0204bf1c(4, 1, HEAPID_BSUBWAY_RECORDS);
    wk->clUnits[1] = func_0204bf1c(2, 1, HEAPID_BSUBWAY_RECORDS);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    wk->objRes[1] = func_0204b81c(wk->arc, 7, TRUE, CLACT_VRAM_BOTH, HEAPID_BSUBWAY_RECORDS);
    wk->objRes[0] = func_0204bba0(wk->arc, 10, CLACT_VRAM_BOTH, 0, HEAPID_BSUBWAY_RECORDS);
    wk->objRes[2] = func_0204bde0(wk->arc, 8, 9, HEAPID_BSUBWAY_RECORDS);
    wk->objRes[4] = func_0204b81c(uiArc, func_0202d814(), FALSE, CLACT_VRAM_MAIN, HEAPID_BSUBWAY_RECORDS);
    wk->objRes[3] = func_0204bbb8(uiArc, func_0202d810(), CLACT_VRAM_MAIN, 0x160, 0, 3, HEAPID_BSUBWAY_RECORDS);
    wk->objRes[5] = func_0204bde0(uiArc, func_0202d818(0), func_0202d81c(0), HEAPID_BSUBWAY_RECORDS);
    GFL_ArcToolFree(uiArc);
}

static void BSubwayRecords_CreateActors(BSubwayRecordsWork *wk) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        const BSubwayRecordsActor *def = &sBSubwayRecordsActors[i];
        ClActorSetup setup;

        setup.x = def->x;
        setup.y = def->y;
        setup.sequence = def->sequence;
        setup.bgPriority = 0;
        setup.priority = 0;
        wk->actors[i] = func_0204c040(wk->clUnits[0], wk->objRes[def->charIndex], wk->objRes[def->palIndex],
                                      wk->objRes[def->cellIndex], &setup, 0, HEAPID_BSUBWAY_RECORDS);
        func_0204c520(wk->actors[i], TRUE);
    }
}

static void BSubwayRecords_FreeObjRes(BSubwayRecordsWork *wk) {
    func_0204b98c(wk->objRes[4]);
    func_0204bcd0(wk->objRes[3]);
    func_0204be64(wk->objRes[5]);
    func_0204b98c(wk->objRes[1]);
    func_0204bcd0(wk->objRes[0]);
    func_0204be64(wk->objRes[2]);
    func_0204bf98(wk->clUnits[1]);
    func_0204bf98(wk->clUnits[0]);
    func_0204b758();
}

static void BSubwayRecords_CreateWindows(BSubwayRecordsWork *wk) {
    int i;

    BmpWin_InitAllocator(HEAPID_BSUBWAY_RECORDS);
    for (i = 0; i < INFO_WINDOW_COUNT; i++) {
        const BSubwayRecordsWindow *def = sBSubwayRecordsWindows[i];

        wk->windows[FRAME_WINDOW_COUNT + i] =
            BmpWin_CreateDynamic(def->bg, def->x, def->y, def->width, def->height, def->palette, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[FRAME_WINDOW_COUNT + i]), 0);
        BmpWin_TransferNow(wk->windows[FRAME_WINDOW_COUNT + i]);
    }
    for (i = 0; i < FRAME_WINDOW_COUNT; i++) {
        wk->windows[i] = BmpWin_CreateDynamic(1, 2, 1, 10, 2, 1, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 4);
    }
}

static void BSubwayRecords_FreeWindows(BSubwayRecordsWork *wk) {
    int i;

    for (i = 0; i < FRAME_WINDOW_COUNT; i++) {
        BmpWin_Free(wk->windows[i]);
    }
    for (i = 0; i < INFO_WINDOW_COUNT; i++) {
        BmpWin_Free(wk->windows[FRAME_WINDOW_COUNT + i]);
    }
    BmpWin_FreeAllocator();
}

static void BSubwayRecords_InitText(BSubwayRecordsWork *wk) {
    int i;

    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_BSUBWAY_RECORDS);
    wk->printQueue = func_02021998(HEAPID_BSUBWAY_RECORDS);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintWindow_Init(&wk->printWindows[i], wk->windows[i]);
    }
    PrintWindow_Print(&wk->printWindows[INFO_WINDOW_TITLE], wk->printQueue, 0, 0,
                      wk->messages[Title6_Text_SuccessiveTrainers], wk->font, PRINT_COLOR(15, 14, 0));
    PrintWindow_Print(&wk->printWindows[INFO_WINDOW_LOCATION_LABEL], wk->printQueue, 0, 0,
                      wk->messages[Title6_Text_Location], wk->font, PRINT_COLOR(15, 14, 0));
    PrintWindow_Print(&wk->printWindows[INFO_WINDOW_QUOTE_LABEL], wk->printQueue, 0, 0,
                      wk->messages[Title6_Text_TrainersQuote], wk->font, PRINT_COLOR(15, 14, 0));
}

static void BSubwayRecords_FreeText(BSubwayRecordsWork *wk) {
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
}

static void BSubwayRecords_FlushText(BSubwayRecordsWork *wk) {
    int i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintWindow_Flush(&wk->printWindows[i], wk->printQueue);
    }
}

static BOOL BSubwayRecords_RunState(BSubwayRecordsWork *wk) {
    return sBSubwayRecordsStateFuncs[wk->state](wk);
}

static BOOL BSubwayRecords_StateFadeIn(BSubwayRecordsWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->state = STATE_INPUT;
    }
    return TRUE;
}

static BOOL BSubwayRecords_StateInput(BSubwayRecordsWork *wk) {
    int input;

    if (!func_02021c0c(wk->printQueue)) {
        return TRUE;
    }
    input = BSubwayRecords_GetInput(wk);
    if (input == INPUT_NONE) {
        input = CursorMove_UpdatePressed(wk->cursor);
        if (input == CURSOR_MOVE_EDGE_LEFT) {
            BSubwayRecords_HandleInput(wk, INPUT_PREV_PAGE);
        } else if (input == CURSOR_MOVE_EDGE_RIGHT) {
            BSubwayRecords_HandleInput(wk, INPUT_NEXT_PAGE);
        }
    }
    return TRUE;
}

static BOOL BSubwayRecords_StateWaitBack(BSubwayRecordsWork *wk) {
    if (!func_0204c560(wk->actors[ACTOR_BACK])) {
        wk->state = STATE_FADE_OUT;
    }
    return TRUE;
}

static BOOL BSubwayRecords_StateFadeOut(BSubwayRecordsWork *wk) {
    GFL_WipeSet(0, 0, 0, 0, 16, TRUE, HEAPID_BSUBWAY_RECORDS);
    wk->state = STATE_WAIT_FADE_OUT;
    return TRUE;
}

static BOOL BSubwayRecords_StateWaitFadeOut(BSubwayRecordsWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->state = STATE_FADE_IN;
        return FALSE;
    }
    return TRUE;
}

static void BSubwayRecords_CreateCursor(BSubwayRecordsWork *wk) {
    wk->cursor = CursorMove_Create(sBSubwayRecordsCursorData, &sBSubwayRecordsCursorCallbacks, wk, TRUE, 0,
                                   HEAPID_BSUBWAY_RECORDS);
    BSubwayRecords_MoveCursorActor(wk, 0);
}

static void BSubwayRecords_FreeCursor(BSubwayRecordsWork *wk) {
    CursorMove_Delete(wk->cursor);
}

static void BSubwayRecords_OnCursorOn(void *work, int pos, int prevPos) {
}

static void BSubwayRecords_OnCursorOff(void *work, int pos, int prevPos) {
}

static void BSubwayRecords_OnCursorMove(void *work, int pos, int prevPos) {
    BSubwayRecordsWork *wk = work;

    BSubwayRecords_ShowRecord(wk, wk->records, wk->page * RECORDS_PER_PAGE + pos);
    BSubwayRecords_MoveCursorActor(wk, pos);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void BSubwayRecords_OnCursorTouch(void *work, int pos, int prevPos) {
    BSubwayRecordsWork *wk = work;

    BSubwayRecords_ShowRecord(wk, wk->records, wk->page * RECORDS_PER_PAGE + pos);
    BSubwayRecords_MoveCursorActor(wk, pos);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static void BSubwayRecords_MoveCursorActor(BSubwayRecordsWork *wk, int pos) {
    const CursorMoveData *data = CursorMove_GetData(wk->cursor, pos);
    ClActorPos actorPos;

    actorPos.x = data->x;
    actorPos.y = data->y;
    func_0204c140(wk->actors[ACTOR_CURSOR], &actorPos, 0);
}

// Shows a record's name, place, message and numbers in the windows under the list, or clears them for a position past
// the last record
static void BSubwayRecords_ShowRecord(BSubwayRecordsWork *wk, BSubwayRecord *records, int index) {
    u32 country = records[index].country;
    u32 area = records[index].area;
    PMSData *message = &records[index].message;
    StrBuf *name = GFL_StrBufCreate(0x10, HEAPID_BSUBWAY_RECORDS);

    WordSetNumber(wk->wordSet, 0, wk->param->rank, 2, 0, TRUE);
    WordSetNumber(wk->wordSet, 1, wk->param->trainNumber, 3, 0, TRUE);
    loadCountryToStrbuf(wk->wordSet, 2, country);
    loadCountryAreaToStrbuf(wk->wordSet, 3, country, area);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[INFO_WINDOW_RANK]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[INFO_WINDOW_TRAIN_NUMBER]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[INFO_WINDOW_NAME]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[INFO_WINDOW_LOCATION]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[INFO_WINDOW_QUOTE]), 0);
    if (index < wk->count) {
        u16 color = PRINT_COLOR(15, 14, 0);
        StrBuf *text;

        GFL_StrBufLoadFixedString(name, records[index].name, 8);
        PrintWindow_Print(&wk->printWindows[INFO_WINDOW_NAME], wk->printQueue, 0, 0, name, wk->font, color);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->messages[Title6_Text_Rank]);
        PrintWindow_Print(&wk->printWindows[INFO_WINDOW_RANK], wk->printQueue, 0, 0, wk->strbuf, wk->font, color);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->messages[Title6_Text_TrainNumber]);
        PrintWindow_Print(&wk->printWindows[INFO_WINDOW_TRAIN_NUMBER], wk->printQueue, 0, 0, wk->strbuf, wk->font,
                          color);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->messages[Title6_Text_Empty]);
        PrintWindow_Print(&wk->printWindows[INFO_WINDOW_LOCATION], wk->printQueue, 0, 0, wk->strbuf, wk->font, color);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->messages[Title6_Text_Empty_2]);
        PrintWindow_Print(&wk->printWindows[INFO_WINDOW_LOCATION], wk->printQueue, 0, 16, wk->strbuf, wk->font, color);
        text = PMSData_ToString(message, HEAPID_BSUBWAY_RECORDS);
        PrintWindow_Print(&wk->printWindows[INFO_WINDOW_QUOTE], wk->printQueue, 0, 0, text, wk->font, color);
        GFL_StrBufFree(text);
    } else {
        BmpWin_Transfer(wk->windows[INFO_WINDOW_NAME]);
        BmpWin_Transfer(wk->windows[INFO_WINDOW_LOCATION]);
        BmpWin_Transfer(wk->windows[INFO_WINDOW_QUOTE]);
    }
    GFL_StrBufFree(name);
}

static int BSubwayRecords_GetInput(BSubwayRecordsWork *wk) {
    int input = func_0203da0c(sBSubwayRecordsTouchRects);

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        input = INPUT_BACK;
    }
    BSubwayRecords_HandleInput(wk, input);
    return input;
}

static void BSubwayRecords_HandleInput(BSubwayRecordsWork *wk, int input) {
    switch (input) {
    case INPUT_PREV_PAGE:
        if (BSubwayRecords_StepPage(wk, input)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0204c488(wk->actors[ACTOR_ARROW_LEFT], 12);
            BSubwayRecords_ShowPage(wk, wk->page);
        }
        break;
    case INPUT_NEXT_PAGE:
        if (BSubwayRecords_StepPage(wk, input)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0204c488(wk->actors[ACTOR_ARROW_RIGHT], 13);
            BSubwayRecords_ShowPage(wk, wk->page);
        }
        break;
    case INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->state = STATE_WAIT_BACK;
        func_0204c488(wk->actors[ACTOR_BACK], 9);
        break;
    }
}

static void BSubwayRecords_InitPages(BSubwayRecordsWork *wk) {
    wk->page = 0;
    wk->pageCount = wk->count / RECORDS_PER_PAGE;
    if (wk->count % RECORDS_PER_PAGE != 0) {
        wk->pageCount++;
    }
}

// Shows the page's arrows when there is a page to go to
static void BSubwayRecords_UpdateArrows(BSubwayRecordsWork *wk, int page, int pageCount) {
    if (pageCount == 1) {
        func_0204c124(wk->actors[ACTOR_ARROW_LEFT], FALSE);
        func_0204c124(wk->actors[ACTOR_ARROW_RIGHT], FALSE);
        return;
    }
    if (page == 0) {
        func_0204c124(wk->actors[ACTOR_ARROW_LEFT], FALSE);
        func_0204c124(wk->actors[ACTOR_ARROW_RIGHT], TRUE);
    } else if (page != 0 && page < pageCount - 1) {
        func_0204c124(wk->actors[ACTOR_ARROW_LEFT], TRUE);
        func_0204c124(wk->actors[ACTOR_ARROW_RIGHT], TRUE);
    } else if (page == pageCount - 1) {
        func_0204c124(wk->actors[ACTOR_ARROW_LEFT], TRUE);
        func_0204c124(wk->actors[ACTOR_ARROW_RIGHT], FALSE);
    }
}

// Turns the page back or forward if there is one, and returns whether it did
static BOOL BSubwayRecords_StepPage(BSubwayRecordsWork *wk, int direction) {
    BOOL moved = FALSE;

    if (direction == PAGE_PREV) {
        if (wk->page != 0) {
            wk->page--;
            moved = TRUE;
        }
    } else if (direction == PAGE_NEXT) {
        if (wk->page < wk->pageCount - 1) {
            wk->page++;
            moved = TRUE;
        }
    }
    return moved;
}

static void BSubwayRecords_ShowPage(BSubwayRecordsWork *wk, int page) {
    wk->page = page;
    BSubwayRecords_UpdateArrows(wk, page, wk->pageCount);
    BSubwayRecords_ShowFrames(wk);
    BSubwayRecords_ShowRecord(wk, wk->records, wk->page * RECORDS_PER_PAGE + CursorMove_GetPos(wk->cursor));
}

// The number of records on a page, which is ten but for the last page
static int BSubwayRecords_GetPageSize(int page, int pageCount, int count) {
    int size = count - page * RECORDS_PER_PAGE;

    if (size > RECORDS_PER_PAGE) {
        size = RECORDS_PER_PAGE;
    }
    return size;
}

static void BSubwayRecords_ShowFrames(BSubwayRecordsWork *wk) {
    StrBuf *name = GFL_StrBufCreate(0x10, HEAPID_BSUBWAY_RECORDS);
    int size = BSubwayRecords_GetPageSize(wk->page, wk->pageCount, wk->count);
    int i;

    for (i = 0; i < size; i++) {
        GFL_StrBufLoadFixedString(name, wk->records[wk->page * RECORDS_PER_PAGE + i].name, 8);
        BSubwayRecords_DrawFrame(wk, i, name, wk->records[wk->page * RECORDS_PER_PAGE + i].female);
    }
    for (; i < RECORDS_PER_PAGE; i++) {
        BGWinFrame_Hide(wk->frames, i);
    }
    GFL_StrBufFree(name);
}

static void BSubwayRecords_DrawFrame(BSubwayRecordsWork *wk, int index, StrBuf *name, BOOL female) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[index]), 4);
    if (female == FALSE) {
        GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[index]), 0, 0, name, wk->font,
                                       PRINT_COLOR(13, 12, 4));
    } else {
        GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[index]), 0, 0, name, wk->font,
                                       PRINT_COLOR(11, 10, 4));
    }
    BmpWin_FlushChar(wk->windows[index]);
    BGWinFrame_WriteBmpWin(wk->frames, index, wk->windows[index]);
    BGWinFrame_Show(wk->frames, index);
}
