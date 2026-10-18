// The host's ARM7 side of the game card (PLAN.md §4.5, "Saves"): the backup memory and the card's PXI channel. On the
// DS the ARM9's CARD library (lib/nitro/src/card) asks the ARM7 to read and write the backup chip over the PXI FIFO,
// with the file system's tag; here the ARM7's service is host code on the PXI bus (port/hal/pxi/pxi_bus.h), over an
// image of the chip that BW2 asks for, a 4 Mbit flash (512 KiB; CARD_BACKUP_TYPE_FLASH_4MBITS_EX). The image is kept
// in a file of the save directory, named for the game code (IREO.sav, IRDO.sav), byte for byte the chip as a cartridge
// dump or melonDS's .sav has it; it is written whole, to a temporary file renamed over the old one, as soon as a frame
// passes without a write, every 60 frames while the game goes on writing, and when the program exits. A write that
// fails is reported once and tried again every 60 frames, while the game goes on with the image in memory. A file the
// game only reads is never written.
//
// melonDS's .sav is the same file: melonDS 1.1 gives IREO and IRDO save type 6 in its ROM list, a flash of 524,288
// bytes, and writes the chip's memory to the .sav as it is, with nothing before or after it (its frontend's
// SaveManager), so a save moves either way by copying the file. Only the names differ: melonDS names a save after the
// ROM's file, next to it or in its save folder, where the port names it after the game code. A melonDS save of another
// size is padded with 0xff or cut by melonDS without a word; the port refuses it and never overwrites it, saying what
// it seems to be: a size another chip's saves have, DeSmuME's .dsv by its footer, this chip's image padded with 0xff
// (with how to cut it), or a directory. When the game's file isn't there, a .sav file of the image's size under another
// name in the same directory is named, with how to play it, unless it is named as the port names the saves of this
// game's other versions. The save path may be a symbolic link, to melonDS's .sav to keep one save for both programs:
// the image is written to the file it links to, and the link stays (not on Windows, where a link is replaced).
// port/tools/oracle/README.md, "Saves", has the evidence.
//
// The save directory: --save-dir PATH, else the user's, named after the product (PORT_PRODUCT_NAME, port_app.h):
// ~/Library/Application Support/Frigate on macOS, $XDG_DATA_HOME/Frigate (or ~/.local/share/Frigate) elsewhere,
// %APPDATA%\Frigate on Windows.
// Plain C types only, like port_os.h.
#ifndef PORT_HAL_CARD_PORT_CARD_H
#define PORT_HAL_CARD_PORT_CARD_H

#include <stddef.h>
#include <stdint.h>

// The backup chip: 4 Mbit of flash, which reads 0xff where it is erased
#define PORT_CARD_BACKUP_SIZE 0x80000

// The exit status of a program whose save file can't be used: one of another size, which is never overwritten, or a
// save directory that can't be found or read (sysexits.h's EX_DATAERR)
#define PORT_CARD_EXIT_STATUS 65

// What the DS's ARM7 has done for the card by the time the ARM9's main() runs: its services are registered on the PXI
// bus, so that the ARM9's CARD library finds them ready, and it knows of no backup type yet. The CARD library's
// initialization calls it (CARDi_InitRom), after the PXI bus has started; calling it again registers the services
// again, as after a reset of the bus. Without a PXI bus in the build it says so and registers nothing
void port_card_boot(void);

// Writes the backup image to its file now if the game changed it since the last write
void port_card_flush(void);

// The backup image (loaded from its file at the first command that needs it), and its file's path; for the tests and
// for the save import and export to come
uint8_t *port_card_backup_image(void);
const char *port_card_save_path(void);
// Uses path for the backup image from now on, in place of the save directory's file, and forgets the image in memory
// with any change not yet written (for the tests); NULL goes back to the save directory
void port_card_set_save_path(const char *path);

// The ARM7's service of the file system's PXI tag, which the CARD library's backup commands use, as the bus calls it
// with each word the ARM9 sends; for the tests, which drive it with a stand-in bus
void port_card_on_fs_word(uint32_t tag, uint32_t data, int err);

#endif // PORT_HAL_CARD_PORT_CARD_H
