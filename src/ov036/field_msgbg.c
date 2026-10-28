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

// The checker window's frame characters in their WINFRAME file, eight to a row
typedef enum {
    CHECKER_FRAME_TOP_LEFT,
    CHECKER_FRAME_TOP,
    CHECKER_FRAME_TOP_RIGHT,
    CHECKER_FRAME_LEFT = 8,
    CHECKER_FRAME_CENTER,
    CHECKER_FRAME_RIGHT,
    CHECKER_FRAME_BOTTOM_LEFT = 0x10,
    CHECKER_FRAME_BOTTOM,
    CHECKER_FRAME_BOTTOM_RIGHT,
} CheckerFrameTile;

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
static ListMenuUI *MsgBGPrinter_CreateMenuFuncPos(FieldMsgBG *sys, const ListMenuRequest *request,
                                                  ListMenuOption *options, u16 listTop, u16 cursorRow);
static u32 FieldMsgBG_GetWidestOption(FieldMsgBG *sys, ListMenuOption *options, u32 *widestIndex, u32 spacing);
static void BmpMenuListSetup_Init(BmpMenuListHeader *header, const ListMenuRequest *request);

// The yes/no menu's request, which MsgBGPrinter_CreateYesNoMenu copies and sets the menu's position and size in. It is
// defined after the BG setup functions, where the size sort lays it out after their setups, as the ROM has it
static const ListMenuRequest sYesNoMenuRequest;

static MsgStream *MsgBGPrinter_CreateMsgStreamColor(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y,
                                                    u32 wait, u16 backgroundColor);
static BOOL MsgBGPrinter_MsgStreamCallback(u32 event);
static MsgStream *MsgBGPrinter_CreateMsgStream(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y, u32 wait);
static void MsgStream_Free(MsgStream *stream);
static u32 MsgStream_Update(MsgStream *stream);
static FieldMsgWin *MsgBGPrinter_CreateSysWin(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height);
static void ActorMsgWin_Create(FieldMsgBG *sys, ActorMsgWin *win, u32 type, const VecFx32 *pos, StrBuf *strbuf,
                               u32 frameType, u32 tail);
static void MsgBGPrinter_SetupPlainWin(FieldMsgBG *sys, InfoMsgWin *win, u32 x, u32 y, u16 width, u16 height,
                                       u32 frameType);
static void MsgBGPrinter_SetupSubMsgWin(FieldMsgBG *sys, NumberedMsgWin *win, StrBuf *strbuf, u16 index, u8 x, u8 y,
                                        u8 width, u8 height, u32 userId, u32 frameType);
static void MsgBGPrinter_FreeSubMsgWin(NumberedMsgWin *win);
static BOOL MsgBGPrinter_SubMsgWinMain(NumberedMsgWin *win);
static void MsgBGPrinter_LoadBgWinGraphics(u32 bg, u32 palSlot, u32 style, HeapID heapId);
static void MsgBGPrinter_DrawBgWinFrame(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal);
static void MsgBGPrinter_ClearBgWinFrame(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal);
static BOOL MsgBGPrinter_IsLineMissing(const StrBuf *src, StrBuf *dest, u32 line);
static BOOL MsgBGPrinter_PrintTwoLines(GFLBitmap *bitmap, Font *font, const StrBuf *src, StrBuf *dest, u32 *line,
                                       u8 color);
static BOOL MsgBGPrinter_RollLines(GFLBitmap *dest, GFLBitmap *src1, GFLBitmap *src2, int amount, u16 color);
static void CheckerMsgWin_LoadPalette(u32 bg, u32 slot, HeapID heapId);
static void CheckerMsgWin_LoadFrameTile(GFLBitmap *bitmap, u32 style, HeapID heapId, CheckerFrameTile tile);
static void CheckerMsgWin_DrawFrame(GFLBitmap *bitmap, u32 style, HeapID heapId);
static void FieldMsgBG_DrawTextLines(u16 x, u16 y, GFLBitmap *bitmap, Font *font, const StrBuf *text, HeapID heapId);
static void MsgBGCursor_Init(MsgBGCursor *cursor, FieldMsgBG *sys);
static void MsgBGCursor_Free(MsgBGCursor *cursor);
static BOOL MsgBGCursor_IsShown(MsgBGCursor *cursor);
static void MsgBGCursor_Clear(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color);
static void MsgBGCursor_Draw(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color);
static void MsgBGPrinter_AnimateCursorOnFrame(MsgBGCursor *cursor, GFLBitmap *bitmap, GFLBitmap *dest);
static void MsgBGPrinter_LoadSysWindowFrameGraphics(HeapID heapId);
static void MsgBGPrinter_DrawSysWindowFrame(BmpWin *win);
static BmpWin *MsgBGPrinter_CreateSysFrameWindow(u8 x, u8 y, u8 w, u8 h, HeapID heapId);
static void MsgBGPrinter_ClearSysWindowFrame(BmpWin *win);
static void MsgBGPrinter_DeleteSysFrameWindow(BmpWin *win);
static void MsgBGPrinter_InitBgTwoLayerTalk(FieldMsgBG *sys, BOOL loadFrame);
static void MsgBGPrinter_InitBgTwoLayer(FieldMsgBG *sys, BOOL loadFrame);
static void MsgBGPrinter_InitBgOneLayer(FieldMsgBG *sys);
static void MsgBGPrinter_VblankTalkSysTask(TCB *tcb, void *data);
static void MsgBGPrinter_SetupSysFrameBg(u32 create, u32 alphaBlend);
static void MsgBGPrinter_SetupSysFrame(FieldMsgBG *sys);
static void MsgBGPrinter_LoadWindowGraphicsAll(u32 bg, HeapID heapId);
static void MsgBGPrinter_LoadWindowGraphicsStepFn(u32 bg, HeapID heapId, u32 step);
static void setAlphaBlend(BOOL on);
static BmpWin *FieldMsgBG_CreateBmpWin(u32 bg, u32 heapId, u32 x, u32 y, u16 w, u16 h, u16 pal);
static void MsgBGPrinter_DeleteWindow(BmpWin *win);
static void MsgBGPrinter_LoadMenuGraphicsLower(u32 heapId);
static void loadPltt(u32 heapId);
static int FieldMsgBG_AddNumberedWindow(FieldMsgBG *sys, NumberedMsgWin *win);
static NumberedMsgWin *FieldMsgBG_TakeNumberedWindow(FieldMsgBG *sys, u32 userId);
static void MsgBGInput_Init(MsgBGInput *input);
static void MsgBGInput_Reset(MsgBGInput *input);
static void MsgBGInput_SetAutoAdvance(MsgBGInput *input, BOOL value);
static BOOL MsgBGInput_GetAutoAdvance(MsgBGInput *input);
static s32 MsgBGInput_GetTextWait(MsgBGInput *input);
static BOOL MsgBGPrinter_KeyWaitCheckSkip(MsgBGInput *input);
static BOOL MsgBGPrinter_KeyWaitCheckAdvance(MsgBGInput *input, BOOL fast);
static void MsgBGPrinter_StreamSetSpeed(PrintStream *stream, BOOL fast);

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

