#include "nitro/mi.h"
#include "nitrowifi/cps.h"
#include "ssl_internal.h"

// A multi-precision integer is a little-endian array of n u16 digits, every length given explicitly by the caller.
// No swan name for the object or any of its functions; they carry NitroWiFi's own names, as ntrtwl's reconstruction of
// NitroWiFi has them

static int count_digits(const u16 *digits, int n) {
    while (n != 0 && digits[n - 1] == 0) {
        n--;
    }
    return n;
}

s32 CPSi_big_sign(const u16 *digits, int n) {
    if (digits[n - 1] & 0x8000) {
        return -1;
    }
    return count_digits(digits, n) != 0;
}

void CPSi_big_add(u16 *dst, const u16 *a, const u16 *b, int n) {
    int lenB;
    int len;
    int i;
    int carry;
    u32 sum;

    len = count_digits(a, n);
    lenB = count_digits(b, n);
    if (len < lenB) {
        len = lenB;
    }
    if (len != n) {
        len++;
    }
    carry = 0;
    for (i = 0; i < len; i++) {
        sum = a[i] + b[i] + carry;
        dst[i] = (u16)sum;
        carry = sum >> 16;
    }
    if (dst != a && dst != b) {
        MI_CpuFill8(dst + i, 0, (n - i) * 2);
    }
}

// Adds value into dst/src starting at digit 0, propagating the carry, and copies the rest of src unchanged
static void CPSi_big_add_small(u16 *dst, const u16 *src, u32 value, int n) {
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

void CPSi_big_negate(u16 *digits, int n) {
    int i;

    for (i = 0; i < n; i++) {
        digits[i] = ~digits[i];
    }
    CPSi_big_add_small(digits, digits, 1, n);
}

void CPSi_big_sub(u16 *dst, const u16 *a, const u16 *b, int n) {
    int lenA = count_digits(a, n);
    int lenB = count_digits(b, n);
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
        MI_CpuFill8(dst + i, 0, (n - i) * 2);
    }
}

// Subtracts value from src starting at digit 0, propagating the borrow, and copies the rest of src unchanged
static void CPSi_big_sub_small(u16 *dst, const u16 *src, u32 value, int n) {
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

static int CPSi_big_compare(const u16 *a, const u16 *b, int n) {
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
static void CPSi_big_add_part(u16 *digits, u32 value, int index, int n) {
    while (value != 0 && index < n) {
        value += digits[index];
        digits[index] = (u16)value;
        value >>= 16;
        index++;
    }
}

void CPSi_big_mult(u16 *dst, const u16 *a, const u16 *b, int n) {
    int lenA;
    int lenB;
    int i;
    int j;

    MI_CpuFill8(dst, 0, n * 2);
    lenA = count_digits(a, n);
    lenB = count_digits(b, n);
    for (i = 0; i < lenB; i++) {
        for (j = 0; j < lenA && j < n - i; j++) {
            CPSi_big_add_part(dst, a[j] * b[i], i + j, n);
        }
    }
}

static void CPSi_big_mult_small(u16 *dst, const u16 *src, u16 value, int n) {
    int len = count_digits(src, n);
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
    MI_CpuFill8(dst + i, 0, (n - i) * 2);
}

static void CPSi_big_sqr(u16 *dst, const u16 *src, int n) {
    int len = count_digits(src, n);
    int doubleLen = len * 2;
    int k;
    int outer;

    if (doubleLen < n) {
        MI_CpuFill8(dst + doubleLen, 0, (n - doubleLen) * 2);
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
                CPSi_big_add_part(dst, product * 2, outer + inner, n);
            } else {
                CPSi_big_add_part(dst, product, outer + inner, n);
                CPSi_big_add_part(dst, product, outer + inner, n);
            }
        }
    }
}

// The top one, two or three digits ending at p, or four for get64bits, as a 64-bit value with p's digit at bit 32
// (bit 48 for get64bits). Black 1's NitroWiFi has them as functions of these names, in ARM assembly; here they are
// inlined.
static inline u64 get48bits_1(const u16 *p) {
    return (u64)p[0] << 32;
}

static inline u64 get48bits_2(const u16 *p) {
    return ((u64)p[0] << 32) + ((u64)p[-1] << 16);
}

static inline u64 get48bits_3(const u16 *p) {
    return ((u64)p[0] << 32) + ((u64)p[-1] << 16) + (u64)p[-2];
}

static inline u64 get64bits(const u16 *p) {
    return ((u64)p[0] << 48) + ((u64)p[-1] << 32) + ((u64)p[-2] << 16) + (u64)p[-3];
}

