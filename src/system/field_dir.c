#include "types.h"
#include "constants/directions.h"
#include "field/field_rail.h"
#include "field/zone.h"

// The rail's direction keys for the directions of the actors.
// The file's name is a guess, from what it does: the ROM has no string for it. The function's name is swan's

static const u8 DIR_TO_RAIL_DIR[] = {
    [DIR_UP] = RAIL_DIR_UP,
    [DIR_DOWN] = RAIL_DIR_DOWN,
    [DIR_LEFT] = RAIL_DIR_LEFT,
    [DIR_RIGHT] = RAIL_DIR_RIGHT,
};

u32 ConvDirToRailDir(u32 direction) {
    return DIR_TO_RAIL_DIR[direction];
}
