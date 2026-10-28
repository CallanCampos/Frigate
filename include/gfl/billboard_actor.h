#ifndef POKEBW2_GFL_BILLBOARD_ACTOR_H
#define POKEBW2_GFL_BILLBOARD_ACTOR_H

#include "types.h"
#include "gfl/areaman.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/gfd.h"

// Billboard actors: textured quads drawn facing the camera, each showing one face of its material's texture, which
// holds the faces in rows. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) in English: swan's
// BlAct is BillboardActor

// No material
#define BILLBOARD_ACTOR_MATERIAL_NONE 0x3fff

// A material whose slot is free
#define BILLBOARD_ACTOR_TEXFMT_FREE 5

// Where an actor's quad is from its position: BILLBOARD_ACTOR_ORIGIN_*
enum {
    BILLBOARD_ACTOR_ORIGIN_CENTER,
    BILLBOARD_ACTOR_ORIGIN_CENTER_TOP,
    BILLBOARD_ACTOR_ORIGIN_CENTER_BOTTOM,
    BILLBOARD_ACTOR_ORIGIN_LEFT_CENTER,
    BILLBOARD_ACTOR_ORIGIN_RIGHT_CENTER,
    BILLBOARD_ACTOR_ORIGIN_TOP_LEFT,
    BILLBOARD_ACTOR_ORIGIN_TOP_RIGHT,
    BILLBOARD_ACTOR_ORIGIN_BOTTOM_LEFT,
    BILLBOARD_ACTOR_ORIGIN_BOTTOM_RIGHT,
};

// How an actor faces the camera: fully, turning only about the y axis, not at all, or turning about the y axis while
// lying in the xz plane
enum {
    BILLBOARD_ACTOR_TYPE_FACE_CAMERA,
    BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y,
    BILLBOARD_ACTOR_TYPE_FIXED,
    BILLBOARD_ACTOR_TYPE_FACE_CAMERA_Y_FLAT,
};

typedef struct {
    u16 materialCount;
    u16 actorCount;
    VecFx32 scale;
    GXRgb diffuse;
    GXRgb ambient;
    GXRgb specular;
    GXRgb emission;
    u8 basePolygonId;
    u8 origin;
} BillboardActorSceneSetup;

typedef struct {
    G3DResource *texResource;
    u16 texWidth;
    u16 texHeight;
    u8 faceWidth;
    u8 faceHeight;
    u8 facesPerRow;
    u8 facesPerColumn;
    u32 texAddr;
    u32 plttAddr;
    // A GX_TEXFMT_*, or BILLBOARD_ACTOR_TEXFMT_FREE
    u32 texFormat;
    u32 texSizeS;
    u32 texSizeT;
    NNSGfdTexKey texKey;
    NNSGfdPlttKey plttKey;
} BillboardActorMaterial;

typedef struct {
    u16 material : 14;
    u16 type : 2;
    VecFx32 pos;
    u16 face;
    s16 scaleX;
    s16 scaleY;
    u16 rotation;
    u16 alpha : 5;
    u16 polygonId : 4;
    u16 visible : 1;
    // Faces are read right to left or bottom to top
    u16 flipS : 1;
    u16 flipT : 1;
    // GX_LIGHTMASK_*
    u16 lights : 4;
} BillboardActor;

typedef struct {
    HeapID heapId;
    BillboardActorMaterial *materials;
    BillboardActor *actors;
    BillboardActorSceneSetup setup;
    // Whether the camera is only computed, without being loaded into the geometry engine
    BOOL keepViewMtx;
    BOOL customNormal;
    VecFx16 normal;
    s16 texOffsetS;
    s16 texOffsetT;
} BillboardActorScene;

