#include "types.h"
#include "app/ui/frame_list.h"
#include "app/unova_link.h"
#include "constants/arc.h"
#include "constants/sound.h"
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
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/bmp_menulist.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The parts that Unova Link's screens are built from: message windows, menus and lists, the sequence of steps and the
// scenes it runs, text shown as OAM, moves over frames, the scrolling list, particles and palette fades

// The background color index of a PRINT_COLOR
#define COLOR_BACKGROUND(color) ((u8)((color) & 0x1f))

// A window that prints messages, all at once or a character at a time
struct KeySystemMsgWin {
    u32 unk00;
    Font *font;
    PrintStream *stream;
    TCBExManager *tcbManager;
    WaitIcon *waitIcon;
    BmpWin *window;
    StrBuf *str;
    // The low 5 bits are the background
    u16 color;
    u16 heapId;
    PrintWindow printWindow;
    PrintQueue *printQueue;
    // KEY_SYSTEM_MSG_*
    u32 mode;
    u16 done;
    u16 hasFrame;
    // Named after the ROM's assert
    KeyCursor *p_keycursor;
    AppPrintsysCommon printCommon;
    KeySystemPos pos;
    // KEY_SYSTEM_ALIGN_*
    u32 align;
};

struct KeySystemMsgWinGroup {
    u32 count;
    KeySystemMsgWin *wins[];
};

struct KeySystemMenu {
    BmpWin *window;
    PrintQueue *printQueue;
    PrintWindow printWindow;
    BmpMenuList *list;
    ListMenuOption *options;
    u32 cancelValue;
};

// The list's windows, in the palette of the setup or the next one for the cursor, which blinks in that palette
// between the list's palettes 1 and 2
struct KeySystemList {
    KeySystemListSetup setup;
    KeySystemMsgWin *wins[KEY_SYSTEM_LIST_MAX];
    u32 state;
    BOOL decided;
    BOOL changed;
    int cursor;
    u16 palettes[3][16];
    u16 colors[16];
    u16 angle;
    u16 timer;
};

// The steps run as a stack: a step that pushes another runs again once that one is popped
typedef struct {
    KeySystemSeqFunc func;
    int state;
    u32 unk8;
} KeySystemSeqEntry;

struct KeySystemSeq {
    void *work;
    int current;
    u32 depth;
    KeySystemSeqEntry entries[];
};

enum {
    KEY_SYSTEM_SCENE_NONE,
    KEY_SYSTEM_SCENE_INIT,
    KEY_SYSTEM_SCENE_MAIN,
    KEY_SYSTEM_SCENE_IDLE,
    KEY_SYSTEM_SCENE_EXIT,
    KEY_SYSTEM_SCENE_FREE,
};

struct KeySystemScene {
    void *work;
    // KEY_SYSTEM_SCENE_*
    u32 state;
    KeySystemSceneFuncs funcs;
    u16 heapId;
};

// Text printed into a bitmap that is shown as OAM
struct KeySystemOamText {
    GFLBitmap *bitmap;
    u16 color;
    BOOL printing;
    KeySystemPos pos;
    u32 align;
    StrBuf *str;
    BmpOamActor *actor;
    PrintQueue *printQueue;
};

struct KeySystemScrollList {
    FrameList *list;
    KeySystemScrollListSetup setup;
    StrBuf *str;
    BOOL started;
};

#define PARTICLE_WORK_SIZE 0x4800

struct KeySystemParticle {
    ParticleSystem *system;
    u32 unk4;
    void *work;
};

static void KeySystemMsgWin_Print(KeySystemMsgWin *p_wk, u32 mode);
static void KeySystemMsgWin_SetPaletteNo(KeySystemMsgWin *win, u8 palette);
static void KeySystem_CalcTextPos(u32 align, const KeySystemPos *pos, GFLBitmap *bitmap, const StrBuf *str, Font *font,
                                  KeySystemPos *out);
static KeySystemSeqEntry *KeySystemSeq_GetEntry(KeySystemSeq *seq, int index);
static void KeySystemScrollList_PrintItem(void *work, u32 index, PrintWindow *printWindow, s16 y, BOOL firstBG);
static void KeySystemScrollList_Select(void *work, u32 index, BOOL moved);
static void KeySystemScrollList_Scroll(void *work, s16 delta);
static void KeySystem_BlendColor(u32 type, u16 *dest, u16 angle, u8 palette, u8 index, u16 from, u16 to);

const u8 data_ov332_021c8d64[4] = { 10, 21, 0, 0 };

static const FrameListTouch sScrollListTouch[] = {
    { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

static const FrameListCallbacks sScrollListCallbacks = {
    KeySystemScrollList_PrintItem,
    KeySystemScrollList_Select,
    KeySystemScrollList_Scroll,
};

static const FrameListSetup sScrollListSetup = {
    { 0, 0xff, 2, 4, 28, 5, 1, 1, 26, 2, 0, 20, 20, 10, 8, 5, 4, 0, 1, 0 }, 0, 1, 0, 0, 0, sScrollListTouch, NULL, NULL,
};

static const BmpMenuListHeader sMenuListHeader = {
    NULL, NULL, NULL, 0, 0, 0, 13, 0, 3, 1, 15, 2, 0, 1, 0, 0, 0, NULL, 12, 13, NULL, NULL, NULL, NULL, 20,
};

KeySystemMsgWin *KeySystemMsgWin_Create(u16 bg, u16 x, u16 y, u16 width, u16 height, u16 palette, Font *font,
                                        HeapID heapId) {
    KeySystemMsgWin *win = GFL_HeapAllocate(heapId, sizeof(KeySystemMsgWin), FALSE, "key_system_util.c", 98);

    sys_memset(win, 0, sizeof(KeySystemMsgWin));
    win->font = font;
    win->printQueue = func_02021998(heapId);
    win->mode = KEY_SYSTEM_MSG_IDLE;
    win->heapId = heapId;
    win->str = GFL_StrBufCreate(0x300, heapId);
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    win->printWindow.window = win->window;
    win->printWindow.flushPending = FALSE;
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), COLOR_BACKGROUND(win->color));
    BmpWin_FlushChar(win->window);
    BmpWin_FlushMap(win->window);
    win->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 32);
    return win;
}

