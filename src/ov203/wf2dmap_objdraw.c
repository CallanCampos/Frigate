#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_obj.h"
#include "app/wf2dmap/wf2dmap_objdraw.h"
#include "app/wf2dmap/wifi_2dchar.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// Draws the objects of the Wi-Fi Club's 2D map, named after the ROM's "wf2dmap_objdraw.c"

#define WF2DMAP_OBJDRAW_BG_PRI 2
#define WF2DMAP_OBJDRAW_SHADOW_PRI 0xff

// The priority of an object at the top of the map. The player's own character is in front of the others on its row
#define WF2DMAP_OBJDRAW_PRI_HERO 250
#define WF2DMAP_OBJDRAW_PRI_OTHER 251

struct WF2DMAP_OBJDRAWWK {
    const WF2DMAP_OBJWK *objWk;
    WF_2DCWK *drawWk;
    // What the object did when its animation was last set
    u16 status;
    u16 way;
    u16 playId;
    // The frame the last animation that keeps its frame was left at, and that animation
    u16 lastAnmFrame;
    u16 lastAnm;
    u8 hero;
    u8 updata;
};

struct WF2DMAP_OBJDRAWSYS {
    WF_2DCSYS *charSys;
    WF2DMAP_OBJDRAWWK *wk;
    u16 num;
    u8 bgPriority;
    u8 vramType;
};

static s32 WF2DMAP_OBJDrawWkDrawPriCalc(s16 y, u32 hero);
static BOOL WF2DMAP_OBJDrawWkCleanCheck(const WF2DMAP_OBJDRAWWK *wk);
static WF2DMAP_OBJDRAWWK *WF2DMAP_OBJDrawSysCleanWkGet(WF2DMAP_OBJDRAWSYS *sys);
static u32 WF2DMAP_OBJDrawWkAnmTypeGet(const WF_2DCWK *drawWk, u32 status);
static void WF2DMAP_OBJDrawWkFrameMatrixGet(const WF2DMAP_OBJDRAWWK *wk, s16 *x, s16 *y);
static void WF2DMAP_OBJDrawWkAnmSet(WF2DMAP_OBJDRAWWK *wk, u32 anmType, u32 way);

// Whether an animation goes on from the frame it was left at when it starts again
static const u8 sWF2DMAP_OBJDrawAnmKeepFrame[WF_2DC_ANMNUM] = {
    FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE,
};

// The animation of each WF2DMAP_OBJST_*
static const u8 sWF2DMAP_OBJDrawStAnm[WF2DMAP_OBJST_NUM] = {
    WF_2DC_ANMWAY,       WF_2DC_ANMTURN,      WF_2DC_ANMWALK,     WF_2DC_ANMRUN,
    WF_2DC_ANMWAY,       WF_2DC_ANMWALLWALK,  WF_2DC_ANMSLOWWALK, WF_2DC_ANMHIGHWALK4,
    WF_2DC_ANMHIGHWALK2, WF_2DC_ANMHIGHWALK4, WF_2DC_ANMWALK,     WF_2DC_ANMSLOWWALK,
};

WF2DMAP_OBJDRAWSYS *WF2DMAP_OBJDrawSysInit(ClActUnit *unit, void *unk1, u32 objNum, u32 charaId, u32 moveType,
                                           u32 vramType, u32 heapId) {
    WF2DMAP_OBJDRAWSYS *sys;

    sys = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_OBJDRAWSYS), TRUE, "wf2dmap_objdraw.c", 161);
    sys->charSys = WF_2DC_SysInit(unit, unk1, objNum, heapId);
    sys->num = objNum;
    sys->wk = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_OBJDRAWWK) * sys->num, TRUE, "wf2dmap_objdraw.c", 168);
    sys->bgPriority = WF2DMAP_OBJDRAW_BG_PRI;
    sys->vramType = vramType;
    WF_2DC_ResSet(sys->charSys, charaId, sys->vramType, moveType, heapId);
    WF_2DC_ShadowResSet(sys->charSys, sys->vramType, WF2DMAP_OBJDRAW_SHADOW_PRI, heapId);
    return sys;
}

void WF2DMAP_OBJDrawSysExit(WF2DMAP_OBJDRAWSYS *sys) {
    int i;

    for (i = 0; i < sys->num; i++) {
        if (WF2DMAP_OBJDrawWkCleanCheck(&sys->wk[i]) == FALSE) {
            WF2DMAP_OBJDrawWkDel(&sys->wk[i]);
        }
    }
    WF_2DC_ShadowResDel(sys->charSys);
    WF2DMAP_OBJDrawSysAllResDel(sys);
    WF_2DC_SysExit(sys->charSys);
    GFL_HeapFree(sys->wk);
    GFL_HeapFree(sys);
}

