#ifndef POKEBW2_FIELD_FIELD_ACTOR_H
#define POKEBW2_FIELD_FIELD_ACTOR_H

#include "types.h"
#include "constants/directions.h"
#include "field/zone.h"
#include "gfl/blact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

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
GameEventReturnCode EventPrepareResidentActors_Callback(GameEvent *event, u32 *state, void *data);
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
BOOL FldAct_CheckAcmdSet(FieldActor *actor);
void func_ov036_0219634c(FieldActor *actor, u16 *a1, u16 *a2);
void func_ov036_021963a4(FieldActor *actor, u16 a1, u16 a2);
// Sets the frame of the model's animation for a time. FALSE if the actor has no model
BOOL func_ov036_02196b10(FieldActor *actor, u16 time);
// Overlay 36's fldmmdl_acmd.c, not written yet: whether the actor is free of its action command, and the setting of
// the field effect it hides in
BOOL func_ov036_0218ed68(FieldActor *actor);
void func_ov036_02193110(FieldActor *actor, FieldEffectTask *task);
// Overlay 36's fldmmdl_blact.c: the freeing of the billboard actors' work, the flag that their load callback sets,
// their billboard system, and the change of an actor's model to another object code
void func_ov036_021937a4(MMSys *system);
u8 func_ov036_0219381c(FieldActor *actor);
BlActSys *FieldBlAct_GetBlActSys(FieldBlAct *blact);
void FldAct_SwapObjCode(FieldActor *actor, u16 newObjCode);
// The creation of an actor's billboard actor into *index, and its deletion
BOOL func_ov036_02193a00(FieldActor *actor, u16 *index);
void func_ov036_02193ac0(FieldActor *actor, u32 index);
// The creation of a billboard actor of an object code at a position, into out[1] with the material's flags in out[0],
// calling back once a material that has to be loaded first is there; its deletion; and the scale of an actor's
// billboard
BOOL func_ov036_02193b14(MMSys *system, const FieldActorConfig *config, u16 *out, VecFx32 *pos,
                         void (*callback)(u16 index, void *data), void *callbackData, u32 lights);
void func_ov036_02193b7c(MMSys *system, u16 *out);
void func_ov036_02193bd0(FieldActor *actor, u16 *scaleX, u16 *scaleY);
// Overlay 36's field_rail_loader.c: sets a flag of the actor's rail billboard work
void func_ov036_0219690c(FieldActor *actor, BOOL value);
// The script call of an actor's script; the animation and frame of the billboard actors of object codes 0xd3 and 0xd5,
// and of 0xdd and 0xde (FALSE without a billboard actor); and whether the billboard actor of object code 0x7b or 0x176
// has run its animation, with the reset of that animation
GameEvent *CreatePlayerXActorScriptCall(GameSystem *gsys, u32 unused1, u32 scriptId, u32 unused3, FieldActor *actor,
                                        HeapID heapId);
BOOL func_ov036_02197060(FieldActor *actor, u16 *anim, u16 *step);
BOOL func_ov036_021970b8(FieldActor *actor, u16 *anim, u16 *step);
BOOL func_ov036_02197238(FieldActor *actor);
void func_ov036_02197288(FieldActor *actor);
// The functions that run an action command: what to do on the first frame, each frame and at the end
typedef BOOL (*ActorAcmdFunc)(FieldActor *actor);

typedef struct {
    ActorAcmdFunc start;
    ActorAcmdFunc update;
    ActorAcmdFunc end;
} ActorAcmdFuncs;

// The work of a jump along the rail, which fldmmdl_g3dobj.c's func_ov036_02195354 starts and fld_act_rail.c updates
typedef struct {
    s32 speed;
    s32 progress;
    u16 step;
    u16 phase;
    s8 dir;
    s8 timer;
    s8 unkE;
    s8 height;
} ActorRailJumpWork;

