#ifndef POKEBW2_FIELD_INFINITY_DUNGEON_CREATOR_H
#define POKEBW2_FIELD_INFINITY_DUNGEON_CREATOR_H

#include "types.h"
#include "gfl/heap.h"
#include "field/infinity_dungeon.h"

// infinity_dungeon_creator.c, overlay 127, which the ROM names: lays out the dungeon's trainers from its seed

// Chooses the trainers of the floors by rank, then the goal's room and trainer, the reward and the special trainers
// that stand on the floors, all from `seed`
void InfinityDungeonCreator_Create(InfinityDungeon *dungeon, u32 rank, u32 seed, u8 floorCount, HeapID heapId);

#endif // POKEBW2_FIELD_INFINITY_DUNGEON_CREATOR_H
