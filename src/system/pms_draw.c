#include "system/pms_draw.h"
#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "system/pms_data.h"
#include "system/pms_word.h"
#include "system/printsys.h"

// Draws sentences into windows: the text through a print queue, and the words that are icons as cell actors over the
// window. Our names

// The files of ARCID_PMSI the icons use
#define PMSI_PALETTE_FILE 1
#define PMSI_ICON_PALETTE_COUNT 5
#define PMSI_ICON_CELL_FILE_1D_128K 0x20
#define PMSI_ICON_CELL_FILE_1D_32K 0x21
#define PMSI_ICON_CELL_FILE_1D_64K 0x22
#define PMSI_ICON_ANIM_FILE_1D_128K 0x26
#define PMSI_ICON_ANIM_FILE_1D_32K 0x27
#define PMSI_ICON_ANIM_FILE_1D_64K 0x28
#define PMSI_ICON_CHAR_FILE_1D_128K 0x2c
#define PMSI_ICON_CHAR_FILE_1D_32K 0x2d
#define PMSI_ICON_CHAR_FILE_1D_64K 0x2e

// The space a word that is left empty takes, in pixels
#define PMS_DRAW_EMPTY_WORD_WIDTH 84
#define PMS_DRAW_LINE_HEIGHT 16

// The icons' resources and the unit their actors are on
typedef struct {
    ClActUnit *unit;
    u32 vramType;
    u32 palette;
    u32 chars;
    u32 cellAnims;
} PMSDrawRes;

// One sentence, with an actor for each word that may be an icon
typedef struct {
    BmpWin *window;
    // Whether the text is still in the print queue
    u8 printing;
    ClActor *icons[PMS_SENTENCE_WORD_MAX];
    // The BG's offset when the icons were last moved with it
    int scrollX;
    int scrollY;
    u32 drawn : 1;
    u32 visible : 1;
    u32 showIcons : 1;
    u32 unk1C;
    BOOL isIcon[PMS_SENTENCE_WORD_MAX];
} PMSDrawSlot;

struct PMSDraw {
    u16 heapId;
    Font *font;
    PrintQueue *queue;
    PMSDrawSlot *slots;
    PMSDrawRes res;
    u16 color;
    BOOL printEnd;
    BOOL followScroll;
    u8 count;
    u8 backColor;
};

static void PMSDraw_GetIconPos(BmpWin *window, u32 width, u32 lines, const PMSDrawPos *offset, PMSDrawPos *pos);
static u32 PMSDraw_GetIconCellFile(u32 vramType);
static u32 PMSDraw_GetIconAnimFile(u32 vramType);
static u32 PMSDraw_GetIconCharFile(u32 vramType);
static void PMSDrawRes_Load(PMSDrawRes *res, u8 palette, HeapID heapId);
static void PMSDrawRes_Free(PMSDrawRes *res);
static ClActor *PMSDrawRes_CreateIcon(PMSDrawRes *res, HeapID heapId);
static void PMSDraw_SetIcon(ClActor *icon, BmpWin *window, u32 width, u32 lines, u32 word, const PMSDrawPos *offset);
static void PMSDrawSlot_Init(PMSDrawSlot *slot, PMSDrawRes *res, HeapID heapId);
static void PMSDrawSlot_Delete(PMSDrawSlot *slot);
static BOOL PMSDrawSlot_Main(PMSDrawSlot *slot, PrintQueue *queue, BOOL followScroll);
static void PMSDrawSlot_Print(PMSDrawSlot *slot, PrintQueue *queue, Font *font, BmpWin *window, const PMSData *sentence,
                              const PMSDrawPos *offset, u16 color, u8 backColor, HeapID heapId);
static void PMSDrawSlot_Clear(PMSDrawSlot *slot, BOOL clearScreen);
static void PMSDrawSlot_SetVisible(PMSDrawSlot *slot, BOOL visible);
static u32 PMSDraw_GetSurface(u32 bg);