// Divides n-digit operands into optional quotient/remainder outputs. Three n-digit scratch regions follow;
// when both operands have n significant digits, the original numerator window also reads workBuf[3*n].
// That extra digit is not initialized here; padded operands avoid this access.
void CPSi_big_div(u16 *quotient, const u16 *dividend, const u16 *divisor, u16 *remainder, int n, u16 *workBuf) {
    int lenA;
    int lenB;
    int pos;
    u64 divisorTop;
    u32 digit;
    u16 *qbuf;
    u16 *rem;

    qbuf = workBuf + n;
    rem = qbuf + n;
    MI_CpuFill8(qbuf, 0, n * 4);
    lenA = count_digits(dividend, n);
    lenB = count_digits(divisor, n);
    if (lenA > 0 && lenB > 0) {
        pos = lenB + (n - lenA) - 1;
        if (pos >= n) {
            MI_CpuCopy8(dividend, rem, n * 2);
        } else {
            MI_CpuCopy8(dividend, qbuf + pos, lenA * 2);
            if (lenB > 2) {
                divisorTop = get48bits_3(divisor + lenB - 1);
            } else if (lenB > 1) {
                divisorTop = get48bits_2(divisor + lenB - 1);
            } else {
                divisorTop = get48bits_1(divisor + lenB - 1);
            }
            for (; pos < n; pos++) {
                memmove(qbuf + 1, qbuf, (n * 2 - 1) * 2);
                digit = get64bits(rem + lenB) / divisorTop;
                if (digit > 0xffff) {
                    digit = 0xffff;
                }
                for (;;) {
                    CPSi_big_mult_small(workBuf, divisor, (u16)digit, n);
                    if (CPSi_big_compare(rem, workBuf, n) >= 0) {
                        break;
                    }
                    digit--;
                }
                CPSi_big_sub(rem, rem, workBuf, n);
                *qbuf = digit;
            }
        }
    }
    if (quotient != NULL) {
        MI_CpuCopy8(qbuf, quotient, n * 2);
    }
    if (remainder != NULL) {
        MI_CpuCopy8(rem, remainder, n * 2);
    }
}

void CPSi_big_power(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus) {
    u16 *work;
    u16 *temp;
    int bit;

    work = CPSiAlloc(n * sizeof(u16) * 4);
    if (work == NULL) {
        return;
    }
    temp = work + n;
    MI_CpuFill8(dst + 1, 0, (n - 1) * sizeof(u16));
    dst[0] = 1;
    for (bit = (n - count_digits(exponent, n)) * 16; bit < n * sizeof(u16) * 8; bit++) {
        if (exponent[n - (bit >> 4) - 1] & (0x8000u >> (bit & 0xf))) {
            MI_CpuCopy8(base, dst, n * sizeof(u16));
            bit++;
            break;
        }
    }
    for (; bit < n * sizeof(u16) * 8; bit++) {
        CPSi_big_sqr(work, dst, n);
        MI_CpuCopy8(work, dst, n * sizeof(u16));
        if (modulus != NULL) {
            CPSi_big_div(NULL, dst, modulus, dst, n, temp);
        }
        if (exponent[n - (bit >> 4) - 1] & (0x8000u >> (bit & 0xf))) {
            CPSi_big_mult(work, dst, base, n);
            MI_CpuCopy8(work, dst, n * sizeof(u16));
            if (modulus != NULL) {
                CPSi_big_div(NULL, dst, modulus, dst, n, temp);
            }
        }
    }
    CPSiFree(work);
}

// Extended Euclidean coefficient recurrence, normalized with the original modulus at the end.
// The caller zeroes scratch: six n-digit regions precede nominal 3*n division workspace (9*n total).
// That partition alone does not establish sufficient headroom for every CPSi_big_div input.
static void CPSi_big_modinv(u16 *result, const u16 *value, const u16 *modulus, int n, u16 *scratch) {
    u16 *quotient;
    u16 *previousRemainder;
    u16 *currentCoefficient;
    u16 *previousCoefficient;
    u16 *temporary;
    u16 *divisionWork;

    quotient = scratch + n;
    previousRemainder = quotient + n;
    currentCoefficient = previousRemainder + n;
    previousCoefficient = currentCoefficient + n;
    temporary = previousCoefficient + n;
    divisionWork = temporary + n;

    MI_CpuCopy8(value, scratch, n * 2);
    MI_CpuCopy8(modulus, previousRemainder, n * 2);
    currentCoefficient[0] = 1;
    while (CPSi_big_sign(scratch, n) > 0) {
        CPSi_big_div(quotient, previousRemainder, scratch, temporary, n, divisionWork);
        MI_CpuCopy8(scratch, previousRemainder, n * 2);
        MI_CpuCopy8(temporary, scratch, n * 2);
        CPSi_big_mult(temporary, quotient, currentCoefficient, n);
        CPSi_big_sub(temporary, previousCoefficient, temporary, n);
        MI_CpuCopy8(currentCoefficient, previousCoefficient, n * 2);
        MI_CpuCopy8(temporary, currentCoefficient, n * 2);
    }
    CPSi_big_add(previousCoefficient, previousCoefficient, modulus, n);
    CPSi_big_div(NULL, previousCoefficient, modulus, result, n, divisionWork);
}

