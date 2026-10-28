// The ROM names this file zukan_detail_touchbar.c.
#include "types.h"
#include "app/pokedex_detail.h"
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
    ICON_INFO = POKEDEX_DETAIL_TBAR_ICON_CUSTOM,
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
} PokedexDetailTouchbarIcon;

typedef struct {
    const PokedexDetailTouchbarIcon *icon;
    ClActor *actor;
} PokedexDetailTouchbarItem;

// The resources of the Pokédex's own icons
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} PokedexDetailTouchbarRes;

struct PokedexDetailTouchbar {
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
    PokedexDetailTbar *tbar;
    BOOL active;
    u8 bgPriority;
    BOOL bgPriorityChanged;
    BOOL showArrows;
    PokedexDetailTouchbarRes res[3];
    u8 itemCount;
    PokedexDetailTouchbarItem items[ITEM_MAX];
    s16 scrollY;
    // The selected tab's palette glows between two palettes
    u16 glowPalettes[2][16];
    u16 glowPalette[16];
    int glowPhase;
};

static void PokedexDetailTouchbar_VBlank(TCB *tcb, void *data);
static void PokedexDetailTouchbar_CreateType(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_FreeType(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_SetItemsY(PokedexDetailTouchbar *touchbar, s16 offset);
static void PokedexDetailTouchbar_CreateGeneral(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_FreeGeneral(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_UpdateGeneral(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_InitGlow(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_FreeGlow(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_StartGlow(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_StopGlow(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_UpdateGlow(PokedexDetailTouchbar *touchbar);
static u8 PokedexDetailTouchbar_GetPageItem(int page);
static u8 PokedexDetailTouchbar_GetCommandItem(int command);
static u8 PokedexDetailTouchbar_GetCommandPage(int command);
static void PokedexDetailTouchbar_CreateMap(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_FreeMap(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_CreateForm(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_FreeForm(PokedexDetailTouchbar *touchbar);
static void PokedexDetailTouchbar_UpdateForm(PokedexDetailTouchbar *touchbar);

// How long the bar waits between moves, and how far it moves, at each speed
static const u8 sPokedexDetailTouchbarWaits[2] = { 0, 0 };
static const u32 sPokedexDetailTouchbarSpeeds[2] = { 3, 3 };

// Where the general icons go when the bar is in the other mode, and when the forms' tab is hidden
static const ClActorPos sPokedexDetailTouchbarNoFormPositions[ITEM_MAX] = {
    { 232, 168 }, { 208, 168 }, { 184, 172 }, { 168, 168 }, { 136, 168 },
    { 96, 168 },  { 56, 168 },  { 24, 168 },  { 0, 168 },
};

static const ClActorPos sPokedexDetailTouchbarModePositions[ITEM_MAX] = {
    { 232, 168 }, { 208, 168 }, { 184, 172 }, { 136, 168 }, { 96, 168 },
    { 96, 168 },  { 56, 168 },  { 24, 168 },  { 0, 168 },
};

static const PokedexDetailTouchbarIcon sPokedexDetailTouchbarMapIcons[] = {
    { POKEDEX_DETAIL_TBAR_ICON_RETURN,
      { 232, 168 },
      24,
      POKEDEX_DETAIL_CMD_MAP_RETURN,
      POKEDEX_DETAIL_CMD_MAP_RETURN_TOUCH },
    { ICON_MAP, { 56, 168 }, 160, POKEDEX_DETAIL_CMD_MAP_PLACE, POKEDEX_DETAIL_CMD_MAP_PLACE_TOUCH },
};

static const PokedexDetailTouchbarIcon sPokedexDetailTouchbarFormIcons[] = {
    { POKEDEX_DETAIL_TBAR_ICON_RETURN,
      { 232, 168 },
      24,
      POKEDEX_DETAIL_CMD_FORM_RETURN,
      POKEDEX_DETAIL_CMD_FORM_RETURN_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CUR_R,
      { 192, 168 },
      24,
      POKEDEX_DETAIL_CMD_FORM_CUR_R,
      POKEDEX_DETAIL_CMD_FORM_CUR_R_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CUR_L,
      { 168, 168 },
      24,
      POKEDEX_DETAIL_CMD_FORM_CUR_L,
      POKEDEX_DETAIL_CMD_FORM_CUR_L_TOUCH },
    { ICON_INFO, { 104, 168 }, 48, POKEDEX_DETAIL_CMD_FORM_BUTTON, POKEDEX_DETAIL_CMD_FORM_BUTTON_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CUR_D,
      { 24, 168 },
      24,
      POKEDEX_DETAIL_CMD_FORM_CUR_D,
      POKEDEX_DETAIL_CMD_FORM_CUR_D_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CUR_U,
      { 0, 168 },
      24,
      POKEDEX_DETAIL_CMD_FORM_CUR_U,
      POKEDEX_DETAIL_CMD_FORM_CUR_U_TOUCH },
};

static const PokedexDetailTouchbarIcon sPokedexDetailTouchbarGeneralIcons[] = {
    { POKEDEX_DETAIL_TBAR_ICON_RETURN, { 232, 168 }, 24, POKEDEX_DETAIL_CMD_RETURN, POKEDEX_DETAIL_CMD_RETURN_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CLOSE, { 208, 168 }, 24, POKEDEX_DETAIL_CMD_CLOSE, POKEDEX_DETAIL_CMD_CLOSE_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CHECK, { 184, 172 }, 24, POKEDEX_DETAIL_CMD_CHECK, POKEDEX_DETAIL_CMD_CHECK_TOUCH },
    { ICON_FORM, { 144, 168 }, 32, POKEDEX_DETAIL_CMD_FORM, POKEDEX_DETAIL_CMD_FORM_TOUCH },
    { ICON_VOICE, { 112, 168 }, 32, POKEDEX_DETAIL_CMD_VOICE, POKEDEX_DETAIL_CMD_VOICE_TOUCH },
    { ICON_MAP, { 80, 168 }, 32, POKEDEX_DETAIL_CMD_MAP, POKEDEX_DETAIL_CMD_MAP_TOUCH },
    { ICON_INFO, { 48, 168 }, 32, POKEDEX_DETAIL_CMD_INFO, POKEDEX_DETAIL_CMD_INFO_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CUR_D, { 24, 168 }, 24, POKEDEX_DETAIL_CMD_CUR_D, POKEDEX_DETAIL_CMD_CUR_D_TOUCH },
    { POKEDEX_DETAIL_TBAR_ICON_CUR_U, { 0, 168 }, 24, POKEDEX_DETAIL_CMD_CUR_U, POKEDEX_DETAIL_CMD_CUR_U_TOUCH },
};

PokedexDetailTouchbar *PokedexDetailTouchbar_Create(HeapID heapId, BOOL showFormTab, u32 mode) {
    PokedexDetailTouchbar *touchbar =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailTouchbar), TRUE, "zukan_detail_touchbar.c", 342);

    touchbar->heapId = heapId;
    touchbar->showFormTab = showFormTab;
    touchbar->mode = mode;
    touchbar->state = POKEDEX_DETAIL_TOUCHBAR_HIDDEN;
    touchbar->type = POKEDEX_DETAIL_TOUCHBAR_GENERAL;
    touchbar->page = POKEDEX_DETAIL_PAGE_INFO - 1;
    touchbar->selectedPage = POKEDEX_DETAIL_PAGE_INFO - 1;
    touchbar->speed = 0;
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, -HIDDEN_Y);
    GFL_BGSysSetBGPriority(1, 0);
    touchbar->bgPriority = 0;
    touchbar->bgPriorityChanged = FALSE;
    touchbar->unit = ClActUnit_Create(16, 0, touchbar->heapId);
    ClActUnit_UseSystemRenderer(touchbar->unit);
    touchbar->tbar = NULL;
    PokedexDetailTouchbar_CreateType(touchbar);
    PokedexDetailTbar_SetActiveAll(touchbar->tbar, FALSE);
    touchbar->scrollY = HIDDEN_Y;
    PokedexDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
    touchbar->vblankTcb = GFL_VBlankTCBAdd(PokedexDetailTouchbar_VBlank, touchbar, 1);
    return touchbar;
}

void PokedexDetailTouchbar_Free(PokedexDetailTouchbar *touchbar) {
    GFL_TCBRemove(touchbar->vblankTcb);
    PokedexDetailTouchbar_FreeType(touchbar);
    ClActUnit_Delete(touchbar->unit);
    GFL_HeapFree(touchbar);
}

void PokedexDetailTouchbar_Update(PokedexDetailTouchbar *touchbar) {
    switch (touchbar->state) {
    case POKEDEX_DETAIL_TOUCHBAR_HIDDEN:
    case POKEDEX_DETAIL_TOUCHBAR_SHOWN:
        break;
    case POKEDEX_DETAIL_TOUCHBAR_APPEARING:
        if (touchbar->wait == 0) {
            if (GFL_BGSysGetBGOffsetY(1) >= 0) {
                GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, 0);
                touchbar->scrollY = 0;
                PokedexDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->state = POKEDEX_DETAIL_TOUCHBAR_SHOWN;
                if (touchbar->active) {
                    PokedexDetailTbar_SetActiveAll(touchbar->tbar, TRUE);
                }
            } else {
                GFL_BGSysMoveBGReq(1, BG_MOVE_DOWN, sPokedexDetailTouchbarSpeeds[touchbar->speed]);
                touchbar->scrollY = touchbar->scrollY - (s16)sPokedexDetailTouchbarSpeeds[touchbar->speed];
                PokedexDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->wait = sPokedexDetailTouchbarWaits[touchbar->speed];
            }
        } else {
            touchbar->wait--;
        }
        break;
    case POKEDEX_DETAIL_TOUCHBAR_DISAPPEARING:
        if (touchbar->wait == 0) {
            if (GFL_BGSysGetBGOffsetY(1) <= -HIDDEN_Y) {
                GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, -HIDDEN_Y);
                touchbar->scrollY = HIDDEN_Y;
                PokedexDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->state = POKEDEX_DETAIL_TOUCHBAR_HIDDEN;
            } else {
                GFL_BGSysMoveBGReq(1, BG_MOVE_UP, sPokedexDetailTouchbarSpeeds[touchbar->speed]);
                touchbar->scrollY = touchbar->scrollY + (s16)sPokedexDetailTouchbarSpeeds[touchbar->speed];
                PokedexDetailTouchbar_SetItemsY(touchbar, touchbar->scrollY);
                touchbar->wait = sPokedexDetailTouchbarWaits[touchbar->speed];
            }
        } else {
            touchbar->wait--;
        }
        break;
    }

    PokedexDetailTbar_Main(touchbar->tbar);
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_GENERAL) {
        PokedexDetailTouchbar_UpdateGeneral(touchbar);
        PokedexDetailTouchbar_UpdateGlow(touchbar);
    } else if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_FORM) {
        PokedexDetailTouchbar_UpdateForm(touchbar);
    }
}

void PokedexDetailTouchbar_SetType(PokedexDetailTouchbar *touchbar, int type, int page, BOOL showArrows) {
    PokedexDetailTouchbar_FreeType(touchbar);
    touchbar->type = type;
    touchbar->page = page;
    touchbar->showArrows = showArrows;
    touchbar->selectedPage = page;
    PokedexDetailTouchbar_CreateType(touchbar);
}

int PokedexDetailTouchbar_GetState(PokedexDetailTouchbar *touchbar) {
    return touchbar->state;
}

void PokedexDetailTouchbar_Appear(PokedexDetailTouchbar *touchbar, u32 speed) {
    touchbar->speed = speed;
    if (touchbar->state != POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
        touchbar->state = POKEDEX_DETAIL_TOUCHBAR_APPEARING;
        touchbar->wait = 0;
    }
}

void PokedexDetailTouchbar_Disappear(PokedexDetailTouchbar *touchbar, u32 speed) {
    touchbar->speed = speed;
    if (touchbar->state != POKEDEX_DETAIL_TOUCHBAR_HIDDEN) {
        touchbar->state = POKEDEX_DETAIL_TOUCHBAR_DISAPPEARING;
        touchbar->wait = 0;
        PokedexDetailTbar_SetActiveAll(touchbar->tbar, FALSE);
    }
}

int PokedexDetailTouchbar_GetTrigger(PokedexDetailTouchbar *touchbar) {
    int command;
    int icon;
    u8 i;

    icon = PokedexDetailTbar_GetTrigger(touchbar->tbar);
    command = POKEDEX_DETAIL_CMD_NONE;
    if (icon != -1) {
        switch (touchbar->type) {
        case POKEDEX_DETAIL_TOUCHBAR_GENERAL:
        case POKEDEX_DETAIL_TOUCHBAR_MAP:
        case POKEDEX_DETAIL_TOUCHBAR_FORM:
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

int PokedexDetailTouchbar_GetTouch(PokedexDetailTouchbar *touchbar) {
    int command;
    int icon;
    u8 i;

    icon = PokedexDetailTbar_GetTouch(touchbar->tbar);
    command = POKEDEX_DETAIL_CMD_NONE;
    if (icon != -1) {
        switch (touchbar->type) {
        case POKEDEX_DETAIL_TOUCHBAR_GENERAL:
        case POKEDEX_DETAIL_TOUCHBAR_MAP:
        case POKEDEX_DETAIL_TOUCHBAR_FORM:
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

void PokedexDetailTouchbar_Unlock(PokedexDetailTouchbar *touchbar) {
    PokedexDetailTbar_Unlock(touchbar->tbar);
}

void PokedexDetailTouchbar_SetVisibleAll(PokedexDetailTouchbar *touchbar, BOOL visible) {
    PokedexDetailTbar_SetVisibleAll(touchbar->tbar, visible);
}

void PokedexDetailTouchbar_SetPage(PokedexDetailTouchbar *touchbar, int page) {
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_GENERAL) {
        BOOL restore = FALSE;
        int selectedPage;

        touchbar->page = page;
        if (page == touchbar->selectedPage) {
            selectedPage = touchbar->selectedPage;
            restore = TRUE;
            touchbar->selectedPage = POKEDEX_DETAIL_PAGE_FORM;
        }
        PokedexDetailTouchbar_StopGlow(touchbar);
        PokedexDetailTouchbar_StartGlow(touchbar);
        if (restore) {
            touchbar->selectedPage = selectedPage;
        }
    }
}

void PokedexDetailTouchbar_SetFormArrowsVisible(PokedexDetailTouchbar *touchbar, BOOL visible) {
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_FORM) {
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_R].icon->icon, visible);
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_L].icon->icon, visible);
    }
}

void PokedexDetailTouchbar_SetCheck(PokedexDetailTouchbar *touchbar, BOOL check) {
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_GENERAL) {
        PokedexDetailTbar_SetFlip(touchbar->tbar, touchbar->items[GENERAL_CHECK].icon->icon, check);
    }
}

BOOL PokedexDetailTouchbar_GetCheck(PokedexDetailTouchbar *touchbar) {
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_GENERAL) {
        return PokedexDetailTbar_GetFlip(touchbar->tbar, touchbar->items[GENERAL_CHECK].icon->icon);
    }
    return FALSE;
}

void PokedexDetailTouchbar_SetActive(PokedexDetailTouchbar *touchbar, BOOL active) {
    touchbar->active = active;
    if (active) {
        if (!PokedexDetailTbar_GetActiveAll(touchbar->tbar) && touchbar->state == POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
            PokedexDetailTbar_SetActiveAll(touchbar->tbar, TRUE);
        }
    } else {
        if (PokedexDetailTbar_GetActiveAll(touchbar->tbar)) {
            PokedexDetailTbar_SetActiveAll(touchbar->tbar, FALSE);
        }
    }
}

// The palette of the Pokédex's own icons
u32 PokedexDetailTouchbar_GetIconPalette(PokedexDetailTouchbar *touchbar) {
    switch (touchbar->type) {
    case POKEDEX_DETAIL_TOUCHBAR_GENERAL:
        return touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette;
    case POKEDEX_DETAIL_TOUCHBAR_MAP:
        return touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].palette;
    case POKEDEX_DETAIL_TOUCHBAR_FORM:
        return touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].palette;
    }
    return -1;
}

// Sets the priority of the bar's BG at the next VBlank, and the icons' with it
void PokedexDetailTouchbar_SetBGPriority(PokedexDetailTouchbar *touchbar, u8 priority) {
    u8 i;

    touchbar->bgPriority = priority;
    touchbar->bgPriorityChanged = TRUE;
    for (i = 0; i < touchbar->itemCount; i++) {
        ClActor_SetBgPriority(touchbar->items[i].actor, priority);
    }
}

static void PokedexDetailTouchbar_VBlank(TCB *tcb, void *data) {
    PokedexDetailTouchbar *touchbar = data;

    if (touchbar->bgPriorityChanged) {
        GFL_BGSysSetBGPriority(1, touchbar->bgPriority);
        touchbar->bgPriorityChanged = FALSE;
    }
}

static void PokedexDetailTouchbar_CreateType(PokedexDetailTouchbar *touchbar) {
    switch (touchbar->type) {
    case POKEDEX_DETAIL_TOUCHBAR_GENERAL:
        PokedexDetailTouchbar_CreateGeneral(touchbar);
        break;
    case POKEDEX_DETAIL_TOUCHBAR_MAP:
        PokedexDetailTouchbar_CreateMap(touchbar);
        break;
    case POKEDEX_DETAIL_TOUCHBAR_FORM:
        PokedexDetailTouchbar_CreateForm(touchbar);
        break;
    }
    touchbar->active = TRUE;
}

static void PokedexDetailTouchbar_FreeType(PokedexDetailTouchbar *touchbar) {
    switch (touchbar->type) {
    case POKEDEX_DETAIL_TOUCHBAR_GENERAL:
        PokedexDetailTouchbar_FreeGeneral(touchbar);
        break;
    case POKEDEX_DETAIL_TOUCHBAR_MAP:
        PokedexDetailTouchbar_FreeMap(touchbar);
        break;
    case POKEDEX_DETAIL_TOUCHBAR_FORM:
        PokedexDetailTouchbar_FreeForm(touchbar);
        break;
    }
}

// Moves the icons with the bar's BG
static void PokedexDetailTouchbar_SetItemsY(PokedexDetailTouchbar *touchbar, s16 offset) {
    u8 i;
    ClActorPos pos;

    for (i = 0; i < touchbar->itemCount; i++) {
        ClActor_GetPos(touchbar->items[i].actor, &pos);
        pos.y = touchbar->items[i].icon->pos.y;
        pos.y += offset;
        ClActor_SetPos(touchbar->items[i].actor, &pos);
    }
}

static void PokedexDetailTouchbar_CreateGeneral(PokedexDetailTouchbar *touchbar) {
    PokedexDetailTbarIcon icons[ITEM_MAX];
    PokedexDetailTbarParam param;
    ArcTool *arc;
    u8 i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette =
        ClActSys_LoadPlttRange(arc, 3, CLACT_VRAM_MAIN, 11 * 32, 0, 3, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].chars =
        ClActSys_LoadChars(arc, 13, FALSE, CLACT_VRAM_MAIN, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].cellAnims = ClActSys_LoadCellAnims(arc, 28, 45, touchbar->heapId);
    GFL_ArcToolFree(arc);

    touchbar->itemCount = NELEMS(sPokedexDetailTouchbarGeneralIcons);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].icon = &sPokedexDetailTouchbarGeneralIcons[i];
    }
    for (i = 0; i < touchbar->itemCount; i++) {
        icons[i].icon = touchbar->items[i].icon->icon;
        icons[i].pos = touchbar->items[i].icon->pos;
        icons[i].width = touchbar->items[i].icon->width;
    }

    icons[GENERAL_FORM].chars = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_FORM].palette = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_FORM].cellAnims = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_FORM].activeAnim = 3;
    icons[GENERAL_FORM].inactiveAnim = 3;
    icons[GENERAL_FORM].pushedAnim = 7;
    icons[GENERAL_FORM].key = 0;
    icons[GENERAL_FORM].se = SEQ_SE_SELECT3;
    icons[GENERAL_VOICE].chars = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_VOICE].palette = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_VOICE].cellAnims = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_VOICE].activeAnim = 2;
    icons[GENERAL_VOICE].inactiveAnim = 2;
    icons[GENERAL_VOICE].pushedAnim = 6;
    icons[GENERAL_VOICE].key = 0;
    icons[GENERAL_VOICE].se = SEQ_SE_SELECT3;
    icons[GENERAL_MAP].chars = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_MAP].palette = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_MAP].cellAnims = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_MAP].activeAnim = 1;
    icons[GENERAL_MAP].inactiveAnim = 1;
    icons[GENERAL_MAP].pushedAnim = 5;
    icons[GENERAL_MAP].key = 0;
    icons[GENERAL_MAP].se = SEQ_SE_SELECT3;
    icons[GENERAL_INFO].chars = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].chars;
    icons[GENERAL_INFO].palette = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette;
    icons[GENERAL_INFO].cellAnims = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].cellAnims;
    icons[GENERAL_INFO].activeAnim = 0;
    icons[GENERAL_INFO].inactiveAnim = 0;
    icons[GENERAL_INFO].pushedAnim = 4;
    icons[GENERAL_INFO].key = 0;
    icons[GENERAL_INFO].se = SEQ_SE_SELECT3;

    MI_CpuFill8(&param, 0, sizeof(PokedexDetailTbarParam));
    param.icons = icons;
    param.iconCount = NELEMS(sPokedexDetailTouchbarGeneralIcons);
    param.unit = touchbar->unit;
    param.bg = 1;
    param.bgPalette = 13;
    param.objPalette = 8;
    param.mapping = 2;
    touchbar->tbar = PokedexDetailTbar_Create(&param, touchbar->heapId);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].actor = PokedexDetailTbar_GetActor(touchbar->tbar, touchbar->items[i].icon->icon);
    }

    if (!touchbar->showFormTab) {
        for (i = 0; i < ITEM_MAX; i++) {
            PokedexDetailTbar_SetPos(touchbar->tbar, touchbar->items[i].icon->icon,
                                     &sPokedexDetailTouchbarNoFormPositions[i]);
        }
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_FORM].icon->icon, FALSE);
    }
    if (touchbar->mode != 0) {
        for (i = 0; i < ITEM_MAX; i++) {
            PokedexDetailTbar_SetPos(touchbar->tbar, touchbar->items[i].icon->icon,
                                     &sPokedexDetailTouchbarModePositions[i]);
        }
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_MAP].icon->icon, FALSE);
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_CHECK].icon->icon, FALSE);
    }

    i = PokedexDetailTouchbar_GetPageItem(touchbar->page);
    if (i != ITEM_NONE) {
        PokedexDetailTbar_SetActive(touchbar->tbar, touchbar->items[i].icon->icon, FALSE);
    }
    PokedexDetailTouchbar_InitGlow(touchbar);
    PokedexDetailTouchbar_StartGlow(touchbar);
    if (!touchbar->showArrows) {
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_CUR_U].icon->icon, FALSE);
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[GENERAL_CUR_D].icon->icon, FALSE);
    }
}

