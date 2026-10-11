#include "ghttp_internal.h"

// ghttpCallbacks: the calls to the game's callbacks of a request

// Tells the game that the request has ended, with its result and the data that it received. If the game doesn't want
// the buffer (the callback returns FALSE), the buffer stays
void ghiCallCompletedCallback(GHIConnection *connection) {
    GHICompletedFunc completed;
    char *buffer;

    if (connection->result != 0) {
        func_ov011_0216cca8(8, 0, 2, "Socket Error: %d\n", connection->socketError);
    }
    completed = connection->completedCallback;
    if (completed != NULL) {
        BOOL keep;

        if (connection->type == 0) {
            buffer = connection->fileBuffer.data;
        } else {
            buffer = NULL;
        }
        keep = completed(connection->request, connection->result, buffer, connection->bytesReceived,
                         connection->callbackParam);
        if (buffer != NULL && !keep) {
            connection->fileBuffer.dontFree = TRUE;
        }
    }
}

void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, int length) {
    GHIProgressFunc progress = connection->progressCallback;

    if (progress != NULL) {
        progress(connection->request, connection->state, buffer, length, connection->bytesReceived,
                 connection->totalSize, connection->callbackParam);
    }
}

void ghiCallPostCallback(GHIConnection *connection) {
    if (connection->postingState.callback != NULL) {
        connection->postingState.callback(connection->request, connection->postingState.bytesPosted,
                                          connection->postingState.totalBytes, connection->postingState.index,
                                          func_ov011_02171588(connection->postingState.states),
                                          connection->callbackParam);
    }
}
