// The Battle Recorder's sidebars: the twelve strips along the edges of the two screens, which slide in shaking,
// sway in place and slide back out between its screens. The name is the ROM's string, which GFL_HeapAllocate is
// given.

#include "types.h"
#include "app/battle_recorder/br_sidebar.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/math_util.h"
#include "gfl/std.h"
#include "nitro/fx.h"

#define BR_SIDEBAR_MAX 12
// The scale the sidebars shrink to, and the step they shrink by
#define BR_SIDEBAR_SCALE_END 0x19a

// Where a sidebar starts and how it moves
typedef struct {
    s16 x;
    s16 y;
    s16 scale;
    // The steps of a slide
    s16 sync;
    s16 end_x;
    s16 end_y;
    // The angle the sway starts at, and how far it turns each step, in degrees
    s16 theta;
    s16 theta_add;
    u32 display;
    // Which way it shakes and sways
    BOOL dir;
} BrSidebarData;

typedef struct BrSidebarWork BrSidebarWork;

typedef void (*BrSidebarMoveFunc)(BrSidebarWork *p_wk);

struct BrSidebarWork {
    BOOL is_use;
    ClActor *clwk;
    fx32 x;
    fx32 y;
    fx32 now_x;
    fx32 scale;
    BrSidebarMoveFunc move_func;
    u32 seq;
    s32 theta;
    s32 cnt;
    s32 wait;
    s32 dx;
    fx32 dscale;
    BrSidebarData data;
};

struct BrSidebar {
    BrSidebarWork wk[BR_SIDEBAR_MAX];
};

// How a sidebar moves
enum {
    BR_SIDEBAR_MOVE_NONE,
    BR_SIDEBAR_MOVE_OPEN,
    BR_SIDEBAR_MOVE_BOUND,
    BR_SIDEBAR_MOVE_CLOSE,
};

static void BrSidebarWork_Init(BrSidebarWork *p_wk, ClActUnit *unit, BrRes *res, const BrSidebarData *cp_data,
                               HeapID heapId);
static void BrSidebarWork_Exit(BrSidebarWork *p_wk);
static void BrSidebarWork_Main(BrSidebarWork *p_wk);
static BOOL BrSidebarWork_IsUse(const BrSidebarWork *cp_wk);
static void BrSidebarWork_SetVisible(BrSidebarWork *p_wk, BOOL isVisible);
static void BrSidebarWork_StartMove(BrSidebarWork *p_wk, u32 type);
static void BrSidebarWork_SetEndPos(BrSidebarWork *p_wk);
static void BrSidebarWork_Move_Open(BrSidebarWork *p_wk);
static void BrSidebarWork_Move_Bound(BrSidebarWork *p_wk);
static void BrSidebarWork_Move_Close(BrSidebarWork *p_wk);

static const BrSidebarData sc_sidebar_data[BR_SIDEBAR_MAX] = {
    { 82, 96, 0x1666, 6, 6, 96, 90, 4, CLACT_SURFACE_MAIN, FALSE },
    { 178, 96, 0x1ccd, 4, 5, 96, 135, -4, CLACT_SURFACE_MAIN, FALSE },
    { 118, 96, 0x2000, 14, 10, 96, 270, 4, CLACT_SURFACE_MAIN, FALSE },
    { 42, 96, 0x199a, 8, 246, 96, 90, 4, CLACT_SURFACE_MAIN, TRUE },
    { 150, 96, 0x1666, 4, 244, 96, 135, -4, CLACT_SURFACE_MAIN, TRUE },
    { 210, 96, 0x199a, 14, 249, 96, 270, 4, CLACT_SURFACE_MAIN, TRUE },
    { 82, 96, 0x1666, 6, 6, 96, 90, 4, CLACT_SURFACE_SUB, FALSE },
    { 178, 96, 0x1ccd, 4, 5, 96, 135, -4, CLACT_SURFACE_SUB, FALSE },
    { 118, 96, 0x2000, 14, 10, 96, 270, 4, CLACT_SURFACE_SUB, FALSE },
    { 42, 96, 0x199a, 8, 246, 96, 90, 4, CLACT_SURFACE_SUB, TRUE },
    { 150, 96, 0x1666, 4, 244, 96, 135, -4, CLACT_SURFACE_SUB, TRUE },
    { 210, 96, 0x199a, 14, 249, 96, 270, 4, CLACT_SURFACE_SUB, TRUE },
};

