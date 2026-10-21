#include "dwc/gt2Callback.h"
#include "dwc/ext.h"
#include "dwc/gt2Socket.h"

// GameSpy GT2's calls into the app's callbacks (gt2Callback.c)

gsi_bool gti2SocketErrorCallback(GTI2Socket *socket) {
    if (socket == NULL) {
        return gsi_true;
    }
    if (socket->onSocketError == NULL) {
        return gsi_true;
    }
    socket->callbackLevel++;
    socket->onSocketError(socket);
    socket->callbackLevel--;
    if (socket->close && socket->callbackLevel == 0) {
        gti2CloseSocket(socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2ConnectAttemptCallback(GTI2Socket *socket, GTI2Connection *connection, gsi_u32 ip, gsi_u16 port,
                                    int latency, const void *message, int length) {
    if (socket == NULL || connection == NULL) {
        return gsi_true;
    }
    if (socket->onConnectAttempt == NULL) {
        return gsi_true;
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
        gti2CloseSocket(socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2ConnectedCallback(GTI2Connection *connection, int result, const void *message, int length) {
    if (connection == NULL) {
        return gsi_true;
    }
    connection->result = result;
    if (connection->callbacks.onConnected == NULL) {
        return gsi_true;
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
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2ReceivedCallback(GTI2Connection *connection, const void *message, int length, gsi_bool reliable) {
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->callbacks.onReceived == NULL) {
        return gsi_true;
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
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2ClosedCallback(GTI2Connection *connection, int reason) {
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->callbacks.onClosed == NULL) {
        return gsi_true;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.onClosed(connection, reason);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2PingCallback(GTI2Connection *connection, int latency) {
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->callbacks.onPing == NULL) {
        return gsi_true;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.onPing(connection, latency);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2SendFilterCallback(GTI2Connection *connection, int filterId, const void *message, int length,
                                gsi_bool reliable) {
    GTI2FilterCallback *filter;

    if (connection == NULL) {
        return gsi_true;
    }
    filter = func_ov011_0217158c(connection->sendFilters, filterId);
    if (filter == NULL) {
        return gsi_true;
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
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2ReceiveFilterCallback(GTI2Connection *connection, int filterId, const void *message, int length,
                                   gsi_bool reliable) {
    GTI2FilterCallback *filter;

    if (connection == NULL) {
        return gsi_true;
    }
    filter = func_ov011_0217158c(connection->receiveFilters, filterId);
    if (filter == NULL) {
        return gsi_true;
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
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2DumpCallback(GTI2Socket *socket, GTI2Connection *connection, gsi_u32 ip, gsi_u16 port, gsi_bool reset,
                          const void *message, int length, gsi_bool send) {
    GTI2DumpCallback callback;

    if (socket == NULL) {
        return gsi_true;
    }
    if (send) {
        callback = socket->onSendDump;
    } else {
        callback = socket->onReceiveDump;
    }
    if (callback == NULL) {
        return gsi_true;
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
        gti2CloseSocket(socket);
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gti2UnrecognizedMessageCallback(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port, const void *message, int length,
                                         gsi_bool *handled) {
    *handled = gsi_false;
    if (socket == NULL) {
        return gsi_true;
    }
    if (socket->onUnrecognizedMessage == NULL) {
        return gsi_true;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    socket->callbackLevel++;
    *handled = socket->onUnrecognizedMessage(socket, ip, port, message, length);
    socket->callbackLevel--;
    if (socket->close && socket->callbackLevel == 0) {
        gti2CloseSocket(socket);
        return gsi_false;
    }
    return gsi_true;
}
