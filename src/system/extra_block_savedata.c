#include "types.h"
#include "save/extra_block_savedata.h"
#include "gfl/std.h"
#include "save/wbt_save.h"

// Two extra save blocks that are only cleared here: one of 0xc8 bytes, and the three of a downloaded tournament of
// the Pokémon World Tournament, with the check of its checksum. The ROM has no string for the file, so the name is
// descriptive.

u32 func_020103f4(void) {
    return 0xc8;
}

void func_020103f8(void *block) {
    sys_memset(block, 0, func_020103f4());
}

u32 func_0201040c(void) {
    return sizeof(WbtDownloadData);
}

void func_02010414(void *block) {
    sys_memset(block, 0, func_0201040c());
}

BOOL func_02010428(void *data) {
    WbtDownloadData *download = data;
    u16 crc = download->crc;

    if (crc == getCRC16(download, sizeof(download->data))) {
        return TRUE;
    }
    return FALSE;
}
