#include "types.h"
#include "app/box2_ui.h"
#include "app/box2_bgwfrm.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/cursor_move.h"

// The PC box's cursor: the positions it moves between in each of the box's modes and what moving it and touching a
// position do, and the touch screen's areas. The ROM names this file in its asserts. Names are ours

// A cursor's positions and callbacks
typedef struct {
    const CursorMoveData *data;
    const CursorMoveCallbacks *callbacks;
} Box2CursorMoveTable;

static void func_ov255_021d251c(Box2SysWork *syswk, u32 pos);
static void func_ov255_021d252c(void *work, int pos, int prevPos);
static void func_ov255_021d2540(void *work, int pos, int prevPos);
static void func_ov255_021d2550(void *work, int pos, int prevPos);
static void func_ov255_021d2568(void *work, int pos, int prevPos);
static void func_ov255_021d2584(void *work, int pos, int prevPos);
static void func_ov255_021d2624(void *work, int pos, int prevPos);
static void func_ov255_021d2760(void *work, int pos, int prevPos);
static void func_ov255_021d2848(void *work, int pos, int prevPos);
static void func_ov255_021d286c(void *work, int pos, int prevPos);
static void func_ov255_021d2954(void *work, int pos, int prevPos);
static void func_ov255_021d2984(void *work, int pos, int prevPos);
static void func_ov255_021d2b0c(void *work, int pos, int prevPos);
static void func_ov255_021d2b30(void *work, int pos, int prevPos);
static void func_ov255_021d2bf0(void *work, int pos, int prevPos);
static void func_ov255_021d2c20(void *work, int pos, int prevPos);
static void func_ov255_021d2d4c(void *work, int pos, int prevPos);
static void func_ov255_021d2d70(void *work, int pos, int prevPos);
static void func_ov255_021d2e48(void *work, int pos, int prevPos);
static void func_ov255_021d2e6c(void *work, int pos, int prevPos);
static void func_ov255_021d2ea8(void *work, int pos, int prevPos);
static void func_ov255_021d2ec0(void *work, int pos, int prevPos);
static void func_ov255_021d2edc(void *work, int pos, int prevPos);
static void func_ov255_021d2ef4(void *work, int pos, int prevPos);
static void func_ov255_021d2f10(void *work, int pos, int prevPos);
static void func_ov255_021d2f54(void *work, int pos, int prevPos);
static void func_ov255_021d3064(void *work, int pos, int prevPos);
static void func_ov255_021d3088(void *work, int pos, int prevPos);
static void func_ov255_021d3188(void *work, int pos, int prevPos);
static void func_ov255_021d31ac(void *work, int pos, int prevPos);
static void func_ov255_021d32b0(void *work, int pos, int prevPos);
static void func_ov255_021d32c8(void *work, int pos, int prevPos);
static void func_ov255_021d3304(Box2AppWork *app, u8 fromX, u8 fromY, u8 toX, u8 toY);
static u32 func_ov255_021d33a8(const CursorMoveData *data, u32 count);
static u32 func_ov255_021d3408(const CursorMoveData *data, u32 count);
static u32 func_ov255_021d3468(const CursorMoveData *data, u32 count, u32 x, u32 y);

// MWCC sorts a file's data by size, in an order that depends on where each object is declared; the objects of the same
// size are declared in the order that lays them out as in the ROM (tools/decomp/rodata_order.py)

// The end of a table of touch rectangles
static const TouchRect sTouchRectEnd = { TOUCH_RECT_END, 0, 0, 0 };

static const TouchRect sTouchRects_78cc[2] = {
    { 0, 191, 168, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTouchRects_78d4[2] = {
    { 48, 167, 16, 103 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTouchRects_78c4[2] = {
    { 40, 160, 0, 167 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTouchRects_78dc[2] = {
    { 48, 167, 168, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTouchRects_78f0[3] = {
    { 0, 167, 228, 255 }, { 0, 167, 168, 195 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTouchRects_78e4[3] = {
    { 0, 21, 173, 255 }, { 148, 167, 173, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const CursorMoveCallbacks sCursorCallbacks10 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2ea8, func_ov255_021d2ec0,
};

static const CursorMoveCallbacks sCursorCallbacks12 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2f10, func_ov255_021d2f54,
};

static const CursorMoveCallbacks sCursorCallbacks13 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d3064, func_ov255_021d3088,
};

static const CursorMoveCallbacks sCursorCallbacks5 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2b0c, func_ov255_021d2b30,
};

static const CursorMoveCallbacks sCursorCallbacks3 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2848, func_ov255_021d286c,
};

static const CursorMoveCallbacks sCursorCallbacks2 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2550, func_ov255_021d2760,
};

static const CursorMoveCallbacks sCursorCallbacks4 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2954, func_ov255_021d2984,
};

static const CursorMoveCallbacks sCursorCallbacks6 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2bf0, func_ov255_021d2c20,
};

static const CursorMoveCallbacks sCursorCallbacks14 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d3188, func_ov255_021d31ac,
};

static const CursorMoveCallbacks sCursorCallbacks7 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2d4c, func_ov255_021d2d70,
};

static const CursorMoveCallbacks sCursorCallbacks11 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2edc, func_ov255_021d2ef4,
};

static const CursorMoveCallbacks sCursorCallbacks15 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d32b0, func_ov255_021d32c8,
};

static const CursorMoveCallbacks sCursorCallbacks8 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2e48, func_ov255_021d2e6c,
};

static const CursorMoveCallbacks sCursorCallbacks1 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2584, func_ov255_021d2584,
};

static const CursorMoveCallbacks sCursorCallbacks9 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2550, func_ov255_021d2568,
};

static const CursorMoveCallbacks sCursorCallbacks0 = {
    func_ov255_021d252c, func_ov255_021d2540, func_ov255_021d2550, func_ov255_021d2624,
};

