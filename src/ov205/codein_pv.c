#include "types.h"
#include "app/codein.h"
#include "app/codein_pv.h"
#include "constants/sound.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "system/codein_param.h"
#include "system/str_tool.h"
#include "system/wipe.h"

// The code input screen's states and input. The ROM has no name for this file; "codein_pv.c" is a guess from
// pokeplatinum, whose code input screen has the same three files. Our names

#define KEY_COLUMNS 5
#define KEY_ROWS 3
// The frames of a change of focus: the digits move, then finish scaling
#define MOVE_FRAMES 4
#define SCALE_COUNT 6

typedef BOOL (*CodeInStateFunc)(CodeInWork *wk);

// A key's center and half its size
typedef struct {
    s16 x;
    s16 y;
    s16 halfWidth;
    s16 halfHeight;
} CodeInKeyRect;

static void CodeIn_SetFocus(CodeInWork *wk, int focus);
static void CodeIn_SetState(CodeInWork *wk, int state);
static BOOL CodeIn_StateInit(CodeInWork *wk);
static BOOL CodeIn_StateEnd(CodeInWork *wk);
static BOOL CodeIn_StateError(CodeInWork *wk);
static BOOL CodeIn_StateInput(CodeInWork *wk);
static BOOL CodeIn_StateMove(CodeInWork *wk);
static void CodeIn_HandleKeys(CodeInWork *wk);
static int CodeIn_GetNumber(CodeInWork *wk);
static BOOL CodeIn_IsNumberValid(CodeInWork *wk);
static void CodeIn_Confirm(CodeInWork *wk);
static void CodeIn_Backspace(CodeInWork *wk);
static void CodeIn_SetKeyCursorByRect(CodeInWork *wk, int rect);
static void CodeIn_ButtonCallback(u32 button, u32 event, void *work);
static void CodeIn_RunNext(CodeInWork *wk);
static void CodeIn_ClearNext(CodeInWork *wk);
static int CodeIn_FindGroupFirst(CodeInWork *wk, int group);
static int CodeIn_FindGroupLast(CodeInWork *wk, int group);

// By CODEIN_STATE_*
static const CodeInStateFunc sCodeInStates[] = {
    CodeIn_StateInit, CodeIn_StateInput, CodeIn_StateMove, CodeIn_StateEnd, CodeIn_StateError,
};

// The column and row of each key, by its touch rectangle
static const u8 sCodeInKeyCursorPos[CODEIN_KEY_COUNT][2] = {
    { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 0, 1 },
    { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 0, 2 }, { 3, 2 },
};

// The scales of a digit as it grows and as it shrinks
static fx32 sCodeInScalesLarge[] = { 0x800, 0x333, 0x800, FX32_ONE, 0x1333, FX32_ONE, FX32_ONE };
static fx32 sCodeInScalesSmall[] = { 0xccd, 0x99a, 0x666, 0x333, 0xccd, FX32_ONE, FX32_ONE };

static void CodeIn_SetFocus(CodeInWork *wk, int focus) {
    wk->focusOld = wk->focus;
    wk->focus = focus;
    wk->largeStart = 0;
    wk->largeEnd = 0;
    wk->smallStart = 0;
    wk->smallEnd = 0;
    if (focus != 0) {
        wk->largeStart = wk->range[focus - 1].start;
        wk->largeEnd = wk->range[focus - 1].end;
    }
    if (wk->focusOld != 0) {
        wk->smallStart = wk->range[wk->focusOld - 1].start;
        wk->smallEnd = wk->range[wk->focusOld - 1].end;
    }
}