static void PokedexDetailTouchbar_FreeGeneral(PokedexDetailTouchbar *touchbar) {
    if (touchbar->tbar != NULL) {
        PokedexDetailTouchbar_StopGlow(touchbar);
        PokedexDetailTouchbar_FreeGlow(touchbar);
        PokedexDetailTbar_Free(touchbar->tbar);
        ClActSys_FreePltt(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].palette);
        ClActSys_FreeChars(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].chars);
        ClActSys_FreeCellAnims(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_GENERAL].cellAnims);
    }
    touchbar->tbar = NULL;
}

// The left and right keys move between the pages' tabs, skipping hidden ones, and the up and down keys repeat the
// arrows
static void PokedexDetailTouchbar_UpdateGeneral(PokedexDetailTouchbar *touchbar) {
    u8 item = ITEM_NONE;
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();
    int trigger;
    int touch;

    if (typed & PAD_KEY_LEFT) {
        switch (touchbar->page) {
        case POKEDEX_DETAIL_PAGE_INFO - 1:
            if (PokedexDetailTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_FORM].icon->icon)) {
                item = GENERAL_FORM;
            } else {
                item = GENERAL_VOICE;
            }
            break;
        case POKEDEX_DETAIL_PAGE_MAP - 1:
            item = GENERAL_INFO;
            break;
        case POKEDEX_DETAIL_PAGE_VOICE - 1:
            if (PokedexDetailTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_MAP].icon->icon)) {
                item = GENERAL_MAP;
            } else {
                item = GENERAL_INFO;
            }
            break;
        case POKEDEX_DETAIL_PAGE_FORM - 1:
            item = GENERAL_VOICE;
            break;
        }
    } else if (typed & PAD_KEY_RIGHT) {
        switch (touchbar->page) {
        case POKEDEX_DETAIL_PAGE_INFO - 1:
            if (PokedexDetailTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_MAP].icon->icon)) {
                item = GENERAL_MAP;
            } else {
                item = GENERAL_VOICE;
            }
            break;
        case POKEDEX_DETAIL_PAGE_MAP - 1:
            item = GENERAL_VOICE;
            break;
        case POKEDEX_DETAIL_PAGE_VOICE - 1:
            if (PokedexDetailTbar_GetVisible(touchbar->tbar, touchbar->items[GENERAL_FORM].icon->icon)) {
                item = GENERAL_FORM;
            } else {
                item = GENERAL_INFO;
            }
            break;
        case POKEDEX_DETAIL_PAGE_FORM - 1:
            item = GENERAL_INFO;
            break;
        }
    }
    if (item != ITEM_NONE) {
        PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[item].icon->icon);
    }

    if (!(pressed & (PAD_KEY_UP | PAD_KEY_DOWN))) {
        if (typed & PAD_KEY_UP) {
            PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[GENERAL_CUR_U].icon->icon);
        } else if (typed & PAD_KEY_DOWN) {
            PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[GENERAL_CUR_D].icon->icon);
        }
    }

    trigger = PokedexDetailTouchbar_GetTrigger(touchbar);
    touch = PokedexDetailTouchbar_GetTouch(touchbar);
    if (trigger != POKEDEX_DETAIL_CMD_NONE) {
        u8 pageItem = PokedexDetailTouchbar_GetCommandItem(trigger);
        u8 page = PokedexDetailTouchbar_GetCommandPage(trigger);

        if (pageItem != ITEM_NONE && page != POKEDEX_DETAIL_PAGE_FORM) {
            for (item = GENERAL_FORM; item <= GENERAL_INFO; item++) {
                PokedexDetailTbar_SetActive(touchbar->tbar, touchbar->items[item].icon->icon,
                                            item == pageItem ? FALSE : TRUE);
            }
            touchbar->selectedPage = page;
            PokedexDetailTouchbar_StartGlow(touchbar);
        }
    }
    if (touch != POKEDEX_DETAIL_CMD_NONE) {
        switch (touch) {
        case POKEDEX_DETAIL_CMD_INFO_TOUCH:
        case POKEDEX_DETAIL_CMD_MAP_TOUCH:
        case POKEDEX_DETAIL_CMD_VOICE_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_TOUCH:
            PokedexDetailTouchbar_StopGlow(touchbar);
            break;
        }
    }
}

