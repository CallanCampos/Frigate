#include "types.h"
#include "app/mystery/mystery_util.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
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
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"

// Mystery Gift's windows, menus, sequences and palette fades. Our names; swan has none for this overlay

// A position, and how the text is placed against it
typedef struct {
    s32 x;
    s32 y;
} MysteryPos;

enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_CENTER_Y,
    ALIGN_RIGHT,
};

// A window that prints one string
struct MysteryTextLine {
    PrintWindow printWindow;
    PrintQueue *queue;
    BmpWin *window;
    StrBuf *str;
    u16 bg;
    u16 color;
    MysteryPos pos;
    u32 align;
    // The window's screen is sent to VRAM by MysteryTextLine_Flush rather than when it is made
    BOOL deferFlush;
};

struct MysteryTextWin {
    const MysteryTextWinEntry *entries;
    u32 count;
    Font *font;
    MsgData *msgData;
    MysteryTextLine *lines[0];
};

// A copy of the bitmaps of a MysteryTextWin's windows
struct MysteryTextWinCopy {
    HeapID heapId;
    MysteryTextWin *textWin;
    GFLBitmap *bitmaps[0];
};

struct MysteryMsgWin {
    u32 unk0;
    Font *font;
    PrintStream *stream;
    TCBExManager *tcbManager;
    BmpWin *window;
    StrBuf *str;
    u16 bgColor;
    // BUG: Never set, so the key cursor, the wait icon and the stream allocate from heap 0 (the system heap); with
    // BUGFIX the create functions set it
    HeapID heapId;
    PrintWindow printWindow;
    PrintQueue *queue;
    u32 mode;
    BOOL done;
    KeyCursor *keyCursor;
    AppPrintsysCommon printCommon;
    WaitIcon *waitIcon;
};

struct MysteryYesNo {
    BmpWin *window;
    PrintQueue *queue;
    PrintWindow printWindow;
    BmpMenuList *menu;
    ListMenuOption *options;
};

struct MysteryList {
    MysteryTextWin *textWin;
    ClActor *cursor;
    u32 pos;
    u32 count;
    u16 unk10;
    u16 baseColors[4];
    u16 colors[4];
    u16 grayColor;
    u16 palFrame;
    u16 palette;
    u16 bg;
    u16 unk2A;
    u16 bgPalette;
    s16 offsetY;
    u16 cursorSequence;
    void (*onMove)(void *work);
    void *work;
    BOOL grayed[4];
    BOOL grayedNext[4];
    u32 unk5C;
    u32 *cursorPos;
};

struct MysterySeq {
    MysterySeqFunc func;
    BOOL end;
    u32 state;
    void *work;
    u32 returnState;
};

struct MysteryOamText {
    GFLBitmap *bitmap;
    u16 color;
    BOOL printing;
    MysteryPos pos;
    u32 align;
    StrBuf *str;
    BmpOamActor *actor;
    PrintQueue *queue;
};

static void MysteryTextLine_ClearScreen(MysteryTextLine *line);
static void MysteryTextLine_PrintMsg(MysteryTextLine *line, MsgData *msgData, u32 msgId, Font *font);
static void MysteryTextLine_Flush(MysteryTextLine *line);
static void MysteryTextLine_GetPos(const MysteryTextLine *line, Font *font, MysteryPos *pos);
static void Mystery_AlignText(u32 align, const MysteryPos *pos, GFLBitmap *bitmap, const StrBuf *str, Font *font,
                              MysteryPos *out);
static void MysteryMsgWin_PrintStr(MysteryMsgWin *win, u32 mode);
static void MysteryMsgWin_EndWait(MysteryMsgWin *win);
static void MysteryOamText_GetPos(MysteryOamText *oamText, Font *font, MysteryPos *pos);

// The width and the x of the yes/no menu's window, in tiles. No code loads from it: its reads have constant indices
static const u8 sYesNoWindow[2] = { 12, 19 };

static const BmpMenuListHeader sYesNoHeader = {
    NULL, NULL, NULL, 0, 0, 0, 13, 0, 3, 1, 15, 2, 0, 1, 0, 0, 0, NULL, 12, 13, NULL, NULL, NULL, NULL, 20,
};

static const MysteryPos sListCursorPositions[4] = {
    { 128, 16 },
    { 128, 44 },
    { 128, 72 },
    { 128, 104 },
};

