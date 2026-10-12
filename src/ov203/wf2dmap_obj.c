#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_obj.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// The objects of the Wi-Fi Club's 2D map, named after the ROM's "wf2dmap_obj.c"

struct WF2DMAP_OBJWK {
    BOOL used;
    WF2DMAP_POS pos;
    WF2DMAP_POS lastPos;
    u16 playId;
    u16 charaId;
    u16 frame;
    u16 endFrame;
    u8 way;
    u8 status;
    u16 statusFrame;
};

struct WF2DMAP_OBJSYS {
    WF2DMAP_OBJWK *wk;
    u32 num;
};

typedef struct {
    s8 x;
    s8 y;
} WF2DMAP_OBJWAYPOS;

static WF2DMAP_OBJWK *WF2DMAP_OBJSysCleanWkGet(WF2DMAP_OBJSYS *sys);
static void WF2DMAP_OBJWkCmdSet(WF2DMAP_OBJWK *wk, const WF2DMAP_ACTCMD *act);
static void WF2DMAP_OBJWkMain(WF2DMAP_OBJWK *wk);
static void WF2DMAP_OBJWkStInit_None(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_Turn(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_Walk(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_Run(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_Busy(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_WallWalk(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_SlowWalk(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_Walk4(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_StayWalk2(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_StayWalk4(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_StayWalk8(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static void WF2DMAP_OBJWkStInit_StayWalk16(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way);
static BOOL WF2DMAP_OBJWkStMain_None(WF2DMAP_OBJWK *wk);
static BOOL WF2DMAP_OBJWkStMain_Turn(WF2DMAP_OBJWK *wk);
static BOOL WF2DMAP_OBJWkStMain_Walk(WF2DMAP_OBJWK *wk);
static BOOL WF2DMAP_OBJWkStMain_Run(WF2DMAP_OBJWK *wk);
static BOOL WF2DMAP_OBJWkStMain_Busy(WF2DMAP_OBJWK *wk);
static BOOL WF2DMAP_OBJWkStMain_WallWalk(WF2DMAP_OBJWK *wk);
static BOOL WF2DMAP_OBJWkStMain_StayWalk(WF2DMAP_OBJWK *wk);
static WF2DMAP_POS WF2DMAP_OBJWayPosGet(WF2DMAP_POS pos, u32 way);
static BOOL WF2DMAP_OBJWkFrameCount(WF2DMAP_OBJWK *wk);
static void WF2DMAP_OBJWkStEnd(WF2DMAP_OBJWK *wk);

static const u8 sWF2DMAP_OBJRetWay[WF2DMAP_WAY_NUM] = {
    WF2DMAP_WAY_DOWN,
    WF2DMAP_WAY_UP,
    WF2DMAP_WAY_RIGHT,
    WF2DMAP_WAY_LEFT,
};

static const WF2DMAP_OBJWAYPOS sWF2DMAP_OBJWayPos[WF2DMAP_WAY_NUM] = {
    { 0, -WF2DMAP_GRID_SIZE },
    { 0, WF2DMAP_GRID_SIZE },
    { -WF2DMAP_GRID_SIZE, 0 },
    { WF2DMAP_GRID_SIZE, 0 },
};

static void (*const sWF2DMAP_OBJStInitFuncs[WF2DMAP_OBJST_NUM])(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) = {
    WF2DMAP_OBJWkStInit_None,      WF2DMAP_OBJWkStInit_Turn,      WF2DMAP_OBJWkStInit_Walk,
    WF2DMAP_OBJWkStInit_Run,       WF2DMAP_OBJWkStInit_Busy,      WF2DMAP_OBJWkStInit_WallWalk,
    WF2DMAP_OBJWkStInit_SlowWalk,  WF2DMAP_OBJWkStInit_Walk4,     WF2DMAP_OBJWkStInit_StayWalk2,
    WF2DMAP_OBJWkStInit_StayWalk4, WF2DMAP_OBJWkStInit_StayWalk8, WF2DMAP_OBJWkStInit_StayWalk16,
};

// Each returns TRUE when the state is over
static BOOL (*const sWF2DMAP_OBJStMainFuncs[WF2DMAP_OBJST_NUM])(WF2DMAP_OBJWK *wk) = {
    WF2DMAP_OBJWkStMain_None,     WF2DMAP_OBJWkStMain_Turn,     WF2DMAP_OBJWkStMain_Walk,
    WF2DMAP_OBJWkStMain_Run,      WF2DMAP_OBJWkStMain_Busy,     WF2DMAP_OBJWkStMain_WallWalk,
    WF2DMAP_OBJWkStMain_Walk,     WF2DMAP_OBJWkStMain_Walk,     WF2DMAP_OBJWkStMain_StayWalk,
    WF2DMAP_OBJWkStMain_StayWalk, WF2DMAP_OBJWkStMain_StayWalk, WF2DMAP_OBJWkStMain_StayWalk,
};

WF2DMAP_OBJSYS *WF2DMAP_OBJSysInit(u32 objNum, u32 heapId) {
    WF2DMAP_OBJSYS *sys;

    sys = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_OBJSYS), TRUE, "wf2dmap_obj.c", 128);
    sys->num = objNum;
    sys->wk = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_OBJWK) * sys->num, TRUE, "wf2dmap_obj.c", 133);
    return sys;
}

void WF2DMAP_OBJSysExit(WF2DMAP_OBJSYS *sys) {
    GFL_HeapFree(sys->wk);
    GFL_HeapFree(sys);
}

void WF2DMAP_OBJSysCmdSet(WF2DMAP_OBJSYS *sys, const WF2DMAP_ACTCMD *act) {
    WF2DMAP_OBJWK *wk;

    wk = WF2DMAP_OBJWkGet(sys, act->playId);
    WF2DMAP_OBJWkCmdSet(wk, act);
}

void WF2DMAP_OBJSysMain(WF2DMAP_OBJSYS *sys) {
    u32 i;
    u32 num;
    WF2DMAP_OBJWK *wk;

    num = WF2DMAP_OBJSysObjNumGet(sys);
    for (i = 0; i < num; i++) {
        wk = WF2DMAP_OBJWkIdxGet(sys, i);
        if (wk != NULL) {
            WF2DMAP_OBJWkMain(wk);
        }
    }
}

u32 WF2DMAP_OBJSysObjNumGet(const WF2DMAP_OBJSYS *sys) {
    return sys->num;
}

WF2DMAP_OBJWK *WF2DMAP_OBJWkNew(WF2DMAP_OBJSYS *sys, const WF2DMAP_OBJDATA *data) {
    WF2DMAP_OBJWK *wk;

    wk = WF2DMAP_OBJSysCleanWkGet(sys);
    wk->pos.x = data->x;
    wk->pos.y = data->y;
    wk->lastPos.x = data->x;
    wk->lastPos.y = data->y;
    wk->playId = data->playId;
    wk->status = data->status;
    wk->way = data->way;
    wk->charaId = data->charaId;
    wk->used = TRUE;
    return wk;
}

void WF2DMAP_OBJWkDel(WF2DMAP_OBJWK *wk) {
    sys_memset(wk, 0, sizeof(WF2DMAP_OBJWK));
}

WF2DMAP_OBJWK *WF2DMAP_OBJWkGet(WF2DMAP_OBJSYS *sys, u16 playId) {
    int i;

    for (i = 0; i < sys->num; i++) {
        if (sys->wk[i].used == TRUE) {
            if (sys->wk[i].playId == playId) {
                return &sys->wk[i];
            }
        }
    }
    return NULL;
}

WF2DMAP_OBJWK *WF2DMAP_OBJWkIdxGet(WF2DMAP_OBJSYS *sys, u16 index) {
    if (sys->wk[index].used == TRUE) {
        return &sys->wk[index];
    }
    return NULL;
}

const WF2DMAP_OBJWK *WF2DMAP_OBJWkConstGet(const WF2DMAP_OBJSYS *sys, u16 playId) {
    return WF2DMAP_OBJWkGet((WF2DMAP_OBJSYS *)sys, playId);
}

const WF2DMAP_OBJWK *WF2DMAP_OBJWkConstIdxGet(const WF2DMAP_OBJSYS *sys, u16 index) {
    return WF2DMAP_OBJWkIdxGet((WF2DMAP_OBJSYS *)sys, index);
}

s32 WF2DMAP_OBJWkDataGet(const WF2DMAP_OBJWK *wk, u32 id) {
    s32 data;

    switch (id) {
    case WF2DMAP_OBJPM_X:
        data = wk->pos.x;
        break;
    case WF2DMAP_OBJPM_Y:
        data = wk->pos.y;
        break;
    case WF2DMAP_OBJPM_LX:
        data = wk->lastPos.x;
        break;
    case WF2DMAP_OBJPM_LY:
        data = wk->lastPos.y;
        break;
    case WF2DMAP_OBJPM_PLID:
        data = wk->playId;
        break;
    case WF2DMAP_OBJPM_ST:
        data = wk->status;
        break;
    case WF2DMAP_OBJPM_WAY:
        data = wk->way;
        break;
    case WF2DMAP_OBJPM_CHARA:
        data = wk->charaId;
        break;
    case WF2DMAP_OBJPM_FRAME:
        data = wk->frame;
        break;
    case WF2DMAP_OBJPM_ENDFRAME:
        data = wk->endFrame;
        break;
    case WF2DMAP_OBJPM_STFRAME:
        data = wk->statusFrame;
        break;
    }
    return data;
}

void WF2DMAP_OBJWkDataSet(WF2DMAP_OBJWK *wk, u32 id, s32 data) {
    switch (id) {
    case WF2DMAP_OBJPM_X:
        wk->pos.x = data;
        break;
    case WF2DMAP_OBJPM_Y:
        wk->pos.y = data;
        break;
    case WF2DMAP_OBJPM_LX:
        wk->lastPos.x = data;
        break;
    case WF2DMAP_OBJPM_LY:
        wk->lastPos.y = data;
        break;
    case WF2DMAP_OBJPM_PLID:
        wk->playId = data;
        break;
    case WF2DMAP_OBJPM_ST:
        wk->status = data;
        break;
    case WF2DMAP_OBJPM_WAY:
        wk->way = data;
        break;
    case WF2DMAP_OBJPM_CHARA:
        wk->charaId = data;
        break;
    case WF2DMAP_OBJPM_FRAME:
        wk->frame = data;
        break;
    case WF2DMAP_OBJPM_ENDFRAME:
        wk->endFrame = data;
        break;
    case WF2DMAP_OBJPM_STFRAME:
        wk->statusFrame = data;
        break;
    }
}

WF2DMAP_POS WF2DMAP_OBJToolWayPosGet(WF2DMAP_POS pos, u32 way) {
    pos.x += sWF2DMAP_OBJWayPos[way].x;
    pos.y += sWF2DMAP_OBJWayPos[way].y;
    return pos;
}

u32 WF2DMAP_OBJToolRetWayGet(u32 way) {
    return sWF2DMAP_OBJRetWay[way];
}

static WF2DMAP_OBJWK *WF2DMAP_OBJSysCleanWkGet(WF2DMAP_OBJSYS *sys) {
    int i;

    for (i = 0; i < sys->num; i++) {
        if (sys->wk[i].used == FALSE) {
            return &sys->wk[i];
        }
    }
    return NULL;
}

WF2DMAP_POS WF2DMAP_OBJWkMatrixGet(const WF2DMAP_OBJWK *wk) {
    return wk->pos;
}

WF2DMAP_POS WF2DMAP_OBJWkLastMatrixGet(const WF2DMAP_OBJWK *wk) {
    return wk->lastPos;
}

WF2DMAP_POS WF2DMAP_OBJWkFrameMatrixGet(const WF2DMAP_OBJWK *wk) {
    WF2DMAP_POS pos;
    WF2DMAP_POS lastPos;
    s32 frame;
    s32 endFrame;

    frame = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_FRAME);
    endFrame = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ENDFRAME);
    pos = WF2DMAP_OBJWkMatrixGet(wk);
    lastPos = WF2DMAP_OBJWkLastMatrixGet(wk);

    pos.x -= lastPos.x;
    pos.y -= lastPos.y;
    if (frame > 0) {
        pos.x = (frame * pos.x) / endFrame;
        pos.y = (frame * pos.y) / endFrame;
    } else {
        pos.x = 0;
        pos.y = 0;
    }
    pos.x += lastPos.x;
    pos.y += lastPos.y;
    return pos;
}

void WF2DMAP_OBJWkMatrixSet(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos) {
    wk->pos = pos;
}

void WF2DMAP_OBJWkLastMatrixSet(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos) {
    wk->lastPos = pos;
}

const WF2DMAP_OBJWK *WF2DMAP_OBJSysHitCheck(const WF2DMAP_OBJWK *wk, const WF2DMAP_OBJSYS *sys, u32 way) {
    WF2DMAP_POS pos;
    WF2DMAP_POS objPos;
    const WF2DMAP_OBJWK *obj;
    u32 num;
    s32 playId;
    s32 objPlayId;
    int i;

    pos = WF2DMAP_OBJWkMatrixGet(wk);
    pos = WF2DMAP_OBJToolWayPosGet(pos, way);
    playId = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_PLID);
    num = WF2DMAP_OBJSysObjNumGet(sys);

    for (i = 0; i < num; i++) {
        obj = WF2DMAP_OBJWkConstIdxGet(sys, i);
        if (obj != NULL) {
            objPlayId = WF2DMAP_OBJWkDataGet(obj, WF2DMAP_OBJPM_PLID);
            if (objPlayId != playId) {
                objPos = WF2DMAP_OBJWkMatrixGet(obj);
                if (objPos.x == pos.x && objPos.y == pos.y) {
                    return obj;
                }
                objPos = WF2DMAP_OBJWkLastMatrixGet(obj);
                if (objPos.x == pos.x && objPos.y == pos.y) {
                    return obj;
                }
            }
        }
    }
    return NULL;
}

static void WF2DMAP_OBJWkCmdSet(WF2DMAP_OBJWK *wk, const WF2DMAP_ACTCMD *act) {
    sWF2DMAP_OBJStInitFuncs[act->cmd](wk, act->pos, act->way);
    wk->statusFrame = 0;
}

static void WF2DMAP_OBJWkMain(WF2DMAP_OBJWK *wk) {
    s32 status;
    BOOL end;

    status = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ST);
    end = sWF2DMAP_OBJStMainFuncs[status](wk);
    wk->statusFrame++;
    if (end == TRUE) {
        WF2DMAP_OBJWkStEnd(wk);
        wk->statusFrame = 0;
    }
}

static void WF2DMAP_OBJWkStInit_None(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_NONE);
}

static void WF2DMAP_OBJWkStInit_Turn(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_TURN);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 2);
}

