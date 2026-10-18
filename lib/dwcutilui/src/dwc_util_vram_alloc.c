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

void func_ov182_021bccec(void) {
    int i;

    sVramScreens = func_ov182_021bc4d8(sizeof(DWCUtilVramScreen) * 2, 4);
    for (i = 0; i < 2; i++) {
        sVramScreens[i].freeNodes = func_ov182_021bb9c8(32, (u8 *)sVramScreens[i].nodes, sizeof(DWCUtilVramNode));
        sVramScreens[i].list = func_ov182_021bcb3c();
        sVramScreens[i].head.pos = 0x300;
        sVramScreens[i].tail.pos = 0x400;
        func_ov182_021bcbbc(sVramScreens[i].list, &sVramScreens[i].head);
        func_ov182_021bcbb0(sVramScreens[i].list, &sVramScreens[i].tail);
    }
}

DWCUtilVramNode *func_ov182_021bcdbc(int screen, int size, s16 fromFront, u32 *pos) {
    DWCUtilVramNode *node = func_ov182_021bba38(sVramScreens[screen].freeNodes);
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
                func_ov182_021bcb90(next, node);
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
                func_ov182_021bcb90(position, node);
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

void func_ov182_021bcec4(DWCUtilVramNode *node) {
    int screen = 0;

    func_ov182_021bcb6c(node);
    if (node >= sVramScreens[1].nodes) {
        screen = 1;
    }
    func_ov182_021bba00(sVramScreens[screen].freeNodes, node);
}
