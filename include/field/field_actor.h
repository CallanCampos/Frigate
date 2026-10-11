#ifndef POKEBW2_FIELD_FIELD_ACTOR_H
#define POKEBW2_FIELD_FIELD_ACTOR_H

#include "types.h"
#include "constants/directions.h"
#include "gfl/heap.h"
#include "gfl/blact.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "field/zone.h"

// A position on the grid of 16-unit tiles. The game copies it as 8 bytes, with a field after z
typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 unk6;
} GridPos;

// The resources of an object code's model
typedef struct {
    u16 res1;
    u16 res2;
    u16 animations[3];
} FieldActorResGroup;

// An object code's record in ARCID_MMODEL_TBL, from 4 bytes into the file, at the index GetIndexOfObjID returns. Names
// from swan's FieldActorConfig
typedef struct {
    u16 uid;
    u8 entityType;
    u8 sceneNodeType;
    u8 enableShadow;
    u8 footprintType;
    u8 enableReflections;
    u8 billboardSize;
    u8 spriteAtlasSize;
    u8 spriteControllerType;
    u8 gender;
    u8 collWidth;
    u8 collHeight;
    s8 wPosOffsetX;
    s8 wPosOffsetY;
    s8 wPosOffsetZ;
    FieldActorResGroup rscIndices;
    u16 padding;
} FieldActorConfig;

// An actor's place on a rail, from swan's ActorPositionRail. unk0 is ACTOR_RAIL_POS_SET once the place is set
typedef struct {
    u32 unk0;
    RailPosition position;
} ActorPositionRail;

#define ACTOR_RAIL_POS_SET 0xefefefef

// An actor of a zone's entities, from which actors are created. Names and layout from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Field names from swan's ZoneNPCPositionGrid and ZoneNPCPositionRail.
struct ZoneNPCGridPosition {
    u16 x;
    u16 z;
    s32 y;
};

struct ZoneNPCRailPosition {
    u16 railIndex;
    u16 frontPos;
    s16 sidePos;
};

struct ZoneNPC {
    u16 uid;
    u16 modelId;
    u16 moveCode;
    u16 evType;
    u16 spawnFlag;
    u16 scrId;
    u16 direction;
    u16 param0;
    u16 param1;
    u16 param2;
    u16 areaW;
    u16 areaH;
    BOOL isRail;
    union {
        ZoneNPCGridPosition grid;
        ZoneNPCRailPosition rail;
    } pos;
};

// An actor's move code and its functions, called with the actor: unkC before the code is changed
typedef struct {
    u32 code;
    void (*unk4)(FieldActor *actor);
    void (*unk8)(FieldActor *actor);
    void (*unkC)(FieldActor *actor);
    void (*unk10)(FieldActor *actor);
} FieldActorMoveCode;

// The saved positions of the strength boulders that were moved, in grid units, by the slots of fldmmdl.c's table of
// boulders. A slot without a saved position holds STRENGTH_ROCK_POS_NONE
#define STRENGTH_ROCK_SLOT_COUNT 80
#define STRENGTH_ROCK_POS_NONE 0x7fff

typedef struct {
    s16 x[STRENGTH_ROCK_SLOT_COUNT];
    s16 y[STRENGTH_ROCK_SLOT_COUNT];
    s16 z[STRENGTH_ROCK_SLOT_COUNT];
} StrengthRockSave;

// What tells one actor from another, for finding it again
typedef struct {
    u16 uid;
    u16 objCode;
    u16 zoneId;
} FieldActorIdentity;

