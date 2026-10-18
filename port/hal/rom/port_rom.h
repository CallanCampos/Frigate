// The player's ROM, the program's one source of the game's files (PLAN.md §1, §4.5). The ROM is opened once per
// process from the dump the player gives: --rom PATH, else the FRIGATE_ROM environment variable, else, for
// development, orig/baserom_<version>.nds in the repository the program was built from. It is mapped read-only and
// checked: its game code must be the version the program was built for (IREO for Black 2, IRDO for White 2), and the
// dump must be the one the decomp builds, by the SHA-1 of the whole file (b2_us.sha1, w2_us.sha1), or, failing that,
// what the host reads of it must match the port's manifest (rom_manifest.h): every file of its NitroFS
// (port/tools/research/assets/nitrofs_manifest.json), the ARM9 overlay table, and each overlay's file as long as the
// image its header gives. That accepts trimmed and re-padded dumps. Anything else stops the program with a message that
// says what is wrong.
//
// The CARD library's ROM reads (port/hal/card/card_rom.c) read the mapped file as a card answers them; and
// port_rom_place_system_area puts what the DS's boot code and ARM7 leave in main memory for the card and file system
// before the game's main() runs.
// Plain C types only, like port_os.h.
#ifndef PORT_HAL_ROM_PORT_ROM_H
#define PORT_HAL_ROM_PORT_ROM_H

#include <stddef.h>
#include <stdint.h>

// The exit status of a program that has no usable ROM (sysexits.h's EX_NOINPUT)
#define PORT_ROM_EXIT_STATUS 66

// The environment variable that names the player's ROM when --rom doesn't
#define PORT_ROM_ENVIRONMENT_VARIABLE "FRIGATE_ROM"

// How a ROM passed its check
typedef enum PortRomVerdict {
    PORT_ROM_REFUSED = 0, // not a usable dump of this version
    PORT_ROM_EXACT = 1,   // the SHA-1 of the whole file is the decomp's
    PORT_ROM_FILES = 2,   // another dump (trimmed, re-padded, another header), whose NitroFS files all match
} PortRomVerdict;

// Opens, maps and checks the ROM the options name (above), once per process, then places the system area. Stops the
// program with PORT_ROM_EXIT_STATUS and a message when there is no usable ROM. The program calls it before it runs
// the game; the CARD library's initialization calls it too, so that a program that doesn't still has its ROM
void port_rom_boot(void);

// The open ROM's bytes and size (the size of the file, which a trimmed dump makes smaller than the card's); NULL and 0
// before port_rom_boot
const uint8_t *port_rom_data(void);
size_t port_rom_size(void);
// How the open ROM passed its check
PortRomVerdict port_rom_verdict(void);

// Copies length bytes from offset of the ROM file to dst. What lies past the end of the file reads 0xff, as the unused
// end of a card that a trimmed dump left out
void port_rom_read(void *dst, uint32_t offset, uint32_t length);
// Copies length bytes of a card data read from address to dst, as a retail card answers the read (GBATEK, "DS
// Cartridge Protocol", command B7): the address wraps at the card's capacity (the header's 0x14), and the first 32 KiB,
// the header and the secure area, can't be read: an address below 0x8000 reads 0x8000 + (address & 0x1ff). Within the
// capacity, past the end of a trimmed dump, the card reads 0xff (port_rom_read)
void port_rom_card_read(void *dst, uint32_t address, uint32_t length);

// Puts what the DS's boot code and ARM7 leave at the end of main memory before the game's main() runs, for the card and
// the file system: the ROM header at HW_ROM_HEADER_BUF and the copy at HW_CARD_ROM_HEADER (CARD_Init makes it again
// on a boot from the card), and the card's ID, which the card library compares with the ID of every read
void port_rom_place_system_area(void);
// The card ID the system area and every read report: the DS's form (GBATEK's chip ID: the maker, then the size code)
// for a card of the header's capacity, with no flags
uint32_t port_rom_card_id(void);

// The check itself, for the tests: whether size bytes at data are a usable dump of this version, with what is wrong in
// message (a line without a newline) when not, and with how it passed when it did
PortRomVerdict port_rom_check(const uint8_t *data, size_t size, char *message, size_t messageSize);

// Opens, maps and checks the ROM at path, replacing the one open; for the tests. Returns the verdict, with the message
// as port_rom_check gives it; PORT_ROM_REFUSED leaves no ROM open
PortRomVerdict port_rom_open(const char *path, char *message, size_t messageSize);
// The path port_rom_boot would open, from the options, the environment and the development default, or NULL
const char *port_rom_default_path(void);

#endif // PORT_HAL_ROM_PORT_ROM_H
