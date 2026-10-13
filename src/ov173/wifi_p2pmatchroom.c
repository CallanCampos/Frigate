#include "types.h"
#include "app/wifi_p2pmatchroom.h"
#include "app/wf2dmap/wf2dmap_judge.h"
#include "app/wifi_p2pmatchroom_map.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/math_util.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/os.h"
#include "nnsys/gfd.h"

// The room of the Wi-Fi Club's match board, where the player walks among the friends who are online. The ROM has no
// name for this file: it is named after the older games' wifi_p2pmatchroom.c, and so are its functions

// How long a friend takes to spin in and out, in frames
#define WIFI_MCR_NPC_KURU_FRAMES 45
// The frames of a friend's jump, and how many of them it stands before jumping
#define WIFI_MCR_NPC_JUMP_FRAMES 28
#define WIFI_MCR_NPC_JUMP_WAIT 16
// A jump is this many frames up and down
#define WIFI_MCR_NPC_JUMP_ONE 6
// How long a friend waits before it turns: at least the minimum, plus up to the random part
#define WIFI_MCR_NPC_TURN_WAIT_MIN 0x100
#define WIFI_MCR_NPC_TURN_WAIT_RANDOM 0x400

// Where the color of a PC's screen is in the BG palettes, in bytes: PC n has color n + 1 of palette 7
#define WIFI_MCR_PCANM_COLOR(pc) (((pc) + 1) * 2 + 7 * 32)
#define WIFI_MCR_PCANM_STEP_NUM 8

// A step of the animation of the PC the player uses
typedef struct {
    // Whether the step ends after wait frames, rather than when asked to
    u8 timed;
    u8 wait;
    // Whether the step changes the color, and the color it changes it to
    u8 trans;
    u8 color;
} MCR_PCANM_STEP;

