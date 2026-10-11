#include "dwc/crypt.h"

#include "crypt_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The signature check of overlay 189 (ARM code beside the HTTP client): the server's answer to a Pokémon's check is signed,
// and this library verifies the signature with an RSA public key, a SHA-1 digest and a library of big numbers. The ROM
// names none of it, so the names say what the code does; they are none of the library's own.
//
// A big number keeps its words, the least significant first, with a length in words that has no zero word on top, and
// the sign apart. The calls that need working numbers take them from a pool, which hands out the next of its twelve
// numbers and takes them back in order.

#define BIGNUM_HEAP 0x1
#define BIGNUM_STATIC 0x2
#define BIGNUM_FREED 0x8000

#define POOL_SIZE 12
#define POOL_HEAP 0x1
#define POOL_CANCELED 0x4000

typedef struct BigNum {
    u32 *digits;
    int length;
    int capacity;
    int negative;
    u32 flags;
} BigNum;

typedef struct CryptProgress CryptProgress;
struct CryptProgress {
    int (*func)(CryptProgress *progress, int a1, int count);
};

typedef struct BigNumPool {
    int used;
    BigNum numbers[POOL_SIZE];
    u8 unkF4[0x14];
    u32 flags;
    CryptProgress *progress;
} BigNumPool;

// Divides by a number whose reciprocal is kept, to reduce a product with a few subtractions: the modulus, its reciprocal,
// the modulus' length in bits and the length in bits that the reciprocal is for
typedef struct Reducer {
    BigNum modulus;
    BigNum reciprocal;
    int modulusBits;
    int reciprocalBits;
    u32 flags;
} Reducer;

// The numbers of the Montgomery form: the modulus, R squared and the inverse that the reduction needs
typedef struct Montgomery {
    int unk0;
    int unk4;
    int words;
    BigNum rSquared;
    BigNum modulus;
    BigNum unk34;
    u32 inverse;
    u32 flags;
} Montgomery;

static void *CryptAlloc(u32 size);
static void CryptFree(void *ptr);
static int BitLength32(u32 x);
static int BitLength(const BigNum *n);
static void CryptDestroy(BigNum *n);
static void FreeNum(BigNum *n);
static BigNum *CryptResize(BigNum *n, int words);
static void CryptNormalize(BigNum *n);
static void InitNum(BigNum *n);
static BigNum *NewNum(void);
static BigNumPool *NewPool(void);
static void InitPool(BigNumPool *pool);
static void FreePool(BigNumPool *pool);
static BigNum *CryptExpand(BigNum *n, int words);
static BigNum *CryptCopy(BigNum *dest, const BigNum *src);
static void CryptClear(BigNum *n);
static BOOL CryptSetWord(BigNum *n, u32 word);
static BigNum *FromBytes(const u8 *bytes, int size, BigNum *out);
static int ToBytes(const BigNum *n, u8 *out);
static int CryptCompare(const BigNum *a, const BigNum *b);
static BOOL CryptSetBit(BigNum *n, int bit);
static BOOL CryptTestBit(const BigNum *n, int bit);
static BOOL ShiftLeft(BigNum *dest, const BigNum *src, int bits);
static BOOL ShiftRight(BigNum *dest, const BigNum *src, int bits);
static BOOL AddWord(BigNum *n, u32 word);
static BOOL SubWord(BigNum *n, u32 word);
static BOOL AddSigned(BigNum *dest, const BigNum *a, const BigNum *b);
static BOOL AddMagnitude(BigNum *dest, const BigNum *a, const BigNum *b);
static BOOL SubMagnitude(BigNum *dest, const BigNum *a, const BigNum *b);
static BOOL SubSigned(BigNum *dest, const BigNum *a, const BigNum *b);
static BOOL CryptMul(BigNum *dest, const BigNum *a, const BigNum *b, BigNumPool *pool);
static void MulWords(u32 *dest, const u32 *a, int aSize, const u32 *b, int bSize);
static BOOL BigDiv(BigNum *quotient, BigNum *remainder, const BigNum *numerator, const BigNum *denominator,
                BigNumPool *pool);
static BOOL BigMod(BigNum *remainder, const BigNum *numerator, const BigNum *denominator, BigNumPool *pool);
static BOOL ModExp(BigNum *dest, const BigNum *base, const BigNum *exponent, const BigNum *modulus, BigNumPool *pool);
static BOOL CryptSquare(BigNum *dest, const BigNum *a, BigNumPool *pool);
static void SquareWords(u32 *dest, const u32 *a, int size, u32 *scratch);
static void MontReduce(u32 *dest, u32 *product, const u32 *modulus, int size, u32 inverse);
static u32 InverseOfWord(u32 x);
static int EncodeExponent(const BigNum *exponent, const u8 **encoded, int unused, BigNumPool *pool);
static int EncodeWindows(u8 *dest, const BigNum *exponent, int window);
static inline void ReadWindow(const u8 **cursor, int *index, int *squarings);
static BOOL MontInit(Montgomery *mont, const BigNum *modulus, BigNumPool *pool);
static Montgomery *NewMont(void);
static void InitMont(Montgomery *mont);
static void FreeMont(Montgomery *mont);
static void InitReducer(Reducer *reducer);
static void FreeReducer(Reducer *reducer);
static BOOL SetReducerModulus(Reducer *reducer, const BigNum *modulus);
static BOOL ModMul(BigNum *dest, const BigNum *a, const BigNum *b, Reducer *reducer, BigNumPool *pool);
static BOOL CryptReduce(BigNum *quotient, BigNum *remainder, const BigNum *numerator, Reducer *reducer, BigNumPool *pool);
static int ComputeReciprocal(BigNum *reciprocal, const BigNum *modulus, int bits, BigNumPool *pool);
static BOOL ModExpPlain(BigNum *dest, const BigNum *base, const BigNum *exponent, const BigNum *modulus,
                        BigNumPool *pool);
static BOOL ModExpMont(BigNum *dest, const BigNum *base, const BigNum *exponent, const BigNum *modulus,
                       BigNumPool *pool, Montgomery *mont);
static asm u32 MulAddWords(u32 *dest, const u32 *a, int size, u32 word);
static asm u32 MulWord(u32 *dest, const u32 *a, int size, u32 word);
static asm void SquareDigits(u32 *dest, const u32 *a, int size);
static u32 DivWords(u32 high, u32 low, u32 divisor);
static asm u32 AddWords(u32 *dest, const u32 *a, const u32 *b, int size);
static asm u32 SubWords(u32 *dest, const u32 *a, const u32 *b, int size);
static BOOL ReadAsn1(const u8 **data, u32 *size, int tag, u32 *length);
static BOOL ParsePadding(const u8 *block, u32 size, const u8 **out, u32 *outSize);
static BOOL ParseDigestInfo(const u8 *data, u32 size, const u8 **oid, u32 *oidSize, const u8 **digest,
                            u32 *digestSize);
static int RsaPublic(u8 *out, int outSize, const u8 *input, int inputSize, const u8 *modulus, int modulusSize,
                     u32 exponent);
static int ExtractDigest(u8 *out, int outSize, const u8 *signature, int signatureSize, const u8 *modulus,
                         int modulusSize, u32 exponent);
static BOOL VerifyDigest(const u8 *digest, const u8 *signature, const u8 *modulus);

// The allocator that the game gave: its function to free, the one to allocate, and an argument that is not used
static struct {
    CryptFreeFunc free;
    CryptAllocFunc alloc;
    u32 arg;
} sAllocator;

// The number of trailing zero bits of each value of a window, for windows of up to 6 bits: a window of w bits reads the
// last 2^w of them
static const s8 sTrailingZeros[64] = {
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
};
static const s8 *sTrailingZerosByWindow[7] = {
    sTrailingZeros + 63, sTrailingZeros + 62, sTrailingZeros + 60, sTrailingZeros + 56,
    sTrailingZeros + 48, sTrailingZeros + 32, sTrailingZeros,
};

// The bit length of each value of a nibble
static const s8 sNibbleBits[16] = {0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};

// The encodings of the exponents 3, 17 and 65537, which the key uses: a length, the window and the table's size, and
// the pairs of a window's value and the zeros after it
static const u8 sEncodedExponent3[10] = {0, 8, 1, 1, 1, 1, 1, 0, 0, 0};
static const u8 sEncodedExponent17[10] = {0, 8, 1, 1, 1, 4, 1, 0, 0, 0};
static const u8 sEncodedExponent65537[10] = {0, 8, 1, 1, 1, 16, 1, 0, 0, 0};

