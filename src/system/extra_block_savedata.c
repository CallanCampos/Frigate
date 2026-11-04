#include "types.h"
#include "gfl/std.h"
#include "save/extra_block_savedata.h"
#include "save/wbt_save.h"

// Two extra save blocks that are only cleared here: one of 0xc8 bytes, and the three of a downloaded tournament of
// the Pokémon World Tournament, with the check of its checksum. The ROM has no string for the file, so the name is
// descriptive.

u32 ExtraBlock20_GetSize(void) {
    return 0xc8;
}

void ExtraBlock20_Clear(void *block) {
    MI_CpuFill8(block, 0, ExtraBlock20_GetSize());
}

u32 WbtDownloadData_GetSize(void) {
    return sizeof(WbtDownloadData);
}

void WbtDownloadData_Clear(void *block) {
    MI_CpuFill8(block, 0, WbtDownloadData_GetSize());
}

BOOL WbtDownloadData_IsCRCValid(WbtDownloadData *download) {
    u16 crc = download->crc;

    if (crc == getCRC16(download, sizeof(download->data))) {
        return TRUE;
    }
    return FALSE;
}