static BOOL WifiMcr_PlayerKeyCheck(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_PlayerKeyCheckAll(WIFI_MATCHROOM *wk);
static void WifiMcr_DelPeopleAll(WIFI_MATCHROOM *wk);
static BOOL WifiMcr_PlayerMoveEndCheck(const WIFI_MATCHROOM *wk);
static BOOL WifiMcr_PlayerFrontPcCheck(const WIFI_MATCHROOM *wk);
static void WifiMcr_ReqCmdMain(WIFI_MATCHROOM *wk);
static void WifiMcr_MoveObjMoveAll(WIFI_MATCHROOM *wk);
static void WifiMcr_MoveObjDrawAll(WIFI_MATCHROOM *wk);
static MCR_MOVEOBJ *WifiMcr_HitObjGet(WIFI_MATCHROOM *wk, const MCR_MOVEOBJ *obj, u32 way);
static void WifiMcr_ObjDrawInit(WIFI_MATCHROOM *wk, u32 heroView, u32 heapId);
static void WifiMcr_ObjDrawExit(WIFI_MATCHROOM *wk);
static void WifiMcr_ScrDrawInit(WIFI_MATCHROOM *wk, u32 heapId, ArcTool *arc, u32 mapNo);
static void WifiMcr_ScrDrawExit(WIFI_MATCHROOM *wk);
static void WifiMcr_ClactInit(MCR_CLACT *clact, u32 heapId, ArcTool *arc);
static void WifiMcr_ClactExit(MCR_CLACT *clact);
static void WifiMcr_ClactResLoad(MCR_CLACT *clact, u32 heapId, ArcTool *arc);
static void WifiMcr_ClactResFree(MCR_CLACT *clact);
static void WifiMcr_ClactActorAdd(MCR_CLACT *clact, u32 heapId);
static void WifiMcr_ClactActorDel(MCR_CLACT *clact);
static void WifiMcr_BgInit(void *unk, u32 heapId);
static void WifiMcr_BgExit(void *unk);
static void WifiMcr_BgLoad(void *unk, u32 heapId, ArcTool *arc, u32 arcId);
static void WifiMcr_ClactResLoadCore(MCR_CLACT *clact, u32 heapId, ArcTool *arc);
static void WifiMcr_ClactResFreeCore(MCR_CLACT *clact);
static void WifiMcr_ClactActorAddCore(MCR_CLACT *clact, u32 heapId);
static void WifiMcr_ClactActorDelCore(MCR_CLACT *clact);
static void WifiMcr_ExitCursorOn(WIFI_MATCHROOM *wk);
static void WifiMcr_ExitCursorOff(WIFI_MATCHROOM *wk);
static void WifiMcr_CursorSet(WIFI_MATCHROOM *wk, WF2DMAP_POS pos, u32 priority);
static void WifiMcr_CursorHide(WIFI_MATCHROOM *wk);
static BOOL WifiMcr_PlayerOnExitCheck(const WIFI_MATCHROOM *wk);
static void WifiMcr_FriendSeatPosGet(const WIFI_MATCHROOM *wk, WF2DMAP_POS *pos, u32 friendIndex);
static void WifiMcr_ExitPosGet(const WIFI_MATCHROOM *wk, WF2DMAP_POS *pos);
static MCR_MOVEOBJ *WifiMcr_FreeObjGet(WIFI_MATCHROOM *wk);
static void WifiMcr_ObjDrawNew(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, BOOL hero);
static void WifiMcr_ObjNew(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, const WF2DMAP_POS *pos, u32 friendNo, u16 view,
                           u8 way, u32 status);
static BOOL WifiMcr_ObjAccessCheck(const MCR_MOVEOBJ *obj);
static void WifiMcr_PlayerMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_NpcInMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_NpcNormalMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_NpcJumpMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_DrawNone(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_DrawKuru(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_DrawJump(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static BOOL WifiMcr_PlayerMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static BOOL WifiMcr_NpcInMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static BOOL WifiMcr_NpcOutMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static BOOL WifiMcr_NpcNormalMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static BOOL WifiMcr_NpcJumpMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static BOOL WifiMcr_NpcPauseMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
static void WifiMcr_ReqCmdSet(WIFI_MATCHROOM *wk, u32 cmd, u32 way, u32 playId);
static void WifiMcr_ReqCmdSetObj(WIFI_MATCHROOM *wk, u32 cmd, const MCR_MOVEOBJ *obj);
static BOOL WifiMcr_NpcMoveSetCheck(const MCR_MOVEOBJ *obj);
static void WifiMcr_NpcMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, u32 moveId);
static void WifiMcr_PcAnmInit(WIFI_MATCHROOM *wk, MCR_PCANM *anm, ArcTool *arc, u32 arcId);
static void WifiMcr_PcAnmExit(WIFI_MATCHROOM *wk, MCR_PCANM *anm);
static void WifiMcr_PcAnmMain(WIFI_MATCHROOM *wk, MCR_PCANM *anm);
static void WifiMcr_PcAnmFreeMain(MCR_PCANM *anm);
static void WifiMcr_PcAnmUseMain(MCR_PCANM *anm);
static const void *WifiMcr_PcAnmColorGet(const MCR_PCANM *anm, u32 color);
static void WifiMcr_PcAnmUseEnd(MCR_PCANM *anm);
static void WifiMcr_PcAnmNextReq(MCR_PCANM *anm);
static void WifiMcr_PcAnmUseStart(MCR_PCANM *anm, u8 pcNo);
static void WifiMcr_PcAnmUseStop(MCR_PCANM *anm);

// How long a free PC's screen keeps each of its two colors
static const u16 sWifiMcrPcAnmFreeWait[2] = { 3, 2 };

static const MCR_PCANM_STEP sWifiMcrPcAnmSteps[WIFI_MCR_PCANM_STEP_NUM] = {
    { FALSE, 0, FALSE, 1 }, { TRUE, 2, TRUE, 0 }, { TRUE, 4, TRUE, 1 }, { TRUE, 2, TRUE, 0 },
    { FALSE, 0, FALSE, 1 }, { TRUE, 2, TRUE, 1 }, { TRUE, 4, TRUE, 0 }, { TRUE, 3, TRUE, 1 },
};

void WIFI_MCR_Init(WIFI_MATCHROOM *wk, u32 heapId, ArcTool *arc, u32 heroView, u32 friendNum, u32 arcId,
                   ClActUnit *unit, ClActRenderer *renderer) {
    u32 mapNo = 0;
    WIFI_MCR_MAPSIZE size;

    sys_memset(wk, 0, sizeof(WIFI_MATCHROOM));

    if (friendNum != 0) {
        mapNo = (friendNum - 1) / WIFI_MCR_BLOCK_FRIEND_NUM;
    }

    wk->heapId = heapId;
    wk->clact.unit = unit;
    wk->clact.renderer = renderer;

    WifiMcr_ClactInit(&wk->clact, heapId, arc);
    WifiMcr_BgInit(wk->unk10, heapId);

    size = WIFI_MCR_MapSizeGet(mapNo);
    wk->mapSys = WF2DMAP_MAPSysInit(size.x, size.y, heapId);
    WF2DMAP_MAPSysDataSet(wk->mapSys, WIFI_MCR_MapDataGet(mapNo));

    wk->objSys = WF2DMAP_OBJSysInit(WIFI_MCR_OBJ_MAX, heapId);
    WifiMcr_ObjDrawInit(wk, heroView, heapId);
    WF2DMAP_SCROLLSysDataInit(&wk->scroll);
    WifiMcr_ScrDrawInit(wk, heapId, arc, mapNo);
    WifiMcr_BgLoad(wk->unk10, heapId, arc, arcId);
    WifiMcr_PcAnmInit(wk, &wk->pcAnm, arc, arcId);
    wk->cmdQueue = WF2DMAP_REQCMDQSysInit(128, heapId);

    wk->init = TRUE;
}

void WIFI_MCR_Dest(WIFI_MATCHROOM *wk) {
    WF2DMAP_REQCMDQSysExit(wk->cmdQueue);
    WifiMcr_ScrDrawExit(wk);
    WifiMcr_PcAnmExit(wk, &wk->pcAnm);
    WifiMcr_DelPeopleAll(wk);
    WifiMcr_ObjDrawExit(wk);
    WF2DMAP_OBJSysExit(wk->objSys);
    WF2DMAP_MAPSysExit(wk->mapSys);
    WifiMcr_ClactExit(&wk->clact);
    WifiMcr_BgExit(wk->unk10);
    sys_memset(wk, 0, sizeof(WIFI_MATCHROOM));
}

BOOL WIFI_MCR_GetInitFlag(const WIFI_MATCHROOM *wk) {
    return wk->init;
}

u32 WIFI_MCR_Main(WIFI_MATCHROOM *wk) {
    // The room runs at 30 frames a second
    if (OS_GetVBlankCount() & 1) {
        WF2DMAP_OBJSysMain(wk->objSys);
        WifiMcr_MoveObjMoveAll(wk);
        WifiMcr_ReqCmdMain(wk);
        WF2DMAP_SCRContSysMain(&wk->scroll, wk->player->obj);
        WF2DMAP_SCRDrawSysMain(wk->scrDraw, &wk->scroll);
        WF2DMAP_OBJDrawSysUpdata(wk->objDrawSys);
        WifiMcr_MoveObjDrawAll(wk);
        WifiMcr_ExitCursorOff(wk);
    }

    if (WifiMcr_PlayerMoveEndCheck(wk) == TRUE) {
        WifiMcr_PlayerKeyCheckAll(wk);

        if (WifiMcr_PlayerOnExitCheck(wk) == TRUE) {
            WifiMcr_ExitCursorOn(wk);
            if (GCTX_HIDGetHeldKeys() & PAD_KEY_DOWN) {
                return WIFI_MCR_RET_CANCEL;
            }
        }

        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            if (WIFI_MCR_PlayerSelect(wk) != 0) {
                return WIFI_MCR_RET_SELECT;
            }
            if (WifiMcr_PlayerFrontPcCheck(wk) == TRUE) {
                return WIFI_MCR_RET_MYSELECT;
            }
        }
    }

    return WIFI_MCR_RET_NONE;
}

static BOOL WifiMcr_PlayerKeyCheck(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    u32 way = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_WAY);

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        MCR_MOVEOBJ *hitObj = WifiMcr_HitObjGet(wk, obj, way);
        u32 friendNo;

        if (hitObj != NULL) {
            if (WifiMcr_ObjAccessCheck(hitObj) == TRUE) {
                friendNo = WF2DMAP_OBJWkDataGet(hitObj->obj, WF2DMAP_OBJPM_PLID);
            } else {
                friendNo = 0;
            }
        } else {
            friendNo = 0;
        }
        obj->accessFriend = friendNo;
    }

    return TRUE;
}

