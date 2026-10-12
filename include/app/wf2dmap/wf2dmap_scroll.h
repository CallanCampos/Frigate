#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_SCROLL_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_SCROLL_H

// Where the Wi-Fi Club's 2D map (overlay 203) is scrolled to. The ROM has no name for this file: it is named after
// the older games' wf2dmap_scroll.c (the scroll data) and wf2dmap_scrcont.c (following an object), whose functions it
// holds

#include "types.h"
#include "app/wf2dmap/wf2dmap_obj.h"

// The screen's top left corner on the map, in pixels
typedef struct {
    s16 top;
    s16 left;
} WF2DMAP_SCROLL;

// Centers the screen on an object
void WF2DMAP_SCRContSysMain(WF2DMAP_SCROLL *scroll, const WF2DMAP_OBJWK *wk);
void WF2DMAP_SCROLLSysDataInit(WF2DMAP_SCROLL *scroll);
void WF2DMAP_SCROLLSysDataSet(WF2DMAP_SCROLL *scroll, s16 top, s16 left);
s16 WF2DMAP_SCROLLSysTopGet(const WF2DMAP_SCROLL *scroll);
s16 WF2DMAP_SCROLLSysLeftGet(const WF2DMAP_SCROLL *scroll);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_SCROLL_H
