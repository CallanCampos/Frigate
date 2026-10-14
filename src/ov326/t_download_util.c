#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/print_msg.h"
#include "app/t_download/t_download_graphic.h"
#include "app/t_download/t_download_local.h"
#include "app/t_download/t_download_save.h"
#include "app/t_download/t_download_util.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/hw.h"
#include "nnsys/gfd.h"
#include "pml/poke_graphic.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/scroll_bar.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The BGs, windows, messages, menus and cell actors of the Pokémon World Tournament's downloaded tournaments' screens.
// The name is ours, after the file between t_download.c and the ROM's "t_download_save.c"

// A palette, characters or a screen of the BGs to load
typedef struct {
    u8 bg;
    u8 file;
    u8 offset;
    u8 size;
} TDownloadBGRes;

// A resource of a set of cell actors to load: its file, and where it goes in the work's resources
typedef struct {
    s16 file;
    s16 index;
    u16 offset;
    u16 count;
} TDownloadObjResFile;

typedef struct {
    TDownloadObjResFile pltt;
    TDownloadObjResFile chars;
    TDownloadObjResFile cells;
    TDownloadObjResFile anims;
} TDownloadObjRes;

// Which resources a set of cell actors is drawn with, how many there are, and which array of the work holds them
typedef struct {
    s16 pltt;
    s16 chars;
    s16 cells;
    s16 count;
    s16 kind;
} TDownloadActorRes;

// A cell actor: its setup, and its position as an index into the work's positions
typedef struct {
    u8 anim;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unk4;
    u8 flags;
    u8 pos;
    u8 unk7;
} TDownloadActorData;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} TDownloadWindowData;

static void TDownloadUtil_LoadBG(TDownloadWork *wk);
static void TDownloadUtil_InitWindows(TDownloadWork *wk);
static void TDownloadUtil_Print(TDownloadWork *wk, int window, int x, int y, const StrBuf *strbuf, u32 align,
                                u16 color);
static void TDownloadUtil_ExitWindows(TDownloadWork *wk);
static void TDownloadUtil_CreateActors(TDownloadWork *wk, u32 surface, const TDownloadActorData *data,
                                       const TDownloadActorRes *res);
static void TDownloadUtil_LoadObjRes(TDownloadWork *wk, u32 vramType, const TDownloadObjRes *res);
static void TDownloadUtil_LoadUIRes(TDownloadWork *wk, u32 vramType);
static void TDownloadUtil_InitObj(TDownloadWork *wk);
static void TDownloadUtil_ExitObj(TDownloadWork *wk);
static void TDownloadUtil_SetActorAnim(TDownloadWork *wk, int kind, int index, int anim);
static s16 TDownloadUtil_GetPosition(TDownloadWork *wk, u32 index);
static void TDownloadUtil_PrintStream(TDownloadWork *wk, int window, int message);
static void TDownloadUtil_SaveListState(TDownloadWork *wk);
static void TDownloadUtil_ListPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void TDownloadUtil_ListSelect(void *work, u32 index, BOOL moved);
static void TDownloadUtil_ListScroll(void *work, s16 delta);
static void TDownloadUtil_PrintItem(TDownloadWork *wk, u32 index, PrintWindow *window, MsgData *msgData);
static void TDownloadUtil_PrintItemAt(TDownloadWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                      u32 msgId, u16 color, int y, u32 index);
static void TDownloadUtil_PrintItemLine(TDownloadWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                        u32 msgId, u32 index);
static void TDownloadUtil_LoadPalettes(TDownloadWork *wk);
static void TDownloadUtil_ShowCursorBGs(TDownloadWork *wk, BOOL show);
static void TDownloadUtil_LoadTextPalette(TDownloadWork *wk);
static void TDownloadUtil_TransferTextPalette(TDownloadWork *wk, BOOL dim);

// The tables are declared in this order for the layout of .rodata and .data
static const s16 sTDownloadReceivedMenuMsgs[] = { 0x32, 0x2d, 0x2e, -1 };

static const s16 sTDownloadSavedMenuMsgs[] = { 0x2c, 0x2d, 0x2e, -1 };

static const s16 sTDownloadRulesMsgs[] = { 3, 4, 5, 6 };

static const s16 sTDownloadSelectMenuMsgs[] = { 0x2d, 0x2e, -1, -1 };

static const TDownloadBGRes sTDownloadModeBGChars[] = {
    { 2, 4, 0 },
    { 0xff },
};

static const TDownloadBGRes sTDownloadModeBGScreens[] = {
    { 2, 0xc, 0 },
    { 0xff },
};

static const TouchRect sTDownloadReceivedTouchRects[] = {
    { 0xa8, 0xc0, 0xc8, 0xe0 },
    { 0xa8, 0xc0, 0xe8, 0xff },
    { TOUCH_RECT_END },
};

// The first window of each screen, and how many there are
static const s16 sTDownloadModeWindowRanges[T_DOWNLOAD_MODE_COUNT][2] = {
    { 14, 3 },
    { 9, 5 },
    { 14, 3 },
    { 9, 5 },
};

static const s16 sTDownloadReturnMenuMsgs[] = { 0x10, 0x11, -1, -1 };

// The animations of the tabs, each one's idle and picked
static const s16 sTDownloadTabAnims[][2] = {
    { 4, 5 },
    { 6, 7 },
    { 8, 9 },
};

static const TDownloadActorRes sTDownloadButtonActorRes = {
    0, 1, 2, T_DOWNLOAD_BUTTON_COUNT, T_DOWNLOAD_ACTOR_BUTTON,
};

static const TDownloadActorRes sTDownloadObjActorRes = {
    3, 4, 5, T_DOWNLOAD_OBJ_COUNT, T_DOWNLOAD_ACTOR_OBJ,
};

static const TDownloadBGRes sTDownloadBGPltts[] = {
    { 4, 1, 0, 4 },
    { 0, 0, 0, 5 },
    { 0xff },
};

static const TDownloadBGRes sTDownloadBGScreens[] = {
    { 7, 0xd, 0 },
    { 6, 0xe, 0 },
    { 3, 8, 0 },
    { 1, 9, 0 },
    { 0xff },
};

static const s16 sTDownloadDetailMsgs[] = { 0x2f, 0x30, 0x31, 0xc };

static const TDownloadBGRes sTDownloadBGChars[] = {
    { 7, 5, 0 },
    { 6, 5, 0 },
    { 3, 4, 0 },
    { 1, 4, 0 },
    { 0xff },
};

static const s16 sTDownloadTitleMsgs[] = { 8, 0, 9, 10, 11 };

static const s16 *sTDownloadMenuMsgs[] = {
    sTDownloadSavedMenuMsgs,
    sTDownloadReceivedMenuMsgs,
    sTDownloadReturnMenuMsgs,
    sTDownloadSelectMenuMsgs,
};

// The messages of the screens, each in a message box
static const s16 sTDownloadMessages[] = {
    0x2a, 0x2b, 0x27, 0x1d, 0x20, 0x0e, 0x15, 0x33, 0x1a, 0x19, 0x23, 0x17, 0x18,
};