// Montgomery reduction with R = 2^(16*modLen) and mu = -modulus^(-1) mod R, as constructed by ModExpFast.
// After optional squaring/multiplication, q = (acc*mu) mod R makes acc + q*modulus divisible by R.
// A NULL multiplier performs reduction alone; q and t are scratch. The names describe the recovered roles.
static void CPSi_big_montmult(u16 *acc, u16 *scratch, const u16 *multiplier, int n, int modLen, const u16 *modulus,
                              const u16 *mu, u16 *q, u16 *t) {
    MI_CpuCopy8(acc, scratch, n * 2);
    if (multiplier == BIGINT_SQUARE) {
        CPSi_big_sqr(acc, scratch, n);
    } else if (multiplier != NULL) {
        CPSi_big_mult(acc, scratch, multiplier, n);
    }

    CPSi_big_mult(q, acc, mu, modLen);
    MI_CpuFill8(q + modLen, 0, (n - modLen) * 2);
    CPSi_big_mult(t, q, modulus, n);
    CPSi_big_add(acc, acc, t, n);
    memmove(acc, acc + modLen, (n - modLen) * 2);
    MI_CpuFill8(acc + n - modLen, 0, modLen * 2);

    switch (CPSi_big_compare(acc, modulus, n)) {
    case 0:
        MI_CpuFill8(acc, 0, n * 2);
        break;
    case 1:
        CPSi_big_sub(acc, acc, modulus, n);
        break;
    }
}

void CPSi_big_montpower(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus) {
    int modLen;
    int bit;
    u16 *work;
    u16 *mu;
    u16 *montBase;
    u16 *buf3;
    u16 *buf4;
    u16 *buf5;
    u16 *workBuf;

    work = CPSiAlloc(n * 22);
    if (work == NULL) {
        return;
    }
    MI_CpuFill8(work, 0, n * 22);
    mu = work + n;
    montBase = mu + n;
    buf3 = montBase + n;
    buf4 = buf3 + n;
    buf5 = buf4 + n;
    workBuf = buf5 + n;
    modLen = count_digits(modulus, n);
    work[modLen] = 1;
    CPSi_big_modinv(mu, work, modulus, n, montBase);
    CPSi_big_mult(buf3, work, mu, n);
    CPSi_big_sub_small(mu, buf3, 1, n);
    CPSi_big_div(mu, mu, modulus, NULL, n, workBuf);
    CPSi_big_mult(montBase, base, work, n);
    CPSi_big_div(NULL, montBase, modulus, montBase, n, workBuf);
    CPSi_big_div(NULL, work, modulus, dst, n, workBuf);
    for (bit = 0; bit < modLen * sizeof(u16) * 8; bit++) {
        CPSi_big_montmult(dst, buf5, BIGINT_SQUARE, n, modLen, modulus, mu, buf3, buf4);
        if (exponent[modLen - (bit >> 4) - 1] & (0x8000u >> (bit & 0xf))) {
            CPSi_big_montmult(dst, buf5, montBase, n, modLen, modulus, mu, buf3, buf4);
        }
    }
    CPSi_big_montmult(dst, buf5, NULL, n, modLen, modulus, mu, buf3, buf4);
    CPSiFree(work);
}

void CPSi_big_from_char(u16 *dst, const u8 *src, int nbytes, int n) {
    MI_CpuFill8(dst, 0, n * 2);
    src += nbytes - 1;
    while (nbytes > 1) {
        *dst++ = (u16)(src[0] + (src[-1] << 8));
        src -= 2;
        nbytes -= 2;
    }
    if (nbytes > 0) {
        *dst = *src;
    }
}

// The original accepts the source digit count n but overwrites its register before reading it.
void CPSi_char_from_big(u8 *dst, u16 *src, int nbytes, int n) {
    u8 *hi;

    dst += nbytes - 1;
    while (nbytes > 1) {
        hi = dst - 1;
        *dst = (u8)*src;
        dst -= 2;
        *hi = (u8)(*src++ >> 8);
        nbytes -= 2;
    }
    if (nbytes > 0) {
        *dst = (u8)*src;
    }
}
