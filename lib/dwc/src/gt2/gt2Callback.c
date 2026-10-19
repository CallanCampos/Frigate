#include "dwc/gt2Callback.h"
#include "dwc/ext.h"
#include "dwc/gt2Socket.h"

// GameSpy GT2's calls into the app's callbacks (gt2Callback.c). The functions' names, from pret/pokerevo's Wii build,
// are in dwc/gt2Callback.h

BOOL func_ov011_0217c9f4(GTI2Socket *socket) {
    if (socket == NULL) {
        return TRUE;
    }
    if (socket->onSocketError == NULL) {
        return TRUE;
    }
    socket->callbackLevel++;
    socket->onSocketError(socket);
    socket->callbackLevel--;
    if (socket->close && socket->callbackLevel == 0) {
        func_ov011_0217e944(socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217ca30(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, u32 latency,
                         const void *message, int length) {
    if (socket == NULL || connection == NULL) {
        return TRUE;
    }
    if (socket->onConnectAttempt == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    socket->callbackLevel++;
    connection->callbackLevel++;
    socket->onConnectAttempt(socket, connection, ip, port, latency, message, length);
    socket->callbackLevel--;
    connection->callbackLevel--;
    if (socket->close && socket->callbackLevel == 0) {
        func_ov011_0217e944(socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cab0(GTI2Connection *connection, int result, const void *message, int length) {
    if (connection == NULL) {
        return TRUE;
    }
    connection->result = result;
    if (connection->callbacks.onConnected == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.onConnected(connection, result, message, length);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        func_ov011_0217e944(connection->socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cb10(GTI2Connection *connection, const void *message, int length, BOOL reliable) {
    if (connection == NULL) {
        return TRUE;
    }
    if (connection->callbacks.onReceived == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.onReceived(connection, message, length, reliable);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        func_ov011_0217e944(connection->socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cb6c(GTI2Connection *connection, int reason) {
    if (connection == NULL) {
        return TRUE;
    }
    if (connection->callbacks.onClosed == NULL) {
        return TRUE;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.onClosed(connection, reason);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        func_ov011_0217e944(connection->socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cbbc(GTI2Connection *connection, u32 latency) {
    if (connection == NULL) {
        return TRUE;
    }
    if (connection->callbacks.onPing == NULL) {
        return TRUE;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.onPing(connection, latency);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        func_ov011_0217e944(connection->socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cc0c(GTI2Connection *connection, int filterId, const void *message, int length, BOOL reliable) {
    GTI2FilterCallback *filter;

    if (connection == NULL) {
        return TRUE;
    }
    filter = func_ov011_0217158c(connection->sendFilters, filterId);
    if (filter == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    (*filter)(connection, filterId, message, length, reliable);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        func_ov011_0217e944(connection->socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cc8c(GTI2Connection *connection, int filterId, const void *message, int length, BOOL reliable) {
    GTI2FilterCallback *filter;

    if (connection == NULL) {
        return TRUE;
    }
    filter = func_ov011_0217158c(connection->receiveFilters, filterId);
    if (filter == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    (*filter)(connection, filterId, message, length, reliable);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        func_ov011_0217e944(connection->socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cd0c(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, BOOL reset,
                         const void *message, int length, BOOL send) {
    GTI2DumpCallback callback;

    if (socket == NULL) {
        return TRUE;
    }
    if (send) {
        callback = socket->onSendDump;
    } else {
        callback = socket->onReceiveDump;
    }
    if (callback == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    socket->callbackLevel++;
    if (connection != NULL) {
        connection->callbackLevel++;
    }
    callback(socket, connection, ip, port, reset, message, length);
    socket->callbackLevel--;
    if (connection != NULL) {
        connection->callbackLevel--;
    }
    if (socket->close && socket->callbackLevel == 0) {
        func_ov011_0217e944(socket);
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217cd98(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length, BOOL *handled) {
    *handled = FALSE;
    if (socket == NULL) {
        return TRUE;
    }
    if (socket->onUnrecognizedMessage == NULL) {
        return TRUE;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    socket->callbackLevel++;
    *handled = socket->onUnrecognizedMessage(socket, ip, port, message, length);
    socket->callbackLevel--;
    if (socket->close && socket->callbackLevel == 0) {
        func_ov011_0217e944(socket);
        return FALSE;
    }
    return TRUE;
}
