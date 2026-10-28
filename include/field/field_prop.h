#ifndef POKEBW2_FIELD_FIELD_PROP_H
#define POKEBW2_FIELD_FIELD_PROP_H

#include "types.h"
#include "field/field_map_chunk.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The most resource infos a prop system has, which is also what resIdToIndex holds for a resource it has none of
#define FIELD_PROP_RES_INFO_MAX 0x80
// A resource info without a door, an animation slot without an animation, and a resource instance without a resource
#define FIELD_PROP_DOOR_NONE 0xffff
#define FIELD_PROP_ANM_NONE 0xffffffff
#define FIELD_PROP_RES_INDEX_NONE 0xffffffff

// What each of a prop's animations is doing, its animationState
enum {
    FIELD_PROP_ANM_STATE_STOPPED,
    FIELD_PROP_ANM_STATE_LOOP,
    // Paused, or played to its end
    FIELD_PROP_ANM_STATE_PAUSED,
    FIELD_PROP_ANM_STATE_PLAY,
    FIELD_PROP_ANM_STATE_PLAY_INV,
};

// The commands of a dynamic prop's animations
enum {
    FIELD_PROP_ANM_CMD_PLAY,
    FIELD_PROP_ANM_CMD_PLAY_INV,
    FIELD_PROP_ANM_CMD_LOOP,
    FIELD_PROP_ANM_CMD_PAUSE,
    FIELD_PROP_ANM_CMD_STOP,
};

// A prop of a chunk's or a land data patch's file: where it is, and its models
struct FieldPropSourceInfo {
    s32 x;
    s32 y;
    s32 z;
    u16 rotationY;
    u8 highResId;
    u8 lowResId;
};

struct FieldPropAreaBounds {
    fx32 minZ;
    fx32 maxZ;
    fx32 minX;
    fx32 maxX;
};

// The ways a prop's animations play, the index of its controller in field_buildmodel.c's FIELD_PROP_ANMCNT_VTABLES
typedef enum {
    FIELD_PROP_ANMCNT_STATIC,
    FIELD_PROP_ANMCNT_AMBIENT,
    FIELD_PROP_ANMCNT_DYNAMIC,
    FIELD_PROP_ANMCNT_RTC, // by the time of day
    FIELD_PROP_ANMCNT_COUNT
} FieldPropAnmControllerType;

// Partial resource layouts inferred from the Swan-named overlay 36 helpers.
struct FieldPropResAnmHeader {
    u8 controllerType; // FieldPropAnmControllerType
    u8 unk1;
    u8 ambientAnimationCount;
    u8 unk3;
    u32 animationIds[4];
};

// How a prop's animations play, by its animation header's controllerType: static, ambient, dynamic or by the time of
// day
struct FieldPropAnmController {
    void (*init)(FieldPropSystem *system, FieldPropResInstance *instance);
    void (*update)(FieldPropSystem *system, FieldPropResInstance *instance);
    void (*command)(FieldPropResInstance *instance, u32 animation, u32 command);
};

struct FieldPropResBundle {
    u8 unk0[2];
    u8 countFlags;
    u8 unk3;
    u32 offsets[1];
};

// Field names from swan's field_static_prop.h.
struct FieldPropResInfo {
    u16 resId;
    u16 type;
    u16 doorResId;
    s16 doorX;
    s16 doorY;
    s16 doorZ;
    u16 unk0c;
    u16 unk0e;
    FieldPropResAnmHeader animationHeader;
};

// A prop's loaded resources, 0x18 bytes. Layout from swan.
struct FieldPropResource {
    FieldPropResInfo *info;
    G3DResource *model;
    G3DResource *animations[4];
};

// A prop placed in a chunk's data. Layout from swan's field_static_prop.h.
struct FieldPropPosition {
    VecFx32 position;
    u16 rotationY;
    u16 pad0e;
};

struct FieldPropInstance {
    u32 resIndex;
    FieldPropPosition pos;
};

struct FieldChunkPropHolder {
    FieldChunk *chunk;
    u32 savedResIndex;
    u16 propIndex;
    u16 visible;
    FieldPropInstance *instance;
};

// Layout inferred from the Swan-named FieldPropRTCState helpers in overlay 36.
struct FieldPropRTCState {
    u32 dayPeriod;
    u32 previousDayPeriod;
    BOOL dayPartChanged;
    u8 playAnmIndex;
    u8 season;
    u8 padding[2];
};

struct FieldPropResInstanceHandle {
    G3DActor *actor;
    u32 unk4;
};

// The props' actors, which the mapper reads. Swan has the first two fields; the rest is cleared and freed with them
struct FieldPropResBank {
    FieldPropResInstanceHandle *handles;
    u32 count;
    void *unk08;
    u32 unk0C;
    u32 unk10;
    u32 unk14;
};

