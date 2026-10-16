#include "types.h"
#include "app/pokemon_trade/pokemontrade_mcss.h"
#include "app/pokemon_trade_local.h"
#include "constants/sound.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/mcss.h"

// The sounds of the trade's animation and the moves of the Pokémon's sprites across the screen

// A sound and the frame of the animation it plays at
typedef struct {
    u32 se;
    int frame;
} TradeSoundCue;

static void func_ov194_021be878(int frame, PokemonTradeWork *wk);
static fx32 TradeMcssMove_Interpolate(fx32 start, fx32 end, TradeMcssMove *move, BOOL bounce);
static void TradeMcssMove_FollowPath(TradeMcssMove *move, PokemonTradeWork *wk);

void func_ov194_021be808(int frame) {
    TradeSoundCue cues[] = {
        { SEQ_SE_W554_PAKIN, 1 },   { SEQ_SE_BOWA2, 4 },       { SEQ_SE_KON, 29 },         { SEQ_SE_KON, 45 },
        { SEQ_SE_KON, 56 },         { SEQ_SE_KON, 63 },        { SEQ_SE_TDEMO_001, 17 },   { SEQ_SE_TDEMO_002, 107 },
        { SEQ_SE_W028_02, 152 },    { SEQ_SE_TDEMO_006, 266 }, { SEQ_SE_TDEMO_007, 350 },  { SEQ_SE_TDEMO_008, 636 },
        { SEQ_SE_TDEMO_009, 650 },  { SEQ_SE_TDEMO_010, 694 }, { SEQ_SE_TDEMO_011, 857 },  { SEQ_SE_TDEMO_001, 858 },
        { SEQ_SE_TDEMO_009, 896 },  { SEQ_SE_KON, 1048 },      { SEQ_SE_KON, 1068 },       { SEQ_SE_KON, 1082 },
        { SEQ_SE_KON, 1092 },       { SEQ_SE_KON, 1103 },      { SEQ_SE_TDEMO_004, 1123 }, { SEQ_SE_BOWA2, 1166 },
        { SEQ_SE_TDEMO_001, 1159 },
    };
    u32 i;
    for (i = 0; i < NELEMS(cues); i++) {
        if (frame == cues[i].frame) {
            GFL_SndSEPlay(cues[i].se);
        }
    }
}

void func_ov194_021be840(int frame) {
    TradeSoundCue cues[] = {
        { SEQ_SE_W554_PAKIN, 32 }, { SEQ_SE_BOWA2, 33 },      { SEQ_SE_KON, 38 },        { SEQ_SE_KON, 45 },
        { SEQ_SE_KON, 56 },        { SEQ_SE_KON, 64 },        { SEQ_SE_TDEMO_001, 30 },  { SEQ_SE_W028_02, 110 },
        { SEQ_SE_TDEMO_009, 130 }, { SEQ_SE_TDEMO_002, 241 }, { SEQ_SE_TDEMO_003, 320 }, { SEQ_SE_W054_01, 364 },
        { SEQ_SE_W179_02, 409 },   { SEQ_SE_W307_03, 484 },   { SEQ_SE_KON, 494 },       { SEQ_SE_KON, 514 },
        { SEQ_SE_KON, 528 },       { SEQ_SE_KON, 538 },       { SEQ_SE_KON, 547 },       { SEQ_SE_TDEMO_004, 569 },
        { SEQ_SE_TDEMO_005, 615 }, { SEQ_SE_TDEMO_001, 605 },
    };
    u32 i;
    for (i = 0; i < NELEMS(cues); i++) {
        if (frame == cues[i].frame) {
            GFL_SndSEPlay(cues[i].se);
        }
    }
}

