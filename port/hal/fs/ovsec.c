// The overlays' sections (ovsec.h): finding them, the snapshot, and the reset. The Mach-O path is the one the spike
// tested and macOS runs; the ELF path builds on Linux, and the COFF path on Windows, where integrate_overlay_sections
// checks that the sections hold an overlay's statics.
#include "hal/fs/ovsec.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__)
#include <mach-o/loader.h>
// The Mach-O header of the image this file is linked into, which the static linker defines. It keeps the scan right if
// the game is ever linked into a dynamic library, which _dyld_get_image_header(0) wouldn't
extern const struct mach_header_64 __dso_handle;
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PORT_OVSEC_ASAN 1
#endif
#endif
#if !defined(PORT_OVSEC_ASAN) && defined(__SANITIZE_ADDRESS__)
#define PORT_OVSEC_ASAN 1
#endif

static PortOvsecOverlay sOverlays[PORT_OVSEC_MAX_OVERLAYS];
static unsigned sOverlayCount;
static unsigned sSectionCount;
static int sInitialized;

#if defined(PORT_OVSEC_ASAN)
// ASan gives every global a poisoned redzone, and the redzones lie inside the overlays' sections, so memcpy and memset
// over a section report an overflow, even called from an unsanitized function, since ASan intercepts the libc
// functions themselves. A loop in an unsanitized function, which the compiler may not turn back into a library call,
// copies the redzones too; that changes nothing, since their shadow stays poisoned
__attribute__((no_sanitize("address"), no_builtin("memcpy", "memset"), noinline)) static void
CopyBytes(unsigned char *dst, const unsigned char *src, size_t size) {
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
}

__attribute__((no_sanitize("address"), no_builtin("memcpy", "memset"), noinline)) static void
ZeroBytes(unsigned char *dst, size_t size) {
    for (size_t i = 0; i < size; i++) {
        dst[i] = 0;
    }
}
#else
static void CopyBytes(unsigned char *dst, const unsigned char *src, size_t size) {
    __builtin_memcpy(dst, src, size);
}

static void ZeroBytes(unsigned char *dst, size_t size) {
    __builtin_memset(dst, 0, size);
}
#endif

#if defined(PORT_OVSEC_DISABLED)

// A build by a compiler without "#pragma clang section" has no overlay sections (the forced includes refuse to compile
// without this define): the scan finds nothing and the resets do nothing
static void ScanImage(void) {
}

unsigned port_ovsec_image_section_count(void) {
    return 0;
}

#else

// Records one section of overlay id: 'd' for data, set back from the snapshot, 'b' for bss, zeroed. A section the
// linker didn't make (a weak bound that is NULL) is left out
static void AddSection(unsigned id, int kind, unsigned char *start, size_t size) {
    PortOvsecRange *range;

    if (id >= PORT_OVSEC_MAX_OVERLAYS || start == NULL) {
        return;
    }
    range = kind == 'b' ? &sOverlays[id].bss : &sOverlays[id].data;
    range->start = start;
    range->size = size;
    sSectionCount++;
}

#if defined(__APPLE__)

// Compares a Mach-O name field of 16 bytes, not terminated when all 16 are used, with a C string
static int SameName(const char field[16], const char *name) {
    for (int i = 0; i < 16; i++) {
        if (field[i] != name[i]) {
            return 0;
        }
        if (name[i] == '\0') {
            return 1;
        }
    }
    return name[16] == '\0';
}

// Reads "__ovNNN_d", "__ovNNN_b" or, in the combined layout, "__ovNNN" (data)
static int ParseSectionName(const char field[16], unsigned *id, int *kind) {
    if (field[0] != '_' || field[1] != '_' || field[2] != 'o' || field[3] != 'v') {
        return 0;
    }
    *id = 0;
    for (int i = 4; i < 7; i++) {
        if (field[i] < '0' || field[i] > '9') {
            return 0;
        }
        *id = *id * 10 + (unsigned)(field[i] - '0');
    }
    if (field[7] == '\0') {
        *kind = 'd';
        return 1;
    }
    if (field[7] == '_' && (field[8] == 'd' || field[8] == 'b') && field[9] == '\0') {
        *kind = field[8];
        return 1;
    }
    return 0;
}