static inline void MysteryMsgWin_PrintQueue(PrintWindow *printWindow, PrintQueue *queue, const StrBuf *str,
                                            Font *font) {
    func_02021c54(queue, BmpWin_GetBitmap(printWindow->window), 0, 0, str, font);
    printWindow->flushPending = TRUE;
}

MysteryTextLine *MysteryTextLine_Create(BOOL deferFlush, u16 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                        PrintQueue *queue, HeapID heapId) {
    MysteryTextLine *line = GFL_HeapAllocate(heapId, sizeof(MysteryTextLine), FALSE, "mystery_util.c", 85);

    sys_memset(line, 0, sizeof(MysteryTextLine));
    line->queue = queue;
    line->bg = bg;
    line->color = PRINT_COLOR(1, 2, 0);
    line->deferFlush = deferFlush;
    line->str = GFL_StrBufCreate(512, heapId);
    line->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    PrintWindow_Init(&line->printWindow, line->window);
    GFL_BitmapFill(BmpWin_GetBitmap(line->window), line->color & 0x1f);
    if (!line->deferFlush) {
        BmpWin_FlushChar(line->window);
        BmpWin_FlushMap(line->window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(line->window));
    }
    return line;
}

void MysteryTextLine_Delete(MysteryTextLine *line) {
    BmpWin_Free(line->window);
    GFL_StrBufFree(line->str);
    GFL_HeapFree(line);
}

static void MysteryTextLine_ClearScreen(MysteryTextLine *line) {
    BmpWin_ClearScreen(line->window);
}

static void MysteryTextLine_PrintMsg(MysteryTextLine *line, MsgData *msgData, u32 msgId, Font *font) {
    MysteryPos pos;

    GFL_BitmapFill(BmpWin_GetBitmap(line->window), line->color & 0x1f);
    GFL_MsgDataLoadStrbuf(msgData, msgId, line->str);
    MysteryTextLine_GetPos(line, font, &pos);
    PrintWindow_Print(&line->printWindow, line->queue, pos.x, pos.y, line->str, font, line->color);
}

void MysteryTextLine_PrintStr(MysteryTextLine *line, const StrBuf *str, Font *font) {
    MysteryPos pos;

    GFL_BitmapFill(BmpWin_GetBitmap(line->window), line->color & 0x1f);
    GFL_StrBufCopy(line->str, str);
    MysteryTextLine_GetPos(line, font, &pos);
    PrintWindow_Print(&line->printWindow, line->queue, pos.x, pos.y, line->str, font, line->color);
}

void MysteryTextLine_SetColor(MysteryTextLine *line, u16 color) {
    line->color = color;
}

void MysteryTextLine_SetPos(MysteryTextLine *line, s32 x, s32 y, u32 align) {
    line->pos.x = x;
    line->pos.y = y;
    line->align = align;
}

BOOL MysteryTextLine_Update(MysteryTextLine *line) {
    if (!line->deferFlush) {
        PrintWindow_Flush(&line->printWindow, line->queue);
        if (!line->printWindow.flushPending) {
            return TRUE;
        }
        return FALSE;
    } else {
        if (line->printWindow.flushPending && !func_02021c1c(line->queue, BmpWin_GetBitmap(line->printWindow.window))) {
            line->printWindow.flushPending = FALSE;
        }
        if (!line->printWindow.flushPending) {
            return TRUE;
        }
        return FALSE;
    }
}

static void MysteryTextLine_Flush(MysteryTextLine *line) {
    BmpWin_FlushChar(line->window);
    if (line->deferFlush == TRUE) {
        BmpWin_FlushMap(line->window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(line->window));
    }
}

static void MysteryTextLine_GetPos(const MysteryTextLine *line, Font *font, MysteryPos *pos) {
    Mystery_AlignText(line->align, &line->pos, BmpWin_GetBitmap(line->window), line->str, font, pos);
}

