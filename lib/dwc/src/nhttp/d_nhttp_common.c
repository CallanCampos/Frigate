#include "nhttp_internal.h"

// d_nhttp_common: what the library keeps (one NHttpRoot), the list of the connections and the callbacks that tell the
// game about a connection

// The connections, linked by NHttpConnection.next
static NHttpConnection *sConnectionList;
static NHttpRoot *sRoot;
static NHttpWaitInfo sWaitInfo;
static NHttpRoot sRootStorage;

static NHttpWaitInfo *NHTTPi_GetWaitInfoP(void);
static void NHTTPi_InitSystemInfo(NHttpRoot *root);
static int NHTTPi_SetBufferInfo(NHttpMutexInfo *mutexInfo, NHttpConnection *connection, void *buffer, int size);
static int NHTTPi_SetResponseBuffer(NHttpMutexInfo *mutexInfo, NHttpConnection *connection, void *buffer, int size,
                                    int used);

static NHttpWaitInfo *NHTTPi_GetWaitInfoP(void) {
    NHttpWaitInfo *info = &sWaitInfo;

    if (!sWaitInfo.ready) {
        NHTTPi_InitMutex(&info->mutex);
        info->waiters.tail = NULL;
        info->waiters.head = NULL;
        info->ready = TRUE;
    }
    return info;
}

// Sleeps until the connection is not in use any more
void NHTTPi_WaitConnection(NHttpConnection *connection) {
    NHttpWaitInfo *info = NHTTPi_GetWaitInfoP();

    if (connection->running != 0) {
        do {
            scheduler_yield(&info->waiters);
        } while (connection->running != 0);
    }
}

void NHTTPi_WakeConnection(NHttpConnection *connection) {
    NHttpWaitInfo *info = NHTTPi_GetWaitInfoP();

    connection->running = FALSE;
    OS_WakeupThread(&info->waiters);
}

// Looks for a connection in the list, or puts one in it or takes one out. How it looks for it is the mode: 0 for the
// connection itself, 1 for the one that has a request, 2 for the one that has a response, 3 to put one in and 4 to take
// one out. Returns the connection found, or NULL
NHttpConnection *NHTTPi_ControlConnectionList(NHttpMutexInfo *mutexInfo, NHttpConnection *key, int mode) {
    NHttpConnection *found = NULL;
    NHttpConnection **link;
    NHttpConnection *node;

    NHTTPi_lockReqList(mutexInfo);
    if (mode == 3) {
        found = key;
        key->next = sConnectionList;
        sConnectionList = key;
    } else {
        link = &sConnectionList;
        node = sConnectionList;
        if (node != NULL) {
            do {
                switch (mode) {
                case 0:
                    if (node == key) {
                        found = node;
                    }
                    break;
                case 1:
                    if (node->request == (NHttpRequest *)key) {
                        found = node;
                    }
                    break;
                case 2:
                    if (node->response == (NHttpResponse *)key) {
                        found = node;
                    }
                    break;
                case 4:
                    if (node == key) {
                        found = *link;
                        *link = found->next;
                    }
                    break;
                }
                if (found != NULL) {
                    break;
                }
                node = *link;
                link = &node->next;
                node = node->next;
            } while (node != NULL);
        }
    }
    NHTTPi_unlockReqList(mutexInfo);
    return found;
}

int NHTTPi_CommitConnectionList(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    return -(NHTTPi_ControlConnectionList(mutexInfo, connection, 3) == NULL);
}

int NHTTPi_OmitConnectionList(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    return -(NHTTPi_ControlConnectionList(mutexInfo, connection, 4) == NULL);
}

NHttpResponse *NHTTPi_Connection2Response(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHttpConnection *found = NHTTPi_GetConnection(mutexInfo, connection);

    if (found != NULL) {
        return found->response;
    }
    return NULL;
}

