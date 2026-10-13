#include "app/key_system_flow.h"

// The frames of the key's moves in and out of view and of a setting's reveal (key_system_flow.c has them as
// constants: nothing reads this table). A data-only object between key_system_net.c and key_system_flow.c; the
// ROM's layout needs it in a unit of its own, since MWCC sorts a unit's objects by size and the 4-byte sEffectPos
// follows it. The ROM doesn't name the unit; the name is ours.
const u32 data_ov332_021c8e68[3] = { 150, 80, 150 };
