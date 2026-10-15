#include "types.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/gfd.h"

// A scene of billboard actors and the materials they show, drawn straight to the geometry engine

// An actor's quad, in units of its scale, by origin
typedef struct {
    s16 top;
    s16 bottom;
    s16 left;
    s16 right;
} BlActQuad;

// The GX_TEXSIZE_S* and GX_TEXSIZE_T* of each size code
typedef struct {
    u32 s;
    u32 t;
} BlActTexSize;

// The GX_TEXFMT_* of each format a material is added with
const u32 BLACT_PIXEL_FORMATS[5] = {
    GX_TEXFMT_PLTT16, GX_TEXFMT_PLTT256, GX_TEXFMT_PLTT4, GX_TEXFMT_A3I5, GX_TEXFMT_A5I3,
};
const BlActTexSize MMDL_BB_SIZETBL[8] = {
    { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 },
};
const BlActQuad BLACT_GEOM_QUADS[9] = {
    { FX32_ONE, -FX32_ONE, -FX32_ONE, FX32_ONE },
    { 0, -2 * FX32_ONE, -FX32_ONE, FX32_ONE },
    { 2 * FX32_ONE, 0, -FX32_ONE, FX32_ONE },
    { FX32_ONE, -FX32_ONE, 0, 2 * FX32_ONE },
    { FX32_ONE, -FX32_ONE, -2 * FX32_ONE, 0 },
    { 0, -2 * FX32_ONE, 0, 2 * FX32_ONE },
    { 0, -2 * FX32_ONE, -2 * FX32_ONE, 0 },
    { 2 * FX32_ONE, 0, 0, 2 * FX32_ONE },
    { 2 * FX32_ONE, 0, -2 * FX32_ONE, 0 },
};

BlActScene *BlActScene_Create(const BlActSceneSetup *setup, HeapID heapId) {
    BlActScene *scene = GFL_HeapAllocate(heapId, sizeof(BlActScene), TRUE, "billboard.c", 153);
    int i;
    int light;

    scene->heapId = heapId;
    scene->setup = *setup;
    scene->materials =
        GFL_HeapAllocate(heapId, scene->setup.materialCount * sizeof(BlActMaterial), TRUE, "billboard.c", 159);
    scene->actors = GFL_HeapAllocate(heapId, scene->setup.actorCount * sizeof(BlAct), TRUE, "billboard.c", 161);
    for (i = 0; i < scene->setup.materialCount; i++) {
        scene->materials[i].texResource = NULL;
        scene->materials[i].texFormat = BLACT_TEXFMT_FREE;
    }
    for (i = 0; i < scene->setup.actorCount; i++) {
        scene->actors[i].material = BLACT_MATERIAL_NONE;
    }
    scene->customNormal = FALSE;
    scene->normal.x = 0;
    scene->normal.y = 0;
    scene->normal.z = 0;
    scene->keepViewMtx = FALSE;
    scene->texOffsetS = 0;
    scene->texOffsetT = 0;
    return scene;
}

void func_0204e450(BlActScene *scene) {
    func_0204e7b8(scene);
    GFL_HeapFree(scene->actors);
    GFL_HeapFree(scene->materials);
    GFL_HeapFree(scene);
}

void BlActScene_SetCustomNormal(BlActScene *scene, VecFx16 *normal) {
    scene->customNormal = TRUE;
    VEC_Fx16Set(&scene->normal, normal->x, normal->y, normal->z);
}

void BlActScene_SetScale(BlActScene *scene, const VecFx32 *scale) {
    scene->setup.scale = *scale;
}

void BlActScene_SetColDiffuse(BlActScene *scene, const GXRgb *color) {
    scene->setup.diffuse = *color;
}

void BlActScene_SetColAmbient(BlActScene *scene, const GXRgb *color) {
    scene->setup.ambient = *color;
}

void BlActScene_SetColSpecular(BlActScene *scene, const GXRgb *color) {
    scene->setup.specular = *color;
}

void BlActScene_SetColEmissive(BlActScene *scene, const GXRgb *color) {
    scene->setup.emission = *color;
}

void BlActScene_SetBasePolyID(BlActScene *scene, const u8 *id) {
    scene->setup.basePolygonId = *id;
}

void BlActScene_SetGeomOrigin(BlActScene *scene, u8 origin) {
    scene->setup.origin = origin;
}

void BlActScene_SetTexcoordOffset(BlActScene *scene, s16 s, s16 t) {
    scene->texOffsetS = s;
    scene->texOffsetT = t;
}

