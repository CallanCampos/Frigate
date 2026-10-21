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

void DWCUtilOam_Init(void);
void DWCUtilOam_Exit(void);
// A run of count entries, the first free one from the front if fromFront, else from the back
DWCUtilOamNode *DWCUtilOam_Alloc(int screen, int count, BOOL fromFront);
// Hides the run's entries and frees it
void DWCUtilOam_Free(DWCUtilOamNode *node);
GXOamAttr *DWCUtilOam_GetEntry(DWCUtilOamNode *node, int index);
int DWCUtilOam_GetCount(DWCUtilOamNode *node);
// The functions that change the run's entries change the one at index, or every entry if index is negative
void DWCUtilOam_SetEffect(DWCUtilOamNode *node, int index, int effect, int rsParam);
void DWCUtilOam_SetMode(DWCUtilOamNode *node, int index, int mode, int cParam);
// Moves the entry, or the whole run with its first entry at (x, y)
void DWCUtilOam_SetNodePosition(DWCUtilOamNode *node, int index, int x, int y);
void DWCUtilOam_SetPriority(DWCUtilOamNode *node, int index, int priority);
void DWCUtilOam_GetNodePosition(DWCUtilOamNode *node, int index, int *x, int *y);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OAM_H
