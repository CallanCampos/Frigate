// The field menu on the touch screen: its items' icons and names, the cursor, the return button, and scrolling it in
// and out. The name is the ROM's own, from GFL_HeapAllocate's file argument
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/directions.h"
#include "constants/flags.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_menu.h"
#include "field/field_msgbg.h"
#include "field/player_state.h"
#include "field/subscreen.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/math_util.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "text/system/load_field_menu_texts.h"

// The slots of the items, x + y * 2 for the cursor's column and row, and the return button at the bottom
#define FIELD_MENU_SLOT_BOTTOM 6
#define FIELD_MENU_SLOT_COUNT 7

// The moves of FIELD_MENU_CURSOR_MOVES from the return button up to the second or the third row, keeping the column
#define FIELD_MENU_MOVE_TO_ROW_1 0xfe
#define FIELD_MENU_MOVE_TO_ROW_2 0xff

// The menu's states: scroll in, wait for the names to print, wait for the wipe, take input, wait for the return
// button's animation, act on the choice, wait for the cursor's animation, report the menu closed or the item chosen,
// scroll out, and done
enum {
    FIELD_MENU_STATE_SCROLL_IN,
    FIELD_MENU_STATE_PRINT,
    FIELD_MENU_STATE_WAIT_WIPE,
    FIELD_MENU_STATE_INPUT,
    FIELD_MENU_STATE_WAIT_RETURN_BUTTON,
    FIELD_MENU_STATE_DECIDE,
    FIELD_MENU_STATE_WAIT_CURSOR,
    FIELD_MENU_STATE_CLOSED,
    FIELD_MENU_STATE_SELECTED,
    FIELD_MENU_STATE_SCROLL_OUT,
    FIELD_MENU_STATE_DONE,
};

// An item of the menu
typedef struct {
    BOOL active;
    BmpWin *win;
    u32 itemId;
    u32 chars;
    u32 cellAnims;
    // Where the cursor goes on the item
    u8 cursorX;
    u8 cursorY;
    ClActor *icon;
} FieldMenuItem;

struct FieldMenu {
    HeapID heapId;
    HeapID tmpHeapId;
    Field *field;
    FieldSubscreen *subscreen;
    u32 state;
    FieldMsgBG *msgBGSys;
    EventWork *eventWork;
    PlayerInfo *playerInfo;
    u16 zoneId;
    // The cursor's column and row; the item is x + y * 2, and row 3 is the bottom button
    u8 cursorX;
    u8 cursorY;
    u8 decideFrames;
    u8 menuType;
    BOOL cursorDirty;
    BOOL cancel;
    // Whether the keys move the cursor, rather than the touch screen
    BOOL keyMode;
    u32 selectedItemId;
    FieldMenuItem items[FIELD_MENU_SLOT_COUNT];
    FieldMenuItem *curItem;
    s16 scrollY;
    BOOL scrollDirty;
    u32 menuPalette;
    u32 menuChars;
    u32 menuCellAnims;
    u32 commonPalette;
    u32 commonChars;
    u32 commonCellAnims;
    ClActUnit *actUnit;
    ClActRenderer *renderer;
    ClActor *cursor;
    ClActor *returnButton;
    PrintQueue *printQueue;
    TCB *vblankTcb;
    u32 cursorFadeFrame;
};

// What FieldMenu_CreateItem makes an item from
typedef struct {
    u32 itemId;
    u8 x;
    u8 y;
    u8 winX;
    u8 winY;
    u8 iconX;
    u8 iconY;
    StrBuf *name;
    ArcTool *arc;
    u32 charFile;
    u32 cellFile;
    u32 animFile;
} FieldMenuItemSetup;

// The palette endpoints store their channels in red, blue, green order.
typedef struct {
    s32 red;
    s32 blue;
    s32 green;
} FieldMenuCursorColor;

static inline GXRgb FieldMenu_InterpolateCursorColor(const FieldMenuCursorColor *start, const FieldMenuCursorColor *end,
                                                     fx32 phase) {
    s32 red = phase * (end->red - start->red) / FX32_CONST(2) + start->red;
    s32 blue = phase * (end->blue - start->blue) / FX32_CONST(2) + start->blue;
    s32 green = phase * (end->green - start->green) / FX32_CONST(2) + start->green;

    // Packs the channels from the highest field down, as palanm.c and fld3d_ci.c pack theirs
    return (blue << GX_RGB_B_SHIFT) | (green << GX_RGB_G_SHIFT) | red;
}

static void FieldMenu_RestoreSubscreenOnCancel(FieldMenu *menu);
static void FieldMenu_EnableBGs(void);
static void FieldMenu_ScrollActors(FieldMenu *menu);
static void FieldMenu_LoadResources(FieldMenu *menu, ArcTool *arc, u8 menuType, FieldMsgBG *msgBGSys);
static int FieldMenu_CountItems(u8 menuType);
static void LoadFieldMenuTexts(FieldMenu *menu, ArcTool *arc, u8 menuType);
static void FieldMenu_PulseCursorTCB(TCB *tcb, void *data);
static void FieldMenu_SetOpeningScroll(FieldMenu *menu);
static int FieldMenu_GetSlot(u8 x, u8 y);
static BOOL FieldMenu_MoveCursor(FieldMenu *menu, u32 dir);
static void FieldMenu_UpdateKeys(FieldMenu *menu);
static void FieldMenu_UpdateTouch(FieldMenu *menu);
static void FieldMenu_SetCursorPosition(FieldMenu *menu, u8 x, u8 y);
static void FieldMenu_SelectOrCancel(FieldMenu *menu, BOOL cancel);
static void FieldMenu_UpdateCursor(FieldMenu *menu);
static void FieldMenu_CreateItem(FieldMenu *menu, FieldMenuItem *item, const FieldMenuItemSetup *setup);
static void FieldMenu_FreeItem(FieldMenu *menu, FieldMenuItem *item);
static void FieldMenu_FlushItem(FieldMenu *menu, FieldMenuItem *item);
static void FieldMenu_LoadBottomBar(HeapID heapId, u32 bg);

