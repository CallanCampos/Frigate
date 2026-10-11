#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/union_comm.h"
#include "field/union_main.h"
#include "field/union_oneself.h"
#include "field/union_subdisp.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/wm_icon.h"
#include "nitro/gx.h"
#include "system/game_comm.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "text/system/0400.h"

// The Union Room's touch screen, named after the ROM's "union_subdisp.c". Five icons at the bottom pick what the
// player wants to do, and the three lines above show the Union Room's chat of what the players around say, newest
// at the bottom, which a bar at the right scrolls. The names are ours

// The sub engine's BGs: the frame, the chat lines and the lines' background
#define BG_FRAME 5
#define BG_LINES 6
#define BG_TEXT 7

#define LINE_COUNT 3
#define ICON_COUNT 5
// The scroll bar's knob is the sixth actor
#define ACTOR_KNOB ICON_COUNT
#define ACTOR_COUNT 6

// What the lines show: a sentence drawn by PMSDraw, or a message printed in the window
#define CONTENT_SENTENCE 0
#define CONTENT_MESSAGE 1

// A line is 7 tiles high, 56 pixels
#define LINE_TILES 7
#define LINE_HEIGHT 56
// The words of a line, which the line's background is a row of the screen of BG 6 for
#define SCREEN_ROW_SIZE 0x1c0
#define SCREEN_ROW_WORDS 0xe0

// The scroll bar's track: where the knob moves, and its width
#define KNOB_X 0xf4
#define TRACK_TOP 0x1c
#define TRACK_BOTTOM 0x9c
#define TRACK_LEFT 0xe9

// How many frames a touched line stays lit
#define FLASH_FRAMES 90

// The lines that the chat keeps, and how far the knob can go
#define CHAT_VISIBLE_MAX 0x1d

#define PRINT_COLOR_LINE 0x440

// A tile of a screen
typedef struct {
    u16 tile : 10;
    u16 flipX : 1;
    u16 flipY : 1;
    u16 palette : 4;
} UnionSubdispScreenEntry;

// A line that was touched, lit until the timer runs out
typedef struct {
    u16 timer;
    u8 line;
} UnionSubdispFlash;

// What a line prints
typedef struct {
    BmpWin *window;
    u8 printing;
} UnionSubdispPrint;

struct UnionSubdisp {
    GameSystem *gsys;
    u32 frameChars;
    u32 iconChars;
    u32 iconPalette;
    u32 iconCellAnims;
    ClActUnit *unit;
    ClActor *actors[ACTOR_COUNT];
    // The icon that is selected, and whether the knob is held
    u8 selected;
    u8 holdingKnob;
    u16 unk32;
    UnionSubdispFlash flash[LINE_COUNT];
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    BmpWin *windows[LINE_COUNT];
    u32 content[LINE_COUNT];
    UnionSubdispPrint print[LINE_COUNT];
    PMSDraw *pmsDraw;
};

static void UnionSubdisp_InitUnit(UnionSubdisp *work);
static void UnionSubdisp_FreeUnit(UnionSubdisp *work);
static void UnionSubdisp_InitBG(UnionSubdisp *work, ArcTool *arc);
static void UnionSubdisp_ExitBG(UnionSubdisp *work);
static void UnionSubdisp_InitFrame(UnionSubdisp *work);
static void UnionSubdisp_FreeFrame(UnionSubdisp *work);
static void UnionSubdisp_InitWindows(UnionSubdisp *work);
static void UnionSubdisp_FreeWindows(UnionSubdisp *work);
static void UnionSubdisp_InitIconResources(UnionSubdisp *work, ArcTool *arc);
static void UnionSubdisp_FreeIconResources(UnionSubdisp *work);
static void UnionSubdisp_InitIcons(UnionSubdisp *work);
static void UnionSubdisp_FreeIcons(UnionSubdisp *work);
static BOOL UnionSubdisp_UpdateIcons(UnionSubdisp *work, UnionSystem *sys);
static void UnionSubdisp_SelectIcon(UnionSubdisp *work, u32 icon);
static void UnionSubdisp_UpdateChat(UnionSubdisp *work, UnionVisitorList *list);
static void UnionSubdisp_DrawLine(UnionSubdisp *work, UnionVisitor *visitor, u8 line);
static void UnionSubdisp_DrawAllLines(UnionSubdisp *work, UnionVisitorList *list);
static void UnionSubdisp_MoveLine(UnionSubdisp *work, u8 from, u8 to, UnionVisitorList *list, int index);
static void UnionSubdisp_ScrollLines(UnionSubdisp *work, UnionVisitorList *list, int delta);
static BOOL UnionSubdisp_UpdateKnobTouch(UnionSubdisp *work);
static void UnionSubdisp_UpdateKnob(UnionSystem *sys, UnionSubdisp *work);
static void UnionSubdisp_DragKnob(UnionSubdisp *work, UnionVisitorList *list);
static void UnionSubdisp_PlaceKnob(UnionSubdisp *work, UnionVisitorList *list);
static int UnionSubdisp_GetScrollRange(UnionVisitorList *list);
static u32 UnionSubdisp_GetScrollStep(UnionVisitorList *list, u32 range);
static BOOL UnionSubdisp_UpdateTrackTouch(UnionSubdisp *work);
static BOOL UnionSubdisp_UpdateLineTouch(UnionSystem *sys, UnionSubdisp *work, UnionVisitorList *list);
static UnionSubdispFlash *UnionSubdisp_FindFlash(UnionSubdisp *work, UnionVisitorList *list, int line);
static BOOL UnionSubdisp_StartFlash(UnionSubdisp *work, UnionVisitorList *list, int line);
static void UnionSubdisp_UpdateFlash(UnionSubdisp *work, UnionVisitorList *list);
static void UnionSubdisp_CancelOtherFlashes(UnionSubdisp *work, UnionVisitorList *list, int line);
static BOOL UnionSubdisp_SetLinePalette(UnionSubdisp *work, int line, BOOL touched);
static void UnionSubdisp_UpdateKeyScroll(UnionSubdisp *work, UnionSystem *sys);

