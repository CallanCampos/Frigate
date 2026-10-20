#include "dwc_util_vram_alloc.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_list.h"
#include "nitro/os.h"

// Each screen has 32 nodes, a list of the blocks in use between a head and a tail node that bracket the screen's
// range, and a stack of the free nodes
typedef struct DWCUtilVramScreen {
    DWCUtilVramNode nodes[32];
    DWCUtilVramNode head;
    DWCUtilVramNode tail;
    DWCUtilList *list;
    DWCUtilStack *freeNodes;
} DWCUtilVramScreen;

static DWCUtilVramScreen *sVramScreens;

void DWCUtilVramAlloc_Init(void) {
    int i;

    sVramScreens = DWCUtilHeap_AllocClear(sizeof(DWCUtilVramScreen) * 2, 4);
    for (i = 0; i < 2; i++) {
        sVramScreens[i].freeNodes = DWCUtilStack_CreateFilled(32, (u8 *)sVramScreens[i].nodes, sizeof(DWCUtilVramNode));
        sVramScreens[i].list = DWCUtilList_Create();
        sVramScreens[i].head.pos = 0x300;
        sVramScreens[i].tail.pos = 0x400;
        DWCUtilList_Prepend(sVramScreens[i].list, &sVramScreens[i].head);
        DWCUtilList_Append(sVramScreens[i].list, &sVramScreens[i].tail);
    }
}

DWCUtilVramNode *DWCUtilVramAlloc_Alloc(int screen, int size, s16 fromFront, u32 *pos) {
    DWCUtilVramNode *node = DWCUtilStack_Pop(sVramScreens[screen].freeNodes);
    DWCUtilVramNode *position;
    int units = (size + 3 & ~3) >> 2;
    u32 irq;

    node->size = units;
    irq = OS_DisableIrqMask(OS_IE_V_BLANK);
    if (fromFront) {
        position = &sVramScreens[screen].head;
        while (position != &sVramScreens[screen].tail) {
            DWCUtilVramNode *next = position->next;
            int end = position->pos + position->size;

            if (end + units <= next->pos) {
                node->pos = end;
                DWCUtilList_InsertBefore(next, node);
                break;
            }
            position = next;
        }
        if (position == &sVramScreens[screen].tail) {
            OS_Terminate();
        }
    } else {
        position = &sVramScreens[screen].tail;
        while (position != &sVramScreens[screen].head) {
            DWCUtilVramNode *prev = position->prev;
            int start = position->pos - units;

            if (start >= prev->pos + prev->size) {
                node->pos = start;
                DWCUtilList_InsertBefore(position, node);
                break;
            }
            position = prev;
        }
        if (position == &sVramScreens[screen].head) {
            OS_Terminate();
        }
    }
    *pos = node->pos;
    OS_EnableIrqMask(irq);
    return node;
}

void DWCUtilVramAlloc_Free(DWCUtilVramNode *node) {
    int screen = 0;

    DWCUtilList_Remove(node);
    if (node >= sVramScreens[1].nodes) {
        screen = 1;
    }
    DWCUtilStack_Push(sVramScreens[screen].freeNodes, node);
}
