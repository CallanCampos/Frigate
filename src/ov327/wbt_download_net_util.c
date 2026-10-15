#include "types.h"
#include "app/net_common_download.h"
#include "app/wbt_download_net_graphic.h"
#include "app/wbt_download_net_util.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The process that downloads the tournaments: it shows messages over the animated background of the Dream World
// account screens while overlay 196's NetCommonDownload receives the file, then checks the file. The name is the ROM's
// string, from GFL_HeapAllocate's call. Function names are ours

// The tournaments in the file, and the size of the file
#define WBT_DOWNLOAD_TOURNAMENT_COUNT 12
#define WBT_DOWNLOAD_FILE_SIZE 0xd8f4
#define WBT_DOWNLOAD_BUFFER_SIZE 0xe000

// The messages of text bank 404
#define WBT_DOWNLOAD_MSG_CONNECTING 0xd
#define WBT_DOWNLOAD_MSG_RECEIVED 0x16
#define WBT_DOWNLOAD_MSG_INVALID 0x24
#define WBT_DOWNLOAD_MSG_TIMEOUT 0x26
#define WBT_DOWNLOAD_MSG_NO_FILE 0x25

// How WbtDownloadMessage_Print prints a message
enum {
    // At once, through the print queue
    WBT_DOWNLOAD_MSG_PRINT,
    // A character at a time, with a key cursor that A and B and the touch screen continue
    WBT_DOWNLOAD_MSG_STREAM,
    // At once, with the wait icon over the window
    WBT_DOWNLOAD_MSG_PRINT_WAIT,
    // The same stream, that can't be sped up
    WBT_DOWNLOAD_MSG_STREAM_NO_SKIP,
};

// Where a message goes in its window
enum {
    WBT_DOWNLOAD_ALIGN_ORIGIN,
    WBT_DOWNLOAD_ALIGN_CENTER,
    WBT_DOWNLOAD_ALIGN_CENTER_Y,
    WBT_DOWNLOAD_ALIGN_RIGHT,
};

// The states of a message
enum {
    // Printed through the queue
    WBT_DOWNLOAD_MSG_STATE_QUEUE,
    // Printing as a stream
    WBT_DOWNLOAD_MSG_STATE_STREAM,
    WBT_DOWNLOAD_MSG_STATE_2,
    WBT_DOWNLOAD_MSG_STATE_3,
    WBT_DOWNLOAD_MSG_STATE_IDLE,
};

typedef struct {
    int x;
    int y;
} WbtDownloadMessagePos;

// A window that shows the messages
typedef struct {
    u32 unk0;
    Font *font;
    PrintStream *stream;
    TCBExManager *tcbManager;
    WaitIcon *waitIcon;
    BmpWin *window;
    StrBuf *strbuf;
    u16 color;
    u16 heapId;
    PrintWindow printWindow;
    PrintQueue *queue;
    u32 state;
    // Whether the message is printed
    u16 done;
    // Set when the window's frame is drawn
    u16 frameDrawn;
    KeyCursor *p_keycursor;
    AppPrintsysCommon printsys;
    WbtDownloadMessagePos origin;
    u32 align;
} WbtDownloadMessage;

typedef struct WbtDownloadStack WbtDownloadStack;

// A step of the stack, which gets the stack, its state and the user data
typedef void (*WbtDownloadStackFunc)(WbtDownloadStack *stack, u32 *state, void *userData);

typedef struct {
    WbtDownloadStackFunc func;
    u32 state;
    u32 unk8;
} WbtDownloadStackEntry;

// The steps that the process runs one at a time: the last one pushed runs, and when it pops itself the one below it
// goes on
struct WbtDownloadStack {
    void *userData;
    int depth;
    int capacity;
    WbtDownloadStackEntry entries[1];
};

// What the Dream World account screens' palette cycle, which the background of the download copies, keeps
#define WBT_DOWNLOAD_PALETTE_COUNT 15
#define WBT_DOWNLOAD_BLEND_STEPS 4

// Palette 1's colors 4 to 10 cycle through blends of 15 palettes, a step every other frame, with a scrolling BG
typedef struct {
    TCB *task;
    BOOL active;
    u16 palettes[WBT_DOWNLOAD_PALETTE_COUNT][16];
    u16 blended[WBT_DOWNLOAD_PALETTE_COUNT * WBT_DOWNLOAD_BLEND_STEPS][16];
    s16 frame;
    u8 unk96A;
    u8 skip;
    u8 scrollBg;
    u8 unk96D[3];
} WbtDownloadPaletteCycle;

