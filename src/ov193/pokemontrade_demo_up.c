#include "types.h"
#include "app/pokemon_trade_local.h"
#include "app/pokemontrade_demo_common.h"
#include "app/pokemontrade_demo_up.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "system/mcss.h"
#include "system/wipe.h"

// A guessed name: the scene of the trade demo whose steps the other two files copy

static void PokemonTradeDemoUp_FreeEffects(PokemonTradeWork *wk);
static void PokemonTradeDemoUp_SetUpScene(PokemonTradeWork *wk);
static void PokemonTradeDemoUp_StateRun(PokemonTradeWork *wk);

void PokemonTradeDemoUp_Start(PokemonTradeWork *wk) {
    func_ov194_021bb4b4(wk);
    ClActSys_Delete();
    func_ov194_021c2c44(wk);
    wk->clactUnit = ClActUnit_Create(3, 0, wk->heapId);
    wk->work3D = GFL_HeapAllocate(wk->heapId, sizeof(PokemonTrade3DWork), TRUE, "pokemontrade_demo_up.c", 152);
    wk->work3D->heapId = wk->heapId;
    func_ov194_021c4088(wk, FALSE);
    *(vu16 *)REG_DB_BLDCNT_ADDR = 0;
    *(vu16 *)REG_BLDCNT_ADDR = 0;
    func_ov194_021c200c(wk, 3);
    func_ov194_021c24cc(wk, 1, FALSE, PokemonTrade_GetPkm(wk, 1), TRUE);
    func_ov194_021c24cc(wk, 2, FALSE, PokemonTrade_GetPkm(wk, 0), TRUE);
    func_ov194_021c24cc(wk, 3, TRUE, PokemonTrade_GetPkm(wk, 1), TRUE);
    MCSS_Hide(wk->mcss[1]);
    MCSS_Hide(wk->mcss[2]);
    MCSS_Hide(wk->mcss[3]);
    func_ov194_021c1d38(wk->work3D);
    func_ov194_021c1db4(wk->work3D);
    func_ov194_021c1b74(wk->work3D, TRUE);
    PokemonTradeDemo_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeDemoUp_FreeEffects);
}

static void PokemonTradeDemoUp_FreeEffects(PokemonTradeWork *wk) {
    func_ov194_021c1e08(wk->work3D);
    PokemonTrade_SetState(wk, PokemonTradeDemoUp_SetUpScene);
}

static void PokemonTradeDemoUp_SetUpScene(PokemonTradeWork *wk) {
    func_ov194_021c2034(wk);
    func_ov194_021c200c(wk, 3);
    PokemonTrade_SetState(wk, PokemonTradeDemoUp_StateRun);
}

