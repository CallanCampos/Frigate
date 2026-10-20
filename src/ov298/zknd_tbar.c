#include "types.h"
#include "app/zukan_detail.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nnsys/g2d.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"

// The Pokédex's copy of the touch bar: a bar at the bottom of a screen with icons that are touched or pressed with
// their keys. An icon plays its pushed animation, or flips on or off, and the bar reports it once the animation ends

// The bar's steps
enum {
    TBAR_SEQ_WAIT,
    TBAR_SEQ_TOUCHED,
    TBAR_SEQ_ANIM,
    TBAR_SEQ_DONE,
};

// The icons' resources
enum {
    OBJRES_PALETTE,
    OBJRES_CHARS,
    OBJRES_CELLANIMS,
    OBJRES_COUNT,
};

// The resources of the bar's icons and BG
typedef struct {
    u32 objRes[OBJRES_COUNT];
    u32 bg;
    u32 bgChars;
} ZkndTbarResource;

typedef struct ZkndTbarItem ZkndTbarItem;
typedef void (*ZkndTbarItemCallback)(ZkndTbarItem *item);

struct ZkndTbarItem {
    ClActor *actor;
    BOOL active;
    u32 anim;
    ZkndTbarItemCallback callback;
    u32 type;
    ZkndTbarIcon icon;
    BOOL pushed;
    BOOL triggered;
};

struct ZkndTbar {
    int trigger;
    u32 itemCount;
    int seq;
    BOOL active;
    // Set when an icon is touched, until the app unlocks the bar
    BOOL locked;
    ZkndTbarResource resource;
    ZkndTbarItem items[0];
};

// The animations, sound, key and type of the bar's own icons
typedef struct {
    u16 activeAnim;
    u16 inactiveAnim;
    u16 pushedAnim;
    u16 se;
    u32 key;
    u32 type;
} ZkndTbarIconData;

static void ZkndTbarResource_Load(ZkndTbarResource *resource, u8 bg, u32 engine, u32 palType, u32 mapping, u8 bgPalette,
                                  u8 objPalette, BOOL noBG, HeapID heapId);
static void ZkndTbarResource_Free(ZkndTbarResource *resource);
static u32 ZkndTbarResource_Get(const ZkndTbarResource *resource, u32 index);
static void ZkndTbar_LoadScreen(ArcTool *arc, u32 fileId, u32 bg, u32 charOffset, u8 srcX, u8 srcY, u8 srcWidth,
                                u8 srcHeight, u8 x, u8 y, u8 width, u8 height, u8 palette, BOOL compressed,
                                HeapID heapId);
static ZkndTbarItem *ZkndTbar_FindItem(ZkndTbar *tbar, int icon);
static const ZkndTbarItem *ZkndTbar_FindItemConst(ZkndTbar *tbar, int icon);
static void ZkndTbarItem_Init(ZkndTbarItem *item, ClActUnit *unit, const ZkndTbarResource *resource,
                              const ZkndTbarIcon *icon, u32 engine, HeapID heapId);
static void ZkndTbarItem_Free(ZkndTbarItem *item);
static BOOL ZkndTbarItem_CheckTouch(ZkndTbarItem *item);
static BOOL ZkndTbarItem_IsAnimEnd(const ZkndTbarItem *item);
static int ZkndTbarItem_GetIcon(const ZkndTbarItem *item);
static void ZkndTbarItem_SetVisible(ZkndTbarItem *item, BOOL visible);
static BOOL ZkndTbarItem_GetVisible(const ZkndTbarItem *item);
static void ZkndTbarItem_SetActive(ZkndTbarItem *item, BOOL active);
static void ZkndTbarItem_SetKey(ZkndTbarItem *item, u32 key);
static void ZkndTbarItem_SetFlip(ZkndTbarItem *item, BOOL flip);
static BOOL ZkndTbarItem_GetFlip(const ZkndTbarItem *item);
static void ZkndTbarItem_ClearPush(ZkndTbarItem *item);
static void ZkndTbarItem_PushCallback(ZkndTbarItem *item);
static void ZkndTbarItem_FlipCallback(ZkndTbarItem *item);