PMSDraw *PMSDraw_Create(ClActUnit *unit, u32 vramType, PrintQueue *queue, Font *font, u8 palette, u8 count,
                        HeapID heapId) {
    int i;
    PMSDraw *draw = GFL_HeapAllocate(heapId, sizeof(PMSDraw), TRUE, "pms_draw.c", 130);

    MI_CpuFill8(draw, 0, sizeof(PMSDraw));
    draw->heapId = heapId;
    draw->queue = queue;
    draw->font = font;
    draw->count = count;
    draw->backColor = 15;
    draw->followScroll = FALSE;
    draw->slots = GFL_HeapAllocate(heapId, count * sizeof(PMSDrawSlot), TRUE, "pms_draw.c", 141);
    MI_CpuFill8(draw->slots, 0, count * sizeof(PMSDrawSlot));
    draw->res.unit = unit;
    draw->res.vramType = vramType;
    draw->color = PRINT_COLOR(1, 2, 0);
    PMSDrawRes_Load(&draw->res, palette, draw->heapId);
    for (i = 0; i < draw->count; i++) {
        PMSDrawSlot_Init(&draw->slots[i], &draw->res, draw->heapId);
    }
    return draw;
}

void PMSDraw_Main(PMSDraw *draw) {
    int i;

    draw->printEnd = TRUE;
    for (i = 0; i < draw->count; i++) {
        if (draw->slots[i].drawn && PMSDrawSlot_Main(&draw->slots[i], draw->queue, draw->followScroll)) {
            draw->printEnd = FALSE;
        }
    }
}

void PMSDraw_Delete(PMSDraw *draw) {
    int i;

    for (i = 0; i < draw->count; i++) {
        PMSDrawSlot_Delete(&draw->slots[i]);
    }
    GFL_HeapFree(draw->slots);
    PMSDrawRes_Free(&draw->res);
    GFL_HeapFree(draw);
}

void PMSDraw_Print(PMSDraw *draw, BmpWin *window, const PMSData *sentence, u8 slot) {
    PMSDrawPos offset = { 0, 0 };

    PMSDraw_PrintEx(draw, window, sentence, slot, &offset);
}

void PMSDraw_PrintEx(PMSDraw *draw, BmpWin *window, const PMSData *sentence, u8 slot, const PMSDrawPos *pos) {
    PMSDrawSlot_Print(&draw->slots[slot], draw->queue, draw->font, window, sentence, pos, draw->color, draw->backColor,
                      draw->heapId);
    draw->printEnd = FALSE;
}

BOOL PMSDraw_IsPrintEnd(PMSDraw *draw) {
    return draw->printEnd;
}

void PMSDraw_Clear(PMSDraw *draw, u8 slot, BOOL clearScreen) {
    PMSDrawSlot_Clear(&draw->slots[slot], clearScreen);
}

void PMSDraw_SetVisible(PMSDraw *draw, u8 slot, BOOL visible) {
    PMSDrawSlot_SetVisible(&draw->slots[slot], visible);
}

BOOL PMSDraw_IsDrawn(PMSDraw *draw, u8 slot) {
    return draw->slots[slot].drawn;
}

void PMSDraw_SetIconVisible(PMSDraw *draw, u8 slot, BOOL visible) {
    if (PMSDraw_IsDrawn(draw, slot)) {
        draw->slots[slot].showIcons = visible;
    }
}

void PMSDraw_SetObjMode(PMSDraw *draw, u8 slot, u32 mode) {
    int i;

    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        ClActor_SetObjMode(draw->slots[slot].icons[i], mode);
    }
}

void PMSDraw_Copy(PMSDraw *draw, u8 src, u8 dest) {
    int i;
    ClActorPos pos;
    PMSDrawSlot *srcSlot = &draw->slots[src];
    PMSDrawSlot *destSlot = &draw->slots[dest];
    int moveX = BmpWin_GetPosX(destSlot->window) - BmpWin_GetPosX(srcSlot->window);
    int moveY = BmpWin_GetPosY(destSlot->window) - BmpWin_GetPosY(srcSlot->window);

    GFL_BitmapCopy(BmpWin_GetBitmap(srcSlot->window), BmpWin_GetBitmap(destSlot->window));
    BmpWin_Transfer(destSlot->window);
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        u32 surface = PMSDraw_GetSurface(BmpWin_GetBGIndex(srcSlot->window));

        ClActor_GetSurfacePos(srcSlot->icons[i], &pos, surface);
        pos.x += moveX * 8;
        pos.y += moveY * 8;
        ClActor_SetSurfacePos(destSlot->icons[i], &pos, surface);
        ClActor_SetSequence(destSlot->icons[i], ClActor_GetSequence(srcSlot->icons[i]));
        ClActor_SetVisible(destSlot->icons[i], srcSlot->isIcon[i]);
    }
    destSlot->printing = srcSlot->printing;
    destSlot->drawn = srcSlot->drawn;
    destSlot->visible = srcSlot->visible;
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        destSlot->isIcon[i] = srcSlot->isIcon[i];
    }
}