// The state that each icon picks, where 0 leaves the icon unselected
static const u8 sIconStates[ICON_COUNT] = { 0, 1, 2, 4, 5 };

// Where a line prints its message, and the first icon, which the others are made from
static const PMSDrawPos sMessagePos = { 0, 20 };

static const ClActorSetup sIconSetup = { 0, 180, 0, 10, 0 };

static const BGSetup sFrameBGSetup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x00000), 0x6800,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sLinesBGSetup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x00000), 0x6800,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sTextBGSetup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x00000), 0x6800,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};

UnionSubdisp *UnionSubdisp_Create(GameSystem *gsys) {
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);
    UnionSystem *sys;
    ArcTool *arc;
    UnionSubdisp *work = GFL_HeapAllocate(HEAPID_FIELDMAP, sizeof(UnionSubdisp), TRUE, "union_subdisp.c", 271);
    u32 i;

    work->gsys = gsys;
    arc = GFL_ArcSysCreateFileHandle(ARCID_UNION_SUBDISP, HEAPID_FIELDMAP);
    UnionSubdisp_InitUnit(work);
    UnionSubdisp_InitBG(work, arc);
    UnionSubdisp_InitWindows(work);
    UnionSubdisp_InitIconResources(work, arc);
    UnionSubdisp_InitIcons(work);
    UnionSubdisp_InitFrame(work);
    GFL_ArcToolFree(arc);
    work->pmsDraw = PMSDraw_Create(work->unit, CLACT_VRAM_SUB, work->queue, work->font, 9, LINE_COUNT, HEAPID_FIELDMAP);
    PMSDraw_SetBackColor(work->pmsDraw, 0);
    sys = GameCommSys_GetWork(comm);
    i = 0;
    if (sys != NULL) {
        work->selected = func_ov028_021710c8(sys);
        UnionSubdisp_DrawAllLines(work, &sys->visitorList);
        if (work->selected != 0) {
            for (; i < ICON_COUNT; i++) {
                if (sIconStates[i] == work->selected) {
                    func_0204c378(work->actors[i], 1, 1);
                    break;
                }
            }
        }
    }
    func_02042ba8(FALSE, HEAPID_FIELDMAP);
    func_0203e84c();
    G2S_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3
                                 | GX_WND_PLANEMASK_OBJ,
                             FALSE);
    G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3
                               | GX_WND_PLANEMASK_OBJ,
                           TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_OW);
    return work;
}

void UnionSubdisp_Free(UnionSubdisp *work) {
    UnionSystem *sys = GameCommSys_GetWork(GSYS_GetGameCommSystem(work->gsys));

    if (sys != NULL) {
        sys->unk282E = 0;
    }
    PMSDraw_Delete(work->pmsDraw);
    UnionSubdisp_FreeFrame(work);
    UnionSubdisp_FreeIcons(work);
    UnionSubdisp_FreeIconResources(work);
    UnionSubdisp_FreeWindows(work);
    UnionSubdisp_ExitBG(work);
    UnionSubdisp_FreeUnit(work);
    GFL_HeapFree(work);
    G2S_BlendNone();
    func_0203e860();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

void UnionSubdisp_Update(UnionSubdisp *work, BOOL active) {
    int i;

    GameCommSys_GetWork(GSYS_GetGameCommSystem(work->gsys));
    func_0203e84c();
    if (active == TRUE) {
        G2S_BlendNone();
    } else {
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0x3f, -8);
    }
    func_02021a3c(work->queue);
    PMSDraw_Main(work->pmsDraw);
    for (i = 0; i < LINE_COUNT; i++) {
        if (work->content[i] == CONTENT_MESSAGE) {
            PrintQueue *queue = work->queue;
            if (work->print[i].printing != 0) {
                if (func_02021c1c(queue, BmpWin_GetBitmap(work->print[i].window)) == 0) {
                    BmpWin_FlushChar(work->print[i].window);
                    work->print[i].printing = 0;
                }
            }
        }
    }
}

