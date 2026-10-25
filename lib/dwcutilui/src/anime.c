#include "anime.h"
#include "dwc_util_shared.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

typedef struct DWCUtilUiEffect {
    DWCUtilTask *task;
    GXOamAttr *oam;
    u8 variant;
    u8 frame;
    u8 padding[2];
} DWCUtilUiEffect;

// The OBJ palette of each screen variant, and where the effect is drawn: the first position is the one used
static const struct {
    u8 palettes[3];
    struct {
        u16 x;
        u16 y;
    } positions[2];
} sEffectData = { { 0x0e, 0x04, 0x05 }, { { 0xe6, 0x8b }, { 0x00, 0xa8 } } };
static DWCUtilUiEffect *sEffect;

static void DWCUtilUiEffect_Update(DWCUtilTask *task, void *arg);

void DWCi_ANIMElInitEx(u8 variant) {
    int mode = 4;

    sEffect = DWCi_HEAPlAllocEx(sizeof(DWCUtilUiEffect), 4);
    sEffect->variant = variant;
    sEffect->oam = DWCUtilCell_AllocOam(0, 0x47);
    sEffect->oam->attr2 = (sEffect->oam->attr2 & 0xfffff3ff) | (mode << 8);
    G2_SetOBJMode(sEffect->oam, GX_OAM_MODE_NORMAL, sEffectData.palettes[variant]);
    G2_SetOBJPosition(sEffect->oam, sEffectData.positions[0].x, sEffectData.positions[0].y);
    sEffect->task = DWCi_TSKlForm(1, DWCUtilUiEffect_Update, NULL, 0x78);
}

void DWCi_ANIMElEnd(void) {
    DWCi_TSKlDeleteEx(1, sEffect->task);
    DWCi_OBJlDelete(sEffect->oam);
    DWCi_HEAPlFree_(&sEffect);
}

static void DWCUtilUiEffect_Update(DWCUtilTask *task, void *arg) {
    int cell;

    sEffect->frame = FX_ModS32(sEffect->frame + 1, 40);
    cell = FX_DivS32(sEffect->frame, 5) + 0x47;
    DWCUtilCell_Copy(0, cell, sEffect->oam);
    G2_SetOBJPriority(sEffect->oam, 1);
    G2_SetOBJMode(sEffect->oam, GX_OAM_MODE_NORMAL, sEffectData.palettes[sEffect->variant]);
    G2_SetOBJPosition(sEffect->oam, sEffectData.positions[0].x, sEffectData.positions[0].y);
}
