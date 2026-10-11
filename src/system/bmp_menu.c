#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "system/bmp_cursor.h"
#include "system/bmp_menu.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// Menus of options in columns, with a cursor, and the yes/no menu made from one

// The yes/no menu's messages
#define MSG_YESNO TEXT_BANK_YES_NO
#define MSG_YESNO_YES 0
#define MSG_YESNO_NO 1

// Frames before the yes/no menu's cursor appears, halved when the game runs at 30 frames a second
#define YESNO_WAIT 20

static BmpMenu *BmpMenu_AddEx(const BmpMenuHeader *header, u8 x, u8 y, u8 cursorPos, HeapID heapId, u32 cancelKeys);
static BmpMenu *BmpMenu_Add(const BmpMenuHeader *header, u8 x, u8 y, u8 cursorPos, HeapID heapId, u32 cancelKeys);
static void BmpMenu_Exit(BmpMenu *menu, u8 *cursorPos);
static s32 shopDecision(BmpMenu *menu);
static BOOL BmpMenu_MoveCursor(BmpMenu *menu, u8 dir, u16 se);
static BOOL BmpMenu_NextCursorPos(BmpMenu *menu, u8 dir);
static u8 BmpMenu_GetMaxWidth(BmpMenu *menu);
static void BmpMenu_PrintOptions(BmpMenu *menu);
static void BmpMenu_DrawCursor(BmpMenu *menu);
static void BmpMenu_GetCursorXY(BmpMenu *menu, u8 *x, u8 *y, u8 pos);

static BmpMenu *BmpMenu_AddEx(const BmpMenuHeader *header, u8 x, u8 y, u8 cursorPos, HeapID heapId, u32 cancelKeys) {
    BmpMenu *menu = GFL_HeapAllocate(heapId, sizeof(BmpMenu), TRUE, "bmp_menu.c", 96);

    menu->header = *header;
    menu->cursor = BmpCursor_Create(heapId);
    menu->cancelKeys = cancelKeys;
    menu->cursorPos = cursorPos;
    menu->maxWidth = BmpMenu_GetMaxWidth(menu);
    menu->heapId = heapId;
    menu->x = x;
    menu->y = y;
    BmpCursor_LoadBitmap(menu->cursor, heapId);
    menu->fontSizeX = menu->header.fontSizeX;
    menu->fontSizeY = menu->header.fontSizeY;
    BmpMenu_PrintOptions(menu);
    return menu;
}

static BmpMenu *BmpMenu_Add(const BmpMenuHeader *header, u8 x, u8 y, u8 cursorPos, HeapID heapId, u32 cancelKeys) {
    return BmpMenu_AddEx(header, x, y, cursorPos, heapId, cancelKeys);
}

static void BmpMenu_Exit(BmpMenu *menu, u8 *cursorPos) {
    if (cursorPos != NULL) {
        *cursorPos = menu->cursorPos;
    }
    BmpCursor_Free(menu->cursor);
    GFL_HeapFree(menu);
}

static s32 shopDecision(BmpMenu *menu) {
    u32 keys = GCTX_HIDGetPressedKeys();

    menu->moveDir = 0;
    if (keys & PAD_BUTTON_A) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return menu->header.options[menu->cursorPos].value;
    }
    if (keys & menu->cancelKeys) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return BMPMENU_CANCEL;
    }
    if (keys & PAD_KEY_UP) {
        if (BmpMenu_MoveCursor(menu, BMPMENU_MOVE_UP, SEQ_SE_SELECT1) == TRUE) {
            menu->moveDir = BMPMENU_MOVE_UP + 1;
        }
        return BMPMENU_NULL;
    }
    if (keys & PAD_KEY_DOWN) {
        if (BmpMenu_MoveCursor(menu, BMPMENU_MOVE_DOWN, SEQ_SE_SELECT1) == TRUE) {
            menu->moveDir = BMPMENU_MOVE_DOWN + 1;
        }
        return BMPMENU_NULL;
    }
    if (keys & PAD_KEY_LEFT) {
        if (BmpMenu_MoveCursor(menu, BMPMENU_MOVE_LEFT, SEQ_SE_SELECT1) == TRUE) {
            menu->moveDir = BMPMENU_MOVE_LEFT + 1;
        }
        return BMPMENU_NULL;
    }
    if (keys & PAD_KEY_RIGHT) {
        if (BmpMenu_MoveCursor(menu, BMPMENU_MOVE_RIGHT, SEQ_SE_SELECT1) == TRUE) {
            menu->moveDir = BMPMENU_MOVE_RIGHT + 1;
        }
        return BMPMENU_NULL;
    }
    return BMPMENU_NULL;
}

