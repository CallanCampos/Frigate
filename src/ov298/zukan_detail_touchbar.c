#include "types.h"
#include "app/zukan_detail.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/g2d.h"

// The detail screen's touch bar, on BG 1 of the main screen: the tabs of the pages with the buttons to close, return,
// check the Pokémon and move through the list, or the map's and the forms' own buttons. It slides in and out, and the
// tab of the page being shown glows

#define ITEM_MAX 9
#define ITEM_NONE ITEM_MAX

// How far the bar is hidden below the screen
#define HIDDEN_Y 24

// The bar's icons of each type, in the order of the bar's items
enum {
    GENERAL_RETURN,
    GENERAL_CLOSE,
    GENERAL_CHECK,
    GENERAL_FORM,
    GENERAL_VOICE,
    GENERAL_MAP,
    GENERAL_INFO,
    GENERAL_CUR_D,
    GENERAL_CUR_U,
};

enum {
    MAP_RETURN,
    MAP_PLACE,
};

enum {
    FORM_RETURN,
    FORM_CUR_R,
    FORM_CUR_L,
    FORM_BUTTON,
    FORM_CUR_D,
    FORM_CUR_U,
};

// The Pokédex's own icons
enum {
    ICON_INFO = ZKND_TBAR_ICON_CUSTOM,
    ICON_MAP,
    ICON_VOICE,
    ICON_FORM,
};

typedef struct {
    int icon;
    ClActorPos pos;
    u16 width;
    // The commands of the icon once its animation has played, and as it is touched
    int trigger;
    int touch;
} ZukanDetailTouchbarIcon;

typedef struct {
    const ZukanDetailTouchbarIcon *icon;
    ClActor *actor;
} ZukanDetailTouchbarItem;

// The resources of the Pokédex's own icons
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} ZukanDetailTouchbarRes;

struct ZukanDetailTouchbar {
    HeapID heapId;
    BOOL showFormTab;
    u32 mode;
    int state;
    int type;
    // The page shown, and the page whose tab glows
    int page;
    int selectedPage;
    u32 speed;
    u8 wait;
    ClActUnit *unit;
    TCB *vblankTcb;
    ZkndTbar *tbar;
    BOOL active;
    u8 bgPriority;
    BOOL bgPriorityChanged;
    BOOL showArrows;
    ZukanDetailTouchbarRes res[3];
    u8 itemCount;
    ZukanDetailTouchbarItem items[ITEM_MAX];
    s16 scrollY;
    // The selected tab's palette glows between two palettes
    u16 glowPalettes[2][16];
    u16 glowPalette[16];
    int glowPhase;
};