// Layout from swan
struct FieldPropSystem {
    u16 heapId;
    FieldG3DMapper *mapper;
    FieldPropRTCState rtcState;
    FieldPropResBundle *resBundle;
    u8 resIdToIndex[0x200];
    u32 resInfoCount;
    FieldPropResBank resBank;
    u32 resourceCount;
    FieldPropResource *resources;
    G3DResource *textureResource;
    u32 resInstanceCount;
    FieldPropResInstance *resInstances;
    FieldPropHandle *handles[7];
    FieldChunkPropHolder chunkPropHolders[0x120];
};

struct FieldPropResInstance {
    G3DActor *actor;
    FieldPropResource *resource;
    u32 animationState[4];
};

struct FieldPropHandle {
    FieldPropSystem *system;
    u32 animation;
    FieldChunkPropHolder *holder;
    FieldPropResInstance instance;
    SRTMatrix transform;
};

u32 FieldPropResAnmHeader_GetAnmCount(const FieldPropResAnmHeader *header);
FieldPropResAnmHeader *FieldPropResInfo_GetAnmHeader(FieldPropResInfo *resInfo);
u8 FieldPropResInfo_GetTypeConv(const FieldPropResInfo *resInfo);
u32 FieldPropSystem_ConvResIDToIndex(const FieldPropSystem *system, u32 resId);
void *FieldPropResBundle_GetResInfo(FieldPropResBundle *bundle, u32 index);
void *FieldPropResBundle_GetModelData(FieldPropResBundle *bundle, u32 index);
void *FieldPropResAnmHeader_GetAnmData(FieldPropResAnmHeader *header, u32 index);
FieldPropResInfo *FieldPropSystem_FindResInfo(FieldPropSystem *system, u32 resId);
FieldPropResInfo *FieldPropSystem_GetResInfo(FieldPropSystem *system, u32 index);
FieldPropResBank *FieldPropSystem_GetResBank(FieldPropSystem *system);
FieldPropSystem *FieldPropSystem_Create(HeapID heapId, FieldG3DMapper *mapper, u16 season);
// Loads the props of the area, and recolors their textures with the field's color post-FX
void FieldPropSystem_LoadArea(FieldPropSystem *system, u16 zoneId, AreaData *area, FieldColorPostFX *postFx);
void FieldPropSystem_LoadTextures(FieldPropSystem *system, u16 arcId, u32 fileId, FieldColorPostFX *postFx);
void FieldPropSystem_InitResources(FieldPropSystem *system, FieldColorPostFX *postFx);
void FieldPropSystem_InstantiateResources(FieldPropSystem *system, FieldPropResBank *bank);
void FieldPropSystem_InitResource(FieldPropSystem *system, FieldPropResource *resource, u32 index,
                                  FieldColorPostFX *postFx);