void func_0204e4d0(int size, u16 *width, u16 *height) {
    if (width != NULL) {
        *width = 1 << (((size & 0x70) >> 4) + 3);
    }
    if (height != NULL) {
        *height = 1 << ((size & 7) + 3);
    }
}

static BOOL BlActScene_GetAvailMatSlotIdx(BlActScene *scene, u32 *idx) {
    int i;
    int light;

    for (i = 0; i < scene->setup.materialCount; i++) {
        if (scene->materials[i].texResource == NULL && scene->materials[i].texFormat == BLACT_TEXFMT_FREE) {
            *idx = i;
            return TRUE;
        }
    }
    *idx = 0;
    return FALSE;
}

static void BlActMaterial_BindTexture(BlActMaterial *material, void *texResource) {
    material->texResource = texResource;
    if (!GFL_G3DResUploadTexData(texResource)) {
        material->texAddr = 0;
        material->plttAddr = 0;
        return;
    }
    material->texKey = GFL_G3DResGetTexVRAMHandle(texResource);
    material->plttKey = GFL_G3DResGetPltVRAMHandle(texResource);
    material->texAddr = NNS_GfdGetTexKeyAddr(material->texKey);
    material->plttAddr = NNS_GfdGetPlttKeyAddr(material->plttKey);
}

static inline void BlAct_GetTexSize(int size, u16 *width, u16 *height) {
    *width = 1 << (((size & 0x70) >> 4) + 3);
    *height = 1 << ((size & 7) + 3);
}

static void BlActMaterial_Set(BlActMaterial *material, u32 format, int size, u8 faceWidth, u8 faceHeight) {
    material->texFormat = BLACT_PIXEL_FORMATS[format];
    material->texSizeS = MMDL_BB_SIZETBL[(size & 0x70) >> 4].s;
    material->texSizeT = MMDL_BB_SIZETBL[size & 7].t;
    BlAct_GetTexSize(size, &material->texWidth, &material->texHeight);
    material->faceWidth = faceWidth;
    material->faceHeight = faceHeight;
    material->facesPerRow = material->texWidth / material->faceWidth;
    material->facesPerColumn = material->texHeight / material->faceHeight;
}

u32 BlActScene_AddMaterialNewTex(BlActScene *scene, void *texResource, u32 format, int size, u8 faceWidth,
                                 u8 faceHeight) {
    u32 idx;

    if (BlActScene_GetAvailMatSlotIdx(scene, &idx) == TRUE) {
        BlActMaterial_BindTexture(&scene->materials[idx], texResource);
        BlActMaterial_Set(&scene->materials[idx], format, size, faceWidth, faceHeight);
    }
    return idx;
}

u32 func_0204e614(BlActScene *scene, u32 arcId, u32 fileId, u32 format, int size, u8 faceWidth, u8 faceHeight) {
    u32 idx;

    if (BlActScene_GetAvailMatSlotIdx(scene, &idx) == TRUE) {
        BlActMaterial_BindTexture(&scene->materials[idx], GFL_G3DSysReadArcSysResource(arcId, fileId));
        BlActMaterial_Set(&scene->materials[idx], format, size, faceWidth, faceHeight);
    }
    return idx;
}

u32 BlActScene_AddMaterialExistTex(BlActScene *scene, NNSGfdTexKey texKey, NNSGfdPlttKey plttKey, u32 format, int size,
                                   u8 faceWidth, u8 faceHeight) {
    u32 idx;

    if (BlActScene_GetAvailMatSlotIdx(scene, &idx) == TRUE) {
        scene->materials[idx].texResource = NULL;
        scene->materials[idx].texKey = texKey;
        scene->materials[idx].plttKey = plttKey;
        scene->materials[idx].texAddr = NNS_GfdGetTexKeyAddr(scene->materials[idx].texKey);
        scene->materials[idx].plttAddr = NNS_GfdGetPlttKeyAddr(scene->materials[idx].plttKey);
        BlActMaterial_Set(&scene->materials[idx], format, size, faceWidth, faceHeight);
    }
    return idx;
}

void BlActScene_TrimMatTex(BlActScene *scene, u32 material) {
    void *texResource = scene->materials[material].texResource;
    void *data = GFL_G3DResGetResData(texResource);
    NNSG3dResTex *tex = GFL_G3DResGetTexData(texResource);

    GFL_HeapResize(data, (u8 *)tex + tex->texInfo.ofsTex - (u8 *)data);
}

