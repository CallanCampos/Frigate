// The states and the input of the Battle Recorder's code input, which is by touch only. The ROM has no name for this
// file; "br_codein_pv.c" is a guess after overlay 205's codein_pv.c, which it is a copy of. Our names

#include "types.h"
#include "app/battle_recorder/br_codein.h"
#include "app/battle_recorder/br_codein_pv.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "system/str_tool.h"

#define KEY_COLUMNS 5
// The frames of a change of focus: the digits move, then finish scaling
#define SCALE_COUNT 6

typedef BOOL (*BrCodeInStateFunc)(BrCodeInWork *wk);

// A key's center and half its size
typedef struct {
    s16 x;
    s16 y;
    s16 halfWidth;
    s16 halfHeight;
} BrCodeInKeyRect;

static void BrCodeIn_SetFocus(BrCodeInWork *wk, int focus);
static void BrCodeIn_SetState(BrCodeInWork *wk, int state);
static BOOL BrCodeIn_StateInit(BrCodeInWork *wk);
static BOOL BrCodeIn_StateEnd(BrCodeInWork *wk);
static BOOL BrCodeIn_StateInput(BrCodeInWork *wk);
static BOOL BrCodeIn_StateMove(BrCodeInWork *wk);
static void BrCodeIn_Confirm(BrCodeInWork *wk);
static void BrCodeIn_Backspace(BrCodeInWork *wk);
static void BrCodeIn_ButtonCallback(u32 button, u32 event, void *work);
static void BrCodeIn_RunNext(BrCodeInWork *wk);
static void BrCodeIn_ClearNext(BrCodeInWork *wk);
static int BrCodeIn_FindGroupFirst(BrCodeInWork *wk, int group);
static int BrCodeIn_FindGroupLast(BrCodeInWork *wk, int group);

// By BR_CODEIN_STATE_*
static const BrCodeInStateFunc sBrCodeInStates[] = {
    BrCodeIn_StateInit,
    BrCodeIn_StateInput,
    BrCodeIn_StateMove,
    BrCodeIn_StateEnd,
};

// The scales of a digit as it grows and as it shrinks
static fx32 sBrCodeInScalesLarge[] = { 0x800, 0x333, 0x800, FX32_ONE, 0x1333, FX32_ONE, FX32_ONE };
static fx32 sBrCodeInScalesSmall[] = { 0xccd, 0x99a, 0x666, 0x333, 0xccd, FX32_ONE, FX32_ONE };