typedef struct {
    WbtDownloadPaletteCycle cycle;
    // The characters of the windows' frames on BG 1 and BG 2
    u32 frameChars1;
    u32 frameChars2;
} WbtDownloadBG;

typedef struct {
    WbtDownloadNetParam *param;
    WbtDownloadStack *stack;
    WbtDownloadGraphic *graphic;
    NetCommonDownload *download;
    WbtDownloadBG bg;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    WordSet *wordSet;
    WbtDownloadMessage *message;
} WbtDownloadNetWork;

// A tournament of the file: it is valid if it has a name, a kind for this version and its checksum
typedef struct {
    u32 kind;
    u8 unk4[3];
    u8 version;
    u16 name;
    u8 data[0x1206];
    u32 checksum;
} WbtDownloadTournament;

typedef struct {
    WbtDownloadTournament tournaments[WBT_DOWNLOAD_TOURNAMENT_COUNT];
    u32 checksum;
} WbtDownloadFile;

static BOOL WbtDownloadNet_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WbtDownloadNet_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WbtDownloadNet_Main(GameProc *proc, u32 *state, void *param, void *work);
static void WbtDownloadNet_FirstStep(WbtDownloadStack *stack, u32 *state, void *userData);
static void WbtDownloadNet_DownloadStep(WbtDownloadStack *stack, u32 *state, void *userData);
static void WbtDownloadNet_FadeInStep(WbtDownloadStack *stack, u32 *state, void *userData);
static void WbtDownloadNet_FadeOutStep(WbtDownloadStack *stack, u32 *state, void *userData);
static u32 WbtDownloadNet_SelectFile(DWCNdFileInfo *files, u32 count);
static WbtDownloadMessage *WbtDownloadMessage_Create(u16 bg, u16 x, u16 y, u16 width, u16 height, u16 palette,
                                                     Font *font, HeapID heapId);
static void WbtDownloadMessage_Free(WbtDownloadMessage *p_wk);
static void WbtDownloadMessage_Update(WbtDownloadMessage *p_wk);
static void WbtDownloadMessage_PrintMsg(WbtDownloadMessage *p_wk, MsgData *msgData, u32 msgId, u32 mode);
static void WbtDownloadMessage_SetColor(WbtDownloadMessage *p_wk, u16 color);
static void WbtDownloadMessage_Print(WbtDownloadMessage *p_wk, u32 mode);
static u16 WbtDownloadMessage_IsDone(WbtDownloadMessage *p_wk);
static void WbtDownloadMessage_StopWaitIcon(WbtDownloadMessage *p_wk);
static void WbtDownloadMessage_DrawFrame(WbtDownloadMessage *p_wk, u16 frameChar, u8 palette);
static void WbtDownloadMessage_Align(u32 align, const WbtDownloadMessagePos *origin, GFLBitmap *bitmap,
                                     const StrBuf *strbuf, Font *font, WbtDownloadMessagePos *out);
static WbtDownloadStack *WbtDownloadStack_Create(int capacity, void *userData, WbtDownloadStackFunc func,
                                                 HeapID heapId);
static void WbtDownloadStack_Free(WbtDownloadStack *stack);
static void WbtDownloadStack_Run(WbtDownloadStack *stack);
static BOOL WbtDownloadStack_IsEmpty(WbtDownloadStack *stack);
static void WbtDownloadStack_SetFunc(WbtDownloadStack *stack, WbtDownloadStackFunc func);
static void WbtDownloadStack_Push(WbtDownloadStack *stack, WbtDownloadStackFunc func);
static void WbtDownloadStack_Pop(WbtDownloadStack *stack);
static WbtDownloadStackEntry *WbtDownloadStack_GetEntry(WbtDownloadStack *stack, int index);
static void WbtDownloadNetBG_Create(WbtDownloadBG *bg, HeapID heapId);
static void WbtDownloadNetBG_Free(WbtDownloadBG *bg);
static void WbtDownloadNetBG_Update(WbtDownloadBG *bg);
static void WbtDownloadNetBG_Load(WbtDownloadBG *bg, HeapID heapId);
static void WbtDownloadNetBG_CyclePalette(TCB *task, void *work);
static void WbtDownloadNetBG_Unload(WbtDownloadBG *bg);
static u32 WbtDownloadNetBG_GetFrameChars(WbtDownloadBG *bg, u32 which);

// The background color of the message's color
static inline u8 WbtDownloadMessage_GetBgColor(const WbtDownloadMessage *p_wk) {
    return p_wk->color & 0x1f;
}

