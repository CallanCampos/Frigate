#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "nitro/gx.h"
#include "struct_decls.h"

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

void func_ov182_021bcc9c(GXOamAttr *oam);

// The utility's object and task helpers, in the same later files. A task's
// callback is handed the task and the argument it was created with.
typedef void (*DWCUtilTaskCallback)(DWCUtilTask *task, void *arg);

BOOL func_ov182_021bc888(const void *rect);
DWCUtilTask *func_ov182_021bd200(int list, DWCUtilTaskCallback callback, void *arg, u8 priority);
void func_ov182_021bd264(DWCUtilTask *task, DWCUtilTaskCallback callback);
void func_ov182_021bd268(int list, DWCUtilTask *task);
void func_ov182_021bd280(int list, DWCUtilTask *task);

// The list and the OAM buffer under the OAM allocator and the cells, in the same later files
void *func_ov182_021bcb3c(void);
void func_ov182_021bcb58(void *list);
void func_ov182_021bcb6c(void *node);
void func_ov182_021bcb90(void *position, void *node);
void func_ov182_021bcbb0(void *list, void *node);
void func_ov182_021bcbbc(void *list, void *node);
GXOamAttr *func_ov182_021bcc80(int screen);
GXOamAttr *func_ov182_021bccd8(int screen, int index);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
