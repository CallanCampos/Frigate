#include "nnsys/fnd.h"

// NitroSystem's doubly linked lists (NNS_Fnd). An object is linked through the NNSFndLink at the list's offset in it,
// so one struct can sit in several lists. swan calls none of them by name

static void SetFirstObject(NNSFndList *list, void *object);

// The link inside object that this list uses
static inline NNSFndLink *OBJ_TO_LINK(const NNSFndList *list, void *object) {
    return (NNSFndLink *)((u8 *)object + list->offset);
}

void NNS_FndInitList(NNSFndList *list, u16 offset) {
    list->headObject = NULL;
    list->tailObject = NULL;
    list->numObjects = 0;
    list->offset = offset;
}

// Makes object the only member of an empty list
static void SetFirstObject(NNSFndList *list, void *object) {
    NNSFndLink *link = OBJ_TO_LINK(list, object);

    link->nextObject = NULL;
    link->prevObject = NULL;
    list->headObject = object;
    list->tailObject = object;
    list->numObjects++;
}

void NNS_FndAppendListObject(NNSFndList *list, void *object) {
    NNSFndLink *link;

    if (list->headObject == NULL) {
        SetFirstObject(list, object);
        return;
    }
    link = OBJ_TO_LINK(list, object);
    link->prevObject = list->tailObject;
    link->nextObject = NULL;
    OBJ_TO_LINK(list, list->tailObject)->nextObject = object;
    list->tailObject = object;
    list->numObjects++;
}

void NNS_FndPrependListObject(NNSFndList *list, void *object) {
    NNSFndLink *link;

    if (list->headObject == NULL) {
        SetFirstObject(list, object);
        return;
    }
    link = OBJ_TO_LINK(list, object);
    link->prevObject = NULL;
    link->nextObject = list->headObject;
    OBJ_TO_LINK(list, list->headObject)->prevObject = object;
    list->headObject = object;
    list->numObjects++;
}

// Puts object just before target, or at the end when target is NULL
void NNS_FndInsertListObject(NNSFndList *list, void *target, void *object) {
    if (target == NULL) {
        NNS_FndAppendListObject(list, object);
    } else if (target == list->headObject) {
        NNS_FndPrependListObject(list, object);
    } else {
        NNSFndLink *beforeLink;
        NNSFndLink *link = OBJ_TO_LINK(list, object);
        void *before = OBJ_TO_LINK(list, target)->prevObject;

        beforeLink = OBJ_TO_LINK(list, before);
        link->prevObject = before;
        link->nextObject = target;
        beforeLink->nextObject = object;
        OBJ_TO_LINK(list, target)->prevObject = object;
        list->numObjects++;
    }
}

void NNS_FndRemoveListObject(NNSFndList *list, void *object) {
    NNSFndLink *link = OBJ_TO_LINK(list, object);

    if (link->prevObject == NULL) {
        list->headObject = link->nextObject;
    } else {
        OBJ_TO_LINK(list, link->prevObject)->nextObject = link->nextObject;
    }
    if (link->nextObject == NULL) {
        list->tailObject = link->prevObject;
    } else {
        OBJ_TO_LINK(list, link->nextObject)->prevObject = link->prevObject;
    }
    link->prevObject = NULL;
    link->nextObject = NULL;
    list->numObjects--;
}

// The object after object, or the first when object is NULL
void *NNS_FndGetNextListObject(NNSFndList *list, void *object) {
    if (object == NULL) {
        return list->headObject;
    }
    return OBJ_TO_LINK(list, object)->nextObject;
}

// The object before object, or the last when object is NULL
void *NNS_FndGetPrevListObject(NNSFndList *list, void *object) {
    if (object == NULL) {
        return list->tailObject;
    }
    return OBJ_TO_LINK(list, object)->prevObject;
}
