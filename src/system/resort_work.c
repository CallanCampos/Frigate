#include "system/resort_work.h"
#include "types.h"
#include "gfl/heap.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"

// The Join Avenue's values that GameData keeps. Our names

ResortWork *ResortWork_Create(HeapID heapId, SaveControl *save) {
    ResortWork *work = GFL_HeapAllocate(heapId, sizeof(ResortWork), TRUE, "resort_work.c", 50);

    ResortWork_UpdateRecords(work, save);
    return work;
}

void ResortWork_Free(ResortWork *work) {
    GFL_HeapFree(work);
}

void ResortWork_UpdateRecords(ResortWork *work, SaveControl *save) {
    GameRecords *records = getTrainerCardInfoBlkAddress(save);
    TrainerCardSave *trainerCard = getTrainerCardData_wrapper(save);
    u64 sum;

    sum = 0;
    sum += RecordGet(records, 0xc);
    sum += RecordGet(records, 0x10);
    sum += RecordGet(records, 0x24);
    if (sum > RESORT_WORK_RECORD_MAX) {
        sum = RESORT_WORK_RECORD_MAX;
    }
    work->values[0] = sum;
    work->values[1] = RecordGet(records, 0x1e);
    work->values[2] = RecordGet(records, 0x7f) + RecordGet(records, 0x80);
    work->values[3] = RecordGet(records, 0x16);
    work->values[4] = TrainerCard_GetPasserbyCount(trainerCard);

    sum = 0;
    sum += RecordGet(records, 0xd);
    sum += RecordGet(records, 0x11);
    sum += RecordGet(records, 0x25);
    if (sum > RESORT_WORK_RECORD_MAX) {
        sum = RESORT_WORK_RECORD_MAX;
    }
    work->values[5] = sum;
    work->values[6] = RecordGet(records, 7);
    work->values[7] = RecordGet(records, 9);
}

u32 ResortWork_Get(ResortWork *work, u32 index) {
    return work->values[index];
}

void ResortWork_Set(ResortWork *work, u32 index, u32 value) {
    work->values[index] = value;
}
