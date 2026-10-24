#ifndef POKEBW2_APP_MUSICAL_STA_ACT_LIGHT_H
#define POKEBW2_APP_MUSICAL_STA_ACT_LIGHT_H

// Overlay 209's sta_act_light.c: the stage's spotlights, cell actors drawn over the stage, and the one that follows
// the Pokémon in the limelight

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "struct_decls.h"

struct StaActLight {
    // The light's kind, 0 for none
    u32 type;
    GXRgb color;
    u8 alpha;
    VecFx32 pos;
    u32 unk14;
    u32 unk18;
    ClActor *actor;
};

struct StaActLightSys {
    HeapID heapId;
    // NULL when the lights don't follow a stage, as in the photo
    StaActing *stage;
    StaActLight lights[4];
    // Whether the light that follows the Pokémon in the limelight is shown
    BOOL followActive;
    u8 unk8C[0x18];
    ClActor *followActor;
    ClActUnit *clactUnit;
    u32 palette;
    u32 chars;
    u32 cellAnims;
};

StaActLightSys *StaActLight_InitSystem(HeapID heapId, StaActing *stage);
void StaActLight_TermSystem(StaActLightSys *sys);
void StaActLight_UpdateSystem(StaActLightSys *sys);
void StaActLight_DrawSystem(StaActLightSys *sys);
StaActLight *StaActLight_AddLight(StaActLightSys *sys, u32 type);
void StaActLight_DelLight(StaActLightSys *sys, StaActLight *light);
void StaActLight_SetPosition(StaActLightSys *sys, StaActLight *light, VecFx32 *pos);
void StaActLight_GetPosition(StaActLightSys *sys, StaActLight *light, VecFx32 *pos);
void StaActLight_SetColor(StaActLightSys *sys, StaActLight *light, GXRgb color, u8 alpha);
void StaActLight_SetElemB(StaActLightSys *sys, StaActLight *light, u32 a2, u32 a3);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_LIGHT_H
