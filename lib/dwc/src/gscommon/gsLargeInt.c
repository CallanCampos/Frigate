#include "dwc/gsLargeInt.h"
#include "dwc/ext.h"
#include "nitro/std.h"

// GameSpy's large integers (gsLargeInt.c). The ROM names neither the file nor its functions: the file and most
// functions are GameSpy's, as pret's pokerevo Wii build has them (gsLargeInt.s), or the later SDK's (gsLargeIntDiv,
// gsiLargeIntStripLeadingZeroes, gsLargeIntSetFromHexString, gsLargeIntAddToMD5, gsLargeIntGetByteLength); the timers'
// table gTimers and gsiLargeIntInverseMod are the later SDK's too, and the timers' functions are ours

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

static GSLintTimer gTimers[GSLintTimerCount];

static void gsiLargeIntTimerStart(GSLintTimerID index) {
    if (gTimers[index].running == 0) {
        gTimers[index].count++;
        gTimers[index].start = current_time_hires();
        gTimers[index].running = 1;
    }
}

static void gsiLargeIntTimerStop(GSLintTimerID index) {
    if (gTimers[index].running == 1) {
        gTimers[index].total += current_time_hires() - gTimers[index].start;
        gTimers[index].running = 0;
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
    while (value->mLength != 0 && value->mData[value->mLength - 1] == 0) {
        value->mLength--;
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
                    product += temp.mData[k];
                    temp.mData[k] = (gsi_u32)product;
                    product >>= 32;
                    k++;
                    if (k > maxLength || (k == maxLength && product != 0)) {
                        gsiLargeIntTimerStop(GSLintTimerMult);
                        return gsi_false;
                    }
                }
                if (k > (int)temp.mLength) {
                    temp.mLength = k;
                }
            }
        }
    }
    while (temp.mLength != 0 && temp.mData[temp.mLength - 1] == 0) {
        temp.mLength--;
    }
    *destLength = temp.mLength;
    memcpy(dest, temp.mData, *destLength * 4);
    gsiLargeIntTimerStop(GSLintTimerMult);
    return gsi_true;
}

static gsi_bool gsLargeIntDiv(const gsLargeInt_t *a, const gsLargeInt_t *b, gsLargeInt_t *quotient,
                              gsLargeInt_t *remainder) {
    return gsiLargeIntDiv(a->mData, a->mLength, b, quotient, remainder);
}