void BlActScene_FreeMaterial(BlActScene *scene, u32 material) {
    if (scene->materials[material].texResource != NULL) {
        GFL_G3DResFreeTexData(scene->materials[material].texResource);
        GFL_G3DResFree(scene->materials[material].texResource);
        scene->materials[material].texResource = NULL;
    }
    scene->materials[material].texFormat = BLACT_TEXFMT_FREE;
}

void func_0204e73c(BlActScene *scene, u32 material) {
    if (scene->materials[material].texResource != NULL) {
        NNS_GfdFreeTexVram(scene->materials[material].texKey);
        NNS_GfdFreePlttVram(scene->materials[material].plttKey);
        scene->materials[material].texResource = NULL;
    }
    scene->materials[material].texFormat = BLACT_TEXFMT_FREE;
}

void func_0204e77c(BlActScene *scene, u32 material) {
    NNS_GfdFreeTexVram(scene->materials[material].texKey);
    NNS_GfdFreePlttVram(scene->materials[material].plttKey);
    scene->materials[material].texResource = NULL;
    scene->materials[material].texFormat = BLACT_TEXFMT_FREE;
}

void func_0204e7b8(BlActScene *scene) {
    int i;
    int light;

    for (i = 0; i < scene->setup.materialCount; i++) {
        BlActScene_FreeMaterial(scene, i);
    }
}

void func_0204e7d8(BlActScene *scene, u32 material, u32 *texAddr) {
    *texAddr = scene->materials[material].texAddr;
}

void func_0204e7e8(BlActScene *scene, u32 material, u32 *plttAddr) {
    *plttAddr = scene->materials[material].plttAddr;
}

void func_0204e7f8(BlActScene *scene, u32 material, NNSGfdTexKey *texKey) {
    *texKey = scene->materials[material].texKey;
}

void func_0204e808(BlActScene *scene, u32 material, NNSGfdTexKey texKey) {
    scene->materials[material].texKey = texKey;
    scene->materials[material].texAddr = NNS_GfdGetTexKeyAddr(scene->materials[material].texKey);
}

void func_0204e820(BlActScene *scene, u32 material, NNSGfdPlttKey *plttKey) {
    *plttKey = scene->materials[material].plttKey;
}

void func_0204e830(BlActScene *scene, u32 material, int face, u32 *offset, u32 *faceSize, u8 byRow) {
    BlActMaterial *mat = &scene->materials[material];

    switch (mat->texFormat) {
    case GX_TEXFMT_PLTT16:
        *faceSize = mat->faceWidth * 32 / 8 * mat->faceHeight / 8;
        break;
    case GX_TEXFMT_A3I5:
    case GX_TEXFMT_PLTT256:
    case GX_TEXFMT_A5I3:
        *faceSize = mat->faceWidth * 64 / 8 * mat->faceHeight / 8;
        break;
    case GX_TEXFMT_PLTT4:
        *faceSize = mat->faceWidth * 16 / 8 * mat->faceHeight / 8;
        break;
    }
    if (byRow == FALSE) {
        *offset = face * *faceSize;
    } else {
        int width = mat->texWidth;

        *offset = *faceSize * (face / width) + face % width * mat->faceWidth;
    }
}

u32 BlActScene_AddNewActor(BlActScene *scene, u32 material, s16 scaleX, s16 scaleY, const VecFx32 *pos, u8 alpha,
                           u32 lights, u32 type) {
    int i;
    int light;

    for (i = 0; i < scene->setup.actorCount; i++) {
        if (scene->actors[i].material == BLACT_MATERIAL_NONE) {
            scene->actors[i].material = material;
            scene->actors[i].type = type;
            scene->actors[i].scaleX = scaleX;
            scene->actors[i].scaleY = scaleY;
            scene->actors[i].rotation = 0;
            scene->actors[i].pos = *pos;
            scene->actors[i].alpha = alpha;
            scene->actors[i].polygonId = 0;
            scene->actors[i].visible = FALSE;
            scene->actors[i].lights = lights;
            scene->actors[i].flipS = FALSE;
            scene->actors[i].flipT = FALSE;
            scene->actors[i].face = 0;
            return i;
        }
    }
    return 0;
}

void BlActScene_ClearActorMaterial(BlActScene *scene, u32 actor) {
    if (scene->actors[actor].material != BLACT_MATERIAL_NONE) {
        scene->actors[actor].material = BLACT_MATERIAL_NONE;
    }
}

void func_0204e9f8(BlActScene *scene, u32 actor, u16 *material) {
    *material = scene->actors[actor].material;
}