const GameProcFunctions WBT_DOWNLOAD_NET_PROC_FUNCTIONS = {
    WbtDownloadNet_Init,
    WbtDownloadNet_Main,
    WbtDownloadNet_Exit,
};

static WbtDownloadMessage *WbtDownloadMessage_Create(u16 bg, u16 x, u16 y, u16 width, u16 height, u16 palette,
                                                     Font *font, HeapID heapId) {
    WbtDownloadMessage *p_wk =
        GFL_HeapAllocate(heapId, sizeof(WbtDownloadMessage), FALSE, "wbt_download_net_util.c", 94);

    sys_memset(p_wk, 0, sizeof(WbtDownloadMessage));
    p_wk->font = font;
    p_wk->queue = func_02021998(heapId);
    p_wk->state = WBT_DOWNLOAD_MSG_STATE_IDLE;
    p_wk->heapId = heapId;
    p_wk->strbuf = GFL_StrBufCreate(0x300, heapId);
    p_wk->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    PrintWindow_Init(&p_wk->printWindow, p_wk->window);
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->window), WbtDownloadMessage_GetBgColor(p_wk));
    BmpWin_FlushChar(p_wk->window);
    BmpWin_FlushMap(p_wk->window);
    p_wk->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 0x20);
    return p_wk;
}

static void WbtDownloadMessage_Free(WbtDownloadMessage *p_wk) {
    if (p_wk->stream != NULL) {
        func_020223cc(p_wk->stream);
        p_wk->stream = NULL;
    }
    if (p_wk->waitIcon != NULL) {
        WaitIcon_Free(p_wk->waitIcon);
        p_wk->waitIcon = NULL;
    }
    if (p_wk->p_keycursor != NULL) {
        KeyCursor_Free(p_wk->p_keycursor);
    }
    GFL_TCBExMgrFree(p_wk->tcbManager);
    BmpWin_ClearFrame(p_wk->window, 2);
    BmpWin_Free(p_wk->window);
    GFL_StrBufFree(p_wk->strbuf);
    func_02021a18(p_wk->queue);
    GFL_HeapFree(p_wk);
}

static void WbtDownloadMessage_Update(WbtDownloadMessage *p_wk) {
    func_02021a3c(p_wk->queue);
    switch (p_wk->state) {
    case WBT_DOWNLOAD_MSG_STATE_2:
        PrintWindow_Flush(&p_wk->printWindow, p_wk->queue);
        break;
    case WBT_DOWNLOAD_MSG_STATE_QUEUE:
        PrintWindow_Flush(&p_wk->printWindow, p_wk->queue);
        p_wk->done = PrintWindow_IsPrinted(&p_wk->printWindow);
        break;
    case WBT_DOWNLOAD_MSG_STATE_STREAM:
    case WBT_DOWNLOAD_MSG_STATE_3:
        if (p_wk->stream != NULL) {
            if (p_wk->p_keycursor != NULL) {
                KeyCursor_Update(p_wk->p_keycursor, p_wk->stream, p_wk->window);
            }
            if (AppPrintsysCommon_Update(&p_wk->printsys, p_wk->stream)) {
                p_wk->done = TRUE;
            }
        }
        break;
    case WBT_DOWNLOAD_MSG_STATE_IDLE:
        break;
    }
    GFL_TCBExMgrUpdate(p_wk->tcbManager);
}

static void WbtDownloadMessage_PrintMsg(WbtDownloadMessage *p_wk, MsgData *msgData, u32 msgId, u32 mode) {
    GFL_MsgDataLoadStrbuf(msgData, msgId, p_wk->strbuf);
    WbtDownloadMessage_Print(p_wk, mode);
}

static void WbtDownloadMessage_SetColor(WbtDownloadMessage *p_wk, u16 color) {
    p_wk->color = color;
}

