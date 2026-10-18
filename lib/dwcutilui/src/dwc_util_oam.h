#ifndef POKEBW2_DWCUTIL_DWC_UTIL_OAM_H
#define POKEBW2_DWCUTIL_DWC_UTIL_OAM_H

#include "types.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// The connection utility's OAM allocator: runs of OAM entries on each screen (0 for the main engine's, 1 for the
// sub engine's), handed out from the front of the screen's range or from its back.
// The ROM does not embed this source file's name; dwc_util_oam.c is descriptive.

// A run of OAM entries on its screen's list, which is kept in address order
struct DWCUtilOamNode {
    DWCUtilOamNode *prev;
    DWCUtilOamNode *next;
    GXOamAttr *oam;
    u8 count;
};

void func_ov182_021bb480(void);
void func_ov182_021bb54c(void);
// A run of count entries, the first free one from the front if fromFront, else from the back
DWCUtilOamNode *func_ov182_021bb594(int screen, int count, BOOL fromFront);
// Hides the run's entries and frees it
void func_ov182_021bb690(DWCUtilOamNode *node);
GXOamAttr *func_ov182_021bb6ec(DWCUtilOamNode *node, int index);
int func_ov182_021bb6f4(DWCUtilOamNode *node);
// The functions that change the run's entries change the one at index, or every entry if index is negative
void func_ov182_021bb6f8(DWCUtilOamNode *node, int index, int effect, int rsParam);
void func_ov182_021bb778(DWCUtilOamNode *node, int index, int mode, int cParam);
// Moves the entry, or the whole run with its first entry at (x, y)
void func_ov182_021bb7e0(DWCUtilOamNode *node, int index, int x, int y);
void func_ov182_021bb878(DWCUtilOamNode *node, int index, int priority);
void func_ov182_021bb8bc(DWCUtilOamNode *node, int index, int *x, int *y);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OAM_H
