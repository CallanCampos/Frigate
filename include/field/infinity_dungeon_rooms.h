#ifndef POKEBW2_FIELD_INFINITY_DUNGEON_ROOMS_H
#define POKEBW2_FIELD_INFINITY_DUNGEON_ROOMS_H

#include "types.h"
#include "field/infinity_dungeon.h"

// infinity_dungeon_rooms.c, overlay 127, named after what it holds (the ROM has no name for it): the dungeon's room
// and NPC templates, its trainers and the tables by rank, the generator's shuffles, the seeds, the trainer slots of a
// room, and the layout of the floors

u8 InfinityDungeon_GetRoomNpcCount(u8 a0);
u16 InfinityDungeon_IsTargetSlot(InfinityDungeon *gimmick, u8 a1, u8 a2);
u8 InfinityDungeon_GetVersion(InfinityDungeon *gimmick);
u16 InfinityDungeon_GetTrainerId(InfinityDungeon *gimmick, u8 a1, u8 a2);
const u16 *InfinityDungeon_GetRoomZones(u8 version);
const InfinityDungeonPos *InfinityDungeon_GetFloorPos(u8 version);
const InfinityDungeonPos *InfinityDungeon_GetArrivalPos(u32 index);
const InfinityDungeonPos *InfinityDungeon_GetRoomEntryPos(u8 version, u8 kind, u16 direction);
const InfinityDungeonPos *InfinityDungeon_GetTrainerPos(u8 version, u8 kind, u8 index);
void InfinityDungeon_Seed(InfinityDungeon *gimmick, s32 seed);
// (added for infinity_dungeon_creator.c)
const InfinityDungeonTrainer *InfinityDungeon_GetTrainer(u16 trainer);
u8 InfinityDungeon_GetTrainerClass(u16 trainer);
// The share of each of the 30 trainer classes among a rank's trainers
const u8 *InfinityDungeon_GetTrainerClassShares(u16 rank, u8 version);
// Shuffle an array of u32 or u16 with the generator
void InfinityDungeon_ShuffleU32(u32 *array, int count, InfinityDungeonRandom *rnd);
void InfinityDungeon_ShuffleU16(u16 *array, int count, InfinityDungeonRandom *rnd);
// The room of the floor that a trainer slot is in
u32 InfinityDungeon_GetSlotRoom(InfinityDungeon *dungeon, u32 floor, int slot);
// The trainer slot of a floor that is the trainer-th of the room
u32 InfinityDungeon_GetTrainerSlot(InfinityDungeon *dungeon, unsigned int floor, unsigned int room, u8 trainer);
// infinity_dungeon_rooms.c: the templates of the rooms and their NPCs, the tables that go with the ranks and the layout
const InfinityDungeonNpc *InfinityDungeon_GetNpc(u16 npc);
const InfinityDungeonRoomTemplate *InfinityDungeon_GetRoomTemplate(u16 type);
// The size of the floors of a rank, in rooms
void InfinityDungeon_GetFloorSize(u32 rank, u32 *width, u32 *height);
const InfinityDungeonLevelRange *InfinityDungeon_GetLevelRange(u16 mode);
u8 InfinityDungeon_GetIvs(u16 mode);
// Which of the NPC scripts an NPC of the object code takes
u32 InfinityDungeon_GetNpcScriptIndex(InfinityDungeon *dungeon, u32 objCode, u8 floor, u8 slot);
u8 InfinityDungeon_GetNpcIndex(u32 objCode);
// Digs the passages of the floors from the seed, and puts the kinds of rooms in them
void InfinityDungeon_LayoutFloors(InfinityDungeon *dungeon, u32 rank, u32 seed, int width, int height, u8 floorCount);

#endif // POKEBW2_FIELD_INFINITY_DUNGEON_ROOMS_H