BrSidebar *BrSidebar_Init(ClActUnit *unit, BrFade *fade, BrRes *res, HeapID heapId) {
    BrSidebar *p_wk;
    int i;

    p_wk = GFL_HeapAllocate(heapId, sizeof(BrSidebar), FALSE, "br_sidebar.c", 306);
    MI_CpuFill8(p_wk, 0, sizeof(BrSidebar));
    BrRes_LoadOBJ(res, BR_RES_OBJ_SIDEBAR_M, heapId);
    BrRes_LoadOBJ(res, BR_RES_OBJ_SIDEBAR_S, heapId);
    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        BrSidebarWork_Init(&p_wk->wk[i], unit, res, &sc_sidebar_data[i], heapId);
    }
    return p_wk;
}

void BrSidebar_Exit(BrSidebar *p_wk, BrRes *res) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (BrSidebarWork_IsUse(&p_wk->wk[i])) {
            BrSidebarWork_Exit(&p_wk->wk[i]);
        }
    }
    BrRes_UnloadOBJ(res, BR_RES_OBJ_SIDEBAR_M);
    BrRes_UnloadOBJ(res, BR_RES_OBJ_SIDEBAR_S);
    GFL_HeapFree(p_wk);
}

void BrSidebar_Main(BrSidebar *p_wk) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (BrSidebarWork_IsUse(&p_wk->wk[i])) {
            BrSidebarWork_Main(&p_wk->wk[i]);
        }
    }
}

void BrSidebar_StartOpen(BrSidebar *p_wk) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (BrSidebarWork_IsUse(&p_wk->wk[i])) {
            BrSidebarWork_StartMove(&p_wk->wk[i], BR_SIDEBAR_MOVE_OPEN);
        }
    }
}

void BrSidebar_StartBound(BrSidebar *p_wk) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (BrSidebarWork_IsUse(&p_wk->wk[i])) {
            BrSidebarWork_StartMove(&p_wk->wk[i], BR_SIDEBAR_MOVE_BOUND);
        }
    }
}

void BrSidebar_StartClose(BrSidebar *p_wk) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (BrSidebarWork_IsUse(&p_wk->wk[i])) {
            BrSidebarWork_StartMove(&p_wk->wk[i], BR_SIDEBAR_MOVE_CLOSE);
        }
    }
}

void BrSidebar_SetEndPos(BrSidebar *p_wk) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (BrSidebarWork_IsUse(&p_wk->wk[i])) {
            BrSidebarWork_SetEndPos(&p_wk->wk[i]);
        }
    }
}

void BrSidebar_SetVisible(BrSidebar *p_wk, u32 display, BOOL isVisible) {
    int i;

    for (i = 0; i < BR_SIDEBAR_MAX; i++) {
        if (display == sc_sidebar_data[i].display) {
            BrSidebarWork_SetVisible(&p_wk->wk[i], isVisible);
        }
    }
}

static void BrSidebarWork_Init(BrSidebarWork *p_wk, ClActUnit *unit, BrRes *res, const BrSidebarData *cp_data,
                               HeapID heapId) {
    ClActorSetup setup;
    BrResObjData obj;
    BOOL ret;
    u32 objID;

    MI_CpuFill8(p_wk, 0, sizeof(BrSidebarWork));
    p_wk->data = *cp_data;
    objID = BR_RES_OBJ_SIDEBAR_M;
    if (cp_data->display) {
        objID = BR_RES_OBJ_SIDEBAR_S;
    }
    ret = BrRes_GetObjData(res, objID, &obj);
    GFL_ASSERT(ret);

    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    setup.x = cp_data->x;
    setup.y = cp_data->y;
    setup.bgPriority = 1;
    p_wk->x = FX32_CONST(setup.x);
    p_wk->y = FX32_CONST(setup.y);
    p_wk->scale = cp_data->scale;
    p_wk->clwk = ClActor_Create(unit, obj.chr, obj.plt, obj.cell, &setup, p_wk->data.display, heapId);
    ClActor_SetAffineMode(p_wk->clwk, 2);
    ClActor_SetScaleAxis(p_wk->clwk, p_wk->scale, 0);
    ClActor_SetObjMode(p_wk->clwk, 0);
    ClActor_SetVisible(p_wk->clwk, FALSE);
    p_wk->is_use = TRUE;
}

