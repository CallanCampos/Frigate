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

void DWCUtilMsg_Init(void);
void DWCUtilMsg_Exit(void);
DWCUtilMsg *DWCUtilMsg_Open(const char *path);
void DWCUtilMsg_Close(DWCUtilMsg *msg);
// The message's 16-bit characters
u16 *DWCUtilMsg_Get(DWCUtilMsg *msg, int index);
// The message, with the digit value written at its character slot if slot isn't negative
u16 *DWCUtilMsg_GetWithDigit(DWCUtilMsg *msg, int index, int slot, int value);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MSG_RESOURCE_H