static const TDownloadObjRes sTDownloadObjRes = {
    { 3, 3, 0, 6 },
    { 6, 4 },
    { 7, 5 },
    { 0xf, -1 },
};

static const FrameListCallbacks sTDownloadListCallbacks = {
    TDownloadUtil_ListPrint,
    TDownloadUtil_ListSelect,
    TDownloadUtil_ListScroll,
};

static const TouchRect sTDownloadDetailTouchRects[] = {
    { 0xa8, 0xc0, 0xc8, 0xe0 }, { 0xa8, 0xc0, 0xe8, 0xff }, { 0xa8, 0xc0, 0x88, 0xa0 },
    { 0xa8, 0xc0, 0xa8, 0xc0 }, { 0xa8, 0xc0, 0x08, 0x20 }, { 0xa8, 0xc0, 0x28, 0x40 },
    { 0xa8, 0xc0, 0x48, 0x60 }, { 0x20, 0xa0, 0xec, 0xff }, { TOUCH_RECT_END },
};

static const FrameListTouch sTDownloadListTouch[] = {
    { { 0x18, 0x2f, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x30, 0x47, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x48, 0x5f, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x60, 0x77, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x78, 0x8f, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x90, 0xa7, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x20, 0xa0, 0xec, 0xff }, FRAMELIST_TOUCH_BAR },
    { { 0xa8, 0xc0, 0x88, 0xa0 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 0xa8, 0xc0, 0xa8, 0xc0 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END } },
};

static const TDownloadWindowData sTDownloadSavedWindows[] = {
    { 2, 3, 5, 0x1c, 3, 2 },
    { 2, 3, 0xa, 0x1c, 3, 2 },
    { 2, 3, 0xf, 0x1c, 3, 2 },
    { 0xff },
};

static const TouchRect sTDownloadSavedTouchRects[] = {
    { 0xa8, 0xc0, 0xe8, 0xff }, { 0xa8, 0xc0, 0x88, 0xa0 }, { 0xa8, 0xc0, 0xa8, 0xc0 },
    { 0x2c, 0x44, 0x08, 0xe8 }, { 0x54, 0x6c, 0x08, 0xe8 }, { 0x7c, 0x94, 0x08, 0xe8 },
    { TOUCH_RECT_END },
};

static const TDownloadWindowData sTDownloadReceivedWindows[] = {
    { 0, 2, 5, 0x1c, 2, 3 },
    { 0, 2, 8, 0x1c, 2, 3 },
    { 0, 2, 0xb, 0x1c, 2, 3 },
    { 0, 2, 0xe, 0x1c, 2, 3 },
    { 0, 2, 0x11, 0x1c, 2, 3 },
    { 0xff },
};

static const TouchRect sTDownloadSelectTouchRects[] = {
    { 0xa8, 0xc0, 0xe8, 0xff }, { 0xa8, 0xc0, 0x88, 0xa0 }, { 0xa8, 0xc0, 0xa8, 0xc0 },
    { 0x2c, 0x44, 0x08, 0xe8 }, { 0x54, 0x6c, 0x08, 0xe8 }, { 0x7c, 0x94, 0x08, 0xe8 },
    { TOUCH_RECT_END },
};

// The rows and buttons of a list that doesn't scroll
static const FrameListTouch sTDownloadListTouchNoScroll[] = {
    { { 0x18, 0x2f, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x30, 0x47, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x48, 0x5f, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x60, 0x77, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x78, 0x8f, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0x90, 0xa7, 0x08, 0xe8 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 0xa8, 0xc0, 0x88, 0xa0 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 0xa8, 0xc0, 0xa8, 0xc0 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END } },
};

static const FrameListSetup sTDownloadListSetup = {
    { 2, 0xff, 1, 3, 0x1c, 3, 2, 0, 0x1c, 3, 2, { 0x18, 0xc, 8, 6, 4, 3 }, 2, 0x10 },
    7,
    2,
    0,
    6,
    0,
    sTDownloadListTouch,
};

static const TDownloadActorData sTDownloadObjActors[] = {
    { 0, 0x30, 1, TRUE, 0, 1, 4 },
    { 1, 0x38, 1, TRUE, 0, 1, 0x11 },
    { 2, 0x30, 1, TRUE, 0, 1, 0x12 },
    { 4, 0x30, 1, FALSE, 0, 1, 0x18 },
    { 6, 0x30, 1, FALSE, 0, 1, 0x19 },
    { 8, 0x30, 1, FALSE, 0, 1, 0x1a },
    { 0xa, 0x30, 3, FALSE, 0, 1, 0x1b },
    { 0xa, 0x30, 3, FALSE, 0, 1, 0x1c },
    { 0xa, 0x30, 3, FALSE, 0, 1, 0x1d },
    { 0xff },
};

static const TDownloadBGRes *sTDownloadModeChars[T_DOWNLOAD_MODE_COUNT] = {
    sTDownloadModeBGChars,
    sTDownloadModeBGChars,
    sTDownloadModeBGChars,
    sTDownloadModeBGChars,
};

// The animations of the buttons, for a button pressed while it can't be, when it is pressed and when it can't be
static const s16 sTDownloadButtonAnims[T_DOWNLOAD_BUTTON_COUNT * 4] = {
    4, 0xc, 0x12, 0, 5, 0xd, 0x13, 0, 0, 8, 0xe, 0, 1, 9, 0xf, 0, 2, 0xa, 0x10, 0, 3, 0xb, 0x11, 0,
};

static const TDownloadWindowData *sTDownloadModeWindows[T_DOWNLOAD_MODE_COUNT] = {
    sTDownloadSavedWindows,
    sTDownloadReceivedWindows,
    sTDownloadSavedWindows,
    sTDownloadReceivedWindows,
};

static const TDownloadActorData sTDownloadButtonActors[] = {
    { 4, 0x30, 1, TRUE, 0, 1, 7 },
    { 5, 0x30, 1, TRUE, 0, 1, 8 },
    { 0, 0x30, 1, TRUE, 0, 1, 9 },
    { 1, 0x30, 1, TRUE, 0, 1, 10 },
    { 2, 0x30, 1, FALSE, 0, 1, 7 },
    { 3, 0x30, 1, FALSE, 0, 1, 8 },
    { 0xff },
};

static const TouchRect *sTDownloadTouchRects[T_DOWNLOAD_MODE_COUNT] = {
    sTDownloadSavedTouchRects,
    sTDownloadReceivedTouchRects,
    sTDownloadSelectTouchRects,
    sTDownloadDetailTouchRects,
};

static const TDownloadBGRes *sTDownloadModeScreens[T_DOWNLOAD_MODE_COUNT] = {
    sTDownloadModeBGScreens,
    sTDownloadModeBGScreens,
    sTDownloadModeBGScreens,
    sTDownloadModeBGScreens,
};

