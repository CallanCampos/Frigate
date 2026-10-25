#include "objvram.h"
#include "deque.h"
#include "heap.h"
#include "list.h"
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

void DWCi_OVRlInit(void) {
    int i;

    sVramScreens = DWCi_HEAPlAllocEx(sizeof(DWCUtilVramScreen) * 2, 4);
    for (i = 0; i < 2; i++) {
        sVramScreens[i].freeNodes = DWCi_QUElFormEx(32, (u8 *)sVramScreens[i].nodes, sizeof(DWCUtilVramNode));
        sVramScreens[i].list = DWCi_LSTlForm();
        sVramScreens[i].head.pos = 0x300;
        sVramScreens[i].tail.pos = 0x400;
        DWCi_LSTlPushFront(sVramScreens[i].list, &sVramScreens[i].head);
        DWCi_LSTlPushBack(sVramScreens[i].list, &sVramScreens[i].tail);
    }
}

DWCUtilVramNode *DWCi_OVRlAlloc(int screen, int size, s16 fromFront, u32 *pos) {
    DWCUtilVramNode *node = DWCi_QUElPopBack(sVramScreens[screen].freeNodes);
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
                DWCi_LSTlInsert(next, node);
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
                DWCi_LSTlInsert(position, node);
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

void DWCi_OVRlFree(DWCUtilVramNode *node) {
    int screen = 0;

    DWCi_LSTlErase(node);
    if (node >= sVramScreens[1].nodes) {
        screen = 1;
    }
    DWCi_QUElPushBack(sVramScreens[screen].freeNodes, node);
}
