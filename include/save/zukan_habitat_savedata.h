#ifndef POKEBW2_SAVE_ZUKAN_HABITAT_SAVEDATA_H
#define POKEBW2_SAVE_ZUKAN_HABITAT_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// The Pokédex's habitat list (zukan_habitat_savedata.c, a descriptive name), which the save keeps at the end of the
// medal box's block. Its clearing, its getter and the functions that the Pokédex calls are declared in
// save/medal_box.h

#define ZUKAN_HABITAT_SAVE_COUNT 70

// The kinds of habitat that a habitat's progress is kept for
enum {
    ZUKAN_HABITAT_TYPE_0,
    ZUKAN_HABITAT_TYPE_1,
    ZUKAN_HABITAT_TYPE_2,
};

// What the player has done of a habitat: a value for each kind of habitat, and a flag
typedef struct {
    u8 type0 : 2;
    u8 type1 : 2;
    u8 type2 : 2;
    u8 flag : 1;
} ZukanHabitatProgress;

struct ZukanHabitatSave {
    u8 unk0[0x4a];
    ZukanHabitatProgress habitats[ZUKAN_HABITAT_SAVE_COUNT];
    // What the list shows: the zone, the season and the kind of habitat
    u16 zone;
    u8 season;
    u8 type;
    // Whether the messages of every Pokémon's habitat seen and caught have been shown
    u8 seenMessageShown;
    u8 caughtMessageShown;
}; // size: 0x96

// A habitat's flag, and setting it
BOOL ZukanHabitatSave_IsHabitatFlagSet(ZukanHabitatSave *save, u8 habitat);
void ZukanHabitatSave_SetHabitatFlag(ZukanHabitatSave *save, u8 habitat);
// Sets what the player has done of a habitat's kind of habitat, ZUKAN_HABITAT_TYPE_*
void ZukanHabitatSave_SetHabitatField(ZukanHabitatSave *save, u8 habitat, u32 type, u32 value);
// The two flags at the end of the list, and setting them
u8 ZukanHabitatSave_IsSeenMessageShown(ZukanHabitatSave *save);
u8 ZukanHabitatSave_IsCaughtMessageShown(ZukanHabitatSave *save);
void ZukanHabitatSave_SetSeenMessageShown(ZukanHabitatSave *save);
void ZukanHabitatSave_SetCaughtMessageShown(ZukanHabitatSave *save);

#endif // POKEBW2_SAVE_ZUKAN_HABITAT_SAVEDATA_H
