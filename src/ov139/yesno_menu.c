// yesno_menu.c: the menu of two choices with which the evolution demo asks about learning a move. The names are ours

#include "app/ui/yesno_menu.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "system/bmp_winframe.h"

static u8 TwoChoiceMenu_GetActorPos(u32 cursor, u32 index, u32 axis);
static void TwoChoiceMenu_LoadPalette(u32 type, u32 palette, const u16 *src);

typedef struct {
    u16 timer;
    u16 palette;
} TwoChoiceMenuFlash;

TwoChoiceMenu *TwoChoiceMenu_Create(HeapID heapId, u32 textBg, u32 priority, u32 palette, u8 objPalette,
                                   ClActUnit *unit, Font *font, PrintQueue *queue, u32 a8) {
    TwoChoiceMenu *menu;
    ArcTool *arc;
    void *file;
    NNSG2dPaletteData *paletteData;
    UIObjResSetup setup;
    u16 *colors;
    u32 palType = 0;
    BOOL isSub;
    u8 i;
    u8 j;

    menu = GFL_HeapAllocate(heapId, sizeof(TwoChoiceMenu), FALSE, "yesno_menu.c", 211);
    sys_memset(menu, 0, sizeof(TwoChoiceMenu));
    menu->heapId = heapId;
    menu->unit = unit;
    menu->font = font;
    menu->queue = queue;
    if (textBg > 3) {
        palType = 4;
    }
    menu->palType = palType;
    menu->frameBg = textBg + 1;
    menu->textBg = textBg;
    menu->framePriority = priority + 1;
    menu->actorPriority = priority;
    menu->framePalette = palette;
    menu->framePalette2 = palette + 1;
    menu->textPalette = palette + 2;
    isSub = FALSE;
    if (textBg > 3) {
        isSub = TRUE;
    }
    menu->isSub = isSub;
    menu->objPalette = objPalette;
    GFL_BGSysClearBG(menu->frameBg);
    GFL_BGSysClearBG(menu->textBg);
    menu->state = TWO_CHOICE_MENU_STATE_CLOSED;
    menu->result = TWO_CHOICE_MENU_NONE;
    menu->cursor = 0;

    arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, menu->heapId);
    file = GFL_HeapAllocate(menu->heapId, GFL_ArcToolGetDataLength(arc, 163), FALSE, "yesno_menu.c", 269);
    GFL_ArcToolRead(arc, 163, file);
    NNS_G2dGetUnpackedPaletteData(file, &paletteData);
    colors = paletteData->rawData;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 16; j++) {
            menu->palettes[i][j] = colors[(i + 1) * 16 + j];
        }
    }
    GFL_HeapFree(file);
    GFL_G2DIOLoadArcNCLRDefault(arc, 163, menu->palType, menu->framePalette * 32, 0x40, menu->heapId);
    menu->frameChars = GFL_BGSysLoadArcNCGRDynamic(arc, 164, menu->frameBg, 0x1a0, FALSE, menu->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 165, menu->frameBg, CHAR_POS(menu->frameChars), 0x600, FALSE,
                                           menu->heapId);
    GFL_BGSysSetScrPaletteNo(menu->frameBg, 0, 0, 32, 11, menu->framePalette);
    GFL_BGSysSetScrPaletteNo(menu->frameBg, 0, 11, 32, 13, menu->framePalette2);
    GFL_BGSysLoadScr(menu->frameBg);
    GFL_ArcToolFree(arc);
    TwoChoiceMenu_LoadPalette(menu->palType, menu->framePalette, menu->palettes[0]);
    TwoChoiceMenu_LoadPalette(menu->palType, menu->framePalette2, menu->palettes[0]);
    GFL_BGSysSetBGEnabled(menu->frameBg, FALSE);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, menu->palType, menu->textPalette * 32, 0x20, menu->heapId);
    menu->firstWindow = BmpWin_CreateDynamic(menu->textBg, 0, 5, 32, 6, menu->textPalette, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(menu->firstWindow), 0);
    menu->secondWindow = BmpWin_CreateDynamic(menu->textBg, 0, 11, 32, 6, menu->textPalette, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(menu->secondWindow), 0);
    GFL_BGSysSetBGEnabled(menu->textBg, FALSE);

    setup.vramType = menu->isSub;
    setup.flags = 0;
    setup.arcId = 11;
    setup.paletteFile = 421;
    setup.charFile = 424;
    setup.cellFile = 425;
    setup.animFile = 426;
    setup.paletteOffset = menu->objPalette;
    setup.paletteStart = 0;
    setup.paletteCount = 5;
    UIObjRes_Load(&menu->objRes, &setup, menu->unit, menu->heapId);
    for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
        menu->actors[i] = UIObjRes_CreateActor(&menu->objRes, menu->unit, TwoChoiceMenu_GetActorPos(menu->cursor, i, 0),
                                              TwoChoiceMenu_GetActorPos(menu->cursor, i, 1), i, menu->heapId);
        func_0204c468(menu->actors[i], menu->actorPriority);
        func_0204c124(menu->actors[i], FALSE);
        func_0204c520(menu->actors[i], FALSE);
    }
    if (menu->isSub == FALSE) {
        GFL_BGSysSetBGEnabledA(0x10, TRUE);
    } else {
        GFL_BGSysSetBGEnabledB(0x10, TRUE);
    }
    return menu;
}

