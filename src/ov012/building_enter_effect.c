#include "types.h"
#include "field/building_enter_effect.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

// The zoom into a building as the player enters it, an affine scale of BG 2 each VBlank (a descriptive name).
// setBuildingEnterVBlankCallback is swan's name

typedef struct {
    f32 scale;
    u32 unk4;
} BuildingEnterEffect;

static void func_ov012_02160dc8(void *data);

static f32 sScaleStep = 0.0044f;
static BuildingEnterEffect sEffect;

static void func_ov012_02160dc8(void *data) {
    MtxFx22 mtx;
    f32 zoom;
    f32 zero = 0.0f;
    fx32 scale;

    if (reg_GX_DISPSTAT & REG_GX_DISPSTAT_VBLK_MASK) {
        zoom = sEffect.scale + sScaleStep;
        sEffect.scale = zoom;
        scale = (fx32)(((1.0f / zoom) > zero) ?
                       ((1.0f / zoom) * FX32_ONE + 0.5f) :
                       ((1.0f / zoom) * FX32_ONE - 0.5f));
        MAT2_Scaling(&mtx, scale, scale);
        G2_SetBG2Affine(&mtx, 128, 92, 0, 0);
    }
}

void setBuildingEnterVBlankCallback(void) {
    sEffect.scale = 1.0f;
    sEffect.unk4 = 0;
    GFL_VBlankSetCallback(func_ov012_02160dc8, NULL);
}

void func_ov012_02160e88(void) {
    MtxFx22 mtx;

    MTX_Identity22_(&mtx);
    G2_SetBG2Affine(&mtx, 128, 92, 0, 0);
    GFL_VBlankResetCallback();
}
