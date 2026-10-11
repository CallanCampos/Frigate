#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "p_status_local.h"
#include "pml/poke_party.h"

// The summary screen's ribbon page: a scrolling list of the Pokémon's ribbons, each a row drawn into a bitmap shown
// by sprites, and the name, description and icon of the ribbon picked. The names of the fields and functions are
// guesses

#define ROW_COUNT 10
#define ROWS_SHOWN 6
#define ROW_HEIGHT 24
#define ROW_X 8
#define ROW_NONE 0xff
// No entry has the cursor
#define CURSOR_NONE 0xff
// The list's area: the first row's y, the right edge and the bottom
#define LIST_TOP 8
#define LIST_RIGHT 144
#define LIST_BOTTOM 168
// The lowest y the cursor's row is scrolled to
#define CURSOR_Y_MAX 128
// The message of the first category's heading, followed by the others in RIBBON_CATEGORY_ order
#define MSG_CATEGORY_FIRST 0xa0
#define ENTRY_NONE 0xffff
// The size of a row's characters, 17x3 tiles
#define ROW_CHAR_SIZE (17 * 3 * 32)

// A row of the list, which shows an entry
typedef struct {
    u16 entry;
    u32 unk4;
    // Never set, and the work isn't cleared when it is allocated, so the check of it reads what the heap held
    BOOL isPrinting;
    GFLBitmap *bitmap;
    PStaOamActor *oam;
} RibbonRow;

// A ribbon the Pokémon has, its category and its number among them
typedef struct {
    BOOL isValid;
    u8 ribbon;
    u8 category;
    u8 number;
} RibbonEntry;

struct PStaRibbonWork {
    BOOL isShown;
    BOOL isDetailShown;
    BOOL isDetailPending;
    BOOL needsRedraw;
    u8 ribbonCount;
    u32 scrollY;
    u32 iconChars;
    ClActor *icon;
    BmpWin *nameWindow;
    BmpWin *descWindow;
    PStaScreen screens[4];
    NNSG2dCharacterData *rowChars;
    void *rowCharsFile;
    MsgData *msgData;
    ClActor *cursor;
    u8 cursorRow;
    u8 cursorEntry;
    u8 prevCursorRow;
    int scrollSpeed;
    u8 seWait;
    BOOL isDragging;
    BOOL cursorMoved;
    PStaOam *oam;
    RibbonRow rows[ROW_COUNT];
    RibbonEntry entries[RIBBON_COUNT];
};

static void PStaRibbon_HandleInput(PStatusWork *wk, PStaRibbonWork *ribbon);
static BOOL PStaRibbon_HandleDetailKeys(PStatusWork *wk, PStaRibbonWork *ribbon);
static void PStaRibbon_HandleDetailTouch(PStatusWork *wk, PStaRibbonWork *ribbon);
static s32 PStaRibbon_GetTouchedRow(PStatusWork *wk, PStaRibbonWork *ribbon);
static void PStaRibbon_SelectVisibleRow(PStatusWork *wk, PStaRibbonWork *ribbon);
static void PStaRibbon_MoveCursor(PStatusWork *wk, PStaRibbonWork *ribbon, u8 row);
static void PStaRibbon_Scroll(PStatusWork *wk, PStaRibbonWork *ribbon, s16 dy);
static void PStaRibbon_DrawRow(PStatusWork *wk, PStaRibbonWork *ribbon, RibbonRow *row);
static void PStaRibbon_Update(PStatusWork *wk, PStaRibbonWork *ribbon);
static u8 PStaRibbon_GetRowIndex(PStaRibbonWork *ribbon, u8 row);
static u32 PStaRibbon_GetRowEntry(PStaRibbonWork *ribbon, u8 row);
static s16 PStaRibbon_GetRowY(PStaRibbonWork *ribbon, u8 row);
static void PStaRibbon_PrintDetail(PStatusWork *wk, PStaRibbonWork *ribbon);
static void PStaRibbon_ShowDetail(PStatusWork *wk, PStaRibbonWork *ribbon);
static void PStaRibbon_FreeDetailWindows(PStatusWork *wk, PStaRibbonWork *ribbon);
static void PStaRibbon_HideDetail(PStatusWork *wk, PStaRibbonWork *ribbon);

