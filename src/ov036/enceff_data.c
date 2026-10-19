// The table of overlay 36's encounter effects, which EncEff_StartEvent (enceff.c) reads. The file name is a guess. The
// ROM has the table ahead of enceff.c's own two tables, and no layout of one file gives that: a table of 64 bytes or
// more gets a section of its own after the shared one, whether static or global, declared before or after them. It
// is no end of comm_entry_menu.c, which nothing of this table uses, so it is a file of its own. Table name from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/encounter_effect.h"
#include "field/encounter_effect_model.h"
#include "field/encounter_effect_ripple.h"
#include "field/event_encounter_cutin.h"

const EncEffInfo ENC_EFFECT_CONTROLLERS[ENCEFF_COUNT] = {
    /* 0 */ { (EncEffCreateFunc)func_ov146_021f59e0, EncEffModel_Render, 149, 1, 0x13010 },
    /* 1 */ { (EncEffCreateFunc)func_ov149_021f59f0, EncEffModel_Render, 149, 1, 0x131a4 },
    /* 2 */ { (EncEffCreateFunc)EncEffRipple_CreateWhite, EncEffRipple_ClearAndDraw, 151, 1, 0x0 },
    /* 3 */ { func_ov146_021f5b0c, NULL, 146, 0, 0x1e04 },
    /* 4 */ { func_ov146_021f5b18, NULL, 146, 1, 0x14bc8 },
    /* 5 */ { (EncEffCreateFunc)func_ov149_021f5a00, EncEffModel_Render, 149, 1, 0x12fbc },
    /* 6 */ { func_ov146_021f5a10, EncEffModel_Render, 149, 1, 0x13b20 },
    /* 7 */ { (EncEffCreateFunc)func_ov152_021f6200, EncEffGrid_ClearAndDraw, 152, 0, 0x0 },
    /* 8 */ { (EncEffCreateFunc)func_ov152_021f6200, EncEffGrid_ClearAndDraw, 154, 1, 0x0 },
    /* 9 */ { (EncEffCreateFunc)func_ov152_021f6200, EncEffGrid_ClearAndDraw, 153, 1, 0x0 },
    /* 10 */ { func_ov146_021f5a70, NULL, 146, 0, 0xe570 },
    /* 11 */ { func_ov146_021f59e0, NULL, 146, 0, 0xe570 },
    /* 12 */ { func_ov146_021f59ec, NULL, 146, 0, 0xe570 },
    /* 13 */ { func_ov146_021f59f8, NULL, 146, 0, 0xe570 },
    /* 14 */ { func_ov146_021f5a04, NULL, 146, 0, 0xe570 },
    /* 15 */ { func_ov146_021f5a10, NULL, 146, 0, 0xe570 },
    /* 16 */ { func_ov146_021f5a1c, NULL, 146, 0, 0xe570 },
    /* 17 */ { func_ov146_021f5a28, NULL, 146, 0, 0xe570 },
    /* 18 */ { func_ov146_021f5a34, NULL, 146, 0, 0xe570 },
    /* 19 */ { func_ov146_021f5a40, NULL, 146, 0, 0x12a68 },
    /* 20 */ { func_ov146_021f5a4c, NULL, 146, 0, 0x12a68 },
    /* 21 */ { func_ov146_021f5a58, NULL, 146, 0, 0x12a68 },
    /* 22 */ { func_ov146_021f5a64, NULL, 146, 0, 0x12a68 },
    /* 23 */ { func_ov146_021f5aa0, NULL, 146, 0, 0x12a68 },
    /* 24 */ { func_ov146_021f5a7c, NULL, 146, 0, 0xe570 },
    /* 25 */ { func_ov146_021f5a88, NULL, 146, 0, 0xe570 },
    /* 26 */ { func_ov146_021f5a94, NULL, 146, 0, 0xcf20 },
    /* 27 */ { func_ov146_021f5aac, NULL, 146, 0, 0xd530 },
    /* 28 */ { func_ov146_021f5ab8, NULL, 146, 0, 0xcf20 },
    /* 29 */ { func_ov146_021f5ac4, NULL, 146, 0, 0xcf20 },
    /* 30 */ { func_ov146_021f5ad0, NULL, 146, 0, 0xe570 },
    /* 31 */ { func_ov146_021f5adc, NULL, 146, 0, 0xe570 },
    /* 32 */ { (EncEffCreateFunc)func_ov149_021f5a20, EncEffModel_Render, 149, 1, 0x132e8 },
    /* 33 */ { func_ov146_021f5ae8, NULL, 146, 1, 0x80e0 },
    /* 34 */ { func_ov146_021f5af4, NULL, 146, 1, 0x619c },
    /* 35 */ { func_ov146_021f5b00, NULL, 146, 1, 0xac88 },
    /* 36 */ { (EncEffCreateFunc)EncEffModel_CreateZoroark, EncEffModel_Render, 149, 1, 0x12dc0 },
    /* 37 */ { (EncEffCreateFunc)EncEffRipple_CreateBlack, EncEffRipple_ClearAndDraw, 151, 0, 0x0 },
};
