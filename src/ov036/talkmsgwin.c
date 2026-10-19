// The field's talk windows. The name is the ROM's string, from GFL_HeapAllocate's assert. A talk window is a BmpWin
// whose frame is drawn in its BG's screen in one of three styles, and which can point a tail at a spot in the 3D scene
// or at another window: the tail is a textured triangle, and while the window opens it grows from the tail's tip as a
// quad. loadTexture_wrapper, loadTexture, the ActorMsgWin_ functions, TALKMSG_WIN_TEX and TALKMSG_WIN_PLT are swan's
// names; the other functions are unnamed, and the types' and constants' names are ours. The code was first written by
// mariano54 on their parallel-decomp branch.

#include "types.h"
#include "constants/arc.h"
#include "field/talkmsgwin.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "system/printsys.h"

#define TALKMSG_WIN_COUNT 32

// The screen's size in characters, which a window and its frame stay within
#define TALKMSG_WIN_SCREEN_WIDTH 32
#define TALKMSG_WIN_SCREEN_HEIGHT 24

// The windows across the top or the bottom of the screen: as wide as the screen within their frame, 4 characters high
#define TALKMSG_WIN_BAR_HEIGHT 4

// The frames a window takes to grow from its tail's tip, and to close
#define TALKMSG_WIN_OPEN_FRAMES 8
#define TALKMSG_WIN_CLOSE_FRAMES 4

enum {
    TALKMSG_WIN_STATE_FREE,
    TALKMSG_WIN_STATE_CREATED,
    TALKMSG_WIN_STATE_OPEN,
    TALKMSG_WIN_STATE_OPENING,
    TALKMSG_WIN_STATE_SHOWN,
    TALKMSG_WIN_STATE_CLOSING,
    TALKMSG_WIN_STATE_CLOSED,
};

// Where a window is placed: as asked, or across the top or the bottom of the screen with its tail on the other side
enum {
    TALKMSG_WIN_SHAPE_FREE,
    TALKMSG_WIN_SHAPE_TOP,
    TALKMSG_WIN_SHAPE_BOTTOM,
};

typedef struct {
    u32 shape;
    u32 tail;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 color;
    u16 frameType;
    u32 wait;
} TalkMsgWinParam;

// The polygons a window draws: the tail, a triangle from the target to two points on the frame, and the window's quad,
// their vertices relative to the target and divided by the size, the farthest of them
typedef struct {
    VecFx32 origin;
    fx32 size;
    VecFx16 vtx[7];
    u32 tail;
} TalkMsgWinPolygon;

typedef struct {
    u16 state;
    u8 skipAnim;
    u8 frameType;
    u32 shape;
    PrintStream *stream;
    BmpWin *bmpWin;
    const VecFx32 *target;
    StrBuf *strbuf;
    u16 color;
    u16 anchorWin;
    TalkMsgWinPolygon poly;
    u8 textX;
    u8 textY;
    u16 frame;
    u32 wait;
} TalkMsgWin;

struct TalkMsgWinSys {
    TalkMsgWinSetup setup;
    TalkMsgWin windows[TALKMSG_WIN_COUNT];
    TCBExManager *tcbMgr;
    u16 frameChars;
    VecFx32 camPos;
    VecFx32 camUp;
    VecFx32 camTarget;
    fx32 camZNear;
    NNSGfdTexKey texKey;
    NNSGfdPlttKey plttKey;
};

static void func_ov036_0218b5e0(TalkMsgWin *win);
static BOOL func_ov036_0218b5fc(TalkMsgWin *win);
static BOOL func_ov036_0218b60c(TalkMsgWin *win);
static BmpWin *func_ov036_0218b61c(TalkMsgWinSys *sys, u8 x, u8 y, u8 width, u8 height);
static void ActorMsgWin_CreateCore(TalkMsgWinSys *sys, TalkMsgWin *win, const VecFx32 *target, StrBuf *strbuf,
                                   const TalkMsgWinParam *param);
static void func_ov036_0218b6c4(TalkMsgWinSys *sys, TalkMsgWin *win);
static void func_ov036_0218b6f8(TalkMsgWin *win);
static void func_ov036_0218b700(TalkMsgWin *win);
// A window's state, for windows that grow from their tail: the polygons follow the target, the frame is redrawn
// when the tail moves to another side, and printing starts once the window is full size
static void func_ov036_0218b70c(TalkMsgWinSys *sys, TalkMsgWin *win);
// A window's state, for windows created by func_ov036_0218b370, which open and close at once
static void func_ov036_0218b7e8(TalkMsgWinSys *sys, TalkMsgWin *win);
static void func_ov036_0218b870(TalkMsgWinSys *sys, TalkMsgWin *win);
// Draws the tail, and the window's quad scaled by the frame of its opening while it isn't shown
static void func_ov036_0218b8a4(TalkMsgWinSys *sys, TalkMsgWin *win, BOOL shown);
// The screen position of a point of the scene, and the point on the near plane there
static void func_ov036_0218b9fc(const VecFx32 *world, int *x, int *y, VecFx32 *out);
// The point of another window that a tail between them ends at, on the near plane
static void func_ov036_0218ba18(TalkMsgWin *win, TalkMsgWin *other, int *outX, int *outY, VecFx32 *out);
// Where a tail leaves the frame of a window across the screen, on its left or its right part
static void func_ov036_0218bae4(int x, int width, BOOL right, u16 *outLeft, u16 *outRight);
// Where the tail leaves the frame, on the near plane, for a target at targetX and targetY on the screen, and which
// side of the frame that is
static u32 func_ov036_0218bb0c(TalkMsgWin *win, int targetX, int targetY, VecFx32 *start, VecFx32 *end);
// The window's corners on the near plane
static void func_ov036_0218bcb0(TalkMsgWin *win, VecFx32 *topLeft, VecFx32 *topRight, VecFx32 *bottomLeft,
                                VecFx32 *bottomRight);
// Sets the polygons from the tail's tip, where it leaves the frame, and the window's corners
static void func_ov036_0218bd14(TalkMsgWinSys *sys, TalkMsgWin *win, const VecFx32 *target, const VecFx32 *start,
                                const VecFx32 *end, const VecFx32 *topLeft, const VecFx32 *topRight,
                                const VecFx32 *bottomLeft, const VecFx32 *bottomRight);
// Moves the polygons to the target, and returns whether the tail went to another side of the frame
static BOOL func_ov036_0218bec0(TalkMsgWinSys *sys, TalkMsgWin *win);
// Looks at the target from the camera's place relative to it, with the near plane in front of the camera's, through
// the camera or, to draw, straight to the geometry engine
static void func_ov036_0218bf5c(TalkMsgWinSys *sys, BOOL draw);
static void func_ov036_0218bfe4(TalkMsgWinSys *sys);
static void func_ov036_0218c018(TalkMsgWinSys *sys);
static u32 func_ov036_0218c054(TalkMsgWinSys *sys, TalkMsgWinSetup *setup);
static void loadTexture(TalkMsgWinSys *sys);
// The frames, drawn in the BG's screen around a window except at the screen's edges, with the tail's tile on one side.
// A tile number's 0x400 and 0x800 flip it horizontally and vertically
static void func_ov036_0218c174(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 tail);
static void func_ov036_0218c4c0(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 tail);
static void func_ov036_0218c9dc(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 tail);
static void func_ov036_0218cf00(TalkMsgWinSys *sys, BmpWin *bmpWin, u8 frameType, u32 tail);
static void func_ov036_0218cf28(TalkMsgWinSys *sys, TalkMsgWin *win);
static void func_ov036_0218cf38(TalkMsgWinSys *sys, TalkMsgWin *win);
// Clears the frame around a window
static void func_ov036_0218cf48(TalkMsgWinSys *sys, BmpWin *bmpWin);

