#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_judge.h"
#include "app/wf2dmap/wf2dmap_map.h"
#include "app/wf2dmap/wf2dmap_obj.h"

// Decides whether an object of the Wi-Fi Club's 2D map can do what its player asks. The ROM has no name for this
// file: it is named after the older games' wf2dmap_judge.c

// How long an object has to stand before it walks against a wall
#define WF2DMAP_JUDGE_WALLWALK_FRAME 8

static BOOL WF2DMAP_JUDGESysNoneJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysTurnJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysRunJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                     const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysBusyJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysWallWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                          const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysSlowWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                          const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysStayWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                          const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act);
static BOOL WF2DMAP_JUDGESysMapHitCheck(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys, u32 way);
static BOOL WF2DMAP_JUDGESysObjHitCheck(const WF2DMAP_OBJWK *wk, const WF2DMAP_OBJSYS *objSys, u32 way);
static void WF2DMAP_JUDGESysActCmdSet(WF2DMAP_ACTCMD *act, const WF2DMAP_OBJWK *wk, u32 cmd, u32 playId, u32 way);

// By the request's command, a WF2DMAP_OBJST_*
static BOOL (*const sWF2DMAP_JUDGEFuncs[WF2DMAP_OBJST_NUM])(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                                            const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req,
                                                            WF2DMAP_ACTCMD *act) = {
    WF2DMAP_JUDGESysNoneJudge,     WF2DMAP_JUDGESysTurnJudge,     WF2DMAP_JUDGESysWalkJudge,
    WF2DMAP_JUDGESysRunJudge,      WF2DMAP_JUDGESysBusyJudge,     WF2DMAP_JUDGESysWallWalkJudge,
    WF2DMAP_JUDGESysSlowWalkJudge, WF2DMAP_JUDGESysWalkJudge,     WF2DMAP_JUDGESysStayWalkJudge,
    WF2DMAP_JUDGESysStayWalkJudge, WF2DMAP_JUDGESysStayWalkJudge, WF2DMAP_JUDGESysStayWalkJudge,
};

BOOL WF2DMAP_JUDGESysCmdJudge(const WF2DMAP_MAPSYS *mapSys, const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req,
                              WF2DMAP_ACTCMD *act) {
    const WF2DMAP_OBJWK *wk;

    wk = WF2DMAP_OBJWkConstGet(objSys, req->playId);
    return sWF2DMAP_JUDGEFuncs[req->cmd](wk, mapSys, objSys, req, act);
}

static BOOL WF2DMAP_JUDGESysNoneJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act) {
    s32 status;

    status = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ST);
    if (status == WF2DMAP_OBJST_NONE || status == WF2DMAP_OBJST_BUSY) {
        WF2DMAP_JUDGESysActCmdSet(act, wk, req->cmd, req->playId, req->way);
        return TRUE;
    }
    return FALSE;
}

static BOOL WF2DMAP_JUDGESysTurnJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act) {
    s32 status;
    s32 way;

    status = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ST);
    way = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_WAY);
    if (status == WF2DMAP_OBJST_NONE && way != req->way) {
        WF2DMAP_JUDGESysActCmdSet(act, wk, req->cmd, req->playId, req->way);
        return TRUE;
    }
    return FALSE;
}

// An object that has stood facing a wall for a while walks against it instead
static BOOL WF2DMAP_JUDGESysWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act) {
    s32 status;

    status = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ST);
    if (status == WF2DMAP_OBJST_NONE) {
        if (WF2DMAP_JUDGESysMapHitCheck(wk, mapSys, req->way) != TRUE) {
            if (WF2DMAP_JUDGESysObjHitCheck(wk, objSys, req->way) != TRUE) {
                WF2DMAP_JUDGESysActCmdSet(act, wk, req->cmd, req->playId, req->way);
                return TRUE;
            }
        }
    }

    if (status == WF2DMAP_OBJST_NONE) {
        if (WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_STFRAME) >= WF2DMAP_JUDGE_WALLWALK_FRAME) {
            WF2DMAP_JUDGESysActCmdSet(act, wk, WF2DMAP_OBJST_WALLWALK, req->playId, req->way);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL WF2DMAP_JUDGESysRunJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                     const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act) {
    return WF2DMAP_JUDGESysWalkJudge(wk, mapSys, objSys, req, act);
}

static BOOL WF2DMAP_JUDGESysBusyJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                      const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req, WF2DMAP_ACTCMD *act) {
    s32 status;

    status = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ST);
    if (status == WF2DMAP_OBJST_NONE) {
        WF2DMAP_JUDGESysActCmdSet(act, wk, req->cmd, req->playId, req->way);
        return TRUE;
    }
    return FALSE;
}

static BOOL WF2DMAP_JUDGESysWallWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                          const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req,
                                          WF2DMAP_ACTCMD *act) {
    return FALSE;
}

static BOOL WF2DMAP_JUDGESysSlowWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                          const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req,
                                          WF2DMAP_ACTCMD *act) {
    return WF2DMAP_JUDGESysWalkJudge(wk, mapSys, objSys, req, act);
}

static BOOL WF2DMAP_JUDGESysStayWalkJudge(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys,
                                          const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req,
                                          WF2DMAP_ACTCMD *act) {
    s32 status;

    status = WF2DMAP_OBJWkDataGet(wk, WF2DMAP_OBJPM_ST);
    if (status == WF2DMAP_OBJST_NONE) {
        WF2DMAP_JUDGESysActCmdSet(act, wk, req->cmd, req->playId, req->way);
        return TRUE;
    }
    return FALSE;
}

static BOOL WF2DMAP_JUDGESysMapHitCheck(const WF2DMAP_OBJWK *wk, const WF2DMAP_MAPSYS *mapSys, u32 way) {
    WF2DMAP_POS pos;

    pos = WF2DMAP_OBJWkMatrixGet(wk);
    pos = WF2DMAP_OBJToolWayPosGet(pos, way);
    return WF2DMAP_MAPSysHitGet(mapSys, pos.x / WF2DMAP_GRID_SIZE, pos.y / WF2DMAP_GRID_SIZE);
}

static BOOL WF2DMAP_JUDGESysObjHitCheck(const WF2DMAP_OBJWK *wk, const WF2DMAP_OBJSYS *objSys, u32 way) {
    if (WF2DMAP_OBJSysHitCheck(wk, objSys, way) != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void WF2DMAP_JUDGESysActCmdSet(WF2DMAP_ACTCMD *act, const WF2DMAP_OBJWK *wk, u32 cmd, u32 playId, u32 way) {
    act->pos = WF2DMAP_OBJWkMatrixGet(wk);
    act->cmd = cmd;
    act->playId = playId;
    act->way = way;
}