void CodeIn_InitParam(CodeInWork *wk) {
    int i;
    int j;
    int count;

    wk->touchMode = TRUE;
    wk->range[0].start = 0;
    wk->range[0].end = wk->param.blocks[0];
    wk->range[1].start = wk->param.blocks[0];
    wk->range[1].end = wk->param.blocks[0] + wk->param.blocks[1];
    wk->range[2].start = wk->param.blocks[0] + wk->param.blocks[1];
    wk->range[2].end = wk->param.blocks[0] + wk->param.blocks[1] + wk->param.blocks[2];
    CodeIn_SetFocus(wk, 1);
    for (i = 0; i < CODEIN_BLOCK_COUNT; i++) {
        wk->codeMax += wk->param.blocks[i];
    }
    {
        int kind = 0;
        // For a Friend Code and for a single number
        int focusX[2][CODEIN_BLOCK_COUNT] = { { 56, 20, 8 }, { 56, 12, 0 } };

        if (wk->param.blocks[0] != 4) {
            kind = 1;
        }
        for (i = 0; i < CODEIN_BLOCK_COUNT; i++) {
            wk->focusX[i] = focusX[kind][i];
        }
        wk->focusX[i] = focusX[kind][i - 1];
    }
    count = 0;
    for (i = 0; i < CODEIN_BAR_COUNT; i++) {
        count += wk->param.blocks[i];
        wk->bar[i].state = count - 1;
    }
    i = 0;
    count = 0;
    do {
        for (j = 0; j < wk->param.blocks[i]; j++) {
            wk->code[count].group = i + 1;
            count++;
        }
        i++;
    } while (count < wk->codeMax);
}

static void CodeIn_SetState(CodeInWork *wk, int state) {
    wk->state = state;
    wk->unk230 = 0;
    wk->seq = 0;
    wk->count = 0;
}

static BOOL CodeIn_StateInit(CodeInWork *wk) {
    CodeInDisp_LoadResources(wk);
    CodeInDisp_SetCodeSizes(wk);
    CodeInDisp_AddCodeActors(wk);
    CodeInDisp_SetCodePositions(wk, FALSE);
    CodeInDisp_AddCursorActors(wk);
    CodeInDisp_AddButtonActors(wk);
    CodeInDisp_UpdateCodeRects(wk);
    CodeInDisp_InitBmpOam(wk);
    CodeInDisp_AddButtonTexts(wk);
    CodeInDisp_CreateMessageWindow(&wk->window, wk->param.mode, 4, 2, 19, 27, 4, wk->font, wk->param.max);
    CodeIn_SetState(wk, CODEIN_STATE_INPUT);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_CODEIN);
    return FALSE;
}

static BOOL CodeIn_StateEnd(CodeInWork *wk) {
    switch (wk->seq) {
    case 0:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_CODEIN);
        wk->seq++;
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            wk->seq++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL CodeIn_StateError(CodeInWork *wk) {
    switch (wk->seq) {
    case 0:
        CodeInDisp_DrawErrorMessage(wk->window, wk->param.mode, wk->font, wk->param.max);
        wk->seq++;
        break;
    case 1:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) ||
            GFL_HIDIsTouchPressed()) {
            CodeInDisp_DrawMessage(wk->window, wk->param.mode, wk->font, wk->param.max);
            CodeIn_SetState(wk, CODEIN_STATE_INPUT);
        }
        break;
    }
    return FALSE;
}

static BOOL CodeIn_StateInput(CodeInWork *wk) {
    if (wk->seq == 0) {
        if (GFL_WipeIsFinished() == TRUE) {
            wk->seq++;
        }
    } else {
        CodeIn_RunNext(wk);
        GFL_BMN_Main(wk->buttons);
        CodeIn_HandleKeys(wk);
    }
    return FALSE;
}

