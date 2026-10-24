#include "types.h"
#include "gfl/std.h"
#include "save/records.h"

// The results of the random matches and the competitions, which the record block (records.h) holds at 0x5a: three
// counts for each of 11 kinds of match. The ROM has no string for the file, so the name is a guess.

void RndMatchData_Init(RndMatchData *results) {
    MI_CpuFill8(results, 0, sizeof(RndMatchData));
}

u16 RndMatchData_GetParam(RndMatchData *data, u32 index, u32 field) {
    return data->values[index][field];
}

void RndMatchData_SetParam(RndMatchData *data, u32 index, u32 field, s32 value) {
    if (value > RNDMATCH_VALUE_MAX) {
        value = RNDMATCH_VALUE_MAX;
    } else if (value < 0) {
        value = 0;
    }
    data->values[index][field] = value;
}

void RndMatchData_IncParam(RndMatchData *data, u32 index, u32 field) {
    if (data->values[index][field] < RNDMATCH_VALUE_MAX) {
        data->values[index][field]++;
    }
}