static void Mystery_AlignText(u32 align, const MysteryPos *pos, GFLBitmap *bitmap, const StrBuf *str, Font *font,
                              MysteryPos *out) {
    s32 x;
    s32 y;
    s32 height;

    switch (align) {
    case ALIGN_LEFT:
        *out = *pos;
        break;
    case ALIGN_CENTER:
        x = GFL_BitmapGetWidth(bitmap) / 2;
        height = GFL_BitmapGetHeight(bitmap) / 2;
        x -= GFL_FontGetBlockWidth(str, font, 0) / 2;
        y = height - GFL_FontGetBlockHeight(str, font) / 2;
        out->x = x + pos->x;
        out->y = y + pos->y;
        break;
    case ALIGN_CENTER_Y:
        height = GFL_BitmapGetHeight(bitmap) / 2;
        y = height - GFL_FontGetBlockHeight(str, font) / 2;
        out->x = pos->x;
        out->y = y + pos->y;
        break;
    case ALIGN_RIGHT:
        x = GFL_BitmapGetWidth(bitmap) - GFL_FontGetBlockWidth(str, font, 0);
        if (x < 0) {
            x = 0;
        }
        out->x = x + pos->x;
        out->y = pos->y;
        break;
    }
}

MysteryMsgWin *MysteryMsgWin_Create(u16 bg, u8 palette, PrintQueue *queue, Font *font, HeapID heapId) {
    MysteryMsgWin *win = GFL_HeapAllocate(heapId, sizeof(MysteryMsgWin), FALSE, "mystery_util.c", 388);
    BmpWin *window;

    sys_memset(win, 0, sizeof(MysteryMsgWin));
    win->bgColor = 15;
    win->font = font;
#ifdef BUGFIX
    win->heapId = heapId;
#endif
    win->queue = queue;
    win->mode = MYSTERY_PRINT_NONE;
    win->str = GFL_StrBufCreate(768, heapId);
    win->window = BmpWin_CreateDynamic(bg, 1, 19, 30, 4, palette, TRUE);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->bgColor);
    window = win->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    win->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 32);
    return win;
}

MysteryMsgWin *MysteryMsgWin_CreateSmall(u16 bg, u8 palette, PrintQueue *queue, Font *font, HeapID heapId) {
    MysteryMsgWin *win = GFL_HeapAllocate(heapId, sizeof(MysteryMsgWin), FALSE, "mystery_util.c", 430);
    BmpWin *window;

    sys_memset(win, 0, sizeof(MysteryMsgWin));
    win->bgColor = 15;
    win->font = font;
#ifdef BUGFIX
    win->heapId = heapId;
#endif
    win->queue = queue;
    win->mode = MYSTERY_PRINT_NONE;
    AppPrintsysCommon_Init(&win->printCommon, 2);
    win->str = GFL_StrBufCreate(512, heapId);
    win->window = BmpWin_CreateDynamic(bg, 1, 21, 30, 2, palette, TRUE);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->bgColor);
    window = win->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    win->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 32);
    return win;
}

void MysteryMsgWin_Delete(MysteryMsgWin *win) {
    if (win->stream != NULL) {
        func_020223cc(win->stream);
        win->stream = NULL;
    }
    if (win->waitIcon != NULL) {
        WaitIcon_Free(win->waitIcon);
        win->waitIcon = NULL;
    }
    if (win->keyCursor != NULL) {
        KeyCursor_Free(win->keyCursor);
    }
    GFL_TCBExMgrFree(win->tcbManager);
    BmpWin_ClearFrame(win->window, WINFRAME_TRANSFER_NOW);
    BmpWin_Free(win->window);
    GFL_StrBufFree(win->str);
    GFL_HeapFree(win);
}

void MysteryMsgWin_Update(MysteryMsgWin *win) {
    switch (win->mode) {
    case MYSTERY_PRINT_WAIT_ICON:
        PrintWindow_Flush(&win->printWindow, win->queue);
        break;
    case MYSTERY_PRINT_QUEUE:
        PrintWindow_Flush(&win->printWindow, win->queue);
        win->done = !win->printWindow.flushPending ? TRUE : FALSE;
        break;
    case MYSTERY_PRINT_STREAM:
        if (win->stream != NULL) {
            if (win->keyCursor != NULL) {
                KeyCursor_Update(win->keyCursor, win->stream, win->window);
            }
            if (AppPrintsysCommon_Update(&win->printCommon, win->stream)) {
                win->done = TRUE;
            }
        }
        break;
    case MYSTERY_PRINT_NONE:
        break;
    }
    GFL_TCBExMgrUpdate(win->tcbManager);
}

