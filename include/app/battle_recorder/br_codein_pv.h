#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_PV_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_PV_H

#include "types.h"
#include "app/battle_recorder/br_codein.h"
#include "gfl/arc.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "system/bmp_oam.h"
#include "system/gf_font.h"

// What the files of the Battle Recorder's code input share: its work, and the functions of br_codein_pv.c and
// br_codein_disp.c. The ROM names neither file; the names are guesses after overlay 205's codein_pv.c and
// codein_disp.c, which they are copies of (app/codein_pv.h). Our names

// The most digits a code has
#define BR_CODEIN_CODE_MAX 12
#define BR_CODEIN_BAR_COUNT 2
#define BR_CODEIN_BUTTON_COUNT 2
// The lower screen's keys: the ten digits, then Back and OK
#define BR_CODEIN_KEY_COUNT 12
#define BR_CODEIN_KEY_BACK 10
#define BR_CODEIN_KEY_OK 11

// The touch rectangles: the digits of the code, then the keys
#define BR_CODEIN_RECT_KEYS BR_CODEIN_CODE_MAX
#define BR_CODEIN_RECT_BACK (BR_CODEIN_RECT_KEYS + BR_CODEIN_KEY_BACK)
#define BR_CODEIN_RECT_OK (BR_CODEIN_RECT_KEYS + BR_CODEIN_KEY_OK)
#define BR_CODEIN_RECT_COUNT (BR_CODEIN_RECT_KEYS + BR_CODEIN_KEY_COUNT)

// The cursors: the one under the code's digit, the one on the keys, and the flash of a key that was pressed. Nothing
// shows the last two here
enum {
    BR_CODEIN_CURSOR_CODE,
    BR_CODEIN_CURSOR_KEY,
    BR_CODEIN_CURSOR_PRESS,
    BR_CODEIN_CURSOR_COUNT,
};

// The sets of OBJ resources
enum {
    BR_CODEIN_RES_CODE,
    BR_CODEIN_RES_BUTTON,
    BR_CODEIN_RES_CURSOR,
    BR_CODEIN_RES_COUNT,
};

// The states of the code input
enum {
    BR_CODEIN_STATE_INIT,
    BR_CODEIN_STATE_INPUT,
    BR_CODEIN_STATE_MOVE,
    BR_CODEIN_STATE_END,
};

// What the input state does next
enum {
    BR_CODEIN_NEXT_NONE,
    // Moves the focus to the group work
    BR_CODEIN_NEXT_FOCUS,
    // Moves the cursor to the digit work
    BR_CODEIN_NEXT_CURSOR,
    BR_CODEIN_NEXT_WAIT = 0xff,
};

// An actor's movement: the step of each frame, the frames left and the index of its next scale
typedef struct {
    s16 x;
    s16 y;
    u8 wait;
    u8 scale;
} BrCodeInMove;

typedef struct {
    // A digit: 0 while empty, or its value plus 1. A bar: the index of the digit before it. The code's cursor: the
    // index of its digit. Another cursor: its animation. A button: 1 once it was touched
    int state;
    // The group of a digit, from 1
    int group;
    // Whether a digit is shown large, as the focused group's are
    int size;
    ClActor *actor;
    TouchRect *rect;
    // The key's cursor keeps its column and row in x and y
    BrCodeInMove move;
} BrCodeInObj;

// The digits of a group, from start up to end
typedef struct {
    u16 start;
    u16 end;
} BrCodeInRange;

struct BrCodeInWork {
    BrCodeInObj code[BR_CODEIN_CODE_MAX];
    BrCodeInObj bar[BR_CODEIN_BAR_COUNT];
    BrCodeInObj cur[BR_CODEIN_CURSOR_COUNT];
    BrCodeInObj btn[BR_CODEIN_BUTTON_COUNT];
    // Where the code starts for each focus, none and the three groups
    s16 focusX[BR_CODEIN_BLOCK_COUNT + 1];
    BrCodeInRange range[BR_CODEIN_BLOCK_COUNT];
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
    u32 plttRes[BR_CODEIN_RES_COUNT];
    u32 charRes[BR_CODEIN_RES_COUNT];
    u32 unk278[BR_CODEIN_RES_COUNT];
    u32 cellAnimRes[BR_CODEIN_RES_COUNT];
    u32 unk290;
    ButtonMan *buttons;
    TouchRect rects[BR_CODEIN_RECT_COUNT + 1];
    // Set at the start and never cleared: there is no input by keys
    BOOL touchMode;
    BmpOamSys *bmpOam;
    BmpOamActor *btnText[BR_CODEIN_BUTTON_COUNT];
    u32 unk30C;
    Font *font;
    TCB *vblankTask;
    // BR_CODEIN_NEXT_*, its group or digit, and whether the cursor goes to the group's last digit
    int next;
    int nextWork;
    BOOL nextLast;
    BrCodeInParam param;
    HeapID heapId;
    // BR_CODEIN_RESULT_*
    int result;
    // The number of each group, and the whole number
    u32 blockNumber[BR_CODEIN_BLOCK_COUNT];
    u64 number;
};

// br_codein_pv.c
void BrCodeIn_InitParam(BrCodeInWork *wk);
BOOL BrCodeIn_MainPv(BrCodeInWork *wk);
void BrCodeIn_InitButtons(BrCodeInWork *wk);

// br_codein_disp.c
void BrCodeInDisp_GetResources(BrCodeInWork *wk);
void BrCodeInDisp_Free(BrCodeInWork *wk);
void BrCodeInDisp_AddCodeActors(BrCodeInWork *wk);
void BrCodeInDisp_AddCursorActors(BrCodeInWork *wk);
void BrCodeInDisp_AddButtonActors(BrCodeInWork *wk);
void BrCodeInDisp_SetCursorVisible(BrCodeInWork *wk, int cursor, BOOL visible);
void BrCodeInDisp_SetCodeCursor(BrCodeInWork *wk, int digit);
void BrCodeInDisp_SetKeyCursor(BrCodeInWork *wk, int key);
void BrCodeInDisp_UpdateCursors(BrCodeInWork *wk);
// The animation of a digit's actor
int BrCodeInDisp_GetCodeSequence(int state, int size);
void BrCodeInDisp_SetCodeSizes(BrCodeInWork *wk);
// Puts the digits and bars where the focus has them, at once or by starting their movement
void BrCodeInDisp_SetCodePositions(BrCodeInWork *wk, BOOL move);
void BrCodeInDisp_UpdateCodeRects(BrCodeInWork *wk);
void BrCodeInDisp_InitBmpOam(BrCodeInWork *wk);
void BrCodeInDisp_AddButtonTexts(BrCodeInWork *wk);

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_PV_H