// Reads the two palettes that the selected tab glows between
static void PokedexDetailTouchbar_InitGlow(PokedexDetailTouchbar *touchbar) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_POKEDEX_GRA, 3, &palette, touchbar->heapId);
    u16 *data = palette->pRawData;

    MI_CpuCopy8(data + 16, touchbar->glowPalettes[0], 32);
    MI_CpuCopy8(data + 48, touchbar->glowPalettes[1], 32);
    GFL_HeapFree(file);
    touchbar->glowPhase = 0;
}

static void PokedexDetailTouchbar_FreeGlow(PokedexDetailTouchbar *touchbar) {
}

static void PokedexDetailTouchbar_StartGlow(PokedexDetailTouchbar *touchbar) {
    u8 item = ITEM_NONE;

    switch (touchbar->selectedPage) {
    case POKEDEX_DETAIL_PAGE_INFO - 1:
        item = GENERAL_INFO;
        break;
    case POKEDEX_DETAIL_PAGE_MAP - 1:
        item = GENERAL_MAP;
        break;
    case POKEDEX_DETAIL_PAGE_VOICE - 1:
        item = GENERAL_VOICE;
        break;
    case POKEDEX_DETAIL_PAGE_FORM - 1:
        item = GENERAL_FORM;
        break;
    }
    if (item != ITEM_NONE) {
        ClActor_SetPalette(PokedexDetailTbar_GetActor(touchbar->tbar, touchbar->items[item].icon->icon), 2, 0);
    }
}

