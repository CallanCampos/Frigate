// The ROM names this file zknd_tbar.c.
#include "types.h"
#include "app/pokedex_detail.h"
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
} PokedexDetailTbarResource;

typedef struct PokedexDetailTbarItem PokedexDetailTbarItem;
typedef void (*PokedexDetailTbarItemCallback)(PokedexDetailTbarItem *item);

struct PokedexDetailTbarItem {
    ClActor *actor;
    BOOL active;
    u32 anim;
    PokedexDetailTbarItemCallback callback;
    u32 type;
    PokedexDetailTbarIcon icon;
    BOOL pushed;
    BOOL triggered;
};

struct PokedexDetailTbar {
    int trigger;
    u32 itemCount;
    int seq;
    BOOL active;
    // Set when an icon is touched, until the app unlocks the bar
    BOOL locked;
    PokedexDetailTbarResource resource;
    PokedexDetailTbarItem items[0];
};

// The animations, sound, key and type of the bar's own icons
typedef struct {
    u16 activeAnim;
    u16 inactiveAnim;
    u16 pushedAnim;
    u16 se;
    u32 key;
    u32 type;
} PokedexDetailTbarIconData;

static void PokedexDetailTbarResource_Load(PokedexDetailTbarResource *resource, u8 bg, u32 engine, u32 palType,
                                           u32 mapping, u8 bgPalette, u8 objPalette, BOOL noBG, HeapID heapId);
static void PokedexDetailTbarResource_Free(PokedexDetailTbarResource *resource);
static u32 PokedexDetailTbarResource_Get(const PokedexDetailTbarResource *resource, u32 index);
static void PokedexDetailTbar_LoadScreen(ArcTool *arc, u32 fileId, u32 bg, u32 charOffset, u8 srcX, u8 srcY,
                                         u8 srcWidth, u8 srcHeight, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                         BOOL compressed, HeapID heapId);
static PokedexDetailTbarItem *PokedexDetailTbar_FindItem(PokedexDetailTbar *tbar, int icon);
static const PokedexDetailTbarItem *PokedexDetailTbar_FindItemConst(PokedexDetailTbar *tbar, int icon);
static void PokedexDetailTbarItem_Init(PokedexDetailTbarItem *item, ClActUnit *unit,
                                       const PokedexDetailTbarResource *resource, const PokedexDetailTbarIcon *icon,
                                       u32 engine, HeapID heapId);
static void PokedexDetailTbarItem_Free(PokedexDetailTbarItem *item);
static BOOL PokedexDetailTbarItem_CheckTouch(PokedexDetailTbarItem *item);
static BOOL PokedexDetailTbarItem_IsAnimEnd(const PokedexDetailTbarItem *item);
static int PokedexDetailTbarItem_GetIcon(const PokedexDetailTbarItem *item);
static void PokedexDetailTbarItem_SetVisible(PokedexDetailTbarItem *item, BOOL visible);
static BOOL PokedexDetailTbarItem_GetVisible(const PokedexDetailTbarItem *item);
static void PokedexDetailTbarItem_SetActive(PokedexDetailTbarItem *item, BOOL active);
static void PokedexDetailTbarItem_SetKey(PokedexDetailTbarItem *item, u32 key);
static void PokedexDetailTbarItem_SetFlip(PokedexDetailTbarItem *item, BOOL flip);
static BOOL PokedexDetailTbarItem_GetFlip(const PokedexDetailTbarItem *item);
static void PokedexDetailTbarItem_ClearPush(PokedexDetailTbarItem *item);
static void PokedexDetailTbarItem_PushCallback(PokedexDetailTbarItem *item);
static void PokedexDetailTbarItem_FlipCallback(PokedexDetailTbarItem *item);

static const PokedexDetailTbarIconData sPokedexDetailTbarIconData[POKEDEX_DETAIL_TBAR_ICON_CUSTOM] = {
    { 0, 14, 8, SEQ_SE_CLOSE1, PAD_BUTTON_X, POKEDEX_DETAIL_TBAR_TYPE_PUSH },
    { 1, 15, 9, SEQ_SE_CANCEL1, PAD_BUTTON_B, POKEDEX_DETAIL_TBAR_TYPE_PUSH },
    { 2, 16, 10, SEQ_SE_SELECT1, PAD_KEY_DOWN, POKEDEX_DETAIL_TBAR_TYPE_PUSH },
    { 3, 17, 11, SEQ_SE_SELECT1, PAD_KEY_UP, POKEDEX_DETAIL_TBAR_TYPE_PUSH },
    { 4, 18, 12, SEQ_SE_SELECT1, PAD_KEY_LEFT, POKEDEX_DETAIL_TBAR_TYPE_PUSH },
    { 5, 19, 13, SEQ_SE_SELECT1, PAD_KEY_RIGHT, POKEDEX_DETAIL_TBAR_TYPE_PUSH },
    { 6, 20, 7, SEQ_SE_SYS_07, PAD_BUTTON_Y, POKEDEX_DETAIL_TBAR_TYPE_FLIP },
};

