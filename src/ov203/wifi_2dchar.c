#include "types.h"
#include "app/wf2dmap/wifi_2dchar.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"

// The characters of the Wi-Fi Club's 2D map, named after the ROM's "wifi_2dchar.c"

// No resource
#define WF_2DC_RES_NONE 0xffffffff

// The files of ARCID_WIFI_2DCHAR: for each of the two player characters a cell and an animation, then a palette and
// characters, then the shadow's
#define WF_2DC_ARC_ANM(chara) (((chara) - WF_2DC_CHARA_HERO) * 2)
#define WF_2DC_ARC_PLTT(chara) (((chara) - WF_2DC_CHARA_HERO) * 2 + 4)
#define WF_2DC_ARC_CHAR(chara) (((chara) - WF_2DC_CHARA_HERO) * 2 + 5)
#define WF_2DC_ARC_SHADOW_ANM 8
#define WF_2DC_ARC_SHADOW_CELL 9
#define WF_2DC_ARC_SHADOW_PLTT 10
#define WF_2DC_ARC_SHADOW_CHAR 11

// The files of ARCID_WIFI_UNIONCHAR: the palettes all its characters share, their characters, then a cell and an
// animation for each
#define WF_2DC_ARC_UNIONPLTT 0
#define WF_2DC_ARC_UNIONCHAR(chara) ((chara) + 1)
#define WF_2DC_ARC_UNIONCELL(chara) ((chara) * 2 + 17)
#define WF_2DC_ARC_UNIONANM(chara) ((chara) * 2 + 18)

#define WF_2DC_UNIONPLTT_NUM 8

// Where the shadow is from its character
#define WF_2DC_SHADOW_OFS_X 8
#define WF_2DC_SHADOW_OFS_Y 14

// A pattern animation that never ends
#define WF_2DC_ANMFRAME_LOOP 0xff

// A character's graphics
typedef struct {
    // Whether it is a Union Room character, whose palette is the shared one
    BOOL unionChara;
    u32 chars;
    u32 palette;
    u32 vramType;
    u32 moveType;
} WF_2DCRES;

typedef struct {
    u32 unk0[4];
    u32 cellAnims;
} WF_2DCANMRES;

typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
} WF_2DCSHRES;

struct WF_2DCWK {
    const WF_2DCRES *res;
    ClActor *actor;
    ClActor *shadow;
    u32 anmType;
    u32 way;
    s16 frame;
};

struct WF_2DCSYS {
    ClActUnit *unit;
    void *unk4;
    WF_2DCWK *objs;
    u32 objNum;
    WF_2DCRES res[WF_2DC_CHARA_NUM];
    WF_2DCANMRES anmRes[WF_2DC_CHARA_NUM];
    WF_2DCSHRES shadowRes;
    u32 unk2ec;
    u32 shadowPriority;
    u32 unionPltt;
    ArcTool *handles[2];
    u8 unk300[0x30];
    // The player character that is loaded, which nothing reads
    u32 shadowChara;
};

