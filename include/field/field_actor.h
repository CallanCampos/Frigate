#ifndef POKEBW2_FIELD_FIELD_ACTOR_H
#define POKEBW2_FIELD_FIELD_ACTOR_H

#include "types.h"
#include "constants/actor_event_types.h"
#include "constants/directions.h"
#include "constants/move_codes.h"
#include "field/zone.h"
#include "gfl/billboard_actor.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The UID of the player's actor
#define ACTOR_UID_PLAYER 0xff

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

// A config's billboardSize: the size of the actor's billboard in pixels, by field_actor_drawdata.c's
// ACTOR_BILLBOARD_SIZES_INT
enum {
    ACTOR_BILLBOARD_SIZE_32,
    ACTOR_BILLBOARD_SIZE_16,
    ACTOR_BILLBOARD_SIZE_64,
};

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

// The saved positions of the strength boulders that were moved, in grid units, by the slots of field_actor.c's table of
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
// The flag that keeps an actor's face direction, which CheckSetActorFaceDir leaves alone while it is set
#define ACTOR_FLAG_FACE_DIR_LOCK 0x8
// The flag of an actor that moves on rails, which SetRailActorFlag sets
#define ACTOR_FLAG_RAIL 0x2000
void SetActorFlag(FieldActor *actor, u32 flag);
void ClearActorFlag(FieldActor *actor, u32 flag);
// The movement flag that hides an actor's shadow, which the actor terrain effects set off a shadow tile
#define ACTOR_MOVEMENT_FLAG_HIDE_SHADOW 0x8000
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
void RailActorAnim_GetBillboardActorAnime(FieldActor *actor, u16 *a1, u16 *a2);
void RailActorAnim_SetBillboardActorAnime(FieldActor *actor, u16 a1, u16 a2);
// Sets the frame of the model's animation for a time. FALSE if the actor has no model
BOOL RailActorAnim_SyncFrameGroup(FieldActor *actor, u16 time);
// Overlay 36's field_actor_acmd.c: whether the actor is free of its action command, and field_actor_movecode_pair.c's
// setting of the field effect it hides in
BOOL FieldAcmdTCB_CheckAnimePaused(FieldActor *actor);
void FieldActorMove_SetHideEffect(FieldActor *actor, FieldEffectTask *task);
// Overlay 36's field_actor_billboard.c: the freeing of the billboard actors' work, the flag that their load callback
// sets, their billboard system, and the change of an actor's model to another object code
void FldBillboardActor_Exit(MMSys *system);
u8 FldBillboardActor_GetEntryHookFlag(FieldActor *actor);
BillboardActorSys *FieldBillboardActor_GetBillboardActorSys(FieldBillboardActor *billboardActor);
void FldAct_SwapObjCode(FieldActor *actor, u16 newObjCode);
// The creation of an actor's billboard actor into *index, and its deletion
BOOL FldBillboardActor_AddFieldActorOrReserve(FieldActor *actor, u16 *index);
void FldBillboardActor_DeleteFieldActor(FieldActor *actor, u32 index);
// The creation of a billboard actor of an object code at a position, into out[1] with the material's flags in out[0],
// calling back once a material that has to be loaded first is there; its deletion; and the scale of an actor's
// billboard
BOOL FldBillboardActor_AddActorPosOrReserve(MMSys *system, const FieldActorConfig *config, u16 *out, VecFx32 *pos,
                                            void (*callback)(u16 index, void *data), void *callbackData, u32 lights);
void FldBillboardActor_DeleteActorPos(MMSys *system, u16 *out);
void FldBillboardActor_GetActorSize(FieldActor *actor, u16 *scaleX, u16 *scaleY);
// Overlay 36's field_rail_loader.c: sets a flag of the actor's rail billboard work
void RailActorAnim_SetState(FieldActor *actor, BOOL value);
// The script call of an actor's script; the animation and frame of the billboard actors of object codes 0xd3 and 0xd5,
// and of 0xdd and 0xde (FALSE without a billboard actor); and whether the billboard actor of object code 0x7b or 0x176
// has run its animation, with the reset of that animation
GameEvent *CreatePlayerXActorScriptCall(GameSystem *gsys, Field *field, u32 scriptId, FieldActor *player,
                                        FieldActor *actor, HeapID heapId);
