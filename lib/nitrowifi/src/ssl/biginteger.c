#include "ssl_internal.h"

#include "gfl/std.h"
#include "nitrowifi/cps.h"

// A multi-precision integer is a little-endian array of n u16 digits, every length given explicitly by the caller.
// No swan name for the object or any of its functions; named for what each does from the asm's shape

// The C library's memmove, which the main module links
void *memmove(void *dest, const void *src, u32 size);

static int BigInti_GetLength(const u16 *digits, int n) {
    while (n != 0 && digits[n - 1] == 0) {
        n--;
    }
    return n;
}

s32 BigInt_GetSign(const u16 *digits, int n) {
    if (digits[n - 1] & 0x8000) {
        return -1;
    }
    return BigInti_GetLength(digits, n) != 0;
}

void BigInt_Add(u16 *dst, const u16 *a, const u16 *b, int n) {
    int lenA = BigInti_GetLength(a, n);
    int lenB = BigInti_GetLength(b, n);
    int len = lenA >= lenB ? lenA : lenB;
    u32 carry;
    int i;

    if (len != n) {
        len++;
    }
    carry = 0;
    for (i = 0; i < len; i++) {
        u32 sum = carry + a[i] + b[i];
        dst[i] = (u16)sum;
        carry = sum >> 16;
    }
    if (dst != a && dst != b) {
        sys_memset(dst + len, 0, (n - len) * 2);
    }
}

// Adds value into dst/src starting at digit 0, propagating the carry, and copies the rest of src unchanged
static void BigInti_AddDigit(u16 *dst, const u16 *src, u32 value, int n) {
    int i;

    for (i = 0; i < n; i++) {
        value += src[i];
        dst[i] = (u16)value;
        value >>= 16;
        if (value == 0) {
            break;
        }
    }
    if (dst != src) {
        for (i++; i < n; i++) {
            dst[i] = src[i];
        }
    }
}

void BigInt_Negate(u16 *digits, int n) {
    int i;

    for (i = 0; i < n; i++) {
        digits[i] = ~digits[i];
    }
    BigInti_AddDigit(digits, digits, 1, n);
}

void BigInt_Sub(u16 *dst, const u16 *a, const u16 *b, int n) {
    int lenA = BigInti_GetLength(a, n);
    int lenB = BigInti_GetLength(b, n);
    int len = lenA >= lenB ? lenA : lenB;
    s32 borrow;
    int i;

    if (len != n) {
        len++;
    }
    borrow = 0;
    i = 0;
    while (i < len || (i < n && borrow != 0)) {
        borrow += (s32)a[i] - b[i];
        dst[i] = (u16)borrow;
        borrow >>= 16;
        i++;
    }
    if (dst != a && dst != b) {
        sys_memset(dst + i, 0, (n - i) * 2);
    }
}

// Subtracts value from src starting at digit 0, propagating the borrow, and copies the rest of src unchanged
static void BigInti_SubDigit(u16 *dst, const u16 *src, u32 value, int n) {
    int i;

    for (i = 0; i < n; i++) {
        value = src[i] - value;
        dst[i] = (u16)value;
        value = (value >> 16) & 1;
        if (value == 0) {
            break;
        }
    }
    if (dst != src) {
        for (i++; i < n; i++) {
            dst[i] = src[i];
        }
    }
}

static int BigInti_Compare(const u16 *a, const u16 *b, int n) {
    int i;

    for (i = n - 1; i >= 0; i--) {
        if (a[i] > b[i]) {
            return 1;
        }
        if (a[i] < b[i]) {
            return -1;
        }
    }
    return 0;
}

// Adds value into digits starting at index, propagating the carry, without touching digits below index
static void BigInti_AddDigitFrom(u16 *digits, u32 value, int index, int n) {
    while (value != 0 && index < n) {
        value += digits[index];
        digits[index] = (u16)value;
        value >>= 16;
        index++;
    }
}