// Raises the input to the power of the exponent modulo the modulus, all as bytes with the most significant first, and
// returns the length of the output, or a negative number: -1 when it does not fit, -2 when it failed and -3 for a
// missing argument
static int RsaPublic(u8 *out, int outSize, const u8 *input, int inputSize, const u8 *modulus, int modulusSize,
                     u32 exponent) {
    BigNum inputNum;
    BigNum resultNum;
    BigNum exponentNum;
    BigNum modulusNum;
    BigNumPool *pool;
    int result;

    if (out == NULL || input == NULL || modulus == NULL) {
        return -3;
    }
    pool = NewPool();
    InitNum(&inputNum);
    InitNum(&resultNum);
    InitNum(&exponentNum);
    InitNum(&modulusNum);
    if (pool == NULL) {
        result = -2;
    } else if (FromBytes(input, inputSize, &inputNum) == NULL) {
        result = -2;
    } else if (!CryptSetWord(&exponentNum, exponent)) {
        result = -2;
    } else if (FromBytes(modulus, modulusSize, &modulusNum) == NULL) {
        result = -2;
    } else if (!ModExp(&resultNum, &inputNum, &exponentNum, &modulusNum, pool)) {
        result = -2;
    } else if (outSize < (BitLength(&resultNum) + 7) / 8) {
        result = -1;
    } else {
        result = ToBytes(&resultNum, out);
    }
    CryptDestroy(&inputNum);
    CryptDestroy(&resultNum);
    CryptDestroy(&exponentNum);
    CryptDestroy(&modulusNum);
    if (pool != NULL) {
        FreePool(pool);
    }
    return result;
}

// Reads the tag and the length of a piece of ASN.1, leaving the data at its content and the size at what is left
static BOOL ReadAsn1(const u8 **data, u32 *size, int tag, u32 *length) {
    const u8 *p = *data;
    u32 left;
    u32 value = 0;
    u32 count;
    u32 b;

    if (*p++ != tag) {
        return FALSE;
    }
    left = *size;
    if (left < 1) {
        return FALSE;
    }
    b = *p;
    left--;
    if ((b & 0x80) != 0) {
        count = b & 0x7f;
        if (left < count + 1) {
            return FALSE;
        }
        if (length != NULL) {
            u32 digit = p[1] & 0x7f;

            p++;
            do {
                value = digit + (value << 7);
            } while ((--count & 0xff) != 0);
            left -= (b & 0x7f);
        } else {
            p += count;
            left -= count;
        }
    } else {
        p++;
        value = b;
        if (left < 1) {
            return FALSE;
        }
        left--;
    }
    *data = p;
    *size = left;
    if (length != NULL) {
        *length = value;
    }
    return TRUE;
}

// Takes the padding off a block that a signature decrypts to: a 1, at least 8 bytes of 255 and a 0
static BOOL ParsePadding(const u8 *block, u32 size, const u8 **out, u32 *outSize) {
    const u8 *p;
    const u8 *end;
    int count;

    if (size < 10) {
        return FALSE;
    }
    end = block + size;
    if (block[0] != 1) {
        return FALSE;
    }
    p = block + 1;
    count = 0;
    do {
        if (*p++ != 0xff) {
            return FALSE;
        }
        count++;
    } while (count < 8);
    while (p != end) {
        if (*p != 0xff) {
            break;
        }
        p++;
    }
    if (p == end) {
        return FALSE;
    }
    if (*p != 0) {
        return FALSE;
    }
    *outSize = end - (p + 1);
    *out = p + 1;
    return TRUE;
}

// Reads a digest out of its ASN.1: a sequence of the algorithm (an identifier and a null) and the digest
static BOOL ParseDigestInfo(const u8 *data, u32 size, const u8 **oid, u32 *oidSize, const u8 **digest,
                            u32 *digestSize) {
    u32 length;

    if (!ReadAsn1(&data, &size, 0x30, NULL)) {
        return FALSE;
    }
    if (!ReadAsn1(&data, &size, 0x30, NULL)) {
        return FALSE;
    }
    if (!ReadAsn1(&data, &size, 6, &length)) {
        return FALSE;
    }
    if (oid != NULL) {
        *oid = data;
    }
    if (oidSize != NULL) {
        *oidSize = length;
    }
    data += length;
    if (size < length) {
        return FALSE;
    }
    size -= length;
    if (!ReadAsn1(&data, &size, 5, &length)) {
        return FALSE;
    }
    data += length;
    if (size < length) {
        return FALSE;
    }
    size -= length;
    if (!ReadAsn1(&data, &size, 4, &length)) {
        return FALSE;
    }
    if (size != length) {
        return FALSE;
    }
    if (digest != NULL) {
        *digest = data;
    }
    if (digestSize != NULL) {
        *digestSize = length;
    }
    return TRUE;
}

// Decrypts a signature with the public key and takes the digest out of what it holds. The length of the digest, or -1
static int ExtractDigest(u8 *out, int outSize, const u8 *signature, int signatureSize, const u8 *modulus,
                         int modulusSize, u32 exponent) {
    u8 block[0x100];
    const u8 *unpadded = NULL;
    u32 unpaddedSize = 0;
    const u8 *digest;
    u32 digestSize = 0;
    int size;
    int i;

    size = RsaPublic(block, sizeof(block), signature, signatureSize, modulus, modulusSize, exponent);
    if (size < 0) {
        return -1;
    }
    if (!ParsePadding(block, size, &unpadded, &unpaddedSize)) {
        return -1;
    }
    if (!ParseDigestInfo(unpadded, unpaddedSize, NULL, NULL, &digest, &digestSize)) {
        return -1;
    }
    if (digestSize > outSize) {
        return -1;
    }
    for (i = 0; i < digestSize; i++) {
        out[i] = digest[i];
    }
    for (; i < outSize; i++) {
        out[i] = 0;
    }
    return digestSize;
}