// Walks the image's segments twice: for the slide (where __TEXT, which begins with the header, is, less where it asked
// to be), then for the sections of __DATA whose names are overlays'
static void ScanImage(void) {
    const struct mach_header_64 *header = &__dso_handle;
    const unsigned char *commands = (const unsigned char *)(header + 1);
    const unsigned char *p = commands;
    uintptr_t slide = 0;
    uint32_t i;

    for (i = 0; i < header->ncmds; i++) {
        const struct segment_command_64 *segment = (const struct segment_command_64 *)p;
        if (segment->cmd == LC_SEGMENT_64 && SameName(segment->segname, "__TEXT")) {
            slide = (uintptr_t)header - (uintptr_t)segment->vmaddr;
        }
        p += segment->cmdsize;
    }
    p = commands;
    for (i = 0; i < header->ncmds; i++) {
        const struct segment_command_64 *segment = (const struct segment_command_64 *)p;
        if (segment->cmd == LC_SEGMENT_64 && SameName(segment->segname, "__DATA")) {
            const struct section_64 *section = (const struct section_64 *)(segment + 1);
            for (uint32_t j = 0; j < segment->nsects; j++, section++) {
                unsigned id;
                int kind;
                if (ParseSectionName(section->sectname, &id, &kind)) {
                    AddSection(id, kind, (unsigned char *)(uintptr_t)(section->addr + slide), (size_t)section->size);
                }
            }
        }
        p += segment->cmdsize;
    }
}

unsigned port_ovsec_image_section_count(void) {
    const struct mach_header_64 *header = &__dso_handle;
    const unsigned char *p = (const unsigned char *)(header + 1);
    unsigned count = 0;

    for (uint32_t i = 0; i < header->ncmds; i++) {
        const struct segment_command_64 *segment = (const struct segment_command_64 *)p;
        if (segment->cmd == LC_SEGMENT_64) {
            count += segment->nsects;
        }
        p += segment->cmdsize;
    }
    return count;
}

#else

// ELF and COFF give each section's bounds as symbols. An empty range is no section: on COFF the markers of an overlay
// without writable statics end up next to each other
static void AddBounds(unsigned id, int kind, unsigned char *start, unsigned char *end) {
    if (start != NULL && end != NULL && end > start) {
        AddSection(id, kind, start, (size_t)(end - start));
    }
}

unsigned port_ovsec_image_section_count(void) {
    return 0;
}

#if defined(_WIN32)