static BOOL CodeIn_StateMove(CodeInWork *wk) {
    int i;
    ClActorScale scale;
    ClActorPos pos;
    ClActorPos barPos;

    switch (wk->seq) {
    case 0:
        CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_CODE, FALSE);
        for (i = 0; i < wk->codeMax; i++) {
            if (wk->code[i].move.wait == 0) {
                continue;
            }
            ClActor_GetSurfacePos(wk->code[i].actor, &pos, 0);
            pos.x += wk->code[i].move.x;
            pos.y += wk->code[i].move.y;
            ClActor_SetSurfacePos(wk->code[i].actor, &pos, 0);
            wk->code[i].move.wait--;
            if (i >= wk->largeStart && i < wk->largeEnd) {
                scale.x = sCodeInScalesLarge[wk->code[i].move.scale];
                scale.y = sCodeInScalesLarge[wk->code[i].move.scale];
                ClActor_SetScale(wk->code[i].actor, &scale);
                wk->code[i].move.scale++;
            }
            if (i >= wk->smallStart && i < wk->smallEnd) {
                scale.x = sCodeInScalesSmall[wk->code[i].move.scale];
                scale.y = sCodeInScalesSmall[wk->code[i].move.scale];
                ClActor_SetScale(wk->code[i].actor, &scale);
                wk->code[i].move.scale++;
            }
        }
        for (i = 0; i < CODEIN_BAR_COUNT; i++) {
            if (wk->bar[i].move.wait == 0) {
                continue;
            }
            ClActor_GetSurfacePos(wk->bar[i].actor, &barPos, 0);
            barPos.x += wk->bar[i].move.x;
            barPos.y += wk->bar[i].move.y;
            ClActor_SetSurfacePos(wk->bar[i].actor, &barPos, 0);
            wk->bar[i].move.wait--;
        }
        if (wk->code[0].move.wait == 0) {
            for (i = wk->largeStart; i < wk->largeEnd; i++) {
                ClActor_SetSequence(wk->code[i].actor, CodeInDisp_GetCodeSequence(wk->code[i].state, wk->code[i].size));
            }
            for (i = wk->smallStart; i < wk->smallEnd; i++) {
                ClActor_SetSequence(wk->code[i].actor, CodeInDisp_GetCodeSequence(wk->code[i].state, wk->code[i].size));
            }
            wk->seq++;
        }
        wk->count++;
        break;
    case 1:
        for (i = wk->largeStart; i < wk->largeEnd; i++) {
            if (wk->code[i].move.scale == SCALE_COUNT) {
                continue;
            }
            scale.x = sCodeInScalesLarge[wk->code[i].move.scale];
            scale.y = sCodeInScalesLarge[wk->code[i].move.scale];
            ClActor_SetScale(wk->code[i].actor, &scale);
            wk->code[i].move.scale++;
        }
        for (i = wk->smallStart; i < wk->smallEnd; i++) {
            if (wk->code[i].move.scale == SCALE_COUNT) {
                continue;
            }
            scale.x = sCodeInScalesSmall[wk->code[i].move.scale];
            scale.y = sCodeInScalesSmall[wk->code[i].move.scale];
            ClActor_SetScale(wk->code[i].actor, &scale);
            wk->code[i].move.scale++;
        }
        wk->count++;
        if (wk->count == SCALE_COUNT) {
            wk->seq++;
        }
        break;
    default:
        CodeInDisp_UpdateCodeRects(wk);
        if (wk->nextLast == FALSE) {
            i = CodeIn_FindGroupFirst(wk, wk->nextWork);
        } else {
            i = CodeIn_FindGroupLast(wk, wk->nextWork);
        }
        CodeInDisp_SetCodeCursor(wk, i);
        if (wk->focus != 0) {
            CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_CODE, TRUE);
        }
        CodeIn_ClearNext(wk);
        CodeIn_SetState(wk, CODEIN_STATE_INPUT);
        break;
    }
    return FALSE;
}

BOOL CodeIn_Main(CodeInWork *wk) {
    BOOL done = sCodeInStates[wk->state](wk);

    CodeInDisp_UpdateCursors(wk);
    CodeInDisp_UpdateButtons(wk);
    return done;
}