BOOL RailActorAnim_GetBillboardActorAnimeTypeA(FieldActor *actor, u16 *anim, u16 *step);
BOOL RailActorAnim_GetBillboardActorAnimeTypeB(FieldActor *actor, u16 *anim, u16 *step);
BOOL RailActorAnim_CheckBillboardActorReady(FieldActor *actor);
void RailActorAnim_ResetBillboardActorAnime(FieldActor *actor);
// The functions that run an action command: what to do on the first frame, each frame and at the end
typedef BOOL (*ActorAcmdFunc)(FieldActor *actor);

// The work of a jump along the rail, which field_actor_g3dobj.c's RailActorAcmd_InitJumpWithSe starts and
// fld_act_rail.c updates
typedef struct {
    s32 speed;
    s32 progress;
    u16 step;
    u16 phase;
    s8 dir;
    s8 timer;
    s8 drawStatus;
    s8 height;
} ActorRailJumpWork;

// Overlay 36's field_actor_g3dobj.c: the start, update and end functions of the action commands that the rail actors'
// tables in fld_act_rail.c name, and the jump along the rail that their start functions begin
BOOL RailActor_AcSetAcmdEnd(FieldActor *actor);
BOOL RailActor_AcNextState(FieldActor *actor);
BOOL RailActor_AcFaceU(FieldActor *actor);
BOOL RailActor_AcFaceD(FieldActor *actor);
BOOL RailActor_AcFaceL(FieldActor *actor);
BOOL RailActor_AcFaceR(FieldActor *actor);
BOOL RailActor_AcWalk1(FieldActor *actor);
BOOL RailActor_AcWalkEnd(FieldActor *actor);
BOOL RailActor_AcWalkU16F0(FieldActor *actor);
BOOL RailActor_AcWalkD16F0(FieldActor *actor);
BOOL RailActor_AcWalkL16F0(FieldActor *actor);
BOOL RailActor_AcWalkR16F0(FieldActor *actor);
BOOL RailActor_AcWalkU8F0(FieldActor *actor);
BOOL RailActor_AcWalkD8F0(FieldActor *actor);
BOOL RailActor_AcWalkL8F0(FieldActor *actor);
BOOL RailActor_AcWalkR8F0(FieldActor *actor);
BOOL RailActor_AcWalkU4F0(FieldActor *actor);
BOOL RailActor_AcWalkD4F0(FieldActor *actor);
BOOL RailActor_AcWalkL4F0(FieldActor *actor);
BOOL RailActor_AcWalkR4F0(FieldActor *actor);
BOOL RailActor_AcWalkU2F0(FieldActor *actor);
BOOL RailActor_AcWalkD2F0(FieldActor *actor);
BOOL RailActor_AcWalkL2F0(FieldActor *actor);
BOOL RailActor_AcWalkR2F0(FieldActor *actor);
BOOL RailActor_AcDashU4F0(FieldActor *actor);
BOOL RailActor_AcDashD4F0(FieldActor *actor);
BOOL RailActor_AcDashL4F0(FieldActor *actor);
BOOL RailActor_AcDashR4F0(FieldActor *actor);
BOOL RailActor_AcStayWalk1(FieldActor *actor);
BOOL RailActor_AcStayWalkU32F0(FieldActor *actor);
BOOL RailActor_AcStayWalkD32F0(FieldActor *actor);
BOOL RailActor_AcStayWalkL32F0(FieldActor *actor);
BOOL RailActor_AcStayWalkR32F0(FieldActor *actor);
BOOL RailActor_AcStayWalkU16F0(FieldActor *actor);
BOOL RailActor_AcStayWalkD16F0(FieldActor *actor);
BOOL RailActor_AcStayWalkL16F0(FieldActor *actor);
BOOL RailActor_AcStayWalkR16F0(FieldActor *actor);
BOOL RailActor_AcStayWalkU8F0(FieldActor *actor);
BOOL RailActor_AcStayWalkD8F0(FieldActor *actor);
BOOL RailActor_AcStayWalkL8F0(FieldActor *actor);
BOOL RailActor_AcStayWalkR8F0(FieldActor *actor);
BOOL RailActor_AcStayWalkU2F0(FieldActor *actor);
BOOL RailActor_AcStayWalkD2F0(FieldActor *actor);
BOOL RailActor_AcStayWalkL2F0(FieldActor *actor);
BOOL RailActor_AcStayWalkR2F0(FieldActor *actor);
BOOL RailActor_AcStayWalkU4F0(FieldActor *actor);
BOOL RailActor_AcStayWalkD4F0(FieldActor *actor);
BOOL RailActor_AcStayWalkL4F0(FieldActor *actor);
BOOL RailActor_AcStayWalkR4F0(FieldActor *actor);
void RailActorAcmd_InitJumpWithSe(FieldActor *actor, u16 dir, u32 speed, u32 timer, u16 drawStatus, s16 height,
                                  u16 step);
