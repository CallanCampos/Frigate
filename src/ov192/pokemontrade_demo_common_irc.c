#include "types.h"
#include "app/pokemon_trade_local.h"
#include "app/pokemontrade_ircdemo.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/mcss.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_3.h"

// pokemontrade_demo_common.c as the ROM's string has it, the part of the IR trade's demo that its steps share. Overlay
// 193's trade demos are built from the same template; this overlay's copy is the file's first half, and
// pokemontrade_ircdemo.c is the second. Names are descriptive guesses. Overlay 193 also embeds
// "pokemontrade_demo_common.c", for a file of other code (src/ov193/pokemontrade_demo_common.c), and the build can't
// link two files of one name, so this one is named _irc after its overlay's other file

static const VecFx32 sIrcDemoPosSecond = { 0, -0x17000, -0xa000 };
static const VecFx32 sIrcDemoPosEnd = { 0, -0x17000, 0xa000 };
static const VecFx32 sIrcDemoPosFirst = { 0, -0x17000, 0xa000 };

static void PokemonTradeIrcDemo_UpdateFade(TradeFade *fade);
static void PokemonTradeIrcDemo_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
                                         BOOL partner);
static void PokemonTradeIrcDemo_FreeSprite(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_StartBgmTimer(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_StateWaitDemo(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_StateWaitSync(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_StateSetUpMoves(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_StateMoves(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_ShowMessage(PokemonTradeWork *wk);
static void PokemonTradeIrcDemo_StateMessage(PokemonTradeWork *wk);

// The hops of a Pokémon's sprite, as offsets from its place: one hop of 18 frames, the same three frames later, or
// two of 40
static VecFx32 sIrcDemoHopPath[18] = {
    { 0, 0, 0 },     { 0, 15901, 0 }, { 0, 30720, 0 }, { 0, 43444, 0 }, { 0, 53208, 0 }, { 0, 59346, 0 },
    { 0, 61440, 0 }, { 0, 59346, 0 }, { 0, 53208, 0 }, { 0, 43444, 0 }, { 0, 30720, 0 }, { 0, 15901, 0 },
    { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },
};

static VecFx32 sIrcDemoDelayedHopPath[18] = {
    { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },     { 0, 15901, 0 }, { 0, 30720, 0 },
    { 0, 43444, 0 }, { 0, 53208, 0 }, { 0, 59346, 0 }, { 0, 61440, 0 }, { 0, 59346, 0 }, { 0, 53208, 0 },
    { 0, 43444, 0 }, { 0, 30720, 0 }, { 0, 15901, 0 }, { 0, 0, 0 },     { 0, 0, 0 },     { 0, 0, 0 },
};

static VecFx32 sIrcDemoLongHopPath[40] = {
    { 0, 0, 0 },     { 0, 10601, 0 }, { 0, 20480, 0 }, { 0, 28963, 0 }, { 0, 35472, 0 }, { 0, 39564, 0 },
    { 0, 40960, 0 }, { 0, 39564, 0 }, { 0, 35472, 0 }, { 0, 28963, 0 }, { 0, 20480, 0 }, { 0, 10601, 0 },
    { 0, 0, 0 },     { 0, 0, 0 },     { 0, 10601, 0 }, { 0, 20480, 0 }, { 0, 28963, 0 }, { 0, 35472, 0 },
    { 0, 39564, 0 }, { 0, 40960, 0 }, { 0, 39564, 0 }, { 0, 35472, 0 }, { 0, 28963, 0 }, { 0, 20480, 0 },
    { 0, 10601, 0 }, { 0, 0, 0 },     { 0, 0, 0 },     { 0, 10601, 0 }, { 0, 20480, 0 }, { 0, 28963, 0 },
    { 0, 35472, 0 }, { 0, 39564, 0 }, { 0, 40960, 0 }, { 0, 39564, 0 }, { 0, 35472, 0 }, { 0, 28963, 0 },
    { 0, 20480, 0 }, { 0, 10601, 0 }, { 0, 0, 0 },     { 0, 0, 0 },
};

// The path that the sprites follow in the other scenes, which they fill
static VecFx32 sIrcDemoScenePath[20];

static void PokemonTradeIrcDemo_UpdateFade(TradeFade *fade) {
    if (fade != NULL && fade->frames >= fade->frame) {
        fx32 step;
        int brightness;

        fade->frame++;
        step = ((fade->end - fade->start) << FX32_SHIFT) / fade->frames;
        brightness = step * fade->frame / FX32_ONE;
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 0x1e, brightness);
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x1f, brightness);
    }
}

static void PokemonTradeIrcDemo_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
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

void PokemonTradeIrcDemo_ShowActor(PokemonTradeWork *wk) {
    if (wk->work3D->actor == NULL) {
        ClActorSetup setup;

        setup.x = 128;
        setup.y = 99;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 0;
        wk->work3D->actor =
            ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_DEMO], wk->objRes[TRADE_OBJRES_PLTT_DEMO],
                           wk->objRes[TRADE_OBJRES_CELL_DEMO], &setup, 0, wk->heapId);
        ClActor_SetAutoAnim(wk->work3D->actor, TRUE);
        ClActor_SetVisible(wk->work3D->actor, TRUE);
    } else {
        ClActor_RestartAnim(wk->work3D->actor);
    }
}

void PokemonTradeIrcDemo_LoadResources(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE_DEMO, wk->heapId);

    wk->objRes[TRADE_OBJRES_CHAR_DEMO] = ClActSys_LoadChars(arc, 2, FALSE, CLACT_VRAM_MAIN, wk->heapId);
    wk->objRes[TRADE_OBJRES_PLTT_DEMO] = ClActSys_LoadPlttRange(arc, 3, CLACT_VRAM_MAIN, 0xe0, 0, 1, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_DEMO] = ClActSys_LoadCellAnims(arc, 1, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->work3D->actor = NULL;
}

static void PokemonTradeIrcDemo_FreeSprite(PokemonTradeWork *wk) {
    if (wk->work3D->actor != NULL) {
        ClActor_Delete(wk->work3D->actor);
    }
    if (wk->objRes[TRADE_OBJRES_CHAR_DEMO] != 0) {
        ClActSys_FreeChars(wk->objRes[TRADE_OBJRES_CHAR_DEMO]);
        ClActSys_FreePltt(wk->objRes[TRADE_OBJRES_PLTT_DEMO]);
        ClActSys_FreeCellAnims(wk->objRes[TRADE_OBJRES_CELL_DEMO]);
    }
    wk->objRes[TRADE_OBJRES_CHAR_DEMO] = 0;
    wk->objRes[TRADE_OBJRES_PLTT_DEMO] = 0;
    wk->objRes[TRADE_OBJRES_CELL_DEMO] = 0;
    wk->work3D->actor = NULL;
}

int PokemonTradeIrcDemo_Frames(int frame) {
    return frame;
}

void PokemonTradeIrcDemo_SetTimer(PokemonTradeWork *wk, int timer) {
    wk->timer = timer;
}

static void PokemonTradeIrcDemo_StartBgmTimer(PokemonTradeWork *wk) {
    wk->bgmTimer = 1;
}

static void PokemonTradeIrcDemo_StateWaitDemo(PokemonTradeWork *wk) {
    func_ov194_021c29b4(wk);
    if (wk->unk854 == 1) {
        PokemonTradeIrcDemo_InitSide(wk, &wk->unk11ED, &wk->unk11EF, wk->partnerInfo, TRUE);
        PokemonTradeIrcDemo_InitSide(wk, &wk->unk11EE, &wk->unk11F0[0], wk->myInfo, FALSE);
        func_ov194_021c2034(wk);
        if (PokemonTrade_IsNetwork(wk)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x11, 8);
            PokemonTrade_SetState(wk, PokemonTradeIrcDemo_StateWaitSync);
        } else {
            PokemonTrade_SetState(wk, PokemonTradeIrcDemo_StateSetUpMoves);
        }
    }
}

static void PokemonTradeIrcDemo_StateWaitSync(PokemonTradeWork *wk) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x11, 8)) {
        PokemonTrade_SetState(wk, PokemonTradeIrcDemo_StateSetUpMoves);
    }
}