static void PokedexDetailTouchbar_StopGlow(PokedexDetailTouchbar *touchbar) {
    u8 item = ITEM_NONE;

    switch (touchbar->selectedPage) {
    case POKEDEX_DETAIL_PAGE_INFO - 1:
        item = GENERAL_INFO;
        break;
    case POKEDEX_DETAIL_PAGE_MAP - 1:
        item = GENERAL_MAP;
        break;
    case POKEDEX_DETAIL_PAGE_VOICE - 1:
        item = GENERAL_VOICE;
        break;
    case POKEDEX_DETAIL_PAGE_FORM - 1:
        item = GENERAL_FORM;
        break;
    }
    if (item != ITEM_NONE) {
        ClActor_SetPalette(PokedexDetailTbar_GetActor(touchbar->tbar, touchbar->items[item].icon->icon), 0, 0);
        touchbar->selectedPage = POKEDEX_DETAIL_PAGE_FORM;
    }
}

// Blends the glowing palette between the two palettes with a cosine, and loads it into OBJ palette 13
static void PokedexDetailTouchbar_UpdateGlow(PokedexDetailTouchbar *touchbar) {
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
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, 13 * 32, touchbar->glowPalette, 32);
}

static u8 PokedexDetailTouchbar_GetPageItem(int page) {
    switch (page) {
    case POKEDEX_DETAIL_PAGE_INFO - 1:
        return GENERAL_INFO;
    case POKEDEX_DETAIL_PAGE_MAP - 1:
        return GENERAL_MAP;
    case POKEDEX_DETAIL_PAGE_VOICE - 1:
        return GENERAL_VOICE;
    case POKEDEX_DETAIL_PAGE_FORM - 1:
        return GENERAL_FORM;
    }
    return ITEM_NONE;
}