GameEvent *CallEventPrepareResidentActorsForZoneChange(GameSystem *gsys, Field *field);
GameEventReturnCode func_ov012_0215c59c(GameEvent *event, u32 *state, void *data);
void DisableAllActorsMovement(MMSys *mmSys);
FieldActor *FindPlayerFieldActor(MMSys *mmSys);
void EnableAllActorsMovement(MMSys *mmSys);
// Whether the actor has finished its movement commands
BOOL IsAllActorAcmdFinished(FieldActor *actor);
void FldActSys_ClearCache(MMSys *mmSys);
void FldActSys_DeleteAllActors(MMSys *mmSys);
VecFx32 *GetMModelWPosPtr(FieldActor *actor);
s32 GetZoneNPCInfoCacheIdx(u16 zoneId);
ZoneNPC *GetZoneNPCs(EventData *eventData);
u32 GetZoneNPCsCount(EventData *eventData);
void SetZoneNPCLocation(EventData *eventData, u32 npcId, u16 direction, u16 x, s32 y, u16 z);
void SetZoneNPCMdlID(EventData *eventData, u16 npcId, u16 objCode);
void SetZoneNPCSCRID(EventData *eventData, u16 npcId, u16 scriptId);
void GetNPCMdlInfoForOBJCODE(const MMSys *actorSystem, u16 objCode, FieldActorConfig *config);
void LoadMModelSystemInfoCache(MMSys *mmSys, s32 index);
void SetActorFlag(FieldActor *actor, u32 flag);
void ClearActorFlag(FieldActor *actor, u32 flag);
void SetActorMovementFlag(FieldActor *actor, u32 flag);
void ClearActorMovementFlag(FieldActor *actor, u32 flag);
void FldAct_InvokeUpdateCallback(FieldActor *actor);
void ChangeActorDirection(FieldActor *actor, u16 dir);
void SetActorHidden(FieldActor *actor, BOOL hidden);
void FldAct_GetGPos(FieldActor *actor, GridPos *pos);
u16 GetActorUID(FieldActor *actor);
u16 FldAct_GetSCRID(FieldActor *actor);
u16 FldAct_GetObjCode(FieldActor *actor);
// Makes the actor the one an entry describes
void FldAct_Transplant(FieldActor *actor, const ZoneNPC *npc);
u16 GetActorMotionDir(FieldActor *actor);
BOOL func_ov012_0216773c(FieldActor *actor);
void func_ov036_0219634c(FieldActor *actor, u16 *a1, u16 *a2);
void func_ov036_021963a4(FieldActor *actor, u16 a1, u16 a2);
// Sets the frame of the model's animation for a time. FALSE if the actor has no model
BOOL func_ov036_02196b10(FieldActor *actor, u16 time);
// The unit vector of a direction along the actor's rail
void func_ov036_02195a78(FieldActor *actor, u32 dir, VecFx16 *dest);
void func_ov036_02195a58(FieldActor *actor, VecFx16 *dest);
// The actor's rail position, the one a step ahead of it, and the one a step in a direction, FALSE if there is none
void GetActorRailPos(FieldActor *actor, RailPosition *position);
void func_ov036_02195928(FieldActor *actor, RailPosition *position);
BOOL GetActorRailPosForDir(FieldActor *actor, u16 dir, RailPosition *position);
u32 GetIndexOfObjID(u16 objCode);

// Overlay 36's table that func_ov036_02194650 indexes, by a record's unk9
typedef struct {
    u16 unk0_0 : 14;
    u16 unk0_14 : 2;
} Ov036Unk021cf1c8Entry;

typedef struct {
    const Ov036Unk021cf1c8Entry *const *unk0;
    u32 unk4;
} Ov036Unk021cf1c8;