static void BrCodeIn_SetFocus(BrCodeInWork *wk, int focus) {
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

void BrCodeIn_InitParam(BrCodeInWork *wk) {
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
    BrCodeIn_SetFocus(wk, 1);
    for (i = 0; i < BR_CODEIN_BLOCK_COUNT; i++) {
        wk->codeMax += wk->param.blocks[i];
    }
    {
        int kind = 0;
        // For a Friend Code and for another number, as a battle video's is
        int focusX[2][BR_CODEIN_BLOCK_COUNT] = { { 56, 20, 8 }, { 56, 12, 0 } };

        if (wk->param.blocks[0] != 4) {
            kind = 1;
        }
        for (i = 0; i < BR_CODEIN_BLOCK_COUNT; i++) {
            wk->focusX[i] = focusX[kind][i];
        }
        wk->focusX[i] = focusX[kind][i - 1];
    }
    count = 0;
    for (i = 0; i < BR_CODEIN_BAR_COUNT; i++) {
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

static void BrCodeIn_SetState(BrCodeInWork *wk, int state) {
    wk->state = state;
    wk->unk230 = 0;
    wk->seq = 0;
    wk->count = 0;
}

static BOOL BrCodeIn_StateInit(BrCodeInWork *wk) {
    BrCodeInDisp_GetResources(wk);
    BrCodeInDisp_SetCodeSizes(wk);
    BrCodeInDisp_AddCodeActors(wk);
    BrCodeInDisp_SetCodePositions(wk, FALSE);
    BrCodeInDisp_AddCursorActors(wk);
    BrCodeInDisp_AddButtonActors(wk);
    BrCodeInDisp_UpdateCodeRects(wk);
    BrCodeInDisp_InitBmpOam(wk);
    BrCodeInDisp_AddButtonTexts(wk);
    BrCodeIn_SetState(wk, BR_CODEIN_STATE_INPUT);
    return FALSE;
}

static BOOL BrCodeIn_StateEnd(BrCodeInWork *wk) {
    return TRUE;
}

static BOOL BrCodeIn_StateInput(BrCodeInWork *wk) {
    if (wk->seq == 0) {
        wk->seq++;
    } else {
        BrCodeIn_RunNext(wk);
        GFL_BMN_Main(wk->buttons);
    }
    return FALSE;
}

static BOOL BrCodeIn_StateMove(BrCodeInWork *wk) {
    int i;
    ClActorScale scale;
    ClActorPos pos;
    ClActorPos barPos;

    switch (wk->seq) {
    case 0:
        BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_CODE, FALSE);
        for (i = 0; i < wk->codeMax; i++) {
            if (wk->code[i].move.wait == 0) {
                continue;
            }
            func_0204c178(wk->code[i].actor, &pos, CLACT_SURFACE_SUB);
            pos.x += wk->code[i].move.x;
            pos.y += wk->code[i].move.y;
            func_0204c140(wk->code[i].actor, &pos, CLACT_SURFACE_SUB);
            wk->code[i].move.wait--;
            if (i >= wk->largeStart && i < wk->largeEnd) {
                scale.x = sBrCodeInScalesLarge[wk->code[i].move.scale];
                scale.y = sBrCodeInScalesLarge[wk->code[i].move.scale];
                func_0204c270(wk->code[i].actor, &scale);
                wk->code[i].move.scale++;
            }
            if (i >= wk->smallStart && i < wk->smallEnd) {
                scale.x = sBrCodeInScalesSmall[wk->code[i].move.scale];
                scale.y = sBrCodeInScalesSmall[wk->code[i].move.scale];
                func_0204c270(wk->code[i].actor, &scale);
                wk->code[i].move.scale++;
            }
        }
        for (i = 0; i < BR_CODEIN_BAR_COUNT; i++) {
            if (wk->bar[i].move.wait == 0) {
                continue;
            }
            func_0204c178(wk->bar[i].actor, &barPos, CLACT_SURFACE_SUB);
            barPos.x += wk->bar[i].move.x;
            barPos.y += wk->bar[i].move.y;
            func_0204c140(wk->bar[i].actor, &barPos, CLACT_SURFACE_SUB);
            wk->bar[i].move.wait--;
        }
        if (wk->code[0].move.wait == 0) {
            for (i = wk->largeStart; i < wk->largeEnd; i++) {
                func_0204c488(wk->code[i].actor, BrCodeInDisp_GetCodeSequence(wk->code[i].state, wk->code[i].size));
            }
            for (i = wk->smallStart; i < wk->smallEnd; i++) {
                func_0204c488(wk->code[i].actor, BrCodeInDisp_GetCodeSequence(wk->code[i].state, wk->code[i].size));
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
            scale.x = sBrCodeInScalesLarge[wk->code[i].move.scale];
            scale.y = sBrCodeInScalesLarge[wk->code[i].move.scale];
            func_0204c270(wk->code[i].actor, &scale);
            wk->code[i].move.scale++;
        }
        for (i = wk->smallStart; i < wk->smallEnd; i++) {
            if (wk->code[i].move.scale == SCALE_COUNT) {
                continue;
            }
            scale.x = sBrCodeInScalesSmall[wk->code[i].move.scale];
            scale.y = sBrCodeInScalesSmall[wk->code[i].move.scale];
            func_0204c270(wk->code[i].actor, &scale);
            wk->code[i].move.scale++;
        }
        wk->count++;
        if (wk->count == SCALE_COUNT) {
            wk->seq++;
        }
        break;
    default:
        BrCodeInDisp_UpdateCodeRects(wk);
        if (wk->nextLast == FALSE) {
            i = BrCodeIn_FindGroupFirst(wk, wk->nextWork);
        } else {
            i = BrCodeIn_FindGroupLast(wk, wk->nextWork);
        }
        BrCodeInDisp_SetCodeCursor(wk, i);
        if (wk->focus != 0) {
            BrCodeInDisp_SetCursorVisible(wk, BR_CODEIN_CURSOR_CODE, TRUE);
        }
        BrCodeIn_ClearNext(wk);
        BrCodeIn_SetState(wk, BR_CODEIN_STATE_INPUT);
        break;
    }
    return FALSE;
}

BOOL BrCodeIn_MainPv(BrCodeInWork *wk) {
    BOOL done = sBrCodeInStates[wk->state](wk);

    BrCodeInDisp_UpdateCursors(wk);
    return done;
}

// Writes the digits to the parameter's string, empty ones as 0, works out the number and each group's, and ends the
// input
static void BrCodeIn_Confirm(BrCodeInWork *wk) {
    int i;
    int block = 0;
    int blockCount = 0;
    StrBuf *digit = GFL_StrBufCreate(100, wk->heapId);

    wk->btn[1].state = 1;
    wk->btn[1].move.wait = 0;
    wk->blockNumber[0] = 0;
    wk->blockNumber[1] = 0;
    wk->blockNumber[2] = 0;
    wk->number = 0;
    for (i = 0; i < wk->codeMax; i++) {
        u32 value;

        if (wk->code[i].state == 0) {
            wk->code[i].state = 1;
            func_0204c488(wk->code[i].actor, BrCodeInDisp_GetCodeSequence(wk->code[i].state, wk->code[i].size));
        }
        value = wk->code[i].state - 1;
        GFL_WordSetFormatNumber(digit, value, 1, 1, TRUE);
        GFL_StrBufConcat(wk->param.strbuf, digit);
        wk->blockNumber[block] *= 10;
        wk->blockNumber[block] += value;
        blockCount++;
        if (blockCount >= wk->param.blocks[block]) {
            block++;
            blockCount = 0;
        }
        wk->number *= 10;
        wk->number += value;
    }
    GFL_StrBufFree(digit);
    BrCodeIn_SetState(wk, BR_CODEIN_STATE_END);
    wk->result = BR_CODEIN_RESULT_DECIDE;
}

// Clears the cursor's digit and steps back, or goes to the last digit when nothing is focused. At the first digit
// it ends the input instead
static void BrCodeIn_Backspace(BrCodeInWork *wk) {
    int digit;
    int group;
    int prevGroup;

    wk->btn[0].state = 1;
    wk->btn[0].move.wait = 0;
    if (wk->focus == 0) {
        wk->cur[BR_CODEIN_CURSOR_CODE].state = wk->codeMax - 1;
        group = wk->code[wk->cur[BR_CODEIN_CURSOR_CODE].state].group;
        wk->next = BR_CODEIN_NEXT_FOCUS;
        wk->nextWork = group;
        wk->nextLast = TRUE;
        return;
    }
    digit = wk->cur[BR_CODEIN_CURSOR_CODE].state;
    wk->code[digit].state = 0;
    func_0204c488(wk->code[digit].actor, BrCodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size));
    group = wk->code[digit].group;
    if (digit > 0) {
        func_0204c488(wk->code[digit - 1].actor,
                      BrCodeInDisp_GetCodeSequence(wk->code[digit - 1].state, wk->code[digit - 1].size));
        prevGroup = wk->code[digit - 1].group;
        if (group != prevGroup) {
            wk->next = BR_CODEIN_NEXT_FOCUS;
            wk->nextWork = prevGroup;
            wk->nextLast = TRUE;
        } else {
            wk->next = BR_CODEIN_NEXT_CURSOR;
            wk->nextWork = digit - 1;
        }
    } else {
        wk->result = BR_CODEIN_RESULT_CANCEL;
    }
}

void BrCodeIn_InitButtons(BrCodeInWork *wk) {
    int i;

    for (i = 0; i < BR_CODEIN_CODE_MAX; i++) {
        wk->code[i].rect = &wk->rects[i];
    }
    {
        BrCodeInKeyRect keys[BR_CODEIN_KEY_COUNT] = {
            { 32, 80, 20, 20 },   { 80, 80, 20, 20 },   { 128, 80, 20, 20 }, { 176, 80, 20, 20 },
            { 224, 80, 20, 20 },  { 32, 128, 20, 20 },  { 80, 128, 20, 20 }, { 128, 128, 20, 20 },
            { 176, 128, 20, 20 }, { 224, 128, 20, 20 }, { 80, 168, 48, 16 }, { 176, 168, 48, 16 },
        };

        for (; i < BR_CODEIN_RECT_COUNT; i++) {
            wk->rects[i].top = keys[i - BR_CODEIN_RECT_KEYS].y - keys[i - BR_CODEIN_RECT_KEYS].halfHeight;
            wk->rects[i].left = keys[i - BR_CODEIN_RECT_KEYS].x - keys[i - BR_CODEIN_RECT_KEYS].halfWidth;
            wk->rects[i].bottom = keys[i - BR_CODEIN_RECT_KEYS].y + keys[i - BR_CODEIN_RECT_KEYS].halfHeight;
            wk->rects[i].right = keys[i - BR_CODEIN_RECT_KEYS].x + keys[i - BR_CODEIN_RECT_KEYS].halfWidth;
        }
    }
    wk->rects[BR_CODEIN_RECT_COUNT].top = TOUCH_RECT_END;
    wk->buttons = GFL_BMN_Create(wk->rects, BrCodeIn_ButtonCallback, wk, wk->heapId);
}

static void BrCodeIn_ButtonCallback(u32 button, u32 event, void *work) {
    BrCodeInWork *wk = work;

    if (wk->state != BR_CODEIN_STATE_INPUT) {
        return;
    }
    if (event == BMN_EVENT_TOUCH) {
        if (button <= BR_CODEIN_RECT_KEYS - 1) {
            if (wk->code[button].size == 1) {
                wk->next = BR_CODEIN_NEXT_CURSOR;
                wk->nextWork = button;
            } else {
                wk->next = BR_CODEIN_NEXT_FOCUS;
                wk->nextWork = wk->code[button].group;
            }
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            return;
        } else if (button == BR_CODEIN_RECT_BACK) {
            u32 x;
            u32 y;
            BrPoint pos;

            wk->cur[BR_CODEIN_CURSOR_KEY].move.x = 0;
            wk->cur[BR_CODEIN_CURSOR_KEY].move.y = 2;
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->param.ballEff, BR_BALL_EFFECT_SPREAD, &pos);
        } else if (button == BR_CODEIN_RECT_OK) {
            u32 x;
            u32 y;
            BrPoint pos;

            wk->cur[BR_CODEIN_CURSOR_KEY].move.x = 3;
            wk->cur[BR_CODEIN_CURSOR_KEY].move.y = 2;
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->param.ballEff, BR_BALL_EFFECT_SPREAD, &pos);
        } else {
            wk->cur[BR_CODEIN_CURSOR_KEY].move.x = (button - BR_CODEIN_RECT_KEYS) % KEY_COLUMNS;
            wk->cur[BR_CODEIN_CURSOR_KEY].move.y = (button - BR_CODEIN_RECT_KEYS) / KEY_COLUMNS;
            GFL_SndSEPlay(SEQ_SE_BREC_06);
        }
        if (button >= BR_CODEIN_RECT_KEYS && button <= BR_CODEIN_RECT_BACK - 1) {
            u32 x;
            u32 y;
            BrPoint pos;
            int digit;
            int group;
            int next;
            int nextGroup;

            if (wk->focus != 0) {
                func_0203dac8(&x, &y);
                pos.x = x;
                pos.y = y;
                BrBallEff_Start(wk->param.ballEff, BR_BALL_EFFECT_SPREAD, &pos);
                digit = wk->cur[BR_CODEIN_CURSOR_CODE].state;
                wk->code[digit].state = button - BR_CODEIN_RECT_KEYS + 1;
                func_0204c488(wk->code[digit].actor,
                              BrCodeInDisp_GetCodeSequence(wk->code[digit].state, wk->code[digit].size));
                group = wk->code[digit].group;
                next = digit + 1;
                if (next == wk->codeMax) {
                    wk->next = BR_CODEIN_NEXT_FOCUS;
                    wk->nextWork = 0;
                    wk->nextLast = FALSE;
                } else {
                    nextGroup = wk->code[next].group;
                    if (group != nextGroup) {
                        wk->next = BR_CODEIN_NEXT_FOCUS;
                        wk->nextWork = nextGroup;
                        wk->nextLast = FALSE;
                    } else {
                        wk->next = BR_CODEIN_NEXT_CURSOR;
                        wk->nextWork = next;
                    }
                }
            }
        } else if (button == BR_CODEIN_RECT_BACK) {
            BrCodeIn_Backspace(wk);
        } else {
            BrCodeIn_Confirm(wk);
        }
    }
}