static void ZukanDetailTouchbar_VBlank(TCB *tcb, void *data);
static void ZukanDetailTouchbar_CreateType(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_FreeType(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_SetItemsY(ZukanDetailTouchbar *touchbar, s16 offset);
static void ZukanDetailTouchbar_CreateGeneral(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_FreeGeneral(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_UpdateGeneral(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_InitGlow(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_FreeGlow(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_StartGlow(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_StopGlow(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_UpdateGlow(ZukanDetailTouchbar *touchbar);
static u8 ZukanDetailTouchbar_GetPageItem(int page);
static u8 ZukanDetailTouchbar_GetCommandItem(int command);
static u8 ZukanDetailTouchbar_GetCommandPage(int command);
static void ZukanDetailTouchbar_CreateMap(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_FreeMap(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_CreateForm(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_FreeForm(ZukanDetailTouchbar *touchbar);
static void ZukanDetailTouchbar_UpdateForm(ZukanDetailTouchbar *touchbar);

// How long the bar waits between moves, and how far it moves, at each speed
static const u8 sZukanDetailTouchbarWaits[2] = { 0, 0 };
static const u32 sZukanDetailTouchbarSpeeds[2] = { 3, 3 };

// Where the general icons go when the bar is in the other mode, and when the forms' tab is hidden
static const ClActorPos sZukanDetailTouchbarNoFormPositions[ITEM_MAX] = {
    { 232, 168 }, { 208, 168 }, { 184, 172 }, { 168, 168 }, { 136, 168 },
    { 96, 168 },  { 56, 168 },  { 24, 168 },  { 0, 168 },
};

static const ClActorPos sZukanDetailTouchbarModePositions[ITEM_MAX] = {
    { 232, 168 }, { 208, 168 }, { 184, 172 }, { 136, 168 }, { 96, 168 },
    { 96, 168 },  { 56, 168 },  { 24, 168 },  { 0, 168 },
};

static const ZukanDetailTouchbarIcon sZukanDetailTouchbarMapIcons[] = {
    { ZKND_TBAR_ICON_RETURN, { 232, 168 }, 24, ZUKAN_DETAIL_CMD_MAP_RETURN, ZUKAN_DETAIL_CMD_MAP_RETURN_TOUCH },
    { ICON_MAP, { 56, 168 }, 160, ZUKAN_DETAIL_CMD_MAP_PLACE, ZUKAN_DETAIL_CMD_MAP_PLACE_TOUCH },
};

static const ZukanDetailTouchbarIcon sZukanDetailTouchbarFormIcons[] = {
    { ZKND_TBAR_ICON_RETURN, { 232, 168 }, 24, ZUKAN_DETAIL_CMD_FORM_RETURN, ZUKAN_DETAIL_CMD_FORM_RETURN_TOUCH },
    { ZKND_TBAR_ICON_CUR_R, { 192, 168 }, 24, ZUKAN_DETAIL_CMD_FORM_CUR_R, ZUKAN_DETAIL_CMD_FORM_CUR_R_TOUCH },
    { ZKND_TBAR_ICON_CUR_L, { 168, 168 }, 24, ZUKAN_DETAIL_CMD_FORM_CUR_L, ZUKAN_DETAIL_CMD_FORM_CUR_L_TOUCH },
    { ICON_INFO, { 104, 168 }, 48, ZUKAN_DETAIL_CMD_FORM_BUTTON, ZUKAN_DETAIL_CMD_FORM_BUTTON_TOUCH },
    { ZKND_TBAR_ICON_CUR_D, { 24, 168 }, 24, ZUKAN_DETAIL_CMD_FORM_CUR_D, ZUKAN_DETAIL_CMD_FORM_CUR_D_TOUCH },
    { ZKND_TBAR_ICON_CUR_U, { 0, 168 }, 24, ZUKAN_DETAIL_CMD_FORM_CUR_U, ZUKAN_DETAIL_CMD_FORM_CUR_U_TOUCH },
};

static const ZukanDetailTouchbarIcon sZukanDetailTouchbarGeneralIcons[] = {
    { ZKND_TBAR_ICON_RETURN, { 232, 168 }, 24, ZUKAN_DETAIL_CMD_RETURN, ZUKAN_DETAIL_CMD_RETURN_TOUCH },
    { ZKND_TBAR_ICON_CLOSE, { 208, 168 }, 24, ZUKAN_DETAIL_CMD_CLOSE, ZUKAN_DETAIL_CMD_CLOSE_TOUCH },
    { ZKND_TBAR_ICON_CHECK, { 184, 172 }, 24, ZUKAN_DETAIL_CMD_CHECK, ZUKAN_DETAIL_CMD_CHECK_TOUCH },
    { ICON_FORM, { 144, 168 }, 32, ZUKAN_DETAIL_CMD_FORM, ZUKAN_DETAIL_CMD_FORM_TOUCH },
    { ICON_VOICE, { 112, 168 }, 32, ZUKAN_DETAIL_CMD_VOICE, ZUKAN_DETAIL_CMD_VOICE_TOUCH },
    { ICON_MAP, { 80, 168 }, 32, ZUKAN_DETAIL_CMD_MAP, ZUKAN_DETAIL_CMD_MAP_TOUCH },
    { ICON_INFO, { 48, 168 }, 32, ZUKAN_DETAIL_CMD_INFO, ZUKAN_DETAIL_CMD_INFO_TOUCH },
    { ZKND_TBAR_ICON_CUR_D, { 24, 168 }, 24, ZUKAN_DETAIL_CMD_CUR_D, ZUKAN_DETAIL_CMD_CUR_D_TOUCH },
    { ZKND_TBAR_ICON_CUR_U, { 0, 168 }, 24, ZUKAN_DETAIL_CMD_CUR_U, ZUKAN_DETAIL_CMD_CUR_U_TOUCH },
};

ZukanDetailTouchbar *ZukanDetailTouchbar_Create(HeapID heapId, BOOL showFormTab, u32 mode) {
    ZukanDetailTouchbar *touchbar =
        GFL_HeapAllocate(heapId, sizeof(ZukanDetailTouchbar), TRUE, "zukan_detail_touchbar.c", 342);

    touchbar->heapId = heapId;
    touchbar->showFormTab = showFormTab;
    touchbar->mode = mode;
    touchbar->state = ZUKAN_DETAIL_TOUCHBAR_HIDDEN;
    touchbar->type = ZUKAN_DETAIL_TOUCHBAR_GENERAL;
    touchbar->page = ZUKAN_DETAIL_PAGE_INFO - 1;
    touchbar->selectedPage = ZUKAN_DETAIL_PAGE_INFO - 1;
    touchbar->speed = 0;
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, -HIDDEN_Y);
    GFL_BGSysSetBGPriority(1, 0);
    touchbar->bgPriority = 0;
    touchbar->bgPriorityChanged = FALSE;
    touchbar->unit = func_0204bf1c(16, 0, touchbar->heapId);
    func_0204c028(touchbar->unit);
    touchbar->tbar = NULL;
    ZukanDetailTouchbar_CreateType(touchbar);
    ZkndTbar_SetActiveAll(touchbar->tbar, FALSE);
    touchbar->scrollY = HIDDEN_Y;
    ZukanDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
    touchbar->vblankTcb = GFL_VBlankTCBAdd(ZukanDetailTouchbar_VBlank, touchbar, 1);
    return touchbar;
}

void ZukanDetailTouchbar_Free(ZukanDetailTouchbar *touchbar) {
    GFL_TCBRemove(touchbar->vblankTcb);
    ZukanDetailTouchbar_FreeType(touchbar);
    func_0204bf98(touchbar->unit);
    GFL_HeapFree(touchbar);
}

void ZukanDetailTouchbar_Update(ZukanDetailTouchbar *touchbar) {
    switch (touchbar->state) {
    case ZUKAN_DETAIL_TOUCHBAR_HIDDEN:
    case ZUKAN_DETAIL_TOUCHBAR_SHOWN:
        break;
    case ZUKAN_DETAIL_TOUCHBAR_APPEARING:
        if (touchbar->wait == 0) {
            if (GFL_BGSysGetBGOffsetY(1) >= 0) {
                GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, 0);
                touchbar->scrollY = 0;
                ZukanDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->state = ZUKAN_DETAIL_TOUCHBAR_SHOWN;
                if (touchbar->active) {
                    ZkndTbar_SetActiveAll(touchbar->tbar, TRUE);
                }
            } else {
                GFL_BGSysMoveBGReq(1, BG_MOVE_DOWN, sZukanDetailTouchbarSpeeds[touchbar->speed]);
                touchbar->scrollY = touchbar->scrollY - (s16)sZukanDetailTouchbarSpeeds[touchbar->speed];
                ZukanDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->wait = sZukanDetailTouchbarWaits[touchbar->speed];
            }
        } else {
            touchbar->wait--;
        }
        break;
    case ZUKAN_DETAIL_TOUCHBAR_DISAPPEARING:
        if (touchbar->wait == 0) {
            if (GFL_BGSysGetBGOffsetY(1) <= -HIDDEN_Y) {
                GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, -HIDDEN_Y);
                touchbar->scrollY = HIDDEN_Y;
                ZukanDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->state = ZUKAN_DETAIL_TOUCHBAR_HIDDEN;
            } else {
                GFL_BGSysMoveBGReq(1, BG_MOVE_UP, sZukanDetailTouchbarSpeeds[touchbar->speed]);
                touchbar->scrollY = touchbar->scrollY + (s16)sZukanDetailTouchbarSpeeds[touchbar->speed];
                ZukanDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->wait = sZukanDetailTouchbarWaits[touchbar->speed];
            }
        } else {
            touchbar->wait--;
        }
        break;
    }

    ZkndTbar_Main(touchbar->tbar);
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_GENERAL) {
        ZukanDetailTouchbar_UpdateGeneral(touchbar);
        ZukanDetailTouchbar_UpdateGlow(touchbar);
    } else if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_FORM) {
        ZukanDetailTouchbar_UpdateForm(touchbar);
    }
}

void ZukanDetailTouchbar_SetType(ZukanDetailTouchbar *touchbar, int type, int page, BOOL showArrows) {
    ZukanDetailTouchbar_FreeType(touchbar);
    touchbar->type = type;
    touchbar->page = page;
    touchbar->showArrows = showArrows;
    touchbar->selectedPage = page;
    ZukanDetailTouchbar_CreateType(touchbar);
}

int ZukanDetailTouchbar_GetState(ZukanDetailTouchbar *touchbar) {
    return touchbar->state;
}

void ZukanDetailTouchbar_Appear(ZukanDetailTouchbar *touchbar, u32 speed) {
    touchbar->speed = speed;
    if (touchbar->state != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
        touchbar->state = ZUKAN_DETAIL_TOUCHBAR_APPEARING;
        touchbar->wait = 0;
    }
}

void ZukanDetailTouchbar_Disappear(ZukanDetailTouchbar *touchbar, u32 speed) {
    touchbar->speed = speed;
    if (touchbar->state != ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
        touchbar->state = ZUKAN_DETAIL_TOUCHBAR_DISAPPEARING;
        touchbar->wait = 0;
        ZkndTbar_SetActiveAll(touchbar->tbar, FALSE);
    }
}

int ZukanDetailTouchbar_GetTrigger(ZukanDetailTouchbar *touchbar) {
    int command;
    int icon;
    u8 i;

    icon = ZkndTbar_GetTrigger(touchbar->tbar);
    command = ZUKAN_DETAIL_CMD_NONE;
    if (icon != -1) {
        switch (touchbar->type) {
        case ZUKAN_DETAIL_TOUCHBAR_GENERAL:
        case ZUKAN_DETAIL_TOUCHBAR_MAP:
        case ZUKAN_DETAIL_TOUCHBAR_FORM:
            for (i = 0; i < touchbar->itemCount; i++) {
                if (icon == touchbar->items[i].icon->icon) {
                    command = touchbar->items[i].icon->trigger;
                    break;
                }
            }
            break;
        }
    }
    return command;
}

int ZukanDetailTouchbar_GetTouch(ZukanDetailTouchbar *touchbar) {
    int command;
    int icon;
    u8 i;

    icon = ZkndTbar_GetTouch(touchbar->tbar);
    command = ZUKAN_DETAIL_CMD_NONE;
    if (icon != -1) {
        switch (touchbar->type) {
        case ZUKAN_DETAIL_TOUCHBAR_GENERAL:
        case ZUKAN_DETAIL_TOUCHBAR_MAP:
        case ZUKAN_DETAIL_TOUCHBAR_FORM:
            for (i = 0; i < touchbar->itemCount; i++) {
                if (icon == touchbar->items[i].icon->icon) {
                    command = touchbar->items[i].icon->touch;
                    break;
                }
            }
            break;
        }
    }
    return command;
}

void ZukanDetailTouchbar_Unlock(ZukanDetailTouchbar *touchbar) {
    ZkndTbar_Unlock(touchbar->tbar);
}

void ZukanDetailTouchbar_SetVisibleAll(ZukanDetailTouchbar *touchbar, BOOL visible) {
    ZkndTbar_SetVisibleAll(touchbar->tbar, visible);
}

void ZukanDetailTouchbar_SetPage(ZukanDetailTouchbar *touchbar, int page) {
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_GENERAL) {
        BOOL restore = FALSE;
        int selectedPage;

        touchbar->page = page;
        if (page == touchbar->selectedPage) {
            selectedPage = touchbar->selectedPage;
            restore = TRUE;
            touchbar->selectedPage = ZUKAN_DETAIL_PAGE_FORM;
        }
        ZukanDetailTouchbar_StopGlow(touchbar);
        ZukanDetailTouchbar_StartGlow(touchbar);
        if (restore) {
            touchbar->selectedPage = selectedPage;
        }
    }
}

void ZukanDetailTouchbar_SetFormArrowsVisible(ZukanDetailTouchbar *touchbar, BOOL visible) {
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_FORM) {
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_R].icon->icon, visible);
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_L].icon->icon, visible);
    }
}

void ZukanDetailTouchbar_SetCheck(ZukanDetailTouchbar *touchbar, BOOL check) {
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_GENERAL) {
        ZkndTbar_SetFlip(touchbar->tbar, touchbar->items[GENERAL_CHECK].icon->icon, check);
    }
}