void MysteryMsgWin_Print(MysteryMsgWin *win, MsgData *msgData, u32 msgId, u32 mode) {
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->str);
    MysteryMsgWin_PrintStr(win, mode);
}

static void MysteryMsgWin_PrintStr(MysteryMsgWin *win, u32 mode) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->bgColor);
    if (win->stream != NULL) {
        func_020223cc(win->stream);
        win->stream = NULL;
    }
    if (win->keyCursor != NULL) {
        KeyCursor_Free(win->keyCursor);
        win->keyCursor = NULL;
    }
    MysteryMsgWin_EndWait(win);
    switch (mode) {
    case MYSTERY_PRINT_WAIT_ICON:
        win->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), win->window, (u8)win->bgColor, 16, win->heapId);
        MysteryMsgWin_PrintQueue(&win->printWindow, win->queue, win->str, win->font);
        win->mode = MYSTERY_PRINT_QUEUE;
        break;
    case MYSTERY_PRINT_QUEUE:
        MysteryMsgWin_PrintQueue(&win->printWindow, win->queue, win->str, win->font);
        win->mode = MYSTERY_PRINT_QUEUE;
        break;
    case MYSTERY_PRINT_STREAM:
        AppPrintsysCommon_Init(&win->printCommon, 2);
        win->keyCursor = KeyCursor_Create(win->bgColor, TRUE, TRUE, win->heapId);
        win->stream = func_02022268(win->window, 0, 0, win->str, win->font, func_02017bcc(), win->tcbManager, 0,
                                    win->heapId, win->bgColor);
        win->mode = MYSTERY_PRINT_STREAM;
        break;
    default:
        break;
    }
    win->done = FALSE;
}

BOOL MysteryMsgWin_IsDone(MysteryMsgWin *win) {
    return win->done;
}

void MysteryMsgWin_DrawFrame(MysteryMsgWin *win, u16 frameChar, u8 framePalette) {
    BmpWin_DrawFrame(win->window, WINFRAME_TRANSFER_NOW, frameChar, framePalette);
}

static void MysteryMsgWin_EndWait(MysteryMsgWin *win) {
    win->done = TRUE;
    if (win->waitIcon != NULL) {
        WaitIcon_Free(win->waitIcon);
        win->waitIcon = NULL;
        BmpWin_FlushMap(win->window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win->window));
    }
}

MysteryYesNo *MysteryYesNo_Create(const MysteryYesNoSetup *setup, HeapID heapId) {
    u32 i = 0;
    MysteryYesNo *menu = GFL_HeapAllocate(heapId, sizeof(MysteryYesNo), FALSE, "mystery_util.c", 727);
    u8 height;
    BmpWin *window;
    BmpMenuListHeader header;

    sys_memset(menu, 0, sizeof(MysteryYesNo));
    menu->queue = setup->queue;
    height = setup->count * 2;
    menu->window = BmpWin_CreateDynamic(setup->bg, sYesNoWindow[1], 17 - height, sYesNoWindow[0], height, setup->palette, TRUE);
    BmpWin_DrawFrame(menu->window, WINFRAME_TRANSFER_NONE, setup->frameChar, setup->framePalette);
    window = menu->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    PrintWindow_Init(&menu->printWindow, menu->window);
    menu->options = ListMenuCore_CreateOptionList(setup->count, heapId);
    for (i = 0; i < setup->count; i++) {
        ListMenuCore_AppendMsgOption(menu->options, setup->msgData, setup->msgIds[i], i, heapId);
    }
    header = sYesNoHeader;
    header.options = menu->options;
    header.count = setup->count;
    header.maxShown = setup->count;
    header.msgData = setup->msgData;
    header.printWindow = &menu->printWindow;
    header.queue = setup->queue;
    header.font = setup->font;
    menu->menu = BmpMenuList_Create(&header, 0, 0, heapId);
    BmpMenuList_LoadCursor(menu->menu, heapId);
    BmpMenuList_SetCancelDisabled(menu->menu, FALSE);
    return menu;
}

void MysteryYesNo_Delete(MysteryYesNo *menu) {
    BmpMenuList_Free(menu->menu, NULL, NULL);
    ListMenuCore_FreeOptionList(menu->options);
    BmpWin_ClearFrame(menu->window, WINFRAME_TRANSFER_NOW);
    BmpWin_ClearScreen(menu->window);
    BmpWin_Free(menu->window);
    GFL_HeapFree(menu);
}

