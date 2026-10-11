#ifndef POKEBW2_SAVE_RECORDS_H
#define POKEBW2_SAVE_RECORDS_H

#include "types.h"
#include "struct_decls.h"

// RecordSave is the block that getRecordBlkAddress returns. The encrypted records that GameData_GetRecords returns
// are GameRecords: the counters of the first 0x47 records are words, and those of the rest halfwords. All but the
// first are encrypted, with the sum of them as the seed

#define RECORD_COUNT_WORD 0x47
#define RECORD_COUNT 0xa7

struct GameRecords {
    u32 words[RECORD_COUNT_WORD];
    u16 halfwords[RECORD_COUNT - RECORD_COUNT_WORD];
    u16 checksum;
    u16 seedHigh;
};

// The size of the records
u32 func_020093d0(void);
void initTrainerCardInfoBlk(GameRecords *records);
void EncryptRecordStorage(GameRecords *records, u32 id);
void DecryptRecordStorage(GameRecords *records, u32 id);
u32 GetRecordCounter(GameRecords *records, int id);
u32 SetRecordCounter(GameRecords *records, int id, u32 value);
u32 GetRecordCounterMax(u32 id);
// Sets a record to value, or to its maximum
u32 func_020094cc(GameRecords *records, u32 id, u32 value);

// Clears the flag that is set, with the console's MAC address and the time, while a match is in progress
void RecordSave_ClearMatchInProgress(RecordSave *record);

u32 RecordAddOne(GameRecords *records, u32 id);
u32 RecordGet(GameRecords *records, u32 id);
u32 RecordAdd(GameRecords *records, u32 id, u32 value);
// Sets a record to value if that is higher, up to the record's maximum
u32 func_02009508(GameRecords *records, u32 id, u32 value);
// The Trial House's best rank and best points
void func_02009618(GameRecords *records, u8 rank);
// The Battle Test rank
u8 func_02009628(GameRecords *records);
// Record 0x7c
u32 func_02009650(GameRecords *records);
void func_020095e0(GameRecords *records);
void func_02009638(GameRecords *records, u32 points);
RecordSave *func_0200f2bc(SaveControl *save);
// The results of the random matches
void *func_0200f2d4(RecordSave *record);
void func_0200f2dc(RecordSave *record);
u8 func_0200f300(RecordSave *record);
u32 func_0200f308(RecordSave *record);
u32 func_0200f334(RecordSave *record);
void func_0200f37c(RecordSave *record, u32 value);
u8 func_0200f384(RecordSave *record);
// The parts of the record that the Wi-Fi competition's card reads (6-byte results with halfword fields), and the
// live competition's (the record itself, with a byte after each result's 6 bytes)
void *func_0200f2d0(RecordSave *record);
void *func_0200f2d8(RecordSave *record);
u32 func_0200f1d8(void *results, u32 index);
u16 func_0200e870(void *results, u32 index, u32 field);

#endif // POKEBW2_SAVE_RECORDS_H