BOOL ZukanDetailTouchbar_GetCheck(ZukanDetailTouchbar *touchbar) {
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_GENERAL) {
        return ZkndTbar_GetFlip(touchbar->tbar, touchbar->items[GENERAL_CHECK].icon->icon);
    }
    return FALSE;
}

void ZukanDetailTouchbar_SetActive(ZukanDetailTouchbar *touchbar, BOOL active) {
    touchbar->active = active;
    if (active) {
        if (!ZkndTbar_GetActiveAll(touchbar->tbar) && touchbar->state == ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZkndTbar_SetActiveAll(touchbar->tbar, TRUE);
        }
    } else {
        if (ZkndTbar_GetActiveAll(touchbar->tbar)) {
            ZkndTbar_SetActiveAll(touchbar->tbar, FALSE);
        }
    }
}

// The palette of the Pokédex's own icons
u32 ZukanDetailTouchbar_GetIconPalette(ZukanDetailTouchbar *touchbar) {
    switch (touchbar->type) {
    case ZUKAN_DETAIL_TOUCHBAR_GENERAL:
        return touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette;
    case ZUKAN_DETAIL_TOUCHBAR_MAP:
        return touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].palette;
    case ZUKAN_DETAIL_TOUCHBAR_FORM:
        return touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].palette;
    }
    return -1;
}

