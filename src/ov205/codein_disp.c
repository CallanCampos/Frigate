#include "types.h"
#include "app/codein.h"
#include "app/codein_pv.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/wordset.h"
#include "text/system/codein_disp.h"

// The code input screen's graphics. The ROM has no name for this file; "codein_disp.c" is a guess from pokeplatinum,
// whose code input screen has the same three files. Our names

// The digits' animations: the ten large digits after an empty place, then the same small. The bar follows
#define SEQ_SMALL 11
#define SEQ_BAR 22
// The animation of the cursor that flashes on a pressed key
#define SEQ_CURSOR_PRESS 3

#define CODE_X 76
#define CODE_Y 24
#define CODE_PRIORITY 10
// The frames a button's press animation takes
#define BUTTON_FRAMES 10
#define BUTTON_TEXT_COLOR PRINT_COLOR(15, 14, 0)
// The window and frame of the upper screen's message
#define MESSAGE_BG 4
#define MESSAGE_PALETTE 12
#define FRAME_CHAR 1
#define FRAME_PALETTE 11

static void CodeInDisp_SetButtonTextPos(BmpOamActor *actor, int x, int y);
static void CodeInDisp_FreeBmpOam(CodeInWork *wk);
static void CodeInDisp_LoadButtonTextPltt(CodeInWork *wk);
static void CodeInDisp_AddButtonText(CodeInWork *wk, int button, int x, int y, int unused);

void CodeInDisp_InitActors(CodeInWork *wk, const BGSysVRAMConfig *vramConfig) {
    ClActSysSetup setup = data_02093f08;

    setup.oamStartMain = 16;
    setup.oamCountMain = 112;
    ClActSys_Create(&setup, vramConfig, HEAPID_CODEIN);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    wk->unit = func_0204bf1c(64, 0, HEAPID_CODEIN);
    func_0204c028(wk->unit);
}

void CodeInDisp_LoadResources(CodeInWork *wk) {
    PaletteFade *fade = wk->fade;
    ArcTool *arc = wk->arc;

    GFL_BGSysLoadArcNCGRStatic(arc, 6, 1, 0, 0, FALSE, HEAPID_CODEIN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 1, 0, 0, FALSE, HEAPID_CODEIN);
    PaletteFade_LoadNCLR(fade, ARCID_CODEIN, 1, HEAPID_CODEIN, 0, 0x40, 0);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 5, 0, 0, FALSE, HEAPID_CODEIN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 11, 5, 0, 0, FALSE, HEAPID_CODEIN);
    GFL_G2DIOLoadArcNCLRDefault(arc, 2, 4, 0, 0, HEAPID_CODEIN);
    wk->plttRes[CODEIN_RES_CODE] = func_0204bba0(arc, 0, CLACT_VRAM_MAIN, 0, HEAPID_CODEIN);
    wk->charRes[CODEIN_RES_CODE] = func_0204b81c(arc, 5, FALSE, CLACT_VRAM_MAIN, HEAPID_CODEIN);
    wk->cellAnimRes[CODEIN_RES_CODE] = func_0204bde0(arc, 12, 15, HEAPID_CODEIN);
    wk->plttRes[CODEIN_RES_CURSOR] = func_0204bba0(arc, 4, CLACT_VRAM_MAIN, 0x20, HEAPID_CODEIN);
    wk->charRes[CODEIN_RES_CURSOR] = func_0204b81c(arc, 9, FALSE, CLACT_VRAM_MAIN, HEAPID_CODEIN);
    wk->cellAnimRes[CODEIN_RES_CURSOR] = func_0204bde0(arc, 14, 17, HEAPID_CODEIN);
    wk->plttRes[CODEIN_RES_BUTTON] = func_0204bba0(arc, 3, CLACT_VRAM_MAIN, 0x40, HEAPID_CODEIN);
    wk->charRes[CODEIN_RES_BUTTON] = func_0204b81c(arc, 8, FALSE, CLACT_VRAM_MAIN, HEAPID_CODEIN);
    wk->cellAnimRes[CODEIN_RES_BUTTON] = func_0204bde0(arc, 13, 16, HEAPID_CODEIN);
    LoadSysMsgBox(MESSAGE_BG, FRAME_CHAR, FRAME_PALETTE, 0, HEAPID_CODEIN);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, MESSAGE_PALETTE * 0x20, 0x20, HEAPID_CODEIN);
}

