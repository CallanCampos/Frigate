#include "types.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"

// The menus' task menus: a column of buttons, one window each, picked with the keys or the touch screen, and single
// buttons of the same look

#define DEFAULT_WIDTH 13
#define DEFAULT_HEIGHT 3

// BGs 0 to 3 are the main screen's
#define MAIN_BG_LAST 3

// The frame's characters and palettes in the menus' common archive
#define FRAME_CHAR_FILE 0x20
#define FRAME_PLTT_FILE 0x1f

// The frame's tiles: the corners and edges, then three tiles for the icon of each item type from 1
#define FRAME_TILE_SIZE 0x20
#define FRAME_TILE_TOP_LEFT 0
#define FRAME_TILE_TOP 1
#define FRAME_TILE_TOP_RIGHT 2
#define FRAME_TILE_LEFT 3
#define FRAME_TILE_CENTER 4
#define FRAME_TILE_RIGHT 5
#define FRAME_TILE_BOTTOM_LEFT 6
#define FRAME_TILE_BOTTOM 7
#define FRAME_TILE_BOTTOM_RIGHT 8
#define FRAME_TILE_ICON(type) (((type) - 1) * 3 + 9)

// The color of a button's palette that pulses
#define PULSE_COLOR_OFFSET (6 * sizeof(GXRgb))

// Where a button's text starts
#define TEXT_X 10
#define TEXT_Y 6

struct AppTaskMenuRes {
    u16 bg;
    u16 palette;
    NNSG2dCharacterData *character;
    void *characterFile;
    u32 unkC;
    Font *font;
    PrintQueue *queue;
};

struct AppTaskMenu {
    u8 cursorPos;
    u8 flashCount;
    u16 pulseAngle;
    GXRgb pulseColor;
    BOOL isUpdatingText;
    BOOL isDecided;
    BOOL isLocked;
    u32 flashSpeed;
    BmpWin **windows;
    AppTaskMenuItem *items;
    AppTaskMenuInit init;
    AppTaskMenuRes *res;
};

struct AppTaskMenuWin {
    BmpWin *window;
    u32 unk4;
    BOOL isFlashing;
    BOOL isUpdatingText;
    u16 flashCount;
    u16 flashSpeed;
    u16 pulseAngle;
    GXRgb pulseColor;
    AppTaskMenuRes *res;
    GXRgb color1;
    GXRgb color2;
};

static void AppTaskMenu_CreateWindows(AppTaskMenu *menu, AppTaskMenuRes *res);
static void AppTaskMenu_SetWindowActive(BmpWin *window, AppTaskMenuRes *res, BOOL active);
static void AppTaskMenu_PulseCursorColor(u16 *angle, GXRgb *color, u8 bg, u8 palette);
static void AppTaskMenuWin_PulseColor(u16 *angle, GXRgb *color, u8 bg, u8 palette, GXRgb color1, GXRgb color2);
static void AppTaskMenu_UpdateKeys(AppTaskMenu *menu);
static void AppTaskMenu_UpdateTouch(AppTaskMenu *menu);
static AppTaskMenuWin *AppTaskMenuWin_CreateCore(AppTaskMenuRes *res, const AppTaskMenuItem *item, u8 x, u8 y, u8 width,
                                                 u8 height, u32 fastFlash, BOOL centered, HeapID heapId);
static void AppTaskMenuWin_FreeCore(AppTaskMenuWin *win);
static void AppTaskMenu_DrawFrame(BmpWin *window, AppTaskMenuRes *res, u32 type);

