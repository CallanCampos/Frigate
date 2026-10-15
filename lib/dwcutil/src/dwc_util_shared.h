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
void *func_ov182_021bd200(int priority, void (*callback)(void *task), int arg, int period);
void func_ov182_021bd280(int priority, void *task);
void func_ov182_021bcc9c(GXOamAttr *oam);
void func_ov182_021bc4f0(void *work);
void func_ov182_021bb914(int screen, int resource, GXOamAttr *oam);

// The utility's object and task helpers, in the same later files. A task's
// callback is handed the task it was created for.
void *func_ov182_021bb97c(int screen, int resource, int arg);
void func_ov182_021bb690(void *object);
GXOamAttr *func_ov182_021bb6ec(void *object, int index);
void func_ov182_021bb7e0(void *object, int index, int x, int y);
void func_ov182_021bb878(void *object, int index, int value);
void func_ov182_021bb8bc(void *object, int index, int *x, int *y);
void func_ov182_021bc1c0(const void *position, const void *size, void *rect);
BOOL func_ov182_021bc888(const void *rect);
void func_ov182_021bd264(void *task, void (*callback)(void *task));
void func_ov182_021bd268(int priority, void *task);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
