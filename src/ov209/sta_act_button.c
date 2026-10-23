#include "types.h"
#include "app/musical/sta_act_button.h"
#include "app/musical/sta_acting.h"
#include "constants/arc.h"
#include "field/musical.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"

// Overlay 209's sta_act_button.c: the buttons of the props in the player's Pokémon's hands, which show the prop's
// texture copied into the button's characters

// The equip positions of the props in the hands
#define STA_ACT_BUTTON_EQUIP_LEFT 7
#define STA_ACT_BUTTON_EQUIP_RIGHT 8

static void StaActButton_InitGraphic(StaActButton *sys);
static void StaActButton_LoadItemTexture(StaActButton *sys, u8 index, u16 itemId, u32 size);

StaActButton *StaActButton_InitSystem(HeapID heapId, StaActing *stage, MusicalPoke *poke) {
    StaActButton *sys = GFL_HeapAllocate(heapId, sizeof(StaActButton), FALSE, "sta_act_button.c", 87);

    sys->heapId = heapId;
    sys->stage = stage;
    sys->poke = poke;
    sys->active = FALSE;
    sys->pressed = FALSE;
    sys->selected = STA_ACT_BUTTON_NONE;
    sys->itemIds[0] = poke->equips[STA_ACT_BUTTON_EQUIP_LEFT].itemId;
    sys->itemIds[1] = sys->poke->equips[STA_ACT_BUTTON_EQUIP_RIGHT].itemId;
    sys->used[0] = FALSE;
    sys->used[1] = FALSE;
    StaActButton_InitGraphic(sys);
    GXS_SetVisibleWnd(GX_WNDMASK_W0);
    G2S_SetWnd0InsidePlane(GX_PLANEMASK_ALL, TRUE);
    G2S_SetWndOutsidePlane(GX_PLANEMASK_ALL, FALSE);
    G2S_SetWnd0Position(0, 128, 255, 192);
    return sys;
}

void StaActButton_TermSystem(StaActButton *sys) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    if (sys->itemIds[0] != 0xff) {
        ClActor_Delete(sys->buttons[0]);
    }
    if (sys->itemIds[1] != 0xff) {
        ClActor_Delete(sys->buttons[1]);
    }
    ClActor_Delete(sys->effect);
    ClActSys_FreePltt(sys->palettes[0]);
    ClActSys_FreeChars(sys->chars[0]);
    ClActSys_FreeCellAnims(sys->cellAnims[0]);
    ClActSys_FreePltt(sys->palettes[1]);
    ClActSys_FreeChars(sys->chars[1]);
    ClActSys_FreeCellAnims(sys->cellAnims[1]);
    ClActSys_FreePltt(sys->palettes[2]);
    ClActSys_FreeChars(sys->chars[2]);
    ClActSys_FreeCellAnims(sys->cellAnims[2]);
    ClActUnit_Delete(sys->clactUnit);
    GFL_HeapFree(sys);
}

void StaActButton_UpdateSystem(StaActButton *sys) {
    if (sys->active == TRUE) {
        TouchRect rects[] = {
            { 144, 176, 16, 48 },
            { 144, 176, 208, 240 },
            { TOUCH_RECT_END, 0, 0, 0 },
        };
        s32 hit = GFL_HIDGetTouchPressedRect(rects);

        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x10, 0);
        if (hit != TOUCH_RECT_NONE && sys->itemIds[hit] != 0xff && sys->used[hit] == FALSE) {
            ClActorPos pos;

            if (hit == 0) {
                StaActing_UseItem(sys->stage, STA_ACT_BUTTON_EQUIP_LEFT);
            } else {
                StaActing_UseItem(sys->stage, STA_ACT_BUTTON_EQUIP_RIGHT);
            }
            G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x10, -8);
            sys->used[hit] = TRUE;
            sys->active = FALSE;
            sys->pressed = TRUE;
            sys->selected = hit;
            pos.x = hit == 0 ? 32 : 224;
            pos.y = 128;
            ClActor_RestartAnim(sys->effect);
            ClActor_SetVisible(sys->effect, TRUE);
            ClActor_SetSurfacePos(sys->effect, &pos, CLACT_SURFACE_SUB);
            GCTX_HIDSetUsingTouch(TRUE);
        }
    }
    if (sys->selected != STA_ACT_BUTTON_NONE) {
        if (StaActing_IsUsingItem(sys->stage) == FALSE) {
            if (sys->pressed == FALSE) {
                sys->active = TRUE;
                ClActor_SetVisible(sys->buttons[sys->selected], FALSE);
                ClActor_SetVisible(sys->items[sys->selected], FALSE);
                sys->selected = STA_ACT_BUTTON_NONE;
            }
        } else if (sys->pressed == TRUE) {
            sys->pressed = FALSE;
        }
    }
    if (ClActor_IsVisible(sys->effect) == TRUE && ClActor_IsAnimActive(sys->effect) == FALSE) {
        ClActor_SetVisible(sys->effect, FALSE);
    }
}