BillboardActorScene *BillboardActorScene_Create(const BillboardActorSceneSetup *setup, HeapID heapId);
void BillboardActorScene_Delete(BillboardActorScene *scene);
void BillboardActorScene_SetCustomNormal(BillboardActorScene *scene, VecFx16 *normal);
void BillboardActorScene_SetScale(BillboardActorScene *scene, const VecFx32 *scale);
void BillboardActorScene_SetColDiffuse(BillboardActorScene *scene, const GXRgb *color);
void BillboardActorScene_SetColAmbient(BillboardActorScene *scene, const GXRgb *color);
void BillboardActorScene_SetColSpecular(BillboardActorScene *scene, const GXRgb *color);
void BillboardActorScene_SetColEmissive(BillboardActorScene *scene, const GXRgb *color);
void BillboardActorScene_SetBasePolyID(BillboardActorScene *scene, const u8 *id);
void BillboardActorScene_SetGeomOrigin(BillboardActorScene *scene, u8 origin);
void BillboardActorScene_SetTexcoordOffset(BillboardActorScene *scene, s16 s, s16 t);
// The width and height of a texture of a billboard actor size, its width's GX_TEXSIZE_S* in bits 4 to 6 and its
// height's in bits 0 to 2
void BillboardActor_GetTexDimensions(int size, u16 *width, u16 *height);

// Add a material of a texture resource, of one read from an archive, or of a texture already in VRAM, faceWidth by
// faceHeight pixels a face, and return its index
u32 BillboardActorScene_AddMaterialNewTex(BillboardActorScene *scene, G3DResource *texResource, u32 format, int size,
                                          u8 faceWidth, u8 faceHeight);
u32 BillboardActorScene_AddMaterialArcTex(BillboardActorScene *scene, u32 arcId, u32 fileId, u32 format, int size,
                                          u8 faceWidth, u8 faceHeight);
u32 BillboardActorScene_AddMaterialExistTex(BillboardActorScene *scene, NNSGfdTexKey texKey, NNSGfdPlttKey plttKey,
                                            u32 format, int size, u8 faceWidth, u8 faceHeight);
// Free a material's texture image from main memory, once it is in VRAM
void BillboardActorScene_TrimMatTex(BillboardActorScene *scene, u32 material);
void BillboardActorScene_FreeMaterial(BillboardActorScene *scene, u32 material);
// Free a material that has its own VRAM, if it has a texture, or always
void BillboardActorScene_FreeMaterialVramIfTex(BillboardActorScene *scene, u32 material);
void BillboardActorScene_FreeMaterialVram(BillboardActorScene *scene, u32 material);
void BillboardActorScene_FreeAllMaterials(BillboardActorScene *scene);
void BillboardActorScene_GetMaterialTexAddr(BillboardActorScene *scene, u32 material, u32 *texAddr);
void BillboardActorScene_GetMaterialPlttAddr(BillboardActorScene *scene, u32 material, u32 *plttAddr);
void BillboardActorScene_GetMaterialTexKey(BillboardActorScene *scene, u32 material, NNSGfdTexKey *texKey);
void BillboardActorScene_SetMaterialTexKey(BillboardActorScene *scene, u32 material, NNSGfdTexKey texKey);
void BillboardActorScene_GetMaterialPlttKey(BillboardActorScene *scene, u32 material, NNSGfdPlttKey *plttKey);
// Where a face is in its material's texture, and how big it is
void BillboardActorScene_GetMaterialFaceLocation(BillboardActorScene *scene, u32 material, int face, u32 *offset,
                                                 u32 *faceSize, u8 byRow);

u32 BillboardActorScene_AddNewActor(BillboardActorScene *scene, u32 material, s16 scaleX, s16 scaleY,
                                    const VecFx32 *pos, u8 alpha, u32 lights, u32 type);
