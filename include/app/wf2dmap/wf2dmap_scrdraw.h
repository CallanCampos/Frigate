#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_SCRDRAW_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_SCRDRAW_H

// Draws the Wi-Fi Club's 2D map (overlay 203) on a BG and scrolls it and a cell actor renderer's surface, named after
// the ROM's "wf2dmap_scrdraw.c"

#include "types.h"
#include "app/wf2dmap/wf2dmap_scroll.h"
#include "gfl/clact.h"

typedef struct {
    // 0 for the main screen's surface of the renderer, or else the sub screen's
    u8 surface;
    u8 bg;
    // The BG's BGSetup values
    u8 colorMode;
    u8 screenBase;
    u8 charBase;
    u8 extPaletteSlot;
    u8 priority;
    u8 mosaic;
    // The map's screen (NSCR)
    u8 arcId;
    u8 fileId;
    u8 compressed;
} WF2DMAP_SCRDRAWINIT;

typedef struct WF2DMAP_SCRDRAW WF2DMAP_SCRDRAW;

// unk1 is never used
WF2DMAP_SCRDRAW *WF2DMAP_SCRDrawSysInit(ClActRenderer *renderer, void *unk1, const WF2DMAP_SCRDRAWINIT *init,
                                        u32 heapId);
void WF2DMAP_SCRDrawSysExit(WF2DMAP_SCRDRAW *sys);
void WF2DMAP_SCRDrawSysMain(WF2DMAP_SCRDRAW *sys, const WF2DMAP_SCROLL *scroll);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_SCRDRAW_H