static void StaActButton_InitGraphic(StaActButton *sys) {
    u8 i;
    MusItemData *itemData = StaActing_GetItemData(sys->stage);
    ArcTool *arc;
    ClActorSetup setup;
    ClActorSetup effectSetup;

    sys->clactUnit = ClActUnit_Create(5, 0, sys->heapId);
    ClActUnit_UseSystemRenderer(sys->clactUnit);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL, sys->heapId);
    sys->palettes[0] = ClActSys_LoadPlttRange(arc, 0, CLACT_VRAM_SUB, 0, 0, 2, sys->heapId);
    sys->chars[0] = ClActSys_LoadChars(arc, 6, FALSE, CLACT_VRAM_SUB, sys->heapId);
    sys->cellAnims[0] = ClActSys_LoadCellAnims(arc, 18, 21, sys->heapId);
    sys->palettes[1] = ClActSys_LoadPlttRange(arc, 1, CLACT_VRAM_SUB, 0x40, 0, 1, sys->heapId);
    sys->chars[1] = ClActSys_LoadChars(arc, 7, FALSE, CLACT_VRAM_SUB, sys->heapId);
    sys->cellAnims[1] = ClActSys_LoadCellAnims(arc, 19, 22, sys->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL_SHOT, sys->heapId);
    sys->palettes[2] = ClActSys_LoadPltt(arc, 0, CLACT_VRAM_SUB, 0x60, sys->heapId);
    sys->chars[2] = ClActSys_LoadChars(arc, 26, FALSE, CLACT_VRAM_SUB, sys->heapId);
    sys->cellAnims[2] = ClActSys_LoadCellAnims(arc, 18, 22, sys->heapId);
    GFL_ArcToolFree(arc);
    for (i = 0; i < 2; i++) {
        if (sys->itemIds[i] != 0xff) {
            u32 size = MusItemData_GetTexSize(MusItemData_GetItem(itemData, sys->itemIds[i]));

            if (i == 0) {
                setup.x = 32;
            } else {
                setup.x = 224;
            }
            setup.y = 160;
            setup.sequence = i;
            setup.priority = 10;
            setup.bgPriority = 0;
            sys->buttons[i] = ClActor_Create(sys->clactUnit, sys->chars[0], sys->palettes[0], sys->cellAnims[0], &setup,
                                             CLACT_SURFACE_SUB, sys->heapId);
            setup.sequence = 0;
            sys->items[i] = ClActor_Create(sys->clactUnit, sys->chars[1], sys->palettes[1], sys->cellAnims[1], &setup,
                                           CLACT_SURFACE_SUB, sys->heapId);
            ClActor_SetVisible(sys->buttons[i], FALSE);
            ClActor_SetVisible(sys->items[i], FALSE);
            StaActButton_LoadItemTexture(sys, i, sys->itemIds[i], size);
        }
    }
    effectSetup.x = 128;
    effectSetup.y = 96;
    effectSetup.sequence = 0;
    effectSetup.priority = 0;
    effectSetup.bgPriority = 0;
    sys->effect = ClActor_Create(sys->clactUnit, sys->chars[2], sys->palettes[2], sys->cellAnims[2], &effectSetup,
                                 CLACT_SURFACE_SUB, sys->heapId);
    ClActor_SetVisible(sys->effect, FALSE);
    ClActor_SetAutoAnim(sys->effect, TRUE);
}

// Copies the prop's texture, of the BlAct size `size`, into the characters of the prop's cell
static void StaActButton_LoadItemTexture(StaActButton *sys, u8 index, u16 itemId, u32 size) {
    void *file;
    NNSG3dResTex *tex;
    u32 plttOfs;
    u32 texSize;
    u32 plttSize;
    u32 charAddr;
    u8 row;
    u8 height;
    u8 line;
    u8 col;
    u8 width;
    u32 yStart;
    u32 xStart;
    u8 *texData;

    file = GFL_ArcSysReadHeapNewLZ(ARCID_MUSICAL_ITEM, itemId, FALSE, sys->heapId);
    tex = NNS_G3dGetTex(file);
    texData = (u8 *)tex + tex->texInfo.ofsTex;
    plttOfs = tex->plttInfo.ofsPlttData;
    texSize = tex->texInfo.sizeTex << 3;
    plttSize = tex->plttInfo.sizePltt << 3;
    charAddr = ClActSys_GetCharVramAddr(sys->chars[0], TRUE);
    charAddr += index << 11;

    DC_FlushRange((u8 *)tex + plttOfs, plttSize);
    GXS_LoadOBJPltt((u8 *)tex + plttOfs, index << 5, plttSize);
    DC_FlushRange(texData, texSize);
    yStart = 0;
    switch (size) {
    case 0x22:
        // 32 by 32
        xStart = 2;
        yStart = 2;
        width = 4;
        height = 4;
        break;
    case 0x23:
        // 32 by 64
        xStart = 2;
        width = 4;
        height = 8;
        break;
    case 0x32:
        // 64 by 32
        xStart = 0;
        width = 8;
        yStart = 2;
        height = 4;
        break;
    default:
        xStart = 2;
        yStart = 2;
        width = 4;
        height = 4;
        break;
    }
    for (row = 0; row < height; row++) {
        for (line = 0; line < 8; line++) {
            for (col = 0; col < width; col++) {
                GXS_LoadOBJ(texData + (col * 4 + (row * 32 * width + line * width * 4)),
                            charAddr + (line * 4 + ((col + xStart) * 32 + (row + yStart) * 256)), 4);
            }
        }
    }
    GFL_HeapFree(file);
}

void StaActButton_SetShowFlg(StaActButton *sys, BOOL show) {
    if (sys->itemIds[0] != 0xff) {
        ClActor_SetVisible(sys->buttons[0], show);
        ClActor_SetVisible(sys->items[0], show);
    }
    if (sys->itemIds[1] != 0xff) {
        ClActor_SetVisible(sys->buttons[1], show);
        ClActor_SetVisible(sys->items[1], show);
    }
    sys->active = show;
}