static void WifiMcr_PlayerKeyCheckAll(WIFI_MATCHROOM *wk) {
    int i;

    for (i = 0; i < WIFI_MCR_OBJ_MAX; i++) {
        if (wk->objs[i].obj != NULL) {
            WifiMcr_PlayerKeyCheck(wk, &wk->objs[i]);
        }
    }
}

u8 WIFI_MCR_PlayerSelect(const WIFI_MATCHROOM *wk) {
    return wk->player->accessFriend;
}

u32 WIFI_MCR_GetPlayerOnMapParam(const WIFI_MATCHROOM *wk) {
    WF2DMAP_POS pos;

    pos = WF2DMAP_OBJWkMatrixGet(wk->player->obj);
    return WF2DMAP_MAPSysParamGet(wk->mapSys, pos.x / WF2DMAP_GRID_SIZE, pos.y / WF2DMAP_GRID_SIZE);
}

u32 WIFI_MCR_GetPlayerOnUnderMapParam(const WIFI_MATCHROOM *wk) {
    WF2DMAP_POS pos;

    pos = WF2DMAP_OBJWkMatrixGet(wk->player->obj);
    return WF2DMAP_MAPSysParamGet(wk->mapSys, pos.x / WF2DMAP_GRID_SIZE, pos.y / WF2DMAP_GRID_SIZE + 1);
}

void WIFI_MCR_CursorOn(WIFI_MATCHROOM *wk, const MCR_MOVEOBJ *obj) {
    WF2DMAP_POS pos;
    u32 priority;

    pos = WF2DMAP_OBJWkMatrixGet(obj->obj);
    priority = WF2DMAP_OBJDrawWkDrawPriGet(obj->draw);
    WifiMcr_CursorSet(wk, pos, priority - 2);
    WF2DMAP_OBJDrawWkShadowDrawFlagSet(obj->draw, FALSE);
}

void WIFI_MCR_ShadowOn(WIFI_MATCHROOM *wk, const MCR_MOVEOBJ *obj) {
    WF2DMAP_OBJDrawWkShadowDrawFlagSet(obj->draw, TRUE);
}

void WIFI_MCR_CursorOff(WIFI_MATCHROOM *wk) {
    WifiMcr_CursorHide(wk);
}

void WIFI_MCR_PlayerMovePause(WIFI_MATCHROOM *wk, BOOL pause) {
    wk->playerPause = pause;
}

BOOL WIFI_MCR_PlayerMovePauseGet(const WIFI_MATCHROOM *wk) {
    return wk->playerPause;
}

MCR_MOVEOBJ *WIFI_MCR_SetPlayer(WIFI_MATCHROOM *wk, u32 view) {
    MCR_MOVEOBJ *obj;
    WF2DMAP_POS pos;

    obj = WifiMcr_FreeObjGet(wk);
    wk->player = obj;

    WifiMcr_ExitPosGet(wk, &pos);
    WifiMcr_ObjNew(wk, obj, &pos, 0, view, WF2DMAP_WAY_UP, WF2DMAP_OBJST_NONE);
    WifiMcr_PlayerMoveInit(wk, obj);
    WifiMcr_ObjDrawNew(wk, obj, TRUE);

    WF2DMAP_SCRContSysMain(&wk->scroll, wk->player->obj);
    WF2DMAP_SCRDrawSysMain(wk->scrDraw, &wk->scroll);

    return obj;
}

MCR_MOVEOBJ *WIFI_MCR_SetNpc(WIFI_MATCHROOM *wk, u32 view, u32 friendNo) {
    MCR_MOVEOBJ *obj;
    WF2DMAP_POS pos;
    WF2DMAP_POS playerPos;

    obj = WifiMcr_FreeObjGet(wk);
    WifiMcr_FriendSeatPosGet(wk, &pos, friendNo - 1);

    if (wk->player != NULL) {
        playerPos = WF2DMAP_OBJWkMatrixGet(wk->player->obj);
        if (playerPos.x == pos.x && playerPos.y == pos.y) {
            return NULL;
        }
    }

    WifiMcr_ObjNew(wk, obj, &pos, friendNo, view, WF2DMAP_WAY_DOWN, WF2DMAP_OBJST_BUSY);
    WifiMcr_NpcInMoveInit(wk, obj);
    WifiMcr_ObjDrawNew(wk, obj, FALSE);

    return obj;
}

void WIFI_MCR_DelPeopleReq(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_WAY);

    obj->move = WifiMcr_NpcOutMove;
    obj->drawFunc = WifiMcr_DrawKuru;
    obj->moveCount = 0;
    obj->moveState = WIFI_MCR_MOVESTATE_NPC_OUT;

    WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, FALSE);
    WF2DMAP_OBJDrawWkKuruAnimeStart(obj->draw);
    WifiMcr_ReqCmdSetObj(wk, WF2DMAP_OBJST_BUSY, obj);
}