void UnionSubdisp_UpdateAltFrame(UnionSubdisp *work, BOOL active) {
    GameCommSys *comm = GSYS_GetGameCommSystem(work->gsys);
    UnionSystem *sys = GameCommSys_GetWork(comm);

    if (sys != NULL) {
        UnionSelf *self = &sys->situ;

        if (self->changed == 1) {
            func_ov036_021b445c(sys, NULL, self->look, self->state);
            self->changed = 0;
        }
        UnionSubdisp_UpdateChat(work, &sys->visitorList);
        if (sys->visitorList.count >= 3) {
            func_0204c124(work->actors[ACTOR_KNOB], TRUE);
        }
    }
    if (active == 1) {
        BOOL touched = UnionSubdisp_UpdateIcons(work, sys);

        if (sys != NULL && touched == 1) {
            switch (work->selected) {
            case 1:
                UnionComm_SetState(sys, 2);
                break;
            case 2:
                UnionComm_SetState(sys, 3);
                break;
            case 4:
                UnionComm_SetState(sys, 4);
                break;
            case 5:
                UnionComm_SetState(sys, 5);
                break;
            }
        }
        work->holdingKnob = UnionSubdisp_UpdateKnobTouch(work);
        if (work->holdingKnob == 0) {
            work->holdingKnob = UnionSubdisp_UpdateTrackTouch(work);
        }
        if (work->holdingKnob == 0 && sys != NULL) {
            UnionSubdisp_UpdateKeyScroll(work, sys);
        }
    }
    if (sys != NULL) {
        UnionSubdisp_UpdateKnob(sys, work);
        if (active == 1) {
            UnionSubdisp_UpdateLineTouch(sys, work, &sys->visitorList);
        }
        UnionSubdisp_UpdateFlash(work, &sys->visitorList);
    }
    if (GameCommSys_BootCheck(comm) == 3 && sys != NULL) {
        func_ov028_021710bc(sys, work->selected);
    }
}

static void UnionSubdisp_InitUnit(UnionSubdisp *work) {
    work->unit = func_0204bf1c(12, 5, HEAPID_FIELDMAP);
    work->queue = func_02021998(HEAPID_FIELDMAP);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_FIELDMAP);
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0400, HEAPID_FIELDMAP);
}

static void UnionSubdisp_FreeUnit(UnionSubdisp *work) {
    GFL_MsgDataFree(work->msgData);
    GFL_FontFree(work->font);
    func_02021c44(work->queue);
    func_02021a18(work->queue);
    func_0204bf98(work->unit);
}

static void UnionSubdisp_InitBG(UnionSubdisp *work, ArcTool *arc) {
    NNSG2dScreenData *screen;
    void *screenBuffer;
    void *lines;
    int i = 0;

    GFL_BGSysCreateBG(BG_FRAME, &sFrameBGSetup, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_LINES, &sLinesBGSetup, BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_TEXT, &sTextBGSetup, BGMODE_TEXT);
    sys_memset32(0, (void *)HW_DB_BG_VRAM, 0x20000);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    G2S_BlendNone();
    GFL_BGSysLoadArcNCGRStatic(arc, 0xf, BG_TEXT, 0, 0, FALSE, HEAPID_FIELDMAP);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x11, BG_TEXT, 0, 0x800, FALSE, HEAPID_FIELDMAP);
    lines = GFL_BGSysIsScrHeapExists(BG_LINES);
    screenBuffer = GFL_G2DIOReadNSCRArc(arc, 0x12, FALSE, &screen, HEAPID_FIELDMAP);
    sys_memcpy(screen->rawData, lines, SCREEN_ROW_SIZE);
    GFL_HeapFree(screenBuffer);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0x10, PALTYPE_SUB_BG, 0, 0xa0, HEAPID_FIELDMAP);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 15 * 32, 0x20, HEAPID_FIELDMAP);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(BG_TEXT, TRUE);
    GFL_BGSysSetBGEnabled(BG_LINES, TRUE);
    GFL_BGSysSetBGEnabled(BG_FRAME, TRUE);
}

