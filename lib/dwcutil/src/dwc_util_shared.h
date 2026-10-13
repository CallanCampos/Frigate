#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"

// Entry points in later, still unrecovered files of the DWC settings utility.
void func_ov182_021ab67c(const char *path, void (*upload)(const void *, u32, u32));
void *func_ov182_021bbdec(int kind, int flags);
void *func_ov182_021ae118(void *work, void *param, int arg1, int arg2);
void *func_ov182_021ab7b4(void);
void func_ov182_021bc0a4(void *object, int kind, int flags, int priority, int size, int count, void *resource,
                         void *work);
void func_ov182_021bc184(void *object);
void func_ov182_021bbf7c(int kind);

extern void *data_ov182_021c40b8;

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