void KeySystemMsgWin_Free(KeySystemMsgWin *win) {
    if (win->stream != NULL) {
        func_020223cc(win->stream);
        win->stream = NULL;
    }
    if (win->waitIcon != NULL) {
        WaitIcon_Free(win->waitIcon);
        win->waitIcon = NULL;
    }
    if (win->p_keycursor != NULL) {
        KeyCursor_Free(win->p_keycursor);
    }
    GFL_TCBExMgrFree(win->tcbManager);
    KeySystemMsgWin_ClearFrame(win);
    BmpWin_Free(win->window);
    GFL_StrBufFree(win->str);
    func_02021a18(win->printQueue);
    GFL_HeapFree(win);
}

void KeySystemMsgWin_Update(KeySystemMsgWin *win) {
    BOOL done;

    func_02021a3c(win->printQueue);
    switch (win->mode) {
    case KEY_SYSTEM_MSG_PRINT_WAIT_ICON:
        PrintWindow_Flush(&win->printWindow, win->printQueue);
        break;
    case KEY_SYSTEM_MSG_PRINT:
        PrintWindow_Flush(&win->printWindow, win->printQueue);
        win->done = !win->printWindow.flushPending;
        break;
    case KEY_SYSTEM_MSG_STREAM:
    case KEY_SYSTEM_MSG_STREAM_FAST:
        if (win->stream != NULL) {
            if (win->p_keycursor != NULL) {
                KeyCursor_Update(win->p_keycursor, win->stream, win->window);
            }
            if (AppPrintsysCommon_Update(&win->printCommon, win->stream)) {
                win->done = TRUE;
            }
        }
        break;
    case KEY_SYSTEM_MSG_STREAM_NO_CURSOR:
        if (win->stream != NULL) {
            done = FALSE;
            if (win->p_keycursor != NULL) {
                KeyCursor_Update(win->p_keycursor, win->stream, win->window);
                done = AppPrintsysCommon_Update(&win->printCommon, win->stream);
            }
            if (done) {
                win->done = TRUE;
            }
        }
        break;
    case KEY_SYSTEM_MSG_IDLE:
        break;
    }
    GFL_TCBExMgrUpdate(win->tcbManager);
}

void KeySystemMsgWin_PrintMsg(KeySystemMsgWin *win, MsgData *msgData, u32 msgId, u32 mode) {
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->str);
    KeySystemMsgWin_Print(win, mode);
}

void KeySystemMsgWin_PrintStr(KeySystemMsgWin *win, const StrBuf *str, u32 mode) {
    GFL_StrBufCopy(win->str, str);
    KeySystemMsgWin_Print(win, mode);
}

void KeySystemMsgWin_SetColor(KeySystemMsgWin *win, u16 color) {
    win->color = color;
}

void KeySystemMsgWin_SetPos(KeySystemMsgWin *win, s32 x, s32 y, u32 align) {
    win->pos.x = x;
    win->pos.y = y;
    win->align = align;
}

static void KeySystemMsgWin_Print(KeySystemMsgWin *p_wk, u32 mode) {
    KeySystemPos pos;
    s32 wait;

    func_02021c44(p_wk->printQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->window), COLOR_BACKGROUND(p_wk->color));
    if (p_wk->stream != NULL) {
        func_020223cc(p_wk->stream);
        p_wk->stream = NULL;
    }
    if (p_wk->p_keycursor != NULL) {
        KeyCursor_Free(p_wk->p_keycursor);
        p_wk->p_keycursor = NULL;
    }
    KeySystemMsgWin_StopWaitIcon(p_wk);
    KeySystem_CalcTextPos(p_wk->align, &p_wk->pos, BmpWin_GetBitmap(p_wk->window), p_wk->str, p_wk->font, &pos);
    switch (mode) {
    case KEY_SYSTEM_MSG_PRINT_WAIT_ICON:
        p_wk->waitIcon =
            WaitIcon_Create(GFL_VBlankGetTCBMgr(), p_wk->window, COLOR_BACKGROUND(p_wk->color), 16, p_wk->heapId);
        PrintWindow_Print(&p_wk->printWindow, p_wk->printQueue, pos.x, pos.y, p_wk->str, p_wk->font, p_wk->color);
        p_wk->mode = KEY_SYSTEM_MSG_PRINT;
        break;
    case KEY_SYSTEM_MSG_PRINT:
        PrintWindow_Print(&p_wk->printWindow, p_wk->printQueue, pos.x, pos.y, p_wk->str, p_wk->font, p_wk->color);
        p_wk->mode = KEY_SYSTEM_MSG_PRINT;
        break;
    case KEY_SYSTEM_MSG_STREAM_FAST:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printCommon, 0x402);
        p_wk->p_keycursor = KeyCursor_Create(COLOR_BACKGROUND(p_wk->color), 1, 0, p_wk->heapId);
        wait = func_02017c50(1);
        p_wk->stream = func_02022268(p_wk->window, pos.x, pos.y, p_wk->str, p_wk->font, wait, p_wk->tcbManager, 0,
                                     p_wk->heapId, COLOR_BACKGROUND(p_wk->color));
        p_wk->mode = KEY_SYSTEM_MSG_STREAM;
        break;
    case KEY_SYSTEM_MSG_STREAM:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printCommon, 2);
        p_wk->p_keycursor = KeyCursor_Create(COLOR_BACKGROUND(p_wk->color), 1, 0, p_wk->heapId);
        wait = func_02017bcc();
        p_wk->stream = func_02022268(p_wk->window, pos.x, pos.y, p_wk->str, p_wk->font, wait, p_wk->tcbManager, 0,
                                     p_wk->heapId, COLOR_BACKGROUND(p_wk->color));
        p_wk->mode = KEY_SYSTEM_MSG_STREAM;
        break;
    case KEY_SYSTEM_MSG_STREAM_NO_CURSOR:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printCommon, 2);
        wait = func_02017bcc();
        p_wk->stream = func_02022268(p_wk->window, pos.x, pos.y, p_wk->str, p_wk->font, wait, p_wk->tcbManager, 0,
                                     p_wk->heapId, COLOR_BACKGROUND(p_wk->color));
        p_wk->mode = KEY_SYSTEM_MSG_STREAM_NO_CURSOR;
        break;
    }
    p_wk->done = FALSE;
}

