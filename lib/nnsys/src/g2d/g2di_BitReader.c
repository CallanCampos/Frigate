#include "g2di_BitReader.h"

// NitroSystem's glyph bit reader, which the character canvas reads glyphs with. The file name is NitroSystem's, as
// ntrtwl's reconstruction of NitroSystem has it: the function sits alone after the font loader, before the UTF-16
// splitter, and only g2d_CharCanvas.c calls it

u32 NNSi_G2dBitReaderRead(NNSiG2dBitReader *reader, int nBits) {
    u32 ret = reader->bits;

    if (reader->availableBits < nBits) {
        const int bitsNeeded = nBits - reader->availableBits;

        ret <<= bitsNeeded;
        reader->bits = *reader->src++;
        reader->availableBits = 8;
        ret |= NNSi_G2dBitReaderRead(reader, bitsNeeded);
    } else {
        ret >>= reader->availableBits - nBits;
        reader->availableBits -= nBits;
    }
    return ret & (0xff >> (8 - nBits));
}
