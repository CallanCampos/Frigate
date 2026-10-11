// touchbar.c: the bar of icons at the bottom of the lower screen, such as the return button. The names are ours

#include "app/ui/touchbar.h"
#include "types.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"

// The states of the bar
enum {
    TOUCHBAR_STATE_IDLE,
    TOUCHBAR_STATE_TOUCHED,
    TOUCHBAR_STATE_ANIMATING,
    TOUCHBAR_STATE_DONE,
};

// The kinds of icon: one that plays an animation when pressed, or one that toggles between two
enum {
    TOUCHBAR_KIND_PRESS,
    TOUCHBAR_KIND_TOGGLE,
};

// The index of the animations in an icon's item
enum {
    TOUCHBAR_ANIM_REST,
    TOUCHBAR_ANIM_INACTIVE,
    TOUCHBAR_ANIM_PRESSED,
    TOUCHBAR_ANIM_TOGGLED,
};

// The resources of TouchBarGfx::res
#define TOUCHBAR_RES_PALETTE 0
#define TOUCHBAR_RES_CHARS 1
#define TOUCHBAR_RES_CELL_ANIMS 2

// An icon of the bar's own, by icon number
typedef struct {
    u16 anims[4];
    u16 se;
    u32 key;
    u32 kind;
} TouchBarIconInfo;

static const TouchBarIconInfo sIconInfo[TOUCHBAR_ICON_CUSTOM] = {
    { { 0, 14, 8, 0 }, 0x556, PAD_BUTTON_X, TOUCHBAR_KIND_PRESS },
    { { 1, 15, 9, 0 }, 0x551, PAD_BUTTON_B, TOUCHBAR_KIND_PRESS },
    { { 2, 16, 10, 0 }, 0x54c, PAD_KEY_DOWN, TOUCHBAR_KIND_PRESS },
    { { 3, 17, 11, 0 }, 0x54c, PAD_KEY_UP, TOUCHBAR_KIND_PRESS },
    { { 4, 18, 12, 0 }, 0x54c, PAD_KEY_LEFT, TOUCHBAR_KIND_PRESS },
    { { 5, 19, 13, 0 }, 0x54c, PAD_KEY_RIGHT, TOUCHBAR_KIND_PRESS },
    { { 6, 21, 7, 22 }, 0x646, PAD_BUTTON_Y, TOUCHBAR_KIND_TOGGLE },
};

static TouchBarIcon *TouchBar_FindIcon(TouchBar *bar, u32 icon);
static TouchBarIcon *TouchBar_GetIcon(TouchBar *bar, u32 icon);
static void TouchBarGfx_Load(TouchBarGfx *gfx, u8 bg, u32 vramType, u32 bgScreenType, u32 mapping, u8 bgPalette,
                             u8 objPalette, BOOL noBg, HeapID heapId);
static void TouchBarGfx_Unload(TouchBarGfx *gfx);
static u32 TouchBarGfx_GetResource(const TouchBarGfx *gfx, u32 res);
static void LoadBarScreen(ArcTool *arc, u32 fileId, u32 bg, u32 charPos, u8 srcX, u8 srcY, u8 srcWidth, u8 srcHeight,
                          u8 x, u8 y, u8 width, u8 height, u8 palette, BOOL compressed, HeapID heapId);
static void TouchBarIcon_Init(TouchBarIcon *icon, ClActUnit *unit, TouchBarGfx *gfx,
                              const TouchBarItem *item, u32 surface, HeapID heapId);
static void TouchBarIcon_Delete(TouchBarIcon *icon);
static BOOL TouchBarIcon_CheckTouch(TouchBarIcon *icon);
static BOOL TouchBarIcon_IsAnimEnded(TouchBarIcon *icon);
static u32 TouchBarIcon_GetId(TouchBarIcon *icon);
static void TouchBarIcon_SetVisible(TouchBarIcon *icon, BOOL visible);
static void TouchBarIcon_SetActive(TouchBarIcon *icon, BOOL active);
static void TouchBarIcon_SetSe(TouchBarIcon *icon, u32 se);
static void TouchBarIcon_SetKey(TouchBarIcon *icon, u32 key);
static void TouchBarIcon_SetPressed(TouchBarIcon *icon, BOOL pressed);
static BOOL TouchBarIcon_IsPressed(TouchBarIcon *icon);
static void TouchBarIcon_SetPriority(TouchBarIcon *icon, u8 priority);
static void TouchBarIcon_SetBgPriority(TouchBarIcon *icon, u8 bgPriority);
static void TouchBarIcon_OnTouchPress(TouchBarIcon *icon);
static void TouchBarIcon_OnTouchToggle(TouchBarIcon *icon);
static void TouchBarIcon_SetActivePress(TouchBarIcon *icon, BOOL active);
static void TouchBarIcon_SetActiveToggle(TouchBarIcon *icon, BOOL active);

