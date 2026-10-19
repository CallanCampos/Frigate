#ifndef POKEBW2_DWC_GT2BUFFER_H
#define POKEBW2_DWC_GT2BUFFER_H

#include "types.h"

// The byte buffers of GameSpy's GT2 (gt2Buffer.c), in overlay 11, which a connection keeps its incoming and outgoing
// messages in. The ROM has no names for the file or its functions: the file is named after GameSpy's, as
// pret/pokerevo's Wii build has it (asm/SDK/DWC/gamespy/gt2Buffer.s), and so are the functions in the comments beside
// them, while the symbols keep their addresses

// length bytes of capacity are used
typedef struct GTI2Buffer {
    u8 *data;
    int capacity;
    int length;
} GTI2Buffer;

// gti2AllocateBuffer: FALSE when out of memory
BOOL func_ov011_0217c944(GTI2Buffer *buffer, int capacity);
// gti2GetBufferFreeSpace
int func_ov011_0217c960(GTI2Buffer *buffer);
// gti2BufferWriteByte, gti2BufferWriteUShort (big-endian) and gti2BufferWriteData (a size of -1 writes a text)
void func_ov011_0217c968(GTI2Buffer *buffer, u8 value);
void func_ov011_0217c974(GTI2Buffer *buffer, u16 value);
void func_ov011_0217c990(GTI2Buffer *buffer, const void *data, int size);
// gti2BufferShorten: removes size bytes at offset, or the last size bytes for an offset of -1
void func_ov011_0217c9c8(GTI2Buffer *buffer, int offset, int size);

#endif // POKEBW2_DWC_GT2BUFFER_H
