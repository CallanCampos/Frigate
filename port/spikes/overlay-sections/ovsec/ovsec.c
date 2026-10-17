/*
 * Finds each overlay's data and bss sections, snapshots the data at startup, and resets an overlay the way the DS
 * reloads it. See ovsec.h for the design. The Mach-O path is the tested one; the ELF and COFF paths are compiled
 * by the spike's cross check but not linked or run.
 *
 * This file uses no libc, so that the cross check can compile it freestanding for other targets.
 */
#include "ovsec.h"

#include <stdint.h>

#if defined(__APPLE__)
#include <mach-o/loader.h>
/* The Mach-O header of the image this file is linked into, defined by the static linker. Using it rather than
 * _dyld_get_image_header(0) keeps the scan right if the game is ever linked into a dynamic library. */
extern const struct mach_header_64 __dso_handle;
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define OVSEC_ASAN 1
#endif
#endif
#if !defined(OVSEC_ASAN) && defined(__SANITIZE_ADDRESS__)
#define OVSEC_ASAN 1
#endif

static OvSecOverlay sOverlays[OVSEC_MAX_OVERLAYS];
static unsigned sOverlayCount;
static unsigned sSectionCount;

#if defined(OVSEC_ASAN)
/* ASan pads every global with a poisoned redzone, and the redzones lie inside the overlay sections, so memcpy and
 * memset over a section report a global-buffer-overflow. They do even when called from an unsanitized function,
 * because ASan intercepts the libc functions themselves. A loop in an unsanitized function, which the compiler may
 * not turn back into a library call, copies the redzones too; that changes nothing, since their shadow stays
 * poisoned and instrumented code still reports any access to them. */
__attribute__((no_sanitize("address"), no_builtin("memcpy", "memset"), noinline))
static void CopyBytes(unsigned char *dst, const unsigned char *src, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
}

__attribute__((no_sanitize("address"), no_builtin("memcpy", "memset"), noinline))
static void ZeroBytes(unsigned char *dst, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        dst[i] = 0;
    }
}
#else
static void CopyBytes(unsigned char *dst, const unsigned char *src, size_t size)
{
    __builtin_memcpy(dst, src, size);
}

static void ZeroBytes(unsigned char *dst, size_t size)
{
    __builtin_memset(dst, 0, size);
}
#endif

#if defined(OVSEC_DISABLED)

/* A build by a compiler without "#pragma clang section" (GCC, as an extra check) has no overlay sections, and the
 * forced includes refuse to compile without this define. The scan finds nothing and resets do nothing. */
static void ScanImage(void)
{
}

unsigned OvSec_ImageSectionCount(void)
{
    return 0;
}

#else

/* Records one section of overlay id. Kind 'd' is data (restored from the snapshot), 'b' is bss (zeroed). A
 * section that the linker did not create (a weak bound that resolved to NULL) is ignored. */
static void AddSection(unsigned id, int kind, unsigned char *start, size_t size)
{
    OvSecRange *range;

    if (id >= OVSEC_MAX_OVERLAYS || start == NULL) {
        return;
    }
    range = kind == 'b' ? &sOverlays[id].bss : &sOverlays[id].data;
    range->start = start;
    range->size = size;
    sSectionCount++;
}

#if defined(__APPLE__)

/* Compares a fixed-size Mach-O name field (not terminated when all 16 bytes are used) with a C string. */
static int SameName(const char field[16], const char *name)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (field[i] != name[i]) {
            return 0;
        }
        if (name[i] == '\0') {
            return 1;
        }
    }
    return name[16] == '\0';
}

