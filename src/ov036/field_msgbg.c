// The field's message BG: its message windows, money and list windows, the message balloons over actors, and their
// cursors and input. The ROM names the file in its heap allocations. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). First written by mariano54 on their parallel-decomp branch
// Additional checker-window APIs, helpers and table names are our descriptive guesses.
#include "field/field_msgbg.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/talkmsgwin.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "system/bmp_menulist.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "text/system/yes_no.h"

// The value of a BG index that has not been created
#define MSGBG_NO_BG 0xff

// The BGs of the windows: the talk windows' and the second, which is alpha blended over the field
#define MSGBG_BG 1
#define MSGBG_BG2 2

// The palettes: the sign and checker windows', the frame of the first BG's windows, the talk windows' and the first
// BG's, the talk windows' frame, and the frame and the windows of the second BG, which is alpha blended
#define MSGBG_PLTT_SIGN 9
#define MSGBG_PLTT_FRAME 10
#define MSGBG_PLTT_TALK 11
#define MSGBG_PLTT_TALK_FRAME 12
#define MSGBG_PLTT_BG2_FRAME 13
#define MSGBG_PLTT_BG2 14

// The size of a palette of 16 colors, in bytes
#define MSGBG_PLTT_SIZE 0x20

// The first characters of the frames in the BGs: the system message window's, the talk windows', the list window's and
// the sign's
#define MSGBG_CHAR_FRAME 1
#define MSGBG_CHAR_TALK_FRAME 0xb
#define MSGBG_CHAR_LIST_FRAME 0x24
#define MSGBG_CHAR_SIGN_FRAME 0x37

// The windows' background color, and a byte of two pixels of it, which the windows are cleared with
#define MSGBG_COLOR_BG 15
#define MSGBG_COLOR_BG_FILL 0xff

// The printers that the windows share, and the numbered talk windows, which take the talk windows after the balloons'
#define MSGBG_PRINTER_COUNT 4
#define MSGBG_NUMBERED_WIN_COUNT 8

// The length of the windows' strings, and of the numbered talk windows' messages
#define MSGBG_STRBUF_LEN 0xa0
#define MSGBG_NUMBERED_STRBUF_LEN 0x40

// The events of a message's commands that the print streams pass on: one plays a knock, and the other waits until the
// sounds have finished
#define MSGBG_EVENT_KNOCK 3
#define MSGBG_EVENT_WAIT_SOUND 6

// The sign window's states: print the first page, scroll in, check for another page, wait for the A or B button and
// print it, scroll to it, wait at the last page, scroll out, and closed
enum {
    SIGN_STATE_PRINT,
    SIGN_STATE_SCROLL_IN,
    SIGN_STATE_CHECK_PAGE,
    SIGN_STATE_WAIT_PAGE,
    SIGN_STATE_SCROLL_PAGE,
    SIGN_STATE_WAIT_END,
    SIGN_STATE_SCROLL_OUT,
    SIGN_STATE_CLOSED,
};

// The sign window's styles, each a palette of its frame
#define SIGN_STYLE_COUNT 4

// How far below its place the sign window is when hidden, and how many pixels it scrolls in or out and to the next
// page in a frame
#define SIGN_HIDDEN_Y (-0x30)
#define SIGN_SCROLL_SPEED 0xc
#define SIGN_PAGE_SCROLL_SPEED 4

// The planes that the second BG's windows are alpha blended over
#define MSGBG_BLEND_PLANES_UNDER                                                                                       \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ)

// How the player advances a message: automatically, or with a held button
typedef struct {
    u8 autoAdvance;
    u8 timer;
    u8 held;
    u8 pad;
} MsgBGInput;

// The state of a window's wait-for-input cursor
struct MsgBGCursor {
    u8 shown;
    u8 frame;
    u8 timer;
    GFLBitmap *bitmap;
};

// Prints strings in a window through the system's print queue
typedef struct {
    FieldMsgBG *sys;
    BOOL idle;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    BmpWin *window;
    StrBuf *strbuf;
    PrintWindow printWindow;
} MsgBGPrinter;

// A message balloon over an actor, which shakes as it opens
struct ActorMsgWin {
    u8 unk0;
    u8 type;
    s16 shake;
    StrBuf *strbuf;
    TalkMsgWinSys *talk;
    MsgBGCursor cursor;
    FieldMsgBG *sys;
    const VecFx32 *posSource;
    VecFx32 pos;
    u8 unk28[12];
};

// A message in a numbered talk window
typedef struct {
    StrBuf *strbuf;
    u32 userId;
    u32 index;
    TalkMsgWinSys *talk;
    u32 unk10[2];
} NumberedMsgWin;

// The numbered windows' text origin in pixels, also included in their measured bounds
typedef struct {
    u8 x;
    u8 y;
} NumberedMsgWinTextOrigin;

struct FieldMsgBG {
    HeapID heapId;
    u16 bg;
    u16 bg2;
    u16 palette;
    Font *font;
    PrintQueue *queue;
    MsgBGPrinter printers[MSGBG_PRINTER_COUNT];
    NumberedMsgWin *numberedWins[MSGBG_NUMBERED_WIN_COUNT];
    ActorMsgWin actorWins[ACTOR_MSG_WIN_COUNT];
    TalkMsgWinSys *talk;
    MsgBGInput input;
    TCBExManager *tcbMgr;
    G3DCamera *camera;
    TCB *vblankTcb;
    u8 resetBG2;
    u8 alphaBlend;
    void *bgCharFile;
    NNSG2dCharacterData *charData;
};

// A message printed a character at a time
typedef struct {
    u16 unk0;
    u16 wait;
    PrintStream *stream;
    FieldMsgBG *sys;
} MsgStream;

// The talk and system message windows
struct FieldMsgWin {
    BmpWin *window;
    MsgStream *stream;
    MsgBGPrinter *printer;
    MsgData *msgData;
    StrBuf *strbuf;
    FieldMsgBG *sys;
    MsgBGCursor cursor;
};

// The money and list windows
struct MoneyWin {
    BmpWin *window;
    MsgBGPrinter *printer;
    FieldMsgBG *sys;
};

struct ListMenuUI {
    FieldMsgBG *sys;
    MsgBGPrinter *printer;
    BmpWin *window;
    ListMenuOption *options;
    BmpMenuList *list;
};

// The info message window, drawn with the talk window's frame, which shakes as it opens
struct InfoMsgWin {
    MsgBGCursor cursor;
    StrBuf *strbuf;
    FieldMsgBG *sys;
    TalkMsgWinSys *talk;
    MsgData *msgData;
    BmpWin *window;
    MsgStream *stream;
    MsgBGPrinter *printer;
    u32 state;
    s16 shake;
};

// The sign window, which scrolls in from below and shows its message two lines at a time
struct SignMsgWin {
    FieldMsgBG *sys;
    BmpWin *window;
    GFLBitmap *nextPage;
    GFLBitmap *page;
    const StrBuf *src;
    StrBuf *line;
    u32 state;
    s32 scroll;
    u32 pageScroll;
    u32 lineNo;
    MsgBGCursor cursor;
    Font *font;
};

// The checker window, whose frame is drawn in its own bitmap
struct CheckerMsgWin {
    BmpWin *window;
    u32 unk4;
    FieldMsgBG *sys;
    GFLBitmap *cursorBack;
    MsgBGCursor cursor;
};

static MsgBGPrinter *MsgBGPrinter_Create(FieldMsgBG *sys, MsgData *msgData, BmpWin *window);
static void MsgBGPrinter_Free(MsgBGPrinter *printer);
static void MsgBGPrinter_PrintMessage(MsgBGPrinter *printer, u16 x, u16 y, u32 messageId);
static void MsgBGPrinter_PrintStrBuf(MsgBGPrinter *printer, u16 x, u16 y, StrBuf *strbuf);
static void MsgBGPrinter_PrintColorFont(MsgBGPrinter *printer, u16 x, u16 y, StrBuf *strbuf, u16 color, Font *font);
static void MsgBGPrinter_PrintColor(MsgBGPrinter *printer, u16 x, u16 y, StrBuf *strbuf, u16 color);
static BOOL MsgBGPrinter_IsIdle(MsgBGPrinter *printer);
static void MsgBGPrinter_Clear(MsgBGPrinter *printer);
static void MsgBGPrinter_ClearArea(MsgBGPrinter *printer, u32 x, u32 y, u32 width, u32 height);
static PrintQueue *MsgBGPrinter_GetQueue(MsgBGPrinter *printer);
static PrintWindow *MsgBGPrinter_GetPrintWindow(MsgBGPrinter *printer);
static MoneyWin *MoneyWin_Create(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height, u16 palette);
static ListMenuUI *ListMenuUI_CreateCore(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options,
                                         u16 listTop, u16 cursorRow, u16 palette,
                                         BmpMenuListCursorCallback cursorCallback,
                                         BmpMenuListPrintCallback printCallback, void *work);
static ListMenuUI *func_ov036_02187e44(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options,
                                       u16 listTop, u16 cursorRow);
static u32 FieldMsgBG_GetWidestOption(FieldMsgBG *sys, ListMenuOption *options, u32 *widestIndex, u32 spacing);
static void BmpMenuListSetup_Init(BmpMenuListHeader *header, const ListMenuRequest *request);

// The yes/no menu's request, which func_ov036_021880d4 copies and sets the menu's position and size in. It is defined
// after the BG setup functions, where the size sort lays it out after their setups, as the ROM has it
static const ListMenuRequest sYesNoMenuRequest;

static MsgStream *func_ov036_02188188(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y, u32 wait, u16 a6);
static BOOL func_ov036_021881fc(u32 event);
static MsgStream *func_ov036_02188220(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y, u32 wait);
static void MsgStream_Free(MsgStream *stream);
static u32 MsgStream_Update(MsgStream *stream);
static FieldMsgWin *func_ov036_021882c0(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height);
static void ActorMsgWin_Create(FieldMsgBG *sys, ActorMsgWin *win, u32 type, const VecFx32 *pos, StrBuf *strbuf,
                               u32 frameType, u32 tailBase);
static void func_ov036_02188a00(FieldMsgBG *sys, InfoMsgWin *win, u32 x, u32 y, u16 width, u16 height, u32 frameType);
static void func_ov036_02188cc0(FieldMsgBG *sys, NumberedMsgWin *win, StrBuf *strbuf, u16 index, u8 x, u8 y, u8 width,
                                u8 height, u32 userId, u32 frameType);
