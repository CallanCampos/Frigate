// Host versions of NitroSDK's mi_memory.c (lib/nitro/src/mi/mi_memory.c): the CPU copies, fills and compare, which the
// SDK wrote in assembly. Each does what the original does on every input the original accepts: it writes the same
// bytes, rounds odd sizes the same way, depends on the pointers' alignment in the same way, and gives the same result
// where the source and the destination overlap, because it loads and stores in the same units and in the same order.
// A unit is one load and store of the original's (a byte, a halfword, a word, or an LDM and STM of several words),
// which loads all its bytes before it stores any, as memmove does.
//
// The original reads and rewrites bytes just outside the range in a few places: a halfword or word access that keeps
// a neighbouring byte stores back the value it has just read. That changes nothing, so these versions read and write
// only the bytes they copy or fill, and never a byte past either end that the original doesn't change.
//
// The halfword and word functions (MIi_*16, MIi_*32, the Fast ones and MI_Copy*B) take aligned pointers, as their
// names say; the ARM9 ignores the low bits of an unaligned address, which these versions don't reproduce.
//
// The SDK also gives MIi_CpuSend32 and MI_Copy*B a geometry command port to store to: &reg_G3X_GXFIFO, as NNS's
// command buffer and G3_MultMtx33 do. On the host a port is an entry of port/hal/hw's command log, which holds one
// word, so these functions hand the words to hw, which logs each one as the port's write.
#include "hal/hw/hw.h"
#include "nitro/mi.h"
#include <stdint.h>
#include <string.h>

static void StoreHalf(u8 *dest, u16 value) {
    dest[0] = (u8)value;
    dest[1] = (u8)(value >> 8);
}

static u32 LoadWord(const u8 *src) {
    return src[0] | src[1] << 8 | src[2] << 16 | (u32)src[3] << 24;
}

static void StoreWord(u8 *dest, u32 value) {
    dest[0] = (u8)value;
    dest[1] = (u8)(value >> 8);
    dest[2] = (u8)(value >> 16);
    dest[3] = (u8)(value >> 24);
}

// The loops of the halfword and word functions compare as signed numbers (the offset with the size, or the address
// with the end address, which comes to the same for a DS address and a size that fits in memory), so a size of 2 GiB
// or more, a negative s32, does nothing. A size that isn't a multiple of the unit is rounded up to one: the last unit
// goes past the end

// Fills size bytes with a halfword at a time
void MIi_CpuClear16(u16 value, void *dest, u32 size) {
    u8 *d = dest;
    u32 offset;

    if ((s32)size <= 0) {
        return;
    }
    for (offset = 0; offset < size; offset += 2) {
        StoreHalf(d + offset, value);
    }
}

// Copies size bytes a halfword at a time
void MIi_CpuCopy16(const void *src, void *dest, u32 size) {
    const u8 *s = src;
    u8 *d = dest;
    u32 offset;

    if ((s32)size <= 0) {
        return;
    }
    for (offset = 0; offset < size; offset += 2) {
        memmove(d + offset, s + offset, 2);
    }
}

// Fills size bytes with a word at a time
void MIi_CpuClear32(u32 value, void *dest, u32 size) {
    u8 *d = dest;
    u32 offset;

    if ((s32)size <= 0) {
        return;
    }
    for (offset = 0; offset < size; offset += 4) {
        StoreWord(d + offset, value);
    }
}

// Copies size bytes a word at a time
void MIi_CpuCopy32(const void *src, void *dest, u32 size) {
    const u8 *s = src;
    u8 *d = dest;
    u32 offset;

    if ((s32)size <= 0) {
        return;
    }
    for (offset = 0; offset < size; offset += 4) {
        memmove(d + offset, s + offset, 4);
    }
}

// Sends size bytes, a word at a time, to one address, such as the geometry FIFO, where every write is a command
void MIi_CpuSend32(const void *src, volatile void *dest, u32 size) {
    const u8 *s = src;
    // Each write to the port acts, so none may be merged or dropped
    volatile u32 *port = dest;
    u32 count = (s32)size <= 0 ? 0 : (size + 3) / 4;
    u32 i;

    if (port_hw_g3_send(dest, src, count, 0)) {
        return;
    }
    for (i = 0; i < count; i++) {
        *port = LoadWord(s + 4 * i);
    }
}

// Fills size bytes with a word, 32 bytes at a time while it can; the blocks only make it faster
void MIi_CpuClearFast(u32 value, void *dest, u32 size) {
    u8 *d = dest;
    u32 offset;

    if ((s32)size <= 0) {
        return;
    }
    for (offset = 0; offset < size; offset += 4) {
        StoreWord(d + offset, value);
    }
}

