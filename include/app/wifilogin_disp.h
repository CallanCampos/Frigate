#ifndef POKEBW2_APP_WIFILOGIN_DISP_H
#define POKEBW2_APP_WIFILOGIN_DISP_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Wi-Fi login's display (wifilogin_disp.c, overlay 190): its BGs, with a palette cycle and a scrolling top screen
// in the style of the Dream World account screens, or a waving one in the style of Game Sync

// The graphics the display is made of
#define WIFILOGIN_DISP_STYLE_PDWACC 0
#define WIFILOGIN_DISP_STYLE_GSYNC 1

typedef struct WifiLoginDisp WifiLoginDisp;

WifiLoginDisp *WifiLoginDisp_Create(HeapID heapId, u32 style, u32 unused);
void WifiLoginDisp_Main(WifiLoginDisp *disp);
void WifiLoginDisp_Free(WifiLoginDisp *disp);
void WifiLoginDisp_SetUnk9C4(WifiLoginDisp *disp);
void WifiLoginDisp_ResetBlend(WifiLoginDisp *disp);

#endif // POKEBW2_APP_WIFILOGIN_DISP_H