static void CodeIn_HandleKeys(CodeInWork *wk) {
    int keys[KEY_ROWS][KEY_COLUMNS] = {
        { 0, 1, 2, 3, 4 },
        { 5, 6, 7, 8, 9 },
        { CODEIN_KEY_BACK, CODEIN_KEY_BACK, CODEIN_KEY_BACK, CODEIN_KEY_OK, CODEIN_KEY_OK },
    };
    BOOL moved = FALSE;
    int key = keys[wk->cur[CODEIN_CURSOR_KEY].move.y][wk->cur[CODEIN_CURSOR_KEY].move.x];

    // The first key press after a touch only shows the cursor
    if (wk->touchMode == TRUE && GCTX_HIDGetPressedKeys()) {
        wk->touchMode = FALSE;
        CodeInDisp_SetKeyCursor(wk, key);
        if (key == CODEIN_KEY_BACK || key == CODEIN_KEY_OK) {
            if (wk->cur[CODEIN_CURSOR_KEY].state != 2) {
                wk->cur[CODEIN_CURSOR_KEY].state = 2;
            }
        } else {
            if (wk->cur[CODEIN_CURSOR_KEY].state != 1) {
                wk->cur[CODEIN_CURSOR_KEY].state = 1;
            }
        }
        return;
    }
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        if (wk->cur[CODEIN_CURSOR_KEY].move.y > 0) {
            wk->cur[CODEIN_CURSOR_KEY].move.y--;
        } else {
            wk->cur[CODEIN_CURSOR_KEY].move.y = KEY_ROWS - 1;
        }
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        wk->cur[CODEIN_CURSOR_KEY].move.y++;
        wk->cur[CODEIN_CURSOR_KEY].move.y %= KEY_ROWS;
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        if (key == CODEIN_KEY_BACK) {
            wk->cur[CODEIN_CURSOR_KEY].move.x = 3;
        } else if (key == CODEIN_KEY_OK) {
            wk->cur[CODEIN_CURSOR_KEY].move.x = 0;
        } else {
            wk->cur[CODEIN_CURSOR_KEY].move.x++;
            wk->cur[CODEIN_CURSOR_KEY].move.x %= KEY_COLUMNS;
        }
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        if (key == CODEIN_KEY_BACK) {
            wk->cur[CODEIN_CURSOR_KEY].move.x = 3;
        } else if (key == CODEIN_KEY_OK) {
            wk->cur[CODEIN_CURSOR_KEY].move.x = 0;
        } else if (wk->cur[CODEIN_CURSOR_KEY].move.x > 0) {
            wk->cur[CODEIN_CURSOR_KEY].move.x--;
        } else {
            wk->cur[CODEIN_CURSOR_KEY].move.x = KEY_COLUMNS - 1;
        }
        moved = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        if (key == CODEIN_KEY_BACK) {
            CodeIn_Backspace(wk);
            GFL_SndSEPlay(SEQ_SE_CANCEL3);
        } else if (key == CODEIN_KEY_OK) {
            if (CodeIn_IsNumberValid(wk)) {
                CodeIn_Confirm(wk);
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            } else {
                CodeIn_SetState(wk, CODEIN_STATE_ERROR);
            }
        } else {
            int digit;
            int group;
            int next;
            int nextGroup;

            if (wk->focus == 0) {
                return;
            }
            digit = wk->cur[CODEIN_CURSOR_CODE].state;
            wk->code[digit].state = key + 1;
            CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, FALSE);
            CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_PRESS, TRUE);
            CodeInDisp_SetCursorOnKey(wk, key, CODEIN_CURSOR_PRESS);
            ClActor_SetSequence(wk->code[digit].actor,
                                CodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size));
            ClActor_SetSequence(wk->cur[CODEIN_CURSOR_PRESS].actor, 3);
            group = wk->code[digit].group;
            next = digit + 1;
            if (next == wk->codeMax) {
                wk->next = CODEIN_NEXT_FOCUS;
                wk->nextWork = 0;
                wk->cur[CODEIN_CURSOR_KEY].move.x = 3;
                wk->cur[CODEIN_CURSOR_KEY].move.y = 2;
                moved = TRUE;
            } else {
                nextGroup = wk->code[next].group;
                if (group != nextGroup) {
                    wk->next = CODEIN_NEXT_FOCUS;
                    wk->nextWork = nextGroup;
                } else {
                    wk->next = CODEIN_NEXT_CURSOR;
                    wk->nextWork = next;
                }
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        CodeIn_Backspace(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL3);
    } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
        int digit;

        if (wk->cur[CODEIN_CURSOR_CODE].state == 0) {
            wk->cur[CODEIN_CURSOR_CODE].state = wk->codeMax - 1;
        } else {
            wk->cur[CODEIN_CURSOR_CODE].state--;
        }
        digit = wk->cur[CODEIN_CURSOR_CODE].state;
        if (wk->code[digit].size == 1) {
            wk->next = CODEIN_NEXT_CURSOR;
            wk->nextWork = digit;
        } else {
            wk->next = CODEIN_NEXT_FOCUS;
            wk->nextWork = wk->code[digit].group;
            wk->nextLast = TRUE;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
        int digit;

        if (wk->cur[CODEIN_CURSOR_CODE].state == wk->codeMax - 1) {
            wk->cur[CODEIN_CURSOR_CODE].state = 0;
        } else {
            wk->cur[CODEIN_CURSOR_CODE].state++;
        }
        digit = wk->cur[CODEIN_CURSOR_CODE].state;
        if (wk->code[digit].size == 1) {
            wk->next = CODEIN_NEXT_CURSOR;
            wk->nextWork = digit;
        } else {
            wk->next = CODEIN_NEXT_FOCUS;
            wk->nextWork = wk->code[digit].group;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
    if (moved == TRUE) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        key = keys[wk->cur[CODEIN_CURSOR_KEY].move.y][wk->cur[CODEIN_CURSOR_KEY].move.x];
        CodeInDisp_SetKeyCursor(wk, key);
        if (key == CODEIN_KEY_BACK || key == CODEIN_KEY_OK) {
            if (wk->cur[CODEIN_CURSOR_KEY].state != 2) {
                wk->cur[CODEIN_CURSOR_KEY].state = 2;
            }
        } else {
            if (wk->cur[CODEIN_CURSOR_KEY].state != 1) {
                wk->cur[CODEIN_CURSOR_KEY].state = 1;
            }
        }
    }
}