static const TDownloadWindowData sTDownloadWindows[] = {
    { 5, 2, 3, 0x1c, 2, 2 },
    { 5, 2, 7, 0xc, 2, 2 },
    { 5, 0xf, 7, 0x10, 2, 2 },
    { 5, 2, 9, 0x1c, 6, 2 },
    { 5, 2, 0x10, 0x12, 2, 2 },
    { 5, 2, 0x12, 0x1c, 4, 2 },
    { 0, 1, 0, 0x1e, 3, 7 },
    { 0, 1, 4, 0x1e, 4, 9 },
    { 0, 1, 0x13, 0x1e, 4, 9 },
    { 0xff },
};

void TDownloadUtil_Nop(TDownloadWork *wk) {
}

void TDownloadUtil_Init(TDownloadWork *wk) {
    wk->taskMenu = NULL;
    wk->taskMenuRes = AppTaskMenuRes_Create(0, 11, wk->font, wk->printQueue, wk->heapId);
    TDownloadUtil_LoadBG(wk);
    TDownloadUtil_InitWindows(wk);
    TDownloadUtil_InitObj(wk);
    TDownloadUtil_LoadPalettes(wk);
    TDownloadUtil_LoadTextPalette(wk);
}

void TDownloadUtil_Exit(TDownloadWork *wk) {
    TDownloadUtil_ExitWindows(wk);
    TDownloadUtil_ExitObj(wk);
    TDownloadUtil_FreeTaskMenu(wk);
}

static void TDownloadUtil_LoadBG(TDownloadWork *wk) {
    int i;
    const TDownloadBGRes *pltts = sTDownloadBGPltts;
    const TDownloadBGRes *chars = sTDownloadBGChars;
    const TDownloadBGRes *screens = sTDownloadBGScreens;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_T_DOWNLOAD, wk->heapId);

    for (i = 0; i < 8; i++) {
        if (pltts->bg == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, pltts->file, pltts->bg, pltts->offset, pltts->size * 32, wk->heapId);
        pltts++;
    }
    for (i = 0; i < 8; i++) {
        if (chars->bg == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->file, chars->bg, chars->offset, 0, FALSE, wk->heapId);
        chars++;
    }
    for (i = 0; i < 8; i++) {
        if (screens->bg == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screens->file, screens->bg, screens->offset, 0, FALSE, wk->heapId);
        screens++;
    }
    GFL_ArcToolFree(arc);
}

void TDownloadUtil_LoadModeBG(TDownloadWork *wk, int mode) {
    int i;
    const TDownloadBGRes *chars = sTDownloadModeChars[mode];
    const TDownloadBGRes *screens = sTDownloadModeScreens[mode];
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_T_DOWNLOAD, wk->heapId);

    for (i = 0; i < 8; i++) {
        if (chars->bg == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->file, chars->bg, chars->offset, 0, FALSE, wk->heapId);
        chars++;
    }
    for (i = 0; i < 8; i++) {
        if (screens->bg == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screens->file, screens->bg, screens->offset, 0, FALSE, wk->heapId);
        screens++;
    }
    GFL_ArcToolFree(arc);
}

static void TDownloadUtil_InitWindows(TDownloadWork *wk) {
    int i;
    const TDownloadWindowData *data = sTDownloadWindows;
    ArcTool *arc;

    for (i = 0; i < T_DOWNLOAD_WINDOW_COUNT; i++) {
        if (data->bg == 0xff) {
            break;
        }
        wk->windows[i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        BmpWin_FlushMap(wk->windows[i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[i].window));
        wk->mapFlushMask &= ~(1 << i);
        data++;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0xe0, 0x20, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0x120, 0x20, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 4, 0x100, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void TDownloadUtil_Print(TDownloadWork *wk, int window, int x, int y, const StrBuf *strbuf, u32 align,
                                u16 color) {
    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, strbuf, wk->font, color, align);
}

static void TDownloadUtil_ExitWindows(TDownloadWork *wk) {
    int i;

    for (i = 0; i < T_DOWNLOAD_WINDOW_COUNT; i++) {
        if (wk->windows[i].window != NULL) {
            BmpWin_Free(wk->windows[i].window);
            wk->windows[i].window = NULL;
        }
    }
    if (wk->streamStr != NULL) {
        GFL_StrBufFree(wk->streamStr);
        wk->streamStr = NULL;
    }
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
        wk->stream = NULL;
    }
}

void TDownloadUtil_ClearWindow(TDownloadWork *wk, int window) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[window].window), 0);
    wk->windows[window].flushPending = TRUE;
}

void TDownloadUtil_InitModeWindows(TDownloadWork *wk, int mode) {
    int i;
    const TDownloadWindowData *data = sTDownloadModeWindows[mode];
    int first = sTDownloadModeWindowRanges[mode][0];

    for (i = 0; i < T_DOWNLOAD_WINDOW_COUNT; i++) {
        if (data->bg == 0xff) {
            break;
        }
        wk->windows[first + i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        BmpWin_FlushMap(wk->windows[first + i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[first + i].window));
        data++;
    }
}

void TDownloadUtil_ExitModeWindows(TDownloadWork *wk, int mode) {
    int i;
    s16 first = sTDownloadModeWindowRanges[mode][0];
    s16 count = sTDownloadModeWindowRanges[mode][1];

    for (i = 0; i < count; i++) {
        if (wk->windows[first + i].window != NULL) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[first + i].window), 0);
            BmpWin_ClearScreen(wk->windows[first + i].window);
            BmpWin_Free(wk->windows[first + i].window);
            wk->windows[first + i].window = NULL;
        }
    }
    if (mode == T_DOWNLOAD_MODE_DETAIL) {
        GFL_BGSysClearScr(0);
    } else {
        GFL_BGSysClearScr(2);
    }
}