void WIFI_MCR_DelPeople(MCR_MOVEOBJ *obj) {
    WF2DMAP_OBJDrawWkDel(obj->draw);
    WF2DMAP_OBJWkDel(obj->obj);
    sys_memset(obj, 0, sizeof(MCR_MOVEOBJ));
}

u8 WIFI_MCR_GetFriendNo(const MCR_MOVEOBJ *obj) {
    return WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_PLID);
}

u32 WIFI_MCR_GetRetWay(const MCR_MOVEOBJ *obj) {
    return WF2DMAP_OBJToolRetWayGet(WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_WAY));
}

void WIFI_MCR_NpcPauseOn(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, u32 way) {
    WF2DMAP_POS pos;

    obj->moveTmp = obj->move;
    obj->move = WifiMcr_NpcPauseMove;
    obj->moveStateTmp = obj->moveState;
    obj->moveState = WIFI_MCR_MOVESTATE_NPC_PAUSE;

    WifiMcr_ReqCmdSet(wk, WF2DMAP_OBJST_NONE, way, WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_PLID));

    WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, FALSE);
    WF2DMAP_OBJDrawWkWaySet(obj->draw, way);

    pos.x = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_X);
    pos.y = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_Y);
    WF2DMAP_OBJDrawWkMatrixSet(obj->draw, pos);
}

void WIFI_MCR_NpcPauseOff(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    if (obj != NULL) {
        if (obj->moveTmp != NULL) {
            obj->move = obj->moveTmp;
            obj->moveTmp = NULL;
            obj->moveState = obj->moveStateTmp;
            WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, TRUE);
        }
    }
}

void WIFI_MCR_NpcMoveSet(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, u32 moveId) {
    // A friend that is spinning in takes its move when it has arrived
    if (WifiMcr_NpcMoveSetCheck(obj) == FALSE) {
        obj->moveId = moveId;
        return;
    }

    WifiMcr_NpcMoveInit(wk, obj, moveId);
}

void WIFI_MCR_PCAnmStart(WIFI_MATCHROOM *wk) {
    if (WifiMcr_PlayerFrontPcCheck(wk) == TRUE) {
        s8 pcNo = WIFI_MCR_GetPlayerOnUnderMapParam(wk);

        pcNo -= WIFI_MCR_MAPPM_BLOCK0;
        WifiMcr_PcAnmUseStart(&wk->pcAnm, pcNo);
    }
}

void WIFI_MCR_PCAnmOff(WIFI_MATCHROOM *wk) {
    WifiMcr_PcAnmUseStop(&wk->pcAnm);
}

void WIFI_MCR_PCAnmMain(WIFI_MATCHROOM *wk) {
    if (OS_GetVBlankCount() & 1) {
        WifiMcr_PcAnmMain(wk, &wk->pcAnm);
    }
}

static void WifiMcr_DelPeopleAll(WIFI_MATCHROOM *wk) {
    int i;

    for (i = 0; i < WIFI_MCR_OBJ_MAX; i++) {
        if (wk->objs[i].obj != NULL) {
            WIFI_MCR_DelPeople(&wk->objs[i]);
        }
    }
}

