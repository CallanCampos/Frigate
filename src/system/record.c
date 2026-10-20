#include "types.h"
#include "gfl/std.h"
#include "save/records.h"

// The counters of the records the game keeps (the ROM has no string for the file, so the name is a guess). Their
// maximums are 0, or one of the values of nines, as the byte of the record in sRecordMaxKind says.

static const struct {
    u16 destination;
    u16 source;
} sRecordMoves[] = {
    { 26, 27 }, { 81, 82 }, { 83, 84 }, { 85, 86 }, { 87, 88 },
};

static const u32 nines[] = { 999999999, 999999, 65535, 9999, 999, 6 };

// clang-format off
static u8 sRecordMaxKind[] = {
    0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 4, 3, 3, 5, 3, 3, 3, 3,
    3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 0,
};

// clang-format on

void EncryptRecordStorage(GameRecords *records, u32 id) {
    if (id != 0) {
        records->checksum = addUpDataStream((u8 *)&records->words[1], 0x1d8);
        decryptData(&records->words[1], 0x1d8, records->checksum + (records->seedHigh << 16));
    }
}

void DecryptRecordStorage(GameRecords *records, u32 id) {
    if (id != 0) {
        _decryptData(&records->words[1], 0x1d8, records->checksum + (records->seedHigh << 16));
    }
}

u32 GetRecordCounter(GameRecords *records, int id) {
    if (id < RECORD_COUNT_WORD) {
        return records->words[id];
    }
    if (id < RECORD_COUNT) {
        return records->halfwords[id - RECORD_COUNT_WORD];
    }
    return 0;
}

u32 SetRecordCounter(GameRecords *records, int id, u32 value) {
    if (id < RECORD_COUNT_WORD) {
        records->words[id] = value;
    } else if (id < RECORD_COUNT) {
        records->halfwords[id - RECORD_COUNT_WORD] = value;
    }
    return GetRecordCounter(records, id);
}

u32 GetRecordCounterMax(u32 id) {
    int kind = sRecordMaxKind[id];

    if (kind >= 0xa7) {
        return 0;
    }
    if (kind >= 6) {
        return 0;
    }
    return nines[kind];
}

u32 RecordSet(GameRecords *records, u32 id, u32 value) {
    u32 max = GetRecordCounterMax(id);
    u32 result;

    DecryptRecordStorage(records, id);
    if (value < max) {
        result = SetRecordCounter(records, id, value);
    } else {
        result = SetRecordCounter(records, id, max);
    }
    EncryptRecordStorage(records, id);
    return result;
}

u32 RecordSetIfHigher(GameRecords *records, u32 id, u32 value) {
    u32 max = GetRecordCounterMax(id);
    u32 current;

    DecryptRecordStorage(records, id);
    current = GetRecordCounter(records, id);
    if (value > max) {
        value = max;
    }
    if (current < value) {
        current = SetRecordCounter(records, id, value);
    } else if (current > max) {
        current = SetRecordCounter(records, id, max);
    }
    EncryptRecordStorage(records, id);
    return current;
}

u32 RecordAdd(GameRecords *records, u32 id, u32 value) {
    u32 max = GetRecordCounterMax(id);
    u32 current;
    u32 result;

    DecryptRecordStorage(records, id);
    current = GetRecordCounter(records, id);
    if (current + value < max) {
        result = SetRecordCounter(records, id, current + value);
    } else {
        result = SetRecordCounter(records, id, max);
    }
    EncryptRecordStorage(records, id);
    return result;
}

u32 RecordAddOne(GameRecords *records, u32 id) {
    return RecordAdd(records, id, 1);
}

u32 RecordGet(GameRecords *records, u32 id) {
    u32 max = GetRecordCounterMax(id);
    u32 value;

    DecryptRecordStorage(records, id);
    value = GetRecordCounter(records, id);
    EncryptRecordStorage(records, id);
    if (value > max) {
        return max;
    }
    return value;
}

void func_020095e0(GameRecords *records) {
    u32 i;

    for (i = 0; i < 5; i++) {
        u32 source = sRecordMoves[i].source;

        RecordSetIfHigher(records, sRecordMoves[i].destination, RecordGet(records, source));
        RecordSet(records, source, 0);
    }
}

void RecordSetTrialHouseRank(GameRecords *records, u8 rank) {
    if (rank < 7) {
        RecordSetIfHigher(records, 0x7b, rank);
    }
}

u8 RecordGetTrialHouseRank(GameRecords *records) {
    return RecordGet(records, 0x7b);
}

void RecordSetTrialHousePoints(GameRecords *records, u32 points) {
    if (points < 9999) {
        RecordSetIfHigher(records, 0x7c, points);
    }
}

u32 RecordGetTrialHousePoints(GameRecords *records) {
    return RecordGet(records, 0x7c);
}
