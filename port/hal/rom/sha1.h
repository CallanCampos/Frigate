// SHA-1 (FIPS 180-4) for checking the player's ROM and its files against the decomp's hashes (port/hal/rom/rom.c).
// The game's own SHA-1 (NitroSDK's MATH_CalcSHA1) is for the game; this one is the platform layer's, and reads at the
// speed of the host. Plain C types only.
#ifndef PORT_HAL_ROM_SHA1_H
#define PORT_HAL_ROM_SHA1_H

#include <stddef.h>
#include <stdint.h>

#define PORT_SHA1_DIGEST_SIZE 20

typedef struct PortSha1 {
    uint32_t state[5];
    uint64_t length; // bytes so far
    uint8_t block[64];
    size_t blockUsed;
} PortSha1;

void port_sha1_init(PortSha1 *sha1);
void port_sha1_update(PortSha1 *sha1, const void *data, size_t size);
void port_sha1_final(PortSha1 *sha1, uint8_t digest[PORT_SHA1_DIGEST_SIZE]);
// The digest of size bytes at data
void port_sha1(const void *data, size_t size, uint8_t digest[PORT_SHA1_DIGEST_SIZE]);

#endif // PORT_HAL_ROM_SHA1_H
