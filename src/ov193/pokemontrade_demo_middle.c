#include "types.h"
#include "app/pokemon_trade_local.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/mcss.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_3.h"

// pokemontrade_demo_middle.c, a guessed name: one of the trade demo's three scenes, a copy of the template that
// pokemontrade_demo_common.c and pokemontrade_demo_up.c share

static const VecFx32 sPosFirst = { 0, FX32_CONST(-23), FX32_CONST(10) };
static const VecFx32 sPosSecond = { 0, FX32_CONST(-23), FX32_CONST(-10) };

static void PokemonTradeDemoMiddle_UpdateFade(TradeFade *fade);
static void PokemonTradeDemoMiddle_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
                                            BOOL partner);
static void PokemonTradeDemoMiddle_CreateSprite(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_LoadSprite(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_FreeSprite(PokemonTradeWork *wk);
static int PokemonTradeDemoMiddle_Frame(int frame);
static void PokemonTradeDemoMiddle_SetTimer(PokemonTradeWork *wk, int timer);
static void PokemonTradeDemoMiddle_StartBgmTimer(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_StateWaitDemo(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_StateWaitSync(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_StateSetUpMoves(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_StateMoves(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_ShowMessage(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_StateMessage(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_End(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_FreeMove(PokemonTradeWork *wk, int index);
static void PokemonTradeDemoMiddle_StartMove(PokemonTradeWork *wk, int index);
static void PokemonTradeDemoMiddle_ShowSprite(PokemonTradeWork *wk, int index);
static void PokemonTradeDemoMiddle_FollowPath0(SPLEmitter *emitter, u32 type);
static void PokemonTradeDemoMiddle_AttachPath0(SPLEmitter *emitter);
static void PokemonTradeDemoMiddle_FollowPath1(SPLEmitter *emitter, u32 type);
static void PokemonTradeDemoMiddle_AttachPath1(SPLEmitter *emitter);
static void PokemonTradeDemoMiddle_Init(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_Wait1(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_Wait2(PokemonTradeWork *wk);
static void PokemonTradeDemoMiddle_Run(PokemonTradeWork *wk);

// The hops of a Pokémon's sprite, as offsets from its place: one hop of 18 frames, a second copy of it, or two of 40.
// The common file has the same
static VecFx32 sTradeDemoMiddleHopPath[18] = {
    { 0, 0, 0 },      { 0, 0x3e1d, 0 }, { 0, 0x7800, 0 }, { 0, 0xa9b4, 0 }, { 0, 0xcfd8, 0 }, { 0, 0xe7d2, 0 },
    { 0, 0xf000, 0 }, { 0, 0xe7d2, 0 }, { 0, 0xcfd8, 0 }, { 0, 0xa9b4, 0 }, { 0, 0x7800, 0 }, { 0, 0x3e1d, 0 },
    { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },
};

static VecFx32 sTradeDemoMiddleMoveHopPath[18] = {
    { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0x3e1d, 0 }, { 0, 0x7800, 0 },
    { 0, 0xa9b4, 0 }, { 0, 0xcfd8, 0 }, { 0, 0xe7d2, 0 }, { 0, 0xf000, 0 }, { 0, 0xe7d2, 0 }, { 0, 0xcfd8, 0 },
    { 0, 0xa9b4, 0 }, { 0, 0x7800, 0 }, { 0, 0x3e1d, 0 }, { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },
};

static VecFx32 sTradeDemoMiddleLongHopPath[40] = {
    { 0, 0, 0 },      { 0, 0x2969, 0 }, { 0, 0x5000, 0 }, { 0, 0x7123, 0 }, { 0, 0x8a90, 0 }, { 0, 0x9a8c, 0 },
    { 0, 0xa000, 0 }, { 0, 0x9a8c, 0 }, { 0, 0x8a90, 0 }, { 0, 0x7123, 0 }, { 0, 0x5000, 0 }, { 0, 0x2969, 0 },
    { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0x2969, 0 }, { 0, 0x5000, 0 }, { 0, 0x7123, 0 }, { 0, 0x8a90, 0 },
    { 0, 0x9a8c, 0 }, { 0, 0xa000, 0 }, { 0, 0x9a8c, 0 }, { 0, 0x8a90, 0 }, { 0, 0x7123, 0 }, { 0, 0x5000, 0 },
    { 0, 0x2969, 0 }, { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0x2969, 0 }, { 0, 0x5000, 0 }, { 0, 0x7123, 0 },
    { 0, 0x8a90, 0 }, { 0, 0x9a8c, 0 }, { 0, 0xa000, 0 }, { 0, 0x9a8c, 0 }, { 0, 0x8a90, 0 }, { 0, 0x7123, 0 },
    { 0, 0x5000, 0 }, { 0, 0x2969, 0 }, { 0, 0, 0 },      { 0, 0, 0 },
};

// The path that the sprites follow when they don't hop, which nothing fills
static VecFx32 sTradeDemoMiddleScenePath[20];

static void PokemonTradeDemoMiddle_UpdateFade(TradeFade *fade) {
    if (fade != NULL && fade->frames >= fade->frame) {
        fx32 step;
        int brightness;

        fade->frame++;
        step = ((fade->end - fade->start) << FX32_SHIFT) / fade->frames;
        brightness = step * fade->frame / FX32_ONE;
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 0x1e, brightness);
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0x1f, brightness);
    }
}

// Works out, for a side of the trade, whether its Pokémon was caught by the player and whether its sprite doesn't hop
static void PokemonTradeDemoMiddle_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
                                            BOOL partner) {
    PartyPkm *pkm;
    int side;
    u32 egg;
    u32 species;
    u32 form;
    u32 noBounceParam;
    void *personal;

    if (partner) {
        if (PokemonTrade_IsNegoType(wk)) {
            side = 1;
        } else {
            side = 0;
        }
    } else {
        if (PokemonTrade_IsNegoType(wk)) {
            side = 0;
        } else {
            side = 1;
        }
    }
    pkm = PokemonTrade_GetPkm(wk, side);
    PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
    species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    egg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (egg == 0) {
        personal = PML_PersonalLoad(species, form, wk->heapId);
        noBounceParam = PML_PersonalGetParam(personal, PERSONAL_NO_BOUNCE);
        PML_PersonalFree(personal);
    }
    if (egg != 0) {
        *noBounce = TRUE;
    } else if (noBounceParam != 0) {
        *noBounce = TRUE;
    } else {
        *noBounce = FALSE;
    }
    if (!IsTrainerOT(pkm, player)) {
        *ownedByPlayer = FALSE;
    } else {
        *ownedByPlayer = TRUE;
    }
}

// Creates the effects' sprite, or restarts it
static void PokemonTradeDemoMiddle_CreateSprite(PokemonTradeWork *wk) {
    if (wk->work3D->actor == NULL) {
        ClActorSetup setup;

        setup.x = 128;
        setup.y = 99;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 0;
        wk->work3D->actor = func_0204c040(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_DEMO],
                                          wk->objRes[TRADE_OBJRES_PLTT_DEMO], wk->objRes[TRADE_OBJRES_CELL_DEMO],
                                          &setup, 0, wk->heapId);
        func_0204c520(wk->work3D->actor, TRUE);
        func_0204c124(wk->work3D->actor, TRUE);
    } else {
        func_0204c56c(wk->work3D->actor);
    }
}

// Loads the sprite's resources
static void PokemonTradeDemoMiddle_LoadSprite(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0x68, wk->heapId);

    wk->objRes[TRADE_OBJRES_CHAR_DEMO] = func_0204b81c(arc, 2, FALSE, 0, wk->heapId);
    wk->objRes[TRADE_OBJRES_PLTT_DEMO] = func_0204bbb8(arc, 3, 0, 0xe0, 0, 1, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_DEMO] = func_0204bde0(arc, 1, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->work3D->actor = NULL;
}

static void PokemonTradeDemoMiddle_FreeSprite(PokemonTradeWork *wk) {
    if (wk->work3D->actor != NULL) {
        func_0204c108(wk->work3D->actor);
    }
    if (wk->objRes[TRADE_OBJRES_CHAR_DEMO] != 0) {
        func_0204b98c(wk->objRes[TRADE_OBJRES_CHAR_DEMO]);
        func_0204bcd0(wk->objRes[TRADE_OBJRES_PLTT_DEMO]);
        func_0204be64(wk->objRes[TRADE_OBJRES_CELL_DEMO]);
    }
    wk->objRes[TRADE_OBJRES_CHAR_DEMO] = 0;
    wk->objRes[TRADE_OBJRES_PLTT_DEMO] = 0;
    wk->objRes[TRADE_OBJRES_CELL_DEMO] = 0;
    wk->work3D->actor = NULL;
}

// Does nothing and returns its argument. The frame that a step happens at goes through it
static int PokemonTradeDemoMiddle_Frame(int frame) {
    return frame;
}

static void PokemonTradeDemoMiddle_SetTimer(PokemonTradeWork *wk, int timer) {
    wk->timer = timer;
}

static void PokemonTradeDemoMiddle_StartBgmTimer(PokemonTradeWork *wk) {
    wk->bgmTimer = 1;
}

static void PokemonTradeDemoMiddle_StateWaitDemo(PokemonTradeWork *wk) {
    func_ov194_021c29b4(wk);
    if (wk->unk854 == 1) {
        PokemonTradeDemoMiddle_InitSide(wk, &wk->unk11ED, &wk->unk11EF, wk->partnerInfo, TRUE);
        PokemonTradeDemoMiddle_InitSide(wk, &wk->unk11EE, wk->unk11F0, wk->myInfo, FALSE);
        func_ov194_021c2034(wk);
        if (PokemonTrade_IsNetwork(wk)) {
            func_02040624(func_02040440(), 0x11, 8);
            PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_StateWaitSync);
        } else {
            PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_StateSetUpMoves);
        }
    }
}

static void PokemonTradeDemoMiddle_StateWaitSync(PokemonTradeWork *wk) {
    if (func_02040664(func_02040440(), 0x11, 8)) {
        PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_StateSetUpMoves);
    }
}

static void PokemonTradeDemoMiddle_StateSetUpMoves(PokemonTradeWork *wk) {
    VecFx32 first;
    VecFx32 second;

    PokemonTradeDemoMiddle_SetTimer(wk, 0);
    wk->fade = GFL_HeapAllocate(wk->heapId, sizeof(TradeFade), TRUE, "pokemontrade_demo_common.c", 246);
    wk->fade->frames = PokemonTradeDemoMiddle_Frame(14);
    wk->fade->frame = 0;
    wk->fade->start = 0;
    wk->fade->end = -16;
    first = sPosFirst;
    TradeMcssMove_Free(wk->mcssMoves[0]);
    wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeDemoMiddle_Frame(42), &first, wk->heapId);
    second = sPosSecond;
    TradeMcssMove_Free(wk->mcssMoves[1]);
    wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeDemoMiddle_Frame(84), &second, wk->heapId);
    MCSS_SetAnimationEndCallback(wk->mcss[0], (u32)wk, (MCSSAnimationCallback)func_ov194_021beab4, TRUE);
    func_0201ae2c(wk->mcss[1], 0, 16, (s8)(PokemonTradeDemoMiddle_Frame(14) / 16), 0);
    func_ov194_021bfe70(wk);
    GFL_BGSysResetStdPalette(1, 0);
    GFL_BGSysResetStdPalette(5, 0);
    PokemonTradeDemoMiddle_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_StateMoves);
}

static void PokemonTradeDemoMiddle_StateMoves(PokemonTradeWork *wk) {
    PokemonTradeDemoMiddle_UpdateFade(wk->fade);
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
    if (wk->timer > PokemonTradeDemoMiddle_Frame(42)) {
        PartyPkm *pkm;

        func_ov194_021c5ff4(wk, 0x11);
        func_ov194_021c600c(wk, 0);
        func_ov194_021c52bc(wk);
        func_ov194_021c368c(wk);
        func_ov194_021c1484(wk);
        PokemonTradeDemoMiddle_SetTimer(wk, 0);
        pkm = PokemonTrade_GetPkm(wk, PokemonTrade_IsNegoType(wk));
        PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            u32 species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
            u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            PokeVoiceChatterInfo info;

            PokeVoice_CreateChatterInfo(&info);
            PokeVoice_Play(species, form, 64, 0, 0, 0, 0, &info);
        }
        if (wk->mcssMoves[0] != NULL) {
            GFL_HeapFree(wk->mcssMoves[0]);
            wk->mcssMoves[0] = NULL;
        }
        if (wk->unk11ED == 0) {
            VecFx32 pos;

            MCSS_GetPosition(wk->mcss[0], &pos);
            TradeMcssMove_Free(wk->mcssMoves[0]);
            if (wk->unk11EF != 0) {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 20, &pos, sTradeDemoMiddleScenePath, wk->heapId);
            } else {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 18, &pos, sTradeDemoMiddleHopPath, wk->heapId);
            }
            func_ov194_021c24cc(wk, 2, FALSE, PokemonTrade_GetPkm(wk, PokemonTrade_IsNegoType(wk)), TRUE);
            MCSS_PauseAnimation(wk->mcss[2]);
            MCSS_Hide(wk->mcss[2]);
        } else {
            VecFx32 pos;

            MCSS_GetPosition(wk->mcss[0], &pos);
            TradeMcssMove_Free(wk->mcssMoves[0]);
            if (wk->unk11EF != 0) {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 20, &pos, sTradeDemoMiddleScenePath, wk->heapId);
            } else {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 40, &pos, sTradeDemoMiddleLongHopPath, wk->heapId);
            }
        }
        PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_StateMessage);
    }
}

