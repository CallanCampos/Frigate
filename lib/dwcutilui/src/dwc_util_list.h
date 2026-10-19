#ifndef POKEBW2_DWCUTIL_DWC_UTIL_LIST_H
#define POKEBW2_DWCUTIL_DWC_UTIL_LIST_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's doubly linked lists: a head and a tail node bracket the nodes of a list, and every node
// starts with its links. Linking and unlinking hold off the VBlank interrupt.
// The ROM does not embed this source file's name; dwc_util_list.c is descriptive.

struct DWCUtilNode {
    DWCUtilNode *prev;
    DWCUtilNode *next;
};

struct DWCUtilList {
    DWCUtilNode head;
    DWCUtilNode tail;
};

DWCUtilList *func_ov182_021bcb3c(void);
void func_ov182_021bcb58(DWCUtilList *list);
void func_ov182_021bcb6c(void *node);
// Links the node in before position
void func_ov182_021bcb90(void *position, void *node);
// Links the node in last, or first
void func_ov182_021bcbb0(DWCUtilList *list, void *node);
void func_ov182_021bcbbc(DWCUtilList *list, void *node);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_LIST_H
