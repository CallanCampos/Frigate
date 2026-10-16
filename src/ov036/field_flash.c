// The darkness of a dark cave around the player: a sphere drawn around the camera that Flash widens. The name is the
// ROM's string
#include "types.h"
#include "field/field_flash.h"
#include "field/fld_wipe_3dobj.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/aeabi.h"

typedef struct FieldFlashAnimation {
    fx32 scale;
    u8 alpha;
    u16 frame;
    u16 frameCount;
    fx32 scaleStart;
    fx32 scaleDelta;
    s16 alphaStart;
    s16 alphaDelta;
} FieldFlashAnimation;

struct FieldFlash {
    FieldFlashG3D *g3d;
    FieldFlashAnimation animation;
    u16 state;
    u16 nextState;
};

void FieldFlashAnimation_Init(FieldFlashAnimation *animation);
void FieldFlashAnimation_Reset(FieldFlashAnimation *animation);
BOOL FieldFlashAnimation_Update(FieldFlashAnimation *animation);
fx32 FieldFlashAnimation_GetScale(FieldFlashAnimation *animation);
void FieldFlashAnimation_SetScale(FieldFlashAnimation *animation, fx32 scale);
u8 FieldFlashAnimation_GetAlpha(FieldFlashAnimation *animation);
void FieldFlashAnimation_SetAlpha(FieldFlashAnimation *animation, u32 alpha);

void func_ov036_021c1d14(FieldFlash *flash);
void func_ov036_021c1d30(FieldFlash *flash);
void func_ov036_021c1d5c(FieldFlash *flash);
void func_ov036_021c1d78(FieldFlash *flash);

static void (*const FIELD_FLASH_FUNCTIONS[])(FieldFlash *) = {
    func_ov036_021c1d14,
    func_ov036_021c1d30,
    func_ov036_021c1d5c,
    func_ov036_021c1d78,
};

FieldFlash *FieldFlash_Create(HeapID heapId) {
    FieldFlash *flash;

    flash = GFL_HeapAllocate(heapId, sizeof(FieldFlash), TRUE, "field_flash.c", 0x7a);
    flash->g3d = FieldFlashG3D_Create(heapId);
    flash->state = 0;
    flash->nextState = 4;
    FieldFlashAnimation_Init(&flash->animation);
    return flash;
}

void FieldFlash_Free(FieldFlash *flash) {
    if (flash) {
        FieldFlashAnimation_Reset(&flash->animation);
        FieldFlashG3D_Free(flash->g3d);
        GFL_HeapFree(flash);
    }
}

void FieldFlash_Update(FieldFlash *flash) {
    if (flash) {
        if (flash->nextState != 4) {
            FIELD_FLASH_FUNCTIONS[flash->nextState](flash);
            flash->nextState = 4;
        }
        if (flash->state == 2) {
            if (FieldFlashAnimation_Update(&flash->animation)) {
                flash->state = 3;
            }
        }
    }
}

void FieldFlash_Draw(FieldFlash *flash) {
    if (flash) {
        fx32 scale = FieldFlashAnimation_GetScale(&flash->animation);
        u8 alpha = FieldFlashAnimation_GetAlpha(&flash->animation);
        FieldFlashG3D_Draw(flash->g3d, scale, alpha);
    }
}

void func_ov036_021c1c68(FieldFlash *flash, u32 nextState) {
    flash->nextState = nextState;
}

u16 func_ov036_021c1c6c(FieldFlash *flash) {
    return flash->state;
}

void FieldFlashAnimation_Init(FieldFlashAnimation *animation) {
    MI_CpuFill8(animation, 0, sizeof(FieldFlashAnimation));
}

void FieldFlashAnimation_Reset(FieldFlashAnimation *animation) {
    MI_CpuFill8(animation, 0, sizeof(FieldFlashAnimation));
}

void FieldFlashAnimation_Set(FieldFlashAnimation *animation, fx32 scaleStart, fx32 scaleEnd, s16 alphaStart,
                             u8 alphaEnd, u32 frameCount) {
    animation->scale = scaleStart;
    animation->scaleStart = scaleStart;
    animation->scaleDelta = scaleEnd - scaleStart;
    animation->alpha = alphaStart;
    animation->alphaStart = alphaStart;
    animation->alphaDelta = alphaEnd - alphaStart;
    animation->frame = 0;
    animation->frameCount = frameCount;
}

BOOL FieldFlashAnimation_Update(FieldFlashAnimation *animation) {
    if (animation->frame < animation->frameCount) {
        animation->frame++;
        animation->scale = animation->scaleStart +
                           FX_Div(FX_MUL(animation->scaleDelta, animation->frame << 12), animation->frameCount << 12);
        animation->alpha = animation->alphaStart + animation->alphaDelta * animation->frame / animation->frameCount;
        return FALSE;
    }
    return TRUE;
}

fx32 FieldFlashAnimation_GetScale(FieldFlashAnimation *animation) {
    return animation->scale;
}

void FieldFlashAnimation_SetScale(FieldFlashAnimation *animation, fx32 scale) {
    animation->scale = scale;
}

u8 FieldFlashAnimation_GetAlpha(FieldFlashAnimation *animation) {
    return animation->alpha;
}

void FieldFlashAnimation_SetAlpha(FieldFlashAnimation *animation, u32 alpha) {
    animation->alpha = alpha;
}

void func_ov036_021c1d14(FieldFlash *flash) {
    flash->state = 1;
    FieldFlashAnimation_SetScale(&flash->animation, 0x3000);
    FieldFlashAnimation_SetAlpha(&flash->animation, 0x1f);
}

void func_ov036_021c1d30(FieldFlash *flash) {
    flash->state = 2;
    FieldFlashAnimation_Set(&flash->animation, 0x3000, 0xaa00, 0x1f, 0x1f, 0xf);
    GFL_SndSEPlay(0x766);
}

void func_ov036_021c1d5c(FieldFlash *flash) {
    flash->state = 3;
    FieldFlashAnimation_SetScale(&flash->animation, 0xaa00);
    FieldFlashAnimation_SetAlpha(&flash->animation, 0x1f);
}

void func_ov036_021c1d78(FieldFlash *flash) {
    flash->state = 0;
    FieldFlashAnimation_SetScale(&flash->animation, 0);
}
