// Unit tests of port/hal/mi, the host versions of NitroSDK's MI routines that the SDK wrote in assembly: known vectors
// for each function, among them the cases where the original's behaviour is not a plain memcpy or memset (odd sizes
// rounded up, fill values above a byte, overlapping copies, LZ77 copies past the size). Each expected result is what
// the ROM's original code gives (port/tests/asm_oracle/test_mi.py compares the two on thousands of random inputs);
// these need no ROM. The sends and copies to a geometry command port are checked in port/hal/hw's command log, so the
// program links port/hal/hw/hw.c too. It prints each failure and returns nonzero if there was one.
#include "hal/hw/hw.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include <stdio.h>
#include <string.h>

static int sFailures;

// A buffer at a 64-byte boundary, so that offsets into it give the same alignment on every host
static _Alignas(64) u8 sBuffer[256];

static void Check(const char *name, const u8 *actual, const u8 *expected, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (actual[i] != expected[i]) {
            printf("FAIL %s: byte %u is 0x%02x, expected 0x%02x\n", name, (unsigned)i, actual[i], expected[i]);
            sFailures++;
            return;
        }
    }
}

static void CheckInt(const char *name, int actual, int expected) {
    if (actual != expected) {
        printf("FAIL %s: %d, expected %d\n", name, actual, expected);
        sFailures++;
    }
}

static void CheckWord(const char *name, u32 actual, u32 expected) {
    if (actual != expected) {
        printf("FAIL %s: 0x%08x, expected 0x%08x\n", name, (unsigned)actual, (unsigned)expected);
        sFailures++;
    }
}

// Fills the buffer with 0xee
static void Clear(void) {
    memset(sBuffer, 0xee, sizeof(sBuffer));
}

// Fills the buffer with its offsets: byte i is i
static void Count(void) {
    u32 i;

    for (i = 0; i < sizeof(sBuffer); i++) {
        sBuffer[i] = (u8)i;
    }
}

static void TestHalfwordsAndWords(void) {
    static const u8 clear16[] = { 0xcd, 0xab, 0xcd, 0xab, 0xee };
    static const u8 untouched[] = { 0xee, 0xee, 0xee, 0xee };
    static const u8 copy16[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x46, 0x47 };
    static const u8 clear32[] = { 0x04, 0x03, 0x02, 0x01, 0x04, 0x03, 0x02, 0x01, 0xee };
    static const u8 copy32[] = { 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01,
                                 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13 };
    static const u8 send32[] = { 0x08, 0x09, 0x0a, 0x0b, 0x44 };
    u8 expected[48];
    u32 i;

    // An odd size is rounded up to a halfword
    Clear();
    MIi_CpuClear16(0xabcd, sBuffer, 3);
    Check("MIi_CpuClear16 of 3 bytes", sBuffer, clear16, sizeof(clear16));
    // A size of 2 GiB or more is negative to the original's signed loop
    Clear();
    MIi_CpuClear16(0xabcd, sBuffer, 0x80000000);
    Check("MIi_CpuClear16 of 2 GiB", sBuffer, untouched, sizeof(untouched));
    Count();
    MIi_CpuCopy16(sBuffer, sBuffer + 64, 5);
    Check("MIi_CpuCopy16 of 5 bytes", sBuffer + 64, copy16, sizeof(copy16));
    // An odd size is rounded up to a word
    Clear();
    MIi_CpuClear32(0x01020304, sBuffer, 5);
    Check("MIi_CpuClear32 of 5 bytes", sBuffer, clear32, sizeof(clear32));
    Clear();
    MIi_CpuClear32(0x01020304, sBuffer, 0xfffffffc);
    Check("MIi_CpuClear32 of -4 bytes", sBuffer, untouched, sizeof(untouched));
    // A word at a time forwards: a destination one word after the source repeats the first word
    Count();
    MIi_CpuCopy32(sBuffer, sBuffer + 4, 12);
    Check("MIi_CpuCopy32 one word up", sBuffer, copy32, sizeof(copy32));
    // 32 bytes at a time: the block is loaded before it is stored, then the last word sees the block's store
    Count();
    MIi_CpuCopyFast(sBuffer, sBuffer + 4, 36);
    for (i = 0; i < 36; i++) {
        expected[i] = (u8)(i < 4 ? i : i - 4);
    }
    for (i = 36; i < 40; i++) {
        expected[i] = (u8)(i - 8);
    }
    Check("MIi_CpuCopyFast one word up", sBuffer, expected, 40);
    Clear();
    MIi_CpuClearFast(0x5a5a5a5a, sBuffer, 37);
    memset(expected, 0x5a, 40);
    Check("MIi_CpuClearFast of 37 bytes", sBuffer, expected, 40);
    Check("MIi_CpuClearFast's end", sBuffer + 40, untouched, sizeof(untouched));
    // Every word goes to the same address, rounded up to whole words: the last is 0x0b0a0908
    Count();
    MIi_CpuSend32(sBuffer, sBuffer + 64, 10);
    Check("MIi_CpuSend32", sBuffer + 64, send32, sizeof(send32));
}