void TwoChoiceMenu_Free(TwoChoiceMenu *menu) {
    u8 i;

    for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
        func_0204c108(menu->actors[i]);
    }
    UIObjRes_Free(&menu->objRes);
    BmpWin_Free(menu->firstWindow);
    BmpWin_Free(menu->secondWindow);
    GFL_BGSysFreeCharMemory(menu->frameBg, CHAR_POS(menu->frameChars), CHAR_SIZE(menu->frameChars));
    GFL_HeapFree(menu);
}

void TwoChoiceMenu_Open(TwoChoiceMenu *menu, StrBuf *first, StrBuf *second) {
    u8 i;
    u32 width;
    u16 x;

    menu->state = TWO_CHOICE_MENU_STATE_OPENING;
    menu->result = TWO_CHOICE_MENU_NONE;
    menu->cursor = 0;
    for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
        ClActorPos pos;

        pos.x = TwoChoiceMenu_GetActorPos(menu->cursor, i, 0);
        pos.y = TwoChoiceMenu_GetActorPos(menu->cursor, i, 1);
        func_0204c140(menu->actors[i], &pos, menu->isSub);
    }
    TwoChoiceMenu_LoadPalette(menu->palType, menu->framePalette, menu->palettes[0]);
    TwoChoiceMenu_LoadPalette(menu->palType, menu->framePalette2, menu->palettes[0]);
    GFL_BGSysSetBGEnabled(menu->frameBg, TRUE);

    GFL_BitmapFill(BmpWin_GetBitmap(menu->firstWindow), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(menu->secondWindow), 0);
    width = GFL_FontGetBlockWidth(first, menu->font, 0);
    x = (256 - width) / 2;
    func_02021c7c(menu->queue, BmpWin_GetBitmap(menu->firstWindow), x, 16, first, menu->font, PRINT_COLOR(15, 2, 0));
    width = GFL_FontGetBlockWidth(second, menu->font, 0);
    x = (256 - width) / 2;
    func_02021c7c(menu->queue, BmpWin_GetBitmap(menu->secondWindow), x, 16, second, menu->font,
                  PRINT_COLOR(15, 2, 0));
    menu->firstSent = FALSE;
    menu->secondSent = FALSE;
    GFL_BGSysSetBGEnabled(menu->textBg, TRUE);
    if (!menu->firstSent && !func_02021c1c(menu->queue, BmpWin_GetBitmap(menu->firstWindow))) {
        BmpWin_Transfer(menu->firstWindow);
        menu->firstSent = TRUE;
    }
    if (!menu->secondSent && !func_02021c1c(menu->queue, BmpWin_GetBitmap(menu->secondWindow))) {
        BmpWin_Transfer(menu->secondWindow);
        menu->secondSent = TRUE;
    }
    if (!func_0203d554()) {
        for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
            func_0204c124(menu->actors[i], TRUE);
            func_0204c520(menu->actors[i], TRUE);
        }
    }
}

