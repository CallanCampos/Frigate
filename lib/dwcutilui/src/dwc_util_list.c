#include "dwc_util_list.h"
#include "dwc_util_heap.h"
#include "nitro/os.h"

DWCUtilList *DWCUtilList_Create(void) {
    DWCUtilList *list = DWCUtilHeap_AllocClear(sizeof(DWCUtilList), 4);

    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return list;
}

void DWCUtilList_Destroy(DWCUtilList *list) {
    DWCUtilHeap_FreeAndNull(&list);
}

void DWCUtilList_Remove(void *node) {
    DWCUtilNode *links = node;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    links->prev->next = links->next;
    links->next->prev = links->prev;
    links->next = NULL;
    links->prev = NULL;
    OS_EnableIrqMask(irq);
}

void DWCUtilList_InsertBefore(void *position, void *node) {
    DWCUtilNode *at = position;
    DWCUtilNode *links = node;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    at->prev->next = links;
    links->prev = at->prev;
    links->next = at;
    at->prev = links;
    OS_EnableIrqMask(irq);
}

void DWCUtilList_Append(DWCUtilList *list, void *node) {
    DWCUtilList_InsertBefore(&list->tail, node);
}

void DWCUtilList_Prepend(DWCUtilList *list, void *node) {
    DWCUtilList_InsertBefore(list->head.next, node);
}