static void TDownloadUtil_CreateActors(TDownloadWork *wk, u32 surface, const TDownloadActorData *data,
                                       const TDownloadActorRes *res) {
    int i;
    ClActUnit *unit = TDownloadGraphic_GetClActUnit(wk->graphic);
    ClActorSetup setup;
    ClActor **actors;
    s16 pltt, chars, cells, count;
    u32 pos;
    s16 x, y;
    BOOL visible, autoAnim;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    pltt = res->pltt;
    chars = res->chars;
    cells = res->cells;
    count = res->count;
    if (res->kind == T_DOWNLOAD_ACTOR_OBJ) {
        actors = wk->actors;
    } else if (res->kind == T_DOWNLOAD_ACTOR_BUTTON) {
        actors = wk->buttons;
    }
    for (i = 0; i < count; i++) {
        if (data->anim == 0xff) {
            break;
        }
        pos = data->pos * 2;
        x = TDownloadUtil_GetPosition(wk, pos);
        y = TDownloadUtil_GetPosition(wk, pos + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = data->anim;
        setup.priority = data->priority;
        setup.bgPriority = data->bgPriority;
        actors[i] = func_0204c040(unit, wk->res[chars], wk->res[pltt], wk->res[cells], &setup, surface, wk->heapId);
        visible = FALSE;
        if (data->visible == TRUE) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        autoAnim = FALSE;
        if (data->flags & 1) {
            autoAnim = TRUE;
        }
        func_0204c520(actors[i], autoAnim);
        if (data->flags & 2) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (data->flags & 4) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        data++;
    }
}

static void TDownloadUtil_LoadObjRes(TDownloadWork *wk, u32 vramType, const TDownloadObjRes *res) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_T_DOWNLOAD, wk->heapId);
    ClActUnit *unit = TDownloadGraphic_GetClActUnit(wk->graphic);

    wk->res[res->pltt.index] =
        func_0204bbb8(arc, res->pltt.file, vramType, res->pltt.offset, 0, res->pltt.count, wk->heapId);
    wk->res[res->chars.index] = func_0204b81c(arc, res->chars.file, FALSE, vramType, wk->heapId);
    wk->res[res->cells.index] = func_0204bde0(arc, res->cells.file, res->anims.file, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void TDownloadUtil_LoadUIRes(TDownloadWork *wk, u32 vramType) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    ClActUnit *unit = TDownloadGraphic_GetClActUnit(wk->graphic);
    u32 pltt = func_0202d810();
    u32 chars = func_0202d814();
    u32 cells = func_0202d818(2);
    u32 anims = func_0202d81c(2);

    wk->res[0] = func_0204bbb8(arc, pltt, vramType, 0x160, 0, 4, wk->heapId);
    wk->res[1] = func_0204b81c(arc, chars, FALSE, vramType, wk->heapId);
    wk->res[2] = func_0204bde0(arc, cells, anims, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void TDownloadUtil_InitObj(TDownloadWork *wk) {
    ClActUnit *unit = TDownloadGraphic_GetClActUnit(wk->graphic);

    TDownloadUtil_LoadObjRes(wk, 0, &sTDownloadObjRes);
    TDownloadUtil_LoadUIRes(wk, 0);
    TDownloadUtil_CreateActors(wk, 0, sTDownloadObjActors, &sTDownloadObjActorRes);
    TDownloadUtil_CreateActors(wk, 0, sTDownloadButtonActors, &sTDownloadButtonActorRes);
    func_02042ba8(TRUE, wk->heapId);
}

static void TDownloadUtil_ExitObj(TDownloadWork *wk) {
    int i;

    for (i = 0; i < T_DOWNLOAD_OBJ_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    for (i = 0; i < T_DOWNLOAD_BUTTON_COUNT; i++) {
        func_0204c108(wk->buttons[i]);
    }
    func_0204bcd0(wk->res[3]);
    func_0204b98c(wk->res[4]);
    func_0204be64(wk->res[5]);
    func_0204bcd0(wk->res[0]);
    func_0204b98c(wk->res[1]);
    func_0204be64(wk->res[2]);
}

static void TDownloadUtil_SetActorAnim(TDownloadWork *wk, int kind, int index, int anim) {
    if (kind == T_DOWNLOAD_ACTOR_OBJ) {
        if (index < T_DOWNLOAD_OBJ_COUNT) {
            func_0204c488(wk->actors[index], anim);
            func_0204c520(wk->actors[index], TRUE);
        }
    } else if (kind == T_DOWNLOAD_ACTOR_BUTTON) {
        if (index < T_DOWNLOAD_BUTTON_COUNT) {
            func_0204c488(wk->buttons[index], anim);
            func_0204c520(wk->buttons[index], TRUE);
        }
    }
}

void TDownloadUtil_SetActorVisible(TDownloadWork *wk, int kind, int index, BOOL visible) {
    if (kind == T_DOWNLOAD_ACTOR_OBJ) {
        if (index < T_DOWNLOAD_OBJ_COUNT) {
            func_0204c124(wk->actors[index], visible);
        }
    } else if (kind == T_DOWNLOAD_ACTOR_BUTTON) {
        if (index < T_DOWNLOAD_BUTTON_COUNT) {
            func_0204c124(wk->buttons[index], visible);
        }
    }
}

BOOL TDownloadUtil_IsActorAnimating(TDownloadWork *wk, int kind, int index) {
    BOOL animating = TRUE;

    if (kind == T_DOWNLOAD_ACTOR_OBJ) {
        if (index >= T_DOWNLOAD_OBJ_COUNT) {
            return FALSE;
        }
        if (func_0204c560(wk->actors[index]) == FALSE) {
            animating = FALSE;
        }
    } else if (kind == T_DOWNLOAD_ACTOR_BUTTON) {
        if (index >= T_DOWNLOAD_BUTTON_COUNT) {
            return FALSE;
        }
        if (func_0204c560(wk->buttons[index]) == FALSE) {
            animating = FALSE;
        }
    }
    return animating;
}

static s16 TDownloadUtil_GetPosition(TDownloadWork *wk, u32 index) {
    if (index >= 60) {
        return 0;
    }
    return wk->objPos[index];
}

void TDownloadUtil_InitTaskMenu(TDownloadWork *wk, int type) {
    int i;
    int count = 0;
    const s16 *msgs = sTDownloadMenuMsgs[type];
    AppTaskMenuInit init;

    for (i = 0; i < 3; i++) {
        if (msgs[i] == -1) {
            break;
        }
        count++;
    }
    if (count > 3) {
        count = 3;
    }
    init.heapId = wk->heapId;
    init.itemCount = count;
    init.items = wk->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 13;
    init.height = 3;
    for (i = 0; i < count; i++) {
        wk->menuItems[i].str = GFL_StrBufCreate(100, wk->heapId);
        GFL_MsgDataLoadStrbuf(wk->msgData[0], msgs[i], wk->menuItems[i].str);
        wk->menuItems[i].color = 0x39e0;
        if (i == 2) {
            wk->menuItems[i].type = APP_TASKMENU_ITEM_RETURN;
        } else {
            wk->menuItems[i].type = 0;
        }
    }
    wk->taskMenu = AppTaskMenu_Create(&init, wk->taskMenuRes);
    for (i = 0; i < count; i++) {
        GFL_StrBufFree(wk->menuItems[i].str);
    }
}

void TDownloadUtil_FreeTaskMenu(TDownloadWork *wk) {
    if (wk->taskMenu != NULL) {
        AppTaskMenu_Free(wk->taskMenu);
        wk->taskMenu = NULL;
    }
}

int TDownloadUtil_UpdateTaskMenu(TDownloadWork *wk) {
    if (wk->taskMenu == NULL) {
        return -1;
    }
    AppTaskMenu_Update(wk->taskMenu);
    if (AppTaskMenu_IsFlashFinished(wk->taskMenu)) {
        return AppTaskMenu_GetCursorPos(wk->taskMenu);
    }
    return -1;
}

static void TDownloadUtil_PrintStream(TDownloadWork *wk, int window, int message) {
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
        wk->stream = NULL;
    }
    if (wk->streamStr != NULL) {
        GFL_StrBufFree(wk->streamStr);
        wk->streamStr = NULL;
    }
    wk->streamStr = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadMessages[message]);
    wk->stream = func_02022268(wk->windows[window].window, 0, 0, wk->streamStr, wk->font, func_02017bcc(), wk->tcbEx, 4,
                               wk->heapId, 15);
}

BOOL TDownloadUtil_UpdateMessage(TDownloadWork *wk) {
    if (wk->stream == NULL) {
        return FALSE;
    }
    KeyCursor_Update(wk->keyCursor, wk->stream, wk->windows[T_DOWNLOAD_WINDOW_MESSAGE].window);
    if (AppPrintsysCommon_Update(&wk->printWait, wk->stream) == TRUE) {
        return FALSE;
    }
    return TRUE;
}

void TDownloadUtil_ClearMessage(TDownloadWork *wk, BOOL clear) {
    if (clear == FALSE) {
        GFL_BGSysClearBG(0);
    } else {
        GFL_BGSysClearScr(0);
    }
}

void TDownloadUtil_PrintMessage(TDownloadWork *wk, int window, u32 message, BOOL stream) {
    BOOL formatted = FALSE;
    StrBuf *strbuf;
    StrBuf *format;
    StrBuf *name;
    TDownloadTournament *record;

    LoadSysMsgBox(0, 1, 8, 0, wk->heapId);
    BmpWin_DrawFrame(wk->windows[window].window, 0, 1, 8);
    BmpWin_FlushMap(wk->windows[window].window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[window].window));
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[window].window), 15);
    BmpWin_FlushChar(wk->windows[window].window);
    if (stream == FALSE) {
        switch (message) {
        case 1:
        case 2:
        case 3:
        case 4:
            formatted = TRUE;
            break;
        }
        if (formatted == FALSE) {
            strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadMessages[message]);
            TDownloadUtil_Print(wk, window, 0, 0, strbuf, 0, 0x440);
            GFL_StrBufFree(strbuf);
            return;
        }
        record = wk->record;
        format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadMessages[message]);
        name = GFL_StrBufCreate(128, wk->heapId);
        strbuf = GFL_StrBufCreate(128, wk->heapId);
        TDownloadSave_LoadString(wk, record, 0, name);
        func_0202437c(wk->wordSet, 0, name, 0, 1, 2);
        GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
        TDownloadUtil_Print(wk, window, 0, 0, strbuf, 0, 0x440);
        GFL_StrBufFree(format);
        GFL_StrBufFree(name);
        GFL_StrBufFree(strbuf);
    } else {
        TDownloadUtil_PrintStream(wk, window, message);
    }
}