static void func_ov036_02188e70(NumberedMsgWin *win);
static BOOL func_ov036_02188eec(NumberedMsgWin *win);
static void func_ov036_021892b0(u32 bg, u32 palSlot, u32 style, HeapID heapId);
static void func_ov036_021892f0(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal);
static void func_ov036_021894c8(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal);
static BOOL func_ov036_02189660(const StrBuf *src, StrBuf *dest, u32 line);
static BOOL func_ov036_02189674(GFLBitmap *bitmap, Font *font, const StrBuf *src, StrBuf *dest, u32 *line, u8 color);
static BOOL func_ov036_021896f0(GFLBitmap *dest, GFLBitmap *src1, GFLBitmap *src2, int amount, u16 color);
static void CheckerMsgWin_LoadPalette(u32 bg, u32 slot, HeapID heapId);
static void CheckerMsgWin_LoadFrameTile(GFLBitmap *bitmap, u32 style, HeapID heapId, u32 tile);
static void CheckerMsgWin_DrawFrame(GFLBitmap *bitmap, u32 style, HeapID heapId);
static void FieldMsgBG_DrawTextLines(u16 x, u16 y, GFLBitmap *bitmap, Font *font, const StrBuf *text, HeapID heapId);
static void MsgBGCursor_Init(MsgBGCursor *cursor, FieldMsgBG *sys);
static void MsgBGCursor_Free(MsgBGCursor *cursor);
static BOOL MsgBGCursor_IsShown(MsgBGCursor *cursor);
static void MsgBGCursor_Clear(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color);
static void MsgBGCursor_Draw(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color);
static void func_ov036_02189e20(MsgBGCursor *cursor, GFLBitmap *bitmap, GFLBitmap *dest);
static void func_ov036_02189f10(HeapID heapId);
static void func_ov036_02189f2c(BmpWin *win);
static BmpWin *func_ov036_0218a048(u8 x, u8 y, u8 w, u8 h, HeapID heapId);
static void func_ov036_0218a08c(BmpWin *win);
static void func_ov036_0218a0e8(BmpWin *win);
static void func_ov036_0218a0f8(FieldMsgBG *sys, BOOL loadFrame);
static void func_ov036_0218a218(FieldMsgBG *sys, BOOL loadFrame);
static void func_ov036_0218a334(FieldMsgBG *sys);
static void func_ov036_0218a3e0(TCB *tcb, void *data);
static void func_ov036_0218a408(u32 create, u32 alphaBlend);
static void func_ov036_0218a4a4(FieldMsgBG *sys);
static void func_ov036_0218a4dc(u32 bg, HeapID heapId);
static void func_ov036_0218a524(u32 bg, HeapID heapId, u32 mode);
static void setAlphaBlend(BOOL on);
static BmpWin *FieldMsgBG_CreateBmpWin(u32 bg, u32 heapId, u32 x, u32 y, u16 w, u16 h, u16 pal);
static void func_ov036_0218a668(BmpWin *win);
static void func_ov036_0218a67c(u32 heapId);
static void loadPltt(u32 heapId);
static int FieldMsgBG_AddNumberedWindow(FieldMsgBG *sys, NumberedMsgWin *win);
static NumberedMsgWin *FieldMsgBG_TakeNumberedWindow(FieldMsgBG *sys, u32 userId);
static void MsgBGInput_Init(MsgBGInput *input);
static void MsgBGInput_Reset(MsgBGInput *input);
static void MsgBGInput_SetAutoAdvance(MsgBGInput *input, BOOL value);
static BOOL MsgBGInput_GetAutoAdvance(MsgBGInput *input);
static s32 MsgBGInput_GetTextWait(MsgBGInput *input);
static BOOL func_ov036_0218a760(MsgBGInput *input);
static BOOL func_ov036_0218a798(MsgBGInput *input, BOOL fast);
static void func_ov036_0218a7d8(PrintStream *stream, u32 mode);

FieldMsgBG *CreateFieldMsgBGSystem(HeapID heapId, G3DCamera *camera) {
    FieldMsgBG *sys = GFL_HeapAllocate(heapId, sizeof(FieldMsgBG), TRUE, "field_msgbg.c", 422);

    sys->heapId = heapId;
    sys->bg = MSGBG_NO_BG;
    sys->bg2 = MSGBG_NO_BG;
    sys->camera = camera;
    sys->alphaBlend = TRUE;
    sys->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, sys->heapId);
    sys->queue = PrintQueue_CreateDefault(sys->heapId);
    sys->tcbMgr = GFL_TCBExMgrCreate(sys->heapId, sys->heapId, 1, 4);
    MsgBGInput_Init(&sys->input);
    sys->bgCharFile = GFL_ArcSysReadHeapNewLZ(ARCID_WINFRAME, 11, FALSE, heapId);
    NNS_G2dGetUnpackedBGCharacterData(sys->bgCharFile, &sys->charData);
    return sys;
}

void func_ov036_02187760(FieldMsgBG *sys) {
    func_ov036_0218a0f8(sys, TRUE);
}

void func_ov036_0218776c(FieldMsgBG *sys) {
    func_ov036_0218a218(sys, FALSE);
    if (sys->vblankTcb == NULL) {
        sys->vblankTcb = GFL_VBlankTCBAdd(func_ov036_0218a3e0, sys, 0);
    }
}

void func_ov036_02187794(u32 bg, HeapID heapId) {
    func_ov036_0218a4dc(bg, heapId);
}

void func_ov036_0218779c(FieldMsgBG *sys, u32 step) {
    func_ov036_0218a524(sys->bg, sys->heapId, step);
}

void func_ov036_021877ac(FieldMsgBG *sys) {
    int i = 0;
    MsgBGPrinter *printer = sys->printers;

    if (sys->bgCharFile != NULL) {
        GFL_HeapFree(sys->bgCharFile);
    }
    if (sys->talk != NULL) {
        TalkMsgWinSys_Free(sys->talk);
    }
    if (sys->tcbMgr != NULL) {
        GFL_TCBExMgrFree(sys->tcbMgr);
    }
    if (sys->bg != MSGBG_NO_BG) {
        GFL_BGSysFreeFilledChar(sys->bg, 1, 0);
        GFL_BGSysReleaseBG(sys->bg);
    }
    if (sys->bg2 != MSGBG_NO_BG) {
        GFL_BGSysReleaseBG(sys->bg2);
    }
    for (; i < MSGBG_PRINTER_COUNT; i++, printer++) {
        if (printer->strbuf != NULL) {
            GFL_StrBufFree(printer->strbuf);
        }
    }
    if (sys->queue != NULL) {
        PrintQueue_Free(sys->queue);
    }
    if (sys->font != NULL) {
        GFL_FontFree(sys->font);
    }
    GFL_HeapFree(sys);
}

void func_ov036_02187834(FieldMsgBG *sys) {
    if (sys->talk != NULL) {
        TalkMsgWinSys_Free(sys->talk);
        sys->talk = NULL;
    }
    if (sys->bg != MSGBG_NO_BG) {
        GFL_BGSysFreeFilledChar(sys->bg, 1, 0);
        GFL_BGSysReleaseBG(sys->bg);
        sys->bg = MSGBG_NO_BG;
    }
}

BOOL func_ov036_02187868(FieldMsgBG *sys) {
    if (sys->bg2 != MSGBG_NO_BG) {
        GFL_BGSysReleaseBG(sys->bg2);
        sys->bg2 = MSGBG_NO_BG;
        return TRUE;
    }
    return FALSE;
}

void func_ov036_02187888(FieldMsgBG *sys) {
    if (sys->talk != NULL) {
        TalkMsgWinSys_Free(sys->talk);
    }
    func_ov036_0218a334(sys);
    if (sys->vblankTcb == NULL) {
        sys->vblankTcb = GFL_VBlankTCBAdd(func_ov036_0218a3e0, sys, 0);
    }
}

BOOL func_ov036_021878bc(FieldMsgBG *sys) {
    if (sys->vblankTcb == NULL) {
        return TRUE;
    }
    return FALSE;
}

void func_ov036_021878d0(FieldMsgBG *sys) {
    MsgBGPrinter *printer;
    int i;
    NumberedMsgWin **numberedWin;

    PrintQueue_Main(sys->queue);
    numberedWin = sys->numberedWins;
    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++, numberedWin++) {
        if (*numberedWin != NULL) {
            func_ov036_02188eec(*numberedWin);
        }
    }
    printer = sys->printers;
    for (i = 0; i < MSGBG_PRINTER_COUNT; i++, printer++) {
        if (printer->queue != NULL) {
            PrintWindow_Flush(&printer->printWindow, printer->queue);
            printer->idle = PrintWindow_IsPrinted(&printer->printWindow);
        }
    }
    GFL_TCBExMgrUpdate(sys->tcbMgr);
    if (sys->talk != NULL) {
        TalkMsgWinSys_Update(sys->talk);
        func_ov036_0218b2e8(sys->talk);
    }
}

void func_ov036_0218796c(FieldMsgBG *sys) {
    if (sys->talk != NULL) {
        TalkMsgWinSys_Draw(sys->talk);
    }
}

void func_ov036_02187980(FieldMsgBG *sys) {
    PrintQueue_Clear(sys->queue);
}

BOOL func_ov036_0218798c(FieldMsgBG *sys) {
    return PrintQueue_IsEmpty(sys->queue);
}

PrintQueue *func_ov036_02187998(FieldMsgBG *sys) {
    return sys->queue;
}

Font *func_ov036_0218799c(FieldMsgBG *sys) {
    return sys->font;
}

MsgData *func_ov036_021879a0(FieldMsgBG *sys, u32 fileId) {
    return GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, fileId, sys->heapId);
}

void func_ov036_021879b8(MsgData *msgData) {
    GFL_MsgDataFree(msgData);
}

void func_ov036_021879c0(FieldMsgBG *sys) {
    sys->resetBG2 = TRUE;
}

void func_ov036_021879cc(FieldMsgBG *sys, BOOL enable) {
    MsgBGInput_SetAutoAdvance(&sys->input, enable);
}

static MsgBGPrinter *MsgBGPrinter_Create(FieldMsgBG *sys, MsgData *msgData, BmpWin *window) {
    int i = 0;
    MsgBGPrinter *printer = sys->printers;

    for (; i < MSGBG_PRINTER_COUNT; i++, printer++) {
        if (printer->queue == NULL) {
            printer->sys = sys;
            printer->window = window;
            printer->idle = TRUE;
            printer->font = sys->font;
            printer->queue = sys->queue;
            printer->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
            printer->msgData = msgData;
            printer->printWindow.window = window;
            printer->printWindow.flushPending = FALSE;
            return printer;
        }
    }
    return NULL;
}

static void MsgBGPrinter_Free(MsgBGPrinter *printer) {
    printer->font = NULL;
    printer->queue = NULL;
    GFL_StrBufFree(printer->strbuf);
    printer->strbuf = NULL;
    printer->msgData = NULL;
}

static void MsgBGPrinter_PrintMessage(MsgBGPrinter *printer, u16 x, u16 y, u32 messageId) {
    GFL_MsgDataLoadStrbuf(printer->msgData, messageId, printer->strbuf);
    PrintWindow_PrintNoColor(&printer->printWindow, printer->queue, x, y, printer->strbuf, printer->font);
}

static void MsgBGPrinter_PrintStrBuf(MsgBGPrinter *printer, u16 x, u16 y, StrBuf *strbuf) {
    PrintWindow_PrintNoColor(&printer->printWindow, printer->queue, x, y, strbuf, printer->font);
}

static void MsgBGPrinter_PrintColorFont(MsgBGPrinter *printer, u16 x, u16 y, StrBuf *strbuf, u16 color, Font *font) {
    PrintWindow_Print(&printer->printWindow, printer->queue, x, y, strbuf, font, color);
}

static void MsgBGPrinter_PrintColor(MsgBGPrinter *printer, u16 x, u16 y, StrBuf *strbuf, u16 color) {
    MsgBGPrinter_PrintColorFont(printer, x, y, strbuf, color, printer->font);
}

static BOOL MsgBGPrinter_IsIdle(MsgBGPrinter *printer) {
    return printer->idle;
}

