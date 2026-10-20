#include "types.h"
#include "app/trainer_card.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/app_menu_common.h"

// The trainer card screen's cell actors: its buttons, the scroll arrows and the rank. The ROM has no name for this
// file; trcard_obj.c is a guess

typedef struct {
    u16 x;
    u16 y;
    // Which of the work's resources the actor uses: the card's own (0) or the touch bar's (1)
    u8 charRes;
    u8 paletteRes;
    u8 cellRes;
    u8 bgPriority;
    u32 sequence;
} TrCardObjActorDef;

void TrCardObj_Init(TrainerCardObjWork *obj, const BGSysVRAMConfig *vram) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_TRAINER_CARD_GRA, obj->heapId);

    ClActSys_Create(&data_02093f08, vram, obj->heapId);
    obj->unit = ClActUnit_Create(35, 0, obj->heapId);
    obj->res[0].palette = ClActSys_LoadPlttRange(arc, 15, CLACT_VRAM_SUB, 0, 0, 5, obj->heapId);
    obj->res[0].chars = ClActSys_LoadChars(arc, 34, FALSE, CLACT_VRAM_SUB, obj->heapId);
    obj->res[0].cellAnims = ClActSys_LoadCellAnims(arc, 53, 55, obj->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), obj->heapId);
    obj->res[1].palette =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_SUB, 0xa0, obj->heapId);
    obj->res[1].chars =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_SUB, obj->heapId);
    obj->res[1].cellAnims = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(0),
                                                   AppMenuCommon_GetTouchBarAnimFile(0), obj->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysSetBGEnabledA(0x10, TRUE);
    GFL_BGSysSetBGEnabledB(0x10, TRUE);
}

void TrCardObj_AddActors(TrainerCardObjWork *obj) {
    TrCardObjActorDef defs[TRCARD_ACTOR_COUNT] = {
        { 228, 168, 1, 1, 1, 0, 1 },  { 204, 168, 1, 1, 1, 0, 0 }, { 4, 168, 0, 0, 0, 0, 16 },
        { 0, 0, 0, 0, 0, 0, 5 },      { 36, 168, 0, 0, 0, 0, 18 }, { 68, 168, 0, 0, 0, 0, 6 },
        { 100, 168, 0, 0, 0, 0, 22 }, { 180, 172, 1, 1, 1, 0, 6 }, { 16, 48, 0, 0, 0, 1, 24 },
        { 16, 64, 0, 0, 0, 1, 24 },   { 16, 120, 0, 0, 0, 1, 25 }, { 236, 16, 0, 0, 0, 1, 26 },
        { 236, 72, 0, 0, 0, 1, 27 },  { 224, 80, 0, 0, 0, 1, 28 },
    };
    int i;
    ClActorSetup setup;

    for (i = 0; i < TRCARD_ACTOR_COUNT; i++) {
        setup.x = defs[i].x;
        setup.y = defs[i].y;
        setup.sequence = defs[i].sequence;
        setup.bgPriority = defs[i].bgPriority;
        // BUG: the palette's resource is passed for the characters and the characters' for the palette. Both kinds
        // are numbered from 0 in the order they are loaded in, so the indices are the same
#ifdef BUGFIX
        obj->actors[i] =
            ClActor_Create(obj->unit, obj->res[defs[i].charRes].chars, obj->res[defs[i].paletteRes].palette,
                           obj->res[defs[i].cellRes].cellAnims, &setup, CLACT_SURFACE_SUB, obj->heapId);
#else
        obj->actors[i] =
            ClActor_Create(obj->unit, obj->res[defs[i].paletteRes].palette, obj->res[defs[i].charRes].chars,
                           obj->res[defs[i].cellRes].cellAnims, &setup, CLACT_SURFACE_SUB, obj->heapId);
#endif
        ClActor_SetVisible(obj->actors[i], TRUE);
        ClActor_SetAutoAnimSpeed(obj->actors[i], FX32_ONE);
        ClActor_SetAutoAnim(obj->actors[i], TRUE);
        if (i == TRCARD_ACTOR_3) {
            ClActor_SetVisible(obj->actors[i], FALSE);
        }
    }
    ClActor_SetVisible(obj->actors[TRCARD_ACTOR_BRUSH], FALSE);
}

void TrCardObj_Exit(TrainerCardObjWork *obj) {
    u8 i;

    for (i = 0; i < 2; i++) {
        ClActSys_FreePltt(obj->res[i].palette);
        ClActSys_FreeChars(obj->res[i].chars);
        ClActSys_FreeCellAnims(obj->res[i].cellAnims);
    }
    ClActUnit_Delete(obj->unit);
    ClActSys_Delete();
    GX_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);
}

// Restarts an actor on an animation
void TrCardObj_SetAnime(TrainerCardObjWork *obj, int actor, u8 sequence, u8 visible) {
    ClActor_SetSequence(obj->actors[actor], sequence);
    ClActor_RestartAnim(obj->actors[actor]);
    ClActor_SetVisible(obj->actors[actor], visible);
}

// Changes an actor's animation and palette, unless it is already on the animation
void TrCardObj_SetAnimePalette(TrainerCardObjWork *obj, int actor, u8 sequence, u8 visible, u8 palette) {
    u16 old = ClActor_GetSequence(obj->actors[actor]);

    ClActor_SetVisible(obj->actors[actor], visible);
    if (old != sequence) {
        ClActor_SetSequence(obj->actors[actor], sequence);
        ClActor_RestartAnim(obj->actors[actor]);
        ClActor_SetPalette(obj->actors[actor], palette, 0);
    }
}

void TrCardObj_SetVisible(TrainerCardObjWork *obj, u8 actor, BOOL visible) {
    ClActor_SetVisible(obj->actors[actor], visible);
}