// Sets the priority of the bar's BG at the next VBlank, and the icons' with it
void ZukanDetailTouchbar_SetBGPriority(ZukanDetailTouchbar *touchbar, u8 priority) {
    u8 i;

    touchbar->bgPriority = priority;
    touchbar->bgPriorityChanged = TRUE;
    for (i = 0; i < touchbar->itemCount; i++) {
        func_0204c468(touchbar->items[i].actor, priority);
    }
}

static void ZukanDetailTouchbar_VBlank(TCB *tcb, void *data) {
    ZukanDetailTouchbar *touchbar = data;

    if (touchbar->bgPriorityChanged) {
        GFL_BGSysSetBGPriority(1, touchbar->bgPriority);
        touchbar->bgPriorityChanged = FALSE;
    }
}

static void ZukanDetailTouchbar_CreateType(ZukanDetailTouchbar *touchbar) {
    switch (touchbar->type) {
    case ZUKAN_DETAIL_TOUCHBAR_GENERAL:
        ZukanDetailTouchbar_CreateGeneral(touchbar);
        break;
    case ZUKAN_DETAIL_TOUCHBAR_MAP:
        ZukanDetailTouchbar_CreateMap(touchbar);
        break;
    case ZUKAN_DETAIL_TOUCHBAR_FORM:
        ZukanDetailTouchbar_CreateForm(touchbar);
        break;
    }
    touchbar->active = TRUE;
}

static void ZukanDetailTouchbar_FreeType(ZukanDetailTouchbar *touchbar) {
    switch (touchbar->type) {
    case ZUKAN_DETAIL_TOUCHBAR_GENERAL:
        ZukanDetailTouchbar_FreeGeneral(touchbar);
        break;
    case ZUKAN_DETAIL_TOUCHBAR_MAP:
        ZukanDetailTouchbar_FreeMap(touchbar);
        break;
    case ZUKAN_DETAIL_TOUCHBAR_FORM:
        ZukanDetailTouchbar_FreeForm(touchbar);
        break;
    }
}

// Moves the icons with the bar's BG
static void ZukanDetailTouchbar_SetItemsY(ZukanDetailTouchbar *touchbar, s16 offset) {
    u8 i;
    ClActorPos pos;

    for (i = 0; i < touchbar->itemCount; i++) {
        func_0204c21c(touchbar->items[i].actor, &pos);
        pos.y = touchbar->items[i].icon->pos.y;
        pos.y += offset;
        func_0204c210(touchbar->items[i].actor, &pos);
    }
}

