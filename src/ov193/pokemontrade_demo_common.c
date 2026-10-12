#include "types.h"
#include "app/pokemontrade_demo_common.h"
#include "app/pokemontrade_demo_up.h"
#include "app/pokemon_trade_local.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/mcss.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_3.h"

// A guessed name: the part of the trade demo's scenes that the three overlay-193 files share

static const VecFx32 sTradeDemoCommonPosFirst = { 0, -0x17000, 0xa000 };
static const VecFx32 sTradeDemoCommonPosEnd = { 0, -0x17000, 0xa000 };
static const VecFx32 sTradeDemoCommonPosSecond = { 0, -0x17000, -0xa000 };

static void PokemonTradeDemoCommon_UpdateFade(TradeFade *fade);
static void PokemonTradeDemoCommon_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
                                            BOOL partner);
static void PokemonTradeDemoCommon_FreeSprite(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_StartBgmTimer(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_StateWaitDemo(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_StateWaitSync(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_StateSetUpMoves(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_StateMoves(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_ShowMessage(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_StateMessage(PokemonTradeWork *wk);
static void PokemonTradeDemoCommon_FollowPath0(SPLEmitter *emitter, u32 type);
static void PokemonTradeDemoCommon_FollowPath1(SPLEmitter *emitter, u32 type);

// The hops of a Pokémon's sprite, as offsets from its place: one hop of 18 frames, or two of 40
static VecFx32 sTradeDemoCommonHopPath[18] = {
    { 0, 0, 0 },         { 0, 15901, 0 },     { 0, 30720, 0 },     { 0, 43444, 0 },     { 0, 53208, 0 },
    { 0, 59346, 0 },     { 0, 61440, 0 },     { 0, 59346, 0 },     { 0, 53208, 0 },     { 0, 43444, 0 },
    { 0, 30720, 0 },     { 0, 15901, 0 },     { 0, 0, 0 },         { 0, 0, 0 },         { 0, 0, 0 },
    { 0, 0, 0 },         { 0, 0, 0 },         { 0, 0, 0 },
};

static VecFx32 sTradeDemoCommonLongHopPath[40] = {
    { 0, 0, 0 },     { 0, 10601, 0 }, { 0, 20480, 0 }, { 0, 28963, 0 }, { 0, 35472, 0 }, { 0, 39564, 0 },
    { 0, 40960, 0 }, { 0, 39564, 0 }, { 0, 35472, 0 }, { 0, 28963, 0 }, { 0, 20480, 0 }, { 0, 10601, 0 },
    { 0, 0, 0 },     { 0, 0, 0 },     { 0, 10601, 0 }, { 0, 20480, 0 }, { 0, 28963, 0 }, { 0, 35472, 0 },
    { 0, 39564, 0 }, { 0, 40960, 0 }, { 0, 39564, 0 }, { 0, 35472, 0 }, { 0, 28963, 0 }, { 0, 20480, 0 },
    { 0, 10601, 0 }, { 0, 0, 0 },     { 0, 0, 0 },     { 0, 10601, 0 }, { 0, 20480, 0 }, { 0, 28963, 0 },
    { 0, 35472, 0 }, { 0, 39564, 0 }, { 0, 40960, 0 }, { 0, 39564, 0 }, { 0, 35472, 0 }, { 0, 28963, 0 },
    { 0, 20480, 0 }, { 0, 10601, 0 }, { 0, 0, 0 },     { 0, 0, 0 },
};

// The path that the sprites follow in the other scenes, which they fill
static VecFx32 sTradeDemoCommonScenePath[20];

static void PokemonTradeDemoCommon_UpdateFade(TradeFade *fade) {
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

// The side of the trade whose Pokémon is shown as the partner's or the player's: a nego trade swaps them
static inline int PokemonTradeDemoCommon_GetSide(PokemonTradeWork *wk, BOOL partner) {
    if (partner) {
        if (PokemonTrade_IsNegoType(wk)) {
            return 1;
        }
        return 0;
    }
    if (PokemonTrade_IsNegoType(wk)) {
        return 0;
    }
    return 1;
}

// Works out, for a side of the trade, whether its Pokémon was caught by the player and whether its sprite doesn't hop
static void PokemonTradeDemoCommon_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
                                            BOOL partner) {
    PartyPkm *pkm;
    u32 species;
    u32 form;
    BOOL isEgg;
    u32 noHop;

    pkm = PokemonTrade_GetPkm(wk, PokemonTradeDemoCommon_GetSide(wk, partner));
    PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
    species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (isEgg == FALSE) {
        void *personal = PML_PersonalLoad(species, form, wk->heapId);

        noHop = PML_PersonalGetParam(personal, PERSONAL_NO_BOUNCE);
        PML_PersonalFree(personal);
    }
    if (isEgg) {
        *noBounce = TRUE;
    } else if (noHop) {
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

void PokemonTradeDemo_ShowActor(PokemonTradeWork *wk) {
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

void PokemonTradeDemo_LoadResources(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0x68, wk->heapId);

    wk->objRes[TRADE_OBJRES_CHAR_DEMO] = func_0204b81c(arc, 2, FALSE, CLACT_VRAM_MAIN, wk->heapId);
    wk->objRes[TRADE_OBJRES_PLTT_DEMO] = func_0204bbb8(arc, 3, CLACT_VRAM_MAIN, 0xe0, 0, 1, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_DEMO] = func_0204bde0(arc, 1, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->work3D->actor = NULL;
}

static void PokemonTradeDemoCommon_FreeSprite(PokemonTradeWork *wk) {
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

int PokemonTradeDemo_Frames(int frame) {
    return frame;
}

void PokemonTradeDemo_SetTimer(PokemonTradeWork *wk, int timer) {
    wk->timer = timer;
}

static void PokemonTradeDemoCommon_StartBgmTimer(PokemonTradeWork *wk) {
    wk->bgmTimer = 1;
}

static void PokemonTradeDemoCommon_StateWaitDemo(PokemonTradeWork *wk) {
    func_ov194_021c29b4(wk);
    if (wk->unk854 == 1) {
        PokemonTradeDemoCommon_InitSide(wk, &wk->unk11ED, &wk->unk11EF, wk->partnerInfo, TRUE);
        PokemonTradeDemoCommon_InitSide(wk, &wk->unk11EE, &wk->unk11F0[0], wk->myInfo, FALSE);
        func_ov194_021c2034(wk);
        if (PokemonTrade_IsNetwork(wk)) {
            func_02040624(func_02040440(), 0x11, 8);
            PokemonTrade_SetState(wk, PokemonTradeDemoCommon_StateWaitSync);
        } else {
            PokemonTrade_SetState(wk, PokemonTradeDemoCommon_StateSetUpMoves);
        }
    }
}

static void PokemonTradeDemoCommon_StateWaitSync(PokemonTradeWork *wk) {
    if (func_02040664(func_02040440(), 0x11, 8)) {
        PokemonTrade_SetState(wk, PokemonTradeDemoCommon_StateSetUpMoves);
    }
}

static void PokemonTradeDemoCommon_StateSetUpMoves(PokemonTradeWork *wk) {
    VecFx32 first;
    VecFx32 second;

    PokemonTradeDemo_SetTimer(wk, 0);
    wk->fade = GFL_HeapAllocate(wk->heapId, sizeof(TradeFade), TRUE, "pokemontrade_demo_common.c", 246);
    wk->fade->frames = PokemonTradeDemo_Frames(14);
    wk->fade->frame = 0;
    wk->fade->start = 0;
    wk->fade->end = -16;
    first = sTradeDemoCommonPosFirst;
    TradeMcssMove_Free(wk->mcssMoves[0]);
    wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeDemo_Frames(42), &first, wk->heapId);
    second = sTradeDemoCommonPosSecond;
    TradeMcssMove_Free(wk->mcssMoves[1]);
    wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeDemo_Frames(84), &second, wk->heapId);
    MCSS_SetAnimationEndCallback(wk->mcss[0], (u32)wk, (MCSSAnimationCallback)func_ov194_021beab4, TRUE);
    func_0201ae2c(wk->mcss[1], 0, 16, (s8)(PokemonTradeDemo_Frames(14) / 16), 0);
    func_ov194_021bfe70(wk);
    GFL_BGSysResetStdPalette(1, 0);
    GFL_BGSysResetStdPalette(5, 0);
    PokemonTradeDemo_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeDemoCommon_StateMoves);
}

static void PokemonTradeDemoCommon_StateMoves(PokemonTradeWork *wk) {
    PokemonTradeDemoCommon_UpdateFade(wk->fade);
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
    if (wk->timer > PokemonTradeDemo_Frames(42)) {
        PartyPkm *pkm;

        func_ov194_021c5ff4(wk, 0x11);
        func_ov194_021c600c(wk, 0);
        func_ov194_021c52bc(wk);
        func_ov194_021c368c(wk);
        func_ov194_021c1484(wk);
        PokemonTradeDemo_SetTimer(wk, 0);
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
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 20, &pos, sTradeDemoCommonScenePath, wk->heapId);
            } else {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 18, &pos, sTradeDemoCommonHopPath, wk->heapId);
            }
            func_ov194_021c24cc(wk, 2, FALSE, PokemonTrade_GetPkm(wk, PokemonTrade_IsNegoType(wk)), TRUE);
            MCSS_PauseAnimation(wk->mcss[2]);
            MCSS_Hide(wk->mcss[2]);
        } else {
            VecFx32 pos;

            MCSS_GetPosition(wk->mcss[0], &pos);
            TradeMcssMove_Free(wk->mcssMoves[0]);
            if (wk->unk11EF != 0) {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 20, &pos, sTradeDemoCommonScenePath, wk->heapId);
            } else {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 40, &pos, sTradeDemoCommonLongHopPath, wk->heapId);
            }
        }
        PokemonTrade_SetState(wk, PokemonTradeDemoCommon_StateMessage);
    }
}

static void PokemonTradeDemoCommon_ShowMessage(PokemonTradeWork *wk) {
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

static void PokemonTradeDemoCommon_StateMessage(PokemonTradeWork *wk) {
    if (wk->timer == 10) {
        PokemonTradeDemoCommon_ShowMessage(wk);
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
        PokemonTrade_SetState(wk, PokemonTradeDemoUp_Start);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
}

void PokemonTradeDemo_Finish(PokemonTradeWork *wk) {
    PokemonTradeDemoCommon_FreeSprite(wk);
    func_ov194_021c1e38(wk->work3D, 6);
    func_ov194_021c1d30(wk->work3D);
    GFL_HeapFree(wk->work3D);
    wk->work3D = NULL;
    PokemonTrade_SetState(wk, func_ov194_021beb48);
}

static void PokemonTradeDemoCommon_FollowPath0(SPLEmitter *emitter, u32 type) {
    PokemonTradeWork *wk;

    if (type != 0 && emitter != NULL) {
        wk = func_02050364(emitter);
        if (wk->work3D->emitterPaths[0] != NULL) {
            VecFx32 pos = { 0, 0, 0 };

            GFL_G3DCurveGetNowTranslation(wk->work3D->emitterPaths[0], &pos);
            func_02050208(emitter, &pos);
        }
    }
}

void PokemonTradeDemo_InitEmitter0(SPLEmitter *emitter) {
    func_0205035c(emitter, func_02050188());
    func_02050354(emitter, PokemonTradeDemoCommon_FollowPath0);
}

static void PokemonTradeDemoCommon_FollowPath1(SPLEmitter *emitter, u32 type) {
    PokemonTradeWork *wk;

    if (type != 0 && emitter != NULL) {
        wk = func_02050364(emitter);
        if (wk->work3D->emitterPaths[1] != NULL) {
            VecFx32 pos = { 0, 0, 0 };

            GFL_G3DCurveGetNowTranslation(wk->work3D->emitterPaths[1], &pos);
            func_02050208(emitter, &pos);
        }
    }
}

void PokemonTradeDemo_InitEmitter1(SPLEmitter *emitter) {
    func_0205035c(emitter, func_02050188());
    func_02050354(emitter, PokemonTradeDemoCommon_FollowPath1);
}

void PokemonTradeDemoCommon_Start(PokemonTradeWork *wk) {
    VecFx32 pos;

    PokemonTradeDemoCommon_StartBgmTimer(wk);
    wk->unk854 = 1;
    MCSS_SetAnimation(wk->mcss[0], 0);
    if (wk->type == 6) {
        pos = sTradeDemoCommonPosEnd;
        MCSS_SetPosition(wk->mcss[0], &pos);
    }
    PokemonTradeDemo_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeDemoCommon_StateWaitDemo);
}