static void WbtDownloadMessage_Print(WbtDownloadMessage *p_wk, u32 mode) {
    WbtDownloadMessagePos pos;
    int wait;

    func_02021c44(p_wk->queue);
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->window), WbtDownloadMessage_GetBgColor(p_wk));
    if (p_wk->stream != NULL) {
        func_020223cc(p_wk->stream);
        p_wk->stream = NULL;
    }
    if (p_wk->p_keycursor != NULL) {
        KeyCursor_Free(p_wk->p_keycursor);
        p_wk->p_keycursor = NULL;
    }
    WbtDownloadMessage_StopWaitIcon(p_wk);
    WbtDownloadMessage_Align(p_wk->align, &p_wk->origin, BmpWin_GetBitmap(p_wk->window), p_wk->strbuf, p_wk->font,
                             &pos);
    switch (mode) {
    case WBT_DOWNLOAD_MSG_PRINT_WAIT:
        p_wk->waitIcon =
            WaitIcon_Create(GFL_VBlankGetTCBMgr(), p_wk->window, WbtDownloadMessage_GetBgColor(p_wk), 16, p_wk->heapId);
        PrintWindow_Print(&p_wk->printWindow, p_wk->queue, pos.x, pos.y, p_wk->strbuf, p_wk->font, p_wk->color);
        p_wk->state = WBT_DOWNLOAD_MSG_STATE_QUEUE;
        break;
    case WBT_DOWNLOAD_MSG_PRINT:
        PrintWindow_Print(&p_wk->printWindow, p_wk->queue, pos.x, pos.y, p_wk->strbuf, p_wk->font, p_wk->color);
        p_wk->state = WBT_DOWNLOAD_MSG_STATE_QUEUE;
        break;
    case WBT_DOWNLOAD_MSG_STREAM_NO_SKIP:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printsys, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_NO_SKIP);
        p_wk->p_keycursor = KeyCursor_Create(WbtDownloadMessage_GetBgColor(p_wk), TRUE, FALSE, p_wk->heapId);
        wait = func_02017c50(1);
        p_wk->stream = func_02022268(p_wk->window, pos.x, pos.y, p_wk->strbuf, p_wk->font, wait, p_wk->tcbManager, 0,
                                     p_wk->heapId, WbtDownloadMessage_GetBgColor(p_wk));
        p_wk->state = WBT_DOWNLOAD_MSG_STATE_STREAM;
        break;
    case WBT_DOWNLOAD_MSG_STREAM:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printsys, APP_PRINTSYS_COMMON_KEYS);
        p_wk->p_keycursor = KeyCursor_Create(WbtDownloadMessage_GetBgColor(p_wk), TRUE, FALSE, p_wk->heapId);
        wait = func_02017bcc();
        p_wk->stream = func_02022268(p_wk->window, pos.x, pos.y, p_wk->strbuf, p_wk->font, wait, p_wk->tcbManager, 0,
                                     p_wk->heapId, WbtDownloadMessage_GetBgColor(p_wk));
        p_wk->state = WBT_DOWNLOAD_MSG_STATE_STREAM;
        break;
    }
    p_wk->done = FALSE;
}

static u16 WbtDownloadMessage_IsDone(WbtDownloadMessage *p_wk) {
    return p_wk->done;
}

static void WbtDownloadMessage_StopWaitIcon(WbtDownloadMessage *p_wk) {
    p_wk->done = TRUE;
    if (p_wk->waitIcon != NULL) {
        WaitIcon_Free(p_wk->waitIcon);
        p_wk->waitIcon = NULL;
        BmpWin_FlushMap(p_wk->window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(p_wk->window));
    }
}

static void WbtDownloadMessage_DrawFrame(WbtDownloadMessage *p_wk, u16 frameChar, u8 palette) {
    BmpWin_DrawFrame(p_wk->window, 2, frameChar, palette);
    p_wk->frameDrawn = TRUE;
}

static void WbtDownloadMessage_Align(u32 align, const WbtDownloadMessagePos *origin, GFLBitmap *bitmap,
                                     const StrBuf *strbuf, Font *font, WbtDownloadMessagePos *out) {
    switch (align) {
    case WBT_DOWNLOAD_ALIGN_ORIGIN:
        *out = *origin;
        break;
    case WBT_DOWNLOAD_ALIGN_CENTER: {
        u32 x = GFL_BitmapGetWidth(bitmap) / 2;
        u32 y = GFL_BitmapGetHeight(bitmap) / 2;

        x -= GFL_FontGetBlockWidth(strbuf, font, 0) / 2;
        y -= GFL_FontGetBlockHeight(strbuf, font) / 2;

        out->x = x + origin->x;
        out->y = y + origin->y;
        break;
    }
    case WBT_DOWNLOAD_ALIGN_CENTER_Y: {
        u32 y = GFL_BitmapGetHeight(bitmap) / 2;

        y -= GFL_FontGetBlockHeight(strbuf, font) / 2;
        out->x = origin->x;
        out->y = y + origin->y;
        break;
    }
    case WBT_DOWNLOAD_ALIGN_RIGHT: {
        int x = GFL_BitmapGetWidth(bitmap) - GFL_FontGetBlockWidth(strbuf, font, 0);

        if (x < 0) {
            x = 0;
        }
        out->x = x + origin->x;
        out->y = origin->y;
        break;
    }
    }
}

