#include "types.h"
#include "app/comm_tvt_field.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/field_sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_whpipe.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "save/wifi_list.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/ctvt_beacon.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The Xtransceiver's screen in the field: the player picks to call a friend, to answer one that calls or to quit, and
// the screen leaves to the video chat of overlay 257. The ROM has no string for the file, so the name is a guess.
// Function names are ours

#define WINDOW_COUNT 11
#define CALLER_COUNT 16
#define ACTOR_COUNT 6
#define PALETTE_COUNT 7
#define TEXT_COLOR 0x39e3

// The frames a call lasts without a new beacon of its caller
#define CALLER_FRAMES 600

// The heap of the screen
#define HEAPID_COMM_TVT_FIELD 0x1f

// Where a window goes: its position and size in tiles
typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} CommTvtFieldWindowParam;

// A call in the neighbourhood, from the beacon of its caller
typedef struct {
    u8 mac[6];
    u8 unk6[2];
    // The frames until the call is forgotten
    u16 timer;
    // How many children the caller's beacon says it has
    u16 count;
} CommTvtFieldCaller;

typedef struct CommTvtFieldWork CommTvtFieldWork;
typedef void (*CommTvtFieldFunc)(CommTvtFieldWork *wk);
typedef BOOL (*CommTvtFieldButtonFunc)(u32 button, CommTvtFieldWork *wk);

struct CommTvtFieldWork {
    // The function that runs each frame, none to leave
    CommTvtFieldFunc func;
    CommTvtFieldButtonFunc buttonFunc;
    // What the screen leaves to do: 0 to wait, 1 to start the video chat
    int mode;
    HeapID heapId;
    BmpWin *windows[WINDOW_COUNT];
    BmpWin *nameWindow;
    ButtonMan *buttonMan;
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    StrBuf *strbuf50;
    StrBuf *strbuf54;
    StrBuf *strbuf58;
    u32 frameChars;
    u32 backgroundChars;
    CommTvtFieldCaller callers[CALLER_COUNT];
    // The status of the calls that the screen shows, and the number of the caller it shows
    u32 shownState;
    u32 shownCaller;
    u16 palettes[PALETTE_COUNT][16];
    u16 adjustedPalettes[PALETTE_COUNT][16];
    u8 unk2EC[0x20];
    ClActUnit *unit;
    TCB *vblankTask;
    BmpWin *unk314;
    PrintStream *stream;
    TCBExManager *tcbManager;
    PrintQueue *queue;
    AppTaskMenu *taskMenu;
    StrBuf *unk328;
    u16 unk32C;
    int unk330;
    u8 unk334[0xc];
    AppTaskMenuRes *taskMenuRes;
    CommTvtFieldParam *param;
    int windowCount;
    u8 unk34C[4];
    GameData *gameData;
    GameSystem *gsys;
    int scroll;
    int selected;
    u8 unk360[4];
    u32 wifiState;
    u32 prevWifiState;
    u32 callCount;
    u32 prevCallCount;
    int unk374;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u8 unk384[8];
    BOOL leaving;
    ClActor *actors[ACTOR_COUNT];
};

static BOOL CommTvtFieldApp_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CommTvtFieldApp_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CommTvtFieldApp_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void CommTvtField_SetFunc(CommTvtFieldWork *wk, CommTvtFieldFunc func);
static void CommTvtField_SetFuncLine(CommTvtFieldWork *wk, CommTvtFieldFunc func, int line);
static void CommTvtField_InitBG(CommTvtFieldWork *wk);
static void CommTvtField_CreateWindows(int count, const u32 *msgIds, CommTvtFieldWork *wk,
                                       const CommTvtFieldWindowParam *params, const int *centersX, const int *centersY);
static void CommTvtField_CreateWindow(u32 index, CommTvtFieldWork *wk, const CommTvtFieldWindowParam *param,
                                      const int *centersX, const int *centersY);
