// The Entralink monolith's helpers that its screens share: text in frame actors, the panels' palette effects, the
// return button, the menus and the bar of the White and Black levels. The name is the ROM's string, from
// GFL_HeapAllocate's assert

#include "types.h"
#include "app/monolith/monolith_tool.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/app_taskmenu.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The color of the task menus' text
#define MONOLITH_MENU_TEXT_COLOR 0x39e0

static void MonolithTool_SetPanelMode(MonolithScreenParam *param, u8 mode, int req);
static u32 MonolithTool_GetPanelColorPos(u32 req);
static u32 MonolithTool_GetPanelTargetPos(u32 req);
static u32 MonolithTool_GetPanelColorCount(u32 req);

static const ClActorSetup sMonolithTextFrameSetup = { 128, 0, 0, 11, 1 };

// A setup for text behind the other actors, which nothing uses. The ROM has it after sMonolithTextSetup; MWCC's sort
// of the objects by size puts it there only when it is declared first
const BmpOamActorSetup MonolithTool_UnusedTextSetup = { NULL, 0, 0, 0, 0, 200, 3, 0, CLACT_VRAM_MAIN };

static const BmpOamActorSetup sMonolithTextSetup = { NULL, 0, 0, 0, 0, 10, 1, 0, CLACT_VRAM_MAIN };

// Reconstructed, not known from the ROM: MWCC puts the unused setup with the others in .rodata, as the original has it,
// only when some code refers to it, even code it never emits. Nothing calls this
static inline const BmpOamActorSetup *MonolithTool_GetUnusedTextSetup(void) {
    return &MonolithTool_UnusedTextSetup;
}

void MonolithTool_CreateText(MonolithWork *wk, MonolithTextActor *text, int sub, int width, int x, int y, u32 msgId,
                             WordSet *wordSet) {
    ClActorSetup frameSetup = sMonolithTextFrameSetup;
    BmpOamActorSetup textSetup = sMonolithTextSetup;
    int tiles;
    StrBuf *str;
    u32 vramType;

    switch (width) {
    case MONOLITH_TEXT_WIDE:
        tiles = 22;
        frameSetup.sequence = 0;
        textSetup.x = x - 88;
        break;
    case MONOLITH_TEXT_MEDIUM:
        tiles = 18;
        frameSetup.sequence = 1;
        textSetup.x = x - 72;
        break;
    case MONOLITH_TEXT_NARROW:
    default:
        tiles = 12;
        frameSetup.sequence = 8;
        textSetup.x = x - 48;
        break;
    }
    text->bitmap = GFL_BitmapCreate(tiles, 2, 0x20, HEAPID_MONOLITH);
    frameSetup.x = x;
    frameSetup.y = y;
    textSetup.y = y - 8;
    textSetup.bitmap = text->bitmap;
    textSetup.palette = wk->actorRes[sub].fontPalette;
    vramType = sub == 0 ? CLACT_VRAM_MAIN : CLACT_VRAM_SUB;
    textSetup.vramType = vramType;
    textSetup.surface = vramType;
    text->frame = ClActor_Create(wk->clactUnit, wk->actorRes[sub].chars, wk->actorRes[sub].palette,
                                 wk->actorRes[sub].cellAnims, &frameSetup, vramType, HEAPID_MONOLITH);
    text->text = BmpOam_ActorAdd(wk->bmpOam, &textSetup);
    str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
    if (wordSet != NULL) {
        StrBuf *expanded = GFL_StrBufCreate(128, HEAPID_MONOLITH);

        GFL_WordSetFormatStrbuf(wordSet, expanded, str);
        PrintQueue_Print(wk->printQueue, text->bitmap, 0, 0, expanded, wk->font);
        GFL_StrBufFree(expanded);
    } else {
        PrintQueue_Print(wk->printQueue, text->bitmap, 0, 0, str, wk->font);
    }
    GFL_StrBufFree(str);
    text->printing = TRUE;
}