// Divides a number by another one, giving the quotient and the remainder (either may be NULL)
static gsi_bool gsiLargeIntDiv(const gsi_u32 *a, gsi_u32 aLength, const gsLargeInt_t *b, gsLargeInt_t *quotient,
                               gsLargeInt_t *remainder) {
    const gsi_u32 *bData = b->mData;
    gsi_u32 bLength = b->mLength;
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
            quotient->mData[0] = 0;
            quotient->mLength = 0;
        }
        if (remainder != NULL) {
            remainder->mData[0] = 0;
            remainder->mLength = 0;
        }
        gsiLargeIntTimerStop(GSLintTimerDiv);
        return bLength != 0;
    }
    if (gsiLargeIntCompare(work, aLength, bData, bLength) == -1) {
        if (quotient != NULL) {
            quotient->mLength = 0;
            quotient->mData[0] = 0;
        }
        remainder->mLength = aLength;
        memcpy(remainder->mData, work, aLength * 4);
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
            quotientWork.mData[offset]++;
            if (quotientWork.mLength < offset + length) {
                quotientWork.mLength = offset + length;
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
            quotientWork.mData[offset] += digit;
            if (quotientWork.mLength < offset + length) {
                quotientWork.mLength = offset + length;
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
        memcpy(remainder->mData, work + offset, length * 4);
        remainder->mLength = length;
    } else {
        remainder->mData[0] = 0;
        remainder->mLength = 0;
    }
    if (quotient != NULL) {
        memcpy(quotient->mData, quotientWork.mData, quotientWork.mLength * 4);
        quotient->mLength = quotientWork.mLength;
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
    quotient.mData[0] = estimate;
    quotient.mData[1] = estimate >> 32;
    if (quotient.mData[1] != 0) {
        quotient.mData[0] = 0xffffffff;
        quotient.mData[1] = 0;
    }
    quotient.mLength = 1;
    if (!gsiLargeIntMult(b, bLength, quotient.mData, quotient.mLength, product.mData, &product.mLength, 0x40)) {
        gsiLargeIntTimerStop(GSLintTimerSubDivide);
        return gsi_false;
    }
    while (gsiLargeIntCompare(product.mData, product.mLength, a, aLength) == 1) {
        quotient.mData[0] >>= 1;
        if (!gsiLargeIntMult(b, bLength, quotient.mData, quotient.mLength, product.mData, &product.mLength, 0x40)) {
            gsiLargeIntTimerStop(GSLintTimerSubDivide);
            return gsi_false;
        }
    }
    gsiLargeIntSub(product.mData, product.mLength, a, aLength, a, &aLength);
    *result = quotient.mData[0];
    gsiLargeIntTimerStop(GSLintTimerSubDivide);
    return gsi_true;
}

// Squares a number and takes the remainder of the division by the modulus
static gsi_bool gsLargeIntSquareMod(const gsLargeInt_t *value, const gsLargeInt_t *modulus, gsLargeInt_t *result) {
    int i;
    int j;
    int count = value->mLength;
    gsi_u64 carry = 0;
    int bit = 0;
    gsi_bool ok;
    gsi_u32 length = 0;

    gsiLargeIntTimerStart(GSLintTimerSquareMod);
    gsi_u32 squares[0x80] = { 0 };
    gsi_u32 cross[0x80] = { 0 };
    for (i = 1; i < count; i++) {
        for (j = 0; j < i; j++) {
            carry += (gsi_u64)value->mData[i] * value->mData[j] + cross[i + j];
            cross[i + j] = (gsi_u32)carry;
            carry >>= 32;
        }
        if (carry != 0) {
            cross[i + j] = (gsi_u32)carry;
            carry >>= 32;
        }
    }
    for (i = 0; i < count * 2; i++) {
        j = (cross[i] & 0x80000000) == 0x80000000 ? 1 : 0;
        cross[i] = (cross[i] << 1) + bit;
        bit = j;
    }
    for (i = 0; i < count; i++) {
        carry = (gsi_u64)value->mData[i] * value->mData[i];
        squares[i * 2] = (gsi_u32)carry;
        squares[i * 2 + 1] = (gsi_u32)(carry >> 32);
    }
    length = count * 2;
    gsiLargeIntAdd(cross, length, squares, length, squares, &length, 0x80);
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
    if (m.mLength == 0) {
        result->mLength = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_false;
    }
    if (m.mLength == 1 && m.mData[0] == 1) {
        result->mLength = 0;
        result->mData[0] = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_true;
    }
    if (e.mLength == 0) {
        result->mLength = 1;
        result->mData[0] = 1;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_true;
    }
    if ((m.mData[0] & 1) == 0) {
        result->mLength = 0;
        result->mData[0] = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_false;
    }
    if (gsiLargeIntCompare(b.mData, b.mLength, m.mData, m.mLength) != -1) {
        gsLargeIntDiv(&b, &m, NULL, &b);
    }
    if (b.mLength == 0) {
        result->mLength = 0;
        result->mData[0] = 0;
        gsiLargeIntTimerStop(GSLintTimerPowerMod);
        return gsi_true;
    }
    e.mLength--;
    bits = 32;
    if ((e.mData[e.mLength] & 0x80000000) == 0) {
        do {
            bits--;
        } while (((1 << (bits - 1)) & e.mData[e.mLength]) == 0);
    }
    top = bits + e.mLength * 32;
    radix.mLength = m.mLength + 1;
    if (radix.mLength > 0x40) {
        return gsi_false;
    }
    radix.mData[radix.mLength - 1] = 1;
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
        if (mask == (e.mData[bit >> 5] & mask)) {
            gsiLargeIntMultM(result, &baseMont, &m, inverse, result);
        }
    }
    one.mLength = 1;
    one.mData[0] = 1;
    gsiLargeIntMultM(result, &one, &m, inverse, result);
    gsiLargeIntTimerStop(GSLintTimerPowerMod);
    return gsi_true;
}

// The Montgomery product of two numbers: a times b, divided by the radix, modulo the modulus
static gsi_bool gsiLargeIntMultM(gsLargeInt_t *a, gsLargeInt_t *b, const gsLargeInt_t *modulus, gsi_u32 inverse,
                                 gsLargeInt_t *result) {
    gsi_u32 length = 0;
    int count = modulus->mLength;
    gsi_u32 work[0x80] = { 0 };
    gsi_u32 *p;
    gsi_u32 *q;
    const gsi_u32 *m;
    gsi_u32 *workLast;
    const gsi_u32 *modulusLast;
    gsi_u64 carry;
    gsi_u32 factor;

    if (!gsiLargeIntMult(a->mData, a->mLength, b->mData, b->mLength, work, &length, 0x80)) {
        return gsi_false;
    }
    workLast = work + (modulus->mLength - 1);
    modulusLast = modulus->mData + (modulus->mLength - 1);
    if (length < modulus->mLength * 2) {
        memset(work + length, 0, (modulus->mLength * 2 - length) * 4);
        length = modulus->mLength * 2;
    }
    for (p = work; p <= workLast; p++) {
        gsi_u64 product;

        carry = 0;
        factor = (gsi_u64)inverse * *p;
        q = p;
        m = modulus->mData;
        if (m <= modulusLast) {
            for (; m <= modulusLast; m++) {
                product = (gsi_u64)factor * *m;
                product = carry + ((gsi_u64)*q + product);
                *q = (gsi_u32)product;
                carry = product >> 32;
                q++;
            }
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
    if (gsiLargeIntCompare(work + count, length - count, modulus->mData, modulus->mLength) != -1) {
        if (!gsiLargeIntSub(modulus->mData, modulus->mLength, work + count, length - count, result->mData,
                            (gsi_u32 *)&result->mLength)) {
            memset(work, 0, sizeof(work));
            memset(result, 0, sizeof(gsLargeInt_t));
            return gsi_false;
        }
    } else {
        memset(result, 0, sizeof(gsLargeInt_t));
        result->mLength = modulus->mLength;
        memcpy(result->mData, work + count, (length - count) * 4);
        memset(work, 0, sizeof(work));
    }
    return gsi_true;
}

// Finds the word that, with the modulus, makes the Montgomery reduction work: the negated inverse of the lowest word
static gsi_bool gsiLargeIntInverseMod(const gsLargeInt_t *modulus, gsi_u32 *result) {
    gsi_u64 x = 2;
    gsi_u64 y = 1;
    gsi_u32 i;

    for (i = 2; i <= 32; i++) {
        gsi_u64 mask = (x << 1) - 1;
        gsi_u64 product = (gsi_u64)modulus->mData[0] * y;

        if (x < (product & mask)) {
            y += x;
        }
        x <<= 1;
    }
    *result = x - y;
    return gsi_true;
}

// Reads a text of hexadecimal digits into a number
gsi_bool gsLargeIntSetFromHexString(gsLargeInt_t *value, const char *text) {
    gsi_u32 *word = value->mData;
    gsi_u32 shift = 0;
    gsi_u32 length = strlen(text);
    gsi_u32 byte;

    if (length == 0) {
        value->mLength = 0;
        value->mData[0] = 0;
        return gsi_true;
    }
    if (length / 2 > 0x100) {
        return gsi_false;
    }
    value->mLength = (length + 7) / 8;
    value->mData[value->mLength - 1] = 0;
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
    char *low = (char *)value->mData;
    char *high = (char *)(value->mData + value->mLength) - 1;
    char swap;

    if (value->mLength == 0) {
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
        bytes = (gsi_u8 *)copy.mData;
        if ((size & 3) != 0) {
            bytes += 4 - (size & 3);
        }
        gsLargeIntReverseBytes(&copy);
        MD5Update(context, bytes, size);
        gsLargeIntReverseBytes(&copy);
    }
}

// How many bytes the number takes
static gsi_u32 gsLargeIntGetByteLength(const gsLargeInt_t *value) {
    int length = value->mLength;
    int bytes;
    gsi_u32 i;
    gsi_u32 mask;

    while (length > 0 && value->mData[length - 1] == 0) {
        length--;
    }
    if (length == 0) {
        return 0;
    }
    bytes = length * 4;
    mask = 0xff;
    for (i = 1; i < 4; i++) {
        if (value->mData[length - 1] <= mask) {
            bytes -= 4 - i;
            break;
        }
        mask <<= 8;
        mask |= 0xff;
    }
    return bytes;
}
