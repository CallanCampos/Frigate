#ifndef POKEBW2_FIELD_FLD_EXP_OBJ_H
#define POKEBW2_FIELD_FLD_EXP_OBJ_H

// The .rodata of overlay 36's fld_exp_obj.c, which only field_transition.c refers to. The file's functions are declared
// in field/field_exp_obj.h

#include "types.h"
#include "gfl/bg_sys.h"

// The screen of the building entrance's effect, which shows the captured screen on the main screen's BG 2 in mode 5
extern const BGSysLCDConfig gEnterBuildingLCDConfig;

#endif // POKEBW2_FIELD_FLD_EXP_OBJ_H