static void CommTvtField_FreeWindow5(u32 index, CommTvtFieldWork *wk);
static void CommTvtField_FreeWindow7(u32 index, CommTvtFieldWork *wk);
static void CommTvtField_FreeWindows(CommTvtFieldWork *wk);
static void CommTvtField_ButtonCallback(u32 button, u32 event, void *work);
static void CommTvtField_AddPaletteOffsets(u16 (*palettes)[16], const u16 *offsets, int count);
static void CommTvtField_InitActors(CommTvtFieldWork *wk);
static void CommTvtField_InitGraphics(CommTvtFieldWork *wk);
static void CommTvtField_InitUI(CommTvtFieldWork *wk);
static void CommTvtField_FreeGraphics(CommTvtFieldWork *wk);
static void CommTvtField_WaitExitWipe(CommTvtFieldWork *wk);
static void CommTvtField_StartExit(CommTvtFieldWork *wk);
static void CommTvtField_QueueStartExit(CommTvtFieldWork *wk);
static void CommTvtField_Idle(CommTvtFieldWork *wk);
static void CommTvtField_StartPress(CommTvtFieldWork *wk);
static BOOL CommTvtField_OnButton(u32 button, CommTvtFieldWork *wk);
static void CommTvtField_DrawWindowText(u32 index, u32 msgId, BOOL redraw, BOOL format, CommTvtFieldWork *wk);
static void CommTvtField_UpdateStatus(CommTvtFieldWork *wk);
static void CommTvtField_Run(CommTvtFieldWork *wk);
static void CommTvtField_InitMessage(CommTvtFieldWork *wk);
static void CommTvtField_AnimateIcon(CommTvtFieldWork *wk);
static void CommTvtField_SetAnswerMode(CommTvtFieldWork *wk, int index);
static void CommTvtField_SetCallMode(CommTvtFieldWork *wk);
static void CommTvtField_UpdateCalls(CommTvtFieldWork *wk);
static void CommTvtField_OnBeacon(void *arg, const void *beacon, int gameCommandBase, u8 childCount);
static void CommTvtField_TickCallers(CommTvtFieldWork *wk);
static u32 CommTvtField_CountCalls(CommTvtFieldWork *wk);
static void CommTvtField_VBlank(TCB *tcb, void *data);

static const int sTitleCenterX[1] = { 40 };

static const int sButtonCenterX[2] = { 40, 40 };

static const int sButtonCenterY[2] = { 16, 16 };

static CommTvtFieldWindowParam sMessageWindow = { 5, 5, 10, 4 };

static CommTvtFieldWindowParam sNoticeWindow = { 9, 10, 10, 2 };

static CommTvtFieldWindowParam sCallWindow = { 5, 5, 10, 4 };

static BGSysVRAMConfig sCommTvtFieldVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The colors of the palettes that the screen brightens
static u16 sPaletteOffsets[16] = { 0, 5, 15, 15, 15, 15, 15, 5, 5, 0, 0, 0, 0, 15, 15, 15 };

static CommTvtFieldWindowParam sButtonWindows[2] = { { 5, 5, 10, 4 }, { 17, 5, 11, 4 } };

// The windows of the status texts, from index 4
static CommTvtFieldWindowParam sStatusWindows[10] = {
    { 0, 0, 0, 0 },   { 0, 0, 0, 0 },   { 0, 0, 0, 0 },   { 0, 0, 0, 0 },   { 2, 15, 16, 2 },
    { 18, 15, 8, 2 }, { 2, 18, 16, 2 }, { 18, 18, 8, 2 }, { 2, 21, 16, 2 }, { 18, 21, 8, 2 },
};

static void CommTvtField_SetFunc(CommTvtFieldWork *wk, CommTvtFieldFunc func) {
    wk->func = func;
}

static void CommTvtField_SetFuncLine(CommTvtFieldWork *wk, CommTvtFieldFunc func, int line) {
    CommTvtField_SetFunc(wk, func);
}

static void CommTvtField_InitBG(CommTvtFieldWork *wk) {
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0x6800),
                          GX_BG_CHARBASE(0),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(0);
        GFL_BGSysLoadScr(0);
        GFL_BGSysSetBGPriority(0, 0);
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          3,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysLoadScr(4);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x8000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, 17);
        GFL_BGSysLoadScr(5);
        GFL_BGSysSetBGEnabled(5, TRUE);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe000),
                          GX_BG_CHARBASE(0),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe800),
                          GX_BG_CHARBASE(0x8000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysLoadScr(7);
    }
}

