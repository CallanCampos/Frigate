#include "types.h"
#include "app/musical/sta_act_light.h"
#include "app/musical/sta_act_poke.h"
#include "app/musical/sta_acting.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// Overlay 209's sta_act_light.c: the stage's spotlights, 2D cell actors over the 3D stage, placed in fixed point
// pixels and scrolled with the stage

// The position of StaActing_GetLightUpPoke when no Pokémon is in the limelight
#define STA_ACT_LIGHT_NO_POKE 4

static void StaActLight_UpdateLight(StaActLightSys *sys, StaActLight *light);
static void StaActLight_UpdateFollowLight(StaActLightSys *sys);

StaActLightSys *StaActLight_InitSystem(HeapID heapId, StaActing *stage) {
    u8 i;
    ArcTool *arc;
    StaActLightSys *sys = GFL_HeapAllocate(heapId, sizeof(StaActLightSys), FALSE, "sta_act_light.c", 87);

    sys->heapId = heapId;
    sys->stage = stage;
    for (i = 0; i < 4; i++) {
        sys->lights[i].type = 0;
    }
    sys->followActive = FALSE;
    sys->clactUnit = ClActUnit_Create(5, 0, sys->heapId);
    ClActUnit_UseSystemRenderer(sys->clactUnit);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL, sys->heapId);
    sys->palette = ClActSys_LoadPltt(arc, 4, CLACT_VRAM_MAIN, 0, sys->heapId);
    sys->chars = ClActSys_LoadChars(arc, 10, FALSE, CLACT_VRAM_MAIN, sys->heapId);
    sys->cellAnims = ClActSys_LoadCellAnims(arc, 20, 23, sys->heapId);
    GFL_ArcToolFree(arc);
    return sys;
}

void StaActLight_TermSystem(StaActLightSys *sys) {
    ClActSys_FreePltt(sys->palette);
    ClActSys_FreeChars(sys->chars);
    ClActSys_FreeCellAnims(sys->cellAnims);
    ClActUnit_Delete(sys->clactUnit);
    GFL_HeapFree(sys);
}

void StaActLight_UpdateSystem(StaActLightSys *sys) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sys->lights[i].type != 0) {
            StaActLight_UpdateLight(sys, &sys->lights[i]);
        }
    }
    if (sys->stage != NULL) {
        StaActLight_UpdateFollowLight(sys);
    }
}

static void StaActLight_UpdateLight(StaActLightSys *sys, StaActLight *light) {
    u16 scroll = sys->stage == NULL ? 128 : StaActing_GetScrollOffset(sys->stage);

    if (light->type == 1) {
        ClActorPos pos;

        pos.x = FX_FX32_TO_F32(light->pos.x) - scroll;
        pos.y = FX_FX32_TO_F32(light->pos.y);
        ClActor_SetSurfacePos(light->actor, &pos, CLACT_SURFACE_MAIN);
        if (sys->stage != NULL && StaActing_GetLightUpPoke(sys->stage) != STA_ACT_LIGHT_NO_POKE) {
            ClActor_SetVisible(light->actor, FALSE);
        } else {
            ClActor_SetVisible(light->actor, TRUE);
        }
    }
}

static void StaActLight_UpdateFollowLight(StaActLightSys *sys) {
    if (sys->followActive == FALSE && StaActing_GetLightUpPoke(sys->stage) != STA_ACT_LIGHT_NO_POKE) {
        ClActorSetup setup;

        sys->followActive = TRUE;
        setup.x = 128;
        setup.y = 96;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 2;
        sys->followActor = ClActor_Create(sys->clactUnit, sys->chars, sys->palette, sys->cellAnims, &setup,
                                          CLACT_SURFACE_MAIN, sys->heapId);
        ClActor_SetVisible(sys->followActor, TRUE);
    }
    if (sys->followActive == TRUE) {
        StaActPokeSys *pokeSys;
        StaActPoke *poke;
        u16 scroll;
        VecFx32 pokePos;
        ClActorPos pos;

        if (StaActing_GetLightUpPoke(sys->stage) == STA_ACT_LIGHT_NO_POKE) {
            ClActor_Delete(sys->followActor);
            sys->followActive = FALSE;
            return;
        }
        pokeSys = StaActing_GetPokeSys(sys->stage);
        poke = StaActing_GetPoke(sys->stage, StaActing_GetLightUpPoke(sys->stage));
        scroll = StaActing_GetScrollOffset(sys->stage);
        StaActPoke_GetPosition(pokeSys, poke, &pokePos);
        pos.x = FX_FX32_TO_F32(pokePos.x) - scroll;
        pos.y = FX_FX32_TO_F32(pokePos.y) - 32.0f;
        ClActor_SetSurfacePos(sys->followActor, &pos, CLACT_SURFACE_MAIN);
    }
}

void StaActLight_DrawSystem(StaActLightSys *sys) {
}

StaActLight *StaActLight_AddLight(StaActLightSys *sys, u32 type) {
    u8 i;
    StaActLight *light;
    ClActorSetup setup;

    for (i = 0; i < 4; i++) {
        if (sys->lights[i].type == 0) {
            break;
        }
    }
    sys->lights[i].type = type;
    light = &sys->lights[i];
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 2;
    light->actor = ClActor_Create(sys->clactUnit, sys->chars, sys->palette, sys->cellAnims, &setup, CLACT_SURFACE_MAIN,
                                  sys->heapId);
    ClActor_SetVisible(light->actor, TRUE);
    return light;
}

void StaActLight_DelLight(StaActLightSys *sys, StaActLight *light) {
    ClActor_Delete(light->actor);
    light->type = 0;
}

void StaActLight_SetPosition(StaActLightSys *sys, StaActLight *light, VecFx32 *pos) {
    light->pos.x = pos->x;
    light->pos.y = pos->y;
    light->pos.z = pos->z;
}

void StaActLight_GetPosition(StaActLightSys *sys, StaActLight *light, VecFx32 *pos) {
    pos->x = light->pos.x;
    pos->y = light->pos.y;
    pos->z = light->pos.z;
}

void StaActLight_SetColor(StaActLightSys *sys, StaActLight *light, GXRgb color, u8 alpha) {
    light->color = color;
    light->alpha = alpha;
}

void func_ov209_021bd770(StaActLightSys *sys, StaActLight *light, u32 a2, u32 a3) {
    light->unk14 = a2;
    light->unk18 = a3;
}