// The tails' texture, 8x8 texels of 16 colors, and its palette
static u8 TALKMSG_WIN_TEX[] = {
    0x44, 0x44, 0x44, 0x44, 0x34, 0x33, 0x33, 0x33, 0x34, 0x22, 0x22, 0x22, 0x34, 0x12, 0x11, 0x11,
    0x34, 0x12, 0x11, 0x11, 0x34, 0x12, 0x11, 0x11, 0x34, 0x12, 0x11, 0x11, 0x34, 0x12, 0x11, 0xff,
};

static u8 TALKMSG_WIN_PLT[] = {
    0x00, 0x00, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x63,
};

TalkMsgWinSys *func_ov036_0218b1f8(const TalkMsgWinSetup *setup) {
    TalkMsgWinSys *sys = GFL_HeapAllocate(setup->heapId, sizeof(TalkMsgWinSys), TRUE, "talkmsgwin.c", 236);
    int i;

    sys->setup = *setup;
    sys->tcbMgr = GFL_TCBExMgrCreate(setup->heapId, setup->heapId, TALKMSG_WIN_COUNT, 32);
    for (i = 0; i < TALKMSG_WIN_COUNT; i++) {
        func_ov036_0218b5e0(&sys->windows[i]);
    }
    sys->texKey = NNS_GfdAllocTexVram(sizeof(TALKMSG_WIN_TEX), FALSE, 0);
    sys->plttKey = NNS_GfdAllocPlttVram(sizeof(TALKMSG_WIN_PLT), FALSE, 0);
    sys->frameChars = func_ov036_0218c054(sys, &sys->setup) / BGSYS_TILE_SIZE_16;
    return sys;
}

void loadTexture_wrapper(TalkMsgWinSys *sys) {
    loadTexture(sys);
}

void func_ov036_0218b294(TalkMsgWinSys *sys) {
    int i;

    GFL_TCBExMgrUpdate(sys->tcbMgr);
    func_ov036_0218bfe4(sys);
    func_ov036_0218bf5c(sys, FALSE);
    for (i = 0; i < TALKMSG_WIN_COUNT; i++) {
        if (sys->windows[i].skipAnim == 0) {
            func_ov036_0218b70c(sys, &sys->windows[i]);
        } else {
            func_ov036_0218b7e8(sys, &sys->windows[i]);
        }
    }
    func_ov036_0218c018(sys);
}

void func_ov036_0218b2e8(TalkMsgWinSys *sys) {
}

void func_ov036_0218b2ec(TalkMsgWinSys *sys) {
    int i;

    NNS_G3dGeFlushBuffer();
    func_ov036_0218bf5c(sys, TRUE);
    for (i = 0; i < TALKMSG_WIN_COUNT; i++) {
        if (sys->windows[i].skipAnim == 0) {
            func_ov036_0218b870(sys, &sys->windows[i]);
        }
    }
}

void func_ov036_0218b320(TalkMsgWinSys *sys) {
    int i;

    for (i = 0; i < TALKMSG_WIN_COUNT; i++) {
        func_ov036_0218b6c4(sys, &sys->windows[i]);
    }
    NNS_GfdFreePlttVram(sys->plttKey);
    NNS_GfdFreeTexVram(sys->texKey);
    GFL_TCBExMgrFree(sys->tcbMgr);
    GFL_HeapFree(sys);
}

void func_ov036_0218b370(TalkMsgWinSys *sys, u32 index, StrBuf *strbuf, u8 x, u8 y, u8 width, u8 height, u32 unused,
                         u32 frameType, u32 wait, u8 textX, u8 textY) {
    TalkMsgWinParam setup;
    TalkMsgWin *win;

    setup.shape = TALKMSG_WIN_SHAPE_FREE;
    setup.tail = TALKMSG_WIN_TAIL_NONE;
    setup.x = x;
    setup.y = y;
    setup.width = width;
    setup.height = height;
    setup.color = GX_RGB(31, 31, 31);
    setup.frameType = frameType;
    setup.wait = wait;
    win = &sys->windows[index];
    ActorMsgWin_CreateCore(sys, win, NULL, strbuf, &setup);
    win->textX = textX;
    win->textY = textY;
    win->skipAnim = 1;
}

void ActorMsgWin_CreateAtTop(TalkMsgWinSys *sys, u32 index, const VecFx32 *target, StrBuf *strbuf, u8 unused,
                             u32 frameType, u32 tail, u32 wait) {
    TalkMsgWinParam setup;

    setup.shape = TALKMSG_WIN_SHAPE_TOP;
    setup.tail = tail;
    setup.x = 1;
    setup.y = 1;
    setup.width = TALKMSG_WIN_SCREEN_WIDTH - 2;
    setup.height = TALKMSG_WIN_BAR_HEIGHT;
    setup.color = GX_RGB(31, 31, 31);
    setup.frameType = frameType;
    setup.wait = wait;
    ActorMsgWin_CreateCore(sys, &sys->windows[index], target, strbuf, &setup);
}

void ActorMsgWin_CreateAtBottom(TalkMsgWinSys *sys, u32 index, const VecFx32 *target, StrBuf *strbuf, u8 unused,
                                u32 frameType, u32 tail, u32 wait) {
    TalkMsgWinParam setup;

    setup.shape = TALKMSG_WIN_SHAPE_BOTTOM;
    setup.tail = tail;
    setup.x = 1;
    setup.y = TALKMSG_WIN_SCREEN_HEIGHT - TALKMSG_WIN_BAR_HEIGHT - 1;
    setup.width = TALKMSG_WIN_SCREEN_WIDTH - 2;
    setup.height = TALKMSG_WIN_BAR_HEIGHT;
    setup.color = GX_RGB(31, 31, 31);
    setup.frameType = frameType;
    setup.wait = wait;
    ActorMsgWin_CreateCore(sys, &sys->windows[index], target, strbuf, &setup);
}

void ActorMsgWin_CreateAtAutoPos(TalkMsgWinSys *sys, u32 index, const VecFx32 *target, StrBuf *strbuf, u8 unused,
                                 u32 frameType, u32 wait) {
    int x;
    int y;

    NNS_G3dWorldPosToScrPos(target, &x, &y);
    if (y < TALKMSG_WIN_SCREEN_HEIGHT * 8 / 2) {
        ActorMsgWin_CreateAtBottom(sys, index, target, strbuf, unused, frameType, TALKMSG_WIN_TAIL_NONE, wait);
    } else {
        ActorMsgWin_CreateAtTop(sys, index, target, strbuf, unused, frameType, TALKMSG_WIN_TAIL_NONE, wait);
    }
}

void func_ov036_0218b4b4(TalkMsgWinSys *sys, u32 index) {
    func_ov036_0218b6c4(sys, &sys->windows[index]);
}

void func_ov036_0218b4c8(TalkMsgWinSys *sys, u32 index) {
    func_ov036_0218b6f8(&sys->windows[index]);
}

void func_ov036_0218b4d8(TalkMsgWinSys *sys, u32 index) {
    func_ov036_0218b700(&sys->windows[index]);
}