static void UnionSubdisp_ExitBG(UnionSubdisp *work) {
    GFL_BGSysSetBGEnabled(BG_FRAME, FALSE);
    GFL_BGSysSetBGEnabled(BG_LINES, FALSE);
    GFL_BGSysSetBGEnabled(BG_TEXT, FALSE);
    GFL_BGSysReleaseBG(BG_FRAME);
    GFL_BGSysReleaseBG(BG_LINES);
    GFL_BGSysReleaseBG(BG_TEXT);
}

// Loads the frame around the chat to BG 5
static void UnionSubdisp_InitFrame(UnionSubdisp *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_FIELDMAP);
    NNSG2dScreenData *screen;
    u16 *screenBuffer;
    void *buffer;
    u32 chars;
    int i;

    chars = GFL_BGSysAllocChar(BG_FRAME, 0x80, TRUE);
    work->frameChars = chars;
    GFL_BGSysLoadArcNCGRStatic(arc, 0x1c, BG_FRAME, chars, 0x80, FALSE, HEAPID_FIELDMAP);
    screenBuffer = GFL_BGSysIsScrHeapExists(BG_FRAME);
    buffer = GFL_G2DIOReadNSCRArc(arc, 0x1d, FALSE, &screen, HEAPID_FIELDMAP);
    sys_memcpy((u8 *)screen->rawData + 0x15 * 0x40, (u8 *)screenBuffer + 0x15 * 0x40, 0xc0);
    GFL_HeapFree(buffer);
    for (i = 0; i < 0x60; i++) {
        screenBuffer[0x2a0 + i] = (screenBuffer[0x2a0 + i] & 0xfff) + chars + 0xd000;
    }
    GFL_G2DIOLoadArcNCLRDefault(arc, 0x1b, PALTYPE_SUB_BG, 0x1a0, 0x20, HEAPID_FIELDMAP);
    GFL_ArcToolFree(arc);
}

static void UnionSubdisp_FreeFrame(UnionSubdisp *work) {
    GFL_BGSysFreeCharMemory(BG_FRAME, work->frameChars, 0x80);
}

static void UnionSubdisp_InitWindows(UnionSubdisp *work) {
    int i;

    for (i = 0; i < LINE_COUNT; i++) {
        work->windows[i] = BmpWin_CreateDynamic(BG_FRAME, 1, (i * LINE_HEIGHT) / 8, 29, LINE_TILES, 15, TRUE);
        work->print[i].window = work->windows[i];
        work->print[i].printing = 0;
    }
}

static void UnionSubdisp_FreeWindows(UnionSubdisp *work) {
    int i;

    for (i = 0; i < LINE_COUNT; i++) {
        BmpWin_Free(work->windows[i]);
    }
}

static void UnionSubdisp_InitIconResources(UnionSubdisp *work, ArcTool *arc) {
    work->iconPalette = func_0204bbb8(arc, 0x13, 1, 0, 0, 3, HEAPID_FIELDMAP);
    work->iconChars = func_0204b81c(arc, 0xe, FALSE, TRUE, HEAPID_FIELDMAP);
    work->iconCellAnims = func_0204bde0(arc, 0xc, 0xd, HEAPID_FIELDMAP);
}

static void UnionSubdisp_FreeIconResources(UnionSubdisp *work) {
    func_0204bcd0(work->iconPalette);
    func_0204b98c(work->iconChars);
    func_0204be64(work->iconCellAnims);
}

static void UnionSubdisp_InitIcons(UnionSubdisp *work) {
    ClActorSetup setup = sIconSetup;
    int i;

    for (i = 0; i <= ICON_COUNT - 1; i++) {
        setup.x = 0x30 * i + 0x20;
        setup.sequence = i;
        work->actors[i] = func_0204c040(work->unit, work->iconChars, work->iconPalette, work->iconCellAnims, &setup, 1,
                                        HEAPID_FIELDMAP);
    }
    setup.sequence = 5;
    setup.x = KNOB_X;
    setup.y = TRACK_BOTTOM;
    work->actors[ACTOR_KNOB] = func_0204c040(work->unit, work->iconChars, work->iconPalette, work->iconCellAnims, &setup, 1,
                                             HEAPID_FIELDMAP);
    func_0204c124(work->actors[ACTOR_KNOB], FALSE);
}

static void UnionSubdisp_FreeIcons(UnionSubdisp *work) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(work->actors[i]);
    }
}