void CodeInDisp_Free(CodeInWork *wk) {
    int i;

    for (i = 0; i < wk->codeMax; i++) {
        func_0204c108(wk->code[i].actor);
    }
    for (i = 0; i < CODEIN_BAR_COUNT; i++) {
        func_0204c108(wk->bar[i].actor);
        func_0204c108(wk->btn[i].actor);
    }
    for (i = 0; i < CODEIN_CURSOR_COUNT; i++) {
        func_0204c108(wk->cur[i].actor);
    }
    CodeInDisp_FreeBmpOam(wk);
    BmpWin_Free(wk->window);
    for (i = 0; i < CODEIN_RES_COUNT; i++) {
        func_0204bcd0(wk->plttRes[i]);
        func_0204b98c(wk->charRes[i]);
        func_0204be64(wk->cellAnimRes[i]);
    }
    func_0204bf98(wk->unit);
    func_0204b758();
}

// Adds the digits' actors, with a bar after each of the first two groups
void CodeInDisp_AddCodeActors(CodeInWork *wk) {
    int i;
    int bar;
    int digit = 0;
    ClActUnit *unit = wk->unit;
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = CODE_PRIORITY;
    setup.bgPriority = 0;
    bar = 0;
    for (i = 0; i < wk->codeMax + CODEIN_BAR_COUNT; i++) {
        if (i == wk->bar[bar].state + bar + 1) {
            setup.x = CODE_X + i * 8;
            setup.y = CODE_Y;
            setup.sequence = SEQ_BAR;
            wk->bar[bar].actor = func_0204c040(unit, wk->charRes[CODEIN_RES_CODE], wk->plttRes[CODEIN_RES_CODE],
                                               wk->cellAnimRes[CODEIN_RES_CODE], &setup, 0, HEAPID_CODEIN);
            bar++;
        } else {
            setup.x = CODE_X + i * 8;
            setup.y = CODE_Y;
            setup.sequence = CodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size);
            wk->code[digit].actor = func_0204c040(unit, wk->charRes[CODEIN_RES_CODE], wk->plttRes[CODEIN_RES_CODE],
                                                  wk->cellAnimRes[CODEIN_RES_CODE], &setup, 0, HEAPID_CODEIN);
            digit++;
        }
    }
    // A single number has no bars
    if (wk->param.blocks[1] == 0 && wk->param.blocks[2] == 0) {
        func_0204c124(wk->bar[0].actor, FALSE);
        func_0204c124(wk->bar[1].actor, FALSE);
    }
}