static void CommTvtField_CreateWindows(int count, const u32 *msgIds, CommTvtFieldWork *wk,
                                       const CommTvtFieldWindowParam *params, const int *centersX,
                                       const int *centersY) {
    int i;
    int j;

    wk->windowCount = count;
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x180, 0x20, wk->heapId);
    for (i = 0; i < WINDOW_COUNT; i++) {
        if (wk->windows[i] != NULL) {
            BmpWin_ClearScreen(wk->windows[i]);
            BmpWin_ClearFrame(wk->windows[i], 2);
            BmpWin_Free(wk->windows[i]);
        }
        wk->windows[i] = NULL;
    }
    for (j = 0; j < count; j++) {
        s32 x;
        s32 y;

        wk->windows[j] =
            BmpWin_CreateDynamic(7, params[j].x, params[j].y, params[j].width, params[j].height, 13, FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[j]), 0);
        GFL_MsgDataLoadStrbuf(wk->msgData, msgIds[j], wk->strbuf54);
        if (centersX != NULL) {
            x = centersX[j] - GFL_FontGetBlockWidth(wk->strbuf54, wk->font, 0) / 2;
        } else {
            x = 0;
        }
        if (centersY != NULL) {
            y = centersY[j] - GFL_FontGetBlockHeight(wk->strbuf54, wk->font) / 2;
        } else {
            y = 0;
        }
        GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[j]), x, y, wk->strbuf54, wk->font, TEXT_COLOR);
        BmpWin_FlushChar(wk->windows[j]);
        BmpWin_FlushMap(wk->windows[j]);
    }
    if (wk->buttonMan != NULL) {
        GFL_BMN_Delete(wk->buttonMan);
    }
    wk->buttonMan = NULL;
    GFL_BGSysQueueScrLoad(7);
}

static void CommTvtField_CreateWindow(u32 index, CommTvtFieldWork *wk, const CommTvtFieldWindowParam *param,
                                      const int *centersX, const int *centersY) {
    s32 x;
    s32 y;

    if (wk->windows[index] != NULL) {
        BmpWin_ClearScreen(wk->windows[index]);
        BmpWin_ClearFrame(wk->windows[index], 2);
        BmpWin_Free(wk->windows[index]);
        wk->windows[index] = NULL;
    }
    x = 0;
    wk->windows[index] = BmpWin_CreateDynamic(7, param->x, param->y, param->width, param->height, 13, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[index]), 0);
    if (centersX != NULL) {
        x = centersX[index] - GFL_FontGetBlockWidth(wk->strbuf54, wk->font, x) / 2;
    }
    if (centersY != NULL) {
        y = centersY[index] - GFL_FontGetBlockHeight(wk->strbuf54, wk->font) / 2;
    } else {
        y = 0;
    }
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[index]), x, y, wk->strbuf54, wk->font, TEXT_COLOR);
    BmpWin_FlushChar(wk->windows[index]);
    BmpWin_FlushMap(wk->windows[index]);
    GFL_BGSysQueueScrLoad(7);
}

static void CommTvtField_FreeWindow5(u32 index, CommTvtFieldWork *wk) {
    if (wk->windows[index] != NULL) {
        BmpWin_ClearScreen(wk->windows[index]);
        BmpWin_ClearFrame(wk->windows[index], 2);
        BmpWin_Free(wk->windows[index]);
    }
    wk->windows[index] = NULL;
    GFL_BGSysQueueScrLoad(5);
}

static void CommTvtField_FreeWindow7(u32 index, CommTvtFieldWork *wk) {
    if (wk->windows[index] != NULL) {
        BmpWin_ClearScreen(wk->windows[index]);
        BmpWin_ClearFrame(wk->windows[index], 2);
        BmpWin_Free(wk->windows[index]);
    }
    wk->windows[index] = NULL;
    GFL_BGSysQueueScrLoad(7);
}

static void CommTvtField_FreeWindows(CommTvtFieldWork *wk) {
    int i;

    if (wk->buttonMan != NULL) {
        GFL_BMN_Delete(wk->buttonMan);
    }
    wk->buttonMan = NULL;
    for (i = 0; i < WINDOW_COUNT; i++) {
        if (wk->windows[i] != NULL) {
            BmpWin_ClearScreen(wk->windows[i]);
            GFL_BGSysQueueScrLoad(5);
            BmpWin_ClearFrame(wk->windows[i], 2);
            BmpWin_Free(wk->windows[i]);
        }
        wk->windows[i] = NULL;
    }
    wk->windowCount = 0;
}

static void CommTvtField_ButtonCallback(u32 button, u32 event, void *work) {
    CommTvtFieldWork *wk = work;

    if (event == BMN_EVENT_TOUCH && wk->buttonFunc != NULL) {
        if (!wk->buttonFunc(button, wk)) {
            return;
        }
    }
}