NHttpConnection *NHTTPi_Request2Connection(NHttpMutexInfo *mutexInfo, NHttpRequest *request) {
    return NHTTPi_ControlConnectionList(mutexInfo, (NHttpConnection *)request, 1);
}

NHttpConnection *NHTTPi_Response2Connection(NHttpMutexInfo *mutexInfo, NHttpResponse *response) {
    return NHTTPi_ControlConnectionList(mutexInfo, (NHttpConnection *)response, 2);
}

NHttpConnection *NHTTPi_GetConnection(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    return NHTTPi_ControlConnectionList(mutexInfo, connection, 0);
}

// The request of a connection, or the argument if it is not one
NHttpRequest *NHTTPi_GetRequest(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHttpRequest *request = (NHttpRequest *)connection;
    NHttpConnection *found = NHTTPi_GetConnection(mutexInfo, connection);

    if (found != NULL) {
        request = found->request;
    }
    return request;
}

NHttpResponse *NHTTPi_GetResponse(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHttpResponse *response = (NHttpResponse *)connection;
    NHttpConnection *found = NHTTPi_GetConnection(mutexInfo, connection);

    if (found != NULL) {
        response = found->response;
    }
    return response;
}

int NHTTPi_GetConnectionListLength(void) {
    NHttpConnection *node = sConnectionList;
    int length = 0;

    if (node != NULL) {
        do {
            length++;
            node = node->next;
        } while (node != NULL);
    }
    return length;
}

// Calls the callback of a connection when the post data asks for more: with the name of the part and how much was
// given, and the buffer the callback sets. Returns what the callback returned, or -1
int NHTTPi_PostSendCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection, const char *name, int offset) {
    int result = -1;
    NHttpPostEvent event;

    if (NHTTPi_GetConnection(mutexInfo, connection) != NULL) {
        if (NHTTPi_Connection2Response(mutexInfo, connection) != NULL && connection->callback != NULL) {
            event.name = name;
            event.buffer = connection->userBuffer;
            event.bufferSize = connection->userBufferSize;
            event.offset = offset;
            result = connection->callback((int)connection, 1, &event);
            NHTTPi_SetBufferInfo(mutexInfo, connection, event.buffer, event.bufferSize);
        }
    }
    return result;
}

// Calls the callback of a connection when the buffer of its answer is full, and takes the buffer that it gives
void NHTTPi_BufferFullCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHttpResponse *response;
    NHttpBufferEvent event;

    if (NHTTPi_GetConnection(mutexInfo, connection) != NULL) {
        response = NHTTPi_Connection2Response(mutexInfo, connection);
        if (response != NULL && connection->callback != NULL) {
            event.buffer = response->buffer;
            event.bufferSize = response->bufferSize;
            event.used = response->bufferUsed;
            connection->callback((int)connection, 2, &event);
            NHTTPi_SetResponseBuffer(mutexInfo, connection, event.buffer, event.bufferSize, event.used);
        }
    }
}

// The same when the body has been received
void NHTTPi_ReceivedCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHttpResponse *response;
    NHttpBufferEvent event;

    if (NHTTPi_GetConnection(mutexInfo, connection) != NULL) {
        response = NHTTPi_Connection2Response(mutexInfo, connection);
        if (response != NULL && connection->callback != NULL) {
            event.buffer = response->buffer;
            event.bufferSize = response->bufferSize;
            event.used = response->bufferUsed;
            connection->callback((int)connection, 3, &event);
            NHTTPi_SetResponseBuffer(mutexInfo, connection, event.buffer, event.bufferSize, event.used);
        }
    }
}

// Calls the callback of a connection when it has ended
void NHTTPi_CompleteCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    if (NHTTPi_GetConnection(mutexInfo, connection) != NULL && connection->callback != NULL) {
        connection->callback((int)connection, 4, NULL);
    }
}

static int NHTTPi_SetBufferInfo(NHttpMutexInfo *mutexInfo, NHttpConnection *connection, void *buffer, int size) {
    NHttpConnection *found = NHTTPi_GetConnection(mutexInfo, connection);

    if (found == NULL) {
        return -1;
    }
    found->userBuffer = buffer;
    found->userBufferSize = size;
    return 0;
}

