#ifndef POKEBW2_SAVE_DOWNLOAD_DATA_H
#define POKEBW2_SAVE_DOWNLOAD_DATA_H

// The data the save keeps at 0x68 of SaveControl, which looks like what the game downloads: entries of 0x18 bytes with
// an 8-character name, flags and values, and the musical program and props, of ARM9 main. The name is descriptive

#include "types.h"
#include "struct_decls.h"
#include "save/memory_link_savedata.h"

void *func_020074d8(SaveControl *save);

#endif // POKEBW2_SAVE_DOWNLOAD_DATA_H