static const ZkndTbarIconData sZkndTbarIconData[ZKND_TBAR_ICON_CUSTOM] = {
    { 0, 14, 8, SEQ_SE_CLOSE1, PAD_BUTTON_X, ZKND_TBAR_TYPE_PUSH },
    { 1, 15, 9, SEQ_SE_CANCEL1, PAD_BUTTON_B, ZKND_TBAR_TYPE_PUSH },
    { 2, 16, 10, SEQ_SE_SELECT1, PAD_KEY_DOWN, ZKND_TBAR_TYPE_PUSH },
    { 3, 17, 11, SEQ_SE_SELECT1, PAD_KEY_UP, ZKND_TBAR_TYPE_PUSH },
    { 4, 18, 12, SEQ_SE_SELECT1, PAD_KEY_LEFT, ZKND_TBAR_TYPE_PUSH },
    { 5, 19, 13, SEQ_SE_SELECT1, PAD_KEY_RIGHT, ZKND_TBAR_TYPE_PUSH },
    { 6, 20, 7, SEQ_SE_SYS_07, PAD_BUTTON_Y, ZKND_TBAR_TYPE_FLIP },
};

ZkndTbar *ZkndTbar_Create(ZkndTbarParam *param, HeapID heapId) {
    u32 size = sizeof(ZkndTbar) + param->iconCount * sizeof(ZkndTbarItem);
    ZkndTbar *tbar = GFL_HeapAllocate(heapId, size, FALSE, "zknd_tbar.c", 264);
    u32 engine;
    u32 palType;
    u32 i;

    MI_CpuFill8(tbar, 0, size);
    tbar->itemCount = param->iconCount;
    tbar->seq = TBAR_SEQ_WAIT;
    tbar->active = TRUE;
    tbar->locked = FALSE;

    palType = PALTYPE_MAIN_BG;
    engine = CLACT_VRAM_SUB;
    if (param->bg >= 4) {
        palType = PALTYPE_SUB_BG;
    } else {
        engine = CLACT_VRAM_MAIN;
    }
    ZkndTbarResource_Load(&tbar->resource, param->bg, engine, palType, param->mapping, param->bgPalette,
                          param->objPalette, param->noBG, heapId);
    for (i = 0; i < tbar->itemCount; i++) {
        ZkndTbarItem_Init(&tbar->items[i], param->unit, &tbar->resource, &param->icons[i], engine, heapId);
    }
    return tbar;
}

void ZkndTbar_Free(ZkndTbar *tbar) {
    u32 i;

    for (i = 0; i < tbar->itemCount; i++) {
        ZkndTbarItem_Free(&tbar->items[i]);
    }
    ZkndTbarResource_Free(&tbar->resource);
    GFL_HeapFree(tbar);
}

void ZkndTbar_Main(ZkndTbar *tbar) {
    u32 i;

    switch (tbar->seq) {
    case TBAR_SEQ_WAIT:
        tbar->trigger = -1;
        if (tbar->active && !tbar->locked) {
            for (i = 0; i < tbar->itemCount; i++) {
                if (ZkndTbarItem_CheckTouch(&tbar->items[i])) {
                    tbar->trigger = i;
                    tbar->seq = TBAR_SEQ_TOUCHED;
                    tbar->locked = TRUE;
                    break;
                }
            }
        }
        break;
    case TBAR_SEQ_TOUCHED:
        tbar->seq = TBAR_SEQ_ANIM;
        // fallthrough
    case TBAR_SEQ_ANIM:
        if (ZkndTbarItem_IsAnimEnd(&tbar->items[tbar->trigger])) {
            tbar->seq = TBAR_SEQ_DONE;
        }
        break;
    case TBAR_SEQ_DONE:
        tbar->seq = TBAR_SEQ_WAIT;
        break;
    }

    for (i = 0; i < tbar->itemCount; i++) {
        ZkndTbarItem_ClearPush(&tbar->items[i]);
    }
}

int ZkndTbar_GetTrigger(ZkndTbar *tbar) {
    if (tbar->seq == TBAR_SEQ_DONE && tbar->trigger != -1) {
        return ZkndTbarItem_GetIcon(&tbar->items[tbar->trigger]);
    }
    return -1;
}