void CodeInDisp_AddCursorActors(CodeInWork *wk) {
    ClActUnit *unit = wk->unit;
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    wk->cur[CODEIN_CURSOR_CODE].actor =
        func_0204c040(unit, wk->charRes[CODEIN_RES_CURSOR], wk->plttRes[CODEIN_RES_CURSOR],
                      wk->cellAnimRes[CODEIN_RES_CURSOR], &setup, 0, HEAPID_CODEIN);
    wk->cur[CODEIN_CURSOR_KEY].actor =
        func_0204c040(unit, wk->charRes[CODEIN_RES_CURSOR], wk->plttRes[CODEIN_RES_CURSOR],
                      wk->cellAnimRes[CODEIN_RES_CURSOR], &setup, 0, HEAPID_CODEIN);
    wk->cur[CODEIN_CURSOR_PRESS].actor =
        func_0204c040(unit, wk->charRes[CODEIN_RES_CURSOR], wk->plttRes[CODEIN_RES_CURSOR],
                      wk->cellAnimRes[CODEIN_RES_CURSOR], &setup, 0, HEAPID_CODEIN);
    CodeInDisp_SetCodeCursor(wk, 0);
    func_0204c488(wk->cur[CODEIN_CURSOR_CODE].actor, 0);
    func_0204c520(wk->cur[CODEIN_CURSOR_CODE].actor, TRUE);
    wk->cur[CODEIN_CURSOR_KEY].move.x = 0;
    wk->cur[CODEIN_CURSOR_KEY].move.y = 0;
    wk->cur[CODEIN_CURSOR_KEY].state = 1;
    CodeInDisp_SetKeyCursor(wk, 0);
    func_0204c488(wk->cur[CODEIN_CURSOR_KEY].actor, wk->cur[CODEIN_CURSOR_KEY].state);
    func_0204c520(wk->cur[CODEIN_CURSOR_KEY].actor, TRUE);
    func_0204c318(wk->cur[CODEIN_CURSOR_KEY].actor, GX_OAM_MODE_XLU);
    wk->cur[CODEIN_CURSOR_PRESS].move.x = 0;
    wk->cur[CODEIN_CURSOR_PRESS].move.y = 0;
    wk->cur[CODEIN_CURSOR_PRESS].state = 1;
    CodeInDisp_SetKeyCursor(wk, 0);
    func_0204c488(wk->cur[CODEIN_CURSOR_PRESS].actor, wk->cur[CODEIN_CURSOR_PRESS].state);
    func_0204c318(wk->cur[CODEIN_CURSOR_PRESS].actor, GX_OAM_MODE_XLU);
    func_0204c520(wk->cur[CODEIN_CURSOR_PRESS].actor, TRUE);
    CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, FALSE);
    CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_PRESS, FALSE);
}

void CodeInDisp_AddButtonActors(CodeInWork *wk) {
    ClActUnit *unit = wk->unit;
    ClActorSetup setup;
    ClActorPos pos;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = CODE_PRIORITY;
    setup.bgPriority = 0;
    wk->btn[0].actor = func_0204c040(unit, wk->charRes[CODEIN_RES_BUTTON], wk->plttRes[CODEIN_RES_BUTTON],
                                     wk->cellAnimRes[CODEIN_RES_BUTTON], &setup, 0, HEAPID_CODEIN);
    wk->btn[1].actor = func_0204c040(unit, wk->charRes[CODEIN_RES_BUTTON], wk->plttRes[CODEIN_RES_BUTTON],
                                     wk->cellAnimRes[CODEIN_RES_BUTTON], &setup, 0, HEAPID_CODEIN);
    pos.x = (wk->rects[CODEIN_RECT_BACK].left + wk->rects[CODEIN_RECT_BACK].right) / 2;
    pos.y = (wk->rects[CODEIN_RECT_BACK].top + wk->rects[CODEIN_RECT_BACK].bottom) / 2;
    func_0204c140(wk->btn[0].actor, &pos, 0);
    func_0204c488(wk->btn[0].actor, 0);
    pos.x = (wk->rects[CODEIN_RECT_OK].left + wk->rects[CODEIN_RECT_OK].right) / 2;
    pos.y = (wk->rects[CODEIN_RECT_OK].top + wk->rects[CODEIN_RECT_OK].bottom) / 2;
    func_0204c140(wk->btn[1].actor, &pos, 0);
    func_0204c488(wk->btn[1].actor, 0);
}

void CodeInDisp_SetCursorVisible(CodeInWork *wk, int cursor, BOOL visible) {
    func_0204c124(wk->cur[cursor].actor, visible);
}

// Puts the code's cursor under a digit
void CodeInDisp_SetCodeCursor(CodeInWork *wk, int digit) {
    ClActorPos pos;
    ClActor *actor = wk->code[digit].actor;

    wk->cur[CODEIN_CURSOR_CODE].state = digit;
    func_0204c178(actor, &pos, 0);
    pos.y += 16;
    func_0204c140(wk->cur[CODEIN_CURSOR_CODE].actor, &pos, 0);
}