static WbtDownloadStack *WbtDownloadStack_Create(int capacity, void *userData, WbtDownloadStackFunc func,
                                                 HeapID heapId) {
    u32 size = (capacity + 1) * sizeof(WbtDownloadStackEntry);
    WbtDownloadStack *stack = GFL_HeapAllocate(heapId, size, FALSE, "wbt_download_net_util.c", 1236);

    sys_memset(stack, 0, size);
    stack->userData = userData;
    stack->capacity = capacity;
    WbtDownloadStack_SetFunc(stack, func);
    return stack;
}

static void WbtDownloadStack_Free(WbtDownloadStack *stack) {
    GFL_HeapFree(stack);
}

static void WbtDownloadStack_Run(WbtDownloadStack *stack) {
    WbtDownloadStackEntry *entry = WbtDownloadStack_GetEntry(stack, stack->depth);

    if (entry->func != NULL) {
        entry->func(stack, &entry->state, stack->userData);
    }
}

static BOOL WbtDownloadStack_IsEmpty(WbtDownloadStack *stack) {
    BOOL empty = FALSE;

    if (WbtDownloadStack_GetEntry(stack, 0)->func == NULL) {
        empty = TRUE;
    }
    return empty;
}

static void WbtDownloadStack_SetFunc(WbtDownloadStack *stack, WbtDownloadStackFunc func) {
    WbtDownloadStackEntry *entry = WbtDownloadStack_GetEntry(stack, stack->depth);

    sys_memset(entry, 0, sizeof(WbtDownloadStackEntry));
    entry->func = func;
}

static void WbtDownloadStack_Push(WbtDownloadStack *stack, WbtDownloadStackFunc func) {
    stack->depth++;
    WbtDownloadStack_SetFunc(stack, func);
}

static void WbtDownloadStack_Pop(WbtDownloadStack *stack) {
    WbtDownloadStack_SetFunc(stack, NULL);
    if (stack->depth > 0) {
        stack->depth--;
    }
}

static void WbtDownloadNet_FadeInStep(WbtDownloadStack *stack, u32 *state, void *userData) {
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        WbtDownloadStack_Pop(stack);
        break;
    }
}

static void WbtDownloadNet_FadeOutStep(WbtDownloadStack *stack, u32 *state, void *userData) {
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 0, 16, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        WbtDownloadStack_Pop(stack);
        break;
    }
}

static WbtDownloadStackEntry *WbtDownloadStack_GetEntry(WbtDownloadStack *stack, int index) {
    return &stack->entries[index];
}

static BOOL WbtDownloadNet_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WbtDownloadNetWork *wk;
    NetCommonDownloadParam downloadParam;

    GFL_OvlLoad(OVERLAY_ID(196));
    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WBT_DOWNLOAD_NET, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WbtDownloadNetWork), HEAPID_WBT_DOWNLOAD_NET);
    sys_memset(wk, 0, sizeof(WbtDownloadNetWork));
    wk->param = param;
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_WBT_DOWNLOAD_NET);
    wk->queue = func_02021998(HEAPID_WBT_DOWNLOAD_NET);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0404, HEAPID_WBT_DOWNLOAD_NET);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_WBT_DOWNLOAD_NET);
    wk->graphic = WbtDownloadGraphic_Create(1, HEAPID_WBT_DOWNLOAD_NET);
    WbtDownloadNetBG_Create(&wk->bg, HEAPID_WBT_DOWNLOAD_NET);
    sys_memset(&downloadParam, 0, sizeof(NetCommonDownloadParam));
    downloadParam.mode = 0;
    downloadParam.gameData = wk->param->gameData;
    downloadParam.dataSize = WBT_DOWNLOAD_BUFFER_SIZE;
    downloadParam.heapId = HEAPID_WBT_DOWNLOAD_NET;
    wk->download = NetCommonDownload_Create(&downloadParam);
    func_02042ba8(TRUE, HEAPID_WBT_DOWNLOAD_NET);
    wk->message = WbtDownloadMessage_Create(1, 1, 0x13, 0x1e, 4, 0xc, wk->font, HEAPID_WBT_DOWNLOAD_NET);
    WbtDownloadMessage_SetColor(wk->message, PRINT_COLOR(1, 2, 15));
    WbtDownloadMessage_DrawFrame(wk->message, WbtDownloadNetBG_GetFrameChars(&wk->bg, 0), 0xd);
    WbtDownloadMessage_PrintMsg(wk->message, wk->msgData, WBT_DOWNLOAD_MSG_CONNECTING, WBT_DOWNLOAD_MSG_PRINT_WAIT);
    wk->stack = WbtDownloadStack_Create(5, wk, WbtDownloadNet_FirstStep, HEAPID_WBT_DOWNLOAD_NET);
    return TRUE;
}

