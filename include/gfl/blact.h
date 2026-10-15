#ifndef POKEBW2_GFL_BLACT_H
#define POKEBW2_GFL_BLACT_H

#include "types.h"
#include "gfl/areaman.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/gfd.h"

// Billboard actors: textured quads drawn facing the camera, each showing one face of its material's texture, which
// holds the faces in rows

// No material
#define BLACT_MATERIAL_NONE 0x3fff

// A material whose slot is free
#define BLACT_TEXFMT_FREE 5

// Where an actor's quad is from its position: BLACT_ORIGIN_*
enum {
    BLACT_ORIGIN_CENTER,
    BLACT_ORIGIN_CENTER_TOP,
    BLACT_ORIGIN_CENTER_BOTTOM,
    BLACT_ORIGIN_LEFT_CENTER,
    BLACT_ORIGIN_RIGHT_CENTER,
    BLACT_ORIGIN_TOP_LEFT,
    BLACT_ORIGIN_TOP_RIGHT,
    BLACT_ORIGIN_BOTTOM_LEFT,
    BLACT_ORIGIN_BOTTOM_RIGHT,
};

// How an actor faces the camera: fully, turning only about the y axis, not at all, or turning about the y axis while
// lying in the xz plane
enum {
    BLACT_TYPE_FACE_CAMERA,
    BLACT_TYPE_FACE_CAMERA_Y,
    BLACT_TYPE_FIXED,
    BLACT_TYPE_FACE_CAMERA_Y_FLAT,
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
} BlActSceneSetup;

typedef struct {
    void *texResource;
    u16 texWidth;
    u16 texHeight;
    u8 faceWidth;
    u8 faceHeight;
    u8 facesPerRow;
    u8 facesPerColumn;
    u32 texAddr;
    u32 plttAddr;
    // A GX_TEXFMT_*, or BLACT_TEXFMT_FREE
    u32 texFormat;
    u32 texSizeS;
    u32 texSizeT;
    NNSGfdTexKey texKey;
    NNSGfdPlttKey plttKey;
} BlActMaterial;

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
} BlAct;

typedef struct {
    HeapID heapId;
    BlActMaterial *materials;
    BlAct *actors;
    BlActSceneSetup setup;
    // Whether the camera is only computed, without being loaded into the geometry engine
    BOOL keepViewMtx;
    BOOL customNormal;
    VecFx16 normal;
    s16 texOffsetS;
    s16 texOffsetT;
} BlActScene;

BlActScene *BlActScene_Create(const BlActSceneSetup *setup, HeapID heapId);
void func_0204e450(BlActScene *scene);
void BlActScene_SetCustomNormal(BlActScene *scene, VecFx16 *normal);
void BlActScene_SetScale(BlActScene *scene, const VecFx32 *scale);
void BlActScene_SetColDiffuse(BlActScene *scene, const GXRgb *color);
void BlActScene_SetColAmbient(BlActScene *scene, const GXRgb *color);
void BlActScene_SetColSpecular(BlActScene *scene, const GXRgb *color);
void BlActScene_SetColEmissive(BlActScene *scene, const GXRgb *color);
void BlActScene_SetBasePolyID(BlActScene *scene, const u8 *id);
void BlActScene_SetGeomOrigin(BlActScene *scene, u8 origin);
void BlActScene_SetTexcoordOffset(BlActScene *scene, s16 s, s16 t);
// The width and height of a texture of a BLACT size, its width's GX_TEXSIZE_S* in bits 4 to 6 and its height's in
// bits 0 to 2
void func_0204e4d0(int size, u16 *width, u16 *height);

// Add a material of a texture resource, of one read from an archive, or of a texture already in VRAM, faceWidth by
// faceHeight pixels a face, and return its index
u32 BlActScene_AddMaterialNewTex(BlActScene *scene, void *texResource, u32 format, int size, u8 faceWidth,
                                 u8 faceHeight);