static u8 PokedexDetailTouchbar_GetCommandItem(int command) {
    switch (command) {
    case POKEDEX_DETAIL_CMD_INFO:
    case POKEDEX_DETAIL_CMD_INFO_TOUCH:
        return GENERAL_INFO;
    case POKEDEX_DETAIL_CMD_MAP:
    case POKEDEX_DETAIL_CMD_MAP_TOUCH:
        return GENERAL_MAP;
    case POKEDEX_DETAIL_CMD_VOICE:
    case POKEDEX_DETAIL_CMD_VOICE_TOUCH:
        return GENERAL_VOICE;
    case POKEDEX_DETAIL_CMD_FORM:
    case POKEDEX_DETAIL_CMD_FORM_TOUCH:
        return GENERAL_FORM;
    }
    return ITEM_NONE;
}

static u8 PokedexDetailTouchbar_GetCommandPage(int command) {
    switch (command) {
    case POKEDEX_DETAIL_CMD_INFO:
    case POKEDEX_DETAIL_CMD_INFO_TOUCH:
        return POKEDEX_DETAIL_PAGE_INFO - 1;
    case POKEDEX_DETAIL_CMD_MAP:
    case POKEDEX_DETAIL_CMD_MAP_TOUCH:
        return POKEDEX_DETAIL_PAGE_MAP - 1;
    case POKEDEX_DETAIL_CMD_VOICE:
    case POKEDEX_DETAIL_CMD_VOICE_TOUCH:
        return POKEDEX_DETAIL_PAGE_VOICE - 1;
    case POKEDEX_DETAIL_CMD_FORM:
    case POKEDEX_DETAIL_CMD_FORM_TOUCH:
        return POKEDEX_DETAIL_PAGE_FORM - 1;
    }
    return POKEDEX_DETAIL_PAGE_FORM;
}