u32 MysteryYesNo_Update(MysteryYesNo *menu) {
    s32 result;

    PrintWindow_Flush(&menu->printWindow, menu->queue);
    result = BmpMenuList_Update(menu->menu);
    if (result == BMPMENULIST_CANCEL) {
        result = BmpMenuList_GetParam(menu->menu, 2) - 1;
    }
    return result;
}

MysteryTextWin *MysteryTextWin_Create(BOOL deferFlush, const MysteryTextWinEntry *entries, u32 count, u16 bg,
                                      u8 palette, PrintQueue *queue, MsgData *msgData, Font *font, HeapID heapId) {
    u32 i = 0;
    MysteryTextWin *win = GFL_HeapAllocate(heapId, sizeof(MysteryTextWin) + count * sizeof(MysteryTextLine *), FALSE,
                                           "mystery_util.c", 894);

    sys_memset(win, 0, sizeof(MysteryTextWin) + count * sizeof(MysteryTextLine *));
    win->entries = entries;
    win->count = count;
    win->font = font;
    win->msgData = msgData;
    for (i = 0; i < win->count; i++) {
        const MysteryTextWinEntry *entry = &entries[i];

        win->lines[i] = MysteryTextLine_Create(deferFlush, bg, entry->x, entry->y, entry->width, entry->height, palette,
                                               queue, heapId);
        MysteryTextLine_SetPos(win->lines[i], entry->textX, entry->textY, entry->align);
        MysteryTextLine_SetColor(win->lines[i], entry->color);
        if (entry->str != NULL) {
            MysteryTextLine_PrintStr(win->lines[i], entry->str, win->font);
        } else {
            MysteryTextLine_PrintMsg(win->lines[i], msgData, entry->msgId, win->font);
        }
    }
    return win;
}

void MysteryTextWin_Delete(MysteryTextWin *win) {
    u32 i;

    for (i = 0; i < win->count; i++) {
        MysteryTextLine_Delete(win->lines[i]);
    }
    GFL_HeapFree(win);
}

void MysteryTextWin_Clear(MysteryTextWin *win) {
    u32 i;

    for (i = 0; i < win->count; i++) {
        MysteryTextLine_ClearScreen(win->lines[i]);
    }
}

BOOL MysteryTextWin_Update(MysteryTextWin *win) {
    BOOL done = TRUE;
    u32 i;

    for (i = 0; i < win->count; i++) {
        done &= MysteryTextLine_Update(win->lines[i]);
    }
    return done;
}

void MysteryTextWin_ClearLine(MysteryTextWin *win, u32 index) {
    MysteryTextLine *line = win->lines[index];

    GFL_BitmapFill(BmpWin_GetBitmap(line->window), line->color & 0x1f);
    BmpWin_FlushChar(win->lines[index]->window);
}

void MysteryTextWin_Flush(MysteryTextWin *win) {
    u32 i;

    for (i = 0; i < win->count; i++) {
        MysteryTextLine_Flush(win->lines[i]);
    }
}

MysteryTextWinCopy *MysteryTextWinCopy_Create(MysteryTextWin *textWin, HeapID heapId) {
    u32 i = 0;
    u32 size = sizeof(MysteryTextWinCopy) + textWin->count * sizeof(GFLBitmap *);
    MysteryTextWinCopy *copy = GFL_HeapAllocate(heapId, size, FALSE, "mystery_util.c", 1080);
    GFLBitmap *bitmap;

    sys_memset(copy, 0, size);
    copy->textWin = textWin;
    copy->heapId = heapId;
    for (i = 0; i < textWin->count; i++) {
        bitmap = BmpWin_GetBitmap(textWin->lines[i]->window);
        copy->bitmaps[i] = GFL_BitmapCreate(GFL_BitmapGetWidth(bitmap) / 8, GFL_BitmapGetHeight(bitmap) / 8,
                                            GFL_BitmapGetBytesPerTile(bitmap), heapId);
    }
    return copy;
}

void MysteryTextWinCopy_Delete(MysteryTextWinCopy *copy) {
    u32 i;

    for (i = 0; i < copy->textWin->count; i++) {
        GFL_BitmapFree(copy->bitmaps[i]);
    }
    GFL_HeapFree(copy);
}

