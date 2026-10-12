#include "types.h"
#include "save/records.h"
#include "gfl/std.h"

// The results of the random matches and the competitions, which the record block (records.h) holds at 0x5a: three
// counts for each of 11 kinds of match. The ROM has no string for the file, so the name is a guess.

#define RNDMATCH_TYPE_MAX 11
#define RNDMATCH_FIELD_MAX 3
#define RNDMATCH_VALUE_MAX 0xffff

typedef struct {
    u16 values[RNDMATCH_TYPE_MAX][RNDMATCH_FIELD_MAX];
} RndMatchData;

void RndMatchData_Init(void *results) {
    sys_memset(results, 0, sizeof(RndMatchData));
}

u16 RndMatchData_GetParam(void *results, u32 index, u32 field) {
    RndMatchData *data = results;

    return data->values[index][field];
}

void RndMatchData_SetParam(void *results, u32 index, u32 field, s32 value) {
    RndMatchData *data = results;

    if (value > RNDMATCH_VALUE_MAX) {
        value = RNDMATCH_VALUE_MAX;
    } else if (value < 0) {
        value = 0;
    }
    data->values[index][field] = value;
}

void RndMatchData_IncParam(void *results, u32 index, u32 field) {
    RndMatchData *data = results;

    if (data->values[index][field] < RNDMATCH_VALUE_MAX) {
        data->values[index][field]++;
    }
}