PStaRibbonWork *PStaRibbon_Create(PStatusWork *wk) {
    PStaRibbonWork *ribbon = GFL_HeapAllocate(wk->heapId, sizeof(PStaRibbonWork), FALSE, "p_sta_ribbon.c", 175);

    ribbon->isShown = FALSE;
    ribbon->seWait = 0;
    ribbon->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_RIBBON_NAMES, wk->heapId);
    return ribbon;
}

void PStaRibbon_Free(PStatusWork *wk, PStaRibbonWork *ribbon) {
    GFL_MsgDataFree(ribbon->msgData);
    GFL_HeapFree(ribbon);
}

void PStaRibbon_Main(PStatusWork *wk, PStaRibbonWork *ribbon) {
    PStaRibbon_HandleInput(wk, ribbon);
    if (ribbon->isDetailPending == TRUE) {
        if (func_02021c0c(wk->printQueue) == TRUE) {
            PStaRibbon_HideDetail(wk, ribbon);
            PStaRibbon_ShowDetail(wk, ribbon);
            ribbon->isDetailPending = FALSE;
        }
    } else {
        PStaRibbon_Update(wk, ribbon);
    }
    if (ribbon->seWait != 0) {
        ribbon->seWait--;
    }
}

void PStaRibbon_LoadResources(PStatusWork *wk, PStaRibbonWork *ribbon, ArcTool *arc) {
    ribbon->screens[0].file = GFL_G2DIOReadNSCRArc(arc, 68, FALSE, &ribbon->screens[0].screen, wk->heapId);
    ribbon->screens[1].file = GFL_G2DIOReadNSCRArc(arc, 69, FALSE, &ribbon->screens[1].screen, wk->heapId);
    ribbon->screens[2].file = GFL_G2DIOReadNSCRArc(arc, 70, FALSE, &ribbon->screens[2].screen, wk->heapId);
    ribbon->screens[3].file = GFL_G2DIOReadNSCRArc(arc, 71, FALSE, &ribbon->screens[3].screen, wk->heapId);
    ribbon->rowCharsFile = GFL_G2DIOReadOBJNCGRArc(arc, 14, FALSE, &ribbon->rowChars, wk->heapId);
    ribbon->oam = PStaOam_Create(wk->heapId, wk->actorUnit);
}

void PStaRibbon_FreeResources(PStatusWork *wk, PStaRibbonWork *ribbon) {
    if (ribbon->isShown == TRUE) {
        PStaRibbon_Unload(wk, ribbon);
    }
    PStaOam_Free(ribbon->oam);
    GFL_HeapFree(ribbon->rowCharsFile);
    GFL_HeapFree(ribbon->screens[0].file);
    GFL_HeapFree(ribbon->screens[1].file);
    GFL_HeapFree(ribbon->screens[2].file);
    GFL_HeapFree(ribbon->screens[3].file);
}