TouchBar *TouchBar_Create(const TouchBarSetup *setup, HeapID heapId) {
    TouchBar *bar;
    u32 bgScreenType;
    u32 i;
    u32 size = sizeof(TouchBar) + setup->count * sizeof(TouchBarIcon);
    u32 sub;

    sub = 0;
    bar = GFL_HeapAllocate(heapId, size, FALSE, "touchbar.c", 291);
    sys_memset(bar, 0, size);
    bar->count = setup->count;
    bar->state = sub;
    if (setup->bg >= 4) {
        sub = 1;
        bgScreenType = 4;
    } else {
        bgScreenType = 0;
    }
    TouchBarGfx_Load(&bar->gfx, setup->bg, sub, bgScreenType, setup->vramType, setup->bgPalette, setup->objPalette,
                     setup->unk1C, heapId);
    for (i = 0; i < bar->count; i++) {
        TouchBarIcon_Init(&bar->icons[i], setup->unit, &bar->gfx, &setup->items[i], sub, heapId);
    }
    return bar;
}

void TouchBar_Free(TouchBar *bar) {
    u32 i;

    for (i = 0; i < bar->count; i++) {
        TouchBarIcon_Delete(&bar->icons[i]);
    }
    TouchBarGfx_Unload(&bar->gfx);
    GFL_HeapFree(bar);
}

void TouchBar_Main(TouchBar *bar) {
    u32 i;

    switch (bar->state) {
    case TOUCHBAR_STATE_IDLE:
        bar->selected = -1;
        for (i = 0; i < bar->count; i++) {
            if (TouchBarIcon_CheckTouch(&bar->icons[i])) {
                bar->selected = i;
                bar->state = TOUCHBAR_STATE_TOUCHED;
                return;
            }
        }
        break;
    case TOUCHBAR_STATE_TOUCHED:
        bar->state = TOUCHBAR_STATE_ANIMATING;
        // fallthrough
    case TOUCHBAR_STATE_ANIMATING:
        if (TouchBarIcon_IsAnimEnded(&bar->icons[bar->selected])) {
            bar->state = TOUCHBAR_STATE_DONE;
        }
        break;
    case TOUCHBAR_STATE_DONE:
        bar->state = TOUCHBAR_STATE_IDLE;
        break;
    }
}

u32 TouchBar_GetDecided(TouchBar *bar) {
    if (bar->state == TOUCHBAR_STATE_DONE && bar->selected != -1) {
        return TouchBarIcon_GetId(&bar->icons[bar->selected]);
    }
    return -1;
}

u32 TouchBar_GetTouched(TouchBar *bar) {
    if (bar->state == TOUCHBAR_STATE_TOUCHED && bar->selected != -1) {
        return TouchBarIcon_GetId(&bar->icons[bar->selected]);
    }
    return -1;
}

BOOL TouchBar_IsTouching(TouchBar *bar) {
    if (bar->state != TOUCHBAR_STATE_IDLE) {
        return TRUE;
    }
    return FALSE;
}

void TouchBar_SetActive(TouchBar *bar, BOOL active) {
    u32 i;

    for (i = 0; i < bar->count; i++) {
        TouchBarIcon_SetActive(&bar->icons[i], active);
    }
}

void TouchBar_SetVisible(TouchBar *bar, BOOL visible) {
    u32 i;

    for (i = 0; i < bar->count; i++) {
        TouchBarIcon_SetVisible(&bar->icons[i], visible);
    }
}

void TouchBar_SetBgPriority(TouchBar *bar, u8 bgPriority) {
    u32 i;

    for (i = 0; i < bar->count; i++) {
        TouchBarIcon_SetBgPriority(&bar->icons[i], bgPriority);
    }
}