void PMSDraw_SetBackColor(PMSDraw *draw, u8 color) {
    draw->backColor = color;
}

void PMSDraw_SetColor(PMSDraw *draw, u16 color) {
    draw->color = color;
}

void PMSDraw_SetFollowScroll(PMSDraw *draw, BOOL follow) {
    draw->followScroll = follow;
}

// Where an icon goes on the screen: after width pixels of text on line lines of the sentence
static void PMSDraw_GetIconPos(BmpWin *window, u32 width, u32 lines, const PMSDrawPos *offset, PMSDrawPos *pos) {
    pos->x = offset->x + (width + BmpWin_GetPosX(window) * 8);
    pos->y = offset->y + (lines * PMS_DRAW_LINE_HEIGHT + BmpWin_GetPosY(window) * 8);
}

static u32 PMSDraw_GetIconCellFile(u32 vramType) {
    GXOBJVRamModeChar mode;

    if (vramType == CLACT_VRAM_MAIN) {
        mode = GX_GetOBJVRamModeChar();
    } else {
        mode = GXS_GetOBJVRamModeChar();
    }
    switch (mode) {
    case GX_OBJVRAMMODE_CHAR_1D_32K:
        return PMSI_ICON_CELL_FILE_1D_32K;
    case GX_OBJVRAMMODE_CHAR_1D_64K:
        return PMSI_ICON_CELL_FILE_1D_64K;
    case GX_OBJVRAMMODE_CHAR_1D_128K:
        return PMSI_ICON_CELL_FILE_1D_128K;
    default:
        return PMSI_ICON_CELL_FILE_1D_128K;
    }
}

static u32 PMSDraw_GetIconAnimFile(u32 vramType) {
    GXOBJVRamModeChar mode;

    if (vramType == CLACT_VRAM_MAIN) {
        mode = GX_GetOBJVRamModeChar();
    } else {
        mode = GXS_GetOBJVRamModeChar();
    }
    switch (mode) {
    case GX_OBJVRAMMODE_CHAR_1D_32K:
        return PMSI_ICON_ANIM_FILE_1D_32K;
    case GX_OBJVRAMMODE_CHAR_1D_64K:
        return PMSI_ICON_ANIM_FILE_1D_64K;
    case GX_OBJVRAMMODE_CHAR_1D_128K:
        return PMSI_ICON_ANIM_FILE_1D_128K;
    default:
        return PMSI_ICON_ANIM_FILE_1D_128K;
    }
}

static u32 PMSDraw_GetIconCharFile(u32 vramType) {
    GXOBJVRamModeChar mode;

    if (vramType == CLACT_VRAM_MAIN) {
        mode = GX_GetOBJVRamModeChar();
    } else {
        mode = GXS_GetOBJVRamModeChar();
    }
    switch (mode) {
    case GX_OBJVRAMMODE_CHAR_1D_32K:
        return PMSI_ICON_CHAR_FILE_1D_32K;
    case GX_OBJVRAMMODE_CHAR_1D_64K:
        return PMSI_ICON_CHAR_FILE_1D_64K;
    case GX_OBJVRAMMODE_CHAR_1D_128K:
        return PMSI_ICON_CHAR_FILE_1D_128K;
    default:
        // BUG: The default is the 128K animation file, not the characters. No screen uses another OBJ mapping, so it
        // is never reached
#ifdef BUGFIX
        return PMSI_ICON_CHAR_FILE_1D_128K;
#else
        return PMSI_ICON_ANIM_FILE_1D_128K;
#endif
    }
}

