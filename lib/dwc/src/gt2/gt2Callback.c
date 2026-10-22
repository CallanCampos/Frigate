#include "dwc/gt2Callback.h"
#include "dwc/ext.h"
#include "dwc/gt2Socket.h"

// GameSpy GT2's calls into the app's callbacks (gt2Callback.c)

GT2Bool gti2SocketErrorCallback(GT2Socket socket) {
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

GT2Bool gti2ConnectAttemptCallback(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port,
                                   int latency, GT2Byte *message, int length) {
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

GT2Bool gti2ConnectedCallback(GT2Connection connection, GT2Result result, GT2Byte *message, int length) {
    if (connection == NULL) {
        return gsi_true;
    }
    connection->result = result;
    if (connection->callbacks.connected == NULL) {
        return gsi_true;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.connected(connection, result, message, length);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

GT2Bool gti2ReceivedCallback(GT2Connection connection, GT2Byte *message, int length, GT2Bool reliable) {
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->callbacks.received == NULL) {
        return gsi_true;
    }
    if (length == 0 || message == NULL) {
        message = NULL;
        length = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.received(connection, message, length, reliable);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

GT2Bool gti2ClosedCallback(GT2Connection connection, GT2CloseReason reason) {
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->callbacks.closed == NULL) {
        return gsi_true;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.closed(connection, reason);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

GT2Bool gti2PingCallback(GT2Connection connection, int latency) {
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->callbacks.ping == NULL) {
        return gsi_true;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.ping(connection, latency);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    if (connection->socket->close && connection->socket->callbackLevel == 0) {
        gti2CloseSocket(connection->socket);
        return gsi_false;
    }
    return gsi_true;
}

GT2Bool gti2SendFilterCallback(GT2Connection connection, int filterId, const GT2Byte *message, int length,
                               GT2Bool reliable) {
    gt2SendFilterCallback *filter;

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

GT2Bool gti2ReceiveFilterCallback(GT2Connection connection, int filterId, GT2Byte *message, int length,
                                  GT2Bool reliable) {
    gt2ReceiveFilterCallback *filter;

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

GT2Bool gti2DumpCallback(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port,
                         GT2Bool reset, const GT2Byte *message, int length, GT2Bool send) {
    gt2DumpCallback callback;

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

GT2Bool gti2UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip, unsigned short port, GT2Byte *message,
                                        int length, GT2Bool *handled) {
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