u32 func_0204e614(BlActScene *scene, u32 arcId, u32 fileId, u32 format, int size, u8 faceWidth, u8 faceHeight);
u32 BlActScene_AddMaterialExistTex(BlActScene *scene, NNSGfdTexKey texKey, NNSGfdPlttKey plttKey, u32 format, int size,
                                   u8 faceWidth, u8 faceHeight);
// Free a material's texture image from main memory, once it is in VRAM
void BlActScene_TrimMatTex(BlActScene *scene, u32 material);
void BlActScene_FreeMaterial(BlActScene *scene, u32 material);
// Free a material that has its own VRAM, if it has a texture, or always
void func_0204e73c(BlActScene *scene, u32 material);
void func_0204e77c(BlActScene *scene, u32 material);
void func_0204e7b8(BlActScene *scene);
void func_0204e7d8(BlActScene *scene, u32 material, u32 *texAddr);
void func_0204e7e8(BlActScene *scene, u32 material, u32 *plttAddr);
void func_0204e7f8(BlActScene *scene, u32 material, NNSGfdTexKey *texKey);
void func_0204e808(BlActScene *scene, u32 material, NNSGfdTexKey texKey);
void func_0204e820(BlActScene *scene, u32 material, NNSGfdPlttKey *plttKey);
// Where a face is in its material's texture, and how big it is
void func_0204e830(BlActScene *scene, u32 material, int face, u32 *offset, u32 *faceSize, u8 byRow);

u32 BlActScene_AddNewActor(BlActScene *scene, u32 material, s16 scaleX, s16 scaleY, const VecFx32 *pos, u8 alpha,
                           u32 lights, u32 type);
void BlActScene_ClearActorMaterial(BlActScene *scene, u32 actor);
void func_0204e9f8(BlActScene *scene, u32 actor, u16 *material);
void func_0204ea08(BlActScene *scene, u32 actor, const u16 *material);
void BlActScene_SetActorPos(BlActScene *scene, u32 actor, const VecFx32 *pos);
void func_0204ea3c(BlActScene *scene, u32 actor, u16 *face);
void func_0204ea4c(BlActScene *scene, u32 actor, const u16 *face);
void func_0204ea5c(BlActScene *scene, u32 actor, const s16 *scaleX, const s16 *scaleY);
void func_0204ea78(BlActScene *scene, u32 actor, const u8 *alpha);
void func_0204ea98(BlActScene *scene, u32 actor, const u8 *polygonId);
BOOL func_0204eab8(BlActScene *scene, u32 actor);
// Despite its name, shows the actor when hidden is TRUE
void BlActScene_SetActorHidden(BlActScene *scene, u32 actor, const BOOL *hidden);
void BlActScene_EnableActorLight(BlActScene *scene, u32 actor, u32 lights);
void BlActScene_DisableActorLight(BlActScene *scene, u32 actor, u32 lights);
BOOL func_0204eb48(BlActScene *scene, u32 actor);
void func_0204eb58(BlActScene *scene, u32 actor, const BOOL *flip);
void func_0204eb7c(BlActScene *scene, u32 actor, const BOOL *flip);
void func_0204eba0(BlActScene *scene, u32 actor, const u16 *rotation);
void BlActScene_Draw(BlActScene *scene, G3DCamera *camera, G3DLight *lights);

// The system over a scene: materials and actors allocated in runs, and actors that play animations of faces

// No actor or material
#define BLACT_NONE 0xffff

// An animation's step: show a face for wait frames, or, when face is one of these, a command with arg
#define BLACT_ANIM_CMD_GOTO 0x3ffc
#define BLACT_ANIM_CMD_LOOP 0x3ffd
#define BLACT_ANIM_CMD_CHANGE 0x3ffe
#define BLACT_ANIM_CMD_END 0x3fff

typedef struct {
    u16 face : 14;
    u16 flipS : 1;
    u16 flipT : 1;
    // The frames to wait, or the command's argument: the step to go to, the animation to change to, or the step to
    // loop back to in the low byte and how many times in the high byte
    u16 arg;
} BlActAnimStep;

typedef struct BlActSys BlActSys;

