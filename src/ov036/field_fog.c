#include "types.h"
#include "field/field_fog.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "system/aeabi.h"

typedef struct FieldFogAnimation {
    u16 frame;
    u16 frameCount;
    s32 offsetStart;
    s32 offsetDelta;
    s32 depthShiftStart;
    s32 depthShiftDelta;
} FieldFogAnimation;

struct FieldFog {
    u8 enabled;
    u8 alphaMode;
    u8 depthShift;
    u8 alpha;
    s32 offset;
    GXRgb color;
    union {
        u32 words[8];
        u8 bytes[32];
    } table;
    FieldFogAnimation animation;
    BOOL dirty;
};

BOOL FieldFogAnimation_IsRunning(FieldFogAnimation *animation);
void FieldFogAnimation_Set(FieldFogAnimation *animation, s32 offsetStart, s32 offsetEnd, u16 depthShiftStart,
                           u16 depthShiftEnd, u16 frameCount);
void FieldFogAnimation_Step(FieldFogAnimation *animation);
s32 FieldFogAnimation_CalcFogOffset(FieldFogAnimation *animation);
u16 FieldFogAnimation_CalcFogDepthShift(FieldFogAnimation *animation);

FieldFog *FieldFog_Create(HeapID heapId) {
    FieldFog *fog = GFL_HeapAllocate(heapId, sizeof(FieldFog), TRUE, "field_fog.c", 0x89);

    fog->alphaMode = 0;
    fog->alpha = 0x1f;
    fog->offset = 0x7fff;
    FieldFog_SetDefaultTable(fog);
    return fog;
}

void FieldFog_Free(FieldFog *fog) {
    GFL_HeapFree(fog);
}

void FieldFog_Update(FieldFog *fog) {
    if (FieldFogAnimation_IsRunning(&fog->animation)) {
        FieldFogAnimation_Step(&fog->animation);
        fog->offset = FieldFogAnimation_CalcFogOffset(&fog->animation);
        fog->depthShift = FieldFogAnimation_CalcFogDepthShift(&fog->animation);
        fog->dirty = TRUE;
    }
}

void FieldFog_Flush(FieldFog *fog) {
    if (fog->dirty) {
        G3X_SetFog(fog->enabled, fog->alphaMode, fog->depthShift, fog->offset);
        G3X_SetFogColor(fog->color, fog->alpha);
        G3X_SetFogTable(fog->table.words);
        fog->dirty = FALSE;
    }
}

void FieldFog_SetEnabled(FieldFog *fog, BOOL enabled) {
    fog->enabled = enabled;
    fog->dirty = TRUE;
}

void FieldFog_SetAlphaMode(FieldFog *fog, u32 alphaMode) {
    fog->alphaMode = alphaMode;
    fog->dirty = TRUE;
}

void FieldFog_SetDepthShift(FieldFog *fog, u32 depthShift) {
    fog->depthShift = depthShift;
    fog->dirty = TRUE;
}

void FieldFog_SetOffset(FieldFog *fog, u32 offset) {
    fog->offset = offset;
    fog->dirty = TRUE;
}

void FieldFog_SetColor(FieldFog *fog, GXRgb color) {
    fog->color = color;
    fog->dirty = TRUE;
}

void FieldFog_SetAlpha(FieldFog *fog, u32 alpha) {
    fog->alpha = alpha;
    fog->dirty = TRUE;
}

void FieldFog_SetTable(FieldFog *fog, const u8 *table) {
    MIi_CpuCopy32(table, fog->table.bytes, sizeof(fog->table));
    fog->dirty = TRUE;
}

BOOL FieldFog_IsEnabled(FieldFog *fog) {
    return fog->enabled;
}

u32 FieldFog_GetDepthShift(FieldFog *fog) {
    return fog->depthShift;
}

void FieldFog_SetDefaultTable(FieldFog *fog) {
    s32 i;

    for (i = 0; i < 32; i++) {
        fog->table.bytes[i] = i * 4;
        fog->table.bytes[i] += (i * 4) / 32;
    }
}

void FieldFog_Animate(FieldFog *fog, u32 offsetEnd, u32 depthShiftEnd, u32 frameCount) {
    FieldFogAnimation_Set(&fog->animation, fog->offset, offsetEnd, fog->depthShift, depthShiftEnd, frameCount);
}

void FieldFog_AnimateEx(FieldFog *fog, s32 offsetStart, s32 offsetEnd, u32 depthShiftStart, u32 depthShiftEnd,
                        u16 frameCount) {
    FieldFogAnimation_Set(&fog->animation, offsetStart, offsetEnd, depthShiftStart, depthShiftEnd, frameCount);
}

BOOL FieldFog_IsAnimationRunning(FieldFog *fog) {
    return FieldFogAnimation_IsRunning(&fog->animation);
}

BOOL FieldFogAnimation_IsRunning(FieldFogAnimation *animation) {
    if (animation->frame < animation->frameCount) {
        return TRUE;
    }
    return FALSE;
}

void FieldFogAnimation_Set(FieldFogAnimation *animation, s32 offsetStart, s32 offsetEnd, u16 depthShiftStart,
                           u16 depthShiftEnd, u16 frameCount) {
    animation->frame = 0;
    animation->frameCount = frameCount;
    animation->offsetStart = offsetStart;
    animation->depthShiftStart = depthShiftStart;
    animation->offsetDelta = offsetEnd - offsetStart;
    animation->depthShiftDelta = depthShiftEnd - depthShiftStart;
}

void FieldFogAnimation_Step(FieldFogAnimation *animation) {
    if (animation->frame < animation->frameCount) {
        animation->frame++;
    }
}

s32 FieldFogAnimation_CalcFogOffset(FieldFogAnimation *animation) {
    s32 offset = animation->offsetDelta * animation->frame / (s32)animation->frameCount;

    offset += animation->offsetStart;
    return offset;
}

u16 FieldFogAnimation_CalcFogDepthShift(FieldFogAnimation *animation) {
    s32 depthShift = animation->depthShiftDelta * animation->frame / (s32)animation->frameCount;

    depthShift += animation->depthShiftStart;
    return depthShift;
}
