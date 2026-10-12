#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_OBJ_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_OBJ_H

// The objects of the Wi-Fi Club's 2D map (overlay 203), named after the ROM's "wf2dmap_obj.c"

#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"

// The values of an object that WF2DMAP_OBJWkDataGet and WF2DMAP_OBJWkDataSet reach
enum {
    WF2DMAP_OBJPM_X,
    WF2DMAP_OBJPM_Y,
    // Where it was before its last move
    WF2DMAP_OBJPM_LX,
    WF2DMAP_OBJPM_LY,
    WF2DMAP_OBJPM_PLID,
    // WF2DMAP_OBJST_*
    WF2DMAP_OBJPM_ST,
    WF2DMAP_OBJPM_WAY,
    WF2DMAP_OBJPM_CHARA,
    // The frame of its state and the frame the state ends at
    WF2DMAP_OBJPM_FRAME,
    WF2DMAP_OBJPM_ENDFRAME,
    // How many frames it has been in its state
    WF2DMAP_OBJPM_STFRAME,
    WF2DMAP_OBJPM_NUM,
};

typedef struct {
    s16 x;
    s16 y;
    u16 playId;
    u16 status;
    u16 way;
    u16 charaId;
} WF2DMAP_OBJDATA;

typedef struct WF2DMAP_OBJSYS WF2DMAP_OBJSYS;
typedef struct WF2DMAP_OBJWK WF2DMAP_OBJWK;

WF2DMAP_OBJSYS *WF2DMAP_OBJSysInit(u32 objNum, u32 heapId);
void WF2DMAP_OBJSysExit(WF2DMAP_OBJSYS *sys);
void WF2DMAP_OBJSysCmdSet(WF2DMAP_OBJSYS *sys, const WF2DMAP_ACTCMD *act);
void WF2DMAP_OBJSysMain(WF2DMAP_OBJSYS *sys);
u32 WF2DMAP_OBJSysObjNumGet(const WF2DMAP_OBJSYS *sys);
WF2DMAP_OBJWK *WF2DMAP_OBJWkNew(WF2DMAP_OBJSYS *sys, const WF2DMAP_OBJDATA *data);
void WF2DMAP_OBJWkDel(WF2DMAP_OBJWK *wk);
// The object of a player, or NULL
WF2DMAP_OBJWK *WF2DMAP_OBJWkGet(WF2DMAP_OBJSYS *sys, u16 playId);
// The object in a slot, or NULL when the slot is free
WF2DMAP_OBJWK *WF2DMAP_OBJWkIdxGet(WF2DMAP_OBJSYS *sys, u16 index);
const WF2DMAP_OBJWK *WF2DMAP_OBJWkConstGet(const WF2DMAP_OBJSYS *sys, u16 playId);
const WF2DMAP_OBJWK *WF2DMAP_OBJWkConstIdxGet(const WF2DMAP_OBJSYS *sys, u16 index);
s32 WF2DMAP_OBJWkDataGet(const WF2DMAP_OBJWK *wk, u32 id);
void WF2DMAP_OBJWkDataSet(WF2DMAP_OBJWK *wk, u32 id, s32 data);
// The position one square from pos
WF2DMAP_POS WF2DMAP_OBJToolWayPosGet(WF2DMAP_POS pos, u32 way);
// The opposite way
u32 WF2DMAP_OBJToolRetWayGet(u32 way);
WF2DMAP_POS WF2DMAP_OBJWkMatrixGet(const WF2DMAP_OBJWK *wk);
WF2DMAP_POS WF2DMAP_OBJWkLastMatrixGet(const WF2DMAP_OBJWK *wk);
// Where it is between its last position and its position at this frame of its move
WF2DMAP_POS WF2DMAP_OBJWkFrameMatrixGet(const WF2DMAP_OBJWK *wk);
void WF2DMAP_OBJWkMatrixSet(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos);
void WF2DMAP_OBJWkLastMatrixSet(WF2DMAP_OBJWK *wk, WF2DMAP_POS pos);
// Another object on, or leaving, the square in front of wk, or NULL
const WF2DMAP_OBJWK *WF2DMAP_OBJSysHitCheck(const WF2DMAP_OBJWK *wk, const WF2DMAP_OBJSYS *sys, u32 way);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_OBJ_H