static int NHTTPi_SetResponseBuffer(NHttpMutexInfo *mutexInfo, NHttpConnection *connection, void *buffer, int size,
                                    int used) {
    NHttpConnection *found = NHTTPi_GetConnection(mutexInfo, connection);
    NHttpResponse *response;

    if (found != NULL) {
        response = NHTTPi_Connection2Response(mutexInfo, found);
        if (response != NULL) {
            response->buffer = buffer;
            response->bufferSize = size;
            response->bufferUsed = used;
            return 0;
        }
        return -1;
    }
    return -1;
}

static void NHTTPi_InitSystemInfo(NHttpRoot *root) {
    NHTTPi_InitBgnEndInfo(NHTTPi_GetBgnEndInfoP(root));
    NHTTPi_InitListInfo(NHTTPi_GetListInfoP(root));
    NHTTPi_InitRequestInfo(NHTTPi_GetReqInfoP(root));
    NHTTPi_InitMutexInfo(NHTTPi_GetMutexInfoP(root));
    NHTTPi_InitThreadInfo(NHTTPi_GetThreadInfoP(root));
}

NHttpRoot *NHTTPi_GetSystemInfoP(void) {
    NHttpRoot *root = sRoot;

    if (root == NULL) {
        root = &sRootStorage;
        sRoot = root;
        NHTTPi_InitSystemInfo(root);
    }
    return root;
}

NHttpState *NHTTPi_GetBgnEndInfoP(NHttpRoot *root) {
    return &root->state;
}

NHttpListInfo *NHTTPi_GetListInfoP(NHttpRoot *root) {
    return &root->list;
}

NHttpReqInfo *NHTTPi_GetReqInfoP(NHttpRoot *root) {
    return &root->req;
}

NHttpThreadInfo *NHTTPi_GetThreadInfoP(NHttpRoot *root) {
    return &root->threadInfo;
}

NHttpMutexInfo *NHTTPi_GetMutexInfoP(NHttpRoot *root) {
    return &root->mutexInfo;
}

// Sets how much of the body the connection is to receive, as the chunk in the answer says
void NHTTPi_SetVirtualContentLength(NHttpConnection *connection, int length) {
    if (connection != NULL) {
        connection->virtualContentLength = length;
    }
}

void NHTTPi_InitConnectionList(void) {
    if (sConnectionList != NULL) {
        NHTTPi_GetConnectionListLength();
    }
    sConnectionList = NULL;
}

void NHTTPi_InitMutexInfo(NHttpMutexInfo *mutexInfo) {
    mutexInfo->ready = FALSE;
}

void NHTTPi_initLockReqList(NHttpMutexInfo *mutexInfo) {
    if (!mutexInfo->ready) {
        NHTTPi_InitMutex(&mutexInfo->mutex);
        mutexInfo->ready = TRUE;
    }
}

void NHTTPi_exitLockReqList(void) {
}

void NHTTPi_lockReqList(NHttpMutexInfo *mutexInfo) {
    NHTTPi_LockMutex(&mutexInfo->mutex);
}

void NHTTPi_unlockReqList(NHttpMutexInfo *mutexInfo) {
    OS_UnlockMutex(&mutexInfo->mutex);
}

// Counts the users of a connection, and gives how many there are
int NHTTPi_AddWaiter(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHTTPi_lockReqList(mutexInfo);
    connection->waiters++;
    NHTTPi_unlockReqList(mutexInfo);
    return connection->waiters;
}

int NHTTPi_RemoveWaiter(NHttpMutexInfo *mutexInfo, NHttpConnection *connection) {
    NHTTPi_lockReqList(mutexInfo);
    connection->waiters--;
    NHTTPi_unlockReqList(mutexInfo);
    return connection->waiters;
}
