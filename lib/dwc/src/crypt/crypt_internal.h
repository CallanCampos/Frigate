#ifndef POKEBW2_DWC_CRYPT_INTERNAL_H
#define POKEBW2_DWC_CRYPT_INTERNAL_H

#include "types.h"

// What crypt.c calls outside itself, none of it with a name in the ROM

// The SHA-1 digest of size bytes into 20 bytes
void func_02077720(u8 *digest, const void *data, u32 size);

#endif // POKEBW2_DWC_CRYPT_INTERNAL_H