extern const Ov036Unk021cf1c8 data_ov036_021cf1c8[];
void FldAct_SetShadowGroup(FieldActor *actor, u32 group);
u16 GetActorFaceDir(FieldActor *actor);
void CheckSetActorFaceDir(FieldActor *actor, u16 dir);
void DisableActorMovement(FieldActor *actor);
void EnableActorMovement(FieldActor *actor);
void DeleteActor(FieldActor *actor);
// The actor's current movement command
u16 FldAct_GetAcmd(FieldActor *actor);
void FldAct_UpdateBlInfoForNewObjCode(FieldActor *actor, u16 objCode);
void SetActorGPosX(FieldActor *actor, s16 x);
void SetActorGPosZ(FieldActor *actor, s16 z);
// Whether the actor has flag 4, and setting or clearing flag 0x80 (set when the value is not TRUE)
BOOL func_ov012_02167520(FieldActor *actor);
void func_ov012_02167580(FieldActor *actor, BOOL value);
// Ends the actor's movement command
void func_ov012_02166f2c(FieldActor *actor);
s16 GetGPosX(FieldActor *actor);
s16 GetGPosZ(FieldActor *actor);
void SetActorGPos(FieldActor *actor, s16 x, s16 y, s16 z, u16 dir);
void SetActorMotionDir(FieldActor *actor, u16 dir);
void ChangeActorMoveCodeSeq(FieldActor *actor, u16 moveCode);
MMSys *GetActorMModelSystem(FieldActor *actor);
FieldG3DMapper *GetMMSysG3DMapper(MMSys *system);
BOOL GetTerrainAtPosByActor(FieldActor *actor, const VecFx32 *position, MapTerrainBuf *terrain);
Field *GetMMSysField(MMSys *mmSys);
// The movement command of a direction in the row of a table that has the command
u16 GetAcmdForDir(u16 dir, u32 acmd);
// The collision flags of the tile next to the actor in a direction
u32 ActorRouteCollCheckOneTileInDir(FieldActor *actor, u16 dir);
// Starts a movement command
void func_ov012_02166eb0(FieldActor *actor, u16 acmd);
void FldAct_SetAcmd(FieldActor *actor, u16 acmd);
BOOL func_ov012_02166ef8(FieldActor *actor);
// Whether the actor's movement command has finished
BOOL func_ov036_0218f01c(FieldActor *actor);
// Set movement flag 0x10, clear it, and clear flag 0x40
void func_ov012_021674b0(FieldActor *actor);
void func_ov012_021674bc(FieldActor *actor);
void func_ov012_021674e8(FieldActor *actor);
// Gives the actor a move code of its own functions
void func_ov012_021682e8(FieldActor *actor, const FieldActorMoveCode *moveCode);
u16 GetActorZoneID(FieldActor *actor);
BOOL IsActorFlag16(FieldActor *actor);
// Steps through the system's actors from *index, returning TRUE with the next one in *actor
BOOL NextActor(const MMSys *mmSys, FieldActor **actor, u32 *index);
FieldActor *CreateNewActorByEntityNoWKOBJCODE(MMSys *mmSys, const ZoneNPC *npc, u32 zoneId);
// Sets a ZoneNPC's position on the grid
void func_ov012_021682c0(ZoneNPC *npc, u16 x, u16 z, s32 y);
// The actor's user parameters 0 to 2
u16 GetActorUserParam(FieldActor *actor, u32 index);
void SetActorUserParam(FieldActor *actor, u16 value, u32 index);
void SetActorSCRID(FieldActor *actor, u16 scriptId);
void SetActorWPosAll(FieldActor *actor, const VecFx32 *pos, u16 dir);
FieldActor *CreateNewActorByParam(MMSys *mmSys, s16 x, s16 z, u16 dir, u16 id, u16 objCode, u16 moveCode, u32 zoneId);
void CopyActorWPos(FieldActor *actor, VecFx32 *dest);
// The position with every offset added
void CopyActorPosAllAdd(FieldActor *actor, VecFx32 *dest);
void SetActorWPosValue(FieldActor *actor, const VecFx32 *pos);
// The actor with an ID, or NULL
FieldActor *FindFieldActor(MMSys *mmSys, u16 id);
FieldActor *FindActorByMoveCode(MMSys *mmSys, u16 code);
// The other Trainer of a double battle pair
FieldActor *FindPairedTrainerActor(FieldActor *actor);
// Moves grid coordinates or a position by a distance in a direction
void AdjusGridXZByDir(u32 dir, s16 *x, s16 *z, s16 distance);
void ExpandVecInGridDir(u16 dir, VecFx32 *pos, fx32 distance);
void func_ov012_021670f4(FieldActor *actor, u32 value);
void func_ov012_02167564(FieldActor *actor, u32 value);
// Sets x and z to the center of a tile, leaving y
void ConvGXZToVector(s16 x, s16 z, VecFx32 *pos);
void SpawnAllZoneNPCs(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork);

