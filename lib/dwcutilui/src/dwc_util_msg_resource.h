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

void DWCi_MSGlInit(void);
void DWCi_MSGlEnd(void);
DWCUtilMsg *DWCi_MSGlRead(const char *path);
void DWCi_MSGlRelease(DWCUtilMsg *msg);
// The message's 16-bit characters
u16 *DWCi_MSGlGet(DWCUtilMsg *msg, int index);
// The message, with the digit value written at its character slot if slot isn't negative
u16 *DWCi_MSGlGetEx(DWCUtilMsg *msg, int index, int slot, int value);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MSG_RESOURCE_H
