#ifndef POKEBW2_DWC_GT2BUFFER_H
#define POKEBW2_DWC_GT2BUFFER_H

#include "dwc/nonport.h"

// The byte buffers of GameSpy's GT2 (gt2Buffer.c), in overlay 11, which a connection keeps its incoming and outgoing
// messages in. The ROM has no names for the file or its functions: the file is named after GameSpy's, as
// pret/pokerevo's Wii build has it (asm/SDK/DWC/gamespy/gt2Buffer.s), and so are its functions, after GameSpy's
// gt2Buffer.c

// length bytes of capacity are used
typedef struct GTI2Buffer {
    gsi_u8 *data;
    int capacity;
    int length;
} GTI2Buffer;

// gsi_false when out of memory
gsi_bool gti2AllocateBuffer(GTI2Buffer *buffer, int capacity);
int gti2GetBufferFreeSpace(GTI2Buffer *buffer);
// gti2BufferWriteUShort writes big-endian, and a size of -1 has gti2BufferWriteData write a text
void gti2BufferWriteByte(GTI2Buffer *buffer, gsi_u8 value);
void gti2BufferWriteUShort(GTI2Buffer *buffer, gsi_u16 value);
void gti2BufferWriteData(GTI2Buffer *buffer, const void *data, int size);
// Removes size bytes at offset, or the last size bytes for an offset of -1
void gti2BufferShorten(GTI2Buffer *buffer, int offset, int size);

#endif // POKEBW2_DWC_GT2BUFFER_H