// Copies size bytes, 32 bytes at a time while it can, then a word at a time
void MIi_CpuCopyFast(const void *src, void *dest, u32 size) {
    const u8 *s = src;
    u8 *d = dest;
    u32 offset;

    if ((s32)size <= 0) {
        return;
    }
    for (offset = 0; offset < (size & ~31u); offset += 32) {
        memmove(d + offset, s + offset, 32);
    }
    for (; offset < size; offset += 4) {
        memmove(d + offset, s + offset, 4);
    }
}

// MI_Copy32B (swan's gfxUploadFogTable): copies 32 bytes, three words at a time, then two
void MI_Copy32B(const void *src, void *dest) {
    const u8 *s = src;
    u8 *d = dest;

    if (port_hw_g3_send(dest, src, 8, 4)) {
        return;
    }
    memmove(d, s, 12);
    memmove(d + 12, s + 12, 12);
    memmove(d + 24, s + 24, 8);
}

// MI_Copy36B: copies 36 bytes, a 3x3 matrix, three words at a time
void MI_Copy36B(const void *src, void *dest) {
    const u8 *s = src;
    u8 *d = dest;

    if (port_hw_g3_send(dest, src, 9, 4)) {
        return;
    }
    memmove(d, s, 12);
    memmove(d + 12, s + 12, 12);
    memmove(d + 24, s + 24, 12);
}

// MI_Copy48B: copies 48 bytes, a 4x3 matrix, three words at a time
void MI_Copy48B(const void *src, void *dest) {
    const u8 *s = src;
    u8 *d = dest;

    if (port_hw_g3_send(dest, src, 12, 4)) {
        return;
    }
    memmove(d, s, 12);
    memmove(d + 12, s + 12, 12);
    memmove(d + 24, s + 24, 12);
    memmove(d + 36, s + 36, 12);
}

// MI_Copy64B: copies 64 bytes, a 4x4 matrix, three words at a time, then four
void MI_Copy64B(const void *src, void *dest) {
    const u8 *s = src;
    u8 *d = dest;

    if (port_hw_g3_send(dest, src, 16, 4)) {
        return;
    }
    memmove(d, s, 12);
    memmove(d + 12, s + 12, 12);
    memmove(d + 24, s + 24, 12);
    memmove(d + 36, s + 36, 12);
    memmove(d + 48, s + 48, 16);
}

// MI_CpuFill8: fills size bytes with a byte. The original writes halfwords and words only, since the ARM9 can't write
// a byte to VRAM, and makes them by ORing the SDK's byte-sized value with itself shifted.
void MI_CpuFill8(void *dest, u8 value, u32 size) {
    u8 *d = dest;
    u16 halfword;
    u32 word;

    if (size == 0) {
        return;
    }
    if ((uintptr_t)d & 1) {
        *d++ = (u8)value;
        if (--size == 0) {
            return;
        }
    }
    if (size >= 2) {
        halfword = (u16)(value | value << 8);
        if ((uintptr_t)d & 2) {
            StoreHalf(d, halfword);
            d += 2;
            size -= 2;
            if (size == 0) {
                return;
            }
        }
        // The SDK byte argument promotes to signed int; widen before the high-half shift.
        word = (u32)halfword | (u32)halfword << 16;
        for (; size >= 4; size -= 4) {
            StoreWord(d, word);
            d += 4;
        }
        if (size & 2) {
            StoreHalf(d, halfword);
            d += 2;
        }
    }
    if (size & 1) {
        *d = (u8)value;
    }
}

// MI_CpuFill: fills size bytes with a byte. Up to 8 bytes are stored one at a time. More are a byte and a halfword as
// needed to reach a word boundary, then words, then bytes; the original also stores a word to reach an 8-byte boundary
// and stores words 32 bytes at a time, which store the same words. Unlike the byte-sized MI_CpuFill8, bits above its
// low byte reach the halfword's and the words' other bytes
void MI_CpuFill(void *dest, u32 value, u32 size) {
    u8 *d = dest;
    u32 word;

    if (size <= 8) {
        for (; size != 0; size--) {
            *d++ = (u8)value;
        }
        return;
    }
    word = value | value << 8;
    word |= word << 16;
    if ((uintptr_t)d & 1) {
        *d++ = (u8)word;
        size -= 1;
    }
    if ((uintptr_t)d & 2) {
        StoreHalf(d, (u16)word);
        d += 2;
        size -= 2;
    }
    for (; size >= 4; size -= 4) {
        StoreWord(d, word);
        d += 4;
    }
    for (; size != 0; size--) {
        *d++ = (u8)word;
    }
}