static BOOL WifiMcr_PlayerMoveEndCheck(const WIFI_MATCHROOM *wk) {
    if (WF2DMAP_OBJWkDataGet(wk->player->obj, WF2DMAP_OBJPM_ST) == WF2DMAP_OBJST_NONE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL WifiMcr_PlayerFrontPcCheck(const WIFI_MATCHROOM *wk) {
    WF2DMAP_POS pos;
    WF2DMAP_POS frontPos;
    u32 way;
    u32 param;

    pos = WF2DMAP_OBJWkMatrixGet(wk->player->obj);
    way = WF2DMAP_OBJWkDataGet(wk->player->obj, WF2DMAP_OBJPM_WAY);
    frontPos = WF2DMAP_OBJToolWayPosGet(pos, way);
    param = WF2DMAP_MAPSysParamGet(wk->mapSys, frontPos.x / WF2DMAP_GRID_SIZE, frontPos.y / WF2DMAP_GRID_SIZE);

    if (way == WF2DMAP_WAY_UP && param == WIFI_MCR_MAPPM_PC) {
        return TRUE;
    }
    return FALSE;
}

static void WifiMcr_ReqCmdMain(WIFI_MATCHROOM *wk) {
    WF2DMAP_REQCMD req;
    WF2DMAP_ACTCMD act;

    while (WF2DMAP_REQCMDQSysCmdPop(wk->cmdQueue, &req) == TRUE) {
        if (WF2DMAP_JUDGESysCmdJudge(wk->mapSys, wk->objSys, &req, &act) == TRUE) {
            WF2DMAP_OBJSysCmdSet(wk->objSys, &act);
        }
    }
}

static void WifiMcr_MoveObjMoveAll(WIFI_MATCHROOM *wk) {
    int i;

    for (i = 0; i < WIFI_MCR_OBJ_MAX; i++) {
        if (wk->objs[i].obj != NULL) {
            if (wk->objs[i].move(wk, &wk->objs[i]) == TRUE) {
                WIFI_MCR_DelPeople(&wk->objs[i]);
            }
        }
    }
}

static void WifiMcr_MoveObjDrawAll(WIFI_MATCHROOM *wk) {
    int i;

    for (i = 0; i < WIFI_MCR_OBJ_MAX; i++) {
        if (wk->objs[i].obj != NULL) {
            wk->objs[i].drawFunc(wk, &wk->objs[i]);
        }
    }
}

// The character in front of obj, or NULL
static MCR_MOVEOBJ *WifiMcr_HitObjGet(WIFI_MATCHROOM *wk, const MCR_MOVEOBJ *obj, u32 way) {
    const WF2DMAP_OBJWK *hitObj = WF2DMAP_OBJSysHitCheck(obj->obj, wk->objSys, way);
    int i;

    if (hitObj != NULL) {
        for (i = 0; i < WIFI_MCR_OBJ_MAX; i++) {
            if (wk->objs[i].obj == hitObj) {
                return &wk->objs[i];
            }
        }
    }
    return NULL;
}

static void WifiMcr_ObjDrawInit(WIFI_MATCHROOM *wk, u32 heroView, u32 heapId) {
    wk->objDrawSys = WF2DMAP_OBJDrawSysInit(wk->clact.unit, NULL, WIFI_MCR_OBJ_MAX, heroView, 0, 0, heapId);
    WF2DMAP_OBJDrawSysUnionResSet(wk->objDrawSys, 2, heapId);
}

static void WifiMcr_ObjDrawExit(WIFI_MATCHROOM *wk) {
    WF2DMAP_OBJDrawSysExit(wk->objDrawSys);
}

static void WifiMcr_ScrDrawInit(WIFI_MATCHROOM *wk, u32 heapId, ArcTool *arc, u32 mapNo) {
    // The maps' screens follow each other in the archive
    WF2DMAP_SCRDRAWINIT init = { 0, 0, 0, 28, 0, 0, 3, 0, ARCID_WIFIP2PMATCH, 26, FALSE };

    init.fileId += mapNo;
    wk->scrDraw = WF2DMAP_SCRDrawSysInit(wk->clact.renderer, wk->unk10, &init, heapId);
}

static void WifiMcr_ScrDrawExit(WIFI_MATCHROOM *wk) {
    WF2DMAP_SCRDrawSysExit(wk->scrDraw);
}

static void WifiMcr_ClactInit(MCR_CLACT *clact, u32 heapId, ArcTool *arc) {
    WifiMcr_ClactResLoad(clact, heapId, arc);
    WifiMcr_ClactActorAdd(clact, heapId);
}

static void WifiMcr_ClactExit(MCR_CLACT *clact) {
    WifiMcr_ClactActorDel(clact);
    WifiMcr_ClactResFree(clact);
}

static void WifiMcr_ClactResLoad(MCR_CLACT *clact, u32 heapId, ArcTool *arc) {
    WifiMcr_ClactResLoadCore(clact, heapId, arc);
}

static void WifiMcr_ClactResFree(MCR_CLACT *clact) {
    WifiMcr_ClactResFreeCore(clact);
}

static void WifiMcr_ClactActorAdd(MCR_CLACT *clact, u32 heapId) {
    WifiMcr_ClactActorAddCore(clact, heapId);
}

static void WifiMcr_ClactActorDel(MCR_CLACT *clact) {
    WifiMcr_ClactActorDelCore(clact);
}

static void WifiMcr_BgInit(void *unk, u32 heapId) {
}

static void WifiMcr_BgExit(void *unk) {
}

static void WifiMcr_BgLoad(void *unk, u32 heapId, ArcTool *arc, u32 arcId) {
    GFL_BGSysLoadNCLRDefault(arcId, 7, 0, 0, 0x100, heapId);
    GFL_BGSysResetStdPalette(0, 0);
    GFL_BGSysLoadNCGRStatic(arcId, 16, 0, 0, 0, FALSE, heapId);
}

static void WifiMcr_ClactResLoadCore(MCR_CLACT *clact, u32 heapId, ArcTool *arc) {
    clact->chars = func_0204b81c(arc, 17, FALSE, 0, heapId);
    clact->palette = func_0204bbb8(arc, 8, 0, 11 * 32, 0, 2, heapId);
    clact->cellAnims = func_0204bde0(arc, 20, 32, heapId);
}

static void WifiMcr_ClactResFreeCore(MCR_CLACT *clact) {
    func_0204b98c(clact->chars);
    func_0204bcd0(clact->palette);
    func_0204be64(clact->cellAnims);
}

static void WifiMcr_ClactActorAddCore(MCR_CLACT *clact, u32 heapId) {
    ClActorSetup setup;

    setup.x = 1;
    setup.y = 1;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    clact->exitCursor = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup, 0, heapId);

    setup.sequence = 2;
    setup.priority = 254;
    setup.bgPriority = 3;
    clact->cursor = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup, 0, heapId);

    func_0204c124(clact->exitCursor, FALSE);
    func_0204c124(clact->cursor, FALSE);
    func_0204c520(clact->exitCursor, TRUE);
    func_0204c520(clact->cursor, TRUE);
}

static void WifiMcr_ClactActorDelCore(MCR_CLACT *clact) {
    func_0204c108(clact->exitCursor);
    func_0204c108(clact->cursor);
}

static void WifiMcr_ExitCursorOn(WIFI_MATCHROOM *wk) {
    WF2DMAP_POS pos;
    ClActorPos actorPos;

    WifiMcr_ExitPosGet(wk, &pos);
    pos.x += 8;
    pos.y += 32;

    actorPos.x = pos.x;
    actorPos.y = pos.y;
    func_0204c210(wk->clact.exitCursor, &actorPos);
    func_0204c124(wk->clact.exitCursor, TRUE);
}

static void WifiMcr_ExitCursorOff(WIFI_MATCHROOM *wk) {
    func_0204c124(wk->clact.exitCursor, FALSE);
}

static void WifiMcr_CursorSet(WIFI_MATCHROOM *wk, WF2DMAP_POS pos, u32 priority) {
    ClActorPos actorPos;

    actorPos.x = pos.x + 8;
    actorPos.y = pos.y;
    func_0204c210(wk->clact.cursor, &actorPos);
    func_0204c438(wk->clact.cursor, priority);
    func_0204c124(wk->clact.cursor, TRUE);
}

static void WifiMcr_CursorHide(WIFI_MATCHROOM *wk) {
    func_0204c124(wk->clact.cursor, FALSE);
}

