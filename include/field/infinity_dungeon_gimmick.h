#ifndef POKEBW2_FIELD_INFINITY_DUNGEON_GIMMICK_H
#define POKEBW2_FIELD_INFINITY_DUNGEON_GIMMICK_H

#include "types.h"
#include "field/infinity_dungeon.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// infinity_dungeon_gimmick.c, overlay 127, which the ROM names: the field gimmick of the Black Tower and White
// Treehollow, which holds the dungeon and moves the player between its rooms

// Where the player is put in the zone of the room that a move leads to
typedef struct {
    VecFx32 pos;
    u32 zoneId;
} InfinityDungeonDestination;

#define INFINITY_DUNGEON_OBJECT_COUNT 4
#define INFINITY_DUNGEON_WALL_COUNT 4

// A model of the scene, an actor of the field's expansion objects
typedef struct {
    u32 scene;
    u32 actor;
    u32 unk8;
    FieldExpObjSystem *expObj;
} InfinityDungeonObject;

typedef struct {
    u32 zoneId;
    FieldExpObjSystem *expObjSystem;
    Field *field;
    InfinityDungeonObject objects[INFINITY_DUNGEON_OBJECT_COUNT];
    KeyDataSave *keyData;
    u32 unk50;
    InfinityDungeon *dungeon;
    FieldPropHandle *walls[INFINITY_DUNGEON_WALL_COUNT];
} InfinityDungeonGimmick; // what InfinityDungeonGimmick_Setup allocates in the field's gimmick work

// The event of a move to another room, which infinity_dungeon.c also sets up
struct InfinityDungeonRoomChange {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    KeyDataSave *keyData;
    FieldG3DMapper *g3dMapper;
    FieldPropSystem *propSystem;
    MMSys *actorSystem;
    InfinityDungeon *dungeon;
    u16 heapId;
    u8 mode; // 0 for a room of the same floor, 1 for the arrival on a floor
    u8 inTransition; // 8 for the field's own transition into the room, 7 for the arrival on a floor
    u8 outTransition; // the same out of the room
    u16 direction; // up, down, left, right, or 4 for the arrival
    InfinityDungeonDestination destination;
    const u16 *roomZones;
    u32 room;
    u32 floor;
    s32 nextRoom;
    u32 nextFloor;
    u16 level;
};

void InfinityDungeonGimmick_Setup(Field *field);
void InfinityDungeonGimmick_Delete(Field *field);
void InfinityDungeonGimmick_Update(Field *field);
InfinityDungeon *InfinityDungeonGimmick_GetDungeon(Field *field);
GameEvent *InfinityDungeonGimmick_CreateDoorEvent(GameSystem *gsys, Field *field, u16 kind, u16 object);
GameEvent *InfinityDungeonGimmick_CreateMachineEvent(GameSystem *gsys, Field *field);
GameEvent *InfinityDungeonGimmick_CreateLiftEvent(GameSystem *gsys, Field *field);
void InfinityDungeonGimmick_CallRoomChange(ScriptWork *work, GameSystem *gsys, InfinityDungeon *dungeon, u16 direction, u8 inTransition,
                         u8 outTransition);
void InfinityDungeonGimmick_CallFloorChange(ScriptWork *work, GameSystem *gsys, InfinityDungeon *dungeon, u16 a3);
void InfinityDungeonRoomChange_Init(InfinityDungeonRoomChange *roomChange, GameSystem *gsys);
InfinityDungeonRoom *InfinityDungeonRoomChange_GetRoom(InfinityDungeonRoomChange *roomChange);
// Puts a wall prop in a passage
void InfinityDungeonGimmick_SetWall(InfinityDungeonGimmick *gimmick, FieldPropHandle *handle, u32 index);
void InfinityDungeonRoomChange_CreateTrainers(InfinityDungeonRoomChange *roomChange, InfinityDungeonRoom *room);
void InfinityDungeonRoomChange_CreateWalls(InfinityDungeonRoomChange *roomChange, InfinityDungeonRoom *room, u32 floor, u32 roomIndex);

#endif // POKEBW2_FIELD_INFINITY_DUNGEON_GIMMICK_H