static void PokemonTradeIrcDemo_StateSetUpMoves(PokemonTradeWork *wk) {
    VecFx32 first;
    VecFx32 second;

    PokemonTradeIrcDemo_SetTimer(wk, 0);
    wk->fade = GFL_HeapAllocate(wk->heapId, sizeof(TradeFade), TRUE, "pokemontrade_demo_common.c", 246);
    wk->fade->frames = PokemonTradeIrcDemo_Frames(14);
    wk->fade->frame = 0;
    wk->fade->start = 0;
    wk->fade->end = -16;
    first = sIrcDemoPosFirst;
    TradeMcssMove_Free(wk->mcssMoves[0]);
    wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeIrcDemo_Frames(42), &first, wk->heapId);
    second = sIrcDemoPosSecond;
    TradeMcssMove_Free(wk->mcssMoves[1]);
    wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeIrcDemo_Frames(84), &second, wk->heapId);
    MCSS_SetAnimationEndCallback(wk->mcss[0], (u32)wk, (MCSSAnimationCallback)func_ov194_021beab4, TRUE);
    MCSS_StartFade(wk->mcss[1], 0, 16, (s8)(PokemonTradeIrcDemo_Frames(14) / 16), 0);
    func_ov194_021bfe70(wk);
    GFL_BGSysResetStdPalette(1, 0);
    GFL_BGSysResetStdPalette(5, 0);
    PokemonTradeIrcDemo_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeIrcDemo_StateMoves);
}

