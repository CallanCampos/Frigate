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

void func_ov194_021c1b74(PokemonTrade3DWork *work, int path);
void func_ov194_021c1d00(PokemonTrade3DWork *work);
void func_ov194_021c1d30(PokemonTrade3DWork *work);
void func_ov194_021c1d38(PokemonTrade3DWork *work);
void func_ov194_021c1db4(PokemonTrade3DWork *work);
void func_ov194_021c1e08(PokemonTrade3DWork *work);
void func_ov194_021c1e38(PokemonTrade3DWork *work, int count);
void func_ov194_021c1e74(PokemonTrade3DWork *work);
void func_ov194_021c1ef0(PokemonTradeWork *wk);
void func_ov194_021c1fb8(PokemonTradeWork *wk);
void func_ov194_021c1fc0(PokemonTradeWork *wk);
void func_ov194_021c2000(PokemonTradeWork *wk);
void func_ov194_021c200c(PokemonTradeWork *wk, int sceneId);
void func_ov194_021c2034(PokemonTradeWork *wk);
// Adds the sprite of a Pokémon on a side, facing front or back, mirrored if asked and the species allows it
void func_ov194_021c23a4(PokemonTradeWork *wk, int side, BOOL front, PartyPkm *pkm, BOOL mirror);
void func_ov194_021c24ac(PokemonTradeWork *wk, int side, BOOL front, PartyPkm *pkm, BOOL mirror, BOOL a5);
void func_ov194_021c24cc(PokemonTradeWork *wk, int side, BOOL front, PartyPkm *pkm, BOOL mirror);
void func_ov194_021c24dc(PokemonTradeWork *wk, int side);
void func_ov194_021c29b4(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_3D_H
