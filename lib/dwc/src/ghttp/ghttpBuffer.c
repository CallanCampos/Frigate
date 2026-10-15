#include "ghttp_internal.h"

// ghttpBuffer: the buffers that a connection reads into and writes from

BOOL ghiResizeBuffer(GHIBuffer *buffer, int count) {
    int newSize;
    char *newData;

    if (buffer == NULL) {
        return FALSE;
    }
    if (count <= 0) {
        return FALSE;
    }
    newSize = buffer->size + count;
    newData = func_ov011_0216ddec(buffer->data, newSize);
    if (newData == NULL) {
        return FALSE;
    }
    buffer->data = newData;
    buffer->size = newSize;
    return TRUE;
}

BOOL ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement) {
    if (connection == NULL) {
        return FALSE;
    }
    if (buffer == NULL) {
        return FALSE;
    }
    if (initialSize <= 0) {
        return FALSE;
    }
    if (sizeIncrement <= 0) {
        return FALSE;
    }
    buffer->connection = connection;
    buffer->data = NULL;
    buffer->size = 0;
    buffer->length = 0;
    buffer->pos = 0;
    buffer->sizeIncrement = sizeIncrement;
    buffer->fixed = FALSE;
    buffer->dontFree = FALSE;
    buffer->readOnly = FALSE;
    if (!ghiResizeBuffer(buffer, initialSize)) {
        return FALSE;
    }
    buffer->data[0] = 0;
    return TRUE;
}

BOOL ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *data, int size) {
    if (connection == NULL) {
        return FALSE;
    }
    if (buffer == NULL) {
        return FALSE;
    }
    if (data == NULL) {
        return FALSE;
    }
    if (size <= 0) {
        return FALSE;
    }
    buffer->connection = connection;
    buffer->size = size;
    buffer->data = data;
    buffer->length = 0;
    buffer->pos = 0;
    buffer->sizeIncrement = 0;
    buffer->fixed = TRUE;
    buffer->dontFree = TRUE;
    buffer->readOnly = FALSE;
    data[0] = 0;
    return TRUE;
}

void ghiFreeBuffer(GHIBuffer *buffer) {
    if (buffer != NULL && buffer->data != NULL) {
        if (!buffer->dontFree) {
            func_ov011_0216ddfc(buffer->data);
        }
        memset(buffer, 0, sizeof(GHIBuffer));
    }
}

// Adds data to the end of a buffer (size 0 is for a text), making the buffer bigger if it can. When it can't, the
// connection ends with the result 2 (a fixed buffer is full) or 1 (no memory)
BOOL ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int size) {
    int newLength;

    if (buffer == NULL) {
        return FALSE;
    }
    if (data == NULL) {
        return FALSE;
    }
    if (size < 0) {
        return FALSE;
    }
    if (buffer->readOnly) {
        return FALSE;
    }
    if (size == 0) {
        size = strlen(data);
    }
    newLength = buffer->length + size;
    while (newLength >= buffer->size) {
        if (buffer->fixed) {
            buffer->connection->completed = TRUE;
            buffer->connection->result = 2;
            return FALSE;
        }
        if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
            buffer->connection->completed = TRUE;
            buffer->connection->result = 1;
            return FALSE;
        }
    }
    memcpy(buffer->data + buffer->length, data, size);
    buffer->length = newLength;
    buffer->data[newLength] = 0;
    return TRUE;
}