static void PokemonTradeDemoMiddle_ShowMessage(PokemonTradeWork *wk) {
    func_02042ba8(FALSE, wk->heapId);
    if (wk->type == 6) {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_WillOffered, wk->strbufTemplate);
    } else if (wk->type == 8) {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_WillSent, wk->strbufTemplate);
    } else if (wk->unk11ED != 0) {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_ReturningGoodBye, wk->strbufTemplate);
    } else {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_SendingGoodBye, wk->strbufTemplate);
    }
    loadPokemonNicknameToStrbuf(wk->wordSet, 0, PokemonTrade_GetPkm(wk, PokemonTrade_IsNegoType(wk)));
    copyVarForText(wk->wordSet, 1, wk->partnerInfo);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
    GFL_BGSysFillScrArea(4, 0, 0, 0, 32, 24, 0x11);
    func_ov194_021bfdf8(wk, TRUE, FALSE);
    func_ov194_021c600c(wk, 0x14);
    *(vu16 *)REG_DB_BLDCNT_ADDR = 0;
}

static void PokemonTradeDemoMiddle_StateMessage(PokemonTradeWork *wk) {
    if (wk->timer == 10) {
        PokemonTradeDemoMiddle_ShowMessage(wk);
    }
    if (wk->unk11ED == 0 && wk->timer == 6) {
        VecFx32 pos;

        MCSS_GetPosition(wk->mcss[0], &pos);
        MCSS_SetPosition(wk->mcss[2], &pos);
        MCSS_Show(wk->mcss[2]);
        MCSS_Hide(wk->mcss[0]);
        MCSS_ResumeAnimation(wk->mcss[2]);
        wk->mcssMoves[0]->mcss = wk->mcss[2];
    }
    if (wk->timer > 200 && func_ov194_021c00b0(wk)) {
        func_ov194_021bfe9c(wk);
        if (wk->unk11ED == 0) {
            func_ov194_021c24dc(wk, 0);
            wk->mcss[0] = wk->mcss[2];
            wk->mcss[2] = NULL;
        }
        if (wk->mcssMoves[0] != NULL) {
            GFL_HeapFree(wk->mcssMoves[0]);
            wk->mcssMoves[0] = NULL;
        }
        PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Init);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
}