// Sends a face's characters or a palette, size bytes, to VRAM
typedef void (*BlActTransferFunc)(BOOL pltt, u32 dst, void *src, u32 size);
typedef void (*BlActCallback)(BlActSys *sys, u32 actor, void *data);

typedef struct {
    u16 material;
    void *texResource;
} BlActTexMat;

typedef struct {
    u32 actor;
    u16 texMat;
    const BlActAnimStep *const *anims;
    u16 animCount;
    u16 anim;
    u16 step;
    u16 wait;
    u16 loops;
    BOOL animate;
    // The last command run
    u16 cmd;
    BlActCallback callback;
    void *callbackData;
} BlActHandle;

struct BlActSys {
    HeapID heapId;
    BlActScene *scene;
    BlActTransferFunc transfer;
    BlActTexMat *texMats;
    u16 texMatCount;
    AreaMan *texMatAlloc;
    BlActHandle *handles;
    u16 handleCount;
    AreaMan *handleAlloc;
};

// How a material's texture is kept: in main memory, without its image once in VRAM, or not loaded into the scene,
// for actors that transfer their faces
enum {
    BLACT_LOAD_KEEP,
    BLACT_LOAD_TRIM,
    BLACT_LOAD_TRANSFER,
};

typedef struct {
    void *texResource;
    u8 format;
    u8 size;
    u8 faceWidth;
    u8 faceHeight;
    u32 mode;
} BlActMatRequest;

typedef struct {
    u16 texMat;
    s16 scaleX;
    s16 scaleY;
    u8 alpha;
    BOOL visible;
    u32 lights;
    VecFx32 pos;
    BlActCallback callback;
    void *callbackData;
} BlActActorRequest;

BlActSys *BlActSys_Create(u16 texMatCount, u16 handleCount, BlActTransferFunc transfer, HeapID heapId);
void BlActSys_Free(BlActSys *sys);
BlActScene *BlActSys_GetScene(BlActSys *sys);
// Load materials or actors and return the first's index, or BLACT_NONE for actors that do not fit
u16 BlActSys_ExecMatLoadRequests(BlActSys *sys, const BlActMatRequest *requests, u32 count);
u16 func_0204f31c(BlActSys *sys, u16 material, void *texResource);
void BlActSys_FreeMaterials(BlActSys *sys, u16 first, u32 count);
void func_0204f3a0(BlActSys *sys, u32 texMat);
u16 func_0204f3bc(BlActSys *sys, u32 texMat);
u16 BlActSys_ExecActorRequests(BlActSys *sys, u16 firstTexMat, const BlActActorRequest *requests, u32 count, u32 type);
void BlActSys_DeleteActors(BlActSys *sys, u32 first, u32 count);
void BlActSys_UpdateActors(BlActSys *sys);
void BlActSys_Draw(BlActSys *sys, G3DCamera *camera, G3DLight *lights);
void func_0204f664(BlActSys *sys, u32 actor, BOOL visible);
BOOL func_0204f684(BlActSys *sys, u32 actor);
void BlActSys_SetActorLight(BlActSys *sys, u32 actor, BOOL enable, u32 light);
void func_0204f6c8(BlActSys *sys, u32 actor, BOOL animate);
void func_0204f6d4(BlActSys *sys, u32 actor, const BlActAnimStep *const *anims, u16 animCount);
void func_0204f6ec(BlActSys *sys, u32 actor, u16 anim);
u16 func_0204f700(BlActSys *sys, u32 actor);
void func_0204f70c(BlActSys *sys, u32 actor, u16 step);
u16 func_0204f724(BlActSys *sys, u32 actor);
// Make the actor's material use another's palette
void func_0204f730(BlActSys *sys, u32 actor, u32 texMat);
u32 func_0204f750(BlActSys *sys, u32 actor);
u16 BlActSys_GetMatByActor(BlActSys *sys, u32 actor);
// The last command the actor's animation ran, and whether it was one
BOOL func_0204f768(BlActSys *sys, u32 actor, u16 *cmd);

#endif // POKEBW2_GFL_BLACT_H
