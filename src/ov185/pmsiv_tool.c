#include "app/pmsiv_tool.h"
#include "types.h"
#include "app/pms_input_view.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/tcb.h"
#include "nitro/hw.h"

// The phrase input's BG effects: a BG scrolled by a step each frame, and a blend or a brightness changed by a step
// each frame, run by V-blank tasks. The ROM doesn't name this file; pmsiv_tool.c is a guess after its neighbors. The
// names are ours, guessed

static void PMSIVTool_ScrollTask(TCB *tcb, void *data);
static void PMSIVTool_BlendTask(TCB *tcb, void *data);
static void PMSIVTool_BrightTask(TCB *tcb, void *data);

const ClActorPos PMSIV_CURSOR_HIDE_POS = { 400, 300 };

void PMSIVTool_SetupScrollWork(PMSIVToolScrollWork *wk, u32 bg, int direction, int vector, int wait) {
    wk->bg = bg;
    if (direction == PMSIV_TOOL_SCROLL_X) {
        wk->pos = GFL_BGSysGetBGOffsetX2(bg);
        wk->op = BG_MOVE_SET_X;
    } else {
        wk->pos = GFL_BGSysGetBGOffsetY2(bg);
        wk->op = BG_MOVE_SET_Y;
    }
    wk->endPos = (wk->pos + vector) & 0x1ff;
    wk->pos <<= FX32_SHIFT;
    wk->step = (vector << FX32_SHIFT) / wait;
    wk->timer = wait;
    wk->seq = 0;
}

BOOL PMSIVTool_WaitScroll(PMSIVToolScrollWork *wk) {
    switch (wk->seq) {
    case 0:
        wk->tcb = PMSIView_AddVTask(PMSIVTool_ScrollTask, wk, 0);
        wk->seq++;
        break;
    case 1:
        if (wk->timer == 0) {
            GFL_TCBRemove(wk->tcb);
            wk->seq++;
            return TRUE;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static void PMSIVTool_ScrollTask(TCB *tcb, void *data) {
    PMSIVToolScrollWork *wk = data;

    if (wk->timer) {
        wk->pos += wk->step;
        GFL_BGSysMoveBG(wk->bg, wk->op, wk->pos >> FX32_SHIFT);
        wk->timer--;
    } else {
        GFL_BGSysMoveBG(wk->bg, wk->op, wk->endPos);
    }
}

void PMSIVTool_SetupBlendWork(PMSIVToolBlendWork *wk, int plane1, int plane2, int start, int end, int wait) {
    wk->plane1 = plane1;
    wk->plane2 = plane2;
    wk->wait = wait;
    wk->ev = start;
    wk->evStep = (end - start) / wait;
    wk->evEnd = end;
    wk->seq = 0;
    wk->tcb = PMSIView_AddVTask(PMSIVTool_BlendTask, wk, 0);
}

BOOL PMSIVTool_WaitBlend(PMSIVToolBlendWork *wk) {
    if (wk->seq == 0) {
        if (wk->wait == 0) {
            GFL_TCBRemove(wk->tcb);
            wk->seq++;
            return TRUE;
        }
    } else {
        return TRUE;
    }
    return FALSE;
}

static void PMSIVTool_BlendTask(TCB *tcb, void *data) {
    PMSIVToolBlendWork *wk = data;
    int ev;

    if (wk->wait) {
        wk->ev += wk->evStep;
        ev = wk->ev >> 3;
        wk->wait--;
    } else {
        ev = wk->evEnd >> 3;
    }
    if (ev > 16) {
        ev = 16;
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, wk->plane1, wk->plane2, ev, 16 - ev);
}

void PMSIVTool_SetupBrightWork(PMSIVToolBlendWork *wk, int plane, int start, int end, int wait) {
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, plane, start);
    wk->plane1 = plane;
    wk->wait = wait;
    wk->ev = start << FX32_SHIFT;
    wk->evEnd = end << FX32_SHIFT;
    wk->evStep = (wk->evEnd - wk->ev) / wait;
    wk->seq = 0;
    wk->tcb = PMSIView_AddVTask(PMSIVTool_BrightTask, wk, 0);
}

BOOL PMSIVTool_WaitBright(PMSIVToolBlendWork *wk) {
    if (wk->seq == 0) {
        if (wk->wait == 0) {
            GFL_TCBRemove(wk->tcb);
            wk->seq++;
            return TRUE;
        }
    } else {
        return TRUE;
    }
    return FALSE;
}

static void PMSIVTool_BrightTask(TCB *tcb, void *data) {
    PMSIVToolBlendWork *wk = data;
    int brightness;

    if (wk->wait) {
        wk->ev += wk->evStep;
        brightness = wk->ev >> FX32_SHIFT;
        wk->wait--;
    } else {
        brightness = wk->evEnd >> FX32_SHIFT;
    }
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, wk->plane1, brightness);
}