// Frees the effects and goes on to the save
static void PokemonTradeDemoMiddle_End(PokemonTradeWork *wk) {
    PokemonTradeDemoMiddle_FreeSprite(wk);
    func_ov194_021c1e38(wk->work3D, 6);
    func_ov194_021c1d30(wk->work3D);
    GFL_HeapFree(wk->work3D);
    wk->work3D = NULL;
    PokemonTrade_SetState(wk, func_ov194_021beb48);
}

static void PokemonTradeDemoMiddle_FreeMove(PokemonTradeWork *wk, int index) {
    // BUG: frees the first move whatever the index, so the second call of the scene does nothing
#ifdef BUGFIX
    if (wk->mcssMoves[index] != NULL) {
        GFL_HeapFree(wk->mcssMoves[index]);
        wk->mcssMoves[index] = NULL;
    }
#else
    if (wk->mcssMoves[0] != NULL) {
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
    }
#endif
}

// Plays the voice of the other machine's Pokémon and makes a sprite hop or bounce where it is
static void PokemonTradeDemoMiddle_StartMove(PokemonTradeWork *wk, int index) {
    PartyPkm *pkm;
    VecFx32 pos;

    pkm = PokemonTrade_GetPkm(wk, 1 - PokemonTrade_IsNegoType(wk));
    PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
        u32 species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
        u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        PokeVoice_Play(species, form, 0x40, 0, 0, 0, 0, NULL);
    }
    if (wk->unk11EE == 0) {
        MCSS_GetPosition(wk->mcss[index], &pos);
        TradeMcssMove_Free(wk->mcssMoves[3]);
        if (wk->unk11F0[0] != 0) {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(wk->mcss[index], 20, &pos, sTradeDemoMiddleScenePath, wk->heapId);
        } else {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(wk->mcss[index], 40, &pos, sTradeDemoMiddleLongHopPath, wk->heapId);
        }
    } else {
        u32 pair[4] = { 2, 3, 0, 1 };
        MCSS *target = wk->mcss[pair[index]];

        MCSS_GetPosition(wk->mcss[index], &pos);
        TradeMcssMove_Free(wk->mcssMoves[3]);
        if (wk->unk11F0[0] != 0) {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(target, 20, &pos, sTradeDemoMiddleScenePath, wk->heapId);
        } else {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(target, 18, &pos, sTradeDemoMiddleMoveHopPath, wk->heapId);
        }
    }
}

