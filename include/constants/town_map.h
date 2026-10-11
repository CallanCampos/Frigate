#ifndef POKEBW2_CONSTANTS_TOWN_MAP_H
#define POKEBW2_CONSTANTS_TOWN_MAP_H

// The pseudo flags the town map asks about besides event flags (func_ov012_02160f74), which data/town_map/places.json
// names
#define TOWNMAP_FLAG_ONE_SHOT_DR 0xf000
#define TOWNMAP_FLAG_UNITY_TOWER_VISITED 0xf001
// Set while the Plasma Frigate is at its own place on the map (EVENT_WORK_PLASMA_FRIGATE_LOCATION is 3)
#define TOWNMAP_FLAG_PLASMA_FRIGATE 0xf002

#endif // POKEBW2_CONSTANTS_TOWN_MAP_H