// Moves the cursor a step in dir, erasing it where it was and drawing it again; FALSE if it can't move
static BOOL BmpMenu_MoveCursor(BmpMenu *menu, u8 dir, u16 se) {
    u8 oldPos = menu->cursorPos;
    u8 x, y;

    if (BmpMenu_NextCursorPos(menu, dir) == FALSE) {
        return FALSE;
    }
    BmpMenu_GetCursorXY(menu, &x, &y, oldPos);
    GFL_BitmapFillArea(BmpWin_GetBitmap(menu->header.printWindow->window), x, y, menu->fontSizeX, menu->fontSizeY,
                       0xff);
    BmpMenu_DrawCursor(menu);
    GFL_SndSEPlay(se);
    return TRUE;
}

// Moves the cursor's position a step in dir, wrapping around if the menu loops,
// but never onto a BMPMENU_DUMMY option
static BOOL BmpMenu_NextCursorPos(BmpMenu *menu, u8 dir) {
    s8 pos;

    if (dir == BMPMENU_MOVE_UP) {
        if (menu->header.yCount <= 1) {
            return FALSE;
        }
        if (menu->cursorPos % menu->header.yCount == 0) {
            if (menu->header.loop == FALSE) {
                return FALSE;
            }
            pos = menu->cursorPos + (menu->header.yCount - 1);
        } else {
            pos = menu->cursorPos - 1;
        }
    } else if (dir == BMPMENU_MOVE_DOWN) {
        if (menu->header.yCount <= 1) {
            return FALSE;
        }
        if (menu->cursorPos % menu->header.yCount == menu->header.yCount - 1) {
            if (menu->header.loop == FALSE) {
                return FALSE;
            }
            pos = menu->cursorPos - (menu->header.yCount - 1);
        } else {
            pos = menu->cursorPos + 1;
        }
    } else if (dir == BMPMENU_MOVE_LEFT) {
        if (menu->header.xCount <= 1) {
            return FALSE;
        }
        if (menu->cursorPos < menu->header.yCount) {
            if (menu->header.loop == FALSE) {
                return FALSE;
            }
            pos = menu->cursorPos + menu->header.yCount * (menu->header.xCount - 1);
        } else {
            pos = menu->cursorPos - menu->header.yCount;
        }
    } else {
        if (menu->header.xCount <= 1) {
            return FALSE;
        }
        if (menu->cursorPos >= menu->header.yCount * (menu->header.xCount - 1)) {
            if (menu->header.loop == FALSE) {
                return FALSE;
            }
            pos = menu->cursorPos % menu->header.yCount;
        } else {
            pos = menu->cursorPos + menu->header.yCount;
        }
    }
    if (menu->header.options[pos].value == BMPMENU_DUMMY) {
        return FALSE;
    }
    menu->cursorPos = pos;
    return TRUE;
}

static u8 BmpMenu_GetMaxWidth(BmpMenu *menu) {
    u8 maxWidth = 0;
    u8 i;

    for (i = 0; i < menu->header.xCount * menu->header.yCount; i++) {
        u8 width = GFL_FontGetBlockWidth(menu->header.options[i].text, menu->header.font, 0);

        if (maxWidth < width) {
            maxWidth = width;
        }
    }
    return maxWidth;
}

static void BmpMenu_PrintOptions(BmpMenu *menu) {
    u8 x, column, row, columnWidth;

    GFL_BitmapFill(BmpWin_GetBitmap(menu->header.printWindow->window), 0xff);
    x = menu->x;
    columnWidth = menu->maxWidth + menu->fontSizeX * 2;
    for (column = 0; column < menu->header.xCount; column++) {
        for (row = 0; row < menu->header.yCount; row++) {
            PrintWindow *printWindow = menu->header.printWindow;
            PrintQueue *queue = menu->header.queue;
            u8 y = menu->y + (menu->fontSizeY + menu->header.lineSpacing) * row;

            func_02021c54(queue, BmpWin_GetBitmap(printWindow->window), x, y,
                          menu->header.options[row + column * menu->header.yCount].text, menu->header.font);
            printWindow->flushPending = TRUE;
        }
        x += columnWidth;
    }
}