BOOL KeySystemMsgWin_IsDone(KeySystemMsgWin *win) {
    return win->done;
}

void KeySystemMsgWin_StopWaitIcon(KeySystemMsgWin *win) {
    win->done = TRUE;
    if (win->waitIcon != NULL) {
        WaitIcon_Free(win->waitIcon);
        win->waitIcon = NULL;
        BmpWin_FlushMap(win->window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win->window));
    }
}

void KeySystemMsgWin_DrawFrame(KeySystemMsgWin *win, u16 frameChar, u8 framePalette) {
    BmpWin_DrawFrame(win->window, WINFRAME_TRANSFER_NONE, frameChar, framePalette);
    win->hasFrame = TRUE;
}

void KeySystemMsgWin_Clear(KeySystemMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), COLOR_BACKGROUND(win->color));
}

void KeySystemMsgWin_ClearFrame(KeySystemMsgWin *win) {
    if (win->hasFrame) {
        BmpWin_ClearFrame(win->window, WINFRAME_TRANSFER_NONE);
        win->hasFrame = FALSE;
    } else {
        BmpWin_ClearScreen(win->window);
    }
}

static void KeySystemMsgWin_SetPaletteNo(KeySystemMsgWin *win, u8 palette) {
    u8 x = BmpWin_GetPosX(win->window);
    u8 y = BmpWin_GetPosY(win->window);
    u8 width = BmpWin_GetSizeX(win->window);
    u8 height = BmpWin_GetSizeY(win->window);

    if (win->hasFrame) {
        x--;
        y--;
        width += 2;
        height += 2;
    }
    GFL_BGSysSetScrPaletteNo(BmpWin_GetBGIndex(win->window), x, y, width, height, palette);
}

void KeySystemMsgWin_CreateCursor(KeySystemMsgWin *win) {
    if (win->p_keycursor == NULL) {
        win->p_keycursor = KeyCursor_Create(COLOR_BACKGROUND(win->color), 1, 0, win->heapId);
    }
}

static void KeySystem_CalcTextPos(u32 align, const KeySystemPos *pos, GFLBitmap *bitmap, const StrBuf *str, Font *font,
                                  KeySystemPos *out) {
    u32 x;
    u32 y;
    s32 right;

    switch (align) {
    case KEY_SYSTEM_ALIGN_TOP_LEFT:
        *out = *pos;
        break;
    case KEY_SYSTEM_ALIGN_CENTER:
        x = GFL_BitmapGetWidth(bitmap) / 2;
        y = GFL_BitmapGetHeight(bitmap) / 2;
        x -= GFL_FontGetBlockWidth(str, font, 0) / 2;
        y -= GFL_FontGetBlockHeight(str, font) / 2;
        out->x = x + pos->x;
        out->y = y + pos->y;
        break;
    case KEY_SYSTEM_ALIGN_CENTER_Y:
        y = GFL_BitmapGetHeight(bitmap) / 2;
        y -= GFL_FontGetBlockHeight(str, font) / 2;
        out->x = pos->x;
        out->y = y + pos->y;
        break;
    case KEY_SYSTEM_ALIGN_RIGHT:
        right = GFL_BitmapGetWidth(bitmap) - GFL_FontGetBlockWidth(str, font, 0);
        if (right < 0) {
            right = 0;
        }
        out->x = right + pos->x;
        out->y = pos->y;
        break;
    }
}

KeySystemMsgWinGroup *KeySystemMsgWinGroup_Create(const KeySystemMsgWinTemplate *templates, u16 count, u16 bg,
                                                  u16 palette, Font *font, MsgData *msgData, HeapID heapId) {
    KeySystemMsgWinGroup *group =
        GFL_HeapAllocate(heapId, sizeof(KeySystemMsgWin *) * count + sizeof(u32), TRUE, "key_system_util.c", 605);
    u32 i;

    group->count = count;
    for (i = 0; i < group->count; i++) {
        const KeySystemMsgWinTemplate *template = &templates[i];

        group->wins[i] = KeySystemMsgWin_Create(bg, template->x, template->y, template->width, template->height,
                                                palette, font, heapId);
        KeySystemMsgWin_SetColor(group->wins[i], PRINT_COLOR(1, 2, 0));
        KeySystemMsgWin_PrintMsg(group->wins[i], msgData, template->msgId, KEY_SYSTEM_MSG_PRINT);
    }
    return group;
}

void KeySystemMsgWinGroup_Free(KeySystemMsgWinGroup *group) {
    u32 i;

    for (i = 0; i < group->count; i++) {
        KeySystemMsgWin_Free(group->wins[i]);
    }
    GFL_HeapFree(group);
}

void KeySystemMsgWinGroup_Update(KeySystemMsgWinGroup *group) {
    u32 i;

    for (i = 0; i < group->count; i++) {
        KeySystemMsgWin_Update(group->wins[i]);
    }
}

BOOL KeySystemMsgWinGroup_IsDone(KeySystemMsgWinGroup *group) {
    BOOL done = TRUE;
    u32 i;

    for (i = 0; i < group->count; i++) {
        // BUG: TRUE ORed with anything stays TRUE, so the windows are never waited for
#ifdef BUGFIX
        done &= KeySystemMsgWin_IsDone(group->wins[i]);
#else
        done |= KeySystemMsgWin_IsDone(group->wins[i]);
#endif
    }
    return done;
}