void MonolithTool_CreateTextCentered(MonolithWork *wk, MonolithTextActor *text, int sub, int width, int x, int y,
                                     u32 msgId, WordSet *wordSet, u16 color) {
    ClActorSetup frameSetup = sMonolithTextFrameSetup;
    BmpOamActorSetup textSetup = sMonolithTextSetup;
    int tiles;
    StrBuf *str;
    int textX;
    u16 halfWidth;
    u32 vramType;

    switch (width) {
    case MONOLITH_TEXT_WIDE:
        tiles = 22;
        frameSetup.sequence = 0;
        textSetup.x = x - 88;
        break;
    case MONOLITH_TEXT_MEDIUM:
        tiles = 18;
        frameSetup.sequence = 1;
        textSetup.x = x - 72;
        break;
    case MONOLITH_TEXT_NARROW:
    default:
        tiles = 12;
        frameSetup.sequence = 8;
        textSetup.x = x - 48;
        break;
    }
    text->bitmap = GFL_BitmapCreate(tiles, 2, 0x20, HEAPID_MONOLITH);
    frameSetup.x = x;
    frameSetup.y = y;
    textSetup.y = y - 8;
    textSetup.bitmap = text->bitmap;
    textSetup.palette = wk->actorRes[sub].fontPalette;
    vramType = sub == 0 ? CLACT_VRAM_MAIN : CLACT_VRAM_SUB;
    textSetup.vramType = vramType;
    textSetup.surface = vramType;
    text->frame = ClActor_Create(wk->clactUnit, wk->actorRes[sub].chars, wk->actorRes[sub].palette,
                                 wk->actorRes[sub].cellAnims, &frameSetup, vramType, HEAPID_MONOLITH);
    text->text = BmpOam_ActorAdd(wk->bmpOam, &textSetup);
    str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
    halfWidth = GFL_FontGetBlockWidth(str, wk->font, 0) / 2;
    textX = tiles * 8 / 2 - halfWidth;
    if (wordSet != NULL) {
        StrBuf *expanded = GFL_StrBufCreate(128, HEAPID_MONOLITH);

        GFL_WordSetFormatStrbuf(wordSet, expanded, str);
        PrintQueue_PrintColor(wk->printQueue, text->bitmap, textX, 0, expanded, wk->font, color);
        GFL_StrBufFree(expanded);
    } else {
        PrintQueue_PrintColor(wk->printQueue, text->bitmap, textX, 0, str, wk->font, color);
    }
    GFL_StrBufFree(str);
    text->printing = TRUE;
}

void MonolithTool_DeleteText(MonolithTextActor *text) {
    ClActor_Delete(text->frame);
    BmpOam_ActorDel(text->text);
    GFL_BitmapFree(text->bitmap);
    text->frame = NULL;
    text->text = NULL;
    text->bitmap = NULL;
}

void MonolithTool_SetTextVisible(MonolithTextActor *text, BOOL visible) {
    ClActor_SetVisible(text->frame, visible);
    BmpOam_ActorSetDrawEnable(text->text, visible);
}

BOOL MonolithTool_UpdateText(MonolithWork *wk, MonolithTextActor *text) {
    if (text->printing == TRUE && PrintQueue_IsBitmapPending(wk->printQueue, text->bitmap) == FALSE) {
        BmpOam_ActorBmpTrans(text->text);
        text->printing = FALSE;
        return TRUE;
    }
    return FALSE;
}

void MonolithTool_SetTextCursor(MonolithScreenParam *param, MonolithTextActor *texts, int count, int cursor,
                                int panel) {
    int i;

    for (i = 0; i < count; i++) {
        if (i == cursor) {
            ClActor_SetPalette(texts[i].frame, 1, TRUE);
            MonolithTool_SetPanelMode(param, PANEL_MODE_PULSE, panel);
        } else {
            ClActor_SetPalette(texts[i].frame, 0, TRUE);
        }
    }
    if (cursor == 0xff) {
        MonolithTool_SetPanelMode(param, PANEL_MODE_NONE, panel);
    }
}

