#ifndef POKEBW2_DWCUTIL_CELL_H
#define POKEBW2_DWCUTIL_CELL_H

#include "types.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// The connection utility's OAM allocator: runs of OAM entries on each screen (0 for the main engine's, 1 for the
// sub engine's), handed out from the front of the screen's range or from its back.
// The file is the DWC utility library's cell.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.

// A run of OAM entries on its screen's list, which is kept in address order
struct DWCUtilOamNode {
    DWCUtilOamNode *prev;
    DWCUtilOamNode *next;
    GXOamAttr *oam;
    u8 count;
};

void DWCi_CELLlInit(void);
void DWCi_CELLlEnd(void);
// A run of count entries, the first free one from the front if fromFront, else from the back
DWCUtilOamNode *DWCi_CELLlForm(int screen, int count, BOOL fromFront);
// Hides the run's entries and frees it
void DWCi_CELLlDelete(DWCUtilOamNode *node);
GXOamAttr *DWCi_CELLlGetObj(DWCUtilOamNode *node, int index);
int DWCi_CELLlGetObjNum(DWCUtilOamNode *node);
// The functions that change the run's entries change the one at index, or every entry if index is negative
void DWCi_CELLlSetEffect(DWCUtilOamNode *node, int index, int effect, int rsParam);
void DWCi_CELLlSetPalette(DWCUtilOamNode *node, int index, int mode, int cParam);
// Moves the entry, or the whole run with its first entry at (x, y)
void DWCi_CELLlSetPosition(DWCUtilOamNode *node, int index, int x, int y);
void DWCi_CELLlSetPriority(DWCUtilOamNode *node, int index, int priority);
void DWCi_CELLlGetPosition(DWCUtilOamNode *node, int index, int *x, int *y);

#endif // POKEBW2_DWCUTIL_CELL_H
