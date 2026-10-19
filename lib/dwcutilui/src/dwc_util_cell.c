#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "dwc_util_rom_resource.h"
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

void func_ov182_021bb8dc(int screen, const char *path) {
    u32 size;

    sCells[screen] = func_ov182_021bb37c(path, &size, 4);
}

void func_ov182_021bb8fc(int screen) {
    func_ov182_021bb420(sCells[screen]);
    sCells[screen] = NULL;
}

void func_ov182_021bb914(int screen, int cell, GXOamAttr *oam) {
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

GXOamAttr *func_ov182_021bb960(int screen, int cell) {
    GXOamAttr *oam = func_ov182_021bcc80(screen);

    func_ov182_021bb914(screen, cell, oam);
    return oam;
}

DWCUtilOamNode *func_ov182_021bb97c(int screen, int cell, BOOL fromFront) {
    DWCUtilOamNode *node = func_ov182_021bb594(screen, sCells[screen][cell].count, fromFront);

    func_ov182_021bb914(screen, cell, func_ov182_021bb6ec(node, 0));
    return node;
}

// The stack has a slot more than its items: pushing into the last one stops the game
DWCUtilStack *func_ov182_021bb9ac(int count) {
    DWCUtilStack *stack = func_ov182_021bc4d8((count + 1) * sizeof(void *) + 8, 4);

    stack->capacity = count + 1;
    stack->head = 0;
    stack->tail = 0;
    return stack;
}

DWCUtilStack *func_ov182_021bb9c8(int count, u8 *first, int stride) {
    DWCUtilStack *stack = func_ov182_021bb9ac(count);
    int i;

    for (i = 0; i < count; i++) {
        stack->items[i] = first;
        first += stride;
    }
    stack->tail = count;
    return stack;
}

void func_ov182_021bb9ec(DWCUtilStack *stack) {
    func_ov182_021bc4f0(&stack);
}

void func_ov182_021bba00(DWCUtilStack *stack, void *item) {
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);
    int next = FX_ModS32(stack->tail + 1, stack->capacity);

    if (next == stack->head) {
        OS_Terminate();
    }
    stack->items[stack->tail] = item;
    stack->tail = next;
    OS_EnableIrqMask(irq);
}

void *func_ov182_021bba38(DWCUtilStack *stack) {
    void *item = NULL;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    if (stack->head != stack->tail) {
        stack->tail = FX_ModS32(stack->tail + stack->capacity - 1, stack->capacity);
        item = stack->items[stack->tail];
    }
    OS_EnableIrqMask(irq);
    return item;
}