// The number the digits make, empty ones counting as 0
static int CodeIn_GetNumber(CodeInWork *wk) {
    int i;
    int number = 0;
    int place = 1;

    for (i = wk->codeMax - 1; i > -1; i--) {
        if (wk->code[i].state != 0) {
            number += (wk->code[i].state - 1) * place;
        }
        place *= 10;
    }
    return number;
}

// Whether the number is at most the parameter's largest. A Friend Code always is
static BOOL CodeIn_IsNumberValid(CodeInWork *wk) {
    if (wk->param.mode == 0) {
        return TRUE;
    }
    if (CodeIn_GetNumber(wk) <= wk->param.max) {
        return TRUE;
    }
    return FALSE;
}

// Writes the digits to the parameter's string, empty ones as 0, and ends the screen
static void CodeIn_Confirm(CodeInWork *wk) {
    int i;
    StrBuf *digit = GFL_StrBufCreate(100, HEAPID_CODEIN);

    wk->btn[1].state = 1;
    wk->btn[1].move.wait = 0;
    for (i = 0; i < wk->codeMax; i++) {
        if (wk->code[i].state == 0) {
            wk->code[i].state = 1;
            ClActor_SetSequence(wk->code[i].actor, CodeInDisp_GetCodeSequence(wk->code[i].state, wk->code[i].size));
        }
        GFL_WordSetFormatNumber(digit, wk->code[i].state - 1, 1, 1, TRUE);
        GFL_StrBufConcat(wk->param.str, digit);
    }
    GFL_StrBufFree(digit);
    CodeIn_SetState(wk, CODEIN_STATE_END);
}