static void ZukanDetailTouchbar_CreateGeneral(ZukanDetailTouchbar *touchbar) {
    ZkndTbarIcon icons[ITEM_MAX];
    ZkndTbarParam param;
    ArcTool *arc;
    u8 i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette =
        func_0204bbb8(arc, 3, CLACT_VRAM_MAIN, 11 * 32, 0, 3, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].chars =
        func_0204b81c(arc, 13, FALSE, CLACT_VRAM_MAIN, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].cellAnims = func_0204bde0(arc, 28, 45, touchbar->heapId);
    GFL_ArcToolFree(arc);

    touchbar->itemCount = NELEMS(sZukanDetailTouchbarGeneralIcons);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].icon = &sZukanDetailTouchbarGeneralIcons[i];
    }
    for (i = 0; i < touchbar->itemCount; i++) {
        icons[i].icon = touchbar->items[i].icon->icon;
        icons[i].pos = touchbar->items[i].icon->pos;
        icons[i].width = touchbar->items[i].icon->width;
    }

    icons[GENERAL_FORM].chars = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_FORM].palette = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_FORM].cellAnims = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_FORM].activeAnim = 3;
    icons[GENERAL_FORM].inactiveAnim = 3;
    icons[GENERAL_FORM].pushedAnim = 7;
    icons[GENERAL_FORM].key = 0;
    icons[GENERAL_FORM].se = SEQ_SE_SELECT3;
    icons[GENERAL_VOICE].chars = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_VOICE].palette = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_VOICE].cellAnims = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_VOICE].activeAnim = 2;
    icons[GENERAL_VOICE].inactiveAnim = 2;
    icons[GENERAL_VOICE].pushedAnim = 6;
    icons[GENERAL_VOICE].key = 0;
    icons[GENERAL_VOICE].se = SEQ_SE_SELECT3;
    icons[GENERAL_MAP].chars = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_MAP].palette = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_MAP].cellAnims = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_MAP].activeAnim = 1;
    icons[GENERAL_MAP].inactiveAnim = 1;
    icons[GENERAL_MAP].pushedAnim = 5;
    icons[GENERAL_MAP].key = 0;
    icons[GENERAL_MAP].se = SEQ_SE_SELECT3;
    icons[GENERAL_INFO].chars = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_INFO].palette = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_INFO].cellAnims = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_INFO].activeAnim = 0;
    icons[GENERAL_INFO].inactiveAnim = 0;
    icons[GENERAL_INFO].pushedAnim = 4;
    icons[GENERAL_INFO].key = 0;
    icons[GENERAL_INFO].se = SEQ_SE_SELECT3;

    sys_memset(&param, 0, sizeof(ZkndTbarParam));
    param.icons = icons;
    param.iconCount = NELEMS(sZukanDetailTouchbarGeneralIcons);
    param.unit = touchbar->unit;
    param.bg = 1;
    param.bgPalette = 13;
    param.objPalette = 8;
    param.mapping = 2;
    touchbar->tbar = ZkndTbar_Create(&param, touchbar->heapId);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].actor = ZkndTbar_GetActor(touchbar->tbar, touchbar->items[i].icon->icon);
    }

    if (!touchbar->showFormTab) {
        for (i = 0; i < ITEM_MAX; i++) {
            ZkndTbar_SetPos(touchbar->tbar, touchbar->items[i].icon->icon, &sZukanDetailTouchbarNoFormPositions[i]);
        }
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_FORM].icon->icon, FALSE);
    }
    if (touchbar->mode != 0) {
        for (i = 0; i < ITEM_MAX; i++) {
            ZkndTbar_SetPos(touchbar->tbar, touchbar->items[i].icon->icon, &sZukanDetailTouchbarModePositions[i]);
        }
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_MAP].icon->icon, FALSE);
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_CHECK].icon->icon, FALSE);
    }

    i = ZukanDetailTouchbar_GetPageItem(touchbar->page);
    if (i != ITEM_NONE) {
        ZkndTbar_SetActive(touchbar->tbar, touchbar->items[i].icon->icon, FALSE);
    }
    ZukanDetailTouchbar_InitGlow(touchbar);
    ZukanDetailTouchbar_StartGlow(touchbar);
    if (!touchbar->showArrows) {
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_CUR_U].icon->icon, FALSE);
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_CUR_D].icon->icon, FALSE);
    }
}

static void ZukanDetailTouchbar_FreeGeneral(ZukanDetailTouchbar *touchbar) {
    if (touchbar->tbar != NULL) {
        ZukanDetailTouchbar_StopGlow(touchbar);
        ZukanDetailTouchbar_FreeGlow(touchbar);
        ZkndTbar_Free(touchbar->tbar);
        func_0204bcd0(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].palette);
        func_0204b98c(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].chars);
        func_0204be64(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_GENERAL].cellAnims);
    }
    touchbar->tbar = NULL;
}

