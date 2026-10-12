#ifndef POKEBW2_SAVE_KEY_DATA_SAVEDATA_H
#define POKEBW2_SAVE_KEY_DATA_SAVEDATA_H

#include "types.h"
#include "save/key_info.h"
#include "struct_decls.h"

// Block 0x45, which swan calls the key data: the run through the Black Tower or the White Treehollow, and the key
// information (key_data_savedata.c, a descriptive name). The accessors are in save/save_control.h.

#define KEY_DATA_AREA_COUNT 10
#define KEY_DATA_TRAINER_FLAG_WORDS 5

struct KeyDataSave {
    // The seed the floors are generated from
    u32 seed;
    // A bit for each Trainer of each of five floors, set when defeated
    u32 trainerFlags[KEY_DATA_TRAINER_FLAG_WORDS];
    u32 flags;
    u8 unk1C;
    // The floors climbed, counted through all areas, and the room of the floor
    u8 floor;
    u8 room;
    // The area reached of the other kind and of kind 0
    u8 progressOther;
    u8 progress;
    // The player's money when the run began
    u32 money;
    KeyInfoSave keyInfo;
}; // size: 0x60

// The block's size and its initial state
u32 func_02010230(void);
void func_02010234(KeyDataSave *keyData);
// The key information in the block
KeyInfoSave *func_020103f0(KeyDataSave *keyData);

#endif // POKEBW2_SAVE_KEY_DATA_SAVEDATA_H