// Where each slot's name window is, in tiles
static const u8 FIELD_MENU_NAME_WINDOW_POSITIONS[FIELD_MENU_SLOT_COUNT][2] = {
    { 6, 4 }, { 22, 4 }, { 6, 10 }, { 22, 10 }, { 6, 16 }, { 22, 16 }, { 13, 21 },
};

// The cursor's column and row of each slot
static const u8 FIELD_MENU_SLOT_COORDINATES[FIELD_MENU_SLOT_COUNT][2] = {
    { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }, { 1, 2 }, { 0, 3 },
};

// The message of each item's name
static const u16 FIELD_MENU_NAME_MESSAGES[FIELD_MENU_ITEM_MAX] = {
    LoadFieldMenuTexts_Text_Empty_3, LoadFieldMenuTexts_Text_Pokemon, LoadFieldMenuTexts_Text_Pokedex,
    LoadFieldMenuTexts_Text_Bag,     LoadFieldMenuTexts_Text_Empty_2, LoadFieldMenuTexts_Text_Save,
    LoadFieldMenuTexts_Text_Options, LoadFieldMenuTexts_Text_Empty_5, LoadFieldMenuTexts_Text_Retire
};

// The screen of each kind of menu
static const u32 FIELD_MENU_SCREEN_FILES[FIELD_MENU_TYPE_MAX] = { 0x12, 0x14, 0x14, 0x13, 0x14, 0x14, 0x12 };

// The cursor's colors 0 and 1, each pulsing between a start and an end endpoint
static const FieldMenuCursorColor sCursorColorEndpoints[2][2] = {
    { { 9, 31, 18 }, { 17, 31, 23 } },
    { { 0, 25, 18 }, { 31, 31, 31 } },
};

// Where the cursor moves from each slot, by DIR_ direction, for menus of four, five and six items: the slot it moves
// to, the same one for none, or a FIELD_MENU_MOVE_ to a row. 6 is the return button
// clang-format off
static const u8 FIELD_MENU_CURSOR_MOVES[3][8][4] = {
    {
        {0, 2, 0, 1}, {1, 3, 0, 1}, {0, 2, 2, 3}, {1, 3, 2, 3}, {6, 6, 6, 6}, {6, 6, 6, 6},
        {FIELD_MENU_MOVE_TO_ROW_1, 6, 6, 6}, {FIELD_MENU_MOVE_TO_ROW_1, 6, 6, 6},
    },
    {
        {0, 2, 0, 1}, {1, 3, 0, 1}, {0, 4, 2, 3}, {1, 3, 2, 3}, {2, 4, 4, 4}, {6, 6, 6, 6},
        {4, 6, 6, 6}, {4, 6, 6, 6},
    },
    {
        {0, 2, 0, 1}, {1, 3, 0, 1}, {0, 4, 2, 3}, {1, 5, 2, 3}, {2, 4, 4, 5}, {3, 5, 4, 5},
        {FIELD_MENU_MOVE_TO_ROW_2, 6, 6, 6}, {FIELD_MENU_MOVE_TO_ROW_2, 6, 6, 6},
    },
};

// clang-format on

// The characters, cells and animations of each item's icon, then the female player's Bag's and Pokédex's and the
// retire item's, which LoadFieldMenuTexts picks
static const u32 FIELD_MENU_ICON_FILES[11][3] = {
    { 0x0f, 0x1f, 0x2a }, { 0x09, 0x19, 0x24 }, { 0x0d, 0x1d, 0x28 }, { 0x07, 0x17, 0x22 },
    { 0x0c, 0x1c, 0x27 }, { 0x0a, 0x1a, 0x25 }, { 0x06, 0x16, 0x21 }, { 0x09, 0x19, 0x24 },
    { 0x08, 0x18, 0x23 }, { 0x0e, 0x1e, 0x29 }, { 0x0b, 0x1b, 0x26 },
};

