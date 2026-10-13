#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
#include "nitro/gx.h"

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

void *func_ov182_021bc4d8(int size, int align);
GXOamAttr *func_ov182_021bb960(int screen, int resource);
void *func_ov182_021bd200(int priority, void (*callback)(void), int arg, int period);
void func_ov182_021bd280(int priority, void *task);
void func_ov182_021bcc9c(GXOamAttr *oam);
void func_ov182_021bc4f0(void *work);
void func_ov182_021bb914(int screen, int resource, GXOamAttr *oam);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