void FieldPropResource_Free(FieldPropResource *resource);
void FieldPropSystem_LoadResBundle(FieldPropSystem *system, u16 arcId, u32 fileId);
void FieldPropSystem_FreeResBundle(FieldPropSystem *system);
void FieldPropSystem_BuildResIDLUT(FieldPropSystem *system, u32 defaultResId);
void FieldPropSystem_FreeTextures(FieldPropSystem *system);
void FieldPropResInstance_CallAnmCmd(FieldPropResInstance *instance, u32 animation, u32 command);
BOOL FieldPropResInstance_IsAnmIdle(FieldPropResInstance *instance, u32 animation);
void FieldPropResInstance_Free(FieldPropResInstance *instance);
void FieldPropResInstance_Init(FieldPropSystem *system, FieldPropResInstance *instance, FieldPropResource *resource);
void FieldPropSystem_DeleteHandle(FieldPropSystem *system, FieldPropHandle *handle);
void FieldPropSystem_RegistHandle(FieldPropSystem *system, FieldPropHandle *handle);
u16 FieldPropSystem_GetHandleID(FieldPropSystem *system, FieldPropHandle *handle);
FieldPropHandle *FieldPropSystem_FindHandleByID(FieldPropSystem *system, u32 id);
void FieldPropSystem_UpdateResInstance(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropSystem_Update(FieldPropSystem *system);
void FieldPropSystem_DrawAllHandles(FieldPropSystem *system);
void FieldPropSystem_UnlinkChunk(FieldPropSystem *system, FieldChunk *chunk);
s32 FieldPropSystem_InstantiateProps(FieldPropSystem *system, FieldChunk *chunk, const FieldPropSourceInfo *infos,
                                     s32 count);
BOOL FieldPropSystem_CheckCreateDoorReq(FieldPropSystem *system, u32 resId, VecFx32 *position, u32 *resIndex);
void FieldPropSystem_InstantiateFromInfo(FieldPropSystem *system, FieldChunk *chunk, const FieldPropSourceInfo *info,
                                         u32 propIndex);
void FieldPropSystem_FreeResInstances(FieldPropSystem *system, FieldPropResBank *bank);
void FieldPropSystem_FreeResources(FieldPropSystem *system);
void FieldPropSystem_Free(FieldPropSystem *system);
void FieldPropSystem_InstantiateProp(FieldChunk *chunk, FieldPropInstance *instance, u32 propIndex);
FieldPropInstance *FieldChunk_GetPropInstance(FieldChunk *chunk, u32 propIndex);
FieldChunkPropHolder *FieldPropSystem_LinkPropToChunk(FieldPropSystem *system, FieldChunk *chunk,
                                                      FieldPropInstance *instance, u32 propIndex);
void FieldChunkPropHolder_Release(FieldPropSystem *system, FieldChunkPropHolder *holder);
void FieldPropSystem_ReleaseChunkPropHolders(FieldPropSystem *system, FieldChunk *chunk);
void FieldPropSystem_ReleaseChunkPropHolder(FieldPropSystem *system, FieldChunkPropHolder *holder);
u8 FieldChunkPropHolder_GetResIndex(FieldChunkPropHolder *holder);
u8 FieldChunkPropHolder_GetPropType(FieldPropSystem *system, FieldChunkPropHolder *holder);
void FieldChunkPropHolder_SetVisible(FieldChunkPropHolder *holder, BOOL visible);
void FieldChunkPropHolder_ChangeResID(FieldPropSystem *system, FieldChunkPropHolder *holder, u32 resId);
void FieldPropRTCState_Init(FieldPropRTCState *state, u8 season);
void FieldPropAnmController_Static_Update(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_Static_Init(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_Ambient_Init(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_RTC_Init(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_Static_ExecCommand(FieldPropResInstance *instance, u32 animation, u32 command);
void FieldPropAnmController_Ambient_Update(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_RTC_Update(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_Dynamic_Update(FieldPropSystem *system, FieldPropResInstance *instance);
void FieldPropAnmController_Dynamic_ExecCommand(FieldPropResInstance *instance, u32 animation, u32 command);
void FieldPropResInstance_AnmStopAll(FieldPropResInstance *instance);
void FieldPropResInstance_AnmSetPlay(FieldPropResInstance *instance, u32 animation);
void FieldPropResInstance_AnmSetPlayLoop(FieldPropResInstance *instance, u32 animation);
void FieldPropResInstance_AnmSetPlayInv(FieldPropResInstance *instance, u32 animation);
void FieldPropResInstance_AnmSetPause(FieldPropResInstance *instance, u32 animation);
void FieldPropRTCState_Update(FieldPropRTCState *state);
BOOL FieldPropRTCState_HasDayPartChanged(FieldPropRTCState *state);
u8 FieldPropRTCState_GetPlayAnmIndex(FieldPropRTCState *state);

void FieldPropHandle_CallAnmCmd(FieldPropHandle *handle, u32 animation, u32 command);
void FieldPropHandle_CallAnmCmdSilent(FieldPropHandle *handle, u32 command);
BOOL FieldPropHandle_IsAnmIdle(FieldPropHandle *handle, u32 animation);
BOOL FieldPropHandle_IsCurrentAnmIdle(FieldPropHandle *handle);
BOOL FieldPropHandle_IsAnmFinished(FieldPropHandle *handle);
u16 FieldPropHandle_GetPropType(FieldPropHandle *handle);
void FieldPropHandle_Draw(FieldPropHandle *handle);
BOOL FieldPropHandle_GetAnimSoundIDCore(FieldPropHandle *handle, u32 animation, u16 *soundId);
BOOL FieldPropHandle_GetAnimSoundID(FieldPropHandle *handle, u16 *soundId);
BOOL FieldPropHandle_IsAnimSoundFinished(FieldPropHandle *handle);
void FieldPropHandle_Free(FieldPropHandle *handle);
void FieldChunkPropHolder_CallAnmCmd(FieldPropSystem *system, FieldChunkPropHolder *prop, u32 animation, u32 command);
FieldPropHandle *FieldPropSystem_CreateHandleNew(FieldPropSystem *system, u32 propId, SRTMatrix *transform);
FieldPropHandle *FieldPropSystem_CreateHandleFromExisting(FieldPropSystem *system, FieldChunkPropHolder *prop);
FieldPropHandle *FieldPropSystem_CreateHandleAtPos(FieldPropSystem *system, u32 propId, const VecFx32 *position);
FieldChunkPropHolder *FieldPropSystem_FindProp(FieldPropSystem *system, u32 propId, const FieldPropAreaBounds *bounds);
FieldChunkPropHolder *FieldPropSystem_FindPropAtPos(FieldPropSystem *system, u32 propId, const VecFx32 *position);
FieldChunkPropHolder **FieldPropSystem_FindPropsInArea(FieldPropSystem *system, const FieldPropAreaBounds *bounds,
                                                       u32 filter, u32 *count);
void FieldPropSearchArea_Set(FieldPropAreaBounds *bounds, const VecFx32 *position);
void FieldChunkPropHolder_GetPosAbs(FieldChunkPropHolder *prop, VecFx32 *position);

#endif // POKEBW2_FIELD_FIELD_PROP_H