AppTaskMenu *AppTaskMenu_Create(AppTaskMenuInit *init, AppTaskMenuRes *res) {
    AppTaskMenu *menu = GFL_HeapAllocate(init->heapId, sizeof(AppTaskMenu), FALSE, "app_taskmenu.c", 162);

    menu->init = *init;
    menu->res = res;
    menu->windows =
        GFL_HeapAllocate(menu->init.heapId, menu->init.itemCount * sizeof(BmpWin *), FALSE, "app_taskmenu.c", 168);
    menu->items = GFL_HeapAllocate(menu->init.heapId, menu->init.itemCount * sizeof(AppTaskMenuItem), FALSE,
                                   "app_taskmenu.c", 169);
    MIi_CpuCopy16(menu->init.items, menu->items, menu->init.itemCount * sizeof(AppTaskMenuItem));
    menu->isDecided = FALSE;
    menu->isLocked = FALSE;
    menu->cursorPos = 0;
    menu->flashCount = 0;
    menu->pulseAngle = 0;
    menu->flashSpeed = 1;
    menu->init.width = init->width == 0 ? DEFAULT_WIDTH : init->width;
    menu->init.height = init->height == 0 ? DEFAULT_HEIGHT : init->height;

    AppTaskMenu_CreateWindows(menu, res);
    if (func_0203d554() == FALSE) {
        AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, TRUE);
    }
    menu->isUpdatingText = TRUE;
    return menu;
}

AppTaskMenu *AppTaskMenu_CreateFastFlash(AppTaskMenuInit *init, AppTaskMenuRes *res) {
    AppTaskMenu *menu = AppTaskMenu_Create(init, res);
    menu->flashSpeed = 2;
    return menu;
}

void AppTaskMenu_Free(AppTaskMenu *menu) {
    u8 bg = BmpWin_GetBGIndex(menu->windows[0]);
    u8 i;

    for (i = 0; i < menu->init.itemCount; i++) {
        while (PrintQueue_IsBitmapPending(menu->res->queue, BmpWin_GetBitmap(menu->windows[i]))) {
            PrintQueue_Main(menu->res->queue);
        }
    }
    for (i = 0; i < menu->init.itemCount; i++) {
        BmpWin_ClearScreen(menu->windows[i]);
        BmpWin_Free(menu->windows[i]);
    }
    GFL_BGSysQueueScrLoad(bg);
    GFL_HeapFree(menu->windows);
    GFL_HeapFree(menu->items);
    GFL_HeapFree(menu);
}

void AppTaskMenu_UpdateText(AppTaskMenu *menu) {
    if (menu->isUpdatingText == TRUE) {
        BOOL printed = TRUE;
        u8 i;

        for (i = 0; i < menu->init.itemCount; i++) {
            if (PrintQueue_IsBitmapPending(menu->res->queue, BmpWin_GetBitmap(menu->windows[i])) == TRUE) {
                printed = FALSE;
            }
        }
        if (printed == TRUE) {
            for (i = 0; i < menu->init.itemCount; i++) {
                BmpWin_Transfer(menu->windows[i]);
            }
            menu->isUpdatingText = FALSE;
        }
    }
}

void AppTaskMenu_Update(AppTaskMenu *menu) {
    AppTaskMenu_UpdateText(menu);
    if (menu->isDecided == FALSE) {
        AppTaskMenu_UpdateKeys(menu);
        if (menu->isDecided == FALSE) {
            AppTaskMenu_UpdateTouch(menu);
        }
    } else {
        if ((u8)(menu->flashCount / (4 / menu->flashSpeed) % 2) == 0) {
            AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, TRUE);
        } else {
            AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, FALSE);
        }
        menu->flashCount++;
    }
    AppTaskMenu_PulseCursorColor(&menu->pulseAngle, &menu->pulseColor, menu->res->bg, menu->res->palette);
}

BOOL AppTaskMenu_IsFlashFinished(AppTaskMenu *menu) {
    if (menu->flashCount >= 16 / menu->flashSpeed) {
        return TRUE;
    }
    return FALSE;
}

u8 AppTaskMenu_GetCursorPos(AppTaskMenu *menu) {
    return menu->cursorPos;
}

void AppTaskMenu_SetCursorActive(AppTaskMenu *menu, BOOL active) {
    AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, active);
}

BOOL AppTaskMenu_IsDecided(AppTaskMenu *menu) {
    return menu->isDecided;
}