void MonolithTool_SetTextPicked(MonolithScreenParam *param, MonolithTextActor *texts, int count, int cursor,
                                int panel) {
    int i;

    for (i = 0; i < count; i++) {
        if (i == cursor) {
            ClActor_SetPalette(texts[i].frame, 1, TRUE);
            MonolithTool_SetPanelMode(param, PANEL_MODE_FLASH, panel);
        } else {
            ClActor_SetPalette(texts[i].frame, 0, TRUE);
        }
    }
}

void MonolithTool_SetPanelPulse(MonolithScreenParam *param, BOOL pulse, int panel) {
    if (pulse == TRUE) {
        MonolithTool_SetPanelMode(param, PANEL_MODE_PULSE, panel);
    } else {
        MonolithTool_SetPanelMode(param, PANEL_MODE_NONE, panel);
    }
}

void MonolithTool_FlashPanel(MonolithScreenParam *param, int panel) {
    MonolithTool_SetPanelMode(param, PANEL_MODE_FLASH, panel);
}

void MonolithTool_InitPanels(MonolithScreenParam *param) {
    int i;

    for (i = 0; i < PANEL_CONTROL_MAX; i++) {
        MI_CpuFill8(&param->panels[i], 0, sizeof(MonolithPanelControl));
    }
}

u8 MonolithTool_GetPanelMode(MonolithScreenParam *param, int panel) {
    return param->panels[panel].mode;
}

void MonolithTool_UpdatePanel(MonolithScreenParam *param, int req) {
    MonolithPanelControl *panel = &param->panels[req];
    int colorPos;
    int targetPos;
    int count;
    int i;
    int fraction;

    GFL_ASSERT(req < PANEL_CONTROL_MAX);
    colorPos = MonolithTool_GetPanelColorPos(req);
    targetPos = MonolithTool_GetPanelTargetPos(req);
    count = MonolithTool_GetPanelColorCount(req);
    switch (panel->mode) {
    case PANEL_MODE_PULSE:
        fraction = panel->fraction;
        if (panel->falling == FALSE) {
            fraction += 0x80;
            if (fraction >= 0x1100) {
                fraction = 0x1000;
                panel->falling ^= 1;
            }
        } else {
            fraction -= 0x80;
            if (fraction <= 0) {
                fraction = 0x100;
                panel->falling ^= 1;
            }
        }
        panel->fraction = fraction;
        for (i = 0; i < count; i++) {
            PaletteFade_BlendBuffer(param->work->fade, req, colorPos + i, 1, fraction >> 8,
                                    PaletteFade_GetColor(param->work->fade, req, 1, targetPos + i));
        }
        break;
    case PANEL_MODE_FLASH:
        if (panel->wait == 0) {
            panel->wait = 3;
            if (panel->fraction == 0) {
                panel->fraction = 0x1000;
            } else {
                panel->fraction = 0;
                panel->flashes++;
                if (panel->flashes > 1) {
                    MonolithTool_SetPanelMode(param, PANEL_MODE_NONE, req);
                }
            }
            for (i = 0; i < count; i++) {
                PaletteFade_BlendBuffer(param->work->fade, req, colorPos + i, 1, panel->fraction >> 8,
                                        PaletteFade_GetColor(param->work->fade, req, 1, targetPos + i));
            }
        } else {
            panel->wait--;
        }
        break;
    }
}

static void MonolithTool_SetPanelMode(MonolithScreenParam *param, u8 mode, int req) {
    MonolithPanelControl *panel = &param->panels[req];

    MI_CpuFill8(panel, 0, sizeof(MonolithPanelControl));
    panel->mode = mode;
    PaletteFade_BlendBuffer(param->work->fade, req, MonolithTool_GetPanelColorPos(req),
                            MonolithTool_GetPanelColorCount(req), 0, 0);
}