void TouchBar_SetPriority(TouchBar *bar, u8 priority) {
    u32 i;

    for (i = 0; i < bar->count; i++) {
        TouchBarIcon_SetPriority(&bar->icons[i], priority);
    }
}

void TouchBar_SetIconActive(TouchBar *bar, u32 icon, BOOL active) {
    TouchBarIcon_SetActive(TouchBar_FindIcon(bar, icon), active);
}

void TouchBar_SetIconVisible(TouchBar *bar, u32 icon, BOOL visible) {
    TouchBarIcon_SetVisible(TouchBar_FindIcon(bar, icon), visible);
}

void TouchBar_SetIconSe(TouchBar *bar, u32 icon, u32 se) {
    TouchBarIcon_SetSe(TouchBar_FindIcon(bar, icon), se);
}

void TouchBar_SetIconKey(TouchBar *bar, u32 icon, u32 key) {
    TouchBarIcon_SetKey(TouchBar_FindIcon(bar, icon), key);
}

void TouchBar_SetIconBgPriority(TouchBar *bar, u32 icon, u8 bgPriority) {
    TouchBarIcon_SetBgPriority(TouchBar_FindIcon(bar, icon), bgPriority);
}

void TouchBar_SetIconPressed(TouchBar *bar, u32 icon, BOOL pressed) {
    TouchBarIcon_SetPressed(TouchBar_FindIcon(bar, icon), pressed);
}

BOOL TouchBar_IsIconPressed(TouchBar *bar, u32 icon) {
    return TouchBarIcon_IsPressed(TouchBar_GetIcon(bar, icon));
}

// Makes the bar idle again, then sets whether an icon is active
void TouchBar_Reset(TouchBar *bar, u32 icon) {
    bar->state = TOUCHBAR_STATE_IDLE;
    bar->selected = -1;
    TouchBar_SetIconActive(bar, icon, TRUE);
}

static TouchBarIcon *TouchBar_FindIcon(TouchBar *bar, u32 icon) {
    u32 i;

    for (i = 0; i < bar->count; i++) {
        if (icon == TouchBarIcon_GetId(&bar->icons[i])) {
            return bar->icons + i;
        }
    }
    return NULL;
}

static TouchBarIcon *TouchBar_GetIcon(TouchBar *bar, u32 icon) {
    return TouchBar_FindIcon(bar, icon);
}

static void TouchBarGfx_Load(TouchBarGfx *gfx, u8 bg, u32 vramType, u32 bgScreenType, u32 mapping, u8 bgPalette,
                             u8 objPalette, BOOL noBg, HeapID heapId) {
    ArcTool *arc;

    sys_memset(gfx, 0, sizeof(TouchBarGfx));
    gfx->bg = bg;
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    if (!noBg) {
        GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), bgScreenType, bgPalette * 32, 0x20, heapId);
        gfx->bgChars = GFL_BGSysLoadArcNCGRDynamic(arc, func_0202d824(), gfx->bg, 0x20 << 6, FALSE, heapId);
        LoadBarScreen(arc, func_0202d828(), gfx->bg, CHAR_POS(gfx->bgChars), 0, 21, 32, 24, 0, 21, 32, 3, bgPalette,
                      FALSE, heapId);
    }
    gfx->res[TOUCHBAR_RES_PALETTE] =
        func_0204bbb8(arc, func_0202d810(), vramType, objPalette * 32, 0, 3, heapId);
    gfx->res[TOUCHBAR_RES_CHARS] = func_0204b81c(arc, func_0202d814(), FALSE, vramType, heapId);
    gfx->res[TOUCHBAR_RES_CELL_ANIMS] = func_0204bde0(arc, func_0202d818(mapping), func_0202d81c(mapping), heapId);
    GFL_ArcToolFree(arc);
}

static void TouchBarGfx_Unload(TouchBarGfx *gfx) {
    func_0204be64(gfx->res[TOUCHBAR_RES_CELL_ANIMS]);
    func_0204b98c(gfx->res[TOUCHBAR_RES_CHARS]);
    func_0204bcd0(gfx->res[TOUCHBAR_RES_PALETTE]);
    GFL_BGSysFreeCharMemory(gfx->bg, CHAR_POS(gfx->bgChars), CHAR_SIZE(gfx->bgChars));
    sys_memset(gfx, 0, sizeof(TouchBarGfx));
}

