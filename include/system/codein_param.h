#ifndef POKEBW2_SYSTEM_CODEIN_PARAM_H
#define POKEBW2_SYSTEM_CODEIN_PARAM_H

#include "types.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The parameters of the code input screen (codein_param.c), where the player types a number such as a Friend Code.
// Our names

// The groups the digits are shown in, the last one repeated after them
#define CODEIN_BLOCK_COUNT 3

// What is asked for, which is also the message shown (text bank 28)
enum {
    CODEIN_MODE_FRIEND_CODE,
    CODEIN_MODE_TRAIN,
    CODEIN_MODE_RANK,
};

struct CodeInParam {
    // CODEIN_MODE_*
    u32 mode;
    // How many digits the code has
    u32 length;
    int blocks[CODEIN_BLOCK_COUNT + 1];
    u32 unk18;
    // The largest number accepted, in the modes other than the Friend Code's
    int max;
    // The code typed, length characters
    StrBuf *str;
};

// blocks holds the number of digits of each group
CodeInParam *CodeInParam_Create(u32 heapId, u32 mode, u32 max, u32 length, const int *blocks);
void CodeInParam_Free(CodeInParam *param);

#endif // POKEBW2_SYSTEM_CODEIN_PARAM_H
