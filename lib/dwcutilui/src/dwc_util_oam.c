#include "dwc_util_oam.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_shared.h"
#include "nitro/os.h"

// Each screen has 32 nodes, a list of the runs in use between a head and a tail node that bracket the screen's range
// of entries (64 to 127 of its OAM buffer), and a stack of the free nodes
typedef struct DWCUtilOamScreen {
    DWCUtilOamNode nodes[32];
    DWCUtilOamNode head;
    DWCUtilOamNode tail;
    void *list;
    DWCUtilStack *freeNodes;
} DWCUtilOamScreen;

static DWCUtilOamScreen *sScreens;

void DWCUtilOam_Init(void) {
    int i;

    sScreens = DWCUtilHeap_AllocClear(sizeof(DWCUtilOamScreen) * 2, 4);
    for (i = 0; i < 2; i++) {
        sScreens[i].freeNodes = DWCUtilStack_CreateFilled(32, (u8 *)sScreens[i].nodes, sizeof(DWCUtilOamNode));
        sScreens[i].list = DWCUtilList_Create();
        sScreens[i].head.oam = DWCUtilOamBuf_GetEntry(i, 64);
        sScreens[i].tail.oam = DWCUtilOamBuf_GetEntry(i, 127) + 1;
        DWCUtilList_Prepend(sScreens[i].list, &sScreens[i].head);
        DWCUtilList_Append(sScreens[i].list, &sScreens[i].tail);
    }
}

void DWCUtilOam_Exit(void) {
    int i;

    for (i = 0; i < 2; i++) {
        DWCUtilList_Destroy(sScreens[i].list);
        DWCUtilStack_Destroy(sScreens[i].freeNodes);
    }
    DWCUtilHeap_FreeAndNull(&sScreens);
}

DWCUtilOamNode *DWCUtilOam_Alloc(int screen, int count, BOOL fromFront) {
    DWCUtilOamNode *node = DWCUtilStack_Pop(sScreens[screen].freeNodes);
    DWCUtilOamNode *position;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    if (fromFront) {
        position = &sScreens[screen].head;
        while (position != &sScreens[screen].tail) {
            DWCUtilOamNode *next = position->next;
            GXOamAttr *end = position->oam + position->count;

            if (end + count <= next->oam) {
                node->oam = end;
                DWCUtilList_InsertBefore(next, node);
                break;
            }
            position = next;
        }
        if (position == &sScreens[screen].tail) {
            OS_Terminate();
        }
    } else {
        position = &sScreens[screen].tail;
        while (position != &sScreens[screen].head) {
            DWCUtilOamNode *prev = position->prev;
            GXOamAttr *start = position->oam - count;

            if (start >= prev->oam + prev->count) {
                node->oam = start;
                DWCUtilList_InsertBefore(position, node);
                break;
            }
            position = prev;
        }
        if (position == &sScreens[screen].head) {
            OS_Terminate();
        }
    }
    OS_EnableIrqMask(irq);
    node->count = count;
    return node;
}

void DWCUtilOam_Free(DWCUtilOamNode *node) {
    GXOamAttr *oam = node->oam;
    int screen = 0;
    int i;

    for (i = 0; i < node->count; i++) {
        G2_SetOBJEffect(oam, GX_OAM_EFFECT_NODISPLAY, 0);
        oam++;
    }
    DWCUtilList_Remove(node);
    if (node >= sScreens[1].nodes) {
        screen = 1;
    }
    DWCUtilStack_Push(sScreens[screen].freeNodes, node);
}

GXOamAttr *DWCUtilOam_GetEntry(DWCUtilOamNode *node, int index) {
    return &node->oam[index];
}

int DWCUtilOam_GetCount(DWCUtilOamNode *node) {
    return node->count;
}

void DWCUtilOam_SetEffect(DWCUtilOamNode *node, int index, int effect, int rsParam) {
    GXOamAttr *oam = node->oam;
    int i;

    if (index >= 0) {
        G2_SetOBJEffect(&oam[index], effect, rsParam);
        return;
    }
    for (i = 0; i < node->count; i++) {
        G2_SetOBJEffect(&oam[i], effect, rsParam);
    }
}

void DWCUtilOam_SetMode(DWCUtilOamNode *node, int index, int mode, int cParam) {
    GXOamAttr *oam = node->oam;
    int i;

    if (index >= 0) {
        G2_SetOBJMode(&oam[index], mode, cParam);
        return;
    }
    for (i = 0; i < node->count; i++) {
        G2_SetOBJMode(&oam[i], mode, cParam);
    }
}

void DWCUtilOam_SetNodePosition(DWCUtilOamNode *node, int index, int x, int y) {
    int i;
    int firstX;
    int firstY;
    DWCUtilOamAttr01 first;
    GXOamAttr *oam = node->oam;

    if (index >= 0) {
        DWCUtilOam_SetPosition(&oam[index], x, y);
        return;
    }
    first = DWCUtilOam_ReadAttr01(&oam[0]);
    firstX = (first.attr01 & GX_OAM_ATTR01_X_MASK) >> GX_OAM_ATTR01_X_SHIFT;
    firstY = (u8)first.attr01;
    DWCUtilOam_SetPosition(&oam[0], x, y);
    for (i = 1; i < node->count; i++) {
        DWCUtilOamAttr01 word = DWCUtilOam_ReadAttr01(&oam[i]);

        DWCUtilOam_SetPosition(&oam[i], ((word.attr01 & GX_OAM_ATTR01_X_MASK) >> GX_OAM_ATTR01_X_SHIFT) + (x - firstX),
                               (u8)word.attr01 + (y - firstY));
    }
}

void DWCUtilOam_SetPriority(DWCUtilOamNode *node, int index, int priority) {
    GXOamAttr *oam = node->oam;
    int i;

    if (index >= 0) {
        G2_SetOBJPriority(&oam[index], priority);
        return;
    }
    for (i = 0; i < node->count; i++) {
        G2_SetOBJPriority(&oam[i], priority);
    }
}

void DWCUtilOam_GetNodePosition(DWCUtilOamNode *node, int index, int *x, int *y) {
    DWCUtilOam_GetPosition(&node->oam[index], x, y);
}