// Checks that the geometry command log holds count writes since the last check: values[i] to port + i * step
static void CheckLog(const char *name, u32 port, u32 step, const u32 *values, u32 count) {
    size_t logged;
    const PortG3Write *log = port_hw_g3_take(&logged);
    u32 i;

    if (logged != count) {
        printf("FAIL %s: %u writes logged, expected %u\n", name, (unsigned)logged, (unsigned)count);
        sFailures++;
        return;
    }
    for (i = 0; i < count; i++) {
        if (log[i].port != port + i * step || log[i].value != values[i]) {
            printf("FAIL %s: write %u is 0x%08x to 0x%08x, expected 0x%08x to 0x%08x\n", name, (unsigned)i,
                   (unsigned)log[i].value, (unsigned)log[i].port, (unsigned)values[i], (unsigned)(port + i * step));
            sFailures++;
            return;
        }
    }
}

// The geometry FIFO takes every word sent or copied to it, in order, and a send of nothing sends nothing. A command
// port takes a send's words in the same way, while a copy's STM stores to the ports that follow it
static void TestGeometryPort(void) {
    u32 words[16];
    u32 i;

    for (i = 0; i < 16; i++) {
        words[i] = 0x01010101 * (i + 1);
    }
    CheckLog("the log before", 0x04000400, 0, words, 0);
    // A size that isn't whole words is rounded up to them
    MIi_CpuSend32(words + 1, &reg_G3X_GXFIFO, 10);
    CheckLog("MIi_CpuSend32 to GXFIFO", 0x04000400, 0, words + 1, 3);
    MIi_CpuSend32(words, &reg_G3X_GXFIFO, 64);
    CheckLog("MIi_CpuSend32 of 64 bytes to GXFIFO", 0x04000400, 0, words, 16);
    // NNS_G3dGeBufferOP_N's command with no arguments
    reg_G3X_GXFIFO = words[0];
    MIi_CpuSend32(words + 1, &reg_G3X_GXFIFO, 0);
    CheckLog("MIi_CpuSend32 of nothing to GXFIFO", 0x04000400, 0, words, 1);
    MIi_CpuSend32(words, &reg_G3X_GXFIFO, 0x80000000);
    CheckLog("MIi_CpuSend32 of 2 GiB to GXFIFO", 0x04000400, 0, words, 0);
    // G3_MultMtx33: a command, then a 3x3 matrix copied to the FIFO, whose mirrors take all of an STM's stores
    reg_G3X_GXFIFO = words[0];
    MI_Copy36B(words + 1, (void *)&reg_G3X_GXFIFO);
    CheckLog("MI_Copy36B to GXFIFO", 0x04000400, 0, words, 10);
    MI_Copy32B(words, (void *)&reg_G3X_GXFIFO);
    CheckLog("MI_Copy32B to GXFIFO", 0x04000400, 0, words, 8);
    MI_Copy48B(words, (void *)&reg_G3X_GXFIFO);
    CheckLog("MI_Copy48B to GXFIFO", 0x04000400, 0, words, 12);
    MI_Copy64B(words, (void *)&reg_G3X_GXFIFO);
    CheckLog("MI_Copy64B to GXFIFO", 0x04000400, 0, words, 16);
    MIi_CpuSend32(words, &reg_G3_MTX_TRANS, 12);
    CheckLog("MIi_CpuSend32 to MTX_TRANS", 0x04000470, 0, words, 3);
    MI_Copy36B(words, (void *)&reg_G3_MTX_SCALE);
    CheckLog("MI_Copy36B to MTX_SCALE", 0x0400046c, 4, words, 9);
}