// The items of each kind of menu, the last one at the bottom button
// clang-format off
static const u32 FIELD_MENU_ITEMS[FIELD_MENU_TYPE_MAX][FIELD_MENU_SLOT_COUNT] = {
    [FIELD_MENU_TYPE_NORMAL] = {
        FIELD_MENU_ITEM_POKEMON, FIELD_MENU_ITEM_POKEDEX, FIELD_MENU_ITEM_BAG, FIELD_MENU_ITEM_TRAINER_CARD,
        FIELD_MENU_ITEM_SAVE, FIELD_MENU_ITEM_OPTIONS, FIELD_MENU_ITEM_RETURN,
    },
    [FIELD_MENU_TYPE_UNION_ROOM] = {
        FIELD_MENU_ITEM_POKEMON, FIELD_MENU_ITEM_POKEDEX, FIELD_MENU_ITEM_BAG, FIELD_MENU_ITEM_TRAINER_CARD,
        FIELD_MENU_ITEM_OPTIONS, FIELD_MENU_ITEM_NONE, FIELD_MENU_ITEM_RETURN,
    },
    [FIELD_MENU_TYPE_NO_POKEDEX] = {
        FIELD_MENU_ITEM_POKEMON, FIELD_MENU_ITEM_BAG, FIELD_MENU_ITEM_TRAINER_CARD, FIELD_MENU_ITEM_SAVE,
        FIELD_MENU_ITEM_OPTIONS, FIELD_MENU_ITEM_NONE, FIELD_MENU_ITEM_RETURN,
    },
    [FIELD_MENU_TYPE_NO_POKEMON] = {
        FIELD_MENU_ITEM_BAG, FIELD_MENU_ITEM_TRAINER_CARD, FIELD_MENU_ITEM_SAVE, FIELD_MENU_ITEM_OPTIONS,
        FIELD_MENU_ITEM_NONE, FIELD_MENU_ITEM_NONE, FIELD_MENU_ITEM_RETURN,
    },
    [FIELD_MENU_TYPE_ROYAL_UNOVA] = {
        FIELD_MENU_ITEM_POKEMON, FIELD_MENU_ITEM_POKEDEX, FIELD_MENU_ITEM_BAG, FIELD_MENU_ITEM_TRAINER_CARD,
        FIELD_MENU_ITEM_OPTIONS, FIELD_MENU_ITEM_NONE, FIELD_MENU_ITEM_RETURN,
    },
    [FIELD_MENU_TYPE_FORCE_SEASON_SYNC] = {
        FIELD_MENU_ITEM_POKEMON, FIELD_MENU_ITEM_POKEDEX, FIELD_MENU_ITEM_BAG, FIELD_MENU_ITEM_TRAINER_CARD,
        FIELD_MENU_ITEM_OPTIONS, FIELD_MENU_ITEM_NONE, FIELD_MENU_ITEM_RETURN,
    },
    [FIELD_MENU_TYPE_INF_DUNGEON] = {
        FIELD_MENU_ITEM_POKEMON, FIELD_MENU_ITEM_POKEDEX, FIELD_MENU_ITEM_TRAINER_CARD, FIELD_MENU_ITEM_SAVE,
        FIELD_MENU_ITEM_OPTIONS, FIELD_MENU_ITEM_RETIRE, FIELD_MENU_ITEM_RETURN,
    },
};
// clang-format on

FieldMenu *FieldMenu_Create(HeapID heapId, HeapID tmpHeapId, FieldSubscreen *subscreen, Field *field, BOOL open) {
    FieldMenu *menu = GFL_HeapAllocate(heapId, sizeof(FieldMenu), FALSE, "field_menu.c", 323);
    GameSystem *gsys = Field_GetGameSystem(field);
    GameData *gameData = GSYS_GetGameData(gsys);
    ArcTool *arc;
    u8 menuType;

    menu->heapId = heapId;
    menu->tmpHeapId = tmpHeapId;
    menu->field = field;
    menu->subscreen = subscreen;
    menu->state = FIELD_MENU_STATE_PRINT;
    menu->zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
    menu->eventWork = GameData_GetEventWork(gameData);
    menu->playerInfo = GetGameDataPlayerInfo(gameData);
    menu->msgBGSys = Field_GetMsgBGSys(field);
    menu->cursorX = 0;
    menu->cursorY = 0;
    menu->cursorDirty = menu->keyMode = GCTX_HIDIsUsingTouch() ^ TRUE;
    menu->curItem = NULL;
    menu->vblankTcb = GFL_VBlankTCBAdd(FieldMenu_PulseCursorTCB, menu, 0);
    menu->printQueue = PrintQueue_CreateDefault(menu->heapId);
    menu->scrollDirty = FALSE;
    menu->scrollY = 0;
    arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_MENU, menu->tmpHeapId);
    menuType = FieldMenu_GetMenuType(gameData, menu->eventWork, menu->zoneId);
    menu->menuType = menuType;
    FieldMenu_LoadResources(menu, arc, menuType, menu->msgBGSys);
    LoadFieldMenuTexts(menu, arc, menuType);
    GFL_ArcToolFree(arc);
    GFL_BGSysSetBGEnabled(1, TRUE);
    if (open == TRUE) {
        menu->cursorX = 0;
        menu->cursorY = 0;
        menu->cursorDirty = TRUE;
        FieldMenu_UpdateCursor(menu);
        FieldMenu_SetOpeningScroll(menu);
    } else {
        FieldMenu_SetCursorItem(menu, GameData_GetFieldMenuItem(gameData));
        menu->cursorDirty = TRUE;
        if (menu->keyMode) {
            ClActor_SetVisible(menu->cursor, TRUE);
        } else {
            ClActor_SetVisible(menu->cursor, FALSE);
        }
    }
    GFL_NetSetIconScreen(FALSE, menu->tmpHeapId);
    return menu;
}

static void FieldMenu_RestoreSubscreenOnCancel(FieldMenu *menu) {
    if (menu->cancel == TRUE) {
        func_ov036_021984f0(menu->subscreen, menu->heapId);
    }
}

void FieldMenu_Free(FieldMenu *menu) {
    u8 i;

    for (i = 0; i < FIELD_MENU_SLOT_COUNT; i++) {
        FieldMenu_FreeItem(menu, &menu->items[i]);
    }
    FieldMenu_RestoreSubscreenOnCancel(menu);
    GFL_TCBRemove(menu->vblankTcb);
    ClActor_Delete(menu->returnButton);
    ClActor_Delete(menu->cursor);
    ClActRenderer_Delete(menu->renderer);
    ClActUnit_Delete(menu->actUnit);
    ClActSys_FreeCellAnims(menu->menuCellAnims);
    ClActSys_FreeChars(menu->menuChars);
    ClActSys_FreePltt(menu->menuPalette);
    ClActSys_FreeCellAnims(menu->commonCellAnims);
    ClActSys_FreeChars(menu->commonChars);
    ClActSys_FreePltt(menu->commonPalette);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, 0);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    PrintQueue_Clear(menu->printQueue);
    PrintQueue_Free(menu->printQueue);
    GFL_HeapFree(menu);
}

