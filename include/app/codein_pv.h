#ifndef POKEBW2_APP_CODEIN_PV_H
#define POKEBW2_APP_CODEIN_PV_H

#include "types.h"
#include "gfl/arc.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "system/bmp_oam.h"
#include "system/codein_param.h"
#include "system/gf_font.h"
#include "system/palanm.h"

// What the code input screen's files share: its work, and the functions of codein_pv.c and codein_disp.c. The ROM
// names none of the three files; this header is named after pokeplatinum's codein_pv.h, a guess. Our names

// The most digits a code has
#define CODEIN_CODE_MAX 12
#define CODEIN_BAR_COUNT 2
#define CODEIN_BUTTON_COUNT 2
// The lower screen's keys: the ten digits, then Back and OK
#define CODEIN_KEY_COUNT 12
#define CODEIN_KEY_BACK 10
#define CODEIN_KEY_OK 11

// The touch rectangles: the digits of the code, then the keys
#define CODEIN_RECT_KEYS CODEIN_CODE_MAX
#define CODEIN_RECT_BACK (CODEIN_RECT_KEYS + CODEIN_KEY_BACK)
#define CODEIN_RECT_OK (CODEIN_RECT_KEYS + CODEIN_KEY_OK)
#define CODEIN_RECT_COUNT (CODEIN_RECT_KEYS + CODEIN_KEY_COUNT)

// The cursors: the one under the code's digit, the one on the keys, and the flash of a key that was pressed
enum {
    CODEIN_CURSOR_CODE,
    CODEIN_CURSOR_KEY,
    CODEIN_CURSOR_PRESS,
    CODEIN_CURSOR_COUNT,
};

// The sets of OBJ resources
enum {
    CODEIN_RES_CODE,
    CODEIN_RES_BUTTON,
    CODEIN_RES_CURSOR,
    CODEIN_RES_COUNT,
};

// The states of the screen
enum {
    CODEIN_STATE_INIT,
    CODEIN_STATE_INPUT,
    CODEIN_STATE_MOVE,
    CODEIN_STATE_END,
    CODEIN_STATE_ERROR,
};

// What the input state does next
enum {
    CODEIN_NEXT_NONE,
    // Moves the focus to the group work
    CODEIN_NEXT_FOCUS,
    // Moves the cursor to the digit work
    CODEIN_NEXT_CURSOR,
    CODEIN_NEXT_WAIT = 0xff,
};

// An actor's movement: the step of each frame, the frames left and the index of its next scale
typedef struct {
    s16 x;
    s16 y;
    u8 wait;
    u8 scale;
} CodeInMove;

typedef struct {
    // A digit: 0 while empty, or its value plus 1. A bar: the index of the digit before it. The code's cursor: the
    // index of its digit. Another cursor: its animation. A button: 1 while its press animation runs
    int state;
    // The group of a digit, from 1
    int group;
    // Whether a digit is shown large, as the focused group's are
    int size;
    ClActor *actor;
    TouchRect *rect;
    // The key's cursor keeps its column and row in x and y
    CodeInMove move;
} CodeInObj;

// The digits of a group, from start up to end
typedef struct {
    u16 start;
    u16 end;
} CodeInRange;

typedef struct {
    CodeInObj code[CODEIN_CODE_MAX];
    CodeInObj bar[CODEIN_BAR_COUNT];
    CodeInObj cur[CODEIN_CURSOR_COUNT];
    CodeInObj btn[CODEIN_BUTTON_COUNT];
    // Where the code starts for each focus, none and the three groups
    s16 focusX[CODEIN_BLOCK_COUNT + 1];
    CodeInRange range[CODEIN_BLOCK_COUNT];
    u32 unk228;
    int state;
    int unk230;
    int seq;
    int count;
    int codeMax;
    // The group that is shown large, from 1, or 0 for none, and the one before it
    int focus;
    int focusOld;
    // The digits that grow and the digits that shrink
    int largeStart;
    int largeEnd;
    int smallStart;
    int smallEnd;
    ArcTool *arc;
    ClActUnit *unit;
    u32 plttRes[CODEIN_RES_COUNT];
    u32 charRes[CODEIN_RES_COUNT];
    u32 unk278[CODEIN_RES_COUNT];
    u32 cellAnimRes[CODEIN_RES_COUNT];
    PaletteFade *fade;
    ButtonMan *buttons;
    TouchRect rects[CODEIN_RECT_COUNT + 1];
    // Whether the touch screen was used last, which hides the keys' cursor
    BOOL touchMode;
    BmpOamSys *bmpOam;
    BmpOamActor *btnText[CODEIN_BUTTON_COUNT];
    u32 btnTextPltt;
    BmpWin *window;
    Font *font;
    TCB *vblankTask;
    // CODEIN_NEXT_*, its group or digit, and whether the cursor goes to the group's last digit
    int next;
    int nextWork;
    BOOL nextLast;
    CodeInParam param;
} CodeInWork;

// codein_pv.c
void CodeIn_InitParam(CodeInWork *wk);
BOOL CodeIn_Main(CodeInWork *wk);
void CodeIn_InitButtons(CodeInWork *wk);

// codein_disp.c
void CodeInDisp_InitActors(CodeInWork *wk, const BGSysVRAMConfig *vramConfig);
void CodeInDisp_LoadResources(CodeInWork *wk);
void CodeInDisp_Free(CodeInWork *wk);
void CodeInDisp_AddCodeActors(CodeInWork *wk);
void CodeInDisp_AddCursorActors(CodeInWork *wk);
void CodeInDisp_AddButtonActors(CodeInWork *wk);
void CodeInDisp_SetCursorVisible(CodeInWork *wk, int cursor, BOOL visible);
void CodeInDisp_SetCodeCursor(CodeInWork *wk, int digit);
void CodeInDisp_SetKeyCursor(CodeInWork *wk, int key);
void CodeInDisp_SetCursorOnKey(CodeInWork *wk, int key, int cursor);
void CodeInDisp_UpdateCursors(CodeInWork *wk);
void CodeInDisp_UpdateButtons(CodeInWork *wk);
// The animation of a digit's actor
int CodeInDisp_GetCodeSequence(int state, int size);
void CodeInDisp_SetCodeSizes(CodeInWork *wk);
// Puts the digits and bars where the focus has them, at once or by starting their movement
void CodeInDisp_SetCodePositions(CodeInWork *wk, BOOL move);
void CodeInDisp_UpdateCodeRects(CodeInWork *wk);
void CodeInDisp_InitBmpOam(CodeInWork *wk);
void CodeInDisp_AddButtonTexts(CodeInWork *wk);
void CodeInDisp_CreateMessageWindow(BmpWin **window, u32 messageId, int bg, int x, int y, int width, int height,
                                    Font *font, int number);
void CodeInDisp_DrawMessage(BmpWin *window, u32 messageId, Font *font, int number);
void CodeInDisp_DrawErrorMessage(BmpWin *window, u32 mode, Font *font, int number);

#endif // POKEBW2_APP_CODEIN_PV_H
