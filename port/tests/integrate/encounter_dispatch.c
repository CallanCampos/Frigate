// Overlay 36's encounter-effect table (src/ov036/enceff_data.c), used by EncEff_StartEvent (enceff.c) to create
// the model, ripple, grid and trainer cut-in effects before a battle. Compile the actual table with SDK-header-typed
// stand-ins for its constructors: all 38 generic calls must reach the intended constructor with the same object
// pointers, the table's parameter where used, and its return value. UBSan checks every call, with no ignorelist;
// LLVM's function check catches the old BOOL/u32 and two/three-argument casts (R14). Apple clang still checks
// forwarding.
#include "field/encounter_effect.h"
#include "field/encounter_effect_model.h"
#include "field/encounter_effect_ripple.h"
#include "field/encounter_effect_spread.h"
#include "field/event_encounter_cutin.h"
#include <stdio.h>
#include <stdlib.h>

static int sExpected;
static int sSeen;
static uptr sSystemToken;
static uptr sFieldToken;
static uptr sResults[ENCEFF_COUNT];

static GameEvent *Check(int id, GameSystem *gsys, Field *field, u32 param) {
    if (id != sExpected || gsys != (GameSystem *)&sSystemToken || field != (Field *)&sFieldToken ||
        param != ENC_EFFECT_CONTROLLERS[id].param) {
        fputs("encounter callback changed its target or arguments\n", stderr);
        exit(1);
    }
    sSeen++;
    return (GameEvent *)&sResults[id];
}

#define CREATOR_U32(name, id)                                                                                          \
    GameEvent *name(GameSystem *gsys, Field *field, u32 param) {                                                       \
        return Check(id, gsys, field, param);                                                                          \
    }
#define CREATOR_BOOL(name, id)                                                                                         \
    GameEvent *name(GameSystem *gsys, Field *field, BOOL param) {                                                      \
        return Check(id, gsys, field, (u32)param);                                                                     \
    }
#define CREATOR_TWO(name, id)                                                                                          \
    GameEvent *name(GameSystem *gsys, Field *field) {                                                                  \
        return Check(id, gsys, field, ENC_EFFECT_CONTROLLERS[id].param);                                               \
    }

CREATOR_BOOL(EncEffModel_CreateModel7, 0)
CREATOR_BOOL(EncEffModel_CreateModel9, 1)
CREATOR_TWO(EncEffRipple_CreateWhite, 2)
CREATOR_U32(EventEncountEffectCutin_Create25, 3)
CREATOR_U32(EventEncountEffectCutin_Create26, 4)
CREATOR_BOOL(EncEffModel_CreateModel5, 5)
CREATOR_BOOL(EncEffModel_CreateModel3, 6)
CREATOR_TWO(EncEffCells_Create, 7)
CREATOR_TWO(EncEffSpread_Create, 8)
CREATOR_TWO(EncEffColumns_Create, 9)
CREATOR_U32(EventEncountEffectCutin_Create00, 10)
CREATOR_U32(EventEncountEffectCutin_Create01, 11)
CREATOR_U32(EventEncountEffectCutin_Create02, 12)
CREATOR_U32(EventEncountEffectCutin_Create03, 13)
CREATOR_U32(EventEncountEffectCutin_Create04, 14)
CREATOR_U32(EventEncountEffectCutin_Create05, 15)
CREATOR_U32(EventEncountEffectCutin_Create06, 16)
CREATOR_U32(EventEncountEffectCutin_Create07, 17)
CREATOR_U32(EventEncountEffectCutin_Create08, 18)
CREATOR_U32(EventEncountEffectCutin_Create09, 19)
CREATOR_U32(EventEncountEffectCutin_Create10, 20)
CREATOR_U32(EventEncountEffectCutin_Create11, 21)
CREATOR_U32(EventEncountEffectCutin_Create12, 22)
CREATOR_U32(EventEncountEffectCutin_Create13, 23)
CREATOR_U32(EventEncountEffectCutin_Create15, 24)
CREATOR_U32(EventEncountEffectCutin_Create14, 25)
CREATOR_U32(EventEncountEffectCutin_Create16, 26)
CREATOR_U32(EventEncountEffectCutin_Create17, 27)
CREATOR_U32(EventEncountEffectCutin_Create18, 28)
CREATOR_U32(EventEncountEffectCutin_Create19, 29)
CREATOR_U32(EventEncountEffectCutin_Create20, 30)
CREATOR_U32(EventEncountEffectCutin_Create21, 31)
CREATOR_BOOL(EncEffModel_CreateModel1, 32)
CREATOR_U32(EventEncountEffectCutin_Create22, 33)
CREATOR_U32(EventEncountEffectCutin_Create23, 34)
CREATOR_U32(EventEncountEffectCutin_Create24, 35)
CREATOR_BOOL(EncEffModel_CreateZoroark, 36)
CREATOR_TWO(EncEffRipple_CreateBlack, 37)

#undef CREATOR_U32
#undef CREATOR_BOOL
#undef CREATOR_TWO

// The table's render pointers must link too; this test creates effects but does not render them.
void EncEffModel_Render(EncEff *effect) {
    (void)effect;
}
void EncEffRipple_ClearAndDraw(EncEff *effect) {
    (void)effect;
}
void EncEffGrid_ClearAndDraw(EncEff *effect) {
    (void)effect;
}

int main(void) {
    for (sExpected = 0; sExpected < ENCEFF_COUNT; sExpected++) {
        const EncEffInfo *entry = &ENC_EFFECT_CONTROLLERS[sExpected];
        GameEvent *result = entry->create((GameSystem *)&sSystemToken, (Field *)&sFieldToken, entry->param);
        if (result != (GameEvent *)&sResults[sExpected]) {
            fputs("encounter callback changed its return value\n", stderr);
            return 1;
        }
    }
    if (sSeen != ENCEFF_COUNT) {
        return 1;
    }
    puts("all 38 typed encounter callbacks preserve target, arguments and return value");
    return 0;
}