void PStaRibbon_CreateActors(PStatusWork *wk, PStaRibbonWork *ribbon) {
    ClActorSetup setup;
    PStaOamSetup oamSetup;
    u8 i;
    u8 *vram;

    setup.x = 0;
    setup.y = 0;
    setup.priority = 10;
    setup.bgPriority = 0;
    setup.sequence = 0;
    ribbon->cursor =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(12)], wk->clResources[PSTA_RES_PLTT(11)],
                      wk->clResources[PSTA_RES_CELL(12)], &setup, 0, wk->heapId);
    func_0204c124(ribbon->cursor, FALSE);
    oamSetup.x = ROW_X;
    oamSetup.palette = wk->clResources[PSTA_RES_PLTT(8)];
    oamSetup.paletteOffset = 0;
    oamSetup.priority = 0;
    oamSetup.bgPriority = 2;
    oamSetup.surface = 0;
    oamSetup.vramType = 0;
    vram = (u8 *)G2_GetOBJCharPtr() + 0x20000;
    for (i = 0; i < ROW_COUNT; i++) {
        oamSetup.y = PStaRibbon_GetRowY(ribbon, i);
        ribbon->rows[i].bitmap = GFL_BitmapWrapVRAM(vram - (i + 1) * ROW_CHAR_SIZE, 17, 3, 32, wk->heapId);
        oamSetup.bitmap = ribbon->rows[i].bitmap;
        ribbon->rows[i].oam = PStaOam_CreateActor(ribbon->oam, &oamSetup);
        PStaOam_SetVisible(ribbon->rows[i].oam, FALSE);
    }
}

void PStaRibbon_FreeActors(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 i;

    func_0204c108(ribbon->cursor);
    for (i = 0; i < ROW_COUNT; i++) {
        PStaOam_FreeActor(ribbon->rows[i].oam);
        GFL_BitmapFree(ribbon->rows[i].bitmap);
    }
}

static void PStaRibbon_HandleInput(PStatusWork *wk, PStaRibbonWork *ribbon) {
    s32 hit;

    if (wk->isInputEnabled == TRUE) {
        if (ribbon->ribbonCount == 0) {
            return;
        }
        hit = PStaRibbon_GetTouchedRow(wk, ribbon);
        if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A || hit != TOUCH_RECT_NONE) {
            ribbon->scrollSpeed = 0;
            PStatus_EnableInput(wk, FALSE);
            if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
                PStaRibbon_SelectVisibleRow(wk, ribbon);
                wk->isTouch = FALSE;
            } else {
                ribbon->cursorRow = hit;
                ribbon->isDragging = TRUE;
                ribbon->cursorMoved = TRUE;
                ribbon->cursorEntry = PStaRibbon_GetRowEntry(ribbon, hit);
                PStatus_EnableInput(wk, FALSE);
                wk->isTouch = TRUE;
            }
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PStaRibbon_FreeDetailWindows(wk, ribbon);
            PStaRibbon_PrintDetail(wk, ribbon);
        }
    } else if (ribbon->isDetailPending == FALSE && PStaRibbon_HandleDetailKeys(wk, ribbon) == FALSE) {
        PStaRibbon_HandleDetailTouch(wk, ribbon);
    }
}