static BOOL WifiMcr_PlayerOnExitCheck(const WIFI_MATCHROOM *wk) {
    WF2DMAP_POS pos;
    u32 way;
    u32 param;

    if (wk->player == NULL) {
        return FALSE;
    }

    pos = WF2DMAP_OBJWkMatrixGet(wk->player->obj);
    way = WF2DMAP_OBJWkDataGet(wk->player->obj, WF2DMAP_OBJPM_WAY);
    param = WF2DMAP_MAPSysParamGet(wk->mapSys, pos.x / WF2DMAP_GRID_SIZE, pos.y / WF2DMAP_GRID_SIZE);

    if (param == WIFI_MCR_MAPPM_EXIT && way == WF2DMAP_WAY_DOWN) {
        return TRUE;
    }
    return FALSE;
}

// A friend's seat is the square of its number in the block that the square below names
static void WifiMcr_FriendSeatPosGet(const WIFI_MATCHROOM *wk, WF2DMAP_POS *pos, u32 friendIndex) {
    u32 x;
    u32 y;
    u32 gridX;
    u32 gridY;
    u32 block;
    u32 seat;

    seat = friendIndex % WIFI_MCR_BLOCK_FRIEND_NUM;
    block = friendIndex / WIFI_MCR_BLOCK_FRIEND_NUM;
    gridX = WF2DMAP_MAPSysGridXGet(wk->mapSys);
    gridY = WF2DMAP_MAPSysGridYGet(wk->mapSys);

    for (y = 0; y < gridY; y++) {
        for (x = 0; x < gridX; x++) {
            u32 param = WF2DMAP_MAPSysParamGet(wk->mapSys, x, y);

            if (param == WIFI_MCR_MAPPM_FRIEND0 + seat) {
                param = WF2DMAP_MAPSysParamGet(wk->mapSys, x, y + 1);
                if (param == WIFI_MCR_MAPPM_BLOCK0 + block) {
                    pos->x = x * WF2DMAP_GRID_SIZE;
                    pos->y = y * WF2DMAP_GRID_SIZE;
                    return;
                }
            }
        }
    }
}

static void WifiMcr_ExitPosGet(const WIFI_MATCHROOM *wk, WF2DMAP_POS *pos) {
    u32 x;
    u32 y;
    u32 gridX = WF2DMAP_MAPSysGridXGet(wk->mapSys);
    u32 gridY = WF2DMAP_MAPSysGridYGet(wk->mapSys);

    for (y = 0; y < gridY; y++) {
        for (x = 0; x < gridX; x++) {
            if (WF2DMAP_MAPSysParamGet(wk->mapSys, x, y) == WIFI_MCR_MAPPM_EXIT) {
                pos->x = x * WF2DMAP_GRID_SIZE;
                pos->y = y * WF2DMAP_GRID_SIZE;
                return;
            }
        }
    }
}

static MCR_MOVEOBJ *WifiMcr_FreeObjGet(WIFI_MATCHROOM *wk) {
    int i;

    for (i = 0; i < WIFI_MCR_OBJ_MAX; i++) {
        if (wk->objs[i].obj == NULL) {
            return &wk->objs[i];
        }
    }
    return NULL;
}

static void WifiMcr_ObjDrawNew(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, BOOL hero) {
    obj->draw = WF2DMAP_OBJDrawWkNew(wk->objDrawSys, obj->obj, hero, wk->heapId);

    // A friend spins in
    if (hero == FALSE) {
        WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, FALSE);
        WF2DMAP_OBJDrawWkKuruAnimeStart(obj->draw);
        WF2DMAP_OBJDrawWkDrawFlagSet(obj->draw, FALSE);
    }
}

static void WifiMcr_ObjNew(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, const WF2DMAP_POS *pos, u32 friendNo, u16 view,
                           u8 way, u32 status) {
    WF2DMAP_OBJDATA data;

    data.x = pos->x;
    data.y = pos->y;
    data.playId = friendNo;
    data.status = status;
    data.way = way;
    data.charaId = view;
    obj->obj = WF2DMAP_OBJWkNew(wk->objSys, &data);
}

// Whether the player can talk to the character
static BOOL WifiMcr_ObjAccessCheck(const MCR_MOVEOBJ *obj) {
    switch (obj->moveState) {
    case WIFI_MCR_MOVESTATE_PLAYER:
    case WIFI_MCR_MOVESTATE_NPC_NORMAL:
    case WIFI_MCR_MOVESTATE_NPC_JUMP:
        return TRUE;
    case WIFI_MCR_MOVESTATE_NPC_IN:
    case WIFI_MCR_MOVESTATE_NPC_OUT:
    case WIFI_MCR_MOVESTATE_NPC_PAUSE:
        return FALSE;
    }
    return FALSE;
}

static void WifiMcr_PlayerMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->move = WifiMcr_PlayerMove;
    obj->drawFunc = WifiMcr_DrawNone;
    obj->moveState = WIFI_MCR_MOVESTATE_PLAYER;
    WifiMcr_ReqCmdSetObj(wk, WF2DMAP_OBJST_NONE, obj);
}

static void WifiMcr_NpcInMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->move = WifiMcr_NpcInMove;
    obj->drawFunc = WifiMcr_DrawKuru;
    obj->moveCount = WIFI_MCR_NPC_KURU_FRAMES;
    obj->moveState = WIFI_MCR_MOVESTATE_NPC_IN;
}

static void WifiMcr_NpcNormalMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->move = WifiMcr_NpcNormalMove;
    obj->drawFunc = WifiMcr_DrawNone;
    obj->moveCount = GFL_RandomLC(WIFI_MCR_NPC_TURN_WAIT_RANDOM) + WIFI_MCR_NPC_TURN_WAIT_MIN;
    obj->moveState = WIFI_MCR_MOVESTATE_NPC_NORMAL;
    WifiMcr_ReqCmdSetObj(wk, WF2DMAP_OBJST_NONE, obj);
    WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, TRUE);
}