static void PokemonTradeIrcDemo_StateMoves(PokemonTradeWork *wk) {
    PokemonTradeIrcDemo_UpdateFade(wk->fade);
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
    if (wk->timer > PokemonTradeIrcDemo_Frames(42)) {
        PartyPkm *pkm;

        func_ov194_021c5ff4(wk, 0x11);
        func_ov194_021c600c(wk, 0);
        func_ov194_021c52bc(wk);
        func_ov194_021c368c(wk);
        func_ov194_021c1484(wk);
        PokemonTradeIrcDemo_SetTimer(wk, 0);
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
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 20, &pos, sIrcDemoScenePath, wk->heapId);
            } else {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 18, &pos, sIrcDemoHopPath, wk->heapId);
            }
            func_ov194_021c24cc(wk, 2, FALSE, PokemonTrade_GetPkm(wk, PokemonTrade_IsNegoType(wk)), TRUE);
            MCSS_PauseAnimation(wk->mcss[2]);
            MCSS_Hide(wk->mcss[2]);
        } else {
            VecFx32 pos;

            MCSS_GetPosition(wk->mcss[0], &pos);
            TradeMcssMove_Free(wk->mcssMoves[0]);
            if (wk->unk11EF != 0) {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 20, &pos, sIrcDemoScenePath, wk->heapId);
            } else {
                wk->mcssMoves[0] = TradeMcssMove_CreateWithPath(wk->mcss[0], 40, &pos, sIrcDemoLongHopPath, wk->heapId);
            }
        }
        PokemonTrade_SetState(wk, PokemonTradeIrcDemo_StateMessage);
    }
}

static void PokemonTradeIrcDemo_ShowMessage(PokemonTradeWork *wk) {
    GFL_NetSetIconScreen(FALSE, wk->heapId);
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

static void PokemonTradeIrcDemo_StateMessage(PokemonTradeWork *wk) {
    if (wk->timer == 10) {
        PokemonTradeIrcDemo_ShowMessage(wk);
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
        PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Init);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
}

void PokemonTradeIrcDemo_Finish(PokemonTradeWork *wk) {
    PokemonTradeIrcDemo_FreeSprite(wk);
    func_ov194_021c1e38(wk->work3D, 6);
    func_ov194_021c1d30(wk->work3D);
    GFL_HeapFree(wk->work3D);
    wk->work3D = NULL;
    PokemonTrade_SetState(wk, func_ov194_021beb48);
}

// Shows a sprite still, in front of the scene, and scales it by its size
void PokemonTradeIrcDemo_ShowSprite(MCSS *mcss, PokemonTradeWork *wk) {
    VecFx32 pos;

    MCSS_SetAlpha(mcss, 31);
    MCSS_Show(mcss);
    pos.x = 0;
    pos.y = FX32_CONST(-23);
    pos.z = FX32_CONST(10);
    MCSS_SetPosition(mcss, &pos);
    MCSS_PauseAnimation(mcss);
    MCSS_SetScale(mcss, &wk->work3D->ircSpriteSizes[0]);
    MCSS_StartFade(mcss, 16, 16, 0, 0x7fff);
}

// Plays the voice of the other machine's Pokémon and makes a sprite hop or bounce where it is
void PokemonTradeIrcDemo_StartMove(PokemonTradeWork *wk, int index) {
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
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(wk->mcss[index], 20, &pos, sIrcDemoScenePath, wk->heapId);
        } else {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(wk->mcss[index], 40, &pos, sIrcDemoLongHopPath, wk->heapId);
        }
    } else {
        u32 pair[4] = { 2, 3, 0, 1 };
        MCSS *target = wk->mcss[pair[index]];

        MCSS_GetPosition(wk->mcss[index], &pos);
        TradeMcssMove_Free(wk->mcssMoves[3]);
        if (wk->unk11F0[0] != 0) {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(target, 20, &pos, sIrcDemoScenePath, wk->heapId);
        } else {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(target, 18, &pos, sIrcDemoDelayedHopPath, wk->heapId);
        }
    }
}

void PokemonTradeIrcDemo_Start(PokemonTradeWork *wk) {
    PokemonTradeIrcDemo_StartBgmTimer(wk);
    wk->unk854 = TRUE;
    MCSS_SetAnimation(wk->mcss[0], 0);
    MCSS_PauseAnimation(wk->mcss[0]);
    if (wk->type == 4) {
        VecFx32 pos = sIrcDemoPosEnd;

        MCSS_SetPosition(wk->mcss[0], &pos);
    }
    PokemonTradeIrcDemo_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeIrcDemo_StateWaitDemo);
}