// Overlay 36's fldmmdl_g3dobj.c: the start, update and end functions of the action commands that the rail actors'
// tables in fld_act_rail.c name, and the jump along the rail that their start functions begin
BOOL func_ov036_02194ee8(FieldActor *actor);
BOOL func_ov036_02194f94(FieldActor *actor);
BOOL func_ov036_02194fe8(FieldActor *actor);
BOOL func_ov036_02194ff4(FieldActor *actor);
BOOL func_ov036_02195004(FieldActor *actor);
BOOL func_ov036_02195014(FieldActor *actor);
BOOL func_ov036_02195024(FieldActor *actor);
BOOL func_ov036_02195034(FieldActor *actor);
BOOL func_ov036_02195044(FieldActor *actor);
BOOL func_ov036_02195054(FieldActor *actor);
BOOL func_ov036_02195064(FieldActor *actor);
BOOL func_ov036_02195074(FieldActor *actor);
BOOL func_ov036_02195084(FieldActor *actor);
BOOL func_ov036_02195094(FieldActor *actor);
BOOL func_ov036_021950a4(FieldActor *actor);
BOOL func_ov036_021950b4(FieldActor *actor);
BOOL func_ov036_021950c4(FieldActor *actor);
BOOL func_ov036_021950d4(FieldActor *actor);
BOOL func_ov036_021950e4(FieldActor *actor);
BOOL func_ov036_021950f4(FieldActor *actor);
BOOL func_ov036_02195104(FieldActor *actor);
BOOL func_ov036_02195114(FieldActor *actor);
BOOL func_ov036_02195124(FieldActor *actor);
BOOL func_ov036_02195168(FieldActor *actor);
BOOL func_ov036_0219519c(FieldActor *actor);
BOOL func_ov036_021951ac(FieldActor *actor);
BOOL func_ov036_021951bc(FieldActor *actor);
BOOL func_ov036_021951cc(FieldActor *actor);
BOOL func_ov036_021951dc(FieldActor *actor);
BOOL func_ov036_021951ec(FieldActor *actor);
BOOL func_ov036_021951fc(FieldActor *actor);
BOOL func_ov036_0219520c(FieldActor *actor);
BOOL func_ov036_0219521c(FieldActor *actor);
BOOL func_ov036_0219522c(FieldActor *actor);
BOOL func_ov036_0219523c(FieldActor *actor);
BOOL func_ov036_0219524c(FieldActor *actor);
BOOL func_ov036_0219525c(FieldActor *actor);
BOOL func_ov036_0219526c(FieldActor *actor);
BOOL func_ov036_0219527c(FieldActor *actor);
BOOL func_ov036_0219528c(FieldActor *actor);
BOOL func_ov036_0219529c(FieldActor *actor);
BOOL func_ov036_021952ac(FieldActor *actor);
BOOL func_ov036_021952cc(FieldActor *actor);
void func_ov036_02195354(FieldActor *actor, u16 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step);
// Also fldmmdl_g3dobj.c's: the freeing of the actors' 3D system, their 3D object system, the creation and deletion of
// an actor's 3D object, the tile under a rail position, and the direction an actor on the rails faces for a camera
// angle
void func_ov036_021946f4(MMSys *system);
FieldG3DObjSystem *FldAct_GetG3DObjSystem(FieldActor *actor);
u16 func_ov036_021948a0(FieldActor *actor);
void func_ov036_021948ec(FieldActor *actor, u16 objIdx);
u32 func_ov036_02194990(FieldActor *actor, const RailPosition *pos);
u8 func_ov036_021949a4(FieldActor *actor, int angle);
// The unit vector of a direction along the actor's rail
void func_ov036_02195a78(FieldActor *actor, u32 dir, VecFx16 *dest);
void func_ov036_02195a58(FieldActor *actor, VecFx16 *dest);
// The actor's rail position, the one a step ahead of it, and the one a step in a direction, FALSE if there is none
void GetActorRailPos(FieldActor *actor, RailPosition *position);
void func_ov036_02195928(FieldActor *actor, RailPosition *position);
BOOL GetActorRailPosForDir(FieldActor *actor, u16 dir, RailPosition *position);
// Overlay 36's fld_act_rail.c: the actor's position as its rail unit holds it, the unit's step, the collision flags of
// a step in a direction (1 off the rails, 2 a blocked tile, 4 another actor), the first actor on a rail position other
// than exclude, and the change of the actor's grid position to the one a step ahead
void func_ov036_02195604(FieldActor *actor, RailPosition *position);
void func_ov036_02195624(FieldActor *actor);
void func_ov036_02195640(FieldActor *actor, u16 dir, u32 speed);
u32 func_ov036_021956e8(FieldActor *actor, u16 dir);
u32 func_ov036_021956f4(FieldActor *actor, u16 dir, u32 *tile);
FieldActor *func_ov036_0219584c(MMSys *system, const RailPosition *position, BOOL checkInit, FieldActor *exclude);
void func_ov036_021959bc(FieldActor *actor, u16 dir);
// The move codes of the rail actors: their init, update and delete functions
void func_ov036_02195b1c(FieldActor *actor);
void func_ov036_02195b28(FieldActor *actor);
void func_ov036_02195b38(FieldActor *actor);
void func_ov036_02195b48(FieldActor *actor);
void func_ov036_02195b58(FieldActor *actor);
void func_ov036_02195b64(FieldActor *actor);
void func_ov036_02195b70(FieldActor *actor);
void func_ov036_02195b7c(FieldActor *actor);
void func_ov036_02195b88(FieldActor *actor);
void func_ov036_02195b94(FieldActor *actor);
void func_ov036_02195ba0(FieldActor *actor);
void func_ov036_02195bac(FieldActor *actor);
void func_ov036_02195bb8(FieldActor *actor);
void func_ov036_02195bc4(FieldActor *actor);
void func_ov036_02195bd0(FieldActor *actor);
void func_ov036_02195bdc(FieldActor *actor);
void func_ov036_02195be4(FieldActor *actor);
void func_ov036_02195bec(FieldActor *actor);
void func_ov036_02195bf8(FieldActor *actor);
void func_ov036_02195c04(FieldActor *actor);
void func_ov036_02195c0c(FieldActor *actor);
void func_ov036_02195c14(FieldActor *actor);
void func_ov036_02195c1c(FieldActor *actor);
void func_ov036_02195c24(FieldActor *actor);
// Clears the rail data to no points, with the curves' and cameras' function tables
void RailDataHandle_Reset(RailDataHandle *handle);
u32 GetIndexOfObjID(u16 objCode);

