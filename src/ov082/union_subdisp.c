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

// A line that was touched, lit until the timer runs out
typedef struct {
    u16 timer;
    u8 line;
} UnionSubdispFlash;

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
    PrintWindow print[LINE_COUNT];
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
static void UnionSubdisp_InitIcons(UnionSubdisp *work, ArcTool *arc);
static void UnionSubdisp_FreeIcons(UnionSubdisp *work);
static BOOL UnionSubdisp_UpdateIcons(UnionSubdisp *work, UnionSystem *sys);
static void UnionSubdisp_SelectIcon(UnionSubdisp *work, u32 icon);
static void UnionSubdisp_UpdateChat(UnionSubdisp *work, UnionVisitorList *list);
static void UnionSubdisp_DrawLine(UnionSubdisp *work, UnionVisitor *chat, u8 line);
static void UnionSubdisp_DrawAllLines(UnionSubdisp *work, UnionVisitorList *list);
static void UnionSubdisp_MoveLine(UnionSubdisp *work, u8 from, u8 to, UnionVisitorList *list, int index);
static void UnionSubdisp_ScrollLines(UnionSubdisp *work, UnionVisitorList *list, int offset);
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

// The frame, the lines' background and the chat lines. One array: three objects would share a section with
// sIconStates, ahead of the icons' and the messages' local initializers
static const BGSetup sBGSetups[] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x00000), 0x6800,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x00000), 0x6800,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x00000), 0x6800,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
};

// Shows the selected icon as selected. The loop's test is a 0/1 value, as in UnionSubdisp_SelectIcon: the original
// enters both loops with a jump to their test, which a plain comparison doesn't give
static inline void UnionSubdisp_ShowSelected(const UnionSubdisp *work) {
    u32 i;

    if (work->selected == 0) {
        return;
    }
    for (i = 0; i < ICON_COUNT ? TRUE : FALSE; i++) {
        if (sIconStates[i] == work->selected) {
            func_0204c378(work->actors[i], 1, 1);
            return;
        }
    }
}