// Also field_actor_g3dobj.c's: the freeing of the actors' 3D system, their 3D object system, the creation and deletion
// of an actor's 3D object, the tile under a rail position, and the direction an actor on the rails faces for a camera
// angle
void FieldActorG3DSystem_Delete(MMSys *system);
FieldG3DObjSystem *FldAct_GetG3DObjSystem(FieldActor *actor);
u16 FieldActorG3DSystem_AddObj(FieldActor *actor);
void FieldActorG3DSystem_DelObj(FieldActor *actor, u16 objIdx);
u32 RailActor_GetLocationAttr(FieldActor *actor, const RailPosition *pos);
u8 RailActor_GetDirByAngle(FieldActor *actor, int angle);
// The unit vector of a direction along the actor's rail
void RailActor_GetDrawOffsetByDir(FieldActor *actor, u32 dir, VecFx16 *dest);
void RailActor_GetDrawOffset(FieldActor *actor, VecFx16 *dest);
// The actor's rail position, the one a step ahead of it, and the one a step in a direction, FALSE if there is none
void GetActorRailPos(FieldActor *actor, RailPosition *position);
BOOL RailActor_GetFrontLocation(FieldActor *actor, RailPosition *position);
BOOL GetActorRailPosForDir(FieldActor *actor, u16 dir, RailPosition *position);
// Overlay 36's field_actor_rail.c: the actor's position as its rail unit holds it, the unit's step, the collision flags
// of a step in a direction (1 off the rails, 2 a blocked tile, 4 another actor), the first actor on a rail position
// other than exclude, and the change of the actor's grid position to the one a step ahead
void RailActor_GetNowLocation(FieldActor *actor, RailPosition *position);
void RailActor_UpdateRailWork(FieldActor *actor);
void RailActor_SetMoveRequest(FieldActor *actor, u16 dir, u32 speed);
u32 RailActor_GetHitBitsDirNoAttr(FieldActor *actor, u16 dir);
u32 RailActor_GetHitBitsDir(FieldActor *actor, u16 dir, u32 *tile);
FieldActor *FindActorByRailPosExcluding(MMSys *system, const RailPosition *position, BOOL checkInit,
                                        FieldActor *exclude);