static void FieldMenu_EnableBGs(void) {
    GFL_BGSysLoadScr(6);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, TRUE);
}

void FieldMenu_Update(FieldMenu *menu) {
    switch (menu->state) {
    case FIELD_MENU_STATE_PRINT:
        if (PrintQueue_IsEmpty(menu->printQueue) == TRUE) {
            u8 i;

            func_ov036_02198884(menu->subscreen, menu->tmpHeapId);
            for (i = 0; i < FIELD_MENU_SLOT_COUNT; i++) {
                FieldMenu_FlushItem(menu, &menu->items[i]);
            }
            if (menu->scrollY != 0) {
                GFL_SndSEPlay(SEQ_SE_OPEN1);
                menu->state = FIELD_MENU_STATE_SCROLL_IN;
            } else {
                menu->state = FIELD_MENU_STATE_WAIT_WIPE;
                FieldMenu_EnableBGs();
            }
        }
        break;
    case FIELD_MENU_STATE_SCROLL_IN:
        if (menu->scrollY < 0x40) {
            menu->scrollY = 0;
            menu->state = FIELD_MENU_STATE_INPUT;
            FieldMenu_EnableBGs();
        } else {
            menu->scrollY -= 0x40;
            FieldMenu_ScrollActors(menu);
        }
        menu->scrollDirty = TRUE;
        break;
    case FIELD_MENU_STATE_WAIT_WIPE:
        if (GFL_WipeIsFinished() == TRUE) {
            func_ov036_02198884(menu->subscreen, menu->tmpHeapId);
            menu->state = FIELD_MENU_STATE_INPUT;
        }
        break;
    case FIELD_MENU_STATE_INPUT:
        FieldMenu_UpdateKeys(menu);
        FieldMenu_UpdateTouch(menu);
        FieldMenu_UpdateCursor(menu);
        break;
    case FIELD_MENU_STATE_WAIT_RETURN_BUTTON:
        if (!ClActor_IsAnimActive(menu->returnButton)) {
            menu->state = FIELD_MENU_STATE_DECIDE;
            GFL_BGSysSetBGEnabled(5, TRUE);
            GFL_BGSysSetBGEnabled(6, TRUE);
            GFL_BGSysSetBGEnabled(7, TRUE);
            GFL_BGSysSetBGEnabled(4, TRUE);
        }
        break;
    case FIELD_MENU_STATE_DECIDE:
        if (menu->cancel == TRUE || menu->cursorY == 3) {
            menu->state = FIELD_MENU_STATE_SCROLL_OUT;
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            GFL_BGSysSetBGEnabled(4, FALSE);
            GFL_BGSysSetBGEnabled(5, TRUE);
            GFL_BGSysSetBGEnabled(6, TRUE);
            GFL_BGSysSetBGEnabled(7, TRUE);
            ClActor_SetVisible(menu->returnButton, FALSE);
        } else if (menu->curItem->icon != NULL) {
            menu->decideFrames = 0;
            FieldMenu_SetCursorPosition(menu, menu->curItem->cursorX, menu->curItem->cursorY);
            ClActor_SetSequence(menu->cursor, 1);
            ClActor_SetVisible(menu->cursor, TRUE);
            menu->state = FIELD_MENU_STATE_WAIT_CURSOR;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            menu->state = FIELD_MENU_STATE_SELECTED;
        }
        break;
    case FIELD_MENU_STATE_WAIT_CURSOR:
        menu->decideFrames++;
        if (!ClActor_IsAnimActive(menu->cursor)) {
            menu->state = FIELD_MENU_STATE_SELECTED;
        }
        break;
    case FIELD_MENU_STATE_CLOSED:
        FieldSubscreen_SaveReturnSubscreen(menu->subscreen, FIELD_MENU_RESULT_CANCELLED);
        menu->state = FIELD_MENU_STATE_DONE;
        break;
    case FIELD_MENU_STATE_SELECTED:
        FieldSubscreen_SaveReturnSubscreen(menu->subscreen, FIELD_MENU_RESULT_SELECTED);
        menu->state = FIELD_MENU_STATE_DONE;
        break;
    case FIELD_MENU_STATE_SCROLL_OUT:
        if (menu->scrollY + 0x40 > 0xc0) {
            menu->scrollY = 0xc0;
            menu->state = FIELD_MENU_STATE_CLOSED;
        } else {
            menu->scrollY += 0x40;
            FieldMenu_ScrollActors(menu);
        }
        menu->scrollDirty = TRUE;
        break;
    }
    PrintQueue_Main(menu->printQueue);
}