UnionSubdisp *UnionSubdisp_Create(GameSystem *gsys) {
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);
    UnionSystem *sys;
    ArcTool *arc;
    UnionSubdisp *work = GFL_HeapAllocate(HEAPID_FIELDMAP, sizeof(UnionSubdisp), TRUE, "union_subdisp.c", 271);

    work->gsys = gsys;
    arc = GFL_ArcSysCreateFileHandle(ARCID_UNION_SUBDISP, HEAPID_FIELDMAP);
    UnionSubdisp_InitUnit(work);
    UnionSubdisp_InitBG(work, arc);
    UnionSubdisp_InitWindows(work);
    UnionSubdisp_InitIconResources(work, arc);
    UnionSubdisp_InitIcons(work, arc);
    UnionSubdisp_InitFrame(work);
    GFL_ArcToolFree(arc);
    work->pmsDraw = PMSDraw_Create(work->unit, CLACT_VRAM_SUB, work->queue, work->font, 9, LINE_COUNT, HEAPID_FIELDMAP);
    PMSDraw_SetBackColor(work->pmsDraw, 0);
    sys = GameCommSys_GetWork(comm);
    if (sys != NULL) {
        work->selected = func_ov028_021710c8(sys);
        UnionSubdisp_DrawAllLines(work, &sys->visitorList);
        UnionSubdisp_ShowSelected(work);
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

// PrintWindow_Flush that tells whether the window has nothing left to send. The result is a u32 and not a BOOL: as
// a BOOL the compiler folds it into the caller's test, and the original keeps it as a value
static inline u32 UnionSubdisp_FlushPrint(PrintWindow *print, PrintQueue *queue) {
    if (print->flushPending != 0) {
        if (func_02021c1c(queue, BmpWin_GetBitmap(print->window)) == 0) {
            BmpWin_FlushChar(print->window);
            print->flushPending = 0;
        }
    }
    if (print->flushPending == 0) {
        return TRUE;
    }
    return FALSE;
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
            UnionSubdisp_FlushPrint(&work->print[i], work->queue);
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

    GFL_BGSysCreateBG(BG_FRAME, &sBGSetups[0], BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_LINES, &sBGSetups[1], BGMODE_TEXT);
    GFL_BGSysCreateBG(BG_TEXT, &sBGSetups[2], BGMODE_TEXT);
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
        PrintWindow_Init(&work->print[i], work->windows[i]);
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

static void UnionSubdisp_InitIcons(UnionSubdisp *work, ArcTool *arc) {
    ClActorSetup setup = { 0, 180, 0, 10, 0 };
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
        work->selected = 0;
    } else {
        work->selected = state;
    }
    for (i = 0; i <= ICON_COUNT - 1 ? TRUE : FALSE; i++) {
        if (i == icon && work->selected != 0) {
            func_0204c378(work->actors[i], 1, 1);
        } else {
            func_0204c378(work->actors[i], 0, 1);
        }
    }
}

static void UnionSubdisp_UpdateChat(UnionSubdisp *work, UnionVisitorList *list) {
    BOOL redrawn = FALSE;
    int i;

    if (list->drawnEnd == list->viewEnd) {
        return;
    }
    if (!PMSDraw_IsPrintEnd(work->pmsDraw)) {
        return;
    }
    for (i = 0; i < LINE_COUNT; i++) {
        if (work->content[i] == CONTENT_MESSAGE) {
            if (!UnionSubdisp_FlushPrint(&work->print[i], work->queue)) {
                return;
            }
        }
    }
    if (list->count < 3) {
        int index;

        for (index = list->drawnEnd; index <= list->viewEnd; index++) {
            if (index > -1) {
                UnionVisitor *chat = func_ov036_021b4620(list, index);

                GFL_ASSERT(chat != NULL);
                UnionSubdisp_DrawLine(work, chat, index);
            }
        }
    } else {
        int delta = list->viewEnd - list->drawnEnd;
        int size = delta < 0 ? -delta : delta;

        if (size >= 3 || list->drawnEnd < 2) {
            UnionSubdisp_DrawAllLines(work, list);
        } else {
            UnionSubdisp_ScrollLines(work, list, delta);
        }
        redrawn = TRUE;
    }
    if (redrawn == TRUE) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
    list->drawnEnd = list->viewEnd;
}

static void UnionSubdisp_DrawLine(UnionSubdisp *work, UnionVisitor *chat, u8 line) {
    u16 *screen = GFL_BGSysIsScrHeapExists(BG_LINES);
    u16 *row = screen + SCREEN_ROW_WORDS * line;
    int palette = chat->gender == 0 ? 0 : 2;
    BmpWin **window;
    StrBuf *strbuf;
    StrBuf *name;

    sys_memcpy(screen, row, SCREEN_ROW_SIZE);
    GFL_BGSysSetScrPaletteNo(BG_LINES, 0, LINE_TILES * line, 32, LINE_TILES, palette);
    GFL_BGSysQueueScrLoad(BG_LINES);
    {
        PMSDrawPos pos = { 0, 20 };
        u32 *content = work->content;

        if (content[line] == CONTENT_SENTENCE) {
            if (PMSDraw_IsDrawn(work->pmsDraw, line) == TRUE) {
                PMSDraw_Clear(work->pmsDraw, line, TRUE);
            }
        } else if (content[line] == CONTENT_MESSAGE) {
            BmpWin **windows = work->windows;

            GFL_BitmapFill(BmpWin_GetBitmap(windows[line]), 0);
            BmpWin_FlushChar(windows[line]);
        }
        if (chat->kind == UNION_CHAT_KIND_SENTENCE) {
            window = &work->windows[line];
            PMSDraw_PrintEx(work->pmsDraw, work->windows[line], &chat->sentence, line, &pos);
            content[line] = CONTENT_SENTENCE;
        } else {
            BmpWin **windows;

            switch (chat->kind) {
            case UNION_CHAT_KIND_ENTERED:
                strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0400_Text_IveEnteredUnionRoom);
                break;
            case UNION_CHAT_KIND_BATTLE:
                strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0400_Text_TouchBattleIconIf);
                break;
            case UNION_CHAT_KIND_TRADE:
                strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0400_Text_TouchTradeIconIf);
                break;
            case UNION_CHAT_KIND_DRAW:
                strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0400_Text_TouchDrawIconIf);
                break;
            case UNION_CHAT_KIND_SPIN_TRADE:
                strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0400_Text_TouchSpinTradeIcon);
                break;
            default:
                return;
            }
            windows = work->windows;
            window = &windows[line];
            BmpWin_FlushMap(windows[line]);
            GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(windows[line]));
            PrintWindow_Print(&work->print[line], work->queue, 0, pos.y, strbuf, work->font, PRINT_COLOR_LINE);
            GFL_StrBufFree(strbuf);
            content[line] = CONTENT_MESSAGE;
        }
    }
    name = GFL_StrBufCreate(8, HEAPID_FIELDMAP);
    GFL_StrBufLoadString(name, chat->name);
    func_02021c7c(work->queue, BmpWin_GetBitmap(*window), 0, 0, name, work->font, PRINT_COLOR_LINE);
    GFL_StrBufFree(name);
    if (chat->subKind != 0) {
        u32 message = Bank0400_Text_Friend;

        if (chat->subKind != 1) {
            message = Bank0400_Text_Player;
        }
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, message);
        func_02021c7c(work->queue, BmpWin_GetBitmap(*window), 0x74, 0, strbuf, work->font, PRINT_COLOR_LINE);
        GFL_StrBufFree(strbuf);
    }
}

