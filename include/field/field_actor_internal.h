#ifndef POKEBW2_FIELD_FIELD_ACTOR_INTERNAL_H
#define POKEBW2_FIELD_FIELD_ACTOR_INTERNAL_H

// The layouts of the actors and their system. Names and layouts from swan's FieldActor and FieldActorSystem
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_actor.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The functions of an actor's kind of scene node, by its config's sceneNodeType: onDelete when the actor is deleted
// or its model is changed, suspend and wakeUp around a suspension of the system
typedef struct {
    void (*onCreate)(FieldActor *actor);
    void (*update)(FieldActor *actor);
    void (*onDelete)(FieldActor *actor);
    void (*suspend)(FieldActor *actor);
    void (*wakeUp)(FieldActor *actor);
    u32 (*getBlActIdx)(FieldActor *actor, u32 param);
} FieldActorSceneNodeVTable;

// Overlay 36's tables of the move codes' functions, by move code (fldmmdl_movedata.c's), and of the scene nodes'
// functions, by sceneNodeType (fldmmdl_drawdata.c's)
extern const FieldActorMoveCode *const ACTOR_MOVE_CODE_FUNCTION_TABLES[];
// Overlay 36's fldmmdl_movedata.c: the tables of the action commands' functions, by command, for the actors on the
// grid and on the rails, and the rows of commands that differ only by direction, by direction
extern const ActorAcmdFunc *const ACTOR_ACTION_COMMAND_TABLES[];
extern const ActorAcmdFunc *const ACTOR_ACTION_COMMAND_TABLES_RAIL[];
extern const u32 *const ACMD_DIR_LUT[];
extern const FieldActorSceneNodeVTable *const FIELD_ACTOR_MAIN_VTABLES[];
// Overlay 36's field_rail_loader.c: the functions of the kinds of scene node that FIELD_ACTOR_MAIN_VTABLES lists
extern const FieldActorSceneNodeVTable data_ov036_021cf7d8;
extern const FieldActorSceneNodeVTable data_ov036_021cf7f0;
extern const FieldActorSceneNodeVTable data_ov036_021cf808;
extern const FieldActorSceneNodeVTable data_ov036_021cf820;
extern const FieldActorSceneNodeVTable data_ov036_021cf838;
extern const FieldActorSceneNodeVTable data_ov036_021cf850;
extern const FieldActorSceneNodeVTable data_ov036_021cf868;
extern const FieldActorSceneNodeVTable data_ov036_021cf880;
extern const FieldActorSceneNodeVTable data_ov036_021cf898;
extern const FieldActorSceneNodeVTable data_ov036_021cf8b0;
extern const FieldActorSceneNodeVTable data_ov036_021cf8c8;
extern const FieldActorSceneNodeVTable data_ov036_021cf8e0;
extern const FieldActorSceneNodeVTable data_ov036_021cf8f8;
extern const FieldActorSceneNodeVTable data_ov036_021cf910;
extern const FieldActorSceneNodeVTable data_ov036_021cf928;
extern const FieldActorSceneNodeVTable data_ov036_021cf940;
extern const FieldActorSceneNodeVTable data_ov036_021cf958;
// Overlay 36's fldmmdl_g3dobj_draw.c: the functions of the actors drawn as a 3D model
extern const FieldActorSceneNodeVTable data_ov036_021cf9a0;

// A position on the grid
typedef struct {
    s16 x;
    s16 y;
    s16 z;
} GPosXYZ;

struct FieldActor {
    u32 flags;
    u32 movementFlags;
    u16 uid;
    u16 zoneId;
    u16 modelId;
    u16 moveCode;
    u16 evType;
    u16 spawnFlag;
    u16 scrId;
    u16 defaultDir;
    u16 faceDir;
    u16 motionDir;
    u16 lastFaceDir;
    u16 lastMotionDir;
    u16 param0;
    u16 param1;
    u16 param2;
    u16 nextAcmd;
    u16 acmdState;
    u16 unk2A;
    s16 areaW;
    s16 areaH;
    GPosXYZ defaultGPos;
    // The position before the last move
    GPosXYZ initGPos;
    GPosXYZ gPos;
    s16 unk42;
    VecFx32 wPos;
    VecFx32 wPosOffset;
    VecFx32 unk5C;
    VecFx32 unk68;
    u32 currentTileUnder;
    u32 currentTileUnderOrigY;
    u8 collisionWidth;
    u8 collisionHeight;
    s8 modelPosOffsetX;
    s8 modelPosOffsetY;
    s8 modelPosOffsetZ;
    u8 shadowGroup;
    u16 padShadowGroup;
    TCB *tcb;
    MMSys *actorSystem;
    const FieldActorMoveCode *moveCodeVTable;
    const FieldActorSceneNodeVTable *sceneNodeVTable;
    ActorPositionRail defaultRailPos;
    u32 unkA0;
    u8 unkA4[0x10];
    u8 unkB4[0x10];
    u8 blActWork[0x20];
    FieldActorConfig config;
};

struct MMSys {
    u32 flags;
    u16 actorCapacity;
    s16 actorCount;
    HeapID heapId;
    HeapID fieldHeapId;
    u16 unkC;
    u16 unkE;
    u32 unk10;
    ArcTool *dataArc;
    ArcTool *actorConfigArc;
    FieldActor *actorHeap;
    void *tcbMgrHeap;
    TCBManager *tcbMgr;
    FieldBlAct *fieldBlAct;
    ActorG3DSystem *actorG3DSystem;
    StrengthRockSave *mmodelSave;
    GameData *gameData;
    FieldG3DMapper *g3dMapper;
    NoGridMapper *noGridMapper;
    Field *field;
    u16 *cameraAngle;
    FieldColorPostFX *colorPostFx;
    // The count of cached configs and their object codes, then the configs, from the word after the even count of codes
    u16 *actorConfigCache;
};

#endif // POKEBW2_FIELD_FIELD_ACTOR_INTERNAL_H