static BOOL PStaRibbon_HandleDetailKeys(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 i;
    s16 y;
    u8 row;

    if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_B) {
        PStatus_EnableInput(wk, TRUE);
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        func_0204c124(ribbon->cursor, FALSE);
        ribbon->cursorRow = ROW_NONE;
        ribbon->cursorEntry = CURSOR_NONE;
        ribbon->cursorMoved = TRUE;
        PStaRibbon_FreeDetailWindows(wk, ribbon);
        PStaRibbon_HideDetail(wk, ribbon);
        wk->isTouch = FALSE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return TRUE;
    }
    if (wk->isTouch == TRUE) {
        if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A || GCTX_HIDGetPressedKeys() == PAD_KEY_DOWN ||
            GCTX_HIDGetPressedKeys() == PAD_KEY_UP) {
            y = -16;
            ribbon->scrollSpeed = 0;
            ribbon->isDragging = FALSE;
            for (i = 0; i < ROW_COUNT; i++) {
                if (ribbon->cursorEntry == ribbon->rows[i].entry) {
                    y = PStaRibbon_GetRowY(ribbon, i);
                }
            }
            if (y < -4 || y > LIST_BOTTOM - ROW_HEIGHT) {
                PStaRibbon_SelectVisibleRow(wk, ribbon);
            }
            PStaRibbon_FreeDetailWindows(wk, ribbon);
            PStaRibbon_PrintDetail(wk, ribbon);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->isTouch = FALSE;
            return TRUE;
        }
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_DOWN) {
        row = ribbon->cursorRow + 1;
        if (row >= ROW_COUNT) {
            row -= ROW_COUNT;
        }
        if (ribbon->rows[row].entry != ENTRY_NONE) {
            ribbon->cursorRow = row;
            ribbon->cursorMoved = TRUE;
            if (PStaRibbon_GetRowY(ribbon, ribbon->cursorRow) > CURSOR_Y_MAX) {
                // The distance is narrowed to a u8 going down, but not going up
                PStaRibbon_Scroll(wk, ribbon, (u8)(PStaRibbon_GetRowY(ribbon, ribbon->cursorRow) - CURSOR_Y_MAX));
            }
            PStaRibbon_MoveCursor(wk, ribbon, ribbon->cursorRow);
            PStaRibbon_FreeDetailWindows(wk, ribbon);
            PStaRibbon_PrintDetail(wk, ribbon);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return TRUE;
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_UP) {
        if (ribbon->cursorRow == 0) {
            row = ROW_COUNT - 1;
        } else {
            row = ribbon->cursorRow - 1;
        }
        if (ribbon->rows[row].entry != ENTRY_NONE) {
            ribbon->cursorRow = row;
            ribbon->cursorMoved = TRUE;
            if (PStaRibbon_GetRowY(ribbon, ribbon->cursorRow) < LIST_TOP) {
                PStaRibbon_Scroll(wk, ribbon, PStaRibbon_GetRowY(ribbon, ribbon->cursorRow) - LIST_TOP);
            }
            PStaRibbon_MoveCursor(wk, ribbon, ribbon->cursorRow);
            PStaRibbon_FreeDetailWindows(wk, ribbon);
            PStaRibbon_PrintDetail(wk, ribbon);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return TRUE;
    }
    return FALSE;
}

static void PStaRibbon_HandleDetailTouch(PStatusWork *wk, PStaRibbonWork *ribbon) {
    s32 hit;
    s16 dy;
    int speed;

    if (wk->touchHit == PSTA_BUTTON_BACK) {
        PStatus_EnableInput(wk, TRUE);
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        func_0204c124(ribbon->cursor, FALSE);
        ribbon->cursorRow = ROW_NONE;
        ribbon->cursorEntry = CURSOR_NONE;
        ribbon->cursorMoved = TRUE;
        wk->isTouch = TRUE;
        PStaRibbon_FreeDetailWindows(wk, ribbon);
        PStaRibbon_HideDetail(wk, ribbon);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return;
    }
    if (func_0203da48() == TRUE && wk->touchX > ROW_X && wk->touchX < LIST_RIGHT) {
        hit = PStaRibbon_GetTouchedRow(wk, ribbon);
        if (hit != TOUCH_RECT_NONE) {
            ribbon->cursorRow = hit;
            ribbon->cursorMoved = TRUE;
            ribbon->cursorEntry = PStaRibbon_GetRowEntry(ribbon, hit);
            ribbon->isDragging = TRUE;
            ribbon->scrollSpeed = 0;
            func_0204c124(ribbon->cursor, FALSE);
            PStaRibbon_FreeDetailWindows(wk, ribbon);
            PStaRibbon_PrintDetail(wk, ribbon);
        }
        wk->isTouch = TRUE;
    } else if (func_0203da2c() == TRUE && ribbon->isDragging == TRUE && wk->touchX > ROW_X && wk->touchX < LIST_RIGHT) {
        dy = wk->prevTouchY - wk->touchY;
        speed = dy * 2;
        PStaRibbon_Scroll(wk, ribbon, dy);
        if (speed >= 0 && ribbon->scrollSpeed > 0 && speed < ribbon->scrollSpeed) {
            ribbon->scrollSpeed = (ribbon->scrollSpeed + speed) / 2;
        } else if (speed <= 0 && ribbon->scrollSpeed < 0 && speed > ribbon->scrollSpeed) {
            ribbon->scrollSpeed = (ribbon->scrollSpeed + speed) / 2;
        } else {
            ribbon->scrollSpeed = speed;
        }
    } else {
        ribbon->isDragging = FALSE;
        if (ribbon->scrollSpeed < 0) {
            ribbon->scrollSpeed++;
            PStaRibbon_Scroll(wk, ribbon, ribbon->scrollSpeed / 2);
        }
        if (ribbon->scrollSpeed > 0) {
            ribbon->scrollSpeed--;
            PStaRibbon_Scroll(wk, ribbon, ribbon->scrollSpeed / 2);
        }
    }
}

static s32 PStaRibbon_GetTouchedRow(PStatusWork *wk, PStaRibbonWork *ribbon) {
    TouchRect rects[ROW_COUNT + 1];
    TouchRect *rect;
    u8 i;
    int y;

    for (i = 0; i < ROW_COUNT; i++) {
        y = PStaRibbon_GetRowY(ribbon, i);
        rect = &rects[i];
        rect->left = ROW_X;
        rect->right = LIST_RIGHT;
        if (y < 0) {
            rect->top = 0;
        } else if (y > LIST_BOTTOM) {
            rect->top = LIST_BOTTOM;
        } else {
            rect->top = y;
        }
        y += ROW_HEIGHT;
        if (y < 0) {
            rect->bottom = 0;
        } else if (y > LIST_BOTTOM) {
            rect->bottom = LIST_BOTTOM;
        } else {
            rect->bottom = y;
        }
        if (rect->top == 0 && rect->bottom == 0) {
            rect->top = TOUCH_RECT_SKIP;
        }
        if (PStaRibbon_GetRowEntry(ribbon, i) == ENTRY_NONE) {
            rect->top = TOUCH_RECT_SKIP;
        }
    }
    rects[ROW_COUNT].top = TOUCH_RECT_END;
    return func_0203da0c(rects);
}

static void PStaRibbon_SelectVisibleRow(PStatusWork *wk, PStaRibbonWork *ribbon) {
    PStaRibbon_MoveCursor(wk, ribbon, (ribbon->scrollY + ROW_HEIGHT - 1) / ROW_HEIGHT % ROW_COUNT);
}

static void PStaRibbon_MoveCursor(PStatusWork *wk, PStaRibbonWork *ribbon, u8 row) {
    ClActorPos pos;

    pos.x = ROW_X;
    pos.y = PStaRibbon_GetRowY(ribbon, row);
    func_0204c140(ribbon->cursor, &pos, 0);
    ribbon->cursorRow = row;
    ribbon->cursorMoved = TRUE;
    ribbon->cursorEntry = PStaRibbon_GetRowEntry(ribbon, row);
}

static void PStaRibbon_Scroll(PStatusWork *wk, PStaRibbonWork *ribbon, s16 dy) {
    int max = (ribbon->ribbonCount - ROWS_SHOWN) * ROW_HEIGHT;

    if (max < 0) {
        max = 0;
    }
    if (dy > ROW_HEIGHT) {
        dy = ROW_HEIGHT;
    }
    if (dy < -ROW_HEIGHT) {
        dy = -ROW_HEIGHT;
    }
    if ((s16)ribbon->scrollY + dy < 0) {
        ribbon->scrollY = 0;
    } else if (ribbon->scrollY + dy > max) {
        ribbon->scrollY = max;
    } else {
        ribbon->scrollY = ribbon->scrollY + dy;
    }
    if (dy != 0) {
        ribbon->needsRedraw = TRUE;
    }
}

void PStaRibbon_Load(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 i;

    ribbon->cursorRow = ROW_NONE;
    ribbon->cursorEntry = CURSOR_NONE;
    ribbon->prevCursorRow = ROW_NONE;
    ribbon->scrollY = 0;
    ribbon->isShown = TRUE;
    ribbon->isDetailShown = FALSE;
    ribbon->isDetailPending = FALSE;
    ribbon->needsRedraw = TRUE;
    ribbon->nameWindow = NULL;
    ribbon->descWindow = NULL;
    for (i = 0; i < ROW_COUNT; i++) {
        ribbon->rows[i].entry = PStaRibbon_GetRowEntry(ribbon, i);
        PStaRibbon_DrawRow(wk, ribbon, &ribbon->rows[i]);
    }
}

void PStaRibbon_Draw(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 i;
    u16 entry;

    GFL_BGSysLoadScrArea(2, 0, 0, 32, 24, ribbon->screens[0].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysBufferScrDefault(6, ribbon->screens[1].screen->rawData, ribbon->screens[1].screen->size);
    GFL_BGSysQueueScrLoad(6);
    GFL_BGSysLoadScrArea(5, 0, 0, 32, 3, ribbon->screens[3].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(5);
    for (i = 0; i < ROW_COUNT; i++) {
        entry = ribbon->rows[i].entry;
        PStaOam_SetPosition(ribbon->rows[i].oam, ROW_X, PStaRibbon_GetRowY(ribbon, i));
        if (entry != ENTRY_NONE) {
            PStaOam_SetVisible(ribbon->rows[i].oam, TRUE);
            PStaOam_Upload(ribbon->rows[i].oam);
        } else {
            PStaOam_SetVisible(ribbon->rows[i].oam, FALSE);
        }
    }
    ribbon->scrollY = 0;
    ribbon->cursorRow = ROW_NONE;
    ribbon->cursorEntry = CURSOR_NONE;
    ribbon->prevCursorRow = ROW_NONE;
    ribbon->isShown = TRUE;
    ribbon->needsRedraw = TRUE;
    func_0204c488(wk->buttons[PSTA_BUTTON_RIBBON], 5);
}

void PStaRibbon_Unload(PStatusWork *wk, PStaRibbonWork *ribbon) {
    ribbon->isShown = FALSE;
    if (ribbon->nameWindow != NULL) {
        BmpWin_Free(ribbon->nameWindow);
        ribbon->nameWindow = NULL;
    }
    if (ribbon->descWindow != NULL) {
        BmpWin_Free(ribbon->descWindow);
        ribbon->descWindow = NULL;
    }
}

void PStaRibbon_Clear(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 i;

    for (i = 0; i < ROW_COUNT; i++) {
        PStaOam_SetVisible(ribbon->rows[i].oam, FALSE);
    }
    GFL_BGSysFillScrArea(1, 0, 0, 0, 19, 21, 16);
    GFL_BGSysLoadScr(1);
    ribbon->isShown = FALSE;
    func_0204c488(wk->buttons[PSTA_BUTTON_RIBBON], 2);
}

void PStaRibbon_LoadPokemon(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 counts[RIBBON_CATEGORY_COUNT] = { 1, 1, 1, 1, 1 };
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u8 i;
    u8 count;
    u8 category;

    for (i = 0; i < RIBBON_COUNT; i++) {
        ribbon->entries[i].isValid = FALSE;
    }
    count = 0;
    for (i = 0; i < RIBBON_COUNT; i++) {
        if (PML_PkmGetParam(pkm, Ribbon_GetData(i, RIBBON_DATA_PARAM), NULL) == TRUE) {
            category = Ribbon_GetData(i, RIBBON_DATA_CATEGORY);
            ribbon->entries[count].isValid = TRUE;
            ribbon->entries[count].ribbon = i;
            ribbon->entries[count].category = category;
            ribbon->entries[count].number = counts[category];
            counts[category]++;
            count++;
        }
    }
    ribbon->ribbonCount = count;
    for (i = 0; i < ROW_COUNT; i++) {
        ribbon->rows[i].entry = PStaRibbon_GetRowEntry(ribbon, i);
    }
}

void PStaRibbon_UnloadPokemon(PStatusWork *wk, PStaRibbonWork *ribbon) {
}

static void PStaRibbon_DrawRow(PStatusWork *wk, PStaRibbonWork *ribbon, RibbonRow *row) {
    RibbonEntry *entry = &ribbon->entries[row->entry];
    u8 *src;
    u8 *dst;
    StrBuf *format;
    StrBuf *str;
    WordSet *wordSet;

    if (entry->isValid == TRUE) {
        src = ribbon->rowChars->rawData;
        dst = GFL_BitmapGetPixelData(row->bitmap);
        if (row->entry == ribbon->cursorEntry) {
            src += ROW_CHAR_SIZE;
        }
        sys_memcpy(src, dst, ROW_CHAR_SIZE);
        format = GFL_MsgDataLoadStrbufNew(ribbon->msgData, MSG_CATEGORY_FIRST + entry->category);
        str = GFL_StrBufCreate(32, wk->heapId);
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, entry->number, 2, 2, 1);
        GFL_WordSetFormatStrbuf(wordSet, str, format);
        GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
        GFL_TextRendererDrawToBitmap(row->bitmap, 10, 6, str, wk->font);
        func_020232d8();
        GFL_StrBufFree(format);
        GFL_StrBufFree(str);
        GFL_WordSetSystemFree(wordSet);
        PStaOam_Upload(row->oam);
    }
}

static void PStaRibbon_Update(PStatusWork *wk, PStaRibbonWork *ribbon) {
    BOOL redrawn = FALSE;
    u8 i;
    u32 entry;
    s16 y;

    for (i = 0; i < ROW_COUNT; i++) {
        if (ribbon->needsRedraw == TRUE) {
            y = PStaRibbon_GetRowY(ribbon, i);
            PStaOam_SetPosition(ribbon->rows[i].oam, ROW_X, y);
            entry = PStaRibbon_GetRowEntry(ribbon, i);
            if (entry == ENTRY_NONE) {
                ribbon->rows[i].entry = entry;
                PStaOam_SetVisible(ribbon->rows[i].oam, FALSE);
            } else if (entry != ribbon->rows[i].entry) {
                redrawn = TRUE;
                ribbon->rows[i].entry = entry;
                PStaRibbon_DrawRow(wk, ribbon, &ribbon->rows[i]);
                PStaOam_SetVisible(ribbon->rows[i].oam, TRUE);
            }
        }
        if (ribbon->rows[i].isPrinting == TRUE && func_02021c1c(wk->printQueue, ribbon->rows[i].bitmap) == FALSE) {
            ribbon->rows[i].isPrinting = FALSE;
            PStaOam_Upload(ribbon->rows[i].oam);
        }
    }
    if (redrawn == TRUE && ribbon->seWait == 0) {
        GFL_SEPlayKeepVol(SEQ_SE_SYS_06, 1);
        ribbon->seWait = 3;
    }
    if (ribbon->cursorMoved == TRUE) {
        if (ribbon->cursorRow < ROW_COUNT) {
            PStaRibbon_DrawRow(wk, ribbon, &ribbon->rows[ribbon->cursorRow]);
        }
        if (ribbon->prevCursorRow < ROW_COUNT) {
            PStaRibbon_DrawRow(wk, ribbon, &ribbon->rows[ribbon->prevCursorRow]);
        }
        ribbon->prevCursorRow = ribbon->cursorRow;
        ribbon->cursorMoved = FALSE;
    }
    ribbon->needsRedraw = FALSE;
}

static u8 PStaRibbon_GetRowIndex(PStaRibbonWork *ribbon, u8 row) {
    u8 top = ribbon->scrollY / ROW_HEIGHT;
    s8 offset = row - (u8)(top % ROW_COUNT);

    if (offset < -2) {
        offset += ROW_COUNT;
    }
    if (offset > 8) {
        offset -= ROW_COUNT;
    }
    return top + offset;
}

static u32 PStaRibbon_GetRowEntry(PStaRibbonWork *ribbon, u8 row) {
    u32 entry = PStaRibbon_GetRowIndex(ribbon, row);

    if (entry >= ribbon->ribbonCount) {
        entry = ENTRY_NONE;
    }
    return entry;
}

static s16 PStaRibbon_GetRowY(PStaRibbonWork *ribbon, u8 row) {
    u8 top = ribbon->scrollY / ROW_HEIGHT;
    u8 scroll = ribbon->scrollY % ROW_HEIGHT;
    s8 offset = row - (u8)(top % ROW_COUNT);

    if (offset < -2) {
        offset += ROW_COUNT;
    }
    if (offset > 8 && top > ROWS_SHOWN) {
        offset -= ROW_COUNT;
    }
    return offset * ROW_HEIGHT + LIST_TOP - scroll;
}

static void PStaRibbon_PrintDetail(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 id;
    StrBuf *str;

    ribbon->nameWindow = BmpWin_CreateDynamic(4, 5, 7, 22, 2, 14, 1);
    ribbon->descWindow = BmpWin_CreateDynamic(4, 1, 16, 30, 4, 14, 1);
    id = ribbon->entries[ribbon->cursorEntry].ribbon;
    str = GFL_MsgDataLoadStrbufNew(ribbon->msgData, Ribbon_GetData(id, RIBBON_DATA_NAME));
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(ribbon->nameWindow), 1, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(ribbon->msgData, Ribbon_GetDescription(id));
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(ribbon->descWindow), 1, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    ribbon->isDetailPending = TRUE;
}

static void PStaRibbon_ShowDetail(PStatusWork *wk, PStaRibbonWork *ribbon) {
    u8 id = ribbon->entries[ribbon->cursorEntry].ribbon;
    BmpWin *window;
    ArcTool *arc;
    ClActorSetup setup;

    window = ribbon->nameWindow;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    window = ribbon->descWindow;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    arc = GFL_ArcSysCreateFileHandle(ARCID_P_STATUS, wk->heapId);
    ribbon->iconChars = func_0204b81c(arc, Ribbon_GetData(id, RIBBON_DATA_ICON), FALSE, 1, wk->heapId);
    GFL_ArcToolFree(arc);
    setup.x = 128;
    setup.y = 100;
    setup.priority = 10;
    setup.bgPriority = 0;
    setup.sequence = 0;
    ribbon->icon = func_0204c040(wk->actorUnit, ribbon->iconChars, wk->clResources[PSTA_RES_PLTT(7)],
                                 wk->clResources[PSTA_RES_CELL(11)], &setup, 1, wk->heapId);
    func_0204c378(ribbon->icon, (u8)Ribbon_GetData(id, RIBBON_DATA_PALETTE), 1);
    func_0204c124(ribbon->icon, TRUE);
    GFL_BGSysBufferScrDefault(6, ribbon->screens[2].screen->rawData, ribbon->screens[2].screen->size);
    GFL_BGSysQueueScrLoad(6);
    ribbon->isDetailShown = TRUE;
}

static void PStaRibbon_FreeDetailWindows(PStatusWork *wk, PStaRibbonWork *ribbon) {
    if (ribbon->nameWindow != NULL) {
        BmpWin_Free(ribbon->nameWindow);
        ribbon->nameWindow = NULL;
    }
    if (ribbon->descWindow != NULL) {
        BmpWin_Free(ribbon->descWindow);
        ribbon->descWindow = NULL;
    }
}

static void PStaRibbon_HideDetail(PStatusWork *wk, PStaRibbonWork *ribbon) {
    if (ribbon->isDetailShown == TRUE) {
        func_0204b98c(ribbon->iconChars);
        func_0204c108(ribbon->icon);
        GFL_BGSysFillScrAsync(4, 0);
        GFL_BGSysQueueScrLoad(4);
        ribbon->isDetailShown = FALSE;
        GFL_BGSysBufferScrDefault(6, ribbon->screens[1].screen->rawData, ribbon->screens[1].screen->size);
        GFL_BGSysQueueScrLoad(6);
    }
}