BOOL func_ov036_0218b4e8(TalkMsgWinSys *sys, u32 index) {
    if (sys->windows[index].state == TALKMSG_WIN_STATE_FREE) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov036_0218b4fc(TalkMsgWinSys *sys, u32 index) {
    return func_ov036_0218b60c(&sys->windows[index]);
}

PrintStream *func_ov036_0218b50c(TalkMsgWinSys *sys, u32 index) {
    return sys->windows[index].stream;
}

BmpWin *func_ov036_0218b518(TalkMsgWinSys *sys, u32 index) {
    return sys->windows[index].bmpWin;
}

void func_ov036_0218b524(TalkMsgWinSys *sys, u32 index, StrBuf *strbuf, u32 wait) {
    TalkMsgWin *win = &sys->windows[index];

    if (win->stream != NULL) {
        PrintStream_Delete(win->stream);
        win->stream = NULL;
    }
    if (strbuf != NULL) {
        win->wait = wait;
        win->strbuf = strbuf;
        win->stream = PrintStream_Create(win->bmpWin, win->textX, win->textY, win->strbuf, sys->setup.font, wait,
                                    sys->tcbMgr, 0, sys->setup.heapId, 15);
        win->state = TALKMSG_WIN_STATE_SHOWN;
    }
}

BmpWin *func_ov036_0218b588(TalkMsgWinSys *sys, u8 x, u8 y, u8 width, u8 height, u32 frameType) {
    BmpWin *bmpWin = func_ov036_0218b61c(sys, x, y, width, height);

    func_ov036_0218cf00(sys, bmpWin, frameType, TALKMSG_WIN_TAIL_NONE);
    return bmpWin;
}

void func_ov036_0218b5ac(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 frameType) {
    func_ov036_0218cf00(sys, bmpWin, frameType, TALKMSG_WIN_TAIL_NONE);
}

void func_ov036_0218b5bc(TalkMsgWinSys *sys, BmpWin *bmpWin) {
    func_ov036_0218cf48(sys, bmpWin);
    GFL_BGSysLoadScr(sys->setup.bg);
}

void func_ov036_0218b5d0(TalkMsgWinSys *sys, BmpWin *bmpWin) {
    func_ov036_0218cf48(sys, bmpWin);
    BmpWin_Free(bmpWin);
}

static void func_ov036_0218b5e0(TalkMsgWin *win) {
    win->state = TALKMSG_WIN_STATE_FREE;
    win->stream = NULL;
    VEC_Fx16Set(&win->poly.vtx[0], 0, 0, 0);
    VEC_Fx16Set(&win->poly.vtx[1], 0, 0, 0);
    VEC_Fx16Set(&win->poly.vtx[2], 0, 0, 0);
}

static BOOL func_ov036_0218b5fc(TalkMsgWin *win) {
    if (win->state == TALKMSG_WIN_STATE_FREE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0218b60c(TalkMsgWin *win) {
    if (win->state == TALKMSG_WIN_STATE_SHOWN) {
        return TRUE;
    }
    return FALSE;
}

static BmpWin *func_ov036_0218b61c(TalkMsgWinSys *sys, u8 x, u8 y, u8 width, u8 height) {
    BmpWin *bmpWin = BmpWin_CreateDynamic(sys->setup.bg, x, y, width, height, sys->setup.winPalette, TRUE);

    GFL_BitmapFill(BmpWin_GetBitmap(bmpWin), 15);
    BmpWin_FlushChar(bmpWin);
    return bmpWin;
}

static void ActorMsgWin_CreateCore(TalkMsgWinSys *sys, TalkMsgWin *win, const VecFx32 *target, StrBuf *strbuf,
                                   const TalkMsgWinParam *param) {
    u8 x;
    int y;
    u8 width;
    u8 height;

    func_ov036_0218b6c4(sys, win);
    win->target = target;
    win->strbuf = strbuf;
    win->color = param->color;
    win->anchorWin = 0;
    win->shape = param->shape;
    win->frameType = param->frameType;
    win->wait = param->wait;
    width = param->width;
    x = param->x + width > TALKMSG_WIN_SCREEN_WIDTH ? TALKMSG_WIN_SCREEN_WIDTH - width : param->x;
    height = param->height;
    y = param->y;
    if (y + height > TALKMSG_WIN_SCREEN_HEIGHT) {
        y = TALKMSG_WIN_SCREEN_HEIGHT - height;
    }
    win->bmpWin = func_ov036_0218b61c(sys, x, y, width, height);
    win->poly.tail = param->tail;
    win->textX = 8;
    win->textY = 0;
    win->state = TALKMSG_WIN_STATE_CREATED;
}

static void func_ov036_0218b6c4(TalkMsgWinSys *sys, TalkMsgWin *win) {
    if (func_ov036_0218b5fc(win) == FALSE) {
        if (win->stream != NULL) {
            PrintStream_Delete(win->stream);
            win->stream = NULL;
        }
        func_ov036_0218cf48(sys, win->bmpWin);
        BmpWin_Free(win->bmpWin);
        win->skipAnim = 0;
        win->state = TALKMSG_WIN_STATE_FREE;
    }
}

static void func_ov036_0218b6f8(TalkMsgWin *win) {
    win->state = TALKMSG_WIN_STATE_OPEN;
}

static void func_ov036_0218b700(TalkMsgWin *win) {
    win->state = TALKMSG_WIN_STATE_CLOSING;
    win->frame = TALKMSG_WIN_CLOSE_FRAMES;
}

static void func_ov036_0218b70c(TalkMsgWinSys *sys, TalkMsgWin *win) {
    switch (win->state) {
    case TALKMSG_WIN_STATE_OPEN:
        func_ov036_0218bec0(sys, win);
        win->frame = 0;
        win->state = TALKMSG_WIN_STATE_OPENING;
        break;
    case TALKMSG_WIN_STATE_OPENING:
        func_ov036_0218bec0(sys, win);
        if (win->frame < TALKMSG_WIN_OPEN_FRAMES) {
            win->frame++;
        } else {
            u32 wait;

            win->state = TALKMSG_WIN_STATE_SHOWN;
            wait = win->wait;
            func_ov036_0218cf38(sys, win);
            win->stream = PrintStream_Create(win->bmpWin, win->textX, win->textY, win->strbuf, sys->setup.font, wait,
                                        sys->tcbMgr, 0, sys->setup.heapId, 15);
        }
        break;
    case TALKMSG_WIN_STATE_SHOWN:
        if (func_ov036_0218bec0(sys, win) == TRUE) {
            func_ov036_0218cf38(sys, win);
        }
        break;
    case TALKMSG_WIN_STATE_CLOSING:
        if (win->frame != 0) {
            win->frame--;
        } else {
            win->state = TALKMSG_WIN_STATE_CLOSED;
        }
        break;
    case TALKMSG_WIN_STATE_CLOSED:
        func_ov036_0218b6c4(sys, win);
        break;
    }
}

static void func_ov036_0218b7e8(TalkMsgWinSys *sys, TalkMsgWin *win) {
    switch (win->state) {
    case TALKMSG_WIN_STATE_OPEN:
        win->frame = 0;
        win->state = TALKMSG_WIN_STATE_OPENING;
    case TALKMSG_WIN_STATE_OPENING: {
        u32 wait;

        win->state = TALKMSG_WIN_STATE_SHOWN;
        wait = win->wait;
        func_ov036_0218cf28(sys, win);
        win->stream = PrintStream_Create(win->bmpWin, win->textX, win->textY, win->strbuf, sys->setup.font, wait,
                                    sys->tcbMgr, 0, sys->setup.heapId, 15);
        break;
    }
    case TALKMSG_WIN_STATE_CLOSING:
        win->state = TALKMSG_WIN_STATE_CLOSED;
        break;
    case TALKMSG_WIN_STATE_CLOSED:
        func_ov036_0218b6c4(sys, win);
        break;
    }
}

static void func_ov036_0218b870(TalkMsgWinSys *sys, TalkMsgWin *win) {
    switch (win->state) {
    case TALKMSG_WIN_STATE_FREE:
    case TALKMSG_WIN_STATE_CREATED:
    case TALKMSG_WIN_STATE_OPEN:
        break;
    case TALKMSG_WIN_STATE_OPENING:
        func_ov036_0218b8a4(sys, win, FALSE);
        break;
    case TALKMSG_WIN_STATE_SHOWN:
        func_ov036_0218b8a4(sys, win, TRUE);
        break;
    case TALKMSG_WIN_STATE_CLOSING:
    case TALKMSG_WIN_STATE_CLOSED:
        break;
    }
}

static void func_ov036_0218b8a4(TalkMsgWinSys *sys, TalkMsgWin *win, BOOL shown) {
    const TalkMsgWinPolygon *poly = &win->poly;
    fx32 scale;

    G3_PushMtx();
    G3_Translate(poly->origin.x, poly->origin.y, poly->origin.z);
    scale = poly->size * win->frame / TALKMSG_WIN_OPEN_FRAMES;
    G3_Scale(scale, scale, scale);
    G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_NONE, GX_TEXSIZE_S8, GX_TEXSIZE_T8, GX_TEXREPEAT_ST, GX_TEXFLIP_NONE,
                     GX_TEXPLTTCOLOR0_USE, NNS_GfdGetTexKeyAddr(sys->texKey));
    G3_TexPlttBase(NNS_GfdGetPlttKeyAddr(sys->plttKey), GX_TEXFMT_PLTT16);
    G3_MaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(16, 16, 16), TRUE);
    G3_MaterialColorSpecEmi(GX_RGB(16, 16, 16), GX_RGB(0, 0, 0), FALSE);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 63, 31, 0);
    if (poly->tail != TALKMSG_WIN_TAIL_NONE) {
        G3_Begin(GX_BEGIN_TRIANGLES);
        G3_Color(win->color);
        G3_TexCoord(0, 8 * FX32_ONE);
        G3_Vtx(poly->vtx[2].x, poly->vtx[2].y, poly->vtx[2].z);
        G3_TexCoord(0, 0);
        G3_Vtx(poly->vtx[0].x, poly->vtx[0].y, poly->vtx[0].z);
        G3_TexCoord(8 * FX32_ONE, 0);
        G3_Vtx(poly->vtx[1].x, poly->vtx[1].y, poly->vtx[1].z);
        G3_End();
    }
    if (shown == FALSE) {
        G3_Begin(GX_BEGIN_QUADS);
        G3_Color(win->color);
        G3_TexCoord(0, 8 * FX32_ONE);
        G3_Vtx(poly->vtx[5].x, poly->vtx[5].y, poly->vtx[5].z);
        G3_TexCoord(0, 0);
        G3_Vtx(poly->vtx[3].x, poly->vtx[3].y, poly->vtx[3].z);
        G3_TexCoord(8 * FX32_ONE, 0);
        G3_Vtx(poly->vtx[4].x, poly->vtx[4].y, poly->vtx[4].z);
        G3_TexCoord(8 * FX32_ONE, 8 * FX32_ONE);
        G3_Vtx(poly->vtx[6].x, poly->vtx[6].y, poly->vtx[6].z);
        G3_End();
    }
    G3_PopMtx(1);
}