// Scrolls the actors with the BGs, hiding the icons that leave the screen
static void FieldMenu_ScrollActors(FieldMenu *menu) {
    ClActorPos scroll;
    ClActorPos iconPos;
    ClActorPos cursorPos;
    u8 i;

    scroll.x = 0;
    scroll.y = -menu->scrollY;
    ClActRenderer_SetSurfacePos(menu->renderer, 0, &scroll);
    for (i = 0; i < FIELD_MENU_SLOT_COUNT; i++) {
        if (menu->items[i].icon != NULL && menu->items[i].itemId != FIELD_MENU_ITEM_NONE) {
            ClActor_GetSurfacePos(menu->items[i].icon, &iconPos, 0);
            if (iconPos.y < 0xd0) {
                ClActor_SetVisible(menu->items[i].icon, TRUE);
            } else {
                ClActor_SetVisible(menu->items[i].icon, FALSE);
            }
        }
    }
    ClActor_GetSurfacePos(menu->cursor, &cursorPos, 0);
    if (menu->keyMode && cursorPos.y < 0xd6) {
        ClActor_SetVisible(menu->cursor, TRUE);
    } else {
        ClActor_SetVisible(menu->cursor, FALSE);
    }
}

void FieldMenu_UpdateScroll(FieldMenu *menu) {
    if (menu->scrollDirty == TRUE) {
        GFL_BGSysMoveBGReq(5, BG_MOVE_SET_Y, -menu->scrollY);
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, -5 - menu->scrollY);
        menu->scrollDirty = FALSE;
    }
}

static void FieldMenu_LoadResources(FieldMenu *menu, ArcTool *arc, u8 menuType, FieldMsgBG *msgBGSys) {
    static const BGSetup sFieldMenuBG6Setup = { 0, 0, 0x1000, 0, 2, 0, 0xc, 0, 0x5800, 0, 1, 0, 0 };
    static const BGSetup sFieldMenuBG7Setup = { 0, 0, 0x800, 0, 1, 0, 0xa, 0, 0x5800, 0, 3, 0, 0 };
    // An unused background setup retained in the menu's resources.
    static const BGSetup sFieldMenuUnusedBGSetup = { 0, 0, 0x800, 0, 1, 0, 2, 1, 0x8000, 0, 0, 0, 0 };
    static const BGSetup sFieldMenuBG5Setup = { 0, 0, 0x1000, 0, 2, 0, 0xe, 0, 0x5800, 0, 3, 0, 0 };

    ArcTool *common;
    ClActorSetup setup;

    GXS_SetMasterBrightness(-16);
    GFL_BGSysCreateBG(5, &sFieldMenuBG5Setup, 0);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysClearScr(5);
    GFL_BGSysCreateBG(6, &sFieldMenuBG6Setup, 0);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysClearScr(6);
    GFL_BGSysCreateBG(7, &sFieldMenuBG7Setup, 0);
    GFL_BGSysSetBGEnabled(7, TRUE);
    GFL_BGSysClearScr(7);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, -5);
    // Each version's colors
#ifdef BLACK2
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0, menu->tmpHeapId);
#else
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 0, menu->tmpHeapId);
#endif
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 5, 0, 0, FALSE, menu->tmpHeapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, FIELD_MENU_SCREEN_FILES[menuType], 5, 0, 0, FALSE, menu->tmpHeapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x10, 7, 0, 0, FALSE, menu->tmpHeapId);
    menu->menuPalette = ClActSys_LoadPlttRange(arc, 2, CLACT_VRAM_SUB, 0, 0, 11, menu->heapId);
    menu->menuChars = ClActSys_LoadChars(arc, 0xf, FALSE, CLACT_VRAM_SUB, menu->heapId);
    menu->menuCellAnims = ClActSys_LoadCellAnims(arc, 0x1f, 0x2a, menu->heapId);
    common = GFL_ArcSysCreateFileHandle(getUINarcIdx(), menu->tmpHeapId);
    menu->commonPalette = ClActSys_LoadPlttRange(common, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_SUB, 0x160,
                                                 0, 2, menu->heapId);
    menu->commonChars =
        ClActSys_LoadChars(common, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_SUB, menu->heapId);
    menu->commonCellAnims = ClActSys_LoadCellAnims(common, AppMenuCommon_GetTouchBarCellFile(0),
                                                   AppMenuCommon_GetTouchBarAnimFile(0), menu->heapId);
    GFL_ArcToolFree(common);
    GFL_BGSysLoadScr(5);
    menu->actUnit = ClActUnit_Create(8, 0, menu->heapId);
    {
        ClActSurfaceSetup surface = { 0, 0, 0x100, 0xbf, 1, 0 };

        menu->renderer = ClActRenderer_Create(&surface, 1, menu->heapId);
    }
    ClActUnit_SetRenderer(menu->actUnit, menu->renderer);
    setup.x = 0x40;
    setup.y = 0x2c;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    menu->cursor =
        ClActor_Create(menu->actUnit, menu->menuChars, menu->menuPalette, menu->menuCellAnims, &setup, 0, menu->heapId);
    ClActor_SetAutoAnimSpeed(menu->cursor, FX32_ONE);
    ClActor_SetAutoAnim(menu->cursor, TRUE);
    ClActor_SetBgPriority(menu->cursor, 2);
    setup.x = 0xe0;
    setup.y = 0xa8;
    menu->returnButton = ClActor_Create(menu->actUnit, menu->commonChars, menu->commonPalette, menu->commonCellAnims,
                                        &setup, 0, menu->heapId);
    ClActor_SetAutoAnimSpeed(menu->returnButton, FX32_ONE);
    ClActor_SetAutoAnim(menu->returnButton, TRUE);
    ClActor_SetVisible(menu->returnButton, TRUE);
    ClActor_SetAutoAnimSpeed(menu->returnButton, FX32_CONST(2));
}

void FieldMenu_LoadBar(HeapID heapId, BOOL hideBg4) {
    FieldMenu_LoadBottomBar(heapId, 4);
    if (hideBg4) {
        GFL_BGSysSetBGEnabled(4, FALSE);
    }
}

