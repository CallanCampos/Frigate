#include "dwc_util_heap.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/fnd.h"

#define DWC_UTIL_HEAP_SIZE 0x40000

static NNSFndHeapHandle sHeap;

void func_ov182_021bc468(void *memory) {
    MI_CpuFill8(memory, 0, DWC_UTIL_HEAP_SIZE);
    sHeap = NNS_FndCreateExpHeapEx(memory, DWC_UTIL_HEAP_SIZE, 0);
    if (sHeap == NULL) {
        OS_Terminate();
    }
}

void func_ov182_021bc494(void) {
    NNS_FndDestroyExpHeap(sHeap);
    sHeap = NULL;
}

void *func_ov182_021bc4a8(int size, int align) {
    void *block;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    block = NNS_FndAllocFromExpHeapEx(sHeap, size, align);
    if (block == NULL) {
        OS_Terminate();
    }
    OS_EnableIrqMask(irq);
    return block;
}

void *func_ov182_021bc4d8(int size, int align) {
    void *block = func_ov182_021bc4a8(size, align);

    MI_CpuFill8(block, 0, size);
    return block;
}

void func_ov182_021bc4f0(void *pointer) {
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    // BUG: Without a block to free, this leaves the VBlank interrupt disabled
#ifdef BUGFIX
    if (*(void **)pointer == NULL) {
        OS_EnableIrqMask(irq);
        return;
    }
#endif
    if (*(void **)pointer != NULL) {
        NNS_FndFreeToExpHeap(sHeap, *(void **)pointer);
        OS_EnableIrqMask(irq);
        *(void **)pointer = NULL;
    }
}

void func_ov182_021bc51c(void *block) {
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    // BUG: As in func_ov182_021bc4f0, a NULL block leaves the VBlank interrupt disabled
#ifdef BUGFIX
    if (block == NULL) {
        OS_EnableIrqMask(irq);
        return;
    }
#endif
    if (block != NULL) {
        NNS_FndFreeToExpHeap(sHeap, block);
        OS_EnableIrqMask(irq);
    }
}