// The left and right keys move between the pages' tabs, skipping hidden ones, and the up and down keys repeat the
// arrows
static void ZukanDetailTouchbar_UpdateGeneral(ZukanDetailTouchbar *touchbar) {
    u8 item = ITEM_NONE;
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();
    int trigger;
    int touch;

    if (typed & PAD_KEY_LEFT) {
        switch (touchbar->page) {
        case ZUKAN_DETAIL_PAGE_INFO - 1:
            if (ZkndTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_FORM].icon->icon)) {
                item = GENERAL_FORM;
            } else {
                item = GENERAL_VOICE;
            }
            break;
        case ZUKAN_DETAIL_PAGE_MAP - 1:
            item = GENERAL_INFO;
            break;
        case ZUKAN_DETAIL_PAGE_VOICE - 1:
            if (ZkndTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_MAP].icon->icon)) {
                item = GENERAL_MAP;
            } else {
                item = GENERAL_INFO;
            }
            break;
        case ZUKAN_DETAIL_PAGE_FORM - 1:
            item = GENERAL_VOICE;
            break;
        }
    } else if (typed & PAD_KEY_RIGHT) {
        switch (touchbar->page) {
        case ZUKAN_DETAIL_PAGE_INFO - 1:
            if (ZkndTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_MAP].icon->icon)) {
                item = GENERAL_MAP;
            } else {
                item = GENERAL_VOICE;
            }
            break;
        case ZUKAN_DETAIL_PAGE_MAP - 1:
            item = GENERAL_VOICE;
            break;
        case ZUKAN_DETAIL_PAGE_VOICE - 1:
            if (ZkndTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_FORM].icon->icon)) {
                item = GENERAL_FORM;
            } else {
                item = GENERAL_INFO;
            }
            break;
        case ZUKAN_DETAIL_PAGE_FORM - 1:
            item = GENERAL_INFO;
            break;
        }
    }
    if (item != ITEM_NONE) {
        ZkndTbar_Push(touchbar->tbar, touchbar->items[item].icon->icon);
    }

    if (!(pressed & (PAD_KEY_UP | PAD_KEY_DOWN))) {
        if (typed & PAD_KEY_UP) {
            ZkndTbar_Push(touchbar->tbar, touchbar->items[GENERAL_CUR_U].icon->icon);
        } else if (typed & PAD_KEY_DOWN) {
            ZkndTbar_Push(touchbar->tbar, touchbar->items[GENERAL_CUR_D].icon->icon);
        }
    }

    trigger = ZukanDetailTouchbar_GetTrigger(touchbar);
    touch = ZukanDetailTouchbar_GetTouch(touchbar);
    if (trigger != ZUKAN_DETAIL_CMD_NONE) {
        u8 pageItem = ZukanDetailTouchbar_GetCommandItem(trigger);
        u8 page = ZukanDetailTouchbar_GetCommandPage(trigger);

        if (pageItem != ITEM_NONE && page != ZUKAN_DETAIL_PAGE_FORM) {
            for (item = GENERAL_FORM; item <= GENERAL_INFO; item++) {
                ZkndTbar_SetActive(touchbar->tbar, touchbar->items[item].icon->icon, item == pageItem ? FALSE : TRUE);
            }
            touchbar->selectedPage = page;
            ZukanDetailTouchbar_StartGlow(touchbar);
        }
    }
    if (touch != ZUKAN_DETAIL_CMD_NONE) {
        switch (touch) {
        case ZUKAN_DETAIL_CMD_INFO_TOUCH:
        case ZUKAN_DETAIL_CMD_MAP_TOUCH:
        case ZUKAN_DETAIL_CMD_VOICE_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_TOUCH:
            ZukanDetailTouchbar_StopGlow(touchbar);
            break;
        }
    }
}

// Reads the two palettes that the selected tab glows between
static void ZukanDetailTouchbar_InitGlow(ZukanDetailTouchbar *touchbar) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_ZUKAN_GRA, 3, &palette, touchbar->heapId);
    u16 *data = palette->rawData;

    sys_memcpy(data + 16, touchbar->glowPalettes[0], 32);
    sys_memcpy(data + 48, touchbar->glowPalettes[1], 32);
    GFL_HeapFree(file);
    touchbar->glowPhase = 0;
}

static void ZukanDetailTouchbar_FreeGlow(ZukanDetailTouchbar *touchbar) {
}

static void ZukanDetailTouchbar_StartGlow(ZukanDetailTouchbar *touchbar) {
    u8 item = ITEM_NONE;

    switch (touchbar->selectedPage) {
    case ZUKAN_DETAIL_PAGE_INFO - 1:
        item = GENERAL_INFO;
        break;
    case ZUKAN_DETAIL_PAGE_MAP - 1:
        item = GENERAL_MAP;
        break;
    case ZUKAN_DETAIL_PAGE_VOICE - 1:
        item = GENERAL_VOICE;
        break;
    case ZUKAN_DETAIL_PAGE_FORM - 1:
        item = GENERAL_FORM;
        break;
    }
    if (item != ITEM_NONE) {
        func_0204c378(ZkndTbar_GetActor(touchbar->tbar, touchbar->items[item].icon->icon), 2, 0);
    }
}

static void ZukanDetailTouchbar_StopGlow(ZukanDetailTouchbar *touchbar) {
    u8 item = ITEM_NONE;

    switch (touchbar->selectedPage) {
    case ZUKAN_DETAIL_PAGE_INFO - 1:
        item = GENERAL_INFO;
        break;
    case ZUKAN_DETAIL_PAGE_MAP - 1:
        item = GENERAL_MAP;
        break;
    case ZUKAN_DETAIL_PAGE_VOICE - 1:
        item = GENERAL_VOICE;
        break;
    case ZUKAN_DETAIL_PAGE_FORM - 1:
        item = GENERAL_FORM;
        break;
    }
    if (item != ITEM_NONE) {
        func_0204c378(ZkndTbar_GetActor(touchbar->tbar, touchbar->items[item].icon->icon), 0, 0);
        touchbar->selectedPage = ZUKAN_DETAIL_PAGE_FORM;
    }
}

// Blends the glowing palette between the two palettes with a cosine, and loads it into OBJ palette 13
static void ZukanDetailTouchbar_UpdateGlow(ZukanDetailTouchbar *touchbar) {
    s16 ratio;
    u8 i;

    if (touchbar->glowPhase + 0x400 >= 0x10000) {
        touchbar->glowPhase = touchbar->glowPhase + 0x400 - 0x10000;
    } else {
        touchbar->glowPhase += 0x400;
    }
    ratio = (FX_CosIdx(touchbar->glowPhase) + FX32_ONE) / 2;

    for (i = 0; i < 16; i++) {
        u16 from = touchbar->glowPalettes[0][i];
        u16 to = touchbar->glowPalettes[1][i];
        u8 r0 = from & 0x1f;
        u8 g0 = (from & 0x3e0) >> 5;
        u8 b0 = (from & 0x7c00) >> 10;
        u8 r1 = to & 0x1f;
        u8 g1 = (to & 0x3e0) >> 5;
        u8 b1 = (to & 0x7c00) >> 10;
        u8 r = r0 + (((r1 - r0) * ratio) >> FX32_SHIFT);
        u8 g = g0 + (((g1 - g0) * ratio) >> FX32_SHIFT);
        u8 b = b0 + (((b1 - b0) * ratio) >> FX32_SHIFT);

        touchbar->glowPalette[i] = r | (g << 5) | (b << 10);
    }
    NNS_GfdRegisterNewVramTransferTask(14, 13 * 32, touchbar->glowPalette, 32);
}