void MsgBGPrinter_SetupBg(FieldMsgBG *sys) {
    MsgBGPrinter_InitBgTwoLayerTalk(sys, TRUE);
}

void MsgBGPrinter_SetupBgDelayed(FieldMsgBG *sys) {
    MsgBGPrinter_InitBgTwoLayer(sys, FALSE);
    if (sys->vblankTcb == NULL) {
        sys->vblankTcb = GFL_VBlankTCBAdd(MsgBGPrinter_VblankTalkSysTask, sys, 0);
    }
}

void MsgBGPrinter_LoadWindowGraphics(u32 bg, HeapID heapId) {
    MsgBGPrinter_LoadWindowGraphicsAll(bg, heapId);
}

void MsgBGPrinter_LoadWindowGraphicsStep(FieldMsgBG *sys, u32 step) {
    MsgBGPrinter_LoadWindowGraphicsStepFn(sys->bg, sys->heapId, step);
}

void MsgBGPrinter_Delete(FieldMsgBG *sys) {
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

void MsgBGPrinter_ReleaseBg(FieldMsgBG *sys) {
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

BOOL MsgBGPrinter_ReleaseSysFrame(FieldMsgBG *sys) {
    if (sys->bg2 != MSGBG_NO_BG) {
        GFL_BGSysReleaseBG(sys->bg2);
        sys->bg2 = MSGBG_NO_BG;
        return TRUE;
    }
    return FALSE;
}

void MsgBGPrinter_SetupBgSingle(FieldMsgBG *sys) {
    if (sys->talk != NULL) {
        TalkMsgWinSys_Free(sys->talk);
    }
    MsgBGPrinter_InitBgOneLayer(sys);
    if (sys->vblankTcb == NULL) {
        sys->vblankTcb = GFL_VBlankTCBAdd(MsgBGPrinter_VblankTalkSysTask, sys, 0);
    }
}

BOOL MsgBGPrinter_IsSetupDone(FieldMsgBG *sys) {
    if (sys->vblankTcb == NULL) {
        return TRUE;
    }
    return FALSE;
}

void MsgBGPrinter_Main(FieldMsgBG *sys) {
    MsgBGPrinter *printer;
    int i;
    NumberedMsgWin **numberedWin;

    PrintQueue_Main(sys->queue);
    numberedWin = sys->numberedWins;
    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++, numberedWin++) {
        if (*numberedWin != NULL) {
            MsgBGPrinter_SubMsgWinMain(*numberedWin);
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
        TalkMsgWin_EndUpdate(sys->talk);
    }
}

void MsgBGPrinter_DrawTalkSys(FieldMsgBG *sys) {
    if (sys->talk != NULL) {
        TalkMsgWinSys_Draw(sys->talk);
    }
}

void MsgBGPrinter_ClearPrintQue(FieldMsgBG *sys) {
    PrintQueue_Clear(sys->queue);
}

BOOL MsgBGPrinter_IsPrintQueFinished(FieldMsgBG *sys) {
    return PrintQueue_IsEmpty(sys->queue);
}

PrintQueue *MsgBGPrinter_GetPrintQue(FieldMsgBG *sys) {
    return sys->queue;
}

Font *MsgBGPrinter_GetFont(FieldMsgBG *sys) {
    return sys->font;
}

MsgData *MsgBGPrinter_CreateMsgData(FieldMsgBG *sys, u32 fileId) {
    return GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, fileId, sys->heapId);
}

void MsgBGPrinter_DeleteMsgData(MsgData *msgData) {
    GFL_MsgDataFree(msgData);
}

void MsgBGPrinter_RequestSysFrame(FieldMsgBG *sys) {
    sys->resetBG2 = TRUE;
}

void MsgBGPrinter_SetAutoMode(FieldMsgBG *sys, BOOL enable) {
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

    MsgBGPrinter_SetupSysFrame(sys);
    if (palette == MSGBG_PLTT_BG2) {
        bg = sys->bg2;
    }
    win = GFL_HeapAllocate(sys->heapId, sizeof(MoneyWin), TRUE, "field_msgbg.c", 1158);
    win->sys = sys;
    win->window = FieldMsgBG_CreateBmpWin(bg, sys->heapId, x, y, width, height, palette);
    win->printer = MsgBGPrinter_Create(sys, msgData, win->window);
    if (palette == MSGBG_PLTT_BG2) {
        MsgBGPrinter_LoadMenuGraphicsLower(sys->heapId);
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

void MsgBGPrinter_DeleteMsgWin(MoneyWin *win) {
    MsgBGPrinter_DeleteWindow(win->window);
    MsgBGPrinter_Free(win->printer);
    GFL_HeapFree(win);
}

void MsgBGPrinter_MsgWinPrintMsg(MoneyWin *win, u16 x, u16 y, u32 messageId) {
    MsgBGPrinter_PrintMessage(win->printer, x, y, messageId);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win->window));
}

void MsgBGPrinter_MsgWinPrintStr(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf) {
    MsgBGPrinter_PrintStrBuf(win->printer, x, y, strbuf);
}

void MsgBGPrinter_MsgWinPrintStrColorFont(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf, u16 color, Font *font) {
    MsgBGPrinter_PrintColorFont(win->printer, x, y, strbuf, color, font);
}

BOOL MsgBGPrinter_IsMsgWinTransDone(MoneyWin *win) {
    return MsgBGPrinter_IsIdle(win->printer);
}

void MsgBGPrinter_ClearMsgWin(MoneyWin *win) {
    MsgBGPrinter_Clear(win->printer);
}

void MsgBGPrinter_FillMsgWin(MoneyWin *win, u32 x, u32 y, u32 width, u32 height) {
    MsgBGPrinter_ClearArea(win->printer, x, y, width, height);
}

BmpWin *MsgBGPrinter_GetMsgWinBmpWin(MoneyWin *win) {
    return win->window;
}

MoneyWin *MsgBGPrinter_CreateInfoMsgWin(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height) {
    MoneyWin *win;

    sys->palette = MSGBG_PLTT_BG2;
    MsgBGPrinter_SetupSysFrame(sys);
    win = GFL_HeapAllocate(sys->heapId, sizeof(MoneyWin), TRUE, "field_msgbg.c", 1379);
    win->sys = sys;
    win->window = MsgBGPrinter_CreateSysFrameWindow(x, y, width, height, sys->heapId);
    win->printer = MsgBGPrinter_Create(sys, msgData, win->window);
    setAlphaBlend(sys->alphaBlend);
    return win;
}

void MsgBGPrinter_DeleteInfoMsgWin(MoneyWin *win) {
    MsgBGPrinter_DeleteSysFrameWindow(win->window);
    MsgBGPrinter_Free(win->printer);
    GFL_HeapFree(win);
}

void MsgBGPrinter_InfoMsgWinPrintStr(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf) {
    MsgBGPrinter_PrintStrBuf(win->printer, x + 8, y, strbuf);
}

void MsgBGPrinter_ClearInfoMsgWin(MoneyWin *win) {
    MsgBGPrinter_Clear(win->printer);
}

static ListMenuUI *ListMenuUI_CreateCore(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options,
                                         u16 listTop, u16 cursorRow, u16 palette,
                                         BmpMenuListCursorCallback cursorCallback,
                                         BmpMenuListPrintCallback printCallback, void *work) {
    u8 bg = sys->bg;
    ListMenuUI *menu;
    BmpMenuListHeader header;

    MsgBGPrinter_SetupSysFrame(sys);
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
        MsgBGPrinter_LoadMenuGraphicsLower(sys->heapId);
        setAlphaBlend(sys->alphaBlend);
    } else {
        loadPltt(sys->heapId);
    }
    return menu;
}

