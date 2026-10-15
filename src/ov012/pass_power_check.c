// Whether the pass powers are unlocked by the Entralink levels and the special pass powers' flags. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the file's name is descriptive
#include "types.h"
#include "save/high_link.h"

#define PASS_POWER_COUNT 64
// The loop over the pass powers tests its constant bound before the first pass, which MWCC only keeps for a counter
// of an enum type, so the pass powers were numbered by an enum; high_link.h has none yet
typedef enum {
    PASS_POWER_ID_FIRST,
    PASS_POWER_ID_COUNT = PASS_POWER_COUNT,
} PassPowerId;
#define PASS_POWER_UNLOCKED 0
#define PASS_POWER_NEAR 1
#define PASS_POWER_LOCKED 2
// The levels a pass power needs, or one of these
#define PASS_POWER_LEVEL_NONE 0xff
#define PASS_POWER_SPECIAL 0xfe

u32 CheckPassPowerUnlocked(void *data, int index, void *levels, u8 *flags) {
    PassPowerData *table = data;
    PassPowerLevel *have = levels;
    int level1 = table[index].level1;
    u32 bit;
    u32 unlocked;

    if (level1 == PASS_POWER_LEVEL_NONE) {
        return PASS_POWER_LOCKED;
    }
    if (level1 == PASS_POWER_SPECIAL) {
        bit = PassPower_GetSPowerBaseID(index);
        if (bit == 0xff) {
            return PASS_POWER_LOCKED;
        }
        unlocked = flags[0] | (flags[1] << 8);
        if (unlocked & (1 << bit)) {
            return PASS_POWER_UNLOCKED;
        }
        return PASS_POWER_LOCKED;
    }
    if (level1 <= have->level1) {
        if (table[index].level2 <= have->level2) {
            return PASS_POWER_UNLOCKED;
        }
        if (table[index].level2 - have->level2 <= 3) {
            return PASS_POWER_NEAR;
        }
    } else if (table[index].level2 <= have->level2) {
        if (level1 <= have->level1) {
            return PASS_POWER_UNLOCKED;
        }
        if (level1 - have->level1 <= 3) {
            return PASS_POWER_NEAR;
        }
    }
    return PASS_POWER_LOCKED;
}

u32 GetUnlockedPassPowerCount(void *data, void *levels, u8 *flags) {
    PassPowerId i;
    u32 count = 0;

    for (i = PASS_POWER_ID_FIRST; i < PASS_POWER_ID_COUNT; i++) {
        if (CheckPassPowerUnlocked(data, i, levels, flags) == PASS_POWER_UNLOCKED) {
            count++;
        }
    }
    return count;
}
