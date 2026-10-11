#include "nhttp_internal.h"

// d_nhttp_private: making, starting and ending a connection, and asking what it has received

static int NHTTPi_WaitForConnection(NHttpConnection *connection);

// Makes a connection for a URL, a method and the buffer for the answer. The callback gets the events of the
// connection, with the argument
int NHTTPCreateConnection(const char *url, int method, void *buffer, u32 bufferSize, NHttpCallback callback, void *arg) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpConnection *connection = NHTTPi_alloc(sizeof(NHttpConnection), 0x20);

    if (connection == NULL) {
        NHTTPi_SetError(state, 1);
        return 0;
    }
    NHTTPi_memclr(connection, sizeof(NHttpConnection));
    connection->request = NHTTP_CreateRequest(state, url, method, buffer, bufferSize, arg, 0, NULL);
    if (connection->request == NULL) {
        NHTTPi_free(connection);
        return 0;
    }
    connection->response = connection->request->response;
    connection->stage = NHTTP_STAGE_NONE;
    connection->callback = callback;
    connection->userBuffer = NULL;
    connection->userBufferSize = 0;
    connection->id = -1;
    NHTTPi_CommitConnectionList(mutexInfo, connection);
    connection->status = 15;
    connection->unk_08 = 0;
    connection->running = TRUE;
    NHTTPi_SetVirtualContentLength(connection, 0);
    connection->unk_30 = 0;
    connection->recvPos = 0;
    connection->recvSize = 0;
    NHTTPi_memclr(connection->recvBuffer, NHTTP_RECV_BUFFER_SIZE);
    connection->waiters = 1;
    return (int)connection;
}

int NHTTPDestroyConnection(int handle) {
    NHttpConnection *connection =
        NHTTPi_GetConnection(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)handle);

    if (connection == NULL) {
        return -1;
    }
    return NHTTPi_DestroyConnection(connection);
}

// Ends a connection once nobody uses it: cancels its request, waits for the thread to let go of it, and frees what it
// has
int NHTTPi_DestroyConnection(NHttpConnection *connection) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);

    NHTTPi_GetThreadInfoP(root);
    if (connection == NULL) {
        return -1;
    }
    if (connection->waiters > 0 && connection->id >= 0) {
        NHTTP_CancelRequestAsync(root, connection->id);
        connection->id = -1;
    }
    if (NHTTPi_RemoveWaiter(mutexInfo, connection) > 0) {
        return 0;
    }
    if (connection->request != NULL && connection->request->sent == 0) {
        NHTTP_DestroyRequest(root, connection->request);
    }
    if (connection->response != NULL) {
        if (connection->request != NULL) {
            NHTTPi_WaitForConnection(connection);
        }
        NHTTP_DestroyResponse(mutexInfo, connection->response);
    }
    NHTTPi_OmitConnectionList(mutexInfo, connection);
    NHTTPi_free(connection);
    return 0;
}

// Waits for the thread to be done with a connection that it is receiving the answer of
static int NHTTPi_WaitForConnection(NHttpConnection *connection) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpThreadInfo *thread = NHTTPi_GetThreadInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpConnection *found = NHTTPi_GetConnection(mutexInfo, connection);
    int stage;

    if (found == NULL) {
        return -1;
    }
    stage = NHTTPGetConnectionState((int)connection);
    NHTTPi_CheckCurrentThread(thread, TRUE);
    if (stage != -1 && stage != 0) {
        if (NHTTPGetConnectionError((int)connection) == 15) {
            NHTTPi_WaitConnection(found);
        }
    }
    return 0;
}

// Sends the request of a connection to the thread
int NHTTPStartConnection(int handle) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpConnection *connection = NHTTPi_GetConnection(mutexInfo, (NHttpConnection *)handle);

    if (connection == NULL) {
        return -1;
    }
    if (connection->request == NULL) {
        return -1;
    }
    connection->id = NHTTP_SendRequestAsync(root, connection->request);
    if (connection->id >= 0) {
        connection->stage = NHTTP_STAGE_QUEUED;
        NHTTPi_AddWaiter(mutexInfo, connection);
    }
    return 0;
}

int NHTTPCancelConnection(int handle) {
    NHttpRoot *root = NHTTPi_GetSystemInfoP();
    NHttpConnection *connection = NHTTPi_GetConnection(NHTTPi_GetMutexInfoP(root), (NHttpConnection *)handle);

    if (connection == NULL) {
        return -1;
    }
    if (connection->id >= 0) {
        NHTTP_CancelRequestAsync(root, connection->id);
        return 0;
    }
    return -1;
}

int NHTTPGetConnectionState(int handle) {
    NHttpConnection *connection =
        NHTTPi_GetConnection(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)handle);

    if (connection == NULL) {
        return -1;
    }
    return connection->stage;
}

int NHTTPGetBodyBuffer(int handle, char **buffer, int *size) {
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHttpConnection *connection = NHTTPi_GetConnection(mutexInfo, (NHttpConnection *)handle);
    NHttpResponse *response;

    if (connection != NULL) {
        response = NHTTPi_Connection2Response(mutexInfo, connection);
        if (response != NULL) {
            *buffer = response->buffer;
            *size = response->bufferSize;
            return response->bufferUsed;
        }
        return -1;
    }
    return -1;
}

void *NHTTPGetUserParam(int handle) {
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHttpConnection *connection = NHTTPi_GetConnection(mutexInfo, (NHttpConnection *)handle);
    NHttpResponse *response;

    if (connection != NULL) {
        response = NHTTPi_Connection2Response(mutexInfo, connection);
        if (response != NULL) {
            return response->userParam;
        }
        return NULL;
    }
    return NULL;
}

// How the connection ended: 0 if well, an error otherwise, and 15 while it has not ended
int NHTTPGetConnectionError(int handle) {
    NHttpConnection *connection =
        NHTTPi_GetConnection(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHttpConnection *)handle);

    if (connection != NULL) {
        return connection->status;
    }
    return -1;
}

// What has been received of the body, and the size that the answer says it has (0 if it does not say)
int NHTTPGetProgress(int handle, int *received, int *size) {
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHttpConnection *connection = NHTTPi_GetConnection(mutexInfo, (NHttpConnection *)handle);
    NHttpResponse *response;

    if (connection != NULL) {
        response = NHTTPi_Connection2Response(mutexInfo, connection);
        if (response != NULL) {
            *received = response->bodyReceived;
            if (response->contentLength == -1) {
                *size = 0;
            } else {
                *size = response->contentLength;
            }
            return 0;
        }
        return -1;
    }
    return -1;
}