static BOOL WbtDownloadNet_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WbtDownloadNetWork *wk = work;

    WbtDownloadStack_Free(wk->stack);
    WbtDownloadMessage_Free(wk->message);
    NetCommonDownload_Free(wk->download);
    WbtDownloadNetBG_Free(&wk->bg);
    WbtDownloadGraphic_Free(wk->graphic);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    func_02021a18(wk->queue);
    GFL_FontFree(wk->font);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WBT_DOWNLOAD_NET);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_OvlUnload(OVERLAY_ID(196));
    return TRUE;
}

static BOOL WbtDownloadNet_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WbtDownloadNetWork *wk = work;

    switch (NetCommonDownload_Update(wk->download)) {
    case NET_COMMON_DOWNLOAD_UPDATE_RUNNING:
    default:
        break;
    case NET_COMMON_DOWNLOAD_UPDATE_ENDED:
        NetCommonDownload_ResetUpdate(wk->download);
        return FALSE;
    case NET_COMMON_DOWNLOAD_UPDATE_ERROR:
        wk->param->result = 1;
        return TRUE;
    }
    NetCommonDownload_Start(wk->download);
    WbtDownloadStack_Run(wk->stack);
    WbtDownloadGraphic_Update(wk->graphic);
    WbtDownloadNetBG_Update(&wk->bg);
    func_02021a3c(wk->queue);
    WbtDownloadMessage_Update(wk->message);
    NetCommonDownload_Step(wk->download);
    if (WbtDownloadStack_IsEmpty(wk->stack)) {
        return TRUE;
    }
    return FALSE;
}

static void WbtDownloadNet_FirstStep(WbtDownloadStack *stack, u32 *state, void *userData) {
    switch (*state) {
    case 0:
        WbtDownloadStack_Push(stack, WbtDownloadNet_FadeInStep);
        (*state)++;
        break;
    case 1:
        WbtDownloadStack_Push(stack, WbtDownloadNet_DownloadStep);
        (*state)++;
        break;
    case 2:
        WbtDownloadStack_Push(stack, WbtDownloadNet_FadeOutStep);
        (*state)++;
        break;
    case 3:
        WbtDownloadStack_Pop(stack);
        break;
    }
}

// The versions a tournament is for
enum {
    WBT_DOWNLOAD_KIND_WHITE2,
    WBT_DOWNLOAD_KIND_BLACK2,
    WBT_DOWNLOAD_KIND_BOTH,
};

#ifdef BLACK2
#define WBT_DOWNLOAD_IS_BLACK2 TRUE
#define WBT_DOWNLOAD_IS_WHITE2 FALSE
#else
#define WBT_DOWNLOAD_IS_BLACK2 FALSE
#define WBT_DOWNLOAD_IS_WHITE2 TRUE
#endif

// Whether a tournament of this kind is for this version
static inline BOOL WbtDownloadTournament_IsForThisVersion(u32 kind) {
    switch (kind) {
    case WBT_DOWNLOAD_KIND_WHITE2:
        return WBT_DOWNLOAD_IS_WHITE2;
    case WBT_DOWNLOAD_KIND_BLACK2:
        return WBT_DOWNLOAD_IS_BLACK2;
    case WBT_DOWNLOAD_KIND_BOTH:
        return TRUE;
    default:
        return FALSE;
    }
}

static inline BOOL WbtDownloadTournament_IsChecksumValid(const WbtDownloadTournament *tournament) {
    if (tournament->checksum == getCRC16(tournament, sizeof(WbtDownloadTournament) - sizeof(u32))) {
        return TRUE;
    }
    return FALSE;
}

static inline BOOL WbtDownloadTournament_IsValid(const WbtDownloadTournament *tournament) {
    if (tournament->name != 0 && WbtDownloadTournament_IsForThisVersion(tournament->kind) && tournament->version == 2 &&
        WbtDownloadTournament_IsChecksumValid(tournament)) {
        return TRUE;
    }
    return FALSE;
}