static void MsgBGPrinter_Clear(MsgBGPrinter *printer) {
    GFL_BitmapFill(BmpWin_GetBitmap(printer->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(printer->window));
}

static void MsgBGPrinter_ClearArea(MsgBGPrinter *printer, u32 x, u32 y, u32 width, u32 height) {
    GFL_BitmapFillArea(BmpWin_GetBitmap(printer->window), x, y, width, height, MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(printer->window));
}

static PrintQueue *MsgBGPrinter_GetQueue(MsgBGPrinter *printer) {
    return printer->queue;
}

static PrintWindow *MsgBGPrinter_GetPrintWindow(MsgBGPrinter *printer) {
    return &printer->printWindow;
}

static MoneyWin *MoneyWin_Create(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height, u16 palette) {
    u8 bg = sys->bg;
    MoneyWin *win;

    func_ov036_0218a4a4(sys);
    if (palette == MSGBG_PLTT_BG2) {
        bg = sys->bg2;
    }
    win = GFL_HeapAllocate(sys->heapId, sizeof(MoneyWin), TRUE, "field_msgbg.c", 1158);
    win->sys = sys;
    win->window = FieldMsgBG_CreateBmpWin(bg, sys->heapId, x, y, width, height, palette);
    win->printer = MsgBGPrinter_Create(sys, msgData, win->window);
    if (palette == MSGBG_PLTT_BG2) {
        func_ov036_0218a67c(sys->heapId);
        setAlphaBlend(sys->alphaBlend);
    } else {
        loadPltt(sys->heapId);
    }
    return win;
}

MoneyWin *FieldMsgBG_CreateMoneyWin(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height) {
    sys->palette = MSGBG_PLTT_BG2;
    return MoneyWin_Create(sys, msgData, x, y, width, height, sys->palette);
}

void func_ov036_02187c1c(MoneyWin *win) {
    func_ov036_0218a668(win->window);
    MsgBGPrinter_Free(win->printer);
    GFL_HeapFree(win);
}

void func_ov036_02187c34(MoneyWin *win, u16 x, u16 y, u32 messageId) {
    MsgBGPrinter_PrintMessage(win->printer, x, y, messageId);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win->window));
}

void func_ov036_02187c4c(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf) {
    MsgBGPrinter_PrintStrBuf(win->printer, x, y, strbuf);
}

void func_ov036_02187c58(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf, u16 color, Font *font) {
    MsgBGPrinter_PrintColorFont(win->printer, x, y, strbuf, color, font);
}

BOOL func_ov036_02187c70(MoneyWin *win) {
    return MsgBGPrinter_IsIdle(win->printer);
}

void func_ov036_02187c7c(MoneyWin *win) {
    MsgBGPrinter_Clear(win->printer);
}

void func_ov036_02187c88(MoneyWin *win, u32 x, u32 y, u32 width, u32 height) {
    MsgBGPrinter_ClearArea(win->printer, x, y, width, height);
}

BmpWin *func_ov036_02187c9c(MoneyWin *win) {
    return win->window;
}

MoneyWin *func_ov036_02187ca0(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height) {
    MoneyWin *win;

    sys->palette = MSGBG_PLTT_BG2;
    func_ov036_0218a4a4(sys);
    win = GFL_HeapAllocate(sys->heapId, sizeof(MoneyWin), TRUE, "field_msgbg.c", 1379);
    win->sys = sys;
    win->window = func_ov036_0218a048(x, y, width, height, sys->heapId);
    win->printer = MsgBGPrinter_Create(sys, msgData, win->window);
    setAlphaBlend(sys->alphaBlend);
    return win;
}

void func_ov036_02187d10(MoneyWin *win) {
    func_ov036_0218a0e8(win->window);
    MsgBGPrinter_Free(win->printer);
    GFL_HeapFree(win);
}

void func_ov036_02187d28(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf) {
    MsgBGPrinter_PrintStrBuf(win->printer, x + 8, y, strbuf);
}

void func_ov036_02187d38(MoneyWin *win) {
    MsgBGPrinter_Clear(win->printer);
}

static ListMenuUI *ListMenuUI_CreateCore(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options,
                                         u16 listTop, u16 cursorRow, u16 palette,
                                         BmpMenuListCursorCallback cursorCallback,
                                         BmpMenuListPrintCallback printCallback, void *work) {
    u8 bg = sys->bg;
    ListMenuUI *menu;
    BmpMenuListHeader header;

    func_ov036_0218a4a4(sys);
    if (palette == MSGBG_PLTT_BG2) {
        bg = sys->bg2;
    }
    menu = GFL_HeapAllocate(sys->heapId, sizeof(ListMenuUI), TRUE, "field_msgbg.c", 1545);
    BmpMenuListSetup_Init(&header, request);
    menu->sys = sys;
    menu->options = options;
    menu->window =
        FieldMsgBG_CreateBmpWin(bg, sys->heapId, request->x, request->y, request->width, request->height, palette);
    menu->printer = MsgBGPrinter_Create(sys, NULL, menu->window);
    header.msgData = NULL;
    header.printWindow = MsgBGPrinter_GetPrintWindow(menu->printer);
    header.queue = MsgBGPrinter_GetQueue(menu->printer);
    header.font = sys->font;
    header.options = options;
    header.printCallback = printCallback;
    header.cursorCallback = cursorCallback;
    header.work = work;
    menu->list = BmpMenuList_Create(&header, listTop, cursorRow, sys->heapId);
    BmpMenuList_LoadCursor(menu->list, sys->heapId);
    if (palette == MSGBG_PLTT_BG2) {
        func_ov036_0218a67c(sys->heapId);
        setAlphaBlend(sys->alphaBlend);
    } else {
        loadPltt(sys->heapId);
    }
    return menu;
}

ListMenuUI *func_ov036_02187e1c(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options, u16 listTop,
                                u16 cursorRow, BmpMenuListCursorCallback cursorCallback,
                                BmpMenuListPrintCallback printCallback, void *work) {
    sys->palette = MSGBG_PLTT_BG2;
    return ListMenuUI_CreateCore(sys, request, options, listTop, cursorRow, sys->palette, cursorCallback, printCallback,
                                 work);
}

static ListMenuUI *func_ov036_02187e44(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options,
                                       u16 listTop, u16 cursorRow) {
    return func_ov036_02187e1c(sys, request, options, listTop, cursorRow, NULL, request->printCallback, NULL);
}

ListMenuUI *func_ov036_02187e60(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options) {
    return func_ov036_02187e44(sys, request, options, 0, 0);
}

ListMenuUI *ListMenuUI_Create(FieldMsgBG *sys, ListMenuRequest *request, ListMenuOption *options,
                              BmpMenuListCursorCallback callback, void *work, u16 listTop, u16 cursorRow,
                              u32 cancelable) {
    ListMenuUI *menu =
        ListMenuUI_CreateCore(sys, request, options, listTop, cursorRow, sys->palette, callback, NULL, work);

    if (cancelable == FALSE) {
        BmpMenuList_SetCancelDisabled(menu->list, TRUE);
    }
    return menu;
}

void func_ov036_02187ea0(ListMenuUI *menu) {
    BmpMenuList_Free(menu->list, NULL, NULL);
    ListMenuCore_FreeOptionList(menu->options);
    func_ov036_0218a668(menu->window);
    MsgBGPrinter_Free(menu->printer);
    GFL_HeapFree(menu);
}

s32 ListMenuUI_Update(ListMenuUI *menu) {
    s32 result;

    if (!func_ov036_0218798c(menu->sys)) {
        return BMPMENULIST_NULL;
    }
    result = BmpMenuList_Update(menu->list);
    if (result == BMPMENULIST_NULL) {
        return BMPMENULIST_NULL;
    }
    if (result == BMPMENULIST_CANCEL) {
        return BMPMENULIST_CANCEL;
    }
    return result;
}

void func_ov036_02187ef8(ListMenuUI *menu) {
    BmpMenuList_Redraw(menu->list);
}

ListMenuOption *InitListMenuOptionHeap(u32 count, HeapID heapId) {
    return ListMenuCore_CreateOptionList(count, heapId);
}

ListMenuOption *func_ov036_02187f0c(const ListMenuRow *rows, s32 count, MsgData *msgData, HeapID heapId) {
    ListMenuOption *options = InitListMenuOptionHeap(count, heapId);
    s32 i;

    for (i = 0; i < count; i++) {
        ListMenuCore_AppendMsgOption(options, msgData, rows[i].messageId, rows[i].value, heapId);
    }
    return options;
}

void AppendListMenuOption(ListMenuOption *options, StrBuf *text, u32 value, HeapID heapId) {
    ListMenuCore_AppendStrBufOption(options, text, value, heapId);
}

void func_ov036_02187f50(ListMenuOption *options) {
    ListMenuCore_FreeStrBufs(options);
}

static u32 FieldMsgBG_GetWidestOption(FieldMsgBG *sys, ListMenuOption *options, u32 *widestIndex, u32 spacing) {
    u32 widest = 0;
    u32 index = 0;
    u32 i = 0;

    while (options->text != LIST_MENU_END) {
        u32 width;

        if (options->text == NULL) {
            break;
        }
        width = GFL_FontGetBlockWidth(options->text, sys->font, spacing);
        if (width > widest) {
            widest = width;
            index = i;
        }
        options++;
        i++;
    }
    *widestIndex = index;
    return widest;
}

u32 ListMenuCore_GetOptionCount(ListMenuOption *options) {
    return ListMenuCore_GetFirstFreeIndex(options);
}

u32 CalcListMenuWidth(FieldMsgBG *sys, ListMenuOption *options, u32 fontSizeX, u32 letterSpacing) {
    u32 widestIndex;
    u32 width = FieldMsgBG_GetWidestOption(sys, options, &widestIndex, letterSpacing);
    u32 tiles;

    width += letterSpacing + 16;
    tiles = width / 8;

    if (width % 8) {
        tiles++;
    }
    return tiles;
}

u32 CalcListMenuHeight(u32 rows, u32 rowHeight, u32 lineSpacing, BOOL scrolls) {
    u32 lineHeight = rowHeight + lineSpacing;
    u32 pixels = rows * lineHeight;
    u32 tiles;

    if (lineHeight < 15) {
        pixels += 15 - lineHeight;
    }
    tiles = (pixels + 7) / 8;
    if (scrolls && tiles * 8 - pixels < 6) {
        tiles++;
    }
    return tiles;
}

void ListMenuRequest_Set(ListMenuRequest *request, u16 count, u16 x, u16 y, u16 width, u16 height) {
    request->count = count;
    request->y = y;
    request->x = x;
    request->width = width;
    request->height = height;
}

void func_ov036_02188008(ListMenuUI *menu) {
    BmpWin_DrawFrame(menu->window, WINFRAME_TRANSFER_NOW, MSGBG_CHAR_FRAME, MSGBG_PLTT_BG2_FRAME);
}

static void BmpMenuListSetup_Init(BmpMenuListHeader *header, const ListMenuRequest *request) {
    MI_CpuFill8(header, 0, sizeof(BmpMenuListHeader));
    header->count = request->count;
    header->maxShown = request->maxShown;
    header->labelX = request->labelX;
    header->itemX = request->itemX;
    header->cursorX = request->cursorX;
    header->y = request->listY;
    header->fgColor = request->fgColor;
    header->bgColor = request->bgColor;
    header->shadowColor = request->shadowColor;
    header->letterSpacing = request->letterSpacing;
    header->lineSpacing = request->lineSpacing;
    header->pageSkip = request->pageSkip;
    header->fontSizeX = request->fontSizeX;
    header->fontSizeY = request->rowHeight;
    header->printCallback = request->printCallback;
    header->wait = 20;
}

