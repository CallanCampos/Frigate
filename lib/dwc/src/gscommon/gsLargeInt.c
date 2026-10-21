#include "dwc/gsLargeInt.h"
#include "dwc/ext.h"
#include "nitro/std.h"

// GameSpy's large integers (gsLargeInt.c). The ROM names neither the file nor its functions: the file and most
// functions are GameSpy's, as pret's pokerevo Wii build has them (gsLargeInt.s), or the later SDK's (gsLargeIntDiv,
// gsiLargeIntStripLeadingZeroes, gsLargeIntSetFromHexString, gsLargeIntAddToMD5, gsLargeIntGetByteLength); the timers
// and gsiLargeIntInverseMod are ours

// The timers of the steps of the arithmetic: when one started, the time it took in all, how many times it started and
// whether it runs
typedef struct LargeIntTimer {
    u32 start;
    u32 total;
    u32 count;
    BOOL running;
} LargeIntTimer;

static void gsiLargeIntTimerStart(int index);
static void gsiLargeIntTimerStop(int index);
static int gsiLargeIntCompare(const u32 *a, u32 aLength, const u32 *b, u32 bLength);
static BOOL gsiLargeIntStripLeadingZeroes(BigNum *value);
static BOOL gsiLargeIntAdd(const u32 *a, u32 aLength, const u32 *b, u32 bLength, u32 *dest, u32 *destLength,
                           u32 maxLength);
static BOOL gsiLargeIntSub(const u32 *a, u32 aLength, const u32 *b, u32 bLength, u32 *dest, u32 *destLength);
static BOOL gsiLargeIntMult(const u32 *a, u32 aLength, const u32 *b, u32 bLength, u32 *dest, u32 *destLength,
                            u32 maxLength);
static BOOL gsLargeIntDiv(const BigNum *a, const BigNum *b, BigNum *quotient, BigNum *remainder);
static BOOL gsiLargeIntDiv(const u32 *a, u32 aLength, const BigNum *b, BigNum *quotient, BigNum *remainder);
static BOOL gsiLargeIntSubDivide(u32 *a, unsigned int aLength, const u32 *b, unsigned int bLength, u32 shift,
                                 u32 *result);
static BOOL gsLargeIntSquareMod(const BigNum *value, const BigNum *modulus, BigNum *result);
static BOOL gsiLargeIntMultM(const BigNum *a, const BigNum *b, const BigNum *modulus, u32 inverse, BigNum *result);
static BOOL gsiLargeIntInverseMod(const BigNum *modulus, u32 *result);
static int gsLargeIntGetByteLength(const BigNum *value);

static LargeIntTimer sTimers[9];

static void gsiLargeIntTimerStart(int index) {
    if (sTimers[index].running == 0) {
        sTimers[index].count++;
        sTimers[index].start = current_time_hires();
        sTimers[index].running = 1;
    }
}

static void gsiLargeIntTimerStop(int index) {
    if (sTimers[index].running == 1) {
        sTimers[index].total += current_time_hires() - sTimers[index].start;
        sTimers[index].running = 0;
    }
}

static int gsiLargeIntCompare(const u32 *a, u32 aLength, const u32 *b, u32 bLength) {
    while (a[aLength - 1] == 0 && aLength != 0) {
        aLength--;
    }
    while (b[bLength - 1] == 0 && bLength != 0) {
        bLength--;
    }
    if (aLength < bLength) {
        return -1;
    }
    if (aLength > bLength) {
        return 1;
    }
    for (; aLength != 0; aLength--) {
        if (a[aLength - 1] < b[aLength - 1]) {
            return -1;
        }
        if (a[aLength - 1] > b[aLength - 1]) {
            return 1;
        }
    }
    return 0;
}

static BOOL gsiLargeIntStripLeadingZeroes(BigNum *value) {
    while (value->length != 0 && value->data[value->length - 1] == 0) {
        value->length--;
    }
    return TRUE;
}