static void WbtDownloadNet_DownloadStep(WbtDownloadStack *stack, u32 *state, void *userData) {
    WbtDownloadNetWork *wk = userData;

    switch (*state) {
    case 0: {
        NetCommonDownloadFileListArgs args;

        sys_memset(&args, 0, sizeof(NetCommonDownloadFileListArgs));
        args.fileCount = 1;
        args.attr1 = "WBT_E";
        // The ROM's strings for these are two bytes long, not one: they hold two NULs
        args.attr2 = "\0";
        args.attr3 = "\0";
        args.select = WbtDownloadNet_SelectFile;
        args.unk14 = WBT_DOWNLOAD_FILE_SIZE;
        if (NetCommonDownload_Request(wk->download, NET_COMMON_DOWNLOAD_FILE_LIST, &args)) {
            (*state)++;
        }
        break;
    }
    case 1:
        if (NetCommonDownload_GetKind(wk->download) == NET_COMMON_DOWNLOAD_KIND_IDLE) {
            void *data = NetCommonDownload_GetData(wk->download);

            if (NetCommonDownload_GetStatus(wk->download) == NET_COMMON_DOWNLOAD_STATUS_DONE) {
                sys_memcpy(data, wk->param->buffer, WBT_DOWNLOAD_BUFFER_SIZE);
            }
            (*state)++;
        }
        break;
    case 2: {
        WbtDownloadFile *file = NetCommonDownload_GetData(wk->download);
        BOOL success = FALSE;

        switch (NetCommonDownload_GetStatus(wk->download)) {
        case NET_COMMON_DOWNLOAD_STATUS_DONE: {
            int validCount;
            int i;
            BOOL fileValid = file->checksum == getCRC16(file, WBT_DOWNLOAD_FILE_SIZE - sizeof(u32)) ? TRUE : success;

            if (fileValid) {
                validCount = 0;
                for (i = 0; i < WBT_DOWNLOAD_TOURNAMENT_COUNT; i++) {
                    if (WbtDownloadTournament_IsValid(&file->tournaments[i])) {
                        validCount++;
                    }
                }
            } else {
                validCount = 0;
            }
            if (validCount > 0) {
                success = TRUE;
                WbtDownloadMessage_PrintMsg(wk->message, wk->msgData, WBT_DOWNLOAD_MSG_RECEIVED,
                                            WBT_DOWNLOAD_MSG_STREAM);
            } else {
                WbtDownloadMessage_PrintMsg(wk->message, wk->msgData, WBT_DOWNLOAD_MSG_INVALID,
                                            WBT_DOWNLOAD_MSG_STREAM);
            }
            break;
        }
        case NET_COMMON_DOWNLOAD_STATUS_TIMEOUT:
            WbtDownloadMessage_PrintMsg(wk->message, wk->msgData, WBT_DOWNLOAD_MSG_TIMEOUT, WBT_DOWNLOAD_MSG_STREAM);
            break;
        case NET_COMMON_DOWNLOAD_STATUS_NONE:
        case NET_COMMON_DOWNLOAD_STATUS_NO_FILE:
        case NET_COMMON_DOWNLOAD_STATUS_CANCELLED:
        case NET_COMMON_DOWNLOAD_STATUS_ERROR:
        default:
            WbtDownloadMessage_PrintMsg(wk->message, wk->msgData, WBT_DOWNLOAD_MSG_NO_FILE, WBT_DOWNLOAD_MSG_STREAM);
            break;
        }
        if (success) {
            wk->param->result = 0;
        } else {
            wk->param->result = 2;
        }
        (*state)++;
        break;
    }
    case 3:
        if (WbtDownloadMessage_IsDone(wk->message)) {
            (*state)++;
        }
        break;
    case 4:
        WbtDownloadStack_Pop(stack);
        break;
    }
}

static void WbtDownloadNetBG_Create(WbtDownloadBG *bg, HeapID heapId) {
    sys_memset(bg, 0, sizeof(WbtDownloadBG));
    WbtDownloadNetBG_Load(bg, heapId);
}

static void WbtDownloadNetBG_Free(WbtDownloadBG *bg) {
    WbtDownloadNetBG_Unload(bg);
}

static void WbtDownloadNetBG_Update(WbtDownloadBG *bg) {
}

