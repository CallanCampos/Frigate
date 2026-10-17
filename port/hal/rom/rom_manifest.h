// What the port knows of the ROM it was built for, written at configure time by port/cmake/rom.cmake into the build's
// rom_manifest.c: the version's game code, the SHA-1 of the dump the decomp builds (from <version>.sha1), the size and
// SHA-1 of its ARM9 overlay table (from rom.cmake), and the size and SHA-1 of every NitroFS file (from
// port/tools/research/assets/nitrofs_manifest.json). Hashes only: nothing of the game itself.
#ifndef PORT_HAL_ROM_ROM_MANIFEST_H
#define PORT_HAL_ROM_ROM_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

typedef struct PortRomManifestFile {
    const char *path; // as the file name table builds it, without a leading slash: "a/0/0/4"
    uint32_t size;
    uint8_t sha1[20];
} PortRomManifestFile;

extern const char port_rom_manifest_version[];   // "b2_us"
extern const char port_rom_manifest_game_code[]; // "IREO"
extern const char port_rom_manifest_title[];     // "Pokémon Black 2 (US)"
extern const uint8_t port_rom_manifest_sha1[20];
// The ARM9 overlay table (the header's 0x50 and 0x54): its size, 32 bytes an overlay, and its SHA-1
extern const uint32_t port_rom_manifest_overlay_table_size;
extern const uint8_t port_rom_manifest_overlay_table_sha1[20];
extern const PortRomManifestFile port_rom_manifest_files[];
extern const size_t port_rom_manifest_count;

#endif // PORT_HAL_ROM_ROM_MANIFEST_H