// Whether the signature, of 128 bytes, holds the digest: the key's modulus is 128 bytes and its exponent is 65537
static BOOL VerifyDigest(const u8 *digest, const u8 *signature, const u8 *modulus) {
    u8 extracted[0x14];
    int i;

    if (ExtractDigest(extracted, 0x14, signature, 0x80, modulus, 0x80, 0x10001) != 0x14) {
        return FALSE;
    }
    for (i = 0; i < 0x14; i++) {
        if (extracted[i] != digest[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

int Crypt_VerifySignature(const void *data, u32 size, void *signature, const u8 *key) {
    u8 digest[0x14];

    func_02077720(digest, data, size);
    return VerifyDigest(digest, signature, key);
}

static void *CryptAlloc(u32 size) {
    if (sAllocator.alloc != NULL) {
        return sAllocator.alloc(size);
    }
    return malloc_device(0, -1, size);
}

static void CryptFree(void *ptr) {
    if (sAllocator.free != NULL) {
        sAllocator.free(ptr);
        return;
    }
    free_device(0, -1, ptr);
}

void Crypt_SetAllocator(CryptAllocFunc alloc, CryptFreeFunc free, u32 arg) {
    sAllocator.alloc = alloc;
    sAllocator.free = free;
    sAllocator.arg = arg;
}

// The length in bits of a word
static int BitLength32(u32 x) {
    int shift;
    u32 rest;

    if ((x & 0xffff0000) != 0) {
        shift = (x & 0xff000000) != 0 ? 24 : 16;
    } else {
        shift = (x & 0xff00) != 0 ? 8 : 0;
    }
    rest = x >> shift;
    if ((rest & 0xf0) == 0) {
        return sNibbleBits[rest] + shift;
    }
    return sNibbleBits[rest >> 4] + shift + 4;
}

static int BitLength(const BigNum *n) {
    int top;

    if (n->length == 0) {
        return 0;
    }
    top = n->length - 1;
    return BitLength32(n->digits[top]) + top * 32;
}

static void FreeNum(BigNum *n) {
    if (n == NULL) {
        return;
    }
    CryptClear(n);
    CryptDestroy(n);
}

static void CryptDestroy(BigNum *n) {
    if (n == NULL) {
        return;
    }
    if (n->digits != NULL && (n->flags & BIGNUM_STATIC) == 0) {
        CryptFree(n->digits);
    }
    n->flags |= BIGNUM_FREED;
    if ((n->flags & BIGNUM_HEAP) != 0) {
        CryptFree(n);
    }
}

// Makes room for words, with zeros above the length
static BigNum *CryptResize(BigNum *n, int words) {
    int i;

    if (n->length < words) {
        if (words > n->capacity) {
            n = CryptExpand(n, words);
        }
        if (n == NULL) {
            return NULL;
        }
        if (n->digits != NULL) {
            for (i = n->length; i < words; i++) {
                n->digits[i] = 0;
            }
        }
    }
    return n;
}

// Drops the zero words on top
static void CryptNormalize(BigNum *n) {
    u32 *top;

    if (n->length > 0) {
        top = n->digits + (n->length - 1);
        while (n->length > 0) {
            if (*top-- != 0) {
                return;
            }
            n->length--;
        }
    }
}

static void InitNum(BigNum *n) {
    sys_memset(n, 0, sizeof(BigNum));
}

static BigNum *NewNum(void) {
    BigNum *n = CryptAlloc(sizeof(BigNum));

    if (n == NULL) {
        return NULL;
    }
    n->flags = BIGNUM_HEAP;
    n->length = 0;
    n->negative = 0;
    n->capacity = 0;
    n->digits = NULL;
    return n;
}

static BigNumPool *NewPool(void) {
    BigNumPool *pool = CryptAlloc(sizeof(BigNumPool));

    if (pool == NULL) {
        return NULL;
    }
    InitPool(pool);
    pool->flags = POOL_HEAP;
    return pool;
}

static void InitPool(BigNumPool *pool) {
    sys_memset(pool, 0, sizeof(BigNumPool));
    pool->used = 0;
    pool->flags = 0;
}

static void FreePool(BigNumPool *pool) {
    int i;

    for (i = 0; i < POOL_SIZE; i++) {
        FreeNum(&pool->numbers[i]);
    }
    if ((pool->flags & POOL_HEAP) != 0) {
        CryptFree(pool);
    }
}

// Makes room for words, keeping the words it has
static BigNum *CryptExpand(BigNum *n, int words) {
    u32 *digits;

    if (words > n->capacity) {
        if ((n->flags & BIGNUM_STATIC) != 0) {
            return NULL;
        }
        digits = CryptAlloc((words + 1) * sizeof(u32));
        if (digits == NULL) {
            return NULL;
        }
        if (n->digits != NULL) {
            sys_memcpy(n->digits, digits, n->length * sizeof(u32));
            CryptFree(n->digits);
        }
        n->digits = digits;
        n->capacity = words;
    }
    return n;
}

static BigNum *CryptCopy(BigNum *dest, const BigNum *src) {
    if (dest == src) {
        return dest;
    }
    if (src->length > dest->capacity) {
        dest = CryptExpand(dest, src->length);
    }
    if (dest == NULL) {
        return NULL;
    }
    sys_memcpy(src->digits, dest->digits, src->length * sizeof(u32));
    dest->length = src->length;
    if (dest->length == 0 && dest->digits != NULL) {
        dest->digits[0] = 0;
    }
    dest->negative = src->negative;
    return dest;
}

static void CryptClear(BigNum *n) {
    if (n->digits != NULL) {
        sys_memset(n->digits, 0, n->capacity * sizeof(u32));
    }
    n->length = 0;
    n->negative = 0;
}

static BOOL CryptSetWord(BigNum *n, u32 word) {
    if (n->capacity < 1) {
        n = CryptExpand(n, 2);
    }
    if (n == NULL) {
        return FALSE;
    }
    n->negative = 0;
    n->length = 0;
    n->digits[0] = word;
    if (n->digits[0] != 0) {
        n->length = 1;
    }
    return TRUE;
}

// Reads size bytes, the most significant first, into out or into a new number
static BigNum *FromBytes(const u8 *bytes, int size, BigNum *out) {
    u32 word = 0;
    int words;
    int phase;
    int count;

    if (out == NULL) {
        out = NewNum();
    }
    if (out == NULL) {
        return NULL;
    }
    if (size == 0) {
        out->length = 0;
        return out;
    }
    if (out->capacity < ((size + 2) * 8 + 31) / 32) {
        if (CryptExpand(out, (size + 2) * 8 / 32 + 1) == NULL) {
            return NULL;
        }
    }
    words = (u32)(size - 1) / 4 + 1;
    out->length = words;
    phase = (size - 1) & 3;
    count = size - 1;
    do {
        word = *bytes++ | (word << 8);
        if (phase-- == 0) {
            words--;
            out->digits[words] = word;
            word = 0;
            phase = 3;
        }
    } while (count-- != 0);
    CryptNormalize(out);
    return out;
}

// Writes the number as bytes, the most significant first, and returns how many
static int ToBytes(const BigNum *n, u8 *out) {
    int bytes = (BitLength(n) + 7) / 8;
    int i;

    for (i = bytes - 1; i >= 0; i--) {
        *out++ = n->digits[i / 4] >> (i % 4 * 8);
    }
    return bytes;
}

// Compares the sizes, which is the difference of their lengths, or the most significant word that differs
static int CryptCompare(const BigNum *a, const BigNum *b) {
    int diff = a->length - b->length;
    int i;

    if (diff != 0) {
        return diff;
    }
    for (i = a->length - 1; i >= 0; i--) {
        if (a->digits[i] != b->digits[i]) {
            return a->digits[i] > b->digits[i] ? 1 : -1;
        }
    }
    return 0;
}

static BOOL CryptSetBit(BigNum *n, int bit) {
    int word = bit / 32;
    int shift = bit % 32;
    int i;

    if (n->length <= word) {
        if (word + 1 > n->capacity) {
            if (CryptExpand(n, word + 1) == NULL) {
                return FALSE;
            }
        }
        for (i = n->length; i < word + 1; i++) {
            n->digits[i] = 0;
        }
        n->length = word + 1;
    }
    n->digits[word] |= 1 << shift;
    return TRUE;
}

static BOOL CryptTestBit(const BigNum *n, int bit) {
    int word;

    if (bit < 0) {
        return FALSE;
    }
    word = bit / 32;
    if (n->length <= word) {
        return FALSE;
    }
    return (n->digits[word] & (1 << (bit % 32))) != 0;
}

static BOOL ShiftLeft(BigNum *dest, const BigNum *src, int bits) {
    int words = bits / 32;
    int shift = bits % 32;
    int i;
    u32 *out;

    if (src->length + words + 1 > dest->capacity) {
        if (CryptExpand(dest, src->length + words + 1) == NULL) {
            return FALSE;
        }
    }
    dest->negative = src->negative;
    dest->digits[src->length + words] = 0;
    out = dest->digits + words;
    if (shift == 0) {
        for (i = src->length - 1; i >= 0; i--) {
            out[i] = src->digits[i];
        }
    } else {
        for (i = src->length - 1; i >= 0; i--) {
            u32 word = src->digits[i];

            out[i + 1] |= word >> (32 - shift);
            out[i] = word << shift;
        }
    }
    sys_memset(dest->digits, 0, words * sizeof(u32));
    dest->length = src->length + words + 1;
    CryptNormalize(dest);
    return TRUE;
}

static BOOL ShiftRight(BigNum *dest, const BigNum *src, int bits) {
    int words = bits / 32;
    int shift = bits % 32;
    int count;
    int i;
    const u32 *in;
    u32 *out;
    u32 word;

    if (src->length < words) {
        CryptSetWord(dest, 0);
        return TRUE;
    }
    if (dest != src) {
        if (src->length - words + 2 > dest->capacity) {
            if (CryptExpand(dest, src->length - words + 2) == NULL) {
                return FALSE;
            }
        }
        dest->negative = src->negative;
    }
    in = src->digits + words;
    count = src->length - words;
    dest->length = count;
    out = dest->digits;
    if (shift == 0) {
        for (i = 0; i < count + 1; i++) {
            *out++ = *in++;
        }
    } else {
        word = *in++;
        for (i = 1; i < count; i++) {
            u32 next = *in++;

            *out++ = (word >> shift) | (next << (32 - shift));
            word = next;
        }
        *out++ = word >> shift;
        *out = 0;
    }
    CryptNormalize(dest);
    return TRUE;
}

// Adds a word to the number, whatever its sign
static BOOL AddWord(BigNum *n, u32 word) {
    int i;

    if (word == 0) {
        return TRUE;
    }
    if (n->negative) {
        if (n->length > 1) {
            n->negative = 0;
            SubWord(n, word);
            n->negative = 1;
            return TRUE;
        }
        if (n->digits[0] > word) {
            n->digits[0] -= word;
        } else {
            n->negative = 0;
            if (n->digits[0] < word) {
                n->digits[0] = word - n->digits[0];
            } else {
                n->length = 0;
            }
        }
        return TRUE;
    }
    if (n->length + 1 > n->capacity) {
        if (CryptExpand(n, n->length + 1) == NULL) {
            return FALSE;
        }
    }
    n->digits[n->length] = 0;
    for (i = 0;;) {
        u32 sum = word + n->digits[i];

        n->digits[i] = sum;
        if (word > sum) {
            word = 1;
            i++;
        } else {
            break;
        }
    }
    if (i >= n->length) {
        n->length++;
    }
    return TRUE;
}

static BOOL SubWord(BigNum *n, u32 word) {
    int i;

    if (word == 0) {
        return TRUE;
    }
    if (n->negative) {
        n->negative = 0;
        AddWord(n, word);
        n->negative = 1;
        return TRUE;
    }
    if (n->length <= 1) {
        if (n->length == 0) {
            if (n->capacity < 1) {
                if (CryptExpand(n, 1) == NULL) {
                    return FALSE;
                }
            }
            n->digits[0] = word;
            n->negative = 1;
            n->length = 1;
        } else if (n->digits[0] == word) {
            n->length = 0;
        } else if (n->digits[0] > word) {
            n->digits[0] -= word;
        } else {
            n->negative = 1;
            n->digits[0] = word - n->digits[0];
        }
        return TRUE;
    }
    for (i = 0;;) {
        u32 digit = n->digits[i];

        if (digit >= word) {
            n->digits[i] = digit - word;
            break;
        }
        n->digits[i] = digit - word;
        word = 1;
        i++;
    }
    if (n->digits[i] == 0 && i == n->length - 1) {
        n->length--;
    }
    return TRUE;
}

static BOOL AddSigned(BigNum *dest, const BigNum *a, const BigNum *b) {
    const BigNum *swap;

    if (a->negative == b->negative) {
        dest->negative = a->negative != 0;
        return AddMagnitude(dest, a, b) != FALSE;
    }
    if (a->negative != 0) {
        swap = a;
        a = b;
        b = swap;
    }
    if (CryptCompare(a, b) < 0) {
        if (!SubMagnitude(dest, b, a)) {
            return FALSE;
        }
        dest->negative = 1;
    } else {
        if (!SubMagnitude(dest, a, b)) {
            return FALSE;
        }
        dest->negative = 0;
    }
    return TRUE;
}

// Adds the sizes of two numbers
static BOOL AddMagnitude(BigNum *dest, const BigNum *a, const BigNum *b) {
    const BigNum *swap;
    int aSize;
    int bSize;
    int i;
    u32 carry;
    const u32 *aDigits;
    u32 *destDigits;

    if (a->length < b->length) {
        swap = a;
        a = b;
        b = swap;
    }
    aSize = a->length;
    if (aSize + 1 > dest->capacity) {
        if (CryptExpand(dest, aSize + 1) == NULL) {
            return FALSE;
        }
    }
    bSize = b->length;
    dest->length = aSize;
    aDigits = a->digits;
    destDigits = dest->digits;
    carry = AddWords(destDigits, aDigits, b->digits, bSize);
    destDigits += bSize;
    aDigits += bSize;
    i = bSize;
    if (carry != 0) {
        while (i < aSize) {
            u32 digit = *aDigits++;

            *destDigits++ = digit + 1;
            i++;
            if (*(destDigits - 1) >= digit) {
                carry = 0;
                break;
            }
        }
    }
    if (i >= aSize && carry != 0) {
        *destDigits++ = 1;
        dest->length++;
    }
    if (destDigits != aDigits && i < aSize) {
        do {
            *destDigits++ = *aDigits++;
            i++;
        } while (i < aSize);
    }
    return TRUE;
}

// Subtracts a smaller size from a bigger one
static BOOL SubMagnitude(BigNum *dest, const BigNum *a, const BigNum *b) {
    int aSize = a->length;
    int bSize = b->length;
    int i = 0;
    int borrow = 0;
    const u32 *aDigits;
    const u32 *bDigits;
    u32 *destDigits;

    if (aSize < bSize) {
        return FALSE;
    }
    if (aSize > dest->capacity) {
        if (CryptExpand(dest, aSize) == NULL) {
            return FALSE;
        }
    }
    aDigits = a->digits;
    bDigits = b->digits;
    destDigits = dest->digits;
    for (i = 0; i < bSize; i++) {
        u32 x = *aDigits++;
        u32 y = *bDigits++;

        if (borrow) {
            borrow = x <= y;
            x = x - y - 1;
        } else {
            borrow = x < y;
            x = x - y;
        }
        *destDigits++ = x;
    }
    while (borrow && i < aSize) {
        u32 x = *aDigits++;

        i++;
        *destDigits++ = x - 1;
        borrow = x == 0;
    }
    if (destDigits != aDigits) {
        while (i < aSize) {
            *destDigits++ = *aDigits++;
            i++;
        }
    }
    dest->length = aSize;
    CryptNormalize(dest);
    return TRUE;
}

static BOOL SubSigned(BigNum *dest, const BigNum *a, const BigNum *b) {
    const BigNum *swap;
    BOOL add = FALSE;
    int negative = 0;
    int size;

    if (a->negative != 0) {
        if (b->negative != 0) {
            swap = a;
            a = b;
            b = swap;
        } else {
            add = TRUE;
            negative = 1;
        }
    } else if (b->negative != 0) {
        add = TRUE;
    }
    if (add) {
        if (!AddMagnitude(dest, a, b)) {
            return FALSE;
        }
        dest->negative = negative;
        return TRUE;
    }
    size = a->length > b->length ? a->length : b->length;
    if (size > dest->capacity) {
        if (CryptExpand(dest, size) == NULL) {
            return FALSE;
        }
    }
    if (CryptCompare(a, b) < 0) {
        if (!SubMagnitude(dest, b, a)) {
            return FALSE;
        }
        dest->negative = 1;
    } else {
        if (!SubMagnitude(dest, a, b)) {
            return FALSE;
        }
        dest->negative = 0;
    }
    return TRUE;
}

static BOOL CryptMul(BigNum *dest, const BigNum *a, const BigNum *b, BigNumPool *pool) {
    int total;
    int aSize = a->length;
    int bSize = b->length;
    int negative;
    BigNum *work;

    if (aSize == 0 || bSize == 0) {
        CryptSetWord(dest, 0);
        return TRUE;
    }
    total = aSize + bSize;
    negative = a->negative ^ b->negative;
    if (dest == a || dest == b) {
        work = &pool->numbers[pool->used + 1];
    } else {
        work = dest;
    }
    if (total > work->capacity) {
        if (CryptExpand(work, total) == NULL) {
            return FALSE;
        }
    }
    work->length = total;
    MulWords(work->digits, a->digits, aSize, b->digits, bSize);
    dest->negative = negative;
    CryptNormalize(work);
    if (dest != work) {
        CryptCopy(dest, work);
    }
    return TRUE;
}

// Multiplies the words of two numbers the way it is taught: one row of the shorter number for each of the longer
static void MulWords(u32 *dest, const u32 *a, int aSize, const u32 *b, int bSize) {
    int i;

    if (aSize < bSize) {
        const u32 *swapWords = a;
        int swapSize = aSize;

        a = b;
        b = swapWords;
        aSize = bSize;
        bSize = swapSize;
    }
    dest[aSize] = MulWord(dest, a, aSize, b[0]);
    for (i = 1; i < bSize; i++) {
        dest[aSize + i] = MulAddWords(dest + i, a, aSize, b[i]);
    }
}

// Divides, schoolbook: the denominator is shifted up until its top bit is set, then each word of the quotient is
// estimated from the top words of what is left, and the estimate is corrected
static BOOL BigDiv(BigNum *quotient, BigNum *remainder, const BigNum *numerator, const BigNum *denominator,
                BigNumPool *pool) {
    BigNum *product;
    BigNum *shiftedNumerator;
    BigNum *shiftedDenominator;
    BigNum window;
    int used;
    int shift;
    int denominatorSize;
    int quotientSize;
    int numeratorSize;
    u32 top;
    u32 next = 0;
    u32 *numeratorTop;
    u32 *quotientDigit;
    int i;

    if (denominator->length == 0 || (denominator->length == 1 && denominator->digits[0] == 0)) {
        return FALSE;
    }
    if (CryptCompare(numerator, denominator) < 0) {
        if (remainder != NULL) {
            if (CryptCopy(remainder, numerator) == NULL) {
                return FALSE;
            }
        }
        if (quotient != NULL) {
            CryptSetWord(quotient, 0);
        }
        return TRUE;
    }
    used = pool->used;
    product = &pool->numbers[used];
    product->negative = 0;
    shiftedNumerator = &pool->numbers[used + 1];
    shiftedDenominator = &pool->numbers[used + 2];
    if (quotient == NULL) {
        quotient = &pool->numbers[used + 3];
    }
    shift = 32 - BitLength(denominator) % 32;
    if (!ShiftLeft(shiftedDenominator, denominator, shift)) {
        return FALSE;
    }
    if (!ShiftLeft(shiftedNumerator, numerator, shift + 32)) {
        return FALSE;
    }
    shiftedDenominator->negative = 0;
    shiftedNumerator->negative = 0;
    denominatorSize = shiftedDenominator->length;
    numeratorSize = shiftedNumerator->length;
    quotientSize = numeratorSize - denominatorSize;
    InitNum(&window);
    window.digits = shiftedNumerator->digits + quotientSize;
    window.length = denominatorSize;
    window.capacity = shiftedNumerator->capacity + 1;
    top = shiftedDenominator->digits[denominatorSize - 1];
    if (denominatorSize != 1) {
        next = shiftedDenominator->digits[denominatorSize - 2];
    }
    numeratorTop = shiftedNumerator->digits + (numeratorSize - 1);
    if (quotientSize + 1 > quotient->capacity) {
        if (CryptExpand(quotient, quotientSize + 1) == NULL) {
            return FALSE;
        }
    }
    quotient->negative = numerator->negative ^ denominator->negative;
    quotient->length = quotientSize;
    quotientDigit = quotient->digits + (quotientSize - 1);
    if (denominatorSize + 1 > product->capacity) {
        if (CryptExpand(product, denominatorSize + 1) == NULL) {
            return FALSE;
        }
    }
    if (CryptCompare(&window, shiftedDenominator) >= 0) {
        if (!SubMagnitude(&window, &window, shiftedDenominator)) {
            return FALSE;
        }
        *quotientDigit = 1;
    } else {
        quotient->length--;
    }
    quotientDigit--;
    for (i = 0; i < quotientSize - 1; i++) {
        u32 high;
        u32 low;
        u32 estimate;
        u32 carry;
        int size;
        int oldLength;

        window.digits--;
        window.length++;
        high = numeratorTop[0];
        low = numeratorTop[-1];
        if (high == top) {
            estimate = 0xffffffff;
        } else {
            estimate = DivWords(high, low, top);
        }
        for (;;) {
            u64 byNext = (u64)estimate * next;
            u64 byTop = (u64)estimate * top;
            u32 remainderLow = low - (u32)byTop;
            u32 remainderHigh = high - (u32)(byTop >> 32) - (remainderLow > low);

            if (remainderHigh != 0) {
                break;
            }
            if ((u32)(byNext >> 32) < remainderLow) {
                break;
            }
            if ((u32)(byNext >> 32) == remainderLow && (u32)byNext <= numeratorTop[-2]) {
                break;
            }
            estimate--;
        }
        carry = MulWord(product->digits, shiftedDenominator->digits, denominatorSize, estimate);
        product->digits[denominatorSize] = carry;
        size = denominatorSize + 1;
        while (size > 0 && product->digits[size - 1] == 0) {
            size--;
        }
        product->length = size;
        oldLength = window.length;
        SubSigned(&window, &window, product);
        shiftedNumerator->length = shiftedNumerator->length + window.length - oldLength;
        if (window.negative) {
            estimate--;
            oldLength = window.length;
            AddSigned(&window, &window, shiftedDenominator);
            shiftedNumerator->length = shiftedNumerator->length + window.length - oldLength;
        }
        *quotientDigit-- = estimate;
        numeratorTop--;
    }
    CryptNormalize(shiftedNumerator);
    if (remainder != NULL) {
        int negative = numerator->negative;

        if (!ShiftRight(remainder, shiftedNumerator, shift + 32)) {
            return FALSE;
        }
        remainder->negative = negative;
    }
    return TRUE;
}

static BOOL BigMod(BigNum *remainder, const BigNum *numerator, const BigNum *denominator, BigNumPool *pool) {
    return BigDiv(NULL, remainder, numerator, denominator, pool);
}

// Raises to a power modulo, by Montgomery's method when the modulus is odd
static BOOL ModExp(BigNum *dest, const BigNum *base, const BigNum *exponent, const BigNum *modulus, BigNumPool *pool) {
    if (modulus->length > 0 && (modulus->digits[0] & 1) != 0) {
        return ModExpMont(dest, base, exponent, modulus, pool, NULL);
    }
    return ModExpPlain(dest, base, exponent, modulus, pool);
}

static BOOL CryptSquare(BigNum *dest, const BigNum *a, BigNumPool *pool) {
    int size = a->length;
    int used = pool->used;
    BigNum *scratch = &pool->numbers[used];
    BigNum *work;
    u32 small4[8];
    u32 small8[16];

    if (a != dest) {
        work = dest;
    } else {
        work = &pool->numbers[used + 1];
    }
    if (size <= 0) {
        dest->length = 0;
        return TRUE;
    }
    if (size * 2 > work->capacity) {
        if (CryptExpand(work, size * 2) == NULL) {
            return FALSE;
        }
    }
    work->length = size * 2;
    work->negative = 0;
    if (size == 4) {
        SquareWords(work->digits, a->digits, 4, small4);
    } else if (size == 8) {
        SquareWords(work->digits, a->digits, 8, small8);
    } else {
        if (size * 2 > scratch->capacity) {
            if (CryptExpand(scratch, size * 2) == NULL) {
                return FALSE;
            }
        }
        SquareWords(work->digits, a->digits, size, scratch->digits);
    }
    if (size * 2 > 0 && work->digits[size * 2 - 1] == 0) {
        work->length--;
    }
    if (work != dest) {
        CryptCopy(dest, work);
    }
    return TRUE;
}

// Squares: the products of the pairs of different words are added up once, doubled, and the squares of the words
// are added
static void SquareWords(u32 *dest, const u32 *a, int size, u32 *scratch) {
    int remaining = size - 1;
    const u32 *in = a;
    u32 *out;
    int i;

    dest[size * 2 - 1] = 0;
    dest[0] = 0;
    out = dest + 1;
    if (remaining > 0) {
        in++;
        out[remaining] = MulWord(out, in, remaining, a[0]);
        out += 2;
    }
    for (i = size - 2; i > 0; i--) {
        const u32 *current = in;

        remaining--;
        in++;
        out[remaining] = MulAddWords(out, in, remaining, *current);
        out += 2;
    }
    AddWords(dest, dest, dest, size * 2);
    SquareDigits(scratch, a, size);
    AddWords(dest, dest, scratch, size * 2);
}

// Reduces a product of 2 * size words by the modulus the Montgomery way: size words of multiples of the modulus are
// added so that the low half is zero, and the high half, less the modulus when it is not smaller, is the result
static void MontReduce(u32 *dest, u32 *product, const u32 *modulus, int size, u32 inverse) {
    u32 carry = 0;
    int i;
    u32 *high = product + size;
    u32 *low = product;

    for (i = 0; i < size; i++) {
        u32 added = MulAddWords(low, modulus, size, low[0] * inverse);
        u32 sum = added + carry;

        carry = sum < carry;
        *high += sum;
        if (*high < sum) {
            carry++;
        }
        low++;
        high++;
    }
    low = product + size;
    if (carry == 0) {
        int j = size - 1;

        if (low[j] == modulus[j]) {
            while (j > 0 && low[j] == modulus[j]) {
                j--;
            }
        }
        carry = low[j] >= modulus[j];
    }
    if (carry != 0) {
        SubWords(dest, low, modulus, size);
    } else {
        for (i = 0; i < size; i++) {
            dest[i] = low[i];
        }
    }
}

// The inverse of 2^32 modulo a word, by Euclid's algorithm, or 0 when they have a common factor
static u32 InverseOfWord(u32 x) {
    u32 a = x;
    u32 b = -x % x;
    u32 prev = 1;
    u32 prevPrev = 0;
    int sign = -1;

    while (b != 0) {
        u32 remainder = a % b;
        u32 next = a / b * prev + prevPrev;

        prevPrev = prev;
        a = b;
        prev = next;
        b = remainder;
        sign = -sign;
    }
    if (sign < 0) {
        prevPrev = x - prevPrev;
    }
    if (a != 1) {
        return 0;
    }
    return prevPrev % x;
}

// Writes the exponent as the windows that a power is computed with, after a header: the length, the window's bits and
// the size of the table of odd powers. The exponents 3, 17 and 65537, which keys use, are kept ready
static int EncodeExponent(const BigNum *exponent, const u8 **encoded, int unused, BigNumPool *pool) {
    BigNum *work = &pool->numbers[pool->used];
    const u8 *table = NULL;
    int window;
    int entries;
    int perWord;
    int size;
    int bits = exponent->length * 32;

    if (exponent->length == 0) {
        return 0;
    }
    if (exponent->length == 1) {
        if (exponent->digits[0] == 0x10001) {
            table = sEncodedExponent65537;
        } else if (exponent->digits[0] == 0x11) {
            table = sEncodedExponent17;
        } else if (exponent->digits[0] == 3) {
            table = sEncodedExponent3;
        }
        window = 1;
        entries = 1;
        perWord = 32;
    } else if (bits >= 256) {
        window = 5;
        entries = 16;
        perWord = 7;
    } else if (bits >= 128) {
        window = 4;
        entries = 8;
        perWord = 8;
    } else {
        window = 3;
        entries = 4;
        perWord = 11;
    }
    size = (exponent->length * perWord * 2 + 7) / 4;
    if (table == NULL) {
        u8 *bytes;

        if (size > work->capacity) {
            if (CryptExpand(work, size) == NULL) {
                return 0;
            }
        }
        bytes = (u8 *)work->digits;
        size = EncodeWindows(bytes + 4, exponent, window);
        bytes[0] = (size + 2) >> 8;
        bytes[1] = size + 2;
        bytes[2] = window;
        bytes[3] = entries;
        table = bytes;
    } else {
        size = 8;
    }
    *encoded = table;
    return size + 2;
}

// Cuts the exponent into windows from the top: a pair of bytes for each, the window's value (odd) and the number of
// zero bits after it, and a pair of zeros at the end. Runs of 256 zeros or more are pairs of 0 and 255 first
static int EncodeWindows(u8 *dest, const BigNum *exponent, int window) {
    int mask;
    int total;
    int size = 2;
    int zeros = 0;
    int bitPos = 0;
    int wordsLeft;
    const s8 *zerosTable;
    u8 *out;
    u8 *in;
    u8 *copy;
    const u32 *next;
    u32 low;
    u32 high;
    u32 current;
    u32 index;

    if (window > 6) {
        window = 6;
    }
    mask = (1 << window) - 1;
    zerosTable = sTrailingZerosByWindow[window];
    total = (window + exponent->length * 32 - 1) / window * 2 + 2;
    dest[total] = 0;
    out = dest + total;
    out[-1] = 0;
    out -= 2;
    wordsLeft = exponent->length;
    low = exponent->digits[0];
    if (wordsLeft > 1) {
        high = exponent->digits[1];
        next = exponent->digits + 2;
    } else {
        high = 0;
        next = exponent->digits + 1;
    }
    current = low;
    for (;;) {
        int skip;

        index = current & mask;
        skip = zerosTable[index];
        if (skip != 0) {
            bitPos += skip;
            zeros += skip;
            if (bitPos < 32) {
                goto refill;
            }
            if (wordsLeft > 1) {
                wordsLeft--;
                low = high;
                high = 0;
                if (wordsLeft > 1) {
                    high = *next++;
                }
                bitPos -= 32;
                goto refill;
            }
        }
        if (index == 0) {
            break;
        }
        out[0] = zeros;
        out[-1] = index;
        out -= 2;
        while (zeros >= 256) {
            zeros -= 256;
            out[0] = 0xff;
            out[-1] = 0;
            out -= 2;
        }
        bitPos += window;
        zeros = window;
        if (bitPos >= 32) {
            if (wordsLeft <= 1) {
                break;
            }
            wordsLeft--;
            low = high;
            high = 0;
            if (wordsLeft > 1) {
                high = *next++;
            }
            bitPos -= 32;
        }
    refill:
        if (bitPos == 0) {
            current = low;
        } else {
            current = (high << (32 - bitPos)) | (low >> bitPos);
        }
    }
    in = out + 1;
    copy = dest;
    while (in[0] != 0 || in[1] != 0) {
        copy[0] = in[0];
        copy[1] = in[1];
        in += 2;
        size += 2;
        copy += 2;
    }
    copy[0] = 0;
    copy[1] = 0;
    return size;
}

// Sets up the Montgomery form for an odd modulus: its copy, R squared modulo it, and the word that the reduction
// multiplies by
static BOOL MontInit(Montgomery *mont, const BigNum *modulus, BigNumPool *pool) {
    BOOL ok = FALSE;
    BigNum work;
    u32 first;
    u32 high;
    u32 low;

    if (modulus->length == 0) {
        return FALSE;
    }
    if (CryptCopy(&mont->modulus, modulus) == NULL) {
        return FALSE;
    }
    InitNum(&work);
    mont->unk0 = 1;
    mont->words = (BitLength(modulus) + 31) / 32;
    if (!CryptSetWord(&mont->rSquared, 0)) {
        return FALSE;
    }
    if (CryptSetBit(&mont->rSquared, 32)) {
        first = modulus->digits[0];
        if (CryptSetWord(&work, InverseOfWord(first)) && ShiftLeft(&work, &work, 32)) {
            if (work.length == 0 || (work.length == 1 && work.digits[0] == 0)) {
                if (!CryptSetWord(&work, -1)) {
                    goto done;
                }
            } else {
                SubWord(&work, 1);
            }
            high = work.length >= 2 ? work.digits[1] : 0;
            low = work.length >= 1 ? work.digits[0] : 0;
            mont->inverse = DivWords(high, low, first);
            CryptSetWord(&mont->rSquared, 0);
            if (CryptSetBit(&mont->rSquared, mont->words * 64)
                && BigMod(&mont->rSquared, &mont->rSquared, &mont->modulus, pool)
                && CryptResize(&mont->rSquared, mont->words) != NULL) {
                ok = TRUE;
            }
        }
    }
done:
    CryptDestroy(&work);
    return ok;
}

static Montgomery *NewMont(void) {
    Montgomery *mont = CryptAlloc(sizeof(Montgomery));

    if (mont == NULL) {
        return NULL;
    }
    InitMont(mont);
    mont->flags = 1;
    return mont;
}

static void InitMont(Montgomery *mont) {
    mont->unk0 = 0;
    mont->words = 0;
    InitNum(&mont->rSquared);
    InitNum(&mont->modulus);
    InitNum(&mont->unk34);
    mont->flags = 0;
}

static void FreeMont(Montgomery *mont) {
    CryptDestroy(&mont->rSquared);
    CryptDestroy(&mont->modulus);
    CryptDestroy(&mont->unk34);
    if ((mont->flags & 1) != 0) {
        CryptFree(mont);
    }
}

static void InitReducer(Reducer *reducer) {
    InitNum(&reducer->modulus);
    InitNum(&reducer->reciprocal);
    reducer->modulusBits = 0;
    reducer->flags = 0;
}

static void FreeReducer(Reducer *reducer) {
    CryptDestroy(&reducer->modulus);
    CryptDestroy(&reducer->reciprocal);
    if ((reducer->flags & 1) != 0) {
        CryptFree(reducer);
    }
}

static BOOL SetReducerModulus(Reducer *reducer, const BigNum *modulus) {
    CryptCopy(&reducer->modulus, modulus);
    CryptSetWord(&reducer->reciprocal, 0);
    reducer->modulusBits = BitLength(modulus);
    reducer->reciprocalBits = 0;
    return TRUE;
}

// Multiplies two numbers and reduces the product
static BOOL ModMul(BigNum *dest, const BigNum *a, const BigNum *b, Reducer *reducer, BigNumPool *pool) {
    BigNum *product = &pool->numbers[pool->used];
    const BigNum *input;
    BOOL ok = FALSE;

    pool->used++;
    if (b == NULL) {
        input = a;
    } else {
        input = product;
        if (a == b) {
            if (!CryptSquare(product, a, pool)) {
                goto done;
            }
        } else if (!CryptMul(product, a, b, pool)) {
            goto done;
        }
    }
    CryptReduce(NULL, dest, input, reducer, pool);
    ok = TRUE;
done:
    pool->used--;
    return ok;
}

// Divides with the reciprocal of the modulus: the quotient is taken from the top half of the product with the
// reciprocal, and off by at most 3
static BOOL CryptReduce(BigNum *quotient, BigNum *remainder, const BigNum *numerator, Reducer *reducer, BigNumPool *pool) {
    int saved = pool->used;
    BigNum *first = &pool->numbers[saved];
    BigNum *second = &pool->numbers[saved + 1];
    BOOL ok = FALSE;
    int bits;
    int shift;
    int tries;

    pool->used = saved + 2;
    if (quotient == NULL) {
        quotient = &pool->numbers[pool->used];
        pool->used++;
    }
    if (remainder == NULL) {
        remainder = &pool->numbers[pool->used];
        pool->used++;
    }
    if (CryptCompare(numerator, &reducer->modulus) < 0) {
        CryptSetWord(quotient, 0);
        CryptCopy(remainder, numerator);
        pool->used = saved;
        return TRUE;
    }
    bits = BitLength(numerator);
    if (bits < reducer->modulusBits * 2) {
        bits = reducer->modulusBits * 2;
        shift = 0;
    } else {
        shift = (bits - reducer->modulusBits * 2) / 2;
    }
    if (bits != reducer->reciprocalBits) {
        reducer->reciprocalBits = ComputeReciprocal(&reducer->reciprocal, &reducer->modulus, bits, pool);
    }
    if (ShiftRight(first, numerator, bits / 2 - shift) && CryptMul(second, first, &reducer->reciprocal, pool)
        && ShiftRight(quotient, second, shift + bits / 2)) {
        quotient->negative = 0;
        if (CryptMul(second, quotient, &reducer->modulus, pool) && SubMagnitude(remainder, numerator, second)) {
            remainder->negative = 0;
            tries = 0;
            while (CryptCompare(remainder, &reducer->modulus) >= 0) {
                if (tries > 2) {
                    goto done;
                }
                tries++;
                if (!SubMagnitude(remainder, remainder, &reducer->modulus)) {
                    goto done;
                }
                if (!AddWord(quotient, 1)) {
                    goto done;
                }
            }
            if (remainder->length == 0 || (remainder->length == 1 && remainder->digits[0] == 0)) {
                remainder->negative = 0;
            } else {
                remainder->negative = numerator->negative;
            }
            ok = TRUE;
            quotient->negative = numerator->negative ^ reducer->modulus.negative;
        }
    }
done:
    pool->used = saved;
    return ok;
}

// The reciprocal of the modulus, scaled by 2^bits, and the bits when it worked
static int ComputeReciprocal(BigNum *reciprocal, const BigNum *modulus, int bits, BigNumPool *pool) {
    BigNum power;
    int result = -1;

    InitNum(&power);
    CryptSetWord(&power, 0);
    if (CryptSetBit(&power, bits)) {
        if (BigDiv(reciprocal, NULL, &power, modulus, pool)) {
            result = bits;
        }
    }
    CryptDestroy(&power);
    return result;
}

// Raises to a power modulo with squarings and multiplications, a window of the exponent at a time
static BOOL ModExpPlain(BigNum *dest, const BigNum *base, const BigNum *exponent, const BigNum *modulus,
                        BigNumPool *pool) {
    BOOL ok = FALSE;
    int count = 0;
    int window;
    int entries;
    int bits = BitLength(exponent);
    int bit;
    int k;
    int sign;
    BOOL first = TRUE;
    BigNum table[16];
    Reducer reducer;
    int squared;

    if (base->length == 0 || (base->length == 1 && base->digits[0] == 0)) {
        CryptSetWord(dest, 0);
        return TRUE;
    }
    if (exponent->length == 0 || (exponent->length == 1 && exponent->digits[0] == 0)) {
        CryptSetWord(dest, 1);
        return TRUE;
    }
    if (exponent->length == 1 && exponent->digits[0] == 1) {
        CryptCopy(dest, base);
        return TRUE;
    }
    InitReducer(&reducer);
    if (!SetReducerModulus(&reducer, modulus)) {
        goto done;
    }
    InitNum(&table[0]);
    count = 1;
    squared = pool->used;
    pool->used++;
    if (!BigMod(&table[0], base, modulus, pool)) {
        goto done;
    }
    if (!ModMul(&pool->numbers[squared], &table[0], &table[0], &reducer, pool)) {
        goto done;
    }
    if (bits <= 17) {
        window = count;
    } else if (bits >= 256) {
        window = 5;
    } else if (bits >= 128) {
        window = 4;
    } else {
        window = 3;
    }
    entries = 1 << (window - 1);
    for (k = 1; k < entries; k++) {
        InitNum(&table[k]);
        if (!ModMul(&table[k], &table[k - 1], &pool->numbers[squared], &reducer, pool)) {
            goto done;
        }
    }
    count = k;
    if (!CryptSetWord(dest, 1)) {
        goto done;
    }
    bit = bits - 1;
    for (;;) {
        int value;
        int last;

        if (!CryptTestBit(exponent, bit)) {
            if (!first) {
                if (!ModMul(dest, dest, dest, &reducer, pool)) {
                    goto done;
                }
            }
            if (bit == 0) {
                break;
            }
            bit--;
            continue;
        }
        value = 1;
        last = 0;
        for (sign = 1; sign < window; sign++) {
            if (bit - sign < 0) {
                break;
            }
            if (CryptTestBit(exponent, bit - sign)) {
                value = (value << (sign - last)) | 1;
                last = sign;
            }
        }
        if (!first) {
            for (k = 0; k < last + 1; k++) {
                if (!ModMul(dest, dest, dest, &reducer, pool)) {
                    goto done;
                }
            }
        }
        if (!ModMul(dest, dest, &table[value >> 1], &reducer, pool)) {
            goto done;
        }
        first = FALSE;
        bit -= last + 1;
        if (bit < 0) {
            break;
        }
    }
    ok = TRUE;
done:
    pool->used--;
    for (k = 0; k < count; k++) {
        FreeNum(&table[k]);
    }
    FreeReducer(&reducer);
    return ok;
}

// Reads a pair of the exponent's windows: a window's value and the number of squarings before the next one
static inline void ReadWindow(const u8 **cursor, int *index, int *squarings) {
    const u8 *p = *cursor;
    int value = p[0];
    int count = p[1];

    p += 2;
    if (count == 0xff && value == 0) {
        for (;;) {
            value = p[0];
            if (!(p[1] == 0xff && value == 0)) {
                count += p[1] + 1;
                p += 2;
                break;
            }
            count += 0x100;
            p += 2;
        }
    }
    *cursor = p;
    *index = value;
    *squarings = count;
}

// Raises to a power modulo an odd number in the Montgomery form, with a table of the odd powers of the base
static BOOL ModExpMont(BigNum *dest, const BigNum *base, const BigNum *exponent, const BigNum *modulus,
                       BigNumPool *pool, Montgomery *mont) {
    BOOL ok = FALSE;
    int saved = pool->used;
    const u8 *cursor;
    CryptProgress *progress = NULL;
    int counter = 0;
    Montgomery *made = mont;
    BigNum *reduced;
    u32 *table[16];
    int n;
    int window;
    int entries;
    int perWord;
    int index;
    int squarings;
    int i;
    u32 *acc;
    u32 *product;
    u32 *scratch;
    u32 *modulusDigits;
    u32 inverse;
    int tableWords;
    BigNum *tableNum;
    BigNum *scratchNum;
    BigNum *accNum;
    BigNum *productNum;
    int next;

    if ((modulus->digits[0] & 1) == 0) {
        return FALSE;
    }
    if (base->length == 0 || (base->length == 1 && base->digits[0] == 0)) {
        CryptSetWord(dest, 0);
        return TRUE;
    }
    if (exponent->length == 0 || (exponent->length == 1 && exponent->digits[0] == 0)) {
        CryptSetWord(dest, 1);
        return TRUE;
    }
    if (exponent->length == 1 && exponent->digits[0] == 1) {
        CryptCopy(dest, base);
        return TRUE;
    }
    if (mont == NULL) {
        made = NewMont();
        if (made == NULL) {
            goto done;
        }
        if (!MontInit(made, modulus, pool)) {
            goto done;
        }
    }
    progress = pool->progress;
    if (!EncodeExponent(exponent, &cursor, 0, pool)) {
        goto done;
    }
    pool->used++;
    window = cursor[2];
    entries = cursor[3];
    perWord = (window + 63) / window;
    cursor += 4;
    n = made->modulus.length;
    if (base->length == n && base->digits[n - 1] < modulus->digits[n - 1]) {
        reduced = (BigNum *)base;
    } else if (base->length < n) {
        reduced = &pool->numbers[pool->used];
        pool->used++;
        reduced->length = base->length;
        if (CryptResize(reduced, n) == NULL) {
            goto done;
        }
        for (i = 0; i < base->length; i++) {
            reduced->digits[i] = base->digits[i];
        }
    } else {
        reduced = &pool->numbers[pool->used];
        pool->used++;
        if (!BigMod(reduced, base, modulus, pool)) {
            goto done;
        }
        if (CryptResize(reduced, n) == NULL) {
            goto done;
        }
    }
    next = pool->used;
    tableNum = &pool->numbers[next];
    scratchNum = &pool->numbers[next + 1];
    accNum = &pool->numbers[next + 2];
    productNum = &pool->numbers[next + 3];
    pool->used = next + 4;
    tableWords = entries * n + (exponent->length * perWord * 2 + 7) / 4;
    if (n > dest->capacity) {
        if (CryptExpand(dest, n) == NULL) {
            goto done;
        }
    }
    if (scratchNum->capacity < n * 4) {
        if (CryptExpand(scratchNum, n * 4) == NULL) {
            goto done;
        }
    }
    if (accNum->capacity < n * 2) {
        if (CryptExpand(accNum, n * 2) == NULL) {
            goto done;
        }
    }
    if (tableNum->capacity < tableWords) {
        if (CryptExpand(tableNum, tableWords) == NULL) {
            goto done;
        }
    }
    if (productNum->capacity < n * 2) {
        if (CryptExpand(productNum, n * 2) == NULL) {
            goto done;
        }
    }
    table[0] = tableNum->digits;
    scratch = scratchNum->digits;
    acc = accNum->digits;
    product = productNum->digits;
    inverse = made->inverse;
    modulusDigits = made->modulus.digits;
    MulWords(product, reduced->digits, n, made->rSquared.digits, n);
    MontReduce(table[0], product, modulusDigits, n, inverse);
    if (entries > 1) {
        SquareWords(product, table[0], n, scratch);
        MontReduce(scratch, product, modulusDigits, n, inverse);
        for (i = 1; i < entries; i++) {
            table[i] = table[i - 1] + n;
            MulWords(product, table[i - 1], n, scratch, n);
            MontReduce(table[i], product, modulusDigits, n, inverse);
        }
    }
    ReadWindow(&cursor, &index, &squarings);
    sys_memcpy(table[index >> 1], acc, n * sizeof(u32));
    while (squarings != 0) {
        if (progress != NULL) {
            int stop = progress->func(progress, 0xff, counter);

            counter++;
            if (stop != 0) {
                goto done;
            }
        }
        if ((pool->flags & POOL_CANCELED) != 0) {
            goto done;
        }
        for (i = 0; i < squarings; i++) {
            SquareWords(product, acc, n, scratch);
            MontReduce(acc, product, modulusDigits, n, inverse);
        }
        ReadWindow(&cursor, &index, &squarings);
        if (index == 0 && squarings == 0) {
            break;
        }
        if (squarings == 0 && index == 1) {
            MulWords(product, acc, n, reduced->digits, n);
            MontReduce(dest->digits, product, modulusDigits, n, inverse);
            goto finish;
        }
        MulWords(product, acc, n, table[index >> 1], n);
        MontReduce(acc, product, modulusDigits, n, inverse);
    }
    for (i = n; i < n * 2; i++) {
        acc[i] = 0;
    }
    MontReduce(dest->digits, acc, modulusDigits, n, inverse);
finish:
    if ((pool->flags & POOL_CANCELED) != 0) {
        goto done;
    }
    dest->length = n;
    CryptNormalize(dest);
    ok = TRUE;
done:
    if (progress != NULL) {
        if (progress->func(progress, 0xff, -1) != 0) {
            ok = FALSE;
        }
    }
    if (mont == NULL && made != NULL) {
        FreeMont(made);
    }
    pool->used = saved;
    return ok;
}

// The primitives of the numbers' words are MWCC `asm` functions: the assembly that the bundled big-number library
// shipped, unrolled by four with the long multiplications and the add with carry that the compiler does not use
static asm u32 MulAddWords(u32 *dest, const u32 *a, int size, u32 word) {
    stmdb sp!, {r4, r5, r6, r7, r8, lr}
    mov r8, r0
    mov lr, #0x0
    mov r0, #0x0
    subs r2, r2, #0x4
    blt MulAddWords_Rest
    ldr r7, [r8, #0x0]
    ldr r6, [r1], #0x4
MulAddWords_Loop4:
    adds r7, r7, r0
    adc r0, lr, #0x0
    umlal r7, r0, r3, r6
    ldr r5, [r8, #0x4]
    ldr r4, [r1], #0x4
    str r7, [r8], #0x4
    adds r5, r5, r0
    adc r0, lr, #0x0
    umlal r5, r0, r3, r4
    ldr r7, [r8, #0x4]
    ldr r6, [r1], #0x4
    str r5, [r8], #0x4
    adds r7, r7, r0
    adc r0, lr, #0x0
    umlal r7, r0, r3, r6
    ldr r5, [r8, #0x4]
    ldr r4, [r1], #0x4
    str r7, [r8], #0x4
    adds r5, r5, r0
    adc r0, lr, #0x0
    umlal r5, r0, r3, r4
    subs r2, r2, #0x4
    ldrge r7, [r8, #0x4]
    ldrge r6, [r1], #0x4
    str r5, [r8], #0x4
    bge MulAddWords_Loop4
MulAddWords_Rest:
    adds r2, r2, #0x4
    ldmleia sp!, {r4, r5, r6, r7, r8, pc}
MulAddWords_Loop1:
    ldr r7, [r8], #0x0
    ldr r6, [r1], #0x4
    adds r7, r7, r0
    adc r0, lr, #0x0
    umlal r7, r0, r3, r6
    subs r2, r2, #0x1
    str r7, [r8], #0x4
    bgt MulAddWords_Loop1
    ldmia sp!, {r4, r5, r6, r7, r8, pc}
}

static asm u32 MulWord(u32 *dest, const u32 *a, int size, u32 word) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    mov r11, r0
    mov r0, #0x0
    subs r2, r2, #0x4
    blt MulWord_Rest
MulWord_Loop4:
    ldr r9, [r1], #0x4
    umull r9, r10, r3, r9
    ldr r7, [r1], #0x4
    ldr r5, [r1], #0x4
    umull r7, r8, r3, r7
    ldr lr, [r1], #0x4
    adds r9, r9, r0
    umull r5, r6, r3, r5
    adcs r7, r7, r10
    str r9, [r11], #0x4
    umull lr, r4, r3, lr
    str r7, [r11], #0x4
    adcs r5, r5, r8
    str r5, [r11], #0x4
    adcs lr, lr, r6
    adc r0, r4, #0x0
    str lr, [r11], #0x4
    subs r2, r2, #0x4
    bge MulWord_Loop4
MulWord_Rest:
    adds r2, r2, #0x4
    ldmleia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
MulWord_Loop1:
    ldr r9, [r1], #0x4
    mov r10, #0x0
    umlal r0, r10, r9, r3
    subs r2, r2, #0x1
    str r0, [r11], #0x4
    mov r0, r10
    bgt MulWord_Loop1
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
}

static asm void SquareDigits(u32 *dest, const u32 *a, int size) {
    stmdb sp!, {r4, r5, lr}
    subs r2, r2, #0x1
    ldmltia sp!, {r4, r5, pc}
SquareDigits_Loop:
    ldr lr, [r1], #0x4
    umull r4, r5, lr, lr
    subs r2, r2, #0x1
    str r4, [r0], #0x4
    str r5, [r0], #0x4
    bge SquareDigits_Loop
    ldmia sp!, {r4, r5, pc}
}

static u32 DivWords(u32 high, u32 low, u32 divisor) {
    return (((u64)high << 32) | low) / divisor;
}

static asm u32 AddWords(u32 *dest, const u32 *a, const u32 *b, int size) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, lr}
    mov r9, r0
    subs r3, r3, #0x4
    mov r0, #0x0
    mov lr, #0x0
    blt AddWords_Rest
AddWords_Loop4:
    ldr r8, [r1], #0x4
    ldr r6, [r2], #0x4
    subs r0, r0, #0x1
    ldr r7, [r1], #0x4
    ldr r5, [r2], #0x4
    adcs r8, r8, r6
    adcs r7, r7, r5
    str r8, [r9], #0x4
    str r7, [r9], #0x4
    ldr r8, [r1], #0x4
    ldr r6, [r2], #0x4
    ldr r7, [r1], #0x4
    ldr r5, [r2], #0x4
    adcs r8, r8, r6
    adcs r7, r7, r5
    adc r0, lr, #0x0
    str r8, [r9], #0x4
    subs r3, r3, #0x4
    str r7, [r9], #0x4
    bge AddWords_Loop4
AddWords_Rest:
    adds r3, r3, #0x4
    ldmleia sp!, {r4, r5, r6, r7, r8, r9, pc}
AddWords_Loop1:
    ldr r8, [r1], #0x4
    ldr r6, [r2], #0x4
    subs r0, r0, #0x1
    adcs r8, r8, r6
    adc r0, lr, #0x0
    str r8, [r9], #0x4
    subs r3, r3, #0x1
    bgt AddWords_Loop1
    ldmia sp!, {r4, r5, r6, r7, r8, r9, pc}
}

static asm u32 SubWords(u32 *dest, const u32 *a, const u32 *b, int size) {
    stmdb sp!, {r4, r5, r6, r7, lr}
    mov r7, r0
    subs r3, r3, #0x1
    mov r0, #0x0
    ldmltia sp!, {r4, r5, r6, r7, pc}
    mov lr, #0x0
SubWords_Loop:
    ldr r5, [r2], #0x4
    ldr r6, [r1], #0x4
    adds r5, r5, r0
    adc r0, lr, #0x0
    subs r6, r6, r5
    movlo r0, #0x1
    subs r3, r3, #0x1
    str r6, [r7], #0x4
    bge SubWords_Loop
    ldmia sp!, {r4, r5, r6, r7, pc}
}