void BigInt_Mul(u16 *dst, const u16 *a, const u16 *b, int n) {
    int lenA, lenB;
    int i;

    sys_memset(dst, 0, n * 2);
    lenA = BigInti_GetLength(a, n);
    lenB = BigInti_GetLength(b, n);
    for (i = 0; i < lenB; i++) {
        int j = 0;
        int maxJ = n - i;

        for (; j < lenA && j < maxJ; j++) {
            BigInti_AddDigitFrom(dst, (u32)a[j] * b[i], i + j, n);
        }
    }
}

static void BigInti_MulDigit(u16 *dst, const u16 *src, u32 value, int n) {
    int len = BigInti_GetLength(src, n);
    u32 carry = 0;
    int i;

    for (i = 0; i < len; i++) {
        carry += src[i] * value;
        dst[i] = (u16)carry;
        carry >>= 16;
    }
    if (i < n) {
        dst[i++] = (u16)carry;
    }
    sys_memset(dst + i, 0, (n - i) * 2);
}

static void BigInti_Square(u16 *dst, const u16 *src, int n) {
    int len = BigInti_GetLength(src, n);
    int doubleLen = len * 2;
    int k;
    int outer;

    if (doubleLen < n) {
        sys_memset(dst + doubleLen, 0, (n - doubleLen) * 2);
    }

    for (k = 0; k < len; k++) {
        int idx = k * 2;
        u32 sq;

        if (idx >= n) {
            break;
        }
        sq = (u32)src[k] * src[k];
        dst[idx] = (u16)sq;
        if (idx == n - 1) {
            break;
        }
        dst[idx + 1] = (u16)(sq >> 16);
    }

    for (outer = 0; outer < len; outer++) {
        int inner;

        for (inner = outer + 1; inner < len && outer + inner < n; inner++) {
            u32 product = (u32)src[inner] * src[outer];

            if (product <= 0x7fff8000) {
                BigInti_AddDigitFrom(dst, product * 2, outer + inner, n);
            } else {
                BigInti_AddDigitFrom(dst, product, outer + inner, n);
                BigInti_AddDigitFrom(dst, product, outer + inner, n);
            }
        }
    }
}

// Divides n-digit operands into optional quotient/remainder outputs. Three n-digit scratch regions follow;
// when both operands have n significant digits, the original numerator window also reads workBuf[3*n].
// That extra digit is not initialized here; padded operands avoid this access.
void BigInt_Div(u16 *quotient, const u16 *dividend, const u16 *divisor, u16 *remainder, int n, u16 *workBuf) {
    u16 *qbuf = workBuf + n;
    u16 *rem = qbuf + n;
    int lenA = BigInti_GetLength(dividend, n);
    int lenB = BigInti_GetLength(divisor, n);

    sys_memset(qbuf, 0, n * 4);

    if (lenA > 0 && lenB > 0) {
        int pos = lenB + (n - lenA) - 1;

        if (pos >= n) {
            sys_memcpy(dividend, rem, n * 2);
        } else {
            u64 divisorTop;
            u16 *top;
            int span = (2 * n - 1) * 2;

            sys_memcpy(dividend, qbuf + pos, lenA * 2);

            if (lenB > 2) {
                divisorTop = ((u64)divisor[lenB - 1] << 32) + ((u64)divisor[lenB - 2] << 16) + divisor[lenB - 3];
            } else if (lenB > 1) {
                divisorTop = ((u64)divisor[lenB - 1] << 32) + ((u64)divisor[lenB - 2] << 16);
            } else {
                divisorTop = (u64)divisor[lenB - 1] << 32;
            }

            top = rem + lenB;

            for (; pos < n; pos++) {
                u64 numerator;
                u32 digit;

                memmove(qbuf + 1, qbuf, span);
                numerator =
                    (u64)top[-3] + ((u64)top[-2] << 16) + ((u64)top[-1] << 32) + ((u64)top[0] << 48);
                digit = (u32)(numerator / divisorTop);
                if (digit > 0xffff) {
                    digit = 0xffff;
                }

                for (;;) {
                    BigInti_MulDigit(workBuf, divisor, digit, n);
                    if (BigInti_Compare(rem, workBuf, n) >= 0) {
                        break;
                    }
                    digit--;
                }
                BigInt_Sub(rem, rem, workBuf, n);
                qbuf[0] = (u16)digit;
            }
        }
    }

    if (quotient != NULL) {
        sys_memcpy(qbuf, quotient, n * 2);
    }
    if (remainder != NULL) {
        sys_memcpy(rem, remainder, n * 2);
    }
}