static BOOL gsiLargeIntAdd(const u32 *a, u32 aLength, const u32 *b, u32 bLength, u32 *dest, u32 *destLength,
                           u32 maxLength) {
    const u32 *longer;
    u32 shortLength;
    u32 i = 0;
    u64 carry = 0;

    gsiLargeIntTimerStart(3);
    if (maxLength < aLength || maxLength < bLength) {
        return FALSE;
    }
    if (aLength < bLength) {
        shortLength = aLength;
        longer = b;
    } else {
        shortLength = bLength;
        bLength = aLength;
        longer = a;
    }
    for (; i < shortLength; i++) {
        u64 sum = (u64)a[i] + b[i];
        u64 total = carry + sum;

        dest[i] = (u32)total;
        carry = total >> 32;
    }
    while (carry != 0 && i < bLength) {
        u64 total = carry + longer[i];

        dest[i] = (u32)total;
        carry = total >> 32;
        i++;
    }
    if (carry != 0) {
        if (maxLength <= i) {
            return FALSE;
        }
        dest[i] = (u32)carry;
        i++;
        carry = 0;
    }
    if (i < bLength) {
        if (dest + i != longer + i) {
            memcpy(dest + i, longer + i, (bLength - i) * 4);
        }
        i = bLength;
    }
    *destLength = i;
    gsiLargeIntTimerStop(3);
    if (carry != 0) {
        return FALSE;
    }
    return TRUE;
}

static BOOL gsiLargeIntSub(const u32 *a, u32 aLength, const u32 *b, u32 bLength, u32 *dest, u32 *destLength) {
    u32 shortLength;
    u32 i;
    u64 borrow = 0;

    if (aLength < bLength) {
        shortLength = aLength;
    } else {
        shortLength = bLength;
    }
    i = 0;
    gsiLargeIntTimerStart(4);
    for (; i < shortLength; i++) {
        u64 diff = (u64)b[i] - a[i] - borrow;

        dest[i] = (u32)diff;
        borrow = diff >> 63;
    }
    for (; i < bLength; i++) {
        u64 diff = (u64)b[i] - borrow;

        dest[i] = (u32)diff;
        borrow = diff >> 63;
    }
    if (borrow != 0) {
        gsiLargeIntTimerStop(4);
        return FALSE;
    }
    for (; i < aLength; i++) {
        if (a[i] != 0) {
            gsiLargeIntTimerStop(4);
            return FALSE;
        }
    }
    *destLength = bLength;
    gsiLargeIntTimerStop(4);
    return TRUE;
}

static BOOL gsiLargeIntMult(const u32 *a, u32 aLength, const u32 *b, u32 bLength, u32 *dest, u32 *destLength,
                            u32 maxLength) {
    BigNum temp = { 0 };
    u32 i = 0;
    u32 j;

    *destLength = 0;
    gsiLargeIntTimerStart(0);
    for (; i < bLength; i++) {
        if (b[i] != 0) {
            for (j = 0; j < aLength; j++) {
                u64 product = (u64)a[j] * b[i];
                int k = i + j;

                if (k >= maxLength) {
                    gsiLargeIntTimerStop(0);
                    return FALSE;
                }
                while (product != 0) {
                    product += temp.data[k];
                    temp.data[k] = (u32)product;
                    product >>= 32;
                    k++;
                    if (k > maxLength || (k == maxLength && product != 0)) {
                        gsiLargeIntTimerStop(0);
                        return FALSE;
                    }
                }
                if (k > (int)temp.length) {
                    temp.length = k;
                }
            }
        }
    }
    while (temp.length != 0 && temp.data[temp.length - 1] == 0) {
        temp.length--;
    }
    *destLength = temp.length;
    memcpy(dest, temp.data, *destLength * 4);
    gsiLargeIntTimerStop(0);
    return TRUE;
}

static BOOL gsLargeIntDiv(const BigNum *a, const BigNum *b, BigNum *quotient, BigNum *remainder) {
    return gsiLargeIntDiv(a->data, a->length, b, quotient, remainder);
}