static void PMSDrawRes_Load(PMSDrawRes *res, u8 palette, HeapID heapId) {
    u32 cellFile = PMSDraw_GetIconCellFile(res->vramType);
    u32 animFile = PMSDraw_GetIconAnimFile(res->vramType);
    u32 charFile = PMSDraw_GetIconCharFile(res->vramType);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PMSI, heapId);

    res->palette = ClActSys_LoadPlttRange(arc, PMSI_PALETTE_FILE, res->vramType, palette * 0x20, 0,
                                          PMSI_ICON_PALETTE_COUNT, heapId);
    res->chars = ClActSys_LoadChars(arc, charFile, FALSE, res->vramType, heapId);
    res->cellAnims = ClActSys_LoadCellAnims(arc, cellFile, animFile, heapId);
    GFL_ArcToolFree(arc);
}

static void PMSDrawRes_Free(PMSDrawRes *res) {
    ClActSys_FreePltt(res->palette);
    ClActSys_FreeChars(res->chars);
    ClActSys_FreeCellAnims(res->cellAnims);
}

static ClActor *PMSDrawRes_CreateIcon(PMSDrawRes *res, HeapID heapId) {
    ClActorSetup setup = { 0 };
    ClActor *icon = ClActor_Create(res->unit, res->chars, res->palette, res->cellAnims, &setup, res->vramType, heapId);

    ClActor_SetVisible(icon, FALSE);
    ClActor_SetAutoAnim(icon, TRUE);
    if (GCTX_HIDGetUpdateRate() == 30) {
        ClActor_SetAutoAnimSpeed(icon, FX32_CONST(2));
    }
    return icon;
}

static void PMSDraw_SetIcon(ClActor *icon, BmpWin *window, u32 width, u32 lines, u32 word, const PMSDrawPos *offset) {
    PMSDrawPos pos;
    ClActorPos actorPos;
    u8 bg;

    PMSDraw_GetIconPos(window, width, lines, offset, &pos);
    actorPos.x = pos.x;
    actorPos.y = pos.y;
    bg = BmpWin_GetBGIndex(window);
    ClActor_SetSurfacePos(icon, &actorPos, PMSDraw_GetSurface(bg));
    ClActor_SetBgPriority(icon, GFL_BGSysGetBGPriority(bg));
    ClActor_SetSequence(icon, word - 1);
}

static void PMSDrawSlot_Init(PMSDrawSlot *slot, PMSDrawRes *res, HeapID heapId) {
    int i;

    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        slot->icons[i] = PMSDrawRes_CreateIcon(res, heapId);
    }
    slot->drawn = FALSE;
    slot->showIcons = TRUE;
}

static void PMSDrawSlot_Delete(PMSDrawSlot *slot) {
    int i;

    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        ClActor_Delete(slot->icons[i]);
    }
}

// Returns whether the text is still printing
static BOOL PMSDrawSlot_Main(PMSDrawSlot *slot, PrintQueue *queue, BOOL followScroll) {
    int i;
    BOOL printing;

    if (slot->printing && !PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(slot->window))) {
        BmpWin_FlushChar(slot->window);
        slot->printing = FALSE;
    }
    printing = slot->printing ? TRUE : FALSE;
    if (!printing && slot->visible) {
        for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
            if (slot->isIcon[i] && slot->showIcons) {
                ClActor_SetVisible(slot->icons[i], TRUE);
            } else {
                ClActor_SetVisible(slot->icons[i], FALSE);
            }
        }
    }
    if (followScroll) {
        u32 bg = BmpWin_GetBGIndex(slot->window);
        ClActorPos move = { 0, 0 };
        ClActorPos pos;
        int scrollX = GFL_BGSysGetBGOffsetX2(bg);
        int scrollY = GFL_BGSysGetBGOffsetY2(bg);

        if (scrollX != slot->scrollX) {
            move.x = slot->scrollX - scrollX;
            slot->scrollX = scrollX;
        }
        if (scrollY != slot->scrollY) {
            move.y = slot->scrollY - scrollY;
            slot->scrollY = scrollY;
        }
        if (move.x != 0 || move.y != 0) {
            for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
                ClActor_GetPos(slot->icons[i], &pos);
                pos.x += move.x;
                pos.y += move.y;
                ClActor_SetPos(slot->icons[i], &pos);
            }
        }
    }
    return printing;
}

