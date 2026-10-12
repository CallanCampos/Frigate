// The graphics of the Battle Recorder's code input: the actors of the digits, cursors and buttons on the lower
// screen, from resources that the screen has loaded with br_res.c. The ROM has no name for this file;
// "br_codein_disp.c" is a guess after overlay 205's codein_disp.c, which it is a copy of. Our names

#include "types.h"
#include "app/battle_recorder/br_codein.h"
#include "app/battle_recorder/br_codein_pv.h"
#include "app/battle_recorder/br_res.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "system/bmp_oam.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "text/system/0008.h"

// The digits' animations: the ten large digits after an empty place, then the same small. The bar follows
#define SEQ_SMALL 11
#define SEQ_BAR 22
// The animation of the cursor that flashes on a pressed key
#define SEQ_CURSOR_PRESS 3

#define CODE_X 76
#define CODE_Y 24
#define CODE_PRIORITY 10

static void BrCodeInDisp_FreeBmpOam(BrCodeInWork *wk);
static void BrCodeInDisp_LoadButtonTextPltt(BrCodeInWork *wk);
static void BrCodeInDisp_AddButtonText(BrCodeInWork *wk, int button, int x, int y, int unused);

// The names of Back and OK
static const u32 sBrCodeInButtonMessages[BR_CODEIN_BUTTON_COUNT] = { Bank0008_Text_Quit, Bank0008_Text_Ok };

// Takes the resources of the digits, the cursors and the buttons from the ones the screen loaded
void BrCodeInDisp_GetResources(BrCodeInWork *wk) {
    BrResObjData data;
    BOOL ret;

    ret = BrRes_GetObjData(wk->param.res, 11, &data);
    GFL_ASSERT(ret);
    wk->plttRes[BR_CODEIN_RES_CODE] = data.plt;
    wk->charRes[BR_CODEIN_RES_CODE] = data.chr;
    wk->cellAnimRes[BR_CODEIN_RES_CODE] = data.cell;
    ret = BrRes_GetObjData(wk->param.res, 12, &data);
    GFL_ASSERT(ret);
    wk->plttRes[BR_CODEIN_RES_CURSOR] = data.plt;
    wk->charRes[BR_CODEIN_RES_CURSOR] = data.chr;
    wk->cellAnimRes[BR_CODEIN_RES_CURSOR] = data.cell;
    ret = BrRes_GetObjData(wk->param.res, 6, &data);
    GFL_ASSERT(ret);
    wk->plttRes[BR_CODEIN_RES_BUTTON] = data.plt;
    wk->charRes[BR_CODEIN_RES_BUTTON] = data.chr;
    wk->cellAnimRes[BR_CODEIN_RES_BUTTON] = data.cell;
}

void BrCodeInDisp_Free(BrCodeInWork *wk) {
    int i;

    for (i = 0; i < wk->codeMax; i++) {
        func_0204c108(wk->code[i].actor);
    }
    for (i = 0; i < BR_CODEIN_BAR_COUNT; i++) {
        func_0204c108(wk->bar[i].actor);
        func_0204c108(wk->btn[i].actor);
    }
    for (i = 0; i < BR_CODEIN_CURSOR_COUNT; i++) {
        func_0204c108(wk->cur[i].actor);
    }
    BrCodeInDisp_FreeBmpOam(wk);
}

// Adds the digits' actors, with a bar after each of the first two groups
void BrCodeInDisp_AddCodeActors(BrCodeInWork *wk) {
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
    for (i = 0; i < wk->codeMax + BR_CODEIN_BAR_COUNT; i++) {
        if (i == wk->bar[bar].state + bar + 1) {
            setup.x = CODE_X + i * 8;
            setup.y = CODE_Y;
            setup.sequence = SEQ_BAR;
            wk->bar[bar].actor =
                func_0204c040(unit, wk->charRes[BR_CODEIN_RES_CODE], wk->plttRes[BR_CODEIN_RES_CODE],
                              wk->cellAnimRes[BR_CODEIN_RES_CODE], &setup, CLACT_SURFACE_SUB, wk->heapId);
            func_0204c318(wk->bar[bar].actor, GX_OAM_MODE_XLU);
            bar++;
        } else {
            setup.x = CODE_X + i * 8;
            setup.y = CODE_Y;
            setup.sequence = BrCodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size);
            wk->code[digit].actor =
                func_0204c040(unit, wk->charRes[BR_CODEIN_RES_CODE], wk->plttRes[BR_CODEIN_RES_CODE],
                              wk->cellAnimRes[BR_CODEIN_RES_CODE], &setup, CLACT_SURFACE_SUB, wk->heapId);
            func_0204c318(wk->code[digit].actor, GX_OAM_MODE_XLU);
            digit++;
        }
    }
}