// Does what the input asked for: a change of focus, which the move state animates, or a step of the cursor
static void BrCodeIn_RunNext(BrCodeInWork *wk) {
    switch (wk->next) {
    case BR_CODEIN_NEXT_NONE:
        break;
    case BR_CODEIN_NEXT_FOCUS:
        BrCodeIn_SetFocus(wk, wk->nextWork);
        BrCodeInDisp_SetCodeSizes(wk);
        BrCodeInDisp_SetCodePositions(wk, TRUE);
        BrCodeIn_SetState(wk, BR_CODEIN_STATE_MOVE);
        wk->next = BR_CODEIN_NEXT_WAIT;
        break;
    case BR_CODEIN_NEXT_CURSOR:
        BrCodeInDisp_SetCodeCursor(wk, wk->nextWork);
        BrCodeIn_ClearNext(wk);
        break;
    case BR_CODEIN_NEXT_WAIT:
        break;
    }
}

static void BrCodeIn_ClearNext(BrCodeInWork *wk) {
    wk->next = BR_CODEIN_NEXT_NONE;
    wk->nextWork = 0;
    wk->nextLast = FALSE;
}

static int BrCodeIn_FindGroupFirst(BrCodeInWork *wk, int group) {
    int i;

    for (i = 0; i < wk->codeMax; i++) {
        if (wk->code[i].group == group) {
            return i;
        }
    }
    return 0;
}

static int BrCodeIn_FindGroupLast(BrCodeInWork *wk, int group) {
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
