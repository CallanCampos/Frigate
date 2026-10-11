#include "nhttp_internal.h"

// NHTTP_bgnend: starting the library up and ending it, the allocator and the error that the library keeps

void NHTTPi_InitBgnEndInfo(NHttpState *state) {
    state->alloc = NULL;
    state->free = NULL;
    state->started = FALSE;
    state->socket = -1;
    NHTTPi_SetSSLError(state, 0);
    NHTTPi_SetError(state, 0);
    state->quit = FALSE;
    state->stack = NULL;
    state->unk_20 = 0;
}

void *NHTTPi_alloc(u32 size, int align) {
    NHttpState *state = &NHTTPi_GetSystemInfoP()->state;

    if (state->alloc != NULL) {
        return state->alloc(size, align);
    }
    return NULL;
}

void NHTTPi_free(void *ptr) {
    NHttpState *state = &NHTTPi_GetSystemInfoP()->state;

    if (state->free != NULL) {
        state->free(ptr);
    }
}

void NHTTPi_SetError(NHttpState *state, int error) {
    state->error = error;
}

void NHTTPi_SetSSLError(NHttpState *state, int error) {
    state->sslError = error;
}

int NHTTPi_GetSSLError(NHttpState *state) {
    return state->sslError;
}

// Starts the library: its lists and its mutex, the stack of the thread that runs the requests and the thread. Returns
// whether it did, and the error says why not
BOOL NHTTPi_Startup(NHttpRoot *root, NHttpAllocFunc alloc, NHttpFreeFunc free, int priority) {
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpListInfo *list = NHTTPi_GetListInfoP(root);
    NHttpReqInfo *req = NHTTPi_GetReqInfoP(root);
    NHttpMutexInfo *mutexInfo = NHTTPi_GetMutexInfoP(root);
    NHttpThreadInfo *thread = NHTTPi_GetThreadInfoP(root);

    state->alloc = alloc;
    state->free = free;
    state->error = 0;
    state->sslError = 0;
    state->quit = FALSE;
    state->unk_24 = 0;
    NHTTPi_InitListInfo(list);
    NHTTPi_InitRequestInfo(req);
    NHTTPi_initLockReqList(mutexInfo);
    NHTTPi_InitConnectionList();
    state->socket = -1;
    state->stack = NHTTPi_alloc(0x2000, 8);
    if (state->stack == NULL) {
        NHTTPi_SetError(state, 1);
        NHTTPi_exitLockReqList();
        return FALSE;
    }
    if (!NHTTPi_createCommThread(thread, priority, state->stack)) {
        NHTTPi_SetError(state, 9);
        NHTTPi_free(state->stack);
        state->stack = NULL;
        NHTTPi_exitLockReqList();
        return FALSE;
    }
    state->started = TRUE;
    return TRUE;
}

// Ends the library: cancels the requests, ends the thread and frees its stack, then calls back
void NHTTPi_CleanupAsync(NHttpRoot *root, void (*callback)(void)) {
    NHttpState *state = NHTTPi_GetBgnEndInfoP(root);
    NHttpThreadInfo *thread = NHTTPi_GetThreadInfoP(root);

    NHTTPi_CheckCurrentThread(thread, TRUE);
    NHTTPi_cancelAllRequests(root);
    NHTTPi_destroyCommThread(thread, state);
    NHTTPi_free(state->stack);
    state->stack = NULL;
    NHTTPi_exitLockReqList();
    state->started = FALSE;
    if (callback != NULL) {
        callback();
    }
    NHTTPi_GetConnectionListLength();
    if (state->socket >= 0) {
        NHTTPi_SocClose(NULL, NULL, state->socket);
        state->socket = -1;
    }
}

int NHTTPi_GetError(NHttpState *state) {
    return state->error;
}