void RailActor_SetGridPosByDir(FieldActor *actor, u16 dir);
// The move codes of the rail actors (0x45 to 0x54): their init, move, delete and recover functions; the waits turn to
// a random direction of a set of DIR_LISTS, the walks step to one, and the patrol walks back and forth
void RailActor_StayInit(FieldActor *actor);
void RailActor_StayMove(FieldActor *actor);
void RailActor_StayDelete(FieldActor *actor);
void RailActor_Recover(FieldActor *actor);
void RailActor_WaitInitAll(FieldActor *actor);
void RailActor_WaitInitUpLeft(FieldActor *actor);
void RailActor_WaitInitUpRight(FieldActor *actor);
void RailActor_WaitInitDownLeft(FieldActor *actor);
void RailActor_WaitInitDownRight(FieldActor *actor);
void RailActor_WaitInitUpDownLeft(FieldActor *actor);
void RailActor_WaitInitUpDownRight(FieldActor *actor);
void RailActor_WaitInitUpLeftRight(FieldActor *actor);
void RailActor_WaitInitDownLeftRight(FieldActor *actor);
void RailActor_WaitInitUpDown(FieldActor *actor);
void RailActor_WaitInitLeftRight(FieldActor *actor);
void RailActor_WaitMove(FieldActor *actor);
void RailActor_WaitDelete(FieldActor *actor);
void RailActor_WalkInitAll(FieldActor *actor);
void RailActor_WalkInitUpDown(FieldActor *actor);
void RailActor_WalkMove(FieldActor *actor);
void RailActor_WalkDelete(FieldActor *actor);
void RailActor_PatrolInit(FieldActor *actor);
void RailActor_PatrolMove(FieldActor *actor);
void RailActor_PatrolDelete(FieldActor *actor);
// Overlay 36's rail_data_handle.c: clears the rail data to no points, with the curves' and cameras' function tables
void RailDataHandle_Reset(RailDataHandle *handle);
u32 GetIndexOfObjID(u16 objCode);

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
u16 GetAcmdForDir(u16 dir, u16 acmd);
// The collision flags of the tile next to the actor in a direction
u32 ActorRouteCollCheckOneTileInDir(FieldActor *actor, u16 dir);
// Starts a movement command
void FldAct_SetLocalAcmd(FieldActor *actor, u16 acmd);
// Starts the movement command of a direction, as GetAcmdForDir finds it for acmd: the move codes hold the command in a
// u32, which the call narrows again
static inline void StartDirAcmd(FieldActor *actor, u32 dir, u32 acmd) {
    u32 result = GetAcmdForDir(dir, acmd);

    FldAct_SetLocalAcmd(actor, result);
}
void FldAct_SetAcmd(FieldActor *actor, u16 acmd);
BOOL FldAct_EndAcmd(FieldActor *actor);
// Whether the actor's movement command has finished
BOOL FieldAcmdTCB_Update(FieldActor *actor);
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
// An actor's draw status, which picks the animation of its model: a billboard actor shows animation status * 4 + its
// direction, or status + 30 from DRAW_STATUS_ANIME_10 on for the models that have one animation for all directions. The
// walking models stand still, and walk at the frames a tile that the names give; DRAW_STATUS_ANIME_10, 11 and 12 are
// their runs at 4, 6 and 2 frames a tile. Other models give the numbered ones their own animations, which the action
// commands that wait for an animation to end play
enum {
    DRAW_STATUS_STOP,
    DRAW_STATUS_WALK_32F,
    DRAW_STATUS_WALK_16F,
    DRAW_STATUS_WALK_8F,
    DRAW_STATUS_WALK_4F,
    DRAW_STATUS_WALK_2F,
    DRAW_STATUS_WALK_6F,
    DRAW_STATUS_WALK_3F,
    DRAW_STATUS_WALK_7F,
    DRAW_STATUS_WALK_12F,
    DRAW_STATUS_ANIME_10,
    DRAW_STATUS_ANIME_11,
    DRAW_STATUS_ANIME_12,
    DRAW_STATUS_ANIME_13,
};
void FldAct_SetDrawStatus(FieldActor *actor, u32 value);
void func_ov012_02167564(FieldActor *actor, u32 value);
// Sets x and z to the center of a tile, leaving y
void ConvGXZToVector(s16 x, s16 z, VecFx32 *pos);
void SpawnAllZoneNPCs(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork);

BOOL FldAct_CheckAcmdEnd(FieldActor *actor);
FldActMatColorPreset *FldActMatColorPreset_Create(HeapID heapId);
void FldActMatColorPreset_Free(FldActMatColorPreset *preset);
void FldActMatColorPreset_Load(FldActMatColorPreset *preset, u32 presetId);
void FldActMatColorPreset_Apply(FldActMatColorPreset *preset, BillboardActorScene *scene);
void FldActSys_AttachField(MMSys *system, HeapID heapId, GameData *gameData, Field *field, FieldG3DMapper *mapper,
                           NoGridMapper *noGridMapper, FieldColorPostFX *colorPostFx);
void FldActSys_InitBillboardActor(MMSys *system, BillboardActorSys *billboardActor, u32 count);
void FldActSys_LoadCachedBillboardActor(MMSys *system);
void FldActSys_LoadStaticBillboardActor(MMSys *system, u16 objCode);
void FldActSys_Update(MMSys *system);
void FldActSys_FinishAsyncMatLoad(MMSys *system);
void FldActSys_FinishAsyncMatLoadSafe(MMSys *system);
BOOL FldActSys_IsAsyncLoadPending(MMSys *system);
void FldActSys_SuspendAllActors(MMSys *system);
void DeleteAllActors(MMSys *system);
void FieldActorG3DSystem_Create(MMSys *system, FieldG3DObjSystem *g3dObjSystem);
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
// The work of the actor's move code, which each move code lays out its own way (ActorPositionRail for the rail
// ones), as it is or cleared to a size
void *GetNPCRailPosPtrAddr(FieldActor *actor);
// The rail unit that moves the actor along the rails
RailUnit *FldAct_GetRailUnit(FieldActor *actor);
void *ClearActorPositionBlock(FieldActor *actor, u32 size);
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
BOOL FldAct_CheckStatusBitAttrOffsetOff(FieldActor *actor);
void FldAct_SetReflectingFlag(FieldActor *actor, BOOL value);
BOOL FldAct_CheckReflectingFlag(FieldActor *actor);
BOOL FldAct_IsMapAttrDisabled(FieldActor *actor);
BOOL FldAct_IsLandingDustDisabled(FieldActor *actor);
const FieldActorConfig *GetActorMdlInfo(FieldActor *actor);
u32 CheckMMSysFlag(MMSys *system, u32 flag);
// Resets a strength boulder's saved position in a zone
void ResetStrengthRockSaveDataPosition(MMSys *system, u16 zoneId, u16 uid);
void FieldAcmdTCB_RunCmds(FieldActor *actor);
// Overlay 36's field_actor_acmd.c: moves the actor's billboard to the actor's position
void FieldAcmdSyncBillboardPos(FieldActor *actor);
// Overlay 36's field_actor_evtype.c: runs the init function of the actor's event type, and whether both checks of the
// type pass
void FieldActorMovecode_InitEventTypeProcThunk(FieldActor *actor);
BOOL FieldActorMovecode_UpdateEventTypeProc(FieldActor *actor);
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
    FieldEffects *effects;
    u8 season;
} ActorTerrainEffectParam;