// Shows a sprite still, at the start of the scene, and scales it by its size
static void PokemonTradeDemoMiddle_ShowSprite(PokemonTradeWork *wk, int index) {
    VecFx32 pos;

    MCSS_Show(wk->mcss[index]);
    pos.x = 0;
    pos.y = FX32_CONST(-23);
    pos.z = FX32_CONST(10);
    MCSS_SetPosition(wk->mcss[index], &pos);
    MCSS_PauseAnimation(wk->mcss[index]);
    func_0201ab70(wk->mcss[index], &pos);
    if (pos.x < 0) {
        pos.x = -FX32_ONE * 16;
    } else {
        pos.x = FX32_ONE * 16;
    }
    pos.y = FX32_ONE * 16;
    pos.z = FX32_ONE;
    MCSS_SetScale(wk->mcss[index], &pos);
}

// The emitter follows the first path of the 3D effects
static void PokemonTradeDemoMiddle_FollowPath0(SPLEmitter *emitter, u32 type) {
    if (type != 0 && emitter != NULL) {
        PokemonTradeWork *wk = func_02050364(emitter);

        if (wk->work3D->emitterPaths[0] != NULL) {
            VecFx32 pos = { 0, 0, 0 };

            GFL_G3DCurveGetNowTranslation(wk->work3D->emitterPaths[0], &pos);
            func_02050208(emitter, &pos);
        }
    }
}