static void func_ov036_0218b9fc(const VecFx32 *world, int *x, int *y, VecFx32 *out) {
    NNS_G3dWorldPosToScrPos(world, x, y);
    NNS_G3dScrPosToWorldLine(*x, *y, out, NULL);
}

static void func_ov036_0218ba18(TalkMsgWin *win, TalkMsgWin *other, int *outX, int *outY, VecFx32 *out) {
    int winX = (BmpWin_GetPosX(win->bmpWin) - 1) * 8;
    int winY = (BmpWin_GetPosY(win->bmpWin) - 1) * 8;
    int winW = (BmpWin_GetWidth1(win->bmpWin) + 2) * 8;
    int winH = (BmpWin_GetHeight2(win->bmpWin) + 2) * 8;
    int otherX = (BmpWin_GetPosX(other->bmpWin) - 1) * 8;
    int otherY = (BmpWin_GetPosY(other->bmpWin) - 1) * 8;
    int otherW = (BmpWin_GetWidth1(other->bmpWin) + 2) * 8;
    int otherH = (BmpWin_GetHeight2(other->bmpWin) + 2) * 8;

    if (winX + winW < otherX) {
        *outX = otherX;
    } else if (winX > otherX + otherW) {
        *outX = otherX + otherW;
    } else {
        *outX = otherX + otherW / 2;
    }
    if (winY + winH < otherY) {
        *outY = otherY;
    } else if (winY > otherY + otherH) {
        *outY = otherY + otherH;
    } else {
        *outY = otherY + otherH / 2;
    }
    NNS_G3dScrPosToWorldLine(*outX, *outY, out, NULL);
}

static void func_ov036_0218bae4(int x, int width, BOOL right, u16 *outLeft, u16 *outRight) {
    if (right == FALSE) {
        *outLeft = x + 64;
        *outRight = *outLeft + 16;
    } else {
        *outRight = x + width - 64;
        *outLeft = *outRight - 16;
    }
}

