#ifndef POKEBW2_DWCUTIL_DEQUE_H
#define POKEBW2_DWCUTIL_DEQUE_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's stacks of pointers, which hand out a pool's items; the VBlank interrupt is held off around
// each push and pop. The file is the DWC utility library's deque.o (libdwcutil.TWL.HYB.a), as aletheia's map of
// Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it.

struct DWCUtilStack {
    u16 capacity;
    u8 head;
    u8 tail;
    void *items[1];
};

// An empty stack for count items, and one filled with the count items that start at first, stride bytes apart
DWCUtilStack *DWCi_QUElForm(int count);
DWCUtilStack *DWCi_QUElFormEx(int count, u8 *first, int stride);
void DWCi_QUElDelete(DWCUtilStack *stack);
void DWCi_QUElPushBack(DWCUtilStack *stack, void *item);
// The last item pushed, or NULL
void *DWCi_QUElPopBack(DWCUtilStack *stack);

#endif // POKEBW2_DWCUTIL_DEQUE_H