// Whether an icon was touched: the first leaves the Union Room, the others pick what to do
static BOOL UnionSubdisp_UpdateIcons(UnionSubdisp *work, UnionSystem *sys) {
    u32 x;
    u32 y;
    int i;

    if (!func_0203dac8(&x, &y)) {
        return FALSE;
    }
    for (i = 0; i <= ICON_COUNT - 1; i++) {
        ClActorPos pos;
        s16 top;
        s16 bottom;
        s16 right;
        s16 left;

        func_0204c178(work->actors[i], &pos, 1);
        top = pos.y - 10;
        bottom = pos.y + 10;
        right = pos.x + 10;
        left = pos.x - 10;
        if (left < x && right > x && top < y && bottom > y) {
            if (i == 0 && sys != NULL) {
                sys->unk282E = 1;
                return FALSE;
            }
            UnionSubdisp_SelectIcon(work, i);
            GFL_SndSEPlay(SEQ_SE_SELECT4);
            return TRUE;
        }
    }
    return FALSE;
}

static void UnionSubdisp_SelectIcon(UnionSubdisp *work, u32 icon) {
    u8 state = sIconStates[icon];
    int i;

    if (state == work->selected) {
        state = 0;
    }
    work->selected = state;
    for (i = 0; i <= ICON_COUNT - 1; i++) {
        if (i == icon && work->selected != 0) {
            func_0204c378(work->actors[i], 1, 1);
        } else {
            func_0204c378(work->actors[i], 0, 1);
        }
    }
}

static void UnionSubdisp_UpdateChat(UnionSubdisp *work, UnionVisitorList *list) {
    BOOL redrawn = FALSE;
    BOOL idle;
    int i;

    if (list->drawnEnd == list->viewEnd) {
        return;
    }
    if (!PMSDraw_IsPrintEnd(work->pmsDraw)) {
        return;
    }
    for (i = 0; i < LINE_COUNT; i++) {
        if (work->content[i] == CONTENT_MESSAGE) {
            PrintQueue *queue = work->queue;

            if (work->print[i].printing != 0) {
                if (func_02021c1c(queue, BmpWin_GetBitmap(work->print[i].window)) == 0) {
                    BmpWin_FlushChar(work->print[i].window);
                    work->print[i].printing = 0;
                }
            }
            idle = work->print[i].printing == 0;
            if (!idle) {
                return;
            }
        }
    }
    if (list->count < 3) {
        int index;

        for (index = list->drawnEnd; index <= list->viewEnd; index++) {
            if (index > -1) {
                UnionVisitor *visitor = func_ov036_021b4620(list, index);

                GFL_ASSERT(visitor != NULL);
                UnionSubdisp_DrawLine(work, visitor, index);
            }
        }
    } else {
        int delta = list->viewEnd - list->drawnEnd;
        int size = delta;

        if (size < 0) {
            size = -size;
        }
        if (size < 3 && list->drawnEnd >= 2) {
            UnionSubdisp_ScrollLines(work, list, delta);
        } else {
            UnionSubdisp_DrawAllLines(work, list);
        }
        redrawn = TRUE;
    }
    if (redrawn == TRUE) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
    list->drawnEnd = list->viewEnd;
}