static u32 func_ov036_0218bb0c(TalkMsgWin *win, int targetX, int targetY, VecFx32 *start, VecFx32 *end) {
    int x = (BmpWin_GetPosX(win->bmpWin) - 1) * 8;
    int y = (BmpWin_GetPosY(win->bmpWin) - 1) * 8;
    int width = (BmpWin_GetWidth1(win->bmpWin) + 2) * 8;
    int height = (BmpWin_GetHeight2(win->bmpWin) + 2) * 8;
    u16 startY;
    u16 endY;
    u16 startX;
    u16 endX;
    u32 result;
    BOOL right;

    y += GFL_BGSysGetBGOffsetY(BmpWin_GetBGIndex(win->bmpWin));
    switch (win->shape) {
    case TALKMSG_WIN_SHAPE_FREE:
    default: {
        int size;

        if (width >= 16 && height >= 16) {
            size = 16;
        } else {
            size = height;
        }
        if (targetY < y) {
            endY = startY = y + 3;
            startX = x + width / 2 - size / 2;
            endX = x + width / 2 + size / 2;
            result = TALKMSG_WIN_TAIL_TOP;
        } else if (targetY > y + height) {
            endY = startY = y + height - 3;
            startX = x + width / 2 - size / 2;
            endX = x + width / 2 + size / 2;
            result = TALKMSG_WIN_TAIL_BOTTOM;
        } else {
            startY = y + height / 2 - size / 2;
            endY = y + height / 2 + size / 2;
            if (targetX < x) {
                startX = x + 1;
                endX = x + 1;
                result = TALKMSG_WIN_TAIL_LEFT;
            } else if (targetX > x + width) {
                startX = x + width - 1;
                endX = x + width - 1;
                result = TALKMSG_WIN_TAIL_RIGHT;
            } else {
                startX = x + width / 2;
                endX = x + width / 2;
                result = TALKMSG_WIN_TAIL_NONE;
            }
        }
        break;
    }
    case TALKMSG_WIN_SHAPE_TOP:
        endY = startY = y + height - 4;
        if (win->poly.tail == TALKMSG_WIN_TAIL_NONE) {
            if (targetX < TALKMSG_WIN_SCREEN_WIDTH * 8 / 2) {
                right = FALSE;
                result = TALKMSG_WIN_TAIL_BOTTOM_LEFT;
            } else {
                right = TRUE;
                result = TALKMSG_WIN_TAIL_BOTTOM_RIGHT;
            }
        } else if (win->poly.tail == TALKMSG_WIN_TAIL_BOTTOM_LEFT) {
            right = FALSE;
            result = TALKMSG_WIN_TAIL_BOTTOM_LEFT;
        } else {
            right = TRUE;
            result = TALKMSG_WIN_TAIL_BOTTOM_RIGHT;
        }
        func_ov036_0218bae4(x, width, right, &startX, &endX);
        break;
    case TALKMSG_WIN_SHAPE_BOTTOM:
        endY = startY = y + 4;
        if (win->poly.tail == TALKMSG_WIN_TAIL_NONE) {
            if (targetX < TALKMSG_WIN_SCREEN_WIDTH * 8 / 2) {
                right = FALSE;
                result = TALKMSG_WIN_TAIL_TOP_LEFT;
            } else {
                right = TRUE;
                result = TALKMSG_WIN_TAIL_TOP_RIGHT;
            }
        } else if (win->poly.tail == TALKMSG_WIN_TAIL_TOP_LEFT) {
            right = FALSE;
            result = TALKMSG_WIN_TAIL_TOP_LEFT;
        } else {
            right = TRUE;
            result = TALKMSG_WIN_TAIL_TOP_RIGHT;
        }
        func_ov036_0218bae4(x, width, right, &startX, &endX);
        break;
    }
    NNS_G3dScrPosToWorldLine(startX, startY, start, NULL);
    NNS_G3dScrPosToWorldLine(endX, endY, end, NULL);
    return result;
}

static void func_ov036_0218bcb0(TalkMsgWin *win, VecFx32 *topLeft, VecFx32 *topRight, VecFx32 *bottomLeft,
                                VecFx32 *bottomRight) {
    int x = BmpWin_GetPosX(win->bmpWin) * 8;
    int y = (BmpWin_GetPosY(win->bmpWin) - 1) * 8;
    int width = BmpWin_GetWidth1(win->bmpWin) * 8;
    int height = (BmpWin_GetHeight2(win->bmpWin) + 2) * 8;

    NNS_G3dScrPosToWorldLine(x, y, topLeft, NULL);
    NNS_G3dScrPosToWorldLine(x + width, y, topRight, NULL);
    NNS_G3dScrPosToWorldLine(x, y + height, bottomLeft, NULL);
    NNS_G3dScrPosToWorldLine(x + width, y + height, bottomRight, NULL);
}

static void func_ov036_0218bd14(TalkMsgWinSys *sys, TalkMsgWin *win, const VecFx32 *target, const VecFx32 *start,
                                const VecFx32 *end, const VecFx32 *topLeft, const VecFx32 *topRight,
                                const VecFx32 *bottomLeft, const VecFx32 *bottomRight) {
    VecFx32 toStart;
    VecFx32 toEnd;
    VecFx32 toTopLeft;
    VecFx32 toTopRight;
    VecFx32 toBottomLeft;
    VecFx32 toBottomRight;
    fx32 lenStart;
    fx32 lenEnd;
    fx32 lenTopLeft;
    fx32 lenTopRight;
    fx32 lenBottomLeft;
    fx32 lenBottomRight;
    fx32 size;

    VEC_Subtract(start, target, &toStart);
    VEC_Subtract(end, target, &toEnd);
    VEC_Subtract(topLeft, target, &toTopLeft);
    VEC_Subtract(topRight, target, &toTopRight);
    VEC_Subtract(bottomLeft, target, &toBottomLeft);
    VEC_Subtract(bottomRight, target, &toBottomRight);
    lenStart = VEC_Mag(&toStart);
    lenEnd = VEC_Mag(&toEnd);
    lenTopLeft = VEC_Mag(&toTopLeft);
    lenTopRight = VEC_Mag(&toTopRight);
    lenBottomLeft = VEC_Mag(&toBottomLeft);
    lenBottomRight = VEC_Mag(&toBottomRight);
    size = lenStart;
    if (size < lenEnd) {
        size = lenEnd;
    }
    if (size < lenTopLeft) {
        size = lenTopLeft;
    }
    if (size < lenTopRight) {
        size = lenTopRight;
    }
    if (size < lenBottomLeft) {
        size = lenBottomLeft;
    }
    if (size < lenBottomRight) {
        size = lenBottomRight;
    }
    VEC_Set(&win->poly.origin, target->x, target->y, target->z);
    win->poly.size = size;
    VEC_Fx16Set(&win->poly.vtx[0], 0, 0, 0);
    VEC_Fx16Set(&win->poly.vtx[1], FX_Div(toStart.x, size), FX_Div(toStart.y, size), FX_Div(toStart.z, size));
    VEC_Fx16Set(&win->poly.vtx[2], FX_Div(toEnd.x, size), FX_Div(toEnd.y, size), FX_Div(toEnd.z, size));
    VEC_Fx16Set(&win->poly.vtx[3], FX_Div(toTopLeft.x, size), FX_Div(toTopLeft.y, size), FX_Div(toTopLeft.z, size));
    VEC_Fx16Set(&win->poly.vtx[4], FX_Div(toTopRight.x, size), FX_Div(toTopRight.y, size), FX_Div(toTopRight.z, size));
    VEC_Fx16Set(&win->poly.vtx[5], FX_Div(toBottomLeft.x, size), FX_Div(toBottomLeft.y, size),
                FX_Div(toBottomLeft.z, size));
    VEC_Fx16Set(&win->poly.vtx[6], FX_Div(toBottomRight.x, size), FX_Div(toBottomRight.y, size),
                FX_Div(toBottomRight.z, size));
}

