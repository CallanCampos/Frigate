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

static inline void DWCUtilOam_GetPosition(const GXOamAttr *oam, int *x, int *y) {
    *x = (oam->attr01 & GX_OAM_ATTR01_X_MASK) >> GX_OAM_ATTR01_X_SHIFT;
    *y = (u8)oam->attr01;
}

void func_ov182_021bb480(void) {
    int i;

    sScreens = func_ov182_021bc4d8(sizeof(DWCUtilOamScreen) * 2, 4);
    for (i = 0; i < 2; i++) {
        sScreens[i].freeNodes = func_ov182_021bb9c8(32, (u8 *)sScreens[i].nodes, sizeof(DWCUtilOamNode));
        sScreens[i].list = func_ov182_021bcb3c();
        sScreens[i].head.oam = func_ov182_021bccd8(i, 64);
        sScreens[i].tail.oam = func_ov182_021bccd8(i, 127) + 1;
        func_ov182_021bcbbc(sScreens[i].list, &sScreens[i].head);
        func_ov182_021bcbb0(sScreens[i].list, &sScreens[i].tail);
    }
}

void func_ov182_021bb54c(void) {
    int i;

    for (i = 0; i < 2; i++) {
        func_ov182_021bcb58(sScreens[i].list);
        func_ov182_021bb9ec(sScreens[i].freeNodes);
    }
    func_ov182_021bc4f0(&sScreens);
}

DWCUtilOamNode *func_ov182_021bb594(int screen, int count, BOOL fromFront) {
    DWCUtilOamNode *node = func_ov182_021bba38(sScreens[screen].freeNodes);
    DWCUtilOamNode *position;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    if (fromFront) {
        position = &sScreens[screen].head;
        while (position != &sScreens[screen].tail) {
            DWCUtilOamNode *next = position->next;
            GXOamAttr *end = position->oam + position->count;

            if (end + count <= next->oam) {
                node->oam = end;
                func_ov182_021bcb90(next, node);
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
                func_ov182_021bcb90(position, node);
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

void func_ov182_021bb690(DWCUtilOamNode *node) {
    GXOamAttr *oam = node->oam;
    int screen = 0;
    int i;

    for (i = 0; i < node->count; i++) {
        G2_SetOBJEffect(oam, GX_OAM_EFFECT_NODISPLAY, 0);
        oam++;
    }
    func_ov182_021bcb6c(node);
    if (node >= sScreens[1].nodes) {
        screen = 1;
    }
    func_ov182_021bba00(sScreens[screen].freeNodes, node);
}

GXOamAttr *func_ov182_021bb6ec(DWCUtilOamNode *node, int index) {
    return &node->oam[index];
}

int func_ov182_021bb6f4(DWCUtilOamNode *node) {
    return node->count;
}

void func_ov182_021bb6f8(DWCUtilOamNode *node, int index, int effect, int rsParam) {
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

void func_ov182_021bb778(DWCUtilOamNode *node, int index, int mode, int cParam) {
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

void func_ov182_021bb7e0(DWCUtilOamNode *node, int index, int x, int y) {
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

void func_ov182_021bb878(DWCUtilOamNode *node, int index, int priority) {
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

void func_ov182_021bb8bc(DWCUtilOamNode *node, int index, int *x, int *y) {
    DWCUtilOam_GetPosition(&node->oam[index], x, y);
}