static void CommTvtField_AddPaletteOffsets(u16 (*palettes)[16], const u16 *offsets, int count) {
    int i;
    int j;

    for (i = 0; i < count; i++) {
        for (j = 0; j < 16; j++) {
            u16 color = palettes[i][j];
            u16 r = color & GX_RGB_R_MASK;
            u16 g = (color & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
            u16 b = (color & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;

            r += offsets[j];
            g += offsets[j];
            b += offsets[j];
            if (r > 31) {
                r = 31;
            }
            if (g > 31) {
                g = 31;
            }
            if (b > 31) {
                b = 31;
            }
            palettes[i][j] = r + (g << GX_RGB_G_SHIFT) + (b << GX_RGB_B_SHIFT);
        }
    }
}

static void CommTvtField_InitActors(CommTvtFieldWork *wk) {
    u8 xs[3] = { 0x80, 0x80, 0xe0 };
    u8 ys[3] = { 0x60, 0x60, 0xb1 };
    u8 sequences[3] = { 12, 11, 0 };
    ClActorSetup setup;
    int i;

    for (i = 0; i < 3; i++) {
        setup.x = xs[i];
        setup.y = ys[i];
        setup.sequence = sequences[i];
        setup.priority = 1;
        setup.bgPriority = 2;
        wk->actors[i] =
            func_0204c040(wk->unit, wk->chars, wk->palette, wk->cellAnims, &setup, CLACT_SURFACE_SUB, wk->heapId);
        func_0204c520(wk->actors[i], FALSE);
        func_0204c124(wk->actors[i], TRUE);
        func_0204c318(wk->actors[i], TRUE);
    }
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 15;
    setup.priority = 0;
    setup.bgPriority = 2;
    wk->actors[3] =
        func_0204c040(wk->unit, wk->chars, wk->palette, wk->cellAnims, &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c520(wk->actors[3], TRUE);
    func_0204c124(wk->actors[3], TRUE);
    func_0204c318(wk->actors[3], TRUE);
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 18;
    setup.priority = 0;
    setup.bgPriority = 2;
    wk->actors[4] =
        func_0204c040(wk->unit, wk->chars, wk->palette, wk->cellAnims, &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c520(wk->actors[4], TRUE);
    func_0204c124(wk->actors[4], FALSE);
    func_0204c318(wk->actors[4], TRUE);
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 25;
    setup.priority = 0;
    setup.bgPriority = 2;
    wk->actors[5] =
        func_0204c040(wk->unit, wk->chars, wk->palette, wk->cellAnims, &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c520(wk->actors[5], TRUE);
    func_0204c124(wk->actors[5], FALSE);
    func_0204c318(wk->actors[5], TRUE);
}

static void CommTvtField_InitGraphics(CommTvtFieldWork *wk) {
    ArcTool *arc;
    NNSG2dPaletteData *palette;
    void *file;

    wk->strbuf54 = GFL_StrBufCreate(200, wk->heapId);
    wk->strbuf58 = GFL_StrBufCreate(200, wk->heapId);
    wk->strbuf50 = GFL_StrBufCreate(200, wk->heapId);
    wk->nameWindow = BmpWin_CreateDynamic(5, 16, 10, 10, 2, 13, FALSE);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, 26, wk->heapId);
    arc = GFL_ArcSysCreateFileHandle(29, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 0, wk->heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 1, &palette, wk->heapId);
    sys_memcpy(palette->rawData, wk->palettes, sizeof(wk->palettes));
    sys_memcpy(palette->rawData, wk->adjustedPalettes, sizeof(wk->palettes));
    CommTvtField_AddPaletteOffsets(wk->adjustedPalettes, sPaletteOffsets, PALETTE_COUNT);
    GFL_HeapFree(file);
    wk->backgroundChars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 4, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 9, 4, 0, (u16)wk->backgroundChars, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 15, 6, 0, (u16)wk->backgroundChars, 0, FALSE, wk->heapId);
    wk->chars = func_0204b81c(arc, 5, FALSE, CLACT_VRAM_SUB, wk->heapId);
    wk->palette = func_0204bbb8(arc, 3, CLACT_VRAM_SUB, 0, 0, 10, wk->heapId);
    wk->cellAnims = func_0204bde0(arc, 7, 16, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->frameChars = LoadCursorImageEndOfHeap(5, 11, 0, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x180, 0x20, wk->heapId);
    CommTvtField_InitActors(wk);
}

static const int sTitleCenterY[1] = { 16 };

static const TouchRect sCommTvtFieldTouchRects[6] = {
    { 32, 80, 24, 128 }, { 32, 80, 128, 232 },  { 160, 192, 192, 255 },
    { 80, 104, 8, 64 },  { 80, 104, 192, 248 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static void CommTvtField_InitUI(CommTvtFieldWork *wk) {
    u32 msgIds[2] = { 0, 1 };

    CommTvtField_CreateWindows(2, msgIds, wk, sButtonWindows, sButtonCenterX, sButtonCenterY);
    wk->buttonMan = GFL_BMN_Create(sCommTvtFieldTouchRects, CommTvtField_ButtonCallback, wk, wk->heapId);
    wk->buttonFunc = CommTvtField_OnButton;
    CommTvtField_InitMessage(wk);
    CommTvtField_SetFuncLine(wk, CommTvtField_Run, 795);
}

static void CommTvtField_FreeGraphics(CommTvtFieldWork *wk) {
    func_020232d8();
    CommTvtField_FreeWindows(wk);
    GFL_BGSysFreeFilledChar(5, 1, 0);
    GFL_BGSysFreeCharMemory(5, (u16)wk->frameChars, (u16)(wk->frameChars >> 16));
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
    GFL_StrBufFree(wk->strbuf54);
    GFL_StrBufFree(wk->strbuf50);
    GFL_StrBufFree(wk->strbuf58);
    BmpWin_Free(wk->nameWindow);
}

static void CommTvtField_WaitExitWipe(CommTvtFieldWork *wk) {
    if (GFL_WipeIsFinished()) {
        if (wk->mode == 1) {
            if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(wk->gsys))) {
                CommTvtField_SetFuncLine(wk, NULL, 833);
            }
        } else {
            CommTvtField_SetFuncLine(wk, NULL, 837);
        }
    }
}

static void CommTvtField_StartExit(CommTvtFieldWork *wk) {
    wk->leaving = TRUE;
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
    func_ov030_02174120(NULL, NULL);
    if (wk->mode == 1) {
        if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(wk->gsys))) {
            GameCommSys_ExitReq(GSYS_GetGameCommSystem(wk->gsys));
        }
    }
    CommTvtField_SetFuncLine(wk, CommTvtField_WaitExitWipe, 862);
}

static void CommTvtField_QueueStartExit(CommTvtFieldWork *wk) {
    CommTvtField_SetFuncLine(wk, CommTvtField_StartExit, 879);
}

static void CommTvtField_Idle(CommTvtFieldWork *wk) {
}

static void CommTvtField_StartPress(CommTvtFieldWork *wk) {
    ClActorCallback callback;
    int index = wk->selected;

    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (u32)wk;
    callback.func = (NNSG2dAnmCallBackPtr)CommTvtField_QueueStartExit;
    if (index == 3) {
        index = 0;
    } else if (index == 4) {
        index = 1;
    }
    func_0204c520(wk->actors[index], TRUE);
    func_0204c5b0(wk->actors[index], &callback);
    func_0204c56c(wk->actors[index]);
    CommTvtField_SetFuncLine(wk, CommTvtField_Idle, 912);
}

static BOOL CommTvtField_OnButton(u32 button, CommTvtFieldWork *wk) {
    GameCommSys *comm = GSYS_GetGameCommSystem(wk->gsys);

    wk->selected = button;
    switch (button) {
    case 0:
    case 3:
        wk->param->exitArgument = func_ov012_02153658(comm);
        wk->mode = 0;
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        CommTvtField_SetFuncLine(wk, CommTvtField_StartPress, 964);
        break;
    case 1:
    case 4:
        wk->mode = 1;
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        CommTvtField_SetFuncLine(wk, CommTvtField_StartPress, 973);
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_70);
        wk->mode = 2;
        CommTvtField_SetFuncLine(wk, CommTvtField_StartPress, 979);
        break;
    }
    return TRUE;
}

static void CommTvtField_DrawWindowText(u32 index, u32 msgId, BOOL redraw, BOOL format, CommTvtFieldWork *wk) {
    if (wk->windows[index] != NULL && redraw == 0) {
        return;
    }
    if (wk->windows[index] == NULL) {
        wk->windows[index] = BmpWin_CreateDynamic(7, sStatusWindows[index].x, sStatusWindows[index].y,
                                                  sStatusWindows[index].width, sStatusWindows[index].height, 13, FALSE);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[index]), 0);
    if (format != 0) {
        GFL_MsgDataLoadStrbuf(wk->msgData, msgId, wk->strbuf50);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf54, wk->strbuf50);
    } else {
        GFL_MsgDataLoadStrbuf(wk->msgData, msgId, wk->strbuf54);
    }
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[index]), 0, 0, wk->strbuf54, wk->font, TEXT_COLOR);
    BmpWin_FlushChar(wk->windows[index]);
    BmpWin_FlushMap(wk->windows[index]);
}

