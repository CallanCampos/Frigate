/*
 * Restore cost with the DS's own sizes: the generated overlays have each real overlay's .data and .bss sizes
 * (168 overlays, 87.8 KB of data and 221 KB of bss on Black 2). Reports the one-time scan and snapshot, then three
 * costs per reset, for the largest overlay and for all of them once:
 * - first load: the first reset after the snapshot, which also takes the page faults of first touch (zero-fill
 *   pages for bss, copy-on-write for data pages that dyld didn't write). This is what a scene's first load costs.
 * - later load: the pages are mapped, but other work has evicted the overlay and the snapshot from the caches.
 *   This is what reloading an overlay at a scene transition costs.
 * - warm: the same overlay reset back to back, 2,000 times. A lower bound, not what a load costs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ovsec.h"
#include "ovtest.h"

#if defined(CLOCK_MONOTONIC_RAW)
#define BENCH_CLOCK CLOCK_MONOTONIC_RAW
#else
#define BENCH_CLOCK CLOCK_MONOTONIC
#endif

static double NowNs(void)
{
    struct timespec ts;
    clock_gettime(BENCH_CLOCK, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* Where EvictCaches leaves its sum, so that the compiler keeps the reads. */
volatile unsigned gBenchSink;

/* Writes and reads 64 MB, four times the M2's largest cache, so that no overlay or snapshot byte stays cached. */
static void EvictCaches(void)
{
    enum { SIZE = 64 << 20 };
    unsigned char *buffer = malloc(SIZE);
    unsigned sum = 0;
    size_t i;

    for (i = 0; i < SIZE; i++) {
        buffer[i] = (unsigned char)i;
    }
    for (i = 0; i < SIZE; i += 64) {
        sum += buffer[i];
    }
    gBenchSink = sum;
    free(buffer);
}

/* Resets every overlay but skip once, in ID order (OVSEC_MAX_OVERLAYS skips none); returns the time taken. */
static double ResetAll(unsigned skip)
{
    double start = NowNs();
    int i;

    for (i = 0; i < gOvTestCount; i++) {
        if (gOvTests[i].id != skip) {
            OvSec_Reset(gOvTests[i].id);
        }
    }
    return NowNs() - start;
}

static double ResetOne(unsigned id)
{
    double start = NowNs();

    OvSec_Reset(id);
    return NowNs() - start;
}

typedef struct Sample {
    unsigned id;
    size_t bytes;
    double ns;
} Sample;

static int CompareNs(const void *a, const void *b)
{
    double x = ((const Sample *)a)->ns, y = ((const Sample *)b)->ns;
    return (x > y) - (x < y);
}

int main(void)
{
    enum { ITERATIONS = 2000 };
    Sample *samples = calloc((size_t)gOvTestCount, sizeof(Sample));
    double t0, t1, t2, total = 0, firstOne, firstAll, laterOne, laterAll;
    size_t need, bytes = 0;
    unsigned char *snapshot;
    int i, n = 0, k;
    Sample largest = { 0, 0, 0 };

    t0 = NowNs();
    need = OvSec_Scan();
    t1 = NowNs();
    snapshot = malloc(need);
    OvSec_Snapshot(snapshot);
    t2 = NowNs();
    printf("scan %.1f us, snapshot %.1f us (%zu bytes) for %u overlays, %u sections\n", (t1 - t0) / 1e3,
           (t2 - t1) / 1e3, need, OvSec_OverlayCount(), OvSec_SectionCount());

    for (i = 0; i < gOvTestCount; i++) {
        const OvSecOverlay *ov = OvSec_Get(gOvTests[i].id);
        if (ov != NULL && ov->data.size + ov->bss.size > largest.bytes) {
            largest.id = gOvTests[i].id;
            largest.bytes = ov->data.size + ov->bss.size;
        }
    }

    /* First load: the largest overlay alone, then the others. */
    firstOne = ResetOne(largest.id);
    firstAll = firstOne + ResetAll(largest.id);
    printf("first load (with page faults): overlay %u (%zu bytes) %.1f us; all %d overlays %.1f us\n", largest.id,
           largest.bytes, firstOne / 1e3, gOvTestCount, firstAll / 1e3);

    /* Later load: caches evicted before each measurement. */
    EvictCaches();
    laterOne = ResetOne(largest.id);
    EvictCaches();
    laterAll = ResetAll(OVSEC_MAX_OVERLAYS);
    printf("later load (caches evicted): overlay %u %.1f us; all %d overlays %.1f us\n", largest.id, laterOne / 1e3,
           gOvTestCount, laterAll / 1e3);

    /* Warm: each overlay reset back to back. */
    for (i = 0; i < gOvTestCount; i++) {
        const OvSecOverlay *ov = OvSec_Get(gOvTests[i].id);
        double start;
        if (ov == NULL) {
            continue;
        }
        start = NowNs();
        for (k = 0; k < ITERATIONS; k++) {
            OvSec_Reset(gOvTests[i].id);
        }
        samples[n].id = gOvTests[i].id;
        samples[n].bytes = ov->data.size + ov->bss.size;
        samples[n].ns = (NowNs() - start) / ITERATIONS;
        total += samples[n].ns;
        bytes += samples[n].bytes;
        if (samples[n].id == largest.id) {
            largest.ns = samples[n].ns;
        }
        n++;
    }
    qsort(samples, (size_t)n, sizeof(Sample), CompareNs);
    printf("warm (%d resets in a row): overlay %u %.0f ns; median %.0f ns; slowest %.0f ns (overlay %u); "
           "all %d once %.1f us for %zu bytes (%.1f GB/s)\n",
           ITERATIONS, largest.id, largest.ns, samples[n / 2].ns, samples[n - 1].ns, samples[n - 1].id, n,
           total / 1e3, bytes, (double)bytes / total);
    free(samples);
    free(snapshot);
    return 0;
}
