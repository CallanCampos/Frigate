#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// Entry points in later, still unrecovered files of the DWC settings utility.
void func_ov182_021ab67c(const char *path, void (*upload)(const void *, u32, u32));
void *func_ov182_021ae118(void *work, void *param, int arg1, int arg2);
// The text flags of the utility's language
u32 func_ov182_021ab7b4(void);
// The utility's language
u8 func_ov182_021ae018(void);

extern void *data_ov182_021c40b8;

void func_ov182_021bcc9c(GXOamAttr *oam);

// The utility's object and task helpers, in the same later files. A task's
// callback is handed the task and the argument it was created with.
typedef void (*DWCUtilTaskCallback)(DWCUtilTask *task, void *arg);

DWCUtilTask *func_ov182_021bd200(int list, DWCUtilTaskCallback callback, void *arg, u8 priority);
void func_ov182_021bd264(DWCUtilTask *task, DWCUtilTaskCallback callback);
void func_ov182_021bd268(int list, DWCUtilTask *task);
void func_ov182_021bd280(int list, DWCUtilTask *task);
// Runs the list's tasks: list 1 is the VBlank's
void func_ov182_021bd19c(int list);

// The list and the OAM buffer under the OAM allocator and the cells, in the same later files
void *func_ov182_021bcb3c(void);
void func_ov182_021bcb58(void *list);
void func_ov182_021bcb6c(void *node);
void func_ov182_021bcb90(void *position, void *node);
void func_ov182_021bcbb0(void *list, void *node);
void func_ov182_021bcbbc(void *list, void *node);
GXOamAttr *func_ov182_021bcc80(int screen);
GXOamAttr *func_ov182_021bccd8(int screen, int index);

// The OBJ characters under the text canvases, in the same later files
void *func_ov182_021bcdbc(int screen, int size, s16 fromFront, u32 *charName);
void func_ov182_021bcec4(void *block);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