static void UnionSubdisp_DrawLine(UnionSubdisp *work, UnionVisitor *visitor, u8 line) {
    u16 *screen = GFL_BGSysIsScrHeapExists(BG_LINES);
    u8 palette = 0;
    PMSDrawPos pos = sMessagePos;
    u32 *content;
    StrBuf *strbuf;
    StrBuf *name;
    u32 contentType;
    BmpWin **window;

    if (visitor->gender != 0) {
        palette = 2;
    }
    sys_memcpy(screen, screen + SCREEN_ROW_WORDS * line, SCREEN_ROW_SIZE);
    GFL_BGSysSetScrPaletteNo(BG_LINES, 0, LINE_TILES * line, 32, LINE_TILES, palette);
    GFL_BGSysQueueScrLoad(BG_LINES);
    content = &work->content[line];
    if (*content == CONTENT_SENTENCE) {
        if (PMSDraw_IsDrawn(work->pmsDraw, line) == TRUE) {
            PMSDraw_Clear(work->pmsDraw, line, TRUE);
        }
    } else if (*content == CONTENT_MESSAGE) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[line]), 0);
        BmpWin_FlushChar(work->windows[line]);
    }
    window = &work->windows[line];
    if (visitor->kind == UNION_CHAT_KIND_SENTENCE) {
        PMSDraw_PrintEx(work->pmsDraw, *window, &visitor->sentence, line, &pos);
        contentType = CONTENT_SENTENCE;
    } else {
        u32 message;

        switch (visitor->kind) {
        case UNION_CHAT_KIND_ENTERED:
            message = Bank0400_Text_IveEnteredUnionRoom;
            break;
        case UNION_CHAT_KIND_BATTLE:
            message = Bank0400_Text_TouchBattleIconIf;
            break;
        case UNION_CHAT_KIND_TRADE:
            message = Bank0400_Text_TouchTradeIconIf;
            break;
        case UNION_CHAT_KIND_DRAW:
            message = Bank0400_Text_TouchDrawIconIf;
            break;
        case UNION_CHAT_KIND_SPIN_TRADE:
            message = Bank0400_Text_TouchSpinTradeIcon;
            break;
        default:
            return;
        }
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, message);
        BmpWin_FlushMap(*window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(*window));
        func_02021c7c(work->queue, BmpWin_GetBitmap(work->print[line].window), 0, pos.y, strbuf, work->font,
                      PRINT_COLOR_LINE);
        work->print[line].printing = TRUE;
        GFL_StrBufFree(strbuf);
        contentType = CONTENT_MESSAGE;
    }
    *content = contentType;
    name = GFL_StrBufCreate(8, HEAPID_FIELDMAP);
    GFL_StrBufLoadString(name, visitor->name);
    func_02021c7c(work->queue, BmpWin_GetBitmap(*window), 0, 0, name, work->font, PRINT_COLOR_LINE);
    GFL_StrBufFree(name);
    if (visitor->subKind != 0) {
        u32 message = Bank0400_Text_Friend;

        if (visitor->subKind != 1) {
            message = Bank0400_Text_Player;
        }
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, message);
        func_02021c7c(work->queue, BmpWin_GetBitmap(*window), 0x74, 0, strbuf, work->font, PRINT_COLOR_LINE);
        GFL_StrBufFree(strbuf);
    }
}

// Draws the three newest lines, the newest at the bottom
static void UnionSubdisp_DrawAllLines(UnionSubdisp *work, UnionVisitorList *list) {
    int line;
    int newest = list->viewEnd;
    int oldest = newest - 3;

    if (oldest < 0) {
        oldest = -1;
    }
    line = newest - oldest - 1;
    for (; newest > oldest; newest--, line--) {
        UnionVisitor *visitor = func_ov036_021b4620(list, newest);

        GFL_ASSERT(visitor != NULL);
        GFL_ASSERT(line > -1);
        UnionSubdisp_DrawLine(work, visitor, line);
    }
}

// Moves a line to another place of the screen, with what it shows
static void UnionSubdisp_MoveLine(UnionSubdisp *work, u8 from, u8 to, UnionVisitorList *list, int index) {
    u16 *screen = GFL_BGSysIsScrHeapExists(BG_LINES);

    sys_memcpy(screen + SCREEN_ROW_WORDS * from, screen + SCREEN_ROW_WORDS * to, SCREEN_ROW_SIZE);
    GFL_BGSysQueueScrLoad(BG_LINES);
    if (work->content[from] == CONTENT_SENTENCE && work->content[to] == CONTENT_SENTENCE) {
        PMSDraw_Copy(work->pmsDraw, from, to);
        return;
    }
    if (work->content[from] == CONTENT_MESSAGE && work->content[to] == CONTENT_MESSAGE) {
        GFL_BitmapCopy(BmpWin_GetBitmap(work->windows[from]), BmpWin_GetBitmap(work->windows[to]));
        BmpWin_FlushChar(work->windows[to]);
        return;
    }
    UnionSubdisp_DrawLine(work, func_ov036_021b4620(list, index - 2 + from), to);
}

// Scrolls the lines by delta lines, drawing the ones that come in
static void UnionSubdisp_ScrollLines(UnionSubdisp *work, UnionVisitorList *list, int delta) {
    int from;
    int to;

    if (delta > 0) {
        from = delta;
        to = 0;
        for (; from < LINE_COUNT; from++, to++) {
            UnionSubdisp_MoveLine(work, from, to, list, list->viewEnd - delta);
        }
        for (to = LINE_COUNT - delta; to < LINE_COUNT; to++) {
            UnionVisitor *visitor = func_ov036_021b4620(list, list->viewEnd - 2 + to);

            GFL_ASSERT(visitor != NULL);
            UnionSubdisp_DrawLine(work, visitor, to);
        }
    } else {
        to = 2;
        from = delta + 2;
        for (; from > -1; from--, to--) {
            UnionSubdisp_MoveLine(work, from, to, list, list->viewEnd - delta);
        }
        for (to = -1 - delta; to > -1; to--) {
            UnionVisitor *visitor = func_ov036_021b4620(list, list->viewEnd - 2 + to);

            GFL_ASSERT_MSG(visitor != NULL, "view_no=%d, write_pos=%d, offset=%d\n", list->viewEnd, to, delta);
            UnionSubdisp_DrawLine(work, visitor, to);
        }
    }
}