static u32 TouchBarGfx_GetResource(const TouchBarGfx *gfx, u32 res) {
    return gfx->res[res];
}

// Loads a screen's rectangle into a BG, adding charPos to its characters when the BG has 16 colors palettes
static void LoadBarScreen(ArcTool *arc, u32 fileId, u32 bg, u32 charPos, u8 srcX, u8 srcY, u8 srcWidth, u8 srcHeight,
                          u8 x, u8 y, u8 width, u8 height, u8 palette, BOOL compressed, HeapID heapId) {
    NNSG2dScreenData *screen;
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, HEAPID_TAIL(heapId));
    int i;

    NNS_G2dGetUnpackedScreenData(file, &screen);
    if (charPos != 0 && GFL_BGSysGetBGColorPaletteMode(bg) == 0) {
        u16 *data = (u16 *)screen->rawData;

        for (i = 0; i < srcWidth * srcHeight; i++) {
            data[i] += (u16)charPos;
        }
    }
    if (GFL_BGSysIsScrHeapExists(bg)) {
        GFL_BGSysLoadScrArea(bg, x, y, width, height, screen->rawData, srcX, srcY, srcWidth, srcHeight);
        GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, palette);
        GFL_BGSysLoadScr(bg);
    }
    GFL_HeapFree(file);
}

static void TouchBarIcon_Init(TouchBarIcon *icon, ClActUnit *unit, TouchBarGfx *gfx,
                              const TouchBarItem *item, u32 surface, HeapID heapId) {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cellAnims;

    sys_memset(icon, 0, sizeof(TouchBarIcon));
    icon->item = *item;
    icon->active = TRUE;
    sys_memset(&setup, 0, sizeof(setup));
    setup.x = item->pos.x;
    setup.y = item->pos.y;
    if (item->icon >= TOUCHBAR_ICON_CUSTOM) {
        chars = item->charRes;
        palette = item->plttRes;
        cellAnims = item->cellRes;
        icon->kind = TOUCHBAR_KIND_PRESS;
    } else {
        chars = TouchBarGfx_GetResource(gfx, TOUCHBAR_RES_CHARS);
        palette = TouchBarGfx_GetResource(gfx, TOUCHBAR_RES_PALETTE);
        cellAnims = TouchBarGfx_GetResource(gfx, TOUCHBAR_RES_CELL_ANIMS);
        icon->item.anims[0] = sIconInfo[item->icon].anims[0];
        icon->item.anims[1] = sIconInfo[item->icon].anims[1];
        icon->item.anims[2] = sIconInfo[item->icon].anims[2];
        icon->item.anims[3] = sIconInfo[item->icon].anims[3];
        icon->item.key = sIconInfo[item->icon].key;
        icon->item.se = sIconInfo[item->icon].se;
        icon->kind = sIconInfo[item->icon].kind;
    }
    icon->anim = icon->item.anims[0];
    setup.sequence = icon->item.anims[0];
    icon->actor = func_0204c040(unit, chars, palette, cellAnims, &setup, surface, heapId);
    func_0204c520(icon->actor, TRUE);
    switch (icon->kind) {
    case TOUCHBAR_KIND_PRESS:
        icon->onTouch = TouchBarIcon_OnTouchPress;
        icon->setActive = TouchBarIcon_SetActivePress;
        break;
    case TOUCHBAR_KIND_TOGGLE:
        icon->onTouch = TouchBarIcon_OnTouchToggle;
        icon->setActive = TouchBarIcon_SetActiveToggle;
        break;
    }
}

static void TouchBarIcon_Delete(TouchBarIcon *icon) {
    func_0204c108(icon->actor);
    sys_memset(icon, 0, sizeof(TouchBarIcon));
}

