#include "nhttp_internal.h"

// NHTTP_list: the requests that were sent and wait for the thread, in a ring where the head is the one that runs next

void NHTTPi_InitListInfo(NHttpListInfo *info) {
    info->head = NULL;
    info->count = 0;
}

// Puts a request at the end of the list and gives it an id, or returns -1 if there is no memory
int NHTTPi_setReqQueue(NHttpListInfo *info, NHttpRequest *request) {
    int id = -1;
    NHttpQueueNode *node = NHTTPi_alloc(sizeof(NHttpQueueNode), 4);

    if (node != NULL) {
        if (info->head != NULL) {
            node->prev = info->head->prev;
            node->next = info->head;
            info->head->prev->next = node;
            info->head->prev = node;
        } else {
            node->prev = node;
            node->next = node;
            info->head = node;
        }
        id = info->count++;
        node->id = id;
        node->request = request;
        node->socket = -1;
        if (info->count < 0) {
            info->count = 0;
        }
    }
    return id;
}

// Finds the request that has an id
NHttpQueueNode *NHTTPi_findReqQueue(NHttpListInfo *info, int id) {
    NHttpQueueNode *found = NULL;
    NHttpQueueNode *head = info->head;
    NHttpQueueNode *node;

    if (head != NULL) {
        if (head->id == id) {
            found = head;
        } else {
            node = head->next;
            while (node != head) {
                if (node->id == id) {
                    found = node;
                    break;
                }
                node = node->next;
            }
        }
    }
    return found;
}

// Takes a request out of the list and ends its connection
BOOL NHTTPi_freeReqQueue(NHttpListInfo *info, NHttpMutexInfo *mutexInfo, int id) {
    BOOL freed = FALSE;
    NHttpQueueNode *node = NHTTPi_findReqQueue(info, id);

    if (node != NULL) {
        NHttpConnection *connection;

        if (info->head != info->head->prev) {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            if (info->head == node) {
                info->head = node->next;
            }
        } else {
            info->head = NULL;
        }
        connection = NHTTPi_Request2Connection(mutexInfo, node->request);
        NHTTPi_destroyRequestObject(mutexInfo, node->request);
        NHTTPi_free(node);
        if (connection != NULL) {
            connection->status = 8;
            NHTTPi_CompleteCallback(mutexInfo, connection);
            NHTTPi_DestroyConnection(connection);
        }
        freed = TRUE;
    }
    return freed;
}

void NHTTPi_allFreeReqQueue(NHttpListInfo *info, NHttpMutexInfo *mutexInfo) {
    NHttpQueueNode *node = info->head;

    if (node != NULL) {
        do {
            NHTTPi_freeReqQueue(info, mutexInfo, node->id);
            node = info->head;
        } while (node != NULL);
    }
}

// Takes the request that runs next off the list
NHttpQueueNode *NHTTPi_getReqFromReqQueue(NHttpListInfo *info) {
    return (NHttpQueueNode *)NHTTPi_getHdrFromList(info);
}