void MysteryTextWinCopy_Print(MysteryTextWinCopy *copy, const MysteryTextWinUpdate *updates, PrintQueue *queue) {
    StrBuf *str = GFL_StrBufCreate(512, copy->heapId);
    u32 i;

    for (i = 0; i < copy->textWin->count; i++) {
        const MysteryTextWinUpdate *update = &updates[i];

        if (updates[i].update) {
            MysteryPos textPos;
            MysteryPos pos;

            GFL_BitmapFill(copy->bitmaps[i], update->color & 0x1f);
            textPos.x = update->textX;
            textPos.y = update->textY;
            Mystery_AlignText(update->align, &textPos, copy->bitmaps[i], str, copy->textWin->font, &pos);
            if (update->str != NULL) {
                GFL_StrBufCopy(str, update->str);
            } else {
                GFL_MsgDataLoadStrbuf(copy->textWin->msgData, update->msgId, str);
            }
            func_02021c7c(queue, copy->bitmaps[i], pos.x, pos.y, str, copy->textWin->font, update->color);
        }
    }
    GFL_HeapFree(str);
}

void MysteryTextWinCopy_Apply(MysteryTextWinCopy *copy) {
    u32 i;

    for (i = 0; i < copy->textWin->count; i++) {
        GFLBitmap *bitmap = copy->bitmaps[i];

        GFL_BitmapCopy(bitmap, BmpWin_GetBitmap(copy->textWin->lines[i]->window));
    }
}

MysteryList *MysteryList_Create(const MysteryListSetup *setup, HeapID heapId) {
    MysteryList *list = GFL_HeapAllocate(heapId, sizeof(MysteryList), FALSE, "mystery_util.c", 1265);
    u32 i;
    ClActorPos pos;

    sys_memset(list, 0, sizeof(MysteryList));
    list->count = setup->count;
    list->cursor = setup->cursor;
    list->palette = setup->palette;
    list->unk2A = setup->unk28;
    list->bgPalette = setup->bgPalette;
    list->bg = setup->bg;
    list->onMove = setup->onMove;
    list->work = setup->work;
    list->offsetY = setup->offsetY;
    list->cursorSequence = setup->cursorSequence;
    list->cursorPos = setup->cursorPos;
    if (setup->cursorPos != NULL) {
        list->pos = *setup->cursorPos;
        list->pos = list->pos > list->count - 1 ? list->count - 1 : list->pos;
    }
    for (i = 0; i < list->count; i++) {
        list->baseColors[i] = *(u16 *)(HW_BG_PLTT + setup->bgPalette * 32 + (i + 10) * 2);
    }
    list->grayColor = *(u16 *)(HW_BG_PLTT + setup->bgPalette * 32 + 15 * 2);
    {
        MysteryTextWinEntry entries[4] = {
            { 5, 2, 27, 2, 0, NULL, ALIGN_CENTER_Y, 0, 0, PRINT_COLOR(15, 2, 0) },
            { 5, 5, 27, 3, 0, NULL, ALIGN_CENTER_Y, 0, 0, PRINT_COLOR(15, 2, 0) },
            { 5, 9, 27, 2, 0, NULL, ALIGN_CENTER_Y, 0, 0, PRINT_COLOR(15, 2, 0) },
            { 5, 13, 5, 2, 0, NULL, ALIGN_CENTER_Y, 0, 0, PRINT_COLOR(15, 2, 0) },
        };

        for (i = 0; i < list->count; i++) {
            MysteryTextWinEntry *entry = &entries[i];

            entry->y += setup->offsetY;
            entries[i].x += setup->offsetX;
            if (setup->msgData != NULL) {
                entry->str = GFL_MsgDataLoadStrbufNew(setup->msgData, setup->items[i]);
            } else {
                entry->str = GFL_StrBufClone((StrBuf *)setup->items[i], HEAPID_TAIL(heapId));
            }
        }
        list->textWin = MysteryTextWin_Create(FALSE, entries, list->count, setup->bg, setup->palette, setup->queue,
                                              setup->msgData, setup->font, heapId);
        for (i = 0; i < list->count; i++) {
            if (entries[i].str != NULL) {
                GFL_StrBufFree(entries[i].str);
            }
        }
    }
    pos.x = sListCursorPositions[list->pos].x;
    pos.y = sListCursorPositions[list->pos].y + setup->offsetY * 8;
    func_0204c140(list->cursor, &pos, 0);
    func_0204c124(list->cursor, TRUE);
    func_0204c488(list->cursor, list->cursorSequence);
    func_0204c520(list->cursor, TRUE);
    return list;
}

