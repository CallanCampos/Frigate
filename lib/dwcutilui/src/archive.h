#ifndef POKEBW2_DWCUTIL_ARCHIVE_H
#define POKEBW2_DWCUTIL_ARCHIVE_H

#include "types.h"

// The connection utility's resources: rom:/dwc/utility.bin holds a file system of its own, which is mounted as the
// archive "dwc" and made the current directory. A file read from it is decompressed when its name ends in ".l".
// The file is the DWC utility library's archive.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilRomResource_Init(void);
void DWCi_ARClEnd(void);
// Reads the file into a new block aligned to align, and its size into size if that isn't NULL
void *DWCUtilRomResource_LoadFile(const char *path, u32 *size, int align);
void DWCi_ARClRelease(void *data);
// Whether the last length characters of the two strings are the same
BOOL cmpBack(const char *str, const char *suffix, int length);

#endif // POKEBW2_DWCUTIL_ARCHIVE_H
