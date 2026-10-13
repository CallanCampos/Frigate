#include "app/key_system_flow.h"

// Where the unlock effect plays, and the frames of the plate's moves down and up and of a setting's hide
// (key_system_flow.c has the frames as constants: nothing reads the table). A data-only unit between
// key_system_net.c and key_system_flow.c, laid out by size as MWCC does. The ROM doesn't name it; the name is ours.
const ClActorPos sEffectPos = { 184, 114 };
const u32 data_ov332_021c8e78[3] = { 40, 70, 40 };