KeySystemMenu *KeySystemMenu_Create(const KeySystemMenuSetup *setup, HeapID heapId) {
    u8 height = setup->count * 2;

    return KeySystemMenu_CreateAt(setup, 21, 17 - height, 10, height, heapId);
}

KeySystemMenu *KeySystemMenu_CreateAt(const KeySystemMenuSetup *setup, u8 x, u8 y, u8 width, u8 height, HeapID heapId) {
    u32 i;
    KeySystemMenu *menu = GFL_HeapAllocate(heapId, sizeof(KeySystemMenu), FALSE, "key_system_util.c", 741);
    BmpMenuListHeader header;

    sys_memset(menu, 0, sizeof(KeySystemMenu));
    menu->printQueue = setup->printQueue;
    menu->cancelValue = setup->cancelValue;
    menu->window = BmpWin_CreateDynamic(setup->bg, x, y, width, height, setup->palette, TRUE);
    BmpWin_DrawFrame(menu->window, WINFRAME_TRANSFER_NONE, setup->frameChar, setup->framePalette);
    BmpWin_FlushChar(menu->window);
    BmpWin_FlushMap(menu->window);
    menu->printWindow.window = menu->window;
    menu->printWindow.flushPending = FALSE;
    menu->options = ListMenuCore_CreateOptionList(setup->count, heapId);
    for (i = 0; i < setup->count; i++) {
        ListMenuCore_AppendMsgOption(menu->options, setup->msgData, setup->msgIds[i], i, heapId);
    }
    header = sMenuListHeader;
    header.options = menu->options;
    header.count = setup->count;
    header.maxShown = setup->count;
    header.msgData = setup->msgData;
    header.printWindow = &menu->printWindow;
    header.queue = setup->printQueue;
    header.font = setup->font;
    menu->list = BmpMenuList_Create(&header, 0, setup->cursor, heapId);
    BmpMenuList_LoadCursor(menu->list, heapId);
    if (setup->cancelable) {
        BmpMenuList_SetCancelDisabled(menu->list, FALSE);
    } else {
        BmpMenuList_SetCancelDisabled(menu->list, TRUE);
    }
    return menu;
}

void KeySystemMenu_Free(KeySystemMenu *menu) {
    BmpMenuList_Free(menu->list, NULL, NULL);
    ListMenuCore_FreeOptionList(menu->options);
    BmpWin_ClearFrame(menu->window, WINFRAME_TRANSFER_NOW);
    BmpWin_ClearScreen(menu->window);
    BmpWin_Free(menu->window);
    GFL_HeapFree(menu);
}

s32 KeySystemMenu_Update(KeySystemMenu *menu) {
    s32 result;

    KeySystemMenu_UpdatePrint(menu);
    result = BmpMenuList_Update(menu->list);
    if (result == BMPMENULIST_CANCEL) {
        result = menu->cancelValue;
    }
    return result;
}

BOOL KeySystemMenu_UpdatePrint(KeySystemMenu *menu) {
    PrintWindow_Flush(&menu->printWindow, menu->printQueue);
    if (!menu->printWindow.flushPending) {
        return TRUE;
    }
    return FALSE;
}

KeySystemList *KeySystemList_Create(const KeySystemListSetup *setup, HeapID heapId) {
    KeySystemList *list = GFL_HeapAllocate(heapId, sizeof(KeySystemList), TRUE, "key_system_util.c", 932);
    NNSG2dPaletteData *palette;
    void *file;
    u16 *colors;
    u32 i;

    list->setup = *setup;
    list->cursor = setup->cursor;
    file = GFL_G2DIOReadNCLR(ARCID_KEY_SYSTEM, 1, &palette, heapId);
    colors = palette->rawData;
    sys_memcpy(&colors[0], list->palettes[0], sizeof(list->palettes[0]));
    sys_memcpy(&colors[16], list->palettes[1], sizeof(list->palettes[1]));
    sys_memcpy(&colors[32], list->palettes[2], sizeof(list->palettes[2]));
    GFL_HeapFree(file);
    for (i = 0; i < setup->count; i++) {
        list->wins[i] = KeySystemMsgWin_Create(setup->bg, setup->items[i].x, setup->items[i].y, setup->items[i].width,
                                               setup->items[i].height, setup->palette, setup->font, heapId);
        KeySystemMsgWin_SetColor(list->wins[i], PRINT_COLOR(14, 15, 1));
        KeySystemMsgWin_Clear(list->wins[i]);
        KeySystemMsgWin_DrawFrame(list->wins[i], setup->frameChar, setup->palette);
        KeySystemMsgWin_PrintMsg(list->wins[i], setup->msgData, setup->items[i].msgId, KEY_SYSTEM_MSG_PRINT);
    }
    KeySystemMsgWin_SetPaletteNo(list->wins[list->cursor], setup->palette + 1);
    return list;
}

void KeySystemList_Free(KeySystemList *list) {
    u32 i;

    for (i = 0; i < list->setup.count; i++) {
        KeySystemMsgWin_Free(list->wins[i]);
    }
    GFL_HeapFree(list);
}

