#include "nhttp_internal.h"

// NHTTP_control: the header fields and the post data of a request, and the boundary of a multipart post

// The boundary that a request starts with; its last 18 characters change while the data has the boundary in it
const char NHTTPi_boundary[] = "--t9Sf4yfjf1RtvDu3AA";

static char NHTTPi_nextBoundaryChar(u8 c);
static BOOL NHTTPi_makeBoundaryUnique(NHttpRequest *request, const char *data, int length);

// Adds a part to the end of a list, or sets the value of the part that has the name (the name is compared as
// NHTTPi_compareToken does)
BOOL NHTTPi_addHdrList(NHttpHeader **list, NHttpState *state, const char *name, const char *value) {
    NHttpHeader *head = *list;
    NHttpHeader *node;
    BOOL found = FALSE;
    NHttpHeader *created;

    if (head != NULL) {
        node = head;
        if (NHTTPi_compareToken(name, head->name) != 0) {
            node = head->next;
            if (node != *list) {
                do {
                    if (NHTTPi_compareToken(name, node->name) == 0) {
                        found = TRUE;
                        break;
                    }
                    node = node->next;
                } while (node != *list);
            }
        } else {
            found = TRUE;
        }
    }
    if (found) {
        node->value = (char *)value;
    } else {
        created = NHTTPi_alloc(sizeof(NHttpHeader), 4);
        if (created == NULL) {
            NHTTPi_SetError(state, 1);
            return FALSE;
        }
        created->name = (char *)name;
        created->value = (char *)value;
        created->length = 0;
        created->binary = 0;
        if (*list != NULL) {
            created->prev = (*list)->prev;
            created->next = *list;
            (*list)->prev->next = created;
            (*list)->prev = created;
        } else {
            created->next = created;
            created->prev = created;
            *list = created;
        }
    }
    return TRUE;
}

static char NHTTPi_nextBoundaryChar(u8 c) {
    u8 next = c + 1;

    if (next == '{') {
        next = '0';
    } else if (next == '[') {
        next = 'a';
    } else if (next == ':') {
        next = 'A';
    }
    return next;
}

// Changes the end of the boundary until it is not in the data. Returns FALSE if no change is enough
static BOOL NHTTPi_makeBoundaryUnique(NHttpRequest *request, const char *data, int length) {
    char *name;
    int i;
    char c;
    char original;

    if (NHTTPi_memfind(data, length, request->boundary + 2, NHTTP_BOUNDARY_SIZE - 2) < 0) {
        return TRUE;
    }
    i = NHTTP_BOUNDARY_SIZE - 1;
    name = request->boundary + 2;
    for (; i >= 2; i--) {
        c = request->boundary[i];
        original = NHTTPi_boundary[i];
        do {
            c = NHTTPi_nextBoundaryChar(c);
            request->boundary[i] = c;
            if (c == original) {
                break;
            }
            if (NHTTPi_memfind(data, length, name, NHTTP_BOUNDARY_SIZE - 2) < 0) {
                return TRUE;
            }
        } while (TRUE);
    }
    return FALSE;
}

// Takes the first part off a list and returns it
NHttpHeader *NHTTPi_getHdrFromList(void *list) {
    NHttpHeader **headPtr = list;
    NHttpHeader *head = *headPtr;

    if (head != NULL) {
        NHttpHeader *prev = head->prev;
        NHttpHeader *next;

        if (head != prev) {
            prev->next = head->next;
            head->next->prev = head->prev;
            next = head->next;
        } else {
            next = NULL;
        }
        *headPtr = next;
    }
    return head;
}

BOOL NHTTP_AddHeaderField(NHttpRequest *request, NHttpState *state, const char *name, const char *value) {
    if (request->sent != 0) {
        return FALSE;
    }
    return NHTTPi_addHdrList(&request->headers, state, name, value);
}

BOOL NHTTP_AddPostDataAscii(NHttpRequest *request, NHttpState *state, const char *name, const char *value) {
    BOOL added = FALSE;
    int length = 0;

    if (request->sent != 0) {
        return FALSE;
    }
    if (request->rawPost != 0) {
        return FALSE;
    }
    if (value != NULL) {
        length = NHTTPi_strlen(value);
    }
    if (NHTTPi_makeBoundaryUnique(request, value, length)) {
        added = NHTTPi_addHdrList(&request->postData, state, name, value);
        if (added) {
            request->postData->prev->length = length;
        }
    }
    return added;
}