// COFF: the linker sorts the parts of a grouped section by the name after '$' and drops the suffix, so .data$ovNNNa,
// .data$ovNNNm (the overlay's statics) and .data$ovNNNz come in that order inside .data. The range is everything
// between the markers of one byte each, alignment padding included. lld groups by contents as well as by name, so
// each marker has the contents of the statics it brackets: the data markers are initialized, and the bss markers are
// placed by "#pragma clang section bss", as the overlays' own zeroed statics are (gen_overlay_sections.py's forced
// includes), since a section attribute would make their sections initialized data, another .bss than the statics'
#define OVSEC_PRAGMA(text) _Pragma(#text)
#define OVSEC_BSS_MARKER(n, end, name)                                                                                 \
    OVSEC_PRAGMA(clang section bss = ".bss$ov" #n #end)                                                                \
    static unsigned char name[1];                                                                                      \
    OVSEC_PRAGMA(clang section bss = "")
#define OVSEC_OVERLAY(n, id)                                                                                           \
    __attribute__((section(".data$ov" #n "a"))) static unsigned char sDataBegin##n[1] = { 1 };                         \
    __attribute__((section(".data$ov" #n "z"))) static unsigned char sDataEnd##n[1] = { 1 };                           \
    OVSEC_BSS_MARKER(n, a, sBssBegin##n)                                                                               \
    OVSEC_BSS_MARKER(n, z, sBssEnd##n)
#include "ovsec_list.h"
#undef OVSEC_OVERLAY

static void ScanImage(void) {
#define OVSEC_OVERLAY(n, id)                                                                                           \
    AddBounds(id, 'd', sDataBegin##n + 1, sDataEnd##n);                                                                \
    AddBounds(id, 'b', sBssBegin##n + 1, sBssEnd##n);
#include "ovsec_list.h"
#undef OVSEC_OVERLAY
}

#else

// ELF: the linker defines __start_NAME and __stop_NAME for every output section whose name is a C identifier. The
// references are weak, so that an overlay whose files have no writable statics (no section) gets NULL bounds instead
// of a link error
#define OVSEC_BOUND(name) extern unsigned char name[] __attribute__((weak, visibility("hidden")));
#define OVSEC_OVERLAY(n, id)                                                                                           \
    OVSEC_BOUND(__start_ov##n##_d)                                                                                     \
    OVSEC_BOUND(__stop_ov##n##_d)                                                                                      \
    OVSEC_BOUND(__start_ov##n##_b) OVSEC_BOUND(__stop_ov##n##_b) OVSEC_BOUND(__start_ov##n) OVSEC_BOUND(__stop_ov##n)
#include "ovsec_list.h"
#undef OVSEC_OVERLAY

static void ScanImage(void) {
#define OVSEC_OVERLAY(n, id)                                                                                           \
    AddBounds(id, 'd', __start_ov##n##_d, __stop_ov##n##_d);                                                           \
    AddBounds(id, 'b', __start_ov##n##_b, __stop_ov##n##_b);                                                           \
    AddBounds(id, 'd', __start_ov##n, __stop_ov##n);
#include "ovsec_list.h"
#undef OVSEC_OVERLAY
}

#endif // _WIN32

#endif // __APPLE__

#endif // PORT_OVSEC_DISABLED

unsigned port_ovsec_init(void) {
    size_t total = 0;
    unsigned char *buffer;
    unsigned id;

    if (sInitialized) {
        return sOverlayCount;
    }
    sInitialized = 1;
    ScanImage();
    for (id = 0; id < PORT_OVSEC_MAX_OVERLAYS; id++) {
        if (sOverlays[id].data.start != NULL || sOverlays[id].bss.start != NULL) {
            sOverlayCount++;
        }
        // Each snapshot starts 16-byte aligned, like the sections
        total += (sOverlays[id].data.size + 15) & ~(size_t)15;
    }
#if defined(PORT_OVSEC_DISABLED)
    fprintf(stderr,
            "port: fs: built without overlay sections (PORT_OVSEC_DISABLED): loading an overlay doesn't set its "
            "statics back as the DS does\n");
    return 0;
#else
    if (sOverlayCount == 0) {
        fprintf(stderr, "port: fs: the program has no overlay sections, so the overlays' forced includes "
                        "(port/cmake/overlay_sections.cmake) didn't take effect\n");
        exit(1);
    }
#endif
    buffer = malloc(total != 0 ? total : 1);
    if (buffer == NULL) {
        fprintf(stderr, "port: fs: no memory for the overlays' snapshot (%zu bytes)\n", total);
        exit(1);
    }
    for (id = 0; id < PORT_OVSEC_MAX_OVERLAYS; id++) {
        PortOvsecOverlay *overlay = &sOverlays[id];
        if (overlay->data.start == NULL) {
            continue;
        }
        overlay->snapshot = buffer;
        CopyBytes(buffer, overlay->data.start, overlay->data.size);
        buffer += (overlay->data.size + 15) & ~(size_t)15;
    }
    return sOverlayCount;
}

// The section bounds aren't pointers the compiler can relate to the overlays' objects, so without the barrier it could
// keep a static's old value in a register across a reset, and LTO could inline the reset into a caller that does: the
// clobber makes every caller read memory again. noinline keeps the barrier where the copy is
__attribute__((noinline)) void port_ovsec_restore_data(unsigned id) {
    PortOvsecOverlay *overlay;

    if (id >= PORT_OVSEC_MAX_OVERLAYS) {
        return;
    }
    overlay = &sOverlays[id];
    if (overlay->snapshot != NULL) {
        CopyBytes(overlay->data.start, overlay->snapshot, overlay->data.size);
    }
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline)) void port_ovsec_clear_bss(unsigned id) {
    PortOvsecOverlay *overlay;

    if (id >= PORT_OVSEC_MAX_OVERLAYS) {
        return;
    }
    overlay = &sOverlays[id];
    if (overlay->bss.start != NULL) {
        ZeroBytes(overlay->bss.start, overlay->bss.size);
    }
    __asm__ volatile("" ::: "memory");
}

void port_ovsec_reset(unsigned id) {
    port_ovsec_clear_bss(id);
    port_ovsec_restore_data(id);
}

const PortOvsecOverlay *port_ovsec_get(unsigned id) {
    if (id >= PORT_OVSEC_MAX_OVERLAYS || (sOverlays[id].data.start == NULL && sOverlays[id].bss.start == NULL)) {
        return NULL;
    }
    return &sOverlays[id];
}

unsigned port_ovsec_overlay_count(void) {
    return sOverlayCount;
}

unsigned port_ovsec_section_count(void) {
    return sSectionCount;
}