void MysteryList_Delete(MysteryList *list) {
    if (list->cursorPos != NULL && list->pos != 3) {
        *list->cursorPos = list->pos;
    }
    func_0204c124(list->cursor, FALSE);
    MysteryTextWin_Clear(list->textWin);
    MysteryTextWin_Delete(list->textWin);
    GFL_BGSysLoadScr(list->bg);
    GFL_HeapFree(list);
}

u32 MysteryList_Update(MysteryList *list) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();
    BOOL moved = FALSE;
    ClActorPos pos;
    int i;

    if (typed & PAD_KEY_UP) {
        if (list->pos == 0) {
            list->pos = list->count - 1;
        } else {
            list->pos = list->pos - 1;
        }
        if (list->count != 1) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        moved = TRUE;
    } else if (typed & PAD_KEY_DOWN) {
        if (list->count != 1) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        list->pos++;
        list->pos %= list->count;
        moved = TRUE;
    } else if (pressed & PAD_BUTTON_A) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return list->pos;
    } else if (pressed & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (list->count == 1) {
            return MYSTERY_MENU_CANCEL;
        }
        if (list->pos != 3) {
            *list->cursorPos = list->pos;
        }
        list->pos = list->count - 1;
        return list->pos;
    }
    if (moved) {
        pos.x = sListCursorPositions[list->pos].x;
        pos.y = sListCursorPositions[list->pos].y + list->offsetY * 8;
        func_0204c140(list->cursor, &pos, 0);
        if (list->pos == 3) {
            func_0204c488(list->cursor, 2);
        } else {
            func_0204c488(list->cursor, list->cursorSequence);
        }
        if (list->onMove != NULL) {
            list->onMove(list->work);
        }
    }
    if (list->palFrame + 0x400 >= 0x10000) {
        list->palFrame = list->palFrame + 0x400 - 0x10000;
    } else {
        list->palFrame += 0x400;
    }
    for (i = 0; i < 4; i++) {
        if (list->grayedNext[i] != list->grayed[i] && list->palFrame >= 0x7dff && list->palFrame <= 0x81ff) {
            list->grayed[i] = list->grayedNext[i];
        }
        if (list->grayed[i]) {
            MysteryPal_BlendOne(15, &list->colors[i], list->palFrame, list->bgPalette, i + 10, list->baseColors[i],
                                list->grayColor);
        }
    }
    return MYSTERY_MENU_NONE;
}

void MysteryList_UpdatePrint(MysteryList *list) {
    MysteryTextWin_Update(list->textWin);
}

void MysteryList_SetItemGrayed(MysteryList *list, u32 item, BOOL grayed) {
    list->grayedNext[item] = grayed;
}

MysterySeq *MysterySeq_Create(void *work, MysterySeqFunc func, HeapID heapId) {
    MysterySeq *seq = GFL_HeapAllocate(heapId, sizeof(MysterySeq), FALSE, "mystery_util.c", 1574);

    sys_memset(seq, 0, sizeof(MysterySeq));
    seq->work = work;
    MysterySeq_SetNext(seq, func);
    return seq;
}

void MysterySeq_Delete(MysterySeq *seq) {
    GFL_HeapFree(seq);
}

void MysterySeq_Main(MysterySeq *seq) {
    if (!seq->end) {
        seq->func(seq, &seq->state, seq->work);
    }
}

BOOL MysterySeq_IsEnd(MysterySeq *seq) {
    return seq->end;
}