void KeySystemList_Update(KeySystemList *list) {
    BOOL moved = FALSE;
    BOOL decided;
    u8 palette;
    u32 repeat;
    u32 pressed;
    u32 i;

    list->changed = FALSE;
    switch (list->state) {
    case 0:
        repeat = GCTX_HIDGetTypedKeys();
        pressed = GCTX_HIDGetPressedKeys();
        decided = FALSE;
        if (repeat & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            list->cursor--;
            list->cursor = list->cursor < 0 ? list->setup.count - 1 : list->cursor;
            moved = TRUE;
        } else if (repeat & PAD_KEY_DOWN) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            list->cursor++;
            list->cursor %= list->setup.count;
            moved = TRUE;
        } else if (pressed & PAD_BUTTON_A) {
            decided = TRUE;
            if (list->setup.select != NULL && !list->setup.select(list->cursor, list->setup.arg)) {
                decided = FALSE;
            }
            if (decided) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        } else if (pressed & PAD_BUTTON_B) {
            decided = TRUE;
            list->cursor = list->setup.count - 1;
            list->changed = TRUE;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        }
        if (list->angle + 0x400 >= 0x10000) {
            list->angle = list->angle + 0x400 - 0x10000;
        } else {
            list->angle += 0x400;
        }
        if (moved) {
            for (i = 0; i < list->setup.count; i++) {
                if (i == list->cursor) {
                    palette = list->setup.palette + 1;
                } else {
                    palette = list->setup.palette;
                }
                KeySystemMsgWin_SetPaletteNo(list->wins[i], palette);
            }
            list->changed = TRUE;
            list->angle = 0;
            GFL_BGSysQueueScrLoad(list->setup.bg);
        }
        if (decided) {
            for (i = 0; i < list->setup.count; i++) {
                KeySystemMsgWin_SetPaletteNo(list->wins[i], list->setup.palette);
            }
            GFL_BGSysUploadStdPalette(list->setup.bg, list->palettes[1], sizeof(list->palettes[1]),
                                      (list->setup.palette + 1) * 32);
            list->state++;
            list->angle = 0;
            list->timer = 0;
        }
        break;
    case 1:
        if ((list->timer / 4) % 2) {
            palette = list->setup.palette + 1;
        } else {
            palette = list->setup.palette;
        }
        KeySystemMsgWin_SetPaletteNo(list->wins[list->cursor], palette);
        GFL_BGSysQueueScrLoad(list->setup.bg);
        list->angle = 0;
        if (list->timer++ > 16) {
            list->state++;
        }
        break;
    case 2:
        list->decided = TRUE;
        break;
    }
}

void KeySystemList_Draw(KeySystemList *list) {
    u32 i;
    int j;

    for (i = 0; i < list->setup.count; i++) {
        KeySystemMsgWin_Update(list->wins[i]);
    }
    for (j = 0; j < 16; j++) {
        KeySystem_BlendColor(15, &list->colors[j], list->angle, list->setup.palette + 1, j, list->palettes[2][j],
                             list->palettes[1][j]);
    }
}

BOOL KeySystemList_IsPrinted(KeySystemList *list) {
    BOOL done = TRUE;
    u32 i;

    for (i = 0; i < list->setup.count; i++) {
        // BUG: The same as in KeySystemMsgWinGroup_IsDone
#ifdef BUGFIX
        done &= KeySystemMsgWin_IsDone(list->wins[i]);
#else
        done |= KeySystemMsgWin_IsDone(list->wins[i]);
#endif
    }
    return done;
}

BOOL KeySystemList_IsDecided(KeySystemList *list) {
    return list->decided;
}

// Takes a choice back, to choose again
void KeySystemList_Reset(KeySystemList *list) {
    list->state = 0;
    list->decided = FALSE;
    KeySystemMsgWin_SetPaletteNo(list->wins[list->cursor], list->setup.palette + 1);
    GFL_BGSysQueueScrLoad(list->setup.bg);
}

const KeySystemListSetup *KeySystemList_GetSetup(KeySystemList *list) {
    return &list->setup;
}

u32 KeySystemList_GetCursor(KeySystemList *list) {
    return list->cursor;
}

BOOL KeySystemList_IsChanged(KeySystemList *list) {
    return list->changed;
}

KeySystemSeq *KeySystemSeq_Create(u32 depth, void *work, KeySystemSeqFunc func, HeapID heapId) {
    u32 size = sizeof(KeySystemSeqEntry) * (depth + 1);
    KeySystemSeq *seq = GFL_HeapAllocate(heapId, size, FALSE, "key_system_util.c", 1294);

    sys_memset(seq, 0, size);
    seq->work = work;
    seq->depth = depth;
    KeySystemSeq_Set(seq, func);
    return seq;
}

void KeySystemSeq_Free(KeySystemSeq *seq) {
    GFL_HeapFree(seq);
}

void KeySystemSeq_Run(KeySystemSeq *seq) {
    KeySystemSeqEntry *entry = KeySystemSeq_GetEntry(seq, seq->current);

    if (entry->func != NULL) {
        entry->func(seq, &entry->state, seq->work);
    }
}

BOOL KeySystemSeq_IsEmpty(KeySystemSeq *seq) {
    BOOL empty = FALSE;

    if (KeySystemSeq_GetEntry(seq, 0)->func == NULL) {
        empty = TRUE;
    }
    return empty;
}

void KeySystemSeq_Set(KeySystemSeq *seq, KeySystemSeqFunc func) {
    KeySystemSeqEntry *entry = KeySystemSeq_GetEntry(seq, seq->current);

    sys_memset(entry, 0, sizeof(KeySystemSeqEntry));
    entry->func = func;
}

void KeySystemSeq_Push(KeySystemSeq *seq, KeySystemSeqFunc func) {
    if (KeySystemSeq_GetEntry(seq, seq->current)->func != NULL) {
        seq->current++;
    }
    KeySystemSeq_Set(seq, func);
}

void KeySystemSeq_Reset(KeySystemSeq *seq) {
    seq->current = 0;
    KeySystemSeq_Set(seq, NULL);
}

void KeySystemSeq_Pop(KeySystemSeq *seq) {
    KeySystemSeq_Set(seq, NULL);
    if (seq->current > 0) {
        seq->current--;
    }
}

BOOL KeySystemSeq_IsCurrent(KeySystemSeq *seq, KeySystemSeqFunc func) {
    if (KeySystemSeq_GetEntry(seq, seq->current)->func == func) {
        return TRUE;
    }
    return FALSE;
}