static void PokedexDetailTouchbar_CreateMap(PokedexDetailTouchbar *touchbar) {
    PokedexDetailTbarParam param;
    PokedexDetailTbarIcon icons[NELEMS(sPokedexDetailTouchbarMapIcons)];
    ArcTool *arc;
    u8 i;
    PokedexDetailTbarIcon *icon;

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].palette =
        ClActSys_LoadPlttRange(arc, 6, CLACT_VRAM_MAIN, 11 * 32, 0, 3, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].chars =
        ClActSys_LoadChars(arc, 16, FALSE, CLACT_VRAM_MAIN, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].cellAnims = ClActSys_LoadCellAnims(arc, 30, 47, touchbar->heapId);
    GFL_ArcToolFree(arc);

    touchbar->itemCount = NELEMS(sPokedexDetailTouchbarMapIcons);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].icon = &sPokedexDetailTouchbarMapIcons[i];
    }
    for (i = 0; i < touchbar->itemCount; i++) {
        icons[i].icon = touchbar->items[i].icon->icon;
        icons[i].pos = touchbar->items[i].icon->pos;
        icons[i].width = touchbar->items[i].icon->width;
    }
    icon = &icons[MAP_PLACE];
    icon->chars = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].chars;
    icon->palette = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].palette;
    icon->cellAnims = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].cellAnims;
    icon->activeAnim = 6;
    icon->inactiveAnim = 7;
    icon->pushedAnim = 8;
    icon->key = PAD_BUTTON_A;
    icon->se = SEQ_SE_DECIDE1;

    MI_CpuFill8(&param, 0, sizeof(PokedexDetailTbarParam));
    param.icons = icons;
    param.iconCount = NELEMS(sPokedexDetailTouchbarMapIcons);
    param.unit = touchbar->unit;
    param.bg = 1;
    param.bgPalette = 13;
    param.objPalette = 8;
    param.mapping = 2;
    touchbar->tbar = PokedexDetailTbar_Create(&param, touchbar->heapId);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].actor = PokedexDetailTbar_GetActor(touchbar->tbar, touchbar->items[i].icon->icon);
    }
    PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon, TRUE);
}