// Draws the three newest lines, the newest at the bottom
static void UnionSubdisp_DrawAllLines(UnionSubdisp *work, UnionVisitorList *list) {
    int view_no;
    int write_pos;
    int oldest;
    UnionVisitor *chat;

    oldest = list->viewEnd - 3;
    if (oldest < 0) {
        oldest = -1;
    }
    write_pos = list->viewEnd - oldest - 1;
    for (view_no = list->viewEnd; view_no > oldest; view_no--) {
        chat = func_ov036_021b4620(list, view_no);
        GFL_ASSERT(chat != NULL);
        GFL_ASSERT(write_pos > -1);
        UnionSubdisp_DrawLine(work, chat, write_pos);
        write_pos--;
    }
}

// Moves a line to another place of the screen, with what it shows
static void UnionSubdisp_MoveLine(UnionSubdisp *work, u8 from, u8 to, UnionVisitorList *list, int index) {
    u16 *src = GFL_BGSysIsScrHeapExists(BG_LINES);
    u16 *dst = src;

    dst += SCREEN_ROW_WORDS * to;
    src += SCREEN_ROW_WORDS * from;
    sys_memcpy(src, dst, SCREEN_ROW_SIZE);
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
static void UnionSubdisp_ScrollLines(UnionSubdisp *work, UnionVisitorList *list, int offset) {
    // The SDK's s32 is long, which keeps the first loop's test on the counter and not on offset
    long from;
    int write_pos;
    UnionVisitor *chat;

    if (offset > 0) {
        from = offset;
        write_pos = 0;
        for (; from < LINE_COUNT; from++) {
            UnionSubdisp_MoveLine(work, from, write_pos, list, list->viewEnd - offset);
            write_pos++;
        }
        for (write_pos = LINE_COUNT - offset; write_pos < LINE_COUNT; write_pos++) {
            chat = func_ov036_021b4620(list, list->viewEnd - 2 + write_pos);
            GFL_ASSERT(chat != NULL);
            UnionSubdisp_DrawLine(work, chat, write_pos);
        }
    } else {
        write_pos = 2;
        for (from = offset + 2; from > -1; from--) {
            UnionSubdisp_MoveLine(work, from, write_pos, list, list->viewEnd - offset);
            write_pos--;
        }
        for (write_pos = -1 - offset; write_pos > -1; write_pos--) {
            chat = func_ov036_021b4620(list, list->viewEnd - 2 + write_pos);
            GFL_ASSERT_MSG(chat != NULL, "view_no=%d, write_pos=%d, offset=%d\n", list->viewEnd, write_pos, offset);
            UnionSubdisp_DrawLine(work, chat, write_pos);
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
    line = 0;
    offset += step >> 1;
    for (i = 0; i < range; i++) {
        if (offset <= step * (line + 1)) {
            break;
        }
        line++;
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
    int index;
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
        index = line;
        if (list->viewEnd >= 3) {
            index = list->viewEnd - 2 + line;
        }
        UnionSubdisp_StartFlash(work, list, index);
        visitor = func_ov036_021b4620(list, index);
        if (visitor != NULL) {
            sys_memcpy(visitor->mac, sys->situ.mac78, 6);
        }
        UnionSubdisp_CancelOtherFlashes(work, list, index);
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
    u16 *screen = GFL_BGSysIsScrHeapExists(BG_LINES);
    u16 palette = screen[line * SCREEN_ROW_WORDS] >> 12;

    switch (palette) {
    case 0:
    case 1:
        if (touched == FALSE) {
            palette = 0;
        } else {
            palette = 1;
        }
        break;
    case 2:
    case 3:
        if (touched == FALSE) {
            palette = 2;
        } else {
            palette = 3;
        }
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
    if (sys != NULL && sys->visitorList.count >= 3) {
        int viewEnd = sys->visitorList.viewEnd;
        int count = sys->visitorList.count;
        int oldest = count - CHAT_VISIBLE_MAX;

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