static void WF2DMAP_OBJWkStInit_Walk(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_POS wayPos;

    wayPos = WF2DMAP_OBJWayPosGet(pos, way);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, wayPos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_WALK);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 8);
}

static void WF2DMAP_OBJWkStInit_Run(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_POS wayPos;

    wayPos = WF2DMAP_OBJWayPosGet(pos, way);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, wayPos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_RUN);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 4);
}

static void WF2DMAP_OBJWkStInit_Busy(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_BUSY);
}

static void WF2DMAP_OBJWkStInit_WallWalk(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_WALLWALK);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 16);
}

static void WF2DMAP_OBJWkStInit_SlowWalk(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_POS wayPos;

    wayPos = WF2DMAP_OBJWayPosGet(pos, way);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, wayPos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_SLOWWALK);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 16);
}

static void WF2DMAP_OBJWkStInit_Walk4(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_POS wayPos;

    wayPos = WF2DMAP_OBJWayPosGet(pos, way);
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, wayPos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_WALK4);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 4);
}

static void WF2DMAP_OBJWkStInit_StayWalk2(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_STAYWALK2);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 2);
}

static void WF2DMAP_OBJWkStInit_StayWalk4(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_STAYWALK4);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 4);
}

static void WF2DMAP_OBJWkStInit_StayWalk8(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_STAYWALK8);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 8);
}