ListMenuUI *func_ov036_021880d4(FieldMsgBG *sys, u32 cursorRow) {
    ListMenuRequest request = sYesNoMenuRequest;
    ListMenuRow rows[2] = { { YesNo_Text_Yes, { MSGBG_YES_NO_YES } }, { YesNo_Text_No, { MSGBG_YES_NO_NO } } };
    MsgData *msgData = func_ov036_021879a0(sys, TEXT_BANK_YES_NO);
    ListMenuOption *options = func_ov036_02187f0c(rows, 2, msgData, sys->heapId);
    ListMenuUI *menu;

    GFL_MsgDataFree(msgData);
    ListMenuRequest_Set(&request, 2, 24, 10, 7, 4);
    menu = ListMenuUI_CreateCore(sys, &request, options, 0, cursorRow, sys->palette, NULL, NULL, NULL);
    BmpMenuList_SetCancelDisabled(menu->list, FALSE);
    return menu;
}

u32 func_ov036_0218816c(ListMenuUI *menu) {
    s32 result = ListMenuUI_Update(menu);

    if (result == BMPMENULIST_NULL) {
        return MSGBG_YES_NO_WAITING;
    }
    if (result == BMPMENULIST_CANCEL) {
        return MSGBG_YES_NO_NO;
    }
    return result;
}

static MsgStream *func_ov036_02188188(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y, u32 wait, u16 a6) {
    MsgStream *stream = GFL_HeapAllocate(sys->heapId, sizeof(MsgStream), TRUE, "field_msgbg.c", 2179);

    stream->sys = sys;
    stream->wait = wait;
    stream->stream = PrintStream_CreateWithCallback(window, x, y, strbuf, sys->font, wait, sys->tcbMgr, 0, sys->heapId,
                                                    a6, func_ov036_021881fc);
    MsgBGInput_Reset(&stream->sys->input);
    return stream;
}

static BOOL func_ov036_021881fc(u32 event) {
    switch (event) {
    case MSGBG_EVENT_KNOCK:
        GFL_SndSEPlay(SEQ_SE_KON);
        break;
    case MSGBG_EVENT_WAIT_SOUND:
        return GFL_SndPlayerIsActiveAny();
    }
    return FALSE;
}

static MsgStream *func_ov036_02188220(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y, u32 wait) {
    return func_ov036_02188188(sys, strbuf, window, x, y, wait, 15);
}

static void MsgStream_Free(MsgStream *stream) {
    PrintStream_Delete(stream->stream);
    GFL_HeapFree(stream);
}

static u32 MsgStream_Update(MsgStream *stream) {
    u32 state = PrintStream_GetState(stream->stream);

    switch (state) {
    case PRINT_STREAM_RUNNING:
        if (func_ov036_0218a760(&stream->sys->input)) {
            func_ov036_0218a7d8(stream->stream, 1);
        } else {
            func_ov036_0218a7d8(stream->stream, 0);
        }
        break;
    case PRINT_STREAM_PAUSED:
        if (func_ov036_0218a798(&stream->sys->input, PrintStream_GetPageMode(stream->stream))) {
            if (!MsgBGInput_GetAutoAdvance(&stream->sys->input)) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            PrintStream_Continue(stream->stream);
            state = PRINT_STREAM_RUNNING;
            func_ov036_0218a7d8(stream->stream, 0);
        }
        break;
    case PRINT_STREAM_DONE:
        break;
    }
    return state;
}

static FieldMsgWin *func_ov036_021882c0(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height) {
    FieldMsgWin *win;

    sys->palette = MSGBG_PLTT_BG2;
    func_ov036_0218a4a4(sys);
    win = GFL_HeapAllocate(sys->heapId, sizeof(FieldMsgWin), TRUE, "field_msgbg.c", 2340);
    win->sys = sys;
    win->msgData = msgData;
    win->window = FieldMsgBG_CreateBmpWin(sys->bg2, sys->heapId, x, y, width, height, sys->palette);
    win->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
    MsgBGCursor_Init(&win->cursor, sys);
    func_ov036_0218a67c(sys->heapId);
    setAlphaBlend(sys->alphaBlend);
    return win;
}

void func_ov036_02188338(FieldMsgWin *win) {
    func_ov036_0218a668(win->window);
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    GFL_StrBufFree(win->strbuf);
    MsgBGCursor_Free(&win->cursor);
    GFL_HeapFree(win);
}

void func_ov036_0218836c(FieldMsgWin *win, u16 x, u16 y, u32 messageId) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    GFL_MsgDataLoadStrbuf(win->msgData, messageId, win->strbuf);
    win->stream =
        func_ov036_02188220(win->sys, win->strbuf, win->window, x, y, MsgBGInput_GetTextWait(&win->sys->input));
}

void func_ov036_021883b0(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    win->stream = func_ov036_02188220(win->sys, strbuf, win->window, x, y, MsgBGInput_GetTextWait(&win->sys->input));
}

BOOL func_ov036_021883e8(FieldMsgWin *win) {
    u32 state = MsgStream_Update(win->stream);

    GCTX_HIDGetPressedKeys();
    GCTX_HIDGetHeldKeys();
    switch (state) {
    case PRINT_STREAM_RUNNING:
        if (MsgBGCursor_IsShown(&win->cursor) == TRUE) {
            MsgBGCursor_Clear(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_PAUSED:
        if (!MsgBGInput_GetAutoAdvance(&win->sys->input)) {
            func_ov036_02189de8(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_DONE:
        return TRUE;
    }
    return FALSE;
}

FieldMsgWin *func_ov036_0218845c(FieldMsgBG *sys, MsgData *msgData) {
    return func_ov036_021882c0(sys, msgData, 1, 19, 30, 4);
}

void func_ov036_02188474(FieldMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(win->sys->bg);
}

BmpWin *func_ov036_02188494(FieldMsgWin *win) {
    return win->window;
}

FieldMsgWin *func_ov036_02188498(FieldMsgBG *sys, MsgData *msgData, u32 y) {
    FieldMsgWin *win;

    sys->palette = MSGBG_PLTT_BG2;
    func_ov036_0218a4a4(sys);
    win = GFL_HeapAllocate(sys->heapId, sizeof(FieldMsgWin), TRUE, "field_msgbg.c", 2585);
    win->sys = sys;
    win->msgData = msgData;
    win->window = func_ov036_0218a048(1, y, 30, 4, sys->heapId);
    win->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
    MsgBGCursor_Init(&win->cursor, sys);
    func_ov036_0218a67c(sys->heapId);
    setAlphaBlend(sys->alphaBlend);
    return win;
}

void func_ov036_02188504(FieldMsgWin *win) {
    func_ov036_0218a0e8(win->window);
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    GFL_StrBufFree(win->strbuf);
    MsgBGCursor_Free(&win->cursor);
    GFL_HeapFree(win);
}

void func_ov036_02188538(FieldMsgWin *win, u16 x, u16 y, u32 messageId) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    GFL_MsgDataLoadStrbuf(win->msgData, messageId, win->strbuf);
    win->stream =
        func_ov036_02188220(win->sys, win->strbuf, win->window, x + 8, y, MsgBGInput_GetTextWait(&win->sys->input));
}

void func_ov036_02188580(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    win->stream =
        func_ov036_02188220(win->sys, strbuf, win->window, x + 8, y, MsgBGInput_GetTextWait(&win->sys->input));
}

BOOL func_ov036_021885bc(FieldMsgWin *win) {
    u32 state = MsgStream_Update(win->stream);

    GCTX_HIDGetPressedKeys();
    GCTX_HIDGetHeldKeys();
    switch (state) {
    case PRINT_STREAM_RUNNING:
        if (MsgBGCursor_IsShown(&win->cursor) == TRUE) {
            MsgBGCursor_Clear(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_PAUSED:
        if (!MsgBGInput_GetAutoAdvance(&win->sys->input)) {
            func_ov036_02189de8(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_DONE:
        return TRUE;
    }
    return FALSE;
}

void func_ov036_02188630(FieldMsgWin *win) {
    if (!MsgBGInput_GetAutoAdvance(&win->sys->input)) {
        func_ov036_02189de8(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
        BmpWin_FlushChar(win->window);
    }
}

void func_ov036_02188660(FieldMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(win->sys->bg);
}

void func_ov036_02188680(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, NULL, win->window);
    MsgBGPrinter_PrintStrBuf(win->printer, x + 8, y, strbuf);
}

BmpWin *func_ov036_021886b0(FieldMsgWin *win) {
    return win->window;
}

static void ActorMsgWin_Create(FieldMsgBG *sys, ActorMsgWin *win, u32 type, const VecFx32 *pos, StrBuf *strbuf,
                               u32 frameType, u32 tailBase) {
    win->posSource = pos;
    win->pos = *pos;
    sys->palette = MSGBG_PLTT_TALK;
    win->sys = sys;
    win->talk = sys->talk;
    win->type = type;
    loadPltt(sys->heapId);
    setAlphaBlend(FALSE);
    switch (type) {
    case ACTOR_MSG_WIN_TOP:
        ActorMsgWin_CreateAtTop(win->talk, ACTOR_MSG_WIN_TOP, pos, strbuf, 15, frameType, tailBase,
                                MsgBGInput_GetTextWait(&sys->input));
        break;
    case ACTOR_MSG_WIN_BOTTOM:
        ActorMsgWin_CreateAtBottom(win->talk, ACTOR_MSG_WIN_BOTTOM, pos, strbuf, 15, frameType, tailBase,
                                   MsgBGInput_GetTextWait(&sys->input));
        break;
    default:
        ActorMsgWin_CreateAtAutoPos(win->talk, ACTOR_MSG_WIN_AUTO, pos, strbuf, 15, frameType,
                                    MsgBGInput_GetTextWait(&sys->input));
        break;
    }
    MsgBGInput_Reset(&sys->input);
    func_ov036_0218b4c8(win->talk, win->type);
    MsgBGCursor_Init(&win->cursor, sys);
    if (frameType == TALKMSG_WIN_FRAME_SPIKY) {
        win->shake = 8;
        GFL_SndSEPlay(SEQ_SE_SYS_58);
    }
}

ActorMsgWin *ActorMsgWin_CheckAndCreate(FieldMsgBG *sys, s32 type, const VecFx32 *pos, StrBuf *strbuf, u32 frameType,
                                        u32 tailBase) {
    if (type < ACTOR_MSG_WIN_COUNT) {
        ActorMsgWin *win = &sys->actorWins[type];

        if (win->talk == NULL) {
            ActorMsgWin_Create(sys, win, type, pos, strbuf, frameType, tailBase);
            return win;
        }
    }
    return NULL;
}

void func_ov036_021887d4(ActorMsgWin *win) {
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    MsgBGCursor_Free(&win->cursor);
    func_ov036_0218b4d8(win->talk, win->type);
}

BOOL func_ov036_021887f4(ActorMsgWin *win) {
    if (func_ov036_0218b4e8(win->talk, win->type)) {
        MI_CpuFill8(win, 0, sizeof(ActorMsgWin));
        return TRUE;
    }
    return FALSE;
}

void func_ov036_02188818(ActorMsgWin *win) {
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    func_ov036_0218b4b4(win->talk, win->type);
    MsgBGCursor_Free(&win->cursor);
    MI_CpuFill8(win, 0, sizeof(ActorMsgWin));
}

void func_ov036_02188844(ActorMsgWin *actorWin, StrBuf *strbuf) {
    BmpWin *win = func_ov036_0218b518(actorWin->talk, actorWin->type);

    GFL_BitmapFill(BmpWin_GetBitmap(win), MSGBG_COLOR_BG_FILL);
    func_ov036_0218b524(actorWin->talk, actorWin->type, strbuf, MsgBGInput_GetTextWait(&actorWin->sys->input));
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
}

BOOL func_ov036_02188884(ActorMsgWin *actorWin) {
    BmpWin *win;
    GFLBitmap *bitmap;
    PrintStream *stream;
    u32 state;

    if (!func_ov036_0218b4fc(actorWin->talk, actorWin->type)) {
        return FALSE;
    }
    stream = func_ov036_0218b50c(actorWin->talk, actorWin->type);
    state = PrintStream_GetState(stream);
    GFL_BGSysMoveBG(MSGBG_BG, BG_MOVE_SET_Y, actorWin->shake);
    if (actorWin->shake < 0) {
        actorWin->shake += 2;
        if (actorWin->shake > 0) {
            actorWin->shake = 0;
        }
    }
    actorWin->shake = -actorWin->shake;
    switch (state) {
    case PRINT_STREAM_RUNNING:
        if (MsgBGCursor_IsShown(&actorWin->cursor) == TRUE) {
            win = func_ov036_0218b518(actorWin->talk, actorWin->type);
            MsgBGCursor_Clear(&actorWin->cursor, BmpWin_GetBitmap(win), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win);
        }
        if (func_ov036_0218a760(&actorWin->sys->input)) {
            func_ov036_0218a7d8(stream, 1);
        } else {
            func_ov036_0218a7d8(stream, 0);
        }
        break;
    case PRINT_STREAM_PAUSED:
        win = func_ov036_0218b518(actorWin->talk, actorWin->type);
        bitmap = BmpWin_GetBitmap(win);
        if (MsgBGInput_GetAutoAdvance(&actorWin->sys->input) == FALSE) {
            func_ov036_02189de8(&actorWin->cursor, bitmap, MSGBG_COLOR_BG);
        }
        if (func_ov036_0218a798(&actorWin->sys->input, PrintStream_GetPageMode(stream))) {
            if (MsgBGInput_GetAutoAdvance(&actorWin->sys->input) == FALSE) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            PrintStream_Continue(stream);
            MsgBGCursor_Clear(&actorWin->cursor, bitmap, MSGBG_COLOR_BG);
            func_ov036_0218a7d8(stream, 0);
        }
        BmpWin_FlushChar(win);
        break;
    case PRINT_STREAM_DONE:
        if (actorWin->shake != 0) {
            actorWin->shake = 0;
            GFL_BGSysMoveBG(MSGBG_BG, BG_MOVE_SET_Y, actorWin->shake);
        }
        return TRUE;
    }
    return FALSE;
}

void func_ov036_021889c8(ActorMsgWin *actorWin) {
    BmpWin *win;

    if (MsgBGInput_GetAutoAdvance(&actorWin->sys->input) == FALSE) {
        win = func_ov036_0218b518(actorWin->talk, actorWin->type);
        func_ov036_02189de8(&actorWin->cursor, BmpWin_GetBitmap(win), MSGBG_COLOR_BG);
        BmpWin_FlushChar(win);
    }
}

static void func_ov036_02188a00(FieldMsgBG *sys, InfoMsgWin *win, u32 x, u32 y, u16 width, u16 height, u32 frameType) {
    sys->palette = MSGBG_PLTT_TALK;
    win->talk = sys->talk;
    loadPltt(sys->heapId);
    setAlphaBlend(FALSE);
    win->window = func_ov036_0218b588(win->talk, x, y, width, height, frameType);
    MsgBGCursor_Init(&win->cursor, sys);
}

InfoMsgWin *func_ov036_02188a54(FieldMsgBG *sys, MsgData *msgData, u32 frameType, u16 x, u16 y, u16 width, u16 height) {
    InfoMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(InfoMsgWin), TRUE, "field_msgbg.c", 3276);

    win->sys = sys;
    win->msgData = msgData;
    win->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
    func_ov036_02188a00(sys, win, x, y, width, height, frameType);
    if (frameType == TALKMSG_WIN_FRAME_SPIKY) {
        win->shake = 8;
    }
    return win;
}

void func_ov036_02188ab0(InfoMsgWin *win) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    func_ov036_0218b5d0(win->talk, win->window);
    MsgBGCursor_Free(&win->cursor);
    GFL_HeapFree(win);
}

void func_ov036_02188ae8(InfoMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(win->sys->bg);
}

void func_ov036_02188b08(InfoMsgWin *win) {
    func_ov036_0218b5bc(win->talk, win->window);
}

void func_ov036_02188b18(InfoMsgWin *win, u16 x, u16 y, u32 messageId) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, win->msgData, win->window);
    MsgBGPrinter_PrintMessage(win->printer, x, y, messageId);
}

void func_ov036_02188b44(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, NULL, win->window);
    MsgBGPrinter_PrintStrBuf(win->printer, x, y, strbuf);
}

void func_ov036_02188b70(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf, u16 color) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, NULL, win->window);
    MsgBGPrinter_PrintColor(win->printer, x, y, strbuf, color);
}

