#ifndef POKEBW2_DWCUTIL_DWC_UTIL_CELL_H
#define POKEBW2_DWCUTIL_DWC_UTIL_CELL_H

#include "types.h"
#include "dwc_util_oam.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// The connection utility's sprite cells: a table per screen, from a file, of the OBJ attribute lists that are copied
// into OAM entries. And its stacks of pointers, which hand out a pool's items; the VBlank interrupt is held off around
// each push and pop.
// The ROM does not embed this source file's name; dwc_util_cell.c is descriptive.

struct DWCUtilStack {
    u16 capacity;
    u8 head;
    u8 tail;
    void *items[1];
};

void DWCUtilCell_Load(int screen, const char *path);
void DWCUtilCell_Unload(int screen);
void DWCUtilCell_Copy(int screen, int cell, GXOamAttr *oam);
// Copies the cell into entries from the screen's OAM buffer, or into a run of entries
GXOamAttr *DWCUtilCell_AllocOam(int screen, int cell);
DWCUtilOamNode *DWCUtilCell_AllocOamRun(int screen, int cell, BOOL fromFront);

// An empty stack for count items, and one filled with the count items that start at first, stride bytes apart
DWCUtilStack *DWCUtilStack_Create(int count);
DWCUtilStack *DWCUtilStack_CreateFilled(int count, u8 *first, int stride);
void DWCUtilStack_Destroy(DWCUtilStack *stack);
void DWCUtilStack_Push(DWCUtilStack *stack, void *item);
// The last item pushed, or NULL
void *DWCUtilStack_Pop(DWCUtilStack *stack);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_CELL_H