static void WF2DMAP_OBJWkStInit_StayWalk16(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos, u32 way) {
    WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    WF2DMAP_OBJWkMatrixSet(wk, pos);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_WAY, way);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ST, WF2DMAP_OBJST_STAYWALK16);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, 0);
    WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_ENDFRAME, 16);
}

static BOOL WF2DMAP_OBJWkStMain_None(WF2DMAP_OBJWK *wk) {
    return FALSE;
}

static BOOL WF2DMAP_OBJWkStMain_Turn(WF2DMAP_OBJWK *wk) {
    return WF2DMAP_OBJWkFrameCount(wk);
}

// Arrives when the frames are over
static BOOL WF2DMAP_OBJWkStMain_Walk(WF2DMAP_OBJWK *wk) {
    BOOL end;
    WF2DMAP_POS pos;

    end = WF2DMAP_OBJWkFrameCount(wk);
    if (end == TRUE) {
        pos = WF2DMAP_OBJWkMatrixGet(wk);
        WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    }
    return end;
}

static BOOL WF2DMAP_OBJWkStMain_Run(WF2DMAP_OBJWK *wk) {
    BOOL end;
    WF2DMAP_POS pos;

    end = WF2DMAP_OBJWkFrameCount(wk);
    if (end == TRUE) {
        pos = WF2DMAP_OBJWkMatrixGet(wk);
        WF2DMAP_OBJWkLastMatrixSet(wk, pos);
    }
    return end;
}