void func_0204ea08(BlActScene *scene, u32 actor, const u16 *material) {
    scene->actors[actor].material = *material;
}

void BlActScene_SetActorPos(BlActScene *scene, u32 actor, const VecFx32 *pos) {
    scene->actors[actor].pos = *pos;
}

void func_0204ea3c(BlActScene *scene, u32 actor, u16 *face) {
    *face = scene->actors[actor].face;
}

void func_0204ea4c(BlActScene *scene, u32 actor, const u16 *face) {
    scene->actors[actor].face = *face;
}

void func_0204ea5c(BlActScene *scene, u32 actor, const s16 *scaleX, const s16 *scaleY) {
    scene->actors[actor].scaleX = *scaleX;
    scene->actors[actor].scaleY = *scaleY;
}

void func_0204ea78(BlActScene *scene, u32 actor, const u8 *alpha) {
    scene->actors[actor].alpha = *alpha;
}

void func_0204ea98(BlActScene *scene, u32 actor, const u8 *polygonId) {
    scene->actors[actor].polygonId = *polygonId;
}

BOOL func_0204eab8(BlActScene *scene, u32 actor) {
    return scene->actors[actor].visible;
}

void BlActScene_SetActorHidden(BlActScene *scene, u32 actor, const BOOL *hidden) {
    if (*hidden == TRUE) {
        scene->actors[actor].visible = TRUE;
    } else {
        scene->actors[actor].visible = FALSE;
    }
}

void BlActScene_EnableActorLight(BlActScene *scene, u32 actor, u32 lights) {
    scene->actors[actor].lights |= lights;
}

void BlActScene_DisableActorLight(BlActScene *scene, u32 actor, u32 lights) {
    scene->actors[actor].lights &= ~lights;
}

BOOL func_0204eb48(BlActScene *scene, u32 actor) {
    return scene->actors[actor].flipS;
}

void func_0204eb58(BlActScene *scene, u32 actor, const BOOL *flip) {
    scene->actors[actor].flipS = *flip;
}

void func_0204eb7c(BlActScene *scene, u32 actor, const BOOL *flip) {
    scene->actors[actor].flipT = *flip;
}

void func_0204eba0(BlActScene *scene, u32 actor, const u16 *rotation) {
    scene->actors[actor].rotation = *rotation;
}

