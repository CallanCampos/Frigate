#ifndef POKEBW2_DEMO_EVOLUTION_DEMO_H
#define POKEBW2_DEMO_EVOLUTION_DEMO_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The evolution demo
#define OVERLAY_EVOLUTION_DEMO OVERLAY_ID(284)

typedef struct {
    GameData *gameData;
    PokeParty *party;
    // The species to evolve into
    u16 species;
    u8 partyIndex;
    // EVO_METHOD_*
    u8 method;
    // Set, the demo pushes and brings back the music that was playing, and picks a move to forget on overlay 207's
    // screen instead of overlay 287's
    u32 unkC;
    // Whether B stops the evolution. The demo clears it for an evolution by item
    BOOL canCancel;
} EvolutionDemoParam;

extern const GameProcFunctions EVOLUTION_DEMO_PROC_FUNCTIONS;

typedef struct EvolutionDemoGraphic EvolutionDemoGraphic;
typedef struct EvolutionDemoEffect EvolutionDemoEffect;
typedef struct EvolutionDemoView EvolutionDemoView;

// evolution_demo_graphic.c
EvolutionDemoGraphic *EvolutionDemoGraphic_Create(u32 layout, HeapID heapId);
void EvolutionDemoGraphic_Free(EvolutionDemoGraphic *graphic);
void EvolutionDemoGraphic_Update(EvolutionDemoGraphic *graphic);
void EvolutionDemoGraphic_Begin3D(EvolutionDemoGraphic *graphic);
void EvolutionDemoGraphic_End3D(EvolutionDemoGraphic *graphic);
ClActUnit *EvolutionDemoGraphic_GetClActUnit(EvolutionDemoGraphic *graphic);
// Creates and frees the BGs of the sub screen
void EvolutionDemoGraphic_InitSubBG(EvolutionDemoGraphic *graphic);
void EvolutionDemoGraphic_FreeSubBG(EvolutionDemoGraphic *graphic);

// evolution_demo_view.c: the evolving Pokémon. played creates it after the evolution has been shown, with the evolved
// Pokémon
EvolutionDemoView *EvolutionDemoView_Create(HeapID heapId, BOOL played, PartyPkm *pkm, u16 species);
void EvolutionDemoView_Free(EvolutionDemoView *view);
void EvolutionDemoView_Update(EvolutionDemoView *view);
void EvolutionDemoView_Draw(EvolutionDemoView *view);
// Plays the Pokémon's cry
void EvolutionDemoView_Start(EvolutionDemoView *view);
BOOL EvolutionDemoView_IsCryDone(EvolutionDemoView *view);
// Turns the sprite white and breaks it into pieces
void EvolutionDemoView_Evolve(EvolutionDemoView *view);
BOOL EvolutionDemoView_HavePiecesReturned(EvolutionDemoView *view);
// Shows the evolved sprite, still white
void EvolutionDemoView_Reveal(EvolutionDemoView *view);
// Fades the sprite in from white, then plays its cry
void EvolutionDemoView_FadeIn(EvolutionDemoView *view);
BOOL EvolutionDemoView_IsDone(EvolutionDemoView *view);
BOOL EvolutionDemoView_HavePiecesStarted(EvolutionDemoView *view);
// Set with HavePiecesReturned
BOOL EvolutionDemoView_GetUnk3C(EvolutionDemoView *view);
// Stops the evolution while the pieces are moving, and returns whether it did
BOOL EvolutionDemoView_Cancel(EvolutionDemoView *view);

// evolution_demo_effect.c: the particles and 3D model around the evolving Pokémon
EvolutionDemoEffect *EvolutionDemoEffect_Create(HeapID heapId, BOOL played);
void EvolutionDemoEffect_Free(EvolutionDemoEffect *effect);
void EvolutionDemoEffect_Update(EvolutionDemoEffect *effect);
void EvolutionDemoEffect_Draw(EvolutionDemoEffect *effect);
void EvolutionDemoEffect_Cancel(EvolutionDemoEffect *effect);
// Starts the particles
void EvolutionDemoEffect_Start(EvolutionDemoEffect *effect);
// Plays the 3D model once the particles have started
void EvolutionDemoEffect_Start3D(EvolutionDemoEffect *effect);
BOOL EvolutionDemoEffect_Is3DHeld(EvolutionDemoEffect *effect);
BOOL EvolutionDemoEffect_Is3DReversing(EvolutionDemoEffect *effect);

#endif // POKEBW2_DEMO_EVOLUTION_DEMO_H
