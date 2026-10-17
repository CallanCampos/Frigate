// Each overlay's writable statics, set back the way the DS reloads an overlay (PLAN.md §4.5; the reviewed design of
// port/spikes/overlay-sections/README.md).
//
// On the DS, FS_LoadOverlay clears an overlay's .bss and copies its .data from the ROM every time it loads the overlay,
// and some game code relies on it (lazily created state that expects NULL, counters nothing else resets). The host
// links every overlay into the program, so each overlay's C files are compiled with a forced include
// (port/tools/gen_overlay_sections.py, applied by port/cmake/overlay_sections.cmake) whose "#pragma clang section" puts
// their initialized writable statics in the overlay's data section and their zeroed ones in its bss section. Constants,
// string literals and code aren't touched.
//
// port_ovsec_init finds those sections in the running program and copies every data section once, after the dynamic
// loader's fixups, so that the copy holds the pointers as they are in this run; port_ovsec_reset then puts an
// overlay's data back and zeroes its bss, where the DS would read the overlay in (the host's FS_LoadOverlayImage).
//
// Section names, NNN being the overlay's number in three digits:
//   Mach-O  __DATA,__ovNNN_d and __DATA,__ovNNN_b (zerofill), found by walking the program's load commands;
//   ELF     ovNNN_d and ovNNN_b, between the linker's __start_ and __stop_ symbols;
//   COFF    .data$ovNNNm and .bss$ovNNNm, grouped into .data and .bss between markers this module defines in $a and $z.
// The combined layout, one section per overlay with its zeroed statics in it, is __DATA,__ovNNN, ovNNN or .data$ovNNNm.
#ifndef PORT_HAL_FS_OVSEC_H
#define PORT_HAL_FS_OVSEC_H

#include <stddef.h>

// Overlay numbers have three digits in the sections' names
#define PORT_OVSEC_MAX_OVERLAYS 1000

typedef struct PortOvsecRange {
    unsigned char *start;
    size_t size;
} PortOvsecRange;

typedef struct PortOvsecOverlay {
    PortOvsecRange data;
    PortOvsecRange bss;
    unsigned char *snapshot; // the data section as it was before anything ran
} PortOvsecOverlay;

// Finds the overlay sections and snapshots their data, once per process; later calls do nothing. Returns the number of
// overlays with sections. A program with overlay code has some, so finding none means that the forced includes didn't
// take effect, and stops the program, unless the build defines PORT_OVSEC_DISABLED (a compiler without "#pragma clang
// section"), which it then says once: overlays aren't reset
unsigned port_ovsec_init(void);

// Sets overlay id back as the DS's load leaves it: its data from the snapshot, its bss zeroed. The two halves apart, as
// the SDK's overlay loading does them (FS_ClearOverlayImage clears the bss; the read of the image brings the data). An
// overlay without sections, which has no writable statics on the host, is left as it is
void port_ovsec_reset(unsigned id);
void port_ovsec_clear_bss(unsigned id);
void port_ovsec_restore_data(unsigned id);

// Overlay id's sections, or NULL when it has none
const PortOvsecOverlay *port_ovsec_get(unsigned id);

// The overlays and the sections the scan found
unsigned port_ovsec_overlay_count(void);
unsigned port_ovsec_section_count(void);
// On Mach-O, every section of the program, which can't pass 255 (nlist's n_sect is a byte); 0 elsewhere. The build
// counts them before the link (port/tools/gen_overlay_sections.py check); this is for logs and tests
unsigned port_ovsec_image_section_count(void);

#endif // PORT_HAL_FS_OVSEC_H