static void PokemonTradeDemoUp_StateRun(PokemonTradeWork *wk) {
    VecFx32 pos1;
    VecFx32 pos2;
    VecFx32 pos3;
    VecFx32 pos4;
    VecFx32 pos5;
    VecFx32 pos6;
    VecFx32 pos7;
    VecFx32 pos8;

    func_ov194_021be808(wk->timer);
    if (wk->timer == PokemonTradeDemo_Frames(1)) {
        MCSS_StartFade(wk->mcss[0], 8, 8, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeDemo_Frames(2)) {
        MCSS_StartFade(wk->mcss[0], 16, 16, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeDemo_Frames(3)) {
        PokemonTradeDemo_LoadResources(wk);
        PokemonTradeDemo_ShowActor(wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(4)) {
        func_ov194_021c5ff4(wk, 0x11);
        func_ov194_021c600c(wk, 0x18);
    }
    if (wk->timer == PokemonTradeDemo_Frames(4)) {
        MCSS_Hide(wk->mcss[0]);
    }
    if (wk->timer == PokemonTradeDemo_Frames(854)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[0], 10, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(101)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[1], 1, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(154)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[2], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(854)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[8], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(14)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[3], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(1155)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[7], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(154)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[4], 4, PokemonTradeDemo_InitEmitter0, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(338)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[5], 5, NULL, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(700)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[6], 6, PokemonTradeDemo_InitEmitter1, wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(1151)) {
        PokemonTradeDemo_ShowActor(wk);
    }
    if (wk->timer == PokemonTradeDemo_Frames(266)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemo_Frames(20), 1, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemo_Frames(301) - 2) {
        if (wk->mcssMoves[0] != NULL) {
            GFL_HeapFree(wk->mcssMoves[0]);
            wk->mcssMoves[0] = NULL;
        }
        if (wk->mcssMoves[1] != NULL) {
            GFL_HeapFree(wk->mcssMoves[1]);
            wk->mcssMoves[1] = NULL;
        }
        func_ov194_021c2034(wk);
        PokemonTrade_SetState(wk, PokemonTradeDemo_Finish);
    }
    if (wk->timer == PokemonTradeDemo_Frames(330)) {
        MCSS_Show(wk->mcss[2]);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-150);
        pos1.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[2], &pos1);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-55);
        pos1.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[2], PokemonTradeDemo_Frames(15), &pos1, wk->heapId);
        MCSS_PauseAnimation(wk->mcss[2]);
        MCSS_GetScale(wk->mcss[2], &pos1);
        pos1.x *= 1.2;
        pos1.y *= 1.2;
        MCSS_SetScale(wk->mcss[2], &pos1);
    }
    if (wk->timer == PokemonTradeDemo_Frames(331)) {
        MCSS_Show(wk->mcss[3]);
        pos2.x = FX32_CONST(60);
        pos2.y = FX32_CONST(80);
        pos2.z = FX32_CONST(-10);
        MCSS_SetPosition(wk->mcss[3], &pos2);
        pos2.y = FX32_CONST(5);
        pos2.x = FX32_CONST(60);
        pos2.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[3], PokemonTradeDemo_Frames(15), &pos2, wk->heapId);
        MCSS_PauseAnimation(wk->mcss[3]);
        MCSS_GetScale(wk->mcss[3], &pos2);
        pos2.x *= 1.2;
        pos2.y *= 1.2;
        MCSS_SetScale(wk->mcss[3], &pos2);
    }
    if (wk->timer == PokemonTradeDemo_Frames(340)) {
        pos3.x = FX32_CONST(-50);
        pos3.y = FX32_CONST(-55);
        pos3.z = FX32_CONST(10);
        TradeMcssMove_Start(wk->mcssMoves[0], PokemonTradeDemo_Frames(28), &pos3);
        pos3.x = FX32_CONST(60);
        pos3.y = FX32_CONST(5);
        pos3.z = FX32_CONST(-10);
        TradeMcssMove_Start(wk->mcssMoves[1], PokemonTradeDemo_Frames(28), &pos3);
    }
    if (wk->timer == PokemonTradeDemo_Frames(368)) {
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        pos4.x = FX32_CONST(-150);
        pos4.y = FX32_CONST(-55);
        pos4.z = 0;
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[2], PokemonTradeDemo_Frames(46), &pos4, wk->heapId);
        pos4.x = FX32_CONST(150);
        pos4.y = FX32_CONST(5);
        pos4.z = 0;
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[3], PokemonTradeDemo_Frames(46), &pos4, wk->heapId);
        wk->mcssMoves[0]->bounce = 1;
        wk->mcssMoves[1]->bounce = 1;
    }
    if (wk->timer == PokemonTradeDemo_Frames(434)) {
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
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeDemo_Frames(103), &pos5, wk->heapId);
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
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeDemo_Frames(103), &pos6, wk->heapId);
        MCSS_GetScale(wk->mcss[1], &pos6);
        pos6.x *= 1.2;
        pos6.y *= 1.2;
        MCSS_SetScale(wk->mcss[1], &pos6);
        wk->mcssMoves[0]->bounce = 1;
        wk->mcssMoves[1]->bounce = 1;
    }
    if (wk->timer == PokemonTradeDemo_Frames(504)) {
        TradeMcssMove_Start(wk->mcssMoves[0], PokemonTradeDemo_Frames(128), &wk->mcssMoves[0]->end);
        TradeMcssMove_Start(wk->mcssMoves[1], PokemonTradeDemo_Frames(128), &wk->mcssMoves[1]->end);
    }
    if (wk->timer == PokemonTradeDemo_Frames(637)) {
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        pos7.x = FX32_CONST(60);
        pos7.z = FX32_CONST(-10);
        pos7.y = FX32_CONST(100);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[0], PokemonTradeDemo_Frames(7), &pos7, wk->heapId);
        pos7.x = FX32_CONST(-45);
        pos7.y = FX32_CONST(-160);
        pos7.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[1], PokemonTradeDemo_Frames(7), &pos7, wk->heapId);
    }
    PokemonTradeDemo_Frames(1163);
    if (wk->timer == PokemonTradeDemo_Frames(1166)) {
        MCSS_Show(wk->mcss[3]);
        pos8.x = 0;
        pos8.y = FX32_CONST(-23);
        pos8.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[3], &pos8);
        MCSS_PauseAnimation(wk->mcss[3]);
        MCSS_StartFade(wk->mcss[3], 16, 16, 0, 0x7fff);
    }
    if (wk->timer == PokemonTradeDemo_Frames(1167)) {
        MCSS_StartFade(wk->mcss[3], 8, 8, 0, 0x7fff);
    }
    if (wk->timer == PokemonTradeDemo_Frames(1168)) {
        MCSS_StartFade(wk->mcss[3], 0, 0, 0, 0x7fff);
    }
    if (wk->timer == PokemonTradeDemo_Frames(651)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemo_Frames(10), 1, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemo_Frames(693)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemo_Frames(25), 1, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemo_Frames(896)) {
        GFL_WipeSet(0, 0, 0, 0x7fff, PokemonTradeDemo_Frames(30), 1, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemo_Frames(970)) {
        GFL_WipeSet(0, 1, 1, 0x7fff, PokemonTradeDemo_Frames(30), 1, wk->heapId);
    }
    if (wk->timer == PokemonTradeDemo_Frames(1217)) {
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        func_ov194_021c2034(wk);
        PokemonTrade_SetState(wk, PokemonTradeDemo_Finish);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
}
