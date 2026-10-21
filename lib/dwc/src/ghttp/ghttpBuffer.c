#include "ghttp_internal.h"

// ghttpBuffer: the buffers that a connection reads into and writes from

gsi_bool ghiResizeBuffer(GHIBuffer *buffer, int count) {
    int newSize;
    char *newData;

    if (buffer == NULL) {
        return gsi_false;
    }
    if (count <= 0) {
        return gsi_false;
    }
    newSize = buffer->size + count;
    newData = gsirealloc(buffer->data, newSize);
    if (newData == NULL) {
        return gsi_false;
    }
    buffer->data = newData;
    buffer->size = newSize;
    return gsi_true;
}

gsi_bool ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement) {
    if (connection == NULL) {
        return gsi_false;
    }
    if (buffer == NULL) {
        return gsi_false;
    }
    if (initialSize <= 0) {
        return gsi_false;
    }
    if (sizeIncrement <= 0) {
        return gsi_false;
    }
    buffer->connection = connection;
    buffer->data = NULL;
    buffer->size = 0;
    buffer->length = 0;
    buffer->pos = 0;
    buffer->sizeIncrement = sizeIncrement;
    buffer->fixed = gsi_false;
    buffer->dontFree = gsi_false;
    buffer->readOnly = gsi_false;
    if (!ghiResizeBuffer(buffer, initialSize)) {
        return gsi_false;
    }
    buffer->data[0] = 0;
    return gsi_true;
}

gsi_bool ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *data, int size) {
    if (connection == NULL) {
        return gsi_false;
    }
    if (buffer == NULL) {
        return gsi_false;
    }
    if (data == NULL) {
        return gsi_false;
    }
    if (size <= 0) {
        return gsi_false;
    }
    buffer->connection = connection;
    buffer->size = size;
    buffer->data = data;
    buffer->length = 0;
    buffer->pos = 0;
    buffer->sizeIncrement = 0;
    buffer->fixed = gsi_true;
    buffer->dontFree = gsi_true;
    buffer->readOnly = gsi_false;
    data[0] = 0;
    return gsi_true;
}

void ghiFreeBuffer(GHIBuffer *buffer) {
    if (buffer != NULL && buffer->data != NULL) {
        if (!buffer->dontFree) {
            gsifree(buffer->data);
        }
        memset(buffer, 0, sizeof(GHIBuffer));
    }
}

// Adds data to the end of a buffer (size 0 is for a text), making the buffer bigger if it can. When it can't, the
// connection ends with the result 2 (a fixed buffer is full) or 1 (no memory)
gsi_bool ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int size) {
    int newLength;

    if (buffer == NULL) {
        return gsi_false;
    }
    if (data == NULL) {
        return gsi_false;
    }
    if (size < 0) {
        return gsi_false;
    }
    if (buffer->readOnly) {
        return gsi_false;
    }
    if (size == 0) {
        size = strlen(data);
    }
    newLength = buffer->length + size;
    while (newLength >= buffer->size) {
        if (buffer->fixed) {
            buffer->connection->completed = gsi_true;
            buffer->connection->result = 2;
            return gsi_false;
        }
        if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
            buffer->connection->completed = gsi_true;
            buffer->connection->result = 1;
            return gsi_false;
        }
    }
    memcpy(buffer->data + buffer->length, data, size);
    buffer->length = newLength;
    buffer->data[newLength] = 0;
    return gsi_true;
}

// The same through the connection's encryption, if it has one active
gsi_bool ghiEncryptDataToBuffer(GHIBuffer *buffer, const char *data, int size) {
    int offset = 0;
    int available = 0;

    if (buffer == NULL) {
        return gsi_false;
    }
    if (data == NULL) {
        return gsi_false;
    }
    if (size < 0) {
        return gsi_false;
    }
    if (buffer->readOnly) {
        return gsi_false;
    }
    if (buffer->connection->encryptor.engine == 0 || !buffer->connection->encryptor.active) {
        return ghiAppendDataToBuffer(buffer, data, size);
    }
    if (size == 0) {
        size = strlen(data);
    }
    if (size == 0) {
        return gsi_true;
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
                return gsi_false;
            }
            available = buffer->size - buffer->length;
        } else if (result == 1) {
            offset += chunk;
            buffer->length = buffer->size - available;
        } else {
            gsDebugFormat(8, 4, 2, "ghiEncryptDataToBuffer encountered unhandled return code: %d\r\n", result);
            return gsi_false;
        }
    } while (offset < size);
    return gsi_true;
}

// Adds a header field: name, ": ", value and a line end
gsi_bool ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value) {
    gsi_bool result = gsi_false;

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
        result = gsi_true;
    }
    return result;
}

gsi_bool ghiAppendCharToBuffer(GHIBuffer *buffer, int c) {
    if (buffer == NULL) {
        return gsi_false;
    }
    if (buffer->readOnly) {
        return gsi_false;
    }
    if (buffer->length + 1 >= buffer->size) {
        if (buffer->fixed) {
            buffer->connection->completed = gsi_true;
            buffer->connection->result = 2;
            return gsi_false;
        }
        if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
            buffer->connection->completed = gsi_true;
            buffer->connection->result = 1;
            return gsi_false;
        }
    }
    buffer->data[buffer->length] = c;
    buffer->length++;
    buffer->data[buffer->length] = 0;
    return gsi_true;
}

gsi_bool ghiAppendIntToBuffer(GHIBuffer *buffer, int value) {
    char text[16];

    OS_SPrintf(text, "%d", value);
    return ghiAppendDataToBuffer(buffer, text, 0);
}

void ghiResetBuffer(GHIBuffer *buffer) {
    buffer->length = 0;
    buffer->pos = 0;
    if (!buffer->readOnly) {
        buffer->data[0] = 0;
    }
}

// Sends what the send buffer has, as far as the socket takes it. Returns gsi_false if the connection broke
gsi_bool ghiSendBufferedData(GHIConnection *connection) {
    int ready;
    int failed;

    do {
        int sent;

        if (connection->encryptor.engine == 0 || connection->encryptor.mode == 1) {
            int result = GSISocketSelect(connection->socket, 0, &ready, &failed);

            if (result == -1 || (result == 1 && failed != 0)) {
                connection->completed = gsi_true;
                connection->result = 5;
                if (result == -1) {
                    connection->socketError = GOAGetLastError(connection->socket);
                } else {
                    connection->socketError = 0;
                }
                return gsi_false;
            }
            if (result < 1 || ready == 0) {
                return gsi_true;
            }
        }
        sent = ghiDoSend(connection, connection->sendBuffer.data + connection->sendBuffer.pos,
                         connection->sendBuffer.length - connection->sendBuffer.pos);
        switch (sent) {
        case -1:
            return gsi_false;
        case -2:
            return gsi_true;
        }
        connection->sendBuffer.pos += sent;
    } while (connection->sendBuffer.pos < connection->sendBuffer.length);
    return gsi_true;
}
