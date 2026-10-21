#ifndef POKEBW2_DWC_GSCRYPT_H
#define POKEBW2_DWC_GSCRYPT_H

#include "types.h"
#include "dwc/gsLargeInt.h"

// GameSpy's RSA (gsCrypt.c), in overlay 11: the check of a signature, which the authentication's certificates have

// A public key: the modulus and the exponent (GameSpy's gsCryptRSAKey)
typedef struct RsaPublicKey {
    BigNum modulus;
    BigNum exponent;
} RsaPublicKey;

// Checks a PKCS #1 signature of an MD5 or SHA-1 hash with a public key: 0 when it is right, below 0 when it isn't
int gsCryptRSAVerifySignedHash(const RsaPublicKey *key, const u8 *hash, u32 hashSize, const u8 *signature,
                               unsigned int signatureSize);

#endif // POKEBW2_DWC_GSCRYPT_H