// Whether the knob is being held: when it is touched, and while the touch lasts
static BOOL UnionSubdisp_UpdateKnobTouch(UnionSubdisp *work) {
    ClActorPos pos;
    u32 x;
    u32 y;
    BOOL held = TRUE;

    if (!func_0203da84(&x, &y)) {
        return FALSE;
    }
    func_0204c178(work->actors[ACTOR_KNOB], &pos, 1);
    if (work->holdingKnob == 0) {
        if (!func_0203da48()) {
            return FALSE;
        }
        if (pos.x - 11 <= x && pos.x + 11 >= x && pos.y - 11 <= y && pos.y + 11 >= y) {
            return held;
        }
    } else {
        if (y < TRACK_TOP) {
            y = TRACK_TOP;
        } else if (y > TRACK_BOTTOM) {
            y = TRACK_BOTTOM;
        }
        pos.y = y;
        func_0204c140(work->actors[ACTOR_KNOB], &pos, 1);
        return TRUE;
    }
    return FALSE;
}

static void UnionSubdisp_UpdateKnob(UnionSystem *sys, UnionSubdisp *work) {
    if (work->holdingKnob == 1) {
        UnionSubdisp_DragKnob(work, &sys->visitorList);
    } else {
        UnionSubdisp_PlaceKnob(work, &sys->visitorList);
    }
}

// Scrolls the lines to where the knob is
static void UnionSubdisp_DragKnob(UnionSubdisp *work, UnionVisitorList *list) {
    ClActorPos pos;
    int range;
    int offset;
    int step;
    int i;
    int line;

    func_0204c178(work->actors[ACTOR_KNOB], &pos, 1);
    offset = (pos.y - TRACK_TOP) << 8;
    range = UnionSubdisp_GetScrollRange(list);
    step = UnionSubdisp_GetScrollStep(list, range);
    offset += step >> 1;
    line = 0;
    i = 0;
    if (range > 0) {
        do {
            int next = line + 1;

            if (offset <= step * next) {
                break;
            }
            i++;
            line = next;
        } while (i < range);
    }
    if (line >= range) {
        line = range;
    }
    list->viewEnd = list->count - range + line;
    if (list->viewEnd < 3) {
        if (list->count < 3) {
            list->viewEnd = list->count;
        } else {
            list->viewEnd = 2;
        }
    }
    if (list->viewEnd < 0 || list->viewEnd > list->count) {
        list->viewEnd = list->count;
    }
}

// Puts the knob where the lines are
static void UnionSubdisp_PlaceKnob(UnionSubdisp *work, UnionVisitorList *list) {
    ClActorPos pos;
    int range = UnionSubdisp_GetScrollRange(list);
    u32 step = UnionSubdisp_GetScrollStep(list, range);

    if (list->count - list->viewEnd > CHAT_VISIBLE_MAX - 2) {
        list->viewEnd = list->count - (CHAT_VISIBLE_MAX - 2);
    }
    pos.y = TRACK_BOTTOM - (((list->count - list->viewEnd) * step) >> 8);
    pos.x = KNOB_X;
    func_0204c140(work->actors[ACTOR_KNOB], &pos, 1);
}

// How many lines the knob moves over
static int UnionSubdisp_GetScrollRange(UnionVisitorList *list) {
    int count = list->count;

    if (count >= 0x1e) {
        count = CHAT_VISIBLE_MAX;
    }
    count -= 2;
    if (count < 0) {
        count = 0;
    }
    return count;
}

static u32 UnionSubdisp_GetScrollStep(UnionVisitorList *list, u32 range) {
    return 0x8000 / range;
}

// Whether the track of the knob was touched, which puts the knob there
static BOOL UnionSubdisp_UpdateTrackTouch(UnionSubdisp *work) {
    ClActorPos pos;
    u32 x;
    u32 y;

    if (!func_0203dac8(&x, &y)) {
        return FALSE;
    }
    if (x >= TRACK_LEFT && x <= 0xff && y >= TRACK_TOP && y <= TRACK_BOTTOM) {
        pos.x = KNOB_X;
        pos.y = y;
        func_0204c140(work->actors[ACTOR_KNOB], &pos, 1);
        return TRUE;
    }
    return FALSE;
}

