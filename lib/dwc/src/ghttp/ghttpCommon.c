#include "ghttp_internal.h"

// ghttpCommon: the locks, and the reading and writing of a connection's socket, through its encryption if it has one

// How a request's receiving is throttled, in milliseconds and in bytes (ghttpStartup sets them again), and the proxy
// that the requests go through, which nothing in this build sets
int ghiThrottleInterval = 250;
int ghiThrottleBytes = 125;
char *ghiProxyAddress;
u16 ghiProxyPort;

// The locks do nothing in this build
void ghiCreateLock(void) {
}

void ghiFreeLock(void) {
}

void ghiLock(void) {
}

void ghiUnlock(void) {
}

// Decrypts what was received into the connection's decode buffer, to its receive buffer
BOOL ghiDecryptReceivedData(GHIConnection *connection) {
    int inSize = 0;
    int outSize = 0;
    int result;

    do {
        do {
            const char *in = connection->decodeBuffer.data + connection->decodeBuffer.pos;
            char *out;

            inSize = connection->decodeBuffer.length - connection->decodeBuffer.pos;
            out = connection->recvBuffer.data + connection->recvBuffer.length;
            outSize = connection->recvBuffer.size - connection->recvBuffer.length;
            result = connection->encryptor.decrypt(connection, &connection->encryptor, in, &inSize, out, &outSize);
            if (result == 2) {
                if (!ghiResizeBuffer(&connection->recvBuffer, connection->recvBuffer.sizeIncrement)) {
                    return FALSE;
                }
            } else if (result == 3) {
                return FALSE;
            }
        } while (result == 2 && outSize == 0);

        if (inSize > connection->decodeBuffer.length) {
            func_ov011_0216cca8(8, 4, 1,
                                "ghiDecryptReceivedData read past the end of connection->decodeBuffer! (%d\\%d bytes)\r\n",
                                inSize, connection->decodeBuffer.length);
            return FALSE;
        }
        connection->decodeBuffer.pos += inSize;
        connection->recvBuffer.length += outSize;
    } while (outSize > 0);

    if (connection->decodeBuffer.pos > 0xff) {
        int remaining = connection->decodeBuffer.length - connection->decodeBuffer.pos;

        if (remaining == 0) {
            ghiResetBuffer(&connection->decodeBuffer);
        } else {
            memmove(connection->decodeBuffer.data, connection->decodeBuffer.data + connection->decodeBuffer.pos,
                    remaining);
            connection->decodeBuffer.pos = 0;
            connection->decodeBuffer.length = remaining;
        }
    }
    return TRUE;
}

// Reads what the socket has, up to size - 1 bytes, into buffer and sets size to the count. Returns 0 for data, 1 for
// none yet, 2 for the end of the connection and 3 for an error
int ghiDoReceive(GHIConnection *connection, char *buffer, int *size) {
    int max = *size - 1;
    int received;
    int error;

    if (connection->throttle != 0) {
        u32 now = func_ov011_0216e324();

        if (now < connection->lastThrottleTime + ghiThrottleInterval) {
            return 1;
        }
        connection->lastThrottleTime = now;
        if (max >= ghiThrottleBytes) {
            max = ghiThrottleBytes;
        }
    }

    if (connection->encryptor.engine != 0 && connection->encryptor.active == 1 &&
        connection->encryptor.established == 1) {
        int length = max;

        if (ghiEncryptorRecv(connection, &connection->encryptor, buffer, &length) == 1) {
            if (length == -1) {
                return 1;
            }
            received = length;
        } else {
            connection->completed = TRUE;
            connection->result = 5;
            connection->socketError = 0;
            connection->closing = TRUE;
            return 3;
        }
    } else {
        received = func_ov011_0216def0(connection->socket, buffer, max, 0);
        if (received == -1) {
            error = func_ov011_0216dfa8(connection->socket);
            if (error == -56) {
                connection->closing = TRUE;
                return 2;
            }
            if (error == -6 || error == -26 || error == -76) {
                return 1;
            }
            connection->completed = TRUE;
            connection->result = 5;
            connection->socketError = error;
            connection->closing = TRUE;
            return 3;
        }
    }

    if (received == 0) {
        connection->closing = TRUE;
        return 2;
    }
    buffer[received] = '\0';
    *size = received;
    func_ov011_0216cca8(8, 0, 0x20, "Received %d bytes\n", received);
    return 0;
}

// Writes to the socket. Returns the count sent (0 if it would block), -1 for an error and -2 if the encryption wants
// the data again later
int ghiDoSend(GHIConnection *connection, const char *data, int size) {
    int sent;

    if (data == NULL || size == 0) {
        return 0;
    }

    if (connection->encryptor.engine != 0 && connection->encryptor.active == 1 &&
        connection->encryptor.established == 1) {
        int count = 0;

        if (ghiEncryptorSend(connection, &connection->encryptor, data, size, &count) == 1) {
            if (count == -1) {
                return -2;
            }
            sent = count;
        } else {
            connection->completed = TRUE;
            connection->result = 5;
            connection->socketError = 0;
            return -1;
        }
    } else {
        sent = func_ov011_0216df24(connection->socket, data, size, 0);
        if (sent == -1) {
            int error = func_ov011_0216dfa8(connection->socket);

            if (error == -6 || error == -26 || error == -76) {
                return 0;
            }
            connection->completed = TRUE;
            connection->result = 5;
            connection->socketError = error;
            return -1;
        }
    }

    if (connection->state == 6 && !connection->postingState.isXml) {
        connection->postingState.bytesPosted += sent;
    }
    return sent;
}

// Sends the data if nothing waits to be sent before it, and buffers what could not be. Returns 0 for an error, 1 if
// all was sent and 2 if some is buffered
int ghiTrySendThenBuffer(GHIConnection *connection, const char *data, int size) {
    int sent = 0;

    if (connection->encryptor.engine != 0 && connection->encryptor.active == 1 && connection->encryptor.mode == 1) {
        if (!ghiEncryptDataToBuffer(&connection->sendBuffer, data, size)) {
            return 0;
        }
        if (!ghiSendBufferedData(connection)) {
            return 0;
        }
        if (connection->sendBuffer.pos >= connection->sendBuffer.length) {
            ghiResetBuffer(&connection->sendBuffer);
            return 1;
        }
        return 2;
    }

    if (connection->sendBuffer.pos >= connection->sendBuffer.length) {
        sent = ghiDoSend(connection, data, size);
        if (sent == -2) {
            return 2;
        } else if (sent == -1) {
            return 0;
        } else if (sent == size) {
            return 1;
        }
    }
    if (!ghiAppendDataToBuffer(&connection->sendBuffer, data + sent, size - sent)) {
        return 0;
    }
    return 2;
}