// Clears the cursor's digit and steps back, or goes to the last digit when nothing is focused
static void CodeIn_Backspace(CodeInWork *wk) {
    int digit;
    int group;
    int prevGroup;

    wk->btn[0].state = 1;
    wk->btn[0].move.wait = 0;
    if (wk->focus == 0) {
        wk->cur[CODEIN_CURSOR_CODE].state = wk->codeMax - 1;
        group = wk->code[wk->cur[CODEIN_CURSOR_CODE].state].group;
        wk->next = CODEIN_NEXT_FOCUS;
        wk->nextWork = group;
        wk->nextLast = TRUE;
        return;
    }
    digit = wk->cur[CODEIN_CURSOR_CODE].state;
    wk->code[digit].state = 0;
    ClActor_SetSequence(wk->code[digit].actor, CodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size));
    group = wk->code[digit].group;
    if (digit > 0) {
        ClActor_SetSequence(wk->code[digit - 1].actor,
                            CodeInDisp_GetCodeSequence(wk->code[digit - 1].state, wk->code[digit - 1].size));
        prevGroup = wk->code[digit - 1].group;
        if (group != prevGroup) {
            wk->next = CODEIN_NEXT_FOCUS;
            wk->nextWork = prevGroup;
            wk->nextLast = TRUE;
        } else {
            wk->next = CODEIN_NEXT_CURSOR;
            wk->nextWork = digit - 1;
        }
    }
}

void CodeIn_InitButtons(CodeInWork *wk) {
    int i;

    for (i = 0; i < CODEIN_CODE_MAX; i++) {
        wk->code[i].rect = &wk->rects[i];
    }
    {
        CodeInKeyRect keys[CODEIN_KEY_COUNT] = {
            { 32, 80, 20, 20 },   { 80, 80, 20, 20 },   { 128, 80, 20, 20 }, { 176, 80, 20, 20 },
            { 224, 80, 20, 20 },  { 32, 128, 20, 20 },  { 80, 128, 20, 20 }, { 128, 128, 20, 20 },
            { 176, 128, 20, 20 }, { 224, 128, 20, 20 }, { 64, 176, 60, 12 }, { 192, 176, 60, 12 },
        };

        for (; i < CODEIN_RECT_COUNT; i++) {
            wk->rects[i].top = keys[i - CODEIN_RECT_KEYS].y - keys[i - CODEIN_RECT_KEYS].halfHeight;
            wk->rects[i].left = keys[i - CODEIN_RECT_KEYS].x - keys[i - CODEIN_RECT_KEYS].halfWidth;
            wk->rects[i].bottom = keys[i - CODEIN_RECT_KEYS].y + keys[i - CODEIN_RECT_KEYS].halfHeight;
            wk->rects[i].right = keys[i - CODEIN_RECT_KEYS].x + keys[i - CODEIN_RECT_KEYS].halfWidth;
        }
    }
    wk->rects[CODEIN_RECT_COUNT].top = TOUCH_RECT_END;
    wk->buttons = GFL_BMN_Create(wk->rects, CodeIn_ButtonCallback, wk, HEAPID_CODEIN);
}

static void CodeIn_SetKeyCursorByRect(CodeInWork *wk, int rect) {
    if (rect >= CODEIN_RECT_KEYS && rect < CODEIN_RECT_COUNT) {
        wk->cur[CODEIN_CURSOR_KEY].move.x = sCodeInKeyCursorPos[rect - CODEIN_RECT_KEYS][0];
        wk->cur[CODEIN_CURSOR_KEY].move.y = sCodeInKeyCursorPos[rect - CODEIN_RECT_KEYS][1];
    }
}

