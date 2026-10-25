#include "foundation.h"

void DWCi_InputCoord(u16 x, u16 y, u16 *point) {
    point[0] = x;
    point[1] = y;
}

void DWCi_InputRect(int left, int top, int right, int bottom, u16 *rect) {
    rect[0] = left;
    rect[1] = top;
    rect[2] = right;
    rect[3] = bottom;
}

void DWCi_Coord2Rect(const void *position, const void *size, void *rect) {
    const u16 *pos = position;
    const u16 *extent = size;
    u16 *out = rect;

    out[0] = pos[0];
    out[1] = pos[1];
    out[2] = pos[0] + extent[0];
    out[3] = pos[1] + extent[1];
}