// Puts the keys' cursor on a key and restarts its animation
void CodeInDisp_SetKeyCursor(CodeInWork *wk, int key) {
    ClActorPos pos;

    pos.x = (wk->rects[CODEIN_RECT_KEYS + key].left + wk->rects[CODEIN_RECT_KEYS + key].right) / 2;
    pos.y = (wk->rects[CODEIN_RECT_KEYS + key].top + wk->rects[CODEIN_RECT_KEYS + key].bottom) / 2;
    func_0204c140(wk->cur[CODEIN_CURSOR_KEY].actor, &pos, 0);
    func_0204c4d4(wk->cur[CODEIN_CURSOR_KEY].actor, 0);
}

void CodeInDisp_SetCursorOnKey(CodeInWork *wk, int key, int cursor) {
    ClActorPos pos;

    pos.x = (wk->rects[CODEIN_RECT_KEYS + key].left + wk->rects[CODEIN_RECT_KEYS + key].right) / 2;
    pos.y = (wk->rects[CODEIN_RECT_KEYS + key].top + wk->rects[CODEIN_RECT_KEYS + key].bottom) / 2;
    func_0204c140(wk->cur[cursor].actor, &pos, 0);
    func_0204c4d4(wk->cur[cursor].actor, 0);
}

// Gives the keys' cursors their animations, and hides the keys' cursor while a press flashes or the touch screen is
// in use
void CodeInDisp_UpdateCursors(CodeInWork *wk) {
    int i;

    for (i = CODEIN_CURSOR_KEY; i < CODEIN_CURSOR_COUNT; i++) {
        int sequence = func_0204c4a0(wk->cur[i].actor);

        if (sequence == SEQ_CURSOR_PRESS) {
            if (!func_0204c560(wk->cur[i].actor)) {
                func_0204c488(wk->cur[i].actor, wk->cur[i].state);
                if (wk->touchMode == TRUE) {
                    CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, FALSE);
                } else {
                    CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, TRUE);
                }
                CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_PRESS, FALSE);
            }
        } else {
            if (sequence != wk->cur[i].state) {
                func_0204c488(wk->cur[i].actor, wk->cur[i].state);
            }
            if (func_0204c4a0(wk->cur[i].actor) != SEQ_CURSOR_PRESS) {
                if (wk->touchMode == TRUE) {
                    CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, FALSE);
                } else {
                    CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, TRUE);
                }
            }
        }
    }
}

static void CodeInDisp_SetButtonTextPos(BmpOamActor *actor, int x, int y) {
    if (actor != NULL) {
        BmpOam_ActorSetPos(actor, x, y);
    }
}

// Runs the press animation of Back and OK
void CodeInDisp_UpdateButtons(CodeInWork *wk) {
    int i;

    for (i = 0; i < CODEIN_BUTTON_COUNT; i++) {
        s16 x = wk->rects[CODEIN_RECT_BACK + i].left + 20;
        s16 y = (wk->rects[CODEIN_RECT_BACK + i].top + wk->rects[CODEIN_RECT_BACK + i].bottom) / 2;

        y -= 7;

        switch (wk->btn[i].state) {
        case 0:
            wk->btn[i].move.wait = 0;
            break;
        case 1:
            wk->btn[i].move.wait++;
            if (wk->btn[i].move.wait == 1) {
                func_0204c488(wk->btn[i].actor, 1);
                CodeInDisp_SetButtonTextPos(wk->btnText[i], x, y);
            } else if (wk->btn[i].move.wait == 2) {
                func_0204c488(wk->btn[i].actor, 2);
                CodeInDisp_SetButtonTextPos(wk->btnText[i], x, y);
            } else if (wk->btn[i].move.wait == BUTTON_FRAMES) {
                func_0204c488(wk->btn[i].actor, 0);
                CodeInDisp_SetButtonTextPos(wk->btnText[i], x, y);
                wk->btn[i].state++;
            }
            break;
        default:
            func_0204c488(wk->btn[i].actor, 0);
            CodeInDisp_SetButtonTextPos(wk->btnText[i], x, y);
            wk->btn[i].state = 0;
            break;
        }
    }
}

