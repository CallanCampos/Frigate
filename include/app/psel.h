#ifndef POKEBW2_APP_PSEL_H
#define POKEBW2_APP_PSEL_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The choice of a starter Pokémon, overlay 316 (psel.c)
#define OVERLAY_PSEL OVERLAY_ID(316)

// The screen's heap and its archive of graphics. Our names, not swan's
#define HEAPID_PSEL 0x87
#define ARCID_PSEL 202

// What the screen is called with. It writes the chosen Pokémon to *result: 0 for Snivy, 1 for Tepig and 2 for
// Oshawott, and to choice as well
typedef struct {
    u16 *result;
    s32 choice;
} PselParam;

extern const GameProcFunctions PselProcFunctions;

#endif // POKEBW2_APP_PSEL_H
