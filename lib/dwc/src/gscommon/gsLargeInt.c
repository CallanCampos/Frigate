#include "dwc/gsLargeInt.h"
#include "dwc/ext.h"
#include "nitro/std.h"

// GameSpy's large integers (gsLargeInt.c). The ROM names neither the file nor its functions: the file and most
// functions are GameSpy's, as pret's pokerevo Wii build has them (gsLargeInt.s), or the later SDK's (gsLargeIntDiv,
// gsiLargeIntStripLeadingZeroes, gsLargeIntSetFromHexString, gsLargeIntAddToMD5, gsLargeIntGetByteLength); the timers
// and gsiLargeIntInverseMod are ours

// The timers of the steps of the arithmetic: when one started, the time it took in all, how many times it started and
// whether it runs
typedef enum {
    GSLintTimerMult,
    GSLintTimerMultM,
    GSLintTimerKMult,
    GSLintTimerAdd,
    GSLintTimerSub,
    GSLintTimerDiv,
    GSLintTimerSubDivide,
    GSLintTimerSquareMod,
    GSLintTimerPowerMod,
    GSLintTimerCount
} GSLintTimerID;

typedef struct GSLintTimer {
    gsi_time start;
    gsi_time total;
    gsi_u32 count;
    gsi_u32 running;
} GSLintTimer;

static void gsiLargeIntTimerStart(GSLintTimerID index);
static void gsiLargeIntTimerStop(GSLintTimerID index);
static gsi_i32 gsiLargeIntCompare(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength);
static gsi_bool gsiLargeIntStripLeadingZeroes(gsLargeInt_t *value);
static gsi_bool gsiLargeIntAdd(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 *dest,
                               gsi_u32 *destLength, gsi_u32 maxLength);
static gsi_bool gsiLargeIntSub(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 *dest,
                               gsi_u32 *destLength);
static gsi_bool gsiLargeIntMult(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 *dest,
                                gsi_u32 *destLength, gsi_u32 maxLength);
static gsi_bool gsLargeIntDiv(const gsLargeInt_t *a, const gsLargeInt_t *b, gsLargeInt_t *quotient,
                              gsLargeInt_t *remainder);
static gsi_bool gsiLargeIntDiv(const gsi_u32 *a, gsi_u32 aLength, const gsLargeInt_t *b, gsLargeInt_t *quotient,
                               gsLargeInt_t *remainder);
static gsi_bool gsiLargeIntSubDivide(gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 shift,
                                     gsi_u32 *result);
static gsi_bool gsLargeIntSquareMod(const gsLargeInt_t *value, const gsLargeInt_t *modulus, gsLargeInt_t *result);
static gsi_bool gsiLargeIntMultM(gsLargeInt_t *a, gsLargeInt_t *b, const gsLargeInt_t *modulus, gsi_u32 inverse,
                                 gsLargeInt_t *result);
static gsi_bool gsiLargeIntInverseMod(const gsLargeInt_t *modulus, gsi_u32 *result);
static gsi_u32 gsLargeIntGetByteLength(const gsLargeInt_t *value);

static GSLintTimer sTimers[GSLintTimerCount];

static void gsiLargeIntTimerStart(GSLintTimerID index) {
    if (sTimers[index].running == 0) {
        sTimers[index].count++;
        sTimers[index].start = current_time_hires();
        sTimers[index].running = 1;
    }
}

static void gsiLargeIntTimerStop(GSLintTimerID index) {
    if (sTimers[index].running == 1) {
        sTimers[index].total += current_time_hires() - sTimers[index].start;
        sTimers[index].running = 0;
    }
}