int CodeInDisp_GetCodeSequence(int state, int size) {
    int base = 0;

    if (size == 0) {
        base = SEQ_SMALL;
    }
    return base + state;
}

void CodeInDisp_SetCodeSizes(CodeInWork *wk) {
    int i;

    for (i = 0; i < wk->codeMax; i++) {
        if (i >= wk->largeStart && i < wk->largeEnd) {
            wk->code[i].size = 1;
        } else {
            wk->code[i].size = 0;
        }
    }
}

void CodeInDisp_SetCodePositions(CodeInWork *wk, BOOL move) {
    int i;
    s16 x = wk->focusX[wk->focus];
    int bar = 0;
    ClActorPos pos;
    ClActorPos barPos;

    for (i = 0; i < wk->codeMax; i++) {
        if (i >= wk->largeStart && i < wk->largeEnd) {
            if (i == wk->largeStart) {
                x += 20;
            } else {
                x += 32;
            }
        } else {
            if (i == 0) {
                x += 20;
            } else {
                x += 8;
            }
        }
        func_0204c178(wk->code[i].actor, &pos, 0);
        if (!move) {
            pos.x = x;
            func_0204c140(wk->code[i].actor, &pos, 0);
        } else {
            wk->code[i].move.x = (x - pos.x) / 4;
            wk->code[i].move.y = 0;
            wk->code[i].move.wait = 4;
            wk->code[i].move.scale = 0;
        }
        if (i == wk->bar[bar].state && bar != CODEIN_BAR_COUNT) {
            func_0204c178(wk->bar[bar].actor, &barPos, 0);
            if (wk->largeStart == wk->largeEnd) {
                x += 8;
            } else if (i > wk->largeStart && i < wk->largeEnd) {
                x += 20;
            } else {
                x += 8;
            }
            if (!move) {
                barPos.x = x;
                func_0204c140(wk->bar[bar].actor, &barPos, 0);
            } else {
                wk->bar[bar].move.x = (x - barPos.x) / 4;
                wk->bar[bar].move.y = 0;
                wk->bar[bar].move.wait = 4;
            }
            bar++;
        }
    }
}

// Puts the digits' touch rectangles around their actors
void CodeInDisp_UpdateCodeRects(CodeInWork *wk) {
    int i;
    int halfWidth;
    int halfHeight;
    ClActorPos pos;

    for (i = 0; i < wk->codeMax; i++) {
        if (i >= wk->largeStart && i < wk->largeEnd) {
            halfWidth = 16;
            halfHeight = 16;
        } else {
            halfWidth = 4;
            halfHeight = 8;
        }
        func_0204c178(wk->code[i].actor, &pos, 0);
        wk->code[i].rect->top = pos.y - halfHeight;
        wk->code[i].rect->left = pos.x - halfWidth;
        wk->code[i].rect->bottom = pos.y + halfHeight;
        wk->code[i].rect->right = pos.x + halfWidth;
    }
}

void CodeInDisp_InitBmpOam(CodeInWork *wk) {
    wk->bmpOam = BmpOam_Init(HEAPID_CODEIN, wk->unit);
}

static void CodeInDisp_FreeBmpOam(CodeInWork *wk) {
    func_0204bcd0(wk->btnTextPltt);
    BmpOam_ActorDel(wk->btnText[0]);
    BmpOam_ActorDel(wk->btnText[1]);
    BmpOam_Exit(wk->bmpOam);
}