// Overlay 36's table that func_ov036_02194650 indexes, by a record's unk9: the animations of the billboard actors
typedef struct {
    const BlActAnimStep *const *unk0;
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
BOOL IsActorHidden(FieldActor *actor);
void SetActorCollisionEnabled(FieldActor *actor, BOOL value);
// Ends the actor's movement command
void FldAct_FreeAcmd(FieldActor *actor);
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
void FldAct_SetLocalAcmd(FieldActor *actor, u16 acmd);
void FldAct_SetAcmd(FieldActor *actor, u16 acmd);
BOOL FldAct_EndAcmd(FieldActor *actor);
// Whether the actor's movement command has finished
BOOL func_ov036_0218f01c(FieldActor *actor);
// Set movement flag 0x10, clear it, and clear flag 0x40
void FldAct_SetMoving(FieldActor *actor);
void FldAct_ClearMoving(FieldActor *actor);
void FldAct_ClearMoveEnd(FieldActor *actor);
// Gives the actor a move code of its own functions
void FldAct_SetMoveCodeFuncs(FieldActor *actor, const FieldActorMoveCode *moveCode);
u16 GetActorZoneID(FieldActor *actor);
BOOL IsActorFlag16(FieldActor *actor);
// Steps through the system's actors from *index, returning TRUE with the next one in *actor
BOOL NextActor(const MMSys *mmSys, FieldActor **actor, u32 *index);
FieldActor *CreateNewActorByEntityNoWKOBJCODE(MMSys *mmSys, const ZoneNPC *npc, u32 zoneId);
// Sets a ZoneNPC's position on the grid
void ZoneNPC_SetGridPos(ZoneNPC *npc, u16 x, u16 z, s32 y);
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
void FldAct_SetDrawStatus(FieldActor *actor, u32 value);
void func_ov012_02167564(FieldActor *actor, u32 value);
// Sets x and z to the center of a tile, leaving y
void ConvGXZToVector(s16 x, s16 z, VecFx32 *pos);
void SpawnAllZoneNPCs(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork);

BOOL FldAct_CheckAcmdEnd(FieldActor *actor);
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
void FldActSys_DetachField(MMSys *system);
void FldActSys_InitDraw(MMSys *system, u16 *cameraAngle);
void FldActSys_WakeUpAllActors(MMSys *system);
void FldActSys_SetShadowsEnabled(MMSys *system, BOOL flag);

BOOL CheckActorFlag(FieldActor *actor, u32 flag);
u32 GetActorMovementFlags(FieldActor *actor);
BOOL CheckActorMovementFlag(FieldActor *actor, u32 flag);
u16 GetActorMoveCode(FieldActor *actor);
s16 GetActorWalkAreaW(FieldActor *actor);
s16 GetActorWalkAreaH(FieldActor *actor);
ActorPositionRail *GetNPCRailPosPtrAddr(FieldActor *actor);
// The rail unit that moves the actor along the rails
RailUnit *FldAct_GetRailUnit(FieldActor *actor);
ActorPositionRail *ClearActorPositionBlock(FieldActor *actor, u32 size);
// Call the move code's unk4 and unk8 functions
void FldAct_CallMoveInitProc(FieldActor *actor);
void FldAct_CallMoveProc(FieldActor *actor);
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
void FldAct_SetAttrDrawOffset(FieldActor *actor, const VecFx32 *offset);
u8 GetActorCollWidth(FieldActor *actor);
u8 GetActorCollHeight(FieldActor *actor);
BOOL FldActSys_AreShadowsEnabled(MMSys *system);
u32 CheckActorMMSysFlag(FieldActor *actor, u32 flag);
// Sets movement flag 0x20
void FldAct_SetMoveStart(FieldActor *actor);
BOOL IsActorMovementDisabled(FieldActor *actor);
BOOL FldAct_IsDrawReady(FieldActor *actor);
BOOL FldAct_IsHeightCalcDisabled(FieldActor *actor);
// Sets or clears movement flag 0x40000, and whether it is set
void FldAct_SetSplashed(FieldActor *actor, BOOL value);
BOOL FldAct_IsSplashed(FieldActor *actor);
BOOL func_ov012_021676f0(FieldActor *actor);
void FldAct_SetReflectingFlag(FieldActor *actor, BOOL value);
BOOL FldAct_CheckReflectingFlag(FieldActor *actor);
BOOL FldAct_IsMapAttrDisabled(FieldActor *actor);
BOOL FldAct_IsLandingDustDisabled(FieldActor *actor);
const FieldActorConfig *GetActorMdlInfo(FieldActor *actor);
u32 CheckMMSysFlag(MMSys *system, u32 flag);
// Resets a strength boulder's saved position in a zone
void ResetStrengthRockSaveDataPosition(MMSys *system, u16 zoneId, u16 uid);
void func_ov036_0218eff4(FieldActor *actor);
// Overlay 36's fldmmdl_acmd.c, not written yet: moves the actor's billboard to the actor's position
void func_ov036_0218ee08(FieldActor *actor);
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
void FldAct_InitMoveProcGrid(FieldActor *actor);
void FldAct_UpdateMoveProcGrid(FieldActor *actor);
void ActorTerrainEffect_ApplyAll(FieldActor *actor);
// The collision flags at a grid position, with the world position's y
u32 ActorRouteCollCheckCore(FieldActor *actor, const VecFx32 *position, s16 x, s16 y, s16 z, u16 dir);
u32 ActorRouteCollCheck(FieldActor *actor, s16 x, s16 y, s16 z, u16 dir);
BOOL CheckActorNewPosOtherActorCollision(const FieldActor *actor, s16 x, s16 y, s16 z);
BOOL CheckActorVolumeOverGPos(FieldActor *actor, s16 x, s16 z, BOOL checkInit);
BOOL IsGPosOutsideActorWalkArea(FieldActor *actor, s16 x, s16 z);
BOOL CheckBlockedCollPathToPosition(FieldActor *actor, u16 dir, VecFx32 position);
BOOL GetTileTypeAtPosByActor(FieldActor *actor, const VecFx32 *position, u32 *tileType);
BOOL GetHeightFromMap(FieldActor *actor, const VecFx32 *position, fx32 *height);
void FldAct_StepGPosInDir(FieldActor *actor, u16 dir);
void SetActorInitialGPosToNowGPos(FieldActor *actor);
u32 FldAct_GetTileTypeInDir(FieldActor *actor, u16 dir);
void FldAct_AddWPos(FieldActor *actor, const VecFx32 *offset);
void FldAct_MoveWPosInDir(FieldActor *actor, u16 dir, fx32 distance);
BOOL CheckRecalcActorY(FieldActor *actor);
BOOL FldAct_CacheTerrainInfo(FieldActor *actor);
// Whether the player is within 17 tiles of the actor on both axes
BOOL FldAct_IsNearPlayer(FieldActor *actor);
void SetRailActorFlag(FieldActor *actor);
void InitRailActor(FieldActor *actor, const ZoneNPC *npc);
void SetActorInitPositionRail(FieldActor *actor, const RailPosition *position);
BOOL FldAct_CanRunMoveProc(FieldActor *actor);
void FldAct_RecalcHeightIfError(FieldActor *actor);
void FldAct_CacheTerrainIfError(FieldActor *actor);
void FldAct_ProcPendingMoveStart(FieldActor *actor);
void FldAct_ProcMoveStartEffects(FieldActor *actor);
void FldAct_ProcMoveEndEffects(FieldActor *actor);
void ActorTerrainEffect_InitParam(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_ApplyOnSet(FieldActor *actor);
void ActorTerrainEffect_ApplyJumpStart(FieldActor *actor);
void ActorTerrainEffect_ApplyMoveEnd(FieldActor *actor);
void ActorTerrainEffect_ApplyJumpEnd(FieldActor *actor);
void ActorTerrainEffect_DeepSand(FieldActor *actor, ActorTerrainEffectParam *param);
u32 ActorTerrainEffect_GetGrassKind(u32 tileClass, u8 season);
void ActorTerrainEffect_GrassStill(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_TallGrass(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_TallGrassUp(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_TallGrassLanding(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_Footmark(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_FallenLeaves(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_Splash(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_SplashOrSwamp(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_ClearSplash(FieldActor *actor, ActorTerrainEffectParam *param);
BOOL ActorTerrainEffect_IsShadowEnabled(FieldActor *actor, ActorTerrainEffectParam *param);
BOOL ActorTerrainEffect_IsShadowTile(ActorTerrainEffectParam *param);
void ActorTerrainEffect_AddShadow(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_UpdateShadowVisible(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_LandingDust(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_RippleOrigY(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_Ripple(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_Reflection(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_UpdateReflection(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_ElectricField(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_None(FieldActor *actor, ActorTerrainEffectParam *param);
void ActorTerrainEffect_Shadow(FieldActor *actor, ActorTerrainEffectParam *param);
void func_ov012_0215ec28(FieldActor *actor);
BOOL GetActorInitPositionRail(FieldActor *actor, RailPosition *position);

// Overlay 12's fldmmdl.c
MMSys *FldActSys_Create(HeapID heapId, u32 capacity, StrengthRockSave *save);
void FldActSys_Free(MMSys *system);
// Updates until the materials have loaded
void FldActSys_ForceFullSync(MMSys *system);
// Creates the actor of the entity with an ID, if it is spawned
FieldActor *SpawnZoneNPCByUID(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork, u16 uid);
u16 GetActorLimit(const MMSys *system);
// The base priority of the actors' tasks
u16 FldActSys_GetTaskPriority(MMSys *system);
HeapID FldActSys_GetFieldHeapID(MMSys *system);
TCBManager *GetMMSysTCBMgr(MMSys *system);
ArcTool *GetMMSysDataArcHandle(MMSys *system);
void FldActSys_BindFieldBlAct(MMSys *system, void *fieldBlAct);
void *FldActSys_GetFieldBlAct(MMSys *system);
void FldActSys_BindActorG3DSystem(MMSys *system, void *actorG3DSystem);
void *FldActSys_GetActorG3DSystem(MMSys *system);
NoGridMapper *FldActSys_GetNoGridMapper(MMSys *system);
// The camera angle that FldActSys_InitDraw set, or 0
u16 FldActSys_GetCameraAngle(MMSys *system);
u32 func_ov012_02166fc0(MMSys *system);
void func_ov012_02166fc4(MMSys *system, u32 value);
void SetActorEvType(FieldActor *actor, u16 evType);
u16 GetActorEvType(FieldActor *actor);
u16 GetActorSpawnFlag(FieldActor *actor);
u32 GetDefaultActorDir(FieldActor *actor);
void SetActorFaceDir(FieldActor *actor, u16 dir);
// The direction the actor faced before
u16 GetActorLastFaceDir(FieldActor *actor);
void SetActorAreaW(FieldActor *actor, s16 w);
void SetActorAreaH(FieldActor *actor, s16 h);
u16 FldAct_GetDrawStatus(FieldActor *actor);
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
void FldAct_SetAcmdState(FieldActor *actor, u16 state);
void FieldActor_NextAcmdState(FieldActor *actor);
u16 FldAct_GetAcmdState(FieldActor *actor);
void GetActorInitGPos(FieldActor *actor, GridPos *pos);
void adjustZPos(FieldActor *actor, s16 dy);
void CopyActorWPosOffset(FieldActor *actor, VecFx32 *dest);
void SetActorWPosOffset(FieldActor *actor, const VecFx32 *offset);
void FldAct_GetOuterDrawOffset(FieldActor *actor, VecFx32 *dest);
void FldAct_SetOuterDrawOffset(FieldActor *actor, const VecFx32 *value);
void FldAct_GetAttrDrawOffset(FieldActor *actor, VecFx32 *dest);
void *FldAct_GetFieldBlAct(FieldActor *actor);
// The model's position offset of the actor's config
void CopyActorPosOffset(FieldActor *actor, VecFx32 *offset);
void FldActSys_SetDrawInited(MMSys *system, BOOL flag);
// Whether the actor exists
BOOL FldAct_IsInUse(FieldActor *actor);
BOOL FldAct_IsDrawCreated(FieldActor *actor);
BOOL func_ov012_0216754c(FieldActor *actor);
void SetActorFlag8000(FieldActor *actor, BOOL value);
BOOL func_ov012_021675b4(FieldActor *actor);
void SetActorFlag256(FieldActor *actor, BOOL value);
void SetActorFlag32(FieldActor *actor, BOOL value);
void FldAct_SetHeightCalcDisabled(FieldActor *actor, BOOL a1);
void FldAct_SetIgnoreDisableAll(FieldActor *actor, BOOL value);
void FldAct_SetMapAttrDisabled(FieldActor *actor, BOOL value);
void func_ov012_021677bc(FieldActor *actor, BOOL value);
void FldAct_SetLandingDustDisabled(FieldActor *actor, BOOL value);
u8 FldAct_GetShadowGroup(FieldActor *actor);
void FldAct_SetForceOffShadow(FieldActor *actor, BOOL value);
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
void FldAct_GetIdentity(FieldActor *actor, FieldActorIdentity *identity);
BOOL FldAct_CheckIdentity(FieldActor *actor, const FieldActorIdentity *identity);
void GetMMSysMdlInfoCacheEntry(MMSys *system, u32 index, FieldActorConfig *config);
u16 GetMMSysMdlInfoCacheEntryCount(MMSys *system);
const FieldActorResGroup *GetNPCMdlInfoG2DRscGroup(const FieldActorConfig *config);
const FieldActorResGroup *GetNPCMdlInfoG3DRscGroup(const FieldActorConfig *config);
// The object code that an object code stands for, through the work values of WKOBJCODE00 and on
u16 ResolvePossibleWKOBJCODE(EventWork *eventWork, u16 objCode);
// How an actor of the event type looks for the player: the types that see only the way they face give 1
u16 EvType_GetSightType(u16 type);
void MoveCodeNone_Init(FieldActor *actor);
void MoveCodeNone_Move(FieldActor *actor);
void MoveCodeNone_Delete(FieldActor *actor);
void MoveCodeNone_Return(FieldActor *actor);
// The size of the strength boulders' save, and its initialization
u32 StrengthRockSave_GetSize(void);
void StrengthRockSave_Init(StrengthRockSave *save);
// Saves the position of the actor's boulder
void SaveActorStrengthRockPosition(FieldActor *actor);
// The count of boulders with a saved position
u32 CountMovedStrengthRocks(MMSys *system);
// Sets a ZoneNPC's position on a rail
void ZoneNPC_SetRailPos(ZoneNPC *npc, u16 railIndex, u16 frontPos, u16 sidePos);

// Overlay 36's functions that fldmmdl.c calls
void func_ov036_0218ed18(MMSys *system);
void func_ov036_0218ed44(FieldActor *actor);
void FldActSys_UpdateResourceLoader(MMSys *system);
void func_ov036_02194924(FieldActor *actor);
void func_ov036_02194938(FieldActor *actor);

#endif // POKEBW2_FIELD_FIELD_ACTOR_H