static BOOL WF2DMAP_OBJWkStMain_Busy(WF2DMAP_OBJWK *wk) {
    return FALSE;
}

static BOOL WF2DMAP_OBJWkStMain_WallWalk(WF2DMAP_OBJWK *wk) {
    return WF2DMAP_OBJWkFrameCount(wk);
}

static BOOL WF2DMAP_OBJWkStMain_StayWalk(WF2DMAP_OBJWK *wk) {
    return WF2DMAP_OBJWkFrameCount(wk);
}

static WF2DMAP_POS WF2DMAP_OBJWayPosGet(WF2DMAP_POS pos, u32 way) {
    return WF2DMAP_OBJToolWayPosGet(pos, way);
}

// Returns TRUE at the state's last frame
static BOOL WF2DMAP_OBJWkFrameCount(WF2DMAP_OBJWK *wk) {
    s32 frame;
    s32 endFrame;

    frame = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_FRAME);
    endFrame = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ENDFRAME);
    if (frame < endFrame) {
        WF2DMAP_OBJWkDataSet(wk, WF2DMAP_OBJPM_FRAME, frame + 1);
        if (frame + 1 < endFrame) {
            return FALSE;
        }
    }
    return TRUE;
}

// Stands where it is
static void WF2DMAP_OBJWkStEnd(WF2DMAP_OBJWK *wk) {
    WF2DMAP_POS pos;
    s32 way;

    pos = WF2DMAP_OBJWkMatrixGet(wk);
    way = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_WAY);
    WF2DMAP_OBJWkStInit_None(wk, pos, way);
}