u8 FieldMenu_GetMenuType(GameData *gameData, EventWork *eventWork, u32 zoneId) {
    u8 type = FIELD_MENU_TYPE_NORMAL;

    if (GetZoneIsUnionRoom(zoneId) || IsZone150Or151(zoneId)) {
        type = FIELD_MENU_TYPE_UNION_ROOM;
    } else if (IsZoneRoyalUnova(zoneId)) {
        type = FIELD_MENU_TYPE_ROYAL_UNOVA;
    } else if (GameData_IsForceSeasonSync(gameData)) {
        type = FIELD_MENU_TYPE_FORCE_SEASON_SYNC;
    } else if (!EventWork_FlagGet(eventWork, EVENT_FLAG_0x0961)) {
        type = FIELD_MENU_TYPE_NO_POKEMON;
    } else if (!EventWork_FlagGet(eventWork, EVENT_FLAG_0x0962)) {
        type = FIELD_MENU_TYPE_NO_POKEDEX;
    } else if (IsZoneBlackTowerOrWhiteTreehollow(zoneId)) {
        type = FIELD_MENU_TYPE_INF_DUNGEON;
    }
    return type;
}

// How many items the kind of menu has
static int FieldMenu_CountItems(u8 menuType) {
    int i = 0;
    int count = 0;

    for (; FIELD_MENU_ITEMS[menuType][i] != FIELD_MENU_ITEM_RETURN; i++) {
        if (FIELD_MENU_ITEMS[menuType][i] != FIELD_MENU_ITEM_NONE) {
            count++;
        }
    }
    return count;
}

static void LoadFieldMenuTexts(FieldMenu *menu, ArcTool *arc, u8 menuType) {
    FieldMenuItemSetup setup;
    MsgData *msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_LOAD_FIELD_MENU_TEXTS, menu->tmpHeapId);
    const u32 *items;
    u8 i;

    for (i = 0; i < FIELD_MENU_SLOT_COUNT; i++) {
        menu->items[i].active = FALSE;
    }
    items = FIELD_MENU_ITEMS[menuType];
    for (i = 0; i < FIELD_MENU_SLOT_COUNT; i++) {
        u32 itemId = items[i];

        if (itemId == FIELD_MENU_ITEM_TRAINER_CARD) {
            setup.name = GFL_StrBufCreate(16, menu->tmpHeapId);
            if (!PlayerInfo_IsNameEmpty(menu->playerInfo)) {
                GFL_StrBufLoadString(setup.name, GetPlayerName(menu->playerInfo));
            } else {
                // The player's name when the save has none
                u16 name[] = { 'N', 'o', 'N', 'a', 'm', 'e', 0xffff };

                GFL_StrBufLoadString(setup.name, name);
            }
        } else {
            setup.name = GFL_MsgDataLoadStrbufNew(msgData, FIELD_MENU_NAME_MESSAGES[itemId]);
        }
        setup.itemId = itemId;
        if (i != FIELD_MENU_SLOT_BOTTOM) {
            setup.x = (i % 2) * 0x80 + 0x40;
            setup.y = (i / 2) * 0x30 + 0x2c;
            setup.iconX = setup.x - 0x20;
            setup.iconY = setup.y;
            setup.arc = arc;
            setup.charFile = FIELD_MENU_ICON_FILES[itemId][0];
            setup.cellFile = FIELD_MENU_ICON_FILES[itemId][1];
            setup.animFile = FIELD_MENU_ICON_FILES[itemId][2];
            if (itemId == FIELD_MENU_ITEM_BAG && getTrainerGender(menu->playerInfo) == GENDER_FEMALE) {
                setup.charFile = 8;
                setup.cellFile = 0x18;
                setup.animFile = 0x23;
            }
            if (itemId == FIELD_MENU_ITEM_POKEDEX && getTrainerGender(menu->playerInfo) == GENDER_FEMALE) {
                setup.charFile = 0xe;
                setup.cellFile = 0x1e;
                setup.animFile = 0x29;
            }
            if (itemId == FIELD_MENU_ITEM_RETIRE) {
                setup.charFile = 0xb;
                setup.cellFile = 0x1b;
                setup.animFile = 0x26;
            }
        } else {
            setup.x = 0x80;
            setup.y = 0xb4;
            setup.iconX = 0;
            setup.iconY = 0;
            setup.arc = NULL;
            setup.charFile = 0;
            setup.cellFile = 0;
            setup.animFile = 0;
        }
        setup.winX = FIELD_MENU_NAME_WINDOW_POSITIONS[i][0];
        setup.winY = FIELD_MENU_NAME_WINDOW_POSITIONS[i][1];
        FieldMenu_CreateItem(menu, &menu->items[i], &setup);
        GFL_StrBufFree(setup.name);
    }
    GFL_MsgDataFree(msgData);
}

// Pulses the cursor's colors
static void FieldMenu_PulseCursorTCB(TCB *tcb, void *data) {
    FieldMenu *menu = data;
    fx32 t = GFL_MathSinDegWrap(menu->cursorFadeFrame) + FX32_ONE;
    GXRgb cursorColor0;
    GXRgb cursorColor1;

    cursorColor0 = FieldMenu_InterpolateCursorColor(&sCursorColorEndpoints[0][0], &sCursorColorEndpoints[0][1], t);
    GXS_LoadOBJPltt(&cursorColor0, 0x142, sizeof(GXRgb));
    cursorColor1 = FieldMenu_InterpolateCursorColor(&sCursorColorEndpoints[1][0], &sCursorColorEndpoints[1][1], t);
    GXS_LoadOBJPltt(&cursorColor1, 0x15e, sizeof(GXRgb));
    menu->cursorFadeFrame += 6;
}