static BOOL func_ov036_0218bec0(TalkMsgWinSys *sys, TalkMsgWin *win) {
    VecFx32 tip;
    VecFx32 start;
    VecFx32 end;
    VecFx32 topLeft;
    VecFx32 topRight;
    VecFx32 bottomLeft;
    VecFx32 bottomRight;
    VecFx32 target;
    int x;
    int y;
    u32 tail;

    if (win->target != NULL) {
        VEC_Subtract(win->target, &sys->camTarget, &target);
        func_ov036_0218b9fc(&target, &x, &y, &tip);
    } else {
        func_ov036_0218ba18(win, &sys->windows[win->anchorWin], &x, &y, &tip);
    }
    tail = func_ov036_0218bb0c(win, x, y, &start, &end);
    func_ov036_0218bcb0(win, &topLeft, &topRight, &bottomLeft, &bottomRight);
    func_ov036_0218bd14(sys, win, &tip, &start, &end, &topLeft, &topRight, &bottomLeft, &bottomRight);
    if (win->poly.tail != tail) {
        win->poly.tail = tail;
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0218bf5c(TalkMsgWinSys *sys, BOOL draw) {
    VecFx32 offset;
    VecFx32 pos;
    VecFx32 up;
    VecFx32 target;
    fx32 zNear;

    VEC_Subtract(&sys->camPos, &sys->camTarget, &offset);
    VEC_Set(&pos, offset.x, offset.y, offset.z);
    VEC_Set(&up, 0, FX32_ONE, 0);
    VEC_Set(&target, 0, 0, 0);
    if (draw == FALSE) {
        zNear = FX32_CONST(64);
        if (sys->camZNear >= zNear) {
            zNear = sys->camZNear + FX32_ONE;
        }
        GFL_G3DCameraSetLookatPos(sys->setup.camera, &pos);
        GFL_G3DCameraSetLookatUpVector(sys->setup.camera, &up);
        GFL_G3DCameraSetLookatTarget(sys->setup.camera, &target);
        GFL_G3DCameraSetProjectionZNear(sys->setup.camera, &zNear);
        GFL_G3DCameraFlush(sys->setup.camera);
    } else {
        G3i_LookAt_(&pos, &up, &target, TRUE, NULL);
    }
}

static void func_ov036_0218bfe4(TalkMsgWinSys *sys) {
    GFL_G3DCameraGetLookatPos(sys->setup.camera, &sys->camPos);
    GFL_G3DCameraGetLookatUpVector(sys->setup.camera, &sys->camUp);
    GFL_G3DCameraGetLookatTarget(sys->setup.camera, &sys->camTarget);
    GFL_G3DCameraGetProjectionZNear(sys->setup.camera, &sys->camZNear);
}

static void func_ov036_0218c018(TalkMsgWinSys *sys) {
    GFL_G3DCameraSetLookatPos(sys->setup.camera, &sys->camPos);
    GFL_G3DCameraSetLookatUpVector(sys->setup.camera, &sys->camUp);
    GFL_G3DCameraSetLookatTarget(sys->setup.camera, &sys->camTarget);
    GFL_G3DCameraSetProjectionZNear(sys->setup.camera, &sys->camZNear);
    GFL_G3DCameraFlush(sys->setup.camera);
}

static u32 func_ov036_0218c054(TalkMsgWinSys *sys, TalkMsgWinSetup *setup) {
    u32 palType;

    GFL_BGSysClearScr(setup->bg);
    palType = PALTYPE_MAIN_BG;
    switch (setup->bg) {
    case BGSYS_BG_SUB:
    case BGSYS_BG_SUB + 1:
    case BGSYS_BG_SUB + 2:
    case BGSYS_BG_SUB + 3:
        palType = PALTYPE_SUB_BG;
        break;
    }
    GFL_BGSysLoadNCLRDefault(ARCID_WINFRAME, 0, palType, setup->framePalette * 0x20, 0x20, setup->heapId);
    return GFL_BGSysLoadNCGRStatic(ARCID_WINFRAME, 5, setup->bg, setup->frameChar, 0, FALSE, setup->heapId);
}

static void loadTexture(TalkMsgWinSys *sys) {
    GX_BeginLoadTex();
    DC_FlushRange(TALKMSG_WIN_TEX, sizeof(TALKMSG_WIN_TEX));
    GX_LoadTex(TALKMSG_WIN_TEX, NNS_GfdGetTexKeyAddr(sys->texKey), sizeof(TALKMSG_WIN_TEX));
    GX_EndLoadTex();
    GX_BeginLoadTexPltt();
    DC_FlushRange(TALKMSG_WIN_PLT, sizeof(TALKMSG_WIN_PLT));
    GX_LoadTexPltt(TALKMSG_WIN_PLT, NNS_GfdGetPlttKeyAddr(sys->plttKey), sizeof(TALKMSG_WIN_PLT));
    GX_EndLoadTexPltt();
}

void func_ov036_0218c110(TalkMsgWinSys *sys) {
    u32 palType;

    GFL_BGSysClearScr(sys->setup.bg);
    palType = PALTYPE_MAIN_BG;
    switch (sys->setup.bg) {
    case BGSYS_BG_SUB:
    case BGSYS_BG_SUB + 1:
    case BGSYS_BG_SUB + 2:
    case BGSYS_BG_SUB + 3:
        palType = PALTYPE_SUB_BG;
        break;
    }
    GFL_BGSysLoadNCLRDefault(ARCID_WINFRAME, 0, palType, sys->setup.framePalette * 0x20, 0x20, sys->setup.heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_WINFRAME, 5, sys->setup.bg, sys->setup.frameChar, 0, FALSE, sys->setup.heapId);
}

static void func_ov036_0218c174(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 tail) {
    int x = BmpWin_GetPosX(bmpWin);
    int y = BmpWin_GetPosY(bmpWin);
    u16 width = BmpWin_GetWidth1(bmpWin);
    u16 height = BmpWin_GetHeight2(bmpWin);
    int atTop = 0;
    int atBottom = 0;
    int atLeft = 0;
    int atRight = 0;
    u8 bg = sys->setup.bg;
    u32 pal = (sys->setup.framePalette & 0xf) << 12;
    u16 base = sys->setup.frameChar;
    u16 tileTail;
    int above;
    int right;
    int bottom;

    if (x == 0) {
        atLeft = 1;
    }
    right = x + width;
    if (right == TALKMSG_WIN_SCREEN_WIDTH) {
        atRight = 1;
    }
    if (y == 0) {
        atTop = 1;
    }
    bottom = y + height;
    if (bottom == TALKMSG_WIN_SCREEN_HEIGHT) {
        atBottom = 1;
    }
    BmpWin_FlushMap(bmpWin);
    if (atTop == 0) {
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, (base + 1) | pal, x - 1, y - 1, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        above = y - 1;
        GFL_BGSysFillScrArea(bg, (base + 3) | pal, x, above, width, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, (base + 0x401) | pal, right, above, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atLeft == 0) {
        GFL_BGSysFillScrArea(bg, (base + 4) | pal, x - 1, y, 1, height, BGSYS_FILL_TILE_PALETTE);
    }
    if (atRight == 0) {
        GFL_BGSysFillScrArea(bg, (base + 0x404) | pal, right, y, 1, height, BGSYS_FILL_TILE_PALETTE);
    }
    if (atBottom == 0) {
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, (base + 0x801) | pal, x - 1, bottom, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        GFL_BGSysFillScrArea(bg, (base + 0x803) | pal, x, bottom, width, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, (base + 0xc01) | pal, right, bottom, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
    }
    tileTail = (base + 2) | pal;
    switch (tail) {
    case TALKMSG_WIN_TAIL_NONE:
        break;
    case TALKMSG_WIN_TAIL_TOP:
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail, x + width / 2 - 1, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_BOTTOM:
        if (atBottom == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + width / 2 - 1, bottom, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_LEFT:
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, (base + 5) | pal, x - 1, y + height / 2 - 1, 1, 2, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_RIGHT:
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, (base + 5) | pal, right, y + height / 2 - 1, 1, 2, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_TOP_LEFT:
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail, x + 7, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_BOTTOM_LEFT:
        // BUG: tests the top edge for a tail on the bottom edge, as TALKMSG_WIN_TAIL_TOP_RIGHT tests the bottom edge
        // for one on the top. Only the windows across the top or the bottom of the screen take these tails, and they
        // never touch its edges
#ifdef BUGFIX
        if (atBottom == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + 7, bottom, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#else
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + 7, bottom, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#endif
        break;
    case TALKMSG_WIN_TAIL_TOP_RIGHT:
        // BUG: the bottom edge, see above
#ifdef BUGFIX
        if (atTop == 0) {
            right -= 9;
            GFL_BGSysFillScrArea(bg, tileTail, right, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#else
        if (atBottom == 0) {
            right -= 9;
            GFL_BGSysFillScrArea(bg, tileTail, right, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#endif
        break;
    case TALKMSG_WIN_TAIL_BOTTOM_RIGHT:
        if (atBottom == 0) {
            right -= 9;
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, right, bottom, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    }
    GFL_BGSysLoadScr(bg);
}

static void func_ov036_0218c4c0(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 tail) {
    // The order of these declarations, with the values assigned after them, sets the locals' registers and stack
    // slots
    int atTop;
    int atBottom;
    int atLeft;
    int atRight;
    u8 bg;
    int topLeftX;
    int rightEdge;
    int topRightY;
    int bottomEdge;
    int bottomLeftX;
    int bottomRightY;
    u32 x;
    u32 width;
    u32 height;
    int y;
    u32 pal;
    u16 base;
    int topRightX;
    int bottomLeftY;
    int bottomRightX;
    int topLeftY;
    u16 tileTail;

    x = BmpWin_GetPosX(bmpWin);
    y = BmpWin_GetPosY(bmpWin);
    width = BmpWin_GetWidth1(bmpWin);
    height = BmpWin_GetHeight2(bmpWin);
    atTop = 0;
    atBottom = 0;
    atLeft = 0;
    atRight = 0;
    bg = sys->setup.bg;
    pal = (sys->setup.framePalette & 0xf) << 12;
    base = sys->setup.frameChar;

    if (x == 0) {
        atLeft = 1;
    }
    rightEdge = x + width;
    if (rightEdge == TALKMSG_WIN_SCREEN_WIDTH) {
        atRight = 1;
    }
    if (y == 0) {
        atTop = 1;
    }
    bottomEdge = y + height;
    if (bottomEdge == TALKMSG_WIN_SCREEN_HEIGHT) {
        atBottom = 1;
    }
    BmpWin_FlushMap(bmpWin);
    if (atTop == 0) {
        if (atLeft == 0) {
            topLeftX = x - 1;
            topLeftY = y - 1;
            GFL_BGSysFillScrArea(bg, (base + 9) | pal, topLeftX, topLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0xa) | pal, x, topLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0xb) | pal, topLeftX, y, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        GFL_BGSysFillScrArea(bg, (base + 6) | pal, x + 1, y - 1, width - 1, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            topRightY = y - 1;
            topRightX = x + width;
            GFL_BGSysFillScrArea(bg, (base + 0x409) | pal, topRightX, topRightY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x40a) | pal, topRightX - 1, topRightY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x40b) | pal, topRightX, y, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atLeft == 0) {
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, (base + 7) | pal, x - 1, y + 1, 1, height - 1, BGSYS_FILL_TILE_PALETTE);
        } else {
            GFL_BGSysFillScrArea(bg, (base + 7) | pal, x - 1, y, 1, height, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atRight == 0) {
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, (base + 0x407) | pal, rightEdge, y + 1, 1, height - 1, BGSYS_FILL_TILE_PALETTE);
        } else {
            GFL_BGSysFillScrArea(bg, (base + 0x407) | pal, rightEdge, y, 1, height, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atBottom == 0) {
        if (atLeft == 0) {
            bottomLeftY = y + height;
            bottomLeftX = x - 1;
            GFL_BGSysFillScrArea(bg, (base + 0x809) | pal, bottomLeftX, bottomLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x80a) | pal, x, bottomLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x80b) | pal, bottomLeftX, bottomLeftY - 1, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        GFL_BGSysFillScrArea(bg, (base + 0x806) | pal, x + 1, bottomEdge, width - 1, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            bottomRightY = y + height;
            bottomRightX = x + width;
            GFL_BGSysFillScrArea(bg, (base + 0xc09) | pal, bottomRightX, bottomRightY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0xc0a) | pal, bottomRightX - 1, bottomRightY, 1, 1,
                                 BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0xc0b) | pal, bottomRightX, bottomRightY - 1, 1, 1,
                                 BGSYS_FILL_TILE_PALETTE);
        }
    }
    tileTail = (base + 8) | pal;
    switch (tail) {
    case TALKMSG_WIN_TAIL_NONE:
        break;
    case TALKMSG_WIN_TAIL_TOP:
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail, x + width / 2 - 1, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_BOTTOM:
        if (atBottom == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + width / 2 - 1, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_LEFT:
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, (base + 5) | pal, x - 1, y + height / 2 - 1, 1, 2, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_RIGHT:
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, (base + 5) | pal, rightEdge, y + height / 2 - 1, 1, 2, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_TOP_LEFT:
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail, x + 7, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_BOTTOM_LEFT:
        // BUG: tests the top edge for a tail on the bottom edge, as TALKMSG_WIN_TAIL_TOP_RIGHT tests the bottom edge
        // for one on the top. Only the windows across the top or the bottom of the screen take these tails, and they
        // never touch its edges
#ifdef BUGFIX
        if (atBottom == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + 7, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#else
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + 7, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#endif
        break;
    case TALKMSG_WIN_TAIL_TOP_RIGHT:
        // BUG: the bottom edge, see above
#ifdef BUGFIX
        if (atTop == 0) {
            rightEdge -= 9;
            GFL_BGSysFillScrArea(bg, tileTail, rightEdge, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#else
        if (atBottom == 0) {
            rightEdge -= 9;
            GFL_BGSysFillScrArea(bg, tileTail, rightEdge, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#endif
        break;
    case TALKMSG_WIN_TAIL_BOTTOM_RIGHT:
        if (atBottom == 0) {
            rightEdge -= 9;
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, rightEdge, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    }
    GFL_BGSysLoadScr(bg);
}

static void func_ov036_0218c9dc(TalkMsgWinSys *sys, BmpWin *bmpWin, u32 tail) {
    // The order of these declarations, with the values assigned after them, sets the locals' registers and stack
    // slots
    int atTop;
    int atBottom;
    int atLeft;
    int atRight;
    u8 bg;
    int topLeftX;
    int rightEdge;
    int topRightY;
    int bottomEdge;
    int bottomLeftX;
    int bottomRightY;
    u32 x;
    u32 width;
    u32 height;
    int y;
    u32 pal;
    u16 base;
    int topRightX;
    int bottomLeftY;
    int bottomRightX;
    int topLeftY;
    u16 tileTail;

    x = BmpWin_GetPosX(bmpWin);
    y = BmpWin_GetPosY(bmpWin);
    width = BmpWin_GetWidth1(bmpWin);
    height = BmpWin_GetHeight2(bmpWin);
    atTop = 0;
    atBottom = 0;
    atLeft = 0;
    atRight = 0;
    bg = sys->setup.bg;
    pal = (sys->setup.framePalette & 0xf) << 12;
    base = sys->setup.frameChar;

    if (x == 0) {
        atLeft = 1;
    }
    rightEdge = x + width;
    if (rightEdge == TALKMSG_WIN_SCREEN_WIDTH) {
        atRight = 1;
    }
    if (y == 0) {
        atTop = 1;
    }
    bottomEdge = y + height;
    if (bottomEdge == TALKMSG_WIN_SCREEN_HEIGHT) {
        atBottom = 1;
    }
    BmpWin_FlushMap(bmpWin);
    if (atTop == 0) {
        if (atLeft == 0) {
            topLeftX = x - 1;
            topLeftY = y - 1;
            GFL_BGSysFillScrArea(bg, (base + 0xc) | pal, topLeftX, topLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x11) | pal, x, topLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x12) | pal, topLeftX, y, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        GFL_BGSysFillScrArea(bg, (base + 0xe) | pal, x + 1, y - 1, width - 1, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            topRightY = y - 1;
            topRightX = x + width;
            GFL_BGSysFillScrArea(bg, (base + 0x40c) | pal, topRightX, topRightY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x11) | pal, topRightX - 1, topRightY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x412) | pal, topRightX, y, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atLeft == 0) {
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, (base + 0xf) | pal, x - 1, y + 1, 1, height - 1, BGSYS_FILL_TILE_PALETTE);
        } else {
            GFL_BGSysFillScrArea(bg, (base + 0xf) | pal, x - 1, y, 1, height, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atRight == 0) {
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, (base + 0x40f) | pal, rightEdge, y + 1, 1, height - 1, BGSYS_FILL_TILE_PALETTE);
        } else {
            GFL_BGSysFillScrArea(bg, (base + 0x40f) | pal, rightEdge, y, 1, height, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atBottom == 0) {
        if (atLeft == 0) {
            bottomLeftY = y + height;
            bottomLeftX = x - 1;
            GFL_BGSysFillScrArea(bg, (base + 0x80c) | pal, bottomLeftX, bottomLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x811) | pal, x, bottomLeftY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x12) | pal, bottomLeftX, bottomLeftY - 1, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        GFL_BGSysFillScrArea(bg, (base + 0x80e) | pal, x + 1, bottomEdge, width - 1, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            bottomRightY = y + height;
            bottomRightX = x + width;
            GFL_BGSysFillScrArea(bg, (base + 0xc0c) | pal, bottomRightX, bottomRightY, 1, 1, BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x811) | pal, bottomRightX - 1, bottomRightY, 1, 1,
                                 BGSYS_FILL_TILE_PALETTE);
            GFL_BGSysFillScrArea(bg, (base + 0x412) | pal, bottomRightX, bottomRightY - 1, 1, 1,
                                 BGSYS_FILL_TILE_PALETTE);
        }
    }
    tileTail = (base + 0xd) | pal;
    switch (tail) {
    case TALKMSG_WIN_TAIL_NONE:
        break;
    case TALKMSG_WIN_TAIL_TOP:
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail, x + width / 2 - 1, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_BOTTOM:
        if (atBottom == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + width / 2 - 1, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_LEFT:
        // The order of the operands here and on the right sets the registers
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, pal | (base + 0x10), x - 1, y + height / 2 - 1, 1, 2, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_RIGHT:
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, pal | (base + 0x10), rightEdge, y + height / 2 - 1, 1, 2, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_TOP_LEFT:
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail, x + 7, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    case TALKMSG_WIN_TAIL_BOTTOM_LEFT:
        // BUG: tests the top edge for a tail on the bottom edge, as TALKMSG_WIN_TAIL_TOP_RIGHT tests the bottom edge
        // for one on the top. Only the windows across the top or the bottom of the screen take these tails, and they
        // never touch its edges
#ifdef BUGFIX
        if (atBottom == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + 7, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#else
        if (atTop == 0) {
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, x + 7, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#endif
        break;
    case TALKMSG_WIN_TAIL_TOP_RIGHT:
        // BUG: the bottom edge, see above
#ifdef BUGFIX
        if (atTop == 0) {
            rightEdge -= 9;
            GFL_BGSysFillScrArea(bg, tileTail, rightEdge, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#else
        if (atBottom == 0) {
            rightEdge -= 9;
            GFL_BGSysFillScrArea(bg, tileTail, rightEdge, y - 1, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
#endif
        break;
    case TALKMSG_WIN_TAIL_BOTTOM_RIGHT:
        if (atBottom == 0) {
            rightEdge -= 9;
            GFL_BGSysFillScrArea(bg, tileTail | 0x800, rightEdge, bottomEdge, 2, 1, BGSYS_FILL_TILE_PALETTE);
        }
        break;
    }
    GFL_BGSysLoadScr(bg);
}

static void func_ov036_0218cf00(TalkMsgWinSys *sys, BmpWin *bmpWin, u8 frameType, u32 tail) {
    switch (frameType) {
    case TALKMSG_WIN_FRAME_NORMAL:
        func_ov036_0218c174(sys, bmpWin, tail);
        break;
    case TALKMSG_WIN_FRAME_SPIKY:
        func_ov036_0218c4c0(sys, bmpWin, tail);
        break;
    case TALKMSG_WIN_FRAME_BUMPY:
        func_ov036_0218c9dc(sys, bmpWin, tail);
        break;
    }
}

static void func_ov036_0218cf28(TalkMsgWinSys *sys, TalkMsgWin *win) {
    func_ov036_0218cf00(sys, win->bmpWin, win->frameType, TALKMSG_WIN_TAIL_NONE);
}

static void func_ov036_0218cf38(TalkMsgWinSys *sys, TalkMsgWin *win) {
    func_ov036_0218cf00(sys, win->bmpWin, win->frameType, win->poly.tail);
}

static void func_ov036_0218cf48(TalkMsgWinSys *sys, BmpWin *bmpWin) {
    // The order of these declarations sets the locals' registers
    int y;
    int x;
    int width;
    int height;
    int atTop;
    int atBottom;
    int atLeft;
    int above;
    int right;
    int bottom;
    int atRight;
    u8 bg;

    x = BmpWin_GetPosX(bmpWin);
    y = BmpWin_GetPosY(bmpWin);
    width = BmpWin_GetWidth1(bmpWin);
    height = BmpWin_GetHeight2(bmpWin);
    atTop = 0;
    atBottom = 0;
    atLeft = 0;
    atRight = 0;
    bg = sys->setup.bg;
    BmpWin_ClearScreen(bmpWin);
    if (x == 0) {
        atLeft = 1;
    }
    right = x + width;
    if (right == TALKMSG_WIN_SCREEN_WIDTH) {
        atRight = 1;
    }
    if (y == 0) {
        atTop = 1;
    }
    bottom = y + height;
    if (bottom == TALKMSG_WIN_SCREEN_HEIGHT) {
        atBottom = 1;
    }
    if (atTop == 0) {
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, 0, x - 1, y - 1, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        above = y - 1;
        GFL_BGSysFillScrArea(bg, 0, x, above, width, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, 0, right, above, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
    }
    if (atLeft == 0) {
        GFL_BGSysFillScrArea(bg, 0, x - 1, y, 1, height, BGSYS_FILL_TILE_PALETTE);
    }
    if (atRight == 0) {
        GFL_BGSysFillScrArea(bg, 0, right, y, 1, height, BGSYS_FILL_TILE_PALETTE);
    }
    if (atBottom == 0) {
        if (atLeft == 0) {
            GFL_BGSysFillScrArea(bg, 0, x - 1, bottom, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
        GFL_BGSysFillScrArea(bg, 0, x, bottom, width, 1, BGSYS_FILL_TILE_PALETTE);
        if (atRight == 0) {
            GFL_BGSysFillScrArea(bg, 0, right, bottom, 1, 1, BGSYS_FILL_TILE_PALETTE);
        }
    }
    GFL_BGSysLoadScr(bg);
}
