/*
 * Reload semantics over many overlay sections. Every generated overlay starts pristine; Dirty() changes all its
 * statics (initialized, zeroed, function-local, a pointer with a relocation); OvSec_Reset puts them back; and
 * resetting one overlay leaves the others alone. On Mach-O the scan is also checked against ld64's
 * section$start/section$end symbols. The image's section count is printed, not checked: an image past the 255
 * that nlist's n_sect allows does not link, and check_sections.py checks the count before the link.
 */
#include <stdio.h>
#include <stdlib.h>

#include "ovsec.h"
#include "ovtest.h"

static int sFailures;

#define CHECK(cond, ...)                                      \
    do {                                                      \
        if (!(cond)) {                                        \
            sFailures++;                                      \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);       \
            printf(__VA_ARGS__);                              \
            printf("\n");                                     \
        }                                                     \
    } while (0)

static void CheckBounds(const OvTestEntry *e, const OvSecOverlay *ov)
{
#if defined(__APPLE__)
    if (e->dataStart != NULL) {
        CHECK(ov->data.start == e->dataStart && ov->data.size == (size_t)(e->dataEnd - e->dataStart),
              "overlay %u: data scan %p+%zu, ld64 %p+%td", e->id, (void *)ov->data.start, ov->data.size,
              (void *)e->dataStart, e->dataEnd - e->dataStart);
    }
    if (e->bssStart != NULL) {
        CHECK(ov->bss.start == e->bssStart && ov->bss.size == (size_t)(e->bssEnd - e->bssStart),
              "overlay %u: bss scan %p+%zu, ld64 %p+%td", e->id, (void *)ov->bss.start, ov->bss.size,
              (void *)e->bssStart, e->bssEnd - e->bssStart);
    }
#else
    (void)e;
    (void)ov;
#endif
}

int main(void)
{
    size_t need = OvSec_Scan();
    unsigned char *snapshot = malloc(need != 0 ? need : 1);
    size_t dataBytes = 0, bssBytes = 0;
    int i, round;

    OvSec_Snapshot(snapshot);
    CHECK((int)OvSec_SectionCount() == gOvTestSections, "found %u sections, generated %d", OvSec_SectionCount(),
          gOvTestSections);
    CHECK((int)OvSec_OverlayCount() == gOvTestCount, "found %u overlays, generated %d", OvSec_OverlayCount(),
          gOvTestCount);

    for (i = 0; i < gOvTestCount; i++) {
        const OvTestEntry *e = &gOvTests[i];
        const OvSecOverlay *ov = OvSec_Get(e->id);
        CHECK(ov != NULL, "overlay %u has no sections", e->id);
        if (ov == NULL) {
            continue;
        }
        dataBytes += ov->data.size;
        bssBytes += ov->bss.size;
        CheckBounds(e, ov);
        CHECK(e->pristine(), "overlay %u is not pristine at startup", e->id);
    }

    /* Dirty every overlay, then reset only the even ones: the odd ones must stay dirty. */
    for (i = 0; i < gOvTestCount; i++) {
        gOvTests[i].dirty();
        CHECK(!gOvTests[i].pristine(), "overlay %u is pristine after Dirty", gOvTests[i].id);
    }
    for (i = 0; i < gOvTestCount; i += 2) {
        OvSec_Reset(gOvTests[i].id);
    }
    for (i = 0; i < gOvTestCount; i++) {
        CHECK(gOvTests[i].pristine() == (i % 2 == 0), "overlay %u: pristine=%d after resetting the even ones",
              gOvTests[i].id, gOvTests[i].pristine());
    }

    /* Load, use and reload each overlay a few times, as the field and the menus do. */
    for (round = 0; round < 3; round++) {
        for (i = 0; i < gOvTestCount; i++) {
            OvSec_Reset(gOvTests[i].id);
            CHECK(gOvTests[i].pristine(), "overlay %u is not pristine after reset (round %d)", gOvTests[i].id,
                  round);
            gOvTests[i].dirty();
        }
    }

    /* An overlay without sections is a no-op, as is an ID out of range. */
    OvSec_Reset(999);
    OvSec_Reset(100000);

    printf("overlays %d, overlay sections %u, image sections %u (others %u), data %zu bytes, bss %zu bytes, "
           "snapshot %zu bytes: %s\n",
           gOvTestCount, OvSec_SectionCount(), OvSec_ImageSectionCount(),
           OvSec_ImageSectionCount() - OvSec_SectionCount(), dataBytes, bssBytes, need,
           sFailures != 0 ? "FAILED" : "ok");
    free(snapshot);
    return sFailures != 0;
}
