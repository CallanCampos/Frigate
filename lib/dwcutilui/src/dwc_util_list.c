#include "dwc_util_list.h"
#include "dwc_util_heap.h"
#include "nitro/os.h"

DWCUtilList *func_ov182_021bcb3c(void) {
    DWCUtilList *list = func_ov182_021bc4d8(sizeof(DWCUtilList), 4);

    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return list;
}

void func_ov182_021bcb58(DWCUtilList *list) {
    func_ov182_021bc4f0(&list);
}

void func_ov182_021bcb6c(void *node) {
    DWCUtilNode *links = node;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    links->prev->next = links->next;
    links->next->prev = links->prev;
    links->next = NULL;
    links->prev = NULL;
    OS_EnableIrqMask(irq);
}

void func_ov182_021bcb90(void *position, void *node) {
    DWCUtilNode *at = position;
    DWCUtilNode *links = node;
    u32 irq = OS_DisableIrqMask(OS_IE_V_BLANK);

    at->prev->next = links;
    links->prev = at->prev;
    links->next = at;
    at->prev = links;
    OS_EnableIrqMask(irq);
}

void func_ov182_021bcbb0(DWCUtilList *list, void *node) {
    func_ov182_021bcb90(&list->tail, node);
}

void func_ov182_021bcbbc(DWCUtilList *list, void *node) {
    func_ov182_021bcb90(list->head.next, node);
}