static void PokedexDetailTouchbar_FreeMap(PokedexDetailTouchbar *touchbar) {
    if (touchbar->tbar != NULL) {
        PokedexDetailTbar_Free(touchbar->tbar);
        ClActSys_FreePltt(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].palette);
        ClActSys_FreeChars(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].chars);
        ClActSys_FreeCellAnims(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_MAP].cellAnims);
    }
    touchbar->tbar = NULL;
}

static void PokedexDetailTouchbar_CreateForm(PokedexDetailTouchbar *touchbar) {
    PokedexDetailTbarIcon icons[NELEMS(sPokedexDetailTouchbarFormIcons)];
    PokedexDetailTbarParam param;
    ArcTool *arc;
    u8 i;
    PokedexDetailTbarIcon *icon;

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].palette =
        ClActSys_LoadPlttRange(arc, 3, CLACT_VRAM_MAIN, 11 * 32, 0, 3, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].chars =
        ClActSys_LoadChars(arc, 13, FALSE, CLACT_VRAM_MAIN, touchbar->heapId);
    touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].cellAnims = ClActSys_LoadCellAnims(arc, 28, 45, touchbar->heapId);
    GFL_ArcToolFree(arc);

    touchbar->itemCount = NELEMS(sPokedexDetailTouchbarFormIcons);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].icon = &sPokedexDetailTouchbarFormIcons[i];
    }
    for (i = 0; i < touchbar->itemCount; i++) {
        icons[i].icon = touchbar->items[i].icon->icon;
        icons[i].pos = touchbar->items[i].icon->pos;
        icons[i].width = touchbar->items[i].icon->width;
    }
    icon = &icons[FORM_BUTTON];
    icon->chars = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].chars;
    icon->palette = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].palette;
    icon->cellAnims = touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].cellAnims;
    icon->activeAnim = 14;
    icon->inactiveAnim = 14;
    icon->pushedAnim = 20;
    icon->key = PAD_BUTTON_A;
    icon->se = SEQ_SE_DECIDE1;

    MI_CpuFill8(&param, 0, sizeof(PokedexDetailTbarParam));
    param.icons = icons;
    param.iconCount = NELEMS(sPokedexDetailTouchbarFormIcons);
    param.unit = touchbar->unit;
    param.bg = 1;
    param.bgPalette = 13;
    param.objPalette = 8;
    param.mapping = 2;
    touchbar->tbar = PokedexDetailTbar_Create(&param, touchbar->heapId);
    for (i = 0; i < touchbar->itemCount; i++) {
        touchbar->items[i].actor = PokedexDetailTbar_GetActor(touchbar->tbar, touchbar->items[i].icon->icon);
    }
    PokedexDetailTbar_SetKey(touchbar->tbar, touchbar->items[FORM_CUR_R].icon->icon, PAD_BUTTON_R | PAD_KEY_RIGHT);
    PokedexDetailTbar_SetKey(touchbar->tbar, touchbar->items[FORM_CUR_L].icon->icon, PAD_BUTTON_L | PAD_KEY_LEFT);
    if (!touchbar->showArrows) {
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_U].icon->icon, FALSE);
        PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[FORM_CUR_D].icon->icon, FALSE);
    }
}