PokedexDetailTbar *PokedexDetailTbar_Create(PokedexDetailTbarParam *param, HeapID heapId) {
    u32 size = sizeof(PokedexDetailTbar) + param->iconCount * sizeof(PokedexDetailTbarItem);
    PokedexDetailTbar *tbar = GFL_HeapAllocate(heapId, size, FALSE, "zknd_tbar.c", 264);
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
    PokedexDetailTbarResource_Load(&tbar->resource, param->bg, engine, palType, param->mapping, param->bgPalette,
                                   param->objPalette, param->noBG, heapId);
    for (i = 0; i < tbar->itemCount; i++) {
        PokedexDetailTbarItem_Init(&tbar->items[i], param->unit, &tbar->resource, &param->icons[i], engine, heapId);
    }
    return tbar;
}

void PokedexDetailTbar_Free(PokedexDetailTbar *tbar) {
    u32 i;

    for (i = 0; i < tbar->itemCount; i++) {
        PokedexDetailTbarItem_Free(&tbar->items[i]);
    }
    PokedexDetailTbarResource_Free(&tbar->resource);
    GFL_HeapFree(tbar);
}

void PokedexDetailTbar_Main(PokedexDetailTbar *tbar) {
    u32 i;

    switch (tbar->seq) {
    case TBAR_SEQ_WAIT:
        tbar->trigger = -1;
        if (tbar->active && !tbar->locked) {
            for (i = 0; i < tbar->itemCount; i++) {
                if (PokedexDetailTbarItem_CheckTouch(&tbar->items[i])) {
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
        if (PokedexDetailTbarItem_IsAnimEnd(&tbar->items[tbar->trigger])) {
            tbar->seq = TBAR_SEQ_DONE;
        }
        break;
    case TBAR_SEQ_DONE:
        tbar->seq = TBAR_SEQ_WAIT;
        break;
    }

    for (i = 0; i < tbar->itemCount; i++) {
        PokedexDetailTbarItem_ClearPush(&tbar->items[i]);
    }
}

int PokedexDetailTbar_GetTrigger(PokedexDetailTbar *tbar) {
    if (tbar->seq == TBAR_SEQ_DONE && tbar->trigger != -1) {
        return PokedexDetailTbarItem_GetIcon(&tbar->items[tbar->trigger]);
    }
    return -1;
}

int PokedexDetailTbar_GetTouch(PokedexDetailTbar *tbar) {
    if (tbar->seq == TBAR_SEQ_TOUCHED && tbar->trigger != -1) {
        return PokedexDetailTbarItem_GetIcon(&tbar->items[tbar->trigger]);
    }
    return -1;
}

void PokedexDetailTbar_SetVisibleAll(PokedexDetailTbar *tbar, BOOL visible) {
    u32 i;

    for (i = 0; i < tbar->itemCount; i++) {
        PokedexDetailTbarItem_SetVisible(&tbar->items[i], visible);
    }
}

void PokedexDetailTbar_SetActiveAll(PokedexDetailTbar *tbar, BOOL active) {
    tbar->active = active;
}

BOOL PokedexDetailTbar_GetActiveAll(PokedexDetailTbar *tbar) {
    return tbar->active;
}

void PokedexDetailTbar_Unlock(PokedexDetailTbar *tbar) {
    tbar->locked = FALSE;
}

void PokedexDetailTbar_SetActive(PokedexDetailTbar *tbar, int icon, BOOL active) {
    PokedexDetailTbarItem_SetActive(PokedexDetailTbar_FindItem(tbar, icon), active);
}

void PokedexDetailTbar_SetVisible(PokedexDetailTbar *tbar, int icon, BOOL visible) {
    PokedexDetailTbarItem_SetVisible(PokedexDetailTbar_FindItem(tbar, icon), visible);
}

BOOL PokedexDetailTbar_GetVisible(PokedexDetailTbar *tbar, int icon) {
    return PokedexDetailTbarItem_GetVisible(PokedexDetailTbar_FindItemConst(tbar, icon));
}

void PokedexDetailTbar_SetKey(PokedexDetailTbar *tbar, int icon, u32 key) {
    PokedexDetailTbarItem_SetKey(PokedexDetailTbar_FindItem(tbar, icon), key);
}

void PokedexDetailTbar_SetFlip(PokedexDetailTbar *tbar, int icon, BOOL flip) {
    PokedexDetailTbarItem_SetFlip(PokedexDetailTbar_FindItem(tbar, icon), flip);
}

BOOL PokedexDetailTbar_GetFlip(PokedexDetailTbar *tbar, int icon) {
    return PokedexDetailTbarItem_GetFlip(PokedexDetailTbar_FindItemConst(tbar, icon));
}

ClActor *PokedexDetailTbar_GetActor(PokedexDetailTbar *tbar, int icon) {
    return PokedexDetailTbar_FindItem(tbar, icon)->actor;
}

void PokedexDetailTbar_SetPos(PokedexDetailTbar *tbar, int icon, const ClActorPos *pos) {
    PokedexDetailTbarItem *item = PokedexDetailTbar_FindItem(tbar, icon);
    u32 surface;

    item->icon.pos = *pos;
    surface = CLACT_VRAM_SUB;
    if (tbar->resource.bg < 4) {
        surface = CLACT_VRAM_MAIN;
    }
    // The original truncates the surface to 16 bits
    ClActor_SetSurfacePos(item->actor, pos, surface);
}

void PokedexDetailTbar_Push(PokedexDetailTbar *tbar, int icon) {
    PokedexDetailTbar_FindItem(tbar, icon)->pushed = TRUE;
}

BOOL PokedexDetailTbar_IsTriggered(PokedexDetailTbar *tbar, int icon) {
    return PokedexDetailTbar_FindItem(tbar, icon)->triggered;
}

static PokedexDetailTbarItem *PokedexDetailTbar_FindItem(PokedexDetailTbar *tbar, int icon) {
    u32 i;

    for (i = 0; i < tbar->itemCount; i++) {
        if (icon == PokedexDetailTbarItem_GetIcon(&tbar->items[i])) {
            return &tbar->items[i];
        }
    }
    return NULL;
}

static const PokedexDetailTbarItem *PokedexDetailTbar_FindItemConst(PokedexDetailTbar *tbar, int icon) {
    return PokedexDetailTbar_FindItem(tbar, icon);
}

static void PokedexDetailTbarResource_Load(PokedexDetailTbarResource *resource, u8 bg, u32 engine, u32 palType,
                                           u32 mapping, u8 bgPalette, u8 objPalette, BOOL noBG, HeapID heapId) {
    ArcTool *arc;

    MI_CpuFill8(resource, 0, sizeof(PokedexDetailTbarResource));
    resource->bg = bg;
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    if (!noBG) {
        GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), palType, bgPalette * 32, 32, heapId);
        resource->bgChars =
            GFL_BGSysLoadArcNCGRDynamic(arc, AppMenuCommon_GetBarBgCharFile(), resource->bg, 32 * 64, FALSE, heapId);
        PokedexDetailTbar_LoadScreen(arc, AppMenuCommon_GetBarScreenFile(), resource->bg, CHAR_POS(resource->bgChars),
                                     0, 21, 32, 24, 0, 21, 32, 3, bgPalette, FALSE, heapId);
    }
    resource->objRes[OBJRES_PALETTE] =
        ClActSys_LoadPlttRange(arc, AppMenuCommon_GetTouchBarPaletteFile(), engine, objPalette * 32, 0, 3, heapId);
    resource->objRes[OBJRES_CHARS] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, engine, heapId);
    resource->objRes[OBJRES_CELLANIMS] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(mapping),
                                                                AppMenuCommon_GetTouchBarAnimFile(mapping), heapId);
    GFL_ArcToolFree(arc);
}