// Divides a number by another one, giving the quotient and the remainder (either may be NULL)
static BOOL gsiLargeIntDiv(const u32 *a, u32 aLength, const BigNum *b, BigNum *quotient, BigNum *remainder) {
    const u32 *bData = b->data;
    u32 bLength = b->length;
    int done = 0;
    int bit = 31;
    int offset;
    u32 length;
    int top;

    gsiLargeIntTimerStart(5);
    u32 work[0x80] = { 0 };
    if (aLength > 0x40 && quotient != NULL) {
        return FALSE;
    }
    while (aLength != 0 && a[aLength - 1] == 0) {
        aLength--;
    }
    while (bLength != 0 && bData[bLength - 1] == 0) {
        bLength--;
    }
    memcpy(work, a, aLength * 4);
    BigNum quotientWork = { 0 };
    if (aLength == 0 || bLength == 0) {
        if (quotient != NULL) {
            quotient->data[0] = 0;
            quotient->length = 0;
        }
        if (remainder != NULL) {
            remainder->data[0] = 0;
            remainder->length = 0;
        }
        gsiLargeIntTimerStop(5);
        return bLength != 0;
    }
    if (gsiLargeIntCompare(work, aLength, bData, bLength) == -1) {
        if (quotient != NULL) {
            quotient->length = 0;
            quotient->data[0] = 0;
        }
        remainder->length = aLength;
        memcpy(remainder->data, work, aLength * 4);
        gsiLargeIntTimerStop(5);
        return TRUE;
    }
    while ((1 << bit & bData[bLength - 1]) == 0 && bit >= 0) {
        bit--;
    }
    if (bit == -1) {
        gsiLargeIntTimerStop(5);
        return FALSE;
    }
    top = (bLength - 1) * 32 + bit;
    offset = aLength - bLength;
    length = bLength;
    while (done == 0) {
        int compare = gsiLargeIntCompare(work + offset, length, bData, bLength);

        if (compare == -1) {
            if (offset > 0) {
                offset--;
                length++;
            } else {
                done = 1;
            }
        } else if (compare == 0) {
            u32 *at = work + offset;

            memset(at, 0, length * 4);
            quotientWork.data[offset]++;
            if (quotientWork.length < offset + length) {
                quotientWork.length = offset + length;
            }
            offset -= length;
            length = 1;
            if (offset < 0) {
                done = 1;
            }
        } else {
            u32 digit = 0;

            if (!gsiLargeIntSubDivide(work + offset, length, bData, bLength, top, &digit)) {
                gsiLargeIntTimerStop(5);
                return FALSE;
            }
            quotientWork.data[offset] += digit;
            if (quotientWork.length < offset + length) {
                quotientWork.length = offset + length;
            }
            while (work[offset + length - 1] == 0 && length > 1) {
                length--;
            }
            while (work[length + offset - 1] == 0 && offset > 1) {
                offset--;
            }
        }
    }
    if (offset >= 0) {
        memcpy(remainder->data, work + offset, length * 4);
        remainder->length = length;
    } else {
        remainder->data[0] = 0;
        remainder->length = 0;
    }
    if (quotient != NULL) {
        memcpy(quotient->data, quotientWork.data, quotientWork.length * 4);
        quotient->length = quotientWork.length;
    }
    gsiLargeIntTimerStop(5);
    return TRUE;
}

// Estimates the next word of a quotient: the top words of the dividend over the top word of the divisor, lowered until
// the divisor times it is not more than the dividend, which it then takes away from
static BOOL gsiLargeIntSubDivide(u32 *a, unsigned int aLength, const u32 *b, unsigned int bLength, u32 shift,
                                 u32 *result) {
    BigNum product;
    u64 estimate;

    gsiLargeIntTimerStart(6);
    if (aLength > bLength) {
        estimate = ((u64)a[aLength - 1] << 32) | (a[aLength - 2] & b[bLength - 1]);
    } else {
        estimate = a[aLength - 1];
    }
    estimate /= b[bLength - 1];
    BigNum quotient = { 0 };
    quotient.data[0] = estimate;
    quotient.data[1] = estimate >> 32;
    if (quotient.data[1] != 0) {
        quotient.data[0] = 0xffffffff;
        quotient.data[1] = 0;
    }
    quotient.length = 1;
    if (!gsiLargeIntMult(b, bLength, quotient.data, quotient.length, product.data, &product.length, 0x40)) {
        gsiLargeIntTimerStop(6);
        return FALSE;
    }
    while (gsiLargeIntCompare(product.data, product.length, a, aLength) == 1) {
        quotient.data[0] >>= 1;
        if (!gsiLargeIntMult(b, bLength, quotient.data, quotient.length, product.data, &product.length, 0x40)) {
            gsiLargeIntTimerStop(6);
            return FALSE;
        }
    }
    gsiLargeIntSub(product.data, product.length, a, aLength, a, &aLength);
    *result = quotient.data[0];
    gsiLargeIntTimerStop(6);
    return TRUE;
}

// Squares a number and takes the remainder of the division by the modulus
static BOOL gsLargeIntSquareMod(const BigNum *value, const BigNum *modulus, BigNum *result) {
    int count = value->length;
    u64 carry = 0;
    u32 bit = 0;
    int length = 0;
    int i;
    int j;
    BOOL ok;

    gsiLargeIntTimerStart(7);
    u32 squares[0x80] = { 0 };
    u32 cross[0x80] = { 0 };
    for (i = 1; i < count; i++) {
        for (j = 0; j < i; j++) {
            u64 product = (u64)value->data[i] * value->data[j];
            u64 total = carry + ((u64)cross[i + j] + product);

            cross[i + j] = (u32)total;
            carry = total >> 32;
        }
        if (carry != 0) {
            cross[i + j] = carry;
            carry = 0;
        }
    }
    for (i = 0; i < count * 2; i++) {
        u32 word = cross[i];

        cross[i] = bit + word * 2;
        bit = (word & 0x80000000) == 0x80000000;
    }
    for (i = 0; i < count; i++) {
        u64 square = (u64)value->data[i] * value->data[i];

        squares[i * 2] = (u32)square;
        squares[i * 2 + 1] = square >> 32;
    }
    length = count * 2;
    gsiLargeIntAdd(cross, count * 2, squares, count * 2, squares, (u32 *)&length, 0x80);
    ok = gsiLargeIntDiv(squares, length, modulus, NULL, result);
    gsiLargeIntTimerStop(7);
    return ok;
}

