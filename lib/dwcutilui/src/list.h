#ifndef POKEBW2_DWCUTIL_LIST_H
#define POKEBW2_DWCUTIL_LIST_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's doubly linked lists: a head and a tail node bracket the nodes of a list, and every node
// starts with its links. Linking and unlinking hold off the VBlank interrupt.
// The file is the DWC utility library's list.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it. NitroSystem's lib/nnsys/src/fnd/list.c has its name too, so the
// link takes their objects under names of their paths (docs/configs.md, "Files of one name").

struct DWCUtilNode {
    DWCUtilNode *prev;
    DWCUtilNode *next;
};

struct DWCUtilList {
    DWCUtilNode head;
    DWCUtilNode tail;
};

DWCUtilList *DWCi_LSTlForm(void);
void DWCi_LSTlDelete(DWCUtilList *list);
void DWCi_LSTlErase(void *node);
// Links the node in before position
void DWCi_LSTlInsert(void *position, void *node);
// Links the node in last, or first
void DWCi_LSTlPushBack(DWCUtilList *list, void *node);
void DWCi_LSTlPushFront(DWCUtilList *list, void *node);

#endif // POKEBW2_DWCUTIL_LIST_H