static void AppTaskMenu_CreateWindows(AppTaskMenu *menu, AppTaskMenuRes *res) {
    u8 i;
    u8 x;
    u8 y;

    if (menu->init.posType == APP_TASKMENU_POS_TOP_LEFT) {
        x = menu->init.x;
        y = menu->init.y;
    } else {
        x = menu->init.x - menu->init.width;
        y = menu->init.y - menu->init.height * menu->init.itemCount;
    }

    for (i = 0; i < menu->init.itemCount; i++) {
        menu->windows[i] = BmpWin_CreateDynamic(menu->res->bg, x, y + i * menu->init.height, menu->init.width,
                                                menu->init.height, menu->res->palette + 1, TRUE);
        AppTaskMenu_DrawFrame(menu->windows[i], res, menu->items[i].type);
        PrintQueue_PrintColor(res->queue, BmpWin_GetBitmap(menu->windows[i]), TEXT_X, TEXT_Y, menu->items[i].str,
                              res->font, menu->items[i].color);
        BmpWin_Transfer(menu->windows[i]);
    }
}

static void AppTaskMenu_SetWindowActive(BmpWin *window, AppTaskMenuRes *res, BOOL active) {
    if (active == TRUE) {
        BmpWin_SetPalette(window, res->palette);
    } else if (active == FALSE) {
        BmpWin_SetPalette(window, res->palette + 1);
    }
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

static void AppTaskMenu_PulseCursorColor(u16 *angle, GXRgb *color, u8 bg, u8 palette) {
    fx16 t;
    u8 r;
    u8 g;
    u8 b;

    if (*angle + 0x400 >= 0x10000) {
        *angle = *angle - 0x10000 + 0x400;
    } else {
        *angle += 0x400;
    }

    t = (FX_CosIdx(*angle) + FX16_ONE) / 2;
    r = 5 + (t * 7 >> FX32_SHIFT);
    g = 10 + (t * 15 >> FX32_SHIFT);
    b = 13 + (t * 17 >> FX32_SHIFT);
    *color = GX_RGB(r, g, b);

    if (bg <= MAIN_BG_LAST) {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, palette * 0x20 + PULSE_COLOR_OFFSET, color,
                                           sizeof(GXRgb));
    } else {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, palette * 0x20 + PULSE_COLOR_OFFSET, color,
                                           sizeof(GXRgb));
    }
}

