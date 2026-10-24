#include "types.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/wipe_wnd.h"

// The windows the screen wipes draw with: each window register write, made at once or kept in the WipeWnd and made
// by a task at the next VBlank. The file's name is a guess: the ROM has no string for it

static void WipeWnd_SetVisibleTask(TCB *tcb, void *data);
static void WipeWnd_SetInsidePlaneTask(TCB *tcb, void *data);
static void WipeWnd_SetOutsidePlaneTask(TCB *tcb, void *data);
static void WipeWnd_SetPositionTask(TCB *tcb, void *data);

void WipeWnd_SetVisible(u32 visible, u32 screen) {
    if (screen == 0) {
        GX_SetVisibleWnd(visible);
    } else {
        GXS_SetVisibleWnd(visible);
    }
}

void WipeWnd_SetInsidePlane(u32 planes, BOOL effect, u32 window, u32 screen) {
    if (window == 0) {
        if (screen == 0) {
            G2_SetWnd0InsidePlane(planes, effect);
        } else {
            G2S_SetWnd0InsidePlane(planes, effect);
        }
    } else {
        if (screen == 0) {
            G2_SetWnd1InsidePlane(planes, effect);
        } else {
            G2S_SetWnd1InsidePlane(planes, effect);
        }
    }
}

void WipeWnd_SetOutsidePlane(u32 planes, BOOL effect, u32 screen) {
    if (screen == 0) {
        G2_SetWndOutsidePlane(planes, effect);
    } else {
        G2S_SetWndOutsidePlane(planes, effect);
    }
}

// Dead-stripped, as config/deadstrip.toml lists: the planes inside a window and outside the windows, read back from the
// registers. Only wipe_sub.c's dead-stripped plane swap reads them, so the original link dropped both, and the ROM
// has neither. They are reconstructed from HGSS's window module (pret's pokeheartgold, sub_020132E8 and sub_0201333C
// in asm/unk_02012DD8.s, between the counterparts of WipeWnd_SetOutsidePlane and WipeWnd_SetPosition), in the shape of
// this file's setters, and can't be checked against BW2's
GXWndPlane WipeWnd_GetInsidePlane(u32 window, u32 screen) {
    if (window == 0) {
        if (screen == 0) {
            return G2_GetWnd0InsidePlane();
        } else {
            return G2S_GetWnd0InsidePlane();
        }
    } else {
        if (screen == 0) {
            return G2_GetWnd1InsidePlane();
        } else {
            return G2S_GetWnd1InsidePlane();
        }
    }
}

GXWndPlane WipeWnd_GetOutsidePlane(u32 screen) {
    if (screen == 0) {
        return G2_GetWndOutsidePlane();
    } else {
        return G2S_GetWndOutsidePlane();
    }
}

void WipeWnd_SetPosition(int x1, int y1, int x2, int y2, u32 window, u32 screen) {
    if (window == 0) {
        if (screen == 0) {
            G2_SetWnd0Position(x1, y1, x2, y2);
        } else {
            G2S_SetWnd0Position(x1, y1, x2, y2);
        }
    } else {
        if (screen == 0) {
            G2_SetWnd1Position(x1, y1, x2, y2);
        } else {
            G2S_SetWnd1Position(x1, y1, x2, y2);
        }
    }
}

void WipeWnd_SetVisibleAtVBlank(WipeWnd *wnd, u32 visible, u32 screen) {
    WipeWndDisp *disp = &wnd->disp[screen];

    disp->visible = visible;
    disp->screen = screen;
    GFL_VBlankTCBAdd(WipeWnd_SetVisibleTask, disp, 1);
}

void WipeWnd_SetInsidePlaneAtVBlank(WipeWnd *wnd, u32 planes, BOOL effect, u32 window, u32 screen) {
    WipeWndIn *in = &wnd->in[screen][window];

    in->planes = planes;
    in->effect = effect;
    in->window = window;
    in->screen = screen;
    GFL_VBlankTCBAdd(WipeWnd_SetInsidePlaneTask, in, 1);
}

void WipeWnd_SetOutsidePlaneAtVBlank(WipeWnd *wnd, u32 planes, BOOL effect, u32 screen) {
    WipeWndOut *out = &wnd->out[screen];

    out->planes = planes;
    out->effect = effect;
    out->screen = screen;
    GFL_VBlankTCBAdd(WipeWnd_SetOutsidePlaneTask, out, 1);
}

void WipeWnd_SetPositionAtVBlank(WipeWnd *wnd, int x1, int y1, int x2, int y2, u32 window, u32 screen) {
    WipeWndPos *pos = &wnd->pos[screen][window];

    pos->x1 = x1;
    pos->y1 = y1;
    pos->x2 = x2;
    pos->y2 = y2;
    pos->window = window;
    pos->screen = screen;
    GFL_VBlankTCBAdd(WipeWnd_SetPositionTask, pos, 1);
}

static void WipeWnd_SetVisibleTask(TCB *tcb, void *data) {
    WipeWndDisp *disp = data;

    WipeWnd_SetVisible(disp->visible, disp->screen);
    GFL_TCBRemove(tcb);
}

static void WipeWnd_SetInsidePlaneTask(TCB *tcb, void *data) {
    WipeWndIn *in = data;

    WipeWnd_SetInsidePlane(in->planes, in->effect, in->window, in->screen);
    GFL_TCBRemove(tcb);
}

static void WipeWnd_SetOutsidePlaneTask(TCB *tcb, void *data) {
    WipeWndOut *out = data;

    WipeWnd_SetOutsidePlane(out->planes, out->effect, out->screen);
    GFL_TCBRemove(tcb);
}

static void WipeWnd_SetPositionTask(TCB *tcb, void *data) {
    WipeWndPos *pos = data;

    WipeWnd_SetPosition(pos->x1, pos->y1, pos->x2, pos->y2, pos->window, pos->screen);
    GFL_TCBRemove(tcb);
}