// The panels of PaletteFade buffers 0 and 1 (the BGs) blend colors 0x32 to 0x34 toward 0x42 to 0x44, those of 2 and 3
// (the actors) 0x12 to 0x14 toward 0x22 to 0x24
static u32 MonolithTool_GetPanelColorPos(u32 req) {
    if (req <= 1) {
        return 0x32;
    }
    return 0x12;
}

static u32 MonolithTool_GetPanelTargetPos(u32 req) {
    if (req <= 1) {
        return 0x42;
    }
    return 0x22;
}

static u32 MonolithTool_GetPanelColorCount(u32 req) {
    return 3;
}

PlayerInfo *MonolithTool_GetPlayerInfo(MonolithScreenParam *param) {
    return GetGameDataPlayerInfo(GSYS_GetGameData(param->param->gsys));
}

PassPowerLevel *MonolithTool_GetLevels(MonolithScreenParam *param) {
    return GameData_GetMyPassPowerLevels(GSYS_GetGameData(param->param->gsys));
}

void MonolithTool_CreateReturnButton(MonolithWork *wk, MonolithReturnButton *button) {
    ClActorSetup setup = { 240, 180, 6, 0, 2 };
    ClActor *actor = ClActor_Create(wk->clactUnit, wk->actorRes[1].chars, wk->actorRes[1].palette,
                                    wk->actorRes[1].cellAnims, &setup, CLACT_VRAM_SUB, HEAPID_MONOLITH);

    func_0204c520(actor, TRUE);
    button->actor = actor;
}

void MonolithTool_DeleteReturnButton(MonolithReturnButton *button) {
    ClActor_Delete(button->actor);
}

void MonolithTool_UpdateReturnButton(MonolithReturnButton *button) {
    if (button->blinking == TRUE) {
        if (button->timer % 5 == 0) {
            ClActor_SetPalette(button->actor, button->lit, TRUE);
            button->lit ^= 1;
            if (button->timer / 5 == 3) {
                button->blinking = FALSE;
            }
        }
        button->timer++;
    }
}

void MonolithTool_PressReturnButton(MonolithReturnButton *button) {
    button->blinking = TRUE;
    button->timer = 0;
    button->lit = TRUE;
}

BOOL MonolithTool_IsReturnButtonBlinking(MonolithReturnButton *button) {
    return button->blinking;
}

ClActor *MonolithTool_CreateActor(MonolithWork *wk, s16 x, s16 y, u16 sequence) {
    ClActorSetup setup = { x, y, sequence, 0, 1 };
    ClActor *actor;

    actor = ClActor_Create(wk->clactUnit, wk->actorRes[1].chars, wk->actorRes[1].palette, wk->actorRes[1].cellAnims,
                           &setup, CLACT_VRAM_SUB, HEAPID_MONOLITH);
    func_0204c520(actor, TRUE);
    return actor;
}

void MonolithTool_DeleteActor(ClActor *actor) {
    ClActor_Delete(actor);
}

void MonolithTool_UpdateActor(ClActor *actor) {
}

AppTaskMenuRes *MonolithTool_CreateMenuRes(MonolithWork *wk, u32 bg, HeapID heapId) {
    return AppTaskMenuRes_Create(bg, 14, wk->font, wk->printQueue, heapId);
}

void MonolithTool_FreeMenuRes(AppTaskMenuRes *res) {
    AppTaskMenuRes_Free(res);
}

AppTaskMenu *MonolithTool_CreateYesNoMenu(MonolithWork *wk, AppTaskMenuRes *res, HeapID heapId) {
    AppTaskMenuItem items[2];
    AppTaskMenuInit init;
    AppTaskMenu *menu;

    items[0].str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 32);
    items[1].str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 33);
    items[0].color = MONOLITH_MENU_TEXT_COLOR;
    items[1].color = MONOLITH_MENU_TEXT_COLOR;
    items[0].type = 0;
    items[1].type = 0;
    init.heapId = heapId;
    init.itemCount = 2;
    init.items = items;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 24;
    init.y = 12;
    init.width = 8;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, res);
    GFL_StrBufFree(items[0].str);
    GFL_StrBufFree(items[1].str);
    return menu;
}