static void WF_2DC_AnmResLoad(WF_2DCSYS *sys, u32 unused, u32 heapId);
static void WF_2DC_AnmResDel(WF_2DCSYS *sys, u32 unused);
static u32 WF_2DC_CharNoGet(u32 charaId);
static BOOL WF_2DC_CharNoCheck(u32 charaId, u32 moveType);
static void WF_2DC_CharResLoad(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId, BOOL unionChara);
static void WF_2DC_CharResLoadNormal(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId);
static void WF_2DC_CharResLoadUnion(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId);
static void WF_2DC_CharResDel(WF_2DCSYS *sys, u32 charaId);
static void WF_2DC_CharResDelNormal(WF_2DCSYS *sys, u32 charaId);
static void WF_2DC_CharResDelUnion(WF_2DCSYS *sys, u32 charaId);
static BOOL WF_2DC_CharResCheck(const WF_2DCSYS *sys, u32 charaId);
static void WF_2DC_ShResLoad(WF_2DCSYS *sys, u32 vramType, u32 heapId);
static void WF_2DC_ShResDel(WF_2DCSYS *sys);
static BOOL WF_2DC_ShResCheck(const WF_2DCSHRES *res);
static void WF_2DC_UnionPlttLoad(WF_2DCSYS *sys, u32 vramType, u32 plttOffset, u32 heapId);
static void WF_2DC_UnionPlttDel(WF_2DCSYS *sys);
static void WF_2DC_UnionAnmResLoad(WF_2DCSYS *sys, u32 heapId);
static void WF_2DC_UnionAnmResDel(WF_2DCSYS *sys);
static WF_2DCWK *WF_2DC_WkCleanGet(WF_2DCSYS *sys);
static void WF_2DC_WkPatAnmUpdate(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmWayInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmRotaInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmWalkInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmTurnInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmRunInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmWallWalkInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmSlowWalkInit(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmHighWalk2Init(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmHighWalk4Init(WF_2DCWK *wk, u32 way);
static void WF_2DC_WkPatAnmRotaMain(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmWalkMain(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmTurnMain(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmRunMain(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmWallWalkMain(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmSlowWalkMain(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmHighWalk2Main(WF_2DCWK *wk);
static void WF_2DC_WkPatAnmHighWalk4Main(WF_2DCWK *wk);
static BOOL WF_2DC_WkCleanCheck(const WF_2DCWK *wk);

// How many frames each animation lasts
static const u8 sWF_2DC_AnmFrames[WF_2DC_ANMNUM] = {
    1, WF_2DC_ANMFRAME_LOOP, 8, 2, 4, 16, 16, 2, 4, 0,
};

static void (*const sWF_2DC_AnmInitFuncs[WF_2DC_ANMNUM])(WF_2DCWK *wk, u32 way) = {
    WF_2DC_WkPatAnmWayInit,       WF_2DC_WkPatAnmRotaInit,
    WF_2DC_WkPatAnmWalkInit,      WF_2DC_WkPatAnmTurnInit,
    WF_2DC_WkPatAnmRunInit,       WF_2DC_WkPatAnmWallWalkInit,
    WF_2DC_WkPatAnmSlowWalkInit,  WF_2DC_WkPatAnmHighWalk2Init,
    WF_2DC_WkPatAnmHighWalk4Init, NULL,
};

static void (*const sWF_2DC_AnmMainFuncs[WF_2DC_ANMNUM])(WF_2DCWK *wk) = {
    NULL,
    WF_2DC_WkPatAnmRotaMain,
    WF_2DC_WkPatAnmWalkMain,
    WF_2DC_WkPatAnmTurnMain,
    WF_2DC_WkPatAnmRunMain,
    WF_2DC_WkPatAnmWallWalkMain,
    WF_2DC_WkPatAnmSlowWalkMain,
    WF_2DC_WkPatAnmHighWalk2Main,
    WF_2DC_WkPatAnmHighWalk4Main,
    NULL,
};

WF_2DCSYS *WF_2DC_SysInit(ClActUnit *unit, void *unk1, u32 objNum, u32 heapId) {
    WF_2DCSYS *sys;
    int i;

    sys = GFL_HeapAllocate(heapId, sizeof(WF_2DCSYS), TRUE, "wifi_2dchar.c", 339);
    sys->unit = unit;
    sys->unk4 = unk1;
    sys->handles[0] = GFL_ArcSysCreateFileHandle(ARCID_WIFI_2DCHAR, heapId);
    sys->handles[1] = GFL_ArcSysCreateFileHandle(ARCID_WIFI_UNIONCHAR, heapId);

    for (i = 0; i < WF_2DC_CHARA_NUM; i++) {
        sys->res[i].chars = WF_2DC_RES_NONE;
    }
    sys->shadowRes.chars = WF_2DC_RES_NONE;
    sys->unionPltt = WF_2DC_RES_NONE;

    WF_2DC_AnmResLoad(sys, 0, heapId);

    sys->objNum = objNum;
    sys->objs = GFL_HeapAllocate(heapId, sizeof(WF_2DCWK) * sys->objNum, TRUE, "wifi_2dchar.c", 379);
    return sys;
}

void WF_2DC_SysExit(WF_2DCSYS *sys) {
    int i;

    for (i = 0; i < sys->objNum; i++) {
        if (WF_2DC_WkCleanCheck(&sys->objs[i]) == FALSE) {
            WF_2DC_WkDel(&sys->objs[i]);
        }
    }
    WF_2DC_AnmResDel(sys, 0);
    WF_2DC_AllResDel(sys);
    GFL_ArcToolFree(sys->handles[0]);
    GFL_ArcToolFree(sys->handles[1]);
    GFL_HeapFree(sys->objs);
    GFL_HeapFree(sys);
}

void WF_2DC_ResSet(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId) {
    u32 charNo;

    WF_2DC_CharNoCheck(charaId, moveType);
    charNo = WF_2DC_CharNoGet(charaId);
    WF_2DC_CharResLoad(sys, charNo, vramType, moveType, heapId, FALSE);
}

void WF_2DC_AllResDel(WF_2DCSYS *sys) {
    int i;

    if (sys->unionPltt != WF_2DC_RES_NONE) {
        WF_2DC_UnionResDel(sys);
    }
    for (i = 0; i < WF_2DC_CHARA_NUM; i++) {
        if (WF_2DC_CharResCheck(sys, i) == TRUE) {
            WF_2DC_CharResDel(sys, i);
        }
    }
}

void WF_2DC_UnionResSet(WF_2DCSYS *sys, u32 vramType, u32 moveType, u32 plttOffset, u32 heapId) {
    int i;

    WF_2DC_UnionPlttLoad(sys, vramType, plttOffset, heapId);
    WF_2DC_UnionAnmResLoad(sys, heapId);
    for (i = 0; i < WF_2DC_UNIONCHARA_NUM; i++) {
        WF_2DC_CharResLoad(sys, i, vramType, moveType, heapId, TRUE);
    }
}

void WF_2DC_UnionResDel(WF_2DCSYS *sys) {
    int i;

    WF_2DC_UnionPlttDel(sys);
    WF_2DC_UnionAnmResDel(sys);
    for (i = 0; i < WF_2DC_UNIONCHARA_NUM; i++) {
        WF_2DC_CharResDel(sys, i);
    }
}

void WF_2DC_ShadowResSet(WF_2DCSYS *sys, u32 vramType, u32 priority, u32 heapId) {
    WF_2DC_ShResLoad(sys, vramType, heapId);
    sys->shadowPriority = priority;
}

void WF_2DC_ShadowResDel(WF_2DCSYS *sys) {
    WF_2DC_ShResDel(sys);
}

WF_2DCWK *WF_2DC_WkAdd(WF_2DCSYS *sys, const WF_2DC_WKDATA *data, u32 charaId, u32 heapId) {
    WF_2DCWK *wk;
    u32 charNo;
    ClActorSetup setup;

    wk = WF_2DC_WkCleanGet(sys);
    charNo = WF_2DC_CharNoGet(charaId);

    setup.x = data->x;
    setup.y = data->y;
    setup.sequence = 0;
    setup.priority = data->priority;
    setup.bgPriority = data->bgPriority;
    wk->actor = func_0204c040(sys->unit, sys->res[charNo].chars, sys->res[charNo].palette,
                              sys->anmRes[charNo].cellAnims, &setup, 0, heapId);
    func_0204c468(wk->actor, (u8)data->bgPriority);

    if (sys->res[charNo].moveType == WF_2DC_MOVENORMAL) {
        func_0204c504(wk->actor, 6);
    } else {
        func_0204c504(wk->actor, 1);
    }

    wk->anmType = WF_2DC_ANMNONE;
    wk->frame = 0;
    wk->way = 1;
    wk->res = &sys->res[charNo];

    if (WF_2DC_ShResCheck(&sys->shadowRes)) {
        setup.x = data->x;
        setup.y = data->y;
        setup.sequence = 0;
        setup.priority = sys->shadowPriority;
        setup.bgPriority = 0;
        wk->shadow = func_0204c040(sys->unit, sys->shadowRes.chars, sys->shadowRes.palette, sys->shadowRes.cellAnims,
                                   &setup, 0, heapId);
        func_0204c468(wk->shadow, (u8)data->bgPriority);
    } else {
        wk->shadow = NULL;
    }
    return wk;
}

void WF_2DC_WkDel(WF_2DCWK *wk) {
    if (wk->shadow != NULL) {
        func_0204c108(wk->shadow);
    }
    func_0204c108(wk->actor);
    sys_memset(wk, 0, sizeof(WF_2DCWK));
}

void WF_2DC_WkMatrixSet(WF_2DCWK *wk, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c210(wk->actor, &pos);
    if (wk->shadow != NULL) {
        pos.x += WF_2DC_SHADOW_OFS_X;
        pos.y += WF_2DC_SHADOW_OFS_Y;
        func_0204c210(wk->shadow, &pos);
    }
}

void WF_2DC_WkDrawPriSet(WF_2DCWK *wk, u16 priority) {
    func_0204c438(wk->actor, priority);
}

u32 WF_2DC_WkDrawPriGet(const WF_2DCWK *wk) {
    return func_0204c45c(wk->actor);
}

void WF_2DC_WkDrawFlagSet(WF_2DCWK *wk, BOOL flag) {
    func_0204c124(wk->actor, flag);
    if (wk->shadow != NULL) {
        func_0204c124(wk->shadow, flag);
    }
}

void WF_2DC_WkShadowDrawFlagSet(WF_2DCWK *wk, BOOL flag) {
    if (wk->shadow != NULL) {
        func_0204c124(wk->shadow, flag);
    }
}

BOOL WF_2DC_WkDrawFlagGet(const WF_2DCWK *wk) {
    return func_0204c138(wk->actor);
}

u32 WF_2DC_WkMoveTypeGet(const WF_2DCWK *wk) {
    return wk->res->moveType;
}

u32 WF_2DC_WkAnmTypeGet(const WF_2DCWK *wk) {
    return wk->anmType;
}

void WF_2DC_WkAnmFrameSet(WF_2DCWK *wk, u16 frame) {
    func_0204c504(wk->actor, frame);
}

u16 WF_2DC_WkAnmFrameGet(const WF_2DCWK *wk) {
    return func_0204c510(wk->actor);
}

void WF_2DC_WkShadowMatrixSet(WF_2DCWK *wk, s16 x, s16 y) {
    ClActorPos pos;

    if (wk->shadow != NULL) {
        pos.x = x + WF_2DC_SHADOW_OFS_X;
        pos.y = y + WF_2DC_SHADOW_OFS_Y;
        func_0204c210(wk->shadow, &pos);
    }
}

void WF_2DC_WkPatAnmStart(WF_2DCWK *wk, u32 anmType, u32 way) {
    wk->frame = 0;
    wk->anmType = anmType;
    if (sWF_2DC_AnmInitFuncs[anmType] != NULL) {
        sWF_2DC_AnmInitFuncs[anmType](wk, way);
    }
}

void WF_2DC_WkPatAnmMain(WF_2DCWK *wk) {
    BOOL update = FALSE;

    if (sWF_2DC_AnmFrames[wk->anmType] == WF_2DC_ANMFRAME_LOOP) {
        update = TRUE;
    } else if (sWF_2DC_AnmFrames[wk->anmType] >= wk->frame + 1) {
        update = TRUE;
    }

    if (update == TRUE) {
        WF_2DC_WkPatAnmUpdate(wk);
        wk->frame++;
    }
}

static void WF_2DC_AnmResLoad(WF_2DCSYS *sys, u32 unused, u32 heapId) {
    int i;

    for (i = WF_2DC_CHARA_HERO; i < WF_2DC_CHARA_NUM; i++) {
        sys->anmRes[i].cellAnims = func_0204bde0(sys->handles[0], WF_2DC_ARC_ANM(i), WF_2DC_ARC_ANM(i) + 1, heapId);
    }
}

static void WF_2DC_AnmResDel(WF_2DCSYS *sys, u32 unused) {
    int i;

    for (i = WF_2DC_CHARA_HERO; i < WF_2DC_CHARA_NUM; i++) {
        func_0204be64(sys->anmRes[i].cellAnims);
        sys->anmRes[i].cellAnims = 0;
    }
}

static u32 WF_2DC_CharNoGet(u32 charaId) {
    return charaId;
}

static BOOL WF_2DC_CharNoCheck(u32 charaId, u32 moveType) {
    return TRUE;
}

static void WF_2DC_CharResLoad(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId, BOOL unionChara) {
    if (unionChara == FALSE) {
        WF_2DC_CharResLoadNormal(sys, charaId, vramType, moveType, heapId);
    } else {
        WF_2DC_CharResLoadUnion(sys, charaId, vramType, moveType, heapId);
    }
}

static void WF_2DC_CharResLoadNormal(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId) {
    sys->res[charaId].chars = func_0204b81c(sys->handles[0], WF_2DC_ARC_CHAR(charaId), FALSE, vramType, heapId);
    sys->res[charaId].palette = func_0204bbb8(sys->handles[0], WF_2DC_ARC_PLTT(charaId), vramType, 0x100, 0, 2, heapId);
    sys->res[charaId].vramType = vramType;
    sys->res[charaId].moveType = moveType;
    sys->res[charaId].unionChara = FALSE;
}

static void WF_2DC_CharResLoadUnion(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId) {
    sys->res[charaId].chars = func_0204b81c(sys->handles[1], WF_2DC_ARC_UNIONCHAR(charaId), FALSE, vramType, heapId);
    sys->res[charaId].palette = sys->unionPltt;
    sys->res[charaId].vramType = vramType;
    sys->res[charaId].moveType = moveType;
    sys->res[charaId].unionChara = TRUE;
}

static void WF_2DC_CharResDel(WF_2DCSYS *sys, u32 charaId) {
    if (sys->res[charaId].unionChara == FALSE) {
        WF_2DC_CharResDelNormal(sys, charaId);
    } else {
        WF_2DC_CharResDelUnion(sys, charaId);
    }
}

static void WF_2DC_CharResDelNormal(WF_2DCSYS *sys, u32 charaId) {
    func_0204b98c(sys->res[charaId].chars);
    func_0204bcd0(sys->res[charaId].palette);
    sys->res[charaId].chars = WF_2DC_RES_NONE;
    sys->res[charaId].palette = WF_2DC_RES_NONE;
}

static void WF_2DC_CharResDelUnion(WF_2DCSYS *sys, u32 charaId) {
    func_0204b98c(sys->res[charaId].chars);
    sys->res[charaId].chars = WF_2DC_RES_NONE;
}

static BOOL WF_2DC_CharResCheck(const WF_2DCSYS *sys, u32 charaId) {
    if (sys->res[charaId].chars != WF_2DC_RES_NONE) {
        return TRUE;
    }
    return FALSE;
}

static void WF_2DC_ShResLoad(WF_2DCSYS *sys, u32 vramType, u32 heapId) {
    if (WF_2DC_CharResCheck(sys, WF_2DC_CHARA_HERO) == TRUE) {
        sys->shadowChara = WF_2DC_CHARA_HERO;
    } else if (WF_2DC_CharResCheck(sys, WF_2DC_CHARA_HEROINE) == TRUE) {
        sys->shadowChara = WF_2DC_CHARA_HEROINE;
    }

    sys->shadowRes.cellAnims = func_0204bde0(sys->handles[0], WF_2DC_ARC_SHADOW_CELL, WF_2DC_ARC_SHADOW_ANM, heapId);
    sys->shadowRes.chars = func_0204b81c(sys->handles[0], WF_2DC_ARC_SHADOW_CHAR, FALSE, vramType, heapId);
    sys->shadowRes.palette = func_0204bbb8(sys->handles[0], WF_2DC_ARC_SHADOW_PLTT, vramType, 0x140, 0, 1, heapId);
}

static void WF_2DC_ShResDel(WF_2DCSYS *sys) {
    if (WF_2DC_ShResCheck(&sys->shadowRes) == TRUE) {
        func_0204b98c(sys->shadowRes.chars);
        sys->shadowRes.chars = WF_2DC_RES_NONE;
        func_0204bcd0(sys->shadowRes.palette);
        sys->shadowRes.palette = WF_2DC_RES_NONE;
        func_0204be64(sys->shadowRes.cellAnims);
        sys->shadowRes.cellAnims = WF_2DC_RES_NONE;
    }
}

static BOOL WF_2DC_ShResCheck(const WF_2DCSHRES *res) {
    if (res->chars != WF_2DC_RES_NONE) {
        return TRUE;
    }
    return FALSE;
}

static void WF_2DC_UnionPlttLoad(WF_2DCSYS *sys, u32 vramType, u32 plttOffset, u32 heapId) {
    sys->unionPltt =
        func_0204bbb8(sys->handles[1], WF_2DC_ARC_UNIONPLTT, vramType, plttOffset, 0, WF_2DC_UNIONPLTT_NUM, heapId);
}

static void WF_2DC_UnionPlttDel(WF_2DCSYS *sys) {
    func_0204bcd0(sys->unionPltt);
    sys->unionPltt = WF_2DC_RES_NONE;
}

static void WF_2DC_UnionAnmResLoad(WF_2DCSYS *sys, u32 heapId) {
    int i;

    for (i = 0; i < WF_2DC_UNIONCHARA_NUM; i++) {
        sys->anmRes[i].cellAnims =
            func_0204bde0(sys->handles[1], WF_2DC_ARC_UNIONCELL(i), WF_2DC_ARC_UNIONANM(i), heapId);
    }
}

static void WF_2DC_UnionAnmResDel(WF_2DCSYS *sys) {
    int i;

    for (i = 0; i < WF_2DC_UNIONCHARA_NUM; i++) {
        func_0204be64(sys->anmRes[i].cellAnims);
        sys->anmRes[i].cellAnims = WF_2DC_RES_NONE;
    }
}

static WF_2DCWK *WF_2DC_WkCleanGet(WF_2DCSYS *sys) {
    int i;

    for (i = 0; i < sys->objNum; i++) {
        if (WF_2DC_WkCleanCheck(&sys->objs[i]) == TRUE) {
            return &sys->objs[i];
        }
    }
    return NULL;
}

static void WF_2DC_WkPatAnmUpdate(WF_2DCWK *wk) {
    if (sWF_2DC_AnmMainFuncs[wk->anmType] != NULL) {
        sWF_2DC_AnmMainFuncs[wk->anmType](wk);
    }
}

static void WF_2DC_WkPatAnmWayInit(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way);
}

static void WF_2DC_WkPatAnmRotaInit(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, 4);
}

static void WF_2DC_WkPatAnmWalkInit(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way + 5);
}

// Finishes in the way the character faced
static void WF_2DC_WkPatAnmTurnInit(WF_2DCWK *wk, u32 way) {
    func_0204c4b8(wk->actor, wk->way + 5);
    wk->way = way;
}

static void WF_2DC_WkPatAnmRunInit(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way + 9);
}

static void WF_2DC_WkPatAnmWallWalkInit(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way + 5);
}

static void WF_2DC_WkPatAnmSlowWalkInit(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way + 5);
}

static void WF_2DC_WkPatAnmHighWalk2Init(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way + 5);
}

static void WF_2DC_WkPatAnmHighWalk4Init(WF_2DCWK *wk, u32 way) {
    wk->way = way;
    func_0204c4b8(wk->actor, way + 5);
}

static void WF_2DC_WkPatAnmRotaMain(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, 2 * FX32_ONE);
}

static void WF_2DC_WkPatAnmWalkMain(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, 2 * FX32_ONE);
}

// Turns to the new way after the first frame
static void WF_2DC_WkPatAnmTurnMain(WF_2DCWK *wk) {
    if (wk->frame == 0) {
        func_0204c4e0(wk->actor, 1);
    } else {
        func_0204c488(wk->actor, wk->way + 5);
    }
}

static void WF_2DC_WkPatAnmRunMain(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, 2 * FX32_ONE);
}

static void WF_2DC_WkPatAnmWallWalkMain(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, FX32_ONE);
}

static void WF_2DC_WkPatAnmSlowWalkMain(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, FX32_ONE);
}

static void WF_2DC_WkPatAnmHighWalk2Main(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, 8 * FX32_ONE);
}

static void WF_2DC_WkPatAnmHighWalk4Main(WF_2DCWK *wk) {
    func_0204c4e0(wk->actor, 6 * FX32_ONE);
}

static BOOL WF_2DC_WkCleanCheck(const WF_2DCWK *wk) {
    if (wk->actor == NULL) {
        return TRUE;
    }
    return FALSE;
}
