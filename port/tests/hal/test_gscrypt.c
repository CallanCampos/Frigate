// AuthService's missing RSA encryption call, through its exact SDK declaration in dwc/gsCrypt.h. NULL and
// ordinary key/input/output arguments must both stop with the named not-in-ROM diagnostic; no buffer is needed.
// check_gscrypt.py verifies exit 70 in separate processes, without accepting an unrelated fault as a trap.
#include "dwc/gsCrypt.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    gsCryptRSAKey key = { 0 };
    unsigned char ciphertext[0x80] = { 0 };
    const unsigned char plaintext[] = "password";
    // Compile-time type checking includes the actual 32-bit return/size and pointer qualifiers.
    gsi_i32 (*encrypt)(const gsCryptRSAKey *, const unsigned char *, gsi_u32, unsigned char *) =
        gsCryptRSAEncryptBuffer;

    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "null") == 0) {
        (void)encrypt(NULL, NULL, 0, NULL);
    } else if (strcmp(argv[1], "buffers") == 0) {
        (void)encrypt(&key, plaintext, sizeof(plaintext) - 1, ciphertext);
    } else {
        return 2;
    }
    fputs("missing RSA encryption function returned\n", stderr);
    return 1;
}