BOOL func_ov012_02166ecc(FieldActor *actor);
void *FldActMatColorPreset_Create(HeapID heapId);
void FldActMatColorPreset_Free(void *preset);
void FldActMatColorPreset_Load(void *preset, u32 presetId);
void FldActMatColorPreset_Apply(void *preset, BlActScene *scene);
void FldActSys_AttachField(MMSys *system, HeapID heapId, GameData *gameData, Field *field, FieldG3DMapper *mapper,
                           NoGridMapper *noGridMapper, void *colorPostFx);
void FldActSys_InitBlAct(MMSys *system, BlActSys *blAct, u32 count);
void FldActSys_LoadCachedBlact(MMSys *system);
void FldActSys_LoadStaticBlact(MMSys *system, u16 objCode);
void FldActSys_Update(MMSys *system);
void FldActSys_FinishAsyncMatLoad(MMSys *system);
void FldActSys_FinishAsyncMatLoadSafe(MMSys *system);
BOOL FldActSys_IsAsyncLoadPending(MMSys *system);
void FldActSys_SuspendAllActors(MMSys *system);
void DeleteAllActors(MMSys *system);
void FieldActorG3DSystem_Create(MMSys *system, void *g3dObjSystem);
void func_ov012_02166764(MMSys *system);
void func_ov012_021667cc(MMSys *system, u16 *cameraAngle);
void func_ov012_02166d48(MMSys *system);
void func_ov012_021673e0(MMSys *system, BOOL flag);

BOOL CheckActorFlag(FieldActor *actor, u32 flag);
u32 func_ov012_02166fe4(FieldActor *actor);
BOOL CheckActorMovementFlag(FieldActor *actor, u32 flag);
u16 GetActorMoveCode(FieldActor *actor);
s16 GetActorWalkAreaW(FieldActor *actor);
s16 GetActorWalkAreaH(FieldActor *actor);
ActorPositionRail *GetNPCRailPosPtrAddr(FieldActor *actor);
// The rail unit that moves the actor along the rails
RailUnit *FldAct_GetRailUnit(FieldActor *actor);
ActorPositionRail *ClearActorPositionBlock(FieldActor *actor, u32 size);
// Call the move code's unk4 and unk8 functions
void func_ov012_02167174(FieldActor *actor);
void func_ov012_02167188(FieldActor *actor);
void SetCachedTileUnderActor(FieldActor *actor, u32 tileType);
void SetCachedOrigYTileUnderActor(FieldActor *actor, u32 tileType);
u32 GetCachedTileUnderActor(FieldActor *actor);
u32 GetCachedOrigYTileUnderActor(FieldActor *actor);
s16 GetActorDefaultGPosX(FieldActor *actor);
s16 GetActorDefaultGPosZ(FieldActor *actor);
s16 FldAct_GetInitGPosX(FieldActor *actor);
void SetActorInitialGPosX(FieldActor *actor, s16 x);
s16 FldAct_GetInitGPosY(FieldActor *actor);
void SetActorInitialGPosY(FieldActor *actor, s16 y);
s16 FldAct_GetInitGPosZ(FieldActor *actor);
void SetActorInitialGPosZ(FieldActor *actor, s16 z);
void adjustXPos(FieldActor *actor, s16 dx);
s16 FldAct_GetGPosY(FieldActor *actor);
void SetActorGPosY(FieldActor *actor, s16 y);
void adjustYPos(FieldActor *actor, s16 dz);
fx32 GetActorPosY(FieldActor *actor);
void func_ov012_0216736c(FieldActor *actor, const VecFx32 *offset);
u8 GetActorCollWidth(FieldActor *actor);
u8 GetActorCollHeight(FieldActor *actor);
BOOL func_ov012_021673f8(MMSys *system);
u32 func_ov012_0216748c(FieldActor *actor, u32 flag);
// Sets movement flag 0x20
void func_ov012_021674dc(FieldActor *actor);
BOOL func_ov012_02167600(FieldActor *actor);
BOOL func_ov012_02167614(FieldActor *actor);
BOOL func_ov012_02167658(FieldActor *actor);
// Sets or clears movement flag 0x40000, and whether it is set
void func_ov012_021676bc(FieldActor *actor, BOOL value);
BOOL func_ov012_021676d8(FieldActor *actor);
BOOL func_ov012_021676f0(FieldActor *actor);
void FldAct_SetReflectingFlag(FieldActor *actor, BOOL value);
BOOL FldAct_CheckReflectingFlag(FieldActor *actor);
BOOL func_ov012_021677a4(FieldActor *actor);
BOOL func_ov012_021677f4(FieldActor *actor);
const FieldActorConfig *GetActorMdlInfo(FieldActor *actor);
u32 CheckMMSysFlag(MMSys *system, u32 flag);
// Resets a strength boulder's saved position in a zone
void func_ov012_02168258(MMSys *system, u16 zoneId, u16 uid);
void func_ov036_0218eff4(FieldActor *actor);
void func_ov036_021925a4(FieldActor *actor);
BOOL func_ov036_021925ac(FieldActor *actor);
void SetActorInitedPositionRail(FieldActor *actor, const RailPosition *position);