static void BmpMenu_DrawCursor(BmpMenu *menu) {
    u8 x, y;

    if (menu->header.cursorDisplay != BMPMENU_CURSOR_HIDE) {
        BmpMenu_GetCursorXY(menu, &x, &y, menu->cursorPos);
        BmpCursor_Print(menu->cursor, x, y, menu->header.printWindow, menu->header.queue, menu->header.font);
    }
}

static void BmpMenu_GetCursorXY(BmpMenu *menu, u8 *x, u8 *y, u8 pos) {
    *x = (pos / menu->header.yCount) * (menu->maxWidth + menu->fontSizeX * 2);
    *y = menu->y + (pos % menu->header.yCount) * (menu->fontSizeY + menu->header.lineSpacing);
}

BmpMenu *ShopUI_CreateConfirmDialog(const ConfirmDialogSetup *setup, u16 frameChar, u8 framePalette, u8 cursorPos,
                                    HeapID heapId) {
    BmpMenuHeader header;
    MsgData *msgData;
    ListMenuOption *options;
    BmpWin *window;
    BmpMenu *menu;

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_YESNO, heapId);
    options = ListMenuCore_CreateOptionList(2, heapId);
    ListMenuCore_AppendMsgOption(options, msgData, MSG_YESNO_YES, 0, heapId);
    ListMenuCore_AppendMsgOption(options, msgData, MSG_YESNO_NO, BMPMENU_CANCEL, heapId);
    GFL_MsgDataFree(msgData);

    sys_memset(&header, 0, sizeof(header));
    header.options = options;
    window = BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, 7, 4, setup->palette, TRUE);
    header.xCount = 1;
    header.yCount = 2;
    header.lineSpacing = 1;
    header.cursorDisplay = BMPMENU_CURSOR_SHOW;
    header.loop = FALSE;
    header.printWindow = GFL_HeapAllocate(heapId, sizeof(PrintWindow), TRUE, "bmp_menu.c", 611);
    header.wait = YESNO_WAIT;
    header.font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, heapId);
    header.fontSizeY = GFL_FontGetCharHeight(header.font);
    header.fontSizeX = 10;
    PrintWindow_Init(header.printWindow, window);
    header.queue = func_02021998(heapId);
    header.printed = FALSE;

    menu = BmpMenu_Add(&header, 14, 0, cursorPos, heapId, PAD_BUTTON_B);
    if (header.wait == 0) {
        menu->wait = 2;
    } else {
        menu->wait = YESNO_WAIT;
        if (GCTX_HIDGetUpdateRate() == 30) {
            menu->wait /= 2;
        }
    }
    BmpWin_FlushMap(header.printWindow->window);
    BmpWin_DrawFrame(header.printWindow->window, WINFRAME_TRANSFER_NOW, frameChar, framePalette);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(header.printWindow->window));
    BmpWin_FlushChar(header.printWindow->window);
    return menu;
}

u32 ConfirmDialog_Update(BmpMenu *menu) {
    u32 result;

    if (menu->wait != 0) {
        menu->wait--;
        if (menu->wait == 0) {
            BmpMenu_DrawCursor(menu);
            BmpWin_FlushChar(menu->header.printWindow->window);
        }
        result = BMPMENU_NULL;
    } else {
        result = shopDecision(menu);
    }
    ConfirmDialog_IsPrinted(menu);
    if (result != BMPMENU_NULL) {
        ConfirmDialog_Free(menu);
    }
    return result;
}

BOOL ConfirmDialog_IsPrinted(BmpMenu *menu) {
    BOOL done = func_02021a3c(menu->header.queue);

    if (!menu->header.printed && done) {
        BmpWin_FlushChar(menu->header.printWindow->window);
        menu->header.printed = TRUE;
    }
    return menu->header.printed;
}

void ConfirmDialog_Free(BmpMenu *menu) {
    BmpWin *window = menu->header.printWindow->window;

    func_02021c44(menu->header.queue);
    func_02021a18(menu->header.queue);
    GFL_HeapFree(menu->header.printWindow);
    BmpWin_ClearFrame(window, WINFRAME_TRANSFER_NOW);
    GFL_FontFree(menu->header.font);
    ListMenuCore_FreeOptionList(menu->header.options);
    BmpWin_Free(window);
    BmpMenu_Exit(menu, NULL);
}