void TDownloadUtil_PressButton(TDownloadWork *wk, int button, int dir) {
    TDownloadUtil_SetActorAnim(wk, T_DOWNLOAD_ACTOR_BUTTON, button, sTDownloadButtonAnims[button * 4 + 1 + dir]);
    if (dir == 0) {
        wk->objFlags |= 1 << button;
    }
}

void TDownloadUtil_ScrollBG(TDownloadWork *wk) {
    // The speed of the scroll, and where it wraps around
    static const u16 scroll[] = { 0x40, 0x2000 };
    int y;

    wk->bgScroll += scroll[0];
    if (wk->bgScroll >= scroll[1]) {
        wk->bgScroll = 0;
    }
    y = -(u8)(wk->bgScroll >> 8);
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_Y, y);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, y);
}

void TDownloadUtil_PrintSlotInfo(TDownloadWork *wk, int type, int index) {
    // Unused, and only here for the layout of .rodata. The first three are the messages the function prints
    static const s16 unused[] = { 1, 2, 7, 0x24 };
    int i;
    StrBuf *name;
    StrBuf *buf;
    StrBuf *format;
    StrBuf *strbuf;
    TDownloadTournament *data;

    if (type == 0) {
        data = wk->savedData[index];
    } else {
        data = wk->receivedData[index];
    }
    for (i = 0; i < 6; i++) {
        TDownloadUtil_ClearWindow(wk, i);
    }
    name = GFL_StrBufCreate(128, wk->heapId);
    buf = GFL_StrBufCreate(128, wk->heapId);
    TDownloadSave_LoadString(wk, data, 0, name);
    func_0202437c(wk->wordSet, 0, name, 0, 1, 2);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], unused[0]);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, format);
    TDownloadUtil_Print(wk, 0, 112, 0, buf, 2, 0x31a0);
    GFL_StrBufFree(name);
    GFL_StrBufFree(format);
    GFL_StrBufFree(buf);

    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], unused[1]);
    TDownloadUtil_Print(wk, 1, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);

    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadRulesMsgs[data->rules & 3]);
    TDownloadUtil_Print(wk, 2, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);

    strbuf = GFL_StrBufCreate(256, wk->heapId);
    TDownloadSave_LoadString(wk, data, 2, strbuf);
    TDownloadUtil_Print(wk, 3, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);

    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], unused[2]);
    TDownloadUtil_Print(wk, 4, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);

    strbuf = GFL_StrBufCreate(256, wk->heapId);
    TDownloadSave_LoadString(wk, data, 1, strbuf);
    TDownloadUtil_Print(wk, 5, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);
    wk->record = data;
}

void TDownloadUtil_InitList(TDownloadWork *wk, int mode) {
    FrameListSetup setup = sTDownloadListSetup;
    ArcTool *arc;
    int i;

    setup.work = wk;
    TDownloadUtil_FreeList(wk);
    arc = GFL_ArcSysCreateFileHandle(ARCID_T_DOWNLOAD, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 2, 0, 0, FALSE, wk->heapId);
    setup.callbacks = &sTDownloadListCallbacks;
    setup.count = wk->listCount;
    setup.cursorPos = wk->listCursor[0];
    setup.scroll = wk->listScroll[0];
    if (wk->listCount <= 6) {
        setup.touch = sTDownloadListTouchNoScroll;
    }
    wk->list = FrameList_Create(&setup, wk->heapId);
    FrameList_LoadScreen(wk->list, arc, 10, FALSE, 0);
    FrameList_LoadScreen(wk->list, arc, 11, FALSE, 1);
    FrameList_LoadCursorPalette(wk->list, arc, 0, 2, 2);
    GFL_ArcToolFree(arc);
    for (i = 0; i < setup.count; i++) {
        BOOL type = FALSE;

        if (TDownloadSave_IsSaved(wk, i) == TRUE) {
            type = TRUE;
        }
        FrameList_AddItem(wk->list, type, i);
    }
}

u32 TDownloadUtil_UpdateList(TDownloadWork *wk) {
    u32 result = FRAMELIST_NONE;

    if (wk->list != NULL) {
        result = FrameList_Main(wk->list);
    }
    return result;
}