static void WifiMcr_NpcJumpMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->move = WifiMcr_NpcJumpMove;
    obj->drawFunc = WifiMcr_DrawJump;
    obj->moveCount = WIFI_MCR_NPC_JUMP_FRAMES;
    obj->moveState = WIFI_MCR_MOVESTATE_NPC_JUMP;
    WifiMcr_ReqCmdSetObj(wk, WF2DMAP_OBJST_BUSY, obj);
    WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, FALSE);
}

static void WifiMcr_DrawNone(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
}

// Spinning, the character comes down from, or goes up to, 192 pixels above its square
static void WifiMcr_DrawKuru(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    WF2DMAP_POS pos;
    WF2DMAP_POS shadowPos;

    WF2DMAP_OBJDrawWkKuruAnimeMain(obj->draw);
    if (WF2DMAP_OBJDrawWkDrawFlagGet(obj->draw) == FALSE) {
        WF2DMAP_OBJDrawWkDrawFlagSet(obj->draw, TRUE);
    }

    pos.x = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_X);
    pos.y = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_Y);
    shadowPos = pos;

    pos.y += FX_Mul(func_02044304(obj->moveCount), -192 * FX32_ONE) >> FX32_SHIFT;

    WF2DMAP_OBJDrawWkMatrixSet(obj->draw, pos);
    WF2DMAP_OBJDrawWkShadowMatrixSet(obj->draw, shadowPos);
}

static void WifiMcr_DrawJump(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    WF2DMAP_POS pos;
    WF2DMAP_POS shadowPos;
    s16 count;

    pos.x = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_X);
    pos.y = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_Y);
    shadowPos = pos;

    count = obj->moveCount - WIFI_MCR_NPC_JUMP_WAIT;
    if (count > 0) {
        u16 degrees;

        count %= WIFI_MCR_NPC_JUMP_ONE;
        degrees = (count * 180) / WIFI_MCR_NPC_JUMP_ONE;
        pos.y -= FX_Mul(func_02044304(degrees), 4 * FX32_ONE) >> FX32_SHIFT;
    }

    WF2DMAP_OBJDrawWkMatrixSet(obj->draw, pos);
    WF2DMAP_OBJDrawWkShadowMatrixSet(obj->draw, shadowPos);
    WF2DMAP_OBJDrawWkWaySet(obj->draw, WF2DMAP_WAY_DOWN);
}

static BOOL WifiMcr_PlayerMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    u32 way = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_WAY);
    u32 playId = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_PLID);
    u32 cmd;

    if (WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_ST) != WF2DMAP_OBJST_NONE) {
        return FALSE;
    }
    if (wk->playerPause == TRUE) {
        return FALSE;
    }

    cmd = (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B) ? WF2DMAP_OBJST_RUN : WF2DMAP_OBJST_WALK;

    // A key of another way than the one the player faces only turns the player
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_UP) {
        if (way == WF2DMAP_WAY_UP) {
            WifiMcr_ReqCmdSet(wk, cmd, way, playId);
        } else {
            WifiMcr_ReqCmdSet(wk, WF2DMAP_OBJST_TURN, WF2DMAP_WAY_UP, playId);
        }
    } else if (GCTX_HIDGetHeldKeys() & PAD_KEY_DOWN) {
        if (way == WF2DMAP_WAY_DOWN) {
            WifiMcr_ReqCmdSet(wk, cmd, way, playId);
        } else {
            WifiMcr_ReqCmdSet(wk, WF2DMAP_OBJST_TURN, WF2DMAP_WAY_DOWN, playId);
        }
    } else if (GCTX_HIDGetHeldKeys() & PAD_KEY_LEFT) {
        if (way == WF2DMAP_WAY_LEFT) {
            WifiMcr_ReqCmdSet(wk, cmd, way, playId);
        } else {
            WifiMcr_ReqCmdSet(wk, WF2DMAP_OBJST_TURN, WF2DMAP_WAY_LEFT, playId);
        }
    } else if (GCTX_HIDGetHeldKeys() & PAD_KEY_RIGHT) {
        if (way == WF2DMAP_WAY_RIGHT) {
            WifiMcr_ReqCmdSet(wk, cmd, way, playId);
        } else {
            WifiMcr_ReqCmdSet(wk, WF2DMAP_OBJST_TURN, WF2DMAP_WAY_RIGHT, playId);
        }
    }

    return FALSE;
}

static BOOL WifiMcr_NpcInMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->moveCount--;
    if (obj->moveCount <= 0) {
        u32 playerId;
        u32 friendNo;

        WF2DMAP_OBJDrawWkKuruAnimeEnd(obj->draw);
        WF2DMAP_OBJDrawWkUpdataFlagSet(obj->draw, TRUE);

        playerId = WF2DMAP_OBJWkDataGet(wk->player->obj, WF2DMAP_OBJPM_PLID);
        friendNo = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_PLID);
        if (friendNo == playerId) {
            WifiMcr_PlayerMoveInit(wk, obj);
        } else {
            WifiMcr_NpcMoveInit(wk, obj, obj->moveId);
        }
    }
    return FALSE;
}

static BOOL WifiMcr_NpcOutMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->moveCount++;
    if (obj->moveCount > WIFI_MCR_NPC_KURU_FRAMES) {
        return TRUE;
    }
    return FALSE;
}

static BOOL WifiMcr_NpcNormalMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->moveCount--;
    if (obj->moveCount <= 0) {
        u32 way;

        obj->moveCount = GFL_RandomLC(WIFI_MCR_NPC_TURN_WAIT_RANDOM) + WIFI_MCR_NPC_TURN_WAIT_MIN;
        way = GFL_RandomLC(WF2DMAP_WAY_NUM);
        WifiMcr_ReqCmdSet(wk, WF2DMAP_OBJST_NONE, way, WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_PLID));
    }
    return FALSE;
}

