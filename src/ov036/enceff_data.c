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
    /* 0 */ { (EncEffCreateFunc)EventEncountEffectCutin_Create01, EncEffModel_Render, 149, 1, 0x13010 },
    /* 1 */ { (EncEffCreateFunc)EncEffModel_CreateModel9, EncEffModel_Render, 149, 1, 0x131a4 },
    /* 2 */ { (EncEffCreateFunc)EncEffRipple_CreateWhite, EncEffRipple_ClearAndDraw, 151, 1, 0x0 },
    /* 3 */ { EventEncountEffectCutin_Create25, NULL, 146, 0, 0x1e04 },
    /* 4 */ { EventEncountEffectCutin_Create26, NULL, 146, 1, 0x14bc8 },
    /* 5 */ { (EncEffCreateFunc)EncEffModel_CreateModel5, EncEffModel_Render, 149, 1, 0x12fbc },
    /* 6 */ { EventEncountEffectCutin_Create05, EncEffModel_Render, 149, 1, 0x13b20 },
    /* 7 */ { (EncEffCreateFunc)EncEffCells_Create, EncEffGrid_ClearAndDraw, 152, 0, 0x0 },
    /* 8 */ { (EncEffCreateFunc)EncEffCells_Create, EncEffGrid_ClearAndDraw, 154, 1, 0x0 },
    /* 9 */ { (EncEffCreateFunc)EncEffCells_Create, EncEffGrid_ClearAndDraw, 153, 1, 0x0 },
    /* 10 */ { EventEncountEffectCutin_Create00, NULL, 146, 0, 0xe570 },
    /* 11 */ { EventEncountEffectCutin_Create01, NULL, 146, 0, 0xe570 },
    /* 12 */ { EventEncountEffectCutin_Create02, NULL, 146, 0, 0xe570 },
    /* 13 */ { EventEncountEffectCutin_Create03, NULL, 146, 0, 0xe570 },
    /* 14 */ { EventEncountEffectCutin_Create04, NULL, 146, 0, 0xe570 },
    /* 15 */ { EventEncountEffectCutin_Create05, NULL, 146, 0, 0xe570 },
    /* 16 */ { EventEncountEffectCutin_Create06, NULL, 146, 0, 0xe570 },
    /* 17 */ { EventEncountEffectCutin_Create07, NULL, 146, 0, 0xe570 },
    /* 18 */ { EventEncountEffectCutin_Create08, NULL, 146, 0, 0xe570 },
    /* 19 */ { EventEncountEffectCutin_Create09, NULL, 146, 0, 0x12a68 },
    /* 20 */ { EventEncountEffectCutin_Create10, NULL, 146, 0, 0x12a68 },
    /* 21 */ { EventEncountEffectCutin_Create11, NULL, 146, 0, 0x12a68 },
    /* 22 */ { EventEncountEffectCutin_Create12, NULL, 146, 0, 0x12a68 },
    /* 23 */ { EventEncountEffectCutin_Create13, NULL, 146, 0, 0x12a68 },
    /* 24 */ { EventEncountEffectCutin_Create15, NULL, 146, 0, 0xe570 },
    /* 25 */ { EventEncountEffectCutin_Create14, NULL, 146, 0, 0xe570 },
    /* 26 */ { EventEncountEffectCutin_Create16, NULL, 146, 0, 0xcf20 },
    /* 27 */ { EventEncountEffectCutin_Create17, NULL, 146, 0, 0xd530 },
    /* 28 */ { EventEncountEffectCutin_Create18, NULL, 146, 0, 0xcf20 },
    /* 29 */ { EventEncountEffectCutin_Create19, NULL, 146, 0, 0xcf20 },
    /* 30 */ { EventEncountEffectCutin_Create20, NULL, 146, 0, 0xe570 },
    /* 31 */ { EventEncountEffectCutin_Create21, NULL, 146, 0, 0xe570 },
    /* 32 */ { (EncEffCreateFunc)EncEffModel_CreateModel1, EncEffModel_Render, 149, 1, 0x132e8 },
    /* 33 */ { EventEncountEffectCutin_Create22, NULL, 146, 1, 0x80e0 },
    /* 34 */ { EventEncountEffectCutin_Create23, NULL, 146, 1, 0x619c },
    /* 35 */ { EventEncountEffectCutin_Create24, NULL, 146, 1, 0xac88 },
    /* 36 */ { (EncEffCreateFunc)EncEffModel_CreateZoroark, EncEffModel_Render, 149, 1, 0x12dc0 },
    /* 37 */ { (EncEffCreateFunc)EncEffRipple_CreateBlack, EncEffRipple_ClearAndDraw, 151, 0, 0x0 },
};
