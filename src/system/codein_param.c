#include "system/codein_param.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"

// The parameters of the code input screen. Our names

CodeInParam *CodeInParam_Create(u32 heapId, u32 mode, u32 max, u32 length, const int *blocks) {
    int i;
    CodeInParam *param = GFL_HeapAllocate(heapId, sizeof(CodeInParam), FALSE, "codein_param.c", 55);

    param->mode = mode;
    param->length = length;
    param->str = GFL_StrBufCreate(length + 1, heapId);
    param->max = max;
    for (i = 0; i < CODEIN_BLOCK_COUNT; i++) {
        param->blocks[i] = blocks[i];
    }
    param->blocks[i] = blocks[i - 1];
    return param;
}

void CodeInParam_Free(CodeInParam *param) {
    GFL_StrBufFree(param->str);
    GFL_HeapFree(param);
}