static void CommTvtField_UpdateStatus(CommTvtFieldWork *wk) {
    BOOL changed = FALSE;
    int newIndex = 0;
    GameCommSys *comm = GSYS_GetGameCommSystem(wk->gsys);
    u32 state = func_ov012_02153658(comm);

    if (wk->unk374 == 0) {
        newIndex = 0;
        if (GameBeaconSys_GetNextNew(&newIndex) != 30) {
            wk->unk374 = 1;
        }
    }
    if (wk->callCount != wk->prevCallCount) {
        changed = TRUE;
    }
    if (wk->prevWifiState != wk->wifiState) {
        changed = TRUE;
    }
    CommTvtField_DrawWindowText(4, 2, FALSE, FALSE, wk);
    CommTvtField_DrawWindowText(6, 4, FALSE, FALSE, wk);
    CommTvtField_DrawWindowText(8, 12, FALSE, FALSE, wk);
    if (state == 0) {
        WordSetNumber(wk->wordSet, 0, wk->callCount, 2, 0, TRUE);
        CommTvtField_DrawWindowText(5, 3, changed, TRUE, wk);
        if (wk->wifiState & 0x20) {
            CommTvtField_DrawWindowText(7, 5, changed, TRUE, wk);
        } else {
            CommTvtField_DrawWindowText(7, 6, changed, TRUE, wk);
        }
        if (wk->unk374 != 0) {
            CommTvtField_DrawWindowText(9, 5, changed, TRUE, wk);
        } else {
            CommTvtField_DrawWindowText(9, 6, changed, TRUE, wk);
        }
    } else {
        CommTvtField_FreeWindow5(5, wk);
        CommTvtField_FreeWindow5(7, wk);
        CommTvtField_FreeWindow5(9, wk);
    }
    if (state == 2) {
        if (wk->shownCaller != func_ov012_0215365c(comm)) {
            wk->shownState = 0;
        }
    }
    if (wk->shownState != state) {
        switch (state) {
        case 0:
            func_0204c124(wk->actors[5], FALSE);
            CommTvtField_FreeWindow7(10, wk);
            GFL_MsgDataLoadStrbuf(wk->msgData, 0, wk->strbuf54);
            CommTvtField_CreateWindow(0, wk, &sMessageWindow, sTitleCenterX, sTitleCenterY);
            break;
        case 1:
            func_0204c124(wk->actors[5], FALSE);
            CommTvtField_FreeWindow7(10, wk);
            GFL_MsgDataLoadStrbuf(wk->msgData, 15, wk->strbuf54);
            CommTvtField_CreateWindow(0, wk, &sCallWindow, sTitleCenterX, sTitleCenterY);
            break;
        case 2: {
            u32 caller = func_ov012_0215365c(comm);

            wk->shownCaller = caller;
            func_0204c124(wk->actors[5], FALSE);
            CommTvtField_FreeWindow7(10, wk);
            WordSetNumber(wk->wordSet, 0, caller, 1, 0, TRUE);
            GFL_MsgDataLoadStrbuf(wk->msgData, 16, wk->strbuf50);
            GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf54, wk->strbuf50);
            CommTvtField_CreateWindow(0, wk, &sCallWindow, sTitleCenterX, sTitleCenterY);
            break;
        }
        case 3:
            GFL_MsgDataLoadStrbuf(wk->msgData, 17, wk->strbuf54);
            CommTvtField_CreateWindow(0, wk, &sCallWindow, sTitleCenterX, sTitleCenterY);
            if (func_ov012_02153660(comm, wk->strbuf54)) {
                CommTvtField_CreateWindow(10, wk, &sNoticeWindow, NULL, NULL);
                func_0204c124(wk->actors[5], TRUE);
            }
            break;
        }
        wk->shownState = state;
    }
    GFL_BGSysQueueScrLoad(7);
}