ListMenuUI *MsgBGPrinter_CreateMenuFuncCallback(FieldMsgBG *sys, const ListMenuRequest *request,
                                                ListMenuOption *options, u16 listTop, u16 cursorRow,
                                                BmpMenuListCursorCallback cursorCallback,
                                                BmpMenuListPrintCallback printCallback, void *work) {
    sys->palette = MSGBG_PLTT_BG2;
    return ListMenuUI_CreateCore(sys, request, options, listTop, cursorRow, sys->palette, cursorCallback, printCallback,
                                 work);
}

static ListMenuUI *MsgBGPrinter_CreateMenuFuncPos(FieldMsgBG *sys, const ListMenuRequest *request,
                                                  ListMenuOption *options, u16 listTop, u16 cursorRow) {
    return MsgBGPrinter_CreateMenuFuncCallback(sys, request, options, listTop, cursorRow, NULL, request->printCallback,
                                               NULL);
}

ListMenuUI *MsgBGPrinter_CreateMenuFunc(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options) {
    return MsgBGPrinter_CreateMenuFuncPos(sys, request, options, 0, 0);
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

void MsgBGPrinter_DeleteMenuFunc(ListMenuUI *menu) {
    BmpMenuList_Free(menu->list, NULL, NULL);
    ListMenuCore_FreeOptionList(menu->options);
    MsgBGPrinter_DeleteWindow(menu->window);
    MsgBGPrinter_Free(menu->printer);
    GFL_HeapFree(menu);
}

s32 ListMenuUI_Update(ListMenuUI *menu) {
    s32 result;

    if (!MsgBGPrinter_IsPrintQueFinished(menu->sys)) {
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

void MsgBGPrinter_RewriteMenuFunc(ListMenuUI *menu) {
    BmpMenuList_Redraw(menu->list);
}

ListMenuOption *InitListMenuOptionHeap(u32 count, HeapID heapId) {
    return ListMenuCore_CreateOptionList(count, heapId);
}

ListMenuOption *MsgBGPrinter_CreateMenuItemsFromTable(const ListMenuRow *rows, s32 count, MsgData *msgData,
                                                      HeapID heapId) {
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

void MsgBGPrinter_DeleteMenuItems(ListMenuOption *options) {
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

void MsgBGPrinter_DrawMenuFuncFrame(ListMenuUI *menu) {
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

ListMenuUI *MsgBGPrinter_CreateYesNoMenu(FieldMsgBG *sys, u32 cursorRow) {
    ListMenuRequest request = sYesNoMenuRequest;
    ListMenuRow rows[2] = { { YesNo_Text_Yes, { MSGBG_YES_NO_YES } }, { YesNo_Text_No, { MSGBG_YES_NO_NO } } };
    MsgData *msgData = MsgBGPrinter_CreateMsgData(sys, TEXT_BANK_YES_NO);
    ListMenuOption *options = MsgBGPrinter_CreateMenuItemsFromTable(rows, 2, msgData, sys->heapId);
    ListMenuUI *menu;

    GFL_MsgDataFree(msgData);
    ListMenuRequest_Set(&request, 2, 24, 10, 7, 4);
    menu = ListMenuUI_CreateCore(sys, &request, options, 0, cursorRow, sys->palette, NULL, NULL, NULL);
    BmpMenuList_SetCancelDisabled(menu->list, FALSE);
    return menu;
}

u32 MsgBGPrinter_YesNoMenuMain(ListMenuUI *menu) {
    s32 result = ListMenuUI_Update(menu);

    if (result == BMPMENULIST_NULL) {
        return MSGBG_YES_NO_WAITING;
    }
    if (result == BMPMENULIST_CANCEL) {
        return MSGBG_YES_NO_NO;
    }
    return result;
}

static MsgStream *MsgBGPrinter_CreateMsgStreamColor(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y,
                                                    u32 wait, u16 backgroundColor) {
    MsgStream *stream = GFL_HeapAllocate(sys->heapId, sizeof(MsgStream), TRUE, "field_msgbg.c", 2179);

    stream->sys = sys;
    stream->wait = wait;
    stream->stream = PrintStream_CreateWithCallback(window, x, y, strbuf, sys->font, wait, sys->tcbMgr, 0, sys->heapId,
                                                    backgroundColor, MsgBGPrinter_MsgStreamCallback);
    MsgBGInput_Reset(&stream->sys->input);
    return stream;
}

static BOOL MsgBGPrinter_MsgStreamCallback(u32 event) {
    switch (event) {
    case MSGBG_EVENT_KNOCK:
        GFL_SndSEPlay(SEQ_SE_KON);
        break;
    case MSGBG_EVENT_WAIT_SOUND:
        return GFL_SndPlayerIsActiveAny();
    }
    return FALSE;
}

static MsgStream *MsgBGPrinter_CreateMsgStream(FieldMsgBG *sys, StrBuf *strbuf, BmpWin *window, u16 x, u16 y,
                                               u32 wait) {
    return MsgBGPrinter_CreateMsgStreamColor(sys, strbuf, window, x, y, wait, 15);
}

static void MsgStream_Free(MsgStream *stream) {
    PrintStream_Delete(stream->stream);
    GFL_HeapFree(stream);
}

static u32 MsgStream_Update(MsgStream *stream) {
    u32 state = PrintStream_GetState(stream->stream);

    switch (state) {
    case PRINT_STREAM_RUNNING:
        if (MsgBGPrinter_KeyWaitCheckSkip(&stream->sys->input)) {
            MsgBGPrinter_StreamSetSpeed(stream->stream, TRUE);
        } else {
            MsgBGPrinter_StreamSetSpeed(stream->stream, FALSE);
        }
        break;
    case PRINT_STREAM_PAUSED:
        if (MsgBGPrinter_KeyWaitCheckAdvance(&stream->sys->input, PrintStream_GetPageMode(stream->stream))) {
            if (!MsgBGInput_GetAutoAdvance(&stream->sys->input)) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            PrintStream_Continue(stream->stream);
            state = PRINT_STREAM_RUNNING;
            MsgBGPrinter_StreamSetSpeed(stream->stream, FALSE);
        }
        break;
    case PRINT_STREAM_DONE:
        break;
    }
    return state;
}

static FieldMsgWin *MsgBGPrinter_CreateSysWin(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height) {
    FieldMsgWin *win;

    sys->palette = MSGBG_PLTT_BG2;
    MsgBGPrinter_SetupSysFrame(sys);
    win = GFL_HeapAllocate(sys->heapId, sizeof(FieldMsgWin), TRUE, "field_msgbg.c", 2340);
    win->sys = sys;
    win->msgData = msgData;
    win->window = FieldMsgBG_CreateBmpWin(sys->bg2, sys->heapId, x, y, width, height, sys->palette);
    win->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
    MsgBGCursor_Init(&win->cursor, sys);
    MsgBGPrinter_LoadMenuGraphicsLower(sys->heapId);
    setAlphaBlend(sys->alphaBlend);
    return win;
}

void MsgBGPrinter_DeleteSysWin(FieldMsgWin *win) {
    MsgBGPrinter_DeleteWindow(win->window);
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

void MsgBGPrinter_SysWinPrintMsg(FieldMsgWin *win, u16 x, u16 y, u32 messageId) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    GFL_MsgDataLoadStrbuf(win->msgData, messageId, win->strbuf);
    win->stream = MsgBGPrinter_CreateMsgStream(win->sys, win->strbuf, win->window, x, y,
                                               MsgBGInput_GetTextWait(&win->sys->input));
}

void MsgBGPrinter_SysWinPrintStr(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    win->stream =
        MsgBGPrinter_CreateMsgStream(win->sys, strbuf, win->window, x, y, MsgBGInput_GetTextWait(&win->sys->input));
}

BOOL MsgBGPrinter_SysWinMain(FieldMsgWin *win) {
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
            MsgBGPrinter_AnimateCursor(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_DONE:
        return TRUE;
    }
    return FALSE;
}

FieldMsgWin *MsgBGPrinter_CreateBottomSysWin(FieldMsgBG *sys, MsgData *msgData) {
    return MsgBGPrinter_CreateSysWin(sys, msgData, 1, 19, 30, 4);
}

void MsgBGPrinter_ClearSysWin(FieldMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(win->sys->bg);
}

BmpWin *MsgBGPrinter_GetSysWinBmpWin(FieldMsgWin *win) {
    return win->window;
}

FieldMsgWin *MsgBGPrinter_CreateInfoSysWin(FieldMsgBG *sys, MsgData *msgData, u32 y) {
    FieldMsgWin *win;

    sys->palette = MSGBG_PLTT_BG2;
    MsgBGPrinter_SetupSysFrame(sys);
    win = GFL_HeapAllocate(sys->heapId, sizeof(FieldMsgWin), TRUE, "field_msgbg.c", 2585);
    win->sys = sys;
    win->msgData = msgData;
    win->window = MsgBGPrinter_CreateSysFrameWindow(1, y, 30, 4, sys->heapId);
    win->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
    MsgBGCursor_Init(&win->cursor, sys);
    MsgBGPrinter_LoadMenuGraphicsLower(sys->heapId);
    setAlphaBlend(sys->alphaBlend);
    return win;
}

void MsgBGPrinter_DeleteInfoSysWin(FieldMsgWin *win) {
    MsgBGPrinter_DeleteSysFrameWindow(win->window);
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

void MsgBGPrinter_InfoSysWinPrintMsg(FieldMsgWin *win, u16 x, u16 y, u32 messageId) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    GFL_MsgDataLoadStrbuf(win->msgData, messageId, win->strbuf);
    win->stream = MsgBGPrinter_CreateMsgStream(win->sys, win->strbuf, win->window, x + 8, y,
                                               MsgBGInput_GetTextWait(&win->sys->input));
}

void MsgBGPrinter_InfoSysWinPrintStr(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    win->stream =
        MsgBGPrinter_CreateMsgStream(win->sys, strbuf, win->window, x + 8, y, MsgBGInput_GetTextWait(&win->sys->input));
}

BOOL MsgBGPrinter_InfoSysWinMain(FieldMsgWin *win) {
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
            MsgBGPrinter_AnimateCursor(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
        }
        break;
    case PRINT_STREAM_DONE:
        return TRUE;
    }
    return FALSE;
}

void MsgBGPrinter_SysWinDrawCursor(FieldMsgWin *win) {
    if (!MsgBGInput_GetAutoAdvance(&win->sys->input)) {
        MsgBGPrinter_AnimateCursor(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
        BmpWin_FlushChar(win->window);
    }
}

void MsgBGPrinter_ClearInfoSysWin(FieldMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(win->sys->bg);
}

void MsgBGPrinter_InfoSysWinPrintStrNow(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, NULL, win->window);
    MsgBGPrinter_PrintStrBuf(win->printer, x + 8, y, strbuf);
}

BmpWin *MsgBGPrinter_GetInfoSysWinBmpWin(FieldMsgWin *win) {
    return win->window;
}

static void ActorMsgWin_Create(FieldMsgBG *sys, ActorMsgWin *win, u32 type, const VecFx32 *pos, StrBuf *strbuf,
                               u32 frameType, u32 tail) {
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
        ActorMsgWin_CreateAtTop(win->talk, ACTOR_MSG_WIN_TOP, pos, strbuf, 15, frameType, tail,
                                MsgBGInput_GetTextWait(&sys->input));
        break;
    case ACTOR_MSG_WIN_BOTTOM:
        ActorMsgWin_CreateAtBottom(win->talk, ACTOR_MSG_WIN_BOTTOM, pos, strbuf, 15, frameType, tail,
                                   MsgBGInput_GetTextWait(&sys->input));
        break;
    default:
        ActorMsgWin_CreateAtAutoPos(win->talk, ACTOR_MSG_WIN_AUTO, pos, strbuf, 15, frameType,
                                    MsgBGInput_GetTextWait(&sys->input));
        break;
    }
    MsgBGInput_Reset(&sys->input);
    TalkMsgWin_Open(win->talk, win->type);
    MsgBGCursor_Init(&win->cursor, sys);
    if (frameType == TALKMSG_WIN_FRAME_SPIKY) {
        win->shake = 8;
        GFL_SndSEPlay(SEQ_SE_SYS_58);
    }
}

ActorMsgWin *ActorMsgWin_CheckAndCreate(FieldMsgBG *sys, s32 type, const VecFx32 *pos, StrBuf *strbuf, u32 frameType,
                                        u32 tail) {
    if (type < ACTOR_MSG_WIN_COUNT) {
        ActorMsgWin *win = &sys->actorWins[type];

        if (win->talk == NULL) {
            ActorMsgWin_Create(sys, win, type, pos, strbuf, frameType, tail);
            return win;
        }
    }
    return NULL;
}

void MsgBGPrinter_DeleteTalkWin(ActorMsgWin *win) {
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    MsgBGCursor_Free(&win->cursor);
    TalkMsgWin_Close(win->talk, win->type);
}

BOOL MsgBGPrinter_WaitTalkWinClose(ActorMsgWin *win) {
    if (TalkMsgWin_IsWindowFree(win->talk, win->type)) {
        MI_CpuFill8(win, 0, sizeof(ActorMsgWin));
        return TRUE;
    }
    return FALSE;
}

void MsgBGPrinter_ClearTalkWin(ActorMsgWin *win) {
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    TalkMsgWin_Release(win->talk, win->type);
    MsgBGCursor_Free(&win->cursor);
    MI_CpuFill8(win, 0, sizeof(ActorMsgWin));
}

void MsgBGPrinter_TalkWinPrintStr(ActorMsgWin *actorWin, StrBuf *strbuf) {
    BmpWin *win = TalkMsgWin_GetBmpWin(actorWin->talk, actorWin->type);

    GFL_BitmapFill(BmpWin_GetBitmap(win), MSGBG_COLOR_BG_FILL);
    TalkMsgWin_Reprint(actorWin->talk, actorWin->type, strbuf, MsgBGInput_GetTextWait(&actorWin->sys->input));
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
}

BOOL MsgBGPrinter_TalkWinMain(ActorMsgWin *actorWin) {
    BmpWin *win;
    GFLBitmap *bitmap;
    PrintStream *stream;
    u32 state;

    if (!TalkMsgWin_IsWindowShown(actorWin->talk, actorWin->type)) {
        return FALSE;
    }
    stream = TalkMsgWin_GetPrintStream(actorWin->talk, actorWin->type);
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
            win = TalkMsgWin_GetBmpWin(actorWin->talk, actorWin->type);
            MsgBGCursor_Clear(&actorWin->cursor, BmpWin_GetBitmap(win), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win);
        }
        if (MsgBGPrinter_KeyWaitCheckSkip(&actorWin->sys->input)) {
            MsgBGPrinter_StreamSetSpeed(stream, TRUE);
        } else {
            MsgBGPrinter_StreamSetSpeed(stream, FALSE);
        }
        break;
    case PRINT_STREAM_PAUSED:
        win = TalkMsgWin_GetBmpWin(actorWin->talk, actorWin->type);
        bitmap = BmpWin_GetBitmap(win);
        if (MsgBGInput_GetAutoAdvance(&actorWin->sys->input) == FALSE) {
            MsgBGPrinter_AnimateCursor(&actorWin->cursor, bitmap, MSGBG_COLOR_BG);
        }
        if (MsgBGPrinter_KeyWaitCheckAdvance(&actorWin->sys->input, PrintStream_GetPageMode(stream))) {
            if (MsgBGInput_GetAutoAdvance(&actorWin->sys->input) == FALSE) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            PrintStream_Continue(stream);
            MsgBGCursor_Clear(&actorWin->cursor, bitmap, MSGBG_COLOR_BG);
            MsgBGPrinter_StreamSetSpeed(stream, FALSE);
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

void MsgBGPrinter_TalkWinDrawCursor(ActorMsgWin *actorWin) {
    BmpWin *win;

    if (MsgBGInput_GetAutoAdvance(&actorWin->sys->input) == FALSE) {
        win = TalkMsgWin_GetBmpWin(actorWin->talk, actorWin->type);
        MsgBGPrinter_AnimateCursor(&actorWin->cursor, BmpWin_GetBitmap(win), MSGBG_COLOR_BG);
        BmpWin_FlushChar(win);
    }
}

static void MsgBGPrinter_SetupPlainWin(FieldMsgBG *sys, InfoMsgWin *win, u32 x, u32 y, u16 width, u16 height,
                                       u32 frameType) {
    sys->palette = MSGBG_PLTT_TALK;
    win->talk = sys->talk;
    loadPltt(sys->heapId);
    setAlphaBlend(FALSE);
    win->window = TalkMsgWin_AddFrameWindow(win->talk, x, y, width, height, frameType);
    MsgBGCursor_Init(&win->cursor, sys);
}

InfoMsgWin *MsgBGPrinter_CreatePlainWin(FieldMsgBG *sys, MsgData *msgData, u32 frameType, u16 x, u16 y, u16 width,
                                        u16 height) {
    InfoMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(InfoMsgWin), TRUE, "field_msgbg.c", 3276);

    win->sys = sys;
    win->msgData = msgData;
    win->strbuf = GFL_StrBufCreate(MSGBG_STRBUF_LEN, sys->heapId);
    MsgBGPrinter_SetupPlainWin(sys, win, x, y, width, height, frameType);
    if (frameType == TALKMSG_WIN_FRAME_SPIKY) {
        win->shake = 8;
    }
    return win;
}

void MsgBGPrinter_DeletePlainWin(InfoMsgWin *win) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    TalkMsgWin_DeleteFrameWindow(win->talk, win->window);
    MsgBGCursor_Free(&win->cursor);
    GFL_HeapFree(win);
}

void MsgBGPrinter_ClearPlainWin(InfoMsgWin *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG_FILL);
    GFL_BGSysLoadScr(win->sys->bg);
}

void MsgBGPrinter_OpenPlainWin(InfoMsgWin *win) {
    TalkMsgWin_ClearWindowFrame(win->talk, win->window);
}

void MsgBGPrinter_PlainWinPrintMsg(InfoMsgWin *win, u16 x, u16 y, u32 messageId) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, win->msgData, win->window);
    MsgBGPrinter_PrintMessage(win->printer, x, y, messageId);
}