// Raises a number to a power modulo an odd modulus, working with Montgomery products
BOOL gsLargeIntPowerMod(const BigNum *base, const BigNum *exponent, const BigNum *modulus, BigNum *result) {
    u32 inverse;
    int bits;
    int bit;
    int top;
    BOOL ok;

    gsiLargeIntTimerStart(8);
    BigNum radix = { 0 };
    BigNum radixMod = { 0 };
    BigNum radixSquare = { 0 };
    BigNum one = { 0 };
    BigNum baseMont = { 0 };
    BigNum b;
    BigNum e;
    BigNum m;
    memcpy(&b, base, sizeof(BigNum));
    memcpy(&e, exponent, sizeof(BigNum));
    memcpy(&m, modulus, sizeof(BigNum));
    gsiLargeIntStripLeadingZeroes(&b);
    gsiLargeIntStripLeadingZeroes(&e);
    gsiLargeIntStripLeadingZeroes(&m);
    if (m.length == 0) {
        result->length = 0;
        gsiLargeIntTimerStop(8);
        return FALSE;
    }
    if (m.length == 1 && m.data[0] == 1) {
        result->length = 0;
        result->data[0] = 0;
        gsiLargeIntTimerStop(8);
        return TRUE;
    }
    if (e.length == 0) {
        result->length = 1;
        result->data[0] = 1;
        gsiLargeIntTimerStop(8);
        return TRUE;
    }
    if ((m.data[0] & 1) == 0) {
        result->length = 0;
        result->data[0] = 0;
        gsiLargeIntTimerStop(8);
        return FALSE;
    }
    if (gsiLargeIntCompare(b.data, b.length, m.data, m.length) != -1) {
        gsLargeIntDiv(&b, &m, NULL, &b);
    }
    if (b.length == 0) {
        result->length = 0;
        result->data[0] = 0;
        gsiLargeIntTimerStop(8);
        return TRUE;
    }
    e.length--;
    bits = 32;
    if ((e.data[e.length] & 0x80000000) == 0) {
        do {
            bits--;
        } while (((1 << (bits - 1)) & e.data[e.length]) == 0);
    }
    top = bits + e.length * 32;
    radix.length = m.length + 1;
    if (radix.length > 0x40) {
        return FALSE;
    }
    radix.data[radix.length - 1] = 1;
    gsiLargeIntInverseMod(&m, &inverse);
    ok = FALSE;
    if (!gsLargeIntDiv(&radix, &m, NULL, &radixMod)) {
        gsiLargeIntTimerStop(8);
        return ok;
    }
    if (!gsLargeIntSquareMod(&radixMod, &m, &radixSquare)) {
        gsiLargeIntTimerStop(8);
        return ok;
    }
    if (!gsiLargeIntMultM(&b, &radixSquare, &m, inverse, &baseMont)) {
        gsiLargeIntTimerStop(8);
        return ok;
    }
    memcpy(result, &radixMod, sizeof(BigNum));
    for (bit = top - 1; bit >= 0; bit--) {
        u32 mask;

        gsiLargeIntMultM(result, result, &m, inverse, result);
        mask = 1 << (bit & 0x1f);
        if (mask == (e.data[bit >> 5] & mask)) {
            gsiLargeIntMultM(result, &baseMont, &m, inverse, result);
        }
    }
    one.length = 1;
    one.data[0] = 1;
    gsiLargeIntMultM(result, &one, &m, inverse, result);
    gsiLargeIntTimerStop(8);
    return TRUE;
}