static void PokemonTradeDemoMiddle_AttachPath0(SPLEmitter *emitter) {
    func_0205035c(emitter, func_02050188());
    func_02050354(emitter, PokemonTradeDemoMiddle_FollowPath0);
}

// The emitter follows the second path
static void PokemonTradeDemoMiddle_FollowPath1(SPLEmitter *emitter, u32 type) {
    if (type != 0 && emitter != NULL) {
        PokemonTradeWork *wk = func_02050364(emitter);

        if (wk->work3D->emitterPaths[1] != NULL) {
            VecFx32 pos = { 0, 0, 0 };

            GFL_G3DCurveGetNowTranslation(wk->work3D->emitterPaths[1], &pos);
            func_02050208(emitter, &pos);
        }
    }
}

static void PokemonTradeDemoMiddle_AttachPath1(SPLEmitter *emitter) {
    func_0205035c(emitter, func_02050188());
    func_02050354(emitter, PokemonTradeDemoMiddle_FollowPath1);
}

void PokemonTradeDemoMiddle_Start(PokemonTradeWork *wk) {
    MCSS *tmp;

    PokemonTradeDemoMiddle_StartBgmTimer(wk);
    wk->unk854 = TRUE;
    MCSS_SetAnimation(wk->mcss[0], 0);
    wk->unk858 = TRUE;
    MCSS_SetAnimation(wk->mcss[1], 0);
    if (PokemonTrade_IsNegoType(wk)) {
        tmp = wk->mcss[0];
        wk->mcss[0] = wk->mcss[1];
        wk->mcss[1] = tmp;
    }
    PokemonTradeDemoMiddle_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_StateWaitDemo);
}