static void CodeInDisp_LoadButtonTextPltt(CodeInWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, HEAPID_CODEIN);

    wk->btnTextPltt = func_0204bc48(arc, 5, CLACT_VRAM_MAIN, 0x80, HEAPID_CODEIN);
    GFL_ArcToolFree(arc);
}

void CodeInDisp_AddButtonTexts(CodeInWork *wk) {
    CodeInDisp_LoadButtonTextPltt(wk);
    CodeInDisp_AddButtonText(wk, 0, 78, 165, 0);
    CodeInDisp_AddButtonText(wk, 1, 172, 165, 0);
}

// Draws the name of Back or OK and shows it on its button, in the button's palette: the palette that
// CodeInDisp_LoadButtonTextPltt loaded is not used, and neither is the position given
static void CodeInDisp_AddButtonText(CodeInWork *wk, int button, int x, int y, int unused) {
    BmpOamActorSetup setup;
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_CODEIN_DISP, HEAPID_CODEIN);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, CodeinDisp_Text_Back + button);
    GFLBitmap *bitmap = GFL_BitmapCreate(10, 2, 0x20, HEAPID_CODEIN);
    int width = GFL_FontGetBlockWidth(str, wk->font, 0);
    s16 textX;
    s16 textY;

    GFL_TextRendererDrawToBitmapEx(bitmap, (80 - width) / 2, 0, str, wk->font, BUTTON_TEXT_COLOR);
    textX = wk->rects[CODEIN_RECT_BACK + button].left + 20;
    textY = (wk->rects[CODEIN_RECT_BACK + button].top + wk->rects[CODEIN_RECT_BACK + button].bottom) / 2;
    setup.bitmap = bitmap;
    setup.x = textX;
    setup.y = textY - 7;
    setup.palette = wk->plttRes[CODEIN_RES_BUTTON];
    setup.paletteOffset = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    setup.surface = 0;
    setup.vramType = CLACT_VRAM_MAIN;
    wk->btnText[button] = BmpOam_ActorAdd(wk->bmpOam, &setup);
    BmpOam_ActorBmpTrans(wk->btnText[button]);
    GFL_BitmapFree(bitmap);
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
}

void CodeInDisp_CreateMessageWindow(BmpWin **window, u32 messageId, int bg, int x, int y, int width, int height,
                                    Font *font, int number) {
    *window = BmpWin_CreateDynamic(bg, x, y, width, height, MESSAGE_PALETTE, TRUE);
    BmpWin_DrawFrame(*window, 2, FRAME_CHAR, FRAME_PALETTE);
    BmpWin_FlushMap(*window);
    CodeInDisp_DrawMessage(*window, messageId, font, number);
}

// Draws a message of the screen's text bank, with a number for its "{0}"
void CodeInDisp_DrawMessage(BmpWin *window, u32 messageId, Font *font, int number) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(HEAPID_CODEIN);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_CODEIN_DISP, HEAPID_CODEIN);
    StrBuf *format = GFL_MsgDataLoadStrbufNew(msgData, messageId);
    StrBuf *str = GFL_StrBufCreate(88, HEAPID_CODEIN);

    WordSetNumber(wordSet, 0, number, 2, NUM_PAD_ZERO, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, str, font);
    GFL_StrBufFree(str);
    GFL_StrBufFree(format);
    GFL_MsgDataFree(msgData);
    GFL_WordSetSystemFree(wordSet);
    BmpWin_FlushChar(window);
    GFL_BGSysLoadScr(MESSAGE_BG);
}

// Says that there is no such train or rank
void CodeInDisp_DrawErrorMessage(BmpWin *window, u32 mode, Font *font, int number) {
    if (mode == CODEIN_MODE_TRAIN) {
        CodeInDisp_DrawMessage(window, CodeinDisp_Text_ThereNoSuchTrain, font, number);
    } else if (mode == CODEIN_MODE_RANK) {
        CodeInDisp_DrawMessage(window, CodeinDisp_Text_ThereNoSuchRank, font, number);
    }
}