static const TouchRect sTouchRects_79fc[5] = {
    { 20, 50, 180, 254 }, { 51, 84, 180, 254 }, { 85, 119, 180, 254 }, { 120, 150, 180, 254 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const CursorMoveData sCursorData10[5] = {
    { 212, 60, 0, 0, 3, 1, 0, 0, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 0, 2, 1, 1, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 1, 3, 2, 2, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 2, 0, 3, 3, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData1[6] = {
    { 84, 16, 0, 0, 0, 0, 0, 0, { 18, 38, 27, 140 } },
    { 0, 0, 0, 0, 1, 1, 1, 1, { 51, 172, 3, 164 } },
    { 0, 0, 0, 0, 2, 2, 2, 2, { 18, 38, 6, 26 } },
    { 0, 0, 0, 0, 3, 3, 3, 3, { 18, 38, 141, 161 } },
    { 0, 0, 0, 0, 4, 4, 4, 4, { 168, 191, 232, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData11[7] = {
    { 212, 12, 0, 0, 5, 1, 0, 0, { 16, 39, 168, 255 } },
    { 212, 36, 0, 0, 0, 2, 1, 1, { 40, 63, 168, 255 } },
    { 212, 60, 0, 0, 1, 3, 2, 2, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 2, 4, 3, 3, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 3, 5, 4, 4, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 4, 0, 5, 5, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData12[8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, { 0, 11, 200, 223 } },
    { 212, 22, 0, 0, 1, 2, 1, 1, { 22, 45, 200, 223 } },
    { 212, 56, 0, 0, 1, 3, 2, 2, { 56, 79, 200, 223 } },
    { 212, 90, 0, 0, 2, 4, 3, 3, { 90, 113, 200, 223 } },
    { 212, 124, 0, 0, 3, 4, 4, 4, { 124, 147, 200, 223 } },
    { 0, 0, 0, 0, 5, 5, 5, 5, { 158, 167, 200, 223 } },
    { 0, 0, 0, 0, 6, 6, 6, 6, { 168, 191, 232, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData9[9] = {
    { 200, 36, 0, 0, 0, 2, 0, 1, { 44, 59, 188, 203 } },
    { 232, 36, 0, 0, 1, 3, 0, 1, { 44, 59, 220, 235 } },
    { 200, 60, 0, 0, 0, 4, 2, 3, { 68, 83, 188, 203 } },
    { 232, 60, 0, 0, 1, 5, 2, 3, { 68, 83, 220, 235 } },
    { 200, 84, 0, 0, 2, 6, 4, 5, { 92, 107, 188, 203 } },
    { 232, 84, 0, 0, 3, 6, 4, 5, { 92, 107, 220, 235 } },
    { 212, 108, 0, 0, 132, 7, 6, 6, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 6, 7, 7, 7, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData0[14] = {
    { 40, 52, 0, 0, 4, 2, 5, 1, { 56, 79, 30, 53 } },
    { 80, 60, 0, 0, 5, 3, 0, 2, { 64, 87, 66, 89 } },
    { 40, 84, 0, 0, 0, 4, 1, 3, { 88, 111, 30, 53 } },
    { 80, 92, 0, 0, 1, 5, 2, 4, { 96, 119, 66, 89 } },
    { 40, 116, 0, 0, 2, 0, 3, 5, { 120, 143, 30, 53 } },
    { 80, 124, 0, 0, 3, 1, 4, 0, { 128, 151, 66, 89 } },
    { 0, 0, 0, 0, 6, 6, 6, 6, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 7, 7, 7, 7, { 168, 191, 232, 255 } },
    { 212, 36, 0, 0, 12, 9, 8, 8, { 40, 63, 168, 255 } },
    { 212, 60, 0, 0, 8, 10, 9, 9, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 9, 11, 10, 10, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 10, 12, 11, 11, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 11, 8, 12, 12, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData8[15] = {
    { 40, 52, 0, 0, 6, 2, 0, 1, { 56, 79, 30, 53 } },
    { 80, 60, 0, 0, 6, 3, 0, 2, { 64, 87, 66, 89 } },
    { 40, 84, 0, 0, 0, 4, 1, 3, { 88, 111, 30, 53 } },
    { 80, 92, 0, 0, 1, 5, 2, 4, { 96, 119, 66, 89 } },
    { 40, 116, 0, 0, 2, 6, 3, 5, { 120, 143, 30, 53 } },
    { 80, 124, 0, 0, 3, 6, 4, 6, { 128, 151, 66, 89 } },
    { 44, 168, 0, 0, 133, 0, 5, 0, { 168, 191, 0, 95 } },
    { 0, 0, 0, 0, 7, 7, 7, 7, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 8, 8, 8, 8, { 168, 191, 232, 255 } },
    { 212, 84, 0, 0, 11, 10, 9, 9, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 9, 11, 10, 10, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 10, 9, 11, 11, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 12, 12, 12, 12, { 0, 10, 8, 59 } },
    { 0, 0, 0, 0, 12, 13, 13, 13, { 0, 10, 60, 109 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData14[18] = {
    { 40, 52, 0, 0, 6, 2, 0, 1, { 56, 79, 30, 53 } },
    { 80, 60, 0, 0, 6, 3, 0, 2, { 64, 87, 66, 89 } },
    { 40, 84, 0, 0, 0, 4, 1, 3, { 88, 111, 30, 53 } },
    { 80, 92, 0, 0, 1, 5, 2, 4, { 96, 119, 66, 89 } },
    { 40, 116, 0, 0, 2, 6, 3, 5, { 120, 143, 30, 53 } },
    { 80, 124, 0, 0, 3, 6, 4, 6, { 128, 151, 66, 89 } },
    { 44, 168, 0, 0, 133, 0, 5, 0, { 168, 191, 0, 95 } },
    { 0, 0, 0, 0, 7, 7, 7, 7, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 8, 8, 8, 8, { 168, 191, 232, 255 } },
    { 212, 60, 0, 0, 12, 10, 9, 9, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 9, 11, 10, 10, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 10, 12, 11, 11, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 11, 9, 12, 12, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 13, 13, 13, 13, { 0, 10, 8, 59 } },
    { 0, 0, 0, 0, 14, 14, 14, 14, { 0, 10, 60, 109 } },
    { 0, 0, 0, 0, 15, 15, 15, 15, { 0, 10, 110, 159 } },
    { 0, 0, 0, 0, 16, 16, 16, 16, { 168, 191, 168, 191 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData5[20] = {
    { 40, 52, 0, 0, 6, 2, 0, 1, { 56, 79, 30, 53 } },
    { 80, 60, 0, 0, 6, 3, 0, 2, { 64, 87, 66, 89 } },
    { 40, 84, 0, 0, 0, 4, 1, 3, { 88, 111, 30, 53 } },
    { 80, 92, 0, 0, 1, 5, 2, 4, { 96, 119, 66, 89 } },
    { 40, 116, 0, 0, 2, 6, 3, 5, { 120, 143, 30, 53 } },
    { 80, 124, 0, 0, 3, 6, 4, 6, { 128, 151, 66, 89 } },
    { 44, 168, 0, 0, 133, 0, 5, 0, { 168, 191, 0, 95 } },
    { 0, 0, 0, 0, 7, 7, 7, 7, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 8, 8, 8, 8, { 168, 191, 232, 255 } },
    { 212, 12, 0, 0, 14, 10, 9, 9, { 16, 39, 168, 255 } },
    { 212, 36, 0, 0, 9, 11, 10, 10, { 40, 63, 168, 255 } },
    { 212, 60, 0, 0, 10, 12, 11, 11, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 11, 13, 12, 12, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 12, 14, 13, 13, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 13, 9, 14, 14, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 15, 15, 15, 15, { 0, 10, 8, 59 } },
    { 0, 0, 0, 0, 16, 16, 16, 16, { 0, 10, 60, 109 } },
    { 0, 0, 0, 0, 17, 17, 17, 17, { 0, 10, 110, 159 } },
    { 0, 0, 0, 0, 18, 18, 18, 18, { 168, 191, 168, 191 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData15[37] = {
    { 24, 36, 0, 0, 30, 6, 5, 1, { 40, 63, 12, 35 } },
    { 48, 36, 0, 0, 30, 7, 0, 2, { 40, 63, 36, 59 } },
    { 72, 36, 0, 0, 30, 8, 1, 3, { 40, 63, 60, 83 } },
    { 96, 36, 0, 0, 30, 9, 2, 4, { 40, 63, 84, 107 } },
    { 120, 36, 0, 0, 30, 10, 3, 5, { 40, 63, 108, 131 } },
    { 144, 36, 0, 0, 30, 11, 4, 0, { 40, 63, 132, 155 } },
    { 24, 60, 0, 0, 0, 12, 11, 7, { 64, 87, 12, 35 } },
    { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
    { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
    { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
    { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
    { 144, 60, 0, 0, 5, 17, 10, 6, { 64, 87, 132, 155 } },
    { 24, 84, 0, 0, 6, 18, 17, 13, { 88, 111, 12, 35 } },
    { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
    { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
    { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
    { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
    { 144, 84, 0, 0, 11, 23, 16, 12, { 88, 111, 132, 155 } },
    { 24, 108, 0, 0, 12, 24, 23, 19, { 112, 135, 12, 35 } },
    { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
    { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
    { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
    { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
    { 144, 108, 0, 0, 17, 29, 22, 18, { 112, 135, 132, 155 } },
    { 24, 132, 0, 0, 18, 30, 29, 25, { 136, 159, 12, 35 } },
    { 48, 132, 0, 0, 19, 30, 24, 26, { 136, 159, 36, 59 } },
    { 72, 132, 0, 0, 20, 30, 25, 27, { 136, 159, 60, 83 } },
    { 96, 132, 0, 0, 21, 30, 26, 28, { 136, 159, 84, 107 } },
    { 120, 132, 0, 0, 22, 30, 27, 29, { 136, 159, 108, 131 } },
    { 144, 132, 0, 0, 23, 30, 28, 24, { 136, 159, 132, 155 } },
    { 84, 16, 0, 0, 24, 128, 30, 30, { 18, 38, 27, 140 } },
    { 0, 0, 0, 0, 31, 31, 31, 31, { 18, 38, 6, 26 } },
    { 0, 0, 0, 0, 32, 32, 32, 32, { 18, 38, 141, 161 } },
    { 0, 0, 0, 0, 33, 33, 33, 33, { 168, 191, 232, 255 } },
    { 212, 108, 0, 0, 35, 35, 34, 34, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 34, 34, 35, 35, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData2[41] = {
    { 24, 36, 0, 0, 30, 6, 5, 1, { 40, 63, 12, 35 } },
    { 48, 36, 0, 0, 30, 7, 0, 2, { 40, 63, 36, 59 } },
    { 72, 36, 0, 0, 30, 8, 1, 3, { 40, 63, 60, 83 } },
    { 96, 36, 0, 0, 30, 9, 2, 4, { 40, 63, 84, 107 } },
    { 120, 36, 0, 0, 30, 10, 3, 5, { 40, 63, 108, 131 } },
    { 144, 36, 0, 0, 30, 11, 4, 0, { 40, 63, 132, 155 } },
    { 24, 60, 0, 0, 0, 12, 11, 7, { 64, 87, 12, 35 } },
    { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
    { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
    { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
    { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
    { 144, 60, 0, 0, 5, 17, 10, 6, { 64, 87, 132, 155 } },
    { 24, 84, 0, 0, 6, 18, 17, 13, { 88, 111, 12, 35 } },
    { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
    { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
    { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
    { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
    { 144, 84, 0, 0, 11, 23, 16, 12, { 88, 111, 132, 155 } },
    { 24, 108, 0, 0, 12, 24, 23, 19, { 112, 135, 12, 35 } },
    { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
    { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
    { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
    { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
    { 144, 108, 0, 0, 17, 29, 22, 18, { 112, 135, 132, 155 } },
    { 24, 132, 0, 0, 18, 30, 29, 25, { 136, 159, 12, 35 } },
    { 48, 132, 0, 0, 19, 30, 24, 26, { 136, 159, 36, 59 } },
    { 72, 132, 0, 0, 20, 30, 25, 27, { 136, 159, 60, 83 } },
    { 96, 132, 0, 0, 21, 30, 26, 28, { 136, 159, 84, 107 } },
    { 120, 132, 0, 0, 22, 30, 27, 29, { 136, 159, 108, 131 } },
    { 144, 132, 0, 0, 23, 30, 28, 24, { 136, 159, 132, 155 } },
    { 84, 16, 0, 0, 24, 128, 30, 30, { 18, 38, 27, 140 } },
    { 0, 0, 0, 0, 31, 31, 31, 31, { 18, 38, 6, 26 } },
    { 0, 0, 0, 0, 32, 32, 32, 32, { 18, 38, 141, 161 } },
    { 0, 0, 0, 0, 33, 33, 33, 33, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 34, 34, 34, 34, { 168, 191, 232, 255 } },
    { 212, 36, 0, 0, 39, 36, 35, 35, { 40, 63, 168, 255 } },
    { 212, 60, 0, 0, 35, 37, 36, 36, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 36, 38, 37, 37, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 37, 39, 38, 38, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 38, 35, 39, 39, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData7[42] = {
    { 24, 36, 0, 0, 30, 6, 5, 1, { 40, 63, 12, 35 } },
    { 48, 36, 0, 0, 30, 7, 0, 2, { 40, 63, 36, 59 } },
    { 72, 36, 0, 0, 30, 8, 1, 3, { 40, 63, 60, 83 } },
    { 96, 36, 0, 0, 30, 9, 2, 4, { 40, 63, 84, 107 } },
    { 120, 36, 0, 0, 30, 10, 3, 5, { 40, 63, 108, 131 } },
    { 144, 36, 0, 0, 30, 11, 4, 0, { 40, 63, 132, 155 } },
    { 24, 60, 0, 0, 0, 12, 11, 7, { 64, 87, 12, 35 } },
    { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
    { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
    { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
    { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
    { 144, 60, 0, 0, 5, 17, 10, 6, { 64, 87, 132, 155 } },
    { 24, 84, 0, 0, 6, 18, 17, 13, { 88, 111, 12, 35 } },
    { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
    { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
    { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
    { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
    { 144, 84, 0, 0, 11, 23, 16, 12, { 88, 111, 132, 155 } },
    { 24, 108, 0, 0, 12, 24, 23, 19, { 112, 135, 12, 35 } },
    { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
    { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
    { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
    { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
    { 144, 108, 0, 0, 17, 29, 22, 18, { 112, 135, 132, 155 } },
    { 24, 132, 0, 0, 18, 161, 29, 25, { 136, 159, 12, 35 } },
    { 48, 132, 0, 0, 19, 161, 24, 26, { 136, 159, 36, 59 } },
    { 72, 132, 0, 0, 20, 161, 25, 27, { 136, 159, 60, 83 } },
    { 96, 132, 0, 0, 21, 161, 26, 28, { 136, 159, 84, 107 } },
    { 120, 132, 0, 0, 22, 161, 27, 29, { 136, 159, 108, 131 } },
    { 144, 132, 0, 0, 23, 161, 28, 24, { 136, 159, 132, 155 } },
    { 84, 16, 0, 0, 33, 128, 30, 30, { 18, 38, 27, 140 } },
    { 0, 0, 0, 0, 31, 31, 31, 31, { 18, 38, 6, 26 } },
    { 0, 0, 0, 0, 32, 32, 32, 32, { 18, 38, 141, 161 } },
    { 44, 168, 0, 0, 152, 30, 33, 33, { 168, 191, 0, 95 } },
    { 0, 0, 0, 0, 34, 34, 34, 34, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 35, 35, 35, 35, { 168, 191, 232, 255 } },
    { 212, 84, 0, 0, 38, 37, 36, 36, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 36, 38, 37, 37, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 37, 36, 38, 38, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 39, 39, 39, 39, { 0, 10, 8, 59 } },
    { 0, 0, 0, 0, 40, 40, 40, 40, { 0, 10, 60, 109 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveData sCursorData13[45] = {
    { 24, 36, 0, 0, 30, 6, 5, 1, { 40, 63, 12, 35 } },
    { 48, 36, 0, 0, 30, 7, 0, 2, { 40, 63, 36, 59 } },
    { 72, 36, 0, 0, 30, 8, 1, 3, { 40, 63, 60, 83 } },
    { 96, 36, 0, 0, 30, 9, 2, 4, { 40, 63, 84, 107 } },
    { 120, 36, 0, 0, 30, 10, 3, 5, { 40, 63, 108, 131 } },
    { 144, 36, 0, 0, 30, 11, 4, 0, { 40, 63, 132, 155 } },
    { 24, 60, 0, 0, 0, 12, 11, 7, { 64, 87, 12, 35 } },
    { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
    { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
    { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
    { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
    { 144, 60, 0, 0, 5, 17, 10, 6, { 64, 87, 132, 155 } },
    { 24, 84, 0, 0, 6, 18, 17, 13, { 88, 111, 12, 35 } },
    { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
    { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
    { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
    { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
    { 144, 84, 0, 0, 11, 23, 16, 12, { 88, 111, 132, 155 } },
    { 24, 108, 0, 0, 12, 24, 23, 19, { 112, 135, 12, 35 } },
    { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
    { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
    { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
    { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
    { 144, 108, 0, 0, 17, 29, 22, 18, { 112, 135, 132, 155 } },
    { 24, 132, 0, 0, 18, 33, 29, 25, { 136, 159, 12, 35 } },
    { 48, 132, 0, 0, 19, 33, 24, 26, { 136, 159, 36, 59 } },
    { 72, 132, 0, 0, 20, 33, 25, 27, { 136, 159, 60, 83 } },
    { 96, 132, 0, 0, 21, 33, 26, 28, { 136, 159, 84, 107 } },
    { 120, 132, 0, 0, 22, 33, 27, 29, { 136, 159, 108, 131 } },
    { 144, 132, 0, 0, 23, 33, 28, 24, { 136, 159, 132, 155 } },
    { 84, 16, 0, 0, 33, 128, 30, 30, { 18, 38, 27, 140 } },
    { 0, 0, 0, 0, 31, 31, 31, 31, { 18, 38, 6, 26 } },
    { 0, 0, 0, 0, 32, 32, 32, 32, { 18, 38, 141, 161 } },
    { 44, 168, 0, 0, 152, 30, 33, 33, { 168, 191, 0, 95 } },
    { 0, 0, 0, 0, 34, 34, 34, 34, { 168, 191, 200, 223 } },
    { 0, 0, 0, 0, 35, 35, 35, 35, { 168, 191, 232, 255 } },
    { 212, 60, 0, 0, 39, 37, 36, 36, { 64, 87, 168, 255 } },
    { 212, 84, 0, 0, 36, 38, 37, 37, { 88, 111, 168, 255 } },
    { 212, 108, 0, 0, 37, 39, 38, 38, { 112, 135, 168, 255 } },
    { 212, 132, 0, 0, 38, 36, 39, 39, { 136, 159, 168, 255 } },
    { 0, 0, 0, 0, 40, 40, 40, 40, { 0, 10, 8, 59 } },
    { 0, 0, 0, 0, 41, 41, 41, 41, { 0, 10, 60, 109 } },
    { 0, 0, 0, 0, 42, 42, 42, 42, { 0, 10, 110, 159 } },
    { 0, 0, 0, 0, 43, 43, 43, 43, { 168, 191, 168, 191 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

// The positions of cursors 3, 4 and 6, which share the tray's layout. Each ends with an empty position, and
// some functions use the positions after the tray's by themselves
static const CursorMoveData sTrayCursorData[3][47] = {
    {
        { 24, 36, 0, 0, 30, 6, 5, 1, { 40, 63, 12, 35 } },
        { 48, 36, 0, 0, 30, 7, 0, 2, { 40, 63, 36, 59 } },
        { 72, 36, 0, 0, 30, 8, 1, 3, { 40, 63, 60, 83 } },
        { 96, 36, 0, 0, 30, 9, 2, 4, { 40, 63, 84, 107 } },
        { 120, 36, 0, 0, 30, 10, 3, 5, { 40, 63, 108, 131 } },
        { 144, 36, 0, 0, 30, 11, 4, 0, { 40, 63, 132, 155 } },
        { 24, 60, 0, 0, 0, 12, 11, 7, { 64, 87, 12, 35 } },
        { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
        { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
        { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
        { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
        { 144, 60, 0, 0, 5, 17, 10, 6, { 64, 87, 132, 155 } },
        { 24, 84, 0, 0, 6, 18, 17, 13, { 88, 111, 12, 35 } },
        { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
        { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
        { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
        { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
        { 144, 84, 0, 0, 11, 23, 16, 12, { 88, 111, 132, 155 } },
        { 24, 108, 0, 0, 12, 24, 23, 19, { 112, 135, 12, 35 } },
        { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
        { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
        { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
        { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
        { 144, 108, 0, 0, 17, 29, 22, 18, { 112, 135, 132, 155 } },
        { 24, 132, 0, 0, 18, 33, 29, 25, { 136, 159, 12, 35 } },
        { 48, 132, 0, 0, 19, 33, 24, 26, { 136, 159, 36, 59 } },
        { 72, 132, 0, 0, 20, 33, 25, 27, { 136, 159, 60, 83 } },
        { 96, 132, 0, 0, 21, 33, 26, 28, { 136, 159, 84, 107 } },
        { 120, 132, 0, 0, 22, 33, 27, 29, { 136, 159, 108, 131 } },
        { 144, 132, 0, 0, 23, 33, 28, 24, { 136, 159, 132, 155 } },
        { 84, 16, 0, 0, 33, 128, 30, 30, { 18, 38, 27, 140 } },
        { 0, 0, 0, 0, 31, 31, 31, 31, { 18, 38, 6, 26 } },
        { 0, 0, 0, 0, 32, 32, 32, 32, { 18, 38, 141, 161 } },
        { 44, 168, 0, 0, 152, 30, 33, 33, { 168, 191, 0, 95 } },
        { 0, 0, 0, 0, 34, 34, 34, 34, { 168, 191, 200, 223 } },
        { 0, 0, 0, 0, 35, 35, 35, 35, { 168, 191, 232, 255 } },
        { 212, 12, 0, 0, 41, 37, 36, 36, { 16, 39, 168, 255 } },
        { 212, 36, 0, 0, 36, 38, 37, 37, { 40, 63, 168, 255 } },
        { 212, 60, 0, 0, 37, 39, 38, 38, { 64, 87, 168, 255 } },
        { 212, 84, 0, 0, 38, 40, 39, 39, { 88, 111, 168, 255 } },
        { 212, 108, 0, 0, 39, 41, 40, 40, { 112, 135, 168, 255 } },
        { 212, 132, 0, 0, 40, 36, 41, 41, { 136, 159, 168, 255 } },
        { 0, 0, 0, 0, 42, 42, 42, 42, { 0, 8, 8, 56 } },
        { 0, 0, 0, 0, 43, 43, 43, 43, { 0, 8, 64, 112 } },
        { 0, 0, 0, 0, 44, 44, 44, 44, { 0, 8, 120, 168 } },
        { 0, 0, 0, 0, 45, 45, 45, 45, { 168, 191, 168, 191 } },
        { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
    },
    {
        { 24, 36, 0, 0, 30, 6, 34, 1, { 40, 63, 12, 35 } },
        { 48, 36, 0, 0, 30, 7, 0, 2, { 40, 63, 36, 59 } },
        { 72, 36, 0, 0, 30, 8, 1, 3, { 40, 63, 60, 83 } },
        { 96, 36, 0, 0, 30, 9, 2, 4, { 40, 63, 84, 107 } },
        { 120, 36, 0, 0, 30, 10, 3, 5, { 40, 63, 108, 131 } },
        { 144, 36, 0, 0, 30, 11, 4, 162, { 40, 63, 132, 155 } },
        { 24, 60, 0, 0, 0, 12, 35, 7, { 64, 87, 12, 35 } },
        { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
        { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
        { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
        { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
        { 144, 60, 0, 0, 5, 17, 10, 163, { 64, 87, 132, 155 } },
        { 24, 84, 0, 0, 6, 18, 36, 13, { 88, 111, 12, 35 } },
        { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
        { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
        { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
        { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
        { 144, 84, 0, 0, 11, 23, 16, 164, { 88, 111, 132, 155 } },
        { 24, 108, 0, 0, 12, 24, 37, 19, { 112, 135, 12, 35 } },
        { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
        { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
        { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
        { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
        { 144, 108, 0, 0, 17, 29, 22, 165, { 112, 135, 132, 155 } },
        { 24, 132, 0, 0, 18, 39, 37, 25, { 136, 159, 12, 35 } },
        { 48, 132, 0, 0, 19, 39, 24, 26, { 136, 159, 36, 59 } },
        { 72, 132, 0, 0, 20, 39, 25, 27, { 136, 159, 60, 83 } },
        { 96, 132, 0, 0, 21, 39, 26, 28, { 136, 159, 84, 107 } },
        { 120, 132, 0, 0, 22, 39, 27, 29, { 136, 159, 108, 131 } },
        { 144, 132, 0, 0, 23, 39, 28, 165, { 136, 159, 132, 155 } },
        { 84, 16, 0, 0, 39, 128, 30, 30, { 18, 38, 27, 140 } },
        { 0, 0, 0, 0, 31, 31, 31, 31, { 18, 38, 6, 26 } },
        { 0, 0, 0, 0, 32, 32, 32, 32, { 18, 38, 141, 161 } },
        { 0, 0, 0, 0, 33, 33, 161, 33, { 0, 11, 200, 223 } },
        { 212, 22, 0, 0, 34, 35, 133, 0, { 22, 45, 200, 223 } },
        { 212, 56, 0, 0, 34, 36, 139, 6, { 56, 79, 200, 223 } },
        { 212, 90, 0, 0, 35, 37, 145, 12, { 90, 113, 200, 223 } },
        { 212, 124, 0, 0, 36, 37, 151, 18, { 124, 147, 200, 223 } },
        { 0, 0, 0, 0, 38, 38, 166, 38, { 158, 167, 200, 223 } },
        { 44, 168, 0, 0, 152, 30, 39, 39, { 168, 191, 0, 87 } },
        { 0, 0, 0, 0, 40, 40, 40, 40, { 168, 191, 112, 135 } },
        { 0, 0, 0, 0, 41, 41, 41, 41, { 168, 191, 232, 255 } },
        { 0, 0, 0, 0, 42, 42, 42, 42, { 0, 10, 8, 59 } },
        { 0, 0, 0, 0, 43, 43, 43, 43, { 0, 10, 60, 109 } },
        { 0, 0, 0, 0, 44, 44, 44, 44, { 0, 10, 110, 159 } },
        { 0, 0, 0, 0, 45, 45, 45, 45, { 168, 191, 168, 191 } },
        { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
    },
    {
        { 24, 36, 0, 0, 36, 6, 31, 1, { 40, 63, 12, 35 } },
        { 48, 36, 0, 0, 36, 7, 0, 2, { 40, 63, 36, 59 } },
        { 72, 36, 0, 0, 36, 8, 1, 3, { 40, 63, 60, 83 } },
        { 96, 36, 0, 0, 36, 9, 2, 4, { 40, 63, 84, 107 } },
        { 120, 36, 0, 0, 36, 10, 3, 5, { 40, 63, 108, 131 } },
        { 144, 36, 0, 0, 36, 11, 4, 158, { 40, 63, 132, 155 } },
        { 24, 60, 0, 0, 0, 12, 31, 7, { 64, 87, 12, 35 } },
        { 48, 60, 0, 0, 1, 13, 6, 8, { 64, 87, 36, 59 } },
        { 72, 60, 0, 0, 2, 14, 7, 9, { 64, 87, 60, 83 } },
        { 96, 60, 0, 0, 3, 15, 8, 10, { 64, 87, 84, 107 } },
        { 120, 60, 0, 0, 4, 16, 9, 11, { 64, 87, 108, 131 } },
        { 144, 60, 0, 0, 5, 17, 10, 158, { 64, 87, 132, 155 } },
        { 24, 84, 0, 0, 6, 18, 33, 13, { 88, 111, 12, 35 } },
        { 48, 84, 0, 0, 7, 19, 12, 14, { 88, 111, 36, 59 } },
        { 72, 84, 0, 0, 8, 20, 13, 15, { 88, 111, 60, 83 } },
        { 96, 84, 0, 0, 9, 21, 14, 16, { 88, 111, 84, 107 } },
        { 120, 84, 0, 0, 10, 22, 15, 17, { 88, 111, 108, 131 } },
        { 144, 84, 0, 0, 11, 23, 16, 160, { 88, 111, 132, 155 } },
        { 24, 108, 0, 0, 12, 24, 35, 19, { 112, 135, 12, 35 } },
        { 48, 108, 0, 0, 13, 25, 18, 20, { 112, 135, 36, 59 } },
        { 72, 108, 0, 0, 14, 26, 19, 21, { 112, 135, 60, 83 } },
        { 96, 108, 0, 0, 15, 27, 20, 22, { 112, 135, 84, 107 } },
        { 120, 108, 0, 0, 16, 28, 21, 23, { 112, 135, 108, 131 } },
        { 144, 108, 0, 0, 17, 29, 22, 162, { 112, 135, 132, 155 } },
        { 24, 132, 0, 0, 18, 39, 35, 25, { 136, 159, 12, 35 } },
        { 48, 132, 0, 0, 19, 39, 24, 26, { 136, 159, 36, 59 } },
        { 72, 132, 0, 0, 20, 39, 25, 27, { 136, 159, 60, 83 } },
        { 96, 132, 0, 0, 21, 39, 26, 28, { 136, 159, 84, 107 } },
        { 120, 132, 0, 0, 22, 39, 27, 29, { 136, 159, 108, 131 } },
        { 144, 132, 0, 0, 23, 39, 28, 162, { 136, 159, 132, 155 } },
        { 192, 52, 0, 0, 34, 32, 133, 31, { 56, 79, 182, 205 } },
        { 232, 60, 0, 0, 35, 33, 30, 128, { 64, 87, 218, 241 } },
        { 192, 84, 0, 0, 30, 34, 145, 33, { 88, 111, 182, 205 } },
        { 232, 92, 0, 0, 31, 35, 32, 140, { 96, 119, 218, 241 } },
        { 192, 116, 0, 0, 32, 30, 151, 35, { 120, 143, 182, 205 } },
        { 232, 124, 0, 0, 33, 31, 34, 152, { 128, 151, 218, 241 } },
        { 84, 16, 0, 0, 39, 128, 36, 36, { 18, 38, 27, 140 } },
        { 0, 0, 0, 0, 37, 37, 37, 37, { 18, 38, 6, 26 } },
        { 0, 0, 0, 0, 38, 38, 38, 38, { 18, 38, 141, 161 } },
        { 44, 168, 0, 0, 152, 36, 39, 39, { 168, 191, 0, 87 } },
        { 0, 0, 0, 0, 40, 40, 40, 40, { 168, 191, 112, 135 } },
        { 0, 0, 0, 0, 41, 41, 41, 41, { 168, 191, 232, 255 } },
        { 0, 0, 0, 0, 42, 42, 42, 42, { 0, 10, 8, 59 } },
        { 0, 0, 0, 0, 43, 43, 43, 43, { 0, 10, 60, 109 } },
        { 0, 0, 0, 0, 44, 44, 44, 44, { 0, 10, 110, 159 } },
        { 0, 0, 0, 0, 45, 45, 45, 45, { 168, 191, 168, 191 } },
        { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
    },
};

// The positions of each cursor and their callbacks
static const Box2CursorMoveTable sCursorMoveTables[16] = {
    { sCursorData0, &sCursorCallbacks0 },
    { sCursorData1, &sCursorCallbacks1 },
    { sCursorData2, &sCursorCallbacks2 },
    { sTrayCursorData[0], &sCursorCallbacks3 },
    { sTrayCursorData[1], &sCursorCallbacks4 },
    { sCursorData5, &sCursorCallbacks5 },
    { sTrayCursorData[2], &sCursorCallbacks6 },
    { sCursorData7, &sCursorCallbacks7 },
    { sCursorData8, &sCursorCallbacks8 },
    { sCursorData9, &sCursorCallbacks9 },
    { sCursorData10, &sCursorCallbacks10 },
    { sCursorData11, &sCursorCallbacks11 },
    { sCursorData12, &sCursorCallbacks12 },
    { sCursorData13, &sCursorCallbacks13 },
    { sCursorData14, &sCursorCallbacks14 },
    { sCursorData15, &sCursorCallbacks15 },
};

void func_ov255_021d23d8(Box2SysWork *syswk) {
    BOOL visible;
    u32 id;

    visible = func_0203d554() == TRUE ? FALSE : TRUE;
    switch (syswk->param->mode) {
    case 0:
        id = 0;
        break;
    case 1:
        id = 2;
        break;
    case 2:
        id = 3;
        break;
    case 3:
        id = 7;
        break;
    case 4:
        id = 13;
        break;
    case 5:
        id = 15;
        break;
    }
    func_ov255_021d24e0(syswk);
    syswk->app->cursorMove = CursorMove_Create(sCursorMoveTables[id].data, sCursorMoveTables[id].callbacks, syswk,
                                           visible, syswk->curRcvPos, HEAPID_BOX2_APP);
    CursorMove_SetHideOnTouch(syswk->app->cursorMove);
    func_ov255_021d251c(syswk, syswk->curRcvPos);
    func_ov255_021d101c(syswk, visible);
    syswk->app->oldCurPos = syswk->curRcvPos;
}

void func_ov255_021d2478(Box2SysWork *syswk, u32 id, u32 pos) {
    BOOL visible = CursorMove_IsCursorVisible(syswk->app->cursorMove);

    func_ov255_021d24e0(syswk);
    syswk->app->cursorMove = CursorMove_Create(sCursorMoveTables[id].data, sCursorMoveTables[id].callbacks, syswk,
                                           visible, pos, HEAPID_BOX2_APP);
    CursorMove_SetHideOnTouch(syswk->app->cursorMove);
    func_ov255_021d251c(syswk, pos);
    func_ov255_021d101c(syswk, visible);
    syswk->app->oldCurPos = pos;
}

void func_ov255_021d24e0(Box2SysWork *syswk) {
    if (syswk->app->cursorMove != NULL) {
        CursorMove_Delete(syswk->app->cursorMove);
        syswk->app->cursorMove = NULL;
    }
}

void func_ov255_021d24f8(Box2SysWork *syswk, u32 pos) {
    const CursorMoveData *data = CursorMove_GetData(syswk->app->cursorMove, pos);

    func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_CURSOR, data->x, data->y, 0);
}

static void func_ov255_021d251c(Box2SysWork *syswk, u32 pos) {
    func_ov255_021d24f8(syswk, pos);
    func_ov255_021d052c(syswk);
}

static void func_ov255_021d252c(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d251c(syswk, pos);
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
}

static void func_ov255_021d2540(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
}

static void func_ov255_021d2550(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
}

static void func_ov255_021d2568(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    CursorMove_SetPos(syswk->app->cursorMove, pos);
    func_ov255_021d251c(syswk, pos);
}

static void func_ov255_021d2584(void *work, int pos, int prevPos) {
}

u32 func_ov255_021d2588(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 8 && res <= 12) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 6;
        }
        if (res == 6) {
            func_0203d564(TRUE);
            return 6;
        }
        if (res == 7) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 7;
        }
    } else if (res == 6 || res == 7) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    return res;
}

static void func_ov255_021d2624(void *work, int pos, int prevPos) {
    if (pos != 6 && pos != 7) {
        func_ov255_021d251c(work, pos);
    }
}

void func_ov255_021d2634(Box2SysWork *syswk, u32 pos) {
    const CursorMoveData *data = CursorMove_GetData(syswk->app->cursorMove, pos);

    func_ov255_021d3304(syswk->app, 84, 16, data->x, data->y);
}

void func_ov255_021d2658(Box2SysWork *syswk, u32 pos) {
    const CursorMoveData *to = sCursorData1;
    const CursorMoveData *data;

    if (pos < BOX2_TRAY_POKE_MAX) {
        data = &sTrayCursorData[0][pos];
    } else {
        data = &sCursorData0[pos - BOX2_TRAY_POKE_MAX];
    }
    func_ov255_021d3304(syswk->app, data->x, data->y, to->x, to->y);
}

u32 func_ov255_021d2690(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 35 && res <= 39) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 33;
        }
        if (res == 33) {
            func_0203d564(TRUE);
            return 33;
        }
        if (res == 34) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 34;
        }
    } else if (res == 33 || res == 34) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            return 31;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            return 32;
        }
    }
    return res;
}

static void func_ov255_021d2760(void *work, int pos, int prevPos) {
    if (pos != 33 && pos != 34) {
        func_ov255_021d251c(work, pos);
    }
}

u32 func_ov255_021d2770(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 36 && res <= 41) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 34;
        }
        if (res == 34) {
            func_0203d564(TRUE);
            return 34;
        }
        if (res == 35) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 35;
        }
    } else if (res == 33 || res == 34 || res == 35 || res == 45) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            return 31;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            return 32;
        }
    }
    return res;
}

static void func_ov255_021d2848(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d286c(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 34 || pos == 35 || pos == 42 || pos == 43 || pos == 44 || pos == 45) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE && pos >= 36 && pos <= 41) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

void func_ov255_021d28c4(Box2SysWork *syswk, u32 pos) {
    u8 button1 = 0;
    u8 button3 = 0;
    u8 button4 = 0;

    if (syswk->unk18 != 0) {
        button1 = 2;
        if (syswk->param->mode == 3) {
            button3 = 1;
            button4 = 1;
        } else if (syswk->moveMode == 2) {
            button3 = button1;
            button4 = button1;
        }
    } else if (syswk->param->mode == 3) {
        button3 = 1;
        button4 = 1;
    } else if (pos >= BOX2_PARTY_POS) {
        if (func_ov255_021d3834(syswk->app->bgWinFrame) == FALSE ||
            Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            button3 = 2;
        }
    } else if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
        button3 = 2;
    }
    func_ov255_021d1af8(syswk, button1, 1, button3, button4);
}

static void func_ov255_021d2954(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d28c4(syswk, pos);
    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2984(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 31 || pos == 32) {
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_PARTY_POS);
        pos = BOX2_PARTY_POS;
    } else if (pos == 33 || pos == 38 || pos == 40 || pos == 41 || pos == 42 || pos == 43 || pos == 44 ||
               pos == 45) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    func_ov255_021c2854(syswk, pos);
    func_ov255_021d28c4(syswk, pos);
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d29e8(Box2SysWork *syswk) {
    u32 x;
    u32 y;
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    func_0203da84(&x, &y);
    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
            Box2Main_ShowCursor(syswk);
            return 40;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            func_ov255_021c2854(syswk, BOX2_PARTY_POS);
            return 31;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            func_ov255_021c2854(syswk, BOX2_PARTY_POS);
            return 32;
        }
    }
    return res;
}

u32 func_ov255_021d2a64(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 9 && res <= 14) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 7;
        }
        if (res == 7) {
            func_0203d564(TRUE);
            return 7;
        }
        if (res == 8) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 8;
        }
    } else if (res == 6 || res == 7 || res == 8 || res == 18) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    return res;
}

static void func_ov255_021d2b0c(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2b30(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 7 || pos == 8 || pos == 15 || pos == 16 || pos == 17 || pos == 18) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE && pos >= 9 && pos <= 14) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d2b88(Box2SysWork *syswk) {
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
            Box2Main_ShowCursor(syswk);
            return 40;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            func_ov255_021c2854(syswk, BOX2_BOXLIST_POS);
            return 37;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            func_ov255_021c2854(syswk, BOX2_BOXLIST_POS);
            return 38;
        }
    }
    return res;
}

static void func_ov255_021d2bf0(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d28c4(syswk, pos);
    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2c20(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 37 || pos == 38) {
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        pos = BOX2_BOXLIST_POS;
    } else if (pos == 40 || pos == 41 || pos == 42 || pos == 43 || pos == 44 || pos == 45) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    func_ov255_021c2854(syswk, pos);
    func_ov255_021d28c4(syswk, pos);
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d2c7c(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res == 36 || res == 37 || res == 38) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 34;
        }
        if (res == 34) {
            func_0203d564(TRUE);
            return 34;
        }
        if (res == 35) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 35;
        }
    } else if (res == 33 || res == 34 || res == 35) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            return 31;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            return 32;
        }
    }
    return res;
}

static void func_ov255_021d2d4c(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2d70(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 34 || pos == 35 || pos == 39 || pos == 40) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d2dac(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res == 9 || res == 10 || res == 11) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 7;
        }
        if (res == 7) {
            func_0203d564(TRUE);
            return 7;
        }
        if (res == 8) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 8;
        }
    } else if (res == 6 || res == 7 || res == 8) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    return res;
}

static void func_ov255_021d2e48(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2e6c(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 7 || pos == 8 || pos == 12 || pos == 13) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2ea8(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
}

static void func_ov255_021d2ec0(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    CursorMove_SetPos(syswk->app->cursorMove, pos);
    func_ov255_021d251c(syswk, pos);
}

static void func_ov255_021d2edc(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
}

static void func_ov255_021d2ef4(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    CursorMove_SetPos(syswk->app->cursorMove, pos);
    func_ov255_021d251c(syswk, pos);
}

static void func_ov255_021d2f10(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos >= 1 && pos <= 4) {
        func_ov255_021d1ac8(syswk, pos - 1, TRUE);
    } else {
        func_ov255_021d1ac8(syswk, 0, FALSE);
    }
    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d2f54(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 0 || pos == 5 || pos == 6) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d2f88(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 36 && res <= 39) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 34;
        }
        if (res == 34) {
            func_0203d564(TRUE);
            return 34;
        }
        if (res == 35) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 35;
        }
    } else if (res == 30 || res == 33 || res == 34 || res == 35 || res == 43) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            return 31;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            return 32;
        }
    }
    return res;
}

static void func_ov255_021d3064(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d3088(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 34 || pos == 35 || pos == 40 || pos == 41 || pos == 42 || pos == 43) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE && pos >= 36 && pos <= 39) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d30e0(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 9 && res <= 12) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            func_0203d564(FALSE);
            return 7;
        }
        if (res == 7) {
            func_0203d564(TRUE);
            return 7;
        }
        if (res == 8) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 8;
        }
    } else if (res == 6 || res == 7 || res == 8 || res == 16) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    return res;
}

static void func_ov255_021d3188(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
    syswk->app->oldCurPos = pos;
}

static void func_ov255_021d31ac(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    if (pos == 7 || pos == 8 || pos == 13 || pos == 14 || pos == 15 || pos == 16) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE && pos >= 9 && pos <= 12) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->app->oldCurPos);
        pos = syswk->app->oldCurPos;
    } else {
        func_ov255_021d251c(syswk, pos);
    }
    syswk->app->oldCurPos = pos;
}

u32 func_ov255_021d3204(Box2SysWork *syswk) {
    u8 prev = CursorMove_GetPos(syswk->app->cursorMove);
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        if (res >= 34 && res <= 35) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            func_ov255_021d251c(syswk, prev);
            return CURSORMOVE_NONE;
        }
        if (res == 33) {
            CursorMove_SetPos(syswk->app->cursorMove, prev);
            return 33;
        }
    } else if (res == 30 || res == 33) {
        CursorMove_SetPos(syswk->app->cursorMove, prev);
        func_ov255_021d251c(syswk, prev);
        return CURSORMOVE_NONE;
    }
    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            return 31;
        }
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            return 32;
        }
    }
    return res;
}

static void func_ov255_021d32b0(void *work, int pos, int prevPos) {
    Box2SysWork *syswk = work;

    func_ov255_021d32d4(syswk->app, pos, prevPos);
    Box2Main_VFuncReq(syswk->app, Box2Main_VFuncCursorMove);
}

static void func_ov255_021d32c8(void *work, int pos, int prevPos) {
    if (pos != 33) {
        func_ov255_021d251c(work, pos);
    }
}

void func_ov255_021d32d4(Box2AppWork *app, u32 pos, u32 curPos) {
    const CursorMoveData *from = CursorMove_GetData(app->cursorMove, pos);
    const CursorMoveData *to = CursorMove_GetData(app->cursorMove, curPos);

    func_ov255_021d3304(app, from->x, from->y, to->x, to->y);
}

static void func_ov255_021d3304(Box2AppWork *app, u8 fromX, u8 fromY, u8 toX, u8 toY) {
    Box2CursorMoveWork *mv = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2CursorMoveWork), FALSE, "box2_ui.c", 2600);

    mv->cnt = 4;
    mv->px = fromX;
    mv->py = fromY;
    if (fromX >= toX) {
        mv->vx = fromX - toX;
        mv->mx = 0;
    } else {
        mv->vx = toX - fromX;
        mv->mx = 1;
    }
    if (fromY >= toY) {
        mv->vy = fromY - toY;
        mv->my = 0;
    } else {
        mv->vy = toY - fromY;
        mv->my = 1;
    }
    mv->vx = (mv->vx << 8) / mv->cnt >> 8;
    mv->vy = (mv->vy << 8) / mv->cnt >> 8;
    app->vfunk.work = mv;
}

static u32 func_ov255_021d33a8(const CursorMoveData *data, u32 count) {
    TouchRect rects[2];
    u32 i;

    rects[1] = sTouchRectEnd;
    for (i = 0; i < count; i++) {
        rects[0] = data[i].rect;
        if (func_0203da0c(rects) != TOUCH_RECT_NONE) {
            return i;
        }
    }
    return CURSORMOVE_NONE;
}

static u32 func_ov255_021d3408(const CursorMoveData *data, u32 count) {
    TouchRect rects[2];
    u32 i;

    rects[1] = sTouchRectEnd;
    for (i = 0; i < count; i++) {
        rects[0] = data[i].rect;
        if (func_0203d9c8(rects) != TOUCH_RECT_NONE) {
            return i;
        }
    }
    return CURSORMOVE_NONE;
}

static u32 func_ov255_021d3468(const CursorMoveData *data, u32 count, u32 x, u32 y) {
    TouchRect rects[2];
    u32 i;

    rects[1] = sTouchRectEnd;
    for (i = 0; i < count; i++) {
        rects[0] = data[i].rect;
        if (func_0203dadc(rects, x, y) != TOUCH_RECT_NONE) {
            return i;
        }
    }
    return CURSORMOVE_NONE;
}

u32 func_ov255_021d34d0(void) {
    return func_ov255_021d33a8(sTrayCursorData[0], BOX2_TRAY_POKE_MAX);
}

u32 func_ov255_021d34e0(void) {
    return func_ov255_021d3408(sTrayCursorData[0], BOX2_TRAY_POKE_MAX);
}

u32 func_ov255_021d34f0(u32 x, u32 y) {
    return func_ov255_021d3468(sTrayCursorData[0], BOX2_TRAY_POKE_MAX, x, y);
}

u32 func_ov255_021d3504(void) {
    return func_ov255_021d33a8(sCursorData5, 6);
}

u32 func_ov255_021d3514(void) {
    return func_ov255_021d33a8(&sTrayCursorData[2][30], 6);
}

u32 func_ov255_021d3524(void) {
    return func_ov255_021d3408(&sTrayCursorData[2][30], 6);
}

u32 func_ov255_021d3534(void) {
    return func_ov255_021d3408(&sTrayCursorData[0][31], 2);
}

u32 func_ov255_021d3544(void) {
    return func_0203d9c8(sTouchRects_78e4);
}

BOOL func_ov255_021d3554(u32 *x, u32 *y) {
    if (func_0203d9c8(sTouchRects_78f0) != TOUCH_RECT_NONE) {
        func_0203da84(x, y);
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov255_021d357c(u32 *x, u32 *y) {
    if (func_0203d9c8(sTouchRects_78cc) != TOUCH_RECT_NONE) {
        func_0203da84(x, y);
        return TRUE;
    }
    return FALSE;
}

int func_ov255_021d35a4(u32 x, u32 y) {
    int res = func_0203dadc(sTouchRects_79fc, x, y);

    if (res == TOUCH_RECT_NONE) {
        return TOUCH_RECT_NONE;
    }
    return res;
}

u32 func_ov255_021d35c4(u32 x, u32 y) {
    u32 res = func_ov255_021d3468(&sTrayCursorData[1][34], 4, x, y);

    if (res != CURSORMOVE_NONE) {
        return res + BOX2_BOXLIST_POS;
    }
    return BOX2_GET_NONE;
}

BOOL func_ov255_021d35e8(void) {
    if (func_0203d9c8(sTouchRects_78d4) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov255_021d3604(void) {
    if (func_0203d9c8(sTouchRects_78dc) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov255_021d3620(void) {
    return func_ov255_021d3408(&sTrayCursorData[0][33], 1);
}

BOOL func_ov255_021d3630(void) {
    if (func_0203d9c8(sTouchRects_78c4) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}