static void PokemonTradeDemoMiddle_Init(PokemonTradeWork *wk) {
    func_ov194_021bb4b4(wk);
    func_0204b758();
    func_ov194_021c2c44(wk);
    wk->clactUnit = func_0204bf1c(3, 0, wk->heapId);
    wk->work3D = GFL_HeapAllocate(wk->heapId, sizeof(PokemonTrade3DWork), TRUE, "pokemontrade_demo_middle.c", 157);
    wk->work3D->heapId = wk->heapId;
    func_ov194_021c4088(wk, FALSE);
    reg_G2S_DB_BLDCNT = 0;
    reg_G2_BLDCNT = 0;
    if (PokemonTrade_IsNegoType(wk)) {
        func_ov194_021c23a4(wk, 1, FALSE, PokemonTrade_GetPkm(wk, 1), FALSE);
        func_ov194_021c23a4(wk, 2, FALSE, PokemonTrade_GetPkm(wk, 0), FALSE);
        func_ov194_021c23a4(wk, 3, TRUE, PokemonTrade_GetPkm(wk, 1), FALSE);
    } else {
        func_ov194_021c24cc(wk, 1, FALSE, PokemonTrade_GetPkm(wk, 1), FALSE);
        func_ov194_021c24cc(wk, 2, FALSE, PokemonTrade_GetPkm(wk, 0), FALSE);
        func_ov194_021c24cc(wk, 3, TRUE, PokemonTrade_GetPkm(wk, 1), FALSE);
    }
    func_0201ab70(wk->mcss[0], &wk->work3D->spriteSizes[0]);
    func_0201ab70(wk->mcss[1], &wk->work3D->spriteSizes[1]);
    func_0201ab70(wk->mcss[2], &wk->work3D->spriteSizes[2]);
    func_0201ab70(wk->mcss[3], &wk->work3D->spriteSizes[3]);
    MCSS_Hide(wk->mcss[1]);
    MCSS_Hide(wk->mcss[2]);
    MCSS_Hide(wk->mcss[3]);
    func_ov194_021c1d38(wk->work3D);
    func_ov194_021c1db4(wk->work3D);
    func_ov194_021c1b74(wk->work3D, 0);
    PokemonTradeDemoMiddle_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Wait1);
}

static void PokemonTradeDemoMiddle_Wait1(PokemonTradeWork *wk) {
    func_ov194_021c1e08(wk->work3D);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Wait2);
}

static void PokemonTradeDemoMiddle_Wait2(PokemonTradeWork *wk) {
    func_ov194_021c200c(wk, 3);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Run);
}

