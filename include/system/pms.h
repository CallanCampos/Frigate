#ifndef POKEBW2_SYSTEM_PMS_H
#define POKEBW2_SYSTEM_PMS_H

// The phrases made of a sentence and words, and the parameter of overlay 185's phrase select, of ARM9 main

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/pms_data.h"
#include "system/pmsi_param.h"



// The C-Gear's phrases, which the save keeps
void *getCGearDataBlkAddress(SaveControl *save);
void func_0200ef48(PMSData *phrases);
void func_0200ef90(void *cgear, u32 index, PMSData *sentence);
void func_0200efa8(void *cgear, u32 index, const PMSData *sentence);
// The same block, through the C-Gear's own accessor
void *func_0200ef7c(SaveControl *save);

// Overlay 185, the phrase select
extern const GameProcFunctions PMS_INPUT_PROC_FUNCTIONS;

#endif // POKEBW2_SYSTEM_PMS_H
