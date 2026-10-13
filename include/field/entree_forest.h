#ifndef POKEBW2_FIELD_ENTREE_FOREST_H
#define POKEBW2_FIELD_ENTREE_FOREST_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Function names from swan.
void GetEntreeForestActorParamBits(u32 *paramBits, FieldActor *actor);
u16 GetActorUserParam0(FieldActor *actor);
GameEvent *EventEntreeForestWarp_Create(GameSystem *gsys, u32 mode, const VecFx32 *position, u32 arg3, u32 arg4);
GameEvent *CheckEntralinkForestFirstWarpEvent(Field *field, GameSystem *gsys, FieldPlayer *player);

// A Pokémon of the Entree Forest, packed in a word that its actor keeps in params 1 and 2
typedef union {
    u32 raw;
    struct {
        u32 species : 11;
        u32 move : 10;
        u32 sex : 2;
        u32 form : 6;
        u32 actorType : 3;
    };
} EntreeForestPokemon;

typedef struct {
    TPokeData *pokemonData;
    Field *field;
    u32 actorIdBase;
} EntreeForestSpawnContext;

FieldActor *EntreeForest_SpawnPkmActor(const EntreeForestSpawnContext *context, const EntreeForestPokemon *pokemon,
                                      u16 index, u16 x, u16 z, s32 y);
void EntreeForest_SpawnAllPokemon(Field *field, u32 actorIdBase, const EntreeForestPokemon *pokemon, u32 unused,
                                  u32 area);

PartyPkm *func_ov033_02176bd0(HeapID heapId, GameData *gameData, const EntreeForestPokemon *pokemon);

#endif // POKEBW2_FIELD_ENTREE_FOREST_H