// The scene, a step for each frame of the timeline
static void PokemonTradeDemoMiddle_Run(PokemonTradeWork *wk) {
    VecFx32 pos1;
    VecFx32 pos2;
    VecFx32 pos3;
    VecFx32 pos4;
    VecFx32 pos5;
    VecFx32 pos6;
    VecFx32 pos7;
    int index1;
    int index2;
    int index3;
    int index4;

    func_ov194_021be808(wk->timer);
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x12d)) {
        func_ov194_021c2034(wk);
        func_ov194_021c200c(wk, 4);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x2b6)) {
        func_ov194_021c2034(wk);
        func_ov194_021c200c(wk, 5);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(1)) {
        func_0201ae2c(wk->mcss[0], 8, 8, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(2)) {
        func_0201ae2c(wk->mcss[0], 16, 16, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(3)) {
        PokemonTradeDemoMiddle_LoadSprite(wk);
        PokemonTradeDemoMiddle_CreateSprite(wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(4)) {
        func_ov194_021c5ff4(wk, 0x11);
        func_ov194_021c600c(wk, 0x18);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(4)) {
        MCSS_Hide(wk->mcss[0]);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x356)) {
        func_0205007c(wk->work3D->systems[0], 10, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x65)) {
        func_0205007c(wk->work3D->systems[1], 1, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x9a)) {
        func_0205007c(wk->work3D->systems[2], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x356)) {
        func_0205007c(wk->work3D->systems[8], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0xe)) {
        func_0205007c(wk->work3D->systems[3], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x483)) {
        func_0205007c(wk->work3D->systems[7], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x9a)) {
        func_0205007c(wk->work3D->systems[4], 4, PokemonTradeDemoMiddle_AttachPath0, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x152)) {
        func_0205007c(wk->work3D->systems[5], 5, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x2bc)) {
        func_0205007c(wk->work3D->systems[6], 6, PokemonTradeDemoMiddle_AttachPath1, wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x47f)) {
        PokemonTradeDemoMiddle_CreateSprite(wk);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x10a)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemoMiddle_Frame(20), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x130)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemoMiddle_Frame(25), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x149)) {
        func_ov194_021c24dc(wk, 0);
        if (PokemonTrade_IsNegoType(wk)) {
            func_ov194_021c23a4(wk, 0, TRUE, PokemonTrade_GetPkm(wk, 0), FALSE);
        } else {
            func_ov194_021c24cc(wk, 0, TRUE, PokemonTrade_GetPkm(wk, 0), FALSE);
        }
        func_0201ab70(wk->mcss[0], &wk->work3D->spriteSizes[0]);
        MCSS_Hide(wk->mcss[0]);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x14a)) {
        index1 = 2;
        if (PokemonTrade_IsNegoType(wk)) {
            index1 = 1;
        }
        MCSS_Show(wk->mcss[index1]);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-150);
        pos1.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[index1], &pos1);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-55);
        pos1.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[index1], PokemonTradeDemoMiddle_Frame(15), &pos1, wk->heapId);
        MCSS_PauseAnimation(wk->mcss[index1]);
        pos1.x = wk->work3D->spriteSizes[index1].x * 1.2;
        pos1.y = wk->work3D->spriteSizes[index1].y * 1.2;
        pos1.z = wk->work3D->spriteSizes[index1].z;
        MCSS_SetScale(wk->mcss[index1], &pos1);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x14b)) {
        index2 = 3;
        if (PokemonTrade_IsNegoType(wk)) {
            index2 = 0;
        }
        MCSS_Show(wk->mcss[index2]);
        pos2.x = FX32_CONST(60);
        pos2.y = FX32_CONST(80);
        pos2.z = FX32_CONST(-10);
        MCSS_SetPosition(wk->mcss[index2], &pos2);
        pos2.x = FX32_CONST(60);
        pos2.y = FX32_CONST(5);
        pos2.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[index2], PokemonTradeDemoMiddle_Frame(15), &pos2, wk->heapId);
        MCSS_PauseAnimation(wk->mcss[index2]);
        pos2.x = wk->work3D->spriteSizes[index2].x * 1.2;
        pos2.y = wk->work3D->spriteSizes[index2].y * 1.2;
        pos2.z = wk->work3D->spriteSizes[index2].z;
        MCSS_SetScale(wk->mcss[index2], &pos2);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x154)) {
        pos3.x = FX32_CONST(-50);
        pos3.y = FX32_CONST(-55);
        pos3.z = FX32_CONST(10);
        TradeMcssMove_Start(wk->mcssMoves[0], PokemonTradeDemoMiddle_Frame(28), &pos3);
        pos3.x = FX32_CONST(60);
        pos3.y = FX32_CONST(5);
        pos3.z = FX32_CONST(-10);
        TradeMcssMove_Start(wk->mcssMoves[1], PokemonTradeDemoMiddle_Frame(28), &pos3);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x170)) {
        int left;
        int right;

        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        left = 2;
        right = 3;
        if (PokemonTrade_IsNegoType(wk)) {
            right = 0;
        }
        if (PokemonTrade_IsNegoType(wk)) {
            left = 1;
        }
        pos4.x = FX32_CONST(-150);
        pos4.y = FX32_CONST(-55);
        pos4.z = 0;
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[left], PokemonTradeDemoMiddle_Frame(46), &pos4, wk->heapId);
        pos4.x = FX32_CONST(150);
        pos4.y = FX32_CONST(5);
        pos4.z = 0;
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[right], PokemonTradeDemoMiddle_Frame(46), &pos4, wk->heapId);
        wk->mcssMoves[0]->bounce = 1;
        wk->mcssMoves[1]->bounce = 1;
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x1b2)) {
        index3 = 0;
        if (PokemonTrade_IsNegoType(wk)) {
            index3 = 3;
        }
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        MCSS_Show(wk->mcss[index3]);
        func_0201ae2c(wk->mcss[index3], 0, 0, 0, 0x7fff);
        pos5.x = FX32_CONST(-150);
        pos5.y = FX32_CONST(5);
        pos5.z = FX32_CONST(-10);
        MCSS_SetPosition(wk->mcss[index3], &pos5);
        pos5.x = FX32_CONST(60);
        pos5.y = FX32_CONST(5);
        pos5.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[index3], PokemonTradeDemoMiddle_Frame(103), &pos5, wk->heapId);
        pos5.x = wk->work3D->spriteSizes[index3].x * 1.2;
        pos5.y = wk->work3D->spriteSizes[index3].y * 1.2;
        pos5.z = wk->work3D->spriteSizes[index3].z;
        MCSS_SetScale(wk->mcss[index3], &pos5);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x1b2)) {
        index4 = 1;
        if (PokemonTrade_IsNegoType(wk)) {
            index4 = 2;
        }
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        MCSS_Show(wk->mcss[index4]);
        pos6.x = FX32_CONST(150);
        pos6.y = FX32_CONST(-55);
        pos6.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[index4], &pos6);
        pos6.x = FX32_CONST(-45);
        pos6.y = FX32_CONST(-55);
        pos6.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[index4], PokemonTradeDemoMiddle_Frame(103), &pos6, wk->heapId);
        pos6.x = wk->work3D->spriteSizes[index4].x * 1.2;
        pos6.y = wk->work3D->spriteSizes[index4].y * 1.2;
        pos6.z = wk->work3D->spriteSizes[index4].z;
        MCSS_SetScale(wk->mcss[index4], &pos6);
        wk->mcssMoves[0]->bounce = 1;
        wk->mcssMoves[1]->bounce = 1;
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x1f8)) {
        TradeMcssMove_Start(wk->mcssMoves[0], PokemonTradeDemoMiddle_Frame(128), &wk->mcssMoves[0]->end);
        TradeMcssMove_Start(wk->mcssMoves[1], PokemonTradeDemoMiddle_Frame(128), &wk->mcssMoves[1]->end);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x27d)) {
        int left = 1;
        int right = 0;

        if (PokemonTrade_IsNegoType(wk)) {
            right = 3;
        }
        if (PokemonTrade_IsNegoType(wk)) {
            left = 2;
        }
        PokemonTradeDemoMiddle_FreeMove(wk, 0);
        PokemonTradeDemoMiddle_FreeMove(wk, 1);
        pos7.x = FX32_CONST(60);
        pos7.y = FX32_CONST(100);
        pos7.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[right], PokemonTradeDemoMiddle_Frame(7), &pos7, wk->heapId);
        pos7.x = FX32_CONST(-45);
        pos7.y = FX32_CONST(-160);
        pos7.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[left], PokemonTradeDemoMiddle_Frame(7), &pos7, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x2b5)) {
        MCSS_Hide(wk->mcss[0]);
        MCSS_Hide(wk->mcss[1]);
        MCSS_Hide(wk->mcss[2]);
        MCSS_Hide(wk->mcss[3]);
    }
    u32 pair[4] = { 2, 3, 0, 1 };
    int index = 3;
    int other;
    if (PokemonTrade_IsNegoType(wk)) {
        index = 0;
    }
    other = pair[index];
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x48e)) {
        if (wk->unk11EE == 0) {
            PokemonTradeDemoMiddle_ShowSprite(wk, index);
            PokemonTradeDemoMiddle_StartMove(wk, index);
            func_0201ae2c(wk->mcss[index], 16, 16, 0, 0x7fff);
        } else {
            PokemonTradeDemoMiddle_ShowSprite(wk, index);
            MCSS_Hide(wk->mcss[index]);
            PokemonTradeDemoMiddle_ShowSprite(wk, other);
            PokemonTradeDemoMiddle_StartMove(wk, index);
            func_0201ae2c(wk->mcss[other], 16, 16, 0, 0x7fff);
        }
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x48f)) {
        func_0201ae2c(wk->mcss[index], 8, 8, 0, 0x7fff);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x490)) {
        func_0201ae2c(wk->mcss[index], 0, 0, 0, 0x7fff);
    }
    if (PokemonTradeDemoMiddle_Frame(0x48e) < wk->timer && wk->unk11EE != 0 && wk->mcssMoves[3] != NULL
        && wk->mcssMoves[3]->frame == 9) {
        VecFx32 pos8;

        MCSS_GetPosition(wk->mcss[other], &pos8);
        MCSS_SetPosition(wk->mcss[index], &pos8);
        MCSS_Show(wk->mcss[index]);
        MCSS_Hide(wk->mcss[other]);
        wk->mcssMoves[3]->mcss = wk->mcss[index];
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x4a0)) {
        MCSS_ResumeAnimation(wk->mcss[index]);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x28b)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemoMiddle_Frame(10), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x2b8)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemoMiddle_Frame(25), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x380)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemoMiddle_Frame(30), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x3ca)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemoMiddle_Frame(30), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoMiddle_Frame(0x4c1)) {
        PokemonTradeDemoMiddle_FreeMove(wk, 0);
        PokemonTradeDemoMiddle_FreeMove(wk, 1);
        func_ov194_021c2034(wk);
        PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_End);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
    TradeMcssMove_Update(wk->mcssMoves[2], wk);
    TradeMcssMove_Update(wk->mcssMoves[3], wk);
}
