#include "list.h"
#include "heap.h"
#include "nitro/os.h"

DWCUtilList *DWCi_LSTlForm(void) {
    DWCUtilList *list = DWCi_HEAPlAllocEx(sizeof(DWCUtilList), 4);

    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return list;
}

void DWCi_LSTlDelete(DWCUtilList *list) {
    DWCi_HEAPlFree_(&list);
}

void DWCi_LSTlErase(void *node) {
    DWCUtilNode *links = node;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    links->prev->next = links->next;
    links->next->prev = links->prev;
    links->next = NULL;
    links->prev = NULL;
    OS_EnableIrqMask(irq);
}

void DWCi_LSTlInsert(void *position, void *node) {
    DWCUtilNode *at = position;
    DWCUtilNode *links = node;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    at->prev->next = links;
    links->prev = at->prev;
    links->next = at;
    at->prev = links;
    OS_EnableIrqMask(irq);
}

void DWCi_LSTlPushBack(DWCUtilList *list, void *node) {
    DWCi_LSTlInsert(&list->tail, node);
}

void DWCi_LSTlPushFront(DWCUtilList *list, void *node) {
    DWCi_LSTlInsert(list->head.next, node);
}
