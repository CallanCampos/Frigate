#include "dwc_util_display.h"
#include "nitro/gx.h"

void DWCUtilDisplay_SetPoint(u16 x, u16 y, u16 *point) {
    point[0] = x;
    point[1] = y;
}

void DWCUtilDisplay_SetRect(int left, int top, int right, int bottom, u16 *rect) {
    rect[0] = left;
    rect[1] = top;
    rect[2] = right;
    rect[3] = bottom;
}

void DWCUtilDisplay_SetRectFromSize(const void *position, const void *size, void *rect) {
    const u16 *pos = position;
    const u16 *extent = size;
    u16 *out = rect;

    out[0] = pos[0];
    out[1] = pos[1];
    out[2] = pos[0] + extent[0];
    out[3] = pos[1] + extent[1];
}

// Adds the planes to the screen's visible ones
void DWCi_GXlInvisible(int screen, int planes) {
    if (screen == 1) {
        GXS_SetVisiblePlane(planes | GXS_GetVisiblePlane());
    } else {
        GX_SetVisiblePlane(planes | GX_GetVisiblePlane());
    }
}

// Hides the planes on the screen
void DWCUtilDisplay_HidePlanes(int screen, int planes) {
    if (screen == 1) {
        GXS_SetVisiblePlane(GXS_GetVisiblePlane() & ~planes);
    } else {
        GX_SetVisiblePlane(GX_GetVisiblePlane() & ~planes);
    }
}

void DWCi_GXlSetWndPosition(int screen, int window, const u16 *rect) {
    if (screen == 1) {
        if (window == 0) {
            G2S_SetWnd0Position(rect[0], rect[1], rect[2], rect[3]);
        } else {
            G2S_SetWnd1Position(rect[0], rect[1], rect[2], rect[3]);
        }
    } else {
        if (window == 0) {
            G2_SetWnd0Position(rect[0], rect[1], rect[2], rect[3]);
        } else {
            G2_SetWnd1Position(rect[0], rect[1], rect[2], rect[3]);
        }
    }
}

void DWCi_GXlSetWndPlane(int screen, int window, int planes, BOOL effect) {
    switch (window) {
    case 0:
        if (screen == 1) {
            G2S_SetWnd0InsidePlane(planes, effect);
        } else {
            G2_SetWnd0InsidePlane(planes, effect);
        }
        break;
    case 1:
        if (screen == 1) {
            G2S_SetWnd1InsidePlane(planes, effect);
        } else {
            G2_SetWnd1InsidePlane(planes, effect);
        }
        break;
    case 2:
        if (screen == 1) {
            G2S_SetWndOBJInsidePlane(planes, effect);
        } else {
            G2_SetWndOBJInsidePlane(planes, effect);
        }
        break;
    case 3:
        if (screen == 1) {
            G2S_SetWndOutsidePlane(planes, effect);
        } else {
            G2_SetWndOutsidePlane(planes, effect);
        }
        break;
    }
}