static void PokedexDetailTouchbar_FreeForm(PokedexDetailTouchbar *touchbar) {
    if (touchbar->tbar != NULL) {
        PokedexDetailTbar_Free(touchbar->tbar);
        ClActSys_FreePltt(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].palette);
        ClActSys_FreeChars(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].chars);
        ClActSys_FreeCellAnims(touchbar->res[POKEDEX_DETAIL_TOUCHBAR_FORM].cellAnims);
    }
    touchbar->tbar = NULL;
}

// The up and down keys and the arrows' keys repeat
static void PokedexDetailTouchbar_UpdateForm(PokedexDetailTouchbar *touchbar) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();

    if (!(pressed & (PAD_PLUS_KEY_MASK | PAD_BUTTON_R | PAD_BUTTON_L))) {
        if (typed & PAD_KEY_UP) {
            PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_U].icon->icon);
        } else if (typed & PAD_KEY_DOWN) {
            PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_D].icon->icon);
        } else if (typed & (PAD_BUTTON_L | PAD_KEY_LEFT)) {
            PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_L].icon->icon);
        } else if (typed & (PAD_BUTTON_R | PAD_KEY_RIGHT)) {
            PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[FORM_CUR_R].icon->icon);
        }
    }
}

void PokedexDetailTouchbar_SetMapPlaceActive(PokedexDetailTouchbar *touchbar, BOOL active) {
    PokedexDetailTbar_SetActive(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon, active);
}

void PokedexDetailTouchbar_SetMapPlaceVisible(PokedexDetailTouchbar *touchbar, BOOL visible) {
    PokedexDetailTbar_SetVisible(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon, visible);
}

void PokedexDetailTouchbar_PushMapPlace(PokedexDetailTouchbar *touchbar) {
    PokedexDetailTbar_Push(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon);
}

BOOL PokedexDetailTouchbar_IsMapPlaceTriggered(PokedexDetailTouchbar *touchbar) {
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_MAP) {
        return PokedexDetailTbar_IsTriggered(touchbar->tbar, touchbar->items[MAP_PLACE].icon->icon);
    }
    return FALSE;
}

BOOL PokedexDetailTouchbar_IsArrowTriggered(PokedexDetailTouchbar *touchbar) {
    BOOL down = PokedexDetailTbar_IsTriggered(touchbar->tbar, touchbar->items[GENERAL_CUR_D].icon->icon);
    BOOL up = PokedexDetailTbar_IsTriggered(touchbar->tbar, touchbar->items[GENERAL_CUR_U].icon->icon);

    if (down || up) {
        return TRUE;
    }
    return FALSE;
}

BOOL PokedexDetailTouchbar_IsFormButtonTriggered(PokedexDetailTouchbar *touchbar) {
    if (touchbar->type == POKEDEX_DETAIL_TOUCHBAR_FORM) {
        return PokedexDetailTbar_IsTriggered(touchbar->tbar, touchbar->items[FORM_BUTTON].icon->icon);
    }
    return FALSE;
}