// Whether a line was touched, which lights it and takes the player of it as the one to show the focus on
static BOOL UnionSubdisp_UpdateLineTouch(UnionSystem *sys, UnionSubdisp *work, UnionVisitorList *list) {
    u32 x;
    u32 y;
    int line;
    UnionVisitor *visitor;

    if (!func_0203dac8(&x, &y)) {
        return FALSE;
    }
    if (x <= 0xe8) {
        for (line = 0; line < LINE_COUNT; line++) {
            if (y >= line * LINE_HEIGHT && y <= line * LINE_HEIGHT + LINE_HEIGHT) {
                break;
            }
        }
    }
    if (line >= LINE_COUNT || line > list->count) {
        return FALSE;
    }
    if (UnionSubdisp_SetLinePalette(work, line, TRUE) == 1) {
        if (list->viewEnd >= 3) {
            line = list->viewEnd - 2 + line;
        }
        UnionSubdisp_StartFlash(work, list, line);
        visitor = func_ov036_021b4620(list, line);
        if (visitor != NULL) {
            sys_memcpy(visitor->mac, sys->situ.mac78, 6);
        }
        UnionSubdisp_CancelOtherFlashes(work, list, line);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return TRUE;
    }
    return FALSE;
}

static UnionSubdispFlash *UnionSubdisp_FindFlash(UnionSubdisp *work, UnionVisitorList *list, int line) {
    int i;

    for (i = 0; i < LINE_COUNT; i++) {
        if (work->flash[i].timer != 0 && line == work->flash[i].line) {
            return &work->flash[i];
        }
    }
    for (i = 0; i < LINE_COUNT; i++) {
        if (work->flash[i].timer == 0) {
            return &work->flash[i];
        }
    }
    for (i = 0; i < LINE_COUNT; i++) {
        if (work->flash[i].line > list->viewEnd || work->flash[i].line <= list->viewEnd - 3) {
            return &work->flash[i];
        }
    }
    GFL_ASSERT(0);
    return NULL;
}

static BOOL UnionSubdisp_StartFlash(UnionSubdisp *work, UnionVisitorList *list, int line) {
    UnionSubdispFlash *flash = UnionSubdisp_FindFlash(work, list, line);

    if (flash == NULL) {
        return FALSE;
    }
    flash->timer = FLASH_FRAMES;
    flash->line = line;
    return TRUE;
}

static void UnionSubdisp_UpdateFlash(UnionSubdisp *work, UnionVisitorList *list) {
    int i;

    for (i = 0; i < LINE_COUNT; i++) {
        if (work->flash[i].timer != 0) {
            work->flash[i].timer--;
            if (work->flash[i].timer == 0) {
                int line;

                if (list->viewEnd < 3) {
                    line = work->flash[i].line;
                } else {
                    line = work->flash[i].line + 2 - list->viewEnd;
                }
                if (line >= 0 && line < LINE_COUNT) {
                    UnionSubdisp_SetLinePalette(work, line, FALSE);
                }
            }
        }
    }
}

static void UnionSubdisp_CancelOtherFlashes(UnionSubdisp *work, UnionVisitorList *list, int line) {
    int i;

    for (i = 0; i < LINE_COUNT; i++) {
        if (work->flash[i].timer != 0 && line != work->flash[i].line) {
            work->flash[i].timer = 1;
        }
    }
}

// Lights a line, or puts it back
static BOOL UnionSubdisp_SetLinePalette(UnionSubdisp *work, int line, BOOL touched) {
    UnionSubdispScreenEntry *screen = GFL_BGSysIsScrHeapExists(BG_LINES);
    u32 palette = screen[line * SCREEN_ROW_WORDS].palette;

    switch (palette) {
    case 0:
    case 1:
        palette = touched == FALSE ? 0 : 1;
        break;
    case 2:
    case 3:
        palette = touched == FALSE ? 2 : 3;
        break;
    default:
        return FALSE;
    }
    GFL_BGSysSetScrPaletteNo(BG_LINES, 0, LINE_TILES * line, 32, LINE_TILES, palette);
    GFL_BGSysQueueScrLoad(BG_LINES);
    return TRUE;
}

// Scrolls the lines by three with the L and R buttons
static void UnionSubdisp_UpdateKeyScroll(UnionSubdisp *work, UnionSystem *sys) {
    if (sys != NULL) {
        int count = sys->visitorList.count;
        int viewEnd;

        if (count >= 3) {
            int oldest = count - CHAT_VISIBLE_MAX;

            viewEnd = sys->visitorList.viewEnd;

            if (oldest < 2) {
                oldest = 2;
            }
            if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
                viewEnd -= 3;
                if (viewEnd < oldest) {
                    viewEnd = oldest;
                }
            } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
                viewEnd += 3;
                if (viewEnd > count) {
                    viewEnd = count;
                }
            }
            sys->visitorList.viewEnd = viewEnd;
        }
    }
}
