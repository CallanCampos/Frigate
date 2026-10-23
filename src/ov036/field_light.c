#include "types.h"
#include "constants/arc.h"
#include "field/field.h"
#include "field/field_environment.h"
#include "field/field_fog.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"
#include "system/palanm.h"
#include "system/rtc.h"

// The field's lights (field_light.c, after the file name that its allocation passes): the light setups of the times of
// day, which it fades between, and flashes to a color. Also the color filter of the field's textures. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the layouts are ours

#define FIELD_LIGHT_MAX_ENTRIES 16

// A light setup at a time of day: which of the four lights are on, their colors and directions, and the material colors
typedef struct {
    u16 period;
    s16 minute;
    u8 enabled[4];
    GXRgb colors[4];
    VecFx16 dirs[4];
    GXRgb materials[6];
} FieldLightEntry;

// A color that moves from one color to another by steps of its red, green and blue
typedef struct {
    u16 r;
    s16 dr;
    u16 g;
    s16 dg;
    u16 b;
    s16 db;
} FieldLightColorInterpolator;

// A direction that turns about an axis by an angle
typedef struct {
    VecFx32 from;
    VecFx32 axis;
    u16 angle;
} FieldLightDirInterpolator;

typedef struct {
    u16 period;
    s16 minute;
    u8 enabled[4];
    FieldLightColorInterpolator colors[4];
    FieldLightDirInterpolator dirs[4];
    FieldLightColorInterpolator materials[6];
    u16 frame;
    u16 frames;
} FieldLightInterpolator;

typedef struct {
    u16 state;
    u16 frames;
    u16 returnFrames;
    u16 color;
} FieldLightFlashInfo;

struct FieldLight {
    u16 state;
    u16 season;
    G3DLight *lights;
    FieldFog *fog;
    u32 count;
    FieldLightEntry entries[FIELD_LIGHT_MAX_ENTRIES];
    u32 index;
    u16 lightsId;
    FieldLightEntry current;
    u16 valid;
    u32 dirty;
    FieldLightInterpolator interpolator;
    FieldLightFlashInfo flash;
    s32 daySeconds;
};

struct FieldColorPostFX {
    const u8 *lut;
    BOOL vintage;
};

static void FieldLight_LoadData(FieldLight *light, u32 lightsId);
static void FieldLight_LoadDataCore(FieldLight *light, u32 arcId, u32 fileId);
static void func_ov036_02197c00(FieldLight *light);
static u32 FieldLight_GetUsedEntryIndex(FieldLight *light, s32 daySeconds);
static s32 FieldLight_GetLightEndSecond(u32 season, const FieldLightEntry *entry);
static void FieldLight_FlushCore(const FieldLight *light, FieldFog *fog, G3DLight *lights);
static void FieldLightInterpolator_BeginDefault(FieldLightInterpolator *interpolator, const FieldLightEntry *from,
                                                const FieldLightEntry *to);
static void FieldLightInterpolator_Begin(FieldLightInterpolator *interpolator, const FieldLightEntry *from,
                                         const FieldLightEntry *to, u32 frames);
static void FieldLightInterpolator_BeginToConstColor(FieldLightInterpolator *interpolator, const FieldLightEntry *from,
                                                     GXRgb color, u32 frames);
static void FieldLightInterpolator_Update(FieldLightInterpolator *interpolator);
static BOOL FieldLightInterpolator_IsRunning(const FieldLightInterpolator *interpolator);
static void FieldLightInterpolator_Apply(const FieldLightInterpolator *interpolator, FieldLightEntry *out);
static void FieldLightFlashInfo_Set(FieldLightFlashInfo *info, u16 frames, u16 returnFrames, u16 color);
static void FieldLightFlashInfo_SetOneWay(FieldLightFlashInfo *info, u16 frames, u16 color);
static BOOL FieldLight_UpdateFlash(FieldLightFlashInfo *info, FieldLightInterpolator *interpolator,
                                   FieldLightEntry *current, const FieldLightEntry *target);
