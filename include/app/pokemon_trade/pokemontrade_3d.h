#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_3D_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_3D_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The particles and the camera and emitter paths of the trade demo's 3D effects. The demo overlays 192 and 193
// allocate it and fill the fields left unknown here
struct PokemonTrade3DWork {
    ParticleSystem *systems[9];
    void *systemWork[9];
    // The sizes of the two sprites that the infrared trade's scene scales
    VecFx32 ircSpriteSizes[2];
    // The size of each of the four Pokémon sprites, as MCSS_GetScale gives it
    VecFx32 spriteSizes[4];
    G3DCurve *cameraPosPath;
    G3DCurve *cameraTargetPath;
    // The paths that the two emitters follow
    G3DCurve *emitterPaths[2];
    SPLEmitter *emitters[2];
    // Whether the camera's paths have come to their end
    BOOL cameraPosDone;
    BOOL cameraTargetDone;
    // The sprite of the demo's scenes
    ClActor *actor;
    void *resource;
    u8 unkB8[2];
    HeapID heapId;
};

void Poketrade3D_CreateIcaStreamsForMode(PokemonTrade3DWork *work, int path);
void Poketrade3D_CreateIcaStreamsEnding(PokemonTrade3DWork *work);
void Poketrade3D_FreeParticleResources(PokemonTrade3DWork *work);
void Poketrade3D_InitParticleSystems(PokemonTrade3DWork *work);
void func_ov194_021c1db4(PokemonTrade3DWork *work);
void Poketrade3D_SetupParticleSystemsHigh(PokemonTrade3DWork *work);
void Poketrade3D_ExitParticleManager(PokemonTrade3DWork *work, int count);
void Poketrade3D_InitTradeParticleSet(PokemonTrade3DWork *work);
void Poketrade3D_Init3DScene(PokemonTradeWork *wk);
void Poketrade3D_Update3DScene(PokemonTradeWork *wk);
void Poketrade3D_Delete3DScene(PokemonTradeWork *wk);
void Poketrade3D_UploadCubeTextures(PokemonTradeWork *wk);
void Poketrade3D_AddUnitAndInit(PokemonTradeWork *wk, int sceneId);
void Poketrade3D_RemoveCurrentUnit(PokemonTradeWork *wk);
// Adds the sprite of a Pokémon on a side, facing front or back, mirrored if asked and the species allows it
void Poketrade3D_AddPokemonMcss(PokemonTradeWork *wk, int side, BOOL front, PartyPkm *pkm, BOOL mirror);
void Poketrade3D_AddPokemonMcssFlipped(PokemonTradeWork *wk, int side, BOOL front, PartyPkm *pkm, BOOL mirror, BOOL a5);
void Poketrade3D_AddPokemonMcssPlain(PokemonTradeWork *wk, int side, BOOL front, PartyPkm *pkm, BOOL mirror);
void Poketrade3D_DeleteMcssSlot(PokemonTradeWork *wk, int side);
void Poketrade3D_FadeTexturePalette(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_3D_H
