// Frozen phase-0 spike (port/spikes/README.md), outside Frigate's build: the live code is port/hal/fs/ovsec.h; change
// that, not this copy.
#ifndef OVSEC_H
#define OVSEC_H

/*
 * Per-overlay writable data, reset the way the DS reloads an overlay.
 *
 * On the DS, FS_LoadOverlay copies an overlay's .data from the ROM and clears its .bss every time it is loaded, and
 * some game code relies on that (engine-boot.md section 3.3). The host links every overlay into one executable, so
 * each overlay's C files are compiled with a forced include, ovNNN_sections.h, that the source-list generator
 * writes: its "#pragma clang section" puts the file's initialized writable statics (globals, file statics and
 * function-local statics) in the overlay's data section and its zero-initialized ones in its bss section.
 * Constants, string literals and code are not affected.
 *
 * OvSec_Scan finds those sections in the running image. OvSec_Snapshot copies every data section once, at startup,
 * after the dynamic loader has applied its fixups, so pointers in the copy are the slid ones. OvSec_Reset then puts
 * an overlay's data back and zeroes its bss; the overlay loader calls it wherever the DS would copy the overlay in.
 *
 * Section names (ovNNN is the overlay ID in three digits):
 *   Mach-O  __DATA,__ovNNN_d and __DATA,__ovNNN_b (zerofill), found by walking the image's load commands.
 *   ELF     ovNNN_d and ovNNN_b, bounded by the linker's __start_ and __stop_ symbols.
 *   COFF    .data$ovNNNm and .bss$ovNNNm, grouped into .data and .bss and bounded by markers that this file
 *           defines in $a and $z.
 * A combined layout, one section per overlay with zero-initialized objects folded into it, is also understood
 * (Mach-O __DATA,__ovNNN, ELF ovNNN, COFF .data$ovNNNm); it needs -fno-zero-initialized-in-bss on the overlay files
 * and, under LTO, at link time too.
 */

#include <stddef.h>

/* Overlay IDs are three digits in section names. */
#define OVSEC_MAX_OVERLAYS 1000

typedef struct OvSecRange {
    unsigned char *start;
    size_t size;
} OvSecRange;

typedef struct OvSecOverlay {
    OvSecRange data;
    OvSecRange bss;
    unsigned char *snapshot;
} OvSecOverlay;

/* Finds the overlay sections of the image that contains this code and returns the number of bytes
 * OvSec_Snapshot needs. Calling it again starts over. The game always has overlays with writable statics, so the
 * HAL stops at startup when the scan finds no section: the forced includes didn't take effect. A build with
 * OVSEC_DISABLED (a compiler without "#pragma clang section", such as GCC as an extra check) finds none on purpose,
 * and the HAL says at startup that overlays are not reset. */
size_t OvSec_Scan(void);

/* Copies every overlay's data section into buffer, which holds at least the size OvSec_Scan returned and
 * lives as long as the overlays are used. */
void OvSec_Snapshot(unsigned char *buffer);

/* Restores overlay id's data from the snapshot and zeroes its bss. An overlay without sections, which has no
 * writable statics on the host, is left alone. */
void OvSec_Reset(unsigned id);

/* Overlay id's sections, or NULL when it has none. */
const OvSecOverlay *OvSec_Get(unsigned id);

/* The number of overlays and of sections found by the last scan. */
unsigned OvSec_OverlayCount(void);
unsigned OvSec_SectionCount(void);

/* Mach-O only: the number of sections in the whole image, which can't exceed 255 (nlist's n_sect is a byte; an
 * image past that doesn't link). For logging; the build checks the count before the link. Returns 0 on other
 * formats. */
unsigned OvSec_ImageSectionCount(void);

#endif /* OVSEC_H */