void KeySystemSeq_AddState(KeySystemSeq *seq, int add) {
    KeySystemSeq_GetEntry(seq, seq->current)->state += add;
}

void KeySystemSeq_PopTo(KeySystemSeq *seq, KeySystemSeqFunc func) {
    while (seq->current > 0) {
        if (KeySystemSeq_GetEntry(seq, seq->current)->func == func) {
            break;
        }
        KeySystemSeq_Pop(seq);
    }
}

static KeySystemSeqEntry *KeySystemSeq_GetEntry(KeySystemSeq *seq, int index) {
    return &seq->entries[index];
}

KeySystemScene *KeySystemScene_Create(void *work, HeapID heapId) {
    KeySystemScene *scene = GFL_HeapAllocate(heapId, sizeof(KeySystemScene), TRUE, "key_system_util.c", 1561);

    scene->work = work;
    return scene;
}

void KeySystemScene_Free(KeySystemScene *scene) {
    GFL_HeapFree(scene);
}

void KeySystemScene_Update(KeySystemScene *scene) {
    switch (scene->state) {
    case KEY_SYSTEM_SCENE_NONE:
        break;
    case KEY_SYSTEM_SCENE_INIT:
        if (scene->funcs.init != NULL) {
            scene->funcs.init(scene->work, scene->heapId);
        }
        scene->state = KEY_SYSTEM_SCENE_MAIN;
    case KEY_SYSTEM_SCENE_MAIN:
        if (scene->funcs.main == NULL || scene->funcs.main(scene->work)) {
            scene->state = KEY_SYSTEM_SCENE_IDLE;
        }
        break;
    case KEY_SYSTEM_SCENE_IDLE:
        break;
    case KEY_SYSTEM_SCENE_EXIT:
        if (scene->funcs.exit != NULL) {
            scene->funcs.exit(scene->work);
        }
        scene->state = KEY_SYSTEM_SCENE_FREE;
    case KEY_SYSTEM_SCENE_FREE:
        if (scene->funcs.free != NULL) {
            scene->funcs.free(scene->work);
        }
        scene->state = KEY_SYSTEM_SCENE_NONE;
        break;
    }
}

void KeySystemScene_Start(KeySystemScene *scene, const KeySystemSceneFuncs *funcs, HeapID heapId) {
    if (scene->state == KEY_SYSTEM_SCENE_NONE) {
        scene->funcs = *funcs;
        scene->heapId = heapId;
        scene->state = KEY_SYSTEM_SCENE_INIT;
    }
}

void KeySystemScene_RequestEnd(KeySystemScene *scene) {
    if (scene->state == KEY_SYSTEM_SCENE_IDLE) {
        scene->state = KEY_SYSTEM_SCENE_EXIT;
    }
}

BOOL KeySystemScene_IsIdle(KeySystemScene *scene) {
    BOOL idle = TRUE;

    if (scene->state != KEY_SYSTEM_SCENE_NONE && scene->state != KEY_SYSTEM_SCENE_IDLE) {
        idle = FALSE;
    }
    return idle;
}

void KeySystemScene_Abort(KeySystemScene *scene) {
    if (scene->state == KEY_SYSTEM_SCENE_IDLE) {
        if (scene->funcs.exit != NULL) {
            scene->funcs.exit(scene->work);
        }
        scene->state = KEY_SYSTEM_SCENE_NONE;
    }
}

KeySystemOamText *KeySystemOamText_Create(const ClActorSetup *setup, u16 width, u16 height, u32 palette,
                                          u8 paletteOffset, u32 surface, BmpOamSys *oamSys, HeapID heapId) {
    KeySystemOamText *text = GFL_HeapAllocate(heapId, sizeof(KeySystemOamText), FALSE, "key_system_util.c", 1742);
    BmpOamActorSetup actorSetup;

    sys_memset(text, 0, sizeof(KeySystemOamText));
    text->printQueue = func_02021998(heapId);
    text->str = GFL_StrBufCreate(128, heapId);
    text->bitmap = GFL_BitmapCreate(width, height, 32, heapId);
    sys_memset(&actorSetup, 0, sizeof(BmpOamActorSetup));
    actorSetup.bitmap = text->bitmap;
    actorSetup.x = setup->x;
    actorSetup.y = setup->y;
    actorSetup.palette = palette;
    actorSetup.priority = setup->priority;
    actorSetup.surface = surface;
    actorSetup.vramType = surface;
    actorSetup.bgPriority = setup->bgPriority;
    actorSetup.paletteOffset = paletteOffset;
    text->actor = BmpOam_ActorAdd(oamSys, &actorSetup);
    return text;
}

void KeySystemOamText_Free(KeySystemOamText *text) {
    BmpOam_ActorDel(text->actor);
    GFL_BitmapFree(text->bitmap);
    GFL_StrBufFree(text->str);
    func_02021a18(text->printQueue);
    GFL_HeapFree(text);
}

void KeySystemOamText_Print(KeySystemOamText *text, MsgData *msgData, u32 msgId, Font *font) {
    KeySystemPos pos;

    GFL_BitmapFill(text->bitmap, COLOR_BACKGROUND(text->color));
    GFL_MsgDataLoadStrbuf(msgData, msgId, text->str);
    KeySystem_CalcTextPos(text->align, &text->pos, text->bitmap, text->str, font, &pos);
    func_02021c7c(text->printQueue, text->bitmap, pos.x, pos.y, text->str, font, text->color);
    text->printing = TRUE;
}

void KeySystemOamText_SetColor(KeySystemOamText *text, u16 color) {
    text->color = color;
}

void KeySystemOamText_Update(KeySystemOamText *text) {
    func_02021a3c(text->printQueue);
    if (text->printing && !func_02021c1c(text->printQueue, text->bitmap)) {
        BmpOam_ActorBmpTrans(text->actor);
        text->printing = FALSE;
    }
}