void WF2DMAP_OBJDrawSysAllResDel(WF2DMAP_OBJDRAWSYS *sys) {
    WF_2DC_AllResDel(sys->charSys);
}

void WF2DMAP_OBJDrawSysUnionResSet(WF2DMAP_OBJDRAWSYS *sys, u32 moveType, u32 heapId) {
    WF_2DC_UnionResSet(sys->charSys, sys->vramType, moveType, 0, heapId);
}

WF2DMAP_OBJDRAWWK *WF2DMAP_OBJDrawWkNew(WF2DMAP_OBJDRAWSYS *sys, const WF2DMAP_OBJWK *objWk, BOOL hero, u32 heapId) {
    WF2DMAP_OBJDRAWWK *wk;
    WF_2DC_WKDATA data;

    wk = WF2DMAP_OBJDrawSysCleanWkGet(sys);
    wk->status = WF2DMAP_OBJWkDataGet(objWk, WF2DMAP_OBJPM_ST);
    wk->way = WF2DMAP_OBJWkDataGet(objWk, WF2DMAP_OBJPM_WAY);
    wk->playId = WF2DMAP_OBJWkDataGet(objWk, WF2DMAP_OBJPM_PLID);
    wk->objWk = objWk;
    wk->hero = hero;
    wk->updata = TRUE;
    wk->lastAnmFrame = 0;
    wk->lastAnm = 0;

    data.x = WF2DMAP_OBJWkDataGet(objWk, WF2DMAP_OBJPM_X);
    data.y = WF2DMAP_OBJWkDataGet(objWk, WF2DMAP_OBJPM_Y);
    data.priority = WF2DMAP_OBJDrawWkDrawPriCalc(data.y, wk->hero);
    data.bgPriority = sys->bgPriority;
    wk->drawWk = WF_2DC_WkAdd(sys->charSys, &data, WF2DMAP_OBJWkDataGet(objWk, WF2DMAP_OBJPM_CHARA), heapId);

    WF2DMAP_OBJDrawWkUpdata(wk);
    return wk;
}

void WF2DMAP_OBJDrawWkDel(WF2DMAP_OBJDRAWWK *wk) {
    WF_2DC_WkDel(wk->drawWk);
    MI_CpuFill8(wk, 0, sizeof(WF2DMAP_OBJDRAWWK));
}

void WF2DMAP_OBJDrawSysUpdata(WF2DMAP_OBJDRAWSYS *sys) {
    int i;

    for (i = 0; i < sys->num; i++) {
        if (WF2DMAP_OBJDrawWkCleanCheck(&sys->wk[i]) == FALSE) {
            WF2DMAP_OBJDrawWkUpdata(&sys->wk[i]);
        }
    }
}

void WF2DMAP_OBJDrawWkUpdata(WF2DMAP_OBJDRAWWK *wk) {
    s32 status;
    s32 way;
    s16 frame;
    s16 x;
    s16 y;
    u16 priority;

    if (wk->updata) {
        status = WF2DMAP_OBJWkDataGet(wk->objWk, WF2DMAP_OBJPM_ST);
        way = WF2DMAP_OBJWkDataGet(wk->objWk, WF2DMAP_OBJPM_WAY);
        frame = WF2DMAP_OBJWkDataGet(wk->objWk, WF2DMAP_OBJPM_FRAME);

        if (wk->status != status || wk->way != way || frame == 0) {
            wk->status = status;
            wk->way = way;
            WF2DMAP_OBJDrawWkAnmSet(wk, WF2DMAP_OBJDrawWkAnmTypeGet(wk->drawWk, status), way);
        }

        WF2DMAP_OBJDrawWkFrameMatrixGet(wk, &x, &y);
        WF_2DC_WkMatrixSet(wk->drawWk, x, y);
        priority = WF2DMAP_OBJDrawWkDrawPriCalc(y, wk->hero);
        WF_2DC_WkDrawPriSet(wk->drawWk, priority);
        WF_2DC_WkPatAnmMain(wk->drawWk);
    }
}

void WF2DMAP_OBJDrawWkUpdataFlagSet(WF2DMAP_OBJDRAWWK *wk, BOOL flag) {
    wk->updata = flag;
}

void WF2DMAP_OBJDrawWkSpinAnimeStart(WF2DMAP_OBJDRAWWK *wk) {
    WF2DMAP_OBJDrawWkAnmSet(wk, WF_2DC_ANMROTA, wk->way);
}

void WF2DMAP_OBJDrawWkSpinAnimeMain(WF2DMAP_OBJDRAWWK *wk) {
    WF_2DC_WkPatAnmMain(wk->drawWk);
}