int ZkndTbar_GetTouch(ZkndTbar *tbar) {
    if (tbar->seq == TBAR_SEQ_TOUCHED && tbar->trigger != -1) {
        return ZkndTbarItem_GetIcon(&tbar->items[tbar->trigger]);
    }
    return -1;
}

void ZkndTbar_SetVisibleAll(ZkndTbar *tbar, BOOL visible) {
    u32 i;

    for (i = 0; i < tbar->itemCount; i++) {
        ZkndTbarItem_SetVisible(&tbar->items[i], visible);
    }
}

void ZkndTbar_SetActiveAll(ZkndTbar *tbar, BOOL active) {
    tbar->active = active;
}

BOOL ZkndTbar_GetActiveAll(ZkndTbar *tbar) {
    return tbar->active;
}

void ZkndTbar_Unlock(ZkndTbar *tbar) {
    tbar->locked = FALSE;
}

void ZkndTbar_SetActive(ZkndTbar *tbar, int icon, BOOL active) {
    ZkndTbarItem_SetActive(ZkndTbar_FindItem(tbar, icon), active);
}

void ZkndTbar_SetVisible(ZkndTbar *tbar, int icon, BOOL visible) {
    ZkndTbarItem_SetVisible(ZkndTbar_FindItem(tbar, icon), visible);
}

BOOL ZkndTbar_GetVisible(ZkndTbar *tbar, int icon) {
    return ZkndTbarItem_GetVisible(ZkndTbar_FindItemConst(tbar, icon));
}

void ZkndTbar_SetKey(ZkndTbar *tbar, int icon, u32 key) {
    ZkndTbarItem_SetKey(ZkndTbar_FindItem(tbar, icon), key);
}

void ZkndTbar_SetFlip(ZkndTbar *tbar, int icon, BOOL flip) {
    ZkndTbarItem_SetFlip(ZkndTbar_FindItem(tbar, icon), flip);
}

BOOL ZkndTbar_GetFlip(ZkndTbar *tbar, int icon) {
    return ZkndTbarItem_GetFlip(ZkndTbar_FindItemConst(tbar, icon));
}

ClActor *ZkndTbar_GetActor(ZkndTbar *tbar, int icon) {
    return ZkndTbar_FindItem(tbar, icon)->actor;
}

void ZkndTbar_SetPos(ZkndTbar *tbar, int icon, const ClActorPos *pos) {
    ZkndTbarItem *item = ZkndTbar_FindItem(tbar, icon);
    u32 surface;

    item->icon.pos = *pos;
    surface = CLACT_VRAM_SUB;
    if (tbar->resource.bg < 4) {
        surface = CLACT_VRAM_MAIN;
    }
    // The original truncates the surface to 16 bits
    ClActor_SetSurfacePos(item->actor, pos, surface);
}

void ZkndTbar_Push(ZkndTbar *tbar, int icon) {
    ZkndTbar_FindItem(tbar, icon)->pushed = TRUE;
}

BOOL ZkndTbar_IsTriggered(ZkndTbar *tbar, int icon) {
    return ZkndTbar_FindItem(tbar, icon)->triggered;
}

static ZkndTbarItem *ZkndTbar_FindItem(ZkndTbar *tbar, int icon) {
    u32 i;

    for (i = 0; i < tbar->itemCount; i++) {
        if (icon == ZkndTbarItem_GetIcon(&tbar->items[i])) {
            return &tbar->items[i];
        }
    }
    return NULL;
}

static const ZkndTbarItem *ZkndTbar_FindItemConst(ZkndTbar *tbar, int icon) {
    return ZkndTbar_FindItem(tbar, icon);
}