BOOL KeySystemOamText_IsDone(KeySystemOamText *text) {
    if (!text->printing) {
        return TRUE;
    }
    return FALSE;
}

BmpOamActor *KeySystemOamText_GetActor(KeySystemOamText *text) {
    return text->actor;
}

void KeySystemTween_Init(KeySystemTween *tween, const KeySystemPos *start, const KeySystemPos *end, int frames) {
    tween->pos = *start;
    tween->start = *start;
    tween->end = *end;
    tween->frames = frames;
    if (frames != 0) {
        tween->stepX = FX32_CONST(tween->end.x - tween->start.x) / frames;
        tween->stepY = FX32_CONST(tween->end.y - tween->start.y) / frames;
        tween->frame = 0;
    } else {
        tween->frame = frames - 2;
    }
}

BOOL KeySystemTween_Update(KeySystemTween *tween) {
    if (tween->frame < tween->frames - 1) {
        tween->frame++;
        tween->pos.x = tween->start.x + ((tween->stepX * tween->frame) >> FX32_SHIFT);
        tween->pos.y = tween->start.y + ((tween->stepY * tween->frame) >> FX32_SHIFT);
        return FALSE;
    }
    tween->pos = tween->end;
    return TRUE;
}

void KeySystemTween_GetPos(const KeySystemTween *tween, ClActorPos *pos) {
    pos->x = tween->pos.x;
    pos->y = tween->pos.y;
}

void KeySystemAccelMove_Init(KeySystemAccelMove *move, const KeySystemPos *start, const KeySystemPos *end, fx32 speed,
                             int frames) {
    fx32 dist;
    fx32 accel;

    VEC_Set(&move->pos, FX32_CONST(start->x), FX32_CONST(start->y), 0);
    VEC_Set(&move->start, FX32_CONST(start->x), FX32_CONST(start->y), 0);
    VEC_Set(&move->end, FX32_CONST(end->x), FX32_CONST(end->y), 0);
    dist = vecfx_dist(&move->end, &move->start);
    VEC_Subtract(&move->end, &move->start, &move->dir);
    vecfx_normalize(&move->dir, &move->dir);
    accel = FX_Div((dist - speed * frames) * 2, frames * frames * FX32_ONE);
    move->speed = speed;
    move->accel = accel;
    move->frame = 0;
    move->frames = frames;
}

BOOL KeySystemAccelMove_Update(KeySystemAccelMove *move) {
    fx32 dist;

    if (move->frame < move->frames - 1) {
        move->frame++;
        dist =
            FX_MUL(move->speed, move->frame * FX32_ONE) + FX_MUL(move->accel, move->frame * move->frame * FX32_ONE) / 2;
        vecfx_muladd(dist, &move->dir, &move->start, &move->pos);
        return FALSE;
    }
    move->pos = move->end;
    return TRUE;
}

void KeySystemAccelMove_GetPos(const KeySystemAccelMove *move, ClActorPos *pos) {
    pos->x = move->pos.x >> FX32_SHIFT;
    pos->y = move->pos.y >> FX32_SHIFT;
}

KeySystemScrollList *KeySystemScrollList_Create(const KeySystemScrollListSetup *setup, HeapID heapId) {
    KeySystemScrollList *list = GFL_HeapAllocate(heapId, sizeof(KeySystemScrollList), TRUE, "key_system_util.c", 2128);
    FrameListSetup listSetup;
    ArcTool *arc;
    u32 i;

    list->str = GFL_StrBufCreate(128, heapId);
    list->setup = *setup;
    listSetup = sScrollListSetup;
    listSetup.layout.bg = setup->bg;
    listSetup.layout.windowPalette = setup->palette;
    listSetup.layout.palette = setup->palette + 1;
    listSetup.count = setup->count;
    listSetup.cursorPos = setup->cursor;
    listSetup.visibleRows = 4;
    listSetup.scroll = 0;
    listSetup.work = list;
    listSetup.callbacks = &sScrollListCallbacks;
    list->list = FrameList_Create(&listSetup, heapId);
    FrameList_SetKeyMask(list->list, 193);
    arc = GFL_ArcSysCreateFileHandle(ARCID_KEY_SYSTEM, heapId);
    FrameList_LoadScreenPalette(list->list, arc, 12, FALSE, 0, setup->unkA8, setup->palette);
    FrameList_LoadCursorPalette(list->list, arc, 1, 1, 2);
    GFL_ArcToolFree(arc);
    for (i = 0; i < setup->count; i++) {
        FrameList_AddItem(list->list, 0, i);
    }
    return list;
}

void KeySystemScrollList_Free(KeySystemScrollList *list) {
    FrameList_Free(list->list);
    GFL_StrBufFree(list->str);
    GFL_BGSysMoveBG(list->setup.bg, 3, 0);
    GFL_HeapFree(list);
}

u32 KeySystemScrollList_Update(KeySystemScrollList *list) {
    u32 result = FrameList_Main(list->list);

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        result = list->setup.values[list->setup.count - 1];
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else if (result >= (u32)-16) {
        result = FRAMELIST_NONE;
    } else {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        result = list->setup.values[FrameList_GetSelected(list->list)];
    }
    return result;
}

BOOL KeySystemScrollList_Start(KeySystemScrollList *list) {
    if (list->started) {
        return TRUE;
    }
    if (!FrameList_Draw(list->list)) {
        FrameList_SetCursor(list->list, list->setup.cursor);
        if (FrameList_CanScrollDown(list->list)) {
            FrameList_ScrollBlocking(list->list, list->setup.unkAC);
        }
        list->started = TRUE;
        return TRUE;
    }
    return FALSE;
}

void KeySystemScrollList_GetPos(KeySystemScrollList *list, u32 *cursor, u32 *top) {
    if (cursor != NULL) {
        *cursor = FrameList_GetCursor(list->list);
    }
    if (top != NULL) {
        *top = FrameList_GetScroll(list->list);
    }
}