static void AppTaskMenuWin_PulseColor(u16 *angle, GXRgb *color, u8 bg, u8 palette, GXRgb color1, GXRgb color2) {
    fx16 t;
    u8 r1 = (color1 & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
    u8 g1 = (color1 & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 b1 = (color1 & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    u8 r2 = (color2 & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
    u8 g2 = (color2 & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 b2 = (color2 & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    u8 r;
    u8 g;
    u8 b;

    if (*angle + 0x400 >= 0x10000) {
        *angle = *angle - 0x10000 + 0x400;
    } else {
        *angle += 0x400;
    }

    t = (FX_CosIdx(*angle) + FX16_ONE) / 2;
    r = r1 + ((r2 - r1) * t >> FX32_SHIFT);
    g = g1 + ((g2 - g1) * t >> FX32_SHIFT);
    b = b1 + ((b2 - b1) * t >> FX32_SHIFT);
    *color = GX_RGB(r, g, b);

    if (bg <= MAIN_BG_LAST) {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, palette * 0x20 + PULSE_COLOR_OFFSET, color,
                                           sizeof(GXRgb));
    } else {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, palette * 0x20 + PULSE_COLOR_OFFSET, color,
                                           sizeof(GXRgb));
    }
}

void AppTaskMenu_SetLocked(AppTaskMenu *menu, BOOL locked) {
    menu->isLocked = locked;
    if (locked == TRUE) {
        func_0203d564(TRUE);
        AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, FALSE);
    }
}

void AppTaskMenu_SetCursorPos(AppTaskMenu *menu, u32 pos) {
    int i;

    menu->cursorPos = pos;
    if (func_0203d554() == FALSE) {
        for (i = 0; i < menu->init.itemCount; i++) {
            if (i == pos) {
                AppTaskMenu_SetWindowActive(menu->windows[i], menu->res, TRUE);
            } else {
                AppTaskMenu_SetWindowActive(menu->windows[i], menu->res, FALSE);
            }
        }
    }
}

static void AppTaskMenu_UpdateKeys(AppTaskMenu *menu) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();
    u8 oldPos;

    if (menu->isLocked == TRUE) {
        return;
    }

    // In touch mode the first key, other than B, only brings the cursor back
    if (func_0203d554() == TRUE && (pressed & PAD_BUTTON_B) == 0) {
        if (pressed & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
            AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, TRUE);
            func_0203d564(FALSE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return;
    }

    oldPos = menu->cursorPos;
    if (typed & PAD_KEY_UP) {
        menu->cursorPos = oldPos == 0 ? menu->init.itemCount - 1 : oldPos - 1;
        menu->pulseAngle = 0;
        if (menu->init.itemCount > 1) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    } else if (typed & PAD_KEY_DOWN) {
        menu->cursorPos = oldPos >= menu->init.itemCount - 1 ? 0 : oldPos + 1;
        menu->pulseAngle = 0;
        if (menu->init.itemCount > 1) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    } else if (pressed & PAD_BUTTON_A) {
        menu->isDecided = TRUE;
        if (menu->items[oldPos].type == APP_TASKMENU_ITEM_RETURN) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    } else if (pressed & PAD_BUTTON_B) {
        // B picks the last item
        menu->cursorPos = menu->init.itemCount - 1;
        menu->isDecided = TRUE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_0203d564(FALSE);
    }

    if (oldPos != menu->cursorPos) {
        AppTaskMenu_SetWindowActive(menu->windows[oldPos], menu->res, FALSE);
        AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, TRUE);
    }
}

static void AppTaskMenu_UpdateTouch(AppTaskMenu *menu) {
    u8 x;
    u8 y;
    u8 i;
    TouchRect rects[APP_TASKMENU_ITEM_MAX + 1];
    s32 hit;

    if (menu->init.posType == APP_TASKMENU_POS_TOP_LEFT) {
        x = menu->init.x;
        y = menu->init.y;
    } else {
        x = menu->init.x - menu->init.width;
        y = menu->init.y - menu->init.height * menu->init.itemCount;
    }

    for (i = 0; i < menu->init.itemCount; i++) {
        rects[i].top = y * 8 + i * (menu->init.height * 8);
        rects[i].bottom = y * 8 + (i + 1) * (menu->init.height * 8);
        rects[i].left = x * 8;
        rects[i].right = (x + menu->init.width) * 8 - 1;
    }
    rects[i].top = TOUCH_RECT_END;

    hit = func_0203da0c(rects);
    if (hit != TOUCH_RECT_NONE) {
        func_0203d564(TRUE);
        AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, FALSE);
        menu->cursorPos = hit;
        menu->isDecided = TRUE;
        AppTaskMenu_SetWindowActive(menu->windows[menu->cursorPos], menu->res, TRUE);
        if (menu->items[menu->cursorPos].type == APP_TASKMENU_ITEM_RETURN) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
}

AppTaskMenuRes *AppTaskMenuRes_Create(u8 bg, u8 palette, Font *font, PrintQueue *queue, HeapID heapId) {
    AppTaskMenuRes *res = GFL_HeapAllocate(heapId, sizeof(AppTaskMenuRes), FALSE, "app_taskmenu.c", 716);
    u32 palType = PALTYPE_MAIN_BG;

    MI_CpuFill8(res, 0, sizeof(AppTaskMenuRes));
    res->bg = bg;
    res->palette = palette;
    res->font = font;
    res->queue = queue;
    if (bg > MAIN_BG_LAST) {
        palType = PALTYPE_SUB_BG;
    }

    res->characterFile = GFL_G2DIOReadBGNCGR(getUINarcIdx(), FRAME_CHAR_FILE, FALSE, &res->character, heapId);
    GFL_BGSysLoadNCLRDefault(getUINarcIdx(), FRAME_PLTT_FILE, palType, palette * 0x20, 2 * 0x20, heapId);
    return res;
}

void AppTaskMenuRes_Free(AppTaskMenuRes *res) {
    GFL_HeapFree(res->characterFile);
    GFL_HeapFree(res);
}

AppTaskMenuWin *AppTaskMenuWin_Create(AppTaskMenuRes *res, const AppTaskMenuItem *item, u8 x, u8 y, u8 width,
                                      HeapID heapId) {
    return AppTaskMenuWin_CreateEx(res, item, x, y, width, DEFAULT_HEIGHT, FALSE, FALSE, heapId);
}

AppTaskMenuWin *AppTaskMenuWin_CreateEx(AppTaskMenuRes *res, const AppTaskMenuItem *item, u8 x, u8 y, u8 width,
                                        u8 height, u32 fastFlash, BOOL centered, HeapID heapId) {
    AppTaskMenuWin *win = AppTaskMenuWin_CreateCore(res, item, x, y, width, height, fastFlash, centered, heapId);
    BmpWin_Transfer(win->window);
    return win;
}

AppTaskMenuWin *AppTaskMenuWin_CreateExNoChar(AppTaskMenuRes *res, const AppTaskMenuItem *item, u8 x, u8 y, u8 width,
                                              u8 height, u32 fastFlash, BOOL centered, HeapID heapId) {
    AppTaskMenuWin *win = AppTaskMenuWin_CreateCore(res, item, x, y, width, height, fastFlash, centered, heapId);
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win->window));
    return win;
}