void BrCodeInDisp_AddCursorActors(BrCodeInWork *wk) {
    ClActUnit *unit = wk->unit;
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    wk->cur[BR_CODEIN_CURSOR_CODE].actor =
        func_0204c040(unit, wk->charRes[BR_CODEIN_RES_CURSOR], wk->plttRes[BR_CODEIN_RES_CURSOR],
                      wk->cellAnimRes[BR_CODEIN_RES_CURSOR], &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c318(wk->cur[BR_CODEIN_CURSOR_CODE].actor, GX_OAM_MODE_XLU);
    wk->cur[BR_CODEIN_CURSOR_KEY].actor =
        func_0204c040(unit, wk->charRes[BR_CODEIN_RES_CURSOR], wk->plttRes[BR_CODEIN_RES_CURSOR],
                      wk->cellAnimRes[BR_CODEIN_RES_CURSOR], &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c318(wk->cur[BR_CODEIN_CURSOR_KEY].actor, GX_OAM_MODE_XLU);
    wk->cur[BR_CODEIN_CURSOR_PRESS].actor =
        func_0204c040(unit, wk->charRes[BR_CODEIN_RES_CURSOR], wk->plttRes[BR_CODEIN_RES_CURSOR],
                      wk->cellAnimRes[BR_CODEIN_RES_CURSOR], &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c318(wk->cur[BR_CODEIN_CURSOR_PRESS].actor, GX_OAM_MODE_XLU);
    BrCodeInDisp_SetCodeCursor(wk, 0);
    func_0204c488(wk->cur[BR_CODEIN_CURSOR_CODE].actor, 0);
    func_0204c520(wk->cur[BR_CODEIN_CURSOR_CODE].actor, TRUE);
    wk->cur[BR_CODEIN_CURSOR_KEY].move.x = 0;
    wk->cur[BR_CODEIN_CURSOR_KEY].move.y = 0;
    wk->cur[BR_CODEIN_CURSOR_KEY].state = 1;
    BrCodeInDisp_SetKeyCursor(wk, 0);
    func_0204c488(wk->cur[BR_CODEIN_CURSOR_KEY].actor, wk->cur[BR_CODEIN_CURSOR_KEY].state);
    func_0204c520(wk->cur[BR_CODEIN_CURSOR_KEY].actor, TRUE);
    func_0204c318(wk->cur[BR_CODEIN_CURSOR_KEY].actor, GX_OAM_MODE_XLU);
    wk->cur[BR_CODEIN_CURSOR_PRESS].move.x = 0;
    wk->cur[BR_CODEIN_CURSOR_PRESS].move.y = 0;
    wk->cur[BR_CODEIN_CURSOR_PRESS].state = 1;
    BrCodeInDisp_SetKeyCursor(wk, 0);
    func_0204c488(wk->cur[BR_CODEIN_CURSOR_PRESS].actor, wk->cur[BR_CODEIN_CURSOR_PRESS].state);
    func_0204c318(wk->cur[BR_CODEIN_CURSOR_PRESS].actor, GX_OAM_MODE_XLU);
    func_0204c520(wk->cur[BR_CODEIN_CURSOR_PRESS].actor, TRUE);
    BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_KEY, FALSE);
    BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_PRESS, FALSE);
}

void BrCodeInDisp_AddButtonActors(BrCodeInWork *wk) {
    ClActUnit *unit = wk->unit;
    ClActorSetup setup;
    ClActorPos pos;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = CODE_PRIORITY;
    setup.bgPriority = 0;
    wk->btn[0].actor = func_0204c040(unit, wk->charRes[BR_CODEIN_RES_BUTTON], wk->plttRes[BR_CODEIN_RES_BUTTON],
                                     wk->cellAnimRes[BR_CODEIN_RES_BUTTON], &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c318(wk->btn[0].actor, GX_OAM_MODE_XLU);
    setup.sequence = 1;
    wk->btn[1].actor = func_0204c040(unit, wk->charRes[BR_CODEIN_RES_BUTTON], wk->plttRes[BR_CODEIN_RES_BUTTON],
                                     wk->cellAnimRes[BR_CODEIN_RES_BUTTON], &setup, CLACT_SURFACE_SUB, wk->heapId);
    func_0204c318(wk->btn[1].actor, GX_OAM_MODE_XLU);
    pos.x = wk->rects[BR_CODEIN_RECT_BACK].left;
    pos.y = wk->rects[BR_CODEIN_RECT_BACK].top + 16;
    func_0204c140(wk->btn[0].actor, &pos, CLACT_SURFACE_SUB);
    func_0204c488(wk->btn[0].actor, 0);
    pos.x = wk->rects[BR_CODEIN_RECT_OK].left;
    pos.y = wk->rects[BR_CODEIN_RECT_OK].top + 16;
    func_0204c140(wk->btn[1].actor, &pos, CLACT_SURFACE_SUB);
    func_0204c488(wk->btn[1].actor, 1);
}

void BrCodeInDisp_SetCursorVisible(BrCodeInWork *wk, int cursor, BOOL visible) {
    func_0204c124(wk->cur[cursor].actor, visible);
}

// Puts the code's cursor under a digit
void BrCodeInDisp_SetCodeCursor(BrCodeInWork *wk, int digit) {
    ClActorPos pos;
    ClActor *actor = wk->code[digit].actor;

    wk->cur[BR_CODEIN_CURSOR_CODE].state = digit;
    func_0204c178(actor, &pos, CLACT_SURFACE_SUB);
    pos.y += 16;
    func_0204c140(wk->cur[BR_CODEIN_CURSOR_CODE].actor, &pos, CLACT_SURFACE_SUB);
}

// Puts the keys' cursor on a key and restarts its animation
void BrCodeInDisp_SetKeyCursor(BrCodeInWork *wk, int key) {
    ClActorPos pos;
    int height = wk->rects[BR_CODEIN_RECT_KEYS + key].bottom - wk->rects[BR_CODEIN_RECT_KEYS + key].top;
    int width = wk->rects[BR_CODEIN_RECT_KEYS + key].right - wk->rects[BR_CODEIN_RECT_KEYS + key].left;

    pos.x = width / 2 + wk->rects[BR_CODEIN_RECT_KEYS + key].left;
    pos.y = height / 2 + wk->rects[BR_CODEIN_RECT_KEYS + key].top;
    func_0204c140(wk->cur[BR_CODEIN_CURSOR_KEY].actor, &pos, CLACT_SURFACE_SUB);
    func_0204c4d4(wk->cur[BR_CODEIN_CURSOR_KEY].actor, 0);
}

// Gives the keys' cursors their animations, and hides the keys' cursor while a press flashes or the touch screen is
// in use
void BrCodeInDisp_UpdateCursors(BrCodeInWork *wk) {
    int i;

    for (i = BR_CODEIN_CURSOR_KEY; i < BR_CODEIN_CURSOR_COUNT; i++) {
        int sequence = func_0204c4a0(wk->cur[i].actor);

        if (sequence == SEQ_CURSOR_PRESS) {
            if (!func_0204c560(wk->cur[i].actor)) {
                func_0204c488(wk->cur[i].actor, wk->cur[i].state);
                if (wk->touchMode == TRUE) {
                    BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_KEY, FALSE);
                } else {
                    BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_KEY, TRUE);
                }
                BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_PRESS, FALSE);
            }
        } else {
            if (sequence != wk->cur[i].state) {
                func_0204c488(wk->cur[i].actor, wk->cur[i].state);
            }
            if (func_0204c4a0(wk->cur[i].actor) != SEQ_CURSOR_PRESS) {
                if (wk->touchMode == TRUE) {
                    BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_KEY, FALSE);
                } else {
                    BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_KEY, TRUE);
                }
            }
        }
    }
}

