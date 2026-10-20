// Rolling what a Hidden Grotto holds, which the field's daily events do for each grotto. The ROM has no name for the
// file; hidden_hollow_generate.c is descriptive. That it is a file of its own, and not the start of
// scrcmd_hidden_hollow.c, is a guess from the data: MWCC sorts a file's data by size, so one file would put the
// mission levels (20 bytes) before this file's slot chances (200 bytes), where the ROM has them the other way round,
// and no call crosses the boundary between the two. generateHollow is swan's name
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/hidden_hollow_generate.h"
#include "gfl/random.h"
#include "save/high_link.h"
#include "save/misc_savedata.h"

#define SLOT_CHANCE_COUNT 10

// By grotto, how likely each of the ten slots is, out of 100
static const u8 HOLLOW_SLOT_CHANCES[HIDDEN_HOLLOW_COUNT][SLOT_CHANCE_COUNT] = {
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
};

void generateHollow(RivalDataSave *data, u8 hollow) {
    u8 slot = 10;
    u8 group = GFL_RandomLC(4);
    u32 volume = PassPower_ApplyGrottoVolume(1);
    u32 i = 0;
    int j;
    const u8 *chances;
    u8 roll;
    u8 sum;

    if (volume != 0) {
        do {
            roll = GFL_RandomLC(100) + 1;
            sum = 0;
            chances = HOLLOW_SLOT_CHANCES[hollow];

            for (j = 0; j < SLOT_CHANCE_COUNT; j++) {
                sum += chances[j];
                if (roll <= sum) {
                    slot = j;
                    break;
                }
            }
            if (slot <= 2) {
                break;
            }
        } while (++i < volume);
    }
    setSlot(data, hollow, slot);
    setGroup(data, hollow, group);
    setHollowGender(data, hollow, (u8)GFL_RandomLC(100));
    setHollowStatus(data, hollow, TRUE);
}