void BillboardActorScene_ClearActorMaterial(BillboardActorScene *scene, u32 actor);
void BillboardActorScene_GetActorMaterial(BillboardActorScene *scene, u32 actor, u16 *material);
void BillboardActorScene_SetActorMaterial(BillboardActorScene *scene, u32 actor, const u16 *material);
void BillboardActorScene_SetActorPos(BillboardActorScene *scene, u32 actor, const VecFx32 *pos);
void BillboardActorScene_GetActorFace(BillboardActorScene *scene, u32 actor, u16 *face);
void BillboardActorScene_SetActorFace(BillboardActorScene *scene, u32 actor, const u16 *face);
void BillboardActorScene_SetActorScale(BillboardActorScene *scene, u32 actor, const s16 *scaleX, const s16 *scaleY);
void BillboardActorScene_SetActorAlpha(BillboardActorScene *scene, u32 actor, const u8 *alpha);
void BillboardActorScene_SetActorPolygonId(BillboardActorScene *scene, u32 actor, const u8 *polygonId);
BOOL BillboardActorScene_IsActorVisible(BillboardActorScene *scene, u32 actor);
// Despite its name, shows the actor when hidden is TRUE
void BillboardActorScene_SetActorHidden(BillboardActorScene *scene, u32 actor, const BOOL *hidden);
void BillboardActorScene_EnableActorLight(BillboardActorScene *scene, u32 actor, u32 lights);
void BillboardActorScene_DisableActorLight(BillboardActorScene *scene, u32 actor, u32 lights);
BOOL BillboardActorScene_GetActorFlipS(BillboardActorScene *scene, u32 actor);
void BillboardActorScene_SetActorFlipS(BillboardActorScene *scene, u32 actor, const BOOL *flip);
void BillboardActorScene_SetActorFlipT(BillboardActorScene *scene, u32 actor, const BOOL *flip);
void BillboardActorScene_SetActorRotation(BillboardActorScene *scene, u32 actor, const u16 *rotation);
void BillboardActorScene_Draw(BillboardActorScene *scene, G3DCamera *camera, G3DLight *lights);

// The system over a scene: materials and actors allocated in runs, and actors that play animations of faces

// No actor or material
#define BILLBOARD_ACTOR_NONE 0xffff

// An animation's step: show a face for wait frames, or, when face is one of these, a command with arg
enum {
    BILLBOARD_ACTOR_ANIM_CMD_GOTO = 0x3ffc,
    BILLBOARD_ACTOR_ANIM_CMD_LOOP,
    BILLBOARD_ACTOR_ANIM_CMD_CHANGE,
    BILLBOARD_ACTOR_ANIM_CMD_END,
};

typedef struct {
    u16 face : 14;
    u16 flipS : 1;
    u16 flipT : 1;
    // The frames to wait, or the command's argument: the step to go to, the animation to change to, or the step to
    // loop back to in the low byte and how many times in the high byte
    u16 arg;
} BillboardActorAnimStep;

typedef struct BillboardActorSys BillboardActorSys;

// Sends a face's characters or a palette, size bytes, to VRAM
typedef void (*BillboardActorTransferFunc)(BOOL pltt, u32 dst, void *src, u32 size);
typedef void (*BillboardActorCallback)(BillboardActorSys *sys, u32 actor, void *data);

typedef struct {
    u16 material;
    G3DResource *texResource;
} BillboardActorTexMat;

typedef struct {
    u32 actor;
    u16 texMat;
    const BillboardActorAnimStep *const *anims;
    u16 animCount;
    u16 anim;
    u16 step;
    u16 wait;
    u16 loops;
    BOOL animate;
    // The last command run
    u16 cmd;
    BillboardActorCallback callback;
    void *callbackData;
} BillboardActorHandle;

struct BillboardActorSys {
    HeapID heapId;
    BillboardActorScene *scene;
    BillboardActorTransferFunc transfer;
    BillboardActorTexMat *texMats;
    u16 texMatCount;
    AreaMan *texMatAlloc;
    BillboardActorHandle *handles;
    u16 handleCount;
    AreaMan *handleAlloc;
};

// How a material's texture is kept: in main memory, without its image once in VRAM, or not loaded into the scene,
// for actors that transfer their faces
enum {
    BILLBOARD_ACTOR_LOAD_KEEP,
    BILLBOARD_ACTOR_LOAD_TRIM,
    BILLBOARD_ACTOR_LOAD_TRANSFER,
};