static void ZkndTbarResource_Load(ZkndTbarResource *resource, u8 bg, u32 engine, u32 palType, u32 mapping, u8 bgPalette,
                                  u8 objPalette, BOOL noBG, HeapID heapId) {
    ArcTool *arc;

    MI_CpuFill8(resource, 0, sizeof(ZkndTbarResource));
    resource->bg = bg;
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    if (!noBG) {
        GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), palType, bgPalette * 32, 32, heapId);
        resource->bgChars =
            GFL_BGSysLoadArcNCGRDynamic(arc, AppMenuCommon_GetBarBgCharFile(), resource->bg, 32 * 64, FALSE, heapId);
        ZkndTbar_LoadScreen(arc, AppMenuCommon_GetBarScreenFile(), resource->bg, CHAR_POS(resource->bgChars), 0, 21, 32,
                            24, 0, 21, 32, 3, bgPalette, FALSE, heapId);
    }
    resource->objRes[OBJRES_PALETTE] =
        ClActSys_LoadPlttRange(arc, AppMenuCommon_GetTouchBarPaletteFile(), engine, objPalette * 32, 0, 3, heapId);
    resource->objRes[OBJRES_CHARS] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, engine, heapId);
    resource->objRes[OBJRES_CELLANIMS] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(mapping),
                                                                AppMenuCommon_GetTouchBarAnimFile(mapping), heapId);
    GFL_ArcToolFree(arc);
}

static void ZkndTbarResource_Free(ZkndTbarResource *resource) {
    ClActSys_FreeCellAnims(resource->objRes[OBJRES_CELLANIMS]);
    ClActSys_FreeChars(resource->objRes[OBJRES_CHARS]);
    ClActSys_FreePltt(resource->objRes[OBJRES_PALETTE]);
    GFL_BGSysFreeCharMemory(resource->bg, CHAR_POS(resource->bgChars), CHAR_SIZE(resource->bgChars));
    MI_CpuFill8(resource, 0, sizeof(ZkndTbarResource));
}

static u32 ZkndTbarResource_Get(const ZkndTbarResource *resource, u32 index) {
    return resource->objRes[index];
}

// Loads a rectangle of a screen file into a BG, with its characters offset by charOffset
static void ZkndTbar_LoadScreen(ArcTool *arc, u32 fileId, u32 bg, u32 charOffset, u8 srcX, u8 srcY, u8 srcWidth,
                                u8 srcHeight, u8 x, u8 y, u8 width, u8 height, u8 palette, BOOL compressed,
                                HeapID heapId) {
    NNSG2dScreenData *screen;
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, HEAPID_TAIL(heapId));

    NNS_G2dGetUnpackedScreenData(file, &screen);
    if (charOffset != 0 && GFL_BGSysGetBGColorPaletteMode(bg) == GX_BG_COLORMODE_16) {
        int i;
        u16 *data = (u16 *)screen->rawData;

        for (i = 0; i < srcWidth * srcHeight; i++) {
            data[i] += charOffset;
        }
    }
    if (GFL_BGSysIsScrHeapExists(bg)) {
        GFL_BGSysLoadScrArea(bg, x, y, width, height, screen->rawData, srcX, srcY, srcWidth, srcHeight);
        GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, palette);
        GFL_BGSysLoadScr(bg);
    }
    GFL_HeapFree(file);
}

static void ZkndTbarItem_Init(ZkndTbarItem *item, ClActUnit *unit, const ZkndTbarResource *resource,
                              const ZkndTbarIcon *icon, u32 engine, HeapID heapId) {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cellAnims;

    MI_CpuFill8(item, 0, sizeof(ZkndTbarItem));
    item->icon = *icon;
    item->active = TRUE;
    item->pushed = FALSE;
    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    setup.bgPriority = GFL_BGSysGetBGPriority(resource->bg);
    setup.x = icon->pos.x;
    setup.y = icon->pos.y;
    if (icon->icon >= ZKND_TBAR_ICON_CUSTOM) {
        chars = icon->chars;
        palette = icon->palette;
        cellAnims = icon->cellAnims;
        item->type = ZKND_TBAR_TYPE_PUSH;
    } else {
        chars = ZkndTbarResource_Get(resource, OBJRES_CHARS);
        palette = ZkndTbarResource_Get(resource, OBJRES_PALETTE);
        cellAnims = ZkndTbarResource_Get(resource, OBJRES_CELLANIMS);
        item->icon.activeAnim = sZkndTbarIconData[icon->icon].activeAnim;
        item->icon.inactiveAnim = sZkndTbarIconData[icon->icon].inactiveAnim;
        item->icon.pushedAnim = sZkndTbarIconData[icon->icon].pushedAnim;
        item->icon.key = sZkndTbarIconData[icon->icon].key;
        item->icon.se = sZkndTbarIconData[icon->icon].se;
        item->type = sZkndTbarIconData[icon->icon].type;
    }
    item->anim = item->icon.activeAnim;
    setup.sequence = item->icon.activeAnim;
    item->actor = ClActor_Create(unit, chars, palette, cellAnims, &setup, engine, heapId);
    ClActor_SetAutoAnim(item->actor, TRUE);
    switch (item->type) {
    case ZKND_TBAR_TYPE_PUSH:
        item->callback = ZkndTbarItem_PushCallback;
        break;
    case ZKND_TBAR_TYPE_FLIP:
        item->callback = ZkndTbarItem_FlipCallback;
        break;
    }
}

