#ifndef POKEBW2_SAVE_SAVE_TABLE_H
#define POKEBW2_SAVE_SAVE_TABLE_H

#include "types.h"
#include "gfl/backup_system.h"
#include "save/save_control.h"

// The saves that save_control.c loads (save_table.c, a guessed name): the main save, with its backup copy, and the
// extra saves that follow it on the card

extern const SaveDataParams save_block_data;
extern const SaveDataParams sv_extra_blk_data[SAVE_EXTRA_ID_MAX];

#endif // POKEBW2_SAVE_SAVE_TABLE_H
