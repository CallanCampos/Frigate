// Calls each missing GameSpy bucket-set function through its actual SDK declaration (dwc/gbucket.h).
// check_gbucket.py starts a separate process for each call and checks its named, not-in-ROM exit diagnostic.
#include "dwc/gbucket.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "NewBucketSet") == 0) {
        (void)NewBucketSet();
    } else if (strcmp(argv[1], "FreeBucketSet") == 0) {
        FreeBucketSet(NULL);
    } else if (strcmp(argv[1], "DumpBucketSet") == 0) {
        (void)DumpBucketSet(NULL);
    } else {
        return 2;
    }
    fputs("missing bucket-set function returned\n", stderr);
    return 1;
}
