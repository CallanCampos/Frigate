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

void DWCi_CELLlInit(void) {
    int i;

    sScreens = DWCi_HEAPlAllocEx(sizeof(DWCUtilOamScreen) * 2, 4);
    for (i = 0; i < 2; i++) {
        sScreens[i].freeNodes = DWCi_QUElFormEx(32, (u8 *)sScreens[i].nodes, sizeof(DWCUtilOamNode));
        sScreens[i].list = DWCi_LSTlForm();
        sScreens[i].head.oam = DWCi_OBJlGetOam(i, 64);
        sScreens[i].tail.oam = DWCi_OBJlGetOam(i, 127) + 1;
        DWCi_LSTlPushFront(sScreens[i].list, &sScreens[i].head);
        DWCi_LSTlPushBack(sScreens[i].list, &sScreens[i].tail);
    }
}

void DWCi_CELLlEnd(void) {
    int i;

    for (i = 0; i < 2; i++) {
        DWCi_LSTlDelete(sScreens[i].list);
        DWCi_QUElDelete(sScreens[i].freeNodes);
    }
    DWCi_HEAPlFree_(&sScreens);
}

DWCUtilOamNode *DWCi_CELLlForm(int screen, int count, BOOL fromFront) {
    DWCUtilOamNode *node = DWCi_QUElPopFront(sScreens[screen].freeNodes);
    DWCUtilOamNode *position;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    if (fromFront) {
        position = &sScreens[screen].head;
        while (position != &sScreens[screen].tail) {
            DWCUtilOamNode *next = position->next;
            GXOamAttr *end = position->oam + position->count;

            if (end + count <= next->oam) {
                node->oam = end;
                DWCi_LSTlInsert(next, node);
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
                DWCi_LSTlInsert(position, node);
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

void DWCi_CELLlDelete(DWCUtilOamNode *node) {
    GXOamAttr *oam = node->oam;
    int screen = 0;
    int i;

    for (i = 0; i < node->count; i++) {
        G2_SetOBJEffect(oam, GX_OAM_EFFECT_NODISPLAY, 0);
        oam++;
    }
    DWCi_LSTlErase(node);
    if (node >= sScreens[1].nodes) {
        screen = 1;
    }
    DWCi_QUElPushBack(sScreens[screen].freeNodes, node);
}

GXOamAttr *DWCi_CELLlGetObj(DWCUtilOamNode *node, int index) {
    return &node->oam[index];
}

int DWCi_CELLlGetObjNum(DWCUtilOamNode *node) {
    return node->count;
}

void DWCi_CELLlSetEffect(DWCUtilOamNode *node, int index, int effect, int rsParam) {
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

void DWCi_CELLlSetPalette(DWCUtilOamNode *node, int index, int mode, int cParam) {
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

void DWCi_CELLlSetPosition(DWCUtilOamNode *node, int index, int x, int y) {
    GXOamAttr *oam;
    int i;
    int firstX, firstY, entryX, entryY, dx, dy;

    oam = node->oam;
    if (index >= 0) {
        DWCUtilOam_SetPosition(&oam[index], x, y);
    } else {
        G2_GetOBJPosition(&oam[0], (u32 *)&firstX, (u32 *)&firstY);
        DWCUtilOam_SetPosition(&oam[0], x, y);
        dx = x - firstX;
        dy = y - firstY;
        for (i = 1; i < node->count; i++) {
            G2_GetOBJPosition(&oam[i], (u32 *)&entryX, (u32 *)&entryY);
            DWCUtilOam_SetPosition(&oam[i], entryX + dx, entryY + dy);
        }
    }
}

void DWCi_CELLlSetPriority(DWCUtilOamNode *node, int index, int priority) {
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

void DWCi_CELLlGetPosition(DWCUtilOamNode *node, int index, int *x, int *y) {
    G2_GetOBJPosition(&node->oam[index], (u32 *)x, (u32 *)y);
}