static BOOL WifiMcr_NpcJumpMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    obj->moveCount--;
    if (obj->moveCount < 0) {
        obj->moveCount = WIFI_MCR_NPC_JUMP_FRAMES;
    }
    return FALSE;
}

static BOOL WifiMcr_NpcPauseMove(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj) {
    return FALSE;
}

static void WifiMcr_ReqCmdSet(WIFI_MATCHROOM *wk, u32 cmd, u32 way, u32 playId) {
    WF2DMAP_REQCMD req;

    req.cmd = cmd;
    req.way = way;
    req.playId = playId;
    WF2DMAP_REQCMDQSysCmdPush(wk->cmdQueue, &req);
}

// Asks for cmd with the way the character faces
static void WifiMcr_ReqCmdSetObj(WIFI_MATCHROOM *wk, u32 cmd, const MCR_MOVEOBJ *obj) {
    u32 way = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_WAY);
    u32 playId = WF2DMAP_OBJWkDataGet(obj->obj, WF2DMAP_OBJPM_PLID);

    WifiMcr_ReqCmdSet(wk, cmd, way, playId);
}

// Whether the character is done spinning
static BOOL WifiMcr_NpcMoveSetCheck(const MCR_MOVEOBJ *obj) {
    if (obj->drawFunc != WifiMcr_DrawKuru) {
        return TRUE;
    }
    return FALSE;
}

static void WifiMcr_NpcMoveInit(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, u32 moveId) {
    switch (moveId) {
    case WIFI_MCR_NPC_MOVE_NORMAL:
        WifiMcr_NpcNormalMoveInit(wk, obj);
        break;
    case WIFI_MCR_NPC_MOVE_JUMP:
        WifiMcr_NpcJumpMoveInit(wk, obj);
        break;
    }
    obj->moveId = moveId;
}

static void WifiMcr_PcAnmInit(WIFI_MATCHROOM *wk, MCR_PCANM *anm, ArcTool *arc, u32 arcId) {
    sys_memset(anm, 0, sizeof(MCR_PCANM));

    anm->buffer = GFL_G2DIOReadNCLR(arcId, 9, &anm->palette, wk->heapId);
    anm->frame = 0;
    anm->freeBits = (1 << WIFI_MCR_PC_NUM) - 1;
    anm->wait = sWifiMcrPcAnmFreeWait[anm->frame];
}

static void WifiMcr_PcAnmExit(WIFI_MATCHROOM *wk, MCR_PCANM *anm) {
    GFL_HeapFree(anm->buffer);
    sys_memset(anm, 0, sizeof(MCR_PCANM));
}

static void WifiMcr_PcAnmMain(WIFI_MATCHROOM *wk, MCR_PCANM *anm) {
    WifiMcr_PcAnmFreeMain(anm);
    WifiMcr_PcAnmUseMain(anm);
}

// The screens of the PCs nobody uses blink
static void WifiMcr_PcAnmFreeMain(MCR_PCANM *anm) {
    anm->wait--;
    if (anm->wait <= 0) {
        u32 color;
        int i;

        anm->frame = (anm->frame + 1) % 2;
        anm->wait = sWifiMcrPcAnmFreeWait[anm->frame];

        if (anm->frame == 1) {
            color = 2;
        } else {
            color = 1;
        }

        for (i = 0; i < WIFI_MCR_PC_NUM; i++) {
            if (anm->freeBits & (1 << i)) {
                NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, WIFI_MCR_PCANM_COLOR(i),
                                                   WifiMcr_PcAnmColorGet(anm, color), sizeof(u16));
            }
        }
    }
}

static void WifiMcr_PcAnmUseMain(MCR_PCANM *anm) {
    u32 trans = FALSE;
    u32 color = 0;
    BOOL next = FALSE;

    if (sWifiMcrPcAnmSteps[anm->seq].timed == TRUE) {
        anm->seqWait--;
        if (anm->seqWait <= 0) {
            next = TRUE;
        }
    } else {
        if (anm->nextReq) {
            anm->nextReq = FALSE;
            next = TRUE;
        }
    }

    if (next) {
        anm->seq++;
        if (anm->seq >= WIFI_MCR_PCANM_STEP_NUM) {
            WifiMcr_PcAnmUseEnd(anm);
            return;
        }

        anm->seqWait = sWifiMcrPcAnmSteps[anm->seq].wait;
        trans = sWifiMcrPcAnmSteps[anm->seq].trans;
        color = sWifiMcrPcAnmSteps[anm->seq].color;
    }

    if (trans) {
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, WIFI_MCR_PCANM_COLOR(anm->pcNo),
                                           WifiMcr_PcAnmColorGet(anm, color), sizeof(u16));
    }
}

static const void *WifiMcr_PcAnmColorGet(const MCR_PCANM *anm, u32 color) {
    const u16 *colors = anm->palette->rawData;

    return &colors[color + 1];
}

// The PC goes back to blinking
static void WifiMcr_PcAnmUseEnd(MCR_PCANM *anm) {
    anm->freeBits |= 1 << anm->pcNo;
    anm->pcNo = 0;
    anm->seq = 0;
    anm->seqWait = 0;
    anm->nextReq = FALSE;
}

static void WifiMcr_PcAnmNextReq(MCR_PCANM *anm) {
    anm->nextReq = TRUE;
}

static void WifiMcr_PcAnmUseStart(MCR_PCANM *anm, u8 pcNo) {
    if (anm->seq != 0) {
        WifiMcr_PcAnmUseEnd(anm);
    }

    anm->pcNo = pcNo;
    anm->freeBits ^= 1 << pcNo;
    WifiMcr_PcAnmNextReq(anm);
}

static void WifiMcr_PcAnmUseStop(MCR_PCANM *anm) {
    if (anm->seq != 0) {
        WifiMcr_PcAnmNextReq(anm);
    }
}