void TwoChoiceMenu_Close(TwoChoiceMenu *menu) {
    u8 i;

    GFL_BGSysSetBGEnabled(menu->frameBg, FALSE);
    GFL_BGSysSetBGEnabled(menu->textBg, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(menu->firstWindow), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(menu->secondWindow), 0);
    BmpWin_Transfer(menu->firstWindow);
    BmpWin_Transfer(menu->secondWindow);
    for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
        func_0204c124(menu->actors[i], FALSE);
        func_0204c520(menu->actors[i], FALSE);
    }
    menu->state = TWO_CHOICE_MENU_STATE_CLOSED;
}

void TwoChoiceMenu_Main(TwoChoiceMenu *menu) {
    static const TwoChoiceMenuFlash flashes[TWO_CHOICE_MENU_FLASH_STEPS] = {
        { 0, 0 }, { 1, 1 }, { 2, 2 }, { 4, 1 }, { 5, 0 }, { 7, 1 }, { 8, 2 }, { 10, 1 }, { 11, 0 }, { 13, 0 },
    };
    BOOL changePalette = FALSE;
    u32 keys = GCTX_HIDGetPressedKeys();
    u32 touchX;
    u32 touchY;
    BOOL touched = func_0203dac8(&touchX, &touchY);
    u8 i;

    switch (menu->state) {
    case TWO_CHOICE_MENU_STATE_CLOSED:
        break;
    case TWO_CHOICE_MENU_STATE_OPENING:
        menu->state = TWO_CHOICE_MENU_STATE_CHOOSING;
        break;
    case TWO_CHOICE_MENU_STATE_CHOOSING: {
        BOOL decided = FALSE;

        if (keys & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL2);
            menu->result = TWO_CHOICE_MENU_SECOND;
            decided = TRUE;
            func_0203d564(FALSE);
        }
        if (!decided && !func_0203d554() && keys != 0) {
            BOOL moved = FALSE;

            if (keys & PAD_BUTTON_A) {
                if (menu->cursor == 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    menu->result = TWO_CHOICE_MENU_FIRST;
                } else {
                    GFL_SndSEPlay(SEQ_SE_CANCEL2);
                    menu->result = TWO_CHOICE_MENU_SECOND;
                }
                decided = TRUE;
            } else if (keys & PAD_KEY_UP) {
                if (menu->cursor != 0) {
                    menu->cursor = 0;
                    moved = TRUE;
                }
            } else if (keys & PAD_KEY_DOWN) {
                if (menu->cursor != 1) {
                    menu->cursor = 1;
                    moved = TRUE;
                }
            }
            if (moved) {
                for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
                    ClActorPos pos;

                    pos.x = TwoChoiceMenu_GetActorPos(menu->cursor, i, 0);
                    pos.y = TwoChoiceMenu_GetActorPos(menu->cursor, i, 1);
                    func_0204c140(menu->actors[i], &pos, menu->isSub);
                }
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
        if (!decided && touched) {
            const TouchRect rects[] = {
                { 0x20, 0x4f, 0x08, 0xf7 },
                { 0x50, 0x7f, 0x08, 0xf7 },
                { TOUCH_RECT_END, 0, 0, 0 },
            };
            s32 hit = func_0203dadc(rects, touchX, touchY);

            if (hit != TOUCH_RECT_NONE) {
                if (hit == 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    menu->result = TWO_CHOICE_MENU_FIRST;
                } else if (hit == 1) {
                    GFL_SndSEPlay(SEQ_SE_CANCEL2);
                    menu->result = TWO_CHOICE_MENU_SECOND;
                }
                decided = TRUE;
                func_0203d564(TRUE);
            }
        }
        if (!decided && func_0203d554() == TRUE && keys != 0) {
            for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
                func_0204c124(menu->actors[i], TRUE);
                func_0204c520(menu->actors[i], TRUE);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0203d564(FALSE);
        }
        if (decided) {
            for (i = 0; i < TWO_CHOICE_MENU_ACTORS; i++) {
                func_0204c124(menu->actors[i], FALSE);
                func_0204c520(menu->actors[i], FALSE);
            }
            menu->state = TWO_CHOICE_MENU_STATE_FLASHING;
            menu->flashPalette = 0;
            menu->flashTimer = 1;
            menu->flashStep = 1;
        }
        break;
    }
    case TWO_CHOICE_MENU_STATE_FLASHING:
        if (menu->flashStep < TWO_CHOICE_MENU_FLASH_STEPS) {
            if (menu->flashTimer == flashes[menu->flashStep].timer) {
                menu->flashPalette = flashes[menu->flashStep].palette;
                changePalette = TRUE;
                menu->flashStep++;
            }
            menu->flashTimer++;
        } else {
            menu->state = TWO_CHOICE_MENU_STATE_DONE;
        }
        break;
    case TWO_CHOICE_MENU_STATE_DONE:
        break;
    }

    if (menu->state != TWO_CHOICE_MENU_STATE_CLOSED) {
        if (!menu->firstSent && !func_02021c1c(menu->queue, BmpWin_GetBitmap(menu->firstWindow))) {
            BmpWin_Transfer(menu->firstWindow);
            menu->firstSent = TRUE;
        }
        if (!menu->secondSent && !func_02021c1c(menu->queue, BmpWin_GetBitmap(menu->secondWindow))) {
            BmpWin_Transfer(menu->secondWindow);
            menu->secondSent = TRUE;
        }
    }
    if (changePalette) {
        u8 palette = menu->result == TWO_CHOICE_MENU_FIRST ? menu->framePalette : menu->framePalette2;

        TwoChoiceMenu_LoadPalette(menu->palType, palette, menu->palettes[menu->flashPalette]);
    }
}

