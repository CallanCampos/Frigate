#ifndef POKEBW2_DWC_GSCRYPT_H
#define POKEBW2_DWC_GSCRYPT_H

#include "dwc/gsLargeInt.h"
#include "dwc/nonport.h"

// GameSpy's RSA (gsCrypt.c), in overlay 11: the check of a signature, which the authentication's certificates have

// A public key: the modulus and the exponent (GameSpy's gsCryptRSAKey)
typedef struct {
    gsLargeInt_t modulus;
    gsLargeInt_t exponent;
} gsCryptRSAKey;

// Checks a PKCS #1 signature of an MD5 or SHA-1 hash with a public key: 0 when it is right, below 0 when it isn't
gsi_i32 gsCryptRSAVerifySignedHash(const gsCryptRSAKey *key, const unsigned char *hash, gsi_u32 hashSize,
                                   const unsigned char *signature, gsi_u32 signatureSize);

#endif // POKEBW2_DWC_GSCRYPT_H
