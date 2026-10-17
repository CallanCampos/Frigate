// Host version of NitroSDK's MATH_QSort, which the SDK wrote in assembly. The original is in
// lib/nitro/src/math/math.c, which the host build doesn't compile. This one calls the comparison function with the
// same arguments in the same order, and swaps the same elements, so elements that compare equal end in the order
// they have on the DS, and a comparison function that records its calls sees the same ones.
#include "nitro/math.h"

// The longest stack of ranges: one range per bit of the element count
#define QSORT_STACK_RANGES 32

// Swaps two elements. The SDK swaps a word at a time when width is a multiple of 4 and a byte at a time otherwise,
// which leaves the same bytes
static void SwapElements(u8 *a, u8 *b, u32 width) {
    while (width != 0) {
        u8 byte = *a;

        *a++ = *b;
        *b++ = byte;
        width--;
    }
}

// Pushes the range from first to last, as the offsets of the two elements from base, and returns the new top
static u32 *PushRange(u32 *top, const u8 *base, const u8 *first, const u8 *last) {
    top[0] = (u32)(first - base);
    top[1] = (u32)(last - base);
    return top + 2;
}

void MATH_QSort(void *head, u32 num, u32 width, MATHCompareFunc comp, void *stackBuffer) {
    u8 *base = head;
    u32 ownStack[2 * QSORT_STACK_RANGES];
    u32 *stack;
    u32 *top;
    u32 widthBits;

    // The SDK compares the count as signed, so a count of 0x80000000 or more sorts nothing
    if ((s32)num <= 1) {
        return;
    }
    // The ranges still to sort, a pair of words each for their first and last elements. The SDK keeps the elements'
    // addresses there, in stackBuffer or in (32 - clz(num)) * 8 bytes of its own stack; the host keeps their offsets
    // from head, which fit the same words with 64-bit pointers
    stack = stackBuffer != NULL ? stackBuffer : ownStack;
    top = PushRange(stack, base, base, base + (num - 1) * width);
    // The pivot is the element at the range's length in bytes shifted right by the bit length of width: the middle for
    // a power-of-two width, and further on for others, up to near the end (three quarters of the way for the 12-byte
    // elements of the game's sorts on the DS)
    widthBits = 32 - MATH_CountLeadingZeros(width);
    while (top != stack) {
        u8 *lo;
        u8 *hi;
        u8 *i;
        u8 *j;

        top -= 2;
        lo = base + top[0];
        hi = base + top[1];
        if ((u32)(hi - lo) == width) {
            if (comp(lo, hi) > 0) {
                SwapElements(lo, hi, width);
            }
            continue;
        }

        // The pivot goes first, then the elements after it are partitioned by comparison with it, and it is swapped
        // into place between the two parts
        SwapElements(lo + ((u32)(hi - lo) >> widthBits) * width, lo, width);
        i = lo + width;
        j = hi;
        do {
            while (i < hi && comp(i, lo) < 0) {
                i += width;
            }
            while (comp(j, lo) > 0) {
                j -= width;
            }
            if (i >= j) {
                break;
            }
            SwapElements(i, j, width);
            i += width;
            j -= width;
        } while (i <= j);
        SwapElements(lo, j, width);

        // The parts of two elements or more are pushed, the larger first, so that the smaller is sorted first
        if (j - lo > hi - j) {
            if (lo + width < j) {
                top = PushRange(top, base, lo, j - width);
            }
            if (j + width < hi) {
                top = PushRange(top, base, j + width, hi);
            }
        } else {
            if (j + width < hi) {
                top = PushRange(top, base, j + width, hi);
            }
            if (lo + width < j) {
                top = PushRange(top, base, lo, j - width);
            }
        }
    }
}
