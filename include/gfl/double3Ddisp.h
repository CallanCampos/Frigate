#ifndef POKEBW2_GFL_DOUBLE3DDISP_H
#define POKEBW2_GFL_DOUBLE3DDISP_H

#include "types.h"
#include "gfl/heap.h"

// 3D on both screens (double3Ddisp.c), rendering to each on alternate frames. GFL_G3DDual3DRaiseNeedRenderFlag is
// called after each frame's 3D, and GFL_G3DDual3DExecVBlank at VBlank switches screens

void GFL_G3DDual3DInit(HeapID heapId);
void GFL_G3DDual3DFree(void);
// Whether the main engine renders to the bottom screen this frame
BOOL GFL_G3DDual3DGetRenderDisp(void);
void GFL_G3DDual3DRaiseNeedRenderFlag(void);
void GFL_G3DDual3DExecVBlank(void);

#endif // POKEBW2_GFL_DOUBLE3DDISP_H
