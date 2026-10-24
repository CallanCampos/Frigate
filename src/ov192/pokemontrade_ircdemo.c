#include "types.h"
#include "app/pokemontrade_ircdemo.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/mcss.h"
#include "system/wipe.h"

// pokemontrade_ircdemo.c, as the ROM's string has it: the scene of the trade demo of a trade over the infrared port,
// whose sprites appear in front of the effects after the common file's steps

static void PokemonTradeIrcDemo_Run(PokemonTradeWork *wk);

void PokemonTradeIrcDemo_Init(PokemonTradeWork *wk) {
    PokemonTrade_DeleteAllResources(wk);
    ClActSys_Delete();
    Poketrade2D_InitClactSystemMain(wk);
    wk->clactUnit = ClActUnit_Create(3, 0, wk->heapId);
    wk->work3D = GFL_HeapAllocate(wk->heapId, sizeof(PokemonTrade3DWork), TRUE, "pokemontrade_ircdemo.c", 101);
    wk->work3D->heapId = wk->heapId;
    Poketrade2D_SetupSummaryBgs(wk, TRUE);
    reg_G2S_DB_BLDCNT = 0;
    reg_G2_BLDCNT = 0;
    Poketrade3D_AddUnitAndInit(wk, 2);
    Poketrade3D_AddPokemonMcssPlain(wk, 1, FALSE, PokemonTrade_GetPkm(wk, 1), TRUE);
    Poketrade3D_AddPokemonMcssPlain(wk, 2, FALSE, PokemonTrade_GetPkm(wk, 0), TRUE);
    Poketrade3D_AddPokemonMcssPlain(wk, 3, TRUE, PokemonTrade_GetPkm(wk, 1), TRUE);
    MCSS_GetScale(wk->mcss[3], &wk->work3D->ircSpriteSizes[1]);
    MCSS_Hide(wk->mcss[1]);
    MCSS_Hide(wk->mcss[2]);
    MCSS_Hide(wk->mcss[3]);
    Poketrade3D_InitTradeParticleSet(wk->work3D);
    Poketrade3D_CreateIcaStreamsEnding(wk->work3D);
    PokemonTradeIrcDemo_SetTimer(wk, 0);
    PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Run);
}