static void PMSDrawSlot_Print(PMSDrawSlot *slot, PrintQueue *queue, Font *font, BmpWin *window, const PMSData *sentence,
                              const PMSDrawPos *offset, u16 color, u8 backColor, HeapID heapId) {
    int wordCount;
    int i;
    StrBuf *format;
    StrBuf *str;
    BOOL isEmpty[PMS_SENTENCE_WORD_MAX] = { FALSE, FALSE };
    PMSData data;

    // Draws a valid copy of the sentence, leaving the caller's as it is
    PMSData_Copy(&data, sentence);
    sentence = &data;
    PMSData_Validate(&data, TRUE, heapId);
    slot->window = window;
    slot->printing = FALSE;
    format = PMSData_GetSentenceString(sentence, heapId);
    wordCount = (u8)GFL_StrCmdGetWordSetCommandCount(format);
    for (i = 0; i < wordCount; i++) {
        if (PMSData_IsWordNumber(sentence, i)) {
            slot->isIcon[i] = TRUE;
        } else if (PMSData_GetWordIndex(sentence, i) == PMS_WORD_NULL) {
            isEmpty[i] = TRUE;
        }
    }
    str = PMSData_ToStringWithWords(sentence, heapId, wordCount);
    GFL_BitmapFill(BmpWin_GetBitmap(slot->window), backColor);
    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(slot->window), offset->x, offset->y, str, font, color);
    slot->printing = TRUE;
    GFL_StrBufFree(str);
    for (i = 0; i < wordCount; i++) {
        if (slot->isIcon[i] == TRUE) {
            u8 width = GFL_StrCmdGetStrWidthUntilWordSetIndex(format, i, font, 0);
            u8 lines = GFL_StrCmdCountLinesUntilWordSetIndex(format, i);

            PMSDraw_SetIcon(slot->icons[i], window, width, lines, PMSData_GetWordIndex(sentence, i), offset);
        } else if (isEmpty[i] == TRUE) {
            u8 width = GFL_StrCmdGetStrWidthUntilWordSetIndex(format, i, font, 0);
            u8 lines = GFL_StrCmdCountLinesUntilWordSetIndex(format, i);

            GFL_BitmapFillArea(BmpWin_GetBitmap(window), width, lines * PMS_DRAW_LINE_HEIGHT, PMS_DRAW_EMPTY_WORD_WIDTH,
                               PMS_DRAW_LINE_HEIGHT, backColor);
        }
    }
    GFL_StrBufFree(format);
    BmpWin_FlushMap(slot->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(slot->window));
    slot->drawn = TRUE;
    slot->visible = TRUE;
}

static void PMSDrawSlot_Clear(PMSDrawSlot *slot, BOOL clearScreen) {
    int i;

    GFL_BitmapFill(BmpWin_GetBitmap(slot->window), 0);
    if (clearScreen) {
        BmpWin_FlushChar(slot->window);
        BmpWin_ClearScreenNow(slot->window);
    }
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        ClActor_SetVisible(slot->icons[i], FALSE);
        slot->isIcon[i] = FALSE;
    }
    slot->drawn = FALSE;
    slot->scrollX = 0;
    slot->scrollY = 0;
    slot->visible = FALSE;
}

static void PMSDrawSlot_SetVisible(PMSDrawSlot *slot, BOOL visible) {
    int i;

    slot->visible = visible;
    if (visible) {
        BmpWin_FlushMap(slot->window);
    } else {
        BmpWin_ClearScreen(slot->window);
    }
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(slot->window));
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        if (slot->isIcon[i]) {
            ClActor_SetVisible(slot->icons[i], visible);
        }
    }
}

// The surface of the default renderer that a BG's screen is
static u32 PMSDraw_GetSurface(u32 bg) {
    return bg >= BGSYS_BG_SUB ? CLACT_SURFACE_SUB : CLACT_SURFACE_MAIN;
}
