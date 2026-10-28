// Native cached-table reads for NitroSDK fs_command_default.c, used by SystemInit's ROM archive and DWC utility
// archives. Positions remain SDK archive offsets; only FSArchiveTables.load_mem carries the caller's native pointer.
#ifndef PORT_FS_TABLES_H
#define PORT_FS_TABLES_H

#include "nitro/fs.h"

FSResult FSi_HostReadArchiveTable(FSArchive *arc, void *buffer, u32 position, u32 length);

#endif