static void TestFixedCopies(void) {
    static const u8 copy36[] = { 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x11,
                                 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x14, 0x15, 0x16, 0x17, 0x1c,
                                 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x28, 0x29, 0x2a, 0x2b };
    u8 expected[72];
    u32 i;

    // Three words are loaded, then stored: one word up, each unit's first word is the last unit's last store
    Count();
    MI_Copy36B(sBuffer, sBuffer + 4);
    Check("MI_Copy36B one word up", sBuffer, copy36, sizeof(copy36));
    Count();
    MI_Copy32B(sBuffer, sBuffer + 128);
    Check("MI_Copy32B", sBuffer + 128, sBuffer, 32);
    CheckInt("MI_Copy32B's end", sBuffer[160], 160);
    Count();
    MI_Copy48B(sBuffer, sBuffer + 128);
    Check("MI_Copy48B", sBuffer + 128, sBuffer, 48);
    Count();
    MI_Copy64B(sBuffer, sBuffer + 128);
    Check("MI_Copy64B", sBuffer + 128, sBuffer, 64);
    for (i = 0; i < 72; i++) {
        expected[i] = (u8)(i < 4 || i >= 40 ? 0xee : 0);
    }
    Clear();
    MI_Zero36B(sBuffer + 4);
    Check("MI_Zero36B", sBuffer, expected, sizeof(expected));
}

static void TestFills(void) {
    // Bits above the low byte reach the halfwords' and words' other bytes: 0x1234 gives halfwords of 0x34, 0x36
    // and words of 0x34, 0x36, 0x36, 0x36
    static const u8 fill8[] = { 0xee, 0x34, 0x34, 0x36, 0x34, 0x36, 0x36, 0x36, 0x34, 0x36, 0xee, 0xee };
    static const u8 fill[] = { 0xee, 0x34, 0x34, 0x36, 0x34, 0x36, 0x36, 0x36, 0x34, 0x36, 0x36, 0x36,
                               0x34, 0x36, 0x36, 0x36, 0x34, 0x36, 0x36, 0x36, 0x34, 0xee, 0xee, 0xee };
    static const u8 small[] = { 0xee, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0xee };
    static const u8 bytes[] = { 0xee, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0xee };

    Clear();
    MI_CpuFill8(sBuffer + 1, 0x1234, 9);
    Check("MI_CpuFill8 of 0x1234", sBuffer, fill8, sizeof(fill8));
    Clear();
    MI_CpuFill8(sBuffer + 1, 0x7f, 10);
    Check("MI_CpuFill8 of 0x7f", sBuffer, bytes, sizeof(bytes));
    Clear();
    MI_CpuFill(sBuffer + 1, 0x1234, 20);
    Check("MI_CpuFill of 0x1234", sBuffer, fill, sizeof(fill));
    // Up to 8 bytes are stored one at a time
    Clear();
    MI_CpuFill(sBuffer + 1, 0x1234, 8);
    Check("MI_CpuFill of 8 bytes", sBuffer, small, sizeof(small));
    Clear();
    MI_CpuFill(sBuffer + 1, 0x7f, 10);
    Check("MI_CpuFill of 0x7f", sBuffer, bytes, sizeof(bytes));
}

static void TestByteCopies(void) {
    // An odd source to an even destination one byte up: each halfword's second byte is loaded before the store, so
    // the bytes move up one, as memmove would
    static const u8 copy8[] = { 0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x09 };
    u8 expected[48];
    u32 i;

    Count();
    MI_CpuCopy8(sBuffer + 1, sBuffer + 2, 6);
    Check("MI_CpuCopy8 one byte up", sBuffer, copy8, sizeof(copy8));
    for (i = 0; i < 4; i++) {
        Count();
        MI_CpuCopy8(sBuffer + 3, sBuffer + 64 + i, 37);
        Check("MI_CpuCopy8", sBuffer + 64 + i, sBuffer + 3, 37);
        CheckInt("MI_CpuCopy8's end", sBuffer[64 + i + 37], 64 + i + 37);
        CheckInt("MI_CpuCopy8's start", sBuffer[64 + i - 1], 64 + i - 1);
    }
    // One byte up: words shifted by a byte, each stored after its load. The last byte is loaded again from the source
    // after the word before it was stored there, so it repeats the byte before it
    Count();
    MI_CpuCopy(sBuffer, sBuffer + 1, 40);
    for (i = 0; i < 44; i++) {
        expected[i] = (u8)(i == 0 ? 0 : i <= 39 ? i - 1 : i == 40 ? 38 : i);
    }
    Check("MI_CpuCopy one byte up", sBuffer, expected, 44);
    // MI_CpuMove gives memmove's result either way
    Count();
    MI_CpuMove(sBuffer, sBuffer + 1, 40);
    for (i = 0; i < 44; i++) {
        expected[i] = (u8)(i == 0 ? 0 : i <= 40 ? i - 1 : i);
    }
    Check("MI_CpuMove one byte up", sBuffer, expected, 44);
    Count();
    MI_CpuMove(sBuffer + 1, sBuffer, 40);
    for (i = 0; i < 44; i++) {
        expected[i] = (u8)(i < 40 ? i + 1 : i);
    }
    Check("MI_CpuMove one byte down", sBuffer, expected, 44);
}

