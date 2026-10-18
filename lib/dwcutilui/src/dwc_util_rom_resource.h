#ifndef POKEBW2_DWCUTIL_DWC_UTIL_ROM_RESOURCE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_ROM_RESOURCE_H

#include "types.h"

// The connection utility's resources: rom:/dwc/utility.bin holds a file system of its own, which is mounted as the
// archive "dwc" and made the current directory. A file read from it is decompressed when its name ends in ".l".
// The ROM does not embed this source file's name; dwc_util_rom_resource.c is descriptive.
void func_ov182_021bb18c(void);
void func_ov182_021bb2a4(void);
// Reads the file into a new block aligned to align, and its size into size if that isn't NULL
void *func_ov182_021bb37c(const char *path, u32 *size, int align);
void func_ov182_021bb420(void *data);
// Whether the last length characters of the two strings are the same
BOOL func_ov182_021bb444(const char *str, const char *suffix, int length);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_ROM_RESOURCE_H