static BOOL FieldLightFlashInfo_IsRunning(const FieldLightFlashInfo *info);
static void FieldLightColorInterpolator_Set(FieldLightColorInterpolator *interpolator, GXRgb from, GXRgb to);
static GXRgb FieldLightColorInterpolator_Calc(const FieldLightColorInterpolator *interpolator, s32 frame, s32 frames);
static void FieldLightDirInterpolator_Set(FieldLightDirInterpolator *interpolator, const VecFx16 *from,
                                          const VecFx16 *to);
static void FieldLightDirInterpolator_Calc(const FieldLightDirInterpolator *interpolator, s32 frame, s32 frames,
                                           VecFx16 *out);

FieldLight *FieldLight_Create(u32 lightsId, s32 daySeconds, u32 season, FieldFog *fog, G3DLight *lights,
                              HeapID heapId) {
    FieldLight *light = GFL_HeapAllocate(heapId, sizeof(FieldLight), TRUE, "field_light.c", 359);

    light->season = season;
    FieldLight_LoadData(light, lightsId);
    light->valid = TRUE;
    light->index = FieldLight_GetUsedEntryIndex(light, daySeconds);
    MI_CpuCopy8(&light->entries[light->index], &light->current, sizeof(FieldLightEntry));
    light->dirty = TRUE;
    light->fog = fog;
    light->lights = lights;
    return light;
}

void FieldLight_Free(FieldLight *light) {
    func_ov036_02197c00(light);
    GFL_HeapFree(light);
}

void FieldLight_Update(FieldLight *light, s32 daySeconds) {
    if (light->count) {
        switch (light->state) {
        case 0: {
            s32 endSeconds = FieldLight_GetLightEndSecond(light->season, &light->entries[light->index]);
            BOOL wrapped = FALSE;
            s32 prevIndex = light->index - 1;
            s32 prevEndSeconds;

            if (prevIndex < 0) {
                prevEndSeconds = 0;
                s32 lastEndSeconds = FieldLight_GetLightEndSecond(light->season, &light->entries[light->count - 1]);

                if (lastEndSeconds <= daySeconds) {
                    prevEndSeconds = lastEndSeconds;
                    wrapped = TRUE;
                    endSeconds += 86400;
                }
            } else {
                prevEndSeconds = FieldLight_GetLightEndSecond(light->season, &light->entries[prevIndex]);
            }
            if ((prevEndSeconds > daySeconds || endSeconds <= daySeconds) && !wrapped) {
                light->index = (light->index + 1) % light->count;
                FieldLightInterpolator_BeginDefault(&light->interpolator, &light->current,
                                                    &light->entries[light->index]);
            }
            break;
        }
        case 1:
            if (FieldLight_UpdateFlash(&light->flash, &light->interpolator, &light->current,
                                       &light->entries[light->index])) {
                light->state = 0;
            }
            break;
        }
        if (FieldLightInterpolator_IsRunning(&light->interpolator)) {
            FieldLightInterpolator_Update(&light->interpolator);
            FieldLightInterpolator_Apply(&light->interpolator, &light->current);
            light->dirty = TRUE;
        }
        light->daySeconds = daySeconds;
    }
}

void FieldLight_Flush(FieldLight *light, BOOL force) {
    if (light->dirty || force) {
        FieldLight_FlushCore(light, light->fog, light->lights);
        light->dirty = FALSE;
    }
}

void FieldLight_ChangeData(FieldLight *light, u32 arcId, u32 fileId, u32 frames) {
    func_ov036_02197c00(light);
    FieldLight_LoadDataCore(light, arcId, fileId);
    light->index = FieldLight_GetUsedEntryIndex(light, light->daySeconds);
    FieldLightInterpolator_Begin(&light->interpolator, &light->current, &light->entries[light->index], frames);
}

void FieldLight_ChangeDataDefault(FieldLight *light) {
    func_ov036_02197c00(light);
    FieldLight_LoadData(light, light->lightsId);
    light->index = FieldLight_GetUsedEntryIndex(light, light->daySeconds);
    FieldLightInterpolator_BeginDefault(&light->interpolator, &light->current, &light->entries[light->index]);
}