int TDownloadUtil_ListResult(TDownloadWork *wk, u32 result) {
    int handled = 0;

    switch (result) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        handled = 2;
        break;
    case -7:
    case -5:
        TDownloadUtil_PressButton(wk, T_DOWNLOAD_BUTTON_UP, 0);
        handled = 3;
        break;
    case -6:
    case -4:
        TDownloadUtil_PressButton(wk, T_DOWNLOAD_BUTTON_DOWN, 0);
        handled = 3;
        break;
    case -3:
    case -2:
        handled = 4;
        break;
    case -11:
    case -10:
        handled = 1;
        break;
    }
    // The empty first case is kept: written as one test of 2 or 4, the code comes out 2 bytes shorter, so the
    // original seems to have handled 1 here, perhaps with code compiled out of the release build
    if (handled == 1) {
    } else if (handled == 2) {
        TDownloadUtil_SaveListState(wk);
        wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_LIST_MOVED;
    } else if (handled == 4) {
        TDownloadUtil_SaveListState(wk);
        wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_LIST_MOVED;
    }
    return handled;
}

void TDownloadUtil_FreeList(TDownloadWork *wk) {
    if (wk->list != NULL) {
        FrameList_Free(wk->list);
        wk->list = NULL;
    }
}

static void TDownloadUtil_SaveListState(TDownloadWork *wk) {
    if (wk->list != NULL) {
        wk->listPos[0] = FrameList_GetSelected(wk->list);
        wk->listCursor[0] = FrameList_GetCursor(wk->list);
        wk->listScroll[0] = FrameList_GetScroll(wk->list);
        FrameList_SetShownRows(wk->list, 7);
    }
}

static void TDownloadUtil_ListPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    TDownloadWork *wk = work;

    TDownloadUtil_PrintItem(wk, index, window, wk->msgData[0]);
}

static void TDownloadUtil_ListSelect(void *work, u32 index, BOOL moved) {
    TDownloadWork *wk = work;

    TDownloadUtil_SaveListState(wk);
    wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_LIST_MOVED;
}

static void TDownloadUtil_ListScroll(void *work, s16 delta) {
}

static void TDownloadUtil_PrintItem(TDownloadWork *wk, u32 index, PrintWindow *window, MsgData *msgData) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    TDownloadUtil_PrintItemLine(wk, window, queue, msgData, FrameList_GetValue(wk->list, index), index);
}

static void TDownloadUtil_PrintItemAt(TDownloadWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                      u32 msgId, u16 color, int y, u32 index) {
    StrBuf *name;
    StrBuf *buf;
    StrBuf *format;
    TDownloadTournament *data = wk->receivedData[index];

    name = GFL_StrBufCreate(128, wk->heapId);
    buf = GFL_StrBufCreate(128, wk->heapId);
    TDownloadSave_LoadString(wk, data, 0, name);
    func_0202437c(wk->wordSet, 0, name, 0, 1, 2);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, format);
    PrintWindow_Print(window, queue, 0, y, buf, wk->font, color);
    GFL_StrBufFree(name);
    GFL_StrBufFree(format);
    GFL_StrBufFree(buf);
}

static void TDownloadUtil_PrintItemLine(TDownloadWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                        u32 msgId, u32 index) {
    TDownloadUtil_PrintItemAt(wk, window, queue, msgData, msgId, 0x39e0, 4, index);
}

void TDownloadUtil_MoveCursor(TDownloadWork *wk, int type, int pos) {
    ClActorPos cursor;
    u32 index;

    if (type == 0) {
        cursor.x = TDownloadUtil_GetPosition(wk, (pos + 18) * 2);
        index = pos + 18;
        cursor.y = TDownloadUtil_GetPosition(wk, index * 2 + 1);
    } else {
        cursor.x = TDownloadUtil_GetPosition(wk, (pos + 27) * 2);
        index = pos + 27;
        cursor.y = TDownloadUtil_GetPosition(wk, index * 2 + 1);
    }
    func_0204c140(wk->actors[2], &cursor, 0);
    if (!(wk->objFlags & T_DOWNLOAD_OBJ_FLAG_CURSOR_KEYS)) {
        TDownloadUtil_SetActorAnim(wk, T_DOWNLOAD_ACTOR_OBJ, 2, 2);
    }
}

void TDownloadUtil_SetCursorKeys(TDownloadWork *wk, BOOL active) {
    int anim;

    wk->objFlags &= ~T_DOWNLOAD_OBJ_FLAG_CURSOR_KEYS;
    anim = 2;
    if (active == TRUE) {
        anim = 3;
        wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_CURSOR_KEYS;
    }
    TDownloadUtil_SetActorAnim(wk, T_DOWNLOAD_ACTOR_OBJ, 2, anim);
}

static void TDownloadUtil_LoadPalettes(TDownloadWork *wk) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_T_DOWNLOAD, 3, &palette, wk->heapId);
    u8 *colors = palette->rawData;

    sys_memcpy(colors + 0x60, wk->palFadeFrom, 0x20);
    sys_memcpy(colors + 0x80, wk->palFadeTo, 0x20);
    GFL_HeapFree(file);
    wk->palAnimCount = 0;
    wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_PALETTE_ANIM;
}

void TDownloadUtil_AnimatePalette(TDownloadWork *wk) {
    int i;
    s16 ratio;

    if (wk->objFlags & T_DOWNLOAD_OBJ_FLAG_PALETTE_ANIM) {
        wk->palAnimCount += 0x400;
        if (wk->palAnimCount >= 0x10000) {
            wk->palAnimCount -= 0x10000;
        }
        ratio = (FX_SinCosTable_[(wk->palAnimCount >> 4) * 2 + 1] + 0x1000) / 2;
        for (i = 0; i < 16; i++) {
            u16 from = wk->palFadeFrom[i];
            u16 to = wk->palFadeTo[i];
            u8 fr, fg, fb, tr, tg, tb;
            u8 r, g, b;

            fg = (from & 0x3e0) >> 5;
            fr = from & 0x1f;
            fb = (from & 0x7c00) >> 10;
            tb = (to & 0x7c00) >> 10;
            b = fb + ((tb - fb) * ratio >> 12);
            tr = to & 0x1f;
            r = fr + ((tr - fr) * ratio >> 12);
            tg = (to & 0x3e0) >> 5;
            g = fg + ((tg - fg) * ratio >> 12);
            wk->palFadeWork[i] = GX_RGB(r, g, b);
        }
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, 0x80, wk->palFadeWork, 0x20);
    }
}

void TDownloadUtil_SetPaletteAnim(TDownloadWork *wk, BOOL start) {
    if (start == TRUE) {
        wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_PALETTE_ANIM;
    } else {
        wk->objFlags &= ~T_DOWNLOAD_OBJ_FLAG_PALETTE_ANIM;
    }
}

void TDownloadUtil_SetTab(TDownloadWork *wk, int tab) {
    int i;

    for (i = 0; i < T_DOWNLOAD_TABS; i++) {
        TDownloadUtil_SetActorAnim(wk, T_DOWNLOAD_ACTOR_OBJ, i + 3, sTDownloadTabAnims[i][0]);
    }
    TDownloadUtil_SetActorAnim(wk, T_DOWNLOAD_ACTOR_OBJ, tab + 3, sTDownloadTabAnims[tab][1]);
    wk->palAnimCount = 0;
}