// Overlay 12's field_actor_tool.c
void FldAct_InitMoveProcGrid(FieldActor *actor);
void FldAct_UpdateMoveProcGrid(FieldActor *actor);
void ActorTerrainEffect_ApplyAll(FieldActor *actor);
// The collision flags at a grid position, with the world position's y: the position is outside the actor's walk area,
// the way there is blocked, another actor stands there, its height is too far from y or the map has none there, and
// it is out of the map's bounds
#define ACTOR_COLL_WALK_AREA 0x1
#define ACTOR_COLL_BLOCKED 0x2
#define ACTOR_COLL_ACTOR 0x4
#define ACTOR_COLL_HEIGHT 0x8
#define ACTOR_COLL_OUT_OF_BOUNDS 0x10
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
void FldAct_StopAcmdWalkCodes(FieldActor *actor);
BOOL GetActorInitPositionRail(FieldActor *actor, RailPosition *position);

// Overlay 12's field_actor.c
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
void FldActSys_BindFieldBillboardActor(MMSys *system, FieldBillboardActor *fieldBillboardActor);
FieldBillboardActor *FldActSys_GetFieldBillboardActor(MMSys *system);
void FldActSys_BindActorG3DSystem(MMSys *system, ActorG3DSystem *actorG3DSystem);
ActorG3DSystem *FldActSys_GetActorG3DSystem(MMSys *system);
NoGridMapper *FldActSys_GetNoGridMapper(MMSys *system);
// The camera angle that FldActSys_InitDraw set, or 0
u16 FldActSys_GetCameraAngle(MMSys *system);
u32 func_ov012_02166fc0(MMSys *system);
void func_ov012_02166fc4(MMSys *system, u32 value);
// The number of event types (constants/actor_event_types.h), by which field_actor_evtype.c's tables go
#define ACTOR_EVENT_TYPE_MAX 21
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
void *FldAct_InitMoveSubProcWork(FieldActor *actor, u32 size);
void *FldAct_GetMoveSubProcWork(FieldActor *actor);
void *FldAct_InitMoveCmdWork(FieldActor *actor, u32 size);
void *FldAct_GetMoveCmdWork(FieldActor *actor);
void *FldAct_ResetBillboardActorWork(FieldActor *actor, u32 size);
void *FldAct_GetBillboardActorWorkPtr(FieldActor *actor);
u32 FldAct_GetBillboardActorIdx(FieldActor *actor, u32 param);
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
FieldBillboardActor *FldAct_GetFieldBillboardActor(FieldActor *actor);
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
void FldAct_SetMoveBitAttrEffectOff(FieldActor *actor, BOOL value);
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
// How a Trainer looks for the player: two of the event types, as EvType_GetSightType gives them
enum {
    TRAINER_SIGHT_FACING = ACTOR_EVENT_TYPE_TRAINER,
    TRAINER_SIGHT_ALL_DIRS = ACTOR_EVENT_TYPE_TRAINER_ALL_DIRS,
};
// How an actor of the event type looks for the player: the types that see only the way they face give
// TRAINER_SIGHT_FACING, and the others their own type, of which TRAINER_SIGHT_ALL_DIRS looks every way
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

// Overlay 36's functions that field_actor.c calls
void FieldAcmdTCB_DeleteDrawSys(MMSys *system);
void FieldAcmdTCB_DrawIfInitialized(FieldActor *actor);
void FldActSys_UpdateResourceLoader(MMSys *system);
void RailActor_InitMove(FieldActor *actor);
void RailActor_UpdateMove(FieldActor *actor);

#endif // POKEBW2_FIELD_FIELD_ACTOR_H
