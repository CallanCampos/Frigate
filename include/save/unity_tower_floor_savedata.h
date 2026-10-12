#ifndef POKEBW2_SAVE_UNITY_TOWER_FLOOR_SAVEDATA_H
#define POKEBW2_SAVE_UNITY_TOWER_FLOOR_SAVEDATA_H

#include "types.h"
#include "field/unity_tower.h"
#include "struct_decls.h"

// Block 0x3e, the visitors on the Unity Tower's floor (unity_tower_floor_savedata.c, a descriptive name).
// UnityTowerSave_Init is declared in field/unity_tower.h

#define UNITY_TOWER_FLOOR_BLOCK_SIZE 16

u32 UnityTowerSave_GetBlockSize(void);
// Copies the game data's floor to the save, and back
void UnityTowerSave_Store(const UnityTowerFloor *floor, SaveControl *save);
void UnityTowerSave_Load(SaveControl *save, UnityTowerFloor *floor);

#endif // POKEBW2_SAVE_UNITY_TOWER_FLOOR_SAVEDATA_H
