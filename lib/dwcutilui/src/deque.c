#include "deque.h"
#include "heap.h"
#include "nitro/fx.h"
#include "nitro/os.h"

// The stack has a slot more than its items: pushing into the last one stops the game
DWCUtilStack *DWCi_QUElForm(int count) {
    DWCUtilStack *stack = DWCi_HEAPlAllocEx((count + 1) * sizeof(void *) + 8, 4);

    stack->capacity = count + 1;
    stack->head = 0;
    stack->tail = 0;
    return stack;
}

DWCUtilStack *DWCi_QUElFormEx(int count, u8 *first, int stride) {
    DWCUtilStack *stack = DWCi_QUElForm(count);
    int i;

    for (i = 0; i < count; i++) {
        stack->items[i] = first;
        first += stride;
    }
    stack->tail = count;
    return stack;
}

void DWCi_QUElDelete(DWCUtilStack *stack) {
    DWCi_HEAPlFree_(&stack);
}

void DWCi_QUElPushBack(DWCUtilStack *stack, void *item) {
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);
    int next = FX_ModS32(stack->tail + 1, stack->capacity);

    if (next == stack->head) {
        OS_Terminate();
    }
    stack->items[stack->tail] = item;
    stack->tail = next;
    OS_EnableIrqMask(irq);
}

void *DWCi_QUElPopBack(DWCUtilStack *stack) {
    void *item = NULL;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    if (stack->head != stack->tail) {
        stack->tail = FX_ModS32(stack->tail + stack->capacity - 1, stack->capacity);
        item = stack->items[stack->tail];
    }
    OS_EnableIrqMask(irq);
    return item;
}