void MsgBGPrinter_PlainWinPrintStr(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, NULL, win->window);
    MsgBGPrinter_PrintStrBuf(win->printer, x, y, strbuf);
}

void MsgBGPrinter_PlainWinPrintStrColor(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf, u16 color) {
    if (win->printer != NULL) {
        MsgBGPrinter_Free(win->printer);
    }
    win->printer = MsgBGPrinter_Create(win->sys, NULL, win->window);
    MsgBGPrinter_PrintColor(win->printer, x, y, strbuf, color);
}

void MsgBGPrinter_PlainWinPrintStream(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf) {
    if (win->stream != NULL) {
        MsgStream_Free(win->stream);
    }
    win->stream =
        MsgBGPrinter_CreateMsgStream(win->sys, strbuf, win->window, x, y, MsgBGInput_GetTextWait(&win->sys->input));
}

BOOL MsgBGPrinter_PlainWinMain(InfoMsgWin *win) {
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
            MsgBGPrinter_AnimateCursor(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
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

void MsgBGPrinter_PlainWinDrawCursor(InfoMsgWin *win) {
    if (MsgBGInput_GetAutoAdvance(&win->sys->input) == FALSE) {
        MsgBGPrinter_AnimateCursor(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
        BmpWin_FlushChar(win->window);
    }
}

u32 MsgBGPrinter_GetPlainWinStreamState(InfoMsgWin *win) {
    return win->state;
}

static inline void NumberedMsgWin_InitTextOrigin(NumberedMsgWinTextOrigin *origin) {
    origin->x = 2;
    origin->y = 0;
}

static void MsgBGPrinter_SetupSubMsgWin(FieldMsgBG *sys, NumberedMsgWin *win, StrBuf *strbuf, u16 index, u8 x, u8 y,
                                        u8 width, u8 height, u32 userId, u32 frameType) {
    NumberedMsgWinTextOrigin origin;

    NumberedMsgWin_InitTextOrigin(&origin);
    sys->palette = MSGBG_PLTT_TALK;
    win->talk = sys->talk;
    win->index = index;
    win->userId = userId;
    loadPltt(sys->heapId);
    setAlphaBlend(FALSE);
    TalkMsgWin_SetupPlainWindow(win->talk, win->index, strbuf, x, y, width, height, 0xff, frameType,
                                MsgBGInput_GetTextWait(&sys->input), origin.x, origin.y);
    TalkMsgWin_Open(win->talk, win->index);
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

void MsgBGPrinter_AddSubMsgWinMsg(FieldMsgBG *sys, MsgData *msgData, u32 msgId, u16 userId, u8 x, u8 y, u8 width,
                                  u8 height) {
    NumberedMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(NumberedMsgWin), TRUE, "field_msgbg.c", 3662);
    int slot = FieldMsgBG_AddNumberedWindow(sys, win);

    win->strbuf = GFL_StrBufCreate(MSGBG_NUMBERED_STRBUF_LEN, sys->heapId);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    MsgBGPrinter_SetupSubMsgWin(sys, win, win->strbuf, slot + ACTOR_MSG_WIN_COUNT, x, y, width, height, userId,
                                TALKMSG_WIN_FRAME_NORMAL);
}

void MsgBGPrinter_AddSubMsgWinStr(FieldMsgBG *sys, StrBuf *strbuf, u16 userId, u8 x, u8 y, u8 width, u8 height) {
    MsgBGPrinter_AddSubMsgWinStrFrame(sys, strbuf, userId, x, y, width, height, TALKMSG_WIN_FRAME_NORMAL);
}

void MsgBGPrinter_AddSubMsgWinStrFrame(FieldMsgBG *sys, StrBuf *strbuf, u16 userId, u8 x, u8 y, u8 width, u8 height,
                                       u32 frameType) {
    NumberedMsgWin *win = GFL_HeapAllocate(sys->heapId, sizeof(NumberedMsgWin), TRUE, "field_msgbg.c", 3710);
    int slot = FieldMsgBG_AddNumberedWindow(sys, win);

    win->strbuf = GFL_StrBufCreate(GFL_StrBufGetCharCount(strbuf) + 1, sys->heapId);
    GFL_StrBufCopy(win->strbuf, strbuf);
    MsgBGPrinter_SetupSubMsgWin(sys, win, win->strbuf, slot + ACTOR_MSG_WIN_COUNT, x, y, width, height, userId,
                                frameType);
}

static void MsgBGPrinter_FreeSubMsgWin(NumberedMsgWin *win) {
    if (win->strbuf != NULL) {
        GFL_StrBufFree(win->strbuf);
    }
    TalkMsgWin_Release(win->talk, win->index);
    GFL_HeapFree(win);
}

void MsgBGPrinter_DeleteSubMsgWin(FieldMsgBG *sys, u16 userId) {
    MsgBGPrinter_FreeSubMsgWin(FieldMsgBG_TakeNumberedWindow(sys, userId));
}

void MsgBGPrinter_DeleteAllSubMsgWin(FieldMsgBG *sys) {
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
            MsgBGPrinter_FreeSubMsgWin(win);
        }
    }
}

BOOL MsgBGPrinter_ExistsSubMsgWin(FieldMsgBG *sys) {
    int i;

    for (i = 0; i < MSGBG_NUMBERED_WIN_COUNT; i++) {
        if (sys->numberedWins[i] != NULL) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL MsgBGPrinter_SubMsgWinMain(NumberedMsgWin *win) {
    if (!TalkMsgWin_IsWindowShown(win->talk, win->index)) {
        return FALSE;
    }
    GCTX_HIDGetPressedKeys();
    GCTX_HIDGetHeldKeys();
    switch (PrintStream_GetState(TalkMsgWin_GetPrintStream(win->talk, win->index))) {
    case PRINT_STREAM_RUNNING:
    case PRINT_STREAM_PAUSED:
        break;
    case PRINT_STREAM_DONE:
        return TRUE;
    }
    return FALSE;
}

SignMsgWin *MsgBGPrinter_CreateBgWin(FieldMsgBG *sys, u16 style) {
    return MsgBGPrinter_CreateBgWinFont(sys, style, sys->font);
}

SignMsgWin *MsgBGPrinter_CreateBgWinFont(FieldMsgBG *sys, u16 style, Font *font) {
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
    MsgBGPrinter_LoadBgWinGraphics(sys->bg, MSGBG_PLTT_SIGN, style, sys->heapId);
    MsgBGPrinter_DrawBgWinFrame(sys->bg, BmpWin_GetPosX(win->window), BmpWin_GetPosY(win->window),
                                BmpWin_GetSizeX(win->window), BmpWin_GetSizeY(win->window), MSGBG_CHAR_SIGN_FRAME,
                                MSGBG_PLTT_SIGN);
    GFL_BGSysLoadScr(sys->bg);
    MsgBGCursor_Init(&win->cursor, win->sys);
    sys->palette = MSGBG_PLTT_TALK;
    setAlphaBlend(FALSE);
    return win;
}

void MsgBGPrinter_DeleteBgWin(SignMsgWin *win) {
    BmpWin_GetBitmap(win->window);
    MsgBGPrinter_ClearBgWinFrame(win->sys->bg, BmpWin_GetPosX(win->window), BmpWin_GetPosY(win->window),
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

BOOL MsgBGPrinter_BgWinMain(SignMsgWin *win, StrBuf *text) {
    switch (win->state) {
    case SIGN_STATE_PRINT:
        win->src = text;
        win->line = GFL_StrBufCreate(GFL_StrBufGetCharCount(text) + 2, win->sys->heapId);
        MsgBGPrinter_PrintTwoLines(BmpWin_GetBitmap(win->window), win->font, win->src, win->line, &win->lineNo,
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
        if (MsgBGPrinter_IsLineMissing(win->src, win->line, win->lineNo) == TRUE) {
            win->state = SIGN_STATE_WAIT_END;
            break;
        }
        win->state++;
        // fallthrough
    case SIGN_STATE_WAIT_PAGE:
        if (!(GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            MsgBGPrinter_AnimateCursor(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
            BmpWin_FlushChar(win->window);
            break;
        }
        MsgBGCursor_Clear(&win->cursor, BmpWin_GetBitmap(win->window), MSGBG_COLOR_BG);
        GFL_BitmapCopy(BmpWin_GetBitmap(win->window), win->page);
        MsgBGPrinter_PrintTwoLines(win->nextPage, win->font, win->src, win->line, &win->lineNo, MSGBG_COLOR_BG);
        win->pageScroll = 0;
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        win->state++;
        // fallthrough
    case SIGN_STATE_SCROLL_PAGE:
        if (MsgBGPrinter_RollLines(BmpWin_GetBitmap(win->window), win->page, win->nextPage, win->pageScroll,
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

static void MsgBGPrinter_LoadBgWinGraphics(u32 bg, u32 palSlot, u32 style, HeapID heapId) {
    if (style >= SIGN_STYLE_COUNT) {
        style = 0;
    }
    GFL_BGSysLoadNCGRStatic(ARCID_WINFRAME, 6, bg, MSGBG_CHAR_SIGN_FRAME, 0, FALSE, heapId);
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, 1, PALTYPE_MAIN_BG, style * MSGBG_PLTT_SIZE, palSlot * MSGBG_PLTT_SIZE,
                      MSGBG_PLTT_SIZE, heapId);
}

static void MsgBGPrinter_DrawBgWinFrame(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal) {
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

static void MsgBGPrinter_ClearBgWinFrame(u8 bg, u8 x, u8 y, u8 w, u8 h, u16 tile, u8 pal) {
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

static BOOL MsgBGPrinter_IsLineMissing(const StrBuf *src, StrBuf *dest, u32 line) {
    if (GFL_FontCopyLine(src, dest, line) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MsgBGPrinter_PrintTwoLines(GFLBitmap *bitmap, Font *font, const StrBuf *src, StrBuf *dest, u32 *line,
                                       u8 color) {
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

static BOOL MsgBGPrinter_RollLines(GFLBitmap *dest, GFLBitmap *src1, GFLBitmap *src2, int amount, u16 color) {
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

// The checker frame uses fixed WINFRAME tiles: style no longer selects a block of them, and the block's first tile,
// which the tile is offset from, is always the first of the file
static void CheckerMsgWin_LoadFrameTile(GFLBitmap *bitmap, u32 style, HeapID heapId, CheckerFrameTile tile) {
    NNSG2dCharacterData *charData;
    CheckerFrameTile first = CHECKER_FRAME_TOP_LEFT;
    u8 *pixels = GFL_BitmapGetPixelData(bitmap);
    void *file = GFL_ArcSysReadHeapNewLZ(ARCID_WINFRAME, 7, FALSE, heapId);

    NNS_G2dGetUnpackedBGCharacterData(file, &charData);
    MI_CpuCopy((u8 *)charData->pRawData + first * BGSYS_TILE_SIZE_16 + tile * BGSYS_TILE_SIZE_16, pixels,
               BGSYS_TILE_SIZE_16);
    GFL_HeapFree(file);
}

static void CheckerMsgWin_DrawFrame(GFLBitmap *bitmap, u32 style, HeapID heapId) {
    u16 x;
    u16 y;
    u16 tilesW = GFL_BitmapGetWidth(bitmap) >> 3;
    u16 tilesH = GFL_BitmapGetHeight(bitmap) >> 3;
    GFLBitmap *tile = GFL_BitmapCreate(1, 1, BGSYS_TILE_SIZE_16, heapId);

    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_CENTER);
    for (y = 0; y < tilesH; y++) {
        for (x = 0; x < tilesW; x++) {
            GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
        }
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_TOP);
    for (x = 0; x < tilesW; x++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, 0, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_BOTTOM);
    y = tilesH - 1;
    for (x = 0; x < tilesW; x++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_LEFT);
    for (y = 0; y < tilesH; y++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, 0, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_RIGHT);
    x = tilesW - 1;
    for (y = 0; y < tilesH; y++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    x = 0;
    y = 0;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_TOP_LEFT);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    x = tilesW - 1;
    y = 0;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_TOP_RIGHT);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    x = 0;
    y = tilesH - 1;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_BOTTOM_LEFT);
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, x * 8, y * 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    x = tilesW - 1;
    y = tilesH - 1;
    CheckerMsgWin_LoadFrameTile(tile, style, heapId, CHECKER_FRAME_BOTTOM_RIGHT);
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
    MsgBGPrinter_AnimateCursorOnFrame(&win->cursor, BmpWin_GetBitmap(win->window), win->cursorBack);
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

MsgBGCursor *MsgBGPrinter_CreateCursor(FieldMsgBG *sys) {
    MsgBGCursor *cursor = GFL_HeapAllocate(sys->heapId, sizeof(MsgBGCursor), TRUE, "field_msgbg.c", 4733);

    MsgBGCursor_Init(cursor, sys);
    return cursor;
}

void MsgBGPrinter_DeleteCursor(MsgBGCursor *cursor) {
    MsgBGCursor_Free(cursor);
    GFL_HeapFree(cursor);
}

static void MsgBGCursor_Init(MsgBGCursor *cursor, FieldMsgBG *sys) {
    MI_CpuFill8(cursor, 0, sizeof(MsgBGCursor));
    cursor->bitmap = GFL_BitmapWrap(sys->charData->pRawData, 1, 1, BGSYS_TILE_SIZE_16, sys->heapId);
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

void MsgBGPrinter_AnimateCursor(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color) {
    MsgBGCursor_Clear(cursor, bitmap, color);
    cursor->timer++;
    if (cursor->timer >= 4) {
        cursor->timer = 0;
        cursor->frame++;
        cursor->frame = cursor->frame % 3;
    }
    MsgBGCursor_Draw(cursor, bitmap, color);
}

static void MsgBGPrinter_AnimateCursorOnFrame(MsgBGCursor *cursor, GFLBitmap *bitmap, GFLBitmap *dest) {
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

static void MsgBGPrinter_LoadSysWindowFrameGraphics(HeapID heapId) {
    GFL_BGSysLoadNCGRStatic(ARCID_WINFRAME, 9, MSGBG_BG, MSGBG_CHAR_LIST_FRAME, 0, FALSE, heapId);
}

static void MsgBGPrinter_DrawSysWindowFrame(BmpWin *win) {
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

static BmpWin *MsgBGPrinter_CreateSysFrameWindow(u8 x, u8 y, u8 w, u8 h, HeapID heapId) {
    BmpWin *win = BmpWin_CreateDynamic(MSGBG_BG2, x, y, w, h, MSGBG_PLTT_BG2, 1);

    GFL_BitmapFill(BmpWin_GetBitmap(win), MSGBG_COLOR_BG_FILL);
    BmpWin_FlushMap(win);
    BmpWin_FlushChar(win);
    MsgBGPrinter_DrawSysWindowFrame(win);
    return win;
}

static void MsgBGPrinter_ClearSysWindowFrame(BmpWin *win) {
    u8 bg = BmpWin_GetBGIndex(win);

    GFL_BGSysFillScrArea(bg, 0, BmpWin_GetPosX(win) - 1, BmpWin_GetPosY(win) - 1, BmpWin_GetSizeX(win) + 2,
                         BmpWin_GetSizeY(win) + 2, 0);
    GFL_BGSysLoadScr(bg);
}

static void MsgBGPrinter_DeleteSysFrameWindow(BmpWin *win) {
    MsgBGPrinter_ClearSysWindowFrame(win);
    BmpWin_Free(win);
}

static void MsgBGPrinter_InitBgTwoLayerTalk(FieldMsgBG *sys, BOOL loadFrame) {
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
        MsgBGPrinter_LoadWindowGraphicsAll(sys->bg, sys->heapId);
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

static void MsgBGPrinter_InitBgTwoLayer(FieldMsgBG *sys, BOOL loadFrame) {
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
        MsgBGPrinter_LoadWindowGraphicsAll(sys->bg, sys->heapId);
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

static void MsgBGPrinter_InitBgOneLayer(FieldMsgBG *sys) {
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
    MsgBGPrinter_LoadWindowGraphicsAll(sys->bg, sys->heapId);
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

static void MsgBGPrinter_VblankTalkSysTask(TCB *tcb, void *data) {
    FieldMsgBG *sys = data;

    if (!GFL_IRQCartDataTransferIsEnabled()) {
        loadTexture_wrapper(sys->talk);
        sys->vblankTcb = NULL;
        GFL_TCBRemove(tcb);
    }
}

static void MsgBGPrinter_SetupSysFrameBg(u32 create, u32 alphaBlend) {
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

static void MsgBGPrinter_SetupSysFrame(FieldMsgBG *sys) {
    if (sys->resetBG2 != FALSE) {
        sys->resetBG2 = FALSE;
        if (sys->bg2 == MSGBG_NO_BG) {
            GFL_BGSysReleaseBG(MSGBG_BG2);
            MsgBGPrinter_SetupSysFrameBg(TRUE, sys->alphaBlend);
            sys->bg2 = MSGBG_BG2;
        } else {
            MsgBGPrinter_SetupSysFrameBg(FALSE, sys->alphaBlend);
        }
    }
}

static void MsgBGPrinter_LoadWindowGraphicsAll(u32 bg, HeapID heapId) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, MSGBG_PLTT_TALK * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE,
                             heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, PALTYPE_MAIN_BG, MSGBG_PLTT_BG2 * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE, heapId);
    LoadSysMsgBox(bg, MSGBG_CHAR_FRAME, MSGBG_PLTT_BG2_FRAME, 0, heapId);
    MsgBGPrinter_LoadSysWindowFrameGraphics(heapId);
}

static void MsgBGPrinter_LoadWindowGraphicsStepFn(u32 bg, HeapID heapId, u32 step) {
    if (step == MSGBG_LOAD_PALETTES) {
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, MSGBG_PLTT_TALK * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE,
                                 heapId);
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, PALTYPE_MAIN_BG, MSGBG_PLTT_BG2 * MSGBG_PLTT_SIZE, MSGBG_PLTT_SIZE,
                                 heapId);
    } else if (step == MSGBG_LOAD_SYSTEM_FRAME) {
        LoadSysMsgBox(bg, MSGBG_CHAR_FRAME, MSGBG_PLTT_BG2_FRAME, 0, heapId);
    } else if (step == MSGBG_LOAD_LIST_FRAME) {
        MsgBGPrinter_LoadSysWindowFrameGraphics(heapId);
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

void MsgBGPrinter_ReloadBg(FieldMsgBG *sys) {
    GFL_BGSysFreeFilledChar(sys->bg, 1, 0);
    GFL_BGSysFillChar(sys->bg, 0, 1, 0);
    GFL_BGSysFillScrArea(sys->bg, 0, 0, 0, 0x20, 0x20, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysLoadScr(sys->bg);
    MsgBGPrinter_LoadWindowGraphicsAll(sys->bg, sys->heapId);
    TalkMsgWin_ReloadFrameGraphics(sys->talk);
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

static void MsgBGPrinter_DeleteWindow(BmpWin *win) {
    BmpWin_ClearFrame(win, WINFRAME_TRANSFER_NOW);
    BmpWin_Free(win);
}

static void MsgBGPrinter_LoadMenuGraphicsLower(u32 heapId) {
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

static BOOL MsgBGPrinter_KeyWaitCheckSkip(MsgBGInput *input) {
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

static BOOL MsgBGPrinter_KeyWaitCheckAdvance(MsgBGInput *input, BOOL fast) {
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

static void MsgBGPrinter_StreamSetSpeed(PrintStream *stream, BOOL fast) {
    if (fast == TRUE) {
        PrintStream_SetFastWait(stream, TextSpeed_GetWait(2));
    } else {
        PrintStream_ClearFast(stream);
    }
}