void func_ov036_02188ba4(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    win->stream = func_ov036_02188220(win->sys, strbuf, win->window, x, y, MsgBGInput_GetTextWait(&win->sys->input));
}

BOOL func_ov036_02188bdc(InfoMsgWin *win) {
    u32 result;

    GCTX_HIDGetPressedKeys();
    GCTX_HIDGetHeldKeys();
    result = MsgStream_Update(win->stream);
    win->state = PrintStream_GetState(win->stream->stream);
    GFL_BGSysMoveBG(MSGBG_BG, BG_MOVE_SET_Y, win->shake);
    if (win->shake < 0) {
        win->shake += 2;
        if (win->shake > 0) {
            win->shake = 0;
        }
    }
    win->shake = -win->shake;
    switch (result) {
    case PRINT_STREAM_RUNNING:
        if (MsgBGCursor_IsShown(&win->cursor) == TRUE) {
            MsgBGCursor_Clear(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_PAUSED:
        if (MsgBGInput_GetAutoAdvance(&win->sys->input) == FALSE) {
            func_ov036_02189de8(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_DONE:
        if (win->shake != 0) {
            win->shake = 0;
            GFL_BGSysMoveBG(MSGBG_BG, BG_MOVE_SET_Y, win->shake);
        }
        return TRUE;
    }
    return FALSE;
}

void func_ov036_02188c90(InfoMsgWin *win) {
    if (MsgBGInput_GetAutoAdvance(&win->sys->input) == FALSE) {
        func_ov036_02189de8(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
        BmpWin_FlushChar(win->window);
    }
}

u32 func_ov036_02188cbc(InfoMsgWin *win) {
    return win->state;
}

static inline void NumberedMsgWin_InitTextOrigin(NumberedMsgWinTextOrigin *origin) {
    origin->x = 2;
    origin->y = 0;
}

static void func_ov036_02188cc0(FieldMsgBG *sys, NumberedMsgWin *win, StrBuf *strbuf, u16 index, u8 x, u8 y, u8 width,
                                u8 height, u32 userId, u32 frameType) {
    NumberedMsgWinTextOrigin origin;

    NumberedMsgWin_InitTextOrigin(&origin);
    sys->palette = MSGBG_PLTT_TALK;
    win->talk = sys->talk;
    win->index = index;
    win->userId = userId;
    loadPltt(sys->heapId);
    setAlphaBlend(FALSE);
    func_ov036_0218b370(win->talk, win->index, strbuf, x, y, width, height, 0xff, frameType,
                        MsgBGInput_GetTextWait(&sys->input), origin.x, origin.y);
    func_ov036_0218b4c8(win->talk, win->index);
}

void CalcMsgWindowDimensions(FieldMsgBG *sys, StrBuf *str, u8 *width, u8 *height) {
    NumberedMsgWinTextOrigin origin;
    u32 w;
    u32 h;
    u32 pixels;

    NumberedMsgWin_InitTextOrigin(&origin);
    w = GFL_FontGetBlockWidth(str, sys->font, 0) + origin.x;
    h = GFL_FontGetBlockHeight(str, sys->font) + origin.y;

    // Each size becomes tiles in place, rounded up when its pixels don't fill the last tile
    pixels = w;
    w = pixels / 8;
    if (pixels % 8) {
        w++;
    }
    *width = w;
    pixels = h;
    h = pixels / 8;
    if (pixels % 8) {
        h++;
    }
    *height = h;
}

void func_ov036_02188d6c(FieldMsgBG *sys, MsgData *msgData, u32 msgId, u16 userId, u8 x, u8 y, u8 width, u8 height) {
    NumberedMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(NumberedMsgWin), TRUE, "field_msgbg.c", 3662);
    int slot = FieldMsgBG_AddNumberedWindow(sys, win);

    win->strbuf = GFL_StrBufCreate(MSGBG_NUMBERED_STRBUF_LEN, sys->heapId);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    func_ov036_02188cc0(sys, win, win->strbuf, slot + ACTOR_MSG_WIN_COUNT, x, y, width, height, userId,
                        TALKMSG_WIN_FRAME_NORMAL);
}

void func_ov036_02188ddc(FieldMsgBG *sys, StrBuf *strbuf, u16 userId, u8 x, u8 y, u8 width, u8 height) {
    func_ov036_02188dfc(sys, strbuf, userId, x, y, width, height, TALKMSG_WIN_FRAME_NORMAL);
}

void func_ov036_02188dfc(FieldMsgBG *sys, StrBuf *strbuf, u16 userId, u8 x, u8 y, u8 width, u8 height, u32 frameType) {
    NumberedMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(NumberedMsgWin), TRUE, "field_msgbg.c", 3710);
    int slot = FieldMsgBG_AddNumberedWindow(sys, win);

    win->strbuf = GFL_StrBufCreate(GFL_StrBufGetCharCount(strbuf) + 1, sys->heapId);
    GFL_StrBufCopy(win->strbuf, strbuf);
    func_ov036_02188cc0(sys, win, win->strbuf, slot + ACTOR_MSG_WIN_COUNT, x, y, width, height, userId, frameType);
}

static void func_ov036_02188e70(NumberedMsgWin *win) {
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    func_ov036_0218b4b4(win->talk, win->index);
    GFL_HeapFree(win);
}

void func_ov036_02188e90(FieldMsgBG *sys, u16 userId) {
    func_ov036_02188e70(FieldMsgBG_TakeNumberedWindow(sys, userId));
}

void func_ov036_02188e9c(FieldMsgBG *sys) {
    int i;
    NumberedMsgWin *win;

    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++) {
        if (sys->numberedWins[i] != NULL) {
            // BUG: The slot is cleared before the window is taken from it, so the window is never freed, and what is
            // freed is a window read from address 0
#ifdef BUGFIX
            win = sys->numberedWins[i];
            sys->numberedWins[i] = NULL;
#else
            sys->numberedWins[i] = NULL;
            win = sys->numberedWins[i];
            sys->numberedWins[i] = NULL;
#endif
            func_ov036_02188e70(win);
        }
    }
}

BOOL func_ov036_02188ed0(FieldMsgBG *sys) {
    int i;

    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++) {
        if (sys->numberedWins[i] != NULL) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_02188eec(NumberedMsgWin *win) {
    if (!func_ov036_0218b4fc(win->talk, win->index)) {
        return FALSE;
    }
    GCTX_HIDGetPressedKeys();
    GCTX_HIDGetHeldKeys();
    switch (PrintStream_GetState(func_ov036_0218b50c(win->talk, win->index))) {
    case PRINT_STREAM_RUNNING:
    case PRINT_STREAM_PAUSED:
        break;
    case PRINT_STREAM_DONE:
        return TRUE;
    }
    return FALSE;
}

SignMsgWin *func_ov036_02188f28(FieldMsgBG *sys, u16 style) {
    return func_ov036_02188f34(sys, style, sys->font);
}

SignMsgWin *func_ov036_02188f34(FieldMsgBG *sys, u16 style, Font *font) {
    SignMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(SignMsgWin), TRUE, "field_msgbg.c", 3862);

    win->sys = sys;
    win->font = font;
    win->scroll = SIGN_HIDDEN_Y;
    GFL_BGSysMoveBG(sys->bg, BG_MOVE_SET_X, 0);
    GFL_BGSysMoveBG(win->sys->bg, BG_MOVE_SET_Y, win->scroll);
    win->window = BmpWin_CreateDynamic(sys->bg, 2, 0x13, 0x1c, 4, MSGBG_PLTT_SIGN, 1);
    win->nextPage = GFL_BitmapCreate(0x1c, 4, BGSYS_TILE_SIZE_16, sys->heapId);
    win->page = GFL_BitmapCreate(0x1c, 4, BGSYS_TILE_SIZE_16, sys->heapId);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
    BmpWin_FlushMap(win->window);
    BmpWin_FlushChar(win->window);
    func_ov036_021892b0(sys->bg, MSGBG_PLTT_SIGN, style, sys->heapId);
    func_ov036_021892f0(sys->bg, BmpWin_GetPosX(win->window), BmpWin_GetPosY(win->window), BmpWin_GetSizeX(win->window),
                        BmpWin_GetSizeY(win->window), MSGBG_CHAR_SIGN_FRAME, MSGBG_PLTT_SIGN);
    GFL_BGSysLoadScr(sys->bg);
    MsgBGCursor_Init(&win->cursor, win->sys);
    sys->palette = MSGBG_PLTT_TALK;
    setAlphaBlend(FALSE);
    return win;
}

void func_ov036_0218903c(SignMsgWin *win) {
    BmpWin_GetBitmap(win->window);
    func_ov036_021894c8(win->sys->bg, BmpWin_GetPosX(win->window), BmpWin_GetPosY(win->window),
                        BmpWin_GetSizeX(win->window), BmpWin_GetSizeY(win->window), 0, 0xf);
    BmpWin_ClearScreen(win->window);
    GFL_BGSysFillScrArea(win->sys->bg, 0, BmpWin_GetPosX(win->window), BmpWin_GetPosY(win->window),
                         BmpWin_GetSizeX(win->window), BmpWin_GetSizeY(win->window), BGSYS_FILL_TILE_PALETTE);
    BmpWin_FlushChar(win->window);
    BmpWin_Free(win->window);
    GFL_BGSysLoadScr(win->sys->bg);
    GFL_BGSysMoveBG(win->sys->bg, BG_MOVE_SET_Y, 0);
    GFL_BitmapFree(win->page);
    GFL_BitmapFree(win->nextPage);
    if (win->line != NULL) {
        GFL_StrBufFree(win->line);
    }
    MsgBGCursor_Free(&win->cursor);
    GFL_HeapFree(win);
}

BOOL func_ov036_02189110(SignMsgWin *win, StrBuf *text) {
    switch (win->state) {
    case SIGN_STATE_PRINT:
        win->src = text;
        win->line = GFL_StrBufCreate(GFL_StrBufGetCharCount(text) + 2, win->sys->heapId);
        func_ov036_02189674(BmpWin_GetBitmap(win->window), win->font, win->src, win->line, &win->lineNo,
                            MSGBG_COLOR_BG);
        BmpWin_FlushChar(win->window);
        GFL_BGSysLoadScr(win->sys->bg);
        win->state++;
        break;
    case SIGN_STATE_SCROLL_IN:
        win->scroll += SIGN_SCROLL_SPEED;
        if (win->scroll >= 0) {
            win->scroll = 0;
            win->state++;
        }
        GFL_BGSysMoveBG(win->sys->bg, BG_MOVE_SET_Y, win->scroll);
        break;
    case SIGN_STATE_CHECK_PAGE:
        if (func_ov036_02189660(win->src, win->line, win->lineNo) == TRUE) {
            win->state = SIGN_STATE_WAIT_END;
            break;
        }
        win->state++;
        // fallthrough
    case SIGN_STATE_WAIT_PAGE:
        if (!(GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            func_ov036_02189de8(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
            break;
        }
        MsgBGCursor_Clear(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
        GFL_BitmapCopy(BmpWin_GetBitmap(win->window), win->page);
        func_ov036_02189674(win->nextPage, win->font, win->src, win->line, &win->lineNo, MSGBG_COLOR_BG);
        win->pageScroll = 0;
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        win->state++;
        // fallthrough
    case SIGN_STATE_SCROLL_PAGE:
        if (func_ov036_021896f0(BmpWin_GetBitmap(win->window), win->page, win->nextPage, win->pageScroll,
                                MSGBG_COLOR_BG) == TRUE) {
            win->state = SIGN_STATE_CHECK_PAGE;
        } else {
            win->pageScroll += SIGN_PAGE_SCROLL_SPEED;
        }
        BmpWin_FlushChar(win->window);
        GFL_BGSysLoadScr(win->sys->bg);
        break;
    case SIGN_STATE_WAIT_END:
        if (!(GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_PLUS_KEY_MASK))) {
            break;
        }
        win->state++;
        // fallthrough
    case SIGN_STATE_SCROLL_OUT:
        win->scroll -= SIGN_SCROLL_SPEED;
        if (win->scroll <= SIGN_HIDDEN_Y) {
            win->scroll = SIGN_HIDDEN_Y;
        }
        GFL_BGSysMoveBG(win->sys->bg, BG_MOVE_SET_Y, win->scroll);
        if (win->scroll == SIGN_HIDDEN_Y) {
            win->state++;
            return TRUE;
        }
        break;
    case SIGN_STATE_CLOSED:
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_021892b0(u32 bg, u32 palSlot, u32 style, HeapID heapId) {
    if (style >= SIGN_STYLE_COUNT) {
        style = 0;
    }
    GFL_BGSysLoadNCGRStatic(ARCID_WINFRAME, 6, bg, MSGBG_CHAR_SIGN_FRAME, 0, FALSE, heapId);
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, 1, PALTYPE_MAIN_BG, style * MSGBG_PLTT_SIZE, palSlot * MSGBG_PLTT_SIZE,
                      MSGBG_PLTT_SIZE, heapId);
}

static void func_ov036_021892f0(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal) {
    GFL_BGSysFillScrArea(bg, tile, x - 2, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 1, x - 1, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 2, x, y - 1, w, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 4, x + w, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 5, x + w + 1, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 6, x - 2, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile + 7, x - 1, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile + 10, x + w, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile + 11, x + w + 1, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile + 12, x - 2, y + h, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 13, x - 1, y + h, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 14, x, y + h, w, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 16, x + w, y + h, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile + 17, x + w + 1, y + h, 1, 1, pal);
}

static void func_ov036_021894c8(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal) {
    GFL_BGSysFillScrArea(bg, tile, x - 2, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x - 1, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x, y - 1, w, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x + w, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x + w + 1, y - 1, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x - 2, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile, x - 1, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile, x + w, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile, x + w + 1, y, 1, h, pal);
    GFL_BGSysFillScrArea(bg, tile, x - 2, y + h, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x - 1, y + h, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x, y + h, w, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x + w, y + h, 1, 1, pal);
    GFL_BGSysFillScrArea(bg, tile, x + w + 1, y + h, 1, 1, pal);
}

static BOOL func_ov036_02189660(const StrBuf *src, StrBuf *dest, u32 line) {
    if (GFL_FontCopyLine(src, dest, line) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_02189674(GFLBitmap *bitmap, Font *font, const StrBuf *src, StrBuf *dest, u32 *line, u8 color) {
    s32 y = 1;

    GFL_BitmapFill(bitmap, color);
    if (GFL_FontCopyLine(src, dest, *line) == FALSE) {
        return TRUE;
    }
    GFL_TextRendererDrawToBitmap(bitmap, 1, y, dest, font);
    y += GFL_FontGetBlockHeight(dest, font) + 1;
    if (GFL_FontCopyLine(src, dest, ++*line) == FALSE) {
        return TRUE;
    }
    GFL_TextRendererDrawToBitmap(bitmap, 1, y, dest, font);
    (*line)++;
    return FALSE;
}

static BOOL func_ov036_021896f0(GFLBitmap *dest, GFLBitmap *src1, GFLBitmap *src2, int amount, u16 color) {
    int w = GFL_BitmapGetWidth(dest);
    int h = GFL_BitmapGetHeight(dest);
    int rest;
    BOOL done;

    GFL_BitmapFill(dest, color);
    if (amount > h) {
        amount = h;
    }
    rest = h - amount;
    GFL_BitmapCopyArea(src1, dest, 0, amount, 0, 0, w, rest, color);
    GFL_BitmapCopyArea(src2, dest, 0, 0, 0, rest, w, amount, color);
    done = TRUE;
    if (amount < h) {
        done = FALSE;
    }
    return done;
}

// The palette always targets main BG memory; bg is retained by the caller interface.
static void CheckerMsgWin_LoadPalette(u32 bg, u32 slot, HeapID heapId) {
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, 2, PALTYPE_MAIN_BG, 0, slot * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE, heapId);
}

// The checker frame uses fixed WINFRAME tiles; style does not select another tile set.
static void CheckerMsgWin_LoadFrameTile(GFLBitmap *bitmap, u32 style, HeapID heapId, u32 tile) {
    NNSG2dCharacterData *charData;
    u8 *pixels = GFL_BitmapGetPixelData(bitmap);
    void *file = GFL_ArcSysReadHeapNewLZ(ARCID_WINFRAME, 7, FALSE, heapId);

    NNS_G2dGetUnpackedBGCharacterData(file, &charData);
    MI_CpuCopy((u8 *)charData->rawData + tile * BGSYS_TILE_SIZE_16, pixels, BGSYS_TILE_SIZE_16);
    GFL_HeapFree(file);
}

static void CheckerMsgWin_DrawFrame(GFLBitmap *bitmap, u32 style, HeapID heapId) {
    u16 x;
    u16 y;
    u16 tilesW = GFL_BitmapGetWidth(bitmap) >> 3;
    u16 tilesH = GFL_BitmapGetHeight(bitmap) >> 3;
    GFLBitmap *tile = GFL_BitmapCreate(1, 1, BGSYS_TILE_SIZE_16, heapId);

    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 9);
    for (y = 0; y < tilesH; y++) {
        for (x = 0; x < tilesW; x++) {
            GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
        }
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 1);
    for (x = 0; x < tilesW; x++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, 0, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 0x11);
    y = tilesH - 1;
    for (x = 0; x < tilesW; x++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 8);
    for (y = 0; y < tilesH; y++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, 0, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 0xa);
    x = tilesW - 1;
    for (y = 0; y < tilesH; y++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    x = 0;
    y = 0;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 0);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    x = tilesW - 1;
    y = 0;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 2);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    x = 0;
    y = tilesH - 1;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 0x10);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    x = tilesW - 1;
    y = tilesH - 1;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, 0x12);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    GFL_BitmapFree(tile);
}

static void FieldMsgBG_DrawTextLines(u16 x, u16 y, GFLBitmap *bitmap, Font *font, const StrBuf *text, HeapID heapId) {
    int lineHeight = GFL_FontGetCharHeight(font) + 2;
    StrBuf *line = GFL_StrBufCreate(GFL_StrBufGetCharCount(text) + 1, heapId);
    u32 i = 0;

    if (GFL_FontCopyLine(text, line, 0) == TRUE) {
        do {
            GFL_TextRendererDrawToBitmap(bitmap, x, y, line, font);
            y += lineHeight;
            i++;
        } while (GFL_FontCopyLine(text, line, i) == TRUE);
    }
    GFL_StrBufFree(line);
}

CheckerMsgWin *CheckerMsgWin_Create(FieldMsgBG *sys, u16 style, u16 x, u16 y, u16 width, u16 height) {
    CheckerMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(CheckerMsgWin), TRUE, "field_msgbg.c", 4568);

    win->sys = sys;
    win->window = BmpWin_CreateDynamic(sys->bg, x, y, width, height, MSGBG_PLTT_SIGN, 1);
    win->cursorBack = GFL_BitmapCreate(1, 1, BGSYS_TILE_SIZE_16, sys->heapId);
    CheckerMsgWin_LoadFrameTile(win->cursorBack, style, sys->heapId, 9);
    CheckerMsgWin_DrawFrame(BmpWin_GetBitmap(win->window), style, sys->heapId);
    CheckerMsgWin_LoadPalette(sys->bg, MSGBG_PLTT_SIGN, sys->heapId);
    BmpWin_FlushMap(win->window);
    BmpWin_FlushChar(win->window);
    GFL_BGSysLoadScr(sys->bg);
    MsgBGCursor_Init(&win->cursor, sys);
    sys->palette = MSGBG_PLTT_TALK;
    setAlphaBlend(FALSE);
    return win;
}

void CheckerMsgWin_Free(CheckerMsgWin *win) {
    BmpWin_ClearScreen(win->window);
    GFL_BGSysFillScrArea(win->sys->bg, 0, BmpWin_GetPosX(win->window), BmpWin_GetPosY(win->window),
                         BmpWin_GetSizeX(win->window), BmpWin_GetSizeY(win->window), BGSYS_FILL_TILE_PALETTE);
    BmpWin_FlushChar(win->window);
    GFL_BGSysLoadScr(win->sys->bg);
    MsgBGCursor_Free(&win->cursor);
    GFL_BitmapFree(win->cursorBack);
    BmpWin_Free(win->window);
    GFL_HeapFree(win);
}

void CheckerMsgWin_PrintStrBuf(CheckerMsgWin *win, u16 x, u16 y, StrBuf *text) {
    FieldMsgBG *sys = win->sys;

    FieldMsgBG_DrawTextLines(x, y, BmpWin_GetBitmap(win->window), sys->font, text, sys->heapId);
    BmpWin_FlushChar(win->window);
    GFL_BGSysLoadScr(win->sys->bg);
}

BOOL CheckerMsgWin_Update(CheckerMsgWin *win) {
    u32 pressed = GCTX_HIDGetPressedKeys();

    GCTX_HIDGetHeldKeys();
    if (pressed & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        return TRUE;
    }
    func_ov036_02189e20(&win->cursor, BmpWin_GetBitmap(win->window), win->cursorBack);
    BmpWin_FlushChar(win->window);
    return FALSE;
}

u32 CheckerMsgWin_CalcWidth(FieldMsgBG *sys, StrBuf *str, u32 margin) {
    u32 w = GFL_FontGetBlockWidth(str, sys->font, 0) + margin * 2;
    u32 tiles = w / 8;

    if (w % 8) {
        tiles++;
    }
    return tiles;
}

u32 CheckerMsgWin_CalcHeight(FieldMsgBG *sys, StrBuf *str, u32 margin) {
    u32 h = margin * 2 + GFL_FontGetBlockHeight(str, sys->font);
    StrBuf *clone = GFL_StrBufClone(str, sys->heapId);
    u32 i = 0;

    if (GFL_FontCopyLine(str, clone, 0) == TRUE) {
        while (++i < 0x18 && GFL_FontCopyLine(str, clone, i) == TRUE) {
        }
    }
    GFL_StrBufFree(clone);
    // The lines after the first are 2 pixels apart
    h += (i - 1) * 2;
    if (h % 8) {
        h += 8;
    }
    return h / 8;
}

MsgBGCursor *func_ov036_02189cb0(FieldMsgBG *sys) {
    MsgBGCursor *cursor = GFL_HeapAllocate(sys->heapId, sizeof(MsgBGCursor), TRUE, "field_msgbg.c", 4733);

    MsgBGCursor_Init(cursor, sys);
    return cursor;
}

void func_ov036_02189cd8(MsgBGCursor *cursor) {
    MsgBGCursor_Free(cursor);
    GFL_HeapFree(cursor);
}

static void MsgBGCursor_Init(MsgBGCursor *cursor, FieldMsgBG *sys) {
    MI_CpuFill8(cursor, 0, sizeof(MsgBGCursor));
    cursor->bitmap = GFL_BitmapWrap(sys->charData->rawData, 1, 1, BGSYS_TILE_SIZE_16, sys->heapId);
}

static void MsgBGCursor_Free(MsgBGCursor *cursor) {
    cursor->shown = FALSE;
    GFL_BitmapFree(cursor->bitmap);
}

static BOOL MsgBGCursor_IsShown(MsgBGCursor *cursor) {
    return cursor->shown;
}

static void MsgBGCursor_Clear(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color) {
    u16 offsets[3] = { 0, 1, 2 };
    u16 x = GFL_BitmapGetWidth(bitmap) - 8;
    u16 y = GFL_BitmapGetHeight(bitmap) - 8 - offsets[cursor->frame];

    GFL_BitmapFillArea(bitmap, x, y, 8, 8, color);
    cursor->shown = FALSE;
}

static void MsgBGCursor_Draw(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color) {
    u16 offsets[3] = { 0, 1, 2 };
    u16 x = GFL_BitmapGetWidth(bitmap) - 8;
    u16 y = GFL_BitmapGetHeight(bitmap) - 8 - offsets[cursor->frame];

    GFL_BitmapCopyArea(cursor->bitmap, bitmap, 0, 0, x, y, 8, 8, 0);
    cursor->shown = TRUE;
}

void func_ov036_02189de8(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color) {
    MsgBGCursor_Clear(cursor, bitmap, color);
    cursor->timer++;
    if (cursor->timer >= 4) {
        cursor->timer = 0;
        cursor->frame++;
        cursor->frame = cursor->frame % 3;
    }
    MsgBGCursor_Draw(cursor, bitmap, color);
}

static void func_ov036_02189e20(MsgBGCursor *cursor, GFLBitmap *bitmap, GFLBitmap *dest) {
    u16 offsets[3] = { 0, 1, 2 };
    s16 x = GFL_BitmapGetWidth(bitmap) - 8;
    s16 y = GFL_BitmapGetHeight(bitmap) - 8 - offsets[cursor->frame];
    s16 rem = y & 7;

    if (rem != 0) {
        GFL_BitmapCopyArea(dest, bitmap, 0, 8 - rem, x, y, 8, rem, GFL_BITMAP_NO_COLOR_KEY);
    }
    GFL_BitmapCopyArea(dest, bitmap, 0, 0, x, y + rem, 8, 8 - rem, GFL_BITMAP_NO_COLOR_KEY);
    cursor->timer++;
    if (cursor->timer >= 4) {
        cursor->timer = 0;
        cursor->frame++;
        cursor->frame = cursor->frame % 3;
    }
    x = GFL_BitmapGetWidth(bitmap) - 8;
    y = GFL_BitmapGetHeight(bitmap) - 8 - offsets[cursor->frame];
    GFL_BitmapCopyArea(cursor->bitmap, bitmap, 0, 0, x, y, 8, 8, 0);
    cursor->shown = TRUE;
}

static void func_ov036_02189f10(HeapID heapId) {
    GFL_BGSysLoadNCGRStatic(ARCID_WINFRAME, 9, MSGBG_BG, MSGBG_CHAR_LIST_FRAME, 0, FALSE, heapId);
}

static void func_ov036_02189f2c(BmpWin *win) {
    u8 x = BmpWin_GetPosX(win);
    u8 y = BmpWin_GetPosY(win);
    u8 w = BmpWin_GetSizeX(win);
    u8 h = BmpWin_GetSizeY(win);
    u8 bg = BmpWin_GetBGIndex(win);

    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME, x - 1, y - 1, 1, 1, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 1, x, y - 1, w, 1, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 5, x + w, y - 1, 1, 1, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 6, x - 1, y, 1, h, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 0xb, x + w, y, 1, h, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 0xc, x - 1, y + h, 1, 1, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 0xe, x, y + h, w, 1, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysFillScrArea(bg, MSGBG_CHAR_LIST_FRAME + 0x11, x + w, y + h, 1, 1, MSGBG_PLTT_BG2_FRAME);
    GFL_BGSysLoadScr(bg);
}

static BmpWin *func_ov036_0218a048(u8 x, u8 y, u8 w, u8 h, HeapID heapId) {
    BmpWin *win = BmpWin_CreateDynamic(MSGBG_BG2, x, y, w, h, MSGBG_PLTT_BG2, 1);

    GFL_BitmapFill(BmpWin_GetBitmap(win), MSGBG_COLOR_BG_FILL);
    BmpWin_FlushMap(win);
    BmpWin_FlushChar(win);
    func_ov036_02189f2c(win);
    return win;
}

static void func_ov036_0218a08c(BmpWin *win) {
    u8 bg = BmpWin_GetBGIndex(win);

    GFL_BGSysFillScrArea(bg, 0, BmpWin_GetPosX(win) - 1, BmpWin_GetPosY(win) - 1, BmpWin_GetSizeX(win) + 2,
                         BmpWin_GetSizeY(win) + 2, 0);
    GFL_BGSysLoadScr(bg);
}

static void func_ov036_0218a0e8(BmpWin *win) {
    func_ov036_0218a08c(win);
    BmpWin_Free(win);
}

static void func_ov036_0218a0f8(FieldMsgBG *sys, BOOL loadFrame) {
    sys->bg = MSGBG_BG;
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE_0x0000,
                          GX_BG_CHARBASE_0x10000,
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(sys->bg, &setup, BGMODE_TEXT);
    }
    sys->bg2 = MSGBG_BG2;
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE_0xf800,
                          GX_BG_CHARBASE_0x10000,
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(sys->bg2, &setup, BGMODE_TEXT);
    }
    GFL_BGSysFillChar(sys->bg, 0, 1, 0);
    GFL_BGSysFillScrArea(sys->bg, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(sys->bg2, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysLoadScr(sys->bg);
    GFL_BGSysLoadScr(sys->bg2);
    if (loadFrame) {
        func_ov036_0218a4dc(sys->bg, sys->heapId);
    }
    if (sys->camera != NULL) {
        TalkMsgWinSetup init;

        init.heapId = sys->heapId;
        init.camera = sys->camera;
        init.font = sys->font;
        init.frameChar = MSGBG_CHAR_TALK_FRAME;
        init.bg = MSGBG_BG;
        init.framePalette = MSGBG_PLTT_TALK_FRAME;
        init.winPalette = MSGBG_PLTT_TALK;
        sys->talk = TalkMsgWinSys_Create(&init);
        loadTexture_wrapper(sys->talk);
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, MSGBG_BLEND_PLANES_UNDER, 16, 4);
    GFL_BGSysSetBGEnabled(sys->bg, 1);
    GFL_BGSysSetBGEnabled(sys->bg2, 1);
    sys->palette = MSGBG_PLTT_BG2;
}

static void func_ov036_0218a218(FieldMsgBG *sys, BOOL loadFrame) {
    sys->bg = MSGBG_BG;
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE_0x0000,
                          GX_BG_CHARBASE_0x10000,
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(sys->bg, &setup, BGMODE_TEXT);
    }
    sys->bg2 = MSGBG_BG2;
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE_0xf800,
                          GX_BG_CHARBASE_0x10000,
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(sys->bg2, &setup, BGMODE_TEXT);
    }
    GFL_BGSysFillChar(sys->bg, 0, 1, 0);
    GFL_BGSysFillScrArea(sys->bg, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(sys->bg2, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysLoadScr(sys->bg);
    GFL_BGSysLoadScr(sys->bg2);
    if (loadFrame) {
        func_ov036_0218a4dc(sys->bg, sys->heapId);
    }
    if (sys->camera != NULL) {
        TalkMsgWinSetup init;

        init.heapId = sys->heapId;
        init.camera = sys->camera;
        init.font = sys->font;
        init.frameChar = MSGBG_CHAR_TALK_FRAME;
        init.bg = MSGBG_BG;
        init.framePalette = MSGBG_PLTT_TALK_FRAME;
        init.winPalette = MSGBG_PLTT_TALK;
        sys->talk = TalkMsgWinSys_Create(&init);
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, MSGBG_BLEND_PLANES_UNDER, 16, 4);
    GFL_BGSysSetBGEnabled(sys->bg, 1);
    GFL_BGSysSetBGEnabled(sys->bg2, 1);
    sys->palette = MSGBG_PLTT_BG2;
}

static void func_ov036_0218a334(FieldMsgBG *sys) {
    sys->bg = MSGBG_BG;
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE_0x0000,
                          GX_BG_CHARBASE_0x10000,
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(sys->bg, &setup, BGMODE_TEXT);
    }
    GFL_BGSysFillChar(sys->bg, 0, 1, 0);
    GFL_BGSysFillScrArea(sys->bg, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysLoadScr(sys->bg);
    func_ov036_0218a4dc(sys->bg, sys->heapId);
    if (sys->camera != NULL) {
        TalkMsgWinSetup init;

        init.heapId = sys->heapId;
        init.camera = sys->camera;
        init.font = sys->font;
        init.frameChar = MSGBG_CHAR_TALK_FRAME;
        init.bg = MSGBG_BG;
        init.framePalette = MSGBG_PLTT_TALK_FRAME;
        init.winPalette = MSGBG_PLTT_TALK;
        sys->talk = TalkMsgWinSys_Create(&init);
    }
    GFL_BGSysSetBGEnabled(sys->bg, 1);
    sys->palette = MSGBG_PLTT_BG2;
}

static void func_ov036_0218a3e0(TCB *tcb, void *data) {
    FieldMsgBG *sys = data;

    if (!GFL_IRQCartDataTransferIsEnabled()) {
        loadTexture_wrapper(sys->talk);
        sys->vblankTcb = NULL;
        GFL_TCBRemove(tcb);
    }
}

static void func_ov036_0218a408(u32 create, u32 alphaBlend) {
    GFL_BGSysSetBGEnabled(MSGBG_BG2, 0);
    if (create == TRUE) {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE_0xf800,
                          GX_BG_CHARBASE_0x10000,
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(MSGBG_BG2, &setup, BGMODE_TEXT);
    } else {
        G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0xf800,
                             GX_BG_CHARBASE_0x10000);
    }
    GFL_BGSysFillScrArea(MSGBG_BG2, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysLoadScr(MSGBG_BG2);
    if (alphaBlend != FALSE) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, MSGBG_BLEND_PLANES_UNDER, 16, 4);
    }
    GFL_BGSysSetBGPriority(MSGBG_BG2, 2);
    GFL_BGSysSetBGEnabled(MSGBG_BG2, 1);
}