static void KeySystemScrollList_PrintItem(void *work, u32 index, PrintWindow *printWindow, s16 y, BOOL firstBG) {
    KeySystemScrollList *list = work;
    PrintQueue *queue = FrameList_GetPrintQueue(list->list);
    ClActorPos pos;

    GFL_MsgDataLoadStrbuf(list->setup.msgData, list->setup.msgIds[index], list->str);
    PrintWindow_Print(printWindow, queue, 0, 0, list->str, list->setup.font, PRINT_COLOR(14, 15, 1));
    if (list->setup.icons[index] != NULL) {
        pos.x = 224;
        pos.y = y + 16;
        func_0204c210(list->setup.icons[index], &pos);
        func_0204c124(list->setup.icons[index], TRUE);
    }
    if (list->setup.arrowUp != NULL) {
        if (FrameList_GetScroll(list->list) != 0) {
            func_0204c124(list->setup.arrowUp, TRUE);
        } else {
            func_0204c124(list->setup.arrowUp, FALSE);
        }
    }
    if (list->setup.arrowDown != NULL) {
        if (FrameList_CanScrollDown(list->list)) {
            func_0204c124(list->setup.arrowDown, TRUE);
        } else {
            func_0204c124(list->setup.arrowDown, FALSE);
        }
    }
}

static void KeySystemScrollList_Select(void *work, u32 index, BOOL moved) {
}

static void KeySystemScrollList_Scroll(void *work, s16 delta) {
    KeySystemScrollList *list = work;
    u32 i;
    ClActorPos pos;
    s16 top;
    int y;

    for (i = 0; i < list->setup.count; i++) {
        if (list->setup.icons[i] != NULL && func_0204c138(list->setup.icons[i])) {
            func_0204c21c(list->setup.icons[i], &pos);
            pos.y = pos.y - delta;
            func_0204c210(list->setup.icons[i], &pos);
            if (pos.y <= -12 || pos.y >= 268) {
                func_0204c124(list->setup.icons[i], FALSE);
            }
        }
    }
    top = FrameList_GetScroll(list->list);
    y = top * 5;
    if (delta < 0) {
        GFL_BGSysFillScrArea(list->setup.bg, 0, 0, y, 32, 4, BGSYS_FILL_KEEP_PALETTE);
    } else if (delta > 0 && top != 0 && FrameList_GetScrollFrames(list->list) == 1) {
        GFL_BGSysFillScrArea(list->setup.bg, 0, 0, y, 32, 4, BGSYS_FILL_KEEP_PALETTE);
    }
}

KeySystemParticle *KeySystemParticle_Create(HeapID heapId) {
    KeySystemParticle *particle = GFL_HeapAllocate(heapId, sizeof(KeySystemParticle), TRUE, "key_system_util.c", 2470);

    particle->work = GFL_HeapAllocate(heapId, PARTICLE_WORK_SIZE, FALSE, "key_system_util.c", 2471);
    return particle;
}

void KeySystemParticle_Free(KeySystemParticle *particle) {
    GFL_HeapFree(particle->work);
    GFL_HeapFree(particle);
}

void KeySystemParticle_Load(KeySystemParticle *particle, u32 arcId, u32 fileId, HeapID heapId) {
    if (particle->system != NULL) {
        func_0204fa84(particle->system);
    }
    particle->system = func_0204f968(particle->work, PARTICLE_WORK_SIZE, TRUE, heapId);
    func_0204fe04(particle->system, func_0204fdf8(arcId, fileId, heapId), TRUE, NULL);
}

void KeySystemParticle_Emit(KeySystemParticle *particle, int resourceId) {
    func_0205007c(particle->system, resourceId, NULL, particle);
}

static void KeySystem_BlendColor(u32 type, u16 *dest, u16 angle, u8 palette, u8 index, u16 from, u16 to) {
    u8 fromG = (from & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    s16 t = (FX_CosIdx(angle) + FX32_ONE) / 2;
    u8 fromR = (from & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
    u8 fromB = (from & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    u8 toB = (to & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    u8 b = fromB + ((toB - fromB) * t >> FX32_SHIFT);
    u8 toR = (to & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
    u8 r = fromR + ((toR - fromR) * t >> FX32_SHIFT);
    u8 toG = (to & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 g = fromG + ((toG - fromG) * t >> FX32_SHIFT);

    *dest = GX_RGB(r, g, b);
    NNS_GfdRegisterNewVramTransferTask(type, palette * 32 + index * 2, dest, sizeof(u16));
}

void KeySystem_BlendPalette(u32 type, u16 *dest, u16 angle, u32 palette, const u16 *from, const u16 *to) {
    int i;
    s16 t = (FX_CosIdx(angle) + FX32_ONE) / 2;

    for (i = 0; i < 16; i++) {
        u16 fromColor = from[i];
        u8 fromG = (fromColor & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u16 toColor = to[i];
        u8 fromR = (fromColor & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
        u8 fromB = (fromColor & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 toB = (toColor & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 b = fromB + ((toB - fromB) * t >> FX32_SHIFT);
        u8 toR = (toColor & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
        u8 r = fromR + ((toR - fromR) * t >> FX32_SHIFT);
        u8 toG = (toColor & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 g = fromG + ((toG - fromG) * t >> FX32_SHIFT);

        dest[i] = GX_RGB(r, g, b);
    }
    NNS_GfdRegisterNewVramTransferTask(type, palette * 32, dest, 32);
}

StrBuf *KeySystem_LoadFormattedStr(WordSet *wordSet, MsgData *msgData, u32 msgId, HeapID heapId) {
    StrBuf *src = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    StrBuf *dest = GFL_StrBufCreate(GFL_StrBufGetCharCount(src) * 2, heapId);

    GFL_WordSetFormatStrbuf(wordSet, dest, src);
    GFL_StrBufFree(src);
    return dest;
}