u32 TwoChoiceMenu_GetResult(TwoChoiceMenu *menu) {
    if (menu->state != TWO_CHOICE_MENU_STATE_CLOSED && menu->state != TWO_CHOICE_MENU_STATE_DONE) {
        return TWO_CHOICE_MENU_NONE;
    }
    return menu->result;
}

// The position of an actor of the menu, one axis at a time (0 for x, 1 for y)
static u8 TwoChoiceMenu_GetActorPos(u32 cursor, u32 index, u32 axis) {
    u8 cursorPos[2][2] = { { 0x00, 0x28 }, { 0x00, 0x58 } };
    u8 actorPos[4][2] = { { 0x00, 0x00 }, { 0x00, 0x2f }, { 0xff, 0x00 }, { 0xff, 0x2f } };

    return cursorPos[cursor][axis] + actorPos[index][axis];
}

// Transfers a palette of the type to the VRAM at the next VBlank
static void TwoChoiceMenu_LoadPalette(u32 type, u32 palette, const u16 *src) {
    NNS_GFD_DST_TYPE dst;

    switch (type) {
    case 0:
        dst = NNS_GFD_DST_2D_BG_PLTT_MAIN;
        break;
    case 1:
        dst = NNS_GFD_DST_2D_OBJ_PLTT_MAIN;
        break;
    case 4:
        dst = NNS_GFD_DST_2D_BG_PLTT_SUB;
        break;
    case 5:
        dst = NNS_GFD_DST_2D_OBJ_PLTT_SUB;
        break;
    default:
        return;
    }
    NNS_GfdRegisterNewVramTransferTask(dst, palette * 32, src, 32);
}