static const ListMenuRequest sYesNoMenuRequest = {
    1, 10, 0, 13, 0, 3, 1, 0xf, 2, 0, 1, 0, 0, 12, 13, 0, 0, 0, 0, NULL
};

static void func_ov036_0218a4a4(FieldMsgBG *sys) {
    if (sys->resetBG2 != FALSE) {
        sys->resetBG2 = FALSE;
        if (sys->bg2 == MSGBG_NO_BG) {
            GFL_BGSysReleaseBG(MSGBG_BG2);
            func_ov036_0218a408(TRUE, sys->alphaBlend);
            sys->bg2 = MSGBG_BG2;
        } else {
            func_ov036_0218a408(FALSE, sys->alphaBlend);
        }
    }
}

static void func_ov036_0218a4dc(u32 bg, HeapID heapId) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, MSGBG_PLTT_TALK * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE,
                             heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, PALTYPE_MAIN_BG, MSGBG_PLTT_BG2 * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE, heapId);
    LoadSysMsgBox(bg, MSGBG_CHAR_FRAME, MSGBG_PLTT_BG2_FRAME, 0, heapId);
    func_ov036_02189f10(heapId);
}

static void func_ov036_0218a524(u32 bg, HeapID heapId, u32 mode) {
    if (mode == 0) {
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, MSGBG_PLTT_TALK * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE,
                                 heapId);
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, PALTYPE_MAIN_BG, MSGBG_PLTT_BG2 * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE,
                                 heapId);
    } else if (mode == 1) {
        LoadSysMsgBox(bg, MSGBG_CHAR_FRAME, MSGBG_PLTT_BG2_FRAME, 0, heapId);
    } else if (mode == 2) {
        func_ov036_02189f10(heapId);
    }
}

