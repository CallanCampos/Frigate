#include "types.h"
#include "gfl/billboard_actor.h"
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
} BillboardActorQuad;

// The GX_TEXSIZE_S* and GX_TEXSIZE_T* of each size code
typedef struct {
    u32 s;
    u32 t;
} BillboardActorTexSize;

// The GX_TEXFMT_* of each format a material is added with
const u32 BILLBOARD_ACTOR_PIXEL_FORMATS[5] = {
    GX_TEXFMT_PLTT16, GX_TEXFMT_PLTT256, GX_TEXFMT_PLTT4, GX_TEXFMT_A3I5, GX_TEXFMT_A5I3,
};
const BillboardActorTexSize ACTOR_BB_SIZETBL[8] = {
    { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 },
};
// clang-format off
const BillboardActorQuad BILLBOARD_ACTOR_GEOM_QUADS[9] = {
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

// clang-format on

BillboardActorScene *BillboardActorScene_Create(const BillboardActorSceneSetup *setup, HeapID heapId) {
    BillboardActorScene *scene = GFL_HeapAllocate(heapId, sizeof(BillboardActorScene), TRUE, "billboard.c", 153);
    int i;
    int light;

    scene->heapId = heapId;
    scene->setup = *setup;
    scene->materials =
        GFL_HeapAllocate(heapId, scene->setup.materialCount * sizeof(BillboardActorMaterial), TRUE, "billboard.c", 159);
    scene->actors =
        GFL_HeapAllocate(heapId, scene->setup.actorCount * sizeof(BillboardActor), TRUE, "billboard.c", 161);
    for (i = 0; i < scene->setup.materialCount; i++) {
        scene->materials[i].texResource = NULL;
        scene->materials[i].texFormat = BILLBOARD_ACTOR_TEXFMT_FREE;
    }
    for (i = 0; i < scene->setup.actorCount; i++) {
        scene->actors[i].material = BILLBOARD_ACTOR_MATERIAL_NONE;
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

void BillboardActorScene_Delete(BillboardActorScene *scene) {
    BillboardActorScene_FreeAllMaterials(scene);
    GFL_HeapFree(scene->actors);
    GFL_HeapFree(scene->materials);
    GFL_HeapFree(scene);
}

void BillboardActorScene_SetCustomNormal(BillboardActorScene *scene, VecFx16 *normal) {
    scene->customNormal = TRUE;
    VEC_Fx16Set(&scene->normal, normal->x, normal->y, normal->z);
}

void BillboardActorScene_SetScale(BillboardActorScene *scene, const VecFx32 *scale) {
    scene->setup.scale = *scale;
}

void BillboardActorScene_SetColDiffuse(BillboardActorScene *scene, const GXRgb *color) {
    scene->setup.diffuse = *color;
}

void BillboardActorScene_SetColAmbient(BillboardActorScene *scene, const GXRgb *color) {
    scene->setup.ambient = *color;
}

void BillboardActorScene_SetColSpecular(BillboardActorScene *scene, const GXRgb *color) {
    scene->setup.specular = *color;
}

void BillboardActorScene_SetColEmissive(BillboardActorScene *scene, const GXRgb *color) {
    scene->setup.emission = *color;
}

void BillboardActorScene_SetBasePolyID(BillboardActorScene *scene, const u8 *id) {
    scene->setup.basePolygonId = *id;
}

void BillboardActorScene_SetGeomOrigin(BillboardActorScene *scene, u8 origin) {
    scene->setup.origin = origin;
}

void BillboardActorScene_SetTexcoordOffset(BillboardActorScene *scene, s16 s, s16 t) {
    scene->texOffsetS = s;
    scene->texOffsetT = t;
}

void BillboardActor_GetTexDimensions(int size, u16 *width, u16 *height) {
    if (width != NULL) {
        *width = 1 << (((size & 0x70) >> 4) + 3);
    }
    if (height != NULL) {
        *height = 1 << ((size & 7) + 3);
    }
}

static BOOL BillboardActorScene_GetAvailMatSlotIdx(BillboardActorScene *scene, u32 *idx) {
    int i;
    int light;

    for (i = 0; i < scene->setup.materialCount; i++) {
        if (scene->materials[i].texResource == NULL && scene->materials[i].texFormat == BILLBOARD_ACTOR_TEXFMT_FREE) {
            *idx = i;
            return TRUE;
        }
    }
    *idx = 0;
    return FALSE;
}

static void BillboardActorMaterial_BindTexture(BillboardActorMaterial *material, G3DResource *texResource) {
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

static inline void BillboardActor_GetTexSize(int size, u16 *width, u16 *height) {
    *width = 1 << (((size & 0x70) >> 4) + 3);
    *height = 1 << ((size & 7) + 3);
}

static void BillboardActorMaterial_Set(BillboardActorMaterial *material, u32 format, int size, u8 faceWidth,
                                       u8 faceHeight) {
    material->texFormat = BILLBOARD_ACTOR_PIXEL_FORMATS[format];
    material->texSizeS = ACTOR_BB_SIZETBL[(size & 0x70) >> 4].s;
    material->texSizeT = ACTOR_BB_SIZETBL[size & 7].t;
    BillboardActor_GetTexSize(size, &material->texWidth, &material->texHeight);
    material->faceWidth = faceWidth;
    material->faceHeight = faceHeight;
    material->facesPerRow = material->texWidth / material->faceWidth;
    material->facesPerColumn = material->texHeight / material->faceHeight;
}

u32 BillboardActorScene_AddMaterialNewTex(BillboardActorScene *scene, G3DResource *texResource, u32 format, int size,
                                          u8 faceWidth, u8 faceHeight) {
    u32 idx;

    if (BillboardActorScene_GetAvailMatSlotIdx(scene, &idx) == TRUE) {
        BillboardActorMaterial_BindTexture(&scene->materials[idx], texResource);
        BillboardActorMaterial_Set(&scene->materials[idx], format, size, faceWidth, faceHeight);
    }
    return idx;
}

u32 BillboardActorScene_AddMaterialArcTex(BillboardActorScene *scene, u32 arcId, u32 fileId, u32 format, int size,
                                          u8 faceWidth, u8 faceHeight) {
    u32 idx;

    if (BillboardActorScene_GetAvailMatSlotIdx(scene, &idx) == TRUE) {
        BillboardActorMaterial_BindTexture(&scene->materials[idx], GFL_G3DSysReadArcSysResource(arcId, fileId));
        BillboardActorMaterial_Set(&scene->materials[idx], format, size, faceWidth, faceHeight);
    }
    return idx;
}

u32 BillboardActorScene_AddMaterialExistTex(BillboardActorScene *scene, NNSGfdTexKey texKey, NNSGfdPlttKey plttKey,
                                            u32 format, int size, u8 faceWidth, u8 faceHeight) {
    u32 idx;

    if (BillboardActorScene_GetAvailMatSlotIdx(scene, &idx) == TRUE) {
        scene->materials[idx].texResource = NULL;
        scene->materials[idx].texKey = texKey;
        scene->materials[idx].plttKey = plttKey;
        scene->materials[idx].texAddr = NNS_GfdGetTexKeyAddr(scene->materials[idx].texKey);
        scene->materials[idx].plttAddr = NNS_GfdGetPlttKeyAddr(scene->materials[idx].plttKey);
        BillboardActorMaterial_Set(&scene->materials[idx], format, size, faceWidth, faceHeight);
    }
    return idx;
}

void BillboardActorScene_TrimMatTex(BillboardActorScene *scene, u32 material) {
    G3DResource *texResource = scene->materials[material].texResource;
    NNSG3dResFileHeader *data = GFL_G3DResGetResData(texResource);
    NNSG3dResTex *tex = GFL_G3DResGetTexData(texResource);

    GFL_HeapResize(data, (u8 *)tex + tex->texInfo.ofsTex - (u8 *)data);
}

void BillboardActorScene_FreeMaterial(BillboardActorScene *scene, u32 material) {
    if (scene->materials[material].texResource != NULL) {
        GFL_G3DResFreeTexData(scene->materials[material].texResource);
        GFL_G3DResFree(scene->materials[material].texResource);
        scene->materials[material].texResource = NULL;
    }
    scene->materials[material].texFormat = BILLBOARD_ACTOR_TEXFMT_FREE;
}

void BillboardActorScene_FreeMaterialVramIfTex(BillboardActorScene *scene, u32 material) {
    if (scene->materials[material].texResource != NULL) {
        NNS_GfdFreeTexVram(scene->materials[material].texKey);
        NNS_GfdFreePlttVram(scene->materials[material].plttKey);
        scene->materials[material].texResource = NULL;
    }
    scene->materials[material].texFormat = BILLBOARD_ACTOR_TEXFMT_FREE;
}

void BillboardActorScene_FreeMaterialVram(BillboardActorScene *scene, u32 material) {
    NNS_GfdFreeTexVram(scene->materials[material].texKey);
    NNS_GfdFreePlttVram(scene->materials[material].plttKey);
    scene->materials[material].texResource = NULL;
    scene->materials[material].texFormat = BILLBOARD_ACTOR_TEXFMT_FREE;
}

void BillboardActorScene_FreeAllMaterials(BillboardActorScene *scene) {
    int i;
    int light;

    for (i = 0; i < scene->setup.materialCount; i++) {
        BillboardActorScene_FreeMaterial(scene, i);
    }
}

void BillboardActorScene_GetMaterialTexAddr(BillboardActorScene *scene, u32 material, u32 *texAddr) {
    *texAddr = scene->materials[material].texAddr;
}

void BillboardActorScene_GetMaterialPlttAddr(BillboardActorScene *scene, u32 material, u32 *plttAddr) {
    *plttAddr = scene->materials[material].plttAddr;
}

void BillboardActorScene_GetMaterialTexKey(BillboardActorScene *scene, u32 material, NNSGfdTexKey *texKey) {
    *texKey = scene->materials[material].texKey;
}

void BillboardActorScene_SetMaterialTexKey(BillboardActorScene *scene, u32 material, NNSGfdTexKey texKey) {
    scene->materials[material].texKey = texKey;
    scene->materials[material].texAddr = NNS_GfdGetTexKeyAddr(scene->materials[material].texKey);
}

void BillboardActorScene_GetMaterialPlttKey(BillboardActorScene *scene, u32 material, NNSGfdPlttKey *plttKey) {
    *plttKey = scene->materials[material].plttKey;
}

void BillboardActorScene_GetMaterialFaceLocation(BillboardActorScene *scene, u32 material, int face, u32 *offset,
                                                 u32 *faceSize, u8 byRow) {
    BillboardActorMaterial *mat = &scene->materials[material];

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

u32 BillboardActorScene_AddNewActor(BillboardActorScene *scene, u32 material, s16 scaleX, s16 scaleY,
                                    const VecFx32 *pos, u8 alpha, u32 lights, u32 type) {
    int i;
    int light;

    for (i = 0; i < scene->setup.actorCount; i++) {
        if (scene->actors[i].material == BILLBOARD_ACTOR_MATERIAL_NONE) {
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

void BillboardActorScene_ClearActorMaterial(BillboardActorScene *scene, u32 actor) {
    if (scene->actors[actor].material != BILLBOARD_ACTOR_MATERIAL_NONE) {
        scene->actors[actor].material = BILLBOARD_ACTOR_MATERIAL_NONE;
    }
}

void BillboardActorScene_GetActorMaterial(BillboardActorScene *scene, u32 actor, u16 *material) {
    *material = scene->actors[actor].material;
}

void BillboardActorScene_SetActorMaterial(BillboardActorScene *scene, u32 actor, const u16 *material) {
    scene->actors[actor].material = *material;
}

void BillboardActorScene_SetActorPos(BillboardActorScene *scene, u32 actor, const VecFx32 *pos) {
    scene->actors[actor].pos = *pos;
}

void BillboardActorScene_GetActorFace(BillboardActorScene *scene, u32 actor, u16 *face) {
    *face = scene->actors[actor].face;
}

void BillboardActorScene_SetActorFace(BillboardActorScene *scene, u32 actor, const u16 *face) {
    scene->actors[actor].face = *face;
}

void BillboardActorScene_SetActorScale(BillboardActorScene *scene, u32 actor, const s16 *scaleX, const s16 *scaleY) {
    scene->actors[actor].scaleX = *scaleX;
    scene->actors[actor].scaleY = *scaleY;
}

void BillboardActorScene_SetActorAlpha(BillboardActorScene *scene, u32 actor, const u8 *alpha) {
    scene->actors[actor].alpha = *alpha;
}

void BillboardActorScene_SetActorPolygonId(BillboardActorScene *scene, u32 actor, const u8 *polygonId) {
    scene->actors[actor].polygonId = *polygonId;
}

BOOL BillboardActorScene_IsActorVisible(BillboardActorScene *scene, u32 actor) {
    return scene->actors[actor].visible;
}

void BillboardActorScene_SetActorHidden(BillboardActorScene *scene, u32 actor, const BOOL *hidden) {
    if (*hidden == TRUE) {
        scene->actors[actor].visible = TRUE;
    } else {
        scene->actors[actor].visible = FALSE;
    }
}

void BillboardActorScene_EnableActorLight(BillboardActorScene *scene, u32 actor, u32 lights) {
    scene->actors[actor].lights |= lights;
}

void BillboardActorScene_DisableActorLight(BillboardActorScene *scene, u32 actor, u32 lights) {
    scene->actors[actor].lights &= ~lights;
}

BOOL BillboardActorScene_GetActorFlipS(BillboardActorScene *scene, u32 actor) {
    return scene->actors[actor].flipS;
}

void BillboardActorScene_SetActorFlipS(BillboardActorScene *scene, u32 actor, const BOOL *flip) {
    scene->actors[actor].flipS = *flip;
}

void BillboardActorScene_SetActorFlipT(BillboardActorScene *scene, u32 actor, const BOOL *flip) {
    scene->actors[actor].flipT = *flip;
}

void BillboardActorScene_SetActorRotation(BillboardActorScene *scene, u32 actor, const u16 *rotation) {
    scene->actors[actor].rotation = *rotation;
}

void BillboardActorScene_Draw(BillboardActorScene *scene, G3DCamera *camera, G3DLight *lights) {
    BillboardActorSceneSetup *setup = &scene->setup;
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

    G3X_Reset();
    scene->keepViewMtx = FALSE;
    GFL_G3DCameraGetLookatPos(camera, &camPos);
    GFL_G3DCameraGetLookatUpVector(camera, &camUp);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    if (scene->keepViewMtx == FALSE) {
        G3i_LookAt_(&camPos, &camUp, &target, TRUE, &lookAt);
    } else {
        MTX_LookAt(&camPos, &camUp, &target, &lookAt);
    }
    MTX_Inverse43(&lookAt, &inv);
    MI_Copy36B(&inv, &rots[BILLBOARD_ACTOR_TYPE_FACE_CAMERA]);

    // The camera's direction in the xz plane
    VEC_Subtract(&camPos, &target, &dir);
    dir.y = 0;
    VEC_Normalize(&dir, &dir);
    MTX_Identity33_(&rots[BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y]);
    MTX_RotY33_(&rots[BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y], dir.x, dir.z);
    MTX_RotY33_(&rots[BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y_FLAT], dir.x, dir.z);
    MTX_Identity33_(&rots[BILLBOARD_ACTOR_TYPE_FIXED]);

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
        BillboardActor *actor = &scene->actors[i];
        BillboardActorMaterial *mat;

        if (actor->material == BILLBOARD_ACTOR_MATERIAL_NONE || !actor->visible) {
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
        G3_MultTransMtx33(&rots[actor->type], &trans);
        G3_RotZ(-FX_SinIdx(actor->rotation), FX_CosIdx(actor->rotation));
        if (actor->type == BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y_FLAT) {
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
        top = BILLBOARD_ACTOR_GEOM_QUADS[setup->origin].top;
        bottom = BILLBOARD_ACTOR_GEOM_QUADS[setup->origin].bottom;
        left = BILLBOARD_ACTOR_GEOM_QUADS[setup->origin].left;
        right = BILLBOARD_ACTOR_GEOM_QUADS[setup->origin].right;
        G3_Begin(GX_BEGIN_QUADS);
        if (actor->lights) {
            G3_Normal(normalX, normalY, normalZ);
        } else {
            G3_Color(GX_RGB(31, 31, 31));
        }
        if (actor->type == BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y_FLAT) {
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