static u8 ZukanDetailTouchbar_GetPageItem(int page) {
    switch (page) {
    case ZUKAN_DETAIL_PAGE_INFO - 1:
        return GENERAL_INFO;
    case ZUKAN_DETAIL_PAGE_MAP - 1:
        return GENERAL_MAP;
    case ZUKAN_DETAIL_PAGE_VOICE - 1:
        return GENERAL_VOICE;
    case ZUKAN_DETAIL_PAGE_FORM - 1:
        return GENERAL_FORM;
    }
    return ITEM_NONE;
}

static u8 ZukanDetailTouchbar_GetCommandItem(int command) {
    switch (command) {
    case ZUKAN_DETAIL_CMD_INFO:
    case ZUKAN_DETAIL_CMD_INFO_TOUCH:
        return GENERAL_INFO;
    case ZUKAN_DETAIL_CMD_MAP:
    case ZUKAN_DETAIL_CMD_MAP_TOUCH:
        return GENERAL_MAP;
    case ZUKAN_DETAIL_CMD_VOICE:
    case ZUKAN_DETAIL_CMD_VOICE_TOUCH:
        return GENERAL_VOICE;
    case ZUKAN_DETAIL_CMD_FORM:
    case ZUKAN_DETAIL_CMD_FORM_TOUCH:
        return GENERAL_FORM;
    }
    return ITEM_NONE;
}

static u8 ZukanDetailTouchbar_GetCommandPage(int command) {
    switch (command) {
    case ZUKAN_DETAIL_CMD_INFO:
    case ZUKAN_DETAIL_CMD_INFO_TOUCH:
        return ZUKAN_DETAIL_PAGE_INFO - 1;
    case ZUKAN_DETAIL_CMD_MAP:
    case ZUKAN_DETAIL_CMD_MAP_TOUCH:
        return ZUKAN_DETAIL_PAGE_MAP - 1;
    case ZUKAN_DETAIL_CMD_VOICE:
    case ZUKAN_DETAIL_CMD_VOICE_TOUCH:
        return ZUKAN_DETAIL_PAGE_VOICE - 1;
    case ZUKAN_DETAIL_CMD_FORM:
    case ZUKAN_DETAIL_CMD_FORM_TOUCH:
        return ZUKAN_DETAIL_PAGE_FORM - 1;
    }
    return ZUKAN_DETAIL_PAGE_FORM;
}

static void ZukanDetailTouchbar_CreateMap(ZukanDetailTouchbar *touchbar) {
    ZkndTbarParam param;
    ZkndTbarIcon icons[NELEMS(sZukanDetailTouchbarMapIcons)];
    ArcTool *arc;
    u8 i;
    ZkndTbarIcon *icon;

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].palette =
        func_0204bbb8(arc, 6, CLACT_VRAM_MAIN, 11 * 32, 0, 3, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].chars = func_0204b81c(arc, 16, FALSE, CLACT_VRAM_MAIN, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].cellAnims = func_0204bde0(arc, 30, 47, touchbar->heapId);
    GFL_ArcToolFree(arc);

    touchbar->itemCount = NELEMS(sZukanDetailTouchbarMapIcons);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].icon = &sZukanDetailTouchbarMapIcons[i];
    }
    for (i = 0; i < touchbar->itemCount; i++) {
        icons[i].icon = touchbar->items[i].icon->icon;
        icons[i].pos = touchbar->items[i].icon->pos;
        icons[i].width = touchbar->items[i].icon->width;
    }
    icon = &icons[MAP_PLACE];
    icon->chars = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].chars;
    icon->palette = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].palette;
    icon->cellAnims = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].cellAnims;
    icon->activeAnim = 6;
    icon->inactiveAnim = 7;
    icon->pushedAnim = 8;
    icon->key = PAD_BUTTON_A;
    icon->se = SEQ_SE_DECIDE1;

    sys_memset(&param, 0, sizeof(ZkndTbarParam));
    param.icons = icons;
    param.iconCount = NELEMS(sZukanDetailTouchbarMapIcons);
    param.unit = touchbar->unit;
    param.bg = 1;
    param.bgPalette = 13;
    param.objPalette = 8;
    param.mapping = 2;
    touchbar->tbar = ZkndTbar_Create(&param, touchbar->heapId);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].actor = ZkndTbar_GetActor(touchbar->tbar, touchbar->items[i].icon->icon);
    }
    ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon, TRUE);
}

static void ZukanDetailTouchbar_FreeMap(ZukanDetailTouchbar *touchbar) {
    if (touchbar->tbar != NULL) {
        ZkndTbar_Free(touchbar->tbar);
        func_0204bcd0(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].palette);
        func_0204b98c(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].chars);
        func_0204be64(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_MAP].cellAnims);
    }
    touchbar->tbar = NULL;
}