void WF2DMAP_OBJDrawWkSpinAnimeEnd(WF2DMAP_OBJDRAWWK *wk) {
    WF2DMAP_OBJDrawWkAnmSet(wk, WF2DMAP_OBJDrawWkAnmTypeGet(wk->drawWk, wk->status), wk->way);
    WF2DMAP_OBJDrawWkUpdata(wk);
}

void WF2DMAP_OBJDrawWkMatrixSet(WF2DMAP_OBJDRAWWK *wk, WF2DMAP_POS pos) {
    WF_2DC_WkMatrixSet(wk->drawWk, pos.x, pos.y);
}

BOOL WF2DMAP_OBJDrawWkDrawFlagGet(const WF2DMAP_OBJDRAWWK *wk) {
    return WF_2DC_WkDrawFlagGet(wk->drawWk);
}

void WF2DMAP_OBJDrawWkDrawFlagSet(WF2DMAP_OBJDRAWWK *wk, BOOL flag) {
    WF_2DC_WkDrawFlagSet(wk->drawWk, flag);
}

void WF2DMAP_OBJDrawWkShadowDrawFlagSet(WF2DMAP_OBJDRAWWK *wk, BOOL flag) {
    WF_2DC_WkShadowDrawFlagSet(wk->drawWk, flag);
}

u32 WF2DMAP_OBJDrawWkDrawPriGet(const WF2DMAP_OBJDRAWWK *wk) {
    return WF_2DC_WkDrawPriGet(wk->drawWk);
}

void WF2DMAP_OBJDrawWkWaySet(WF2DMAP_OBJDRAWWK *wk, u32 way) {
    WF2DMAP_OBJDrawWkAnmSet(wk, WF_2DC_ANMWAY, way);
    WF_2DC_WkPatAnmMain(wk->drawWk);
}

// Objects lower on the map are in front
static s32 WF2DMAP_OBJDrawWkDrawPriCalc(s16 y, u32 hero) {
    if (hero == TRUE) {
        return WF2DMAP_OBJDRAW_PRI_HERO - y / 4;
    }
    return WF2DMAP_OBJDRAW_PRI_OTHER - y / 4;
}

void WF2DMAP_OBJDrawWkShadowMatrixSet(WF2DMAP_OBJDRAWWK *wk, WF2DMAP_POS pos) {
    WF_2DC_WkShadowMatrixSet(wk->drawWk, pos.x, pos.y);
}

static BOOL WF2DMAP_OBJDrawWkCleanCheck(const WF2DMAP_OBJDRAWWK *wk) {
    if (wk->drawWk == NULL) {
        return TRUE;
    }
    return FALSE;
}

static WF2DMAP_OBJDRAWWK *WF2DMAP_OBJDrawSysCleanWkGet(WF2DMAP_OBJDRAWSYS *sys) {
    int i;

    for (i = 0; i < sys->num; i++) {
        if (WF2DMAP_OBJDrawWkCleanCheck(&sys->wk[i]) == TRUE) {
            return &sys->wk[i];
        }
    }
    return NULL;
}

static u32 WF2DMAP_OBJDrawWkAnmTypeGet(const WF_2DCWK *drawWk, u32 status) {
    if (WF_2DC_WkMoveTypeGet(drawWk) == WF_2DC_MOVETURN) {
        return WF_2DC_ANMWAY;
    }
    return sWF2DMAP_OBJDrawStAnm[status];
}

static void WF2DMAP_OBJDrawWkFrameMatrixGet(const WF2DMAP_OBJDRAWWK *wk, s16 *x, s16 *y) {
    WF2DMAP_POS pos;

    pos = WF2DMAP_OBJWkFrameMatrixGet(wk->objWk);
    *x = pos.x;
    *y = pos.y;
}

static void WF2DMAP_OBJDrawWkAnmSet(WF2DMAP_OBJDRAWWK *wk, u32 anmType, u32 way) {
    u16 frame;
    u16 lastAnm;

    frame = WF_2DC_WkAnmFrameGet(wk->drawWk);
    lastAnm = WF_2DC_WkAnmTypeGet(wk->drawWk);
    WF_2DC_WkPatAnmStart(wk->drawWk, anmType, way);

    if (lastAnm != anmType) {
        if (sWF2DMAP_OBJDrawAnmKeepFrame[anmType] == TRUE) {
            if (wk->lastAnm == anmType) {
                WF_2DC_WkAnmFrameSet(wk->drawWk, wk->lastAnmFrame);
            }
        }
        if (sWF2DMAP_OBJDrawAnmKeepFrame[lastAnm] == TRUE) {
            wk->lastAnm = lastAnm;
            wk->lastAnmFrame = frame;
        }
    }
}
