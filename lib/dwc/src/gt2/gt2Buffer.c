#include "dwc/gt2Buffer.h"
#include "dwc/ext.h"

// GameSpy GT2's byte buffers (gt2Buffer.c)

BOOL gti2AllocateBuffer(GTI2Buffer *buffer, int capacity) {
    buffer->data = gsimalloc(capacity);
    if (buffer->data == NULL) {
        return FALSE;
    }
    buffer->capacity = capacity;
    return TRUE;
}

int gti2GetBufferFreeSpace(GTI2Buffer *buffer) {
    return buffer->capacity - buffer->length;
}

void gti2BufferWriteByte(GTI2Buffer *buffer, u8 value) {
    buffer->data[buffer->length++] = value;
}

void gti2BufferWriteUShort(GTI2Buffer *buffer, u16 value) {
    buffer->data[buffer->length++] = value >> 8;
    buffer->data[buffer->length++] = value;
}

void gti2BufferWriteData(GTI2Buffer *buffer, const void *data, int size) {
    if (data == NULL || size == 0) {
        return;
    }
    if (size == -1) {
        size = strlen(data);
    }
    memcpy(buffer->data + buffer->length, data, size);
    buffer->length += size;
}

void gti2BufferShorten(GTI2Buffer *buffer, int offset, int size) {
    if (offset == -1) {
        offset = buffer->length - size;
    }
    memmove(buffer->data + offset, buffer->data + offset + size, buffer->length - offset - size);
    buffer->length -= size;
}