// What the terrain effects of an actor look at
typedef struct {
    u32 tileTypeOrigY;
    u32 tileType;
    u16 tileClassOrigY;
    u16 tileClass;
    u16 tileFlagsOrigY;
    u16 tileFlags;
    const FieldActorConfig *config;
    void *effects;
    u8 season;
} ActorTerrainEffectParam;

// Overlay 12's field_actor_tool.c
void func_ov012_0215dabc(FieldActor *actor);
void func_ov012_0215dad4(FieldActor *actor);
void ActorTerrainEffect_ApplyAll(FieldActor *actor);
// The collision flags at a grid position, with the world position's y
u32 ActorRouteCollCheckCore(FieldActor *actor, const VecFx32 *position, s16 x, s16 y, s16 z, u16 dir);
u32 ActorRouteCollCheck(FieldActor *actor, s16 x, s16 y, s16 z, u16 dir);
BOOL CheckActorNewPosOtherActorCollision(FieldActor *actor, s16 x, s16 y, s16 z);
BOOL CheckActorVolumeOverGPos(FieldActor *actor, s16 x, s16 z, BOOL checkInit);
BOOL IsGPosOutsideActorWalkArea(FieldActor *actor, s16 x, s16 z);
BOOL CheckBlockedCollPathToPosition(FieldActor *actor, u16 dir, VecFx32 position);
BOOL GetTileTypeAtPosByActor(FieldActor *actor, const VecFx32 *position, u32 *tileType);
BOOL GetHeightFromMap(FieldActor *actor, const VecFx32 *position, fx32 *height);
void func_ov012_0215e8ec(FieldActor *actor, u16 dir);
void SetActorInitialGPosToNowGPos(FieldActor *actor);
u32 func_ov012_0215e9b0(FieldActor *actor, u16 dir);
void func_ov012_0215e9fc(FieldActor *actor, const VecFx32 *offset);
void func_ov012_0215ea30(FieldActor *actor, u16 dir, fx32 distance);
BOOL CheckRecalcActorY(FieldActor *actor);
BOOL FldAct_CacheTerrainInfo(FieldActor *actor);
// Whether the player is within 17 tiles of the actor on both axes
BOOL func_ov012_0215ebd8(FieldActor *actor);
void SetRailActorFlag(FieldActor *actor);
void InitRailActor(FieldActor *actor, const ZoneNPC *npc);
void SetActorInitPositionRail(FieldActor *actor, const RailPosition *position);
BOOL func_ov012_0215db3c(FieldActor *actor);
void func_ov012_0215dba0(FieldActor *actor);
void func_ov012_0215dbb8(FieldActor *actor);
void func_ov012_0215dbdc(FieldActor *actor);
void func_ov012_0215dc00(FieldActor *actor);
void func_ov012_0215dc34(FieldActor *actor);
void ActorTerrainEffect_InitParam(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215dcdc(FieldActor *actor);
void func_ov012_0215ddb4(FieldActor *actor);
void func_ov012_0215de0c(FieldActor *actor);
void func_ov012_0215de7c(FieldActor *actor);
void func_ov012_0215def4(FieldActor *actor, ActorTerrainEffectParam *param);
u32 func_ov012_0215df40(u32 tileClass, u8 season);
void func_ov012_0215dfb4(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_TallGrass(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e008(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e038(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e05c(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e0f4(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e110(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e148(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e198(FieldActor *actor, ActorTerrainEffectParam *param);
BOOL func_ov012_0215e1a4(FieldActor *actor, ActorTerrainEffectParam *param);
BOOL func_ov012_0215e1c4(ActorTerrainEffectParam *param);
void func_ov012_0215e1ec(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e228(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e258(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e278(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e298(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e2b8(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e33c(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e380(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215e39c(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_Shadow(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215ec28(FieldActor *actor);
BOOL func_ov012_0215ede4(FieldActor *actor, RailPosition *position);

// Overlay 12's fldmmdl.c
MMSys *FldActSys_Create(HeapID heapId, u32 capacity, StrengthRockSave *save);
void FldActSys_Free(MMSys *system);
// Updates until the materials have loaded
void FldActSys_ForceFullSync(MMSys *system);
// Creates the actor of the entity with an ID, if it is spawned
FieldActor *func_ov012_021668f8(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork, u16 uid);
u16 GetActorLimit(const MMSys *system);
// The base priority of the actors' tasks
u16 func_ov012_02166f6c(MMSys *system);
HeapID FldActSys_GetFieldHeapID(MMSys *system);
TCBManager *GetMMSysTCBMgr(MMSys *system);
ArcTool *GetMMSysDataArcHandle(MMSys *system);
void FldActSys_BindFieldBlAct(MMSys *system, void *fieldBlAct);
void *FldActSys_GetFieldBlAct(MMSys *system);
void FldActSys_BindActorG3DSystem(MMSys *system, void *actorG3DSystem);
void *FldActSys_GetActorG3DSystem(MMSys *system);
NoGridMapper *FldActSys_GetNoGridMapper(MMSys *system);
// The camera angle that func_ov012_021667cc set, or 0
u16 func_ov012_02166fb0(MMSys *system);
u32 func_ov012_02166fc0(MMSys *system);
void func_ov012_02166fc4(MMSys *system, u32 value);
void SetActorEvType(FieldActor *actor, u16 evType);
u16 GetActorEvType(FieldActor *actor);
u16 GetActorSpawnFlag(FieldActor *actor);
u32 GetDefaultActorDir(FieldActor *actor);
void SetActorFaceDir(FieldActor *actor, u16 dir);
// The direction the actor faced before
u16 func_ov012_0216707c(FieldActor *actor);
void SetActorAreaW(FieldActor *actor, s16 w);
void SetActorAreaH(FieldActor *actor, s16 h);
u16 func_ov012_021670f8(FieldActor *actor);
// Work areas of the actor, cleared to a size and returned
void *func_ov012_02167120(FieldActor *actor, u32 size);
void *func_ov012_02167138(FieldActor *actor);
void *func_ov012_0216713c(FieldActor *actor, u32 size);
void *func_ov012_02167154(FieldActor *actor);
void *FldAct_ResetBlActWork(FieldActor *actor, u32 size);
void *FldAct_GetBlActWorkPtr(FieldActor *actor);
u32 FldAct_GetBlActIdx(FieldActor *actor, u32 param);
void SetNextActorAcmd(FieldActor *actor, u16 acmd);
// The state of the actor's movement command
void func_ov012_02167220(FieldActor *actor, u16 state);
void FieldActor_NextAcmdState(FieldActor *actor);
u16 FldAct_GetAcmdState(FieldActor *actor);
void GetActorInitGPos(FieldActor *actor, GridPos *pos);
void adjustZPos(FieldActor *actor, s16 dy);
void func_ov012_0216731c(FieldActor *actor, VecFx32 *dest);
void SetActorWPosOffset(FieldActor *actor, const VecFx32 *offset);
void func_ov012_0216733c(FieldActor *actor, VecFx32 *dest);
void func_ov012_0216734c(FieldActor *actor, const VecFx32 *value);
void func_ov012_0216735c(FieldActor *actor, VecFx32 *dest);
void *FldAct_GetFieldBlAct(FieldActor *actor);
// The model's position offset of the actor's config
void CopyActorPosOffset(FieldActor *actor, VecFx32 *offset);
void func_ov012_021673c8(MMSys *system, BOOL flag);
// Whether the actor exists
BOOL func_ov012_0216749c(FieldActor *actor);
BOOL func_ov012_0216750c(FieldActor *actor);
BOOL func_ov012_0216754c(FieldActor *actor);
void SetActorFlag8000(FieldActor *actor, BOOL value);
BOOL func_ov012_021675b4(FieldActor *actor);
void SetActorFlag256(FieldActor *actor, BOOL value);
void SetActorFlag32(FieldActor *actor, BOOL value);
void func_ov012_0216763c(FieldActor *actor, BOOL a1);
void func_ov012_02167754(FieldActor *actor, BOOL value);
void func_ov012_02167788(FieldActor *actor, BOOL value);
void func_ov012_021677bc(FieldActor *actor, BOOL value);
void func_ov012_021677d8(FieldActor *actor, BOOL value);
u8 FldAct_GetShadowGroup(FieldActor *actor);
void func_ov012_0216783c(FieldActor *actor, BOOL value);
FieldActor *GetFirstActorOnGPos(MMSys *system, s16 x, s16 z, BOOL checkInit);
// The first actor over a grid position whose height is within maxHeightDiff of y, other than exclude
FieldActor *FindActorByGPos(MMSys *system, s16 x, s16 z, fx32 y, fx32 maxHeightDiff, BOOL checkInit);
FieldActor *FindActorByGPos_(MMSys *system, s16 x, s16 z, fx32 y, fx32 maxHeightDiff, BOOL checkInit,
                             FieldActor *exclude);
// The first actor on a rail position
FieldActor *FindActorByRailPos(MMSys *system, const RailPosition *position, BOOL checkInit);
// Whether another actor has the object code
BOOL FldAct_CheckObjCodeShared(FieldActor *actor, u16 objCode);
void ChangeActorUID(FieldActor *actor, u16 uid);
void func_ov012_02167d88(FieldActor *actor, FieldActorIdentity *identity);
BOOL func_ov012_02167da8(FieldActor *actor, const FieldActorIdentity *identity);
void GetMMSysMdlInfoCacheEntry(MMSys *system, u32 index, FieldActorConfig *config);
u16 GetMMSysMdlInfoCacheEntryCount(MMSys *system);
const FieldActorResGroup *GetNPCMdlInfoG2DRscGroup(const FieldActorConfig *config);
const FieldActorResGroup *GetNPCMdlInfoG3DRscGroup(const FieldActorConfig *config);
// The object code that an object code stands for, through the work values of WKOBJCODE00 and on
u16 ResolvePossibleWKOBJCODE(EventWork *eventWork, u16 objCode);
// How an actor of the event type looks for the player: the types that see only the way they face give 1
u16 func_ov012_02168024(u16 type);
void func_ov012_02168054(FieldActor *actor);
void func_ov012_02168058(FieldActor *actor);
void func_ov012_0216805c(FieldActor *actor);
void func_ov012_02168060(FieldActor *actor);
// The size of the strength boulders' save, and its initialization
u32 func_ov012_02168064(void);
void func_ov012_0216806c(StrengthRockSave *save);
// Saves the position of the actor's boulder
void func_ov012_021681b0(FieldActor *actor);
// The count of boulders with a saved position
u32 func_ov012_021682a0(MMSys *system);
// Sets a ZoneNPC's position on a rail
void func_ov012_021682d4(ZoneNPC *npc, u16 railIndex, u16 frontPos, s16 sidePos);

// Overlay 36's functions that fldmmdl.c calls
void func_ov036_0218ed18(MMSys *system);
void func_ov036_0218ed44(FieldActor *actor);
void FldActSys_UpdateResourceLoader(MMSys *system);
void func_ov036_02194924(FieldActor *actor);
void func_ov036_02194938(FieldActor *actor);

#endif // POKEBW2_FIELD_FIELD_ACTOR_H
