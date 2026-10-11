#include "types.h"
#include "save/records.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/save_control.h"

// The save block of the records, which the trainer card shows (the ROM has no string for the file, so the name is a
// guess). Block 0x26.

#define SAVE_BLOCK_TRAINER_CARD 0x26

u32 func_020093d0(void) {
    return sizeof(GameRecords);
}

void initTrainerCardInfoBlk(GameRecords *records) {
    sys_memset32(0, records, sizeof(GameRecords));
    records->seedHigh = OS_GetVBlankCount() | (OS_GetVBlankCount() << 8);
    EncryptRecordStorage(records, 1);
}

GameRecords *getTrainerCardInfoBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_CARD);
}