void FieldLight_StartFlash(FieldLight *light, u16 color, u32 frames, u32 returnFrames) {
    FieldLightFlashInfo_Set(&light->flash, frames, returnFrames, color);
    light->state = 1;
}

void FieldLight_StartFlashOneWay(FieldLight *light, u16 color, u32 frames) {
    FieldLightFlashInfo_SetOneWay(&light->flash, frames, color);
    light->state = 1;
}

BOOL FieldLight_IsFlashRunning(FieldLight *light) {
    return FieldLightFlashInfo_IsRunning(&light->flash);
}

static void FieldLight_FlushCore(const FieldLight *light, FieldFog *fog, G3DLight *lights) {
    VecFx16 zeroDir = { 0, 0, 0 };
    GXRgb zeroColor = 0;
    s32 i;

    if (light->valid) {
        for (i = 0; i < 4; i++) {
            if (light->current.enabled[i]) {
                GFL_G3DLightSetDirVector(lights, i, &light->current.dirs[i]);
                GFL_G3DLightSetColor(lights, i, &light->current.colors[i]);
            } else {
                GFL_G3DLightSetDirVector(lights, i, &zeroDir);
                GFL_G3DLightSetColor(lights, i, &zeroColor);
            }
        }
        NNS_G3dGlbMaterialColorDiffAmb(light->current.materials[0], light->current.materials[1], TRUE);
        NNS_G3dGlbMaterialColorSpecEmi(light->current.materials[2], light->current.materials[3], FALSE);
        FieldFog_SetColor(fog, light->current.materials[4]);
        G3X_SetClearColor(light->current.materials[5], 31, 0x7fff, 0, FALSE);
    }
}

static void FieldLight_LoadData(FieldLight *light, u32 lightsId) {
    light->lightsId = lightsId;
    FieldLight_LoadDataCore(light, ARCID_LIGHTS_FIELD, lightsId);
}

static void FieldLight_LoadDataCore(FieldLight *light, u32 arcId, u32 fileId) {
    u32 i;
    s32 j;

    light->count = GFL_ArcSysGetDataLength(arcId, fileId) / sizeof(FieldLightEntry);
    GFL_ArcSysRead(light->entries, arcId, fileId);
    for (i = 0; i < light->count; i++) {
        for (j = 0; j < 4; j++) {
            VEC_Fx16Normalize(&light->entries[i].dirs[j], &light->entries[i].dirs[j]);
        }
    }
}

static void func_ov036_02197c00(FieldLight *light) {
}

static u32 FieldLight_GetUsedEntryIndex(FieldLight *light, s32 daySeconds) {
    s32 prevEndSeconds = 0;
    u32 i;

    for (i = 0; i < light->count; i++) {
        s32 endSeconds = FieldLight_GetLightEndSecond(light->season, &light->entries[i]);

        if (prevEndSeconds <= daySeconds && endSeconds > daySeconds) {
            return i;
        }
        prevEndSeconds = endSeconds;
    }
    return 0;
}

static s32 FieldLight_GetLightEndSecond(u32 season, const FieldLightEntry *entry) {
    s32 hourSeconds = GetLightChangeHoursForSeasons(season, entry->period) * 3600;

    return hourSeconds + entry->minute * 60;
}

