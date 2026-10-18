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
    obj->unit = func_0204bf1c(35, 0, obj->heapId);
    obj->res[0].palette = func_0204bbb8(arc, 15, CLACT_VRAM_SUB, 0, 0, 5, obj->heapId);
    obj->res[0].chars = func_0204b81c(arc, 34, FALSE, CLACT_VRAM_SUB, obj->heapId);
    obj->res[0].cellAnims = func_0204bde0(arc, 53, 55, obj->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), obj->heapId);
    obj->res[1].palette = func_0204bba0(arc, func_0202d810(), CLACT_VRAM_SUB, 0xa0, obj->heapId);
    obj->res[1].chars = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_SUB, obj->heapId);
    obj->res[1].cellAnims = func_0204bde0(arc, func_0202d818(0), func_0202d81c(0), obj->heapId);
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
        obj->actors[i] = func_0204c040(obj->unit, obj->res[defs[i].charRes].chars, obj->res[defs[i].paletteRes].palette,
                                       obj->res[defs[i].cellRes].cellAnims, &setup, CLACT_SURFACE_SUB, obj->heapId);
#else
        obj->actors[i] = func_0204c040(obj->unit, obj->res[defs[i].paletteRes].palette, obj->res[defs[i].charRes].chars,
                                       obj->res[defs[i].cellRes].cellAnims, &setup, CLACT_SURFACE_SUB, obj->heapId);
#endif
        func_0204c124(obj->actors[i], TRUE);
        func_0204c53c(obj->actors[i], FX32_ONE);
        func_0204c520(obj->actors[i], TRUE);
        if (i == TRCARD_ACTOR_3) {
            func_0204c124(obj->actors[i], FALSE);
        }
    }
    func_0204c124(obj->actors[TRCARD_ACTOR_BRUSH], FALSE);
}

void TrCardObj_Exit(TrainerCardObjWork *obj) {
    u8 i;

    for (i = 0; i < 2; i++) {
        func_0204bcd0(obj->res[i].palette);
        func_0204b98c(obj->res[i].chars);
        func_0204be64(obj->res[i].cellAnims);
    }
    func_0204bf98(obj->unit);
    func_0204b758();
    GX_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);
}

// Restarts an actor on an animation
void TrCardObj_SetAnime(TrainerCardObjWork *obj, int actor, u8 sequence, u8 visible) {
    func_0204c488(obj->actors[actor], sequence);
    func_0204c56c(obj->actors[actor]);
    func_0204c124(obj->actors[actor], visible);
}

// Changes an actor's animation and palette, unless it is already on the animation
void TrCardObj_SetAnimePalette(TrainerCardObjWork *obj, int actor, u8 sequence, u8 visible, u8 palette) {
    u16 old = func_0204c4a0(obj->actors[actor]);

    func_0204c124(obj->actors[actor], visible);
    if (old != sequence) {
        func_0204c488(obj->actors[actor], sequence);
        func_0204c56c(obj->actors[actor]);
        func_0204c378(obj->actors[actor], palette, 0);
    }
}

void TrCardObj_SetVisible(TrainerCardObjWork *obj, u8 actor, BOOL visible) {
    func_0204c124(obj->actors[actor], visible);
}
