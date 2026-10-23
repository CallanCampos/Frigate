#include "types.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "save/unity_tower_floor_savedata.h"

// The save block of the Unity Tower's floor: the visitors on the floor the player is on, which the game data keeps
// while the game runs. The ROM has no string for the file, so the name is descriptive; unity_tower_savedata.c is the
// tower's survey. Block 0x3e.

u32 UnityTowerSave_GetBlockSize(void) {
    return UNITY_TOWER_FLOOR_BLOCK_SIZE;
}

void UnityTowerSave_Init(UnityTowerFloor *save) {
    MI_CpuFill8(save, 0, UNITY_TOWER_FLOOR_BLOCK_SIZE);
}

void UnityTowerSave_Store(const UnityTowerFloor *floor, SaveControl *save) {
    MIi_CpuCopy32(floor, SaveControl_GetBlockPtr(save, SAVE_BLOCK_UNITY_TOWER_FLOOR), UNITY_TOWER_FLOOR_BLOCK_SIZE);
}

void UnityTowerSave_Load(SaveControl *save, UnityTowerFloor *floor) {
    MIi_CpuCopy32(SaveControl_GetBlockPtr(save, SAVE_BLOCK_UNITY_TOWER_FLOOR), floor, UNITY_TOWER_FLOOR_BLOCK_SIZE);
}
