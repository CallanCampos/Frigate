#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_OBJDRAW_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_OBJDRAW_H

// Draws the objects of the Wi-Fi Club's 2D map (overlay 203) with wifi_2dchar.c's characters, named after the ROM's
// "wf2dmap_objdraw.c"

#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_obj.h"
#include "gfl/clact.h"

typedef struct WF2DMAP_OBJDRAWSYS WF2DMAP_OBJDRAWSYS;
typedef struct WF2DMAP_OBJDRAWWK WF2DMAP_OBJDRAWWK;

// Loads the player's character (charaId, a WF_2DC_CHARA_*) and the shadow. unk1 is never used
WF2DMAP_OBJDRAWSYS *WF2DMAP_OBJDrawSysInit(ClActUnit *unit, void *unk1, u32 objNum, u32 charaId, u32 moveType,
                                           u32 vramType, u32 heapId);
void WF2DMAP_OBJDrawSysExit(WF2DMAP_OBJDRAWSYS *sys);
void WF2DMAP_OBJDrawSysAllResDel(WF2DMAP_OBJDRAWSYS *sys);
void WF2DMAP_OBJDrawSysUnionResSet(WF2DMAP_OBJDRAWSYS *sys, u32 moveType, u32 heapId);
WF2DMAP_OBJDRAWWK *WF2DMAP_OBJDrawWkNew(WF2DMAP_OBJDRAWSYS *sys, const WF2DMAP_OBJWK *objWk, BOOL hero, u32 heapId);
void WF2DMAP_OBJDrawWkDel(WF2DMAP_OBJDRAWWK *wk);
void WF2DMAP_OBJDrawSysUpdata(WF2DMAP_OBJDRAWSYS *sys);
// Follows its object: animation, position and priority
void WF2DMAP_OBJDrawWkUpdata(WF2DMAP_OBJDRAWWK *wk);
// Whether WF2DMAP_OBJDrawWkUpdata does anything
void WF2DMAP_OBJDrawWkUpdataFlagSet(WF2DMAP_OBJDRAWWK *wk, BOOL flag);
// Plays the spinning animation
void WF2DMAP_OBJDrawWkKuruAnimeStart(WF2DMAP_OBJDRAWWK *wk);
void WF2DMAP_OBJDrawWkKuruAnimeMain(WF2DMAP_OBJDRAWWK *wk);
void WF2DMAP_OBJDrawWkKuruAnimeEnd(WF2DMAP_OBJDRAWWK *wk);
void WF2DMAP_OBJDrawWkMatrixSet(WF2DMAP_OBJDRAWWK *wk, WF2DMAP_POS pos);
BOOL WF2DMAP_OBJDrawWkDrawFlagGet(const WF2DMAP_OBJDRAWWK *wk);
void WF2DMAP_OBJDrawWkDrawFlagSet(WF2DMAP_OBJDRAWWK *wk, BOOL flag);
void WF2DMAP_OBJDrawWkShadowDrawFlagSet(WF2DMAP_OBJDRAWWK *wk, BOOL flag);
u32 WF2DMAP_OBJDrawWkDrawPriGet(const WF2DMAP_OBJDRAWWK *wk);
// Stands it facing way
void WF2DMAP_OBJDrawWkWaySet(WF2DMAP_OBJDRAWWK *wk, u32 way);
void WF2DMAP_OBJDrawWkShadowMatrixSet(WF2DMAP_OBJDRAWWK *wk, WF2DMAP_POS pos);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_OBJDRAW_H