static AppTaskMenuWin *AppTaskMenuWin_CreateCore(AppTaskMenuRes *res, const AppTaskMenuItem *item, u8 x, u8 y, u8 width,
                                                 u8 height, u32 fastFlash, BOOL centered, HeapID heapId) {
    u8 textX = 2;
    AppTaskMenuWin *win = GFL_HeapAllocate(heapId, sizeof(AppTaskMenuWin), FALSE, "app_taskmenu.c", 829);

    MI_CpuFill8(win, 0, sizeof(AppTaskMenuWin));
    win->res = res;
    win->isUpdatingText = TRUE;
    win->flashSpeed = fastFlash + 1;
    win->window = BmpWin_CreateDynamic(res->bg, x, y, width, height, res->palette + 1, TRUE);
    AppTaskMenu_DrawFrame(win->window, res, item->type);
    if (centered == TRUE) {
        textX = (width * 8 - GFL_FontGetBlockWidth(item->str, res->font, 0) - 16) / 2;
    }
    PrintQueue_PrintColor(res->queue, BmpWin_GetBitmap(win->window), textX + 8, TEXT_Y, item->str, res->font,
                          item->color);
    win->color1 = GX_RGB(5, 10, 13);
    win->color2 = GX_RGB(12, 25, 30);
    return win;
}

void AppTaskMenuWin_Free(AppTaskMenuWin *win) {
    BmpWin_ClearScreen(win->window);
    AppTaskMenuWin_FreeCore(win);
}

void AppTaskMenuWin_FreeKeepScreen(AppTaskMenuWin *win) {
    AppTaskMenuWin_FreeCore(win);
}

static void AppTaskMenuWin_FreeCore(AppTaskMenuWin *win) {
    BmpWin_Free(win->window);
    GFL_HeapFree(win);
}

void AppTaskMenuWin_Update(AppTaskMenuWin *win) {
    if (win->isUpdatingText) {
        if (PrintQueue_IsBitmapPending(win->res->queue, BmpWin_GetBitmap(win->window)) == FALSE) {
            BmpWin_Transfer(win->window);
            win->isUpdatingText = FALSE;
        }
    }
    if (win->isFlashing) {
        if ((u8)(win->flashCount / (4 / win->flashSpeed) % 2) == 0) {
            AppTaskMenu_SetWindowActive(win->window, win->res, TRUE);
        } else {
            AppTaskMenu_SetWindowActive(win->window, win->res, FALSE);
        }
        win->flashCount++;
    }
    AppTaskMenuWin_PulseColor(&win->pulseAngle, &win->pulseColor, win->res->bg, win->res->palette, win->color1,
                              win->color2);
}

void AppTaskMenuWin_SetActive(AppTaskMenuWin *win, BOOL active) {
    win->pulseAngle = 0;
    AppTaskMenu_SetWindowActive(win->window, win->res, active);
}

void AppTaskMenuWin_SetFlashing(AppTaskMenuWin *win, BOOL flashing) {
    win->isFlashing = flashing;
}

BOOL AppTaskMenuWin_IsFlashing(AppTaskMenuWin *win) {
    return win->isFlashing;
}

