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
static int BitLength(BigNum *n);
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
static BigNum *CryptCopy(BigNum *dest, BigNum *src);
static void CryptClear(BigNum *n);
static BOOL CryptSetWord(BigNum *n, u32 word);
static BigNum *FromBytes(const u8 *bytes, u32 size, BigNum *out);
static int ToBytes(BigNum *n, u8 *out);
static int CryptCompare(BigNum *a, BigNum *b);
static BOOL CryptSetBit(BigNum *n, int bit);
static BOOL CryptTestBit(BigNum *n, int bit);
static BOOL ShiftLeft(BigNum *dest, BigNum *src, int bits);
static BOOL ShiftRight(BigNum *dest, BigNum *src, int bits);
static BOOL AddWord(BigNum *n, u32 word);
static BOOL SubWord(BigNum *n, u32 word);
static BOOL AddSigned(BigNum *dest, BigNum *a, BigNum *b);
static BOOL AddMagnitude(BigNum *dest, BigNum *a, BigNum *b);
static BOOL SubMagnitude(BigNum *dest, BigNum *a, BigNum *b);
static BOOL SubSigned(BigNum *dest, BigNum *a, BigNum *b);
static BOOL CryptMul(BigNum *dest, BigNum *a, BigNum *b, BigNumPool *pool);
static void MulWords(u32 *dest, const u32 *a, int aSize, const u32 *b, int bSize);
static BOOL BigDiv(BigNum *quotient, BigNum *remainder, BigNum *numerator, BigNum *denominator,
                BigNumPool *pool);
static BOOL BigMod(BigNum *remainder, BigNum *numerator, BigNum *denominator, BigNumPool *pool);
static BOOL ModExp(BigNum *dest, BigNum *base, BigNum *exponent, BigNum *modulus, BigNumPool *pool);
static BOOL CryptSquare(BigNum *dest, BigNum *a, BigNumPool *pool);
static void SquareWords(u32 *dest, const u32 *a, int size, u32 *scratch);
static void MontReduce(u32 *dest, u32 *product, const u32 *modulus, int size, u32 inverse);
static u32 InverseOfWord(u32 x);
static int EncodeExponent(BigNum *exponent, const u8 **encoded, int unused, BigNumPool *pool);
static int EncodeWindows(u8 *dest, BigNum *exponent, int window);
static inline void ReadWindow(const u8 **cursor, int *index, int *squarings);
static BOOL MontInit(Montgomery *mont, BigNum *modulus, BigNumPool *pool);
static Montgomery *NewMont(void);
static void InitMont(Montgomery *mont);
static void FreeMont(Montgomery *mont);
static void InitReducer(Reducer *reducer);
static void FreeReducer(Reducer *reducer);
static int SetReducerModulus(Reducer *reducer, BigNum *modulus, BigNumPool *pool);
static BOOL ModMul(BigNum *dest, BigNum *a, BigNum *b, Reducer *reducer, BigNumPool *pool);
static BOOL CryptReduce(BigNum *quotient, BigNum *remainder, BigNum *numerator, Reducer *reducer, BigNumPool *pool);
static int ComputeReciprocal(BigNum *reciprocal, BigNum *modulus, int bits, BigNumPool *pool);
static BOOL ModExpPlain(BigNum *dest, BigNum *base, BigNum *exponent, BigNum *modulus,
                        BigNumPool *pool);
static BOOL ModExpMont(BigNum *dest, BigNum *base, BigNum *exponent, BigNum *modulus,
                       BigNumPool *pool, Montgomery *mont);
static asm u32 MulAddWords(u32 *dest, const u32 *a, int size, u32 word);
static asm u32 MulWord(u32 *dest, const u32 *a, int size, u32 word);
static asm void SquareDigits(u32 *dest, const u32 *a, int size);
static asm u64 DivWords(u32 high, u32 low, u32 divisor);
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

// The allocator that the game gave: its function to free, the one to allocate, and an argument that is not used. No
// code in the ROM uses its last two words.
static struct {
    CryptFreeFunc free;
    CryptAllocFunc alloc;
    u32 arg;
    u32 unkC;
    u32 unk10;
} sAllocator;

// The number of trailing zero bits of each value of a window, for windows of up to 6 bits: a window of w bits reads the
// last 2^w of them
static const u8 sTrailingZeros[64] = {
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
};

// The number one, which only code that the link dead-stripped used. MWCC drops a static that nothing reads, so these
// are globals, and they are declared here so that .rodata holds the file's objects in the ROM's order.
const u32 CryptOneDigit = 1;
const BigNum CryptOne = {(u32 *)&CryptOneDigit, 1, 1, 0, 0};