// The scene, a step for each frame of the timeline
static void PokemonTradeIrcDemo_Run(PokemonTradeWork *wk) {
    VecFx32 pos1;
    VecFx32 pos2;
    VecFx32 size;
    VecFx32 pos3;

    if (wk->timer == PokemonTradeIrcDemo_Frames(1)) {
        MCSS_StartFade(wk->mcss[0], 8, 8, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(2)) {
        MCSS_StartFade(wk->mcss[0], 16, 16, 0, 0xffff);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(3)) {
        PokemonTradeIrcDemo_LoadResources(wk);
        PokemonTradeIrcDemo_ShowActor(wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(4)) {
        Poketrade2D_ScheduleMainPlaneVisible(wk, 0x11);
        Poketrade2D_ScheduleSubPlaneVisible(wk, 0x18);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(4)) {
        MCSS_Hide(wk->mcss[0]);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(-1)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[0], 0, NULL, wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x15e)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[1], 0, NULL, wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0xc8)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[2], 1, NULL, wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x14)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[3], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x258)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[4], 2, NULL, wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(-1)) {
        Particle_CreateEmitterWithCallback(wk->work3D->systems[5], 3, NULL, wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x256)) {
        PokemonTradeIrcDemo_ShowActor(wk);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x82)) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0x7fff, PokemonTradeIrcDemo_Frames(20),
                    TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0xb4)) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0x7fff, PokemonTradeIrcDemo_Frames(25), TRUE,
                    wk->heapId);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0xfa)) {
        MCSS_Show(wk->mcss[2]);
        pos1.x = FX32_CONST(-50);
        pos1.y = FX32_CONST(-500);
        pos1.z = FX32_CONST(10);
        MCSS_SetPosition(wk->mcss[2], &pos1);
        pos1.y = FX32_CONST(300);
        pos1.x = FX32_CONST(-50);
        pos1.z = FX32_CONST(10);
        TradeMcssMove_Free(wk->mcssMoves[0]);
        wk->mcssMoves[0] = TradeMcssMove_Create(wk->mcss[2], PokemonTradeIrcDemo_Frames(150), &pos1, wk->heapId);
        MCSS_GetScale(wk->mcss[2], &pos1);
        pos1.x *= 2;
        pos1.y *= 2;
        MCSS_SetScale(wk->mcss[2], &pos1);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0xfb)) {
        MCSS_Show(wk->mcss[3]);
        pos2.x = FX32_CONST(60);
        pos2.y = FX32_CONST(300);
        pos2.z = FX32_CONST(-10);
        MCSS_SetPosition(wk->mcss[3], &pos2);
        pos2.x = FX32_CONST(60);
        pos2.y = FX32_CONST(-370);
        pos2.z = FX32_CONST(-10);
        TradeMcssMove_Free(wk->mcssMoves[1]);
        wk->mcssMoves[1] = TradeMcssMove_Create(wk->mcss[3], PokemonTradeIrcDemo_Frames(150), &pos2, wk->heapId);
        MCSS_SetScale(wk->mcss[3], &wk->work3D->ircSpriteSizes[1]);
        MCSS_GetScale(wk->mcss[3], &pos2);
        wk->work3D->ircSpriteSizes[0].x = pos2.x;
        wk->work3D->ircSpriteSizes[0].y = pos2.y;
        wk->work3D->ircSpriteSizes[0].z = pos2.z;
        MCSS_GetScale(wk->mcss[3], &size);
    }
    PokemonTradeIrcDemo_Frames(0xfc);
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x267)) {
        MCSS *shown;

        if (wk->unk11EE == 0) {
            shown = wk->mcss[3];
        } else {
            PokemonTradeIrcDemo_ShowSprite(wk->mcss[3], wk);
            MCSS_Hide(wk->mcss[3]);
            shown = wk->mcss[1];
        }
        PokemonTradeIrcDemo_ShowSprite(shown, wk);
        PokemonTradeIrcDemo_StartMove(wk, 3);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x268)) {
        MCSS_StartFade(wk->mcss[3], 8, 8, 0, 0x7fff);
        MCSS_StartFade(wk->mcss[1], 8, 8, 0, 0x7fff);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x269)) {
        MCSS_StartFade(wk->mcss[3], 0, 0, 0, 0x7fff);
        MCSS_StartFade(wk->mcss[1], 0, 0, 0, 0x7fff);
    }
    if (PokemonTradeIrcDemo_Frames(0x267) < wk->timer && wk->unk11EE != 0 && wk->mcssMoves[3] != NULL &&
        wk->mcssMoves[3]->frame == 9) {
        MCSS_GetPosition(wk->mcss[1], &pos3);
        MCSS_SetPosition(wk->mcss[3], &pos3);
        MCSS_Show(wk->mcss[3]);
        MCSS_Hide(wk->mcss[1]);
        wk->mcssMoves[3]->mcss = wk->mcss[3];
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x279)) {
        MCSS_ResumeAnimation(wk->mcss[3]);
    }
    PoketradeNego_PlaySeByKeyB(wk->timer);
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x199)) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0x7fff, PokemonTradeIrcDemo_Frames(10),
                    TRUE, wk->heapId);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x1a4)) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0x7fff, PokemonTradeIrcDemo_Frames(25), TRUE,
                    wk->heapId);
    }
    if (wk->timer == PokemonTradeIrcDemo_Frames(0x293)) {
        GFL_HeapFree(wk->mcssMoves[0]);
        wk->mcssMoves[0] = NULL;
        GFL_HeapFree(wk->mcssMoves[1]);
        wk->mcssMoves[1] = NULL;
        Poketrade3D_RemoveCurrentUnit(wk);
        PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Finish);
    }
    TradeMcssMove_Update(wk->mcssMoves[0], wk);
    TradeMcssMove_Update(wk->mcssMoves[1], wk);
    TradeMcssMove_Update(wk->mcssMoves[2], wk);
    TradeMcssMove_Update(wk->mcssMoves[3], wk);
}