static gsi_i32 gsiLargeIntCompare(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength) {
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

static gsi_bool gsiLargeIntStripLeadingZeroes(gsLargeInt_t *value) {
    while (value->length != 0 && value->data[value->length - 1] == 0) {
        value->length--;
    }
    return gsi_true;
}

static gsi_bool gsiLargeIntAdd(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 *dest,
                               gsi_u32 *destLength, gsi_u32 maxLength) {
    const gsi_u32 *longer;
    gsi_u32 shortLength;
    gsi_u32 i = 0;
    gsi_u64 carry = 0;

    gsiLargeIntTimerStart(GSLintTimerAdd);
    if (maxLength < aLength || maxLength < bLength) {
        return gsi_false;
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
        gsi_u64 sum = (gsi_u64)a[i] + b[i];
        gsi_u64 total = carry + sum;

        dest[i] = (gsi_u32)total;
        carry = total >> 32;
    }
    while (carry != 0 && i < bLength) {
        gsi_u64 total = carry + longer[i];

        dest[i] = (gsi_u32)total;
        carry = total >> 32;
        i++;
    }
    if (carry != 0) {
        if (maxLength <= i) {
            return gsi_false;
        }
        dest[i] = (gsi_u32)carry;
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
    gsiLargeIntTimerStop(GSLintTimerAdd);
    if (carry != 0) {
        return gsi_false;
    }
    return gsi_true;
}

static gsi_bool gsiLargeIntSub(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 *dest,
                               gsi_u32 *destLength) {
    gsi_u32 shortLength;
    gsi_u32 i;
    gsi_u64 borrow = 0;

    if (aLength < bLength) {
        shortLength = aLength;
    } else {
        shortLength = bLength;
    }
    i = 0;
    gsiLargeIntTimerStart(GSLintTimerSub);
    for (; i < shortLength; i++) {
        gsi_u64 diff = (gsi_u64)b[i] - a[i] - borrow;

        dest[i] = (gsi_u32)diff;
        borrow = diff >> 63;
    }
    for (; i < bLength; i++) {
        gsi_u64 diff = (gsi_u64)b[i] - borrow;

        dest[i] = (gsi_u32)diff;
        borrow = diff >> 63;
    }
    if (borrow != 0) {
        gsiLargeIntTimerStop(GSLintTimerSub);
        return gsi_false;
    }
    for (; i < aLength; i++) {
        if (a[i] != 0) {
            gsiLargeIntTimerStop(GSLintTimerSub);
            return gsi_false;
        }
    }
    *destLength = bLength;
    gsiLargeIntTimerStop(GSLintTimerSub);
    return gsi_true;
}

static gsi_bool gsiLargeIntMult(const gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 *dest,
                                gsi_u32 *destLength, gsi_u32 maxLength) {
    gsLargeInt_t temp = { 0 };
    gsi_u32 i = 0;
    gsi_u32 j;

    *destLength = 0;
    gsiLargeIntTimerStart(GSLintTimerMult);
    for (; i < bLength; i++) {
        if (b[i] != 0) {
            for (j = 0; j < aLength; j++) {
                gsi_u64 product = (gsi_u64)a[j] * b[i];
                int k = i + j;

                if (k >= maxLength) {
                    gsiLargeIntTimerStop(GSLintTimerMult);
                    return gsi_false;
                }
                while (product != 0) {
                    product += temp.data[k];
                    temp.data[k] = (gsi_u32)product;
                    product >>= 32;
                    k++;
                    if (k > maxLength || (k == maxLength && product != 0)) {
                        gsiLargeIntTimerStop(GSLintTimerMult);
                        return gsi_false;
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
    gsiLargeIntTimerStop(GSLintTimerMult);
    return gsi_true;
}

static gsi_bool gsLargeIntDiv(const gsLargeInt_t *a, const gsLargeInt_t *b, gsLargeInt_t *quotient,
                              gsLargeInt_t *remainder) {
    return gsiLargeIntDiv(a->data, a->length, b, quotient, remainder);
}

// Divides a number by another one, giving the quotient and the remainder (either may be NULL)
static gsi_bool gsiLargeIntDiv(const gsi_u32 *a, gsi_u32 aLength, const gsLargeInt_t *b, gsLargeInt_t *quotient,
                               gsLargeInt_t *remainder) {
    const gsi_u32 *bData = b->data;
    gsi_u32 bLength = b->length;
    int done = 0;
    int bit = 31;
    int offset;
    gsi_u32 length;
    int top;

    gsiLargeIntTimerStart(GSLintTimerDiv);
    gsi_u32 work[0x80] = { 0 };
    if (aLength > 0x40 && quotient != NULL) {
        return gsi_false;
    }
    while (aLength != 0 && a[aLength - 1] == 0) {
        aLength--;
    }
    while (bLength != 0 && bData[bLength - 1] == 0) {
        bLength--;
    }
    memcpy(work, a, aLength * 4);
    gsLargeInt_t quotientWork = { 0 };
    if (aLength == 0 || bLength == 0) {
        if (quotient != NULL) {
            quotient->data[0] = 0;
            quotient->length = 0;
        }
        if (remainder != NULL) {
            remainder->data[0] = 0;
            remainder->length = 0;
        }
        gsiLargeIntTimerStop(GSLintTimerDiv);
        return bLength != 0;
    }
    if (gsiLargeIntCompare(work, aLength, bData, bLength) == -1) {
        if (quotient != NULL) {
            quotient->length = 0;
            quotient->data[0] = 0;
        }
        remainder->length = aLength;
        memcpy(remainder->data, work, aLength * 4);
        gsiLargeIntTimerStop(GSLintTimerDiv);
        return gsi_true;
    }
    while ((1 << bit & bData[bLength - 1]) == 0 && bit >= 0) {
        bit--;
    }
    if (bit == -1) {
        gsiLargeIntTimerStop(GSLintTimerDiv);
        return gsi_false;
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
            gsi_u32 *at = work + offset;

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
            gsi_u32 digit = 0;

            if (!gsiLargeIntSubDivide(work + offset, length, bData, bLength, top, &digit)) {
                gsiLargeIntTimerStop(GSLintTimerDiv);
                return gsi_false;
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
    gsiLargeIntTimerStop(GSLintTimerDiv);
    return gsi_true;
}

// Estimates the next word of a quotient: the top words of the dividend over the top word of the divisor, lowered until
// the divisor times it is not more than the dividend, which it then takes away from
static gsi_bool gsiLargeIntSubDivide(gsi_u32 *a, gsi_u32 aLength, const gsi_u32 *b, gsi_u32 bLength, gsi_u32 shift,
                                     gsi_u32 *result) {
    gsLargeInt_t product;
    gsi_u64 estimate;

    gsiLargeIntTimerStart(GSLintTimerSubDivide);
    if (aLength > bLength) {
        estimate = ((gsi_u64)a[aLength - 1] << 32) | (a[aLength - 2] & b[bLength - 1]);
    } else {
        estimate = a[aLength - 1];
    }
    estimate /= b[bLength - 1];
    gsLargeInt_t quotient = { 0 };
    quotient.data[0] = estimate;
    quotient.data[1] = estimate >> 32;
    if (quotient.data[1] != 0) {
        quotient.data[0] = 0xffffffff;
        quotient.data[1] = 0;
    }
    quotient.length = 1;
    if (!gsiLargeIntMult(b, bLength, quotient.data, quotient.length, product.data, &product.length, 0x40)) {
        gsiLargeIntTimerStop(GSLintTimerSubDivide);
        return gsi_false;
    }
    while (gsiLargeIntCompare(product.data, product.length, a, aLength) == 1) {
        quotient.data[0] >>= 1;
        if (!gsiLargeIntMult(b, bLength, quotient.data, quotient.length, product.data, &product.length, 0x40)) {
            gsiLargeIntTimerStop(GSLintTimerSubDivide);
            return gsi_false;
        }
    }
    gsiLargeIntSub(product.data, product.length, a, aLength, a, &aLength);
    *result = quotient.data[0];
    gsiLargeIntTimerStop(GSLintTimerSubDivide);
    return gsi_true;
}

// Squares a number and takes the remainder of the division by the modulus
static gsi_bool gsLargeIntSquareMod(const gsLargeInt_t *value, const gsLargeInt_t *modulus, gsLargeInt_t *result) {
    int count = value->length;
    gsi_u64 carry = 0;
    gsi_u32 bit = 0;
    int length = 0;
    int i;
    int j;
    gsi_bool ok;

    gsiLargeIntTimerStart(GSLintTimerSquareMod);
    gsi_u32 squares[0x80] = { 0 };
    gsi_u32 cross[0x80] = { 0 };
    for (i = 1; i < count; i++) {
        for (j = 0; j < i; j++) {
            gsi_u64 product = (gsi_u64)value->data[i] * value->data[j];
            gsi_u64 total = carry + ((gsi_u64)cross[i + j] + product);

            cross[i + j] = (gsi_u32)total;
            carry = total >> 32;
        }
        if (carry != 0) {
            cross[i + j] = carry;
            carry = 0;
        }
    }
    for (i = 0; i < count * 2; i++) {
        gsi_u32 word = cross[i];

        cross[i] = bit + word * 2;
        bit = (word & 0x80000000) == 0x80000000;
    }
    for (i = 0; i < count; i++) {
        gsi_u64 square = (gsi_u64)value->data[i] * value->data[i];

        squares[i * 2] = (gsi_u32)square;
        squares[i * 2 + 1] = square >> 32;
    }
    length = count * 2;
    gsiLargeIntAdd(cross, count * 2, squares, count * 2, squares, (gsi_u32 *)&length, 0x80);
    ok = gsiLargeIntDiv(squares, length, modulus, NULL, result);
    gsiLargeIntTimerStop(GSLintTimerSquareMod);
    return ok;
}

// Raises a number to a power modulo an odd modulus, working with Montgomery products
gsi_bool gsLargeIntPowerMod(const gsLargeInt_t *base, const gsLargeInt_t *exponent, const gsLargeInt_t *modulus,
                            gsLargeInt_t *result) {
    gsi_u32 inverse;
    int bits;
    int bit;
    int top;
    gsi_bool ok;

    gsiLargeIntTimerStart(GSLintTimerPowerMod);
    gsLargeInt_t radix = { 0 };
    gsLargeInt_t radixMod = { 0 };
    gsLargeInt_t radixSquare = { 0 };
    gsLargeInt_t one = { 0 };
    gsLargeInt_t baseMont = { 0 };
    gsLargeInt_t b;
    gsLargeInt_t e;
    gsLargeInt_t m;
    memcpy(&b, base, sizeof(gsLargeInt_t));
    memcpy(&e, exponent, sizeof(gsLargeInt_t));
    memcpy(&m, modulus, sizeof(gsLargeInt_t));
    gsiLargeIntStripLeadingZeroes(&b);
    gsiLargeIntStripLeadingZeroes(&e);
    gsiLargeIntStripLeadingZeroes(&m);
    if (m.length == 0) {
        result->length = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_false;
    }
    if (m.length == 1 && m.data[0] == 1) {
        result->length = 0;
        result->data[0] = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_true;
    }
    if (e.length == 0) {
        result->length = 1;
        result->data[0] = 1;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_true;
    }
    if ((m.data[0] & 1) == 0) {
        result->length = 0;
        result->data[0] = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_false;
    }
    if (gsiLargeIntCompare(b.data, b.length, m.data, m.length) != -1) {
        gsLargeIntDiv(&b, &m, NULL, &b);
    }
    if (b.length == 0) {
        result->length = 0;
        result->data[0] = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_true;
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
        return gsi_false;
    }
    radix.data[radix.length - 1] = 1;
    gsiLargeIntInverseMod(&m, &inverse);
    ok = gsi_false;
    if (!gsLargeIntDiv(&radix, &m, NULL, &radixMod)) {
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return ok;
    }
    if (!gsLargeIntSquareMod(&radixMod, &m, &radixSquare)) {
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return ok;
    }
    if (!gsiLargeIntMultM(&b, &radixSquare, &m, inverse, &baseMont)) {
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return ok;
    }
    memcpy(result, &radixMod, sizeof(gsLargeInt_t));
    for (bit = top - 1; bit >= 0; bit--) {
        gsi_u32 mask;

        gsiLargeIntMultM(result, result, &m, inverse, result);
        mask = 1 << (bit & 0x1f);
        if (mask == (e.data[bit >> 5] & mask)) {
            gsiLargeIntMultM(result, &baseMont, &m, inverse, result);
        }
    }
    one.length = 1;
    one.data[0] = 1;
    gsiLargeIntMultM(result, &one, &m, inverse, result);
    gsiLargeIntTimerStop(GSLintTimerPowerMod);
    return gsi_true;
}

// The Montgomery product of two numbers: a times b, divided by the radix, modulo the modulus
static gsi_bool gsiLargeIntMultM(gsLargeInt_t *a, gsLargeInt_t *b, const gsLargeInt_t *modulus, gsi_u32 inverse,
                                 gsLargeInt_t *result) {
    gsi_u32 length = 0;
    int count = modulus->length;
    gsi_u32 work[0x80] = { 0 };
    gsi_u32 *p;
    gsi_u32 *q;
    const gsi_u32 *m;
    gsi_u32 *workLast;
    const gsi_u32 *modulusLast;
    gsi_u64 carry;
    gsi_u32 factor;

    if (!gsiLargeIntMult(a->data, a->length, b->data, b->length, work, &length, 0x80)) {
        return gsi_false;
    }
    workLast = work + (modulus->length - 1);
    modulusLast = modulus->data + (modulus->length - 1);
    if (length < modulus->length * 2) {
        memset(work + length, 0, (modulus->length * 2 - length) * 4);
        length = modulus->length * 2;
    }
    for (p = work; p <= workLast; p++) {
        gsi_u64 product;

        carry = 0;
        factor = (gsi_u64)inverse * *p;
        q = p;
        for (m = modulus->data; m <= modulusLast; m++) {
            product = (gsi_u64)factor * *m;
            product = carry + ((gsi_u64)*q + product);
            *q = (gsi_u32)product;
            carry = product >> 32;
            q++;
        }
        for (; carry != 0 && q <= work + length - 1; q++) {
            gsi_u64 sum = (gsi_u64)*q + carry;

            *q = (gsi_u32)sum;
            carry = sum >> 32;
        }
        if (carry != 0) {
            *q = carry;
            length++;
        }
    }
    if (gsiLargeIntCompare(work + count, length - count, modulus->data, modulus->length) != -1) {
        if (!gsiLargeIntSub(modulus->data, modulus->length, work + count, length - count, result->data,
                            (gsi_u32 *)&result->length)) {
            memset(work, 0, sizeof(work));
            memset(result, 0, sizeof(gsLargeInt_t));
            return gsi_false;
        }
    } else {
        memset(result, 0, sizeof(gsLargeInt_t));
        result->length = modulus->length;
        memcpy(result->data, work + count, (length - count) * 4);
        memset(work, 0, sizeof(work));
    }
    return gsi_true;
}

// Finds the word that, with the modulus, makes the Montgomery reduction work: the negated inverse of the lowest word
static gsi_bool gsiLargeIntInverseMod(const gsLargeInt_t *modulus, gsi_u32 *result) {
    gsi_u32 i;
    gsi_u64 x = 2;
    gsi_u64 y = 1;

    for (i = 2; i <= 32; i++) {
        if (x < ((modulus->data[0] * y) & (2 * x - 1))) {
            y += x;
        }
        x <<= 1;
    }
    *result = x - y;
    return gsi_true;
}

// Reads a text of hexadecimal digits into a number
gsi_bool gsLargeIntSetFromHexString(gsLargeInt_t *value, const char *text) {
    gsi_u32 *word = value->data;
    gsi_u32 shift = 0;
    gsi_u32 length = strlen(text);
    gsi_u32 byte;

    if (length == 0) {
        value->length = 0;
        value->data[0] = 0;
        return gsi_true;
    }
    if (length / 2 > 0x100) {
        return gsi_false;
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
    return gsi_true;
}

// Reverses the order of the bytes of a number
gsi_bool gsLargeIntReverseBytes(gsLargeInt_t *value) {
    char *low = (char *)value->data;
    char *high = (char *)(value->data + value->length) - 1;
    char swap;

    if (value->length == 0) {
        return gsi_true;
    }
    while (low < high) {
        swap = *low;
        *low = *high;
        *high = swap;
        low++;
        high--;
    }
    return gsi_true;
}

// Adds the bytes of a number, most significant first, to a hash
void gsLargeIntAddToMD5(const gsLargeInt_t *value, MD5_CTX *context) {
    gsLargeInt_t copy;
    gsi_u32 size;
    gsi_u8 *bytes;

    memcpy(&copy, value, sizeof(gsLargeInt_t));
    size = gsLargeIntGetByteLength(&copy);

    if (size != 0) {
        bytes = (gsi_u8 *)copy.data;
        if ((size & 3) != 0) {
            bytes += 4 - (size & 3);
        }
        gsLargeIntReverseBytes(&copy);
        func_ov011_02171a24(context, bytes, size);
        gsLargeIntReverseBytes(&copy);
    }
}

// How many bytes the number takes
static gsi_u32 gsLargeIntGetByteLength(const gsLargeInt_t *value) {
    int length = value->length;
    int bytes;
    gsi_u32 i;
    gsi_u32 mask;

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
