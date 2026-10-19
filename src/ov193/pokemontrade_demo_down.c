#include "types.h"
#include "app/pokemon_trade_local.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/mcss.h"
#include "system/wipe.h"

// pokemontrade_demo_down.c, a guessed name: one of the trade demo's three scenes, a copy of the template that
// pokemontrade_demo_common.c and pokemontrade_demo_up.c share, whose sprites fly by in front of the effects

// The scene's frame counter starts at this frame
#define DOWN_START_FRAME 0x2b5

static int PokemonTradeDemoDown_Frame(int frame);
static void PokemonTradeDemoDown_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
                                          BOOL partner);
static void PokemonTradeDemoDown_CreateActor(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_LoadResources(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_FreeSprite(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_SetTimer(PokemonTradeWork *wk, int timer);
static void PokemonTradeDemoDown_StartBgmTimer(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_End(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_FreeMove(PokemonTradeWork *wk, int index);
static void PokemonTradeDemoDown_StartMove(PokemonTradeWork *wk, int index);
static void PokemonTradeDemoDown_ShowSprite(PokemonTradeWork *wk, int index);
static void PokemonTradeDemoDown_FollowEmitterPath0(SPLEmitter *emitter, u32 type);
static void PokemonTradeDemoDown_AttachEmitterPath0(SPLEmitter *emitter);
static void PokemonTradeDemoDown_FollowEmitterPath1(SPLEmitter *emitter, u32 type);
static void PokemonTradeDemoDown_AttachEmitterPath1(SPLEmitter *emitter);
static void PokemonTradeDemoDown_Init(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_Wait1(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_Wait2(PokemonTradeWork *wk);
static void PokemonTradeDemoDown_Run(PokemonTradeWork *wk);

// The hops of a Pokémon's sprite, as offsets from its place: one hop of 18 frames, or two of 40. The common file has
// the same
// clang-format off
static VecFx32 sTradeDemoDownHopPath[18] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0x3e1d, 0 },
    { 0, 0x7800, 0 },
    { 0, 0xa9b4, 0 },
    { 0, 0xcfd8, 0 },
    { 0, 0xe7d2, 0 },
    { 0, 0xf000, 0 },
    { 0, 0xe7d2, 0 },
    { 0, 0xcfd8, 0 },
    { 0, 0xa9b4, 0 },
    { 0, 0x7800, 0 },
    { 0, 0x3e1d, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
};

// clang-format on

static VecFx32 sTradeDemoDownLongHopPath[40] = {
    { 0, 0, 0 },      { 0, 0x2969, 0 }, { 0, 0x5000, 0 }, { 0, 0x7123, 0 }, { 0, 0x8a90, 0 }, { 0, 0x9a8c, 0 },
    { 0, 0xa000, 0 }, { 0, 0x9a8c, 0 }, { 0, 0x8a90, 0 }, { 0, 0x7123, 0 }, { 0, 0x5000, 0 }, { 0, 0x2969, 0 },
    { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0x2969, 0 }, { 0, 0x5000, 0 }, { 0, 0x7123, 0 }, { 0, 0x8a90, 0 },
    { 0, 0x9a8c, 0 }, { 0, 0xa000, 0 }, { 0, 0x9a8c, 0 }, { 0, 0x8a90, 0 }, { 0, 0x7123, 0 }, { 0, 0x5000, 0 },
    { 0, 0x2969, 0 }, { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0x2969, 0 }, { 0, 0x5000, 0 }, { 0, 0x7123, 0 },
    { 0, 0x8a90, 0 }, { 0, 0x9a8c, 0 }, { 0, 0xa000, 0 }, { 0, 0x9a8c, 0 }, { 0, 0x8a90, 0 }, { 0, 0x7123, 0 },
    { 0, 0x5000, 0 }, { 0, 0x2969, 0 }, { 0, 0, 0 },      { 0, 0, 0 },
};

// The path that the sprites follow when they don't hop, which nothing fills
static VecFx32 sTradeDemoDownScenePath[20];

// Whether the Pokémon of a side belongs to a trainer, and whether it is an egg or doesn't bounce
static void PokemonTradeDemoDown_InitSide(PokemonTradeWork *wk, u8 *ownedByPlayer, u8 *noBounce, PlayerInfo *player,
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
static void PokemonTradeDemoDown_CreateActor(PokemonTradeWork *wk) {
    if (wk->work3D->actor == NULL) {
        ClActorSetup setup;

        setup.x = 128;
        setup.y = 99;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 0;
        wk->work3D->actor =
            func_0204c040(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_DEMO], wk->objRes[TRADE_OBJRES_PLTT_DEMO],
                          wk->objRes[TRADE_OBJRES_CELL_DEMO], &setup, 0, wk->heapId);
        func_0204c520(wk->work3D->actor, TRUE);
        func_0204c124(wk->work3D->actor, TRUE);
    } else {
        func_0204c56c(wk->work3D->actor);
    }
}

// Loads the sprite's resources
static void PokemonTradeDemoDown_LoadResources(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE_DEMO, wk->heapId);

    wk->objRes[TRADE_OBJRES_CHAR_DEMO] = func_0204b81c(arc, 2, FALSE, 0, wk->heapId);
    wk->objRes[TRADE_OBJRES_PLTT_DEMO] = func_0204bbb8(arc, 3, 0, 0xe0, 0, 1, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_DEMO] = func_0204bde0(arc, 1, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->work3D->actor = NULL;
}

static void PokemonTradeDemoDown_FreeSprite(PokemonTradeWork *wk) {
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
static int PokemonTradeDemoDown_Frame(int frame) {
    return frame;
}

static void PokemonTradeDemoDown_SetTimer(PokemonTradeWork *wk, int timer) {
    wk->timer = timer;
}

static void PokemonTradeDemoDown_StartBgmTimer(PokemonTradeWork *wk) {
    wk->bgmTimer = 1;
}

// Frees the effects and goes on to the save
static void PokemonTradeDemoDown_End(PokemonTradeWork *wk) {
    PokemonTradeDemoDown_FreeSprite(wk);
    func_ov194_021c1e38(wk->work3D, 6);
    func_ov194_021c1d30(wk->work3D);
    GFL_HeapFree(wk->work3D);
    wk->work3D = NULL;
    PokemonTrade_SetState(wk, func_ov194_021beb48);
}

static void PokemonTradeDemoDown_FreeMove(PokemonTradeWork *wk, int index) {
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
static void PokemonTradeDemoDown_StartMove(PokemonTradeWork *wk, int index) {
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
            wk->mcssMoves[3] =
                TradeMcssMove_CreateWithPath(wk->mcss[index], 20, &pos, sTradeDemoDownScenePath, wk->heapId);
        } else {
            wk->mcssMoves[3] =
                TradeMcssMove_CreateWithPath(wk->mcss[index], 40, &pos, sTradeDemoDownLongHopPath, wk->heapId);
        }
    } else {
        u32 pair[4] = { 2, 3, 0, 1 };
        MCSS *target = wk->mcss[pair[index]];

        MCSS_GetPosition(wk->mcss[index], &pos);
        TradeMcssMove_Free(wk->mcssMoves[3]);
        if (wk->unk11F0[0] != 0) {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(target, 20, &pos, sTradeDemoDownScenePath, wk->heapId);
        } else {
            wk->mcssMoves[3] = TradeMcssMove_CreateWithPath(target, 18, &pos, sTradeDemoDownHopPath, wk->heapId);
        }
    }
}

// Shows a sprite still, at the start of the scene, and scales it by its size
static void PokemonTradeDemoDown_ShowSprite(PokemonTradeWork *wk, int index) {
    VecFx32 pos;

    MCSS_Show(wk->mcss[index]);
    pos.x = 0;
    pos.y = FX32_CONST(-23);
    pos.z = FX32_CONST(10);
    MCSS_SetPosition(wk->mcss[index], &pos);
    MCSS_PauseAnimation(wk->mcss[index]);
    MCSS_GetScale(wk->mcss[index], &pos);
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
static void PokemonTradeDemoDown_FollowEmitterPath0(SPLEmitter *emitter, u32 type) {
    if (type != 0 && emitter != NULL) {
        PokemonTradeWork *wk = func_02050364(emitter);

        if (wk->work3D->emitterPaths[0] != NULL) {
            VecFx32 pos = { 0, 0, 0 };

            GFL_G3DCurveGetNowTranslation(wk->work3D->emitterPaths[0], &pos);
            func_02050208(emitter, &pos);
        }
    }
}

static void PokemonTradeDemoDown_AttachEmitterPath0(SPLEmitter *emitter) {
    func_0205035c(emitter, func_02050188());
    func_02050354(emitter, PokemonTradeDemoDown_FollowEmitterPath0);
}

// The emitter follows the second path
static void PokemonTradeDemoDown_FollowEmitterPath1(SPLEmitter *emitter, u32 type) {
    if (type != 0 && emitter != NULL) {
        PokemonTradeWork *wk = func_02050364(emitter);

        if (wk->work3D->emitterPaths[1] != NULL) {
            VecFx32 pos = { 0, 0, 0 };

            GFL_G3DCurveGetNowTranslation(wk->work3D->emitterPaths[1], &pos);
            func_02050208(emitter, &pos);
        }
    }
}

static void PokemonTradeDemoDown_AttachEmitterPath1(SPLEmitter *emitter) {
    func_0205035c(emitter, func_02050188());
    func_02050354(emitter, PokemonTradeDemoDown_FollowEmitterPath1);
}

void PokemonTradeDemoDown_Start(PokemonTradeWork *wk) {
    PokemonTradeDemoDown_StartBgmTimer(wk);
    wk->unk854 = TRUE;
    MCSS_SetAnimation(wk->mcss[0], 0);
    MCSS_Hide(wk->mcss[0]);
    MCSS_Hide(wk->mcss[1]);
    if (wk->type == 6) {
        VecFx32 pos = { 0, FX32_CONST(-23), FX32_CONST(10) };

        MCSS_SetPosition(wk->mcss[0], &pos);
    }
    func_ov194_021c5ff4(wk, 0x11);
    func_ov194_021c600c(wk, 0);
    PokemonTradeDemoDown_InitSide(wk, &wk->unk11ED, &wk->unk11EF, wk->partnerInfo, TRUE);
    PokemonTradeDemoDown_InitSide(wk, &wk->unk11EE, wk->unk11F0, wk->myInfo, FALSE);
    PokemonTrade_SetState(wk, PokemonTradeDemoDown_Init);
}

static void PokemonTradeDemoDown_Init(PokemonTradeWork *wk) {
    func_ov194_021bb4b4(wk);
    func_0204b758();
    func_ov194_021c2c44(wk);
    wk->clactUnit = func_0204bf1c(3, 0, wk->heapId);
    wk->work3D = GFL_HeapAllocate(wk->heapId, sizeof(PokemonTrade3DWork), TRUE, "pokemontrade_demo_down.c", 168);
    wk->work3D->heapId = wk->heapId;
    func_ov194_021c4088(wk, FALSE);
    reg_G2S_DB_BLDCNT = 0;
    reg_G2_BLDCNT = 0;
    func_ov194_021c200c(wk, 5);
    func_ov194_021c24cc(wk, 1, FALSE, PokemonTrade_GetPkm(wk, 1), TRUE);
    func_ov194_021c24cc(wk, 2, FALSE, PokemonTrade_GetPkm(wk, 0), TRUE);
    func_ov194_021c24cc(wk, 3, TRUE, PokemonTrade_GetPkm(wk, 1), TRUE);
    MCSS_Hide(wk->mcss[0]);
    MCSS_Hide(wk->mcss[1]);
    MCSS_Hide(wk->mcss[2]);
    MCSS_Hide(wk->mcss[3]);
    func_ov194_021c1d38(wk->work3D);
    func_ov194_021c1b74(wk->work3D, 3);
    func_ov194_021c1db4(wk->work3D);
    PokemonTradeDemoDown_SetTimer(wk, DOWN_START_FRAME);
    func_ov194_021be808(wk->timer);
    PokemonTrade_SetState(wk, PokemonTradeDemoDown_Wait1);
    GFL_BGSysSetEnabledBGsB(0x18);
}

static void PokemonTradeDemoDown_Wait1(PokemonTradeWork *wk) {
    func_ov194_021be808(wk->timer);
    func_ov194_021c1e08(wk->work3D);
    PokemonTrade_SetState(wk, PokemonTradeDemoDown_Wait2);
}

static void PokemonTradeDemoDown_Wait2(PokemonTradeWork *wk) {
    func_ov194_021be808(wk->timer);
    PokemonTrade_SetState(wk, PokemonTradeDemoDown_Run);
}

// The scene, a step for each frame of the timeline
static void PokemonTradeDemoDown_Run(PokemonTradeWork *wk) {
    VecFx32 pos1;
    VecFx32 pos2;
    VecFx32 pos3;
    VecFx32 pos4;
    VecFx32 pos5;
    VecFx32 pos6;
    VecFx32 pos7;
    VecFx32 pos8;

    func_ov194_021be808(wk->timer);
    if (wk->timer == PokemonTradeDemoDown_Frame(1)) {
        MCSS_StartFade(wk->mcss[0], 8, 8, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(2)) {
        MCSS_StartFade(wk->mcss[0], 16, 16, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(3)) {
        PokemonTradeDemoDown_LoadResources(wk);
        PokemonTradeDemoDown_CreateActor(wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(4)) {
        func_ov194_021c5ff4(wk, 0x11);
        func_ov194_021c600c(wk, 0x18);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(4)) {
        MCSS_Hide(wk->mcss[0]);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x356)) {
        func_0205007c(wk->work3D->systems[0], 10, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x65)) {
        func_0205007c(wk->work3D->systems[1], 1, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x9a)) {
        func_0205007c(wk->work3D->systems[2], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x356)) {
        func_0205007c(wk->work3D->systems[8], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0xe)) {
        func_0205007c(wk->work3D->systems[3], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x483)) {
        func_0205007c(wk->work3D->systems[7], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x9a)) {
        func_0205007c(wk->work3D->systems[4], 4, PokemonTradeDemoDown_AttachEmitterPath0, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x152)) {
        func_0205007c(wk->work3D->systems[5], 5, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x2bc)) {
        func_0205007c(wk->work3D->systems[6], 6, PokemonTradeDemoDown_AttachEmitterPath1, wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x47f)) {
        PokemonTradeDemoDown_LoadResources(wk);
        PokemonTradeDemoDown_CreateActor(wk);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x10a)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemoDown_Frame(20), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x12d)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemoDown_Frame(25), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x14a)) {
        MCSS_Show(wk->mcss[2]);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-150);
        pos1.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[2], &pos1);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-55);
        pos1.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[2], PokemonTradeDemoDown_Frame(15), &pos1, wk->heapId);
        MCSS_PauseAnimation(wk->mcss[2]);
        MCSS_GetScale(wk->mcss[2], &pos1);
        pos1.x *= 1.2;
        pos1.y *= 1.2;
        MCSS_SetScale(wk->mcss[2], &pos1);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x14b)) {
        MCSS_Show(wk->mcss[3]);
        pos2.x = FX32_CONST(60);
        pos2.y = FX32_CONST(80);
        pos2.z = FX32_CONST(-10);
        MCSS_SetPosition(wk->mcss[3], &pos2);
        pos2.y = FX32_CONST(5);
        pos2.x = FX32_CONST(60);
        pos2.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[3], PokemonTradeDemoDown_Frame(15), &pos2, wk->heapId);
        MCSS_PauseAnimation(wk->mcss[3]);
        MCSS_GetScale(wk->mcss[3], &pos2);
        pos2.x *= 1.2;
        pos2.y *= 1.2;
        MCSS_SetScale(wk->mcss[3], &pos2);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x154)) {
        pos3.x = FX32_CONST(-50);
        pos3.y = FX32_CONST(-55);
        pos3.z = FX32_CONST(10);
        TradeMcssMove_Start(wk->mcssMoves[0], PokemonTradeDemoDown_Frame(28), &pos3);
        pos3.x = FX32_CONST(60);
        pos3.y = FX32_CONST(5);
        pos3.z = FX32_CONST(-10);
        TradeMcssMove_Start(wk->mcssMoves[1], PokemonTradeDemoDown_Frame(28), &pos3);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x170)) {
        PokemonTradeDemoDown_FreeMove(wk, 0);
        PokemonTradeDemoDown_FreeMove(wk, 1);
        pos4.x = FX32_CONST(-150);
        pos4.y = FX32_CONST(-55);
        pos4.z = 0;
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[2], PokemonTradeDemoDown_Frame(46), &pos4, wk->heapId);
        pos4.x = FX32_CONST(150);
        pos4.y = FX32_CONST(5);
        pos4.z = 0;
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[3], PokemonTradeDemoDown_Frame(46), &pos4, wk->heapId);
        wk->mcssMoves[0]->bounce = 1;
        wk->mcssMoves[1]->bounce = 1;
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x1b2)) {
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        MCSS_Show(wk->mcss[0]);
        MCSS_StartFade(wk->mcss[0], 0, 0, 0, 0x7fff);
        pos5.x = FX32_CONST(-150);
        pos5.y = FX32_CONST(5);
        pos5.z = FX32_CONST(-10);
        MCSS_SetPosition(wk->mcss[0], &pos5);
        pos5.x = FX32_CONST(60);
        pos5.y = FX32_CONST(5);
        pos5.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeDemoDown_Frame(103), &pos5, wk->heapId);
        MCSS_GetScale(wk->mcss[0], &pos5);
        pos5.x *= 1.2;
        pos5.y *= 1.2;
        MCSS_SetScale(wk->mcss[0], &pos5);
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        MCSS_Show(wk->mcss[1]);
        pos6.x = FX32_CONST(150);
        pos6.y = FX32_CONST(-55);
        pos6.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[1], &pos6);
        pos6.x = FX32_CONST(-45);
        pos6.y = FX32_CONST(-55);
        pos6.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeDemoDown_Frame(103), &pos6, wk->heapId);
        MCSS_GetScale(wk->mcss[1], &pos6);
        pos6.x *= 1.2;
        pos6.y *= 1.2;
        MCSS_SetScale(wk->mcss[1], &pos6);
        wk->mcssMoves[0]->bounce = 1;
        wk->mcssMoves[1]->bounce = 1;
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x1f8)) {
        TradeMcssMove_Start(wk->mcssMoves[0], PokemonTradeDemoDown_Frame(128), &wk->mcssMoves[0]->end);
        TradeMcssMove_Start(wk->mcssMoves[1], PokemonTradeDemoDown_Frame(128), &wk->mcssMoves[1]->end);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x27d)) {
        PokemonTradeDemoDown_FreeMove(wk, 0);
        PokemonTradeDemoDown_FreeMove(wk, 1);
        pos7.x = FX32_CONST(60);
        pos7.y = FX32_CONST(100);
        pos7.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeDemoDown_Frame(7), &pos7, wk->heapId);
        pos7.x = FX32_CONST(-45);
        pos7.y = FX32_CONST(-160);
        pos7.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeDemoDown_Frame(7), &pos7, wk->heapId);
    }
    PokemonTradeDemoDown_Frame(0x48b);
    if (wk->timer == PokemonTradeDemoDown_Frame(0x48e)) {
        if (wk->unk11EE == 0) {
            PokemonTradeDemoDown_ShowSprite(wk, 3);
            PokemonTradeDemoDown_StartMove(wk, 3);
            MCSS_StartFade(wk->mcss[3], 16, 16, 0, 0x7fff);
        } else {
            PokemonTradeDemoDown_ShowSprite(wk, 3);
            MCSS_Hide(wk->mcss[3]);
            PokemonTradeDemoDown_ShowSprite(wk, 1);
            PokemonTradeDemoDown_StartMove(wk, 3);
            MCSS_StartFade(wk->mcss[1], 16, 16, 0, 0x7fff);
        }
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x48f)) {
        MCSS_StartFade(wk->mcss[3], 8, 8, 0, 0x7fff);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x490)) {
        MCSS_StartFade(wk->mcss[3], 0, 0, 0, 0x7fff);
    }
    if (PokemonTradeDemoDown_Frame(0x48e) < wk->timer && wk->unk11EE != 0 && wk->mcssMoves[3] != NULL &&
        wk->mcssMoves[3]->frame == 9) {
        MCSS_GetPosition(wk->mcss[1], &pos8);
        MCSS_SetPosition(wk->mcss[3], &pos8);
        MCSS_Show(wk->mcss[3]);
        MCSS_Hide(wk->mcss[1]);
        wk->mcssMoves[3]->mcss = wk->mcss[3];
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x4a0)) {
        MCSS_ResumeAnimation(wk->mcss[3]);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x28b)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemoDown_Frame(10), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x2b5)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemoDown_Frame(25), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x380)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemoDown_Frame(30), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x3ca)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemoDown_Frame(30), TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemoDown_Frame(0x4c1)) {
        PokemonTradeDemoDown_FreeMove(wk, 0);
        PokemonTradeDemoDown_FreeMove(wk, 1);
        func_ov194_021c2034(wk);
        PokemonTrade_SetState(wk, PokemonTradeDemoDown_End);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
    TradeMcssMove_Update(wk->mcssMoves[2], wk);
    TradeMcssMove_Update(wk->mcssMoves[3], wk);
}