static void BrSidebarWork_Exit(BrSidebarWork *p_wk) {
    ClActor_Delete(p_wk->clwk);
    MI_CpuFill8(p_wk, 0, sizeof(BrSidebarWork));
}

static void BrSidebarWork_Main(BrSidebarWork *p_wk) {
    if (p_wk->move_func != NULL) {
        p_wk->move_func(p_wk);
    }
}

static BOOL BrSidebarWork_IsUse(const BrSidebarWork *cp_wk) {
    return cp_wk->is_use;
}

static void BrSidebarWork_SetVisible(BrSidebarWork *p_wk, BOOL isVisible) {
    ClActor_SetVisible(p_wk->clwk, isVisible);
}

static void BrSidebarWork_StartMove(BrSidebarWork *p_wk, u32 type) {
    switch (type) {
    case BR_SIDEBAR_MOVE_NONE:
        p_wk->move_func = NULL;
        break;
    case BR_SIDEBAR_MOVE_OPEN:
        p_wk->move_func = BrSidebarWork_Move_Open;
        break;
    case BR_SIDEBAR_MOVE_BOUND:
        p_wk->move_func = BrSidebarWork_Move_Bound;
        break;
    case BR_SIDEBAR_MOVE_CLOSE:
        p_wk->move_func = BrSidebarWork_Move_Close;
        break;
    }
    p_wk->seq = 0;
}

static void BrSidebarWork_SetEndPos(BrSidebarWork *p_wk) {
    p_wk->scale = BR_SIDEBAR_SCALE_END;
    ClActor_SetSurfacePosAxis(p_wk->clwk, p_wk->data.end_x, p_wk->data.display, 0);
    ClActor_SetScaleAxis(p_wk->clwk, p_wk->scale, 0);
}

// The sign of a sidebar's sway: 1 when its data's dir is set, otherwise -1
static inline int BrSidebar_GetDir(BOOL dir) {
    int ret = -1;

    if (dir) {
        ret = 1;
    }
    return ret;
}

// Shakes in place, then slides to its end and shrinks
static void BrSidebarWork_Move_Open(BrSidebarWork *p_wk) {
    p_wk->wait = (p_wk->wait + 1) % 2;
    if (p_wk->wait != 0) {
        return;
    }

    switch (p_wk->seq) {
    case 0:
        ClActor_SetVisible(p_wk->clwk, TRUE);
        p_wk->cnt = 0;
        p_wk->scale = p_wk->data.scale;
        p_wk->seq = 1;
        break;
    case 1:
        if (p_wk->cnt == 3) {
            p_wk->seq = 2;
            p_wk->now_x = FX32_CONST(ClActor_GetSurfacePosAxis(p_wk->clwk, p_wk->data.display, 0));
            p_wk->cnt = 0;
        } else {
            p_wk->cnt++;
        }
        break;
    case 2: {
        s8 dir = BrSidebar_GetDir(p_wk->data.dir);

        if (p_wk->cnt == 4) {
            p_wk->seq = 3;
            p_wk->cnt = 0;
        } else {
            u16 rot = 0xffff * p_wk->theta / 360;

            p_wk->now_x += -dir * GFL_MathSinDegWrap(rot);
            p_wk->theta += 32;
            p_wk->theta %= 360;
            ClActor_SetSurfacePosAxis(p_wk->clwk, p_wk->now_x >> FX32_SHIFT, p_wk->data.display, 0);
            p_wk->cnt++;
        }
        break;
    }
    case 3: {
        s16 x = ClActor_GetSurfacePosAxis(p_wk->clwk, p_wk->data.display, 0);

        if (p_wk->cnt == 0) {
            p_wk->dx = p_wk->data.end_x - x;
            p_wk->dx /= p_wk->data.sync;
            p_wk->cnt++;
        } else if (p_wk->cnt == p_wk->data.sync + 1) {
            x = p_wk->data.end_x;
            p_wk->seq = 4;
        } else {
            x += (s16)p_wk->dx;
            p_wk->cnt++;
        }
        ClActor_SetSurfacePosAxis(p_wk->clwk, x, p_wk->data.display, 0);
        break;
    }
    case 4:
        BrSidebarWork_StartMove(p_wk, BR_SIDEBAR_MOVE_BOUND);
        break;
    }

    if (p_wk->seq == 3 && p_wk->scale > BR_SIDEBAR_SCALE_END) {
        p_wk->scale -= BR_SIDEBAR_SCALE_END;
        ClActor_SetScaleAxis(p_wk->clwk, p_wk->scale, 0);
    }
}