void TDownloadUtil_ShowPageButtons(TDownloadWork *wk, int tab) {
    BOOL show = TRUE;

    if (wk->pageCount[tab] == 1) {
        show = FALSE;
    }
    TDownloadUtil_SetActorVisible(wk, T_DOWNLOAD_ACTOR_OBJ, 0, show);
    TDownloadUtil_SetActorVisible(wk, T_DOWNLOAD_ACTOR_BUTTON, 5, show);
    TDownloadUtil_SetActorVisible(wk, T_DOWNLOAD_ACTOR_BUTTON, 4, show);
}

void TDownloadUtil_ClampScrollBar(TDownloadWork *wk) {
    ClActorPos pos;

    func_0204c178(wk->actors[0], &pos, 0);
    pos.y = FrameList_ClampBarPos(wk->list, pos.y);
    if (pos.y < 0x28) {
        pos.y = 0x28;
    } else if (pos.y > 0x98) {
        pos.y = 0x98;
    }
    func_0204c140(wk->actors[0], &pos, 0);
}

void TDownloadUtil_PrintDetails(TDownloadWork *wk, int type, int index, int tab) {
    int i;
    int none = FALSE;
    int offset;
    StrBuf *buf;
    int last;
    int row;
    StrBuf *format2;
    StrBuf *format;
    int perPage;
    int page;
    TDownloadTournament *data;
    u32 id;

    if (type == 0) {
        data = wk->savedData[index];
    } else {
        data = wk->receivedData[index];
    }
    for (i = 0; i < T_DOWNLOAD_WINDOW_DETAIL_COUNT; i++) {
        TDownloadUtil_ClearWindow(wk, T_DOWNLOAD_WINDOW_DETAIL + i);
    }
    perPage = 10;
    page = wk->page[tab];
    if (page == wk->pageCount[tab] - 1) {
        last = wk->itemCount[tab];
        perPage = last % 10;
        if (perPage == 0) {
            if (last == 0) {
                none = TRUE;
            }
            perPage = 10;
        }
    }
    offset = page * 10;
    buf = GFL_StrBufCreate(128, wk->heapId);
    i = 0;
    row = 0;
    do {
        if (none == TRUE) {
            format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], 0xc);
            TDownloadUtil_Print(wk, T_DOWNLOAD_WINDOW_DETAIL + row, 0, 0, format, 0, 0x39e0);
            GFL_StrBufFree(format);
            break;
        }
        format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadDetailMsgs[tab]);
        if (tab == 0) {
            id = TDownloadSave_GetEntry(wk, data, 0, offset + 1 + i);
            WordSet_LoadSpeciesName(wk->wordSet, 0, (u16)id);
        } else if (tab == 1) {
            id = TDownloadSave_GetEntry(wk, data, 1, offset + 1 + i);
            loadItemNameToStrbuf(wk->wordSet, 0, id);
        } else {
            id = TDownloadSave_GetEntry(wk, data, 2, offset + 1 + i);
            loadMoveNameToStrbuf(wk->wordSet, 0, id);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, buf, format);
        TDownloadUtil_Print(wk, T_DOWNLOAD_WINDOW_DETAIL + row, 0, 0, buf, 0, 0x39e0);
        GFL_StrBufFree(format);
        if (i + 1 >= perPage) {
            break;
        }
        format2 = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadDetailMsgs[tab]);
        if (tab == 0) {
            id = TDownloadSave_GetEntry(wk, data, 0, (i + 2) + offset);
            WordSet_LoadSpeciesName(wk->wordSet, 0, (u16)id);
        } else if (tab == 1) {
            id = TDownloadSave_GetEntry(wk, data, 1, (i + 2) + offset);
            loadItemNameToStrbuf(wk->wordSet, 0, id);
        } else {
            id = TDownloadSave_GetEntry(wk, data, 2, (i + 2) + offset);
            loadMoveNameToStrbuf(wk->wordSet, 0, id);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, buf, format2);
        TDownloadUtil_Print(wk, T_DOWNLOAD_WINDOW_DETAIL + row, 104, 0, buf, 0, 0x39e0);
        GFL_StrBufFree(format2);
        i += 2;
        if (i >= perPage) {
            break;
        }
        row++;
    } while (row < 5);
    GFL_StrBufFree(buf);
}

void TDownloadUtil_CountDetails(TDownloadWork *wk, int type, int index) {
    int i;
    TDownloadTournament *data;

    if (type == 0) {
        data = wk->savedData[index];
    } else {
        data = wk->receivedData[index];
    }
    wk->itemCount[0] = TDownloadSave_CountEntries(wk, data, 0);
    wk->itemCount[1] = TDownloadSave_CountEntries(wk, data, 1);
    wk->itemCount[2] = TDownloadSave_CountEntries(wk, data, 2);
    for (i = 0; i < T_DOWNLOAD_TABS; i++) {
        int count = wk->itemCount[i];

        if (count == 0) {
            wk->pageCount[i] = 1;
        } else {
            int pages = count / 10;

            if (count % 10 != 0) {
                pages++;
            }
            wk->pageCount[i] = pages;
        }
    }
    for (i = 0; i < T_DOWNLOAD_TABS; i++) {
        wk->page[i] = 0;
    }
}

int TDownloadUtil_GetTouchedPage(TDownloadWork *wk, int tab) {
    int page = -1;
    u32 y, x;

    if (func_0203da84(&x, &y) == TRUE) {
        page = ScrollBar_GetValue(wk->pageCount[tab] - 1, y, 0x20, 0xa0, 0x10);
    }
    return page;
}

BOOL TDownloadUtil_UpdatePageBar(TDownloadWork *wk, int tab, int page, BOOL touch) {
    int moved = FALSE;
    ClActorPos pos;
    u32 y, x;

    func_0204c178(wk->actors[0], &pos, 0);
    if (touch == FALSE) {
        pos.y = ScrollBar_GetPos(wk->pageCount[tab] - 1, page, 0x20, 0xa0, 0x10);
    } else if (func_0203da84(&x, &y) == TRUE) {
        int diff = pos.y - y;

        if (diff < 0) {
            diff = -diff;
        }
        if (diff > 1 && pos.y != 0x28 && pos.y != 0x98) {
            moved = TRUE;
        }
        pos.y = y;
    }
    if (pos.y < 0x28) {
        pos.y = 0x28;
    } else if (pos.y > 0x98) {
        pos.y = 0x98;
    }
    func_0204c140(wk->actors[0], &pos, 0);
    return moved;
}

void TDownloadUtil_PrintTitle(TDownloadWork *wk, int mode, int tab) {
    StrBuf *strbuf;
    int index;

    TDownloadUtil_ClearWindow(wk, T_DOWNLOAD_WINDOW_TITLE);
    wk->mapFlushMask |= 1 << T_DOWNLOAD_WINDOW_TITLE;
    index = 0;
    if (mode == T_DOWNLOAD_MODE_RECEIVED) {
        index = 1;
    } else if (mode == T_DOWNLOAD_MODE_DETAIL) {
        index = tab + 2;
    }
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sTDownloadTitleMsgs[index]);
    TDownloadUtil_Print(wk, T_DOWNLOAD_WINDOW_TITLE, 0, 4, strbuf, 0, 0x3c40);
    GFL_StrBufFree(strbuf);
}

