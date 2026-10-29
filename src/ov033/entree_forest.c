#include "types.h"
#include "field/entree_forest.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/symbol_map.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/tpoke_data.h"

typedef struct {
    u16 moveCode;
    u8 areaW;
    u8 areaH;
} EntreeForestActorAppearance;

// Where the Pokémon of an area stand, as pairs of grid X and Z, by which of SymbolMap_IsTopArea and
// SymbolMap_IsUpperRowArea holds for the area
static const u8 sPokemonPositions1[] = {
    13, 20, 18, 13, 18, 21, 13, 12, 9, 19, 22, 14, 22, 19, 9, 14, 18, 18, 13, 15,
};

// The move code and the area of a Pokémon's actor, by EntreeForestPokemon.actorType
// clang-format off
static const EntreeForestActorAppearance sActorAppearances[8] = {
    { MOVE_CODE_TURN_RANDOM, 0, 0 },
    { MOVE_CODE_WALK_RANDOM, 1, 1 },
    { MOVE_CODE_WALK_RANDOM, 2, 2 },
    { MOVE_CODE_WALK_RANDOM_UP_DOWN, 0, 2 },
    { MOVE_CODE_WALK_RANDOM_LEFT_RIGHT, 2, 0 },
    { MOVE_CODE_WALK_RANDOM, 2, 0 },
    { MOVE_CODE_SPIN_CLOCKWISE, 0, 0 },
    { MOVE_CODE_SPIN_COUNTERCLOCKWISE, 0, 0 },
};

// clang-format on

// The actor of a Pokémon, before its ID, model, movement and position are set
static const ZoneNPC sPokemonNPC = {
    0,      0x1000, MOVE_CODE_FACE_DOWN, ACTOR_EVENT_TYPE_NONE, 0, 0x28fa, 1, 0, 0, 0, 0xffff,
    0xffff, FALSE,  { { 0, 0, 0 } },
};

static const u8 sPokemonPositions2[] = {
    14, 18, 17, 15, 17, 18, 14, 15, 11, 21, 20, 12, 20, 21, 11, 12, 14, 21, 17, 12,
    17, 21, 14, 12, 11, 18, 20, 15, 20, 18, 11, 15, 8,  19, 23, 14, 23, 19, 8,  14,
};

static const u8 sPokemonPositions0[] = {
    14, 18, 17, 12, 17, 18, 14, 12, 11, 18, 20, 12, 20, 18, 11, 12, 8, 14, 23, 14,
    14, 21, 17, 15, 17, 21, 14, 15, 11, 21, 20, 15, 20, 21, 11, 15, 8, 19, 23, 19,
};

FieldActor *EntreeForest_SpawnPkmActor(const EntreeForestSpawnContext *context, const EntreeForestPokemon *pokemon,
                                       u16 index, u16 x, u16 z, s32 y) {
    const EntreeForestActorAppearance *appearance;
    MMSys *actorSystem;
    u16 zoneId;
    ZoneNPC npc;
    ZoneNPCGridPosition *grid;

    appearance = &sActorAppearances[pokemon->actorType];
    actorSystem = Field_GetActorSystem(context->field);
    zoneId = Field_GetPlayerStateZoneID(context->field);
    npc = sPokemonNPC;
    npc.uid = index;
    npc.modelId = GetPokemonFieldOBJCODE(context->pokemonData, pokemon->species, pokemon->sex, pokemon->form);
    npc.moveCode = appearance->moveCode;
    npc.areaW = appearance->areaW;
    npc.areaH = appearance->areaH;
    npc.param0 = context->actorIdBase + index;
    npc.param1 = pokemon->raw;
    npc.param2 = pokemon->raw >> 16;
    grid = &npc.pos.grid;
    grid->x = x;
    grid->z = z;
    grid->y = y;
    return CreateNewActorByEntityNoWKOBJCODE(actorSystem, &npc, zoneId);
}

void GetEntreeForestActorParamBits(u32 *paramBits, FieldActor *actor) {
    u32 low = GetActorUserParam(actor, 1);
    u32 high = GetActorUserParam(actor, 2);

    *paramBits = low | (high << 16);
}

u16 GetActorUserParam0(FieldActor *actor) {
    return GetActorUserParam(actor, 0);
}

PartyPkm *EntreeForest_CreateSymbolMon(HeapID heapId, GameData *gameData, const EntreeForestPokemon *pokemon) {
    u8 *levels;
    u8 level;
    u32 id;
    u32 pid;
    PartyPkm *pkm;

    levels = GFL_ArcSysReadHeapNewLZ(0xdf, 0, 0, heapId);
    level = levels[pokemon->species];
    GFL_HeapFree(levels);
    id = getIDAsUInt(GetGameDataPlayerInfo(gameData));
    pid = PML_GenPID(id, pokemon->species, pokemon->form, pokemon->sex, 0, PID_RARE_NEVER);
    pkm = PokeParty_NewTempPkm(pokemon->species, level, -1, heapId);
    PokeParty_CreatePkm(pkm, pokemon->species, level, id, PKM_IVS_RANDOM, pid);
    PokeParty_SetHiddenAbil(pkm, pokemon->species, pokemon->form);
    PokeParty_ChangeForme(pkm, pokemon->form);
    if (pokemon->move != 0) {
        if (PokeParty_LearnMove(pkm, pokemon->move) == 0xffff) {
            PokeParty_SetLastMove(pkm, pokemon->move);
        }
    }
    return pkm;
}

void EntreeForest_SpawnAllPokemon(Field *field, u32 actorIdBase, const EntreeForestPokemon *pokemon, u32 unused,
                                  u32 area) {
    const u8 *positions;
    EntreeForestSpawnContext context;
    FieldActor *playerActor;
    u16 playerX;
    u16 playerZ;
    s32 i;
    s32 offsetX;
    s32 offsetZ;
    s32 x;
    s32 z;

    if (SymbolMap_IsTopArea(area)) {
        positions = sPokemonPositions0;
    } else if (SymbolMap_IsUpperRowArea(area)) {
        positions = sPokemonPositions1;
    } else {
        positions = sPokemonPositions2;
    }
    context.pokemonData = LoadTPokeData(Field_GetHeapID(field));
    context.field = field;
    context.actorIdBase = actorIdBase;
    playerActor = FieldPlayer_GetActor(Field_GetPlayer(field));
    playerX = GetGPosX(playerActor);
    playerZ = GetGPosZ(playerActor);
    for (i = 0; i < 20; i++) {
        if (pokemon[i].species != 0) {
            offsetX = GFL_RandomLCAlt(2) - 1;
            offsetZ = GFL_RandomLCAlt(2) - 1;
            x = positions[i * 2] + offsetX;
            z = positions[i * 2 + 1] + offsetZ;
            if (playerX == x && playerZ == z) {
                if (offsetX != 0) {
                    x -= offsetX;
                } else if (offsetZ != 0) {
                    z -= offsetZ;
                } else {
                    x++;
                }
            }
            EntreeForest_SpawnPkmActor(&context, &pokemon[i], i, x, z, 0);
        }
    }
    FreeTPokeData(context.pokemonData);
}