// Positions the menu offscreen for its opening scroll
static void FieldMenu_SetOpeningScroll(FieldMenu *menu) {
    ClActorPos pos;

    menu->scrollY = 0xc0;
    GFL_BGSysMoveBG(5, BG_MOVE_SET_Y, -0xc0);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, -5 - 0xc0);
    pos.x = 0;
    pos.y = 0xc0;
    ClActRenderer_SetSurfacePos(menu->renderer, 0, &pos);
}

u32 FieldMenu_GetSelectedItem(FieldMenu *menu) {
    if (menu->curItem == NULL) {
        return FIELD_MENU_ITEM_RETURN;
    }
    return menu->curItem->itemId;
}

void FieldMenu_SetCursorItem(FieldMenu *menu, u32 itemId) {
    u8 i;

    for (i = 0; i < FIELD_MENU_SLOT_COUNT; i++) {
        if (menu->items[i].active == TRUE && itemId == menu->items[i].itemId) {
            FieldMenuItem *item;

            menu->cursorX = i % 2;
            menu->cursorY = i / 2;
            menu->cursorDirty = TRUE;
            item = &menu->items[menu->cursorX + menu->cursorY * 2];
            FieldMenu_SetCursorPosition(menu, item->cursorX, item->cursorY);
        }
    }
}

static int FieldMenu_GetSlot(u8 x, u8 y) {
    return x + y * 2;
}

// Moves the cursor in the direction, and returns whether it moved
static BOOL FieldMenu_MoveCursor(FieldMenu *menu, u32 dir) {
    int slot = FieldMenu_GetSlot(menu->cursorX, menu->cursorY);
    u8 next = FIELD_MENU_CURSOR_MOVES[FieldMenu_CountItems(menu->menuType) - 4][slot][dir];

    if (slot != next) {
        if (next == FIELD_MENU_MOVE_TO_ROW_2) {
            menu->cursorY = 2;
            return TRUE;
        }
        if (next == FIELD_MENU_MOVE_TO_ROW_1) {
            menu->cursorY = 1;
            return TRUE;
        }
        if (next != FIELD_MENU_SLOT_BOTTOM) {
            menu->cursorX = FIELD_MENU_SLOT_COORDINATES[next][0];
        }
        menu->cursorY = FIELD_MENU_SLOT_COORDINATES[next][1];
        return TRUE;
    }
    return FALSE;
}

