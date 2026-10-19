#include "dwc/gsCrypt.h"
#include "dwc/ext.h"

// GameSpy's RSA (gsCrypt.c). The ROM names neither the file nor its function; both are GameSpy's, as pret's pokerevo
// Wii build has the file (gsCrypt.s) and the later SDK the function

int gsCryptRSAVerifySignedHash(const RsaPublicKey *key, const u8 *hash, u32 hashSize, const u8 *signature,
                               u32 signatureSize) {
    BigNum value;
    // The signature as bytes, once it is decrypted: 00 01, FF up to a 00, the DER prefix of the hash, the hash
    u8 *bytes = (u8 *)value.data;
    const u8 md5Prefix[18] = { 0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86, 0x48,
                               0x86, 0xf7, 0x0d, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10 };
    const u8 sha1Prefix[15] = {
        0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
    };
    int start;
    int i;
    u32 size;

    if (hash == NULL || signature == NULL) {
        return -1;
    }
    if (signatureSize != key->modulus.length * sizeof(u32)) {
        return -1;
    }
    if (hashSize != 16 && hashSize != 20) {
        return -1;
    }
    value.length = signatureSize / sizeof(u32);
    memcpy(value.data, signature, signatureSize);
    gsLargeIntReverseBytes(&value);
    gsLargeIntPowerMod(&value, &key->exponent, &key->modulus, &value);
    gsLargeIntReverseBytes(&value);
    if (bytes[0] != 0 || bytes[1] != 1) {
        return -2;
    }
    for (i = 2; i < 128; i++) {
        if (bytes[i] == 0) {
            break;
        }
        if (bytes[i] != 0xff) {
            return -3;
        }
    }
    start = i + 1;
    size = value.length * sizeof(u32);
    if (start + 0x22 == size) {
        if (memcmp(md5Prefix, &bytes[start], 18) != 0) {
            return -4;
        }
        if (hashSize != 16) {
            return -5;
        }
        if (memcmp(&bytes[i + 1 + 18], hash, 16) != 0) {
            return -5;
        }
    } else if (start + 0x23 == size) {
        if (memcmp(sha1Prefix, &bytes[start], 15) != 0) {
            return -4;
        }
        if (hashSize != 20) {
            return -5;
        }
        if (memcmp(&bytes[i + 1 + 15], hash, 20) != 0) {
            return -5;
        }
    } else {
        return -4;
    }
    return 0;
}