/* Parses "__ovNNN_d", "__ovNNN_b" or "__ovNNN" (the combined layout, which is data). */
static int ParseSectionName(const char field[16], unsigned *id, int *kind)
{
    int i;

    if (field[0] != '_' || field[1] != '_' || field[2] != 'o' || field[3] != 'v') {
        return 0;
    }
    *id = 0;
    for (i = 4; i < 7; i++) {
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

/* Walks the image's segments twice: once for the slide (where __TEXT, which starts with the header, was loaded
 * minus where it asked to be), then for the __DATA sections whose names are overlay sections. */
static void ScanImage(void)
{
    const struct mach_header_64 *header = &__dso_handle;
    const unsigned char *cmds = (const unsigned char *)(header + 1);
    const unsigned char *p;
    uintptr_t slide = 0;
    uint32_t i, j;

    p = cmds;
    for (i = 0; i < header->ncmds; i++) {
        const struct segment_command_64 *seg = (const struct segment_command_64 *)p;
        if (seg->cmd == LC_SEGMENT_64 && SameName(seg->segname, "__TEXT")) {
            slide = (uintptr_t)header - (uintptr_t)seg->vmaddr;
        }
        p += seg->cmdsize;
    }
    p = cmds;
    for (i = 0; i < header->ncmds; i++) {
        const struct segment_command_64 *seg = (const struct segment_command_64 *)p;
        if (seg->cmd == LC_SEGMENT_64 && SameName(seg->segname, "__DATA")) {
            const struct section_64 *sect = (const struct section_64 *)(seg + 1);
            for (j = 0; j < seg->nsects; j++, sect++) {
                unsigned id;
                int kind;
                if (ParseSectionName(sect->sectname, &id, &kind)) {
                    AddSection(id, kind, (unsigned char *)(uintptr_t)(sect->addr + slide), (size_t)sect->size);
                }
            }
        }
        p += seg->cmdsize;
    }
}

unsigned OvSec_ImageSectionCount(void)
{
    const struct mach_header_64 *header = &__dso_handle;
    const unsigned char *p = (const unsigned char *)(header + 1);
    unsigned count = 0;
    uint32_t i;

    for (i = 0; i < header->ncmds; i++) {
        const struct segment_command_64 *seg = (const struct segment_command_64 *)p;
        if (seg->cmd == LC_SEGMENT_64) {
            count += seg->nsects;
        }
        p += seg->cmdsize;
    }
    return count;
}

#else

/* ELF and COFF get each section's bounds from symbols. An empty range is no section: on COFF the markers of an
 * overlay without writable statics end up next to each other. */
static void AddBounds(unsigned id, int kind, unsigned char *start, unsigned char *end)
{
    if (start != NULL && end != NULL && end > start) {
        AddSection(id, kind, start, (size_t)(end - start));
    }
}

unsigned OvSec_ImageSectionCount(void)
{
    return 0;
}

#if defined(_WIN32)

/* COFF: the linker sorts the contributions to a grouped section by the name after '$' and drops the suffix, so
 * .data$ovNNNa < .data$ovNNNm (the overlay's objects) < .data$ovNNNz inside .data. The markers are one byte each;
 * the range is everything after the begin marker up to the end marker, including any alignment padding. */
#define OVSEC_OVERLAY(n, id)                                                                        \
    __attribute__((section(".data$ov" #n "a"))) static unsigned char sDataBegin##n[1] = { 1 };      \
    __attribute__((section(".data$ov" #n "z"))) static unsigned char sDataEnd##n[1] = { 1 };        \
    __attribute__((section(".bss$ov" #n "a"))) static unsigned char sBssBegin##n[1];                \
    __attribute__((section(".bss$ov" #n "z"))) static unsigned char sBssEnd##n[1];
#include "ovsec_list.h"
#undef OVSEC_OVERLAY

static void ScanImage(void)
{
#define OVSEC_OVERLAY(n, id)                                \
    AddBounds(id, 'd', sDataBegin##n + 1, sDataEnd##n);     \
    AddBounds(id, 'b', sBssBegin##n + 1, sBssEnd##n);
#include "ovsec_list.h"
#undef OVSEC_OVERLAY
}

#else

/* ELF: the linker defines __start_SECNAME and __stop_SECNAME for every output section whose name is a C
 * identifier. The references are weak, so an overlay whose files have no writable statics (no section) gets
 * NULL bounds instead of a link error. */
#define OVSEC_BOUND(name) extern unsigned char name[] __attribute__((weak, visibility("hidden")));
#define OVSEC_OVERLAY(n, id)                                                                        \
    OVSEC_BOUND(__start_ov##n##_d) OVSEC_BOUND(__stop_ov##n##_d)                                    \
    OVSEC_BOUND(__start_ov##n##_b) OVSEC_BOUND(__stop_ov##n##_b)                                    \
    OVSEC_BOUND(__start_ov##n) OVSEC_BOUND(__stop_ov##n)
#include "ovsec_list.h"
#undef OVSEC_OVERLAY

static void ScanImage(void)
{
#define OVSEC_OVERLAY(n, id)                                        \
    AddBounds(id, 'd', __start_ov##n##_d, __stop_ov##n##_d);        \
    AddBounds(id, 'b', __start_ov##n##_b, __stop_ov##n##_b);        \
    AddBounds(id, 'd', __start_ov##n, __stop_ov##n);
#include "ovsec_list.h"
#undef OVSEC_OVERLAY
}

#endif /* _WIN32 */

#endif /* __APPLE__ */

#endif /* OVSEC_DISABLED */

size_t OvSec_Scan(void)
{
    size_t total = 0;
    unsigned id;

    for (id = 0; id < OVSEC_MAX_OVERLAYS; id++) {
        sOverlays[id] = (OvSecOverlay){ 0 };
    }
    sOverlayCount = 0;
    sSectionCount = 0;
    ScanImage();
    for (id = 0; id < OVSEC_MAX_OVERLAYS; id++) {
        if (sOverlays[id].data.start != NULL || sOverlays[id].bss.start != NULL) {
            sOverlayCount++;
        }
        /* Each snapshot starts 16-byte aligned, like the sections themselves. */
        total += (sOverlays[id].data.size + 15) & ~(size_t)15;
    }
    return total;
}

void OvSec_Snapshot(unsigned char *buffer)
{
    unsigned id;

    for (id = 0; id < OVSEC_MAX_OVERLAYS; id++) {
        OvSecOverlay *ov = &sOverlays[id];
        if (ov->data.start == NULL) {
            continue;
        }
        ov->snapshot = buffer;
        CopyBytes(buffer, ov->data.start, ov->data.size);
        buffer += (ov->data.size + 15) & ~(size_t)15;
    }
}

__attribute__((noinline)) void OvSec_Reset(unsigned id)
{
    OvSecOverlay *ov;

    if (id >= OVSEC_MAX_OVERLAYS) {
        return;
    }
    ov = &sOverlays[id];
    if (ov->snapshot != NULL) {
        CopyBytes(ov->data.start, ov->snapshot, ov->data.size);
    }
    if (ov->bss.start != NULL) {
        ZeroBytes(ov->bss.start, ov->bss.size);
    }
    /* The section bounds are not pointers the compiler can relate to the overlay's objects, so without this it
     * could keep a static's old value in a register across the reset, and LTO could inline the reset into a caller
     * that does. The clobber makes every caller reload memory. */
    __asm__ volatile("" ::: "memory");
}

const OvSecOverlay *OvSec_Get(unsigned id)
{
    if (id >= OVSEC_MAX_OVERLAYS || (sOverlays[id].data.start == NULL && sOverlays[id].bss.start == NULL)) {
        return NULL;
    }
    return &sOverlays[id];
}

unsigned OvSec_OverlayCount(void)
{
    return sOverlayCount;
}

unsigned OvSec_SectionCount(void)
{
    return sSectionCount;
}
