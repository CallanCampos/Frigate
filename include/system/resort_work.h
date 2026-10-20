#ifndef POKEBW2_SYSTEM_RESORT_WORK_H
#define POKEBW2_SYSTEM_RESORT_WORK_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Join Avenue's values that GameData keeps (resort_work.c): the first eight are taken from the player's records
// by ResortWork_UpdateRecords. Our names

#define RESORT_WORK_VALUE_COUNT 19
// The records' sums are capped at this
#define RESORT_WORK_RECORD_MAX 999999999

struct ResortWork {
    // 0 and 5: sums of records 0xc, 0x10 and 0x24, and of 0xd, 0x11 and 0x25. 1, 3, 6 and 7: records 0x1e, 0x16, 7
    // and 9. 2: records 0x7f and 0x80 summed. 4: TrainerCard_GetPasserbyCount of the trainer card. 8 is a flag the map
    // change sets
    u32 values[RESORT_WORK_VALUE_COUNT];
};

ResortWork *ResortWork_Create(HeapID heapId, SaveControl *save);
void ResortWork_Free(ResortWork *work);
void ResortWork_UpdateRecords(ResortWork *work, SaveControl *save);
u32 ResortWork_Get(ResortWork *work, u32 index);
void ResortWork_Set(ResortWork *work, u32 index, u32 value);

#endif // POKEBW2_SYSTEM_RESORT_WORK_H