static void TestCompare(void) {
    CheckInt("MI_CpuComp8 of abc and abd", MI_CpuComp8("abc", "abd", 3), -1);
    CheckInt("MI_CpuComp8 of 0xff and 0", MI_CpuComp8("\xff", "\x00", 1), 255);
    CheckInt("MI_CpuComp8 of equal bytes", MI_CpuComp8("abc", "abc", 3), 0);
    CheckInt("MI_CpuComp8 of no bytes", MI_CpuComp8("x", "y", 0), 0);
}

static void TestSwap(void) {
    vu32 word = 0x11223344;

    CheckWord("MI_SwapWord's result", MI_SwapWord(0xcafebabe, &word), 0x11223344);
    CheckWord("MI_SwapWord's store", word, 0xcafebabe);
}

static void TestUncompress(void) {
    // Type 0x10, 12 bytes: abc, then 9 bytes from 3 back
    static _Alignas(4) const u8 plain[] = { 0x10, 0x0c, 0x00, 0x00, 0x10, 'a', 'b', 'c', 0x60, 0x02 };
    // Type 0x11, 324 bytes: x, then copies of 20, 300 and 3 bytes from 1 back, one of each length's form
    static _Alignas(4) const u8 extended[] = { 0x11, 0x44, 0x01, 0x00, 0x70, 'x',  0x00, 0x30,
                                               0x00, 0x10, 0x01, 0xb0, 0x00, 0x20, 0x00 };
    // Type 0x10, 4 bytes: a, then 5 bytes from 1 back, which run past the size and are all written
    static _Alignas(4) const u8 past[] = { 0x10, 0x04, 0x00, 0x00, 0x40, 'a', 0x20, 0x00 };
    // Only the type's low nibble counts: the same copy is 6 bytes in the plain format and 4 in the extended one
    static _Alignas(4) u8 nibble[] = { 0x00, 0x05, 0x00, 0x00, 0x40, 'z', 0x30, 0x00 };
    static const u8 types[] = { 0x10, 0x20, 0x11, 0x13 };
    static u8 output[330];
    u32 i;

    Clear();
    MI_UncompressLZ8(plain, sBuffer);
    Check("MI_UncompressLZ8 of type 0x10", sBuffer, (const u8 *)"abcabcabcabc\xee", 13);
    memset(output, 0xee, sizeof(output));
    MI_UncompressLZ8(extended, output);
    for (i = 0; i < 324; i++) {
        if (output[i] != 'x') {
            break;
        }
    }
    CheckInt("MI_UncompressLZ8 of type 0x11", (int)i, 324);
    CheckInt("MI_UncompressLZ8 of type 0x11's end", output[324], 0xee);
    Clear();
    MI_UncompressLZ8(past, sBuffer);
    Check("MI_UncompressLZ8 past the size", sBuffer, (const u8 *)"aaaaaa\xee", 7);
    for (i = 0; i < 4; i++) {
        nibble[0] = types[i];
        Clear();
        MI_UncompressLZ8(nibble, sBuffer);
        Check("MI_UncompressLZ8 by the type's nibble", sBuffer, (const u8 *)(i < 2 ? "zzzzzzz\xee" : "zzzzz\xee"),
              i < 2 ? 8 : 6);
    }
}

int main(void) {
    TestHalfwordsAndWords();
    TestGeometryPort();
    TestFixedCopies();
    TestFills();
    TestByteCopies();
    TestCompare();
    TestSwap();
    TestUncompress();
    if (sFailures == 0) {
        printf("test_mi: all passed\n");
    }
    return sFailures != 0;
}