void MysterySeq_SetNext(MysterySeq *seq, MysterySeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

void MysterySeq_End(MysterySeq *seq) {
    seq->end = TRUE;
}

void MysterySeq_SetReturn(MysterySeq *seq, u32 state) {
    seq->returnState = state;
}

void MysterySeq_Return(MysterySeq *seq) {
    seq->state = seq->returnState;
}

MysteryOamText *MysteryOamText_Create(const ClActorSetup *setup, u16 width, u16 height, u32 palette, u8 paletteOffset,
                                      u32 surface, BmpOamSys *bmpOam, PrintQueue *queue, HeapID heapId) {
    MysteryOamText *oamText = GFL_HeapAllocate(heapId, sizeof(MysteryOamText), FALSE, "mystery_util.c", 1723);
    BmpOamActorSetup actorSetup;

    sys_memset(oamText, 0, sizeof(MysteryOamText));
    oamText->queue = queue;
    oamText->str = GFL_StrBufCreate(128, heapId);
    oamText->bitmap = GFL_BitmapCreate(width, height, 32, heapId);
    sys_memset(&actorSetup, 0, sizeof(BmpOamActorSetup));
    actorSetup.bitmap = oamText->bitmap;
    actorSetup.x = setup->x;
    actorSetup.y = setup->y;
    actorSetup.palette = palette;
    actorSetup.priority = setup->priority;
    actorSetup.surface = surface;
    actorSetup.vramType = surface;
    actorSetup.bgPriority = setup->bgPriority;
    actorSetup.paletteOffset = paletteOffset;
    oamText->actor = BmpOam_ActorAdd(bmpOam, &actorSetup);
    return oamText;
}

void MysteryOamText_Delete(MysteryOamText *oamText) {
    BmpOam_ActorDel(oamText->actor);
    GFL_BitmapFree(oamText->bitmap);
    GFL_StrBufFree(oamText->str);
    GFL_HeapFree(oamText);
}

void MysteryOamText_Clear(MysteryOamText *oamText) {
    GFL_BitmapFill(oamText->bitmap, oamText->color & 0x1f);
    BmpOam_ActorBmpTrans(oamText->actor);
}

void MysteryOamText_Print(MysteryOamText *oamText, MsgData *msgData, u32 msgId, Font *font) {
    MysteryPos pos;

    GFL_BitmapFill(oamText->bitmap, oamText->color & 0x1f);
    GFL_MsgDataLoadStrbuf(msgData, msgId, oamText->str);
    MysteryOamText_GetPos(oamText, font, &pos);
    func_02021c7c(oamText->queue, oamText->bitmap, pos.x, pos.y, oamText->str, font, oamText->color);
    oamText->printing = TRUE;
}

void MysteryOamText_SetColor(MysteryOamText *oamText, u16 color) {
    oamText->color = color;
}

void MysteryOamText_SetAlign(MysteryOamText *oamText, s32 x, s32 y, u32 align) {
    oamText->pos.x = x;
    oamText->pos.y = y;
    oamText->align = align;
}

BOOL MysteryOamText_Update(MysteryOamText *oamText) {
    if (oamText->printing && !func_02021c1c(oamText->queue, oamText->bitmap)) {
        BmpOam_ActorBmpTrans(oamText->actor);
        oamText->printing = FALSE;
    }
    if (!oamText->printing) {
        return TRUE;
    }
    return FALSE;
}

static void MysteryOamText_GetPos(MysteryOamText *oamText, Font *font, MysteryPos *pos) {
    u32 x;
    u32 y;

    switch (oamText->align) {
    case ALIGN_LEFT:
        *pos = oamText->pos;
        break;
    case ALIGN_CENTER:
        x = GFL_BitmapGetWidth(oamText->bitmap) / 2;
        y = GFL_BitmapGetHeight(oamText->bitmap) / 2;
        x -= (u32)GFL_FontGetBlockWidth(oamText->str, font, 0) / 2;
        y -= GFL_FontGetBlockHeight(oamText->str, font) / 2;
        pos->x = x + oamText->pos.x;
        pos->y = y + oamText->pos.y;
        break;
    case ALIGN_CENTER_Y:
        y = GFL_BitmapGetHeight(oamText->bitmap) / 2;
        y -= GFL_FontGetBlockHeight(oamText->str, font) / 2;
        pos->x = oamText->pos.x;
        pos->y = y + oamText->pos.y;
        break;
    }
}

void MysteryPal_BlendOne(u32 type, u16 *dest, u16 angle, u8 palette, u8 index, u16 from, u16 to) {
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

void MysteryPal_Blend(u32 type, u16 *dest, u16 angle, u32 palette, const u16 *from, const u16 *to) {
    int i;

    for (i = 0; i < 16; i++) {
        fx16 cosine = FX_CosIdx(angle);
        s16 t = (cosine + FX32_ONE) / 2;
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
