// GameSpy's RSA password encryption (dwc/gsCrypt.h), referenced by the reconstructed AuthService password logins
// (lib/dwc/src/auth/AuthService.c) that the DS link discards. No implementation or address exists in the ROM.
// Retained host references must link, but any call stops visibly rather than inventing encryption or a result.
#include "dwc/gsCrypt.h"
#include "hal/trap.h"
#include <stdio.h>
#include <stdlib.h>

gsi_i32 gsCryptRSAEncryptBuffer(const gsCryptRSAKey *key, const unsigned char *plainText, gsi_u32 size,
                                unsigned char cipherText[0x80]) {
    (void)key;
    (void)plainText;
    (void)size;
    (void)cipherText;
    fputs("port: gsCryptRSAEncryptBuffer: not in ROM (GameSpy RSA password encryption)\n", stderr);
    fflush(stderr);
    _Exit(PORT_TRAP_EXIT_STATUS);
}
