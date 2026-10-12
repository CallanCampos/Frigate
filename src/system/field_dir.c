#include "types.h"
#include "constants/directions.h"
#include "field/zone.h"

// The rail's direction keys for the directions of the actors.
// The file's name is a guess, from what it does: the ROM has no string for it. The function's name is swan's

// Up, right, down and left are the rail's keys 1 to 4
static const u8 DIR_TO_RAIL_DIR[] = {
    1, // DIR_UP
    3, // DIR_DOWN
    4, // DIR_LEFT
    2, // DIR_RIGHT
};

u32 ConvDirToRailDir(u32 direction) {
    return DIR_TO_RAIL_DIR[direction];
}
