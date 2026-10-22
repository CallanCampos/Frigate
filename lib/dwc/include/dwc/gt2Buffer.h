#ifndef POKEBW2_DWC_GT2BUFFER_H
#define POKEBW2_DWC_GT2BUFFER_H

#include "dwc/gt2Main.h"

// The byte buffers of GameSpy's GT2 (gt2Buffer.c), in overlay 11, which a connection keeps its incoming and outgoing
// messages in. The ROM has no names for the file or its functions: the file is named after GameSpy's, as
// pret/pokerevo's Wii build has it (asm/SDK/DWC/gamespy/gt2Buffer.s), and so are its functions, after GameSpy's
// gt2Buffer.c

// The buffer itself, GTI2Buffer, is in dwc/gt2Main.h

// GT2False when out of memory
GT2Bool gti2AllocateBuffer(GTI2Buffer *buffer, int capacity);
int gti2GetBufferFreeSpace(const GTI2Buffer *buffer);
// gti2BufferWriteUShort writes big-endian, and a size of -1 has gti2BufferWriteData write a text
void gti2BufferWriteByte(GTI2Buffer *buffer, GT2Byte value);
void gti2BufferWriteUShort(GTI2Buffer *buffer, unsigned short value);
void gti2BufferWriteData(GTI2Buffer *buffer, const GT2Byte *data, int size);
// Removes size bytes at offset, or the last size bytes for an offset of -1
void gti2BufferShorten(GTI2Buffer *buffer, int offset, int size);

#endif // POKEBW2_DWC_GT2BUFFER_H
