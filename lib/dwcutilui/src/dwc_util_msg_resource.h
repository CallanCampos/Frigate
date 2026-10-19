#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MSG_RESOURCE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MSG_RESOURCE_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's message files: each is read whole from the resource archive, and its messages are found
// through the table of offsets in its INF1 block into its DAT1 block. Up to 8 are open at a time.
// The ROM does not embed this source file's name; dwc_util_msg_resource.c is descriptive.

struct DWCUtilMsg {
    const u32 *offsets;
    u8 *text;
    void *file;
};

void func_ov182_021ae084(void);
void func_ov182_021ae0a8(void);
DWCUtilMsg *func_ov182_021ae0b8(const char *path);
void func_ov182_021ae0ec(DWCUtilMsg *msg);
// The message's 16-bit characters
u16 *func_ov182_021ae108(DWCUtilMsg *msg, int index);
// The message, with the digit value written at its character slot if slot isn't negative
u16 *func_ov182_021ae118(DWCUtilMsg *msg, int index, int slot, int value);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MSG_RESOURCE_H