static void ZukanDetailTouchbar_CreateForm(ZukanDetailTouchbar *touchbar) {
    ZkndTbarIcon icons[NELEMS(sZukanDetailTouchbarFormIcons)];
    ZkndTbarParam param;
    ArcTool *arc;
    u8 i;
    ZkndTbarIcon *icon;

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].palette =
        func_0204bbb8(arc, 3, CLACT_VRAM_MAIN, 11 * 32, 0, 3, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].chars = func_0204b81c(arc, 13, FALSE, CLACT_VRAM_MAIN, touchbar->heapId);
    touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].cellAnims = func_0204bde0(arc, 28, 45, touchbar->heapId);
    GFL_ArcToolFree(arc);

    touchbar->itemCount = NELEMS(sZukanDetailTouchbarFormIcons);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].icon = &sZukanDetailTouchbarFormIcons[i];
    }
    for (i = 0; i < touchbar->itemCount; i++) {
        icons[i].icon = touchbar->items[i].icon->icon;
        icons[i].pos = touchbar->items[i].icon->pos;
        icons[i].width = touchbar->items[i].icon->width;
    }
    icon = &icons[FORM_BUTTON];
    icon->chars = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].chars;
    icon->palette = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].palette;
    icon->cellAnims = touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].cellAnims;
    icon->activeAnim = 14;
    icon->inactiveAnim = 14;
    icon->pushedAnim = 20;
    icon->key = PAD_BUTTON_A;
    icon->se = SEQ_SE_DECIDE1;

    sys_memset(&param, 0, sizeof(ZkndTbarParam));
    param.icons = icons;
    param.iconCount = NELEMS(sZukanDetailTouchbarFormIcons);
    param.unit = touchbar->unit;
    param.bg = 1;
    param.bgPalette = 13;
    param.objPalette = 8;
    param.mapping = 2;
    touchbar->tbar = ZkndTbar_Create(&param, touchbar->heapId);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].actor = ZkndTbar_GetActor(touchbar->tbar, touchbar->items[i].icon->icon);
    }
    ZkndTbar_SetKey(touchbar->tbar, touchbar->items[FORM_CUR_R].icon->icon, PAD_BUTTON_R | PAD_KEY_RIGHT);
    ZkndTbar_SetKey(touchbar->tbar, touchbar->items[FORM_CUR_L].icon->icon, PAD_BUTTON_L | PAD_KEY_LEFT);
    if (!touchbar->showArrows) {
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_U].icon->icon, FALSE);
        ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_D].icon->icon, FALSE);
    }
}

static void ZukanDetailTouchbar_FreeForm(ZukanDetailTouchbar *touchbar) {
    if (touchbar->tbar != NULL) {
        ZkndTbar_Free(touchbar->tbar);
        func_0204bcd0(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].palette);
        func_0204b98c(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].chars);
        func_0204be64(touchbar->res[ZUKAN_DETAIL_TOUCHBAR_FORM].cellAnims);
    }
    touchbar->tbar = NULL;
}

// The up and down keys and the arrows' keys repeat
static void ZukanDetailTouchbar_UpdateForm(ZukanDetailTouchbar *touchbar) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();

    if (!(pressed & (PAD_PLUS_KEY_MASK | PAD_BUTTON_R | PAD_BUTTON_L))) {
        if (typed & PAD_KEY_UP) {
            ZkndTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_U].icon->icon);
        } else if (typed & PAD_KEY_DOWN) {
            ZkndTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_D].icon->icon);
        } else if (typed & (PAD_BUTTON_L | PAD_KEY_LEFT)) {
            ZkndTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_L].icon->icon);
        } else if (typed & (PAD_BUTTON_R | PAD_KEY_RIGHT)) {
            ZkndTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_R].icon->icon);
        }
    }
}

void ZukanDetailTouchbar_SetMapPlaceActive(ZukanDetailTouchbar *touchbar, BOOL active) {
    ZkndTbar_SetActive(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon, active);
}

void ZukanDetailTouchbar_SetMapPlaceVisible(ZukanDetailTouchbar *touchbar, BOOL visible) {
    ZkndTbar_SetVisible(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon, visible);
}

void ZukanDetailTouchbar_PushMapPlace(ZukanDetailTouchbar *touchbar) {
    ZkndTbar_Push(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon);
}

BOOL ZukanDetailTouchbar_IsMapPlaceTriggered(ZukanDetailTouchbar *touchbar) {
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_MAP) {
        return ZkndTbar_IsTriggered(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon);
    }
    return FALSE;
}

BOOL ZukanDetailTouchbar_IsArrowTriggered(ZukanDetailTouchbar *touchbar) {
    BOOL down = ZkndTbar_IsTriggered(touchbar->tbar, touchbar->items[GENERAL_CUR_D].icon->icon);
    BOOL up = ZkndTbar_IsTriggered(touchbar->tbar, touchbar->items[GENERAL_CUR_U].icon->icon);

    if (down || up) {
        return TRUE;
    }
    return FALSE;
}

BOOL ZukanDetailTouchbar_IsFormButtonTriggered(ZukanDetailTouchbar *touchbar) {
    if (touchbar->type == ZUKAN_DETAIL_TOUCHBAR_FORM) {
        return ZkndTbar_IsTriggered(touchbar->tbar, touchbar->items[FORM_BUTTON].icon->icon);
    }
    return FALSE;
}