void BlActScene_Draw(BlActScene *scene, G3DCamera *camera, G3DLight *lights) {
    BlActSceneSetup *setup = &scene->setup;
    VecFx32 camPos;
    VecFx32 camUp;
    VecFx32 target;
    VecFx32 camDir;
    MtxFx43 lookAt;
    MtxFx43 inv;
    VecFx32 dir;
    VecFx32 trans;
    MtxFx33 rots[4];
    VecFx16 lightDir;
    GXRgb lightColor;
    fx32 scale;
    fx32 s0;
    fx32 t0;
    fx32 s1;
    fx32 t1;
    s16 top;
    s16 bottom;
    s16 left;
    s16 right;
    int i;
    int light;
    fx16 normalX;
    fx16 normalY;
    fx16 normalZ;

    gfxReset3D();
    scene->keepViewMtx = FALSE;
    GFL_G3DCameraGetLookatPos(camera, &camPos);
    GFL_G3DCameraGetLookatUpVector(camera, &camUp);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    if (scene->keepViewMtx == FALSE) {
        gfxLookAt(&camPos, &camUp, &target, TRUE, &lookAt);
    } else {
        MTX_LookAt(&camPos, &camUp, &target, &lookAt);
    }
    MTX_Inverse43(&lookAt, &inv);
    MI_Copy36B(&inv, &rots[BLACT_TYPE_FACE_CAMERA]);

    // The camera's direction in the xz plane
    VEC_Subtract(&camPos, &target, &dir);
    dir.y = 0;
    VEC_Normalize(&dir, &dir);
    MTX_Identity33_(&rots[BLACT_TYPE_FACE_CAMERA_Y]);
    MTX_RotY33_(&rots[BLACT_TYPE_FACE_CAMERA_Y], dir.x, dir.z);
    MTX_RotY33_(&rots[BLACT_TYPE_FACE_CAMERA_Y_FLAT], dir.x, dir.z);
    MTX_Identity33_(&rots[BLACT_TYPE_FIXED]);

    VEC_Subtract(&camPos, &target, &camDir);
    VEC_Normalize(&camDir, &camDir);
    if (scene->customNormal == TRUE) {
        normalX = scene->normal.x;
        normalY = scene->normal.y;
        normalZ = scene->normal.z;
    } else {
        normalX = camDir.x;
        normalY = camDir.y;
        normalZ = camDir.z;
    }

    if (lights != NULL) {
        for (light = 0; light < 4; light++) {
            GFL_G3DLightGetDirVector(lights, light, &lightDir);
            GFL_G3DLightGetColor(lights, light, &lightColor);
            G3_LightVector(light, lightDir.x, lightDir.y, lightDir.z);
            G3_LightColor(light, lightColor);
        }
    }

    G3_MtxMode(GX_MTXMODE_POSITION);
    scale = setup->scale.x;
    G3_Scale(scale, scale, scale);
    for (i = 0; i < setup->actorCount; i++) {
        BlAct *actor = &scene->actors[i];
        BlActMaterial *mat;

        if (actor->material == BLACT_MATERIAL_NONE || !actor->visible) {
            continue;
        }
        mat = &scene->materials[actor->material];

        // The face's corners in the texture, read from the far side when flipped
        if (actor->flipS) {
            s1 = actor->face % mat->facesPerRow * mat->faceWidth << FX32_SHIFT;
            s1 += scene->texOffsetS;
            s0 = s1 + (mat->faceWidth << FX32_SHIFT);
        } else {
            s0 = actor->face % mat->facesPerRow * mat->faceWidth << FX32_SHIFT;
            s1 = s0 + (mat->faceWidth << FX32_SHIFT);
        }
        if (actor->flipT) {
            t1 = actor->face / mat->facesPerRow * mat->faceHeight << FX32_SHIFT;
            t1 += scene->texOffsetT;
            t0 = t1 + (mat->faceHeight << FX32_SHIFT);
        } else {
            t0 = actor->face / mat->facesPerRow * mat->faceHeight << FX32_SHIFT;
            t1 = t0 + (mat->faceHeight << FX32_SHIFT);
        }

        G3_PushMtx();
        trans.x = FX_Div(actor->pos.x, scale);
        trans.y = FX_Div(actor->pos.y, scale);
        trans.z = FX_Div(actor->pos.z, scale);
        gfxMultTransRot4x3(&rots[actor->type], &trans);
        gfxRotateZ(-FX_SinIdx(actor->rotation), FX_CosIdx(actor->rotation));
        if (actor->type == BLACT_TYPE_FACE_CAMERA_Y_FLAT) {
            G3_Scale(actor->scaleX, FX32_ONE, actor->scaleY);
        } else {
            G3_Scale(actor->scaleX, actor->scaleY, FX32_ONE);
        }
        G3_TexImageParam(mat->texFormat, GX_TEXGEN_TEXCOORD, mat->texSizeS, mat->texSizeT, GX_TEXREPEAT_NONE,
                         GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, mat->texAddr);
        G3_TexPlttBase(mat->plttAddr, mat->texFormat);
        G3_MaterialColorDiffAmb(setup->diffuse, setup->ambient, TRUE);
        G3_MaterialColorSpecEmi(setup->specular, setup->emission, FALSE);
        G3_PolygonAttr(actor->lights, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, setup->basePolygonId + actor->polygonId,
                       actor->alpha, GX_POLYGON_ATTR_MISC_FOG);
        top = BLACT_GEOM_QUADS[setup->origin].top;
        bottom = BLACT_GEOM_QUADS[setup->origin].bottom;
        left = BLACT_GEOM_QUADS[setup->origin].left;
        right = BLACT_GEOM_QUADS[setup->origin].right;
        G3_Begin(GX_BEGIN_QUADS);
        if (actor->lights) {
            G3_Normal(normalX, normalY, normalZ);
        } else {
            G3_Color(GX_RGB(31, 31, 31));
        }
        if (actor->type == BLACT_TYPE_FACE_CAMERA_Y_FLAT) {
            G3_TexCoord(s0, t0);
            G3_Vtx(left, 0, top);
            G3_TexCoord(s0, t1);
            G3_Vtx(left, 0, bottom);
            G3_TexCoord(s1, t1);
            G3_Vtx(right, 0, bottom);
            G3_TexCoord(s1, t0);
            G3_Vtx(right, 0, top);
        } else {
            G3_TexCoord(s0, t0);
            G3_Vtx(left, top, 0);
            G3_TexCoord(s0, t1);
            G3_Vtx(left, bottom, 0);
            G3_TexCoord(s1, t1);
            G3_Vtx(right, bottom, 0);
            G3_TexCoord(s1, t0);
            G3_Vtx(right, top, 0);
        }
        G3_End();
        G3_PopMtx(1);
    }
}

