// Host version of NitroSDK's mi_uncompress.c (lib/nitro/src/mi/mi_uncompress.c): MI_UncompressLZ8, the LZ77
// decompressor, which the SDK wrote in assembly. It accepts the DS's LZ77 format (type 0x10) and the extended one,
// whose copy lengths run to 0x10110 (type 0x11).
#include "nitro/mi.h"

// Decompresses src to dest. src starts with a word: the format in its low byte and the decompressed size above it.
// The original tells the formats apart by the low nibble only: zero is the plain one, anything else the extended one.
// It stores a byte at a time (with SWPB, which the ARM9 can write VRAM with) in the order the stream gives, so a
// stream decompressed in place reads what it has already written as the original does. A copy that runs past the size
// isn't cut short: the size only stops the decoding once it is reached
void MI_UncompressLZ8(const void *src, void *dest) {
    const u8 *s = src;
    u8 *d = dest;
    u32 header = s[0] | s[1] << 8 | s[2] << 16 | (u32)s[3] << 24;
    s32 remaining = header >> 8;
    BOOL extended = (header & 0xf) != 0;
    u32 flags;
    u32 block;
    u32 length;
    const u8 *from;

    s += 4;
    while (remaining > 0) {
        // A flag byte tells the next eight blocks apart, the top bit first: a literal byte, or a copy of bytes already
        // written
        flags = *s++;
        for (block = 0; block < 8 && remaining > 0; block++, flags <<= 1) {
            if (!(flags & 0x80)) {
                *d++ = *s++;
                remaining--;
                continue;
            }
            // The length is a base plus the high nibble of the byte before the offset. The base is 3 in the plain
            // format. In the extended one it is 1 when the first byte's high nibble is 2 or more, and otherwise (0
            // or 1) the length takes one or two more bytes, with a base of 0x11 or 0x111
            if (!extended) {
                length = 3;
            } else if (s[0] & 0xe0) {
                length = 1;
            } else if (!(s[0] & 0x10)) {
                length = 0x11 + ((s[0] & 0xf) << 4);
                s += 1;
            } else {
                length = 0x111 + ((s[0] & 0xf) << 12) + (s[1] << 4);
                s += 2;
            }
            length += s[0] >> 4;
            // The offset back from the next byte to write, less one: the rest of that byte and the next
            from = d - (((s[0] & 0xf) << 8 | s[1]) + 1);
            s += 2;
            remaining -= (s32)length;
            for (; length != 0; length--) {
                *d++ = *from++;
            }
        }
    }
}