static void PokedexDetailTbarResource_Free(PokedexDetailTbarResource *resource) {
    ClActSys_FreeCellAnims(resource->objRes[OBJRES_CELLANIMS]);
    ClActSys_FreeChars(resource->objRes[OBJRES_CHARS]);
    ClActSys_FreePltt(resource->objRes[OBJRES_PALETTE]);
    GFL_BGSysFreeCharMemory(resource->bg, CHAR_POS(resource->bgChars), CHAR_SIZE(resource->bgChars));
    MI_CpuFill8(resource, 0, sizeof(PokedexDetailTbarResource));
}

static u32 PokedexDetailTbarResource_Get(const PokedexDetailTbarResource *resource, u32 index) {
    return resource->objRes[index];
}

// Loads a rectangle of a screen file into a BG, with its characters offset by charOffset
static void PokedexDetailTbar_LoadScreen(ArcTool *arc, u32 fileId, u32 bg, u32 charOffset, u8 srcX, u8 srcY,
                                         u8 srcWidth, u8 srcHeight, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                         BOOL compressed, HeapID heapId) {
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

static void PokedexDetailTbarItem_Init(PokedexDetailTbarItem *item, ClActUnit *unit,
                                       const PokedexDetailTbarResource *resource, const PokedexDetailTbarIcon *icon,
                                       u32 engine, HeapID heapId) {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cellAnims;

    MI_CpuFill8(item, 0, sizeof(PokedexDetailTbarItem));
    item->icon = *icon;
    item->active = TRUE;
    item->pushed = FALSE;
    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    setup.bgPriority = GFL_BGSysGetBGPriority(resource->bg);
    setup.x = icon->pos.x;
    setup.y = icon->pos.y;
    if (icon->icon >= POKEDEX_DETAIL_TBAR_ICON_CUSTOM) {
        chars = icon->chars;
        palette = icon->palette;
        cellAnims = icon->cellAnims;
        item->type = POKEDEX_DETAIL_TBAR_TYPE_PUSH;
    } else {
        chars = PokedexDetailTbarResource_Get(resource, OBJRES_CHARS);
        palette = PokedexDetailTbarResource_Get(resource, OBJRES_PALETTE);
        cellAnims = PokedexDetailTbarResource_Get(resource, OBJRES_CELLANIMS);
        item->icon.activeAnim = sPokedexDetailTbarIconData[icon->icon].activeAnim;
        item->icon.inactiveAnim = sPokedexDetailTbarIconData[icon->icon].inactiveAnim;
        item->icon.pushedAnim = sPokedexDetailTbarIconData[icon->icon].pushedAnim;
        item->icon.key = sPokedexDetailTbarIconData[icon->icon].key;
        item->icon.se = sPokedexDetailTbarIconData[icon->icon].se;
        item->type = sPokedexDetailTbarIconData[icon->icon].type;
    }
    item->anim = item->icon.activeAnim;
    setup.sequence = item->icon.activeAnim;
    item->actor = ClActor_Create(unit, chars, palette, cellAnims, &setup, engine, heapId);
    ClActor_SetAutoAnim(item->actor, TRUE);
    switch (item->type) {
    case POKEDEX_DETAIL_TBAR_TYPE_PUSH:
        item->callback = PokedexDetailTbarItem_PushCallback;
        break;
    case POKEDEX_DETAIL_TBAR_TYPE_FLIP:
        item->callback = PokedexDetailTbarItem_FlipCallback;
        break;
    }
}

static void PokedexDetailTbarItem_Free(PokedexDetailTbarItem *item) {
    ClActor_Delete(item->actor);
    MI_CpuFill8(item, 0, sizeof(PokedexDetailTbarItem));
}

static BOOL PokedexDetailTbarItem_CheckTouch(PokedexDetailTbarItem *item) {
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

static BOOL PokedexDetailTbarItem_IsAnimEnd(const PokedexDetailTbarItem *item) {
    if (!ClActor_IsAnimActive(item->actor)) {
        return TRUE;
    }
    return FALSE;
}

static int PokedexDetailTbarItem_GetIcon(const PokedexDetailTbarItem *item) {
    return item->icon.icon;
}

static void PokedexDetailTbarItem_SetVisible(PokedexDetailTbarItem *item, BOOL visible) {
    ClActor_SetVisible(item->actor, visible);
}

static BOOL PokedexDetailTbarItem_GetVisible(const PokedexDetailTbarItem *item) {
    return ClActor_IsVisible(item->actor);
}

static void PokedexDetailTbarItem_SetActive(PokedexDetailTbarItem *item, BOOL active) {
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

static void PokedexDetailTbarItem_SetKey(PokedexDetailTbarItem *item, u32 key) {
    item->icon.key = key;
}

static void PokedexDetailTbarItem_SetFlip(PokedexDetailTbarItem *item, BOOL flip) {
    if (flip) {
        item->anim = item->icon.pushedAnim;
    } else {
        item->anim = item->icon.activeAnim;
    }
    ClActor_SetSequence(item->actor, item->anim);
}

static BOOL PokedexDetailTbarItem_GetFlip(const PokedexDetailTbarItem *item) {
    if (item->anim == item->icon.pushedAnim) {
        return TRUE;
    }
    return FALSE;
}

static void PokedexDetailTbarItem_ClearPush(PokedexDetailTbarItem *item) {
    item->pushed = FALSE;
}

static void PokedexDetailTbarItem_PushCallback(PokedexDetailTbarItem *item) {
    item->anim = item->icon.pushedAnim;
    ClActor_SetSequence(item->actor, item->icon.pushedAnim);
    if (item->icon.se != 0) {
        GFL_SndSEPlay(item->icon.se);
    }
}

static void PokedexDetailTbarItem_FlipCallback(PokedexDetailTbarItem *item) {
    PokedexDetailTbarItem_SetFlip(item, PokedexDetailTbarItem_GetFlip(item) ? FALSE : TRUE);
    if (item->icon.se != 0) {
        GFL_SndSEPlay(item->icon.se);
    }
}