int TDownloadUtil_GetTouch(TDownloadWork *wk, BOOL held, int mode) {
    if (held == FALSE) {
        return func_0203da0c(sTDownloadTouchRects[mode]);
    }
    return func_0203d9c8(sTDownloadTouchRects[mode]);
}

void TDownloadUtil_DimScreen(TDownloadWork *wk, BOOL show) {
    s32 brightness = 0;

    if (show == TRUE) {
        brightness = -8;
    }
    gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 0x1e, brightness);
    TDownloadUtil_TransferTextPalette(wk, show);
}

void TDownloadUtil_DimScreenUpload(TDownloadWork *wk, BOOL show) {
    s32 brightness = 0;
    int offset;

    if (show == TRUE) {
        brightness = -8;
    }
    gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 0x1e, brightness);
    offset = 0;
    if (show == TRUE) {
        offset = 16;
    }
    gfxUploadStdPaletteBGA(&wk->textPltt[0][offset], 0xe0, 0x20);
}

void TDownloadUtil_PauseActors(TDownloadWork *wk, BOOL show) {
    if (func_0204c138(wk->actors[0]) == TRUE) {
        if (show == TRUE) {
            func_0204c550(wk->actors[0]);
        } else {
            func_0204c540(wk->actors[0]);
        }
    }
    if (func_0204c138(wk->actors[2]) == TRUE) {
        if (show == TRUE) {
            func_0204c550(wk->actors[2]);
        } else {
            func_0204c540(wk->actors[2]);
        }
    }
}

void TDownloadUtil_PrintSlotName(TDownloadWork *wk, int type, int slot) {
    TDownloadTournament *data;
    StrBuf *name;
    StrBuf *buf;
    StrBuf *format;

    if (type == 0) {
        data = wk->savedData[slot];
    } else {
        data = wk->receivedData[slot];
    }
    TDownloadUtil_ClearWindow(wk, T_DOWNLOAD_WINDOW_SLOT + slot);
    name = GFL_StrBufCreate(128, wk->heapId);
    buf = GFL_StrBufCreate(128, wk->heapId);
    TDownloadSave_LoadString(wk, data, 0, name);
    func_0202437c(wk->wordSet, 0, name, 0, 1, 2);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, buf, format);
    TDownloadUtil_Print(wk, T_DOWNLOAD_WINDOW_SLOT + slot, 0, 4, buf, 0, 0x39e2);
    GFL_StrBufFree(name);
    GFL_StrBufFree(format);
    GFL_StrBufFree(buf);
}

static void TDownloadUtil_ShowCursorBGs(TDownloadWork *wk, BOOL show) {
    u8 enabled = TRUE;

    if (show == FALSE) {
        enabled = FALSE;
    }
    GFL_BGSysSetBGEnabled(5, enabled);
    GFL_BGSysSetBGEnabled(6, enabled);
}

void TDownloadUtil_UpdateCursorMode(TDownloadWork *wk, BOOL keys, BOOL touch, BOOL hide) {
    BOOL visible;

    if (keys == FALSE && touch == FALSE) {
        if (func_0203d554() == TRUE) {
            visible = FALSE;
        } else if (func_0203d554() == FALSE) {
            visible = TRUE;
        }
    }
    if (touch == TRUE) {
        func_0203d564(TRUE);
        visible = FALSE;
    } else if (keys == TRUE) {
        func_0203d564(FALSE);
        visible = TRUE;
    }
    if (hide == TRUE) {
        visible = TRUE;
    }
    TDownloadUtil_ShowCursorBGs(wk, visible);
    TDownloadUtil_SetActorVisible(wk, T_DOWNLOAD_ACTOR_OBJ, 2, visible);
}

static void TDownloadUtil_LoadTextPalette(TDownloadWork *wk) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_T_DOWNLOAD, 2, &palette, wk->heapId);

    sys_memcpy(palette->rawData, wk->textPltt, 0x40);
    GFL_HeapFree(file);
}

static void TDownloadUtil_TransferTextPalette(TDownloadWork *wk, BOOL dim) {
    int offset = 0;

    if (dim == TRUE) {
        offset = 16;
    }
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 0xe0, &wk->textPltt[0][offset], 0x20);
}

void TDownloadUtil_UpdateArrows(TDownloadWork *wk, int type, int min, int max, int pos) {
    if (type == 0) {
        if (!(wk->objFlags & 3)) {
            if (min >= pos) {
                TDownloadUtil_PressButton(wk, 0, 1);
            } else {
                TDownloadUtil_PressButton(wk, 0, -1);
            }
            if (pos >= max) {
                TDownloadUtil_PressButton(wk, 1, 1);
            } else {
                TDownloadUtil_PressButton(wk, 1, -1);
            }
        }
    } else if (type == 1) {
        if (!(wk->objFlags & 0x30)) {
            if (min >= pos) {
                TDownloadUtil_PressButton(wk, 5, 1);
            } else {
                TDownloadUtil_PressButton(wk, 5, -1);
            }
            if (pos >= max) {
                TDownloadUtil_PressButton(wk, 4, 1);
            } else {
                TDownloadUtil_PressButton(wk, 4, -1);
            }
        }
    }
}

BOOL TDownloadUtil_IsButtonDone(TDownloadWork *wk, int noResult) {
    BOOL found = FALSE;
    s16 button = 0;
    BOOL done = FALSE;
    u32 flags = wk->objFlags;

    if (flags & 1) {
        button = 0;
        found = TRUE;
    } else if (flags & 2) {
        button = 1;
        found = TRUE;
    } else if (flags & 4) {
        button = 2;
        found = TRUE;
    } else if (flags & 8) {
        button = 3;
        found = TRUE;
    } else if (flags & 0x10) {
        button = 4;
        found = TRUE;
    } else if (flags & 0x20) {
        button = 5;
        found = TRUE;
    }
    if (found == TRUE) {
        if (TDownloadUtil_IsActorAnimating(wk, T_DOWNLOAD_ACTOR_BUTTON, button) == FALSE) {
            wk->objFlags &= ~(1 << button);
            if (noResult == FALSE) {
                if (button == 2 || button == 3) {
                    if (button == 2) {
                        wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_YES;
                    } else if (button == 3) {
                        wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_NO;
                    }
                }
            }
            done = TRUE;
        }
    }
    return done;
}

void TDownloadUtil_ShowSlot(TDownloadWork *wk, int type, int slot) {
    if (type == 1) {
        TDownloadUtil_ShowCursorBGs(wk, FALSE);
        TDownloadUtil_ClearWindow(wk, slot + 14);
        TDownloadUtil_SetActorVisible(wk, T_DOWNLOAD_ACTOR_OBJ, slot + 6, FALSE);
        TDownloadUtil_SetActorVisible(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
    } else {
        TDownloadUtil_ShowCursorBGs(wk, TRUE);
    }
}