typedef struct {
    G3DResource *texResource;
    u8 format;
    u8 size;
    u8 faceWidth;
    u8 faceHeight;
    u32 mode;
} BillboardActorMatRequest;

typedef struct {
    u16 texMat;
    s16 scaleX;
    s16 scaleY;
    u8 alpha;
    BOOL visible;
    u32 lights;
    VecFx32 pos;
    BillboardActorCallback callback;
    void *callbackData;
} BillboardActorRequest;

BillboardActorSys *BillboardActorSys_Create(u16 texMatCount, u16 handleCount, BillboardActorTransferFunc transfer,
                                            HeapID heapId);
void BillboardActorSys_Free(BillboardActorSys *sys);
BillboardActorScene *BillboardActorSys_GetScene(BillboardActorSys *sys);
// Load materials or actors and return the first's index, or BILLBOARD_ACTOR_NONE for actors that do not fit
u16 BillboardActorSys_ExecMatLoadRequests(BillboardActorSys *sys, const BillboardActorMatRequest *requests, u32 count);
// Bind a scene material to a new texture material and return its index. The material is stored as a u16, but
// field_actor_billboard.c passes BillboardActorScene_AddMaterialExistTex's u32 result without narrowing it
u16 BillboardActorSys_AddTexMat(BillboardActorSys *sys, u32 material, G3DResource *texResource);
void BillboardActorSys_FreeMaterials(BillboardActorSys *sys, u16 first, u32 count);
void BillboardActorSys_FreeTexMat(BillboardActorSys *sys, u32 texMat);
u16 BillboardActorSys_GetTexMatMaterial(BillboardActorSys *sys, u32 texMat);
u16 BillboardActorSys_ExecActorRequests(BillboardActorSys *sys, u16 firstTexMat, const BillboardActorRequest *requests,
                                        u32 count, u32 type);
void BillboardActorSys_DeleteActors(BillboardActorSys *sys, u32 first, u32 count);
void BillboardActorSys_UpdateActors(BillboardActorSys *sys);
void BillboardActorSys_Draw(BillboardActorSys *sys, G3DCamera *camera, G3DLight *lights);
void BillboardActorSys_SetActorVisible(BillboardActorSys *sys, u32 actor, BOOL visible);
// A u16 index, which fldeff_reflect.c's u32 index is narrowed to at each call, as for BillboardActorSys_GetSceneActor
BOOL BillboardActorSys_IsActorVisible(BillboardActorSys *sys, u16 actor);
void BillboardActorSys_SetActorLight(BillboardActorSys *sys, u32 actor, BOOL enable, u32 light);
void BillboardActorSys_SetActorAnimating(BillboardActorSys *sys, u32 actor, BOOL animate);
void BillboardActorSys_SetActorAnims(BillboardActorSys *sys, u32 actor, const BillboardActorAnimStep *const *anims,
                                     u16 animCount);
void BillboardActorSys_SetActorAnim(BillboardActorSys *sys, u32 actor, u16 anim);
u16 BillboardActorSys_GetActorAnim(BillboardActorSys *sys, u32 actor);
void BillboardActorSys_SetActorStep(BillboardActorSys *sys, u32 actor, u16 step);
u16 BillboardActorSys_GetActorStep(BillboardActorSys *sys, u32 actor);
// Make the actor's material use another's palette
void BillboardActorSys_ShareActorPltt(BillboardActorSys *sys, u32 actor, u32 texMat);
u32 BillboardActorSys_GetSceneActor(BillboardActorSys *sys, u16 actor);
u16 BillboardActorSys_GetMatByActor(BillboardActorSys *sys, u32 actor);
// The last command the actor's animation ran, and whether it was one
BOOL BillboardActorSys_GetActorLastCmd(BillboardActorSys *sys, u16 actor, u16 *cmd);

#endif // POKEBW2_GFL_BILLBOARD_ACTOR_H