static void setAlphaBlend(BOOL on) {
    if (on == TRUE) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, MSGBG_BLEND_PLANES_UNDER, 16, 4);
    }
}

void setAlphaBlend_wrapper(BOOL on) {
    setAlphaBlend(on);
}

void func_ov036_0218a5a0(FieldMsgBG *sys) {
    GFL_BGSysFreeFilledChar(sys->bg, 1, 0);
    GFL_BGSysFillChar(sys->bg, 0, 1, 0);
    GFL_BGSysFillScrArea(sys->bg, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysLoadScr(sys->bg);
    func_ov036_0218a4dc(sys->bg, sys->heapId);
    func_ov036_0218c110(sys->talk);
}

static BmpWin *FieldMsgBG_CreateBmpWin(u32 bg, u32 heapId, u32 x, u32 y, u16 w, u16 h, u16 pal) {
    BmpWin *win = BmpWin_CreateDynamic(bg, x, y, w, h, pal, 1);

    GFL_BitmapFill(BmpWin_GetBitmap(win), MSGBG_COLOR_BG_FILL);
    BmpWin_FlushMap(win);
    BmpWin_FlushChar(win);
    GFL_BGSysLoadScr(bg);
    BmpWin_DrawFrame(win, WINFRAME_TRANSFER_NOW, MSGBG_CHAR_FRAME,
                     pal == MSGBG_PLTT_BG2 ? MSGBG_PLTT_BG2_FRAME : MSGBG_PLTT_FRAME);
    return win;
}

static void func_ov036_0218a668(BmpWin *win) {
    BmpWin_ClearFrame(win, WINFRAME_TRANSFER_NOW);
    BmpWin_Free(win);
}

static void func_ov036_0218a67c(u32 heapId) {
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), PALTYPE_MAIN_BG, MSGBG_PLTT_SIZE,
                      MSGBG_PLTT_BG2_FRAME * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE, heapId);
}