// Whether the icon was touched or its key pressed, in which case its touch callback has run
static BOOL TouchBarIcon_CheckTouch(TouchBarIcon *icon) {
    BOOL touched = FALSE;

    if (func_0204c138(icon->actor) & icon->active) {
        u32 x, y;

        if (func_0203dac8(&x, &y)) {
            if (x - icon->item.pos.x <= 24 && y - icon->item.pos.y <= 24) {
                touched = TRUE;
                func_0203d564(TRUE);
            }
        }
        if (icon->item.key != 0 && (GCTX_HIDGetPressedKeys() & icon->item.key)) {
            func_0203d564(FALSE);
            touched = TRUE;
        }
        if (touched) {
            icon->onTouch(icon);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL TouchBarIcon_IsAnimEnded(TouchBarIcon *icon) {
    if (func_0204c560(icon->actor) == 0) {
        return TRUE;
    }
    return FALSE;
}

static u32 TouchBarIcon_GetId(TouchBarIcon *icon) {
    return icon->item.icon;
}

static void TouchBarIcon_SetVisible(TouchBarIcon *icon, BOOL visible) {
    func_0204c124(icon->actor, visible);
}

static void TouchBarIcon_SetActive(TouchBarIcon *icon, BOOL active) {
    icon->setActive(icon, active);
}

static void TouchBarIcon_SetSe(TouchBarIcon *icon, u32 se) {
    icon->item.se = se;
}

static void TouchBarIcon_SetKey(TouchBarIcon *icon, u32 key) {
    icon->item.key = key;
}

static void TouchBarIcon_SetPressed(TouchBarIcon *icon, BOOL pressed) {
    if (pressed) {
        icon->anim = icon->item.anims[TOUCHBAR_ANIM_PRESSED];
    } else {
        icon->anim = icon->item.anims[TOUCHBAR_ANIM_REST];
    }
    func_0204c488(icon->actor, icon->anim);
}

static BOOL TouchBarIcon_IsPressed(TouchBarIcon *icon) {
    BOOL pressed = TRUE;

    if (icon->anim != icon->item.anims[TOUCHBAR_ANIM_PRESSED] && icon->anim != icon->item.anims[TOUCHBAR_ANIM_TOGGLED]) {
        pressed = FALSE;
    }
    return pressed;
}

static void TouchBarIcon_SetPriority(TouchBarIcon *icon, u8 priority) {
    func_0204c438(icon->actor, priority);
}

static void TouchBarIcon_SetBgPriority(TouchBarIcon *icon, u8 bgPriority) {
    func_0204c468(icon->actor, bgPriority);
}

static void TouchBarIcon_OnTouchPress(TouchBarIcon *icon) {
    icon->anim = icon->item.anims[TOUCHBAR_ANIM_PRESSED];
    func_0204c488(icon->actor, icon->anim);
    if (icon->item.se != 0) {
        GFL_SndSEPlay(icon->item.se);
    }
}

static void TouchBarIcon_OnTouchToggle(TouchBarIcon *icon) {
    TouchBarIcon_SetPressed(icon, !TouchBarIcon_IsPressed(icon));
    if (icon->item.se != 0) {
        GFL_SndSEPlay(icon->item.se);
    }
}

static void TouchBarIcon_SetActivePress(TouchBarIcon *icon, BOOL active) {
    icon->active = active;
    if (active) {
        if (icon->anim == icon->item.anims[TOUCHBAR_ANIM_PRESSED]) {
            icon->anim = icon->item.anims[TOUCHBAR_ANIM_REST];
        }
        func_0204c488(icon->actor, icon->anim);
    } else {
        func_0204c488(icon->actor, icon->item.anims[TOUCHBAR_ANIM_INACTIVE]);
    }
}

static void TouchBarIcon_SetActiveToggle(TouchBarIcon *icon, BOOL active) {
    icon->active = active;
    if (active) {
        if (icon->anim == icon->item.anims[TOUCHBAR_ANIM_PRESSED]) {
            icon->anim = icon->item.anims[TOUCHBAR_ANIM_TOGGLED];
        } else if (icon->anim == icon->item.anims[TOUCHBAR_ANIM_INACTIVE]) {
            icon->anim = icon->item.anims[TOUCHBAR_ANIM_REST];
        }
        func_0204c488(icon->actor, icon->anim);
    } else {
        if (icon->anim == icon->item.anims[TOUCHBAR_ANIM_TOGGLED]) {
            icon->anim = icon->item.anims[TOUCHBAR_ANIM_PRESSED];
        } else if (icon->anim == icon->item.anims[TOUCHBAR_ANIM_REST]) {
            icon->anim = icon->item.anims[TOUCHBAR_ANIM_INACTIVE];
        }
        func_0204c488(icon->actor, icon->item.anims[TOUCHBAR_ANIM_INACTIVE]);
    }
}