// The Montgomery product of two numbers: a times b, divided by the radix, modulo the modulus
static BOOL gsiLargeIntMultM(const BigNum *a, const BigNum *b, const BigNum *modulus, u32 inverse, BigNum *result) {
    u32 length = 0;
    int count = modulus->length;
    u32 work[0x80] = { 0 };
    u32 *p;
    u32 *q;
    const u32 *m;
    u32 *workLast;
    const u32 *modulusLast;
    u64 carry;
    u32 factor;

    if (!gsiLargeIntMult(a->data, a->length, b->data, b->length, work, &length, 0x80)) {
        return FALSE;
    }
    workLast = work + (modulus->length - 1);
    modulusLast = modulus->data + (modulus->length - 1);
    if (length < modulus->length * 2) {
        memset(work + length, 0, (modulus->length * 2 - length) * 4);
        length = modulus->length * 2;
    }
    for (p = work; p <= workLast; p++) {
        u64 product;

        carry = 0;
        factor = (u64)inverse * *p;
        q = p;
        for (m = modulus->data; m <= modulusLast; m++) {
            product = (u64)factor * *m;
            product = carry + ((u64)*q + product);
            *q = (u32)product;
            carry = product >> 32;
            q++;
        }
        for (; carry != 0 && q <= work + length - 1; q++) {
            u64 sum = (u64)*q + carry;

            *q = (u32)sum;
            carry = sum >> 32;
        }
        if (carry != 0) {
            *q = carry;
            length++;
        }
    }
    if (gsiLargeIntCompare(work + count, length - count, modulus->data, modulus->length) != -1) {
        if (!gsiLargeIntSub(modulus->data, modulus->length, work + count, length - count, result->data,
                            (u32 *)&result->length)) {
            memset(work, 0, sizeof(work));
            memset(result, 0, sizeof(BigNum));
            return FALSE;
        }
    } else {
        memset(result, 0, sizeof(BigNum));
        result->length = modulus->length;
        memcpy(result->data, work + count, (length - count) * 4);
        memset(work, 0, sizeof(work));
    }
    return TRUE;
}

// Finds the word that, with the modulus, makes the Montgomery reduction work: the negated inverse of the lowest word
static BOOL gsiLargeIntInverseMod(const BigNum *modulus, u32 *result) {
    u32 i;
    u64 x = 2;
    u64 y = 1;

    for (i = 2; i <= 32; i++) {
        if (x < ((modulus->data[0] * y) & (2 * x - 1))) {
            y += x;
        }
        x <<= 1;
    }
    *result = x - y;
    return TRUE;
}

// Reads a text of hexadecimal digits into a number
BOOL gsLargeIntSetFromHexString(BigNum *value, const char *text) {
    u32 *word = value->data;
    u32 shift = 0;
    u32 length = strlen(text);
    u32 byte;

    if (length == 0) {
        value->length = 0;
        value->data[0] = 0;
        return TRUE;
    }
    if (length / 2 > 0x100) {
        return FALSE;
    }
    value->length = (length + 7) / 8;
    value->data[value->length - 1] = 0;
    while (length != 0) {
        if (length >= 2) {
            STD_TSScanf(text + length - 2, "%02x", &byte);
        } else {
            STD_TSScanf(text + length - 1, "%01x", &byte);
        }
        if (shift == 0) {
            *word = 0;
        }
        *word |= byte << (shift * 8);
        shift++;
        if (shift == 4) {
            word++;
            shift = 0;
        }
        length -= length > 2 ? 2 : length;
    }
    return TRUE;
}

// Reverses the order of the bytes of a number
BOOL gsLargeIntReverseBytes(BigNum *value) {
    char *low = (char *)value->data;
    char *high = (char *)(value->data + value->length) - 1;
    char swap;

    if (value->length == 0) {
        return TRUE;
    }
    while (low < high) {
        swap = *low;
        *low = *high;
        *high = swap;
        low++;
        high--;
    }
    return TRUE;
}

// Adds the bytes of a number, most significant first, to a hash
void gsLargeIntAddToMD5(const BigNum *value, HashContext *context) {
    BigNum copy;
    u32 size;
    u8 *bytes;

    memcpy(&copy, value, sizeof(BigNum));
    size = gsLargeIntGetByteLength(&copy);

    if (size != 0) {
        bytes = (u8 *)copy.data;
        if ((size & 3) != 0) {
            bytes += 4 - (size & 3);
        }
        gsLargeIntReverseBytes(&copy);
        func_ov011_02171a24(context, bytes, size);
        gsLargeIntReverseBytes(&copy);
    }
}

// How many bytes the number takes
static int gsLargeIntGetByteLength(const BigNum *value) {
    int length = value->length;
    int bytes;
    u32 i;
    u32 mask;

    while (length > 0 && value->data[length - 1] == 0) {
        length--;
    }
    if (length == 0) {
        return 0;
    }
    bytes = length * 4;
    mask = 0xff;
    for (i = 1; i < 4; i++) {
        if (value->data[length - 1] <= mask) {
            bytes -= 4 - i;
            break;
        }
        mask <<= 8;
        mask |= 0xff;
    }
    return bytes;
}