static const u8 *sTrailingZerosByWindow[7] = {
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
    u32 left = *size;
    u32 value = 0;
    u8 count;
    u32 b;

    if (*p++ != tag) {
        return FALSE;
    }
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
        left -= count;
        if (length != NULL) {
            u32 digit = *++p & 0x7f;

            do {
                value = digit + (value << 7);
            } while (--count != 0);
        } else {
            p += count;
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
    const u8 *unpadded;
    u32 unpaddedSize = 0;
    const u8 *digest;
    u32 digestSize = 0;
    int size;
    u32 i;

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
        *out++ = digest[i];
    }
    for (; i < outSize; i++) {
        *out++ = 0;
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

    MATH_CalcSHA1(digest, data, size);
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
    if ((rest & 0xf0) != 0) {
        return sNibbleBits[rest >> 4] + shift + 4;
    }
    return sNibbleBits[rest] + shift;
}

static int BitLength(BigNum *n) {
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

// Makes room for words if the number has less, as CryptExpand does, without the call when it has enough
static inline BigNum *Reserve(BigNum *n, int words) {
    return words > n->capacity ? CryptExpand(n, words) : n;
}

// Makes room for a number of bits, with a word to spare when it lacks room
static inline BigNum *ReserveBits(BigNum *n, int bits) {
    return (bits + 31) / 32 <= n->capacity ? n : CryptExpand(n, bits / 32 + 1);
}

// Makes room for words, with zeros above the length
static BigNum *CryptResize(BigNum *n, int words) {
    int i;

    if (n->length < words) {
        if (Reserve(n, words) == NULL) {
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

static BigNum *CryptCopy(BigNum *dest, BigNum *src) {
    if (dest == src) {
        return dest;
    }
    if (Reserve(dest, src->length) == NULL) {
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
    if (ReserveBits(n, 32) == NULL) {
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
static BigNum *FromBytes(const u8 *bytes, u32 size, BigNum *out) {
    u32 word;
    int words;
    int phase;

    if (out == NULL) {
        out = NewNum();
    }
    if (out == NULL) {
        return NULL;
    }
    word = 0;
    if (size == 0) {
        out->length = 0;
        return out;
    }
    if (ReserveBits(out, (size + 2) * 8) == NULL) {
        return NULL;
    }
    words = (size - 1) / 4 + 1;
    out->length = words;
    phase = (size - 1) % 4;
    while (size-- != 0) {
        word = *bytes++ | (word << 8);
        if (phase-- == 0) {
            words--;
            out->digits[words] = word;
            word = 0;
            phase = 3;
        }
    }
    CryptNormalize(out);
    return out;
}

// Writes the number as bytes, the most significant first, and returns how many
static int ToBytes(BigNum *n, u8 *out) {
    int bytes = (BitLength(n) + 7) / 8;
    int i = bytes;

    while (i-- > 0) {
        *out++ = n->digits[i / 4] >> (i % 4 * 8);
    }
    return bytes;
}

// Compares the sizes, which is the difference of their lengths, or the most significant word that differs
static int CryptCompare(BigNum *a, BigNum *b) {
    int diff = a->length - b->length;
    const u32 *ap;
    const u32 *bp;
    int i;

    if (diff != 0) {
        return diff;
    }
    ap = a->digits;
    bp = b->digits;
    for (i = a->length - 1; i >= 0; i--) {
        if (ap[i] != bp[i]) {
            return ap[i] > bp[i] ? 1 : -1;
        }
    }
    return 0;
}

static BOOL CryptSetBit(BigNum *n, int bit) {
    int word = bit / 32;
    int shift = bit % 32;
    int i;

    if (n->length <= word) {
        if (Reserve(n, word + 1) == NULL) {
            return FALSE;
        }
        for (i = n->length; i < word + 1; i++) {
            n->digits[i] = 0;
        }
        n->length = word + 1;
    }
    n->digits[word] |= 1 << shift;
    return TRUE;
}

static BOOL CryptTestBit(BigNum *n, int bit) {
    int word;
    int shift;

    if (bit < 0) {
        return FALSE;
    }
    word = bit / 32;
    shift = bit % 32;
    if (n->length <= word) {
        return FALSE;
    }
    return (n->digits[word] & (1 << shift)) != 0;
}

static BOOL ShiftLeft(BigNum *dest, BigNum *src, int bits) {
    int words = bits / 32;
    int shift;
    int back;
    int i;
    const u32 *in;
    u32 *out;
    u32 word;

    if (Reserve(dest, src->length + words + 1) == NULL) {
        return FALSE;
    }
    dest->negative = src->negative;
    shift = bits % 32;
    back = 32 - shift;
    in = src->digits;
    out = dest->digits;
    out[src->length + words] = 0;
    if (shift == 0) {
        for (i = src->length - 1; i >= 0; i--) {
            out[words + i] = in[i];
        }
    } else {
        for (i = src->length - 1; i >= 0; i--) {
            word = in[i];
            out[words + i + 1] |= word >> back;
            out[words + i] = word << shift;
        }
    }
    sys_memset(out, 0, words * sizeof(u32));
    dest->length = src->length + words + 1;
    CryptNormalize(dest);
    return TRUE;
}

static BOOL ShiftRight(BigNum *dest, BigNum *src, int bits) {
    int words;
    int back;
    int shift;
    int count;
    int i;
    u32 *in;
    u32 *out;
    u32 word;
    u32 low;

    words = bits / 32;
    shift = bits % 32;
    back = 32 - shift;
    if (src->length < words) {
        CryptSetWord(dest, 0);
        return TRUE;
    }
    if (dest != src) {
        if (Reserve(dest, src->length - words + 2) == NULL) {
            return FALSE;
        }
        dest->negative = src->negative;
    }
    in = src->digits + words;
    count = src->length - words;
    dest->length = count;
    out = dest->digits;
    if (shift == 0) {
        for (i = count + 1; i > 0; i--) {
            *out++ = *in++;
        }
    } else {
        word = *in++;
        for (i = 1; i < count; i++) {
            low = word >> shift;
            word = *in++;
            *out++ = low | (word << back);
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
    u32 *digits;
    u32 low;

    if (word == 0) {
        return TRUE;
    }
    if (n->negative) {
        if (n->length > 1) {
            n->negative = 0;
            i = SubWord(n, word);
            n->negative = 1;
            return i;
        }
        digits = n->digits;
        low = digits[0];
        if (low > word) {
            digits[0] = low - word;
        } else if (low < word) {
            n->negative = 0;
            digits[0] = word - low;
        } else {
            n->negative = 0;
            n->length = 0;
        }
        return TRUE;
    }
    if (Reserve(n, n->length + 1) == NULL) {
        return FALSE;
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
    u32 *digits;
    u32 low;

    if (word == 0) {
        return TRUE;
    }
    if (n->negative) {
        n->negative = 0;
        i = AddWord(n, word);
        n->negative = 1;
        return i;
    }
    if (n->length <= 1) {
        if (n->length == 0) {
            if (Reserve(n, 1) == NULL) {
                return FALSE;
            }
            n->digits[0] = word;
            n->negative = 1;
            n->length = 1;
            return TRUE;
        }
        digits = n->digits;
        low = digits[0];
        if (low == word) {
            n->length = 0;
        } else if (low > word) {
            digits[0] = low - word;
        } else {
            n->negative = 1;
            digits[0] = word - low;
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

static BOOL AddSigned(BigNum *dest, BigNum *a, BigNum *b) {
    BigNum *swap;

    if (a->negative ^ b->negative) {
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
    if (a->negative != 0) {
        dest->negative = 1;
    } else {
        dest->negative = 0;
    }
    if (!AddMagnitude(dest, a, b)) {
        return FALSE;
    }
    return TRUE;
}

// Adds the sizes of two numbers
static BOOL AddMagnitude(BigNum *dest, BigNum *a, BigNum *b) {
    BigNum *swap;
    int max;
    int i;
    u32 carry;
    u32 digit;
    u32 *aDigits;
    u32 *destDigits;

    if (a->length < b->length) {
        swap = a;
        a = b;
        b = swap;
    }
    max = a->length;
    i = b->length;
    if (Reserve(dest, max + 1) == NULL) {
        return FALSE;
    }
    dest->length = max;
    aDigits = a->digits;
    destDigits = dest->digits;
    carry = AddWords(destDigits, aDigits, b->digits, i);
    destDigits += i;
    aDigits += i;
    if (carry != 0) {
        while (i < max) {
            digit = *aDigits++;
            i++;
            if ((*destDigits++ = digit + 1) >= digit) {
                carry = 0;
                break;
            }
        }
        if (i >= max && carry != 0) {
            *destDigits++ = 1;
            dest->length++;
        }
    }
    if (destDigits != aDigits) {
        for (; i < max; i++) {
            *destDigits++ = *aDigits++;
        }
    }
    return TRUE;
}

// Subtracts a smaller size from a bigger one
static BOOL SubMagnitude(BigNum *dest, BigNum *a, BigNum *b) {
    int max;
    int min;
    int i;
    int borrow;
    u32 x;
    u32 y;
    u32 *aDigits;
    u32 *bDigits;
    u32 *destDigits;

    if (a->length < b->length) {
        return FALSE;
    }
    max = a->length;
    min = b->length;
    if (Reserve(dest, max) == NULL) {
        return FALSE;
    }
    aDigits = a->digits;
    bDigits = b->digits;
    destDigits = dest->digits;
    borrow = 0;
    for (i = 0; i < min; i++) {
        x = *aDigits++;
        y = *bDigits++;
        if (borrow) {
            borrow = x <= y;
            x = x - y - 1;
        } else {
            borrow = x < y;
            x = x - y;
        }
        *destDigits++ = x;
    }
    if (borrow) {
        while (i < max) {
            i++;
            x = *aDigits++;
            y = x - 1;
            *destDigits++ = y;
            if (x > y) {
                break;
            }
        }
    }
    if (destDigits != aDigits) {
        for (;;) {
            if (i++ >= max) {
                break;
            }
            destDigits[0] = aDigits[0];
            if (i++ >= max) {
                break;
            }
            destDigits[1] = aDigits[1];
            if (i++ >= max) {
                break;
            }
            destDigits[2] = aDigits[2];
            if (i++ >= max) {
                break;
            }
            destDigits[3] = aDigits[3];
            destDigits += 4;
            aDigits += 4;
        }
    }
    dest->length = max;
    CryptNormalize(dest);
    return TRUE;
}

static BOOL SubSigned(BigNum *dest, BigNum *a, BigNum *b) {
    BigNum *swap;
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
    if (Reserve(dest, size) == NULL) {
        return FALSE;
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

static BOOL CryptMul(BigNum *dest, BigNum *a, BigNum *b, BigNumPool *pool) {
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
    if (Reserve(work, total) == NULL) {
        return FALSE;
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
    int swapSize;
    const u32 *swapWords;
    u32 *top;

    if (aSize < bSize) {
        swapWords = a;
        a = b;
        b = swapWords;
        swapSize = aSize;
        aSize = bSize;
        bSize = swapSize;
    }
    top = dest + aSize;
    top[0] = MulWord(dest, a, aSize, b[0]);
    for (;;) {
        if (--bSize <= 0) {
            return;
        }
        top[1] = MulAddWords(dest + 1, a, aSize, b[1]);
        if (--bSize <= 0) {
            return;
        }
        top[2] = MulAddWords(dest + 2, a, aSize, b[2]);
        if (--bSize <= 0) {
            return;
        }
        top[3] = MulAddWords(dest + 3, a, aSize, b[3]);
        if (--bSize <= 0) {
            return;
        }
        top[4] = MulAddWords(dest + 4, a, aSize, b[4]);
        top += 4;
        dest += 4;
        b += 4;
    }
}

// Multiplies the word whose 16-bit halves are in *low and *high by the one whose halves are bLow and bHigh, from
// 16-bit products, and leaves the low and the high word of the product in *low and *high
static inline void MulHalves(u32 *low, u32 *high, u32 bLow, u32 bHigh) {
    u32 aHigh = *high;
    u32 aLow = *low;
    u32 middle = bHigh * aLow;
    u32 cross;

    aLow = bLow * aLow;
    cross = bLow * aHigh;
    aHigh = bHigh * aHigh;
    middle += cross;
    if (middle < cross) {
        aHigh += 0x10000;
    }
    aHigh += (middle >> 16) & 0xffff;
    cross = middle << 16;
    aLow += cross;
    if (aLow < cross) {
        aHigh++;
    }
    *low = aLow;
    *high = aHigh;
}

// Divides, schoolbook: the denominator is shifted up until its top bit is set, then each word of the quotient is
// estimated from the top words of what is left, and the estimate is corrected
static BOOL BigDiv(BigNum *quotient, BigNum *remainder, BigNum *numerator, BigNum *denominator,
                BigNumPool *pool) {
    BigNum window;
    int shift;
    int i;
    int quotientSize;
    BigNum *product;
    BigNum *shiftedNumerator;
    BigNum *shiftedDenominator;
    u32 top;
    u32 *quotientDigit;
    int denominatorSize;
    int numeratorSize;
    u32 next;
    u32 *numeratorTop;

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
    product = &pool->numbers[pool->used];
    product->negative = 0;
    shiftedNumerator = &pool->numbers[pool->used + 1];
    shiftedDenominator = &pool->numbers[pool->used + 2];
    if (quotient == NULL) {
        quotient = &pool->numbers[pool->used + 3];
    }
    shift = 32 - BitLength(denominator) % 32;
    if (!ShiftLeft(shiftedDenominator, denominator, shift)) {
        return FALSE;
    }
    shiftedDenominator->negative = 0;
    if (!ShiftLeft(shiftedNumerator, numerator, shift + 32)) {
        return FALSE;
    }
    shiftedNumerator->negative = 0;
    denominatorSize = shiftedDenominator->length;
    numeratorSize = shiftedNumerator->length;
    quotientSize = numeratorSize - denominatorSize;
    InitNum(&window);
    window.digits = shiftedNumerator->digits + quotientSize;
    window.length = denominatorSize;
    window.capacity = shiftedNumerator->capacity + 1;
    next = 0;
    top = shiftedDenominator->digits[denominatorSize - 1];
    if (denominatorSize != 1) {
        next = shiftedDenominator->digits[denominatorSize - 2];
    }
    numeratorTop = shiftedNumerator->digits + (numeratorSize - 1);
    if (Reserve(quotient, quotientSize + 1) == NULL) {
        goto fail;
    }
    quotient->negative = numerator->negative ^ denominator->negative;
    quotient->length = quotientSize;
    quotientDigit = quotient->digits + (quotientSize - 1);
    if (Reserve(product, denominatorSize + 1) == NULL) {
        goto fail;
    }
    if (CryptCompare(&window, shiftedDenominator) >= 0) {
        if (!SubMagnitude(&window, &window, shiftedDenominator)) {
            goto fail;
        }
        *quotientDigit = 1;
        quotient->digits[quotient->length - 1] = 1;
    } else {
        quotient->length--;
    }
    quotientDigit--;
    for (i = 0; i < quotientSize - 1; i++) {
        u32 estimate;
        u32 high;
        u32 low;
        u32 estimateLow;
        u32 estimateHigh;
        u32 byNextLow;
        u32 byNextHigh;
        u32 byTopLow;
        u32 byTopHigh;
        u32 rest;
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
            byNextLow = next & 0xffff;
            byNextHigh = (next >> 16) & 0xffff;
            estimateLow = estimate & 0xffff;
            estimateHigh = (estimate >> 16) & 0xffff;
            MulHalves(&byNextLow, &byNextHigh, estimateLow, estimateHigh);
            byTopLow = top & 0xffff;
            byTopHigh = (top >> 16) & 0xffff;
            MulHalves(&byTopLow, &byTopHigh, estimateLow, estimateHigh);
            rest = low - byTopLow;
            if (rest > low) {
                byTopHigh++;
            }
            if (high - byTopHigh != 0) {
                break;
            }
            if (byNextHigh < rest) {
                break;
            }
            if (byNextHigh == rest && byNextLow <= numeratorTop[-2]) {
                break;
            }
            estimate--;
        }
        product->digits[denominatorSize] = MulWord(product->digits, shiftedDenominator->digits, denominatorSize, estimate);
        size = denominatorSize + 1;
        while (size > 0) {
            if (product->digits[size - 1] != 0) {
                break;
            }
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
            shiftedNumerator->length += window.length - oldLength;
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
fail:
    return FALSE;
}

static BOOL BigMod(BigNum *remainder, BigNum *numerator, BigNum *denominator, BigNumPool *pool) {
    return BigDiv(NULL, remainder, numerator, denominator, pool);
}

// Raises to a power modulo, by Montgomery's method when the modulus is odd
static BOOL ModExp(BigNum *dest, BigNum *base, BigNum *exponent, BigNum *modulus, BigNumPool *pool) {
    if (modulus->length > 0 && (modulus->digits[0] & 1) != 0) {
        return ModExpMont(dest, base, exponent, modulus, pool, NULL);
    }
    return ModExpPlain(dest, base, exponent, modulus, pool);
}

static BOOL CryptSquare(BigNum *dest, BigNum *a, BigNumPool *pool) {
    int used = pool->used;
    BigNum *work;
    int max;
    int size = a->length;
    BigNum *scratch = &pool->numbers[used];
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
    max = size * 2;
    if (Reserve(work, max) == NULL) {
        return FALSE;
    }
    work->length = max;
    work->negative = 0;
    if (size == 4) {
        SquareWords(work->digits, a->digits, 4, small4);
    } else if (size == 8) {
        SquareWords(work->digits, a->digits, 8, small8);
    } else {
        if (Reserve(scratch, max) == NULL) {
            return FALSE;
        }
        SquareWords(work->digits, a->digits, size, scratch->digits);
    }
    if (max > 0 && work->digits[max - 1] == 0) {
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
    int i;
    int remaining;
    int max;
    const u32 *in;
    u32 *out;

    max = size * 2;
    in = a;
    out = dest;
    out[0] = out[max - 1] = 0;
    out++;
    remaining = size;
    if (--remaining > 0) {
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
    AddWords(dest, dest, dest, max);
    SquareDigits(scratch, a, size);
    AddWords(dest, dest, scratch, max);
}

// Reduces a product of 2 * size words by the modulus the Montgomery way: size words of multiples of the modulus are
// added so that the low half is zero, and the high half, less the modulus when it is not smaller, is the result
static void MontReduce(u32 *dest, u32 *product, const u32 *modulus, int size, u32 inverse) {
    u32 *high = product + size;
    int i;
    u32 carry = 0;
    u32 sum;
    u32 word;
    int j;

    for (i = 0; i < size; i++) {
        word = MulAddWords(product, modulus, size, product[0] * inverse);
        sum = word + carry;
        carry = sum < carry;
        word = *high + sum;
        *high = word;
        if (word < sum) {
            carry++;
        }
        product++;
        high++;
    }
    j = size - 1;
    if (carry == 0) {
        if (product[j] == modulus[j]) {
            while (j > 0) {
                if (product[j] != modulus[j]) {
                    break;
                }
                j--;
            }
        }
        carry = product[j] >= modulus[j];
    }
    if (carry != 0) {
        SubWords(dest, product, modulus, size);
    } else {
        for (i = 0; i < size; i++) {
            dest[i] = product[i];
        }
    }
}

// The inverse of 2^32 modulo a word, by Euclid's algorithm, or 0 when they have a common factor
static u32 InverseOfWord(u32 x) {
    u32 b;
    u32 a;
    u32 prev;
    u32 prevPrev;
    int sign;

    b = -x % x;
    a = x;
    prevPrev = 0;
    prev = 1;
    sign = -1;

    while (b != 0) {
        u32 remainder = a % b;
        u32 q = a / b;
        u32 next = q * prev + prevPrev;

        prevPrev = prev;
        a = b;
        prev = next;
        b = remainder;
        sign = -sign;
    }
    if (sign < 0) {
        prevPrev = x - prevPrev;
    }
    return a == 1 ? prevPrev % x : 0;
}

// Writes the exponent as the windows that a power is computed with, after a header: the length, the window's bits and
// the size of the table of odd powers. The exponents 3, 17 and 65537, which keys use, are kept ready
static int EncodeExponent(BigNum *exponent, const u8 **encoded, int unused, BigNumPool *pool) {
    u8 *bytes;
    int window;
    int entries;
    const u8 *table = NULL;
    BigNum *work = &pool->numbers[pool->used];
    int perWord;
    int size;
    int bits = exponent->length * 32;

    if (exponent->length == 0) {
        return 0;
    }
    if (exponent->length == 1) {
        if (exponent->length == 1 && exponent->digits[0] == 0x10001) {
            table = sEncodedExponent65537;
        } else if (exponent->digits[0] == 0x11 && exponent->length == 1) {
            table = sEncodedExponent17;
        } else if (exponent->digits[0] == 3 && exponent->length == 1) {
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
        if (Reserve(work, size) == NULL) {
            return 0;
        }
        bytes = (u8 *)work->digits;
        size = EncodeWindows(bytes + 4, exponent, window) + 2;
        bytes[0] = size >> 8;
        bytes[1] = size;
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
static int EncodeWindows(u8 *dest, BigNum *exponent, int window) {
    const u8 *zerosTable;
    int total;
    u32 low;
    u8 *out;
    u32 zeros;
    int wordsLeft;
    u32 high;
    u32 *next;
    u32 current;
    u32 index;
    u32 skip;
    int size;
    u32 bitPos;
    u32 mask;

    if (window > 6) {
        window = 6;
    }
    zerosTable = sTrailingZerosByWindow[window];
    mask = (1 << window) - 1;
    bitPos = 0;
    total = (window + exponent->length * 32 - 1) / window * 2 + 2;
    out = dest + total;
    *out-- = 0;
    *out-- = 0;
    next = exponent->digits;
    wordsLeft = exponent->length;
    current = low = *next++;
    if (wordsLeft <= 1) {
        high = 0;
    } else {
        high = *next++;
    }
    zeros = 0;
    for (;;) {
        for (;;) {
            index = current & mask;
            skip = zerosTable[index];
            if (skip == 0) {
                break;
            }
            bitPos += skip;
            zeros += skip;
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
            if (bitPos == 0) {
                current = low;
            } else {
                current = (low >> bitPos) | (high << (32 - bitPos));
            }
        }
        if (index == 0) {
            break;
        }
        *out-- = zeros;
        *out-- = index;
        if (zeros >= 256) {
            while (zeros >= 256) {
                zeros -= 256;
                *out-- = 0xff;
                *out-- = 0;
            }
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
        if (bitPos == 0) {
            current = low;
        } else {
            current = (low >> bitPos) | (high << (32 - bitPos));
        }
    }
    out++;
    size = 2;
    while (out[0] != 0 || out[1] != 0) {
        dest[0] = out[0];
        dest[1] = out[1];
        out += 2;
        size += 2;
        dest += 2;
    }
    dest[0] = 0;
    dest[1] = 0;
    return size;
}

// Sets up the Montgomery form for an odd modulus: its copy, R squared modulo it, and the word that the reduction
// multiplies by
static BOOL MontInit(Montgomery *mont, BigNum *modulus, BigNumPool *pool) {
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
    if (!CryptSetBit(&mont->rSquared, 32)) {
        goto done;
    }
    first = modulus->digits[0];
    if (!CryptSetWord(&work, InverseOfWord(first))) {
        goto done;
    }
    if (!ShiftLeft(&work, &work, 32)) {
        goto done;
    }
    if (!(work.length == 0 || (work.length == 1 && work.digits[0] == 0))) {
        SubWord(&work, 1);
    } else if (!CryptSetWord(&work, -1)) {
        goto done;
    }
    low = work.length >= 1 ? work.digits[0] : 0;
    high = work.length >= 2 ? work.digits[1] : 0;
    mont->inverse = DivWords(high, low, first);
    CryptSetWord(&mont->rSquared, 0);
    if (!CryptSetBit(&mont->rSquared, mont->words * 64)) {
        goto done;
    }
    BigMod(&mont->rSquared, &mont->rSquared, &mont->modulus, pool);
    if (CryptResize(&mont->rSquared, mont->words) != NULL) {
        ok = TRUE;
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

static int SetReducerModulus(Reducer *reducer, BigNum *modulus, BigNumPool *pool) {
    CryptCopy(&reducer->modulus, modulus);
    CryptSetWord(&reducer->reciprocal, 0);
    reducer->modulusBits = BitLength(modulus);
    reducer->reciprocalBits = 0;
    return TRUE;
}

// Multiplies two numbers and reduces the product
static BOOL ModMul(BigNum *dest, BigNum *a, BigNum *b, Reducer *reducer, BigNumPool *pool) {
    BigNum *product = &pool->numbers[pool->used++];
    BOOL ok = FALSE;

    if (b != NULL) {
        if (a == b) {
            if (!CryptSquare(product, a, pool)) {
                goto done;
            }
        } else {
            if (!CryptMul(product, a, b, pool)) {
                goto done;
            }
        }
    } else {
        product = a;
    }
    CryptReduce(NULL, dest, product, reducer, pool);
    ok = TRUE;
done:
    pool->used--;
    return ok;
}

// Whether the number is zero
static inline BOOL IsZero(BigNum *n) {
    return n->length == 0 || (n->length == 1 && n->digits[0] == 0);
}

// Divides with the reciprocal of the modulus: the quotient is taken from the top half of the product with the
// reciprocal, and off by at most 3
static BOOL CryptReduce(BigNum *quotient, BigNum *remainder, BigNum *numerator, Reducer *reducer, BigNumPool *pool) {
    int saved = pool->used;
    BigNum *first = &pool->numbers[pool->used++];
    BigNum *second = &pool->numbers[pool->used++];
    BOOL ok = FALSE;
    int bits;
    int shift;
    int tries;
    int half;

    if (quotient == NULL) {
        quotient = &pool->numbers[pool->used++];
    }
    if (remainder == NULL) {
        remainder = &pool->numbers[pool->used++];
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
    half = bits / 2;
    if (bits != reducer->reciprocalBits) {
        reducer->reciprocalBits = ComputeReciprocal(&reducer->reciprocal, &reducer->modulus, bits, pool);
    }
    if (ShiftRight(first, numerator, half - shift) && CryptMul(second, first, &reducer->reciprocal, pool)
        && ShiftRight(quotient, second, shift + half)) {
        quotient->negative = 0;
        if (CryptMul(second, &reducer->modulus, quotient, pool) && SubMagnitude(remainder, numerator, second)) {
            remainder->negative = 0;
            tries = 0;
            while (CryptCompare(remainder, &reducer->modulus) >= 0) {
                if (tries++ > 2) {
                    goto done;
                }
                if (!SubMagnitude(remainder, remainder, &reducer->modulus)) {
                    goto done;
                }
                if (!AddWord(quotient, 1)) {
                    goto done;
                }
            }
            remainder->negative = IsZero(remainder) ? 0 : numerator->negative;
            ok = TRUE;
            quotient->negative = numerator->negative ^ reducer->modulus.negative;
        }
    }
done:
    pool->used = saved;
    return ok;
}

// The reciprocal of the modulus, scaled by 2^bits, and the bits when it worked
static int ComputeReciprocal(BigNum *reciprocal, BigNum *modulus, int bits, BigNumPool *pool) {
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
static BOOL ModExpPlain(BigNum *dest, BigNum *base, BigNum *exponent, BigNum *modulus,
                        BigNumPool *pool) {
    int j;
    int bits;
    BOOL ok;
    int window;
    int count;
    int bit;
    int k;
    BOOL first;
    BigNum table[16];
    Reducer reducer;
    BigNum *squared;

    ok = FALSE;
    count = 0;
    bits = BitLength(exponent);

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
    if (SetReducerModulus(&reducer, modulus, pool) <= 0) {
        goto done;
    }
    InitNum(&table[0]);
    count = 1;
    squared = &pool->numbers[pool->used++];
    if (!BigMod(&table[0], base, modulus, pool)) {
        goto done;
    }
    if (!ModMul(squared, &table[0], &table[0], &reducer, pool)) {
        goto done;
    }
    if (bits <= 17) {
        window = 1;
    } else if (bits >= 256) {
        window = 5;
    } else if (bits >= 128) {
        window = 4;
    } else {
        window = 3;
    }
    j = 1 << (window - 1);
    for (k = 1; k < j; k++) {
        InitNum(&table[k]);
        if (!ModMul(&table[k], &table[k - 1], squared, &reducer, pool)) {
            goto done;
        }
    }
    count = k;
    first = TRUE;
    bit = bits - 1;
    if (!CryptSetWord(dest, 1)) {
        goto done;
    }
    for (;;) {
        int last;
        int value;

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
        for (k = 1; k < window; k++) {
            if (bit - k < 0) {
                break;
            }
            if (CryptTestBit(exponent, bit - k)) {
                value = (value << (k - last)) | 1;
                last = k;
            }
        }
        j = last + 1;
        if (!first) {
            for (k = 0; k < j; k++) {
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
    int value = *(*cursor)++;

    *squarings = *(*cursor)++;
    if (*squarings == 0xff && value == 0) {
        value = *(*cursor)++;
        while (**cursor == 0xff && value == 0) {
            *squarings += 0x100;
            (*cursor)++;
            value = *(*cursor)++;
        }
        *squarings += *(*cursor)++ + 1;
    }
    *index = value;
}

// Raises to a power modulo an odd number in the Montgomery form, with a table of the odd powers of the base
static BOOL ModExpMont(BigNum *dest, BigNum *base, BigNum *exponent, BigNum *modulus,
                       BigNumPool *pool, Montgomery *mont) {
    const u8 *cursor = NULL;
    BOOL ok = FALSE;
    int saved;
    int entries;
    BigNum *reduced;
    Montgomery *made;
    CryptProgress *progress = NULL;
    int counter = 0;
    u32 *table[16];
    int n;
    int window;
    int perWord;
    int index;
    int squarings;
    int i;
    BigNum *scratchNum;
    BigNum *accNum;
    BigNum *tableNum;
    BigNum *productNum;
    int tableWords;
    u32 *acc;
    u32 inverse;
    u32 *product;
    u32 *modulusDigits;
    u32 *scratch;

    if ((modulus->digits[0] & 1) == 0) {
        return FALSE;
    }
    saved = pool->used;
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
    made = mont;
    if (made == NULL) {
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
        reduced = base;
    } else if (base->length < n) {
        reduced = &pool->numbers[pool->used++];
        reduced->length = base->length;
        if (CryptResize(reduced, n) == NULL) {
            goto done;
        }
        for (i = 0; i < base->length; i++) {
            reduced->digits[i] = base->digits[i];
        }
    } else {
        reduced = &pool->numbers[pool->used++];
        if (!BigMod(reduced, base, modulus, pool)) {
            goto done;
        }
        if (CryptResize(reduced, n) == NULL) {
            goto done;
        }
    }
    tableWords = entries * n + (exponent->length * perWord * 2 + 7) / 4;
    tableNum = &pool->numbers[pool->used++];
    scratchNum = &pool->numbers[pool->used++];
    accNum = &pool->numbers[pool->used++];
    productNum = &pool->numbers[pool->used++];
    if (Reserve(dest, n) == NULL) {
        goto done;
    }
    if (Reserve(scratchNum, n * 4) == NULL) {
        goto done;
    }
    if (Reserve(accNum, n * 2) == NULL) {
        goto done;
    }
    if (Reserve(tableNum, tableWords) == NULL) {
        goto done;
    }
    if (Reserve(productNum, n * 2) == NULL) {
        goto done;
    }
    scratch = scratchNum->digits;
    acc = accNum->digits;
    product = productNum->digits;
    table[0] = tableNum->digits;
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
        if (squarings != 0 || index != 1) {
            MulWords(product, acc, n, table[index >> 1], n);
            MontReduce(acc, product, modulusDigits, n, inverse);
        } else {
            MulWords(product, acc, n, reduced->digits, n);
            MontReduce(dest->digits, product, modulusDigits, n, inverse);
            goto finish;
        }
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

// Divides a double word by a word; the quotient comes back as a double word with a zero high word. Assembly as well:
// it swaps the halves through r4, which it saves, where MWCC would use a scratch register and move the 64-bit halves
// with `orr #0`, and it sits among the assembly helpers. It calls the runtime's unsigned 64-bit division.
extern u64 _ll_udiv(u64 numerator, u64 denominator);

static asm u64 DivWords(u32 high, u32 low, u32 divisor) {
    stmdb sp!, {r4, lr}
    mov r4, r0
    mov r0, r1
    mov r1, r4
    mov r3, #0x0
    bl _ll_udiv
    mov r1, #0x0
    ldmia sp!, {r4, pc}
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