static void FieldLightColorInterpolator_Set(FieldLightColorInterpolator *interpolator, GXRgb from, GXRgb to) {
    interpolator->r = from & GX_RGB_R_MASK;
    interpolator->dr = (s16)(to & GX_RGB_R_MASK) - interpolator->r;
    interpolator->g = (from & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    interpolator->dg = (s16)((to & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT) - interpolator->g;
    interpolator->b = (from & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    interpolator->db = (s16)((to & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT) - interpolator->b;
}

static GXRgb FieldLightColorInterpolator_Calc(const FieldLightColorInterpolator *interpolator, s32 frame, s32 frames) {
    s16 r = interpolator->dr * frame / frames;
    s16 g;
    s16 b;

    r += interpolator->r;
    g = interpolator->dg * frame / frames;
    g += interpolator->g;
    b = interpolator->db * frame / frames;
    b += interpolator->b;
    return GX_RGB(r, g, b);
}

static void FieldLightDirInterpolator_Set(FieldLightDirInterpolator *interpolator, const VecFx16 *from,
                                          const VecFx16 *to) {
    VecFx16 axis;

    VEC_Set(&interpolator->from, from->x, from->y, from->z);
    VEC_Fx16CrossProduct(from, to, &axis);
    VEC_Set(&interpolator->axis, axis.x, axis.y, axis.z);
    interpolator->angle = FX_AcosIdx(VEC_Fx16DotProduct(from, to));
}

static void FieldLightDirInterpolator_Calc(const FieldLightDirInterpolator *interpolator, s32 frame, s32 frames,
                                           VecFx16 *out) {
    MtxFx43 rotation;
    VecFx32 dir;
    u16 angle = interpolator->angle * frame / frames;

    MTX_RotAxis43(&rotation, &interpolator->axis, FX_SinIdx(angle), FX_CosIdx(angle));
    MTX_MultVec43(&interpolator->from, &rotation, &dir);
    VEC_Normalize(&dir, &dir);
    VEC_Fx16Set(out, dir.x, dir.y, dir.z);
}

static void FieldLightInterpolator_BeginDefault(FieldLightInterpolator *interpolator, const FieldLightEntry *from,
                                                const FieldLightEntry *to) {
    FieldLightInterpolator_Begin(interpolator, from, to, 60);
}

static void FieldLightInterpolator_Begin(FieldLightInterpolator *interpolator, const FieldLightEntry *from,
                                         const FieldLightEntry *to, u32 frames) {
    s32 i;

    interpolator->frame = 0;
    interpolator->frames = frames;
    interpolator->period = to->period;
    interpolator->minute = to->minute;
    for (i = 0; i < 4; i++) {
        FieldLightColorInterpolator_Set(&interpolator->colors[i], from->colors[i], to->colors[i]);
        FieldLightDirInterpolator_Set(&interpolator->dirs[i], &from->dirs[i], &to->dirs[i]);
        interpolator->enabled[i] = to->enabled[i];
    }
    FieldLightColorInterpolator_Set(&interpolator->materials[0], from->materials[0], to->materials[0]);
    FieldLightColorInterpolator_Set(&interpolator->materials[1], from->materials[1], to->materials[1]);
    FieldLightColorInterpolator_Set(&interpolator->materials[2], from->materials[2], to->materials[2]);
    FieldLightColorInterpolator_Set(&interpolator->materials[3], from->materials[3], to->materials[3]);
    FieldLightColorInterpolator_Set(&interpolator->materials[4], from->materials[4], to->materials[4]);
    FieldLightColorInterpolator_Set(&interpolator->materials[5], from->materials[5], to->materials[5]);
}

static void FieldLightInterpolator_BeginToConstColor(FieldLightInterpolator *interpolator, const FieldLightEntry *from,
                                                     GXRgb color, u32 frames) {
    s32 i;

    interpolator->frame = 0;
    interpolator->frames = frames;
    interpolator->period = from->period;
    interpolator->minute = from->minute;
    for (i = 0; i < 4; i++) {
        FieldLightColorInterpolator_Set(&interpolator->colors[i], from->colors[i], color);
        FieldLightDirInterpolator_Set(&interpolator->dirs[i], &from->dirs[i], &from->dirs[i]);
        interpolator->enabled[i] = from->enabled[i];
    }
    FieldLightColorInterpolator_Set(&interpolator->materials[0], from->materials[0], color);
    FieldLightColorInterpolator_Set(&interpolator->materials[1], from->materials[1], color);
    FieldLightColorInterpolator_Set(&interpolator->materials[2], from->materials[2], color);
    FieldLightColorInterpolator_Set(&interpolator->materials[3], from->materials[3], color);
    FieldLightColorInterpolator_Set(&interpolator->materials[4], from->materials[4], color);
    FieldLightColorInterpolator_Set(&interpolator->materials[5], from->materials[5], color);
}

static void FieldLightInterpolator_Update(FieldLightInterpolator *interpolator) {
    if (interpolator->frame < interpolator->frames) {
        interpolator->frame++;
    }
}

static BOOL FieldLightInterpolator_IsRunning(const FieldLightInterpolator *interpolator) {
    if (interpolator->frame < interpolator->frames) {
        return TRUE;
    }
    return FALSE;
}

static void FieldLightInterpolator_Apply(const FieldLightInterpolator *interpolator, FieldLightEntry *out) {
    s32 i;

    out->period = interpolator->period;
    out->minute = interpolator->minute;
    for (i = 0; i < 4; i++) {
        out->colors[i] =
            FieldLightColorInterpolator_Calc(&interpolator->colors[i], interpolator->frame, interpolator->frames);
        FieldLightDirInterpolator_Calc(&interpolator->dirs[i], interpolator->frame, interpolator->frames,
                                       &out->dirs[i]);
        out->enabled[i] = interpolator->enabled[i];
    }
    out->materials[0] =
        FieldLightColorInterpolator_Calc(&interpolator->materials[0], interpolator->frame, interpolator->frames);
    out->materials[1] =
        FieldLightColorInterpolator_Calc(&interpolator->materials[1], interpolator->frame, interpolator->frames);
    out->materials[2] =
        FieldLightColorInterpolator_Calc(&interpolator->materials[2], interpolator->frame, interpolator->frames);
    out->materials[3] =
        FieldLightColorInterpolator_Calc(&interpolator->materials[3], interpolator->frame, interpolator->frames);
    out->materials[4] =
        FieldLightColorInterpolator_Calc(&interpolator->materials[4], interpolator->frame, interpolator->frames);
    out->materials[5] =
        FieldLightColorInterpolator_Calc(&interpolator->materials[5], interpolator->frame, interpolator->frames);
}

static void FieldLightFlashInfo_Set(FieldLightFlashInfo *info, u16 frames, u16 returnFrames, u16 color) {
    info->state = 1;
    info->frames = frames;
    info->returnFrames = returnFrames;
    info->color = color;
}

static void FieldLightFlashInfo_SetOneWay(FieldLightFlashInfo *info, u16 frames, u16 color) {
    info->state = 1;
    info->frames = frames;
    info->color = color;
    info->returnFrames = 0xffff;
}

static BOOL FieldLight_UpdateFlash(FieldLightFlashInfo *info, FieldLightInterpolator *interpolator,
                                   FieldLightEntry *current, const FieldLightEntry *target) {
    switch (info->state) {
    case 1:
        FieldLightInterpolator_BeginToConstColor(interpolator, current, info->color, info->frames);
        info->state = 2;
        break;
    case 2:
        if (!FieldLightInterpolator_IsRunning(interpolator)) {
            if (info->returnFrames != 0xffff) {
                FieldLightInterpolator_Begin(interpolator, current, target, info->returnFrames);
            }
            info->state = 3;
        }
        break;
    case 3:
        if (!FieldLightInterpolator_IsRunning(interpolator)) {
            info->state = 0;
        }
        break;
    case 0:
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldLightFlashInfo_IsRunning(const FieldLightFlashInfo *info) {
    if (info->state) {
        return TRUE;
    }
    return FALSE;
}

void FieldColorPostFX_Set(void *postFx, void *luminanceTable, BOOL flashback) {
    FieldColorPostFX *fx = postFx;

    fx->lut = luminanceTable;
    fx->vintage = flashback;
}

void FieldColorPostFX_Apply(void *postFx, void *texture) {
    FieldColorPostFX *fx = postFx;

    if (fx && texture) {
        if (GFL_G3DResCheckType(texture, G3D_RES_CHECK_TEX)) {
            NNSG3dResTex *tex = NNS_G3dGetTex(GFL_G3DResGetResData(texture));

            if (tex) {
                int count = (tex->plttInfo.sizePltt << 3) / sizeof(u16);
                u16 *colors = (u16 *)((u8 *)tex + tex->plttInfo.ofsPlttData);

                if (fx->lut) {
                    ColorFilter_ApplyLUT(colors, count, fx->lut);
                } else if (fx->vintage) {
                    ColorFilter_ApplyVintage(colors, count);
                }
            }
        }
    }
}
