// GameSpy's three bucket-set functions declared in lib/dwc/include/dwc/gbucket.h but absent from the ROM and its C.
// The imported gstats.c references them from code that the DS linker discards. Host linkers can retain those
// references, so provide named traps, never replacement bucket behavior or fabricated DS addresses. Calling even
// FreeBucketSet(NULL) means execution reached code with no ROM implementation and must stop visibly.
#include "dwc/gbucket.h"
#include "hal/trap.h"

#include <stdio.h>
#include <stdlib.h>

static _Noreturn void missing_bucket_function(const char *name) {
    fprintf(stderr, "port: %s: not in ROM (GameSpy bucket-set function)\n", name);
    fflush(stderr);
    _Exit(PORT_TRAP_EXIT_STATUS);
}

bucketset_t NewBucketSet(void) {
    missing_bucket_function("NewBucketSet");
}

void FreeBucketSet(bucketset_t set) {
    (void)set;
    missing_bucket_function("FreeBucketSet");
}

char *DumpBucketSet(bucketset_t set) {
    (void)set;
    missing_bucket_function("DumpBucketSet");
}
