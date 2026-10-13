#include "dwc_util_ui_effect.h"
#include "dwc_util_shared.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

typedef struct DWCUtilUiEffect {
    void *task;
    GXOamAttr *oam;
    u8 variant;
    u8 frame;
    u8 padding[2];
} DWCUtilUiEffect;

// The first three bytes map the screen variant to an OBJ palette. The rest of
// this ROM table is retained until its remaining values can be named.
static const u8 sEffectData[12] = { 0x0e, 0x04, 0x05, 0x00, 0xe6, 0x00, 0x8b, 0x00, 0x00, 0x00, 0xa8, 0x00 };
static DWCUtilUiEffect *sEffect;

static void DWCUtilUiEffect_Update(void);

void DWCUtilUiEffect_Init(u8 variant) {
    int mode = 4;

    sEffect = func_ov182_021bc4d8(sizeof(DWCUtilUiEffect), 4);
    sEffect->variant = variant;
    sEffect->oam = func_ov182_021bb960(0, 0x47);
    sEffect->oam->attr2 = (sEffect->oam->attr2 & 0xfffff3ff) | (mode << 8);
    sEffect->oam->attr01 &= 0xfffff3ff;
    sEffect->oam->attr2 = (sEffect->oam->attr2 & 0x0fff) | (sEffectData[variant] << 12);
    sEffect->oam->attr01 = (sEffect->oam->attr01 & 0xfe00ff00) | 0x00e6008b;
    sEffect->task = func_ov182_021bd200(1, DWCUtilUiEffect_Update, 0, 0x78);
}

void DWCUtilUiEffect_Destroy(void) {
    func_ov182_021bd280(1, sEffect->task);
    func_ov182_021bcc9c(sEffect->oam);
    func_ov182_021bc4f0(&sEffect);
}

static void DWCUtilUiEffect_Update(void) {
    u8 palette;

    sEffect->frame = FX_ModS32(sEffect->frame + 1, 40);
    func_ov182_021bb914(0, FX_DivS32(sEffect->frame, 5) + 0x47, sEffect->oam);
    sEffect->oam->attr2 = (sEffect->oam->attr2 & 0xfffff3ff) | 0x400;
    palette = sEffectData[sEffect->variant];
    sEffect->oam->attr01 &= 0xfffff3ff;
    sEffect->oam->attr2 = (sEffect->oam->attr2 & 0x0fff) | (palette << 12);
    sEffect->oam->attr01 = (sEffect->oam->attr01 & 0xfe00ff00) | 0x00e6008b;
}