static void CommTvtField_Run(CommTvtFieldWork *wk) {
    if (GFL_WipeIsFinished()) {
        GFL_BMN_Main(wk->buttonMan);
    }
    CommTvtField_AnimateIcon(wk);
    CommTvtField_UpdateStatus(wk);
}

static void CommTvtField_InitMessage(CommTvtFieldWork *wk) {
    wk->unk328 = GFL_StrBufCreate(100, wk->heapId);
    GFL_MsgDataLoadStrbuf(wk->msgData, 9, wk->unk328);
    wk->unk32C = TEXT_COLOR;
    wk->unk330 = 1;
    GFL_StrBufFree(wk->unk328);
}

static void CommTvtField_AnimateIcon(CommTvtFieldWork *wk) {
    switch (func_ov012_02153658(GSYS_GetGameCommSystem(wk->gsys))) {
    case 0:
    case 1:
        if (func_0204c4a0(wk->actors[0]) == 20) {
            func_0204c520(wk->actors[0], FALSE);
            func_0204c488(wk->actors[0], 12);
        }
        break;
    case 2:
    case 3:
        if (func_0204c4a0(wk->actors[0]) == 12) {
            func_0204c520(wk->actors[0], TRUE);
            func_0204c488(wk->actors[0], 20);
        }
        break;
    }
}