static void CodeIn_ButtonCallback(u32 button, u32 event, void *work) {
    CodeInWork *wk = work;

    if (wk->state != CODEIN_STATE_INPUT) {
        return;
    }
    if (wk->touchMode != TRUE) {
        wk->touchMode = TRUE;
    }
    if (event == BMN_EVENT_TOUCH) {
        if (button <= CODEIN_RECT_KEYS - 1) {
            if (wk->code[button].size == 1) {
                wk->next = CODEIN_NEXT_CURSOR;
                wk->nextWork = button;
            } else {
                wk->next = CODEIN_NEXT_FOCUS;
                wk->nextWork = wk->code[button].group;
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        } else if (button == CODEIN_RECT_BACK) {
            wk->cur[CODEIN_CURSOR_KEY].move.x = 0;
            wk->cur[CODEIN_CURSOR_KEY].move.y = 2;
            GFL_SndSEPlay(SEQ_SE_CANCEL3);
            CodeIn_Backspace(wk);
        } else if (button == CODEIN_RECT_OK) {
            wk->cur[CODEIN_CURSOR_KEY].move.x = 3;
            wk->cur[CODEIN_CURSOR_KEY].move.y = 2;
            if (CodeIn_IsNumberValid(wk)) {
                CodeIn_Confirm(wk);
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            } else {
                CodeIn_SetState(wk, CODEIN_STATE_ERROR);
            }
        } else if (button >= CODEIN_RECT_KEYS && button <= CODEIN_RECT_BACK - 1) {
            int digit;
            int key;
            int group;
            int next;
            int nextGroup;

            if (wk->focus != 0) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                digit = wk->cur[CODEIN_CURSOR_CODE].state;
                wk->code[digit].state = button - CODEIN_RECT_KEYS + 1;
                ClActor_SetSequence(wk->code[digit].actor,
                                    CodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size));
                CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, TRUE);
                key = button - CODEIN_RECT_KEYS;
                CodeInDisp_SetKeyCursor(wk, key);
                CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_KEY, FALSE);
                CodeInDisp_SetCursorVisible(wk, CODEIN_CURSOR_PRESS, TRUE);
                CodeInDisp_SetCursorOnKey(wk, key, CODEIN_CURSOR_PRESS);
                ClActor_SetSequence(wk->cur[CODEIN_CURSOR_PRESS].actor, 3);
                group = wk->code[digit].group;
                next = digit + 1;
                if (next == wk->codeMax) {
                    wk->next = CODEIN_NEXT_FOCUS;
                    wk->nextWork = 0;
                    wk->nextLast = FALSE;
                } else {
                    nextGroup = wk->code[next].group;
                    if (group != nextGroup) {
                        wk->next = CODEIN_NEXT_FOCUS;
                        wk->nextWork = nextGroup;
                        wk->nextLast = FALSE;
                    } else {
                        wk->next = CODEIN_NEXT_CURSOR;
                        wk->nextWork = next;
                    }
                }
                CodeIn_SetKeyCursorByRect(wk, button);
            }
        } else {
            wk->cur[CODEIN_CURSOR_KEY].move.x = (button - CODEIN_RECT_KEYS) % KEY_COLUMNS;
            wk->cur[CODEIN_CURSOR_KEY].move.y = (button - CODEIN_RECT_KEYS) / KEY_COLUMNS;
        }
    }
}

// Does what the input asked for: a change of focus, which the move state animates, or a step of the cursor
static void CodeIn_RunNext(CodeInWork *wk) {
    switch (wk->next) {
    case CODEIN_NEXT_NONE:
        break;
    case CODEIN_NEXT_FOCUS:
        CodeIn_SetFocus(wk, wk->nextWork);
        CodeInDisp_SetCodeSizes(wk);
        CodeInDisp_SetCodePositions(wk, TRUE);
        CodeIn_SetState(wk, CODEIN_STATE_MOVE);
        wk->next = CODEIN_NEXT_WAIT;
        break;
    case CODEIN_NEXT_CURSOR:
        CodeInDisp_SetCodeCursor(wk, wk->nextWork);
        CodeIn_ClearNext(wk);
        break;
    case CODEIN_NEXT_WAIT:
        break;
    }
}

static void CodeIn_ClearNext(CodeInWork *wk) {
    wk->next = CODEIN_NEXT_NONE;
    wk->nextWork = 0;
    wk->nextLast = FALSE;
}

static int CodeIn_FindGroupFirst(CodeInWork *wk, int group) {
    int i;

    for (i = 0; i < wk->codeMax; i++) {
        if (wk->code[i].group == group) {
            return i;
        }
    }
    return 0;
}

static int CodeIn_FindGroupLast(CodeInWork *wk, int group) {
    int i;
    BOOL found = FALSE;

    for (i = 0; i < wk->codeMax; i++) {
        if (wk->code[i].group == group) {
            found = TRUE;
        } else if (found == TRUE) {
            return i - 1;
        }
    }
    return wk->codeMax - 1;
}