void BigInt_ModExp(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus) {
    u16 *work = CPSiAlloc(n * 8);
    u16 *temp;
    int expLen;
    u32 totalBits;
    u32 bit;

    if (work == NULL) {
        return;
    }
    temp = work + n;

    sys_memset(dst + 1, 0, (n - 1) * 2);
    dst[0] = 1;

    expLen = BigInti_GetLength(exponent, n);
    bit = (n - expLen) * 16;
    totalBits = n * 16;

    while (bit < totalBits) {
        if (exponent[n - (bit >> 4) - 1] & (0x8000u >> (bit & 0xf))) {
            sys_memcpy(base, dst, n * 2);
            bit++;
            break;
        }
        bit++;
    }

    while (bit < totalBits) {
        BigInti_Square(work, dst, n);
        sys_memcpy(work, dst, n * 2);
        if (modulus != NULL) {
            BigInt_Div(NULL, dst, modulus, dst, n, temp);
        }
        if (exponent[n - (bit >> 4) - 1] & (0x8000u >> (bit & 0xf))) {
            BigInt_Mul(work, dst, base, n);
            sys_memcpy(work, dst, n * 2);
            if (modulus != NULL) {
                BigInt_Div(NULL, dst, modulus, dst, n, temp);
            }
        }
        bit++;
    }

    CPSiFree(work);
}

// Extended Euclidean coefficient recurrence, normalized with the original modulus at the end.
// The caller zeroes scratch: six n-digit regions precede nominal 3*n division workspace (9*n total).
// That partition alone does not establish sufficient headroom for every BigInt_Div input.
static void BigInti_ModInverse(u16 *result, const u16 *value, const u16 *modulus, int n, u16 *scratch) {
    u16 *currentRemainder = scratch;
    u16 *quotient = scratch + n;
    u16 *previousRemainder = scratch + n * 2;
    u16 *currentCoefficient = scratch + n * 3;
    u16 *previousCoefficient = scratch + n * 4;
    u16 *temporary = scratch + n * 5;
    u16 *divisionWork = scratch + n * 6;

    sys_memcpy(value, currentRemainder, n * 2);
    sys_memcpy(modulus, previousRemainder, n * 2);
    currentCoefficient[0] = 1;

    if (BigInt_GetSign(currentRemainder, n) > 0) {
        do {
            BigInt_Div(quotient, previousRemainder, currentRemainder, temporary, n, divisionWork);
            sys_memcpy(currentRemainder, previousRemainder, n * 2);
            sys_memcpy(temporary, currentRemainder, n * 2);
            BigInt_Mul(temporary, quotient, currentCoefficient, n);
            BigInt_Sub(temporary, previousCoefficient, temporary, n);
            sys_memcpy(currentCoefficient, previousCoefficient, n * 2);
            sys_memcpy(temporary, currentCoefficient, n * 2);
        } while (BigInt_GetSign(currentRemainder, n) > 0);
    }

    BigInt_Add(previousCoefficient, previousCoefficient, modulus, n);
    BigInt_Div(NULL, previousCoefficient, modulus, result, n, divisionWork);
}

