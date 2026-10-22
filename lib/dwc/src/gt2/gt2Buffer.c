#include "dwc/gt2Buffer.h"
#include "dwc/ext.h"

// GameSpy GT2's byte buffers (gt2Buffer.c)

GT2Bool gti2AllocateBuffer(GTI2Buffer *buffer, int capacity) {
    buffer->buffer = gsimalloc(capacity);
    if (buffer->buffer == NULL) {
        return gsi_false;
    }
    buffer->size = capacity;
    return gsi_true;
}

int gti2GetBufferFreeSpace(const GTI2Buffer *buffer) {
    return buffer->size - buffer->len;
}

void gti2BufferWriteByte(GTI2Buffer *buffer, GT2Byte value) {
    buffer->buffer[buffer->len++] = value;
}

void gti2BufferWriteUShort(GTI2Buffer *buffer, unsigned short value) {
    buffer->buffer[buffer->len++] = value >> 8;
    buffer->buffer[buffer->len++] = value;
}

void gti2BufferWriteData(GTI2Buffer *buffer, const GT2Byte *data, int size) {
    if (data == NULL || size == 0) {
        return;
    }
    if (size == -1) {
        size = strlen(data);
    }
    memcpy(buffer->buffer + buffer->len, data, size);
    buffer->len += size;
}

void gti2BufferShorten(GTI2Buffer *buffer, int offset, int size) {
    if (offset == -1) {
        offset = buffer->len - size;
    }
    memmove(buffer->buffer + offset, buffer->buffer + offset + size, buffer->len - offset - size);
    buffer->len -= size;
}