static void FieldMenu_UpdateKeys(FieldMenu *menu) {
    u32 trg = GCTX_HIDGetPressedKeys();
    u32 repeat = GCTX_HIDGetTypedKeys();

    if (menu->keyMode == FALSE && (trg | repeat) != 0) {
        if ((trg & PAD_BUTTON_B) || (trg & PAD_BUTTON_X)) {
            FieldMenu_SelectOrCancel(menu, TRUE);
            return;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        menu->keyMode = TRUE;
        menu->curItem = NULL;
        menu->cursorDirty = TRUE;
        ClActor_SetVisible(menu->cursor, TRUE);
        return;
    }
    if (repeat & PAD_KEY_UP) {
        if (FieldMenu_MoveCursor(menu, DIR_UP)) {
            menu->cursorDirty = TRUE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return;
    }
    if (repeat & PAD_KEY_DOWN) {
        if (FieldMenu_MoveCursor(menu, DIR_DOWN)) {
            menu->cursorDirty = TRUE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return;
    }
    if (repeat & PAD_KEY_LEFT) {
        if (FieldMenu_MoveCursor(menu, DIR_LEFT)) {
            menu->cursorDirty = TRUE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return;
    }
    if (repeat & PAD_KEY_RIGHT) {
        if (FieldMenu_MoveCursor(menu, DIR_RIGHT)) {
            menu->cursorDirty = TRUE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        return;
    }
    if ((trg & PAD_BUTTON_B) || (trg & PAD_BUTTON_X)) {
        FieldMenu_SelectOrCancel(menu, TRUE);
        return;
    }
    if (trg & PAD_BUTTON_A) {
        if (menu->items[FieldMenu_GetSlot(menu->cursorX, menu->cursorY)].itemId != FIELD_MENU_ITEM_NONE) {
            FieldMenu_SelectOrCancel(menu, FALSE);
            GCTX_HIDSetUsingTouch(FALSE);
        }
    }
}

static void FieldMenu_UpdateTouch(FieldMenu *menu) {
    TouchRect rects[8] = {
        { 0x18, 0x40, 0x08, 0x78 }, { 0x18, 0x40, 0x88, 0xf8 }, { 0x48, 0x70, 0x08, 0x78 }, { 0x48, 0x70, 0x88, 0xf8 },
        { 0x78, 0xa0, 0x08, 0x78 }, { 0x78, 0xa0, 0x88, 0xf8 }, { 0xa8, 0xbf, 0xe0, 0xf8 }, { TOUCH_RECT_END, 0, 0, 0 },
    };
    int slot = GFL_HIDGetTouchPressedRect(rects);

    if (slot != TOUCH_RECT_NONE && menu->items[slot].itemId != FIELD_MENU_ITEM_NONE) {
        menu->cursorX = slot % 2;
        menu->cursorY = slot / 2;
        menu->keyMode = FALSE;
        ClActor_SetVisible(menu->cursor, FALSE);
        if (slot == FIELD_MENU_SLOT_BOTTOM) {
            FieldMenu_SelectOrCancel(menu, TRUE);
            return;
        }
        menu->cursorDirty = TRUE;
        FieldMenu_SelectOrCancel(menu, FALSE);
        GCTX_HIDSetUsingTouch(TRUE);
    }
}

static void FieldMenu_SetCursorPosition(FieldMenu *menu, u8 x, u8 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(menu->cursor, &pos, 0);
    menu->cursorFadeFrame = 0;
}

// Chooses the item under the cursor, or cancels
static void FieldMenu_SelectOrCancel(FieldMenu *menu, BOOL cancel) {
    if (cancel == TRUE) {
        menu->cancel = TRUE;
        menu->state = FIELD_MENU_STATE_WAIT_RETURN_BUTTON;
        ClActor_SetSequence(menu->returnButton, 8);
    } else {
        menu->cancel = FALSE;
        menu->state = FIELD_MENU_STATE_DECIDE;
        menu->selectedItemId = menu->items[FieldMenu_GetSlot(menu->cursorX, menu->cursorY)].itemId;
    }
}

static void FieldMenu_UpdateCursor(FieldMenu *menu) {
    FieldMenuItem *prev;

    if (menu->cursorDirty == TRUE) {
        prev = menu->curItem;
        menu->curItem = &menu->items[menu->cursorX + menu->cursorY * 2];
        ClActor_SetSequence(menu->cursor, 0);
        FieldMenu_SetCursorPosition(menu, menu->curItem->cursorX, menu->curItem->cursorY);
        if (menu->curItem->icon != NULL && menu->keyMode == TRUE) {
            ClActor_SetSequence(menu->curItem->icon, 1);
        }
        if (prev != NULL && prev->icon != NULL) {
            ClActor_SetSequence(prev->icon, 0);
        }
        menu->cursorDirty = FALSE;
    }
}

static void FieldMenu_CreateItem(FieldMenu *menu, FieldMenuItem *item, const FieldMenuItemSetup *setup) {
    Font *font = func_ov036_0218799c(Field_GetMsgBGSys(menu->field));
    ClActorSetupEx actorSetup;

    item->active = TRUE;
    item->itemId = setup->itemId;
    item->cursorX = setup->x;
    item->cursorY = setup->y;
    item->win = BmpWin_CreateDynamic(6, setup->winX, setup->winY, 8, 2, 13, TRUE);
    BmpWin_FlushMap(item->win);
    GFL_BitmapFill(BmpWin_GetBitmap(item->win), 0);
    PrintQueue_PrintColor(menu->printQueue, BmpWin_GetBitmap(item->win), 2, 1, setup->name, font, 0x440);
    if (setup->arc != NULL) {
        item->chars = ClActSys_LoadChars(setup->arc, setup->charFile, FALSE, CLACT_VRAM_SUB, menu->heapId);
        item->cellAnims = ClActSys_LoadCellAnims(setup->arc, setup->cellFile, setup->animFile, menu->heapId);
        actorSetup.base.x = setup->iconX;
        actorSetup.base.y = setup->iconY;
        actorSetup.base.sequence = 0;
        actorSetup.base.priority = 0;
        actorSetup.base.bgPriority = 0;
        actorSetup.affineCenter.x = 0;
        actorSetup.affineCenter.y = 0;
        actorSetup.scaleX = FX32_ONE;
        actorSetup.scaleY = FX32_ONE;
        actorSetup.rotation = 0;
        actorSetup.affineMode = 2;
        item->icon = ClActor_CreateEx(menu->actUnit, item->chars, menu->menuPalette, item->cellAnims, &actorSetup, 0,
                                      menu->heapId);
        ClActor_SetAutoAnimSpeed(item->icon, FX32_ONE);
        ClActor_SetAutoAnim(item->icon, TRUE);
        ClActor_SetSequence(item->icon, 0);
        if (item->itemId == FIELD_MENU_ITEM_NONE) {
            ClActor_SetVisible(item->icon, FALSE);
        } else {
            ClActor_SetVisible(item->icon, TRUE);
        }
    } else {
        item->icon = NULL;
    }
}

static void FieldMenu_FreeItem(FieldMenu *menu, FieldMenuItem *item) {
    if (item->active == TRUE) {
        BmpWin_Free(item->win);
        if (item->icon != NULL) {
            ClActor_Delete(item->icon);
            ClActSys_FreeCellAnims(item->cellAnims);
            ClActSys_FreeChars(item->chars);
        }
    }
}

static void FieldMenu_FlushItem(FieldMenu *menu, FieldMenuItem *item) {
    if (item->active == TRUE) {
        BmpWin_FlushChar(item->win);
    }
}

// Loads the bar at the bottom of the touch screen into the BG
static void FieldMenu_LoadBottomBar(HeapID heapId, u32 bg) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    NNSG2dScreenData *screen;
    void *data;
    u16 *raw;
    int i;

    GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), 4, 0x60, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, AppMenuCommon_GetBarBgCharFile(), bg, 0x80, 0, FALSE, heapId);
    data = GFL_G2DIOReadNSCRArc(arc, AppMenuCommon_GetBarScreenFile(), FALSE, &screen, heapId);
    GFL_ArcToolFree(arc);
    raw = (u16 *)screen->rawData;
    for (i = 0; i < 0x300; i++) {
        raw[i] = (raw[i] + 0x80) | 0x3000;
    }
    GFL_BGSysLoadScrArea(bg, 0, 0x15, 0x20, 3, screen->rawData, 0, 0x15, 0x20, 0x18);
    GFL_BGSysQueueScrLoad(bg);
    GFL_HeapFree(data);
}
