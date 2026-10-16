#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_obj.h"
#include "app/wf2dmap/wf2dmap_scroll.h"
#include "gfl/std.h"

// Where the Wi-Fi Club's 2D map is scrolled to. The ROM has no name for this file: it is named after the older games'
// wf2dmap_scroll.c, and its first function is their wf2dmap_scrcont.c's

// Where on the screen the object that the map follows is drawn
#define WF2DMAP_SCRCONT_CENTER_X 112
#define WF2DMAP_SCRCONT_CENTER_Y 96

void WF2DMAP_SCRContSysMain(WF2DMAP_SCROLL *scroll, const WF2DMAP_OBJWK *wk) {
    WF2DMAP_POS pos;

    pos = WF2DMAP_OBJWkFrameMatrixGet(wk);
    WF2DMAP_SCROLLSysDataSet(scroll, pos.y - WF2DMAP_SCRCONT_CENTER_Y, pos.x - WF2DMAP_SCRCONT_CENTER_X);
}

void WF2DMAP_SCROLLSysDataInit(WF2DMAP_SCROLL *scroll) {
    MI_CpuFill8(scroll, 0, sizeof(WF2DMAP_SCROLL));
}

void WF2DMAP_SCROLLSysDataSet(WF2DMAP_SCROLL *scroll, s16 top, s16 left) {
    scroll->top = top;
    scroll->left = left;
}

s16 WF2DMAP_SCROLLSysTopGet(const WF2DMAP_SCROLL *scroll) {
    return scroll->top;
}

s16 WF2DMAP_SCROLLSysLeftGet(const WF2DMAP_SCROLL *scroll) {
    return scroll->left;
}
