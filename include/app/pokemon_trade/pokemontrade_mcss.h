#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MCSS_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MCSS_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/mcss.h"

// A Pokémon's sprite moving to a place over some frames, straight, bouncing or along a path of offsets
struct TradeMcssMove {
    MCSS *mcss;
    int duration;
    int frame;
    u32 unkC;
    u8 unk10[4];
    u16 angle;
    u16 bounce;
    u8 unk18[4];
    VecFx32 start;
    VecFx32 end;
    const VecFx32 *path;
};

// The sounds of the animations, at each frame
void func_ov194_021be808(int frame);
void func_ov194_021be840(int frame);
void TradeMcssMove_Start(TradeMcssMove *move, int duration, const VecFx32 *end);
TradeMcssMove *TradeMcssMove_Create(MCSS *mcss, int duration, const VecFx32 *end, HeapID heapId);
TradeMcssMove *TradeMcssMove_CreateWithPath(MCSS *mcss, int duration, const VecFx32 *end, const VecFx32 *path,
                                            HeapID heapId);
void TradeMcssMove_Update(TradeMcssMove *move, PokemonTradeWork *wk);
void func_ov194_021beab4(PokemonTradeWork *wk);
void TradeMcssMove_Free(TradeMcssMove *move);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MCSS_H