// Loads the palettes the cycle blends between, makes the blends and starts the cycle
static inline void WbtDownloadNetBG_InitCycle(WbtDownloadBG *bg, ArcTool *arc, u32 heapId) {
    int i;
    int count = 0;
    NNSG2dPaletteData *palette;
    void *file;

    sys_memset(&bg->cycle, 0, sizeof(bg->cycle));
    file = GFL_G2DIOReadNCLRArc(arc, 2, &palette, heapId);
    sys_memcpy(palette->rawData, bg->cycle.palettes, sizeof(bg->cycle.palettes));
    sys_memcpy(palette->rawData, bg->cycle.blended, sizeof(bg->cycle.palettes));
    GFL_HeapFree(file);

    // Colors 4 to 10 of each palette, blended into the next in 4 steps
    for (i = 0; i < WBT_DOWNLOAD_PALETTE_COUNT; i++) {
        int next = i + 1;
        fx32 fraction;
        BOOL done;

        if (next >= WBT_DOWNLOAD_PALETTE_COUNT) {
            next -= WBT_DOWNLOAD_PALETTE_COUNT;
        }
        fraction = 0;
        done = FALSE;
        while (TRUE) {
            int j;

            for (j = 4; j < 11; j++) {
                BlendColors(&bg->cycle.palettes[i][j], &bg->cycle.blended[count][j], 1, fraction >> 8,
                            bg->cycle.palettes[next][j]);
            }
            count++;
            if (done == TRUE) {
                break;
            }
            fraction += FX32_ONE * 6 / 16;
            if (fraction >= FX32_ONE) {
                fraction = FX32_ONE;
                done = TRUE;
            }
        }
    }
    cp15_flushDC(bg->cycle.blended, sizeof(bg->cycle.blended));
    bg->cycle.active = TRUE;
    bg->cycle.task = GFL_VBlankTCBAdd(WbtDownloadNetBG_CyclePalette, &bg->cycle, 20);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0, GX_BLEND_PLANEMASK_BG3, 8, 16);
    bg->cycle.scrollBg = 0;
}

static void WbtDownloadNetBG_Load(WbtDownloadBG *bg, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PDWACC, heapId);
    u32 chars;

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_SUB_BG, 0, 0, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 4, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 6, 4, 0, (u16)chars, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_MAIN_BG, 0, 0, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 5, 0, 0, (u16)chars, 0, FALSE, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 3, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 7, 3, 0, (u16)chars, 0, FALSE, heapId);
    WbtDownloadNetBG_InitCycle(bg, arc, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysAllocChar(1, 0x20, 0);
    GFL_BGSysAllocChar(2, 0x20, 0);
    bg->frameChars1 = LoadCursorImageEndOfHeap(1, 13, 0, heapId);
    bg->frameChars2 = LoadCursorImageEndOfHeap(2, 13, 0, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x180, 0x20, heapId);
}

static void WbtDownloadNetBG_CyclePalette(TCB *task, void *work) {
    WbtDownloadPaletteCycle *cycle = work;

    if (cycle->active) {
        cycle->skip ^= 1;
        if (!(cycle->skip & 1)) {
            gfxUploadStdPaletteBGA(&cycle->blended[cycle->frame][4], 0x20 + 4 * 2, 7 * 2);
            gfxUploadStdPaletteBGB(&cycle->blended[cycle->frame][4], 0x20 + 4 * 2, 7 * 2);
            cycle->frame++;
            if (cycle->frame >= WBT_DOWNLOAD_PALETTE_COUNT * WBT_DOWNLOAD_BLEND_STEPS) {
                cycle->frame = 0;
            }
            GFL_BGSysMoveBG(cycle->scrollBg, BG_MOVE_LEFT, 7);
        }
    }
}

static void WbtDownloadNetBG_Unload(WbtDownloadBG *bg) {
    GFL_BGSysFreeCharMemory(1, (u16)bg->frameChars1, (u16)(bg->frameChars1 >> 16));
    GFL_BGSysFreeCharMemory(2, (u16)bg->frameChars2, (u16)(bg->frameChars2 >> 16));
    if (bg->cycle.task != NULL) {
        GFL_TCBRemove(bg->cycle.task);
        bg->cycle.task = NULL;
        bg->cycle.active = FALSE;
    }
    *(vu16 *)REG_BLDCNT_ADDR = 0;
}

static u32 WbtDownloadNetBG_GetFrameChars(WbtDownloadBG *bg, u32 which) {
    switch (which) {
    case 0:
        return (u16)bg->frameChars1;
    case 1:
        return (u16)bg->frameChars2;
    default:
        return 0;
    }
}

static u32 WbtDownloadNet_SelectFile(DWCNdFileInfo *files, u32 count) {
    DWCNdFileInfo empty = { 0 };
    u32 i;

    for (i = 0; i < count; i++) {
        if (GFL_STD_MemCmp(&empty, &files[i], sizeof(DWCNdFileInfo)) != 0) {
            return i;
        }
    }
    return 0;
}