static void ZkndTbarItem_Free(ZkndTbarItem *item) {
    ClActor_Delete(item->actor);
    MI_CpuFill8(item, 0, sizeof(ZkndTbarItem));
}

static BOOL ZkndTbarItem_CheckTouch(ZkndTbarItem *item) {
    BOOL touched = FALSE;
    BOOL enabled = ClActor_IsVisible(item->actor) & item->active;
    u32 x;
    u32 y;

    item->triggered = FALSE;
    if (enabled) {
        if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
            if (x - item->icon.pos.x <= item->icon.width && y - item->icon.pos.y <= 24) {
                GCTX_HIDSetUsingTouch(TRUE);
                touched = TRUE;
            }
        }
        if (item->icon.key != 0 && (GCTX_HIDGetPressedKeys() & item->icon.key)) {
            GCTX_HIDSetUsingTouch(FALSE);
            touched = TRUE;
        }
        if (item->pushed) {
            GCTX_HIDSetUsingTouch(FALSE);
            touched = TRUE;
        }
        if (touched) {
            item->triggered = TRUE;
            item->callback(item);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL ZkndTbarItem_IsAnimEnd(const ZkndTbarItem *item) {
    if (!ClActor_IsAnimActive(item->actor)) {
        return TRUE;
    }
    return FALSE;
}

static int ZkndTbarItem_GetIcon(const ZkndTbarItem *item) {
    return item->icon.icon;
}

static void ZkndTbarItem_SetVisible(ZkndTbarItem *item, BOOL visible) {
    ClActor_SetVisible(item->actor, visible);
}

static BOOL ZkndTbarItem_GetVisible(const ZkndTbarItem *item) {
    return ClActor_IsVisible(item->actor);
}

static void ZkndTbarItem_SetActive(ZkndTbarItem *item, BOOL active) {
    item->active = active;
    if (active) {
        if (item->anim == item->icon.pushedAnim) {
            item->anim = item->icon.activeAnim;
        }
        ClActor_SetSequence(item->actor, item->anim);
    } else {
        ClActor_SetSequence(item->actor, item->icon.inactiveAnim);
    }
}

static void ZkndTbarItem_SetKey(ZkndTbarItem *item, u32 key) {
    item->icon.key = key;
}

static void ZkndTbarItem_SetFlip(ZkndTbarItem *item, BOOL flip) {
    if (flip) {
        item->anim = item->icon.pushedAnim;
    } else {
        item->anim = item->icon.activeAnim;
    }
    ClActor_SetSequence(item->actor, item->anim);
}

static BOOL ZkndTbarItem_GetFlip(const ZkndTbarItem *item) {
    if (item->anim == item->icon.pushedAnim) {
        return TRUE;
    }
    return FALSE;
}

static void ZkndTbarItem_ClearPush(ZkndTbarItem *item) {
    item->pushed = FALSE;
}

static void ZkndTbarItem_PushCallback(ZkndTbarItem *item) {
    item->anim = item->icon.pushedAnim;
    ClActor_SetSequence(item->actor, item->icon.pushedAnim);
    if (item->icon.se != 0) {
        GFL_SndSEPlay(item->icon.se);
    }
}

static void ZkndTbarItem_FlipCallback(ZkndTbarItem *item) {
    ZkndTbarItem_SetFlip(item, ZkndTbarItem_GetFlip(item) ? FALSE : TRUE);
    if (item->icon.se != 0) {
        GFL_SndSEPlay(item->icon.se);
    }
}