static void CommTvtField_SetAnswerMode(CommTvtFieldWork *wk, int index) {
    u8 *mac;

    func_02042e84();
    mac = func_020428c8(index);
    wk->param->commParam.unk4 = 2;
    sys_memcpy(mac, wk->param->commParam.parentMac, 6);
}

static void CommTvtField_SetCallMode(CommTvtFieldWork *wk) {
    u8 mac[6] = { 0, 0, 0, 0, 0, 0 };

    func_02042e84();
    wk->param->commParam.unk4 = 1;
    sys_memcpy(mac, wk->param->commParam.parentMac, 6);
}

static void CommTvtField_UpdateCalls(CommTvtFieldWork *wk) {
    int found = func_02012e54(GameData_GetWifiList(wk->gameData));

    if (found != -1) {
        void *beacon = func_020428a8(found);
        StrBuf *name = GFL_StrBufCreate(200, wk->heapId);

        GFL_StrBufLoadFixedString(name, CtvtBeacon_GetPlayerName(beacon), 8);
        if (!func_0204c138(wk->actors[4])) {
            GFL_StrBufLoadFixedString(wk->strbuf58, CtvtBeacon_GetPlayerName(beacon), 8);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindow), 0);
            GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->nameWindow), 0, 0, wk->strbuf58, wk->font, TEXT_COLOR);
            BmpWin_FlushChar(wk->nameWindow);
            BmpWin_FlushMap(wk->nameWindow);
            func_0204c124(wk->actors[4], TRUE);
            GFL_BGSysQueueScrLoad(5);
            CommTvtField_SetAnswerMode(wk, found);
        }
        GFL_StrBufFree(name);
        FieldSnd_RingRingtone(GameData_GetFieldSoundSystem(wk->gameData));
        if (func_0204c4a0(wk->actors[3]) == 15) {
            func_0204c488(wk->actors[3], 16);
        }
        if (func_0204c4a0(wk->actors[1]) == 11) {
            func_0204c520(wk->actors[1], TRUE);
            func_0204c488(wk->actors[1], 19);
        }
    } else {
        FieldSnd_StopRingtone(GameData_GetFieldSoundSystem(wk->gameData));
        func_0204c124(wk->actors[4], FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindow), 0);
        BmpWin_FlushChar(wk->nameWindow);
        CommTvtField_SetCallMode(wk);
        if (func_0204c4a0(wk->actors[1]) == 19) {
            func_0204c520(wk->actors[1], FALSE);
            func_0204c488(wk->actors[1], 11);
        }
        if (func_0204c4a0(wk->actors[3]) == 16) {
            func_0204c488(wk->actors[3], 15);
        }
    }
}

static void CommTvtField_OnBeacon(void *arg, const void *beacon, int gameCommandBase, u8 childCount) {
    CommTvtFieldWork *wk = arg;
    int i;
    int j;

    if (gameCommandBase >= 20 && gameCommandBase <= 27) {
        for (i = 0; i < CALLER_COUNT; i++) {
            if (wk->callers[i].timer != 0 && GFL_STD_MemCmp(wk->callers[i].mac, (const u8 *)beacon + 4, 6) == 0) {
                wk->callers[i].timer = CALLER_FRAMES;
                wk->callers[i].count = childCount;
                return;
            }
        }
        for (j = 0; j < CALLER_COUNT; j++) {
            if (wk->callers[j].timer == 0) {
                sys_memcpy((const u8 *)beacon + 4, wk->callers[j].mac, 6);
                wk->callers[j].timer = CALLER_FRAMES;
                wk->callers[j].count = childCount;
                return;
            }
        }
    }
}

static void CommTvtField_TickCallers(CommTvtFieldWork *wk) {
    int i;

    for (i = 0; i < CALLER_COUNT; i++) {
        if (wk->callers[i].timer != 0) {
            wk->callers[i].timer--;
        }
    }
}

static u32 CommTvtField_CountCalls(CommTvtFieldWork *wk) {
    int i;
    u32 total = 0;

    for (i = 0; i < CALLER_COUNT; i++) {
        if (wk->callers[i].timer != 0) {
            total += wk->callers[i].count;
        }
    }
    return total;
}

static void CommTvtField_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
}

static const ClActSysSetup sCommTvtFieldClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 100, 100, 100, 100, 16, 16 };

const GameProcFunctions COMM_TVT_FIELD_PROC_FUNCTIONS = {
    CommTvtFieldApp_Init,
    CommTvtFieldApp_Main,
    CommTvtFieldApp_Exit,
};