void MonolithTool_FreeYesNoMenu(AppTaskMenu *menu) {
    AppTaskMenu_Free(menu);
}

BOOL MonolithTool_UpdateYesNoMenu(MonolithWork *wk, int unused, AppTaskMenu *menu, BOOL *yes) {
    AppTaskMenu_Update(menu);
    if (AppTaskMenu_IsFlashFinished(menu) == TRUE) {
        *yes = AppTaskMenu_GetCursorPos(menu) == 0 ? TRUE : FALSE;
        return TRUE;
    }
    return FALSE;
}

AppTaskMenu *MonolithTool_CreateReturnPowerMenu(MonolithScreenParam *param, MonolithWork *wk, AppTaskMenuRes *res,
                                                HeapID heapId) {
    AppTaskMenuItem items[4];
    AppTaskMenuInit init;
    AppTaskMenu *menu;
    int i;

    for (i = 0; i < 4; i++) {
        if (i != 3) {
            PassPowerData *powers = wk->passPowerData;

            items[i].str = GFL_MsgDataLoadStrbufNew(wk->msgPowerNames, powers[param->state->equipped[i]].name);
            items[i].type = 0;
        } else {
            items[i].str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 74);
            items[i].type = APP_TASKMENU_ITEM_RETURN;
        }
        items[i].color = MONOLITH_MENU_TEXT_COLOR;
    }
    init.heapId = heapId;
    init.itemCount = 4;
    init.items = items;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 12;
    init.y = 6;
    init.width = 20;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, res);
    for (i = 0; i < 4; i++) {
        GFL_StrBufFree(items[i].str);
    }
    return menu;
}

void MonolithTool_FreeReturnPowerMenu(AppTaskMenu *menu) {
    AppTaskMenu_Free(menu);
}

BOOL MonolithTool_UpdateReturnPowerMenu(MonolithWork *wk, AppTaskMenu *menu, u32 *pos) {
    AppTaskMenu_Update(menu);
    if (AppTaskMenu_IsFlashFinished(menu) == TRUE) {
        *pos = AppTaskMenu_GetCursorPos(menu);
        return TRUE;
    }
    return FALSE;
}

void MonolithTool_DrawLevelBar(MonolithScreenParam *param, BOOL top, u32 bg) {
    PassPowerLevel *levels = MonolithTool_GetLevels(param);
    int row = 20;
    int white;
    int black;
    int whiteWidth;
    int blackWidth;
    u16 *screen;
    int i;

    if (top) {
        row = 14;
    }
    white = levels->level1;
    black = levels->level2;
    if (white == black) {
        whiteWidth = 120;
        blackWidth = 120;
    } else {
        whiteWidth = 240 * white / (white + black);
        if (black > 0) {
            blackWidth = 240 - whiteWidth;
        } else {
            blackWidth = 0;
            whiteWidth = 240;
        }
    }
    // A level above 0 shows at least a pixel
    if (white > 0 && whiteWidth == 0) {
        blackWidth--;
    } else if (black > 0 && blackWidth == 0) {
        blackWidth++;
    }
    screen = GFL_HeapAllocate(HEAPID_MONOLITH, 30 * sizeof(u16), TRUE, "monolith_tool.c", 1322);
    for (i = 0; blackWidth > 0; i++) {
        if (blackWidth >= 8) {
            screen[i] = 1;
            blackWidth -= 8;
        } else {
            screen[i] = 9 - blackWidth;
            blackWidth = 0;
        }
    }
    for (; i < 30; i++) {
        screen[i] = 9;
    }
    for (i = 0; i < 30; i++) {
        screen[i] |= 1 << 12;
    }
    GFL_BGSysLoadScrAreaAll(bg, screen, 1, row, 30, 1);
    GFL_HeapFree(screen);
}