int BrCodeInDisp_GetCodeSequence(int state, int size) {
    int base = 0;

    if (size == 0) {
        base = SEQ_SMALL;
    }
    return base + state;
}

void BrCodeInDisp_SetCodeSizes(BrCodeInWork *wk) {
    int i;

    for (i = 0; i < wk->codeMax; i++) {
        if (i >= wk->largeStart && i < wk->largeEnd) {
            wk->code[i].size = 1;
        } else {
            wk->code[i].size = 0;
        }
    }
}

void BrCodeInDisp_SetCodePositions(BrCodeInWork *wk, BOOL move) {
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
        func_0204c178(wk->code[i].actor, &pos, CLACT_SURFACE_SUB);
        if (!move) {
            pos.x = x;
            func_0204c140(wk->code[i].actor, &pos, CLACT_SURFACE_SUB);
        } else {
            wk->code[i].move.x = (x - pos.x) / 4;
            wk->code[i].move.y = 0;
            wk->code[i].move.wait = 4;
            wk->code[i].move.scale = 0;
        }
        if (i == wk->bar[bar].state && bar != BR_CODEIN_BAR_COUNT) {
            func_0204c178(wk->bar[bar].actor, &barPos, CLACT_SURFACE_SUB);
            if (wk->largeStart == wk->largeEnd) {
                x += 8;
            } else if (i > wk->largeStart && i < wk->largeEnd) {
                x += 20;
            } else {
                x += 8;
            }
            if (!move) {
                barPos.x = x;
                func_0204c140(wk->bar[bar].actor, &barPos, CLACT_SURFACE_SUB);
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
void BrCodeInDisp_UpdateCodeRects(BrCodeInWork *wk) {
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
        func_0204c178(wk->code[i].actor, &pos, CLACT_SURFACE_SUB);
        wk->code[i].rect->top = pos.y - halfHeight;
        wk->code[i].rect->left = pos.x - halfWidth;
        wk->code[i].rect->bottom = pos.y + halfHeight;
        wk->code[i].rect->right = pos.x + halfWidth;
    }
}

void BrCodeInDisp_InitBmpOam(BrCodeInWork *wk) {
    wk->bmpOam = BmpOam_Init(wk->heapId, wk->unit);
}

static void BrCodeInDisp_FreeBmpOam(BrCodeInWork *wk) {
    BmpOam_ActorDel(wk->btnText[0]);
    BmpOam_ActorDel(wk->btnText[1]);
    BmpOam_Exit(wk->bmpOam);
}

// What is left of codein_disp.c's function, which loads the font's palette for the buttons' names: the names use the
// buttons' palette
static void BrCodeInDisp_LoadButtonTextPltt(BrCodeInWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);

    GFL_ArcToolFree(arc);
}

void BrCodeInDisp_AddButtonTexts(BrCodeInWork *wk) {
    BrCodeInDisp_LoadButtonTextPltt(wk);
    BrCodeInDisp_AddButtonText(wk, 0, 32, 8, 0);
    BrCodeInDisp_AddButtonText(wk, 1, 32, 8, 0);
}

// Draws the name of Back or OK and shows it on its button, x and y from the button's corner
static void BrCodeInDisp_AddButtonText(BrCodeInWork *wk, int button, int x, int y, int unused) {
    BmpOamActorSetup setup;
    MsgData *msgData = BrRes_GetMsgData(wk->param.res);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, sBrCodeInButtonMessages[button]);
    GFLBitmap *bitmap = GFL_BitmapCreate(10, 2, 0x20, wk->heapId);
    int left;
    int top;

    GFL_TextRendererDrawToBitmap(bitmap, 1, 0, str, wk->font);
    left = wk->rects[BR_CODEIN_RECT_BACK + button].left;
    top = wk->rects[BR_CODEIN_RECT_BACK + button].top;
    setup.bitmap = bitmap;
    setup.x = left + x;
    setup.y = top + y;
    setup.palette = wk->plttRes[BR_CODEIN_RES_BUTTON];
    setup.paletteOffset = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    setup.surface = CLACT_SURFACE_SUB;
    setup.vramType = CLACT_VRAM_SUB;
    wk->btnText[button] = BmpOam_ActorAdd(wk->bmpOam, &setup);
    BmpOam_ActorBmpTrans(wk->btnText[button]);
    BmpOam_ActorSetObjMode(wk->btnText[button], GX_OAM_MODE_XLU);
    GFL_BitmapFree(bitmap);
    GFL_StrBufFree(str);
}