// Montgomery reduction with R = 2^(16*modLen) and mu = -modulus^(-1) mod R, as constructed by ModExpFast.
// After optional squaring/multiplication, q = (acc*mu) mod R makes acc + q*modulus divisible by R.
// A NULL multiplier performs reduction alone; q and t are scratch. The names describe the recovered roles.
static void BigInti_ModExpStep(u16 *acc, u16 *scratch, const u16 *multiplier, int n, int modLen, const u16 *modulus,
                               const u16 *mu, u16 *q, u16 *t) {
    sys_memcpy(acc, scratch, n * 2);
    if (multiplier == BIGINT_SQUARE) {
        BigInti_Square(acc, scratch, n);
    } else if (multiplier != NULL) {
        BigInt_Mul(acc, scratch, multiplier, n);
    }

    BigInt_Mul(q, acc, mu, modLen);
    sys_memset(q + modLen, 0, (n - modLen) * 2);
    BigInt_Mul(t, q, modulus, n);
    BigInt_Add(acc, acc, t, n);
    memmove(acc, acc + modLen, (n - modLen) * 2);
    sys_memset(acc + (n - modLen), 0, modLen * 2);

    switch (BigInti_Compare(acc, modulus, n)) {
    case 0:
        sys_memset(acc, 0, n * 2);
        break;
    case 1:
        BigInt_Sub(acc, acc, modulus, n);
        break;
    }
}

void BigInt_ModExpFast(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus) {
    u16 *work = CPSiAlloc(n * 22);
    u16 *mu;
    u16 *montBase;
    u16 *buf3;
    u16 *buf4;
    u16 *buf5;
    u16 *workBuf;
    int modLen;
    u32 totalBits;
    u32 bit;

    if (work == NULL) {
        return;
    }
    sys_memset(work, 0, n * 22);

    mu = work + n;
    montBase = mu + n;
    buf3 = montBase + n;
    buf4 = buf3 + n;
    buf5 = buf4 + n;
    workBuf = buf5 + n;

    modLen = BigInti_GetLength(modulus, n);
    work[modLen] = 1;

    BigInti_ModInverse(mu, work, modulus, n, montBase);
    BigInt_Mul(buf3, work, mu, n);
    BigInti_SubDigit(mu, buf3, 1, n);
    BigInt_Div(mu, mu, modulus, NULL, n, workBuf);

    BigInt_Mul(montBase, base, work, n);
    BigInt_Div(NULL, montBase, modulus, montBase, n, workBuf);

    BigInt_Div(NULL, work, modulus, dst, n, workBuf);

    totalBits = modLen * 16;
    for (bit = 0; bit < totalBits; bit++) {
        BigInti_ModExpStep(dst, buf5, BIGINT_SQUARE, n, modLen, modulus, mu, buf3, buf4);
        if (exponent[modLen - (bit >> 4) - 1] & (0x8000u >> (bit & 0xf))) {
            BigInti_ModExpStep(dst, buf5, montBase, n, modLen, modulus, mu, buf3, buf4);
        }
    }
    BigInti_ModExpStep(dst, buf5, NULL, n, modLen, modulus, mu, buf3, buf4);

    CPSiFree(work);
}

void BigInt_BytesToDigits(u16 *dst, const u8 *src, int nbytes, int n) {
    const u8 *p;

    sys_memset(dst, 0, n * 2);
    p = &src[nbytes - 1];
    while (nbytes > 1) {
        u16 hi = p[-1];
        u16 lo = p[0];

        nbytes -= 2;
        *dst++ = (u16)((hi << 8) + lo);
        p -= 2;
    }
    if (nbytes > 0) {
        *dst = p[0];
    }
}

// The original accepts the source digit count n but overwrites its register before reading it.
void BigInt_DigitsToBytes(u8 *dst, const u16 *src, int nbytes, int n) {
    u8 *p = dst + (nbytes - 1);

    (void)n;
    while (nbytes > 1) {
        u8 *hi = p - 1;

        *p = (u8)*src;
        nbytes -= 2;
        p -= 2;
        *hi = (u8)(*src >> 8);
        src++;
    }
    if (nbytes > 0) {
        *p = (u8)*src;
    }
}