static BOOL CommTvtFieldApp_Init(GameProc *proc, u32 *state, void *param, void *work) {
    CommTvtFieldWork *wk;
    CommTvtFieldParam *fieldParam = param;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_COMM_TVT_FIELD, 0x18000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(CommTvtFieldWork), HEAPID_COMM_TVT_FIELD);
    sys_memset(wk, 0, sizeof(CommTvtFieldWork));
    wk->heapId = HEAPID_COMM_TVT_FIELD;
    wk->gameData = fieldParam->gameData;
    wk->gsys = fieldParam->gsys;
    GFL_BGSysSetDisplayLayout(1);
    GXS_DispOn();
    gfxEngineEnableA();
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    func_020232d0();
    GFL_BGSysSetVRAMBanks(&sCommTvtFieldVRAMConfig);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    ClActSys_Create(&sCommTvtFieldClActSetup, &sCommTvtFieldVRAMConfig, wk->heapId);
    wk->unit = func_0204bf1c(40, 0, wk->heapId);
    wk->vblankTask = GFL_VBlankTCBAdd(CommTvtField_VBlank, wk, 0);
    CommTvtField_InitBG(wk);
    CommTvtField_InitGraphics(wk);
    wk->wordSet = GFL_WordSetSystemCreate(11, 200, wk->heapId);
    wk->tcbManager = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 1, 0);
    wk->queue = func_02021998(wk->heapId);
    wk->taskMenuRes = AppTaskMenuRes_Create(5, 13, wk->font, wk->queue, wk->heapId);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG0, 15, 4);
    GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_ALL);
    CommTvtField_SetFuncLine(wk, CommTvtField_InitUI, 1739);
    wk->param = fieldParam;
    func_02042e84();
    wk->param->commParam.unk4 = 1;
    wk->param->commParam.gameData = wk->gameData;
    func_02042ba8(FALSE, wk->heapId);
    if (func_02042788()) {
        // The callback takes the command as an int; net_whpipe.h says u8
        func_ov030_02174120((void (*)(void *, const void *, u8, u8))CommTvtField_OnBeacon, wk);
    }
    func_0203d564(TRUE);
    return TRUE;
}

static BOOL CommTvtFieldApp_Main(GameProc *proc, u32 *state, void *param, void *work) {
    CommTvtFieldWork *wk = work;
    BOOL result = TRUE;

    if (wk->func != NULL) {
        wk->func(wk);
        result = FALSE;
    }
    if (wk->taskMenu != NULL) {
        AppTaskMenu_Update(wk->taskMenu);
    }
    if (wk->mode != 1) {
        CommTvtField_UpdateCalls(wk);
    }
    CommTvtField_TickCallers(wk);
    func_0204b794();
    if (func_02042788()) {
        wk->prevWifiState = wk->wifiState;
        wk->wifiState = func_02012be4(GameData_GetWifiList(wk->gameData));
        wk->prevCallCount = wk->callCount;
        wk->callCount = CommTvtField_CountCalls(wk);
    }
    GFL_TCBExMgrUpdate(wk->tcbManager);
    func_02021a3c(wk->queue);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->scroll);
    wk->scroll--;
    if (GFL_WipeIsFinished() && !wk->leaving) {
        GSYS_TryBootGameComm(wk->gsys);
    }
    if (GFL_WipeIsFinished() && func_02016bec(wk->gsys)) {
        wk->mode = 2;
        result = TRUE;
        Wipe_SetScreenCovered(0, 0);
        Wipe_SetScreenCovered(1, 0);
    }
    return result;
}

static BOOL CommTvtFieldApp_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    CommTvtFieldWork *wk = work;
    CommTvtFieldParam *fieldParam = param;
    int i;

    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    if (func_02042788()) {
        func_ov030_02174120(NULL, NULL);
    }
    CommTvtField_FreeGraphics(wk);
    fieldParam->mode = wk->mode;
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_BGSysReleaseBG(5);
    func_02021c44(wk->queue);
    func_02021a18(wk->queue);
    for (i = 0; i < 3; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204c108(wk->actors[3]);
    func_0204c108(wk->actors[4]);
    func_0204c108(wk->actors[5]);
    func_0204bcd0(wk->palette);
    func_0204b98c(wk->chars);
    func_0204be64(wk->cellAnims);
    GFL_TCBRemove(wk->vblankTask);
    func_0204bf98(wk->unit);
    func_0204b758();
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
    }
    GFL_WordSetSystemFree(wk->wordSet);
    if (wk->taskMenu != NULL) {
        AppTaskMenu_Free(wk->taskMenu);
        wk->taskMenu = NULL;
    }
    if (wk->unk314 != NULL) {
        BmpWin_Free(wk->unk314);
    }
    AppTaskMenuRes_Free(wk->taskMenuRes);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_COMM_TVT_FIELD);
    return TRUE;
}
