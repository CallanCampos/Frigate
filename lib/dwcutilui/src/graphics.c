#include "graphics.h"
#include "nitro/gx.h"

// Adds the planes to the screen's visible ones
void DWCi_GXlVisible(int screen, int planes) {
    if (screen == 1) {
        GXS_SetVisiblePlane(planes | GXS_GetVisiblePlane());
    } else {
        GX_SetVisiblePlane(planes | GX_GetVisiblePlane());
    }
}

// Hides the planes on the screen
void DWCi_GXlInvisible(int screen, int planes) {
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