// Sways about its end
static void BrSidebarWork_Move_Bound(BrSidebarWork *p_wk) {
    p_wk->wait = (p_wk->wait + 1) % 2;
    if (p_wk->wait != 0) {
        return;
    }

    switch (p_wk->seq) {
    case 0:
        ClActor_SetVisible(p_wk->clwk, TRUE);
        ClActor_SetSurfacePosAxis(p_wk->clwk, p_wk->data.end_x, p_wk->data.display, 0);
        p_wk->now_x = FX32_CONST(p_wk->data.end_x);
        p_wk->theta = p_wk->data.theta;
        p_wk->seq = 1;
        break;
    case 1: {
        s8 dir = BrSidebar_GetDir(p_wk->data.dir);
        u16 rot;
        fx32 x;

        rot = 0xffff * p_wk->theta / 360;
        x = p_wk->now_x + -dir * (GFL_MathSinDegWrap(rot) * 3);
        p_wk->theta += p_wk->data.theta_add;
        p_wk->theta %= 360;
        ClActor_SetSurfacePosAxis(p_wk->clwk, x >> FX32_SHIFT, p_wk->data.display, 0);
        break;
    }
    }

    if (p_wk->scale > BR_SIDEBAR_SCALE_END) {
        p_wk->scale -= BR_SIDEBAR_SCALE_END;
        ClActor_SetScaleAxis(p_wk->clwk, p_wk->scale, 0);
    }
}

// Slides back to its start, growing back
static void BrSidebarWork_Move_Close(BrSidebarWork *p_wk) {
    p_wk->wait = (p_wk->wait + 1) % 2;
    if (p_wk->wait != 0) {
        return;
    }

    switch (p_wk->seq) {
    case 0:
        ClActor_SetVisible(p_wk->clwk, TRUE);
        p_wk->cnt = 0;
        p_wk->seq = 1;
        break;
    case 1: {
        s16 x = ClActor_GetSurfacePosAxis(p_wk->clwk, p_wk->data.display, 0);
        fx32 scale = ClActor_GetScaleAxis(p_wk->clwk, 0);

        if (p_wk->cnt == 0) {
            p_wk->dx = p_wk->data.x - x;
            p_wk->dx /= p_wk->data.sync;
            p_wk->cnt++;
            p_wk->dscale = p_wk->data.scale - scale;
            p_wk->dscale /= p_wk->data.sync;
        } else if (p_wk->cnt == p_wk->data.sync + 1) {
            x = p_wk->data.x;
            scale = p_wk->data.scale;
            p_wk->seq = 2;
        } else {
            x += (s16)p_wk->dx;
            scale += p_wk->dscale;
            p_wk->cnt++;
        }
        ClActor_SetSurfacePosAxis(p_wk->clwk, x, p_wk->data.display, 0);
        ClActor_SetScaleAxis(p_wk->clwk, scale, 0);
        break;
    }
    case 2:
        BrSidebarWork_StartMove(p_wk, BR_SIDEBAR_MOVE_NONE);
        break;
    }
}