static void loadPltt(u32 heapId) {
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), PALTYPE_MAIN_BG, 0,
                      MSGBG_PLTT_FRAME * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE, heapId);
}

static int FieldMsgBG_AddNumberedWindow(FieldMsgBG *sys, NumberedMsgWin *win) {
    int i;

    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++) {
        if (sys->numberedWins[i] == NULL) {
            sys->numberedWins[i] = win;
            return i;
        }
    }
    return i;
}

static NumberedMsgWin *FieldMsgBG_TakeNumberedWindow(FieldMsgBG *sys, u32 userId) {
    int i;

    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++) {
        if (sys->numberedWins[i] != NULL && userId == sys->numberedWins[i]->userId) {
            NumberedMsgWin *win = sys->numberedWins[i];

            sys->numberedWins[i] = NULL;
            return win;
        }
    }
    return NULL;
}

static void MsgBGInput_Init(MsgBGInput *input) {
    MI_CpuFill8(input, 0, sizeof(MsgBGInput));
}

static void MsgBGInput_Reset(MsgBGInput *input) {
    input->timer = 0;
    input->held = 0;
}

static void MsgBGInput_SetAutoAdvance(MsgBGInput *input, BOOL value) {
    input->autoAdvance = value;
}

static BOOL MsgBGInput_GetAutoAdvance(MsgBGInput *input) {
    return input->autoAdvance;
}

static s32 MsgBGInput_GetTextWait(MsgBGInput *input) {
    if (input->autoAdvance != 0) {
        return TextSpeed_GetWait(1);
    }
    return TextSpeed_GetCurrentWait();
}

static BOOL func_ov036_0218a760(MsgBGInput *input) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 held = GCTX_HIDGetHeldKeys();

    if (input->autoAdvance != 0) {
        return FALSE;
    }
    if (pressed & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        input->held = TRUE;
    }
    if (input->held != 0 && (held & (PAD_BUTTON_A | PAD_BUTTON_B))) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218a798(MsgBGInput *input, BOOL fast) {
    int limit;
    u32 pressed;

    pressed = GCTX_HIDGetPressedKeys();
    limit = !fast ? 8 : 0x32;
    if (input->autoAdvance != 0) {
        input->timer++;
        if (input->timer >= limit) {
            input->timer = 0;
            return TRUE;
        }
        return FALSE;
    }
    if (pressed & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0218a7d8(PrintStream *stream, u32 mode) {
    if (mode == 1) {
        PrintStream_SetFastWait(stream, TextSpeed_GetWait(2));
    } else {
        PrintStream_ClearFast(stream);
    }
}
