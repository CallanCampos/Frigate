#include "dwc/gt2Buffer.h"
#include "dwc/ext.h"

// GameSpy GT2's byte buffers (gt2Buffer.c). The functions' names, from pret/pokerevo's Wii build, are in
// dwc/gt2Buffer.h

BOOL func_ov011_0217c944(GTI2Buffer *buffer, int capacity) {
    buffer->data = gsimalloc(capacity);
    if (buffer->data == NULL) {
        return FALSE;
    }
    buffer->capacity = capacity;
    return TRUE;
}

int func_ov011_0217c960(GTI2Buffer *buffer) {
    return buffer->capacity - buffer->length;
}

void func_ov011_0217c968(GTI2Buffer *buffer, u8 value) {
    buffer->data[buffer->length++] = value;
}

void func_ov011_0217c974(GTI2Buffer *buffer, u16 value) {
    buffer->data[buffer->length++] = value >> 8;
    buffer->data[buffer->length++] = value;
}

void func_ov011_0217c990(GTI2Buffer *buffer, const void *data, int size) {
    if (data == NULL || size == 0) {
        return;
    }
    if (size == -1) {
        size = strlen(data);
    }
    memcpy(buffer->data + buffer->length, data, size);
    buffer->length += size;
}

void func_ov011_0217c9c8(GTI2Buffer *buffer, int offset, int size) {
    if (offset == -1) {
        offset = buffer->length - size;
    }
    memmove(buffer->data + offset, buffer->data + offset + size, buffer->length - offset - size);
    buffer->length -= size;
}