static void func_ov194_021be878(int frame, PokemonTradeWork *wk) {
    TradeSoundCue cues[] = {
        { SEQ_SE_DANSA, 1 },
        { SEQ_SE_SYS_83, 13 },
    };
    TradeSoundCue bounceCues[] = {
        { SEQ_SE_DANSA, 1 },   { SEQ_SE_SYS_83, 13 }, { SEQ_SE_DANSA, 15 },
        { SEQ_SE_SYS_83, 25 }, { SEQ_SE_DANSA, 27 },  { SEQ_SE_SYS_83, 39 },
    };
    u32 i;
    if (wk->unk11EF == 0) {
        if (wk->unk11ED == 0) {
            for (i = 0; i < NELEMS(cues); i++) {
                if (frame == cues[i].frame) {
                    GFL_SndSEPlay(cues[i].se);
                }
            }
        } else {
            for (i = 0; i < NELEMS(bounceCues); i++) {
                if (frame == bounceCues[i].frame) {
                    GFL_SndSEPlay(bounceCues[i].se);
                }
            }
        }
    }
}

void TradeMcssMove_Start(TradeMcssMove *move, int duration, const VecFx32 *end) {
    VecFx32 start;
    MCSS_GetPosition(move->mcss, &start);
    move->frame = 0;
    move->duration = duration;
    MI_CpuCopy8(end, &move->end, sizeof(VecFx32));
    MI_CpuCopy8(&start, &move->start, sizeof(VecFx32));
}

TradeMcssMove *TradeMcssMove_Create(MCSS *mcss, int duration, const VecFx32 *end, HeapID heapId) {
    TradeMcssMove *move = GFL_HeapAllocate(heapId, sizeof(TradeMcssMove), TRUE, "pokemontrade_mcss.c", 210);
    move->mcss = mcss;
    TradeMcssMove_Start(move, duration, end);
    move->unkC = 0;
    return move;
}

TradeMcssMove *TradeMcssMove_CreateWithPath(MCSS *mcss, int duration, const VecFx32 *end, const VecFx32 *path,
                                            HeapID heapId) {
    TradeMcssMove *move = GFL_HeapAllocate(heapId, sizeof(TradeMcssMove), TRUE, "pokemontrade_mcss.c", 231);
    move->mcss = mcss;
    TradeMcssMove_Start(move, duration, end);
    move->unkC = 0;
    move->path = path;
    return move;
}

static fx32 TradeMcssMove_Interpolate(fx32 start, fx32 end, TradeMcssMove *move, BOOL bounce) {
    fx32 step = (end - start) / move->duration;
    fx32 pos = start + step * move->frame;
    if (move->bounce != 0 && bounce) {
        pos = end + FX_SinIdx(move->angle) * 8;
    }
    return pos;
}

static void TradeMcssMove_FollowPath(TradeMcssMove *move, PokemonTradeWork *wk) {
    VecFx32 pos;
    const VecFx32 *offset = &move->path[move->frame];
    pos.x = offset->x + move->start.x;
    pos.y = offset->y + move->start.y;
    pos.z = offset->z + move->start.z;
    MCSS_SetPosition(move->mcss, &pos);
    func_ov194_021be878(move->frame, wk);
}

void TradeMcssMove_Update(TradeMcssMove *move, PokemonTradeWork *wk) {
    VecFx32 pos;
    VecFx32 current;
    if (move != NULL) {
        move->frame++;
        if (move->duration >= move->frame) {
            if (move->duration != move->frame) {
                if (move->path != NULL) {
                    TradeMcssMove_FollowPath(move, wk);
                    return;
                }
                MCSS_GetPosition(move->mcss, &current);
                pos.x = TradeMcssMove_Interpolate(move->start.x, move->end.x, move, FALSE);
                if (move->bounce != 0) {
                    move->angle -= (u16)(int)((f32)(current.x - pos.x) / FX32_ONE * 374.0f);
                }
                pos.y = TradeMcssMove_Interpolate(move->start.y, move->end.y, move, TRUE);
                pos.z = TradeMcssMove_Interpolate(move->start.z, move->end.z, move, FALSE);
                MCSS_SetPosition(move->mcss, &pos);
            } else {
                MCSS_SetPosition(move->mcss, &move->end);
            }
        }
    }
}

void func_ov194_021beab4(PokemonTradeWork *wk) {
    MCSS_PauseAnimation(wk->mcss[0]);
    wk->unk854 = 1;
}

void TradeMcssMove_Free(TradeMcssMove *move) {
    if (move != NULL) {
        GFL_HeapFree(move);
    }
}