// MI_CpuCopy8: copies size bytes, writing halfwords and words only (see MI_CpuFill8), whatever the alignment of the two
// sides
void MI_CpuCopy8(const void *src, void *dest, u32 size) {
    const u8 *s = src;
    u8 *d = dest;
    u8 carry;
    u8 low;
    u8 high;

    if (size == 0) {
        return;
    }
    // An odd destination: its first byte, stored as a halfword with the byte before it
    if ((uintptr_t)d & 1) {
        *d++ = *s++;
        if (--size == 0) {
            return;
        }
    }
    if (((uintptr_t)d ^ (uintptr_t)s) & 1) {
        // An odd source: it is loaded a halfword at a time from the halfword boundary before it, so each halfword
        // stored is the byte left over from the last load and the first byte of this one. The first load has one byte
        // of the source, and the last one's second byte is past the source's end when the size is even, and unused
        carry = *s++;
        for (; size >= 2; size -= 2) {
            low = carry;
            high = s[0];
            if (size > 2) {
                carry = s[1];
            }
            d[0] = low;
            d[1] = high;
            s += 2;
            d += 2;
        }
        if (size & 1) {
            *d = carry;
        }
        return;
    }
    if (((uintptr_t)d ^ (uintptr_t)s) & 2) {
        // Both even, at different offsets in their words: halfwords
        for (; size >= 2; size -= 2) {
            memmove(d, s, 2);
            s += 2;
            d += 2;
        }
    } else if (size >= 2) {
        // The same offset in their words: a halfword to reach a word boundary, then words, then a halfword
        if ((uintptr_t)d & 2) {
            memmove(d, s, 2);
            s += 2;
            d += 2;
            size -= 2;
            if (size == 0) {
                return;
            }
        }
        for (; size >= 4; size -= 4) {
            memmove(d, s, 4);
            s += 4;
            d += 4;
        }
        if (size & 2) {
            memmove(d, s, 2);
            s += 2;
            d += 2;
        }
    }
    // The last byte, stored as a halfword with the byte after it
    if (size & 1) {
        *d = *s;
    }
}

// Compares size bytes, returning the difference of the first that differ, or 0
int MI_CpuComp8(const void *a, const void *b, u32 size) {
    const u8 *p = a;
    const u8 *q = b;
    u32 i;

    for (i = 0; i < size; i++) {
        if (p[i] != q[i]) {
            return p[i] - q[i];
        }
    }
    return 0;
}

// The word loops of MI_CpuCopy, once the source is at a word boundary: blocks of 32 bytes while there are that many,
// then words. The original stores whole words at the destination's word boundaries, which are lag bytes before the
// source's, so each word stored holds the last lag bytes of the load before and the first of its own. Its first
// store also rewrites the lag bytes before the destination, with the values it read there just before; this leaves
// them alone. Returns the number of bytes loaded, of which the last lag aren't stored
static u32 CopyWords(const u8 *src, u8 *dest, u32 size, u32 lag) {
    // The bytes left over from the last load, then this one's
    u8 unit[3 + 32] = { 0 };
    u32 loaded = 0;
    u32 count;
    u32 i;

    while (size - loaded >= 4) {
        count = size - loaded >= 32 ? 32 : 4;
        for (i = 0; i < count; i++) {
            unit[lag + i] = src[loaded + i];
        }
        for (i = loaded == 0 ? lag : 0; i < count; i++) {
            dest[loaded + i - lag] = unit[i];
        }
        for (i = 0; i < lag; i++) {
            unit[i] = unit[count + i];
        }
        loaded += count;
    }
    return loaded;
}

// MI_CpuCopy: copies size bytes forwards by words, whatever the alignment of the two sides. Up to 8 bytes are copied
// one at a time. Otherwise bytes bring the source to a word boundary, then CopyWords copies by words, and the bytes
// left, including those loaded but not stored, are loaded again and copied one at a time. The units show only where
// the destination overlaps the source from after it, where a load can see an earlier store
void MI_CpuCopy(const void *src, void *dest, u32 size) {
    const u8 *s = src;
    u8 *d = dest;
    u32 lag;
    u32 stored;

    if (size <= 8) {
        for (; size != 0; size--) {
            *d++ = *s++;
        }
        return;
    }
    if ((uintptr_t)s & 1) {
        *d++ = *s++;
        size -= 1;
    }
    if ((uintptr_t)s & 2) {
        *d++ = *s++;
        *d++ = *s++;
        size -= 2;
    }
    lag = (uintptr_t)d & 3;
    stored = CopyWords(s, d, size, lag) - lag;
    s += stored;
    d += stored;
    for (size -= stored; size != 0; size--) {
        *d++ = *s++;
    }
}

// MI_CpuMove: copies size bytes, even when the ranges overlap. Where the destination is below the source the original
// is MI_CpuCopy, and otherwise it copies backwards from the ends in the same way (bytes to bring the source's end to a
// word boundary, blocks of 32 bytes, words shifted to fit the destination, then bytes). Either way each unit loads its
// bytes before it stores any, and no store reaches a source byte that is still to be loaded, so the result is always
// memmove's
void MI_CpuMove(const void *src, void *dest, u32 size) {
    memmove(dest, src, size);
}

// Clears 36 bytes, a 3x3 matrix
void MI_Zero36B(void *dest) {
    memset(dest, 0, 36);
}
