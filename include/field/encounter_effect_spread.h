#ifndef POKEBW2_FIELD_ENCOUNTER_EFFECT_SPREAD_H
#define POKEBW2_FIELD_ENCOUNTER_EFFECT_SPREAD_H

// Overlay 154: the encounter effect whose cells fly off in waves that spread out from three cells. The file's name is
// a guess; the ROM has no string for it

#include "types.h"
#include "field/encounter_effect.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The grid's work from 0x300c on. It runs 0x114 bytes past the end of the grid that overlay 150 allocates, over its
// update function and mode if the wave got long enough, and over whatever follows it on the heap
typedef struct EncEffSpread {
    // The cells shown in the last wave
    u16 wave[96];
    // The cells to show in the next wave
    u16 next[96];
    u16 waveCount;
    u16 nextCount;
} EncEffSpread;

GameEvent *func_ov154_021f6200(GameSystem *gsys, Field *field);
GameEvent *func_ov154_021f623c(GameSystem *gsys);
void func_ov154_021f626c(EncEffGrid *grid);
void func_ov154_021f62b0(EncEffGrid *grid);
BOOL func_ov154_021f6314(EncEffCell *cell);
BOOL func_ov154_021f634c(int cell, EncEffSpread *spread);
void func_ov154_021f637c(EncEffGrid *grid, int column, int row);
void func_ov154_021f63bc(EncEffGrid *grid, int cell);
void func_ov154_021f6420(EncEffGrid *grid);

#endif // POKEBW2_FIELD_ENCOUNTER_EFFECT_SPREAD_H