// The same through the connection's encryption, if it has one active
BOOL ghiEncryptDataToBuffer(GHIBuffer *buffer, const char *data, int size) {
    int offset = 0;
    int available = 0;

    if (buffer == NULL) {
        return FALSE;
    }
    if (data == NULL) {
        return FALSE;
    }
    if (size < 0) {
        return FALSE;
    }
    if (buffer->readOnly) {
        return FALSE;
    }
    if (buffer->connection->encryptor.engine == 0 || !buffer->connection->encryptor.active) {
        return ghiAppendDataToBuffer(buffer, data, size);
    }
    if (size == 0) {
        size = strlen(data);
    }
    if (size == 0) {
        return TRUE;
    }
    available = buffer->size - buffer->length;
    do {
        int chunk = (size < GHI_ENCRYPT_CHUNK_SIZE) ? size : GHI_ENCRYPT_CHUNK_SIZE;
        // BUG: the piece is size bytes, not chunk, so the data is encrypted again from the second piece on; the offset
        // advances by a chunk
#ifdef BUGFIX
        int result = buffer->connection->encryptor.encrypt(buffer->connection, &buffer->connection->encryptor,
                                                           data + offset, size - offset < chunk ? size - offset : chunk,
                                                           buffer->data + buffer->length, &available);
#else
        int result =
            buffer->connection->encryptor.encrypt(buffer->connection, &buffer->connection->encryptor, data + offset,
                                                  size, buffer->data + buffer->length, &available);
#endif

        if (result == 2) {
            if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
                return FALSE;
            }
            available = buffer->size - buffer->length;
        } else if (result == 1) {
            offset += chunk;
            buffer->length = buffer->size - available;
        } else {
            func_ov011_0216cca8(8, 4, 2, "ghiEncryptDataToBuffer encountered unhandled return code: %d\r\n", result);
            return FALSE;
        }
    } while (offset < size);
    return TRUE;
}

// Adds a header field: name, ": ", value and a line end
BOOL ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value) {
    BOOL result = FALSE;

    if (!ghiAppendDataToBuffer(buffer, name, 0)) {
        return result;
    }
    if (!ghiAppendDataToBuffer(buffer, ": ", 2)) {
        return result;
    }
    if (!ghiAppendDataToBuffer(buffer, value, 0)) {
        return result;
    }
    if (ghiAppendDataToBuffer(buffer, "\r\n", 2)) {
        result = TRUE;
    }
    return result;
}

BOOL ghiAppendCharToBuffer(GHIBuffer *buffer, int c) {
    if (buffer == NULL) {
        return FALSE;
    }
    if (buffer->readOnly) {
        return FALSE;
    }
    if (buffer->length + 1 >= buffer->size) {
        if (buffer->fixed) {
            buffer->connection->completed = TRUE;
            buffer->connection->result = 2;
            return FALSE;
        }
        if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
            buffer->connection->completed = TRUE;
            buffer->connection->result = 1;
            return FALSE;
        }
    }
    buffer->data[buffer->length] = c;
    buffer->length++;
    buffer->data[buffer->length] = 0;
    return TRUE;
}

BOOL ghiAppendIntToBuffer(GHIBuffer *buffer, int value) {
    char text[16];

    func_0207a290(text, "%d", value);
    return ghiAppendDataToBuffer(buffer, text, 0);
}

void ghiResetBuffer(GHIBuffer *buffer) {
    buffer->length = 0;
    buffer->pos = 0;
    if (!buffer->readOnly) {
        buffer->data[0] = 0;
    }
}

// Sends what the send buffer has, as far as the socket takes it. Returns FALSE if the connection broke
BOOL ghiSendBufferedData(GHIConnection *connection) {
    int ready;
    int failed;

    do {
        int sent;

        if (connection->encryptor.engine == 0 || connection->encryptor.mode == 1) {
            int result = func_ov011_0216dfb4(connection->socket, 0, &ready, &failed);

            if (result == -1 || (result == 1 && failed != 0)) {
                connection->completed = TRUE;
                connection->result = 5;
                if (result == -1) {
                    connection->socketError = func_ov011_0216dfa8(connection->socket);
                } else {
                    connection->socketError = 0;
                }
                return FALSE;
            }
            if (result < 1 || ready == 0) {
                return TRUE;
            }
        }
        sent = ghiDoSend(connection, connection->sendBuffer.data + connection->sendBuffer.pos,
                         connection->sendBuffer.length - connection->sendBuffer.pos);
        switch (sent) {
        case -1:
            return FALSE;
        case -2:
            return TRUE;
        }
        connection->sendBuffer.pos += sent;
    } while (connection->sendBuffer.pos < connection->sendBuffer.length);
    return TRUE;
}