BOOL AppTaskMenuWin_IsFlashFinished(AppTaskMenuWin *win) {
    if (win->flashCount >= 16 / win->flashSpeed) {
        return TRUE;
    }
    return FALSE;
}

BOOL AppTaskMenuWin_IsTouched(AppTaskMenuWin *win) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y)) {
        u32 left = BmpWin_GetPosX(win->window) * 8;
        u32 top = BmpWin_GetPosY(win->window) * 8;
        u32 right = left + BmpWin_GetSizeX(win->window) * 8;
        // BUG: The bottom is found from the width, so a button wider than it is tall can be touched below it
#ifdef BUGFIX
        u32 bottom = top + BmpWin_GetSizeY(win->window) * 8;
#else
        u32 bottom = top + BmpWin_GetSizeX(win->window) * 8;
#endif
        // Unsigned, so a point left of or above the button is out of range too
        if (x - left <= right - left && y - top <= bottom - top) {
            return TRUE;
        }
    }
    return FALSE;
}

void AppTaskMenuWin_ResetFlash(AppTaskMenuWin *win) {
    win->isFlashing = FALSE;
    win->flashCount = 0;
}

void AppTaskMenuWin_SetPalette(AppTaskMenuWin *win, AppTaskMenuRes *res, u16 palette, u16 color1, u16 color2) {
    res->palette = palette;
    win->color1 = color1;
    win->color2 = color2;
    BmpWin_SetPalette(win->window, palette + 1);
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win->window));
}

static void AppTaskMenu_DrawFrame(BmpWin *window, AppTaskMenuRes *res, u32 type) {
    u8 x;
    u8 y;
    u32 offset;
    u8 *pixels;
    u8 *tile;

    for (y = 0; y < BmpWin_GetSizeY(window); y++) {
        for (x = 0; x < BmpWin_GetSizeX(window); x++) {
            pixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(window));
            offset = (x + BmpWin_GetSizeX(window) * y) * FRAME_TILE_SIZE;

            if (y == 0 && x == 0) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_TOP_LEFT * FRAME_TILE_SIZE;
            } else if (y == 0 && x == BmpWin_GetSizeX(window) - 1) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_TOP_RIGHT * FRAME_TILE_SIZE;
            } else if (y == 0) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_TOP * FRAME_TILE_SIZE;
            } else if (y == BmpWin_GetSizeY(window) - 1 && x == 0) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_BOTTOM_LEFT * FRAME_TILE_SIZE;
            } else if (y == BmpWin_GetSizeY(window) - 1 && x == BmpWin_GetSizeX(window) - 1) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_BOTTOM_RIGHT * FRAME_TILE_SIZE;
            } else if (y == BmpWin_GetSizeY(window) - 1) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_BOTTOM * FRAME_TILE_SIZE;
            } else if (type != 0 && x == BmpWin_GetSizeX(window) - 3) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_ICON(type) * FRAME_TILE_SIZE;
            } else if (type != 0 && x == BmpWin_GetSizeX(window) - 2) {
                tile = (u8 *)res->character->rawData + (FRAME_TILE_ICON(type) + 1) * FRAME_TILE_SIZE;
            } else if (x == 0) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_LEFT * FRAME_TILE_SIZE;
            } else if (x == BmpWin_GetSizeX(window) - 1) {
                tile = (u8 *)res->character->rawData + FRAME_TILE_RIGHT * FRAME_TILE_SIZE;
            } else {
                tile = (u8 *)res->character->rawData + FRAME_TILE_CENTER * FRAME_TILE_SIZE;
            }
            MIi_CpuCopy32(tile, pixels + offset, FRAME_TILE_SIZE);
        }
    }
}

void AppTaskMenuWin_ClearScreen(AppTaskMenuWin *win) {
    BmpWin_ClearScreen(win->window);
    GFL_BGSysQueueScrLoad(win->res->bg);
}

void AppTaskMenuWin_FlushMap(AppTaskMenuWin *win) {
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(win->res->bg);
}

BOOL AppTaskMenuWin_IsUpdatingText(AppTaskMenuWin *win) {
    return win->isUpdatingText;
}
