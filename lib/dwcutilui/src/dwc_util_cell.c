#include "dwc_util_cell.h"
#include "archive.h"
#include "cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// A cell of the table: its count of OBJ attribute lists, the first three halfwords of an OAM entry each, at the offset
// from the table's start
typedef struct DWCUtilCell {
    u16 count;
    u32 offset;
} DWCUtilCell;

static DWCUtilCell *sCells[2];

void DWCUtilCell_Load(int screen, const char *path) {
    u32 size;

    sCells[screen] = DWCUtilRomResource_LoadFile(path, &size, 4);
}

void DWCUtilCell_Unload(int screen) {
    DWCi_ARClRelease(sCells[screen]);
    sCells[screen] = NULL;
}

void DWCUtilCell_Copy(int screen, int cell, GXOamAttr *oam) {
    DWCUtilCell *cells = sCells[screen];
    const u16 *attrs = (const u16 *)((const u8 *)cells + cells[cell].offset);
    u16 count = cells[cell].count;
    GXOamAttr attr;
    int i;

    MI_CpuClear32(&attr, sizeof(attr));
    for (i = 0; i < count; i++, attrs += 3, oam++) {
        MI_CpuCopy16(attrs, &attr, 6);
        MIi_CpuCopy32(&attr, oam, sizeof(attr));
    }
}

GXOamAttr *DWCUtilCell_AllocOam(int screen, int cell) {
    GXOamAttr *oam = DWCi_OBJlFormGx(screen);

    DWCUtilCell_Copy(screen, cell, oam);
    return oam;
}

DWCUtilOamNode *DWCUtilCell_AllocOamRun(int screen, int cell, BOOL fromFront) {
    DWCUtilOamNode *node = DWCi_CELLlForm(screen, sCells[screen][cell].count, fromFront);

    DWCUtilCell_Copy(screen, cell, DWCi_CELLlGetObj(node, 0));
    return node;
}

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

void *DWCi_QUElPopFront(DWCUtilStack *stack) {
    void *item = NULL;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    if (stack->head != stack->tail) {
        stack->tail = FX_ModS32(stack->tail + stack->capacity - 1, stack->capacity);
        item = stack->items[stack->tail];
    }
    OS_EnableIrqMask(irq);
    return item;
}
